/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_FIFO_CONFIG_H
#define WEECHAT_PLUGIN_FIFO_CONFIG_H

#define FIFO_CONFIG_NAME "fifo"
#define FIFO_CONFIG_PRIO_NAME (TO_STR(FIFO_PLUGIN_PRIORITY) "|" FIFO_CONFIG_NAME)

extern struct t_config_option *fifo_config_file_enabled;
extern struct t_config_option *fifo_config_file_path;

extern int fifo_config_init (void);
extern int fifo_config_read (void);
extern int fifo_config_write (void);
extern void fifo_config_free (void);

#endif /* WEECHAT_PLUGIN_FIFO_CONFIG_H */
