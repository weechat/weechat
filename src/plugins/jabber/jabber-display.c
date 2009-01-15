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

/* jabber-display.c: display functions for Jabber plugin */


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "jabber.h"
#include "jabber-command.h"
#include "jabber-config.h"
#include "jabber-server.h"


/*
 * jabber_display_server: display server infos
 */

void
jabber_display_server (struct t_jabber_server *server, int with_detail)
{
    int num_mucs, num_pv;

    if (with_detail)
    {
        weechat_printf (NULL, "");
        weechat_printf (NULL, _("%sServer: %s%s %s[%s%s%s]%s%s"),
                        JABBER_COLOR_CHAT,
                        JABBER_COLOR_CHAT_SERVER,
                        server->name,
                        JABBER_COLOR_CHAT_DELIMITERS,
                        JABBER_COLOR_CHAT,
                        (server->is_connected) ?
                        _("connected") : _("not connected"),
                        JABBER_COLOR_CHAT_DELIMITERS,
                        JABBER_COLOR_CHAT,
                        (server->temp_server) ? _(" (temporary)") : "");
        
        if (weechat_config_option_is_null (server->options[JABBER_SERVER_OPTION_USERNAME]))
            weechat_printf (NULL, "  username . . . . . . :   ('%s')",
                            JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_USERNAME));
        else
            weechat_printf (NULL, "  username . . . . . . : %s'%s'",
                            JABBER_COLOR_CHAT_HOST,
                            weechat_config_string (server->options[JABBER_SERVER_OPTION_USERNAME]));
        if (weechat_config_option_is_null (server->options[JABBER_SERVER_OPTION_SERVER]))
            weechat_printf (NULL, "  server . . . . . . . :   ('%s')",
                            JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_SERVER));
        else
            weechat_printf (NULL, "  server . . . . . . . : %s'%s'",
                            JABBER_COLOR_CHAT_HOST,
                            weechat_config_string (server->options[JABBER_SERVER_OPTION_SERVER]));
        if (weechat_config_option_is_null (server->options[JABBER_SERVER_OPTION_PROXY]))
            weechat_printf (NULL, "  proxy. . . . . . . . :   ('%s')",
                            JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_PROXY));
        else
            weechat_printf (NULL, "  proxy. . . . . . . . : %s'%s'",
                            JABBER_COLOR_CHAT_HOST,
                            weechat_config_string (server->options[JABBER_SERVER_OPTION_PROXY]));
        if (weechat_config_option_is_null (server->options[JABBER_SERVER_OPTION_IPV6]))
            weechat_printf (NULL, "  ipv6 . . . . . . . . :   (%s)",
                            (JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_IPV6)) ?
                            _("on") : _("off"));
        else
            weechat_printf (NULL, "  ipv6 . . . . . . . . : %s%s",
                            JABBER_COLOR_CHAT_HOST,
                            weechat_config_boolean (server->options[JABBER_SERVER_OPTION_IPV6]) ?
                            _("on") : _("off"));
        if (weechat_config_option_is_null (server->options[JABBER_SERVER_OPTION_TLS]))
            weechat_printf (NULL, "  tls. . . . . . . . . :   (%s)",
                            (JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_TLS)) ?
                            _("on") : _("off"));
        else
            weechat_printf (NULL, "  tls. . . . . . . . . : %s%s",
                            JABBER_COLOR_CHAT_HOST,
                            weechat_config_boolean (server->options[JABBER_SERVER_OPTION_TLS]) ?
                            _("on") : _("off"));
        if (weechat_config_option_is_null (server->options[JABBER_SERVER_OPTION_SASL]))
            weechat_printf (NULL, "  sasl . . . . . . . . :   (%s)",
                            (JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_SASL)) ?
                            _("on") : _("off"));
        else
            weechat_printf (NULL, "  sasl . . . . . . . . : %s%s",
                            JABBER_COLOR_CHAT_HOST,
                            weechat_config_boolean (server->options[JABBER_SERVER_OPTION_SASL]) ?
                            _("on") : _("off"));
        if (weechat_config_option_is_null (server->options[JABBER_SERVER_OPTION_RESOURCE]))
            weechat_printf (NULL, "  resource . . . . . . :   ('%s')",
                            JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_RESOURCE));
        else
            weechat_printf (NULL, "  resource . . . . . . : %s'%s'",
                            JABBER_COLOR_CHAT_HOST,
                            weechat_config_string (server->options[JABBER_SERVER_OPTION_RESOURCE]));
        if (weechat_config_option_is_null (server->options[JABBER_SERVER_OPTION_PASSWORD]))
            weechat_printf (NULL, "  password . . . . . . :   %s",
                            _("(hidden)"));
        else
            weechat_printf (NULL, "  password . . . . . . : %s%s",
                            JABBER_COLOR_CHAT_HOST,
                            _("(hidden)"));
        if (weechat_config_option_is_null (server->options[JABBER_SERVER_OPTION_LOCAL_ALIAS]))
            weechat_printf (NULL, "  local_alias. . . . . :   ('%s')",
                            JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_LOCAL_ALIAS));
        else
            weechat_printf (NULL, "  local_alias. . . . . : %s'%s'",
                            JABBER_COLOR_CHAT_HOST,
                            weechat_config_string (server->options[JABBER_SERVER_OPTION_LOCAL_ALIAS]));
        if (weechat_config_option_is_null (server->options[JABBER_SERVER_OPTION_AUTOCONNECT]))
            weechat_printf (NULL, "  autoconnect. . . . . :   (%s)",
                            (JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_AUTOCONNECT)) ?
                            _("on") : _("off"));
        else
            weechat_printf (NULL, "  autoconnect. . . . . : %s%s",
                            JABBER_COLOR_CHAT_HOST,
                            weechat_config_boolean (server->options[JABBER_SERVER_OPTION_AUTOCONNECT]) ?
                            _("on") : _("off"));
        if (weechat_config_option_is_null (server->options[JABBER_SERVER_OPTION_AUTORECONNECT]))
            weechat_printf (NULL, "  autoreconnect. . . . :   (%s)",
                            (JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_AUTORECONNECT)) ?
                            _("on") : _("off"));
        else
            weechat_printf (NULL, "  autoreconnect. . . . : %s%s",
                            JABBER_COLOR_CHAT_HOST,
                            weechat_config_boolean (server->options[JABBER_SERVER_OPTION_AUTORECONNECT]) ?
                            _("on") : _("off"));
        if (weechat_config_option_is_null (server->options[JABBER_SERVER_OPTION_AUTORECONNECT_DELAY]))
            weechat_printf (NULL, "  autoreconnect_delay. :   (%d %s)",
                            JABBER_SERVER_OPTION_INTEGER(server, JABBER_SERVER_OPTION_AUTORECONNECT_DELAY),
                            NG_("second", "seconds", JABBER_SERVER_OPTION_INTEGER(server, JABBER_SERVER_OPTION_AUTORECONNECT_DELAY)));
        else
            weechat_printf (NULL, "  autoreconnect_delay. : %s%d %s",
                            JABBER_COLOR_CHAT_HOST,
                            weechat_config_integer (server->options[JABBER_SERVER_OPTION_AUTORECONNECT_DELAY]),
                            NG_("second", "seconds", weechat_config_integer (server->options[JABBER_SERVER_OPTION_AUTORECONNECT_DELAY])));
        if (weechat_config_option_is_null (server->options[JABBER_SERVER_OPTION_LOCAL_HOSTNAME]))
            weechat_printf (NULL, "  local_hostname . . . :   ('%s')",
                            JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_LOCAL_HOSTNAME));
        else
            weechat_printf (NULL, "  local_hostname . . . : %s'%s'",
                            JABBER_COLOR_CHAT_HOST,
                            weechat_config_string (server->options[JABBER_SERVER_OPTION_LOCAL_HOSTNAME]));
        if (weechat_config_option_is_null (server->options[JABBER_SERVER_OPTION_COMMAND]))
            weechat_printf (NULL, "  command. . . . . . . :   ('%s')",
                            JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_COMMAND));
        else
            weechat_printf (NULL, "  command. . . . . . . : %s'%s'",
                            JABBER_COLOR_CHAT_HOST,
                            weechat_config_string (server->options[JABBER_SERVER_OPTION_COMMAND]));
        if (weechat_config_option_is_null (server->options[JABBER_SERVER_OPTION_COMMAND_DELAY]))
            weechat_printf (NULL, "  command_delay. . . . :   (%d %s)",
                            JABBER_SERVER_OPTION_INTEGER(server, JABBER_SERVER_OPTION_COMMAND_DELAY),
                            NG_("second", "seconds", JABBER_SERVER_OPTION_INTEGER(server, JABBER_SERVER_OPTION_COMMAND_DELAY)));
        else
            weechat_printf (NULL, "  command_delay. . . . : %s%d %s",
                            JABBER_COLOR_CHAT_HOST,
                            weechat_config_integer (server->options[JABBER_SERVER_OPTION_COMMAND_DELAY]),
                            NG_("second", "seconds", weechat_config_integer (server->options[JABBER_SERVER_OPTION_COMMAND_DELAY])));
        if (weechat_config_option_is_null (server->options[JABBER_SERVER_OPTION_AUTOJOIN]))
            weechat_printf (NULL, "  autojoin . . . . . . :   ('%s')",
                            JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_AUTOJOIN));
        else
            weechat_printf (NULL, "  autojoin . . . . . . : %s'%s'",
                            JABBER_COLOR_CHAT_HOST,
                            weechat_config_string (server->options[JABBER_SERVER_OPTION_AUTOJOIN]));
        if (weechat_config_option_is_null (server->options[JABBER_SERVER_OPTION_AUTOREJOIN]))
            weechat_printf (NULL, "  autorejoin . . . . . :   (%s)",
                            (JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_AUTOREJOIN)) ?
                            _("on") : _("off"));
        else
            weechat_printf (NULL, "  autorejoin . . . . . : %s%s",
                            JABBER_COLOR_CHAT_HOST,
                            weechat_config_boolean (server->options[JABBER_SERVER_OPTION_AUTOREJOIN]) ?
                            _("on") : _("off"));
    }
    else
    {
        if (server->is_connected)
        {
            num_mucs = jabber_server_get_muc_count (server);
            num_pv = jabber_server_get_pv_count (server);
            weechat_printf (NULL, " %s %s%s %s[%s%s%s]%s%s, %d %s, %d pv",
                            (server->is_connected) ? "*" : " ",
                            JABBER_COLOR_CHAT_SERVER,
                            server->name,
                            JABBER_COLOR_CHAT_DELIMITERS,
                            JABBER_COLOR_CHAT,
                            (server->is_connected) ?
                            _("connected") : _("not connected"),
                            JABBER_COLOR_CHAT_DELIMITERS,
                            JABBER_COLOR_CHAT,
                            (server->temp_server) ? _(" (temporary)") : "",
                            num_mucs,
                            NG_("MUC", "MUCs", num_mucs),
                            num_pv);
        }
        else
        {
            weechat_printf (NULL, "   %s%s%s%s",
                            JABBER_COLOR_CHAT_SERVER,
                            server->name,
                            JABBER_COLOR_CHAT,
                            (server->temp_server) ? _(" (temporary)") : "");
        }
    }
}
