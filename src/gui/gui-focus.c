/*
 * gui-focus.c - functions about focus (cursor mode and mouse) (used by all GUI)
 *
 * Copyright (C) 2011-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#include "../core/weechat.h"
#include "../core/core-hashtable.h"
#include "../core/core-hook.h"
#include "../core/core-string.h"
#include "../plugins/plugin.h"
#include "gui-bar.h"
#include "gui-bar-window.h"
#include "gui-buffer.h"
#include "gui-color.h"
#include "gui-focus.h"
#include "gui-line.h"
#include "gui-window.h"


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
    focus_info->buffer = (focus_info->window) ? (focus_info->window)->buffer : NULL;

    /* fill info about chat area */
    gui_window_get_context_at_xy (focus_info->window,
                                  x, y,
                                  &focus_info->chat,
                                  &focus_info->chat_line,
                                  &focus_info->chat_line_x,
                                  &focus_info->chat_word,
                                  &focus_info->chat_focused_line,
                                  &focus_info->chat_focused_line_bol,
                                  &focus_info->chat_focused_line_eol,
                                  &focus_info->chat_bol,
                                  &focus_info->chat_eol);

    /* search bar window, item, and line/col in item */
    gui_bar_window_search_by_xy (focus_info->window,
                                 x, y,
                                 &focus_info->bar_window,
                                 &focus_info->bar_item,
                                 &focus_info->bar_item_line,
                                 &focus_info->bar_item_col,
                                 &focus_info->buffer);

    /* force current buffer if not buffer at all was found */
    if (!focus_info->buffer && gui_current_window)
        focus_info->buffer = gui_current_window->buffer;

    return focus_info;
}

/*
 * Frees a focus info structure.
 */

void
gui_focus_free_info (struct t_gui_focus_info *focus_info)
{
    if (!focus_info)
        return;

    free (focus_info->chat_word);
    free (focus_info->chat_focused_line);
    free (focus_info->chat_focused_line_bol);
    free (focus_info->chat_focused_line_eol);
    free (focus_info->chat_bol);
    free (focus_info->chat_eol);

    free (focus_info);
}

/*
 * Adds local variables of buffer in hashtable.
 */

void
gui_focus_buffer_localvar_map_cb (void *data,
                                  struct t_hashtable *hashtable,
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
 * Adds focus info into hashtable.
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
                               NULL, NULL);
    if (!hashtable)
        return NULL;

    /* key (key from keyboard or mouse event) */
    if (key)
        HASHTABLE_SET_STR("_key", key);

    /* x,y */
    HASHTABLE_SET_INT("_x", focus_info->x);
    HASHTABLE_SET_INT("_y", focus_info->y);

    /* window */
    HASHTABLE_SET_POINTER("_window", focus_info->window);
    if (focus_info->window)
    {
        HASHTABLE_SET_INT("_window_number", (focus_info->window)->number);
    }
    else
    {
        HASHTABLE_SET_STR("_window_number", "*");
    }

    /* buffer */
    HASHTABLE_SET_POINTER("_buffer", focus_info->buffer);
    if (focus_info->buffer)
    {
        HASHTABLE_SET_INT("_buffer_number", (focus_info->buffer)->number);
        HASHTABLE_SET_STR("_buffer_plugin", plugin_get_name ((focus_info->buffer)->plugin));
        HASHTABLE_SET_STR("_buffer_name", (focus_info->buffer)->name);
        HASHTABLE_SET_STR("_buffer_full_name", (focus_info->buffer)->full_name);
        hashtable_map ((focus_info->buffer)->local_variables,
                       &gui_focus_buffer_localvar_map_cb, hashtable);
    }
    else
    {
        HASHTABLE_SET_POINTER("_buffer", NULL);
        HASHTABLE_SET_STR("_buffer_number", "-1");
        HASHTABLE_SET_STR("_buffer_plugin", "");
        HASHTABLE_SET_STR("_buffer_name", "");
        HASHTABLE_SET_STR("_buffer_full_name", "");
    }

    /* chat area */
    HASHTABLE_SET_INT("_chat", focus_info->chat);
    str_time = NULL;
    str_prefix = NULL;
    if (focus_info->chat_line)
    {
        str_time = gui_color_decode (((focus_info->chat_line)->data)->str_time, NULL);
        str_prefix = gui_color_decode (((focus_info->chat_line)->data)->prefix, NULL);
        str_tags = string_rebuild_split_string (
            (const char **)((focus_info->chat_line)->data)->tags_array, ",", 0, -1);
        str_message = gui_color_decode (((focus_info->chat_line)->data)->message, NULL);
        nick = gui_line_get_nick_tag (focus_info->chat_line);
        HASHTABLE_SET_POINTER("_chat_line", focus_info->chat_line);
        HASHTABLE_SET_INT("_chat_line_x", focus_info->chat_line_x);
        HASHTABLE_SET_INT("_chat_line_y", ((focus_info->chat_line)->data)->y);
        HASHTABLE_SET_TIME("_chat_line_date", ((focus_info->chat_line)->data)->date);
        HASHTABLE_SET_INT("_chat_line_date_usec", ((focus_info->chat_line)->data)->date_usec);
        HASHTABLE_SET_TIME("_chat_line_date_printed", ((focus_info->chat_line)->data)->date_printed);
        HASHTABLE_SET_INT("_chat_line_date_usec_printed", ((focus_info->chat_line)->data)->date_usec_printed);
        HASHTABLE_SET_STR_NOT_NULL("_chat_line_time", str_time);
        HASHTABLE_SET_STR_NOT_NULL("_chat_line_tags", str_tags);
        HASHTABLE_SET_STR_NOT_NULL("_chat_line_nick", nick);
        HASHTABLE_SET_STR_NOT_NULL("_chat_line_prefix", str_prefix);
        HASHTABLE_SET_STR_NOT_NULL("_chat_line_message", str_message);
        free (str_time);
        free (str_prefix);
        free (str_tags);
        free (str_message);
    }
    else
    {
        HASHTABLE_SET_POINTER("_chat_line", NULL);
        HASHTABLE_SET_STR("_chat_line_x", "-1");
        HASHTABLE_SET_STR("_chat_line_y", "-1");
        HASHTABLE_SET_STR("_chat_line_date", "-1");
        HASHTABLE_SET_STR("_chat_line_date_usec", "-1");
        HASHTABLE_SET_STR("_chat_line_date_printed", "-1");
        HASHTABLE_SET_STR("_chat_line_date_usec_printed", "-1");
        HASHTABLE_SET_STR("_chat_line_time", "");
        HASHTABLE_SET_STR("_chat_line_tags", "");
        HASHTABLE_SET_STR("_chat_line_nick", "");
        HASHTABLE_SET_STR("_chat_line_prefix", "");
        HASHTABLE_SET_STR("_chat_line_message", "");
    }
    HASHTABLE_SET_STR_NOT_NULL("_chat_word", focus_info->chat_word);
    HASHTABLE_SET_STR_NOT_NULL("_chat_focused_line", focus_info->chat_focused_line);
    HASHTABLE_SET_STR_NOT_NULL("_chat_focused_line_bol", focus_info->chat_focused_line_bol);
    HASHTABLE_SET_STR_NOT_NULL("_chat_focused_line_eol", focus_info->chat_focused_line_eol);
    HASHTABLE_SET_STR_NOT_NULL("_chat_bol", focus_info->chat_bol);
    HASHTABLE_SET_STR_NOT_NULL("_chat_eol", focus_info->chat_eol);

    /* bar/item */
    HASHTABLE_SET_POINTER("_bar_window", focus_info->bar_window);
    if (focus_info->bar_window)
    {
        HASHTABLE_SET_STR("_bar_name", ((focus_info->bar_window)->bar)->name);
        HASHTABLE_SET_STR("_bar_filling", gui_bar_filling_string[gui_bar_get_filling ((focus_info->bar_window)->bar)]);
    }
    else
    {
        HASHTABLE_SET_STR("_bar_name", "");
        HASHTABLE_SET_STR("_bar_filling", "");
    }
    HASHTABLE_SET_STR_NOT_NULL("_bar_item_name", focus_info->bar_item);
    HASHTABLE_SET_INT("_bar_item_line", focus_info->bar_item_line);
    HASHTABLE_SET_INT("_bar_item_col", focus_info->bar_item_col);

    return hashtable;
}

/*
 * Returns GUI focus info with hashtable "gui_focus_info".
 */

struct t_hashtable *
gui_focus_info_hashtable_gui_focus_info_cb (const void *pointer, void *data,
                                            const char *info_name,
                                            struct t_hashtable *hashtable)
{
    char *error;
    const char *ptr_value;
    int x, y;
    struct t_gui_focus_info *focus_info;
    struct t_hashtable *focus_hashtable, *ret_hashtable;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    if (!hashtable)
        return NULL;

    /* parse coordinates */
    ptr_value = hashtable_get (hashtable, "x");
    if (!ptr_value)
        return NULL;
    error = NULL;
    x = (int)strtol (ptr_value, &error, 10);
    if (!error || error[0])
        return NULL;

    ptr_value = hashtable_get (hashtable, "y");
    if (!ptr_value)
        return NULL;
    error = NULL;
    y = (int)strtol (ptr_value, &error, 10);
    if (!error || error[0])
        return NULL;

    /* get focus info */
    focus_info = gui_focus_get_info (x, y);
    if (!focus_info)
        return NULL;

    /* convert to hashtable */
    focus_hashtable = gui_focus_to_hashtable (focus_info, NULL); /* no key */
    gui_focus_free_info (focus_info);
    if (!focus_hashtable)
        return NULL;

    /* run hook_focus callbacks that add extra data */
    ret_hashtable = hook_focus_get_data (focus_hashtable, NULL); /* no gesture */

    free (focus_hashtable);

    return ret_hashtable;
}
