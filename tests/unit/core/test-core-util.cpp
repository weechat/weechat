/*
 * test-core-util.cpp - test util functions
 *
 * Copyright (C) 2014-2023 Sébastien Helleu <flashcode@flashtux.org>
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
 *   util_version_number
 */

TEST(CoreUtil, VersionNumber)
{
    LONGS_EQUAL(0x00030200, util_version_number ("0.3.2-dev"));
    LONGS_EQUAL(0x00030200, util_version_number ("0.3.2-rc1"));
    LONGS_EQUAL(0x00030200, util_version_number ("0.3.2"));
    LONGS_EQUAL(0x00030101, util_version_number ("0.3.1.1"));
    LONGS_EQUAL(0x00030100, util_version_number ("0.3.1"));
    LONGS_EQUAL(0x00030000, util_version_number ("0.3.0"));
    LONGS_EQUAL(0x01000000, util_version_number ("1.0"));
    LONGS_EQUAL(0x01000000, util_version_number ("1.0.0"));
    LONGS_EQUAL(0x01000000, util_version_number ("1.0.0.0"));
    LONGS_EQUAL(0x01000100, util_version_number ("1.0.1"));
    LONGS_EQUAL(0x01000200, util_version_number ("1.0.2"));
    LONGS_EQUAL(0x01010000, util_version_number ("1.1"));
    LONGS_EQUAL(0x01010100, util_version_number ("1.1.1"));
    LONGS_EQUAL(0x01010200, util_version_number ("1.1.2"));
    LONGS_EQUAL(0x01020304, util_version_number ("1.2.3.4"));
}
