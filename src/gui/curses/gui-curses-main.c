/*
 * gui-curses-main.c - main loop for Curses GUI
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "../../core/weechat.h"
#include "../../core/core-command.h"
#include "../../core/core-config.h"
#include "../../core/core-hook.h"
#include "../../core/core-log.h"
#include "../../core/core-signal.h"
#include "../../core/core-string.h"
#include "../../core/core-utf8.h"
#include "../../core/core-version.h"
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
#include "../gui-hotlist.h"
#include "../gui-input.h"
#include "../gui-layout.h"
#include "../gui-line.h"
#include "../gui-history.h"
#include "../gui-mouse.h"
#include "../gui-nicklist.h"
#include "../gui-window.h"
#include "gui-curses.h"
#include "gui-curses-color.h"
#include "gui-curses-key.h"
#include "gui-curses-window.h"


volatile sig_atomic_t gui_signal_sigwinch_received = 0;  /* sigwinch signal */
                                       /* (terminal has been resized)       */
int gui_term_cols = 0;                 /* number of columns in terminal     */
int gui_term_lines = 0;                /* number of lines in terminal       */


/*
 * Gets a password from user (called on startup, when GUI is not initialized).
 *
 * The result is stored in "password" with max "size" bytes (including the
 * final '\0').
 */

void
gui_main_get_password (const char **prompt, char *password, int size)
{
    int line, i, ch;

    memset (password, '\0', size);

    if (weechat_headless)
    {
        password[0] = ' ';
        return;
    }

    initscr ();
    cbreak ();
    noecho ();
    raw ();

    clear ();

    line = 0;

    while (prompt[line])
    {
        mvaddstr (line, 0, prompt[line]);
        line++;
    }

    mvaddstr (line, 0, "=> ");
    refresh ();

    i = 0;
    while (i < size - 1)
    {
        ch = getch ();
        /* enter */
        if (ch == '\n')
            break;
        /* terminal lost or ctrl-c */
        if ((ch == ERR) || (ch == 3))
        {
            password[0] = 3;
            i = 1;
            break;
        }
        if (ch == 127)
        {
            if (i > 0)
            {
                i--;
                password[i] = '\0';
                mvaddstr (line, 3 + i, " ");
                move (line, 3 + i);
            }
        }
        else
        {
            password[i] = ch;
            mvaddstr (line, 3 + i, "*");
            i++;
        }
        refresh ();
    }
    password[i] = '\0';

    refresh ();
    endwin ();
}

/*
 * Callback for system signal SIGINT: quits WeeChat.
 */

void
gui_main_signal_sigint ()
{
    weechat_quit = 1;
}

/*
 * Initializes GUI.
 */

void
gui_main_init ()
{
    struct t_gui_buffer *ptr_buffer;
    char title[256];

    /* allow ctrl-c to quit WeeChat in headless mode */
    if (weechat_headless)
        signal_catch (SIGINT, &gui_main_signal_sigint);

    initscr ();

    if (CONFIG_BOOLEAN(config_look_eat_newline_glitch))
        gui_term_set_eat_newline_glitch (0);

    curs_set (1);
    noecho ();
    nodelay (stdscr, TRUE);
    raw ();

    gui_color_alloc ();

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
                                 NULL, NULL, NULL,
                                 NULL, NULL, NULL);
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

            if (CONFIG_STRING(config_look_window_title)
                && CONFIG_STRING(config_look_window_title)[0])
            {
                gui_window_set_title (CONFIG_STRING(config_look_window_title));
            }
        }

        /* switch to buffer */
        gui_window_switch_to_buffer (gui_current_window, ptr_buffer, 0);
    }

    if (CONFIG_BOOLEAN(config_look_mouse))
        gui_mouse_enable ();
    else
        gui_mouse_disable ();

    gui_window_set_bracketed_paste_mode (CONFIG_BOOLEAN(config_look_paste_bracketed));
}

/*
 * Callback for system signal SIGWINCH: refreshes screen.
 */

void
gui_main_signal_sigwinch ()
{
    gui_signal_sigwinch_received = 1;
}

/*
 * Displays infos about ncurses lib.
 */

void
gui_main_debug_libs ()
{
#if defined(NCURSES_VERSION) && defined(NCURSES_VERSION_PATCH)
    gui_chat_printf (NULL, "    ncurses: %s (patch %d)",
                     NCURSES_VERSION, NCURSES_VERSION_PATCH);
#else
    gui_chat_printf (NULL, "    ncurses: (?)");
#endif /* defined(NCURSES_VERSION) && defined(NCURSES_VERSION_PATCH) */
}

/*
 * Refreshes for windows, buffers, bars.
 */

void
gui_main_refreshes ()
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

    /* compute max length for prefix/buffer if needed */
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        /* compute buffer/prefix max length for own_lines */
        if (ptr_buffer->own_lines)
        {
            if (ptr_buffer->own_lines->buffer_max_length_refresh)
            {
                gui_line_compute_buffer_max_length (ptr_buffer,
                                                    ptr_buffer->own_lines);
            }
            if (ptr_buffer->own_lines->prefix_max_length_refresh)
                gui_line_compute_prefix_max_length (ptr_buffer->own_lines);
        }

        /* compute buffer/prefix max length for mixed_lines */
        if (ptr_buffer->mixed_lines)
        {
            if (ptr_buffer->mixed_lines->buffer_max_length_refresh)
            {
                gui_line_compute_buffer_max_length (ptr_buffer,
                                                    ptr_buffer->mixed_lines);
            }
            if (ptr_buffer->mixed_lines->prefix_max_length_refresh)
                gui_line_compute_prefix_max_length (ptr_buffer->mixed_lines);
        }
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
            gui_bar_draw (ptr_bar);
    }

    /* refresh window if needed (if asked during refresh of bars) */
    if (gui_window_refresh_needed)
    {
        gui_window_refresh_screen ((gui_window_refresh_needed > 1) ? 1 : 0);
        gui_window_refresh_needed = 0;
    }

    /* refresh windows if needed */
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->refresh_needed)
        {
            gui_window_switch_to_buffer (ptr_win, ptr_win->buffer, 0);
            gui_chat_draw (ptr_win->buffer, 1);
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

    if (!gui_window_bare_display)
    {
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
}

/*
 * Main loop for WeeChat with ncurses GUI.
 */

void
gui_main_loop ()
{
    struct t_hook *hook_fd_keyboard;
    int send_signal_sigwinch;

    send_signal_sigwinch = 0;

    /* catch SIGWINCH signal: redraw screen */
    if (!weechat_headless)
    {
        gui_signal_sigwinch_received = 1;
        signal_catch (SIGWINCH, &gui_main_signal_sigwinch);
    }

    /* hook stdin (read keyboard) */
    if (weechat_headless)
    {
        hook_fd_keyboard = NULL;
    }
    else
    {
        hook_fd_keyboard = hook_fd (NULL, STDIN_FILENO, 1, 0, 0,
                                    &gui_key_read_cb, NULL, NULL);
    }

    gui_window_ask_refresh (1);

    while (!weechat_quit)
    {
        /* execute timer hooks */
        hook_timer_exec ();

        /* auto reset of color pairs */
        if (gui_color_pairs_auto_reset)
        {
            gui_color_reset_pairs ();
            gui_color_pairs_auto_reset_last = time (NULL);
            gui_color_pairs_auto_reset = 0;
            gui_color_pairs_auto_reset_pending = 1;
        }

        if (gui_signal_sigwinch_received)
        {
            gui_window_ask_refresh (2);
            gui_signal_sigwinch_received = 0;
            send_signal_sigwinch = 1;
        }

        gui_main_refreshes ();
        if (gui_window_refresh_needed && !gui_window_bare_display)
            gui_main_refreshes ();

        if (send_signal_sigwinch)
        {
            (void) hook_signal_send ("signal_sigwinch",
                                     WEECHAT_HOOK_SIGNAL_STRING, NULL);
            send_signal_sigwinch = 0;
        }

        gui_color_pairs_auto_reset_pending = 0;

        /* execute fd hooks */
        hook_fd_exec ();

        /* run process (with fork) */
        hook_process_exec ();

        /* handle signals received */
        signal_handle ();
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
         * final refreshes, to see messages just before exiting
         * (if we are upgrading, don't refresh anything!)
         */
        if (!weechat_upgrading)
        {
            gui_main_refreshes ();
            if (gui_window_refresh_needed)
                gui_main_refreshes ();
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
        free (gui_input_clipboard);

        /* delete layouts */
        gui_layout_remove_all ();

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
        if (CONFIG_STRING(config_look_window_title)
            && CONFIG_STRING(config_look_window_title)[0])
        {
            gui_window_set_title (NULL);
        }

        /* end color */
        gui_color_end ();

        /* free some variables used for chat area */
        gui_chat_end ();

        /* free some variables used for nicklist */
        gui_nicklist_end ();

        /* free some variables used for hotlist */
        gui_hotlist_end ();
    }

    /* end of Curses output */
    refresh ();
    endwin ();
}
