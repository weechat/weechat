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

#include <regex.h>

struct t_gui_window;
struct t_gui_buffer;
struct t_gui_line;

#define GUI_CHAT_BUFFER_PRINTF_SIZE (128*1024)

#define gui_chat_printf(buffer, argz...)                        \
    gui_chat_printf_date_tags(buffer, 0, NULL, ##argz)

enum t_gui_prefix
{
    GUI_CHAT_PREFIX_ERROR = 0,
    GUI_CHAT_PREFIX_NETWORK,
    GUI_CHAT_PREFIX_ACTION,
    GUI_CHAT_PREFIX_JOIN,
    GUI_CHAT_PREFIX_QUIT,
    
    GUI_CHAT_NUM_PREFIXES,
};

extern char *gui_chat_prefix[GUI_CHAT_NUM_PREFIXES];
extern char gui_chat_prefix_empty[];
extern int gui_chat_time_length;

/* chat functions */

extern void gui_chat_prefix_build_empty ();
extern void gui_chat_prefix_build ();
extern int gui_chat_strlen_screen (const char *string);
extern char *gui_chat_string_add_offset (const char *string, int offset);
extern int gui_chat_string_real_pos (const char *string, int pos);
extern void gui_chat_get_word_info (struct t_gui_window *window,
                                    const char *data, int *word_start_offset,
                                    int *word_end_offset,
                                    int *word_length_with_spaces,
                                    int *word_length);
extern void gui_chat_change_time_format ();
extern int gui_chat_get_line_align (struct t_gui_buffer *buffer,
                                    struct t_gui_line *line,
                                    int with_suffix);
extern int gui_chat_line_displayed (struct t_gui_line *line);
extern struct t_gui_line *gui_chat_get_first_line_displayed (struct t_gui_buffer *buffer);
extern struct t_gui_line *gui_chat_get_last_line_displayed (struct t_gui_buffer *buffer);
extern struct t_gui_line *gui_chat_get_prev_line_displayed (struct t_gui_line *line);
extern struct t_gui_line *gui_chat_get_next_line_displayed (struct t_gui_line *line);
extern int gui_chat_line_search (struct t_gui_line *line, const char *text,
                                 int case_sensitive);
extern int gui_chat_line_match_regex (struct t_gui_line *line,
                                      regex_t *regex_prefix,
                                      regex_t *regex_message);
extern int gui_chat_line_match_tags (struct t_gui_line *line, int tags_count,
                                     char **tags_array);
extern void gui_chat_line_free (struct t_gui_buffer *buffer,
                                struct t_gui_line *line);
extern void gui_chat_line_free_all (struct t_gui_buffer *buffer);
extern void gui_chat_printf_date_tags (struct t_gui_buffer *buffer,
                                       time_t date, const char *tags,
                                       const char *message, ...);
extern void gui_chat_printf_y (struct t_gui_buffer *buffer, int y,
                               const char *message, ...);

/* chat functions (GUI dependent) */

extern void gui_chat_draw_title (struct t_gui_buffer *buffer, int erase);
extern char *gui_chat_string_next_char (struct t_gui_window *window,
                                        const unsigned char *string,
                                        int apply_style);
extern void gui_chat_draw (struct t_gui_buffer *buffer, int erase);
extern void gui_chat_draw_line (struct t_gui_buffer *buffer,
                                struct t_gui_line *line);

#endif /* gui-chat.h */
