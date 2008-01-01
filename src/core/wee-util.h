/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __WEECHAT_UTIL_H
#define __WEECHAT_UTIL_H 1

extern int util_timeval_cmp (struct timeval *tv1, struct timeval *tv2);
extern long util_timeval_diff (struct timeval *tv1, struct timeval *tv2);
extern void util_timeval_add (struct timeval *tv, long interval);
extern int util_get_time_length (char *time_format);
extern void util_catch_signal (int signum, void (*handler)(int));
extern int util_create_dir (char *directory, int permissions);
extern void util_exec_on_files (char *directory, void *data,
                                int (*callback)(void *data, char *filename));
extern char *util_search_full_lib_name (char *filename, char *sys_directory);

#endif /* wee-util.h */
