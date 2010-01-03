/*
 * Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __WEECHAT_GUI_INPUT_H
#define __WEECHAT_GUI_INPUT_H 1

struct t_gui_buffer;
struct t_gui_window;

/* input variables */

extern char *gui_input_clipboard;

/* input functions */

extern void gui_input_paste_pending_signal ();
extern void gui_input_text_changed_modifier_and_signal (struct t_gui_buffer *buffer);
extern void gui_input_move (struct t_gui_buffer *buffer, char *target,
                            const char *source, int size);
extern int gui_input_insert_string (struct t_gui_buffer *buffer,
                                    const char *string, int pos);
extern void gui_input_clipboard_paste (struct t_gui_buffer *buffer);
extern void gui_input_return (struct t_gui_window *window);
extern void gui_input_complete_next (struct t_gui_buffer *buffer);
extern void gui_input_complete_previous (struct t_gui_buffer *buffer);
extern void gui_input_search_text (struct t_gui_window *window);
extern void gui_input_delete_previous_char (struct t_gui_buffer *buffer);
extern void gui_input_delete_next_char (struct t_gui_buffer *buffer);
extern void gui_input_delete_previous_word (struct t_gui_buffer *buffer);
extern void gui_input_delete_next_word (struct t_gui_buffer *buffer);
extern void gui_input_delete_beginning_of_line (struct t_gui_buffer *buffer);
extern void gui_input_delete_end_of_line (struct t_gui_buffer *buffer);
extern void gui_input_delete_line (struct t_gui_buffer *buffer);
extern void gui_input_transpose_chars (struct t_gui_buffer *buffer);
extern void gui_input_move_beginning_of_line (struct t_gui_buffer *buffer);
extern void gui_input_move_end_of_line (struct t_gui_buffer *buffer);
extern void gui_input_move_previous_char (struct t_gui_buffer *buffer);
extern void gui_input_move_next_char (struct t_gui_buffer *buffer);
extern void gui_input_move_previous_word (struct t_gui_buffer *buffer);
extern void gui_input_move_next_word (struct t_gui_buffer *buffer);
extern void gui_input_history_previous (struct t_gui_window *window);
extern void gui_input_history_next (struct t_gui_window *window);
extern void gui_input_history_global_previous (struct t_gui_buffer *buffer);
extern void gui_input_history_global_next (struct t_gui_buffer *buffer);
extern void gui_input_jump_smart (struct t_gui_window *window);
extern void gui_input_jump_last_buffer (struct t_gui_window *window);
extern void gui_input_jump_previously_visited_buffer (struct t_gui_window *window);
extern void gui_input_jump_next_visited_buffer (struct t_gui_window *window);
extern void gui_input_hotlist_clear (struct t_gui_window *window);
extern void gui_input_grab_key (struct t_gui_buffer *buffer);
extern void gui_input_grab_key_command (struct t_gui_buffer *buffer);
extern void gui_input_scroll_unread (struct t_gui_window *window);
extern void gui_input_set_unread ();
extern void gui_input_set_unread_buffer (struct t_gui_buffer *buffer);
extern void gui_input_switch_active_buffer (struct t_gui_window *window);
extern void gui_input_insert (struct t_gui_buffer *buffer, const char *args);

#endif /* gui-input.h */
