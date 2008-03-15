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
#include "gui-hotlist.h"
#include "gui-window.h"


struct t_gui_bar_item *gui_bar_items = NULL;     /* first bar item          */
struct t_gui_bar_item *last_gui_bar_item = NULL; /* last bar item           */
char *gui_bar_item_names[GUI_BAR_NUM_ITEMS] =
{ "buffer_count", "buffer_plugin", "buffer_name", "nicklist_count", "scroll",
  "hotlist"
};
struct t_gui_bar_item_hook *gui_bar_item_hooks = NULL;


/*
 * gui_bar_item_search: search a bar item
 */

struct t_gui_bar_item *
gui_bar_item_search (char *name)
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
gui_bar_item_search_with_plugin (struct t_weechat_plugin *plugin, char *name)
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
 * gui_bar_item_new: create a new bar item
 */

struct t_gui_bar_item *
gui_bar_item_new (struct t_weechat_plugin *plugin, char *name,
                  char *(*build_callback)(void *data,
                                          struct t_gui_bar_item *item,
                                          struct t_gui_window *window,
                                          int max_width, int max_height),
                  void *build_callback_data)
{
    struct t_gui_bar_item *new_bar_item;
    
    if (!name || !name[0])
        return NULL;
    
    /* it's not possible to create 2 bar items with same name for same plugin*/
    if (gui_bar_item_search_with_plugin (plugin, name))
        return NULL;
    
    /* create bar item */
    new_bar_item = (struct t_gui_bar_item *) malloc (sizeof (struct t_gui_bar_item));
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
gui_bar_contains_item (struct t_gui_bar *bar, char *name)
{
    int i;
    
    if (!bar || !name || !name[0])
        return 0;
    
    for (i = 0; i < bar->items_count; i++)
    {
        if (strcmp (bar->items_array[i], name) == 0)
            return 1;
    }
    
    /* item is not in bar */
    return 0;
}

/*
 * gui_bar_item_update: update an item on all bars displayed on screen
 */

void
gui_bar_item_update (char *name)
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
 * gui_bar_item_default_buffer_count: default item for number of buffers
 */

char *
gui_bar_item_default_buffer_count (void *data, struct t_gui_bar_item *item,
                                   struct t_gui_window *window,
                                   int max_width, int max_height)
{
    char buf[64];
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) window;
    (void) max_width;
    (void) max_height;
    
    snprintf (buf, sizeof (buf), "%s[%s%d%s] ",
              GUI_COLOR(GUI_COLOR_STATUS_DELIMITERS),
              GUI_COLOR(GUI_COLOR_STATUS),
              (last_gui_buffer) ? last_gui_buffer->number : 0,
              GUI_COLOR(GUI_COLOR_STATUS_DELIMITERS));
    
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
    char buf[64];
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) max_width;
    (void) max_height;
    
    if (!window)
        window = gui_current_window;
    
    snprintf (buf, sizeof (buf), "%s[%s%s%s] ",
              GUI_COLOR(GUI_COLOR_STATUS_DELIMITERS),
              GUI_COLOR(GUI_COLOR_STATUS),
              (window->buffer->plugin) ? window->buffer->plugin->name : "core",
              GUI_COLOR(GUI_COLOR_STATUS_DELIMITERS));
    
    return strdup (buf);
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
    
    snprintf (buf, sizeof (buf), "%s%d%s:%s%s%s/%s%s ",
              GUI_COLOR(GUI_COLOR_STATUS_NUMBER),
              window->buffer->number,
              GUI_COLOR(GUI_COLOR_STATUS_DELIMITERS),
              GUI_COLOR(GUI_COLOR_STATUS_CATEGORY),
              window->buffer->category,
              GUI_COLOR(GUI_COLOR_STATUS_DELIMITERS),
              GUI_COLOR(GUI_COLOR_STATUS_NAME),
              window->buffer->name);
    
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
    char buf[64];
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) max_width;
    (void) max_height;
    
    if (!window)
        window = gui_current_window;
    
    if (!window->buffer->nicklist)
        return NULL;
    
    snprintf (buf, sizeof (buf), "%s[%s%d%s] ",
              GUI_COLOR(GUI_COLOR_STATUS_DELIMITERS),
              GUI_COLOR(GUI_COLOR_STATUS),
              window->buffer->nicklist_visible_count,
              GUI_COLOR(GUI_COLOR_STATUS_DELIMITERS));
    
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
    
    snprintf (buf, sizeof (buf), "%s%s ",
              GUI_COLOR(GUI_COLOR_STATUS_MORE),
              _("(MORE)"));
    
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
    
    strcat (buf, GUI_COLOR(GUI_COLOR_STATUS_DELIMITERS));
    strcat (buf, "[");
    strcat (buf, GUI_COLOR(GUI_COLOR_STATUS));
    strcat (buf, _("Act: "));
    
    names_count = 0;
    for (ptr_hotlist = gui_hotlist; ptr_hotlist;
         ptr_hotlist = ptr_hotlist->next_hotlist)
    {
        switch (ptr_hotlist->priority)
        {
            case GUI_HOTLIST_LOW:
                strcat (buf, GUI_COLOR(GUI_COLOR_STATUS_DATA_OTHER));
                display_name = ((CONFIG_INTEGER(config_look_hotlist_names_level) & 1) != 0);
                break;
            case GUI_HOTLIST_MESSAGE:
                strcat (buf, GUI_COLOR(GUI_COLOR_STATUS_DATA_MSG));
                display_name = ((CONFIG_INTEGER(config_look_hotlist_names_level) & 2) != 0);
                break;
            case GUI_HOTLIST_PRIVATE:
                strcat (buf, GUI_COLOR(GUI_COLOR_STATUS_DATA_PRIVATE));
                display_name = ((CONFIG_INTEGER(config_look_hotlist_names_level) & 4) != 0);
                break;
            case GUI_HOTLIST_HIGHLIGHT:
                strcat (buf, GUI_COLOR(GUI_COLOR_STATUS_DATA_HIGHLIGHT));
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
            
            strcat (buf, GUI_COLOR(GUI_COLOR_STATUS_DELIMITERS));
            strcat (buf, ":");
            strcat (buf, GUI_COLOR(GUI_COLOR_STATUS));
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
    strcat (buf, GUI_COLOR(GUI_COLOR_STATUS_DELIMITERS));
    strcat (buf, "] ");
    
    return strdup (buf);
}

/*
 * gui_bar_item_signal_cb: callback when a signal is received, for rebuilding
 *                         an item
 */

int
gui_bar_item_signal_cb (void *data, char *signal,
                        char *type_data, void *signal_data)
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
gui_bar_item_hook (char *signal, char *item)
{
    struct t_gui_bar_item_hook *bar_item_hook;
    
    bar_item_hook = (struct t_gui_bar_item_hook *)malloc (sizeof (struct t_gui_bar_item_hook));
    if (bar_item_hook)
    {
        bar_item_hook->hook = hook_signal (NULL, signal,
                                           &gui_bar_item_signal_cb, item);
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
    /* buffer count */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_WEECHAT_BUFFER_COUNT],
                      &gui_bar_item_default_buffer_count, NULL);
    gui_bar_item_hook ("buffer_open",
                       gui_bar_item_names[GUI_BAR_ITEM_WEECHAT_BUFFER_COUNT]);
    gui_bar_item_hook ("buffer_closed",
                       gui_bar_item_names[GUI_BAR_ITEM_WEECHAT_BUFFER_COUNT]);
    
    /* buffer plugin */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_WEECHAT_BUFFER_PLUGIN],
                      &gui_bar_item_default_buffer_plugin, NULL);
    gui_bar_item_hook ("buffer_switch",
                       gui_bar_item_names[GUI_BAR_ITEM_WEECHAT_BUFFER_PLUGIN]);
    
    /* buffer name */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_WEECHAT_BUFFER_NAME],
                      &gui_bar_item_default_buffer_name, NULL);
    gui_bar_item_hook ("buffer_switch",
                       gui_bar_item_names[GUI_BAR_ITEM_WEECHAT_BUFFER_NAME]);
    gui_bar_item_hook ("buffer_renamed",
                       gui_bar_item_names[GUI_BAR_ITEM_WEECHAT_BUFFER_NAME]);
    gui_bar_item_hook ("buffer_moved",
                       gui_bar_item_names[GUI_BAR_ITEM_WEECHAT_BUFFER_NAME]);
    
    /* nicklist count */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_WEECHAT_NICKLIST_COUNT],
                      &gui_bar_item_default_nicklist_count, NULL);
    gui_bar_item_hook ("buffer_switch",
                       gui_bar_item_names[GUI_BAR_ITEM_WEECHAT_NICKLIST_COUNT]);
    gui_bar_item_hook ("nicklist_changed",
                       gui_bar_item_names[GUI_BAR_ITEM_WEECHAT_NICKLIST_COUNT]);
    
    /* scroll indicator */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_WEECHAT_SCROLL],
                      &gui_bar_item_default_scroll, NULL);
    gui_bar_item_hook ("window_scrolled",
                       gui_bar_item_names[GUI_BAR_ITEM_WEECHAT_SCROLL]);
    
    /* hotlist */
    gui_bar_item_new (NULL,
                      gui_bar_item_names[GUI_BAR_ITEM_WEECHAT_HOTLIST],
                      &gui_bar_item_default_hotlist, NULL);
    gui_bar_item_hook ("hotlist_changed",
                       gui_bar_item_names[GUI_BAR_ITEM_WEECHAT_HOTLIST]);
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
