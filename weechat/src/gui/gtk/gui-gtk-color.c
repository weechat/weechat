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

/* gui-gtk-color.c: color functions for Gtk GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/util.h"
#include "../../common/weeconfig.h"
#include "gui-gtk.h"


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


/*
 * gui_color_assign: assign a WeeChat color (read from config)
 */

int
gui_color_assign (int *color, char *color_name)
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
 * gui_color_get_name: get color name
 */

char *
gui_color_get_name (int num_color)
{
    return gui_weechat_colors[num_color].string;
}

/*
 * gui_color_decode: parses a message (coming from IRC server),
 *                   if keep_colors == 0: remove any color/style in message
 *                     otherwise change colors by internal WeeChat color codes
 *                   if wkeep_eechat_attr == 0: remove any weechat color/style attribute
 *                   After use, string returned has to be free()
 */

unsigned char *
gui_color_decode (unsigned char *string, int keep_irc_colors, int keep_weechat_attr)
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
    while (string && string[0] && (out_pos < out_length - 1))
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
                if (keep_irc_colors)
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
                if (keep_irc_colors)
                {
                    if (!str_fg[0] && !str_bg[0])
                        out[out_pos++] = GUI_ATTR_COLOR_CHAR;
                    else
                    {
                        attr = 0;
                        if (str_fg[0])
                        {
                            sscanf (str_fg, "%d", &fg);
                            fg %= GUI_NUM_IRC_COLORS;
                            attr |= gui_irc_colors[fg][1];
                        }
                        if (str_bg[0])
                        {
                            sscanf (str_bg, "%d", &bg);
                            bg %= GUI_NUM_IRC_COLORS;
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
                if (keep_weechat_attr)
                    out[out_pos++] = string[0];
                string++;
                if (isdigit (string[0]) && isdigit (string[1]))
                {
                    if (keep_weechat_attr)
                    {
                        out[out_pos++] = string[0];
                        out[out_pos++] = string[1];
                    }
                    string += 2;
                }
                break;
            case GUI_ATTR_WEECHAT_SET_CHAR:
            case GUI_ATTR_WEECHAT_REMOVE_CHAR:
                if (keep_weechat_attr)
                    out[out_pos++] = string[0];
                string++;
                if (string[0])
                {
                    if (keep_weechat_attr)
                        out[out_pos++] = string[0];
                    string++;
                }
                break;
            case GUI_ATTR_WEECHAT_RESET_CHAR:
                if (keep_weechat_attr)
                    out[out_pos++] = string[0];
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
 * gui_color_decode_for_user_entry: parses a message (coming from IRC server),
 *                                  and replaces colors/bold/.. by ^C, ^B, ..
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
    while (string && string[0] && (out_pos < out_length - 1))
    {
        switch (string[0])
        {
            case GUI_ATTR_BOLD_CHAR:
                out[out_pos++] = 0x02; /* ^B */
                string++;
                break;
            case GUI_ATTR_FIXED_CHAR:
                string++;
                break;
            case GUI_ATTR_RESET_CHAR:
                out[out_pos++] = 0x0F; /* ^O */
                string++;
                break;
            case GUI_ATTR_REVERSE_CHAR:
            case GUI_ATTR_REVERSE2_CHAR:
                out[out_pos++] = 0x12; /* ^R */
                string++;
                break;
            case GUI_ATTR_ITALIC_CHAR:
                string++;
                break;
            case GUI_ATTR_UNDERLINE_CHAR:
                out[out_pos++] = 0x15; /* ^U */
                string++;
                break;
            case GUI_ATTR_COLOR_CHAR:
                out[out_pos++] = 0x03; /* ^C */
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
 *                   encode special chars (^Cb, ^Cc, ..) in IRC colors
 *                   if keep_colors == 0: remove any color/style in message
 *                   otherwise: keep colors
 *                   After use, string returned has to be free()
 */

unsigned char *
gui_color_encode (unsigned char *string, int keep_colors)
{
    unsigned char *out;
    int out_length, out_pos;
    
    out_length = (strlen ((char *)string) * 2) + 1;
    out = (unsigned char *)malloc (out_length);
    if (!out)
        return NULL;
    
    out_pos = 0;
    while (string && string[0] && (out_pos < out_length - 1))
    {
        switch (string[0])
        {
            case 0x02: /* ^B */
                if (keep_colors)
                    out[out_pos++] = GUI_ATTR_BOLD_CHAR;
                string++;
                break;
            case 0x03: /* ^C */
                if (keep_colors)
                    out[out_pos++] = GUI_ATTR_COLOR_CHAR;
                string++;
                if (isdigit (string[0]))
                {
                    if (keep_colors)
                        out[out_pos++] = string[0];
                    string++;
                    if (isdigit (string[0]))
                    {
                        if (keep_colors)
                            out[out_pos++] = string[0];
                        string++;
                    }
                }
                if (string[0] == ',')
                {
                    if (keep_colors)
                        out[out_pos++] = ',';
                    string++;
                    if (isdigit (string[0]))
                    {
                        if (keep_colors)
                            out[out_pos++] = string[0];
                        string++;
                        if (isdigit (string[0]))
                        {
                            if (keep_colors)
                                out[out_pos++] = string[0];
                            string++;
                        }
                    }
                }
                break;
            case 0x0F: /* ^O */
                if (keep_colors)
                    out[out_pos++] = GUI_ATTR_RESET_CHAR;
                string++;
                break;
            case 0x12: /* ^R */
                if (keep_colors)
                    out[out_pos++] = GUI_ATTR_REVERSE_CHAR;
                string++;
                break;
            case 0x15: /* ^U */
                if (keep_colors)
                    out[out_pos++] = GUI_ATTR_UNDERLINE_CHAR;
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
 * gui_color_init_pairs: init color pairs
 */

void
gui_color_init_pairs ()
{
    /* This function does nothing in Gtk GUI */
}

/*
 * gui_color_init_weechat: init WeeChat colors
 */

void
gui_color_init_weechat ()
{
    /* TODO: write this function for Gtk */
}

/*
 * gui_color_rebuild_weechat: rebuild WeeChat colors
 */

void
gui_color_rebuild_weechat ()
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
    gui_color_init_weechat ();
}

/*
 * gui_color_init: init GUI colors
 */

void
gui_color_init ()
{
    gui_color_init_pairs ();
    gui_color_init_weechat ();
}
