/*
 * TSM - VTE State Machine Tests
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

#include "test_common.h"
#include "libtsm.h"
#include "libtsm-int.h"

char write_buffer[512];

bool mouse_cb_called = false;
unsigned int mouse_track_mode = 0;
bool mouse_track_pixels = false;

static void write_cb(struct tsm_vte *vte, const char *u8, size_t len, void *data)
{
	ck_assert_ptr_ne(vte, NULL);
	ck_assert_ptr_ne(u8, NULL);

	memcpy(&write_buffer, u8, len);
	write_buffer[len] = '\0';
}

static void mouse_cb(struct tsm_vte *vte, enum tsm_mouse_track_mode track_mode, bool track_pixels, void *data)
{
	mouse_cb_called = true;
	mouse_track_mode = track_mode;
	mouse_track_pixels = track_pixels;
}

struct tsm_screen *screen;
struct tsm_vte *vte;

void setup()
{
	int r;

	r = tsm_screen_new(&screen, NULL, NULL);
	ck_assert_int_eq(r, 0);

	r = tsm_vte_new(&vte, screen, write_cb, NULL, NULL, NULL);
	ck_assert_int_eq(r, 0);

	tsm_vte_set_mouse_cb(vte, mouse_cb, NULL);

	mouse_cb_called = false;
	mouse_track_mode = 0;
	mouse_track_pixels = false;

	bzero(&write_buffer, sizeof(write_buffer));
}

void teardown()
{
	tsm_vte_unref(vte);
	vte = NULL;

	tsm_screen_unref(screen);
	screen = NULL;
}

START_TEST(test_mouse_cb_x10)
{
	char *msg;

	/* Set X10 Mode */
	msg = "\e[?9h";
	tsm_vte_input(vte, msg, strlen(msg));

	ck_assert(mouse_cb_called);
	ck_assert_int_eq(mouse_track_mode, TSM_MOUSE_TRACK_BTN);
	ck_assert(!mouse_track_pixels);
}
END_TEST

START_TEST(test_mouse_x10)
{
	char *msg;
	char *expected;
	int r;

	/* Set X10 Mode */
	msg = "\e[?9h";
	tsm_vte_input(vte, msg, strlen(msg));

	/* left click on left upper cell (0, 0) should be translated to (1, 1) in output */
	tsm_vte_handle_mouse(vte, 0, 0, 0, 0, 0, TSM_MOUSE_EVENT_PRESSED, 0);
	expected = "\e[M !!";
	r = memcmp(&write_buffer, expected, strlen(expected));
	ck_assert_int_eq(r, 0);

	/* right click on (0, 0) */
	bzero(&write_buffer, sizeof(write_buffer));
	tsm_vte_handle_mouse(vte, 0, 0, 0, 0, 2, TSM_MOUSE_EVENT_PRESSED, 0);
	expected = "\e[M\"!!";
	r = memcmp(&write_buffer, expected, strlen(expected));
	ck_assert_int_eq(r, 0);

	/* middle click on (0, 0) */
	bzero(&write_buffer, sizeof(write_buffer));
	tsm_vte_handle_mouse(vte, 0, 0, 0, 0, 1, TSM_MOUSE_EVENT_PRESSED, 0);
	expected = "\e[M!!!";
	r = memcmp(&write_buffer, expected, strlen(expected));
	ck_assert_int_eq(r, 0);

	/* left click out of range (299, 279) */
	bzero(&write_buffer, sizeof(write_buffer));
	tsm_vte_handle_mouse(vte, 299, 279, 0, 0, 0, TSM_MOUSE_EVENT_PRESSED, 0);
	expected = "\e[M \xff\xff";
	r = memcmp(&write_buffer, expected, strlen(expected));
	ck_assert_int_eq(r, 0);
}
END_TEST

START_TEST(test_mouse_cb_sgr)
{
	char *msg;

	/* Set SGR Mode */
	msg = "\e[?1006h";
	tsm_vte_input(vte, msg, strlen(msg));

	/* mouse_cb should not be called if event type is not set */
	ck_assert(!mouse_cb_called);

	/* reset for next test */
	mouse_cb_called = false;
	mouse_track_mode = 0;
	mouse_track_pixels = false;

	/* Set Button Events */
	bzero(&write_buffer, sizeof(write_buffer));
	msg = "\e[?1002h";
	tsm_vte_input(vte, msg, strlen(msg));

	ck_assert(mouse_cb_called);
	ck_assert_int_eq(mouse_track_mode, TSM_MOUSE_TRACK_BTN);
	ck_assert(!mouse_track_pixels);
}
END_TEST

START_TEST(test_mouse_sgr)
{
	char *expected;
	char *msg;
	int r;

	/* Set SGR Mode and only notify for mouse button clicks */
	msg = "\e[?1006h\e[?1002h";
	tsm_vte_input(vte, msg, strlen(msg));

	/* left click on left upper cell (0, 0) should be translated to (1, 1) in output */
	tsm_vte_handle_mouse(vte, 0, 0, 0, 0, 0, TSM_MOUSE_EVENT_PRESSED, 0);
	expected = "\e[<0;1;1M";
	r = memcmp(&write_buffer, expected, strlen(expected));
	ck_assert_int_eq(r, 0);

	/* button release event for (1, 1) */
	bzero(&write_buffer, sizeof(write_buffer));
	tsm_vte_handle_mouse(vte, 0, 0, 0, 0, 0, TSM_MOUSE_EVENT_RELEASED, 0);
	expected = "\e[<0;1;1m";
	r = memcmp(&write_buffer, expected, strlen(expected));
	ck_assert_int_eq(r, 0);

	/* button 1 (middle mouse button) */
	bzero(&write_buffer, sizeof(write_buffer));
	tsm_vte_handle_mouse(vte, 0, 0, 0, 0, 1, TSM_MOUSE_EVENT_PRESSED, 0);
	expected = "\e[<1;1;1M";
	r = memcmp(&write_buffer, expected, strlen(expected));
	ck_assert_int_eq(r, 0);

	/* button 2 (right mouse button) */
	bzero(&write_buffer, sizeof(write_buffer));
	tsm_vte_handle_mouse(vte, 0, 0, 0, 0, 2, TSM_MOUSE_EVENT_PRESSED, 0);
	expected = "\e[<2;1;1M";
	r = memcmp(&write_buffer, expected, strlen(expected));
	ck_assert_int_eq(r, 0);

	/* button 4 (mouse wheel up scroll) */
	bzero(&write_buffer, sizeof(write_buffer));
	tsm_vte_handle_mouse(vte, 0, 0, 0, 0, 4, TSM_MOUSE_EVENT_PRESSED, 0);
	expected = "\e[<64;1;1M";
	r = memcmp(&write_buffer, expected, strlen(expected));
	ck_assert_int_eq(r, 0);

	/* button 5 (mouse wheel down scroll) */
	bzero(&write_buffer, sizeof(write_buffer));
	tsm_vte_handle_mouse(vte, 0, 0, 0, 0, 5, TSM_MOUSE_EVENT_PRESSED, 0);
	expected = "\e[<65;1;1M";
	r = memcmp(&write_buffer, expected, strlen(expected));
	ck_assert_int_eq(r, 0);

	/* check for (50, 120) */
	bzero(&write_buffer, sizeof(write_buffer));
	tsm_vte_handle_mouse(vte, 49, 119, 0, 0, 0, TSM_MOUSE_EVENT_PRESSED, 0);
	expected = "\e[<0;50;120M";
	r = memcmp(&write_buffer, expected, strlen(expected));
	ck_assert_int_eq(r, 0);
}
END_TEST

START_TEST(test_mouse_sgr_cell_change)
{
	char *msg;
	char *expected;
	int r;

	/* Set SGR Mode and only notify for mouse button clicks */
	msg = "\e[?1006h\e[?1003h";
	tsm_vte_input(vte, msg, strlen(msg));

	/* move over (0, 0) */
	tsm_vte_handle_mouse(vte, 0, 0, 0, 0, 0, TSM_MOUSE_EVENT_MOVED, 0);
	expected = "\e[<35;1;1M";
	r = memcmp(&write_buffer, expected, strlen(expected));
	ck_assert_int_eq(r, 0);

	/* repeated reportings of the same cell should be ignored */
	bzero(&write_buffer, sizeof(write_buffer));
	tsm_vte_handle_mouse(vte, 0, 0, 0, 0, 0, TSM_MOUSE_EVENT_MOVED, 0);
	ck_assert_int_eq(write_buffer[0], 0);

	/* different cells must be reported */
	bzero(&write_buffer, sizeof(write_buffer));
	tsm_vte_handle_mouse(vte, 1, 1, 0, 0, 0, TSM_MOUSE_EVENT_MOVED, 0);
	expected = "\e[<35;2;2M";
	r = memcmp(&write_buffer, expected, strlen(expected));
	ck_assert_int_eq(r, 0);

	/* a click must be reported in all cases */
	bzero(&write_buffer, sizeof(write_buffer));
	tsm_vte_handle_mouse(vte, 1, 1, 0, 0, 0, TSM_MOUSE_EVENT_PRESSED, 0);
	expected = "\e[<0;2;2M";
	r = memcmp(&write_buffer, expected, strlen(expected));
	ck_assert_int_eq(r, 0);
}
END_TEST

START_TEST(test_mouse_cb_pixels)
{
	char *msg;

	/* Set Pixel Mode */
	msg = "\e[?1016h";
	tsm_vte_input(vte, msg, strlen(msg));

	/* mouse_cb should not be called if event type is not set */
	ck_assert(!mouse_cb_called);

	/* reset for next test */
	mouse_cb_called = false;
	mouse_track_mode = 0;
	mouse_track_pixels = false;

	/* Set Button Events */
	bzero(&write_buffer, sizeof(write_buffer));
	msg = "\e[?1003h";
	tsm_vte_input(vte, msg, strlen(msg));

	ck_assert(mouse_cb_called);
	ck_assert_int_eq(mouse_track_mode, TSM_MOUSE_TRACK_ANY);
	ck_assert(mouse_track_pixels);
}
END_TEST

START_TEST(test_mouse_pixels)
{
	char *msg;
	char *expected;
	int r;

	/* Set SGR Mode and only notify for mouse button clicks */
	msg = "\e[?1016h\e[?1003h";
	tsm_vte_input(vte, msg, strlen(msg));

	tsm_vte_handle_mouse(vte, 0, 0, 236, 120, 0, TSM_MOUSE_EVENT_MOVED, 0);
	expected = "\e[<35;236;120M";
	r = memcmp(&write_buffer, expected, strlen(expected));
	ck_assert_int_eq(r, 0);

	bzero(&write_buffer, sizeof(write_buffer));
	tsm_vte_handle_mouse(vte, 0, 0, 236, 120, 0, TSM_MOUSE_EVENT_PRESSED, 0);
	expected = "\e[<0;236;120M";
	r = memcmp(&write_buffer, expected, strlen(expected));
	ck_assert_int_eq(r, 0);

	bzero(&write_buffer, sizeof(write_buffer));
	tsm_vte_handle_mouse(vte, 0, 0, 236, 120, 0, TSM_MOUSE_EVENT_RELEASED, 0);
	expected = "\e[<0;236;120m";
	r = memcmp(&write_buffer, expected, strlen(expected));
	ck_assert_int_eq(r, 0);
}
END_TEST

TEST_DEFINE_CASE(tests_x10)
	CHECKED_FIXTURE(setup, teardown)
	TEST(test_mouse_cb_x10)
	TEST(test_mouse_x10)
TEST_END_CASE

TEST_DEFINE_CASE(tests_sgr)
	CHECKED_FIXTURE(setup, teardown)
	TEST(test_mouse_cb_sgr)
	TEST(test_mouse_sgr)
	TEST(test_mouse_sgr_cell_change)
TEST_END_CASE

TEST_DEFINE_CASE(tests_sgr_pixels)
	CHECKED_FIXTURE(setup, teardown)
	TEST(test_mouse_cb_pixels)
	TEST(test_mouse_pixels)
TEST_END_CASE

// clang-format off
TEST_DEFINE(
	TEST_SUITE(vte_mouse,
		TEST_CASE(tests_x10),
		TEST_CASE(tests_sgr),
		TEST_CASE(tests_sgr_pixels),
		TEST_END
	)
)
// clang-format on
