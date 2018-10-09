/*
 * TSM - VTE State Machine Tests
 *
 * Copyright (c) 2018 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include "libtsm-int.h"
#include "libtsm.h"
#include "test_common.h"

static void log_cb(void *data, const char *file, int line, const char *func, const char *subs,
				   unsigned int sev, const char *format, va_list args)
{
	UNUSED(data);
	UNUSED(file);
	UNUSED(line);
	UNUSED(func);
	UNUSED(subs);
	UNUSED(sev);
	UNUSED(format);
	UNUSED(args);
}

static void write_cb(struct tsm_vte *vte, const char *u8, size_t len, void *data)
{
	UNUSED(len);
	UNUSED(data);

	ck_assert_ptr_ne(vte, NULL);
	ck_assert_ptr_ne(u8, NULL);
}

START_TEST(test_vte_init)
{
	struct tsm_screen *screen;
	struct tsm_vte *vte;
	int r;

	r = tsm_screen_new(&screen, log_cb, NULL);
	ck_assert_int_eq(r, 0);

	r = tsm_vte_new(&vte, screen, write_cb, NULL, log_cb, NULL);
	ck_assert_int_eq(r, 0);

	tsm_vte_unref(vte);
	vte = NULL;

	tsm_screen_unref(screen);
	screen = NULL;
}
END_TEST

START_TEST(test_vte_null)
{
	int r;
	bool rb;

	r = tsm_vte_new(NULL, NULL, NULL, NULL, log_cb, NULL);
	ck_assert_int_eq(r, -EINVAL);

	tsm_vte_ref(NULL);
	tsm_vte_unref(NULL);

	tsm_vte_set_osc_cb(NULL, NULL, NULL);

	r = tsm_vte_set_palette(NULL, "");
	ck_assert_int_eq(r, -EINVAL);

	r = tsm_vte_set_custom_palette(NULL, NULL);
	ck_assert_int_eq(r, -EINVAL);

	tsm_vte_get_def_attr(NULL, NULL);

	tsm_vte_reset(NULL);
	tsm_vte_hard_reset(NULL);
	tsm_vte_input(NULL, "", 0);

	rb = tsm_vte_handle_keyboard(NULL, 0, 0, 0, 0);
	ck_assert(!rb);
}
END_TEST

static uint8_t test_palette[TSM_COLOR_NUM][3] = {
	[TSM_COLOR_BLACK]         = {   0,  18,  36 },
	[TSM_COLOR_RED]           = {   1,  19,  37 },
	[TSM_COLOR_GREEN]         = {   2,  20,  38 },
	[TSM_COLOR_YELLOW]        = {   3,  21,  39 },
	[TSM_COLOR_BLUE]          = {   4,  22,  40 },
	[TSM_COLOR_MAGENTA]       = {   5,  23,  41 },
	[TSM_COLOR_CYAN]          = {   6,  24,  42 },
	[TSM_COLOR_LIGHT_GREY]    = {   7,  25,  43 },
	[TSM_COLOR_DARK_GREY]     = {   8,  26,  44 },
	[TSM_COLOR_LIGHT_RED]     = {   9,  27,  45 },
	[TSM_COLOR_LIGHT_GREEN]   = {  10,  28,  46 },
	[TSM_COLOR_LIGHT_YELLOW]  = {  11,  29,  47 },
	[TSM_COLOR_LIGHT_BLUE]    = {  12,  30,  48 },
	[TSM_COLOR_LIGHT_MAGENTA] = {  13,  31,  49 },
	[TSM_COLOR_LIGHT_CYAN]    = {  14,  32,  50 },
	[TSM_COLOR_WHITE]         = {  15,  33,  51 },

	[TSM_COLOR_FOREGROUND]    = {  16,  34,  52 },
	[TSM_COLOR_BACKGROUND]    = {  17,  35,  53 },
};

START_TEST(test_vte_custom_palette)
{
	struct tsm_screen *screen;
	struct tsm_vte *vte;
	int r;

	r = tsm_screen_new(&screen, log_cb, NULL);
	ck_assert_int_eq(r, 0);

	r = tsm_vte_new(&vte, screen, write_cb, NULL, log_cb, NULL);
	ck_assert_int_eq(r, 0);

	r = tsm_vte_set_custom_palette(vte, test_palette);
	ck_assert_int_eq(r, 0);

	r = tsm_vte_set_palette(vte, "custom");
	ck_assert_int_eq(r, 0);

	struct tsm_screen_attr attr;
	tsm_vte_get_def_attr(vte, &attr);
	ck_assert_uint_eq(attr.fr, test_palette[TSM_COLOR_FOREGROUND][0]);
	ck_assert_uint_eq(attr.fg, test_palette[TSM_COLOR_FOREGROUND][1]);
	ck_assert_uint_eq(attr.fb, test_palette[TSM_COLOR_FOREGROUND][2]);

	ck_assert_uint_eq(attr.br, test_palette[TSM_COLOR_BACKGROUND][0]);
	ck_assert_uint_eq(attr.bg, test_palette[TSM_COLOR_BACKGROUND][1]);
	ck_assert_uint_eq(attr.bb, test_palette[TSM_COLOR_BACKGROUND][2]);

	tsm_vte_unref(vte);
	vte = NULL;

	tsm_screen_unref(screen);
	screen = NULL;
}
END_TEST

TEST_DEFINE_CASE(misc)
	TEST(test_vte_init)
    TEST(test_vte_null)
    TEST(test_vte_custom_palette)
TEST_END_CASE

// clang-format off
TEST_DEFINE(
	TEST_SUITE(vte,
		TEST_CASE(misc),
		TEST_END
	)
)
// clang-format on
