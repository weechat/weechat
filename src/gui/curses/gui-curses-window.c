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

/* gui-curses-window.c: window display functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-hook.h"
#include "../../core/wee-log.h"
#include "../../core/wee-string.h"
#include "../../plugins/plugin.h"
#include "../gui-window.h"
#include "../gui-bar.h"
#include "../gui-bar-window.h"
#include "../gui-buffer.h"
#include "../gui-chat.h"
#include "../gui-color.h"
#include "../gui-hotlist.h"
#include "../gui-input.h"
#include "../gui-main.h"
#include "../gui-nicklist.h"
#include "gui-curses.h"


int window_current_style_fg;           /* current foreground color          */
int window_current_style_bg;           /* current background color          */
int window_current_style_attr;         /* current attributes (bold, ..)     */
int window_current_color_attr;         /* attr sum of last color(s) used    */


/*
 * gui_window_get_width: get screen width (terminal width in chars for Curses)
 */

int
gui_window_get_width ()
{
    return COLS;
}

/*
 * gui_window_get_height: get screen height (terminal height in chars for Curses)
 */

int
gui_window_get_height ()
{
    return LINES;
}

/*
 * gui_window_objects_init: init Curses windows
 */

int
gui_window_objects_init (struct t_gui_window *window)
{
    struct t_gui_window_curses_objects *new_objects;
    
    new_objects = malloc (sizeof (*new_objects));
    if (new_objects)
    {
        window->gui_objects = new_objects;
        GUI_WINDOW_OBJECTS(window)->win_chat = NULL;
        GUI_WINDOW_OBJECTS(window)->win_separator = NULL;
        return 1;
    }
    return 0;
}

/*
 * gui_window_objects_free: free Curses windows for a window
 */

void
gui_window_objects_free (struct t_gui_window *window, int free_separator)
{
    if (GUI_WINDOW_OBJECTS(window)->win_chat)
    {
        delwin (GUI_WINDOW_OBJECTS(window)->win_chat);
        GUI_WINDOW_OBJECTS(window)->win_chat = NULL;
    }
    if (free_separator && GUI_WINDOW_OBJECTS(window)->win_separator)
    {
        delwin (GUI_WINDOW_OBJECTS(window)->win_separator);
        GUI_WINDOW_OBJECTS(window)->win_separator = NULL;
    }
}

/*
 * gui_window_utf_char_valid: return 1 if utf char is valid for screen
 *                            otherwise return 0
 */

int
gui_window_utf_char_valid (const char *utf_char)
{
    /* 146 or 0x7F are not valid */
    if ((((unsigned char)(utf_char[0]) == 146)
         || ((unsigned char)(utf_char[0]) == 0x7F))
        && (!utf_char[1]))
        return 0;
    
    /* any other char is valid */
    return 1;
}

/*
 * gui_window_wprintw: decode then display string with wprintw
 */

void
gui_window_wprintw (WINDOW *window, const char *data, ...)
{
    va_list argptr;
    static char buf[4096];
    char *buf2;
    
    va_start (argptr, data);
    vsnprintf (buf, sizeof (buf) - 1, data, argptr);
    va_end (argptr);
    
    buf2 = string_iconv_from_internal (NULL, buf);
    wprintw (window, "%s", (buf2) ? buf2 : buf);
    if (buf2)
        free (buf2);
}

/*
 * gui_window_curses_clear_weechat: clear a Curses window with a weechat color
 */

void
gui_window_clear_weechat (WINDOW *window, int num_color)
{
    if (!gui_ok)
        return;
    
    wbkgdset (window, ' ' | COLOR_PAIR (gui_color_get_pair (num_color)));
    werase (window);
    wmove (window, 0, 0);
}

/*
 * gui_window_clear: clear a Curses window
 */

void
gui_window_clear (WINDOW *window, int bg)
{
    int color;
    
    if (!gui_ok)
        return;

    if ((bg >= 0) && (bg < GUI_CURSES_NUM_WEECHAT_COLORS))
    {
        color = gui_weechat_colors[bg].foreground;
        wbkgdset (window,
                  ' ' | COLOR_PAIR (((color == -1) || (color == 99)) ?
                                    63 : color * 8));
        werase (window);
        wmove (window, 0, 0);
    }
}

/*
 * gui_window_reset_style: reset style (color and attr) for a window
 */

void
gui_window_reset_style (WINDOW *window, int num_color)
{
    window_current_style_fg = -1;
    window_current_style_bg = -1;
    window_current_style_attr = 0;
    window_current_color_attr = 0;
    
    wattron (window, COLOR_PAIR(gui_color_get_pair (num_color)) |
             gui_color[num_color]->attributes);
    wattroff (window, A_BOLD | A_UNDERLINE | A_REVERSE);
}

/*
 * gui_window_set_color_style: set style for color
 */

void
gui_window_set_color_style (WINDOW *window, int style)
{
    window_current_color_attr |= style;
    wattron (window, style);
}

/*
 * gui_window_remove_color_style: remove style for color
 */

void
gui_window_remove_color_style (WINDOW *window, int style)
{
    window_current_color_attr &= !style;
    wattroff (window, style);
}

/*
 * gui_window_set_color: set color for a window
 */

void
gui_window_set_color (WINDOW *window, int fg, int bg)
{
    window_current_style_fg = fg;
    window_current_style_bg = bg;
    
    if (((fg == -1) || (fg == 99))
        && ((bg == -1) || (bg == 99)))
        wattron (window, COLOR_PAIR(63));
    else
    {
        if ((fg == -1) || (fg == 99))
            fg = COLOR_WHITE;
        if ((bg == -1) || (bg == 99))
            bg = 0;
        wattron (window, COLOR_PAIR((bg * 8) + fg));
    }
}

/*
 * gui_window_set_weechat_color: set WeeChat color for window
 */

void
gui_window_set_weechat_color (WINDOW *window, int num_color)
{
    if ((num_color >= 0) && (num_color < GUI_COLOR_NUM_COLORS))
    {
        /*
        wattroff (window, A_BOLD | A_UNDERLINE | A_REVERSE);
        wattron (window, COLOR_PAIR(gui_color_get_pair (num_color)) |
                 gui_color[num_color]->attributes);
        */
        gui_window_reset_style (window, num_color);
        wattron (window, gui_color[num_color]->attributes);
        gui_window_set_color (window,
                              gui_color[num_color]->foreground,
                              gui_color[num_color]->background);
    }
}

/*
 * gui_window_set_custom_color_fg_bg: set a custom color for a window
 *                                    (foreground and background)
 */

void
gui_window_set_custom_color_fg_bg (WINDOW *window, int fg, int bg)
{
    if ((fg >= 0) && (fg < GUI_CURSES_NUM_WEECHAT_COLORS)
        && (bg >= 0) && (bg < GUI_CURSES_NUM_WEECHAT_COLORS))
    {
        gui_window_remove_color_style (window, A_BOLD);
        wattron (window, gui_weechat_colors[fg].attributes);
        gui_window_set_color (window,
                              gui_weechat_colors[fg].foreground,
                              gui_weechat_colors[bg].foreground);
    }
}

/*
 * gui_window_set_custom_color_fg: set a custom color for a window
 *                                 (foreground only)
 */

void
gui_window_set_custom_color_fg (WINDOW *window, int fg)
{
    int current_attr, current_bg;
    
    if ((fg >= 0) && (fg < GUI_CURSES_NUM_WEECHAT_COLORS))
    {
        current_attr = window_current_style_attr;
        current_bg = window_current_style_bg;
        gui_window_remove_color_style (window, A_BOLD);
        gui_window_set_color_style (window, gui_weechat_colors[fg].attributes);
        gui_window_set_color (window,
                              gui_weechat_colors[fg].foreground,
                              current_bg);
    }
}

/*
 * gui_window_set_custom_color_bg: set a custom color for a window
 *                                 (background only)
 */

void
gui_window_set_custom_color_bg (WINDOW *window, int bg)
{
    int current_attr, current_fg;
    
    if ((bg >= 0) && (bg < GUI_CURSES_NUM_WEECHAT_COLORS))
    {
        current_attr = window_current_style_attr;
        current_fg = window_current_style_fg;
        gui_window_set_color_style (window, current_attr);
        gui_window_set_color (window, current_fg,
                              gui_weechat_colors[bg].foreground);
    }
}

/*
 * gui_window_clrtoeol_with_current_bg: clear until end of line with current bg
 */

void
gui_window_clrtoeol_with_current_bg (WINDOW *window)
{
    wbkgdset (window,
              ' ' | COLOR_PAIR ((window_current_style_bg < 0) ? 63 : window_current_style_bg * 8));
    wclrtoeol (window);
}

/*
 * gui_window_calculate_pos_size: calculate position and size for a buffer and
 *                                subwindows
 */

void
gui_window_calculate_pos_size (struct t_gui_window *window)
{
    struct t_gui_bar_window *ptr_bar_win;
    int add_top, add_bottom, add_left, add_right;

    if ((window->win_width < GUI_WINDOW_MIN_WIDTH)
        || (window->win_height < GUI_WINDOW_MIN_HEIGHT))
    {
        gui_ok = 0;
        return;
    }
    
    for (ptr_bar_win = window->bar_windows; ptr_bar_win;
         ptr_bar_win = ptr_bar_win->next_bar_window)
    {
        gui_bar_window_calculate_pos_size (ptr_bar_win, window);
    }
    
    add_bottom = gui_bar_window_get_size (NULL, window, GUI_BAR_POSITION_BOTTOM);
    add_top = gui_bar_window_get_size (NULL, window, GUI_BAR_POSITION_TOP);
    add_left = gui_bar_window_get_size (NULL, window, GUI_BAR_POSITION_LEFT);
    add_right = gui_bar_window_get_size (NULL, window, GUI_BAR_POSITION_RIGHT);
    
    window->win_chat_x = window->win_x + add_left;
    window->win_chat_y = window->win_y + add_top;
    window->win_chat_width = window->win_width - add_left - add_right;
    window->win_chat_height = window->win_height - add_top - add_bottom;
    window->win_chat_cursor_x = window->win_x + add_left;
    window->win_chat_cursor_y = window->win_y + add_top;

    if ((window->win_chat_width <= 1) || (window->win_chat_height <= 0))
        gui_ok = 0;
}

/*
 * gui_window_draw_separator: draw window separation
 */

void
gui_window_draw_separator (struct t_gui_window *window)
{
    if (GUI_WINDOW_OBJECTS(window)->win_separator)
        delwin (GUI_WINDOW_OBJECTS(window)->win_separator);
    
    if (window->win_x > gui_bar_root_get_size (NULL, GUI_BAR_POSITION_LEFT))
    {
        GUI_WINDOW_OBJECTS(window)->win_separator = newwin (window->win_height,
                                                            1,
                                                            window->win_y,
                                                            window->win_x - 1);
        gui_window_set_weechat_color (GUI_WINDOW_OBJECTS(window)->win_separator,
                                      GUI_COLOR_SEPARATOR);
        mvwvline (GUI_WINDOW_OBJECTS(window)->win_separator, 0, 0, ACS_VLINE,
                  window->win_height);
        wnoutrefresh (GUI_WINDOW_OBJECTS(window)->win_separator);
        refresh ();
    }
}

/*
 * gui_window_redraw_buffer: redraw a buffer
 */

void
gui_window_redraw_buffer (struct t_gui_buffer *buffer)
{
    if (!gui_ok)
        return;
    
    gui_chat_draw (buffer, 1);
}

/*
 * gui_window_redraw_all_buffers: redraw all buffers
 */

void
gui_window_redraw_all_buffers ()
{
    struct t_gui_buffer *ptr_buffer;

    if (!gui_ok)
        return;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_window_redraw_buffer (ptr_buffer);
    }
}

/*
 * gui_window_switch_to_buffer: switch to another buffer in a window
 */

void
gui_window_switch_to_buffer (struct t_gui_window *window,
                             struct t_gui_buffer *buffer,
                             int set_last_read)
{
    struct t_gui_bar_window *ptr_bar_win;
    struct t_gui_buffer *old_buffer;
    
    if (!gui_ok)
        return;
    
    if (window->buffer->num_displayed > 0)
        window->buffer->num_displayed--;
    
    old_buffer = window->buffer;
    
    if (window->buffer != buffer)
    {
        window->start_line = NULL;
        window->start_line_pos = 0;
        gui_previous_buffer = window->buffer;
        if (set_last_read)
        {
            if (window->buffer->num_displayed == 0)
                window->buffer->last_read_line = window->buffer->last_line;
            if (buffer->last_read_line == buffer->last_line)
                buffer->last_read_line = NULL;
        }
    }
    
    window->buffer = buffer;
    
    if (gui_ok && (old_buffer == buffer))
    {
        gui_bar_window_remove_unused_bars (window);
        gui_bar_window_add_missing_bars (window);
    }
    
    gui_window_calculate_pos_size (window);
    
    if (gui_ok)
    {
        /* create bar windows */
        for (ptr_bar_win = window->bar_windows; ptr_bar_win;
             ptr_bar_win = ptr_bar_win->next_bar_window)
        {
            gui_bar_window_create_win (ptr_bar_win);
        }
        
        /* destroy Curses windows */
        gui_window_objects_free (window, 0);
        
        /* create Curses windows */
        if (GUI_WINDOW_OBJECTS(window)->win_chat)
            delwin (GUI_WINDOW_OBJECTS(window)->win_chat);
        GUI_WINDOW_OBJECTS(window)->win_chat = newwin (window->win_chat_height,
                                                       window->win_chat_width,
                                                       window->win_chat_y,
                                                       window->win_chat_x);
    }
    
    buffer->num_displayed++;
    
    gui_hotlist_remove_buffer (buffer);
    
    if (gui_ok && (buffer != old_buffer))
    {
        gui_bar_window_remove_unused_bars (window);
        gui_bar_window_add_missing_bars (window);
    }
    
    /* redraw bars in window */
    for (ptr_bar_win = window->bar_windows; ptr_bar_win;
         ptr_bar_win = ptr_bar_win->next_bar_window)
    {
        ptr_bar_win->bar->bar_refresh_needed = 1;
    }
    
    if (window->buffer->type == GUI_BUFFER_TYPE_FREE)
    {
        window->scroll = 0;
        window->scroll_lines_after = 0;
    }
    
    window->refresh_needed = 1;
    
    hook_signal_send ("buffer_switch",
                      WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * gui_window_switch: switch to another window
 */

void
gui_window_switch (struct t_gui_window *window)
{
    struct t_gui_window *old_window;
    int changes;
    
    if (gui_current_window == window)
        return;
    
    old_window = gui_current_window;
    
    gui_current_window = window;
    changes = gui_bar_window_remove_unused_bars (old_window)
        || gui_bar_window_add_missing_bars (old_window);
    if (changes)
    {
        gui_current_window = old_window;
        gui_window_switch_to_buffer (gui_current_window,
                                     gui_current_window->buffer, 1);
        gui_current_window = window;
    }
    
    gui_window_switch_to_buffer (gui_current_window,
                                 gui_current_window->buffer, 1);
}

/*
 * gui_window_page_up: display previous page on buffer
 */

void
gui_window_page_up (struct t_gui_window *window)
{
    char scroll[32];
    int num_lines;
    
    if (!gui_ok)
        return;
    
    num_lines = ((window->win_chat_height - 1) *
                 CONFIG_INTEGER(config_look_scroll_page_percent)) / 100;
    if (num_lines < 1)
        num_lines = 1;
    else if (num_lines > window->win_chat_height - 1)
        num_lines = window->win_chat_height - 1;
    
    switch (window->buffer->type)
    {
        case GUI_BUFFER_TYPE_FORMATED:
            if (!window->first_line_displayed)
            {
                gui_chat_calculate_line_diff (window, &window->start_line,
                                              &window->start_line_pos,
                                              (window->start_line) ?
                                              (-1) * (num_lines) :
                                              (-1) * (num_lines + window->win_chat_height - 1));
                window->scroll_reset_allowed = 1;
                gui_buffer_ask_chat_refresh (window->buffer, 2);
            }
            break;
        case GUI_BUFFER_TYPE_FREE:
            if (window->start_line)
            {
                snprintf (scroll, sizeof (scroll), "-%d",
                          num_lines + 1);
                gui_window_scroll (window, scroll);
                hook_signal_send ("window_scrolled",
                                  WEECHAT_HOOK_SIGNAL_POINTER, window);
            }
            break;
        case GUI_BUFFER_NUM_TYPES:
            break;
    }
}

/*
 * gui_window_page_down: display next page on buffer
 */

void
gui_window_page_down (struct t_gui_window *window)
{
    struct t_gui_line *ptr_line;
    int line_pos, num_lines;
    char scroll[32];
    
    if (!gui_ok)
        return;
    
    num_lines = ((window->win_chat_height - 1) *
                 CONFIG_INTEGER(config_look_scroll_page_percent)) / 100;
    if (num_lines < 1)
        num_lines = 1;
    else if (num_lines > window->win_chat_height - 1)
        num_lines = window->win_chat_height - 1;
    
    switch (window->buffer->type)
    {
        case GUI_BUFFER_TYPE_FORMATED:
            if (window->start_line)
            {
                gui_chat_calculate_line_diff (window, &window->start_line,
                                              &window->start_line_pos,
                                              num_lines);
                
                /* check if we can display all */
                ptr_line = window->start_line;
                line_pos = window->start_line_pos;
                gui_chat_calculate_line_diff (window, &ptr_line,
                                              &line_pos,
                                              num_lines);
                if (!ptr_line)
                {
                    window->start_line = NULL;
                    window->start_line_pos = 0;
                }
                window->scroll_reset_allowed = 1;
                gui_buffer_ask_chat_refresh (window->buffer, 2);
            }
            break;
        case GUI_BUFFER_TYPE_FREE:
            snprintf (scroll, sizeof (scroll), "+%d",
                      num_lines + 1);
            gui_window_scroll (window, scroll);
            hook_signal_send ("window_scrolled",
                              WEECHAT_HOOK_SIGNAL_POINTER, window);
            break;
        case GUI_BUFFER_NUM_TYPES:
            break;
    }
}

/*
 * gui_window_scroll_up: display previous few lines in buffer
 */

void
gui_window_scroll_up (struct t_gui_window *window)
{
    char scroll[32];
    
    if (!gui_ok)
        return;

    switch (window->buffer->type)
    {
        case GUI_BUFFER_TYPE_FORMATED:
            if (!window->first_line_displayed)
            {
                gui_chat_calculate_line_diff (window, &window->start_line,
                                              &window->start_line_pos,
                                              (window->start_line) ?
                                              (-1) * CONFIG_INTEGER(config_look_scroll_amount) :
                                              (-1) * ( (window->win_chat_height - 1) +
                                                       CONFIG_INTEGER(config_look_scroll_amount)));
                window->scroll_reset_allowed = 1;
                gui_buffer_ask_chat_refresh (window->buffer, 2);
            }
            break;
        case GUI_BUFFER_TYPE_FREE:
            if (window->start_line)
            {
                snprintf (scroll, sizeof (scroll), "-%d",
                          CONFIG_INTEGER(config_look_scroll_amount));
                gui_window_scroll (window, scroll);
                hook_signal_send ("window_scrolled",
                                  WEECHAT_HOOK_SIGNAL_POINTER, window);
            }
            break;
        case GUI_BUFFER_NUM_TYPES:
            break;
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
    char scroll[32];
    
    if (!gui_ok)
        return;
    
    switch (window->buffer->type)
    {
        case GUI_BUFFER_TYPE_FORMATED:
            if (window->start_line)
            {
                gui_chat_calculate_line_diff (window, &window->start_line,
                                              &window->start_line_pos,
                                              CONFIG_INTEGER(config_look_scroll_amount));
                
                /* check if we can display all */
                ptr_line = window->start_line;
                line_pos = window->start_line_pos;
                gui_chat_calculate_line_diff (window, &ptr_line,
                                              &line_pos,
                                              window->win_chat_height - 1);
                
                if (!ptr_line)
                {
                    window->start_line = NULL;
                    window->start_line_pos = 0;
                }
                window->scroll_reset_allowed = 1;
                gui_buffer_ask_chat_refresh (window->buffer, 2);
            }
            break;
        case GUI_BUFFER_TYPE_FREE:
            snprintf (scroll, sizeof (scroll), "+%d",
                      CONFIG_INTEGER(config_look_scroll_amount));
            gui_window_scroll (window, scroll);
            hook_signal_send ("window_scrolled",
                              WEECHAT_HOOK_SIGNAL_POINTER, window);
            break;
        case GUI_BUFFER_NUM_TYPES:
            break;
    }
}

/*
 * gui_window_scroll_top: scroll to top of buffer
 */

void
gui_window_scroll_top (struct t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    switch (window->buffer->type)
    {
        case GUI_BUFFER_TYPE_FORMATED:
            if (!window->first_line_displayed)
            {
                window->start_line = gui_chat_get_first_line_displayed (window->buffer);
                window->start_line_pos = 0;
                window->scroll_reset_allowed = 1;
                gui_buffer_ask_chat_refresh (window->buffer, 2);
            }
            break;
        case GUI_BUFFER_TYPE_FREE:
            if (window->start_line)
            {
                window->start_line = NULL;
                gui_buffer_ask_chat_refresh (window->buffer, 2);
                hook_signal_send ("window_scrolled",
                                  WEECHAT_HOOK_SIGNAL_POINTER, window);
            }
            break;
        case GUI_BUFFER_NUM_TYPES:
            break;
    }
}

/*
 * gui_window_scroll_bottom: scroll to bottom of buffer
 */

void
gui_window_scroll_bottom (struct t_gui_window *window)
{
    char scroll[32];
    
    if (!gui_ok)
        return;

    switch (window->buffer->type)
    {
        case GUI_BUFFER_TYPE_FORMATED:
            if (window->start_line)
            {
                window->start_line = NULL;
                window->start_line_pos = 0;
                window->scroll_reset_allowed = 1;
                gui_buffer_ask_chat_refresh (window->buffer, 2);
            }
            break;
        case GUI_BUFFER_TYPE_FREE:
            window->start_line = NULL;
            if (window->buffer->lines_count > window->win_chat_height)
            {
                snprintf (scroll, sizeof (scroll), "-%d",
                          window->win_chat_height - 1);
                gui_window_scroll (window, scroll);
            }
            else
            {
                gui_buffer_ask_chat_refresh (window->buffer, 2);
            }
            hook_signal_send ("window_scrolled",
                              WEECHAT_HOOK_SIGNAL_POINTER, window);
            break;
        case GUI_BUFFER_NUM_TYPES:
            break;
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
    
    if (!gui_ok)
        return 0;
    
    if (tree)
    {
        if (tree->window)
        {
            if ((width < GUI_WINDOW_MIN_WIDTH) || (height < GUI_WINDOW_MIN_HEIGHT))
                return -1;
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
            if (tree->split_horiz)
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
    struct t_gui_window *ptr_win, *old_current_window;
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_bar *ptr_bar;
    int add_bottom, add_top, add_left, add_right;
    
    if (!gui_ok)
        return;
    
    old_current_window = gui_current_window;

    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (CONFIG_INTEGER(ptr_bar->type) == GUI_BAR_TYPE_ROOT)
        {
            gui_bar_window_calculate_pos_size (ptr_bar->bar_window, NULL);
            gui_bar_window_create_win (ptr_bar->bar_window);
            gui_bar_draw (ptr_bar);
        }
    }
    
    add_bottom = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_BOTTOM);
    add_top = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_TOP);
    add_left = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_LEFT);
    add_right = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_RIGHT);
    
    if (gui_window_auto_resize (gui_windows_tree, add_left, add_top,
                                gui_window_get_width () - add_left - add_right,
                                gui_window_get_height () - add_top - add_bottom,
                                0) < 0)
    {
        gui_window_merge_all (gui_current_window);
    }
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        gui_window_switch_to_buffer (ptr_win, ptr_win->buffer, 0);
        gui_window_draw_separator (ptr_win);
        ptr_win->refresh_needed = 0;
    }
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_window_redraw_buffer (ptr_buffer);
    }
    
    gui_current_window = old_current_window;
}

/*
 * gui_window_split_horiz: split a window horizontally
 */

struct t_gui_window *
gui_window_split_horiz (struct t_gui_window *window, int percentage)
{
    struct t_gui_window *new_window;
    int height1, height2;
    
    if (!gui_ok)
        return NULL;
    
    new_window = NULL;
    
    height1 = (window->win_height * percentage) / 100;
    height2 = window->win_height - height1;
    
    if ((height1 >= GUI_WINDOW_MIN_HEIGHT) && (height2 >= GUI_WINDOW_MIN_HEIGHT)
        && (percentage > 0) && (percentage <= 100))
    {
        new_window = gui_window_new (window,
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
            new_window->buffer = window->buffer;
            new_window->buffer->num_displayed++;
            
            gui_window_switch_to_buffer (window, window->buffer, 1);

            gui_window_switch (new_window);
        }
    }
    
    return new_window;
}

/*
 * gui_window_split_vertic: split a window vertically
 */

struct t_gui_window *
gui_window_split_vertic (struct t_gui_window *window, int percentage)
{
    struct t_gui_window *new_window;
    int width1, width2;
    
    if (!gui_ok)
        return NULL;
    
    new_window = NULL;
    
    width1 = (window->win_width * percentage) / 100;
    width2 = window->win_width - width1 - 1;
    
    if ((width1 >= GUI_WINDOW_MIN_WIDTH) && (width2 >= GUI_WINDOW_MIN_WIDTH)
        && (percentage > 0) && (percentage <= 100))
    {
        new_window = gui_window_new (window,
                                     window->win_x + width1 + 1, window->win_y,
                                     width2, window->win_height,
                                     percentage, 100);
        if (new_window)
        {
            /* reduce old window height (left window) */
            window->win_width = width1;
            window->win_width_pct = 100 - percentage;
            
            /* assign same buffer for new window (right window) */
            new_window->buffer = window->buffer;
            new_window->buffer->num_displayed++;
            
            gui_window_switch_to_buffer (window, window->buffer, 1);
            
            gui_window_switch (new_window);
            
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
    struct t_gui_window_tree *parent;
    int old_split_pct, add_bottom, add_top, add_left, add_right;
    
    if (!gui_ok)
        return;
    
    parent = window->ptr_tree->parent_node;
    if (parent)
    {
        old_split_pct = parent->split_pct;
        if (((parent->split_horiz) && (window->ptr_tree == parent->child2))
            || ((!(parent->split_horiz)) && (window->ptr_tree == parent->child1)))
            parent->split_pct = percentage;
        else
            parent->split_pct = 100 - percentage;
        
        add_bottom = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_BOTTOM);
        add_top = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_TOP);
        add_left = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_LEFT);
        add_right = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_RIGHT);
        
        if (gui_window_auto_resize (gui_windows_tree, add_left, add_top,
                                    gui_window_get_width () - add_left - add_right,
                                    gui_window_get_height () - add_top - add_bottom,
                                    1) < 0)
            parent->split_pct = old_split_pct;
        else
            gui_window_refresh_needed = 1;
    }
}

/*
 * gui_window_merge: merge window with its sister
 */

int
gui_window_merge (struct t_gui_window *window)
{
    struct t_gui_window_tree *parent, *sister;
    
    if (!gui_ok)
        return 0;
    
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
        window->refresh_needed = 1;
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
    int num_deleted, add_bottom, add_top, add_left, add_right;
    
    if (!gui_ok)
        return;
    
    num_deleted = 0;
    while (gui_windows->next_window)
    {
        gui_window_free ((gui_windows == window) ? gui_windows->next_window : gui_windows);
        num_deleted++;
    }
    
    if (num_deleted > 0)
    {
        gui_window_tree_free (&gui_windows_tree);
        gui_window_tree_init (window);
        window->ptr_tree = gui_windows_tree;

        add_bottom = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_BOTTOM);
        add_top = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_TOP);
        add_left = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_LEFT);
        add_right = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_RIGHT);
        window->win_x = add_left;
        window->win_y = add_top;
        window->win_width = gui_window_get_width () - add_left - add_right;
        window->win_height = gui_window_get_height () - add_top - add_bottom;
        
        window->win_width_pct = 100;
        window->win_height_pct = 100;
        
        gui_current_window = window;
        gui_window_switch_to_buffer (window, window->buffer, 1);
    }
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
    if (!gui_ok)
        return 0;
    
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
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 1))
        {
            gui_window_switch (ptr_win);
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
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 3))
        {
            gui_window_switch (ptr_win);
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
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 4))
        {
            gui_window_switch (ptr_win);
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
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 2))
        {
            gui_window_switch (ptr_win);
            return;
        }
    }
}

/*
 * gui_window_refresh_screen: called when term size is modified
 *                            force == 1 when Ctrl+L is pressed
 */

void
gui_window_refresh_screen ()
{
    int new_height, new_width;
    
    endwin ();
    refresh ();
    
    getmaxyx (stdscr, new_height, new_width);
    
    gui_ok = ((new_width >= GUI_WINDOW_MIN_WIDTH)
              && (new_height >= GUI_WINDOW_MIN_HEIGHT));
    
    if (gui_ok)
    {
        refresh ();
        gui_window_refresh_windows ();
    }
}

/*
 * gui_window_title_set: set terminal title
 */

void
gui_window_title_set ()
{
    char *envterm = getenv ("TERM");
    
    if (envterm)
    {
	if (strcmp( envterm, "sun-cmd") == 0)
	    printf ("\033]l%s %s\033\\", PACKAGE_NAME, PACKAGE_VERSION);
	else if (strcmp(envterm, "hpterm") == 0)
	    printf ("\033&f0k%dD%s %s",
                    (int)(strlen(PACKAGE_NAME) + strlen(PACKAGE_VERSION) + 1),
		    PACKAGE_NAME, PACKAGE_VERSION);
	/* the following term supports the xterm excapes */
	else if (strncmp (envterm, "xterm", 5) == 0
		 || strncmp (envterm, "rxvt", 4) == 0
		 || strcmp (envterm, "Eterm") == 0
		 || strcmp (envterm, "aixterm") == 0
		 || strcmp (envterm, "iris-ansi") == 0
		 || strcmp (envterm, "dtterm") == 0)
	    printf ("\33]0;%s %s\7", PACKAGE_NAME, PACKAGE_VERSION);
	else if (strcmp (envterm, "screen") == 0)
	{
	    printf ("\033k%s %s\033\\", PACKAGE_NAME, PACKAGE_VERSION);
	    /* tryning to set the title of a backgrounded xterm like terminal */
	    printf ("\33]0;%s %s\7", PACKAGE_NAME, PACKAGE_VERSION);
	}
    }
}

/*
 * gui_window_title_reset: reset terminal title
 */

void
gui_window_title_reset ()
{
    char *shell, *shellname;
    char *envterm = getenv ("TERM");
    char *envshell = getenv ("SHELL");

    if (envterm)
    {
	if (strcmp( envterm, "sun-cmd") == 0)
	    printf ("\033]l%s\033\\", "Terminal");
	else if (strcmp( envterm, "hpterm") == 0)
	    printf ("\033&f0k%dD%s", (int)strlen("Terminal"), "Terminal");
	/* the following term supports the xterm excapes */
	else if (strncmp (envterm, "xterm", 5) == 0
		 || strncmp (envterm, "rxvt", 4) == 0
		 || strcmp (envterm, "Eterm") == 0
		 || strcmp( envterm, "aixterm") == 0
		 || strcmp( envterm, "iris-ansi") == 0
		 || strcmp( envterm, "dtterm") == 0)
	    printf ("\33]0;%s\7", "Terminal");
	else if (strcmp (envterm, "screen") == 0)
	{
	    if (envshell)
	    {
		shell  = strdup (envshell);
		if (shell)
		{
                    shellname = basename (shell);
		    printf ("\033k%s\033\\", (shellname) ? shellname : shell);
		    free (shell);
		}
		else
		    printf ("\033k%s\033\\", envterm);
	    }
	    else
		printf ("\033k%s\033\\", envterm);
	    /* tryning to reset the title of a backgrounded xterm like terminal */
	    printf ("\33]0;%s\7", "Terminal");
	}
    }
}

/*
 * gui_window_objects_print_log: print window Curses objects infos in log
 *                               (usually for crash dump)
 */

void
gui_window_objects_print_log (struct t_gui_window *window)
{
    log_printf ("  window specific objects for Curses:");
    log_printf ("    win_chat. . . . . . : 0x%lx", GUI_WINDOW_OBJECTS(window)->win_chat);
    log_printf ("    win_separator . . . : 0x%lx", GUI_WINDOW_OBJECTS(window)->win_separator);
}
