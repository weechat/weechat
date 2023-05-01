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

#ifndef WEECHAT_INPUT_H
#define WEECHAT_INPUT_H

struct t_gui_buffer;
struct t_weechat_plugin;

extern char **input_commands_allowed;

extern int input_exec_command (struct t_gui_buffer *buffer,
                               int any_plugin,
                               struct t_weechat_plugin *plugin,
                               const char *string,
                               const char *commands_allowed);
extern int input_data (struct t_gui_buffer *buffer, const char *data,
                       const char *commands_allowed, int split_newline);
extern int input_data_delayed (struct t_gui_buffer *buffer, const char *data,
                               const char *commands_allowed, int split_newline,
                               long delay);

#endif /* WEECHAT_INPUT_H */
