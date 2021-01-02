/*
 * test-core-util.cpp - test util functions
 *
 * Copyright (C) 2014-2021 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include "src/core/wee-string.h"
#include "src/core/wee-util.h"
}

TEST_GROUP(CoreUtil)
{
};

/*
 * Tests functions:
 *   util_timeval_cmp
 *   util_timeval_diff
 *   util_timeval_add
 */

TEST(CoreUtil, Timeval)
{
    struct timeval tv_zero = { 0, 0 };
    struct timeval tv1 = { 123456, 12000 };
    struct timeval tv2 = { 123456, 15000 };
    struct timeval tv3 = { 123457, 15000 };
    struct timeval tv4 = { 1409288400, 0 };  /* 2014-08-29 05:00:00 GMT */
    struct timeval tv;

    /* comparison */
    LONGS_EQUAL(0, util_timeval_cmp (NULL, NULL));
    LONGS_EQUAL(-1, util_timeval_cmp (NULL, &tv1));
    LONGS_EQUAL(1, util_timeval_cmp (&tv1, NULL));
    LONGS_EQUAL(0, util_timeval_cmp (&tv1, &tv1));
    LONGS_EQUAL(-1, util_timeval_cmp (&tv1, &tv2));
    LONGS_EQUAL(1, util_timeval_cmp (&tv2, &tv1));
    LONGS_EQUAL(-1, util_timeval_cmp (&tv1, &tv3));
    LONGS_EQUAL(1, util_timeval_cmp (&tv3, &tv1));

    /* difference */
    LONGS_EQUAL(0, util_timeval_diff (NULL, NULL));
    LONGS_EQUAL(0, util_timeval_diff (NULL, &tv1));
    LONGS_EQUAL(0, util_timeval_diff (&tv1, NULL));
    LONGS_EQUAL(3000, util_timeval_diff (&tv1, &tv2));
    LONGS_EQUAL(-3000, util_timeval_diff (&tv2, &tv1));
    LONGS_EQUAL(1003000, util_timeval_diff (&tv1, &tv3));
    LONGS_EQUAL(-1003000, util_timeval_diff (&tv3, &tv1));
    CHECK(1409288400 * 1000000LL == util_timeval_diff (&tv_zero, &tv4));

    /* add interval */
    util_timeval_add (NULL, 0);
    tv.tv_sec = 123456;
    tv.tv_usec = 12000;
    util_timeval_add (&tv, 10000);
    LONGS_EQUAL(123456, tv.tv_sec);
    LONGS_EQUAL(22000, tv.tv_usec);
    util_timeval_add (&tv, 4000000);
    LONGS_EQUAL(123460, tv.tv_sec);
    LONGS_EQUAL(22000, tv.tv_usec);
    util_timeval_add (&tv, 999000);
    LONGS_EQUAL(123461, tv.tv_sec);
    LONGS_EQUAL(21000, tv.tv_usec);
}

/*
 * Tests functions:
 *   util_get_time_string
 */

TEST(CoreUtil, GetTimeString)
{
    time_t date;
    const char *str_date;

    date = 946684800;  /* 2000-01-01 00:00 */
    str_date = util_get_time_string (&date);
    STRCMP_EQUAL("Sat, 01 Jan 2000 00:00:00", str_date);
}

/*
 * Tests functions:
 *   util_get_time_diff
 */

TEST(CoreUtil, GetTimeDiff)
{
    time_t date1, date2, total_seconds;
    int days, hours, minutes, seconds;

    util_get_time_diff (0, 0, NULL, NULL, NULL, NULL, NULL);

    date1 = 946684800;  /* 2000-01-01 00:00:00 */

    date2 = 946684800;  /* 2000-01-01 00:00:00 */
    util_get_time_diff (date1, date2,
                        &total_seconds, &days, &hours, &minutes, &seconds);
    LONGS_EQUAL(0, total_seconds);
    LONGS_EQUAL(0, days);
    LONGS_EQUAL(0, hours);
    LONGS_EQUAL(0, minutes);
    LONGS_EQUAL(0, seconds);

    date2 = 946684801;  /* 2000-01-01 00:00:01 */
    util_get_time_diff (date1, date2,
                        &total_seconds, &days, &hours, &minutes, &seconds);
    LONGS_EQUAL(1, total_seconds);
    LONGS_EQUAL(0, days);
    LONGS_EQUAL(0, hours);
    LONGS_EQUAL(0, minutes);
    LONGS_EQUAL(1, seconds);

    date2 = 946684880;  /* 2000-01-01 00:01:20 */
    util_get_time_diff (date1, date2,
                        &total_seconds, &days, &hours, &minutes, &seconds);
    LONGS_EQUAL(80, total_seconds);
    LONGS_EQUAL(0, days);
    LONGS_EQUAL(0, hours);
    LONGS_EQUAL(1, minutes);
    LONGS_EQUAL(20, seconds);

    date2 = 946695680;  /* 2000-01-01 03:01:20 */
    util_get_time_diff (date1, date2,
                        &total_seconds, &days, &hours, &minutes, &seconds);
    LONGS_EQUAL(10880, total_seconds);
    LONGS_EQUAL(0, days);
    LONGS_EQUAL(3, hours);
    LONGS_EQUAL(1, minutes);
    LONGS_EQUAL(20, seconds);

    date2 = 947127680;  /* 2000-01-06 03:01:20 */
    util_get_time_diff (date1, date2,
                        &total_seconds, &days, &hours, &minutes, &seconds);
    LONGS_EQUAL(442880, total_seconds);
    LONGS_EQUAL(5, days);
    LONGS_EQUAL(3, hours);
    LONGS_EQUAL(1, minutes);
    LONGS_EQUAL(20, seconds);

    date2 = 979527680;  /* 2001-01-15 03:01:20 */
    util_get_time_diff (date1, date2,
                        &total_seconds, &days, &hours, &minutes, &seconds);
    LONGS_EQUAL(32842880, total_seconds);
    LONGS_EQUAL(380, days);
    LONGS_EQUAL(3, hours);
    LONGS_EQUAL(1, minutes);
    LONGS_EQUAL(20, seconds);
}

/*
 * Tests functions:
 *   util_parse_delay
 */

TEST(CoreUtil, ParseDelay)
{
    /* error: no string */
    LONGS_EQUAL(-1, util_parse_delay (NULL, -1));
    LONGS_EQUAL(-1, util_parse_delay (NULL, 0));
    LONGS_EQUAL(-1, util_parse_delay (NULL, 1));
    LONGS_EQUAL(-1, util_parse_delay ("", -1));
    LONGS_EQUAL(-1, util_parse_delay ("", 0));
    LONGS_EQUAL(-1, util_parse_delay ("", 1));

    /* error: bad default_factor */
    LONGS_EQUAL(-1, util_parse_delay ("abcd", -1));
    LONGS_EQUAL(-1, util_parse_delay ("abcd", 0));
    LONGS_EQUAL(-1, util_parse_delay ("123", -1));
    LONGS_EQUAL(-1, util_parse_delay ("123", 0));

    /* error: bad unit */
    LONGS_EQUAL(-1, util_parse_delay ("123a", 1));
    LONGS_EQUAL(-1, util_parse_delay ("123ss", 1));
    LONGS_EQUAL(-1, util_parse_delay ("123mss", 1));

    /* error: bad number */
    LONGS_EQUAL(-1, util_parse_delay ("abcd", 1));

    /* tests with delay == 0 */
    LONGS_EQUAL(0, util_parse_delay ("0", 1));
    LONGS_EQUAL(0, util_parse_delay ("0ms", 1));
    LONGS_EQUAL(0, util_parse_delay ("0s", 1));
    LONGS_EQUAL(0, util_parse_delay ("0m", 1));
    LONGS_EQUAL(0, util_parse_delay ("0h", 1));

    /* tests with delay == 123, default_factor = 1 */
    LONGS_EQUAL(123, util_parse_delay ("123", 1));
    LONGS_EQUAL(123, util_parse_delay ("123", 1));
    LONGS_EQUAL(123, util_parse_delay ("123ms", 1));
    LONGS_EQUAL(123 * 1000, util_parse_delay ("123s", 1));
    LONGS_EQUAL(123 * 1000 * 60, util_parse_delay ("123m", 1));
    LONGS_EQUAL(123 * 1000 * 60 * 60, util_parse_delay ("123h", 1));

    /* tests with delay == 123, default_factor = 1000 */
    LONGS_EQUAL(123 * 1000, util_parse_delay ("123", 1000));
    LONGS_EQUAL(123, util_parse_delay ("123ms", 1000));
    LONGS_EQUAL(123 * 1000, util_parse_delay ("123s", 1000));
    LONGS_EQUAL(123 * 1000 * 60, util_parse_delay ("123m", 1000));
    LONGS_EQUAL(123 * 1000 * 60 * 60, util_parse_delay ("123h", 1000));
}

/*
 * Tests functions:
 *   util_signal_search
 */

TEST(CoreUtil, SignalSearch)
{
    int count;

    /* make tests fail if the util_signals structure is changed */
    for (count = 0; util_signals[count].name; count++)
    {
    }
    LONGS_EQUAL(7, count);

    LONGS_EQUAL(-1, util_signal_search (NULL));
    LONGS_EQUAL(-1, util_signal_search (""));
    LONGS_EQUAL(-1, util_signal_search ("signal_does_not_exist"));

    LONGS_EQUAL(SIGHUP, util_signal_search ("hup"));
    LONGS_EQUAL(SIGINT, util_signal_search ("int"));
    LONGS_EQUAL(SIGQUIT, util_signal_search ("quit"));
    LONGS_EQUAL(SIGKILL, util_signal_search ("kill"));
    LONGS_EQUAL(SIGTERM, util_signal_search ("term"));
    LONGS_EQUAL(SIGUSR1, util_signal_search ("usr1"));
    LONGS_EQUAL(SIGUSR2, util_signal_search ("usr2"));

    LONGS_EQUAL(SIGHUP, util_signal_search ("HUP"));
    LONGS_EQUAL(SIGINT, util_signal_search ("INT"));
    LONGS_EQUAL(SIGQUIT, util_signal_search ("QUIT"));
    LONGS_EQUAL(SIGKILL, util_signal_search ("KILL"));
    LONGS_EQUAL(SIGTERM, util_signal_search ("TERM"));
    LONGS_EQUAL(SIGUSR1, util_signal_search ("USR1"));
    LONGS_EQUAL(SIGUSR2, util_signal_search ("USR2"));
}

/*
 * Tests functions:
 *   util_signal_search_number
 */

TEST(CoreUtil, SignalSearchNumber)
{
    POINTERS_EQUAL(NULL, util_signal_search_number (-1));
    POINTERS_EQUAL(NULL, util_signal_search_number (999999999));

    STRCMP_EQUAL("hup", util_signal_search_number (SIGHUP));
    STRCMP_EQUAL("int", util_signal_search_number (SIGINT));
    STRCMP_EQUAL("quit", util_signal_search_number (SIGQUIT));
    STRCMP_EQUAL("kill", util_signal_search_number (SIGKILL));
    STRCMP_EQUAL("term", util_signal_search_number (SIGTERM));
    STRCMP_EQUAL("usr1", util_signal_search_number (SIGUSR1));
    STRCMP_EQUAL("usr2", util_signal_search_number (SIGUSR2));
}

/*
 * Tests functions:
 *   util_catch_signal
 */

TEST(CoreUtil, CatchSignal)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   util_mkdir_home
 *   util_mkdir
 *   util_mkdir_parents
 */

TEST(CoreUtil, Mkdir)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   util_exec_on_files
 */

TEST(CoreUtil, ExecOnFiles)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   util_search_full_lib_name
 */

TEST(CoreUtil, LibName)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   util_file_get_content
 */

TEST(CoreUtil, FileGetContent)
{
    char *path, *content, *content_read;
    const char *content_small = "line 1\nline 2\nend";
    int length, i;
    FILE *f;

    /* file not found */
    POINTERS_EQUAL(NULL, util_file_get_content (NULL));
    POINTERS_EQUAL(NULL, util_file_get_content (""));
    POINTERS_EQUAL(NULL, util_file_get_content ("/tmp/does/not/exist.xyz"));

    path = string_eval_path_home ("%h/test_file.txt", NULL, NULL, NULL);

    /* small file */
    length = strlen (content_small);
    f = fopen (path, "wb");
    CHECK(f);
    LONGS_EQUAL(length, fwrite (content_small, 1, length, f));
    fclose (f);
    content_read = util_file_get_content (path);
    STRCMP_EQUAL(content_small, content_read);
    free (content_read);
    unlink (path);

    /* bigger file: 26 lines of 5000 bytes */
    length = 26 * 5001;
    content = (char *)malloc (length + 1);
    CHECK(content);
    for (i = 0; i < 26; i++)
    {
        memset (content + (i * 5001), 'a' + i, 5000);
        content[(i * 5001) + 5000] = '\n';
    }
    content[26 * 5001] = '\0';
    f = fopen (path, "wb");
    CHECK(f);
    LONGS_EQUAL(length, fwrite (content, 1, length, f));
    fclose (f);
    content_read = util_file_get_content (path);
    STRCMP_EQUAL(content, content_read);
    free (content_read);
    unlink (path);
    free (content);
}

/*
 * Tests functions:
 *   util_version_number
 */

TEST(CoreUtil, VersionNumber)
{
    BYTES_EQUAL(0x00030200, util_version_number ("0.3.2-dev"));
    BYTES_EQUAL(0x00030200, util_version_number ("0.3.2-rc1"));
    BYTES_EQUAL(0x00030200, util_version_number ("0.3.2"));
    BYTES_EQUAL(0x00030101, util_version_number ("0.3.1.1"));
    BYTES_EQUAL(0x00030100, util_version_number ("0.3.1"));
    BYTES_EQUAL(0x00030000, util_version_number ("0.3.0"));
    BYTES_EQUAL(0x01000000, util_version_number ("1.0"));
    BYTES_EQUAL(0x01000000, util_version_number ("1.0.0"));
    BYTES_EQUAL(0x01000000, util_version_number ("1.0.0.0"));
    BYTES_EQUAL(0x01000100, util_version_number ("1.0.1"));
    BYTES_EQUAL(0x01000200, util_version_number ("1.0.2"));
    BYTES_EQUAL(0x01010000, util_version_number ("1.1"));
    BYTES_EQUAL(0x01010100, util_version_number ("1.1.1"));
    BYTES_EQUAL(0x01010200, util_version_number ("1.1.2"));
    BYTES_EQUAL(0x01020304, util_version_number ("1.2.3.4"));
}
