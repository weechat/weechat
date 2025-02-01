/*
 * gui-bar-window.c - bar window functions (used by all GUI)
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <limits.h>
#include <string.h>

#include "../core/weechat.h"
#include "../core/core-config.h"
#include "../core/core-hashtable.h"
#include "../core/core-hdata.h"
#include "../core/core-infolist.h"
#include "../core/core-log.h"
#include "../core/core-string.h"
#include "../plugins/plugin.h"
#include "gui-bar-window.h"
#include "gui-bar.h"
#include "gui-bar-item.h"
#include "gui-buffer.h"
#include "gui-chat.h"
#include "gui-color.h"
#include "gui-window.h"


/*
 * Checks if a bar window pointer is valid.
 *
 * Returns:
 *   1: bar window exists
 *   0: bar window does not exist
 */

int
gui_bar_window_valid (struct t_gui_bar_window *bar_window)
{
    struct t_gui_bar *ptr_bar;
    struct t_gui_window *ptr_window;
    struct t_gui_bar_window *ptr_bar_window;

    if (!bar_window)
        return 0;

    /* check root bars */
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (ptr_bar->bar_window && (ptr_bar->bar_window == bar_window))
            return 1;
    }

    /* check window bars */
    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        for (ptr_bar_window = ptr_window->bar_windows; ptr_bar_window;
             ptr_bar_window = ptr_bar_window->next_bar_window)
        {
            if (ptr_bar_window == bar_window)
                return 1;
        }
    }

    /* bar window not found */
    return 0;
}

/*
 * Searches for a reference to a bar in a window.
 *
 * Returns pointer to bar window found, NULL if not found.
 */

struct t_gui_bar_window *
gui_bar_window_search_bar (struct t_gui_window *window, struct t_gui_bar *bar)
{
    struct t_gui_bar_window *ptr_bar_win;

    if (!window)
        return NULL;

    for (ptr_bar_win = window->bar_windows; ptr_bar_win;
         ptr_bar_win = ptr_bar_win->next_bar_window)
    {
        if (ptr_bar_win->bar == bar)
            return ptr_bar_win;
    }

    /* bar window not found for window */
    return NULL;
}

/*
 * Gets bar_window pointer displayed at (x,y).
 *
 * If window is not NULL, search is done in bar windows of window.
 * If window is NULL, search is done in root bar windows.
 */

void
gui_bar_window_search_by_xy (struct t_gui_window *window, int x, int y,
                             struct t_gui_bar_window **bar_window,
                             char **bar_item,
                             int *bar_item_line, int *bar_item_col,
                             struct t_gui_buffer **buffer)
{
    struct t_gui_bar *ptr_bar;
    struct t_gui_bar_window *ptr_bar_window;
    int filling, position, num_cols, column, lines, lines_old, i, j;
    int coord_x, coord_y, item, subitem;

    *bar_window = NULL;
    *bar_item = NULL;
    *bar_item_line = -1;
    *bar_item_col = -1;

    if (window)
    {
        for (ptr_bar_window = window->bar_windows; ptr_bar_window;
             ptr_bar_window = ptr_bar_window->next_bar_window)
        {
            if ((x >= ptr_bar_window->x) && (y >= ptr_bar_window->y)
                && (x <= ptr_bar_window->x + ptr_bar_window->width - 1)
                && (y <= ptr_bar_window->y + ptr_bar_window->height - 1))
            {
                *bar_window = ptr_bar_window;
                break;
            }
        }
    }
    else
    {
        for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
        {
            if (ptr_bar->bar_window
                && (x >= ptr_bar->bar_window->x)
                && (y >= ptr_bar->bar_window->y)
                && (x <= ptr_bar->bar_window->x + ptr_bar->bar_window->width - 1)
                && (y <= ptr_bar->bar_window->y + ptr_bar->bar_window->height - 1))
            {
                *bar_window = ptr_bar->bar_window;
                break;
            }
        }
    }

    if (*bar_window)
    {
        filling = gui_bar_get_filling ((*bar_window)->bar);
        position = CONFIG_ENUM((*bar_window)->bar->options[GUI_BAR_OPTION_POSITION]);

        *bar_item_line = y - (*bar_window)->y + (*bar_window)->scroll_y;
        *bar_item_col = x - (*bar_window)->x + (*bar_window)->scroll_x;

        if ((filling == GUI_BAR_FILLING_COLUMNS_HORIZONTAL)
            && ((*bar_window)->screen_col_size > 0))
        {
            if ((position == GUI_BAR_POSITION_LEFT)
                || (position == GUI_BAR_POSITION_RIGHT))
            {
                /*
                 * when the bar is on left/right, the last space (after last
                 * column) is not displayed, so we add 1 to width for finding
                 * number of columns
                 */
                num_cols = ((*bar_window)->width + 1) / (*bar_window)->screen_col_size;
            }
            else
            {
                num_cols = (*bar_window)->width / (*bar_window)->screen_col_size;
            }
            column = *bar_item_col / (*bar_window)->screen_col_size;
            *bar_item_line = (*bar_item_line * num_cols) + column;
            *bar_item_col = *bar_item_col - (column * ((*bar_window)->screen_col_size));
        }

        if ((filling == GUI_BAR_FILLING_COLUMNS_VERTICAL)
            && ((*bar_window)->screen_col_size > 0))
        {
            column = *bar_item_col / (*bar_window)->screen_col_size;
            *bar_item_line = (column * ((*bar_window)->screen_lines)) + *bar_item_line;
            *bar_item_col = *bar_item_col % ((*bar_window)->screen_col_size);
        }

        if (filling == GUI_BAR_FILLING_HORIZONTAL)
        {
            i = 0;
            while (i < (*bar_window)->coords_count)
            {
                coord_x = (*bar_window)->coords[i]->x;
                coord_y = (*bar_window)->coords[i]->y;
                while ((i < (*bar_window)->coords_count - 1)
                       && (coord_x == (*bar_window)->coords[i + 1]->x)
                       && (coord_y == (*bar_window)->coords[i + 1]->y))
                {
                    i++;
                }
                if (i == (*bar_window)->coords_count - 1)
                {
                    item = (*bar_window)->coords[i]->item;
                    subitem = (*bar_window)->coords[i]->subitem;
                    if ((item >= 0) && (subitem >= 0))
                    {
                        *bar_item = (*bar_window)->bar->items_name[item][subitem];
                        *bar_item_line = (*bar_window)->coords[i]->line;
                        *bar_item_col = x - (*bar_window)->coords[i]->x;
                    }
                    break;
                }
                coord_x = (*bar_window)->coords[i + 1]->x;
                coord_y = (*bar_window)->coords[i + 1]->y;
                if ((y < coord_y) || ((y == coord_y) && (x < coord_x)))
                {
                    item = (*bar_window)->coords[i]->item;
                    subitem = (*bar_window)->coords[i]->subitem;
                    *bar_item = (*bar_window)->bar->items_name[item][subitem];
                    *bar_item_line = (*bar_window)->coords[i]->line;
                    *bar_item_col = x - (*bar_window)->coords[i]->x;
                    if ((*bar_window)->bar->items_buffer[item][subitem])
                        *buffer = gui_buffer_search_by_full_name ((*bar_window)->bar->items_buffer[item][subitem]);
                    break;
                }
                i++;
            }
        }
        else
        {
            i = 0;
            j = 0;
            lines = 0;
            lines_old = 0;
            while (i < (*bar_window)->items_count)
            {
                lines_old = lines;
                lines += (*bar_window)->items_num_lines[i][j];
                if (*bar_item_line < lines)
                {
                    *bar_item = (*bar_window)->bar->items_name[i][j];
                    if ((*bar_window)->bar->items_buffer[i][j])
                        *buffer = gui_buffer_search_by_full_name ((*bar_window)->bar->items_buffer[i][j]);
                    break;
                }
                j++;
                if (j >= (*bar_window)->items_subcount[i])
                {
                    j = 0;
                    i++;
                }
            }
            *bar_item_line -= lines_old;
        }
    }
}

/*
 * Gets total bar size (window bars) for a position.
 *
 * Bar is optional, if not NULL, computes size from bar 1 to bar # - 1.
 */

int
gui_bar_window_get_size (struct t_gui_bar *bar, struct t_gui_window *window,
                         enum t_gui_bar_position position)
{
    struct t_gui_bar_window *ptr_bar_window;
    int total_size;

    total_size = 0;

    if (!window)
        return total_size;

    for (ptr_bar_window = window->bar_windows; ptr_bar_window;
         ptr_bar_window = ptr_bar_window->next_bar_window)
    {
        /* stop before bar */
        if (bar && (ptr_bar_window->bar == bar))
            return total_size;

        if (!CONFIG_BOOLEAN(ptr_bar_window->bar->options[GUI_BAR_OPTION_HIDDEN]))
        {
            if ((CONFIG_ENUM(ptr_bar_window->bar->options[GUI_BAR_OPTION_TYPE]) != GUI_BAR_TYPE_ROOT)
                && (CONFIG_ENUM(ptr_bar_window->bar->options[GUI_BAR_OPTION_POSITION]) == (int)position))
            {
                switch (position)
                {
                    case GUI_BAR_POSITION_BOTTOM:
                    case GUI_BAR_POSITION_TOP:
                        total_size += ptr_bar_window->height;
                        break;
                    case GUI_BAR_POSITION_LEFT:
                    case GUI_BAR_POSITION_RIGHT:
                        total_size += ptr_bar_window->width;
                        break;
                    case GUI_BAR_NUM_POSITIONS:
                        break;
                }
                if (CONFIG_INTEGER(ptr_bar_window->bar->options[GUI_BAR_OPTION_SEPARATOR]))
                    total_size++;
            }
        }
    }

    return total_size;
}

/*
 * Calculates position and size of a bar.
 */

void
gui_bar_window_calculate_pos_size (struct t_gui_bar_window *bar_window,
                                   struct t_gui_window *window)
{
    int x1, y1, x2, y2;
    int add_bottom, add_top, add_left, add_right;

    if (!bar_window
        || CONFIG_BOOLEAN(bar_window->bar->options[GUI_BAR_OPTION_HIDDEN]))
    {
        return;
    }

    if (window)
    {
        x1 = window->win_x;
        y1 = window->win_y;
        x2 = x1 + window->win_width - 1;
        y2 = y1 + window->win_height - 1;
        add_bottom = gui_bar_window_get_size (bar_window->bar, window, GUI_BAR_POSITION_BOTTOM);
        add_top = gui_bar_window_get_size (bar_window->bar, window, GUI_BAR_POSITION_TOP);
        add_left = gui_bar_window_get_size (bar_window->bar, window, GUI_BAR_POSITION_LEFT);
        add_right = gui_bar_window_get_size (bar_window->bar, window, GUI_BAR_POSITION_RIGHT);
    }
    else
    {
        x1 = 0;
        y1 = 0;
        x2 = gui_window_get_width () - 1;
        y2 = gui_window_get_height () - 1;
        add_bottom = gui_bar_root_get_size (bar_window->bar, GUI_BAR_POSITION_BOTTOM);
        add_top = gui_bar_root_get_size (bar_window->bar, GUI_BAR_POSITION_TOP);
        add_left = gui_bar_root_get_size (bar_window->bar, GUI_BAR_POSITION_LEFT);
        add_right = gui_bar_root_get_size (bar_window->bar, GUI_BAR_POSITION_RIGHT);
    }

    switch (CONFIG_ENUM(bar_window->bar->options[GUI_BAR_OPTION_POSITION]))
    {
        case GUI_BAR_POSITION_BOTTOM:
            bar_window->x = x1 + add_left;
            bar_window->y = y2 - add_bottom - bar_window->current_size + 1;
            bar_window->width = x2 - x1 + 1 - add_left - add_right;
            bar_window->height = bar_window->current_size;
            break;
        case GUI_BAR_POSITION_TOP:
            bar_window->x = x1 + add_left;
            bar_window->y = y1 + add_top;
            bar_window->width = x2 - x1 + 1 - add_left - add_right;
            bar_window->height = bar_window->current_size;
            break;
        case GUI_BAR_POSITION_LEFT:
            bar_window->x = x1 + add_left;
            bar_window->y = y1 + add_top;
            bar_window->width = bar_window->current_size;
            bar_window->height = y2 - y1 + 1 - add_top - add_bottom;
            break;
        case GUI_BAR_POSITION_RIGHT:
            bar_window->x = x2 - add_right - bar_window->current_size + 1;
            bar_window->y = y1 + add_top;
            bar_window->width = bar_window->current_size;
            bar_window->height = y2 - y1 + 1 - add_top - add_bottom;
            break;
        case GUI_BAR_NUM_POSITIONS:
            break;
    }

    /* bar window cannot be displayed? (not enough space left) */
    if ((bar_window->x < x1) || (bar_window->x > x2)
        || (bar_window->y < y1) || (bar_window->y > y2)
        || (bar_window->width < 1) || (bar_window->height < 1))
    {
        /* invalidate the bar window, it will not be displayed */
        bar_window->x = -1;
        bar_window->y = -1;
        bar_window->width = 0;
        bar_window->height = 0;
    }
}

/*
 * Searches for position of a bar window (to keep list sorted by bar priority).
 */

struct t_gui_bar_window *
gui_bar_window_find_pos (struct t_gui_bar *bar, struct t_gui_window *window)
{
    struct t_gui_bar_window *ptr_bar_window;

    if (!window)
        return NULL;

    for (ptr_bar_window = window->bar_windows; ptr_bar_window;
         ptr_bar_window = ptr_bar_window->next_bar_window)
    {
        if (CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_PRIORITY]) >=
            CONFIG_INTEGER(ptr_bar_window->bar->options[GUI_BAR_OPTION_PRIORITY]))
            return ptr_bar_window;
    }

    /* position not found, best position is at the end */
    return NULL;
}

/*
 * Allocates content for a bar window.
 */

void
gui_bar_window_content_alloc (struct t_gui_bar_window *bar_window)
{
    int i, j;

    if (!bar_window)
        return;

    bar_window->items_count = bar_window->bar->items_count;
    bar_window->items_subcount = NULL;
    bar_window->items_content = NULL;
    bar_window->items_num_lines = NULL;
    bar_window->items_refresh_needed = NULL;
    bar_window->screen_col_size = 0;
    bar_window->screen_lines = 0;
    bar_window->items_subcount = calloc (1,
                                         bar_window->items_count *
                                         sizeof (*bar_window->items_subcount));
    if (!bar_window->items_subcount)
        goto error;
    bar_window->items_content = calloc (1,
                                        (bar_window->items_count) *
                                        sizeof (*bar_window->items_content));
    if (!bar_window->items_content)
        goto error;
    bar_window->items_num_lines = calloc (1,
                                          (bar_window->items_count) *
                                          sizeof (*bar_window->items_num_lines));
    if (!bar_window->items_num_lines)
        goto error;
    bar_window->items_refresh_needed = calloc (1,
                                               bar_window->items_count *
                                               sizeof (*bar_window->items_refresh_needed));
    if (!bar_window->items_refresh_needed)
        goto error;

    for (i = 0; i < bar_window->items_count; i++)
    {
        bar_window->items_content[i] = NULL;
        bar_window->items_num_lines[i] = NULL;
        bar_window->items_refresh_needed[i] = NULL;
    }

    for (i = 0; i < bar_window->items_count; i++)
    {
        bar_window->items_subcount[i] = bar_window->bar->items_subcount[i];
        bar_window->items_content[i] = malloc (bar_window->items_subcount[i] *
                                               sizeof (**bar_window->items_content));
        if (!bar_window->items_content[i])
            goto error;
        bar_window->items_num_lines[i] = malloc (bar_window->items_subcount[i] *
                                                 sizeof (**bar_window->items_num_lines));
        if (!bar_window->items_num_lines[i])
            goto error;
        bar_window->items_refresh_needed[i] = malloc (bar_window->items_subcount[i] *
                                                      sizeof (**bar_window->items_refresh_needed));
        if (!bar_window->items_refresh_needed[i])
            goto error;
        for (j = 0; j < bar_window->items_subcount[i]; j++)
        {
            if (bar_window->items_content[i])
                bar_window->items_content[i][j] = NULL;
            if (bar_window->items_num_lines[i])
                bar_window->items_num_lines[i][j] = 0;
            if (bar_window->items_refresh_needed[i])
                bar_window->items_refresh_needed[i][j] = 1;
        }
    }
    return;

error:
    if (bar_window->items_subcount)
    {
        free (bar_window->items_subcount);
        bar_window->items_subcount = NULL;
    }
    if (bar_window->items_content)
    {
        for (i = 0; i < bar_window->items_count; i++)
        {
            free (bar_window->items_content[i]);
        }
        free (bar_window->items_content);
        bar_window->items_content = NULL;
    }
    if (bar_window->items_num_lines)
    {
        for (i = 0; i < bar_window->items_count; i++)
        {
            free (bar_window->items_num_lines[i]);
        }
        free (bar_window->items_num_lines);
        bar_window->items_num_lines = NULL;
    }
    if (bar_window->items_refresh_needed)
    {
        for (i = 0; i < bar_window->items_count; i++)
        {
            free (bar_window->items_refresh_needed[i]);
        }
        free (bar_window->items_refresh_needed);
        bar_window->items_refresh_needed = NULL;
    }
}

/*
 * Frees content of a bar window.
 */

void
gui_bar_window_content_free (struct t_gui_bar_window *bar_window)
{
    int i, j;

    if (!bar_window)
        return;

    if (bar_window->items_content)
    {
        for (i = 0; i < bar_window->items_count; i++)
        {
            for (j = 0; j < bar_window->items_subcount[i]; j++)
            {
                if (bar_window->items_content[i]
                    && bar_window->items_content[i][j])
                {
                    free (bar_window->items_content[i][j]);
                }
            }
            free (bar_window->items_content[i]);
            free (bar_window->items_num_lines[i]);
            free (bar_window->items_refresh_needed[i]);
        }
        free (bar_window->items_subcount);
        bar_window->items_subcount = NULL;
        free (bar_window->items_content);
        bar_window->items_content = NULL;
        free (bar_window->items_num_lines);
        bar_window->items_num_lines = NULL;
        free (bar_window->items_refresh_needed);
        bar_window->items_refresh_needed = NULL;
    }
}

/*
 * Builds content of an item for a bar window.
 */

void
gui_bar_window_content_build_item (struct t_gui_bar_window *bar_window,
                                   struct t_gui_window *window,
                                   int index_item, int index_subitem)
{
    if (!bar_window)
        return;

    if (bar_window->items_content)
    {
        if (bar_window->items_content[index_item][index_subitem])
        {
            free (bar_window->items_content[index_item][index_subitem]);
            bar_window->items_content[index_item][index_subitem] = NULL;
        }
        bar_window->items_num_lines[index_item][index_subitem] = 0;

        /* build item, but only if there's a buffer in window */
        if ((window && window->buffer)
            || (gui_current_window && gui_current_window->buffer))
        {
            bar_window->items_content[index_item][index_subitem] =
                gui_bar_item_get_value (bar_window->bar, window,
                                        index_item, index_subitem);
            bar_window->items_num_lines[index_item][index_subitem] =
                gui_bar_item_count_lines (bar_window->items_content[index_item][index_subitem]);
            bar_window->items_refresh_needed[index_item][index_subitem] = 0;
        }
    }
}

/*
 * Builds content of a bar window: calls callback for each item, then
 * concatenates values (according to bar position and filling).
 */

void
gui_bar_window_content_build (struct t_gui_bar_window *bar_window,
                              struct t_gui_window *window)
{
    int i, j;

    if (!bar_window)
        return;

    gui_bar_window_content_free (bar_window);
    gui_bar_window_content_alloc (bar_window);

    for (i = 0; i < bar_window->items_count; i++)
    {
        for (j = 0; j < bar_window->items_subcount[i]; j++)
        {
            gui_bar_window_content_build_item (bar_window, window, i, j);
        }
    }
}

/*
 * Gets item or subitem content (first rebuilds content if refresh is needed).
 */

const char *
gui_bar_window_content_get (struct t_gui_bar_window *bar_window,
                            struct t_gui_window *window,
                            int index_item, int index_subitem)
{
    if (!bar_window)
        return NULL;

    /* rebuild content if refresh is needed */
    if (bar_window->items_refresh_needed[index_item][index_subitem])
    {
        gui_bar_window_content_build_item (bar_window, window,
                                           index_item, index_subitem);
    }

    /* return content */
    return bar_window->items_content[index_item][index_subitem];
}

/*
 * Checks if the item content is a spacer (bar item "spacer").
 *
 * Returns:
 *    1: item is a spacer
 *    1: item is not a spacer
 */

int
gui_bar_window_item_is_spacer (const char *item)
{
    return (item
            && (item[0] == GUI_COLOR_COLOR_CHAR)
            && (item[1] == GUI_COLOR_BAR_CHAR)
            && (item[2] == GUI_COLOR_BAR_SPACER)
            && !item[3]);
}

/*
 * Gets content of a bar window, formatted for display, according to filling
 * for bar position.
 *
 * The integer variable *num_spacers is set with the number of spacers found
 * (bar item "spacer").
 *
 * Note: result must be freed after use.
 */

char *
gui_bar_window_content_get_with_filling (struct t_gui_bar_window *bar_window,
                                         struct t_gui_window *window,
                                         int *num_spacers)
{
    enum t_gui_bar_filling filling;
    const char *ptr_content;
    char **content, str_reinit_color[32];
    char str_reinit_color_space[32], str_reinit_color_space_start_line[32];
    char str_start_item[32];
    char *item_value, *item_value2, ****split_items, **linear_items, **ptr_item;
    int i, j, k, sub, index;
    int at_least_one_item, first_sub_item;
    int length_reinit_color, length_reinit_color_space, length_start_item;
    int length, max_length, max_length_screen;
    int total_items, columns, lines;
    int item_is_spacer;

    *num_spacers = 0;

    if (!bar_window
        || !bar_window->items_subcount || !bar_window->items_content
        || !bar_window->items_num_lines || !bar_window->items_refresh_needed)
    {
        return NULL;
    }

    snprintf (str_reinit_color, sizeof (str_reinit_color),
              "%c",
              GUI_COLOR_RESET_CHAR);
    length_reinit_color = strlen (str_reinit_color);

    snprintf (str_reinit_color_space, sizeof (str_reinit_color_space),
              "%c ",
              GUI_COLOR_RESET_CHAR);
    length_reinit_color_space = strlen (str_reinit_color_space);

    snprintf (str_reinit_color_space_start_line,
              sizeof (str_reinit_color_space_start_line),
              "%c %c%c%c",
              GUI_COLOR_RESET_CHAR,
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_BAR_CHAR,
              GUI_COLOR_BAR_START_LINE_ITEM);

    snprintf (str_start_item, sizeof (str_start_item),
              "%c%c%c",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_BAR_CHAR,
              GUI_COLOR_BAR_START_ITEM);
    length_start_item = strlen (str_start_item);

    content = string_dyn_alloc (256);
    filling = gui_bar_get_filling (bar_window->bar);
    at_least_one_item = 0;
    switch (filling)
    {
        case GUI_BAR_FILLING_HORIZONTAL: /* items separated by space */
        case GUI_BAR_FILLING_VERTICAL:   /* items separated by \n */
            for (i = 0; i < bar_window->items_count; i++)
            {
                first_sub_item = 1;
                for (sub = 0; sub < bar_window->items_subcount[i]; sub++)
                {
                    ptr_content = gui_bar_window_content_get (bar_window, window,
                                                              i, sub);
                    if (ptr_content && ptr_content[0])
                    {
                        if (filling == GUI_BAR_FILLING_HORIZONTAL)
                        {
                            item_value = string_replace (ptr_content, "\n",
                                                         str_reinit_color_space_start_line);
                            if (item_value)
                            {
                                item_value2 = string_replace (item_value,
                                                              "\r", "\n");
                                if (item_value2)
                                {
                                    free (item_value);
                                    item_value = item_value2;
                                }
                            }
                        }
                        else
                        {
                            item_value = NULL;
                        }
                        item_is_spacer = gui_bar_window_item_is_spacer (
                            (item_value) ? item_value : ptr_content);
                        if (at_least_one_item && first_sub_item
                            && !item_is_spacer)
                        {
                            /* first sub item: insert space after last item */
                            if (filling == GUI_BAR_FILLING_HORIZONTAL)
                            {
                                string_dyn_concat (content,
                                                   str_reinit_color_space,
                                                   length_reinit_color_space);
                            }
                            else
                            {
                                string_dyn_concat (content, "\n", -1);
                            }
                        }
                        else
                        {
                            string_dyn_concat (content, str_reinit_color,
                                               length_reinit_color);
                        }
                        if (filling == GUI_BAR_FILLING_HORIZONTAL)
                        {
                            string_dyn_concat (content, str_start_item,
                                               length_start_item);
                        }
                        string_dyn_concat (
                            content,
                            (item_value) ? item_value : ptr_content,
                            -1);
                        first_sub_item = 0;
                        free (item_value);
                        if (item_is_spacer)
                            (*num_spacers)++;
                        else
                            at_least_one_item = 1;
                    }
                    else
                    {
                        if (filling == GUI_BAR_FILLING_HORIZONTAL)
                        {
                            string_dyn_concat (content, str_start_item,
                                               length_start_item);
                        }
                    }
                }
            }
            if (filling == GUI_BAR_FILLING_HORIZONTAL)
                string_dyn_concat (content, str_start_item, length_start_item);
            break;
        case GUI_BAR_FILLING_COLUMNS_HORIZONTAL: /* items in columns, with horizontal filling */
        case GUI_BAR_FILLING_COLUMNS_VERTICAL:   /* items in columns, with vertical filling */
            total_items = 0;
            max_length = 1;
            max_length_screen = 1;
            split_items = malloc (bar_window->items_count * sizeof (*split_items));
            for (i = 0; i < bar_window->items_count; i++)
            {
                if (bar_window->items_subcount[i] > 0)
                {
                    split_items[i] = malloc (bar_window->items_subcount[i] * sizeof (**split_items));
                    for (sub = 0; sub < bar_window->items_subcount[i]; sub++)
                    {
                        ptr_content = gui_bar_window_content_get (bar_window, window,
                                                                  i, sub);
                        if (ptr_content && ptr_content[0])
                        {
                            split_items[i][sub] = string_split (
                                ptr_content,
                                "\n",
                                NULL,
                                WEECHAT_STRING_SPLIT_STRIP_LEFT
                                | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                0,
                                NULL);
                            for (ptr_item = split_items[i][sub]; *ptr_item; ptr_item++)
                            {
                                total_items++;

                                length = strlen (*ptr_item);
                                if (length > max_length)
                                    max_length = length;

                                length = gui_chat_strlen_screen (*ptr_item);
                                if (length > max_length_screen)
                                    max_length_screen = length;
                            }
                        }
                        else
                            split_items[i][sub] = NULL;
                    }
                }
                else
                    split_items[i] = NULL;
            }
            if ((CONFIG_ENUM(bar_window->bar->options[GUI_BAR_OPTION_POSITION]) == GUI_BAR_POSITION_BOTTOM)
                || (CONFIG_ENUM(bar_window->bar->options[GUI_BAR_OPTION_POSITION]) == GUI_BAR_POSITION_TOP))
            {
                columns = bar_window->width / (max_length_screen + 1);
                if (columns == 0)
                    columns = 1;
                lines = total_items / columns;
                if (total_items % columns != 0)
                    lines++;
            }
            else
            {
                columns = total_items / bar_window->height;
                if (total_items % bar_window->height != 0)
                    columns++;
                lines = bar_window->height;
            }
            bar_window->screen_col_size = max_length_screen + 1;
            bar_window->screen_lines = lines;

            /* build array with pointers to split items */

            linear_items = (total_items > 0) ?
                malloc (total_items * sizeof (*linear_items)) : NULL;
            if (linear_items)
            {
                index = 0;
                for (i = 0; i < bar_window->items_count; i++)
                {
                    if (split_items[i])
                    {
                        for (sub = 0; sub < bar_window->items_subcount[i]; sub++)
                        {
                            if (split_items[i][sub])
                            {
                                for (ptr_item = split_items[i][sub]; *ptr_item;
                                     ptr_item++)
                                {
                                    linear_items[index++] = *ptr_item;
                                }
                            }
                        }
                    }
                }

                /* build content with lines and columns */
                for (i = 0; i < lines; i++)
                {
                    for (j = 0; j < columns; j++)
                    {
                        if (filling == GUI_BAR_FILLING_COLUMNS_HORIZONTAL)
                            index = (i * columns) + j;
                        else
                            index = (j * lines) + i;

                        if (index >= total_items)
                        {
                            for (k = 0; k < max_length_screen; k++)
                            {
                                string_dyn_concat (content, " ", -1);
                            }
                        }
                        else
                        {
                            string_dyn_concat (content, linear_items[index], -1);
                            length = max_length_screen -
                                gui_chat_strlen_screen (linear_items[index]);
                            for (k = 0; k < length; k++)
                            {
                                string_dyn_concat (content, " ", -1);
                            }
                        }
                        if (j < columns - 1)
                        {
                            string_dyn_concat (content, str_reinit_color_space,
                                               length_reinit_color_space);
                        }
                    }
                    string_dyn_concat (content, "\n", -1);
                }

                free (linear_items);
            }

            for (i = 0; i < bar_window->items_count; i++)
            {
                if (split_items[i])
                {
                    for (sub = 0; sub < bar_window->items_subcount[i]; sub++)
                    {
                        string_free_split (split_items[i][sub]);
                    }
                    free (split_items[i]);
                }
            }
            free (split_items);
            break;
        case GUI_BAR_NUM_FILLING:
            break;
    }

    if (!(*content)[0])
    {
        string_dyn_free (content, 1);
        return NULL;
    }

    return string_dyn_free (content, 0);
}

/*
 * Checks if spacer can be used in the bar according to its position, filling
 * and size.
 *
 * Returns:
 *   1: spacers can be used
 *   0: spacers cannot be used
 */

int
gui_bar_window_can_use_spacer (struct t_gui_bar_window *bar_window)
{
    int position, filling, bar_size;
    int pos_ok, filling_ok, size_ok;

    if (!bar_window)
        return 0;

    position = CONFIG_ENUM(bar_window->bar->options[GUI_BAR_OPTION_POSITION]);
    filling = gui_bar_get_filling (bar_window->bar);
    bar_size = CONFIG_INTEGER(bar_window->bar->options[GUI_BAR_OPTION_SIZE]);

    pos_ok = ((position == GUI_BAR_POSITION_TOP)
              || (position == GUI_BAR_POSITION_BOTTOM));
    filling_ok = (filling == GUI_BAR_FILLING_HORIZONTAL);
    size_ok = (bar_size == 1);

    return pos_ok && filling_ok && size_ok;
}

/*
 * Computes size of each spacer in the bar.
 *
 * Note: result must be freed after use.
 */

int *
gui_bar_window_compute_spacers_size (int length_on_screen,
                                     int bar_window_width,
                                     int num_spacers)
{
    int *spacers, spacer_size, i;

    if ((length_on_screen < 0) || (bar_window_width < 1) || (num_spacers <= 0))
        return NULL;

    /* if not enough space for spacers, ignore them all */
    if (length_on_screen >= bar_window_width)
        return NULL;

    spacers = malloc (num_spacers * sizeof (spacers[0]));
    if (!spacers)
        return NULL;

    spacer_size = (bar_window_width - length_on_screen) / num_spacers;

    for (i = 0; i < num_spacers; i++)
    {
        spacers[i] = spacer_size;
    }

    length_on_screen += num_spacers * spacer_size;

    i = 0;
    while ((length_on_screen < bar_window_width) && (i < num_spacers))
    {
        spacers[i]++;
        i++;
        length_on_screen++;
    }

    return spacers;
}

/*
 * Adds coordinates (item index/subindex and x,y).
 */

void
gui_bar_window_coords_add (struct t_gui_bar_window *bar_window,
                           int index_item, int index_subitem, int index_line,
                           int x, int y)
{
    struct t_gui_bar_window_coords **coords2;

    if (!bar_window)
        return;

    if (!bar_window->coords)
    {
        bar_window->coords_count = 1;
        bar_window->coords = malloc (sizeof (*(bar_window->coords)));
    }
    else
    {
        bar_window->coords_count++;
        coords2 = realloc (bar_window->coords,
                           bar_window->coords_count * sizeof (*(bar_window->coords)));
        if (!coords2)
        {
            if (bar_window->coords)
            {
                free (bar_window->coords);
                bar_window->coords = NULL;
            }
            bar_window->coords_count = 0;
            return;
        }
        bar_window->coords = coords2;
    }
    bar_window->coords[bar_window->coords_count - 1] = malloc (sizeof (*(bar_window->coords[bar_window->coords_count - 1])));
    bar_window->coords[bar_window->coords_count - 1]->item = index_item;
    bar_window->coords[bar_window->coords_count - 1]->subitem = index_subitem;
    bar_window->coords[bar_window->coords_count - 1]->line = index_line;
    bar_window->coords[bar_window->coords_count - 1]->x = x;
    bar_window->coords[bar_window->coords_count - 1]->y = y;
}

/*
 * Frees coords of a bar window.
 */

void
gui_bar_window_coords_free (struct t_gui_bar_window *bar_window)
{
    int i;

    if (!bar_window)
        return;

    if (bar_window->coords)
    {
        for (i = 0; i < bar_window->coords_count; i++)
        {
            free (bar_window->coords[i]);
        }
        free (bar_window->coords);
        bar_window->coords = NULL;
    }
    bar_window->coords_count = 0;
}

/*
 * Inserts bar window in list of bar windows (at good position, according to
 * priority).
 */

void
gui_bar_window_insert (struct t_gui_bar_window *bar_window,
                       struct t_gui_window *window)
{
    struct t_gui_bar_window *pos_bar_window;

    if (!bar_window || !window)
        return;

    if (window->bar_windows)
    {
        pos_bar_window = gui_bar_window_find_pos (bar_window->bar, window);
        if (pos_bar_window)
        {
            /* insert before bar window found */
            bar_window->prev_bar_window = pos_bar_window->prev_bar_window;
            bar_window->next_bar_window = pos_bar_window;
            if (pos_bar_window->prev_bar_window)
                (pos_bar_window->prev_bar_window)->next_bar_window = bar_window;
            else
                window->bar_windows = bar_window;
            pos_bar_window->prev_bar_window = bar_window;
        }
        else
        {
            /* add to end of list for window */
            bar_window->prev_bar_window = window->last_bar_window;
            bar_window->next_bar_window = NULL;
            (window->last_bar_window)->next_bar_window = bar_window;
            window->last_bar_window = bar_window;
        }
    }
    else
    {
        bar_window->prev_bar_window = NULL;
        bar_window->next_bar_window = NULL;
        window->bar_windows = bar_window;
        window->last_bar_window = bar_window;
    }
}

/*
 * Creates a new "window bar" for a bar, in screen or a window.
 *
 * If window is not NULL, bar window will be in this window.
 */

void
gui_bar_window_new (struct t_gui_bar *bar, struct t_gui_window *window)
{
    struct t_gui_bar_window *new_bar_window;

    if (CONFIG_BOOLEAN(bar->options[GUI_BAR_OPTION_HIDDEN]))
        return;

    if (window)
    {
        if ((CONFIG_ENUM(bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_WINDOW)
            && (!gui_bar_check_conditions (bar, window)))
            return;
    }

    new_bar_window =  malloc (sizeof (*new_bar_window));
    if (new_bar_window)
    {
        new_bar_window->bar = bar;
        if (window)
        {
            bar->bar_window = NULL;
            gui_bar_window_insert (new_bar_window, window);
        }
        else
        {
            bar->bar_window = new_bar_window;
            new_bar_window->prev_bar_window = NULL;
            new_bar_window->next_bar_window = NULL;
        }

        new_bar_window->x = 0;
        new_bar_window->y = 0;
        new_bar_window->width = 1;
        new_bar_window->height = 1;
        new_bar_window->scroll_x = 0;
        new_bar_window->scroll_y = 0;
        new_bar_window->cursor_x = -1;
        new_bar_window->cursor_y = -1;
        new_bar_window->current_size = (CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_SIZE]) == 0) ?
            1 : CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_SIZE]);
        new_bar_window->items_count = 0;
        new_bar_window->items_subcount = NULL;
        new_bar_window->items_content = NULL;
        new_bar_window->items_num_lines = NULL;
        new_bar_window->items_refresh_needed = NULL;
        new_bar_window->screen_col_size = 0;
        new_bar_window->screen_lines = 0;
        new_bar_window->coords_count = 0;
        new_bar_window->coords = NULL;
        gui_bar_window_objects_init (new_bar_window);
        gui_bar_window_content_alloc (new_bar_window);

        if (gui_init_ok)
        {
            gui_bar_window_calculate_pos_size (new_bar_window, window);
            gui_bar_window_create_win (new_bar_window);
            gui_window_ask_refresh (1);
        }
    }
}

/*
 * Gets current size of bar window.
 *
 * Returns width or height, depending on bar position.
 */

int
gui_bar_window_get_current_size (struct t_gui_bar_window *bar_window)
{
    return (bar_window) ? bar_window->current_size : 0;
}

/*
 * Returns max size for bar window in a window.
 */

int
gui_bar_window_get_max_size_in_window (struct t_gui_bar_window *bar_window,
                                       struct t_gui_window *window)
{
    int max_size;

    max_size = 1;

    if (bar_window && window)
    {
        switch (CONFIG_ENUM(bar_window->bar->options[GUI_BAR_OPTION_POSITION]))
        {
            case GUI_BAR_POSITION_BOTTOM:
            case GUI_BAR_POSITION_TOP:
                max_size = (window->win_chat_height + bar_window->height) - 1;
                break;
            case GUI_BAR_POSITION_LEFT:
            case GUI_BAR_POSITION_RIGHT:
                max_size = (window->win_chat_width + bar_window->width) - 1;
                break;
            case GUI_BAR_NUM_POSITIONS:
                break;
        }
    }

    return (max_size >= 1) ? max_size : -1;
}

/*
 * Returns max size for bar window.
 */

int
gui_bar_window_get_max_size (struct t_gui_bar_window *bar_window,
                             struct t_gui_window *window)
{
    int max_size_found, max_size;
    struct t_gui_window *ptr_window;

    if (window)
    {
        max_size_found = gui_bar_window_get_max_size_in_window (bar_window,
                                                                window);
    }
    else
    {
        max_size_found = INT_MAX;
        for (ptr_window = gui_windows; ptr_window;
             ptr_window = ptr_window->next_window)
        {
            max_size = gui_bar_window_get_max_size_in_window (bar_window,
                                                              ptr_window);
            if ((max_size >= 0) && (max_size < max_size_found))
                max_size_found = max_size;
        }
        if (max_size_found == INT_MAX)
            max_size_found = 1;
    }

    return max_size_found;
}

/*
 * Sets current size of all bar windows for a bar.
 */

void
gui_bar_window_set_current_size (struct t_gui_bar_window *bar_window,
                                 struct t_gui_window *window, int size)
{
    int new_size, max_size;

    if (!bar_window)
        return;

    if (size == 0)
        new_size = 1;
    else
    {
        new_size = size;
        if ((size != 0) && (CONFIG_INTEGER(bar_window->bar->options[GUI_BAR_OPTION_SIZE_MAX]) > 0)
            && (size > CONFIG_INTEGER(bar_window->bar->options[GUI_BAR_OPTION_SIZE_MAX])))
        {
            new_size = CONFIG_INTEGER(bar_window->bar->options[GUI_BAR_OPTION_SIZE_MAX]);
            if (new_size < 1)
                new_size = 1;
        }
    }

    if (bar_window->current_size != new_size)
    {
        max_size = gui_bar_window_get_max_size (bar_window, window);
        new_size = ((max_size >= 0) && (max_size < new_size)) ? max_size : new_size;
        if (bar_window->current_size != new_size)
        {
            bar_window->current_size = new_size;
            if (!CONFIG_BOOLEAN(bar_window->bar->options[GUI_BAR_OPTION_HIDDEN]))
            {
                gui_bar_window_calculate_pos_size (bar_window, window);
                gui_bar_window_create_win (bar_window);
                if (window)
                    window->refresh_needed = 1;
                else
                    gui_window_ask_refresh (1);
            }
        }
    }
}

/*
 * Frees a bar window.
 */

void
gui_bar_window_free (struct t_gui_bar_window *bar_window,
                     struct t_gui_window *window)
{
    if (!bar_window)
        return;

    /* remove window bar from list */
    if (window)
    {
        if (bar_window->prev_bar_window)
            (bar_window->prev_bar_window)->next_bar_window = bar_window->next_bar_window;
        if (bar_window->next_bar_window)
            (bar_window->next_bar_window)->prev_bar_window = bar_window->prev_bar_window;
        if (window->bar_windows == bar_window)
            window->bar_windows = bar_window->next_bar_window;
        if (window->last_bar_window == bar_window)
            window->last_bar_window = bar_window->prev_bar_window;
    }
    else
    {
        if (bar_window->bar)
            (bar_window->bar)->bar_window = NULL;
    }

    /* free data */
    gui_bar_window_content_free (bar_window);
    gui_bar_window_coords_free (bar_window);
    gui_bar_window_objects_free (bar_window);
    free (bar_window->gui_objects);

    free (bar_window);

    gui_window_ask_refresh (1);
}

/*
 * Removes unused bars, according to bars conditions.
 *
 * If window is NULL, unused root bars are removed.
 * If window is not NULL, unused window bars in this window are removed.
 *
 * Returns:
 *   1: at least one bar was removed
 *   0: no bar was removed
 */

int
gui_bar_window_remove_unused_bars (struct t_gui_window *window)
{
    int rc;
    struct t_gui_bar_window *ptr_bar_win, *next_bar_win;
    struct t_gui_bar *ptr_bar;

    rc = 0;

    if (window)
    {
        /* remove unused window bars in window */
        ptr_bar_win = window->bar_windows;
        while (ptr_bar_win)
        {
            next_bar_win = ptr_bar_win->next_bar_window;

            if ((CONFIG_ENUM(ptr_bar_win->bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_WINDOW)
                && (!gui_bar_check_conditions (ptr_bar_win->bar, window)))
            {
                gui_bar_window_free (ptr_bar_win, window);
                rc = 1;
            }

            ptr_bar_win = next_bar_win;
        }
    }
    else
    {
        /* remove unused root bars */
        for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
        {
            if ((CONFIG_ENUM(ptr_bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
                && ptr_bar->bar_window
                && (!gui_bar_check_conditions (ptr_bar, NULL)))
            {
                gui_bar_window_free (ptr_bar->bar_window, NULL);
                rc = 1;
            }
        }
    }

    return rc;
}

/*
 * Adds missing bars, according to bars conditions.
 *
 * If window is NULL, missing root bars are added.
 * If window is not NULL, missing window bars in this window are added.
 *
 * Returns:
 *   1: at least one bar was created
 *   0: no bar was created
 */

int
gui_bar_window_add_missing_bars (struct t_gui_window *window)
{
    int rc;
    struct t_gui_bar *ptr_bar;

    rc = 0;

    if (window)
    {
        /* add missing window bars in window */
        for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
        {
            if ((CONFIG_ENUM(ptr_bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_WINDOW)
                && (!gui_bar_window_search_bar (window, ptr_bar))
                && (gui_bar_check_conditions (ptr_bar, window)))
            {
                gui_bar_window_new (ptr_bar, window);
                rc = 1;
            }
        }
    }
    else
    {
        /* add missing root bars */
        for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
        {
            if ((CONFIG_ENUM(ptr_bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
                && !ptr_bar->bar_window
                && (gui_bar_check_conditions (ptr_bar, NULL)))
            {
                gui_bar_window_new (ptr_bar, NULL);
                rc = 1;
            }
        }
    }

    return rc;
}

/*
 * Scrolls a bar window with a value.
 *
 * If add == 1, then value is added (otherwise subtracted).
 * If add_x == 1, then value is added to scroll_x (otherwise scroll_y).
 * If percent == 1, then value is a percentage (otherwise number of chars).
 */

void
gui_bar_window_scroll (struct t_gui_bar_window *bar_window,
                       struct t_gui_window *window,
                       int add_x, int scroll_beginning, int scroll_end,
                       int add, int percent, int value)
{
    int old_scroll_x, old_scroll_y;

    if (!bar_window)
        return;

    old_scroll_x = bar_window->scroll_x;
    old_scroll_y = bar_window->scroll_y;

    if (scroll_beginning)
    {
        if (add_x)
            bar_window->scroll_x = 0;
        else
            bar_window->scroll_y = 0;
    }
    else if (scroll_end)
    {
        if (add_x)
            bar_window->scroll_x = INT_MAX;
        else
            bar_window->scroll_y = INT_MAX;
    }
    else
    {
        if (percent)
        {
            if (add_x)
                value = (bar_window->width * value) / 100;
            else
                value = (bar_window->height * value) / 100;
            if (value == 0)
                value = 1;
        }
        if (add)
        {
            if (add_x)
                bar_window->scroll_x += value;
            else
                bar_window->scroll_y += value;
        }
        else
        {
            if (add_x)
                bar_window->scroll_x -= value;
            else
                bar_window->scroll_y -= value;
        }
    }

    if (bar_window->scroll_x < 0)
        bar_window->scroll_x = 0;

    if (bar_window->scroll_y < 0)
        bar_window->scroll_y = 0;

    /* refresh only if scroll has changed (X and/or Y) */
    if ((old_scroll_x != bar_window->scroll_x)
        || (old_scroll_y != bar_window->scroll_y))
    {
        gui_bar_window_draw (bar_window, window);
    }
}

/*
 * Callback for updating data of a bar window.
 */

int
gui_bar_window_update_cb (void *data, struct t_hdata *hdata, void *pointer,
                          struct t_hashtable *hashtable)
{
    const char *value;
    int rc;

    /* make C compiler happy */
    (void) data;

    rc = 0;

    if (hashtable_has_key (hashtable, "scroll_x"))
    {
        value = hashtable_get (hashtable, "scroll_x");
        if (value)
        {
            hdata_set (hdata, pointer, "scroll_x", value);
            rc++;
        }
    }

    if (hashtable_has_key (hashtable, "scroll_y"))
    {
        value = hashtable_get (hashtable, "scroll_y");
        if (value)
        {
            hdata_set (hdata, pointer, "scroll_y", value);
            rc++;
        }
    }

    return rc;
}

/*
 * Returns hdata for bar window.
 */

struct t_hdata *
gui_bar_window_hdata_bar_window_cb (const void *pointer, void *data,
                                    const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_bar_window", "next_bar_window",
                       0, 0, &gui_bar_window_update_cb, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_bar_window, bar, POINTER, 0, NULL, "bar");
        HDATA_VAR(struct t_gui_bar_window, x, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_window, y, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_window, width, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_window, height, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_window, scroll_x, INTEGER, 1, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_window, scroll_y, INTEGER, 1, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_window, cursor_x, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_window, cursor_y, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_window, current_size, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_window, items_count, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_window, items_subcount, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_window, items_content, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_window, items_num_lines, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_window, items_refresh_needed, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_window, screen_col_size, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_window, screen_lines, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_window, coords_count, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_window, coords, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_window, gui_objects, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar_window, prev_bar_window, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_bar_window, next_bar_window, POINTER, 0, NULL, hdata_name);
    }
    return hdata;
}

/*
 * Adds a bar window in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_bar_window_add_to_infolist (struct t_infolist *infolist,
                                struct t_gui_bar_window *bar_window)
{
    struct t_infolist_item *ptr_item;
    int i, j;
    char option_name[64];

    if (!infolist || !bar_window)
        return 0;

    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_pointer (ptr_item, "bar", bar_window->bar))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "x", bar_window->x))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "y", bar_window->y))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "width", bar_window->width))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "height", bar_window->height))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "scroll_x", bar_window->scroll_x))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "scroll_y", bar_window->scroll_y))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "cursor_x", bar_window->cursor_x))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "cursor_y", bar_window->cursor_y))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "current_size", bar_window->current_size))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "items_count", bar_window->current_size))
        return 0;
    for (i = 0; i < bar_window->items_count; i++)
    {
        for (j = 0; j < bar_window->items_subcount[i]; j++)
        {
            snprintf (option_name, sizeof (option_name),
                      "items_content_%05d_%05d", i + 1, j + 1);
            if (!infolist_new_var_string (ptr_item, option_name,
                                          bar_window->items_content[i][j]))
                return 0;
            snprintf (option_name, sizeof (option_name),
                      "items_num_lines_%05d_%05d", i + 1, j + 1);
            if (!infolist_new_var_integer (ptr_item, option_name,
                                           bar_window->items_num_lines[i][j]))
                return 0;
        }
    }
    if (!infolist_new_var_integer (ptr_item, "screen_col_size", bar_window->screen_col_size))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "screen_lines", bar_window->screen_lines))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "gui_objects", bar_window->gui_objects))
        return 0;

    return 1;
}

/*
 * Prints bar window infos in WeeChat log file (usually for crash dump).
 */

void
gui_bar_window_print_log (struct t_gui_bar_window *bar_window)
{
    int i, j;

    log_printf ("");
    log_printf ("  [window bar (addr:%p)]", bar_window);
    log_printf ("    bar. . . . . . . . . . : %p ('%s')",
                bar_window->bar,
                (bar_window->bar) ? bar_window->bar->name : "");
    log_printf ("    x. . . . . . . . . . . : %d", bar_window->x);
    log_printf ("    y. . . . . . . . . . . : %d", bar_window->y);
    log_printf ("    width. . . . . . . . . : %d", bar_window->width);
    log_printf ("    height . . . . . . . . : %d", bar_window->height);
    log_printf ("    scroll_x . . . . . . . : %d", bar_window->scroll_x);
    log_printf ("    scroll_y . . . . . . . : %d", bar_window->scroll_y);
    log_printf ("    cursor_x . . . . . . . : %d", bar_window->cursor_x);
    log_printf ("    cursor_y . . . . . . . : %d", bar_window->cursor_y);
    log_printf ("    current_size . . . . . : %d", bar_window->current_size);
    log_printf ("    items_count. . . . . . : %d", bar_window->items_count);
    for (i = 0; i < bar_window->items_count; i++)
    {
        log_printf ("    items_subcount[%03d]. . : %d",
                    i, bar_window->items_subcount[i]);
        if (bar_window->items_content && bar_window->bar
            && bar_window->bar->items_array)
        {
            for (j = 0; j < bar_window->items_subcount[i]; j++)
            {
                log_printf ("    items_content[%03d][%03d]: '%s' "
                            "(item: '%s', num_lines: %d, refresh_needed: %d)",
                            i, j,
                            bar_window->items_content[i][j],
                            (bar_window->items_count >= i + 1) ?
                            bar_window->bar->items_array[i][j] : "?",
                            bar_window->items_num_lines[i][j],
                            bar_window->items_refresh_needed[i][j]);
            }
        }
        else
        {
            log_printf ("    items_content. . . . . . : %p", bar_window->items_content);
        }
    }
    log_printf ("    screen_col_size. . . . : %d", bar_window->screen_col_size);
    log_printf ("    screen_lines . . . . . : %d", bar_window->screen_lines);
    log_printf ("    coords_count . . . . . : %d", bar_window->coords_count);
    for (i = 0; i < bar_window->coords_count; i++)
    {
        log_printf ("    coords[%03d]. . . . . . : item=%d, subitem=%d, "
                    "line=%d, x=%d, y=%d",
                    i,
                    bar_window->coords[i]->item,
                    bar_window->coords[i]->subitem,
                    bar_window->coords[i]->line,
                    bar_window->coords[i]->x,
                    bar_window->coords[i]->y);
    }
    log_printf ("    gui_objects. . . . . . : %p", bar_window->gui_objects);
    gui_bar_window_objects_print_log (bar_window);
    log_printf ("    prev_bar_window. . . . : %p", bar_window->prev_bar_window);
    log_printf ("    next_bar_window. . . . : %p", bar_window->next_bar_window);
}
