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

/* irc-recv.c: implementation of IRC commands (server to client),
               according to RFC 1459,2810,2811,2812 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/utsname.h>

#include "../common/weechat.h"
#include "irc.h"
#include "../common/command.h"
#include "../common/weeconfig.h"
#include "../gui/gui.h"
#include "../plugins/plugins.h"


/*
 * irc_recv_command: executes action when receiving IRC command
 *                   returns:  0 = all ok, command executed
 *                            -1 = command failed
 *                            -2 = no command to execute
 *                            -3 = command not found
 */

int
irc_recv_command (t_irc_server *server, char *entire_line,
                  char *host, char *command, char *arguments)
{
    int i, cmd_found, return_code;

    if (command == NULL)
        return -2;

    /* look for IRC command */
    cmd_found = -1;
    for (i = 0; irc_commands[i].command_name; i++)
    {
        if (strcasecmp (irc_commands[i].command_name, command) == 0)
        {
            cmd_found = i;
            break;
        }
    }

    /* command not found */
    if (cmd_found < 0)
        return -3;

    if (irc_commands[i].recv_function != NULL)
    {
        return_code = (int) (irc_commands[i].recv_function) (server, host, arguments);
        plugin_event_msg (irc_commands[i].command_name, entire_line);
        return return_code;
    }
    
    return 0;
}

/*
 * irc_cmd_recv_error: error received from server
 */

int
irc_cmd_recv_error (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos2;
    int first;
    
    /* make gcc happy */
    (void) server;
    (void) host;
    
    if (strncmp (arguments, "Closing Link", 12) == 0)
    {
        server_disconnect (server);
        return 0;
    }
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
    }
    else
        pos = arguments;
    
    irc_display_prefix (server->window, PREFIX_ERROR);
    first = 1;
    
    while (pos && pos[0])
    {
        pos2 = strchr (pos, ' ');
        if ((pos[0] == ':') || (!pos2))
        {
            if (pos[0] == ':')
                pos++;
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT,
                              "%s%s\n", (first) ? "" : ": ", pos);
            pos = NULL;
        }
        else
        {
            pos2[0] = '\0';
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_CHANNEL,
                              "%s%s",
                              (first) ? "" : " ", pos);
            first = 0;
            pos = pos2 + 1;
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_join: 'join' message received
 */

int
irc_cmd_recv_join (t_irc_server *server, char *host, char *arguments)
{
    t_irc_channel *ptr_channel;
    char *pos;
    
    ptr_channel = channel_search (server, arguments);
    if (!ptr_channel)
    {
        ptr_channel = channel_new (server, CHAT_CHANNEL, arguments, 1);
        if (!ptr_channel)
        {
            gui_printf (server->window,
                        _("%s cannot create new channel \"%s\"\n"),
                        WEECHAT_ERROR, arguments);
            return -1;
        }
    }
    
    pos = strchr (host, '!');
    if (pos)
        pos[0] = '\0';
    
    irc_display_prefix (ptr_channel->window, PREFIX_JOIN);
    gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_NICK,
                      "%s ", host);
    gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_DARK,
                      "(");
    gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_HOST,
                      "%s", pos + 1);
    gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_DARK,
                      ")");
    gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT,
                      _(" has joined "));
    gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_CHANNEL,
                      "%s\n", arguments);
    (void) nick_new (ptr_channel, host, 0, 0, 0);
    gui_redraw_window_nick (gui_current_window);
    return 0;
}

/*
 * irc_cmd_recv_kick: 'kick' message received
 */

int
irc_cmd_recv_kick (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos_nick, *pos_comment;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
    pos = strchr (host, '!');
    if (pos)
        pos[0] = '\0';
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        pos_nick[0] = '\0';
        pos_nick++;
        while (pos_nick[0] == ' ')
            pos_nick++;
    
        pos_comment = strchr (pos_nick, ' ');
        if (pos_comment)
        {
            pos_comment[0] = '\0';
            pos_comment++;
            while (pos_comment[0] == ' ')
                pos_comment++;
            if (pos_comment[0] == ':')
                pos_comment++;
        }
    
        ptr_channel = channel_search (server, arguments);
        if (!ptr_channel)
        {
            gui_printf (server->window,
                        _("%s channel not found for \"%s\" command\n"),
                        WEECHAT_ERROR, "kick");
            return -1;
        }
    
        irc_display_prefix (ptr_channel->window, PREFIX_PART);
        gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_NICK,
                          "%s", host);
        gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT,
                          _(" has kicked "));
        gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_NICK,
                          "%s", pos_nick);
        gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT,
                          _(" from "));
        if (pos_comment)
        {
            gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_CHANNEL,
                              "%s ", arguments);
            gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_DARK,
                              "(");
            gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT,
                              "%s", pos_comment);
            gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_DARK,
                              ")\n");
        }
        else
            gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_CHANNEL,
                              "%s\n", arguments);
    }
    else
    {
        gui_printf (server->window,
                    _("%s nick not found for \"%s\" command\n"),
                    WEECHAT_ERROR, "kick");
        return -1;
    }
    ptr_nick = nick_search (ptr_channel, pos_nick);
    if (ptr_nick)
    {
        nick_free (ptr_channel, ptr_nick);
        gui_redraw_window_nick (gui_current_window);
    }
    return 0;
}

/*
 * irc_cmd_recv_mode: 'mode' message received
 */

int
irc_cmd_recv_mode (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos2, *pos_parm;
    char set_flag;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
    /* no host => we can't identify sender of message! */
    if (host == NULL)
    {
        gui_printf (server->window,
                    _("%s \"%s\" command received without host\n"),
                    WEECHAT_ERROR, "mode");
        return -1;
    }
    
    /* keep only nick name from host */
    pos = strchr (host, '!');
    if (pos)
        pos[0] = '\0';
    
    pos = strchr (arguments, ' ');
    if (!pos)
    {
        gui_printf (server->window,
                    _("%s \"%s\" command received without channel or nickname\n"),
                    WEECHAT_ERROR, "mode");
        return -1;
    }
    pos[0] = '\0';
    pos++;
    while (pos[0] == ' ')
        pos++;
    
    pos_parm = strchr (pos, ' ');
    if (pos_parm)
    {
        pos_parm[0] = '\0';
        pos_parm++;
        while (pos_parm[0] == ' ')
            pos_parm++;
        pos2 = strchr (pos_parm, ' ');
        if (pos2)
            pos2[0] = '\0';
    }
    
    set_flag = '+';
    
    if (string_is_channel (arguments))
    {
        ptr_channel = channel_search (server, arguments);
        if (ptr_channel)
        {
            /* channel modes */
            while (pos && pos[0])
            {
                switch (pos[0])
                {
                    case '+':
                        set_flag = '+';
                        break;
                    case '-':
                        set_flag = '-';
                        break;
                    case 'b':
                        irc_display_mode (ptr_channel->window,
                                          arguments, set_flag, "b", host,
                                          (set_flag == '+') ?
                                              _("sets ban on") :
                                              _("removes ban on"),
                                          pos_parm);
                        /* TODO: change & redraw channel modes */
                        break;
                    case 'i':
                        irc_display_mode (ptr_channel->window,
                                          arguments, set_flag, "i", host,
                                          (set_flag == '+') ?
                                              _("sets invite-only channel flag") :
                                              _("removes invite-only channel flag"),
                                          NULL);
                        /* TODO: change & redraw channel modes */
                        break;
                    case 'l':
                        irc_display_mode (ptr_channel->window,
                                          arguments, set_flag, "l", host,
                                          (set_flag == '+') ?
                                              _("sets the user limit to") :
                                              _("removes user limit"),
                                          (set_flag == '+') ? pos_parm : NULL);
                        /* TODO: change & redraw channel modes */
                        break;
                    case 'm':
                        irc_display_mode (ptr_channel->window,
                                          arguments, set_flag, "m", host,
                                          (set_flag == '+') ?
                                              _("sets moderated channel flag") :
                                              _("removes moderated channel flag"),
                                          NULL);
                        /* TODO: change & redraw channel modes */
                        break;
                    case 'o':
                        irc_display_mode (ptr_channel->window,
                                          arguments, set_flag, "o", host,
                                          (set_flag == '+') ?
                                              _("gives channel operator status to") :
                                              _("removes channel operator status from"),
                                          pos_parm);
                        ptr_nick = nick_search (ptr_channel, pos_parm);
                        if (ptr_nick)
                        {
                            ptr_nick->is_op = (set_flag == '+') ? 1 : 0;
                            nick_resort (ptr_channel, ptr_nick);
                            gui_redraw_window_nick (ptr_channel->window);
                        }
                        break;
                    /* TODO: remove this obsolete (?) channel flag? */
                    case 'p':
                        irc_display_mode (ptr_channel->window,
                                          arguments, set_flag, "p", host,
                                          (set_flag == '+') ?
                                              _("sets private channel flag") :
                                              _("removes private channel flag"),
                                          NULL);
                        /* TODO: change & redraw channel modes */
                        break;
                    case 's':
                        irc_display_mode (ptr_channel->window,
                                          arguments, set_flag, "s", host,
                                          (set_flag == '+') ?
                                              _("sets secret channel flag") :
                                              _("removes secret channel flag"),
                                          NULL);
                        /* TODO: change & redraw channel modes */
                        break;
                    case 't':
                        irc_display_mode (ptr_channel->window,
                                          arguments, set_flag, "t", host,
                                          (set_flag == '+') ?
                                              _("sets topic protection") :
                                              _("removes topic protection"),
                                          NULL);
                        /* TODO: change & redraw channel modes */
                        break;
                    case 'v':
                        irc_display_mode (ptr_channel->window,
                                          arguments, set_flag, "v", host,
                                          (set_flag == '+') ?
                                              _("gives voice to") :
                                              _("removes voice from"),
                                          pos_parm);
                        
                        ptr_nick = nick_search (ptr_channel, pos_parm);
                        if (ptr_nick)
                        {
                            ptr_nick->has_voice = (set_flag == '+') ? 1 : 0;
                            nick_resort (ptr_channel, ptr_nick);
                            gui_redraw_window_nick (ptr_channel->window);
                        }
                        break;
                }
                pos++;
            }
        }
        else
        {
            gui_printf (server->window,
                        _("%s channel not found for \"%s\" command\n"),
                        WEECHAT_ERROR, "mode");
            return -1;
        }
    }
    else
    {
        /* nickname modes */
        gui_printf (server->window, "(TODO!) nickname modes: channel=%s, args=%s\n", arguments, pos);
    }
    return 0;
}

/*
 * irc_cmd_recv_nick: 'nick' message received
 */

int
irc_cmd_recv_nick (t_irc_server *server, char *host, char *arguments)
{
    char *pos;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    int nick_is_me;
    
    /* no host => we can't identify sender of message! */
    if (host == NULL)
    {
        gui_printf (server->window,
                    _("%s \"%s\" command received without host\n"),
                    WEECHAT_ERROR, "nick");
        return -1;
    }
    
    /* keep only nick name from host */
    pos = strchr (host, '!');
    if (pos)
        pos[0] = '\0';
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        ptr_nick = nick_search (ptr_channel, host);
        if (ptr_nick)
        {
            nick_is_me = (strcmp (ptr_nick->nick, server->nick) == 0) ? 1 : 0;
            nick_change (ptr_channel, ptr_nick, arguments);
            irc_display_prefix (ptr_channel->window, PREFIX_INFO);
            if (nick_is_me)
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT,
                                  _("You are "));
            else
            {
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_NICK,
                                  "%s", host);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT, _(" is "));
            }
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT,
                              _("now known as "));
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT_NICK,
                              "%s\n",
                              arguments);
            if (gui_window_has_nicklist (ptr_channel->window))
                gui_redraw_window_nick (ptr_channel->window);
        }
    }
    
    if (strcmp (server->nick, host) == 0)
    {
        free (server->nick);
        server->nick = strdup (arguments);
    }
    gui_redraw_window_input (gui_current_window);
    
    return 0;
}

/*
 * irc_cmd_recv_notice: 'notice' message received
 */

int
irc_cmd_recv_notice (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos2, *pos_usec;
    struct timeval tv;
    struct timezone tz;
    long sec1, usec1, sec2, usec2, difftime;
    
    if (host)
    {
        pos = strchr (host, '!');
        if (pos)
            pos[0] = '\0';
    }
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
        if (pos[0] == ':')
            pos++;
    }
    else
    {
        gui_printf (server->window,
                    _("%s nickname not found for \"%s\" command\n"),
                    WEECHAT_ERROR, "notice");
        return -1;
    }
    if (strncmp (pos, "\01VERSION", 8) == 0)
    {
        pos += 9;
        pos2 = strchr (pos, '\01');
        if (pos2)
            pos2[0] = '\0';
        irc_display_prefix (server->window, PREFIX_SERVER);
        gui_printf_color (server->window, COLOR_WIN_CHAT, "CTCP ");
        gui_printf_color (server->window, COLOR_WIN_CHAT_CHANNEL, "VERSION ");
        gui_printf_color (server->window, COLOR_WIN_CHAT, _("reply from"));
        gui_printf_color (server->window, COLOR_WIN_CHAT_NICK, " %s", host);
        gui_printf_color (server->window, COLOR_WIN_CHAT, ": %s\n", pos);
    }
    else
    {
        if (strncmp (pos, "\01PING", 5) == 0)
        {
            pos += 5;
            while (pos[0] == ' ')
                pos++;
            pos_usec = strchr (pos, ' ');
            if (pos_usec)
            {
                pos_usec[0] = '\0';
                pos_usec++;
                pos2 = strchr (pos_usec, '\01');
                if (pos2)
                {
                    pos2[0] = '\0';
                    
                    gettimeofday (&tv, &tz);
                    sec1 = atol (pos);
                    usec1 = atol (pos_usec);
                    sec2 = tv.tv_sec;
                    usec2 = tv.tv_usec;
                    
                    difftime = ((sec2 * 1000000) + usec2) - ((sec1 * 1000000) + usec1);
                    
                    irc_display_prefix (server->window, PREFIX_SERVER);
                    gui_printf_color (server->window, COLOR_WIN_CHAT, "CTCP ");
                    gui_printf_color (server->window, COLOR_WIN_CHAT_CHANNEL, "PING ");
                    gui_printf_color (server->window, COLOR_WIN_CHAT, _("reply from"));
                    gui_printf_color (server->window, COLOR_WIN_CHAT_NICK, " %s", host);
                    gui_printf_color (server->window, COLOR_WIN_CHAT,
                                      _(": %ld.%ld seconds\n"),
                                      difftime / 1000000,
                                      (difftime % 1000000) / 1000);
                }
            }
        }
        else
        {
            irc_display_prefix (server->window, PREFIX_SERVER);
            gui_printf_color (server->window, COLOR_WIN_CHAT, "%s\n", pos);
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_part: 'part' message received
 */

int
irc_cmd_recv_part (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos_args;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
    /* no host => we can't identify sender of message! */
    if (!host || !arguments)
    {
        gui_printf (server->window,
                    _("%s \"%s\" command received without host or channel\n"),
                    WEECHAT_ERROR, "part");
        return -1;
    }
    
    pos_args = strchr (arguments, ' ');
    if (pos_args)
    {
        pos_args[0] = '\0';
        pos_args++;
        while (pos_args[0] == ' ')
            pos_args++;
        if (pos_args[0] == ':')
            pos_args++;
    }
    
    /* keep only nick name from host */
    pos = strchr (host, '!');
    if (pos)
        pos[0] = '\0';
    
    ptr_channel = channel_search (server, arguments);
    if (ptr_channel)
    {
        ptr_nick = nick_search (ptr_channel, host);
        if (ptr_nick)
        {
            if (strcmp (ptr_nick->nick, server->nick) == 0)
            {
                /* part request was issued by local client */
                gui_window_free (ptr_channel->window);
                channel_free (server, ptr_channel);
                gui_redraw_window_status (gui_current_window);
                gui_redraw_window_input (gui_current_window);
            }
            else
            {
            
                /* remove nick from nick list and display message */
                nick_free (ptr_channel, ptr_nick);
                irc_display_prefix (ptr_channel->window, PREFIX_PART);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_NICK, "%s ", host);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_DARK, "(");
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_HOST, "%s", pos+1);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_DARK, ")");
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT, _(" has left "));
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_CHANNEL,
                                  "%s", ptr_channel->name);
                if (pos_args && pos_args[0])
                {
                    gui_printf_color (ptr_channel->window,
                                      COLOR_WIN_CHAT_DARK, " (");
                    gui_printf_color (ptr_channel->window,
                                      COLOR_WIN_CHAT, "%s", pos_args);
                    gui_printf_color (ptr_channel->window,
                                      COLOR_WIN_CHAT_DARK, ")");
                }
                gui_printf (ptr_channel->window, "\n");
            
                /* redraw nick list if this is current window */
                if (gui_window_has_nicklist (ptr_channel->window))
                    gui_redraw_window_nick (ptr_channel->window);
            }
        }
    }
    else
    {
        gui_printf (server->window,
                    _("%s channel not found for \"%s\" command\n"),
                    WEECHAT_ERROR, "part");
        return -1;
    }
    
    return 0;
}

/*
 * irc_cmd_recv_ping: 'ping' command received
 */

int
irc_cmd_recv_ping (t_irc_server *server, char *host, char *arguments)
{
    char *pos;
    
    (void)host;
    pos = strrchr (arguments, ' ');
    if (pos)
        pos[0] = '\0';
    server_sendf (server, "PONG :%s\r\n", arguments);
    return 0;
}

/*
 * irc_cmd_recv_privmsg: 'privmsg' command received
 */

int
irc_cmd_recv_privmsg (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos2, *host2;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    struct utsname *buf;
    
    /* no host => we can't identify sender of message! */
    if (host == NULL)
    {
        gui_printf (server->window,
                    _("%s \"%s\" command received without host\n"),
                    WEECHAT_ERROR, "privmsg");
        return -1;
    }
    
    /* keep only nick name from host */
    pos = strchr (host, '!');
    if (pos)
    {
        pos[0] = '\0';
        host2 = pos+1;
    }
    else
        host2 = host;
    
    /* receiver is a channel ? */
    if (string_is_channel (arguments))
    {
        pos = strchr (arguments, ' ');
        if (pos)
        {
            pos[0] = '\0';
            pos++;
            while (pos[0] == ' ')
                pos++;
            if (pos[0] == ':')
                pos++;
            
            ptr_channel = channel_search (server, arguments);
            if (ptr_channel)
            {
                if (strncmp (pos, "\01ACTION ", 8) == 0)
                {
                    pos += 8;
                    pos2 = strchr (pos, '\01');
                    if (pos2)
                        pos2[0] = '\0';
                    irc_display_prefix (ptr_channel->window, PREFIX_ACTION_ME);
                    if (strstr (pos, server->nick))
                    {
                        gui_printf_color (ptr_channel->window,
                                          COLOR_WIN_CHAT_HIGHLIGHT, "%s", host);
                        if ( (cfg_look_infobar_delay_highlight > 0)
                            && (ptr_channel->window != gui_current_window) )
                            gui_infobar_printf (cfg_look_infobar_delay_highlight,
                                                COLOR_WIN_INFOBAR_HIGHLIGHT,
                                                _("On %s: * %s %s"),
                                                ptr_channel->name,
                                                host, pos);
                    }
                    else
                        gui_printf_color (ptr_channel->window,
                                          COLOR_WIN_CHAT_NICK, "%s", host);
                    gui_printf_color (ptr_channel->window,
                                      COLOR_WIN_CHAT, " %s\n", pos);
                }
                else
                {
                    ptr_nick = nick_search (ptr_channel, host);
                    if (ptr_nick)
                    {
                        if (strstr (pos, server->nick))
                        {
                            irc_display_nick (ptr_channel->window, ptr_nick,
                                              MSG_TYPE_NICK, 1, -1, 0);
                            if ( (cfg_look_infobar_delay_highlight > 0)
                                && (ptr_channel->window != gui_current_window) )
                                gui_infobar_printf (cfg_look_infobar_delay_highlight,
                                                    COLOR_WIN_INFOBAR_HIGHLIGHT,
                                                    _("On %s: %s> %s"),
                                                    ptr_channel->name,
                                                    host, pos);
                        }
                        else
                            irc_display_nick (ptr_channel->window, ptr_nick,
                                              MSG_TYPE_NICK, 1, 1, 0);
                        gui_printf_color_type (ptr_channel->window,
                                               MSG_TYPE_MSG,
                                               COLOR_WIN_CHAT, "%s\n", pos);
                    }
                    else
                    {
                        gui_printf (server->window,
                                    _("%s nick not found for \"%s\" command\n"),
                                    WEECHAT_ERROR, "privmsg");
                        return -1;
                    }
                }
            }
            else
            {
                gui_printf (server->window,
                            _("%s channel not found for \"%s\" command\n"),
                            WEECHAT_ERROR, "privmsg");
                return -1;
            }
        }
    }
    else
    {
        pos = strchr (host, '!');
        if (pos)
            pos[0] = '\0';
        
        pos = strchr (arguments, ' ');
        if (pos)
        {
            pos[0] = '\0';
            pos++;
            while (pos[0] == ' ')
                pos++;
            if (pos[0] == ':')
                pos++;
            
            /* version asked by another user => answer with WeeChat version */
            if (strcmp (pos, "\01VERSION\01") == 0)
            {
                buf = (struct utsname *) malloc (sizeof (struct utsname));
                if (buf && (uname (buf) == 0))
                {
                    server_sendf (server,
                                  "NOTICE %s :%sVERSION %s v%s"
                                  " compiled on %s, running "
                                  "%s %s / %s%s",
                                  host, "\01", PACKAGE_NAME, PACKAGE_VERSION, __DATE__,
                                  &buf->sysname,
                                  &buf->release, &buf->machine, "\01\r\n");
                    free (buf);
                }
                else
                    server_sendf (server,
                                  "NOTICE %s :%sVERSION %s v%s"
                                  " compiled on %s%s",
                                  host, "\01", PACKAGE_NAME, PACKAGE_VERSION, __DATE__,
                                  "\01\r\n");
                irc_display_prefix (server->window, PREFIX_INFO);
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT, _("Received a "));
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT_CHANNEL, _("CTCP VERSION "));
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT, _("from"));
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT_NICK, " %s\n", host);
            }
            else
            {
                /* ping request from another user => answer */
                if (strncmp (pos, "\01PING", 5) == 0)
                {
                    pos += 5;
                    while (pos[0] == ' ')
                        pos++;
                    pos2 = strchr (pos, '\01');
                    if (pos2)
                        pos2[0] = '\0';
                    else
                        pos = NULL;
                    if (pos && !pos[0])
                        pos = NULL;
                    if (pos)
                        server_sendf (server, "NOTICE %s :\01PING %s\01\r\n",
                                      host, pos);
                    else
                        server_sendf (server, "NOTICE %s :\01PING\01\r\n",
                                      host);
                }
                else
                {
                    /* private message received => display it */
                    ptr_channel = channel_search (server, host);
                    if (!ptr_channel)
                    {
                        ptr_channel = channel_new (server, CHAT_PRIVATE, host, 0);
                        if (!ptr_channel)
                        {
                            gui_printf (server->window,
                                        _("%s cannot create new private window \"%s\"\n"),
                                        WEECHAT_ERROR, host);
                            return -1;
                        }
                    }
                    if (!ptr_channel->topic)
                    {
                        ptr_channel->topic = strdup (host2);
                        gui_redraw_window_title (ptr_channel->window);
                    }
                    
                    gui_printf_color_type (ptr_channel->window,
                                           MSG_TYPE_NICK,
                                           COLOR_WIN_CHAT_DARK, "<");
                    if (strstr (pos, server->nick))
                    {
                        gui_printf_color_type (ptr_channel->window,
                                               MSG_TYPE_NICK,
                                               COLOR_WIN_CHAT_HIGHLIGHT,
                                               "%s", host);
                        if ( (cfg_look_infobar_delay_highlight > 0)
                            && (ptr_channel->window != gui_current_window) )
                            gui_infobar_printf (cfg_look_infobar_delay_highlight,
                                                COLOR_WIN_INFOBAR_HIGHLIGHT,
                                                _("Private %s> %s"),
                                                host, pos);
                    }
                    else
                        gui_printf_color_type (ptr_channel->window,
                                               MSG_TYPE_NICK,
                                               COLOR_WIN_NICK_PRIVATE,
                                               "%s", host);
                    gui_printf_color_type (ptr_channel->window,
                                           MSG_TYPE_NICK,
                                           COLOR_WIN_CHAT_DARK, "> ");
                    gui_printf_color_type (ptr_channel->window,
                                           MSG_TYPE_MSG,
                                           COLOR_WIN_CHAT, "%s\n", pos);
                }
            }
        }
        else
        {
            gui_printf (server->window,
                        _("%s cannot parse \"%s\" command\n"),
                        WEECHAT_ERROR, "privmsg");
            return -1;
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_quit: 'quit' command received
 */

int
irc_cmd_recv_quit (t_irc_server *server, char *host, char *arguments)
{
    char *pos;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
    /* no host => we can't identify sender of message! */
    if (host == NULL)
    {
        gui_printf (server->window,
                    _("%s \"%s\" command received without host\n"),
                    WEECHAT_ERROR, "quit");
        return -1;
    }
    
    /* keep only nick name from host */
    pos = strchr (host, '!');
    if (pos)
        pos[0] = '\0';
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ptr_channel->type == CHAT_PRIVATE)
            ptr_nick = NULL;
        else
            ptr_nick = nick_search (ptr_channel, host);
        
        if (ptr_nick || (strcmp (ptr_channel->name, host) == 0))
        {
            if (ptr_nick)
                nick_free (ptr_channel, ptr_nick);
            irc_display_prefix (ptr_channel->window, PREFIX_QUIT);
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT_NICK, "%s ", host);
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT_DARK, "(");
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT_HOST, "%s", pos + 1);
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT_DARK, ") ");
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT, _("has quit"));
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT_DARK, " (");
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT, "%s",
                              arguments);
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT_DARK, ")\n");
            if ((ptr_channel->window == gui_current_window) &&
                (gui_window_has_nicklist (ptr_channel->window)))
                gui_redraw_window_nick (ptr_channel->window);
        }
    }
    
    return 0;
}

/*
 * irc_cmd_recv_server_msg: command received from server (numeric)
 */

int
irc_cmd_recv_server_msg (t_irc_server *server, char *host, char *arguments)
{
    /* make gcc happy */
    (void) host;
    
    /* skip nickname if at beginning of server message */
    if (strncmp (server->nick, arguments, strlen (server->nick)) == 0)
    {
        arguments += strlen (server->nick) + 1;
        while (arguments[0] == ' ')
            arguments++;
    }
    
    if (arguments[0] == ':')
        arguments++;
    
    /* display server message */
    irc_display_prefix (server->window, PREFIX_SERVER);
    gui_printf_color (server->window, COLOR_WIN_CHAT, "%s\n", arguments);
    return 0;
}

/*
 * irc_cmd_recv_server_reply: server reply
 */

int
irc_cmd_recv_server_reply (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos2;
    int first;
    
    /* make gcc happy */
    (void) server;
    (void) host;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
    }
    else
        pos = arguments;
    
    irc_display_prefix (server->window, PREFIX_ERROR);
    first = 1;
    
    while (pos && pos[0])
    {
        pos2 = strchr (pos, ' ');
        if ((pos[0] == ':') || (!pos2))
        {
            if (pos[0] == ':')
                pos++;
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT,
                              "%s%s\n", (first) ? "" : ": ", pos);
            pos = NULL;
        }
        else
        {
            pos2[0] = '\0';
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_CHANNEL,
                              "%s%s",
                              (first) ? "" : " ", pos);
            first = 0;
            pos = pos2 + 1;
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_topic: 'topic' command received
 */

int
irc_cmd_recv_topic (t_irc_server *server, char *host, char *arguments)
{
    char *pos;
    t_irc_channel *ptr_channel;
    t_gui_window *window;
    
    /* make gcc happy */
    (void) host;
    
    /* keep only nick name from host */
    pos = strchr (host, '!');
    if (pos)
        pos[0] = '\0';
    
    if (!string_is_channel (arguments))
    {
        gui_printf (server->window,
                    _("%s \"%s\" command received without channel\n"),
                    WEECHAT_ERROR, "topic");
        return -1;
    }
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
        if (pos[0] == ':')
            pos++;
        if (!pos[0])
            pos = NULL;
    }
    
    ptr_channel = channel_search (server, arguments);
    window = (ptr_channel) ? ptr_channel->window : server->window;
    
    irc_display_prefix (window, PREFIX_INFO);
    gui_printf_color (window,
                      COLOR_WIN_CHAT_NICK, "%s",
                      host);
    if (pos)
    {
        gui_printf_color (window,
                          COLOR_WIN_CHAT, _(" has changed topic for "));
        gui_printf_color (window,
                          COLOR_WIN_CHAT_CHANNEL, "%s",
                          arguments);
        gui_printf_color (window,
                          COLOR_WIN_CHAT, _(" to: \"%s\"\n"),
                          pos);
    }
    else
    {
        gui_printf_color (window,
                          COLOR_WIN_CHAT, _(" has unset topic for "));
        gui_printf_color (window,
                          COLOR_WIN_CHAT_CHANNEL, "%s\n",
                          arguments);
    }
    
    if (ptr_channel)
    {
        if (ptr_channel->topic)
            free (ptr_channel->topic);
        if (pos)
            ptr_channel->topic = strdup (pos);
        else
            ptr_channel->topic = strdup ("");
        gui_redraw_window_title (ptr_channel->window);
    }
    
    return 0;
}

/*
 * irc_cmd_recv_004: '004' command (connected to irc server)
 */

int
irc_cmd_recv_004 (t_irc_server *server, char *host, char *arguments)
{
    char *pos;
    
    /* make gcc happy */
    (void) host;
    
    pos = strchr (arguments, ' ');
    if (pos)
        pos[0] = '\0';
    if (strcmp (server->nick, arguments) != 0)
    {
        free (server->nick);
        server->nick = strdup (arguments);
    }
    
    irc_cmd_recv_server_msg (server, host, arguments);
    
    /* connection to IRC server is ok! */
    server->is_connected = 1;
    gui_redraw_window_status (server->window);
    gui_redraw_window_input (server->window);
    
    /* execute command once connected */
    if (server->command && server->command[0])
        user_command(server, server->command);
    
    /* autojoin */
    if (server->autojoin && server->autojoin[0])
        return irc_cmd_send_join (server, server->autojoin);
    
    return 0;
}

/*
 * irc_cmd_recv_301: '301' command (away message)
 */

int
irc_cmd_recv_301 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_nick, *pos_message;
    
    /* make gcc happy */
    (void) server;
    (void) host;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        while (pos_nick[0] == ' ')
            pos_nick++;
        pos_message = strchr (pos_nick, ' ');
        if (pos_message)
        {
            pos_message[0] = '\0';
            pos_message++;
            while (pos_message[0] == ' ')
                pos_message++;
            if (pos_message[0] == ':')
                pos_message++;
            
            irc_display_prefix (gui_current_window, PREFIX_INFO);
            gui_printf_color (gui_current_window,
                              COLOR_WIN_CHAT_NICK, "%s", pos_nick);
            gui_printf_color (gui_current_window,
                              COLOR_WIN_CHAT, _(" is away: %s\n"), pos_message);
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_302: '302' command (userhost)
 */

int
irc_cmd_recv_302 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_host, *ptr_next;
    
    /* make gcc happy */
    (void) host;
    
    arguments = strchr (arguments, ' ');
    if (arguments)
    {
        while (arguments[0] == ' ')
            arguments++;
        if (arguments[0] == ':')
            arguments++;
        while (arguments)
        {
            pos_host = strchr (arguments, '=');
            if (pos_host)
            {
                pos_host[0] = '\0';
                pos_host++;
                
                ptr_next = strchr (pos_host, ' ');
                if (ptr_next)
                {
                    ptr_next[0] = '\0';
                    ptr_next++;
                    while (ptr_next[0] == ' ')
                        ptr_next++;
                }
                
                irc_display_prefix (server->window, PREFIX_SERVER);
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT_NICK, "%s", arguments);
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT, "=");
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT_HOST, "%s\n", pos_host);
            }
            else
                ptr_next = NULL;
            arguments = ptr_next;
            if (arguments && !arguments[0])
                arguments = NULL;
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_303: '303' command (ison)
 */

int
irc_cmd_recv_303 (t_irc_server *server, char *host, char *arguments)
{
    char *ptr_next;
    
    /* make gcc happy */
    (void) host;
    
    irc_display_prefix (server->window, PREFIX_SERVER);
    gui_printf_color (server->window,
                      COLOR_WIN_CHAT, _("Users online: "));
    
    arguments = strchr (arguments, ' ');
    if (arguments)
    {
        while (arguments[0] == ' ')
            arguments++;
        if (arguments[0] == ':')
            arguments++;
        while (arguments)
        {
            ptr_next = strchr (arguments, ' ');
            if (ptr_next)
            {
                ptr_next[0] = '\0';
                ptr_next++;
                while (ptr_next[0] == ' ')
                    ptr_next++;
            }
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_NICK, "%s ", arguments);
            arguments = ptr_next;
            if (arguments && !arguments[0])
                arguments = NULL;
        }
    }
    gui_printf (server->window, "\n");
    return 0;
}

/*
 * irc_cmd_recv_305: '305' command (unaway)
 */

int
irc_cmd_recv_305 (t_irc_server *server, char *host, char *arguments)
{
    /* make gcc happy */
    (void) host;
    
    arguments = strchr (arguments, ' ');
    if (arguments)
    {
        while (arguments[0] == ' ')
            arguments++;
        if (arguments[0] == ':')
            arguments++;
        irc_display_prefix (server->window, PREFIX_SERVER);
        gui_printf_color (server->window,
                          COLOR_WIN_CHAT, "%s\n", arguments);
    }
    server->is_away = 0;
    return 0;
}

/*
 * irc_cmd_recv_306: '306' command (now away)
 */

int
irc_cmd_recv_306 (t_irc_server *server, char *host, char *arguments)
{
    /* make gcc happy */
    (void) host;
    
    arguments = strchr (arguments, ' ');
    if (arguments)
    {
        while (arguments[0] == ' ')
            arguments++;
        if (arguments[0] == ':')
            arguments++;
        irc_display_prefix (server->window, PREFIX_SERVER);
        gui_printf_color (server->window,
                          COLOR_WIN_CHAT, "%s\n", arguments);
    }
    server->is_away = 1;
    return 0;
}

/*
 * irc_cmd_recv_311: '311' command (whois, user)
 */

int
irc_cmd_recv_311 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_nick, *pos_user, *pos_host, *pos_realname;
    
    /* make gcc happy */
    (void) host;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        while (pos_nick[0] == ' ')
            pos_nick++;
        pos_user = strchr (pos_nick, ' ');
        if (pos_user)
        {
            pos_user[0] = '\0';
            pos_user++;
            while (pos_user[0] == ' ')
                pos_user++;
            pos_host = strchr (pos_user, ' ');
            if (pos_host)
            {
                pos_host[0] = '\0';
                pos_host++;
                while (pos_host[0] == ' ')
                    pos_host++;
                pos_realname = strchr (pos_host, ' ');
                if (pos_realname)
                {
                    pos_realname[0] = '\0';
                    pos_realname++;
                    while (pos_realname[0] == ' ')
                        pos_realname++;
                    if (pos_realname[0] == '*')
                        pos_realname++;
                    while (pos_realname[0] == ' ')
                        pos_realname++;
                    if (pos_realname[0] == ':')
                        pos_realname++;
                    
                    irc_display_prefix (server->window, PREFIX_SERVER);
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_DARK, "[");
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_NICK, "%s", pos_nick);
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_DARK, "] (");
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_HOST, "%s@%s",
                                      pos_user, pos_host);
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_DARK, ")");
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT, ": %s\n", pos_realname);
                }
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_312: '312' command (whois, server)
 */

int
irc_cmd_recv_312 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_nick, *pos_server, *pos_serverinfo;
    
    /* make gcc happy */
    (void) host;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        while (pos_nick[0] == ' ')
            pos_nick++;
        pos_server = strchr (pos_nick, ' ');
        if (pos_server)
        {
            pos_server[0] = '\0';
            pos_server++;
            while (pos_server[0] == ' ')
                pos_server++;
            pos_serverinfo = strchr (pos_server, ' ');
            if (pos_serverinfo)
            {
                pos_serverinfo[0] = '\0';
                pos_serverinfo++;
                while (pos_serverinfo[0] == ' ')
                    pos_serverinfo++;
                if (pos_serverinfo[0] == ':')
                        pos_serverinfo++;
                
                irc_display_prefix (server->window, PREFIX_SERVER);
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT_DARK, "[");
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT_NICK, "%s", pos_nick);
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT_DARK, "] ");
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT, "%s ", pos_server);
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT_DARK, "(");
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT, "%s", pos_serverinfo);
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT_DARK, ")\n");
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_313: '313' command (whois, operator)
 */

int
irc_cmd_recv_313 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_nick, *pos_message;
    
    /* make gcc happy */
    (void) host;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        while (pos_nick[0] == ' ')
            pos_nick++;
        pos_message = strchr (pos_nick, ' ');
        if (pos_message)
        {
            pos_message[0] = '\0';
            pos_message++;
            while (pos_message[0] == ' ')
                pos_message++;
            if (pos_message[0] == ':')
                pos_message++;
            
            irc_display_prefix (server->window, PREFIX_SERVER);
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_DARK, "[");
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_NICK, "%s", pos_nick);
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_DARK, "] ");
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT, "%s\n", pos_message);
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_314: '314' command (whowas)
 */

int
irc_cmd_recv_314 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_nick, *pos_user, *pos_host, *pos_realname;
    
    /* make gcc happy */
    (void) host;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        while (pos_nick[0] == ' ')
            pos_nick++;
        pos_user = strchr (pos_nick, ' ');
        if (pos_user)
        {
            pos_user[0] = '\0';
            pos_user++;
            while (pos_user[0] == ' ')
                pos_user++;
            pos_host = strchr (pos_user, ' ');
            if (pos_host)
            {
                pos_host[0] = '\0';
                pos_host++;
                while (pos_host[0] == ' ')
                    pos_host++;
                pos_realname = strchr (pos_host, ' ');
                if (pos_realname)
                {
                    pos_realname[0] = '\0';
                    pos_realname++;
                    while (pos_realname[0] == ' ')
                        pos_realname++;
                    pos_realname = strchr (pos_realname, ' ');
                    if (pos_realname)
                    {
                        pos_realname[0] = '\0';
                        pos_realname++;
                        while (pos_realname[0] == ' ')
                            pos_realname++;
                        if (pos_realname[0] == ':')
                            pos_realname++;
                        
                        irc_display_prefix (server->window, PREFIX_SERVER);
                        gui_printf_color (server->window,
                                          COLOR_WIN_CHAT_NICK, "%s", pos_nick);
                        gui_printf_color (server->window,
                                          COLOR_WIN_CHAT_DARK, " (");
                        gui_printf_color (server->window,
                                          COLOR_WIN_CHAT_HOST,
                                          "%s@%s", pos_user, pos_host);
                        gui_printf_color (server->window,
                                          COLOR_WIN_CHAT_DARK, ")");
                        gui_printf_color (server->window,
                                          COLOR_WIN_CHAT,
                                          " was %s\n", pos_realname);
                    }
                }
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_317: '317' command (whois, idle)
 */

int
irc_cmd_recv_317 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_nick, *pos_idle, *pos_signon, *pos_message;
    int idle_time, day, hour, min, sec;
    time_t datetime;
    
    /* make gcc happy */
    (void) host;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        while (pos_nick[0] == ' ')
            pos_nick++;
        pos_idle = strchr (pos_nick, ' ');
        if (pos_idle)
        {
            pos_idle[0] = '\0';
            pos_idle++;
            while (pos_idle[0] == ' ')
                pos_idle++;
            pos_signon = strchr (pos_idle, ' ');
            if (pos_signon)
            {
                pos_signon[0] = '\0';
                pos_signon++;
                while (pos_signon[0] == ' ')
                    pos_signon++;
                pos_message = strchr (pos_signon, ' ');
                if (pos_message)
                {
                    pos_message[0] = '\0';
                    
                    idle_time = atoi (pos_idle);
                    day = idle_time / (60 * 60 * 24);
                    hour = (idle_time % (60 * 60 * 24)) / (60 * 60);
                    min = ((idle_time % (60 * 60 * 24)) % (60 * 60)) / 60;
                    sec = ((idle_time % (60 * 60 * 24)) % (60 * 60)) % 60;
                    
                    irc_display_prefix (server->window, PREFIX_SERVER);
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_DARK, "[");
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_NICK, "%s", pos_nick);
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_DARK, "] ");
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT, _("idle: "));
                    if (day > 0)
                    {
                        gui_printf_color (server->window,
                                          COLOR_WIN_CHAT_CHANNEL,
                                          "%d ", day);
                        gui_printf_color (server->window,
                                          COLOR_WIN_CHAT,
                                          (day > 1) ? _("days") : _("day"));
                        gui_printf_color (server->window,
                                          COLOR_WIN_CHAT,
                                          ", ");
                    }
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_CHANNEL,
                                      "%02d ", hour);
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT,
                                      (hour > 1) ? _("hours") : _("hour"));
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_CHANNEL,
                                      " %02d ", min);
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT,
                                      (min > 1) ? _("minutes") : _("minute"));
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_CHANNEL,
                                      " %02d ", sec);
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT,
                                      (sec > 1) ? _("seconds") : _("second"));
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT,
                                      ", ");
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT, _("signon at: "));
                    datetime = (time_t)(atol (pos_signon));
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_CHANNEL,
                                      "%s", ctime (&datetime));
                }
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_318: '318' command (whois, end)
 */

int
irc_cmd_recv_318 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_nick, *pos_message;
    
    /* make gcc happy */
    (void) host;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        while (pos_nick[0] == ' ')
            pos_nick++;
        pos_message = strchr (pos_nick, ' ');
        if (pos_message)
        {
            pos_message[0] = '\0';
            pos_message++;
            while (pos_message[0] == ' ')
                pos_message++;
            if (pos_message[0] == ':')
                pos_message++;
            
            irc_display_prefix (server->window, PREFIX_SERVER);
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_DARK, "[");
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_NICK, "%s", pos_nick);
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_DARK, "] ");
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT, "%s\n", pos_message);
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_319: '319' command (whois, end)
 */

int
irc_cmd_recv_319 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_nick, *pos_channel, *pos;
    
    /* make gcc happy */
    (void) host;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        while (pos_nick[0] == ' ')
            pos_nick++;
        pos_channel = strchr (pos_nick, ' ');
        if (pos_channel)
        {
            pos_channel[0] = '\0';
            pos_channel++;
            while (pos_channel[0] == ' ')
                pos_channel++;
            if (pos_channel[0] == ':')
                pos_channel++;
            
            irc_display_prefix (server->window, PREFIX_SERVER);
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_DARK, "[");
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_NICK, "%s", pos_nick);
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_DARK, "] ");
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT, _("Channels: "));
            
            while (pos_channel && pos_channel[0])
            {
                if (pos_channel[0] == '@')
                {
                    gui_printf_color (server->window,
                                      COLOR_WIN_NICK_OP, "@");
                    pos_channel++;
                }
                else
                {
                    if (pos_channel[0] == '%')
                    {
                        gui_printf_color (server->window,
                                          COLOR_WIN_NICK_HALFOP, "%");
                        pos_channel++;
                    }
                    else
                        if (pos_channel[0] == '+')
                        {
                            gui_printf_color (server->window,
                                              COLOR_WIN_NICK_VOICE, "+");
                            pos_channel++;
                        }
                }
                pos = strchr (pos_channel, ' ');
                if (pos)
                {
                    pos[0] = '\0';
                    pos++;
                    while (pos[0] == ' ')
                        pos++;
                }
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT_CHANNEL,
                                  "%s%s",
                                  pos_channel,
                                  (pos && pos[0]) ? " " : "\n");
                pos_channel = pos;
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_320: '320' command (whois, identified user)
 */

int
irc_cmd_recv_320 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_nick, *pos_message;
    
    /* make gcc happy */
    (void) host;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        while (pos_nick[0] == ' ')
            pos_nick++;
        pos_message = strchr (pos_nick, ' ');
        if (pos_message)
        {
            pos_message[0] = '\0';
            pos_message++;
            while (pos_message[0] == ' ')
                pos_message++;
            if (pos_message[0] == ':')
                pos_message++;
            
            irc_display_prefix (server->window, PREFIX_SERVER);
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_DARK, "[");
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_NICK, "%s", pos_nick);
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_DARK, "] ");
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT, "%s\n", pos_message);
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_321: '321' command (/list start)
 */

int
irc_cmd_recv_321 (t_irc_server *server, char *host, char *arguments)
{
    char *pos;
    
    /* make gcc happy */
    (void) host;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
    }
    else
        pos = arguments;
        
    irc_display_prefix (server->window, PREFIX_SERVER);
    gui_printf (server->window, "%s\n", pos);
    return 0;
}

/*
 * irc_cmd_recv_322: '322' command (channel for /list)
 */

int
irc_cmd_recv_322 (t_irc_server *server, char *host, char *arguments)
{
    char *pos;
    
    /* make gcc happy */
    (void) host;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
    }
    else
        pos = arguments;
        
    irc_display_prefix (server->window, PREFIX_SERVER);
    gui_printf (server->window, "%s\n", pos);
    return 0;
}

/*
 * irc_cmd_recv_323: '323' command (/list end)
 */

int
irc_cmd_recv_323 (t_irc_server *server, char *host, char *arguments)
{
    char *pos;
    
    /* make gcc happy */
    (void) host;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
    }
    else
        pos = arguments;
        
    irc_display_prefix (server->window, PREFIX_SERVER);
    gui_printf (server->window, "%s\n", pos);
    return 0;
}

/*
 * irc_cmd_recv_331: '331' command received (no topic for channel)
 */

int
irc_cmd_recv_331 (t_irc_server *server, char *host, char *arguments)
{
    char *pos;
    
    /* make gcc happy */
    (void) server;
    (void) host;
    
    pos = strchr (arguments, ' ');
    if (pos)
        pos[0] = '\0';
    gui_printf_color (gui_current_window,
                      COLOR_WIN_CHAT, _("No topic set for "));
    gui_printf_color (gui_current_window,
                      COLOR_WIN_CHAT_CHANNEL, "%s\n", arguments);
    return 0;
}

/*
 * irc_cmd_recv_332: '332' command received (topic of channel)
 */

int
irc_cmd_recv_332 (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos2;
    t_irc_channel *ptr_channel;
    
    /* make gcc happy */
    (void) host;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        while (pos[0] == ' ')
            pos++;
        pos2 = strchr (pos, ' ');
        if (pos2)
        {
            pos2[0] = '\0';
            ptr_channel = channel_search (server, pos);
            if (ptr_channel)
            {
                pos2++;
                while (pos2[0] == ' ')
                    pos2++;
                if (pos2[0] == ':')
                    pos2++;
                if (ptr_channel->topic)
                    free (ptr_channel->topic);
                ptr_channel->topic = strdup (pos2);
                
                irc_display_prefix (ptr_channel->window, PREFIX_INFO);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT, _("Topic for "));
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_CHANNEL, "%s", pos);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT, _(" is: \"%s\"\n"), pos2);
                
                gui_redraw_window_title (ptr_channel->window);
            }
            else
            {
                gui_printf (server->window,
                            _("%s channel not found for \"%s\" command\n"),
                            WEECHAT_ERROR, "332");
                return -1;
            }
        }
    }
    else
    {
        gui_printf (server->window,
                    _("%s cannot identify channel for \"%s\" command\n"),
                    WEECHAT_ERROR, "332");
        return -1;
    }
    return 0;
}

/*
 * irc_cmd_recv_333: '333' command received (infos about topic (nick / date))
 */

int
irc_cmd_recv_333 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_channel, *pos_nick, *pos_date;
    t_irc_channel *ptr_channel;
    time_t datetime;
    
    /* make gcc happy */
    (void) host;
    
    pos_channel = strchr (arguments, ' ');
    if (pos_channel)
    {
        while (pos_channel[0] == ' ')
            pos_channel++;
        pos_nick = strchr (pos_channel, ' ');
        if (pos_nick)
        {
            pos_nick[0] = '\0';
            pos_nick++;
            while (pos_nick[0] == ' ')
                pos_nick++;
            pos_date = strchr (pos_nick, ' ');
            if (pos_date)
            {
                pos_date[0] = '\0';
                pos_date++;
                while (pos_date[0] == ' ')
                    pos_date++;
                
                ptr_channel = channel_search (server, pos_channel);
                if (ptr_channel)
                {
                    irc_display_prefix (ptr_channel->window, PREFIX_INFO);
                    gui_printf_color (ptr_channel->window,
                                      COLOR_WIN_CHAT, _("Topic set by "));
                    gui_printf_color (ptr_channel->window,
                                      COLOR_WIN_CHAT_NICK, "%s", pos_nick);
                    datetime = (time_t)(atol (pos_date));
                    gui_printf_color (ptr_channel->window,
                                      COLOR_WIN_CHAT, ", %s", ctime (&datetime));
                }
                else
                {
                    gui_printf (server->window,
                                _("%s channel not found for \"%s\" command\n"),
                                WEECHAT_ERROR, "333");
                    return -1;
                }
            }
            else
            {
                gui_printf (server->window,
                            _("%s cannot identify date/time for \"%s\" command\n"),
                            WEECHAT_ERROR, "333");
                return -1;
            }
        }
        else
        {
            gui_printf (server->window,
                        _("%s cannot identify nickname for \"%s\" command\n"),
                        WEECHAT_ERROR, "333");
            return -1;
        }
    }
    else
    {
        gui_printf (server->window,
                    _("%s cannot identify channel for \"%s\" command\n"),
                    WEECHAT_ERROR, "333");
        return -1;
    }
    return 0;
}

/*
 * irc_cmd_recv_351: '351' command received (server version)
 */

int
irc_cmd_recv_351 (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos2;
    
    /* make gcc happy */
    (void) server;
    (void) host;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
    }
    else
        pos = arguments;
    
    pos2 = strstr (pos, " :");
    if (pos2)
    {
        pos2[0] = '\0';
        pos2 += 2;
    }
    
    irc_display_prefix (server->window, PREFIX_SERVER);
    if (pos2)
        gui_printf (server->window, "%s %s\n", pos, pos2);
    else
        gui_printf (server->window, "%s\n", pos);
    return 0;
}

/*
 * irc_cmd_recv_352: '352' command (who)
 */

int
irc_cmd_recv_352 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_channel, *pos_user, *pos_host, *pos_server, *pos_nick;
    char *pos_attr, *pos_hopcount, *pos_realname;
    
    /* make gcc happy */
    (void) host;
    
    pos_channel = strchr (arguments, ' ');
    if (pos_channel)
    {
        while (pos_channel[0] == ' ')
            pos_channel++;
        pos_user = strchr (pos_channel, ' ');
        if (pos_user)
        {
            pos_user[0] = '\0';
            pos_user++;
            while (pos_user[0] == ' ')
                pos_user++;
            pos_host = strchr (pos_user, ' ');
            if (pos_host)
            {
                pos_host[0] = '\0';
                pos_host++;
                while (pos_host[0] == ' ')
                    pos_host++;
                pos_server = strchr (pos_host, ' ');
                if (pos_server)
                {
                    pos_server[0] = '\0';
                    pos_server++;
                    while (pos_server[0] == ' ')
                        pos_server++;
                    pos_nick = strchr (pos_server, ' ');
                    if (pos_nick)
                    {
                        pos_nick[0] = '\0';
                        pos_nick++;
                        while (pos_nick[0] == ' ')
                            pos_nick++;
                        pos_attr = strchr (pos_nick, ' ');
                        if (pos_attr)
                        {
                            pos_attr[0] = '\0';
                            pos_attr++;
                            while (pos_attr[0] == ' ')
                                pos_attr++;
                            pos_hopcount = strchr (pos_attr, ' ');
                            if (pos_hopcount)
                            {
                                pos_hopcount[0] = '\0';
                                pos_hopcount++;
                                while (pos_hopcount[0] == ' ')
                                    pos_hopcount++;
                                if (pos_hopcount[0] == ':')
                                    pos_hopcount++;
                                pos_realname = strchr (pos_hopcount, ' ');
                                if (pos_realname)
                                {
                                    pos_realname[0] = '\0';
                                    pos_realname++;
                                    while (pos_realname[0] == ' ')
                                        pos_realname++;
                                    
                                    irc_display_prefix (server->window,
                                                        PREFIX_SERVER);
                                    gui_printf_color (server->window,
                                                      COLOR_WIN_CHAT_NICK,
                                                      "%s", pos_nick);
                                    gui_printf_color (server->window,
                                                      COLOR_WIN_CHAT,
                                                      _(" on "));
                                    gui_printf_color (server->window,
                                                      COLOR_WIN_CHAT_CHANNEL,
                                                      "%s ", pos_channel);
                                    gui_printf_color (server->window,
                                                      COLOR_WIN_CHAT,
                                                      "%s %s ",
                                                      pos_attr, pos_hopcount);
                                    gui_printf_color (server->window,
                                                      COLOR_WIN_CHAT_HOST,
                                                      "%s@%s",
                                                      pos_user, pos_host);
                                    gui_printf_color (server->window,
                                                      COLOR_WIN_CHAT_DARK,
                                                      " (");
                                    gui_printf_color (server->window,
                                                      COLOR_WIN_CHAT,
                                                      "%s", pos_realname);
                                    gui_printf_color (server->window,
                                                      COLOR_WIN_CHAT_DARK,
                                                      ")\n");
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_353: '353' command received (list of users on a channel)
 */

int
irc_cmd_recv_353 (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos_nick;
    int is_op, is_halfop, has_voice;
    t_irc_channel *ptr_channel;
    
    /* make gcc happy */
    (void) host;
        
    pos = strstr (arguments, " = ");
    if (pos)
        arguments = pos + 3;
    else
    {
        pos = strstr (arguments, " * ");
        if (pos)
            arguments = pos + 3;
        else
        {
            pos = strstr (arguments, " @ ");
            if (pos)
                arguments = pos + 3;
        }
    }
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        
        ptr_channel = channel_search (server, arguments);
        if (!ptr_channel)
            return 0;
        
        pos++;
        while (pos[0] == ' ')
            pos++;
        if (pos[0] != ':')
        {
            gui_printf (server->window,
                        _("%s cannot parse \"%s\" command\n"),
                        WEECHAT_ERROR, "353");
            return -1;
        }
        pos++;
        if (pos[0])
        {
            while (pos && pos[0])
            {
                is_op = 0;
                is_halfop = 0;
                has_voice = 0;
                while ((pos[0] == '@') || (pos[0] == '%') || (pos[0] == '+'))
                {
                    if (pos[0] == '@')
                        is_op = 1;
                    if (pos[0] == '%')
                        is_halfop = 1;
                    if (pos[0] == '+')
                        has_voice = 1;
                    pos++;
                }
                pos_nick = pos;
                pos = strchr (pos, ' ');
                if (pos)
                {
                    pos[0] = '\0';
                    pos++;
                }
                if (!nick_new (ptr_channel, pos_nick, is_op, is_halfop, has_voice))
                    gui_printf (server->window,
                                _("%s cannot create nick \"%s\" for channel \"%s\"\n"),
                                WEECHAT_ERROR, pos_nick, ptr_channel->name);
            }
        }
        gui_redraw_window_nick (ptr_channel->window);
    }
    else
    {
        gui_printf (server->window,
                    _("%s cannot parse \"%s\" command\n"),
                    WEECHAT_ERROR, "353");
        return -1;
    }
    return 0;
}

/*
 * irc_cmd_recv_366: '366' command received (end of /names list)
 */

int
irc_cmd_recv_366 (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos2;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    int num_nicks, num_op, num_halfop, num_voice, num_normal;
    
    /* make gcc happy */
    (void) host;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        while (pos[0] == ' ')
            pos++;
        pos2 = strchr (pos, ' ');
        if (pos2)
        {
            pos2[0] = '\0';
            pos2++;
            while (pos2[0] == ' ')
                pos2++;
            if (pos2[0] == ':')
                pos2++;
            
            ptr_channel = channel_search (server, pos);
            if (ptr_channel)
            {
    
                /* display users on channel */
                irc_display_prefix (ptr_channel->window, PREFIX_SERVER);
                gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT,
                                  _("Nicks "));
                gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_CHANNEL,
                                  "%s", ptr_channel->name);
                gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT, ": ");
                gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_DARK, "[");
                
                for (ptr_nick = ptr_channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
                {
                    irc_display_nick (ptr_channel->window, ptr_nick,
                                      MSG_TYPE_INFO, 0, 0, 1);
                    if (ptr_nick != ptr_channel->last_nick)
                        gui_printf (ptr_channel->window, " ");
                }
                gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_DARK, "]\n");
    
                /* display number of nicks, ops, halfops & voices on the channel */
                nick_count (ptr_channel, &num_nicks, &num_op, &num_halfop, &num_voice,
                            &num_normal);
                irc_display_prefix (ptr_channel->window, PREFIX_INFO);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT, _("Channel "));
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_CHANNEL,
                                  "%s", ptr_channel->name);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT, ": ");
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_CHANNEL,
                                  "%d ", num_nicks);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT,
                                  (num_nicks > 1) ? _("nicks") : _("nick"));
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_DARK, " (");
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_CHANNEL,
                                  "%d ", num_op);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT,
                                  (num_op > 1) ? _("ops") : _("op"));
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT,
                                  ", ");
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_CHANNEL,
                                  "%d ", num_halfop);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT,
                                  (num_halfop > 1) ? _("halfops") : _("halfop"));
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT,
                                  ", ");
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_CHANNEL,
                                  "%d ", num_voice);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT,
                                  (num_voice > 1) ? _("voices") : _("voice"));
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT,
                                  ", ");
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_CHANNEL,
                                  "%d ", num_normal);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT,
                                  _("normal"));
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_DARK, ")\n");
            }
            else
            {
                irc_display_prefix (gui_current_window, PREFIX_INFO);
                gui_printf_color (gui_current_window,
                                  COLOR_WIN_CHAT_CHANNEL, pos);
                gui_printf_color (gui_current_window,
                                  COLOR_WIN_CHAT, ": %s\n", pos2);
                return 0;
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_433: '433' command received (nickname already in use)
 */

int
irc_cmd_recv_433 (t_irc_server *server, char *host, char *arguments)
{
    char hostname[128];
        
    if (!server->is_connected)
    {
        if (strcmp (server->nick, server->nick1) == 0)
        {
            gui_printf (server->window,
                        _("%s: nickname \"%s\" is already in use, "
                        "trying 2nd nickname \"%s\"\n"),
                        PACKAGE_NAME, server->nick, server->nick2);
            free (server->nick);
            server->nick = strdup (server->nick2);
        }
        else
        {
            if (strcmp (server->nick, server->nick2) == 0)
            {
                gui_printf (server->window,
                            _("%s: nickname \"%s\" is already in use, "
                            "trying 3rd nickname \"%s\"\n"),
                            PACKAGE_NAME, server->nick, server->nick3);
                free (server->nick);
                server->nick = strdup (server->nick3);
            }
            else
            {
                gui_printf (server->window,
                            _("%s: all declared nicknames are already in use, "
                            "closing connection with server!\n"),
                            PACKAGE_NAME);
                server_disconnect (server);
                return 0;
            }
        }
    
        gethostname (hostname, sizeof (hostname) - 1);
        hostname[sizeof (hostname) - 1] = '\0';
        if (!hostname[0])
            strcpy (hostname, _("unknown"));
        server_sendf (server,
                      "NICK %s\r\n",
                      server->nick);
    }
    else
        return irc_cmd_recv_error (server, host, arguments);
    return 0;
}
