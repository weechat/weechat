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

#ifndef WEECHAT_GUI_INPUT_H
#define WEECHAT_GUI_INPUT_H

struct t_gui_buffer;

/* input variables */

extern char *gui_input_clipboard;

/* input functions */

extern void gui_input_replace_input (struct t_gui_buffer *buffer,
                                     const char *new_input);
extern void gui_input_paste_pending_signal ();
extern void gui_input_text_changed_modifier_and_signal (struct t_gui_buffer *buffer,
                                                        int save_undo,
                                                        int stop_completion);
extern void gui_input_search_signal (struct t_gui_buffer *buffer);
extern void gui_input_set_pos (struct t_gui_buffer *buffer, int pos);
extern void gui_input_insert_string (struct t_gui_buffer *buffer,
                                     const char *string);
extern void gui_input_clipboard_paste (struct t_gui_buffer *buffer);
extern void gui_input_return (struct t_gui_buffer *buffer);
extern void gui_input_split_return (struct t_gui_buffer *buffer);
extern void gui_input_complete_next (struct t_gui_buffer *buffer);
extern void gui_input_complete_previous (struct t_gui_buffer *buffer);
extern void gui_input_search_text_here (struct t_gui_buffer *buffer);
extern void gui_input_search_text (struct t_gui_buffer *buffer);
extern void gui_input_search_history (struct t_gui_buffer *buffer);
extern void gui_input_search_compile_regex (struct t_gui_buffer *buffer);
extern void gui_input_search_switch_case (struct t_gui_buffer *buffer);
extern void gui_input_search_switch_regex (struct t_gui_buffer *buffer);
extern void gui_input_search_switch_where (struct t_gui_buffer *buffer);
extern void gui_input_search_previous (struct t_gui_buffer *buffer);
extern void gui_input_search_next (struct t_gui_buffer *buffer);
extern void gui_input_search_stop_here (struct t_gui_buffer *buffer);
extern void gui_input_search_stop (struct t_gui_buffer *buffer);
extern void gui_input_delete_previous_char (struct t_gui_buffer *buffer);
extern void gui_input_delete_next_char (struct t_gui_buffer *buffer);
extern void gui_input_delete_previous_word (struct t_gui_buffer *buffer);
extern void gui_input_delete_previous_word_whitespace (struct t_gui_buffer *buffer);
extern void gui_input_delete_next_word (struct t_gui_buffer *buffer);
extern void gui_input_delete_beginning_of_line (struct t_gui_buffer *buffer);
extern void gui_input_delete_end_of_line (struct t_gui_buffer *buffer);
extern void gui_input_delete_beginning_of_input (struct t_gui_buffer *buffer);
extern void gui_input_delete_end_of_input (struct t_gui_buffer *buffer);
extern void gui_input_delete_line (struct t_gui_buffer *buffer);
extern void gui_input_delete_input (struct t_gui_buffer *buffer);
extern void gui_input_transpose_chars (struct t_gui_buffer *buffer);
extern void gui_input_move_beginning_of_line (struct t_gui_buffer *buffer);
extern void gui_input_move_end_of_line (struct t_gui_buffer *buffer);
extern void gui_input_move_beginning_of_input (struct t_gui_buffer *buffer);
extern void gui_input_move_end_of_input (struct t_gui_buffer *buffer);
extern void gui_input_move_previous_char (struct t_gui_buffer *buffer);
extern void gui_input_move_next_char (struct t_gui_buffer *buffer);
extern void gui_input_move_previous_word (struct t_gui_buffer *buffer);
extern void gui_input_move_next_word (struct t_gui_buffer *buffer);
extern void gui_input_move_previous_line (struct t_gui_buffer *buffer);
extern void gui_input_move_next_line (struct t_gui_buffer *buffer);
extern void gui_input_history_local_previous (struct t_gui_buffer *buffer);
extern void gui_input_history_local_next (struct t_gui_buffer *buffer);
extern void gui_input_history_global_previous (struct t_gui_buffer *buffer);
extern void gui_input_history_global_next (struct t_gui_buffer *buffer);
extern void gui_input_history_use_get_next (struct t_gui_buffer *buffer);
extern void gui_input_grab_key (struct t_gui_buffer *buffer, int command,
                                const char *delay);
extern void gui_input_grab_mouse (struct t_gui_buffer *buffer, int area);
extern void gui_input_insert (struct t_gui_buffer *buffer, const char *args);
extern void gui_input_undo (struct t_gui_buffer *buffer);
extern void gui_input_redo (struct t_gui_buffer *buffer);

#endif /* WEECHAT_GUI_INPUT_H */
