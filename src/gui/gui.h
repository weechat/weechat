/*
 * Copyright (c) 2003-2005 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef __WEECHAT_GUI_H
#define __WEECHAT_GUI_H 1

#include "../common/completion.h"
#include "../common/history.h"

#define INPUT_BUFFER_BLOCK_SIZE 256

#define NUM_COLORS                      56
#define COLOR_WIN_TITLE                 1
#define COLOR_WIN_CHAT                  2
#define COLOR_WIN_CHAT_TIME             3
#define COLOR_WIN_CHAT_TIME_SEP         4
#define COLOR_WIN_CHAT_PREFIX1          5
#define COLOR_WIN_CHAT_PREFIX2          6
#define COLOR_WIN_CHAT_JOIN             7
#define COLOR_WIN_CHAT_PART             8
#define COLOR_WIN_CHAT_NICK             9
#define COLOR_WIN_CHAT_HOST             10
#define COLOR_WIN_CHAT_CHANNEL          11
#define COLOR_WIN_CHAT_DARK             12
#define COLOR_WIN_CHAT_HIGHLIGHT        13
#define COLOR_WIN_STATUS                14
#define COLOR_WIN_STATUS_DELIMITERS     15
#define COLOR_WIN_STATUS_CHANNEL        16
#define COLOR_WIN_STATUS_DATA_MSG       17
#define COLOR_WIN_STATUS_DATA_PRIVATE   18
#define COLOR_WIN_STATUS_DATA_HIGHLIGHT 19
#define COLOR_WIN_STATUS_DATA_OTHER     20
#define COLOR_WIN_STATUS_MORE           21
#define COLOR_WIN_INFOBAR               22
#define COLOR_WIN_INFOBAR_DELIMITERS    23
#define COLOR_WIN_INFOBAR_HIGHLIGHT     24
#define COLOR_WIN_INPUT                 25
#define COLOR_WIN_INPUT_CHANNEL         26
#define COLOR_WIN_INPUT_NICK            27
#define COLOR_WIN_INPUT_DELIMITERS      28
#define COLOR_WIN_NICK                  29
#define COLOR_WIN_NICK_AWAY             30
#define COLOR_WIN_NICK_CHANOWNER        31
#define COLOR_WIN_NICK_CHANADMIN        32
#define COLOR_WIN_NICK_OP               33
#define COLOR_WIN_NICK_HALFOP           34
#define COLOR_WIN_NICK_VOICE            35
#define COLOR_WIN_NICK_MORE             36
#define COLOR_WIN_NICK_SEP              37
#define COLOR_WIN_NICK_SELF             38
#define COLOR_WIN_NICK_PRIVATE          39
#define COLOR_WIN_NICK_FIRST            40
#define COLOR_WIN_NICK_LAST             49
#define COLOR_WIN_NICK_NUMBER           (COLOR_WIN_NICK_LAST - COLOR_WIN_NICK_FIRST + 1)
#define COLOR_DCC_SELECTED              50
#define COLOR_DCC_WAITING               51
#define COLOR_DCC_CONNECTING            52
#define COLOR_DCC_ACTIVE                53
#define COLOR_DCC_DONE                  54
#define COLOR_DCC_FAILED                55
#define COLOR_DCC_ABORTED               56

#define SERVER(buffer)  ((t_irc_server *)(buffer->server))
#define CHANNEL(buffer) ((t_irc_channel *)(buffer->channel))

#define BUFFER_IS_SERVER(buffer)  (SERVER(buffer) && !CHANNEL(buffer))
#define BUFFER_IS_CHANNEL(buffer) (CHANNEL(buffer) && (CHANNEL(buffer)->type == CHAT_CHANNEL))
#define BUFFER_IS_PRIVATE(buffer) (CHANNEL(buffer) && (CHANNEL(buffer)->type == CHAT_PRIVATE))

#define MSG_TYPE_TIME      1
#define MSG_TYPE_PREFIX    2
#define MSG_TYPE_NICK      4
#define MSG_TYPE_INFO      8
#define MSG_TYPE_MSG       16
#define MSG_TYPE_HIGHLIGHT 32
#define MSG_TYPE_NOLOG     64

#define gui_printf_color(buffer, color, fmt, argz...) \
    gui_printf_type_color(buffer, MSG_TYPE_INFO, color, fmt, ##argz)

#define gui_printf_type(buffer, type, fmt, argz...) \
    gui_printf_type_color(buffer, type, -1, fmt, ##argz)

#define gui_printf(buffer, fmt, argz...) \
    gui_printf_type_color(buffer, MSG_TYPE_INFO, -1, fmt, ##argz)

#define gui_printf_nolog(buffer, fmt, argz...) \
    gui_printf_type_color(buffer, MSG_TYPE_INFO | MSG_TYPE_NOLOG, -1, fmt, ##argz)

#define NOTIFY_LEVEL_MIN        0
#define NOTIFY_LEVEL_MAX        3
#define NOTIFY_LEVEL_DEFAULT    NOTIFY_LEVEL_MAX

#define KEY_SHOW_MODE_DISPLAY   1
#define KEY_SHOW_MODE_BIND      2

typedef struct t_gui_message t_gui_message;

struct t_gui_message
{
    int type;                       /* type of message (time, nick, other)  */
    int color;                      /* color of message                     */
    char *message;                  /* message content                      */
    t_gui_message *prev_message;    /* link to previous message for line    */
    t_gui_message *next_message;    /* link to next message for line        */
};

typedef struct t_gui_line t_gui_line;

struct t_gui_line
{
    int length;                     /* length of the line (in char)         */
    int length_align;               /* alignment length (time or time/nick) */
    int log_write;                  /* = 1 if line will be written to log   */
    int line_with_message;          /* line contains a message from a user? */
    int line_with_highlight;        /* line contains highlight              */
    t_gui_message *messages;        /* messages for the line                */
    t_gui_message *last_message;    /* last message of the line             */
    t_gui_line *prev_line;          /* link to previous line                */
    t_gui_line *next_line;          /* link to next line                    */
};

typedef struct t_gui_color t_gui_color;

struct t_gui_color
{
    char *name;
    int color;
};

typedef struct t_gui_infobar t_gui_infobar;

struct t_gui_infobar
{
    int color;                      /* text color                           */
    char *text;                     /* infobar text                         */
    int remaining_time;             /* delay (sec) before erasing this text */
                                    /* if < 0, text is never erased (except */
                                    /* by user action to erase it)          */
    t_gui_infobar *next_infobar;    /* next message for infobar             */
};

typedef struct t_gui_buffer t_gui_buffer;

struct t_gui_buffer
{
    int num_displayed;              /* number of windows displaying buffer  */
    
    int number;                     /* buffer number (for jump/switch)      */
    
    /* server/channel */
    void *server;                   /* buffer's server                      */
    void *channel;                  /* buffer's channel                     */
    int dcc;                        /* buffer is dcc status                 */
    
    /* chat content (lines, line is composed by many messages) */
    t_gui_line *lines;              /* lines of chat window                 */
    t_gui_line *last_line;          /* last line of chat window             */
    int num_lines;                  /* number of lines in the window        */
    int line_complete;              /* current line complete ? (\n ending)  */
    
    /* notify level: when activity should be displayed? default: 3 (always) */
    int notify_level;               /* 0 = never                            */
                                    /* 1 = highlight only                   */
                                    /* 2 = highlight + message              */
                                    /* 3 = highlight + message + join/part  */
    
    /* file to save buffer content */
    char *log_filename;             /* filename for saving buffer content   */
    FILE *log_file;                 /* for logging buffer to file           */
    
    /* inupt buffer */
    int has_input;                  /* = 1 if buffer has input (DCC has not)*/
    char *input_buffer;             /* input buffer                         */
    int input_buffer_alloc;         /* input buffer: allocated size in mem  */
    int input_buffer_size;          /* buffer size (user input length)      */
    int input_buffer_pos;           /* position into buffer                 */
    int input_buffer_1st_display;   /* first char displayed on screen       */
    
    /* completion */
    t_completion completion;        /* for cmds/nicks completion            */
    
    /* history */
    t_history *history;             /* commands history                     */
    t_history *last_history;        /* last command in history              */
    t_history *ptr_history;         /* current command in history           */
    int num_history;                /* number of commands in history        */
    
    /* channel buffer before jumping to next server */
    t_gui_buffer *old_channel_buffer; /* only used for server buffer        */
    
    /* link to previous/next buffer */
    t_gui_buffer *prev_buffer;      /* link to previous buffer              */
    t_gui_buffer *next_buffer;      /* link to next buffer                  */
};

typedef struct t_gui_window t_gui_window;

struct t_gui_window
{
    /* global position & size */
    int win_x, win_y;               /* position of window                   */
    int win_width, win_height;      /* window geometry                      */
    
    /* chat window settings */
    int win_chat_x, win_chat_y;     /* chat window position                 */
    int win_chat_width;             /* width of chat window                 */
    int win_chat_height;            /* height of chat window                */
    int win_chat_cursor_x;          /* position of cursor in chat window    */
    int win_chat_cursor_y;          /* position of cursor in chat window    */
    
    /* nicklist window settings */
    int win_nick_x, win_nick_y;     /* nick window position                 */
    int win_nick_width;             /* width of nick window                 */
    int win_nick_height;            /* height of nick window                */
    int win_nick_start;             /* # of 1st nick for display (scroll)   */
    
    /* input window settings */
    int win_input_x;                /* position of cursor in input window   */
    
    /* windows for Curses GUI */
    void *win_title;                /* title window                         */
    void *win_chat;                 /* chat window (example: channel)       */
    void *win_nick;                 /* nick window                          */
    void *win_status;               /* status window                        */
    void *win_infobar;              /* info bar window                      */
    void *win_input;                /* input window                         */
    void *win_separator;            /* separation between 2 splited (V) win */
    
    /* windows for Gtk GUI */
    void *textview_chat;            /* textview widget for chat             */
    void *textbuffer_chat;          /* textbuffer widget for chat           */
    void *texttag_chat;             /* texttag widget for chat              */
    void *textview_nicklist;        /* textview widget for nicklist         */
    void *textbuffer_nicklist;      /* textbuffer widget for nicklist       */
    
    /* windows for Qt GUI */
    /* TODO: declare Qt window */
    
    /* DCC */
    void *dcc_first;                /* first dcc displayed                  */
    void *dcc_selected;             /* selected dcc                         */
    void *dcc_last_displayed;       /* last dcc displayed (for scroll)      */
    
    t_gui_buffer *buffer;           /* buffer currently displayed in window */
    
    int first_line_displayed;       /* = 1 if first line is displayed       */
    t_gui_line *start_line;         /* pointer to line if scrolling         */
    int start_line_pos;             /* position in first line displayed     */
    
    t_gui_window *prev_window;      /* link to previous window              */
    t_gui_window *next_window;      /* link to next window                  */
};

typedef struct t_gui_key t_gui_key;

struct t_gui_key
{
    char *key;                      /* key combo (ex: a, ^W, ^W^C, meta-a)  */
    char *command;                  /* associated command (may be NULL)     */
    void (*function)(t_gui_window *);
                                    /* associated function (if cmd is NULL) */
    t_gui_key *prev_key;            /* link to previous key                 */
    t_gui_key *next_key;            /* link to next key                     */
};

typedef struct t_gui_key_function t_gui_key_function;

struct t_gui_key_function
{
    char *function_name;            /* name of function                     */
    void (*function)();             /* associated function                  */
    char *description;              /* description of function              */
};

/* variables */

extern int gui_init_ok;
extern int gui_ok;
extern int gui_add_hotlist;
extern t_gui_window *gui_windows;
extern t_gui_window *last_gui_window;
extern t_gui_window *gui_current_window;
extern t_gui_buffer *gui_buffers;
extern t_gui_buffer *last_gui_buffer;
extern t_gui_buffer *buffer_before_dcc;
extern t_gui_infobar *gui_infobar;
extern t_gui_key *gui_keys;
extern t_gui_key *last_gui_key;
extern t_gui_key_function gui_key_functions[];
extern char gui_key_buffer[128];
extern int gui_key_grab;
extern int gui_key_grab_count;
extern char *gui_input_clipboard;

/* GUI independent functions: windows & buffers */

extern t_gui_window *gui_window_new (int, int, int, int);
extern t_gui_buffer *gui_buffer_new (t_gui_window *, void *, void *, int, int);
extern void gui_buffer_clear (t_gui_buffer *);
extern void gui_buffer_clear_all ();
extern void gui_infobar_printf (int, int, char *, ...);
extern void gui_window_free (t_gui_window *);
extern void gui_infobar_remove ();
extern void gui_buffer_free (t_gui_buffer *, int);
extern t_gui_line *gui_new_line (t_gui_buffer *);
extern t_gui_message *gui_new_message (t_gui_buffer *);
extern void gui_input_clipboard_copy (char *, int);
extern void gui_input_clipboard_paste (t_gui_window *);
extern void gui_input_insert_string (t_gui_window *, char *, int);
extern void gui_input_insert_char (t_gui_window *, int);
extern void gui_input_return (t_gui_window *);
extern void gui_input_tab (t_gui_window *);
extern void gui_input_backspace (t_gui_window *);
extern void gui_input_delete (t_gui_window *);
extern void gui_input_delete_previous_word (t_gui_window *);
extern void gui_input_delete_next_word (t_gui_window *);
extern void gui_input_delete_begin_of_line (t_gui_window *);
extern void gui_input_delete_end_of_line (t_gui_window *);
extern void gui_input_delete_line (t_gui_window *);
extern void gui_input_transpose_chars (t_gui_window *);
extern void gui_input_home (t_gui_window *);
extern void gui_input_end (t_gui_window *);
extern void gui_input_left (t_gui_window *);
extern void gui_input_previous_word (t_gui_window *);
extern void gui_input_right (t_gui_window *);
extern void gui_input_next_word (t_gui_window *);
extern void gui_input_up (t_gui_window *);
extern void gui_input_up_global (t_gui_window *);
extern void gui_input_down (t_gui_window *);
extern void gui_input_down_global (t_gui_window *);
extern void gui_input_jump_smart (t_gui_window *);
extern void gui_input_jump_dcc (t_gui_window *);
extern void gui_input_jump_last_buffer (t_gui_window *);
extern void gui_input_jump_server (t_gui_window *);
extern void gui_input_jump_next_server (t_gui_window *);
extern void gui_input_hotlist_clear (t_gui_window *);
extern void gui_input_infobar_clear (t_gui_window *);
extern void gui_input_grab_key (t_gui_window *);
extern void gui_switch_to_previous_buffer (t_gui_window *);
extern void gui_switch_to_next_buffer (t_gui_window *);
extern void gui_switch_to_previous_window (t_gui_window *);
extern void gui_switch_to_next_window (t_gui_window *);
extern void gui_switch_to_dcc_buffer (t_gui_window *);
extern t_gui_buffer *gui_switch_to_buffer_by_number (t_gui_window *, int);
extern void gui_move_buffer_to_number (t_gui_window *, int);
extern void gui_window_print_log (t_gui_window *);
extern void gui_buffer_print_log (t_gui_buffer *);

/* GUI independent functions: keys */

extern void gui_key_init ();
extern void gui_key_init_grab ();
extern char *gui_key_get_internal_code (char *);
extern char *gui_key_get_expanded_name (char *);
extern void *gui_key_function_search_by_name (char *);
extern char *gui_key_function_search_by_ptr (void *);
extern t_gui_key *gui_key_bind (char *, char *);
extern int gui_key_unbind (char *);
extern int gui_key_pressed (char *);
extern void gui_key_free (t_gui_key *);
extern void gui_key_free_all ();

/* GUI dependant functions: display */

extern int gui_assign_color (int *, char *);
extern int gui_get_color_by_name (char *);
extern char *gui_get_color_by_value (int);
extern int gui_buffer_has_nicklist (t_gui_buffer *);
extern void gui_calculate_pos_size (t_gui_window *);
extern void gui_draw_buffer_title (t_gui_buffer *, int);
extern void gui_draw_buffer_chat (t_gui_buffer *, int);
extern void gui_draw_buffer_nick (t_gui_buffer *, int);
extern void gui_draw_buffer_status (t_gui_buffer *, int);
extern void gui_draw_buffer_infobar_time (t_gui_buffer *);
extern void gui_draw_buffer_infobar (t_gui_buffer *, int);
extern void gui_draw_buffer_input (t_gui_buffer *, int);
extern void gui_redraw_buffer (t_gui_buffer *);
extern void gui_switch_to_buffer (t_gui_window *, t_gui_buffer *);
extern t_gui_buffer *gui_get_dcc_buffer (t_gui_window *);
extern void gui_input_page_up (t_gui_window *);
extern void gui_input_page_down (t_gui_window *);
extern void gui_input_nick_beginning (t_gui_window *);
extern void gui_input_nick_end (t_gui_window *);
extern void gui_input_nick_page_up (t_gui_window *);
extern void gui_input_nick_page_down (t_gui_window *);
extern void gui_curses_resize_handler ();
extern void gui_window_init_subwindows (t_gui_window *);
extern void gui_window_split_horiz (t_gui_window *);
extern void gui_window_split_vertic (t_gui_window *);
extern int gui_window_merge_up (t_gui_window *);
extern int gui_window_merge_down (t_gui_window *);
extern int gui_window_merge_left (t_gui_window *);
extern int gui_window_merge_right (t_gui_window *);
extern void gui_window_merge_auto (t_gui_window *);
extern void gui_window_merge_all (t_gui_window *);
extern void gui_pre_init (int *, char **[]);
extern void gui_init_colors ();
extern void gui_set_window_title ();
extern void gui_init ();
extern void gui_end ();
extern void gui_printf_type_color (/*@null@*/ t_gui_buffer *, int, int, char *, ...);
extern void gui_input_default_key_bindings ();
extern void gui_main_loop ();

#endif /* gui.h */
