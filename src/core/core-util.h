/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_UTIL_H
#define WEECHAT_UTIL_H

#include <time.h>
#include <sys/time.h>

/* Parse numbers */
extern int util_parse_int (const char *string, int base, int *result);
extern int util_parse_long (const char *string, int base, long *result);
extern int util_parse_longlong (const char *string, int base, long long *result);

/* Timeval */
extern int util_timeval_cmp (struct timeval *tv1, struct timeval *tv2);
extern long long util_timeval_diff (struct timeval *tv1, struct timeval *tv2);
extern void util_timeval_add (struct timeval *tv, long long interval);

/* Time */
extern char *util_get_microseconds_string (unsigned long long microseconds);
extern const char *util_get_time_string (const time_t *date);
extern int util_strftimeval (char *string, int max, const char *format,
                             struct timeval *tv);
extern int util_parse_time (const char *datetime, struct timeval *tv);
extern void util_get_time_diff (time_t time1, time_t time2,
                                time_t *total_seconds,
                                int *days, int *hours, int *minutes,
                                int *seconds);

/* Delay */

extern int util_parse_delay (const char *string_delay,
                             unsigned long long default_factor,
                             unsigned long long *delay);

/* Version */
extern unsigned long util_version_number (const char *version);

#endif /* WEECHAT_UTIL_H */
