/*
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

#ifndef WEECHAT_UTIL_H
#define WEECHAT_UTIL_H

#include <time.h>
#include <sys/time.h>

#ifdef HAVE_SYS_RESOURCE_H
struct t_rlimit_resource
{
    char *name;                        /* name of resource                  */
    int resource;                      /* value of resource                 */
};
#endif /* HAVE_SYS_RESOURCE_H */

/* limits */
extern void util_setrlimit ();

/* timeval */
extern int util_timeval_cmp (struct timeval *tv1, struct timeval *tv2);
extern long long util_timeval_diff (struct timeval *tv1, struct timeval *tv2);
extern void util_timeval_add (struct timeval *tv, long long interval);

/* time */
extern const char *util_get_time_string (const time_t *date);
extern void util_get_time_diff (time_t time1, time_t time2,
                                time_t *total_seconds,
                                int *days, int *hours, int *minutes,
                                int *seconds);

/* delay */

extern long util_parse_delay (const char *string_delay, long default_factor);

/* version */
extern int util_version_number (const char *version);

#endif /* WEECHAT_UTIL_H */
