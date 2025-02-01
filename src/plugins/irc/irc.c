/*
 * irc.c - IRC (Internet Relay Chat) plugin for WeeChat
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-bar-item.h"
#include "irc-batch.h"
#include "irc-buffer.h"
#include "irc-channel.h"
#include "irc-color.h"
#include "irc-command.h"
#include "irc-completion.h"
#include "irc-config.h"
#include "irc-debug.h"
#include "irc-ignore.h"
#include "irc-info.h"
#include "irc-input.h"
#include "irc-list.h"
#include "irc-nick.h"
#include "irc-notify.h"
#include "irc-protocol.h"
#include "irc-raw.h"
#include "irc-redirect.h"
#include "irc-server.h"
#include "irc-tag.h"
#include "irc-typing.h"
#include "irc-upgrade.h"


WEECHAT_PLUGIN_NAME(IRC_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("IRC (Internet Relay Chat) protocol"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(IRC_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_irc_plugin = NULL;

struct t_hook *irc_hook_timer = NULL;

int irc_signal_quit_received = 0;      /* signal "quit" received?           */


/*
 * Callback for signal "quit".
 */

int
irc_signal_quit_cb (const void *pointer, void *data,
                    const char *signal, const char *type_data,
                    void *signal_data)
{
    struct t_irc_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;

    irc_signal_quit_received = 1;

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            irc_command_quit_server (
                ptr_server,
                (signal_data) ? (char *)signal_data : NULL);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for signal "upgrade".
 */

int
irc_signal_upgrade_cb (const void *pointer, void *data,
                       const char *signal, const char *type_data,
                       void *signal_data)
{
    struct t_irc_server *ptr_server;
    int quit, tls_disconnected;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    /* only save session and continue? */
    if (signal_data && (strcmp (signal_data, "save") == 0))
    {
        /*
         * save session with a disconnected state in servers and a scheduled
         * reconnection
         */
        if (!irc_upgrade_save (1))
        {
            weechat_printf (
                NULL,
                _("%s%s: failed to save upgrade data"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            return WEECHAT_RC_ERROR;
        }
        return WEECHAT_RC_OK;
    }

    quit = (signal_data && (strcmp (signal_data, "quit") == 0));

    tls_disconnected = 0;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        /*
         * FIXME: it's not possible to upgrade with TLS servers connected
         * (GnuTLS library can't reload data after upgrade), so we close
         * connection for all TLS servers currently connected
         */
        if (ptr_server->is_connected && (ptr_server->tls_connected || quit))
        {
            if (!quit)
            {
                tls_disconnected++;
                weechat_printf (
                    ptr_server->buffer,
                    _("%s%s: disconnecting from server because upgrade can't "
                      "work for servers connected via TLS"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME);
            }
            /* send QUIT to server, then disconnect */
            irc_command_quit_server (ptr_server, NULL);
            irc_server_disconnect (ptr_server, 0, 0);
            /*
             * schedule reconnection: WeeChat will reconnect to this server
             * after restart
             */
            ptr_server->index_current_address = 0;
            ptr_server->reconnect_delay = IRC_SERVER_OPTION_INTEGER(
                ptr_server, IRC_SERVER_OPTION_AUTORECONNECT_DELAY);
            ptr_server->reconnect_start = time (NULL) -
                ptr_server->reconnect_delay - 1;
        }
    }
    if (tls_disconnected > 0)
    {
        weechat_printf (
            NULL,
            NG_("%s%s: disconnected from %d server "
                "(TLS connection not supported with upgrade)",
                "%s%s: disconnected from %d servers "
                "(TLS connection not supported with upgrade)",
                tls_disconnected),
            weechat_prefix ("error"),
            IRC_PLUGIN_NAME,
            tls_disconnected);
    }

    if (!irc_upgrade_save (0))
    {
        weechat_printf (
            NULL,
            _("%s%s: failed to save upgrade data"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME);
        return WEECHAT_RC_ERROR;
    }

    return WEECHAT_RC_OK;
}

/*
 * Initializes IRC plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    int i, auto_connect;
    char *info_auto_connect;

    weechat_plugin = plugin;

    irc_signal_quit_received = 0;

    irc_color_init ();

    if (!irc_config_init ())
        return WEECHAT_RC_ERROR;

    irc_config_read ();

    irc_list_init ();

    irc_raw_init ();

    irc_command_init ();

    irc_info_init ();

    irc_redirect_init ();

    irc_notify_init ();

    /* hook some signals */
    irc_debug_init ();
    weechat_hook_signal ("quit",
                         &irc_signal_quit_cb, NULL, NULL);
    weechat_hook_signal ("upgrade",
                         &irc_signal_upgrade_cb, NULL, NULL);
    weechat_hook_signal ("xfer_send_ready",
                         &irc_server_xfer_send_ready_cb, NULL, NULL);
    weechat_hook_signal ("xfer_resume_ready",
                         &irc_server_xfer_resume_ready_cb, NULL, NULL);
    weechat_hook_signal ("xfer_send_accept_resume",
                         &irc_server_xfer_send_accept_resume_cb, NULL, NULL);
    weechat_hook_signal ("irc_input_send",
                         &irc_input_send_cb, NULL, NULL);
    weechat_hook_signal ("typing_self_*",
                         &irc_typing_signal_typing_self_cb, NULL, NULL);
    weechat_hook_signal ("window_scrolled",
                         &irc_list_window_scrolled_cb, NULL, NULL);

    /* hook hsignals for redirection */
    weechat_hook_hsignal ("irc_redirect_pattern",
                          &irc_redirect_pattern_hsignal_cb, NULL, NULL);
    weechat_hook_hsignal ("irc_redirect_command",
                          &irc_redirect_command_hsignal_cb, NULL, NULL);
    weechat_hook_hsignal ("irc_redirection_server_*_list",
                          &irc_list_hsignal_redirect_list_cb, NULL, NULL);

    /* modifiers */
    weechat_hook_modifier ("irc_color_decode",
                           &irc_color_modifier_cb, NULL, NULL);
    weechat_hook_modifier ("irc_color_encode",
                           &irc_color_modifier_cb, NULL, NULL);
    weechat_hook_modifier ("irc_color_decode_ansi",
                           &irc_color_modifier_cb, NULL, NULL);
    weechat_hook_modifier ("irc_tag_escape_value",
                           &irc_tag_modifier_cb, NULL, NULL);
    weechat_hook_modifier ("irc_tag_unescape_value",
                           &irc_tag_modifier_cb, NULL, NULL);
    weechat_hook_modifier ("irc_batch",
                           &irc_batch_modifier_cb, NULL, NULL);

    /* hook completions */
    irc_completion_init ();

    irc_bar_item_init ();

    /* check if auto-connect is enabled */
    info_auto_connect = weechat_info_get ("auto_connect", NULL);
    auto_connect = (info_auto_connect && (strcmp (info_auto_connect, "1") == 0)) ?
        1 : 0;
    free (info_auto_connect);

    /* look at arguments */
    for (i = 0; i < argc; i++)
    {
        if ((weechat_strncmp (argv[i], IRC_PLUGIN_NAME, 3) == 0))
        {
            if (!irc_server_alloc_with_url (argv[i]))
            {
                weechat_printf (
                    NULL,
                    _("%s%s: unable to add temporary server \"%s\" (check "
                      "if there is already a server with this name)"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, argv[i]);
            }
        }
    }

    if (weechat_irc_plugin->upgrading)
    {
        if (!irc_upgrade_load ())
        {
            weechat_printf (
                NULL,
                _("%s%s: WARNING: some network connections may still be "
                  "opened and not visible, you should restart WeeChat now "
                  "(with /quit)."),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
        }
    }
    else
    {
        irc_server_auto_connect (auto_connect);
    }

    irc_hook_timer = weechat_hook_timer (1 * 1000, 0, 0,
                                         &irc_server_timer_cb,
                                         NULL, NULL);

    return WEECHAT_RC_OK;
}

/*
 * Ends IRC plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    if (irc_hook_timer)
    {
        weechat_unhook (irc_hook_timer);
        irc_hook_timer = NULL;
    }

    if (weechat_irc_plugin->unload_with_upgrade)
    {
        irc_config_write (1);
    }
    else
    {
        irc_config_write (0);
        irc_server_disconnect_all ();
    }

    irc_ignore_free_all ();

    irc_list_end ();

    irc_raw_end ();

    irc_server_free_all ();

    irc_config_free ();

    irc_notify_end ();

    irc_redirect_end ();

    irc_color_end ();

    return WEECHAT_RC_OK;
}
