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
#include "../../core/wee-command.h"
#include "../../core/wee-config.h"
#include "../../core/wee-hook.h"
#include "../../core/wee-log.h"
#include "../../core/wee-string.h"
#include "../../core/wee-utf8.h"
#include "../../core/wee-util.h"
#include "../../plugins/plugin.h"
#include "../gui-main.h"
#include "../gui-bar.h"
#include "../gui-bar-item.h"
#include "../gui-buffer.h"
#include "../gui-chat.h"
#include "../gui-color.h"
#include "../gui-filter.h"
#include "../gui-input.h"
#include "../gui-layout.h"
#include "../gui-history.h"
#include "../gui-nicklist.h"
#include "../gui-window.h"
#include "gui-curses.h"


int gui_reload_config = 0;


/*
 * gui_main_pre_init: pre-initialize GUI (called before gui_init)
 */

void
gui_main_pre_init (int *argc, char **argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    /* pre-init colors */
    gui_color_pre_init ();
    
    /* build empty prefixes (before reading config) */
    gui_chat_prefix_build_empty ();
}

/*
 * gui_main_init: init GUI
 */

void
gui_main_init ()
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_bar *ptr_bar;
    struct t_gui_bar_window *ptr_bar_win;
    
    initscr ();
    
    curs_set (1);
    noecho ();
    nodelay (stdscr, TRUE);
    raw ();
    
    gui_color_init ();

    /* build prefixes according to config */
    gui_chat_prefix_build ();
    
    gui_ok = ((COLS >= GUI_WINDOW_MIN_WIDTH)
              && (LINES >= GUI_WINDOW_MIN_HEIGHT));
    
    refresh ();
    
    /* init clipboard buffer */
    gui_input_clipboard = NULL;
    
    /* get time length */
    gui_chat_time_length = util_get_time_length (CONFIG_STRING(config_look_buffer_time_format));
    
    /* init bar items */
    gui_bar_item_init ();
    
    /* create new window/buffer */
    if (gui_window_new (NULL, 0, 0, COLS, LINES, 100, 100))
    {
        gui_current_window = gui_windows;
        ptr_buffer = gui_buffer_new (NULL, "weechat", NULL, NULL, NULL, NULL);
        if (ptr_buffer)
        {
            gui_init_ok = 1;
            gui_buffer_set_title (ptr_buffer,
                                  "WeeChat " WEECHAT_COPYRIGHT_DATE
                                  " - " WEECHAT_WEBSITE);
        }
        else
            gui_init_ok = 0;
        
        if (CONFIG_BOOLEAN(config_look_set_title))
            gui_window_title_set ();
    }
    
    if (gui_init_ok)
    {
        /* create bar windows for root bars (they were read from config,
           but no window was created (GUI was not initialized) */
        for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
        {
            if ((CONFIG_INTEGER(ptr_bar->type) == GUI_BAR_TYPE_ROOT) && (!ptr_bar->bar_window))
                gui_bar_window_new (ptr_bar, NULL);
        }
        for (ptr_bar_win = GUI_CURSES(gui_windows)->bar_windows;
             ptr_bar_win; ptr_bar_win = ptr_bar_win->next_bar_window)
        {
            gui_bar_window_calculate_pos_size (ptr_bar_win, gui_windows);
            gui_bar_window_create_win (ptr_bar_win);
        }
    }
}

/*
 * gui_main_signal_sigquit: quit WeeChat
 */

void
gui_main_signal_sigquit ()
{
    log_printf (_("Signal %s received, exiting WeeChat..."),
                "SIGQUIT");
    hook_signal_send ("quit", WEECHAT_HOOK_SIGNAL_STRING, NULL);
    weechat_quit = 1;
}

/*
 * gui_main_signal_sigterm: quit WeeChat
 */

void
gui_main_signal_sigterm ()
{
    log_printf (_("Signal %s received, exiting WeeChat..."),
                "SIGTERM");
    hook_signal_send ("quit", WEECHAT_HOOK_SIGNAL_STRING, NULL);
    weechat_quit = 1;
}

/*
 * gui_main_signal_sighup: reload WeeChat configuration
 */

void
gui_main_signal_sighup ()
{
    /* SIGHUP signal is received when terminal is closed (exit of WeeChat
       without using /quit command), that's why we set only flag to reload
       config files later (when terminal is closed, config files are NOT
       reloaded, but they are if signal SIGHUP is sent to WeeChat by user)
    */
    gui_reload_config = 1;
}

/*
 * gui_main_signal_sigwinch: called when signal SIGWINCH is received
 */

void
gui_main_signal_sigwinch ()
{
    gui_window_refresh_needed = 1;
}

/*
 * gui_main_loop: main loop for WeeChat with ncurses GUI
 */

void
gui_main_loop ()
{
    struct t_hook *hook_fd_keyboard;
    struct t_gui_window *ptr_win;
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_bar *ptr_bar;
    struct timeval tv_timeout;
    fd_set read_fds, write_fds, except_fds;
    int max_fd;
    int ready;
    
    weechat_quit = 0;
    
    /* catch SIGTERM signal: quit program */
    util_catch_signal (SIGTERM, &gui_main_signal_sigterm);
    util_catch_signal (SIGQUIT, &gui_main_signal_sigquit);
    
    /* catch SIGHUP signal: reload configuration */
    util_catch_signal (SIGHUP, &gui_main_signal_sighup);
    
    /* catch SIGWINCH signal: redraw screen */
    util_catch_signal (SIGWINCH, &gui_main_signal_sigwinch);
    
    /* hook stdin (read keyboard) */
    hook_fd_keyboard = hook_fd (NULL, STDIN_FILENO, 1, 0, 0,
                                &gui_keyboard_read_cb, NULL);
    
    while (!weechat_quit)
    {
        /* reload config, if SIGHUP reveived */
        if (gui_reload_config)
        {
            gui_reload_config = 0;
            log_printf (_("Signal SIGHUP received, reloading configuration "
                          "files"));
            command_reload (NULL, NULL, 0, NULL, NULL);
        }
        
        /* execute hook timers */
        hook_timer_exec ();
        
        /* refresh window if needed */
        if (gui_window_refresh_needed)
        {
            gui_window_refresh_screen ();
            gui_window_refresh_needed = 0;
        }
        else
        {
            /* refresh bars if needed */
            for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
            {
                if (ptr_bar->bar_refresh_needed)
                    gui_bar_draw (ptr_bar);
            }
            
            for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
            {
                if (ptr_win->refresh_needed)
                {
                    gui_window_switch_to_buffer (ptr_win, ptr_win->buffer, 0);
                    gui_window_redraw_buffer (ptr_win->buffer);
                    ptr_win->refresh_needed = 0;
                }
            }
            
            for (ptr_buffer = gui_buffers; ptr_buffer;
                 ptr_buffer = ptr_buffer->next_buffer)
            {
                /* refresh chat if needed */
                if (ptr_buffer->chat_refresh_needed)
                {
                    gui_chat_draw (ptr_buffer,
                                   (ptr_buffer->chat_refresh_needed) > 1 ? 1 : 0);
                }
            }
            
            /* refresh bars if needed */
            for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
            {
                if (ptr_bar->bar_refresh_needed)
                    gui_bar_draw (ptr_bar);
            }
        }
        
        /* wait for keyboard or network activity */
        FD_ZERO (&read_fds);
        FD_ZERO (&write_fds);
        FD_ZERO (&except_fds);
        max_fd = hook_fd_set (&read_fds, &write_fds, &except_fds);
        hook_timer_time_to_next (&tv_timeout);
        ready = select (max_fd + 1, &read_fds, &write_fds, &except_fds,
                        &tv_timeout);
        if (ready > 0)
        {
            hook_fd_exec (&read_fds, &write_fds, &except_fds);
        }
    }
    
    /* remove keyboard hook */
    unhook (hook_fd_keyboard);
}

/*
 * gui_main_end: GUI end
 *               clean_exit is 0 when WeeChat is crashing (we don't clean
 *               objects because WeeChat can crash again during this cleanup...)
 */

void
gui_main_end (int clean_exit)
{
    if (clean_exit)
    {
        /* remove bar items and bars */
        gui_bar_item_end ();
        gui_bar_free_all ();
        
        /* remove filters */
        gui_filter_free_all ();
        
        /* free clipboard buffer */
        if (gui_input_clipboard)
            free (gui_input_clipboard);
        
        /* delete layout saved */
        gui_layout_window_remove_all ();
        gui_layout_buffer_remove_all ();
        
        /* delete all windows */
        while (gui_windows)
        {
            gui_window_free (gui_windows);
        }
        gui_window_tree_free (&gui_windows_tree);
        
        /* delete all buffers */
        while (gui_buffers)
        {
            gui_buffer_close (gui_buffers, 0);
        }
        
        gui_ok = 0;
        gui_init_ok = 0;
        
        /* delete global history */
        gui_history_global_free ();
        
        /* reset title */
        if (CONFIG_BOOLEAN(config_look_set_title))
            gui_window_title_reset ();
        
        /* end color */
        gui_color_end ();
        
        /* free chat buffer */
        gui_chat_free_buffer ();
    }
    
    /* end of Curses output */
    refresh ();
    endwin ();
}
