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


#ifndef __WEECHAT_GUI_BUFFER_H
#define __WEECHAT_GUI_BUFFER_H 1

enum t_gui_buffer_type
{
    GUI_BUFFER_TYPE_FORMATED = 0,
    GUI_BUFFER_TYPE_FREE,
};

#define GUI_BUFFER_NOTIFY_LEVEL_MIN     0
#define GUI_BUFFER_NOTIFY_LEVEL_MAX     3
#define GUI_BUFFER_NOTIFY_LEVEL_DEFAULT GUI_BUFFER_NOTIFY_LEVEL_MAX

#define GUI_TEXT_SEARCH_DISABLED 0
#define GUI_TEXT_SEARCH_BACKWARD 1
#define GUI_TEXT_SEARCH_FORWARD  2

#define GUI_BUFFER_INPUT_BLOCK_SIZE 256

/* buffer structures */

struct t_gui_line
{
    int y;                             /* line position (for free buffer)   */
    time_t date;                       /* date/time of line (may be past)   */
    time_t date_printed;               /* date/time when weechat print it   */
    char *str_time;                    /* time string (for display)         */
    int tags_count;                    /* number of tags for line           */
    char **tags_array;                 /* tags for line                     */
    char displayed;                    /* 1 if line is displayed            */
    char refresh_needed;               /* 1 if refresh asked (free buffer)  */
    char *prefix;                      /* prefix for line (may be NULL)     */
    int prefix_length;                 /* prefix length (on screen)         */
    char *message;                     /* line content (after prefix)       */
    struct t_gui_line *prev_line;      /* link to previous line             */
    struct t_gui_line *next_line;      /* link to next line                 */
};

struct t_gui_buffer
{
    struct t_weechat_plugin *plugin;   /* plugin which created this buffer  */
                                       /* (NULL for a WeeChat buffer)       */
    int number;                        /* buffer number (for jump/switch)   */
    char *category;                    /* category name                     */
    char *name;                        /* buffer name                       */
    enum t_gui_buffer_type type;       /* buffer type (formated, free, ..)  */
    int notify_level;                  /* 0 = never                         */
                                       /* 1 = highlight only                */
                                       /* 2 = highlight + msg               */
                                       /* 3 = highlight + msg + join/part   */
    int num_displayed;                 /* number of windows displaying buf. */
    
    /* close callback */
    int (*close_callback)(void *data,  /* called when buffer is closed      */
                          struct t_gui_buffer *buffer);
    void *close_callback_data;         /* data for callback                 */
    
    /* buffer title */
    char *title;                       /* buffer title                      */
    int title_refresh_needed;          /* refresh for title is needed ?     */
    
    /* chat content */
    struct t_gui_line *lines;          /* lines of chat window              */
    struct t_gui_line *last_line;      /* last line of chat window          */
    struct t_gui_line *last_read_line; /* last read line before jump        */
    int lines_count;                   /* number of lines in the buffer     */
    int lines_hidden;                  /* 1 if at least one line is hidden  */
    int prefix_max_length;             /* length for prefix align           */
    int chat_refresh_needed;           /* refresh for chat is needed ?      */
                                       /* (1=refresh, 2=erase+refresh)      */
    
    /* nicklist */ 
    int nicklist;                      /* = 1 if nicklist is enabled        */
    int nicklist_case_sensitive;       /* nicks are case sensitive ?        */
    struct t_gui_nick_group *nicklist_root; /* pointer to groups root       */
    int nicklist_max_length;           /* max length for a nick             */
    int nicklist_display_groups;       /* display groups ?                  */
    int nicklist_visible_count;        /* number of nicks/groups to display */
    int nicklist_refresh_needed;       /* refresh for nicklist is needed ?  */
    
    /* inupt */
    int input;                         /* = 1 if input is enabled           */
    int (*input_callback)(void *data,  /* called when user send data        */
                          struct t_gui_buffer *buffer,
                          char *input_data);
    void *input_callback_data;         /* data for callback                 */
                                       /* to this buffer                    */
    char *input_nick;                  /* self nick                         */
    char *input_buffer;                /* input buffer                      */
    char *input_buffer_color_mask;     /* color mask for input buffer       */
    int input_buffer_alloc;            /* input buffer: allocated size      */
    int input_buffer_size;             /* buffer size in bytes              */
    int input_buffer_length;           /* number of chars in buffer         */
    int input_buffer_pos;              /* position into buffer              */
    int input_buffer_1st_display;      /* first char displayed on screen    */
    int input_refresh_needed;          /* refresh for input is needed ?     */
    
    /* completion */
    struct t_gui_completion *completion; /* completion                      */
    
    /* history */
    struct t_gui_history *history;     /* commands history                  */
    struct t_gui_history *last_history;/* last command in history           */
    struct t_gui_history *ptr_history; /* current command in history        */
    int num_history;                   /* number of commands in history     */
    
    /* text search */
    int text_search;                   /* text search type                  */
    int text_search_exact;             /* exact search (case sensitive) ?   */
    int text_search_found;             /* 1 if text found, otherwise 0      */
    char *text_search_input;           /* input saved before text search    */
    
    /* keys associated to buffer */
    struct t_gui_key *keys;            /* keys specific to buffer           */
    struct t_gui_key *last_key;        /* last key for buffer               */
    
    /* link to previous/next buffer */
    struct t_gui_buffer *prev_buffer;  /* link to previous buffer           */
    struct t_gui_buffer *next_buffer;  /* link to next buffer               */
};

/* buffer variables */

extern struct t_gui_buffer *gui_buffers;
extern struct t_gui_buffer *last_gui_buffer;
extern struct t_gui_buffer *gui_previous_buffer;

/* buffer functions */

extern struct t_gui_buffer *gui_buffer_new (struct t_weechat_plugin *plugin,
                                            char *category, char *name,
                                            int (*input_callback)(void *data,
                                                                  struct t_gui_buffer *buffer,
                                                                  char *input_data),
                                            void *input_callback_data,
                                            int (*close_callback)(void *data,
                                                                  struct t_gui_buffer *buffer),
                                            void *close_callback_data);
extern int gui_buffer_valid (struct t_gui_buffer *buffer);
extern char *gui_buffer_get_string (struct t_gui_buffer *buffer,
                                    char *property);
extern void *gui_buffer_get_pointer (struct t_gui_buffer *buffer,
                                     char *property);
extern void gui_buffer_ask_title_refresh (struct t_gui_buffer *buffer,
                                          int refresh);
extern void gui_buffer_ask_chat_refresh (struct t_gui_buffer *buffer,
                                         int refresh);
extern void gui_buffer_ask_nicklist_refresh (struct t_gui_buffer *buffer,
                                             int refresh);
extern void gui_buffer_ask_input_refresh (struct t_gui_buffer *buffer,
                                          int refresh);
extern void gui_buffer_set_category (struct t_gui_buffer *buffer,
                                     char *category);
extern void gui_buffer_set_name (struct t_gui_buffer *buffer, char *name);
extern void gui_buffer_set_title (struct t_gui_buffer *buffer,
                                  char *new_title);
extern void gui_buffer_set_nicklist (struct t_gui_buffer *buffer,
                                     int nicklist);
extern void gui_buffer_set_nicklist_case_sensitive (struct t_gui_buffer * buffer,
                                                    int case_sensitive);
extern void gui_buffer_set_nick (struct t_gui_buffer *buffer, char *new_nick);
extern void gui_buffer_set (struct t_gui_buffer *buffer, char *property,
                            char *value);
extern struct t_gui_buffer *gui_buffer_search_main ();
extern struct t_gui_buffer *gui_buffer_search_by_category_name (char *category,
                                                                char *name);
extern struct t_gui_buffer *gui_buffer_search_by_number (int number);
extern struct t_gui_window *gui_buffer_find_window (struct t_gui_buffer *buffer);
extern int gui_buffer_is_scrolled (struct t_gui_buffer *buffer);
extern int gui_buffer_match_category_name (struct t_gui_buffer *buffer,
                                           char *mask, int case_sensitive);
extern struct t_gui_buffer *gui_buffer_get_dcc (struct t_gui_window *window);
extern void gui_buffer_clear (struct t_gui_buffer *buffer);
extern void gui_buffer_clear_all ();
extern void gui_buffer_close (struct t_gui_buffer *buffer,
                              int switch_to_another);
extern void gui_buffer_switch_previous (struct t_gui_window *window);
extern void gui_buffer_switch_next (struct t_gui_window *window);
extern void gui_buffer_switch_dcc (struct t_gui_window *window);
extern void gui_buffer_switch_by_number (struct t_gui_window *window,
                                         int number);
extern void gui_buffer_move_to_number (struct t_gui_buffer *buffer, int number);
extern void gui_buffer_dump_hexa (struct t_gui_buffer *buffer);
extern void gui_buffer_print_log ();

#endif /* gui-buffer.h */
