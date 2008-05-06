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

/* xfer-chat.c: chat with direct connection to remote host */


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "../weechat-plugin.h"
#include "xfer.h"
#include "xfer-chat.h"
#include "xfer-buffer.h"


/*
 * xfer_chat_send: send data to remote host via xfer chat
 */

int
xfer_chat_send (struct t_xfer *xfer, char *buffer, int size_buf)
{
    if (!xfer)
        return -1;
    
    return send (xfer->sock, buffer, size_buf, 0);
}

/*
 * xfer_chat_sendf: send formatted data to remote host via DCC CHAT
 */

void
xfer_chat_sendf (struct t_xfer *xfer, char *format, ...)
{
    va_list args;
    static char buffer[4096];
    int size_buf;
    
    if (!xfer || (xfer->sock < 0))
        return;
    
    va_start (args, format);
    size_buf = vsnprintf (buffer, sizeof (buffer) - 1, format, args);
    va_end (args);
    
    if (size_buf == 0)
        return;
    
    buffer[sizeof (buffer) - 1] = '\0';
    if ((size_buf < 0) || (size_buf > (int) (sizeof (buffer) - 1)))
        size_buf = strlen (buffer);
    
    if (xfer_chat_send (xfer, buffer, strlen (buffer)) <= 0)
    {
        weechat_printf (NULL,
                        _("%s%s: error sending data to \"%s\" via xfer chat"),
                        weechat_prefix ("error"), "xfer", xfer->remote_nick);
        xfer_close (xfer, XFER_STATUS_FAILED);
    }
}

/*
 * xfer_chat_recv_cb: receive data from xfer chat remote host
 */

int
xfer_chat_recv_cb (void *arg_xfer)
{
    struct t_xfer *xfer;
    static char buffer[4096 + 2];
    char *buf2, *pos, *ptr_buf, *next_ptr_buf;
    int num_read;
    
    xfer = (struct t_xfer *)arg_xfer;
    
    num_read = recv (xfer->sock, buffer, sizeof (buffer) - 2, 0);
    if (num_read > 0)
    {
        buffer[num_read] = '\0';
        
        buf2 = NULL;
        ptr_buf = buffer;
        if (xfer->unterminated_message)
        {
            buf2 = malloc (strlen (xfer->unterminated_message) +
                                   strlen (buffer) + 1);
            if (buf2)
            {
                strcpy (buf2, xfer->unterminated_message);
                strcat (buf2, buffer);
            }
            ptr_buf = buf2;
            free (xfer->unterminated_message);
            xfer->unterminated_message = NULL;
        }
        
        while (ptr_buf && ptr_buf[0])
        {
            next_ptr_buf = NULL;
            pos = strstr (ptr_buf, "\n");
            if (pos)
            {
                pos[0] = '\0';
                next_ptr_buf = pos + 1;
            }
            else
            {
                xfer->unterminated_message = strdup (ptr_buf);
                ptr_buf = NULL;
                next_ptr_buf = NULL;
            }
            
            if (ptr_buf)
            {
                weechat_printf (xfer->buffer, "%s\t%s", xfer->remote_nick, ptr_buf);
            }
            
            ptr_buf = next_ptr_buf;
        }
        
        if (buf2)
            free (buf2);
    }
    else
    {
        xfer_close (xfer, XFER_STATUS_ABORTED);
        xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * xfer_chat_buffer_input_cb: callback called when user send data to xfer chat
 *                            buffer
 */

int
xfer_chat_buffer_input_cb (void *data, struct t_gui_buffer *buffer,
                           char *input_data)
{
    struct t_xfer *xfer;
    
    xfer = (struct t_xfer *)data;
    
    if (!XFER_HAS_ENDED(xfer->status))
    {
        xfer_chat_sendf (xfer, "%s\n", input_data);
        if (!XFER_HAS_ENDED(xfer->status))
        {
            weechat_printf (buffer,
                            "%s\t%s",
                            xfer->local_nick,
                            input_data);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * xfer_chat_close_buffer_cb: callback called when a buffer with direct chat
 *                            is closed
 */

int
xfer_chat_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    struct t_xfer *xfer;
    
    /* make C compiler happy */
    (void) buffer;
    
    xfer = (struct t_xfer *)data;
    
    if (!XFER_HAS_ENDED(xfer->status))
    {
        xfer_close (xfer, XFER_STATUS_ABORTED);
        xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
    }
    
    xfer->buffer = NULL;
    
    return WEECHAT_RC_OK;
}

/*
 * xfer_chat_open_buffer: create channel for DCC chat
 */

void
xfer_chat_open_buffer (struct t_xfer *xfer)
{
    char *name;
    int length;

    length = strlen (xfer->plugin_name) + 1 + strlen (xfer->remote_nick) + 1;
    name = malloc (length);
    if (name)
    {
        snprintf (name, length, "%s_%s", xfer->plugin_name, xfer->remote_nick);
        xfer->buffer = weechat_buffer_new ("xfer", name,
                                           &xfer_chat_buffer_input_cb, xfer,
                                           &xfer_chat_buffer_close_cb, xfer);
        if (xfer->buffer)
        {
            weechat_buffer_set (xfer->buffer, "title", _("xfer chat"));
            weechat_printf (xfer->buffer,
                            _("Connected to %s (%d.%d.%d.%d) via "
                              "xfer chat"),
                            xfer->remote_nick,
                            xfer->address >> 24,
                            (xfer->address >> 16) & 0xff,
                            (xfer->address >> 8) & 0xff,
                            xfer->address & 0xff);
        }
        free (name);
    }
}
