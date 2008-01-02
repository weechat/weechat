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

extern void gui_input_optimize_size (struct t_gui_buffer *buffer);
extern void gui_input_init_color_mask (struct t_gui_buffer *buffer);
extern void gui_input_move (struct t_gui_buffer *buffer, char *target,
                            char *source, int size);
extern int gui_input_insert_string (struct t_gui_buffer *buffer, char *string,
                                    int pos);
extern void gui_input_complete (struct t_gui_buffer *buffer);
extern void gui_input_delete_line (struct t_gui_buffer *buffer);
extern int gui_input_get_prompt_length (struct t_gui_buffer *buffer);

/* input functions (GUI dependent) */

extern void gui_input_draw (struct t_gui_buffer *buffer, int erase);

#endif /* gui-input.h */
