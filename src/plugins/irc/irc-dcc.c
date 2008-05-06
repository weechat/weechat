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
