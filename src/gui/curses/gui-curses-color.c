/*
 * Copyright (C) 2003-2011 Sebastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
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
#include "../../core/wee-hashtable.h"
#include "../../core/wee-list.h"
#include "../../core/wee-string.h"
#include "../../core/wee-utf8.h"
#include "../../plugins/plugin.h"
#include "../gui-buffer.h"
#include "../gui-color.h"
#include "../gui-chat.h"
#include "../gui-window.h"
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
struct t_gui_buffer *gui_color_buffer = NULL;
int gui_color_terminal_colors = 0;


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
 *                   return 1 if ok, 0 if error
 */

int
gui_color_assign (int *color, const char *color_name)
{
    int color_index, pair;
    char *error;
    
    /* search for color alias */
    pair = gui_color_palette_get_alias (color_name);
    if (pair >= 0)
    {
        *color = GUI_COLOR_PAIR_FLAG | pair;
        return 1;
    }
    
    /* is it pair number? */
    error = NULL;
    pair = (int)strtol (color_name, &error, 10);
    if (color_name[0] && error && !error[0] && (pair >= 0))
    {
        /* color_name is a number, use this pair number */
        *color = GUI_COLOR_PAIR_FLAG | pair;
        return 1;
    }
    else
    {
        /* search for basic WeeChat color */
        color_index = gui_color_search (color_name);
        if (color_index >= 0)
        {
            *color = color_index;
            return 1;
        }
    }
    
    /* color not found */
    return 0;
}

/*
 * gui_color_assign_by_diff: assign color by difference
 *                           It is called when a color option is
 *                           set with value ++X or --X, to search
 *                           another color (for example ++1 is
 *                           next color/alias in list)
 *                           return 1 if ok, 0 if error
 */

int
gui_color_assign_by_diff (int *color, const char *color_name, int diff)
{
    int index, list_size;
    struct t_weelist_item *ptr_item;
    const char *name;
    
    index = weelist_search_pos (gui_color_list_with_alias, color_name);
    if (index < 0)
        index = 0;
    
    list_size = weelist_size (gui_color_list_with_alias);
    
    diff = diff % (list_size + 1);
    
    if (diff > 0)
    {
        index = (index + diff) % (list_size + 1);
        while (index > list_size - 1)
        {
            index -= list_size;
        }
    }
    else
    {
        index = (index + list_size + diff) % list_size;
        while (index < 0)
        {
            index += list_size;
        }
    }
    
    ptr_item = weelist_get (gui_color_list_with_alias, index);
    if (!ptr_item)
        return 0;
    name = weelist_string (ptr_item);
    if (name)
        return gui_color_assign (color, name);
    
    return 0;
}

/*
 * gui_color_get_weechat_colors_number: get number of WeeChat colors
 */

int
gui_color_get_weechat_colors_number ()
{
    return GUI_CURSES_NUM_WEECHAT_COLORS;
}

/*
 * gui_color_get_last_pair: get last pair number
 */

int
gui_color_get_last_pair ()
{
    return gui_color_last_pair;
}

/*
 * gui_color_get_name: get color name
 */

const char *
gui_color_get_name (int num_color)
{
    static char color[32][16];
    static int index_color = 0;
    struct t_gui_color_palette *ptr_color_palette;
    
    if (num_color & GUI_COLOR_PAIR_FLAG)
    {
        ptr_color_palette = gui_color_palette_get (num_color & GUI_COLOR_PAIR_MASK);
        if (ptr_color_palette && ptr_color_palette->alias)
            return ptr_color_palette->alias;
        index_color = (index_color + 1) % 32;
        color[index_color][0] = '\0';
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%d", num_color & GUI_COLOR_PAIR_MASK);
        return color[index_color];
    }
    
    return gui_weechat_colors[num_color].string;
}

/*
 * gui_color_build: build a WeeChat color with foreground and background
 *                  (foreground and background must be >= 0,
 *                  if they are >= 0x10000, then it is a pair number
 *                  (pair = value & 0xFFFF))
 */

void
gui_color_build (int number, int foreground, int background)
{
    if (foreground < 0)
        foreground = 0;
    if (background < 0)
        background = 0;
    
    if (!gui_color[number])
    {
        gui_color[number] = malloc (sizeof (*gui_color[number]));
        if (!gui_color[number])
            return;
        gui_color[number]->string = malloc (4);
    }
    
    if (foreground & GUI_COLOR_PAIR_FLAG)
    {
        gui_color[number]->foreground = foreground;
        gui_color[number]->background = 0;
        gui_color[number]->attributes = 0;
    }
    else
    {
        if (background & GUI_COLOR_PAIR_FLAG)
            background = 0;
        gui_color[number]->foreground = gui_weechat_colors[foreground].foreground;
        gui_color[number]->background = gui_weechat_colors[background].foreground;
        gui_color[number]->attributes = gui_weechat_colors[foreground].attributes;
    }
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
    
    if ((fg > 0) && (fg & GUI_COLOR_PAIR_FLAG))
        return fg & GUI_COLOR_PAIR_MASK;
    
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
 * gui_color_init_pair: init a color pair
 */

void
gui_color_init_pair (int number)
{
    struct t_gui_color_palette *ptr_color_palette;
    int fg, bg;
    
    if ((number >= 1) && (number <= gui_color_last_pair))
    {
        if (gui_color_terminal_colors)
        {
            init_pair (number, number, -1);
        }
        else
        {
            ptr_color_palette = gui_color_palette_get (number);
            if (ptr_color_palette)
            {
                init_pair (number,
                           ptr_color_palette->foreground,
                           ptr_color_palette->background);
            }
            else
            {
                fg = (number - 1) % gui_color_num_bg;
                bg = ((number - 1) < gui_color_num_bg) ? -1 : (number - 1) / gui_color_num_bg;
                init_pair (number, fg, bg);
            }
        }
    }
}

/*
 * gui_color_init_pairs: init color pairs
 */

void
gui_color_init_pairs ()
{
    int i, num_colors;
    struct t_gui_color_palette *ptr_color_palette;
    
    /*
     * depending on $TERM value, we can have for example:
     *
     *   $TERM                             | colors | pairs
     *   ----------------------------------+--------+------
     *   rxvt-unicode, xterm,...           |     88 |   256
     *   rxvt-256color, xterm-256color,... |    256 | 32767
     *   screen                            |      8 |    64
     *   screen-256color                   |    256 | 32767
     */
    
    if (has_colors ())
    {
        gui_color_num_bg = (COLOR_PAIRS >= 256) ? 16 : 8;
        num_colors = (COLOR_PAIRS >= 256) ? 256 : COLOR_PAIRS;
        gui_color_last_pair = num_colors - 1;
        
        /* WeeChat pairs */
        for (i = 1; i < num_colors; i++)
        {
            gui_color_init_pair (i);
        }
        
        if (!gui_color_terminal_colors)
        {
            /* disable white on white, replaced by black on white */
            ptr_color_palette = gui_color_palette_get (gui_color_last_pair);
            if (!ptr_color_palette)
                init_pair (gui_color_last_pair, -1, -1);
            
            /*
             * white on default bg is default (-1) (for terminals with white/light
             * background)
             */
            if (!CONFIG_BOOLEAN(config_look_color_real_white))
            {
                ptr_color_palette = gui_color_palette_get (COLOR_WHITE);
                if (!ptr_color_palette)
                    init_pair (COLOR_WHITE + 1, -1, -1);
            }
        }
    }
}

/*
 * gui_color_init_weechat: init WeeChat colors
 */

void
gui_color_init_weechat ()
{
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
    gui_color_build (GUI_COLOR_CHAT_HOST, CONFIG_COLOR(config_color_chat_host), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_DELIMITERS, CONFIG_COLOR(config_color_chat_delimiters), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_HIGHLIGHT, CONFIG_COLOR(config_color_chat_highlight), CONFIG_COLOR(config_color_chat_highlight_bg));
    gui_color_build (GUI_COLOR_CHAT_READ_MARKER, CONFIG_COLOR(config_color_chat_read_marker), CONFIG_COLOR(config_color_chat_read_marker_bg));
    gui_color_build (GUI_COLOR_CHAT_TEXT_FOUND, CONFIG_COLOR(config_color_chat_text_found), CONFIG_COLOR(config_color_chat_text_found_bg));
    gui_color_build (GUI_COLOR_CHAT_VALUE, CONFIG_COLOR(config_color_chat_value), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_PREFIX_BUFFER, CONFIG_COLOR(config_color_chat_prefix_buffer), CONFIG_COLOR(config_color_chat_bg));
    
    /*
     * define old nick colors for compatibility on /upgrade with previous
     * versions: these colors have been removed in version 0.3.4 and replaced
     * by new option "weechat.color.chat_nick_colors", which is a list of
     * colors (without limit on number of colors)
     */
    gui_color_build (GUI_COLOR_CHAT_NICK1_OBSOLETE,  gui_color_search ("cyan"), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_NICK2_OBSOLETE,  gui_color_search ("magenta"), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_NICK3_OBSOLETE,  gui_color_search ("green"), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_NICK4_OBSOLETE,  gui_color_search ("brown"), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_NICK5_OBSOLETE,  gui_color_search ("lightblue"), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_NICK6_OBSOLETE,  gui_color_search ("default"), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_NICK7_OBSOLETE,  gui_color_search ("lightcyan"), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_NICK8_OBSOLETE,  gui_color_search ("lightmagenta"), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_NICK9_OBSOLETE,  gui_color_search ("lightgreen"), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_NICK10_OBSOLETE, gui_color_search ("blue"), CONFIG_COLOR(config_color_chat_bg));
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
    gui_color_palette_build_aliases ();
}

/*
 * gui_color_display_terminal_colors: display terminal colors
 *                                    This is called by command line option
 *                                    "-c" / "--colors"
 */

void
gui_color_display_terminal_colors ()
{
    int lines, line, col, color;
    int colors, color_pairs, change_color;
    char str_line[1024], str_color[64];
    
    colors = 0;
    color_pairs = 0;
    change_color = 0;
    
    initscr ();
    if (has_colors ())
    {
        start_color ();
        use_default_colors ();
        colors = COLORS;
        color_pairs = COLOR_PAIRS;
        change_color = can_change_color () ? 1 : 0;
        refresh ();
        endwin ();
    }
    printf ("\n");
    printf ("%s $TERM=%s  COLORS: %d, COLOR_PAIRS: %d, "
            "can_change_color: %s\n",
            _("Terminal infos:"),
            getenv ("TERM"), colors, color_pairs,
            (change_color) ? "yes" : "no");
    if (colors == 0)
    {
        printf ("%s\n", _("No color support in terminal."));
    }
    else
    {
        printf ("\n");
        printf ("%s\n", _("Default colors:"));
        printf ("------------------------------------------------------------"
                "--------------------\n");
        lines = (colors < 16) ? colors : 16;
        for (line = 0; line < lines; line++)
        {
            str_line[0] = '\0';
            for (col = 0; col < 16; col++)
            {
                color = (col * 16) + line;
                if (color < colors)
                {
                    snprintf (str_color, sizeof (str_color),
                              "\33[0;38;5;%dm %03d ", color, color);
                    strcat (str_line, str_color);
                }
            }
            printf ("%s\n", str_line);
        }
        printf ("\33[0m");
        printf ("------------------------------------------------------------"
                "--------------------\n");
    }
    printf ("\n");
}

/*
 * gui_color_buffer_display: display content of color buffer
 */

void
gui_color_buffer_display ()
{
    int y, i, lines, line, col, color, max_color;
    int colors, color_pairs, change_color, num_items;
    char str_line[1024], str_color[64], str_rgb[64], **items;
    struct t_gui_color_palette *color_palette;
    
    if (!gui_color_buffer)
        return;
    
    gui_buffer_clear (gui_color_buffer);
    
    colors = 0;
    color_pairs = 0;
    change_color = 0;
    
    if (has_colors ())
    {
        colors = COLORS;
        color_pairs = COLOR_PAIRS;
        change_color = can_change_color () ? 1 : 0;
    }
    
    /* set title buffer */
    gui_buffer_set_title (gui_color_buffer,
                          _("WeeChat colors | Actions: [R] Refresh "
                            "[Q] Close buffer | Keys: [alt-c] Toggle colors"));
    
    /* display terminal/colors infos */
    y = 0;
    gui_chat_printf_y (gui_color_buffer, y++,
                       "$TERM=%s  COLORS: %d, COLOR_PAIRS: %d, "
                       "can_change_color: %s",
                       getenv ("TERM"),
                       colors,
                       color_pairs,
                       (change_color) ? "yes" : "no");
    
    /* display palette of colors */
    y++;
    gui_chat_printf_y (gui_color_buffer, y++,
                       (gui_color_terminal_colors) ?
                       _("Terminal colors:") :
                       _("WeeChat colors:"));
    gui_chat_printf_y (gui_color_buffer, y++,
                       " %s%s00000 000 %s%s",
                       GUI_COLOR_COLOR_STR,
                       GUI_COLOR_PAIR_STR,
                       GUI_COLOR(GUI_COLOR_CHAT),
                       _("fixed color"));
    max_color = (gui_color_terminal_colors) ? colors - 1 : gui_color_last_pair;
    if (max_color > 255)
        max_color = 255;
    lines = (max_color <= 64) ? 8 : 16;
    for (line = 0; line < lines; line++)
    {
        str_line[0] = '\0';
        for (col = 0; col < 16; col++)
        {
            color = (col * lines) + line + 1;
            if (color <= max_color)
            {
                snprintf (str_color, sizeof (str_color),
                          "%s%s%05d %03d ",
                          GUI_COLOR_COLOR_STR,
                          GUI_COLOR_PAIR_STR,
                          color,
                          color);
                strcat (str_line, str_color);
            }
            else
            {
                snprintf (str_color, sizeof (str_color),
                          "%s     ",
                          GUI_COLOR(GUI_COLOR_CHAT));
                strcat (str_line, str_color);
            }
        }
        gui_chat_printf_y (gui_color_buffer, y++,
                           " %s",
                           str_line);
    }
    
    /* display WeeChat basic colors */
    y++;
    gui_chat_printf_y (gui_color_buffer, y++,
                       _("WeeChat basic colors:"));
    str_line[0] = '\0';
    for (i = 0; i < GUI_CURSES_NUM_WEECHAT_COLORS; i++)
    {
        if (gui_color_terminal_colors)
        {
            snprintf (str_color, sizeof (str_color),
                      " %s",
                      gui_weechat_colors[i].string);
        }
        else
        {
            snprintf (str_color, sizeof (str_color),
                      "%s%s%02d %s",
                      GUI_COLOR_COLOR_STR,
                      GUI_COLOR_FG_STR,
                      i,
                      gui_weechat_colors[i].string);
        }
        if (gui_chat_strlen_screen (str_line) + gui_chat_strlen_screen (str_color) > 80)
        {
            gui_chat_printf_y (gui_color_buffer, y++,
                               " %s", str_line);
            str_line[0] = '\0';
        }
        strcat (str_line, str_color);
    }
    if (str_line[0])
    {
        gui_chat_printf_y (gui_color_buffer, y++,
                           " %s", str_line);
    }
    
    /* display nick colors */
    y++;
    gui_chat_printf_y (gui_color_buffer, y++,
                       _("Nick colors:"));
    items = string_split (CONFIG_STRING(config_color_chat_nick_colors),
                          ",", 0, 0, &num_items);
    if (items)
    {
        str_line[0] = '\0';
        for (i = 0; i < num_items; i++)
        {
            if (gui_color_terminal_colors)
            {
                snprintf (str_color, sizeof (str_color),
                          " %s",
                          items[i]);
            }
            else
            {
                snprintf (str_color, sizeof (str_color),
                          "%s %s",
                          gui_color_get_custom (items[i]),
                          items[i]);
            }
            if (gui_chat_strlen_screen (str_line) + gui_chat_strlen_screen (str_color) > 80)
            {
                gui_chat_printf_y (gui_color_buffer, y++,
                                   " %s", str_line);
                str_line[0] = '\0';
            }
            strcat (str_line, str_color);
        }
        if (str_line[0])
        {
            gui_chat_printf_y (gui_color_buffer, y++,
                               " %s", str_line);
        }
        string_free_split (items);
    }
    
    /* display palette colors */
    if (hashtable_get_integer (gui_color_hash_palette_color,
                               "items_count") > 0)
    {
        y++;
        gui_chat_printf_y (gui_color_buffer, y++,
                           _("Palette colors:"));
        for (i = 1; i <= gui_color_last_pair; i++)
        {
            color_palette = gui_color_palette_get (i);
            if (color_palette)
            {
                str_color[0] = '\0';
                if (!gui_color_terminal_colors)
                {
                    snprintf (str_color, sizeof (str_color),
                              "%s%s%05d",
                              GUI_COLOR_COLOR_STR,
                              GUI_COLOR_PAIR_STR,
                              i);
                }
                snprintf (str_rgb, sizeof (str_rgb), "              ");
                if ((color_palette->r >= 0) && (color_palette->g >= 0)
                    && (color_palette->b >= 0))
                {
                    snprintf (str_rgb, sizeof (str_rgb),
                              "%04d/%04d/%04d",
                              color_palette->r,
                              color_palette->g,
                              color_palette->b);
                }
                gui_chat_printf_y (gui_color_buffer, y++,
                                   " %5d: %s%5d,%-5d %s %s",
                                   i,
                                   str_color,
                                   color_palette->foreground,
                                   color_palette->background,
                                   str_rgb,
                                   (color_palette->alias) ?
                                   color_palette->alias : "");
            }
        }
    }
}

/*
 * gui_color_switch_colrs: switch between WeeChat and terminal colors
 */

void
gui_color_switch_colors ()
{
    gui_color_terminal_colors ^= 1;
    gui_color_init_pairs ();
    gui_color_buffer_display ();
}

/*
 * gui_color_buffer_input_cb: input callback for color buffer
 */

int
gui_color_buffer_input_cb (void *data, struct t_gui_buffer *buffer,
                           const char *input_data)
{
    /* make C compiler happy */
    (void) data;

    if (string_strcasecmp (input_data, "r") == 0)
    {
        gui_color_buffer_display ();
    }
    else if (string_strcasecmp (input_data, "q") == 0)
    {
        gui_buffer_close (buffer);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * gui_color_buffer_close_cb: close callback for color buffer
 */

int
gui_color_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    
    gui_color_buffer = NULL;
    
    return WEECHAT_RC_OK;
}

/*
 * gui_color_buffer_assign: assign color buffer to pointer if it is not yet set
 */

void
gui_color_buffer_assign ()
{
    if (!gui_color_buffer)
    {
        gui_color_buffer = gui_buffer_search_by_name (NULL, "color");
        if (gui_color_buffer)
        {
            gui_color_buffer->input_callback = &gui_color_buffer_input_cb;
            gui_color_buffer->close_callback = &gui_color_buffer_close_cb;
        }
    }
}

/*
 * gui_color_buffer_open: open a buffer to display colors
 */

void
gui_color_buffer_open ()
{
    if (!gui_color_buffer)
    {
        gui_color_buffer = gui_buffer_new (NULL, "color",
                                           &gui_color_buffer_input_cb, NULL,
                                           &gui_color_buffer_close_cb, NULL);
        if (gui_color_buffer)
        {
            gui_buffer_set (gui_color_buffer, "type", "free");
            gui_buffer_set (gui_color_buffer, "localvar_set_no_log", "1");
            gui_buffer_set (gui_color_buffer, "key_bind_meta-c", "/color switch");
        }
    }
    
    if (!gui_color_buffer)
        return;
    
    gui_window_switch_to_buffer (gui_current_window, gui_color_buffer, 1);
    
    gui_color_buffer_display ();
}

/*
 * gui_color_palette_add_alias_cb: add an alias in hashtable with aliases
 */

void
gui_color_palette_add_alias_cb (void *data,
                                struct t_hashtable *hashtable,
                                const void *key, const void *value)
{
    struct t_gui_color_palette *color_palette;
    char *error;
    int number;
    
    /* make C compiler happy */
    (void) data;
    (void) hashtable;
    
    color_palette = (struct t_gui_color_palette *)value;
    
    if (color_palette && color_palette->alias)
    {
        error = NULL;
        number = (int)strtol ((char *)key, &error, 10);
        if (error && !error[0])
        {
            hashtable_set (gui_color_hash_palette_alias,
                           color_palette->alias,
                           &number);
        }
        weelist_add (gui_color_list_with_alias, color_palette->alias,
                     WEECHAT_LIST_POS_END, NULL);
    }
}

/*
 * gui_color_palette_build_aliases: build aliases for palette
 */

void
gui_color_palette_build_aliases ()
{
    int i;
    
    if (!gui_color_hash_palette_alias || !gui_color_list_with_alias
        || !gui_color_hash_palette_color)
    {
        gui_color_palette_alloc_structs ();
    }
    
    hashtable_remove_all (gui_color_hash_palette_alias);
    weelist_remove_all (gui_color_list_with_alias);
    for (i = 0; i < GUI_CURSES_NUM_WEECHAT_COLORS; i++)
    {
        weelist_add (gui_color_list_with_alias,
                     gui_weechat_colors[i].string,
                     WEECHAT_LIST_POS_END,
                     NULL);
    }
    hashtable_map (gui_color_hash_palette_color,
                   &gui_color_palette_add_alias_cb, NULL);
}

/*
 * gui_color_palette_new: create a new color in palette
 */

struct t_gui_color_palette *
gui_color_palette_new (int number, const char *value)
{
    struct t_gui_color_palette *new_color_palette;
    char **items, *pos, *pos2, *error1, *error2, *error3;
    char *str_alias, *str_pair, *str_rgb, str_number[64];
    int num_items, i, fg, bg, r, g, b;
    
    if (!value)
        return NULL;
    
    new_color_palette = malloc (sizeof (*new_color_palette));
    if (new_color_palette)
    {
        new_color_palette->alias = NULL;
        new_color_palette->foreground = number;
        new_color_palette->background = -1;
        new_color_palette->r = -1;
        new_color_palette->g = -1;
        new_color_palette->b = -1;
        
        str_alias = NULL;
        str_pair = NULL;
        str_rgb = NULL;
        
        items = string_split (value, ";", 0, 0, &num_items);
        if (items)
        {
            for (i = 0; i < num_items; i++)
            {
                pos = strchr (items[i], ',');
                if (pos)
                    str_pair = items[i];
                else
                {
                    pos = strchr (items[i], '/');
                    if (pos)
                        str_rgb = items[i];
                    else
                        str_alias = items[i];
                }
            }
            
            if (str_alias)
            {
                new_color_palette->alias = strdup (str_alias);
            }
            
            if (str_pair)
            {
                pos = strchr (str_pair, ',');
                if (pos)
                {
                    pos[0] = '\0';
                    error1 = NULL;
                    fg = (int)strtol (str_pair, &error1, 10);
                    error2 = NULL;
                    bg = (int)strtol (pos + 1, &error2, 10);
                    if (error1 && !error1[0] && error2 && !error2[0]
                        && (fg >= -1) && (bg >= -1))
                    {
                        new_color_palette->foreground = fg;
                        new_color_palette->background = bg;
                    }
                }
            }
            
            if (str_rgb)
            {
                pos = strchr (str_rgb, '/');
                if (pos)
                {
                    pos[0] = '\0';
                    pos2 = strchr (pos + 1, '/');
                    if (pos2)
                    {
                        pos2[0] = '\0';
                        error1 = NULL;
                        r = (int)strtol (str_rgb, &error1, 10);
                        error2 = NULL;
                        g = (int)strtol (pos + 1, &error2, 10);
                        error3 = NULL;
                        b = (int)strtol (pos2 + 1, &error3, 10);
                        if (error1 && !error1[0] && error2 && !error2[0]
                            && error3 && !error3[0]
                            && (r >= 0) && (r <= 1000)
                            && (g >= 0) && (g <= 1000)
                            && (b >= 0) && (b <= 1000))
                        {
                            new_color_palette->r = r;
                            new_color_palette->g = g;
                            new_color_palette->b = b;
                        }
                    }
                }
            }
            string_free_split (items);
        }
        if (!new_color_palette->alias)
        {
            snprintf (str_number, sizeof (str_number), "%d", number);
            new_color_palette->alias = strdup (str_number);
        }
    }
    
    return new_color_palette;
}

/*
 * gui_color_palette_free: free a color in palette
 */

void
gui_color_palette_free (struct t_gui_color_palette *color_palette)
{
    if (!color_palette)
        return;
    
    if (color_palette->alias)
        free (color_palette->alias);
    
    free (color_palette);
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
    gui_color_palette_free_structs ();
}
