/*
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

#ifndef WEECHAT_UTIL_H
#define WEECHAT_UTIL_H

#include <time.h>
#include <sys/time.h>

/* timeval */
extern int util_timeval_cmp (struct timeval *tv1, struct timeval *tv2);
extern long long util_timeval_diff (struct timeval *tv1, struct timeval *tv2);
extern void util_timeval_add (struct timeval *tv, long long interval);

/* time */
extern char *util_get_microseconds_string (unsigned long long microseconds);
extern const char *util_get_time_string (const time_t *date);
extern int util_strftimeval (char *string, int max, const char *format,
                             struct timeval *tv);
extern int util_parse_time (const char *datetime, struct timeval *tv);
extern void util_get_time_diff (time_t time1, time_t time2,
                                time_t *total_seconds,
                                int *days, int *hours, int *minutes,
                                int *seconds);

/* delay */

extern int util_parse_delay (const char *string_delay,
                             unsigned long long default_factor,
                             unsigned long long *delay);

/* version */
extern int util_version_number (const char *version);

#endif /* WEECHAT_UTIL_H */
