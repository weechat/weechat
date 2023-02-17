/*
 * gui-nick.c - nick functions (used by all GUI)
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
#include <stdint.h>
#include <string.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hashtable.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "gui-nick.h"
#include "gui-color.h"


/*
 * Hashes a string with a variant of djb2 hash, using 64-bit integer.
 *
 * Number pointed by *color_64 is updated by the function.
 */

void
gui_nick_hash_djb2_64 (const char *nickname, uint64_t *color_64)
{
    while (nickname && nickname[0])
    {
        *color_64 ^= (*color_64 << 5) + (*color_64 >> 2) +
            utf8_char_int (nickname);
        nickname = utf8_next_char (nickname);
    }
}

/*
 * Hashes a string with a variant of djb2 hash, using 32-bit integer.
 *
 * Number pointed by *color_32 is updated by the function.
 */

void
gui_nick_hash_djb2_32 (const char *nickname, uint32_t *color_32)
{
    while (nickname && nickname[0])
    {
        *color_32 ^= (*color_32 << 5) + (*color_32 >> 2) +
            utf8_char_int (nickname);
        nickname = utf8_next_char (nickname);
    }
}

/*
 * Hashes a string with sum of letters, using 64-bit integer.
 *
 * Number pointed by *color_64 is updated by the function.
 */

void
gui_nick_hash_sum_64 (const char *nickname, uint64_t *color_64)
{
    while (nickname && nickname[0])
    {
        *color_64 += utf8_char_int (nickname);
        nickname = utf8_next_char (nickname);
    }
}

/*
 * Hashes a string with sum of letters, using 32-bit integer.
 *
 * Number pointed by *color_32 is updated by the function.
 */

void
gui_nick_hash_sum_32 (const char *nickname, uint32_t *color_32)
{
    while (nickname && nickname[0])
    {
        *color_32 += utf8_char_int (nickname);
        nickname = utf8_next_char (nickname);
    }
}

/*
 * Hashes a nickname to find color.
 *
 * Returns a number which is between 0 and num_colors - 1 (inclusive).
 *
 * num_colors is commonly the number of colors in the option
 * "weechat.color.chat_nick_colors".
 * If num_colors is < 0, the hash itself is returned (64-bit unsigned number).
 */

uint64_t
gui_nick_hash_color (const char *nickname, int num_colors)
{
    const char *ptr_salt;
    uint64_t color_64;
    uint32_t color_32;

    if (!nickname || !nickname[0])
        return 0;

    if (num_colors == 0)
        return 0;

    ptr_salt = CONFIG_STRING(config_look_nick_color_hash_salt);

    color_64 = 0;

    switch (CONFIG_INTEGER(config_look_nick_color_hash))
    {
        case CONFIG_LOOK_NICK_COLOR_HASH_DJB2:
            /* variant of djb2 hash, using 64-bit integer */
            color_64 = 5381;
            gui_nick_hash_djb2_64 (ptr_salt, &color_64);
            gui_nick_hash_djb2_64 (nickname, &color_64);
            break;
        case CONFIG_LOOK_NICK_COLOR_HASH_SUM:
            /* sum of letters, using 64-bit integer  */
            color_64 = 0;
            gui_nick_hash_sum_64 (ptr_salt, &color_64);
            gui_nick_hash_sum_64 (nickname, &color_64);
            break;
        case CONFIG_LOOK_NICK_COLOR_HASH_DJB2_32:
            /* variant of djb2 hash, using 32-bit integer */
            color_32 = 5381;
            gui_nick_hash_djb2_32 (ptr_salt, &color_32);
            gui_nick_hash_djb2_32 (nickname, &color_32);
            color_64 = color_32;
            break;
        case CONFIG_LOOK_NICK_COLOR_HASH_SUM_32:
            /* sum of letters, using 32-bit integer */
            color_32 = 0;
            gui_nick_hash_sum_32 (ptr_salt, &color_32);
            gui_nick_hash_sum_32 (nickname, &color_32);
            color_64 = color_32;
            break;
    }

    return (num_colors > 0) ? color_64 % num_colors : color_64;
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

    nick_lower = string_tolower (nickname);
    if (nick_lower)
    {
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
 * Finds a color name for a nick (according to nick letters).
 *
 * If colors is NULL (most common case), the color returned is either a forced
 * color (from option "weechat.look.nick_color_force") or a color from option
 * "weechat.color.chat_nick_colors".
 *
 * If colors is set and not empty, a color from this list is returned
 * (format of argument: comma-separated list of colors, a background is
 * allowed with format "fg:bg", for example: "blue,yellow:red" for blue and
 * yellow on red).
 *
 * Returns the name of a color (for example: "green").
 *
 * Note: result must be freed after use.
 */

char *
gui_nick_find_color_name (const char *nickname, const char *colors)
{
    int color, num_colors;
    char *nickname2, **list_colors, *result;
    const char *forced_color, *ptr_result;
    static char *default_color = "default";

    list_colors = NULL;
    num_colors = 0;
    nickname2 = NULL;
    ptr_result = NULL;

    if (!nickname || !nickname[0])
        goto end;

    if (colors && colors[0])
    {
        list_colors = string_split (colors, ",", NULL, 0, 0, &num_colors);
        if (!list_colors || (num_colors == 0))
            goto end;
    }

    nickname2 = gui_nick_strdup_for_color (nickname);

    if (!list_colors)
    {
        /* look if color is forced for the nick */
        forced_color = gui_nick_get_forced_color (
            (nickname2) ? nickname2 : nickname);
        if (forced_color)
        {
            ptr_result = forced_color;
            goto end;
        }
        /* ensure nick colors are properly set */
        if (!config_nick_colors)
            config_set_nick_colors ();
        if (config_num_nick_colors == 0)
            goto end;
    }

    /* hash nickname to get color */
    color = gui_nick_hash_color (
        (nickname2) ? nickname2 : nickname,
        (list_colors) ? num_colors : config_num_nick_colors);
    ptr_result = (list_colors) ?
        list_colors[color] : config_nick_colors[color];

end:
    result = strdup ((ptr_result) ? ptr_result : default_color);
    if (list_colors)
        string_free_split (list_colors);
    if (nickname2)
        free (nickname2);
    return result;
}

/*
 * Finds a color code for a nick (according to nick letters).
 *
 * If colors is NULL (most common case), the color returned is either a forced
 * color (from option "weechat.look.nick_color_force") or a color from option
 * "weechat.color.chat_nick_colors".
 *
 * If colors is set and not empty, a color from this list is returned
 * (format of argument: comma-separated list of colors, a background is
 * allowed with format "fg:bg", for example: "blue,yellow:red" for blue and
 * yellow on red).
 *
 * Returns a WeeChat color code (that can be used for display).
 *
 * Note: result must be freed after use.
 */

char *
gui_nick_find_color (const char *nickname, const char *colors)
{
    char *color;
    const char *ptr_result;

    color = gui_nick_find_color_name (nickname, colors);
    ptr_result = gui_color_get_custom (color);
    if (color)
        free (color);
    return (ptr_result) ? strdup (ptr_result) : NULL;
}
