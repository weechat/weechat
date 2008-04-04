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
{ "root", "window", "window_active", "window_inactive" };
char *gui_bar_position_str[GUI_BAR_NUM_POSITIONS] =
{ "bottom", "top", "left", "right" };

struct t_gui_bar *gui_bars = NULL;               /* first bar               */
struct t_gui_bar *last_gui_bar = NULL;           /* last bar                */


/*
 * gui_bar_get_type: get type number with string
 *                   return -1 if type is not found
 */

int
gui_bar_get_type (char *type)
{
    int i;
    
    for (i = 0; i < GUI_BAR_NUM_TYPES; i++)
    {
        if (string_strcasecmp (type, gui_bar_type_str[i]) == 0)
            return i;
    }
    
    /* type not found */
    return -1;
}

/*
 * gui_bar_get_position: get position number with string
 *                       return -1 if type is not found
 */

int
gui_bar_get_position (char *position)
{
    int i;
    
    for (i = 0; i < GUI_BAR_NUM_POSITIONS; i++)
    {
        if (string_strcasecmp (position, gui_bar_position_str[i]) == 0)
            return i;
    }
    
    /* position not found */
    return -1;
}

/*
 * gui_bar_root_get_size: get total bar size ("root" type) for a position
 */

int
gui_bar_root_get_size (struct t_gui_bar *bar, enum t_gui_bar_position position)
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
            total_size += ptr_bar->current_size;
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
 * gui_bar_search_by_number: search a bar by number
 */

struct t_gui_bar *
gui_bar_search_by_number (int number)
{
    struct t_gui_bar *ptr_bar;
    
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (ptr_bar->number == number)
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
    int type_value, position_value;
    
    if (!name || !name[0])
        return NULL;
    
    /* it's not possible to create 2 bars with same name */
    if (gui_bar_search (name))
        return NULL;
    
    /* look for type */
    type_value = gui_bar_get_type (type);
    if (type_value < 0)
        return NULL;
    
    /* look for position */
    position_value = gui_bar_get_position (position);
    if (position_value < 0)
        return NULL;
    
    /* create bar */
    new_bar = malloc (sizeof (*new_bar));
    if (new_bar)
    {
        new_bar->plugin = plugin;
        new_bar->number = (last_gui_bar) ? last_gui_bar->number + 1 : 1;
        new_bar->name = strdup (name);
        new_bar->type = type_value;
        new_bar->position = position_value;
        new_bar->size = size;
        new_bar->current_size = (size == 0) ? 1 : size;
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
        new_bar->bar_window = NULL;
        
        /* add bar to bars queue */
        new_bar->prev_bar = last_gui_bar;
        if (gui_bars)
            last_gui_bar->next_bar = new_bar;
        else
            gui_bars = new_bar;
        last_gui_bar = new_bar;
        new_bar->next_bar = NULL;
        
        /* add window bar */
        if (type_value == GUI_BAR_TYPE_ROOT)
        {
            /* create only one window for bar */
            gui_bar_window_new (new_bar, NULL);
            gui_window_refresh_needed = 1;
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
        
        return new_bar;
    }
    
    /* failed to create bar */
    return NULL;
}

/*
 * gui_bar_refresh: ask for bar refresh on screen (for all windows where bar is)
 */

void
gui_bar_refresh (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    
    if (bar->type == GUI_BAR_TYPE_ROOT)
        gui_window_refresh_needed = 1;
    else
    {
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            if (gui_bar_window_search_bar (ptr_win, bar))
                ptr_win->refresh_needed = 1;
        }
    }
}

/*
 * gui_bar_set_name: set name for a bar
 */

void
gui_bar_set_name (struct t_gui_bar *bar, char *name)
{
    if (!name || !name[0])
        return;
    
    if (bar->name)
        free (bar->name);
    bar->name = strdup (name);
}

/*
 * gui_bar_set_number: set number for a bar
 */

void
gui_bar_set_number (struct t_gui_bar *bar, int number)
{
    struct t_gui_bar *ptr_bar;
    struct t_gui_window *ptr_win;
    int i;

    if (number < 1)
        number = 1;
    
    /* bar number is already ok? */
    if (number == bar->number)
        return;
    
    /* remove bar from list */
    if (bar == gui_bars)
    {
        gui_bars = bar->next_bar;
        gui_bars->prev_bar = NULL;
    }
    if (bar == last_gui_bar)
    {
        last_gui_bar = bar->prev_bar;
        last_gui_bar->next_bar = NULL;
    }
    if (bar->prev_bar)
        (bar->prev_bar)->next_bar = bar->next_bar;
    if (bar->next_bar)
        (bar->next_bar)->prev_bar = bar->prev_bar;

    if (number == 1)
    {
        gui_bars->prev_bar = bar;
        bar->prev_bar = NULL;
        bar->next_bar = gui_bars;
        gui_bars = bar;
    }
    else
    {
        /* assign new number to all bars */
        i = 1;
        for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
        {
            ptr_bar->number = i++;
        }
        
        ptr_bar = gui_bar_search_by_number (number);
        if (ptr_bar)
        {
            /* add bar before ptr_bar */
            bar->prev_bar = ptr_bar->prev_bar;
            bar->next_bar = ptr_bar;
            if (ptr_bar->prev_bar)
                (ptr_bar->prev_bar)->next_bar = bar;
            ptr_bar->prev_bar = bar;
        }
        else
        {
            /* add to end of list */
            bar->prev_bar = last_gui_bar;
            bar->next_bar = NULL;
            last_gui_bar->next_bar = bar;
            last_gui_bar = bar;
        }
    }
    
    /* assign new number to all bars */
    i = 1;
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        ptr_bar->number = i++;
        gui_bar_free_bar_windows (ptr_bar);
    }
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
        {
            if (ptr_bar->type != GUI_BAR_TYPE_ROOT)
                gui_bar_window_new (ptr_bar, ptr_win);
        }
    }
}

/*
 * gui_bar_set_position: set position for a bar
 */

void
gui_bar_set_position (struct t_gui_bar *bar, char *position)
{
    int position_value;
    
    if (!position || !position[0])
        return;
    
    position_value = gui_bar_get_position (position);
    if ((position_value >= 0) && ((int)bar->position != position_value))
    {
        bar->position = position_value;
    }
}

/*
 * gui_bar_set_current_size: set current size for a bar
 */

void
gui_bar_set_current_size (struct t_gui_bar *bar, int current_size)
{
    if (current_size < 0)
        return;
    
    if (current_size == 0)
        current_size = 1;
    
    /* check if new size is ok if it's more than before */
    if (current_size > bar->current_size
        && !gui_bar_check_size_add (bar, current_size - bar->current_size))
        return;
    
    bar->current_size = current_size;
}

/*
 * gui_bar_set_size: set size for a bar
 */

void
gui_bar_set_size (struct t_gui_bar *bar, int size)
{
    if (size < 0)
        return;
    
    /* check if new size is ok if it's more than before */
    if (size > bar->current_size
        && !gui_bar_check_size_add (bar, size - bar->current_size))
        return;
    
    bar->size = size;
    bar->current_size = (size == 0) ? 1 : size;
}

/*
 * gui_bar_set_items: set items for a bar
 */

void
gui_bar_set_items (struct t_gui_bar *bar, char *items)
{
    if (bar->items)
        free (bar->items);
    if (bar->items_array)
        string_free_exploded (bar->items_array);
    
    if (items && items[0])
    {
        bar->items = strdup (items);
        bar->items_array = string_explode (items, ",", 0, 0,
                                           &bar->items_count);
    }
    else
    {
        bar->items = NULL;
        bar->items_count = 0;
        bar->items_array = NULL;
    }
}

/*
 * gui_bar_set: set a property for bar
 */

void
gui_bar_set (struct t_gui_bar *bar, char *property, char *value)
{
    long number;
    char *error;
    int new_size;
    
    if (!bar || !property || !value)
        return;
    
    if (string_strcasecmp (property, "name") == 0)
    {
        gui_bar_set_name (bar, value);
    }
    if (string_strcasecmp (property, "number") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
        {
            gui_bar_set_number (bar, number);
            gui_window_refresh_needed = 1;
        }
    }
    else if (string_strcasecmp (property, "position") == 0)
    {
        gui_bar_set_position (bar, value);
        gui_bar_refresh (bar);
    }
    else if (string_strcasecmp (property, "size") == 0)
    {
        error = NULL;
        number = strtol (((value[0] == '+') || (value[0] == '-')) ?
                         value + 1 : value,
                         &error,
                         10);
        if (!error || error[0])
            return;
        if (value[0] == '+')
            new_size = bar->current_size + number;
        else if (value[0] == '-')
            new_size = bar->current_size - number;
        else
            new_size = number;
        if ((value[0] == '-') && (new_size < 1))
            return;
        gui_bar_set_size (bar, new_size);
        gui_bar_refresh (bar);
    }
    else if (string_strcasecmp (property, "separator") == 0)
    {
        bar->separator = (string_strcasecmp (value, "1") == 0) ? 1 : 0;
        gui_bar_refresh (bar);
    }
    else if (string_strcasecmp (property, "items") == 0)
    {
        gui_bar_set_items (bar, value);
        gui_bar_draw (bar);
    }
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
    /* remove bar window(s) */
    if (bar->bar_window)
    {
        gui_bar_window_free (bar->bar_window, NULL);
        gui_window_refresh_needed = 1;
    }
    else
        gui_bar_free_bar_windows (bar);
    
    /* remove bar from bars list */
    if (bar->prev_bar)
        (bar->prev_bar)->next_bar = bar->next_bar;
    if (bar->next_bar)
        (bar->next_bar)->prev_bar = bar->prev_bar;
    if (gui_bars == bar)
        gui_bars = bar->next_bar;
    if (last_gui_bar == bar)
        last_gui_bar = bar->prev_bar;
    
    /* free data */
    if (bar->name)
        free (bar->name);
    if (bar->items)
        free (bar->items);
    if (bar->items_array)
        string_free_exploded (bar->items_array);
    
    free (bar);
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
        log_printf ("  current_size . . . . . : %d",   ptr_bar->current_size);
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
