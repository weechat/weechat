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


#ifndef __WEECHAT_GUI_COLOR_H
#define __WEECHAT_GUI_COLOR_H 1

#define INPUT_BUFFER_BLOCK_SIZE 256

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
    COLOR_WIN_INPUT_SERVER,
    GUI_NUM_COLORS
};

#define GUI_NUM_IRC_COLORS 16

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

/* color structure */

typedef struct t_gui_color t_gui_color;

struct t_gui_color
{
    int foreground;                 /* foreground color                     */
    int background;                 /* background color                     */
    int attributes;                 /* attributes (bold, ..)                */
    char *string;                   /* WeeChat color: "\x19??", ?? is #color*/
};

/* color variables */

extern t_gui_color *gui_color[GUI_NUM_COLORS];

#endif /* gui-color.h */
