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

/* gui-curses-infobar.c: infobar display functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-hook.h"
#include "../../core/wee-string.h"
#include "../../plugins/plugin.h"
#include "../gui-infobar.h"
#include "../gui-color.h"
#include "../gui-main.h"
#include "../gui-window.h"
#include "gui-curses.h"


/*
 * gui_infobar_draw_time: draw time in infobar window
 */

void
gui_infobar_draw_time (struct t_gui_buffer *buffer)
{
    struct t_gui_window *ptr_win;
    time_t time_seconds;
    struct tm *local_time;
    
    /* make C compiler happy */
    (void) buffer;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {        
        time_seconds = time (NULL);
        local_time = localtime (&time_seconds);
        if (local_time)
        {
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_infobar,
                                          GUI_COLOR_INFOBAR);
            mvwprintw (GUI_CURSES(ptr_win)->win_infobar,
                       0, 1,
                       "%02d:%02d",
                       local_time->tm_hour, local_time->tm_min);
            if (CONFIG_BOOLEAN(config_look_infobar_seconds))
                wprintw (GUI_CURSES(ptr_win)->win_infobar,
                         ":%02d",
                         local_time->tm_sec);
        }
        wnoutrefresh (GUI_CURSES(ptr_win)->win_infobar);
    }
}

/*
 * gui_infobar_draw: draw infobar window for a buffer
 */

void
gui_infobar_draw (struct t_gui_buffer *buffer, int erase)
{
    struct t_gui_window *ptr_win;
    time_t time_seconds;
    struct tm *local_time;
    char text_time[1024], *buf;
    
    /* make C compiler happy */
    (void) buffer;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (erase)
            gui_window_curses_clear (GUI_CURSES(ptr_win)->win_infobar,
                                     GUI_COLOR_INFOBAR);
        
        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_infobar,
                                      GUI_COLOR_INFOBAR);
        
        time_seconds = time (NULL);
        local_time = localtime (&time_seconds);
        if (local_time)
        {
            strftime (text_time, sizeof (text_time),
                      CONFIG_STRING(config_look_infobar_time_format),
                      local_time);
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_infobar,
                                          GUI_COLOR_INFOBAR_DELIMITERS);
            wprintw (GUI_CURSES(ptr_win)->win_infobar, "[");
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_infobar,
                                          GUI_COLOR_INFOBAR);
            wprintw (GUI_CURSES(ptr_win)->win_infobar,
                     "%02d:%02d",
                     local_time->tm_hour, local_time->tm_min);
            if (CONFIG_BOOLEAN(config_look_infobar_seconds))
                wprintw (GUI_CURSES(ptr_win)->win_infobar,
                         ":%02d",
                         local_time->tm_sec);
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_infobar,
                                          GUI_COLOR_INFOBAR_DELIMITERS);
            wprintw (GUI_CURSES(ptr_win)->win_infobar, "]");
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_infobar,
                                          GUI_COLOR_INFOBAR);
            wprintw (GUI_CURSES(ptr_win)->win_infobar,
                     " %s", text_time);
        }
        if (gui_infobar)
        {
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_infobar,
                                          GUI_COLOR_INFOBAR_DELIMITERS);
            wprintw (GUI_CURSES(ptr_win)->win_infobar, " | ");
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_infobar,
                                          gui_infobar->color);
            buf = string_iconv_from_internal (NULL, gui_infobar->text);
            wprintw (GUI_CURSES(ptr_win)->win_infobar, "%s",
                     (buf) ? buf : gui_infobar->text);
            if (buf)
                free (buf);
        }
        
        wnoutrefresh (GUI_CURSES(ptr_win)->win_infobar);
        refresh ();
    }
}

/*
 * gui_infobar_refresh_timer_cb: timer callback for refresh of infobar
 */

int
gui_infobar_refresh_timer_cb (void *data)
{
    /* make C compiler happy */
    (void) data;

    if (gui_ok)
    {
        if (data)
            gui_infobar_draw (gui_current_window->buffer, 1);
        else
            gui_infobar_draw_time (gui_current_window->buffer);
        wmove (GUI_CURSES(gui_current_window)->win_input,
               0, gui_current_window->win_input_cursor_x);
        wrefresh (GUI_CURSES(gui_current_window)->win_input);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * gui_infobar_highlight_timer_cb: timer callback for highlights in infobar
 */

int
gui_infobar_highlight_timer_cb (void *data)
{
    /* make C compiler happy */
    (void) data;
    
    if (gui_ok)
    {
        if (gui_infobar && (gui_infobar->remaining_time > 0))
        {
            gui_infobar->remaining_time--;
            if (gui_infobar->remaining_time == 0)
            {
                gui_infobar_remove ();
                gui_infobar_draw (gui_current_window->buffer, 1);
            }
        }
        /* remove this timer if there's no more data for infobar */
        if (!gui_infobar)
        {
            unhook (gui_infobar_highlight_timer);
            gui_infobar_highlight_timer = NULL;
        }
    }
    
    return WEECHAT_RC_OK;
}
