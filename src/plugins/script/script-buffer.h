/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
extern void script_buffer_check_line_outside_window (void);
extern int script_buffer_window_scrolled_cb (const void *pointer, void *data,
                                             const char *signal,
                                             const char *type_data,
                                             void *signal_data);
extern int script_buffer_input_cb (const void *pointer, void *data,
                                   struct t_gui_buffer *buffer,
                                   const char *input_data);
extern int script_buffer_close_cb (const void *pointer, void *data,
                                   struct t_gui_buffer *buffer);
extern void script_buffer_set_callbacks (void);
extern void script_buffer_set_keys (struct t_hashtable *hashtable);
extern void script_buffer_set_localvar_filter (void);
extern void script_buffer_open (void);

#endif /* WEECHAT_PLUGIN_SCRIPT_BUFFER_H */
