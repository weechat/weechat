/*
 * gui-cursor.c - functions for free movement of cursor (used by all GUI)
 *
 * Copyright (C) 2011-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdio.h>
#include <string.h>

#include "../core/weechat.h"
#include "../core/wee-hook.h"
#include "../plugins/plugin.h"
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
int gui_cursor_debug = 0;              /* debug mode (0-2)                  */
int gui_cursor_x = 0;                  /* position of cursor in cursor mode */
int gui_cursor_y = 0;                  /* position of cursor in cursor mode */


/*
 * Toggles cursor mode.
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
        gui_input_text_changed_modifier_and_signal (gui_current_window->buffer,
                                                    0, /* save undo */
                                                    1); /* stop completion */
        gui_buffer_ask_chat_refresh (gui_current_window->buffer, 2);
    }

    (void) hook_signal_send (
        (gui_cursor_mode) ? "cursor_start" : "cursor_stop",
        WEECHAT_HOOK_SIGNAL_STRING,
        NULL);
}

/*
 * Stops cursor mode.
 */

void
gui_cursor_mode_stop ()
{
    if (gui_cursor_mode)
        gui_cursor_mode_toggle ();
}

/*
 * Sets debug for cursor mode.
 */

void
gui_cursor_debug_set (int debug)
{
    gui_cursor_debug = debug;

    if (gui_cursor_debug)
    {
        gui_chat_printf (NULL, _("Debug enabled for cursor mode (%s)"),
                         (debug > 1) ? _("verbose") : _("normal"));
    }
    else
        gui_chat_printf (NULL, _("Debug disabled for cursor mode"));
}

/*
 * Displays debug info about (x,y) in input.
 */

void
gui_cursor_display_debug_info ()
{
    struct t_gui_focus_info *focus_info;
    char str_info[1024];

    if (!gui_cursor_debug)
        return;

    focus_info = gui_focus_get_info (gui_cursor_x, gui_cursor_y);
    if (focus_info)
    {
        snprintf (str_info, sizeof (str_info),
                  "%s(%d,%d) window:0x%lx, buffer:0x%lx (%s), "
                  "bar_window:0x%lx (bar: %s, item: %s, line: %d, col: %d), "
                  "chat: %d, word: \"%s\"",
                  gui_color_get_custom ("yellow,red"),
                  focus_info->x, focus_info->y,
                  (unsigned long)focus_info->window,
                  (unsigned long)focus_info->buffer,
                  (focus_info->buffer) ? (focus_info->buffer)->full_name : "-",
                  (unsigned long)focus_info->bar_window,
                  (focus_info->bar_window) ? ((focus_info->bar_window)->bar)->name : "-",
                  (focus_info->bar_item) ? focus_info->bar_item : "-",
                  focus_info->bar_item_line,
                  focus_info->bar_item_col,
                  focus_info->chat,
                  focus_info->chat_word);
        gui_input_delete_line (gui_current_window->buffer);
        gui_input_insert_string (gui_current_window->buffer, str_info);
        gui_focus_free_info (focus_info);
    }
}

/*
 * Sets cursor at position (x,y).
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
 * Moves cursor by adding values to (x,y).
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
 * Moves cursor to another area by adding values to (x,y).
 */

void
gui_cursor_move_area_add_xy (int add_x, int add_y)
{
    int x, y, width, height, area_found;
    struct t_gui_focus_info *focus_info_old, *focus_info_new;

    if (!gui_cursor_mode)
        gui_cursor_mode_toggle ();

    area_found = 0;

    x = gui_cursor_x;
    y = gui_cursor_y;
    width = gui_window_get_width ();
    height = gui_window_get_height ();

    focus_info_old = gui_focus_get_info (x, y);
    if (!focus_info_old)
        return;
    focus_info_new = NULL;

    if (add_x != 0)
        x += add_x;
    else
        y += add_y;

    while ((x >= 0) && (x < width) && (y >= 0) && (y < height))
    {
        focus_info_new = gui_focus_get_info (x, y);
        if (!focus_info_new)
        {
            gui_focus_free_info (focus_info_old);
            return;
        }
        if (((focus_info_new->window && focus_info_new->chat)
             || focus_info_new->bar_window)
            && ((focus_info_old->window != focus_info_new->window)
                || (focus_info_old->bar_window != focus_info_new->bar_window)))
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
        if (focus_info_new->window && focus_info_new->chat)
        {
            x = (focus_info_new->window)->win_chat_x;
            y = (focus_info_new->window)->win_chat_y;
        }
        else if (focus_info_new->bar_window)
        {
            x = (focus_info_new->bar_window)->x;
            y = (focus_info_new->bar_window)->y;
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

    gui_focus_free_info (focus_info_old);
    if (focus_info_new)
        gui_focus_free_info (focus_info_new);
}

/*
 * Moves cursor to another area by name.
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
