/*
 * gui-bar-item.c - bar item functions (used by all GUI)
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stddef.h>
#include <string.h>

#include "../core/weechat.h"
#include "../core/core-arraylist.h"
#include "../core/core-config.h"
#include "../core/core-debug.h"
#include "../core/core-hashtable.h"
#include "../core/core-hdata.h"
#include "../core/core-hook.h"
#include "../core/core-infolist.h"
#include "../core/core-log.h"
#include "../core/core-string.h"
#include "../core/core-utf8.h"
#include "../core/core-util.h"
#include "../plugins/plugin.h"
#include "gui-bar-item.h"
#include "gui-bar.h"
#include "gui-bar-window.h"
#include "gui-buffer.h"
#include "gui-chat.h"
#include "gui-color.h"
#include "gui-completion.h"
#include "gui-cursor.h"
#include "gui-filter.h"
#include "gui-history.h"
#include "gui-hotlist.h"
#include "gui-key.h"
#include "gui-line.h"
#include "gui-nicklist.h"
#include "gui-window.h"
#include "gui-mouse.h"


struct t_gui_bar_item *gui_bar_items = NULL;     /* first bar item          */
struct t_gui_bar_item *last_gui_bar_item = NULL; /* last bar item           */
char *gui_bar_item_names[GUI_BAR_NUM_ITEMS] =
{ "input_paste", "input_prompt", "input_search", "input_text", "time",
  "buffer_count", "buffer_last_number", "buffer_plugin", "buffer_number",
  "buffer_name", "buffer_short_name", "buffer_modes", "buffer_filter",
  "buffer_zoom", "buffer_nicklist_count", "buffer_nicklist_count_groups",
  "buffer_nicklist_count_all", "scroll", "hotlist", "completion",
  "buffer_title", "buffer_nicklist", "window_number", "mouse_status", "lag",
  "away", "spacer"
};
struct t_gui_bar_item_hook *gui_bar_item_hooks = NULL;
struct t_hook *gui_bar_item_timer = NULL;
struct t_hook *gui_bar_item_timer_hotlist_resort = NULL;


/*
 * Checks if a bar item pointer is valid.
 *
 * Returns:
 *   1: bar item exists
 *   0: bar item does not exist
 */

int
gui_bar_item_valid (struct t_gui_bar_item *bar_item)
{
    struct t_gui_bar_item *ptr_bar_item;

    if (!bar_item)
        return 0;

    for (ptr_bar_item = gui_bar_items; ptr_bar_item;
         ptr_bar_item = ptr_bar_item->next_item)
    {
        if (ptr_bar_item == bar_item)
            return 1;
    }

    /* bar item not found */
    return 0;
}

/*
 * Searches for a default bar item by name.
 *
 * Returns index in gui_bar_item_names[], -1 if not found.
 */

int
gui_bar_item_search_default (const char *item_name)
{
    int i;

    if (!item_name || !item_name[0])
        return -1;

    for (i = 0; i < GUI_BAR_NUM_ITEMS; i++)
    {
        if (strcmp (gui_bar_item_names[i], item_name) == 0)
            return i;
    }

    /* default bar item not found */
    return -1;
}

/*
 * Searches for a bar item by name.
 */

struct t_gui_bar_item *
gui_bar_item_search (const char *item_name)
{
    struct t_gui_bar_item *ptr_item;

    if (!item_name || !item_name[0])
        return NULL;

    for (ptr_item = gui_bar_items; ptr_item; ptr_item = ptr_item->next_item)
    {
        if (strcmp (ptr_item->name, item_name) == 0)
            return ptr_item;
    }

    /* bar item not found */
    return NULL;
}

/*
 * Searches for a bar item in a plugin.
 *
 * If exact_plugin == 1, then searches only for this plugin, otherwise, if
 * plugin is not found, function may return item for core (plugin == NULL).
 *
 * Returns pointer to bar item found, NULL if not found.
 */

struct t_gui_bar_item *
gui_bar_item_search_with_plugin (struct t_weechat_plugin *plugin,
                                 int exact_plugin,
                                 const char *item_name)
{
    struct t_gui_bar_item *ptr_item, *item_found_plugin;
    struct t_gui_bar_item *item_found_without_plugin;

    if (!item_name || !item_name[0])
        return NULL;

    item_found_plugin = NULL;
    item_found_without_plugin = NULL;

    for (ptr_item = gui_bar_items; ptr_item; ptr_item = ptr_item->next_item)
    {
        if (strcmp (ptr_item->name, item_name) == 0)
        {
            if (ptr_item->plugin == plugin)
                return ptr_item;
            if (!exact_plugin)
            {
                if (ptr_item->plugin)
                    item_found_plugin = ptr_item;
                else
                    item_found_without_plugin = ptr_item;
            }
        }
    }

    if (item_found_without_plugin)
        return item_found_without_plugin;

    return item_found_plugin;
}

/*
 * Check if an item is used in a bar.
 *
 * If partial_name == 1, then searches if an item begins with "item_name".
 *
 * Returns:
 *   1: bar item is used in the bar
 *   0: bar item is not used in the bar
 */

int
gui_bar_item_used_in_bar (struct t_gui_bar *bar, const char *item_name,
                          int partial_name)
{
    int i, j, length;

    if (!bar || !item_name)
        return 0;

    length = strlen (item_name);

    for (i = 0; i < bar->items_count; i++)
    {
        for (j = 0; j < bar->items_subcount[i]; j++)
        {
            if (bar->items_name[i][j])
            {
                if ((partial_name
                     && strncmp (bar->items_name[i][j],
                                 item_name, length) == 0)
                    || (!partial_name
                        && strcmp (bar->items_name[i][j],
                                   item_name) == 0))
                {
                    return 1;
                }
            }
        }
    }

    /* item not used in the bar */
    return 0;
}

/*
 * Checks if a bar item is used in at least one bar.
 *
 * If partial_name == 1, then searches a bar that contains item beginning with
 * "item_name".
 *
 * Returns:
 *   1: bar item is used in at least one bar
 *   0: bar item is not used in a bar
 */

int
gui_bar_item_used_in_at_least_one_bar (const char *item_name, int partial_name,
                                       int ignore_hidden_bars)
{
    struct t_gui_bar *ptr_bar;
    int i, j, length;

    if (!item_name)
        return 0;

    length = strlen (item_name);

    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        /* if bar is hidden and that hidden bars are ignored, just skip it */
        if (ignore_hidden_bars
            && CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
        {
            continue;
        }

        for (i = 0; i < ptr_bar->items_count; i++)
        {
            for (j = 0; j < ptr_bar->items_subcount[i]; j++)
            {
                if (ptr_bar->items_name[i][j])
                {
                    if ((partial_name
                         && strncmp (ptr_bar->items_name[i][j],
                                     item_name, length) == 0)
                        || (!partial_name
                            && strcmp (ptr_bar->items_name[i][j],
                                       item_name) == 0))
                    {
                        return 1;
                    }
                }
            }
        }
    }

    /* item not used in any bar */
    return 0;
}

/*
 * Gets buffer, prefix, name and suffix for an item.
 *
 * Examples:
 *   - item name "[time]" returns:
 *       buffer: NULL
 *       prefix: "["
 *       name  : "time"
 *       suffix: "]"
 *   - item name "@irc.bitlbee.&bitlbee:buffer_nicklist" returns:
 *       buffer: "irc.bitlbee.&bitlbee"
 *       prefix: NULL
 *       name  : "buffer_nicklist"
 *       suffix: NULL
 */

void
gui_bar_item_get_vars (const char *item_name,
                       char **buffer, char **prefix, char **name, char **suffix)
{
    const char *ptr, *ptr_start, *pos, *start, *end;
    int valid_char;

    *buffer = NULL;
    *prefix = NULL;
    *name = NULL;
    *suffix = NULL;

    if (!item_name)
        return;

    start = NULL;
    end = NULL;

    ptr = item_name;
    ptr_start = item_name;

    if (ptr[0] == '@')
    {
        pos = strchr (ptr, ':');
        if (pos && (pos > ptr + 1))
        {
            *buffer = string_strndup (ptr + 1, pos - ptr - 1);
            ptr = pos + 1;
            ptr_start = pos + 1;
        }
    }

    while (ptr[0])
    {
        valid_char = (((ptr[0] >= 'a') && (ptr[0] <= 'z'))
                      || ((ptr[0] >= 'A') && (ptr[0] <= 'Z'))
                      || ((ptr[0] >= '0') && (ptr[0] <= '9'))
                      || (ptr[0] == '-')
                      || (ptr[0] == '_')) ? 1 : 0;
        if (!start && valid_char)
            start = ptr;
        else if (start && !end && !valid_char)
            end = ptr - 1;
        ptr++;
    }
    if (start)
    {
        if (start > ptr_start)
            *prefix = string_strndup (ptr_start, start - ptr_start);
    }
    else
        *prefix = strdup (ptr_start);
    if (start)
    {
        *name = (end) ?
            string_strndup (start, end - start + 1) : strdup (start);
    }
    if (end && end[0] && end[1])
        *suffix = strdup (end + 1);
}

/*
 * Returns value of a bar item.
 *
 * Run callbacks if a name exists, then concatenates:
 * prefix + return of callback + suffix
 *
 * For example:  if item == "[time]"
 *               returns: color(delimiter) + "[" +
 *                        (value of item "time") + color(delimiter) + "]"
 *
 * Note: result must be freed after use.
 */

char *
gui_bar_item_get_value (struct t_gui_bar *bar, struct t_gui_window *window,
                        int item, int subitem)
{
    char *item_value, delimiter_color[32], bar_color[32];
    char **result, str_attr[8], *str_diff;
    struct t_gui_buffer *buffer;
    struct t_gui_bar_item *ptr_item;
    struct timeval start_time, end_time;
    long long time_diff;

    if (!bar || !bar->items_array[item][subitem])
        return NULL;

    buffer = (window) ?
        window->buffer : ((gui_current_window) ? gui_current_window->buffer : NULL);

    item_value = NULL;
    if (bar->items_name[item][subitem])
    {
        if (bar->items_buffer[item][subitem])
        {
            buffer = gui_buffer_search_by_full_name (bar->items_buffer[item][subitem]);
            if (!buffer)
                return NULL;
        }
        ptr_item = gui_bar_item_search_with_plugin ((buffer) ? buffer->plugin : NULL,
                                                    0,
                                                    bar->items_name[item][subitem]);
        if (ptr_item && ptr_item->build_callback)
        {
            if (debug_long_callbacks > 0)
            {
                gettimeofday (&start_time, NULL);
            }
            else
            {
                start_time.tv_sec = 0;
                start_time.tv_usec = 0;
            }
            item_value = (ptr_item->build_callback) (
                ptr_item->build_callback_pointer,
                ptr_item->build_callback_data,
                ptr_item,
                window,
                buffer,
                NULL);
            if ((debug_long_callbacks > 0) && (start_time.tv_sec > 0))
            {
                gettimeofday (&end_time, NULL);
                time_diff = util_timeval_diff (&start_time, &end_time);
                if (time_diff >= debug_long_callbacks)
                {
                    str_diff = util_get_microseconds_string ((unsigned long long)time_diff);
                    log_printf (
                        _("debug: long callback: bar: %s, item: %s, plugin: %s, "
                          "time elapsed: %s"),
                        bar->name,
                        ptr_item->name,
                        plugin_get_name (ptr_item->plugin),
                        str_diff);
                    free (str_diff);
                }
            }
        }
        if (item_value && !item_value[0])
        {
            free (item_value);
            item_value = NULL;
        }
        if (!item_value)
            return NULL;
    }

    result = string_dyn_alloc (128);
    if (!result)
    {
        free (item_value);
        return NULL;
    }

    delimiter_color[0] = '\0';
    bar_color[0] = '\0';
    if (bar->items_prefix[item][subitem]
        || bar->items_suffix[item][subitem])
    {
        /* color for text in bar */
        gui_color_attr_build_string (CONFIG_COLOR(bar->options[GUI_BAR_OPTION_COLOR_FG]),
                                     str_attr);
        if (CONFIG_COLOR(bar->options[GUI_BAR_OPTION_COLOR_FG]) & GUI_COLOR_EXTENDED_FLAG)
        {
            snprintf (bar_color, sizeof (bar_color),
                      "%c%c%c%s%05d",
                      GUI_COLOR_COLOR_CHAR,
                      GUI_COLOR_FG_CHAR,
                      GUI_COLOR_EXTENDED_CHAR,
                      str_attr,
                      CONFIG_COLOR(bar->options[GUI_BAR_OPTION_COLOR_FG]) & GUI_COLOR_EXTENDED_MASK);
        }
        else
        {
            snprintf (bar_color, sizeof (bar_color),
                      "%c%c%s%02d",
                      GUI_COLOR_COLOR_CHAR,
                      GUI_COLOR_FG_CHAR,
                      str_attr,
                      CONFIG_COLOR(bar->options[GUI_BAR_OPTION_COLOR_FG]) & GUI_COLOR_EXTENDED_MASK);
        }

        /* color for bar delimiters */
        gui_color_attr_build_string (CONFIG_COLOR(bar->options[GUI_BAR_OPTION_COLOR_DELIM]),
                                     str_attr);
        if (CONFIG_COLOR(bar->options[GUI_BAR_OPTION_COLOR_DELIM]) & GUI_COLOR_EXTENDED_FLAG)
        {
            snprintf (delimiter_color, sizeof (delimiter_color),
                      "%c%c%c%s%05d",
                      GUI_COLOR_COLOR_CHAR,
                      GUI_COLOR_FG_CHAR,
                      GUI_COLOR_EXTENDED_CHAR,
                      str_attr,
                      CONFIG_COLOR(bar->options[GUI_BAR_OPTION_COLOR_DELIM]) & GUI_COLOR_EXTENDED_MASK);
        }
        else
        {
            snprintf (delimiter_color, sizeof (delimiter_color),
                      "%c%c%s%02d",
                      GUI_COLOR_COLOR_CHAR,
                      GUI_COLOR_FG_CHAR,
                      str_attr,
                      CONFIG_COLOR(bar->options[GUI_BAR_OPTION_COLOR_DELIM]) & GUI_COLOR_EXTENDED_MASK);
        }
    }

    if (bar->items_prefix[item][subitem])
    {
        string_dyn_concat (result, delimiter_color, -1);
        string_dyn_concat (result, bar->items_prefix[item][subitem], -1);
        string_dyn_concat (result, bar_color, -1);
    }
    if (item_value)
    {
        string_dyn_concat (result, item_value, -1);
    }
    if (bar->items_suffix[item][subitem])
    {
        string_dyn_concat (result, delimiter_color, -1);
        string_dyn_concat (result, bar->items_suffix[item][subitem], -1);
    }

    free (item_value);

    return string_dyn_free (result, 0);
}

/*
 * Counts number of lines in item.
 */

int
gui_bar_item_count_lines (char *string)
{
    int count, i;

    if (!string || !string[0])
        return 0;

    count = 1;
    i = 0;
    while (string[i])
    {
        if ((string[i] == '\n') && string[i + 1])
            count++;
        i++;
    }

    return count;
}

/*
 * Creates a new bar item.
 *
 * Returns pointer to new bar item, NULL if not found.
 */

struct t_gui_bar_item *
gui_bar_item_new (struct t_weechat_plugin *plugin, const char *name,
                  char *(*build_callback)(const void *pointer,
                                          void *data,
                                          struct t_gui_bar_item *item,
                                          struct t_gui_window *window,
                                          struct t_gui_buffer *buffer,
                                          struct t_hashtable *extra_info),
                  const void *build_callback_pointer,
                  void *build_callback_data)
{
    struct t_gui_bar_item *new_bar_item;

    if (!name || !name[0])
        return NULL;

    /* it's not possible to create 2 bar items with same name for same plugin */
    if (gui_bar_item_search_with_plugin (plugin, 1, name))
        return NULL;

    /* create bar item */
    new_bar_item =  malloc (sizeof (*new_bar_item));
    if (new_bar_item)
    {
        new_bar_item->plugin = plugin;
        new_bar_item->name = strdup (name);
        new_bar_item->build_callback = build_callback;
        new_bar_item->build_callback_pointer = build_callback_pointer;
        new_bar_item->build_callback_data = build_callback_data;

        /* add bar item to bar items queue */
        new_bar_item->prev_item = last_gui_bar_item;
        if (last_gui_bar_item)
            last_gui_bar_item->next_item = new_bar_item;
        else
            gui_bar_items = new_bar_item;
        last_gui_bar_item = new_bar_item;
        new_bar_item->next_item = NULL;

        return new_bar_item;
    }

    /* failed to create bar item */
    return NULL;
}

/*
 * Updates an item on all bars displayed on screen.
 */

void
gui_bar_item_update (const char *item_name)
{
    struct t_gui_bar *ptr_bar;
    struct t_gui_window *ptr_window;
    struct t_gui_bar_window *ptr_bar_window;
    int i, j, check_bar_conditions, condition_ok;

    if (!item_name)
        return;

    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        check_bar_conditions = 0;

        for (i = 0; i < ptr_bar->items_count; i++)
        {
            for (j = 0; j < ptr_bar->items_subcount[i]; j++)
            {
                if (ptr_bar->items_name[i][j]
                    && (strcmp (ptr_bar->items_name[i][j], item_name) == 0))
                {
                    if (!CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
                        check_bar_conditions = 1;

                    if (CONFIG_ENUM(ptr_bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
                    {
                        if (ptr_bar->bar_window)
                        {
                            ptr_bar->bar_window->items_refresh_needed[i][j] = 1;
                        }
                    }
                    else
                    {
                        for (ptr_window = gui_windows; ptr_window;
                             ptr_window = ptr_window->next_window)
                        {
                            for (ptr_bar_window = ptr_window->bar_windows;
                                 ptr_bar_window;
                                 ptr_bar_window = ptr_bar_window->next_bar_window)
                            {
                                if (ptr_bar_window->bar == ptr_bar)
                                {
                                    ptr_bar_window->items_refresh_needed[i][j] = 1;
                                }
                            }
                        }
                    }
                    gui_bar_ask_refresh (ptr_bar);
                }
            }
        }

        /*
         * evaluate bar conditions (if needed) to check if bar must be toggled
         * (hidden if shown, or shown if hidden)
         */
        if (check_bar_conditions)
        {
            if (CONFIG_ENUM(ptr_bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
            {
                condition_ok = gui_bar_check_conditions (ptr_bar, NULL);
                if ((condition_ok && !ptr_bar->bar_window)
                    || (!condition_ok && ptr_bar->bar_window))
                {
                    gui_window_ask_refresh (1);
                }
            }
            else
            {
                for (ptr_window = gui_windows; ptr_window;
                     ptr_window = ptr_window->next_window)
                {
                    condition_ok = gui_bar_check_conditions (ptr_bar,
                                                             ptr_window);
                    ptr_bar_window = gui_bar_window_search_bar (ptr_window,
                                                                ptr_bar);
                    if ((condition_ok && !ptr_bar_window)
                        || (!condition_ok && ptr_bar_window))
                    {
                        gui_window_ask_refresh (1);
                    }
                }
            }
        }
    }
}

/*
 * Deletes a bar item.
 */

void
gui_bar_item_free (struct t_gui_bar_item *item)
{
    if (!item)
        return;

    /* force refresh of bars displaying this bar item */
    gui_bar_item_update (item->name);

    /* remove bar item from bar items list */
    if (item->prev_item)
        (item->prev_item)->next_item = item->next_item;
    if (item->next_item)
        (item->next_item)->prev_item = item->prev_item;
    if (gui_bar_items == item)
        gui_bar_items = item->next_item;
    if (last_gui_bar_item == item)
        last_gui_bar_item = item->prev_item;

    /* free data */
    free (item->name);
    free (item->build_callback_data);

    free (item);
}

/*
 * Deletes all bar items.
 */

void
gui_bar_item_free_all (void)
{
    while (gui_bar_items)
    {
        gui_bar_item_free (gui_bar_items);
    }
}

/*
 * Deletes all bar items for a plugin.
 */

void
gui_bar_item_free_all_plugin (struct t_weechat_plugin *plugin)
{
    struct t_gui_bar_item *ptr_item, *next_item;

    ptr_item = gui_bar_items;
    while (ptr_item)
    {
        next_item = ptr_item->next_item;

        if (ptr_item->plugin == plugin)
            gui_bar_item_free (ptr_item);

        ptr_item = next_item;
    }
}

/*
 * Bar item with input paste question.
 */

char *
gui_bar_item_input_paste_cb (const void *pointer, void *data,
                             struct t_gui_bar_item *item,
                             struct t_gui_window *window,
                             struct t_gui_buffer *buffer,
                             struct t_hashtable *extra_info)
{
    char str_paste[1024];
    int lines;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) buffer;
    (void) extra_info;

    if (window && (window != gui_current_window))
        return NULL;

    if (!gui_key_paste_pending)
        return NULL;

    lines = gui_key_get_paste_lines ();
    snprintf (str_paste, sizeof (str_paste),
              NG_("%sPaste %d line? [ctrl-y] Yes [ctrl-n] No",
                  "%sPaste %d lines? [ctrl-y] Yes [ctrl-n] No",
                  lines),
              gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_input_actions))),
              lines);
    return strdup (str_paste);
}

/*
 * Bar item with input prompt.
 */

char *
gui_bar_item_input_prompt_cb (const void *pointer, void *data,
                              struct t_gui_bar_item *item,
                              struct t_gui_window *window,
                              struct t_gui_buffer *buffer,
                              struct t_hashtable *extra_info)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    return (buffer->input_prompt) ? strdup (buffer->input_prompt) : NULL;
}

/*
 * Bar item with input search status.
 */

char *
gui_bar_item_input_search_cb (const void *pointer, void *data,
                              struct t_gui_bar_item *item,
                              struct t_gui_window *window,
                              struct t_gui_buffer *buffer,
                              struct t_hashtable *extra_info)
{
    char str_search[1024], str_where[256];
    int text_found;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer || (buffer->text_search == GUI_BUFFER_SEARCH_DISABLED))
        return NULL;

    str_where[0] = '\0';

    switch (buffer->text_search)
    {
        case GUI_BUFFER_SEARCH_DISABLED:
            return NULL;
        case GUI_BUFFER_SEARCH_LINES:
            snprintf (
                str_where, sizeof (str_where),
                "%s%s%s",
                (buffer->text_search_where & GUI_BUFFER_SEARCH_IN_PREFIX) ? "pre" : "",
                ((buffer->text_search_where & GUI_BUFFER_SEARCH_IN_PREFIX)
                 && (buffer->text_search_where & GUI_BUFFER_SEARCH_IN_MESSAGE)) ? "|" : "",
                (buffer->text_search_where & GUI_BUFFER_SEARCH_IN_MESSAGE) ? "msg" : "");
            break;
        case GUI_BUFFER_SEARCH_HISTORY:
            snprintf (str_where, sizeof (str_where),
                      "%s",
                      (buffer->text_search_history == GUI_BUFFER_SEARCH_HISTORY_LOCAL) ?
                      /* TRANSLATORS: search in "local" history */
                      _("local") :
                      /* TRANSLATORS: search in "global" history */
                      _("global"));
            break;
        case GUI_BUFFER_NUM_SEARCH:
            return NULL;
    }

    text_found = (buffer->text_search_found
                  || !buffer->input_buffer
                  || !buffer->input_buffer[0]);

    snprintf (
        str_search, sizeof (str_search),
        "%s%s (%s %s,%s)",
        (text_found) ?
        GUI_COLOR_CUSTOM_BAR_FG :
        gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_input_text_not_found))),
        (buffer->text_search == GUI_BUFFER_SEARCH_LINES) ?
        _("Search lines") : _("Search command"),
        (buffer->text_search_exact) ? "==" : "~",
        (buffer->text_search_regex) ? "regex" : "str",
        str_where);

    return strdup (str_search);
}

/*
 * Bar item with input text.
 */

char *
gui_bar_item_input_text_cb (const void *pointer, void *data,
                            struct t_gui_bar_item *item,
                            struct t_gui_window *window,
                            struct t_gui_buffer *buffer,
                            struct t_hashtable *extra_info)
{
    char *ptr_input, *ptr_input2, str_buffer[128], str_start_input[16];
    char str_cursor[16], *buf, str_key_debug[1024], *str_lead_linebreak;
    const char *pos_cursor;
    int length, length_cursor;
    int buf_pos, is_multiline;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    snprintf (str_cursor, sizeof (str_cursor), "%c%c%c",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_BAR_CHAR,
              GUI_COLOR_BAR_MOVE_CURSOR_CHAR);
    length_cursor = strlen (str_cursor);
    snprintf (str_start_input, sizeof (str_start_input), "%c%c%c",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_BAR_CHAR,
              GUI_COLOR_BAR_START_INPUT_CHAR);

    if (gui_key_debug)
    {
        snprintf (str_key_debug, sizeof (str_key_debug),
                  "%s%s",
                  _("keyboard and mouse debug ('q' to quit debug mode)"),
                  str_cursor);
        return strdup (str_key_debug);
    }

    /* for modifiers */
    snprintf (str_buffer, sizeof (str_buffer), "0x%lx", (unsigned long)buffer);

    /* execute modifier with basic string (without cursor tag) */
    ptr_input = NULL;
    if (!gui_cursor_mode)
    {
        ptr_input = hook_modifier_exec (NULL,
                                        "input_text_display",
                                        str_buffer,
                                        (buffer->input_buffer) ?
                                        buffer->input_buffer : "");
    }
    if (!ptr_input)
    {
        ptr_input = (buffer->input_buffer) ?
            strdup (buffer->input_buffer) : NULL;
    }

    /* insert "move cursor" id in string */
    if (ptr_input)
    {
        pos_cursor = gui_chat_string_add_offset (ptr_input,
                                                 buffer->input_buffer_pos);
        length = strlen (ptr_input) + length_cursor + 1;
        buf = malloc (length);
        if (buf)
        {
            buf_pos = 0;

            if (!pos_cursor)
                pos_cursor = ptr_input;

            /* add beginning of buffer */
            if (pos_cursor != ptr_input)
            {
                memmove (buf + buf_pos, ptr_input, pos_cursor - ptr_input);
                buf_pos += (pos_cursor - ptr_input);
            }
            /* add "move cursor here" identifier in string */
            snprintf (buf + buf_pos, length - buf_pos, "%s",
                      str_cursor);
            /* add end of buffer */
            strcat (buf, pos_cursor);

            free (ptr_input);
            ptr_input = buf;
        }
    }
    else
    {
        ptr_input = strdup (str_cursor);
    }

    /* execute modifier with cursor in string */
    if (!gui_cursor_mode)
    {
        ptr_input2 = hook_modifier_exec (NULL,
                                         "input_text_display_with_cursor",
                                         str_buffer,
                                         (ptr_input) ? ptr_input : "");
        free (ptr_input);
        ptr_input = ptr_input2;
    }

    /* add matching text found in history (in history search mode) */
    if ((buffer->text_search == GUI_BUFFER_SEARCH_HISTORY)
        && buffer->text_search_ptr_history)
    {
        if (string_asprintf (&buf,
                             "%s  => %s",
                             ptr_input,
                             (buffer->text_search_ptr_history->text) ?
                             buffer->text_search_ptr_history->text : "") >= 0)
        {
            free (ptr_input);
            ptr_input = buf;
        }
    }

    /*
     * transform '\n' to '\r' so the newlines are displayed as real new lines
     * instead of spaces
     */
    is_multiline = 0;
    ptr_input2 = ptr_input;
    while (ptr_input2 && ptr_input2[0])
    {
        if (ptr_input2[0] == '\n')
        {
            ptr_input2[0] = '\r';
            is_multiline = 1;
        }
        ptr_input2 = (char *)utf8_next_char (ptr_input2);
    }

    str_lead_linebreak = (is_multiline &&
        CONFIG_BOOLEAN(config_look_input_multiline_lead_linebreak)) ? "\r" : "";

    /* insert "start input" at beginning of string */
    if (ptr_input)
    {
        if (string_asprintf (&buf,
                             "%s%s%s",
                             str_start_input,
                             str_lead_linebreak,
                             ptr_input) >= 0)
        {
            free (ptr_input);
            ptr_input = buf;
        }
    }
    else
    {
        string_asprintf (&ptr_input, "%s%s", str_start_input, str_cursor);
    }

    return ptr_input;
}

/*
 * Bar item with time.
 */

char *
gui_bar_item_time_cb (const void *pointer, void *data,
                      struct t_gui_bar_item *item,
                      struct t_gui_window *window,
                      struct t_gui_buffer *buffer,
                      struct t_hashtable *extra_info)
{
    char text_time[128], text_time2[256];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) buffer;
    (void) extra_info;

    config_get_item_time (text_time, sizeof (text_time));
    if (!text_time[0])
        return NULL;

    snprintf (text_time2, sizeof (text_time2), "%s%s",
              gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_time))),
              text_time);

    return strdup (text_time2);
}

/*
 * Bar item with number of buffers.
 */

char *
gui_bar_item_buffer_count_cb (const void *pointer, void *data,
                              struct t_gui_bar_item *item,
                              struct t_gui_window *window,
                              struct t_gui_buffer *buffer,
                              struct t_hashtable *extra_info)
{
    char buf[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) buffer;
    (void) extra_info;

    snprintf (buf, sizeof (buf), "%d", gui_buffers_count);

    return strdup (buf);
}

/*
 * Bar item with last buffer number.
 */

char *
gui_bar_item_buffer_last_number_cb (const void *pointer, void *data,
                                    struct t_gui_bar_item *item,
                                    struct t_gui_window *window,
                                    struct t_gui_buffer *buffer,
                                    struct t_hashtable *extra_info)
{
    char buf[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) buffer;
    (void) extra_info;

    snprintf (buf, sizeof (buf), "%d",
              (last_gui_buffer) ? last_gui_buffer->number : 0);

    return strdup (buf);
}

/*
 * Bar item with name of buffer plugin.
 */

char *
gui_bar_item_buffer_plugin_cb (const void *pointer, void *data,
                               struct t_gui_bar_item *item,
                               struct t_gui_window *window,
                               struct t_gui_buffer *buffer,
                               struct t_hashtable *extra_info)
{
    const char *plugin_name;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    plugin_name = gui_buffer_get_plugin_name (buffer);

    return (plugin_name) ? strdup (plugin_name) : strdup ("");
}

/*
 * Bar item with number of buffer.
 */

char *
gui_bar_item_buffer_number_cb (const void *pointer, void *data,
                               struct t_gui_bar_item *item,
                               struct t_gui_window *window,
                               struct t_gui_buffer *buffer,
                               struct t_hashtable *extra_info)
{
    char str_number[64];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    snprintf (str_number, sizeof (str_number), "%s%d",
              gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_number))),
              buffer->number);

    return strdup (str_number);
}

/*
 * Bar item with name of buffer.
 */

char *
gui_bar_item_buffer_name_cb (const void *pointer, void *data,
                             struct t_gui_bar_item *item,
                             struct t_gui_window *window,
                             struct t_gui_buffer *buffer,
                             struct t_hashtable *extra_info)
{
    char str_name[256];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    snprintf (str_name, sizeof (str_name), "%s%s",
              gui_color_get_custom (
                  gui_color_get_name (CONFIG_COLOR(config_color_status_name))),
              buffer->name);

    return strdup (str_name);
}

/*
 * Bar item with short name of buffer.
 */

char *
gui_bar_item_buffer_short_name_cb (const void *pointer, void *data,
                                   struct t_gui_bar_item *item,
                                   struct t_gui_window *window,
                                   struct t_gui_buffer *buffer,
                                   struct t_hashtable *extra_info)
{
    char str_short_name[256];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    snprintf (str_short_name, sizeof (str_short_name), "%s%s",
              gui_color_get_custom (
                  gui_color_get_name (CONFIG_COLOR(config_color_status_name))),
              buffer->short_name);

    return strdup (str_short_name);
}

/*
 * Bar item with modes of buffer.
 *
 * Note: this bar item is empty for WeeChat core, this is used only by plugins
 * like irc to display channel modes.
 */

char *
gui_bar_item_buffer_modes_cb (const void *pointer, void *data,
                              struct t_gui_bar_item *item,
                              struct t_gui_window *window,
                              struct t_gui_buffer *buffer,
                              struct t_hashtable *extra_info)
{
    char *modes;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer || !buffer->modes)
        return NULL;

    string_asprintf (
        &modes,
        "%s%s",
        gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_modes))),
        buffer->modes);
    return modes;
}

/*
 * Bar item with buffer filter indicator.
 */

char *
gui_bar_item_buffer_filter_cb (const void *pointer, void *data,
                               struct t_gui_bar_item *item,
                               struct t_gui_window *window,
                               struct t_gui_buffer *buffer,
                               struct t_hashtable *extra_info)
{
    char str_filter[512];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    if (!gui_filters_enabled || !gui_filters
        || !buffer->filter || !buffer->lines->lines_hidden)
    {
        return NULL;
    }

    snprintf (str_filter, sizeof (str_filter),
              "%s%s",
              gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_filter))),
              CONFIG_STRING(config_look_item_buffer_filter));

    return strdup (str_filter);
}

/*
 * Bar item with number of visible nicks in buffer nicklist.
 */

char *
gui_bar_item_buffer_nicklist_count_cb (const void *pointer, void *data,
                                       struct t_gui_bar_item *item,
                                       struct t_gui_window *window,
                                       struct t_gui_buffer *buffer,
                                       struct t_hashtable *extra_info)
{
    char str_count[64];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer || !buffer->nicklist)
        return NULL;

    snprintf (str_count, sizeof (str_count),
              "%s%d",
              gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_nicklist_count))),
              buffer->nicklist_nicks_visible_count);

    return strdup (str_count);
}

/*
 * Bar item with number of visible groups in buffer nicklist.
 */

char *
gui_bar_item_buffer_nicklist_count_groups_cb (const void *pointer, void *data,
                                              struct t_gui_bar_item *item,
                                              struct t_gui_window *window,
                                              struct t_gui_buffer *buffer,
                                              struct t_hashtable *extra_info)
{
    char str_count[64];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer || !buffer->nicklist)
        return NULL;

    snprintf (str_count, sizeof (str_count),
              "%s%d",
              gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_nicklist_count))),
              buffer->nicklist_groups_visible_count);

    return strdup (str_count);
}

/*
 * Bar item with number of visible groups and nicks in buffer nicklist.
 */

char *
gui_bar_item_buffer_nicklist_count_all_cb (const void *pointer, void *data,
                                           struct t_gui_bar_item *item,
                                           struct t_gui_window *window,
                                           struct t_gui_buffer *buffer,
                                           struct t_hashtable *extra_info)
{
    char str_count[64];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer || !buffer->nicklist)
        return NULL;

    snprintf (str_count, sizeof (str_count),
              "%s%d",
              gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_nicklist_count))),
              buffer->nicklist_visible_count);

    return strdup (str_count);
}

/*
 * Bar item with zoom on merged buffer.
 */

char *
gui_bar_item_buffer_zoom_cb (const void *pointer, void *data,
                             struct t_gui_bar_item *item,
                             struct t_gui_window *window,
                             struct t_gui_buffer *buffer,
                             struct t_hashtable *extra_info)

{
    char buf[512];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    /* don't display item if current buffer is not merged + zoomed */
    if (buffer->active != 2)
        return NULL;

    snprintf (buf, sizeof (buf), "%s",
              CONFIG_STRING(config_look_item_buffer_zoom));

    return strdup (buf);
}

/*
 * Bar item with scrolling indicator.
 */

char *
gui_bar_item_scroll_cb (const void *pointer, void *data,
                        struct t_gui_bar_item *item,
                        struct t_gui_window *window,
                        struct t_gui_buffer *buffer,
                        struct t_hashtable *extra_info)
{
    char str_scroll[512];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) buffer;
    (void) extra_info;

    if (!window)
        window = gui_current_window;

    if (!window->scroll->scrolling)
        return NULL;

    snprintf (str_scroll, sizeof (str_scroll), _("%s-MORE(%d)-"),
              gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_more))),
              window->scroll->lines_after);

    return strdup (str_scroll);
}

/*
 * Bar item with hotlist.
 */

char *
gui_bar_item_hotlist_cb (const void *pointer, void *data,
                         struct t_gui_bar_item *item,
                         struct t_gui_window *window,
                         struct t_gui_buffer *buffer,
                         struct t_hashtable *extra_info)
{
    char str_hotlist[4096], *buffer_without_name_displayed, *buffer_name;
    const char *hotlist_suffix, *ptr_buffer_name;
    struct t_gui_hotlist *ptr_hotlist;
    int numbers_count, names_count, display_name, count_max;
    int priority, priority_min, priority_min_displayed, private;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) buffer;
    (void) extra_info;

    if (!gui_hotlist)
        return NULL;

    str_hotlist[0] = '\0';

    strcat (str_hotlist, CONFIG_STRING(config_look_hotlist_prefix));

    buffer_without_name_displayed = NULL;
    if (CONFIG_BOOLEAN(config_look_hotlist_unique_numbers) && last_gui_buffer)
        buffer_without_name_displayed = calloc (last_gui_buffer->number, 1);

    numbers_count = 0;
    names_count = 0;
    for (ptr_hotlist = gui_hotlist; ptr_hotlist;
         ptr_hotlist = ptr_hotlist->next_hotlist)
    {
        switch (ptr_hotlist->priority)
        {
            case GUI_HOTLIST_LOW:
                display_name = ((CONFIG_INTEGER(config_look_hotlist_names_level) & 1) != 0);
                break;
            case GUI_HOTLIST_MESSAGE:
                display_name = ((CONFIG_INTEGER(config_look_hotlist_names_level) & 2) != 0);
                break;
            case GUI_HOTLIST_PRIVATE:
                display_name = ((CONFIG_INTEGER(config_look_hotlist_names_level) & 4) != 0);
                break;
            case GUI_HOTLIST_HIGHLIGHT:
                display_name = ((CONFIG_INTEGER(config_look_hotlist_names_level) & 8) != 0);
                break;
            default:
                display_name = 0;
                break;
        }

        display_name = ((CONFIG_BOOLEAN(config_look_hotlist_names_merged_buffers)
                         && (gui_buffer_count_merged_buffers (ptr_hotlist->buffer->number) > 1))
                        || (display_name
                            && (CONFIG_INTEGER(config_look_hotlist_names_count) != 0)
                            && (names_count < CONFIG_INTEGER(config_look_hotlist_names_count))));

        if (display_name || !buffer_without_name_displayed
            || (buffer_without_name_displayed[ptr_hotlist->buffer->number - 1] == 0))
        {
            if ((numbers_count > 0)
                && (CONFIG_STRING(config_look_hotlist_buffer_separator))
                && (CONFIG_STRING(config_look_hotlist_buffer_separator)[0]))
            {
                strcat (str_hotlist, GUI_COLOR_CUSTOM_BAR_DELIM);
                strcat (str_hotlist,
                        CONFIG_STRING(config_look_hotlist_buffer_separator));
            }

            switch (ptr_hotlist->priority)
            {
                case GUI_HOTLIST_LOW:
                    strcat (str_hotlist,
                            gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_data_other))));
                    break;
                case GUI_HOTLIST_MESSAGE:
                    strcat (str_hotlist,
                            gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_data_msg))));
                    break;
                case GUI_HOTLIST_PRIVATE:
                    strcat (str_hotlist,
                            gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_data_private))));
                    break;
                case GUI_HOTLIST_HIGHLIGHT:
                    strcat (str_hotlist,
                            gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_data_highlight))));
                    break;
                case GUI_HOTLIST_NUM_PRIORITIES:
                    /*
                     * this constant is used to count hotlist priorities only,
                     * it is never used as priority
                     */
                    break;
            }
            snprintf (str_hotlist + strlen (str_hotlist), 16,
                      "%d", ptr_hotlist->buffer->number);
            numbers_count++;

            if (display_name)
            {
                names_count++;

                strcat (str_hotlist, GUI_COLOR_CUSTOM_BAR_DELIM);
                strcat (str_hotlist, ":");
                strcat (str_hotlist, GUI_COLOR_CUSTOM_BAR_FG);
                ptr_buffer_name = (CONFIG_BOOLEAN(config_look_hotlist_short_names)) ?
                    ptr_hotlist->buffer->short_name : ptr_hotlist->buffer->name;
                if (CONFIG_INTEGER(config_look_hotlist_names_length) == 0)
                {
                    buffer_name = strdup (ptr_buffer_name);
                }
                else
                {
                    buffer_name = utf8_strndup (
                        ptr_buffer_name,
                        CONFIG_INTEGER(config_look_hotlist_names_length));
                }
                if (buffer_name)
                {
                    if (strlen (buffer_name) > 128)
                        buffer_name[128] = '\0';
                    strcat (str_hotlist, buffer_name);
                    free (buffer_name);
                }
            }
            else
            {
                if (buffer_without_name_displayed)
                    buffer_without_name_displayed[ptr_hotlist->buffer->number - 1] = 1;
            }

            /* display messages count by priority */
            if (CONFIG_INTEGER(config_look_hotlist_count_max) > 0)
            {
                private = (ptr_hotlist->count[GUI_HOTLIST_PRIVATE] > 0) ? 1 : 0;
                priority_min_displayed = ptr_hotlist->priority + 1;
                count_max = CONFIG_INTEGER(config_look_hotlist_count_max);
                if (!private
                    && (ptr_hotlist->priority == GUI_HOTLIST_HIGHLIGHT)
                    && (count_max > 1))
                {
                    priority_min = ptr_hotlist->priority - count_max;
                }
                else
                {
                    priority_min = ptr_hotlist->priority - count_max + 1;
                }
                if (priority_min < 0)
                    priority_min = 0;
                for (priority = ptr_hotlist->priority;
                     priority >= priority_min;
                     priority--)
                {
                    if (!private && (priority == GUI_HOTLIST_PRIVATE))
                        continue;
                    if (private && (priority == GUI_HOTLIST_MESSAGE))
                        continue;
                    if (((priority == (int)ptr_hotlist->priority)
                         && (ptr_hotlist->count[priority] >= CONFIG_INTEGER(config_look_hotlist_count_min_msg)))
                        || ((priority != (int)ptr_hotlist->priority)
                            && (ptr_hotlist->count[priority] > 0)))
                    {
                        priority_min_displayed = priority;
                    }
                }
                if (priority_min_displayed <= (int)ptr_hotlist->priority)
                {
                    for (priority = ptr_hotlist->priority;
                         priority >= priority_min_displayed;
                         priority--)
                    {
                        if (!private && (priority == GUI_HOTLIST_PRIVATE))
                            continue;
                        if (private && (priority == GUI_HOTLIST_MESSAGE))
                            continue;
                        strcat (str_hotlist, GUI_COLOR_CUSTOM_BAR_DELIM);
                        strcat (str_hotlist, (priority == (int)ptr_hotlist->priority) ? "(" : ",");
                        switch (priority)
                        {
                            case GUI_HOTLIST_LOW:
                                strcat (str_hotlist, gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_count_other))));
                                break;
                            case GUI_HOTLIST_MESSAGE:
                                strcat (str_hotlist, gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_count_msg))));
                                break;
                            case GUI_HOTLIST_PRIVATE:
                                strcat (str_hotlist, gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_count_private))));
                                break;
                            case GUI_HOTLIST_HIGHLIGHT:
                                strcat (str_hotlist, gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_count_highlight))));
                                break;
                            case GUI_HOTLIST_NUM_PRIORITIES:
                                /*
                                 * this constant is used to count hotlist priorities only,
                                 * it is never used as priority
                                 */
                                break;
                        }
                        snprintf (str_hotlist + strlen (str_hotlist), 16,
                                  "%d", ptr_hotlist->count[priority]);
                    }
                    strcat (str_hotlist, GUI_COLOR_CUSTOM_BAR_DELIM);
                    strcat (str_hotlist, ")");
                }
            }

            if (strlen (str_hotlist) > sizeof (str_hotlist) - 256)
                break;
        }
    }

    free (buffer_without_name_displayed);

    hotlist_suffix = CONFIG_STRING(config_look_hotlist_suffix);
    if (hotlist_suffix[0]
        && (strlen (str_hotlist) + strlen (CONFIG_STRING(config_look_hotlist_suffix)) + 16 < sizeof (str_hotlist)))
    {
        strcat (str_hotlist, GUI_COLOR_CUSTOM_BAR_FG);
        strcat (str_hotlist, hotlist_suffix);
    }

    return strdup (str_hotlist);
}

/*
 * Bar item with (partial) completion.
 */

char *
gui_bar_item_completion_cb (const void *pointer, void *data,
                            struct t_gui_bar_item *item,
                            struct t_gui_window *window,
                            struct t_gui_buffer *buffer,
                            struct t_hashtable *extra_info)
{
    int length, i;
    char *buf, str_number[64];
    struct t_gui_completion_word *ptr_completion_word;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer || !buffer->completion
        || (buffer->completion->partial_list->size == 0))
    {
        return NULL;
    }

    length = 1;
    for (i = 0; i < buffer->completion->partial_list->size; i++)
    {
        ptr_completion_word =
            (struct t_gui_completion_word *)(buffer->completion->partial_list->data[i]);
        length += strlen (ptr_completion_word->word) + 32;
    }

    buf = malloc (length);
    if (buf)
    {
        buf[0] = '\0';
        for (i = 0; i < buffer->completion->partial_list->size; i++)
        {
            ptr_completion_word =
                (struct t_gui_completion_word *)(buffer->completion->partial_list->data[i]);
            strcat (buf, GUI_COLOR_CUSTOM_BAR_FG);
            strcat (buf, ptr_completion_word->word);
            if (ptr_completion_word->count > 0)
            {
                strcat (buf, GUI_COLOR_CUSTOM_BAR_DELIM);
                strcat (buf, "(");
                snprintf (str_number, sizeof (str_number),
                          "%d", ptr_completion_word->count);
                strcat (buf, str_number);
                strcat (buf, ")");
            }
            if (i < buffer->completion->partial_list->size - 1)
                strcat (buf, " ");
        }
    }

    return buf;
}

/*
 * Bar item with buffer title.
 */

char *
gui_bar_item_buffer_title_cb (const void *pointer, void *data,
                              struct t_gui_bar_item *item,
                              struct t_gui_window *window,
                              struct t_gui_buffer *buffer,
                              struct t_hashtable *extra_info)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    return (buffer->title) ? strdup (buffer->title) : NULL;
}

/*
 * Bar item with nicklist.
 */

char *
gui_bar_item_buffer_nicklist_cb (const void *pointer, void *data,
                                 struct t_gui_bar_item *item,
                                 struct t_gui_window *window,
                                 struct t_gui_buffer *buffer,
                                 struct t_hashtable *extra_info)
{
    struct t_gui_nick_group *ptr_group;
    struct t_gui_nick *ptr_nick;
    struct t_config_option *ptr_option;
    char **nicklist;
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    nicklist = string_dyn_alloc (256);
    if (!nicklist)
        return NULL;

    ptr_group = NULL;
    ptr_nick = NULL;
    gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
    while (ptr_group || ptr_nick)
    {
        if ((ptr_nick && ptr_nick->visible)
            || (ptr_group && !ptr_nick
                && buffer->nicklist_display_groups
                && ptr_group->visible))
        {
            if ((*nicklist)[0])
                string_dyn_concat (nicklist, "\n", -1);

            if (ptr_nick)
            {
                if (buffer->nicklist_display_groups)
                {
                    for (i = 0; i < ptr_nick->group->level; i++)
                    {
                        string_dyn_concat (nicklist, " ", -1);
                    }
                }
                if (ptr_nick->prefix_color)
                {
                    if (strchr (ptr_nick->prefix_color, '.'))
                    {
                        config_file_search_with_string (ptr_nick->prefix_color,
                                                        NULL, NULL, &ptr_option,
                                                            NULL);
                        if (ptr_option)
                        {
                            string_dyn_concat (
                                nicklist,
                                gui_color_get_custom (
                                    gui_color_get_name (
                                        CONFIG_COLOR(ptr_option))),
                                -1);
                        }
                    }
                    else
                    {
                        string_dyn_concat (nicklist,
                                           gui_color_get_custom (
                                               ptr_nick->prefix_color),
                                           -1);
                    }
                }
                if (ptr_nick->prefix)
                    string_dyn_concat (nicklist, ptr_nick->prefix, -1);
                if (ptr_nick->color)
                {
                    if (strchr (ptr_nick->color, '.'))
                    {
                        config_file_search_with_string (ptr_nick->color,
                                                        NULL, NULL, &ptr_option,
                                                        NULL);
                        if (ptr_option)
                        {
                            string_dyn_concat (
                                nicklist,
                                gui_color_get_custom (
                                    gui_color_get_name (
                                        CONFIG_COLOR(ptr_option))),
                                -1);
                        }
                    }
                    else
                    {
                        string_dyn_concat (nicklist,
                                           gui_color_get_custom (
                                               ptr_nick->color),
                                           -1);
                    }
                }
                string_dyn_concat (nicklist, ptr_nick->name, -1);
            }
            else
            {
                for (i = 0; i < ptr_group->level - 1; i++)
                {
                    string_dyn_concat (nicklist, " ", -1);
                }
                if (ptr_group->color)
                {
                    if (strchr (ptr_group->color, '.'))
                    {
                        config_file_search_with_string (ptr_group->color,
                                                        NULL, NULL, &ptr_option,
                                                        NULL);
                        if (ptr_option)
                        {
                            string_dyn_concat (
                                nicklist,
                                gui_color_get_custom (
                                    gui_color_get_name (
                                        CONFIG_COLOR(ptr_option))),
                                -1);
                        }
                    }
                    else
                    {
                        string_dyn_concat (nicklist,
                                           gui_color_get_custom (
                                               ptr_group->color),
                                           -1);
                    }
                }
                string_dyn_concat (nicklist,
                                   gui_nicklist_get_group_start (
                                       ptr_group->name),
                                   -1);
            }
        }
        gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
    }

    return string_dyn_free (nicklist, 0);
}

/*
 * Bar item with number of window.
 */

char *
gui_bar_item_window_number_cb (const void *pointer, void *data,
                               struct t_gui_bar_item *item,
                               struct t_gui_window *window,
                               struct t_gui_buffer *buffer,
                               struct t_hashtable *extra_info)
{
    char str_number[64];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) buffer;
    (void) extra_info;

    if (!window)
        window = gui_current_window;

    snprintf (str_number, sizeof (str_number), "%d", window->number);

    return strdup (str_number);
}

/*
 * Bar item with mouse status.
 */

char *
gui_bar_item_mouse_status_cb (const void *pointer, void *data,
                              struct t_gui_bar_item *item,
                              struct t_gui_window *window,
                              struct t_gui_buffer *buffer,
                              struct t_hashtable *extra_info)
{
    char str_mouse[512];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer || !gui_mouse_enabled)
        return NULL;

    snprintf (str_mouse, sizeof (str_mouse),
              "%s%s",
              gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_mouse))),
              CONFIG_STRING(config_look_item_mouse_status));

    return strdup (str_mouse);
}

/*
 * Bar item with buffer lag.
 */

char *
gui_bar_item_lag_cb (const void *pointer, void *data,
                     struct t_gui_bar_item *item,
                     struct t_gui_window *window,
                     struct t_gui_buffer *buffer,
                     struct t_hashtable *extra_info)
{
    const char *lag;
    char str_lag[1024];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    lag = (const char *)hashtable_get (buffer->local_variables, "lag");
    if (!lag)
        return NULL;

    snprintf (str_lag, sizeof (str_lag), "%s: %s", _("Lag"), lag);
    return strdup (str_lag);
}

/*
 * Bar item with away message.
 */

char *
gui_bar_item_away_cb (const void *pointer, void *data,
                      struct t_gui_bar_item *item,
                      struct t_gui_window *window,
                      struct t_gui_buffer *buffer,
                      struct t_hashtable *extra_info)
{
    const char *away;
    char *buf, *message;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    away = (const char *)hashtable_get (buffer->local_variables, "away");
    if (!away)
        return NULL;

    buf = NULL;
    message = (CONFIG_BOOLEAN(config_look_item_away_message)) ?
        strdup (away) : strdup (_("away"));
    if (message)
    {
        string_asprintf (
            &buf,
            "%s%s",
            gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_item_away))),
            message);
        free (message);
    }

    return buf;
}

/*
 * Bar item with spacer.
 */

char *
gui_bar_item_spacer_cb (const void *pointer, void *data,
                        struct t_gui_bar_item *item,
                        struct t_gui_window *window,
                        struct t_gui_buffer *buffer,
                        struct t_hashtable *extra_info)
{
    char str_spacer[16];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) buffer;
    (void) extra_info;

    snprintf (str_spacer, sizeof (str_spacer), "%c%c%c",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_BAR_CHAR,
              GUI_COLOR_BAR_SPACER);

    return strdup (str_spacer);
}

/*
 * Focus on nicklist.
 */

struct t_hashtable *
gui_bar_item_focus_buffer_nicklist_cb (const void *pointer,
                                       void *data,
                                       struct t_hashtable *info)
{
    struct t_gui_nick_group *ptr_group;
    struct t_gui_nick *ptr_nick;
    int i, rc, bar_item_line;
    const char *str_window, *str_buffer, *str_bar_item_line;
    struct t_gui_window *window;
    struct t_gui_buffer *buffer;
    char *error;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    str_bar_item_line = hashtable_get (info, "_bar_item_line");
    if (!str_bar_item_line || !str_bar_item_line[0])
        return NULL;

    /* get window */
    str_window = hashtable_get (info, "_window");
    if (str_window && str_window[0])
    {
        rc = sscanf (str_window, "%p", &window);
        if ((rc == EOF) || (rc == 0))
            return NULL;
    }
    else
    {
        /* no window, is it a root bar? then use current window */
        window = gui_current_window;
    }
    if (!window)
        return NULL;

    /* get buffer */
    buffer = window->buffer;
    str_buffer = hashtable_get (info, "_buffer");
    if (str_buffer && str_buffer[0])
    {
        rc = sscanf (str_buffer, "%p", &buffer);
        if ((rc == EOF) || (rc == 0))
            return NULL;
    }
    if (!buffer)
        return NULL;

    error = NULL;
    bar_item_line = (int) strtol (str_bar_item_line, &error, 10);
    if (!error || error[0])
        return NULL;

    i = 0;
    ptr_group = NULL;
    ptr_nick = NULL;
    gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
    while (ptr_group || ptr_nick)
    {
        if ((ptr_nick && ptr_nick->visible)
            || (ptr_group && !ptr_nick
                && buffer->nicklist_display_groups
                && ptr_group->visible))
        {
            if (i == bar_item_line)
                break;
            i++;
        }
        gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
    }

    if (i != bar_item_line)
        return NULL;

    if (ptr_nick)
    {
        hashtable_set (info, "nick", ptr_nick->name);
        hashtable_set (info, "prefix", ptr_nick->prefix);
    }
    else if (ptr_group)
    {
        hashtable_set (info,
                       "group",
                       gui_nicklist_get_group_start (ptr_group->name));
    }
    return info;
}

/*
 * Timer callback for updating time.
 */

int
gui_bar_item_timer_cb (const void *pointer, void *data, int remaining_calls)
{
    static char item_time_text[128] = { '\0' };
    char new_item_time_text[128];

    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    config_get_item_time (new_item_time_text, sizeof (new_item_time_text));

    /*
     * we update item only if it changed since last time
     * for example if time is only hours:minutes, we'll update
     * only when minute changed
     */
    if (strcmp (new_item_time_text, item_time_text) != 0)
    {
        snprintf (item_time_text, sizeof (item_time_text),
                  "%s", new_item_time_text);
        gui_bar_item_update ((char *)pointer);
    }

    return WEECHAT_RC_OK;
}

/*
 * Timer callback for resorting hotlist.
 */

int
gui_bar_item_timer_hotlist_resort_cb (const void *pointer, void *data,
                                      int remaining_calls)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) remaining_calls;

    gui_hotlist_resort ();

    gui_bar_item_timer_hotlist_resort = NULL;

    return WEECHAT_RC_OK;
}
/*
 * Callback when a signal is received: rebuilds an item.
 */

int
gui_bar_item_signal_cb (const void *pointer, void *data,
                        const char *signal,
                        const char *type_data, void *signal_data)
{
    const char *item;

    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    item = (const char *)pointer;
    if (item)
    {
        if ((strcmp (item, "hotlist") == 0)
            && (strcmp (signal, "hotlist_changed") != 0))
        {
            if (!gui_bar_item_timer_hotlist_resort)
            {
                gui_bar_item_timer_hotlist_resort = hook_timer (
                    NULL,
                    1, 0, 1,
                    &gui_bar_item_timer_hotlist_resort_cb, NULL, NULL);
            }
        }
        gui_bar_item_update (item);
    }

    return WEECHAT_RC_OK;
}

/*
 * Hooks a signal to update bar items.
 */

void
gui_bar_item_hook_signal (const char *signal, const char *item)
{
    struct t_gui_bar_item_hook *bar_item_hook;

    bar_item_hook = malloc (sizeof (*bar_item_hook));
    if (bar_item_hook)
    {
        bar_item_hook->hook = hook_signal (NULL, signal,
                                           &gui_bar_item_signal_cb,
                                           (void *)item, NULL);
        bar_item_hook->next_hook = gui_bar_item_hooks;
        gui_bar_item_hooks = bar_item_hook;
    }
}

/*
 * Initializes default items in WeeChat.
 */

void
gui_bar_item_init (void)
{
    char name[128];

    /* input paste */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_INPUT_PASTE],
                      &gui_bar_item_input_paste_cb, NULL, NULL);
    gui_bar_item_hook_signal ("input_paste_pending",
                              gui_bar_item_names[GUI_BAR_ITEM_INPUT_PASTE]);

    /* input prompt */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_INPUT_PROMPT],
                      &gui_bar_item_input_prompt_cb, NULL, NULL);
    gui_bar_item_hook_signal ("window_switch;buffer_switch;input_prompt_changed;"
                              "buffer_localvar_*",
                              gui_bar_item_names[GUI_BAR_ITEM_INPUT_PROMPT]);

    /* input search */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_INPUT_SEARCH],
                      &gui_bar_item_input_search_cb, NULL, NULL);
    gui_bar_item_hook_signal ("window_switch;buffer_switch;input_search;"
                              "input_text_changed",
                              gui_bar_item_names[GUI_BAR_ITEM_INPUT_SEARCH]);

    /* input text */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT],
                      &gui_bar_item_input_text_cb, NULL, NULL);
    gui_bar_item_hook_signal ("window_switch;buffer_switch;input_search;"
                              "input_text_*",
                              gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT]);

    /* time */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_TIME],
                      &gui_bar_item_time_cb, NULL, NULL);
    gui_bar_item_timer = hook_timer (NULL, 1000, 1, 0,
                                     &gui_bar_item_timer_cb,
                                     gui_bar_item_names[GUI_BAR_ITEM_TIME],
                                     NULL);

    /* buffer count */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_COUNT],
                      &gui_bar_item_buffer_count_cb, NULL, NULL);
    gui_bar_item_hook_signal ("buffer_opened;buffer_closed",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_COUNT]);

    /* last buffer number */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_LAST_NUMBER],
                      &gui_bar_item_buffer_last_number_cb, NULL, NULL);
    gui_bar_item_hook_signal ("buffer_opened;buffer_closed;buffer_moved;"
                              "buffer_merged;buffer_unmerged",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_LAST_NUMBER]);

    /* buffer plugin */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_PLUGIN],
                      &gui_bar_item_buffer_plugin_cb, NULL, NULL);
    gui_bar_item_hook_signal ("window_switch;buffer_switch;buffer_renamed",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_PLUGIN]);

    /* buffer number */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NUMBER],
                      &gui_bar_item_buffer_number_cb, NULL, NULL);
    gui_bar_item_hook_signal ("window_switch;buffer_switch;buffer_moved;"
                              "buffer_merged;buffer_unmerged;buffer_closed",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NUMBER]);

    /* buffer name */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NAME],
                      &gui_bar_item_buffer_name_cb, NULL, NULL);
    gui_bar_item_hook_signal ("window_switch;buffer_switch;buffer_renamed;"
                              "buffer_moved",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NAME]);

    /* buffer short name */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_SHORT_NAME],
                      &gui_bar_item_buffer_short_name_cb, NULL, NULL);
    gui_bar_item_hook_signal ("window_switch;buffer_switch;buffer_renamed;"
                              "buffer_moved",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_SHORT_NAME]);

    /* buffer modes */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_MODES],
                      &gui_bar_item_buffer_modes_cb, NULL, NULL);
    gui_bar_item_hook_signal ("window_switch;buffer_switch;buffer_modes_changed",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_MODES]);

    /* buffer filter */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_FILTER],
                      &gui_bar_item_buffer_filter_cb, NULL, NULL);
    gui_bar_item_hook_signal ("window_switch;buffer_switch;buffer_lines_hidden;"
                              "filters_*",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_FILTER]);

    /* buffer zoom */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_ZOOM],
                      &gui_bar_item_buffer_zoom_cb, NULL, NULL);
    gui_bar_item_hook_signal ("buffer_zoomed;buffer_unzoomed;buffer_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_ZOOM]);

    /* buffer nicklist count: nicks displayed */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT],
                      &gui_bar_item_buffer_nicklist_count_cb, NULL, NULL);
    gui_bar_item_hook_signal ("window_switch;buffer_switch;nicklist_*",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT]);

    /* buffer nicklist count: groups displayed */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT_GROUPS],
                      &gui_bar_item_buffer_nicklist_count_groups_cb, NULL, NULL);
    gui_bar_item_hook_signal ("window_switch;buffer_switch;nicklist_*",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT_GROUPS]);

    /* buffer nicklist count: groups + nicks displayed */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT_ALL],
                      &gui_bar_item_buffer_nicklist_count_all_cb, NULL, NULL);
    gui_bar_item_hook_signal ("window_switch;buffer_switch;nicklist_*",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT_ALL]);

    /* scroll indicator */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_SCROLL],
                      &gui_bar_item_scroll_cb, NULL, NULL);
    gui_bar_item_hook_signal ("window_switch;buffer_switch;window_scrolled",
                              gui_bar_item_names[GUI_BAR_ITEM_SCROLL]);

    /* hotlist */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_HOTLIST],
                      &gui_bar_item_hotlist_cb, NULL, NULL);
    gui_bar_item_hook_signal ("hotlist_changed;buffer_moved;buffer_closed;"
                              "buffer_localvar_*",
                              gui_bar_item_names[GUI_BAR_ITEM_HOTLIST]);

    /* completion (possible words when a partial completion occurs) */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_COMPLETION],
                      &gui_bar_item_completion_cb, NULL, NULL);
    gui_bar_item_hook_signal ("partial_completion",
                              gui_bar_item_names[GUI_BAR_ITEM_COMPLETION]);

    /* buffer title */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_TITLE],
                      &gui_bar_item_buffer_title_cb, NULL, NULL);
    gui_bar_item_hook_signal ("window_switch;buffer_switch;buffer_title_changed",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_TITLE]);

    /* buffer nicklist */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST],
                      &gui_bar_item_buffer_nicklist_cb, NULL, NULL);
    gui_bar_item_hook_signal ("nicklist_*;window_switch;buffer_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST]);
    snprintf (name, sizeof (name), "2000|%s",
              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST]);
    hook_focus (NULL, name,
                &gui_bar_item_focus_buffer_nicklist_cb, NULL, NULL);

    /* window number */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_WINDOW_NUMBER],
                      &gui_bar_item_window_number_cb, NULL, NULL);
    gui_bar_item_hook_signal ("window_switch;window_closed",
                              gui_bar_item_names[GUI_BAR_ITEM_WINDOW_NUMBER]);

    /* mouse status */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_MOUSE_STATUS],
                      &gui_bar_item_mouse_status_cb, NULL, NULL);
    gui_bar_item_hook_signal ("mouse_enabled;mouse_disabled",
                              gui_bar_item_names[GUI_BAR_ITEM_MOUSE_STATUS]);

    /* lag */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_LAG],
                      &gui_bar_item_lag_cb, NULL, NULL);
    gui_bar_item_hook_signal ("window_switch;buffer_switch;buffer_localvar_*",
                              gui_bar_item_names[GUI_BAR_ITEM_LAG]);

    /* away message */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_AWAY],
                      &gui_bar_item_away_cb, NULL, NULL);
    gui_bar_item_hook_signal ("buffer_localvar_*",
                              gui_bar_item_names[GUI_BAR_ITEM_AWAY]);

    /* spacer */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_SPACER],
                      &gui_bar_item_spacer_cb, NULL, NULL);
}

/*
 * Removes bar items and hooks.
 */

void
gui_bar_item_end (void)
{
    struct t_gui_bar_item_hook *next_bar_item_hook;

    /* remove hooks */
    while (gui_bar_item_hooks)
    {
        next_bar_item_hook = gui_bar_item_hooks->next_hook;

        unhook (gui_bar_item_hooks->hook);
        free (gui_bar_item_hooks);

        gui_bar_item_hooks = next_bar_item_hook;
    }

    /* remove bar items */
    gui_bar_item_free_all ();
}

/*
 * Return hdata for bar item.
 */

struct t_hdata *
gui_bar_item_hdata_bar_item_cb (const void *pointer, void *data,
                                const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_item", "next_item",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_bar_item, plugin, POINTER, 0, NULL, "plugin");
        HDATA_VAR(struct t_gui_bar_item, name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_item, build_callback, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_item, build_callback_pointer, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_item, build_callback_data, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_item, prev_item, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_bar_item, next_item, POINTER, 0, NULL, hdata_name);
        HDATA_LIST(gui_bar_items, WEECHAT_HDATA_LIST_CHECK_POINTERS);
        HDATA_LIST(last_gui_bar_item, 0);
    }
    return hdata;
}

/*
 * Adds a bar item in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_bar_item_add_to_infolist (struct t_infolist *infolist,
                              struct t_gui_bar_item *bar_item)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !bar_item)
        return 0;

    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_pointer (ptr_item, "plugin", bar_item->plugin))
        return 0;
    if (!infolist_new_var_string (ptr_item, "name", bar_item->name))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "build_callback", bar_item->build_callback))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "build_callback_pointer", (void *)bar_item->build_callback_pointer))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "build_callback_data", bar_item->build_callback_data))
        return 0;

    return 1;
}

/*
 * Prints bar items infos in WeeChat log file (usually for crash dump).
 */

void
gui_bar_item_print_log (void)
{
    struct t_gui_bar_item *ptr_item;

    for (ptr_item = gui_bar_items; ptr_item; ptr_item = ptr_item->next_item)
    {
        log_printf ("");
        log_printf ("[bar item (addr:%p)]", ptr_item);
        log_printf ("  plugin . . . . . . . . : %p ('%s')",
                    ptr_item->plugin, plugin_get_name (ptr_item->plugin));
        log_printf ("  name . . . . . . . . . : '%s'", ptr_item->name);
        log_printf ("  build_callback . . . . : %p", ptr_item->build_callback);
        log_printf ("  build_callback_pointer : %p", ptr_item->build_callback_pointer);
        log_printf ("  build_callback_data. . : %p", ptr_item->build_callback_data);
        log_printf ("  prev_item. . . . . . . : %p", ptr_item->prev_item);
        log_printf ("  next_item. . . . . . . : %p", ptr_item->next_item);
    }
}
