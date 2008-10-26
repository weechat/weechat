/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* gui-bar-item.c: bar item functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hook.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../plugins/plugin.h"
#include "gui-bar-item.h"
#include "gui-bar.h"
#include "gui-buffer.h"
#include "gui-chat.h"
#include "gui-color.h"
#include "gui-completion.h"
#include "gui-filter.h"
#include "gui-hotlist.h"
#include "gui-keyboard.h"
#include "gui-nicklist.h"
#include "gui-window.h"


struct t_gui_bar_item *gui_bar_items = NULL;     /* first bar item          */
struct t_gui_bar_item *last_gui_bar_item = NULL; /* last bar item           */
char *gui_bar_item_names[GUI_BAR_NUM_ITEMS] =
{ "input_paste", "input_prompt", "input_search", "input_text", "time",
  "buffer_count", "buffer_plugin", "buffer_name", "buffer_filter",
  "buffer_nicklist_count", "scroll", "hotlist", "completion", "buffer_title",
  "buffer_nicklist"
};
struct t_gui_bar_item_hook *gui_bar_item_hooks = NULL;
struct t_hook *gui_bar_item_timer = NULL;


/*
 * gui_bar_item_search: search a bar item
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
 * gui_bar_item_valid_char_name: return 1 if char is valid for item name
 *                               (any letter, digit, "-" or "_")
 *                               return 0 otherwise
 */

int
gui_bar_item_valid_char_name (char c)
{
    return  (((c >= 'a') && (c <= 'z'))
             || ((c >= 'A') && (c <= 'Z'))
             || ((c >= '0') && (c <= '9'))
             || (c == '-')
             || (c == '_')) ?
        1 : 0;
}

/*
 * gui_bar_item_string_get_item_start: return pointer to beginning of item name
 *                                     (string may contain special delimiters
 *                                     at beginning, which are ignored)
 */

const char *
gui_bar_item_string_get_item_start (const char *string)
{
    while (string && string[0])
    {
        if (gui_bar_item_valid_char_name (string[0]))
            break;
        string++;
    }
    if (string && string[0])
        return string;

    return NULL;
}

/*
 * gui_bar_item_string_is_item: return 1 if string is item (string may contain
 *                              special delimiters at beginning and end of
 *                              string, which are ignored)
 */

int
gui_bar_item_string_is_item (const char *string, const char *item_name)
{
    const char *item_start;
    int length;

    item_start = gui_bar_item_string_get_item_start (string);
    if (!item_start)
        return 0;
    
    length = strlen (item_name);
    if (strncmp (item_start, item_name, length) == 0)
    {
        if (!gui_bar_item_valid_char_name (item_start[length]))
            return 1;
    }
    
    return 0;
}

/*
 * gui_bar_item_search_with_plugin: search a bar item for a plugin
 *                                  if exact_plugin == 1, then search only for
 *                                  this plugin, otherwise, if plugin is not
 *                                  found, function may return item for core
 *                                  (plugin == NULL)
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
 * gui_bar_contains_item: return 1 if a bar contains item, O otherwise
 */

int
gui_bar_contains_item (struct t_gui_bar *bar, const char *item_name)
{
    int i;
    
    if (!bar || !item_name || !item_name[0])
        return 0;
    
    for (i = 0; i < bar->items_count; i++)
    {
        /* skip non letters chars at beginning (prefix) */
        if (gui_bar_item_string_is_item (bar->items_array[i], item_name))
            return 1;
    }
    
    /* item is not in bar */
    return 0;
}

/*
 * gui_bar_item_used_in_a_bar: return 1 if an item is used in at least one bar
 *                             if partial_name == 1, then search a bar that
 *                             contains item beginning with "item_name"
 */

int
gui_bar_item_used_in_a_bar (const char *item_name, int partial_name)
{
    struct t_gui_bar *ptr_bar;
    int i, length;
    const char *ptr_start;
    
    length = strlen (item_name);
    
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        for (i = 0; i < ptr_bar->items_count; i++)
        {
            ptr_start = gui_bar_item_string_get_item_start (ptr_bar->items_array[i]);
            if (ptr_start)
            {
                if ((partial_name
                     && strncmp (ptr_start, item_name, length) == 0)
                    || (!partial_name
                        && strcmp (ptr_start, item_name) == 0))
                {
                    return 1;
                }
            }
        }
    }
    
    /* item not used by any bar */
    return 0;
}

/*
 * gui_bar_item_input_text_update_for_display: update input text item for
 *                                             display:
 *                                             - scroll if needed, to see only
 *                                               the end of input
 *                                             - insert "move cursor" id to
 *                                               move cursor to good position
 */

char *
gui_bar_item_input_text_update_for_display (const char *item_content,
                                            struct t_gui_window *window,
                                            int chars_available)
{
    char *buf, str_cursor[16];
    const char *pos_cursor, *ptr_start;
    int diff, buf_pos;
    int length, length_screen_before_cursor, length_screen_after_cursor;
    int total_length_screen;
    
    snprintf (str_cursor, sizeof (str_cursor), "%c%c%c",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_BAR_CHAR,
              GUI_COLOR_BAR_MOVE_CURSOR_CHAR);
    
    ptr_start = item_content;
    
    pos_cursor = gui_chat_string_add_offset (item_content,
                                             window->buffer->input_buffer_pos);
    
    /* if bar size is fixed (chars_available > 0), then truncate it at
       beginning if needed */
    if (chars_available > 0)
    {
        length_screen_before_cursor = -1;
        length_screen_after_cursor = -1;
        if (pos_cursor && (pos_cursor > item_content))
        {
            buf = string_strndup (item_content, pos_cursor - item_content);
            if (buf)
            {
                length_screen_before_cursor = gui_chat_strlen_screen (buf);
                length_screen_after_cursor = gui_chat_strlen_screen (pos_cursor);
                free (buf);
            }
        }
        if ((length_screen_before_cursor < 0) || (length_screen_after_cursor < 0))
        {
            length_screen_before_cursor = gui_chat_strlen_screen (item_content);
            length_screen_after_cursor = 0;
        }
        
        total_length_screen = length_screen_before_cursor + length_screen_after_cursor;
        
        diff = length_screen_before_cursor - chars_available;
        if (diff > 0)
        {
            ptr_start = gui_chat_string_add_offset (item_content, diff);
            if (pos_cursor && (ptr_start > pos_cursor))
                ptr_start = pos_cursor;
        }
    }
    
    /* insert "move cursor" id in string and return it */
    length = 16 + strlen (ptr_start);
    buf = malloc (length);
    if (buf)
    {
        buf[0] = '\0';
        buf_pos = 0;
        
        if (!pos_cursor)
            pos_cursor = ptr_start;
        
        /* add beginning of buffer */
        if (pos_cursor != ptr_start)
        {
            memmove (buf, ptr_start, pos_cursor - ptr_start);
            buf_pos = buf_pos + (pos_cursor - ptr_start);
        }
        /* add "move cursor here" identifier in string */
        snprintf (buf + buf_pos, length - buf_pos, "%s",
                  str_cursor);
        /* add end of buffer */
        strcat (buf, pos_cursor);
    }
    
    return buf;
}

/*
 * gui_bar_item_get_value: return value of a bar item
 *                         first look for prefix/suffix in name, then run item
 *                         callback (if found) and concatene strings
 *                         for example:  if name == "[time]"
 *                         return: color(delimiter) + "[" +
 *                                 (value of item "time") + color(delimiter) +
 *                                 "]"
 */

char *
gui_bar_item_get_value (const char *name, struct t_gui_bar *bar,
                        struct t_gui_window *window,
                        int width, int height,
                        int chars_available)
{
    const char *ptr, *start, *end;
    char *prefix, *item_name, *suffix;
    char *item_value, *item_value2, delimiter_color[32], bar_color[32];
    char *result;
    int valid_char, length;
    struct t_gui_bar_item *ptr_item;
    
    start = NULL;
    end = NULL;
    prefix = NULL;
    item_name = NULL;
    suffix = NULL;
    
    ptr = name;
    while (ptr && ptr[0])
    {
        valid_char = gui_bar_item_valid_char_name (ptr[0]);
        if (!start && valid_char)
            start = ptr;
        else if (start && !end && !valid_char)
            end = ptr - 1;
        ptr++;
    }
    if (start)
    {
        if (start > name)
            prefix = string_strndup (name, start - name);
    }
    else
        prefix = strdup (name);
    if (start)
    {
        item_name = (end) ?
            string_strndup (start, end - start + 1) : strdup (start);
    }
    if (end && end[0] && end[1])
        suffix = strdup (end + 1);
    
    item_value = NULL;
    if (item_name)
    {
        ptr_item = gui_bar_item_search_with_plugin ((window) ? window->buffer->plugin : NULL,
                                                    0,
                                                    item_name);
        if (ptr_item)
        {
            if  (ptr_item->build_callback)
            {
                item_value = (ptr_item->build_callback) (ptr_item->build_callback_data,
                                                         ptr_item, window,
                                                         width,
                                                         height);
                if (window
                    && (strncmp (item_name,
                                 gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT],
                                 strlen (gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT])) == 0))
                {
                    if (prefix)
                        chars_available -= gui_chat_strlen_screen (prefix);
                    
                    item_value2 = gui_bar_item_input_text_update_for_display (
                        (item_value) ? item_value : "",
                        window,
                        chars_available);
                    
                    if (item_value2)
                    {
                        if (item_value)
                            free (item_value);
                        item_value = item_value2;
                    }
                }
            }
        }
        if (!item_value || !item_value[0])
        {
            if (prefix)
            {
                free (prefix);
                prefix = NULL;
            }
            if (suffix)
            {
                free (suffix);
                suffix = NULL;
            }
        }
    }
    
    length = 0;
    if (prefix)
        length += 64 + strlen (prefix) + 1; /* color + prefix + color */
    if (item_value && item_value[0])
        length += strlen (item_value) + 16 + 1; /* length + "move cursor" id */
    if (suffix)
        length += 32 + strlen (suffix) + 1; /* color + suffix */

    result = NULL;
    if (length > 0)
    {
        result = malloc (length);
        if (result)
        {
            delimiter_color[0] = '\0';
            bar_color[0] = '\0';
            if (prefix || suffix)
            {
                snprintf (delimiter_color, sizeof (delimiter_color),
                          "%c%c%02d",
                          GUI_COLOR_COLOR_CHAR,
                          GUI_COLOR_FG_CHAR,
                          CONFIG_COLOR(bar->color_delim));
                snprintf (bar_color, sizeof (bar_color),
                          "%c%c%02d",
                          GUI_COLOR_COLOR_CHAR,
                          GUI_COLOR_FG_CHAR,
                          CONFIG_COLOR(bar->color_fg));
            }
            snprintf (result, length,
                      "%s%s%s%s%s%s",
                      (prefix) ? delimiter_color : "",
                      (prefix) ? prefix : "",
                      (prefix) ? bar_color : "",
                      (item_value) ? item_value : "",
                      (suffix) ? delimiter_color : "",
                      (suffix) ? suffix : "");
        }
    }
    
    if (prefix)
        free (prefix);
    if (item_name)
        free (item_name);
    if (suffix)
        free (suffix);
    if (item_value)
        free (item_value);
    
    return result;
}

/*
 * gui_bar_item_new: create a new bar item
 */

struct t_gui_bar_item *
gui_bar_item_new (struct t_weechat_plugin *plugin, const char *name,
                  char *(*build_callback)(void *data,
                                          struct t_gui_bar_item *item,
                                          struct t_gui_window *window,
                                          int max_width, int max_height),
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
 * gui_bar_item_update: update an item on all bars displayed on screen
 */

void
gui_bar_item_update (const char *item_name)
{
    struct t_gui_bar *ptr_bar;
    
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (!CONFIG_BOOLEAN(ptr_bar->hidden)
            && gui_bar_contains_item (ptr_bar, item_name))
        {
            gui_bar_ask_refresh (ptr_bar);
        }
    }
}

/*
 * gui_bar_item_free: delete a bar item
 */

void
gui_bar_item_free (struct t_gui_bar_item *item)
{
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
 * gui_bar_item_free_all: delete all bar items
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
 * gui_bar_item_free_all_plugin: delete all bar items for a plugin
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
 * gui_bar_item_default_input_paste: default item for input paste question
 */

char *
gui_bar_item_default_input_paste (void *data, struct t_gui_bar_item *item,
                                  struct t_gui_window *window,
                                  int max_width, int max_height)
{
    char *text_paste_pending = N_("%sPaste %d lines ? [ctrl-Y] Yes [ctrl-N] No");
    char *ptr_message, *buf;
    int length;
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) max_width;
    (void) max_height;
    
    if (!window)
        return NULL;
    
    if (window != gui_current_window)
        return NULL;
    
    if (!gui_keyboard_paste_pending)
        return NULL;

    ptr_message = _(text_paste_pending);
    length = strlen (ptr_message) + 16 + 1;
    buf = malloc (length);
    if (buf)
        snprintf (buf, length, ptr_message,
                  gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_input_actions))),
                  gui_keyboard_get_paste_lines ());
    
    return buf;
}

/*
 * gui_bar_item_default_input_prompt: default item for input prompt
 */

char *
gui_bar_item_default_input_prompt (void *data, struct t_gui_bar_item *item,
                                   struct t_gui_window *window,
                                   int max_width, int max_height)
{
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) window;
    (void) max_width;
    (void) max_height;
    
    return NULL;
}

/*
 * gui_bar_item_default_input_search: default item for input search status
 */

char *
gui_bar_item_default_input_search (void *data, struct t_gui_bar_item *item,
                                   struct t_gui_window *window,
                                   int max_width, int max_height)
{
    char *text_search = N_("Text search");
    char *text_search_exact = N_("Text search (exact)");
    char *ptr_message, *buf;
    int length;
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) max_width;
    (void) max_height;
    
    if (!window)
        window = gui_current_window;
    
    if (window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
        return NULL;
    
    ptr_message = (window->buffer->text_search_exact) ?
        _(text_search_exact) : _(text_search);
    length = 16 + strlen (ptr_message) + 1;
    buf = malloc (length);
    if (buf)
    {
        snprintf (buf, length, "%s%s",
                  (window->buffer->text_search_found
                   || !window->buffer->input_buffer
                   || !window->buffer->input_buffer[0]) ?
                  gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_input))) :
                  gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_input_text_not_found))),
                  ptr_message);
    }
    
    return buf;
}

/*
 * gui_bar_item_default_input_text: default item for input text
 */

char *
gui_bar_item_default_input_text (void *data, struct t_gui_bar_item *item,
                                 struct t_gui_window *window,
                                 int max_width, int max_height)
{
    char *new_input, str_buffer[128];
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) max_width;
    (void) max_height;
    
    if (!window)
        window = gui_current_window;

    snprintf (str_buffer, sizeof (str_buffer),
              "0x%x", (unsigned int)(window->buffer));
    
    new_input = hook_modifier_exec (NULL,
                                    "weechat_input_text_display",
                                    str_buffer,
                                    (window->buffer->input_buffer) ?
                                    window->buffer->input_buffer : "");
    if (new_input)
        return new_input;
    
    return (window->buffer->input_buffer) ?
        strdup (window->buffer->input_buffer) : NULL;
}

/*
 * gui_bar_item_default_time: default item for time
 */

char *
gui_bar_item_default_time (void *data, struct t_gui_bar_item *item,
                           struct t_gui_window *window,
                           int max_width, int max_height)
{
    time_t date;
    struct tm *local_time;
    char text_time[128];
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) window;
    (void) max_width;
    (void) max_height;
    
    date = time (NULL);
    local_time = localtime (&date);
    if (strftime (text_time, sizeof (text_time),
                  CONFIG_STRING(config_look_item_time_format),
                  local_time) == 0)
        return NULL;
    
    return strdup (text_time);
}

/*
 * gui_bar_item_default_buffer_count: default item for number of buffers
 */

char *
gui_bar_item_default_buffer_count (void *data, struct t_gui_bar_item *item,
                                   struct t_gui_window *window,
                                   int max_width, int max_height)
{
    char buf[32];
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) window;
    (void) max_width;
    (void) max_height;
    
    snprintf (buf, sizeof (buf), "%d",
              (last_gui_buffer) ? last_gui_buffer->number : 0);
    
    return strdup (buf);
}

/*
 * gui_bar_item_default_buffer_plugin: default item for name of buffer plugin
 */

char *
gui_bar_item_default_buffer_plugin (void *data, struct t_gui_bar_item *item,
                                    struct t_gui_window *window,
                                    int max_width, int max_height)
{
    char *plugin_name;
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) max_width;
    (void) max_height;
    
    if (!window)
        window = gui_current_window;

    plugin_name = plugin_get_name (window->buffer->plugin);
    return (plugin_name) ? strdup (plugin_name) : strdup ("");
}

/*
 * gui_bar_item_default_buffer_name: default item for name of buffer
 */

char *
gui_bar_item_default_buffer_name (void *data, struct t_gui_bar_item *item,
                                  struct t_gui_window *window,
                                  int max_width, int max_height)
{
    char buf[256];
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) max_width;
    (void) max_height;
    
    if (!window)
        window = gui_current_window;
    
    snprintf (buf, sizeof (buf), "%s%d%s:%s%s",
              gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_number))),
              window->buffer->number,
              GUI_COLOR_CUSTOM_BAR_DELIM,
              gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_name))),
              window->buffer->name);
    
    return strdup (buf);
}

/*
 * gui_bar_item_default_buffer_filter: default item for buffer filter
 */

char *
gui_bar_item_default_buffer_filter (void *data, struct t_gui_bar_item *item,
                                    struct t_gui_window *window,
                                    int max_width, int max_height)
{
    char buf[256];
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) max_width;
    (void) max_height;
    
    if (!window)
        window = gui_current_window;
    
    if (!gui_filters_enabled || !gui_filters)
        return NULL;
    
    snprintf (buf, sizeof (buf),
              "F%s%s",
              (window->buffer->lines_hidden) ? "," : "",
              (window->buffer->lines_hidden) ? _("filtered") : "");
    
    return strdup (buf);
}

/*
 * gui_bar_item_default_buffer_nicklist_count: default item for number of nicks
 *                                             in buffer nicklist
 */

char *
gui_bar_item_default_buffer_nicklist_count (void *data,
                                            struct t_gui_bar_item *item,
                                            struct t_gui_window *window,
                                            int max_width, int max_height)
{
    char buf[32];
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) max_width;
    (void) max_height;
    
    if (!window)
        window = gui_current_window;
    
    if (!window->buffer->nicklist)
        return NULL;
    
    snprintf (buf, sizeof (buf), "%d",
              window->buffer->nicklist_visible_count);
    
    return strdup (buf);
}

/*
 * gui_bar_item_default_scroll: default item for scrolling indicator
 */

char *
gui_bar_item_default_scroll (void *data, struct t_gui_bar_item *item,
                             struct t_gui_window *window,
                             int max_width, int max_height)
{
    char buf[64];
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) max_width;
    (void) max_height;
    
    if (!window)
        window = gui_current_window;
    
    if (!window->scroll)
        return NULL;
    
    snprintf (buf, sizeof (buf), _("%s-MORE(%d)-"),
              gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_more))),
              window->scroll_lines_after);
    
    return strdup (buf);
}

/*
 * gui_bar_item_default_hotlist: default item for hotlist
 */

char *
gui_bar_item_default_hotlist (void *data, struct t_gui_bar_item *item,
                              struct t_gui_window *window,
                              int max_width, int max_height)
{
    char buf[1024], format[32];
    struct t_gui_hotlist *ptr_hotlist;
    int names_count, display_name;
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) window;
    (void) max_width;
    (void) max_height;
    
    if (!gui_hotlist)
        return NULL;
    
    buf[0] = '\0';
    
    strcat (buf, _("Act: "));
    
    names_count = 0;
    for (ptr_hotlist = gui_hotlist; ptr_hotlist;
         ptr_hotlist = ptr_hotlist->next_hotlist)
    {
        switch (ptr_hotlist->priority)
        {
            case GUI_HOTLIST_LOW:
                strcat (buf, gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_data_other))));
                display_name = ((CONFIG_INTEGER(config_look_hotlist_names_level) & 1) != 0);
                break;
            case GUI_HOTLIST_MESSAGE:
                strcat (buf, gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_data_msg))));
                display_name = ((CONFIG_INTEGER(config_look_hotlist_names_level) & 2) != 0);
                break;
            case GUI_HOTLIST_PRIVATE:
                strcat (buf, gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_data_private))));
                display_name = ((CONFIG_INTEGER(config_look_hotlist_names_level) & 4) != 0);
                break;
            case GUI_HOTLIST_HIGHLIGHT:
                strcat (buf, gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_data_highlight))));
                display_name = ((CONFIG_INTEGER(config_look_hotlist_names_level) & 8) != 0);
                break;
            default:
                display_name = 0;
                break;
        }
        
        sprintf (buf + strlen (buf), "%d", ptr_hotlist->buffer->number);
        
        if (display_name
            && (CONFIG_INTEGER(config_look_hotlist_names_count) != 0)
            && (names_count < CONFIG_INTEGER(config_look_hotlist_names_count)))
        {
            names_count++;
            
            strcat (buf, GUI_COLOR_CUSTOM_BAR_DELIM);
            strcat (buf, ":");
            strcat (buf, GUI_COLOR_CUSTOM_BAR_FG);
            if (CONFIG_INTEGER(config_look_hotlist_names_length) == 0)
                snprintf (format, sizeof (format) - 1, "%%s");
            else
                snprintf (format, sizeof (format) - 1,
                          "%%.%ds",
                          CONFIG_INTEGER(config_look_hotlist_names_length));
            sprintf (buf + strlen (buf), format,
                     (CONFIG_BOOLEAN(config_look_hotlist_short_names)) ?
                     ptr_hotlist->buffer->short_name : ptr_hotlist->buffer->name);
        }
        
        if (ptr_hotlist->next_hotlist)
            strcat (buf, ",");

        if (strlen (buf) > sizeof (buf) - 32)
            break;
    }
    
    return strdup (buf);
}

/*
 * gui_bar_item_default_completion: default item for (partial) completion
 */

char *
gui_bar_item_default_completion (void *data, struct t_gui_bar_item *item,
                                 struct t_gui_window *window,
                                 int max_width, int max_height)
{
    int length;
    char *buf, number_str[16];
    struct t_gui_completion_partial *ptr_item;
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) window;
    (void) max_width;
    (void) max_height;
    
    length = 1;
    for (ptr_item = gui_completion_partial_list; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        length += strlen (ptr_item->word) + 32;
    }
    
    buf = malloc (length);
    if (buf)
    {
        buf[0] = '\0';
        for (ptr_item = gui_completion_partial_list; ptr_item;
             ptr_item = ptr_item->next_item)
        {
            strcat (buf, GUI_COLOR_CUSTOM_BAR_FG);
            strcat (buf, ptr_item->word);
            if (ptr_item->count > 0)
            {
                strcat (buf, GUI_COLOR_CUSTOM_BAR_DELIM);
                strcat (buf, "(");
                snprintf (number_str, sizeof (number_str),
                          "%d", ptr_item->count);
                strcat (buf, number_str);
                strcat (buf, ")");
            }
            if (ptr_item->next_item)
                strcat (buf, " ");
        }
    }
    
    return buf;
}

/*
 * gui_bar_item_default_buffer_title: default item for buffer title
 */

char *
gui_bar_item_default_buffer_title (void *data, struct t_gui_bar_item *item,
                                   struct t_gui_window *window,
                                   int max_width, int max_height)
{
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) max_width;
    (void) max_height;
    
    if (!window)
        window = gui_current_window;
    
    return (window->buffer->title) ?
        strdup (window->buffer->title) : NULL;
}

/*
 * gui_bar_item_default_buffer_nicklist: default item for nicklist
 */

char *
gui_bar_item_default_buffer_nicklist (void *data, struct t_gui_bar_item *item,
                                      struct t_gui_window *window,
                                      int max_width, int max_height)
{
    struct t_gui_nick_group *ptr_group;
    struct t_gui_nick *ptr_nick;
    struct t_config_option *ptr_option;
    int i, length;
    char *buf, str_prefix[2];
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) max_width;
    (void) max_height;
    
    if (!window)
        window = gui_current_window;
    
    length = 1;
    ptr_group = NULL;
    ptr_nick = NULL;
    gui_nicklist_get_next_item (window->buffer, &ptr_group, &ptr_nick);
    while (ptr_group || ptr_nick)
    {
        if ((ptr_nick && ptr_nick->visible)
            || (ptr_group && window->buffer->nicklist_display_groups
                && ptr_group->visible))
        {
            if (ptr_nick)
                length += ptr_nick->group->level + 16 /* color */
                    + 1 /* prefix */ + 16 /* color */
                    + strlen (ptr_nick->name) + 1;
            else
                length += ptr_group->level - 1
                    + strlen (gui_nicklist_get_group_start (ptr_group->name))
                    + 1;
        }
        gui_nicklist_get_next_item (window->buffer, &ptr_group, &ptr_nick);
    }

    buf = malloc (length);
    if (buf)
    {
        buf[0] = '\0';
        ptr_group = NULL;
        ptr_nick = NULL;
        gui_nicklist_get_next_item (window->buffer, &ptr_group, &ptr_nick);
        while (ptr_group || ptr_nick)
        {
            if ((ptr_nick && ptr_nick->visible)
                || (ptr_group && window->buffer->nicklist_display_groups
                    && ptr_group->visible))
            {
                if (buf[0])
                    strcat (buf, "\n");
                
                if (ptr_nick)
                {
                    if (window->buffer->nicklist_display_groups)
                    {
                        for (i = 0; i < ptr_nick->group->level; i++)
                        {
                            strcat (buf, " ");
                        }
                    }
                    config_file_search_with_string (ptr_nick->prefix_color,
                                                    NULL, NULL, &ptr_option,
                                                    NULL);
                    if (ptr_option)
                        strcat (buf, gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(ptr_option))));
                    str_prefix[0] = ptr_nick->prefix;
                    str_prefix[1] = '\0';
                    strcat (buf, str_prefix);
                    config_file_search_with_string (ptr_nick->color,
                                                    NULL, NULL, &ptr_option,
                                                    NULL);
                    if (ptr_option)
                        strcat (buf, gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(ptr_option))));
                    strcat (buf, ptr_nick->name);
                }
                else
                {
                    for (i = 0; i < ptr_group->level - 1; i++)
                    {
                        strcat (buf, " ");
                    }
                    strcat (buf, gui_nicklist_get_group_start (ptr_group->name));
                }
            }
            gui_nicklist_get_next_item (window->buffer, &ptr_group, &ptr_nick);
        }
    }
    
    return buf;
}

/*
 * gui_bar_item_timer_cb: timer callback
 */

int
gui_bar_item_timer_cb (void *data)
{
    time_t date;
    struct tm *local_time;
    static char item_time_text[128] = { '\0' };
    char new_item_time_text[128];
    
    date = time (NULL);
    local_time = localtime (&date);
    if (strftime (new_item_time_text, sizeof (new_item_time_text),
                  CONFIG_STRING(config_look_item_time_format),
                  local_time) == 0)
        return WEECHAT_RC_OK;
    
    /* we update item only if it changed since last time
       for example if time is only hours:minutes, we'll update
       only when minute changed */
    if (strcmp (new_item_time_text, item_time_text) != 0)
    {
        snprintf (item_time_text, sizeof (item_time_text),
                  "%s", new_item_time_text);
        gui_bar_item_update ((char *)data);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * gui_bar_item_signal_cb: callback when a signal is received, for rebuilding
 *                         an item
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
 * gui_bar_item_hook_signal: hook a signal to update bar items
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
 * gui_bar_item_init: init default items in WeeChat
 */

void
gui_bar_item_init ()
{
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
    gui_bar_item_hook_signal ("input_prompt_changed",
                              gui_bar_item_names[GUI_BAR_ITEM_INPUT_PROMPT]);
    
    /* input search */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_INPUT_SEARCH],
                      &gui_bar_item_default_input_search, NULL);
    gui_bar_item_hook_signal ("input_search",
                              gui_bar_item_names[GUI_BAR_ITEM_INPUT_SEARCH]);
    gui_bar_item_hook_signal ("input_text_changed",
                              gui_bar_item_names[GUI_BAR_ITEM_INPUT_SEARCH]);
    
    /* input text */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT],
                      &gui_bar_item_default_input_text, NULL);
    gui_bar_item_hook_signal ("input_text_*",
                              gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT]);
    gui_bar_item_hook_signal ("buffer_switch",
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
    gui_bar_item_hook_signal ("buffer_open",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_COUNT]);
    gui_bar_item_hook_signal ("buffer_closed",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_COUNT]);
    
    /* buffer plugin */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_PLUGIN],
                      &gui_bar_item_default_buffer_plugin, NULL);
    gui_bar_item_hook_signal ("buffer_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_PLUGIN]);
    
    /* buffer name */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NAME],
                      &gui_bar_item_default_buffer_name, NULL);
    gui_bar_item_hook_signal ("buffer_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NAME]);
    gui_bar_item_hook_signal ("buffer_renamed",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NAME]);
    gui_bar_item_hook_signal ("buffer_moved",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NAME]);
    
    /* buffer filter */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_FILTER],
                      &gui_bar_item_default_buffer_filter, NULL);
    gui_bar_item_hook_signal ("buffer_lines_hidden",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_FILTER]);
    gui_bar_item_hook_signal ("filters_*",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_FILTER]);
    
    /* buffer nicklist count */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT],
                      &gui_bar_item_default_buffer_nicklist_count, NULL);
    gui_bar_item_hook_signal ("buffer_switch",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT]);
    gui_bar_item_hook_signal ("nicklist_changed",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT]);
    
    /* scroll indicator */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_SCROLL],
                      &gui_bar_item_default_scroll, NULL);
    gui_bar_item_hook_signal ("window_scrolled",
                              gui_bar_item_names[GUI_BAR_ITEM_SCROLL]);
    
    /* hotlist */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_HOTLIST],
                      &gui_bar_item_default_hotlist, NULL);
    gui_bar_item_hook_signal ("hotlist_changed",
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
    gui_bar_item_hook_signal ("buffer_title_changed",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_TITLE]);
    
    /* buffer nicklist */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST],
                      &gui_bar_item_default_buffer_nicklist, NULL);
    gui_bar_item_hook_signal ("nicklist_changed",
                              gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST]);
}

/*
 * gui_bar_item_end: remove bar items and hooks
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
 * gui_bar_item_print_log: print bar items infos in log (usually for crash dump)
 */

void
gui_bar_item_print_log ()
{
    struct t_gui_bar_item *ptr_item;
    
    for (ptr_item = gui_bar_items; ptr_item; ptr_item = ptr_item->next_item)
    {
        log_printf ("");
        log_printf ("[bar item (addr:0x%x)]", ptr_item);
        log_printf ("  plugin . . . . . . . . : 0x%x ('%s')",
                    ptr_item->plugin,
                    (ptr_item->plugin) ? ptr_item->plugin->name : "");
        log_printf ("  name . . . . . . . . . : '%s'", ptr_item->name);
        log_printf ("  build_callback . . . . : 0x%x", ptr_item->build_callback);
        log_printf ("  build_callback_data. . : 0x%x", ptr_item->build_callback_data);
        log_printf ("  prev_item. . . . . . . : 0x%x", ptr_item->prev_item);
        log_printf ("  next_item. . . . . . . : 0x%x", ptr_item->next_item);
    }
}
