/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_GUI_BUFFER_H
#define __WEECHAT_GUI_BUFFER_H 1

#include "../common/completion.h"
#include "../common/history.h"

#define BUFFER_TYPE_STANDARD    0
#define BUFFER_TYPE_DCC         1
#define BUFFER_TYPE_RAW_DATA    2

#define SERVER(buffer)  ((t_irc_server *)(buffer->server))
#define CHANNEL(buffer) ((t_irc_channel *)(buffer->channel))

#define BUFFER_IS_SERVER(buffer)  ((SERVER(buffer) || (buffer->all_servers)) && !CHANNEL(buffer))
#define BUFFER_IS_CHANNEL(buffer) (CHANNEL(buffer) && (CHANNEL(buffer)->type == CHANNEL_TYPE_CHANNEL))
#define BUFFER_IS_PRIVATE(buffer) (CHANNEL(buffer) && \
                                  ((CHANNEL(buffer)->type == CHANNEL_TYPE_PRIVATE) \
                                  || (CHANNEL(buffer)->type == CHANNEL_TYPE_DCC_CHAT)))

#define BUFFER_HAS_NICKLIST(buffer) (BUFFER_IS_CHANNEL(buffer))

#define MSG_TYPE_TIME      1
#define MSG_TYPE_PREFIX    2
#define MSG_TYPE_NICK      4
#define MSG_TYPE_INFO      8
#define MSG_TYPE_MSG       16
#define MSG_TYPE_HIGHLIGHT 32
#define MSG_TYPE_NOLOG     64

#define NOTIFY_LEVEL_MIN        0
#define NOTIFY_LEVEL_MAX        3
#define NOTIFY_LEVEL_DEFAULT    NOTIFY_LEVEL_MAX

#define TEXT_SEARCH_DISABLED    0
#define TEXT_SEARCH_BACKWARD    1
#define TEXT_SEARCH_FORWARD     2

/* buffer structures */

typedef struct t_gui_line t_gui_line;

struct t_gui_line
{
    int length;                     /* length of the line (in char)         */
    int length_align;               /* alignment length (time or time/nick) */
    int log_write;                  /* = 1 if line will be written to log   */
    int line_with_message;          /* line contains a message from a user? */
    int line_with_highlight;        /* line contains highlight              */
    time_t date;                    /* date/time of line                    */
    char *nick;                     /* nickname for line (may be NULL)      */
    char *data;                     /* line content                         */
    int ofs_after_date;             /* offset to first char after date      */
    int ofs_start_message;          /* offset to first char after date/nick */
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
    int type;                       /* type: standard (server/channel/pv),  */
                                    /* dcc or raw data                      */
    
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
    char *input_buffer_color_mask;  /* color mask for input buffer          */
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
    
    /* text search */
    int text_search;                /* text search type                     */
    int text_search_exact;          /* exact search (case sensitive) ?      */
    char *text_search_input;        /* input saved before text search       */
    
    /* link to previous/next buffer */
    t_gui_buffer *prev_buffer;      /* link to previous buffer              */
    t_gui_buffer *next_buffer;      /* link to next buffer                  */
};

/* buffer variables */

extern t_gui_buffer *gui_buffers;
extern t_gui_buffer *last_gui_buffer;
extern t_gui_buffer *gui_buffer_before_dcc;
extern t_gui_buffer *gui_buffer_raw_data;
extern t_gui_buffer *gui_buffer_before_raw_data;

#endif /* gui-buffer.h */
