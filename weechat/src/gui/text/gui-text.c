/*
 * Copyright (c) 2003 by FlashCode <flashcode@flashtux.org>
 *                       Bounga <bounga@altern.org>
 *                       Xahlexx <xahlexx@tuxisland.org>
 * See README for License detail.
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


/* gui-text.c: text GUI - display functions */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>

#include "weechat.h"
#include "gui-text.h"
#include "command.h"
#include "irc.h"


/*
 * gui_init: init GUI
 */

void
gui_init ()
{
}


/*
 * gui_init_irc_window: allocates a window for a channel or server
 */

void
gui_init_irc_window (t_irc_window * window)
{
    /* no window in text GUI */
    window->text = NULL;
    window->window = NULL;
}


/*
 * gui_free_irc_window: free a GUI window
 */

void
gui_free_irc_window (t_irc_window * window)
{
    /* no window in text GUI */
}


/*
 * gui_end: GUI end
 */

void
gui_end ()
{
}


/*
 * read_keyb: read keyboard line
 */

void
read_keyb ()
{
    int num_read;
    static char buffer[4096];
    static int pos_buffer = 0;
    char buffer_tmp[1024];
    int pos_buffer_tmp;

    num_read = read (STDIN_FILENO, buffer_tmp, sizeof (buffer_tmp) - 1);
    pos_buffer_tmp = 0;
    while (pos_buffer_tmp < num_read)
    {
        switch (buffer_tmp[pos_buffer_tmp])
        {
        case '\r':
            break;
        case '\n':
            buffer[pos_buffer] = '\0';
            pos_buffer = 0;
            user_command (buffer);
            break;
        default:
            buffer[pos_buffer] = buffer_tmp[pos_buffer_tmp];
            if (pos_buffer < (int) (sizeof (buffer) - 2))
                pos_buffer++;
        }
        pos_buffer_tmp++;
    }
}


/*
 * gui_main_loop: main loop for WeeChat with text GUI
 */

void
gui_main_loop ()
{
    struct timeval timeout;
    fd_set read_fd;
    t_irc_server *ptr_server;

    quit_weechat = 0;
    while (!quit_weechat)
    {
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000;
        FD_ZERO (&read_fd);
        FD_SET (STDIN_FILENO, &read_fd);
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            FD_SET (ptr_server->sock4, &read_fd);
        }
        select (FD_SETSIZE, &read_fd, NULL, NULL, &timeout);
        if (FD_ISSET (STDIN_FILENO, &read_fd))
        {
            read_keyb ();
        }
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (FD_ISSET (ptr_server->sock4, &read_fd))
                recv_from_server (ptr_server);
        }
    }
}


/*
 * gui_display_message: display a message on the screen
 */

void
gui_display_message (char *message)
{
    printf ("%s\n", message);
}
