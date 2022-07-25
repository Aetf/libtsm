/*
 * TSM - Screen Management Tests
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

#include "test_common.h"
#include "libtsm.h"
#include "libtsm-int.h"

START_TEST(test_screen_init)
{
    struct tsm_screen *screen;
	int r;

	r = tsm_screen_new(&screen, NULL, NULL);
	ck_assert_int_eq(r, 0);

	tsm_screen_unref(screen);
	screen = NULL;
}
END_TEST

START_TEST(test_screen_null)
{
    int r;
    unsigned int n;

    r = tsm_screen_new(NULL, NULL, NULL);
    ck_assert_int_eq(r, -EINVAL);

    tsm_screen_ref(NULL);
    tsm_screen_unref(NULL);

    tsm_screen_set_opts(NULL, 0u);
	tsm_screen_reset_opts(NULL, 0u);

	n = tsm_screen_get_opts(NULL);
	ck_assert_int_eq(n, 0);

	n = tsm_screen_get_width(NULL);
	ck_assert_int_eq(n, 0);

	n = tsm_screen_get_height(NULL);
	ck_assert_int_eq(n, 0);

	r = tsm_screen_resize(NULL, 0, 0);
	ck_assert_int_eq(r, -EINVAL);

	r = tsm_screen_set_margins(NULL, 0u, 0u);
	ck_assert_int_eq(r, -EINVAL);

	tsm_screen_set_max_sb(NULL, 0u);

	tsm_screen_clear_sb(NULL);

	tsm_screen_sb_up(NULL, 0u);
	tsm_screen_sb_down(NULL, 0u);
	tsm_screen_sb_page_up(NULL, 0u);
	tsm_screen_sb_page_down(NULL, 0u);
	tsm_screen_sb_reset(NULL);

	tsm_screen_set_def_attr(NULL, NULL);

	tsm_screen_reset(NULL);

	tsm_screen_set_flags(NULL, 0u);
	tsm_screen_reset_flags(NULL, 0u);

	n = tsm_screen_get_flags(NULL);
	ck_assert_int_eq(n, 0);

	n = tsm_screen_get_cursor_x(NULL);
	ck_assert_int_eq(n, 0);

	n = tsm_screen_get_cursor_y(NULL);
	ck_assert_int_eq(n, 0);

	tsm_screen_set_tabstop(NULL);
	tsm_screen_reset_tabstop(NULL);
	tsm_screen_reset_all_tabstops(NULL);

	tsm_screen_write(NULL, 0u, NULL);

	tsm_screen_newline(NULL);

	tsm_screen_scroll_up(NULL, 0u);
	tsm_screen_scroll_down(NULL, 0u);

	tsm_screen_move_to(NULL, 0u, 0u);
	tsm_screen_move_up(NULL, 0u, false);
	tsm_screen_move_down(NULL, 0u, false);
	tsm_screen_move_right(NULL, 0u);
	tsm_screen_move_left(NULL, 0u);
	tsm_screen_move_line_end(NULL);
	tsm_screen_move_line_home(NULL);

	tsm_screen_tab_right(NULL, 0u);
	tsm_screen_tab_left(NULL, 0u);

	tsm_screen_insert_lines(NULL, 0u);
	tsm_screen_delete_lines(NULL, 0u);
	tsm_screen_insert_chars(NULL, 0u);
	tsm_screen_delete_chars(NULL, 0u);

	tsm_screen_erase_cursor(NULL);
	tsm_screen_erase_chars(NULL, 0u);
	tsm_screen_erase_cursor_to_end(NULL, false);
	tsm_screen_erase_home_to_cursor(NULL, false);
	tsm_screen_erase_current_line(NULL, false);
	tsm_screen_erase_screen_to_cursor(NULL, false);
	tsm_screen_erase_cursor_to_screen(NULL, false);
	tsm_screen_erase_screen(NULL, false);
}
END_TEST

START_TEST(test_screen_resize_alt_colors)
{
	struct tsm_screen *screen;
	struct line *line;
	int r, y, x;
	struct tsm_screen_attr *attr;
	struct tsm_screen_attr new_attr;

	r = tsm_screen_new(&screen, NULL, NULL);
	ck_assert_int_eq(r, 0);

	/* start with an initial 2x2 screen */
	r = tsm_screen_resize(screen, 2, 2);
	ck_assert_int_eq(r, 0);

	/* switch to alternate screen */
	tsm_screen_set_flags(screen, TSM_SCREEN_ALTERNATE);

	/* change background color to red */
	bzero(&new_attr, sizeof(new_attr));
	new_attr.br = 255;
	new_attr.bg = 0;
	new_attr.bb = 0;

	tsm_screen_set_def_attr(screen, &new_attr);
	tsm_screen_erase_screen(screen, false);

	/* now all cells should be red */
	for (y = 0; y < screen->size_y; y++) {
		line = screen->lines[y];
		for (x = 0; x < screen->size_x; x++) {
			attr = &line->cells[x].attr;
			ck_assert_int_eq(attr->br, 255);
			ck_assert_int_eq(attr->bg, 0);
			ck_assert_int_eq(attr->bb, 0);
		}
	}

	/* enlarge to 4x4 while on alternate screen */
	r = tsm_screen_resize(screen, 4, 4);
	ck_assert_int_eq(r, 0);

	/* leave alternate screen */
	tsm_screen_reset_flags(screen, TSM_SCREEN_ALTERNATE);

	/* now all cells should be black (including the new ones) */
	for (y = 0; y < screen->size_y; y++) {
		line = screen->lines[y];
		for (x = 0; x < screen->size_x; x++) {
			attr = &line->cells[x].attr;
			ck_assert_int_eq(attr->br, 0);
			ck_assert_int_eq(attr->bg, 0);
			ck_assert_int_eq(attr->bb, 0);
		}
	}

	tsm_screen_unref(screen);
	screen = NULL;
}
END_TEST


TEST_DEFINE_CASE(misc)
	TEST(test_screen_init)
	TEST(test_screen_null)
	TEST(test_screen_resize_alt_colors)
TEST_END_CASE

TEST_DEFINE(
	TEST_SUITE(screen,
		TEST_CASE(misc),
		TEST_END
	)
)
