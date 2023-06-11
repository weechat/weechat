/*
 * gui-curses-color.c - color functions for Curses GUI
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-hashtable.h"
#include "../../core/wee-hook.h"
#include "../../core/wee-list.h"
#include "../../core/wee-string.h"
#include "../../core/wee-utf8.h"
#include "../../plugins/plugin.h"
#include "../gui-buffer.h"
#include "../gui-color.h"
#include "../gui-chat.h"
#include "../gui-window.h"
#include "gui-curses.h"
#include "gui-curses-color.h"


#define GUI_COLOR_TIMER_TERM_COLORS 10

struct t_gui_color gui_weechat_colors_bold[GUI_CURSES_NUM_WEECHAT_COLORS + 1] =
{ { -1,                -1,                0,      "default"      },
  { COLOR_BLACK,       COLOR_BLACK,       0,      "black"        },
  { COLOR_BLACK,       COLOR_BLACK + 8,   A_BOLD, "darkgray"     },
  { COLOR_RED,         COLOR_RED,         0,      "red"          },
  { COLOR_RED,         COLOR_RED + 8,     A_BOLD, "lightred"     },
  { COLOR_GREEN,       COLOR_GREEN,       0,      "green"        },
  { COLOR_GREEN,       COLOR_GREEN + 8,   A_BOLD, "lightgreen"   },
  { COLOR_YELLOW,      COLOR_YELLOW,      0,      "brown"        },
  { COLOR_YELLOW,      COLOR_YELLOW + 8,  A_BOLD, "yellow"       },
  { COLOR_BLUE,        COLOR_BLUE,        0,      "blue"         },
  { COLOR_BLUE,        COLOR_BLUE + 8,    A_BOLD, "lightblue"    },
  { COLOR_MAGENTA,     COLOR_MAGENTA,     0,      "magenta"      },
  { COLOR_MAGENTA,     COLOR_MAGENTA + 8, A_BOLD, "lightmagenta" },
  { COLOR_CYAN,        COLOR_CYAN,        0,      "cyan"         },
  { COLOR_CYAN,        COLOR_CYAN + 8,    A_BOLD, "lightcyan"    },
  { COLOR_WHITE,       COLOR_WHITE,       0,      "gray"         },
  { COLOR_WHITE,       COLOR_WHITE + 8,   A_BOLD, "white"        },
  { 0,                 0,                 0,      NULL           }
};
struct t_gui_color gui_weechat_colors_no_bold[GUI_CURSES_NUM_WEECHAT_COLORS + 1] =
{ { -1,                -1,                0,      "default"      },
  { COLOR_BLACK,       COLOR_BLACK,       0,      "black"        },
  { COLOR_BLACK + 8,   COLOR_BLACK + 8,   0,      "darkgray"     },
  { COLOR_RED,         COLOR_RED,         0,      "red"          },
  { COLOR_RED + 8,     COLOR_RED + 8,     0,      "lightred"     },
  { COLOR_GREEN,       COLOR_GREEN,       0,      "green"        },
  { COLOR_GREEN + 8,   COLOR_GREEN + 8,   0,      "lightgreen"   },
  { COLOR_YELLOW,      COLOR_YELLOW,      0,      "brown"        },
  { COLOR_YELLOW + 8,  COLOR_YELLOW + 8,  0,      "yellow"       },
  { COLOR_BLUE,        COLOR_BLUE,        0,      "blue"         },
  { COLOR_BLUE + 8,    COLOR_BLUE + 8,    0,      "lightblue"    },
  { COLOR_MAGENTA,     COLOR_MAGENTA,     0,      "magenta"      },
  { COLOR_MAGENTA + 8, COLOR_MAGENTA + 8, 0,      "lightmagenta" },
  { COLOR_CYAN,        COLOR_CYAN,        0,      "cyan"         },
  { COLOR_CYAN + 8,    COLOR_CYAN + 8,    0,      "lightcyan"    },
  { COLOR_WHITE,       COLOR_WHITE,       0,      "gray"         },
  { COLOR_WHITE + 8,   COLOR_WHITE + 8,   0,      "white"        },
  { 0,                 0,                 0,      NULL           }
};
struct t_gui_color *gui_weechat_colors = gui_weechat_colors_bold;

/* terminal colors */
int gui_color_term_has_colors = 0;       /* terminal supports colors?       */
int gui_color_term_colors = 0;           /* number of colors in terminal    */
int gui_color_term_color_pairs = 0;      /* number of pairs in terminal     */
int gui_color_term_can_change_color = 0; /* change color allowed in term?   */
int gui_color_use_term_colors = 0;       /* temp. use of terminal colors?   */
short *gui_color_term_color_content = NULL; /* content of colors (r/b/g)    */

/* pairs */
int gui_color_num_pairs = 63;            /* number of pairs used by WeeChat */
short *gui_color_pairs = NULL;           /* table with pair for each fg+bg  */
int gui_color_pairs_used = 0;            /* number of pairs currently used  */
int gui_color_warning_pairs_full = 0;    /* warning displayed?              */
int gui_color_pairs_auto_reset = 0;         /* auto reset of pairs needed   */
int gui_color_pairs_auto_reset_pending = 0; /* auto reset is pending        */
time_t gui_color_pairs_auto_reset_last = 0; /* time of last auto reset      */

/* color buffer */
struct t_gui_buffer *gui_color_buffer = NULL; /* buffer with colors         */
int gui_color_buffer_extra_info = 0;          /* display extra info?        */
int gui_color_buffer_refresh_needed = 0;      /* refresh needed on buffer?  */
struct t_hook *gui_color_hook_timer = NULL;   /* timer for terminal colors  */
int gui_color_timer = 0;                      /* timer in seconds           */


/*
 * Searches for a color by name.
 *
 * Returns index of color in WeeChat colors table, -1 if not found.
 */

int
gui_color_search (const char *color_name)
{
    int i;

    if (!color_name)
        return -1;

    for (i = 0; gui_weechat_colors[i].string; i++)
    {
        if (strcmp (gui_weechat_colors[i].string, color_name) == 0)
            return i;
    }

    /* color not found */
    return -1;
}

/*
 * Searches for a color by index.
 *
 * Returns name of color in WeeChat colors table, NULL if not found.
 */

const char *
gui_color_search_index (int index)
{
    int i;

    for (i = 0; gui_weechat_colors[i].string; i++)
    {
        if (i == index)
            return gui_weechat_colors[i].string;
    }

    /* color not found */
    return NULL;
}

/*
 * Get Curses attributes corresponding to extended attributes flags in a color.
 */

int
gui_color_get_gui_attrs (int color)
{
    int attributes;

    attributes = 0;

    if (color & GUI_COLOR_EXTENDED_BLINK_FLAG)
        attributes |= A_BLINK;
    if (color & GUI_COLOR_EXTENDED_DIM_FLAG)
        attributes |= A_DIM;
    if (color & GUI_COLOR_EXTENDED_BOLD_FLAG)
        attributes |= A_BOLD;
    if (color & GUI_COLOR_EXTENDED_REVERSE_FLAG)
        attributes |= A_REVERSE;
    if (color & GUI_COLOR_EXTENDED_ITALIC_FLAG)
        attributes |= A_ITALIC;
    if (color & GUI_COLOR_EXTENDED_UNDERLINE_FLAG)
        attributes |= A_UNDERLINE;
    return attributes;
}

/*
 * Get extended flags corresponding to Curses attributes in a color.
 */

int
gui_color_get_extended_flags (int attrs)
{
    int flags;

    flags = 0;

    if (attrs & A_BLINK)
        flags |= GUI_COLOR_EXTENDED_BLINK_FLAG;
    if (attrs & A_DIM)
        flags |= GUI_COLOR_EXTENDED_DIM_FLAG;
    if (attrs & A_BOLD)
        flags |= GUI_COLOR_EXTENDED_BOLD_FLAG;
    if (attrs & A_REVERSE)
        flags |= GUI_COLOR_EXTENDED_REVERSE_FLAG;
    if (attrs & A_ITALIC)
        flags |= GUI_COLOR_EXTENDED_ITALIC_FLAG;
    if (attrs & A_UNDERLINE)
        flags |= GUI_COLOR_EXTENDED_UNDERLINE_FLAG;

    return flags;
}

/*
 * Assigns a WeeChat color (read from configuration).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_color_assign (int *color, const char *color_name)
{
    int flag, extra_attr, color_index, number;
    char *error;

    /* read extended attributes */
    extra_attr = 0;
    while ((flag = gui_color_attr_get_flag (color_name[0])) > 0)
    {
        extra_attr |= flag;
        color_name++;
    }

    /* is it a color alias? */
    number = gui_color_palette_get_alias (color_name);
    if (number >= 0)
    {
        *color = number | GUI_COLOR_EXTENDED_FLAG | extra_attr;
        return 1;
    }

    /* is it a color number? */
    error = NULL;
    number = (int)strtol (color_name, &error, 10);
    if (color_name[0] && error && !error[0] && (number >= 0))
    {
        /* color_name is a number, use this color number */
        if (number > GUI_COLOR_EXTENDED_MAX)
            number = GUI_COLOR_EXTENDED_MAX;
        *color = number | GUI_COLOR_EXTENDED_FLAG | extra_attr;
        return 1;
    }
    else
    {
        /* search for basic WeeChat color */
        color_index = gui_color_search (color_name);
        if (color_index >= 0)
        {
            *color = color_index | extra_attr;
            return 1;
        }
    }

    /* color not found */
    return 0;
}

/*
 * Assigns color by difference.
 *
 * It is called when a color option is set with value ++X or --X, to search
 * another color (for example ++1 is next color/alias in list).
 *
 * Returns:
 *   1: OK
 *   0: error
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
 * Gets number of WeeChat colors.
 */

int
gui_color_get_weechat_colors_number ()
{
    return GUI_CURSES_NUM_WEECHAT_COLORS;
}

/*
 * Gets number of colors supported by terminal.
 */

int
gui_color_get_term_colors ()
{
    return gui_color_term_colors;
}

/*
 * Gets number of color pairs supported by terminal.
 */

int
gui_color_get_term_color_pairs ()
{
    return gui_color_term_color_pairs;
}

/*
 * Gets current pairs as arrays (one array for foregrounds, another for
 * backgrounds).
 *
 * Each array has "gui_color_num_pairs+1" entries. Pairs not used have value -2
 * in both arrays.
 */

void
gui_color_get_pairs_arrays (short **foregrounds, short **backgrounds)
{
    int i, fg, bg, index;

    if (!foregrounds || !backgrounds)
        return;

    *foregrounds = NULL;
    *backgrounds = NULL;

    *foregrounds = malloc (sizeof (*foregrounds[0]) * (gui_color_num_pairs + 1));
    if (!*foregrounds)
        goto error;
    *backgrounds = malloc (sizeof (*backgrounds[0]) * (gui_color_num_pairs + 1));
    if (!*backgrounds)
        goto error;

    for (i = 0; i <= gui_color_num_pairs; i++)
    {
        (*foregrounds)[i] = -2;
        (*backgrounds)[i] = -2;
    }

    for (bg = -1; bg <= gui_color_term_colors; bg++)
    {
        for (fg = -1; fg <= gui_color_term_colors; fg++)
        {
            index = ((bg + 1) * (gui_color_term_colors + 2)) + (fg + 1);
            if ((gui_color_pairs[index] >= 1)
                && (gui_color_pairs[index] <= gui_color_num_pairs))
            {
                (*foregrounds)[gui_color_pairs[index]] = fg;
                (*backgrounds)[gui_color_pairs[index]] = bg;
            }
        }
    }

    return;

error:
    if (*foregrounds)
    {
        free (*foregrounds);
        *foregrounds = NULL;
    }
    if (*backgrounds)
    {
        free (*backgrounds);
        *backgrounds = NULL;
    }
}

/*
 * Displays a warning when no more pair is available in table.
 */

int
gui_color_timer_warning_pairs_full (const void *pointer, void *data,
                                    int remaining_calls)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) remaining_calls;

    gui_chat_printf (NULL,
                     _("Warning: the %d color pairs are used, do "
                       "\"/color reset\" to remove unused pairs"),
                     gui_color_num_pairs);

    return WEECHAT_RC_OK;
}

/*
 * Gets a pair with given foreground/background colors.
 *
 * If no pair is found for fg/bg, a new pair is created.
 *
 * Returns a value between 0 and COLOR_PAIRS-1.
 */

int
gui_color_get_pair (int fg, int bg)
{
    int index;

    /* only one color when displaying terminal colors */
    if (gui_color_use_term_colors)
        return COLOR_WHITE;

    /* if invalid color, use nearest color or default fg/bg */
    if (fg >= gui_color_term_colors)
    {
        /* find nearest color */
        if ((fg >= 0) && (fg <= 255))
        {
            fg = gui_color_convert_rgb_to_term (gui_color_term256[fg],
                                                gui_color_term_colors);
        }
        else
        {
            /* fallback to default foreground */
            fg = -1;
        }
    }
    if (bg >= gui_color_term_colors)
    {
        if ((bg >= 0) && (bg <= 255))
        {
            bg = gui_color_convert_rgb_to_term (gui_color_term256[bg],
                                                gui_color_term_colors);
        }
        else
        {
            /* fallback to default background */
            bg = -1;
        }
    }

    /* compute index for gui_color_pairs with foreground and background */
    index = ((bg + 1) * (gui_color_term_colors + 2)) + (fg + 1);

    /* pair not allocated for this fg/bg? */
    if (gui_color_pairs[index] == 0)
    {
        if (gui_color_pairs_used >= gui_color_num_pairs)
        {
            /* oh no, no more pair available! */
            if (!gui_color_warning_pairs_full
                && (CONFIG_INTEGER(config_look_color_pairs_auto_reset) < 0))
            {
                /* display warning if auto reset of pairs is disabled */
                hook_timer (NULL, 1, 0, 1,
                            &gui_color_timer_warning_pairs_full, NULL, NULL);
                gui_color_warning_pairs_full = 1;
            }
            return 1;
        }

        /* create a new pair if no pair exists for this fg/bg */
        gui_color_pairs_used++;
        gui_color_pairs[index] = gui_color_pairs_used;
        init_pair (gui_color_pairs_used, fg, bg);
        if ((gui_color_num_pairs > 1) && !gui_color_pairs_auto_reset_pending
            && (CONFIG_INTEGER(config_look_color_pairs_auto_reset) >= 0)
            && (gui_color_num_pairs - gui_color_pairs_used <= CONFIG_INTEGER(config_look_color_pairs_auto_reset)))
        {
            gui_color_pairs_auto_reset = 1;
        }
        gui_color_buffer_refresh_needed = 1;
    }

    return gui_color_pairs[index];
}

/*
 * Gets color pair with a WeeChat color number.
 */

int
gui_color_weechat_get_pair (int weechat_color)
{
    int fg, bg;

    if ((weechat_color < 0) || (weechat_color > GUI_COLOR_NUM_COLORS - 1))
    {
        fg = -1;
        bg = -1;
    }
    else
    {
        fg = gui_color[weechat_color]->foreground;
        bg = gui_color[weechat_color]->background;

        if ((fg > 0) && (fg & GUI_COLOR_EXTENDED_FLAG))
            fg &= GUI_COLOR_EXTENDED_MASK;
        if ((bg > 0) && (bg & GUI_COLOR_EXTENDED_FLAG))
            bg &= GUI_COLOR_EXTENDED_MASK;
    }

    return gui_color_get_pair (fg, bg);
}

/*
 * Gets color name.
 */

const char *
gui_color_get_name (int num_color)
{
    static char color[16][64];
    static int index_color = 0;
    char str_attr[8];
    struct t_gui_color_palette *ptr_color_palette;

    /* init color string */
    index_color = (index_color + 1) % 16;
    color[index_color][0] = '\0';

    /* build string with extra-attributes */
    gui_color_attr_build_string (num_color, str_attr);

    if (num_color & GUI_COLOR_EXTENDED_FLAG)
    {
        /* search alias */
        ptr_color_palette = gui_color_palette_get (num_color & GUI_COLOR_EXTENDED_MASK);
        if (ptr_color_palette && ptr_color_palette->alias)
        {
            /* alias */
            snprintf (color[index_color], sizeof (color[index_color]),
                      "%s%s", str_attr, ptr_color_palette->alias);
        }
        else
        {
            /* color number */
            snprintf (color[index_color], sizeof (color[index_color]),
                      "%s%d", str_attr, num_color & GUI_COLOR_EXTENDED_MASK);
        }
    }
    else
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%s%s",
                  str_attr,
                  gui_weechat_colors[num_color & GUI_COLOR_EXTENDED_MASK].string);
    }

    return color[index_color];
}

/*
 * Builds a WeeChat color with foreground and background.
 *
 * Foreground and background must be >= 0 and can be a WeeChat or extended
 * color, with optional attributes for foreground.
 */

void
gui_color_build (int number, int foreground, int background)
{
    char str_color[128];

    if (foreground < 0)
        foreground = 0;
    if (background < 0)
        background = 0;

    /* allocate color */
    if (!gui_color[number])
    {
        gui_color[number] = malloc (sizeof (*gui_color[number]));
        if (!gui_color[number])
            return;
    }

    /* set foreground and attributes */
    if (foreground & GUI_COLOR_EXTENDED_FLAG)
    {
        gui_color[number]->foreground = foreground & GUI_COLOR_EXTENDED_MASK;
        gui_color[number]->attributes = 0;
    }
    else
    {
        gui_color[number]->foreground = gui_weechat_colors[foreground & GUI_COLOR_EXTENDED_MASK].foreground;
        gui_color[number]->attributes = gui_weechat_colors[foreground & GUI_COLOR_EXTENDED_MASK].attributes;
    }
    gui_color[number]->attributes |= gui_color_get_gui_attrs (foreground);

    /* set background */
    if (background & GUI_COLOR_EXTENDED_FLAG)
        gui_color[number]->background = background & GUI_COLOR_EXTENDED_MASK;
    else
        gui_color[number]->background = gui_weechat_colors[background & GUI_COLOR_EXTENDED_MASK].background;

    /* set string */
    snprintf (str_color, sizeof (str_color),
              "%c%02d",
              GUI_COLOR_COLOR_CHAR, number);
    gui_color[number]->string = strdup (str_color);
}

/*
 * Initializes color variables using terminal infos.
 */

void
gui_color_init_vars ()
{
    gui_color_term_has_colors = (has_colors ()) ? 1 : 0;
    gui_color_term_colors = 0;
    gui_color_term_color_pairs = 0;
    gui_color_term_can_change_color = 0;
    gui_color_num_pairs = 63;
    if (gui_color_pairs)
    {
        free (gui_color_pairs);
        gui_color_pairs = NULL;
    }
    gui_color_pairs_used = 0;

    if (gui_color_term_has_colors)
    {
        gui_color_term_colors = COLORS;
        gui_color_term_color_pairs = COLOR_PAIRS;
        gui_color_term_can_change_color = (can_change_color ()) ? 1 : 0;

        /* TODO: ncurses may support 65536, but short type used for pairs supports only 32768? */
        gui_color_num_pairs = (gui_color_term_color_pairs >= 32768) ?
            32767 : gui_color_term_color_pairs - 1;
        gui_color_pairs = calloc (
            (gui_color_term_colors + 2) * (gui_color_term_colors + 2),
            sizeof (gui_color_pairs[0]));
        gui_color_pairs_used = 0;

        /* reserved for future usage */
        /*
        gui_color_term_color_content = malloc (sizeof (gui_color_term_color_content[0]) *
                                               (gui_color_term_colors + 1) * 3);
        if (gui_color_term_color_content)
        {
            for (i = 0; i <= gui_color_term_colors; i++)
            {
                color_content ((short) i,
                               &gui_color_term_color_content[i * 3],
                               &gui_color_term_color_content[(i * 3) + 1],
                               &gui_color_term_color_content[(i * 3) + 2]);
            }
        }
        */
    }
    else
    {
        gui_color_term_colors = 1;
        gui_color_term_color_pairs = 1;
        gui_color_term_can_change_color = 0;
        gui_color_num_pairs = 1;
        gui_color_pairs = calloc (1, sizeof (gui_color_pairs[0]));
        gui_color_pairs_used = 0;
    }
}

/*
 * Frees color variables.
 */

void
gui_color_free_vars ()
{
    if (gui_color_pairs)
    {
        free (gui_color_pairs);
        gui_color_pairs = NULL;
    }
    if (gui_color_term_color_content)
    {
        free (gui_color_term_color_content);
        gui_color_term_color_content = NULL;
    }
}

/*
 * Initializes color pairs with terminal colors.
 */

void
gui_color_init_pairs_terminal ()
{
    int i;

    if (gui_color_term_has_colors)
    {
        for (i = 1; i <= gui_color_num_pairs; i++)
        {
            init_pair (i, i, -1);
        }
    }
}

/*
 * Initializes color pairs with WeeChat colors.
 *
 * Pairs defined by WeeChat are set with their values (from pair 1 to pair N),
 * and other pairs are set with terminal color and default background (-1).
 */

void
gui_color_init_pairs_weechat ()
{
    int i;
    short *foregrounds, *backgrounds;

    if (gui_color_term_has_colors)
    {
        gui_color_get_pairs_arrays (&foregrounds, &backgrounds);
        if (foregrounds && backgrounds)
        {
            for (i = 1; i <= gui_color_num_pairs; i++)
            {
                if ((foregrounds[i] >= -1) && (backgrounds[i] >= -1))
                    init_pair (i, foregrounds[i], backgrounds[i]);
                else
                    init_pair (i, i, -1);
            }
        }
        if (foregrounds)
            free (foregrounds);
        if (backgrounds)
            free (backgrounds);
    }
}

/*
 * Displays terminal colors.
 *
 * This is called by command line option "-c" / "--colors".
 */

void
gui_color_display_terminal_colors ()
{
    int lines, columns, line, col, color;
    char str_line[1024], str_color[64];

    initscr ();
    if (has_colors ())
    {
        start_color ();
        use_default_colors ();
        gui_color_init_vars ();
        refresh ();
        endwin ();
    }
    gui_color_info_term_colors (str_line, sizeof (str_line));
    printf ("\n");
    printf ("%s %s\n", _("Terminal infos:"), str_line);
    if (gui_color_term_colors == 0)
    {
        printf ("%s\n", _("No color support in terminal."));
    }
    else
    {
        printf ("\n");
        printf ("%s\n", _("Default colors:"));
        printf ("------------------------------------------------------------"
                "--------------------\n");
        columns = 16;
        lines = (gui_color_term_colors - 1) / columns + 1;
        for (line = 0; line < lines; line++)
        {
            str_line[0] = '\0';
            for (col = 0; col < columns; col++)
            {
                color = line * columns + col;
                if (color < gui_color_term_colors)
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
    gui_color_free_vars ();
}

/*
 * Displays line with terminal colors and timer (remaining time for display of
 * terminal colors).
 */

void
gui_color_buffer_display_timer ()
{
    if (gui_color_buffer && gui_color_use_term_colors)
    {
        gui_chat_printf_y (gui_color_buffer, 2,
                           "%s  (%d)",
                           _("Terminal colors:"),
                           gui_color_timer);
    }
}

/*
 * Put info about terminal and colors in buffer: $TERM, COLORS, COLOR_PAIRS,
 * can_change_color.
 */

void
gui_color_info_term_colors (char *buffer, int size)
{
    snprintf (buffer, size,
              "$TERM=%s  COLORS: %d, COLOR_PAIRS: %d, "
              "can_change_color: %s",
              getenv ("TERM"),
              gui_color_term_colors,
              gui_color_term_color_pairs,
              (gui_color_term_can_change_color) ? "yes" : "no");
}

/*
 * Displays content of color buffer.
 */

void
gui_color_buffer_display ()
{
    int y, i, lines, columns, line, col, color, max_color, num_items;
    char str_line[1024], str_color[64], str_rgb[64], **items;
    struct t_gui_color_palette *color_palette;

    if (!gui_color_buffer)
        return;

    gui_buffer_clear (gui_color_buffer);

    /* set title buffer */
    gui_buffer_set_title (gui_color_buffer,
                          _("WeeChat colors | Actions: "
                            "[e] Display extra infos [r] Refresh "
                            "[z] Reset colors [q] Close buffer | "
                            "Keys: [alt-c] Temporarily switch to terminal "
                            "colors"));

    /* display terminal/colors infos */
    y = 0;
    gui_color_info_term_colors (str_line, sizeof (str_line));
    gui_chat_printf_y (gui_color_buffer, y++, "%s", str_line);

    /* display palette of colors */
    y++;
    if (gui_color_use_term_colors)
    {
        gui_color_buffer_display_timer ();
        y++;
    }
    else
    {
        gui_chat_printf_y (gui_color_buffer, y++,
                           _("WeeChat color pairs auto-allocated "
                             "(in use: %d, left: %d):"),
                           gui_color_pairs_used,
                           gui_color_num_pairs - gui_color_pairs_used);
    }
    columns = 16;
    max_color = (gui_color_use_term_colors) ?
        gui_color_term_colors - 1 : gui_color_pairs_used;
    /* round up to nearest multiple of columns */
    max_color = (max_color / columns) * columns + columns - 1;
    lines = max_color / columns + 1;
    for (line = 0; line < lines; line++)
    {
        str_line[0] = '\0';
        for (col = 0; col < columns; col++)
        {
            color = line * columns + col;
            if (color <= max_color)
            {
                if (color == 0)
                {
                    snprintf (str_color, sizeof (str_color),
                              "     ");
                }
                else if (gui_color_use_term_colors
                         || (color <= gui_color_pairs_used))
                {
                    snprintf (str_color, sizeof (str_color),
                              (color <= 999) ? "%c%c%05d %03d " : "%c%c%05d%5d",
                              GUI_COLOR_COLOR_CHAR,
                              GUI_COLOR_EXTENDED_CHAR,
                              color,
                              color);
                }
                else
                {
                    snprintf (str_color, sizeof (str_color),
                              "%s  -  ",
                              GUI_NO_COLOR);
                }
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
    gui_chat_printf_y (gui_color_buffer, y++,
                       (gui_color_use_term_colors) ?
                       "" : _("(press alt-c to see the colors you can use "
                              "in options)"));

    if (gui_color_buffer_extra_info)
    {
        /* display time of last auto reset of color pairs */
        y++;
        gui_chat_printf_y (gui_color_buffer, y++,
                           _("Last auto reset of pairs: %s"),
                           (gui_color_pairs_auto_reset_last == 0) ?
                           "-" : ctime (&gui_color_pairs_auto_reset_last));

        /* display WeeChat basic colors */
        y++;
        gui_chat_printf_y (gui_color_buffer, y++,
                           _("WeeChat basic colors:"));
        str_line[0] = '\0';
        for (i = 0; i < GUI_CURSES_NUM_WEECHAT_COLORS; i++)
        {
            if (gui_color_use_term_colors)
            {
                snprintf (str_color, sizeof (str_color),
                          " %s",
                          gui_weechat_colors[i].string);
            }
            else
            {
                snprintf (str_color, sizeof (str_color),
                          "%c %c%c%02d%s",
                          GUI_COLOR_RESET_CHAR,
                          GUI_COLOR_COLOR_CHAR,
                          GUI_COLOR_FG_CHAR,
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
                              ",",
                              NULL,
                              WEECHAT_STRING_SPLIT_STRIP_LEFT
                              | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                              | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                              0,
                              &num_items);
        if (items)
        {
            str_line[0] = '\0';
            for (i = 0; i < num_items; i++)
            {
                if (gui_color_use_term_colors)
                {
                    snprintf (str_color, sizeof (str_color),
                              " %s",
                              items[i]);
                }
                else
                {
                    snprintf (str_color, sizeof (str_color),
                              "%c %s%s",
                              GUI_COLOR_RESET_CHAR,
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
        if (gui_color_hash_palette_color->items_count > 0)
        {
            y++;
            gui_chat_printf_y (gui_color_buffer, y++,
                               _("Color aliases:"));
            for (i = 1; i <= gui_color_num_pairs; i++)
            {
                color_palette = gui_color_palette_get (i);
                if (color_palette)
                {
                    str_color[0] = '\0';
                    if (!gui_color_use_term_colors)
                    {
                        snprintf (str_color, sizeof (str_color),
                                  "%c%c%c%05d",
                                  GUI_COLOR_COLOR_CHAR,
                                  GUI_COLOR_FG_CHAR,
                                  GUI_COLOR_EXTENDED_CHAR,
                                  i);
                    }
                    str_rgb[0] = '\0';
                    if ((color_palette->r >= 0) && (color_palette->g >= 0)
                        && (color_palette->b >= 0))
                    {
                        snprintf (str_rgb, sizeof (str_rgb),
                                  " (%d/%d/%d)",
                                  color_palette->r,
                                  color_palette->g,
                                  color_palette->b);
                    }
                    gui_chat_printf_y (gui_color_buffer, y++,
                                       " %5d: %s%s%s",
                                       i,
                                       str_color,
                                       (color_palette->alias) ? color_palette->alias : "",
                                       str_rgb);
                }
            }
        }

        /* display content of colors */
        if (gui_color_term_color_content)
        {
            y++;
            gui_chat_printf_y (gui_color_buffer, y++,
                               _("Content of colors (r/g/b):"));
            for (i = 0; i < gui_color_term_colors; i++)
            {
                gui_chat_printf_y (gui_color_buffer, y++,
                                   " %3d: %4hd / %4hd / %4hd",
                                   i,
                                   gui_color_term_color_content[i * 3],
                                   gui_color_term_color_content[(i* 3) + 1],
                                   gui_color_term_color_content[(i* 3) + 2]);
            }
        }
    }
}

/*
 * Callback for timer.
 */

int
gui_color_timer_cb (const void *pointer, void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) remaining_calls;

    gui_color_timer--;

    if (gui_color_timer <= 0)
    {
        if (gui_color_use_term_colors)
            gui_color_switch_colors ();
    }
    else
    {
        if (gui_color_buffer && gui_color_use_term_colors)
        {
            gui_color_buffer_display_timer ();
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Switches between WeeChat and terminal colors.
 */

void
gui_color_switch_colors ()
{
    if (gui_color_hook_timer)
    {
        unhook (gui_color_hook_timer);
        gui_color_hook_timer = NULL;
    }

    /*
     * when we press alt-c many times quickly, this just adds some time for
     * display of terminal colors
     */
    if (gui_color_use_term_colors
        && (gui_color_timer > 0) && (gui_color_timer % 10 == 0))
    {
        if (gui_color_timer < 120)
            gui_color_timer += 10;
        gui_color_buffer_display_timer ();
    }
    else
    {
        gui_color_use_term_colors ^= 1;

        if (gui_color_use_term_colors)
            gui_color_init_pairs_terminal ();
        else
            gui_color_init_pairs_weechat ();

        gui_color_buffer_refresh_needed = 1;
        gui_window_ask_refresh (1);

        if (gui_color_use_term_colors)
            gui_color_timer = GUI_COLOR_TIMER_TERM_COLORS;
    }

    if (gui_color_use_term_colors)
    {
        gui_color_hook_timer = hook_timer (NULL, 1000, 0, 0,
                                           &gui_color_timer_cb, NULL, NULL);
    }
}

/*
 * Resets all color pairs (the next refresh will auto reallocate needed pairs).
 *
 * It is useful when color pairs table is full, to remove non used pairs.
 */

void
gui_color_reset_pairs ()
{
    if (gui_color_pairs)
    {
        memset (gui_color_pairs, 0,
                (gui_color_term_colors + 2)
                * (gui_color_term_colors + 2)
                * sizeof (gui_color_pairs[0]));
        gui_color_pairs_used = 0;
        gui_color_warning_pairs_full = 0;
        gui_color_buffer_refresh_needed = 1;
        gui_window_ask_refresh (1);
    }
}

/*
 * Input callback for color buffer.
 */

int
gui_color_buffer_input_cb (const void *pointer, void *data,
                           struct t_gui_buffer *buffer,
                           const char *input_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (string_strcmp (input_data, "e") == 0)
    {
        gui_color_buffer_extra_info ^= 1;
        gui_color_buffer_display ();
    }
    else if (string_strcmp (input_data, "r") == 0)
    {
        gui_color_buffer_display ();
    }
    else if (string_strcmp (input_data, "q") == 0)
    {
        gui_buffer_close (buffer);
    }
    else if (string_strcmp (input_data, "z") == 0)
    {
        gui_color_reset_pairs ();
    }

    return WEECHAT_RC_OK;
}

/*
 * Close callback for color buffer.
 */

int
gui_color_buffer_close_cb (const void *pointer, void *data,
                           struct t_gui_buffer *buffer)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    gui_color_buffer = NULL;

    return WEECHAT_RC_OK;
}

/*
 * Assigns color buffer to pointer if it is not yet set.
 */

void
gui_color_buffer_assign ()
{
    if (!gui_color_buffer)
    {
        gui_color_buffer = gui_buffer_search_by_name (NULL,
                                                      GUI_COLOR_BUFFER_NAME);
        if (gui_color_buffer)
        {
            gui_color_buffer->input_callback = &gui_color_buffer_input_cb;
            gui_color_buffer->close_callback = &gui_color_buffer_close_cb;
        }
    }
}

/*
 * Opens a buffer to display colors.
 */

void
gui_color_buffer_open ()
{
    struct t_hashtable *properties;

    if (!gui_color_buffer)
    {
        properties = hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL, NULL);
        if (properties)
        {
            hashtable_set (properties, "type", "free");
            hashtable_set (properties, "localvar_set_no_log", "1");
            hashtable_set (properties, "key_bind_meta-c", "/color switch");
        }

        gui_color_buffer = gui_buffer_new_props (
            NULL, GUI_COLOR_BUFFER_NAME, properties,
            &gui_color_buffer_input_cb, NULL, NULL,
            &gui_color_buffer_close_cb, NULL, NULL);

        if (gui_color_buffer && !gui_color_buffer->short_name)
            gui_color_buffer->short_name = strdup (GUI_COLOR_BUFFER_NAME);

        if (properties)
            hashtable_free (properties);
    }

    if (!gui_color_buffer)
        return;

    gui_window_switch_to_buffer (gui_current_window, gui_color_buffer, 1);

    gui_color_buffer_display ();
}

/*
 * Adds an alias in hashtable with aliases.
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
    }
}

/*
 * Builds aliases for palette.
 */

void
gui_color_palette_build_aliases ()
{
    int i;
    struct t_gui_color_palette *color_palette;
    char str_number[64];

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
    for (i = 0; i < 256; i++)
    {
        color_palette = gui_color_palette_get (i);
        if (color_palette)
        {
            weelist_add (gui_color_list_with_alias,
                         color_palette->alias,
                         WEECHAT_LIST_POS_END,
                         NULL);
        }
        else
        {
            snprintf (str_number, sizeof (str_number),
                      "%d", i);
            weelist_add (gui_color_list_with_alias,
                         str_number,
                         WEECHAT_LIST_POS_END,
                         NULL);
        }
    }
    hashtable_map (gui_color_hash_palette_color,
                   &gui_color_palette_add_alias_cb, NULL);
}

/*
 * Creates a new color in palette.
 */

struct t_gui_color_palette *
gui_color_palette_new (int number, const char *value)
{
    struct t_gui_color_palette *new_color_palette;
    char **items, *pos, *pos2, *error1, *error2, *error3;
    char *str_alias, *str_rgb, str_number[64];
    int num_items, i, r, g, b;

    if (!value)
        return NULL;

    new_color_palette = malloc (sizeof (*new_color_palette));
    if (new_color_palette)
    {
        new_color_palette->alias = NULL;
        new_color_palette->r = -1;
        new_color_palette->g = -1;
        new_color_palette->b = -1;

        str_alias = NULL;
        str_rgb = NULL;

        items = string_split (value, ";", NULL,
                              WEECHAT_STRING_SPLIT_STRIP_LEFT
                              | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                              | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                              0, &num_items);
        if (items)
        {
            for (i = 0; i < num_items; i++)
            {
                pos = strchr (items[i], '/');
                if (pos)
                    str_rgb = items[i];
                else
                {
                    pos = strchr (items[i], ',');
                    if (!pos)
                        str_alias = items[i];
                }
            }

            if (str_alias)
            {
                new_color_palette->alias = strdup (str_alias);
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
 * Frees a color in palette.
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
 * Initializes WeeChat colors.
 */

void
gui_color_init_weechat ()
{
    if (CONFIG_BOOLEAN(config_look_color_basic_force_bold)
        || (gui_color_term_colors < 16))
    {
        gui_weechat_colors = gui_weechat_colors_bold;
    }
    else
    {
        gui_weechat_colors = gui_weechat_colors_no_bold;
    }

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
    gui_color_build (GUI_COLOR_CHAT_TAGS, CONFIG_COLOR(config_color_chat_tags), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_INACTIVE_WINDOW, CONFIG_COLOR(config_color_chat_inactive_window), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_INACTIVE_BUFFER, CONFIG_COLOR(config_color_chat_inactive_buffer), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_PREFIX_BUFFER_INACTIVE_BUFFER, CONFIG_COLOR(config_color_chat_prefix_buffer_inactive_buffer), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_NICK_OFFLINE, CONFIG_COLOR(config_color_chat_nick_offline), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_NICK_OFFLINE_HIGHLIGHT, CONFIG_COLOR(config_color_chat_nick_offline_highlight), CONFIG_COLOR(config_color_chat_nick_offline_highlight_bg));
    gui_color_build (GUI_COLOR_CHAT_NICK_PREFIX, CONFIG_COLOR(config_color_chat_nick_prefix), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_NICK_SUFFIX, CONFIG_COLOR(config_color_chat_nick_suffix), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_EMPHASIS, CONFIG_COLOR(config_color_emphasized), CONFIG_COLOR(config_color_emphasized_bg));
    gui_color_build (GUI_COLOR_CHAT_DAY_CHANGE, CONFIG_COLOR(config_color_chat_day_change), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_VALUE_NULL, CONFIG_COLOR(config_color_chat_value_null), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_STATUS_DISABLED, CONFIG_COLOR(config_color_chat_status_disabled), CONFIG_COLOR(config_color_chat_bg));
    gui_color_build (GUI_COLOR_CHAT_STATUS_ENABLED, CONFIG_COLOR(config_color_chat_status_enabled), CONFIG_COLOR(config_color_chat_bg));

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
 * Allocates GUI colors.
 */

void
gui_color_alloc ()
{
    if (has_colors ())
    {
        start_color ();
        use_default_colors ();
    }
    gui_color_init_vars ();
    gui_color_init_pairs_terminal ();
    gui_color_init_weechat ();
    gui_color_palette_build_aliases ();
}

/*
 * Dumps colors.
 */

void
gui_color_dump ()
{
    char str_line[1024];
    int fg, bg, index, used;

    gui_color_info_term_colors (str_line, sizeof (str_line));

    gui_chat_printf (NULL, "");
    gui_chat_printf (NULL, "%s", str_line);
    gui_chat_printf (NULL,
                     _("WeeChat colors (in use: %d, left: %d):"),
                     gui_color_pairs_used,
                     gui_color_num_pairs - gui_color_pairs_used);
    if (gui_color_pairs)
    {
        used = 0;
        for (bg = -1; bg <= gui_color_term_colors; bg++)
        {
            for (fg = -1; fg <= gui_color_term_colors; fg++)
            {
                index = ((bg + 1) * (gui_color_term_colors + 2)) + (fg + 1);
                if (gui_color_pairs[index] >= 1)
                {
                    gui_chat_printf (NULL,
                                     "  fg:%3d, bg:%3d, pairs[%05d] = %hd",
                                     fg, bg, index, gui_color_pairs[index]);
                    used++;
                }
            }
        }
    }
}
