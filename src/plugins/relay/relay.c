/*
 * Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
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

/*
 * relay.c: network communication between WeeChat and remote client
 */


#include <stdlib.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-buffer.h"
#include "relay-client.h"
#include "relay-command.h"
#include "relay-completion.h"
#include "relay-config.h"
#include "relay-info.h"
#include "relay-server.h"
#include "relay-upgrade.h"


WEECHAT_PLUGIN_NAME(RELAY_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION("Network communication between WeeChat and "
                           "remote application");
WEECHAT_PLUGIN_AUTHOR("FlashCode <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);

struct t_weechat_plugin *weechat_relay_plugin = NULL;

int relay_signal_upgrade_received = 0; /* signal "upgrade" received ?       */

char *relay_protocol_string[] =        /* strings for protocols             */
{ "weechat", "irc" };


/*
 * relay_protocol_search: search a protocol by name
 */

int
relay_protocol_search (const char *name)
{
    int i;

    for (i = 0; i < RELAY_NUM_PROTOCOLS; i++)
    {
        if (weechat_strcasecmp (relay_protocol_string[i], name) == 0)
        {
            return i;
        }
    }
    
    /* protocol not found */
    return -1;
}

/*
 * relay_signal_upgrade_cb: callback for "upgrade" signal
 */

int
relay_signal_upgrade_cb (void *data, const char *signal, const char *type_data,
                         void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;
    
    relay_signal_upgrade_received = 1;
    
    return WEECHAT_RC_OK;
}

/*
 * relay_debug_dump_cb: callback for "debug_dump" signal
 */

int
relay_debug_dump_cb (void *data, const char *signal, const char *type_data,
                     void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;
    
    weechat_log_printf ("");
    weechat_log_printf ("***** \"%s\" plugin dump *****",
                        weechat_plugin->name);
    
    relay_server_print_log ();
    relay_client_print_log ();
    
    weechat_log_printf ("");
    weechat_log_printf ("***** End of \"%s\" plugin dump *****",
                        weechat_plugin->name);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_init: initialize relay plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    weechat_plugin = plugin;
    
    if (!relay_config_init ())
        return WEECHAT_RC_ERROR;
    
    if (relay_config_read () < 0)
        return WEECHAT_RC_ERROR;
    
    relay_command_init ();
    
    /* hook completions */
    relay_completion_init ();
    
    weechat_hook_signal ("upgrade", &relay_signal_upgrade_cb, NULL);
    weechat_hook_signal ("debug_dump", &relay_debug_dump_cb, NULL);
    
    relay_info_init ();
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end relay plugin
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;
    
    relay_config_write ();
    
    if (relay_signal_upgrade_received)
        relay_upgrade_save ();
    else
    {
        /* remove all servers */
        relay_server_free_all ();
        
        /* remove all clients */
        relay_client_disconnect_all ();
        if (relay_buffer)
            weechat_buffer_close (relay_buffer);
        relay_client_free_all ();
    }
    
    return WEECHAT_RC_OK;
}
