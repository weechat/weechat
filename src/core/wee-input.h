/*
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WEECHAT_INPUT_H
#define __WEECHAT_INPUT_H 1

struct t_gui_buffer;
struct t_weechat_plugin;

extern void input_exec_command (struct t_gui_buffer *buffer,
                                int any_plugin,
                                struct t_weechat_plugin *plugin,
                                const char *string);
extern void input_data (struct t_gui_buffer *buffer, const char *data);

#endif /* __WEECHAT_INPUT_H */
