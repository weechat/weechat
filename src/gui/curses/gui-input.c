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
#include <ncursesw/ncurses.h>

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/weeconfig.h"
#include "../../common/command.h"
#include "../../common/hotlist.h"
#include "../../common/fifo.h"
#include "../../common/utf8.h"
#include "../../irc/irc.h"


/*
 * gui_input_default_key_bindings: create default key bindings
 */

void
gui_input_default_key_bindings ()
{
    int i;
    char key_str[32], command[32];
    
    /* keys binded with internal functions */
    gui_key_bind ( /* RC      */ "ctrl-M",        "return");
    gui_key_bind ( /* RC      */ "ctrl-J",        "return");
    gui_key_bind ( /* tab     */ "ctrl-I",        "tab");
    gui_key_bind ( /* basckp  */ "ctrl-H",        "backspace");
    gui_key_bind ( /* basckp  */ "ctrl-?",        "backspace");
    gui_key_bind ( /* del     */ "meta2-3~",      "delete");
    gui_key_bind ( /* ^K      */ "ctrl-K",        "delete_end_line");
    gui_key_bind ( /* ^U      */ "ctrl-U",        "delete_beginning_line");
    gui_key_bind ( /* ^W      */ "ctrl-W",        "delete_previous_word");
    gui_key_bind ( /* ^Y      */ "ctrl-Y",        "clipboard_paste");
    gui_key_bind ( /* ^T      */ "ctrl-T",        "transpose_chars");
    gui_key_bind ( /* home    */ "meta2-1~",      "home");
    gui_key_bind ( /* home    */ "meta2-H",       "home");
    gui_key_bind ( /* home    */ "meta2-7~",      "home");
    gui_key_bind ( /* ^A      */ "ctrl-A",        "home");
    gui_key_bind ( /* end     */ "meta2-4~",      "end");
    gui_key_bind ( /* end     */ "meta2-F",       "end");
    gui_key_bind ( /* end     */ "meta2-8~",      "end");
    gui_key_bind ( /* ^E      */ "ctrl-E",        "end");
    gui_key_bind ( /* left    */ "meta2-D",       "left");
    gui_key_bind ( /* right   */ "meta2-C",       "right");
    gui_key_bind ( /* up      */ "meta2-A",       "up");
    gui_key_bind ( /* ^up     */ "meta-Oa",       "up_global");
    gui_key_bind ( /* down    */ "meta2-B",       "down");
    gui_key_bind ( /* ^down   */ "meta-Ob",       "down_global");
    gui_key_bind ( /* pgup    */ "meta2-5~",      "page_up");
    gui_key_bind ( /* pgdn    */ "meta2-6~",      "page_down");
    gui_key_bind ( /* F10     */ "meta2-21~",     "infobar_clear");
    gui_key_bind ( /* F11     */ "meta2-23~",     "nick_page_up");
    gui_key_bind ( /* F12     */ "meta2-24~",     "nick_page_down");
    gui_key_bind ( /* m-F11   */ "meta-meta2-1~", "nick_beginning");
    gui_key_bind ( /* m-F12   */ "meta-meta2-4~", "nick_end");
    gui_key_bind ( /* ^L      */ "ctrl-L",        "refresh");
    gui_key_bind ( /* m-a     */ "meta-a",        "jump_smart");
    gui_key_bind ( /* m-b     */ "meta-b",        "previous_word");
    gui_key_bind ( /* ^left   */ "meta-Od",       "previous_word");
    gui_key_bind ( /* m-d     */ "meta-d",        "delete_next_word");
    gui_key_bind ( /* m-f     */ "meta-f",        "next_word");
    gui_key_bind ( /* ^right  */ "meta-Oc",       "next_word");
    gui_key_bind ( /* m-h     */ "meta-h",        "hotlist_clear");
    gui_key_bind ( /* m-j,m-d */ "meta-jmeta-d",  "jump_dcc");
    gui_key_bind ( /* m-j,m-l */ "meta-jmeta-l",  "jump_last_buffer");
    gui_key_bind ( /* m-j,m-s */ "meta-jmeta-s",  "jump_server");
    gui_key_bind ( /* m-j,m-x */ "meta-jmeta-x",  "jump_next_server");
    gui_key_bind ( /* m-k     */ "meta-k",        "grab_key");
    gui_key_bind ( /* m-r     */ "meta-r",        "delete_line");
    
    /* keys binded with commands */
    gui_key_bind ( /* m-left  */ "meta-meta2-D", "/buffer -1");
    gui_key_bind ( /* F5      */ "meta2-15~",    "/buffer -1");
    gui_key_bind ( /* m-right */ "meta-meta2-C", "/buffer +1");
    gui_key_bind ( /* F6      */ "meta2-17~",    "/buffer +1");
    gui_key_bind ( /* F7      */ "meta2-18~",    "/window -1");
    gui_key_bind ( /* F8      */ "meta2-19~",    "/window +1");
    gui_key_bind ( /* m-0     */ "meta-0",       "/buffer 10");
    gui_key_bind ( /* m-1     */ "meta-1",       "/buffer 1");
    gui_key_bind ( /* m-2     */ "meta-2",       "/buffer 2");
    gui_key_bind ( /* m-3     */ "meta-3",       "/buffer 3");
    gui_key_bind ( /* m-4     */ "meta-4",       "/buffer 4");
    gui_key_bind ( /* m-5     */ "meta-5",       "/buffer 5");
    gui_key_bind ( /* m-6     */ "meta-6",       "/buffer 6");
    gui_key_bind ( /* m-7     */ "meta-7",       "/buffer 7");
    gui_key_bind ( /* m-8     */ "meta-8",       "/buffer 8");
    gui_key_bind ( /* m-9     */ "meta-9",       "/buffer 9");
    
    /* bind meta-j + {01..99} to switch to buffers # > 10 */
    for (i = 1; i < 100; i++)
    {
        sprintf (key_str, "meta-j%02d", i);
        sprintf (command, "/buffer %d", i);
        gui_key_bind (key_str, command);
    }
}

/*
 * gui_input_grab_end: insert grabbed key in input buffer
 */

void
gui_input_grab_end ()
{
    char *expanded_key;

    /* get expanded name (for example: ^U => ctrl-u) */
    expanded_key = gui_key_get_expanded_name (gui_key_buffer);
    
    if (expanded_key)
    {
        if (gui_current_window->buffer->has_input)
        {
            gui_input_insert_string (gui_current_window, expanded_key, -1);
            gui_current_window->buffer->input_buffer_pos += strlen (expanded_key);
            gui_draw_buffer_input (gui_current_window->buffer, 1);
        }
        free (expanded_key);
    }
    
    /* end grab mode */
    gui_key_grab = 0;
    gui_key_grab_count = 0;
    gui_key_buffer[0] = '\0';
}

/*
 * gui_input_read: read keyboard chars
 */

void
gui_input_read ()
{
    int key, i, insert_ok;
    char key_str[32];
    
    i = 0;
    /* do not loop too much here (for example when big paste was made),
       to read also socket & co */
    while (i < 8)
    {
        if (gui_key_grab && (gui_key_grab_count > 10))
            gui_input_grab_end ();
        
        key = getch ();
        insert_ok = 1;
        
        if (key == ERR)
        {
            if (gui_key_grab && (gui_key_grab_count > 0))
                gui_input_grab_end ();
            break;
        }
        
        if (key == KEY_RESIZE)
        {
            gui_curses_resize_handler ();
            continue;
        }
                
        if (key < 32)
        {
            insert_ok = 0;
            key_str[0] = '^';
            key_str[1] = (char) key + '@';
            key_str[2] = '\0';
        }
        else if (key == 127)
        {
            key_str[0] = '^';
            key_str[1] = '?';
            key_str[2] = '\0';
        }
        else
        {
            if (local_utf8)
            {
                /* 1 char: 0vvvvvvv */
                if (key < 0x80)
                {
                    key_str[0] = (char) key;
                    key_str[1] = '\0';
                }
                /* 2 chars: 110vvvvv 10vvvvvv */
                else if ((key & 0xE0) == 0xC0)
                {
                    key_str[0] = (char) key;
                    key_str[1] = (char) (getch ());
                    key_str[2] = '\0';
                }
                 /* 3 chars: 1110vvvv 10vvvvvv 10vvvvvv */
                else if ((key & 0xF0) == 0xE0)
                {
                    key_str[0] = (char) key;
                    key_str[1] = (char) (getch ());
                    key_str[2] = (char) (getch ());
                    key_str[3] = '\0';
                }
                /* 4 chars: 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv */
                else if ((key & 0xF8) == 0xF0)
                {
                    key_str[0] = (char) key;
                    key_str[1] = (char) (getch ());
                    key_str[2] = (char) (getch ());
                    key_str[3] = (char) (getch ());
                    key_str[4] = '\0';
                }
            }
            else
            {
                key_str[0] = (char) key;
                key_str[1] = '\0';
            }
        }
        
        if (strcmp (key_str, "^") == 0)
        {
            key_str[1] = '^';
            key_str[2] = '\0';
        }
        
        /*gui_printf (gui_current_window->buffer, "gui_input_read: key = %s (%d)\n", key_str, key);*/
        
        if ((gui_key_pressed (key_str) != 0) && (insert_ok))
        {
            if (strcmp (key_str, "^^") == 0)
                key_str[1] = '\0';
            
            if (gui_current_window->buffer->dcc)
                gui_input_action_dcc (gui_current_window, key_str);
            else
            {
                gui_input_insert_string (gui_current_window, key_str, -1);
                gui_current_window->buffer->input_buffer_pos += utf8_strlen (key_str);
                gui_draw_buffer_input (gui_current_window->buffer, 0);
                gui_current_window->buffer->completion.position = -1;
            }
        }
        
        i++;
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
    t_gui_buffer *ptr_buffer;
    int old_day, old_min, old_sec, diff;
    char text_time[1024];
    time_t new_time;
    struct tm *local_time;

    quit_weechat = 0;

    new_time = time (NULL);
    local_time = localtime (&new_time);
    old_day = local_time->tm_mday;
    
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
            
            if (cfg_look_day_change
                && (local_time->tm_mday != old_day))
            {
                strftime (text_time, sizeof (text_time),
                          cfg_look_day_change_timestamp, local_time);
                gui_add_hotlist = 0;
                for (ptr_buffer = gui_buffers; ptr_buffer;
                     ptr_buffer = ptr_buffer->next_buffer)
                {
                    if (!ptr_buffer->dcc)
                        gui_printf_nolog_notime (ptr_buffer,
                                                 _("Day changed to %s\n"),
                                                 text_time);
                }
                gui_add_hotlist = 1;
            }
            old_day = local_time->tm_mday;
        }
        
        /* second has changed ? */
        if (local_time->tm_sec != old_sec)
        {
            old_sec = local_time->tm_sec;
            
            if (cfg_look_infobar_seconds)
            {
                gui_draw_buffer_infobar_time (gui_current_window->buffer);
                wmove (gui_current_window->win_input,
                       0, gui_current_window->win_input_x);
                wrefresh (gui_current_window->win_input);
            }
            
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
                gui_input_read ();
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
