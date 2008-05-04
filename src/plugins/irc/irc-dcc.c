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

/* irc-dcc.c: Direct Client-to-Client (DCC) communication (files & chat) */


#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-dcc.h"
#include "irc-config.h"


struct t_irc_dcc *irc_dcc_list = NULL; /* DCC files & chat list             */
struct t_irc_dcc *irc_last_dcc = NULL; /* last DCC in list                  */


/*
 * irc_dcc_channel_for_chat: create channel for DCC chat
 */
/*
void
irc_dcc_channel_for_chat (struct t_irc_dcc *dcc)
{
    if (!irc_channel_create_dcc (dcc))
    {
        gui_chat_printf_error (dcc->server->buffer,
                               _("%s can't associate DCC chat with private "
                                 "buffer (maybe private buffer has already "
                                 "DCC CHAT?)\n"),
                               WEECHAT_ERROR);
        irc_dcc_close (dcc, IRC_DCC_FAILED);
        irc_dcc_redraw (WEECHAT_HOTLIST_MESSAGE);
        return;
    }
    
    gui_chat_printf_type (dcc->channel->buffer, GUI_MSG_TYPE_MSG,
                          cfg_look_prefix_info, cfg_col_chat_prefix_info,
                          _("Connected to %s%s %s(%s%d.%d.%d.%d%s)%s via DCC "
                            "chat\n"),
                          GUI_COLOR(GUI_COLOR_CHAT_NICK),
                          dcc->nick,
                          GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                          GUI_COLOR(GUI_COLOR_CHAT_HOST),
                          dcc->addr >> 24,
                          (dcc->addr >> 16) & 0xff,
                          (dcc->addr >> 8) & 0xff,
                          dcc->addr & 0xff,
                          GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                          GUI_COLOR(GUI_COLOR_CHAT));
}
*/
/*
 * irc_dcc_chat_remove_channel: remove a buffer for DCC chat
 */
 /*
void
irc_dcc_chat_remove_channel (struct t_irc_channel *channel)
{
    struct t_irc_dcc *ptr_dcc;
    
    if (!channel)
        return;

    for (ptr_dcc = irc_dcc_list; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
    {
        if (ptr_dcc->channel == channel)
            ptr_dcc->channel = NULL;
    }
}
*/
/*
 * irc_dcc_accept_resume: accepts a resume and inform the receiver
 */
/*
void
irc_dcc_accept_resume (struct t_irc_server *server, char *filename, int port,
                       unsigned long pos_start)
{
    struct t_irc_dcc *ptr_dcc;
    
    ptr_dcc = irc_dcc_search (server, IRC_DCC_FILE_SEND, IRC_DCC_CONNECTING,
                              port);
    if (ptr_dcc)
    {
        ptr_dcc->pos = pos_start;
        ptr_dcc->ack = pos_start;
        ptr_dcc->start_resume = pos_start;
        ptr_dcc->last_check_pos = pos_start;
        irc_server_sendf (ptr_dcc->server,
                          (strchr (ptr_dcc->filename, ' ')) ?
                          "PRIVMSG %s :\01DCC ACCEPT \"%s\" %d %u\01\n" :
                          "PRIVMSG %s :\01DCC ACCEPT %s %d %u\01",
                          ptr_dcc->nick, ptr_dcc->filename,
                          ptr_dcc->port, ptr_dcc->start_resume);
        
        gui_chat_printf_info (ptr_dcc->server->buffer,
                              _("DCC: file %s%s%s resumed at position %u\n"),
                              GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                              ptr_dcc->filename,
                              GUI_COLOR(GUI_COLOR_CHAT),
                              ptr_dcc->start_resume);
        irc_dcc_redraw (WEECHAT_HOTLIST_MESSAGE);
    }
    else
        gui_chat_printf (server->buffer,
                         _("%s can't resume file \"%s\" (port: %d, start "
                           "position: %u): DCC not found or ended\n"),
                         WEECHAT_ERROR, filename, port, pos_start);
}
*/
/*
 * irc_dcc_start_resume: called when "DCC ACCEPT" is received
 *                       (resume accepted by sender)
 */
/*
void
irc_dcc_start_resume (struct t_irc_server *server, char *filename, int port,
                      unsigned long pos_start)
{
    struct t_irc_dcc *ptr_dcc;
    
    ptr_dcc = irc_dcc_search (server, IRC_DCC_FILE_RECV, IRC_DCC_CONNECTING,
                              port);
    if (ptr_dcc)
    {
        ptr_dcc->pos = pos_start;
        ptr_dcc->ack = pos_start;
        ptr_dcc->start_resume = pos_start;
        ptr_dcc->last_check_pos = pos_start;
        irc_dcc_recv_connect_init (ptr_dcc);
    }
    else
        gui_chat_printf (server->buffer,
                         _("%s can't resume file \"%s\" (port: %d, start "
                           "position: %u): DCC not found or ended\n"),
                         WEECHAT_ERROR, filename, port, pos_start);
}
*/
/*
 * irc_dcc_handle: receive/send data for all active DCC
 */
/*
void
irc_dcc_handle ()
{
    struct t_irc_dcc *dcc;
    fd_set read_fd;
    static struct timeval timeout;
    int sock;
    struct sockaddr_in addr;
    socklen_t length;
    
    for (dcc = irc_dcc_list; dcc; dcc = dcc->next_dcc)
    {
        // check DCC timeout
        if (IRC_DCC_IS_FILE(dcc->type) && !IRC_DCC_ENDED(dcc->status))
        {
            if ((irc_cfg_dcc_timeout != 0)
                && (time (NULL) > dcc->last_activity + irc_cfg_dcc_timeout))
            {
                gui_chat_printf_error (dcc->server->buffer,
                                       _("%s DCC: timeout\n"),
                                       WEECHAT_ERROR);
                irc_dcc_close (dcc, IRC_DCC_FAILED);
                irc_dcc_redraw (WEECHAT_HOTLIST_MESSAGE);
                continue;
            }
        }
        
        if (dcc->status == IRC_DCC_CONNECTING)
        {
            if (dcc->type == IRC_DCC_FILE_SEND)
            {
                FD_ZERO (&read_fd);
                FD_SET (dcc->sock, &read_fd);
                timeout.tv_sec = 0;
                timeout.tv_usec = 0;
                
                // something to read on socket?
                if (select (FD_SETSIZE, &read_fd, NULL, NULL, &timeout) > 0)
                {
                    if (FD_ISSET (dcc->sock, &read_fd))
                    {
                        dcc->last_activity = time (NULL);
                        length = sizeof (addr);
                        sock = accept (dcc->sock,
                                       (struct sockaddr *) &addr, &length);
                        close (dcc->sock);
                        dcc->sock = -1;
                        if (sock < 0)
                        {
                            gui_chat_printf_error (dcc->server->buffer,
                                                   _("%s DCC: unable to "
                                                     "create socket for "
                                                     "sending file\n"),
                                                   WEECHAT_ERROR);
                            irc_dcc_close (dcc, IRC_DCC_FAILED);
                            irc_dcc_redraw (WEECHAT_HOTLIST_MESSAGE);
                            continue;
                        }
                        dcc->sock = sock;
                        if (fcntl (dcc->sock, F_SETFL, O_NONBLOCK) == -1)
                        {
                            gui_chat_printf_error (dcc->server->buffer,
                                                   _("%s DCC: unable to set "
                                                     "'nonblock' option for "
                                                     "socket\n"),
                                                   WEECHAT_ERROR);
                            irc_dcc_close (dcc, IRC_DCC_FAILED);
                            irc_dcc_redraw (WEECHAT_HOTLIST_MESSAGE);
                            continue;
                        }
                        dcc->addr = ntohl (addr.sin_addr.s_addr);
                        dcc->status = IRC_DCC_ACTIVE;
                        dcc->start_transfer = time (NULL);
                        irc_dcc_redraw (WEECHAT_HOTLIST_MESSAGE);
                        irc_dcc_file_send_fork (dcc);
                    }
                }
            }
            if (dcc->type == IRC_DCC_FILE_RECV)
            {
                if (dcc->child_read != -1)
                    irc_dcc_file_child_read (dcc);
            }
        }
        
        if (dcc->status == IRC_DCC_WAITING)
        {
            if (dcc->type == IRC_DCC_CHAT_SEND)
            {
                FD_ZERO (&read_fd);
                FD_SET (dcc->sock, &read_fd);
                timeout.tv_sec = 0;
                timeout.tv_usec = 0;
                
                // something to read on socket?
                if (select (FD_SETSIZE, &read_fd, NULL, NULL, &timeout) > 0)
                {
                    if (FD_ISSET (dcc->sock, &read_fd))
                    {
                        length = sizeof (addr);
                        sock = accept (dcc->sock, (struct sockaddr *) &addr, &length);
                        close (dcc->sock);
                        dcc->sock = -1;
                        if (sock < 0)
                        {
                            irc_dcc_close (dcc, IRC_DCC_FAILED);
                            irc_dcc_redraw (WEECHAT_HOTLIST_MESSAGE);
                            continue;
                        }
                        dcc->sock = sock;
                        if (fcntl (dcc->sock, F_SETFL, O_NONBLOCK) == -1)
                        {
                            irc_dcc_close (dcc, IRC_DCC_FAILED);
                            irc_dcc_redraw (WEECHAT_HOTLIST_MESSAGE);
                            continue;
                        }
                        dcc->addr = ntohl (addr.sin_addr.s_addr);
                        dcc->status = IRC_DCC_ACTIVE;
                        irc_dcc_redraw (WEECHAT_HOTLIST_MESSAGE);
                        irc_dcc_channel_for_chat (dcc);
                    }
                }
            }
        }
        
        if (dcc->status == IRC_DCC_ACTIVE)
        {
            if (IRC_DCC_IS_CHAT(dcc->type))
                irc_dcc_chat_recv (dcc);
            else
                irc_dcc_file_child_read (dcc);
        }
    }
}
*/
