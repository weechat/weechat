/*
 * wee-util.c - some useful functions
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include "wee-util.h"
#include "wee-config.h"
#include "wee-log.h"
#include "wee-string.h"
#include "wee-utf8.h"
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
util_get_microseconds_string (long long microseconds)
{
    long long hour, min, sec, usec;
    char result[128];

    usec = microseconds % 1000000;
    sec = (microseconds / 1000000) % 60;
    min = ((microseconds / 1000000) / 60) % 60;
    hour = (microseconds / 1000000) / 3600;

    snprintf (result, sizeof (result),
              "%lld:%02lld:%02lld.%06lld",
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
 * Returns the delay in microseconds, -1 if error.
 */

long long
util_parse_delay (const char *string_delay, long long default_factor)
{
    const char *pos;
    char *str_number, *error;
    long long factor, delay;

    if (!string_delay || !string_delay[0] || (default_factor < 1))
        return -1LL;

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
            factor = 1LL;
        else if (strcmp (pos, "ms") == 0)
            factor = 1000LL;
        else if (strcmp (pos, "s") == 0)
            factor = 1000LL * 1000LL;
        else if (strcmp (pos, "m") == 0)
            factor = 1000LL * 1000LL * 60LL;
        else if (strcmp (pos, "h") == 0)
            factor = 1000LL * 1000LL * 60LL * 60LL;
        else
            return -1LL;
    }
    else
    {
        str_number = strdup (string_delay);
    }

    if (!str_number)
        return -1LL;

    error = NULL;
    delay = strtoll (str_number, &error, 10);
    if (!error || error[0] || (delay < 0))
    {
        free (str_number);
        return -1LL;
    }

    free (str_number);

    return delay * factor;
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
    if (items)
        string_free_split (items);

    return (version_int[0] << 24) | (version_int[1] << 16)
        | (version_int[2] << 8) | version_int[3];
}
