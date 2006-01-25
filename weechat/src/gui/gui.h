/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef __WEECHAT_GUI_H
#define __WEECHAT_GUI_H 1

#include "../common/completion.h"
#include "../common/history.h"

#define INPUT_BUFFER_BLOCK_SIZE 256

/* shift ncurses colors for compatibility with colors
   in IRC messages (same as other IRC clients) */

#define WEECHAT_COLOR_BLACK   COLOR_BLACK
#define WEECHAT_COLOR_RED     COLOR_BLUE
#define WEECHAT_COLOR_GREEN   COLOR_GREEN
#define WEECHAT_COLOR_YELLOW  COLOR_CYAN
#define WEECHAT_COLOR_BLUE    COLOR_RED
#define WEECHAT_COLOR_MAGENTA COLOR_MAGENTA
#define WEECHAT_COLOR_CYAN    COLOR_YELLOW
#define WEECHAT_COLOR_WHITE   COLOR_WHITE

#define COLOR_WIN_NICK_NUMBER 10

typedef enum t_weechat_color t_weechat_color;

enum t_weechat_color
{
    COLOR_WIN_SEPARATOR = 0,
    COLOR_WIN_TITLE,
    COLOR_WIN_CHAT,
    COLOR_WIN_CHAT_TIME,
    COLOR_WIN_CHAT_TIME_SEP,
    COLOR_WIN_CHAT_PREFIX1,
    COLOR_WIN_CHAT_PREFIX2,
    COLOR_WIN_CHAT_SERVER,
    COLOR_WIN_CHAT_JOIN,
    COLOR_WIN_CHAT_PART,
    COLOR_WIN_CHAT_NICK,
    COLOR_WIN_CHAT_HOST,
    COLOR_WIN_CHAT_CHANNEL,
    COLOR_WIN_CHAT_DARK,
    COLOR_WIN_CHAT_HIGHLIGHT,
    COLOR_WIN_CHAT_READ_MARKER,
    COLOR_WIN_STATUS,
    COLOR_WIN_STATUS_DELIMITERS,
    COLOR_WIN_STATUS_CHANNEL,
    COLOR_WIN_STATUS_DATA_MSG,
    COLOR_WIN_STATUS_DATA_PRIVATE,
    COLOR_WIN_STATUS_DATA_HIGHLIGHT,
    COLOR_WIN_STATUS_DATA_OTHER,
    COLOR_WIN_STATUS_MORE,
    COLOR_WIN_INFOBAR,
    COLOR_WIN_INFOBAR_DELIMITERS,
    COLOR_WIN_INFOBAR_HIGHLIGHT,
    COLOR_WIN_INPUT,
    COLOR_WIN_INPUT_CHANNEL,
    COLOR_WIN_INPUT_NICK,
    COLOR_WIN_INPUT_DELIMITERS,
    COLOR_WIN_NICK,
    COLOR_WIN_NICK_AWAY,
    COLOR_WIN_NICK_CHANOWNER,
    COLOR_WIN_NICK_CHANADMIN,
    COLOR_WIN_NICK_OP,
    COLOR_WIN_NICK_HALFOP,
    COLOR_WIN_NICK_VOICE,
    COLOR_WIN_NICK_MORE,
    COLOR_WIN_NICK_SEP,
    COLOR_WIN_NICK_SELF,
    COLOR_WIN_NICK_PRIVATE,
    COLOR_WIN_NICK_1,
    COLOR_WIN_NICK_2,
    COLOR_WIN_NICK_3,
    COLOR_WIN_NICK_4,
    COLOR_WIN_NICK_5,
    COLOR_WIN_NICK_6,
    COLOR_WIN_NICK_7,
    COLOR_WIN_NICK_8,
    COLOR_WIN_NICK_9,
    COLOR_WIN_NICK_10,
    COLOR_DCC_SELECTED,
    COLOR_DCC_WAITING,
    COLOR_DCC_CONNECTING,
    COLOR_DCC_ACTIVE,
    COLOR_DCC_DONE,
    COLOR_DCC_FAILED,
    COLOR_DCC_ABORTED,
    NUM_COLORS
};

/* attributes in IRC messages for color & style (bold, ..) */

#define GUI_ATTR_BOLD_CHAR        '\x02'
#define GUI_ATTR_BOLD_STR         "\x02"
#define GUI_ATTR_COLOR_CHAR       '\x03'
#define GUI_ATTR_COLOR_STR        "\x03"
#define GUI_ATTR_RESET_CHAR       '\x0F'
#define GUI_ATTR_RESET_STR        "\x0F"
#define GUI_ATTR_FIXED_CHAR       '\x11'
#define GUI_ATTR_FIXED_STR        "\x11"
#define GUI_ATTR_REVERSE_CHAR     '\x12'
#define GUI_ATTR_REVERSE_STR      "\x12"
#define GUI_ATTR_REVERSE2_CHAR    '\x16'
#define GUI_ATTR_REVERSE2_STR     "\x16"
#define GUI_ATTR_ITALIC_CHAR      '\x1D'
#define GUI_ATTR_ITALIC_STR       "\x1D"
#define GUI_ATTR_UNDERLINE_CHAR   '\x1F'
#define GUI_ATTR_UNDERLINE_STR    "\x1F"

/* WeeChat internal attributes (should never be in IRC messages) */

#define GUI_ATTR_WEECHAT_COLOR_CHAR  '\x19'
#define GUI_ATTR_WEECHAT_COLOR_STR   "\x19"
#define GUI_ATTR_WEECHAT_SET_CHAR    '\x1A'
#define GUI_ATTR_WEECHAT_SET_STR     "\x1A"
#define GUI_ATTR_WEECHAT_REMOVE_CHAR '\x1B'
#define GUI_ATTR_WEECHAT_REMOVE_STR  "\x1B"

#define GUI_COLOR(color) ((gui_color[color]) ? gui_color[color]->string : "")
#define GUI_NO_COLOR     GUI_ATTR_RESET_STR

#define SERVER(buffer)  ((t_irc_server *)(buffer->server))
#define CHANNEL(buffer) ((t_irc_channel *)(buffer->channel))

#define BUFFER_IS_SERVER(buffer)  ((SERVER(buffer) || (buffer->all_servers)) && !CHANNEL(buffer))
#define BUFFER_IS_CHANNEL(buffer) (CHANNEL(buffer) && (CHANNEL(buffer)->type == CHANNEL_TYPE_CHANNEL))
#define BUFFER_IS_PRIVATE(buffer) (CHANNEL(buffer) && (CHANNEL(buffer)->type == CHANNEL_TYPE_PRIVATE))

#define BUFFER_HAS_NICKLIST(buffer) (BUFFER_IS_CHANNEL(buffer))

#define MSG_TYPE_TIME      1
#define MSG_TYPE_PREFIX    2
#define MSG_TYPE_NICK      4
#define MSG_TYPE_INFO      8
#define MSG_TYPE_MSG       16
#define MSG_TYPE_HIGHLIGHT 32
#define MSG_TYPE_NOLOG     64

#define gui_printf(buffer, fmt, argz...) \
    gui_printf_internal(buffer, 1, MSG_TYPE_INFO, fmt, ##argz)

#define gui_printf_type(buffer, type, fmt, argz...) \
    gui_printf_internal(buffer, 1, type, fmt, ##argz)

#define gui_printf_nolog(buffer, fmt, argz...) \
    gui_printf_internal(buffer, 1, MSG_TYPE_INFO | MSG_TYPE_NOLOG, fmt, ##argz)

#define gui_printf_nolog_notime(buffer, fmt, argz...) \
    gui_printf_internal(buffer, 0, MSG_TYPE_NOLOG, fmt, ##argz)

#define WINDOW_MIN_WIDTH        10
#define WINDOW_MIN_HEIGHT       5

#define NOTIFY_LEVEL_MIN        0
#define NOTIFY_LEVEL_MAX        3
#define NOTIFY_LEVEL_DEFAULT    NOTIFY_LEVEL_MAX

#define KEY_SHOW_MODE_DISPLAY   1
#define KEY_SHOW_MODE_BIND      2

typedef struct t_gui_color t_gui_color;

struct t_gui_color
{
    int foreground;                 /* foreground color                     */
    int background;                 /* background color                     */
    int attributes;                 /* attributes (bold, ..)                */
    char *string;                   /* WeeChat color: "\x19??", ?? is #color*/
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

typedef struct t_gui_line t_gui_line;

struct t_gui_line
{
    int length;                     /* length of the line (in char)         */
    int length_align;               /* alignment length (time or time/nick) */
    int log_write;                  /* = 1 if line will be written to log   */
    int line_with_message;          /* line contains a message from a user? */
    int line_with_highlight;        /* line contains highlight              */
    char *data;                     /* line content                         */
    int ofs_after_date;             /* offset to first char after date      */
    t_gui_line *prev_line;          /* link to previous line                */
    t_gui_line *next_line;          /* link to next line                    */
};

typedef struct t_gui_buffer t_gui_buffer;

struct t_gui_buffer
{
    int num_displayed;              /* number of windows displaying buffer  */
    
    int number;                     /* buffer number (for jump/switch)      */
    
    /* server/channel */
    void *server;                   /* buffer's server                      */
    int all_servers;                /* =1 if all servers are displayed here */
    void *channel;                  /* buffer's channel                     */
    int dcc;                        /* buffer is dcc status                 */
    
    /* chat content (lines, line is composed by many messages) */
    t_gui_line *lines;              /* lines of chat window                 */
    t_gui_line *last_line;          /* last line of chat window             */
    t_gui_line *last_read_line;     /* last read line before jump           */
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
    int input_buffer_size;          /* buffer size in bytes                 */
    int input_buffer_length;        /* number of chars in buffer            */
    int input_buffer_pos;           /* position into buffer                 */
    int input_buffer_1st_display;   /* first char displayed on screen       */
    
    /* completion */
    t_completion completion;        /* for cmds/nicks completion            */
    
    /* history */
    t_history *history;             /* commands history                     */
    t_history *last_history;        /* last command in history              */
    t_history *ptr_history;         /* current command in history           */
    int num_history;                /* number of commands in history        */
    
    /* link to previous/next buffer */
    t_gui_buffer *prev_buffer;      /* link to previous buffer              */
    t_gui_buffer *next_buffer;      /* link to next buffer                  */
};

typedef struct t_gui_window_tree t_gui_window_tree;
typedef struct t_gui_window t_gui_window;

struct t_gui_window
{
    /* global position & size */
    int win_x, win_y;               /* position of window                   */
    int win_width, win_height;      /* window geometry                      */
    int win_width_pct;              /* % of width (compared to term size)   */
    int win_height_pct;             /* % of height (compared to term size)  */
    
    int new_x, new_y;               /* used for computing new position      */
    int new_width, new_height;      /* used for computing new size          */
    
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
    
    int current_style_fg;;          /* current color used for foreground    */
    int current_style_bg;;          /* current color used for background    */
    int current_style_attr;         /* current attributes (bold, ..)        */
    int current_color_attr;         /* attr sum of last color(s) displayed  */
    
    /* DCC */
    void *dcc_first;                /* first dcc displayed                  */
    void *dcc_selected;             /* selected dcc                         */
    void *dcc_last_displayed;       /* last dcc displayed (for scroll)      */
    
    t_gui_buffer *buffer;           /* buffer currently displayed in window */
    
    int first_line_displayed;       /* = 1 if first line is displayed       */
    t_gui_line *start_line;         /* pointer to line if scrolling         */
    int start_line_pos;             /* position in first line displayed     */
    int scroll;                     /* = 1 if "MORE" should be displayed    */
    t_gui_window_tree *ptr_tree;    /* pointer to leaf in windows tree      */
    
    t_gui_window *prev_window;      /* link to previous window              */
    t_gui_window *next_window;      /* link to next window                  */
};

struct t_gui_window_tree
{
    t_gui_window_tree *parent_node; /* pointer to parent node               */
    
    /* node info */
    int split_horiz;                /* 1 if horizontal, 0 if vertical       */
    int split_pct;                  /* % of split size (represents child1)  */
    t_gui_window_tree *child1;      /* first child, NULL if a leaf          */
    t_gui_window_tree *child2;      /* second child, NULL if a leaf         */
    
    /* leaf info */
    t_gui_window *window;           /* pointer to window, NULL if a node    */
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
extern t_gui_window_tree *gui_windows_tree;
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

extern t_gui_color *gui_color[NUM_COLORS];

/* GUI independent functions: windows & buffers */

extern int gui_window_tree_init (t_gui_window *);
extern void gui_window_tree_node_to_leaf (t_gui_window_tree *, t_gui_window *);
extern void gui_window_tree_free (t_gui_window_tree **);
extern t_gui_window *gui_window_new (t_gui_window *, int, int, int, int, int, int);
extern t_gui_buffer *gui_buffer_search (char *, char *);
extern t_gui_window *gui_buffer_find_window (t_gui_buffer *);
extern t_gui_buffer *gui_buffer_new (t_gui_window *, void *, void *, int, int);
extern void gui_buffer_clear (t_gui_buffer *);
extern void gui_buffer_clear_all ();
extern void gui_infobar_printf (int, int, char *, ...);
extern void gui_infobar_printf_from_buffer (t_gui_buffer *, int, int, char *, char *, ...);
extern void gui_window_free (t_gui_window *);
extern void gui_infobar_remove ();
extern void gui_buffer_free (t_gui_buffer *, int);
extern t_gui_line *gui_line_new (t_gui_buffer *);
extern int gui_word_strlen (t_gui_window *, char *);
extern int gui_word_real_pos (t_gui_window *, char *, int);
extern void gui_printf_internal (t_gui_buffer *, int, int, char *, ...);
extern void gui_optimize_input_buffer_size (t_gui_buffer *);
extern void gui_exec_action_dcc (t_gui_window *, char *);
extern int gui_insert_string_input (t_gui_window *, char *, int);
extern void gui_merge_servers (t_gui_window *);
extern void gui_split_server (t_gui_window *);
extern void gui_window_switch_server (t_gui_window *);
extern void gui_buffer_switch_previous (t_gui_window *);
extern void gui_buffer_switch_next (t_gui_window *);
extern void gui_window_switch_previous (t_gui_window *);
extern void gui_window_switch_next (t_gui_window *);
extern void gui_window_switch_by_buffer (t_gui_window *, int);
extern void gui_buffer_switch_dcc (t_gui_window *);
extern t_gui_buffer *gui_buffer_switch_by_number (t_gui_window *, int);
extern void gui_buffer_move_to_number (t_gui_buffer *, int);
extern void gui_window_print_log (t_gui_window *);
extern void gui_buffer_print_log (t_gui_buffer *);

/* GUI independent functions: actions */

extern void gui_action_clipboard_copy (char *, int);
extern void gui_action_clipboard_paste (t_gui_window *);
extern void gui_action_return (t_gui_window *);
extern void gui_action_tab (t_gui_window *);
extern void gui_action_backspace (t_gui_window *);
extern void gui_action_delete (t_gui_window *);
extern void gui_action_delete_previous_word (t_gui_window *);
extern void gui_action_delete_next_word (t_gui_window *);
extern void gui_action_delete_begin_of_line (t_gui_window *);
extern void gui_action_delete_end_of_line (t_gui_window *);
extern void gui_action_delete_line (t_gui_window *);
extern void gui_action_transpose_chars (t_gui_window *);
extern void gui_action_home (t_gui_window *);
extern void gui_action_end (t_gui_window *);
extern void gui_action_left (t_gui_window *);
extern void gui_action_previous_word (t_gui_window *);
extern void gui_action_right (t_gui_window *);
extern void gui_action_next_word (t_gui_window *);
extern void gui_action_up (t_gui_window *);
extern void gui_action_up_global (t_gui_window *);
extern void gui_action_down (t_gui_window *);
extern void gui_action_down_global (t_gui_window *);
extern void gui_action_page_up (t_gui_window *);
extern void gui_action_page_down (t_gui_window *);
extern void gui_action_scroll_up (t_gui_window *);
extern void gui_action_scroll_down (t_gui_window *);
extern void gui_action_nick_beginning (t_gui_window *);
extern void gui_action_nick_end (t_gui_window *);
extern void gui_action_nick_page_up (t_gui_window *);
extern void gui_action_nick_page_down (t_gui_window *);
extern void gui_action_jump_smart (t_gui_window *);
extern void gui_action_jump_dcc (t_gui_window *);
extern void gui_action_jump_last_buffer (t_gui_window *);
extern void gui_action_jump_server (t_gui_window *);
extern void gui_action_jump_next_server (t_gui_window *);
extern void gui_action_switch_server (t_gui_window *);
extern void gui_action_scroll_previous_highlight (t_gui_window *);
extern void gui_action_scroll_next_highlight (t_gui_window *);
extern void gui_action_scroll_unread (t_gui_window *);
extern void gui_action_hotlist_clear (t_gui_window *);
extern void gui_action_infobar_clear (t_gui_window *);
extern void gui_action_refresh_screen ();
extern void gui_action_grab_key (t_gui_window *);

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
extern char *gui_get_color_name (int);
extern unsigned char *gui_color_decode (unsigned char *, int);
extern unsigned char *gui_color_decode_for_user_entry (unsigned char *);
extern unsigned char *gui_color_encode (unsigned char *);
extern int gui_buffer_has_nicklist (t_gui_buffer *);
extern void gui_calculate_pos_size (t_gui_window *);
extern void gui_draw_buffer_title (t_gui_buffer *, int);
extern char *gui_word_get_next_char (t_gui_window *, unsigned char *, int);
extern void gui_draw_buffer_chat (t_gui_buffer *, int);
extern void gui_draw_buffer_nick (t_gui_buffer *, int);
extern void gui_draw_buffer_status (t_gui_buffer *, int);
extern void gui_draw_buffer_infobar_time (t_gui_buffer *);
extern void gui_draw_buffer_infobar (t_gui_buffer *, int);
extern void gui_draw_buffer_input (t_gui_buffer *, int);
extern void gui_redraw_buffer (t_gui_buffer *);
extern void gui_switch_to_buffer (t_gui_window *, t_gui_buffer *);
extern t_gui_buffer *gui_get_dcc_buffer (t_gui_window *);
extern void gui_window_page_up (t_gui_window *);
extern void gui_window_page_down (t_gui_window *);
extern void gui_window_scroll_up (t_gui_window *);
extern void gui_window_scroll_down (t_gui_window *);
extern void gui_window_nick_beginning (t_gui_window *);
extern void gui_window_nick_end (t_gui_window *);
extern void gui_window_nick_page_up (t_gui_window *);
extern void gui_window_nick_page_down (t_gui_window *);
extern void gui_window_init_subwindows (t_gui_window *);
extern void gui_refresh_windows ();
extern void gui_window_split_horiz (t_gui_window *, int);
extern void gui_window_split_vertic (t_gui_window *, int);
extern void gui_window_resize (t_gui_window *, int);
extern int gui_window_merge (t_gui_window *);
extern void gui_window_merge_all (t_gui_window *);
extern void gui_window_switch_up (t_gui_window *);
extern void gui_window_switch_down (t_gui_window *);
extern void gui_window_switch_left (t_gui_window *);
extern void gui_window_switch_right (t_gui_window *);
extern void gui_refresh_screen ();
extern void gui_pre_init (int *, char **[]);
extern void gui_init_color_pairs ();
extern void gui_rebuild_weechat_colors ();
extern void gui_set_window_title ();
extern void gui_reset_window_title ();
extern void gui_init ();
extern void gui_end ();
extern void gui_input_default_key_bindings ();
extern void gui_main_loop ();

#endif /* gui.h */
