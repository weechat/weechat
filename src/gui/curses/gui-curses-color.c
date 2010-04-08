/*
 * Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
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

/*
 * gui-curses-color.c: color functions for Curses GUI
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-string.h"
#include "../../core/wee-utf8.h"
#include "../gui-color.h"
#include "../gui-chat.h"
#include "gui-curses.h"


struct t_gui_color gui_weechat_colors[GUI_CURSES_NUM_WEECHAT_COLORS + 1] =
{ { -1,            0, 0,                      "default"      },
  { COLOR_BLACK,   COLOR_BLACK,       0,      "black"        },
  { COLOR_BLACK,   COLOR_BLACK + 8,   A_BOLD, "darkgray"     },
  { COLOR_RED,     COLOR_RED,         0,      "red"          },
  { COLOR_RED,     COLOR_RED + 8,     A_BOLD, "lightred"     },
  { COLOR_GREEN,   COLOR_GREEN,      0,       "green"        },
  { COLOR_GREEN,   COLOR_GREEN + 8,   A_BOLD, "lightgreen"   },
  { COLOR_YELLOW,  COLOR_YELLOW,      0,      "brown"        },
  { COLOR_YELLOW,  COLOR_YELLOW + 8,  A_BOLD, "yellow"       },
  { COLOR_BLUE,    COLOR_BLUE,        0,      "blue"         },
  { COLOR_BLUE,    COLOR_BLUE + 8,    A_BOLD, "lightblue"    },
  { COLOR_MAGENTA, COLOR_MAGENTA,     0,      "magenta"      },
  { COLOR_MAGENTA, COLOR_MAGENTA + 8, A_BOLD, "lightmagenta" },
  { COLOR_CYAN,    COLOR_CYAN,        0,      "cyan"         },
  { COLOR_CYAN,    COLOR_CYAN + 8,    A_BOLD, "lightcyan"    },
  { COLOR_WHITE,   COLOR_WHITE,       A_BOLD, "white"        },
  { 0,             0,                 0,      NULL           }
};

int gui_color_last_pair = 63;
int gui_color_num_bg = 8;


/*
 * gui_color_search: search a color by name
 *                   Return: number of color in WeeChat colors table
 */

int
gui_color_search (const char *color_name)
{
    int i;

    for (i = 0; gui_weechat_colors[i].string; i++)
    {
        if (string_strcasecmp (gui_weechat_colors[i].string, color_name) == 0)
            return i;
    }

    /* color not found */
    return -1;
}

/*
 * gui_color_assign: assign a WeeChat color (read from config)
 */

int
gui_color_assign (int *color, const char *color_name)
{
    int i;
    
    /* look for curses colors in table */
    i = 0;
    while (gui_weechat_colors[i].string)
    {
        if (string_strcasecmp (gui_weechat_colors[i].string, color_name) == 0)
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
 * gui_color_get_number: get number of available colors
 */

int
gui_color_get_number ()
{
    return GUI_CURSES_NUM_WEECHAT_COLORS;
}

/*
 * gui_color_get_name: get color name
 */

const char *
gui_color_get_name (int num_color)
{
    return gui_weechat_colors[num_color].string;
}

/*
 * gui_color_build: build a WeeChat color with foreground,
 *                  background and attributes (attributes are
 *                  given with foreground color, with a OR)
 */

void
gui_color_build (int number, int foreground, int background)
{
    if (!gui_color[number])
    {
        gui_color[number] = malloc (sizeof (*gui_color[number]));
        if (!gui_color[number])
            return;
        gui_color[number]->string = malloc (4);
    }
    
    gui_color[number]->foreground = gui_weechat_colors[foreground].foreground;
    gui_color[number]->background = gui_weechat_colors[background].foreground;
    gui_color[number]->attributes = gui_weechat_colors[foreground].attributes;
    if (gui_color[number]->string)
    {
        snprintf (gui_color[number]->string, 4,
                  "%s%02d",
                  GUI_COLOR_COLOR_STR, number);
    }
}

/*
 * gui_color_get_pair: get color pair with a WeeChat color number
 */

int
gui_color_get_pair (int num_color)
{
    int fg, bg;
    
    if ((num_color < 0) || (num_color > GUI_COLOR_NUM_COLORS - 1))
        return COLOR_WHITE;
    
    fg = gui_color[num_color]->foreground;
    bg = gui_color[num_color]->background;
    
    if (((fg == -1) || (fg == 99))
        && ((bg == -1) || (bg == 99)))
        return gui_color_last_pair;
    if ((fg == -1) || (fg == 99))
        fg = COLOR_WHITE;
    if ((bg == -1) || (bg == 99))
        bg = 0;
    
    return (bg * gui_color_num_bg) + fg + 1;
}

/*
 * gui_color_init_pairs: init color pairs
 */

void
gui_color_init_pairs ()
{
    int i, fg, bg, num_colors;
    
    /*
     * depending on terminal and $TERM value, we can have for example:
     *
     *   terminal | $TERM           | colors | pairs
     *   ---------+-----------------+--------+------
     *   urxvt    | rxvt-unicode    |     88 |   256
     *   urxvt    | xterm-256color  |    256 | 32767
     *   screen   | screen          |      8 |    64
     *   screen   | screen-256color |    256 | 32767
    */
    
    if (has_colors ())
    {
        gui_color_num_bg = (COLOR_PAIRS >= 256) ? 16 : 8;
        num_colors = (COLOR_PAIRS >= 256) ? 256 : COLOR_PAIRS;
        for (i = 1; i < num_colors; i++)
        {
            fg = (i - 1) % gui_color_num_bg;
            bg = ((i - 1) < gui_color_num_bg) ? -1 : (i - 1) / gui_color_num_bg;
            init_pair (i, fg, bg);
        }
        gui_color_last_pair = num_colors - 1;
        
        /* disable white on white, replaced by black on white */
        init_pair (gui_color_last_pair, -1, -1);
        
        /*
         * white on default bg is default (-1) (for terminals with white/light
         * background)
         */
        if (!CONFIG_BOOLEAN(config_look_color_real_white))
            init_pair (COLOR_WHITE + 1, -1, -1);
    }
}

/*
 * gui_color_init_weechat: init WeeChat colors
 */

void
gui_color_init_weechat ()
{
    int i;
    
    gui_color_build (GUI_COLOR_SEPARATOR, CONFIG_COLOR(config_color_separator), CONFIG_COLOR(config_color_chat_bg));
    
    gui_color_build (GUI_COLOR_CHAT, CONFIG_COLOR(config_color_chat), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_TIME, CONFIG_COLOR(config_color_chat_time), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_TIME_DELIMITERS, CONFIG_COLOR(config_color_chat_time_delimiters), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_PREFIX_ERROR, CONFIG_COLOR(config_color_chat_prefix[GUI_CHAT_PREFIX_ERROR]), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_PREFIX_NETWORK, CONFIG_COLOR(config_color_chat_prefix[GUI_CHAT_PREFIX_NETWORK]), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_PREFIX_ACTION, CONFIG_COLOR(config_color_chat_prefix[GUI_CHAT_PREFIX_ACTION]), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_PREFIX_JOIN, CONFIG_COLOR(config_color_chat_prefix[GUI_CHAT_PREFIX_JOIN]), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_PREFIX_QUIT, CONFIG_COLOR(config_color_chat_prefix[GUI_CHAT_PREFIX_QUIT]), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_PREFIX_MORE, CONFIG_COLOR(config_color_chat_prefix_more), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_PREFIX_SUFFIX, CONFIG_COLOR(config_color_chat_prefix_suffix), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_BUFFER, CONFIG_COLOR(config_color_chat_buffer), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_SERVER, CONFIG_COLOR(config_color_chat_server), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_CHANNEL, CONFIG_COLOR(config_color_chat_channel), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_NICK, CONFIG_COLOR(config_color_chat_nick), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_NICK_SELF, CONFIG_COLOR(config_color_chat_nick_self), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_NICK_OTHER, CONFIG_COLOR(config_color_chat_nick_other), CONFIG_COLOR(config_color_chat_bg));
    for (i = 0; i < GUI_COLOR_NICK_NUMBER; i++)
    {
        gui_color_build (GUI_COLOR_CHAT_NICK1 + i, CONFIG_COLOR(config_color_chat_nick_colors[i]), CONFIG_COLOR(config_color_chat_bg));
    }
    gui_color_build (GUI_COLOR_CHAT_HOST, CONFIG_COLOR(config_color_chat_host), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_DELIMITERS, CONFIG_COLOR(config_color_chat_delimiters), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_HIGHLIGHT, CONFIG_COLOR(config_color_chat_highlight), CONFIG_COLOR(config_color_chat_highlight_bg));
    gui_color_build (GUI_COLOR_CHAT_READ_MARKER, CONFIG_COLOR(config_color_chat_read_marker), CONFIG_COLOR(config_color_chat_read_marker_bg));
    gui_color_build (GUI_COLOR_CHAT_TEXT_FOUND, CONFIG_COLOR(config_color_chat_text_found), CONFIG_COLOR(config_color_chat_text_found_bg));
    gui_color_build (GUI_COLOR_CHAT_VALUE, CONFIG_COLOR(config_color_chat_value), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_PREFIX_BUFFER, CONFIG_COLOR(config_color_chat_prefix_buffer), CONFIG_COLOR(config_color_chat_bg));
}

/*
 * gui_color_pre_init: pre-init colors
 */

void
gui_color_pre_init ()
{
    int i;
    
    for (i = 0; i < GUI_COLOR_NUM_COLORS; i++)
    {
        gui_color[i] = NULL;
    }
}

/*
 * gui_color_init: init GUI colors
 */

void
gui_color_init ()
{
    if (has_colors ())
    {
        start_color ();
        use_default_colors ();
    }
    gui_color_init_pairs ();
    gui_color_init_weechat ();
}

/*
 * gui_color_end: end GUI colors
 */

void
gui_color_end ()
{
    int i;
    
    for (i = 0; i < GUI_COLOR_NUM_COLORS; i++)
    {
        gui_color_free (gui_color[i]);
    }
}
