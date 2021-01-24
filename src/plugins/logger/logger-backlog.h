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

#ifndef WEECHAT_PLUGIN_LOGGER_BACKLOG_H
#define WEECHAT_PLUGIN_LOGGER_BACKLOG_H

extern int logger_backlog_check_conditions (struct t_gui_buffer *buffer);
extern void logger_backlog (struct t_gui_buffer *buffer, const char *filename,
                            int lines);
extern int logger_backlog_signal_cb (const void *pointer, void *data,
                                     const char *signal,
                                     const char *type_data, void *signal_data);

#endif /* WEECHAT_PLUGIN_LOGGER_BACKLOG_H */
