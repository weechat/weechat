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
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

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
    gui_redraw_buffer (gui_get_dcc_buffer ());
    if (highlight)
    {
        hotlist_add (highlight, gui_get_dcc_buffer ());
        gui_draw_buffer_status (gui_current_window->buffer, 0);
    }
}

/*
 * dcc_connect: connect to another host
 */

int
dcc_connect (t_irc_dcc *ptr_dcc)
{
    struct sockaddr_in addr;
    
    ptr_dcc->status = DCC_CONNECTING;
    
    if (ptr_dcc->sock == -1)
    {
        ptr_dcc->sock = socket (AF_INET, SOCK_STREAM, 0);
        if (ptr_dcc->sock == -1)
            return 0;
    }
    if (fcntl (ptr_dcc->sock, F_SETFL, O_NONBLOCK) == -1)
        return 0;
    
    /* for DCC SEND, listen to socket for a connection */
    if (ptr_dcc->type == DCC_FILE_SEND)
    {
        if (listen (ptr_dcc->sock, 1) == -1)
            return 0;
        if (fcntl (ptr_dcc->sock, F_SETFL, 0) == -1)
            return 0;
    }
    
    /* for DCC RECV, connect to listening host */
    if (ptr_dcc->type == DCC_FILE_RECV)
    {
        memset (&addr, 0, sizeof (addr));
        addr.sin_port = htons (ptr_dcc->port);
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl (ptr_dcc->addr);
        connect (ptr_dcc->sock, (struct sockaddr *) &addr, sizeof (addr));
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
    ptr_dcc->status = status;
    
    if (status == DCC_DONE)
    {
        if ((ptr_dcc->type == DCC_FILE_SEND) || (ptr_dcc->type == DCC_FILE_RECV))
        {
            irc_display_prefix (ptr_dcc->server->buffer, PREFIX_INFO);
            gui_printf (ptr_dcc->server->buffer, _("DCC: file "));
            gui_printf_color (ptr_dcc->server->buffer,
                              COLOR_WIN_CHAT_CHANNEL,
                              "%s",
                              ptr_dcc->filename);
            gui_printf (ptr_dcc->server->buffer, _(" (local filename: "));
            gui_printf_color (ptr_dcc->server->buffer,
                              COLOR_WIN_CHAT_CHANNEL,
                              "%s",
                              ptr_dcc->local_filename);
            if (ptr_dcc->type == DCC_FILE_SEND)
                gui_printf (ptr_dcc->server->buffer, _(") sent to "));
            else
                gui_printf (ptr_dcc->server->buffer, _(") received from "));
            gui_printf_color (ptr_dcc->server->buffer,
                              COLOR_WIN_CHAT_NICK,
                              "%s",
                              ptr_dcc->nick);
            gui_printf (ptr_dcc->server->buffer, _(": ok!\n"));
        }
    }
    
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
 * dcc_accept: accepts a DCC file or chat request
 */

void
dcc_accept (t_irc_dcc *ptr_dcc)
{
    char *ptr_home, *filename2;
    
    if (!dcc_connect (ptr_dcc))
    {
        dcc_close (ptr_dcc, DCC_FAILED);
        dcc_redraw (1);
    }
    else
    {
        ptr_dcc->status = DCC_ACTIVE;
        ptr_home = getenv ("HOME");
        ptr_dcc->local_filename = (char *) malloc (strlen (cfg_dcc_download_path) +
                                                   strlen (ptr_dcc->nick) +
                                                   strlen (ptr_dcc->filename) +
                                                   ((cfg_dcc_download_path[0] == '~') ?
                                                       strlen (ptr_home) : 0) +
                                                   4);
        if (!ptr_dcc->local_filename)
        {
            dcc_close (ptr_dcc, DCC_FAILED);
            dcc_redraw (1);
            return;
        }
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
            /* if auto rename is not set, then abort DCC */
            if (!cfg_dcc_auto_rename)
            {
                dcc_close (ptr_dcc, DCC_FAILED);
                dcc_redraw (1);
                return;
            }
            
            filename2 = (char *) malloc (strlen (ptr_dcc->local_filename) + 16);
            if (!filename2)
            {
                dcc_close (ptr_dcc, DCC_FAILED);
                dcc_redraw (1);
                return;
            }
            ptr_dcc->filename_suffix = 0;
            do
            {
                ptr_dcc->filename_suffix++;
                sprintf (filename2, "%s.%d",
                         ptr_dcc->local_filename,
                         ptr_dcc->filename_suffix);
            }
            while (access (filename2, F_OK) == 0);
            
            free (ptr_dcc->local_filename);
            ptr_dcc->local_filename = strdup (filename2);
            free (filename2);
        }
        ptr_dcc->file = open (ptr_dcc->local_filename,
                              O_CREAT | O_TRUNC | O_WRONLY | O_NONBLOCK,
                              0644);
    }
    dcc_redraw (1);
}

/*
 * dcc_add: add a DCC file to queue
 */

t_irc_dcc *
dcc_add (t_irc_server *server, int type, unsigned long addr, int port, char *nick,
         int sock, char *filename, char *local_filename, unsigned long size)
{
    t_irc_dcc *new_dcc;
    
    if ((new_dcc = (t_irc_dcc *) malloc (sizeof (t_irc_dcc))) == NULL)
    {
        gui_printf_nolog (server->buffer,
                          _("%s not enough memory for new DCC\n"),
                          WEECHAT_ERROR);
        return NULL;
    }
    new_dcc->server = server;
    new_dcc->type = type;
    new_dcc->status = DCC_WAITING;
    new_dcc->addr = addr;
    new_dcc->port = port;
    new_dcc->nick = strdup (nick);
    new_dcc->sock = sock;
    new_dcc->file = -1;
    new_dcc->filename = strdup (filename);
    new_dcc->local_filename = (local_filename) ? strdup (local_filename) : NULL;
    new_dcc->filename_suffix = -1;
    new_dcc->size = size;
    new_dcc->pos = 0;
    new_dcc->ack = 0;
    new_dcc->prev_dcc = NULL;
    new_dcc->next_dcc = dcc_list;
    if (dcc_list)
        dcc_list->prev_dcc = new_dcc;
    dcc_list = new_dcc;
    
    gui_current_window->dcc_first = NULL;
    gui_current_window->dcc_selected = NULL;
    
    if (type == DCC_FILE_RECV)
    {
        irc_display_prefix (server->buffer, PREFIX_INFO);
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
    }
    
    if (type == DCC_FILE_SEND)
    {
        irc_display_prefix (server->buffer, PREFIX_INFO);
        gui_printf (server->buffer, _("Sending DCC file to "));
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_NICK, "%s", nick);
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_DARK, " (");
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_HOST,
                          "%d.%d.%d.%d",
                          addr >> 24, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff);
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_DARK, ")");
        gui_printf (server->buffer, ": ");
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_CHANNEL, "%s", filename);
        gui_printf (server->buffer, _(" (local filename: "));
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_CHANNEL, "%s", local_filename);
        gui_printf (server->buffer, "), ");
        gui_printf_color (server->buffer, COLOR_WIN_CHAT_CHANNEL, "%lu", size);
        gui_printf (server->buffer, _(" bytes\n"));
    }
    
    if (type == DCC_FILE_SEND)
    {
        if (!dcc_connect (new_dcc))
        {
            dcc_close (new_dcc, DCC_FAILED);
            return NULL;
        }
    }
    
    if ( ( (type == DCC_CHAT_RECV) && (cfg_dcc_auto_accept_chats) )
        || ( (type == DCC_FILE_RECV) && (cfg_dcc_auto_accept_files) ) )
        dcc_accept (new_dcc);
    else
        dcc_redraw (2);
    gui_draw_buffer_status (gui_current_window->buffer, 0);
    
    return new_dcc;
}

/*
 * dcc_send: send DCC request (file or chat)
 */

void
dcc_send (t_irc_server *server, char *nick, char *filename)
{
    char *ptr_home, *filename2, *short_filename, *pos;
    int spaces;
    struct stat st;
    int sock, port;
    struct sockaddr_in addr;
    socklen_t length;
    unsigned long local_addr;
    
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
            irc_display_prefix (server->buffer, PREFIX_ERROR);
            gui_printf_nolog (server->buffer,
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
        irc_display_prefix (server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s cannot access file \"%s\"\n"),
                          WEECHAT_ERROR, filename2);
        free (filename2);
        return;
    }
    
    /* get local IP address */
    memset (&addr, 0, sizeof (struct sockaddr_in));
    length = sizeof (addr);
    getsockname (server->sock4, (struct sockaddr *) &addr, &length);
    addr.sin_family = AF_INET;
    local_addr = ntohl (addr.sin_addr.s_addr);
    
    /* open socket for DCC */
    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        irc_display_prefix (server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s cannot create socket for DCC\n"),
                          WEECHAT_ERROR);
        free (filename2);
        return;
    }
    
    /* find port automatically */
    addr.sin_port = 0;
    if (bind (sock, (struct sockaddr *) &addr, sizeof (addr)) == -1)
    {
        irc_display_prefix (server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s cannot find port for DCC\n"),
                          WEECHAT_ERROR);
        close (sock);
        free (filename2);
        return;
    }
    length = sizeof (addr);
    getsockname (sock, (struct sockaddr *) &addr, &length);
    port = ntohs (addr.sin_port);
    
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
    
    /* add DCC entry and listen to socket */
    if (!dcc_add (server, DCC_FILE_SEND, local_addr, port, nick, sock,
                  short_filename, filename2, st.st_size))
    {
        irc_display_prefix (server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s cannot send DCC\n"),
                          WEECHAT_ERROR);
        close (sock);
        free (short_filename);
        free (filename2);
        return;
    }
    
    /* send DCC request to nick */
    server_sendf (server, 
                  (spaces) ?
                      "PRIVMSG %s :\01DCC SEND \"%s\" %lu %d %u\01\r\n" :
                      "PRIVMSG %s :\01DCC SEND %s %lu %d %u\01\r\n",
                  nick, short_filename, local_addr, port,
                  (unsigned long) st.st_size);
    
    free (short_filename);
    free (filename2);
}


/*
 * dcc_handle: receive/send data for each active DCC
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
                        length = sizeof (addr);
                        sock = accept (ptr_dcc->sock, (struct sockaddr *) &addr, &length);
                        close (ptr_dcc->sock);
                        ptr_dcc->sock = -1;
                        if (sock < 0)
                        {
                            dcc_close (ptr_dcc, DCC_FAILED);
                            dcc_redraw (1);
                            return;
                        }
                        ptr_dcc->sock = sock;
                        if (fcntl (ptr_dcc->sock, F_SETFL, O_NONBLOCK) == -1)
                        {
                            dcc_close (ptr_dcc, DCC_FAILED);
                            dcc_redraw (1);
                            return;
                        }
                        ptr_dcc->addr = ntohl (addr.sin_addr.s_addr);
                        ptr_dcc->status = DCC_ACTIVE;
                        ptr_dcc->file = open (ptr_dcc->local_filename, O_RDONLY | O_NONBLOCK, 0644);
                    }
                }
            }
        }
        
        if (ptr_dcc->status == DCC_ACTIVE)
        {
            if (ptr_dcc->type == DCC_FILE_RECV)
            {
                num_read = recv (ptr_dcc->sock, buffer, sizeof (buffer), 0);
                if (num_read != -1)
                {
                    if (num_read == 0)
                    {
                        dcc_close (ptr_dcc, DCC_FAILED);
                        dcc_redraw (1);
                        return;
                    }
                    
                    if (write (ptr_dcc->file, buffer, num_read) == -1)
                    {
                        dcc_close (ptr_dcc, DCC_FAILED);
                        dcc_redraw (1);
                        return;
                    }
                    ptr_dcc->pos += (unsigned long) num_read;
                    pos = htonl (ptr_dcc->pos);
                    send (ptr_dcc->sock, (char *) &pos, 4, 0);
                    if (ptr_dcc->pos >= ptr_dcc->size)
                    {
                        dcc_close (ptr_dcc, DCC_DONE);
                        dcc_redraw (1);
                    }
                    else
                        dcc_redraw (0);
                }
            }
            if (ptr_dcc->type == DCC_FILE_SEND)
            {
                if (cfg_dcc_blocksize > (int) sizeof (buffer))
                {
                    gui_printf (NULL, _("%s DCC failed because blocksize is too "
                                "big. Check value of \"dcc_blocksize\" option, "
                                "max is %d.\n"),
                                sizeof (buffer));
                    dcc_close (ptr_dcc, DCC_FAILED);
                    dcc_redraw (1);
                    return;
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
                            dcc_redraw (1);
                            return;
                        }
                        if (num_read < 4)
                            return;
                        recv (ptr_dcc->sock, (char *) &pos, 4, 0);
                        ptr_dcc->ack = ntohl (pos);
                        
                        if ((ptr_dcc->pos >= ptr_dcc->size)
                            && (ptr_dcc->ack >= ptr_dcc->size))
                        {
                            dcc_close (ptr_dcc, DCC_DONE);
                            dcc_redraw (1);
                            return;
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
                        dcc_redraw (1);
                        return;
                    }
                    num_sent = send (ptr_dcc->sock, buffer, num_read, 0);
                    if (num_sent < 0)
                    {
                        dcc_close (ptr_dcc, DCC_FAILED);
                        dcc_redraw (1);
                        return;
                    }
                    ptr_dcc->pos += (unsigned long) num_sent;
                    dcc_redraw (0);
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
                wee_log_printf (_("aborting active DCC: \"%s\" from %s\n"),
                                ptr_dcc->filename, ptr_dcc->nick);
            dcc_close (ptr_dcc, DCC_FAILED);
        }
    }
}
