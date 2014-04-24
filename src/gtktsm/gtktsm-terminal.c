/*
 * GtkTsm - Terminal Emulator
 *
 * Copyright (c) 2011-2014 David Herrmann <dh.herrmann@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define G_LOG_DOMAIN "GtkTsm"

#include <cairo.h>
#include <errno.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libtsm.h>
#include <math.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <xkbcommon/xkbcommon.h>
#include "gtktsm-terminal.h"
#include "shl-htable.h"
#include "shl-llog.h"
#include "shl-macro.h"
#include "shl-pty.h"

/*
 * Glyph Renderer
 * With terminal-emulators, we have the problem that our grid is fixed.
 * Existing text-renderers want to apply kerning and other heuristics to
 * render fonts properly. We cannot do that. Therefore, we render each glyph
 * separately and provide them as single-glyph objects to the upper layers.
 *
 * We also do some heuristics to calculate the real font-metrics. Most fonts
 * are not mono-space, so they don't provide any generic metrics. We therefore
 * render the ASCII glyphs and some more one-column glyphs to get a proper
 * global metric for the font.
 */

struct gtktsm_font {
	unsigned long ref;
	PangoFontMap *map;
};

struct gtktsm_face {
	unsigned long ref;
	struct gtktsm_font *font;
	PangoContext *ctx;
	cairo_antialias_t aa;
	cairo_subpixel_order_t subpixel;

	struct shl_htable glyphs;
	unsigned int width;
	unsigned int height;
	unsigned int baseline;
	unsigned int line_thickness;
	unsigned int underline_pos;
	unsigned int strikethrough_pos;
};

enum gtktsm_glyph_format {
	GTKTSM_GLYPH_INVALID,
	GTKTSM_GLYPH_A1,
	GTKTSM_GLYPH_A8,
	GTKTSM_GLYPH_XRGB32,
};

struct gtktsm_glyph {
	unsigned long id;

	unsigned int cwidth;
	unsigned int format;
	unsigned int width;
	int stride;
	unsigned int height;
	uint8_t *buffer;
	cairo_surface_t *surface;
};

#define gtktsm_glyph_from_id(_id) \
	shl_htable_offsetof((_id), struct gtktsm_glyph, id)

static void gtktsm_face_free(struct gtktsm_face *face);
static void gtktsm_glyph_free(struct gtktsm_glyph *glyph);

static int gtktsm_font_new(struct gtktsm_font **out)
{
	_shl_free_ struct gtktsm_font *font = NULL;

	if (!out)
		return -EINVAL;

	font = calloc(1, sizeof(*font));
	if (!font)
		return -ENOMEM;

	font->ref = 1;

	font->map = pango_cairo_font_map_get_default();
	if (font->map) {
		g_object_ref(font->map);
	} else {
		font->map = pango_cairo_font_map_new();
		if (!font->map)
			return -ENOMEM;
	}

	*out = font;
	font = NULL;
	return 0;
}

static void gtktsm_font_ref(struct gtktsm_font *font)
{
	if (!font || !font->ref)
		return;

	++font->ref;
}

static void gtktsm_font_unref(struct gtktsm_font *font)
{
	if (!font || !font->ref || --font->ref)
		return;

	g_object_unref(font->map);
	free(font);
}

static void init_pango_desc(PangoFontDescription *desc,
			    int desc_size,
			    int desc_bold,
			    int desc_italic)
{
	PangoFontMask mask;
	int v;

	if (desc_size >= 0) {
		v = desc_size * PANGO_SCALE;
		if (desc_size > 0 && v > 0)
			pango_font_description_set_absolute_size(desc, v);
	}

	if (desc_bold >= 0) {
		v = desc_bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL;
		pango_font_description_set_weight(desc, v);
	}

	if (desc_italic >= 0) {
		v = desc_italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL;
		pango_font_description_set_style(desc, v);
	}

	pango_font_description_set_variant(desc, PANGO_VARIANT_NORMAL);
	pango_font_description_set_stretch(desc, PANGO_STRETCH_NORMAL);
	pango_font_description_set_gravity(desc, PANGO_GRAVITY_SOUTH);

	mask = pango_font_description_get_set_fields(desc);

	if (!(mask & PANGO_FONT_MASK_FAMILY))
		pango_font_description_set_family(desc, "monospace");
	if (!(mask & PANGO_FONT_MASK_WEIGHT))
		pango_font_description_set_weight(desc, PANGO_WEIGHT_NORMAL);
	if (!(mask & PANGO_FONT_MASK_STYLE))
		pango_font_description_set_style(desc, PANGO_STYLE_NORMAL);
	if (!(mask & PANGO_FONT_MASK_SIZE))
		pango_font_description_set_size(desc, 10 * PANGO_SCALE);
}

static void measure_pango(struct gtktsm_face *face)
{
	static const char str[] = "abcdefghijklmnopqrstuvwxyz"
				  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				  "@!\"$%&/()=?\\}][{°^~+*#'<>|-_.:,;`´";
	static const size_t str_len = sizeof(str) - 1;
	PangoLayout *layout;
	PangoRectangle rec;
	unsigned int thick, upos, spos;

	/*
	 * There is no way to check whether a font is a monospace font.
	 * Moreover, there is no "monospace extents" field of fonts that we can
	 * use to calculate a suitable cell size. Any bounding-boxes provided
	 * by the fonts are mostly useless for cell-size computations.
	 * Therefore, we simply render a bunch of ASCII characters and compute
	 * the cell-size from these. If you passed a monospace font, it will
	 * work out greatly. If you passed some other font, you will get a
	 * suitable tradeoff (well, don't do that..).
	 */

	layout = pango_layout_new(face->ctx);

	pango_layout_set_height(layout, 0);
	pango_layout_set_spacing(layout, 0);
	pango_layout_set_text(layout, str, str_len);
	pango_layout_get_pixel_extents(layout, NULL, &rec);

	/* We use an example layout to render a bunch of ASCII characters in a
	 * single line. The height and baseline of the resulting extents can be
	 * copied unchanged into the face. For the width we calculate the
	 * average (rounding up). */
	face->width = (rec.width + (str_len - 1)) / str_len;
	face->height = rec.height;
	face->baseline = PANGO_PIXELS_CEIL(pango_layout_get_baseline(layout));

	/* heuristics to calculate underline/strikethrough positions */
	thick = shl_min((face->height - face->baseline) / 2, face->height / 14);
	thick = shl_max(thick, 1U);
	upos = shl_min(face->baseline + thick, face->height - thick);
	spos = face->baseline - face->height / 4;

	face->line_thickness = thick;
	face->underline_pos = upos;
	face->strikethrough_pos = spos;

	g_object_unref(layout);
}

static int init_pango(struct gtktsm_face *face,
		      const char *desc_str,
		      int desc_size,
		      int desc_bold,
		      int desc_italic,
		      cairo_antialias_t aa,
		      cairo_subpixel_order_t subpixel)
{
	PangoFontDescription *desc;
	cairo_font_options_t *options;

	/* set context options */
	pango_context_set_base_dir(face->ctx, PANGO_DIRECTION_LTR);
	pango_context_set_language(face->ctx, pango_language_get_default());

	/* set font description */
	desc = pango_font_description_from_string(desc_str);
	init_pango_desc(desc, desc_size, desc_bold, desc_italic);
	pango_context_set_font_description(face->ctx, desc);
	pango_font_description_free(desc);

	/* set anti-aliasing */
	options = cairo_font_options_create();
	if (cairo_font_options_status(options) != CAIRO_STATUS_SUCCESS)
		return -ENOMEM;

	cairo_font_options_set_antialias(options, aa);
	cairo_font_options_set_subpixel_order(options, subpixel);
	pango_cairo_context_set_font_options(face->ctx, options);
	cairo_font_options_destroy(options);

	/* measure font */
	measure_pango(face);

	if (!face->width || !face->height)
		return -EINVAL;

	return 0;
}

static int gtktsm_face_new(struct gtktsm_face **out,
			   struct gtktsm_font *font,
			   const char *desc_str,
			   int desc_size,
			   int desc_bold,
			   int desc_italic,
			   cairo_antialias_t aa,
			   cairo_subpixel_order_t subpixel)
{
	struct gtktsm_face *face;
	int r;

	if (!out || !font || !desc_str)
		return -EINVAL;

	face = calloc(1, sizeof(*face));
	if (!face)
		return -ENOMEM;

	face->font = font;
	gtktsm_font_ref(face->font);
	shl_htable_init_ulong(&face->glyphs);
	face->ctx = pango_font_map_create_context(font->map);
	face->aa = aa;
	face->subpixel = subpixel;

	r = init_pango(face,
		       desc_str,
		       desc_size,
		       desc_bold,
		       desc_italic,
		       aa,
		       subpixel);
	if (r < 0)
		goto error;

	*out = face;
	return 0;

error:
	gtktsm_face_free(face);
	return r;
}

static void free_glyph(unsigned long *elem, void *ctx)
{
	gtktsm_glyph_free(gtktsm_glyph_from_id(elem));
}

static void gtktsm_face_free(struct gtktsm_face *face)
{
	if (!face)
		return;

	if (face->ctx)
		g_object_unref(face->ctx);
	shl_htable_clear_ulong(&face->glyphs, free_glyph, NULL);
	gtktsm_font_unref(face->font);
	free(face);
}

static unsigned int c2f(cairo_format_t format)
{
	switch (format) {
	case CAIRO_FORMAT_A1:
		return GTKTSM_GLYPH_A1;
	case CAIRO_FORMAT_A8:
		return GTKTSM_GLYPH_A8;
	case CAIRO_FORMAT_RGB24:
		return GTKTSM_GLYPH_XRGB32;
	default:
		return GTKTSM_GLYPH_INVALID;
	}
}

static int create_glyph(struct gtktsm_face *face,
			struct gtktsm_glyph *glyph,
			const uint32_t *ch,
			size_t len)
{
	PangoLayoutLine *line;
	cairo_surface_t *surface;
	PangoRectangle rec;
	cairo_format_t format;
	PangoLayout *layout;
	cairo_t *cr;
	size_t cnt;
	glong ulen;
	char *val;
	int r;

	switch (face->aa) {
	case CAIRO_ANTIALIAS_NONE:
		format = CAIRO_FORMAT_A1;
		break;
	case CAIRO_ANTIALIAS_GRAY:
		format = CAIRO_FORMAT_A8;
		break;
	case CAIRO_ANTIALIAS_SUBPIXEL:
		/* fallthrough */
	default:
		format = CAIRO_FORMAT_RGB24;
		break;
	}

	glyph->format = c2f(format);
	glyph->width = face->width * glyph->cwidth;
	glyph->stride = cairo_format_stride_for_width(format, glyph->width);
	glyph->height = face->height;

	glyph->buffer = calloc(1, glyph->stride * glyph->height);
	if (!glyph->buffer)
		return -ENOMEM;

	surface = cairo_image_surface_create_for_data(glyph->buffer,
						      format,
						      glyph->width,
						      glyph->height,
						      glyph->stride);
	if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
		r = -ENOMEM;
		goto err_buffer;
	}

	cr = cairo_create(surface);
	if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
		r = -ENOMEM;
		goto err_surface;
	}

	pango_cairo_update_context(cr, face->ctx);
	layout = pango_layout_new(face->ctx);

	val = g_ucs4_to_utf8(ch, len, NULL, &ulen, NULL);
	if (!val) {
		r = -ERANGE;
		goto err_layout;
	}

	/* render one line only */
	pango_layout_set_height(layout, 0);
	/* no line spacing */
	pango_layout_set_spacing(layout, 0);
	/* set text to char [+combining-chars] */
	pango_layout_set_text(layout, val, ulen);

	g_free(val);

	cnt = pango_layout_get_line_count(layout);
	if (cnt == 0) {
		r = -ERANGE;
		goto err_layout;
	}

	line = pango_layout_get_line_readonly(layout, 0);
	pango_layout_line_get_pixel_extents(line, NULL, &rec);

	cairo_move_to(cr, -rec.x, face->baseline),
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	pango_cairo_show_layout_line(cr, line);

	g_object_unref(layout);
	cairo_destroy(cr);
	glyph->surface = surface;
	return 0;

err_layout:
	g_object_unref(layout);
	cairo_destroy(cr);
err_surface:
	cairo_surface_destroy(surface);
err_buffer:
	free(glyph->buffer);
	glyph->buffer = NULL;
	return r;
}

static int gtktsm_face_render(struct gtktsm_face *face,
			      struct gtktsm_glyph **out,
			      unsigned long id,
			      const uint32_t *ch,
			      size_t len,
			      size_t cwidth)
{
	struct gtktsm_glyph *glyph;
	unsigned long *gid;
	bool b;
	int r;

	if (!face || !out)
		return -EINVAL;

	b = shl_htable_lookup_ulong(&face->glyphs, id, &gid);
	if (b) {
		*out = gtktsm_glyph_from_id(gid);
		return 0;
	}

	if (!len || !ch || !cwidth)
		return -EINVAL;

	glyph = calloc(1, sizeof(*glyph));
	if (!glyph)
		return -ENOMEM;

	glyph->id = id;
	glyph->cwidth = cwidth;

	r = create_glyph(face, glyph, ch, len);
	if (r < 0)
		goto error;

	r = shl_htable_insert_ulong(&face->glyphs, &glyph->id);
	if (r < 0)
		goto error;

	*out = glyph;
	return 0;

error:
	gtktsm_glyph_free(glyph);
	return r;
}

static void gtktsm_glyph_free(struct gtktsm_glyph *glyph)
{
	if (glyph->surface)
		cairo_surface_destroy(glyph->surface);
	free(glyph->buffer);
	free(glyph);
}

/*
 * Cell Renderer
 * Gtk uses cairo for rendering. Unfortunately, cairo isn't very suitable for
 * terminal/cell rendering. Rendering each glyph separately causes like 10
 * function calls per cell. Therefore, we render our terminal into a shadow
 * buffer and only tell cairo to blit it onto the widget-buffer.
 */

struct gtktsm_renderer {
	unsigned int width;
	unsigned int height;
	int stride;
	uint8_t *data;
	cairo_surface_t *surface;
	tsm_age_t age;
};

struct gtktsm_renderer_ctx {
	struct gtktsm_renderer *rend;
	cairo_t *cr;

	struct tsm_screen *screen;
	struct tsm_vte *vte;
	struct gtktsm_face *face_regular;
	struct gtktsm_face *face_bold;
	struct gtktsm_face *face_italic;
	struct gtktsm_face *face_bold_italic;
	unsigned int cell_width;
	unsigned int cell_height;

	bool debug;
};

static int renderer_realloc(struct gtktsm_renderer *rend,
			    unsigned int width,
			    unsigned int height)
{
	int stride;
	uint8_t *data;
	cairo_surface_t *surface;

	if (!width)
		width = 1;
	if (!height)
		height = 1;

	stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
	data = malloc(abs(stride * height));
	if (!data)
		return -ENOMEM;

	surface = cairo_image_surface_create_for_data(data,
						      CAIRO_FORMAT_ARGB32,
						      width,
						      height,
						      stride);
	if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(surface);
		free(data);
		return -ENOMEM;
	}

	if (rend->data) {
		cairo_surface_destroy(rend->surface);
		free(rend->data);
	}

	rend->width = width;
	rend->height = height;
	rend->stride = stride;
	rend->data = data;
	rend->surface = surface;
	rend->age = 0;

	return 0;
}

static int gtktsm_renderer_new(struct gtktsm_renderer **out,
			       unsigned int width,
			       unsigned int height)
{
	struct gtktsm_renderer *rend;
	int r;

	if (!out)
		return -EINVAL;

	rend = calloc(1, sizeof(*rend));
	if (!rend)
		return -ENOMEM;

	r = renderer_realloc(rend, width, height);
	if (r < 0)
		goto err_rend;

	*out = rend;
	return 0;

err_rend:
	free(rend);
	return r;
}

static void gtktsm_renderer_free(struct gtktsm_renderer *rend)
{
	if (!rend)
		return;

	cairo_surface_destroy(rend->surface);
	free(rend->data);
	free(rend);
}

static int gtktsm_renderer_resize(struct gtktsm_renderer *rend,
				  unsigned int width,
				  unsigned int height)
{
	if (!rend)
		return -EINVAL;

	return renderer_realloc(rend, width, height);
}

static void renderer_fill(struct gtktsm_renderer *rend,
			  unsigned int x,
			  unsigned int y,
			  unsigned int width,
			  unsigned int height,
			  uint8_t br, uint8_t bg, uint8_t bb)
{
	unsigned int i, tmp;
	uint8_t *dst;
	uint32_t out;

	/* clip width */
	tmp = x + width;
	if (tmp <= x || x >= rend->width)
		return;
	if (tmp > rend->width)
		width = rend->width - x;

	/* clip height */
	tmp = y + height;
	if (tmp <= y || y >= rend->height)
		return;
	if (tmp > rend->height)
		height = rend->height - y;

	/* prepare */
	dst = rend->data;
	dst = &dst[y * rend->stride + x * 4];
	out = (0xff << 24) | (br << 16) | (bg << 8) | bb;

	/* fill buffer */
	while (height--) {
		for (i = 0; i < width; ++i)
			((uint32_t*)dst)[i] = out;

		dst += rend->stride;
	}
}

/* used for debugging; draws a border on the given rectangle */
static void renderer_highlight(struct gtktsm_renderer *rend,
			       unsigned int x,
			       unsigned int y,
			       unsigned int width,
			       unsigned int height)
{
	unsigned int i, j, tmp;
	uint8_t *dst;
	uint32_t out;

	/* clip width */
	tmp = x + width;
	if (tmp <= x || x >= rend->width)
		return;
	if (tmp > rend->width)
		width = rend->width - x;

	/* clip height */
	tmp = y + height;
	if (tmp <= y || y >= rend->height)
		return;
	if (tmp > rend->height)
		height = rend->height - y;

	/* prepare */
	dst = rend->data;
	dst = &dst[y * rend->stride + x * 4];
	out = (0xff << 24) | (0xd0 << 16) | (0x10 << 8) | 0x10;

	/* draw outline into buffer */
	for (i = 0; i < height; ++i) {
		((uint32_t*)dst)[0] = out;
		((uint32_t*)dst)[width - 1] = out;

		if (!i || i + 1 == height) {
			for (j = 0; j < width; ++j)
				((uint32_t*)dst)[j] = out;
		}

		dst += rend->stride;
	}
}

static void renderer_blend_a1(uint8_t *dst,
			      unsigned int dst_stride,
			      const uint8_t *src,
			      unsigned int src_stride,
			      unsigned int width,
			      unsigned int height,
			      uint8_t fr, uint8_t fg, uint8_t fb,
			      uint8_t br, uint8_t bg, uint8_t bb)
{
	unsigned int i;
	uint32_t out;
	uint_fast32_t r, g, b;

	while (height--) {
		for (i = 0; i < width; ++i) {
			if (src[i / 8] & (1 << (i % 8))) {
				r = fr;
				g = fg;
				b = fb;
			} else {
				r = br;
				g = bg;
				b = bb;
			}

			out = (0xff << 24) | (r << 16) | (g << 8) | b;
			((uint32_t*)dst)[i] = out;
		}

		dst += dst_stride;
		src += src_stride;
	}
}

static void renderer_blend_a8(uint8_t *dst,
			      unsigned int dst_stride,
			      const uint8_t *src,
			      unsigned int src_stride,
			      unsigned int width,
			      unsigned int height,
			      uint8_t fr, uint8_t fg, uint8_t fb,
			      uint8_t br, uint8_t bg, uint8_t bb)
{
	unsigned int i;
	uint32_t out;
	uint_fast32_t r, g, b;

	while (height--) {
		for (i = 0; i < width; ++i) {
			if (src[i] == 0) {
				r = br;
				g = bg;
				b = bb;
			} else if (src[i] == 255) {
				r = fr;
				g = fg;
				b = fb;
			} else {
				/* Division by 255 (t /= 255) is done with:
				 *   t += 0x80
				 *   t = (t + (t >> 8)) >> 8
				 * This speeds up the computation by ~20% as
				 * the division is skipped. */
				r = fr * src[i] + br * (255 - src[i]);
				r += 0x80;
				r = (r + (r >> 8)) >> 8;

				g = fg * src[i] + bg * (255 - src[i]);
				g += 0x80;
				g = (g + (g >> 8)) >> 8;

				b = fb * src[i] + bb * (255 - src[i]);
				b += 0x80;
				b = (b + (b >> 8)) >> 8;
			}

			out = (0xff << 24) | (r << 16) | (g << 8) | b;
			((uint32_t*)dst)[i] = out;
		}

		dst += dst_stride;
		src += src_stride;
	}
}

static void renderer_blend_xrgb32(uint8_t *dst,
				  unsigned int dst_stride,
				  const uint8_t *src,
				  unsigned int src_stride,
				  unsigned int width,
				  unsigned int height,
				  uint8_t fr, uint8_t fg, uint8_t fb,
				  uint8_t br, uint8_t bg, uint8_t bb)
{
	unsigned int i;
	uint32_t out, mask;
	uint_fast32_t r, g, b;
	uint_fast8_t rm, gm, bm;

	while (height--) {
		for (i = 0; i < width; ++i) {
			mask = *(uint32_t*)&src[i * 4];
			rm = (mask & 0x00ff0000) >> 16;
			gm = (mask & 0x0000ff00) >> 8;
			bm = mask & 0x000000ff;

			/* Division by 255 (t /= 255) is done with:
			 *   t += 0x80
			 *   t = (t + (t >> 8)) >> 8
			 * This speeds up the computation by ~20% as
			 * the division is skipped. */

			if (rm == 0) {
				r = br;
			} else if (rm == 255) {
				r = fr;
			} else {
				r = fr * rm + br * (255 - rm);
				r += 0x80;
				r = (r + (r >> 8)) >> 8;
			}

			if (gm == 0) {
				g = bg;
			} else if (gm == 255) {
				g = fg;
			} else {
				g = fg * gm + bg * (255 - gm);
				g += 0x80;
				g = (g + (g >> 8)) >> 8;
			}

			if (bm == 0) {
				b = bb;
			} else if (bm == 255) {
				b = fb;
			} else {
				b = fb * bm + bb * (255 - bm);
				b += 0x80;
				b = (b + (b >> 8)) >> 8;
			}

			out = (0xff << 24) | (r << 16) | (g << 8) | b;
			((uint32_t*)dst)[i] = out;
		}

		dst += dst_stride;
		src += src_stride;
	}
}

static void renderer_blend(struct gtktsm_renderer *rend,
			   const struct gtktsm_glyph *glyph,
			   unsigned int x,
			   unsigned int y,
			   uint8_t fr, uint8_t fg, uint8_t fb,
			   uint8_t br, uint8_t bg, uint8_t bb)
{
	unsigned int tmp, width, height;
	const uint8_t *src;
	uint8_t *dst;

	/* clip width */
	tmp = x + glyph->width;
	if (tmp <= x || x >= rend->width)
		return;
	if (tmp > rend->width)
		width = rend->width - x;
	else
		width = glyph->width;

	/* clip height */
	tmp = y + glyph->height;
	if (tmp <= y || y >= rend->height)
		return;
	if (tmp > rend->height)
		height = rend->height - y;
	else
		height = glyph->height;

	/* prepare */
	dst = rend->data;
	dst = &dst[y * rend->stride + x * 4];
	src = glyph->buffer;

	switch (glyph->format) {
	case GTKTSM_GLYPH_A1:
		renderer_blend_a1(dst,
				  rend->stride,
				  src,
				  glyph->stride,
				  width,
				  height,
				  fr, fg, fb,
				  br, bg, bb);
		break;
	case GTKTSM_GLYPH_A8:
		renderer_blend_a8(dst,
				  rend->stride,
				  src,
				  glyph->stride,
				  width,
				  height,
				  fr, fg, fb,
				  br, bg, bb);
		break;
	case GTKTSM_GLYPH_XRGB32:
		renderer_blend_xrgb32(dst,
				      rend->stride,
				      src,
				      glyph->stride,
				      width,
				      height,
				      fr, fg, fb,
				      br, bg, bb);
		break;
	default:
		g_error("invalid glyph format: %d", glyph->format);
		break;
	}
}

static int renderer_draw_cell(struct tsm_screen *screen,
			      uint32_t id,
			      const uint32_t *ch,
			      size_t len,
			      unsigned int cwidth,
			      unsigned int posx,
			      unsigned int posy,
			      const struct tsm_screen_attr *attr,
			      tsm_age_t age,
			      void *data)
{
	const struct gtktsm_renderer_ctx *ctx = data;
	struct gtktsm_renderer *rend = ctx->rend;
	struct gtktsm_face *face;
	uint8_t fr, fg, fb, br, bg, bb;
	unsigned int x, y;
	struct gtktsm_glyph *glyph;
	bool skip;
	int r;

	/* Skip if our age and the cell age is non-zero *and* the cell-age is
	 * smaller than our age. */
	skip = age && rend->age && age <= rend->age;

	if (skip && !ctx->debug)
		return 0;

	x = posx * ctx->cell_width;
	y = posy * ctx->cell_height;

	/* invert colors if requested */
	if (attr->inverse) {
		fr = attr->br;
		fg = attr->bg;
		fb = attr->bb;
		br = attr->fr;
		bg = attr->fg;
		bb = attr->fb;
	} else {
		fr = attr->fr;
		fg = attr->fg;
		fb = attr->fb;
		br = attr->br;
		bg = attr->bg;
		bb = attr->bb;
	}

	/* select correct font */
	if (attr->bold && ctx->face_bold)
		face = ctx->face_bold;
	else
		face = ctx->face_regular;

	/* !len means background-only */
	if (!len) {
		renderer_fill(rend,
			      x,
			      y,
			      ctx->cell_width * cwidth,
			      ctx->cell_height,
			      br, bg, bb);
	} else {
		r = gtktsm_face_render(face,
				       &glyph,
				       id,
				       ch,
				       len,
				       cwidth);
		if (r < 0)
			renderer_fill(rend,
				      x,
				      y,
				      ctx->cell_width * cwidth,
				      ctx->cell_height,
				      br, bg, bb);
		else
			renderer_blend(rend,
				       glyph,
				       x,
				       y,
				       fr, fg, fb,
				       br, bg, bb);
	}

	if (attr->underline)
		renderer_fill(rend,
			      x,
			      y + face->underline_pos,
			      ctx->cell_width * cwidth,
			      face->line_thickness,
			      fr, fg, fb);

	if (!skip && ctx->debug)
		renderer_highlight(rend,
				   x,
				   y,
				   ctx->cell_width * cwidth,
				   ctx->cell_height);

	return 0;
}

static void gtktsm_renderer_draw(const struct gtktsm_renderer_ctx *ctx)
{
	struct gtktsm_renderer *rend = ctx->rend;
	struct tsm_screen_attr attr;
	unsigned int w, h;

	/* cairo is *way* too slow to render all masks efficiently. Therefore,
	 * we render all glyphs into a shadow buffer on the CPU and then tell
	 * cairo to blit it into the gtk buffer. This way we get two mem-writes
	 * but at least it's fast enough to render a whole screen. */

	cairo_surface_flush(rend->surface);
	rend->age = tsm_screen_draw(ctx->screen,
				    renderer_draw_cell,
				    (void*)ctx);
	cairo_surface_mark_dirty(rend->surface);

	cairo_set_source_surface(ctx->cr, rend->surface, 0, 0);
	cairo_paint(ctx->cr);

	/* draw padding */
	w = tsm_screen_get_width(ctx->screen);
	h = tsm_screen_get_height(ctx->screen);
	tsm_vte_get_def_attr(ctx->vte, &attr);
	cairo_set_source_rgb(ctx->cr,
			     attr.br / 255.0,
			     attr.bg / 255.0,
			     attr.bb / 255.0);
	cairo_move_to(ctx->cr, w * ctx->cell_width, 0);
	cairo_line_to(ctx->cr, w * ctx->cell_width, h * ctx->cell_height);
	cairo_line_to(ctx->cr, 0, h * ctx->cell_height);
	cairo_line_to(ctx->cr, 0, rend->height);
	cairo_line_to(ctx->cr, rend->width, rend->height);
	cairo_line_to(ctx->cr, rend->width, 0);
	cairo_close_path(ctx->cr);
	cairo_fill(ctx->cr);
}

/*
 * GtkTsmTerminal Widget
 * The GtkTsmTerminal widget is very similar to libvte. It uses libtsm and
 * the other helpers here to implement a Gtk widget that displays a terminal
 * emulator.
 *
 * The creator of the widget has exclusive control over the PTY process, as it
 * returns after fork() and before doing any exec().
 */

typedef struct _GtkTsmTerminalPrivate {
	/* child objects */
	struct gtktsm_renderer *rend;
	struct gtktsm_font *font;
	struct tsm_screen *screen;
	struct tsm_vte *vte;

	/* pty bridge */
	int pty_bridge;
	GIOChannel *bridge_chan;
	guint bridge_src;

	/* properties */
	char *prop_font;
	cairo_antialias_t prop_aa;
	cairo_subpixel_order_t prop_subpixel;
	unsigned int prop_sb_size;

	/* font faces */
	struct gtktsm_face *face_regular;
	struct gtktsm_face *face_bold;
	struct gtktsm_face *face_italic;
	struct gtktsm_face *face_bold_italic;

	/* selection */
	unsigned int sel;
	guint32 sel_start;
	gdouble sel_x;
	gdouble sel_y;

	/* pty */
	struct shl_pty *pty;
	guint child_src;
	guint idle_src;

	/* cache */
	GdkKeymap *keymap;
	unsigned int width;
	unsigned int height;
	unsigned int columns;
	unsigned int rows;

	bool realized : 1;
	bool show_dirty : 1;
	bool debug : 1;
} GtkTsmTerminalPrivate;

enum {
	TERMINAL_PROP_0,
	TERMINAL_PROP_FONT,
	TERMINAL_PROP_SB_SIZE,
	TERMINAL_PROP_ANTI_ALIASING,
	TERMINAL_PROP_SUBPIXEL_ORDER,
	TERMINAL_PROP_SHOW_DIRTY,
	TERMINAL_PROP_DEBUG,
	TERMINAL_PROP_CNT,
};

enum {
	TERMINAL_SIGNAL_CHANGED,
	TERMINAL_SIGNAL_STOPPED,
	TERMINAL_SIGNAL_CNT,
};

static GParamSpec *terminal_props[TERMINAL_PROP_CNT];
static guint terminal_signals[TERMINAL_SIGNAL_CNT];

G_DEFINE_TYPE_WITH_PRIVATE(GtkTsmTerminal,
			   gtktsm_terminal,
			   GTK_TYPE_DRAWING_AREA);

static void terminal_recalculate_cells(GtkTsmTerminal *term,
				       unsigned int width,
				       unsigned int height)
{
	GtkTsmTerminalPrivate *p = gtktsm_terminal_get_instance_private(term);

	p->width = width;
	p->height = height;

	if (p->face_regular && p->face_regular->width)
		p->columns = (p->width / p->face_regular->width) ? : 1;
	else
		p->columns = 1;

	if (p->face_regular && p->face_regular->height)
		p->rows = (p->height / p->face_regular->height) ? : 1;
	else
		p->rows = 1;
}

static void terminal_set_font(GtkTsmTerminal *term)
{
	GtkTsmTerminalPrivate *p = gtktsm_terminal_get_instance_private(term);
	struct gtktsm_face *regular;
	int r;

	r = gtktsm_face_new(&regular,
			    p->font,
			    p->prop_font,
			    -1,
			    0,
			    0,
			    p->prop_aa,
			    p->prop_subpixel);
	if (r < 0)
		g_error("cannot initialize pango font face (desc: %s)",
			p->prop_font);

	gtktsm_face_free(p->face_regular);
	gtktsm_face_free(p->face_bold);
	gtktsm_face_free(p->face_italic);
	gtktsm_face_free(p->face_bold_italic);

	/* mandatory regular face */
	p->face_regular = regular;

	/* optional bold face */
	r = gtktsm_face_new(&p->face_bold,
			    p->font,
			    p->prop_font,
			    -1,
			    1,
			    0,
			    p->prop_aa,
			    p->prop_subpixel);
	if (r < 0)
		p->face_bold = NULL;

	/* optional italic face */
	r = gtktsm_face_new(&p->face_italic,
			    p->font,
			    p->prop_font,
			    -1,
			    0,
			    1,
			    p->prop_aa,
			    p->prop_subpixel);
	if (r < 0)
		p->face_italic = NULL;

	/* optional bold, italic face */
	r = gtktsm_face_new(&p->face_bold_italic,
			    p->font,
			    p->prop_font,
			    -1,
			    1,
			    1,
			    p->prop_aa,
			    p->prop_subpixel);
	if (r < 0)
		p->face_bold_italic = NULL;

	terminal_recalculate_cells(term, p->width, p->height);
	gtk_widget_queue_draw(GTK_WIDGET(term));
}

static gboolean terminal_configure_fn(GtkWidget *widget,
				      GdkEvent *ev,
				      gpointer data)
{
	GtkTsmTerminal *term = GTKTSM_TERMINAL(widget);
	GtkTsmTerminalPrivate *p = gtktsm_terminal_get_instance_private(term);
	GdkEventConfigure *cev = (void*)ev;
	bool state_changed = false;
	GdkWindow *wnd;
	GdkEventMask mask;
	int r;

	terminal_recalculate_cells(term, cev->width, cev->height);

	r = gtktsm_renderer_resize(p->rend, p->width, p->height);
	if (r < 0)
		g_error("gtktsm_renderer_resize() failed: %d", r);

	r = tsm_screen_resize(p->screen, p->columns, p->rows);
	if (r < 0)
		g_error("tsm_screen_resize() failed: %d", r);

	if (p->pty) {
		r = shl_pty_resize(p->pty, p->columns, p->rows);
		if (r < 0)
			g_error("shl_pty_resize() failed: %d", r);
	}

	if (!p->realized) {
		p->realized = true;
		wnd = gtk_widget_get_window(widget);
		mask = gdk_window_get_events(wnd);
		mask |= GDK_KEY_PRESS_MASK;
		mask |= GDK_BUTTON_MOTION_MASK;
		mask |= GDK_BUTTON_PRESS_MASK;
		mask |= GDK_BUTTON_RELEASE_MASK;
		gdk_window_set_events(wnd, mask);
	}

	gtk_widget_queue_draw(widget);

	if (state_changed)
		g_signal_emit(term,
			      terminal_signals[TERMINAL_SIGNAL_CHANGED],
			      0);

	return TRUE;
}

static gboolean terminal_draw_fn(GtkWidget *widget,
				 cairo_t *cr,
				 gpointer data)
{
	GtkTsmTerminal *term = GTKTSM_TERMINAL(widget);
	GtkTsmTerminalPrivate *p = gtktsm_terminal_get_instance_private(term);
	struct gtktsm_renderer_ctx ctx;
	struct tsm_screen_attr attr;
	int64_t start, end;

	if (!p->face_regular) {
		tsm_vte_get_def_attr(p->vte, &attr);
		cairo_set_source_rgb(cr,
				     attr.br / 255.0,
				     attr.bg / 255.0,
				     attr.bb / 255.0);
		cairo_paint(cr);
		return FALSE;
	}

	start = g_get_monotonic_time();

	memset(&ctx, 0, sizeof(ctx));
	ctx.debug = p->show_dirty;
	ctx.rend = p->rend;
	ctx.cr = cr;
	ctx.face_regular = p->face_regular;
	ctx.face_bold = p->face_bold;
	ctx.face_italic = p->face_italic;
	ctx.face_bold_italic = p->face_bold_italic;
	ctx.screen = p->screen;
	ctx.vte = p->vte;
	ctx.cell_width = p->face_regular->width;
	ctx.cell_height = p->face_regular->height;

	gtktsm_renderer_draw(&ctx);

	end = g_get_monotonic_time();
	if (p->debug)
		g_message("frame rendered in: %lldms", (long long)((end - start) / 1000));

	return FALSE;
}

#define ALL_MODS (GDK_SHIFT_MASK | GDK_LOCK_MASK | GDK_CONTROL_MASK | \
		  GDK_MOD1_MASK | GDK_MOD4_MASK)

static gboolean terminal_key_fn(GtkWidget *widget,
				GdkEvent *ev,
				gpointer data)
{
	GtkTsmTerminal *term = GTKTSM_TERMINAL(widget);
	GtkTsmTerminalPrivate *p = gtktsm_terminal_get_instance_private(term);
	GdkEventKey *e = (void*)ev;
	unsigned int mods = 0;
	gboolean b;
	GdkModifierType cmod;
	guint key;
	uint32_t ucs4;

	if (e->type != GDK_KEY_PRESS)
		return FALSE;

	if (e->state & GDK_SHIFT_MASK)
		mods |= TSM_SHIFT_MASK;
	if (e->state & GDK_LOCK_MASK)
		mods |= TSM_LOCK_MASK;
	if (e->state & GDK_CONTROL_MASK)
		mods |= TSM_CONTROL_MASK;
	if (e->state & GDK_MOD1_MASK)
		mods |= TSM_ALT_MASK;
	if (e->state & GDK_MOD4_MASK)
		mods |= TSM_LOGO_MASK;

	if (!p->keymap)
		p->keymap = gdk_keymap_get_default();

	b = gdk_keymap_translate_keyboard_state(p->keymap,
						e->hardware_keycode,
						e->state,
						e->group,
						&key,
						NULL,
						NULL,
						&cmod);

	if (b) {
		if (key == GDK_KEY_Up &&
		    ((e->state & ~cmod & ALL_MODS) == GDK_SHIFT_MASK)) {
			tsm_screen_sb_up(p->screen, 1);
			gtk_widget_queue_draw(GTK_WIDGET(term));
			return TRUE;
		} else if (key == GDK_KEY_Down &&
		    ((e->state & ~cmod & ALL_MODS) == GDK_SHIFT_MASK)) {
			tsm_screen_sb_down(p->screen, 1);
			gtk_widget_queue_draw(GTK_WIDGET(term));
			return TRUE;
		} else if (key == GDK_KEY_Page_Up &&
		    ((e->state & ~cmod & ALL_MODS) == GDK_SHIFT_MASK)) {
			tsm_screen_sb_page_up(p->screen, 1);
			gtk_widget_queue_draw(GTK_WIDGET(term));
			return TRUE;
		} else if (key == GDK_KEY_Page_Down &&
		    ((e->state & ~cmod & ALL_MODS) == GDK_SHIFT_MASK)) {
			tsm_screen_sb_page_down(p->screen, 1);
			gtk_widget_queue_draw(GTK_WIDGET(term));
			return TRUE;
		}
	}

	ucs4 = xkb_keysym_to_utf32(e->keyval);
	if (!ucs4)
		ucs4 = TSM_VTE_INVALID;

	if (tsm_vte_handle_keyboard(p->vte, e->keyval, 0, mods, ucs4)) {
		tsm_screen_sb_reset(p->screen);
		gtk_widget_queue_draw(GTK_WIDGET(term));
		return TRUE;
	}

	return FALSE;
}

static gboolean terminal_button_fn(GtkWidget *widget,
				   GdkEvent *ev,
				   gpointer data)
{
	GtkTsmTerminal *term = GTKTSM_TERMINAL(widget);
	GtkTsmTerminalPrivate *p = gtktsm_terminal_get_instance_private(term);
	GdkEventButton *e = (void*)ev;
	unsigned int cell_width, cell_height;

	if (e->button != 1)
		return FALSE;

	if (!p->face_regular)
		return FALSE;

	cell_width = p->face_regular->width;
	cell_height = p->face_regular->height;

	if (e->type == GDK_BUTTON_PRESS) {
		p->sel = 1;
		p->sel_start = e->time;
		p->sel_x = e->x;
		p->sel_y = e->y;
	} else if (e->type == GDK_2BUTTON_PRESS) {
		p->sel = 2;
		/* TODO: select word */
		tsm_screen_selection_start(p->screen,
					   e->x / cell_width,
					   e->y / cell_height);
		gtk_widget_queue_draw(GTK_WIDGET(term));
	} else if (e->type == GDK_3BUTTON_PRESS) {
		p->sel = 2;
		/* TODO: select line */
		tsm_screen_selection_start(p->screen,
					   e->x / cell_width,
					   e->y / cell_height);
		gtk_widget_queue_draw(GTK_WIDGET(term));
	} else if (e->type == GDK_BUTTON_RELEASE) {
		if (p->sel == 1 && p->sel_start + 100 > e->time) {
			tsm_screen_selection_reset(p->screen);
			gtk_widget_queue_draw(GTK_WIDGET(term));
		} else if (p->sel > 1) {
			/* TODO: copy */
		}

		p->sel = 0;
	}

	return TRUE;
}

static gboolean terminal_motion_fn(GtkWidget *widget,
				   GdkEvent *ev,
				   gpointer data)
{
	GtkTsmTerminal *term = GTKTSM_TERMINAL(widget);
	GtkTsmTerminalPrivate *p = gtktsm_terminal_get_instance_private(term);
	GdkEventMotion *e = (void*)ev;
	unsigned int cell_width, cell_height;

	if (!p->sel)
		return TRUE;

	if (!p->face_regular)
		return FALSE;

	cell_width = p->face_regular->width;
	cell_height = p->face_regular->height;

	if (p->sel == 1) {
		if (fabs(p->sel_x - e->x) > 3 ||
		    fabs(p->sel_y - e->y) > 3) {
			p->sel = 2;
			tsm_screen_selection_start(p->screen,
						   p->sel_x / cell_width,
						   p->sel_y / cell_height);
			gtk_widget_queue_draw(GTK_WIDGET(term));
		}
	} else {
		tsm_screen_selection_target(p->screen,
					    e->x / cell_width,
					    e->y / cell_height);
		gtk_widget_queue_draw(GTK_WIDGET(term));
	}

	return FALSE;
}

static gboolean terminal_idle_fn(gpointer data)
{
	GtkTsmTerminal *term = data;
	GtkTsmTerminalPrivate *p = gtktsm_terminal_get_instance_private(term);
	int r;

	p->idle_src = 0;
	if (!p->pty)
		return FALSE;

	r = shl_pty_bridge_dispatch_pty(p->pty_bridge, p->pty);
	if (r < 0)
		g_error("shl_pty_bridge_dispatch() failed: %d", r);

	return FALSE;
}

static void terminal_write_fn(struct tsm_vte *vte,
			      const char *u8,
			      size_t len,
			      void *data)
{
	GtkTsmTerminal *term = data;
	GtkTsmTerminalPrivate *p = gtktsm_terminal_get_instance_private(term);
	int r;

	if (!p->pty)
		return;

	r = shl_pty_write(p->pty, u8, len);
	if (r < 0)
		g_error("OOM in pty-write: %d", r);

	/* dont directly call into pty-bridge to avoid possible recursion */
	if (!p->idle_src)
		p->idle_src = g_idle_add(terminal_idle_fn, term);
}

static void terminal_log_fn(void *data,
			    const char *file,
			    int line,
			    const char *fn,
			    const char *subs,
			    unsigned int sev,
			    const char *format,
			    va_list args)
{
	GLogLevelFlags flags = 0;

	switch (sev) {
	case LLOG_DEBUG:
		flags |= G_LOG_LEVEL_DEBUG;
		break;
	case LLOG_INFO:
		flags |= G_LOG_LEVEL_INFO;
		break;
	case LLOG_NOTICE:
		flags |= G_LOG_LEVEL_MESSAGE;
		break;
	case LLOG_WARNING:
		flags |= G_LOG_LEVEL_WARNING;
		break;
	case LLOG_ERROR:
		flags |= G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL;
		break;
	case LLOG_CRITICAL:
	case LLOG_ALERT:
	case LLOG_FATAL:
		flags |= G_LOG_LEVEL_CRITICAL | G_LOG_FLAG_FATAL;
		break;
	}

	g_logv(G_LOG_DOMAIN "-tsm", flags, format, args);
}

static gboolean terminal_bridge_fn(GIOChannel *chan,
				   GIOCondition cond,
				   gpointer data)
{
	GtkTsmTerminal *term = data;
	GtkTsmTerminalPrivate *p = gtktsm_terminal_get_instance_private(term);
	int r;

	r = shl_pty_bridge_dispatch(p->pty_bridge, 0);
	if (r < 0)
		g_error("shl_pty_bridge_dispatch() failed: %d", r);

	return TRUE;
}

static void terminal_get_property(GObject *gobj,
				  guint prop,
				  GValue *val,
				  GParamSpec *spec)
{
	GtkTsmTerminal *term = GTKTSM_TERMINAL(gobj);
	GtkTsmTerminalPrivate *p = gtktsm_terminal_get_instance_private(term);
	const char *str;

	switch (prop) {
	case TERMINAL_PROP_FONT:
		g_value_set_string(val, p->prop_font);
		break;
	case TERMINAL_PROP_SB_SIZE:
		g_value_set_uint(val, p->prop_sb_size);
		break;
	case TERMINAL_PROP_ANTI_ALIASING:
		switch (p->prop_aa) {
		case CAIRO_ANTIALIAS_NONE:
			str = "none";
			break;
		case CAIRO_ANTIALIAS_GRAY:
			str = "gray";
			break;
		case CAIRO_ANTIALIAS_SUBPIXEL:
			str = "subpixel";
			break;
		default:
			str = "default";
			break;
		}

		g_value_set_string(val, str);
		break;
	case TERMINAL_PROP_SUBPIXEL_ORDER:
		switch (p->prop_subpixel) {
		case CAIRO_SUBPIXEL_ORDER_RGB:
			str = "rgb";
			break;
		case CAIRO_SUBPIXEL_ORDER_BGR:
			str = "bgr";
			break;
		case CAIRO_SUBPIXEL_ORDER_VRGB:
			str = "vrgb";
			break;
		case CAIRO_SUBPIXEL_ORDER_VBGR:
			str = "vbgr";
			break;
		default:
			str = "default";
			break;
		}

		g_value_set_string(val, str);
		break;
	case TERMINAL_PROP_SHOW_DIRTY:
		g_value_set_boolean(val, p->show_dirty);
		break;
	case TERMINAL_PROP_DEBUG:
		g_value_set_boolean(val, p->debug);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(gobj, prop, spec);
		break;
	}
}

static void terminal_set_property(GObject *gobj,
				  guint prop,
				  const GValue *val,
				  GParamSpec *spec)
{
	GtkTsmTerminal *term = GTKTSM_TERMINAL(gobj);
	GtkTsmTerminalPrivate *p = gtktsm_terminal_get_instance_private(term);
	const char *str;
	bool update_font = false;

	switch (prop) {
	case TERMINAL_PROP_FONT:
		update_font = true;
		g_free(p->prop_font);
		p->prop_font = g_strdup(g_value_get_string(val));
		break;
	case TERMINAL_PROP_SB_SIZE:
		p->prop_sb_size = g_value_get_uint(val);
		tsm_screen_set_max_sb(p->screen, p->prop_sb_size);
		break;
	case TERMINAL_PROP_ANTI_ALIASING:
		update_font = true;
		str = g_value_get_string(val);
		if (!strcmp(str, "none"))
			p->prop_aa = CAIRO_ANTIALIAS_NONE;
		else if (!strcmp(str, "gray"))
			p->prop_aa = CAIRO_ANTIALIAS_GRAY;
		else if (!strcmp(str, "subpixel"))
			p->prop_aa = CAIRO_ANTIALIAS_SUBPIXEL;
		else
			p->prop_aa = CAIRO_ANTIALIAS_DEFAULT;
		break;
	case TERMINAL_PROP_SUBPIXEL_ORDER:
		update_font = true;
		str = g_value_get_string(val);
		if (!strcmp(str, "rgb"))
			p->prop_subpixel = CAIRO_SUBPIXEL_ORDER_RGB;
		else if (!strcmp(str, "bgr"))
			p->prop_subpixel = CAIRO_SUBPIXEL_ORDER_BGR;
		else if (!strcmp(str, "vrgb"))
			p->prop_subpixel = CAIRO_SUBPIXEL_ORDER_VRGB;
		else if (!strcmp(str, "vbgr"))
			p->prop_subpixel = CAIRO_SUBPIXEL_ORDER_VBGR;
		else
			p->prop_subpixel = CAIRO_SUBPIXEL_ORDER_DEFAULT;
		break;
	case TERMINAL_PROP_SHOW_DIRTY:
		p->show_dirty = g_value_get_boolean(val);
		gtk_widget_queue_draw(GTK_WIDGET(term));
		break;
	case TERMINAL_PROP_DEBUG:
		p->debug = g_value_get_boolean(val);
		gtk_widget_queue_draw(GTK_WIDGET(term));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(gobj, prop, spec);
		break;
	}

	/* Only update font if we already have one. Otherwise, a new font is
	 * created on demand. */
	if (update_font && p->face_regular)
		terminal_set_font(term);
}

static void terminal_finalize(GObject *gobj)
{
	GtkTsmTerminal *term = GTKTSM_TERMINAL(gobj);
	GtkTsmTerminalPrivate *p = gtktsm_terminal_get_instance_private(term);

	if (p->pty) {
		if (p->child_src)
			g_source_remove(p->child_src);
		shl_pty_bridge_remove(p->pty_bridge, p->pty);
		shl_pty_close(p->pty);
		shl_pty_unref(p->pty);
	}

	if (p->idle_src)
		g_source_remove(p->idle_src);

	gtktsm_face_free(p->face_regular);
	gtktsm_face_free(p->face_bold);
	gtktsm_face_free(p->face_italic);
	gtktsm_face_free(p->face_bold_italic);

	g_free(p->prop_font);

	g_source_remove(p->bridge_src);
	g_io_channel_unref(p->bridge_chan);
	shl_pty_bridge_free(p->pty_bridge);

	tsm_vte_unref(p->vte);
	tsm_screen_unref(p->screen);
	gtktsm_font_unref(p->font);
	gtktsm_renderer_free(p->rend);

	G_OBJECT_CLASS(gtktsm_terminal_parent_class)->finalize(gobj);
}

static void gtktsm_terminal_init(GtkTsmTerminal *term)
{
	GtkTsmTerminalPrivate *p = gtktsm_terminal_get_instance_private(term);
	int r;

	r = gtktsm_renderer_new(&p->rend, 0, 0);
	if (r < 0)
		g_error("gtktsm_renderer_new() failed: %d", r);

	r = gtktsm_font_new(&p->font);
	if (r < 0)
		g_error("gtktsm_font_new() failed: %d", r);

	r = tsm_screen_new(&p->screen,
			   terminal_log_fn,
			   term);
	if (r < 0)
		g_error("tsm_screen_new() failed: %d", r);

	r = tsm_vte_new(&p->vte,
			p->screen,
			terminal_write_fn,
			term,
			terminal_log_fn,
			term);
	if (r < 0)
		g_error("tsm_vte_new() failed: %d", r);

	p->pty_bridge = shl_pty_bridge_new();
	if (p->pty_bridge < 0)
		g_error("shl_pty_bridge_new() failed: %d",
			p->pty_bridge);

	p->bridge_chan = g_io_channel_unix_new(p->pty_bridge);
	p->bridge_src = g_io_add_watch(p->bridge_chan,
				       G_IO_IN,
				       terminal_bridge_fn,
				       term);

	g_signal_connect(G_OBJECT(term),
			 "configure-event",
			 G_CALLBACK(terminal_configure_fn),
			 NULL);
	g_signal_connect(G_OBJECT(term),
			 "draw",
			 G_CALLBACK(terminal_draw_fn),
			 NULL);

	gtk_widget_set_can_focus(GTK_WIDGET(term), TRUE);

	g_signal_connect(GTK_WIDGET(term),
			 "key-press-event",
			 G_CALLBACK(terminal_key_fn),
			 term);
	g_signal_connect(GTK_WIDGET(term),
			 "button-press-event",
			 G_CALLBACK(terminal_button_fn),
			 term);
	g_signal_connect(GTK_WIDGET(term),
			 "button-release-event",
			 G_CALLBACK(terminal_button_fn),
			 term);
	g_signal_connect(GTK_WIDGET(term),
			 "motion-notify-event",
			 G_CALLBACK(terminal_motion_fn),
			 term);
}

static void gtktsm_terminal_class_init(GtkTsmTerminalClass *klass)
{
	GParamSpec **prop;
	guint *sig;

	G_OBJECT_CLASS(klass)->finalize = terminal_finalize;
	G_OBJECT_CLASS(klass)->set_property = terminal_set_property;
	G_OBJECT_CLASS(klass)->get_property = terminal_get_property;

	/* properties */

	prop = &terminal_props[TERMINAL_PROP_FONT];
	*prop = g_param_spec_string("font",
				    "Terminal font",
				    "The font to be used for terminal screens",
				    "Monospace",
				    G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

	prop = &terminal_props[TERMINAL_PROP_SB_SIZE];
	*prop = g_param_spec_uint("sb-size",
				  "Scrollback-buffer size",
				  "Number of lines that are kept in the scrollback buffer",
				  0,
				  G_MAXUINT,
				  2000,
				  G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

	prop = &terminal_props[TERMINAL_PROP_ANTI_ALIASING];
	*prop = g_param_spec_string("anti-aliasing",
				    "Anti-Aliasing",
				    "The anti-aliasing mode for terminal fonts",
				    "default",
				    G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

	prop = &terminal_props[TERMINAL_PROP_SUBPIXEL_ORDER];
	*prop = g_param_spec_string("subpixel-order",
				    "Subpixel-Order",
				    "The subpixel-order used for anti-aliasing",
				    "default",
				    G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

	prop = &terminal_props[TERMINAL_PROP_SHOW_DIRTY];
	*prop = g_param_spec_boolean("show-dirty",
				     "Show dirty cells",
				     "Highlight dirty cells to debug terminal rendering",
				     FALSE,
				     G_PARAM_READWRITE);

	prop = &terminal_props[TERMINAL_PROP_DEBUG];
	*prop = g_param_spec_boolean("debug",
				     "Debug mode",
				     "Enable extensive live debugging",
				     FALSE,
				     G_PARAM_READWRITE);

	g_object_class_install_properties(G_OBJECT_CLASS(klass),
					  TERMINAL_PROP_CNT,
					  terminal_props);

	/* signals */

	sig = &terminal_signals[TERMINAL_SIGNAL_CHANGED];
	*sig = g_signal_new("terminal-changed",
			    G_OBJECT_CLASS_TYPE(G_OBJECT_CLASS(klass)),
			    G_SIGNAL_RUN_FIRST,
			    0,
			    NULL, NULL, /* accu + accu-data */
			    g_cclosure_marshal_generic,
			    G_TYPE_NONE, 0);

	sig = &terminal_signals[TERMINAL_SIGNAL_STOPPED];
	*sig = g_signal_new("terminal-stopped",
			    G_OBJECT_CLASS_TYPE(klass),
			    G_SIGNAL_RUN_FIRST,
			    0,
			    NULL, NULL, /* accu + accu-data */
			    g_cclosure_marshal_generic,
			    G_TYPE_NONE, 0);
}

GtkTsmTerminal *gtktsm_terminal_new(void)
{
	return g_object_new(GTKTSM_TYPE_TERMINAL,
			    NULL);
}

static void terminal_read_fn(struct shl_pty *pty,
			     void *data,
			     char *u8,
			     size_t len)
{
	GtkTsmTerminal *term = data;
	GtkTsmTerminalPrivate *p = gtktsm_terminal_get_instance_private(term);

	tsm_vte_input(p->vte, u8, len);
	gtk_widget_queue_draw(GTK_WIDGET(term));
}

static void terminal_child_fn(GPid pid,
			      gint status,
			      gpointer data)
{
	GtkTsmTerminal *term = data;
	GtkTsmTerminalPrivate *p = gtktsm_terminal_get_instance_private(term);

	g_spawn_close_pid(pid);
	p->child_src = 0;

	shl_pty_bridge_remove(p->pty_bridge, p->pty);
	shl_pty_close(p->pty);
	shl_pty_unref(p->pty);
	p->pty = NULL;

	g_signal_emit(term,
		      terminal_signals[TERMINAL_SIGNAL_STOPPED],
		      0);
}

pid_t gtktsm_terminal_fork(GtkTsmTerminal *term)
{
	GtkTsmTerminalPrivate *p;
	pid_t pid;
	int r;

	if (!term)
		return -EINVAL;

	p = gtktsm_terminal_get_instance_private(term);
	if (p->pty)
		return -EALREADY;

	if (!p->face_regular)
		terminal_set_font(term);

	pid = shl_pty_open(&p->pty,
			   terminal_read_fn,
			   term,
			   p->columns,
			   p->rows);
	if (pid < 0)
		g_error("shl_pty_open() failed: %d", (int)pid);
	else if (!pid)
		return pid;

	r = shl_pty_bridge_add(p->pty_bridge, p->pty);
	if (r < 0)
		g_error("shl_pty_bridge_add() failed: %d", r);

	p->child_src = g_child_watch_add(pid, terminal_child_fn, term);

	return pid;
}

void gtktsm_terminal_kill(GtkTsmTerminal *term, int sig)
{
	GtkTsmTerminalPrivate *p;

	if (!term || sig < 1)
		return;

	p = gtktsm_terminal_get_instance_private(term);
	if (!p->pty)
		return;

	shl_pty_signal(p->pty, sig);
}
