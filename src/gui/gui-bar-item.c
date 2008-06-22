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
#include "../plugins/plugin.h"
#include "gui-bar-item.h"
#include "gui-bar.h"
#include "gui-buffer.h"
#include "gui-color.h"
#include "gui-completion.h"
#include "gui-filter.h"
#include "gui-hotlist.h"
#include "gui-window.h"


struct t_gui_bar_item *gui_bar_items = NULL;     /* first bar item          */
struct t_gui_bar_item *last_gui_bar_item = NULL; /* last bar item           */
char *gui_bar_item_names[GUI_BAR_NUM_ITEMS] =
{ "time", "buffer_count", "buffer_plugin", "buffer_name", "buffer_filter",
  "nicklist_count", "scroll", "hotlist", "completion"
};
struct t_gui_bar_item_hook *gui_bar_item_hooks = NULL;
struct t_hook *gui_bar_item_timer = NULL;


/*
 * gui_bar_item_search: search a bar item
 */

struct t_gui_bar_item *
gui_bar_item_search (const char *name)
{
    struct t_gui_bar_item *ptr_item;
    
    if (!name || !name[0])
        return NULL;
    
    for (ptr_item = gui_bar_items; ptr_item; ptr_item = ptr_item->next_item)
    {
        if (strcmp (ptr_item->name, name) == 0)
            return ptr_item;
    }
    
    /* bar item not found */
    return NULL;
}

/*
 * gui_bar_item_search_with_plugin: search a bar item for a plugin
 */

struct t_gui_bar_item *
gui_bar_item_search_with_plugin (struct t_weechat_plugin *plugin, const char *name)
{
    struct t_gui_bar_item *ptr_item;
    
    if (!name || !name[0])
        return NULL;
    
    for (ptr_item = gui_bar_items; ptr_item; ptr_item = ptr_item->next_item)
    {
        if ((ptr_item->plugin == plugin)
            && (strcmp (ptr_item->name, name) == 0))
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
                        int width, int height)
{
    const char *ptr, *start, *end;
    char *prefix, *item_name, *suffix;
    char *item_value, delimiter_color[32], bar_color[32];
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
        ptr_item = gui_bar_item_search (item_name);
        if (ptr_item)
        {
            if  (ptr_item->build_callback)
            {
                item_value = (ptr_item->build_callback) (ptr_item->build_callback_data,
                                                         ptr_item, window,
                                                         width,
                                                         height);
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
        }
        else
            item_value = strdup (item_name);
    }
    
    length = 0;
    if (prefix)
        length += 64 + strlen (prefix) + 1; /* color + prefix + color */
    if (item_value && item_value[0])
        length += strlen (item_value) + 1;
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
                          "%cF%02d",
                          GUI_COLOR_COLOR_CHAR,
                          CONFIG_COLOR(bar->color_delim));
                snprintf (bar_color, sizeof (bar_color),
                          "%cF%02d",
                          GUI_COLOR_COLOR_CHAR,
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
    if (gui_bar_item_search_with_plugin (plugin, name))
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
 * gui_bar_contains_item: return 1 if a bar contains item, O otherwise
 */

int
gui_bar_contains_item (struct t_gui_bar *bar, const char *name)
{
    int i, length;
    char *ptr;
    
    if (!bar || !name || !name[0])
        return 0;
    
    length = strlen (name);
    
    for (i = 0; i < bar->items_count; i++)
    {
        /* skip non letters chars at beginning (prefix) */
        ptr = bar->items_array[i];
        while (ptr && ptr[0])
        {
            if (gui_bar_item_valid_char_name (ptr[0]))
                break;
            ptr++;
        }
        if (ptr && ptr[0] && (strncmp (ptr, name, length) == 0))
            return 1;
    }
    
    /* item is not in bar */
    return 0;
}

/*
 * gui_bar_item_update: update an item on all bars displayed on screen
 */

void
gui_bar_item_update (const char *name)
{
    struct t_gui_bar *ptr_bar;
    
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (gui_bar_contains_item (ptr_bar, name))
            gui_bar_draw (ptr_bar);
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
        item->prev_item->next_item = item->next_item;
    if (item->next_item)
        item->next_item->prev_item = item->prev_item;
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
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) max_width;
    (void) max_height;
    
    if (!window)
        window = gui_current_window;
    
    return (window->buffer->plugin) ?
        strdup (window->buffer->plugin->name) : strdup ("core");
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
    
    snprintf (buf, sizeof (buf), "%s%d%s:%s%s%s/%s%s",
              gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_number))),
              window->buffer->number,
              GUI_COLOR_BAR_DELIM,
              gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_category))),
              window->buffer->category,
              GUI_COLOR_BAR_DELIM,
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
 * gui_bar_item_default_nicklist_count: default item for number of nicks in
 *                                      buffer nicklist
 */

char *
gui_bar_item_default_nicklist_count (void *data, struct t_gui_bar_item *item,
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
    
    snprintf (buf, sizeof (buf), "%s%s",
              gui_color_get_custom (gui_color_get_name (CONFIG_COLOR(config_color_status_more))),
              _("-MORE-"));
    
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
            
            strcat (buf, GUI_COLOR_BAR_DELIM);
            strcat (buf, ":");
            strcat (buf, GUI_COLOR_BAR_FG);
            if (CONFIG_INTEGER(config_look_hotlist_names_length) == 0)
                snprintf (format, sizeof (format) - 1, "%%s");
            else
                snprintf (format, sizeof (format) - 1,
                          "%%.%ds",
                          CONFIG_INTEGER(config_look_hotlist_names_length));
            sprintf (buf + strlen (buf), format, ptr_hotlist->buffer->name);
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
            strcat (buf, GUI_COLOR_BAR_FG);
            strcat (buf, ptr_item->word);
            if (ptr_item->count > 0)
            {
                strcat (buf, GUI_COLOR_BAR_DELIM);
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
 * gui_bar_item_hook: hook a signal to update bar items
 */

void
gui_bar_item_hook (const char *signal, const char *item)
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
    gui_bar_item_hook ("buffer_open",
                       gui_bar_item_names[GUI_BAR_ITEM_BUFFER_COUNT]);
    gui_bar_item_hook ("buffer_closed",
                       gui_bar_item_names[GUI_BAR_ITEM_BUFFER_COUNT]);
    
    /* buffer plugin */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_PLUGIN],
                      &gui_bar_item_default_buffer_plugin, NULL);
    gui_bar_item_hook ("buffer_switch",
                       gui_bar_item_names[GUI_BAR_ITEM_BUFFER_PLUGIN]);
    
    /* buffer name */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NAME],
                      &gui_bar_item_default_buffer_name, NULL);
    gui_bar_item_hook ("buffer_switch",
                       gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NAME]);
    gui_bar_item_hook ("buffer_renamed",
                       gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NAME]);
    gui_bar_item_hook ("buffer_moved",
                       gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NAME]);
    
    /* buffer filter */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_FILTER],
                      &gui_bar_item_default_buffer_filter, NULL);
    gui_bar_item_hook ("buffer_lines_hidden",
                       gui_bar_item_names[GUI_BAR_ITEM_BUFFER_FILTER]);
    gui_bar_item_hook ("filters_*",
                       gui_bar_item_names[GUI_BAR_ITEM_BUFFER_FILTER]);
    
    /* nicklist count */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_NICKLIST_COUNT],
                      &gui_bar_item_default_nicklist_count, NULL);
    gui_bar_item_hook ("buffer_switch",
                       gui_bar_item_names[GUI_BAR_ITEM_NICKLIST_COUNT]);
    gui_bar_item_hook ("nicklist_changed",
                       gui_bar_item_names[GUI_BAR_ITEM_NICKLIST_COUNT]);
    
    /* scroll indicator */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_SCROLL],
                      &gui_bar_item_default_scroll, NULL);
    gui_bar_item_hook ("window_scrolled",
                       gui_bar_item_names[GUI_BAR_ITEM_SCROLL]);
    
    /* hotlist */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_HOTLIST],
                      &gui_bar_item_default_hotlist, NULL);
    gui_bar_item_hook ("hotlist_changed",
                       gui_bar_item_names[GUI_BAR_ITEM_HOTLIST]);
    
    /* completion (possible words when a partial completion occurs) */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_COMPLETION],
                      &gui_bar_item_default_completion, NULL);
    gui_bar_item_hook ("partial_completion",
                       gui_bar_item_names[GUI_BAR_ITEM_COMPLETION]);
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
        log_printf ("  plugin . . . . . . . . : 0x%x", ptr_item->plugin);
        log_printf ("  name . . . . . . . . . : '%s'", ptr_item->name);
        log_printf ("  build_callback . . . . : 0x%x", ptr_item->build_callback);
        log_printf ("  build_callback_data. . : 0x%x", ptr_item->build_callback_data);
        log_printf ("  prev_item. . . . . . . : 0x%x", ptr_item->prev_item);
        log_printf ("  next_item. . . . . . . : 0x%x", ptr_item->next_item);
    }
}
