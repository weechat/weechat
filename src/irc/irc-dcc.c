/*
 * Copyright (c) 2004 by FlashCode <flashcode@flashtux.org>
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../common/weechat.h"
#include "irc.h"
#include "../common/weeconfig.h"
#include "../common/hotlist.h"
#include "../gui/gui.h"


t_dcc *dcc_list = NULL;         /* DCC files & chat list                    */
char *dcc_status_string[] =     /* strings for DCC status                   */
{ N_("Waiting"), N_("Connecting"), N_("Active"), N_("Done"), N_("Failed"),
  N_("Aborted") };


/*
 * dcc_redraw: redraw DCC buffer (and add to hotlist)
 */

void
dcc_redraw (int highlight)
{
    gui_draw_buffer_chat (gui_get_dcc_buffer (), 0);
    if (highlight)
    {
        hotlist_add (1, gui_get_dcc_buffer ());
        gui_draw_buffer_status (gui_current_window->buffer, 0);
    }
}

/*
 * dcc_connect: connect to another host
 */

void
dcc_connect (t_dcc *ptr_dcc)
{
    struct sockaddr_in addr;
    
    ptr_dcc->status = DCC_CONNECTING;
    
    ptr_dcc->sock = socket (AF_INET, SOCK_STREAM, 0);
    if (ptr_dcc->sock == -1)
        return;
    memset (&addr, 0, sizeof (addr));
    addr.sin_port = htons (ptr_dcc->port);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl (ptr_dcc->addr);
    fcntl (ptr_dcc->sock, F_SETFL, O_NONBLOCK);
    connect (ptr_dcc->sock, (struct sockaddr *) &addr, sizeof (addr));
}

/*
 * dcc_send: send DCC request (file or chat)
 */

void
dcc_send ()
{
}

/*
 * dcc_free: free DCC struct
 */

void
dcc_free (t_dcc *ptr_dcc)
{
    if (ptr_dcc->nick)
        free (ptr_dcc->nick);
    if (ptr_dcc->filename)
        free (ptr_dcc->filename);
}

/*
 * dcc_close: close a DCC connection
 */

void
dcc_close (t_dcc *ptr_dcc, int status)
{
    ptr_dcc->status = status;
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
dcc_accept (t_dcc *ptr_dcc)
{
    char *ptr_home, *filename2;
    
    dcc_connect (ptr_dcc);
    if (ptr_dcc->sock == -1)
        ptr_dcc->status = DCC_FAILED;
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
            ptr_dcc->status = DCC_FAILED;
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
                ptr_dcc->status = DCC_FAILED;
                dcc_redraw (1);
                return;
            }
            
            filename2 = (char *) malloc (strlen (ptr_dcc->local_filename) + 16);
            if (!filename2)
            {
                ptr_dcc->status = DCC_FAILED;
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
                              O_CREAT | O_TRUNC | O_WRONLY,
                              0644);
    }
    dcc_redraw (1);
}

/*
 * dcc_add: add a DCC file to queue
 */

t_dcc *
dcc_add (t_irc_server *server, int type, unsigned long addr, int port, char *nick, char *filename,
         unsigned int size)
{
    t_dcc *new_dcc;
    
    if ((new_dcc = (t_dcc *) malloc (sizeof (t_dcc))) == NULL)
    {
        gui_printf (NULL, _("%s not enough memory for new DCC\n"), WEECHAT_ERROR);
        return NULL;
    }
    new_dcc->server = server;
    new_dcc->type = type;
    new_dcc->status = DCC_WAITING;
    new_dcc->addr = addr;
    new_dcc->port = port;
    new_dcc->nick = strdup (nick);
    new_dcc->sock = -1;
    new_dcc->file = -1;
    new_dcc->filename = strdup (filename);
    new_dcc->local_filename = NULL;
    new_dcc->filename_suffix = -1;
    new_dcc->size = size;
    new_dcc->pos = 0;
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
        gui_printf_color (server->buffer,
                          COLOR_WIN_CHAT_NICK,
                          "%s",
                          nick);
        gui_printf_color (server->buffer,
                          COLOR_WIN_CHAT_DARK,
                          " (");
        gui_printf_color (server->buffer,
                          COLOR_WIN_CHAT_HOST,
                          "%d.%d.%d.%d",
                          addr >> 24, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff);
        gui_printf_color (server->buffer,
                          COLOR_WIN_CHAT_DARK,
                          ")");
        gui_printf (server->buffer, ": ");
        gui_printf_color (server->buffer,
                          COLOR_WIN_CHAT_CHANNEL,
                          "%s",
                          filename);
        gui_printf (server->buffer, ", ");
        gui_printf_color (server->buffer,
                          COLOR_WIN_CHAT_CHANNEL,
                          "%lu",
                          size);
        gui_printf (server->buffer, _(" bytes\n"));
    }
    
    if ( ( (type == DCC_CHAT_RECV) && (cfg_dcc_auto_accept_chats) )
        || ( (type == DCC_FILE_RECV) && (cfg_dcc_auto_accept_files) ) )
        dcc_accept (new_dcc);
    else
        hotlist_add (2, gui_get_dcc_buffer ());
    gui_draw_buffer_status (gui_current_window->buffer, 0);
    
    return new_dcc;
}

/*
 * dcc_handle: receive/send data for each active DCC
 */

void
dcc_handle ()
{
    t_dcc *ptr_dcc;
    int num;
    char buffer[8192];
    uint32_t pos;
    
    for (ptr_dcc = dcc_list; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
    {
        if (ptr_dcc->status == DCC_ACTIVE)
        {
            if (ptr_dcc->type == DCC_FILE_RECV)
            {
                num = recv (ptr_dcc->sock, buffer, sizeof (buffer), 0);
                if (num != -1)
                {
                    if (num == 0)
                    {
                        dcc_close (ptr_dcc, DCC_FAILED);
                        dcc_redraw (1);
                    }
                    else
                    {
                        if (write (ptr_dcc->file, buffer, num) == -1)
                        {
                            dcc_close (ptr_dcc, DCC_FAILED);
                            dcc_redraw (1);
                            return;
                        }
                        ptr_dcc->pos += (unsigned long) num;
                        pos = htonl (ptr_dcc->pos);
                        send (ptr_dcc->sock, (char *) &pos, 4, 0);
                        if (ptr_dcc->pos >= ptr_dcc->size)
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
                            gui_printf (ptr_dcc->server->buffer, _(") from "));
                            gui_printf_color (ptr_dcc->server->buffer,
                                              COLOR_WIN_CHAT_NICK,
                                              "%s",
                                              ptr_dcc->nick);
                            gui_printf (ptr_dcc->server->buffer, _(": ok!\n"));
                            dcc_close (ptr_dcc, DCC_DONE);
                            dcc_redraw (1);
                        }
                        dcc_redraw (0);
                    }
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
    t_dcc *ptr_dcc;
    
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
