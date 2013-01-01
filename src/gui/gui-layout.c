/*
 * gui-layout.c - layout functions (used by all GUI)
 *
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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
#include <string.h>

#include "../core/weechat.h"
#include "../core/wee-log.h"
#include "../core/wee-config.h"
#include "../core/wee-infolist.h"
#include "../core/wee-string.h"
#include "../plugins/plugin.h"
#include "gui-layout.h"
#include "gui-buffer.h"
#include "gui-window.h"


struct t_gui_layout_buffer *gui_layout_buffers = NULL;
struct t_gui_layout_buffer *last_gui_layout_buffer = NULL;

struct t_gui_layout_window *gui_layout_windows = NULL;

/* used to attribute a unique id for each window in tree */
int gui_layout_internal_id = 0;

/* internal id of current window, when saving windows layout */
int gui_layout_internal_id_current_window = 0;

/* pointer to current window, found when applying windows layout */
struct t_gui_window *gui_layout_ptr_current_window = NULL;


/*
 * Removes a buffer layout.
 */

void
gui_layout_buffer_remove (struct t_gui_layout_buffer **layout_buffers,
                          struct t_gui_layout_buffer **last_layout_buffer,
                          struct t_gui_layout_buffer *layout_buffer)
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
    if (*layout_buffers == layout_buffer)
        *layout_buffers = layout_buffer->next_layout;
    if (*last_layout_buffer == layout_buffer)
        *last_layout_buffer = layout_buffer->prev_layout;

    free (layout_buffer);
}

/*
 * Removes all buffer layouts.
 */

void
gui_layout_buffer_remove_all (struct t_gui_layout_buffer **layout_buffers,
                              struct t_gui_layout_buffer **last_layout_buffer)
{
    while (*layout_buffers)
    {
        gui_layout_buffer_remove (layout_buffers, last_layout_buffer,
                                  *layout_buffers);
    }
}

/*
 * Resets layout for buffers.
 */

void
gui_layout_buffer_reset (struct t_gui_layout_buffer **layout_buffers,
                         struct t_gui_layout_buffer **last_layout_buffer)
{
    struct t_gui_buffer *ptr_buffer;

    gui_layout_buffer_remove_all (layout_buffers, last_layout_buffer);

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        ptr_buffer->layout_number = 0;
    }
}

/*
 * Adds a buffer layout.
 *
 * Returns pointer to buffer layout, NULL if error.
 */

struct t_gui_layout_buffer *
gui_layout_buffer_add (struct t_gui_layout_buffer **layout_buffers,
                       struct t_gui_layout_buffer **last_layout_buffer,
                       const char *plugin_name, const char *buffer_name,
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
        new_layout_buffer->prev_layout = *last_layout_buffer;
        if (*layout_buffers)
            (*last_layout_buffer)->next_layout = new_layout_buffer;
        else
            *layout_buffers = new_layout_buffer;
        *last_layout_buffer = new_layout_buffer;
        new_layout_buffer->next_layout = NULL;
    }

    return new_layout_buffer;
}

/*
 * Gets layout number for a plugin/buffer.
 */

void
gui_layout_buffer_get_number (struct t_gui_layout_buffer *layout_buffers,
                              const char *plugin_name, const char *buffer_name,
                              int *layout_number,
                              int *layout_number_merge_order)
{
    struct t_gui_layout_buffer *ptr_layout_buffer;
    int old_number, merge_order;

    *layout_number = 0;
    *layout_number_merge_order = 0;

    old_number = -1;
    merge_order = 0;

    for (ptr_layout_buffer = layout_buffers; ptr_layout_buffer;
         ptr_layout_buffer = ptr_layout_buffer->next_layout)
    {
        if (ptr_layout_buffer->number != old_number)
        {
            old_number = ptr_layout_buffer->number;
            merge_order = 0;
        }
        else
            merge_order++;

        if ((string_strcasecmp (ptr_layout_buffer->plugin_name, plugin_name) == 0)
            && (string_strcasecmp (ptr_layout_buffer->buffer_name, buffer_name) == 0))
        {
            *layout_number = ptr_layout_buffer->number;
            *layout_number_merge_order = merge_order;
            return;
        }
    }
}

/*
 * Gets layout numbers for all buffers.
 */

void
gui_layout_buffer_get_number_all (struct t_gui_layout_buffer *layout_buffers)
{
    struct t_gui_buffer *ptr_buffer;

    if (!layout_buffers)
        return;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_layout_buffer_get_number (layout_buffers,
                                      gui_buffer_get_plugin_name (ptr_buffer),
                                      ptr_buffer->name,
                                      &(ptr_buffer->layout_number),
                                      &(ptr_buffer->layout_number_merge_order));
    }
}

/*
 * Saves current layout for buffers.
 */

void
gui_layout_buffer_save (struct t_gui_layout_buffer **layout_buffers,
                        struct t_gui_layout_buffer **last_layout_buffer)
{
    struct t_gui_buffer *ptr_buffer;

    if (!layout_buffers || !last_layout_buffer)
        return;

    gui_layout_buffer_remove_all (layout_buffers, last_layout_buffer);

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_layout_buffer_add (layout_buffers, last_layout_buffer,
                               gui_buffer_get_plugin_name (ptr_buffer),
                               ptr_buffer->name,
                               ptr_buffer->number);
    }

    /* get layout number for all buffers */
    gui_layout_buffer_get_number_all (*layout_buffers);
}

/*
 * Applies a layout for buffers.
 */

void
gui_layout_buffer_apply (struct t_gui_layout_buffer *layout_buffers)
{
    struct t_gui_buffer *ptr_buffer, *ptr_next_buffer;
    int number, count_merged;

    /* get layout number for all buffers */
    gui_layout_buffer_get_number_all (layout_buffers);

    /* unmerge all buffers */
    gui_buffer_unmerge_all ();

    /* sort buffers by layout number (without merge) */
    gui_buffer_sort_by_layout_number ();

    /* merge buffers */
    ptr_buffer = gui_buffers->next_buffer;
    while (ptr_buffer)
    {
        ptr_next_buffer = ptr_buffer->next_buffer;

        if ((ptr_buffer->layout_number >= 1)
            && (ptr_buffer->layout_number == (ptr_buffer->prev_buffer)->layout_number))
        {
            gui_buffer_merge (ptr_buffer, ptr_buffer->prev_buffer);
        }

        ptr_buffer = ptr_next_buffer;
    }

    /* set appropriate active buffers */
    number = 1;
    while (number <= last_gui_buffer->number)
    {
        count_merged = gui_buffer_count_merged_buffers (number);
        if (count_merged > 1)
        {
            ptr_buffer = gui_buffer_search_by_layout_number (number, 0);
            if (ptr_buffer && !ptr_buffer->active)
            {
                gui_buffer_set_active_buffer (ptr_buffer);
            }
        }
        number++;
    }
}

/*
 * Removes a window layout.
 */

void
gui_layout_window_remove (struct t_gui_layout_window *layout_window)
{
    /* first free children */
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
 * Removes all window layouts.
 */

void
gui_layout_window_remove_all (struct t_gui_layout_window **layout_windows)
{
    if (*layout_windows)
    {
        gui_layout_window_remove (*layout_windows);
        *layout_windows = NULL;
    }
}

/*
 * Resets layout for windows.
 */

void
gui_layout_window_reset (struct t_gui_layout_window **layout_windows)
{
    struct t_gui_window *ptr_win;

    gui_layout_window_remove_all (layout_windows);

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
 * Searches for a window layout by internal id in a layout.
 *
 * Returns pointer to window layout found, NULL if not found.
 */

struct t_gui_layout_window *
gui_layout_window_search_by_id (struct t_gui_layout_window *layout_windows,
                                int id)
{
    struct t_gui_layout_window *res;

    if (!layout_windows)
        return NULL;

    if (layout_windows->internal_id == id)
        return layout_windows;

    if (layout_windows->child1)
    {
        res = gui_layout_window_search_by_id (layout_windows->child1, id);
        if (res)
            return res;
    }

    if (layout_windows->child2)
    {
        res = gui_layout_window_search_by_id (layout_windows->child2, id);
        if (res)
            return res;
    }

    return NULL;
}

/*
 * Adds a window layout.
 *
 * Returns pointer to new window layout, NULL if not found.
 */

struct t_gui_layout_window *
gui_layout_window_add (struct t_gui_layout_window **layout_windows,
                       int internal_id,
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
            *layout_windows = new_layout_window;
        }
    }

    return new_layout_window;
}

/*
 * Saves tree of windows.
 */

void
gui_layout_window_save_tree (struct t_gui_layout_window **layout_windows,
                             struct t_gui_layout_window *parent_layout,
                             struct t_gui_window_tree *tree)
{
    struct t_gui_layout_window *layout_window;

    if (tree->window)
    {
        if (tree->window == gui_current_window)
            gui_layout_internal_id_current_window = gui_layout_internal_id;

        layout_window = gui_layout_window_add (layout_windows,
                                               gui_layout_internal_id,
                                               parent_layout,
                                               0, 0,
                                               gui_buffer_get_plugin_name (tree->window->buffer),
                                               tree->window->buffer->name);
    }
    else
    {
        layout_window = gui_layout_window_add (layout_windows,
                                               gui_layout_internal_id,
                                               parent_layout,
                                               tree->split_pct,
                                               tree->split_horizontal,
                                               NULL,
                                               NULL);
    }

    gui_layout_internal_id++;

    if (tree->child1)
        gui_layout_window_save_tree (layout_windows,
                                     layout_window, tree->child1);

    if (tree->child2)
        gui_layout_window_save_tree (layout_windows,
                                     layout_window, tree->child2);
}

/*
 * Saves current layout for windows.
 *
 * Returns internal id of current window.
 */

int
gui_layout_window_save (struct t_gui_layout_window **layout_windows)
{
    gui_layout_window_remove_all (layout_windows);

    gui_layout_internal_id = 1;
    gui_layout_internal_id_current_window = -1;

    gui_layout_window_save_tree (layout_windows, NULL, gui_windows_tree);

    return gui_layout_internal_id_current_window;
}

/*
 * Checks if buffer can be assigned to one window.
 */

void
gui_layout_window_check_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_window *ptr_win;
    const char *plugin_name;

    plugin_name = gui_buffer_get_plugin_name (buffer);

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
 * For each window, checks if another buffer should be assigned, and if yes,
 * assigns it.
 */

void
gui_layout_window_check_all_buffers ()
{
    struct t_gui_window *ptr_win;
    struct t_gui_buffer *ptr_buffer;

    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->layout_plugin_name && ptr_win->layout_buffer_name)
        {
            for (ptr_buffer = gui_buffers; ptr_buffer;
                 ptr_buffer = ptr_buffer->next_buffer)
            {
                if ((strcmp (ptr_win->layout_plugin_name, gui_buffer_get_plugin_name (ptr_buffer)) == 0)
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
 * Applies tree windows (re-splits screen according to windows tree and assigns
 * buffer to windows).
 */

void
gui_layout_window_apply_tree (struct t_gui_layout_window *layout_window,
                              int internal_id_current_window)
{
    struct t_gui_window *old_window;

    if (layout_window->split_pct != 0)
    {
        /* node */
        old_window = gui_current_window;

        if (layout_window->split_horiz)
        {
            gui_window_split_horizontal (gui_current_window,
                                         layout_window->split_pct);
        }
        else
        {
            gui_window_split_vertical (gui_current_window,
                                       layout_window->split_pct);
        }

        if (layout_window->child2)
            gui_layout_window_apply_tree (layout_window->child2,
                                          internal_id_current_window);

        if (old_window != gui_current_window)
            gui_window_switch (old_window);

        if (layout_window->child1)
            gui_layout_window_apply_tree (layout_window->child1,
                                          internal_id_current_window);
    }
    else
    {
        /* leaf */
        if (layout_window->internal_id == internal_id_current_window)
            gui_layout_ptr_current_window = gui_current_window;

        gui_window_set_layout_plugin_name (gui_current_window,
                                           layout_window->plugin_name);
        gui_window_set_layout_buffer_name (gui_current_window,
                                           layout_window->buffer_name);
    }
}

/*
 * Applies current layout for windows.
 */

void
gui_layout_window_apply (struct t_gui_layout_window *layout_windows,
                         int internal_id_current_window)
{
    struct t_gui_window *old_window;

    if (!layout_windows)
        return;

    gui_window_merge_all (gui_current_window);

    old_window = gui_current_window;
    gui_layout_ptr_current_window = NULL;

    gui_layout_window_apply_tree (layout_windows,
                                  internal_id_current_window);

    gui_layout_window_check_all_buffers ();

    gui_window_switch ((gui_layout_ptr_current_window) ?
                       gui_layout_ptr_current_window : old_window);
}

/*
 * Saves layout according to option "save_layout_on_exit".
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
            gui_layout_buffer_save (&gui_layout_buffers, &last_gui_layout_buffer);
            break;
        case CONFIG_LOOK_SAVE_LAYOUT_ON_EXIT_WINDOWS:
            gui_layout_window_save (&gui_layout_windows);
            break;
        case CONFIG_LOOK_SAVE_LAYOUT_ON_EXIT_ALL:
            gui_layout_buffer_save (&gui_layout_buffers, &last_gui_layout_buffer);
            gui_layout_window_save (&gui_layout_windows);
            break;
    }
}

/*
 * Adds a buffer layout in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_layout_buffer_add_to_infolist (struct t_infolist *infolist,
                                   struct t_gui_layout_buffer *layout_buffer)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !layout_buffer)
        return 0;

    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_string (ptr_item, "plugin_name", layout_buffer->plugin_name))
        return 0;
    if (!infolist_new_var_string (ptr_item, "buffer_name", layout_buffer->buffer_name))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "number", layout_buffer->number))
        return 0;

    return 1;
}

/*
 * Adds a window layout in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_layout_window_add_to_infolist (struct t_infolist *infolist,
                                   struct t_gui_layout_window *layout_window)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !layout_window)
        return 0;

    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_integer (ptr_item, "internal_id", layout_window->internal_id))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "parent_id",
                                   (layout_window->parent_node) ?
                                   (layout_window->parent_node)->internal_id : 0))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "parent_node", layout_window->parent_node))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "split_pct", layout_window->split_pct))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "split_horiz", layout_window->split_horiz))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "child1", layout_window->child1))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "child2", layout_window->child2))
        return 0;
    if (!infolist_new_var_string (ptr_item, "plugin_name", layout_window->plugin_name))
        return 0;
    if (!infolist_new_var_string (ptr_item, "buffer_name", layout_window->buffer_name))
        return 0;

    return 1;
}

/*
 * Prints windows layout infos in WeeChat log file (usually for crash dump).
 */

void
gui_layout_print_log_window (struct t_gui_layout_window *layout_window,
                             int level)
{
    log_printf ("");
    log_printf ("[layout window (addr:0x%lx) (%s) (level %d)]",
                layout_window,
                (layout_window->plugin_name) ? "leaf" : "node",
                level);

    log_printf ("  internal_id. . . . . . : %d",    layout_window->internal_id);
    log_printf ("  parent_node. . . . . . : 0x%lx", layout_window->parent_node);
    log_printf ("  split_pct. . . . . . . : %d",    layout_window->split_pct);
    log_printf ("  split_horiz. . . . . . : %d",    layout_window->split_horiz);
    log_printf ("  child1 . . . . . . . . : 0x%lx", layout_window->child1);
    log_printf ("  child2 . . . . . . . . : 0x%lx", layout_window->child2);
    log_printf ("  plugin_name. . . . . . : '%s'",  layout_window->plugin_name);
    log_printf ("  buffer_name. . . . . . : '%s'",  layout_window->buffer_name);

    if (layout_window->child1)
        gui_layout_print_log_window (layout_window->child1, level + 1);

    if (layout_window->child2)
        gui_layout_print_log_window (layout_window->child2, level + 1);
}

/*
 * Prints layout infos in WeeChat log file (usually for crash dump).
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
        log_printf ("[layout buffer (addr:0x%lx)]", ptr_layout_buffer);
        log_printf ("  plugin_name. . . . . . : '%s'",  ptr_layout_buffer->plugin_name);
        log_printf ("  buffer_name. . . . . . : '%s'",  ptr_layout_buffer->buffer_name);
        log_printf ("  number . . . . . . . . : %d",    ptr_layout_buffer->number);
        log_printf ("  prev_layout. . . . . . : 0x%lx", ptr_layout_buffer->prev_layout);
        log_printf ("  next_layout. . . . . . : 0x%lx", ptr_layout_buffer->next_layout);
    }

    if (gui_layout_windows)
        gui_layout_print_log_window (gui_layout_windows, 0);
}
