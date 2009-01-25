/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
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

/* gui-bar-window.c: bar window functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-infolist.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "gui-bar-window.h"
#include "gui-bar.h"
#include "gui-bar-item.h"
#include "gui-chat.h"
#include "gui-color.h"
#include "gui-window.h"


/*
 * gui_bar_window_valid: check if a bar window pointer exists
 *                       return 1 if bar window exists
 *                              0 if bar window is not found
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
 * gui_bar_window_search_bar: search a reference to a bar in a window
 */

struct t_gui_bar_window *
gui_bar_window_search_bar (struct t_gui_window *window, struct t_gui_bar *bar)
{
    struct t_gui_bar_window *ptr_bar_win;

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
 * gui_bar_window_get_size: get total bar size (window bars) for a position
 *                          bar is optional, if not NULL, size is computed
 *                          from bar 1 to bar # - 1
 */

int
gui_bar_window_get_size (struct t_gui_bar *bar, struct t_gui_window *window,
                         enum t_gui_bar_position position)
{
    struct t_gui_bar_window *ptr_bar_window;
    int total_size;
    
    total_size = 0;
    for (ptr_bar_window = window->bar_windows; ptr_bar_window;
         ptr_bar_window = ptr_bar_window->next_bar_window)
    {
        /* stop before bar */
        if (bar && (ptr_bar_window->bar == bar))
            return total_size;

        if (!CONFIG_BOOLEAN(ptr_bar_window->bar->options[GUI_BAR_OPTION_HIDDEN]))
        {
            if ((CONFIG_INTEGER(ptr_bar_window->bar->options[GUI_BAR_OPTION_TYPE]) != GUI_BAR_TYPE_ROOT)
                && (CONFIG_INTEGER(ptr_bar_window->bar->options[GUI_BAR_OPTION_POSITION]) == (int)position))
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
 * gui_bar_window_calculate_pos_size: calculate position and size of a bar
 */

void
gui_bar_window_calculate_pos_size (struct t_gui_bar_window *bar_window,
                                   struct t_gui_window *window)
{
    int x1, y1, x2, y2;
    int add_bottom, add_top, add_left, add_right;
    
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
    
    switch (CONFIG_INTEGER(bar_window->bar->options[GUI_BAR_OPTION_POSITION]))
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
}

/*
 * gui_bar_window_find_pos: find position for bar window (keeping list sorted
 *                          by bar priority)
 */

struct t_gui_bar_window *
gui_bar_window_find_pos (struct t_gui_bar *bar, struct t_gui_window *window)
{
    struct t_gui_bar_window *ptr_bar_window;
    
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
 * gui_bar_window_content_alloc: allocate content for a bar window
 */

void
gui_bar_window_content_alloc (struct t_gui_bar_window *bar_window)
{
    int i, j;

    bar_window->items_count = bar_window->bar->items_count;
    bar_window->items_subcount = malloc (bar_window->items_count *
                                         sizeof (*bar_window->items_subcount));
    bar_window->items_content = malloc ((bar_window->items_count) *
                                        sizeof (*bar_window->items_content));
    bar_window->items_refresh_needed = malloc (bar_window->items_count *
                                               sizeof (*bar_window->items_refresh_needed));
    if (bar_window->items_content)
    {
        for (i = 0; i < bar_window->items_count; i++)
        {
            bar_window->items_subcount[i] = bar_window->bar->items_subcount[i];
            bar_window->items_content[i] = malloc (bar_window->items_subcount[i] *
                                                   sizeof (**bar_window->items_content));
            bar_window->items_refresh_needed[i] = malloc (bar_window->items_subcount[i] *
                                                          sizeof (**bar_window->items_refresh_needed));
            for (j = 0; j < bar_window->items_subcount[i]; j++)
            {
                if (bar_window->items_content[i])
                    bar_window->items_content[i][j] = NULL;
                if (bar_window->items_refresh_needed[i])
                    bar_window->items_refresh_needed[i][j] = 1;
            }
        }
    }
}

/*
 * gui_bar_window_content_free: free content of a bar window
 */

void
gui_bar_window_content_free (struct t_gui_bar_window *bar_window)
{
    int i, j;
    
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
            free (bar_window->items_refresh_needed[i]);
        }
        free (bar_window->items_content);
        bar_window->items_content = NULL;
        free (bar_window->items_refresh_needed);
        bar_window->items_refresh_needed = NULL;
    }
}

/*
 * gui_bar_window_content_build_item: build content of an item for a bar window,
 *                                    by calling callback for each item, then
 *                                    concat values (according to bar position
 *                                    and filling)
 */

void
gui_bar_window_content_build_item (struct t_gui_bar_window *bar_window,
                                   struct t_gui_window *window,
                                   int index_item, int index_subitem)
{
    if (bar_window->items_content[index_item][index_subitem])
    {
        free (bar_window->items_content[index_item][index_subitem]);
        bar_window->items_content[index_item][index_subitem] = NULL;
    }
    
    /* build item, but only if there's a buffer in window */
    if ((window && window->buffer)
        || (gui_current_window && gui_current_window->buffer))
    {
        bar_window->items_content[index_item][index_subitem] =
            gui_bar_item_get_value (bar_window->bar->items_array[index_item][index_subitem],
                                    bar_window->bar, window);
        bar_window->items_refresh_needed[index_item][index_subitem] = 0;
    }
}

/*
 * gui_bar_window_content_build: build content of a bar window, by calling
 *                               callback for each item, then concat values
 *                               (according to bar position and filling)
 */

void
gui_bar_window_content_build (struct t_gui_bar_window *bar_window,
                              struct t_gui_window *window)
{
    int i, j;
    
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
 * gui_bar_window_content_get: get item or subitem content (first rebuild
 *                             content if refresh is needed)
 */

char *
gui_bar_window_content_get (struct t_gui_bar_window *bar_window,
                            struct t_gui_window *window,
                            int index_item, int index_subitem)
{
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
 * gui_bar_window_content_get_with_filling: get content of a bar window,
 *                                          formated for display, according
 *                                          to filling for bar position
 */

char *
gui_bar_window_content_get_with_filling (struct t_gui_bar_window *bar_window,
                                         struct t_gui_window *window)
{
    enum t_gui_bar_filling filling;
    char *ptr_content, *content, reinit_color[32], reinit_color_space[32];
    char *item_value, ****splitted_items, **linear_items;
    int index_content, content_length, i, sub, j, k, index;
    int length_reinit_color, length_reinit_color_space;
    int length, max_length, max_length_screen, total_items, columns, lines;
    
    snprintf (reinit_color, sizeof (reinit_color),
              "%c%c%02d,%02d",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_FG_BG_CHAR,
              CONFIG_COLOR(bar_window->bar->options[GUI_BAR_OPTION_COLOR_FG]),
              CONFIG_COLOR(bar_window->bar->options[GUI_BAR_OPTION_COLOR_BG]));
    length_reinit_color = strlen (reinit_color);
    
    snprintf (reinit_color_space, sizeof (reinit_color_space),
              "%c%c%02d,%02d ",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_FG_BG_CHAR,
              CONFIG_COLOR(bar_window->bar->options[GUI_BAR_OPTION_COLOR_FG]),
              CONFIG_COLOR(bar_window->bar->options[GUI_BAR_OPTION_COLOR_BG]));
    length_reinit_color_space = strlen (reinit_color_space);
    
    content = NULL;
    content_length = 1;
    filling = gui_bar_get_filling (bar_window->bar);
    switch (filling)
    {
        case GUI_BAR_FILLING_HORIZONTAL: /* items separated by space */
        case GUI_BAR_FILLING_VERTICAL:   /* items separated by \n */
            for (i = 0; i < bar_window->items_count; i++)
            {
                for (sub = 0; sub < bar_window->items_subcount[i]; sub++)
                {
                    ptr_content = gui_bar_window_content_get (bar_window, window,
                                                              i, sub);
                    if (ptr_content && ptr_content[0])
                    {
                        if (gui_bar_get_filling (bar_window->bar) == GUI_BAR_FILLING_HORIZONTAL)
                        {
                            item_value = string_replace (ptr_content, "\n",
                                                         reinit_color_space);
                        }
                        else
                            item_value = NULL;
                        if (!content)
                        {
                            content_length += strlen ((item_value) ?
                                                      item_value : ptr_content);
                            content = strdup ((item_value) ? item_value : ptr_content);
                        }
                        else
                        {
                            content_length += length_reinit_color_space +
                                strlen ((item_value) ? item_value : ptr_content);
                            content = realloc (content, content_length);
                            if (sub == 0)
                            {
                                /* first sub item: insert space after last item */
                                if (gui_bar_get_filling (bar_window->bar) == GUI_BAR_FILLING_HORIZONTAL)
                                    strcat (content, reinit_color_space);
                                else
                                    strcat (content, "\n");
                            }
                            else
                            {
                                strcat (content, reinit_color);
                            }
                            strcat (content,
                                    (item_value) ? item_value : ptr_content);
                        }
                        if (item_value)
                            free (item_value);
                    }
                }
            }
            break;
        case GUI_BAR_FILLING_COLUMNS_HORIZONTAL: /* items in columns, with horizontal filling */
        case GUI_BAR_FILLING_COLUMNS_VERTICAL:   /* items in columns, with vertical filling */
            total_items = 0;
            max_length = 1;
            max_length_screen = 1;
            splitted_items = malloc(bar_window->items_count * sizeof(*splitted_items));
            for (i = 0; i < bar_window->items_count; i++)
            {
                if (bar_window->items_subcount[i] > 0)
                {
                    splitted_items[i] = malloc (bar_window->items_subcount[i] * sizeof (**splitted_items));
                    for (sub = 0; sub < bar_window->items_subcount[i]; sub++)
                    {
                        ptr_content = gui_bar_window_content_get (bar_window, window,
                                                                  i, sub);
                        if (ptr_content && ptr_content[0])
                        {
                            splitted_items[i][sub] = string_explode (ptr_content,
                                                                     "\n", 0, 0, NULL);
                            for (j = 0; splitted_items[i][sub][j]; j++)
                            {
                                total_items++;
                                
                                length = strlen (splitted_items[i][sub][j]);
                                if (length > max_length)
                                    max_length = length;
                                
                                length = gui_chat_strlen_screen (splitted_items[i][sub][j]);
                                if (length > max_length_screen)
                                    max_length_screen = length;
                            }
                        }
                        else
                            splitted_items[i] = NULL;
                    }
                }
                else
                    splitted_items[i] = NULL;
            }
            if ((CONFIG_INTEGER(bar_window->bar->options[GUI_BAR_OPTION_POSITION]) == GUI_BAR_POSITION_BOTTOM)
                || (CONFIG_INTEGER(bar_window->bar->options[GUI_BAR_OPTION_POSITION]) == GUI_BAR_POSITION_TOP))
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
            
            /* build array with pointers to splitted items */
            linear_items = malloc (total_items * sizeof (*linear_items));
            index = 0;
            for (i = 0; i < bar_window->items_count; i++)
            {
                if (splitted_items[i])
                {
                    for (sub = 0; sub < bar_window->items_subcount[i]; sub++)
                    {
                        if (splitted_items[i][sub])
                        {
                            for (j = 0; splitted_items[i][sub][j]; j++)
                            {
                                linear_items[index++] = splitted_items[i][sub][j];
                            }
                        }
                    }
                }
            }
            
            /* build content with lines and columns */
            content = malloc (1 + (lines *
                                   ((columns * (max_length + length_reinit_color_space)) + 1)));
            content[0] = '\0';
            index_content = 0;
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
                            content[index_content++] = ' ';
                        }
                    }
                    else
                    {
                        strcpy (content + index_content, linear_items[index]);
                        index_content += strlen (linear_items[index]);
                        length = max_length_screen -
                            gui_chat_strlen_screen (linear_items[index]);
                        for (k = 0; k < length; k++)
                        {
                            content[index_content++] = ' ';
                        }
                    }
                    if (j < columns - 1)
                    {
                        strcpy (content + index_content, reinit_color_space);
                        index_content += length_reinit_color_space;
                    }
                }
                content[index_content++] = '\n';
            }
            content[index_content] = '\0';
            
            free (linear_items);
            for (i = 0; i < bar_window->items_count; i++)
            {
                if (splitted_items[i])
                {
                    for (sub = 0; sub < bar_window->items_subcount[i]; sub++)
                    {
                        if (splitted_items[i][sub])
                            string_free_exploded (splitted_items[i][sub]);
                    }
                    free (splitted_items[i]);
                }
            }
            free (splitted_items);
            break;
        case GUI_BAR_NUM_FILLING:
            break;
    }
    
    return content;
}

/*
 * gui_bar_window_new: create a new "window bar" for a bar, in screen or a window
 *                     if window is not NULL, bar window will be in this window
 *                     return 1 if ok, 0 if error
 */

int
gui_bar_window_new (struct t_gui_bar *bar, struct t_gui_window *window)
{
    struct t_gui_bar_window *new_bar_window, *pos_bar_window;
    
    if (window)
    {
        if ((CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_WINDOW)
            && (!gui_bar_check_conditions_for_window (bar, window)))
            return 1;
    }
    
    new_bar_window =  malloc (sizeof (*new_bar_window));
    if (new_bar_window)
    {
        new_bar_window->bar = bar;
        if (window)
        {
            bar->bar_window = NULL;
            if (window->bar_windows)
            {
                pos_bar_window = gui_bar_window_find_pos (bar, window);
                if (pos_bar_window)
                {
                    /* insert before bar window found */
                    new_bar_window->prev_bar_window = pos_bar_window->prev_bar_window;
                    new_bar_window->next_bar_window = pos_bar_window;
                    if (pos_bar_window->prev_bar_window)
                        (pos_bar_window->prev_bar_window)->next_bar_window = new_bar_window;
                    else
                        window->bar_windows = new_bar_window;
                    pos_bar_window->prev_bar_window = new_bar_window;
                }
                else
                {
                    /* add to end of list for window */
                    new_bar_window->prev_bar_window = window->last_bar_window;
                    new_bar_window->next_bar_window = NULL;
                    (window->last_bar_window)->next_bar_window = new_bar_window;
                    window->last_bar_window = new_bar_window;
                }
            }
            else
            {
                new_bar_window->prev_bar_window = NULL;
                new_bar_window->next_bar_window = NULL;
                window->bar_windows = new_bar_window;
                window->last_bar_window = new_bar_window;
            }
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
        new_bar_window->items_refresh_needed = NULL;
        gui_bar_window_objects_init (new_bar_window);
        gui_bar_window_content_alloc (new_bar_window);
        
        if (gui_init_ok)
        {
            gui_bar_window_calculate_pos_size (new_bar_window, window);
            gui_bar_window_create_win (new_bar_window);
        }
        
        return 1;
    }
    
    /* failed to create bar window */
    return 0;
}

/*
 * gui_bar_window_get_current_size: get current size of bar window
 *                                  return width or height, depending on bar
 *                                  position
 */

int
gui_bar_window_get_current_size (struct t_gui_bar_window *bar_window)
{
    return bar_window->current_size;
}

/*
 * gui_bar_window_get_max_size_in_window: return max size for bar window
 *                                        in a window
 */

int
gui_bar_window_get_max_size_in_window (struct t_gui_bar_window *bar_window,
                                       struct t_gui_window *window)
{
    int max_size;
    
    max_size = 1;
    
    if (bar_window && window)
    {
        switch (CONFIG_INTEGER(bar_window->bar->options[GUI_BAR_OPTION_POSITION]))
        {
            case GUI_BAR_POSITION_BOTTOM:
            case GUI_BAR_POSITION_TOP:
                max_size = (window->win_chat_height + bar_window->height) -
                    GUI_WINDOW_CHAT_MIN_HEIGHT;
                break;
            case GUI_BAR_POSITION_LEFT:
            case GUI_BAR_POSITION_RIGHT:
                max_size = (window->win_chat_width + bar_window->width) -
                    GUI_WINDOW_CHAT_MIN_HEIGHT;
                break;
            case GUI_BAR_NUM_POSITIONS:
                break;
        }
    }
    
    return max_size;
}

/*
 * gui_bar_window_get_max_size: return max size for bar window
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
            if (max_size < max_size_found)
                max_size_found = max_size;
        }
        if (max_size_found == INT_MAX)
            max_size_found = 1;
    }
    
    return max_size_found;
}

/*
 * gui_bar_window_set_current_size: set current size of all bar windows for a bar
 */

void
gui_bar_window_set_current_size (struct t_gui_bar_window *bar_window,
                                 struct t_gui_window *window, int size)
{
    int new_size, max_size;
    
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
        new_size = (max_size < new_size) ? max_size : new_size;
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
                    gui_window_refresh_needed = 1;
            }
        }
    }
}

/*
 * gui_bar_window_free: free a bar window
 */

void
gui_bar_window_free (struct t_gui_bar_window *bar_window,
                     struct t_gui_window *window)
{
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
    
    /* free data */
    gui_bar_window_content_free (bar_window);
    gui_bar_window_objects_free (bar_window);
    free (bar_window->gui_objects);
    
    free (bar_window);
}

/*
 * gui_bar_window_remove_unused_bars: remove unused bars for a window
 *                                    return 1 if at least one bar was removed
 *                                           0 if no bar was removed
 */

int
gui_bar_window_remove_unused_bars (struct t_gui_window *window)
{
    int rc;
    struct t_gui_bar_window *ptr_bar_win, *next_bar_win;
    
    rc = 0;

    ptr_bar_win = window->bar_windows;
    while (ptr_bar_win)
    {
        next_bar_win = ptr_bar_win->next_bar_window;
        
        if ((CONFIG_INTEGER(ptr_bar_win->bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_WINDOW)
            && (!gui_bar_check_conditions_for_window (ptr_bar_win->bar, window)))
        {
            gui_bar_window_free (ptr_bar_win, window);
            rc = 1;
        }
        
        ptr_bar_win = next_bar_win;
    }
    
    return rc;
}

/*
 * gui_bar_window_add_missing_bars: add missing bars for a window
 *                                  return 1 if at least one bar was created
 *                                         0 if no bar was created
 */

int
gui_bar_window_add_missing_bars (struct t_gui_window *window)
{
    int rc;
    struct t_gui_bar *ptr_bar;
    
    rc = 0;
    
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if ((CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_WINDOW)
            && gui_bar_check_conditions_for_window (ptr_bar, window))
        {
            if (!gui_bar_window_search_bar (window, ptr_bar))
            {
                gui_bar_window_new (ptr_bar, window);
                rc = 1;
            }
        }
    }
    
    return rc;
}

/*
 * gui_bar_window_scroll: scroll a bar window with a value
 *                        if add == 1, then value is added (otherwise subtracted)
 *                        if add_x == 1, then value is added to scroll_x (otherwise scroll_y)
 *                        if percent == 1, then value is a percentage (otherwise number of chars)
 */

void
gui_bar_window_scroll (struct t_gui_bar_window *bar_window,
                       struct t_gui_window *window,
                       int add_x, int scroll_beginning, int scroll_end,
                       int add, int percent, int value)
{
    int old_scroll_x, old_scroll_y;
    
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
 * gui_bar_window_add_to_infolist: add a bar window in an infolist
 *                                 return 1 if ok, 0 if error
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
        }
    }
    if (!infolist_new_var_pointer (ptr_item, "gui_objects", bar_window->gui_objects))
        return 0;
    
    return 1;
}

/*
 * gui_bar_window_print_log: print bar window infos in log (usually for crash dump)
 */

void
gui_bar_window_print_log (struct t_gui_bar_window *bar_window)
{
    int i, j;
    
    log_printf ("");
    log_printf ("  [window bar (addr:0x%lx)]",     bar_window);
    log_printf ("    bar. . . . . . . . . : 0x%lx ('%s')",
                bar_window->bar,
                (bar_window->bar) ? bar_window->bar->name : "");
    log_printf ("    x. . . . . . . . . . . : %d",    bar_window->x);
    log_printf ("    y. . . . . . . . . . . : %d",    bar_window->y);
    log_printf ("    width. . . . . . . . . : %d",    bar_window->width);
    log_printf ("    height . . . . . . . . : %d",    bar_window->height);
    log_printf ("    scroll_x . . . . . . . : %d",    bar_window->scroll_x);
    log_printf ("    scroll_y . . . . . . . : %d",    bar_window->scroll_y);
    log_printf ("    cursor_x . . . . . . . : %d",    bar_window->cursor_x);
    log_printf ("    cursor_y . . . . . . . : %d",    bar_window->cursor_y);
    log_printf ("    current_size . . . . . : %d",    bar_window->current_size);
    log_printf ("    items_count. . . . . . : %d",    bar_window->items_count);
    for (i = 0; i < bar_window->items_count; i++)
    {
        log_printf ("    items_subcount[%03d]. . : %d",
                    i, bar_window->items_subcount[i]);
        for (j = 0; j < bar_window->items_subcount[i]; j++)
        {
            log_printf ("    items_content[%03d][%03d]: '%s' (item: '%s')",
                        i, j,
                        bar_window->items_content[i][j],
                        (bar_window->items_count >= i + 1) ?
                        bar_window->bar->items_array[i][j] : "?");
        }
    }
    log_printf ("    gui_objects. . . . . . : 0x%lx", bar_window->gui_objects);
    gui_bar_window_objects_print_log (bar_window);
    log_printf ("    prev_bar_window. . . . : 0x%lx", bar_window->prev_bar_window);
    log_printf ("    next_bar_window. . . . : 0x%lx", bar_window->next_bar_window);
}
