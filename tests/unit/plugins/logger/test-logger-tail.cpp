/*
 * test-logger-tail.cpp - test logger tail functions
 *
 * Copyright (C) 2023-2025 Sébastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CppUTest/TestHarness.h"

#include "tests/tests.h"

extern "C"
{
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "src/core/core-arraylist.h"
#include "src/core/core-string.h"
#include "src/plugins/logger/logger-tail.h"

extern const char *logger_tail_last_eol (const char *string_start,
                                         const char *string_ptr);
}

TEST_GROUP(LoggerTail)
{
};

/*
 * Tests functions:
 *   logger_tail_last_eol
 */

TEST(LoggerTail, LastEol)
{
    const char *str = "abc\ndef\rghi";

    STRCMP_EQUAL(NULL, logger_tail_last_eol (NULL, NULL));
    STRCMP_EQUAL(NULL, logger_tail_last_eol (NULL, ""));
    STRCMP_EQUAL(NULL, logger_tail_last_eol ("", NULL));
    STRCMP_EQUAL(NULL, logger_tail_last_eol ("", ""));
    STRCMP_EQUAL(NULL, logger_tail_last_eol (str + 9, str));

    STRCMP_EQUAL(NULL, logger_tail_last_eol (str, str));
    STRCMP_EQUAL(NULL, logger_tail_last_eol (str, str + 1));
    STRCMP_EQUAL(NULL, logger_tail_last_eol (str, str + 2));

    STRCMP_EQUAL(str + 3, logger_tail_last_eol (str, str + 3));
    STRCMP_EQUAL(str + 3, logger_tail_last_eol (str, str + 4));
    STRCMP_EQUAL(str + 3, logger_tail_last_eol (str, str + 5));
    STRCMP_EQUAL(str + 3, logger_tail_last_eol (str, str + 6));

    STRCMP_EQUAL(str + 7, logger_tail_last_eol (str, str + 7));
    STRCMP_EQUAL(str + 7, logger_tail_last_eol (str, str + 8));
    STRCMP_EQUAL(str + 7, logger_tail_last_eol (str, str + 9));
    STRCMP_EQUAL(str + 7, logger_tail_last_eol (str, str + 10));
}

/*
 * Tests functions:
 *   logger_tail_lines_cmp_cb
 *   logger_tail_lines_free_cb
 *   logger_tail_file
 */

TEST(LoggerTail, File)
{
    const char *content_3_lines = "line 1\nline 2\nline 3\n";
    const char *content_5_lines = "line 1\nline 2\n\nline 3\n\n";
    char *filename, line[4096];
    struct t_arraylist *lines;
    FILE *file;
    int i;

    POINTERS_EQUAL(NULL, logger_tail_file (NULL, 0));
    POINTERS_EQUAL(NULL, logger_tail_file (NULL, 1));

    /* write a small test file */
    filename = string_eval_path_home ("${weechat_data_dir}/test_file.txt",
                                      NULL, NULL, NULL);
    file = fopen (filename, "w");
    fwrite (content_3_lines, 1, strlen (content_3_lines), file);
    fflush (file);
    fclose (file);

    POINTERS_EQUAL(NULL, logger_tail_file (filename, 0));

    lines = logger_tail_file (filename, 1);
    CHECK(lines);
    LONGS_EQUAL(1, arraylist_size (lines));
    STRCMP_EQUAL("line 3", (const char *)arraylist_get (lines, 0));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 2);
    CHECK(lines);
    LONGS_EQUAL(2, arraylist_size (lines));
    STRCMP_EQUAL("line 2", (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("line 3", (const char *)arraylist_get (lines, 1));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 3);
    CHECK(lines);
    LONGS_EQUAL(3, arraylist_size (lines));
    STRCMP_EQUAL("line 1", (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("line 2", (const char *)arraylist_get (lines, 1));
    STRCMP_EQUAL("line 3", (const char *)arraylist_get (lines, 2));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 4);
    CHECK(lines);
    LONGS_EQUAL(3, arraylist_size (lines));
    STRCMP_EQUAL("line 1", (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("line 2", (const char *)arraylist_get (lines, 1));
    STRCMP_EQUAL("line 3", (const char *)arraylist_get (lines, 2));
    arraylist_free (lines);

    unlink (filename);
    free (filename);

    /* write a small test file, with empty lines */
    filename = string_eval_path_home ("${weechat_data_dir}/test_file.txt",
                                      NULL, NULL, NULL);
    file = fopen (filename, "w");
    fwrite (content_5_lines, 1, strlen (content_5_lines), file);
    fflush (file);
    fclose (file);

    POINTERS_EQUAL(NULL, logger_tail_file (filename, 0));

    lines = logger_tail_file (filename, 1);
    CHECK(lines);
    LONGS_EQUAL(1, arraylist_size (lines));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 0));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 2);
    CHECK(lines);
    LONGS_EQUAL(2, arraylist_size (lines));
    STRCMP_EQUAL("line 3", (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 1));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 3);
    CHECK(lines);
    LONGS_EQUAL(3, arraylist_size (lines));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("line 3", (const char *)arraylist_get (lines, 1));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 2));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 4);
    CHECK(lines);
    LONGS_EQUAL(4, arraylist_size (lines));
    STRCMP_EQUAL("line 2", (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 1));
    STRCMP_EQUAL("line 3", (const char *)arraylist_get (lines, 2));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 3));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 5);
    CHECK(lines);
    LONGS_EQUAL(5, arraylist_size (lines));
    STRCMP_EQUAL("line 1", (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("line 2", (const char *)arraylist_get (lines, 1));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 2));
    STRCMP_EQUAL("line 3", (const char *)arraylist_get (lines, 3));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 4));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 6);
    CHECK(lines);
    LONGS_EQUAL(5, arraylist_size (lines));
    STRCMP_EQUAL("line 1", (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("line 2", (const char *)arraylist_get (lines, 1));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 2));
    STRCMP_EQUAL("line 3", (const char *)arraylist_get (lines, 3));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 4));
    arraylist_free (lines);

    unlink (filename);
    free (filename);

    /* write a bigger test file */
    filename = string_eval_path_home ("${weechat_data_dir}/test_file.txt",
                                      NULL, NULL, NULL);
    file = fopen (filename, "w");
    for (i = 0; i < 1000; i++)
    {
        snprintf (line, sizeof (line), "this is a test, line %d\n", i + 1);
        fwrite (line, 1, strlen (line), file);
    }
    fflush (file);
    fclose (file);

    POINTERS_EQUAL(NULL, logger_tail_file (filename, 0));

    lines = logger_tail_file (filename, 1);
    CHECK(lines);
    LONGS_EQUAL(1, arraylist_size (lines));
    STRCMP_EQUAL("this is a test, line 1000", (const char *)arraylist_get (lines, 0));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 2);
    CHECK(lines);
    LONGS_EQUAL(2, arraylist_size (lines));
    STRCMP_EQUAL("this is a test, line 999", (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("this is a test, line 1000", (const char *)arraylist_get (lines, 1));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 3);
    CHECK(lines);
    LONGS_EQUAL(3, arraylist_size (lines));
    STRCMP_EQUAL("this is a test, line 998", (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("this is a test, line 999", (const char *)arraylist_get (lines, 1));
    STRCMP_EQUAL("this is a test, line 1000", (const char *)arraylist_get (lines, 2));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 4);
    CHECK(lines);
    LONGS_EQUAL(4, arraylist_size (lines));
    STRCMP_EQUAL("this is a test, line 997", (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("this is a test, line 998", (const char *)arraylist_get (lines, 1));
    STRCMP_EQUAL("this is a test, line 999", (const char *)arraylist_get (lines, 2));
    STRCMP_EQUAL("this is a test, line 1000", (const char *)arraylist_get (lines, 3));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 1000);
    CHECK(lines);
    LONGS_EQUAL(1000, arraylist_size (lines));
    STRCMP_EQUAL("this is a test, line 1", (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("this is a test, line 2", (const char *)arraylist_get (lines, 1));
    STRCMP_EQUAL("this is a test, line 3", (const char *)arraylist_get (lines, 2));
    STRCMP_EQUAL("this is a test, line 4", (const char *)arraylist_get (lines, 3));
    STRCMP_EQUAL("this is a test, line 998", (const char *)arraylist_get (lines, 997));
    STRCMP_EQUAL("this is a test, line 999", (const char *)arraylist_get (lines, 998));
    STRCMP_EQUAL("this is a test, line 1000", (const char *)arraylist_get (lines, 999));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 2000);
    CHECK(lines);
    LONGS_EQUAL(1000, arraylist_size (lines));
    STRCMP_EQUAL("this is a test, line 1", (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("this is a test, line 2", (const char *)arraylist_get (lines, 1));
    STRCMP_EQUAL("this is a test, line 3", (const char *)arraylist_get (lines, 2));
    STRCMP_EQUAL("this is a test, line 4", (const char *)arraylist_get (lines, 3));
    STRCMP_EQUAL("this is a test, line 998", (const char *)arraylist_get (lines, 997));
    STRCMP_EQUAL("this is a test, line 999", (const char *)arraylist_get (lines, 998));
    STRCMP_EQUAL("this is a test, line 1000", (const char *)arraylist_get (lines, 999));
    arraylist_free (lines);

    unlink (filename);
    free (filename);

    /* write a bigger test file, with empty lines */
    filename = string_eval_path_home ("${weechat_data_dir}/test_file.txt",
                                      NULL, NULL, NULL);
    file = fopen (filename, "w");
    for (i = 0; i < 1000; i++)
    {
        snprintf (line, sizeof (line), "this is a test, line %d\n\n", i + 1);
        fwrite (line, 1, strlen (line), file);
    }
    fflush (file);
    fclose (file);

    POINTERS_EQUAL(NULL, logger_tail_file (filename, 0));

    lines = logger_tail_file (filename, 1);
    CHECK(lines);
    LONGS_EQUAL(1, arraylist_size (lines));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 0));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 2);
    CHECK(lines);
    LONGS_EQUAL(2, arraylist_size (lines));
    STRCMP_EQUAL("this is a test, line 1000", (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 1));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 3);
    CHECK(lines);
    LONGS_EQUAL(3, arraylist_size (lines));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("this is a test, line 1000", (const char *)arraylist_get (lines, 1));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 2));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 4);
    CHECK(lines);
    LONGS_EQUAL(4, arraylist_size (lines));
    STRCMP_EQUAL("this is a test, line 999", (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 1));
    STRCMP_EQUAL("this is a test, line 1000", (const char *)arraylist_get (lines, 2));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 3));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 2000);
    CHECK(lines);
    LONGS_EQUAL(2000, arraylist_size (lines));
    STRCMP_EQUAL("this is a test, line 1", (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 1));
    STRCMP_EQUAL("this is a test, line 2", (const char *)arraylist_get (lines, 2));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 3));
    STRCMP_EQUAL("this is a test, line 3", (const char *)arraylist_get (lines, 4));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 5));
    STRCMP_EQUAL("this is a test, line 998", (const char *)arraylist_get (lines, 1994));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 1995));
    STRCMP_EQUAL("this is a test, line 999", (const char *)arraylist_get (lines, 1996));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 1997));
    STRCMP_EQUAL("this is a test, line 1000", (const char *)arraylist_get (lines, 1998));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 1999));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 4000);
    CHECK(lines);
    LONGS_EQUAL(2000, arraylist_size (lines));
    STRCMP_EQUAL("this is a test, line 1", (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 1));
    STRCMP_EQUAL("this is a test, line 2", (const char *)arraylist_get (lines, 2));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 3));
    STRCMP_EQUAL("this is a test, line 3", (const char *)arraylist_get (lines, 4));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 5));
    STRCMP_EQUAL("this is a test, line 998", (const char *)arraylist_get (lines, 1994));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 1995));
    STRCMP_EQUAL("this is a test, line 999", (const char *)arraylist_get (lines, 1996));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 1997));
    STRCMP_EQUAL("this is a test, line 1000", (const char *)arraylist_get (lines, 1998));
    STRCMP_EQUAL("", (const char *)arraylist_get (lines, 1999));
    arraylist_free (lines);

    unlink (filename);
    free (filename);
}
