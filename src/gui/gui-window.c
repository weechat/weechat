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

/* gui-window.c: window functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hook.h"
#include "../core/wee-infolist.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../plugins/plugin.h"
#include "gui-window.h"
#include "gui-bar.h"
#include "gui-bar-window.h"
#include "gui-buffer.h"
#include "gui-filter.h"
#include "gui-input.h"
#include "gui-hotlist.h"
#include "gui-layout.h"
#include "gui-line.h"


int gui_init_ok = 0;                            /* = 1 if GUI is initialized*/
int gui_ok = 0;                                 /* = 1 if GUI is ok         */
                                                /* (0 when size too small)  */
int gui_window_refresh_needed = 0;              /* = 1 if refresh needed    */
                                                /* = 2 for full refresh     */
struct t_gui_window *gui_windows = NULL;        /* first window             */
struct t_gui_window *last_gui_window = NULL;    /* last window              */
struct t_gui_window *gui_current_window = NULL; /* current window           */

struct t_gui_window_tree *gui_windows_tree = NULL; /* windows tree          */

struct t_gui_layout_window *gui_window_layout_before_zoom = NULL;
                                       /* layout before zooming on a window */
int gui_window_layout_id_current_window = -1;
                                       /* current window id before zoom     */


/*
 * gui_window_ask_refresh: set "gui_window_refresh_needed" flag
 */

void
gui_window_ask_refresh (int refresh)
{
    if (refresh > gui_window_refresh_needed)
        gui_window_refresh_needed = refresh;
}

/*
 * gui_window_tree_init: create first entry in windows tree
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
 * gui_window_tree_node_to_leaf: convert a node to a leaf (free any leafs)
 *                               Called when 2 windows are merging into one
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
 * gui_window_tree_free: delete entire windows tree
 */

void
gui_window_tree_free (struct t_gui_window_tree **tree)
{
    if (*tree)
    {
        if ((*tree)->child1)
            gui_window_tree_free (&((*tree)->child1));
        if ((*tree)->child2)
            gui_window_tree_free (&((*tree)->child2));
        free (*tree);
        *tree = NULL;
    }
}

/*
 * gui_window_new: create a new window
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
        
        /* parent window leaf becomes node and we add 2 leafs below
           (#1 is parent win, #2 is new win) */
        
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
    
    if ((new_window = (malloc (sizeof (*new_window)))))
    {
        /* create window objects */
        if (!gui_window_objects_init (new_window))
        {
            free (new_window);
            return NULL;
        }
        
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
        new_window->first_line_displayed = 0;
        new_window->start_line = NULL;
        new_window->start_line_pos = 0;
        new_window->scroll = 0;
        new_window->scroll_lines_after = 0;
        new_window->scroll_reset_allowed = 0;
        
        new_window->ptr_tree = ptr_leaf;
        ptr_leaf->window = new_window;
        
        /* add window to windows queue */
        new_window->prev_window = last_gui_window;
        if (gui_windows)
            last_gui_window->next_window = new_window;
        else
            gui_windows = new_window;
        last_gui_window = new_window;
        new_window->next_window = NULL;
        
        /* create bar windows */
        for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
        {
            if (CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_TYPE]) != GUI_BAR_TYPE_ROOT)
                gui_bar_window_new (ptr_bar, new_window);
        }
    }
    else
        return NULL;
    
    return new_window;
}

/*
 * gui_window_valid: check if a buffer pointer exists
 *                   return 1 if buffer exists
 *                          0 if buffer is not found
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
 * gui_window_get_integer: get a window property as integer
 */

int
gui_window_get_integer (struct t_gui_window *window, const char *property)
{
    if (window && property)
    {
        if (string_strcasecmp (property, "win_x") == 0)
            return window->win_x;
        if (string_strcasecmp (property, "win_y") == 0)
            return window->win_y;
        if (string_strcasecmp (property, "win_width") == 0)
            return window->win_width;
        if (string_strcasecmp (property, "win_height") == 0)
            return window->win_height;
        if (string_strcasecmp (property, "win_width_pct") == 0)
            return window->win_width_pct;
        if (string_strcasecmp (property, "win_height_pct") == 0)
            return window->win_height_pct;
        if (string_strcasecmp (property, "win_chat_x") == 0)
            return window->win_chat_x;
        if (string_strcasecmp (property, "win_chat_y") == 0)
            return window->win_chat_y;
        if (string_strcasecmp (property, "win_chat_width") == 0)
            return window->win_chat_width;
        if (string_strcasecmp (property, "win_chat_height") == 0)
            return window->win_chat_height;
        if (string_strcasecmp (property, "first_line_displayed") == 0)
            return window->first_line_displayed;
        if (string_strcasecmp (property, "scroll") == 0)
            return window->scroll;
        if (string_strcasecmp (property, "scroll_lines_after") == 0)
            return window->scroll_lines_after;
    }
    
    return 0;
}

/*
 * gui_window_get_string: get a window property as string
 */

const char *
gui_window_get_string (struct t_gui_window *window, const char *property)
{
    if (window && property)
    {
    }
    
    return NULL;
}

/*
 * gui_windowr_get_pointer: get a window property as pointer
 */

void *
gui_window_get_pointer (struct t_gui_window *window, const char *property)
{
    if (property)
    {
        if (string_strcasecmp (property, "current") == 0)
            return gui_current_window;
        
        if (window)
        {
            if (string_strcasecmp (property, "buffer") == 0)
                return window->buffer;
        }
    }
    
    return NULL;
}

/*
 * gui_window_set_layout_plugin_name: set layout plugin name for window
 */

void
gui_window_set_layout_plugin_name (struct t_gui_window *window,
                                   const char *plugin_name)
{
    if (window->layout_plugin_name)
    {
        free (window->layout_plugin_name);
        window->layout_plugin_name = NULL;
    }
    
    if (plugin_name)
        window->layout_plugin_name = strdup (plugin_name);
}

/*
 * gui_window_set_layout_buffer_name: set layout buffer name for window
 */

void
gui_window_set_layout_buffer_name (struct t_gui_window *window,
                                   const char *buffer_name)
{
    if (window->layout_buffer_name)
    {
        free (window->layout_buffer_name);
        window->layout_buffer_name = NULL;
    }
    
    if (buffer_name)
        window->layout_buffer_name = strdup (buffer_name);
}

/*
 * gui_window_free: delete a window
 */

void
gui_window_free (struct t_gui_window *window)
{
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
    if (window->layout_plugin_name)
        free (window->layout_plugin_name);
    if (window->layout_buffer_name)
        free (window->layout_buffer_name);
    
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
    
    free (window);
}

/*
 * gui_window_switch_previous: switch to previous window
 */

void
gui_window_switch_previous (struct t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    gui_window_switch ((window->prev_window) ?
                       window->prev_window : last_gui_window);
}

/*
 * gui_window_switch_next: switch to next window
 */

void
gui_window_switch_next (struct t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    gui_window_switch ((window->next_window) ?
                       window->next_window : gui_windows);
}

/*
 * gui_window_switch_by_buffer: switch to next window displaying a buffer
 */

void
gui_window_switch_by_buffer (struct t_gui_window *window, int buffer_number)
{
    struct t_gui_window *ptr_win;
    
    if (!gui_ok)
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
 * gui_window_scroll: scroll window by # messages or time
 */

void
gui_window_scroll (struct t_gui_window *window, char *scroll)
{
    int direction, stop, count_msg;
    char time_letter, saved_char;
    time_t old_date, diff_date;
    char *pos, *error;
    long number;
    struct t_gui_line *ptr_line;
    struct tm *date_tmp, line_date, old_line_date;
    
    if (window->buffer->lines->first_line)
    {
        direction = 1;
        number = 0;
        time_letter = ' ';
        
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
        
        /* search number and letter */
        pos = scroll;
        while (pos && pos[0] && isdigit (pos[0]))
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
            ptr_line = (window->start_line) ?
                window->start_line : window->buffer->lines->last_line;
            while (ptr_line && !gui_line_is_displayed (ptr_line))
            {
                ptr_line = ptr_line->prev_line;
            }
        }
        else
        {
            ptr_line = (window->start_line) ?
                window->start_line : window->buffer->lines->first_line;
            while (ptr_line && !gui_line_is_displayed (ptr_line))
            {
                ptr_line = ptr_line->next_line;
            }
        }
        
        old_date = ptr_line->data->date;
        date_tmp = localtime (&old_date);
        memcpy (&old_line_date, date_tmp, sizeof (struct tm));
        
        while (ptr_line)
        {
            ptr_line = (direction < 0) ?
                gui_line_get_prev_displayed (ptr_line) : gui_line_get_next_displayed (ptr_line);
            
            if (ptr_line)
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
                            /* we consider month is 30 days, who will find I'm too
                               lazy to code exact date diff ? ;) */
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
                            /* we consider year is 365 days, who will find I'm too
                               lazy to code exact date diff ? ;) */
                            else if (diff_date >= number * 60 * 60 * 24 * 365)
                                stop = 1;
                            break;
                    }
                }
                if (stop)
                {
                    window->start_line = ptr_line;
                    window->start_line_pos = 0;
                    window->first_line_displayed =
                        (window->start_line == gui_line_get_first_displayed (window->buffer));
                    gui_buffer_ask_chat_refresh (window->buffer, 2);
                    return;
                }
            }
        }
        if (direction < 0)
            gui_window_scroll_top (window);
        else
        {
            if (window->buffer->type == GUI_BUFFER_TYPE_FORMATTED)
                gui_window_scroll_bottom (window);
        }
    }
}

/*
 * gui_window_scroll_previous_highlight: scroll to previous highlight
 */

void
gui_window_scroll_previous_highlight (struct t_gui_window *window)
{
    struct t_gui_line *ptr_line;
    
    if ((window->buffer->type == GUI_BUFFER_TYPE_FORMATTED)
        && (window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED))
    {
        if (window->buffer->lines->first_line)
        {
            ptr_line = (window->start_line) ?
                window->start_line->prev_line : window->buffer->lines->last_line;
            while (ptr_line)
            {
                if (ptr_line->data->highlight)
                {
                    window->start_line = ptr_line;
                    window->start_line_pos = 0;
                    window->first_line_displayed =
                        (window->start_line == window->buffer->lines->first_line);
                    gui_buffer_ask_chat_refresh (window->buffer, 2);
                    return;
                }
                ptr_line = ptr_line->prev_line;
            }
        }
    }
}

/*
 * gui_window_scroll_next_highlight: scroll to next highlight
 */

void
gui_window_scroll_next_highlight (struct t_gui_window *window)
{
    struct t_gui_line *ptr_line;
    
    if ((window->buffer->type == GUI_BUFFER_TYPE_FORMATTED)
        && (window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED))
    {
        if (window->buffer->lines->first_line)
        {
            ptr_line = (window->start_line) ?
                window->start_line->next_line : window->buffer->lines->first_line->next_line;
            while (ptr_line)
            {
                if (ptr_line->data->highlight)
                {
                    window->start_line = ptr_line;
                    window->start_line_pos = 0;
                    window->first_line_displayed =
                        (window->start_line == window->buffer->lines->first_line);
                    gui_buffer_ask_chat_refresh (window->buffer, 2);
                    return;
                }
                ptr_line = ptr_line->next_line;
            }
        }
    }
}

/*
 * gui_window_search_text: search text in a buffer
 */

int
gui_window_search_text (struct t_gui_window *window)
{
    struct t_gui_line *ptr_line;
    
    if (window->buffer->text_search == GUI_TEXT_SEARCH_BACKWARD)
    {
        if (window->buffer->lines->first_line
            && window->buffer->input_buffer && window->buffer->input_buffer[0])
        {
            ptr_line = (window->start_line) ?
                gui_line_get_prev_displayed (window->start_line) :
                gui_line_get_last_displayed (window->buffer);
            while (ptr_line)
            {
                if (gui_line_search_text (ptr_line,
                                          window->buffer->input_buffer,
                                          window->buffer->text_search_exact))
                {
                    window->start_line = ptr_line;
                    window->start_line_pos = 0;
                    window->first_line_displayed =
                        (window->start_line == gui_line_get_first_displayed (window->buffer));
                    gui_buffer_ask_chat_refresh (window->buffer, 2);
                    return 1;
                }
                ptr_line = gui_line_get_prev_displayed (ptr_line);
            }
        }
    }
    else if (window->buffer->text_search == GUI_TEXT_SEARCH_FORWARD)
    {
        if (window->buffer->lines->first_line
            && window->buffer->input_buffer && window->buffer->input_buffer[0])
        {
            ptr_line = (window->start_line) ?
                gui_line_get_next_displayed (window->start_line) :
                gui_line_get_first_displayed (window->buffer);
            while (ptr_line)
            {
                if (gui_line_search_text (ptr_line,
                                          window->buffer->input_buffer,
                                          window->buffer->text_search_exact))
                {
                    window->start_line = ptr_line;
                    window->start_line_pos = 0;
                    window->first_line_displayed =
                        (window->start_line == window->buffer->lines->first_line);
                    gui_buffer_ask_chat_refresh (window->buffer, 2);
                    return 1;
                }
                ptr_line = gui_line_get_next_displayed (ptr_line);
            }
        }
    }
    return 0;
}

/*
 * gui_window_search_start: start search in a buffer
 */

void
gui_window_search_start (struct t_gui_window *window)
{
    window->buffer->text_search = GUI_TEXT_SEARCH_BACKWARD;
    window->buffer->text_search_exact = 0;
    window->buffer->text_search_found = 0;
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
 * gui_window_search_restart: restart search (after input changes or exact
 *                            flag (un)set)
 */

void
gui_window_search_restart (struct t_gui_window *window)
{
    window->start_line = NULL;
    window->start_line_pos = 0;
    window->buffer->text_search = GUI_TEXT_SEARCH_BACKWARD;
    window->buffer->text_search_found = 0;
    if (gui_window_search_text (window))
        window->buffer->text_search_found = 1;
    else
        gui_buffer_ask_chat_refresh (window->buffer, 2);
}

/*
 * gui_window_search_stop: stop search in a buffer
 */

void
gui_window_search_stop (struct t_gui_window *window)
{
    window->buffer->text_search = GUI_TEXT_SEARCH_DISABLED;
    window->buffer->text_search = 0;
    gui_input_delete_line (window->buffer);
    if (window->buffer->text_search_input)
    {
        gui_input_insert_string (window->buffer,
                                 window->buffer->text_search_input, -1);
        gui_input_text_changed_modifier_and_signal (window->buffer);
        free (window->buffer->text_search_input);
        window->buffer->text_search_input = NULL;
    }
    window->start_line = NULL;
    window->start_line_pos = 0;
    gui_hotlist_remove_buffer (window->buffer);
    gui_buffer_ask_chat_refresh (window->buffer, 2);
}

/*
 * gui_window_zoom: zoom window (maximize it or restore layout before previous
 *                  zoom)
 */

void
gui_window_zoom (struct t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (gui_window_layout_before_zoom)
    {
        /* restore layout as it was before zooming a window */
        hook_signal_send ("window_unzoom",
                          WEECHAT_HOOK_SIGNAL_POINTER, gui_current_window);
        gui_layout_window_apply (gui_window_layout_before_zoom,
                                 gui_window_layout_id_current_window);
        gui_layout_window_remove_all (&gui_window_layout_before_zoom);
        gui_window_layout_id_current_window = -1;
        hook_signal_send ("window_unzoomed",
                          WEECHAT_HOOK_SIGNAL_POINTER, gui_current_window);
    }
    else
    {
        /* save layout and zoom on current window */
        hook_signal_send ("window_zoom",
                          WEECHAT_HOOK_SIGNAL_POINTER, gui_current_window);
        gui_window_layout_id_current_window =
            gui_layout_window_save (&gui_window_layout_before_zoom);
        gui_window_merge_all (window);
        hook_signal_send ("window_zoomed",
                          WEECHAT_HOOK_SIGNAL_POINTER, gui_current_window);
    }
}

/*
 * gui_window_add_to_infolist: add a window in an infolist
 *                             return 1 if ok, 0 if error
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
                                    && (window->start_line)) ?
                                   window->start_line->data->y : 0))
        return 0;
    
    return 1;
}

/*
 * gui_window_print_log: print window infos in log (usually for crash dump)
 */

void
gui_window_print_log ()
{
    struct t_gui_window *ptr_window;
    struct t_gui_bar_window *ptr_bar_win;
    
    log_printf ("");
    log_printf ("gui_windows . . . . . . . . . : 0x%lx", gui_windows);
    log_printf ("last_gui_window . . . . . . . : 0x%lx", last_gui_window);
    log_printf ("gui_current window. . . . . . : 0x%lx", gui_current_window);
    log_printf ("gui_windows_tree. . . . . . . : 0x%lx", gui_windows_tree);
    log_printf ("gui_window_layout_before_zoom : 0x%lx", gui_window_layout_before_zoom);
    
    for (ptr_window = gui_windows; ptr_window; ptr_window = ptr_window->next_window)
    {
        log_printf ("");
        log_printf ("[window (addr:0x%lx)]", ptr_window);
        log_printf ("  win_x . . . . . . . : %d",    ptr_window->win_x);
        log_printf ("  win_y . . . . . . . : %d",    ptr_window->win_y);
        log_printf ("  win_width . . . . . : %d",    ptr_window->win_width);
        log_printf ("  win_height. . . . . : %d",    ptr_window->win_height);
        log_printf ("  win_width_pct . . . : %d",    ptr_window->win_width_pct);
        log_printf ("  win_height_pct. . . : %d",    ptr_window->win_height_pct);
        log_printf ("  win_chat_x. . . . . : %d",    ptr_window->win_chat_x);
        log_printf ("  win_chat_y. . . . . : %d",    ptr_window->win_chat_y);
        log_printf ("  win_chat_width. . . : %d",    ptr_window->win_chat_width);
        log_printf ("  win_chat_height . . : %d",    ptr_window->win_chat_height);
        log_printf ("  win_chat_cursor_x . : %d",    ptr_window->win_chat_cursor_x);
        log_printf ("  win_chat_cursor_y . : %d",    ptr_window->win_chat_cursor_y);
        log_printf ("  refresh_needed. . . : %d",    ptr_window->refresh_needed);
        log_printf ("  gui_objects . . . . : 0x%lx", ptr_window->gui_objects);
        gui_window_objects_print_log (ptr_window);
        log_printf ("  buffer. . . . . . . : 0x%lx", ptr_window->buffer);
        log_printf ("  layout_plugin_name. : '%s'",  ptr_window->layout_plugin_name);
        log_printf ("  layout_buffer_name. : '%s'",  ptr_window->layout_buffer_name);
        log_printf ("  first_line_displayed: %d",    ptr_window->first_line_displayed);
        log_printf ("  start_line. . . . . : 0x%lx", ptr_window->start_line);
        log_printf ("  start_line_pos. . . : %d",    ptr_window->start_line_pos);
        log_printf ("  scroll. . . . . . . : %d",    ptr_window->scroll);
        log_printf ("  scroll_lines_after. : %d",    ptr_window->scroll_lines_after);
        log_printf ("  scroll_reset_allowed: %d",    ptr_window->scroll_reset_allowed);
        log_printf ("  ptr_tree. . . . . . : 0x%lx", ptr_window->ptr_tree);
        log_printf ("  prev_window . . . . : 0x%lx", ptr_window->prev_window);
        log_printf ("  next_window . . . . : 0x%lx", ptr_window->next_window);
        
        for (ptr_bar_win = ptr_window->bar_windows; ptr_bar_win;
             ptr_bar_win = ptr_bar_win->next_bar_window)
        {
            gui_bar_window_print_log (ptr_bar_win);
        }
    }
}
