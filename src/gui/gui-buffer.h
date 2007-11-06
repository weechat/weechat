/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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
    time_t date;                       /* date/time of line (may be past)   */
    time_t date_printed;               /* date/time when weechat print it   */
    char *str_time;                    /* time string (for display)         */
    char *prefix;                      /* prefix for line (may be NULL)     */
    int prefix_length;                 /* prefix length (on screen)         */
    char *message;                     /* line content (after prefix)       */
    struct t_gui_line *prev_line;      /* link to previous line             */
    struct t_gui_line *next_line;      /* link to next line                 */
};

struct t_gui_nick
{
    char *nick;                        /* nickname                          */
    int sort_index;                    /* index to force sort               */
    int color_nick;                    /* color for nick in nicklist        */
    char prefix;                       /* prefix for nick (for admins, ..)  */
    int color_prefix;                  /* color for prefix                  */
    struct t_gui_nick *prev_nick;      /* link to previous nick in nicklist */
    struct t_gui_nick *next_nick;      /* link to next nick in nicklist     */
};

struct t_gui_buffer
{
    void *plugin;                      /* plugin which created this buffer  */
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
    
    /* logging */
    char *log_filename;                /* filename for saving content       */
    FILE *log_file;                    /* file descriptor for log           */
    
    /* buffer title */
    char *title;                       /* buffer title                      */
    
    /* chat content */
    struct t_gui_line *lines;          /* lines of chat window              */
    struct t_gui_line *last_line;      /* last line of chat window          */
    struct t_gui_line *last_read_line; /* last read line before jump        */
    int lines_count;                   /* number of lines in the buffer     */
    int prefix_max_length;             /* length for prefix align           */
    int chat_refresh_needed;           /* if refresh is needed (printf)     */
    
    /* nicklist */ 
    int nicklist;                      /* = 1 if nicklist is enabled        */
    int nick_case_sensitive;           /* nicks are case sensitive ?        */
    struct t_gui_nick *nicks;          /* pointer to nicks for nicklist     */
    struct t_gui_nick *last_nick;      /* last nick in nicklist             */
    int nick_max_length;               /* max length for a nick             */
    int nicks_count;                   /* number of nicks on buffer         */
    
    /* inupt */
    int input;                         /* = 1 if input is enabled           */
    void (*input_data_cb)(struct t_gui_buffer *, char *);
                                       /* called when user send data        */
                                       /* to this buffer                    */
    char *input_nick;                  /* self nick                         */
    char *input_buffer;                /* input buffer                      */
    char *input_buffer_color_mask;     /* color mask for input buffer       */
    int input_buffer_alloc;            /* input buffer: allocated size      */
    int input_buffer_size;             /* buffer size in bytes              */
    int input_buffer_length;           /* number of chars in buffer         */
    int input_buffer_pos;              /* position into buffer              */
    int input_buffer_1st_display;      /* first char displayed on screen    */
    
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
    
    /* link to previous/next buffer */
    struct t_gui_buffer *prev_buffer;  /* link to previous buffer           */
    struct t_gui_buffer *next_buffer;  /* link to next buffer               */
};

/* buffer variables */

extern struct t_gui_buffer *gui_buffers;
extern struct t_gui_buffer *last_gui_buffer;
extern struct t_gui_buffer *gui_previous_buffer;
extern struct t_gui_buffer *gui_buffer_before_dcc;
extern struct t_gui_buffer *gui_buffer_raw_data;
extern struct t_gui_buffer *gui_buffer_before_raw_data;

/* buffer functions */

extern struct t_gui_buffer *gui_buffer_new (void *, char *, char *);
extern int gui_buffer_valid (struct t_gui_buffer *);
extern void gui_buffer_set_category (struct t_gui_buffer *, char *);
extern void gui_buffer_set_name (struct t_gui_buffer *, char *);
extern void gui_buffer_set_log (struct t_gui_buffer *, char *);
extern void gui_buffer_set_title (struct t_gui_buffer *, char *);
extern void gui_buffer_set_nick_case_sensitive (struct t_gui_buffer *, int);
extern void gui_buffer_set_nick (struct t_gui_buffer *, char *);
extern void gui_buffer_set (struct t_gui_buffer *, char *, char *);
extern struct t_gui_buffer *gui_buffer_search_main ();
extern struct t_gui_buffer *gui_buffer_search_by_category_name (char *,
                                                                char *);
extern struct t_gui_buffer *gui_buffer_search_by_number (int);
extern struct t_gui_window *gui_buffer_find_window (struct t_gui_buffer *);
extern void gui_buffer_find_context (void *, void *,
                                     struct t_gui_window **,
                                     struct t_gui_buffer **);
extern int gui_buffer_is_scrolled (struct t_gui_buffer *);
extern struct t_gui_buffer *gui_buffer_get_dcc (struct t_gui_window *);
extern void gui_buffer_clear (struct t_gui_buffer *);
extern void gui_buffer_clear_all ();
extern void gui_buffer_free (struct t_gui_buffer *, int);
extern void gui_buffer_switch_previous (struct t_gui_window *);
extern void gui_buffer_switch_next (struct t_gui_window *);
extern void gui_buffer_switch_dcc (struct t_gui_window *);
extern void gui_buffer_switch_raw_data (struct t_gui_window *);
extern struct t_gui_buffer *gui_buffer_switch_by_number (struct t_gui_window *,
                                                         int);
extern void gui_buffer_move_to_number (struct t_gui_buffer *, int);
extern void gui_buffer_dump_hexa (struct t_gui_buffer *);
extern void gui_buffer_print_log ();

#endif /* gui-buffer.h */
