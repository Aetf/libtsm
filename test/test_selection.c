/*
 * TSM - Selection Copy Tests
 *
 * Copyright (c) 2022 Andreas Heck <aheck@gmx.de>
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

#include <stdio.h>
#include "test_common.h"
#include "libtsm.h"
#include "libtsm-int.h"

static void write_string(struct tsm_screen *screen, const char *str)
{
	struct tsm_screen_attr attr;
	int i;

	bzero(&attr, sizeof(attr));
	attr.fccode = 37; /* white */
	attr.bccode = 40; /* black */

	for (i = 0; str[i]; i++) {
		tsm_screen_write(screen, str[i], &attr);
	}
}

START_TEST(test_screen_copy_incomplete)
{
	struct tsm_screen *screen;
	int r;
	char *str = NULL;

	r = tsm_screen_new(&screen, NULL, NULL);
	ck_assert_int_eq(r, 0);

	r = tsm_screen_resize(screen, 80, 40);
	ck_assert_int_eq(r, 0);

	tsm_screen_newline(screen);
	write_string(screen, "   Hello World!");

	/* select start but don't end selection and copy it */
	tsm_screen_selection_start(screen, 3, 1);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_int_eq(1, r); // strlen == 1?
	ck_assert_str_eq("H", str);
	free(str);
	str = NULL;

	tsm_screen_unref(screen);
	screen = NULL;
}

START_TEST(test_screen_copy_one_cell)
{
	struct tsm_screen *screen;
	int r;
	char *str = NULL;

	r = tsm_screen_new(&screen, NULL, NULL);
	ck_assert_int_eq(r, 0);

	r = tsm_screen_resize(screen, 80, 40);
	ck_assert_int_eq(r, 0);

	tsm_screen_newline(screen);
	write_string(screen, "   Hello World!");

	/* select start, end selection on same cell and copy it */
	tsm_screen_selection_start(screen, 3, 1);
	tsm_screen_selection_target(screen, 3, 1);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_int_eq(1, r); // strlen == 1?
	ck_assert_str_eq("H", str);
	free(str);
	str = NULL;

	tsm_screen_unref(screen);
	screen = NULL;
}

START_TEST(test_screen_copy_line)
{
	struct tsm_screen *screen;
	int r;
	char *str = NULL;

	r = tsm_screen_new(&screen, NULL, NULL);
	ck_assert_int_eq(r, 0);

	r = tsm_screen_resize(screen, 80, 40);
	ck_assert_int_eq(r, 0);

	tsm_screen_newline(screen);
	write_string(screen, "   Hello World!");
	tsm_screen_newline(screen);
	write_string(screen, "Filler Text");
	tsm_screen_newline(screen);

	/* select "Hello World!" from left to right and copy it */
	tsm_screen_selection_start(screen, 3, 1);
	tsm_screen_selection_target(screen, 14, 1);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	const char *expected = "Hello World!";
	ck_assert_int_eq(strlen(expected), r);
	ck_assert_str_eq(expected, str);
	free(str);
	str = NULL;

	/* select "Hello" from left to right and copy it */
	tsm_screen_selection_reset(screen);
	tsm_screen_selection_start(screen, 3, 1);
	tsm_screen_selection_target(screen, 7, 1);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("Hello", str);
	free(str);
	str = NULL;

	/* select "   Hello" from left to right and copy it */
	tsm_screen_selection_reset(screen);
	tsm_screen_selection_start(screen, 0, 1);
	tsm_screen_selection_target(screen, 7, 1);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("   Hello", str);
	free(str);
	str = NULL;

	/* select "Hello World!" from right to left and copy it */
	tsm_screen_selection_reset(screen);
	tsm_screen_selection_start(screen, 14, 1);
	tsm_screen_selection_target(screen, 3, 1);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("Hello World!", str);
	free(str);
	str = NULL;

	/* select "Hello" from right to left and copy it */
	tsm_screen_selection_reset(screen);
	tsm_screen_selection_start(screen, 7, 1);
	tsm_screen_selection_target(screen, 3, 1);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("Hello", str);
	free(str);
	str = NULL;

	tsm_screen_unref(screen);
	screen = NULL;
}
END_TEST

START_TEST(test_screen_copy_line_scrolled)
{
	struct tsm_screen *screen;
	int r, i;
	char *str = NULL;

	r = tsm_screen_new(&screen, NULL, NULL);
	ck_assert_int_eq(r, 0);

	r = tsm_screen_resize(screen, 80, 40);
	ck_assert_int_eq(r, 0);

	for (int i = 0; i < 39; i++) {
		tsm_screen_newline(screen);
	}

	write_string(screen, "   Hello World!");

	/* select "Hello World!" from left to right and copy it */
	tsm_screen_selection_start(screen, 3, 39);
	tsm_screen_selection_target(screen, 14, 39);

	ck_assert_int_eq(screen->sel_start.x, 3);
	ck_assert_int_eq(screen->sel_start.y, 39);
	ck_assert_int_eq(screen->sel_end.x, 14);
	ck_assert_int_eq(screen->sel_end.y, 39);

	/* force the selected text to scroll up */
	tsm_screen_newline(screen);
	tsm_screen_newline(screen);
	tsm_screen_newline(screen);
	tsm_screen_newline(screen);
	tsm_screen_newline(screen);
	tsm_screen_newline(screen);
	tsm_screen_newline(screen);

	ck_assert_int_eq(screen->sel_start.x, 3);
	ck_assert_int_eq(screen->sel_start.y, 32);
	ck_assert_int_eq(screen->sel_end.x, 14);
	ck_assert_int_eq(screen->sel_end.y, 32);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("Hello World!", str);
	free(str);
	str = NULL;

	tsm_screen_unref(screen);
	screen = NULL;
}
END_TEST

START_TEST(test_screen_copy_lines)
{
	struct tsm_screen *screen;
	int r;
	char *str = NULL;

	r = tsm_screen_new(&screen, NULL, NULL);
	ck_assert_int_eq(r, 0);

	r = tsm_screen_resize(screen, 80, 40);
	ck_assert_int_eq(r, 0);

	tsm_screen_newline(screen);
	write_string(screen, "   Hello World!");
	tsm_screen_newline(screen);
	write_string(screen, "This is a copy test");
	tsm_screen_newline(screen);
	write_string(screen, "for a selection with multiple lines.");
	tsm_screen_newline(screen);
	write_string(screen, "All of them are on screen (not in the sb).------");
	tsm_screen_newline(screen);

	/* Select all text excluding the first 3 spaces and the trailing '-' chars from top left to bottom right and copy it */
	tsm_screen_selection_start(screen, 3, 1);
	tsm_screen_selection_target(screen, 41, 4);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	const char *expected = "Hello World!\nThis is a copy test\nfor a selection with multiple lines.\nAll of them are on screen (not in the sb).";
	ck_assert_int_eq(strlen(expected), r);
	ck_assert_str_eq(expected, str);
	free(str);
	str = NULL;

	/* Select "This is a copy test\nfor a selection" from top left and to bottom right copy it */
	tsm_screen_reset(screen);
	tsm_screen_selection_start(screen, 0, 2);
	tsm_screen_selection_target(screen, 14, 3);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("This is a copy test\nfor a selection", str);
	free(str);
	str = NULL;

	/* Select all text excluding the first 3 spaces and the trailing '-' chars from bottom right to top left and copy it */
	tsm_screen_reset(screen);
	tsm_screen_selection_start(screen, 41, 4);
	tsm_screen_selection_target(screen, 3, 1);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("Hello World!\nThis is a copy test\nfor a selection with multiple lines.\nAll of them are on screen (not in the sb).", str);
	free(str);
	str = NULL;

	tsm_screen_unref(screen);
	screen = NULL;
}
END_TEST

START_TEST(test_screen_copy_lines_scrolled)
{
	struct tsm_screen *screen;
	int r, i;
	char *str = NULL;

	r = tsm_screen_new(&screen, NULL, NULL);
	ck_assert_int_eq(r, 0);

	r = tsm_screen_resize(screen, 80, 40);
	ck_assert_int_eq(r, 0);

	for (int i = 0; i < 37; i++) {
		tsm_screen_newline(screen);
	}
	tsm_screen_newline(screen);

	write_string(screen, "   Hello World!");
	tsm_screen_newline(screen);
	write_string(screen, "Line 2");
	tsm_screen_newline(screen);
	write_string(screen, "Line 3");

	/* select "Hello World!" from left to right and copy it */
	tsm_screen_selection_start(screen, 3, 37);
	tsm_screen_selection_target(screen, 5, 39);

	ck_assert_int_eq(screen->sel_start.x, 3);
	ck_assert_int_eq(screen->sel_start.y, 37);
	ck_assert_int_eq(screen->sel_end.x, 5);
	ck_assert_int_eq(screen->sel_end.y, 39);

	/* force the selected text to scroll up */
	tsm_screen_newline(screen);
	tsm_screen_newline(screen);
	tsm_screen_newline(screen);
	tsm_screen_newline(screen);
	tsm_screen_newline(screen);
	tsm_screen_newline(screen);
	tsm_screen_newline(screen);

	ck_assert_int_eq(screen->sel_start.x, 3);
	ck_assert_int_eq(screen->sel_start.y, 30);
	ck_assert_int_eq(screen->sel_end.x, 5);
	ck_assert_int_eq(screen->sel_end.y, 32);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("Hello World!\nLine 2\nLine 3", str);
	free(str);
	str = NULL;

	tsm_screen_unref(screen);
	screen = NULL;
}
END_TEST

START_TEST(test_screen_copy_line_sb)
{
	struct tsm_screen *screen;
	int r;
	char *str = NULL;
	int i;

	r = tsm_screen_new(&screen, NULL, NULL);
	ck_assert_int_eq(r, 0);

	tsm_screen_set_max_sb(screen, 10);

	r = tsm_screen_resize(screen, 80, 40);
	ck_assert_int_eq(r, 0);

	tsm_screen_newline(screen);
	write_string(screen, "   Hello World!");
	tsm_screen_newline(screen);
	write_string(screen, "Filler Text");
	tsm_screen_newline(screen);

	for (i = 0; i < 40; i++) {
		tsm_screen_newline(screen);
	}

	tsm_screen_sb_up(screen, 4);

	/* select "Hello World!" from left to right and copy it */
	tsm_screen_selection_start(screen, 3, 1);
	tsm_screen_selection_target(screen, 14, 1);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("Hello World!", str);
	free(str);
	str = NULL;

	/* select "Hello" from left to right and copy it */
	tsm_screen_selection_reset(screen);
	tsm_screen_selection_start(screen, 3, 1);
	tsm_screen_selection_target(screen, 7, 1);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("Hello", str);
	free(str);
	str = NULL;

	/* select "   Hello" from left to right and copy it */
	tsm_screen_selection_reset(screen);
	tsm_screen_selection_start(screen, 0, 1);
	tsm_screen_selection_target(screen, 7, 1);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("   Hello", str);
	free(str);
	str = NULL;

	/* select "Hello World!" from right to left and copy it */
	tsm_screen_selection_reset(screen);
	tsm_screen_selection_start(screen, 14, 1);
	tsm_screen_selection_target(screen, 3, 1);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("Hello World!", str);
	free(str);
	str = NULL;

	/* select "Hello" from right to left and copy it */
	tsm_screen_selection_reset(screen);
	tsm_screen_selection_start(screen, 7, 1);
	tsm_screen_selection_target(screen, 3, 1);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("Hello", str);
	free(str);
	str = NULL;

	tsm_screen_unref(screen);
	screen = NULL;
}
END_TEST

START_TEST(test_screen_copy_line_sb_scrolled)
{
	struct tsm_screen *screen;
	int r, i;
	char *str = NULL;

	r = tsm_screen_new(&screen, NULL, NULL);
	ck_assert_int_eq(r, 0);

	r = tsm_screen_resize(screen, 80, 40);
	ck_assert_int_eq(r, 0);

	/*
	 * Create a selection of text in one line on the main screen, scroll it
	 * into the sb and copy it.
	 */

	tsm_screen_set_max_sb(screen, 10);

	write_string(screen, "   Hello World!");

	/* select "Hello World!" from left to right */
	tsm_screen_selection_start(screen, 3, 0);
	tsm_screen_selection_target(screen, 14, 0);

	ck_assert_int_eq(screen->sel_start.x, 3);
	ck_assert_int_eq(screen->sel_start.y, 0);
	ck_assert_int_eq(screen->sel_end.x, 14);
	ck_assert_int_eq(screen->sel_end.y, 0);

	/* force the selected text to scroll up */
	for (int i = 0; i < 40; i++) {
		tsm_screen_newline(screen);
	}

	ck_assert_int_eq(screen->sel_start.x, 3);
	ck_assert_int_eq(screen->sel_start.y, -1);
	ck_assert_ptr_ne(screen->sel_start.line, NULL);
	ck_assert_int_eq(screen->sel_end.x, 14);
	ck_assert_int_eq(screen->sel_end.y, -1);
	ck_assert_ptr_ne(screen->sel_end.line, NULL);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("Hello World!", str);
	free(str);
	str = NULL;

	/*
	 * Create a selection of the text in the sb, scroll it further into the
	 * sb and copy it.
	 */

	tsm_screen_selection_reset(screen);
	tsm_screen_sb_up(screen, 1);

	tsm_screen_selection_start(screen, 3, 0);
	tsm_screen_selection_target(screen, 14, 0);

	ck_assert_int_eq(screen->sel_start.x, 3);
	ck_assert_int_eq(screen->sel_start.y, 0);
	ck_assert_ptr_ne(screen->sel_start.line, NULL);
	ck_assert_int_eq(screen->sel_end.x, 14);
	ck_assert_int_eq(screen->sel_end.y, 0);
	ck_assert_ptr_ne(screen->sel_end.line, NULL);

	tsm_screen_newline(screen);
	tsm_screen_newline(screen);
	tsm_screen_newline(screen);

	ck_assert_int_eq(screen->sel_start.x, 3);
	ck_assert_int_eq(screen->sel_start.y, 0);
	ck_assert_ptr_ne(screen->sel_start.line, NULL);
	ck_assert_int_eq(screen->sel_end.x, 14);
	ck_assert_int_eq(screen->sel_end.y, 0);
	ck_assert_ptr_ne(screen->sel_end.line, NULL);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("Hello World!", str);
	free(str);
	str = NULL;

	tsm_screen_unref(screen);
	screen = NULL;
}
END_TEST

START_TEST(test_screen_copy_line_sb_scrolled_invalid)
{
	struct tsm_screen *screen;
	int r, i;
	char *str = NULL;

	r = tsm_screen_new(&screen, NULL, NULL);
	ck_assert_int_eq(r, 0);

	r = tsm_screen_resize(screen, 80, 40);
	ck_assert_int_eq(r, 0);

	/*
	 * Disable the sb. If a selection is moved out of the sb it becomes invalid.
	 * Copying it should return an empty string.
	 */
	tsm_screen_set_max_sb(screen, 0);

	write_string(screen, "   Hello World!");

	/* select "Hello World!" from left to right */
	tsm_screen_selection_start(screen, 3, 0);
	tsm_screen_selection_target(screen, 14, 0);

	ck_assert_int_eq(screen->sel_start.x, 3);
	ck_assert_int_eq(screen->sel_start.y, 0);
	ck_assert_int_eq(screen->sel_end.x, 14);
	ck_assert_int_eq(screen->sel_end.y, 0);

	/* force the selected text to scroll up */
	for (int i = 0; i < 40; i++) {
		tsm_screen_newline(screen);
	}

	/*
	 * sel_start.y == -1, sel_start.line == NULL
	 * sel_end.y == -1, sel_end.line == NULL
	 *
	 * => Invalid selection
	 */
	ck_assert_int_eq(screen->sel_start.x, 3);
	ck_assert_int_eq(screen->sel_start.y, -1);
	ck_assert_ptr_eq(screen->sel_start.line, NULL);
	ck_assert_int_eq(screen->sel_end.x, 14);
	ck_assert_int_eq(screen->sel_end.y, -1);
	ck_assert_ptr_eq(screen->sel_end.line, NULL);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("", str);
	free(str);
	str = NULL;

	tsm_screen_unref(screen);
	screen = NULL;
}
END_TEST

START_TEST(test_screen_copy_lines_sb)
{
	struct tsm_screen *screen;
	int r;
	char *str = NULL;
	int i;

	r = tsm_screen_new(&screen, NULL, NULL);
	ck_assert_int_eq(r, 0);

	tsm_screen_set_max_sb(screen, 10);

	r = tsm_screen_resize(screen, 80, 40);
	ck_assert_int_eq(r, 0);

	tsm_screen_newline(screen);
	write_string(screen, "   Hello World!");
	tsm_screen_newline(screen);
	write_string(screen, "This is a copy test");
	tsm_screen_newline(screen);
	write_string(screen, "for a selection with multiple lines.");
	tsm_screen_newline(screen);
	write_string(screen, "All of them are on screen (not in the sb).------");
	tsm_screen_newline(screen);

	write_string(screen, "Text not in SB");
	tsm_screen_newline(screen);
	write_string(screen, "More Text not in SB");
	tsm_screen_newline(screen);


	for (i = 0; i < 38; i++) {
		tsm_screen_newline(screen);
	}

	tsm_screen_sb_up(screen, 6);

	/* Select all text excluding the first 3 spaces and the trailing '-' chars from top left to bottom right and copy it */
	tsm_screen_selection_start(screen, 3, 1);
	tsm_screen_selection_target(screen, 41, 4);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("Hello World!\nThis is a copy test\nfor a selection with multiple lines.\nAll of them are on screen (not in the sb).", str);
	free(str);
	str = NULL;

	/* Select "This is a copy test\nfor a selection" from top left and to bottom right copy it */
	tsm_screen_reset(screen);
	tsm_screen_selection_start(screen, 0, 2);
	tsm_screen_selection_target(screen, 14, 3);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("This is a copy test\nfor a selection", str);
	free(str);
	str = NULL;

	/* Select all text excluding the first 3 spaces and the trailing '-' chars from bottom right to top left and copy it */
	tsm_screen_reset(screen);
	tsm_screen_selection_start(screen, 41, 4);
	tsm_screen_selection_target(screen, 3, 1);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("Hello World!\nThis is a copy test\nfor a selection with multiple lines.\nAll of them are on screen (not in the sb).", str);
	free(str);
	str = NULL;

	/* Select from scroll back buffer and the screen from top left to bottom right and copy it */
	tsm_screen_reset(screen);
	tsm_screen_selection_start(screen, 0, 4);
	tsm_screen_selection_target(screen, 18, 6);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("All of them are on screen (not in the sb).------\nText not in SB\nMore Text not in SB", str);
	free(str);
	str = NULL;

	/* Select from scroll back buffer and the screen from bottom right to top left and copy it */
	tsm_screen_reset(screen);
	tsm_screen_selection_start(screen, 18, 6);
	tsm_screen_selection_target(screen, 0, 4);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("All of them are on screen (not in the sb).------\nText not in SB\nMore Text not in SB", str);
	free(str);
	str = NULL;

	/* Select from scroll back buffer and the screen from bottom right to top left and copy it */
	tsm_screen_reset(screen);
	tsm_screen_selection_start(screen, 8, 6);
	tsm_screen_selection_target(screen, 7, 4);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("them are on screen (not in the sb).------\nText not in SB\nMore Text", str);
	free(str);
	str = NULL;

	tsm_screen_unref(screen);
	screen = NULL;
}
END_TEST

START_TEST(test_screen_copy_lines_sb_scrolled)
{
	struct tsm_screen *screen;
	int r, i;
	char *str = NULL;

	r = tsm_screen_new(&screen, NULL, NULL);
	ck_assert_int_eq(r, 0);

	r = tsm_screen_resize(screen, 80, 40);
	ck_assert_int_eq(r, 0);

	/*
	 * Create a selection of text on the main screen, scroll it into the sb
	 * and copy it.
	 */

	tsm_screen_set_max_sb(screen, 10);

	write_string(screen, "   Hello World!");
	tsm_screen_newline(screen);
	write_string(screen, "Line 2");
	tsm_screen_newline(screen);
	write_string(screen, "Line 3");

	/* select "Hello World!" from left to right */
	tsm_screen_selection_start(screen, 3, 0);
	tsm_screen_selection_target(screen, 5, 2);

	ck_assert_int_eq(screen->sel_start.x, 3);
	ck_assert_int_eq(screen->sel_start.y, 0);
	ck_assert_int_eq(screen->sel_end.x, 5);
	ck_assert_int_eq(screen->sel_end.y, 2);

	/* force the selected text to scroll into the sb */
	for (int i = 0; i < 40; i++) {
		tsm_screen_newline(screen);
	}

	ck_assert_int_eq(screen->sel_start.x, 3);
	ck_assert_int_eq(screen->sel_start.y, -1);
	ck_assert_ptr_ne(screen->sel_start.line, NULL);
	ck_assert_int_eq(screen->sel_end.x, 5);
	ck_assert_int_eq(screen->sel_end.y, -1);
	ck_assert_ptr_ne(screen->sel_end.line, NULL);
	ck_assert_ptr_ne(screen->sel_start.line, screen->sel_end.line);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("Hello World!\nLine 2\nLine 3", str);
	free(str);
	str = NULL;

	/*
	 * Create a selection of the text in the sb, scroll it further into
	 * the sb and copy it.
	 */

	tsm_screen_selection_reset(screen);
	tsm_screen_sb_up(screen, 3);

	tsm_screen_selection_start(screen, 3, 0);
	tsm_screen_selection_target(screen, 5, 2);

	ck_assert_int_eq(screen->sel_start.x, 3);
	ck_assert_int_eq(screen->sel_start.y, 0);
	ck_assert_int_eq(screen->sel_end.x, 5);
	ck_assert_int_eq(screen->sel_end.y, 0);

	tsm_screen_newline(screen);
	tsm_screen_newline(screen);
	tsm_screen_newline(screen);

	ck_assert_int_eq(screen->sel_start.x, 3);
	ck_assert_int_eq(screen->sel_start.y, 0);
	ck_assert_ptr_ne(screen->sel_start.line, NULL);
	ck_assert_int_eq(screen->sel_end.x, 5);
	ck_assert_int_eq(screen->sel_end.y, 0);
	ck_assert_ptr_ne(screen->sel_end.line, NULL);
	ck_assert_ptr_ne(screen->sel_start.line, screen->sel_end.line);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("Hello World!\nLine 2\nLine 3", str);
	free(str);
	str = NULL;

	tsm_screen_unref(screen);
	screen = NULL;
}
END_TEST

START_TEST(test_screen_copy_lines_sb_scrolled_cut_off)
{
	struct tsm_screen *screen;
	int r, i;
	char *str = NULL;

	r = tsm_screen_new(&screen, NULL, NULL);
	ck_assert_int_eq(r, 0);

	r = tsm_screen_resize(screen, 80, 40);
	ck_assert_int_eq(r, 0);

	/*
	 * Disable the sb. If a part of a selection is moved out of the sb it is
	 * cut off. Copying it should return the remaining part of the selection.
	 */
	tsm_screen_set_max_sb(screen, 0);

	write_string(screen, "   Hello World!");
	tsm_screen_newline(screen);
	write_string(screen, "Line 2");
	tsm_screen_newline(screen);
	write_string(screen, "Line 3");

	/* select "Hello World!" from left to right */
	tsm_screen_selection_start(screen, 3, 0);
	tsm_screen_selection_target(screen, 5, 2);

	ck_assert_int_eq(screen->sel_start.x, 3);
	ck_assert_int_eq(screen->sel_start.y, 0);
	ck_assert_int_eq(screen->sel_end.x, 5);
	ck_assert_int_eq(screen->sel_end.y, 2);

	/* force the selected text to scroll up */
	for (int i = 0; i < 39; i++) {
		tsm_screen_newline(screen);
	}

	ck_assert_int_eq(screen->sel_start.x, 3);
	ck_assert_int_eq(screen->sel_start.y, -1);
	ck_assert_ptr_eq(screen->sel_start.line, NULL);
	ck_assert_int_eq(screen->sel_end.x, 5);
	ck_assert_int_eq(screen->sel_end.y, 0);
	ck_assert_ptr_eq(screen->sel_end.line, NULL);

	r = tsm_screen_selection_copy(screen, &str);
	ck_assert_ptr_ne(NULL, str);
	ck_assert_str_eq("Line 3", str);
	free(str);
	str = NULL;

	tsm_screen_unref(screen);
	screen = NULL;
}
END_TEST

TEST_DEFINE_CASE(misc)
	TEST(test_screen_copy_incomplete)
	TEST(test_screen_copy_one_cell)
	TEST(test_screen_copy_line)
	TEST(test_screen_copy_line_scrolled)
	TEST(test_screen_copy_lines)
	TEST(test_screen_copy_lines_scrolled)
	TEST(test_screen_copy_line_sb)
	TEST(test_screen_copy_line_sb_scrolled)
	TEST(test_screen_copy_line_sb_scrolled_invalid)
	TEST(test_screen_copy_lines_sb)
	TEST(test_screen_copy_lines_sb_scrolled)
	TEST(test_screen_copy_lines_sb_scrolled_cut_off)
TEST_END_CASE

TEST_DEFINE(
	TEST_SUITE(selection,
		TEST_CASE(misc),
		TEST_END
	)
)
