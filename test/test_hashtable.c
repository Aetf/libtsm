/*
 * TSM - Hashtable Tests
 *
 * Copyright (c) 2012-2013 David Herrmann <dh.herrmann@gmail.com>
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

/*
 * Hashtable Tests
 * Stress tests for the internal hashtable implementation.
 */

#include "test_common.h"

START_TEST(test_hashtable_setup_valid)
{
	struct shl_hashtable *t = NULL;
	int r;

	r = shl_hashtable_new(&t, shl_direct_hash, shl_direct_equal,
			      NULL, NULL);
	ck_assert(r == 0);
	ck_assert(t != NULL);

	shl_hashtable_free(t);
}
END_TEST

START_TEST(test_hashtable_setup_invalid)
{
	struct shl_hashtable *t = TEST_INVALID_PTR;
	int r;

	r = shl_hashtable_new(NULL, shl_direct_hash, shl_direct_equal,
			      NULL, NULL);
	ck_assert(r != 0);
	ck_assert(t == TEST_INVALID_PTR);

	r = shl_hashtable_new(&t, NULL, shl_direct_equal,
			      NULL, NULL);
	ck_assert(r != 0);
	ck_assert(t == TEST_INVALID_PTR);

	r = shl_hashtable_new(&t, shl_direct_hash, NULL,
			      NULL, NULL);
	ck_assert(r != 0);
	ck_assert(t == TEST_INVALID_PTR);

	r = shl_hashtable_new(&t, NULL, NULL,
			      NULL, NULL);
	ck_assert(r != 0);
	ck_assert(t == TEST_INVALID_PTR);

	r = shl_hashtable_new(NULL, NULL, NULL,
			      NULL, NULL);
	ck_assert(r != 0);
	ck_assert(t == TEST_INVALID_PTR);
}
END_TEST

TEST_DEFINE_CASE(setup)
	TEST(test_hashtable_setup_valid)
	TEST(test_hashtable_setup_invalid)
TEST_END_CASE

START_TEST(test_hashtable_add_1)
{
}
END_TEST

TEST_DEFINE_CASE(add)
	TEST(test_hashtable_add_1)
TEST_END_CASE

TEST_DEFINE(
	TEST_SUITE(hashtable,
		TEST_CASE(setup),
		TEST_CASE(add),
		TEST_END
	)
)
