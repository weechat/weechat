/*
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_GUI_CHAT_H
#define WEECHAT_GUI_CHAT_H

#include <stdio.h>
#include <time.h>

struct t_hashtable;
struct t_gui_window;
struct t_gui_buffer;
struct t_gui_line;

#define gui_chat_printf(buffer, argz...)                        \
    gui_chat_printf_date_tags(buffer, 0, NULL, ##argz)

#define gui_chat_printf_y(buffer, y, argz...)                   \
    gui_chat_printf_y_date_tags(buffer, y, 0, NULL, ##argz)

#define GUI_CHAT_TAG_NO_HIGHLIGHT "no_highlight"

#define GUI_CHAT_PREFIX_ERROR_DEFAULT   "=!="
#define GUI_CHAT_PREFIX_NETWORK_DEFAULT "--"
#define GUI_CHAT_PREFIX_ACTION_DEFAULT  " *"
#define GUI_CHAT_PREFIX_JOIN_DEFAULT    "-->"
#define GUI_CHAT_PREFIX_QUIT_DEFAULT    "<--"

enum t_gui_chat_prefix
{
    GUI_CHAT_PREFIX_ERROR = 0,
    GUI_CHAT_PREFIX_NETWORK,
    GUI_CHAT_PREFIX_ACTION,
    GUI_CHAT_PREFIX_JOIN,
    GUI_CHAT_PREFIX_QUIT,

    GUI_CHAT_NUM_PREFIXES,
};

enum t_gui_chat_mute
{
    GUI_CHAT_MUTE_DISABLED = 0,
    GUI_CHAT_MUTE_BUFFER,
    GUI_CHAT_MUTE_ALL_BUFFERS,
};

extern char *gui_chat_prefix[GUI_CHAT_NUM_PREFIXES];
extern char gui_chat_prefix_empty[];
extern int gui_chat_time_length;
extern int gui_chat_mute;
extern struct t_gui_buffer *gui_chat_mute_buffer;
extern int gui_chat_display_tags;

/* chat functions */

extern void gui_chat_init ();
extern void gui_chat_prefix_build ();
extern int gui_chat_strlen (const char *string);
extern int gui_chat_strlen_screen (const char *string);
extern const char *gui_chat_string_add_offset (const char *string, int offset);
extern const char *gui_chat_string_add_offset_screen (const char *string,
                                                      int offset_screen);
extern int gui_chat_string_real_pos (const char *string, int pos,
                                     int use_screen_size);
extern int gui_chat_string_pos (const char *string, int real_pos);
extern void gui_chat_get_word_info (struct t_gui_window *window,
                                    const char *data, int *word_start_offset,
                                    int *word_end_offset,
                                    int *word_length_with_spaces,
                                    int *word_length);
extern char *gui_chat_get_time_string (time_t date);
extern int gui_chat_get_time_length ();
extern void gui_chat_change_time_format ();
extern int gui_chat_buffer_valid (struct t_gui_buffer *buffer,
                                  int buffer_type);
extern void gui_chat_printf_date_tags (struct t_gui_buffer *buffer,
                                       time_t date, const char *tags,
                                       const char *message, ...);
extern void gui_chat_printf_y_date_tags (struct t_gui_buffer *buffer, int y,
                                         time_t date, const char *tags,
                                         const char *message, ...);
extern void gui_chat_print_lines_waiting_buffer (FILE *f);
extern int gui_chat_hsignal_quote_line_cb (const void *pointer, void *data,
                                           const char *signal,
                                           struct t_hashtable *hashtable);
extern void gui_chat_end ();

/* chat functions (GUI dependent) */

extern const char *gui_chat_string_next_char (struct t_gui_window *window,
                                              struct t_gui_line *line,
                                              const unsigned char *string,
                                              int apply_style,
                                              int apply_style_inactive,
                                              int nick_offline);
extern void gui_chat_draw (struct t_gui_buffer *buffer, int clear_chat);

#endif /* WEECHAT_GUI_CHAT_H */
