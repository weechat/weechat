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
#include "../core/wee-log.h"
#include "../core/wee-utf8.h"
#include "gui-window.h"
#include "gui-buffer.h"
#include "gui-chat.h"
#include "gui-input.h"
#include "gui-hotlist.h"
#include "gui-status.h"


int gui_init_ok = 0;                            /* = 1 if GUI is initialized*/
int gui_ok = 0;                                 /* = 1 if GUI is ok         */
                                                /* (0 when size too small)  */
int gui_window_refresh_needed = 0;              /* = 1 if refresh needed    */

struct t_gui_window *gui_windows = NULL;        /* first window             */
struct t_gui_window *last_gui_window = NULL;    /* last window              */
struct t_gui_window *gui_current_window = NULL; /* current window           */

struct t_gui_window_tree *gui_windows_tree = NULL; /* windows tree          */


/*
 * gui_window_tree_init: create first entry in windows tree
 */

int
gui_window_tree_init (struct t_gui_window *window)
{
    gui_windows_tree = (struct t_gui_window_tree *)malloc (sizeof (struct t_gui_window_tree));
    if (!gui_windows_tree)
        return 0;
    gui_windows_tree->parent_node = NULL;
    gui_windows_tree->split_horiz = 0;
    gui_windows_tree->split_pct = 0;
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
    node->split_horiz = 0;
    node->split_pct = 0;
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
gui_window_new (struct t_gui_window *parent, int x, int y, int width, int height,
                int width_pct, int height_pct)
{
    struct t_gui_window *new_window;
    struct t_gui_window_tree *ptr_tree, *child1, *child2, *ptr_leaf;
    
#ifdef DEBUG
    log_printf ("Creating new window (x:%d, y:%d, width:%d, height:%d)",
                x, y, width, height);
#endif
    
    if (parent)
    {
        child1 = (struct t_gui_window_tree *)malloc (sizeof (struct t_gui_window_tree));
        if (!child1)
            return NULL;
        child2 = (struct t_gui_window_tree *)malloc (sizeof (struct t_gui_window_tree));
        if (!child2)
        {
            free (child1);
            return NULL;
        }
        ptr_tree = parent->ptr_tree;
        
        if (width_pct == 100)
        {
            ptr_tree->split_horiz = 1;
            ptr_tree->split_pct = height_pct;
        }
        else
        {
            ptr_tree->split_horiz = 0;
            ptr_tree->split_pct = width_pct;
        }
        
        /* parent window leaf becomes node and we add 2 leafs below
           (#1 is parent win, #2 is new win) */
        
        parent->ptr_tree = child1;
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
    
    if ((new_window = (struct t_gui_window *)(malloc (sizeof (struct t_gui_window)))))
    {
        if (!gui_window_objects_init (new_window))
        {
            free (new_window);
            return NULL;
        }
        new_window->win_x = x;
        new_window->win_y = y;
        new_window->win_width = width;
        new_window->win_height = height;
        new_window->win_width_pct = width_pct;
        new_window->win_height_pct = height_pct;
        
        new_window->new_x = -1;
        new_window->new_y = -1;
        new_window->new_width = -1;
        new_window->new_height = -1;
        
        new_window->win_chat_x = 0;
        new_window->win_chat_y = 0;
        new_window->win_chat_width = 0;
        new_window->win_chat_height = 0;
        new_window->win_chat_cursor_x = 0;
        new_window->win_chat_cursor_y = 0;
        
        new_window->win_nick_x = 0;
        new_window->win_nick_y = 0;
        new_window->win_nick_width = 0;
        new_window->win_nick_height = 0;
        new_window->win_nick_num_max = 0;
        new_window->win_nick_start = 0;
        
        new_window->win_title_x = 0;
        new_window->win_title_y = 0;
        new_window->win_title_width = 0;
        new_window->win_title_height = 0;
        new_window->win_title_start = 0;
        
        new_window->win_status_x = 0;
        new_window->win_status_y = 0;
        new_window->win_status_width = 0;
        new_window->win_status_height = 0;
        
        new_window->win_infobar_x = 0;
        new_window->win_infobar_y = 0;
        new_window->win_infobar_width = 0;
        new_window->win_infobar_height = 0;
        
        new_window->win_input_x = 0;
        new_window->win_input_y = 0;
        new_window->win_input_width = 0;
        new_window->win_input_height = 0;
        new_window->win_input_cursor_x = 0;
        
        new_window->dcc_first = NULL;
        new_window->dcc_selected = NULL;
        new_window->dcc_last_displayed = NULL;
        
        new_window->buffer = NULL;
        
        new_window->first_line_displayed = 0;
        new_window->start_line = NULL;
        new_window->start_line_pos = 0;
        new_window->scroll = 0;
        
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
    }
    else
        return NULL;
    
    return new_window;
}

/*
 * gui_window_free: delete a window
 */

void
gui_window_free (struct t_gui_window *window)
{
    if (window->buffer && (window->buffer->num_displayed > 0))
        window->buffer->num_displayed--;
    
    /* free data */
    gui_window_objects_free (window, 1);
    
    /* remove window from windows list */
    if (window->prev_window)
        window->prev_window->next_window = window->next_window;
    if (window->next_window)
        window->next_window->prev_window = window->prev_window;
    if (gui_windows == window)
        gui_windows = window->next_window;
    if (last_gui_window == window)
        last_gui_window = window->prev_window;

    if (gui_current_window == window)
        gui_current_window = gui_windows;
    
    free (window);
}

/*
 * gui_window_search_by_buffer: search a window by buffer
 *                              (return first window displaying this buffer)
 */

struct t_gui_window *
gui_window_search_by_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_window *ptr_window;
    
    if (!gui_ok)
        return NULL;
    
    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if (ptr_window->buffer == buffer)
            return ptr_window;
    }
    
    /* window not found */
    return NULL;
}

/*
 * gui_window_switch_server: switch server on servers buffer
 *                           (if same buffer is used for all buffers)
 */

void
gui_window_switch_server (struct t_gui_window *window)
{
    (void) window;
    /*struct t_gui_buffer *ptr_buffer;
    t_irc_server *ptr_server;
    
    ptr_buffer = gui_buffer_servers_search ();
    
    if (ptr_buffer)
    {
        ptr_server = (GUI_SERVER(ptr_buffer)
                      && GUI_SERVER(ptr_buffer)->next_server) ?
            GUI_SERVER(ptr_buffer)->next_server : irc_servers;
        while (ptr_server != GUI_SERVER(window->buffer))
        {
            if (ptr_server->buffer)
                break;
            if (ptr_server->next_server)
                ptr_server = ptr_server->next_server;
            else
            {
                if (GUI_SERVER(ptr_buffer) == NULL)
                {
                    ptr_server = NULL;
                    break;
                }
                ptr_server = irc_servers;
            }
        }
        if (ptr_server && (ptr_server != GUI_SERVER(ptr_buffer)))
        {
            ptr_buffer->server = ptr_server;
            gui_status_draw (window->buffer, 1);
            gui_input_draw (window->buffer, 1);
        }
    }*/
}

/*
 * gui_window_switch_previous: switch to previous window
 */

void
gui_window_switch_previous (struct t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    /* if only one window then return */
    if (gui_windows == last_gui_window)
        return;
    
    gui_current_window = (window->prev_window) ? window->prev_window : last_gui_window;
    gui_window_switch_to_buffer (gui_current_window, gui_current_window->buffer);
    gui_window_redraw_buffer (gui_current_window->buffer);
}

/*
 * gui_window_switch_next: switch to next window
 */

void
gui_window_switch_next (struct t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    /* if only one window then return */
    if (gui_windows == last_gui_window)
        return;
    
    gui_current_window = (window->next_window) ? window->next_window : gui_windows;
    gui_window_switch_to_buffer (gui_current_window, gui_current_window->buffer);
    gui_window_redraw_buffer (gui_current_window->buffer);
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
            gui_current_window = ptr_win;
            gui_window_switch_to_buffer (gui_current_window, gui_current_window->buffer);
            gui_window_redraw_buffer (gui_current_window->buffer);
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
    char *error;
    long number;
    struct t_gui_line *ptr_line;
    struct tm *date_tmp, line_date, old_line_date;
    
    if (window->buffer->lines)
    {
        direction = -1;
        number = 0;
        time_letter = ' ';
        
        // search direction
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
        
        // search number and letter
        char *pos = scroll;
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
                if (!error || (error[0] != '\0'))
                    number = 0;
                pos[0] = saved_char;
            }
        }
        
        /* at least number or letter has to he given */
        if ((number == 0) && (time_letter == ' '))
            return;
        
        // do the scroll!
        stop = 0;
        count_msg = 0;
        if (direction < 0)
            ptr_line = (window->start_line) ?
                window->start_line : window->buffer->last_line;
        else
            ptr_line = (window->start_line) ?
                window->start_line : window->buffer->lines;
        
        old_date = ptr_line->date;
        date_tmp = localtime (&old_date);
        memcpy (&old_line_date, date_tmp, sizeof (struct tm));
        
        while (ptr_line)
        {
            ptr_line = (direction < 0) ? ptr_line->prev_line : ptr_line->next_line;
            
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
                    date_tmp = localtime (&(ptr_line->date));
                    memcpy (&line_date, date_tmp, sizeof (struct tm));
                    if (old_date > ptr_line->date)
                        diff_date = old_date - ptr_line->date;
                    else
                        diff_date = ptr_line->date - old_date;
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
                        (window->start_line == window->buffer->lines);
                    window->buffer->chat_refresh_needed = 1;
                    gui_status_refresh_needed = 1;
                    return;
                }
            }
        }
        if (direction < 0)
            gui_window_scroll_top (window);
        else
            gui_window_scroll_bottom (window);
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
        if (window->buffer->lines
            && window->buffer->input_buffer && window->buffer->input_buffer[0])
        {
            ptr_line = (window->start_line) ?
                window->start_line->prev_line : window->buffer->last_line;
            while (ptr_line)
            {
                if (gui_chat_line_search (ptr_line,
                                          window->buffer->input_buffer,
                                          window->buffer->text_search_exact))
                {
                    window->start_line = ptr_line;
                    window->start_line_pos = 0;
                    window->first_line_displayed =
                        (window->start_line == window->buffer->lines);
                    window->buffer->chat_refresh_needed = 1;
                    gui_status_refresh_needed = 1;
                    return 1;
                }
                ptr_line = ptr_line->prev_line;
            }
        }
    }
    else if (window->buffer->text_search == GUI_TEXT_SEARCH_FORWARD)
    {
        if (window->buffer->lines
            && window->buffer->input_buffer && window->buffer->input_buffer[0])
        {
            ptr_line = (window->start_line) ?
                window->start_line->next_line : window->buffer->lines->next_line;
            while (ptr_line)
            {
                if (gui_chat_line_search (ptr_line,
                                          window->buffer->input_buffer,
                                          window->buffer->text_search_exact))
                {
                    window->start_line = ptr_line;
                    window->start_line_pos = 0;
                    window->first_line_displayed =
                        (window->start_line == window->buffer->lines);
                    window->buffer->chat_refresh_needed = 1;
                    gui_status_refresh_needed = 1;
                    return 1;
                }
                ptr_line = ptr_line->next_line;
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
    gui_status_refresh_needed = 1;
    window->buffer->input_refresh_needed = 1;
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
    {
        window->buffer->chat_refresh_needed = 1;
        gui_status_refresh_needed = 1;
    }
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
        free (window->buffer->text_search_input);
        window->buffer->text_search_input = NULL;
    }
    window->start_line = NULL;
    window->start_line_pos = 0;
    gui_hotlist_remove_buffer (window->buffer);
    window->buffer->chat_refresh_needed = 1;
    gui_status_refresh_needed = 1;
    window->buffer->input_refresh_needed = 1;
}

/*
 * gui_window_print_log: print window infos in log (usually for crash dump)
 */

void
gui_window_print_log ()
{
    struct t_gui_window *ptr_window;
    
    log_printf ("");
    log_printf ("current window = 0x%x", gui_current_window);
    
    for (ptr_window = gui_windows; ptr_window; ptr_window = ptr_window->next_window)
    {
        log_printf ("");
        log_printf ("[window (addr:0x%x)]", ptr_window);
        log_printf ("  win_x . . . . . . . : %d",   ptr_window->win_x);
        log_printf ("  win_y . . . . . . . : %d",   ptr_window->win_y);
        log_printf ("  win_width . . . . . : %d",   ptr_window->win_width);
        log_printf ("  win_height. . . . . : %d",   ptr_window->win_height);
        log_printf ("  win_width_pct . . . : %d",   ptr_window->win_width_pct);
        log_printf ("  win_height_pct. . . : %d",   ptr_window->win_height_pct);
        log_printf ("  win_chat_x. . . . . : %d",   ptr_window->win_chat_x);
        log_printf ("  win_chat_y. . . . . : %d",   ptr_window->win_chat_y);
        log_printf ("  win_chat_width. . . : %d",   ptr_window->win_chat_width);
        log_printf ("  win_chat_height . . : %d",   ptr_window->win_chat_height);
        log_printf ("  win_chat_cursor_x . : %d",   ptr_window->win_chat_cursor_x);
        log_printf ("  win_chat_cursor_y . : %d",   ptr_window->win_chat_cursor_y);
        log_printf ("  win_nick_x. . . . . : %d",   ptr_window->win_nick_x);
        log_printf ("  win_nick_y. . . . . : %d",   ptr_window->win_nick_y);
        log_printf ("  win_nick_width. . . : %d",   ptr_window->win_nick_width);
        log_printf ("  win_nick_height . . : %d",   ptr_window->win_nick_height);
        log_printf ("  win_nick_start. . . : %d",   ptr_window->win_nick_start);
        log_printf ("  win_title_x . . . . : %d",   ptr_window->win_title_x);
        log_printf ("  win_title_y . . . . : %d",   ptr_window->win_title_y);
        log_printf ("  win_title_width . . : %d",   ptr_window->win_title_width);
        log_printf ("  win_title_height. . : %d",   ptr_window->win_title_height);
        log_printf ("  win_title_start . . : %d",   ptr_window->win_title_start);
        log_printf ("  win_status_x. . . . : %d",   ptr_window->win_status_x);
        log_printf ("  win_status_y. . . . : %d",   ptr_window->win_status_y);
        log_printf ("  win_status_width. . : %d",   ptr_window->win_status_width);
        log_printf ("  win_status_height . : %d",   ptr_window->win_status_height);
        log_printf ("  win_infobar_x . . . : %d",   ptr_window->win_infobar_x);
        log_printf ("  win_infobar_y . . . : %d",   ptr_window->win_infobar_y);
        log_printf ("  win_infobar_width . : %d",   ptr_window->win_infobar_width);
        log_printf ("  win_infobar_height. : %d",   ptr_window->win_infobar_height);
        log_printf ("  win_input_x . . . . : %d",   ptr_window->win_input_x);
        log_printf ("  win_input_y . . . . : %d",   ptr_window->win_input_y);
        log_printf ("  win_input_width . . : %d",   ptr_window->win_input_width);
        log_printf ("  win_input_height. . : %d",   ptr_window->win_input_height);
        log_printf ("  win_input_cursor_x. : %d",   ptr_window->win_input_cursor_x);
        gui_window_objects_print_log (ptr_window);
        log_printf ("  dcc_first . . . . . : 0x%x", ptr_window->dcc_first);
        log_printf ("  dcc_selected. . . . : 0x%x", ptr_window->dcc_selected);
        log_printf ("  dcc_last_displayed. : 0x%x", ptr_window->dcc_last_displayed);
        log_printf ("  buffer. . . . . . . : 0x%x", ptr_window->buffer);
        log_printf ("  first_line_displayed: %d",   ptr_window->first_line_displayed);
        log_printf ("  start_line. . . . . : 0x%x", ptr_window->start_line);
        log_printf ("  start_line_pos. . . : %d",   ptr_window->start_line_pos);
        log_printf ("  prev_window . . . . : 0x%x", ptr_window->prev_window);
        log_printf ("  next_window . . . . : 0x%x", ptr_window->next_window);
    }
}
