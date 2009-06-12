/*
 * Copyright (c) 2009 by FlashCode <flashcode@flashtux.org>
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

/* jabber.c: Jabber plugin for WeeChat */


#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "jabber.h"
#include "jabber-bar-item.h"
#include "jabber-command.h"
#include "jabber-completion.h"
#include "jabber-config.h"
#include "jabber-debug.h"
#include "jabber-info.h"
#include "jabber-server.h"
#include "jabber-upgrade.h"


WEECHAT_PLUGIN_NAME(JABBER_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION("Jabber plugin for WeeChat");
WEECHAT_PLUGIN_AUTHOR("FlashCode <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE("GPL3");

struct t_weechat_plugin *weechat_jabber_plugin = NULL;

struct t_hook *jabber_hook_timer = NULL;

int jabber_signal_upgrade_received = 0;   /* signal "upgrade" received ?    */


/*
 * jabber_signal_quit_cb: callback for "quit" signal
 */

int
jabber_signal_quit_cb (void *data, const char *signal, const char *type_data,
                       void *signal_data)
{
    struct t_jabber_server *ptr_server;
    
    /* make C compiler happy */
    (void) data;
    (void) signal;
    
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        for (ptr_server = jabber_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            jabber_command_quit_server (ptr_server,
                                        (signal_data) ? (char *)signal_data : NULL);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_signal_upgrade_cb: callback for "upgrade" signal
 */

int
jabber_signal_upgrade_cb (void *data, const char *signal, const char *type_data,
                          void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;
    
    jabber_signal_upgrade_received = 1;
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_init: initialize Jabber plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    int i, auto_connect, upgrading;
    
    weechat_plugin = plugin;
    
    if (!jabber_config_init ())
        return WEECHAT_RC_ERROR;
    
    if (jabber_config_read () < 0)
        return WEECHAT_RC_ERROR;
    
    jabber_command_init ();
    
    jabber_info_init ();
    
    /* hook some signals */
    jabber_debug_init ();
    weechat_hook_signal ("quit", &jabber_signal_quit_cb, NULL);
    weechat_hook_signal ("upgrade", &jabber_signal_upgrade_cb, NULL);
    //weechat_hook_signal ("xfer_send_ready", &jabber_server_xfer_send_ready_cb, NULL);
    //weechat_hook_signal ("xfer_resume_ready", &jabber_server_xfer_resume_ready_cb, NULL);
    //weechat_hook_signal ("xfer_send_accept_resume", &jabber_server_xfer_send_accept_resume_cb, NULL);
    
    /* hook completions */
    jabber_completion_init ();
    
    jabber_bar_item_init ();
    
    /* look at arguments */
    auto_connect = 1;
    upgrading = 0;
    for (i = 0; i < argc; i++)
    {
        if ((weechat_strcasecmp (argv[i], "-a") == 0)
            || (weechat_strcasecmp (argv[i], "--no-connect") == 0))
        {
            auto_connect = 0;
        }
        else if ((weechat_strncasecmp (argv[i], JABBER_PLUGIN_NAME, 3) == 0))
        {
            /*
            if (!jabber_server_alloc_with_url (argv[i]))
            {
                weechat_printf (NULL,
                                _("%s%s: error with server from URL "
                                  "(\"%s\"), ignored"),
                                weechat_prefix ("error"), JABBER_PLUGIN_NAME,
                                argv[i]);
            }
            */
        }
        else if (weechat_strcasecmp (argv[i], "--upgrade") == 0)
        {
            upgrading = 1;
        }
    }
    
    if (upgrading)
    {
        if (!jabber_upgrade_load ())
        {
            weechat_printf (NULL,
                            _("%s%s: WARNING: some network connections may "
                              "still be opened and not visible, you should "
                              "restart WeeChat now (with /quit)."),
                            weechat_prefix ("error"), JABBER_PLUGIN_NAME);
        }
    }
    else
    {
        if (auto_connect)
            jabber_server_auto_connect ();
    }
    
    jabber_hook_timer = weechat_hook_timer (1 * 1000, 0, 0,
                                            &jabber_server_timer_cb, NULL);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end Jabber plugin
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;
    
    if (jabber_hook_timer)
        weechat_unhook (jabber_hook_timer);
    
    if (jabber_signal_upgrade_received)
    {
        jabber_config_write (1);
        jabber_upgrade_save ();
    }
    else
    {
        jabber_config_write (0);
        jabber_server_disconnect_all ();
    }
    
    jabber_server_free_all ();
    
    jabber_config_free ();
    
    return WEECHAT_RC_OK;
}
