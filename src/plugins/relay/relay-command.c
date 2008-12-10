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

/* relay-command.c: relay command */


#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-buffer.h"
#include "relay-client.h"
#include "relay-config.h"


/*
 * relay_command_client_list: list clients
 */

void
relay_command_client_list (int full)
{
    struct t_relay_client *ptr_client;
    int i;
    char date_start[128], date_activity[128];
    struct tm *date_tmp;
    
    if (relay_clients)
    {
        weechat_printf (NULL, "");
        weechat_printf (NULL, _("Clients for relay:"));
        i = 1;
        for (ptr_client = relay_clients; ptr_client;
             ptr_client = ptr_client->next_client)
        {
            date_tmp = localtime (&(ptr_client->start_time));
            strftime (date_start, sizeof (date_start),
                      "%a, %d %b %Y %H:%M:%S", date_tmp);
            
            date_tmp = localtime (&(ptr_client->last_activity));
            strftime (date_activity, sizeof (date_activity),
                      "%a, %d %b %Y %H:%M:%S", date_tmp);
                
            if (full)
            {
                weechat_printf (NULL,
                                _("%3d. %s, started on: %s, last activity: %s, "
                                  "bytes: %lu recv, %lu sent"),
                                i,
                                ptr_client->address,
                                date_start,
                                date_activity,
                                ptr_client->bytes_recv,
                                ptr_client->bytes_sent);
            }
            else
            {
                weechat_printf (NULL,
                                _("%3d. %s, started on: %s"),
                                i,
                                ptr_client->address);
            }
            i++;
        }
    }
    else
        weechat_printf (NULL, _("No client for relay"));
}

/*
 * relay_command_relay: command /relay
 */

int
relay_command_relay (void *data, struct t_gui_buffer *buffer, int argc,
                     char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;

    if ((argc > 1) && (weechat_strcasecmp (argv[1], "list") == 0))
    {
        relay_command_client_list (0);
        return WEECHAT_RC_OK;
    }
    
    if ((argc > 1) && (weechat_strcasecmp (argv[1], "listfull") == 0))
    {
        relay_command_client_list (1);
        return WEECHAT_RC_OK;
    }
    
    if (!relay_buffer)
        relay_buffer_open ();
    
    if (relay_buffer)
    {
        weechat_buffer_set (relay_buffer, "display", "1");
        
        if (argc > 1)
        {
            if (strcmp (argv[1], "up") == 0)
            {
                if (relay_buffer_selected_line > 0)
                    relay_buffer_selected_line--;
            }
            else if (strcmp (argv[1], "down") == 0)
            {
                if (relay_buffer_selected_line < relay_client_count - 1)
                    relay_buffer_selected_line++;
            }
        }
    }
    
    relay_buffer_refresh (NULL);
    
    return WEECHAT_RC_OK;
}

/*
 * relay_command_init: add /relay command
 */

void
relay_command_init ()
{
    weechat_hook_command ("relay",
                          N_("relay control"),
                          "[list | listfull]",
                          N_("    list: list relay clients\n"
                             "listfull: list relay clients (verbose)\n\n"
                             "Without argument, this command opens buffer "
                             "with list of relay clients."),
                          "list|listfull", &relay_command_relay, NULL);
}
