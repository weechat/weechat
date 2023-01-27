/*
 * gui-layout.c - layout functions (used by all GUI)
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include "../core/wee-config.h"
#include "../core/wee-hdata.h"
#include "../core/wee-infolist.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../plugins/plugin.h"
#include "gui-layout.h"
#include "gui-buffer.h"
#include "gui-window.h"


struct t_gui_layout *gui_layouts = NULL;
struct t_gui_layout *last_gui_layout = NULL;
struct t_gui_layout *gui_layout_current = NULL;


/*
 * Searches a layout by name.
 *
 * Returns pointer to layout found, NULL if not found.
 */

struct t_gui_layout *
gui_layout_search (const char *name)
{
    struct t_gui_layout *ptr_layout;

    if (!name)
        return NULL;

    for (ptr_layout = gui_layouts; ptr_layout;
         ptr_layout = ptr_layout->next_layout)
    {
        if (strcmp (ptr_layout->name, name) == 0)
            return ptr_layout;
    }

    /* layout not found */
    return NULL;
}

/*
 * Allocates a new layout.
 *
 * Note: the layout is not added to the list (a call to gui_layout_add() will do
 * that).
 *
 * Returns pointer to new layout, NULL if error.
 */

struct t_gui_layout *
gui_layout_alloc (const char *name)
{
    struct t_gui_layout *new_layout;

    if (!name || !name[0])
        return NULL;

    /* create new layout */
    new_layout = malloc (sizeof (*new_layout));
    if (!new_layout)
        return NULL;

    new_layout->name = strdup (name);
    new_layout->layout_buffers = NULL;
    new_layout->last_layout_buffer = NULL;
    new_layout->layout_windows = NULL;
    new_layout->internal_id = 0;
    new_layout->internal_id_current_window = 0;
    new_layout->prev_layout = NULL;
    new_layout->next_layout = NULL;

    return new_layout;
}

/*
 * Adds a layout in "gui_layouts".
 *
 * Returns:
 *   1: layout added
 *   0: layout not added (another layout exists with same name)
 */

int
gui_layout_add (struct t_gui_layout *layout)
{
    struct t_gui_layout *ptr_layout;

    if (!layout)
        return 0;

    ptr_layout = gui_layout_search (layout->name);
    if (ptr_layout)
        return 0;

    layout->prev_layout = last_gui_layout;
    layout->next_layout = NULL;
    if (last_gui_layout)
        last_gui_layout->next_layout = layout;
    else
        gui_layouts = layout;
    last_gui_layout = layout;

    return 1;
}

/*
 * Renames a layout.
 */

void
gui_layout_rename (struct t_gui_layout *layout, const char *new_name)
{
    if (!layout || !new_name || !new_name[0])
        return;

    if (layout->name)
        free (layout->name);
    layout->name = strdup (new_name);
}

/*
 * Removes a buffer layout from a layout.
 */

void
gui_layout_buffer_remove (struct t_gui_layout *layout,
                          struct t_gui_layout_buffer *layout_buffer)
{
    if (!layout)
        return;

    /* remove layout from list */
    if (layout_buffer->prev_layout)
        (layout_buffer->prev_layout)->next_layout = layout_buffer->next_layout;
    if (layout_buffer->next_layout)
        (layout_buffer->next_layout)->prev_layout = layout_buffer->prev_layout;
    if (layout->layout_buffers == layout_buffer)
        layout->layout_buffers = layout_buffer->next_layout;
    if (layout->last_layout_buffer == layout_buffer)
        layout->last_layout_buffer = layout_buffer->prev_layout;

    /* free data */
    if (layout_buffer->plugin_name)
        free (layout_buffer->plugin_name);
    if (layout_buffer->buffer_name)
        free (layout_buffer->buffer_name);

    free (layout_buffer);
}

/*
 * Removes all buffer layouts from a layout.
 */

void
gui_layout_buffer_remove_all (struct t_gui_layout *layout)
{
    if (!layout)
        return;

    while (layout->layout_buffers)
    {
        gui_layout_buffer_remove (layout, layout->layout_buffers);
    }
}

/*
 * Resets layout_number in all buffers.
 */

void
gui_layout_buffer_reset ()
{
    struct t_gui_buffer *ptr_buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        ptr_buffer->layout_number = 0;
    }
}

/*
 * Adds a buffer layout in a layout.
 *
 * Returns pointer to buffer layout, NULL if error.
 */

struct t_gui_layout_buffer *
gui_layout_buffer_add (struct t_gui_layout *layout,
                       const char *plugin_name, const char *buffer_name,
                       int number)
{
    struct t_gui_layout_buffer *new_layout_buffer;

    if (!layout || !plugin_name || !buffer_name)
        return NULL;

    new_layout_buffer = malloc (sizeof (*new_layout_buffer));
    if (new_layout_buffer)
    {
        /* init layout buffer */
        new_layout_buffer->plugin_name = strdup (plugin_name);
        new_layout_buffer->buffer_name = strdup (buffer_name);
        new_layout_buffer->number = number;

        /* add layout buffer to list */
        new_layout_buffer->prev_layout = layout->last_layout_buffer;
        if (layout->last_layout_buffer)
            (layout->last_layout_buffer)->next_layout = new_layout_buffer;
        else
            layout->layout_buffers = new_layout_buffer;
        layout->last_layout_buffer = new_layout_buffer;
        new_layout_buffer->next_layout = NULL;
    }

    return new_layout_buffer;
}

/*
 * Gets layout number for a plugin/buffer.
 */

void
gui_layout_buffer_get_number (struct t_gui_layout *layout,
                              const char *plugin_name, const char *buffer_name,
                              int *layout_number,
                              int *layout_number_merge_order)
{
    struct t_gui_layout_buffer *ptr_layout_buffer;
    int old_number, merge_order;

    *layout_number = 0;
    *layout_number_merge_order = 0;

    if (!layout || !plugin_name || !buffer_name)
        return;

    old_number = -1;
    merge_order = 0;

    for (ptr_layout_buffer = layout->layout_buffers; ptr_layout_buffer;
         ptr_layout_buffer = ptr_layout_buffer->next_layout)
    {
        if (ptr_layout_buffer->number != old_number)
        {
            old_number = ptr_layout_buffer->number;
            merge_order = 0;
        }
        else
            merge_order++;

        if ((strcmp (ptr_layout_buffer->plugin_name, plugin_name) == 0)
            && (strcmp (ptr_layout_buffer->buffer_name, buffer_name) == 0))
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
gui_layout_buffer_get_number_all (struct t_gui_layout *layout)
{
    struct t_gui_buffer *ptr_buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_layout_buffer_get_number (layout,
                                      gui_buffer_get_plugin_name (ptr_buffer),
                                      ptr_buffer->name,
                                      &(ptr_buffer->layout_number),
                                      &(ptr_buffer->layout_number_merge_order));
    }
}

/*
 * Stores current layout for buffers in a layout.
 */

void
gui_layout_buffer_store (struct t_gui_layout *layout)
{
    struct t_gui_buffer *ptr_buffer;

    if (!layout)
        return;

    gui_layout_buffer_remove_all (layout);

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_layout_buffer_add (layout,
                               gui_buffer_get_plugin_name (ptr_buffer),
                               ptr_buffer->name,
                               ptr_buffer->number);
    }

    /* get layout number for all buffers */
    gui_layout_buffer_get_number_all (layout);
}

/*
 * Applies a layout for buffers.
 */

void
gui_layout_buffer_apply (struct t_gui_layout *layout)
{
    struct t_gui_buffer *ptr_buffer;

    if (!layout)
        return;

    /* get layout number for all buffers */
    gui_layout_buffer_get_number_all (layout);

    /* unmerge all buffers */
    gui_buffer_unmerge_all ();

    /* sort buffers by layout number (without merge) */
    gui_buffer_sort_by_layout_number ();

    /* set appropriate active buffers */
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if ((gui_buffer_count_merged_buffers (ptr_buffer->number) > 1)
            && (ptr_buffer->layout_number == ptr_buffer->number)
            && (ptr_buffer->layout_number_merge_order == 0))
        {
            gui_buffer_set_active_buffer (ptr_buffer);
        }
    }
}

/*
 * Removes a window layout.
 */

void
gui_layout_window_remove (struct t_gui_layout_window *layout_window)
{
    if (!layout_window)
        return;

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
 * Removes all window layouts from a layout.
 */

void
gui_layout_window_remove_all (struct t_gui_layout *layout)
{
    if (!layout)
        return;

    if (layout->layout_windows)
    {
        gui_layout_window_remove (layout->layout_windows);
        layout->layout_windows = NULL;
    }
}


/*
 * Resets layout for windows.
 */

void
gui_layout_window_reset ()
{
    struct t_gui_window *ptr_win;

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
 * Searches for a window layout by internal id.
 *
 * Returns pointer to window layout found, NULL if not found.
 */

struct t_gui_layout_window *
gui_layout_window_search_by_id (struct t_gui_layout_window *layout_window,
                                int id)
{
    struct t_gui_layout_window *res;

    if (!layout_window)
        return NULL;

    if (layout_window->internal_id == id)
        return layout_window;

    if (layout_window->child1)
    {
        res = gui_layout_window_search_by_id (layout_window->child1, id);
        if (res)
            return res;
    }

    if (layout_window->child2)
    {
        res = gui_layout_window_search_by_id (layout_window->child2, id);
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
gui_layout_window_add (struct t_gui_layout_window **layout_window,
                       int internal_id,
                       struct t_gui_layout_window *parent,
                       int split_pct, int split_horiz,
                       const char *plugin_name, const char *buffer_name)
{
    struct t_gui_layout_window *new_layout_window;

    if (!layout_window)
        return NULL;

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
            *layout_window = new_layout_window;
        }
    }

    return new_layout_window;
}

/*
 * Stores tree of windows.
 */

void
gui_layout_window_store_tree (struct t_gui_layout *layout,
                             struct t_gui_layout_window **layout_windows,
                             struct t_gui_layout_window *parent_layout,
                             struct t_gui_window_tree *tree)
{
    struct t_gui_layout_window *layout_window;

    if (tree->window)
    {
        if (tree->window == gui_current_window)
            layout->internal_id_current_window = layout->internal_id;

        layout_window = gui_layout_window_add (layout_windows,
                                               layout->internal_id,
                                               parent_layout,
                                               0, 0,
                                               gui_buffer_get_plugin_name (tree->window->buffer),
                                               tree->window->buffer->name);
    }
    else
    {
        layout_window = gui_layout_window_add (layout_windows,
                                               layout->internal_id,
                                               parent_layout,
                                               tree->split_pct,
                                               tree->split_horizontal,
                                               NULL,
                                               NULL);
    }

    layout->internal_id++;

    if (tree->child1)
    {
        gui_layout_window_store_tree (layout, layout_windows,
                                     layout_window, tree->child1);
    }

    if (tree->child2)
    {
        gui_layout_window_store_tree (layout, layout_windows,
                                     layout_window, tree->child2);
    }
}

/*
 * Stores current layout for windows in a layout.
 *
 * Returns internal id of current window.
 */

void
gui_layout_window_store (struct t_gui_layout *layout)
{
    if (!layout)
        return;

    gui_layout_window_remove (layout->layout_windows);

    layout->internal_id = 1;
    layout->internal_id_current_window = -1;

    gui_layout_window_store_tree (layout, &layout->layout_windows, NULL,
                                 gui_windows_tree);
}

/*
 * Checks whether a window has its layout buffer displayed or not.
 *
 * Returns:
 *    1: the window has layout info and the proper buffer displayed
 *    0: the window has layout info but NOT the proper buffer displayed
 *   -1: the window has no layout info
 */

int
gui_layout_window_check_buffer (struct t_gui_window *window)
{
    /* no layout? return -1 */
    if (!window->layout_plugin_name || !window->layout_buffer_name)
        return -1;

    /* layout and buffer displayed matches? return 1 */
    if ((strcmp (window->layout_plugin_name,
                 gui_buffer_get_plugin_name (window->buffer)) == 0)
        && (strcmp (window->layout_buffer_name, (window->buffer)->name) == 0))
    {
        return 1;
    }

    /* buffer displayed does not match the layout, return 0 */
    return 0;
}

/*
 * Assigns a buffer to windows.
 */

void
gui_layout_window_assign_buffer (struct t_gui_buffer *buffer)
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
gui_layout_window_assign_all_buffers ()
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
                              int internal_id_current_window,
                              struct t_gui_window **current_window)
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
        {
            gui_layout_window_apply_tree (layout_window->child2,
                                          internal_id_current_window,
                                          current_window);
        }

        if (old_window != gui_current_window)
            gui_window_switch (old_window);

        if (layout_window->child1)
        {
            gui_layout_window_apply_tree (layout_window->child1,
                                          internal_id_current_window,
                                          current_window);
        }
    }
    else
    {
        /* leaf */
        if (layout_window->internal_id == internal_id_current_window)
            *current_window = gui_current_window;

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
gui_layout_window_apply (struct t_gui_layout *layout,
                         int internal_id_current_window)
{
    struct t_gui_window *old_window, *ptr_current_window;

    if (!layout || !layout->layout_windows)
        return;

    gui_window_merge_all (gui_current_window);

    old_window = gui_current_window;
    ptr_current_window = NULL;

    gui_layout_window_apply_tree (layout->layout_windows,
                                  internal_id_current_window,
                                  &ptr_current_window);

    gui_layout_window_assign_all_buffers ();

    gui_window_switch ((ptr_current_window) ? ptr_current_window : old_window);
}

/*
 * Stores layout according to option "store_layout_on_exit".
 */

void
gui_layout_store_on_exit ()
{
    struct t_gui_layout *ptr_layout;

    if (CONFIG_BOOLEAN(config_look_save_layout_on_exit) == CONFIG_LOOK_SAVE_LAYOUT_ON_EXIT_NONE)
        return;

    ptr_layout = gui_layout_current;
    if (!ptr_layout)
    {
        /* create a "default" layout if needed */
        ptr_layout = gui_layout_search (GUI_LAYOUT_DEFAULT_NAME);
        if (!ptr_layout)
        {
            ptr_layout = gui_layout_alloc (GUI_LAYOUT_DEFAULT_NAME);
            if (!ptr_layout)
                return;
            gui_layout_add (ptr_layout);
        }
    }

    /* store current layout */
    switch (CONFIG_BOOLEAN(config_look_save_layout_on_exit))
    {
        case CONFIG_LOOK_SAVE_LAYOUT_ON_EXIT_BUFFERS:
            gui_layout_buffer_store (ptr_layout);
            break;
        case CONFIG_LOOK_SAVE_LAYOUT_ON_EXIT_WINDOWS:
            gui_layout_window_store (ptr_layout);
            break;
        case CONFIG_LOOK_SAVE_LAYOUT_ON_EXIT_ALL:
            gui_layout_buffer_store (ptr_layout);
            gui_layout_window_store (ptr_layout);
            break;
        default:
            break;
    }

    /* use layout, so it will be used after restart of WeeChat */
    gui_layout_current = ptr_layout;
}

/*
 * Frees a layout.
 */

void
gui_layout_free (struct t_gui_layout *layout)
{
    if (!layout)
        return;

    /* remove current layout if this is the layout we are removing */
    if (gui_layout_current == layout)
        gui_layout_current = NULL;

    /* remove buffers and windows */
    gui_layout_buffer_remove_all (layout);
    gui_layout_window_remove (layout->layout_windows);

    /* free data */
    if (layout->name)
        free (layout->name);

    free (layout);
}

/*
 * Removes a layout from hashtable "gui_layouts".
 */

void
gui_layout_remove (struct t_gui_layout *layout)
{
    struct t_gui_layout *new_gui_layouts;

    if (!layout)
        return;

    /* remove current layout if this is the layout we are removing */
    if (gui_layout_current == layout)
        gui_layout_current = NULL;

    /* remove alias from list */
    if (last_gui_layout == layout)
        last_gui_layout = layout->prev_layout;
    if (layout->prev_layout)
    {
        (layout->prev_layout)->next_layout = layout->next_layout;
        new_gui_layouts = gui_layouts;
    }
    else
        new_gui_layouts = layout->next_layout;
    if (layout->next_layout)
        (layout->next_layout)->prev_layout = layout->prev_layout;

    /* free data */
    gui_layout_free (layout);

    gui_layouts = new_gui_layouts;
}

/*
 * Removes all layouts from "gui_layouts".
 */

void
gui_layout_remove_all ()
{
    while (gui_layouts)
    {
        gui_layout_remove (gui_layouts);
    }
}

/*
 * Returns hdata for buffer layout.
 */

struct t_hdata *
gui_layout_hdata_layout_buffer_cb (const void *pointer, void *data,
                                   const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_layout", "next_layout",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_layout_buffer, plugin_name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_layout_buffer, buffer_name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_layout_buffer, number, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_layout_buffer, prev_layout, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_layout_buffer, next_layout, POINTER, 0, NULL, hdata_name);
    }
    return hdata;
}

/*
 * Returns hdata for window layout.
 */

struct t_hdata *
gui_layout_hdata_layout_window_cb (const void *pointer, void *data,
                                   const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, NULL, NULL, 0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_layout_window, internal_id, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_layout_window, parent_node, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_layout_window, split_pct, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_layout_window, split_horiz, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_layout_window, child1, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_layout_window, child2, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_layout_window, plugin_name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_layout_window, buffer_name, STRING, 0, NULL, NULL);
    }
    return hdata;
}

/*
 * Returns hdata for layout.
 */

struct t_hdata *
gui_layout_hdata_layout_cb (const void *pointer, void *data,
                            const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_layout", "next_layout",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_layout, name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_layout, layout_buffers, POINTER, 0, NULL, "layout_buffer");
        HDATA_VAR(struct t_gui_layout, last_layout_buffer, POINTER, 0, NULL, "layout_buffer");
        HDATA_VAR(struct t_gui_layout, layout_windows, POINTER, 0, NULL, "layout_window");
        HDATA_VAR(struct t_gui_layout, internal_id, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_layout, internal_id_current_window, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_layout, prev_layout, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_layout, next_layout, POINTER, 0, NULL, hdata_name);
        HDATA_LIST(gui_layouts, WEECHAT_HDATA_LIST_CHECK_POINTERS);
        HDATA_LIST(last_gui_layout, 0);
        HDATA_LIST(gui_layout_current, 0);
    }
    return hdata;
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
 * Adds a layout in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_layout_add_to_infolist (struct t_infolist *infolist,
                            struct t_gui_layout *layout)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !layout)
        return 0;

    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_integer (ptr_item, "current_layout",
                                   (gui_layout_current == layout) ? 1 : 0))
        return 0;
    if (!infolist_new_var_string (ptr_item, "name", layout->name))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "internal_id", layout->internal_id))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "internal_id_current_window", layout->internal_id_current_window))
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
    log_printf ("  [layout window (addr:0x%lx) (%s) (level %d)]",
                layout_window,
                (layout_window->plugin_name) ? "leaf" : "node",
                level);

    log_printf ("    internal_id. . . . . : %d",    layout_window->internal_id);
    log_printf ("    parent_node. . . . . : 0x%lx", layout_window->parent_node);
    log_printf ("    split_pct. . . . . . : %d",    layout_window->split_pct);
    log_printf ("    split_horiz. . . . . : %d",    layout_window->split_horiz);
    log_printf ("    child1 . . . . . . . : 0x%lx", layout_window->child1);
    log_printf ("    child2 . . . . . . . : 0x%lx", layout_window->child2);
    log_printf ("    plugin_name. . . . . : '%s'",  layout_window->plugin_name);
    log_printf ("    buffer_name. . . . . : '%s'",  layout_window->buffer_name);

    if (layout_window->child1)
        gui_layout_print_log_window (layout_window->child1, level + 1);

    if (layout_window->child2)
        gui_layout_print_log_window (layout_window->child2, level + 1);
}

/*
 * Prints layouts in WeeChat log file (usually for crash dump).
 */

void
gui_layout_print_log ()
{
    struct t_gui_layout *ptr_layout;
    struct t_gui_layout_buffer *ptr_layout_buffer;

    log_printf ("");
    log_printf ("gui_layouts . . . . . . . . . : 0x%lx", gui_layouts);
    log_printf ("last_gui_layout . . . . . . . : 0x%lx", last_gui_layout);
    log_printf ("gui_layout_current. . . . . . : 0x%lx", gui_layout_current);

    for (ptr_layout = gui_layouts; ptr_layout;
         ptr_layout = ptr_layout->next_layout)
    {
        log_printf ("");
        log_printf ("[layout \"%s\" (addr:0x%lx)]", ptr_layout->name, ptr_layout);
        log_printf ("  layout_buffers . . . . : 0x%lx", ptr_layout->layout_buffers);
        log_printf ("  last_layout_buffer . . : 0x%lx", ptr_layout->last_layout_buffer);
        log_printf ("  layout_windows . . . . : 0x%lx", ptr_layout->layout_windows);
        log_printf ("  internal_id. . . . . . : %d",    ptr_layout->internal_id);
        log_printf ("  internal_id_current_win: %d",    ptr_layout->internal_id_current_window);
        for (ptr_layout_buffer = ptr_layout->layout_buffers; ptr_layout_buffer;
             ptr_layout_buffer = ptr_layout_buffer->next_layout)
        {
            log_printf ("");
            log_printf ("  [layout buffer (addr:0x%lx)]",   ptr_layout_buffer);
            log_printf ("    plugin_name. . . . . : '%s'",  ptr_layout_buffer->plugin_name);
            log_printf ("    buffer_name. . . . . : '%s'",  ptr_layout_buffer->buffer_name);
            log_printf ("    number . . . . . . . : %d",    ptr_layout_buffer->number);
            log_printf ("    prev_layout. . . . . : 0x%lx", ptr_layout_buffer->prev_layout);
            log_printf ("    next_layout. . . . . : 0x%lx", ptr_layout_buffer->next_layout);
        }
        if (ptr_layout->layout_windows)
            gui_layout_print_log_window (ptr_layout->layout_windows, 0);
    }
}
