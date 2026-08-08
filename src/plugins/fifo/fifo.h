/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_FIFO_H
#define WEECHAT_PLUGIN_FIFO_H

#define weechat_plugin weechat_fifo_plugin
#define FIFO_PLUGIN_NAME "fifo"
#define FIFO_PLUGIN_PRIORITY 9000

extern struct t_weechat_plugin *weechat_fifo_plugin;
extern int fifo_quiet;
extern int fifo_fd;
extern char *fifo_filename;

extern void fifo_create (void);
extern void fifo_remove (void);

#endif /* WEECHAT_PLUGIN_FIFO_H */
