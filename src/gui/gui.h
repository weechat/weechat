/*
 * Copyright (c) 2003 by FlashCode <flashcode@flashtux.org>
 *                       Bounga <bounga@altern.org>
 *                       Xahlexx <xahlexx@tuxisland.org>
 * See README for License detail.
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

#define NUM_COLORS                  35
#define COLOR_WIN_TITLE             1
#define COLOR_WIN_CHAT              2
#define COLOR_WIN_CHAT_TIME         3
#define COLOR_WIN_CHAT_TIME_SEP     4
#define COLOR_WIN_CHAT_PREFIX1      5
#define COLOR_WIN_CHAT_PREFIX2      6
#define COLOR_WIN_CHAT_NICK         7
#define COLOR_WIN_CHAT_HOST         8
#define COLOR_WIN_CHAT_CHANNEL      9
#define COLOR_WIN_CHAT_DARK         10
#define COLOR_WIN_STATUS            11
#define COLOR_WIN_STATUS_ACTIVE     12
#define COLOR_WIN_STATUS_DATA_MSG   13
#define COLOR_WIN_STATUS_DATA_OTHER 14
#define COLOR_WIN_STATUS_MORE       15
#define COLOR_WIN_INPUT             16
#define COLOR_WIN_INPUT_CHANNEL     17
#define COLOR_WIN_INPUT_NICK        18
#define COLOR_WIN_NICK              19
#define COLOR_WIN_NICK_OP           20
#define COLOR_WIN_NICK_HALFOP       21
#define COLOR_WIN_NICK_VOICE        22
#define COLOR_WIN_NICK_SEP          23
#define COLOR_WIN_NICK_SELF         24
#define COLOR_WIN_NICK_PRIVATE      25
#define COLOR_WIN_NICK_FIRST        26
#define COLOR_WIN_NICK_LAST         35
#define COLOR_WIN_NICK_NUMBER       (COLOR_WIN_NICK_LAST - COLOR_WIN_NICK_FIRST + 1)

#define SERVER(window)  ((t_irc_server *)(window->server))
#define CHANNEL(window) ((t_irc_channel *)(window->channel))

#define WIN_IS_SERVER(window)  (SERVER(window) && !CHANNEL(window))
#define WIN_IS_CHANNEL(window) (CHANNEL(window) && (CHANNEL(window)->type == CHAT_CHANNEL))
#define WIN_IS_PRIVATE(window) (CHANNEL(window) && (CHANNEL(window)->type == CHAT_PRIVATE))

#define MSG_TYPE_TIME  0
#define MSG_TYPE_NICK  1
#define MSG_TYPE_INFO  2
#define MSG_TYPE_MSG   3

#define gui_printf_color(window, color, fmt, argz...) \
    gui_printf_color_type(window, MSG_TYPE_INFO, color, fmt, ##argz)

#define gui_printf(window, fmt, argz...) \
    gui_printf_color_type(window, MSG_TYPE_INFO, -1, fmt, ##argz)

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
    int line_with_message;          /* line contains a message from a user? */
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

typedef struct t_gui_window t_gui_window;

struct t_gui_window
{
    int is_displayed;               /* = 1 if window is displayed           */
    
    /* server/channel */
    void *server;                   /* window's server                      */
    void *channel;                  /* window's channel                     */
    
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
    int win_nick_x, win_nick_y;     /* chat window position                 */
    int win_nick_width;             /* width of chat window                 */
    int win_nick_height;            /* height of chat window                */
    
    /* windows for Curses GUI */
    void *win_title;                /* title window                         */
    void *win_chat;                 /* chat window (exemple: channel)       */
    void *win_nick;                 /* nick window                          */
    void *win_status;               /* status window                        */
    void *win_input;                /* input window                         */
    
    /* windows for Curses GUI */
    void *textview_chat;            /* textview widget for chat             */
    void *textbuffer_chat;          /* textbuffer widget for chat           */
    void *texttag_chat;             /* texttag widget for chat              */
    void *textview_nicklist;        /* textview widget for nicklist         */
    void *textbuffer_nicklist;      /* textbuffer widget for nicklist       */
    
    /* windows for Curses GUI */
    /* TODO: declare Qt window */
    
    /* chat content (lines, line is composed by many messages) */
    t_gui_line *lines;              /* lines of chat window                 */
    t_gui_line *last_line;          /* last line of chat window             */
    int first_line_displayed;       /* = 1 if first line is displayed       */
    int sub_lines;                  /* if > 0 then do not display until end */
    int line_complete;              /* current line complete ? (\n ending)  */
    int unread_data;                /* highlight windows with unread data   */
    
    /* inupt buffer */
    char *input_buffer;             /* input buffer                         */
    int input_buffer_alloc;         /* input buffer: allocated size in mem  */
    int input_buffer_size;          /* buffer size (user input length)      */
    int input_buffer_pos;           /* position into buffer                 */
    int input_buffer_1st_display;   /* first char displayed on screen       */
    
    /* completion */
    t_completion completion;        /* for cmds/nicks completion            */
    
    /* history */
    t_history *history;             /* commands history                     */
    t_history *ptr_history;         /* current command in history           */
    
    /* link to next window */
    t_gui_window *prev_window;      /* link to previous window              */
    t_gui_window *next_window;      /* link to next window                  */
};

/* variables */

extern int gui_ready;
extern t_gui_window *gui_windows;
extern t_gui_window *last_gui_window;
extern t_gui_window *gui_current_window;

/* prototypes */

/* GUI independent functions */
extern t_gui_window *gui_window_new (void *, void * /*int, int, int, int*/); /* TODO: add coordinates and size */
extern void gui_window_clear (t_gui_window *);
extern void gui_window_clear_all ();
extern t_gui_line *gui_new_line (t_gui_window *);
extern t_gui_message *gui_new_message (t_gui_window *);
extern void gui_optimize_input_buffer_size (t_gui_window *);
extern void gui_delete_previous_word ();
extern void gui_move_previous_word ();
extern void gui_move_next_word ();
extern void gui_buffer_insert_string (char *, int);
/* GUI dependant functions */
extern int gui_assign_color (int *, char *);
extern int gui_get_color_by_name (char *);
extern char *gui_get_color_by_value (int);
extern int gui_window_has_nicklist (t_gui_window *);
extern void gui_calculate_pos_size (t_gui_window *);
extern void gui_draw_window_title (t_gui_window *);
extern void gui_redraw_window_title (t_gui_window *);
extern void gui_draw_window_chat (t_gui_window *);
extern void gui_redraw_window_chat (t_gui_window *);
extern void gui_draw_window_nick (t_gui_window *);
extern void gui_redraw_window_nick (t_gui_window *);
extern void gui_draw_window_status (t_gui_window *);
extern void gui_redraw_window_status (t_gui_window *);
extern void gui_draw_window_input (t_gui_window *);
extern void gui_redraw_window_input (t_gui_window *);
extern void gui_redraw_window (t_gui_window *);
extern void gui_switch_to_window (t_gui_window *);
extern void gui_switch_to_previous_window ();
extern void gui_switch_to_next_window ();
extern void gui_move_page_up ();
extern void gui_move_page_down ();
extern void gui_window_init_subwindows (t_gui_window *);
extern void gui_pre_init (int *, char **[]);
//extern void gui_init_colors ();
extern void gui_init ();
extern void gui_window_free (t_gui_window *);
extern void gui_end ();
extern void gui_printf_color_type (t_gui_window *, int, int, char *, ...);
extern void gui_main_loop ();

#endif /* gui.h */
