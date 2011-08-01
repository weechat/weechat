/*
 * Copyright (C) 2011 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * gui-cursor.c: functions for free movement of cursor (used by all GUI)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../core/weechat.h"
#include "gui-cursor.h"
#include "gui-bar.h"
#include "gui-bar-window.h"
#include "gui-buffer.h"
#include "gui-chat.h"
#include "gui-color.h"
#include "gui-focus.h"
#include "gui-input.h"
#include "gui-window.h"


int gui_cursor_mode = 0;               /* cursor mode? (free movement)      */
int gui_cursor_debug = 0;              /* debug mode (show info in input)   */
int gui_cursor_x = 0;                  /* position of cursor in cursor mode */
int gui_cursor_y = 0;                  /* position of cursor in cursor mode */


/*
 * gui_cursor_mode_toggle: toggle cursor mode
 */

void
gui_cursor_mode_toggle ()
{
    gui_cursor_mode ^= 1;
    
    if (gui_cursor_mode)
    {
        if (gui_cursor_debug)
            gui_input_delete_line (gui_current_window->buffer);
        gui_cursor_x = gui_window_cursor_x;
        gui_cursor_y = gui_window_cursor_y;
        gui_cursor_move_xy (0, 0);
    }
    else
    {
        /* restore input (and move cursor in input) */
        if (gui_cursor_debug)
            gui_input_delete_line (gui_current_window->buffer);
        gui_input_text_changed_modifier_and_signal (gui_current_window->buffer, 0);
        gui_buffer_ask_chat_refresh (gui_current_window->buffer, 2);
    }
}

/*
 * gui_cursor_debug_toggle: toggle debug for cursor mode
 */

void
gui_cursor_debug_toggle ()
{
    gui_cursor_debug ^= 1;

    if (gui_cursor_debug)
        gui_chat_printf (NULL, _("Debug enabled for cursor mode"));
    else
        gui_chat_printf (NULL, _("Debug disabled for cursor mode"));
}

/*
 * gui_cursor_display_debug_info: display debug info about (x,y) in input
 */

void
gui_cursor_display_debug_info ()
{
    struct t_gui_focus_info focus_info;
    char str_info[1024];
    
    if (!gui_cursor_debug)
        return;
    
    gui_focus_get_info (gui_cursor_x, gui_cursor_y, &focus_info);
    
    snprintf (str_info, sizeof (str_info),
              "%s(%d,%d) window:0x%lx (buffer: %s), chat: %d, "
              "bar_window:0x%lx (bar: %s, item: %s, line: %d, col: %d)",
              gui_color_get_custom ("yellow,red"),
              focus_info.x, focus_info.y,
              (long unsigned int)focus_info.window,
              (focus_info.window) ? (focus_info.window)->buffer->name : "-",
              focus_info.chat,
              (long unsigned int)focus_info.bar_window,
              (focus_info.bar_window) ? (focus_info.bar_window)->bar->name : "-",
              (focus_info.bar_item) ? focus_info.bar_item : "-",
              focus_info.item_line,
              focus_info.item_col);
    gui_input_delete_line (gui_current_window->buffer);
    gui_input_insert_string (gui_current_window->buffer, str_info, -1);
}

/*
 * gui_cursor_move_xy: set cursor at position (x,y)
 */

void
gui_cursor_move_xy (int x, int y)
{
    if (!gui_cursor_mode)
        gui_cursor_mode_toggle ();
    
    gui_cursor_x = x;
    gui_cursor_y = y;
    
    if (gui_cursor_x < 0)
        gui_cursor_x = 0;
    else if (gui_cursor_x > gui_window_get_width () - 1)
        gui_cursor_x = gui_window_get_width () - 1;
    
    if (gui_cursor_y < 0)
        gui_cursor_y = 0;
    else if (gui_cursor_y > gui_window_get_height () - 1)
        gui_cursor_y = gui_window_get_height () - 1;
    
    gui_cursor_display_debug_info ();
    gui_window_move_cursor ();
}

/*
 * gui_cursor_move_add_xy: move cursor by adding values to (x,y)
 */

void
gui_cursor_move_add_xy (int add_x, int add_y)
{
    if (!gui_cursor_mode)
        gui_cursor_mode_toggle ();
    
    gui_cursor_x += add_x;
    gui_cursor_y += add_y;
    
    if (gui_cursor_x < 0)
        gui_cursor_x = gui_window_get_width () - 1;
    else if (gui_cursor_x > gui_window_get_width () - 1)
        gui_cursor_x = 0;
    
    if (gui_cursor_y < 0)
        gui_cursor_y = gui_window_get_height () - 1;
    else if (gui_cursor_y > gui_window_get_height () - 1)
        gui_cursor_y = 0;
    
    gui_cursor_display_debug_info ();
    gui_window_move_cursor ();
}

/*
 * gui_cursor_move_area_add_xy: move cursor to another area by adding values to
 *                              (x,y)
 */

void
gui_cursor_move_area_add_xy (int add_x, int add_y)
{
    int x, y, width, height, area_found;
    struct t_gui_focus_info focus_info_old, focus_info_new;
    
    if (!gui_cursor_mode)
        gui_cursor_mode_toggle ();
    
    area_found = 0;
    
    x = gui_cursor_x;
    y = gui_cursor_y;
    width = gui_window_get_width ();
    height = gui_window_get_height ();
    
    gui_focus_get_info (x, y, &focus_info_old);
    
    if (add_x != 0)
        x += add_x;
    else
        y += add_y;
    
    while ((x >= 0) && (x < width) && (y >= 0) && (y < height))
    {
        gui_focus_get_info (x, y, &focus_info_new);
        if (((focus_info_new.window && focus_info_new.chat)
             || focus_info_new.bar_window)
            && ((focus_info_old.window != focus_info_new.window)
                || (focus_info_old.bar_window != focus_info_new.bar_window)))
        {
            area_found = 1;
            break;
        }
        
        if (add_x != 0)
            x += add_x;
        else
            y += add_y;
    }
    
    if (area_found)
    {
        if (focus_info_new.window && focus_info_new.chat)
        {
            x = (focus_info_new.window)->win_chat_x;
            y = (focus_info_new.window)->win_chat_y;
        }
        else if (focus_info_new.bar_window)
        {
            x = (focus_info_new.bar_window)->x;
            y = (focus_info_new.bar_window)->y;
        }
        else
            area_found = 0;
    }
    
    if (area_found)
    {
        gui_cursor_x = x;
        gui_cursor_y = y;
        gui_cursor_display_debug_info ();
        gui_window_move_cursor ();
    }
}

/*
 * gui_cursor_move_area: move cursor to another area by name
 */

void
gui_cursor_move_area (const char *area)
{
    int area_found, x, y;
    struct t_gui_bar_window *ptr_bar_win;
    struct t_gui_bar *ptr_bar;
    
    area_found = 0;
    x = 0;
    y = 0;
    
    if (strcmp (area, "chat") == 0)
    {
        area_found = 1;
        x = gui_current_window->win_chat_x;
        y = gui_current_window->win_chat_y;
    }
    else
    {
        for (ptr_bar_win = gui_current_window->bar_windows; ptr_bar_win;
             ptr_bar_win = ptr_bar_win->next_bar_window)
        {
            if (strcmp (ptr_bar_win->bar->name, area) == 0)
            {
                area_found = 1;
                x = ptr_bar_win->x;
                y = ptr_bar_win->y;
                break;
            }
        }
        if (!area_found)
        {
            for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
            {
                if (ptr_bar->bar_window && (strcmp (ptr_bar->name, area) == 0))
                {
                    area_found = 1;
                    x = ptr_bar->bar_window->x;
                    y = ptr_bar->bar_window->y;
                }
            }
        }
    }
    
    if (area_found)
    {
        if (!gui_cursor_mode)
            gui_cursor_mode_toggle ();
        gui_cursor_x = x;
        gui_cursor_y = y;
        gui_cursor_display_debug_info ();
        gui_window_move_cursor ();
    }
}
