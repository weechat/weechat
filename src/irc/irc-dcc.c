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
#include "../gui/gui.h"


t_dcc *dcc_list = NULL;         /* DCC files & chat list                    */
char *dcc_status_string[] =     /* strings for DCC status                   */
{ N_("Waiting"), N_("Connecting"), N_("Active"), N_("Done"), N_("Failed"),
  N_("Aborted") };


/*
 * dcc_connect: connect to another host
 */

void
dcc_connect (t_dcc *dcc)
{
    struct sockaddr_in addr;
    
    dcc->status = DCC_CONNECTING;
    
    dcc->sock = socket (AF_INET, SOCK_STREAM, 0);
    if (dcc->sock == -1)
        return;
    memset (&addr, 0, sizeof (addr));
    addr.sin_port = htons (dcc->port);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl (dcc->addr);
    fcntl (dcc->sock, F_SETFL, O_NONBLOCK);
    connect (dcc->sock, (struct sockaddr *) &addr, sizeof (addr));
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
 * dcc_add: add a DCC file to queue
 */

t_dcc *
dcc_add (t_irc_server *server, int type, unsigned long addr, int port, char *nick, char *filename,
         unsigned int size)
{
    t_dcc *new_dcc;
    int num;
    char *filename2;
    
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
    new_dcc->size = size;
    new_dcc->pos = 0;
    new_dcc->next_dcc = dcc_list;
    dcc_list = new_dcc;
    
    gui_printf (NULL, "Incoming DCC file: from %s, address:%d.%d.%d.%d, "
                "port=%d, name=%s, size=%lu bytes\n",
                nick,
                addr >> 24, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff,
                port, filename, size);
    
    if (type == DCC_FILE_RECV)
    {
        dcc_connect (new_dcc);
        if (new_dcc->sock == -1)
            new_dcc->status = DCC_FAILED;
        else
        {
            new_dcc->status = DCC_ACTIVE;
            new_dcc->local_filename = (char *) malloc (strlen (cfg_dcc_download_path) +
                                                       strlen (nick) +
                                                       strlen (filename) + 4);
            if (!new_dcc->local_filename)
            {
                new_dcc->status = DCC_FAILED;
                return NULL;
            }
            strcpy (new_dcc->local_filename, cfg_dcc_download_path);
            if (new_dcc->local_filename[strlen (new_dcc->local_filename) - 1] != '/')
                strcat (new_dcc->local_filename, "/");
            strcat (new_dcc->local_filename, nick);
            strcat (new_dcc->local_filename, ".");
            strcat (new_dcc->local_filename, filename);
            if (access (new_dcc->local_filename, F_OK) == 0)
            {
                filename2 = (char *) malloc (strlen (new_dcc->local_filename) + 16);
                if (!filename2)
                {
                    new_dcc->status = DCC_FAILED;
                    return NULL;
                }
                num = 0;
                do
                {
                    num++;
                    sprintf (filename2, "%s.%d", new_dcc->local_filename, num);
                }
                while (access (filename2, F_OK) == 0);
                
                free (new_dcc->local_filename);
                new_dcc->local_filename = strdup (filename2);
                free (filename2);
            }
            new_dcc->file = open (new_dcc->local_filename,
                                  O_CREAT | O_TRUNC | O_WRONLY,
                                  0644);
        }
    }
    
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
                        dcc_close (ptr_dcc, DCC_FAILED);
                    else
                    {
                        if (write (ptr_dcc->file, buffer, num) == -1)
                        {
                            dcc_close (ptr_dcc, DCC_FAILED);
                            return;
                        }
                        ptr_dcc->pos += (unsigned long) num;
                        pos = htonl (ptr_dcc->pos);
                        send (ptr_dcc->sock, (char *) &pos, 4, 0);
                        if (ptr_dcc->pos >= ptr_dcc->size)
                        {
                            gui_printf (NULL,
                                        _("DCC: file \"%s\" received from \"%s\": done!\n"),
                                        ptr_dcc->filename, ptr_dcc->nick);
                            dcc_close (ptr_dcc, DCC_DONE);
                        }
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
            close (ptr_dcc->sock);
        }
    }
}
