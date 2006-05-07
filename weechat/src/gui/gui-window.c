/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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

#include "../common/weechat.h"
#include "gui.h"
#include "../common/command.h"
#include "../common/weeconfig.h"
#include "../common/history.h"
#include "../common/hotlist.h"
#include "../common/log.h"
#include "../common/utf8.h"
#include "../irc/irc.h"


t_gui_window *gui_windows = NULL;           /* pointer to first window      */
t_gui_window *last_gui_window = NULL;       /* pointer to last window       */
t_gui_window *gui_current_window = NULL;    /* pointer to current window    */

t_gui_window_tree *gui_windows_tree = NULL; /* pointer to windows tree      */


/*
 * gui_window_tree_init: create first entry in windows tree
 */

int
gui_window_tree_init (t_gui_window *window)
{
    gui_windows_tree = (t_gui_window_tree *)malloc (sizeof (t_gui_window_tree));
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
gui_window_tree_node_to_leaf (t_gui_window_tree *node, t_gui_window *window)
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
gui_window_tree_free (t_gui_window_tree **tree)
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

t_gui_window *
gui_window_new (t_gui_window *parent, int x, int y, int width, int height,
                int width_pct, int height_pct)
{
    t_gui_window *new_window;
    t_gui_window_tree *ptr_tree, *child1, *child2, *ptr_leaf;
    t_gui_panel *ptr_panel;
    
#ifdef DEBUG
    weechat_log_printf ("Creating new window (x:%d, y:%d, width:%d, height:%d)\n",
                        x, y, width, height);
#endif
    
    if (parent)
    {
        child1 = (t_gui_window_tree *)malloc (sizeof (t_gui_window_tree));
        if (!child1)
            return NULL;
        child2 = (t_gui_window_tree *)malloc (sizeof (t_gui_window_tree));
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
    
    if ((new_window = (t_gui_window *)(malloc (sizeof (t_gui_window)))))
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
            
        new_window->win_input_x = 0;
            
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

        /* add panels to window */
        for (ptr_panel = gui_panels; ptr_panel;
             ptr_panel = ptr_panel->next_panel)
        {
            if (!ptr_panel->panel_window)
                gui_panel_window_new (ptr_panel, new_window);
        }
        
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
gui_window_free (t_gui_window *window)
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
    
    free (window);
}

/*
 * gui_window_switch_server: switch server on servers buffer
 *                           (if same buffer is used for all buffers)
 */

void
gui_window_switch_server (t_gui_window *window)
{
    t_gui_buffer *ptr_buffer;
    t_irc_server *ptr_server;
    
    ptr_buffer = gui_buffer_servers_search ();
    
    if (ptr_buffer)
    {
        ptr_server = (SERVER(ptr_buffer) && SERVER(ptr_buffer)->next_server) ?
            SERVER(ptr_buffer)->next_server : irc_servers;
        while (ptr_server != SERVER(window->buffer))
        {
            if (ptr_server->buffer)
                break;
            if (ptr_server->next_server)
                ptr_server = ptr_server->next_server;
            else
            {
                if (SERVER(ptr_buffer) == NULL)
                {
                    ptr_server = NULL;
                    break;
                }
                ptr_server = irc_servers;
            }
        }
        if (ptr_server && (ptr_server != SERVER(ptr_buffer)))
        {
            ptr_buffer->server = ptr_server;
            gui_status_draw (window->buffer, 1);
            gui_input_draw (window->buffer, 1);
        }
    }
}

/*
 * gui_window_switch_previous: switch to previous window
 */

void
gui_window_switch_previous (t_gui_window *window)
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
gui_window_switch_next (t_gui_window *window)
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
gui_window_switch_by_buffer (t_gui_window *window, int buffer_number)
{
    t_gui_window *ptr_win;
    
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
 * gui_window_print_log: print window infos in log (usually for crash dump)
 */

void
gui_window_print_log (t_gui_window *window)
{
    weechat_log_printf ("[window (addr:0x%X)]\n", window);
    weechat_log_printf ("  win_x . . . . . . . : %d\n",   window->win_x);
    weechat_log_printf ("  win_y . . . . . . . : %d\n",   window->win_y);
    weechat_log_printf ("  win_width . . . . . : %d\n",   window->win_width);
    weechat_log_printf ("  win_height. . . . . : %d\n",   window->win_height);
    weechat_log_printf ("  win_width_pct . . . : %d\n",   window->win_width_pct);
    weechat_log_printf ("  win_height_pct. . . : %d\n",   window->win_height_pct);
    weechat_log_printf ("  win_chat_x. . . . . : %d\n",   window->win_chat_x);
    weechat_log_printf ("  win_chat_y. . . . . : %d\n",   window->win_chat_y);
    weechat_log_printf ("  win_chat_width. . . : %d\n",   window->win_chat_width);
    weechat_log_printf ("  win_chat_height . . : %d\n",   window->win_chat_height);
    weechat_log_printf ("  win_chat_cursor_x . : %d\n",   window->win_chat_cursor_x);
    weechat_log_printf ("  win_chat_cursor_y . : %d\n",   window->win_chat_cursor_y);
    weechat_log_printf ("  win_nick_x. . . . . : %d\n",   window->win_nick_x);
    weechat_log_printf ("  win_nick_y. . . . . : %d\n",   window->win_nick_y);
    weechat_log_printf ("  win_nick_width. . . : %d\n",   window->win_nick_width);
    weechat_log_printf ("  win_nick_height . . : %d\n",   window->win_nick_height);
    weechat_log_printf ("  win_nick_start. . . : %d\n",   window->win_nick_start);
    gui_window_objects_print_log (window);
    weechat_log_printf ("  dcc_first . . . . . : 0x%X\n", window->dcc_first);
    weechat_log_printf ("  dcc_selected. . . . : 0x%X\n", window->dcc_selected);
    weechat_log_printf ("  dcc_last_displayed. : 0x%X\n", window->dcc_last_displayed);
    weechat_log_printf ("  buffer. . . . . . . : 0x%X\n", window->buffer);
    weechat_log_printf ("  first_line_displayed: %d\n",   window->first_line_displayed);
    weechat_log_printf ("  start_line. . . . . : 0x%X\n", window->start_line);
    weechat_log_printf ("  start_line_pos. . . : %d\n",   window->start_line_pos);
    weechat_log_printf ("  prev_window . . . . : 0x%X\n", window->prev_window);
    weechat_log_printf ("  next_window . . . . : 0x%X\n", window->next_window);
    
}
