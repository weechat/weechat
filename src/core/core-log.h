/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
