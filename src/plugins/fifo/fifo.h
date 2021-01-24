/*
 * Copyright (C) 2003-2021 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_FIFO_H
#define WEECHAT_PLUGIN_FIFO_H

#define weechat_plugin weechat_fifo_plugin
#define FIFO_PLUGIN_NAME "fifo"

extern struct t_weechat_plugin *weechat_fifo_plugin;
extern int fifo_quiet;
extern int fifo_fd;
extern char *fifo_filename;

extern void fifo_create ();
extern void fifo_remove ();

#endif /* WEECHAT_PLUGIN_FIFO_H */
