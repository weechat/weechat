/*
 * gui-bar-item.c - bar item functions (used by all GUI)
 *
 * Copyright (C) 2003-2014 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stddef.h>
#include <string.h>
#include <time.h>

#include "../core/weechat.h"
#include "../core/wee-arraylist.h"
#include "../core/wee-config.h"
#include "../core/wee-hashtable.h"
#include "../core/wee-hdata.h"
#include "../core/wee-hook.h"
#include "../core/wee-infolist.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
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
  "buffer_zoom", "buffer_nicklist_count", "scroll", "hotlist", "completion",
  "buffer_title", "buffer_nicklist", "window_number", "mouse_status"
};
char *gui_bar_items_default_for_bars[][2] =
{ { GUI_BAR_DEFAULT_NAME_INPUT,
    "[input_prompt]+(away),[input_search],[input_paste],input_text" },
  { GUI_BAR_DEFAULT_NAME_TITLE,
    "buffer_title" },
  { GUI_BAR_DEFAULT_NAME_STATUS,
    "[time],[buffer_last_number],[buffer_plugin],buffer_number+:+"
    "buffer_name+(buffer_modes)+{buffer_nicklist_count}+buffer_zoom+"
    "buffer_filter,scroll,[lag],[hotlist],completion" },
  { GUI_BAR_DEFAULT_NAME_NICKLIST,
    "buffer_nicklist" },
  { NULL,
    NULL },
};
struct t_gui_bar_item_hook *gui_bar_item_hooks = NULL;
struct t_hook *gui_bar_item_timer = NULL;

struct t_hdata *gui_bar_item_hdata_bar_item = NULL;


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
 */

char *
gui_bar_item_get_value (struct t_gui_bar *bar, struct t_gui_window *window,
                        int item, int subitem)
{
    char *item_value, delimiter_color[32], bar_color[32];
    char *result, str_attr[8];
    int length;
    struct t_gui_buffer *buffer;
    struct t_gui_bar_item *ptr_item;

    if (!bar->items_array[item][subitem])
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
            item_value = (ptr_item->build_callback) (ptr_item->build_callback_data,
                                                     ptr_item, window, buffer,
                                                     NULL);
        }
        if (item_value && !item_value[0])
        {
            free (item_value);
            item_value = NULL;
        }
        if (!item_value)
            return NULL;
    }

    length = 0;
    if (bar->items_prefix[item][subitem])
        length += 64 + strlen (bar->items_prefix[item][subitem]) + 1; /* color + prefix + color */
    if (item_value && item_value[0])
        length += strlen (item_value) + 1;
    if (bar->items_suffix[item][subitem])
        length += 32 + strlen (bar->items_suffix[item][subitem]) + 1; /* color + suffix */

    result = NULL;
    if (length > 0)
    {
        result = malloc (length);
        if (result)
        {
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
            snprintf (result, length,
                      "%s%s%s%s%s%s",
                      (bar->items_prefix[item][subitem]) ? delimiter_color : "",
                      (bar->items_prefix[item][subitem]) ? bar->items_prefix[item][subitem] : "",
                      (bar->items_prefix[item][subitem]) ? bar_color : "",
                      (item_value) ? item_value : "",
                      (bar->items_suffix[item][subitem]) ? delimiter_color : "",
                      (bar->items_suffix[item][subitem]) ? bar->items_suffix[item][subitem] : "");
        }
    }

    if (item_value)
        free (item_value);

    return result;
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
                  char *(*build_callback)(void *data,
                                          struct t_gui_bar_item *item,
                                          struct t_gui_window *window,
                                          struct t_gui_buffer *buffer,
                                          struct t_hashtable *extra_info),
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
        new_bar_item->build_callback_data = build_callback_data;

        /* add bar item to bar items queue */
        new_bar_item->prev_item = last_gui_bar_item;
        if (gui_bar_items)
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
    int i, j;

    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        for (i = 0; i < ptr_bar->items_count; i++)
        {
            for (j = 0; j < ptr_bar->items_subcount[i]; j++)
            {
                if (ptr_bar->items_name[i][j]
                    && (strcmp (ptr_bar->items_name[i][j], item_name) == 0))
                {
                    if (ptr_bar->bar_window)
                    {
                        ptr_bar->bar_window->items_refresh_needed[i][j] = 1;
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
    }
}

/*
 * Deletes a bar item.
 */

void
gui_bar_item_free (struct t_gui_bar_item *item)
{
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
    if (item->name)
        free (item->name);

    free (item);
}

/*
 * Deletes all bar items.
 */

void
gui_bar_item_free_all ()
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
 * Default item for input paste question.
 */

char *
gui_bar_item_default_input_paste (void *data, struct t_gui_bar_item *item,
                                  struct t_gui_window *window,
                                  struct t_gui_buffer *buffer,
                                  struct t_hashtable *extra_info)
{
    char str_paste[1024];
    int lines;

    /* make C compiler happy */
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
              NG_("%sPaste %d line ? [ctrl-Y] Yes [ctrl-N] No",
                  "%sPaste %d lines ? [ctrl-Y] Yes [ctrl-N] No",
                  lines),
              gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_input_actions))),
              lines);
    return strdup (str_paste);
}

/*
 * Default item for input prompt.
 */

char *
gui_bar_item_default_input_prompt (void *data, struct t_gui_bar_item *item,
                                   struct t_gui_window *window,
                                   struct t_gui_buffer *buffer,
                                   struct t_hashtable *extra_info)
{
    const char *nick;

    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    nick = (const char *)hashtable_get (buffer->local_variables, "nick");

    return (nick) ? strdup (nick) : NULL;
}

/*
 * Default item for input search status.
 */

char *
gui_bar_item_default_input_search (void *data, struct t_gui_bar_item *item,
                                   struct t_gui_window *window,
                                   struct t_gui_buffer *buffer,
                                   struct t_hashtable *extra_info)
{
    char str_search[1024];

    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    if (buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
        return NULL;

    snprintf (str_search, sizeof (str_search), "%s%s (%s %s,%s%s%s)",
              (buffer->text_search_found
               || !buffer->input_buffer
               || !buffer->input_buffer[0]) ?
              GUI_COLOR_CUSTOM_BAR_FG :
              gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_input_text_not_found))),
              _("Search"),
              (buffer->text_search_exact) ? "==" : "~",
              (buffer->text_search_regex) ? "regex" : "str",
              (buffer->text_search_where & GUI_TEXT_SEARCH_IN_PREFIX) ? "pre" : "",
              ((buffer->text_search_where & GUI_TEXT_SEARCH_IN_PREFIX)
               && (buffer->text_search_where & GUI_TEXT_SEARCH_IN_MESSAGE)) ? "|" : "",
              (buffer->text_search_where & GUI_TEXT_SEARCH_IN_MESSAGE) ? "msg" : "");

    return strdup (str_search);
}

/*
 * Default item for input text.
 */

char *
gui_bar_item_default_input_text (void *data, struct t_gui_bar_item *item,
                                 struct t_gui_window *window,
                                 struct t_gui_buffer *buffer,
                                 struct t_hashtable *extra_info)
{
    char *ptr_input, *ptr_input2, str_buffer[128], str_start_input[16];
    char str_cursor[16], *buf;
    const char *pos_cursor;
    int length, length_cursor, length_start_input, buf_pos;

    /* make C compiler happy */
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
    length_start_input = strlen (str_start_input);

    /* for modifiers */
    snprintf (str_buffer, sizeof (str_buffer),
              "0x%lx", (long unsigned int)buffer);

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
        if (ptr_input)
            free (ptr_input);
        ptr_input = ptr_input2;
    }

    /* insert "start input" at beginning of string */
    if (ptr_input)
    {
        length = strlen (ptr_input) + length_start_input + 1;
        buf = malloc (length);
        if (buf)
        {
            snprintf (buf, length, "%s%s", str_start_input, ptr_input);
            free (ptr_input);
            ptr_input = buf;
        }
    }
    else
    {
        length = length_start_input + length_cursor + 1;
        ptr_input = malloc (length);
        if (ptr_input)
        {
            snprintf (ptr_input, length, "%s%s", str_start_input, str_cursor);
        }
    }

    return ptr_input;
}

/*
 * Default item for time.
 */

char *
gui_bar_item_default_time (void *data, struct t_gui_bar_item *item,
                           struct t_gui_window *window,
                           struct t_gui_buffer *buffer,
                           struct t_hashtable *extra_info)
{
    time_t date;
    struct tm *local_time;
    char text_time[128], text_time2[128];

    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) window;
    (void) buffer;
    (void) extra_info;

    date = time (NULL);
    local_time = localtime (&date);
    if (strftime (text_time, sizeof (text_time),
                  CONFIG_STRING(config_look_item_time_format),
                  local_time) == 0)
        return NULL;

    snprintf (text_time2, sizeof (text_time2), "%s%s",
              gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_time))),
              text_time);

    return strdup (text_time2);
}

/*
 * Default item for number of buffers.
 */

char *
gui_bar_item_default_buffer_count (void *data, struct t_gui_bar_item *item,
                                   struct t_gui_window *window,
                                   struct t_gui_buffer *buffer,
                                   struct t_hashtable *extra_info)
{
    char buf[32];

    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) window;
    (void) buffer;
    (void) extra_info;

    snprintf (buf, sizeof (buf), "%d", gui_buffers_count);

    return strdup (buf);
}

/*
 * Default item for last buffer number.
 */

char *
gui_bar_item_default_buffer_last_number (void *data,
                                         struct t_gui_bar_item *item,
                                         struct t_gui_window *window,
                                         struct t_gui_buffer *buffer,
                                         struct t_hashtable *extra_info)
{
    char buf[32];

    /* make C compiler happy */
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
 * Default item for name of buffer plugin.
 */

char *
gui_bar_item_default_buffer_plugin (void *data, struct t_gui_bar_item *item,
                                    struct t_gui_window *window,
                                    struct t_gui_buffer *buffer,
                                    struct t_hashtable *extra_info)
{
    const char *plugin_name;

    /* make C compiler happy */
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
 * Default item for number of buffer.
 */

char *
gui_bar_item_default_buffer_number (void *data, struct t_gui_bar_item *item,
                                    struct t_gui_window *window,
                                    struct t_gui_buffer *buffer,
                                    struct t_hashtable *extra_info)
{
    char str_number[64];

    /* make C compiler happy */
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
 * Default item for name of buffer.
 */

char *
gui_bar_item_default_buffer_name (void *data, struct t_gui_bar_item *item,
                                  struct t_gui_window *window,
                                  struct t_gui_buffer *buffer,
                                  struct t_hashtable *extra_info)
{
    char str_name[256];

    /* make C compiler happy */
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
 * Default item for short name of buffer.
 */

char *
gui_bar_item_default_buffer_short_name (void *data,
                                        struct t_gui_bar_item *item,
                                        struct t_gui_window *window,
                                        struct t_gui_buffer *buffer,
                                        struct t_hashtable *extra_info)
{
    char str_short_name[256];

    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    snprintf (str_short_name, sizeof (str_short_name), "%s%s",
              gui_color_get_custom (
                  gui_color_get_name (CONFIG_COLOR(config_color_status_name))),
              gui_buffer_get_short_name (buffer));

    return strdup (str_short_name);
}

/*
 * Default item for modes of buffer.
 *
 * Note: this bar item is empty for WeeChat core, this is used only by plugins
 * like irc to display channel modes.
 */

char *
gui_bar_item_default_buffer_modes (void *data, struct t_gui_bar_item *item,
                                   struct t_gui_window *window,
                                   struct t_gui_buffer *buffer,
                                   struct t_hashtable *extra_info)
{
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) window;
    (void) buffer;
    (void) extra_info;

    return NULL;
}

/*
 * Default item for buffer filter.
 */

char *
gui_bar_item_default_buffer_filter (void *data, struct t_gui_bar_item *item,
                                    struct t_gui_window *window,
                                    struct t_gui_buffer *buffer,
                                    struct t_hashtable *extra_info)
{
    char str_filter[512];

    /* make C compiler happy */
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
 * Default item for number of nicks in buffer nicklist.
 */

char *
gui_bar_item_default_buffer_nicklist_count (void *data,
                                            struct t_gui_bar_item *item,
                                            struct t_gui_window *window,
                                            struct t_gui_buffer *buffer,
                                            struct t_hashtable *extra_info)
{
    char str_count[64];

    /* make C compiler happy */
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
 * Default item for zoom on merged buffer.
 */

char *
gui_bar_item_buffer_zoom (void *data, struct t_gui_bar_item *item,
                          struct t_gui_window *window,
                          struct t_gui_buffer *buffer,
                          struct t_hashtable *extra_info)

{
    char buf[512];

    /* make C compiler happy */
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
 * Default item for scrolling indicator.
 */

char *
gui_bar_item_default_scroll (void *data, struct t_gui_bar_item *item,
                             struct t_gui_window *window,
                             struct t_gui_buffer *buffer,
                             struct t_hashtable *extra_info)
{
    char str_scroll[512];

    /* make C compiler happy */
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
 * Default item for hotlist.
 */

char *
gui_bar_item_default_hotlist (void *data, struct t_gui_bar_item *item,
                              struct t_gui_window *window,
                              struct t_gui_buffer *buffer,
                              struct t_hashtable *extra_info)
{
    char str_hotlist[4096], format[32], *buffer_without_name_displayed;
    const char *hotlist_suffix;
    struct t_gui_hotlist *ptr_hotlist;
    int numbers_count, names_count, display_name, count_max;
    int priority, priority_min, priority_min_displayed, private;

    /* make C compiler happy */
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
    {
        buffer_without_name_displayed = malloc (last_gui_buffer->number);
        if (buffer_without_name_displayed)
            memset (buffer_without_name_displayed, 0, last_gui_buffer->number);
    }

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
                if (CONFIG_INTEGER(config_look_hotlist_names_length) == 0)
                {
                    snprintf (format, sizeof (format), "%%s");
                }
                else
                {
                    snprintf (format, sizeof (format),
                              "%%.%ds",
                              CONFIG_INTEGER(config_look_hotlist_names_length));
                }
                snprintf (str_hotlist + strlen (str_hotlist), 128, format,
                          (CONFIG_BOOLEAN(config_look_hotlist_short_names)) ?
                          gui_buffer_get_short_name (ptr_hotlist->buffer) : ptr_hotlist->buffer->name);
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

    if (buffer_without_name_displayed)
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
 * Default item for (partial) completion.
 */

char *
gui_bar_item_default_completion (void *data, struct t_gui_bar_item *item,
                                 struct t_gui_window *window,
                                 struct t_gui_buffer *buffer,
                                 struct t_hashtable *extra_info)
{
    int length, i;
    char *buf, str_number[64];
    struct t_gui_completion_word *ptr_completion_word;

    /* make C compiler happy */
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
 * Default item for buffer title.
 */

char *
gui_bar_item_default_buffer_title (void *data, struct t_gui_bar_item *item,
                                   struct t_gui_window *window,
                                   struct t_gui_buffer *buffer,
                                   struct t_hashtable *extra_info)
{
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    return (buffer->title) ? strdup (buffer->title) : NULL;
}

/*
 * Default item for nicklist.
 */

char *
gui_bar_item_default_buffer_nicklist (void *data, struct t_gui_bar_item *item,
                                      struct t_gui_window *window,
                                      struct t_gui_buffer *buffer,
                                      struct t_hashtable *extra_info)
{
    struct t_gui_nick_group *ptr_group;
    struct t_gui_nick *ptr_nick;
    struct t_config_option *ptr_option;
    int i, length;
    char *str_nicklist;

    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    length = 1;
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
            if (ptr_nick)
                length += ptr_nick->group->level + 16 /* color */
                    + ((ptr_nick->prefix) ? strlen (ptr_nick->prefix) : 0)
                    + 16 /* color */
                    + strlen (ptr_nick->name) + 1;
            else
                length += ptr_group->level - 1
                    + 16 /* color */
                    + strlen (gui_nicklist_get_group_start (ptr_group->name))
                    + 1;
        }
        gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
    }

    str_nicklist = malloc (length);
    if (str_nicklist)
    {
        str_nicklist[0] = '\0';
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
                if (str_nicklist[0])
                    strcat (str_nicklist, "\n");

                if (ptr_nick)
                {
                    if (buffer->nicklist_display_groups)
                    {
                        for (i = 0; i < ptr_nick->group->level; i++)
                        {
                            strcat (str_nicklist, " ");
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
                                strcat (str_nicklist, gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(ptr_option))));
                        }
                        else
                        {
                            strcat (str_nicklist, gui_color_get_custom (ptr_nick->prefix_color));
                        }
                    }
                    if (ptr_nick->prefix)
                        strcat (str_nicklist, ptr_nick->prefix);
                    if (ptr_nick->color)
                    {
                        if (strchr (ptr_nick->color, '.'))
                        {
                            config_file_search_with_string (ptr_nick->color,
                                                            NULL, NULL, &ptr_option,
                                                            NULL);
                            if (ptr_option)
                                strcat (str_nicklist, gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(ptr_option))));
                        }
                        else
                        {
                            strcat (str_nicklist, gui_color_get_custom (ptr_nick->color));
                        }
                    }
                    strcat (str_nicklist, ptr_nick->name);
                }
                else
                {
                    for (i = 0; i < ptr_group->level - 1; i++)
                    {
                        strcat (str_nicklist, " ");
                    }
                    if (ptr_group->color)
                    {
                        if (strchr (ptr_group->color, '.'))
                        {
                            config_file_search_with_string (ptr_group->color,
                                                            NULL, NULL, &ptr_option,
                                                            NULL);
                            if (ptr_option)
                                strcat (str_nicklist, gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(ptr_option))));
                        }
                        else
                        {
                            strcat (str_nicklist, gui_color_get_custom (ptr_group->color));
                        }
                    }
                    strcat (str_nicklist, gui_nicklist_get_group_start (ptr_group->name));
                }
            }
            gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
        }
    }

    return str_nicklist;
}

/*
 * Default item for number of window.
 */

char *
gui_bar_item_default_window_number (void *data, struct t_gui_bar_item *item,
                                    struct t_gui_window *window,
                                    struct t_gui_buffer *buffer,
                                    struct t_hashtable *extra_info)
{
    char str_number[64];

    /* make C compiler happy */
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
 * Default item for mouse status.
 */

char *
gui_bar_item_default_mouse_status   (void *data, struct t_gui_bar_item *item,
                                    struct t_gui_window *window,
                                    struct t_gui_buffer *buffer,
                                    struct t_hashtable *extra_info)
{
    char str_mouse[512];

    /* make C compiler happy */
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
 * Focus on nicklist.
 */

struct t_hashtable *
gui_bar_item_focus_buffer_nicklist (void *data,
                                    struct t_hashtable *info)
{
    struct t_gui_nick_group *ptr_group;
    struct t_gui_nick *ptr_nick;
    int i, rc, bar_item_line;
    unsigned long int value;
    const char *str_window, *str_buffer, *str_bar_item_line;
    struct t_gui_window *window;
    struct t_gui_buffer *buffer;
    char *error;

    /* make C compiler happy */
    (void) data;

    str_bar_item_line = hashtable_get (info, "_bar_item_line");
    if (!str_bar_item_line || !str_bar_item_line[0])
        return NULL;

    /* get window */
    str_window = hashtable_get (info, "_window");
    if (str_window && str_window[0])
    {
        rc = sscanf (str_window, "%lx", &value);
        if ((rc == EOF) || (rc == 0))
            return NULL;
        window = (struct t_gui_window *)value;
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
        rc = sscanf (str_buffer, "%lx", &value);
        if ((rc == EOF) || (rc == 0))
            return NULL;
        buffer = (struct t_gui_buffer *)value;
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
gui_bar_item_timer_cb (void *data, int remaining_calls)
{
    time_t date;
    struct tm *local_time;
    static char item_time_text[128] = { '\0' };
    char new_item_time_text[128];

    /* make C compiler happy */
    (void) remaining_calls;

    date = time (NULL);
    local_time = localtime (&date);
    if (strftime (new_item_time_text, sizeof (new_item_time_text),
                  CONFIG_STRING(config_look_item_time_format),
                  local_time) == 0)
        return WEECHAT_RC_OK;

    /*
     * we update item only if it changed since last time
     * for example if time is only hours:minutes, we'll update
     * only when minute changed
     */
    if (strcmp (new_item_time_text, item_time_text) != 0)
    {
        snprintf (item_time_text, sizeof (item_time_text),
                  "%s", new_item_time_text);
        gui_bar_item_update ((char *)data);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback when a signal is received: rebuilds an item.
 */

int
gui_bar_item_signal_cb (void *data, const char *signal,
                        const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) signal;
    (void) type_data;
    (void) signal_data;

    gui_bar_item_update ((char *)data);

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
                                           (void *)item);
        bar_item_hook->next_hook = gui_bar_item_hooks;
        gui_bar_item_hooks = bar_item_hook;
    }
}

/*
 * Initializes default items in WeeChat.
 */

void
gui_bar_item_init ()
{
    char name[128];

    /* input paste */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_INPUT_PASTE],
                      &gui_bar_item_default_input_paste, NULL);
    gui_bar_item_hook_signal ("input_paste_pending",
                              gui_bar_item_names[GUI_BAR_ITEM_INPUT_PASTE]);

    /* input prompt */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_INPUT_PROMPT],
                      &gui_bar_item_default_input_prompt, NULL);
    gui_bar_item_hook_signal ("window_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_INPUT_PROMPT]);
    gui_bar_item_hook_signal ("buffer_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_INPUT_PROMPT]);
    gui_bar_item_hook_signal ("buffer_localvar_*",
                              gui_bar_item_names[GUI_BAR_ITEM_INPUT_PROMPT]);

    /* input search */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_INPUT_SEARCH],
                      &gui_bar_item_default_input_search, NULL);
    gui_bar_item_hook_signal ("window_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_INPUT_SEARCH]);
    gui_bar_item_hook_signal ("input_search",
                              gui_bar_item_names[GUI_BAR_ITEM_INPUT_SEARCH]);
    gui_bar_item_hook_signal ("input_text_changed",
                              gui_bar_item_names[GUI_BAR_ITEM_INPUT_SEARCH]);

    /* input text */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT],
                      &gui_bar_item_default_input_text, NULL);
    gui_bar_item_hook_signal ("window_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT]);
    gui_bar_item_hook_signal ("buffer_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT]);
    gui_bar_item_hook_signal ("input_text_*",
                              gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT]);

    /* time */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_TIME],
                      &gui_bar_item_default_time, NULL);
    gui_bar_item_timer = hook_timer (NULL, 1000, 1, 0, &gui_bar_item_timer_cb,
                                     gui_bar_item_names[GUI_BAR_ITEM_TIME]);

    /* buffer count */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_COUNT],
                      &gui_bar_item_default_buffer_count, NULL);
    gui_bar_item_hook_signal ("buffer_opened",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_COUNT]);
    gui_bar_item_hook_signal ("buffer_closed",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_COUNT]);

    /* last buffer number */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_LAST_NUMBER],
                      &gui_bar_item_default_buffer_last_number, NULL);
    gui_bar_item_hook_signal ("buffer_opened",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_LAST_NUMBER]);
    gui_bar_item_hook_signal ("buffer_closed",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_LAST_NUMBER]);
    gui_bar_item_hook_signal ("buffer_moved",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_LAST_NUMBER]);
    gui_bar_item_hook_signal ("buffer_merged",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_LAST_NUMBER]);
    gui_bar_item_hook_signal ("buffer_unmerged",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_LAST_NUMBER]);

    /* buffer plugin */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_PLUGIN],
                      &gui_bar_item_default_buffer_plugin, NULL);
    gui_bar_item_hook_signal ("window_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_PLUGIN]);
    gui_bar_item_hook_signal ("buffer_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_PLUGIN]);
    gui_bar_item_hook_signal ("buffer_renamed",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_PLUGIN]);

    /* buffer number */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NUMBER],
                      &gui_bar_item_default_buffer_number, NULL);
    gui_bar_item_hook_signal ("window_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NUMBER]);
    gui_bar_item_hook_signal ("buffer_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NUMBER]);
    gui_bar_item_hook_signal ("buffer_moved",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NUMBER]);
    gui_bar_item_hook_signal ("buffer_merged",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NUMBER]);
    gui_bar_item_hook_signal ("buffer_unmerged",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NUMBER]);
    gui_bar_item_hook_signal ("buffer_closed",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NUMBER]);

    /* buffer name */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NAME],
                      &gui_bar_item_default_buffer_name, NULL);
    gui_bar_item_hook_signal ("window_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NAME]);
    gui_bar_item_hook_signal ("buffer_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NAME]);
    gui_bar_item_hook_signal ("buffer_renamed",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NAME]);
    gui_bar_item_hook_signal ("buffer_moved",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NAME]);

    /* buffer short name */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_SHORT_NAME],
                      &gui_bar_item_default_buffer_short_name, NULL);
    gui_bar_item_hook_signal ("window_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_SHORT_NAME]);
    gui_bar_item_hook_signal ("buffer_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_SHORT_NAME]);
    gui_bar_item_hook_signal ("buffer_renamed",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_SHORT_NAME]);
    gui_bar_item_hook_signal ("buffer_moved",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_SHORT_NAME]);

    /* buffer modes */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_MODES],
                      &gui_bar_item_default_buffer_modes, NULL);
    gui_bar_item_hook_signal ("window_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_MODES]);
    gui_bar_item_hook_signal ("buffer_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_MODES]);

    /* buffer filter */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_FILTER],
                      &gui_bar_item_default_buffer_filter, NULL);
    gui_bar_item_hook_signal ("window_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_FILTER]);
    gui_bar_item_hook_signal ("buffer_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_FILTER]);
    gui_bar_item_hook_signal ("buffer_lines_hidden",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_FILTER]);
    gui_bar_item_hook_signal ("filters_*",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_FILTER]);

    /* buffer zoom */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_ZOOM],
                      &gui_bar_item_buffer_zoom, NULL);
    gui_bar_item_hook_signal ("buffer_zoomed",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_ZOOM]);
    gui_bar_item_hook_signal ("buffer_unzoomed",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_ZOOM]);
    gui_bar_item_hook_signal ("buffer_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_ZOOM]);

    /* buffer nicklist count */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT],
                      &gui_bar_item_default_buffer_nicklist_count, NULL);
    gui_bar_item_hook_signal ("window_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT]);
    gui_bar_item_hook_signal ("buffer_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT]);
    gui_bar_item_hook_signal ("nicklist_*",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT]);

    /* scroll indicator */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_SCROLL],
                      &gui_bar_item_default_scroll, NULL);
    gui_bar_item_hook_signal ("window_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_SCROLL]);
    gui_bar_item_hook_signal ("buffer_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_SCROLL]);
    gui_bar_item_hook_signal ("window_scrolled",
                              gui_bar_item_names[GUI_BAR_ITEM_SCROLL]);

    /* hotlist */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_HOTLIST],
                      &gui_bar_item_default_hotlist, NULL);
    gui_bar_item_hook_signal ("hotlist_changed",
                              gui_bar_item_names[GUI_BAR_ITEM_HOTLIST]);
    gui_bar_item_hook_signal ("buffer_moved",
                              gui_bar_item_names[GUI_BAR_ITEM_HOTLIST]);
    gui_bar_item_hook_signal ("buffer_closed",
                              gui_bar_item_names[GUI_BAR_ITEM_HOTLIST]);

    /* completion (possible words when a partial completion occurs) */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_COMPLETION],
                      &gui_bar_item_default_completion, NULL);
    gui_bar_item_hook_signal ("partial_completion",
                              gui_bar_item_names[GUI_BAR_ITEM_COMPLETION]);

    /* buffer title */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_TITLE],
                      &gui_bar_item_default_buffer_title, NULL);
    gui_bar_item_hook_signal ("window_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_TITLE]);
    gui_bar_item_hook_signal ("buffer_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_TITLE]);
    gui_bar_item_hook_signal ("buffer_title_changed",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_TITLE]);

    /* buffer nicklist */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST],
                      &gui_bar_item_default_buffer_nicklist, NULL);
    gui_bar_item_hook_signal ("nicklist_*",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST]);
    gui_bar_item_hook_signal ("window_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST]);
    gui_bar_item_hook_signal ("buffer_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST]);
    snprintf (name, sizeof (name), "2000|%s",
              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST]);
    hook_focus (NULL, name, &gui_bar_item_focus_buffer_nicklist, NULL);

    /* window number */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_WINDOW_NUMBER],
                      &gui_bar_item_default_window_number, NULL);
    gui_bar_item_hook_signal ("window_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_WINDOW_NUMBER]);
    gui_bar_item_hook_signal ("window_closed",
                              gui_bar_item_names[GUI_BAR_ITEM_WINDOW_NUMBER]);

    /* mouse status */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_MOUSE_STATUS],
                      &gui_bar_item_default_mouse_status, NULL);
    gui_bar_item_hook_signal ("mouse_enabled",
                              gui_bar_item_names[GUI_BAR_ITEM_MOUSE_STATUS]);
    gui_bar_item_hook_signal ("mouse_disabled",
                              gui_bar_item_names[GUI_BAR_ITEM_MOUSE_STATUS]);
}

/*
 * Removes bar items and hooks.
 */

void
gui_bar_item_end ()
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
gui_bar_item_hdata_bar_item_cb (void *data, const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_item", "next_item",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_bar_item, plugin, POINTER, 0, NULL, "plugin");
        HDATA_VAR(struct t_gui_bar_item, name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_item, build_callback, POINTER, 0, NULL, NULL);
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
    if (!infolist_new_var_pointer (ptr_item, "build_callback_data", bar_item->build_callback_data))
        return 0;

    return 1;
}

/*
 * Prints bar items infos in WeeChat log file (usually for crash dump).
 */

void
gui_bar_item_print_log ()
{
    struct t_gui_bar_item *ptr_item;

    for (ptr_item = gui_bar_items; ptr_item; ptr_item = ptr_item->next_item)
    {
        log_printf ("");
        log_printf ("[bar item (addr:0x%lx)]", ptr_item);
        log_printf ("  plugin . . . . . . . . : 0x%lx ('%s')",
                    ptr_item->plugin, plugin_get_name (ptr_item->plugin));
        log_printf ("  name . . . . . . . . . : '%s'",  ptr_item->name);
        log_printf ("  build_callback . . . . : 0x%lx", ptr_item->build_callback);
        log_printf ("  build_callback_data. . : 0x%lx", ptr_item->build_callback_data);
        log_printf ("  prev_item. . . . . . . : 0x%lx", ptr_item->prev_item);
        log_printf ("  next_item. . . . . . . : 0x%lx", ptr_item->next_item);
    }
}
