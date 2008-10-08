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

/* irc.c: IRC plugin for WeeChat */


#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-bar-item.h"
#include "irc-command.h"
#include "irc-completion.h"
#include "irc-config.h"
#include "irc-debug.h"
#include "irc-info.h"
#include "irc-server.h"
#include "irc-channel.h"
#include "irc-nick.h"
#include "irc-upgrade.h"


WEECHAT_PLUGIN_NAME(IRC_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION("IRC (Internet Relay Chat) plugin for WeeChat");
WEECHAT_PLUGIN_AUTHOR("FlashCode <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_WEECHAT_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE("GPL3");

struct t_weechat_plugin *weechat_irc_plugin = NULL;

struct t_hook *irc_hook_timer = NULL;
struct t_hook *irc_hook_timer_check_away = NULL;

int irc_signal_upgrade_received = 0;   /* signal "upgrade" received ?       */


/*
 * irc_signal_quit_cb: callback for "quit" signal
 */

int
irc_signal_quit_cb (void *data, const char *signal, const char *type_data,
                    void *signal_data)
{
    struct t_irc_server *ptr_server;
    
    /* make C compiler happy */
    (void) data;
    (void) signal;
    
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            irc_command_quit_server (ptr_server,
                                     (signal_data) ? (char *)signal_data : NULL);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_signal_upgrade_cb: callback for "upgrade" signal
 */

int
irc_signal_upgrade_cb (void *data, const char *signal, const char *type_data,
                       void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;
    
    irc_signal_upgrade_received = 1;
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_init: initialize IRC plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    int i, auto_connect, upgrading;
    
    weechat_plugin = plugin;
    
    if (!irc_config_init ())
        return WEECHAT_RC_ERROR;
    
    if (irc_config_read () < 0)
        return WEECHAT_RC_ERROR;
    
    irc_command_init ();
    
    irc_info_init ();
    
    /* hook some signals */
    irc_debug_init ();
    weechat_hook_signal ("quit", &irc_signal_quit_cb, NULL);
    weechat_hook_signal ("upgrade", &irc_signal_upgrade_cb, NULL);
    weechat_hook_signal ("xfer_send_ready", &irc_server_xfer_send_ready_cb, NULL);
    weechat_hook_signal ("xfer_resume_ready", &irc_server_xfer_resume_ready_cb, NULL);
    weechat_hook_signal ("xfer_send_accept_resume", &irc_server_xfer_send_accept_resume_cb, NULL);
    
    /* hook completions */
    irc_completion_init ();
    
    irc_bar_item_init ();
    
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
        else if ((weechat_strncasecmp (argv[i], IRC_PLUGIN_NAME, 3) == 0))
        {
            if (!irc_server_alloc_with_url (argv[i]))
            {
                weechat_printf (NULL,
                                _("%s%s: error with IRC server from URL "
                                  "(\"%s\"), ignored"),
                                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                argv[i]);
            }
        }
        else if (weechat_strcasecmp (argv[i], "--upgrade") == 0)
        {
            upgrading = 1;
        }
    }
    
    if (upgrading)
    {
        if (!irc_upgrade_load ())
        {
            weechat_printf (NULL,
                            _("%s%s: WARNING: some network connections may "
                              "still be open and not visible, you should "
                              "restart WeeChat now (with /quit)."),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME);
        }
    }
    else
        irc_server_auto_connect (auto_connect);
    
    irc_hook_timer = weechat_hook_timer (1 * 1000, 0, 0,
                                         &irc_server_timer_cb, NULL);
    
    /*
    if (irc_cfg_irc_away_check != 0)
        irc_hook_timer_check_away = weechat_hook_timer (irc_cfg_irc_away_check * 60 * 1000,
                                                        0,
                                                        &irc_server_timer_check_away,
                                                        NULL);
    */
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end IRC plugin
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;
    
    irc_config_write ();
    
    if (irc_signal_upgrade_received)
        irc_upgrade_save ();
    else
        irc_server_disconnect_all ();
    
    irc_server_free_all ();
    
    irc_config_free ();
    
    return WEECHAT_RC_OK;
}
