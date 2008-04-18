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

/* irc-display.c: display functions for IRC */


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-channel.h"
#include "irc-command.h"
#include "irc-config.h"
#include "irc-server.h"
#include "irc-nick.h"


/*
 * irc_display_hide_password: hide IRC password(s) in a string
 */

void
irc_display_hide_password (char *string, int look_for_nickserv)
{
    char *pos_nickserv, *pos, *pos_pwd;

    pos = string;
    while (1)
    {
        if (look_for_nickserv)
        {
            pos_nickserv = strstr (pos, "nickserv ");
            if (!pos_nickserv)
                return;
            pos = pos_nickserv + 9;
            while (pos[0] == ' ')
                pos++;
            if ((strncmp (pos, "identify ", 9) == 0)
                || (strncmp (pos, "register ", 9) == 0))
                pos_pwd = pos + 9;
            else
                pos_pwd = NULL;
        }
        else
        {
            pos_pwd = strstr (pos, "identify ");
            if (!pos_pwd)
                pos_pwd = strstr (pos, "register ");
            if (!pos_pwd)
                return;
            pos_pwd += 9;
        }

        if (pos_pwd)
        {
            while (pos_pwd[0] == ' ')
                pos_pwd++;
            
            while (pos_pwd[0] && (pos_pwd[0] != ';') && (pos_pwd[0] != ' ')
                   && (pos_pwd[0] != '"'))
            {
                pos_pwd[0] = '*';
                pos_pwd++;
            }
            pos = pos_pwd;
        }
    }
}

/*
 * irc_display_away: display away on all channels of all servers
 */

void
irc_display_away (struct t_irc_server *server, char *string1, char *string2)
{
    struct t_irc_channel *ptr_channel;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
        {
            weechat_printf (ptr_channel->buffer,
                            "%s[%s%s%s %s: %s%s]",
                            IRC_COLOR_CHAT_DELIMITERS,
                            IRC_COLOR_CHAT_NICK,
                            server->nick,
                            IRC_COLOR_CHAT,
                            string1,
                            string2,
                            IRC_COLOR_CHAT_DELIMITERS);
        }
    }
}

/*
 * irc_display_mode: display IRC message for mode change
 */

void
irc_display_mode (struct t_gui_buffer *buffer,
                  char *channel_name, char *nick_name, char set_flag,
                  char *symbol, char *nick_host, char *message, char *param)
{
    weechat_printf (buffer,
                    "%s[%s%s%s/%s%c%s%s] %s%s %s%s%s%s%s",
                    IRC_COLOR_CHAT_DELIMITERS,
                    (channel_name) ?
                    IRC_COLOR_CHAT_CHANNEL :
                    IRC_COLOR_CHAT_NICK,
                    (channel_name) ? channel_name : nick_name,
                    IRC_COLOR_CHAT,
                    IRC_COLOR_CHAT_CHANNEL,
                    set_flag,
                    symbol,
                    IRC_COLOR_CHAT_DELIMITERS,
                    IRC_COLOR_CHAT_NICK,
                    nick_host,
                    IRC_COLOR_CHAT,
                    message,
                    (param) ? " " : "",
                    (param) ? IRC_COLOR_CHAT_NICK : "",
                    (param) ? param : "");
}

/*
 * irc_display_server: display server description
 */

void
irc_display_server (struct t_irc_server *server, int with_detail)
{
    char *string;
    int num_channels, num_pv;
    
    if (with_detail)
    {
        weechat_printf (NULL, "");
        weechat_printf (NULL, _("%sServer: %s%s %s[%s%s%s]"),
                        IRC_COLOR_CHAT,
                        IRC_COLOR_CHAT_SERVER,
                        server->name,
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_CHAT,
                        (server->is_connected) ?
                        _("connected") : _("not connected"),
                        IRC_COLOR_CHAT_DELIMITERS);
        
        weechat_printf (NULL, "  autoconnect . . . . : %s%s",
                        (server->autoconnect) ? _("on") : _("off"),
                        (server->temp_server) ?
                        _(" (temporary server, will not be saved)") : "");
        weechat_printf (NULL, "  autoreconnect . . . : %s",
                        (server->autoreconnect) ? _("on") : _("off"));
        weechat_printf (NULL, "  autoreconnect_delay : %d %s",
                        server->autoreconnect_delay,
                        NG_("second", "seconds", server->autoreconnect_delay));
        weechat_printf (NULL, "  addresses . . . . . : %s",
                        (server->addresses && server->addresses[0]) ?
                        server->addresses : "");
        weechat_printf (NULL, "  ipv6  . . . . . . . : %s",
                        (server->ipv6) ? _("on") : _("off"));
        weechat_printf (NULL, "  ssl . . . . . . . . : %s",
                        (server->ssl) ? _("on") : _("off"));
        weechat_printf (NULL, "  password  . . . . . : %s",
                        (server->password && server->password[0]) ?
                        _("(hidden)") : "");
        weechat_printf (NULL, "  nicks . . . . . . . : %s",
                        (server->nicks && server->nicks[0]) ?
                        server->nicks : "");
        weechat_printf (NULL, "  username  . . . . . : %s",
                        (server->username && server->username[0]) ?
                        server->username : "");
        weechat_printf (NULL, "  realname  . . . . . : %s",
                        (server->realname && server->realname[0]) ?
                        server->realname : "");
        weechat_printf (NULL, "  hostname  . . . . . : %s",
                        (server->hostname && server->hostname[0]) ?
                        server->hostname : "");
        if (server->command && server->command[0])
            string = strdup (server->command);
        else
            string = NULL;
        if (string)
        {
            if (weechat_config_boolean (irc_config_log_hide_nickserv_pwd))
                irc_display_hide_password (string, 1);
            weechat_printf (NULL, "  command . . . . . . : %s",
                            string);
            free (string);
        }
        else
        {
            weechat_printf (NULL, "  command . . . . . . : %s",
                            (server->command && server->command[0]) ?
                            server->command : "");
        }
        weechat_printf (NULL, "  command_delay . . . : %d %s",
                        server->command_delay,
                        NG_("second", "seconds", server->command_delay));
        weechat_printf (NULL, "  autojoin  . . . . . : %s",
                        (server->autojoin && server->autojoin[0]) ?
                        server->autojoin : "");
        weechat_printf (NULL, "  notify_levels . . . : %s",
                        (server->notify_levels && server->notify_levels[0]) ?
                        server->notify_levels : "");
    }
    else
    {
        if (server->is_connected)
        {
            num_channels = irc_server_get_channel_count (server);
            num_pv = irc_server_get_pv_count (server);
            weechat_printf (NULL, " %s %s%s %s[%s%s%s]%s%s, %d %s, %d pv",
                            (server->is_connected) ? "*" : " ",
                            IRC_COLOR_CHAT_SERVER,
                            server->name,
                            IRC_COLOR_CHAT_DELIMITERS,
                            IRC_COLOR_CHAT,
                            (server->is_connected) ?
                            _("connected") : _("not connected"),
                            IRC_COLOR_CHAT_DELIMITERS,
                            IRC_COLOR_CHAT,
                            (server->temp_server) ? _(" (temporary)") : "",
                            num_channels,
                            NG_("channel", "channels",
                                num_channels),
                            num_pv);
        }
        else
        {
            weechat_printf (NULL, " %s %s%s%s%s",
                            (server->is_connected) ? "*" : " ",
                            IRC_COLOR_CHAT_SERVER,
                            server->name,
                            IRC_COLOR_CHAT,
                            (server->temp_server) ? _(" (temporary)") : "");
        }
    }
}
