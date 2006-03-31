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

/* gui-display.c: display functions for Gtk GUI */


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
#include <gtk/gtk.h>

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/weeconfig.h"
#include "../../common/hotlist.h"
#include "../../common/log.h"
#include "../../common/utf8.h"
#include "../../irc/irc.h"


/* TODO: remove these temporary defines */

#define A_BOLD      1
#define A_UNDERLINE 2
#define A_REVERSE   4

#define COLOR_BLACK   0
#define COLOR_BLUE    1
#define COLOR_GREEN   2
#define COLOR_CYAN    3
#define COLOR_RED     4
#define COLOR_MAGENTA 5
#define COLOR_YELLOW  6
#define COLOR_WHITE   7


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

int gui_irc_colors[GUI_NUM_IRC_COLORS][2] =
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

t_gui_color *gui_color[GUI_NUM_COLORS];

GtkWidget *gtk_main_window;
GtkWidget *vbox1;
GtkWidget *entry_topic;
GtkWidget *notebook1;
GtkWidget *vbox2;
GtkWidget *hbox1;
GtkWidget *hpaned1;
GtkWidget *scrolledwindow_chat;
GtkWidget *scrolledwindow_nick;
GtkWidget *entry_input;
GtkWidget *label1;


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
    
    if ((num_color < 0) || (num_color > GUI_NUM_COLORS - 1))
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

/* TODO: write this function for Gtk */
/*void
gui_window_set_weechat_color (WINDOW *window, int num_color)
{
    if ((num_color >= 0) && (num_color <= GUI_NUM_COLORS - 1))
    {
        wattroff (window, A_BOLD | A_UNDERLINE | A_REVERSE);
        wattron (window, COLOR_PAIR(gui_color_get_pair (num_color)) |
                 gui_color[num_color]->attributes);
    }
}*/

/*
 * gui_window_chat_set_style: set style (bold, underline, ..)
 *                            for a chat window
 */

void
gui_window_chat_set_style (t_gui_window *window, int style)
{
    /* TODO: write this function for Gtk */
    /*wattron (window->win_chat, style);*/
    (void) window;
    (void) style;
}

/*
 * gui_window_chat_remove_style: remove style (bold, underline, ..)
 *                               for a chat window
 */

void
gui_window_chat_remove_style (t_gui_window *window, int style)
{
    /* TODO: write this function for Gtk */
    /*wattroff (window->win_chat, style);*/
    (void) window;
    (void) style;
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
    
    /* TODO: change following function call */
    /*gui_window_set_weechat_color (window->win_chat, COLOR_WIN_CHAT);*/
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
    /* TODO: change following function call */
    /*wattron (window->win_chat, style);*/
}

/*
 * gui_window_chat_remove_color_style: remove style for color
 */

void
gui_window_chat_remove_color_style (t_gui_window *window, int style)
{
    window->current_color_attr &= !style;
    /* TODO: change following function call */
    /*wattroff (window->win_chat, style);*/
}

/*
 * gui_window_chat_reset_color_style: reset style for color
 */

void
gui_window_chat_reset_color_style (t_gui_window *window)
{
    /* TODO: change following function call */
    /*wattroff (window->win_chat, window->current_color_attr);*/
    window->current_color_attr = 0;
}

/*
 * gui_window_chat_set_color: set color for a chat window
 */

void
gui_window_chat_set_color (t_gui_window *window, int fg, int bg)
{
    /* TODO: write this function for Gtk */    
    /*if (((fg == -1) || (fg == 99))
        && ((bg == -1) || (bg == 99)))
        wattron (window->win_chat, COLOR_PAIR(63));
    else
    {
        if ((fg == -1) || (fg == 99))
            fg = WEECHAT_COLOR_WHITE;
        if ((bg == -1) || (bg == 99))
            bg = 0;
        wattron (window->win_chat, COLOR_PAIR((bg * 8) + fg));
    }*/
    (void) window;
    (void) fg;
    (void) bg;
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
 * gui_calculate_pos_size: calculate position and size for a window & sub-win
 */

int
gui_calculate_pos_size (t_gui_window *window, int force_calculate)
{
    /* TODO: write this function for Gtk */
    (void) window;
    (void) force_calculate;
    
    return 0;
}

/*
 * gui_draw_window_separator: draw window separation
 */

void
gui_draw_window_separator (t_gui_window *window)
{
    /* TODO: write this function for Gtk */
    /*if (window->win_separator)
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
    }*/
    (void) window;
}

/*
 * gui_draw_buffer_title: draw title window for a buffer
 */

void
gui_draw_buffer_title (t_gui_buffer *buffer, int erase)
{
    /* TODO: write this function for Gtk */
    (void) buffer;
    (void) erase;
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
    /*char *prev_char, *next_char, saved_char;*/
    
    /* TODO: write this function for Gtk */
    (void) window;
    (void) string;
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
    /*char *end_line, saved_char_end, saved_char;
      int pos_saved_char, chars_to_display, num_displayed;*/
    
    /* TODO: write this function for Gtk */
    (void) window;
    (void) line;
    (void) data;
    (void) end_offset;
    (void) num_lines;
    (void) count;
    (void) lines_displayed;
    (void) simulate;
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
 * gui_display_line: display a line in the chat window
 *                   if count == 0, display whole line
 *                   if count > 0, display 'count' lines (beginning from the end)
 *                   if simulate == 1, nothing is displayed (for counting how
 *                                     many lines would have been lines displayed)
 *                   returns: number of lines displayed (or simulated)
 */

int
gui_display_line (t_gui_window *window, t_gui_line *line, int count, int simulate)
{
    /* TODO: write this function for Gtk */
    (void) window;
    (void) line;
    (void) count;
    (void) simulate;
    return 1;
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
            current_size = gui_display_line (window, *line, 0, 1);
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
            current_size = gui_display_line (window, *line, 0, 1);
        }
    }
    else
        current_size = gui_display_line (window, *line, 0, 1);
    
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
                    current_size = gui_display_line (window, *line, 0, 1);
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
                    current_size = gui_display_line (window, *line, 0, 1);
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
    /*t_gui_window *ptr_win;
    t_gui_line *ptr_line;
    t_irc_dcc *dcc_first, *dcc_selected, *ptr_dcc;
    char format_empty[32];
    int i, j, line_pos, count, num_bars;
    char *unit_name[] = { N_("bytes"), N_("Kb"), N_("Mb"), N_("Gb") };
    char *unit_format[] = { "%.0Lf", "%.1Lf", "%.02Lf", "%.02Lf" };
    long unit_divide[] = { 1, 1024, 1024*1024, 1024*1024,1024 };
    int num_unit;
    char format[32], date[128], *buf;
    struct tm *date_tmp;*/
    
    if (!gui_ok)
        return;
    
    /* TODO: write this function for Gtk */
    (void) buffer;
    (void) erase;
}

/*
 * gui_draw_buffer_chat_line: add a line to chat window for a buffer
 */

void
gui_draw_buffer_chat_line (t_gui_buffer *buffer, t_gui_line *line)
{
    t_gui_window *ptr_win;
    unsigned char *text_without_color;
    GtkTextIter start, end;
    
    ptr_win = gui_buffer_find_window (buffer);
    if (ptr_win)
    {
        text_without_color = gui_color_decode ((unsigned char *)(line->data), 0);
        if (text_without_color)
        {
            gtk_text_buffer_insert_at_cursor (ptr_win->textbuffer_chat,
                                              (char *)text_without_color, -1);
            gtk_text_buffer_insert_at_cursor (ptr_win->textbuffer_chat,
                                              "\n", -1);
            gtk_text_buffer_get_bounds (ptr_win->textbuffer_chat,
                                        &start, &end);
            /* TODO */
            /*gtk_text_buffer_apply_tag (ptr_win->textbuffer_chat, ptr_win->texttag_chat, &start, &end);*/
            free (text_without_color);
        }
    }
}

/*
 * gui_draw_buffer_nick: draw nick window for a buffer
 */

void
gui_draw_buffer_nick (t_gui_buffer *buffer, int erase)
{
    /*t_gui_window *ptr_win;
    int i, j, x, y, column, max_length, nicks_displayed;
    char format[32], format_empty[32];
    t_irc_nick *ptr_nick;*/
    
    if (!gui_ok || !BUFFER_HAS_NICKLIST(buffer))
        return;
    
    /* TODO: write this function for Gtk */
    (void) buffer;
    (void) erase;
}

/*
 * gui_draw_buffer_status: draw status window for a buffer
 */

void
gui_draw_buffer_status (t_gui_buffer *buffer, int erase)
{
    /*t_gui_window *ptr_win;
    t_weechat_hotlist *ptr_hotlist;
    char format[32], str_nicks[32], *more;
    int i, first_mode, x, server_pos, server_total;
    int display_name, names_count;*/
    
    /* make gcc happy */
    (void) buffer;
    
    if (!gui_ok)
        return;
    
    /* TODO: write this function for Gtk */
    (void) buffer;
    (void) erase;
}

/*
 * gui_draw_buffer_infobar_time: draw time in infobar window
 */

void
gui_draw_buffer_infobar_time (t_gui_buffer *buffer)
{
    /*t_gui_window *ptr_win;
    time_t time_seconds;
    struct tm *local_time;*/
    
    /* make gcc happy */
    (void) buffer;
    
    if (!gui_ok)
        return;
    
    /* TODO: write this function for Gtk */
    (void) buffer;
}

/*
 * gui_draw_buffer_infobar: draw infobar window for a buffer
 */

void
gui_draw_buffer_infobar (t_gui_buffer *buffer, int erase)
{
    /*t_gui_window *ptr_win;
    time_t time_seconds;
    struct tm *local_time;
    char text_time[1024 + 1];*/
    
    /* make gcc happy */
    (void) buffer;
    
    if (!gui_ok)
        return;
    
    /* TODO: write this function for Gtk */
    (void) buffer;
    (void) erase;
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
    /*t_gui_window *ptr_win;
    char format[32];
    char *ptr_nickname;
    int input_width;
    t_irc_dcc *dcc_selected;*/
    
    if (!gui_ok)
        return;
    
    /* TODO: write this function for Gtk */
    (void) buffer;
    (void) erase;
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
    GtkTextIter start, end;
    
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
    gui_calculate_pos_size (window, 1);
    
    if (!window->textview_chat)
    {
        window->textview_chat = gtk_text_view_new ();
        gtk_widget_show (window->textview_chat);
        gtk_container_add (GTK_CONTAINER (scrolledwindow_chat), window->textview_chat);
        gtk_widget_set_size_request (window->textview_chat, 300, -1);
        gtk_text_view_set_editable (GTK_TEXT_VIEW (window->textview_chat), FALSE);
        gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (window->textview_chat), FALSE);
        
        window->textbuffer_chat = gtk_text_buffer_new (NULL);
        gtk_text_view_set_buffer (GTK_TEXT_VIEW (window->textview_chat), window->textbuffer_chat);
        
        /*window->texttag_chat = gtk_text_buffer_create_tag(window->textbuffer_chat, "courier", "font_family", "lucida");*/
        gtk_text_buffer_get_bounds (window->textbuffer_chat, &start, &end);
        gtk_text_buffer_apply_tag (window->textbuffer_chat, window->texttag_chat, &start, &end);
    }
    if (BUFFER_IS_CHANNEL(buffer) && !window->textbuffer_nicklist)
    {
        window->textview_nicklist = gtk_text_view_new ();
        gtk_widget_show (window->textview_nicklist);
        gtk_container_add (GTK_CONTAINER (scrolledwindow_nick), window->textview_nicklist);
        gtk_text_view_set_editable (GTK_TEXT_VIEW (window->textview_nicklist), FALSE);
        gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (window->textview_nicklist), FALSE);
        
        window->textbuffer_nicklist = gtk_text_buffer_new (NULL);
        gtk_text_view_set_buffer (GTK_TEXT_VIEW (window->textview_nicklist), window->textbuffer_nicklist);
    }
    
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
 * gui_window_init_subwindows: init subwindows for a WeeChat window
 */

void
gui_window_init_subwindows (t_gui_window *window)
{
    window->textview_chat = NULL;
    window->textbuffer_chat = NULL;
    window->texttag_chat = NULL;
    window->textview_nicklist = NULL;
    window->textbuffer_nicklist = NULL;
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
    /*t_gui_window *ptr_win, *old_current_window;*/
    
    if (gui_ok)
    {
        /* TODO: write this function for Gtk */
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
    /* TODO: write this function for Gtk */
    (void) window;
    (void) pourcentage;
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
    /* TODO: write this function for Gtk */
    (void) window;
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
    /* TODO: write this function for Gtk */
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
    /* Initialise Gtk */
    gtk_init (argc, argv);
}

/*
 * gui_init_color_pairs: init color pairs
 */

void
gui_init_color_pairs ()
{
    /* This function does nothing in Gtk GUI */
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
    
    for (i = 0; i < GUI_NUM_COLORS; i++)
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

/*
 * gui_init_colors: init GUI colors
 */

void
gui_init_colors ()
{
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
    /* This function does nothing in Gtk GUI */
}

/*
 * gui_init: init GUI
 */

void
gui_init ()
{
    GdkColor color_fg, color_bg;
    
    gui_init_colors ();
    
    gui_infobar = NULL;
    
    gui_ok = 1;
    
    /* init clipboard buffer */
    gui_input_clipboard = NULL;
    
    /* create Gtk widgets */

    gdk_color_parse ("white", &color_fg);
    gdk_color_parse ("black", &color_bg);
    
    gtk_main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (gtk_main_window), PACKAGE_STRING);
    
    g_signal_connect (G_OBJECT (gtk_main_window), "destroy",  gtk_main_quit, NULL);
    
    vbox1 = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox1);
    gtk_container_add (GTK_CONTAINER (gtk_main_window), vbox1);
    
    entry_topic = gtk_entry_new ();
    gtk_widget_show (entry_topic);
    gtk_box_pack_start (GTK_BOX (vbox1), entry_topic, FALSE, FALSE, 0);
    gtk_widget_modify_text (entry_topic, GTK_STATE_NORMAL, &color_fg);
    gtk_widget_modify_base (entry_topic, GTK_STATE_NORMAL, &color_bg);
    
    notebook1 = gtk_notebook_new ();
    gtk_widget_show (notebook1);
    gtk_box_pack_start (GTK_BOX (vbox1), notebook1, TRUE, TRUE, 0);
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook1), GTK_POS_BOTTOM);
    
    vbox2 = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox2);
    gtk_container_add (GTK_CONTAINER (notebook1), vbox2);
    
    hbox1 = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox1);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox1, TRUE, TRUE, 0);
    
    hpaned1 = gtk_hpaned_new ();
    gtk_widget_show (hpaned1);
    gtk_box_pack_start (GTK_BOX (hbox1), hpaned1, TRUE, TRUE, 0);
    gtk_paned_set_position (GTK_PANED (hpaned1), 0);
    
    scrolledwindow_chat = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scrolledwindow_chat);
    gtk_paned_pack1 (GTK_PANED (hpaned1), scrolledwindow_chat, FALSE, TRUE);
    //gtk_box_pack_start (GTK_PANED (hpaned1), scrolledwindow_chat, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_chat), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_widget_modify_text (scrolledwindow_chat, GTK_STATE_NORMAL, &color_fg);
    gtk_widget_modify_base (scrolledwindow_chat, GTK_STATE_NORMAL, &color_bg);
    
    scrolledwindow_nick = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scrolledwindow_nick);
    gtk_paned_pack2 (GTK_PANED (hpaned1), scrolledwindow_nick, FALSE, TRUE);
    //gtk_box_pack_start (GTK_PANED (hpaned1), scrolledwindow_nick, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_nick), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_widget_modify_text (scrolledwindow_nick, GTK_STATE_NORMAL, &color_fg);
    gtk_widget_modify_base (scrolledwindow_nick, GTK_STATE_NORMAL, &color_bg);
    
    entry_input = gtk_entry_new ();
    gtk_widget_show (entry_input);
    gtk_box_pack_start (GTK_BOX (vbox2), entry_input, FALSE, FALSE, 0);
    gtk_widget_modify_text (entry_input, GTK_STATE_NORMAL, &color_fg);
    gtk_widget_modify_base (entry_input, GTK_STATE_NORMAL, &color_bg);
    
    label1 = gtk_label_new (_("server"));
    gtk_widget_show (label1);
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 0), label1);
    gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);
    
    gtk_widget_show_all (gtk_main_window);
    
    /* create new window/buffer */
    if (gui_window_new (NULL, 0, 0, 0, 0, 100, 100))
    {
        gui_current_window = gui_windows;
        gui_buffer_new (gui_windows, NULL, NULL, 0, 1);
        
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
        /* TODO: destroy Gtk widgets */
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
}
