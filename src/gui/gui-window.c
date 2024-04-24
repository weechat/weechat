/*
 * gui-window.c - window functions (used by all GUI)
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>

#include "../core/weechat.h"
#include "../core/core-config.h"
#include "../core/core-hdata.h"
#include "../core/core-hook.h"
#include "../core/core-infolist.h"
#include "../core/core-log.h"
#include "../core/core-string.h"
#include "../core/core-utf8.h"
#include "../plugins/plugin.h"
#include "gui-window.h"
#include "gui-bar.h"
#include "gui-bar-window.h"
#include "gui-buffer.h"
#include "gui-chat.h"
#include "gui-color.h"
#include "gui-filter.h"
#include "gui-input.h"
#include "gui-history.h"
#include "gui-hotlist.h"
#include "gui-layout.h"
#include "gui-line.h"


int gui_init_ok = 0;                            /* = 1 if GUI is initialized*/
int gui_window_refresh_needed = 0;              /* = 1 if refresh needed    */
                                                /* = 2 for full refresh     */
struct t_gui_window *gui_windows = NULL;        /* first window             */
struct t_gui_window *last_gui_window = NULL;    /* last window              */
struct t_gui_window *gui_current_window = NULL; /* current window           */

struct t_gui_window_tree *gui_windows_tree = NULL; /* windows tree          */

int gui_window_cursor_x = 0;           /* cursor pos on screen              */
int gui_window_cursor_y = 0;           /* cursor pos on screen              */

int gui_window_bare_display = 0;       /* 1 for bare disp. (disable ncurses)*/
struct t_hook *gui_window_bare_display_timer = NULL;
                                       /* timer for bare display            */


/*
 * Searches for a window by number.
 *
 * Returns pointer to window found, NULL if error.
 */

struct t_gui_window *
gui_window_search_by_number (int number)
{
    struct t_gui_window *ptr_win;

    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->number == number)
            return ptr_win;
    }

    /* window not found */
    return NULL;
}

/*
 * Gets pointer of window displayed at (x,y).
 *
 * Return pointer to window found, NULL if not found.
 */

struct t_gui_window *
gui_window_search_by_xy (int x, int y)
{
    struct t_gui_window *ptr_window;

    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if ((x >= ptr_window->win_x) && (y >= ptr_window->win_y)
            && (x <= ptr_window->win_x + ptr_window->win_width - 1)
            && (y <= ptr_window->win_y + ptr_window->win_height - 1))
        {
            return ptr_window;
        }
    }

    /* no window at this location */
    return NULL;
}

/*
 * Returns following info:
 *   - chat (0/1)
 *   - line
 *   - x in line
 *   - word at (x,y)
 *   - beginning of line until (x,y)
 *   - (x,y) until end of line.
 */

void
gui_window_get_context_at_xy (struct t_gui_window *window,
                              int x, int y,
                              int *chat,
                              struct t_gui_line **line,
                              int *line_x,
                              char **word,
                              char **focused_line,
                              char **focused_line_beginning,
                              char **focused_line_end,
                              char **beginning,
                              char **end)
{
    int win_x, win_y, coords_x_message;
    char *data_next_line, *str_temp;
    const char *ptr_data, *line_start, *line_end, *word_start, *word_end;
    const char *last_newline, *last_whitespace;

    *chat = 0;
    *line = NULL;
    *line_x = -1;
    *word = NULL;
    *focused_line = NULL;
    *focused_line_beginning = NULL;
    *focused_line_end = NULL;
    *beginning = NULL;
    *end = NULL;

    /* not in a window? */
    if (!window)
        return;

    /* in window, but not in chat area? */
    win_x = x - window->win_chat_x;
    win_y = y - window->win_chat_y;
    if ((win_x < 0)
        || (win_y < 0)
        || (win_x > window->win_chat_width - 1)
        || (win_y > window->win_chat_height - 1))
    {
        return;
    }

    /* add horizontal scroll (buffers with free content) */
    if (window->scroll->start_col > 0)
        win_x += window->scroll->start_col;

    *line_x = win_x;

    /* we are in chat area */
    *chat = 1;

    /* get line */
    *line = window->coords[win_y].line;
    if (!*line)
        return;

    /* no data for line? */
    if (!window->coords[win_y].data)
        return;

    coords_x_message = gui_line_get_align (
        (*line)->data->buffer,
        *line,
        1,  /* with suffix */
        ((win_y > 0) && (window->coords[win_y - 1].line != *line)));

    if (win_x < coords_x_message)
    {
        /* X is before message (time/buffer/prefix) */
        if ((win_x >= window->coords[win_y].time_x1)
            && (win_x <= window->coords[win_y].time_x2))
        {
            *word = gui_color_decode ((*line)->data->str_time, NULL);
        }
        else if ((win_x >= window->coords[win_y].buffer_x1)
                 && (win_x <= window->coords[win_y].buffer_x2))
        {
            *word = gui_color_decode (gui_buffer_get_short_name ((*line)->data->buffer), NULL);
        }
        else if ((win_x >= window->coords[win_y].prefix_x1)
                 && (win_x <= window->coords[win_y].prefix_x2))
        {
            *word = gui_color_decode ((*line)->data->prefix, NULL);
        }
    }
    else
    {
        /* X is in message (or after) */
        data_next_line = ((win_y < window->win_chat_height - 1)
                          && (window->coords[win_y + 1].line == *line)) ?
            window->coords[win_y + 1].data : NULL;
        ptr_data = gui_chat_string_add_offset_screen (window->coords[win_y].data,
                                                      win_x - coords_x_message);
        if (ptr_data && ptr_data[0]
            && (!data_next_line || (ptr_data < data_next_line)))
        {
            str_temp = string_strndup ((*line)->data->message,
                                       ptr_data - (*line)->data->message);
            if (str_temp)
            {
                *beginning = gui_color_decode (str_temp, NULL);
                free (str_temp);
            }
            *end = gui_color_decode (ptr_data, NULL);
            if (ptr_data[0] != '\n')
            {
                last_newline = NULL;
                last_whitespace = NULL;
                word_start = (*line)->data->message;
                while (word_start && (word_start < ptr_data))
                {
                    word_start = (char *)gui_chat_string_next_char (NULL, NULL,
                                                                    (unsigned char *)word_start,
                                                                    0, 0, 0);
                    if (word_start)
                    {
                        if (word_start[0] == '\n')
                            last_newline = word_start;
                        if (word_start[0] == ' ' || word_start[0] == '\n')
                            last_whitespace = word_start;
                        word_start = utf8_next_char (word_start);
                    }
                }
                line_start = (last_newline) ? last_newline + 1 : (*line)->data->message;
                word_start = (last_whitespace) ? last_whitespace + 1 : (*line)->data->message;
                line_end = ptr_data;
                word_end = NULL;
                while (line_end && line_end[0])
                {
                    line_end = (char *)gui_chat_string_next_char (NULL, NULL,
                                                                  (unsigned char *)line_end,
                                                                  0, 0, 0);
                    if (line_end)
                    {
                        if (!word_end && line_end[0] == ' ')
                            word_end = line_end;
                        if (line_end[0] == '\n')
                            break;
                        line_end = utf8_next_char (line_end);
                    }
                }
                if (!word_end && line_end)
                    word_end = line_end;
                if (ptr_data[0] != ' ' && word_start && word_end)
                {
                    str_temp = string_strndup (word_start, word_end - word_start);
                    if (str_temp)
                    {
                        *word = gui_color_decode (str_temp, NULL);
                        free (str_temp);
                    }
                }
                if (line_start && line_end)
                {
                    str_temp = string_strndup (line_start, line_end - line_start);
                    if (str_temp)
                    {
                        *focused_line = gui_color_decode (str_temp, NULL);
                        free (str_temp);
                    }
                }
                if (line_start)
                {
                    str_temp = string_strndup (line_start, ptr_data - line_start);
                    if (str_temp)
                    {
                        *focused_line_beginning = gui_color_decode (str_temp, NULL);
                        free (str_temp);
                    }
                }
                if (line_end)
                {
                    str_temp = string_strndup (ptr_data, line_end - ptr_data);
                    if (str_temp)
                    {
                        *focused_line_end = gui_color_decode (str_temp, NULL);
                        free (str_temp);
                    }
                }
            }
        }
    }
}

/*
 * Sets flag "gui_window_refresh_needed".
 */

void
gui_window_ask_refresh (int refresh)
{
    if (refresh > gui_window_refresh_needed)
        gui_window_refresh_needed = refresh;
}

/*
 * Creates first entry in windows tree.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_window_tree_init (struct t_gui_window *window)
{
    gui_windows_tree = malloc (sizeof (*gui_windows_tree));
    if (!gui_windows_tree)
        return 0;
    gui_windows_tree->parent_node = NULL;
    gui_windows_tree->split_pct = 0;
    gui_windows_tree->split_horizontal = 0;
    gui_windows_tree->child1 = NULL;
    gui_windows_tree->child2 = NULL;
    gui_windows_tree->window = window;
    return 1;
}

/*
 * Converts a node to a leaf (free any leafs).
 *
 * Called when 2 windows are merging into one.
 */

void
gui_window_tree_node_to_leaf (struct t_gui_window_tree *node,
                              struct t_gui_window *window)
{
    node->split_pct = 0;
    node->split_horizontal = 0;
    if (node->child1)
    {
        free (node->child1);
        node->child1 = NULL;
    }
    if (node->child2)
    {
        free (node->child2);
        node->child2 = NULL;
    }
    node->window = window;
    window->ptr_tree = node;
}

/*
 * Deletes entire windows tree.
 */

void
gui_window_tree_free (struct t_gui_window_tree **tree)
{
    if (!tree || !*tree)
        return;

    if ((*tree)->child1)
        gui_window_tree_free (&((*tree)->child1));
    if ((*tree)->child2)
        gui_window_tree_free (&((*tree)->child2));

    free (*tree);
    *tree = NULL;
}

/*
 * Searches a parent window which is split on the given direction
 * ('h' or 'v').
 */

struct t_gui_window_tree *
gui_window_tree_get_split (struct t_gui_window_tree *tree,
                           char direction)
{
    if (!tree->parent_node)
        return tree;

    if (((tree->parent_node->split_horizontal) && (direction == 'h'))
        || (!(tree->parent_node->split_horizontal) && (direction == 'v')))
    {
        return tree;
    }

    return gui_window_tree_get_split (tree->parent_node, direction);
}

/*
 * Searches for a scroll with buffer pointer.
 *
 * Returns pointer to window scroll, NULL if not found.
 */

struct t_gui_window_scroll *
gui_window_scroll_search (struct t_gui_window *window,
                          struct t_gui_buffer *buffer)
{
    struct t_gui_window_scroll *ptr_scroll;

    if (!window || !buffer)
        return NULL;

    for (ptr_scroll = window->scroll; ptr_scroll;
         ptr_scroll = ptr_scroll->next_scroll)
    {
        if (ptr_scroll->buffer == buffer)
            return ptr_scroll;
    }

    /* scroll not found for buffer */
    return NULL;
}

/*
 * Initializes a window scroll structure.
 */

void
gui_window_scroll_init (struct t_gui_window_scroll *window_scroll,
                        struct t_gui_buffer *buffer)
{
    window_scroll->buffer = buffer;
    window_scroll->first_line_displayed = 0;
    window_scroll->start_line = NULL;
    window_scroll->start_line_pos = 0;
    window_scroll->scrolling = 0;
    window_scroll->start_col = 0;
    window_scroll->lines_after = 0;
    window_scroll->text_search_start_line = NULL;
    window_scroll->prev_scroll = NULL;
    window_scroll->next_scroll = NULL;
}

/*
 * Frees a scroll structure in a window.
 */

void
gui_window_scroll_free (struct t_gui_window *window,
                        struct t_gui_window_scroll *scroll)
{
    if (!window || !scroll)
        return;

    if (scroll->prev_scroll)
        (scroll->prev_scroll)->next_scroll = scroll->next_scroll;
    if (scroll->next_scroll)
        (scroll->next_scroll)->prev_scroll = scroll->prev_scroll;
    if (window->scroll == scroll)
        window->scroll = scroll->next_scroll;

    free (scroll);
}

/*
 * Frees all scroll structures in a window.
 */

void
gui_window_scroll_free_all (struct t_gui_window *window)
{
    while (window->scroll)
    {
        gui_window_scroll_free (window, window->scroll);
    }
}

/*
 * Removes all scroll structures which are empty (not scrolled).
 *
 * Note: the first scroll in list (current buffer) is NOT removed by this
 * function.
 */

void
gui_window_scroll_remove_not_scrolled (struct t_gui_window *window)
{
    struct t_gui_window_scroll *ptr_scroll, *next_scroll;

    if (window)
    {
        ptr_scroll = window->scroll->next_scroll;
        while (ptr_scroll)
        {
            next_scroll = ptr_scroll->next_scroll;

            if ((ptr_scroll->first_line_displayed == 0)
                && (ptr_scroll->start_line == NULL)
                && (ptr_scroll->start_line_pos == 0)
                && (ptr_scroll->scrolling == 0)
                && (ptr_scroll->start_col == 0)
                && (ptr_scroll->lines_after == 0)
                && (ptr_scroll->text_search_start_line == NULL))
            {
                gui_window_scroll_free (window, ptr_scroll);
            }

            ptr_scroll = next_scroll;
        }
    }
}

/*
 * Switches scroll to a buffer.
 */

void
gui_window_scroll_switch (struct t_gui_window *window,
                          struct t_gui_buffer *buffer)
{
    struct t_gui_window_scroll *ptr_scroll, *new_scroll;

    if (!window || !buffer)
        return;

    ptr_scroll = gui_window_scroll_search (window, buffer);

    /* scroll is already selected (first in list)? */
    if (ptr_scroll && (ptr_scroll == window->scroll))
        return;

    if (ptr_scroll)
    {
        /* scroll found, move it in first position */
        if (ptr_scroll->prev_scroll)
            (ptr_scroll->prev_scroll)->next_scroll = ptr_scroll->next_scroll;
        if (ptr_scroll->next_scroll)
            (ptr_scroll->next_scroll)->prev_scroll = ptr_scroll->prev_scroll;
        (window->scroll)->prev_scroll = ptr_scroll;
        ptr_scroll->prev_scroll = NULL;
        ptr_scroll->next_scroll = window->scroll;
        window->scroll = ptr_scroll;
    }
    else
    {
        /* scroll not found, create one and add it at first position */
        new_scroll = malloc (sizeof (*new_scroll));
        if (new_scroll)
        {
            gui_window_scroll_init (new_scroll, buffer);
            new_scroll->next_scroll = window->scroll;
            (window->scroll)->prev_scroll = new_scroll;
            window->scroll = new_scroll;
        }
    }

    gui_window_scroll_remove_not_scrolled (window);
}

/*
 * Removes buffer from scroll list in a window.
 */

void
gui_window_scroll_remove_buffer (struct t_gui_window *window,
                                 struct t_gui_buffer *buffer)
{
    struct t_gui_window_scroll *ptr_scroll;

    if (!window || !buffer)
        return;

    ptr_scroll = gui_window_scroll_search (window, buffer);
    if (ptr_scroll)
        gui_window_scroll_free (window, ptr_scroll);
}

/*
 * Creates a new window.
 *
 * Returns pointer to new window, NULL if error.
 */

struct t_gui_window *
gui_window_new (struct t_gui_window *parent_window, struct t_gui_buffer *buffer,
                int x, int y, int width, int height,
                int width_pct, int height_pct)
{
    struct t_gui_window *new_window;
    struct t_gui_window_tree *ptr_tree, *child1, *child2, *ptr_leaf;
    struct t_gui_bar *ptr_bar;

    if (parent_window)
    {
        child1 = malloc (sizeof (*child1));
        if (!child1)
            return NULL;
        child2 = malloc (sizeof (*child2));
        if (!child2)
        {
            free (child1);
            return NULL;
        }
        ptr_tree = parent_window->ptr_tree;

        if (width_pct == 100)
        {
            ptr_tree->split_horizontal = 1;
            ptr_tree->split_pct = height_pct;
        }
        else
        {
            ptr_tree->split_horizontal = 0;
            ptr_tree->split_pct = width_pct;
        }

        /*
         * parent window leaf becomes node and we add 2 leafs below
         * (#1 is parent win, #2 is new win)
         */

        parent_window->ptr_tree = child1;
        child1->parent_node = ptr_tree;
        child1->child1 = NULL;
        child1->child2 = NULL;
        child1->window = ptr_tree->window;

        child2->parent_node = ptr_tree;
        child2->child1 = NULL;
        child2->child2 = NULL;
        child2->window = NULL;    /* will be assigned by new window below */

        ptr_tree->child1 = child1;
        ptr_tree->child2 = child2;
        ptr_tree->window = NULL;  /* leaf becomes node */

        ptr_leaf = child2;
    }
    else
    {
        if (!gui_window_tree_init (NULL))
            return NULL;
        ptr_leaf = gui_windows_tree;
    }

    new_window = (malloc (sizeof (*new_window)));
    if (!new_window)
        return NULL;

    /* create scroll structure */
    new_window->scroll = malloc (sizeof (*new_window->scroll));
    if (!new_window->scroll)
    {
        free (new_window);
        return NULL;
    }

    /* create window objects */
    if (!gui_window_objects_init (new_window))
    {
        free (new_window->scroll);
        free (new_window);
        return NULL;
    }

    /* number */
    new_window->number = (last_gui_window) ? last_gui_window->number + 1 : 1;

    /* position & size */
    new_window->win_x = x;
    new_window->win_y = y;
    new_window->win_width = width;
    new_window->win_height = height;
    new_window->win_width_pct = width_pct;
    new_window->win_height_pct = height_pct;

    /* chat window */
    new_window->win_chat_x = 0;
    new_window->win_chat_y = 0;
    new_window->win_chat_width = 0;
    new_window->win_chat_height = 0;
    new_window->win_chat_cursor_x = 0;
    new_window->win_chat_cursor_y = 0;

    /* bar windows */
    new_window->bar_windows = NULL;
    new_window->last_bar_window = NULL;

    /* refresh */
    new_window->refresh_needed = 0;

    /* buffer and layout infos */
    new_window->buffer = buffer;
    new_window->layout_plugin_name = NULL;
    new_window->layout_buffer_name = NULL;

    /* scroll */
    gui_window_scroll_init (new_window->scroll, buffer);

    /* coordinates */
    new_window->coords_size = 0;
    new_window->coords = NULL;

    /* tree */
    new_window->ptr_tree = ptr_leaf;
    ptr_leaf->window = new_window;

    /* add window to windows queue */
    new_window->prev_window = last_gui_window;
    if (last_gui_window)
        last_gui_window->next_window = new_window;
    else
        gui_windows = new_window;
    last_gui_window = new_window;
    new_window->next_window = NULL;

    /* create bar windows */
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (CONFIG_ENUM(ptr_bar->options[GUI_BAR_OPTION_TYPE]) != GUI_BAR_TYPE_ROOT)
            gui_bar_window_new (ptr_bar, new_window);
    }

    /* send signal */
    (void) hook_signal_send ("window_opened",
                             WEECHAT_HOOK_SIGNAL_POINTER, new_window);

    return new_window;
}

/*
 * Checks if a window pointer is valid.
 *
 * Returns:
 *   1: window exists
 *   0: window does not exist
 */

int
gui_window_valid (struct t_gui_window *window)
{
    struct t_gui_window *ptr_window;

    if (!window)
        return 0;

    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if (ptr_window == window)
            return 1;
    }

    /* window not found */
    return 0;
}

/*
 * Searches for window displaying a buffer.
 *
 * Returns NULL if no window is displaying given buffer.
 * If many windows are displaying this buffer, the first window in list is
 * returned (or current window if it is displaying this buffer).
 */

struct t_gui_window *
gui_window_search_with_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_window *ptr_window;

    if (!buffer)
        return NULL;

    if (gui_current_window->buffer == buffer)
        return gui_current_window;

    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if (ptr_window->buffer == buffer)
            return ptr_window;
    }

    /* no window displaying buffer */
    return NULL;
}

/*
 * Gets a window property as integer.
 */

int
gui_window_get_integer (struct t_gui_window *window, const char *property)
{
    if (!window || !property)
        return 0;

    if (strcmp (property, "number") == 0)
        return window->number;
    if (strcmp (property, "win_x") == 0)
        return window->win_x;
    if (strcmp (property, "win_y") == 0)
        return window->win_y;
    if (strcmp (property, "win_width") == 0)
        return window->win_width;
    if (strcmp (property, "win_height") == 0)
        return window->win_height;
    if (strcmp (property, "win_width_pct") == 0)
        return window->win_width_pct;
    if (strcmp (property, "win_height_pct") == 0)
        return window->win_height_pct;
    if (strcmp (property, "win_chat_x") == 0)
        return window->win_chat_x;
    if (strcmp (property, "win_chat_y") == 0)
        return window->win_chat_y;
    if (strcmp (property, "win_chat_width") == 0)
        return window->win_chat_width;
    if (strcmp (property, "win_chat_height") == 0)
        return window->win_chat_height;
    if (strcmp (property, "first_line_displayed") == 0)
        return window->scroll->first_line_displayed;
    if (strcmp (property, "scrolling") == 0)
        return window->scroll->scrolling;
    if (strcmp (property, "lines_after") == 0)
        return window->scroll->lines_after;

    return 0;
}

/*
 * Gets a window property as string.
 */

const char *
gui_window_get_string (struct t_gui_window *window, const char *property)
{
    if (!window || !property)
        return NULL;

    return NULL;
}

/*
 * Gets a window property as pointer.
 */

void *
gui_window_get_pointer (struct t_gui_window *window, const char *property)
{
    if (!property)
        return NULL;

    if (strcmp (property, "current") == 0)
        return gui_current_window;

    if (window)
    {
        if (strcmp (property, "buffer") == 0)
            return window->buffer;
    }

    return NULL;
}

/*
 * Sets layout plugin name for window.
 */

void
gui_window_set_layout_plugin_name (struct t_gui_window *window,
                                   const char *plugin_name)
{
    if (!window)
        return;

    if (window->layout_plugin_name)
    {
        free (window->layout_plugin_name);
        window->layout_plugin_name = NULL;
    }

    if (plugin_name)
        window->layout_plugin_name = strdup (plugin_name);
}

/*
 * Sets layout buffer name for window.
 */

void
gui_window_set_layout_buffer_name (struct t_gui_window *window,
                                   const char *buffer_name)
{
    if (!window)
        return;

    if (window->layout_buffer_name)
    {
        free (window->layout_buffer_name);
        window->layout_buffer_name = NULL;
    }

    if (buffer_name)
        window->layout_buffer_name = strdup (buffer_name);
}

/*
 * Initializes a line in window coordinates.
 */

void
gui_window_coords_init_line (struct t_gui_window *window, int line)
{
    if (!window || !window->coords || (line < 0)
        || (line >= window->coords_size))
    {
        return;
    }

    window->coords[line].line = NULL;
    window->coords[line].data = NULL;
    window->coords[line].time_x1 = -1;
    window->coords[line].time_x2 = -1;
    window->coords[line].buffer_x1 = -1;
    window->coords[line].buffer_x2 = -1;
    window->coords[line].prefix_x1 = -1;
    window->coords[line].prefix_x2 = -1;
}

/*
 * Removes a line from coordinates: each time the line is found in the array
 * "coords", it is reinitialized.
 */

void
gui_window_coords_remove_line (struct t_gui_window *window,
                               struct t_gui_line *line)
{
    int i;

    if (!window || !window->coords)
        return;

    for (i = 0; i < window->coords_size; i++)
    {
        if (window->coords[i].line == line)
            gui_window_coords_init_line (window, i);
    }
}

/*
 * Removes a line from coordinates: each time a line with data == line_data is
 * found in the array "coords", it is reinitialized.
 */

void
gui_window_coords_remove_line_data (struct t_gui_window *window,
                                    struct t_gui_line_data *line_data)
{
    int i;

    if (!window || !window->coords)
        return;

    for (i = 0; i < window->coords_size; i++)
    {
        if (window->coords[i].line
            && (window->coords[i].line->data == line_data))
        {
            gui_window_coords_init_line (window, i);
        }
    }
}

/*
 * Allocates and initializes coordinates for window.
 */

void
gui_window_coords_alloc (struct t_gui_window *window)
{
    int i;

    if (!window)
        return;

    if (window->coords && (window->coords_size != window->win_chat_height))
    {
        free (window->coords);
        window->coords = NULL;
    }
    window->coords_size = window->win_chat_height;
    if (!window->coords)
        window->coords = malloc (window->coords_size * sizeof (window->coords[0]));
    if (window->coords)
    {
        for (i = 0; i < window->win_chat_height; i++)
        {
            gui_window_coords_init_line (window, i);
        }
    }
}

/*
 * Deletes a window.
 */

void
gui_window_free (struct t_gui_window *window)
{
    struct t_gui_window *ptr_win, *old_current_window;
    int i;

    if (!window)
        return;

    old_current_window = gui_current_window;

    (void) hook_signal_send ("window_closing",
                             WEECHAT_HOOK_SIGNAL_POINTER, window);

    if (window->buffer)
        gui_buffer_add_value_num_displayed (window->buffer, -1);

    /* free data */
    if (window->gui_objects)
    {
        gui_window_objects_free (window, 1);
        free (window->gui_objects);
    }

    /* remove bar windows */
    while (window->bar_windows)
    {
        gui_bar_window_free (window->bar_windows, window);
    }

    /* free other data */
    free (window->layout_plugin_name);
    free (window->layout_buffer_name);

    /* remove scroll list */
    gui_window_scroll_free_all (window);

    /* free coords */
    free (window->coords);

    /* remove window from windows list */
    if (window->prev_window)
        (window->prev_window)->next_window = window->next_window;
    if (window->next_window)
        (window->next_window)->prev_window = window->prev_window;
    if (gui_windows == window)
        gui_windows = window->next_window;
    if (last_gui_window == window)
        last_gui_window = window->prev_window;

    if (gui_current_window == window)
        gui_current_window = gui_windows;

    i = 1;
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        ptr_win->number = i;
        i++;
    }

    (void) hook_signal_send ("window_closed",
                             WEECHAT_HOOK_SIGNAL_POINTER, window);

    free (window);

    if (gui_current_window != old_current_window)
    {
        (void) hook_signal_send ("window_switch",
                                 WEECHAT_HOOK_SIGNAL_POINTER,
                                 gui_current_window);
    }
}

/*
 * Switches to previous window.
 */

void
gui_window_switch_previous (struct t_gui_window *window)
{
    if (!gui_init_ok || !window)
        return;

    gui_window_switch ((window->prev_window) ?
                       window->prev_window : last_gui_window);
}

/*
 * Switches to next window.
 */

void
gui_window_switch_next (struct t_gui_window *window)
{
    if (!gui_init_ok || !window)
        return;

    gui_window_switch ((window->next_window) ?
                       window->next_window : gui_windows);
}

/*
 * Switches to window by number.
 */

void
gui_window_switch_by_number (int number)
{
    struct t_gui_window *ptr_win;

    if (!gui_init_ok)
        return;

    ptr_win = gui_window_search_by_number (number);
    if (ptr_win)
        gui_window_switch (ptr_win);
}

/*
 * Switches to next window displaying a buffer.
 */

void
gui_window_switch_by_buffer (struct t_gui_window *window, int buffer_number)
{
    struct t_gui_window *ptr_win;

    if (!gui_init_ok || !window)
        return;

    ptr_win = (window->next_window) ? window->next_window : gui_windows;
    while (ptr_win != window)
    {
        if (ptr_win->buffer->number == buffer_number)
        {
            gui_window_switch (ptr_win);
            return;
        }
        ptr_win = (ptr_win->next_window) ? ptr_win->next_window : gui_windows;
    }
}

/*
 * Scrolls window by a number of messages or time.
 */

void
gui_window_scroll (struct t_gui_window *window, char *scroll)
{
    int direction, stop, count_msg, scroll_from_end_free_buffer;
    char time_letter, saved_char;
    time_t old_date, diff_date;
    char *pos, *error;
    long number;
    struct t_gui_line *ptr_line;
    struct tm *date_tmp, line_date, old_line_date;

    if (!window || !window->buffer->lines->first_line)
        return;

    direction = 1;
    number = 0;
    time_letter = ' ';
    scroll_from_end_free_buffer = 0;

    /* search direction */
    if (scroll[0] == '-')
    {
        direction = -1;
        scroll++;
        if (scroll[0] == '-')
        {
            scroll_from_end_free_buffer = 1;
            scroll++;
        }
    }
    else if (scroll[0] == '+')
    {
        direction = +1;
        scroll++;
    }

    /* search number and letter */
    pos = scroll;
    while (pos && pos[0] && isdigit ((unsigned char)pos[0]))
    {
        pos++;
    }
    if (pos)
    {
        if (pos == scroll)
        {
            if (pos[0])
                time_letter = scroll[0];
        }
        else
        {
            if (pos[0])
                time_letter = pos[0];
            saved_char = pos[0];
            pos[0] = '\0';
            error = NULL;
            number = strtol (scroll, &error, 10);
            if (!error || error[0])
                number = 0;
            pos[0] = saved_char;
        }
    }

    /* at least number or letter has to he given */
    if ((number == 0) && (time_letter == ' '))
        return;

    /* do the scroll! */
    stop = 0;
    count_msg = 0;
    if (direction < 0)
    {
        /*
         * it's not possible to scroll before first line of buffer on a buffer
         * with free content
         */
        if (!scroll_from_end_free_buffer && !window->scroll->start_line
           && (window->buffer->type == GUI_BUFFER_TYPE_FREE))
            return;
        ptr_line = (window->scroll->start_line) ?
            window->scroll->start_line : window->buffer->lines->last_line;
        while (ptr_line
               && (!gui_line_is_displayed (ptr_line)
                   || ((window->buffer->type == GUI_BUFFER_TYPE_FORMATTED)
                       && (ptr_line->data->date == 0))))
        {
            ptr_line = ptr_line->prev_line;
        }
    }
    else
    {
        ptr_line = (window->scroll->start_line) ?
            window->scroll->start_line : window->buffer->lines->first_line;
        while (ptr_line
               && (!gui_line_is_displayed (ptr_line)
                   || ((window->buffer->type == GUI_BUFFER_TYPE_FORMATTED)
                       && (ptr_line->data->date == 0))))
        {
            ptr_line = ptr_line->next_line;
        }
    }

    if (ptr_line)
    {
        old_date = ptr_line->data->date;
        date_tmp = localtime (&old_date);
        if (!date_tmp)
            return;
        memcpy (&old_line_date, date_tmp, sizeof (struct tm));
    }

    while (ptr_line)
    {
        ptr_line = (direction < 0) ?
            gui_line_get_prev_displayed (ptr_line) : gui_line_get_next_displayed (ptr_line);

        if (ptr_line
            && ((window->buffer->type != GUI_BUFFER_TYPE_FORMATTED)
                || (ptr_line->data->date != 0)))
        {
            if (time_letter == ' ')
            {
                count_msg++;
                if (count_msg >= number)
                    stop = 1;
            }
            else
            {
                date_tmp = localtime (&(ptr_line->data->date));
                if (!date_tmp)
                    return;
                memcpy (&line_date, date_tmp, sizeof (struct tm));
                if (old_date > ptr_line->data->date)
                    diff_date = old_date - ptr_line->data->date;
                else
                    diff_date = ptr_line->data->date - old_date;
                switch (time_letter)
                {
                    case 's': /* seconds */
                        if (number == 0)
                        {
                            /* stop if line has different second */
                            if ((line_date.tm_sec != old_line_date.tm_sec)
                                || (line_date.tm_min != old_line_date.tm_min)
                                || (line_date.tm_hour != old_line_date.tm_hour)
                                || (line_date.tm_mday != old_line_date.tm_mday)
                                || (line_date.tm_mon != old_line_date.tm_mon)
                                || (line_date.tm_year != old_line_date.tm_year))
                                if (line_date.tm_sec != old_line_date.tm_sec)
                                    stop = 1;
                        }
                        else if (diff_date >= number)
                            stop = 1;
                        break;
                    case 'm': /* minutes */
                        if (number == 0)
                        {
                            /* stop if line has different minute */
                            if ((line_date.tm_min != old_line_date.tm_min)
                                || (line_date.tm_hour != old_line_date.tm_hour)
                                || (line_date.tm_mday != old_line_date.tm_mday)
                                || (line_date.tm_mon != old_line_date.tm_mon)
                                || (line_date.tm_year != old_line_date.tm_year))
                                stop = 1;
                        }
                        else if (diff_date >= number * 60)
                            stop = 1;
                        break;
                    case 'h': /* hours */
                        if (number == 0)
                        {
                            /* stop if line has different hour */
                            if ((line_date.tm_hour != old_line_date.tm_hour)
                                || (line_date.tm_mday != old_line_date.tm_mday)
                                || (line_date.tm_mon != old_line_date.tm_mon)
                                || (line_date.tm_year != old_line_date.tm_year))
                                stop = 1;
                        }
                        else if (diff_date >= number * 60 * 60)
                            stop = 1;
                        break;
                    case 'd': /* days */
                        if (number == 0)
                        {
                            /* stop if line has different day */
                            if ((line_date.tm_mday != old_line_date.tm_mday)
                                || (line_date.tm_mon != old_line_date.tm_mon)
                                || (line_date.tm_year != old_line_date.tm_year))
                                stop = 1;
                        }
                        else if (diff_date >= number * 60 * 60 * 24)
                            stop = 1;
                        break;
                    case 'M': /* months */
                        if (number == 0)
                        {
                            /* stop if line has different month */
                            if ((line_date.tm_mon != old_line_date.tm_mon)
                                || (line_date.tm_year != old_line_date.tm_year))
                                stop = 1;
                        }
                        /*
                         * we consider month is 30 days, who will notice
                         * I'm too lazy to code exact date diff ? ;)
                         */
                        else if (diff_date >= number * 60 * 60 * 24 * 30)
                            stop = 1;
                        break;
                    case 'y': /* years */
                        if (number == 0)
                        {
                            /* stop if line has different year */
                            if (line_date.tm_year != old_line_date.tm_year)
                                stop = 1;
                        }
                        /*
                         * we consider year is 365 days, who will notice
                         * I'm too lazy to code exact date diff ? ;)
                         */
                        else if (diff_date >= number * 60 * 60 * 24 * 365)
                            stop = 1;
                        break;
                }
            }
            if (stop)
            {
                window->scroll->start_line = ptr_line;
                window->scroll->start_line_pos = 0;
                window->scroll->first_line_displayed =
                    (window->scroll->start_line == gui_line_get_first_displayed (window->buffer));
                gui_buffer_ask_chat_refresh (window->buffer, 2);
                return;
            }
        }
    }

    if (direction < 0)
    {
        gui_window_scroll_top (window);
    }
    else
    {
        if (window->buffer->type == GUI_BUFFER_TYPE_FORMATTED)
            gui_window_scroll_bottom (window);
    }
}

/*
 * Horizontally scrolls window.
 */

void
gui_window_scroll_horiz (struct t_gui_window *window, char *scroll)
{
    int direction, percentage, start_col;
    char saved_char, *pos, *error;
    long number;

    if (!window || !window->buffer->lines->first_line)
        return;

    direction = 1;
    number = 0;
    percentage = 0;

    /* search direction */
    if (scroll[0] == '-')
    {
        direction = -1;
        scroll++;
    }
    else if (scroll[0] == '+')
    {
        direction = +1;
        scroll++;
    }

    /* search number and percentage */
    pos = scroll;
    while (pos && pos[0] && isdigit ((unsigned char)pos[0]))
    {
        pos++;
    }
    if (pos && (pos > scroll))
    {
        percentage = (pos[0] == '%') ? 1 : 0;
        saved_char = pos[0];
        pos[0] = '\0';
        error = NULL;
        number = strtol (scroll, &error, 10);
        if (!error || error[0])
            number = 0;
        pos[0] = saved_char;
    }

    /* for percentage, compute number of columns */
    if (percentage)
    {
        number = (window->win_chat_width * number) / 100;
    }

    /* number must be different from 0 */
    if (number == 0)
        return;

    /* do the horizontal scroll! */
    start_col = window->scroll->start_col + (number * direction);
    if (start_col < 0)
        start_col = 0;
    if (start_col != window->scroll->start_col)
    {
        window->scroll->start_col = start_col;
        gui_buffer_ask_chat_refresh (window->buffer, 2);
    }
}

/*
 * Scrolls to previous highlight.
 */

void
gui_window_scroll_previous_highlight (struct t_gui_window *window)
{
    struct t_gui_line *ptr_line;

    if (!window)
        return;

    if (window->buffer->type == GUI_BUFFER_TYPE_FORMATTED)
    {
        if (window->buffer->lines->first_line)
        {
            ptr_line = (window->scroll->start_line) ?
                window->scroll->start_line->prev_line : window->buffer->lines->last_line;
            while (ptr_line)
            {
                if (ptr_line->data->highlight)
                {
                    window->scroll->start_line = ptr_line;
                    window->scroll->start_line_pos = 0;
                    window->scroll->first_line_displayed =
                        (window->scroll->start_line == window->buffer->lines->first_line);
                    gui_buffer_ask_chat_refresh (window->buffer, 2);
                    return;
                }
                ptr_line = ptr_line->prev_line;
            }
            /* no previous highlight, scroll to bottom */
            gui_window_scroll_bottom (window);
        }
    }
}

/*
 * Scrolls to next highlight.
 */

void
gui_window_scroll_next_highlight (struct t_gui_window *window)
{
    struct t_gui_line *ptr_line;

    if (!window)
        return;

    if (window->buffer->type == GUI_BUFFER_TYPE_FORMATTED)
    {
        if (window->buffer->lines->first_line)
        {
            ptr_line = (window->scroll->start_line) ?
                window->scroll->start_line->next_line : window->buffer->lines->first_line->next_line;
            while (ptr_line)
            {
                if (ptr_line->data->highlight)
                {
                    window->scroll->start_line = ptr_line;
                    window->scroll->start_line_pos = 0;
                    window->scroll->first_line_displayed =
                        (window->scroll->start_line == window->buffer->lines->first_line);
                    gui_buffer_ask_chat_refresh (window->buffer, 2);
                    return;
                }
                ptr_line = ptr_line->next_line;
            }
            /* no next highlight, scroll to bottom */
            gui_window_scroll_bottom (window);
        }
    }
}

/*
 * Scrolls to first unread line of buffer.
 */

void
gui_window_scroll_unread (struct t_gui_window *window)
{
    if (!window)
        return;

    if (CONFIG_STRING(config_look_read_marker) &&
        CONFIG_STRING(config_look_read_marker)[0] &&
        (window->buffer->type == GUI_BUFFER_TYPE_FORMATTED) &&
        (window->buffer->lines->first_line_not_read ||
         (window->buffer->lines->last_read_line &&
          window->buffer->lines->last_read_line != window->buffer->lines->last_line)))
    {
        if (window->buffer->lines->first_line_not_read)
            window->scroll->start_line = window->buffer->lines->first_line;
        else
            window->scroll->start_line = window->buffer->lines->last_read_line->next_line;
        if (window->scroll->start_line)
        {
            if (!gui_line_is_displayed (window->scroll->start_line))
                window->scroll->start_line = gui_line_get_next_displayed (window->scroll->start_line);
        }
        window->scroll->start_line_pos = 0;
        window->scroll->first_line_displayed =
            (window->scroll->start_line == gui_line_get_first_displayed (window->buffer));
        gui_buffer_ask_chat_refresh (window->buffer, 2);
    }
}

/*
 * Searches for text in buffer lines or commands history.
 *
 * Returns:
 *   1: successful search
 *   0: no results found
 */

int
gui_window_search_text (struct t_gui_window *window)
{
    struct t_gui_line *ptr_line;

    if (!window)
        return 0;

    switch (window->buffer->text_search)
    {
        case GUI_BUFFER_SEARCH_DISABLED:
            break;
        case GUI_BUFFER_SEARCH_LINES:
            if (window->buffer->text_search_direction == GUI_BUFFER_SEARCH_DIR_BACKWARD)
            {
                if (window->buffer->lines->first_line
                    && window->buffer->input_buffer && window->buffer->input_buffer[0])
                {
                    ptr_line = (window->scroll->start_line) ?
                        gui_line_get_prev_displayed (window->scroll->start_line) :
                        gui_line_get_last_displayed (window->buffer);
                    while (ptr_line)
                    {
                        if (gui_line_search_text (window->buffer, ptr_line))
                        {
                            window->scroll->start_line = ptr_line;
                            window->scroll->start_line_pos = 0;
                            window->scroll->first_line_displayed =
                                (window->scroll->start_line == gui_line_get_first_displayed (window->buffer));
                            gui_buffer_ask_chat_refresh (window->buffer, 2);
                            return 1;
                        }
                        ptr_line = gui_line_get_prev_displayed (ptr_line);
                    }
                }
            }
            else if (window->buffer->text_search_direction == GUI_BUFFER_SEARCH_DIR_FORWARD)
            {
                if (window->buffer->lines->first_line
                    && window->buffer->input_buffer && window->buffer->input_buffer[0])
                {
                    ptr_line = (window->scroll->start_line) ?
                        gui_line_get_next_displayed (window->scroll->start_line) :
                        gui_line_get_first_displayed (window->buffer);
                    while (ptr_line)
                    {
                        if (gui_line_search_text (window->buffer, ptr_line))
                        {
                            window->scroll->start_line = ptr_line;
                            window->scroll->start_line_pos = 0;
                            window->scroll->first_line_displayed =
                                (window->scroll->start_line == window->buffer->lines->first_line);
                            gui_buffer_ask_chat_refresh (window->buffer, 2);
                            return 1;
                        }
                        ptr_line = gui_line_get_next_displayed (ptr_line);
                    }
                }
            }
            break;
        case GUI_BUFFER_SEARCH_HISTORY:
            return gui_history_search (
                window->buffer,
                (window->buffer->text_search_history == GUI_BUFFER_SEARCH_HISTORY_LOCAL) ?
                window->buffer->history : gui_history);
            break;
        case GUI_BUFFER_NUM_SEARCH:
            break;
    }
    return 0;
}

/*
 * Starts search in a buffer at a given position
 * (or in whole buffer if text_search_start_line is NULL).
 */

void
gui_window_search_start (struct t_gui_window *window, int search,
                         struct t_gui_line *text_search_start_line)
{
    if (!window)
        return;

    window->buffer->text_search = search;

    switch (window->buffer->text_search)
    {
        case GUI_BUFFER_SEARCH_DISABLED:
            break;
        case GUI_BUFFER_SEARCH_LINES:
            window->buffer->text_search_direction = (window->buffer->type == GUI_BUFFER_TYPE_FORMATTED) ?
                GUI_BUFFER_SEARCH_DIR_BACKWARD : GUI_BUFFER_SEARCH_DIR_FORWARD;
            window->scroll->text_search_start_line = text_search_start_line;
            if ((window->buffer->text_search_where == 0)
                || CONFIG_BOOLEAN(config_look_buffer_search_force_default))
            {
                if (window->buffer->type == GUI_BUFFER_TYPE_FORMATTED)
                {
                    switch (CONFIG_ENUM(config_look_buffer_search_where))
                    {
                        case CONFIG_LOOK_BUFFER_SEARCH_PREFIX:
                            window->buffer->text_search_where = GUI_BUFFER_SEARCH_IN_PREFIX;
                            break;
                        case CONFIG_LOOK_BUFFER_SEARCH_MESSAGE:
                            window->buffer->text_search_where = GUI_BUFFER_SEARCH_IN_MESSAGE;
                            break;
                        case CONFIG_LOOK_BUFFER_SEARCH_PREFIX_MESSAGE:
                            window->buffer->text_search_where = GUI_BUFFER_SEARCH_IN_PREFIX
                                | GUI_BUFFER_SEARCH_IN_MESSAGE;
                            break;
                        default:
                            window->buffer->text_search_where = GUI_BUFFER_SEARCH_IN_MESSAGE;
                            break;
                    }
                }
                else
                {
                    window->buffer->text_search_where = GUI_BUFFER_SEARCH_IN_MESSAGE;
                }
                break;
            case GUI_BUFFER_SEARCH_HISTORY:
                window->buffer->text_search_direction = GUI_BUFFER_SEARCH_DIR_BACKWARD;
                if ((window->buffer->text_search_history == GUI_BUFFER_SEARCH_HISTORY_NONE)
                    || CONFIG_BOOLEAN(config_look_buffer_search_force_default))
                {
                    switch (CONFIG_ENUM(config_look_buffer_search_history))
                    {
                        case CONFIG_LOOK_BUFFER_SEARCH_HISTORY_LOCAL:
                            window->buffer->text_search_history = GUI_BUFFER_SEARCH_HISTORY_LOCAL;
                            break;
                        case CONFIG_LOOK_BUFFER_SEARCH_HISTORY_GLOBAL:
                            window->buffer->text_search_history = GUI_BUFFER_SEARCH_HISTORY_GLOBAL;
                            break;
                        default:
                            window->buffer->text_search_history = GUI_BUFFER_SEARCH_HISTORY_LOCAL;
                            break;
                    }
                }
                break;
            case GUI_BUFFER_NUM_SEARCH:
                break;
        }
    }

    window->buffer->text_search_exact = CONFIG_BOOLEAN(config_look_buffer_search_case_sensitive);
    window->buffer->text_search_regex = CONFIG_BOOLEAN(config_look_buffer_search_regex);
    window->buffer->text_search_found = 0;
    gui_input_search_compile_regex (window->buffer);
    if (window->buffer->text_search_input)
    {
        free (window->buffer->text_search_input);
        window->buffer->text_search_input = NULL;
    }
    if (window->buffer->input_buffer && window->buffer->input_buffer[0])
        window->buffer->text_search_input =
            strdup (window->buffer->input_buffer);
    gui_input_delete_line (window->buffer);
}

/*
 * Restarts search (after input changes or exact flag (un)set).
 */

void
gui_window_search_restart (struct t_gui_window *window)
{
    if (!window)
        return;

    switch (window->buffer->text_search)
    {
        case GUI_BUFFER_SEARCH_DISABLED:
            break;
        case GUI_BUFFER_SEARCH_LINES:
            window->scroll->start_line = window->scroll->text_search_start_line;
            window->scroll->start_line_pos = 0;
            window->buffer->text_search_direction =
                (window->buffer->type == GUI_BUFFER_TYPE_FORMATTED) ?
                GUI_BUFFER_SEARCH_DIR_BACKWARD : GUI_BUFFER_SEARCH_DIR_FORWARD;
            gui_input_search_compile_regex (window->buffer);
            window->buffer->text_search_found = 0;
            if (gui_window_search_text (window))
            {
                window->buffer->text_search_found = 1;
            }
            else
            {
                if (CONFIG_BOOLEAN(config_look_search_text_not_found_alert)
                    && window->buffer->input_buffer && window->buffer->input_buffer[0])
                {
                    fprintf (stderr, "\a");
                    fflush (stderr);
                }
                gui_buffer_ask_chat_refresh (window->buffer, 2);
            }
            break;
        case GUI_BUFFER_SEARCH_HISTORY:
            gui_input_search_compile_regex (window->buffer);
            window->buffer->text_search_found = 0;
            window->buffer->text_search_ptr_history = NULL;
            if (gui_window_search_text (window))
            {
                window->buffer->text_search_found = 1;
            }
            else
            {
                if (CONFIG_BOOLEAN(config_look_search_text_not_found_alert)
                    && window->buffer->input_buffer && window->buffer->input_buffer[0])
                {
                    fprintf (stderr, "\a");
                    fflush (stderr);
                }
            }
            break;
        case GUI_BUFFER_NUM_SEARCH:
            break;
    }
}

/*
 * Stops search in a buffer, at current position if stop_here == 1 or reset
 * scroll to the initial value if stop_here == 0.
 */

void
gui_window_search_stop (struct t_gui_window *window, int stop_here)
{
    const char *ptr_new_input;
    int search;

    if (!window)
        return;

    search = window->buffer->text_search;

    ptr_new_input = (stop_here
                     && (window->buffer->text_search == GUI_BUFFER_SEARCH_HISTORY)
                     && window->buffer->text_search_ptr_history
                     && window->buffer->text_search_ptr_history->text) ?
        window->buffer->text_search_ptr_history->text : window->buffer->text_search_input;

    window->buffer->text_search = GUI_BUFFER_SEARCH_DISABLED;
    window->buffer->text_search_direction = GUI_BUFFER_SEARCH_DIR_BACKWARD;
    if (window->buffer->text_search_regex_compiled)
    {
        regfree (window->buffer->text_search_regex_compiled);
        free (window->buffer->text_search_regex_compiled);
        window->buffer->text_search_regex_compiled = NULL;
    }
    gui_input_delete_line (window->buffer);
    if (ptr_new_input)
    {
        gui_input_insert_string (window->buffer, ptr_new_input);
        gui_input_text_changed_modifier_and_signal (window->buffer,
                                                    0, /* save undo */
                                                    1); /* stop completion */
    }
    if (window->buffer->text_search_input)
    {
        free (window->buffer->text_search_input);
        window->buffer->text_search_input = NULL;
    }
    window->buffer->text_search_ptr_history = NULL;

    if (search == GUI_BUFFER_SEARCH_LINES)
    {
        if (!stop_here)
        {
            window->scroll->start_line = window->scroll->text_search_start_line;
            window->scroll->start_line_pos = 0;
            gui_hotlist_remove_buffer (window->buffer, 0);
        }
        window->scroll->text_search_start_line = NULL;
        gui_buffer_ask_chat_refresh (window->buffer, 2);
    }
}

/*
 * Zooms window (maximize it or restore layout before previous zoom).
 */

void
gui_window_zoom (struct t_gui_window *window)
{
    struct t_gui_layout *ptr_layout;

    if (!gui_init_ok || !window)
        return;

    ptr_layout = gui_layout_search (GUI_LAYOUT_ZOOM);
    if (ptr_layout)
    {
        /* restore layout as it was before zooming a window */
        (void) hook_signal_send ("window_unzoom",
                                 WEECHAT_HOOK_SIGNAL_POINTER,
                                 gui_current_window);
        gui_layout_window_apply (ptr_layout,
                                 ptr_layout->internal_id_current_window);
        gui_layout_remove (ptr_layout);
        (void) hook_signal_send ("window_unzoomed",
                                 WEECHAT_HOOK_SIGNAL_POINTER,
                                 gui_current_window);
    }
    else
    {
        /* store layout and zoom on current window */
        ptr_layout = gui_layout_alloc (GUI_LAYOUT_ZOOM);
        if (ptr_layout)
        {
            gui_layout_add (ptr_layout);
            (void) hook_signal_send ("window_zoom",
                                     WEECHAT_HOOK_SIGNAL_POINTER,
                                     gui_current_window);
            gui_layout_window_store (ptr_layout);
            gui_window_merge_all (window);
            (void) hook_signal_send ("window_zoomed",
                                     WEECHAT_HOOK_SIGNAL_POINTER,
                                     gui_current_window);
        }
    }
}

/*
 * Returns hdata for window.
 */

struct t_hdata *
gui_window_hdata_window_cb (const void *pointer, void *data,
                            const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_window", "next_window",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_window, number, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window, win_x, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window, win_y, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window, win_width, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window, win_height, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window, win_width_pct, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window, win_height_pct, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window, win_chat_x, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window, win_chat_y, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window, win_chat_width, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window, win_chat_height, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window, win_chat_cursor_x, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window, win_chat_cursor_y, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window, bar_windows, POINTER, 0, NULL, "bar_window");
        HDATA_VAR(struct t_gui_window, last_bar_window, POINTER, 0, NULL, "bar_window");
        HDATA_VAR(struct t_gui_window, refresh_needed, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window, gui_objects, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window, buffer, POINTER, 0, NULL, "buffer");
        HDATA_VAR(struct t_gui_window, layout_plugin_name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window, layout_buffer_name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window, scroll, POINTER, 0, NULL, "window_scroll");
        HDATA_VAR(struct t_gui_window, ptr_tree, POINTER, 0, NULL, "window_tree");
        HDATA_VAR(struct t_gui_window, prev_window, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_window, next_window, POINTER, 0, NULL, hdata_name);
        HDATA_LIST(gui_windows, WEECHAT_HDATA_LIST_CHECK_POINTERS);
        HDATA_LIST(last_gui_window, 0);
        HDATA_LIST(gui_current_window, 0);
    }
    return hdata;
}

/*
 * Returns hdata for window scroll.
 */

struct t_hdata *
gui_window_hdata_window_scroll_cb (const void *pointer, void *data,
                                   const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_scroll", "next_scroll",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_window_scroll, buffer, POINTER, 0, NULL, "buffer");
        HDATA_VAR(struct t_gui_window_scroll, first_line_displayed, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window_scroll, start_line, POINTER, 0, NULL, "line");
        HDATA_VAR(struct t_gui_window_scroll, start_line_pos, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window_scroll, scrolling, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window_scroll, start_col, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window_scroll, lines_after, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window_scroll, text_search_start_line, POINTER, 0, NULL, "line");
        HDATA_VAR(struct t_gui_window_scroll, prev_scroll, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_window_scroll, next_scroll, POINTER, 0, NULL, hdata_name);
    }
    return hdata;
}

/*
 * Returns hdata for window tree.
 */

struct t_hdata *
gui_window_hdata_window_tree_cb (const void *pointer, void *data,
                                 const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, NULL, NULL, 0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_window_tree, parent_node, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_window_tree, split_pct, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window_tree, split_horizontal, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_window_tree, child1, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_window_tree, child2, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_window_tree, window, POINTER, 0, NULL, "window");
        HDATA_LIST(gui_windows_tree, 0);
    }
    return hdata;
}

/*
 * Adds a window in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_window_add_to_infolist (struct t_infolist *infolist,
                            struct t_gui_window *window)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !window)
        return 0;

    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_pointer (ptr_item, "pointer", window))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "current_window",
                                   (gui_current_window == window) ? 1 : 0))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "number", window->number))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "x", window->win_x))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "y", window->win_y))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "width", window->win_width))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "height", window->win_height))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "width_pct", window->win_width_pct))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "height_pct", window->win_height_pct))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "chat_x", window->win_chat_x))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "chat_y", window->win_chat_y))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "chat_width", window->win_chat_width))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "chat_height", window->win_chat_height))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "buffer", window->buffer))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "start_line_y",
                                   ((window->buffer->type == GUI_BUFFER_TYPE_FREE)
                                    && (window->scroll->start_line)) ?
                                   window->scroll->start_line->data->y : 0))
        return 0;

    return 1;
}

/*
 * Prints window infos in WeeChat log file (usually for crash dump).
 */

void
gui_window_print_log ()
{
    struct t_gui_window *ptr_window;
    struct t_gui_window_scroll *ptr_scroll;
    struct t_gui_bar_window *ptr_bar_win;

    log_printf ("");
    log_printf ("gui_windows . . . . . . . . . : %p", gui_windows);
    log_printf ("last_gui_window . . . . . . . : %p", last_gui_window);
    log_printf ("gui_current window. . . . . . : %p", gui_current_window);
    log_printf ("gui_windows_tree. . . . . . . : %p", gui_windows_tree);

    for (ptr_window = gui_windows; ptr_window; ptr_window = ptr_window->next_window)
    {
        log_printf ("");
        log_printf ("[window (addr:%p)]", ptr_window);
        log_printf ("  number. . . . . . . : %d", ptr_window->number);
        log_printf ("  win_x . . . . . . . : %d", ptr_window->win_x);
        log_printf ("  win_y . . . . . . . : %d", ptr_window->win_y);
        log_printf ("  win_width . . . . . : %d", ptr_window->win_width);
        log_printf ("  win_height. . . . . : %d", ptr_window->win_height);
        log_printf ("  win_width_pct . . . : %d", ptr_window->win_width_pct);
        log_printf ("  win_height_pct. . . : %d", ptr_window->win_height_pct);
        log_printf ("  win_chat_x. . . . . : %d", ptr_window->win_chat_x);
        log_printf ("  win_chat_y. . . . . : %d", ptr_window->win_chat_y);
        log_printf ("  win_chat_width. . . : %d", ptr_window->win_chat_width);
        log_printf ("  win_chat_height . . : %d", ptr_window->win_chat_height);
        log_printf ("  win_chat_cursor_x . : %d", ptr_window->win_chat_cursor_x);
        log_printf ("  win_chat_cursor_y . : %d", ptr_window->win_chat_cursor_y);
        log_printf ("  refresh_needed. . . : %d", ptr_window->refresh_needed);
        log_printf ("  gui_objects . . . . : %p", ptr_window->gui_objects);
        gui_window_objects_print_log (ptr_window);
        log_printf ("  buffer. . . . . . . : %p", ptr_window->buffer);
        log_printf ("  layout_plugin_name. : '%s'", ptr_window->layout_plugin_name);
        log_printf ("  layout_buffer_name. : '%s'", ptr_window->layout_buffer_name);
        log_printf ("  scroll. . . . . . . : %p", ptr_window->scroll);
        log_printf ("  coords_size . . . . : %d", ptr_window->coords_size);
        log_printf ("  coords. . . . . . . : %p", ptr_window->coords);
        log_printf ("  ptr_tree. . . . . . : %p", ptr_window->ptr_tree);
        log_printf ("  prev_window . . . . : %p", ptr_window->prev_window);
        log_printf ("  next_window . . . . : %p", ptr_window->next_window);

        for (ptr_scroll = ptr_window->scroll; ptr_scroll;
             ptr_scroll = ptr_scroll->next_scroll)
        {
            log_printf ("");
            log_printf ("  [scroll (addr:%p)]", ptr_scroll);
            log_printf ("    buffer. . . . . . . . : %p", ptr_scroll->buffer);
            log_printf ("    first_line_displayed. : %d", ptr_scroll->first_line_displayed);
            log_printf ("    start_line. . . . . . : %p", ptr_scroll->start_line);
            log_printf ("    start_line_pos. . . . : %d", ptr_scroll->start_line_pos);
            log_printf ("    scrolling . . . . . . : %d", ptr_scroll->scrolling);
            log_printf ("    start_col . . . . . . : %d", ptr_scroll->start_col);
            log_printf ("    lines_after . . . . . : %d", ptr_scroll->lines_after);
            log_printf ("    text_search_start_line: %p", ptr_scroll->text_search_start_line);
            log_printf ("    prev_scroll . . . . . : %p", ptr_scroll->prev_scroll);
            log_printf ("    next_scroll . . . . . : %p", ptr_scroll->next_scroll);
        }

        for (ptr_bar_win = ptr_window->bar_windows; ptr_bar_win;
             ptr_bar_win = ptr_bar_win->next_bar_window)
        {
            gui_bar_window_print_log (ptr_bar_win);
        }
    }
}
