/*
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_SCRIPT_BUFFER_H
#define WEECHAT_PLUGIN_SCRIPT_BUFFER_H

#define SCRIPT_BUFFER_NAME "scripts"

struct t_script_repo;

extern struct t_gui_buffer *script_buffer;
extern int script_buffer_selected_line;
extern struct t_script_repo *script_buffer_detail_script;
extern int script_buffer_detail_script_last_line;
extern int script_buffer_detail_script_line_diff;

extern void script_buffer_refresh (int clear);
extern void script_buffer_set_current_line (int line);
extern void script_buffer_show_detail_script (struct t_script_repo *script);
extern void script_buffer_get_window_info (struct t_gui_window *window,
                                           int *start_line_y, int *chat_height);
extern void script_buffer_check_line_outside_window ();
extern int script_buffer_window_scrolled_cb (const void *pointer, void *data,
                                             const char *signal,
                                             const char *type_data,
                                             void *signal_data);
extern int script_buffer_input_cb (const void *pointer, void *data,
                                   struct t_gui_buffer *buffer,
                                   const char *input_data);
extern int script_buffer_close_cb (const void *pointer, void *data,
                                   struct t_gui_buffer *buffer);
extern void script_buffer_set_callbacks ();
extern void script_buffer_set_keys (struct t_hashtable *hashtable);
extern void script_buffer_set_localvar_filter ();
extern void script_buffer_open ();

#endif /* WEECHAT_PLUGIN_SCRIPT_BUFFER_H */
