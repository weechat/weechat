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
#include "../../irc/irc.h"

#define KEY_ESCAPE 27

/*
 * gui_read_keyb: read keyboard line
 */

void
gui_read_keyb ()
{
    int key, i;
    t_gui_view *ptr_view;
    char new_char[2];

    key = getch ();
    if (key != ERR)
    {
        switch (key)
        {
            /* resize event */
            case KEY_RESIZE:
                gui_calculate_pos_size (gui_current_view);
                gui_redraw_view (gui_current_view);
                break;
            /* previous view */
            case KEY_F(6):
                gui_switch_to_previous_view ();
                break;
            /* next view */
            case KEY_F(7):
                gui_switch_to_next_view ();
                break;
            /* remove last infobar message */
            case KEY_F(10):
                gui_infobar_remove ();
                break;
            /* cursor up */
            case KEY_UP:
                if (gui_current_view->ptr_history)
                {
                    gui_current_view->ptr_history =
                        gui_current_view->ptr_history->next_history;
                    if (!gui_current_view->ptr_history)
                        gui_current_view->ptr_history =
                            gui_current_view->history;
                }
                else
                    gui_current_view->ptr_history =
                        gui_current_view->history;
                if (gui_current_view->ptr_history)
                {
                    gui_current_view->input_buffer_size =
                        strlen (gui_current_view->ptr_history->text);
                    gui_optimize_input_buffer_size (gui_current_view);
                    gui_current_view->input_buffer_pos =
                        gui_current_view->input_buffer_size;
                    strcpy (gui_current_view->input_buffer,
                        gui_current_view->ptr_history->text);
                    gui_draw_view_input (gui_current_view);
                }
                break;
            /* cursor down */
            case KEY_DOWN:
                if (gui_current_view->ptr_history)
                {
                    gui_current_view->ptr_history =
                        gui_current_view->ptr_history->prev_history;
                    if (gui_current_view->ptr_history)
                        gui_current_view->input_buffer_size =
                            strlen (gui_current_view->ptr_history->text);
                    else
                        gui_current_view->input_buffer_size = 0;
                    gui_optimize_input_buffer_size (gui_current_view);
                    gui_current_view->input_buffer_pos =
                        gui_current_view->input_buffer_size;
                    if (gui_current_view->ptr_history)
                        strcpy (gui_current_view->input_buffer,
                            gui_current_view->ptr_history->text);
                    gui_draw_view_input (gui_current_view);
                }
                break;
            /* cursor left */
            case KEY_LEFT:
                if (gui_current_view->input_buffer_pos > 0)
                {
                    gui_current_view->input_buffer_pos--;
                    gui_draw_view_input (gui_current_view);
                }
                break;
            /* cursor right */
            case KEY_RIGHT:
                if (gui_current_view->input_buffer_pos <
                    gui_current_view->input_buffer_size)
                {
                    gui_current_view->input_buffer_pos++;
                    gui_draw_view_input (gui_current_view);
                }
                break;
            /* home key */
            case KEY_HOME:
                if (gui_current_view->input_buffer_pos > 0)
                {
                    gui_current_view->input_buffer_pos = 0;
                    gui_draw_view_input (gui_current_view);
                }
                break;
            /* end key */
            case KEY_END:
                if (gui_current_view->input_buffer_pos <
                    gui_current_view->input_buffer_size)
                {
                    gui_current_view->input_buffer_pos =
                        gui_current_view->input_buffer_size;
                    gui_draw_view_input (gui_current_view);
                }
                break;
            /* page up */
            case KEY_PPAGE:
                gui_move_page_up ();
                break;
            /* page down */
            case KEY_NPAGE:
                gui_move_page_down ();
                break;
            /* erase before cursor and move cursor to the left */
            case 127:
            case KEY_BACKSPACE:
                if (gui_current_view->input_buffer_pos > 0)
                {
                    i = gui_current_view->input_buffer_pos-1;
                    while (gui_current_view->input_buffer[i])
                    {
                        gui_current_view->input_buffer[i] =
                            gui_current_view->input_buffer[i+1];
                        i++;
                    }
                    gui_current_view->input_buffer_size--;
                    gui_current_view->input_buffer_pos--;
                    gui_current_view->input_buffer[gui_current_view->input_buffer_size] = '\0';
                    gui_draw_view_input (gui_current_view);
                    gui_optimize_input_buffer_size (gui_current_view);
                    gui_current_view->completion.position = -1;
                }
                break;
            /* Control + Backspace */
            case 0x08:
                gui_delete_previous_word ();
                break;
            /* erase char under cursor */
            case KEY_DC:
                if (gui_current_view->input_buffer_pos <
                    gui_current_view->input_buffer_size)
                {
                    i = gui_current_view->input_buffer_pos;
                    while (gui_current_view->input_buffer[i])
                    {
                        gui_current_view->input_buffer[i] =
                            gui_current_view->input_buffer[i+1];
                        i++;
                    }
                    gui_current_view->input_buffer_size--;
                    gui_current_view->input_buffer[gui_current_view->input_buffer_size] = '\0';
                    gui_draw_view_input (gui_current_view);
                    gui_optimize_input_buffer_size (gui_current_view);
                    gui_current_view->completion.position = -1;
                }
                break;
            /* Tab : completion */
            case '\t':
                completion_search (&(gui_current_view->completion),
                                   CHANNEL(gui_current_view),
                                   gui_current_view->input_buffer,
                                   gui_current_view->input_buffer_size,
                                   gui_current_view->input_buffer_pos);
                if (gui_current_view->completion.word_found)
                {
                    // replace word with new completed word into input buffer
                    gui_current_view->input_buffer_size +=
                        gui_current_view->completion.diff_size;
                    gui_optimize_input_buffer_size (gui_current_view);
                    gui_current_view->input_buffer[gui_current_view->input_buffer_size] = '\0';
                    
                    if (gui_current_view->completion.diff_size > 0)
                    {
                        for (i = gui_current_view->input_buffer_size - 1;
                            i >=  gui_current_view->completion.position_replace +
                            (int)strlen (gui_current_view->completion.word_found); i--)
                            gui_current_view->input_buffer[i] =
                                gui_current_view->input_buffer[i -
                                gui_current_view->completion.diff_size];
                    }
                    else
                    {
                        for (i = gui_current_view->completion.position_replace +
                            strlen (gui_current_view->completion.word_found);
                            i < gui_current_view->input_buffer_size; i++)
                            gui_current_view->input_buffer[i] =
                                gui_current_view->input_buffer[i -
                                gui_current_view->completion.diff_size];
                    }
                    
                    strncpy (gui_current_view->input_buffer + gui_current_view->completion.position_replace,
                             gui_current_view->completion.word_found,
                             strlen (gui_current_view->completion.word_found));
                    gui_current_view->input_buffer_pos =
                        gui_current_view->completion.position_replace +
                        strlen (gui_current_view->completion.word_found);
                    gui_current_view->completion.position =
                        gui_current_view->input_buffer_pos;
                    
                    /* add space or completor to the end of completion, if needed */
                    if (gui_current_view->completion.base_word[0] == '/')
                    {
                        if (gui_current_view->input_buffer[gui_current_view->input_buffer_pos] != ' ')
                            gui_buffer_insert_string (" ",
                                                      gui_current_view->input_buffer_pos);
                        gui_current_view->completion.position++;
                        gui_current_view->input_buffer_pos++;
                    }
                    else
                    {
                        if (gui_current_view->completion.base_word_pos == 0)
                        {
                            if (strncmp (gui_current_view->input_buffer + gui_current_view->input_buffer_pos,
                                cfg_look_completor, strlen (cfg_look_completor)) != 0)
                                gui_buffer_insert_string (cfg_look_completor,
                                                          gui_current_view->input_buffer_pos);
                            gui_current_view->completion.position += strlen (cfg_look_completor);
                            gui_current_view->input_buffer_pos += strlen (cfg_look_completor);
                            if (gui_current_view->input_buffer[gui_current_view->input_buffer_pos] != ' ')
                                gui_buffer_insert_string (" ",
                                                          gui_current_view->input_buffer_pos);
                            gui_current_view->completion.position++;
                            gui_current_view->input_buffer_pos++;
                        }
                    }
                    gui_draw_view_input (gui_current_view);
                }
                break;
            /* escape code (for control-key) */
            case KEY_ESCAPE:
                if ((key = getch()) != ERR)
                {
                    switch (key)
                    {
                        case KEY_LEFT:
                            gui_switch_to_previous_view ();
                            break;
                        case KEY_RIGHT:
                            gui_switch_to_next_view ();
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
                                            gui_move_next_word ();
                                            break;
                                        /* Control + Left */
                                        case 100:
                                            gui_move_previous_word ();
                                            break;
                                    }
                                }
                            }
                            break;
                    }
                }
                break;
            /* send command/message */
            case '\n':
                if (gui_current_view->input_buffer_size > 0)
                {
                    gui_current_view->input_buffer[gui_current_view->input_buffer_size] = '\0';
                    history_add (gui_current_view, gui_current_view->input_buffer);
                    gui_current_view->input_buffer_size = 0;
                    gui_current_view->input_buffer_pos = 0;
                    gui_current_view->input_buffer_1st_display = 0;
                    gui_current_view->completion.position = -1;
                    gui_current_view->ptr_history = NULL;
                    ptr_view = gui_current_view;
                    user_command (SERVER(gui_current_view),
                                  gui_current_view->input_buffer);
                    if (ptr_view == gui_current_view)
                        gui_draw_view_input (ptr_view);
                    if (ptr_view)
                        ptr_view->input_buffer[0] = '\0';
                }
                break;
            /* other key => add to input buffer */
            default:
                /*gui_printf (gui_current_view,
                    "[Debug] key pressed = %d, as octal: %o\n", key, key);*/
                new_char[0] = key;
                new_char[1] = '\0';
                gui_buffer_insert_string (new_char,
                                          gui_current_view->input_buffer_pos);
                gui_current_view->input_buffer_pos++;
                gui_draw_view_input (gui_current_view);
                gui_current_view->completion.position = -1;
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
            gui_redraw_view_infobar (gui_current_view);
        }
        
        /* second has changed ? => count down time for infobar, if needed */
        if (local_time->tm_sec != old_sec)
        {
            old_sec = local_time->tm_sec;
            /* TODO: manage splited windows! */
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
