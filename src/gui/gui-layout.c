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

/* gui-layout.c: layout functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../core/weechat.h"
#include "../core/wee-log.h"
#include "../core/wee-config.h"
#include "../core/wee-string.h"
#include "../plugins/plugin.h"
#include "gui-layout.h"
#include "gui-buffer.h"
#include "gui-window.h"


struct t_gui_layout_buffer *gui_layout_buffers = NULL;
struct t_gui_layout_buffer *last_gui_layout_buffer = NULL;

struct t_gui_layout_window *gui_layout_windows = NULL;

int internal_id = 0;


/*
 * gui_layout_buffer_remove: remove a buffer layout
 */

void
gui_layout_buffer_remove (struct t_gui_layout_buffer *layout_buffer)
{
    /* free data */
    if (layout_buffer->plugin_name)
        free (layout_buffer->plugin_name);
    if (layout_buffer->buffer_name)
        free (layout_buffer->buffer_name);
    
    /* remove layout from list */
    if (layout_buffer->prev_layout)
        (layout_buffer->prev_layout)->next_layout = layout_buffer->next_layout;
    if (layout_buffer->next_layout)
        (layout_buffer->next_layout)->prev_layout = layout_buffer->prev_layout;
    if (gui_layout_buffers == layout_buffer)
        gui_layout_buffers = layout_buffer->next_layout;
    if (last_gui_layout_buffer == layout_buffer)
        last_gui_layout_buffer = layout_buffer->prev_layout;
    
    free (layout_buffer);
}

/*
 * gui_layout_buffer_remove_all: remove all buffer layouts
 */

void
gui_layout_buffer_remove_all ()
{
    while (gui_layout_buffers)
    {
        gui_layout_buffer_remove (gui_layout_buffers);
    }
}

/*
 * gui_layout_buffer_reset: reset layout for buffers
 */

void
gui_layout_buffer_reset ()
{
    struct t_gui_buffer *ptr_buffer;
    
    gui_layout_buffer_remove_all ();
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        ptr_buffer->layout_number = 0;
    }
}

/*
 * gui_layout_buffer_add: add a buffer layout
 */

struct t_gui_layout_buffer *
gui_layout_buffer_add (const char *plugin_name, const char *buffer_name,
                       int number)
{
    struct t_gui_layout_buffer *new_layout_buffer;
    
    new_layout_buffer = malloc (sizeof (*new_layout_buffer));
    if (new_layout_buffer)
    {
        /* init layout buffer */
        new_layout_buffer->plugin_name = strdup (plugin_name);
        new_layout_buffer->buffer_name = strdup (buffer_name);
        new_layout_buffer->number = number;
        
        /* add layout buffer to list */
        new_layout_buffer->prev_layout = last_gui_layout_buffer;
        if (gui_layout_buffers)
            last_gui_layout_buffer->next_layout = new_layout_buffer;
        else
            gui_layout_buffers = new_layout_buffer;
        last_gui_layout_buffer = new_layout_buffer;
        new_layout_buffer->next_layout = NULL;
    }
    
    return new_layout_buffer;
}

/*
 * gui_layout_buffer_save: save current layout for buffers
 */

void
gui_layout_buffer_save ()
{
    struct t_gui_buffer *ptr_buffer;
    
    gui_layout_buffer_remove_all ();
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_layout_buffer_add (plugin_get_name (ptr_buffer->plugin),
                               ptr_buffer->name,
                               ptr_buffer->number);
    }
}

/*
 * gui_layout_buffer_get_number: get number for a plugin/buffer
 *                               return 0 if not found
 */

int
gui_layout_buffer_get_number (const char *plugin_name, const char *buffer_name)
{
    struct t_gui_layout_buffer *ptr_layout_buffer;
    
    for (ptr_layout_buffer = gui_layout_buffers; ptr_layout_buffer;
         ptr_layout_buffer = ptr_layout_buffer->next_layout)
    {
        if ((string_strcasecmp (ptr_layout_buffer->plugin_name, plugin_name) == 0)
            && (string_strcasecmp (ptr_layout_buffer->buffer_name, buffer_name) == 0))
        {
            return ptr_layout_buffer->number;
        }
    }
    
    /* plugin/buffer not found */
    return 0;
}

/*
 * gui_layout_buffer_apply: apply current layout for buffers
 */

void
gui_layout_buffer_apply ()
{
    struct t_gui_buffer *ptr_buffer;
    char *plugin_name;
    
    if (gui_layout_buffers)
    {
        for (ptr_buffer = gui_buffers; ptr_buffer;
             ptr_buffer = ptr_buffer->next_buffer)
        {
            plugin_name = plugin_get_name (ptr_buffer->plugin);
            ptr_buffer->layout_number = gui_layout_buffer_get_number (plugin_name,
                                                                      ptr_buffer->name);
            if ((ptr_buffer->layout_number > 0)
                && (ptr_buffer->layout_number != ptr_buffer->number))
            {
                gui_buffer_move_to_number (ptr_buffer,
                                           ptr_buffer->layout_number);
            }
        }
    }
}

/*
 * gui_layout_window_remove: remove a window layout
 */

void
gui_layout_window_remove (struct t_gui_layout_window *layout_window)
{
    /* first free childs */
    if (layout_window->child1)
        gui_layout_window_remove (layout_window->child1);
    if (layout_window->child2)
        gui_layout_window_remove (layout_window->child2);
    
    /* free data */
    if (layout_window->plugin_name)
        free (layout_window->plugin_name);
    if (layout_window->buffer_name)
        free (layout_window->buffer_name);
    
    free (layout_window);
}

/*
 * gui_layout_window_remove_all: remove all window layouts
 */

void
gui_layout_window_remove_all ()
{
    if (gui_layout_windows)
    {
        gui_layout_window_remove (gui_layout_windows);
        gui_layout_windows = NULL;
    }
}

/*
 * gui_layout_window_reset: reset layout for windows
 */

void
gui_layout_window_reset ()
{
    struct t_gui_window *ptr_win;
    
    gui_layout_window_remove_all ();
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->layout_plugin_name)
        {
            free (ptr_win->layout_plugin_name);
            ptr_win->layout_plugin_name = NULL;
        }
        if (ptr_win->layout_buffer_name)
        {
            free (ptr_win->layout_buffer_name);
            ptr_win->layout_buffer_name = NULL;
        }
    }
}

/*
 * gui_layout_window_search_by_id_tree: search a layout window by internal id
 *                                      in a tree
 */

struct t_gui_layout_window *
gui_layout_window_search_by_id_tree (struct t_gui_layout_window *tree, int id)
{
    struct t_gui_layout_window *res;
    
    if (tree->internal_id == id)
        return tree;
    
    if (tree->child1)
    {
        res = gui_layout_window_search_by_id_tree (tree->child1, id);
        if (res)
            return res;
    }
    
    if (tree->child2)
    {
        res = gui_layout_window_search_by_id_tree (tree->child2, id);
        if (res)
            return res;
    }
    
    return NULL;
}

/*
 * gui_layout_window_search_by_id: search a layout window by internal id
 */

struct t_gui_layout_window *
gui_layout_window_search_by_id (int id)
{
    if (!gui_layout_windows)
        return NULL;
    
    return gui_layout_window_search_by_id_tree (gui_layout_windows, id);
}

/*
 * gui_layout_window_add: add a window layout
 */

struct t_gui_layout_window *
gui_layout_window_add (int internal_id,
                       struct t_gui_layout_window *parent,
                       int split_pct, int split_horiz,
                       const char *plugin_name, const char *buffer_name)
{
    struct t_gui_layout_window *new_layout_window;
    
    new_layout_window = malloc (sizeof (*new_layout_window));
    if (new_layout_window)
    {
        /* init layout window */
        new_layout_window->internal_id = internal_id;
        new_layout_window->parent_node = parent;
        new_layout_window->split_pct = split_pct;
        new_layout_window->split_horiz = split_horiz;
        new_layout_window->child1 = NULL;
        new_layout_window->child2 = NULL;
        new_layout_window->plugin_name = (plugin_name) ? strdup (plugin_name) : NULL;
        new_layout_window->buffer_name = (buffer_name) ? strdup (buffer_name) : NULL;
        
        if (parent)
        {
            /* assign this window to child1 or child2 of parent */
            if (!parent->child1)
                parent->child1 = new_layout_window;
            else if (!parent->child2)
                parent->child2 = new_layout_window;
        }
        else
        {
            /* no parent? => it's root! */
            gui_layout_windows = new_layout_window;
        }
    }
    
    return new_layout_window;
}

/*
 * gui_layout_window_save_tree: save tree of windows
 */

void
gui_layout_window_save_tree (struct t_gui_layout_window *parent_layout,
                             struct t_gui_window_tree *tree)
{
    struct t_gui_layout_window *layout_window;
    
    if (tree->window)
    {
        layout_window = gui_layout_window_add (internal_id++,
                                               parent_layout,
                                               0, 0,
                                               plugin_get_name (tree->window->buffer->plugin),
                                               tree->window->buffer->name);
    }
    else
    {
        layout_window = gui_layout_window_add (internal_id++,
                                               parent_layout,
                                               tree->split_pct,
                                               tree->split_horiz,
                                               NULL,
                                               NULL);
    }
    
    if (tree->child1)
        gui_layout_window_save_tree (layout_window, tree->child1);
    
    if (tree->child2)
        gui_layout_window_save_tree (layout_window, tree->child2);
}

/*
 * gui_layout_window_save: save current layout for windows
 */

void
gui_layout_window_save ()
{
    gui_layout_window_remove_all ();
    
    internal_id = 1;
    gui_layout_window_save_tree (NULL, gui_windows_tree);
}

/*
 * gui_layout_window_check_buffer: check if buffer can be assigned to one window
 */

void
gui_layout_window_check_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_window *ptr_win;
    char *plugin_name;
    
    plugin_name = plugin_get_name (buffer->plugin);
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->layout_plugin_name && ptr_win->layout_buffer_name)
        {
            if ((strcmp (ptr_win->layout_plugin_name, plugin_name) == 0)
                && (strcmp (ptr_win->layout_buffer_name, buffer->name) == 0))
            {
                gui_window_switch_to_buffer (ptr_win, buffer, 0);
            }
        }
    }
}

/*
 * gui_layout_window_check_all_buffers: for each window, check if another
 *                                      buffer should be assigned, and if yes,
 *                                      assign it
 */

void
gui_layout_window_check_all_buffers ()
{
    struct t_gui_window *ptr_win;
    struct t_gui_buffer *ptr_buffer;
    char *plugin_name;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->layout_plugin_name && ptr_win->layout_buffer_name)
        {
            for (ptr_buffer = gui_buffers; ptr_buffer;
                 ptr_buffer = ptr_buffer->next_buffer)
            {
                plugin_name = plugin_get_name (ptr_buffer->plugin);
                
                if ((strcmp (ptr_win->layout_plugin_name, plugin_name) == 0)
                    && (strcmp (ptr_win->layout_buffer_name, ptr_buffer->name) == 0))
                {
                    gui_window_switch_to_buffer (ptr_win, ptr_buffer, 0);
                    break;
                }
            }
        }
    }
}

/*
 * gui_layout_window_apply_tree: apply tree windows (resplit screen according
 *                               to windows tree and assing buffer to windows)
 */

void
gui_layout_window_apply_tree (struct t_gui_layout_window *layout_window)
{
    struct t_gui_window *new_window, *old_window;
    
    if (layout_window->split_pct != 0)
    {
        /* node */
        old_window = gui_current_window;
        
        if (layout_window->split_horiz)
        {
            new_window = gui_window_split_horiz (gui_current_window,
                                                 layout_window->split_pct);
        }
        else
        {
            new_window = gui_window_split_vertic (gui_current_window,
                                                  layout_window->split_pct);
        }
        
        if (layout_window->child2)
            gui_layout_window_apply_tree (layout_window->child2);
        
        if (old_window != gui_current_window)
            gui_window_switch (old_window);
        
        if (layout_window->child1)
            gui_layout_window_apply_tree (layout_window->child1);
    }
    else
    {
        /* leaf */
        gui_window_set_layout_plugin_name (gui_current_window,
                                           layout_window->plugin_name);
        gui_window_set_layout_buffer_name (gui_current_window,
                                           layout_window->buffer_name);
    }
}

/*
 * gui_layout_window_apply: apply current layout for windows
 */

void
gui_layout_window_apply ()
{
    struct t_gui_window *old_window;
    
    if (gui_layout_windows)
    {
        gui_window_merge_all (gui_current_window);
        
        old_window = gui_current_window;
        
        gui_layout_window_apply_tree (gui_layout_windows);
        
        gui_layout_window_check_all_buffers ();
        
        gui_window_switch (old_window);
    }
}

/*
 * gui_layout_save_on_exit: save layout according to option
 *                          "save_layout_on_exit"
 */

void
gui_layout_save_on_exit ()
{
    /* save layout on exit */
    switch (CONFIG_BOOLEAN(config_look_save_layout_on_exit))
    {
        case CONFIG_LOOK_SAVE_LAYOUT_ON_EXIT_NONE:
            break;
        case CONFIG_LOOK_SAVE_LAYOUT_ON_EXIT_BUFFERS:
            gui_layout_buffer_save ();
            break;
        case CONFIG_LOOK_SAVE_LAYOUT_ON_EXIT_WINDOWS:
            gui_layout_window_save ();
            break;
        case CONFIG_LOOK_SAVE_LAYOUT_ON_EXIT_ALL:
            gui_layout_buffer_save ();
            gui_layout_window_save ();
            break;
    }
}

/*
 * gui_layout_print_log_window: print windows layout infos in log (usually for
 *                              crash dump)
 */

void
gui_layout_print_log_window (struct t_gui_layout_window *layout_window,
                             int level)
{
    log_printf ("");
    log_printf ("[layout window (addr:0x%x) (%s) (level %d)]",
                layout_window,
                (layout_window->plugin_name) ? "leaf" : "node",
                level);
    
    log_printf ("  internal_id. . . . . . : %d",   layout_window->internal_id);
    log_printf ("  parent_node. . . . . . : 0x%x", layout_window->parent_node);
    log_printf ("  split_pct. . . . . . . : %d",   layout_window->split_pct);
    log_printf ("  split_horiz. . . . . . : %d",   layout_window->split_horiz);
    log_printf ("  child1 . . . . . . . . : 0x%x", layout_window->child1);
    log_printf ("  child2 . . . . . . . . : 0x%x", layout_window->child2);
    log_printf ("  plugin_name. . . . . . : '%s'", layout_window->plugin_name);
    log_printf ("  buffer_name. . . . . . : '%s'", layout_window->buffer_name);
    
    if (layout_window->child1)
        gui_layout_print_log_window (layout_window->child1, level + 1);
    
    if (layout_window->child2)
        gui_layout_print_log_window (layout_window->child2, level + 1);
}

/*
 * gui_layout_print_log: print layout infos in log (usually for crash dump)
 */

void
gui_layout_print_log ()
{
    struct t_gui_layout_buffer *ptr_layout_buffer;
    
    log_printf ("");
    
    for (ptr_layout_buffer = gui_layout_buffers; ptr_layout_buffer;
         ptr_layout_buffer = ptr_layout_buffer->next_layout)
    {
        log_printf ("");
        log_printf ("[layout buffer (addr:0x%x)]", ptr_layout_buffer);
        log_printf ("  plugin_name. . . . . . : '%s'", ptr_layout_buffer->plugin_name);
        log_printf ("  buffer_name. . . . . . : '%s'", ptr_layout_buffer->buffer_name);
        log_printf ("  number . . . . . . . . : %d",   ptr_layout_buffer->number);
        log_printf ("  prev_layout. . . . . . : 0x%x", ptr_layout_buffer->prev_layout);
        log_printf ("  next_layout. . . . . . : 0x%x", ptr_layout_buffer->next_layout);
    }
    
    if (gui_layout_windows)
        gui_layout_print_log_window (gui_layout_windows, 0);
}
