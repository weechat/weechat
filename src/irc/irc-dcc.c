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
#include <string.h>

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
dcc_connect ()
{
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
    new_dcc->status = DCC_ACTIVE;
    new_dcc->addr = addr;
    new_dcc->port = port;
    new_dcc->nick = strdup (nick);
    new_dcc->file = -1;
    new_dcc->filename = strdup (filename);
    new_dcc->size = size;
    new_dcc->pos = 0;
    new_dcc->next_dcc = dcc_list;
    dcc_list = new_dcc;
    
    gui_printf (NULL, "Incoming DCC file: type:%d, nick:%s, address:%d.%d.%d.%d, "
                "port=%d, name=%s, size=%lu\n",
                type, nick,
                addr >> 24, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff,
                port, filename, size);
    return new_dcc;
}
