/*
 * Copyright (c) 2003-2005 by FlashCode <flashcode@flashtux.org>
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
#include <ncurses.h>

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/weeconfig.h"
#include "../../common/command.h"
#include "../../common/hotlist.h"
#include "../../common/fifo.h"
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
    t_irc_server *ptr_server;
    t_irc_dcc *ptr_dcc, *ptr_dcc_next;
    char new_char[3], *decoded_string;
    t_irc_dcc *dcc_selected;

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
            case KEY_F(9):
                break;
            /* previous buffer in window */
            case KEY_F(5):
                gui_switch_to_previous_buffer (gui_current_window);
                break;
            /* next buffer in window */
            case KEY_F(6):
                gui_switch_to_next_buffer (gui_current_window);
                break;
            /* previous window */
            case KEY_F(7):
                gui_switch_to_previous_window (gui_current_window);
                break;
            /* next window */
            case KEY_F(8):
                gui_switch_to_next_window (gui_current_window);
                break;
            /* remove last infobar message */
            case KEY_F(10):
                gui_infobar_remove ();
                gui_draw_buffer_infobar (gui_current_window->buffer, 1);
                break;
            case KEY_F(11):
                gui_nick_move_page_up (gui_current_window);
                break;
            case KEY_F(12):
                gui_nick_move_page_down (gui_current_window);
                break;
            /* cursor up */
            case KEY_UP:
                if (gui_current_window->buffer->dcc)
                {
                    if (dcc_list)
                    {
                        if (gui_current_window->dcc_selected
                            && ((t_irc_dcc *)(gui_current_window->dcc_selected))->prev_dcc)
                        {
                            if (gui_current_window->dcc_selected ==
                                gui_current_window->dcc_first)
                                gui_current_window->dcc_first =
                                    ((t_irc_dcc *)(gui_current_window->dcc_first))->prev_dcc;
                            gui_current_window->dcc_selected =
                                ((t_irc_dcc *)(gui_current_window->dcc_selected))->prev_dcc;
                            gui_draw_buffer_chat (gui_current_window->buffer, 1);
                            gui_draw_buffer_input (gui_current_window->buffer, 1);
                        }
                    }
                }
                else
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
                if (gui_current_window->buffer->dcc)
                {
                    if (dcc_list)
                    {
                        if (!gui_current_window->dcc_selected
                            || ((t_irc_dcc *)(gui_current_window->dcc_selected))->next_dcc)
                        {
                            if (gui_current_window->dcc_last_displayed
                                && (gui_current_window->dcc_selected ==
                                    gui_current_window->dcc_last_displayed))
                            {
                                if (gui_current_window->dcc_first)
                                    gui_current_window->dcc_first =
                                        ((t_irc_dcc *)(gui_current_window->dcc_first))->next_dcc;
                                else
                                    gui_current_window->dcc_first =
                                        dcc_list->next_dcc;
                            }
                            if (gui_current_window->dcc_selected)
                                gui_current_window->dcc_selected =
                                    ((t_irc_dcc *)(gui_current_window->dcc_selected))->next_dcc;
                            else
                                gui_current_window->dcc_selected =
                                    dcc_list->next_dcc;
                            gui_draw_buffer_chat (gui_current_window->buffer, 1);
                            gui_draw_buffer_input (gui_current_window->buffer, 1);
                        }
                    }
                }
                else
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
            /* home key or Control + A */
            case KEY_HOME:
            case 0x01:
                if (!gui_current_window->buffer->dcc)
                {
                    if (gui_current_window->buffer->input_buffer_pos > 0)
                    {
                        gui_current_window->buffer->input_buffer_pos = 0;
                        gui_draw_buffer_input (gui_current_window->buffer, 0);
                    }
                }
                break;
            /* end key or Control + E */
            case KEY_END:
            case 0x05:
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
            /* Control + Backspace or Control + W */
            case 0x08:
            case 0x17:
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
                        if (gui_current_window->buffer->completion.diff_size > 0)
                        {
                            gui_current_window->buffer->input_buffer_size +=
                            gui_current_window->buffer->completion.diff_size;
                            gui_optimize_input_buffer_size (gui_current_window->buffer);
                            gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
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
                            gui_current_window->buffer->input_buffer_size +=
                            gui_current_window->buffer->completion.diff_size;
                            gui_optimize_input_buffer_size (gui_current_window->buffer);
                            gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
                        }
                        
                        strncpy (gui_current_window->buffer->input_buffer + gui_current_window->buffer->completion.position_replace,
                                 gui_current_window->buffer->completion.word_found,
                                 strlen (gui_current_window->buffer->completion.word_found));
                        gui_current_window->buffer->input_buffer_pos =
                            gui_current_window->buffer->completion.position_replace +
                            strlen (gui_current_window->buffer->completion.word_found);
                        
                        /* position is < 0 this means only one word was found to complete,
                           so reinit to stop completion */
                        if (gui_current_window->buffer->completion.position >= 0)
                            gui_current_window->buffer->completion.position =
                                gui_current_window->buffer->input_buffer_pos;
                        
                        /* add space or completor to the end of completion, if needed */
                        if ((gui_current_window->buffer->completion.context == COMPLETION_COMMAND)
                            || (gui_current_window->buffer->completion.context == COMPLETION_COMMAND_ARG))
                        {
                            if (gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_pos] != ' ')
                                gui_buffer_insert_string (gui_current_window->buffer,
                                                          " ",
                                                          gui_current_window->buffer->input_buffer_pos);
                            if (gui_current_window->buffer->completion.position >= 0)
                                gui_current_window->buffer->completion.position++;
                            gui_current_window->buffer->input_buffer_pos++;
                        }
                        else
                        {
                            /* add nick completor if position 0 and completing nick */
                            if ((gui_current_window->buffer->completion.base_word_pos == 0)
                                && (gui_current_window->buffer->completion.context == COMPLETION_NICK))
                            {
                                if (strncmp (gui_current_window->buffer->input_buffer + gui_current_window->buffer->input_buffer_pos,
                                    cfg_look_completor, strlen (cfg_look_completor)) != 0)
                                    gui_buffer_insert_string (gui_current_window->buffer,
                                                              cfg_look_completor,
                                                              gui_current_window->buffer->input_buffer_pos);
                                if (gui_current_window->buffer->completion.position >= 0)
                                    gui_current_window->buffer->completion.position += strlen (cfg_look_completor);
                                gui_current_window->buffer->input_buffer_pos += strlen (cfg_look_completor);
                                if (gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_pos] != ' ')
                                    gui_buffer_insert_string (gui_current_window->buffer,
                                                              " ",
                                                              gui_current_window->buffer->input_buffer_pos);
                                if (gui_current_window->buffer->completion.position >= 0)
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
                        /* Alt + home */
                        case KEY_HOME:
                            gui_nick_move_beginning (gui_current_window);
                            break;
                        /* Alt + end */
                        case KEY_END:
                            gui_nick_move_end (gui_current_window);
                            break;
                        /* Alt + page up */
                        case KEY_PPAGE:
                            gui_nick_move_page_up (gui_current_window);
                            break;
                        /* Alt + page down */
                        case KEY_NPAGE:
                            gui_nick_move_page_down (gui_current_window);
                            break;
                        case 79:
                            /* TODO: replace 79 by constant name! */
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
                            break;
                        /* Alt-number: jump to window by number */
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
                        /* Alt-A: jump to buffer with activity */
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
                        /* Alt-D: jump to DCC buffer */
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
                        /* Alt-R: clear hotlist */
                        case 'r':
                        case 'R':
                            if (hotlist)
                            {
                                hotlist_free_all ();
                                gui_redraw_buffer (gui_current_window->buffer);
                            }
                            hotlist_initial_buffer = gui_current_window->buffer;
                            break;
                        /* Alt-S: jump to server buffer */
                        case 's':
                        case 'S':
                            if (!gui_current_window->buffer->dcc)
                            {
                                if (SERVER(gui_current_window->buffer)->buffer !=
                                    gui_current_window->buffer)
                                {
                                    gui_switch_to_buffer (gui_current_window,
                                                          SERVER(gui_current_window->buffer)->buffer);
                                    gui_redraw_buffer (gui_current_window->buffer);
                                }
                            }
                            break;
                        /* Alt-X: jump to first channel/private of next server */
                        case 'x':
                        case 'X':
                            if (!gui_current_window->buffer->dcc)
                            {
                                ptr_server = SERVER(gui_current_window->buffer)->next_server;
                                if (!ptr_server)
                                    ptr_server = irc_servers;
                                while (ptr_server != SERVER(gui_current_window->buffer))
                                {
                                    if (ptr_server->buffer)
                                        break;
                                    ptr_server = (ptr_server->next_server) ?
                                                 ptr_server->next_server : irc_servers;
                                }
                                if (ptr_server != SERVER(gui_current_window->buffer))
                                {
                                    ptr_buffer = (ptr_server->channels) ?
                                                 ptr_server->channels->buffer : ptr_server->buffer;
                                    gui_switch_to_buffer (gui_current_window, ptr_buffer);
                                    gui_redraw_buffer (gui_current_window->buffer);
                                }
                            }
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
                                      gui_current_window->buffer,
                                      gui_current_window->buffer->input_buffer);
                        if (ptr_buffer == gui_current_window->buffer)
                        {
                            ptr_buffer->input_buffer[0] = '\0';
                            gui_draw_buffer_input (ptr_buffer, 0);
                        }
                    }
                }
                break;
            /* other key => add to input buffer */
            default:
                if (gui_current_window->buffer->dcc)
                {
                    dcc_selected = (gui_current_window->dcc_selected) ?
                        (t_irc_dcc *) gui_current_window->dcc_selected : dcc_list;
                    switch (key)
                    {
                        /* accept DCC */
                        case 'a':
                        case 'A':
                            if (dcc_selected
                                && (DCC_IS_RECV(dcc_selected->status))
                                && (dcc_selected->status == DCC_WAITING))
                            {
                                dcc_accept (dcc_selected);
                            }
                            break;
                        /* cancel DCC */
                        case 'c':
                        case 'C':
                            if (dcc_selected
                                && (!DCC_ENDED(dcc_selected->status)))
                            {
                                dcc_close (dcc_selected, DCC_ABORTED);
                                gui_redraw_buffer (gui_current_window->buffer);
                            }
                            break;
                        /* purge old DCC */
                        case 'p':
                        case 'P':
                            gui_current_window->dcc_selected = NULL;
                            ptr_dcc = dcc_list;
                            while (ptr_dcc)
                            {
                                ptr_dcc_next = ptr_dcc->next_dcc;
                                if (DCC_ENDED(ptr_dcc->status))
                                    dcc_free (ptr_dcc);
                                ptr_dcc = ptr_dcc_next;
                            }
                            gui_redraw_buffer (gui_current_window->buffer);
                            break;
                        /* close DCC window */
                        case 'q':
                        case 'Q':
                            if (buffer_before_dcc)
                            {
                                gui_buffer_free (gui_current_window->buffer, 1);
                                gui_switch_to_buffer (gui_current_window,
                                                      buffer_before_dcc);
                            }
                            else
                                gui_buffer_free (gui_current_window->buffer, 1);
                            gui_redraw_buffer (gui_current_window->buffer);
                            break;
                        /* remove from DCC list */
                        case 'r':
                        case 'R':
                            if (dcc_selected
                                && (DCC_ENDED(dcc_selected->status)))
                            {
                                if (dcc_selected->next_dcc)
                                    gui_current_window->dcc_selected = dcc_selected->next_dcc;
                                else
                                    gui_current_window->dcc_selected = NULL;
                                dcc_free (dcc_selected);
                                gui_redraw_buffer (gui_current_window->buffer);
                            }
                            break;
                    }
                }
                else
                {
                    /*gui_printf (gui_current_window->buffer,
                                "[Debug] key pressed = %d, hex = %02X, octal = %o\n", key, key, key);*/
                    new_char[0] = key;
                    new_char[1] = '\0';
                    decoded_string = NULL;
                    
                    /* UTF-8 input */
                    if (key == 0xC3)
                    {
                        if ((key = getch()) != ERR)
                        {
                            new_char[1] = key;
                            new_char[2] = '\0';
                            decoded_string = weechat_convert_encoding (local_charset, cfg_look_charset_internal, new_char);
                        }
                    }
                    
                    gui_buffer_insert_string (gui_current_window->buffer,
                                              (decoded_string) ? decoded_string : new_char,
                                              gui_current_window->buffer->input_buffer_pos);
                    gui_current_window->buffer->input_buffer_pos++;
                    gui_draw_buffer_input (gui_current_window->buffer, 0);
                    gui_current_window->buffer->completion.position = -1;
                    
                    if (decoded_string)
                        free (decoded_string);
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
    static struct timeval timeout, tv;
    static struct timezone tz;
    t_irc_server *ptr_server;
    int old_min, old_sec, diff;
    time_t new_time;
    struct tm *local_time;

    quit_weechat = 0;
    old_min = -1;
    old_sec = -1;
    check_away = 0;
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
        
        /* second has changed ? */
        if (local_time->tm_sec != old_sec)
        {
            old_sec = local_time->tm_sec;
            
            /* infobar count down */
            if (gui_infobar && gui_infobar->remaining_time > 0)
            {
                gui_infobar->remaining_time--;
                if (gui_infobar->remaining_time == 0)
                {
                    gui_infobar_remove ();
                    gui_draw_buffer_infobar (gui_current_window->buffer, 1);
                }
            }
            
            /* away check */
            if (cfg_irc_away_check != 0)
            {
                check_away++;
                if (check_away >= (cfg_irc_away_check * 60))
                {
                    check_away = 0;
                    server_check_away ();
                }
            }
        }
        
        FD_ZERO (&read_fd);
        
        FD_SET (STDIN_FILENO, &read_fd);
        if (weechat_fifo != -1)
            FD_SET (weechat_fifo, &read_fd);
        
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000;
        
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            /* check if reconnection is pending */
            if ((!ptr_server->is_connected)
                && (ptr_server->reconnect_start > 0)
                && (new_time >= (ptr_server->reconnect_start + ptr_server->autoreconnect_delay)))
                server_reconnect (ptr_server);
            else
            {
                if (ptr_server->is_connected)
                {
                    /* check for lag */
                    if ((ptr_server->lag_check_time.tv_sec == 0)
                        && (new_time >= ptr_server->lag_next_check))
                    {
                        server_sendf (ptr_server, "PING %s\r\n", ptr_server->address);
                        gettimeofday (&(ptr_server->lag_check_time), &tz);
                    }
                    
                    /* lag timeout => disconnect */
                    if ((ptr_server->lag_check_time.tv_sec != 0)
                        && (cfg_irc_lag_disconnect > 0))
                    {
                        gettimeofday (&tv, &tz);
                        diff = (int) get_timeval_diff (&(ptr_server->lag_check_time), &tv);
                        if (diff / 1000 > cfg_irc_lag_disconnect * 60)
                        {
                            irc_display_prefix (ptr_server->buffer, PREFIX_ERROR);
                            gui_printf (ptr_server->buffer,
                                        _("%s lag is high, disconnecting from server...\n"),
                                        WEECHAT_WARNING);
                            server_disconnect (ptr_server, 1);
                            continue;
                        }
                    }
                }
            
                if (!ptr_server->is_connected && (ptr_server->child_pid > 0))
                    FD_SET (ptr_server->child_read, &read_fd);
                else
                {
                    if (ptr_server->sock >= 0)
                        FD_SET (ptr_server->sock, &read_fd);
                }
            }
        }
        
        if (select (FD_SETSIZE, &read_fd, NULL, NULL, &timeout) > 0)
        {
            if (FD_ISSET (STDIN_FILENO, &read_fd))
            {
                gui_read_keyb ();
            }
            if ((weechat_fifo != -1) && (FD_ISSET (weechat_fifo, &read_fd)))
            {
                fifo_read ();
            }
            for (ptr_server = irc_servers; ptr_server;
                 ptr_server = ptr_server->next_server)
            {
                if (!ptr_server->is_connected && (ptr_server->child_pid > 0))
                {
                    if (FD_ISSET (ptr_server->child_read, &read_fd))
                        server_child_read (ptr_server);
                }
                else
                {
                    if ((ptr_server->sock >= 0) &&
                        (FD_ISSET (ptr_server->sock, &read_fd)))
                        server_recv (ptr_server);
                }
            }
        }
        
        /* manages active DCC */
        dcc_handle ();
    }
}
