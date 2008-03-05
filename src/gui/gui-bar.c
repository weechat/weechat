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

/* gui-bar.c: bar functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../core/weechat.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "gui-bar.h"
#include "gui-window.h"


char *gui_bar_type_str[GUI_BAR_NUM_TYPES] =
{ "root", "window_active", "window_inactive" };
char *gui_bar_position_str[GUI_BAR_NUM_POSITIONS] =
{ "bottom", "top", "left", "right" };

struct t_gui_bar *gui_bars = NULL;               /* first bar               */
struct t_gui_bar *last_gui_bar = NULL;           /* last bar                */


/*
 * gui_bar_root_get_size: get total bar size ("root" type) for a position
 */

int
gui_bar_root_get_size (struct t_gui_bar *bar, int position)
{
    struct t_gui_bar *ptr_bar;
    int total_size;
    
    total_size = 0;
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (bar && (ptr_bar == bar))
            return total_size;
        
        if ((ptr_bar->type == GUI_BAR_TYPE_ROOT)
            && (ptr_bar->position == position))
        {
            total_size += ptr_bar->size;
            if (ptr_bar->separator)
                total_size++;
        }
    }
    return total_size;
}

/*
 * gui_bar_search: search a bar by name
 */

struct t_gui_bar *
gui_bar_search (char *name)
{
    struct t_gui_bar *ptr_bar;
    
    if (!name || !name[0])
        return NULL;
    
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (strcmp (ptr_bar->name, name) == 0)
            return ptr_bar;
    }
    
    /* bar not found */
    return NULL;
}

/*
 * gui_bar_new: create a new bar
 */

struct t_gui_bar *
gui_bar_new (struct t_weechat_plugin *plugin, char *name, char *type,
             char *position, int size, int separator, char *items)
{
    struct t_gui_bar *new_bar;
    struct t_gui_window *ptr_win;
    int i, type_value, position_value;
    
    if (!name || !name[0])
        return NULL;
    
    /* it's not possible to create 2 bars with same name */
    if (gui_bar_search (name))
        return NULL;
    
    /* look for type */
    type_value = -1;
    for (i = 0; i < GUI_BAR_NUM_TYPES; i++)
    {
        if (string_strcasecmp (type, gui_bar_type_str[i]) == 0)
        {
            type_value = i;
            break;
        }
    }
    if (type_value < 0)
        return NULL;
    
    /* look for position */
    position_value = -1;
    for (i = 0; i < GUI_BAR_NUM_POSITIONS; i++)
    {
        if (string_strcasecmp (position, gui_bar_position_str[i]) == 0)
        {
            position_value = i;
            break;
        }
    }
    if (position_value < 0)
        return NULL;
    
    /* create bar */
    new_bar = (struct t_gui_bar *) malloc (sizeof (struct t_gui_bar));
    if (new_bar)
    {
        new_bar->plugin = plugin;
        new_bar->number = (last_gui_bar) ? last_gui_bar->number + 1 : 1;
        new_bar->name = strdup (name);
        new_bar->type = type_value;
        new_bar->position = position_value;
        new_bar->size = size;
        new_bar->separator = separator;
        if (items && items[0])
        {
            new_bar->items = strdup (items);
            new_bar->items_array = string_explode (items, ",", 0, 0,
                                                   &new_bar->items_count);
        }
        else
        {
            new_bar->items = NULL;
            new_bar->items_count = 0;
            new_bar->items_array = NULL;
        }
        
        if (type_value == GUI_BAR_TYPE_ROOT)
        {
            /* create only one window for bar */
            gui_bar_window_new (new_bar, NULL);
        }
        else
        {
            /* create bar window for all opened windows */
            for (ptr_win = gui_windows; ptr_win;
                 ptr_win = ptr_win->next_window)
            {
                gui_bar_window_new (new_bar, ptr_win);
            }
        }
        
        /* add bar to bars queue */
        new_bar->prev_bar = last_gui_bar;
        if (gui_bars)
            last_gui_bar->next_bar = new_bar;
        else
            gui_bars = new_bar;
        last_gui_bar = new_bar;
        new_bar->next_bar = NULL;
        
        gui_window_refresh_needed = 1;
        
        return new_bar;
    }
    
    /* failed to create bar */
    return NULL;
}

/*
 * gui_bar_update: update a bar on screen
 */

void
gui_bar_update (char *name)
{
    struct t_gui_bar *ptr_bar;
    
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (strcmp (ptr_bar->name, name) == 0)
            gui_bar_draw (ptr_bar);
    }
}

/*
 * gui_bar_free: delete a bar
 */

void
gui_bar_free (struct t_gui_bar *bar)
{
    /* remove bar from bars list */
    if (bar->prev_bar)
        bar->prev_bar->next_bar = bar->next_bar;
    if (bar->next_bar)
        bar->next_bar->prev_bar = bar->prev_bar;
    if (gui_bars == bar)
        gui_bars = bar->next_bar;
    if (last_gui_bar == bar)
        last_gui_bar = bar->prev_bar;
    
    /* free data */
    if (bar->name)
        free (bar->name);
    if (bar->bar_window)
        gui_bar_window_free (bar->bar_window);
    if (bar->items)
        free (bar->items);
    if (bar->items_array)
        string_free_exploded (bar->items_array);
    
    free (bar);
    
    gui_window_refresh_needed = 1;
}

/*
 * gui_bar_free_all: delete all bars
 */

void
gui_bar_free_all ()
{
    while (gui_bars)
    {
        gui_bar_free (gui_bars);
    }
}

/*
 * gui_bar_free_all_plugin: delete all bars for a plugin
 */

void
gui_bar_free_all_plugin (struct t_weechat_plugin *plugin)
{
    struct t_gui_bar *ptr_bar, *next_bar;

    ptr_bar = gui_bars;
    while (ptr_bar)
    {
        next_bar = ptr_bar->next_bar;
        
        if (ptr_bar->plugin == plugin)
            gui_bar_free (ptr_bar);
        
        ptr_bar = next_bar;
    }
}

/*
 * gui_bar_print_log: print bar infos in log (usually for crash dump)
 */

void
gui_bar_print_log ()
{
    struct t_gui_bar *ptr_bar;
    
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        log_printf ("");
        log_printf ("[bar (addr:0x%x)]", ptr_bar);
        log_printf ("  plugin . . . . . . . . : 0x%x", ptr_bar->plugin);
        log_printf ("  number . . . . . . . . : %d",   ptr_bar->number);
        log_printf ("  name . . . . . . . . . : '%s'", ptr_bar->name);
        log_printf ("  type . . . . . . . . . : %d (%s)",
                    ptr_bar->type,
                    gui_bar_type_str[ptr_bar->type]);
        log_printf ("  position . . . . . . . : %d (%s)",
                    ptr_bar->position,
                    gui_bar_position_str[ptr_bar->position]);
        log_printf ("  size . . . . . . . . . : %d",   ptr_bar->size);
        log_printf ("  separator. . . . . . . : %d",   ptr_bar->separator);
        log_printf ("  items. . . . . . . . . : '%s'", ptr_bar->items);
        log_printf ("  items_count. . . . . . : %d",   ptr_bar->items_count);
        log_printf ("  items_array. . . . . . : 0x%x", ptr_bar->items_array);
        log_printf ("  bar_window . . . . . . : 0x%x", ptr_bar->bar_window);
        log_printf ("  prev_bar . . . . . . . : 0x%x", ptr_bar->prev_bar);
        log_printf ("  next_bar . . . . . . . : 0x%x", ptr_bar->next_bar);
        
        if (ptr_bar->bar_window)
            gui_bar_window_print_log (ptr_bar->bar_window);
    }
}
