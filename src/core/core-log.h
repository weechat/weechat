/*
 * SPDX-FileCopyrightText: 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

#ifndef WEECHAT_LOG_H
#define WEECHAT_LOG_H

#include <stdio.h>

extern char *weechat_log_filename;
extern FILE *weechat_log_file;
extern int weechat_log_use_time;

extern void log_init (void);
extern void log_close (void);
extern void log_printf (const char *message, ...);
extern void log_printf_hexa (const char *spaces, const char *string);
extern int log_crash_rename (void);

#endif /* WEECHAT_LOG_H */
