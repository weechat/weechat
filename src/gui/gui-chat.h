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


#ifndef __WEECHAT_GUI_CHAT_H
#define __WEECHAT_GUI_CHAT_H 1

struct t_gui_window;
struct t_gui_buffer;
struct t_gui_line;

#define gui_chat_printf(buffer, argz...)    \
    gui_chat_printf_date(buffer, 0, ##argz) \

enum t_gui_prefix
{
    GUI_CHAT_PREFIX_INFO = 0,
    GUI_CHAT_PREFIX_ERROR,
    GUI_CHAT_PREFIX_NETWORK,
    GUI_CHAT_PREFIX_ACTION,
    GUI_CHAT_PREFIX_JOIN,
    GUI_CHAT_PREFIX_QUIT,
    
    GUI_CHAT_PREFIX_NUMBER,
};

extern char *gui_chat_prefix[GUI_CHAT_PREFIX_NUMBER];
extern char gui_chat_prefix_empty[];
extern int gui_chat_time_length;

/* chat functions */

extern void gui_chat_prefix_build_empty ();
extern void gui_chat_prefix_build ();
extern int gui_chat_strlen_screen (char *string);
extern int gui_chat_string_real_pos (char *string, int pos);
extern void gui_chat_get_word_info (struct t_gui_window *window,
                                    char *data, int *word_start_offset,
                                    int *word_end_offset,
                                    int *word_length_with_spaces,
                                    int *word_length);
extern void gui_chat_change_time_format ();
extern int gui_chat_get_line_align (struct t_gui_buffer *buffer,
                                    struct t_gui_line *line,
                                    int with_suffix);
extern int gui_chat_line_search (struct t_gui_line *line, char *text,
                                 int case_sensitive);
extern void gui_chat_line_free (struct t_gui_line *line);
extern void gui_chat_printf_date (struct t_gui_buffer *buffer, time_t date,
                                  char *message, ...);
extern void gui_chat_printf_raw_data (void *server, int send, int modified,
                                      char *message);

/* chat functions (GUI dependent) */

extern void gui_chat_draw_title (struct t_gui_buffer *buffer, int erase);
extern char *gui_chat_string_next_char (struct t_gui_window *window,
                                        unsigned char *string,
                                        int apply_style);
extern void gui_chat_draw (struct t_gui_buffer *buffer, int erase);
extern void gui_chat_draw_line (struct t_gui_buffer *buffer,
                                struct t_gui_line *line);

#endif /* gui-chat.h */
