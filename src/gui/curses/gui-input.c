/*
 * Copyright (c) 2004 by FlashCode <flashcode@flashtux.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* gui-input: user input functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>
#include <curses.h>

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/weeconfig.h"
#include "../../common/command.h"
#include "../../common/hotlist.h"
#include "../../irc/irc.h"

#define KEY_ESCAPE 27

/*
 * gui_read_keyb: read keyboard line
 */

void
gui_read_keyb ()
{
    int key, i;
    t_gui_buffer *ptr_buffer;
    char new_char[2];

    key = getch ();
    if (key != ERR)
    {
        switch (key)
        {
            /* resize event */
            case KEY_RESIZE:
                gui_curses_resize_handler ();
                break;
            /* inactive function keys */
            case KEY_F(1):
            case KEY_F(2):
            case KEY_F(3):
            case KEY_F(4):
            case KEY_F(5):
            case KEY_F(9):
            case KEY_F(11):
            case KEY_F(12):
                break;
            /* previous buffer in window */
            case KEY_F(6):
                gui_switch_to_previous_buffer (gui_current_window);
                break;
            /* next buffer in window */
            case KEY_F(7):
                gui_switch_to_next_buffer (gui_current_window);
                break;
            /* next window */
            case KEY_F(8):
                gui_switch_to_next_window (gui_current_window);
                break;
            /* remove last infobar message */
            case KEY_F(10):
                gui_infobar_remove ();
                break;
            /* cursor up */
            case KEY_UP:
                if (!gui_current_window->buffer->dcc)
                {
                    if (gui_current_window->buffer->ptr_history)
                    {
                        gui_current_window->buffer->ptr_history =
                            gui_current_window->buffer->ptr_history->next_history;
                        if (!gui_current_window->buffer->ptr_history)
                            gui_current_window->buffer->ptr_history =
                                gui_current_window->buffer->history;
                    }
                    else
                        gui_current_window->buffer->ptr_history =
                            gui_current_window->buffer->history;
                    if (gui_current_window->buffer->ptr_history)
                    {
                        gui_current_window->buffer->input_buffer_size =
                            strlen (gui_current_window->buffer->ptr_history->text);
                        gui_optimize_input_buffer_size (gui_current_window->buffer);
                        gui_current_window->buffer->input_buffer_pos =
                            gui_current_window->buffer->input_buffer_size;
                        strcpy (gui_current_window->buffer->input_buffer,
                            gui_current_window->buffer->ptr_history->text);
                        gui_draw_buffer_input (gui_current_window->buffer, 0);
                    }
                }
                break;
            /* cursor down */
            case KEY_DOWN:
                if (!gui_current_window->buffer->dcc)
                {
                    if (gui_current_window->buffer->ptr_history)
                    {
                        gui_current_window->buffer->ptr_history =
                            gui_current_window->buffer->ptr_history->prev_history;
                        if (gui_current_window->buffer->ptr_history)
                            gui_current_window->buffer->input_buffer_size =
                                strlen (gui_current_window->buffer->ptr_history->text);
                        else
                            gui_current_window->buffer->input_buffer_size = 0;
                        gui_optimize_input_buffer_size (gui_current_window->buffer);
                        gui_current_window->buffer->input_buffer_pos =
                            gui_current_window->buffer->input_buffer_size;
                        if (gui_current_window->buffer->ptr_history)
                            strcpy (gui_current_window->buffer->input_buffer,
                                gui_current_window->buffer->ptr_history->text);
                        gui_draw_buffer_input (gui_current_window->buffer, 0);
                    }
                }
                break;
            /* cursor left */
            case KEY_LEFT:
                if (!gui_current_window->buffer->dcc)
                {
                    if (gui_current_window->buffer->input_buffer_pos > 0)
                    {
                        gui_current_window->buffer->input_buffer_pos--;
                        gui_draw_buffer_input (gui_current_window->buffer, 0);
                    }
                }
                break;
            /* cursor right */
            case KEY_RIGHT:
                if (!gui_current_window->buffer->dcc)
                {
                    if (gui_current_window->buffer->input_buffer_pos <
                        gui_current_window->buffer->input_buffer_size)
                    {
                        gui_current_window->buffer->input_buffer_pos++;
                        gui_draw_buffer_input (gui_current_window->buffer, 0);
                    }
                }
                break;
            /* home key */
            case KEY_HOME:
                if (!gui_current_window->buffer->dcc)
                {
                    if (gui_current_window->buffer->input_buffer_pos > 0)
                    {
                        gui_current_window->buffer->input_buffer_pos = 0;
                        gui_draw_buffer_input (gui_current_window->buffer, 0);
                    }
                }
                break;
            /* end key */
            case KEY_END:
                if (!gui_current_window->buffer->dcc)
                {
                    if (gui_current_window->buffer->input_buffer_pos <
                        gui_current_window->buffer->input_buffer_size)
                    {
                        gui_current_window->buffer->input_buffer_pos =
                            gui_current_window->buffer->input_buffer_size;
                        gui_draw_buffer_input (gui_current_window->buffer, 0);
                    }
                }
                break;
            /* page up */
            case KEY_PPAGE:
                if (!gui_current_window->buffer->dcc)
                    gui_move_page_up (gui_current_window);
                break;
            /* page down */
            case KEY_NPAGE:
                if (!gui_current_window->buffer->dcc)
                    gui_move_page_down (gui_current_window);
                break;
            /* erase before cursor and move cursor to the left */
            case 127:
            case KEY_BACKSPACE:
                if (!gui_current_window->buffer->dcc)
                {
                    if (gui_current_window->buffer->input_buffer_pos > 0)
                    {
                        i = gui_current_window->buffer->input_buffer_pos-1;
                        while (gui_current_window->buffer->input_buffer[i])
                        {
                            gui_current_window->buffer->input_buffer[i] =
                                gui_current_window->buffer->input_buffer[i+1];
                            i++;
                        }
                        gui_current_window->buffer->input_buffer_size--;
                        gui_current_window->buffer->input_buffer_pos--;
                        gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
                        gui_draw_buffer_input (gui_current_window->buffer, 0);
                        gui_optimize_input_buffer_size (gui_current_window->buffer);
                        gui_current_window->buffer->completion.position = -1;
                    }
                }
                break;
            /* Control + Backspace */
            case 0x08:
                if (!gui_current_window->buffer->dcc)    
                    gui_delete_previous_word (gui_current_window->buffer);
                break;
            /* Control + L */
            case 0x0C:
                gui_curses_resize_handler ();
                break;
            /* erase char under cursor */
            case KEY_DC:
                if (!gui_current_window->buffer->dcc)
                {
                    if (gui_current_window->buffer->input_buffer_pos <
                        gui_current_window->buffer->input_buffer_size)
                    {
                        i = gui_current_window->buffer->input_buffer_pos;
                        while (gui_current_window->buffer->input_buffer[i])
                        {
                            gui_current_window->buffer->input_buffer[i] =
                                gui_current_window->buffer->input_buffer[i+1];
                            i++;
                        }
                        gui_current_window->buffer->input_buffer_size--;
                        gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
                        gui_draw_buffer_input (gui_current_window->buffer, 0);
                        gui_optimize_input_buffer_size (gui_current_window->buffer);
                        gui_current_window->buffer->completion.position = -1;
                    }
                }
                break;
            /* Tab : completion */
            case '\t':
                if (!gui_current_window->buffer->dcc)
                {
                    completion_search (&(gui_current_window->buffer->completion),
                                       CHANNEL(gui_current_window->buffer),
                                       gui_current_window->buffer->input_buffer,
                                       gui_current_window->buffer->input_buffer_size,
                                       gui_current_window->buffer->input_buffer_pos);
                    if (gui_current_window->buffer->completion.word_found)
                    {
                        /* replace word with new completed word into input buffer */
                        gui_current_window->buffer->input_buffer_size +=
                            gui_current_window->buffer->completion.diff_size;
                        gui_optimize_input_buffer_size (gui_current_window->buffer);
                        gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
                        
                        if (gui_current_window->buffer->completion.diff_size > 0)
                        {
                            for (i = gui_current_window->buffer->input_buffer_size - 1;
                                i >=  gui_current_window->buffer->completion.position_replace +
                                (int)strlen (gui_current_window->buffer->completion.word_found); i--)
                                gui_current_window->buffer->input_buffer[i] =
                                    gui_current_window->buffer->input_buffer[i -
                                    gui_current_window->buffer->completion.diff_size];
                        }
                        else
                        {
                            for (i = gui_current_window->buffer->completion.position_replace +
                                strlen (gui_current_window->buffer->completion.word_found);
                                i < gui_current_window->buffer->input_buffer_size; i++)
                                gui_current_window->buffer->input_buffer[i] =
                                    gui_current_window->buffer->input_buffer[i -
                                    gui_current_window->buffer->completion.diff_size];
                        }
                        
                        strncpy (gui_current_window->buffer->input_buffer + gui_current_window->buffer->completion.position_replace,
                                 gui_current_window->buffer->completion.word_found,
                                 strlen (gui_current_window->buffer->completion.word_found));
                        gui_current_window->buffer->input_buffer_pos =
                            gui_current_window->buffer->completion.position_replace +
                            strlen (gui_current_window->buffer->completion.word_found);
                        gui_current_window->buffer->completion.position =
                            gui_current_window->buffer->input_buffer_pos;
                        
                        /* add space or completor to the end of completion, if needed */
                        if (gui_current_window->buffer->completion.base_word[0] == '/')
                        {
                            if (gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_pos] != ' ')
                                gui_buffer_insert_string (gui_current_window->buffer,
                                                          " ",
                                                          gui_current_window->buffer->input_buffer_pos);
                            gui_current_window->buffer->completion.position++;
                            gui_current_window->buffer->input_buffer_pos++;
                        }
                        else
                        {
                            if (gui_current_window->buffer->completion.base_word_pos == 0)
                            {
                                if (strncmp (gui_current_window->buffer->input_buffer + gui_current_window->buffer->input_buffer_pos,
                                    cfg_look_completor, strlen (cfg_look_completor)) != 0)
                                    gui_buffer_insert_string (gui_current_window->buffer,
                                                              cfg_look_completor,
                                                              gui_current_window->buffer->input_buffer_pos);
                                gui_current_window->buffer->completion.position += strlen (cfg_look_completor);
                                gui_current_window->buffer->input_buffer_pos += strlen (cfg_look_completor);
                                if (gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_pos] != ' ')
                                    gui_buffer_insert_string (gui_current_window->buffer,
                                                              " ",
                                                              gui_current_window->buffer->input_buffer_pos);
                                gui_current_window->buffer->completion.position++;
                                gui_current_window->buffer->input_buffer_pos++;
                            }
                        }
                        gui_draw_buffer_input (gui_current_window->buffer, 0);
                    }
                }
                break;
            /* escape code (for control-key) */
            case KEY_ESCAPE:
                if ((key = getch()) != ERR)
                {
                    /*gui_printf (gui_current_window->buffer,
                        "[Debug] key pressed = %d, hex = %02X, octal = %o\n", key, key, key);*/
                    switch (key)
                    {
                        /* Alt + left arrow */
                        case KEY_LEFT:
                            gui_switch_to_previous_buffer (gui_current_window);
                            break;
                        /* Alt + right arrow */
                        case KEY_RIGHT:
                            gui_switch_to_next_buffer (gui_current_window);
                            break;
                        case 79:
                            /* TODO: replace 79 by constant name! */
                            if (key == 79)
                            {
                                if ((key = getch()) != ERR)
                                {
                                    switch (key)
                                    {
                                        /* Control + Right */
                                        case 99:
                                            if (!gui_current_window->buffer->dcc)    
                                                gui_move_next_word (gui_current_window->buffer);
                                            break;
                                        /* Control + Left */
                                        case 100:
                                            if (!gui_current_window->buffer->dcc)
                                                gui_move_previous_word (gui_current_window->buffer);
                                            break;
                                    }
                                }
                            }
                            break;
                        /* Alt-number */
                        case 48: /* Alt-0 */
                        case 49: /* Alt-1 */
                        case 50: /* Alt-2 */
                        case 51: /* Alt-3 */
                        case 52: /* Alt-4 */
                        case 53: /* Alt-5 */
                        case 54: /* Alt-6 */
                        case 55: /* Alt-7 */
                        case 56: /* Alt-8 */
                        case 57: /* Alt-9 */
                            gui_switch_to_buffer_by_number (gui_current_window, (key == 48) ? 10 : key - 48);
                            break;
                        /* Alt-A */
                        case 'a':
                        case 'A':
                            if (hotlist)
                            {
                                if (!hotlist_initial_buffer)
                                    hotlist_initial_buffer = gui_current_window->buffer;
                                gui_switch_to_buffer (gui_current_window, hotlist->buffer);
                                gui_redraw_buffer (gui_current_window->buffer);
                            }
                            else
                            {
                                if (hotlist_initial_buffer)
                                {
                                    gui_switch_to_buffer (gui_current_window, hotlist_initial_buffer);
                                    gui_redraw_buffer (gui_current_window->buffer);
                                    hotlist_initial_buffer = NULL;
                                }
                            }
                            break;
                        /* Alt-D */
                        case 'd':
                        case 'D':
                            if (gui_current_window->buffer->dcc)
                            {
                                if (buffer_before_dcc)
                                {
                                    gui_switch_to_buffer (gui_current_window,
                                                          buffer_before_dcc);
                                    gui_redraw_buffer (gui_current_window->buffer);
                                }
                            }
                            else
                            {
                                buffer_before_dcc = gui_current_window->buffer;
                                gui_switch_to_dcc_buffer ();
                            }
                            break;
                        /* Alt-R */
                        case 'r':
                        case 'R':
                            if (hotlist)
                            {
                                hotlist_free_all ();
                                gui_redraw_buffer (gui_current_window->buffer);
                            }
                            hotlist_initial_buffer = gui_current_window->buffer;
                            break;
                    }
                }
                break;
            /* send command/message */
            case '\n':
                if (!gui_current_window->buffer->dcc)    
                {
                    if (gui_current_window->buffer->input_buffer_size > 0)
                    {
                        gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
                        history_add (gui_current_window->buffer, gui_current_window->buffer->input_buffer);
                        gui_current_window->buffer->input_buffer_size = 0;
                        gui_current_window->buffer->input_buffer_pos = 0;
                        gui_current_window->buffer->input_buffer_1st_display = 0;
                        gui_current_window->buffer->completion.position = -1;
                        gui_current_window->buffer->ptr_history = NULL;
                        ptr_buffer = gui_current_window->buffer;
                        user_command (SERVER(gui_current_window->buffer),
                                      gui_current_window->buffer->input_buffer);
                        if (ptr_buffer == gui_current_window->buffer)
                            gui_draw_buffer_input (ptr_buffer, 0);
                        if (ptr_buffer)
                            ptr_buffer->input_buffer[0] = '\0';
                    }
                }
                break;
            /* other key => add to input buffer */
            default:
                if (!gui_current_window->buffer->dcc)
                {
                    /*gui_printf (gui_current_window->buffer,
                                "[Debug] key pressed = %d, hex = %02X, octal = %o\n", key, key, key);*/
                    new_char[0] = key;
                    new_char[1] = '\0';
                    gui_buffer_insert_string (gui_current_window->buffer,
                                              new_char,
                                              gui_current_window->buffer->input_buffer_pos);
                    gui_current_window->buffer->input_buffer_pos++;
                    gui_draw_buffer_input (gui_current_window->buffer, 0);
                    gui_current_window->buffer->completion.position = -1;
                }
                break;
        }
    }
}

/*
 * gui_main_loop: main loop for WeeChat with ncurses GUI
 */

void
gui_main_loop ()
{
    fd_set read_fd;
    static struct timeval timeout;
    t_irc_server *ptr_server;
    int old_min, old_sec;
    time_t new_time;
    struct tm *local_time;

    quit_weechat = 0;
    old_min = -1;
    old_sec = -1;
    while (!quit_weechat)
    {
        new_time = time (NULL);
        local_time = localtime (&new_time);
        
        /* minute has changed ? => redraw infobar */
        if (local_time->tm_min != old_min)
        {
            old_min = local_time->tm_min;
            gui_draw_buffer_infobar (gui_current_window->buffer, 1);
        }
        
        /* second has changed ? => count down time for infobar, if needed */
        if (local_time->tm_sec != old_sec)
        {
            old_sec = local_time->tm_sec;
            if (gui_infobar && gui_infobar->remaining_time > 0)
            {
                gui_infobar->remaining_time--;
                if (gui_infobar->remaining_time == 0)
                    gui_infobar_remove ();
            }
        }
        
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000;
        FD_ZERO (&read_fd);
        FD_SET (STDIN_FILENO, &read_fd);
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (ptr_server->sock4 >= 0)
                FD_SET (ptr_server->sock4, &read_fd);
        }
        if (select (FD_SETSIZE, &read_fd, NULL, NULL, &timeout))
        {
            if (FD_ISSET (STDIN_FILENO, &read_fd))
            {
                gui_read_keyb ();
            }
            else
            {
                for (ptr_server = irc_servers; ptr_server;
                     ptr_server = ptr_server->next_server)
                {
                    if ((ptr_server->sock4 >= 0) &&
                        (FD_ISSET (ptr_server->sock4, &read_fd)))
                        server_recv (ptr_server);
                }
            }
        }
    }
}
