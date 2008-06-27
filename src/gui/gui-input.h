/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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

/* input variables */

extern char *gui_input_clipboard;

/* input functions */

extern void gui_input_prompt_changed_signal ();
extern void gui_input_text_changed_signal ();
extern void gui_input_optimize_size (struct t_gui_buffer *buffer);
extern void gui_input_init_color_mask (struct t_gui_buffer *buffer);
extern void gui_input_move (struct t_gui_buffer *buffer, char *target,
                            const char *source, int size);
extern int gui_input_insert_string (struct t_gui_buffer *buffer,
                                    const char *string, int pos);
extern int gui_input_get_prompt_length (struct t_gui_buffer *buffer);
extern void gui_input_return ();
extern void gui_input_clipboard_paste ();
extern void gui_input_complete_next ();
extern void gui_input_complete_previous ();
extern void gui_input_search_text ();
extern void gui_input_delete_previous_char ();
extern void gui_input_delete_next_char ();
extern void gui_input_delete_previous_word ();
extern void gui_input_delete_next_word ();
extern void gui_input_delete_beginning_of_line ();
extern void gui_input_delete_end_of_line ();
extern void gui_input_delete_line ();
extern void gui_input_transpose_chars ();
extern void gui_input_move_beginning_of_line ();
extern void gui_input_move_end_of_line ();
extern void gui_input_move_previous_char ();
extern void gui_input_move_next_char ();
extern void gui_input_move_previous_word ();
extern void gui_input_move_next_word ();
extern void gui_input_history_previous ();
extern void gui_input_history_next ();
extern void gui_input_history_global_previous ();
extern void gui_input_history_global_next ();
extern void gui_input_jump_smart ();
extern void gui_input_jump_last_buffer ();
extern void gui_input_jump_previous_buffer ();
extern void gui_input_hotlist_clear ();
extern void gui_input_grab_key ();
extern void gui_input_scroll_unread ();
extern void gui_input_set_unread ();
extern void gui_input_set_unread_current_buffer ();
extern void gui_input_insert ();

/* input functions (GUI dependent) */

extern void gui_input_draw (struct t_gui_buffer *buffer, int erase);

#endif /* gui-input.h */
