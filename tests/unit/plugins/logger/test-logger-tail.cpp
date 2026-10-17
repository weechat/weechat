/*
 * SPDX-FileCopyrightText: 2023-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test logger tail functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "src/core/core-arraylist.h"
#include "src/core/core-string.h"
#include "src/plugins/logger/logger-tail.h"

extern const char *logger_tail_last_eol (const char *string_start,
                                         const char *string_ptr);
extern char *logger_tail_read_line (int fd, off_t start, off_t end);
}

TEST_GROUP(LoggerTail)
{
};

/*
 * Test functions:
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
 * Test functions:
 *   logger_tail_read_line
 */

TEST(LoggerTail, ReadLine)
{
    const char *content = "line 1\nline 2\n";
    char *filename, *str;
    FILE *file;
    int fd;

    POINTERS_EQUAL(NULL, logger_tail_read_line (-1, 0, 0));

    filename = string_eval_path_home ("${weechat_data_dir}/test_file.txt",
                                      NULL, NULL, NULL);
    file = fopen (filename, "w");
    fwrite (content, 1, strlen (content), file);
    fflush (file);
    fclose (file);

    fd = open (filename, O_RDONLY);
    CHECK(fd >= 0);

    POINTERS_EQUAL(NULL, logger_tail_read_line (fd, -1, 0));
    POINTERS_EQUAL(NULL, logger_tail_read_line (fd, 2, 1));

    /* Read beyond end of file. */
    POINTERS_EQUAL(NULL, logger_tail_read_line (fd, 7, 20));

    WEE_TEST_STR("", logger_tail_read_line (fd, 0, 0));
    WEE_TEST_STR("", logger_tail_read_line (fd, 5, 5));
    WEE_TEST_STR("l", logger_tail_read_line (fd, 0, 1));
    WEE_TEST_STR("line 1", logger_tail_read_line (fd, 0, 6));
    WEE_TEST_STR("line 2", logger_tail_read_line (fd, 7, 13));
    WEE_TEST_STR("line 2\n", logger_tail_read_line (fd, 7, 14));
    WEE_TEST_STR("line 1\nline 2\n", logger_tail_read_line (fd, 0, 14));

    close (fd);

    unlink (filename);
    free (filename);
}

/*
 * Test functions:
 *   logger_tail_lines_cmp_cb
 *   logger_tail_lines_free_cb
 *   logger_tail_file
 */

TEST(LoggerTail, File)
{
    const char *content_3_lines = "line 1\nline 2\nline 3\n";
    const char *content_5_lines = "line 1\nline 2\n\nline 3\n\n";
    char *filename, line[4096], *long_line;
    struct t_arraylist *lines;
    FILE *file;
    int i;

    POINTERS_EQUAL(NULL, logger_tail_file (NULL, 0));
    POINTERS_EQUAL(NULL, logger_tail_file (NULL, 1));

    /* Write a small test file. */
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

    /* Write a small test file, with empty lines. */
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

    /* Write a bigger test file. */
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

    /* Write a bigger test file, with empty lines. */
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

    /* Write a test file with long lines (spanning multiple read blocks). */
    long_line = (char *)malloc (20000 + 1);
    memset (long_line, 'a', 20000);
    long_line[20000] = '\0';
    filename = string_eval_path_home ("${weechat_data_dir}/test_file.txt",
                                      NULL, NULL, NULL);
    file = fopen (filename, "w");
    fwrite (long_line, 1, strlen (long_line), file);
    fwrite ("\nline 2\n", 1, 8, file);
    fwrite (long_line, 1, strlen (long_line), file);
    fwrite ("\nline 4\n", 1, 8, file);
    fflush (file);
    fclose (file);

    lines = logger_tail_file (filename, 1);
    CHECK(lines);
    LONGS_EQUAL(1, arraylist_size (lines));
    STRCMP_EQUAL("line 4", (const char *)arraylist_get (lines, 0));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 2);
    CHECK(lines);
    LONGS_EQUAL(2, arraylist_size (lines));
    STRCMP_EQUAL(long_line, (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("line 4", (const char *)arraylist_get (lines, 1));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 3);
    CHECK(lines);
    LONGS_EQUAL(3, arraylist_size (lines));
    STRCMP_EQUAL("line 2", (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL(long_line, (const char *)arraylist_get (lines, 1));
    STRCMP_EQUAL("line 4", (const char *)arraylist_get (lines, 2));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 10);
    CHECK(lines);
    LONGS_EQUAL(4, arraylist_size (lines));
    STRCMP_EQUAL(long_line, (const char *)arraylist_get (lines, 0));
    STRCMP_EQUAL("line 2", (const char *)arraylist_get (lines, 1));
    STRCMP_EQUAL(long_line, (const char *)arraylist_get (lines, 2));
    STRCMP_EQUAL("line 4", (const char *)arraylist_get (lines, 3));
    arraylist_free (lines);

    unlink (filename);
    free (filename);

    /* Write a test file with a single long line, without final EOL. */
    filename = string_eval_path_home ("${weechat_data_dir}/test_file.txt",
                                      NULL, NULL, NULL);
    file = fopen (filename, "w");
    fwrite (long_line, 1, strlen (long_line), file);
    fflush (file);
    fclose (file);

    lines = logger_tail_file (filename, 1);
    CHECK(lines);
    LONGS_EQUAL(1, arraylist_size (lines));
    STRCMP_EQUAL(long_line, (const char *)arraylist_get (lines, 0));
    arraylist_free (lines);

    lines = logger_tail_file (filename, 2);
    CHECK(lines);
    LONGS_EQUAL(1, arraylist_size (lines));
    STRCMP_EQUAL(long_line, (const char *)arraylist_get (lines, 0));
    arraylist_free (lines);

    unlink (filename);
    free (filename);

    free (long_line);
}
