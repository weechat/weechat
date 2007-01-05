/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

/* gui-curses-panel.c: panel functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../irc/irc.h"
#include "gui-curses.h"


/*
 * gui_panel_windows_get_size: get total panel size (window panels) for a position
 *                             panel is optional, if not NULL, size is computed
 *                             from panel 1 to panel # - 1
 */

int
gui_panel_window_get_size (t_gui_panel *panel, t_gui_window *window, int position)
{
    t_gui_panel_window *ptr_panel_win;
    int total_size;

    total_size = 0;
    for (ptr_panel_win = GUI_CURSES(window)->panel_windows; ptr_panel_win;
         ptr_panel_win = ptr_panel_win->next_panel_window)
    {
        /* stop before panel */
        if ((panel) && (ptr_panel_win->panel == panel))
            return total_size;
        
        if (ptr_panel_win->panel->position == position)
        {
            switch (position)
            {
                case GUI_PANEL_TOP:
                case GUI_PANEL_BOTTOM:
                    total_size += ptr_panel_win->height;
                    break;
                case GUI_PANEL_LEFT:
                case GUI_PANEL_RIGHT:
                    total_size += ptr_panel_win->width;
                    break;
            }
            if (ptr_panel_win->panel->separator)
                total_size++;
        }
    }
    return total_size;
}

/*
 * gui_panel_window_new: create a new "window panel" for a panel, in screen or a window
 *                       if window is not NULL, panel window will be in this window
 */

int
gui_panel_window_new (t_gui_panel *panel, t_gui_window *window)
{
    t_gui_panel_window *new_panel_win;
    int x1, y1, x2, y2;
    int add_top, add_bottom, add_left, add_right;

    if (window)
    {
        x1 = window->win_x;
        y1 = window->win_y + 1;
        x2 = x1 + window->win_width - 1;
        y2 = y1 + window->win_height - 1 - 4;
        add_left = gui_panel_window_get_size (panel, window, GUI_PANEL_LEFT);
        add_right = gui_panel_window_get_size (panel, window, GUI_PANEL_RIGHT);
        add_top = gui_panel_window_get_size (panel, window, GUI_PANEL_TOP);
        add_bottom = gui_panel_window_get_size (panel, window, GUI_PANEL_BOTTOM);
    }
    else
    {
        x1 = 0;
        y1 = 0;
        x2 = gui_window_get_width () - 1;
        y2 = gui_window_get_height () - 1;
        add_left = gui_panel_global_get_size (panel, GUI_PANEL_LEFT);
        add_right = gui_panel_global_get_size (panel, GUI_PANEL_RIGHT);
        add_top = gui_panel_global_get_size (panel, GUI_PANEL_TOP);
        add_bottom = gui_panel_global_get_size (panel, GUI_PANEL_BOTTOM);
    }
    
    if ((new_panel_win = (t_gui_panel_window *) malloc (sizeof (t_gui_panel_window))))
    {
        new_panel_win->panel = panel;
        if (window)
        {
            panel->panel_window = NULL;
            new_panel_win->next_panel_window = GUI_CURSES(window)->panel_windows;
            GUI_CURSES(window)->panel_windows = new_panel_win;
        }
        else
        {
            panel->panel_window = new_panel_win;
            new_panel_win->next_panel_window = NULL;
        }
        switch (panel->position)
        {
            case GUI_PANEL_TOP:
                new_panel_win->x = x1 + add_left;
                new_panel_win->y = y1 + add_top;
                new_panel_win->width = x2 - x1 + 1;
                new_panel_win->height = panel->size;
                break;
            case GUI_PANEL_BOTTOM:
                new_panel_win->x = x1;
                new_panel_win->y = y2 - panel->size + 1;
                new_panel_win->width = x2 - x1 + 1;
                new_panel_win->height = panel->size;
                break;
            case GUI_PANEL_LEFT:
                new_panel_win->x = x1;
                new_panel_win->y = y1;
                new_panel_win->width = panel->size;
                new_panel_win->height = y2 - y1 + 1;
                break;
            case GUI_PANEL_RIGHT:
                new_panel_win->x = x2 - panel->size + 1;
                new_panel_win->y = y1;
                new_panel_win->width = panel->size;
                new_panel_win->height = y2 - y1 + 1;
                break;
        }
        new_panel_win->win_panel = newwin (new_panel_win->height,
                                           new_panel_win->width,
                                           new_panel_win->y,
                                           new_panel_win->x);
        new_panel_win->win_separator = NULL;
        if (new_panel_win->panel->separator)
        {
            switch (panel->position)
            {
                case GUI_PANEL_LEFT:
                    new_panel_win->win_separator = newwin (new_panel_win->height,
                                                           1,
                                                           new_panel_win->y,
                                                           new_panel_win->x + new_panel_win->width);
                    break;
            }
        }                                           
        return 1;
    }
    else
        return 0;
}

/*
 * gui_panel_window_free: delete a panel window
 */

void
gui_panel_window_free (void *panel_win)
{
    t_gui_panel_window *ptr_panel_win;
    
    ptr_panel_win = (t_gui_panel_window *)panel_win;
    
    free (ptr_panel_win);
}

/*
 * gui_panel_redraw_buffer: redraw panels for windows displaying a buffer
 */

void
gui_panel_redraw_buffer (t_gui_buffer *buffer)
{
    t_gui_window *ptr_win;
    t_gui_panel_window *ptr_panel_win;
    
    if (!gui_ok)
        return;

    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if ((ptr_win->buffer == buffer) && (buffer->num_displayed > 0))
        {
            for (ptr_panel_win = GUI_CURSES(ptr_win)->panel_windows;
                 ptr_panel_win;
                 ptr_panel_win = ptr_panel_win->next_panel_window)
            {
                switch (ptr_panel_win->panel->position)
                {
                    case GUI_PANEL_LEFT:
                        break;
                }
            }
        }
    }
}
