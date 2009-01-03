/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
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

#define GUI_COLOR_NICK_NUMBER 10

enum t_gui_color_enum
{
    GUI_COLOR_SEPARATOR = 0,
    
    GUI_COLOR_CHAT,
    GUI_COLOR_CHAT_TIME,
    GUI_COLOR_CHAT_TIME_DELIMITERS,
    GUI_COLOR_CHAT_PREFIX_ERROR,
    GUI_COLOR_CHAT_PREFIX_NETWORK,
    GUI_COLOR_CHAT_PREFIX_ACTION,
    GUI_COLOR_CHAT_PREFIX_JOIN,
    GUI_COLOR_CHAT_PREFIX_QUIT,
    GUI_COLOR_CHAT_PREFIX_MORE,
    GUI_COLOR_CHAT_PREFIX_SUFFIX,
    GUI_COLOR_CHAT_BUFFER,
    GUI_COLOR_CHAT_SERVER,
    GUI_COLOR_CHAT_CHANNEL,
    GUI_COLOR_CHAT_NICK,
    GUI_COLOR_CHAT_NICK_SELF,
    GUI_COLOR_CHAT_NICK_OTHER,
    GUI_COLOR_CHAT_NICK1,
    GUI_COLOR_CHAT_NICK2,
    GUI_COLOR_CHAT_NICK3,
    GUI_COLOR_CHAT_NICK4,
    GUI_COLOR_CHAT_NICK5,
    GUI_COLOR_CHAT_NICK6,
    GUI_COLOR_CHAT_NICK7,
    GUI_COLOR_CHAT_NICK8,
    GUI_COLOR_CHAT_NICK9,
    GUI_COLOR_CHAT_NICK10,
    GUI_COLOR_CHAT_HOST,
    GUI_COLOR_CHAT_DELIMITERS,
    GUI_COLOR_CHAT_HIGHLIGHT,
    GUI_COLOR_CHAT_READ_MARKER,
    GUI_COLOR_CHAT_TEXT_FOUND,
    
    /* number of colors */
    GUI_COLOR_NUM_COLORS,
};

/* WeeChat internal color attributes (should never be in protocol messages) */

#define GUI_COLOR_COLOR_CHAR           '\x19'
#define GUI_COLOR_COLOR_STR            "\x19"
#define GUI_COLOR_SET_WEECHAT_CHAR     '\x1A'
#define GUI_COLOR_SET_WEECHAT_STR      "\x1A"
#define GUI_COLOR_REMOVE_WEECHAT_CHAR  '\x1B'
#define GUI_COLOR_REMOVE_WEECHAT_STR   "\x1B"
#define GUI_COLOR_RESET_CHAR           '\x1C'
#define GUI_COLOR_RESET_STR            "\x1C"

#define GUI_COLOR_ATTR_BOLD_CHAR       '\x01'
#define GUI_COLOR_ATTR_BOLD_STR        "\x01"
#define GUI_COLOR_ATTR_REVERSE_CHAR    '\x02'
#define GUI_COLOR_ATTR_REVERSE_STR     "\x02"
#define GUI_COLOR_ATTR_ITALIC_CHAR     '\x03'
#define GUI_COLOR_ATTR_ITALIC_STR      "\x03"
#define GUI_COLOR_ATTR_UNDERLINE_CHAR  '\x04'
#define GUI_COLOR_ATTR_UNDERLINE_STR   "\x04"

#define GUI_COLOR(color) ((gui_color[color]) ? gui_color[color]->string : "")
#define GUI_NO_COLOR     GUI_COLOR_RESET_STR

#define GUI_COLOR_CUSTOM_BAR_FG    (gui_color_get_custom ("bar_fg"))
#define GUI_COLOR_CUSTOM_BAR_DELIM (gui_color_get_custom ("bar_delim"))
#define GUI_COLOR_CUSTOM_BAR_BG    (gui_color_get_custom ("bar_bg"))

#define GUI_COLOR_FG_CHAR              'F'
#define GUI_COLOR_FG_STR               "F"
#define GUI_COLOR_BG_CHAR              'B'
#define GUI_COLOR_BG_STR               "B"
#define GUI_COLOR_FG_BG_CHAR           '*'
#define GUI_COLOR_FG_BG_STR            "*"
#define GUI_COLOR_BAR_CHAR             'b'
#define GUI_COLOR_BAR_STR              "b"
#define GUI_COLOR_BAR_FG_CHAR          'F'
#define GUI_COLOR_BAR_FG_STR           "F"
#define GUI_COLOR_BAR_DELIM_CHAR       'D'
#define GUI_COLOR_BAR_DELIM_STR        "D"
#define GUI_COLOR_BAR_BG_CHAR          'B'
#define GUI_COLOR_BAR_BG_STR           "B"
#define GUI_COLOR_BAR_START_INPUT_CHAR '_'
#define GUI_COLOR_BAR_START_INPUT_STR  "_"
#define GUI_COLOR_BAR_MOVE_CURSOR_CHAR '#'
#define GUI_COLOR_BAR_MOVE_CURSOR_STR  "#"

/* color structure */

struct t_gui_color
{
    int foreground;                 /* foreground color                     */
    int background;                 /* background color                     */
    int attributes;                 /* attributes (bold, ..)                */
    char *string;                   /* WeeChat color: "\x19??", ?? is #color*/
};

/* color variables */

extern struct t_gui_color *gui_color[];

/* color functions */

extern int gui_color_search_config_int (const char *color_name);
extern const char *gui_color_search_config_str (int color_number);
extern const char *gui_color_get_custom (const char *color_name);
extern unsigned char *gui_color_decode (const unsigned char *string);
extern void gui_color_free (struct t_gui_color *color);

/* color functions (GUI dependent) */

extern int gui_color_search (const char *color_name);
extern int gui_color_assign (int *color, char const *color_name);
extern int gui_color_get_number ();
extern const char *gui_color_get_name (int num_color);
extern void gui_color_init_pairs ();
extern void gui_color_init_weechat ();

#endif /* gui-color.h */
