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


#ifndef __WEECHAT_GUI_COLOR_H
#define __WEECHAT_GUI_COLOR_H 1

#define GUI_COLOR_WIN_NICK_NUMBER 10

typedef enum t_weechat_color t_weechat_color;

enum t_weechat_color
{
    GUI_COLOR_WIN_SEPARATOR = 0,
    GUI_COLOR_WIN_TITLE,
    GUI_COLOR_WIN_CHAT,
    GUI_COLOR_WIN_CHAT_TIME,
    GUI_COLOR_WIN_CHAT_TIME_SEP,
    GUI_COLOR_WIN_CHAT_PREFIX1,
    GUI_COLOR_WIN_CHAT_PREFIX2,
    GUI_COLOR_WIN_CHAT_SERVER,
    GUI_COLOR_WIN_CHAT_JOIN,
    GUI_COLOR_WIN_CHAT_PART,
    GUI_COLOR_WIN_CHAT_NICK,
    GUI_COLOR_WIN_CHAT_HOST,
    GUI_COLOR_WIN_CHAT_CHANNEL,
    GUI_COLOR_WIN_CHAT_DARK,
    GUI_COLOR_WIN_CHAT_HIGHLIGHT,
    GUI_COLOR_WIN_CHAT_READ_MARKER,
    GUI_COLOR_WIN_STATUS,
    GUI_COLOR_WIN_STATUS_DELIMITERS,
    GUI_COLOR_WIN_STATUS_CHANNEL,
    GUI_COLOR_WIN_STATUS_DATA_MSG,
    GUI_COLOR_WIN_STATUS_DATA_PRIVATE,
    GUI_COLOR_WIN_STATUS_DATA_HIGHLIGHT,
    GUI_COLOR_WIN_STATUS_DATA_OTHER,
    GUI_COLOR_WIN_STATUS_MORE,
    GUI_COLOR_WIN_INFOBAR,
    GUI_COLOR_WIN_INFOBAR_DELIMITERS,
    GUI_COLOR_WIN_INFOBAR_HIGHLIGHT,
    GUI_COLOR_WIN_INPUT,
    GUI_COLOR_WIN_INPUT_CHANNEL,
    GUI_COLOR_WIN_INPUT_NICK,
    GUI_COLOR_WIN_INPUT_DELIMITERS,
    GUI_COLOR_WIN_NICK,
    GUI_COLOR_WIN_NICK_AWAY,
    GUI_COLOR_WIN_NICK_CHANOWNER,
    GUI_COLOR_WIN_NICK_CHANADMIN,
    GUI_COLOR_WIN_NICK_OP,
    GUI_COLOR_WIN_NICK_HALFOP,
    GUI_COLOR_WIN_NICK_VOICE,
    GUI_COLOR_WIN_NICK_MORE,
    GUI_COLOR_WIN_NICK_SEP,
    GUI_COLOR_WIN_NICK_SELF,
    GUI_COLOR_WIN_NICK_PRIVATE,
    GUI_COLOR_WIN_NICK_1,
    GUI_COLOR_WIN_NICK_2,
    GUI_COLOR_WIN_NICK_3,
    GUI_COLOR_WIN_NICK_4,
    GUI_COLOR_WIN_NICK_5,
    GUI_COLOR_WIN_NICK_6,
    GUI_COLOR_WIN_NICK_7,
    GUI_COLOR_WIN_NICK_8,
    GUI_COLOR_WIN_NICK_9,
    GUI_COLOR_WIN_NICK_10,
    GUI_COLOR_DCC_SELECTED,
    GUI_COLOR_DCC_WAITING,
    GUI_COLOR_DCC_CONNECTING,
    GUI_COLOR_DCC_ACTIVE,
    GUI_COLOR_DCC_DONE,
    GUI_COLOR_DCC_FAILED,
    GUI_COLOR_DCC_ABORTED,
    GUI_COLOR_WIN_INPUT_SERVER,
    GUI_COLOR_WIN_TITLE_MORE,
    GUI_COLOR_WIN_INPUT_TEXT_NOT_FOUND,
    GUI_COLOR_WIN_NICK_CHANUSER,
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
#define GUI_ATTR_WEECHAT_RESET_CHAR  '\x1C'
#define GUI_ATTR_WEECHAT_RESET_STR   "\x1C"

#define GUI_COLOR(color) ((gui_color[color]) ? gui_color[color]->string : "")
#define GUI_NO_COLOR     GUI_ATTR_WEECHAT_RESET_STR

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
