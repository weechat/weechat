/*
 * gui-focus.c - functions about focus (cursor mode and mouse) (used by all GUI)
 *
 * Copyright (C) 2011-2012 Sebastien Helleu <flashcode@flashtux.org>
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

#include "../core/weechat.h"
#include "../core/wee-hashtable.h"
#include "../core/wee-string.h"
#include "../plugins/plugin.h"
#include "gui-bar.h"
#include "gui-bar-window.h"
#include "gui-buffer.h"
#include "gui-color.h"
#include "gui-focus.h"
#include "gui-line.h"
#include "gui-window.h"


#define FOCUS_STR(__name, __string)                                     \
    hashtable_set (hashtable, __name, __string);
#define FOCUS_STR_VAR(__name, __var)                                    \
    hashtable_set (hashtable, __name, (__var) ? __var : "");
#define FOCUS_INT(__name, __int)                                        \
    snprintf (str_value, sizeof (str_value), "%d", __int);              \
    hashtable_set (hashtable, __name, str_value);
#define FOCUS_TIME(__name, __time)                                      \
    snprintf (str_value, sizeof (str_value), "%ld", (long)__time);      \
    hashtable_set (hashtable, __name, str_value);
#define FOCUS_PTR(__name, __pointer)                                    \
    if (__pointer)                                                      \
    {                                                                   \
        snprintf (str_value, sizeof (str_value),                        \
                  "0x%lx", (long unsigned int)__pointer);               \
        hashtable_set (hashtable, __name, str_value);                   \
    }                                                                   \
    else                                                                \
    {                                                                   \
        hashtable_set (hashtable, __name, "");                          \
    }


/*
 * Gets info about what is pointed by cursor at (x,y).
 *
 * Returns pointer to focus info, NULL if error.
 *
 * Note: focus info must be freed after use.
 */

struct t_gui_focus_info *
gui_focus_get_info (int x, int y)
{
    struct t_gui_focus_info *focus_info;

    focus_info = malloc (sizeof (*focus_info));
    if (!focus_info)
        return NULL;

    focus_info->x = x;
    focus_info->y = y;

    /* search window */
    focus_info->window = gui_window_search_by_xy (x, y);

    /* fill info about chat area */
    gui_window_get_context_at_xy (focus_info->window,
                                  x, y,
                                  &focus_info->chat,
                                  &focus_info->chat_line,
                                  &focus_info->chat_line_x,
                                  &focus_info->chat_word,
                                  &focus_info->chat_bol,
                                  &focus_info->chat_eol);

    /* search bar window, item, and line/col in item */
    gui_bar_window_search_by_xy (focus_info->window,
                                 x, y,
                                 &focus_info->bar_window,
                                 &focus_info->bar_item,
                                 &focus_info->bar_item_line,
                                 &focus_info->bar_item_col);

    return focus_info;
}

/*
 * Frees a focus info structure.
 */

void
gui_focus_free_info (struct t_gui_focus_info *focus_info)
{
    if (focus_info->chat_word)
        free (focus_info->chat_word);
    if (focus_info->chat_bol)
        free (focus_info->chat_bol);
    if (focus_info->chat_eol)
        free (focus_info->chat_eol);

    free (focus_info);
}

/*
 * Adds local variables of buffer in hashtable.
 */

void
gui_focus_buffer_localvar_map_cb (void *data, struct t_hashtable *hashtable,
                                  const void *key, const void *value)
{
    struct t_hashtable *hashtable_focus;
    char hash_key[512];

    /* make C compiler happy */
    (void) hashtable;

    hashtable_focus = (struct t_hashtable *)data;

    if (hashtable_focus && key && value)
    {
        snprintf (hash_key, sizeof (hash_key),
                  "_buffer_localvar_%s", (const char *)key);
        hashtable_set (hashtable_focus, hash_key, (const char *)value);
    }
}

/*
 * Adds two focus info into hashtable.
 *
 * Returns pointer to new hashtable.
 *
 * Note: result must be freed after use.
 */

struct t_hashtable *
gui_focus_to_hashtable (struct t_gui_focus_info *focus_info, const char *key)
{
    struct t_hashtable *hashtable;
    char str_value[128], *str_time, *str_prefix, *str_tags, *str_message;
    const char *nick;

    hashtable = hashtable_new (32,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL,
                               NULL);
    if (!hashtable)
        return NULL;

    /* key (key from keyboard or mouse event) */
    FOCUS_STR("_key", key);

    /* x,y */
    FOCUS_INT("_x", focus_info->x);
    FOCUS_INT("_y", focus_info->y);

    /* window/buffer */
    FOCUS_PTR("_window", focus_info->window);
    if (focus_info->window)
    {
        FOCUS_INT("_window_number", (focus_info->window)->number);
        FOCUS_PTR("_buffer", focus_info->window->buffer);
        FOCUS_INT("_buffer_number", ((focus_info->window)->buffer)->number);
        FOCUS_STR("_buffer_plugin", plugin_get_name (((focus_info->window)->buffer)->plugin));
        FOCUS_STR("_buffer_name", ((focus_info->window)->buffer)->name);
        FOCUS_STR("_buffer_full_name", ((focus_info->window)->buffer)->full_name);
        hashtable_map (focus_info->window->buffer->local_variables,
                       &gui_focus_buffer_localvar_map_cb, hashtable);
    }
    else
    {
        FOCUS_STR("_window_number", "*");
        FOCUS_PTR("_buffer", NULL);
        FOCUS_STR("_buffer_number", "-1");
        FOCUS_STR("_buffer_plugin", "");
        FOCUS_STR("_buffer_name", "");
        FOCUS_STR("_buffer_full_name", "");
    }

    /* chat area */
    FOCUS_INT("_chat", focus_info->chat);
    str_time = NULL;
    str_prefix = NULL;
    if (focus_info->chat_line)
    {
        str_time = gui_color_decode (((focus_info->chat_line)->data)->str_time, NULL);
        str_prefix = gui_color_decode (((focus_info->chat_line)->data)->prefix, NULL);
        str_tags = string_build_with_split_string ((const char **)((focus_info->chat_line)->data)->tags_array, ",");
        str_message = gui_color_decode (((focus_info->chat_line)->data)->message, NULL);
        nick = gui_line_get_nick_tag (focus_info->chat_line);
        FOCUS_INT("_chat_line_x", focus_info->chat_line_x);
        FOCUS_INT("_chat_line_y", ((focus_info->chat_line)->data)->y);
        FOCUS_TIME("_chat_line_date", ((focus_info->chat_line)->data)->date);
        FOCUS_TIME("_chat_line_date_printed", ((focus_info->chat_line)->data)->date_printed);
        FOCUS_STR_VAR("_chat_line_time", str_time);
        FOCUS_STR_VAR("_chat_line_tags", str_tags);
        FOCUS_STR_VAR("_chat_line_nick", nick);
        FOCUS_STR_VAR("_chat_line_prefix", str_prefix);
        FOCUS_STR_VAR("_chat_line_message", str_message);
        if (str_time)
            free (str_time);
        if (str_prefix)
            free (str_prefix);
        if (str_tags)
            free (str_tags);
        if (str_message)
            free (str_message);
    }
    else
    {
        FOCUS_STR("_chat_line_x", "-1");
        FOCUS_STR("_chat_line_y", "-1");
        FOCUS_STR("_chat_line_date", "-1");
        FOCUS_STR("_chat_line_date_printed", "-1");
        FOCUS_STR("_chat_line_time", "");
        FOCUS_STR("_chat_line_tags", "");
        FOCUS_STR("_chat_line_nick", "");
        FOCUS_STR("_chat_line_prefix", "");
        FOCUS_STR("_chat_line_message", "");
    }
    FOCUS_STR_VAR("_chat_word", focus_info->chat_word);
    FOCUS_STR_VAR("_chat_bol", focus_info->chat_bol);
    FOCUS_STR_VAR("_chat_eol", focus_info->chat_eol);

    /* bar/item */
    if (focus_info->bar_window)
    {
        FOCUS_STR("_bar_name", ((focus_info->bar_window)->bar)->name);
        FOCUS_STR("_bar_filling", gui_bar_filling_string[gui_bar_get_filling ((focus_info->bar_window)->bar)]);
    }
    else
    {
        FOCUS_STR("_bar_name", "");
        FOCUS_STR("_bar_filling", "");
    }
    FOCUS_STR_VAR("_bar_item_name", focus_info->bar_item);
    FOCUS_INT("_bar_item_line", focus_info->bar_item_line);
    FOCUS_INT("_bar_item_col", focus_info->bar_item_col);

    return hashtable;
}
