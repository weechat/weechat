/*
 * test-core-util.cpp - test util functions
 *
 * Copyright (C) 2014-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#include "tests/tests.h"

extern "C"
{
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "src/core/core-string.h"
#include "src/core/core-util.h"
}

#define WEE_PARSE_DATE(__result, __sec, __usec, __datetime)    \
    tv.tv_sec = 0;                                             \
    tv.tv_usec = 0;                                            \
    LONGS_EQUAL(__result, util_parse_time (__datetime, &tv));  \
    LONGS_EQUAL(__sec, tv.tv_sec);                             \
    LONGS_EQUAL(__usec, tv.tv_usec);

#define WEE_PARSE_DELAY(__result, __result_delay,              \
                        __delay, __factor)                     \
    delay = ULLONG_MAX;                                        \
    LONGS_EQUAL(__result,                                      \
                util_parse_delay (__delay, __factor, &delay)); \
    CHECK(delay == __result_delay);

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
 *   util_get_microseconds_string
 */

TEST(CoreUtil, GetMicrosecondsString)
{
    char *str;

    /* zero */
    WEE_TEST_STR("0:00:00.000000",
                 util_get_microseconds_string (0ULL));

    /* microseconds */
    WEE_TEST_STR("0:00:00.000001", util_get_microseconds_string (1ULL));
    WEE_TEST_STR("0:00:00.000123", util_get_microseconds_string (123ULL));

    /* microseconds */
    WEE_TEST_STR("0:00:00.001000", util_get_microseconds_string (1ULL * 1000ULL));
    WEE_TEST_STR("0:00:00.123000", util_get_microseconds_string (123ULL * 1000ULL));

    /* seconds */
    WEE_TEST_STR("0:00:01.000000", util_get_microseconds_string (1ULL * 1000ULL * 1000ULL));
    WEE_TEST_STR("0:00:12.000000", util_get_microseconds_string (12ULL * 1000ULL * 1000ULL));

    /* minutes */
    WEE_TEST_STR("0:01:00.000000", util_get_microseconds_string (1ULL * 60ULL * 1000ULL * 1000ULL));
    WEE_TEST_STR("0:34:00.000000", util_get_microseconds_string (34ULL * 60ULL * 1000ULL * 1000ULL));

    /* hours */
    WEE_TEST_STR("1:00:00.000000", util_get_microseconds_string (1ULL * 60ULL * 60ULL * 1000ULL * 1000ULL));
    WEE_TEST_STR("34:00:00.000000", util_get_microseconds_string (34ULL * 60ULL * 60ULL * 1000ULL * 1000ULL));

    /* hours + minutes + seconds + milliseconds + microseconds */
    WEE_TEST_STR("3:25:45.678901", util_get_microseconds_string (12345678901ULL));
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
 *   util_strftimeval
 */

TEST(CoreUtil, Strftimeval)
{
    struct timeval tv;
    char str_time[256];

    /* test date: 2023-12-25T10:29:09.456789Z */
    tv.tv_sec = 1703500149;
    tv.tv_usec = 456789;

    LONGS_EQUAL(0, util_strftimeval (NULL, 0, NULL, NULL));
    LONGS_EQUAL(0, util_strftimeval (str_time, 0, NULL, NULL));
    LONGS_EQUAL(0, util_strftimeval (str_time, 0, "", NULL));
    LONGS_EQUAL(0, util_strftimeval (str_time, -1, "", &tv));

    strcpy (str_time, "test");
    LONGS_EQUAL(0, util_strftimeval (str_time, sizeof (str_time), "", &tv));
    STRCMP_EQUAL("", str_time);

    strcpy (str_time, "test");
    LONGS_EQUAL(8, util_strftimeval (str_time, sizeof (str_time),
                                     "%H:%M:%S", &tv));
    STRCMP_EQUAL("10:29:09", str_time);

    strcpy (str_time, "test");
    LONGS_EQUAL(19, util_strftimeval (str_time, sizeof (str_time),
                                      "%Y-%m-%d %H:%M:%S", &tv));
    STRCMP_EQUAL("2023-12-25 10:29:09", str_time);

    strcpy (str_time, "test");
    LONGS_EQUAL(19, util_strftimeval (str_time, sizeof (str_time),
                                      "%Y-%m-%d %H:%M:%S", &tv));
    STRCMP_EQUAL("2023-12-25 10:29:09", str_time);

    strcpy (str_time, "test");
    LONGS_EQUAL(21, util_strftimeval (str_time, sizeof (str_time),
                                      "%Y-%m-%d %% %H:%M:%S", &tv));
    STRCMP_EQUAL("2023-12-25 % 10:29:09", str_time);

    strcpy (str_time, "test");
    LONGS_EQUAL(21, util_strftimeval (str_time, sizeof (str_time),
                                      "%Y-%m-%d %H:%M:%S.%.1", &tv));
    STRCMP_EQUAL("2023-12-25 10:29:09.4", str_time);

    strcpy (str_time, "test");
    LONGS_EQUAL(22, util_strftimeval (str_time, sizeof (str_time),
                                      "%Y-%m-%d %H:%M:%S.%.2", &tv));
    STRCMP_EQUAL("2023-12-25 10:29:09.45", str_time);

    strcpy (str_time, "test");
    LONGS_EQUAL(23, util_strftimeval (str_time, sizeof (str_time),
                                      "%Y-%m-%d %H:%M:%S.%.3", &tv));
    STRCMP_EQUAL("2023-12-25 10:29:09.456", str_time);

    strcpy (str_time, "test");
    LONGS_EQUAL(24, util_strftimeval (str_time, sizeof (str_time),
                                      "%Y-%m-%d %H:%M:%S.%.4", &tv));
    STRCMP_EQUAL("2023-12-25 10:29:09.4567", str_time);

    strcpy (str_time, "test");
    LONGS_EQUAL(25, util_strftimeval (str_time, sizeof (str_time),
                                      "%Y-%m-%d %H:%M:%S.%.5", &tv));
    STRCMP_EQUAL("2023-12-25 10:29:09.45678", str_time);

    strcpy (str_time, "test");
    LONGS_EQUAL(26, util_strftimeval (str_time, sizeof (str_time),
                                      "%Y-%m-%d %H:%M:%S.%.6", &tv));
    STRCMP_EQUAL("2023-12-25 10:29:09.456789", str_time);

    strcpy (str_time, "test");
    LONGS_EQUAL(26, util_strftimeval (str_time, sizeof (str_time),
                                      "%Y-%m-%d %H:%M:%S.%f", &tv));
    STRCMP_EQUAL("2023-12-25 10:29:09.456789", str_time);

    /* invalid microseconds digits (must be 1-6) */
    strcpy (str_time, "test");
    LONGS_EQUAL(20, util_strftimeval (str_time, sizeof (str_time),
                                      "%Y-%m-%d %H:%M:%S.%.0", &tv));
    STRCMP_EQUAL("2023-12-25 10:29:09.", str_time);
    strcpy (str_time, "test");
    LONGS_EQUAL(20, util_strftimeval (str_time, sizeof (str_time),
                                      "%Y-%m-%d %H:%M:%S.%.7", &tv));
    STRCMP_EQUAL("2023-12-25 10:29:09.", str_time);

    /* timestamp */
    strcpy (str_time, "test");
    LONGS_EQUAL(10, util_strftimeval (str_time, sizeof (str_time), "%!", &tv));
    STRCMP_EQUAL("1703500149", str_time);
    strcpy (str_time, "test");
    LONGS_EQUAL(17, util_strftimeval (str_time, sizeof (str_time), "%!.%f", &tv));
    STRCMP_EQUAL("1703500149.456789", str_time);

    /* test date: 2023-12-25T10:29:09.000000Z (with microseconds < 0) */
    tv.tv_sec = 1703500149;
    tv.tv_usec = -1;

    strcpy (str_time, "test");
    LONGS_EQUAL(26, util_strftimeval (str_time, sizeof (str_time),
                                      "%Y-%m-%d %H:%M:%S.%f", &tv));
    STRCMP_EQUAL("2023-12-25 10:29:09.000000", str_time);

    /* test date: 2023-12-25T10:29:09.999999Z (with microseconds > 999999) */
    tv.tv_sec = 1703500149;
    tv.tv_usec = 1000000;

    strcpy (str_time, "test");
    LONGS_EQUAL(26, util_strftimeval (str_time, sizeof (str_time),
                                      "%Y-%m-%d %H:%M:%S.%f", &tv));
    STRCMP_EQUAL("2023-12-25 10:29:09.999999", str_time);
}

/*
 * Tests functions:
 *   util_parse_time
 */

TEST(CoreUtil, ParseTime)
{
    struct timeval tv;
    struct tm *local_time;
    time_t date;
    char str_time[128];

    /* test with local timezone: UTC+1 */
    setenv ("TZ", "UTC+1", 1);

    LONGS_EQUAL(0, util_parse_time (NULL, NULL));
    LONGS_EQUAL(0, util_parse_time (NULL, &tv));

    WEE_PARSE_DATE(0, 0, 0, "");
    WEE_PARSE_DATE(0, 0, 0, "invalid");

    /*
     * expected: 2023-12-25T00:00:00Z == 1703462400
     * (local timezone UTC+1: 1703466000)
     */
    WEE_PARSE_DATE(1, 1703466000, 0, "2023-12-25");

    /* expected: current date with specified local time */
    date = time (NULL);
    local_time = localtime (&date);
    strftime (str_time, sizeof (str_time), "%H:%M:%S", local_time);
    LONGS_EQUAL(1, util_parse_time (str_time, &tv));
    CHECK((tv.tv_sec >= date) && (tv.tv_sec <= date + 10));
    LONGS_EQUAL(0, tv.tv_usec);

    date = time (NULL);
    local_time = localtime (&date);
    strftime (str_time, sizeof (str_time), "%H:%M:%S.456789", local_time);
    LONGS_EQUAL(1, util_parse_time (str_time, &tv));
    CHECK((tv.tv_sec >= date) && (tv.tv_sec <= date + 10));
    LONGS_EQUAL(456789, tv.tv_usec);

    /* expected: current date with specified UTC time */
    date = time (NULL);
    local_time = localtime (&date);
    strftime (str_time, sizeof (str_time), "%H:%M:%SZ", local_time);
    LONGS_EQUAL(1, util_parse_time (str_time, &tv));
    CHECK((tv.tv_sec >= date - 3600) && (tv.tv_sec <= date - 3600 + 10));
    LONGS_EQUAL(0, tv.tv_usec);

    date = time (NULL);
    local_time = localtime (&date);
    strftime (str_time, sizeof (str_time), "%H:%M:%S.456789Z", local_time);
    LONGS_EQUAL(1, util_parse_time (str_time, &tv));
    CHECK((tv.tv_sec >= date - 3600) && (tv.tv_sec <= date - 3600 + 10));
    LONGS_EQUAL(456789, tv.tv_usec);

    /*
     * expected: 2023-12-25T10:29:09.456789Z == 1703500149.456789
     * (local timezone UTC+1: 1703503749.456789)
     */
    WEE_PARSE_DATE(1, 1703500149 + 3600, 0, "2023-12-25T10:29:09");
    WEE_PARSE_DATE(1, 1703500149, 0, "2023-12-25T10:29:09Z");
    WEE_PARSE_DATE(1, 1703500149, 456000, "2023-12-25T10:29:09.456Z");
    WEE_PARSE_DATE(1, 1703500149, 456789, "2023-12-25T10:29:09.456789Z");

    /*
     * expected: 2023-12-25T10:29:09.456789Z == 1703500149.456789
     * with timezone offset
     */
    WEE_PARSE_DATE(1, 1703500149, 0, "2023-12-25T10:29:09+00");
    WEE_PARSE_DATE(1, 1703500149, 0, "2023-12-25T10:29:09+0000");
    WEE_PARSE_DATE(1, 1703500149, 0, "2023-12-25T10:29:09+00:00");
    WEE_PARSE_DATE(1, 1703500149, 0, "2023-12-25T10:29:09-00");
    WEE_PARSE_DATE(1, 1703500149, 0, "2023-12-25T10:29:09-0000");
    WEE_PARSE_DATE(1, 1703500149, 0, "2023-12-25T10:29:09-00:00");
    WEE_PARSE_DATE(1, 1703500149, 456789, "2023-12-25T10:29:09.456789-00:00");

    WEE_PARSE_DATE(1, 1703500149 + 7200, 0, "2023-12-25T10:29:09+02");
    WEE_PARSE_DATE(1, 1703500149 + 60, 0, "2023-12-25T10:29:09+0001");
    WEE_PARSE_DATE(1, 1703500149 + 60, 456789, "2023-12-25T10:29:09.456789+0001");

    WEE_PARSE_DATE(1, 1703500149 + 7200, 0, "2023-12-25T10:29:09+0200");
    WEE_PARSE_DATE(1, 1703500149 + 7200, 0, "2023-12-25T10:29:09+02:00");
    WEE_PARSE_DATE(1, 1703500149 + 3600 + 1800, 0, "2023-12-25T10:29:09+0130");
    WEE_PARSE_DATE(1, 1703500149 + 3600 + 1800, 0, "2023-12-25T10:29:09+01:30");
    WEE_PARSE_DATE(1, 1703500149 + 3600 + 1800, 456789, "2023-12-25T10:29:09.456789+01:30");

    WEE_PARSE_DATE(1, 1703500149 - 60, 0, "2023-12-25T10:29:09-0001");
    WEE_PARSE_DATE(1, 1703500149 - 3600 - 1800, 0, "2023-12-25T10:29:09-0130");
    WEE_PARSE_DATE(1, 1703500149 - 3600 - 1800, 0, "2023-12-25T10:29:09-01:30");
    WEE_PARSE_DATE(1, 1703500149 - 3600 - 1800, 456789, "2023-12-25T10:29:09.456789-01:30");

    /* expected: 2023-12-25T10:29:09.456789Z == 1703500149.456789 */
    WEE_PARSE_DATE(1, 1703500149, 0, "1703500149");
    WEE_PARSE_DATE(1, 1703500149, 456000, "1703500149.456");
    WEE_PARSE_DATE(1, 1703500149, 456789, "1703500149.456789");
    WEE_PARSE_DATE(1, 1703500149, 456000, "1703500149,456");
    WEE_PARSE_DATE(1, 1703500149, 456789, "1703500149,456789");

    setenv ("TZ", "", 1);
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
    unsigned long long delay;

    /* error: no delay */
    LONGS_EQUAL(0, util_parse_delay ("123", 1ULL, NULL));

    /* error: no string */
    WEE_PARSE_DELAY(0, 0ULL, NULL, 0ULL);
    WEE_PARSE_DELAY(0, 0ULL, NULL, 1ULL);
    WEE_PARSE_DELAY(0, 0ULL, "", 0ULL);
    WEE_PARSE_DELAY(0, 0ULL, "", 1ULL);

    /* error: bad default_factor */
    WEE_PARSE_DELAY(0, 0ULL, "abcd", 0ULL);
    WEE_PARSE_DELAY(0, 0ULL, "123", 0ULL);

    /* error: bad unit */
    WEE_PARSE_DELAY(0, 0ULL, "123a", 1ULL);
    WEE_PARSE_DELAY(0, 0ULL, "123ss", 1ULL);
    WEE_PARSE_DELAY(0, 0ULL, "123mss", 1ULL);
    WEE_PARSE_DELAY(0, 0ULL, "123uss", 1ULL);

    /* error: bad number */
    WEE_PARSE_DELAY(0, 0ULL, "abcd", 1LL);

    /* error: bad delay */
    WEE_PARSE_DELAY(0, 0ULL, "-123", 1LL);

    /* tests with delay == 0 */
    WEE_PARSE_DELAY(1, 0ULL, "0", 1ULL);
    WEE_PARSE_DELAY(1, 0ULL, "0us", 1ULL);
    WEE_PARSE_DELAY(1, 0ULL, "0ms", 1ULL);
    WEE_PARSE_DELAY(1, 0ULL, "0s", 1ULL);
    WEE_PARSE_DELAY(1, 0ULL, "0m", 1ULL);
    WEE_PARSE_DELAY(1, 0ULL, "0h", 1ULL);

    /* tests with delay == 123, default_factor = 1 (1 microsecond) */
    WEE_PARSE_DELAY(1, 123ULL, "123", 1ULL);
    WEE_PARSE_DELAY(1, 123ULL, "123us", 1ULL);
    WEE_PARSE_DELAY(1, 123ULL * 1000ULL, "123ms", 1ULL);
    WEE_PARSE_DELAY(1, 123ULL * 1000ULL * 1000ULL, "123s", 1ULL);
    WEE_PARSE_DELAY(1, 123ULL * 1000ULL * 1000ULL * 60ULL, "123m", 1ULL);
    WEE_PARSE_DELAY(1, 123ULL * 1000ULL * 1000ULL * 60ULL * 60ULL, "123h", 1ULL);

    /* tests with delay == 123, default_factor = 1000 (1 millisecond) */
    WEE_PARSE_DELAY(1, 123ULL * 1000ULL, "123", 1000ULL);
    WEE_PARSE_DELAY(1, 123ULL, "123us", 1000ULL);
    WEE_PARSE_DELAY(1, 123ULL * 1000ULL, "123ms", 1000ULL);
    WEE_PARSE_DELAY(1, 123ULL * 1000ULL * 1000ULL, "123s", 1000ULL);
    WEE_PARSE_DELAY(1, 123ULL * 1000ULL * 1000ULL * 60ULL, "123m", 1000ULL);
    WEE_PARSE_DELAY(1, 123ULL * 1000ULL * 1000ULL * 60ULL * 60ULL, "123h", 1000ULL);

    /* tests with delay == 123, default_factor = 1000000 (1 second) */
    WEE_PARSE_DELAY(1, 123ULL * 1000ULL * 1000ULL, "123", 1000000ULL);
    WEE_PARSE_DELAY(1, 123ULL, "123us", 1000000ULL);
    WEE_PARSE_DELAY(1, 123ULL * 1000ULL, "123ms", 1000000ULL);
    WEE_PARSE_DELAY(1, 123ULL * 1000ULL * 1000ULL, "123s", 1000000ULL);
    WEE_PARSE_DELAY(1, 123ULL * 1000ULL * 1000ULL * 60ULL, "123m", 1000000ULL);
    WEE_PARSE_DELAY(1, 123ULL * 1000ULL * 1000ULL * 60ULL * 60ULL, "123h", 1000000ULL);
}

/*
 * Tests functions:
 *   util_version_number
 */

TEST(CoreUtil, VersionNumber)
{
    LONGS_EQUAL(0, util_version_number (NULL));
    LONGS_EQUAL(0, util_version_number (""));
    LONGS_EQUAL(0, util_version_number ("abc"));

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
