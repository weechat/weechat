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

#include "../../core/weechat.h"
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
        
        if ((ptr_bar_window->bar->type != GUI_BAR_TYPE_ROOT)
            && (ptr_bar_window->bar->position == position))
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
                    /* make C compiler happy */
                    break;
            }
            if (ptr_bar_window->bar->separator)
                total_size++;
        }
    }
    return total_size;
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
    
    switch (bar->position)
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
        if ((bar->type == GUI_BAR_TYPE_ROOT)
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
        add_left = gui_bar_window_get_size (bar_window->bar, window, GUI_BAR_POSITION_LEFT);
        add_right = gui_bar_window_get_size (bar_window->bar, window, GUI_BAR_POSITION_RIGHT);
        add_top = gui_bar_window_get_size (bar_window->bar, window, GUI_BAR_POSITION_TOP);
        add_bottom = gui_bar_window_get_size (bar_window->bar, window, GUI_BAR_POSITION_BOTTOM);
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
    
    switch (bar_window->bar->position)
    {
        case GUI_BAR_POSITION_BOTTOM:
            bar_window->x = x1 + add_left;
            bar_window->y = y2 - add_bottom - bar_window->bar->current_size + 1;
            bar_window->width = x2 - x1 + 1 - add_left - add_right;
            bar_window->height = bar_window->bar->current_size;
            break;
        case GUI_BAR_POSITION_TOP:
            bar_window->x = x1 + add_left;
            bar_window->y = y1 + add_top;
            bar_window->width = x2 - x1 + 1 - add_left - add_right;
            bar_window->height = bar_window->bar->current_size;
            break;
        case GUI_BAR_POSITION_LEFT:
            bar_window->x = x1 + add_left;
            bar_window->y = y1 + add_top;
            bar_window->width = bar_window->bar->current_size;
            bar_window->height = y2 - add_top - add_bottom - y1 + 1;
            break;
        case GUI_BAR_POSITION_RIGHT:
            bar_window->x = x2 - add_right - bar_window->bar->current_size + 1;
            bar_window->y = y1 + add_top;
            bar_window->width = bar_window->bar->current_size;
            bar_window->height = y2 - y1 + 1;
            break;
        case GUI_BAR_NUM_POSITIONS:
            /* make C compiler happy */
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
    
    if (bar_window->bar->separator)
    {
        switch (bar_window->bar->position)
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
                /* make C compiler happy */
                break;
        }
    }
}

/*
 * gui_bar_window_find_pos: find position for bar window (keeping list sorted
 *                          by bar number)
 */

struct t_gui_bar_window *
gui_bar_window_find_pos (struct t_gui_bar *bar, struct t_gui_window *window)
{
    struct t_gui_bar_window *ptr_bar_window;
    
    for (ptr_bar_window = GUI_CURSES(window)->bar_windows; ptr_bar_window;
         ptr_bar_window = ptr_bar_window->next_bar_window)
    {
        if (ptr_bar_window->bar->number > bar->number)
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
        /* bar is type "window_active" and window is not active */
        if ((bar->type == GUI_BAR_TYPE_WINDOW_ACTIVE)
            && gui_current_window
            && (gui_current_window != window))
            return 1;
        
        /* bar is type "window_inactive" and window is active */
        if ((bar->type == GUI_BAR_TYPE_WINDOW_INACTIVE)
            && (!gui_current_window || (gui_current_window == window)))
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
        
        if (((ptr_bar_win->bar->type == GUI_BAR_TYPE_WINDOW_ACTIVE)
             && (window != gui_current_window))
            || ((ptr_bar_win->bar->type == GUI_BAR_TYPE_WINDOW_INACTIVE)
                && (window == gui_current_window)))
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
        if (((ptr_bar->type == GUI_BAR_TYPE_WINDOW_ACTIVE)
             && (window == gui_current_window))
            || ((ptr_bar->type == GUI_BAR_TYPE_WINDOW_INACTIVE)
                && (window != gui_current_window)))
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
 *                              return number of chars displayed on screen
 */

int
gui_bar_window_print_string (struct t_gui_bar_window *bar_window,
                             char *string, int max_chars_on_screen)
{
    int weechat_color, chars_displayed, size_on_screen;
    char str_color[3], utf_char[16], *next_char, *output;
    
    if (!string || !string[0])
        return 0;
    
    if ((bar_window->bar->position == GUI_BAR_POSITION_LEFT)
        || (bar_window->bar->position == GUI_BAR_POSITION_RIGHT))
        gui_window_set_weechat_color (bar_window->win_bar, GUI_COLOR_CHAT);
    else
        gui_window_set_weechat_color (bar_window->win_bar, GUI_COLOR_STATUS);
    
    chars_displayed = 0;
    
    while (string && string[0])
    {
        if (string[0] == GUI_COLOR_COLOR_CHAR)
        {
            string++;
            if (isdigit (string[0]) && isdigit (string[1]))
            {
                str_color[0] = string[0];
                str_color[1] = string[1];
                str_color[2] = '\0';
                string += 2;
                sscanf (str_color, "%d", &weechat_color);
                gui_window_set_weechat_color (bar_window->win_bar, weechat_color);
            }
        }
        else
        {
            next_char = utf8_next_char (string);
            if (!next_char)
                break;
            
            memcpy (utf_char, string, next_char - string);
            utf_char[next_char - string] = '\0';
            
            if (gui_window_utf_char_valid (utf_char))
            {
                size_on_screen = utf8_char_size_screen (utf_char);
                if (chars_displayed + size_on_screen > max_chars_on_screen)
                    return chars_displayed;
                chars_displayed += size_on_screen;
                output = string_iconv_from_internal (NULL, utf_char);
                wprintw (bar_window->win_bar, "%s",
                         (output) ? output : utf_char);
                if (output)
                    free (output);
            }
            else
            {
                if (chars_displayed + 1 > max_chars_on_screen)
                    return chars_displayed;
                chars_displayed++;
                wprintw (bar_window->win_bar,  ".");
            }
            
            string = next_char;
        }
    }
    
    return chars_displayed;
}

/*
 * gui_bar_window_draw: draw a bar for a window
 */

void
gui_bar_window_draw (struct t_gui_bar_window *bar_window,
                     struct t_gui_window *window)
{
    int x, y, i, max_width, max_height, items_count, num_lines, line;
    char *item_value, *item_value2, **items;
    struct t_gui_bar_item *ptr_item;
    
    if ((bar_window->bar->position == GUI_BAR_POSITION_LEFT)
        || (bar_window->bar->position == GUI_BAR_POSITION_RIGHT))
        gui_window_curses_clear (bar_window->win_bar, GUI_COLOR_CHAT);
    else
        gui_window_curses_clear (bar_window->win_bar, GUI_COLOR_STATUS);
    
    max_width = bar_window->width;
    max_height = bar_window->height;
    
    x = 0;
    y = 0;
    
    /* for each item of bar */
    for (i = 0; i < bar_window->bar->items_count; i++)
    {
        ptr_item = gui_bar_item_search (bar_window->bar->items_array[i]);
        if (ptr_item && ptr_item->build_callback)
        {
            item_value = (ptr_item->build_callback) (ptr_item->build_callback_data,
                                                     ptr_item, window,
                                                     max_width, max_height);
            if (item_value)
            {
                if (item_value[0])
                {
                    /* replace \n by spaces when height is 1 */
                    item_value2 = (max_height == 1) ?
                        string_replace (item_value, "\n", " ") : NULL;
                    items = string_explode ((item_value2) ? item_value2 : item_value,
                                            "\n", 0, 0,
                                            &items_count);
                    num_lines = (items_count <= max_height) ?
                        items_count : max_height;
                    for (line = 0; line < num_lines; line++)
                    {
                        wmove (bar_window->win_bar, y, x);
                        x += gui_bar_window_print_string (bar_window,
                                                          items[line],
                                                          max_width);
                        if (num_lines > 1)
                        {
                            x = 0;
                            y++;
                        }
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
    
    wnoutrefresh (bar_window->win_bar);
    
    if (bar_window->bar->separator)
    {
        switch (bar_window->bar->position)
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
                /* make C compiler happy */
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
}

/*
 * gui_bar_window_print_log: print bar window infos in log (usually for crash dump)
 */

void
gui_bar_window_print_log (struct t_gui_bar_window *bar_window)
{
    log_printf ("");
    log_printf ("  [window bar (addr:0x%x)]",   bar_window);
    log_printf ("    bar . . . . . . . : 0x%x", bar_window->bar);
    log_printf ("    x . . . . . . . . : %d",   bar_window->x);
    log_printf ("    y . . . . . . . . : %d",   bar_window->y);
    log_printf ("    width . . . . . . : %d",   bar_window->width);
    log_printf ("    height. . . . . . : %d",   bar_window->height);
    log_printf ("    win_bar . . . . . : 0x%x", bar_window->win_bar);
    log_printf ("    win_separator . . : 0x%x", bar_window->win_separator);
    log_printf ("    prev_bar_window . : 0x%x", bar_window->prev_bar_window);
    log_printf ("    next_bar_window . : 0x%x", bar_window->next_bar_window);
}
