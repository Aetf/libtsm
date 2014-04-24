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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include "gtktsm-app.h"
#include "gtktsm-win.h"
#include "shl-macro.h"

struct _GtkTsmApp {
	GtkApplication parent;
};

struct _GtkTsmAppClass {
	GtkApplicationClass parent_class;
};

G_DEFINE_TYPE(GtkTsmApp, gtktsm_app, GTK_TYPE_APPLICATION);

static gboolean arg_version;

static GOptionEntry app_options[] = {
	{ "version", 0, 0, G_OPTION_ARG_NONE, &arg_version, "Show version information and exit", NULL },
	{ "font", 0, 0, G_OPTION_ARG_STRING, NULL, "Terminal font", "FONT" },
	{ "sb-size", 0, 0, G_OPTION_ARG_INT, NULL, "Scroll-back buffer size in lines", "COUNT" },
	{ "anti-aliasing", 0, 0, G_OPTION_ARG_STRING, NULL, "Anti-aliasing mode for font rendering", "{none,gray,subpixel,default}" },
	{ "subpixel-order", 0, 0, G_OPTION_ARG_STRING, NULL, "Subpixel order for font rendering", "{rgb,bgr,vrgb,vbgr,default}" },
	{ "show-dirty", 0, 0, G_OPTION_ARG_NONE, NULL, "Mark dirty cells during redraw", NULL },
	{ "debug", 0, 0, G_OPTION_ARG_NONE, NULL, "Enable extensive live-debugging", NULL },
	{ NULL }
};

static gint app_handle_local_options(GApplication *gapp,
				     GVariantDict *opts)
{
	if (arg_version) {
		g_print("GtkTsm\n");
		return 0;
	}

	return -1;
}

static gint app_command_line(GApplication *gapp,
			     GApplicationCommandLine *cmd)
{
	GtkTsmApp *app = GTKTSM_APP(gapp);
	GtkTsmWin *win;
	GtkTsmTerminal *term;
	GVariantDict *dict;
	const gchar *sval;
	GVariant *val;
	int r;

	win = gtktsm_win_new(app);
	term = gtktsm_win_get_terminal(win);
	dict = g_application_command_line_get_options_dict(cmd);

	val = g_variant_dict_lookup_value(dict,
					  "font",
					  G_VARIANT_TYPE_STRING);
	if (val)
		g_object_set(G_OBJECT(term),
			     "font", g_variant_get_string(val, NULL),
			     NULL);

	val = g_variant_dict_lookup_value(dict,
					  "sb-size",
					  G_VARIANT_TYPE_INT32);
	if (val)
		g_object_set(G_OBJECT(term),
			     "sb-size", shl_max(g_variant_get_int32(val), 0),
			     NULL);

	val = g_variant_dict_lookup_value(dict,
					  "anti-aliasing",
					  G_VARIANT_TYPE_STRING);
	if (val) {
		sval = g_variant_get_string(val, NULL);
		if (strcmp(sval, "none") &&
		    strcmp(sval, "gray") &&
		    strcmp(sval, "subpixel") &&
		    strcmp(sval, "default")) {
			g_application_command_line_printerr(cmd,
					"invalid anti-aliasing argument: %s\n",
					sval);
			r = 1;
			goto error;
		}

		g_object_set(G_OBJECT(term),
			     "anti-aliasing", sval,
			     NULL);
	}

	val = g_variant_dict_lookup_value(dict,
					  "subpixel-order",
					  G_VARIANT_TYPE_STRING);
	if (val) {
		sval = g_variant_get_string(val, NULL);
		if (strcmp(sval, "rgb") &&
		    strcmp(sval, "bgr") &&
		    strcmp(sval, "vrgb") &&
		    strcmp(sval, "vbgr") &&
		    strcmp(sval, "default")) {
			g_application_command_line_printerr(cmd,
					"invalid subpixel-order argument: %s\n",
					sval);
			r = 1;
			goto error;
		}

		g_object_set(G_OBJECT(term),
			     "subpixel-order", sval,
			     NULL);
	}

	val = g_variant_dict_lookup_value(dict,
					  "show-dirty",
					  G_VARIANT_TYPE_BOOLEAN);
	if (val)
		g_object_set(G_OBJECT(term),
			     "show-dirty", g_variant_get_boolean(val),
			     NULL);

	val = g_variant_dict_lookup_value(dict,
					  "debug",
					  G_VARIANT_TYPE_BOOLEAN);
	if (val)
		g_object_set(G_OBJECT(term),
			     "debug", g_variant_get_boolean(val),
			     NULL);

	gtktsm_win_run(win);
	gtk_window_present(GTK_WINDOW(win));

	return 0;

error:
	gtk_widget_destroy(GTK_WIDGET(win));
	return r;
}

static void app_activate(GApplication *gapp)
{
	GtkTsmApp *app = GTKTSM_APP(gapp);
	GtkTsmWin *win;

	win = gtktsm_win_new(app);
	gtktsm_win_run(win);
	gtk_window_present(GTK_WINDOW(win));
}

static void gtktsm_app_init(GtkTsmApp *app)
{
	g_application_add_main_option_entries(G_APPLICATION(app),
					      app_options);
}

static void gtktsm_app_class_init(GtkTsmAppClass *klass)
{
	GApplicationClass *gapp_klass;

	gapp_klass = G_APPLICATION_CLASS(klass);
	gapp_klass->handle_local_options = app_handle_local_options;
	gapp_klass->command_line = app_command_line;
	gapp_klass->activate = app_activate;
}

GtkTsmApp *gtktsm_app_new(void)
{
	return g_object_new(GTKTSM_APP_TYPE,
			    "application-id", "org.freedesktop.libtsm.gtktsm",
			    "flags", G_APPLICATION_SEND_ENVIRONMENT |
			             G_APPLICATION_HANDLES_COMMAND_LINE,
			    NULL);
}
