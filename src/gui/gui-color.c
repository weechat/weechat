/*
 * Copyright (C) 2003-2010 Sebastien Helleu <flashcode@flashtux.org>
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
 * gui-color.c: color functions (used by all GUI)
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

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "gui-color.h"


struct t_gui_color *gui_color[GUI_COLOR_NUM_COLORS]; /* GUI colors          */


/*
 * gui_color_search_config: search a color with configuration option name
 *                          return color string, NULL if not found
 */

const char *
gui_color_search_config (const char *color_name)
{
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    
    if (color_name)
    {
        ptr_section = config_file_search_section (weechat_config_file,
                                                  "color");
        if (ptr_section)
        {
            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                if (string_strcasecmp (ptr_option->name, color_name) == 0)
                {
                    if (ptr_option->min < 0)
                    {
                        return gui_color_get_custom (
                            gui_color_get_name (CONFIG_COLOR(ptr_option)));
                    }
                    return GUI_COLOR(ptr_option->min);
                }
            }
        }
    }
    
    /* color not found */
    return NULL;
}

/*
 * gui_color_get_custom: get a custom color with a name (GUI dependent)
 */

const char *
gui_color_get_custom (const char *color_name)
{
    int fg, bg, pair;
    static char color[32][16];
    static int index_color = 0;
    char *pos_comma, *str_fg, *pos_bg, *error;
    
    /* attribute or other color name (GUI dependent) */
    index_color = (index_color + 1) % 32;
    color[index_color][0] = '\0';
    
    if (!color_name || !color_name[0])
        return color[index_color];
    
    if (string_strcasecmp (color_name, "reset") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%s",
                  GUI_COLOR_RESET_STR);
    }
    else if (string_strcasecmp (color_name, "bold") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%s%s",
                  GUI_COLOR_SET_WEECHAT_STR,
                  GUI_COLOR_ATTR_BOLD_STR);
    }
    else if (string_strcasecmp (color_name, "-bold") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%s%s",
                  GUI_COLOR_REMOVE_WEECHAT_STR,
                  GUI_COLOR_ATTR_BOLD_STR);
    }
    else if (string_strcasecmp (color_name, "reverse") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%s%s",
                  GUI_COLOR_SET_WEECHAT_STR,
                  GUI_COLOR_ATTR_REVERSE_STR);
    }
    else if (string_strcasecmp (color_name, "-reverse") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%s%s",
                  GUI_COLOR_REMOVE_WEECHAT_STR,
                  GUI_COLOR_ATTR_REVERSE_STR);
    }
    else if (string_strcasecmp (color_name, "italic") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%s%s",
                  GUI_COLOR_SET_WEECHAT_STR,
                  GUI_COLOR_ATTR_ITALIC_STR);
    }
    else if (string_strcasecmp (color_name, "-italic") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%s%s",
                  GUI_COLOR_REMOVE_WEECHAT_STR,
                  GUI_COLOR_ATTR_ITALIC_STR);
    }
    else if (string_strcasecmp (color_name, "underline") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%s%s",
                  GUI_COLOR_SET_WEECHAT_STR,
                  GUI_COLOR_ATTR_UNDERLINE_STR);
    }
    else if (string_strcasecmp (color_name, "-underline") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%s%s",
                  GUI_COLOR_REMOVE_WEECHAT_STR,
                  GUI_COLOR_ATTR_UNDERLINE_STR);
    }
    else if (string_strcasecmp (color_name, "bar_fg") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c%c",
                  GUI_COLOR_COLOR_CHAR,
                  GUI_COLOR_BAR_CHAR,
                  GUI_COLOR_BAR_FG_CHAR);
    }
    else if (string_strcasecmp (color_name, "bar_delim") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c%c",
                  GUI_COLOR_COLOR_CHAR,
                  GUI_COLOR_BAR_CHAR,
                  GUI_COLOR_BAR_DELIM_CHAR);
    }
    else if (string_strcasecmp (color_name, "bar_bg") == 0)
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
        error = NULL;
        pair = (int)strtol (color_name, &error, 10);
        if (error && !error[0])
        {
            snprintf (color[index_color], sizeof (color[index_color]),
                      "%s%s%05d",
                      GUI_COLOR_COLOR_STR,
                      GUI_COLOR_PAIR_STR,
                      pair);
        }
        else
        {
            pos_comma = strchr (color_name, ',');
            if (pos_comma)
            {
                if (pos_comma == color_name)
                    str_fg = NULL;
                else
                    str_fg = string_strndup (color_name, pos_comma - color_name);
                pos_bg = pos_comma + 1;
            }
            else
            {
                str_fg = strdup (color_name);
                pos_bg = NULL;
            }
            
            if (str_fg && pos_bg)
            {
                fg = gui_color_search (str_fg);
                bg = gui_color_search (pos_bg);
                if ((fg >= 0) && (bg >= 0))
                {
                    snprintf (color[index_color], sizeof (color[index_color]),
                              "%s%s%02d,%02d",
                              GUI_COLOR_COLOR_STR,
                              GUI_COLOR_FG_BG_STR,
                              fg, bg);
                }
            }
            else if (str_fg && !pos_bg)
            {
                fg = gui_color_search (str_fg);
                if (fg >= 0)
                {
                    snprintf (color[index_color], sizeof (color[index_color]),
                              "%s%s%02d",
                              GUI_COLOR_COLOR_STR,
                              GUI_COLOR_FG_STR,
                              fg);
                }
            }
            else if (!str_fg && pos_bg)
            {
                bg = gui_color_search (pos_bg);
                if (bg >= 0)
                {
                    snprintf (color[index_color], sizeof (color[index_color]),
                              "%s%s%02d",
                              GUI_COLOR_COLOR_STR,
                              GUI_COLOR_BG_STR,
                              bg);
                }
            }
            
            if (str_fg)
                free (str_fg);
        }
    }
    
    return color[index_color];
}

/*
 * gui_color_decode: parses a message and remove WeeChat color codes
 *                   if replacement is not NULL and not empty, it is used to
 *                      replace color codes by first char of replacement (and
 *                      next chars in string are NOT removed)
 *                   if replacement is NULL or empty, color codes are removed,
 *                      with following chars if they are related to color code
 *                   After use, string returned has to be free()
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
                    case GUI_COLOR_BG_CHAR:
                        if (ptr_string[1] && ptr_string[2])
                            ptr_string += 3;
                        break;
                    case GUI_COLOR_FG_BG_CHAR:
                        if (ptr_string[1] && ptr_string[2] && (ptr_string[3] == ',')
                            && ptr_string[4] && ptr_string[5])
                            ptr_string += 6;
                        break;
                    case GUI_COLOR_PAIR_CHAR:
                        if ((isdigit (string[1])) && (isdigit (string[2]))
                            && (isdigit (string[3])) && (isdigit (string[4]))
                            && (isdigit (string[5])))
                            ptr_string += 6;
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
                                ptr_string++;
                                break;
                        }
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
            case GUI_COLOR_SET_WEECHAT_CHAR:
            case GUI_COLOR_REMOVE_WEECHAT_CHAR:
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
 * gui_color_string_replace_colors: replace colors in string with color codes
 *                                  colors are using format: ${name} where name
 *                                  is a color name
 */

char *
gui_color_string_replace_colors (const char *string)
{
    int length, length_color, index_string, index_result;
    char *result, *color_name;
    const char *pos_end_name, *ptr_color;
    
    if (!string)
        return NULL;
    
    length = strlen (string) + 1;
    result = malloc (length);
    if (result)
    {
        index_string = 0;
        index_result = 0;
        while (string[index_string])
        {
            if ((string[index_string] == '\\')
                && (string[index_string + 1] == '$'))
            {
                index_string++;
                result[index_result++] = string[index_string++];
            }
            else if ((string[index_string] == '$')
                     && (string[index_string + 1] == '{'))
            {
                pos_end_name = strchr (string + index_string + 2, '}');
                if (pos_end_name)
                {
                    color_name = string_strndup (string + index_string + 2,
                                                 pos_end_name - (string + index_string + 2));
                    if (color_name)
                    {
                        ptr_color = gui_color_get_custom (color_name);
                        if (ptr_color)
                        {
                            length_color = strlen (ptr_color);
                            length += length_color;
                            result = realloc (result, length);
                            if (!result)
                            {
                                free (color_name);
                                return NULL;
                            }
                            strcpy (result + index_result, ptr_color);
                            index_result += length_color;
                            index_string += pos_end_name - string -
                                index_string + 1;
                        }
                        else
                            result[index_result++] = string[index_string++];
                        
                        free (color_name);
                    }
                    else
                        result[index_result++] = string[index_string++];
                }
                else
                    result[index_result++] = string[index_string++];
            }
            else
                result[index_result++] = string[index_string++];
        }
        result[index_result] = '\0';
    }
    
    return result;
}

/*
 * gui_color_free: free a color
 */

void
gui_color_free (struct t_gui_color *color)
{
    if (color)
    {
        if (color->string)
            free (color->string);
        
        free (color);
    }
}
