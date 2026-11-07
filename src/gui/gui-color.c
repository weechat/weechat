/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Color functions (used by all GUI) */

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
#include <stdint.h>
#include <sys/time.h>
#include <gcrypt.h>

#include "../core/weechat.h"
#include "../core/core-config.h"
#include "../core/core-hashtable.h"
#include "../core/core-list.h"
#include "../core/core-string.h"
#include "../core/core-utf8.h"
#include "../core/core-util.h"
#include "../plugins/plugin.h"
#include "gui-color.h"
#include "gui-chat.h"
#include "gui-window.h"


struct t_gui_color *gui_color[GUI_COLOR_NUM_COLORS]; /* GUI colors          */

/* Palette colors and aliases */
struct t_hashtable *gui_color_hash_palette_color = NULL;
struct t_hashtable *gui_color_hash_palette_alias = NULL;
struct t_weelist *gui_color_list_with_alias = NULL;

/* RGB colors (interned, see GUI_COLOR_RGB_FIRST in gui-color.h) */
struct t_gui_color_rgb_table gui_color_rgb_options =   /* used in options    */
{ GUI_COLOR_RGB_OPTIONS_FIRST, GUI_COLOR_RGB_OPTIONS_MAX,
  NULL, NULL, 0, 0, 0, NULL };
struct t_gui_color_rgb_table gui_color_rgb_display =   /* used in strings    */
{ GUI_COLOR_RGB_DISPLAY_FIRST, GUI_COLOR_RGB_DISPLAY_MAX,
  NULL, NULL, 0, 0, 0, NULL };
int gui_color_rgb_display_reset_needed = 0;  /* display table is full       */
time_t gui_color_rgb_display_reset_last = 0; /* time of last reset          */
unsigned long long gui_color_hash_seed = 0;  /* random seed for hash of keys */
int gui_color_hash_seed_set = 0;             /* 1 if seed is set            */

/* Terminal colors */
/* Cache of RGB colors converted to the 256-color palette (RGB + 1, 0 = empty) */
int gui_color_downgrade_cache_rgb[GUI_COLOR_DOWNGRADE_CACHE_SIZE];
int gui_color_downgrade_cache_term[GUI_COLOR_DOWNGRADE_CACHE_SIZE];

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
 * Return a color code from an option, which can be a color or a string.
 *
 * Return NULL if the option has a wrong type.
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

    /* Never executed */
    return NULL;
}

/*
 * Search for a color with configuration option name.
 *
 * Return color string, NULL if not found.
 */

const char *
gui_color_search_config (const char *color_name)
{
    struct t_config_option *ptr_option;

    if (!color_name)
        return NULL;

    /* Search in weechat.conf colors (example: "chat_delimiters"). */
    for (ptr_option = weechat_config_section_color->options;
         ptr_option; ptr_option = ptr_option->next_option)
    {
        if (strcmp (ptr_option->name, color_name) == 0)
            return gui_color_from_option (ptr_option);
    }

    /* Search in any configuration file (example: "irc.color.message_quit"). */
    if (strchr (color_name, '.'))
    {
        config_file_search_with_string (color_name, NULL, NULL, &ptr_option, NULL);
        if (ptr_option)
            return gui_color_from_option (ptr_option);
    }

    /* Color not found */
    return NULL;
}

/*
 * Check if a char is an attribute after a set/remove attribute char (for
 * example GUI_COLOR_ATTR_BOLD_CHAR).
 *
 * Return 1 if the char is an attribute, 0 otherwise.
 */

int
gui_color_attr_char_is_valid (char c)
{
    return ((c >= GUI_COLOR_ATTR_BOLD_CHAR)
            && (c <= GUI_COLOR_ATTR_DIM_CHAR)) ? 1 : 0;
}

/*
 * Return flag for attribute char of a color.
 *
 * Return 0 if char is unknown.
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
 * Build string with attributes of color.
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
 * Build a part of a custom color code, for a text color (with attributes) or a
 * background (attributes NULL): RGB color ("@" + attributes + "#rrggbb"),
 * terminal color or alias ("@" + attributes + "00123") or WeeChat color
 * (attributes + "01"); an empty string if the color is not found.
 */

void
gui_color_get_custom_part (char *buffer, int size, const char *name,
                           const char *attributes)
{
    int term_color, rgb, color;

    buffer[0] = '\0';

    if (!name)
        return;

    if (!attributes)
        attributes = "";

    rgb = -1;
    term_color = gui_color_palette_get_alias (name);
    if (term_color < 0)
    {
        rgb = gui_color_rgb_parse_exact (name);
        if ((rgb < 0) && util_parse_int (name, 10, &term_color))
        {
            if (term_color < 0)
                term_color = 0;
            else if (term_color > GUI_COLOR_EXTENDED_MAX)
                term_color = GUI_COLOR_EXTENDED_MAX;
        }
        else
        {
            term_color = -1;
        }
    }

    if (rgb >= 0)
    {
        snprintf (buffer, size, "%c%s%c%06x",
                  GUI_COLOR_EXTENDED_CHAR, attributes,
                  GUI_COLOR_EXTENDED_RGB_CHAR, rgb);
    }
    else if (term_color >= 0)
    {
        snprintf (buffer, size, "%c%s%05d",
                  GUI_COLOR_EXTENDED_CHAR, attributes, term_color);
    }
    else
    {
        color = gui_color_search (name);
        if (color >= 0)
            snprintf (buffer, size, "%s%02d", attributes, color);
    }
}

/*
 * Get a custom color with a name.
 */

const char *
gui_color_get_custom (const char *color_name)
{
    static char color[32][96];
    static int index_color = 0;
    char color_fg[32], color_bg[32];
    char *str_fg, *color_attr;
    const char *ptr_color_name, *pos_delim, *pos_bg;

    /* Attribute or other color name (GUI dependent) */
    index_color = (index_color + 1) % 32;
    color[index_color][0] = '\0';

    if (!color_name || !color_name[0])
        return color[index_color];

    /* Read extra attributes (bold, ..). */
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
        /* Custom color name (GUI dependent) */
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

        gui_color_get_custom_part (color_fg, sizeof (color_fg), str_fg,
                                   color_attr);
        gui_color_get_custom_part (color_bg, sizeof (color_bg), pos_bg, NULL);

        if (color_fg[0] && color_bg[0])
        {
            /*
             * Note: until WeeChat 2.5, the separator was a comma, and it has
             * been changed to a tilde (to prevent problems with /eval and
             * ${color:FF,BB}.
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

        free (str_fg);
    }

    free (color_attr);

    return color[index_color];
}

/*
 * Convert a terminal color to its RGB value.
 *
 * Return a RGB color as integer.
 */

int
gui_color_convert_term_to_rgb (int color)
{
    if ((color < 0) || (color > 255))
        return 0;

    return gui_color_term256[color];
}

/*
 * Get the RGB value of a terminal color in a palette of "limit" colors.
 *
 * Terminals with 88 colors (for example rxvt-unicode) have their own palette
 * after the 16 basic colors: a 4x4x4 color cube (16-79) and 8 grays (80-87);
 * other palettes use the colors of the 256-color palette.
 *
 * Return the RGB value, -1 if the color is not in the palette.
 */

int
gui_color_palette_rgb (int color, int limit)
{
    int levels[4] = { 0x00, 0x8b, 0xcd, 0xff };
    int grays[8] = { 0x2e, 0x5c, 0x73, 0x8b, 0xa2, 0xb9, 0xd0, 0xe7 };

    if ((color < 0) || (color >= 256) || (color >= limit))
        return -1;

    if ((limit == 88) && (color >= 16))
    {
        if (color < 80)
        {
            color -= 16;
            return (levels[color / 16] << 16)
                | (levels[(color / 4) % 4] << 8)
                | levels[color % 4];
        }
        return (grays[color - 80] << 16) | (grays[color - 80] << 8)
            | grays[color - 80];
    }

    return gui_color_term256[color];
}

/*
 * Convert a RGB color to the closest terminal color.
 *
 * Argument "limit" is the number of colors to check in the table of terminal
 * colors (starting from 0). So for example 256 will return any of the 256
 * colors, 16 will return a color between 0 and 15. A limit greater than 256
 * is the same as 256, and a limit lower than 1 returns 0. With 88 colors, the
 * palette of terminals with 88 colors is used (see function
 * gui_color_palette_rgb).
 *
 * Return the closest terminal color (0-255).
 */

int
gui_color_convert_rgb_to_term (int rgb, int limit)
{
    return gui_color_convert_rgb_to_term_range (rgb, 0, limit);
}

/*
 * Convert a RGB color to the closest terminal color to display it: on a
 * terminal with more than 16 colors, colors 0-15 are not used, because they
 * are defined by the theme of the terminal (for example color 8 can be the
 * background color).
 *
 * Return the closest terminal color (0-255).
 */

int
gui_color_convert_rgb_to_term_display (int rgb, int limit)
{
    return gui_color_convert_rgb_to_term_range (rgb, (limit > 16) ? 16 : 0,
                                                limit);
}

/*
 * Convert a RGB color to the closest terminal color between "first" and
 * "limit" - 1 (see function gui_color_convert_rgb_to_term).
 *
 * Return the closest terminal color (0-255).
 */

int
gui_color_convert_rgb_to_term_range (int rgb, int first, int limit)
{
    int i, r1, g1, b1, rgb2, r2, g2, b2, diff, best_diff, best_color;

    if (limit > 256)
        limit = 256;
    if ((first < 0) || (first >= limit))
        first = 0;

    r1 = rgb >> 16;
    g1 = (rgb >> 8) & 0xFF;
    b1 = rgb & 0xFF;

    best_diff = INT_MAX;
    best_color = first;

    for (i = first; i < limit; i++)
    {
        rgb2 = gui_color_palette_rgb (i, limit);
        r2 = rgb2 >> 16;
        g2 = (rgb2 >> 8) & 0xFF;
        b2 = rgb2 & 0xFF;

        diff = ((r2 - r1) * (r2 - r1)) +
            ((g2 - g1) * (g2 - g1)) +
            ((b2 - b1) * (b2 - b1));

        /* Exact match! */
        if (diff == 0)
            return i;

        if (diff < best_diff)
        {
            best_color = i;
            best_diff = diff;
        }
    }

    /* Return the closest color. */
    return best_color;
}

/*
 * Check if an ANSI escape sequence (for example built with terminfo
 * capability "setaf") sets a terminal palette color, and not a RGB color.
 *
 * Return:
 *   1: sequence sets a palette color (for example "\e[97m" or "\e[38;5;214m")
 *   0: sequence sets a RGB color (for example "\e[38:2::0:0:255m") or is NULL
 */

int
gui_color_ansi_is_indexed (const char *sequence)
{
    if (!sequence)
        return 0;

    return (strstr (sequence, "38:2") || strstr (sequence, "38;2")
            || strstr (sequence, "48:2") || strstr (sequence, "48;2")) ?
        0 : 1;
}

/*
 * Convert a terminal color (0-255) to a color for a terminal with direct
 * colors (like "xterm-direct", where COLORS is 16777216).
 *
 * With such terminals, the first "num_indexed" colors are palette colors
 * (8 with "xterm-direct", 16 with "xterm-direct16", 256 with
 * "xterm-direct256") and any other value is a RGB color (0xRRGGBB).
 *
 * Return:
 *   - color unchanged if < num_indexed (including -1 for default color)
 *     or > 255
 *   - RGB value of color otherwise (see function gui_color_rgb_to_direct)
 */

int
gui_color_convert_term_to_direct (int color, int num_indexed)
{
    int rgb;

    if ((color < num_indexed) || (color > 255))
        return color;

    rgb = gui_color_term256[color];

    return gui_color_rgb_to_direct (rgb);
}

/*
 * Convert a RGB value to a color for a terminal with direct colors.
 *
 * With direct colors, the first colors are terminal palette indexes
 * (0-7 with "xterm-direct", 0-255 with "xterm-direct256"), so the RGB values
 * below 256 (blue component only) are changed to use a green component of 1,
 * which is visually identical.
 */

int
gui_color_rgb_to_direct (int rgb)
{
    if (rgb < 0)
        return rgb;

    return (rgb < 256) ? (rgb | 0x000100) : rgb;
}

/*
 * Convert a hue (angle in degrees, 0 = red, 120 = green, 240 = blue) to a
 * RGB color with full saturation and brightness.
 *
 * Return the RGB value (0xRRGGBB).
 */

int
gui_color_hue_to_rgb (int hue)
{
    int remainder, r, g, b;

    hue %= 360;
    if (hue < 0)
        hue += 360;

    remainder = ((hue % 60) * 255) / 60;

    switch (hue / 60)
    {
        case 0:
            r = 255; g = remainder; b = 0;
            break;
        case 1:
            r = 255 - remainder; g = 255; b = 0;
            break;
        case 2:
            r = 0; g = 255; b = remainder;
            break;
        case 3:
            r = 0; g = 255 - remainder; b = 255;
            break;
        case 4:
            r = remainder; g = 0; b = 255;
            break;
        default:
            r = 255; g = 0; b = 255 - remainder;
            break;
    }

    return (r << 16) | (g << 8) | b;
}

/*
 * Suggest a value of TERM with direct colors (RGB colors displayed as-is),
 * using the current value of TERM: the name before the first "-" is used to
 * find a terminfo entry with direct colors provided by ncurses.
 *
 * If "tmux" is 1 (WeeChat is running under tmux), "tmux-direct" is returned
 * whatever the value of TERM.
 *
 * Return a TERM value, NULL if no value is known (for example with
 * GNU screen or the Linux console, which have no terminfo entry with direct
 * colors).
 */

const char *
gui_color_suggest_term_direct (const char *term, int tmux)
{
    static char str_term[64];
    const char *terms_direct[] = {
        "alacritty", "contour", "foot", "iterm2", "kitty", "konsole",
        "mintty", "mlterm", "rio", "st", "tmux", "vscode", "vte", NULL,
    };
    const char *pos;
    int i, length;

    if (tmux)
        return "tmux-direct";

    if (!term || !term[0])
        return "xterm-direct256";

    if ((strncmp (term, "screen", 6) == 0) || (strcmp (term, "linux") == 0))
        return NULL;

    pos = strchr (term, '-');
    length = (pos) ? pos - term : (int)strlen (term);

    for (i = 0; terms_direct[i]; i++)
    {
        if (((int)strlen (terms_direct[i]) == length)
            && (strncmp (term, terms_direct[i], length) == 0))
        {
            snprintf (str_term, sizeof (str_term),
                      "%s-direct", terms_direct[i]);
            return str_term;
        }
    }

    return "xterm-direct256";
}

/*
 * Parse a RGB color at the beginning of a string: "#" followed by exactly
 * 6 hexadecimal digits (other chars may follow).
 *
 * Return the RGB value (0x000000 to 0xFFFFFF), -1 if the string does not
 * start with a RGB color.
 */

int
gui_color_rgb_parse (const char *string)
{
    int i, rgb, digit;

    if (!string || (string[0] != GUI_COLOR_EXTENDED_RGB_CHAR))
        return -1;

    rgb = 0;
    for (i = 1; i <= 6; i++)
    {
        if (!isxdigit ((unsigned char)string[i]))
            return -1;
        digit = (isdigit ((unsigned char)string[i])) ?
            string[i] - '0' : (tolower ((unsigned char)string[i]) - 'a' + 10);
        rgb = (rgb << 4) | digit;
    }

    return rgb;
}

/*
 * Hash a key of type integer or long long (with a mix of all bits, so that
 * keys which differ only in high bits are spread in hashtable).
 *
 * It is used for hashtables with RGB colors and color pairs, where keys can
 * be chosen by remote users (colors received in messages): a random seed
 * (chosen once per process) is mixed with the key, so that keys with the same
 * hash can not be computed in advance.
 */

unsigned long long
gui_color_hash_key_cb (struct t_hashtable *hashtable, const void *key)
{
    unsigned long long value, nonce;
    struct timeval tv;

    if (!key)
        return 0ULL;

    if (!gui_color_hash_seed_set)
    {
        /*
         * Seed: random bytes of gcrypt (if initialized), mixed with the
         * pseudo-random generator seeded on startup, the time, the process id
         * and an address of the process.
         */
        gettimeofday (&tv, NULL);
        gui_color_hash_seed = (((unsigned long long)rand ()) << 33)
            ^ (((unsigned long long)rand ()) << 11)
            ^ (unsigned long long)rand ()
            ^ (((unsigned long long)tv.tv_sec) << 20)
            ^ (unsigned long long)tv.tv_usec
            ^ (((unsigned long long)getpid ()) << 40)
            ^ (unsigned long long)(uintptr_t)&gui_color_hash_seed;
        if (!weechat_no_gcrypt)
        {
            nonce = 0;
            gcry_create_nonce (&nonce, sizeof (nonce));
            gui_color_hash_seed ^= nonce;
        }
        gui_color_hash_seed_set = 1;
    }

    value = (hashtable->type_keys == HASHTABLE_LONGLONG) ?
        (unsigned long long)(*((long long *)key)) :
        (unsigned long long)(*((int *)key));

    return hashtable_hash_mix64 (value ^ gui_color_hash_seed);
}

/*
 * Search or add a RGB value in a table of RGB colors.
 *
 * Return the color number, -1 if the table is full or on error.
 */

int
gui_color_rgb_table_add (struct t_gui_color_rgb_table *table, int rgb)
{
    int *ptr_index, new_size, *new_values, *new_term;

    if (!table->index)
    {
        table->index = hashtable_new (256,
                                      WEECHAT_HASHTABLE_INTEGER,
                                      WEECHAT_HASHTABLE_INTEGER,
                                      &gui_color_hash_key_cb, NULL);
        if (!table->index)
            return -1;
    }

    ptr_index = hashtable_get (table->index, &rgb);
    if (ptr_index)
        return table->first + *ptr_index;

    if (table->count >= table->max)
        return -1;

    if (table->count >= table->size)
    {
        new_size = (table->size == 0) ? 64 : table->size * 2;
        if (new_size > table->max)
            new_size = table->max;
        new_values = realloc (table->values,
                              sizeof (table->values[0]) * new_size);
        if (!new_values)
            return -1;
        table->values = new_values;
        new_term = realloc (table->term, sizeof (table->term[0]) * new_size);
        if (!new_term)
            return -1;
        table->term = new_term;
        table->size = new_size;
    }

    table->values[table->count] = rgb;
    table->term[table->count] = -1;
    if (!hashtable_set (table->index, &rgb, &table->count))
        return -1;
    table->count++;

    return table->first + table->count - 1;
}

/*
 * Search a color number in tables of RGB colors.
 *
 * Return the table and set *index to the index of color in table, NULL if
 * the number is not a RGB color.
 */

struct t_gui_color_rgb_table *
gui_color_rgb_table_search (int number, int *index)
{
    struct t_gui_color_rgb_table *tables[2];
    int i;

    tables[0] = &gui_color_rgb_options;
    tables[1] = &gui_color_rgb_display;
    for (i = 0; i < 2; i++)
    {
        if ((number >= tables[i]->first)
            && (number < tables[i]->first + tables[i]->count))
        {
            *index = number - tables[i]->first;
            return tables[i];
        }
    }

    return NULL;
}

/*
 * Free a table of RGB colors (the table can be used again after this call).
 */

void
gui_color_rgb_table_free (struct t_gui_color_rgb_table *table)
{
    hashtable_free (table->index);
    table->index = NULL;
    free (table->values);
    table->values = NULL;
    free (table->term);
    table->term = NULL;
    table->count = 0;
    table->size = 0;
    table->term_limit = 0;
}

/*
 * Parse a string which is exactly a RGB color ("#" followed by 6 hexadecimal
 * digits, without any other char).
 *
 * Return the RGB value (0x000000 to 0xFFFFFF), -1 if the string is not
 * exactly a RGB color.
 */

int
gui_color_rgb_parse_exact (const char *string)
{
    int rgb;

    rgb = gui_color_rgb_parse (string);

    return ((rgb >= 0) && !string[7]) ? rgb : -1;
}

/*
 * Get color number for a RGB value used in a string (for display).
 *
 * The RGB value is added to the table of displayed RGB colors if needed
 * (unless it is already used in an option). This table is emptied when color
 * pairs are reset; when it is full, the nearest color in the 256-color palette
 * is returned, and a reset is requested (at most once per second), unless the
 * automatic reset is disabled (option weechat.look.color_pairs_auto_reset set
 * to -1): then a manual "/color reset" is needed.
 *
 * Return a number between GUI_COLOR_RGB_FIRST and GUI_COLOR_RGB_LAST, or a
 * number between 0 and 255.
 */

int
gui_color_rgb_to_number (int rgb)
{
    int *ptr_index, number;
    time_t now;

    if (rgb < 0)
        rgb = 0;
    rgb &= 0xFFFFFF;

    if (gui_color_rgb_options.index)
    {
        ptr_index = hashtable_get (gui_color_rgb_options.index, &rgb);
        if (ptr_index)
            return gui_color_rgb_options.first + *ptr_index;
    }

    number = gui_color_rgb_table_add (&gui_color_rgb_display, rgb);
    if (number >= 0)
        return number;

    if ((gui_color_rgb_display.count >= gui_color_rgb_display.max)
        && (CONFIG_INTEGER(config_look_color_pairs_auto_reset) >= 0))
    {
        now = time (NULL);
        if (now > gui_color_rgb_display_reset_last)
            gui_color_rgb_display_reset_needed = 1;
    }

    /*
     * Nearest color of the terminal palette (colors are displayed with the
     * palette of the terminal, for example with 88 colors).
     */
    return gui_color_convert_rgb_to_term_display (
        rgb, gui_color_get_term_colors ());
}

/*
 * Get color number for a RGB value used in an option (this number never
 * changes).
 *
 * Return a number between GUI_COLOR_RGB_FIRST and GUI_COLOR_RGB_LAST, -1 if
 * there are too many RGB colors in options.
 */

int
gui_color_rgb_to_number_option (int rgb)
{
    if (rgb < 0)
        rgb = 0;
    rgb &= 0xFFFFFF;

    return gui_color_rgb_table_add (&gui_color_rgb_options, rgb);
}

/*
 * Remove RGB colors used for display (not the ones used in options).
 *
 * It must be called only when color pairs are reset, because color numbers
 * of these RGB colors are not valid any more.
 */

void
gui_color_rgb_display_reset (void)
{
    gui_color_rgb_table_free (&gui_color_rgb_display);
    gui_color_rgb_display_reset_needed = 0;
    gui_color_rgb_display_reset_last = time (NULL);
}

/*
 * Get RGB value of a color number.
 *
 * Return the RGB value, -1 if the number is not a RGB color.
 */

int
gui_color_rgb_from_number (int number)
{
    struct t_gui_color_rgb_table *table;
    int index;

    table = gui_color_rgb_table_search (number, &index);

    return (table) ? table->values[index] : -1;
}

/*
 * Get the nearest terminal color of a RGB color number to display it, using
 * only the first "limit" terminal colors (see function
 * gui_color_convert_rgb_to_term_display); the result is cached.
 *
 * Return the terminal color, -1 if the number is not a RGB color.
 */

int
gui_color_rgb_to_term_cached (int number, int limit)
{
    struct t_gui_color_rgb_table *table;
    int i, index;

    table = gui_color_rgb_table_search (number, &index);
    if (!table)
        return -1;

    if (limit != table->term_limit)
    {
        for (i = 0; i < table->count; i++)
        {
            table->term[i] = -1;
        }
        table->term_limit = limit;
    }

    if (table->term[index] < 0)
    {
        table->term[index] = gui_color_convert_rgb_to_term_display (
            table->values[index], limit);
    }

    return table->term[index];
}

/*
 * Return the size of the extended color at the beginning of "string" (string
 * is after the extended char and the attributes): 7 for a RGB color
 * ("#RRGGBB"), 5 for a color number ("00123"), 0 if string is too short.
 */

int
gui_color_extended_size (const char *string)
{
    if (!string)
        return 0;

    if (gui_color_rgb_parse (string) >= 0)
        return 7;

    if (string[0] && string[1] && string[2] && string[3] && string[4])
        return 5;

    return 0;
}

/*
 * Read the extended color at the beginning of "string" (string is after the
 * extended char and the attributes): a RGB color ("#RRGGBB") or a color
 * number ("00123"), without adding the RGB color to the table of RGB colors.
 *
 * The RGB value is stored in *rgb (-1 if not a RGB color) and the color
 * number in *color (-1 if not a valid color number).
 *
 * Return the size of color in string (see gui_color_extended_size).
 */

int
gui_color_extended_value (const char *string, int *rgb, int *color)
{
    char str_color[6];
    int size;

    *color = -1;

    *rgb = gui_color_rgb_parse (string);
    if (*rgb >= 0)
        return 7;

    size = gui_color_extended_size (string);
    if (size == 5)
    {
        memcpy (str_color, string, 5);
        str_color[5] = '\0';
        if (!util_parse_int (str_color, 10, color) || (*color < 0))
            *color = -1;
    }

    return size;
}

/*
 * Parse the extended color at the beginning of "string" (string is after
 * the extended char and the attributes): a RGB color ("#RRGGBB") or a color
 * number ("00123").
 *
 * If "color" is not NULL, the color number is stored in *color (-1 if
 * invalid): a RGB color is added to the table of RGB colors and its number
 * is returned. If "color" is NULL, only the size is returned.
 *
 * Return the size of color in string (see gui_color_extended_size).
 */

int
gui_color_extended_parse (const char *string, int *color)
{
    int rgb, size;

    if (!color)
        return gui_color_extended_size (string);

    size = gui_color_extended_value (string, &rgb, color);
    if (rgb >= 0)
        *color = gui_color_rgb_to_number (rgb);

    return size;
}

/*
 * Return the string for an extended color number, used in WeeChat color codes
 * after the extended char: "#RRGGBB" for a RGB color, "00123" for other
 * colors.
 */

const char *
gui_color_extended_string (int number)
{
    static char str_color[16][16];
    static int index_color = 0;
    int rgb;

    index_color = (index_color + 1) % 16;

    rgb = gui_color_rgb_from_number (number);
    if (rgb >= 0)
    {
        snprintf (str_color[index_color], sizeof (str_color[index_color]),
                  "%c%06x", GUI_COLOR_EXTENDED_RGB_CHAR, rgb);
    }
    else
    {
        snprintf (str_color[index_color], sizeof (str_color[index_color]),
                  "%05d", number);
    }

    return str_color[index_color];
}

/*
 * Return the size (in bytes) of the WeeChat color code at the beginning
 * of "string".
 *
 * Return 0 if "string" is NULL, empty or does not start with a color code.
 *
 * Return the size of the first color code if "string" begins with multiple
 * color codes.
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
                        ptr_string += gui_color_extended_size ((const char *)ptr_string);
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
                        ptr_string += gui_color_extended_size ((const char *)ptr_string);
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
                        ptr_string += gui_color_extended_size ((const char *)ptr_string);
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
                     * Note: the comma is an old separator not used any
                     * more (since WeeChat 2.6), but we still use it here
                     * so in case of/upgrade this will not break colors in
                     * old messages.
                     */
                    if ((ptr_string[0] == ',') || (ptr_string[0] == '~'))
                    {
                        if (ptr_string[1] == GUI_COLOR_EXTENDED_CHAR)
                        {
                            ptr_string += 2;
                            ptr_string += gui_color_extended_size ((const char *)ptr_string);
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
        case GUI_COLOR_SET_ATTR_CHAR:
        case GUI_COLOR_REMOVE_ATTR_CHAR:
            /*
             * The next char is part of the code only if it is an attribute,
             * like on screen.
             */
            ptr_string++;
            if (gui_color_attr_char_is_valid (ptr_string[0]))
                ptr_string++;
            return ptr_string - string;
        case GUI_COLOR_RESET_CHAR:
            ptr_string++;
            return ptr_string - string;
    }

    return 0;
}

/*
 * Remove WeeChat color codes from a message and optionally replaces them
 * by a string.
 *
 * Note: result must be freed after use.
 */

char *
gui_color_decode (const char *string, const char *replacement)
{
    const char *ptr_string;
    char **out;
    int size;

    if (!string)
        return NULL;

    out = string_dyn_alloc (strlen (string) + 1);
    if (!out)
        return NULL;

    ptr_string = string;
    while (ptr_string[0])
    {
        size = gui_color_code_size (ptr_string);
        if (size > 0)
        {
            if (replacement && replacement[0])
                string_dyn_concat (out, replacement, -1);
        }
        else
        {
            size = utf8_char_size (ptr_string);
            if (size == 0)
                size = 1;
            string_dyn_concat (out, ptr_string, size);
        }
        ptr_string += size;
    }

    return string_dyn_free (out, 0);
}

/*
 * Convert a RGB color to the nearest color of the 256-color palette, with a
 * small cache (the RGB colors of a string are often the same); like on
 * screen, colors 0-15 are not used, because they are defined by the theme of
 * the terminal (see function gui_color_convert_rgb_to_term_display).
 *
 * Return the terminal color (16-255).
 */

int
gui_color_downgrade_rgb_term (int rgb)
{
    int index;

    index = (rgb ^ (rgb >> 6) ^ (rgb >> 12))
        & (GUI_COLOR_DOWNGRADE_CACHE_SIZE - 1);
    if (gui_color_downgrade_cache_rgb[index] != rgb + 1)
    {
        gui_color_downgrade_cache_rgb[index] = rgb + 1;
        gui_color_downgrade_cache_term[index] =
            gui_color_convert_rgb_to_term_display (rgb, 256);
    }

    return gui_color_downgrade_cache_term[index];
}

/*
 * Add "length" bytes of a string to a dynamic string, with RGB colors
 * ("#rrggbb") replaced by the nearest color of the 256-color palette,
 * written on 5 digits if "padded" is 1 (in color codes), as-is otherwise (in
 * color names).
 */

void
gui_color_downgrade_rgb_concat (char **output, const char *string, int length,
                                int padded)
{
    const char *ptr_string, *ptr_end, *ptr_hash;
    char str_color[16];
    int rgb, size;

    ptr_string = string;
    ptr_end = string + length;
    while (ptr_string < ptr_end)
    {
        rgb = ((ptr_string[0] == '#') && (ptr_end - ptr_string >= 7)) ?
            gui_color_rgb_parse (ptr_string) : -1;
        if (rgb >= 0)
        {
            snprintf (str_color, sizeof (str_color),
                      (padded) ? "%05d" : "%d",
                      gui_color_downgrade_rgb_term (rgb));
            string_dyn_concat (output, str_color, -1);
            ptr_string += 7;
        }
        else
        {
            /* Copy the text until the next "#" (only in this text). */
            ptr_hash = memchr (ptr_string + 1, '#', ptr_end - ptr_string - 1);
            size = (ptr_hash) ? ptr_hash - ptr_string : ptr_end - ptr_string;
            string_dyn_concat (output, ptr_string, size);
            ptr_string += size;
        }
    }
}

/*
 * Replace RGB colors ("#rrggbb") in WeeChat color codes by the nearest color
 * in the 256-color palette ("00123"), for consumers that do not support RGB
 * colors (for example old relay clients).
 *
 * Text outside color codes is not changed.
 *
 * Note: result must be freed after use.
 */

char *
gui_color_downgrade_rgb (const char *string)
{
    const char *ptr_string;
    char **out;
    int size;

    if (!string)
        return NULL;

    out = string_dyn_alloc (strlen (string) + 1);
    if (!out)
        return NULL;

    ptr_string = string;
    while (ptr_string[0])
    {
        size = gui_color_code_size (ptr_string);
        if (size > 0)
        {
            /* Color code, with RGB colors replaced. */
            gui_color_downgrade_rgb_concat (out, ptr_string, size, 1);
            ptr_string += size;
        }
        else
        {
            /* Copy the text until the next color code. */
            size = 1 + strcspn (ptr_string + 1, "\x19\x1A\x1B\x1C");
            string_dyn_concat (out, ptr_string, size);
            ptr_string += size;
        }
    }

    return string_dyn_free (out, 0);
}

/*
 * Replace RGB colors ("#rrggbb") in a color name (for example
 * "*#ff8800:#000080") by the nearest color in the 256-color palette
 * (for example "*208:18"), for consumers that do not support RGB colors.
 *
 * Note: result must be freed after use.
 */

char *
gui_color_downgrade_rgb_name (const char *name)
{
    char **out;

    if (!name)
        return NULL;

    out = string_dyn_alloc (strlen (name) + 1);
    if (!out)
        return NULL;

    gui_color_downgrade_rgb_concat (out, name, strlen (name), 0);

    return string_dyn_free (out, 0);
}

/*
 * Get a RGB component (0-255) from an ANSI color code item: values lower
 * than 0 are replaced by 0 and values greater than 255 are replaced by 255.
 */

int
gui_color_ansi_rgb_component (const char *item)
{
    long value;

    value = strtol (item, NULL, 10);
    if (value < 0)
        return 0;
    if (value > 255)
        return 255;
    return (int)value;
}

/*
 * Get the value of a parameter of an ANSI escape sequence (an empty parameter
 * is 0); sub-parameters after the value (separated by ":") are ignored.
 *
 * Return the value, -1 if the parameter is not a number or is greater than
 * 65535.
 */

int
gui_color_ansi_param_value (const char *param)
{
    char *error;
    long value;

    if (!param || !param[0])
        return 0;

    value = strtol (param, &error, 10);
    if ((error[0] && (error[0] != ':')) || (value < 0) || (value > 65535))
        return -1;

    return (int)value;
}

/*
 * Get a color in arguments of a color of an ANSI escape sequence: type of
 * color and its values: 5 and N (terminal color) or 2 and R, G, B (RGB color).
 *
 * Return the terminal color (0-255), the RGB color with flag
 * GUI_COLOR_ANSI_RGB_FLAG (GUI_COLOR_ANSI_RGB_FLAG | 0xRRGGBB), -1 if the
 * color is invalid, -2 if the color is incomplete or has an unknown type.
 */

int
gui_color_ansi_color (int type, char **values, int num_values)
{
    int color;

    switch (type)
    {
        case 2: /* RGB color */
            if (num_values < 3)
                return -2;
            return GUI_COLOR_ANSI_RGB_FLAG
                | (gui_color_ansi_rgb_component (values[0]) << 16)
                | (gui_color_ansi_rgb_component (values[1]) << 8)
                | gui_color_ansi_rgb_component (values[2]);
        case 5: /* Terminal color (0-255) */
            if (num_values < 1)
                return -2;
            color = gui_color_ansi_param_value (values[0]);
            return ((color >= 0) && (color <= 255)) ? color : -1;
    }

    return -2;
}

/*
 * Parse the color in sub-parameters of a parameter 38 (text color), 48
 * (background color) or 58 (underline color) of an ANSI escape sequence
 * (sub-parameters are separated by ":"): "38:5:N" (terminal color),
 * "38:2:R:G:B" or "38:2:<color space>:R:G:B" (RGB color).
 *
 * Return the color (see function gui_color_ansi_color), -1 if the color is
 * invalid or incomplete.
 */

int
gui_color_ansi_parse_color_sub_params (char **sub_params, int num_sub_params)
{
    int type, first_value, color;

    type = (num_sub_params >= 2) ?
        gui_color_ansi_param_value (sub_params[1]) : -1;
    first_value = ((type == 2) && (num_sub_params >= 6)) ? 3 : 2;
    color = gui_color_ansi_color (type, sub_params + first_value,
                                  num_sub_params - first_value);

    return (color >= 0) ? color : -1;
}

/*
 * Parse the color after a parameter 38 (text color), 48 (background color)
 * or 58 (underline color) of an ANSI escape sequence: "5;N" (terminal color)
 * or "2;R;G;B" (RGB color).
 *
 * On input, *index is the index of parameter 38/48/58 in items; on output, it
 * is the index of the last parameter of the color. If the color is incomplete
 * or has an unknown type, all the remaining parameters are ignored.
 *
 * Return the terminal color (0-255), the RGB color with flag
 * GUI_COLOR_ANSI_RGB_FLAG (GUI_COLOR_ANSI_RGB_FLAG | 0xRRGGBB), -1 if the
 * color is invalid.
 */

int
gui_color_ansi_parse_color (char **items, int num_items, int *index)
{
    int type, color;

    type = (*index + 1 < num_items) ?
        gui_color_ansi_param_value (items[*index + 1]) : -1;
    color = gui_color_ansi_color (type, items + *index + 2,
                                  num_items - *index - 2);
    if (color == -2)
    {
        /* Incomplete color or unknown type: ignore remaining parameters. */
        *index = num_items - 1;
        return -1;
    }
    *index += (type == 2) ? 4 : 2;

    return color;
}

/*
 * Build the name of a color returned by function gui_color_ansi_parse_color,
 * with a prefix (for example "|" or "|,"): terminal color ("|208") or RGB
 * color ("|#ff8800").
 */

void
gui_color_ansi_color_name (char *buffer, int size, const char *prefix,
                           int color)
{
    if (color & GUI_COLOR_ANSI_RGB_FLAG)
    {
        snprintf (buffer, size, "%s#%06x",
                  prefix, color & 0xFFFFFF);
    }
    else
    {
        snprintf (buffer, size, "%s%d", prefix, color);
    }
}

/*
 * Parse an ANSI escape sequence: for each parameter of a SGR sequence
 * ("\33[" + parameters + "m"), call the callback with the value of the
 * parameter and its color (for 38 and 48: terminal color 0-255, or RGB color
 * with flag GUI_COLOR_ANSI_RGB_FLAG; -1 for other parameters), then call it
 * with GUI_COLOR_ANSI_PARAM_END; the callback adds its result in "output".
 *
 * The other escape sequences are removed (empty result).
 *
 * Note: result must be freed after use.
 */

char *
gui_color_ansi_parse_sequence (const char *sequence,
                               t_gui_color_ansi_param_cb *callback)
{
    char *text2, **items, **sub_params, **output;
    int i, length, num_items, num_sub_params, value, color;

    if (!sequence || !sequence[0])
        return strdup ("");

    /*
     * Only SGR sequences are used: "\33[" + parameters + "m", where parameters
     * contain only digits, ";" and ":" (no private marker like "<", "=", ">"
     * or "?" and no intermediate bytes); the others are discarded.
     */
    length = strlen (sequence);
    if ((length < 3)
        || (sequence[1] != '[')
        || (sequence[length - 1] != 'm')
        || ((int)strspn (sequence + 2, "0123456789:;") != length - 3))
    {
        return strdup ("");
    }

    output = string_dyn_alloc (32);
    if (!output)
        return strdup ("");

    /* Sequence "\33[m" resets attributes and colors. */
    if (length < 4)
    {
        (callback) (output, 0, -1);
        (callback) (output, GUI_COLOR_ANSI_PARAM_END, -1);
        return string_dyn_free (output, 0);
    }

    text2 = NULL;
    items = NULL;

    /* Extract text between "\33[" and "m". */
    text2 = string_strndup (sequence + 2, length - 3);
    if (!text2)
        goto end;

    /*
     * Keep empty parameters: an empty parameter is 0 (see function
     * gui_color_ansi_param_value), for example "\33[;1m" is "\33[0;1m".
     */
    items = string_split (text2, ";", NULL, 0, 0, &num_items);
    if (!items)
        goto end;

    for (i = 0; i < num_items; i++)
    {
        /*
         * Sub-parameters (separated by ":"): at most 6 are used, for example
         * "38:2:<color space>:R:G:B".
         */
        sub_params = NULL;
        num_sub_params = 0;
        if (strchr (items[i], ':'))
        {
            sub_params = string_split (items[i], ":", NULL, 0, 7,
                                       &num_sub_params);
            if (!sub_params)
                continue;
        }
        value = gui_color_ansi_param_value (items[i]);
        color = -1;
        if ((value == 38) || (value == 48) || (value == 58))
        {
            color = (sub_params) ?
                gui_color_ansi_parse_color_sub_params (sub_params,
                                                       num_sub_params) :
                gui_color_ansi_parse_color (items, num_items, &i);
            /* Invalid color, or underline color (not supported): ignored. */
            if ((color < 0) || (value == 58))
                value = -1;
        }
        else if ((value == 4) && sub_params && (num_sub_params >= 2)
                 && (gui_color_ansi_param_value (sub_params[1]) == 0))
        {
            /* "4:0" is no underline. */
            value = 24;
        }
        /* Invalid value (not a number or out of range) is ignored. */
        if (value >= 0)
            (callback) (output, value, color);
        string_free_split (sub_params);
    }
    (callback) (output, GUI_COLOR_ANSI_PARAM_END, -1);

end:
    string_free_split (items);
    free (text2);

    return string_dyn_free (output, 0);
}

/*
 * Callback for each parameter of an ANSI escape sequence, to convert ANSI
 * colors to WeeChat colors (see function gui_color_ansi_parse_sequence).
 */

void
gui_color_decode_ansi_param_cb (char **output, int value, int color)
{
    char str_color[128];

    switch (value)
    {
        case 0: /* Reset */
            string_dyn_concat (output, gui_color_get_custom ("reset"), -1);
            break;
        case 1: /* Bold */
            string_dyn_concat (output, gui_color_get_custom ("bold"), -1);
            break;
        case 2: /* Dim */
            string_dyn_concat (output, gui_color_get_custom ("dim"), -1);
            break;
        case 3: /* Italic */
            string_dyn_concat (output, gui_color_get_custom ("italic"), -1);
            break;
        case 4: /* Underline */
            string_dyn_concat (output, gui_color_get_custom ("underline"), -1);
            break;
        case 5: /* Blink */
            string_dyn_concat (output, gui_color_get_custom ("blink"), -1);
            break;
        case 7: /* Reverse */
            string_dyn_concat (output, gui_color_get_custom ("reverse"), -1);
            break;
        case 21: /* Remove bold */
            string_dyn_concat (output, gui_color_get_custom ("-bold"), -1);
            break;
        case 22: /* Normal intensity: remove bold and dim */
            string_dyn_concat (output, gui_color_get_custom ("-bold"), -1);
            string_dyn_concat (output, gui_color_get_custom ("-dim"), -1);
            break;
        case 23: /* Remove italic */
            string_dyn_concat (output, gui_color_get_custom ("-italic"), -1);
            break;
        case 24: /* Remove underline */
            string_dyn_concat (output, gui_color_get_custom ("-underline"), -1);
            break;
        case 25: /* Remove blink */
            string_dyn_concat (output, gui_color_get_custom ("-blink"), -1);
            break;
        case 27: /* Remove reverse */
            string_dyn_concat (output, gui_color_get_custom ("-reverse"), -1);
            break;
        case 30: /* Text color */
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
            string_dyn_concat (output, gui_color_get_custom (str_color), -1);
            break;
        case 38: /* Text color */
        case 48: /* Background color */
            gui_color_ansi_color_name (str_color, sizeof (str_color),
                                       (value == 38) ? "|" : "|,", color);
            string_dyn_concat (output, gui_color_get_custom (str_color), -1);
            break;
        case 39: /* Default text color */
            string_dyn_concat (output, gui_color_get_custom ("|default"), -1);
            break;
        case 40: /* Background color */
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
            string_dyn_concat (output, gui_color_get_custom (str_color), -1);
            break;
        case 49: /* Default background color */
            string_dyn_concat (output, gui_color_get_custom (",default"), -1);
            break;
        case 90: /* Text color (bright) */
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
            string_dyn_concat (output, gui_color_get_custom (str_color), -1);
            break;
        case 100: /* Background color (bright) */
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
            string_dyn_concat (output, gui_color_get_custom (str_color), -1);
            break;
    }
}

/*
 * Callback for each parameter of an ANSI escape sequence, to build the
 * normalized ANSI escape sequence (see function gui_color_normalize_ansi).
 */

void
gui_color_normalize_ansi_param_cb (char **output, int value, int color)
{
    char str_param[64];

    if (value == GUI_COLOR_ANSI_PARAM_END)
    {
        if ((*output)[0])
            string_dyn_concat (output, "m", -1);
        return;
    }

    if ((value == 38) || (value == 48))
    {
        if (color & GUI_COLOR_ANSI_RGB_FLAG)
        {
            snprintf (str_param, sizeof (str_param), "%d;2;%d;%d;%d",
                      value,
                      (color >> 16) & 0xFF,
                      (color >> 8) & 0xFF,
                      color & 0xFF);
        }
        else
        {
            snprintf (str_param, sizeof (str_param), "%d;5;%d", value, color);
        }
    }
    else
    {
        snprintf (str_param, sizeof (str_param), "%d", value);
    }
    string_dyn_concat (output, ((*output)[0]) ? ";" : "\33[", -1);
    string_dyn_concat (output, str_param, -1);
}

/*
 * Get a copy of a string to search ANSI escape sequences in: chars which are
 * not ASCII are replaced by "\x01" (the copy has the same length).
 *
 * Escape sequences contain only ASCII chars and the text between them is
 * copied from the original string, so this does not change the result, but
 * the regex then matches with any locale and any C library, even when the
 * string contains bytes which are not valid UTF-8 (for example a window title
 * in ISO-8859-1).
 *
 * Note: result must be freed after use.
 */

char *
gui_color_ansi_string_ascii (const char *string)
{
    char *result, *ptr_result;

    result = strdup (string);
    if (!result)
        return NULL;

    for (ptr_result = result; ptr_result[0]; ptr_result++)
    {
        if ((unsigned char)ptr_result[0] >= 0x80)
            ptr_result[0] = '\x01';
    }

    return result;
}

/*
 * Search for an ANSI escape sequence in a string of the given length.
 *
 * If possible (flag REG_STARTEND), the length is given to regexec, so that
 * regexec does not compute the length of the string at each call (a string
 * with many escape sequences is decoded in a linear time).
 *
 * Return the value returned by regexec.
 */

int
gui_color_ansi_regexec (regex_t *regex, const char *string, size_t length,
                        regmatch_t *regex_match)
{
#ifdef REG_STARTEND
    regex_match->rm_so = 0;
    regex_match->rm_eo = (regoff_t)length;
    return regexec (regex, string, 1, regex_match, REG_STARTEND);
#else
    /* Make C compiler happy. */
    (void) length;
    return regexec (regex, string, 1, regex_match, 0);
#endif
}

/*
 * Replace ANSI escape sequences in a string: each SGR sequence is replaced by
 * the output of the callback called for each of its parameters (if
 * keep_colors is 1), the other escape sequences are removed.
 *
 * Note: result must be freed after use.
 */

char *
gui_color_ansi_replace (const char *string, int keep_colors,
                        t_gui_color_ansi_param_cb *callback)
{
    char **output, *sequence, *code, *string_ascii;
    const char *ptr_string, *ptr_ascii;
    regmatch_t regex_match;
    size_t length;

    if (!string)
        return NULL;

    /* Allocate/compile regex if needed (first call). */
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

    /* Without ESC, there is no escape sequence to replace. */
    if (!strchr (string, '\33'))
        return strdup (string);

    /*
     * Sequences are searched in a copy with only ASCII chars, if the string
     * has other chars.
     */
    string_ascii = NULL;
    if (utf8_has_8bits (string))
    {
        string_ascii = gui_color_ansi_string_ascii (string);
        if (!string_ascii)
            return NULL;
    }

    length = strlen (string);
    output = string_dyn_alloc (length + 1);
    if (!output)
    {
        free (string_ascii);
        return NULL;
    }

    /* Replace each ANSI escape sequence. */
    ptr_string = string;
    ptr_ascii = (string_ascii) ? string_ascii : string;
    while ((length > 0)
           && (gui_color_ansi_regexec (gui_color_regex_ansi, ptr_ascii,
                                       length, &regex_match) == 0)
           && (regex_match.rm_eo > regex_match.rm_so))
    {
        string_dyn_concat (output, ptr_string, regex_match.rm_so);
        /* Only SGR sequences (ending with "m") are used. */
        if (keep_colors && (ptr_string[regex_match.rm_eo - 1] == 'm'))
        {
            sequence = string_strndup (ptr_string + regex_match.rm_so,
                                       regex_match.rm_eo - regex_match.rm_so);
            if (sequence)
            {
                code = gui_color_ansi_parse_sequence (sequence, callback);
                if (code)
                {
                    string_dyn_concat (output, code, -1);
                    free (code);
                }
                free (sequence);
            }
        }
        ptr_string += regex_match.rm_eo;
        ptr_ascii += regex_match.rm_eo;
        length -= regex_match.rm_eo;
    }
    string_dyn_concat (output, ptr_string, -1);

    free (string_ascii);

    return string_dyn_free (output, 0);
}

/*
 * Replace ANSI colors by WeeChat colors.
 *
 * If keep_colors == 0: removes any color/style in message otherwise keeps
 * colors.
 *
 * Note: result must be freed after use.
 */

char *
gui_color_decode_ansi (const char *string, int keep_colors)
{
    return gui_color_ansi_replace (string, keep_colors,
                                   &gui_color_decode_ansi_param_cb);
}

/*
 * Normalize ANSI escape sequences: each SGR sequence is replaced by a SGR
 * sequence with only the parameters which are used, in a canonical form
 * ("\33[" + parameters separated by ";" + "m"), where each parameter is a
 * number, and colors are "38;5;N", "48;5;N" (terminal color) or
 * "38;2;R;G;B", "48;2;R;G;B" (RGB color, components in range 0-255); a
 * sequence without parameter used is removed, and the other escape sequences
 * are removed.
 *
 * If keep_colors == 0, all escape sequences are removed.
 *
 * Note: result must be freed after use.
 */

char *
gui_color_normalize_ansi (const char *string, int keep_colors)
{
    return gui_color_ansi_replace (string, keep_colors,
                                   &gui_color_normalize_ansi_param_cb);
}

/*
 * Update flags bold/dim enabled in output of gui_color_encode_ansi after a
 * text color with the given attribute flags: without flag "keep attributes",
 * the text color removes all attributes on screen, so only the bold/dim of
 * the color itself are enabled.
 */

void
gui_color_ansi_update_intensity (int *intensity, int flags)
{
    if (flags & GUI_COLOR_EXTENDED_KEEPATTR_FLAG)
        *intensity |= flags & (GUI_COLOR_EXTENDED_BOLD_FLAG
                               | GUI_COLOR_EXTENDED_DIM_FLAG);
    else
        *intensity = flags & (GUI_COLOR_EXTENDED_BOLD_FLAG
                              | GUI_COLOR_EXTENDED_DIM_FLAG);
}

/*
 * Add an ANSI color code with a WeeChat attribute flag.
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
            /* Nothing to do here (really? not sure). */
            break;
    }
}

/*
 * Add ANSI color codes for the attribute flags at the beginning of a WeeChat
 * text color (for example "*" for bold), and update flags bold/dim enabled in
 * output of gui_color_encode_ansi (see gui_color_ansi_update_intensity).
 *
 * Return pointer to the string after the attribute flags.
 */

const unsigned char *
gui_color_add_ansi_flags (char **output, const unsigned char *string,
                          int *intensity)
{
    int flag, flags;

    flags = 0;
    while ((flag = gui_color_attr_get_flag (string[0])) > 0)
    {
        gui_color_add_ansi_flag (output, flag);
        flags |= flag;
        string++;
    }
    gui_color_ansi_update_intensity (intensity, flags);

    return string;
}

/*
 * Convert a WeeChat color number to an ANSI color number.
 *
 * Return -1 if the color is not found.
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

    /* Color not found */
    return -1;
}

/*
 * Build an ANSI color code for a RGB color (if "rgb" >= 0) or a terminal
 * color "term" (0-255), for foreground or background.
 *
 * If "rgb_colors" is 0, a RGB color is replaced by the nearest color of the
 * 256-color palette (otherwise a 24-bit ANSI color code is used).
 */

void
gui_color_encode_ansi_rgb_term (char *buffer, int size, int rgb, int term,
                                int background, int rgb_colors)
{
    if ((rgb >= 0) && rgb_colors)
    {
        snprintf (buffer, size,
                  "\x1B[%d;2;%d;%d;%dm",
                  (background) ? 48 : 38,
                  (rgb >> 16) & 0xFF,
                  (rgb >> 8) & 0xFF,
                  rgb & 0xFF);
    }
    else
    {
        snprintf (buffer, size,
                  "\x1B[%d;5;%dm",
                  (background) ? 48 : 38,
                  (rgb >= 0) ? gui_color_downgrade_rgb_term (rgb) : term);
    }
}

/*
 * Add an ANSI color code for the extended color at the beginning of "string"
 * (string is after the extended char and the attributes): RGB color
 * ("#RRGGBB") or color number ("00123").
 *
 * If "rgb_colors" is 0, a RGB color is replaced by the nearest color of the
 * 256-color palette (otherwise a 24-bit ANSI color code is used).
 *
 * Return the size of color in string.
 */

int
gui_color_encode_ansi_extended (char **output, const char *string,
                                int background, int rgb_colors)
{
    char str_concat[64];
    int size, rgb, color;

    size = gui_color_extended_value (string, &rgb, &color);
    if ((rgb >= 0) || (color >= 0))
    {
        gui_color_encode_ansi_rgb_term (str_concat, sizeof (str_concat),
                                        rgb, color, background, rgb_colors);
        string_dyn_concat (output, str_concat, -1);
    }

    return size;
}

/*
 * Build an ANSI color code for a terminal color (0-255) or a RGB color
 * number (24-bit ANSI color code), for foreground or background.
 *
 * If "rgb_colors" is 0, a RGB color is replaced by the nearest color of the
 * 256-color palette.
 */

void
gui_color_encode_ansi_color (char *buffer, int size, int color,
                             int background, int rgb_colors)
{
    gui_color_encode_ansi_rgb_term (buffer, size,
                                    gui_color_rgb_from_number (color), color,
                                    background, rgb_colors);
}

/*
 * Add ANSI color codes for the attribute flags of a WeeChat color (integer
 * with flags like GUI_COLOR_EXTENDED_BOLD_FLAG), and update flags bold/dim
 * enabled in output of gui_color_encode_ansi.
 */

void
gui_color_add_ansi_attrs (char **output, int color, int *intensity)
{
    int flags[] = { GUI_COLOR_EXTENDED_BLINK_FLAG, GUI_COLOR_EXTENDED_DIM_FLAG,
                    GUI_COLOR_EXTENDED_BOLD_FLAG, GUI_COLOR_EXTENDED_REVERSE_FLAG,
                    GUI_COLOR_EXTENDED_ITALIC_FLAG,
                    GUI_COLOR_EXTENDED_UNDERLINE_FLAG, 0 };
    int i;

    for (i = 0; flags[i]; i++)
    {
        if (color & flags[i])
            gui_color_add_ansi_flag (output, flags[i]);
    }
    gui_color_ansi_update_intensity (intensity, color);
}

/*
 * Add ANSI color code for a WeeChat color, text color or background:
 * extended color (with flag GUI_COLOR_EXTENDED_FLAG, terminal color or RGB
 * color number) or basic WeeChat color; attribute flags in color are ignored.
 *
 * If "rgb_colors" is 0, a RGB color is replaced by the nearest color of the
 * 256-color palette.
 */

void
gui_color_add_ansi_color (char **output, int color, int background,
                          int rgb_colors)
{
    char str_concat[64];
    int ansi_color, base;

    if ((color >= 0) && (color & GUI_COLOR_EXTENDED_FLAG))
    {
        gui_color_encode_ansi_color (str_concat, sizeof (str_concat),
                                     color & GUI_COLOR_EXTENDED_MASK,
                                     background, rgb_colors);
    }
    else
    {
        /* Basic colors: 30-37 or 90-97 (text), 40-47 or 100-107 (background). */
        base = (background) ? 40 : 30;
        ansi_color = gui_color_weechat_to_ansi (
            color & GUI_COLOR_EXTENDED_MASK);
        snprintf (str_concat, sizeof (str_concat),
                  "\x1B[%dm",
                  (ansi_color < 0) ?
                  ((background) ?
                   GUI_COLOR_ANSI_DEFAULT_BG : GUI_COLOR_ANSI_DEFAULT_FG) :
                  ((ansi_color < 8) ?
                   base + ansi_color : base + 60 + ansi_color - 8));
    }
    string_dyn_concat (output, str_concat, -1);
}

/*
 * Add ANSI color code for a WeeChat text color (see function
 * gui_color_add_ansi_color).
 */

void
gui_color_add_ansi_fg (char **output, int color, int rgb_colors)
{
    gui_color_add_ansi_color (output, color, 0, rgb_colors);
}

/*
 * Add ANSI color code for a WeeChat background color (see function
 * gui_color_add_ansi_color).
 */

void
gui_color_add_ansi_bg (char **output, int color, int rgb_colors)
{
    gui_color_add_ansi_color (output, color, 1, rgb_colors);
}

/*
 * Add an ANSI color code for a terminal color (0-255, or -1 for the default
 * color), text color or background.
 */

void
gui_color_ansi_add_term_color (char **output, int color, int background)
{
    char str_concat[32];

    if (color < 0)
    {
        snprintf (str_concat, sizeof (str_concat), "\x1B[%dm",
                  (background) ?
                  GUI_COLOR_ANSI_DEFAULT_BG : GUI_COLOR_ANSI_DEFAULT_FG);
    }
    else
    {
        snprintf (str_concat, sizeof (str_concat), "\x1B[%d;5;%dm",
                  (background) ? 48 : 38, color);
    }
    string_dyn_concat (output, str_concat, -1);
}

/*
 * Replace WeeChat colors by ANSI colors.
 *
 * If "rgb_colors" is 0, RGB colors are replaced by the nearest color of the
 * 256-color palette (otherwise 24-bit ANSI color codes are used).
 *
 * Note: result must be freed after use.
 */

char *
gui_color_encode_ansi_rgb (const char *string, int rgb_colors)
{
    const unsigned char *ptr_string;
    char **out, str_concat[128], str_color[8];
    int color, length, fg, bg, attrs, intensity;

    if (!string)
        return NULL;

    out = string_dyn_alloc (((strlen (string) * 3) / 2) + 1);
    if (!out)
        return NULL;

    /*
     * Flags bold/dim currently enabled in output: ANSI code 22 removes both,
     * so the other one must be enabled again when only one is removed.
     */
    intensity = 0;

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
                            ptr_string = gui_color_add_ansi_flags (
                                out, ptr_string, &intensity);
                            ptr_string += gui_color_encode_ansi_extended (
                                out, (const char *)ptr_string, 0, rgb_colors);
                        }
                        else
                        {
                            ptr_string = gui_color_add_ansi_flags (
                                out, ptr_string, &intensity);
                            if (ptr_string[0] && ptr_string[1])
                            {
                                memcpy (str_color, ptr_string, 2);
                                str_color[2] = '\0';
                                if (util_parse_int (str_color, 10, &color))
                                    gui_color_add_ansi_fg (out, color,
                                                           rgb_colors);
                                ptr_string += 2;
                            }
                        }
                        break;
                    case GUI_COLOR_BG_CHAR:
                        ptr_string++;
                        if (ptr_string[0] == GUI_COLOR_EXTENDED_CHAR)
                        {
                            ptr_string++;
                            ptr_string += gui_color_encode_ansi_extended (
                                out, (const char *)ptr_string, 1, rgb_colors);
                        }
                        else
                        {
                            if (ptr_string[0] && ptr_string[1])
                            {
                                memcpy (str_color, ptr_string, 2);
                                str_color[2] = '\0';
                                if (util_parse_int (str_color, 10, &color))
                                    gui_color_add_ansi_bg (out, color,
                                                           rgb_colors);
                                ptr_string += 2;
                            }
                        }
                        break;
                    case GUI_COLOR_FG_BG_CHAR:
                        ptr_string++;
                        if (ptr_string[0] == GUI_COLOR_EXTENDED_CHAR)
                        {
                            ptr_string++;
                            ptr_string = gui_color_add_ansi_flags (
                                out, ptr_string, &intensity);
                            ptr_string += gui_color_encode_ansi_extended (
                                out, (const char *)ptr_string, 0, rgb_colors);
                        }
                        else
                        {
                            ptr_string = gui_color_add_ansi_flags (
                                out, ptr_string, &intensity);
                            if (ptr_string[0] && ptr_string[1])
                            {
                                memcpy (str_color, ptr_string, 2);
                                str_color[2] = '\0';
                                if (util_parse_int (str_color, 10, &color))
                                    gui_color_add_ansi_fg (out, color,
                                                           rgb_colors);
                                ptr_string += 2;
                            }
                        }
                        /*
                         * Note: the comma is an old separator not used any
                         * more (since WeeChat 2.6), but we still use it here
                         * so in case of/upgrade this will not break colors in
                         * old messages.
                         */
                        if ((ptr_string[0] == ',') || (ptr_string[0] == '~'))
                        {
                            if (ptr_string[1] == GUI_COLOR_EXTENDED_CHAR)
                            {
                                ptr_string += 2;
                                ptr_string += gui_color_encode_ansi_extended (
                                    out, (const char *)ptr_string, 1,
                                    rgb_colors);
                            }
                            else
                            {
                                ptr_string++;
                                if (ptr_string[0] && ptr_string[1])
                                {
                                    memcpy (str_color, ptr_string, 2);
                                    str_color[2] = '\0';
                                    if (util_parse_int (str_color, 10, &color))
                                        gui_color_add_ansi_bg (out, color,
                                                               rgb_colors);
                                    ptr_string += 2;
                                }
                            }
                        }
                        break;
                    case GUI_COLOR_EXTENDED_CHAR:
                        ptr_string++;
                        /* Color pair: attributes are removed on screen. */
                        intensity = 0;
                        if ((isdigit (ptr_string[0])) && (isdigit (ptr_string[1]))
                            && (isdigit (ptr_string[2])) && (isdigit (ptr_string[3]))
                            && (isdigit (ptr_string[4])))
                        {
                            memcpy (str_color, ptr_string, 5);
                            str_color[5] = '\0';
                            /*
                             * Colors of the pair (the pair number is not a
                             * color); unknown pair: no color.
                             */
                            if (util_parse_int (str_color, 10, &color)
                                && gui_color_get_pair_colors (color, &fg, &bg))
                            {
                                gui_color_ansi_add_term_color (out, fg, 0);
                                gui_color_ansi_add_term_color (out, bg, 1);
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
                                snprintf (str_concat, sizeof (str_concat),
                                          "\x1B[%dm",
                                          GUI_COLOR_ANSI_DEFAULT_FG);
                                string_dyn_concat (out, str_concat, -1);
                                ptr_string++;
                                break;
                            case GUI_COLOR_BAR_BG_CHAR:
                                snprintf (str_concat, sizeof (str_concat),
                                          "\x1B[%dm",
                                          GUI_COLOR_ANSI_DEFAULT_BG);
                                string_dyn_concat (out, str_concat, -1);
                                ptr_string++;
                                break;
                            case GUI_COLOR_BAR_DELIM_CHAR:
                                /*
                                 * The delimiter is a text color, with the
                                 * attributes of its color.
                                 */
                                color = CONFIG_COLOR(config_color_chat_delimiters);
                                gui_color_add_ansi_attrs (out, color,
                                                          &intensity);
                                gui_color_add_ansi_fg (out, color, rgb_colors);
                                ptr_string++;
                                break;
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
                            if (util_parse_int (str_color, 10, &color)
                                && (color >= 0)
                                && (color < GUI_COLOR_NUM_COLORS))
                            {
                                fg = gui_color[color]->foreground;
                                bg = gui_color[color]->background;
                                attrs = gui_color_get_extended_flags (
                                    gui_color[color]->attributes);
                                /*
                                 * Light color displayed with bold: use the
                                 * light color (bold is not an attribute of
                                 * the color).
                                 */
                                if (gui_color[color]->bold_light
                                    && (fg >= 0) && (fg < 8))
                                {
                                    fg += 8;
                                }
                                string_dyn_concat (out, "\x1B[0m", -1);
                                intensity = 0;
                                gui_color_add_ansi_attrs (out, attrs, &intensity);
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
                                    gui_color_encode_ansi_color (
                                        str_concat, sizeof (str_concat), fg, 0,
                                        rgb_colors);
                                    string_dyn_concat (out, str_concat, -1);
                                }
                                if (bg < 0)
                                {
                                    string_dyn_concat (out, "\x1B[49m", -1);
                                }
                                else
                                {
                                    gui_color_encode_ansi_color (
                                        str_concat, sizeof (str_concat), bg, 1,
                                        rgb_colors);
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
                            intensity |= GUI_COLOR_EXTENDED_DIM_FLAG;
                            break;
                        case GUI_COLOR_ATTR_BOLD_CHAR:
                            string_dyn_concat (out, "\x1B[1m", -1);
                            intensity |= GUI_COLOR_EXTENDED_BOLD_FLAG;
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
                            intensity &= ~GUI_COLOR_EXTENDED_DIM_FLAG;
                            if (intensity & GUI_COLOR_EXTENDED_BOLD_FLAG)
                                string_dyn_concat (out, "\x1B[1m", -1);
                            break;
                        case GUI_COLOR_ATTR_BOLD_CHAR:
                            string_dyn_concat (out, "\x1B[22m", -1);
                            intensity &= ~GUI_COLOR_EXTENDED_BOLD_FLAG;
                            if (intensity & GUI_COLOR_EXTENDED_DIM_FLAG)
                                string_dyn_concat (out, "\x1B[2m", -1);
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
                intensity = 0;
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
 * Replace WeeChat colors by ANSI colors (with 24-bit ANSI color codes for
 * RGB colors).
 *
 * Note: result must be freed after use.
 */

char *
gui_color_encode_ansi (const char *string)
{
    return gui_color_encode_ansi_rgb (string, 1);
}

/*
 * Emphasize a string or regular expression in a string (which can contain
 * colors).
 *
 * Argument "case_sensitive" is used only for "search", if the "regex" is NULL.
 *
 * Return string with the search string/regex emphasized, NULL if error.
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
    int pos1, pos2, real_pos1, real_pos2, count_emphasis, offset;

    /* String is required. */
    if (!string)
        return NULL;

    /* Search or regex is required. */
    if (!search && !regex)
        return NULL;

    color_emphasis = gui_color_get_custom ("emphasis");
    if (!color_emphasis)
        return NULL;
    length_emphasis = strlen (color_emphasis);

    /*
     * Allocate space for 8 emphasized strings (8 before + 8 after = 16);
     * more space will be allocated later (if there are more than 8 emphasized
     * strings).
     */
    length_result = strlen (string) + (length_emphasis * 2 * 8) + 1;
    result = malloc (length_result);
    if (!result)
        return NULL;
    result[0] = '\0';

    /*
     * Build a string without color codes to search in this one (then with the
     * position of text found, we will retrieve position in original string,
     * which can contain color codes).
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
            /* Search next match using the regex. */
            offset = ptr_no_color - string_no_color;
            rc = string_regexec_offset (regex, string_no_color, offset,
                                        1, &regex_match);

            /*
             * No match found: exit the loop (if rm_eo == offset, it is an
             * empty match at the search position: we consider there is no
             * match, to prevent an infinite loop).
             */
            if ((rc != 0) || (regex_match.rm_so < 0)
                || (regex_match.rm_eo <= offset))
            {
                strcat (result, ptr_string);
                break;
            }
            pos1 = regex_match.rm_so - offset;
            pos2 = regex_match.rm_eo - offset;
        }
        else
        {
            /* Search next match in the string. */
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
         * Find the position of match in the original string (which can contain
         * color codes).
         */
        real_pos1 = gui_chat_string_real_pos (ptr_string,
                                              gui_chat_string_pos (ptr_no_color, pos1),
                                              0);
        real_pos2 = gui_chat_string_real_pos (ptr_string,
                                              gui_chat_string_pos (ptr_no_color, pos2),
                                              0);

        /*
         * Concatenate following strings to the result:
         * - beginning of string (before match)
         * - emphasis color code
         * - the matching string
         * - emphasis color code.
         */
        if (real_pos1 > 0)
            strncat (result, ptr_string, real_pos1);
        strcat (result, color_emphasis);
        strncat (result, ptr_string + real_pos1, real_pos2 - real_pos1);
        strcat (result, color_emphasis);

        /* Restart next loop after the matching string. */
        ptr_string += real_pos2;
        ptr_no_color += pos2;

        /* Check if we should allocate more space for emphasis color codes. */
        count_emphasis++;
        if (count_emphasis == 8)
        {
            /* Allocate more space for emphasis color codes. */
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
 * Free a color.
 */

void
gui_color_free (struct t_gui_color *color)
{
    if (!color)
        return;

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

    /* Make C compiler happy. */
    (void) hashtable;
    (void) key;

    color_palette = (struct t_gui_color_palette *)value;
    gui_color_palette_free (color_palette);
}

/*
 * Allocate hashtables and lists for palette.
 */

void
gui_color_palette_alloc_structs (void)
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
 * Get color pair number with alias.
 *
 * Return -1 if alias is not found.
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

    /* Alias not found */
    return -1;
}

/*
 * Get a color palette with number.
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
 * Add a color in palette.
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
 * Remove a color in palette.
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
 * Free hashtables and lists for palette.
 */

void
gui_color_palette_free_structs (void)
{
    hashtable_free (gui_color_hash_palette_color);
    hashtable_free (gui_color_hash_palette_alias);
    weelist_free (gui_color_list_with_alias);
}

/*
 * Initialize colors.
 */

void
gui_color_init (void)
{
    int i;

    for (i = 0; i < GUI_COLOR_NUM_COLORS; i++)
    {
        gui_color[i] = NULL;
    }
}

/*
 * End GUI colors.
 */

void
gui_color_end (void)
{
    int i;

    for (i = 0; i < GUI_COLOR_NUM_COLORS; i++)
    {
        gui_color_free (gui_color[i]);
        gui_color[i] = NULL;
    }
    gui_color_palette_free_structs ();
    gui_color_free_vars ();

    gui_color_rgb_table_free (&gui_color_rgb_options);
    gui_color_rgb_table_free (&gui_color_rgb_display);

    if (gui_color_regex_ansi)
    {
        regfree (gui_color_regex_ansi);
        free (gui_color_regex_ansi);
        gui_color_regex_ansi = NULL;
    }
}
