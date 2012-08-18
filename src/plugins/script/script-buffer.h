/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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

#ifndef __WEECHAT_SCRIPT_BUFFER_H
#define __WEECHAT_SCRIPT_BUFFER_H 1

#define SCRIPT_BUFFER_NAME "scripts"

struct t_repo_script;

extern struct t_gui_buffer *script_buffer;
extern int script_buffer_selected_line;
extern struct t_repo_script *script_buffer_detail_script;

extern void script_buffer_refresh (int clear);
extern void script_buffer_set_current_line (int line);
extern void script_buffer_show_detail_script (struct t_repo_script *script);
extern void script_buffer_check_line_outside_window ();
extern int script_buffer_window_scrolled_cb (void *data, const char *signal,
                                             const char *type_data,
                                             void *signal_data);
extern int script_buffer_input_cb (void *data, struct t_gui_buffer *buffer,
                                   const char *input_data);
extern int script_buffer_close_cb (void *data, struct t_gui_buffer *buffer);
extern void script_buffer_set_callbacks ();
extern void script_buffer_set_keys ();
extern void script_buffer_open ();

#endif /* __WEECHAT_SCRIPT_BUFFER_H */
