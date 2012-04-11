/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * gui-gtk-window.c: window display functions for Gtk GUI
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-hook.h"
#include "../../core/wee-log.h"
#include "../../plugins/plugin.h"
#include "../gui-window.h"
#include "../gui-bar.h"
#include "../gui-buffer.h"
#include "../gui-chat.h"
#include "../gui-hotlist.h"
#include "../gui-line.h"
#include "../gui-nicklist.h"
#include "../gui-main.h"
#include "gui-gtk.h"


/*
 * gui_window_get_width: get screen width
 */

int
gui_window_get_width ()
{
    return 0;
}

/*
 * gui_window_get_height: get screen height
 */

int
gui_window_get_height ()
{
    return 0;
}

/*
 * gui_window_objects_init: init Gtk widgets
 */

int
gui_window_objects_init (struct t_gui_window *window)
{
    struct t_gui_window_gtk_objects *new_objects;

    if ((new_objects = malloc (sizeof (*new_objects))))
    {
        window->gui_objects = new_objects;
        GUI_WINDOW_OBJECTS(window)->textview_chat = NULL;
        GUI_WINDOW_OBJECTS(window)->textbuffer_chat = NULL;
        GUI_WINDOW_OBJECTS(window)->texttag_chat = NULL;
        return 1;
    }
    return 0;
}

/*
 * gui_window_objects_free: free Gtk widgets for a window
 */

void
gui_window_objects_free (struct t_gui_window *window, int free_separator)
{
    /* TODO: write this function for Gtk */
    (void) window;
    (void) free_separator;
}

/*
 * gui_window_set_weechat_color: set WeeChat color for window
 */

/* TODO: write this function for Gtk */
/*void
gui_window_set_weechat_color (WINDOW *window, int num_color)
{
    if ((num_color >= 0) && (num_color <= GUI_NUM_COLORS - 1))
    {
        wattroff (window, A_BOLD | A_UNDERLINE | A_REVERSE);
        wattron (window, COLOR_PAIR(gui_color_get_pair (num_color)) |
                 gui_color[num_color]->attributes);
    }
}*/

/*
 * gui_window_calculate_pos_size: calculate position and size for a window & sub-win
 */

void
gui_window_calculate_pos_size (struct t_gui_window *window)
{
    /* TODO: write this function for Gtk */
    (void) window;
}

/*
 * gui_window_draw_separator: draw window separation
 */

void
gui_window_draw_separator (struct t_gui_window *window)
{
    /* TODO: write this function for Gtk */
    /*if (window->win_separator)
        delwin (window->win_separator);

    if (window->win_x > 0)
    {
        window->win_separator = newwin (window->win_height,
                                        1,
                                        window->win_y,
                                        window->win_x - 1);
        gui_window_set_weechat_color (window->win_separator, COLOR_WIN_SEPARATOR);
        wborder (window->win_separator, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        wnoutrefresh (window->win_separator);
        refresh ();
    }*/
    (void) window;
}

/*
 * gui_window_redraw_buffer: redraw a buffer
 */

void
gui_window_redraw_buffer (struct t_gui_buffer *buffer)
{
    /* TODO: write this function for Gtk */
    (void) buffer;
}

/*
 * gui_window_redraw_all_buffers: redraw all buffers
 */

void
gui_window_redraw_all_buffers ()
{
    /* TODO: write this function for Gtk */
}

/*
 * gui_window_switch: switch to another window
 */

void
gui_window_switch (struct t_gui_window *window)
{
    if (gui_current_window == window)
        return;

    /* remove unused bars from current window */
    /* ... */

    gui_current_window = window;

    gui_window_switch_to_buffer (gui_current_window, gui_current_window->buffer, 1);

    gui_window_redraw_buffer (gui_current_window->buffer);

    hook_signal_send ("window_switch",
                      WEECHAT_HOOK_SIGNAL_POINTER, window);
}

/*
 * gui_window_switch_to_buffer: switch to another buffer
 */

void
gui_window_switch_to_buffer (struct t_gui_window *window,
                             struct t_gui_buffer *buffer,
                             int set_last_read)
{
    GtkTextIter start, end;

    gui_buffer_add_value_num_displayed (window->buffer, -1);

    if (window->buffer != buffer)
    {
        window->scroll->start_line = NULL;
        window->scroll->start_line_pos = 0;
        if (!gui_buffers_visited_frozen)
        {
            gui_buffer_visited_add (window->buffer);
            gui_buffer_visited_add (buffer);
        }
        if (set_last_read)
        {
            if (window->buffer->num_displayed == 0)
            {
                window->buffer->lines->last_read_line = window->buffer->lines->last_line;
                window->buffer->lines->first_line_not_read = 0;
            }
            if (buffer->lines->last_read_line == buffer->lines->last_line)
            {
                buffer->lines->last_read_line = NULL;
                buffer->lines->first_line_not_read = 0;
            }
        }
    }

    window->buffer = buffer;
    gui_window_calculate_pos_size (window);

    if (!GUI_WINDOW_OBJECTS(window)->textview_chat)
    {
        GUI_WINDOW_OBJECTS(window)->textview_chat = gtk_text_view_new ();
        gtk_widget_show (GUI_WINDOW_OBJECTS(window)->textview_chat);
        gtk_container_add (GTK_CONTAINER (gui_gtk_scrolledwindow_chat), GUI_WINDOW_OBJECTS(window)->textview_chat);
        gtk_widget_set_size_request (GUI_WINDOW_OBJECTS(window)->textview_chat, 300, -1);
        gtk_text_view_set_editable (GTK_TEXT_VIEW (GUI_WINDOW_OBJECTS(window)->textview_chat), FALSE);
        gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (GUI_WINDOW_OBJECTS(window)->textview_chat), FALSE);

        GUI_WINDOW_OBJECTS(window)->textbuffer_chat = gtk_text_buffer_new (NULL);
        gtk_text_view_set_buffer (GTK_TEXT_VIEW (GUI_WINDOW_OBJECTS(window)->textview_chat), GUI_WINDOW_OBJECTS(window)->textbuffer_chat);

        /*GUI_WINDOW_OBJECTS(window)->texttag_chat = gtk_text_buffer_create_tag(GUI_WINDOW_OBJECTS(window)->textbuffer_chat, "courier", "font_family", "lucida");*/
        gtk_text_buffer_get_bounds (GUI_WINDOW_OBJECTS(window)->textbuffer_chat, &start, &end);
        gtk_text_buffer_apply_tag (GUI_WINDOW_OBJECTS(window)->textbuffer_chat, GUI_WINDOW_OBJECTS(window)->texttag_chat, &start, &end);
    }

    window->scroll->start_line = NULL;
    window->scroll->start_line_pos = 0;

    gui_buffer_add_value_num_displayed (buffer, 1);

    gui_hotlist_remove_buffer (buffer);
}

/*
 * gui_window_page_up: display previous page on buffer
 */

void
gui_window_page_up (struct t_gui_window *window)
{
    if (!gui_init_ok)
        return;

    if (!window->scroll->first_line_displayed)
    {
        gui_chat_calculate_line_diff (window, &window->scroll->start_line,
                                      &window->scroll->start_line_pos,
                                      (window->scroll->start_line) ?
                                      (-1) * (window->win_chat_height - 1) :
                                      (-1) * ((window->win_chat_height - 1) * 2));
        gui_chat_draw (window->buffer, 0);
    }
}

/*
 * gui_window_page_down: display next page on buffer
 */

void
gui_window_page_down (struct t_gui_window *window)
{
    struct t_gui_line *ptr_line;
    int line_pos;

    if (!gui_init_ok)
        return;

    if (window->scroll->start_line)
    {
        gui_chat_calculate_line_diff (window, &window->scroll->start_line,
                                      &window->scroll->start_line_pos,
                                      window->win_chat_height - 1);

        /* check if we can display all */
        ptr_line = window->scroll->start_line;
        line_pos = window->scroll->start_line_pos;
        gui_chat_calculate_line_diff (window, &ptr_line,
                                      &line_pos,
                                      window->win_chat_height - 1);
        if (!ptr_line)
        {
            window->scroll->start_line = NULL;
            window->scroll->start_line_pos = 0;
        }

        gui_chat_draw (window->buffer, 0);
    }
}

/*
 * gui_window_scroll_up: display previous few lines in buffer
 */

void
gui_window_scroll_up (struct t_gui_window *window)
{
    if (!gui_init_ok)
        return;

    if (!window->scroll->first_line_displayed)
    {
        gui_chat_calculate_line_diff (window, &window->scroll->start_line,
                                      &window->scroll->start_line_pos,
                                      (window->scroll->start_line) ?
                                      (-1) * CONFIG_INTEGER(config_look_scroll_amount) :
                                      (-1) * ( (window->win_chat_height - 1) +
                                               CONFIG_INTEGER(config_look_scroll_amount)));
        gui_chat_draw (window->buffer, 0);
    }
}

/*
 * gui_window_scroll_down: display next few lines in buffer
 */

void
gui_window_scroll_down (struct t_gui_window *window)
{
    struct t_gui_line *ptr_line;
    int line_pos;

    if (!gui_init_ok)
        return;

    if (window->scroll->start_line)
    {
        gui_chat_calculate_line_diff (window, &window->scroll->start_line,
                                      &window->scroll->start_line_pos,
                                      CONFIG_INTEGER(config_look_scroll_amount));

        /* check if we can display all */
        ptr_line = window->scroll->start_line;
        line_pos = window->scroll->start_line_pos;
        gui_chat_calculate_line_diff (window, &ptr_line,
                                      &line_pos,
                                      window->win_chat_height - 1);

        if (!ptr_line)
        {
            window->scroll->start_line = NULL;
            window->scroll->start_line_pos = 0;
        }

        gui_chat_draw (window->buffer, 0);
    }
}

/*
 * gui_window_scroll_top: scroll to top of buffer
 */

void
gui_window_scroll_top (struct t_gui_window *window)
{
    if (!gui_init_ok)
        return;

    if (!window->scroll->first_line_displayed)
    {
        window->scroll->start_line = window->buffer->lines->first_line;
        window->scroll->start_line_pos = 0;
        gui_chat_draw (window->buffer, 0);
    }
}

/*
 * gui_window_scroll_bottom: scroll to bottom of buffer
 */

void
gui_window_scroll_bottom (struct t_gui_window *window)
{
    if (!gui_init_ok)
        return;

    if (window->scroll->start_line)
    {
        window->scroll->start_line = NULL;
        window->scroll->start_line_pos = 0;
        gui_chat_draw (window->buffer, 0);
    }
}

/*
 * gui_window_auto_resize: auto-resize all windows, according to % of global size
 *                         This function is called after a terminal resize.
 *                         Returns 0 if ok, -1 if all window should be merged
 *                         (not enough space according to windows %)
 */

int
gui_window_auto_resize (struct t_gui_window_tree *tree,
                        int x, int y, int width, int height,
                        int simulate)
{
    int size1, size2;

    if (tree)
    {
        if (tree->window)
        {
            if (!simulate)
            {
                tree->window->win_x = x;
                tree->window->win_y = y;
                tree->window->win_width = width;
                tree->window->win_height = height;
            }
        }
        else
        {
            if (tree->split_horizontal)
            {
                size1 = (height * tree->split_pct) / 100;
                size2 = height - size1;
                if (gui_window_auto_resize (tree->child1, x, y + size1,
                                            width, size2, simulate) < 0)
                    return -1;
                if (gui_window_auto_resize (tree->child2, x, y,
                                            width, size1, simulate) < 0)
                    return -1;
            }
            else
            {
                size1 = (width * tree->split_pct) / 100;
                size2 = width - size1 - 1;
                if (gui_window_auto_resize (tree->child1, x, y,
                                            size1, height, simulate) < 0)
                    return -1;
                if (gui_window_auto_resize (tree->child2, x + size1 + 1, y,
                                            size2, height, simulate) < 0)
                    return -1;
            }
        }
    }
    return 0;
}

/*
 * gui_window_refresh_windows: auto resize and refresh all windows
 */

void
gui_window_refresh_windows ()
{
    /*struct t_gui_window *ptr_win, *old_current_window;*/

    if (gui_init_ok)
    {
        /* TODO: write this function for Gtk */
    }
}

/*
 * gui_window_split_horizontal: split a window horizontally
 */

struct t_gui_window *
gui_window_split_horizontal (struct t_gui_window *window, int percentage)
{
    struct t_gui_window *new_window;
    int height1, height2;

    if (!gui_init_ok)
        return NULL;

    new_window = NULL;

    height1 = (window->win_height * percentage) / 100;
    height2 = window->win_height - height1;

    if ((percentage > 0) && (percentage <= 100))
    {
        new_window = gui_window_new (window, window->buffer,
                                     window->win_x, window->win_y,
                                     window->win_width, height1,
                                     100, percentage);
        if (new_window)
        {
            /* reduce old window height (bottom window) */
            window->win_y = new_window->win_y + new_window->win_height;
            window->win_height = height2;
            window->win_height_pct = 100 - percentage;

            /* assign same buffer for new window (top window) */
            gui_buffer_add_value_num_displayed (new_window->buffer, 1);

            gui_window_switch_to_buffer (window, window->buffer, 1);

            gui_current_window = new_window;
            gui_window_switch_to_buffer (gui_current_window, gui_current_window->buffer, 1);
            gui_window_redraw_buffer (gui_current_window->buffer);
        }
    }

    return new_window;
}

/*
 * gui_window_split_vertical: split a window vertically
 */

struct t_gui_window *
gui_window_split_vertical (struct t_gui_window *window, int percentage)
{
    struct t_gui_window *new_window;
    int width1, width2;

    if (!gui_init_ok)
        return NULL;

    new_window = NULL;

    width1 = (window->win_width * percentage) / 100;
    width2 = window->win_width - width1 - 1;

    if ((percentage > 0) && (percentage <= 100))
    {
        new_window = gui_window_new (window, window->buffer,
                                     window->win_x + width1 + 1, window->win_y,
                                     width2, window->win_height,
                                     percentage, 100);
        if (new_window)
        {
            /* reduce old window height (left window) */
            window->win_width = width1;
            window->win_width_pct = 100 - percentage;

            /* assign same buffer for new window (right window) */
            gui_buffer_add_value_num_displayed (new_window->buffer, 1);

            gui_window_switch_to_buffer (window, window->buffer, 1);

            gui_current_window = new_window;
            gui_window_switch_to_buffer (gui_current_window, gui_current_window->buffer, 1);
            gui_window_redraw_buffer (gui_current_window->buffer);

            /* create & draw separator */
            gui_window_draw_separator (gui_current_window);
        }
    }

    return new_window;
}

/*
 * gui_window_resize: resize window
 */

void
gui_window_resize (struct t_gui_window *window, int percentage)
{
    /* TODO: write this function for Gtk */
    (void) window;
    (void) percentage;
}

/*
 * gui_window_resize_delta: resize window using delta percentage
 */

void
gui_window_resize_delta (struct t_gui_window *window, int delta_percentage)
{
    /* TODO: write this function for Gtk */
    (void) window;
    (void) delta_percentage;
}

/*
 * gui_window_merge: merge window with its sister
 */

int
gui_window_merge (struct t_gui_window *window)
{
    struct t_gui_window_tree *parent, *sister;

    parent = window->ptr_tree->parent_node;
    if (parent)
    {
        sister = (parent->child1->window == window) ?
            parent->child2 : parent->child1;

        if (!(sister->window))
            return 0;

        if (window->win_y == sister->window->win_y)
        {
            /* horizontal merge */
            window->win_width += sister->window->win_width + 1;
            window->win_width_pct += sister->window->win_width_pct;
        }
        else
        {
            /* vertical merge */
            window->win_height += sister->window->win_height;
            window->win_height_pct += sister->window->win_height_pct;
        }
        if (sister->window->win_x < window->win_x)
            window->win_x = sister->window->win_x;
        if (sister->window->win_y < window->win_y)
            window->win_y = sister->window->win_y;

        gui_window_free (sister->window);
        gui_window_tree_node_to_leaf (parent, window);

        gui_window_switch_to_buffer (window, window->buffer, 1);
        gui_window_redraw_buffer (window->buffer);
        return 1;
    }
    return 0;
}

/*
 * gui_window_merge_all: merge all windows into only one
 */

void
gui_window_merge_all (struct t_gui_window *window)
{
    /* TODO: write this function for Gtk */
    (void) window;
}

/*
 * gui_window_side_by_side: return a code about position of 2 windows:
 *                          0 = they're not side by side
 *                          1 = side by side (win2 is over the win1)
 *                          2 = side by side (win2 on the right)
 *                          3 = side by side (win2 below win1)
 *                          4 = side by side (win2 on the left)
 */

int
gui_window_side_by_side (struct t_gui_window *win1, struct t_gui_window *win2)
{
    /* win2 over win1 ? */
    if (win2->win_y + win2->win_height == win1->win_y)
    {
        if (win2->win_x >= win1->win_x + win1->win_width)
            return 0;
        if (win2->win_x + win2->win_width <= win1->win_x)
            return 0;
        return 1;
    }

    /* win2 on the right ? */
    if (win2->win_x == win1->win_x + win1->win_width + 1)
    {
        if (win2->win_y >= win1->win_y + win1->win_height)
            return 0;
        if (win2->win_y + win2->win_height <= win1->win_y)
            return 0;
        return 2;
    }

    /* win2 below win1 ? */
    if (win2->win_y == win1->win_y + win1->win_height)
    {
        if (win2->win_x >= win1->win_x + win1->win_width)
            return 0;
        if (win2->win_x + win2->win_width <= win1->win_x)
            return 0;
        return 3;
    }

    /* win2 on the left ? */
    if (win2->win_x + win2->win_width + 1 == win1->win_x)
    {
        if (win2->win_y >= win1->win_y + win1->win_height)
            return 0;
        if (win2->win_y + win2->win_height <= win1->win_y)
            return 0;
        return 4;
    }

    return 0;
}

/*
 * gui_window_switch_up: search and switch to a window over current window
 */

void
gui_window_switch_up (struct t_gui_window *window)
{
    struct t_gui_window *ptr_win;

    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 1))
        {
            gui_current_window = ptr_win;
            gui_window_switch_to_buffer (gui_current_window, gui_current_window->buffer, 1);
            gui_window_redraw_buffer (gui_current_window->buffer);
            return;
        }
    }
}

/*
 * gui_window_switch_down: search and switch to a window below current window
 */

void
gui_window_switch_down (struct t_gui_window *window)
{
    struct t_gui_window *ptr_win;

    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 3))
        {
            gui_current_window = ptr_win;
            gui_window_switch_to_buffer (gui_current_window, gui_current_window->buffer, 1);
            gui_window_redraw_buffer (gui_current_window->buffer);
            return;
        }
    }
}

/*
 * gui_window_switch_left: search and switch to a window on the left of current window
 */

void
gui_window_switch_left (struct t_gui_window *window)
{
    struct t_gui_window *ptr_win;

    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 4))
        {
            gui_current_window = ptr_win;
            gui_window_switch_to_buffer (gui_current_window, gui_current_window->buffer, 1);
            gui_window_redraw_buffer (gui_current_window->buffer);
            return;
        }
    }
}

/*
 * gui_window_switch_right: search and switch to a window on the right of current window
 */

void
gui_window_switch_right (struct t_gui_window *window)
{
    struct t_gui_window *ptr_win;

    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 2))
        {
            gui_current_window = ptr_win;
            gui_window_switch_to_buffer (gui_current_window, gui_current_window->buffer, 1);
            gui_window_redraw_buffer (gui_current_window->buffer);
            return;
        }
    }
}

/*
 * gui_window_balance: balance windows (set all splits to 50%)
 *                     return 1 if some windows have been balanced
 *                            0 if nothing was changed
 */

int
gui_window_balance (struct t_gui_window_tree *tree)
{
    (void) tree;

    /* TODO: write this function for Gtk */
    return 0;
}

/*
 * gui_window_swap: swap buffer of two windows
 *                  direction can be: 0 = auto (swap with sister)
 *                                    1 = window above
 *                                    2 = window on the right
 *                                    3 = window below
 *                                    4 = window on the left
 */

void
gui_window_swap (struct t_gui_window *window, int direction)
{
    (void) window;
    (void) direction;

    /* TODO: write this function for Gtk */
}

/*
 * gui_window_refresh_screen: called when term size is modified
 */

void
gui_window_refresh_screen (int full_refresh)
{
    (void) full_refresh;

    /* TODO: write this function for Gtk */
}

/*
 * gui_window_set_title: set terminal title
 */

void
gui_window_set_title (const char *title)
{
    (void) title;

    /* TODO: write this function for Gtk */
}

/*
 * gui_window_send_clipboard: copy text to clipboard (sent to terminal)
 */

void
gui_window_send_clipboard (const char *storage_unit, const char *text)
{
    (void) storage_unit;
    (void) text;

    /* TODO: write this function for Gtk */
}

/*
 * gui_window_set_bracketed_paste_mode: enable/disable bracketed paste mode
 */

void
gui_window_set_bracketed_paste_mode (int enable)
{
    (void) enable;

    /* TODO: write this function for Gtk */
}

/*
 * gui_window_move_cursor: move cursor on screen (for cursor mode)
 */

void
gui_window_move_cursor ()
{
    /* TODO: write this function for Gtk */
}

/*
 * gui_window_term_display_infos: display some infos about terminal and colors
 */

void
gui_window_term_display_infos ()
{
    /* No term info for Gtk */
}

/*
 * gui_window_objects_print_log: print Gtk objects infos in log
 *                               (usually for crash dump)
 */

void
gui_window_objects_print_log (struct t_gui_window *window)
{
    log_printf ("  window specific objects for Gtk:");
    log_printf ("    textview_chat . . . : 0x%lx", GUI_WINDOW_OBJECTS(window)->textview_chat);
    log_printf ("    textbuffer_chat . . : 0x%lx", GUI_WINDOW_OBJECTS(window)->textbuffer_chat);
    log_printf ("    texttag_chat. . . . : 0x%lx", GUI_WINDOW_OBJECTS(window)->texttag_chat);
    log_printf ("    bar_windows . . . . : 0x%lx", GUI_WINDOW_OBJECTS(window)->bar_windows);
    log_printf ("    last_bar_windows. . : 0x%lx", GUI_WINDOW_OBJECTS(window)->last_bar_window);
    log_printf ("    current_style_fg. . : %d",    GUI_WINDOW_OBJECTS(window)->current_style_fg);
    log_printf ("    current_style_bg. . : %d",    GUI_WINDOW_OBJECTS(window)->current_style_bg);
    log_printf ("    current_style_attr. : %d",    GUI_WINDOW_OBJECTS(window)->current_style_attr);
    log_printf ("    current_color_attr. : %d",    GUI_WINDOW_OBJECTS(window)->current_color_attr);
}
