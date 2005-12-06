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

/* fifo.c: FIFO pipe for WeeChat remote control */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "weechat.h"
#include "fifo.h"
#include "command.h"
#include "weeconfig.h"
#include "../gui/gui.h"


int weechat_fifo = -1;
char *weechat_fifo_filename = NULL;
char *weechat_fifo_unterminated = NULL;


/*
 * fifo_create: create FIFO pipe for remote control
 */

void
fifo_create ()
{
    int filename_length;
    
    if (cfg_irc_fifo_pipe)
    {
        /* build FIFO filename: "~/.weechat/weechat_fifo_" + process PID */
        if (!weechat_fifo_filename)
        {
            filename_length = strlen (weechat_home) + 64;
            weechat_fifo_filename = (char *) malloc (filename_length * sizeof (char));
            snprintf (weechat_fifo_filename, filename_length, "%s/weechat_fifo_%d",
                      weechat_home, (int) getpid());
        }
        
        /* create FIFO pipe, writable for user only */
        if ((weechat_fifo = mkfifo (weechat_fifo_filename, 0600)) != 0)
        {
            weechat_fifo = -1;
            gui_printf (NULL,
                        _("%s unable to create FIFO pipe for remote control (%s)\n"),
                        WEECHAT_ERROR, weechat_fifo_filename);
            weechat_log_printf (_("%s unable to create FIFO pipe for "
                                  "remote control (%s)\n"),
                                WEECHAT_ERROR, weechat_fifo_filename);
            return;
        }
        
        /* open FIFO pipe in read-only (for WeeChat), non nlobking mode */
        if ((weechat_fifo = open (weechat_fifo_filename, O_RDONLY | O_NONBLOCK)) == -1)
        {
            gui_printf (NULL,
                        _("%s unable to open FIFO pipe (%s) for reading\n"),
                        WEECHAT_ERROR, weechat_fifo_filename);
            weechat_log_printf (_("%s unable to open FIFO pipe (%s) for reading\n"),
                                WEECHAT_ERROR, weechat_fifo_filename);
            return;
        }
        
        weechat_log_printf (_("FIFO pipe is open\n"));
    }
}

/*
 * fifo_exec: execute a command/text received by FIFO pipe
 */

void
fifo_exec (char *text)
{
    char *pos_msg, *pos;
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    t_gui_buffer *ptr_buffer;
    
    pos = NULL;
    ptr_server = NULL;
    ptr_channel = NULL;
    ptr_buffer = NULL;
    
    /* look for server/channel at beginning of text */
    /* text may be: "server,channel *text" or "server *text" or "*text" */
    if (text[0] == '*')
    {
        pos_msg = text + 1;
        ptr_buffer = (gui_current_window->buffer->has_input) ? gui_current_window->buffer : gui_buffers;
        ptr_server = SERVER(ptr_buffer);
    }
    else
    {
        pos_msg = strstr (text, " *");
        if (!pos_msg)
        {
            irc_display_prefix (NULL, NULL, PREFIX_ERROR);
            gui_printf (NULL, _("%s invalid text received on FIFO pipe\n"),
                        WEECHAT_WARNING);
            return;
        }
        pos_msg[0] = '\0';
        pos = pos_msg - 1;
        pos_msg += 2;
        while ((pos >= text) && (pos[0] == ' '))
        {
            pos[0] = '\0';
            pos--;
        }
        
        if (text[0])
        {
            pos = strchr (text, ',');
            if (pos)
                pos[0] = '\0';
            ptr_server = server_search (text);
            if (!ptr_server || !ptr_server->buffer)
            {
                irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                gui_printf (NULL, _("%s server \"%s\" not found (FIFO pipe data)\n"),
                            WEECHAT_WARNING, text);
                return;
            }
            if (ptr_server)
            {
                if (pos)
                {
                    ptr_channel = channel_search (ptr_server, pos + 1);
                    if (!ptr_channel)
                    {
                        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                        gui_printf (NULL,
                                    _("%s channel \"%s\" not found (FIFO pipe data)\n"),
                                    WEECHAT_WARNING, pos + 1);
                        return;
                    }
                }
            }
        }
    }
    
    if (!ptr_buffer)
    {
        if (ptr_channel)
            ptr_buffer = ptr_channel->buffer;
        else
            ptr_buffer = gui_buffers;
    }
    
    user_command (ptr_server, ptr_buffer, pos_msg);
}

/*
 * fifo_read: read data in FIFO pipe
 */

void
fifo_read ()
{
    static char buffer[4096 + 2];
    char *buf2, *pos, *ptr_buf, *next_ptr_buf;
    int num_read;
    
    num_read = read (weechat_fifo, buffer, sizeof (buffer) - 2);
    if (num_read > 0)
    {
        buffer[num_read] = '\0';
        
        buf2 = NULL;
        ptr_buf = buffer;
        if (weechat_fifo_unterminated)
        {
            buf2 = (char *) malloc (strlen (weechat_fifo_unterminated) +
                strlen (buffer) + 1);
            if (buf2)
            {
                strcpy (buf2, weechat_fifo_unterminated);
                strcat (buf2, buffer);
            }
            ptr_buf = buf2;
            free (weechat_fifo_unterminated);
            weechat_fifo_unterminated = NULL;
        }
        
        while (ptr_buf && ptr_buf[0])
        {
            next_ptr_buf = NULL;
            pos = strstr (ptr_buf, "\r\n");
            if (pos)
            {
                pos[0] = '\0';
                next_ptr_buf = pos + 2;
            }
            else
            {
                pos = strstr (ptr_buf, "\n");
                if (pos)
                {
                    pos[0] = '\0';
                    next_ptr_buf = pos + 1;
                }
                else
                {
                    weechat_fifo_unterminated = strdup (ptr_buf);
                    ptr_buf = NULL;
                    next_ptr_buf = NULL;
                }
            }
            
            if (ptr_buf)
                fifo_exec (ptr_buf);
            
            ptr_buf = next_ptr_buf;
        }
        
        if (buf2)
            free (buf2);
    }
    else
    {
        if (num_read < 0)
        {
            gui_printf (NULL,
                        _("%s error reading FIFO pipe, closing it\n"),
                        WEECHAT_ERROR);
            weechat_log_printf (_("%s error reading FIFO pipe, closing it\n"),
                                WEECHAT_ERROR);
            fifo_remove ();
        }
        else
        {
            close (weechat_fifo);
            weechat_fifo = open (weechat_fifo_filename, O_RDONLY | O_NONBLOCK);
        }
    }
}

/*
 * fifo_remove: remove FIFO pipe
 */

void
fifo_remove ()
{
    if (weechat_fifo != -1)
    {
        /* close FIFO pipe */
        close (weechat_fifo);
        weechat_fifo = -1;
    }
    
    /* remove FIFO from disk */
    if (weechat_fifo_filename)
        unlink (weechat_fifo_filename);
    
    if (weechat_fifo_unterminated)
    {
        free (weechat_fifo_unterminated);
        weechat_fifo_unterminated = NULL;
    }
    
    if (weechat_fifo_filename)
    {
        free (weechat_fifo_filename);
        weechat_fifo_filename = NULL;
    }
    
    weechat_log_printf (_("FIFO pipe is closed\n"));
}
