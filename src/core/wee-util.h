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

extern long util_timeval_diff (struct timeval *, struct timeval *);
extern int util_get_time_length (char *);
extern int util_create_dir (char *, int);
extern void util_exec_on_files (char *, int (*)(char *));
extern char *util_search_full_lib_name (char *, char *);

#endif /* wee-util.h */
