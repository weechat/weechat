/*
 * Copyright (c) 2003 by FlashCode <flashcode@flashtux.org>
 *                       Bounga <bounga@altern.org>
 *                       Xahlexx <xahlexx@tuxisland.org>
 * See README for License detail.
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


/* irc-send.c: implementation of IRC commands (client to server),
               according to RFC 1459,2810,2811,2812 */


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/utsname.h>

#include "../weechat.h"
#include "irc.h"
#include "../command.h"
#include "../config.h"
#include "../gui/gui.h"


/*
 * irc_login: login to irc server
 */

void
irc_login (t_irc_server *server)
{
    char hostname[128];

    if ((server->password) && (server->password[0]))
        server_sendf (server, "PASS %s\r\n", server->password);
    
    gethostname (hostname, sizeof (hostname) - 1);
    hostname[sizeof (hostname) - 1] = '\0';
    if (!hostname[0])
        strcpy (hostname, _("unknown"));
    gui_printf (server->window,
                _("%s: using local hostname \"%s\"\n"),
                WEECHAT_NAME, hostname);
    server_sendf (server,
                  "NICK %s\r\n"
                  "USER %s %s %s :%s\r\n",
                  server->nick, server->username, hostname, "servername",
                  server->realname);
}

/*
 * irc_cmd_send_away: toggle away status
 */

int
irc_cmd_send_away (t_irc_server *server, char *arguments)
{
    char *pos;
    t_irc_server *ptr_server;
    
    if (arguments && (strncmp (arguments, "-all", 4) == 0))
    {
        pos = arguments + 4;
        while (pos[0] == ' ')
            pos++;
        if (!pos[0])
            pos = NULL;
        
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (server->is_connected)
            {
                if (pos)
                    server_sendf (ptr_server, "AWAY :%s\r\n", pos);
                else
                    server_sendf (ptr_server, "AWAY\r\n");
            }
        }
    }
    else
    {
        if (arguments)
            server_sendf (server, "AWAY :%s\r\n", arguments);
        else
            server_sendf (server, "AWAY\r\n");
    }
    return 0;
}

/*
 * irc_cmd_send_ctcp: send a ctcp message
 */

int
irc_cmd_send_ctcp (t_irc_server *server, char *arguments)
{
    char *pos, *pos2;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
        pos2 = strchr (pos, ' ');
        if (pos2)
        {
            pos2[0] = '\0';
            pos2++;
            while (pos2[0] == ' ')
                pos2++;
        }
        else
            pos2 = NULL;
        
        if (strcasecmp (pos, "version") == 0)
        {
            if (pos2)
                server_sendf (server, "PRIVMSG %s :\01VERSION %s\01\r\n",
                              arguments, pos2);
            else
                server_sendf (server, "PRIVMSG %s :\01VERSION\01\r\n",
                              arguments);
        }
        if (strcasecmp (pos, "action") == 0)
        {
            if (pos2)
                server_sendf (server, "PRIVMSG %s :\01ACTION %s\01\r\n",
                              arguments, pos2);
            else
                server_sendf (server, "PRIVMSG %s :\01ACTION\01\r\n",
                              arguments);
        }
        
    }
    return 0;
}

/*
 * irc_cmd_send_deop: remove operator privileges from nickname(s)
 */

int
irc_cmd_send_deop (t_irc_server *server, int argc, char **argv)
{
    int i;
    
    if (WIN_IS_CHANNEL(gui_current_window))
    {
        for (i = 0; i < argc; i++)
            server_sendf (server, "MODE %s -o %s\r\n",
                          CHANNEL(gui_current_window)->name,
                          argv[i]);
    }
    else
        gui_printf (server->window,
                    _("%s \"%s\" command can only be executed in a channel window\n"),
                    WEECHAT_ERROR, "deop");
    return 0;
}

/*
 * irc_cmd_send_devoice: remove voice from nickname(s)
 */

int
irc_cmd_send_devoice (t_irc_server *server, int argc, char **argv)
{
    int i;
    
    if (WIN_IS_CHANNEL(gui_current_window))
    {
        for (i = 0; i < argc; i++)
            server_sendf (server, "MODE %s -v %s\r\n",
                          CHANNEL(gui_current_window)->name,
                          argv[i]);
    }
    else
    {
        gui_printf (server->window,
                    _("%s \"%s\" command can only be executed in a channel window\n"),
                    WEECHAT_ERROR, "devoice");
        return -1;
    }
    return 0;
}

/*
 * irc_cmd_send_invite: invite a nick on a channel
 */

int
irc_cmd_send_invite (t_irc_server *server, char *arguments)
{
    server_sendf (server, "INVITE %s\r\n", arguments);
    return 0;
}

/*
 * irc_cmd_send_join: join a new channel
 */

int
irc_cmd_send_join (t_irc_server *server, char *arguments)
{
    server_sendf (server, "JOIN %s\r\n", arguments);
    return 0;
}

/*
 * irc_cmd_send_kick: forcibly remove a user from a channel
 */

int
irc_cmd_send_kick (t_irc_server *server, char *arguments)
{
    if (string_is_channel (arguments))
        server_sendf (server, "KICK %s\r\n", arguments);
    else
    {
        if (WIN_IS_CHANNEL (gui_current_window))
        {
            server_sendf (server,
                          "KICK %s %s\r\n",
                          CHANNEL(gui_current_window)->name, arguments);
        }
        else
        {
            gui_printf (server->window,
                        _("%s \"%s\" command can only be executed in a channel window\n"),
                        WEECHAT_ERROR, "kick");
            return -1;
        }
    }
    return 0;
}

/*
 * irc_cmd_send_kill: close client-server connection
 */

int
irc_cmd_send_kill (t_irc_server *server, char *arguments)
{
    server_sendf (server, "KILL %s\r\n", arguments);
    return 0;
}

/*
 * irc_cmd_send_list: close client-server connection
 */

int
irc_cmd_send_list (t_irc_server *server, char *arguments)
{
    if (arguments)
        server_sendf (server, "LIST %s\r\n", arguments);
    else
        server_sendf (server, "LIST\r\n");
    return 0;
}

/*
 * irc_cmd_send_me: send a ctcp action to the current channel
 */

int
irc_cmd_send_me (t_irc_server *server, char *arguments)
{
    if (WIN_IS_SERVER(gui_current_window))
    {
        gui_printf (server->window,
                    _("%s \"%s\" command can not be executed on a server window\n"),
                    WEECHAT_ERROR, "me");
        return -1;
    }
    server_sendf (server, "PRIVMSG %s :\01ACTION %s\01\r\n",
                  CHANNEL(gui_current_window)->name, arguments);
    irc_display_prefix (gui_current_window, PREFIX_ACTION_ME);
    gui_printf_color (gui_current_window,
                      COLOR_WIN_CHAT_NICK, "%s", server->nick);
    gui_printf_color (gui_current_window,
                      COLOR_WIN_CHAT, " %s\n", arguments);
    return 0;
}

/*
 * irc_cmd_send_mode: change mode for channel/nickname
 */

int
irc_cmd_send_mode (t_irc_server *server, char *arguments)
{
    server_sendf (server, "MODE %s\r\n", arguments);
    return 0;
}

/*
 * irc_cmd_send_msg: send a message to a nick or channel
 */

int
irc_cmd_send_msg (t_irc_server *server, char *arguments)
{
    char *pos, *pos_comma;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
        
        while (arguments && arguments[0])
        {
            pos_comma = strchr (arguments, ',');
            if (pos_comma)
            {
                pos_comma[0] = '\0';
                pos_comma++;
            }
            if (strcmp (arguments, "*") == 0)
            {
                if (WIN_IS_SERVER(gui_current_window))
                {
                    gui_printf (server->window,
                                _("%s \"%s\" command can not be executed on a server window\n"),
                                WEECHAT_ERROR, "msg *");
                    return -1;
                }
                ptr_channel = CHANNEL(gui_current_window);
                ptr_nick = nick_search (ptr_channel, server->nick);
                if (ptr_nick)
                {
                    irc_display_nick (ptr_channel->window, ptr_nick,
                                      MSG_TYPE_NICK, 1, 1, 0);
                    gui_printf_color_type (ptr_channel->window,
                                           MSG_TYPE_MSG,
                                           COLOR_WIN_CHAT, "%s\n", pos);
                }
                else
                    gui_printf (server->window,
                                _("%s nick not found for \"%s\" command\n"),
                                WEECHAT_ERROR, "msg");
                server_sendf (server, "PRIVMSG %s :%s\r\n", ptr_channel->name, pos);
            }
            else
            {
                if (string_is_channel (arguments))
                {
                    ptr_channel = channel_search (server, arguments);
                    if (ptr_channel)
                    {
                        ptr_nick = nick_search (ptr_channel, server->nick);
                        if (ptr_nick)
                        {
                            irc_display_nick (ptr_channel->window, ptr_nick,
                                              MSG_TYPE_NICK, 1, 1, 0);
                            gui_printf_color_type (ptr_channel->window,
                                                   MSG_TYPE_MSG,
                                                   COLOR_WIN_CHAT, "%s\n", pos);
                        }
                        else
                            gui_printf (server->window,
                                        _("%s nick not found for \"%s\" command\n"),
                                        WEECHAT_ERROR, "msg");
                    }
                    server_sendf (server, "PRIVMSG %s :%s\r\n", arguments, pos);
                }
                else
                {
                    ptr_channel = channel_search (server, arguments);
                    if (!ptr_channel)
                    {
                        ptr_channel = channel_new (server, CHAT_PRIVATE, arguments);
                        if (!ptr_channel)
                        {
                            gui_printf (server->window,
                                        _("%s cannot create new private window \"%s\"\n"),
                                        WEECHAT_ERROR,
                                        arguments);
                            return -1;
                        }
                        gui_redraw_window_title (ptr_channel->window);
                    }
                        
                    gui_printf_color_type (ptr_channel->window,
                                           MSG_TYPE_NICK,
                                           COLOR_WIN_CHAT_DARK, "<");
                    gui_printf_color_type (ptr_channel->window,
                                           MSG_TYPE_NICK,
                                           COLOR_WIN_NICK_SELF,
                                           "%s", server->nick);
                    gui_printf_color_type (ptr_channel->window,
                                           MSG_TYPE_NICK,
                                           COLOR_WIN_CHAT_DARK, "> ");
                    gui_printf_color_type (ptr_channel->window,
                                           MSG_TYPE_MSG,
                                           COLOR_WIN_CHAT, "%s\n", pos);
                    server_sendf (server, "PRIVMSG %s :%s\r\n", arguments, pos);
                }
            }
            arguments = pos_comma;
        }
    }
    else
    {
        gui_printf (server->window,
                    _("%s wrong argument count for \"%s\" command\n"),
                    WEECHAT_ERROR, "msg");
        return -1;
    }
    return 0;
}

/*
 * irc_cmd_send_names: list nicknames on channels
 */

int
irc_cmd_send_names (t_irc_server *server, char *arguments)
{
    if (arguments)
        server_sendf (server, "NAMES %s\r\n", arguments);
    else
    {
        if (!WIN_IS_CHANNEL(gui_current_window))
        {
            gui_printf (server->window,
                        _("%s \"%s\" command can only be executed in a channel window\n"),
                        WEECHAT_ERROR, "names");
            return -1;
        }
        else
            server_sendf (server, "NAMES %s\r\n",
                          CHANNEL(gui_current_window)->name);
    }
    return 0;
}

/*
 * irc_cmd_send_nick: change nickname
 */

int
irc_cmd_send_nick (t_irc_server *server, int argc, char **argv)
{
    if (argc != 1)
        return -1;
    server_sendf (server, "NICK %s\r\n", argv[0]);
    return 0;
}

/*
 * irc_cmd_send_notice: send notice message
 */

int
irc_cmd_send_notice (t_irc_server *server, char *arguments)
{
    server_sendf (server, "NOTICE %s\r\n", arguments);
    return 0;
}

/*
 * irc_cmd_send_op: give operator privileges to nickname(s)
 */

int
irc_cmd_send_op (t_irc_server *server, int argc, char **argv)
{
    int i;
    
    if (WIN_IS_CHANNEL(gui_current_window))
    {
        for (i = 0; i < argc; i++)
            server_sendf (server, "MODE %s +o %s\r\n",
                          CHANNEL(gui_current_window)->name,
                          argv[i]);
    }
    else
    {
        gui_printf (server->window,
                    _("%s \"%s\" command can only be executed in a channel window\n"),
                    WEECHAT_ERROR, "op");
        return -1;
    }
    return 0;
}

/*
 * irc_cmd_send_oper: get oper privileges
 */

int
irc_cmd_send_oper (t_irc_server *server, int argc, char **argv)
{
    if (argc != 2)
        return -1;
    server_sendf (server, "OPER %s %s\r\n", argv[0], argv[1]);
    return 0;
}

/*
 * irc_cmd_send_part: leave a channel or close a private window
 */

int
irc_cmd_send_part (t_irc_server *server, char *arguments)
{
    char *channel_name, *pos_args;
    t_irc_channel *ptr_channel;
    
    if (arguments)
    {
        if (string_is_channel (arguments))
        {
            channel_name = arguments;
            pos_args = strchr (arguments, ' ');
            if (pos_args)
            {
                pos_args[0] = '\0';
                pos_args++;
                while (pos_args[0] == ' ')
                    pos_args++;
            }
        }
        else
        {
            if (WIN_IS_SERVER(gui_current_window))
            {
                gui_printf (server->window,
                            _("%s \"%s\" command can not be executed on a server window\n"),
                            WEECHAT_ERROR, "part");
                return -1;
            }
            channel_name = CHANNEL(gui_current_window)->name;
            pos_args = arguments;
        }
    }
    else
    {
        if (WIN_IS_SERVER(gui_current_window))
        {
            gui_printf (server->window,
                        _("%s \"%s\" command can not be executed on a server window\n"),
                        WEECHAT_ERROR, "part");
            return -1;
        }
        if (WIN_IS_PRIVATE(gui_current_window))
        {
            ptr_channel = CHANNEL(gui_current_window);
            gui_window_free (ptr_channel->window);
            channel_free (server, ptr_channel);
            gui_redraw_window_status (gui_current_window);
            gui_redraw_window_input (gui_current_window);
            return 0;
        }
        channel_name = CHANNEL(gui_current_window)->name;
        pos_args = NULL;
    }
    
    if (pos_args)
        server_sendf (server, "PART %s :%s\r\n", channel_name, pos_args);
    else
        server_sendf (server, "PART %s\r\n", channel_name);
    return 0;
}

/*
 * irc_cmd_send_ping: ping a server
 */

int
irc_cmd_send_ping (t_irc_server *server, int argc, char **argv)
{
    if (argc == 1)
        server_sendf (server, "PING %s\r\n", argv[0]);
    if (argc == 2)
        server_sendf (server, "PING %s %s\r\n", argv[0],
                      argv[1]);
    return 0;
}

/*
 * irc_cmd_send_pong: send pong answer to a daemon
 */

int
irc_cmd_send_pong (t_irc_server *server, int argc, char **argv)
{
    if (argc == 1)
        server_sendf (server, "PONG %s\r\n", argv[0]);
    if (argc == 2)
        server_sendf (server, "PONG %s %s\r\n", argv[0],
                      argv[1]);
    return 0;
}

/*
 * irc_cmd_send_quit: disconnect from all servers and quit WeeChat
 */

int
irc_cmd_send_quit (t_irc_server *server, char *arguments)
{
    if (server && server->is_connected)
    {
        if (arguments)
            server_sendf (server, "QUIT :%s\r\n", arguments);
        else
            server_sendf (server, "QUIT\r\n");
    }
    quit_weechat = 1;
    return 0;
}

/*
 * irc_cmd_send_quote: send raw data to server
 */

int
irc_cmd_send_quote (t_irc_server *server, char *arguments)
{
    server_sendf (server, "%s\r\n", arguments);
    return 0;
}

/*
 * irc_cmd_send_rehash: tell the server to reload its config file
 */

int
irc_cmd_send_rehash (t_irc_server *server, char *arguments)
{
    /* make gcc happy */
    (void) arguments;
    
    server_sendf (server, "REHASH\r\n");
    return 0;
}

/*
 * irc_cmd_send_restart: tell the server to restart itself
 */

int
irc_cmd_send_restart (t_irc_server *server, char *arguments)
{
    /* make gcc happy */
    (void) arguments;
    
    server_sendf (server, "RESTART\r\n");
    return 0;
}

/*
 * irc_cmd_send_stats: query statistics about server
 */

int
irc_cmd_send_stats (t_irc_server *server, char *arguments)
{
    if (arguments)
        server_sendf (server, "STATS %s\r\n", arguments);
    else
        server_sendf (server, "STATS\r\n");
    return 0;
}

/*
 * irc_cmd_send_topic: get/set topic for a channel
 */

int
irc_cmd_send_topic (t_irc_server *server, char *arguments)
{
    char *channel_name, *new_topic, *pos;
    
    channel_name = NULL;
    new_topic = NULL;
    
    if (arguments)
    {
        if (string_is_channel (arguments))
        {
            channel_name = arguments;
            pos = strchr (arguments, ' ');
            if (pos)
            {
                pos[0] = '\0';
                pos++;
                while (pos[0] == ' ')
                    pos++;
                new_topic = (pos[0]) ? pos : NULL;
            }
        }
        else
            new_topic = arguments;
    }
    
    /* look for current channel if not specified */
    if (!channel_name)
    {
        if (WIN_IS_SERVER(gui_current_window))
        {
            gui_printf (server->window,
                        _("%s \"%s\" command can not be executed on a server window\n"),
                        WEECHAT_ERROR, "topic");
            return -1;
        }
        channel_name = CHANNEL(gui_current_window)->name;
    }
    
    if (new_topic)
    {
        if (strcmp (new_topic, "-delete") == 0)
            server_sendf (server, "TOPIC %s :\r\n", channel_name);
        else
            server_sendf (server, "TOPIC %s :%s\r\n", channel_name, new_topic);
    }
    else
        server_sendf (server, "TOPIC %s\r\n", channel_name);
    return 0;
}

/*
 * irc_cmd_send_version: gives the version info of nick or server (current or specified)
 */

int
irc_cmd_send_version (t_irc_server *server, char *arguments)
{
    if (arguments)
    {
        if (WIN_IS_CHANNEL(gui_current_window) &&
            nick_search (CHANNEL(gui_current_window), arguments))
            server_sendf (server, "PRIVMSG %s :\01VERSION\01\r\n",
                          arguments);
        else
            server_sendf (server, "VERSION %s\r\n",
                          arguments);
    }
    else
    {
        irc_display_prefix (server->window, PREFIX_INFO);
        gui_printf (server->window, _("%s, compiled on %s %s\n"),
                    WEECHAT_NAME_AND_VERSION,
                    __DATE__, __TIME__);
        server_sendf (server, "VERSION\r\n");
    }
    return 0;
}

/*
 * irc_cmd_send_voice: give voice to nickname(s)
 */

int
irc_cmd_send_voice (t_irc_server *server, int argc, char **argv)
{
    int i;
    
    if (WIN_IS_CHANNEL(gui_current_window))
    {
        for (i = 0; i < argc; i++)
            server_sendf (server, "MODE %s +v %s\r\n",
                          CHANNEL(gui_current_window)->name,
                          argv[i]);
    }
    else
    {
        gui_printf (server->window,
                    _("%s \"%s\" command can only be executed in a channel window\n"),
                    WEECHAT_ERROR, "voice");
        return -1;
    }
    return 0;
}

/*
 * irc_cmd_send_whois: query information about user(s)
 */

int
irc_cmd_send_whois (t_irc_server *server, char *arguments)
{
    server_sendf (server, "WHOIS %s\r\n", arguments);
    return 0;
}
