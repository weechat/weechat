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

/* irc-dcc.c: DCC communications (files & chat) */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "../common/weechat.h"
#include "irc.h"
#include "../common/weeconfig.h"
#include "../common/hotlist.h"
#include "../gui/gui.h"


t_irc_dcc *dcc_list = NULL;     /* DCC files & chat list                    */
char *dcc_status_string[] =     /* strings for DCC status                   */
{ N_("Waiting"), N_("Connecting"), N_("Active"), N_("Done"), N_("Failed"),
  N_("Aborted") };


/*
 * dcc_redraw: redraw DCC buffer (and add to hotlist)
 */

void
dcc_redraw (int highlight)
{
    gui_redraw_buffer (gui_get_dcc_buffer (gui_current_window));
    if (highlight)
    {
        hotlist_add (highlight, gui_get_dcc_buffer (gui_current_window));
        gui_draw_buffer_status (gui_current_window->buffer, 0);
    }
}

/*
 * dcc_search: search a DCC
 */

t_irc_dcc *
dcc_search (t_irc_server *server, int type, int status, int port)
{
    t_irc_dcc *ptr_dcc;
    
    for (ptr_dcc = dcc_list; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
    {
        if ((ptr_dcc->server == server)
            && (ptr_dcc->type == type)
            && (ptr_dcc->status = status)
            && (ptr_dcc->port == port))
            return ptr_dcc;
    }
    
    /* DCC not found */
    return NULL;
}

/*
 * dcc_port_in_use: return 1 if a port is in used (by an active or connecting DCC)
 */

int
dcc_port_in_use (int port)
{
    t_irc_dcc *ptr_dcc;
    
    /* skip any currently used ports */
    for (ptr_dcc = dcc_list; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
    {
        if ((ptr_dcc->port == port) && (!DCC_ENDED(ptr_dcc->status)))
            return 1;
    }
    
    /* port not in use */
    return 0;
}

/*
 * dcc_file_is_resumable: check if a file can be used for resuming a download
 */

int
dcc_file_is_resumable (t_irc_dcc *ptr_dcc, char *filename)
{
    struct stat st;
    
    if (!cfg_dcc_auto_resume)
        return 0;
    
    if (access (filename, W_OK) == 0)
    {
        if (stat (filename, &st) != -1)
        {
            if ((unsigned long) st.st_size < ptr_dcc->size)
            {
                ptr_dcc->start_resume = (unsigned long) st.st_size;
                ptr_dcc->pos = st.st_size;
                ptr_dcc->last_check_pos = st.st_size;
                return 1;
            }
        }
    }
    
    /* not resumable */
    return 0;
}

/*
 * dcc_find_filename: find local filename for a DCC
 *                    if type if file/recv, add a suffix (like .1) if needed
 *                    if download is resumable, set "start_resume" to good value
 */

void
dcc_find_filename (t_irc_dcc *ptr_dcc)
{
    char *ptr_home, *filename2;
    
    ptr_home = getenv ("HOME");
    ptr_dcc->local_filename = (char *) malloc (strlen (cfg_dcc_download_path) +
                                               strlen (ptr_dcc->nick) +
                                               strlen (ptr_dcc->filename) +
                                               ((cfg_dcc_download_path[0] == '~') ?
                                                strlen (ptr_home) : 0) +
                                               4);
    if (!ptr_dcc->local_filename)
        return;
    
    if (cfg_dcc_download_path[0] == '~')
    {
        strcpy (ptr_dcc->local_filename, ptr_home);
        strcat (ptr_dcc->local_filename, cfg_dcc_download_path + 1);
    }
    else
        strcpy (ptr_dcc->local_filename, cfg_dcc_download_path);
    if (ptr_dcc->local_filename[strlen (ptr_dcc->local_filename) - 1] != DIR_SEPARATOR_CHAR)
        strcat (ptr_dcc->local_filename, DIR_SEPARATOR);
    strcat (ptr_dcc->local_filename, ptr_dcc->nick);
    strcat (ptr_dcc->local_filename, ".");
    strcat (ptr_dcc->local_filename, ptr_dcc->filename);
    
    /* file already exists? */
    if (access (ptr_dcc->local_filename, F_OK) == 0)
    {
        if (dcc_file_is_resumable (ptr_dcc, ptr_dcc->local_filename))
            return;
        
        /* if auto rename is not set, then abort DCC */
        if (!cfg_dcc_auto_rename)
        {
            dcc_close (ptr_dcc, DCC_FAILED);
            dcc_redraw (HOTLIST_MSG);
            return;
        }
        
        filename2 = (char *) malloc (strlen (ptr_dcc->local_filename) + 16);
        if (!filename2)
        {
            dcc_close (ptr_dcc, DCC_FAILED);
            dcc_redraw (HOTLIST_MSG);
            return;
        }
        ptr_dcc->filename_suffix = 0;
        do
        {
            ptr_dcc->filename_suffix++;
            sprintf (filename2, "%s.%d",
                     ptr_dcc->local_filename,
                     ptr_dcc->filename_suffix);
            if (access (filename2, F_OK) == 0)
            {
                if (dcc_file_is_resumable (ptr_dcc, filename2))
                    break;
            }
            else
                break;
        }
        while (1);
        
        free (ptr_dcc->local_filename);
        ptr_dcc->local_filename = strdup (filename2);
        free (filename2);
    }
}

/*
 * dcc_calculate_speed: calculate DCC speed (for files only)
 */

void
dcc_calculate_speed (t_irc_dcc *ptr_dcc, int ended)
{
    time_t local_time, elapsed;
    
    local_time = time (NULL);
    if (ended || local_time > ptr_dcc->last_check_time)
    {
        
        if (ended)
        {
            elapsed = local_time - ptr_dcc->start_transfer;
            if (elapsed == 0)
                elapsed = 1;
            ptr_dcc->bytes_per_sec = (ptr_dcc->pos - ptr_dcc->start_resume) / elapsed;
        }
        else
        {
            elapsed = local_time - ptr_dcc->last_check_time;
            if (elapsed == 0)
                elapsed = 1;
            ptr_dcc->bytes_per_sec = (ptr_dcc->pos - ptr_dcc->last_check_pos) / elapsed;
        }
        ptr_dcc->last_check_time = local_time;
        ptr_dcc->last_check_pos = ptr_dcc->pos;
    }
}

/*
 * dcc_connect: connect to another host
 */

int
dcc_connect (t_irc_dcc *ptr_dcc)
{
    struct sockaddr_in addr;
    struct hostent *hostent;
    char *ip4;
    
    if (ptr_dcc->type == DCC_CHAT_SEND)
        ptr_dcc->status = DCC_WAITING;
    else
        ptr_dcc->status = DCC_CONNECTING;
    
    if (ptr_dcc->sock == -1)
    {
        ptr_dcc->sock = socket (AF_INET, SOCK_STREAM, 0);
        if (ptr_dcc->sock == -1)
            return 0;
    }

    /* for sending (chat or file), listen to socket for a connection */
    if (DCC_IS_SEND(ptr_dcc->type))
      {
        if (fcntl (ptr_dcc->sock, F_SETFL, O_NONBLOCK) == -1)
	  return 0;	
        if (listen (ptr_dcc->sock, 1) == -1)
            return 0;
        if (fcntl (ptr_dcc->sock, F_SETFL, 0) == -1)
            return 0;
    }
    
    /* for receiving (chat or file), connect to listening host */
    if (DCC_IS_RECV(ptr_dcc->type))
    {
        if (fcntl (ptr_dcc->sock, F_SETFL, O_NONBLOCK) == -1)
            return 0;      
        if (cfg_proxy_use)
	{
            memset (&addr, 0, sizeof (addr));
            addr.sin_addr.s_addr = htonl (ptr_dcc->addr);
            ip4 = inet_ntoa(addr.sin_addr);

            memset (&addr, 0, sizeof (addr));
            addr.sin_port = htons (cfg_proxy_port);
            addr.sin_family = AF_INET;
            if ((hostent = gethostbyname (cfg_proxy_address)) == NULL)
                return 0;
            memcpy(&(addr.sin_addr),*(hostent->h_addr_list), sizeof(struct in_addr));
            connect (ptr_dcc->sock, (struct sockaddr *) &addr, sizeof (addr));
            if (pass_proxy(ptr_dcc->sock, ip4, ptr_dcc->port, ptr_dcc->server->username) == -1)
                return 0;
	}
        else
	{
            memset (&addr, 0, sizeof (addr));
            addr.sin_port = htons (ptr_dcc->port);
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl (ptr_dcc->addr);
            connect (ptr_dcc->sock, (struct sockaddr *) &addr, sizeof (addr));
	}
    }
    
    return 1;
}

/*
 * dcc_free: free DCC struct and remove it from list
 */

void
dcc_free (t_irc_dcc *ptr_dcc)
{
    t_irc_dcc *new_dcc_list;
    
    if (ptr_dcc->prev_dcc)
    {
        (ptr_dcc->prev_dcc)->next_dcc = ptr_dcc->next_dcc;
        new_dcc_list = dcc_list;
    }
    else
        new_dcc_list = ptr_dcc->next_dcc;
    
    if (ptr_dcc->next_dcc)
        (ptr_dcc->next_dcc)->prev_dcc = ptr_dcc->prev_dcc;
    
    if (ptr_dcc->nick)
        free (ptr_dcc->nick);
    if (ptr_dcc->unterminated_message)
        free (ptr_dcc->unterminated_message);
    if (ptr_dcc->filename)
        free (ptr_dcc->filename);
    
    free (ptr_dcc);
    dcc_list = new_dcc_list;
}

/*
 * dcc_close: close a DCC connection
 */

void
dcc_close (t_irc_dcc *ptr_dcc, int status)
{
    t_gui_buffer *ptr_buffer;
    struct stat st;
    
    ptr_dcc->status = status;
    
    if ((status == DCC_DONE) || (status == DCC_ABORTED) || (status == DCC_FAILED))
    {
        if (DCC_IS_FILE(ptr_dcc->type))
        {
            irc_display_prefix (ptr_dcc->server, ptr_dcc->server->buffer,
                                PREFIX_INFO);
            gui_printf (ptr_dcc->server->buffer, _("DCC: file "));
            gui_printf_color (ptr_dcc->server->buffer,
                              COLOR_WIN_CHAT_CHANNEL,
                              "%s",
                              ptr_dcc->filename);
            if (ptr_dcc->local_filename)
            {
                gui_printf (ptr_dcc->server->buffer, _(" (local filename: "));
                gui_printf_color (ptr_dcc->server->buffer,
                                  COLOR_WIN_CHAT_CHANNEL,
                                  "%s",
                                  ptr_dcc->local_filename);
                gui_printf (ptr_dcc->server->buffer, ")");
            }
            if (ptr_dcc->type == DCC_FILE_SEND)
                gui_printf (ptr_dcc->server->buffer, _(" sent to "));
            else
                gui_printf (ptr_dcc->server->buffer, _(" received from "));
            gui_printf_color (ptr_dcc->server->buffer,
                              COLOR_WIN_CHAT_NICK,
                              "%s",
                              ptr_dcc->nick);
            gui_printf (ptr_dcc->server->buffer,
                        (status == DCC_DONE) ? _(": ok!\n") : _(": FAILED\n"));
        }
    }
    if (status == DCC_ABORTED)
    {
        if (DCC_IS_CHAT(ptr_dcc->type))
        {
            if (ptr_dcc->channel)
                ptr_buffer = ptr_dcc->channel->buffer;
            else
                ptr_buffer = ptr_dcc->server->buffer;
            irc_display_prefix (ptr_dcc->server, ptr_buffer, PREFIX_INFO);
            gui_printf (ptr_buffer, _("DCC chat closed with "));
            gui_printf_color (ptr_buffer, COLOR_WIN_CHAT_NICK,
                              "%s", ptr_dcc->nick);
            gui_printf_color (ptr_buffer, COLOR_WIN_CHAT_DARK, " (");
            gui_printf_color (ptr_buffer, COLOR_WIN_CHAT_HOST,
                              "%d.%d.%d.%d",
                              ptr_dcc->addr >> 24, (ptr_dcc->addr >> 16) & 0xff,
                              (ptr_dcc->addr >> 8) & 0xff, ptr_dcc->addr & 0xff);
            gui_printf_color (ptr_buffer, COLOR_WIN_CHAT_DARK, ")\n");
        }
    }
    
    /* remove empty file if received file failed and nothing was transfered */
    if (((status == DCC_FAILED) || (status == DCC_ABORTED))
        && DCC_IS_FILE(ptr_dcc->type)
        && DCC_IS_RECV(ptr_dcc->type)
        && ptr_dcc->local_filename
        && ptr_dcc->pos == 0)
    {
        /* erase file only if really empty on disk */
        if (stat (ptr_dcc->local_filename, &st) != -1)
        {
            if ((unsigned long) st.st_size == 0)
                unlink (ptr_dcc->local_filename);
        }
    }
    
    if (DCC_IS_CHAT(ptr_dcc->type))
        channel_remove_dcc (ptr_dcc);
    
    if (DCC_IS_FILE(ptr_dcc->type))
        dcc_calculate_speed (ptr_dcc, 1);
    
    if (ptr_dcc->sock != -1)
    {
        close (ptr_dcc->sock);
        ptr_dcc->sock = -1;
    }
    if (ptr_dcc->file != -1)
    {
        close (ptr_dcc->file);
        ptr_dcc->file = -1;
    }
}

/*
 * dcc_channel_for_chat: create channel for DCC chat
 */

void
dcc_channel_for_chat (t_irc_dcc *ptr_dcc)
{
    if (!channel_create_dcc (ptr_dcc))
    {
        irc_display_prefix (ptr_dcc->server, ptr_dcc->server->buffer,
                            PREFIX_ERROR);
        gui_printf (ptr_dcc->server->buffer,
                    _("%s can't associate DCC chat with private buffer "
                    "(maybe private buffer has already DCC CHAT?)\n"),
                    WEECHAT_ERROR);
        dcc_close (ptr_dcc, DCC_FAILED);
        dcc_redraw (HOTLIST_MSG);
        return;
    }
    
    irc_display_prefix (ptr_dcc->server, ptr_dcc->channel->buffer,
                        PREFIX_INFO);
    gui_printf_type (ptr_dcc->channel->buffer, MSG_TYPE_MSG,
                     _("Connected to "));
    gui_printf_color (ptr_dcc->channel->buffer, COLOR_WIN_CHAT_NICK,
                      "%s", ptr_dcc->nick);
    gui_printf_color (ptr_dcc->channel->buffer, COLOR_WIN_CHAT_DARK, " (");
    gui_printf_color (ptr_dcc->channel->buffer, COLOR_WIN_CHAT_HOST,
                      "%d.%d.%d.%d",
                      ptr_dcc->addr >> 24, (ptr_dcc->addr >> 16) & 0xff,
                      (ptr_dcc->addr >> 8) & 0xff, ptr_dcc->addr & 0xff);
    gui_printf_color (ptr_dcc->channel->buffer, COLOR_WIN_CHAT_DARK, ") ");
    gui_printf (ptr_dcc->channel->buffer, _("via DCC chat\n"));
}

/*
 * dcc_recv_connect_init: connect to sender and init file or chat
 */

void
dcc_recv_connect_init (t_irc_dcc *ptr_dcc)
{
    if (!dcc_connect (ptr_dcc))
    {
        dcc_close (ptr_dcc, DCC_FAILED);
        dcc_redraw (HOTLIST_MSG);
    }
    else
    {
        ptr_dcc->status = DCC_ACTIVE;
        
        /* DCC file => look for local filename and open it in writing mode */
        if (DCC_IS_FILE(ptr_dcc->type))
        {
            if (ptr_dcc->start_resume > 0)
                ptr_dcc->file = open (ptr_dcc->local_filename,
                                      O_APPEND | O_WRONLY | O_NONBLOCK);
            else
                ptr_dcc->file = open (ptr_dcc->local_filename,
                                      O_CREAT | O_TRUNC | O_WRONLY | O_NONBLOCK,
                                      0644);
            ptr_dcc->start_transfer = time (NULL);
            ptr_dcc->last_check_time = time (NULL);
        }
        else
        {
            /* DCC CHAT => associate DCC with channel */
            dcc_channel_for_chat (ptr_dcc);
        }
    }
    dcc_redraw (HOTLIST_MSG);
}

/*
 * dcc_accept: accepts a DCC file or chat request
 */

void
dcc_accept (t_irc_dcc *ptr_dcc)
{
    if (DCC_IS_FILE(ptr_dcc->type) && (ptr_dcc->start_resume > 0))
    {
        ptr_dcc->status = DCC_CONNECTING;
        server_sendf (ptr_dcc->server,
                      (strchr (ptr_dcc->filename, ' ')) ?
                          "PRIVMSG %s :\01DCC RESUME \"%s\" %d %u\01\r\n" :
                          "PRIVMSG %s :\01DCC RESUME %s %d %u\01\r\n",
                      ptr_dcc->nick, ptr_dcc->filename,
                      ptr_dcc->port, ptr_dcc->start_resume);
        dcc_redraw (HOTLIST_MSG);
    }
    else
        dcc_recv_connect_init (ptr_dcc);
}

/*
 * dcc_accept_resume: accepts a resume and inform the receiver
 */

void
dcc_accept_resume (t_irc_server *server, char *filename, int port,
                   unsigned long pos_start)
{
    t_irc_dcc *ptr_dcc;
    
    ptr_dcc = dcc_search (server, DCC_FILE_SEND, DCC_CONNECTING, port);
    if (ptr_dcc)
    {
        ptr_dcc->pos = pos_start;
        ptr_dcc->ack = pos_start;
        ptr_dcc->start_resume = pos_start;
        ptr_dcc->last_check_pos = pos_start;
        server_sendf (ptr_dcc->server,
                      (strchr (ptr_dcc->filename, ' ')) ?
                          "PRIVMSG %s :\01DCC ACCEPT \"%s\" %d %u\01\r\n" :
                          "PRIVMSG %s :\01DCC ACCEPT %s %d %u\01\r\n",
                      ptr_dcc->nick, ptr_dcc->filename,
                      ptr_dcc->port, ptr_dcc->start_resume);
        
        irc_display_prefix (ptr_dcc->server, ptr_dcc->server->buffer,
                            PREFIX_INFO);
        gui_printf (ptr_dcc->server->buffer, _("DCC: file "));
        gui_printf_color (ptr_dcc->server->buffer,
                          COLOR_WIN_CHAT_CHANNEL,
                          "%s ",
                          ptr_dcc->filename);
        gui_printf (ptr_dcc->server->buffer, _("resumed at position %u\n"),
                    ptr_dcc->start_resume);
        dcc_redraw (HOTLIST_MSG);
    }
    else
        gui_printf (server->buffer,
                    _("%s can't resume file \"%s\" (port: %d, start position: %u): DCC not found or ended\n"),
                    WEECHAT_ERROR, filename, port, pos_start);
}

/*
 * dcc_start_resume: called when "DCC ACCEPT" is received (resume accepted by sender)
 */

void
dcc_start_resume (t_irc_server *server, char *filename, int port,
                  unsigned long pos_start)
{
    t_irc_dcc *ptr_dcc;
    
    ptr_dcc = dcc_search (server, DCC_FILE_RECV, DCC_CONNECTING, port);
    if (ptr_dcc)
    {
        ptr_dcc->pos = pos_start;
        ptr_dcc->ack = pos_start;
        ptr_dcc->start_resume = pos_start;
        ptr_dcc->last_check_pos = pos_start;
        dcc_recv_connect_init (ptr_dcc);
    }
    else
        gui_printf (server->buffer,
                    _("%s can't resume file \"%s\" (port: %d, start position: %u): DCC not found or ended\n"),
                    WEECHAT_ERROR, filename, port, pos_start);
}

/*
 * dcc_add: add a DCC file to queue
 */

t_irc_dcc *
dcc_add (t_irc_server *server, int type, unsigned long addr, int port, char *nick,
         int sock, char *filename, char *local_filename, unsigned long size)
{
    t_irc_dcc *new_dcc;
    
    /* create new DCC struct */
    if ((new_dcc = (t_irc_dcc *) malloc (sizeof (t_irc_dcc))) == NULL)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf (server->buffer,
                    _("%s not enough memory for new DCC\n"),
                    WEECHAT_ERROR);
        return NULL;
    }
    
    /* initialize new DCC */
    new_dcc->server = server;
    new_dcc->channel = NULL;
    new_dcc->type = type;
    new_dcc->status = DCC_WAITING;
    new_dcc->start_time = time (NULL);
    new_dcc->start_transfer = time (NULL);
    new_dcc->addr = addr;
    new_dcc->port = port;
    new_dcc->nick = strdup (nick);
    new_dcc->sock = sock;
    new_dcc->unterminated_message = NULL;
    new_dcc->file = -1;
    if (DCC_IS_CHAT(type))
        new_dcc->filename = strdup (_("DCC chat"));
    else
        new_dcc->filename = (filename) ? strdup (filename) : NULL;
    new_dcc->local_filename = NULL;
    new_dcc->filename_suffix = -1;
    new_dcc->size = size;
    new_dcc->pos = 0;
    new_dcc->ack = 0;
    new_dcc->start_resume = 0;
    new_dcc->last_check_time = time (NULL);
    new_dcc->last_check_pos = 0;
    new_dcc->bytes_per_sec = 0;
    new_dcc->last_activity = time (NULL);
    if (local_filename)
        new_dcc->local_filename = strdup (local_filename);
    else
        dcc_find_filename (new_dcc);
    new_dcc->prev_dcc = NULL;
    new_dcc->next_dcc = dcc_list;
    if (dcc_list)
        dcc_list->prev_dcc = new_dcc;
    dcc_list = new_dcc;
    
    gui_current_window->dcc_first = NULL;
    gui_current_window->dcc_selected = NULL;
    
    /* write info message on server buffer */
    if (type == DCC_FILE_RECV)
    {
        irc_display_prefix (server, server->buffer, PREFIX_INFO);
        gui_printf (server->buffer, _("Incoming DCC file from "));
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_NICK, "%s", nick);
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_DARK, " (");
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_HOST,
                          "%d.%d.%d.%d",
                          addr >> 24, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff);
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_DARK, ")");
        gui_printf (server->buffer, ": ");
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_CHANNEL, "%s", filename);
        gui_printf (server->buffer, ", ");
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_CHANNEL, "%lu", size);
        gui_printf (server->buffer, _(" bytes\n"));
        dcc_redraw (HOTLIST_MSG);
    }
    if (type == DCC_FILE_SEND)
    {
        irc_display_prefix (server, server->buffer, PREFIX_INFO);
        gui_printf (server->buffer, _("Sending DCC file to "));
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_NICK, "%s", nick);
        gui_printf (server->buffer, ": ");
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_CHANNEL, "%s", filename);
        gui_printf (server->buffer, _(" (local filename: "));
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_CHANNEL, "%s", local_filename);
        gui_printf (server->buffer, "), ");
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_CHANNEL, "%lu", size);
        gui_printf (server->buffer, _(" bytes\n"));
        dcc_redraw (HOTLIST_MSG);
    }
    if (type == DCC_CHAT_RECV)
    {
        irc_display_prefix (server, server->buffer, PREFIX_INFO);
        gui_printf (server->buffer, _("Incoming DCC chat request from "));
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_NICK, "%s", nick);
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_DARK, " (");
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_HOST,
                          "%d.%d.%d.%d",
                          addr >> 24, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff);
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_DARK, ")\n");
        dcc_redraw (HOTLIST_MSG);
    }
    if (type == DCC_CHAT_SEND)
    {
        irc_display_prefix (server, server->buffer, PREFIX_INFO);
        gui_printf (server->buffer, _("Sending DCC chat request to "));
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_NICK, "%s\n", nick);
        dcc_redraw (HOTLIST_MSG);
    }
    
    if (DCC_IS_FILE(type) && (!new_dcc->local_filename))
    {
        dcc_close (new_dcc, DCC_FAILED);
        dcc_redraw (HOTLIST_MSG);
        return NULL;
    }
    
    if (DCC_IS_FILE(type) && (new_dcc->start_resume > 0))
    {
        irc_display_prefix (new_dcc->server, new_dcc->server->buffer,
                            PREFIX_INFO);
        gui_printf (new_dcc->server->buffer, _("DCC: file "));
        gui_printf_color (new_dcc->server->buffer,
                          COLOR_WIN_CHAT_CHANNEL,
                          "%s",
                          new_dcc->filename);
        gui_printf (new_dcc->server->buffer, _(" (local filename: "));
        gui_printf_color (new_dcc->server->buffer,
                          COLOR_WIN_CHAT_CHANNEL,
                          "%s",
                          new_dcc->local_filename);
        gui_printf (new_dcc->server->buffer, ") ");
        gui_printf (new_dcc->server->buffer, _("will be resumed at position %u\n"),
                    new_dcc->start_resume);
        dcc_redraw (HOTLIST_MSG);
    }
    
    /* connect if needed and redraw DCC buffer */
    if (DCC_IS_SEND(type))
    {
        if (!dcc_connect (new_dcc))
        {
            dcc_close (new_dcc, DCC_FAILED);
            dcc_redraw (HOTLIST_MSG);
            return NULL;
        }
    }
    
    if ( ( (type == DCC_CHAT_RECV) && (cfg_dcc_auto_accept_chats) )
        || ( (type == DCC_FILE_RECV) && (cfg_dcc_auto_accept_files) ) )
        dcc_accept (new_dcc);
    else
        dcc_redraw (HOTLIST_PRIVATE);
    gui_draw_buffer_status (gui_current_window->buffer, 0);
    
    return new_dcc;
}

/*
 * dcc_send_request: send DCC request (file or chat)
 */

void
dcc_send_request (t_irc_server *server, int type, char *nick, char *filename)
{
    char *ptr_home, *filename2, *short_filename, *pos;
    int spaces, args, port_start, port_end;
    struct stat st;
    int sock, port;
    struct hostent *host;
    struct in_addr tmpaddr;
    struct sockaddr_in addr;
    socklen_t length;
    unsigned long local_addr;
    t_irc_dcc *ptr_dcc;
    
    filename2 = NULL;
    short_filename = NULL;
    spaces = 0;
    
    if (type == DCC_FILE_SEND)
    {
        /* add home if filename not beginning with '/' (not for Win32) */
#ifdef _WIN32
        filename2 = strdup (filename);
#else
        if (filename[0] == '/')
            filename2 = strdup (filename);
        else
        {
            ptr_home = getenv ("HOME");
            filename2 = (char *) malloc (strlen (cfg_dcc_upload_path) +
                                         strlen (filename) +
                                         ((cfg_dcc_upload_path[0] == '~') ?
                                             strlen (ptr_home) : 0) +
                                         4);
            if (!filename2)
            {
                irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                gui_printf (server->buffer,
                            _("%s not enough memory for DCC SEND\n"),
                            WEECHAT_ERROR);
                return;
            }
            if (cfg_dcc_upload_path[0] == '~')
            {
                strcpy (filename2, ptr_home);
                strcat (filename2, cfg_dcc_upload_path + 1);
            }
            else
                strcpy (filename2, cfg_dcc_upload_path);
            if (filename2[strlen (filename2) - 1] != DIR_SEPARATOR_CHAR)
                strcat (filename2, DIR_SEPARATOR);
            strcat (filename2, filename);
        }
#endif
        
        /* check if file exists */
        if (stat (filename2, &st) == -1)
        {
            irc_display_prefix (server, server->buffer, PREFIX_ERROR);
            gui_printf (server->buffer,
                        _("%s cannot access file \"%s\"\n"),
                        WEECHAT_ERROR, filename2);
            if (filename2)
                free (filename2);
            return;
        }
    }
    
    /* get local IP address */
    
    /* look up the IP address from dcc_own_ip, if set */
    local_addr = 0;
    if (cfg_dcc_own_ip && cfg_dcc_own_ip[0])
    {
        host = gethostbyname (cfg_dcc_own_ip);
        if (host)
        {
            memcpy (&tmpaddr, host->h_addr_list[0], sizeof(struct in_addr));
            local_addr = ntohl (tmpaddr.s_addr);
        }
        else
            gui_printf (server->buffer,
                        _("%s could not find address for '%s'. Falling back to local IP.\n"),
                        WEECHAT_WARNING, cfg_dcc_own_ip);
    }
    
    /* use the local interface, from the server socket */
    memset (&addr, 0, sizeof (struct sockaddr_in));
    length = sizeof (addr);
    getsockname (server->sock, (struct sockaddr *) &addr, &length);
    addr.sin_family = AF_INET;
    
    /* fallback to the local IP address on the interface, if required */
    if (local_addr == 0)
        local_addr = ntohl (addr.sin_addr.s_addr);
    
    /* open socket for DCC */
    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf (server->buffer,
                    _("%s cannot create socket for DCC\n"),
                    WEECHAT_ERROR);
        if (filename2)
            free (filename2);
        return;
    }
    
    /* look for port */
    
    port = 0;
    
    if (cfg_dcc_port_range && cfg_dcc_port_range[0])
    {
        /* find a free port in the specified range */
        args = sscanf (cfg_dcc_port_range, "%d-%d", &port_start, &port_end);
        if (args > 0)
        {
            port = port_start;
            if (args == 1)
                port_end = port_start;
            
            /* loop through the entire allowed port range */
            while (port <= port_end)
            {
                if (!dcc_port_in_use (port))
                {
                    /* attempt to bind to the free port */
                    addr.sin_port = htons (port);
                    if (bind (sock, (struct sockaddr *) &addr, sizeof (addr)) == 0)
                        break;
                }
                port++;
            }
            
            if (port > port_end)
                port = -1;
        }
    }
    
    if (port == 0)
    {
        /* find port automatically */
        addr.sin_port = 0;
        if (bind (sock, (struct sockaddr *) &addr, sizeof (addr)) == 0)
        {
            length = sizeof (addr);
            getsockname (sock, (struct sockaddr *) &addr, &length);
            port = ntohs (addr.sin_port);
        }
        else
            port = -1;
    }

    if (port == -1)
    {
        /* Could not find any port to bind */
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf (server->buffer,
                    _("%s cannot find available port for DCC\n"),
                    WEECHAT_ERROR);
        close (sock);
        if (filename2)
            free (filename2);
        return;
    }
    
    if (type == DCC_FILE_SEND)
    {
        /* extract short filename (without path) */
        pos = strrchr (filename2, DIR_SEPARATOR_CHAR);
        if (pos)
            short_filename = strdup (pos + 1);
        else
            short_filename = strdup (filename2);
        
        /* convert spaces to underscore if asked and needed */
        pos = short_filename;
        spaces = 0;
        while (pos[0])
        {
            if (pos[0] == ' ')
            {
                if (cfg_dcc_convert_spaces)
                    pos[0] = '_';
                else
                    spaces = 1;
            }
            pos++;
        }
    }
    
    /* add DCC entry and listen to socket */
    if (type == DCC_CHAT_SEND)
        ptr_dcc = dcc_add (server, DCC_CHAT_SEND, local_addr, port, nick, sock,
                           NULL, NULL, 0);
    else
        ptr_dcc = dcc_add (server, DCC_FILE_SEND, local_addr, port, nick, sock,
                           short_filename, filename2, st.st_size);
    if (!ptr_dcc)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf (server->buffer,
                    _("%s cannot send DCC\n"),
                    WEECHAT_ERROR);
        close (sock);
        if (short_filename)
            free (short_filename);
        if (filename2)
            free (filename2);
        return;
    }
    
    /* send DCC request to nick */
    if (type == DCC_CHAT_SEND)
        server_sendf (server, 
                      "PRIVMSG %s :\01DCC CHAT chat %lu %d\01\r\n",
                      nick, local_addr, port);
    else
        server_sendf (server, 
                      (spaces) ?
                          "PRIVMSG %s :\01DCC SEND \"%s\" %lu %d %u\01\r\n" :
                          "PRIVMSG %s :\01DCC SEND %s %lu %d %u\01\r\n",
                      nick, short_filename, local_addr, port,
                      (unsigned long) st.st_size);
    
    if (short_filename)
        free (short_filename);
    if (filename2)
        free (filename2);
}

/*
 * dcc_chat_send: send data to remote host via DCC CHAT
 */

int
dcc_chat_send (t_irc_dcc *ptr_dcc, char *buffer, int size_buf)
{
    if (!ptr_dcc)
        return -1;
    
    return send (ptr_dcc->sock, buffer, size_buf, 0);
}

/*
 * dcc_chat_sendf: send formatted data to remote host via DCC CHAT
 */

void
dcc_chat_sendf (t_irc_dcc *ptr_dcc, char *fmt, ...)
{
    va_list args;
    static char buffer[4096];
    char *buf2;
    int size_buf;

    if (!ptr_dcc || (ptr_dcc->sock == -1))
        return;
    
    va_start (args, fmt);
    size_buf = vsnprintf (buffer, sizeof (buffer) - 1, fmt, args);
    va_end (args);
    
    if ((size_buf == 0) || (strcmp (buffer, "\r\n") == 0))
        return;
    
    buffer[sizeof (buffer) - 1] = '\0';
    if ((size_buf < 0) || (size_buf > (int) (sizeof (buffer) - 1)))
        size_buf = strlen (buffer);
#ifdef DEBUG
    buffer[size_buf - 2] = '\0';
    gui_printf (ptr_dcc->server->buffer, "[DEBUG] Sending to remote host (DCC CHAT) >>> %s\n", buffer);
    buffer[size_buf - 2] = '\r';
#endif
    buf2 = weechat_convert_encoding ((cfg_look_charset_internal && cfg_look_charset_internal[0]) ?
                                     cfg_look_charset_internal : local_charset,
                                     cfg_look_charset_encode,
                                     buffer);
    if (dcc_chat_send (ptr_dcc, buf2, strlen (buf2)) <= 0)
    {
        irc_display_prefix (ptr_dcc->server, ptr_dcc->server->buffer,
                            PREFIX_ERROR);
        gui_printf (ptr_dcc->server->buffer,
                    _("%s error sending data to \"%s\" via DCC CHAT\n"),
                    WEECHAT_ERROR, ptr_dcc->nick);
        dcc_close (ptr_dcc, DCC_FAILED);
    }
    free (buf2);
}

/*
 * dcc_chat_recv: receive data from DCC CHAT host
 */

void
dcc_chat_recv (t_irc_dcc *ptr_dcc)
{
    static char buffer[4096 + 2];
    char *buf2, *pos, *ptr_buf, *next_ptr_buf;
    int num_read;

    num_read = recv (ptr_dcc->sock, buffer, sizeof (buffer) - 2, 0);
    if (num_read > 0)
    {
        buffer[num_read] = '\0';
        
        buf2 = NULL;
        ptr_buf = buffer;
        if (ptr_dcc->unterminated_message)
        {
            buf2 = (char *) malloc (strlen (ptr_dcc->unterminated_message) +
                strlen (buffer) + 1);
            if (buf2)
            {
                strcpy (buf2, ptr_dcc->unterminated_message);
                strcat (buf2, buffer);
            }
            ptr_buf = buf2;
            free (ptr_dcc->unterminated_message);
            ptr_dcc->unterminated_message = NULL;
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
                    ptr_dcc->unterminated_message = strdup (ptr_buf);
                    ptr_buf = NULL;
                    next_ptr_buf = NULL;
                }
            }
            
            if (ptr_buf)
            {
                gui_printf_type_color (ptr_dcc->channel->buffer,
                                       MSG_TYPE_NICK,
                                       COLOR_WIN_CHAT_DARK, "<");
                if (irc_is_highlight (ptr_buf, ptr_dcc->server->nick))
                {
                    gui_printf_type_color (ptr_dcc->channel->buffer,
                                           MSG_TYPE_NICK | MSG_TYPE_HIGHLIGHT,
                                           COLOR_WIN_CHAT_HIGHLIGHT,
                                           "%s", ptr_dcc->nick);
                    if ( (cfg_look_infobar_delay_highlight > 0)
                        && (ptr_dcc->channel->buffer != gui_current_window->buffer) )
                        gui_infobar_printf (cfg_look_infobar_delay_highlight,
                                            COLOR_WIN_INFOBAR_HIGHLIGHT,
                                            _("Private %s> %s"),
                                            ptr_dcc->nick, ptr_buf);
                }
                else
                    gui_printf_type_color (ptr_dcc->channel->buffer,
                                           MSG_TYPE_NICK,
                                           COLOR_WIN_NICK_PRIVATE,
                                           "%s", ptr_dcc->nick);
                gui_printf_type_color (ptr_dcc->channel->buffer,
                                       MSG_TYPE_NICK,
                                       COLOR_WIN_CHAT_DARK, "> ");
                gui_printf_type_color (ptr_dcc->channel->buffer,
                                       MSG_TYPE_MSG,
                                       COLOR_WIN_CHAT, "%s\n", ptr_buf);
            }
            
            ptr_buf = next_ptr_buf;
        }
        
        if (buf2)
            free (buf2);
    }
    else
    {
        dcc_close (ptr_dcc, DCC_ABORTED);
        dcc_redraw (HOTLIST_MSG);
    }
}

/*
 * dcc_handle: receive/send data for all active DCC
 */

void
dcc_handle ()
{
    t_irc_dcc *ptr_dcc;
    int num_read, num_sent;
    static char buffer[102400];
    uint32_t pos;
    fd_set read_fd;
    static struct timeval timeout;
    int sock;
    struct sockaddr_in addr;
    socklen_t length;
    
    for (ptr_dcc = dcc_list; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
    {
        /* check DCC timeout */
        if (DCC_IS_FILE(ptr_dcc->type) && !DCC_ENDED(ptr_dcc->status))
        {
            if ((cfg_dcc_timeout != 0) && (time (NULL) > ptr_dcc->last_activity + cfg_dcc_timeout))
            {
                dcc_close (ptr_dcc, DCC_FAILED);
                dcc_redraw (HOTLIST_MSG);
                continue;
            }
        }
        
        if (ptr_dcc->status == DCC_CONNECTING)
        {
            if (ptr_dcc->type == DCC_FILE_SEND)
            {
                FD_ZERO (&read_fd);
                FD_SET (ptr_dcc->sock, &read_fd);
                timeout.tv_sec = 0;
                timeout.tv_usec = 0;
                
                /* something to read on socket? */
                if (select (FD_SETSIZE, &read_fd, NULL, NULL, &timeout) > 0)
                {
                    if (FD_ISSET (ptr_dcc->sock, &read_fd))
                    {
                        ptr_dcc->last_activity = time (NULL);
                        length = sizeof (addr);
                        sock = accept (ptr_dcc->sock, (struct sockaddr *) &addr, &length);
                        close (ptr_dcc->sock);
                        ptr_dcc->sock = -1;
                        if (sock < 0)
                        {
                            dcc_close (ptr_dcc, DCC_FAILED);
                            dcc_redraw (HOTLIST_MSG);
                            continue;
                        }
                        ptr_dcc->sock = sock;
                        if (fcntl (ptr_dcc->sock, F_SETFL, O_NONBLOCK) == -1)
                        {
                            dcc_close (ptr_dcc, DCC_FAILED);
                            dcc_redraw (HOTLIST_MSG);
                            continue;
                        }
                        ptr_dcc->addr = ntohl (addr.sin_addr.s_addr);
                        ptr_dcc->status = DCC_ACTIVE;
                        ptr_dcc->file = open (ptr_dcc->local_filename, O_RDONLY | O_NONBLOCK, 0644);
                        ptr_dcc->start_transfer = time (NULL);
                        dcc_redraw (HOTLIST_MSG);
                    }
                }
            }
        }
        
        if (ptr_dcc->status == DCC_WAITING)
        {
            if (ptr_dcc->type == DCC_CHAT_SEND)
            {
                FD_ZERO (&read_fd);
                FD_SET (ptr_dcc->sock, &read_fd);
                timeout.tv_sec = 0;
                timeout.tv_usec = 0;
                
                /* something to read on socket? */
                if (select (FD_SETSIZE, &read_fd, NULL, NULL, &timeout) > 0)
                {
                    if (FD_ISSET (ptr_dcc->sock, &read_fd))
                    {
                        length = sizeof (addr);
                        sock = accept (ptr_dcc->sock, (struct sockaddr *) &addr, &length);
                        close (ptr_dcc->sock);
                        ptr_dcc->sock = -1;
                        if (sock < 0)
                        {
                            dcc_close (ptr_dcc, DCC_FAILED);
                            dcc_redraw (HOTLIST_MSG);
                            continue;
                        }
                        ptr_dcc->sock = sock;
                        if (fcntl (ptr_dcc->sock, F_SETFL, O_NONBLOCK) == -1)
                        {
                            dcc_close (ptr_dcc, DCC_FAILED);
                            dcc_redraw (HOTLIST_MSG);
                            continue;
                        }
                        ptr_dcc->addr = ntohl (addr.sin_addr.s_addr);
                        ptr_dcc->status = DCC_ACTIVE;
                        dcc_redraw (HOTLIST_MSG);
                        dcc_channel_for_chat (ptr_dcc);
                    }
                }
            }
        }
        
        if (ptr_dcc->status == DCC_ACTIVE)
        {
            if (DCC_IS_CHAT(ptr_dcc->type))
            {
                FD_ZERO (&read_fd);
                FD_SET (ptr_dcc->sock, &read_fd);
                timeout.tv_sec = 0;
                timeout.tv_usec = 0;
                
                /* something to read on socket? */
                if (select (FD_SETSIZE, &read_fd, NULL, NULL, &timeout) > 0)
                {
                    if (FD_ISSET (ptr_dcc->sock, &read_fd))
                        dcc_chat_recv (ptr_dcc);
                }
            }
            if (ptr_dcc->type == DCC_FILE_RECV)
            {
                num_read = recv (ptr_dcc->sock, buffer, sizeof (buffer), 0);
                if (num_read != -1)
                {
                    if (num_read == 0)
                    {
                        dcc_close (ptr_dcc, DCC_FAILED);
                        dcc_redraw (HOTLIST_MSG);
                        continue;
                    }
                    
                    if (write (ptr_dcc->file, buffer, num_read) == -1)
                    {
                        dcc_close (ptr_dcc, DCC_FAILED);
                        dcc_redraw (HOTLIST_MSG);
                        continue;
                    }
                    ptr_dcc->last_activity = time (NULL);
                    ptr_dcc->pos += (unsigned long) num_read;
                    pos = htonl (ptr_dcc->pos);
                    send (ptr_dcc->sock, (char *) &pos, 4, 0);
                    dcc_calculate_speed (ptr_dcc, 0);
                    if (ptr_dcc->pos >= ptr_dcc->size)
                    {
                        dcc_close (ptr_dcc, DCC_DONE);
                        dcc_redraw (HOTLIST_MSG);
                    }
                    else
                        dcc_redraw (HOTLIST_LOW);
                }
            }
            if (ptr_dcc->type == DCC_FILE_SEND)
            {
                if (cfg_dcc_blocksize > (int) sizeof (buffer))
                {
                    irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                    gui_printf (NULL, _("%s DCC failed because blocksize is too "
                                "big. Check value of \"dcc_blocksize\" option, "
                                "max is %d.\n"),
                                sizeof (buffer));
                    dcc_close (ptr_dcc, DCC_FAILED);
                    dcc_redraw (HOTLIST_MSG);
                    continue;
                }
                if (ptr_dcc->pos > ptr_dcc->ack)
                {
                    /* we should receive ACK for packets sent previously */
                    num_read = recv (ptr_dcc->sock, (char *) &pos, 4, MSG_PEEK);
                    if (num_read != -1)
                    {
                        if (num_read == 0)
                        {
                            dcc_close (ptr_dcc, DCC_FAILED);
                            dcc_redraw (HOTLIST_MSG);
                            continue;
                        }
                        if (num_read < 4)
                            continue;
                        recv (ptr_dcc->sock, (char *) &pos, 4, 0);
                        ptr_dcc->ack = ntohl (pos);
                        
                        if ((ptr_dcc->pos >= ptr_dcc->size)
                            && (ptr_dcc->ack >= ptr_dcc->size))
                        {
                            dcc_close (ptr_dcc, DCC_DONE);
                            dcc_redraw (HOTLIST_MSG);
                            continue;
                        }
                    }
                }
                if (ptr_dcc->pos <= ptr_dcc->ack)
                {
                    lseek (ptr_dcc->file, ptr_dcc->pos, SEEK_SET);
                    num_read = read (ptr_dcc->file, buffer, cfg_dcc_blocksize);
                    if (num_read < 1)
                    {
                        dcc_close (ptr_dcc, DCC_FAILED);
                        dcc_redraw (HOTLIST_MSG);
                        continue;
                    }
                    num_sent = send (ptr_dcc->sock, buffer, num_read, 0);
                    if (num_sent < 0)
                    {
                        dcc_close (ptr_dcc, DCC_FAILED);
                        dcc_redraw (HOTLIST_MSG);
                        continue;
                    }
                    ptr_dcc->last_activity = time (NULL);
                    ptr_dcc->pos += (unsigned long) num_sent;
                    dcc_calculate_speed (ptr_dcc, 0);
                    dcc_redraw (HOTLIST_LOW);
                }
            }
        }
    }
}

/*
 * dcc_end: close all opened sockets (called when WeeChat is exiting)
 */

void
dcc_end ()
{
    t_irc_dcc *ptr_dcc;
    
    for (ptr_dcc = dcc_list; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
    {
        if (ptr_dcc->sock != -1)
        {
            if (ptr_dcc->status == DCC_ACTIVE)
                wee_log_printf (_("Aborting active DCC: \"%s\" from %s\n"),
                                ptr_dcc->filename, ptr_dcc->nick);
            dcc_close (ptr_dcc, DCC_FAILED);
        }
    }
}
