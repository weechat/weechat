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

#ifndef WEECHAT_PLUGIN_FIFO_CONFIG_H
#define WEECHAT_PLUGIN_FIFO_CONFIG_H

#define FIFO_CONFIG_NAME "fifo"
#define FIFO_CONFIG_PRIO_NAME (TO_STR(FIFO_PLUGIN_PRIORITY) "|" FIFO_CONFIG_NAME)

extern struct t_config_option *fifo_config_file_enabled;
extern struct t_config_option *fifo_config_file_path;

extern int fifo_config_init ();
extern int fifo_config_read ();
extern int fifo_config_write ();
extern void fifo_config_free ();

#endif /* WEECHAT_PLUGIN_FIFO_CONFIG_H */
