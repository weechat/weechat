/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

/* gui-curses-main.c: main loop for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-hook.h"
#include "../../core/wee-string.h"
#include "../../core/wee-utf8.h"
#include "../../core/wee-util.h"
#include "../../plugins/plugin.h"
#include "../gui-infobar.h"
#include "../gui-input.h"
#include "../gui-history.h"
#include "../gui-hotlist.h"
#include "../gui-keyboard.h"
#include "../gui-main.h"
#include "../gui-window.h"
#include "gui-curses.h"


/*
 * gui_main_pre_init: pre-initialize GUI (called before gui_init)
 */

void
gui_main_pre_init (int *argc, char **argv[])
{
    /* nothing for Curses interface */
    (void) argc;
    (void) argv;
}

/*
 * gui_main_init: init GUI
 */

void
gui_main_init ()
{
    struct t_gui_buffer *ptr_buffer;
    
    initscr ();
    
    curs_set (1);
    noecho ();
    nodelay (stdscr, TRUE);
    raw ();
    
    gui_color_init ();
    gui_chat_prefix_build ();
    
    gui_infobar = NULL;
    
    gui_ok = ((COLS > WINDOW_MIN_WIDTH) && (LINES > WINDOW_MIN_HEIGHT));

    refresh ();
    
    /* init clipboard buffer */
    gui_input_clipboard = NULL;
    
    /* get time length */
    gui_chat_time_length = util_get_time_length (cfg_look_buffer_time_format);

    /* create new window/buffer */
    if (gui_window_new (NULL, 0, 0, COLS, LINES, 100, 100))
    {
        gui_current_window = gui_windows;
        ptr_buffer = gui_buffer_new (NULL, "weechat", "weechat");
        if (ptr_buffer)
        {
            gui_init_ok = 1;
            gui_buffer_set_title (ptr_buffer,
                                  PACKAGE_STRING " " WEECHAT_COPYRIGHT_DATE
                                  " - " WEECHAT_WEBSITE);
            gui_window_redraw_buffer (ptr_buffer);
        }
        else
            gui_init_ok = 0;
        
        if (cfg_look_set_title)
            gui_window_title_set ();
        
        signal (SIGWINCH, gui_window_refresh_screen_sigwinch);
    }
}

/*
 * gui_main_quit: quit weechat (signal received)
 */

void
gui_main_quit ()
{
    quit_weechat = 1;
}

/*
 * gui_main_loop: main loop for WeeChat with ncurses GUI
 */

void
gui_main_loop ()
{
    fd_set read_fds, write_fds, except_fds;
    static struct timeval timeout;
    struct t_gui_buffer *ptr_buffer;
    int old_day, old_min, old_sec;
    char text_time[1024], *text_time2;
    struct timeval tv_time;
    struct tm *local_time;
    
    quit_weechat = 0;
    
    gettimeofday (&tv_time, NULL);
    gui_keyboard_last_activity_time = tv_time.tv_sec;
    local_time = localtime (&tv_time.tv_sec);
    old_day = local_time->tm_mday;
    
    old_min = -1;
    old_sec = -1;
    
    /* if SIGTERM or SIGHUP received => quit */
    signal (SIGTERM, gui_main_quit);
    signal (SIGHUP, gui_main_quit);
    
    while (!quit_weechat)
    {
        /* refresh needed ? */
        if (gui_refresh_screen_needed)
            gui_window_refresh_screen (0);

        for (ptr_buffer = gui_buffers; ptr_buffer;
             ptr_buffer = ptr_buffer->next_buffer)
        {
            if (ptr_buffer->chat_refresh_needed)
            {
                gui_chat_draw (ptr_buffer, 0);
                ptr_buffer->chat_refresh_needed = 0;
            }
        }
        
        gettimeofday (&tv_time, NULL);
        local_time = localtime (&tv_time.tv_sec);

        /* execute hook timers */
        hook_timer_exec (&tv_time);
        
        /* minute has changed ? => redraw infobar */
        if (local_time->tm_min != old_min)
        {
            old_min = local_time->tm_min;
            gui_infobar_draw (gui_current_window->buffer, 1);
            
            if (cfg_look_day_change
                && (local_time->tm_mday != old_day))
            {
                strftime (text_time, sizeof (text_time),
                          cfg_look_day_change_time_format, local_time);
                text_time2 = string_iconv_to_internal (NULL, text_time);
                gui_add_hotlist = 0;
                for (ptr_buffer = gui_buffers; ptr_buffer;
                     ptr_buffer = ptr_buffer->next_buffer)
                {
                    if (ptr_buffer->type == GUI_BUFFER_TYPE_FORMATED)
                        gui_chat_printf (ptr_buffer,
                                         _("\t\tDay changed to %s"),
                                         (text_time2) ?
                                         text_time2 : text_time);
                }
                if (text_time2)
                    free (text_time2);
                gui_add_hotlist = 1;
            }
            old_day = local_time->tm_mday;
        }
        
        /* second has changed ? */
        if (local_time->tm_sec != old_sec)
        {
            old_sec = local_time->tm_sec;
            
            /* display time in infobar (if seconds displayed) */
            if (cfg_look_infobar_seconds)
            {
                gui_infobar_draw_time (gui_current_window->buffer);
                wmove (GUI_CURSES(gui_current_window)->win_input,
                       0, gui_current_window->win_input_cursor_x);
                wrefresh (GUI_CURSES(gui_current_window)->win_input);
            }
            
            /* infobar count down */
            if (gui_infobar && gui_infobar->remaining_time > 0)
            {
                gui_infobar->remaining_time--;
                if (gui_infobar->remaining_time == 0)
                {
                    gui_infobar_remove ();
                    gui_infobar_draw (gui_current_window->buffer, 1);
                }
            }
        }
        
        /* read keyboard */

        /* on GNU/Hurd 2 select() are causing troubles with keyboard */
        /* waiting for a fix, we use only one select() */
#ifndef __GNU__        
        FD_ZERO (&read_fds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 8000;
        
        FD_SET (STDIN_FILENO, &read_fds);
        
        if (select (FD_SETSIZE, &read_fds, NULL, NULL, &timeout) > 0)
        {
            if (FD_ISSET (STDIN_FILENO, &read_fds))
            {
                gui_keyboard_read ();
            }
        }
        else
            gui_keyboard_flush ();
#endif
        
        /* read sockets/files/pipes */
        hook_fd_set (&read_fds, &write_fds, &except_fds);
        
#ifdef __GNU__
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000;
        FD_SET (STDIN_FILENO, &read_fds);
#else
        timeout.tv_sec = 0;
        timeout.tv_usec = 2000;
#endif
        
        if (select (FD_SETSIZE,
                    &read_fds, &write_fds, &except_fds,
                    &timeout) > 0)
        {
#ifdef __GNU__
            if (FD_ISSET (STDIN_FILENO, &read_fds))
            {
                gui_keyboard_read ();
            }
#endif
            hook_fd_exec (&read_fds, &write_fds, &except_fds);
        }
    }
}

/*
 * gui_main_end: GUI end
 */

void
gui_main_end ()
{
    /* free clipboard buffer */
    if (gui_input_clipboard)
        free (gui_input_clipboard);
    
    /* delete all windows */
    while (gui_windows)
        gui_window_free (gui_windows);
    gui_window_tree_free (&gui_windows_tree);

    /* delete all buffers */
    while (gui_buffers)
        gui_buffer_free (gui_buffers, 0);
    
    /* delete global history */
    gui_history_global_free ();
    
    /* delete infobar messages */
    while (gui_infobar)
        gui_infobar_remove ();

    /* reset title */
    if (cfg_look_set_title)
	gui_window_title_reset ();
    
    /* end of Curses output */
    refresh ();
    endwin ();
}
