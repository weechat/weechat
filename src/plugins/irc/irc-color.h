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


#ifndef __WEECHAT_IRC_COLOR_H
#define __WEECHAT_IRC_COLOR_H 1

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

/* attributes in IRC messages for color & style (bold, ..) */

#define IRC_COLOR_BOLD_CHAR       '\x02'
#define IRC_COLOR_BOLD_STR        "\x02"
#define IRC_COLOR_COLOR_CHAR      '\x03'
#define IRC_COLOR_COLOR_STR       "\x03"
#define IRC_COLOR_RESET_CHAR      '\x0F'
#define IRC_COLOR_RESET_STR       "\x0F"
#define IRC_COLOR_FIXED_CHAR      '\x11'
#define IRC_COLOR_FIXED_STR       "\x11"
#define IRC_COLOR_REVERSE_CHAR    '\x12'
#define IRC_COLOR_REVERSE_STR     "\x12"
#define IRC_COLOR_REVERSE2_CHAR   '\x16'
#define IRC_COLOR_REVERSE2_STR    "\x16"
#define IRC_COLOR_ITALIC_CHAR     '\x1D'
#define IRC_COLOR_ITALIC_STR      "\x1D"
#define IRC_COLOR_UNDERLINE_CHAR  '\x1F'
#define IRC_COLOR_UNDERLINE_STR   "\x1F"

extern unsigned char *irc_color_decode (unsigned char *string,
                                        int keep_irc_colors,
                                        int keep_weechat_attr);
extern unsigned char *irc_color_decode_for_user_entry (unsigned char *string);
extern unsigned char *irc_color_encode (unsigned char *string, int keep_colors);

#endif /* irc-color.h */
