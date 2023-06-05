/*
 * test-logger-tail.cpp - test logger tail functions
 *
 * Copyright (C) 2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/core/wee-arraylist.h"
#include "src/core/wee-string.h"
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

TEST(LoggerTail, LoggerTailLastEol)
{
    const char *str = "abc\ndef\rghi";

    POINTERS_EQUAL(NULL, logger_tail_last_eol (NULL, NULL));
    POINTERS_EQUAL(NULL, logger_tail_last_eol (NULL, ""));
    POINTERS_EQUAL(NULL, logger_tail_last_eol ("", NULL));
    POINTERS_EQUAL(NULL, logger_tail_last_eol ("", ""));
    POINTERS_EQUAL(NULL, logger_tail_last_eol (str + 9, str));

    POINTERS_EQUAL(NULL, logger_tail_last_eol (str, str));
    POINTERS_EQUAL(NULL, logger_tail_last_eol (str, str + 1));
    POINTERS_EQUAL(NULL, logger_tail_last_eol (str, str + 2));

    POINTERS_EQUAL(str + 3, logger_tail_last_eol (str, str + 3));
    POINTERS_EQUAL(str + 3, logger_tail_last_eol (str, str + 4));
    POINTERS_EQUAL(str + 3, logger_tail_last_eol (str, str + 5));
    POINTERS_EQUAL(str + 3, logger_tail_last_eol (str, str + 6));

    POINTERS_EQUAL(str + 7, logger_tail_last_eol (str, str + 7));
    POINTERS_EQUAL(str + 7, logger_tail_last_eol (str, str + 8));
    POINTERS_EQUAL(str + 7, logger_tail_last_eol (str, str + 9));
    POINTERS_EQUAL(str + 7, logger_tail_last_eol (str, str + 10));
}

/*
 * Tests functions:
 *   logger_tail_lines_cmp_cb
 *   logger_tail_lines_free_cb
 *   logger_tail_file
 */

TEST(LoggerTail, LoggerTailFile)
{
    char *filename;
    FILE *file;
    const char *content = "line 1\nline 2\nline 3";
    struct t_arraylist *lines;

    /* write a test file */
    filename = string_eval_path_home ("${weechat_data_dir}/test_file.txt",
                                      NULL, NULL, NULL);
    file = fopen (filename, "w");
    fwrite (content, 1, strlen (content), file);
    fflush (file);
    fclose (file);

    POINTERS_EQUAL(NULL, logger_tail_file (NULL, 0));
    POINTERS_EQUAL(NULL, logger_tail_file (NULL, 1));
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
}
