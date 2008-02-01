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

/* irc-buffer.c: manages buffers for IRC protocol */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-buffer.h"
#include "irc-channel.h"
#include "irc-command.h"
#include "irc-server.h"


/*
 * irc_buffer_merge_servers: merge server buffers in one buffer
 */

/*
void
irc_buffer_merge_servers (t_gui_window *window)
{
    t_gui_buffer *ptr_buffer_server, *ptr_buffer, *new_ptr_buffer;
    t_irc_server *ptr_server;
    
    // new server buffer is the first server buffer found
    for (ptr_buffer_server = gui_buffers; ptr_buffer_server;
         ptr_buffer_server = ptr_buffer_server->next_buffer)
    {
        if ((ptr_buffer_server->protocol == irc_protocol)
            && (IRC_BUFFER_SERVER(ptr_buffer_server))
            && (!IRC_BUFFER_CHANNEL(ptr_buffer_server)))
            break;
    }
    
    // no server buffer found
    if (!ptr_buffer_server)
        return;
    
    ptr_buffer = gui_buffers;
    while (ptr_buffer)
    {
        if ((ptr_buffer->protocol == irc_protocol)
            && (ptr_buffer != ptr_buffer_server)
            && (IRC_BUFFER_SERVER(ptr_buffer))
            && (!IRC_BUFFER_CHANNEL(ptr_buffer)))
        {
            ptr_server = IRC_BUFFER_SERVER(ptr_buffer);
            
            // add (by pointer artefact) lines from buffer found to server buffer
            if (ptr_buffer->lines)
            {
                if (ptr_buffer_server->lines)
                {
                    ptr_buffer->lines->prev_line =
                        ptr_buffer_server->last_line;
                    ptr_buffer_server->last_line->next_line =
                        ptr_buffer->lines;
                    ptr_buffer_server->last_line =
                        ptr_buffer->last_line;
                }
                else
                {
                    ptr_buffer_server->lines = ptr_buffer->lines;
                    ptr_buffer_server->last_line = ptr_buffer->last_line;
                }
            }
            
            // free buffer but not lines, because they're now used by 
            // our unique server buffer
            new_ptr_buffer = ptr_buffer->next_buffer;
            ptr_buffer->lines = NULL;
            gui_buffer_free (ptr_buffer, 1);
            ptr_buffer = new_ptr_buffer;
            
            // asociate server with new server buffer
            ptr_server->buffer = ptr_buffer_server;
        }
        else
            ptr_buffer = ptr_buffer->next_buffer;
    }
    
    IRC_BUFFER_ALL_SERVERS(ptr_buffer_server) = 1;
    gui_window_redraw_buffer (window->buffer);
}
*/

/*
 * irc_buffer_split_server: split the server buffer into many buffers (one by server)
 */

/*
void
irc_buffer_split_server (t_gui_window *window)
{
    t_gui_buffer *ptr_buffer;
    t_irc_server *ptr_server;
    char *log_filename;
    
    ptr_buffer = gui_buffer_servers_search ();
    
    if (ptr_buffer)
    {
        if (IRC_BUFFER_SERVER(ptr_buffer))
        {
            for (ptr_server = irc_servers; ptr_server;
                 ptr_server = ptr_server->next_server)
            {
                if (ptr_server->buffer
                    && (ptr_server != IRC_BUFFER_SERVER(ptr_buffer))
                    && (ptr_server->buffer == ptr_buffer))
                {
                    ptr_server->buffer = NULL;
                    log_filename = irc_log_get_filename (ptr_server->name,
                                                         NULL,
                                                         0);
                    gui_buffer_new (window, 0,
                                    ptr_server->name,
                                    ptr_server->name,
                                    GUI_BUFFER_ATTRIB_TEXT |
                                    GUI_BUFFER_ATTRIB_INPUT |
                                    GUI_BUFFER_ATTRIB_NICKS,
                                    irc_protocol,
                                    irc_buffer_data_create (ptr_server),
                                    &irc_buffer_data_free,
                                    GUI_NOTIFY_LEVEL_DEFAULT,
                                    NULL, ptr_server->nick,
                                    irc_cfg_log_auto_server, log_filename,
                                    0);
                    if (log_filename)
                        free (log_filename);
                }
            }
        }
        IRC_BUFFER_ALL_SERVERS(ptr_buffer) = 0;
        gui_status_draw (window->buffer, 1);
        gui_input_draw (window->buffer, 1);
    }
}
*/

/*
 * irc_buffer_close_cb: callback called when a buffer is closed
 */

int
irc_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    
    /* make C compiler happy */
    (void) data;
    
    if (ptr_channel)
    {
        /* send PART for channel if its buffer is closed */
        if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
        {
            irc_command_part_channel (ptr_server, ptr_channel->name, NULL);
            irc_channel_free (ptr_server, ptr_channel);
        }
    }
    else
    {
        if (ptr_server)
        {
            /* send PART on all channels for server, then disconnect from server */
            for (ptr_channel = ptr_server->channels; ptr_channel;
                 ptr_channel = ptr_channel->next_channel)
            {
                irc_command_part_channel (ptr_server, ptr_channel->name, NULL);
                irc_channel_free (ptr_server, ptr_channel);
            }
            irc_server_disconnect (ptr_server, 0);
            ptr_server->buffer = NULL;
        }
    }
    
    return WEECHAT_RC_OK;
}
