/*
 * gui-curses-main.c - main loop for Curses GUI
 *
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "../../core/weechat.h"
#include "../../core/wee-command.h"
#include "../../core/wee-config.h"
#include "../../core/wee-hook.h"
#include "../../core/wee-log.h"
#include "../../core/wee-string.h"
#include "../../core/wee-utf8.h"
#include "../../core/wee-util.h"
#include "../../core/wee-version.h"
#include "../../plugins/plugin.h"
#include "../gui-main.h"
#include "../gui-bar.h"
#include "../gui-bar-item.h"
#include "../gui-bar-window.h"
#include "../gui-buffer.h"
#include "../gui-chat.h"
#include "../gui-color.h"
#include "../gui-cursor.h"
#include "../gui-filter.h"
#include "../gui-input.h"
#include "../gui-layout.h"
#include "../gui-history.h"
#include "../gui-mouse.h"
#include "../gui-nicklist.h"
#include "../gui-window.h"
#include "gui-curses.h"


int gui_reload_config = 0;
int gui_term_cols = 0;
int gui_term_lines = 0;


/*
 * Pre-initializes GUI (called before gui_init).
 */

void
gui_main_pre_init (int *argc, char **argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;

    /* pre-init colors */
    gui_color_pre_init ();

    /* init some variables for chat area */
    gui_chat_init ();
}

/*
 * Initializes GUI.
 */

void
gui_main_init ()
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_bar *ptr_bar;
    struct t_gui_bar_window *ptr_bar_win;
    char title[256];

    initscr ();

    if (CONFIG_BOOLEAN(config_look_eat_newline_glitch))
        gui_term_set_eat_newline_glitch (0);

    curs_set (1);
    noecho ();
    nodelay (stdscr, TRUE);
    raw ();

    gui_color_init ();

    /* build prefixes according to configuration */
    gui_chat_prefix_build ();

    refresh ();

    gui_term_cols  = COLS;
    gui_term_lines = LINES;

    gui_window_read_terminal_size ();

    /* init clipboard buffer */
    gui_input_clipboard = NULL;

    /* get time length */
    gui_chat_time_length = gui_chat_get_time_length ();

    /* init bar items */
    gui_bar_item_init ();

    gui_init_ok = 0;

    /* create core buffer */
    ptr_buffer = gui_buffer_new (NULL, GUI_BUFFER_MAIN,
                                 NULL, NULL, NULL, NULL);
    if (ptr_buffer)
    {
        gui_init_ok = 1;

        ptr_buffer->num_displayed = 1;

        /* set short name */
        if (!ptr_buffer->short_name)
            ptr_buffer->short_name = strdup (GUI_BUFFER_MAIN);

        /* set title for core buffer */
        snprintf (title, sizeof (title), "WeeChat %s %s - %s",
                  version_get_version (),
                  WEECHAT_COPYRIGHT_DATE,
                  WEECHAT_WEBSITE);
        gui_buffer_set_title (ptr_buffer, title);

        /* create main window (using full space) */
        if (gui_window_new (NULL, ptr_buffer, 0, 0,
                            gui_term_cols, gui_term_lines, 100, 100))
        {
            gui_current_window = gui_windows;

            if (CONFIG_BOOLEAN(config_look_set_title))
                gui_window_set_title (version_get_name_version ());
        }

        /*
         * create bar windows for root bars (they were read from config,
         * but no window was created, GUI was not initialized)
         */
        for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
        {
            if ((CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
                && (!ptr_bar->bar_window))
            {
                gui_bar_window_new (ptr_bar, NULL);
            }
        }
        for (ptr_bar_win = gui_windows->bar_windows;
             ptr_bar_win; ptr_bar_win = ptr_bar_win->next_bar_window)
        {
            gui_bar_window_calculate_pos_size (ptr_bar_win, gui_windows);
            gui_bar_window_create_win (ptr_bar_win);
        }
    }

    if (CONFIG_BOOLEAN(config_look_mouse))
        gui_mouse_enable ();
    else
        gui_mouse_disable ();

    gui_window_set_bracketed_paste_mode (CONFIG_BOOLEAN(config_look_paste_bracketed));
}

/*
 * Callback for system signal SIGQUIT: quits WeeChat.
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
 * Callback for system signal SIGTERM: quits WeeChat.
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
 * Callback for system signal SIGHUP: reloads WeeChat configuration.
 */

void
gui_main_signal_sighup ()
{
    /*
     * SIGHUP signal is received when terminal is closed (exit of WeeChat
     * without using /quit command), that's why we set only flag to reload
     * configuration files later (when terminal is closed, config files are NOT
     * reloaded, but they are if signal SIGHUP is sent to WeeChat by user)
     */
    gui_reload_config = 1;
}

/*
 * Callback for system signal SIGWINCH: refreshes screen.
 */

void
gui_main_signal_sigwinch ()
{
    gui_window_ask_refresh (2);
}

/*
 * Refreshs for windows, buffers, bars.
 */

void
gui_main_refreshs ()
{
    struct t_gui_window *ptr_win;
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_bar *ptr_bar;

    /* refresh color buffer if needed */
    if (gui_color_buffer_refresh_needed)
    {
        gui_color_buffer_display ();
        gui_color_buffer_refresh_needed = 0;
    }

    /* refresh window if needed */
    if (gui_window_refresh_needed)
    {
        gui_window_refresh_screen ((gui_window_refresh_needed > 1) ? 1 : 0);
        gui_window_refresh_needed = 0;
    }

    /* refresh bars if needed */
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (ptr_bar->bar_refresh_needed)
        {
            gui_bar_draw (ptr_bar);
        }
    }

    /* refresh windows if needed */
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->refresh_needed)
        {
            gui_window_switch_to_buffer (ptr_win, ptr_win->buffer, 0);
            gui_window_redraw_buffer (ptr_win->buffer);
            ptr_win->refresh_needed = 0;
        }
    }

    /* refresh chat buffers if needed */
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
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
        {
            gui_bar_draw (ptr_bar);
        }
    }

    /* move cursor (for cursor mode) */
    if (gui_cursor_mode)
        gui_window_move_cursor ();
}

/*
 * Main loop for WeeChat with ncurses GUI.
 */

void
gui_main_loop ()
{
    struct t_hook *hook_fd_keyboard;
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
                                &gui_key_read_cb, NULL);

    gui_window_ask_refresh (1);

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

        /* auto reset of color pairs */
        if (gui_color_pairs_auto_reset)
        {
            gui_color_reset_pairs ();
            gui_color_pairs_auto_reset_last = time (NULL);
            gui_color_pairs_auto_reset = 0;
            gui_color_pairs_auto_reset_pending = 1;
        }

        gui_main_refreshs ();
        if (gui_window_refresh_needed)
            gui_main_refreshs ();

        gui_color_pairs_auto_reset_pending = 0;

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
 * Ends GUI.
 *
 * Argument "clean_exit" is 0 when WeeChat is crashing (we don't clean objects
 * because WeeChat can crash again during this cleanup...).
 */

void
gui_main_end (int clean_exit)
{
    if (clean_exit)
    {
        /*
         * final refreshs, to see messages just before exiting
         * (if we are upgrading, don't refresh anything!)
         */
        if (!weechat_upgrading)
        {
            gui_main_refreshs ();
            if (gui_window_refresh_needed)
                gui_main_refreshs ();
        }

        /* disable bracketed paste mode */
        gui_window_set_bracketed_paste_mode (0);

        /* disable mouse */
        gui_mouse_disable ();

        /* remove bar items and bars */
        gui_bar_item_end ();
        gui_bar_free_all ();

        /* remove filters */
        gui_filter_free_all ();

        /* free clipboard buffer */
        if (gui_input_clipboard)
            free (gui_input_clipboard);

        /* delete layout saved */
        gui_layout_window_remove_all (&gui_layout_windows);
        gui_layout_buffer_remove_all (&gui_layout_buffers, &last_gui_layout_buffer);

        /* delete all windows */
        while (gui_windows)
        {
            gui_window_free (gui_windows);
        }
        gui_window_tree_free (&gui_windows_tree);

        /* delete all buffers */
        while (gui_buffers)
        {
            gui_buffer_close (gui_buffers);
        }

        gui_init_ok = 0;

        /* delete global history */
        gui_history_global_free ();

        /* reset title */
        if (CONFIG_BOOLEAN(config_look_set_title))
            gui_window_set_title (NULL);

        /* end color */
        gui_color_end ();

        /* free some variables used for chat area */
        gui_chat_end ();
    }

    /* end of Curses output */
    refresh ();
    endwin ();
}
