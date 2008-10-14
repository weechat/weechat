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

/* gui-curses-bar.c: bar functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-log.h"
#include "../../core/wee-string.h"
#include "../../core/wee-utf8.h"
#include "../gui-bar.h"
#include "../gui-bar-item.h"
#include "../gui-chat.h"
#include "../gui-color.h"
#include "../gui-window.h"
#include "gui-curses.h"


/*
 * gui_bar_window_search_bar: search a reference to a bar in a window
 */

struct t_gui_bar_window *
gui_bar_window_search_bar (struct t_gui_window *window, struct t_gui_bar *bar)
{
    struct t_gui_bar_window *ptr_bar_win;

    for (ptr_bar_win = GUI_CURSES(window)->bar_windows; ptr_bar_win;
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
    for (ptr_bar_window = GUI_CURSES(window)->bar_windows; ptr_bar_window;
         ptr_bar_window = ptr_bar_window->next_bar_window)
    {
        /* stop before bar */
        if (bar && (ptr_bar_window->bar == bar))
            return total_size;

        if (!CONFIG_BOOLEAN(ptr_bar_window->bar->hidden))
        {
            if ((CONFIG_INTEGER(ptr_bar_window->bar->type) != GUI_BAR_TYPE_ROOT)
                && (CONFIG_INTEGER(ptr_bar_window->bar->position) == (int)position))
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
                if (CONFIG_INTEGER(ptr_bar_window->bar->separator))
                    total_size++;
            }
        }
    }
    return total_size;
}

/*
 * gui_bar_get_min_width: return minimum width of a bar window displayed for
 *                        a bar
 *                        for example, if a bar is displayed in 3 windows,
 *                        this function return min width of these 3 bar windows
 */

int
gui_bar_get_min_width (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win;
    int min_width;
    
    if (CONFIG_INTEGER(bar->type) == GUI_BAR_TYPE_ROOT)
        return bar->bar_window->width;
    
    min_width = INT_MAX;
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        for (ptr_bar_win = GUI_CURSES(ptr_win)->bar_windows;
             ptr_bar_win; ptr_bar_win = ptr_bar_win->next_bar_window)
        {
            if (ptr_bar_win->bar == bar)
            {
                if (ptr_bar_win->width < min_width)
                    min_width = ptr_bar_win->width;
            }
        }
    }
    
    if (min_width == INT_MAX)
        return 0;
    
    return min_width;
}

/*
 * gui_bar_get_min_height: return minimum height of a bar window displayed for
 *                         a bar
 *                         for example, if a bar is displayed in 3 windows,
 *                         this function return min width of these 3 bar windows
 */

int
gui_bar_get_min_height (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win;
    int min_height;
    
    if (CONFIG_INTEGER(bar->type) == GUI_BAR_TYPE_ROOT)
        return bar->bar_window->height;
    
    min_height = INT_MAX;
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        for (ptr_bar_win = GUI_CURSES(ptr_win)->bar_windows;
             ptr_bar_win; ptr_bar_win = ptr_bar_win->next_bar_window)
        {
            if (ptr_bar_win->bar == bar)
            {
                if (ptr_bar_win->height < min_height)
                    min_height = ptr_bar_win->height;
            }
        }
    }
    
    if (min_height == INT_MAX)
        return 0;
    
    return min_height;
}

/*
 * gui_bar_check_size_add: check if "add_size" is ok for bar
 *                         return 1 if new size is ok
 *                                0 if new size is too big
 */

int
gui_bar_check_size_add (struct t_gui_bar *bar, int add_size)
{
    struct t_gui_window *ptr_win;
    int sub_width, sub_height;
    
    sub_width = 0;
    sub_height = 0;
    
    switch (CONFIG_INTEGER(bar->position))
    {
        case GUI_BAR_POSITION_BOTTOM:
        case GUI_BAR_POSITION_TOP:
            sub_height = add_size;
            break;
        case GUI_BAR_POSITION_LEFT:
        case GUI_BAR_POSITION_RIGHT:
            sub_width = add_size;
            break;
        case GUI_BAR_NUM_POSITIONS:
            break;
    }
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if ((CONFIG_INTEGER(bar->type) == GUI_BAR_TYPE_ROOT)
            || (gui_bar_window_search_bar (ptr_win, bar)))
        {
            if ((ptr_win->win_chat_width - sub_width < GUI_WINDOW_CHAT_MIN_WIDTH)
                || (ptr_win->win_chat_height - sub_height < GUI_WINDOW_CHAT_MIN_HEIGHT))
                return 0;
        }
    }
    
    /* new size ok */
    return 1;
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
    
    switch (CONFIG_INTEGER(bar_window->bar->position))
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
 * gui_bar_window_create_win: create curses window for bar
 */

void
gui_bar_window_create_win (struct t_gui_bar_window *bar_window)
{
    if (bar_window->win_bar)
    {
        delwin (bar_window->win_bar);
        bar_window->win_bar = NULL;
    }
    if (bar_window->win_separator)
    {
        delwin (bar_window->win_separator);
        bar_window->win_separator = NULL;
    }
    
    bar_window->win_bar = newwin (bar_window->height,
                                  bar_window->width,
                                  bar_window->y,
                                  bar_window->x);
    
    if (CONFIG_INTEGER(bar_window->bar->separator))
    {
        switch (CONFIG_INTEGER(bar_window->bar->position))
        {
            case GUI_BAR_POSITION_BOTTOM:
                bar_window->win_separator = newwin (1,
                                                    bar_window->width,
                                                    bar_window->y - 1,
                                                    bar_window->x);
                break;
            case GUI_BAR_POSITION_TOP:
                bar_window->win_separator = newwin (1,
                                                    bar_window->width,
                                                    bar_window->y + bar_window->height,
                                                    bar_window->x);
                break;
            case GUI_BAR_POSITION_LEFT:
                bar_window->win_separator = newwin (bar_window->height,
                                                    1,
                                                    bar_window->y,
                                                    bar_window->x + bar_window->width);
                break;
            case GUI_BAR_POSITION_RIGHT:
                bar_window->win_separator = newwin (bar_window->height,
                                                    1,
                                                    bar_window->y,
                                                    bar_window->x - 1);
                break;
            case GUI_BAR_NUM_POSITIONS:
                break;
        }
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
    
    for (ptr_bar_window = GUI_CURSES(window)->bar_windows; ptr_bar_window;
         ptr_bar_window = ptr_bar_window->next_bar_window)
    {
        if (CONFIG_INTEGER(bar->priority) >= CONFIG_INTEGER(ptr_bar_window->bar->priority))
            return ptr_bar_window;
    }
    
    /* position not found, best position is at the end */
    return NULL;
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
        if ((CONFIG_INTEGER(bar->type) == GUI_BAR_TYPE_WINDOW)
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
            if (GUI_CURSES(window)->bar_windows)
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
                        GUI_CURSES(window)->bar_windows = new_bar_window;
                    pos_bar_window->prev_bar_window = new_bar_window;
                }
                else
                {
                    /* add to end of list for window */
                    new_bar_window->prev_bar_window = GUI_CURSES(window)->last_bar_window;
                    new_bar_window->next_bar_window = NULL;
                    (GUI_CURSES(window)->last_bar_window)->next_bar_window = new_bar_window;
                    GUI_CURSES(window)->last_bar_window = new_bar_window;
                }
            }
            else
            {
                new_bar_window->prev_bar_window = NULL;
                new_bar_window->next_bar_window = NULL;
                GUI_CURSES(window)->bar_windows = new_bar_window;
                GUI_CURSES(window)->last_bar_window = new_bar_window;
            }
        }
        else
        {
            bar->bar_window = new_bar_window;
            new_bar_window->prev_bar_window = NULL;
            new_bar_window->next_bar_window = NULL;
        }
        new_bar_window->win_bar = NULL;
        new_bar_window->win_separator = NULL;
        
        new_bar_window->x = 0;
        new_bar_window->y = 0;
        new_bar_window->width = 1;
        new_bar_window->height = 1;
        new_bar_window->scroll_x = 0;
        new_bar_window->scroll_y = 0;
        new_bar_window->cursor_x = -1;
        new_bar_window->cursor_y = -1;
        new_bar_window->current_size = (CONFIG_INTEGER(bar->size) == 0) ?
            1 : CONFIG_INTEGER(bar->size);
        
        if (gui_init_ok)
        {
            gui_bar_window_calculate_pos_size (new_bar_window, window);
            gui_bar_window_create_win (new_bar_window);
            if (window)
                window->refresh_needed = 1;
        }
        
        return 1;
    }
    
    /* failed to create bar window */
    return 0;
}

/*
 * gui_bar_window_recreate_bar_windows: recreate bar windows for all windows
 */

void
gui_bar_window_recreate_bar_windows (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win;

    if (CONFIG_BOOLEAN(bar->hidden))
        return;
    
    if (CONFIG_INTEGER(bar->type) == GUI_BAR_TYPE_ROOT)
    {
        gui_bar_window_calculate_pos_size (bar->bar_window, NULL);
        gui_bar_window_create_win (bar->bar_window);
        gui_window_refresh_needed = 1;
    }
    else
    {
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            for (ptr_bar_win = GUI_CURSES(ptr_win)->bar_windows;
                 ptr_bar_win; ptr_bar_win = ptr_bar_win->next_bar_window)
            {
                if (ptr_bar_win->bar == bar)
                {
                    gui_bar_window_calculate_pos_size (ptr_bar_win, ptr_win);
                    gui_bar_window_create_win (ptr_bar_win);
                    ptr_win->refresh_needed = 1;
                }
            }
        }
    }
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
 * gui_bar_window_set_current_size: set current size of all bar windows for a bar
 */

void
gui_bar_window_set_current_size (struct t_gui_bar *bar, int size)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win;
    int new_size;
    
    if (size == 0)
        new_size = 1;
    else
    {
        new_size = size;
        if ((size != 0) && (CONFIG_INTEGER(bar->size_max) > 0)
            && (size > CONFIG_INTEGER(bar->size_max)))
        {
            new_size = CONFIG_INTEGER(bar->size_max);
            if (new_size < 1)
                new_size = 1;
        }
    }
    
    if (CONFIG_INTEGER(bar->type) == GUI_BAR_TYPE_ROOT)
    {
        if (bar->bar_window->current_size != new_size)
        {
            bar->bar_window->current_size = new_size;
            gui_bar_window_recreate_bar_windows (bar);
        }
    }
    else
    {
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            for (ptr_bar_win = GUI_CURSES(ptr_win)->bar_windows;
                 ptr_bar_win; ptr_bar_win = ptr_bar_win->next_bar_window)
            {
                if ((ptr_bar_win->bar == bar)
                    && (ptr_bar_win->current_size != new_size))
                {
                    ptr_bar_win->current_size = new_size;
                    gui_bar_window_recreate_bar_windows (bar);
                }
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
        if (GUI_CURSES(window)->bar_windows == bar_window)
            GUI_CURSES(window)->bar_windows = bar_window->next_bar_window;
        if (GUI_CURSES(window)->last_bar_window == bar_window)
            GUI_CURSES(window)->last_bar_window = bar_window->prev_bar_window;
        
        window->refresh_needed = 1;
    }
    
    /* free data */
    if (bar_window->win_bar)
        delwin (bar_window->win_bar);
    if (bar_window->win_separator)
        delwin (bar_window->win_separator);
    
    free (bar_window);
}

/*
 * gui_bar_free_bar_windows: free bar windows for a bar
 */

void
gui_bar_free_bar_windows (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win, *next_bar_win;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        ptr_bar_win = GUI_CURSES(ptr_win)->bar_windows;
        while (ptr_bar_win)
        {
            next_bar_win = ptr_bar_win->next_bar_window;
            
            if (ptr_bar_win->bar == bar)
                gui_bar_window_free (ptr_bar_win, ptr_win);
            
            ptr_bar_win = next_bar_win;
        }
    }
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

    ptr_bar_win = GUI_CURSES(window)->bar_windows;
    while (ptr_bar_win)
    {
        next_bar_win = ptr_bar_win->next_bar_window;
        
        if ((CONFIG_INTEGER(ptr_bar_win->bar->type) == GUI_BAR_TYPE_WINDOW)
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
        if ((CONFIG_INTEGER(ptr_bar->type) == GUI_BAR_TYPE_WINDOW)
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
 * gui_bar_window_print_string: print a string text on a bar window
 *                              return 1 if all was printed, 0 if some text
 *                              was not displayed (wrapped due to bar window
 *                              width)
 */

int
gui_bar_window_print_string (struct t_gui_bar_window *bar_window,
                             int *x, int *y,
                             const char *string,
                             int reset_color_before_display)
{
    int weechat_color, x_with_hidden, size_on_screen, fg, bg;
    char str_fg[3], str_bg[3], utf_char[16], *next_char, *output;
    
    if (!string || !string[0])
        return 1;
    
    wmove (bar_window->win_bar, *y, *x);

    if (reset_color_before_display)
    {
        gui_window_set_custom_color_fg_bg (bar_window->win_bar,
                                           CONFIG_COLOR(bar_window->bar->color_fg),
                                           CONFIG_COLOR(bar_window->bar->color_bg));
    }
    
    x_with_hidden = *x;
    
    while (string && string[0])
    {
        if (string[0] == GUI_COLOR_COLOR_CHAR)
        {
            string++;
            switch (string[0])
            {
                case GUI_COLOR_FG_CHAR: /* fg color */
                    if (string[1] && string[2])
                    {
                        str_fg[0] = string[1];
                        str_fg[1] = string[2];
                        str_fg[2] = '\0';
                        sscanf (str_fg, "%d", &fg);
                        gui_window_set_custom_color_fg (bar_window->win_bar,
                                                        fg);
                        string += 3;
                    }
                    break;
                case GUI_COLOR_BG_CHAR: /* bg color */
                    if (string[1] && string[2])
                    {
                        str_bg[0] = string[1];
                        str_bg[1] = string[2];
                        str_bg[2] = '\0';
                        sscanf (str_bg, "%d", &bg);
                        gui_window_set_custom_color_bg (bar_window->win_bar,
                                                        bg);
                        string += 3;
                    }
                    break;
                case GUI_COLOR_FG_BG_CHAR: /* fg + bg color */
                    if (string[1] && string[2] && (string[3] == ',')
                        && string[4] && string[5])
                    {
                        str_fg[0] = string[1];
                        str_fg[1] = string[2];
                        str_fg[2] = '\0';
                        str_bg[0] = string[4];
                        str_bg[1] = string[5];
                        str_bg[2] = '\0';
                        sscanf (str_fg, "%d", &fg);
                        sscanf (str_bg, "%d", &bg);
                        gui_window_set_custom_color_fg_bg (bar_window->win_bar,
                                                           fg, bg);
                        string += 6;
                    }
                    break;
                case GUI_COLOR_BAR_CHAR: /* bar color */
                    switch (string[1])
                    {
                        case GUI_COLOR_BAR_FG_CHAR:
                            /* bar foreground */
                            gui_window_set_custom_color_fg (bar_window->win_bar,
                                                            CONFIG_INTEGER(bar_window->bar->color_fg));
                            string += 2;
                            break;
                        case GUI_COLOR_BAR_DELIM_CHAR:
                            /* bar delimiter */
                            gui_window_set_custom_color_fg (bar_window->win_bar,
                                                            CONFIG_INTEGER(bar_window->bar->color_delim));
                            string += 2;
                            break;
                        case GUI_COLOR_BAR_BG_CHAR:
                            /* bar background */
                            gui_window_set_custom_color_bg (bar_window->win_bar,
                                                            CONFIG_INTEGER(bar_window->bar->color_bg));
                            string += 2;
                            break;
                        case GUI_COLOR_BAR_MOVE_CURSOR_CHAR:
                            /* move cursor to current position on screen */
                            getyx (bar_window->win_bar,
                                   bar_window->cursor_y, bar_window->cursor_x);
                            bar_window->cursor_x += bar_window->x;
                            bar_window->cursor_y += bar_window->y;
                            string += 2;
                            break;
                        default:
                            string++;
                            break;
                    }
                    break;
                default:
                    if (isdigit (string[0]) && isdigit (string[1]))
                    {
                        str_fg[0] = string[0];
                        str_fg[1] = string[1];
                        str_fg[2] = '\0';
                        sscanf (str_fg, "%d", &weechat_color);
                        gui_window_set_weechat_color (bar_window->win_bar,
                                                      weechat_color);
                        string += 2;
                    }
                    break;
            }
        }
        else
        {
            next_char = utf8_next_char (string);
            if (!next_char)
                break;
            
            memcpy (utf_char, string, next_char - string);
            utf_char[next_char - string] = '\0';
            
            if (!gui_window_utf_char_valid (utf_char))
                snprintf (utf_char, sizeof (utf_char), ".");
            
            size_on_screen = utf8_char_size_screen (utf_char);
            if (size_on_screen > 0)
            {
                if (x_with_hidden < bar_window->scroll_x)
                {
                    /* hidden char (before scroll_x value) */
                    x_with_hidden++;
                }
                else
                {
                    if (*x + size_on_screen > bar_window->width)
                    {
                        if (CONFIG_INTEGER(gui_bar_get_option_filling (bar_window->bar)) == GUI_BAR_FILLING_VERTICAL)
                            return 0;
                        if (*y >= bar_window->height - 1)
                            return 0;
                        *x = 0;
                        (*y)++;
                        wmove (bar_window->win_bar, *y, *x);
                    }
                    
                    output = string_iconv_from_internal (NULL, utf_char);
                    wprintw (bar_window->win_bar, "%s",
                             (output) ? output : utf_char);
                    if (output)
                        free (output);
                    
                    *x += size_on_screen;
                }
            }
            
            string = next_char;
        }
    }
    return 1;
}

/*
 * gui_bar_window_draw: draw a bar for a window
 */

void
gui_bar_window_draw (struct t_gui_bar_window *bar_window,
                     struct t_gui_window *window)
{
    int x, y, i, items_count, num_lines, line;
    char *content, *item_value, *item_value2, **items;
    char space_with_reinit_color[32];
    int length_reinit_color, content_length, length, length_on_screen;
    int max_length, optimal_number_of_lines, chars_available;
    int some_data_not_displayed;
    
    if (!gui_init_ok)
        return;
    
    snprintf (space_with_reinit_color,
              sizeof (space_with_reinit_color),
              "%c%c%02d,%02d ",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_FG_BG_CHAR,
              CONFIG_COLOR(bar_window->bar->color_fg),
              CONFIG_COLOR(bar_window->bar->color_bg));
    length_reinit_color = strlen (space_with_reinit_color);
    
    /* these values will be overwritten later (by gui_bar_window_print_string)
       if cursor has to move somewhere in bar window */
    bar_window->cursor_x = -1;
    bar_window->cursor_y = -1;
    
    if (CONFIG_INTEGER(bar_window->bar->size) == 0)
    {
        content = NULL;
        content_length = 1;
        for (i = 0; i < bar_window->bar->items_count; i++)
        {
            item_value = gui_bar_item_get_value (bar_window->bar->items_array[i],
                                                 bar_window->bar, window,
                                                 0, 0, 0);
            if (item_value)
            {
                if (item_value[0])
                {
                    if (CONFIG_INTEGER(gui_bar_get_option_filling (bar_window->bar)) == GUI_BAR_FILLING_HORIZONTAL)
                    {
                        item_value2 = string_replace (item_value, "\n",
                                                      space_with_reinit_color);
                    }
                    else
                        item_value2 = NULL;
                    if (!content)
                    {
                        content_length += strlen ((item_value2) ?
                                                  item_value2 : item_value);
                        content = strdup ((item_value2) ?
                                          item_value2 : item_value);
                    }
                    else
                    {
                        content_length += length_reinit_color +
                            strlen ((item_value2) ? item_value2 : item_value);
                        content = realloc (content, content_length);
                        if (CONFIG_INTEGER(gui_bar_get_option_filling (bar_window->bar)) == GUI_BAR_FILLING_HORIZONTAL)
                            strcat (content, space_with_reinit_color);
                        else
                            strcat (content, "\n");
                        strcat (content,
                                (item_value2) ? item_value2 : item_value);
                    }
                    if (item_value2)
                        free (item_value2);
                }
                free (item_value);
            }
        }
        if (content)
        {
            items = string_explode (content, "\n", 0, 0, &items_count);
            if (items_count == 0)
            {
                gui_bar_window_set_current_size (bar_window->bar, 1);
                gui_window_clear (bar_window->win_bar,
                                  CONFIG_COLOR(bar_window->bar->color_bg));
            }
            else
            {
                /* search longer line and optimal number of lines */
                max_length = 0;
                optimal_number_of_lines = 0;
                for (line = 0; line < items_count; line++)
                {
                    length_on_screen = gui_chat_strlen_screen (items[line]);
                    
                    length = strlen (items[line]);
                    if ((length >= 3)
                        && (items[line][length - 3] == GUI_COLOR_COLOR_CHAR)
                        && (items[line][length - 2] == GUI_COLOR_BAR_CHAR)
                        && (items[line][length - 1] == GUI_COLOR_BAR_MOVE_CURSOR_CHAR))
                        length_on_screen++;
                    
                    if (length_on_screen > max_length)
                        max_length = length_on_screen;
                    
                    if (length_on_screen % bar_window->width == 0)
                        num_lines = length_on_screen / bar_window->width;
                    else
                        num_lines = (length_on_screen / bar_window->width) + 1;
                    if (num_lines == 0)
                        num_lines = 1;
                    optimal_number_of_lines += num_lines;
                }
                if (max_length == 0)
                    max_length = 1;
                
                switch (CONFIG_INTEGER(bar_window->bar->position))
                {
                    case GUI_BAR_POSITION_BOTTOM:
                    case GUI_BAR_POSITION_TOP:
                        if (CONFIG_INTEGER(gui_bar_get_option_filling (bar_window->bar)) == GUI_BAR_FILLING_HORIZONTAL)
                            num_lines = optimal_number_of_lines;
                        else
                            num_lines = items_count;
                        gui_bar_window_set_current_size (bar_window->bar,
                                                         num_lines);
                        break;
                    case GUI_BAR_POSITION_LEFT:
                    case GUI_BAR_POSITION_RIGHT:
                        gui_bar_window_set_current_size (bar_window->bar,
                                                         max_length);
                        break;
                    case GUI_BAR_NUM_POSITIONS:
                        break;
                }
                gui_window_clear (bar_window->win_bar,
                                  CONFIG_COLOR(bar_window->bar->color_bg));
                x = 0;
                y = 0;
                some_data_not_displayed = 0;
                if ((bar_window->scroll_y > 0)
                    && (bar_window->scroll_y >= items_count))
                {
                    bar_window->scroll_y = items_count - bar_window->height;
                    if (bar_window->scroll_y < 0)
                        bar_window->scroll_y = 0;
                }
                for (line = 0;
                     (line < items_count) && (y < bar_window->height);
                     line++)
                {
                    if ((bar_window->scroll_y == 0)
                        || (line >= bar_window->scroll_y))
                    {
                        if (!gui_bar_window_print_string (bar_window, &x, &y,
                                                          items[line], 1))
                        {
                            some_data_not_displayed = 1;
                        }
                        if (CONFIG_INTEGER(gui_bar_get_option_filling (bar_window->bar)) == GUI_BAR_FILLING_VERTICAL)
                        {
                            while (x < bar_window->width)
                            {
                                gui_bar_window_print_string (bar_window,
                                                             &x, &y,
                                                             " ", 0);
                            }
                            x = 0;
                            y++;
                        }
                        else
                        {
                            gui_bar_window_print_string (bar_window, &x, &y,
                                                         space_with_reinit_color, 0);
                        }
                    }
                }
                if ((bar_window->cursor_x < 0) && (bar_window->cursor_y < 0)
                    && ((bar_window->scroll_x > 0) || (bar_window->scroll_y > 0)))
                {
                    x = (bar_window->height > 1) ? bar_window->width - 2 : 0;
                    if (x < 0)
                        x = 0;
                    y = 0;
                    gui_window_set_custom_color_fg_bg (bar_window->win_bar,
                                                       CONFIG_COLOR(config_color_bar_more),
                                                       CONFIG_INTEGER(bar_window->bar->color_bg));
                    mvwprintw (bar_window->win_bar, y, x, "--");
                }
                if ((bar_window->cursor_x < 0) && (bar_window->cursor_y < 0)
                    && (some_data_not_displayed || (line < items_count)))
                {
                    x = bar_window->width - 2;
                    if (x < 0)
                        x = 0;
                    y = (bar_window->height > 1) ? bar_window->height - 1 : 0;
                    gui_window_set_custom_color_fg_bg (bar_window->win_bar,
                                                       CONFIG_COLOR(config_color_bar_more),
                                                       CONFIG_INTEGER(bar_window->bar->color_bg));
                    mvwprintw (bar_window->win_bar, y, x, "++");
                }
            }
            if (items)
                string_free_exploded (items);
            free (content);
        }
        else
        {
            gui_bar_window_set_current_size (bar_window->bar, 1);
            gui_window_clear (bar_window->win_bar,
                              CONFIG_COLOR(bar_window->bar->color_bg));
        }
    }
    else
    {
        gui_window_clear (bar_window->win_bar,
                          CONFIG_COLOR(bar_window->bar->color_bg));
        
        x = 0;
        y = 0;
        
        for (i = 0; i < bar_window->bar->items_count; i++)
        {
            chars_available =
                ((bar_window->height - y - 1) * bar_window->width) + /* next lines */
                (bar_window->width - x - 1); /* chars on current line */
            
            item_value = gui_bar_item_get_value (bar_window->bar->items_array[i],
                                                 bar_window->bar, window,
                                                 bar_window->width,
                                                 bar_window->height,
                                                 chars_available);
            if (item_value)
            {
                if (item_value[0])
                {
                    if (CONFIG_INTEGER(gui_bar_get_option_filling (bar_window->bar)) == GUI_BAR_FILLING_HORIZONTAL)
                    {
                        item_value2 = string_replace (item_value, "\n",
                                                      space_with_reinit_color);
                    }
                    else
                        item_value2 = NULL;
                    items = string_explode ((item_value2) ?
                                            item_value2 : item_value,
                                            "\n", 0, 0,
                                            &items_count);
                    some_data_not_displayed = 0;
                    if ((bar_window->scroll_y > 0)
                        && (bar_window->scroll_y >= items_count))
                    {
                        bar_window->scroll_y = items_count - bar_window->height;
                        if (bar_window->scroll_y < 0)
                            bar_window->scroll_y = 0;
                    }
                    for (line = 0;
                         (line < items_count) && (y < bar_window->height);
                         line++)
                    {
                        if ((bar_window->scroll_y == 0)
                            || (line >= bar_window->scroll_y))
                        {
                            if (!gui_bar_window_print_string (bar_window, &x, &y,
                                                              items[line], 1))
                            {
                                some_data_not_displayed = 1;
                            }
                            if (CONFIG_INTEGER(gui_bar_get_option_filling (bar_window->bar)) == GUI_BAR_FILLING_VERTICAL)
                            {
                                while (x < bar_window->width)
                                {
                                    gui_bar_window_print_string (bar_window,
                                                                 &x, &y,
                                                                 " ", 0);
                                }
                                x = 0;
                                y++;
                            }
                            else
                            {
                                gui_bar_window_print_string (bar_window, &x, &y,
                                                             space_with_reinit_color, 0);
                            }
                        }
                    }
                    if ((bar_window->cursor_x < 0) && (bar_window->cursor_y < 0)
                        && ((bar_window->scroll_x > 0) || (bar_window->scroll_y > 0)))
                    {
                        x = (bar_window->height > 1) ? bar_window->width - 2 : 0;
                        if (x < 0)
                            x = 0;
                        y = 0;
                        gui_window_set_custom_color_fg_bg (bar_window->win_bar,
                                                           CONFIG_COLOR(config_color_bar_more),
                                                           CONFIG_INTEGER(bar_window->bar->color_bg));
                        mvwprintw (bar_window->win_bar, y, x, "--");
                    }
                    if ((bar_window->cursor_x < 0) && (bar_window->cursor_y < 0)
                        && (some_data_not_displayed || (line < items_count)))
                    {
                        x = bar_window->width - 2;
                        if (x < 0)
                            x = 0;
                        y = (bar_window->height > 1) ? bar_window->height - 1 : 0;
                        gui_window_set_custom_color_fg_bg (bar_window->win_bar,
                                                           CONFIG_COLOR(config_color_bar_more),
                                                           CONFIG_INTEGER(bar_window->bar->color_bg));
                        mvwprintw (bar_window->win_bar, y, x, "++");
                    }
                    if (item_value2)
                        free (item_value2);
                    if (items)
                        string_free_exploded (items);
                }
                free (item_value);
            }
        }
    }

    /* move cursor if it was asked in an item content (input_text does that
       to move cursor in user input text) */
    if (window && (gui_current_window == window)
        && (bar_window->cursor_x >= 0) && (bar_window->cursor_y >= 0))
    {
        move (bar_window->cursor_y, bar_window->cursor_x);
    }
    
    wnoutrefresh (bar_window->win_bar);
    
    if (CONFIG_INTEGER(bar_window->bar->separator))
    {
        switch (CONFIG_INTEGER(bar_window->bar->position))
        {
            case GUI_BAR_POSITION_BOTTOM:
                gui_window_set_weechat_color (bar_window->win_separator,
                                              GUI_COLOR_SEPARATOR);
                mvwhline (bar_window->win_separator, 0, 0, ACS_HLINE,
                          bar_window->width);
                break;
            case GUI_BAR_POSITION_TOP:
                gui_window_set_weechat_color (bar_window->win_separator,
                                              GUI_COLOR_SEPARATOR);
                mvwhline (bar_window->win_separator, 0, 0, ACS_HLINE,
                          bar_window->width);
                break;
            case GUI_BAR_POSITION_LEFT:
                gui_window_set_weechat_color (bar_window->win_separator,
                                              GUI_COLOR_SEPARATOR);
                mvwvline (bar_window->win_separator, 0, 0, ACS_VLINE,
                          bar_window->height);
                break;
            case GUI_BAR_POSITION_RIGHT:
                gui_window_set_weechat_color (bar_window->win_separator,
                                              GUI_COLOR_SEPARATOR);
                mvwvline (bar_window->win_separator, 0, 0, ACS_VLINE,
                          bar_window->height);
                break;
            case GUI_BAR_NUM_POSITIONS:
                break;
        }
        wnoutrefresh (bar_window->win_separator);
    }
    
    refresh ();
}

/*
 * gui_bar_draw: draw a bar
 */

void
gui_bar_draw (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win;
    
    if (CONFIG_BOOLEAN(bar->hidden))
        return;
    
    if (bar->bar_window)
    {
        /* root bar */
        gui_bar_window_draw (bar->bar_window, NULL);
    }
    else
    {
        /* bar on each window */
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            for (ptr_bar_win = GUI_CURSES(ptr_win)->bar_windows; ptr_bar_win;
                 ptr_bar_win = ptr_bar_win->next_bar_window)
            {
                if (ptr_bar_win->bar == bar)
                {
                    gui_bar_window_draw (ptr_bar_win, ptr_win);
                }
            }
        }
    }
    
    bar->bar_refresh_needed = 0;
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
 * gui_bar_scroll: scroll a bar for a buffer
 *                 return 1 if scroll is ok, 0 if error
 */

int
gui_bar_scroll (struct t_gui_bar *bar, struct t_gui_buffer *buffer,
                const char *scroll)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win;
    long number;
    char *str, *error;
    int length, add_x, add, percent, scroll_beginning, scroll_end;
    
    add_x = 0;
    str = NULL;
    number = 0;
    add = 0;
    percent = 0;
    scroll_beginning = 0;
    scroll_end = 0;
    
    if ((scroll[0] == 'x') || (scroll[0] == 'X'))
    {
        add_x = 1;
        scroll++;
    }
    else if ((scroll[0] == 'y') || (scroll[0] == 'Y'))
    {
        scroll++;
    }
    else
        return 0;
    
    if ((scroll[0] == 'b') || (scroll[0] == 'B'))
    {
        scroll_beginning = 1;
    }
    else if ((scroll[0] == 'e') || (scroll[0] == 'E'))
    {
        scroll_end = 1;
    }
    else
    {
        if (scroll[0] == '+')
        {
            add = 1;
            scroll++;
        }
        else if (scroll[0] == '-')
        {
            scroll++;
        }
        else
            return 0;
        
        length = strlen (scroll);
        if (length == 0)
            return 0;
    
        if (scroll[length - 1] == '%')
        {
            str = string_strndup (scroll, length - 1);
            percent = 1;
        }
        else
            str = strdup (scroll);
        if (!str)
            return 0;
        
        error = NULL;
        number = strtol (str, &error, 10);
        
        if (!error || error[0] || (number <= 0))
        {
            free (str);
            return 0;
        }
    }
    
    if (CONFIG_INTEGER(bar->type) == GUI_BAR_TYPE_ROOT)
        gui_bar_window_scroll (bar->bar_window, NULL,
                               add_x, scroll_beginning, scroll_end,
                               add, percent, number);
    else
    {
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            if (ptr_win->buffer == buffer)
            {
                for (ptr_bar_win = GUI_CURSES(ptr_win)->bar_windows;
                     ptr_bar_win; ptr_bar_win = ptr_bar_win->next_bar_window)
                {
                    if (ptr_bar_win->bar == bar)
                    {
                        gui_bar_window_scroll (ptr_bar_win, ptr_win,
                                               add_x, scroll_beginning, scroll_end,
                                               add, percent, number);
                    }
                }
            }
        }
    }
    
    free (str);
    
    return 1;
}

/*
 * gui_bar_window_print_log: print bar window infos in log (usually for crash dump)
 */

void
gui_bar_window_print_log (struct t_gui_bar_window *bar_window)
{
    log_printf ("");
    log_printf ("  [window bar (addr:0x%x)]",   bar_window);
    log_printf ("    bar . . . . . . . : 0x%x ('%s')",
                bar_window->bar,
                (bar_window->bar) ? bar_window->bar->name : "");
    log_printf ("    x . . . . . . . . : %d",   bar_window->x);
    log_printf ("    y . . . . . . . . : %d",   bar_window->y);
    log_printf ("    width . . . . . . : %d",   bar_window->width);
    log_printf ("    height. . . . . . : %d",   bar_window->height);
    log_printf ("    scroll_x. . . . . : %d",   bar_window->scroll_x);
    log_printf ("    scroll_y. . . . . : %d",   bar_window->scroll_y);
    log_printf ("    cursor_x. . . . . : %d",   bar_window->cursor_x);
    log_printf ("    cursor_y. . . . . : %d",   bar_window->cursor_y);
    log_printf ("    current_size. . . : %d",   bar_window->current_size);
    log_printf ("    win_bar . . . . . : 0x%x", bar_window->win_bar);
    log_printf ("    win_separator . . : 0x%x", bar_window->win_separator);
    log_printf ("    prev_bar_window . : 0x%x", bar_window->prev_bar_window);
    log_printf ("    next_bar_window . : 0x%x", bar_window->next_bar_window);
}
