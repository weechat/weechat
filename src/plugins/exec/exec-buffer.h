/*
 * Copyright (C) 2014-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_EXEC_BUFFER_H
#define WEECHAT_PLUGIN_EXEC_BUFFER_H

extern void exec_buffer_set_callbacks ();
extern struct t_gui_buffer *exec_buffer_new (const char *name,
                                             int free_content,
                                             int clear_buffer,
                                             int switch_to_buffer);

#endif /* WEECHAT_PLUGIN_EXEC_BUFFER_H */
