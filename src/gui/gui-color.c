/*
 * gui-color.c - color functions (used by all GUI)
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
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hashtable.h"
#include "../core/wee-list.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../plugins/plugin.h"
#include "gui-color.h"
#include "gui-chat.h"
#include "gui-window.h"


struct t_gui_color *gui_color[GUI_COLOR_NUM_COLORS]; /* GUI colors          */

/* palette colors and aliases */
struct t_hashtable *gui_color_hash_palette_color = NULL;
struct t_hashtable *gui_color_hash_palette_alias = NULL;
struct t_weelist *gui_color_list_with_alias = NULL;

/* terminal colors */
int gui_color_term256[256] =
{
    0x000000, 0x800000, 0x008000, 0x808000, 0x000080, 0x800080,  /*   0-5   */
    0x008080, 0xc0c0c0, 0x808080, 0xff0000, 0x00ff00, 0xffff00,  /*   6-11  */
    0x0000ff, 0xff00ff, 0x00ffff, 0xffffff, 0x000000, 0x00005f,  /*  12-17  */
    0x000087, 0x0000af, 0x0000d7, 0x0000ff, 0x005f00, 0x005f5f,  /*  18-23  */
    0x005f87, 0x005faf, 0x005fd7, 0x005fff, 0x008700, 0x00875f,  /*  24-29  */
    0x008787, 0x0087af, 0x0087d7, 0x0087ff, 0x00af00, 0x00af5f,  /*  30-35  */
    0x00af87, 0x00afaf, 0x00afd7, 0x00afff, 0x00d700, 0x00d75f,  /*  36-41  */
    0x00d787, 0x00d7af, 0x00d7d7, 0x00d7ff, 0x00ff00, 0x00ff5f,  /*  42-47  */
    0x00ff87, 0x00ffaf, 0x00ffd7, 0x00ffff, 0x5f0000, 0x5f005f,  /*  48-53  */
    0x5f0087, 0x5f00af, 0x5f00d7, 0x5f00ff, 0x5f5f00, 0x5f5f5f,  /*  54-59  */
    0x5f5f87, 0x5f5faf, 0x5f5fd7, 0x5f5fff, 0x5f8700, 0x5f875f,  /*  60-65  */
    0x5f8787, 0x5f87af, 0x5f87d7, 0x5f87ff, 0x5faf00, 0x5faf5f,  /*  66-71  */
    0x5faf87, 0x5fafaf, 0x5fafd7, 0x5fafff, 0x5fd700, 0x5fd75f,  /*  72-77  */
    0x5fd787, 0x5fd7af, 0x5fd7d7, 0x5fd7ff, 0x5fff00, 0x5fff5f,  /*  78-83  */
    0x5fff87, 0x5fffaf, 0x5fffd7, 0x5fffff, 0x870000, 0x87005f,  /*  84-89  */
    0x870087, 0x8700af, 0x8700d7, 0x8700ff, 0x875f00, 0x875f5f,  /*  90-95  */
    0x875f87, 0x875faf, 0x875fd7, 0x875fff, 0x878700, 0x87875f,  /*  96-101 */
    0x878787, 0x8787af, 0x8787d7, 0x8787ff, 0x87af00, 0x87af5f,  /* 102-107 */
    0x87af87, 0x87afaf, 0x87afd7, 0x87afff, 0x87d700, 0x87d75f,  /* 108-113 */
    0x87d787, 0x87d7af, 0x87d7d7, 0x87d7ff, 0x87ff00, 0x87ff5f,  /* 114-119 */
    0x87ff87, 0x87ffaf, 0x87ffd7, 0x87ffff, 0xaf0000, 0xaf005f,  /* 120-125 */
    0xaf0087, 0xaf00af, 0xaf00d7, 0xaf00ff, 0xaf5f00, 0xaf5f5f,  /* 126-131 */
    0xaf5f87, 0xaf5faf, 0xaf5fd7, 0xaf5fff, 0xaf8700, 0xaf875f,  /* 132-137 */
    0xaf8787, 0xaf87af, 0xaf87d7, 0xaf87ff, 0xafaf00, 0xafaf5f,  /* 138-143 */
    0xafaf87, 0xafafaf, 0xafafd7, 0xafafff, 0xafd700, 0xafd75f,  /* 144-149 */
    0xafd787, 0xafd7af, 0xafd7d7, 0xafd7ff, 0xafff00, 0xafff5f,  /* 150-155 */
    0xafff87, 0xafffaf, 0xafffd7, 0xafffff, 0xd70000, 0xd7005f,  /* 156-161 */
    0xd70087, 0xd700af, 0xd700d7, 0xd700ff, 0xd75f00, 0xd75f5f,  /* 162-167 */
    0xd75f87, 0xd75faf, 0xd75fd7, 0xd75fff, 0xd78700, 0xd7875f,  /* 168-173 */
    0xd78787, 0xd787af, 0xd787d7, 0xd787ff, 0xd7af00, 0xd7af5f,  /* 174-179 */
    0xd7af87, 0xd7afaf, 0xd7afd7, 0xd7afff, 0xd7d700, 0xd7d75f,  /* 180-185 */
    0xd7d787, 0xd7d7af, 0xd7d7d7, 0xd7d7ff, 0xd7ff00, 0xd7ff5f,  /* 186-191 */
    0xd7ff87, 0xd7ffaf, 0xd7ffd7, 0xd7ffff, 0xff0000, 0xff005f,  /* 192-197 */
    0xff0087, 0xff00af, 0xff00d7, 0xff00ff, 0xff5f00, 0xff5f5f,  /* 198-203 */
    0xff5f87, 0xff5faf, 0xff5fd7, 0xff5fff, 0xff8700, 0xff875f,  /* 204-209 */
    0xff8787, 0xff87af, 0xff87d7, 0xff87ff, 0xffaf00, 0xffaf5f,  /* 210-215 */
    0xffaf87, 0xffafaf, 0xffafd7, 0xffafff, 0xffd700, 0xffd75f,  /* 216-221 */
    0xffd787, 0xffd7af, 0xffd7d7, 0xffd7ff, 0xffff00, 0xffff5f,  /* 222-227 */
    0xffff87, 0xffffaf, 0xffffd7, 0xffffff, 0x080808, 0x121212,  /* 228-233 */
    0x1c1c1c, 0x262626, 0x303030, 0x3a3a3a, 0x444444, 0x4e4e4e,  /* 234-239 */
    0x585858, 0x626262, 0x6c6c6c, 0x767676, 0x808080, 0x8a8a8a,  /* 240-245 */
    0x949494, 0x9e9e9e, 0xa8a8a8, 0xb2b2b2, 0xbcbcbc, 0xc6c6c6,  /* 246-251 */
    0xd0d0d0, 0xdadada, 0xe4e4e4, 0xeeeeee,                      /* 252-255 */
};

/* ANSI colors */
regex_t *gui_color_regex_ansi = NULL;
char *gui_color_ansi[16] =
{
    /* 0-7 */
    "black", "red", "green", "brown", "blue", "magenta", "cyan", "gray",
    /* 8-15 */
    "darkgray", "lightred", "lightgreen", "yellow", "lightblue", "lightmagenta",
    "lightcyan", "white",
};


/*
 * Returns a color code from an option, which can be a color or a string.
 *
 * Returns NULL if the option has a wrong type.
 */

const char *
gui_color_from_option (struct t_config_option *option)
{
    if (!option)
        return NULL;

    switch (option->type)
    {
        case CONFIG_OPTION_TYPE_COLOR:
            if (option->min < 0)
            {
                return gui_color_get_custom (
                    gui_color_get_name (CONFIG_COLOR(option)));
            }
            return GUI_COLOR(option->min);
        case CONFIG_OPTION_TYPE_STRING:
            return gui_color_get_custom (CONFIG_STRING(option));
        default:
            return NULL;
    }

    /* never executed */
    return NULL;
}

/*
 * Searches for a color with configuration option name.
 *
 * Returns color string, NULL if not found.
 */

const char *
gui_color_search_config (const char *color_name)
{
    struct t_config_option *ptr_option;

    if (!color_name)
        return NULL;

    /* search in weechat.conf colors (example: "chat_delimiters") */
    for (ptr_option = weechat_config_section_color->options;
         ptr_option; ptr_option = ptr_option->next_option)
    {
        if (strcmp (ptr_option->name, color_name) == 0)
            return gui_color_from_option (ptr_option);
    }

    /* search in any configuration file (example: "irc.color.message_quit") */
    if (strchr (color_name, '.'))
    {
        config_file_search_with_string (color_name, NULL, NULL, &ptr_option,
                                        NULL);
        if (ptr_option)
            return gui_color_from_option (ptr_option);
    }

    /* color not found */
    return NULL;
}

/*
 * Returns flag for attribute char of a color.
 *
 * Returns 0 if char is unknown.
 */

int
gui_color_attr_get_flag (char c)
{
    if (c == GUI_COLOR_EXTENDED_BLINK_CHAR)
        return GUI_COLOR_EXTENDED_BLINK_FLAG;

    if (c == GUI_COLOR_EXTENDED_DIM_CHAR)
        return GUI_COLOR_EXTENDED_DIM_FLAG;

    if (c == GUI_COLOR_EXTENDED_BOLD_CHAR)
        return GUI_COLOR_EXTENDED_BOLD_FLAG;

    if (c == GUI_COLOR_EXTENDED_REVERSE_CHAR)
        return GUI_COLOR_EXTENDED_REVERSE_FLAG;

    if (c == GUI_COLOR_EXTENDED_ITALIC_CHAR)
        return GUI_COLOR_EXTENDED_ITALIC_FLAG;

    if (c == GUI_COLOR_EXTENDED_UNDERLINE_CHAR)
        return GUI_COLOR_EXTENDED_UNDERLINE_FLAG;

    if (c == GUI_COLOR_EXTENDED_KEEPATTR_CHAR)
        return GUI_COLOR_EXTENDED_KEEPATTR_FLAG;

    return 0;
}

/*
 * Builds string with attributes of color.
 *
 * The str_attr must be at least 6 bytes long (5 for attributes + final '\0').
 */

void
gui_color_attr_build_string (int color, char *str_attr)
{
    int i;

    i = 0;

    if (color & GUI_COLOR_EXTENDED_BLINK_FLAG)
        str_attr[i++] = GUI_COLOR_EXTENDED_BLINK_CHAR;
    if (color & GUI_COLOR_EXTENDED_DIM_FLAG)
        str_attr[i++] = GUI_COLOR_EXTENDED_DIM_CHAR;
    if (color & GUI_COLOR_EXTENDED_BOLD_FLAG)
        str_attr[i++] = GUI_COLOR_EXTENDED_BOLD_CHAR;
    if (color & GUI_COLOR_EXTENDED_REVERSE_FLAG)
        str_attr[i++] = GUI_COLOR_EXTENDED_REVERSE_CHAR;
    if (color & GUI_COLOR_EXTENDED_ITALIC_FLAG)
        str_attr[i++] = GUI_COLOR_EXTENDED_ITALIC_CHAR;
    if (color & GUI_COLOR_EXTENDED_UNDERLINE_FLAG)
        str_attr[i++] = GUI_COLOR_EXTENDED_UNDERLINE_CHAR;
    if (color & GUI_COLOR_EXTENDED_KEEPATTR_FLAG)
        str_attr[i++] = GUI_COLOR_EXTENDED_KEEPATTR_CHAR;

    str_attr[i] = '\0';
}

/*
 * Gets a custom color with a name.
 */

const char *
gui_color_get_custom (const char *color_name)
{
    int fg, bg, fg_term, bg_term, term_color;
    static char color[32][96];
    static int index_color = 0;
    char color_fg[32], color_bg[32];
    char *pos_delim, *str_fg, *pos_bg, *error, *color_attr;
    const char *ptr_color_name;

    /* attribute or other color name (GUI dependent) */
    index_color = (index_color + 1) % 32;
    color[index_color][0] = '\0';

    if (!color_name || !color_name[0])
        return color[index_color];

    /* read extra attributes (bold, ..) */
    color_attr = NULL;
    ptr_color_name = color_name;
    while (gui_color_attr_get_flag (ptr_color_name[0]) > 0)
    {
        ptr_color_name++;
    }
    if (ptr_color_name != color_name)
    {
        color_attr = string_strndup (color_name,
                                     ptr_color_name - color_name);
    }

    if (strcmp (ptr_color_name, "reset") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c",
                  GUI_COLOR_RESET_CHAR);
    }
    else if (strcmp (ptr_color_name, "resetcolor") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_COLOR_CHAR,
                  GUI_COLOR_RESET_CHAR);
    }
    else if (strcmp (ptr_color_name, "emphasis") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_COLOR_CHAR,
                  GUI_COLOR_EMPHASIS_CHAR);
    }
    else if (strcmp (ptr_color_name, "blink") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_SET_ATTR_CHAR,
                  GUI_COLOR_ATTR_BLINK_CHAR);
    }
    else if (strcmp (ptr_color_name, "-blink") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_REMOVE_ATTR_CHAR,
                  GUI_COLOR_ATTR_BLINK_CHAR);
    }
    else if (strcmp (ptr_color_name, "dim") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_SET_ATTR_CHAR,
                  GUI_COLOR_ATTR_DIM_CHAR);
    }
    else if (strcmp (ptr_color_name, "-dim") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_REMOVE_ATTR_CHAR,
                  GUI_COLOR_ATTR_DIM_CHAR);
    }
    else if (strcmp (ptr_color_name, "bold") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_SET_ATTR_CHAR,
                  GUI_COLOR_ATTR_BOLD_CHAR);
    }
    else if (strcmp (ptr_color_name, "-bold") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_REMOVE_ATTR_CHAR,
                  GUI_COLOR_ATTR_BOLD_CHAR);
    }
    else if (strcmp (ptr_color_name, "reverse") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_SET_ATTR_CHAR,
                  GUI_COLOR_ATTR_REVERSE_CHAR);
    }
    else if (strcmp (ptr_color_name, "-reverse") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_REMOVE_ATTR_CHAR,
                  GUI_COLOR_ATTR_REVERSE_CHAR);
    }
    else if (strcmp (ptr_color_name, "italic") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_SET_ATTR_CHAR,
                  GUI_COLOR_ATTR_ITALIC_CHAR);
    }
    else if (strcmp (ptr_color_name, "-italic") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_REMOVE_ATTR_CHAR,
                  GUI_COLOR_ATTR_ITALIC_CHAR);
    }
    else if (strcmp (ptr_color_name, "underline") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_SET_ATTR_CHAR,
                  GUI_COLOR_ATTR_UNDERLINE_CHAR);
    }
    else if (strcmp (ptr_color_name, "-underline") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_REMOVE_ATTR_CHAR,
                  GUI_COLOR_ATTR_UNDERLINE_CHAR);
    }
    else if (strcmp (ptr_color_name, "bar_fg") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c%c",
                  GUI_COLOR_COLOR_CHAR,
                  GUI_COLOR_BAR_CHAR,
                  GUI_COLOR_BAR_FG_CHAR);
    }
    else if (strcmp (ptr_color_name, "bar_delim") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c%c",
                  GUI_COLOR_COLOR_CHAR,
                  GUI_COLOR_BAR_CHAR,
                  GUI_COLOR_BAR_DELIM_CHAR);
    }
    else if (strcmp (ptr_color_name, "bar_bg") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c%c",
                  GUI_COLOR_COLOR_CHAR,
                  GUI_COLOR_BAR_CHAR,
                  GUI_COLOR_BAR_BG_CHAR);
    }
    else
    {
        /* custom color name (GUI dependent) */
        fg_term = -1;
        bg_term = -1;
        fg = -1;
        bg = -1;
        color_fg[0] = '\0';
        color_bg[0] = '\0';

        pos_delim = strchr (ptr_color_name, ',');
        if (!pos_delim)
            pos_delim = strchr (ptr_color_name, ':');
        if (pos_delim)
        {
            if (pos_delim == ptr_color_name)
                str_fg = NULL;
            else
                str_fg = string_strndup (ptr_color_name,
                                         pos_delim - ptr_color_name);
            pos_bg = (pos_delim[1]) ? pos_delim + 1 : NULL;
        }
        else
        {
            str_fg = strdup (ptr_color_name);
            pos_bg = NULL;
        }

        if (str_fg)
        {
            fg_term = gui_color_palette_get_alias (str_fg);
            if (fg_term < 0)
            {
                error = NULL;
                term_color = (int)strtol (str_fg, &error, 10);
                if (error && !error[0])
                {
                    fg_term = term_color;
                    if (fg_term < 0)
                        fg_term = 0;
                    else if (fg_term > GUI_COLOR_EXTENDED_MAX)
                        fg_term = GUI_COLOR_EXTENDED_MAX;
                }
                else
                    fg = gui_color_search (str_fg);
            }
        }
        if (pos_bg)
        {
            bg_term = gui_color_palette_get_alias (pos_bg);
            if (bg_term < 0)
            {
                error = NULL;
                term_color = (int)strtol (pos_bg, &error, 10);
                if (error && !error[0])
                {
                    bg_term = term_color;
                    if (bg_term < 0)
                        bg_term = 0;
                    else if (bg_term > GUI_COLOR_EXTENDED_MAX)
                        bg_term = GUI_COLOR_EXTENDED_MAX;
                }
                else
                    bg = gui_color_search (pos_bg);
            }
        }

        if (fg_term >= 0)
        {
            snprintf (color_fg, sizeof (color_fg), "%c%s%05d",
                      GUI_COLOR_EXTENDED_CHAR,
                      (color_attr) ? color_attr : "",
                      fg_term);
        }
        else if (fg >= 0)
        {
            snprintf (color_fg, sizeof (color_fg), "%s%02d",
                      (color_attr) ? color_attr : "",
                      fg);
        }

        if (bg_term >= 0)
        {
            snprintf (color_bg, sizeof (color_bg), "%c%05d",
                      GUI_COLOR_EXTENDED_CHAR,
                      bg_term);
        }
        else if (bg >= 0)
        {
            snprintf (color_bg, sizeof (color_bg), "%02d",
                      bg);
        }

        if (color_fg[0] && color_bg[0])
        {
            /*
             * note: until WeeChat 2.5, the separator was a comma, and it has
             * been changed to a tilde (to prevent problems with /eval and
             * ${color:FF,BB}
             */
            snprintf (color[index_color], sizeof (color[index_color]),
                      "%c%c%s~%s",
                      GUI_COLOR_COLOR_CHAR,
                      GUI_COLOR_FG_BG_CHAR,
                      color_fg,
                      color_bg);
        }
        else if (color_fg[0])
        {
            snprintf (color[index_color], sizeof (color[index_color]),
                      "%c%c%s",
                      GUI_COLOR_COLOR_CHAR,
                      GUI_COLOR_FG_CHAR,
                      color_fg);
        }
        else if (color_bg[0])
        {
            snprintf (color[index_color], sizeof (color[index_color]),
                      "%c%c%s",
                      GUI_COLOR_COLOR_CHAR,
                      GUI_COLOR_BG_CHAR,
                      color_bg);
        }

        if (str_fg)
            free (str_fg);
    }

    if (color_attr)
        free (color_attr);

    return color[index_color];
}

/*
 * Converts a terminal color to its RGB value.
 *
 * Returns a RGB color as integer.
 */

int
gui_color_convert_term_to_rgb (int color)
{
    if ((color < 0) || (color > 255))
        return 0;

    return gui_color_term256[color];
}

/*
 * Converts a RGB color to the closest terminal color.
 *
 * Argument "limit" is the number of colors to check in the table of terminal
 * colors (starting from 0). So for example 256 will return any of the 256
 * colors, 16 will return a color between 0 and 15.
 *
 * Returns the closest terminal color (0-255).
 */

int
gui_color_convert_rgb_to_term (int rgb, int limit)
{
    int i, r1, g1, b1, r2, g2, b2, diff, best_diff, best_color;

    r1 = rgb >> 16;
    g1 = (rgb >> 8) & 0xFF;
    b1 = rgb & 0xFF;

    best_diff = INT_MAX;
    best_color = 0;

    for (i = 0; i < limit; i++)
    {
        r2 = gui_color_term256[i] >> 16;
        g2 = (gui_color_term256[i] >> 8) & 0xFF;
        b2 = gui_color_term256[i] & 0xFF;

        diff = ((r2 - r1) * (r2 - r1)) +
            ((g2 - g1) * (g2 - g1)) +
            ((b2 - b1) * (b2 - b1));

        /* exact match! */
        if (diff == 0)
            return i;

        if (diff < best_diff)
        {
            best_color = i;
            best_diff = diff;
        }
    }

    /* return the closest color */
    return best_color;
}

/*
 * Returns the size (in bytes) of the WeeChat color code at the beginning
 * of "string".
 *
 * If "string" is NULL, empty or does not start with a color code,
 * it returns 0.
 *
 * If "string" begins with multiple color codes, only the size of the first
 * one is returned.
 */

int
gui_color_code_size (const char *string)
{
    const char *ptr_string;

    if (!string)
        return 0;

    ptr_string = string;

    switch (ptr_string[0])
    {
        case GUI_COLOR_COLOR_CHAR:
            ptr_string++;
            switch (ptr_string[0])
            {
                case GUI_COLOR_FG_CHAR:
                    ptr_string++;
                    if (ptr_string[0] == GUI_COLOR_EXTENDED_CHAR)
                    {
                        ptr_string++;
                        while (gui_color_attr_get_flag (ptr_string[0]) > 0)
                        {
                            ptr_string++;
                        }
                        if (ptr_string[0] && ptr_string[1] && ptr_string[2]
                            && ptr_string[3] && ptr_string[4])
                        {
                            ptr_string += 5;
                        }
                    }
                    else
                    {
                        while (gui_color_attr_get_flag (ptr_string[0]) > 0)
                        {
                            ptr_string++;
                        }
                        if (ptr_string[0] && ptr_string[1])
                            ptr_string += 2;
                    }
                    break;
                case GUI_COLOR_BG_CHAR:
                    ptr_string++;
                    if (ptr_string[0] == GUI_COLOR_EXTENDED_CHAR)
                    {
                        ptr_string++;
                        if (ptr_string[0] && ptr_string[1] && ptr_string[2]
                            && ptr_string[3] && ptr_string[4])
                        {
                            ptr_string += 5;
                        }
                    }
                    else
                    {
                        if (ptr_string[0] && ptr_string[1])
                            ptr_string += 2;
                    }
                    break;
                case GUI_COLOR_FG_BG_CHAR:
                    ptr_string++;
                    if (ptr_string[0] == GUI_COLOR_EXTENDED_CHAR)
                    {
                        ptr_string++;
                        while (gui_color_attr_get_flag (ptr_string[0]) > 0)
                        {
                            ptr_string++;
                        }
                        if (ptr_string[0] && ptr_string[1] && ptr_string[2]
                            && ptr_string[3] && ptr_string[4])
                        {
                            ptr_string += 5;
                        }
                    }
                    else
                    {
                        while (gui_color_attr_get_flag (ptr_string[0]) > 0)
                        {
                            ptr_string++;
                        }
                        if (ptr_string[0] && ptr_string[1])
                            ptr_string += 2;
                    }
                    /*
                     * note: the comma is an old separator not used any
                     * more (since WeeChat 2.6), but we still use it here
                     * so in case of/upgrade this will not break colors in
                     * old messages
                     */
                    if ((ptr_string[0] == ',') || (ptr_string[0] == '~'))
                    {
                        if (ptr_string[1] == GUI_COLOR_EXTENDED_CHAR)
                        {
                            ptr_string += 2;
                            if (ptr_string[0] && ptr_string[1]
                                && ptr_string[2] && ptr_string[3]
                                && ptr_string[4])
                            {
                                ptr_string += 5;
                            }
                        }
                        else
                        {
                            ptr_string++;
                            if (ptr_string[0] && ptr_string[1])
                                ptr_string += 2;
                        }
                    }
                    break;
                case GUI_COLOR_EXTENDED_CHAR:
                    ptr_string++;
                    if ((isdigit ((unsigned char)ptr_string[0]))
                        && (isdigit ((unsigned char)ptr_string[1]))
                        && (isdigit ((unsigned char)ptr_string[2]))
                        && (isdigit ((unsigned char)ptr_string[3]))
                        && (isdigit ((unsigned char)ptr_string[4])))
                    {
                        ptr_string += 5;
                    }
                    break;
                case GUI_COLOR_EMPHASIS_CHAR:
                    ptr_string++;
                    break;
                case GUI_COLOR_BAR_CHAR:
                    ptr_string++;
                    switch (ptr_string[0])
                    {
                        case GUI_COLOR_BAR_FG_CHAR:
                        case GUI_COLOR_BAR_BG_CHAR:
                        case GUI_COLOR_BAR_DELIM_CHAR:
                        case GUI_COLOR_BAR_START_INPUT_CHAR:
                        case GUI_COLOR_BAR_START_INPUT_HIDDEN_CHAR:
                        case GUI_COLOR_BAR_MOVE_CURSOR_CHAR:
                        case GUI_COLOR_BAR_START_ITEM:
                        case GUI_COLOR_BAR_START_LINE_ITEM:
                        case GUI_COLOR_BAR_SPACER:
                            ptr_string++;
                            break;
                    }
                    break;
                case GUI_COLOR_RESET_CHAR:
                    ptr_string++;
                    break;
                default:
                    if (isdigit ((unsigned char)ptr_string[0])
                        && isdigit ((unsigned char)ptr_string[1]))
                    {
                        ptr_string += 2;
                    }
                    break;
            }
            return ptr_string - string;
            break;
        case GUI_COLOR_SET_ATTR_CHAR:
        case GUI_COLOR_REMOVE_ATTR_CHAR:
            ptr_string++;
            if (ptr_string[0])
                ptr_string++;
            return ptr_string - string;
            break;
        case GUI_COLOR_RESET_CHAR:
            ptr_string++;
            return ptr_string - string;
            break;
    }

    return 0;
}

/*
 * Removes WeeChat color codes from a message.
 *
 * If replacement is not NULL and not empty, it is used to replace color codes
 * by first char of replacement (and next chars in string are NOT removed).
 * If replacement is NULL or empty, color codes are removed, with following
 * chars if they are related to color code.
 *
 * Note: result must be freed after use.
 */

char *
gui_color_decode (const char *string, const char *replacement)
{
    const unsigned char *ptr_string;
    unsigned char *out;
    int out_length, out_pos, length;

    if (!string)
        return NULL;

    out_length = (strlen ((char *)string) * 2) + 1;
    out = malloc (out_length);
    if (!out)
        return NULL;

    ptr_string = (unsigned char *)string;
    out_pos = 0;
    while (ptr_string && ptr_string[0] && (out_pos < out_length - 1))
    {
        switch (ptr_string[0])
        {
            case GUI_COLOR_COLOR_CHAR:
                ptr_string++;
                switch (ptr_string[0])
                {
                    case GUI_COLOR_FG_CHAR:
                        ptr_string++;
                        if (ptr_string[0] == GUI_COLOR_EXTENDED_CHAR)
                        {
                            ptr_string++;
                            while (gui_color_attr_get_flag (ptr_string[0]) > 0)
                            {
                                ptr_string++;
                            }
                            if (ptr_string[0] && ptr_string[1] && ptr_string[2]
                                && ptr_string[3] && ptr_string[4])
                            {
                                ptr_string += 5;
                            }
                        }
                        else
                        {
                            while (gui_color_attr_get_flag (ptr_string[0]) > 0)
                            {
                                ptr_string++;
                            }
                            if (ptr_string[0] && ptr_string[1])
                                ptr_string += 2;
                        }
                        break;
                    case GUI_COLOR_BG_CHAR:
                        ptr_string++;
                        if (ptr_string[0] == GUI_COLOR_EXTENDED_CHAR)
                        {
                            ptr_string++;
                            if (ptr_string[0] && ptr_string[1] && ptr_string[2]
                                && ptr_string[3] && ptr_string[4])
                            {
                                ptr_string += 5;
                            }
                        }
                        else
                        {
                            if (ptr_string[0] && ptr_string[1])
                                ptr_string += 2;
                        }
                        break;
                    case GUI_COLOR_FG_BG_CHAR:
                        ptr_string++;
                        if (ptr_string[0] == GUI_COLOR_EXTENDED_CHAR)
                        {
                            ptr_string++;
                            while (gui_color_attr_get_flag (ptr_string[0]) > 0)
                            {
                                ptr_string++;
                            }
                            if (ptr_string[0] && ptr_string[1] && ptr_string[2]
                                && ptr_string[3] && ptr_string[4])
                            {
                                ptr_string += 5;
                            }
                        }
                        else
                        {
                            while (gui_color_attr_get_flag (ptr_string[0]) > 0)
                            {
                                ptr_string++;
                            }
                            if (ptr_string[0] && ptr_string[1])
                                ptr_string += 2;
                        }
                        /*
                         * note: the comma is an old separator not used any
                         * more (since WeeChat 2.6), but we still use it here
                         * so in case of/upgrade this will not break colors in
                         * old messages
                         */
                        if ((ptr_string[0] == ',') || (ptr_string[0] == '~'))
                        {
                            if (ptr_string[1] == GUI_COLOR_EXTENDED_CHAR)
                            {
                                ptr_string += 2;
                                if (ptr_string[0] && ptr_string[1]
                                    && ptr_string[2] && ptr_string[3]
                                    && ptr_string[4])
                                {
                                    ptr_string += 5;
                                }
                            }
                            else
                            {
                                ptr_string++;
                                if (ptr_string[0] && ptr_string[1])
                                    ptr_string += 2;
                            }
                        }
                        break;
                    case GUI_COLOR_EXTENDED_CHAR:
                        ptr_string++;
                        if ((isdigit (ptr_string[0])) && (isdigit (ptr_string[1]))
                            && (isdigit (ptr_string[2])) && (isdigit (ptr_string[3]))
                            && (isdigit (ptr_string[4])))
                        {
                            ptr_string += 5;
                        }
                        break;
                    case GUI_COLOR_EMPHASIS_CHAR:
                        ptr_string++;
                        break;
                    case GUI_COLOR_BAR_CHAR:
                        ptr_string++;
                        switch (ptr_string[0])
                        {
                            case GUI_COLOR_BAR_FG_CHAR:
                            case GUI_COLOR_BAR_BG_CHAR:
                            case GUI_COLOR_BAR_DELIM_CHAR:
                            case GUI_COLOR_BAR_START_INPUT_CHAR:
                            case GUI_COLOR_BAR_START_INPUT_HIDDEN_CHAR:
                            case GUI_COLOR_BAR_MOVE_CURSOR_CHAR:
                            case GUI_COLOR_BAR_START_ITEM:
                            case GUI_COLOR_BAR_START_LINE_ITEM:
                            case GUI_COLOR_BAR_SPACER:
                                ptr_string++;
                                break;
                        }
                        break;
                    case GUI_COLOR_RESET_CHAR:
                        ptr_string++;
                        break;
                    default:
                        if (isdigit (ptr_string[0]) && isdigit (ptr_string[1]))
                            ptr_string += 2;
                        break;
                }
                if (replacement && replacement[0])
                {
                    out[out_pos] = replacement[0];
                    out_pos++;
                }
                break;
            case GUI_COLOR_SET_ATTR_CHAR:
            case GUI_COLOR_REMOVE_ATTR_CHAR:
                ptr_string++;
                if (ptr_string[0])
                    ptr_string++;
                if (replacement && replacement[0])
                {
                    out[out_pos] = replacement[0];
                    out_pos++;
                }
                break;
            case GUI_COLOR_RESET_CHAR:
                ptr_string++;
                if (replacement && replacement[0])
                {
                    out[out_pos] = replacement[0];
                    out_pos++;
                }
                break;
            default:
                length = utf8_char_size ((char *)ptr_string);
                if (length == 0)
                    length = 1;
                memcpy (out + out_pos, ptr_string, length);
                out_pos += length;
                ptr_string += length;
                break;
        }
    }
    out[out_pos] = '\0';

    return (char *)out;
}

/*
 * Converts ANSI color codes to WeeChat colors (or removes them).
 *
 * This callback is called by gui_color_decode_ansi, it must not be called
 * directly.
 */

char *
gui_color_decode_ansi_cb (void *data, const char *text)
{
    char *text2, **items, *output, str_color[128];
    int i, keep_colors, length, num_items, value;

    keep_colors = (data) ? 1 : 0;;

    /* if we don't keep colors or if text is empty, just return empty string */
    if (!keep_colors || !text || !text[0])
        return strdup ("");

    /* only sequences ending with 'm' are used, the others are discarded */
    length = strlen (text);
    if (text[length - 1] != 'm')
        return strdup ("");

    /* sequence "\33[m" resets color */
    if (length < 4)
        return strdup (gui_color_get_custom ("reset"));

    text2 = NULL;
    items = NULL;
    output = NULL;

    /* extract text between "\33[" and "m" */
    text2 = string_strndup (text + 2, length - 3);
    if (!text2)
        goto end;

    items = string_split (text2, ";", NULL,
                          WEECHAT_STRING_SPLIT_STRIP_LEFT
                          | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                          | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                          0, &num_items);
    if (!items)
        goto end;

    output = malloc ((32 * num_items) + 1);
    if (!output)
        goto end;
    output[0] = '\0';

    for (i = 0; i < num_items; i++)
    {
        value = atoi (items[i]);
        switch (value)
        {
            case 0: /* reset */
                strcat (output, gui_color_get_custom ("reset"));
                break;
            case 1: /* bold */
                strcat (output, gui_color_get_custom ("bold"));
                break;
            case 2: /* dim */
                strcat (output, gui_color_get_custom ("dim"));
                break;
            case 3: /* italic */
                strcat (output, gui_color_get_custom ("italic"));
                break;
            case 4: /* underline */
                strcat (output, gui_color_get_custom ("underline"));
                break;
            case 5: /* blink */
                strcat (output, gui_color_get_custom ("blink"));
                break;
            case 7: /* reverse */
                strcat (output, gui_color_get_custom ("reverse"));
                break;
            case 21: /* remove bold */
                strcat (output, gui_color_get_custom ("-bold"));
                break;
            case 22: /* remove dim */
                strcat (output, gui_color_get_custom ("-dim"));
                break;
            case 23: /* remove italic */
                strcat (output, gui_color_get_custom ("-italic"));
                break;
            case 24: /* remove underline */
                strcat (output, gui_color_get_custom ("-underline"));
                break;
            case 25: /* remove blink */
                strcat (output, gui_color_get_custom ("-blink"));
                break;
            case 27: /* remove reverse */
                strcat (output, gui_color_get_custom ("-reverse"));
                break;
            case 30: /* text color */
            case 31:
            case 32:
            case 33:
            case 34:
            case 35:
            case 36:
            case 37:
                snprintf (str_color, sizeof (str_color),
                          "|%s",
                          gui_color_ansi[value - 30]);
                strcat (output, gui_color_get_custom (str_color));
                break;
            case 38: /* text color */
                if (i + 1 < num_items)
                {
                    switch (atoi (items[i + 1]))
                    {
                        case 2: /* RGB color */
                            if (i + 4 < num_items)
                            {
                                snprintf (str_color, sizeof (str_color),
                                          "|%d",
                                          gui_color_convert_rgb_to_term (
                                              (atoi (items[i + 2]) << 16) |
                                              (atoi (items[i + 3]) << 8) |
                                              atoi (items[i + 4]),
                                              256));
                                strcat (output, gui_color_get_custom (str_color));
                                i += 4;
                            }
                            break;
                        case 5: /* terminal color (0-255) */
                            if (i + 2 < num_items)
                            {
                                snprintf (str_color, sizeof (str_color),
                                          "|%d", atoi (items[i + 2]));
                                strcat (output, gui_color_get_custom (str_color));
                                i += 2;
                            }
                            break;
                    }
                }
                break;
            case 39: /* default text color */
                strcat (output, gui_color_get_custom ("default"));
                break;
            case 40: /* background color */
            case 41:
            case 42:
            case 43:
            case 44:
            case 45:
            case 46:
            case 47:
                snprintf (str_color, sizeof (str_color),
                          "|,%s",
                          gui_color_ansi[value - 40]);
                strcat (output, gui_color_get_custom (str_color));
                break;
            case 48: /* background color */
                if (i + 1 < num_items)
                {
                    switch (atoi (items[i + 1]))
                    {
                        case 2: /* RGB color */
                            if (i + 4 < num_items)
                            {
                                snprintf (str_color, sizeof (str_color),
                                          "|,%d",
                                          gui_color_convert_rgb_to_term (
                                              (atoi (items[i + 2]) << 16) |
                                              (atoi (items[i + 3]) << 8) |
                                              atoi (items[i + 4]),
                                              256));
                                strcat (output, gui_color_get_custom (str_color));
                                i += 4;
                            }
                            break;
                        case 5: /* terminal color (0-255) */
                            if (i + 2 < num_items)
                            {
                                snprintf (str_color, sizeof (str_color),
                                          "|,%d", atoi (items[i + 2]));
                                strcat (output, gui_color_get_custom (str_color));
                                i += 2;
                            }
                            break;
                    }
                }
                break;
            case 49: /* default background color */
                strcat (output, gui_color_get_custom (",default"));
                break;
            case 90: /* text color (bright) */
            case 91:
            case 92:
            case 93:
            case 94:
            case 95:
            case 96:
            case 97:
                snprintf (str_color, sizeof (str_color),
                          "|%s",
                          gui_color_ansi[value - 90 + 8]);
                strcat (output, gui_color_get_custom (str_color));
                break;
            case 100: /* background color (bright) */
            case 101:
            case 102:
            case 103:
            case 104:
            case 105:
            case 106:
            case 107:
                snprintf (str_color, sizeof (str_color),
                          "|,%s",
                          gui_color_ansi[value - 100 + 8]);
                strcat (output, gui_color_get_custom (str_color));
                break;
        }
    }

end:
    if (items)
        string_free_split (items);
    if (text2)
        free (text2);

    return (output) ? output : strdup ("");
}

/*
 * Converts ANSI color codes to WeeChat colors (or removes them).
 *
 * Note: result must be freed after use.
 */

char *
gui_color_decode_ansi (const char *string, int keep_colors)
{
    /* allocate/compile regex if needed (first call) */
    if (!gui_color_regex_ansi)
    {
        gui_color_regex_ansi = malloc (sizeof (*gui_color_regex_ansi));
        if (!gui_color_regex_ansi)
            return NULL;
        if (string_regcomp (gui_color_regex_ansi,
                            GUI_COLOR_REGEX_ANSI_DECODE,
                            REG_EXTENDED) != 0)
        {
            free (gui_color_regex_ansi);
            gui_color_regex_ansi = NULL;
            return NULL;
        }
    }

    return string_replace_regex (string, gui_color_regex_ansi,
                                 "$0", '$',
                                 &gui_color_decode_ansi_cb,
                                 (void *)((unsigned long)keep_colors));
}

/*
 * Adds an ANSI color code with a WeeChat attribute flag.
 */

void
gui_color_add_ansi_flag (char **output, int flag)
{
    switch (flag)
    {
        case GUI_COLOR_EXTENDED_BLINK_FLAG:
            string_dyn_concat (output, "\x1B[5m", -1);
            break;
        case GUI_COLOR_EXTENDED_DIM_FLAG:
            string_dyn_concat (output, "\x1B[2m", -1);
            break;
        case GUI_COLOR_EXTENDED_BOLD_FLAG:
            string_dyn_concat (output, "\x1B[1m", -1);
            break;
        case GUI_COLOR_EXTENDED_REVERSE_FLAG:
            string_dyn_concat (output, "\x1B[7m", -1);
            break;
        case GUI_COLOR_EXTENDED_ITALIC_FLAG:
            string_dyn_concat (output, "\x1B[3m", -1);
            break;
        case GUI_COLOR_EXTENDED_UNDERLINE_FLAG:
            string_dyn_concat (output, "\x1B[4m", -1);
            break;
        case GUI_COLOR_EXTENDED_KEEPATTR_FLAG:
            /* nothing to do here (really? not sure) */
            break;
    }
}

/*
 * Converts a WeeChat color number to an ANSI color number.
 *
 * Returns -1 if the color is not found.
 */

int
gui_color_weechat_to_ansi (int color)
{
    const char *ptr_color_name;
    int i;

    ptr_color_name = gui_color_search_index (color);
    if (!ptr_color_name)
        return -1;

    for (i = 0; i < 16; i++)
    {
        if (strcmp (gui_color_ansi[i], ptr_color_name) == 0)
            return i;
    }

    /* color not found */
    return -1;
}

/*
 * Replaces WeeChat colors by ANSI colors.
 *
 * Note: result must be freed after use.
 */

char *
gui_color_encode_ansi (const char *string)
{
    const unsigned char *ptr_string;
    char **out, str_concat[128], str_color[8], *error;
    int flag, color, length, ansi_color, fg, bg, attrs;

    if (!string)
        return NULL;

    out = string_dyn_alloc (((strlen (string) * 3) / 2) + 1);

    ptr_string = (unsigned char *)string;
    while (ptr_string && ptr_string[0])
    {
        switch (ptr_string[0])
        {
            case GUI_COLOR_COLOR_CHAR:
                ptr_string++;
                switch (ptr_string[0])
                {
                    case GUI_COLOR_FG_CHAR:
                        ptr_string++;
                        if (ptr_string[0] == GUI_COLOR_EXTENDED_CHAR)
                        {
                            ptr_string++;
                            while ((flag = gui_color_attr_get_flag (ptr_string[0])) > 0)
                            {
                                gui_color_add_ansi_flag (out, flag);
                                ptr_string++;
                            }
                            if (ptr_string[0] && ptr_string[1] && ptr_string[2]
                                && ptr_string[3] && ptr_string[4])
                            {
                                memcpy (str_color, ptr_string, 5);
                                str_color[5] = '\0';
                                error = NULL;
                                color = (int)strtol (str_color, &error, 10);
                                if (error && !error[0])
                                {
                                    snprintf (str_concat, sizeof (str_concat),
                                              "\x1B[38;5;%dm",
                                              color);
                                    string_dyn_concat (out, str_concat, -1);
                                }
                                ptr_string += 5;
                            }
                        }
                        else
                        {
                            while ((flag = gui_color_attr_get_flag (ptr_string[0])) > 0)
                            {
                                gui_color_add_ansi_flag (out, flag);
                                ptr_string++;
                            }
                            if (ptr_string[0] && ptr_string[1])
                            {
                                memcpy (str_color, ptr_string, 2);
                                str_color[2] = '\0';
                                error = NULL;
                                color = (int)strtol (str_color, &error, 10);
                                if (error && !error[0])
                                {
                                    ansi_color = gui_color_weechat_to_ansi (color);
                                    if (ansi_color >= 0)
                                    {
                                        snprintf (str_concat, sizeof (str_concat),
                                                  "\x1B[%dm",
                                                  (ansi_color < 8) ?
                                                  ansi_color + 30 : ansi_color - 8 + 90);
                                        string_dyn_concat (out, str_concat, -1);
                                    }
                                }
                                ptr_string += 2;
                            }
                        }
                        break;
                    case GUI_COLOR_BG_CHAR:
                        ptr_string++;
                        if (ptr_string[0] == GUI_COLOR_EXTENDED_CHAR)
                        {
                            ptr_string++;
                            if (ptr_string[0] && ptr_string[1] && ptr_string[2]
                                && ptr_string[3] && ptr_string[4])
                            {
                                memcpy (str_color, ptr_string, 5);
                                str_color[5] = '\0';
                                error = NULL;
                                color = (int)strtol (str_color, &error, 10);
                                if (error && !error[0])
                                {
                                    snprintf (str_concat, sizeof (str_concat),
                                              "\x1B[48;5;%dm",
                                              color);
                                    string_dyn_concat (out, str_concat, -1);
                                }
                                ptr_string += 5;
                            }
                        }
                        else
                        {
                            if (ptr_string[0] && ptr_string[1])
                            {
                                memcpy (str_color, ptr_string, 2);
                                str_color[2] = '\0';
                                error = NULL;
                                color = (int)strtol (str_color, &error, 10);
                                if (error && !error[0])
                                {
                                    ansi_color = gui_color_weechat_to_ansi (color);
                                    if (ansi_color >= 0)
                                    {
                                        snprintf (str_concat, sizeof (str_concat),
                                                  "\x1B[%dm",
                                                  (ansi_color < 8) ?
                                                  ansi_color + 40 : ansi_color - 8 + 100);
                                        string_dyn_concat (out, str_concat, -1);
                                    }
                                }
                                ptr_string += 2;
                            }
                        }
                        break;
                    case GUI_COLOR_FG_BG_CHAR:
                        ptr_string++;
                        if (ptr_string[0] == GUI_COLOR_EXTENDED_CHAR)
                        {
                            ptr_string++;
                            while ((flag = gui_color_attr_get_flag (ptr_string[0])) > 0)
                            {
                                gui_color_add_ansi_flag (out, flag);
                                ptr_string++;
                            }
                            if (ptr_string[0] && ptr_string[1] && ptr_string[2]
                                && ptr_string[3] && ptr_string[4])
                            {
                                memcpy (str_color, ptr_string, 5);
                                str_color[5] = '\0';
                                error = NULL;
                                color = (int)strtol (str_color, &error, 10);
                                if (error && !error[0])
                                {
                                    snprintf (str_concat, sizeof (str_concat),
                                              "\x1B[38;5;%dm",
                                              color);
                                    string_dyn_concat (out, str_concat, -1);
                                }
                                ptr_string += 5;
                            }
                        }
                        else
                        {
                            while ((flag = gui_color_attr_get_flag (ptr_string[0])) > 0)
                            {
                                gui_color_add_ansi_flag (out, flag);
                                ptr_string++;
                            }
                            if (ptr_string[0] && ptr_string[1])
                            {
                                memcpy (str_color, ptr_string, 2);
                                str_color[2] = '\0';
                                error = NULL;
                                color = (int)strtol (str_color, &error, 10);
                                if (error && !error[0])
                                {
                                    ansi_color = gui_color_weechat_to_ansi (color);
                                    if (ansi_color >= 0)
                                    {
                                        snprintf (str_concat, sizeof (str_concat),
                                                  "\x1B[%dm",
                                                  (ansi_color < 8) ?
                                                  ansi_color + 30 : ansi_color - 8 + 90);
                                        string_dyn_concat (out, str_concat, -1);
                                    }
                                }
                                ptr_string += 2;
                            }
                        }
                        /*
                         * note: the comma is an old separator not used any
                         * more (since WeeChat 2.6), but we still use it here
                         * so in case of/upgrade this will not break colors in
                         * old messages
                         */
                        if ((ptr_string[0] == ',') || (ptr_string[0] == '~'))
                        {
                            if (ptr_string[1] == GUI_COLOR_EXTENDED_CHAR)
                            {
                                ptr_string += 2;
                                if (ptr_string[0] && ptr_string[1]
                                    && ptr_string[2] && ptr_string[3]
                                    && ptr_string[4])
                                {
                                    memcpy (str_color, ptr_string, 5);
                                    str_color[5] = '\0';
                                    error = NULL;
                                    color = (int)strtol (str_color, &error, 10);
                                    if (error && !error[0])
                                    {
                                        snprintf (str_concat, sizeof (str_concat),
                                                  "\x1B[48;5;%dm",
                                                  color);
                                        string_dyn_concat (out, str_concat, -1);
                                    }
                                    ptr_string += 5;
                                }
                            }
                            else
                            {
                                ptr_string++;
                                if (ptr_string[0] && ptr_string[1])
                                {
                                    memcpy (str_color, ptr_string, 2);
                                    str_color[2] = '\0';
                                    error = NULL;
                                    color = (int)strtol (str_color, &error, 10);
                                    if (error && !error[0])
                                    {
                                        ansi_color = gui_color_weechat_to_ansi (color);
                                        if (ansi_color >= 0)
                                        {
                                            snprintf (str_concat, sizeof (str_concat),
                                                      "\x1B[%dm",
                                                      (ansi_color < 8) ?
                                                      ansi_color + 40 : ansi_color - 8 + 100);
                                            string_dyn_concat (out, str_concat, -1);
                                        }
                                    }
                                    ptr_string += 2;
                                }
                            }
                        }
                        break;
                    case GUI_COLOR_EXTENDED_CHAR:
                        ptr_string++;
                        if ((isdigit (ptr_string[0])) && (isdigit (ptr_string[1]))
                            && (isdigit (ptr_string[2])) && (isdigit (ptr_string[3]))
                            && (isdigit (ptr_string[4])))
                        {
                            memcpy (str_color, ptr_string, 5);
                            str_color[5] = '\0';
                            error = NULL;
                            color = (int)strtol (str_color, &error, 10);
                            if (error && !error[0])
                            {
                                snprintf (str_concat, sizeof (str_concat),
                                          "\x1B[38;5;%dm",
                                          color);
                                string_dyn_concat (out, str_concat, -1);
                            }
                            ptr_string += 5;
                        }
                        break;
                    case GUI_COLOR_EMPHASIS_CHAR:
                        ptr_string++;
                        break;
                    case GUI_COLOR_BAR_CHAR:
                        ptr_string++;
                        switch (ptr_string[0])
                        {
                            case GUI_COLOR_BAR_FG_CHAR:
                            case GUI_COLOR_BAR_BG_CHAR:
                            case GUI_COLOR_BAR_DELIM_CHAR:
                            case GUI_COLOR_BAR_START_INPUT_CHAR:
                            case GUI_COLOR_BAR_START_INPUT_HIDDEN_CHAR:
                            case GUI_COLOR_BAR_MOVE_CURSOR_CHAR:
                            case GUI_COLOR_BAR_START_ITEM:
                            case GUI_COLOR_BAR_START_LINE_ITEM:
                            case GUI_COLOR_BAR_SPACER:
                                ptr_string++;
                                break;
                        }
                        break;
                    case GUI_COLOR_RESET_CHAR:
                        ptr_string++;
                        string_dyn_concat (out, "\x1B[39m\x1B[49m", -1);
                        break;
                    default:
                        if (isdigit (ptr_string[0]) && isdigit (ptr_string[1]))
                        {
                            memcpy (str_color, ptr_string, 2);
                            str_color[2] = '\0';
                            error = NULL;
                            color = (int)strtol (str_color, &error, 10);
                            if (error && !error[0]
                                && (color >= 0)
                                && (color < GUI_COLOR_NUM_COLORS))
                            {
                                fg = gui_color[color]->foreground;
                                bg = gui_color[color]->background;
                                attrs = gui_color_get_extended_flags (
                                    gui_color[color]->attributes);
                                string_dyn_concat (out, "\x1B[0m", -1);
                                if (attrs & GUI_COLOR_EXTENDED_BLINK_FLAG)
                                    string_dyn_concat (out, "\x1B[5m", -1);
                                if (attrs & GUI_COLOR_EXTENDED_DIM_FLAG)
                                    string_dyn_concat (out, "\x1B[2m", -1);
                                if (attrs & GUI_COLOR_EXTENDED_BOLD_FLAG)
                                    string_dyn_concat (out, "\x1B[1m", -1);
                                if (attrs & GUI_COLOR_EXTENDED_REVERSE_FLAG)
                                    string_dyn_concat (out, "\x1B[7m", -1);
                                if (attrs & GUI_COLOR_EXTENDED_ITALIC_FLAG)
                                    string_dyn_concat (out, "\x1B[3m", -1);
                                if (attrs & GUI_COLOR_EXTENDED_UNDERLINE_FLAG)
                                    string_dyn_concat (out, "\x1B[4m", -1);
                                if ((fg > 0) && (fg & GUI_COLOR_EXTENDED_FLAG))
                                    fg &= GUI_COLOR_EXTENDED_MASK;
                                if ((bg > 0) && (bg & GUI_COLOR_EXTENDED_FLAG))
                                    bg &= GUI_COLOR_EXTENDED_MASK;
                                if (fg < 0)
                                {
                                    string_dyn_concat (out, "\x1B[39m", -1);
                                }
                                else
                                {
                                    snprintf (str_concat, sizeof (str_concat),
                                              "\x1B[38;5;%dm",
                                              fg);
                                    string_dyn_concat (out, str_concat, -1);
                                }
                                if (bg < 0)
                                {
                                    string_dyn_concat (out, "\x1B[49m", -1);
                                }
                                else
                                {
                                    snprintf (str_concat, sizeof (str_concat),
                                              "\x1B[48;5;%dm",
                                              bg);
                                    string_dyn_concat (out, str_concat, -1);
                                }
                            }
                            ptr_string += 2;
                        }
                        break;
                }
                break;
            case GUI_COLOR_SET_ATTR_CHAR:
                ptr_string++;
                if (ptr_string[0])
                {
                    switch (ptr_string[0])
                    {
                        case GUI_COLOR_ATTR_BLINK_CHAR:
                            string_dyn_concat (out, "\x1B[5m", -1);
                            break;
                        case GUI_COLOR_ATTR_DIM_CHAR:
                            string_dyn_concat (out, "\x1B[2m", -1);
                            break;
                        case GUI_COLOR_ATTR_BOLD_CHAR:
                            string_dyn_concat (out, "\x1B[1m", -1);
                            break;
                        case GUI_COLOR_ATTR_REVERSE_CHAR:
                            string_dyn_concat (out, "\x1B[7m", -1);
                            break;
                        case GUI_COLOR_ATTR_ITALIC_CHAR:
                            string_dyn_concat (out, "\x1B[3m", -1);
                            break;
                        case GUI_COLOR_ATTR_UNDERLINE_CHAR:
                            string_dyn_concat (out, "\x1B[4m", -1);
                            break;
                    }
                    ptr_string++;
                }
                break;
            case GUI_COLOR_REMOVE_ATTR_CHAR:
                ptr_string++;
                if (ptr_string[0])
                {
                    switch (ptr_string[0])
                    {
                        case GUI_COLOR_ATTR_BLINK_CHAR:
                            string_dyn_concat (out, "\x1B[25m", -1);
                            break;
                        case GUI_COLOR_ATTR_DIM_CHAR:
                            string_dyn_concat (out, "\x1B[22m", -1);
                            break;
                       case GUI_COLOR_ATTR_BOLD_CHAR:
                            string_dyn_concat (out, "\x1B[21m", -1);
                            break;
                        case GUI_COLOR_ATTR_REVERSE_CHAR:
                            string_dyn_concat (out, "\x1B[27m", -1);
                            break;
                        case GUI_COLOR_ATTR_ITALIC_CHAR:
                            string_dyn_concat (out, "\x1B[23m", -1);
                            break;
                        case GUI_COLOR_ATTR_UNDERLINE_CHAR:
                            string_dyn_concat (out, "\x1B[24m", -1);
                            break;
                    }
                    ptr_string++;
                }
                break;
            case GUI_COLOR_RESET_CHAR:
                string_dyn_concat (out, "\x1B[0m", -1);
                ptr_string++;
                break;
            default:
                length = utf8_char_size ((char *)ptr_string);
                if (length == 0)
                    length = 1;
                string_dyn_concat (out, (const char *)ptr_string, length);
                ptr_string += length;
                break;
        }
    }

    return string_dyn_free (out, 0);
}

/*
 * Emphasizes a string or regular expression in a string (which can contain
 * colors).
 *
 * Argument "case_sensitive" is used only for "search", if the "regex" is NULL.
 *
 * Returns string with the search string/regex emphasized, NULL if error.
 *
 * Note: result must be freed after use.
 */

char *
gui_color_emphasize (const char *string,
                     const char *search, int case_sensitive,
                     regex_t *regex)
{
    regmatch_t regex_match;
    char *result, *result2, *string_no_color;
    const char *ptr_string, *ptr_no_color, *color_emphasis, *pos;
    int rc, length_search, length_emphasis, length_result;
    int pos1, pos2, real_pos1, real_pos2, count_emphasis;

    /* string is required */
    if (!string)
        return NULL;

    /* search or regex is required */
    if (!search && !regex)
        return NULL;

    color_emphasis = gui_color_get_custom ("emphasis");
    if (!color_emphasis)
        return NULL;
    length_emphasis = strlen (color_emphasis);

    /*
     * allocate space for 8 emphasized strings (8 before + 8 after = 16);
     * more space will be allocated later (if there are more than 8 emphasized
     * strings)
     */
    length_result = strlen (string) + (length_emphasis * 2 * 8) + 1;
    result = malloc (length_result);
    if (!result)
        return NULL;
    result[0] = '\0';

    /*
     * build a string without color codes to search in this one (then with the
     * position of text found, we will retrieve position in original string,
     * which can contain color codes)
     */
    string_no_color = gui_color_decode (string, NULL);
    if (!string_no_color)
    {
        free (result);
        return NULL;
    }

    length_search = (search) ? strlen (search) : 0;

    ptr_string = string;
    ptr_no_color = string_no_color;

    count_emphasis = 0;

    while (ptr_no_color && ptr_no_color[0])
    {
        if (regex)
        {
            /* search next match using the regex */
            regex_match.rm_so = -1;
            rc = regexec (regex, ptr_no_color, 1, &regex_match, 0);

            /*
             * no match found: exit the loop (if rm_no == 0, it is an empty
             * match at beginning of string: we consider there is no match, to
             * prevent an infinite loop)
             */
            if ((rc != 0) || (regex_match.rm_so < 0) || (regex_match.rm_eo <= 0))
            {
                strcat (result, ptr_string);
                break;
            }
            pos1 = regex_match.rm_so;
            pos2 = regex_match.rm_eo;
        }
        else
        {
            /* search next match in the string */
            if (case_sensitive)
                pos = strstr (ptr_no_color, search);
            else
                pos = string_strcasestr (ptr_no_color, search);
            if (!pos)
            {
                strcat (result, ptr_string);
                break;
            }
            pos1 = pos - ptr_no_color;
            pos2 = pos1 + length_search;
            if (pos2 <= 0)
            {
                strcat (result, ptr_string);
                break;
            }
        }

        /*
         * find the position of match in the original string (which can contain
         * color codes)
         */
        real_pos1 = gui_chat_string_real_pos (ptr_string,
                                              gui_chat_string_pos (ptr_no_color, pos1),
                                              0);
        real_pos2 = gui_chat_string_real_pos (ptr_string,
                                              gui_chat_string_pos (ptr_no_color, pos2),
                                              0);

        /*
         * concatenate following strings to the result:
         * - beginning of string (before match)
         * - emphasis color code
         * - the matching string
         * - emphasis color code
         */
        if (real_pos1 > 0)
            strncat (result, ptr_string, real_pos1);
        strcat (result, color_emphasis);
        strncat (result, ptr_string + real_pos1, real_pos2 - real_pos1);
        strcat (result, color_emphasis);

        /* restart next loop after the matching string */
        ptr_string += real_pos2;
        ptr_no_color += pos2;

        /* check if we should allocate more space for emphasis color codes */
        count_emphasis++;
        if (count_emphasis == 8)
        {
            /* allocate more space for emphasis color codes */
            length_result += (length_emphasis * 2 * 8);
            result2 = realloc (result, length_result);
            if (!result2)
            {
                free (result);
                return NULL;
            }
            result = result2;
            count_emphasis = 0;
        }
    }

    free (string_no_color);

    return result;
}

/*
 * Frees a color.
 */

void
gui_color_free (struct t_gui_color *color)
{
    if (!color)
        return;

    if (color->string)
        free (color->string);

    free (color);
}

/*
 * Callback called to free value in hashtable when item in hashtable is removed.
 */

void
gui_color_palette_free_value_cb (struct t_hashtable *hashtable,
                                 const void *key, void *value)
{
    struct t_gui_color_palette *color_palette;

    /* make C compiler happy */
    (void) hashtable;
    (void) key;

    color_palette = (struct t_gui_color_palette *)value;

    if (color_palette)
        gui_color_palette_free (color_palette);
}

/*
 * Allocates hashtables and lists for palette.
 */

void
gui_color_palette_alloc_structs ()
{
    if (!gui_color_hash_palette_color)
    {
        gui_color_hash_palette_color = hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_POINTER,
            NULL, NULL);
        gui_color_hash_palette_color->callback_free_value = &gui_color_palette_free_value_cb;
    }
    if (!gui_color_hash_palette_alias)
    {
        gui_color_hash_palette_alias = hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_INTEGER,
            NULL, NULL);
    }
    if (!gui_color_list_with_alias)
    {
        gui_color_list_with_alias = weelist_new ();
    }
}

/*
 * Gets color pair number with alias.
 *
 * Returns -1 if alias is not found.
 */

int
gui_color_palette_get_alias (const char *alias)
{
    int *ptr_number;

    if (gui_color_hash_palette_alias)
    {
        ptr_number = hashtable_get (gui_color_hash_palette_alias, alias);
        if (ptr_number)
            return *ptr_number;
    }

    /* alias not found */
    return -1;
}

/*
 * Gets a color palette with number.
 */

struct t_gui_color_palette *
gui_color_palette_get (int number)
{
    char str_number[64];

    snprintf (str_number, sizeof (str_number), "%d", number);
    return hashtable_get (gui_color_hash_palette_color,
                          str_number);
}

/*
 * Adds a color in palette.
 */

void
gui_color_palette_add (int number, const char *value)
{
    struct t_gui_color_palette *new_color_palette;
    char str_number[64];

    gui_color_palette_alloc_structs ();

    new_color_palette = gui_color_palette_new (number, value);
    if (!new_color_palette)
        return;

    snprintf (str_number, sizeof (str_number), "%d", number);
    hashtable_set (gui_color_hash_palette_color,
                   str_number, new_color_palette);
    gui_color_palette_build_aliases ();

    if (gui_init_ok)
        gui_color_buffer_display ();
}

/*
 * Removes a color in palette.
 */

void
gui_color_palette_remove (int number)
{
    struct t_gui_color_palette *ptr_color_palette;
    char str_number[64];

    gui_color_palette_alloc_structs ();

    snprintf (str_number, sizeof (str_number), "%d", number);
    ptr_color_palette = hashtable_get (gui_color_hash_palette_color,
                                       str_number);
    if (ptr_color_palette)
    {
        hashtable_remove (gui_color_hash_palette_color, str_number);
        gui_color_palette_build_aliases ();

        if (gui_init_ok)
            gui_color_buffer_display ();
    }
}

/*
 * Frees hashtables and lists for palette.
 */

void
gui_color_palette_free_structs ()
{
    if (gui_color_hash_palette_color)
        hashtable_free (gui_color_hash_palette_color);
    if (gui_color_hash_palette_alias)
        hashtable_free (gui_color_hash_palette_alias);
    if (gui_color_list_with_alias)
        weelist_free (gui_color_list_with_alias);
}

/*
 * Initializes colors.
 */

void
gui_color_init ()
{
    int i;

    for (i = 0; i < GUI_COLOR_NUM_COLORS; i++)
    {
        gui_color[i] = NULL;
    }
}

/*
 * Ends GUI colors.
 */

void
gui_color_end ()
{
    int i;

    for (i = 0; i < GUI_COLOR_NUM_COLORS; i++)
    {
        gui_color_free (gui_color[i]);
        gui_color[i] = NULL;
    }
    gui_color_palette_free_structs ();
    gui_color_free_vars ();

    if (gui_color_regex_ansi)
    {
        regfree (gui_color_regex_ansi);
        free (gui_color_regex_ansi);
        gui_color_regex_ansi = NULL;
    }
}
