/*
 * gui-nick.c - nick functions (used by all GUI)
 *
 * Copyright (C) 2003-2017 Sébastien Helleu <flashcode@flashtux.org>
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
#include <string.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hashtable.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "gui-nick.h"
#include "gui-color.h"


/*
 * Hashes a nickname to find color.
 *
 * Returns a number which is the index of color in the nicks colors of option
 * "weechat.color.chat_nick_colors".
 */

int
gui_nick_hash_color (const char *nickname)
{
    unsigned long color;
    const char *ptr_nick;

    if (!nickname || !nickname[0])
        return 0;

    if (!config_nick_colors)
        config_set_nick_colors ();

    if (config_num_nick_colors == 0)
        return 0;

    ptr_nick = nickname;
    color = 0;

    switch (CONFIG_INTEGER(config_look_nick_color_hash))
    {
        case CONFIG_LOOK_NICK_COLOR_HASH_DJB2:
            /* variant of djb2 hash */
            color = 5381;
            while (ptr_nick && ptr_nick[0])
            {
                color ^= (color << 5) + (color >> 2) + utf8_char_int (ptr_nick);
                ptr_nick = utf8_next_char (ptr_nick);
            }
            break;
        case CONFIG_LOOK_NICK_COLOR_HASH_SUM:
            /* sum of letters */
            color = 0;
            while (ptr_nick && ptr_nick[0])
            {
                color += utf8_char_int (ptr_nick);
                ptr_nick = utf8_next_char (ptr_nick);
            }
            break;
    }

    return (color % config_num_nick_colors);
}

/*
 * Gets forced color for a nick.
 *
 * Returns the name of color (for example: "green"), NULL if no color is forced
 * for nick.
 */

const char *
gui_nick_get_forced_color (const char *nickname)
{
    const char *forced_color;
    char *nick_lower;

    if (!nickname || !nickname[0])
        return NULL;

    forced_color = hashtable_get (config_hashtable_nick_color_force, nickname);
    if (forced_color)
        return forced_color;

    nick_lower = strdup (nickname);
    if (nick_lower)
    {
        string_tolower (nick_lower);
        forced_color = hashtable_get (config_hashtable_nick_color_force,
                                      nick_lower);
        free (nick_lower);
    }

    return forced_color;
}

/*
 * Duplicates a nick and stops at first char in list (using option
 * weechat.look.nick_color_stop_chars).
 *
 * Note: result must be freed after use.
 */

char *
gui_nick_strdup_for_color (const char *nickname)
{
    int char_size, other_char_seen;
    char *result, *pos, utf_char[16];

    if (!nickname)
        return NULL;

    result = malloc (strlen (nickname) + 1);
    pos = result;
    other_char_seen = 0;
    while (nickname[0])
    {
        char_size = utf8_char_size (nickname);
        memcpy (utf_char, nickname, char_size);
        utf_char[char_size] = '\0';

        if (strstr (CONFIG_STRING(config_look_nick_color_stop_chars),
                    utf_char))
        {
            if (other_char_seen)
            {
                pos[0] = '\0';
                return result;
            }
        }
        else
        {
            other_char_seen = 1;
        }
        memcpy (pos, utf_char, char_size);
        pos += char_size;

        nickname += char_size;
    }
    pos[0] = '\0';
    return result;
}

/*
 * Finds a color code for a nick (according to nick letters).
 *
 * Returns a WeeChat color code (that can be used for display).
 */

const char *
gui_nick_find_color (const char *nickname)
{
    int color;
    char *nickname2;
    const char *forced_color, *str_color;

    if (!nickname || !nickname[0])
        return gui_color_get_custom ("default");

    if (!config_nick_colors)
        config_set_nick_colors ();

    if (config_num_nick_colors == 0)
        return gui_color_get_custom ("default");

    nickname2 = gui_nick_strdup_for_color (nickname);

    /* look if color is forced */
    forced_color = gui_nick_get_forced_color (
        (nickname2) ? nickname2 : nickname);
    if (forced_color)
    {
        forced_color = gui_color_get_custom (forced_color);
        if (forced_color && forced_color[0])
        {
            if (nickname2)
                free (nickname2);
            return forced_color;
        }
    }

    /* hash nickname to get color */
    color = gui_nick_hash_color ((nickname2) ? nickname2 : nickname);

    if (nickname2)
        free (nickname2);

    /* return color */
    str_color = gui_color_get_custom (config_nick_colors[color]);
    return (str_color[0]) ? str_color : gui_color_get_custom ("default");
}

/*
 * Finds a color name for a nick (according to nick letters).
 *
 * Returns the name of a color (for example: "green").
 */

const char *
gui_nick_find_color_name (const char *nickname)
{
    int color;
    char *nickname2;
    const char *forced_color;
    static char *default_color = "default";

    if (!nickname || !nickname[0])
        return default_color;

    if (!config_nick_colors)
        config_set_nick_colors ();

    if (config_num_nick_colors == 0)
        return default_color;

    nickname2 = gui_nick_strdup_for_color (nickname);

    /* look if color is forced */
    forced_color = gui_nick_get_forced_color (
        (nickname2) ? nickname2 : nickname);
    if (forced_color)
    {
        if (nickname2)
            free (nickname2);
        return forced_color;
    }

    /* hash nickname to get color */
    color = gui_nick_hash_color ((nickname2) ? nickname2 : nickname);

    if (nickname2)
        free (nickname2);

    /* return color name */
    return config_nick_colors[color];
}
