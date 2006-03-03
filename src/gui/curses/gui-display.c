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

/* gui-display.c: display functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <libgen.h>

#ifdef HAVE_NCURSESW_CURSES_H
#include <ncursesw/ncurses.h>
#else
#include <ncurses.h>
#endif

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/weeconfig.h"
#include "../../common/hotlist.h"
#include "../../common/log.h"
#include "../../common/utf8.h"
#include "../../irc/irc.h"


t_gui_color gui_weechat_colors[] =
{ { -1,                    0, 0,        "default"      },
  { WEECHAT_COLOR_BLACK,   0, 0,        "black"        },
  { WEECHAT_COLOR_RED,     0, 0,        "red"          },
  { WEECHAT_COLOR_RED,     0, A_BOLD,   "lightred"     },
  { WEECHAT_COLOR_GREEN,   0, 0,        "green"        },
  { WEECHAT_COLOR_GREEN,   0, A_BOLD,   "lightgreen"   },
  { WEECHAT_COLOR_YELLOW,  0, 0,        "brown"        },
  { WEECHAT_COLOR_YELLOW,  0, A_BOLD,   "yellow"       },
  { WEECHAT_COLOR_BLUE,    0, 0,        "blue"         },
  { WEECHAT_COLOR_BLUE,    0, A_BOLD,   "lightblue"    },
  { WEECHAT_COLOR_MAGENTA, 0, 0,        "magenta"      },
  { WEECHAT_COLOR_MAGENTA, 0, A_BOLD,   "lightmagenta" },
  { WEECHAT_COLOR_CYAN,    0, 0,        "cyan"         },
  { WEECHAT_COLOR_CYAN,    0, A_BOLD,   "lightcyan"    },
  { WEECHAT_COLOR_WHITE,   0, A_BOLD,   "white"        },
  { 0,                     0, 0,        NULL           }
};

int gui_irc_colors[16][2] =
{ { /*  0 */ WEECHAT_COLOR_WHITE,   A_BOLD },
  { /*  1 */ WEECHAT_COLOR_BLACK,   0      },
  { /*  2 */ WEECHAT_COLOR_BLUE,    0      },
  { /*  3 */ WEECHAT_COLOR_GREEN,   0      },
  { /*  4 */ WEECHAT_COLOR_RED,     A_BOLD },
  { /*  5 */ WEECHAT_COLOR_RED,     0      },
  { /*  6 */ WEECHAT_COLOR_MAGENTA, 0      },
  { /*  7 */ WEECHAT_COLOR_YELLOW,  0      },
  { /*  8 */ WEECHAT_COLOR_YELLOW,  A_BOLD },
  { /*  9 */ WEECHAT_COLOR_GREEN,   A_BOLD },
  { /* 10 */ WEECHAT_COLOR_CYAN,    0      },
  { /* 11 */ WEECHAT_COLOR_CYAN,    A_BOLD },
  { /* 12 */ WEECHAT_COLOR_BLUE,    A_BOLD },
  { /* 13 */ WEECHAT_COLOR_MAGENTA, A_BOLD },
  { /* 14 */ WEECHAT_COLOR_WHITE,   0      },
  { /* 15 */ WEECHAT_COLOR_WHITE,   A_BOLD }
};

t_gui_color *gui_color[NUM_COLORS];


/*
 * gui_assign_color: assign a WeeChat color (read from config)
 */

int
gui_assign_color (int *color, char *color_name)
{
    int i;
    
    /* look for curses colors in table */
    i = 0;
    while (gui_weechat_colors[i].string)
    {
        if (ascii_strcasecmp (gui_weechat_colors[i].string, color_name) == 0)
        {
            *color = i;
            return 1;
        }
        i++;
    }
    
    /* color not found */
    return 0;
}

/*
 * gui_get_color_name: get color name
 */

char *
gui_get_color_name (int num_color)
{
    return gui_weechat_colors[num_color].string;
}

/*
 * gui_color_decode: parses a message (coming from IRC server),
 *                   and according:
 *                     - remove any color/style in message
 *                   or:
 *                     - change colors by codes to be compatible with
 *                       other IRC clients
 *                   After use, string returned has to be free()
 */

unsigned char *
gui_color_decode (unsigned char *string, int keep_colors)
{
    unsigned char *out;
    int out_length, out_pos;
    char str_fg[3], str_bg[3];
    int fg, bg, attr;
    
    out_length = (strlen ((char *)string) * 2) + 1;
    out = (unsigned char *)malloc (out_length);
    if (!out)
        return NULL;
    
    out_pos = 0;
    while (string[0] && (out_pos < out_length - 1))
    {
        switch (string[0])
        {
            case GUI_ATTR_BOLD_CHAR:
            case GUI_ATTR_RESET_CHAR:
            case GUI_ATTR_FIXED_CHAR:
            case GUI_ATTR_REVERSE_CHAR:
            case GUI_ATTR_REVERSE2_CHAR:
            case GUI_ATTR_ITALIC_CHAR:
            case GUI_ATTR_UNDERLINE_CHAR:
                if (keep_colors)
                    out[out_pos++] = string[0];
                string++;
                break;
            case GUI_ATTR_COLOR_CHAR:
                string++;
                str_fg[0] = '\0';
                str_bg[0] = '\0';
                if (isdigit (string[0]))
                {
                    str_fg[0] = string[0];
                    str_fg[1] = '\0';
                    string++;
                    if (isdigit (string[0]))
                    {
                        str_fg[1] = string[0];
                        str_fg[2] = '\0';
                        string++;
                    }
                }
                if (string[0] == ',')
                {
                    string++;
                    if (isdigit (string[0]))
                    {
                        str_bg[0] = string[0];
                        str_bg[1] = '\0';
                        string++;
                        if (isdigit (string[0]))
                        {
                            str_bg[1] = string[0];
                            str_bg[2] = '\0';
                            string++;
                        }
                    }
                }
                if (keep_colors)
                {
                    if (!str_fg[0] && !str_bg[0])
                        out[out_pos++] = GUI_ATTR_COLOR_CHAR;
                    else
                    {
                        attr = 0;
                        if (str_fg[0])
                        {
                            sscanf (str_fg, "%d", &fg);
                            fg %= 16;
                            attr |= gui_irc_colors[fg][1];
                        }
                        if (str_bg[0])
                        {
                            sscanf (str_bg, "%d", &bg);
                            bg %= 16;
                            attr |= gui_irc_colors[bg][1];
                        }
                        if (attr & A_BOLD)
                        {
                            out[out_pos++] = GUI_ATTR_WEECHAT_SET_CHAR;
                            out[out_pos++] = GUI_ATTR_BOLD_CHAR;
                        }
                        else
                        {
                            out[out_pos++] = GUI_ATTR_WEECHAT_REMOVE_CHAR;
                            out[out_pos++] = GUI_ATTR_BOLD_CHAR;
                        }
                        out[out_pos++] = GUI_ATTR_COLOR_CHAR;
                        if (str_fg[0])
                        {
                            out[out_pos++] = (gui_irc_colors[fg][0] / 10) + '0';
                            out[out_pos++] = (gui_irc_colors[fg][0] % 10) + '0';
                        }
                        if (str_bg[0])
                        {
                            out[out_pos++] = ',';
                            out[out_pos++] = (gui_irc_colors[bg][0] / 10) + '0';
                            out[out_pos++] = (gui_irc_colors[bg][0] % 10) + '0';
                        }
                    }
                }
                break;
            case GUI_ATTR_WEECHAT_COLOR_CHAR:
                string++;
                if (isdigit (string[0]) && isdigit (string[1]))
                {
                    if (keep_colors)
                    {
                        out[out_pos++] = string[0];
                        out[out_pos++] = string[1];
                    }
                    string += 2;
                }
                break;
            case GUI_ATTR_WEECHAT_SET_CHAR:
            case GUI_ATTR_WEECHAT_REMOVE_CHAR:
                string++;
                if (string[0])
                {
                    if (keep_colors)
                    {
                        out[out_pos++] = *(string - 1);
                        out[out_pos++] = string[0];
                    }
                    string++;
                }
                break;
            default:
                out[out_pos++] = string[0];
                string++;
        }
    }
    out[out_pos] = '\0';
    return out;
}

/*
 * gui_color_decode_for_user_entry: parses a message (coming from IRC server),
 *                                  and replaces colors/bold/.. by %C, %B, ..
 *                                  After use, string returned has to be free()
 */

unsigned char *
gui_color_decode_for_user_entry (unsigned char *string)
{
    unsigned char *out;
    int out_length, out_pos;
    
    out_length = (strlen ((char *)string) * 2) + 1;
    out = (unsigned char *)malloc (out_length);
    if (!out)
        return NULL;
    
    out_pos = 0;
    while (string[0] && (out_pos < out_length - 1))
    {
        switch (string[0])
        {
            case GUI_ATTR_BOLD_CHAR:
                out[out_pos++] = '%';
                out[out_pos++] = 'B';
                string++;
                break;
            case GUI_ATTR_FIXED_CHAR:
                string++;
                break;
            case GUI_ATTR_RESET_CHAR:
                out[out_pos++] = '%';
                out[out_pos++] = 'O';
                string++;
                break;
            case GUI_ATTR_REVERSE_CHAR:
            case GUI_ATTR_REVERSE2_CHAR:
                out[out_pos++] = '%';
                out[out_pos++] = 'R';
                string++;
                break;
            case GUI_ATTR_ITALIC_CHAR:
                string++;
                break;
            case GUI_ATTR_UNDERLINE_CHAR:
                out[out_pos++] = '%';
                out[out_pos++] = 'R';
                string++;
                break;
            case GUI_ATTR_COLOR_CHAR:
                out[out_pos++] = '%';
                out[out_pos++] = 'C';
                string++;
                break;
            default:
                out[out_pos++] = string[0];
                string++;
        }
    }
    out[out_pos] = '\0';
    return out;
}

/*
 * gui_color_encode: parses a message (entered by user), and
 *                   encode special chars (%B, %C, ..) in IRC colors
 *                   After use, string returned has to be free()
 */

unsigned char *
gui_color_encode (unsigned char *string)
{
    unsigned char *out;
    int out_length, out_pos;
    
    out_length = (strlen ((char *)string) * 2) + 1;
    out = (unsigned char *)malloc (out_length);
    if (!out)
        return NULL;
    
    out_pos = 0;
    while (string[0] && (out_pos < out_length - 1))
    {
        switch (string[0])
        {
            case '%':
                string++;
                switch (string[0])
                {
                    case '\0':
                        out[out_pos++] = '%';
                        break;
                    case '%': /* double '%' replaced by single '%' */
                        out[out_pos++] = string[0];
                        string++;
                        break;
                    case 'B': /* bold */
                        out[out_pos++] = GUI_ATTR_BOLD_CHAR;
                        string++;
                        break;
                    case 'C': /* color */
                        out[out_pos++] = GUI_ATTR_COLOR_CHAR;
                        string++;
                        if (isdigit (string[0]))
                        {
                            out[out_pos++] = string[0];
                            string++;
                            if (isdigit (string[0]))
                            {
                                out[out_pos++] = string[0];
                                string++;
                            }
                        }
                        if (string[0] == ',')
                        {
                            out[out_pos++] = ',';
                            string++;
                            if (isdigit (string[0]))
                            {
                                out[out_pos++] = string[0];
                                string++;
                                if (isdigit (string[0]))
                                {
                                out[out_pos++] = string[0];
                                string++;
                                }
                            }
                        }
                        break;
                    case 'O': /* reset */
                        out[out_pos++] = GUI_ATTR_RESET_CHAR;
                        string++;
                        break;
                    case 'R': /* reverse */
                        out[out_pos++] = GUI_ATTR_REVERSE_CHAR;
                        string++;
                        break;
                    case 'U': /* underline */
                        out[out_pos++] = GUI_ATTR_UNDERLINE_CHAR;
                        string++;
                        break;
                    default:
                        out[out_pos++] = '%';
                        out[out_pos++] = string[0];
                        string++;
                }
                break;
            default:
                out[out_pos++] = string[0];
                string++;
        }
    }
    out[out_pos] = '\0';
    return out;
}

/*
 * gui_color_build: build a WeeChat color with foreground,
 *                  background and attributes (attributes are
 *                  given with foreground color, with a OR)
 */

t_gui_color *
gui_color_build (int number, int foreground, int background)
{
    t_gui_color *new_color;
    
    new_color = (t_gui_color *)malloc (sizeof (t_gui_color));
    if (!new_color)
        return NULL;
    
    new_color->foreground = gui_weechat_colors[foreground].foreground;
    new_color->background = gui_weechat_colors[background].foreground;
    new_color->attributes = gui_weechat_colors[foreground].attributes;
    new_color->string = (char *)malloc (4);
    if (new_color->string)
        snprintf (new_color->string, 4,
                  "%s%02d",
                  GUI_ATTR_WEECHAT_COLOR_STR, number);
    
    return new_color;
}

/*
 * gui_color_get_pair: get color pair with a WeeChat color number
 */

int
gui_color_get_pair (int num_color)
{
    int fg, bg;
    
    if ((num_color < 0) || (num_color > NUM_COLORS - 1))
        return WEECHAT_COLOR_WHITE;
    
    fg = gui_color[num_color]->foreground;
    bg = gui_color[num_color]->background;
    
    if (((fg == -1) || (fg == 99))
        && ((bg == -1) || (bg == 99)))
        return 63;
    if ((fg == -1) || (fg == 99))
        fg = WEECHAT_COLOR_WHITE;
    if ((bg == -1) || (bg == 99))
        bg = 0;
    
    return (bg * 8) + fg;
}

/*
 * gui_window_set_weechat_color: set WeeChat color for window
 */

void
gui_window_set_weechat_color (WINDOW *window, int num_color)
{
    if ((num_color >= 0) && (num_color <= NUM_COLORS - 1))
    {
        wattroff (window, A_BOLD | A_UNDERLINE | A_REVERSE);
        wattron (window, COLOR_PAIR(gui_color_get_pair (num_color)) |
                 gui_color[num_color]->attributes);
    }
}

/*
 * gui_window_chat_set_style: set style (bold, underline, ..)
 *                            for a chat window
 */

void
gui_window_chat_set_style (t_gui_window *window, int style)
{
    wattron (window->win_chat, style);
}

/*
 * gui_window_chat_remove_style: remove style (bold, underline, ..)
 *                               for a chat window
 */

void
gui_window_chat_remove_style (t_gui_window *window, int style)
{
    wattroff (window->win_chat, style);
}

/*
 * gui_window_chat_toggle_style: toggle a style (bold, underline, ..)
 *                               for a chat window
 */

void
gui_window_chat_toggle_style (t_gui_window *window, int style)
{
    window->current_style_attr ^= style;
    if (window->current_style_attr & style)
        gui_window_chat_set_style (window, style);
    else
        gui_window_chat_remove_style (window, style);
}

/*
 * gui_window_chat_reset_style: reset style (color and attr)
 *                              for a chat window
 */

void
gui_window_chat_reset_style (t_gui_window *window)
{
    window->current_style_fg = -1;
    window->current_style_bg = -1;
    window->current_style_attr = 0;
    window->current_color_attr = 0;
    
    gui_window_set_weechat_color (window->win_chat, COLOR_WIN_CHAT);
    gui_window_chat_remove_style (window,
                                  A_BOLD | A_UNDERLINE | A_REVERSE);
}

/*
 * gui_window_chat_set_color_style: set style for color
 */

void
gui_window_chat_set_color_style (t_gui_window *window, int style)
{
    window->current_color_attr |= style;
    wattron (window->win_chat, style);
}

/*
 * gui_window_chat_remove_color_style: remove style for color
 */

void
gui_window_chat_remove_color_style (t_gui_window *window, int style)
{
    window->current_color_attr &= !style;
    wattroff (window->win_chat, style);
}

/*
 * gui_window_chat_reset_color_style: reset style for color
 */

void
gui_window_chat_reset_color_style (t_gui_window *window)
{
    wattroff (window->win_chat, window->current_color_attr);
    window->current_color_attr = 0;
}

/*
 * gui_window_chat_set_color: set color for a chat window
 */

void
gui_window_chat_set_color (t_gui_window *window, int fg, int bg)
{
    if (((fg == -1) || (fg == 99))
        && ((bg == -1) || (bg == 99)))
        wattron (window->win_chat, COLOR_PAIR(63));
    else
    {
        if ((fg == -1) || (fg == 99))
            fg = WEECHAT_COLOR_WHITE;
        if ((bg == -1) || (bg == 99))
            bg = 0;
        wattron (window->win_chat, COLOR_PAIR((bg * 8) + fg));
    }
}

/*
 * gui_window_chat_set_weechat_color: set a WeeChat color for a chat window
 */

void
gui_window_chat_set_weechat_color (t_gui_window *window, int weechat_color)
{
    gui_window_chat_reset_style (window);
    gui_window_chat_set_style (window,
                               gui_color[weechat_color]->attributes);
    gui_window_chat_set_color (window,
                               gui_color[weechat_color]->foreground,
                               gui_color[weechat_color]->background);
}

/*
 * gui_calculate_pos_size: calculate position and size for a buffer & subwindows
 */

void
gui_calculate_pos_size (t_gui_window *window)
{
    int max_length, lines;
    int num_nicks, num_op, num_halfop, num_voice, num_normal;
    
    if (!gui_ok)
        return;
    
    /* init chat & nicklist settings */
    if (cfg_look_nicklist && BUFFER_IS_CHANNEL(window->buffer))
    {
        max_length = nick_get_max_length (CHANNEL(window->buffer));
        
        if ((cfg_look_nicklist_min_size > 0)
            && (max_length < cfg_look_nicklist_min_size))
            max_length = cfg_look_nicklist_min_size;
        else if ((cfg_look_nicklist_max_size > 0)
                 && (max_length > cfg_look_nicklist_max_size))
            max_length = cfg_look_nicklist_max_size;
        
        switch (cfg_look_nicklist_position)
        {
            case CFG_LOOK_NICKLIST_LEFT:
                window->win_chat_x = window->win_x + max_length + 2;
                window->win_chat_y = window->win_y + 1;
                window->win_chat_width = window->win_width - max_length - 2;
                window->win_nick_x = window->win_x + 0;
                window->win_nick_y = window->win_y + 1;
                window->win_nick_width = max_length + 2;
                if (cfg_look_infobar)
                {
                    window->win_chat_height = window->win_height - 4;
                    window->win_nick_height = window->win_height - 4;
                }
                else
                {
                    window->win_chat_height = window->win_height - 3;
                    window->win_nick_height = window->win_height - 3;
                }
                break;
            case CFG_LOOK_NICKLIST_RIGHT:
                window->win_chat_x = window->win_x;
                window->win_chat_y = window->win_y + 1;
                window->win_chat_width = window->win_width - max_length - 2;
                window->win_nick_x = window->win_x + window->win_width - max_length - 2;
                window->win_nick_y = window->win_y + 1;
                window->win_nick_width = max_length + 2;
                if (cfg_look_infobar)
                {
                    window->win_chat_height = window->win_height - 4;
                    window->win_nick_height = window->win_height - 4;
                }
                else
                {
                    window->win_chat_height = window->win_height - 3;
                    window->win_nick_height = window->win_height - 3;
                }
                break;
            case CFG_LOOK_NICKLIST_TOP:
                nick_count (CHANNEL(window->buffer), &num_nicks, &num_op,
                            &num_halfop, &num_voice, &num_normal);
                if (((max_length + 2) * num_nicks) % window->win_width == 0)
                    lines = ((max_length + 2) * num_nicks) / window->win_width;
                else
                    lines = (((max_length + 2) * num_nicks) / window->win_width) + 1;
                window->win_chat_x = window->win_x;
                window->win_chat_y = window->win_y + 1 + (lines + 1);
                window->win_chat_width = window->win_width;
                if (cfg_look_infobar)
                    window->win_chat_height = window->win_height - 3 - (lines + 1) - 1;
                else
                    window->win_chat_height = window->win_height - 3 - (lines + 1);
                window->win_nick_x = window->win_x;
                window->win_nick_y = window->win_y + 1;
                window->win_nick_width = window->win_width;
                window->win_nick_height = lines + 1;
                break;
            case CFG_LOOK_NICKLIST_BOTTOM:
                nick_count (CHANNEL(window->buffer), &num_nicks, &num_op,
                            &num_halfop, &num_voice, &num_normal);
                if (((max_length + 2) * num_nicks) % window->win_width == 0)
                    lines = ((max_length + 2) * num_nicks) / window->win_width;
                else
                    lines = (((max_length + 2) * num_nicks) / window->win_width) + 1;
                window->win_chat_x = window->win_x;
                window->win_chat_y = window->win_y + 1;
                window->win_chat_width = window->win_width;
                if (cfg_look_infobar)
                    window->win_chat_height = window->win_height - 3 - (lines + 1) - 1;
                else
                    window->win_chat_height = window->win_height - 3 - (lines + 1);
                window->win_nick_x = window->win_x;
                if (cfg_look_infobar)
                    window->win_nick_y = window->win_y + window->win_height - 2 - (lines + 1) - 1;
                else
                    window->win_nick_y = window->win_y + window->win_height - 2 - (lines + 1);
                window->win_nick_width = window->win_width;
                window->win_nick_height = lines + 1;
                break;
        }
        
        window->win_chat_cursor_x = window->win_x;
        window->win_chat_cursor_y = window->win_y;
    }
    else
    {
        window->win_chat_x = window->win_x;
        window->win_chat_y = window->win_y + 1;
        window->win_chat_width = window->win_width;
        if (cfg_look_infobar)
            window->win_chat_height = window->win_height - 4;
        else
            window->win_chat_height = window->win_height - 3;
        window->win_chat_cursor_x = window->win_x;
        window->win_chat_cursor_y = window->win_y;
        window->win_nick_x = -1;
        window->win_nick_y = -1;
        window->win_nick_width = -1;
        window->win_nick_height = -1;
    }
}

/*
 * gui_curses_window_clear: clear a window
 */

void
gui_curses_window_clear (WINDOW *window, int num_color)
{
    if (!gui_ok)
        return;
    
    wbkgdset(window, ' ' | COLOR_PAIR (gui_color_get_pair (num_color)));
    werase (window);
    wmove (window, 0, 0);
}

/*
 * gui_draw_window_separator: draw window separation
 */

void
gui_draw_window_separator (t_gui_window *window)
{
    if (window->win_separator)
        delwin (window->win_separator);
    
    if (window->win_x > 0)
    {
        window->win_separator = newwin (window->win_height,
                                        1,
                                        window->win_y,
                                        window->win_x - 1);
        gui_window_set_weechat_color (window->win_separator, COLOR_WIN_SEPARATOR);
        wborder (window->win_separator, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        wnoutrefresh (window->win_separator);
        refresh ();
    }
}

/*
 * gui_draw_buffer_title: draw title window for a buffer
 */

void
gui_draw_buffer_title (t_gui_buffer *buffer, int erase)
{
    t_gui_window *ptr_win;
    char format[32], *buf, *buf2;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            if (erase)
                gui_curses_window_clear (ptr_win->win_title, COLOR_WIN_TITLE);
            
            gui_window_set_weechat_color (ptr_win->win_title, COLOR_WIN_TITLE);
            snprintf (format, 32, "%%-%ds", ptr_win->win_width);
            if (CHANNEL(buffer))
            {
                if (CHANNEL(buffer)->topic)
                {
                    buf = (char *)gui_color_decode ((unsigned char *)(CHANNEL(buffer)->topic), 0);
                    buf2 = channel_iconv_decode (SERVER(buffer),
                                                 CHANNEL(buffer),
                                                 (buf) ? buf : CHANNEL(buffer)->topic);
                    mvwprintw (ptr_win->win_title, 0, 0, format, (buf2) ? buf2 : CHANNEL(buffer)->topic);
                    if (buf)
                        free (buf);
                    if (buf2)
                        free (buf2);
                }
                else
                    mvwprintw (ptr_win->win_title, 0, 0, format, " ");
            }
            else
            {
                if (buffer->type == BUFFER_TYPE_STANDARD)
                {
                    mvwprintw (ptr_win->win_title, 0, 0,
                               format,
                               PACKAGE_STRING " " WEECHAT_COPYRIGHT_DATE " - "
                               WEECHAT_WEBSITE);
                }
                else
                    mvwprintw (ptr_win->win_title, 0, 0, format, " ");
            }
            wnoutrefresh (ptr_win->win_title);
            refresh ();
        }
    }
}

/*
 * gui_curses_display_new_line: display a new line
 */

void
gui_curses_display_new_line (t_gui_window *window, int num_lines, int count,
                             int *lines_displayed, int simulate)
{
    if ((count == 0) || (*lines_displayed >= num_lines - count))
    {
        if ((!simulate) && (window->win_chat_cursor_x <= window->win_chat_width - 1))
        {
            wmove (window->win_chat,
                   window->win_chat_cursor_y,
                   window->win_chat_cursor_x);
            wclrtoeol (window->win_chat);
        }
        window->win_chat_cursor_y++;
    }
    window->win_chat_cursor_x = 0;
    (*lines_displayed)++;
}

/*
 * gui_word_get_next_char: returns next char of a word
 *                         special chars like colors, bold, .. are skipped
 */

char *
gui_word_get_next_char (t_gui_window *window, unsigned char *string, int apply_style)
{
    char str_fg[3], str_bg[3];
    int fg, bg, weechat_color;
    
    while (string[0])
    {
        switch (string[0])
        {
            case GUI_ATTR_BOLD_CHAR:
                string++;
                if (apply_style)
                    gui_window_chat_toggle_style (window, A_BOLD);
                break;
            case GUI_ATTR_COLOR_CHAR:
                string++;
                str_fg[0] = '\0';
                str_bg[0] = '\0';
                fg = 99;
                bg = 99;
                if (isdigit (string[0]))
                {
                    str_fg[0] = string[0];
                    str_fg[1] = '\0';
                    string++;
                    if (isdigit (string[0]))
                    {
                        str_fg[1] = string[0];
                        str_fg[2] = '\0';
                        string++;
                    }
                }
                if (string[0] == ',')
                {
                    string++;
                    if (isdigit (string[0]))
                    {
                        str_bg[0] = string[0];
                        str_bg[1] = '\0';
                        string++;
                        if (isdigit (string[0]))
                        {
                            str_bg[1] = string[0];
                            str_bg[2] = '\0';
                            string++;
                        }
                    }
                }
                if (apply_style)
                {
                    if (str_fg[0] || str_bg[0])
                    {
                        if (str_fg[0])
                            sscanf (str_fg, "%d", &fg);
                        else
                            fg = window->current_style_fg;
                        if (str_bg[0])
                            sscanf (str_bg, "%d", &bg);
                        else
                            bg = window->current_style_bg;
                    }
                    if (!str_fg[0] && !str_bg[0])
                        gui_window_chat_reset_color_style (window);
                    window->current_style_fg = fg;
                    window->current_style_bg = bg;
                    gui_window_chat_set_color (window, fg, bg);
                }
                break;
            case GUI_ATTR_RESET_CHAR:
                string++;
                if (apply_style)
                    gui_window_chat_reset_style (window);
                break;
            case GUI_ATTR_FIXED_CHAR:
                string++;
                break;
            case GUI_ATTR_REVERSE_CHAR:
            case GUI_ATTR_REVERSE2_CHAR:
                string++;
                if (apply_style)
                    gui_window_chat_toggle_style (window, A_REVERSE);
                break;
            case GUI_ATTR_WEECHAT_COLOR_CHAR:
                string++;
                if (isdigit (string[0]) && isdigit (string[1]))
                {
                    str_fg[0] = string[0];
                    str_fg[1] = string[1];
                    str_fg[2] = '\0';
                    string += 2;
                    if (apply_style)
                    {
                        sscanf (str_fg, "%d", &weechat_color);
                        gui_window_chat_set_weechat_color (window, weechat_color);
                    }
                }
                break;
            case GUI_ATTR_WEECHAT_SET_CHAR:
                string++;
                switch (string[0])
                {
                    case GUI_ATTR_BOLD_CHAR:
                        string++;
                        if (apply_style)
                            gui_window_chat_set_color_style (window, A_BOLD);
                        break;
                    case GUI_ATTR_REVERSE_CHAR:
                    case GUI_ATTR_REVERSE2_CHAR:
                        string++;
                        if (apply_style)
                            gui_window_chat_set_color_style (window, A_REVERSE);
                        break;
                    case GUI_ATTR_UNDERLINE_CHAR:
                        string++;
                        if (apply_style)
                            gui_window_chat_set_color_style (window, A_UNDERLINE);
                        break;
                }
                break;
            case GUI_ATTR_WEECHAT_REMOVE_CHAR:
                string++;
                switch (string[0])
                {
                    case GUI_ATTR_BOLD_CHAR:
                        string++;
                        if (apply_style)
                            gui_window_chat_remove_color_style (window, A_BOLD);
                        break;
                    case GUI_ATTR_REVERSE_CHAR:
                    case GUI_ATTR_REVERSE2_CHAR:
                        string++;
                        if (apply_style)
                            gui_window_chat_remove_color_style (window, A_REVERSE);
                        break;
                    case GUI_ATTR_UNDERLINE_CHAR:
                        string++;
                        if (apply_style)
                            gui_window_chat_remove_color_style (window, A_UNDERLINE);
                        break;
                }
                break;
            case GUI_ATTR_ITALIC_CHAR:
                string++;
                break;
            case GUI_ATTR_UNDERLINE_CHAR:
                string++;
                if (apply_style)
                    gui_window_chat_toggle_style (window, A_UNDERLINE);
                break;
            default:
                if (string[0] < 32)
                    string++;
                else
                    return utf8_next_char ((char *)string);
        }
            
    }
    
    /* nothing found except color/attrib codes, so return NULL */
    return NULL;
}

/*
 * gui_display_word_raw: display word on chat buffer, letter by letter
 *                       special chars like color, bold, .. are interpreted
 */

void
gui_display_word_raw (t_gui_window *window, char *string)
{
    char *prev_char, *next_char, saved_char;
    
    wmove (window->win_chat,
           window->win_chat_cursor_y,
           window->win_chat_cursor_x);
    
    while (string && string[0])
    {
        next_char = gui_word_get_next_char (window, (unsigned char *)string, 1);
        if (!next_char)
            return;
        
        prev_char = utf8_prev_char (string, next_char);
        if (prev_char)
        {
            saved_char = next_char[0];
            next_char[0] = '\0';
            wprintw (window->win_chat, "%s", prev_char);
            next_char[0] = saved_char;
        }
        
        string = next_char;
    }
}

/*
 * gui_display_word: display a word on chat buffer
 */

void
gui_display_word (t_gui_window *window,
                  t_gui_line *line,
                  char *data,
                  char *end_offset,
                  int num_lines, int count, int *lines_displayed, int simulate)
{
    char *end_line, saved_char_end, saved_char;
    int pos_saved_char, chars_to_display, num_displayed;
    
    if (!data ||
        ((!simulate) && (window->win_chat_cursor_y >= window->win_chat_height)))
        return;
    
    end_line = data + strlen (data);
        
    if (end_offset[0])
    {
        saved_char_end = end_offset[1];
        end_offset[1] = '\0';
    }
    else
    {
        end_offset = NULL;
        saved_char_end = '\0';
    }
    
    while (data && data[0])
    {
        /* insert spaces for align text under time/nick */
        if ((line->length_align > 0) &&
            (window->win_chat_cursor_x == 0) &&
            (*lines_displayed > 0) &&
            /* TODO: modify arbitraty value for non aligning messages on time/nick? */
            (line->length_align < (window->win_chat_width - 5)))
        {
            if (!simulate)
            {
                wmove (window->win_chat,
                       window->win_chat_cursor_y,
                       window->win_chat_cursor_x);
                wclrtoeol (window->win_chat);
            }
            window->win_chat_cursor_x += line->length_align;
        }
        
        chars_to_display = gui_word_strlen (window, data);

        /* too long for current line */
        if (window->win_chat_cursor_x + chars_to_display > window->win_chat_width)
        {
            num_displayed = window->win_chat_width - window->win_chat_cursor_x;
            pos_saved_char = gui_word_real_pos (window, data, num_displayed);
            saved_char = data[pos_saved_char];
            data[pos_saved_char] = '\0';
            if ((!simulate) &&
                ((count == 0) || (*lines_displayed >= num_lines - count)))
                gui_display_word_raw (window, data);
            data[pos_saved_char] = saved_char;
            data += pos_saved_char;
        }
        else
        {
            num_displayed = chars_to_display;
            if ((!simulate) &&
                ((count == 0) || (*lines_displayed >= num_lines - count)))
                gui_display_word_raw (window, data);
            data += strlen (data);
        }
        
        window->win_chat_cursor_x += num_displayed;
        
        /* display new line? */
        if ((data >= end_line) ||
            (((simulate) ||
             (window->win_chat_cursor_y <= window->win_chat_height - 1)) &&
            (window->win_chat_cursor_x > (window->win_chat_width - 1))))
            gui_curses_display_new_line (window, num_lines, count,
                                         lines_displayed, simulate);
        
        if ((data >= end_line) ||
            ((!simulate) && (window->win_chat_cursor_y >= window->win_chat_height)))
            data = NULL;
    }
    
    if (end_offset)
        end_offset[1] = saved_char_end;
}

/*
 * gui_get_word_info: returns info about next word: beginning, end, length
 */

void
gui_get_word_info (t_gui_window *window,
                   char *data,
                   int *word_start_offset, int *word_end_offset,
                   int *word_length_with_spaces, int *word_length)
{
    char *start_data, *prev_char, *next_char;
    int leading_spaces, char_size;
    
    *word_start_offset = 0;
    *word_end_offset = 0;
    *word_length_with_spaces = 0;
    *word_length = 0;
    
    start_data = data;
    
    leading_spaces = 1;
    while (data && data[0])
    {
        next_char = gui_word_get_next_char (window, (unsigned char *)data, 0);
        if (next_char)
        {
            prev_char = utf8_prev_char (data, next_char);
            if (prev_char)
            {
                if (prev_char[0] != ' ')
                {
                    if (leading_spaces)
                        *word_start_offset = prev_char - start_data;
                    leading_spaces = 0;
                    char_size = next_char - prev_char;
                    *word_end_offset = next_char - start_data - 1;
                    (*word_length_with_spaces) += char_size;
                    (*word_length) += char_size;
                }
                else
                {
                    if (leading_spaces)
                        (*word_length_with_spaces)++;
                    else
                    {
                        *word_end_offset = prev_char - start_data - 1;
                        return;
                    }
                }
                data = next_char;
            }
        }
        else
        {
            *word_end_offset = data + strlen (data) - start_data - 1;
            return;
        }
    }
}

/*
 * gui_curses_display_line: display a line in the chat window
 *                          if count == 0, display whole line
 *                          if count > 0, display 'count' lines
 *                            (beginning from the end)
 *                          if simulate == 1, nothing is displayed
 *                            (for counting how many lines would have been
 *                            lines displayed)
 *                          returns: number of lines displayed (or simulated)
 */

int
gui_curses_display_line (t_gui_window *window, t_gui_line *line, int count,
                         int simulate)
{
    int num_lines, x, y, lines_displayed;
    int read_marker_x, read_marker_y;
    int word_start_offset, word_end_offset;
    int word_length_with_spaces, word_length;
    char *ptr_data, *ptr_end_offset, *next_char, *prev_char;
    char *ptr_style, saved_char;
    
    if (simulate)
    {
        x = window->win_chat_cursor_x;
        y = window->win_chat_cursor_y;
        window->win_chat_cursor_x = 0;
        window->win_chat_cursor_y = 0;
        num_lines = 0;
    }
    else
    {
        if (window->win_chat_cursor_y > window->win_chat_height - 1)
            return 0;
        x = window->win_chat_cursor_x;
        y = window->win_chat_cursor_y;
        num_lines = gui_curses_display_line (window, line, 0, 1);
        window->win_chat_cursor_x = x;
        window->win_chat_cursor_y = y;
    }
    
    /* calculate marker position (maybe not used for this line!) */
    if (line->ofs_after_date > 0)
    {
        saved_char = line->data[line->ofs_after_date - 1];
        line->data[line->ofs_after_date - 1] = '\0';
        read_marker_x = x + gui_word_strlen (NULL, line->data);
        line->data[line->ofs_after_date - 1] = saved_char;
    }
    else
        read_marker_x = x;
    read_marker_y = y;
    
    /* reset color & style for a new line */
    gui_window_chat_reset_style (window);
    
    lines_displayed = 0;
    ptr_data = line->data;
    while (ptr_data && ptr_data[0])
    {
        gui_get_word_info (window,
                           ptr_data,
                           &word_start_offset,
                           &word_end_offset,
                           &word_length_with_spaces, &word_length);
        
        ptr_end_offset = ptr_data + word_end_offset;
        
        if (word_length > 0)
        {
            /* spaces + word too long for current line but ok for next line */
            if ((window->win_chat_cursor_x + word_length_with_spaces > window->win_chat_width)
                && (word_length <= window->win_chat_width - line->length_align))
            {
                gui_curses_display_new_line (window, num_lines, count,
                                             &lines_displayed, simulate);
                /* apply styles before jumping to start of word */
                if (!simulate && (word_start_offset > 0))
                {
                    saved_char = ptr_data[word_start_offset];
                    ptr_data[word_start_offset] = '\0';
                    ptr_style = ptr_data;
                    while ((ptr_style = gui_word_get_next_char (window, (unsigned char *)ptr_style, 1)) != NULL)
                    {
                        /* loop until no style/char available */
                    }
                    ptr_data[word_start_offset] = saved_char;
                }
                /* jump to start of word */
                ptr_data += word_start_offset;
            }
            
            /* display word */
            gui_display_word (window, line, ptr_data,
                              ptr_end_offset,
                              num_lines, count, &lines_displayed, simulate);
            
            if ((!simulate) && (window->win_chat_cursor_y >= window->win_chat_height))
                ptr_data = NULL;
            else
            {
                /* move pointer after end of word */
                ptr_data = ptr_end_offset + 1;
                if (*(ptr_data - 1) == '\0')
                    ptr_data = NULL;
                
                if (window->win_chat_cursor_x == 0)
                {
                    while (ptr_data && (ptr_data[0] == ' '))
                    {
                        next_char = gui_word_get_next_char (window,
                                                            (unsigned char *)ptr_data, 0);
                        if (!next_char)
                            break;
                        prev_char = utf8_prev_char (ptr_data, next_char);
                        if (prev_char && (prev_char[0] == ' '))
                            ptr_data = next_char;
                        else
                            break;
                    }
                }
            }
        }
        else
        {
            gui_curses_display_new_line (window, num_lines, count,
                                         &lines_displayed, simulate);
            ptr_data = NULL;
        }
    }
    
    if (simulate)
    {
        window->win_chat_cursor_x = x;
        window->win_chat_cursor_y = y;
    }
    else
    {
        /* display read marker if needed */
        if (cfg_look_read_marker && cfg_look_read_marker[0] &&
            window->buffer->last_read_line &&
            (window->buffer->last_read_line == line->prev_line))
        {
            gui_window_chat_set_weechat_color (window, COLOR_WIN_CHAT_READ_MARKER);
            mvwprintw (window->win_chat, read_marker_y, read_marker_x,
                       "%c", cfg_look_read_marker[0]);
        }
    }
    
    return lines_displayed;
}

/*
 * gui_calculate_line_diff: returns pointer to line & offset for a difference
 *                          with given line
 */

void
gui_calculate_line_diff (t_gui_window *window, t_gui_line **line, int *line_pos,
                         int difference)
{
    int backward, current_size;
    
    if (!line || !line_pos)
        return;
    
    backward = (difference < 0);
    
    if (!(*line))
    {
        /* if looking backward, start at last line of buffer */
        if (backward)
        {
            *line = window->buffer->last_line;
            if (!(*line))
                return;
            current_size = gui_curses_display_line (window, *line, 0, 1);
            if (current_size == 0)
                current_size = 1;
            *line_pos = current_size - 1;
        }
        /* if looking forward, start at first line of buffer */
        else
        {
            *line = window->buffer->lines;
            if (!(*line))
                return;
            *line_pos = 0;
            current_size = gui_curses_display_line (window, *line, 0, 1);
        }
    }
    else
        current_size = gui_curses_display_line (window, *line, 0, 1);
    
    while ((*line) && (difference != 0))
    {
        /* looking backward */
        if (backward)
        {
            if (*line_pos > 0)
                (*line_pos)--;
            else
            {
                *line = (*line)->prev_line;
                if (*line)
                {
                    current_size = gui_curses_display_line (window, *line, 0, 1);
                    if (current_size == 0)
                        current_size = 1;
                    *line_pos = current_size - 1;
                }
            }
            difference++;
        }
        /* looking forward */
        else
        {
            if (*line_pos < current_size - 1)
                (*line_pos)++;
            else
            {
                *line = (*line)->next_line;
                if (*line)
                {
                    current_size = gui_curses_display_line (window, *line, 0, 1);
                    if (current_size == 0)
                        current_size = 1;
                    *line_pos = 0;
                }
            }
            difference--;
        }
    }
    
    /* first or last line reached */
    if (!(*line))
    {
        if (backward)
        {
            /* first line reached */
            *line = window->buffer->lines;
            *line_pos = 0;
        }
        else
        {
            /* last line reached => consider we'll display all until the end */
            *line_pos = 0;
        }
    }
}

/*
 * gui_draw_buffer_chat: draw chat window for a buffer
 */

void
gui_draw_buffer_chat (t_gui_buffer *buffer, int erase)
{
    t_gui_window *ptr_win;
    t_gui_line *ptr_line;
    t_irc_dcc *dcc_first, *dcc_selected, *ptr_dcc;
    char format_empty[32];
    int i, j, line_pos, count, num_bars;
    char *unit_name[] = { N_("bytes"), N_("Kb"), N_("Mb"), N_("Gb") };
    char *unit_format[] = { "%.0Lf", "%.1Lf", "%.02Lf", "%.02Lf" };
    long unit_divide[] = { 1, 1024, 1024*1024, 1024*1024,1024 };
    int num_unit;
    char format[32], date[128], *buf;
    struct tm *date_tmp;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            if (erase)
            {
                gui_window_set_weechat_color (ptr_win->win_chat, COLOR_WIN_CHAT);
                
                snprintf (format_empty, 32, "%%-%ds", ptr_win->win_chat_width);
                for (i = 0; i < ptr_win->win_chat_height; i++)
                {
                    mvwprintw (ptr_win->win_chat, i, 0, format_empty, " ");
                }
            }
            
            gui_window_set_weechat_color (ptr_win->win_chat, COLOR_WIN_CHAT);
            
            if (buffer->type == BUFFER_TYPE_DCC)
            {
                i = 0;
                dcc_first = (ptr_win->dcc_first) ? (t_irc_dcc *) ptr_win->dcc_first : dcc_list;
                dcc_selected = (ptr_win->dcc_selected) ? (t_irc_dcc *) ptr_win->dcc_selected : dcc_list;
                for (ptr_dcc = dcc_first; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
                {
                    if (i >= ptr_win->win_chat_height - 1)
                        break;
                    
                    /* nickname and filename */
                    gui_window_set_weechat_color (ptr_win->win_chat,
                                                  (ptr_dcc == dcc_selected) ?
                                                  COLOR_DCC_SELECTED : COLOR_WIN_CHAT);
                    mvwprintw (ptr_win->win_chat, i, 0, "%s %-16s ",
                               (ptr_dcc == dcc_selected) ? "***" : "   ",
                               ptr_dcc->nick);
                    buf = channel_iconv_decode (SERVER(buffer),
                                                CHANNEL(buffer),
                                                (DCC_IS_CHAT(ptr_dcc->type)) ?
                                                _(ptr_dcc->filename) : ptr_dcc->filename);
                    wprintw (ptr_win->win_chat, "%s", buf);
                    free (buf);
                    if (DCC_IS_FILE(ptr_dcc->type))
                    {
                        if (ptr_dcc->filename_suffix > 0)
                            wprintw (ptr_win->win_chat, " (.%d)",
                                     ptr_dcc->filename_suffix);
                    }
                    
                    /* status */
                    gui_window_set_weechat_color (ptr_win->win_chat,
                                                  (ptr_dcc == dcc_selected) ?
                                                  COLOR_DCC_SELECTED : COLOR_WIN_CHAT);
                    mvwprintw (ptr_win->win_chat, i + 1, 0, "%s %s ",
                               (ptr_dcc == dcc_selected) ? "***" : "   ",
                               (DCC_IS_RECV(ptr_dcc->type)) ? "-->>" : "<<--");
                    gui_window_set_weechat_color (ptr_win->win_chat,
                                                  COLOR_DCC_WAITING + ptr_dcc->status);
                    buf = channel_iconv_decode (SERVER(buffer),
                                                CHANNEL(buffer),
                                                _(dcc_status_string[ptr_dcc->status]));
                    wprintw (ptr_win->win_chat, "%-10s", buf);
                    free (buf);
                    
                    /* other infos */
                    gui_window_set_weechat_color (ptr_win->win_chat,
                                                  (ptr_dcc == dcc_selected) ?
                                                  COLOR_DCC_SELECTED : COLOR_WIN_CHAT);
                    if (DCC_IS_FILE(ptr_dcc->type))
                    {
                        wprintw (ptr_win->win_chat, "  [");
                        if (ptr_dcc->size == 0)
                            num_bars = 10;
                        else
                            num_bars = (int)((((long double)(ptr_dcc->pos)/(long double)(ptr_dcc->size))*100) / 10);
                        for (j = 0; j < num_bars - 1; j++)
                            wprintw (ptr_win->win_chat, "=");
                        if (num_bars > 0)
                            wprintw (ptr_win->win_chat, ">");
                        for (j = 0; j < 10 - num_bars; j++)
                            wprintw (ptr_win->win_chat, " ");
                        
                        if (ptr_dcc->size < 1024*10)
                            num_unit = 0;
                        else if (ptr_dcc->size < 1024*1024)
                            num_unit = 1;
                        else if (ptr_dcc->size < 1024*1024*1024)
                            num_unit = 2;
                        else
                            num_unit = 3;
                        wprintw (ptr_win->win_chat, "] %3lu%%   ",
                                 (unsigned long)(((long double)(ptr_dcc->pos)/(long double)(ptr_dcc->size))*100));
                        sprintf (format, "%s %%s / %s %%s",
                                 unit_format[num_unit],
                                 unit_format[num_unit]);
                        wprintw (ptr_win->win_chat, format,
                                 ((long double)(ptr_dcc->pos)) / ((long double)(unit_divide[num_unit])),
                                 unit_name[num_unit],
                                 ((long double)(ptr_dcc->size)) / ((long double)(unit_divide[num_unit])),
                                 unit_name[num_unit]);
                        
                        if (ptr_dcc->bytes_per_sec < 1024*1024)
                            num_unit = 1;
                        else if (ptr_dcc->bytes_per_sec < 1024*1024*1024)
                            num_unit = 2;
                        else
                            num_unit = 3;
                        wprintw (ptr_win->win_chat, "  (");
                        if (ptr_dcc->status == DCC_ACTIVE)
                        {
                            wprintw (ptr_win->win_chat, _("ETA"));
                            wprintw (ptr_win->win_chat, ": %.2lu:%.2lu:%.2lu - ",
                                     ptr_dcc->eta / 3600,
                                     (ptr_dcc->eta / 60) % 60,
                                     ptr_dcc->eta % 60);
                        }
                        sprintf (format, "%s %%s/s)", unit_format[num_unit]);
                        buf = channel_iconv_decode (SERVER(buffer),
                                                    CHANNEL(buffer),
                                                    unit_name[num_unit]);
                        wprintw (ptr_win->win_chat, format,
                                 ((long double) ptr_dcc->bytes_per_sec) / ((long double)(unit_divide[num_unit])),
                                 buf);
                        free (buf);
                    }
                    else
                    {
                        date_tmp = localtime (&(ptr_dcc->start_time));
                        strftime (date, sizeof (date) - 1, "%a, %d %b %Y %H:%M:%S", date_tmp);
                        wprintw (ptr_win->win_chat, "  %s", date);
                    }
                    
                    wclrtoeol (ptr_win->win_chat);
                    
                    ptr_win->dcc_last_displayed = ptr_dcc;
                    i += 2;
                }
            }
            else
            {
                ptr_win->win_chat_cursor_x = 0;
                ptr_win->win_chat_cursor_y = 0;
                
                /* display at position of scrolling */
                if (ptr_win->start_line)
                {
                    ptr_line = ptr_win->start_line;
                    line_pos = ptr_win->start_line_pos;
                }
                else
                {
                    /* look for first line to display, starting from last line */
                    ptr_line = NULL;
                    line_pos = 0;
                    gui_calculate_line_diff (ptr_win, &ptr_line, &line_pos,
                                             (-1) * (ptr_win->win_chat_height - 1));
                }

                if (line_pos > 0)
                {
                    /* display end of first line at top of screen */
                    gui_curses_display_line (ptr_win, ptr_line,
                                             gui_curses_display_line (ptr_win,
                                                                      ptr_line,
                                                                      0, 1) -
                                             line_pos, 0);
                    ptr_line = ptr_line->next_line;
                    ptr_win->first_line_displayed = 0;
                }
                else
                    ptr_win->first_line_displayed =
                        (ptr_line == ptr_win->buffer->lines);
                
                /* display lines */
                count = 0;
                while (ptr_line && (ptr_win->win_chat_cursor_y <= ptr_win->win_chat_height - 1))
                {
                    count = gui_curses_display_line (ptr_win, ptr_line, 0, 0);
                    ptr_line = ptr_line->next_line;
                }
                
                ptr_win->scroll = (ptr_win->win_chat_cursor_y > ptr_win->win_chat_height - 1);
                
                /* check if last line of buffer is entirely displayed and scrolling */
                /* if so, disable scroll indicator */
                if (!ptr_line && ptr_win->scroll)
                {
                    if (count == gui_curses_display_line (ptr_win, ptr_win->buffer->last_line, 0, 1))
                    {
                        ptr_win->scroll = 0;
                        ptr_win->start_line = NULL;
                        ptr_win->start_line_pos = 0;
                    }
                }
                
                if (!ptr_win->scroll && (ptr_win->start_line == ptr_win->buffer->lines))
                {
                    ptr_win->start_line = NULL;
                    ptr_win->start_line_pos = 0;
                }
                
                /* cursor is below end line of chat window? */
                if (ptr_win->win_chat_cursor_y > ptr_win->win_chat_height - 1)
                {
                    ptr_win->win_chat_cursor_x = 0;
                    ptr_win->win_chat_cursor_y = ptr_win->win_chat_height - 1;
                }
            }
            wnoutrefresh (ptr_win->win_chat);
            refresh ();
        }
    }
}

/*
 * gui_draw_buffer_chat_line: add a line to chat window for a buffer
 */

void
gui_draw_buffer_chat_line (t_gui_buffer *buffer, t_gui_line *line)
{
    /* This function does nothing in Curses GUI,
       line will be displayed by gui_buffer_draw_chat()  */
    (void) buffer;
    (void) line;
}

/*
 * gui_draw_buffer_nick: draw nick window for a buffer
 */

void
gui_draw_buffer_nick (t_gui_buffer *buffer, int erase)
{
    t_gui_window *ptr_win;
    int i, j, x, y, column, max_length, nicks_displayed;
    char format[32], format_empty[32];
    t_irc_nick *ptr_nick;
    
    if (!gui_ok || !BUFFER_HAS_NICKLIST(buffer))
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            if (erase)
            {
                gui_window_set_weechat_color (ptr_win->win_nick, COLOR_WIN_NICK);
                
                snprintf (format_empty, 32, "%%-%ds", ptr_win->win_nick_width);
                for (i = 0; i < ptr_win->win_nick_height; i++)
                {
                    mvwprintw (ptr_win->win_nick, i, 0, format_empty, " ");
                }
            }
            
            max_length = nick_get_max_length (CHANNEL(buffer));
            if ((buffer->num_displayed > 0) &&
                ((max_length + 2) != ptr_win->win_nick_width))
            {
                gui_calculate_pos_size (ptr_win);
                delwin (ptr_win->win_chat);
                delwin (ptr_win->win_nick);
                ptr_win->win_chat = newwin (ptr_win->win_chat_height,
                                            ptr_win->win_chat_width,
                                            ptr_win->win_chat_y,
                                            ptr_win->win_chat_x);
                ptr_win->win_nick = newwin (ptr_win->win_nick_height,
                                            ptr_win->win_nick_width,
                                            ptr_win->win_nick_y,
                                            ptr_win->win_nick_x);
                gui_draw_buffer_chat (buffer, 1);
                
                gui_window_set_weechat_color (ptr_win->win_nick, COLOR_WIN_NICK);
                
                snprintf (format_empty, 32, "%%-%ds", ptr_win->win_nick_width);
                for (i = 0; i < ptr_win->win_nick_height; i++)
                {
                    mvwprintw (ptr_win->win_nick, i, 0, format_empty, " ");
                }
            }
            snprintf (format, 32, "%%.%ds",
                      ((cfg_look_nicklist_min_size > 0)
                       && (max_length < cfg_look_nicklist_min_size)) ?
                      cfg_look_nicklist_min_size :
                      (((cfg_look_nicklist_max_size > 0)
                        && (max_length > cfg_look_nicklist_max_size)) ?
                       cfg_look_nicklist_max_size : max_length));
            
            if (has_colors ())
            {
                gui_window_set_weechat_color (ptr_win->win_nick, COLOR_WIN_NICK_SEP);
                switch (cfg_look_nicklist_position)
                {
                    case CFG_LOOK_NICKLIST_LEFT:
                        mvwvline (ptr_win->win_nick,
                                  0, ptr_win->win_nick_width - 1, ACS_VLINE,
                                  ptr_win->win_chat_height);
                        break;
                    case CFG_LOOK_NICKLIST_RIGHT:
                        mvwvline (ptr_win->win_nick,
                                  0, 0, ACS_VLINE,
                                  ptr_win->win_chat_height);
                        break;
                    case CFG_LOOK_NICKLIST_TOP:
                        mvwhline (ptr_win->win_nick,
                                  ptr_win->win_nick_height - 1, 0, ACS_HLINE,
                                  ptr_win->win_chat_width);
                        break;
                    case CFG_LOOK_NICKLIST_BOTTOM:
                        mvwhline (ptr_win->win_nick,
                                  0, 0, ACS_HLINE,
                                  ptr_win->win_chat_width);
                        break;
                }
            }
            
            gui_window_set_weechat_color (ptr_win->win_nick, COLOR_WIN_NICK);
            x = 0;
            y = (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM) ? 1 : 0;
            column = 0;
            
            if ((cfg_look_nicklist_position == CFG_LOOK_NICKLIST_TOP) ||
                (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM))
                nicks_displayed = (ptr_win->win_width / (max_length + 2)) * (ptr_win->win_height - 1);
            else
                nicks_displayed = ptr_win->win_nick_height;
            
            ptr_nick = CHANNEL(buffer)->nicks;
            for (i = 0; i < ptr_win->win_nick_start; i++)
            {
                if (!ptr_nick)
                    break;
                ptr_nick = ptr_nick->next_nick;
            }
            if (ptr_nick)
            {
                for (i = 0; i < nicks_displayed; i++)
                {
                    switch (cfg_look_nicklist_position)
                    {
                        case CFG_LOOK_NICKLIST_LEFT:
                            x = 0;
                            break;
                        case CFG_LOOK_NICKLIST_RIGHT:
                            x = 1;
                            break;
                        case CFG_LOOK_NICKLIST_TOP:
                        case CFG_LOOK_NICKLIST_BOTTOM:
                            x = column;
                            break;
                    }
                    if ( ((i == 0) && (ptr_win->win_nick_start > 0))
                         || ((i == nicks_displayed - 1) && (ptr_nick->next_nick)) )
                    {
                        gui_window_set_weechat_color (ptr_win->win_nick, COLOR_WIN_NICK_MORE);
                        j = (max_length + 1) >= 4 ? 4 : max_length + 1;
                        for (x = 1; x <= j; x++)
                            mvwprintw (ptr_win->win_nick, y, x, "+");
                    }
                    else
                    {
                        if (ptr_nick->flags & NICK_CHANOWNER)
                        {
                            gui_window_set_weechat_color (ptr_win->win_nick, COLOR_WIN_NICK_CHANOWNER);
                            mvwprintw (ptr_win->win_nick, y, x, "~");
                            x++;
                        }
                        else if (ptr_nick->flags & NICK_CHANADMIN)
                        {
                            gui_window_set_weechat_color (ptr_win->win_nick, COLOR_WIN_NICK_CHANADMIN);
                            mvwprintw (ptr_win->win_nick, y, x, "&");
                            x++;
                        }
                        else if (ptr_nick->flags & NICK_OP)
                        {
                            gui_window_set_weechat_color (ptr_win->win_nick, COLOR_WIN_NICK_OP);
                            mvwprintw (ptr_win->win_nick, y, x, "@");
                            x++;
                        }
                        else if (ptr_nick->flags & NICK_HALFOP)
                        {
                            gui_window_set_weechat_color (ptr_win->win_nick, COLOR_WIN_NICK_HALFOP);
                            mvwprintw (ptr_win->win_nick, y, x, "%%");
                            x++;
                        }
                        else if (ptr_nick->flags & NICK_VOICE)
                        {
                            gui_window_set_weechat_color (ptr_win->win_nick, COLOR_WIN_NICK_VOICE);
                            mvwprintw (ptr_win->win_nick, y, x, "+");
                            x++;
                        }
                        else
                        {
                            gui_window_set_weechat_color (ptr_win->win_nick, COLOR_WIN_NICK);
                            mvwprintw (ptr_win->win_nick, y, x, " ");
                            x++;
                        }
                        gui_window_set_weechat_color (ptr_win->win_nick,
                                                      ((cfg_irc_away_check > 0) && (ptr_nick->flags & NICK_AWAY)) ?
                                                      COLOR_WIN_NICK_AWAY : COLOR_WIN_NICK);
                        mvwprintw (ptr_win->win_nick, y, x, format, ptr_nick->nick);
                        
                        ptr_nick = ptr_nick->next_nick;
                        
                        if (!ptr_nick)
                            break;
                    }
                    y++;
                    if ((cfg_look_nicklist_position == CFG_LOOK_NICKLIST_TOP) ||
                        (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM))
                    {
                        if (y - ((cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM) ? 1 : 0) >= ptr_win->win_nick_height - 1)
                        {
                            column += max_length + 2;
                            y = (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_TOP) ?
                                0 : 1;
                        }
                    }
                }
            }
        }
        wnoutrefresh (ptr_win->win_nick);
        refresh ();
    }
}

/*
 * gui_draw_buffer_status: draw status window for a buffer
 */

void
gui_draw_buffer_status (t_gui_buffer *buffer, int erase)
{
    t_gui_window *ptr_win;
    t_weechat_hotlist *ptr_hotlist;
    char format[32], str_nicks[32], *more;
    int i, first_mode, x, server_pos, server_total;
    int display_name, names_count;
    
    /* make gcc happy */
    (void) buffer;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (erase)
            gui_curses_window_clear (ptr_win->win_status, COLOR_WIN_STATUS);
        
        gui_window_set_weechat_color (ptr_win->win_status, COLOR_WIN_STATUS);
        
        /* display number of buffers */
        gui_window_set_weechat_color (ptr_win->win_status,
                                      COLOR_WIN_STATUS_DELIMITERS);
        mvwprintw (ptr_win->win_status, 0, 0, "[");
        gui_window_set_weechat_color (ptr_win->win_status,
                                      COLOR_WIN_STATUS);
        wprintw (ptr_win->win_status, "%d",
                 (last_gui_buffer) ? last_gui_buffer->number : 0);
        gui_window_set_weechat_color (ptr_win->win_status,
                                      COLOR_WIN_STATUS_DELIMITERS);
        wprintw (ptr_win->win_status, "] ");
        
        /* display "<servers>" or current server */
        if (ptr_win->buffer->all_servers)
        {
            wprintw (ptr_win->win_status, "[");
            gui_window_set_weechat_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS);
            wprintw (ptr_win->win_status, _("<servers>"));
            gui_window_set_weechat_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
            wprintw (ptr_win->win_status, "] ");
        }
        else if (SERVER(ptr_win->buffer) && SERVER(ptr_win->buffer)->name)
        {
            wprintw (ptr_win->win_status, "[");
            gui_window_set_weechat_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS);
            wprintw (ptr_win->win_status, "%s", SERVER(ptr_win->buffer)->name);
            if (SERVER(ptr_win->buffer)->is_away)
                wprintw (ptr_win->win_status, _("(away)"));
            gui_window_set_weechat_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
            wprintw (ptr_win->win_status, "] ");
        }
        
        /* infos about current buffer */
        if (SERVER(ptr_win->buffer) && !CHANNEL(ptr_win->buffer))
        {
            gui_window_set_weechat_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS);
            wprintw (ptr_win->win_status, "%d",
                     ptr_win->buffer->number);
            gui_window_set_weechat_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
            wprintw (ptr_win->win_status, ":");
            gui_window_set_weechat_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS_CHANNEL);
            if (SERVER(ptr_win->buffer)->is_connected)
                wprintw (ptr_win->win_status, "[%s] ",
                         SERVER(ptr_win->buffer)->name);
            else
                wprintw (ptr_win->win_status, "(%s) ",
                         SERVER(ptr_win->buffer)->name);
            if (ptr_win->buffer->all_servers)
            {
                server_get_number_buffer (SERVER(ptr_win->buffer),
                                          &server_pos,
                                          &server_total);
                gui_window_set_weechat_color (ptr_win->win_status,
                                              COLOR_WIN_STATUS_DELIMITERS);
                wprintw (ptr_win->win_status, "(");
                gui_window_set_weechat_color (ptr_win->win_status,
                                              COLOR_WIN_STATUS);
                wprintw (ptr_win->win_status, "%d", server_pos);
                gui_window_set_weechat_color (ptr_win->win_status,
                                              COLOR_WIN_STATUS_DELIMITERS);
                wprintw (ptr_win->win_status, "/");
                gui_window_set_weechat_color (ptr_win->win_status,
                                              COLOR_WIN_STATUS);
                wprintw (ptr_win->win_status, "%d", server_total);
                gui_window_set_weechat_color (ptr_win->win_status,
                                              COLOR_WIN_STATUS_DELIMITERS);
                wprintw (ptr_win->win_status, ") ");

            }
        }
        if (SERVER(ptr_win->buffer) && CHANNEL(ptr_win->buffer))
        {
            gui_window_set_weechat_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS);
            wprintw (ptr_win->win_status, "%d",
                     ptr_win->buffer->number);
            gui_window_set_weechat_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
            wprintw (ptr_win->win_status, ":");
            gui_window_set_weechat_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS_CHANNEL);
            if ((!CHANNEL(ptr_win->buffer)->nicks)
                && (CHANNEL(ptr_win->buffer)->type != CHANNEL_TYPE_PRIVATE))
                wprintw (ptr_win->win_status, "(%s)",
                         CHANNEL(ptr_win->buffer)->name);
            else
                wprintw (ptr_win->win_status, "%s",
                         CHANNEL(ptr_win->buffer)->name);
            if (ptr_win->buffer == CHANNEL(ptr_win->buffer)->buffer)
            {
                /* display channel modes */
                if (CHANNEL(ptr_win->buffer)->type == CHANNEL_TYPE_CHANNEL)
                {
                    gui_window_set_weechat_color (ptr_win->win_status,
                                                  COLOR_WIN_STATUS_DELIMITERS);
                    wprintw (ptr_win->win_status, "(");
                    gui_window_set_weechat_color (ptr_win->win_status,
                                                  COLOR_WIN_STATUS);
                    i = 0;
                    first_mode = 1;
                    while (CHANNEL(ptr_win->buffer)->modes[i])
                    {
                        if (CHANNEL(ptr_win->buffer)->modes[i] != ' ')
                        {
                            if (first_mode)
                            {
                                wprintw (ptr_win->win_status, "+");
                                first_mode = 0;
                            }
                            wprintw (ptr_win->win_status, "%c",
                                     CHANNEL(ptr_win->buffer)->modes[i]);
                        }
                        i++;
                    }
                    if (CHANNEL(ptr_win->buffer)->modes[CHANNEL_MODE_KEY] != ' ')
                        wprintw (ptr_win->win_status, ",%s",
                                 CHANNEL(ptr_win->buffer)->key);
                    if (CHANNEL(ptr_win->buffer)->modes[CHANNEL_MODE_LIMIT] != ' ')
                        wprintw (ptr_win->win_status, ",%d",
                                 CHANNEL(ptr_win->buffer)->limit);
                    gui_window_set_weechat_color (ptr_win->win_status,
                                                  COLOR_WIN_STATUS_DELIMITERS);
                    wprintw (ptr_win->win_status, ")");
                    gui_window_set_weechat_color (ptr_win->win_status,
                                                  COLOR_WIN_STATUS);
                }
                
                /* display DCC if private is DCC CHAT */
                if ((CHANNEL(ptr_win->buffer)->type == CHANNEL_TYPE_PRIVATE)
                    && (CHANNEL(ptr_win->buffer)->dcc_chat))
                {
                    gui_window_set_weechat_color (ptr_win->win_status,
                                                  COLOR_WIN_STATUS_DELIMITERS);
                    wprintw (ptr_win->win_status, "(");
                    gui_window_set_weechat_color (ptr_win->win_status,
                                                  COLOR_WIN_STATUS_CHANNEL);
                    wprintw (ptr_win->win_status, "DCC");
                    gui_window_set_weechat_color (ptr_win->win_status,
                                                  COLOR_WIN_STATUS_DELIMITERS);
                    wprintw (ptr_win->win_status, ")");
                    gui_window_set_weechat_color (ptr_win->win_status,
                                                  COLOR_WIN_STATUS);
                }
            }
            wprintw (ptr_win->win_status, " ");
        }
        if (!SERVER(ptr_win->buffer))
        {
            gui_window_set_weechat_color (ptr_win->win_status, COLOR_WIN_STATUS);
            wprintw (ptr_win->win_status, "%d",
                     ptr_win->buffer->number);
            gui_window_set_weechat_color (ptr_win->win_status, COLOR_WIN_STATUS_DELIMITERS);
            wprintw (ptr_win->win_status, ":");
            gui_window_set_weechat_color (ptr_win->win_status, COLOR_WIN_STATUS_CHANNEL);
            switch (ptr_win->buffer->type)
            {
                case BUFFER_TYPE_STANDARD:
                    wprintw (ptr_win->win_status, _("[not connected] "));
                    break;
                case BUFFER_TYPE_DCC:
                    wprintw (ptr_win->win_status, "<DCC> ");
                    break;
                case BUFFER_TYPE_RAW_DATA:
                    wprintw (ptr_win->win_status, _("<RAW_IRC> "));
                    break;
            }
        }
        
        /* display list of other active windows (if any) with numbers */
        if (hotlist)
        {
            gui_window_set_weechat_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
            wprintw (ptr_win->win_status, "[");
            gui_window_set_weechat_color (ptr_win->win_status, COLOR_WIN_STATUS);
            wprintw (ptr_win->win_status, _("Act: "));
            
            names_count = 0;
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                switch (ptr_hotlist->priority)
                {
                    case HOTLIST_LOW:
                        gui_window_set_weechat_color (ptr_win->win_status,
                                                      COLOR_WIN_STATUS_DATA_OTHER);
                        display_name = ((cfg_look_hotlist_names_level & 1) != 0);
                        break;
                    case HOTLIST_MSG:
                        gui_window_set_weechat_color (ptr_win->win_status,
                                                      COLOR_WIN_STATUS_DATA_MSG);
                        display_name = ((cfg_look_hotlist_names_level & 2) != 0);
                        break;
                    case HOTLIST_PRIVATE:
                        gui_window_set_weechat_color (ptr_win->win_status,
                                                      COLOR_WIN_STATUS_DATA_PRIVATE);
                        display_name = ((cfg_look_hotlist_names_level & 4) != 0);
                        break;
                    case HOTLIST_HIGHLIGHT:
                        gui_window_set_weechat_color (ptr_win->win_status,
                                                      COLOR_WIN_STATUS_DATA_HIGHLIGHT);
                        display_name = ((cfg_look_hotlist_names_level & 8) != 0);
                        break;
                    default:
                        display_name = 0;
                        break;
                }
                switch (ptr_hotlist->buffer->type)
                {
                    case BUFFER_TYPE_STANDARD:
                        wprintw (ptr_win->win_status, "%d",
                                 ptr_hotlist->buffer->number);
                        
                        if (display_name && (cfg_look_hotlist_names_count != 0)
                            && (names_count < cfg_look_hotlist_names_count))
                        {
                            names_count++;
                            
                            gui_window_set_weechat_color (ptr_win->win_status,
                                                          COLOR_WIN_STATUS_DELIMITERS);
                            wprintw (ptr_win->win_status, ":");
                            
                            gui_window_set_weechat_color (ptr_win->win_status,
                                                          COLOR_WIN_STATUS);
                            if (cfg_look_hotlist_names_length == 0)
                                snprintf (format, sizeof (format) - 1, "%%s");
                            else
                                snprintf (format, sizeof (format) - 1, "%%.%ds", cfg_look_hotlist_names_length);
                            if (BUFFER_IS_SERVER(ptr_hotlist->buffer))
                                wprintw (ptr_win->win_status, format,
                                         (ptr_hotlist->server) ?
                                         ptr_hotlist->server->name :
                                         SERVER(ptr_hotlist->buffer)->name);
                            else if (BUFFER_IS_CHANNEL(ptr_hotlist->buffer)
                                     || BUFFER_IS_PRIVATE(ptr_hotlist->buffer))
                                wprintw (ptr_win->win_status, format, CHANNEL(ptr_hotlist->buffer)->name);
                        }
                        break;
                    case BUFFER_TYPE_DCC:
                        wprintw (ptr_win->win_status, "%d",
                                 ptr_hotlist->buffer->number);
                        gui_window_set_weechat_color (ptr_win->win_status,
                                                      COLOR_WIN_STATUS_DELIMITERS);
                        wprintw (ptr_win->win_status, ":");
                        gui_window_set_weechat_color (ptr_win->win_status,
                                                      COLOR_WIN_STATUS);
                        wprintw (ptr_win->win_status, "DCC");
                        break;
                    case BUFFER_TYPE_RAW_DATA:
                        wprintw (ptr_win->win_status, "%d",
                                 ptr_hotlist->buffer->number);
                        gui_window_set_weechat_color (ptr_win->win_status,
                                                      COLOR_WIN_STATUS_DELIMITERS);
                        wprintw (ptr_win->win_status, ":");
                        gui_window_set_weechat_color (ptr_win->win_status,
                                                      COLOR_WIN_STATUS);
                        wprintw (ptr_win->win_status, _("RAW_IRC"));
                        break;
                }
                
                if (ptr_hotlist->next_hotlist)
                    wprintw (ptr_win->win_status, ",");
            }
            gui_window_set_weechat_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
            wprintw (ptr_win->win_status, "] ");
        }
        
        /* display lag */
        if (SERVER(ptr_win->buffer))
        {
            if (SERVER(ptr_win->buffer)->lag / 1000 >= cfg_irc_lag_min_show)
            {
                gui_window_set_weechat_color (ptr_win->win_status,
                                              COLOR_WIN_STATUS_DELIMITERS);
                wprintw (ptr_win->win_status, "[");
                gui_window_set_weechat_color (ptr_win->win_status, COLOR_WIN_STATUS);
                wprintw (ptr_win->win_status, _("Lag: %.1f"),
                         ((float)(SERVER(ptr_win->buffer)->lag)) / 1000);
                gui_window_set_weechat_color (ptr_win->win_status,
                                              COLOR_WIN_STATUS_DELIMITERS);
                wprintw (ptr_win->win_status, "]");
            }
        }
        
        /* display "-MORE-" (if last line is not displayed) & nicks count */
        if (BUFFER_HAS_NICKLIST(ptr_win->buffer))
        {
            snprintf (str_nicks, sizeof (str_nicks) - 1, "%d", CHANNEL(ptr_win->buffer)->nicks_count);
            x = ptr_win->win_width - strlen (str_nicks) - 4;
        }
        else
            x = ptr_win->win_width - 2;
        more = strdup (_("-MORE-"));
        x -= strlen (more) - 1;
        if (x < 0)
            x = 0;
        gui_window_set_weechat_color (ptr_win->win_status, COLOR_WIN_STATUS_MORE);
        if (ptr_win->scroll)
            mvwprintw (ptr_win->win_status, 0, x, "%s", more);
        else
        {
            snprintf (format, sizeof (format) - 1, "%%-%ds", (int)(strlen (more)));
            mvwprintw (ptr_win->win_status, 0, x, format, " ");
        }
        if (BUFFER_HAS_NICKLIST(ptr_win->buffer))
        {
            gui_window_set_weechat_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
            wprintw (ptr_win->win_status, " [");
            gui_window_set_weechat_color (ptr_win->win_status, COLOR_WIN_STATUS);
            wprintw (ptr_win->win_status, "%s", str_nicks);
            gui_window_set_weechat_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
            wprintw (ptr_win->win_status, "]");
        }
        free (more);
        
        wnoutrefresh (ptr_win->win_status);
        refresh ();
    }
}

/*
 * gui_draw_buffer_infobar_time: draw time in infobar window
 */

void
gui_draw_buffer_infobar_time (t_gui_buffer *buffer)
{
    t_gui_window *ptr_win;
    time_t time_seconds;
    struct tm *local_time;
    
    /* make gcc happy */
    (void) buffer;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {        
        time_seconds = time (NULL);
        local_time = localtime (&time_seconds);
        if (local_time)
        {
            gui_window_set_weechat_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR);
            mvwprintw (ptr_win->win_infobar,
                       0, 1,
                       "%02d:%02d",
                       local_time->tm_hour, local_time->tm_min);
            if (cfg_look_infobar_seconds)
                wprintw (ptr_win->win_infobar,
                         ":%02d",
                         local_time->tm_sec);
        }
        wnoutrefresh (ptr_win->win_infobar);
    }
}

/*
 * gui_draw_buffer_infobar: draw infobar window for a buffer
 */

void
gui_draw_buffer_infobar (t_gui_buffer *buffer, int erase)
{
    t_gui_window *ptr_win;
    time_t time_seconds;
    struct tm *local_time;
    char text_time[1024 + 1];
    
    /* make gcc happy */
    (void) buffer;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (erase)
            gui_curses_window_clear (ptr_win->win_infobar, COLOR_WIN_INFOBAR);
        
        gui_window_set_weechat_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR);
        
        time_seconds = time (NULL);
        local_time = localtime (&time_seconds);
        if (local_time)
        {
            strftime (text_time, 1024, cfg_look_infobar_timestamp, local_time);
            gui_window_set_weechat_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR_DELIMITERS);
            wprintw (ptr_win->win_infobar, "[");
            gui_window_set_weechat_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR);
            wprintw (ptr_win->win_infobar,
                     "%02d:%02d",
                     local_time->tm_hour, local_time->tm_min);
            if (cfg_look_infobar_seconds)
                wprintw (ptr_win->win_infobar,
                         ":%02d",
                         local_time->tm_sec);
            gui_window_set_weechat_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR_DELIMITERS);
            wprintw (ptr_win->win_infobar, "]");
            gui_window_set_weechat_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR);
            wprintw (ptr_win->win_infobar,
                     " %s", text_time);
        }
        if (gui_infobar)
        {
            gui_window_set_weechat_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR_DELIMITERS);
            wprintw (ptr_win->win_infobar, " | ");
            gui_window_set_weechat_color (ptr_win->win_infobar, gui_infobar->color);
            wprintw (ptr_win->win_infobar, "%s", gui_infobar->text);
        }
        
        wnoutrefresh (ptr_win->win_infobar);
        refresh ();
    }
}

/*
 * gui_get_input_width: return input width (max # chars displayed)
 */

int
gui_get_input_width (t_gui_window *window, char *nick)
{
    if (CHANNEL(window->buffer))
        return (window->win_width - strlen (CHANNEL(window->buffer)->name) -
                strlen (nick) - 4);
    else
        return (window->win_width - strlen (nick) - 3);
}

/*
 * gui_draw_buffer_input: draw input window for a buffer
 */

void
gui_draw_buffer_input (t_gui_buffer *buffer, int erase)
{
    t_gui_window *ptr_win;
    char format[32];
    char *ptr_nickname;
    int input_width;
    t_irc_dcc *dcc_selected;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            if (erase)
                gui_curses_window_clear (ptr_win->win_input, COLOR_WIN_INPUT);
            
            switch (buffer->type)
            {
                case BUFFER_TYPE_STANDARD:
                    if (buffer->has_input)
                    {
                        if (buffer->input_buffer_length == 0)
                            buffer->input_buffer[0] = '\0';
                        
                        if (SERVER(buffer))
                            ptr_nickname = (SERVER(buffer)->nick) ?
                                SERVER(buffer)->nick : SERVER(buffer)->nick1;
                        else
                            ptr_nickname = cfg_look_no_nickname;
                        input_width = gui_get_input_width (ptr_win, ptr_nickname);
                        
                        if (buffer->input_buffer_pos - buffer->input_buffer_1st_display + 1 >
                            input_width)
                            buffer->input_buffer_1st_display = buffer->input_buffer_pos -
                                input_width + 1;
                        else
                        {
                            if (buffer->input_buffer_pos < buffer->input_buffer_1st_display)
                                buffer->input_buffer_1st_display = buffer->input_buffer_pos;
                            else
                            {
                                if ((buffer->input_buffer_1st_display > 0) &&
                                    (buffer->input_buffer_pos -
                                     buffer->input_buffer_1st_display + 1) < input_width)
                                {
                                    buffer->input_buffer_1st_display =
                                        buffer->input_buffer_pos - input_width + 1;
                                    if (buffer->input_buffer_1st_display < 0)
                                        buffer->input_buffer_1st_display = 0;
                                }
                            }
                        }
                        if (CHANNEL(buffer))
                        {
                            gui_window_set_weechat_color (ptr_win->win_input, COLOR_WIN_INPUT_DELIMITERS);
                            mvwprintw (ptr_win->win_input, 0, 0, "[");
                            gui_window_set_weechat_color (ptr_win->win_input, COLOR_WIN_INPUT_CHANNEL);
                            wprintw (ptr_win->win_input, "%s ", CHANNEL(buffer)->name);
                            gui_window_set_weechat_color (ptr_win->win_input, COLOR_WIN_INPUT_NICK);
                            wprintw (ptr_win->win_input, "%s", ptr_nickname);
                            gui_window_set_weechat_color (ptr_win->win_input, COLOR_WIN_INPUT_DELIMITERS);
                            wprintw (ptr_win->win_input, "] ");
                            gui_window_set_weechat_color (ptr_win->win_input, COLOR_WIN_INPUT);
                            snprintf (format, 32, "%%-%ds", input_width);
                            if (ptr_win == gui_current_window)
                                wprintw (ptr_win->win_input, format,
                                         utf8_add_offset (buffer->input_buffer,
                                                          buffer->input_buffer_1st_display));
                            else
                                wprintw (ptr_win->win_input, format, "");
                            wclrtoeol (ptr_win->win_input);
                            ptr_win->win_input_x = utf8_strlen (CHANNEL(buffer)->name) +
                                utf8_strlen (SERVER(buffer)->nick) + 4 +
                                (buffer->input_buffer_pos - buffer->input_buffer_1st_display);
                            if (ptr_win == gui_current_window)
                                move (ptr_win->win_y + ptr_win->win_height - 1,
                                      ptr_win->win_x + ptr_win->win_input_x);
                        }
                        else
                        {
                            gui_window_set_weechat_color (ptr_win->win_input, COLOR_WIN_INPUT_DELIMITERS);
                            mvwprintw (ptr_win->win_input, 0, 0, "[");
                            gui_window_set_weechat_color (ptr_win->win_input, COLOR_WIN_INPUT_NICK);
                            wprintw (ptr_win->win_input, "%s", ptr_nickname);
                            gui_window_set_weechat_color (ptr_win->win_input, COLOR_WIN_INPUT_DELIMITERS);
                            wprintw (ptr_win->win_input, "] ");
                            gui_window_set_weechat_color (ptr_win->win_input, COLOR_WIN_INPUT);
                            snprintf (format, 32, "%%-%ds", input_width);
                            if (ptr_win == gui_current_window)
                                wprintw (ptr_win->win_input, format,
                                         utf8_add_offset (buffer->input_buffer,
                                                          buffer->input_buffer_1st_display));
                            else
                                wprintw (ptr_win->win_input, format, "");
                            wclrtoeol (ptr_win->win_input);
                            ptr_win->win_input_x = utf8_strlen (ptr_nickname) + 3 +
                                (buffer->input_buffer_pos - buffer->input_buffer_1st_display);
                            if (ptr_win == gui_current_window)
                                move (ptr_win->win_y + ptr_win->win_height - 1,
                                      ptr_win->win_x + ptr_win->win_input_x);
                        }
                    }
                    break;
                case BUFFER_TYPE_DCC:
                    dcc_selected = (ptr_win->dcc_selected) ? (t_irc_dcc *) ptr_win->dcc_selected : dcc_list;
                    wmove (ptr_win->win_input, 0, 0);
                    if (dcc_selected)
                    {
                        switch (dcc_selected->status)
                        {
                            case DCC_WAITING:
                                if (DCC_IS_RECV(dcc_selected->type))
                                    wprintw (ptr_win->win_input, _("  [A] Accept"));
                                wprintw (ptr_win->win_input, _("  [C] Cancel"));
                                break;
                            case DCC_CONNECTING:
                            case DCC_ACTIVE:
                                wprintw (ptr_win->win_input, _("  [C] Cancel"));
                                break;
                            case DCC_DONE:
                            case DCC_FAILED:
                            case DCC_ABORTED:
                                wprintw (ptr_win->win_input, _("  [R] Remove"));
                                break;
                        }
                    }
                    wprintw (ptr_win->win_input, _("  [P] Purge old DCC"));
                    wprintw (ptr_win->win_input, _("  [Q] Close DCC view"));
                    wclrtoeol (ptr_win->win_input);
                    ptr_win->win_input_x = 0;
                    if (ptr_win == gui_current_window)
                        move (ptr_win->win_y + ptr_win->win_height - 1,
                              ptr_win->win_x);
                    break;
                case BUFFER_TYPE_RAW_DATA:
                    mvwprintw (ptr_win->win_input, 0, 0, _("  [Q] Close raw data view"));
                    wclrtoeol (ptr_win->win_input);
                    ptr_win->win_input_x = 0;
                    if (ptr_win == gui_current_window)
                        move (ptr_win->win_y + ptr_win->win_height - 1,
                              ptr_win->win_x);
                    break;
            }
            doupdate ();
            wrefresh (ptr_win->win_input);
            refresh ();
        }
    }
}

/*
 * gui_redraw_buffer: redraw a buffer
 */

void
gui_redraw_buffer (t_gui_buffer *buffer)
{
    t_gui_window *ptr_win;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            gui_draw_buffer_title (buffer, 1);
            gui_draw_buffer_chat (buffer, 1);
            if (ptr_win->win_nick)
                gui_draw_buffer_nick (buffer, 1);
            gui_draw_buffer_status (buffer, 1);
            if (cfg_look_infobar)
                gui_draw_buffer_infobar (buffer, 1);
            gui_draw_buffer_input (buffer, 1);
        }
    }
}

/*
 * gui_switch_to_buffer: switch to another buffer
 */

void
gui_switch_to_buffer (t_gui_window *window, t_gui_buffer *buffer)
{
    if (!gui_ok)
        return;
    
    if (window->buffer->num_displayed > 0)
        window->buffer->num_displayed--;
    
    if (window->buffer != buffer)
    {
        window->buffer->last_read_line = window->buffer->last_line;
        if (buffer->last_read_line == buffer->last_line)
            buffer->last_read_line = NULL;
    }
    
    window->buffer = buffer;
    window->win_nick_start = 0;
    gui_calculate_pos_size (window);
    
    /* destroy Curses windows */
    if (window->win_title)
    {
        delwin (window->win_title);
        window->win_title = NULL;
    }
    if (window->win_nick)
    {
        delwin (window->win_nick);
        window->win_nick = NULL;
    }
    if (window->win_status)
    {
        delwin (window->win_status);
        window->win_status = NULL;
    }
    if (window->win_infobar)
    {
        delwin (window->win_infobar);
        window->win_infobar = NULL;
    }
    if (window->win_input)
    {
        delwin (window->win_input);
        window->win_input = NULL;
    }
        
    /* create Curses windows */
    window->win_title = newwin (1,
                                window->win_width,
                                window->win_y,
                                window->win_x);
    window->win_input = newwin (1,
                                window->win_width,
                                window->win_y + window->win_height - 1,
                                window->win_x);
    if (BUFFER_IS_CHANNEL(buffer))
    {
        if (window->win_chat)
            delwin (window->win_chat);
        window->win_chat = newwin (window->win_chat_height,
                                   window->win_chat_width,
                                   window->win_chat_y,
                                   window->win_chat_x);
        if (cfg_look_nicklist)
            window->win_nick = newwin (window->win_nick_height,
                                       window->win_nick_width,
                                       window->win_nick_y,
                                       window->win_nick_x);
        else
            window->win_nick = NULL;
    }
    if (!(BUFFER_IS_CHANNEL(buffer)))
    {
        if (window->win_chat)
            delwin (window->win_chat);
        window->win_chat = newwin (window->win_chat_height,
                                   window->win_chat_width,
                                   window->win_chat_y,
                                   window->win_chat_x);
    }
    
    /* create status/infobar windows */
    if (cfg_look_infobar)
    {
        window->win_infobar = newwin (1, window->win_width,
                                      window->win_y + window->win_height - 2,
                                      window->win_x);
        window->win_status = newwin (1, window->win_width,
                                     window->win_y + window->win_height - 3,
                                     window->win_x);
    }
    else
        window->win_status = newwin (1, window->win_width,
                                     window->win_y + window->win_height - 2,
                                     window->win_x);
    
    window->start_line = NULL;
    window->start_line_pos = 0;
    
    buffer->num_displayed++;
    
    hotlist_remove_buffer (buffer);
}

/*
 * gui_window_page_up: display previous page on buffer
 */

void
gui_window_page_up (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (!window->first_line_displayed)
    {
        gui_calculate_line_diff (window, &window->start_line,
                                 &window->start_line_pos,
                                 (window->start_line) ?
                                 (-1) * (window->win_chat_height - 1) :
                                 (-1) * ((window->win_chat_height - 1) * 2));
        gui_draw_buffer_chat (window->buffer, 0);
        gui_draw_buffer_status (window->buffer, 0);
    }
}

/*
 * gui_window_page_down: display next page on buffer
 */

void
gui_window_page_down (t_gui_window *window)
{
    t_gui_line *ptr_line;
    int line_pos;
    
    if (!gui_ok)
        return;
    
    if (window->start_line)
    {
        gui_calculate_line_diff (window, &window->start_line,
                                 &window->start_line_pos,
                                 window->win_chat_height - 1);
        
        /* check if we can display all */
        ptr_line = window->start_line;
        line_pos = window->start_line_pos;
        gui_calculate_line_diff (window, &ptr_line,
                                 &line_pos,
                                 window->win_chat_height - 1);
        if (!ptr_line)
        {
            window->start_line = NULL;
            window->start_line_pos = 0;
        }
        
        gui_draw_buffer_chat (window->buffer, 0);
        gui_draw_buffer_status (window->buffer, 0);
    }
}

/*
 * gui_window_scroll_up: display previous few lines in buffer
 */

void
gui_window_scroll_up (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (!window->first_line_displayed)
    {
        gui_calculate_line_diff (window, &window->start_line,
                                 &window->start_line_pos,
                                 (window->start_line) ?
                                 (-1) * cfg_look_scroll_amount :
                                 (-1) * ( (window->win_chat_height - 1) + cfg_look_scroll_amount));
        gui_draw_buffer_chat (window->buffer, 0);
        gui_draw_buffer_status (window->buffer, 0);
    }
}

/*
 * gui_window_scroll_down: display next few lines in buffer
 */

void
gui_window_scroll_down (t_gui_window *window)
{
    t_gui_line *ptr_line;
    int line_pos;
    
    if (!gui_ok)
        return;
    
    if (window->start_line)
    {
        gui_calculate_line_diff (window, &window->start_line,
                                 &window->start_line_pos,
                                 cfg_look_scroll_amount);
        
        /* check if we can display all */
        ptr_line = window->start_line;
        line_pos = window->start_line_pos;
        gui_calculate_line_diff (window, &ptr_line,
                                 &line_pos,
                                 window->win_chat_height - 1);
        
        if (!ptr_line)
        {
            window->start_line = NULL;
            window->start_line_pos = 0;
        }
        
        gui_draw_buffer_chat (window->buffer, 0);
        gui_draw_buffer_status (window->buffer, 0);
    }
}

/*
 * gui_window_scroll_top: scroll to top of buffer
 */

void
gui_window_scroll_top (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (!window->first_line_displayed)
    {
        window->start_line = window->buffer->lines;
        window->start_line_pos = 0;
        gui_draw_buffer_chat (window->buffer, 0);
        gui_draw_buffer_status (window->buffer, 0);
    }
}

/*
 * gui_window_scroll_bottom: scroll to bottom of buffer
 */

void
gui_window_scroll_bottom (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (window->start_line)
    {
        window->start_line = NULL;
        window->start_line_pos = 0;
        gui_draw_buffer_chat (window->buffer, 0);
        gui_draw_buffer_status (window->buffer, 0);
    }
}

/*
 * gui_window_nick_beginning: go to beginning of nicklist
 */

void
gui_window_nick_beginning (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (BUFFER_HAS_NICKLIST(window->buffer))
    {
        if (window->win_nick_start > 0)
        {
            window->win_nick_start = 0;
            gui_draw_buffer_nick (window->buffer, 1);
        }
    }
}

/*
 * gui_window_nick_end: go to the end of nicklist
 */

void
gui_window_nick_end (t_gui_window *window)
{
    int new_start;
    
    if (!gui_ok)
        return;
    
    if (BUFFER_HAS_NICKLIST(window->buffer))
    {
        new_start =
            CHANNEL(window->buffer)->nicks_count - window->win_nick_height;
        if (new_start < 0)
            new_start = 0;
        else if (new_start >= 1)
            new_start++;
        
        if (new_start != window->win_nick_start)
        {
            window->win_nick_start = new_start;
            gui_draw_buffer_nick (window->buffer, 1);
        }
    }
}

/*
 * gui_window_nick_page_up: scroll one page up in nicklist
 */

void
gui_window_nick_page_up (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (BUFFER_HAS_NICKLIST(window->buffer))
    {
        if (window->win_nick_start > 0)
        {
            window->win_nick_start -= (window->win_nick_height - 1);
            if (window->win_nick_start <= 1)
                window->win_nick_start = 0;
            gui_draw_buffer_nick (window->buffer, 1);
        }
    }
}

/*
 * gui_window_nick_page_down: scroll one page down in nicklist
 */

void
gui_window_nick_page_down (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (BUFFER_HAS_NICKLIST(window->buffer))
    {
        if ((CHANNEL(window->buffer)->nicks_count > window->win_nick_height)
            && (window->win_nick_start + window->win_nick_height - 1
                < CHANNEL(window->buffer)->nicks_count))
        {
            if (window->win_nick_start == 0)
                window->win_nick_start += (window->win_nick_height - 1);
            else
                window->win_nick_start += (window->win_nick_height - 2);
            gui_draw_buffer_nick (window->buffer, 1);
        }
    }
}

/*
 * gui_window_init_subwindows: init subviews for a WeeChat window
 */

void
gui_window_init_subwindows (t_gui_window *window)
{
    window->win_title = NULL;
    window->win_chat = NULL;
    window->win_nick = NULL;
    window->win_status = NULL;
    window->win_infobar = NULL;
    window->win_input = NULL;
}

/*
 * gui_window_auto_resize: auto-resize all windows, according to % of global size
 *                         This function is called after a terminal resize.
 *                         Returns 0 if ok, -1 if all window should be merged
 *                         (not enough space according to windows %)
 */

int
gui_window_auto_resize (t_gui_window_tree *tree,
                        int x, int y, int width, int height,
                        int simulate)
{
    int size1, size2;
    
    if (tree)
    {
        if (tree->window)
        {
            if ((width < WINDOW_MIN_WIDTH) || (height < WINDOW_MIN_HEIGHT))
                return -1;
            if (!simulate)
            {
                tree->window->win_x = x;
                tree->window->win_y = y;
                tree->window->win_width = width;
                tree->window->win_height = height;
            }
        }
        else
        {
            if (tree->split_horiz)
            {
                size1 = (height * tree->split_pct) / 100;
                size2 = height - size1;
                if (gui_window_auto_resize (tree->child1, x, y + size1,
                                            width, size2, simulate) < 0)
                    return -1;
                if (gui_window_auto_resize (tree->child2, x, y,
                                            width, size1, simulate) < 0)
                    return -1;
            }
            else
            {
                size1 = (width * tree->split_pct) / 100;
                size2 = width - size1 - 1;
                if (gui_window_auto_resize (tree->child1, x, y,
                                            size1, height, simulate) < 0)
                    return -1;
                if (gui_window_auto_resize (tree->child2, x + size1 + 1, y,
                                            size2, height, simulate) < 0)
                    return -1;
            }
        }
    }
    return 0;
}

/*
 * gui_refresh_windows: auto resize and refresh all windows
 */

void
gui_refresh_windows ()
{
    t_gui_window *ptr_win, *old_current_window;
    
    if (gui_ok)
    {
        old_current_window = gui_current_window;
        
        if (gui_window_auto_resize (gui_windows_tree, 0, 0, COLS, LINES, 0) < 0)
            gui_window_merge_all (gui_current_window);
    
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            gui_switch_to_buffer (ptr_win, ptr_win->buffer);
            gui_redraw_buffer (ptr_win->buffer);
            gui_draw_window_separator (ptr_win);
        }
        
        gui_current_window = old_current_window;
        gui_switch_to_buffer (gui_current_window, gui_current_window->buffer);
        gui_redraw_buffer (gui_current_window->buffer);
    }
}

/*
 * gui_window_split_horiz: split a window horizontally
 */

void
gui_window_split_horiz (t_gui_window *window, int pourcentage)
{
    t_gui_window *new_window;
    int height1, height2;
    
    if (!gui_ok)
        return;
    
    height1 = (window->win_height * pourcentage) / 100;
    height2 = window->win_height - height1;
    
    if ((height1 >= WINDOW_MIN_HEIGHT) && (height2 >= WINDOW_MIN_HEIGHT)
        && (pourcentage > 0) && (pourcentage <= 100))
    {
        if ((new_window = gui_window_new (window,
                                          window->win_x, window->win_y,
                                          window->win_width, height1,
                                          100, pourcentage)))
        {
            /* reduce old window height (bottom window) */
            window->win_y = new_window->win_y + new_window->win_height;
            window->win_height = height2;
            window->win_height_pct = 100 - pourcentage;
            
            /* assign same buffer for new window (top window) */
            new_window->buffer = window->buffer;
            new_window->buffer->num_displayed++;
            
            gui_switch_to_buffer (window, window->buffer);
            
            gui_current_window = new_window;
            gui_switch_to_buffer (gui_current_window, gui_current_window->buffer);
            gui_redraw_buffer (gui_current_window->buffer);
        }
    }
}

/*
 * gui_window_split_vertic: split a window vertically
 */

void
gui_window_split_vertic (t_gui_window *window, int pourcentage)
{
    t_gui_window *new_window;
    int width1, width2;
    
    if (!gui_ok)
        return;
    
    width1 = (window->win_width * pourcentage) / 100;
    width2 = window->win_width - width1 - 1;
    
    if ((width1 >= WINDOW_MIN_WIDTH) && (width2 >= WINDOW_MIN_WIDTH)
        && (pourcentage > 0) && (pourcentage <= 100))
    {
        if ((new_window = gui_window_new (window,
                                          window->win_x + width1 + 1, window->win_y,
                                          width2, window->win_height,
                                          pourcentage, 100)))
        {
            /* reduce old window height (left window) */
            window->win_width = width1;
            window->win_width_pct = 100 - pourcentage;
            
            /* assign same buffer for new window (right window) */
            new_window->buffer = window->buffer;
            new_window->buffer->num_displayed++;
            
            gui_switch_to_buffer (window, window->buffer);
            
            gui_current_window = new_window;
            gui_switch_to_buffer (gui_current_window, gui_current_window->buffer);
            gui_redraw_buffer (gui_current_window->buffer);
            
            /* create & draw separator */
            gui_draw_window_separator (gui_current_window);
        }
    }
}

/*
 * gui_window_resize: resize window
 */

void
gui_window_resize (t_gui_window *window, int pourcentage)
{
    t_gui_window_tree *parent;
    int old_split_pct;
    
    parent = window->ptr_tree->parent_node;
    if (parent)
    {
        old_split_pct = parent->split_pct;
        if (((parent->split_horiz) && (window->ptr_tree == parent->child2))
            || ((!(parent->split_horiz)) && (window->ptr_tree == parent->child1)))
            parent->split_pct = pourcentage;
        else
            parent->split_pct = 100 - pourcentage;
        if (gui_window_auto_resize (gui_windows_tree, 0, 0, COLS, LINES, 1) < 0)
            parent->split_pct = old_split_pct;
        else
            gui_refresh_windows ();
    }
}

/*
 * gui_window_merge: merge window with its sister
 */

int
gui_window_merge (t_gui_window *window)
{
    t_gui_window_tree *parent, *sister;
    
    parent = window->ptr_tree->parent_node;
    if (parent)
    {
        sister = (parent->child1->window == window) ?
            parent->child2 : parent->child1;
        
        if (!(sister->window))
            return 0;
        
        if (window->win_y == sister->window->win_y)
        {
            /* horizontal merge */
            window->win_width += sister->window->win_width + 1;
            window->win_width_pct += sister->window->win_width_pct;
        }
        else
        {
            /* vertical merge */
            window->win_height += sister->window->win_height;
            window->win_height_pct += sister->window->win_height_pct;
        }
        if (sister->window->win_x < window->win_x)
            window->win_x = sister->window->win_x;
        if (sister->window->win_y < window->win_y)
            window->win_y = sister->window->win_y;
        
        gui_window_free (sister->window);
        gui_window_tree_node_to_leaf (parent, window);
        
        gui_switch_to_buffer (window, window->buffer);
        gui_redraw_buffer (window->buffer);
        return 1;
    }
    return 0;
}

/*
 * gui_window_merge_all: merge all windows into only one
 */

void
gui_window_merge_all (t_gui_window *window)
{
    while (gui_windows->next_window)
    {
        gui_window_free ((gui_windows == window) ? gui_windows->next_window : gui_windows);
    }
    gui_window_tree_free (&gui_windows_tree);
    gui_window_tree_init (window);
    window->ptr_tree = gui_windows_tree;
    window->win_x = 0;
    window->win_y = 0;
    window->win_width = COLS;
    window->win_height = LINES;
    window->win_width_pct = 100;
    window->win_height_pct = 100;
    gui_switch_to_buffer (window, window->buffer);
    gui_redraw_buffer (window->buffer);
}

/*
 * gui_window_side_by_side: return a code about position of 2 windows:
 *                          0 = they're not side by side
 *                          1 = side by side (win2 is over the win1)
 *                          2 = side by side (win2 on the right)
 *                          3 = side by side (win2 below win1)
 *                          4 = side by side (win2 on the left)
 */

int
gui_window_side_by_side (t_gui_window *win1, t_gui_window *win2)
{
    /* win2 over win1 ? */
    if (win2->win_y + win2->win_height == win1->win_y)
    {
        if (win2->win_x >= win1->win_x + win1->win_width)
            return 0;
        if (win2->win_x + win2->win_width <= win1->win_x)
            return 0;
        return 1;
    }

    /* win2 on the right ? */
    if (win2->win_x == win1->win_x + win1->win_width + 1)
    {
        if (win2->win_y >= win1->win_y + win1->win_height)
            return 0;
        if (win2->win_y + win2->win_height <= win1->win_y)
            return 0;
        return 2;
    }

    /* win2 below win1 ? */
    if (win2->win_y == win1->win_y + win1->win_height)
    {
        if (win2->win_x >= win1->win_x + win1->win_width)
            return 0;
        if (win2->win_x + win2->win_width <= win1->win_x)
            return 0;
        return 3;
    }
    
    /* win2 on the left ? */
    if (win2->win_x + win2->win_width + 1 == win1->win_x)
    {
        if (win2->win_y >= win1->win_y + win1->win_height)
            return 0;
        if (win2->win_y + win2->win_height <= win1->win_y)
            return 0;
        return 4;
    }

    return 0;
}

/*
 * gui_window_switch_up: search and switch to a window over current window
 */

void
gui_window_switch_up (t_gui_window *window)
{
    t_gui_window *ptr_win;
    
    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 1))
        {
            gui_current_window = ptr_win;
            gui_switch_to_buffer (gui_current_window, gui_current_window->buffer);
            gui_redraw_buffer (gui_current_window->buffer);
            return;
        }
    }
}

/*
 * gui_window_switch_down: search and switch to a window below current window
 */

void
gui_window_switch_down (t_gui_window *window)
{
    t_gui_window *ptr_win;
    
    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 3))
        {
            gui_current_window = ptr_win;
            gui_switch_to_buffer (gui_current_window, gui_current_window->buffer);
            gui_redraw_buffer (gui_current_window->buffer);
            return;
        }
    }
}

/*
 * gui_window_switch_left: search and switch to a window on the left of current window
 */

void
gui_window_switch_left (t_gui_window *window)
{
    t_gui_window *ptr_win;
    
    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 4))
        {
            gui_current_window = ptr_win;
            gui_switch_to_buffer (gui_current_window, gui_current_window->buffer);
            gui_redraw_buffer (gui_current_window->buffer);
            return;
        }
    }
}

/*
 * gui_window_switch_right: search and switch to a window on the right of current window
 */

void
gui_window_switch_right (t_gui_window *window)
{
    t_gui_window *ptr_win;
    
    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 2))
        {
            gui_current_window = ptr_win;
            gui_switch_to_buffer (gui_current_window, gui_current_window->buffer);
            gui_redraw_buffer (gui_current_window->buffer);
            return;
        }
    }
}

/*
 * gui_refresh_screen: called when term size is modified
 */

void
gui_refresh_screen ()
{
    int new_height, new_width;
    
    endwin ();
    refresh ();
    
    getmaxyx (stdscr, new_height, new_width);
    
    gui_ok = ((new_width > WINDOW_MIN_WIDTH) && (new_height > WINDOW_MIN_HEIGHT));
    
    if (gui_ok)
        gui_refresh_windows ();
}

/*
 * gui_refresh_screen_sigwinch: called when signal SIGWINCH is received
 */

void
gui_refresh_screen_sigwinch ()
{
    gui_refresh_screen ();
    signal (SIGWINCH, gui_refresh_screen_sigwinch);
}

/*
 * gui_pre_init: pre-initialize GUI (called before gui_init)
 */

void
gui_pre_init (int *argc, char **argv[])
{
    /* nothing for Curses interface */
    (void) argc;
    (void) argv;
}

/*
 * gui_init_color_pairs: init color pairs
 */

void
gui_init_color_pairs ()
{
    int i;
    char shift_colors[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };
    
    if (has_colors ())
    {
        for (i = 1; i < COLOR_PAIRS; i++)
            init_pair (i, shift_colors[i % 8], (i < 8) ? -1 : shift_colors[i / 8]);
        
        /* disable white on white, replaced by black on white */
        init_pair (63, -1, -1);
        
        /* white on default bg is default (-1) */
        if (!cfg_col_real_white)
            init_pair (WEECHAT_COLOR_WHITE, -1, -1);
    }
}

/*
 * gui_init_weechat_colors: init WeeChat colors
 */

void
gui_init_weechat_colors ()
{
    int i;
    
    /* init WeeChat colors */
    gui_color[COLOR_WIN_SEPARATOR] = gui_color_build (COLOR_WIN_SEPARATOR, cfg_col_separator, cfg_col_separator);
    gui_color[COLOR_WIN_TITLE] = gui_color_build (COLOR_WIN_TITLE, cfg_col_title, cfg_col_title_bg);
    gui_color[COLOR_WIN_CHAT] = gui_color_build (COLOR_WIN_CHAT, cfg_col_chat, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_TIME] = gui_color_build (COLOR_WIN_CHAT_TIME, cfg_col_chat_time, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_TIME_SEP] = gui_color_build (COLOR_WIN_CHAT_TIME_SEP, cfg_col_chat_time_sep, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_PREFIX1] = gui_color_build (COLOR_WIN_CHAT_PREFIX1, cfg_col_chat_prefix1, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_PREFIX2] = gui_color_build (COLOR_WIN_CHAT_PREFIX2, cfg_col_chat_prefix2, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_SERVER] = gui_color_build (COLOR_WIN_CHAT_SERVER, cfg_col_chat_server, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_JOIN] = gui_color_build (COLOR_WIN_CHAT_JOIN, cfg_col_chat_join, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_PART] = gui_color_build (COLOR_WIN_CHAT_PART, cfg_col_chat_part, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_NICK] = gui_color_build (COLOR_WIN_CHAT_NICK, cfg_col_chat_nick, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_HOST] = gui_color_build (COLOR_WIN_CHAT_HOST, cfg_col_chat_host, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_CHANNEL] = gui_color_build (COLOR_WIN_CHAT_CHANNEL, cfg_col_chat_channel, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_DARK] = gui_color_build (COLOR_WIN_CHAT_DARK, cfg_col_chat_dark, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_HIGHLIGHT] = gui_color_build (COLOR_WIN_CHAT_HIGHLIGHT, cfg_col_chat_highlight, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_READ_MARKER] = gui_color_build (COLOR_WIN_CHAT_READ_MARKER, cfg_col_chat_read_marker, cfg_col_chat_read_marker_bg);
    gui_color[COLOR_WIN_STATUS] = gui_color_build (COLOR_WIN_STATUS, cfg_col_status, cfg_col_status_bg);
    gui_color[COLOR_WIN_STATUS_DELIMITERS] = gui_color_build (COLOR_WIN_STATUS_DELIMITERS, cfg_col_status_delimiters, cfg_col_status_bg);
    gui_color[COLOR_WIN_STATUS_CHANNEL] = gui_color_build (COLOR_WIN_STATUS_CHANNEL, cfg_col_status_channel, cfg_col_status_bg);
    gui_color[COLOR_WIN_STATUS_DATA_MSG] = gui_color_build (COLOR_WIN_STATUS_DATA_MSG, cfg_col_status_data_msg, cfg_col_status_bg);
    gui_color[COLOR_WIN_STATUS_DATA_PRIVATE] = gui_color_build (COLOR_WIN_STATUS_DATA_PRIVATE, cfg_col_status_data_private, cfg_col_status_bg);
    gui_color[COLOR_WIN_STATUS_DATA_HIGHLIGHT] = gui_color_build (COLOR_WIN_STATUS_DATA_HIGHLIGHT, cfg_col_status_data_highlight, cfg_col_status_bg);
    gui_color[COLOR_WIN_STATUS_DATA_OTHER] = gui_color_build (COLOR_WIN_STATUS_DATA_OTHER, cfg_col_status_data_other, cfg_col_status_bg);
    gui_color[COLOR_WIN_STATUS_MORE] = gui_color_build (COLOR_WIN_STATUS_MORE, cfg_col_status_more, cfg_col_status_bg);
    gui_color[COLOR_WIN_INFOBAR] = gui_color_build (COLOR_WIN_INFOBAR, cfg_col_infobar, cfg_col_infobar_bg);
    gui_color[COLOR_WIN_INFOBAR_DELIMITERS] = gui_color_build (COLOR_WIN_INFOBAR_DELIMITERS, cfg_col_infobar_delimiters, cfg_col_infobar_bg);
    gui_color[COLOR_WIN_INFOBAR_HIGHLIGHT] = gui_color_build (COLOR_WIN_INFOBAR_HIGHLIGHT, cfg_col_infobar_highlight, cfg_col_infobar_bg);
    gui_color[COLOR_WIN_INPUT] = gui_color_build (COLOR_WIN_INPUT, cfg_col_input, cfg_col_input_bg);
    gui_color[COLOR_WIN_INPUT_CHANNEL] = gui_color_build (COLOR_WIN_INPUT_CHANNEL, cfg_col_input_channel, cfg_col_input_bg);
    gui_color[COLOR_WIN_INPUT_NICK] = gui_color_build (COLOR_WIN_INPUT_NICK, cfg_col_input_nick, cfg_col_input_bg);
    gui_color[COLOR_WIN_INPUT_DELIMITERS] = gui_color_build (COLOR_WIN_INPUT_DELIMITERS, cfg_col_input_delimiters, cfg_col_input_bg);
    gui_color[COLOR_WIN_NICK] = gui_color_build (COLOR_WIN_NICK, cfg_col_nick, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_AWAY] = gui_color_build (COLOR_WIN_NICK_AWAY, cfg_col_nick_away, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_CHANOWNER] = gui_color_build (COLOR_WIN_NICK_CHANOWNER, cfg_col_nick_chanowner, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_CHANADMIN] = gui_color_build (COLOR_WIN_NICK_CHANADMIN, cfg_col_nick_chanadmin, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_OP] = gui_color_build (COLOR_WIN_NICK_OP, cfg_col_nick_op, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_HALFOP] = gui_color_build (COLOR_WIN_NICK_HALFOP, cfg_col_nick_halfop, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_VOICE] = gui_color_build (COLOR_WIN_NICK_VOICE, cfg_col_nick_voice, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_MORE] = gui_color_build (COLOR_WIN_NICK_MORE, cfg_col_nick_more, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_SEP] = gui_color_build (COLOR_WIN_NICK_SEP, cfg_col_nick_sep, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_SELF] = gui_color_build (COLOR_WIN_NICK_SELF, cfg_col_nick_self, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_PRIVATE] = gui_color_build (COLOR_WIN_NICK_PRIVATE, cfg_col_nick_private, cfg_col_nick_bg);

    for (i = 0; i < COLOR_WIN_NICK_NUMBER; i++)
    {
        gui_color[COLOR_WIN_NICK_1 + i] = gui_color_build (COLOR_WIN_NICK_1 + i, cfg_col_nick_colors[i], cfg_col_chat_bg);
    }
        
    gui_color[COLOR_DCC_SELECTED] = gui_color_build (COLOR_DCC_SELECTED, cfg_col_dcc_selected, cfg_col_chat_bg);
    gui_color[COLOR_DCC_WAITING] = gui_color_build (COLOR_DCC_WAITING, cfg_col_dcc_waiting, cfg_col_chat_bg);
    gui_color[COLOR_DCC_CONNECTING] = gui_color_build (COLOR_DCC_CONNECTING, cfg_col_dcc_connecting, cfg_col_chat_bg);
    gui_color[COLOR_DCC_ACTIVE] = gui_color_build (COLOR_DCC_ACTIVE, cfg_col_dcc_active, cfg_col_chat_bg);
    gui_color[COLOR_DCC_DONE] = gui_color_build (COLOR_DCC_DONE, cfg_col_dcc_done, cfg_col_chat_bg);
    gui_color[COLOR_DCC_FAILED] = gui_color_build (COLOR_DCC_FAILED, cfg_col_dcc_failed, cfg_col_chat_bg);
    gui_color[COLOR_DCC_ABORTED] = gui_color_build (COLOR_DCC_ABORTED, cfg_col_dcc_aborted, cfg_col_chat_bg);
}

/*
 * gui_rebuild_weechat_colors: rebuild WeeChat colors
 */

void
gui_rebuild_weechat_colors ()
{
    int i;
    
    if (has_colors ())
    {
        for (i = 0; i < NUM_COLORS; i++)
        {
            if (gui_color[i])
            {
                if (gui_color[i]->string)
                    free (gui_color[i]->string);
                free (gui_color[i]);
                gui_color[i] = NULL;
            }
        }
        gui_init_weechat_colors ();
    }
}

/*
 * gui_init_colors: init GUI colors
 */

void
gui_init_colors ()
{
    if (has_colors ())
    {
        start_color ();
        use_default_colors ();
    }
    gui_init_color_pairs ();
    gui_init_weechat_colors ();
}

/*
 * gui_set_window_title: set terminal title
 */

void
gui_set_window_title ()
{
    char *envterm = getenv ("TERM");
    
    if (envterm)
    {
	if (strcmp( envterm, "sun-cmd") == 0)
	    printf ("\033]l%s %s\033\\", PACKAGE_NAME, PACKAGE_VERSION);
	else if (strcmp(envterm, "hpterm") == 0)
	    printf ("\033&f0k%dD%s %s", strlen(PACKAGE_NAME) + 
		    strlen(PACKAGE_VERSION) + 1,
		    PACKAGE_NAME, PACKAGE_VERSION);
	/* the following term supports the xterm excapes */
	else if (strncmp (envterm, "xterm", 5) == 0
		 || strncmp (envterm, "rxvt", 4) == 0
		 || strcmp (envterm, "Eterm") == 0
		 || strcmp (envterm, "aixterm") == 0
		 || strcmp (envterm, "iris-ansi") == 0
		 || strcmp (envterm, "dtterm") == 0)
	    printf ("\33]0;%s %s\7", PACKAGE_NAME, PACKAGE_VERSION);
	else if (strcmp (envterm, "screen") == 0)
	{
	    printf ("\033k%s %s\033\\", PACKAGE_NAME, PACKAGE_VERSION);
	    /* tryning to set the title of a backgrounded xterm like terminal */
	    printf ("\33]0;%s %s\7", PACKAGE_NAME, PACKAGE_VERSION);
	}
    }
}

/*
 * gui_reset_window_title: reset terminal title
 */

void
gui_reset_window_title ()
{
    char *envterm = getenv ("TERM");
    char *envshell = getenv ("SHELL");

    if (envterm)
    {
	if (strcmp( envterm, "sun-cmd") == 0)
	    printf ("\033]l%s\033\\", "Terminal");
	else if (strcmp( envterm, "hpterm") == 0)
	    printf ("\033&f0k%dD%s", strlen("Terminal"), "Terminal");
	/* the following term supports the xterm excapes */
	else if (strncmp (envterm, "xterm", 5) == 0
		 || strncmp (envterm, "rxvt", 4) == 0
		 || strcmp (envterm, "Eterm") == 0
		 || strcmp( envterm, "aixterm") == 0
		 || strcmp( envterm, "iris-ansi") == 0
		 || strcmp( envterm, "dtterm") == 0)
	    printf ("\33]0;%s\7", "Terminal");
	else if (strcmp (envterm, "screen") == 0)
	{
	    char *shell, *shellname;
	    if (envshell)
	    {
		shell  = strdup (envterm);
		shellname = basename(shell);
		if (shell)
		{
		    printf ("\033k%s\033\\", shellname);
		    free (shell);
		}
		else
		    printf ("\033k%s\033\\", envterm);
	    }
	    else
		printf ("\033k%s\033\\", envterm);
	    /* tryning to reset the title of a backgrounded xterm like terminal */
	    printf ("\33]0;%s\7", "Terminal");
	}
    }
}

/*
 * gui_init: init GUI
 */

void
gui_init ()
{
    initscr ();
    
    curs_set (1);
    noecho ();
    nodelay (stdscr, TRUE);

    gui_init_colors ();
    
    gui_infobar = NULL;
    
    gui_ok = ((COLS > 5) && (LINES > 5));

    refresh ();
    
    /* init clipboard buffer */
    gui_input_clipboard = NULL;

    /* create new window/buffer */
    if (gui_window_new (NULL, 0, 0, COLS, LINES, 100, 100))
    {
        gui_current_window = gui_windows;
        gui_buffer_new (gui_windows, NULL, NULL, BUFFER_TYPE_STANDARD, 1);
    
        signal (SIGWINCH, gui_refresh_screen_sigwinch);
	
        if (cfg_look_set_title)
            gui_set_window_title ();
    
        gui_init_ok = 1;
    }
}

/*
 * gui_end: GUI end
 */

void
gui_end ()
{
    t_gui_window *ptr_win;
    
    /* free clipboard buffer */
    if (gui_input_clipboard)
      free(gui_input_clipboard);

    /* delete all windows */
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->win_title)
            delwin (ptr_win->win_title);
        if (ptr_win->win_chat)
            delwin (ptr_win->win_chat);
        if (ptr_win->win_nick)
            delwin (ptr_win->win_nick);
        if (ptr_win->win_status)
            delwin (ptr_win->win_status);
        if (ptr_win->win_infobar)
            delwin (ptr_win->win_infobar);
        if (ptr_win->win_input)
            delwin (ptr_win->win_input);
    }
    
    /* delete all buffers */
    while (gui_buffers)
        gui_buffer_free (gui_buffers, 0);
    
    /* delete all windows */
    while (gui_windows)
        gui_window_free (gui_windows);
    gui_window_tree_free (&gui_windows_tree);
    
    /* delete global history */
    history_global_free ();
    
    /* delete infobar messages */
    while (gui_infobar)
        gui_infobar_remove ();

    /* reset title */
    if (cfg_look_set_title)
	gui_reset_window_title ();
    
    /* end of curses output */
    refresh ();
    endwin ();
}
