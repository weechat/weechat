/*
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_GUI_LINE_H
#define WEECHAT_GUI_LINE_H

#include <time.h>
#include <regex.h>

struct t_infolist;

/* line structures */

struct t_gui_line_data
{
    struct t_gui_buffer *buffer;       /* pointer to buffer                 */
    int id;                            /* formatted buffer: (almost) unique */
                                       /* line id in buffer                 */
                                       /* free buffer: equals to "y"        */
    int y;                             /* line position (for free buffer)   */
    time_t date;                       /* date/time of line (may be past)   */
    int date_usec;                     /* microseconds for date             */
    time_t date_printed;               /* date/time when weechat print it   */
    int date_usec_printed;             /* microseconds for date printed     */
    char *str_time;                    /* time string (for display)         */
    int tags_count;                    /* number of tags for line           */
    char **tags_array;                 /* tags for line                     */
    char displayed;                    /* 1 if line is displayed            */
    char notify_level;                 /* notify level for the line         */
    char highlight;                    /* 1 if line has highlight           */
    char refresh_needed;               /* 1 if refresh asked (free buffer)  */
    char *prefix;                      /* prefix for line (may be NULL)     */
    int prefix_length;                 /* prefix length (on screen)         */
    char *message;                     /* line content (after prefix)       */
};

struct t_gui_line
{
    struct t_gui_line_data *data;      /* pointer to line data              */
    struct t_gui_line *prev_line;      /* link to previous line             */
    struct t_gui_line *next_line;      /* link to next line                 */
};

struct t_gui_lines
{
    struct t_gui_line *first_line;     /* pointer to first line             */
    struct t_gui_line *last_line;      /* pointer to last line              */
    struct t_gui_line *last_read_line; /* last read line                    */
    int lines_count;                   /* number of lines                   */
    int first_line_not_read;           /* if 1, marker is before first line */
    int lines_hidden;                  /* 1 if at least one line is hidden  */
    int buffer_max_length;             /* max length for buffer name (for   */
                                       /* mixed lines only)                 */
    int buffer_max_length_refresh;     /* refresh asked for buffer max len. */
    int prefix_max_length;             /* max length for prefix align       */
    int prefix_max_length_refresh;     /* refresh asked for prefix max len. */
};

/* line functions */

extern struct t_gui_lines *gui_line_lines_alloc (void);
extern void gui_line_lines_free (struct t_gui_lines *lines);
extern void gui_line_tags_alloc (struct t_gui_line_data *line_data,
                                 const char *tags);
extern void gui_line_tags_free (struct t_gui_line_data *line_data);
extern void gui_line_get_prefix_for_display (struct t_gui_line *line,
                                             char **prefix, int *length,
                                             char **color, int *prefix_is_nick);
extern int gui_line_get_align (struct t_gui_buffer *buffer,
                               struct t_gui_line *line,
                               int with_suffix, int first_line);
extern char *gui_line_build_string_prefix_message (const char *prefix,
                                                   const char *message);
extern char *gui_line_build_string_message_nick_offline (const char *message);
extern char *gui_line_build_string_message_tags (const char *message,
                                                 int tags_count,
                                                 char **tags_array,
                                                 int colors);
extern int gui_line_is_displayed (struct t_gui_line *line);
extern struct t_gui_line *gui_line_get_first_displayed (struct t_gui_buffer *buffer);
extern struct t_gui_line *gui_line_get_last_displayed (struct t_gui_buffer *buffer);
extern struct t_gui_line *gui_line_get_prev_displayed (struct t_gui_line *line);
extern struct t_gui_line *gui_line_get_next_displayed (struct t_gui_line *line);
extern struct t_gui_line *gui_line_search_by_id (struct t_gui_buffer *buffer,
                                                 int id);
extern int gui_line_search_text (struct t_gui_buffer *buffer,
                                 struct t_gui_line *line);
extern int gui_line_match_regex (struct t_gui_line_data *line_data,
                                 regex_t *regex_prefix,
                                 regex_t *regex_message);
extern int gui_line_has_tag_no_filter (struct t_gui_line_data *line_data);
extern int gui_line_match_tags (struct t_gui_line_data *line_data,
                                int tags_count, char ***tags_array);
extern const char *gui_line_search_tag_starting_with (struct t_gui_line *line,
                                                      const char *tag);
extern const char *gui_line_get_nick_tag (struct t_gui_line *line);
extern int gui_line_has_highlight (struct t_gui_line *line);
extern int gui_line_has_offline_nick (struct t_gui_line *line);
extern int gui_line_is_action (struct t_gui_line *line);
extern void gui_line_compute_buffer_max_length (struct t_gui_buffer *buffer,
                                                struct t_gui_lines *lines);
extern void gui_line_compute_prefix_max_length (struct t_gui_lines *lines);
extern void gui_line_mixed_free_buffer (struct t_gui_buffer *buffer);
extern void gui_line_mixed_free_all (struct t_gui_buffer *buffer);
extern void gui_line_free_data (struct t_gui_line *line);
extern void gui_line_free (struct t_gui_buffer *buffer,
                           struct t_gui_line *line);
extern void gui_line_free_all (struct t_gui_buffer *buffer);
extern int gui_line_get_max_notify_level (struct t_gui_line *line);
extern void gui_line_set_notify_level (struct t_gui_line *line,
                                       int max_notify_level);
extern void gui_line_set_highlight (struct t_gui_line *line,
                                    int max_notify_level);
extern struct t_gui_line *gui_line_new (struct t_gui_buffer *buffer,
                                        int y,
                                        time_t date,
                                        int date_usec,
                                        time_t date_printed,
                                        int date_usec_printed,
                                        const char *tags,
                                        const char *prefix,
                                        const char *message);
extern void gui_line_hook_update (struct t_gui_line *line,
                                  struct t_hashtable *hashtable,
                                  struct t_hashtable *hashtable2);
extern void gui_line_add (struct t_gui_line *line);
extern void gui_line_add_y (struct t_gui_line *line);
extern void gui_line_clear (struct t_gui_line *line);
extern void gui_line_mix_buffers (struct t_gui_buffer *buffer);
extern struct t_hdata *gui_line_hdata_lines_cb (const void *pointer,
                                                void *data,
                                                const char *hdata_name);
extern struct t_hdata *gui_line_hdata_line_cb (const void *pointer,
                                               void *data,
                                               const char *hdata_name);
extern struct t_hdata *gui_line_hdata_line_data_cb (const void *pointer,
                                                    void *data,
                                                    const char *hdata_name);
extern int gui_line_add_to_infolist (struct t_infolist *infolist,
                                     struct t_gui_lines *lines,
                                     struct t_gui_line *line);
extern void gui_lines_print_log (struct t_gui_lines *lines);

#endif /* WEECHAT_GUI_LINE_H */
