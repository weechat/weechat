/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

/* irc-command.c: IRC commands managment */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../../core/weechat.h"
#include "irc.h"
#include "../../core/command.h"
#include "../../core/util.h"
#include "../../core/weechat-config.h"
#include "../../gui/gui.h"


/*
 * irc_command_admin: find information about the administrator of the server
 */

int
irc_command_admin (t_gui_window *window,
                   char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (arguments)
        irc_server_sendf (ptr_server, "ADMIN %s", arguments);
    else
        irc_server_sendf (ptr_server, "ADMIN");
    return 0;
}

/*
 * irc_command_me_channel: send a ctcp action to a channel
 */

int
irc_command_me_channel (t_irc_server *server, t_irc_channel *channel,
                        char *arguments)
{
    char *string;
    
    irc_server_sendf (server, "PRIVMSG %s :\01ACTION %s\01",
                      channel->name,
                      (arguments && arguments[0]) ? arguments : "");
    string = (arguments && arguments[0]) ?
        (char *)irc_color_decode ((unsigned char *)arguments, 1, 0) : NULL;
    gui_chat_printf_action (channel->buffer,
                            "%s%s %s%s\n",
                            GUI_COLOR(GUI_COLOR_CHAT_NICK),
                            server->nick,
                            GUI_COLOR(GUI_COLOR_CHAT),
                            (string) ? string : "");
    if (string)
        free (string);
    return 0;
}

/*
 * irc_command_me_all_channels: send a ctcp action to all channels of a server
 */

int
irc_command_me_all_channels (t_irc_server *server, char *arguments)
{
    t_irc_channel *ptr_channel;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
            irc_command_me_channel (server, ptr_channel, arguments);
    }
    return 0;
}

/*
 * irc_command_mode_nicks: send mode change for many nicks on a channel
 */

void
irc_command_mode_nicks (t_irc_server *server, char *channel,
                        char *set, char *mode, int argc, char **argv)
{
    int i, length;
    char *command;
    
    length = 0;
    for (i = 0; i < argc; i++)
        length += strlen (argv[i]) + 1;
    length += strlen (channel) + (argc * strlen (mode)) + 32;
    command = (char *)malloc (length);
    if (command)
    {
        snprintf (command, length, "MODE %s %s", channel, set);
        for (i = 0; i < argc; i++)
            strcat (command, mode);
        for (i = 0; i < argc; i++)
        {
            strcat (command, " ");
            strcat (command, argv[i]);
        }
        irc_server_sendf (server, "%s", command);
        free (command);
    }
}

/*
 * irc_command_ame: send a ctcp action to all channels of all connected servers
 */

int
irc_command_ame (t_gui_window *window,
                 char *arguments, int argc, char **argv)
{
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    
    /* make C compiler happy */
    (void) window;
    (void) argc;
    (void) argv;
    
    gui_add_hotlist = 0;
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->is_connected)
        {
            for (ptr_channel = ptr_server->channels; ptr_channel;
                 ptr_channel = ptr_channel->next_channel)
            {
                if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                    irc_command_me_channel (ptr_server, ptr_channel, arguments);
            }
        }
    }
    gui_add_hotlist = 1;
    return 0;
}

/*
 * irc_command_amsg: send message to all channels of all connected servers
 */

int
irc_command_amsg (t_gui_window *window,
                  char *arguments, int argc, char **argv)
{
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    char *string;
    
    /* make C compiler happy */
    (void) window;
    (void) argc;
    (void) argv;
    
    if (arguments)
    {
        gui_add_hotlist = 0;
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (ptr_server->is_connected)
            {
                for (ptr_channel = ptr_server->channels; ptr_channel;
                     ptr_channel = ptr_channel->next_channel)
                {
                    if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                    {
                        irc_server_sendf (ptr_server, "PRIVMSG %s :%s",
                                          ptr_channel->name, arguments);
                        ptr_nick = irc_nick_search (ptr_channel,
                                                    ptr_server->nick);
                        if (ptr_nick)
                        {
                            irc_display_nick (ptr_channel->buffer, ptr_nick,
                                              NULL, GUI_MSG_TYPE_NICK, 1,
                                              NULL, 0);
                            string = (char *)irc_color_decode (
                                (unsigned char *)arguments, 1, 0);
                            gui_chat_printf (ptr_channel->buffer, "%s\n",
                                             (string) ? string : arguments);
                            if (string)
                                free (string);
                        }
                        else
                        {
                            gui_chat_printf_error (ptr_server->buffer,
                                                   _("%s cannot find nick for "
                                                     "sending message\n"),
                                                   WEECHAT_ERROR);
                        }
                    }
                }
            }
        }
        gui_add_hotlist = 1;
    }
    else
        return -1;
    return 0;
}

/*
 * irc_command_away_server: toggle away status for one server
 */

void
irc_command_away_server (t_irc_server *server, char *arguments)
{
    char *string, buffer[4096];
    t_gui_window *ptr_window;
    time_t time_now, elapsed;
    
    if (!server)
        return;
    
    if (arguments)
    {
        if (server->away_message)
            free (server->away_message);
        server->away_message = (char *) malloc (strlen (arguments) + 1);
        if (server->away_message)
            strcpy (server->away_message, arguments);

        /* if server is connected, send away command now */
        if (server->is_connected)
        {
            server->is_away = 1;
            server->away_time = time (NULL);
            irc_server_sendf (server, "AWAY :%s", arguments);
            if (irc_cfg_irc_display_away != CFG_IRC_DISPLAY_AWAY_OFF)
            {
                string = (char *)irc_color_decode ((unsigned char *)arguments,
                                                   1, 0);
                if (irc_cfg_irc_display_away == CFG_IRC_DISPLAY_AWAY_LOCAL)
                    irc_display_away (server, "away",
                                      (string) ? string : arguments);
                else
                {
                    snprintf (buffer, sizeof (buffer), "is away: %s",
                              (string) ? string : arguments);
                    irc_command_me_all_channels (server, buffer);
                }
                if (string)
                    free (string);
            }
            irc_server_set_away (server, server->nick, 1);
            for (ptr_window = gui_windows; ptr_window;
                 ptr_window = ptr_window->next_window)
            {
                if (strcmp (ptr_window->buffer->category, server->name) == 0)
                    ptr_window->buffer->last_read_line =
                        ptr_window->buffer->last_line;
            }
        }
        else
        {
            /* server not connected, store away for future usage
               (when connecting to server) */
            string = (char *)irc_color_decode ((unsigned char *)arguments,
                                               1, 0);
            gui_chat_printf_info_nolog (server->buffer,
                                        _("Future away on %s%s%s: %s\n"),
                                        GUI_COLOR(GUI_COLOR_CHAT_SERVER),
                                        server->name,
                                        GUI_COLOR(GUI_COLOR_CHAT),
                                        (string) ? string : arguments);
            if (string)
                free (string);
        }
    }
    else
    {
        if (server->away_message)
        {
            free (server->away_message);
            server->away_message = NULL;
        }
        
        /* if server is connected, send away command now */
        if (server->is_connected)
        {
            irc_server_sendf (server, "AWAY");
            server->is_away = 0;
            if (server->away_time != 0)
            {
                time_now = time (NULL);
                elapsed = (time_now >= server->away_time) ?
                    time_now - server->away_time : 0;
                server->away_time = 0;
                if (irc_cfg_irc_display_away != CFG_IRC_DISPLAY_AWAY_OFF)
                {
                    if (irc_cfg_irc_display_away == CFG_IRC_DISPLAY_AWAY_LOCAL)
                    {
                        snprintf (buffer, sizeof (buffer),
                                  "gone %.2ld:%.2ld:%.2ld",
                                  (long int)(elapsed / 3600),
                                  (long int)((elapsed / 60) % 60),
                                  (long int)(elapsed % 60));
                        irc_display_away (server, "back", buffer);
                    }
                    else
                    {
                        snprintf (buffer, sizeof (buffer),
                                  "is back (gone %.2ld:%.2ld:%.2ld)",
                                  (long int)(elapsed / 3600),
                                  (long int)((elapsed / 60) % 60),
                                  (long int)(elapsed % 60));
                        irc_command_me_all_channels (server, buffer);
                    }
                }
            }
            irc_server_set_away (server, server->nick, 0);
        }
        else
        {
            /* server not connected, remove away message but do not send
               anything */
            gui_chat_printf_info_nolog (server->buffer,
                                        _("Future away on %s%s%s removed.\n"),
                                        GUI_COLOR(GUI_COLOR_CHAT_SERVER),
                                        server->name,
                                        GUI_COLOR(GUI_COLOR_CHAT));
        }
    }
}

/*
 * irc_command_away: toggle away status
 */

int
irc_command_away (t_gui_window *window,
                  char *arguments, int argc, char **argv)
{
    char *pos;
    
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    gui_add_hotlist = 0;
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
            if (ptr_server->is_connected)
                irc_command_away_server (ptr_server, pos);
        }
    }
    else
        irc_command_away_server (ptr_server, arguments);
    
    gui_status_draw (window->buffer, 1);
    gui_add_hotlist = 1;
    return 0;
}

/*
 * irc_command_ban: bans nicks or hosts
 */

int
irc_command_ban (t_gui_window *window,
                 char *arguments, int argc, char **argv)
{
    char *pos_channel, *pos, *pos2;
    
    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (arguments)
    {
        pos_channel = NULL;
        pos = strchr (arguments, ' ');
        if (pos)
        {
            pos[0] = '\0';
            
            if (irc_channel_is_channel (arguments))
            {
                pos_channel = arguments;
                pos++;
                while (pos[0] == ' ')
                    pos++;
            }
            else
            {
                pos[0] = ' ';
                pos = arguments;
            }
        }
        else
            pos = arguments;
        
        /* channel not given, use default buffer */
        if (!pos_channel)
        {
            if (!ptr_channel)
            {
                gui_chat_printf_error_nolog (ptr_server->buffer,
                                             _("%s \"%s\" command can only "
                                               "be executed in a channel "
                                               "buffer\n"),
                                             WEECHAT_ERROR, "ban");
                return -1;
            }
            pos_channel = ptr_channel->name;
        }
        
        /* loop on users */
        while (pos && pos[0])
        {
            pos2 = strchr (pos, ' ');
            if (pos2)
            {
                pos2[0] = '\0';
                pos2++;
                while (pos2[0] == ' ')
                    pos2++;
            }
            irc_server_sendf (ptr_server, "MODE %s +b %s", pos_channel, pos);
            pos = pos2;
        }
    }
    else
    {
        if (!ptr_channel)
        {
            gui_chat_printf_error_nolog (ptr_server->buffer,
                                         _("%s \"%s\" command can only be "
                                           "executed in a channel buffer\n"),
                                         WEECHAT_ERROR, "ban");
            return -1;
        }
        irc_server_sendf (ptr_server, "MODE %s +b", ptr_channel->name);
    }
    
    return 0;
}

/*
 * irc_command_connect_one_server: connect to one server
 *                             return 0 if error, 1 if ok
 */

int
irc_command_connect_one_server (t_irc_server *server, int no_join)
{
    if (!server)
        return 0;
    
    if (server->is_connected)
    {
        gui_chat_printf_error (NULL,
                               _("%s already connected to server "
                                 "\"%s\"!\n"),
                               WEECHAT_ERROR, server->name);
        return 0;
    }
    if (server->child_pid > 0)
    {
        gui_chat_printf_error (NULL,
                               _("%s currently connecting to server "
                                 "\"%s\"!\n"),
                               WEECHAT_ERROR, server->name);
        return 0;
    }
    if (irc_server_connect (server, no_join))
    {
        server->reconnect_start = 0;
        server->reconnect_join = (server->channels) ? 1 : 0;
        gui_status_draw (server->buffer, 1);
    }
    
    /* connect ok */
    return 1;
}

/*
 * irc_command_connect: connect to server(s)
 */

int
irc_command_connect (t_gui_window *window,
                     char *arguments, int argc, char **argv)
{
    t_irc_server server_tmp;
    int i, nb_connect, connect_ok, all_servers, no_join, port, ipv6, ssl;
    char *error;
    long number;
    
    IRC_BUFFER_GET_SERVER(window->buffer);
    
    /* make C compiler happy */
    (void) window;
    (void) arguments;
    
    nb_connect = 0;
    connect_ok = 1;
    port = IRC_SERVER_DEFAULT_PORT;
    ipv6 = 0;
    ssl = 0;
    
    all_servers = 0;
    no_join = 0;
    for (i = 0; i < argc; i++)
    {
        if (weechat_strcasecmp (argv[i], "-all") == 0)
            all_servers = 1;
        if (weechat_strcasecmp (argv[i], "-nojoin") == 0)
            no_join = 1;
        if (weechat_strcasecmp (argv[i], "-ipv6") == 0)
            ipv6 = 1;
        if (weechat_strcasecmp (argv[i], "-ssl") == 0)
            ssl = 1;
        if (weechat_strcasecmp (argv[i], "-port") == 0)
        {
            if (i == (argc - 1))
            {
                gui_chat_printf_error (NULL,
                                       _("%s missing argument for \"%s\" "
                                         "option\n"),
                                       WEECHAT_ERROR, "-port");
                return -1;
            }
            error = NULL;
            number = strtol (argv[++i], &error, 10);
            if (error && (error[0] == '\0'))
                port = number;
        }
    }
    
    if (all_servers)
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            nb_connect++;
            if (!ptr_server->is_connected && (ptr_server->child_pid == 0))
            {
                if (!irc_command_connect_one_server (ptr_server, no_join))
                    connect_ok = 0;
            }
        }
    }
    else
    {
        for (i = 0; i < argc; i++)
        {
            if (argv[i][0] != '-')
            {
                nb_connect++;
                ptr_server = irc_server_search (argv[i]);
                if (ptr_server)
                {
                    if (!irc_command_connect_one_server (ptr_server, no_join))
                        connect_ok = 0;
                }
                else
                {
                    irc_server_init (&server_tmp);
                    server_tmp.name = strdup (argv[i]);
                    server_tmp.address = strdup (argv[i]);
                    server_tmp.port = port;
                    server_tmp.ipv6 = ipv6;
                    server_tmp.ssl = ssl;
                    ptr_server = irc_server_new (server_tmp.name,
                                                 server_tmp.autoconnect,
                                                 server_tmp.autoreconnect,
                                                 server_tmp.autoreconnect_delay,
                                                 1, /* temp server */
                                                 server_tmp.address,
                                                 server_tmp.port,
                                                 server_tmp.ipv6,
                                                 server_tmp.ssl,
                                                 server_tmp.password,
                                                 server_tmp.nick1,
                                                 server_tmp.nick2,
                                                 server_tmp.nick3,
                                                 server_tmp.username,
                                                 server_tmp.realname,
                                                 server_tmp.hostname,
                                                 server_tmp.command,
                                                 1, /* command_delay */
                                                 server_tmp.autojoin,
                                                 1, /* autorejoin */
                                                 NULL);
                    if (ptr_server)
                    {
                        gui_chat_printf_info (NULL,
                                              _("Server %s%s%s created "
                                                "(temporary server, "
                                                "NOT SAVED!)\n"),
                                              GUI_COLOR(GUI_COLOR_CHAT_SERVER),
                                              server_tmp.name,
                                              GUI_COLOR(GUI_COLOR_CHAT));
                        if (!irc_command_connect_one_server (ptr_server, 0))
                            connect_ok = 0;
                    }
                    else
                    {
                        gui_chat_printf_error (NULL,
                                               _("%s unable to create server "
                                                 "\"%s\"\n"),
                                               WEECHAT_ERROR, argv[i]);
                    }
                }
            }
            else
            {
                if (weechat_strcasecmp (argv[i], "-port") == 0)
                    i++;
            }
        }
    }
    
    if (nb_connect == 0)
        connect_ok = irc_command_connect_one_server (ptr_server, no_join);
    
    if (!connect_ok)
        return -1;
    
    return 0;
}

/*
 * irc_command_ctcp: send a ctcp message
 */

int
irc_command_ctcp (t_gui_window *window,
                  char *arguments, int argc, char **argv)
{
    char *pos_type, *pos_args, *pos;
    struct timeval tv;
    
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    pos_type = strchr (arguments, ' ');
    if (pos_type)
    {
        pos_type[0] = '\0';
        pos_type++;
        while (pos_type[0] == ' ')
            pos_type++;
        pos_args = strchr (pos_type, ' ');
        if (pos_args)
        {
            pos_args[0] = '\0';
            pos_args++;
            while (pos_args[0] == ' ')
                pos_args++;
        }
        else
            pos_args = NULL;
        
        pos = pos_type;
        while (pos[0])
        {
            pos[0] = toupper (pos[0]);
            pos++;
        }
        
        gui_chat_printf_server (ptr_server->buffer,
                                "CTCP%s(%s%s%s)%s: %s%s",
                                GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                arguments,
                                GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                GUI_COLOR(GUI_COLOR_CHAT),
                                GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                pos_type);
        
        if ((weechat_strcasecmp (pos_type, "ping") == 0) && (!pos_args))
        {
            gettimeofday (&tv, NULL);
            irc_server_sendf (ptr_server, "PRIVMSG %s :\01PING %d %d\01",
                              arguments, tv.tv_sec, tv.tv_usec);
            gui_chat_printf (ptr_server->buffer, " %s%d %d\n",
                             GUI_COLOR(GUI_COLOR_CHAT),
                             tv.tv_sec, tv.tv_usec);
        }
        else
        {
            if (pos_args)
            {
                irc_server_sendf (ptr_server, "PRIVMSG %s :\01%s %s\01",
                                  arguments, pos_type, pos_args);
                gui_chat_printf (ptr_server->buffer, " %s%s\n",
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 pos_args);
            }
            else
            {
                irc_server_sendf (ptr_server, "PRIVMSG %s :\01%s\01",
                                  arguments, pos_type);
                gui_chat_printf (ptr_server->buffer, "\n");
            }
        }
    }
    return 0;
}

/*
 * irc_command_cycle: leave and rejoin a channel
 */

int
irc_command_cycle (t_gui_window *window,
                   char *arguments, int argc, char **argv)
{
    char *channel_name, *pos_args, *ptr_arg, *buf;
    char **channels;
    int i, num_channels;
    
    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (arguments)
    {
        if (irc_channel_is_channel (arguments))
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
            channels = weechat_explode_string (channel_name, ",", 0,
                                               &num_channels);
            if (channels)
            {
                for (i = 0; i < num_channels; i++)
                {
                    ptr_channel = irc_channel_search (ptr_server, channels[i]);
                    /* mark channel as cycling */
                    if (ptr_channel &&
                        (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
                        ptr_channel->cycle = 1;
                }
                weechat_free_exploded_string (channels);
            }
        }
        else
        {
            if (!ptr_channel)
            {
                gui_chat_printf_error_nolog (ptr_server->buffer,
                                             _("%s \"%s\" command can not be "
                                               "executed on a server "
                                               "buffer\n"),
                                             WEECHAT_ERROR, "cycle");
                return -1;
            }
            
            /* does nothing on private buffer (cycle has no sense!) */
            if (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
                return 0;
            
            channel_name = ptr_channel->name;
            pos_args = arguments;
            ptr_channel->cycle = 1;
        }
    }
    else
    {
        if (!ptr_channel)
        {
            gui_chat_printf_error_nolog (ptr_server->buffer,
                                         _("%s \"%s\" command can not be "
                                           "executed on a server buffer\n"),
                                         WEECHAT_ERROR, "part");
            return -1;
        }
        
        /* does nothing on private buffer (cycle has no sense!) */
        if (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
            return 0;
        
        channel_name = ptr_channel->name;
        pos_args = NULL;
        ptr_channel->cycle = 1;
    }
    
    ptr_arg = (pos_args) ? pos_args :
        (irc_cfg_irc_default_msg_part && irc_cfg_irc_default_msg_part[0]) ?
        irc_cfg_irc_default_msg_part : NULL;
    
    if (ptr_arg)
    {
        buf = weechat_strreplace (ptr_arg, "%v", PACKAGE_VERSION);
        irc_server_sendf (ptr_server, "PART %s :%s", channel_name,
                          (buf) ? buf : ptr_arg);
        if (buf)
            free (buf);
    }
    else
        irc_server_sendf (ptr_server, "PART %s", channel_name);
        
    return 0;
}

/*
 * irc_command_dcc: DCC control (file or chat)
 */

int
irc_command_dcc (t_gui_window *window,
                 char *arguments, int argc, char **argv)
{
    char *pos_nick, *pos_file;
    
    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make compiler happy */
    (void) argc;
    (void) argv;
    
    /* DCC SEND file */
    if (strncasecmp (arguments, "send", 4) == 0)
    {
        pos_nick = strchr (arguments, ' ');
        if (!pos_nick)
        {
            gui_chat_printf_error_nolog (ptr_server->buffer,
                                         _("%s wrong argument count for "
                                           "\"%s\" command\n"),
                                         WEECHAT_ERROR, "dcc send");
            return -1;
        }
        while (pos_nick[0] == ' ')
            pos_nick++;
        
        pos_file = strchr (pos_nick, ' ');
        if (!pos_file)
        {
            gui_chat_printf_error_nolog (ptr_server->buffer,
                                         _("%s wrong argument count for "
                                           "\"%s\" command\n"),
                                         WEECHAT_ERROR, "dcc send");
            return -1;
        }
        pos_file[0] = '\0';
        pos_file++;
        while (pos_file[0] == ' ')
            pos_file++;
        
        irc_dcc_send_request (ptr_server, IRC_DCC_FILE_SEND,
                              pos_nick, pos_file);
    }
    /* DCC CHAT */
    else if (strncasecmp (arguments, "chat", 4) == 0)
    {
        pos_nick = strchr (arguments, ' ');
        if (!pos_nick)
        {
            gui_chat_printf_error_nolog (ptr_server->buffer,
                                         _("%s wrong argument count for "
                                           "\"%s\" command\n"),
                                         WEECHAT_ERROR, "dcc chat");
            return -1;
        }
        while (pos_nick[0] == ' ')
            pos_nick++;
        
        irc_dcc_send_request (ptr_server, IRC_DCC_CHAT_SEND,
                              pos_nick, NULL);
    }
    /* close DCC CHAT */
    else if (weechat_strcasecmp (arguments, "close") == 0)
    {
        if (ptr_channel && (ptr_channel != IRC_CHANNEL_TYPE_CHANNEL)
            && (ptr_channel->dcc_chat))
        {
            irc_dcc_close ((t_irc_dcc *)(ptr_channel->dcc_chat),
                           IRC_DCC_ABORTED);
            irc_dcc_redraw (1);
        }
    }
    /* unknown DCC action */
    else
    {
        gui_chat_printf_error_nolog (ptr_server->buffer,
                                     _("%s wrong arguments for \"%s\" "
                                       "command\n"),
                                     WEECHAT_ERROR, "dcc");
        return -1;
    }
    
    return 0;
}

/*
 * irc_command_dehalfop: remove half operator privileges from nickname(s)
 */

int
irc_command_dehalfop (t_gui_window *window,
                      char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) arguments;
    
    if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
    {
        if (argc == 0)
            irc_server_sendf (ptr_server, "MODE %s -h %s",
                              ptr_channel->name,
                              ptr_server->nick);
        else
            irc_command_mode_nicks (ptr_server, ptr_channel->name,
                                    "-", "h", argc, argv);
    }
    else
    {
        gui_chat_printf_error_nolog (ptr_server->buffer,
                                     _("%s \"%s\" command can only be "
                                       "executed in a channel buffer\n"),
                                     WEECHAT_ERROR, "dehalfop");
    }
    return 0;
}

/*
 * irc_command_deop: remove operator privileges from nickname(s)
 */

int
irc_command_deop (t_gui_window *window,
              char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) arguments;
    
    if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
    {
        if (argc == 0)
            irc_server_sendf (ptr_server, "MODE %s -o %s",
                              ptr_channel->name,
                              ptr_server->nick);
        else
            irc_command_mode_nicks (ptr_server, ptr_channel->name,
                                    "-", "o", argc, argv);
    }
    else
    {
        gui_chat_printf_error_nolog (ptr_server->buffer,
                                     _("%s \"%s\" command can only be "
                                       "executed in a channel buffer\n"),
                                     WEECHAT_ERROR, "deop");
    }
    return 0;
}

/*
 * irc_command_devoice: remove voice from nickname(s)
 */

int
irc_command_devoice (t_gui_window *window,
                     char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) arguments;
    
    if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
    {
        if (argc == 0)
            irc_server_sendf (ptr_server, "MODE %s -v %s",
                              ptr_channel->name,
                              ptr_server->nick);
        else
            irc_command_mode_nicks (ptr_server, ptr_channel->name,
                                    "-", "v", argc, argv);
    }
    else
    {
        gui_chat_printf_error_nolog (ptr_server->buffer,
                                     _("%s \"%s\" command can only be "
                                       "executed in a channel buffer\n"),
                                     WEECHAT_ERROR, "devoice");
        return -1;
    }
    return 0;
}

/*
 * irc_command_die: shotdown the server
 */

int
irc_command_die (t_gui_window *window,
                 char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) arguments;
    (void) argc;
    (void) argv;
    
    irc_server_sendf (ptr_server, "DIE");
    return 0;
}

/*
 * irc_command_quit_server: send QUIT to a server
 */

void
irc_command_quit_server (t_irc_server *server, char *arguments)
{
    char *ptr_arg, *buf;
    
    if (!server)
        return;
    
    if (server->is_connected)
    {
        ptr_arg = (arguments) ? arguments :
            (irc_cfg_irc_default_msg_quit && irc_cfg_irc_default_msg_quit[0]) ?
            irc_cfg_irc_default_msg_quit : NULL;
        
        if (ptr_arg)
        {
            buf = weechat_strreplace (ptr_arg, "%v", PACKAGE_VERSION);
            irc_server_sendf (server, "QUIT :%s",
                              (buf) ? buf : ptr_arg);
            if (buf)
                free (buf);
        }
        else
            irc_server_sendf (server, "QUIT");
    }
}

/*
 * irc_command_disconnect_one_server: disconnect from a server
 *                                return 0 if error, 1 if ok
 */

int
irc_command_disconnect_one_server (t_irc_server *server)
{
    if (!server)
        return 0;
    
    if ((!server->is_connected) && (server->child_pid == 0)
        && (server->reconnect_start == 0))
    {
        gui_chat_printf_error (server->buffer,
                               _("%s not connected to server \"%s\"!\n"),
                               WEECHAT_ERROR, server->name);
        return 0;
    }
    if (server->reconnect_start > 0)
    {
        gui_chat_printf_info (server->buffer,
                              _("Auto-reconnection is cancelled\n"));
    }
    irc_command_quit_server (server, NULL);
    irc_server_disconnect (server, 0);
    gui_status_draw (server->buffer, 1);
    
    /* disconnect ok */
    return 1;
}

/*
 * irc_command_disconnect: disconnect from server(s)
 */

int
irc_command_disconnect (t_gui_window *window,
                    char *arguments, int argc, char **argv)
{
    int i, disconnect_ok;
    
    IRC_BUFFER_GET_SERVER(window->buffer);
    
    /* make C compiler happy */
    (void) arguments;
    
    if (argc == 0)
        disconnect_ok = irc_command_disconnect_one_server (ptr_server);
    else
    {
        disconnect_ok = 1;
        
        if (weechat_strcasecmp (argv[0], "-all") == 0)
        {
            for (ptr_server = irc_servers; ptr_server;
                 ptr_server = ptr_server->next_server)
            {
                if ((ptr_server->is_connected) || (ptr_server->child_pid != 0)
                    || (ptr_server->reconnect_start != 0))
                {
                    if (!irc_command_disconnect_one_server (ptr_server))
                        disconnect_ok = 0;
                }
            }
        }
        else
        {
            for (i = 0; i < argc; i++)
            {
                ptr_server = irc_server_search (argv[i]);
                if (ptr_server)
                {
                    if (!irc_command_disconnect_one_server (ptr_server))
                        disconnect_ok = 0;
                }
                else
                {
                    gui_chat_printf_error (NULL,
                                           _("%s server \"%s\" not found\n"),
                                           WEECHAT_ERROR, argv[i]);
                    disconnect_ok = 0;
                }
            }
        }
    }
    
    if (!disconnect_ok)
        return -1;
    
    return 0;
}

/*
 * irc_command_halfop: give half operator privileges to nickname(s)
 */

int
irc_command_halfop (t_gui_window *window,
                    char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) arguments;
    
    if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
    {
        if (argc == 0)
            irc_server_sendf (ptr_server, "MODE %s +h %s",
                              ptr_channel->name,
                              ptr_server->nick);
        else
            irc_command_mode_nicks (ptr_server, ptr_channel->name,
                                    "+", "h", argc, argv);
    }
    else
    {
        gui_chat_printf_error_nolog (ptr_server->buffer,
                                     _("%s \"%s\" command can only be "
                                       "executed in a channel buffer\n"),
                                     WEECHAT_ERROR, "halfop");
        return -1;
    }
    return 0;
}

/*
 * irc_command_info: get information describing the server
 */

int
irc_command_info (t_gui_window *window,
                  char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (arguments)
        irc_server_sendf (ptr_server, "INFO %s", arguments);
    else
        irc_server_sendf (ptr_server, "INFO");
    return 0;
}

/*
 * irc_command_invite: invite a nick on a channel
 */

int
irc_command_invite (t_gui_window *window,
                    char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) arguments;
    
    if (argc == 2)
        irc_server_sendf (ptr_server, "INVITE %s %s", argv[0], argv[1]);
    else
    {
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
            irc_server_sendf (ptr_server, "INVITE %s %s",
                              argv[0], ptr_channel->name);
        else
        {
            gui_chat_printf_error_nolog (ptr_server->buffer,
                                         _("%s \"%s\" command can only be "
                                           "executed in a channel buffer\n"),
                                         WEECHAT_ERROR, "invite");
            return -1;
        }

    }
    return 0;
}

/*
 * irc_command_ison: check if a nickname is currently on IRC
 */

int
irc_command_ison (t_gui_window *window,
                  char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    irc_server_sendf (ptr_server, "ISON %s", arguments);
    return 0;
}

/*
 * irc_command_join_server: send JOIN command on a server
 */

void
irc_command_join_server (t_irc_server *server, char *arguments)
{
    if (irc_channel_is_channel (arguments))
        irc_server_sendf (server, "JOIN %s", arguments);
    else
        irc_server_sendf (server, "JOIN #%s", arguments);
}

/*
 * irc_command_join: join a new channel
 */

int
irc_command_join (t_gui_window *window,
                  char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    irc_command_join_server (ptr_server, arguments);
    
    return 0;
}

/*
 * irc_command_kick: forcibly remove a user from a channel
 */

int
irc_command_kick (t_gui_window *window,
                  char *arguments, int argc, char **argv)
{
    char *pos_channel, *pos_nick, *pos_comment;

    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (irc_channel_is_channel (arguments))
    {
        pos_channel = arguments;
        pos_nick = strchr (arguments, ' ');
        if (!pos_nick)
        {
            gui_chat_printf_error_nolog (ptr_server->buffer,
                                         _("%s wrong arguments for \"%s\" "
                                           "command\n"),
                                         WEECHAT_ERROR, "kick");
            return -1;
        }
        pos_nick[0] = '\0';
        pos_nick++;
        while (pos_nick[0] == ' ')
            pos_nick++;
    }
    else
    {
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
        {
            pos_channel = ptr_channel->name;
            pos_nick = arguments;
        }
        else
        {
            gui_chat_printf_error_nolog (ptr_server->buffer,
                                         _("%s \"%s\" command can only be "
                                           "executed in a channel buffer\n"),
                                         WEECHAT_ERROR, "kick");
            return -1;
        }
    }
    
    pos_comment = strchr (pos_nick, ' ');
    if (pos_comment)
    {
        pos_comment[0] = '\0';
        pos_comment++;
        while (pos_comment[0] == ' ')
            pos_comment++;
    }
    
    if (pos_comment)
        irc_server_sendf (ptr_server, "KICK %s %s :%s",
                          pos_channel, pos_nick, pos_comment);
    else
        irc_server_sendf (ptr_server, "KICK %s %s",
                          pos_channel, pos_nick);
    
    return 0;
}

/*
 * irc_command_kickban: forcibly remove a user from a channel and ban it
 */

int
irc_command_kickban (t_gui_window *window,
                     char *arguments, int argc, char **argv)
{
    char *pos_channel, *pos_nick, *pos_comment;
    
    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (irc_channel_is_channel (arguments))
    {
        pos_channel = arguments;
        pos_nick = strchr (arguments, ' ');
        if (!pos_nick)
        {
            gui_chat_printf_error_nolog (ptr_server->buffer,
                                         _("%s wrong arguments for \"%s\" "
                                           "command\n"),
                                         WEECHAT_ERROR, "kickban");
            return -1;
        }
        pos_nick[0] = '\0';
        pos_nick++;
        while (pos_nick[0] == ' ')
            pos_nick++;
    }
    else
    {
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
        {
            pos_channel = ptr_channel->name;
            pos_nick = arguments;
        }
        else
        {
            gui_chat_printf_error_nolog (ptr_server->buffer,
                                         _("%s \"%s\" command can only be "
                                           "executed in a channel buffer\n"),
                                         WEECHAT_ERROR, "kickban");
            return -1;
        }
    }
    
    pos_comment = strchr (pos_nick, ' ');
    if (pos_comment)
    {
        pos_comment[0] = '\0';
        pos_comment++;
        while (pos_comment[0] == ' ')
            pos_comment++;
    }
    
    irc_server_sendf (ptr_server, "MODE %s +b %s",
                      pos_channel, pos_nick);
    if (pos_comment)
        irc_server_sendf (ptr_server, "KICK %s %s :%s",
                          pos_channel, pos_nick, pos_comment);
    else
        irc_server_sendf (ptr_server, "KICK %s %s",
                          pos_channel, pos_nick);
    
    return 0;
}

/*
 * irc_command_kill: close client-server connection
 */

int
irc_command_kill (t_gui_window *window,
                  char *arguments, int argc, char **argv)
{
    char *pos;
    
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
        irc_server_sendf (ptr_server, "KILL %s :%s",
                          arguments, pos);
    }
    else
        irc_server_sendf (ptr_server, "KILL %s",
                          arguments);
    return 0;
}

/*
 * irc_command_links: list all servernames which are known by the server
 *                     answering the query
 */

int
irc_command_links (t_gui_window *window,
                   char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (arguments)
        irc_server_sendf (ptr_server, "LINKS %s", arguments);
    else
        irc_server_sendf (ptr_server, "LINKS");
    return 0;
}

/*
 * irc_command_list: close client-server connection
 */

int
irc_command_list (t_gui_window *window,
                  char *arguments, int argc, char **argv)
{
    char buf[512];
    int ret;
    
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (ptr_server->cmd_list_regexp)
    {
	regfree (ptr_server->cmd_list_regexp);
	free (ptr_server->cmd_list_regexp);
	ptr_server->cmd_list_regexp = NULL;
    }
    
    if (arguments)
    {
	ptr_server->cmd_list_regexp = (regex_t *) malloc (sizeof (regex_t));
	if (ptr_server->cmd_list_regexp)
	{
	    if ((ret = regcomp (ptr_server->cmd_list_regexp,
                                arguments,
                                REG_NOSUB | REG_ICASE)) != 0)
	    {
		regerror (ret, ptr_server->cmd_list_regexp,
                          buf, sizeof(buf));
		gui_chat_printf (ptr_server->buffer,
                                 _("%s \"%s\" is not a valid regular "
                                   "expression (%s)\n"),
                                 WEECHAT_ERROR, arguments, buf);
	    }
	    else
		irc_server_sendf (ptr_server, "LIST");
	}
	else
	{
	    gui_chat_printf (ptr_server->buffer,
                             _("%s not enough memory for regular "
                               "expression\n"),
                             WEECHAT_ERROR);
	}
    }
    else
	irc_server_sendf (ptr_server, "LIST");
    
    return 0;
}

/*
 * irc_command_lusers: get statistics about ths size of the IRC network
 */

int
irc_command_lusers (t_gui_window *window,
                    char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (arguments)
        irc_server_sendf (ptr_server, "LUSERS %s", arguments);
    else
        irc_server_sendf (ptr_server, "LUSERS");
    return 0;
}

/*
 * irc_command_me: send a ctcp action to the current channel
 */

int
irc_command_me (t_gui_window *window,
                char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (!ptr_channel)
    {
        gui_chat_printf_error_nolog (ptr_server->buffer,
                                     _("%s \"%s\" command can not be executed "
                                       "on a server buffer\n"),
                                     WEECHAT_ERROR, "me");
        return -1;
    }
    irc_command_me_channel (ptr_server, ptr_channel, arguments);
    return 0;
}

/*
 * irc_command_mode_server! send MODE command on a server
 */

void
irc_command_mode_server (t_irc_server *server, char *arguments)
{
    irc_server_sendf (server, "MODE %s", arguments);
}

/*
 * irc_command_mode: change mode for channel/nickname
 */

int
irc_command_mode (t_gui_window *window,
                  char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    irc_command_mode_server (ptr_server, arguments);
    
    return 0;
}

/*
 * irc_command_motd: get the "Message Of The Day"
 */

int
irc_command_motd (t_gui_window *window,
                  char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (arguments)
        irc_server_sendf (ptr_server, "MOTD %s", arguments);
    else
        irc_server_sendf (ptr_server, "MOTD");
    return 0;
}

/*
 * irc_command_msg: send a message to a nick or channel
 */

int
irc_command_msg (t_gui_window *window,
                 char *arguments, int argc, char **argv)
{
    char *pos, *pos_comma;
    char *msg_pwd_hidden;
    t_irc_nick *ptr_nick;
    char *string;
    
    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
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
                if (!ptr_channel
                    || ((ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
                        && (ptr_channel->type != IRC_CHANNEL_TYPE_PRIVATE)))
                {
                    gui_chat_printf_error_nolog (ptr_server->buffer,
                                                 _("%s \"%s\" command can "
                                                   "only be executed in a "
                                                   "channel or private "
                                                   "buffer\n"),
                                                 WEECHAT_ERROR, "msg *");
                    return -1;
                }
                if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                    ptr_nick = irc_nick_search (ptr_channel, ptr_server->nick);
                else
                    ptr_nick = NULL;
                irc_display_nick (ptr_channel->buffer, ptr_nick,
                                  (ptr_nick) ? NULL : ptr_server->nick,
                                  GUI_MSG_TYPE_NICK, 1, NULL, 0);
                string = (char *)irc_color_decode ((unsigned char *)pos, 1, 0);
                gui_chat_printf_type (ptr_channel->buffer, GUI_MSG_TYPE_MSG,
                                      NULL, -1,
                                      "%s\n",
                                      (string) ? string : "");
                if (string)
                    free (string);
                
                irc_server_sendf (ptr_server, "PRIVMSG %s :%s",
                                  ptr_channel->name, pos);
            }
            else
            {
                if (irc_channel_is_channel (arguments))
                {
                    ptr_channel = irc_channel_search (ptr_server, arguments);
                    if (ptr_channel)
                    {
                        ptr_nick = irc_nick_search (ptr_channel,
                                                    ptr_server->nick);
                        if (ptr_nick)
                        {
                            irc_display_nick (ptr_channel->buffer, ptr_nick,
                                              NULL, GUI_MSG_TYPE_NICK, 1,
                                              NULL, 0);
                            string = (char *)irc_color_decode (
                                (unsigned char *)pos, 1, 0);
                            gui_chat_printf_type (ptr_channel->buffer,
                                                  GUI_MSG_TYPE_MSG,
                                                  NULL, -1,
                                                  "%s\n",
                                                  (string) ? string : "");
                            if (string)
                                free (string);
                        }
                        else
                        {
                            gui_chat_printf_error_nolog (ptr_server->buffer,
                                                         _("%s nick \"%s\" "
                                                           "not found for "
                                                           "\"%s\" command\n"),
                                                         WEECHAT_ERROR,
                                                         ptr_server->nick,
                                                         "msg");
                        }
                    }
                    irc_server_sendf (ptr_server, "PRIVMSG %s :%s",
                                      arguments, pos);
                }
                else
                {
                    /* message to nickserv with identify ? */
                    if (strcmp (arguments, "nickserv") == 0)
                    {
                        msg_pwd_hidden = strdup (pos);
                        if (irc_cfg_log_hide_nickserv_pwd)
                            irc_display_hide_password (msg_pwd_hidden, 0);
                        gui_chat_printf_type (ptr_server->buffer,
                                              GUI_MSG_TYPE_NICK,
                                              cfg_look_prefix_server,
                                              cfg_col_chat_prefix_server,
                                              "%s-%s%s%s- ",
                                              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                              GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                              arguments,
                                              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
                        string = (char *)irc_color_decode (
                            (unsigned char *)msg_pwd_hidden, 1, 0);
                        gui_chat_printf (ptr_server->buffer,
                                         "%s%s\n",
                                         GUI_COLOR(GUI_COLOR_CHAT),
                                         (string) ? string : "");
                        if (string)
                            free (string);
                        irc_server_sendf (ptr_server, "PRIVMSG %s :%s",
                                          arguments, pos);
                        free (msg_pwd_hidden);
                        return 0;
                    }
                    
                    string = (char *)irc_color_decode (
                        (unsigned char *)pos, 1, 0);
                    ptr_channel = irc_channel_search (ptr_server, arguments);
                    if (ptr_channel)
                    {
                        irc_display_nick (ptr_channel->buffer, NULL,
                                          ptr_server->nick,
                                          GUI_MSG_TYPE_NICK, 1,
                                          GUI_COLOR(GUI_CHAT_NICK_SELF),
                                          0);
                        gui_chat_printf_type (ptr_channel->buffer,
                                              GUI_MSG_TYPE_MSG,
                                              NULL, -1,
                                              "%s%s\n",
                                              GUI_COLOR(GUI_COLOR_CHAT),
                                              (string) ? string : "");
                    }
                    else
                    {
                        gui_chat_printf_server (ptr_server->buffer,
                                                "MSG%s(%s%s%s)%s: ",
                                                GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                                GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                                arguments,
                                                GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                                GUI_COLOR(GUI_COLOR_CHAT));
                        gui_chat_printf_type (ptr_server->buffer,
                                              GUI_MSG_TYPE_MSG,
                                              cfg_look_prefix_server,
                                              cfg_col_chat_prefix_server,
                                              "%s\n",
                                              (string) ? string : pos);
                    }
                    if (string)
                        free (string);
                    irc_server_sendf (ptr_server, "PRIVMSG %s :%s",
                                      arguments, pos);
                }
            }
            arguments = pos_comma;
        }
    }
    else
    {
        gui_chat_printf_error_nolog (ptr_server->buffer,
                                     _("%s wrong argument count for \"%s\" "
                                       "command\n"),
                                     WEECHAT_ERROR, "msg");
        return -1;
    }
    return 0;
}

/*
 * irc_command_names: list nicknames on channels
 */

int
irc_command_names (t_gui_window *window,
                   char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (arguments)
        irc_server_sendf (ptr_server, "NAMES %s", arguments);
    else
    {
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
            irc_server_sendf (ptr_server, "NAMES %s",
                              ptr_channel->name);
        else
        {
            gui_chat_printf_error_nolog (ptr_server->buffer,
                                         _("%s \"%s\" command can only be "
                                           "executed in a channel buffer\n"),
                                         WEECHAT_ERROR, "names");
            return -1;
        }
    }
    return 0;
}

/*
 * irc_send_nick_server: change nickname on a server
 */

void
irc_send_nick_server (t_irc_server *server, char *nickname)
{
    t_irc_channel *ptr_channel;
    
    if (!server)
        return;
    
    if (server->is_connected)
        irc_server_sendf (server, "NICK %s", nickname);
    else
    {
        if (server->nick)
            free (server->nick);
        server->nick = strdup (nickname);
        gui_input_draw (server->buffer, 1);
        for (ptr_channel = server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            gui_input_draw (ptr_channel->buffer, 1);
        }
    }
}

/*
 * irc_command_nick: change nickname
 */

int
irc_command_nick (t_gui_window *window,
                  char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server)
        return -1;
    
    /* make C compiler happy */
    (void) arguments;
    
    if (argc == 2)
    {
        if (strncmp (argv[0], "-all", 4) != 0)
            return -1;
        
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            irc_send_nick_server (ptr_server, argv[1]);
        }
    }
    else
    {
        if (argc == 1)
            irc_send_nick_server (ptr_server, argv[0]);
        else
            return -1;
    }
    
    return 0;
}

/*
 * irc_command_notice: send notice message
 */

int
irc_command_notice (t_gui_window *window,
                    char *arguments, int argc, char **argv)
{
    char *pos, *string;

    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
        string = (char *)irc_color_decode ((unsigned char *)pos, 1, 0);
        gui_chat_printf_server (ptr_server->buffer,
                                "notice%s(%s%s%s)%s: %s\n",
                                GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                arguments,
                                GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                GUI_COLOR(GUI_COLOR_CHAT),
                                (string) ? string : "");
        if (string)
            free (string);
        irc_server_sendf (ptr_server, "NOTICE %s :%s",
                          arguments, pos);
    }
    else
    {
        gui_chat_printf_error_nolog (ptr_server->buffer,
                                     _("%s wrong argument count for \"%s\" "
                                       "command\n"),
                                     WEECHAT_ERROR, "notice");
        return -1;
    }
    return 0;
}

/*
 * irc_command_op: give operator privileges to nickname(s)
 */

int
irc_command_op (t_gui_window *window,
                char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) arguments;
    
    if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
    {
        if (argc == 0)
            irc_server_sendf (ptr_server, "MODE %s +o %s",
                              ptr_channel->name,
                              ptr_server->nick);
        else
            irc_command_mode_nicks (ptr_server, ptr_channel->name,
                                    "+", "o", argc, argv);
    }
    else
    {
        gui_chat_printf_error_nolog (ptr_server->buffer,
                                     _("%s \"%s\" command can only be "
                                       "executed in a channel buffer\n"),
                                     WEECHAT_ERROR, "op");
        return -1;
    }
    return 0;
}

/*
 * irc_command_oper: get oper privileges
 */

int
irc_command_oper (t_gui_window *window,
                  char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    irc_server_sendf (ptr_server, "OPER %s", arguments);
    return 0;
}

/*
 * irc_command_part: leave a channel or close a private window
 */

int
irc_command_part (t_gui_window *window,
                  char *arguments, int argc, char **argv)
{
    char *channel_name, *pos_args, *ptr_arg, *buf;

    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (arguments)
    {
        if (irc_channel_is_channel (arguments))
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
            if (!ptr_channel)
            {
                gui_chat_printf_error_nolog (ptr_server->buffer,
                                             _("%s \"%s\" command can only be "
                                               "executed in a channel or "
                                               "private buffer\n"),
                                             WEECHAT_ERROR, "part");
                return -1;
            }
            channel_name = ptr_channel->name;
            pos_args = arguments;
        }
    }
    else
    {
        if (!ptr_channel)
        {
            gui_chat_printf_error_nolog (ptr_server->buffer,
                                         _("%s \"%s\" command can only be "
                                           "executed in a channel or private "
                                           "buffer\n"),
                                         WEECHAT_ERROR, "part");
            return -1;
        }
        if (!ptr_channel->nicks)
        {
            gui_buffer_free (ptr_channel->buffer, 1);
            irc_channel_free (ptr_server, ptr_channel);
            gui_status_draw (gui_current_window->buffer, 1);
            gui_input_draw (gui_current_window->buffer, 1);
            return 0;
        }
        channel_name = ptr_channel->name;
        pos_args = NULL;
    }
    
    ptr_arg = (pos_args) ? pos_args :
        (irc_cfg_irc_default_msg_part && irc_cfg_irc_default_msg_part[0]) ?
        irc_cfg_irc_default_msg_part : NULL;
    
    if (ptr_arg)
    {
        buf = weechat_strreplace (ptr_arg, "%v", PACKAGE_VERSION);
        irc_server_sendf (ptr_server, "PART %s :%s",
                          channel_name,
                          (buf) ? buf : ptr_arg);
        if (buf)
            free (buf);
    }
    else
        irc_server_sendf (ptr_server, "PART %s", channel_name);
    
    return 0;
}

/*
 * irc_command_ping: ping a server
 */

int
irc_command_ping (t_gui_window *window,
                  char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    irc_server_sendf (ptr_server, "PING %s", arguments);
    return 0;
}

/*
 * irc_command_pong: send pong answer to a daemon
 */

int
irc_command_pong (t_gui_window *window,
                  char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    irc_server_sendf (ptr_server, "PONG %s", arguments);
    return 0;
}

/*
 * irc_command_query: start private conversation with a nick
 */

int
irc_command_query (t_gui_window *window,
                   char *arguments, int argc, char **argv)
{
    char *pos, *string;
    
    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
        if (!pos[0])
            pos = NULL;
    }
    
    /* create private window if not already opened */
    ptr_channel = irc_channel_search (ptr_server, arguments);
    if (!ptr_channel)
    {
        ptr_channel = irc_channel_new (ptr_server,
                                       IRC_CHANNEL_TYPE_PRIVATE,
                                       arguments, 1);
        if (!ptr_channel)
        {
            gui_chat_printf_error_nolog (ptr_server->buffer,
                                         _("%s cannot create new private "
                                           "buffer \"%s\"\n"),
                                         WEECHAT_ERROR, arguments);
            return -1;
        }
        gui_chat_draw_title (ptr_channel->buffer, 1);
    }
    else
    {
        if (window->buffer != ptr_channel->buffer)
        {
            gui_window_switch_to_buffer (window, ptr_channel->buffer);
            gui_window_redraw_buffer (ptr_channel->buffer);
        }
    }
    
    /* display text if given */
    if (pos)
    {
        irc_display_nick (ptr_channel->buffer, NULL, ptr_server->nick,
                          GUI_MSG_TYPE_NICK, 1,
                          GUI_COLOR(GIU_COLOR_CHAT_NICK_SELF), 0);
        string = (char *)irc_color_decode ((unsigned char *)pos, 1, 0);
        gui_chat_printf_type (ptr_channel->buffer, GUI_MSG_TYPE_MSG,
                              NULL, -1,
                              "%s%s\n",
                              GUI_COLOR(GUI_COLOR_CHAT),
                              (string) ? string : "");
        if (string)
            free (string);
        irc_server_sendf (ptr_server, "PRIVMSG %s :%s",
                          arguments, pos);
    }
    return 0;
}

/*
 * irc_command_quit: disconnect from all servers and quit WeeChat
 */

int
irc_command_quit (t_gui_window *window,
                  char *arguments, int argc, char **argv)
{
    t_irc_server *ptr_server;
    
    /* make C compiler happy */
    (void) window;
    (void) argc;
    (void) argv;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        irc_command_quit_server (ptr_server, arguments);
    }
    quit_weechat = 1;
    return 0;
}

/*
 * irc_command_quote: send raw data to server
 */

int
irc_command_quote (t_gui_window *window,
                   char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (ptr_server->sock < 0)
    {
        gui_chat_printf_error_nolog (NULL,
                                     _("%s command \"%s\" needs a server "
                                       "connection!\n"),
                                     WEECHAT_ERROR, "quote");
        return -1;
    }
    irc_server_sendf (ptr_server, "%s", arguments);
    return 0;
}

/*
 * irc_command_reconnect_one_server: reconnect to a server
 *                               return 0 if error, 1 if ok
 */

int
irc_command_reconnect_one_server (t_irc_server *server, int no_join)
{
    if (!server)
        return 0;
    
    if ((!server->is_connected) && (server->child_pid == 0))
    {
        gui_chat_printf_error (server->buffer,
                               _("%s not connected to server \"%s\"!\n"),
                               WEECHAT_ERROR, server->name);
        return 0;
    }
    irc_command_quit_server (server, NULL);
    irc_server_disconnect (server, 0);
    if (irc_server_connect (server, no_join))
    {
        server->reconnect_start = 0;
        server->reconnect_join = (server->channels) ? 1 : 0;    
    }
    gui_status_draw (server->buffer, 1);
    
    /* reconnect ok */
    return 1;
}

/*
 * irc_command_reconnect: reconnect to server(s)
 */

int
irc_command_reconnect (t_gui_window *window,
                       char *arguments, int argc, char **argv)
{
    int i, nb_reconnect, reconnect_ok, all_servers, no_join;

    IRC_BUFFER_GET_SERVER(window->buffer);
    
    /* make C compiler happy */
    (void) arguments;
    
    nb_reconnect = 0;
    reconnect_ok = 1;
    
    all_servers = 0;
    no_join = 0;
    for (i = 0; i < argc; i++)
    {
        if (weechat_strcasecmp (argv[i], "-all") == 0)
            all_servers = 1;
        if (weechat_strcasecmp (argv[i], "-nojoin") == 0)
            no_join = 1;
    }
    
    if (all_servers)
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            nb_reconnect++;
            if ((ptr_server->is_connected) || (ptr_server->child_pid != 0))
            {
                if (!irc_command_reconnect_one_server (ptr_server, no_join))
                    reconnect_ok = 0;
            }
        }
    }
    else
    {
        for (i = 0; i < argc; i++)
        {
            if (argv[i][0] != '-')
            {
                nb_reconnect++;
                ptr_server = irc_server_search (argv[i]);
                if (ptr_server)
                {
                    if (!irc_command_reconnect_one_server (ptr_server, no_join))
                        reconnect_ok = 0;
                }
                else
                {
                    gui_chat_printf_error (NULL,
                                           _("%s server \"%s\" not found\n"),
                                           WEECHAT_ERROR, argv[i]);
                    reconnect_ok = 0;
                }
            }
        }
    }
    
    if (nb_reconnect == 0)
        reconnect_ok = irc_command_reconnect_one_server (ptr_server, no_join);
    
    if (!reconnect_ok)
        return -1;
    
    return 0;
}

/*
 * irc_command_rehash: tell the server to reload its config file
 */

int
irc_command_rehash (t_gui_window *window,
                    char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) arguments;
    (void) argc;
    (void) argv;
    
    irc_server_sendf (ptr_server, "REHASH");
    return 0;
}

/*
 * irc_command_restart: tell the server to restart itself
 */

int
irc_command_restart (t_gui_window *window,
                     char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) arguments;
    (void) argc;
    (void) argv;
    
    irc_server_sendf (ptr_server, "RESTART");
    return 0;
}

/*
 * irc_command_server: manage IRC servers
 */

int
irc_command_server (t_gui_window *window,
                    char *arguments, int argc, char **argv)
{
    int i, detailed_list, one_server_found;
    t_irc_server server_tmp, *ptr_server, *server_found, *new_server;
    char *server_name, *error;
    long number;
    t_gui_buffer *ptr_buffer;
    
    /* make C compiler happy */
    (void) arguments;
    
    if ((argc == 0)
        || (weechat_strcasecmp (argv[0], "list") == 0)
        || (weechat_strcasecmp (argv[0], "listfull") == 0))
    {
        /* list servers */
        server_name = NULL;
        detailed_list = 0;
        for (i = 0; i < argc; i++)
        {
            if (weechat_strcasecmp (argv[i], "list") == 0)
                continue;
            if (weechat_strcasecmp (argv[i], "listfull") == 0)
            {
                detailed_list = 1;
                continue;
            }
            if (!server_name)
                server_name = argv[i];
        }
        if (!server_name)
        {
            if (irc_servers)
            {
                gui_chat_printf (NULL, "\n");
                gui_chat_printf (NULL, _("All servers:\n"));
                for (ptr_server = irc_servers; ptr_server;
                     ptr_server = ptr_server->next_server)
                {
                    irc_display_server (ptr_server, detailed_list);
                }
            }
            else
                gui_chat_printf_info (NULL, _("No server.\n"));
        }
        else
        {
            one_server_found = 0;
            for (ptr_server = irc_servers; ptr_server;
                 ptr_server = ptr_server->next_server)
            {
                if (weechat_strcasestr (ptr_server->name, server_name))
                {
                    if (!one_server_found)
                    {
                        gui_chat_printf (NULL, "\n");
                        gui_chat_printf (NULL, _("Servers with '%s':\n"),
                                         server_name);
                    }
                    one_server_found = 1;
                    irc_display_server (ptr_server, detailed_list);
                }
            }
            if (!one_server_found)
                gui_chat_printf_info (NULL,
                                      _("No server with '%s' found.\n"),
                                      server_name);
        }
    }
    else
    {
        if (weechat_strcasecmp (argv[0], "add") == 0)
        {
            if (argc < 3)
            {
                gui_chat_printf_error (NULL,
                                       _("%s missing parameters for \"%s\" "
                                         "command\n"),
                                       WEECHAT_ERROR, "server");
                return -1;
            }
            
            if (irc_server_name_already_exists (argv[1]))
            {
                gui_chat_printf_error (NULL,
                                       _("%s server \"%s\" already exists, "
                                         "can't create it!\n"),
                                       WEECHAT_ERROR, argv[1]);
                return -1;
            }

            /* init server struct */
            irc_server_init (&server_tmp);
            
            server_tmp.name = strdup (argv[1]);
            server_tmp.address = strdup (argv[2]);
            server_tmp.port = IRC_SERVER_DEFAULT_PORT;
            
            /* parse arguments */
            for (i = 3; i < argc; i++)
            {
                if (argv[i][0] == '-')
                {
                    if (weechat_strcasecmp (argv[i], "-temp") == 0)
                        server_tmp.temp_server = 1;
                    if (weechat_strcasecmp (argv[i], "-auto") == 0)
                        server_tmp.autoconnect = 1;
                    if (weechat_strcasecmp (argv[i], "-noauto") == 0)
                        server_tmp.autoconnect = 0;
                    if (weechat_strcasecmp (argv[i], "-ipv6") == 0)
                        server_tmp.ipv6 = 1;
                    if (weechat_strcasecmp (argv[i], "-ssl") == 0)
                        server_tmp.ssl = 1;
                    if (weechat_strcasecmp (argv[i], "-port") == 0)
                    {
                        if (i == (argc - 1))
                        {
                            gui_chat_printf_error (NULL,
                                                   _("%s missing argument for "
                                                     "\"%s\" option\n"),
                                                   WEECHAT_ERROR, "-port");
                            irc_server_free_data (&server_tmp);
                            return -1;
                        }
                        error = NULL;
                        number = strtol (argv[++i], &error, 10);
                        if (error && (error[0] == '\0'))
                            server_tmp.port = number;
                    }
                    if (weechat_strcasecmp (argv[i], "-pwd") == 0)
                    {
                        if (i == (argc - 1))
                        {
                            gui_chat_printf_error (NULL,
                                                   _("%s missing argument for "
                                                     "\"%s\" option\n"),
                                                   WEECHAT_ERROR, "-pwd");
                            irc_server_free_data (&server_tmp);
                            return -1;
                        }
                        server_tmp.password = strdup (argv[++i]);
                    }
                    if (weechat_strcasecmp (argv[i], "-nicks") == 0)
                    {
                        if (i >= (argc - 3))
                        {
                            gui_chat_printf_error (NULL,
                                                   _("%s missing argument for "
                                                     "\"%s\" option\n"),
                                                   WEECHAT_ERROR, "-nicks");
                            irc_server_free_data (&server_tmp);
                            return -1;
                        }
                        server_tmp.nick1 = strdup (argv[++i]);
                        server_tmp.nick2 = strdup (argv[++i]);
                        server_tmp.nick3 = strdup (argv[++i]);
                    }
                    if (weechat_strcasecmp (argv[i], "-username") == 0)
                    {
                        if (i == (argc - 1))
                        {
                            gui_chat_printf_error (NULL,
                                              _("%s missing argument for "
                                                "\"%s\" option\n"),
                                              WEECHAT_ERROR, "-username");
                            irc_server_free_data (&server_tmp);
                            return -1;
                        }
                        server_tmp.username = strdup (argv[++i]);
                    }
                    if (weechat_strcasecmp (argv[i], "-realname") == 0)
                    {
                        if (i == (argc - 1))
                        {
                            gui_chat_printf_error (NULL,
                                                   _("%s missing argument for "
                                                     "\"%s\" option\n"),
                                                   WEECHAT_ERROR, "-realname");
                            irc_server_free_data (&server_tmp);
                            return -1;
                        }
                        server_tmp.realname = strdup (argv[++i]);
                    }
                    if (weechat_strcasecmp (argv[i], "-command") == 0)
                    {
                        if (i == (argc - 1))
                        {
                            gui_chat_printf_error (NULL,
                                                   _("%s missing argument for "
                                                     "\"%s\" option\n"),
                                                   WEECHAT_ERROR, "-command");
                            irc_server_free_data (&server_tmp);
                            return -1;
                        }
                        server_tmp.command = strdup (argv[++i]);
                    }
                    if (weechat_strcasecmp (argv[i], "-autojoin") == 0)
                    {
                        if (i == (argc - 1))
                        {
                            gui_chat_printf_error (NULL,
                                                   _("%s missing argument for "
                                                     "\"%s\" option\n"),
                                                   WEECHAT_ERROR, "-autojoin");
                            irc_server_free_data (&server_tmp);
                            return -1;
                        }
                        server_tmp.autojoin = strdup (argv[++i]);
                    }
                }
            }
            
            /* create new server */
            new_server = irc_server_new (server_tmp.name,
                                         server_tmp.autoconnect,
                                         server_tmp.autoreconnect,
                                         server_tmp.autoreconnect_delay,
                                         server_tmp.temp_server,
                                         server_tmp.address,
                                         server_tmp.port,
                                         server_tmp.ipv6,
                                         server_tmp.ssl,
                                         server_tmp.password,
                                         server_tmp.nick1,
                                         server_tmp.nick2,
                                         server_tmp.nick3,
                                         server_tmp.username,
                                         server_tmp.realname,
                                         server_tmp.hostname,
                                         server_tmp.command,
                                         1, /* command_delay */
                                         server_tmp.autojoin,
                                         1, /* autorejoin */
                                         NULL);
            if (new_server)
            {
                gui_chat_printf_info (NULL, _("Server %s%s%s created\n"),
                                      GUI_COLOR(GUI_COLOR_CHAT_SERVER),
                                      server_tmp.name,
                                      GUI_COLOR(GUI_COLOR_CHAT));
            }
            else
            {
                gui_chat_printf_error (NULL,
                                       _("%s unable to create server\n"),
                                       WEECHAT_ERROR);
                irc_server_free_data (&server_tmp);
                return -1;
            }
            
            if (new_server->autoconnect)
                irc_server_connect (new_server, 0);
            
            irc_server_free_data (&server_tmp);
        }
        else if (weechat_strcasecmp (argv[0], "copy") == 0)
        {
            if (argc < 3)
            {
                gui_chat_printf_error (NULL,
                                       _("%s missing server name for \"%s\" "
                                         "command\n"),
                                       WEECHAT_ERROR, "server copy");
                return -1;
            }
            
            /* look for server by name */
            server_found = irc_server_search (argv[1]);
            if (!server_found)
            {
                gui_chat_printf_error (NULL,
                                       _("%s server \"%s\" not found for "
                                         "\"%s\" command\n"),
                                       WEECHAT_ERROR, argv[1], "server copy");
                return -1;
            }
            
            /* check if target name already exists */
            if (irc_server_search (argv[2]))
            {
                gui_chat_printf_error (NULL,
                                       _("%s server \"%s\" already exists for "
                                         "\"%s\" command\n"),
                                       WEECHAT_ERROR, argv[2], "server copy");
                return -1;
            }
            
            /* duplicate server */
            new_server = irc_server_duplicate (server_found, argv[2]);
            if (new_server)
            {
                gui_chat_printf_info (NULL,
                                      _("Server %s%s%s has been copied to "
                                        "%s%s\n"),
                                      GUI_COLOR(GUI_COLOR_CHAT_SERVER),
                                      argv[1],
                                      GUI_COLOR(GUI_COLOR_CHAT),
                                      GUI_COLOR(GUI_COLOR_CHAT_SERVER),
                                      argv[2]);
                gui_window_redraw_all_buffers ();
                return 0;
            }
            
            return -1;
        }
        else if (weechat_strcasecmp (argv[0], "rename") == 0)
        {
            if (argc < 3)
            {
                gui_chat_printf_error (NULL,
                                       _("%s missing server name for \"%s\" "
                                         "command\n"),
                                       WEECHAT_ERROR, "server rename");
                return -1;
            }
            
            /* look for server by name */
            server_found = irc_server_search (argv[1]);
            if (!server_found)
            {
                gui_chat_printf_error (NULL,
                                       _("%s server \"%s\" not found for "
                                         "\"%s\" command\n"),
                                       WEECHAT_ERROR, argv[1],
                                       "server rename");
                return -1;
            }
            
            /* check if target name already exists */
            if (irc_server_search (argv[2]))
            {
                gui_chat_printf_error (NULL,
                                       _("%s server \"%s\" already exists for "
                                         "\"%s\" command\n"),
                                       WEECHAT_ERROR, argv[2],
                                       "server rename");
                return -1;
            }

            /* rename server */
            if (irc_server_rename (server_found, argv[2]))
            {
                gui_chat_printf_info (NULL,
                                      _("Server %s%s%s has been renamed to "
                                        "%s%s\n"),
                                      GUI_COLOR(GUI_COLOR_CHAT_SERVER),
                                      argv[1],
                                      GUI_COLOR(GUI_COLOR_CHAT),
                                      GUI_COLOR(GUI_COLOR_CHAT_SERVER),
                                      argv[2]);
                gui_window_redraw_all_buffers ();
                return 0;
            }
            
            return -1;
        }
        else if (weechat_strcasecmp (argv[0], "keep") == 0)
        {
            if (argc < 2)
            {
                gui_chat_printf_error (NULL,
                                       _("%s missing server name for \"%s\" "
                                         "command\n"),
                                       WEECHAT_ERROR, "server keep");
                return -1;
            }
            
            /* look for server by name */
            server_found = irc_server_search (argv[1]);
            if (!server_found)
            {
                gui_chat_printf_error (NULL,
                                       _("%s server \"%s\" not found for "
                                         "\"%s\" command\n"),
                                       WEECHAT_ERROR, argv[1], "server keep");
                return -1;
            }

            /* check that it is temporary server */
            if (!server_found->temp_server)
            {
                gui_chat_printf_error (NULL,
                                       _("%s server \"%s\" is not a temporary "
                                         "server\n"),
                                       WEECHAT_ERROR, argv[1]);
                return -1;
            }
            
            /* remove temporary flag on server */
            server_found->temp_server = 0;

            gui_chat_printf_info (NULL,
                                  _("Server %s%s%s is not temporary any "
                                    "more\n"),
                                  GUI_COLOR(GUI_COLOR_CHAT_SERVER),
                                  argv[1],
                                  GUI_COLOR(GUI_COLOR_CHAT));
            
            return 0;
        }
        else if (weechat_strcasecmp (argv[0], "del") == 0)
        {
            if (argc < 2)
            {
                gui_chat_printf_error (NULL,
                                       _("%s missing server name for \"%s\" "
                                         "command\n"),
                                       WEECHAT_ERROR, "server del");
                return -1;
            }
            
            /* look for server by name */
            server_found = irc_server_search (argv[1]);
            if (!server_found)
            {
                gui_chat_printf_error (NULL,
                                       _("%s server \"%s\" not found for "
                                         "\"%s\" command\n"),
                                       WEECHAT_ERROR, argv[1], "server del");
                return -1;
            }
            if (server_found->is_connected)
            {
                gui_chat_printf_error (NULL,
                                       _("%s you can not delete server \"%s\" "
                                         "because you are connected to. "
                                         "Try /disconnect %s before.\n"),
                                       WEECHAT_ERROR, argv[1], argv[1]);
                return -1;
            }
            
            for (ptr_buffer = gui_buffers; ptr_buffer;
                 ptr_buffer = ptr_buffer->next_buffer)
            {
                if ((ptr_buffer->protocol == irc_protocol)
                    && (IRC_BUFFER_SERVER(ptr_buffer) == server_found))
                {
                    IRC_BUFFER_SERVER(ptr_buffer) = NULL;
                    IRC_BUFFER_CHANNEL(ptr_buffer) = NULL;
                }
            }
            
            server_name = strdup (server_found->name);
            
            irc_server_free (server_found);
            
            gui_chat_printf_info (NULL, _("Server %s%s%s has been deleted\n"),
                                  GUI_COLOR(GUI_COLOR_CHAT_SERVER),
                                  server_name,
                                  GUI_COLOR(GUI_COLOR_CHAT));
            if (server_name)
                free (server_name);
            
            gui_window_redraw_buffer (window->buffer);
            
            return 0;
        }
        else if (weechat_strcasecmp (argv[0], "deloutq") == 0)
        {
            for (ptr_server = irc_servers; ptr_server;
                 ptr_server = ptr_server->next_server)
            {
                irc_server_outqueue_free_all (ptr_server);
            }
            gui_chat_printf_info_nolog (NULL,
                                        _("Messages outqueue DELETED for all "
                                          "servers. Some messages from you or "
                                          "WeeChat may have been lost!\n"));
            return 0;
        }
        else
        {
            gui_chat_printf_error (NULL,
                                   _("%s unknown option for \"%s\" command\n"),
                                   WEECHAT_ERROR, "server");
            return -1;
        }
    }
    return 0;
}

/*
 * irc_command_service: register a new service
 */

int
irc_command_service (t_gui_window *window,
                     char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    irc_server_sendf (ptr_server, "SERVICE %s", arguments);
    return 0;
}

/*
 * irc_command_servlist: list services currently connected to the network
 */

int
irc_command_servlist (t_gui_window *window,
                      char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (arguments)
        irc_server_sendf (ptr_server, "SERVLIST %s", arguments);
    else
        irc_server_sendf (ptr_server, "SERVLIST");
    return 0;
}

/*
 * irc_command_squery: deliver a message to a service
 */

int
irc_command_squery (t_gui_window *window,
                    char *arguments, int argc, char **argv)
{
    char *pos;
    
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
        {
            pos++;
        }
        irc_server_sendf (ptr_server, "SQUERY %s :%s", arguments, pos);
    }
    else
        irc_server_sendf (ptr_server, "SQUERY %s", arguments);
    
    return 0;
}

/*
 * irc_command_squit: disconnect server links
 */

int
irc_command_squit (t_gui_window *window,
                   char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    irc_server_sendf (ptr_server, "SQUIT %s", arguments);
    return 0;
}

/*
 * irc_command_stats: query statistics about server
 */

int
irc_command_stats (t_gui_window *window,
                   char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (arguments)
        irc_server_sendf (ptr_server, "STATS %s", arguments);
    else
        irc_server_sendf (ptr_server, "STATS");
    return 0;
}

/*
 * irc_command_summon: give users who are on a host running an IRC server
 *                      a message asking them to please join IRC
 */

int
irc_command_summon (t_gui_window *window,
                    char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    irc_server_sendf (ptr_server, "SUMMON %s", arguments);
    return 0;
}

/*
 * irc_command_time: query local time from server
 */

int
irc_command_time (t_gui_window *window,
                  char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (arguments)
        irc_server_sendf (ptr_server, "TIME %s", arguments);
    else
        irc_server_sendf (ptr_server, "TIME");
    return 0;
}

/*
 * irc_command_topic: get/set topic for a channel
 */

int
irc_command_topic (t_gui_window *window,
                   char *arguments, int argc, char **argv)
{
    char *channel_name, *new_topic, *pos;
    
    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    channel_name = NULL;
    new_topic = NULL;
    
    if (arguments)
    {
        if (irc_channel_is_channel (arguments))
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
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
            channel_name = ptr_channel->name;
        else
        {
            gui_chat_printf_error_nolog (ptr_server->buffer,
                                         _("%s \"%s\" command can only be "
                                           "executed in a channel buffer\n"),
                                         WEECHAT_ERROR, "topic");
            return -1;
        }
    }
    
    if (new_topic)
    {
        if (strcmp (new_topic, "-delete") == 0)
            irc_server_sendf (ptr_server, "TOPIC %s :",
                              channel_name);
        else
            irc_server_sendf (ptr_server, "TOPIC %s :%s",
                              channel_name, new_topic);
    }
    else
        irc_server_sendf (ptr_server, "TOPIC %s",
                          channel_name);
    
    return 0;
}

/*
 * irc_command_trace: find the route to specific server
 */

int
irc_command_trace (t_gui_window *window,
                   char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (arguments)
        irc_server_sendf (ptr_server, "TRACE %s", arguments);
    else
        irc_server_sendf (ptr_server, "TRACE");
    return 0;
}

/*
 * irc_command_unban: unbans nicks or hosts
 */

int
irc_command_unban (t_gui_window *window,
                   char *arguments, int argc, char **argv)
{
    char *pos_channel, *pos, *pos2;
    
    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (arguments)
    {
        pos_channel = NULL;
        pos = strchr (arguments, ' ');
        if (pos)
        {
            pos[0] = '\0';
            
            if (irc_channel_is_channel (arguments))
            {
                pos_channel = arguments;
                pos++;
                while (pos[0] == ' ')
                    pos++;
            }
            else
            {
                pos[0] = ' ';
                pos = arguments;
            }
        }
        else
            pos = arguments;
        
        /* channel not given, use default buffer */
        if (!pos_channel)
        {
            if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
                pos_channel = ptr_channel->name;
            else
            {
                gui_chat_printf_error_nolog (ptr_server->buffer,
                                             _("%s \"%s\" command can only be "
                                               "executed in a channel "
                                               "buffer\n"),
                                             WEECHAT_ERROR, "unban");
                return -1;
            }
        }
        
        /* loop on users */
        while (pos && pos[0])
        {
            pos2 = strchr (pos, ' ');
            if (pos2)
            {
                pos2[0] = '\0';
                pos2++;
                while (pos2[0] == ' ')
                    pos2++;
            }
            irc_server_sendf (ptr_server, "MODE %s -b %s",
                              pos_channel, pos);
            pos = pos2;
        }
    }
    else
    {
        gui_chat_printf_error_nolog (ptr_server->buffer,
                                     _("%s wrong argument count for \"%s\" "
                                       "command\n"),
                                     WEECHAT_ERROR, "unban");
        return -1;
    }
    return 0;
}

/*
 * irc_command_userhost: return a list of information about nicknames
 */

int
irc_command_userhost (t_gui_window *window,
                      char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    irc_server_sendf (ptr_server, "USERHOST %s", arguments);
    return 0;
}

/*
 * irc_command_users: list of users logged into the server
 */

int
irc_command_users (t_gui_window *window,
                   char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (arguments)
        irc_server_sendf (ptr_server, "USERS %s", arguments);
    else
        irc_server_sendf (ptr_server, "USERS");
    return 0;
}

/*
 * irc_command_version: gives the version info of nick or server (current or specified)
 */

int
irc_command_version (t_gui_window *window,
                     char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (arguments)
    {
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
            && irc_nick_search (ptr_channel, arguments))
            irc_server_sendf (ptr_server, "PRIVMSG %s :\01VERSION\01",
                              arguments);
        else
            irc_server_sendf (ptr_server, "VERSION %s",
                              arguments);
    }
    else
    {
        gui_chat_printf_info (ptr_server->buffer,
                              _("%s, compiled on %s %s\n"),
                              PACKAGE_STRING,
                              __DATE__, __TIME__);
        irc_server_sendf (ptr_server, "VERSION");
    }
    return 0;
}

/*
 * irc_command_voice: give voice to nickname(s)
 */

int
irc_command_voice (t_gui_window *window,
                   char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) arguments;
    
    if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
    {
        if (argc == 0)
            irc_server_sendf (ptr_server, "MODE %s +v %s",
                              ptr_channel->name,
                              ptr_server->nick);
        else
            irc_command_mode_nicks (ptr_server, ptr_channel->name,
                                    "+", "v", argc, argv);
    }
    else
    {
        gui_chat_printf_error_nolog (ptr_server->buffer,
                                     _("%s \"%s\" command can only be "
                                       "executed in a channel buffer\n"),
                                     WEECHAT_ERROR, "voice");
        return -1;
    }
    return 0;
}

/*
 * irc_command_wallops: send a message to all currently connected users who
 *                       have set the 'w' user mode for themselves
 */

int
irc_command_wallops (t_gui_window *window,
                     char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    irc_server_sendf (ptr_server, "WALLOPS :%s", arguments);
    return 0;
}

/*
 * irc_command_who: generate a query which returns a list of information
 */

int
irc_command_who (t_gui_window *window,
                 char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    if (arguments)
        irc_server_sendf (ptr_server, "WHO %s", arguments);
    else
        irc_server_sendf (ptr_server, "WHO");
    return 0;
}

/*
 * irc_command_whois: query information about user(s)
 */

int
irc_command_whois (t_gui_window *window,
                   char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    irc_server_sendf (ptr_server, "WHOIS %s", arguments);
    return 0;
}

/*
 * irc_command_whowas: ask for information about a nickname which no longer exists
 */

int
irc_command_whowas (t_gui_window *window,
                    char *arguments, int argc, char **argv)
{
    IRC_BUFFER_GET_SERVER(window->buffer);
    if (!ptr_server || !ptr_server->is_connected)
        return -1;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    irc_server_sendf (ptr_server, "WHOWAS %s", arguments);
    return 0;
}

/*
 * irc_command_init: init IRC commands (create hooks)
 */

void
irc_command_init ()
{
    weechat_hook_command ("admin",
                          N_("find information about the administrator of the "
                             "server"),
                          N_("[target]"),
                          N_("target: server"),
                          NULL, irc_command_admin);
    weechat_hook_command ("ame",
                          N_("send a CTCP action to all channels of all "
                             "connected servers"),
                          N_("message"),
                          N_("message: message to send"),
                          NULL, irc_command_ame);
    weechat_hook_command ("amsg",
                          N_("send message to all channels of all connected "
                             "servers"),
                          N_("text"),
                          N_("text: text to send"),
                          NULL, irc_command_amsg);
    weechat_hook_command ("away",
                          N_("toggle away status"),
                          N_("[-all] [message]"),
                          N_("   -all: toggle away status on all connected "
                             "servers\n"
                             "message: message for away (if no message is "
                             "given, away status is removed)"),
                          "-all", irc_command_away);
    weechat_hook_command ("ban",
                          N_("bans nicks or hosts"),
                          N_("[channel] [nickname [nickname ...]]"),
                          N_(" channel: channel for ban\n"
                             "nickname: user or host to ban"),
                          "%N", irc_command_ban);
    weechat_hook_command ("connect",
                          N_("connect to server(s)"),
                          N_("[-all [-nojoin] | servername [servername ...] "
                             "[-nojoin] | hostname [-port port] [-ipv6] "
                             "[-ssl]]"),
                          N_("      -all: connect to all servers\n"
                             "servername: internal server name to connect\n"
                             "   -nojoin: do not join any channel (even if "
                             "autojoin is enabled on server)\n"
                             "  hostname: hostname to connect, creating "
                             "temporary server\n"
                             "      port: port for server (integer, default "
                             "is 6667)\n"
                             "      ipv6: use IPv6 protocol\n"
                             "       ssl: use SSL protocol"),
                          "%S|-all|-nojoin|%*", irc_command_connect);
    weechat_hook_command ("ctcp",
                          N_("send a CTCP message (Client-To-Client Protocol)"),
                          N_("receiver type [arguments]"),
                          N_(" receiver: nick or channel to send CTCP to\n"
                             "     type: CTCP type (examples: \"version\", "
                             "\"ping\", ..)\n"
                             "arguments: arguments for CTCP"),
                          "%c|%n action|ping|version", irc_command_ctcp);
    weechat_hook_command ("cycle",
                          N_("leave and rejoin a channel"),
                          N_("[channel[,channel]] [part_message]"),
                          N_("     channel: channel name for cycle\n"
                             "part_message: part message (displayed to other "
                             "users)"),
                          "%p", irc_command_cycle);
    weechat_hook_command ("dcc",
                          N_("starts DCC (file or chat) or close chat"),
                          N_("action [nickname [file]]"),
                          N_("  action: 'send' (file) or 'chat' or 'close' "
                             "(chat)\n"
                             "nickname: nickname to send file or chat\n"
                             "    file: filename (on local host)"),
                          "chat|send|close %n %f", irc_command_dcc);
    weechat_hook_command ("dehalfop",
                          N_("removes half channel operator status from "
                             "nickname(s)"),
                          N_("[nickname [nickname]]"),
                          "",
                          NULL, irc_command_dehalfop);
    weechat_hook_command ("deop",
                          N_("removes channel operator status from "
                             "nickname(s)"),
                          N_("[nickname [nickname]]"),
                          "",
                          NULL, irc_command_deop);
    weechat_hook_command ("devoice",
                          N_("removes voice from nickname(s)"),
                          N_("[nickname [nickname]]"),
                          "",
                          NULL, irc_command_devoice);
    weechat_hook_command ("die",
                          N_("shutdown the server"),
                          "",
                          "",
                          NULL, irc_command_die);
    weechat_hook_command ("disconnect",
                          N_("disconnect from server(s)"),
                          N_("[-all | servername [servername ...]]"),
                          N_("      -all: disconnect from all servers\n"
                             "servername: server name to disconnect"),
                          "%S|-all", irc_command_disconnect);
    weechat_hook_command ("halfop",
                          N_("gives half channel operator status to "
                             "nickname(s)"),
                          N_("[nickname [nickname]]"),
                          "",
                          NULL, irc_command_halfop);
    weechat_hook_command ("info",
                          N_("get information describing the server"),
                          N_("[target]"),
                          N_("target: server name"),
                          NULL, irc_command_info);
    weechat_hook_command ("invite",
                          N_("invite a nick on a channel"),
                          N_("nickname channel"),
                          N_("nickname: nick to invite\n"
                             " channel: channel to invite"),
                          "%n %c", irc_command_invite);
    weechat_hook_command ("ison",
                          N_("check if a nickname is currently on IRC"),
                          N_("nickname [nickname ...]"),
                          N_("nickname: nickname"),
                          NULL, irc_command_ison);
    weechat_hook_command ("join",
                          N_("join a channel"),
                          N_("channel[,channel] [key[,key]]"),
                          N_("channel: channel name to join\n"
                             "    key: key to join the channel"),
                          "%C", irc_command_join);
    weechat_hook_command ("kick",
                          N_("forcibly remove a user from a channel"),
                          N_("[channel] nickname [comment]"),
                          N_(" channel: channel where user is\n"
                             "nickname: nickname to kick\n"
                             " comment: comment for kick"),
                          "%n %-", irc_command_kick);
    weechat_hook_command ("kickban",
                          N_("kicks and bans a nick from a channel"),
                          N_("[channel] nickname [comment]"),
                          N_(" channel: channel where user is\n"
                             "nickname: nickname to kick and ban\n"
                             " comment: comment for kick"),
                          "%n %-", irc_command_kickban);
    weechat_hook_command ("kill",
                          N_("close client-server connection"),
                          N_("nickname comment"),
                          N_("nickname: nickname\n"
                             " comment: comment for kill"),
                          "%n %-", irc_command_kill);
    weechat_hook_command ("links",
                          N_("list all servernames which are known by the "
                             "server answering the query"),
                          N_("[[server] server_mask]"),
                          N_("     server: this server should answer the "
                             "query\n"
                             "server_mask: list of servers must match this "
                             "mask"),
                          NULL, irc_command_links);
    weechat_hook_command ("list",
                          N_("list channels and their topic"),
                          N_("[channel[,channel] [server]]"),
                          N_("channel: channel to list (a regexp is allowed)\n"
                             "server: server name"),
                          NULL, irc_command_list);
    weechat_hook_command ("lusers",
                          N_("get statistics about the size of the IRC "
                             "network"),
                          N_("[mask [target]]"),
                          N_("  mask: servers matching the mask only\n"
                             "target: server for forwarding request"),
                          NULL, irc_command_lusers);
    weechat_hook_command ("me",
                          N_("send a CTCP action to the current channel"),
                          N_("message"),
                          N_("message: message to send"),
                          NULL, irc_command_me);
    weechat_hook_command ("mode",
                          N_("change channel or user mode"),
                          N_("{ channel {[+|-]|o|p|s|i|t|n|b|v} [limit] "
                             "[user] [ban mask] } | { nickname "
                             "{[+|-]|i|w|s|o} }"),
                          N_("channel modes:\n"
                             "  channel: channel name to modify\n"
                             "  o: give/take channel operator privileges\n"
                             "  p: private channel flag\n"
                             "  s: secret channel flag\n"
                             "  i: invite-only channel flag\n"
                             "  t: topic settable by channel operator only "
                             "flag\n"
                             "  n: no messages to channel from clients on the "
                             "outside\n"
                             "  m: moderated channel\n"
                             "  l: set the user limit to channel\n"
                             "  b: set a ban mask to keep users out\n"
                             "  e: set exception mask\n"
                             "  v: give/take the ability to speak on a "
                             "moderated channel\n"
                             "  k: set a channel key (password)\n"
                             "user modes:\n"
                             "  nickname: nickname to modify\n"
                             "  i: mark a user as invisible\n"
                             "  s: mark a user for receive server notices\n"
                             "  w: user receives wallops\n"
                             "  o: operator flag"),
                          "%c|%m", irc_command_mode);
    weechat_hook_command ("motd",
                          N_("get the \"Message Of The Day\""),
                          N_("[target]"),
                          N_("target: server name"),
                          NULL, irc_command_motd);
    weechat_hook_command ("msg",
                          N_("send message to a nick or channel"),
                          N_("receiver[,receiver] text"),
                          N_("receiver: nick or channel (may be mask, '*' = "
                             "current channel)\n"
                             "text: text to send"),
                          NULL, irc_command_msg);
    weechat_hook_command ("names",
                          N_("list nicknames on channels"),
                          N_("[channel[,channel]]"),
                          N_("channel: channel name"),
                          "%C|%*", irc_command_names);
    weechat_hook_command ("nick",
                          N_("change current nickname"),
                          N_("[-all] nickname"),
                          N_("    -all: set new nickname for all connected "
                             "servers\n"
                             "nickname: new nickname"),
                          "-all", irc_command_nick);
    weechat_hook_command ("notice",
                          N_("send notice message to user"),
                          N_("nickname text"),
                          N_("nickname: user to send notice to\n"
                             "    text: text to send"),
                          "%n %-", irc_command_notice);
    weechat_hook_command ("op",
                          N_("gives channel operator status to nickname(s)"),
                          N_("nickname [nickname]"),
                          "",
                          NULL, irc_command_op);
    weechat_hook_command ("oper",
                          N_("get operator privileges"),
                          N_("user password"),
                          N_("user/password: used to get privileges on "
                             "current IRC server"),
                          NULL, irc_command_oper);
    weechat_hook_command ("part",
                          N_("leave a channel"),
                          N_("[channel[,channel]] [part_message]"),
                          N_("     channel: channel name to leave\n"
                             "part_message: part message (displayed to other "
                             "users)"),
                          "%p", irc_command_part);
    weechat_hook_command ("ping",
                          N_("ping server"),
                          N_("server1 [server2]"),
                          N_("server1: server to ping\nserver2: forward ping "
                             "to this server"),
                          NULL, irc_command_ping);
    weechat_hook_command ("pong",
                          N_("answer to a ping message"),
                          N_("daemon [daemon2]"),
                          N_(" daemon: daemon who has responded to Ping "
                             "message\n"
                             "daemon2: forward message to this daemon"),
                          NULL, irc_command_pong);
    weechat_hook_command ("query",
                          N_("send a private message to a nick"),
                          N_("nickname [text]"),
                          N_("nickname: nickname for private conversation\n"
                             "    text: text to send"),
                          "%n %-", irc_command_query);
    weechat_hook_command ("quit",
                          N_("close all connections and quit"),
                          N_("[quit_message]"),
                          N_("quit_message: quit message (displayed to other "
                             "users)"),
                          "%q", irc_command_quit);
    weechat_hook_command ("quote",
                          N_("send raw data to server without parsing"),
                          N_("data"),
                          N_("data: raw data to send"),
                          NULL, irc_command_quote);
    weechat_hook_command ("reconnect",
                          N_("reconnect to server(s)"),
                          N_("[-all [-nojoin] | servername [servername ...] "
                             "[-nojoin]]"),
                          N_("      -all: reconnect to all servers\n"
                             "servername: server name to reconnect\n"
                             "   -nojoin: do not join any channel (even if "
                             "autojoin is enabled on server)"),
                          "%S|-all|-nojoin|%*", irc_command_reconnect);
    weechat_hook_command ("rehash",
                          N_("tell the server to reload its config file"),
                          "",
                          "",
                          NULL, irc_command_rehash);
    weechat_hook_command ("restart",
                          N_("tell the server to restart itself"),
                          "",
                          "",
                          NULL, irc_command_restart);
    weechat_hook_command ("service",
                          N_("register a new service"),
                          N_("nickname reserved distribution type reserved "
                             "info"),
                          N_("distribution: visibility of service\n"
                             "        type: reserved for future usage"),
                          NULL, irc_command_service);
    weechat_hook_command ("server",
                          N_("list, add or remove servers"),
                          N_("[list [servername]] | [listfull [servername]] | "
                             "[add servername hostname [-port port] [-temp] "
                             "[-auto | -noauto] [-ipv6] [-ssl] [-pwd password] "
                             "[-nicks nick1 nick2 nick3] [-username username] "
                             "[-realname realname] [-command command] "
                             "[-autojoin channel[,channel]] ] | [copy "
                             "servername newservername] | [rename servername "
                             "newservername] | [keep servername] | [del "
                             "servername]"),
                          N_("      list: list servers (no parameter implies "
                             "this list)\n"
                             "  listfull: list servers with detailed info for "
                             "each server\n"
                             "       add: create a new server\n"
                             "servername: server name, for internal and "
                             "display use\n"
                             "  hostname: name or IP address of server\n"
                             "      port: port for server (integer, default "
                             "is 6667)\n"
                             "      temp: create temporary server (not saved "
                             "in config file)\n"
                             "      auto: automatically connect to server "
                             "when WeeChat starts\n"
                             "    noauto: do not connect to server when "
                             "WeeChat starts (default)\n"
                             "      ipv6: use IPv6 protocol\n"
                             "       ssl: use SSL protocol\n"
                             "  password: password for server\n"
                             "     nick1: first nick for server\n"
                             "     nick2: alternate nick for server\n"
                             "     nick3: second alternate nick for server\n"
                             "  username: user name\n"
                             "  realname: real name of user\n"
                             "      copy: duplicate a server\n"
                             "    rename: rename a server\n"
                             "      keep: keep server in config file (for "
                             "temporary servers only)\n"
                             "       del: delete a server\n"
                             "   deloutq: delete messages out queue for all "
                             "servers (all messages "
                             "WeeChat is currently sending)"),
                          "add|copy|rename|keep|del|deloutq|list|listfull %S %S",
                          irc_command_server);
    weechat_hook_command ("servlist",
                          N_("list services currently connected to the "
                             "network"),
                          N_("[mask [type]]"),
                          N_("mask: list only services matching this mask\n"
                             "type: list only services of this type"),
                          NULL, irc_command_servlist);
    weechat_hook_command ("squery",
                          N_("deliver a message to a service"),
                          N_("service text"),
                          N_("service: name of service\ntext: text to send"),
                          NULL, irc_command_squery);
    weechat_hook_command ("squit",
                          N_("disconnect server links"),
                          N_("server comment"),
                          N_( "server: server name\n"
                              "comment: comment for quit"),
                          NULL, irc_command_squit);
    weechat_hook_command ("stats",
                          N_("query statistics about server"),
                          N_("[query [server]]"),
                          N_(" query: c/h/i/k/l/m/o/y/u (see RFC1459)\n"
                             "server: server name"),
                          NULL, irc_command_stats);
    weechat_hook_command ("summon",
                          N_("give users who are on a host running an IRC "
                             "server a message asking them to please join "
                             "IRC"),
                          N_("user [target [channel]]"),
                          N_("   user: username\ntarget: server name\n"
                             "channel: channel name"),
                          NULL, irc_command_summon);
    weechat_hook_command ("time",
                          N_("query local time from server"),
                          N_("[target]"),
                          N_("target: query time from specified server"),
                          NULL, irc_command_time);
    weechat_hook_command ("topic",
                          N_("get/set channel topic"),
                          N_("[channel] [topic]"),
                          N_("channel: channel name\ntopic: new topic for "
                             "channel (if topic is \"-delete\" then topic "
                             "is deleted)"),
                          "%t|-delete %-", irc_command_topic);
    weechat_hook_command ("trace",
                          N_("find the route to specific server"),
                          N_("[target]"),
                          N_("target: server"),
                          NULL, irc_command_trace);
    weechat_hook_command ("unban",
                          N_("[channel] nickname [nickname ...]"),
                          N_(" channel: channel for unban\n"
                             "nickname: user or host to unban"),
                          NULL, irc_command_unban);
    weechat_hook_command ("userhost",
                          N_("return a list of information about nicknames"),
                          N_("nickname [nickname ...]"),
                          N_("nickname: nickname"),
                          "%n", irc_command_userhost);
    weechat_hook_command ("users",
                          N_("list of users logged into the server"),
                          N_("[target]"),
                          N_("target: server"),
                          NULL, irc_command_users);
    weechat_hook_command ("version",
                          N_("gives the version info of nick or server "
                             "(current or specified)"),
                          N_("[server | nickname]"),
                          N_("  server: server name\n"
                             "nickname: nickname"),
                          "%n", irc_command_version);
    weechat_hook_command ("voice",
                          N_("gives voice to nickname(s)"),
                          N_("[nickname [nickname]]"),
                          "",
                          NULL, irc_command_voice);
    weechat_hook_command ("wallops",
                          N_("send a message to all currently connected users "
                             "who have set the 'w' user mode for themselves"),
                          N_("text"),
                          N_("text to send"),
                          NULL, irc_command_wallops);
    weechat_hook_command ("who",
                          N_("generate a query which returns a list of "
                             "information"),
                          N_("[mask [\"o\"]]"),
                          N_("mask: only information which match this mask\n"
                             "   o: only operators are returned according to "
                             "the mask supplied"),
                          "%C", irc_command_who);
    weechat_hook_command ("whois",
                          N_("query information about user(s)"),
                          N_("[server] nickname[,nickname]"),
                          N_("  server: server name\n"
                             "nickname: nickname (may be a mask)"),
                          NULL, irc_command_whois);
    weechat_hook_command ("whowas",
                          N_("ask for information about a nickname which no "
                             "longer exists"),
                          N_("nickname [,nickname [,nickname ...]] [count "
                             "[target]]"),
                          N_("nickname: nickname to search\n"
                             "   count: number of replies to return "
                             "(full search if negative number)\n"
                             "  target: reply should match this mask"),
                          NULL, irc_command_whowas);
}
