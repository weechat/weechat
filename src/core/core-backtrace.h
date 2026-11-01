/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_BACKTRACE_H
#define WEECHAT_BACKTRACE_H

#define BACKTRACE_MAX 128

extern int weechat_backtrace_find_file (const char *filename, char *path,
                                        int path_size);
extern void weechat_backtrace (void);

#endif /* WEECHAT_BACKTRACE_H */
