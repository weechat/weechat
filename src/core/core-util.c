/*
 * core-util.c - some useful functions
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#include <ctype.h>
#include <time.h>

#include "weechat.h"
#include "core-util.h"
#include "core-config.h"
#include "core-log.h"
#include "core-string.h"
#include "core-utf8.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"


/*
 * Compares two timeval structures.
 *
 * Returns:
 *   -1: tv1 < tv2
 *    0: tv1 == tv2
 *    1: tv1 > tv2
 */

int
util_timeval_cmp (struct timeval *tv1, struct timeval *tv2)
{
    if (!tv1 || !tv2)
        return (tv1) ? 1 : ((tv2) ? -1 : 0);

    if (tv1->tv_sec < tv2->tv_sec)
        return -1;
    if (tv1->tv_sec > tv2->tv_sec)
        return 1;
    if (tv1->tv_usec < tv2->tv_usec)
        return -1;
    if (tv1->tv_usec > tv2->tv_usec)
        return 1;
    return 0;
}

/*
 * Calculates difference between two timeval structures.
 *
 * Returns difference in microseconds.
 */

long long
util_timeval_diff (struct timeval *tv1, struct timeval *tv2)
{
    long long diff_sec, diff_usec;

    if (!tv1 || !tv2)
        return 0;

    diff_sec = tv2->tv_sec - tv1->tv_sec;
    diff_usec = tv2->tv_usec - tv1->tv_usec;

    return (diff_sec * 1000000) + diff_usec;
}

/*
 * Adds interval (in microseconds) to a timeval structure.
 */

void
util_timeval_add (struct timeval *tv, long long interval)
{
    long long usec;

    if (!tv)
        return;

    tv->tv_sec += (interval / 1000000);
    usec = tv->tv_usec + (interval % 1000000);
    if (usec > 1000000)
    {
        tv->tv_usec = usec % 1000000;
        tv->tv_sec++;
    }
    else
        tv->tv_usec = usec;
}

/*
 * Converts microseconds to a string, using format: "H:MM:SS.mmmmmm"
 * where: H=hours, MM=minutes, SS=seconds, mmmmmm=microseconds
 *
 * Note: result must be freed after use.
 */

char *
util_get_microseconds_string (unsigned long long microseconds)
{
    unsigned long long hour, min, sec, usec;
    char result[128];

    usec = microseconds % 1000000;
    sec = (microseconds / 1000000) % 60;
    min = ((microseconds / 1000000) / 60) % 60;
    hour = (microseconds / 1000000) / 3600;

    snprintf (result, sizeof (result),
              "%llu:%02llu:%02llu.%06llu",
              hour, min, sec, usec);

    return strdup (result);
}

/*
 * Converts date to a string, using format of option "weechat.look.time_format"
 * (can be localized).
 */

const char *
util_get_time_string (const time_t *date)
{
    struct tm *local_time;
    static char text_time[128];

    text_time[0] = '\0';
    local_time = localtime (date);
    if (local_time)
    {
        if (strftime (text_time, sizeof (text_time),
                      CONFIG_STRING(config_look_time_format), local_time) == 0)
            text_time[0] = '\0';
    }

    return text_time;
}

/*
 * Formats date and time like strftime (but with timeval structure as input)
 * and adds extra specifiers:
 *   - "%.1" to "%.6": first N digits of microseconds, zero-padded
 *   - "%f": alias of "%.6" (microseconds, zero-padded to 6 digits)
 *   - "%!": timestamp as integer, in seconds (value of tv->tv_sec)
 */

int
util_strftimeval (char *string, int max, const char *format, struct timeval *tv)
{
    char **format2, str_temp[32];
    const char *ptr_format;
    struct tm *local_time;
    int length, bytes;
    long usec;

    if (!string || (max <= 0) || !format || !tv)
        return 0;

    string[0] = '\0';

    if (!format[0])
        return 0;

    format2 = string_dyn_alloc (strlen (format) + 1);
    if (!format2)
        return 0;

    usec = (long)(tv->tv_usec);
    if (usec < 0)
        usec = 0;
    else if (usec > 999999)
        usec = 999999;

    ptr_format = format;
    while (ptr_format && ptr_format[0])
    {
        if ((ptr_format[0] == '%') && (ptr_format[1] == '%'))
        {
            string_dyn_concat (format2, "%%", -1);
            ptr_format += 2;
        }
        else if ((ptr_format[0] == '%') && (ptr_format[1] == '.'))
        {
            if ((ptr_format[2] >= '1') && (ptr_format[2] <= '6'))
            {
                snprintf (str_temp, sizeof (str_temp), "%06ld", usec);
                length = ptr_format[2] - '1' + 1;
                str_temp[length] = '\0';
                string_dyn_concat (format2, str_temp, -1);
                ptr_format += 3;
            }
            else
            {
                ptr_format += 2;
                if (ptr_format[0])
                    ptr_format++;
            }
        }
        else if ((ptr_format[0] == '%') && (ptr_format[1] == 'f'))
        {
            snprintf (str_temp, sizeof (str_temp), "%06ld", usec);
            string_dyn_concat (format2, str_temp, -1);
            ptr_format += 2;
        }
        else if ((ptr_format[0] == '%') && (ptr_format[1] == '!'))
        {
            snprintf (str_temp, sizeof (str_temp), "%lld", (long long)(tv->tv_sec));
            string_dyn_concat (format2, str_temp, -1);
            ptr_format += 2;
        }
        else
        {
            string_dyn_concat (format2, ptr_format, 1);
            ptr_format++;
        }
    }

    local_time = localtime (&(tv->tv_sec));
    if (!local_time)
    {
        string_dyn_free (format2, 1);
        return 0;
    }

    bytes = strftime (string, max, *format2, local_time);

    string_dyn_free (format2, 1);

    return bytes;
}

/*
 * Parses a date/time string, which can be one of these formats:
 *   "2024-01-04"                  -> date at midnight
 *   "2024-01-04T22:01:02"         -> ISO 8601, local time
 *   "2024-01-04T22:01:02.123"     -> ISO 8601, local time, with milliseconds
 *   "2024-01-04T22:01:02.123456"  -> ISO 8601, local time, with microseconds
 *   "2024-01-04T21:01:02Z"        -> ISO 8601, UTC
 *   "2024-01-04T21:01:02.123Z"    -> ISO 8601, UTC, with milliseconds
 *   "2024-01-04T21:01:02.123456Z" -> ISO 8601, UTC, with microseconds
 *   "22:01:02"                    -> current date, local time
 *   "22:01:02.123"                -> current date, local time with milliseconds
 *   "22.01:02.123456"             -> current date, local time with microseconds
 *   "21:01:02Z"                   -> current date, UTC
 *   "21:01:02.123Z"               -> current date, UTC, with milliseconds
 *   "21.01:02.123456Z"            -> current date, UTC, with microseconds
 *   "1704402062"                  -> timestamp date
 *   "1704402062.123"              -> timestamp date, with milliseconds
 *   "1704402062,123"              -> timestamp date, with milliseconds
 *   "1704402062.123456"           -> timestamp date, with microseconds
 *   "1704402062,123456"           -> timestamp date, with microseconds
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
util_parse_time (const char *datetime, struct timeval *tv)
{
    char *string, *pos, *pos2, str_usec[16], *error, str_date[128];
    struct tm tm_date, tm_date_gm, tm_date_local, *local_time;
    time_t time_now, time_gm, time_local;
    long long value;
    int rc, length, use_local_time, timezone_offset, offset_factor, hour, min;

    if (!datetime || !datetime[0] || !tv)
        return 0;

    rc = 0;

    tv->tv_sec = 0;
    tv->tv_usec = 0;

    use_local_time = 1;
    timezone_offset = 0;
    offset_factor = 1;

    string = strdup (datetime);
    if (!string)
        return 0;

    /* extract microseconds and remove them from string2 */
    pos = strchr (string, '.');
    if (!pos)
        pos = strchr (string, ',');
    if (pos)
    {
        pos2 = pos + 1;
        while (isdigit ((unsigned char)pos2[0]))
        {
            pos2++;
        }
        length = pos2 - pos - 1;
        if (length > 0)
        {
            if (length > 6)
                length = 6;
            memcpy (str_usec, pos + 1, length);
            str_usec[length] = '\0';
            while (strlen (str_usec) < 6)
            {
                strcat (str_usec, "0");
            }
            error = NULL;
            value = strtoll (str_usec, &error, 10);
            if (error && !error[0])
            {
                if (value < 0)
                    value = 0;
                else if (value > 999999)
                    value = 999999;
                tv->tv_usec = (long)value;
            }
        }
        memmove (pos, pos2, strlen (pos2) + 1);
    }

    /* extract timezone and remove it from string2 */
    pos = strchr (string, 'Z');
    if (pos)
    {
        pos[0] = '\0';
        use_local_time = 0;
        timezone_offset = 0;
    }
    else
    {
        pos = strchr (string, 'T');
        if (pos)
        {
            pos2 = strchr (pos, '+');
            if (pos2)
            {
                pos2[0] = '\0';
                pos2++;
                offset_factor = 1;
            }
            else
            {
                pos2 = strchr (pos, '-');
                if (pos2)
                {
                    pos2[0] = '\0';
                    pos2++;
                    offset_factor = -1;
                }
            }
            if (pos2)
            {
                use_local_time = 0;
                hour = 0;
                min = 0;
                if (isdigit ((unsigned char)pos2[0])
                    && isdigit ((unsigned char)pos2[1]))
                {
                    hour = ((pos2[0] - '0') * 10) + (pos2[1] - '0');
                    pos2 += 2;
                    if (pos2[0] == ':')
                        pos2++;
                    if (isdigit ((unsigned char)pos2[0])
                        && isdigit ((unsigned char)pos2[1]))
                    {
                        min = ((pos2[0] - '0') * 10) + (pos2[1] - '0');
                    }
                }
                timezone_offset = offset_factor * ((hour * 3600) + (min * 60));
            }
        }
    }

    if (strchr (string, '-'))
    {
        if (strchr (string, ':'))
        {
            /* ISO 8601 format like: "2024-01-04T21:01:02.123Z" */
            /* initialize structure, because strptime does not do it */
            memset (&tm_date, 0, sizeof (struct tm));
            pos = strptime (string, "%Y-%m-%dT%H:%M:%S", &tm_date);
            if (pos && (tm_date.tm_year > 0))
            {
                if (use_local_time)
                {
                    tv->tv_sec = mktime (&tm_date);
                }
                else
                {
                    /* convert to UTC and add timezone_offset */
                    time_now = mktime (&tm_date);
                    gmtime_r (&time_now, &tm_date_gm);
                    localtime_r (&time_now, &tm_date_local);
                    time_gm = mktime (&tm_date_gm);
                    time_local = mktime (&tm_date_local);
                    tv->tv_sec = mktime (&tm_date_local)
                        + (time_local - time_gm)
                        + timezone_offset;
                }
                rc = 1;
            }
        }
        else
        {
            /* ISO 8601 format like: "2024-01-04" */
            /* initialize structure, because strptime does not do it */
            memset (&tm_date, 0, sizeof (struct tm));
            pos = strptime (string, "%Y-%m-%d", &tm_date);
            if (pos && (tm_date.tm_year > 0))
            {
                tv->tv_sec = mktime (&tm_date);
                rc = 1;
            }
        }
    }
    else if (strchr (string, ':'))
    {
        /* hour format like: "21:01:02" */
        time_now = time (NULL);
        local_time = localtime (&time_now);
        strftime (str_date, sizeof (str_date),
                  "%Y-%m-%dT", local_time);
        strcat (str_date, string);
        /* initialize structure, because strptime does not do it */
        memset (&tm_date, 0, sizeof (struct tm));
        pos = strptime (str_date, "%Y-%m-%dT%H:%M:%S", &tm_date);
        if (pos)
        {
            if (use_local_time)
            {
                tv->tv_sec = mktime (&tm_date);
            }
            else
            {
                /* convert to UTC and add timezone_offset */
                time_now = mktime (&tm_date);
                gmtime_r (&time_now, &tm_date_gm);
                localtime_r (&time_now, &tm_date_local);
                time_gm = mktime (&tm_date_gm);
                time_local = mktime (&tm_date_local);
                tv->tv_sec = mktime (&tm_date_local)
                    + (time_local - time_gm)
                    + timezone_offset;
            }
            rc = 1;
        }
    }
    else
    {
        /* timestamp format: "1704402062" */
        error = NULL;
        value = strtoll (string, &error, 10);
        if (error && !error[0] && (value >= 0))
        {
            tv->tv_sec = (time_t)value;
            rc = 1;
        }
    }

    free (string);

    if (!rc)
    {
        tv->tv_sec = 0;
        tv->tv_usec = 0;
    }

    return rc;
}

/*
 * Returns difference between two times.
 *
 * The following variables are set, if pointer is not NULL:
 *   - number of total seconds between the two times (basic subtraction)
 *   - number of days/hours/minutes/seconds between the two times
 */

void
util_get_time_diff (time_t time1, time_t time2,
                    time_t *total_seconds,
                    int *days, int *hours, int *minutes, int *seconds)
{
    time_t diff;

    diff = time2 - time1;

    if (total_seconds)
        *total_seconds = diff;
    if (days)
        *days = diff / (60 * 60 * 24);
    if (hours)
        *hours = (diff % (60 * 60 * 24)) / (60 * 60);
    if (minutes)
        *minutes = ((diff % (60 * 60 * 24)) % (60 * 60)) / 60;
    if (seconds)
        *seconds = ((diff % (60 * 60 * 24)) % (60 * 60)) % 60;
}

/*
 * Parses a string with a delay and optional unit, returns the delay in
 * microseconds.
 *
 * The delay is a number followed by a unit which can be:
 *   - "us": microseconds
 *   - "ms": milliseconds
 *   - "s": seconds
 *   - "m": minutes
 *   - "h": hours
 *
 * The default factor sets the default unit:
 *   - 1: microseconds
 *   - 1000: milliseconds
 *   - 1000000: seconds
 *   - 60000000: minutes
 *   - 3600000000: hours
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
util_parse_delay (const char *string_delay, unsigned long long default_factor,
                  unsigned long long *delay)
{
    const char *pos;
    char *str_number, *error;
    unsigned long long factor;

    if (!delay)
        return 0;

    *delay = 0;

    if (!string_delay || !string_delay[0] || (default_factor < 1))
        return 0;

    factor = default_factor;

    pos = string_delay;
    while (pos[0] && isdigit ((unsigned char)pos[0]))
    {
        pos++;
    }

    if ((pos > string_delay) && pos[0])
    {
        str_number = string_strndup (string_delay, pos - string_delay);
        if (strcmp (pos, "us") == 0)
            factor = 1ULL;
        else if (strcmp (pos, "ms") == 0)
            factor = 1000ULL;
        else if (strcmp (pos, "s") == 0)
            factor = 1000ULL * 1000ULL;
        else if (strcmp (pos, "m") == 0)
            factor = 1000ULL * 1000ULL * 60ULL;
        else if (strcmp (pos, "h") == 0)
            factor = 1000ULL * 1000ULL * 60ULL * 60ULL;
        else
            return 0;
    }
    else
    {
        if (string_delay[0] == '-')
            return 0;
        str_number = strdup (string_delay);
    }

    if (!str_number)
        return 0;

    error = NULL;
    *delay = strtoull (str_number, &error, 10);
    if (!error || error[0])
    {
        free (str_number);
        *delay = 0;
        return 0;
    }

    *delay *= factor;

    free (str_number);

    return 1;
}

/*
 * Gets version number (integer) with a version as string.
 *
 * Non-digit chars like "-dev" are ignored.
 *
 * Examples:
 *   "4.0.0"     ==> 67108864 (== 0x04000000)
 *   "1.0"       ==> 16777216 (== 0x01000000)
 *   "0.3.2-dev" ==> 197120   (== 0x00030200)
 *   "0.3.2-rc1" ==> 197120   (== 0x00030200)
 *   "0.3.2"     ==> 197120   (== 0x00030200)
 *   "0.3.1.1"   ==> 196865   (== 0x00030101)
 *   "0.3.1"     ==> 196864   (== 0x00030100)
 *   "0.3.0"     ==> 196608   (== 0x00030000)
 */

int
util_version_number (const char *version)
{
    char **items, buf[64], *error;
    const char *ptr_item;
    int num_items, i, version_int[4], index_buf;
    long number;

    if (!version || !version[0])
        return 0;

    items = string_split (version, ".", NULL,
                          WEECHAT_STRING_SPLIT_STRIP_LEFT
                          | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                          | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                          4, &num_items);
    for (i = 0; i < 4; i++)
    {
        version_int[i] = 0;
        if (items && (i < num_items))
        {
            ptr_item = items[i];
            index_buf = 0;
            while (ptr_item && ptr_item[0] && (index_buf < (int)sizeof (buf) - 1))
            {
                if (ptr_item[0] == '-')
                    break;
                if (isdigit ((unsigned char)ptr_item[0]))
                {
                    buf[index_buf] = ptr_item[0];
                    index_buf++;
                }
                ptr_item = utf8_next_char (ptr_item);
            }
            buf[index_buf] = '\0';
            if (buf[0])
            {
                error = NULL;
                number = strtol (buf, &error, 10);
                if (error && !error[0])
                {
                    if (number < 0)
                        number = 0;
                    else if (number > 0xFF)
                        number = 0xFF;
                    version_int[i] = number;
                }
            }
        }
    }
    string_free_split (items);

    return (version_int[0] << 24) | (version_int[1] << 16)
        | (version_int[2] << 8) | version_int[3];
}
