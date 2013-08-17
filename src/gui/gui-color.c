/*
 * gui-color.c - color functions (used by all GUI)
 *
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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


/*
 * Searches for a color with configuration option name.
 *
 * Returns color string, NULL if not found.
 */

const char *
gui_color_search_config (const char *color_name)
{
    struct t_config_option *ptr_option;

    if (color_name)
    {
        for (ptr_option = weechat_config_section_color->options;
             ptr_option; ptr_option = ptr_option->next_option)
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
    static char color[32][32];
    static int index_color = 0;
    char color_fg[32], color_bg[32];
    char *pos_delim, *str_fg, *pos_bg, *error, *color_attr;
    const char *ptr_color_name;

    /* attribute or other color name (GUI dependent) */
    index_color = (index_color + 1) % 32;
    color[index_color][0] = '\0';

    if (!color_name || !color_name[0])
        return color[index_color];

    if (string_strcasecmp (color_name, "reset") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c",
                  GUI_COLOR_RESET_CHAR);
    }
    else if (string_strcasecmp (color_name, "resetcolor") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_COLOR_CHAR,
                  GUI_COLOR_RESET_CHAR);
    }
    else if (string_strcasecmp (color_name, "emphasis") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_COLOR_CHAR,
                  GUI_COLOR_EMPHASIS_CHAR);
    }
    else if (string_strcasecmp (color_name, "bold") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_SET_ATTR_CHAR,
                  GUI_COLOR_ATTR_BOLD_CHAR);
    }
    else if (string_strcasecmp (color_name, "-bold") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_REMOVE_ATTR_CHAR,
                  GUI_COLOR_ATTR_BOLD_CHAR);
    }
    else if (string_strcasecmp (color_name, "reverse") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_SET_ATTR_CHAR,
                  GUI_COLOR_ATTR_REVERSE_CHAR);
    }
    else if (string_strcasecmp (color_name, "-reverse") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_REMOVE_ATTR_CHAR,
                  GUI_COLOR_ATTR_REVERSE_CHAR);
    }
    else if (string_strcasecmp (color_name, "italic") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_SET_ATTR_CHAR,
                  GUI_COLOR_ATTR_ITALIC_CHAR);
    }
    else if (string_strcasecmp (color_name, "-italic") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_REMOVE_ATTR_CHAR,
                  GUI_COLOR_ATTR_ITALIC_CHAR);
    }
    else if (string_strcasecmp (color_name, "underline") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_SET_ATTR_CHAR,
                  GUI_COLOR_ATTR_UNDERLINE_CHAR);
    }
    else if (string_strcasecmp (color_name, "-underline") == 0)
    {
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%c%c",
                  GUI_COLOR_REMOVE_ATTR_CHAR,
                  GUI_COLOR_ATTR_UNDERLINE_CHAR);
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
        fg_term = -1;
        bg_term = -1;
        fg = -1;
        bg = -1;
        color_attr = NULL;
        color_fg[0] = '\0';
        color_bg[0] = '\0';

        /* read extra attributes (bold, ..) */
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
            pos_bg = pos_delim + 1;
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
            snprintf (color[index_color], sizeof (color[index_color]),
                      "%c%c%s,%s",
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

        if (color_attr)
            free (color_attr);
        if (str_fg)
            free (str_fg);
    }

    return color[index_color];
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
                        if (ptr_string[0] == ',')
                        {
                            if (ptr_string[1] == GUI_COLOR_EXTENDED_CHAR)
                            {
                                if (ptr_string[2] && ptr_string[3]
                                    && ptr_string[4] && ptr_string[5]
                                    && ptr_string[6])
                                {
                                    ptr_string += 7;
                                }
                            }
                            else
                            {
                                if (ptr_string[1] && ptr_string[2])
                                    ptr_string += 3;
                            }
                        }
                        break;
                    case GUI_COLOR_EXTENDED_CHAR:
                        if ((isdigit (ptr_string[1])) && (isdigit (ptr_string[2]))
                            && (isdigit (ptr_string[3])) && (isdigit (ptr_string[4]))
                            && (isdigit (ptr_string[5])))
                            ptr_string += 6;
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
    char *result, *result2, *string_no_color, *pos;
    const char *ptr_string, *ptr_no_color, *color_emphasis;
    int rc, length_search, length_emphasis, length_result;
    int pos1, pos2, real_pos1, real_pos2, count_emphasis;

    if (!string && !regex)
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
                                              gui_chat_string_pos (ptr_no_color, pos1));
        real_pos2 = gui_chat_string_real_pos (ptr_string,
                                              gui_chat_string_pos (ptr_no_color, pos2));

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
    if (color)
    {
        if (color->string)
            free (color->string);

        free (color);
    }
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
        gui_color_hash_palette_color = hashtable_new (32,
                                                      WEECHAT_HASHTABLE_STRING,
                                                      WEECHAT_HASHTABLE_POINTER,
                                                      NULL,
                                                      NULL);
        gui_color_hash_palette_color->callback_free_value = &gui_color_palette_free_value_cb;
    }
    if (!gui_color_hash_palette_alias)
    {
        gui_color_hash_palette_alias = hashtable_new (32,
                                                      WEECHAT_HASHTABLE_STRING,
                                                      WEECHAT_HASHTABLE_INTEGER,
                                                      NULL,
                                                      NULL);
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
