/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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
#include <ctype.h>
#include <sys/time.h>
#include <time.h>
#include <sys/utsname.h>
#include <regex.h>

#include "../common/weechat.h"
#include "irc.h"
#include "../common/command.h"
#include "../common/hotlist.h"
#include "../common/util.h"
#include "../common/weeconfig.h"
#include "../gui/gui.h"

#ifdef PLUGINS
#include "../plugins/plugins.h"
#endif


char *irc_last_command_received = NULL;
int command_ignored;


/*
 * irc_is_word_char: return 1 if given character is a "word character"
 */

int
irc_is_word_char (char c)
{
    if (isalnum (c))
        return 1;
    
    switch (c)
    {
        case '-':
        case '_':
        case '|':
            return 1;
    }
    
    /* not a 'word char' */
    return 0;
}

/*
 * irc_is_numeric: return 1 if given string is 100% numeric
 */

int
irc_is_numeric (char *str)
{
    while (str && str[0])
    {
        if (!isdigit (str[0]))
            return 0;
        str++;
    }
    return 1;
}

/*
 * irc_is_highlight: returns 1 if given message contains highlight (with given nick
 *                   or at least one of string in "irc_higlight" setting)
 */

int
irc_is_highlight (char *message, char *nick)
{
    char *msg, *highlight, *match, *match_pre, *match_post, *msg_pos, *pos, *pos_end;
    int end, length, startswith, endswith, wildcard_start, wildcard_end;
    
    /* empty message ? */
    if (!message || !message[0])
        return 0;
    
    /* highlight by nickname */
    match = strstr (message, nick);
    if (match)
    {
        match_pre = match - 1;
        match_post = match + strlen(nick);
        startswith = ((match == message) || (!irc_is_word_char (match_pre[0])));
        endswith = ((!match_post[0]) || (!irc_is_word_char (match_post[0])));
        if (startswith && endswith)
            return 1;
    }
    
    /* no highlight by nickname and "irc_highlight" is empty */
    if (!cfg_irc_highlight || !cfg_irc_highlight[0])
        return 0;
    
    /* convert both strings to lower case */
    if ((msg = strdup (message)) == NULL)
        return 0;
    if ((highlight = strdup (cfg_irc_highlight)) == NULL)
    {
        free (msg);
        return 0;
    }
    pos = msg;
    while (pos[0])
    {
        pos[0] = tolower (pos[0]);
        pos++;
    }
    pos = highlight;
    while (pos[0])
    {
        pos[0] = tolower (pos[0]);
        pos++;
    }
    
    /* look in "irc_highlight" for highlight */
    pos = highlight;
    end = 0;
    while (!end)
    {
        pos_end = strchr (pos, ',');
        if (!pos_end)
        {
            pos_end = strchr (pos, '\0');
            end = 1;
        }
        /* error parsing string! */
        if (!pos_end)
        {
            free (msg);
            free (highlight);
            return 0;
        }
        
        length = pos_end - pos;
        pos_end[0] = '\0';
        if (length > 0)
        {
            if ((wildcard_start = (pos[0] == '*')))
            {
                pos++;
                length--;
            }
            if ((wildcard_end = (*(pos_end - 1) == '*')))
            {
                *(pos_end - 1) = '\0';
                length--;
            }
        }
            
        if (length > 0)
        {
            msg_pos = msg;
            /* highlight found! */
            while ((match = strstr (msg_pos, pos)) != NULL)
            {
                match_pre = match - 1;
                match_post = match + length;
                startswith = ((match == msg) || (!irc_is_word_char (match_pre[0])));
                endswith = ((!match_post[0]) || (!irc_is_word_char (match_post[0])));
                if ((wildcard_start && wildcard_end) ||
                    (!wildcard_start && !wildcard_end && 
                     startswith && endswith) ||
                    (wildcard_start && endswith) ||
                    (wildcard_end && startswith))
                {
                    free (msg);
                    free (highlight);
                    return 1;
                }
                msg_pos = match_post;
            }
        }
        
        if (!end)
            pos = pos_end + 1;
    }
    
    /* no highlight found with "irc_highlight" list */
    free (msg);
    free (highlight);
    return 0;
}

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
    char *pos, *nick, *args_after_color;
    char *dup_entire_line, *dup_host, *dup_arguments;
    t_irc_recv_func *cmd_recv_func;
    char *cmd_name;
    
    if (!command)
        return -2;

    /* look for IRC command */
    cmd_found = -1;
    for (i = 0; irc_commands[i].command_name; i++)
    {
        if (ascii_strcasecmp (irc_commands[i].command_name, command) == 0)
        {
            cmd_found = i;
            break;
        }
    }
    
    /* command not found */
    if (cmd_found < 0)
    {
        /* for numeric commands, we use default recv function (irc_recv_server_msg) */
        if (irc_is_numeric (command))
        {
            cmd_name = command;
            cmd_recv_func = irc_cmd_recv_server_msg;
        }
        else
            return -3;
    }
    else
    {
        cmd_name = irc_commands[cmd_found].command_name;
        cmd_recv_func = irc_commands[cmd_found].recv_function;
    }
    
    if (cmd_recv_func != NULL)
    {
        dup_entire_line = (entire_line) ? strdup (entire_line) : NULL;
        dup_host = (host) ? strdup (host) : NULL;
        dup_arguments = (arguments) ? strdup (arguments) : NULL;
        
        command_ignored = ignore_check (dup_host,
                                        cmd_name,
                                        NULL,
                                        server->name);
#ifdef PLUGINS
        return_code = plugin_msg_handler_exec (server->name,
                                               cmd_name,
                                               dup_entire_line);
        /* plugin handler choosed to discard message for WeeChat,
           so we ignore this message in standard handler */
        if (return_code & PLUGIN_RC_OK_IGNORE_WEECHAT)
            command_ignored = 1;
#endif
        pos = (dup_host) ? strchr (dup_host, '!') : NULL;
        if (pos)
            pos[0] = '\0';
        nick = (dup_host) ? strdup (dup_host) : NULL;
        if (pos)
            pos[0] = '!';
        args_after_color = (char *)gui_color_decode ((unsigned char *)dup_arguments,
                                                     cfg_irc_colors_receive);
        irc_last_command_received = strdup (dup_entire_line);
        return_code = (int) (cmd_recv_func) (server, dup_host, nick,
                                             (args_after_color) ?
                                             args_after_color : dup_arguments);
        if (irc_last_command_received)
            free (irc_last_command_received);
        if (args_after_color)
            free (args_after_color);
        if (nick)
            free (nick);
        if (dup_entire_line)
            free (dup_entire_line);
        if (dup_host)
            free (dup_host);
        if (dup_arguments)
            free (dup_arguments);
        return return_code;
    }
    
    return 0;
}

/*
 * irc_cmd_recv_error: error received from server
 */

int
irc_cmd_recv_error (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos, *pos2;
    int first;
    t_gui_buffer *ptr_buffer;
    t_irc_channel *ptr_channel;
    
    /* make gcc happy */
    (void) server;
    (void) host;
    (void) nick;
    
    if (strncmp (arguments, "Closing Link", 12) == 0)
    {
        server_disconnect (server, 1);
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
    
    first = 1;
    
    ptr_buffer = server->buffer;
    while (pos && pos[0])
    {
        pos2 = strchr (pos, ' ');
        if ((pos[0] == ':') || (!pos2))
        {
            if (pos[0] == ':')
                pos++;
            if (first)
                irc_display_prefix (server, ptr_buffer, PREFIX_ERROR);
            gui_printf (ptr_buffer, "%s%s%s\n",
                        GUI_COLOR(COLOR_WIN_CHAT),
                        (first) ? "" : ": ",
                        pos);
            pos = NULL;
        }
        else
        {
            pos2[0] = '\0';
            if (first)
            {
                ptr_channel = channel_search (server, pos);
                if (ptr_channel)
                    ptr_buffer = ptr_channel->buffer;
                irc_display_prefix (server, ptr_buffer, PREFIX_ERROR);
            }
            gui_printf (ptr_buffer, "%s%s%s",
                        GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                        (first) ? "" : " ",
                        pos);
            first = 0;
            pos = pos2 + 1;
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_invite: 'invite' message received
 */

int
irc_cmd_recv_invite (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_channel;
    
    pos_channel = strchr (arguments, ' ');
    if (pos_channel)
    {
        pos_channel[0] = '\0';
        pos_channel++;
        while (pos_channel[0] == ' ')
            pos_channel++;
        if (pos_channel[0] == ':')
            pos_channel++;
        
        command_ignored |= ignore_check (host, "invite", pos_channel, server->name);
        
        if (!command_ignored)
        {
            irc_display_prefix (server, server->buffer, PREFIX_SERVER);
            gui_printf (server->buffer, _("You have been invited to %s%s%s by %s%s\n"),
                        GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                        pos_channel,
                        GUI_COLOR(COLOR_WIN_CHAT),
                        GUI_COLOR(COLOR_WIN_CHAT_NICK),
                        nick);
            hotlist_add (HOTLIST_HIGHLIGHT, server, server->buffer);
            gui_status_draw (gui_current_window->buffer, 1);
        }
    }
    else
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s channel \"%s\" not found for \"%s\" command\n"),
                          WEECHAT_ERROR, "", "invite");
        return -1;
    }
    return 0;
}


/*
 * irc_cmd_recv_join: 'join' message received
 */

int
irc_cmd_recv_join (t_irc_server *server, char *host, char *nick, char *arguments)
{
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    char *pos;
    
    command_ignored |= ignore_check (host, "join", arguments, server->name);
    
    ptr_channel = channel_search (server, arguments);
    if (!ptr_channel)
    {
        ptr_channel = channel_new (server, CHANNEL_TYPE_CHANNEL, arguments);
        if (!ptr_channel)
        {
            irc_display_prefix (server, server->buffer, PREFIX_ERROR);
            gui_printf_nolog (server->buffer,
                              _("%s cannot create new channel \"%s\"\n"),
                              WEECHAT_ERROR, arguments);
            return -1;
        }
        gui_buffer_new (gui_current_window, server, ptr_channel,
                        BUFFER_TYPE_STANDARD, 1);
    }
    
    pos = strchr (host, '!');
    if (!command_ignored)
    {
        irc_display_prefix (server, ptr_channel->buffer, PREFIX_JOIN);
        gui_printf (ptr_channel->buffer,
                    _("%s%s %s(%s%s%s)%s has joined %s%s\n"),
                    GUI_COLOR(COLOR_WIN_CHAT_NICK),
                    nick,
                    GUI_COLOR(COLOR_WIN_CHAT_DARK),
                    GUI_COLOR(COLOR_WIN_CHAT_HOST),
                    (pos) ? pos + 1 : host,
                    GUI_COLOR(COLOR_WIN_CHAT_DARK),
                    GUI_COLOR(COLOR_WIN_CHAT),
                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                    arguments);
    }
    ptr_nick = nick_new (server, ptr_channel, nick, 0, 0, 0, 0, 0);
    if (ptr_nick)
        ptr_nick->host = strdup ((pos) ? pos + 1 : host);
    gui_nicklist_draw (ptr_channel->buffer, 1, 1);
    gui_status_draw (ptr_channel->buffer, 1);
    return 0;
}

/*
 * irc_cmd_recv_kick: 'kick' message received
 */

int
irc_cmd_recv_kick (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_nick, *pos_comment;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
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
        
        command_ignored |= ignore_check (host, "kick", arguments, server->name);
        
        ptr_channel = channel_search (server, arguments);
        if (!ptr_channel)
        {
            irc_display_prefix (server, server->buffer, PREFIX_ERROR);
            gui_printf_nolog (server->buffer,
                              _("%s channel \"%s\" not found for \"%s\" command\n"),
                              WEECHAT_ERROR, arguments, "kick");
            return -1;
        }
        
        if (!command_ignored)
        {
            irc_display_prefix (server, ptr_channel->buffer, PREFIX_PART);
            gui_printf (ptr_channel->buffer, _("%s%s%s has kicked %s%s%s from %s%s"),
                        GUI_COLOR(COLOR_WIN_CHAT_NICK),
                        nick,
                        GUI_COLOR(COLOR_WIN_CHAT),
                        GUI_COLOR(COLOR_WIN_CHAT_NICK),
                        pos_nick,
                        GUI_COLOR(COLOR_WIN_CHAT),
                        GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                        arguments);
            if (pos_comment)
                gui_printf (ptr_channel->buffer, " %s(%s%s%s)\n",
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT),
                            pos_comment,
                            GUI_COLOR(COLOR_WIN_CHAT_DARK));
            else
                gui_printf (ptr_channel->buffer, "\n");
        }
    }
    else
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s nick \"%s\" not found for \"%s\" command\n"),
                          WEECHAT_ERROR, "", "kick");
        return -1;
    }
    if (strcmp (pos_nick, server->nick) == 0)
    {
        /* my nick was kicked => free all nicks, channel is not active any more */
        nick_free_all (ptr_channel);
        gui_nicklist_draw (ptr_channel->buffer, 1, 1);
        gui_status_draw (ptr_channel->buffer, 1);
        if (server->autorejoin)
            irc_cmd_send_join (server, NULL, ptr_channel->name);
    }
    {
        /* someone was kicked from channel (but not me) => remove only this nick */
        ptr_nick = nick_search (ptr_channel, pos_nick);
        if (ptr_nick)
        {
            nick_free (ptr_channel, ptr_nick);
            gui_nicklist_draw (ptr_channel->buffer, 1, 1);
            gui_status_draw (ptr_channel->buffer, 1);
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_kill: 'kill' message received
 */

int
irc_cmd_recv_kill (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_host2, *pos_comment;
    t_irc_channel *ptr_channel;
    
    pos_host2 = strchr (arguments, ' ');
    if (pos_host2)
    {
        pos_host2[0] = '\0';
        pos_host2++;
        while (pos_host2[0] == ' ')
            pos_host2++;
        
        if (pos_host2[0] == ':')
            pos_comment = pos_host2 + 1;
        else
        {
            pos_comment = strchr (pos_host2, ' ');
            if (pos_comment)
            {
                pos_comment[0] = '\0';
                pos_comment++;
                while (pos_comment[0] == ' ')
                    pos_comment++;
                if (pos_comment[0] == ':')
                    pos_comment++;
            }
        }
        
        for (ptr_channel = server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if (!command_ignored
                && !ignore_check (host, "kill", ptr_channel->name, server->name))
            {
                irc_display_prefix (server, ptr_channel->buffer, PREFIX_PART);
                gui_printf (ptr_channel->buffer, _("%s%s%s has killed %s%s%s from server"),
                            GUI_COLOR(COLOR_WIN_CHAT_NICK),
                            nick,
                            GUI_COLOR(COLOR_WIN_CHAT),
                            GUI_COLOR(COLOR_WIN_CHAT_NICK),
                            arguments,
                            GUI_COLOR(COLOR_WIN_CHAT));
                if (pos_comment)
                    gui_printf (ptr_channel->buffer, " %s(%s%s%s)\n",
                                GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                GUI_COLOR(COLOR_WIN_CHAT),
                                pos_comment,
                                GUI_COLOR(COLOR_WIN_CHAT_DARK));
                else
                    gui_printf (ptr_channel->buffer, "\n");
            }
        }
    }
    else
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s host \"%s\" not found for \"%s\" command\n"),
                          WEECHAT_ERROR, "", "kill");
        return -1;
    }
    return 0;
}

/*
 * irc_get_channel_modes: get channel modes
 */

void irc_get_channel_modes (t_irc_server *server, t_irc_channel *channel,
                            char *channel_name,
                            char *nick_host, char *modes, char *parm,
                            int no_display)
{
    char *pos, set_flag, unknown_mode[3];
    t_irc_nick *ptr_nick;
    
    set_flag = '+';
    while (modes && modes[0])
    {
        switch (modes[0])
        {
            case '+':
                set_flag = '+';
                break;
            case '-':
                set_flag = '-';
                break;
            case 'b':
                pos = NULL;
                if (parm)
                {
                    pos = strchr (parm, ' ');
                    if (pos)
                        pos[0] = '\0';
                }
                if (nick_host && !no_display)
                    irc_display_mode (server, channel->buffer,
                                      channel_name, NULL, set_flag,
                                      "b", nick_host,
                                      (set_flag == '+') ?
                                          _("sets ban on") :
                                          _("removes ban on"),
                                      (parm) ? parm : NULL);
                
                /* look for next parameter */
                if (parm && pos)
                {
                    pos++;
                    while (pos[0] == ' ')
                        pos++;
                    parm = pos;
                }
                break;
            case 'd':
                pos = NULL;
                if (parm)
                {
                    pos = strchr (parm, ' ');
                    if (pos)
                        pos[0] = '\0';
                }
                if (nick_host && !no_display)
                    irc_display_mode (server, channel->buffer,
                                      channel_name, NULL, set_flag,
                                      "d", nick_host,
                                      (set_flag == '+') ?
                                          _("sets realname ban on") :
                                          _("removes realname ban on"),
                                      (parm) ? parm : NULL);
                
                /* look for next parameter */
                if (parm && pos)
                {
                    pos++;
                    while (pos[0] == ' ')
                        pos++;
                    parm = pos;
                }
                break;
            case 'e':
                pos = NULL;
                if (parm)
                {
                    pos = strchr (parm, ' ');
                    if (pos)
                        pos[0] = '\0';
                }
                if (nick_host && !no_display)
                    irc_display_mode (server, channel->buffer,
                                      channel_name, NULL, set_flag,
                                      "e", nick_host,
                                      (set_flag == '+') ?
                                          _("sets ban exemption on") :
                                          _("removes ban exemption on"),
                                      (parm) ? parm : NULL);
                
                /* look for next parameter */
                if (parm && pos)
                {
                    pos++;
                    while (pos[0] == ' ')
                        pos++;
                    parm = pos;
                }
                break;
            case 'f':
                pos = NULL;
                if (parm)
                {
                    pos = strchr (parm, ' ');
                    if (pos)
                        pos[0] = '\0';
                }
                if (nick_host && !no_display)
                    irc_display_mode (server, channel->buffer,
                                      channel_name, NULL, set_flag,
                                      "f", nick_host,
                                      (set_flag == '+') ?
                                          _("sets mode +f") :
                                          _("removes mode +f"),
                                      (parm) ? parm : NULL);
                
                /* look for next parameter */
                if (parm && pos)
                {
                    pos++;
                    while (pos[0] == ' ')
                        pos++;
                    parm = pos;
                }
                break;
            case 'h':
                pos = NULL;
                if (parm)
                {
                    pos = strchr (parm, ' ');
                    if (pos)
                        pos[0] = '\0';
                }
                if (nick_host && !no_display)
                    irc_display_mode (server, channel->buffer,
                                      channel_name, NULL, set_flag,
                                      "h", nick_host,
                                      (set_flag == '+') ?
                                          _("gives half channel operator status to") :
                                          _("removes half channel operator status from"),
                                      (parm) ? parm : NULL);
                if (parm)
                {
                    ptr_nick = nick_search (channel, parm);
                    if (ptr_nick)
                    {
                        NICK_SET_FLAG(ptr_nick, (set_flag == '+'), NICK_HALFOP);
                        nick_resort (channel, ptr_nick);
                        gui_nicklist_draw (channel->buffer, 1, 1);
                    }
                }
                
                /* look for next parameter */
                if (parm && pos)
                {
                    pos++;
                    while (pos[0] == ' ')
                        pos++;
                    parm = pos;
                }
                break;
            case 'i':
                if (nick_host && !no_display)
                    irc_display_mode (server, channel->buffer,
                                      channel_name, NULL, set_flag,
                                      "i", nick_host,
                                      (set_flag == '+') ?
                                          _("sets invite-only channel flag") :
                                          _("removes invite-only channel flag"),
                                      NULL);
                CHANNEL_SET_MODE(channel, (set_flag == '+'),
                                 CHANNEL_MODE_INVITE);
                break;
            case 'I':
                pos = NULL;
                if (parm)
                {
                    pos = strchr (parm, ' ');
                    if (pos)
                        pos[0] = '\0';
                }
                if (nick_host && !no_display)
                    irc_display_mode (server, channel->buffer,
                                      channel_name, NULL, set_flag,
                                      "I", nick_host,
                                      (set_flag == '+') ?
                                          _("sets invite-only exemption on") :
                                          _("removes invite-only exemption on"),
                                      (parm) ? parm : NULL);
                
                /* look for next parameter */
                if (parm && pos)
                {
                    pos++;
                    while (pos[0] == ' ')
                        pos++;
                    parm = pos;
                }
                break;
            case 'k':
                pos = NULL;
                if (parm)
                {
                    pos = strchr (parm, ' ');
                    if (pos)
                        pos[0] = '\0';
                }
                if (nick_host && !no_display)
                    irc_display_mode (server, channel->buffer,
                                      channel_name, NULL, set_flag,
                                      "k", nick_host,
                                      (set_flag == '+') ?
                                          _("sets channel key to") :
                                          _("removes channel key"),
                                      (set_flag == '+') ?
                                      ((parm) ? parm : NULL) :
                                      NULL);
                CHANNEL_SET_MODE(channel, (set_flag == '+'),
                                 CHANNEL_MODE_KEY);
                if (channel->key)
                    free (channel->key);
                channel->key = strdup (parm);
                
                /* look for next parameter */
                if (parm && pos)
                {
                    pos++;
                    while (pos[0] == ' ')
                        pos++;
                    parm = pos;
                }
                break;
            case 'l':
                pos = NULL;
                if (parm)
                {
                    pos = strchr (parm, ' ');
                    if (pos)
                        pos[0] = '\0';
                }
                if (nick_host && !no_display)
                    irc_display_mode (server, channel->buffer,
                                      channel_name, NULL, set_flag,
                                      "l", nick_host,
                                      (set_flag == '+') ?
                                          _("sets the user limit to") :
                                          _("removes user limit"),
                                      (set_flag == '+') ?
                                      ((parm) ? parm : NULL) :
                                      NULL);
                CHANNEL_SET_MODE(channel, (set_flag == '+'),
                                 CHANNEL_MODE_LIMIT);
                if (set_flag == '-')
                    channel->limit = 0;
                if ((set_flag == '+') && parm)
                    channel->limit = atoi (parm);
                
                /* look for next parameter */
                if (parm && pos)
                {
                    pos++;
                    while (pos[0] == ' ')
                        pos++;
                    parm = pos;
                }
                break;
            case 'm':
                if (nick_host && !no_display)
                    irc_display_mode (server, channel->buffer,
                                      channel_name, NULL, set_flag,
                                      "m", nick_host,
                                      (set_flag == '+') ?
                                          _("sets moderated channel flag") :
                                          _("removes moderated channel flag"),
                                      NULL);
                CHANNEL_SET_MODE(channel, (set_flag == '+'),
                                 CHANNEL_MODE_MODERATED);
                break;
            case 'n':
                if (nick_host && !no_display)
                    irc_display_mode (server, channel->buffer,
                                      channel_name, NULL, set_flag,
                                      "n", nick_host,
                                      (set_flag == '+') ?
                                          _("sets messages from channel only flag") :
                                          _("removes messages from channel only flag"),
                                      NULL);
                CHANNEL_SET_MODE(channel, (set_flag == '+'),
                                 CHANNEL_MODE_NO_MSG_OUT);
                break;
            case 'o':
                pos = NULL;
                if (parm)
                {
                    pos = strchr (parm, ' ');
                    if (pos)
                        pos[0] = '\0';
                }
                if (nick_host && !no_display)
                    irc_display_mode (server, channel->buffer,
                                      channel_name, NULL, set_flag,
                                      "o", nick_host,
                                      (set_flag == '+') ?
                                          _("gives channel operator status to") :
                                          _("removes channel operator status from"),
                                      (parm) ? parm : NULL);
                if (parm)
                {
                    ptr_nick = nick_search (channel, parm);
                    if (ptr_nick)
                    {
                        NICK_SET_FLAG(ptr_nick, (set_flag == '+'), NICK_OP);
                        nick_resort (channel, ptr_nick);
                        gui_nicklist_draw (channel->buffer, 1, 1);
                    }
                }
                
                /* look for next parameter */
                if (parm && pos)
                {
                    pos++;
                    while (pos[0] == ' ')
                        pos++;
                    parm = pos;
                }
                break;
            case 'p':
                if (nick_host && !no_display)
                    irc_display_mode (server, channel->buffer,
                                      channel_name, NULL, set_flag,
                                      "p", nick_host,
                                      (set_flag == '+') ?
                                          _("sets private channel flag") :
                                          _("removes private channel flag"),
                                      NULL);
                CHANNEL_SET_MODE(channel, (set_flag == '+'),
                                 CHANNEL_MODE_PRIVATE);
                break;
            case 'q':
                pos = NULL;
                if (parm)
                {
                    pos = strchr (parm, ' ');
                    if (pos)
                        pos[0] = '\0';
                }
                if (nick_host && !no_display)
                    irc_display_mode (server, channel->buffer,
                                      channel_name, NULL, set_flag,
                                      "q", nick_host,
                                      (set_flag == '+') ?
                                          _("sets quiet on") :
                                          _("removes quiet on"),
                                      (parm) ? parm : NULL);
                
                /* look for next parameter */
                if (parm && pos)
                {
                    pos++;
                    while (pos[0] == ' ')
                        pos++;
                    parm = pos;
                }
                break;
            case 's':
                if (nick_host && !no_display)
                    irc_display_mode (server, channel->buffer,
                                      channel_name, NULL, set_flag,
                                      "s", nick_host,
                                      (set_flag == '+') ?
                                          _("sets secret channel flag") :
                                          _("removes secret channel flag"),
                                      NULL);
                CHANNEL_SET_MODE(channel, (set_flag == '+'),
                                 CHANNEL_MODE_SECRET);
                break;
            case 't':
                if (nick_host && !no_display)
                    irc_display_mode (server, channel->buffer,
                                      channel_name, NULL, set_flag,
                                      "t", nick_host,
                                      (set_flag == '+') ?
                                          _("sets topic protection") :
                                          _("removes topic protection"),
                                      NULL);
                CHANNEL_SET_MODE(channel, (set_flag == '+'),
                                 CHANNEL_MODE_TOPIC);
                break;
            case 'v':
                pos = NULL;
                if (parm)
                {
                    pos = strchr (parm, ' ');
                    if (pos)
                        pos[0] = '\0';
                }
                if (nick_host && !no_display)
                    irc_display_mode (server, channel->buffer,
                                      channel_name, NULL, set_flag,
                                      "v", nick_host,
                                      (set_flag == '+') ?
                                          _("gives voice to") :
                                          _("removes voice from"),
                                      (parm) ? parm : NULL);
                
                if (parm)
                {
                    ptr_nick = nick_search (channel, parm);
                    if (ptr_nick)
                    {
                        NICK_SET_FLAG(ptr_nick, (set_flag == '+'), NICK_VOICE);
                        nick_resort (channel, ptr_nick);
                        gui_nicklist_draw (channel->buffer, 1, 1);
                    }
                }
                
                /* look for next parameter */
                if (parm && pos)
                {
                    pos++;
                    while (pos[0] == ' ')
                        pos++;
                    parm = pos;
                }
                break;
            default: /* unknown mode received */
                pos = NULL;
                if (parm)
                {
                    pos = strchr (parm, ' ');
                    if (pos)
                        pos[0] = '\0';
                }
                if (nick_host && !no_display)
                {
                    unknown_mode[0] = set_flag;
                    unknown_mode[1] = modes[0];
                    unknown_mode[2] = '\0';
                    irc_display_mode (server, channel->buffer,
                                      channel_name, NULL, set_flag,
                                      unknown_mode + 1, nick_host,
                                      unknown_mode,
                                      (parm) ? parm : NULL);
                }
                
                /* look for next parameter */
                if (parm && pos)
                {
                    pos++;
                    while (pos[0] == ' ')
                        pos++;
                    parm = pos;
                }
                break;
        }
        modes++;
    }
}

/*
 * irc_get_nick_modes: get nick modes
 */

void irc_get_nick_modes (t_irc_server *server,
                         char *nick_host, char *modes, int no_display)
{
    char set_flag, unknown_mode[3];
    
    set_flag = '+';
    while (modes && modes[0])
    {
        switch (modes[0])
        {
            case ':':
                break;
            case '+':
                set_flag = '+';
                break;
            case '-':
                set_flag = '-';
                break;
            case 'a':
                if (nick_host && !no_display)
                    irc_display_mode (server, server->buffer,
                                      NULL, server->nick, set_flag,
                                      "a", nick_host,
                                      (set_flag == '+') ?
                                          _("sets away") :
                                          _("removes away"),
                                      NULL);
                NICK_SET_MODE(server, (set_flag == '+'), NICK_MODE_AWAY);
                break;
            case 'i':
                if (nick_host && !no_display)
                    irc_display_mode (server, server->buffer,
                                      NULL, server->nick, set_flag,
                                      "i", nick_host,
                                      (set_flag == '+') ?
                                          _("sets invisible") :
                                          _("removes invisible"),
                                      NULL);
                NICK_SET_MODE(server, (set_flag == '+'), NICK_MODE_INVISIBLE);
                break;
            case 'w':
                if (nick_host && !no_display)
                    irc_display_mode (server, server->buffer,
                                      NULL, server->nick, set_flag,
                                      "w", nick_host,
                                      (set_flag == '+') ?
                                          _("sets wallops") :
                                          _("removes wallops"),
                                      NULL);
                NICK_SET_MODE(server, (set_flag == '+'), NICK_MODE_WALLOPS);
                break;
            case 'r':
                if (nick_host && !no_display)
                    irc_display_mode (server, server->buffer,
                                      NULL, server->nick, set_flag,
                                      "r", nick_host,
                                      (set_flag == '+') ?
                                          _("sets restricted") :
                                          _("removes restricted"),
                                      NULL);
                NICK_SET_MODE(server, (set_flag == '+'), NICK_MODE_RESTRICTED);
                break;
            case 'o':
                if (nick_host && !no_display)
                    irc_display_mode (server, server->buffer,
                                      NULL, server->nick, set_flag,
                                      "o", nick_host,
                                      (set_flag == '+') ?
                                          _("sets operator") :
                                          _("removes operator"),
                                      NULL);
                NICK_SET_MODE(server, (set_flag == '+'), NICK_MODE_OPERATOR);
                break;
            case 'O':
                if (nick_host && !no_display)
                    irc_display_mode (server, server->buffer,
                                      NULL, server->nick, set_flag,
                                      "O", nick_host,
                                      (set_flag == '+') ?
                                          _("sets local operator") :
                                          _("removes local operator"),
                                      NULL);
                NICK_SET_MODE(server, (set_flag == '+'), NICK_MODE_LOCAL_OPERATOR);
                break;
            case 's':
                if (nick_host && !no_display)
                    irc_display_mode (server, server->buffer,
                                      NULL, server->nick, set_flag,
                                      "s", nick_host,
                                      (set_flag == '+') ?
                                          _("sets server notices") :
                                          _("removes server notices"),
                                      NULL);
                NICK_SET_MODE(server, (set_flag == '+'), NICK_MODE_LOCAL_OPERATOR);
                break;
            default: /* unknown mode received */
                if (nick_host && !no_display)
                {
                    unknown_mode[0] = set_flag;
                    unknown_mode[1] = modes[0];
                    unknown_mode[2] = '\0';
                    irc_display_mode (server, server->buffer,
                                      NULL, server->nick, set_flag,
                                      unknown_mode + 1, nick_host,
                                      unknown_mode, NULL);
                }
                break;
        }
        modes++;
    }
}

/*
 * irc_cmd_recv_mode: 'mode' message received
 */

int
irc_cmd_recv_mode (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos, *pos_parm;
    t_irc_channel *ptr_channel;
    
    /* no host => we can't identify sender of message! */
    if (host == NULL)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s \"%s\" command received without host\n"),
                          WEECHAT_ERROR, "mode");
        return -1;
    }
    
    pos = strchr (arguments, ' ');
    if (!pos)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
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
    }
    
    if (string_is_channel (arguments))
    {
        ptr_channel = channel_search (server, arguments);
        if (ptr_channel)
        {
            irc_get_channel_modes (server, ptr_channel, arguments, nick,
                                   pos, pos_parm, command_ignored);
            gui_status_draw (ptr_channel->buffer, 1);
            gui_input_draw (ptr_channel->buffer, 1);
        }
        else
        {
            irc_display_prefix (server, server->buffer, PREFIX_ERROR);
            gui_printf_nolog (server->buffer,
                              _("%s channel \"%s\" not found for \"%s\" command\n"),
                              WEECHAT_ERROR, arguments, "mode");
            return -1;
        }
    }
    else
    {
        irc_get_nick_modes (server, nick, pos, command_ignored);
        gui_status_draw (gui_current_window->buffer, 1);
        gui_input_draw (gui_current_window->buffer, 1);
    }
    return 0;
}

/*
 * irc_cmd_recv_nick: 'nick' message received
 */

int
irc_cmd_recv_nick (t_irc_server *server, char *host, char *nick, char *arguments)
{
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    int nick_is_me;
    t_gui_window *ptr_win;
    t_gui_buffer *ptr_buffer;
    
    /* no host => we can't identify sender of message! */
    if (host == NULL)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s \"%s\" command received without host\n"),
                          WEECHAT_ERROR, "nick");
        return -1;
    }
    
    /* change nickname in any opened private window */
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if ((SERVER(ptr_buffer) == server) && BUFFER_IS_PRIVATE(ptr_buffer))
        {
            if ((CHANNEL(ptr_buffer)->name)
                && (ascii_strcasecmp (nick, CHANNEL(ptr_buffer)->name) == 0))
            {
                ptr_channel = channel_search_any (server, arguments);
                if (!ptr_channel)
                {
                    free (CHANNEL(ptr_buffer)->name);
                    CHANNEL(ptr_buffer)->name = strdup (arguments);
                }
            }
        }
    }
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        ptr_nick = nick_search (ptr_channel, nick);
        if (ptr_nick)
        {
            nick_is_me = (strcmp (ptr_nick->nick, server->nick) == 0) ? 1 : 0;
            if (nick_is_me)
                gui_add_hotlist = 0;
            nick_change (ptr_channel, ptr_nick, arguments);
            if (!command_ignored
                && !ignore_check (host, "nick", ptr_channel->name, server->name))
            {
                irc_display_prefix (server, ptr_channel->buffer, PREFIX_INFO);
                if (nick_is_me)
                    gui_printf (ptr_channel->buffer, _("You are now known as %s%s\n"),
                                GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                arguments);
                else
                    gui_printf (ptr_channel->buffer, _("%s%s%s is now known as %s%s\n"),
                                GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                nick,
                                GUI_COLOR(COLOR_WIN_CHAT),
                                GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                arguments);
            }
            gui_nicklist_draw (ptr_channel->buffer, 1, 1);
            gui_add_hotlist = 1;
        }
    }
    
    if (strcmp (server->nick, nick) == 0)
    {
        free (server->nick);
        server->nick = strdup (arguments);
        gui_status_draw (gui_current_window->buffer, 1);
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            if (ptr_win->buffer->server == server)
                gui_input_draw (ptr_win->buffer, 1);
        }
    }
    else
    {
        gui_status_draw (gui_current_window->buffer, 1);
        gui_input_draw (gui_current_window->buffer, 1);
    }
    
    return 0;
}

/*
 * irc_cmd_recv_notice: 'notice' message received
 */

int
irc_cmd_recv_notice (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *host2, *pos, *pos2, *pos_usec;
    struct timeval tv;
    struct timezone tz;
    long sec1, usec1, sec2, usec2, difftime;
    t_irc_channel *ptr_channel;
    int highlight;
    
    host2 = NULL;
    if (host)
    {
        pos = strchr (host, '!');
        if (pos)
            host2 = pos + 1;
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
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s nickname not found for \"%s\" command\n"),
                          WEECHAT_ERROR, "notice");
        return -1;
    }
    
    if (!command_ignored)
    {
        if (strncmp (pos, "\01VERSION", 8) == 0)
        {
            pos += 9;
            pos2 = strchr (pos, '\01');
            if (pos2)
                pos2[0] = '\0';
            irc_display_prefix (server, server->buffer, PREFIX_SERVER);
            gui_printf (server->buffer, _("CTCP %sVERSION%s reply from %s%s%s: %s\n"),
                        GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                        GUI_COLOR(COLOR_WIN_CHAT),
                        GUI_COLOR(COLOR_WIN_CHAT_NICK),
                        nick,
                        GUI_COLOR(COLOR_WIN_CHAT),
                        pos);
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
                        
                        irc_display_prefix (server, server->buffer, PREFIX_SERVER);
                        gui_printf (server->buffer, _("CTCP %sPING%s reply from %s%s%s: %ld.%ld seconds\n"),
                                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                    GUI_COLOR(COLOR_WIN_CHAT),
                                    GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                    nick,
                                    GUI_COLOR(COLOR_WIN_CHAT),
                                    difftime / 1000000,
                                    (difftime % 1000000) / 1000);
                    }
                }
            }
            else
            {
                if (nick && nick[0] && cfg_irc_notice_as_pv)
                {
                    ptr_channel = channel_search (server, nick);
                    if (!ptr_channel)
                    {
                        ptr_channel = channel_new (server, CHANNEL_TYPE_PRIVATE, nick);
                        if (!ptr_channel)
                        {
                            irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                            gui_printf_nolog (server->buffer,
                                              _("%s cannot create new private window \"%s\"\n"),
                                              WEECHAT_ERROR, nick);
                            return -1;
                        }
                        gui_buffer_new (gui_current_window, server, ptr_channel,
                                        BUFFER_TYPE_STANDARD, 0);
                    }
                    if (!ptr_channel->topic)
                        ptr_channel->topic = strdup ((host2) ? host2 : "");
                    
                    gui_printf_type (ptr_channel->buffer, MSG_TYPE_NICK,
                                     "%s<",
                                     GUI_COLOR(COLOR_WIN_CHAT_DARK));
                    if (irc_is_highlight (pos, server->nick))
                    {
                        gui_printf_type (ptr_channel->buffer,
                                         MSG_TYPE_NICK | MSG_TYPE_HIGHLIGHT,
                                         "%s%s",
                                         GUI_COLOR(COLOR_WIN_CHAT_HIGHLIGHT),
                                         nick);
                        if ( (cfg_look_infobar_delay_highlight > 0)
                             && (ptr_channel->buffer != gui_current_window->buffer) )
                            gui_infobar_printf_from_buffer (ptr_channel->buffer,
                                                            cfg_look_infobar_delay_highlight,
                                                            COLOR_WIN_INFOBAR_HIGHLIGHT,
                                                            _("Private"), " %s> %s",
                                                            nick, pos);
                        highlight = 1;
                    }
                    else
                    {
                        gui_printf_type (ptr_channel->buffer, MSG_TYPE_NICK,
                                         "%s%s",
                                         GUI_COLOR(COLOR_WIN_NICK_PRIVATE),
                                         nick);
                        highlight = 0;
                    }
                    gui_printf_type (ptr_channel->buffer, MSG_TYPE_NICK,
                                     "%s> ",
                                     GUI_COLOR(COLOR_WIN_CHAT_DARK));
                    gui_printf_type (ptr_channel->buffer, MSG_TYPE_MSG,
                                     "%s%s\n",
                                     GUI_COLOR(COLOR_WIN_CHAT),
                                     pos);
#ifdef PLUGINS
                    if (highlight)
                        (void) plugin_msg_handler_exec (server->name,
                                                        "weechat_highlight",
                                                        irc_last_command_received);
#endif
                }
                else
                {
                    irc_display_prefix (server, server->buffer, PREFIX_SERVER);
                    if (host)
                    {
                        gui_printf (server->buffer, "%s%s",
                                    GUI_COLOR(COLOR_WIN_CHAT_NICK), nick);
                        if (host2)
                            gui_printf (server->buffer, " %s(%s%s%s)",
                                        GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                        GUI_COLOR(COLOR_WIN_CHAT_HOST),
                                        host2,
                                        GUI_COLOR(COLOR_WIN_CHAT_DARK));
                        gui_printf (server->buffer, "%s: ",
                                    GUI_COLOR(COLOR_WIN_CHAT));
                    }
                    gui_printf (server->buffer, "%s%s\n",
                                GUI_COLOR(COLOR_WIN_CHAT),
                                pos);
                    if ((nick) && (ascii_strcasecmp (nick, "nickserv") != 0) &&
                        (ascii_strcasecmp (nick, "chanserv") != 0) &&
                        (ascii_strcasecmp (nick, "memoserv") != 0))
                    {
                        hotlist_add (HOTLIST_PRIVATE, server, server->buffer);
                        gui_status_draw (gui_current_window->buffer, 1);
                    }
                }
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_part: 'part' message received
 */

int
irc_cmd_recv_part (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos, *pos_args, *join_string;
    int join_length;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
    /* no host => we can't identify sender of message! */
    if (!host || !arguments)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
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
    
    ptr_channel = channel_search (server, arguments);
    if (ptr_channel)
    {
        command_ignored |= ignore_check (host, "part", ptr_channel->name, server->name);
        ptr_nick = nick_search (ptr_channel, nick);
        if (ptr_nick)
        {
            /* display part message */
            if (!command_ignored)
            {
                pos = strchr (host, '!');
                irc_display_prefix (server, ptr_channel->buffer, PREFIX_PART);
                gui_printf (ptr_channel->buffer, _("%s%s %s(%s%s%s)%s has left %s%s"),
                            GUI_COLOR(COLOR_WIN_CHAT_NICK),
                            nick,
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT_HOST),
                            (pos) ? pos + 1 : "",
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT),
                            GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                            ptr_channel->name);
                if (pos_args && pos_args[0])
                    gui_printf (ptr_channel->buffer, " %s(%s%s%s)\n",
                                GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                GUI_COLOR(COLOR_WIN_CHAT),
                                pos_args,
                                GUI_COLOR(COLOR_WIN_CHAT_DARK));
                else
                    gui_printf (ptr_channel->buffer, "\n");
            }
            
            /* part request was issued by local client ? */
            if (strcmp (ptr_nick->nick, server->nick) == 0)
            {
                nick_free_all (ptr_channel);
                
                /* cycling ? => rejoin channel immediately */
                if (ptr_channel->cycle)
                {
                    ptr_channel->cycle = 0;
                    if (ptr_channel->key)
                    {
                        join_length = strlen (ptr_channel->name) + 1 +
                            strlen (ptr_channel->key) + 1;
                        join_string = (char *)malloc (join_length);
                        if (join_string)
                        {
                            snprintf (join_string, join_length, "%s %s",
                                      ptr_channel->name,
                                      ptr_channel->key);
                            irc_cmd_send_join(server, ptr_channel, join_string);
                            free (join_string);
                        }
                        else
                            irc_cmd_send_join(server, ptr_channel, ptr_channel->name);
                    }
                    else
                        irc_cmd_send_join(server, ptr_channel, ptr_channel->name);
                }
                if (ptr_channel->close)
                {
                    gui_buffer_free (ptr_channel->buffer, 1);
                    channel_free (server, ptr_channel);
                    ptr_channel = NULL;
                }
            }
            else
                nick_free (ptr_channel, ptr_nick);
            
            if (ptr_channel)
            {
                gui_nicklist_draw (ptr_channel->buffer, 1, 1);
                gui_status_draw (ptr_channel->buffer, 1);
            }
            gui_input_draw (gui_current_window->buffer, 1);
        }
    }
    else
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s channel \"%s\" not found for \"%s\" command\n"),
                          WEECHAT_ERROR, arguments, "part");
        return -1;
    }
    
    return 0;
}

/*
 * irc_cmd_recv_ping: 'ping' command received
 */

int
irc_cmd_recv_ping (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    pos = strrchr (arguments, ' ');
    if (pos)
        pos[0] = '\0';
    server_sendf (server, "PONG :%s\r\n", arguments);
    return 0;
}

/*
 * irc_cmd_recv_pong: 'pong' command received
 */

int
irc_cmd_recv_pong (t_irc_server *server, char *host, char *nick, char *arguments)
{
    struct timeval tv;
    struct timezone tz;
    int old_lag;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    (void) arguments;
    
    if (server->lag_check_time.tv_sec != 0)
    {
        /* calculate lag (time diff with lag check) */
        old_lag = server->lag;
        gettimeofday (&tv, &tz);
        server->lag = (int) get_timeval_diff (&(server->lag_check_time), &tv);
        if (old_lag != server->lag)
            gui_status_draw (gui_current_window->buffer, 1);
        
        /* schedule next lag check */
        server->lag_check_time.tv_sec = 0;
        server->lag_check_time.tv_usec = 0;
        server->lag_next_check = time (NULL) + cfg_irc_lag_check;
    }
    return 0;
}

/*
 * irc_cmd_recv_privmsg: 'privmsg' command received
 */

int
irc_cmd_recv_privmsg (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos, *pos2, *host2;
    char *pos_file, *pos_addr, *pos_port, *pos_size, *pos_start_resume;  /* for DCC */
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    struct utsname *buf;
    int highlight;
    
    /* no host => we can't identify sender of message! */
    if (host == NULL)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s \"%s\" command received without host\n"),
                          WEECHAT_ERROR, "privmsg");
        return -1;
    }
    
    pos = strchr (host, '!');
    if (pos)
        host2 = pos + 1;
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
                    command_ignored |= ignore_check (host, "action", ptr_channel->name, server->name);
                    pos += 8;
                    pos2 = strchr (pos, '\01');
                    if (pos2)
                        pos2[0] = '\0';
                    if (!command_ignored)
                    {
                        irc_display_prefix (server, ptr_channel->buffer, PREFIX_ACTION_ME);
                        if (irc_is_highlight (pos, server->nick))
                        {
                            gui_printf_type (ptr_channel->buffer,
                                             MSG_TYPE_MSG | MSG_TYPE_HIGHLIGHT,
                                             "%s%s",
                                             GUI_COLOR(COLOR_WIN_CHAT_HIGHLIGHT),
                                             nick);
                            if ( (cfg_look_infobar)
                                 && (cfg_look_infobar_delay_highlight > 0)
                                 && (ptr_channel->buffer != gui_current_window->buffer) )
                                gui_infobar_printf_from_buffer (ptr_channel->buffer,
                                                                cfg_look_infobar_delay_highlight,
                                                                COLOR_WIN_INFOBAR_HIGHLIGHT,
                                                                _("Channel"), " %s: * %s %s",
                                                                ptr_channel->name,
                                                                nick, pos);
                            gui_printf (ptr_channel->buffer, " %s%s\n",
                                        GUI_COLOR(COLOR_WIN_CHAT), pos);
#ifdef PLUGINS
                            (void) plugin_msg_handler_exec (server->name,
                                                            "weechat_highlight",
                                                            irc_last_command_received);
#endif
                        }
                        else
                        {
                            gui_printf_type (ptr_channel->buffer, MSG_TYPE_MSG,
                                             "%s%s",
                                             GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                             nick);
                            gui_printf (ptr_channel->buffer, " %s%s\n",
                                        GUI_COLOR(COLOR_WIN_CHAT), pos);
                        }
                    }
                    return 0;
                }
                if (strncmp (pos, "\01SOUND ", 7) == 0)
                {
                    command_ignored |= ignore_check (host, "ctcp", ptr_channel->name, server->name);
                    pos += 7;
                    pos2 = strchr (pos, '\01');
                    if (pos2)
                        pos2[0] = '\0';
                    if (!command_ignored)
                    {
                        irc_display_prefix (server, ptr_channel->buffer, PREFIX_SERVER);
                        gui_printf (ptr_channel->buffer,
                                    _("Received a CTCP %sSOUND%s \"%s\" from %s%s\n"),
                                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                    GUI_COLOR(COLOR_WIN_CHAT),
                                    pos,
                                    GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                    nick);
                    }
                    return 0;
                }
                if (strncmp (pos, "\01PING", 5) == 0)
                {
                    command_ignored |= ignore_check (host, "ctcp", ptr_channel->name, server->name);
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
                                      nick, pos);
                    else
                        server_sendf (server, "NOTICE %s :\01PING\01\r\n",
                                      nick);
                    irc_display_prefix (server, ptr_channel->buffer, PREFIX_SERVER);
                    gui_printf (ptr_channel->buffer,
                                _("CTCP %sPING%s received from %s%s\n"),
                                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                GUI_COLOR(COLOR_WIN_CHAT),
                                GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                nick);
                    return 0;
                }
                
                /* unknown CTCP ? */
                pos2 = strchr (pos + 1, '\01');
                if ((pos[0] == '\01') && pos2 && (pos2[1] == '\0'))
                {
                    command_ignored |= ignore_check (host, "ctcp", ptr_channel->name, server->name);
                    pos++;
                    pos2[0] = '\0';
                    pos2 = strchr (pos, ' ');
                    if (pos2)
                    {
                        pos2[0] = '\0';
                        pos2++;
                        while (pos2[0] == ' ')
                            pos2++;
                        if (!pos2[0])
                            pos2 = NULL;
                    }
                    if (!command_ignored)
                    {
                        irc_display_prefix (server, ptr_channel->buffer, PREFIX_SERVER);
                        gui_printf (ptr_channel->buffer,
                                    _("Unknown CTCP %s%s%s received from %s%s"),
                                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                    pos,
                                    GUI_COLOR(COLOR_WIN_CHAT),
                                    GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                    nick);
                        if (pos2)
                            gui_printf (ptr_channel->buffer, "%s: %s\n",
                                        GUI_COLOR(COLOR_WIN_CHAT),
                                        pos2);
                        else
                            gui_printf (ptr_channel->buffer, "\n");
                    }
                    return 0;
                }
                
                /* other message */
                command_ignored |= ignore_check (host, "privmsg", ptr_channel->name, server->name);
                if (!command_ignored)
                {
                    ptr_nick = nick_search (ptr_channel, nick);
                    if (irc_is_highlight (pos, server->nick))
                    {
                        irc_display_nick (ptr_channel->buffer, ptr_nick,
                                          (ptr_nick) ? NULL : nick,
                                          MSG_TYPE_NICK | MSG_TYPE_HIGHLIGHT,
                                          1, COLOR_WIN_CHAT_HIGHLIGHT, 0);
                        if ( (cfg_look_infobar)
                             && (cfg_look_infobar_delay_highlight > 0)
                             && (ptr_channel->buffer != gui_current_window->buffer) )
                            gui_infobar_printf_from_buffer (ptr_channel->buffer,
                                                            cfg_look_infobar_delay_highlight,
                                                            COLOR_WIN_INFOBAR_HIGHLIGHT,
                                                            _("Channel"), " %s: %s> %s",
                                                            ptr_channel->name,
                                                            nick, pos);
                        gui_printf_type (ptr_channel->buffer, MSG_TYPE_MSG,
                                         "%s\n", pos);
#ifdef PLUGINS
                        (void) plugin_msg_handler_exec (server->name,
                                                        "weechat_highlight",
                                                        irc_last_command_received);
#endif
                    }
                    else
                    {
                        irc_display_nick (ptr_channel->buffer, ptr_nick,
                                          (ptr_nick) ? NULL : nick,
                                          MSG_TYPE_NICK, 1, -1, 0);
                        gui_printf_type (ptr_channel->buffer, MSG_TYPE_MSG,
                                         "%s\n", pos);
                    }
                }
            }
            else
            {
                irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                gui_printf_nolog (server->buffer,
                                  _("%s channel \"%s\" not found for \"%s\" command\n"),
                                  WEECHAT_ERROR, arguments, "privmsg");
                return -1;
            }
        }
    }
    else
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
            
            /* version asked by another user => answer with WeeChat version */
            if (strncmp (pos, "\01VERSION", 8) == 0)
            {
                command_ignored |= ignore_check (host, "ctcp", NULL, server->name);
                if (!command_ignored)
                {
                    pos2 = strchr (pos + 8, ' ');
                    if (pos2)
                    {
                        while (pos2[0] == ' ')
                            pos2++;
                        if (pos2[0] == '\01')
                            pos2 = NULL;
                        else if (!pos2[0])
                            pos2 = NULL;
                    }
                    
                    buf = (struct utsname *) malloc (sizeof (struct utsname));
                    if (buf && (uname (buf) >= 0))
                    {
                        server_sendf (server,
                                      "NOTICE %s :%sVERSION %s v%s"
                                      " compiled on %s, running "
                                      "%s %s / %s%s",
                                      nick, "\01", PACKAGE_NAME, PACKAGE_VERSION, __DATE__,
                                      &buf->sysname,
                                      &buf->release, &buf->machine, "\01\r\n");
                        free (buf);
                    }
                    else
                        server_sendf (server,
                                      "NOTICE %s :%sVERSION %s v%s"
                                      " compiled on %s%s",
                                      nick, "\01", PACKAGE_NAME, PACKAGE_VERSION, __DATE__,
                                      "\01\r\n");
                    irc_display_prefix (server, server->buffer, PREFIX_SERVER);
                    gui_printf (server->buffer,
                                _("CTCP %sVERSION%s received from %s%s"),
                                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                GUI_COLOR(COLOR_WIN_CHAT),
                                GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                nick);
                    if (pos2)
                        gui_printf (server->buffer, "%s: %s\n",
                                    GUI_COLOR(COLOR_WIN_CHAT),
                                    pos2);
                    else
                        gui_printf (server->buffer, "\n");
#ifdef PLUGINS
                    (void) plugin_msg_handler_exec (server->name,
                                                    "weechat_ctcp",
                                                    irc_last_command_received);
#endif
                }
                return 0;
            }
            
            /* ping request from another user => answer */
            if (strncmp (pos, "\01PING", 5) == 0)
            {
                command_ignored |= ignore_check (host, "ctcp", NULL, server->name);
                if (!command_ignored)
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
                                      nick, pos);
                    else
                        server_sendf (server, "NOTICE %s :\01PING\01\r\n",
                                      nick);
                    irc_display_prefix (server, server->buffer, PREFIX_SERVER);
                    gui_printf (server->buffer,
                                _("CTCP %sPING%s received from %s%s\n"),
                                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                GUI_COLOR(COLOR_WIN_CHAT),
                                GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                nick);
#ifdef PLUGINS
                    (void) plugin_msg_handler_exec (server->name,
                                                    "weechat_ctcp",
                                                    irc_last_command_received);
#endif
                }
                return 0;
            }
            
            /* incoming DCC file */
            if (strncmp (pos, "\01DCC SEND", 9) == 0)
            {
                /* check if DCC SEND is ok, i.e. with 0x01 at end */
                pos2 = strchr (pos + 1, '\01');
                if (!pos2)
                {
                    irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                    gui_printf_nolog (server->buffer,
                                      _("%s cannot parse \"%s\" command\n"),
                                      WEECHAT_ERROR, "privmsg");
                    return -1;
                }
                pos2[0] = '\0';
                
                command_ignored |= ignore_check (host, "dcc", NULL, server->name);
                
                if (!command_ignored)
                {
                    /* DCC filename */
                    pos_file = pos + 9;
                    while (pos_file[0] == ' ')
                        pos_file++;
                    
                    /* look for file size */
                    pos_size = strrchr (pos_file, ' ');
                    if (!pos_size)
                    {
                        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                        gui_printf_nolog (server->buffer,
                                          _("%s cannot parse \"%s\" command\n"),
                                          WEECHAT_ERROR, "privmsg");
                        return -1;
                    }
                    pos2 = pos_size;
                    pos_size++;
                    while (pos2[0] == ' ')
                        pos2--;
                    pos2[1] = '\0';
                    
                    /* look for DCC port */
                    pos_port = strrchr (pos_file, ' ');
                    if (!pos_port)
                    {
                        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                        gui_printf_nolog (server->buffer,
                                          _("%s cannot parse \"%s\" command\n"),
                                          WEECHAT_ERROR, "privmsg");
                        return -1;
                    }
                    pos2 = pos_port;
                    pos_port++;
                    while (pos2[0] == ' ')
                        pos2--;
                    pos2[1] = '\0';
                    
                    /* look for DCC IP address */
                    pos_addr = strrchr (pos_file, ' ');
                    if (!pos_addr)
                    {
                        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                        gui_printf_nolog (server->buffer,
                                          _("%s cannot parse \"%s\" command\n"),
                                          WEECHAT_ERROR, "privmsg");
                        return -1;
                    }
                    pos2 = pos_addr;
                    pos_addr++;
                    while (pos2[0] == ' ')
                        pos2--;
                    pos2[1] = '\0';
                    
                    dcc_add (server, DCC_FILE_RECV, strtoul (pos_addr, NULL, 10),
                             atoi (pos_port), nick, -1, pos_file, NULL,
                             strtoul (pos_size, NULL, 10));
#ifdef PLUGINS
                    (void) plugin_msg_handler_exec (server->name,
                                                    "weechat_dcc",
                                                    irc_last_command_received);
#endif
                }
                return 0;
            }
            
            /* incoming DCC RESUME (asked by receiver) */
            if (strncmp (pos, "\01DCC RESUME", 11) == 0)
            {
                /* check if DCC RESUME is ok, i.e. with 0x01 at end */
                pos2 = strchr (pos + 1, '\01');
                if (!pos2)
                {
                    irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                    gui_printf_nolog (server->buffer,
                                      _("%s cannot parse \"%s\" command\n"),
                                      WEECHAT_ERROR, "privmsg");
                    return -1;
                }
                pos2[0] = '\0';
                
                command_ignored |= ignore_check (host, "dcc", NULL, server->name);
                
                if (!command_ignored)
                {
                    /* DCC filename */
                    pos_file = pos + 11;
                    while (pos_file[0] == ' ')
                        pos_file++;
                    
                    /* look for resume start position */
                    pos_start_resume = strrchr (pos_file, ' ');
                    if (!pos_start_resume)
                    {
                        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                        gui_printf_nolog (server->buffer,
                                          _("%s cannot parse \"%s\" command\n"),
                                          WEECHAT_ERROR, "privmsg");
                        return -1;
                    }
                    pos2 = pos_start_resume;
                    pos_start_resume++;
                    while (pos2[0] == ' ')
                        pos2--;
                    pos2[1] = '\0';
                    
                    /* look for DCC port */
                    pos_port = strrchr (pos_file, ' ');
                    if (!pos_port)
                    {
                        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                        gui_printf_nolog (server->buffer,
                                          _("%s cannot parse \"%s\" command\n"),
                                          WEECHAT_ERROR, "privmsg");
                        return -1;
                    }
                    pos2 = pos_port;
                    pos_port++;
                    while (pos2[0] == ' ')
                        pos2--;
                    pos2[1] = '\0';
                    
                    dcc_accept_resume (server, pos_file, atoi (pos_port),
                                       strtoul (pos_start_resume, NULL, 10));
#ifdef PLUGINS
                    (void) plugin_msg_handler_exec (server->name,
                                                    "weechat_dcc",
                                                    irc_last_command_received);
#endif
                }
                return 0;
            }
            
            /* incoming DCC ACCEPT (resume accepted by sender) */
            if (strncmp (pos, "\01DCC ACCEPT", 11) == 0)
            {
                /* check if DCC ACCEPT is ok, i.e. with 0x01 at end */
                pos2 = strchr (pos + 1, '\01');
                if (!pos2)
                {
                    irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                    gui_printf_nolog (server->buffer,
                                      _("%s cannot parse \"%s\" command\n"),
                                      WEECHAT_ERROR, "privmsg");
                    return -1;
                }
                pos2[0] = '\0';
                
                command_ignored |= ignore_check (host, "dcc", NULL, server->name);
                
                if (!command_ignored)
                {
                    /* DCC filename */
                    pos_file = pos + 11;
                    while (pos_file[0] == ' ')
                        pos_file++;
                    
                    /* look for resume start position */
                    pos_start_resume = strrchr (pos_file, ' ');
                    if (!pos_start_resume)
                    {
                        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                        gui_printf_nolog (server->buffer,
                                          _("%s cannot parse \"%s\" command\n"),
                                          WEECHAT_ERROR, "privmsg");
                        return -1;
                    }
                    pos2 = pos_start_resume;
                    pos_start_resume++;
                    while (pos2[0] == ' ')
                        pos2--;
                    pos2[1] = '\0';
                    
                    /* look for DCC port */
                    pos_port = strrchr (pos_file, ' ');
                    if (!pos_port)
                    {
                        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                        gui_printf_nolog (server->buffer,
                                          _("%s cannot parse \"%s\" command\n"),
                                          WEECHAT_ERROR, "privmsg");
                        return -1;
                    }
                    pos2 = pos_port;
                    pos_port++;
                    while (pos2[0] == ' ')
                        pos2--;
                    pos2[1] = '\0';
                    
                    dcc_start_resume (server, pos_file, atoi (pos_port),
                                      strtoul (pos_start_resume, NULL, 10));
#ifdef PLUGINS
                    (void) plugin_msg_handler_exec (server->name,
                                                    "weechat_dcc",
                                                    irc_last_command_received);
#endif
                }
                return 0;
            }
            
            /* incoming DCC CHAT */
            if (strncmp (pos, "\01DCC CHAT", 9) == 0)
            {
                /* check if DCC CHAT is ok, i.e. with 0x01 at end */
                pos2 = strchr (pos + 1, '\01');
                if (!pos2)
                {
                    irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                    gui_printf_nolog (server->buffer,
                                      _("%s cannot parse \"%s\" command\n"),
                                      WEECHAT_ERROR, "privmsg");
                    return -1;
                }
                pos2[0] = '\0';
                
                command_ignored |= ignore_check (host, "dcc", NULL, server->name);
                
                if (!command_ignored)
                {
                    /* CHAT type */
                    pos_file = pos + 9;
                    while (pos_file[0] == ' ')
                        pos_file++;
                    
                    /* DCC IP address */
                    pos_addr = strchr (pos_file, ' ');
                    if (!pos_addr)
                    {
                        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                        gui_printf_nolog (server->buffer,
                                          _("%s cannot parse \"%s\" command\n"),
                                          WEECHAT_ERROR, "privmsg");
                        return -1;
                    }
                    pos_addr[0] = '\0';
                    pos_addr++;
                    while (pos_addr[0] == ' ')
                        pos_addr++;
                    
                    /* look for DCC port */
                    pos_port = strchr (pos_addr, ' ');
                    if (!pos_port)
                    {
                        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                        gui_printf_nolog (server->buffer,
                                          _("%s cannot parse \"%s\" command\n"),
                                          WEECHAT_ERROR, "privmsg");
                        return -1;
                    }
                    pos_port[0] = '\0';
                    pos_port++;
                    while (pos_port[0] == ' ')
                        pos_port++;
                    
                    if (ascii_strcasecmp (pos_file, "chat") != 0)
                    {
                        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                        gui_printf_nolog (server->buffer,
                                          _("%s unknown DCC CHAT type received from "),
                                          WEECHAT_ERROR);
                        gui_printf (server->buffer, "%s%s%s: \"%s\"\n",
                                    GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                    nick,
                                    GUI_COLOR(COLOR_WIN_CHAT),
                                    pos_file);
                        return -1;
                    }
                    
                    dcc_add (server, DCC_CHAT_RECV, strtoul (pos_addr, NULL, 10),
                             atoi (pos_port), nick, -1, NULL, NULL, 0);
#ifdef PLUGINS
                    (void) plugin_msg_handler_exec (server->name,
                                                    "weechat_dcc",
                                                    irc_last_command_received);
#endif
                }
                return 0;
            }
            
            /* private message received => display it */
            ptr_channel = channel_search (server, nick);
            
            if (strncmp (pos, "\01ACTION ", 8) == 0)
            {
                command_ignored |= ignore_check (host, "action", NULL, server->name);
                command_ignored |= ignore_check (host, "pv", NULL, server->name);
                
                if (!command_ignored)
                {
                    if (!ptr_channel)
                    {
                        ptr_channel = channel_new (server, CHANNEL_TYPE_PRIVATE, nick);
                        if (!ptr_channel)
                        {
                            irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                            gui_printf_nolog (server->buffer,
                                              _("%s cannot create new private window \"%s\"\n"),
                                              WEECHAT_ERROR, nick);
                            return -1;
                        }
                        gui_buffer_new (gui_current_window, server, ptr_channel,
                                        BUFFER_TYPE_STANDARD, 0);
                    }
                    if (!ptr_channel->topic)
                        ptr_channel->topic = strdup (host2);
                    
                    pos += 8;
                    pos2 = strchr (pos, '\01');
                    if (pos2)
                        pos2[0] = '\0';
                    irc_display_prefix (server, ptr_channel->buffer, PREFIX_ACTION_ME);
                    if (irc_is_highlight (pos, server->nick))
                    {
                        gui_printf_type (ptr_channel->buffer,
                                         MSG_TYPE_MSG | MSG_TYPE_HIGHLIGHT,
                                         "%s%s",
                                         GUI_COLOR(COLOR_WIN_CHAT_HIGHLIGHT),
                                         nick);
                        if ( (cfg_look_infobar)
                             && (cfg_look_infobar_delay_highlight > 0)
                             && (ptr_channel->buffer != gui_current_window->buffer) )
                            gui_infobar_printf_from_buffer (ptr_channel->buffer,
                                                            cfg_look_infobar_delay_highlight,
                                                            COLOR_WIN_INFOBAR_HIGHLIGHT,
                                                            _("Channel"), " %s: * %s %s",
                                                            ptr_channel->name,
                                                            nick, pos);
                        gui_printf (ptr_channel->buffer, " %s%s\n",
                                    GUI_COLOR(COLOR_WIN_CHAT), pos);
#ifdef PLUGINS
                        (void) plugin_msg_handler_exec (server->name,
                                                        "weechat_highlight",
                                                        irc_last_command_received);
#endif
                    }
                    else
                    {
                        gui_printf_type (ptr_channel->buffer, MSG_TYPE_MSG,
                                         "%s%s",
                                         GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                         nick);
                        gui_printf (ptr_channel->buffer, " %s%s\n",
                                    GUI_COLOR(COLOR_WIN_CHAT), pos);
#ifdef PLUGINS
                        (void) plugin_msg_handler_exec (server->name,
                                                        "weechat_pv",
                                                        irc_last_command_received);
#endif
                    }
                }
            }
            else
            {
                /* unknown CTCP ? */
                pos2 = strchr (pos + 1, '\01');
                if ((pos[0] == '\01') && pos2 && (pos2[1] == '\0'))
                {
                    command_ignored |= ignore_check (host, "ctcp", NULL, server->name);
                    
                    if (!command_ignored)
                    {
                        pos++;
                        pos2[0] = '\0';
                        pos2 = strchr (pos, ' ');
                        if (pos2)
                        {
                            pos2[0] = '\0';
                            pos2++;
                            while (pos2[0] == ' ')
                                pos2++;
                            if (!pos2[0])
                                pos2 = NULL;
                        }
                        irc_display_prefix (server, server->buffer, PREFIX_SERVER);
                        gui_printf (server->buffer,
                                    _("Unknown CTCP %s%s%s received from %s%s"),
                                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                    pos,
                                    GUI_COLOR(COLOR_WIN_CHAT),
                                    GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                    nick);
                        if (pos2)
                            gui_printf (server->buffer, "%s: %s\n",
                                        GUI_COLOR(COLOR_WIN_CHAT),
                                        pos2);
                        else
                            gui_printf (server->buffer, "\n");
#ifdef PLUGINS
                        (void) plugin_msg_handler_exec (server->name,
                                                        "weechat_ctcp",
                                                        irc_last_command_received);
#endif
                    }
                    return 0;
                }
                else
                {
                    command_ignored |= ignore_check (host, "pv", NULL, server->name);
                    
                    if (!command_ignored)
                    {
                        if (!ptr_channel)
                        {
                            ptr_channel = channel_new (server, CHANNEL_TYPE_PRIVATE, nick);
                            if (!ptr_channel)
                            {
                                irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                                gui_printf_nolog (server->buffer,
                                                  _("%s cannot create new private window \"%s\"\n"),
                                                  WEECHAT_ERROR, nick);
                                return -1;
                            }
                            gui_buffer_new (gui_current_window, server, ptr_channel,
                                            BUFFER_TYPE_STANDARD, 0);
                        }
                        if (!ptr_channel->topic)
                            ptr_channel->topic = strdup (host2);
                        
                        if (irc_is_highlight (pos, server->nick))
                        {
                            irc_display_nick (ptr_channel->buffer, NULL, nick,
                                              MSG_TYPE_NICK | MSG_TYPE_HIGHLIGHT, 1,
                                              COLOR_WIN_CHAT_HIGHLIGHT, 0);
                            if ((cfg_look_infobar_delay_highlight > 0)
                                && (ptr_channel->buffer != gui_current_window->buffer))
                                gui_infobar_printf_from_buffer (ptr_channel->buffer,
                                                                cfg_look_infobar_delay_highlight,
                                                                COLOR_WIN_INFOBAR_HIGHLIGHT,
                                                                _("Private"), " %s> %s",
                                                                nick, pos);
                            highlight = 1;
                        }
                        else
                        {
                            irc_display_nick (ptr_channel->buffer, NULL, nick,
                                              MSG_TYPE_NICK, 1, COLOR_WIN_NICK_PRIVATE, 0);
                            highlight = 0;
                        }
                        gui_printf_type (ptr_channel->buffer, MSG_TYPE_MSG,
                                         "%s%s\n",
                                         GUI_COLOR(COLOR_WIN_CHAT),
                                         pos);
#ifdef PLUGINS
                        (void) plugin_msg_handler_exec (server->name,
                                                        "weechat_pv",
                                                        irc_last_command_received);
                        if (highlight)
                            (void) plugin_msg_handler_exec (server->name,
                                                            "weechat_highlight",
                                                            irc_last_command_received);
#endif
                    }
                }
            }
        }
        else
        {
            irc_display_prefix (server, server->buffer, PREFIX_ERROR);
            gui_printf_nolog (server->buffer,
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
irc_cmd_recv_quit (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
    /* no host => we can't identify sender of message! */
    if (host == NULL)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s \"%s\" command received without host\n"),
                          WEECHAT_ERROR, "quit");
        return -1;
    }
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if ((ptr_channel->type == CHANNEL_TYPE_PRIVATE)
            || (ptr_channel->type == CHANNEL_TYPE_DCC_CHAT))
            ptr_nick = NULL;
        else
            ptr_nick = nick_search (ptr_channel, nick);
        
        if (ptr_nick || (strcmp (ptr_channel->name, nick) == 0))
        {
            if (ptr_nick)
                nick_free (ptr_channel, ptr_nick);
            if (!command_ignored
                && !ignore_check (host, "quit", ptr_channel->name, server->name))
            {
                pos = strchr (host, '!');
                irc_display_prefix (server, ptr_channel->buffer, PREFIX_QUIT);
                gui_printf (ptr_channel->buffer,
                            _("%s%s %s(%s%s%s)%s has quit"),
                            GUI_COLOR(COLOR_WIN_CHAT_NICK),
                            nick,
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT_HOST),
                            (pos) ? pos + 1 : "",
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT));
                gui_printf (ptr_channel->buffer,
                            " %s(%s%s%s)\n",
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT),
                            arguments,
                            GUI_COLOR(COLOR_WIN_CHAT_DARK));
            }
            gui_nicklist_draw (ptr_channel->buffer, 1, 1);
            gui_status_draw (ptr_channel->buffer, 1);
        }
    }
    
    return 0;
}

/*
 * irc_cmd_recv_server_mode_reason: command received from server (numeric),
 *                                  format: "mode :reason"
 */

int
irc_cmd_recv_server_mode_reason (t_irc_server *server, char *host,
                                 char *nick, char *arguments)
{
    char *ptr_msg;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
        /* skip nickname if at beginning of server message */
        if (strncmp (server->nick, arguments, strlen (server->nick)) == 0)
        {
            arguments += strlen (server->nick) + 1;
            while (arguments[0] == ' ')
                arguments++;
        }

        ptr_msg = strchr (arguments, ' ');
        if (ptr_msg)
        {
            ptr_msg[0] = '\0';
            ptr_msg++;
            while (ptr_msg[0] == ' ')
                ptr_msg++;
            if (ptr_msg[0] == ':')
                ptr_msg++;
        }
        
        irc_display_prefix (server, server->buffer, PREFIX_SERVER);
        gui_printf (server->buffer, "%s%s: %s\n",
                    GUI_COLOR(COLOR_WIN_CHAT), arguments,
                    (ptr_msg) ? ptr_msg : "");
    }
    return 0;
}

/*
 * irc_cmd_recv_server_msg: command received from server (numeric)
 */

int
irc_cmd_recv_server_msg (t_irc_server *server, char *host, char *nick, char *arguments)
{
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
        /* skip nickname if at beginning of server message */
        if (strncmp (server->nick, arguments, strlen (server->nick)) == 0)
        {
            arguments += strlen (server->nick) + 1;
            while (arguments[0] == ' ')
                arguments++;
        }
        
        if (arguments[0] == ':')
            arguments++;
        
        irc_display_prefix (server, server->buffer, PREFIX_SERVER);
        gui_printf (server->buffer, "%s%s\n",
                    GUI_COLOR(COLOR_WIN_CHAT), arguments);
    }
    return 0;
}

/*
 * irc_cmd_recv_server_reply: server reply
 */

int
irc_cmd_recv_server_reply (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos, *pos2;
    int first;
    
    /* make gcc happy */
    (void) server;
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
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
        
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        first = 1;
        
        while (pos && pos[0])
        {
            pos2 = strchr (pos, ' ');
            if ((pos[0] == ':') || (!pos2))
            {
                if (pos[0] == ':')
                    pos++;
                gui_printf (server->buffer, "%s%s%s\n",
                            GUI_COLOR(COLOR_WIN_CHAT),
                            (first) ? "" : ": ",
                            pos);
                pos = NULL;
            }
            else
            {
                pos2[0] = '\0';
                gui_printf (server->buffer, "%s%s%s\n",
                            GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                            (first) ? "" : " ",
                            pos);
                first = 0;
                pos = pos2 + 1;
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_topic: 'topic' command received
 */

int
irc_cmd_recv_topic (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos;
    t_irc_channel *ptr_channel;
    t_gui_buffer *buffer;
    
    /* make gcc happy */
    (void) nick;
    
    if (!string_is_channel (arguments))
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
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
    
    command_ignored |= ignore_check (host, "topic", arguments, server->name);
    
    ptr_channel = channel_search (server, arguments);
    buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
    
    if (!command_ignored)
    {
        irc_display_prefix (server, buffer, PREFIX_INFO);
        if (pos)
        {
            gui_printf (buffer,
                        _("%s%s%s has changed topic for %s%s%s to:"),
                        GUI_COLOR(COLOR_WIN_CHAT_NICK),
                        nick,
                        GUI_COLOR(COLOR_WIN_CHAT),
                        GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                        arguments,
                        GUI_COLOR(COLOR_WIN_CHAT));
            gui_printf (buffer, " \"%s\"\n", pos);
        }
        else
            gui_printf (buffer,
                        _("%s%s%s has unset topic for %s%s\n"),
                        GUI_COLOR(COLOR_WIN_CHAT_NICK),
                        nick,
                        GUI_COLOR(COLOR_WIN_CHAT),
                        GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
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
        gui_chat_draw_title (ptr_channel->buffer, 1);
    }
    
    return 0;
}

/*
 * irc_cmd_recv_001: '001' command (connected to irc server)
 */

int
irc_cmd_recv_001 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos;
    char **commands, **ptr;
    t_irc_channel *ptr_channel;
    char *away_msg;
    
    pos = strchr (arguments, ' ');
    if (pos)
        pos[0] = '\0';
    if (strcmp (server->nick, arguments) != 0)
    {
        free (server->nick);
        server->nick = strdup (arguments);
    }
    
    irc_cmd_recv_server_msg (server, host, nick, arguments);
    
    /* connection to IRC server is ok! */
    server->is_connected = 1;
    server->lag_next_check = time (NULL) + cfg_irc_lag_check;
    gui_status_draw (server->buffer, 1);
    gui_input_draw (server->buffer, 1);
    
    /* execute command when connected */
    if (server->command && server->command[0])
    {	
	/* splitting command on ';' which can be escaped with '\;' */ 
	commands = split_multi_command (server->command, ';');
	if (commands)
	{
	    for (ptr = commands; *ptr; ptr++)
		user_command (server, NULL, *ptr, 0);
	    free_multi_command (commands);
	}
	
	if (server->command_delay > 0)
	    sleep (server->command_delay);
    }
	
    /* auto-join after disconnection (only rejoins opened channels) */
    if (server->reconnect_join && server->channels)
    {
        for (ptr_channel = server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if (ptr_channel->type == CHANNEL_TYPE_CHANNEL)
            {
                if (ptr_channel->key)
                    server_sendf (server, "JOIN %s %s\r\n",
                                  ptr_channel->name, ptr_channel->key);
                else
                    server_sendf (server, "JOIN %s\r\n",
                                  ptr_channel->name);
            }
        }
        server->reconnect_join = 0;
    }
    else
    {
        /* auto-join when connecting to server for first time */
        if (server->autojoin && server->autojoin[0])
            return irc_cmd_send_join (server, NULL, server->autojoin);
    }

    /* set away message if user was away (before disconnection for example) */
    if (server->away_message && server->away_message[0])
    {
        away_msg = strdup (server->away_message);
        if (away_msg)
        {
            irc_send_away (server, away_msg);
            free (away_msg);
        }
    }
    
    return 0;
}

/*
 * irc_cmd_recv_221: '221' command (user mode string)
 */

int
irc_cmd_recv_221 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_mode;
    
    /* make gcc happy */
    (void) server;
    (void) host;
    (void) nick;
    
    pos_mode = strchr (arguments, ' ');
    if (pos_mode)
    {
        pos_mode[0] = '\0';
        pos_mode++;
        while (pos_mode[0] == ' ')
            pos_mode++;
        
        if (!command_ignored)
        {
            irc_display_prefix (server, server->buffer, PREFIX_SERVER);
            gui_printf (server->buffer, _("User mode %s[%s%s%s/%s%s%s]\n"),
                        GUI_COLOR(COLOR_WIN_CHAT_DARK),
                        GUI_COLOR(COLOR_WIN_CHAT_NICK),
                        arguments,
                        GUI_COLOR(COLOR_WIN_CHAT),
                        GUI_COLOR(COLOR_WIN_CHAT_NICK),
                        pos_mode,
                        GUI_COLOR(COLOR_WIN_CHAT_DARK));
        }
    }
    else
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s cannot parse \"%s\" command\n"),
                          WEECHAT_ERROR, "221");
        return -1;
    }
    
    return 0;
}

/*
 * irc_cmd_recv_301: '301' command (away message)
 */

int
irc_cmd_recv_301 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_nick, *pos_message;
    t_irc_channel *ptr_channel;
    t_gui_buffer *ptr_buffer;
    
    /* make gcc happy */
    (void) server;
    (void) host;
    (void) nick;
    
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
            
            if (!command_ignored)
            {
                /* look for private buffer to display message */
                ptr_channel = channel_search (server, pos_nick);
                if (!cfg_irc_show_away_once || !ptr_channel ||
                    !(ptr_channel->away_message) ||
                    (strcmp (ptr_channel->away_message, pos_message) != 0))
                {
                    ptr_buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
                    irc_display_prefix (server, ptr_buffer, PREFIX_INFO);
                    gui_printf (ptr_buffer,
                                _("%s%s%s is away: %s\n"),
                                GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                pos_nick,
                                GUI_COLOR(COLOR_WIN_CHAT),
                                pos_message);
                    if (ptr_channel)
                    {
                        if (ptr_channel->away_message)
                            free (ptr_channel->away_message);
                        ptr_channel->away_message = strdup (pos_message);
                    }
                }
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_302: '302' command (userhost)
 */

int
irc_cmd_recv_302 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_host, *ptr_next;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
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
                    
                    irc_display_prefix (server, server->buffer, PREFIX_SERVER);
                    gui_printf (server->buffer, "%s%s%s=%s%s\n",
                                GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                arguments,
                                GUI_COLOR(COLOR_WIN_CHAT),
                                GUI_COLOR(COLOR_WIN_CHAT_HOST),
                                pos_host);
                }
                else
                    ptr_next = NULL;
                arguments = ptr_next;
                if (arguments && !arguments[0])
                    arguments = NULL;
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_303: '303' command (ison)
 */

int
irc_cmd_recv_303 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *ptr_next;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
        irc_display_prefix (server, server->buffer, PREFIX_SERVER);
        gui_printf (server->buffer, _("Users online: "));
        
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
                gui_printf (server->buffer, "%s%s ",
                            GUI_COLOR(COLOR_WIN_CHAT_NICK),
                            arguments);
                arguments = ptr_next;
                if (arguments && !arguments[0])
                    arguments = NULL;
            }
        }
        gui_printf (server->buffer, "\n");
    }
    return 0;
}

/*
 * irc_cmd_recv_305: '305' command (unaway)
 */

int
irc_cmd_recv_305 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    t_gui_window *ptr_window;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
        arguments = strchr (arguments, ' ');
        if (arguments)
        {
            while (arguments[0] == ' ')
                arguments++;
            if (arguments[0] == ':')
                arguments++;
            irc_display_prefix (server, server->buffer, PREFIX_SERVER);
            gui_printf (server->buffer, "%s\n", arguments);
        }
    }
    server->is_away = 0;
    server->away_time = 0;
    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if (SERVER(ptr_window->buffer) == server)
            gui_status_draw (ptr_window->buffer, 1);
    }
    return 0;
}

/*
 * irc_cmd_recv_306: '306' command (now away)
 */

int
irc_cmd_recv_306 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    t_gui_window *ptr_window;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
        arguments = strchr (arguments, ' ');
        if (arguments)
        {
            while (arguments[0] == ' ')
                arguments++;
            if (arguments[0] == ':')
                arguments++;
            irc_display_prefix (server, server->buffer, PREFIX_SERVER);
            gui_printf (server->buffer, "%s\n", arguments);
        }
    }
    server->is_away = 1;
    server->away_time = time (NULL);
    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if (SERVER(ptr_window->buffer) == server)
            gui_status_draw (ptr_window->buffer, 1);
        if (SERVER(ptr_window->buffer) == server)
            ptr_window->buffer->last_read_line =
                ptr_window->buffer->last_line;
    }
    return 0;
}

/*
 * irc_cmd_recv_307: '307' command (whois, registered nick)
 */

int
irc_cmd_recv_307 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_nick, *pos_msg;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
        pos_nick = strchr (arguments, ' ');
        if (pos_nick)
        {
            while (pos_nick[0] == ' ')
                pos_nick++;
            pos_msg = strchr (pos_nick, ' ');
            if (pos_msg)
            {
                pos_msg[0] = '\0';
                pos_msg++;
                while (pos_msg[0] == ' ')
                    pos_msg++;
                if (pos_msg[0] == ':')
                    pos_msg++;
                
                irc_display_prefix (server, server->buffer, PREFIX_SERVER);
                gui_printf (server->buffer, "%s[%s%s%s] %s%s\n",
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT_NICK),
                            pos_nick,
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT),
                            pos_msg);
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_311: '311' command (whois, user)
 */

int
irc_cmd_recv_311 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_nick, *pos_user, *pos_host, *pos_realname;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
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
                        
                        irc_display_prefix (server, server->buffer, PREFIX_SERVER);
                        gui_printf (server->buffer,
                                    "%s[%s%s%s] (%s%s@%s%s)%s: %s\n",
                                    GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                    GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                    pos_nick,
                                    GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                    GUI_COLOR(COLOR_WIN_CHAT_HOST),
                                    pos_user,
                                    pos_host,
                                    GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                    GUI_COLOR(COLOR_WIN_CHAT),
                                    pos_realname);
                    }
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
irc_cmd_recv_312 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_nick, *pos_server, *pos_serverinfo;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
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
                    
                    irc_display_prefix (server, server->buffer, PREFIX_SERVER);
                    gui_printf (server->buffer,
                                "%s[%s%s%s] %s%s %s(%s%s%s)\n",
                                GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                pos_nick,
                                GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                GUI_COLOR(COLOR_WIN_CHAT),
                                pos_server,
                                GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                GUI_COLOR(COLOR_WIN_CHAT),
                                pos_serverinfo,
                                GUI_COLOR(COLOR_WIN_CHAT_DARK));
                }
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_313: '313' command (whois, operator)
 */

int
irc_cmd_recv_313 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_nick, *pos_message;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
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
                
                irc_display_prefix (server, server->buffer, PREFIX_SERVER);
                gui_printf (server->buffer, "%s[%s%s%s] %s%s\n",
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT_NICK),
                            pos_nick,
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT),
                            pos_message);
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_314: '314' command (whowas)
 */

int
irc_cmd_recv_314 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_nick, *pos_user, *pos_host, *pos_realname;
    
    /* make gcc happy */
    (void) host;
    (void) nick;

    if (!command_ignored)
    {
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
                            
                            irc_display_prefix (server, server->buffer, PREFIX_SERVER);
                            gui_printf (server->buffer,
                                        _("%s%s %s(%s%s@%s%s)%s was %s\n"),
                                        GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                        pos_nick,
                                        GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                        GUI_COLOR(COLOR_WIN_CHAT_HOST),
                                        pos_user,
                                        pos_host,
                                        GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                        GUI_COLOR(COLOR_WIN_CHAT),
                                        pos_realname);
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_315: '315' command (end of /who)
 */

int
irc_cmd_recv_315 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos;
    t_irc_channel *ptr_channel;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    /* skip nickname if at beginning of server message */
    if (strncmp (server->nick, arguments, strlen (server->nick)) == 0)
    {
        arguments += strlen (server->nick) + 1;
        while (arguments[0] == ' ')
            arguments++;
    }
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        ptr_channel = channel_search (server, arguments);
        if (ptr_channel && (ptr_channel->checking_away > 0))
        {
            ptr_channel->checking_away--;
            return 0;
        }
        if (!command_ignored)
        {
            irc_display_prefix (server, server->buffer, PREFIX_SERVER);
            gui_printf (server->buffer, "%s%s %s%s\n",
                        GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                        arguments,
                        GUI_COLOR(COLOR_WIN_CHAT),
                        pos);
        }
    }
    else
    {
        if (!command_ignored)
        {
            irc_display_prefix (server, server->buffer, PREFIX_SERVER);
            gui_printf (server->buffer, "%s\n", arguments);
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_317: '317' command (whois, idle)
 */

int
irc_cmd_recv_317 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_nick, *pos_idle, *pos_signon, *pos_message;
    int idle_time, day, hour, min, sec;
    time_t datetime;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
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
                    
                        irc_display_prefix (server, server->buffer, PREFIX_SERVER);
                        gui_printf (server->buffer, _("%s[%s%s%s]%s idle: "),
                                    GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                    GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                    pos_nick,
                                    GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                    GUI_COLOR(COLOR_WIN_CHAT));
                        
                        if (day > 0)
                            gui_printf (server->buffer, "%s%d %s%s, ",
                                        GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                        day,
                                        GUI_COLOR(COLOR_WIN_CHAT),
                                        (day > 1) ? _("days") : _("day"));
                        
                        datetime = (time_t)(atol (pos_signon));
                        gui_printf (server->buffer,
                                    _("%s%02d %s%s %s%02d %s%s %s%02d %s%s, signon at: %s%s"),
                                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                    hour,
                                    GUI_COLOR(COLOR_WIN_CHAT),
                                    (hour > 1) ? _("hours") : _("hour"),
                                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                    min,
                                    GUI_COLOR(COLOR_WIN_CHAT),
                                    (min > 1) ? _("minutes") : _("minute"),
                                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                    sec,
                                    GUI_COLOR(COLOR_WIN_CHAT),
                                    (sec > 1) ? _("seconds") : _("second"),
                                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                    ctime (&datetime));
                    }
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
irc_cmd_recv_318 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_nick, *pos_message;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
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
            
                irc_display_prefix (server, server->buffer, PREFIX_SERVER);
                gui_printf (server->buffer, "%s[%s%s%s] %s%s\n",
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT_NICK),
                            pos_nick,
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT),
                            pos_message);
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_319: '319' command (whois, end)
 */

int
irc_cmd_recv_319 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_nick, *pos_channel, *pos;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
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
            
                irc_display_prefix (server, server->buffer, PREFIX_SERVER);
                gui_printf (server->buffer, "%s[%s%s%s]%s Channels: ",
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT_NICK),
                            pos_nick,
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT));
                while (pos_channel && pos_channel[0])
                {
                    if (pos_channel[0] == '@')
                    {
                        gui_printf (server->buffer, "%s@",
                                    GUI_COLOR(COLOR_WIN_NICK_OP));
                        pos_channel++;
                    }
                    else
                    {
                        if (pos_channel[0] == '%')
                        {
                            gui_printf (server->buffer, "%s%%",
                                        GUI_COLOR(COLOR_WIN_NICK_HALFOP));
                            pos_channel++;
                        }
                        else
                            if (pos_channel[0] == '+')
                            {
                                gui_printf (server->buffer, "%s+",
                                            GUI_COLOR(COLOR_WIN_NICK_VOICE));
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
                    gui_printf (server->buffer, "%s%s%s",
                                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                pos_channel,
                                (pos && pos[0]) ? " " : "\n");
                    pos_channel = pos;
                }
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_320: '320' command (whois, identified user)
 */

int
irc_cmd_recv_320 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_nick, *pos_message;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
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
                
                irc_display_prefix (server, server->buffer, PREFIX_SERVER);
                gui_printf (server->buffer, "%s[%s%s%s] %s%s\n",
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT_NICK),
                            pos_nick,
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT),
                            pos_message);
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_321: '321' command (/list start)
 */

int
irc_cmd_recv_321 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
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
        
        irc_display_prefix (server, server->buffer, PREFIX_SERVER);
        gui_printf (server->buffer, "%s\n", pos);
    }
    return 0;
}

/*
 * irc_cmd_recv_322: '322' command (channel for /list)
 */

int
irc_cmd_recv_322 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
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
        
	if (server->cmd_list_regexp)
	{
	    if (regexec (server->cmd_list_regexp, pos, 0, NULL, 0) == 0) {
		irc_display_prefix (server, server->buffer, PREFIX_SERVER);
		gui_printf (server->buffer, "%s\n", pos);
	    }
	}
	else
	{
	    irc_display_prefix (server, server->buffer, PREFIX_SERVER);	    
	    gui_printf (server->buffer, "%s\n", pos);
	}
    }
    return 0;
}

/*
 * irc_cmd_recv_323: '323' command (/list end)
 */

int
irc_cmd_recv_323 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
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
        
        irc_display_prefix (server, server->buffer, PREFIX_SERVER);
        gui_printf (server->buffer, "%s\n", pos);
    }
    return 0;
}

/*
 * irc_cmd_recv_324: '324' command (channel mode)
 */

int
irc_cmd_recv_324 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_channel, *pos, *pos_parm;
    t_irc_channel *ptr_channel;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    pos_channel = strchr (arguments, ' ');
    if (pos_channel)
    {
        pos_channel[0] = '\0';
        pos_channel++;
        while (pos_channel[0] == ' ')
            pos_channel++;
        
        pos = strchr (pos_channel, ' ');
        if (pos)
        {
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
            }
            ptr_channel = channel_search (server, pos_channel);
            if (ptr_channel)
            {
                irc_get_channel_modes (server, ptr_channel, NULL, NULL,
                                       pos, pos_parm, command_ignored);
                gui_status_draw (ptr_channel->buffer, 0);
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_329: '329' command (???)
 */

int
irc_cmd_recv_329 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    /* make gcc happy */
    (void) server;
    (void) host;
    (void) nick;
    (void) arguments;
    
    return 0;
}

/*
 * irc_cmd_recv_331: '331' command received (no topic for channel)
 */

int
irc_cmd_recv_331 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_channel, *pos;
    t_irc_channel *ptr_channel;
    
    /* make gcc happy */
    (void) server;
    (void) nick;
    
    if (!command_ignored)
    {
        pos_channel = strchr (arguments, ' ');
        if (pos_channel)
        {
            pos_channel++;
            while (pos_channel[0] == ' ')
                pos_channel++;
            pos = strchr (pos_channel, ' ');
            if (pos)
                pos[0] = '\0';
        }
        else
        {
            irc_display_prefix (server, server->buffer, PREFIX_ERROR);
            gui_printf_nolog (server->buffer,
                              _("%s channel \"%s\" not found for \"%s\" command\n"),
                              WEECHAT_ERROR, "", "331");
            return -1;
        }
        
        ptr_channel = channel_search (server, pos_channel);
        if (ptr_channel)
        {
            command_ignored |= ignore_check (host, "331", ptr_channel->name, server->name);
            if (!command_ignored)
            {
                irc_display_prefix (server, ptr_channel->buffer, PREFIX_INFO);
                gui_printf (ptr_channel->buffer, _("No topic set for %s%s\n"),
                            GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                            pos_channel);
            }
        }
        else
        {
            irc_display_prefix (server, server->buffer, PREFIX_ERROR);
            gui_printf_nolog (server->buffer,
                              _("%s channel \"%s\" not found for \"%s\" command\n"),
                              WEECHAT_ERROR, pos_channel, "331");
            return -1;
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_332: '332' command received (topic of channel)
 */

int
irc_cmd_recv_332 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos, *pos2;
    t_irc_channel *ptr_channel;
    t_gui_buffer *ptr_buffer;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
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
            ptr_buffer = (ptr_channel) ?
                ptr_channel->buffer : server->buffer;
            pos2++;
            while (pos2[0] == ' ')
                pos2++;
            if (pos2[0] == ':')
                pos2++;
            if (ptr_channel)
            {
                if (ptr_channel->topic)
                    free (ptr_channel->topic);
                ptr_channel->topic = strdup (pos2);
            }
            
            command_ignored |= ignore_check (host, "332", pos, server->name);
            if (!command_ignored)
            {
                irc_display_prefix (server, ptr_buffer, PREFIX_INFO);
                gui_printf (ptr_buffer, _("Topic for %s%s%s is: "),
                            GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                            pos,
                            GUI_COLOR(COLOR_WIN_CHAT));
                gui_printf (ptr_buffer, "\"%s\"\n", pos2);
            }
            
            if (ptr_channel)
                gui_chat_draw_title (ptr_buffer, 1);
        }
    }
    else
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
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
irc_cmd_recv_333 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_channel, *pos_nick, *pos_date;
    t_irc_channel *ptr_channel;
    t_gui_buffer *ptr_buffer;
    time_t datetime;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
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
                ptr_buffer = (ptr_channel) ?
                    ptr_channel->buffer : server->buffer;
                
                command_ignored |= ignore_check (host, "333",
                                                 pos_channel, server->name);
                if (!command_ignored)
                {
                    datetime = (time_t)(atol (pos_date));
                    irc_display_prefix (server, ptr_buffer, PREFIX_INFO);
                    gui_printf (ptr_buffer, _("Topic set by %s%s%s, %s"),
                                GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                pos_nick,
                                GUI_COLOR(COLOR_WIN_CHAT),
                                ctime (&datetime));
                }
            }
            else
            {
                irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                gui_printf_nolog (server->buffer,
                                  _("%s cannot identify date/time for \"%s\" command\n"),
                                  WEECHAT_ERROR, "333");
                return -1;
            }
        }
        else
        {
            irc_display_prefix (server, server->buffer, PREFIX_ERROR);
            gui_printf_nolog (server->buffer,
                              _("%s cannot identify nickname for \"%s\" command\n"),
                              WEECHAT_ERROR, "333");
            return -1;
        }
    }
    else
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s cannot identify channel for \"%s\" command\n"),
                          WEECHAT_ERROR, "333");
        return -1;
    }
    return 0;
}

/*
 * irc_cmd_recv_338: '338' command (whois, host)
 */

int
irc_cmd_recv_338 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_nick, *pos_host, *pos_message;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
        pos_nick = strchr (arguments, ' ');
        if (pos_nick)
        {
            while (pos_nick[0] == ' ')
                pos_nick++;
            pos_host = strchr (pos_nick, ' ');
            if (pos_host)
            {
                pos_host[0] = '\0';
                pos_host++;
                while (pos_host[0] == ' ')
                    pos_host++;
                pos_message = strchr (pos_host, ' ');
                if (pos_message)
                {
                    pos_message[0] = '\0';
                    pos_message++;
                    while (pos_message[0] == ' ')
                        pos_message++;
                    if (pos_message[0] == ':')
                        pos_message++;
                    
                    irc_display_prefix (server, server->buffer, PREFIX_SERVER);
                    gui_printf (server->buffer, "%s[%s%s%s] %s%s %s%s %s%s\n",
                                GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                pos_nick,
                                GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                pos_nick,
                                GUI_COLOR(COLOR_WIN_CHAT),
                                pos_message,
                                GUI_COLOR(COLOR_WIN_CHAT_HOST),
                                pos_host);
                }
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_341: '341' command received (inviting)
 */

int
irc_cmd_recv_341 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_nick, *pos_channel;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        pos_nick[0] = '\0';
        pos_nick++;
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
            
            irc_display_prefix (server, server->buffer, PREFIX_SERVER);
            gui_printf (server->buffer,
                        _("%s%s%s has invited %s%s%s on %s%s\n"),
                        GUI_COLOR(COLOR_WIN_CHAT_NICK),
                        arguments,
                        GUI_COLOR(COLOR_WIN_CHAT),
                        GUI_COLOR(COLOR_WIN_CHAT_NICK),
                        pos_nick,
                        GUI_COLOR(COLOR_WIN_CHAT),
                        GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                        pos_channel);
            gui_status_draw (gui_current_window->buffer, 1);
        }
        else
        {
            irc_display_prefix (server, server->buffer, PREFIX_ERROR);
            gui_printf_nolog (server->buffer,
                              _("%s cannot identify channel for \"%s\" command\n"),
                              WEECHAT_ERROR, "341");
            return -1;
        }
    }
    else
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s cannot identify nickname for \"%s\" command\n"),
                          WEECHAT_ERROR, "341");
        return -1;
    }
    return 0;
}

/*
 * irc_cmd_recv_344: '344' command (channel reop)
 */

int
irc_cmd_recv_344 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_channel, *pos_host;
    
    /* make gcc happy */
    (void) host;
    (void) nick;

    if (!command_ignored)
    {
        pos_channel = strchr (arguments, ' ');
        if (pos_channel)
        {
            while (pos_channel[0] == ' ')
                pos_channel++;
            pos_host = strchr (pos_channel, ' ');
            if (pos_host)
            {
                pos_host[0] = '\0';
                pos_host++;
                while (pos_host[0] == ' ')
                    pos_host++;
                
                irc_display_prefix (server, server->buffer, PREFIX_SERVER);
                gui_printf (server->buffer, _("Channel reop %s%s%s: %s%s\n"),
                            GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                            pos_channel,
                            GUI_COLOR(COLOR_WIN_CHAT),
                            GUI_COLOR(COLOR_WIN_CHAT_HOST),
                            pos_host);
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_345: '345' command (end of channel reop)
 */

int
irc_cmd_recv_345 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    /* skip nickname if at beginning of server message */
    if (strncmp (server->nick, arguments, strlen (server->nick)) == 0)
    {
        arguments += strlen (server->nick) + 1;
        while (arguments[0] == ' ')
            arguments++;
    }
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        if (!command_ignored)
        {
            irc_display_prefix (server, server->buffer, PREFIX_SERVER);
            gui_printf (server->buffer, "%s%s %s%s\n",
                        GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                        arguments,
                        GUI_COLOR(COLOR_WIN_CHAT),
                        pos);
        }
    }
    else
    {
        if (!command_ignored)
        {
            irc_display_prefix (server, server->buffer, PREFIX_SERVER);
            gui_printf (server->buffer, "%s\n", arguments);
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_348: '348' command received (channel exception list)
 */

int
irc_cmd_recv_348 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_channel, *pos_exception, *pos_user, *pos_date, *pos;
    t_irc_channel *ptr_channel;
    t_gui_buffer *buffer;
    time_t datetime;
    
    /* make gcc happy */
    (void) nick;
    
    /* look for channel */
    pos_channel = strchr (arguments, ' ');
    if (!pos_channel)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s cannot parse \"%s\" command\n"),
                          WEECHAT_ERROR, "348");
        return -1;
    }
    pos_channel[0] = '\0';
    pos_channel++;
    while (pos_channel[0] == ' ')
        pos_channel++;
    
    /* look for exception mask */
    pos_exception = strchr (pos_channel, ' ');
    if (!pos_exception)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s cannot parse \"%s\" command\n"),
                          WEECHAT_ERROR, "348");
        return -1;
    }
    pos_exception[0] = '\0';
    pos_exception++;
    while (pos_exception[0] == ' ')
        pos_exception++;
    
    /* look for user who set exception */
    pos_user = strchr (pos_exception, ' ');
    if (!pos_user)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s cannot parse \"%s\" command\n"),
                          WEECHAT_ERROR, "348");
        return -1;
    }
    pos_user[0] = '\0';
    pos_user++;
    while (pos_user[0] == ' ')
        pos_user++;
    
    /* look for date/time */
    pos_date = strchr (pos_user, ' ');
    if (!pos_date)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s cannot parse \"%s\" command\n"),
                          WEECHAT_ERROR, "348");
        return -1;
    }
    pos_date[0] = '\0';
    pos_date++;
    while (pos_date[0] == ' ')
        pos_date++;
    
    if (!pos_date || !pos_date[0])
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s cannot parse \"%s\" command\n"),
                          WEECHAT_ERROR, "348");
        return -1;
    }
    
    ptr_channel = channel_search (server, pos_channel);
    buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
    
    command_ignored |= ignore_check (host, "348", pos_channel, server->name);
    
    if (!command_ignored)
    {
        irc_display_prefix (server, buffer, PREFIX_INFO);
        gui_printf (buffer, "%s[%s%s%s]%s exception %s%s%s by ",
                    GUI_COLOR(COLOR_WIN_CHAT_DARK),
                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                    pos_channel,
                    GUI_COLOR(COLOR_WIN_CHAT_DARK),
                    GUI_COLOR(COLOR_WIN_CHAT),
                    GUI_COLOR(COLOR_WIN_CHAT_HOST),
                    pos_exception,
                    GUI_COLOR(COLOR_WIN_CHAT));
        pos = strchr (pos_user, '!');
        if (pos)
        {
            pos[0] = '\0';
            gui_printf (buffer, "%s%s %s(%s%s%s)",
                        GUI_COLOR(COLOR_WIN_CHAT_NICK),
                        pos_user,
                        GUI_COLOR(COLOR_WIN_CHAT_DARK),
                        GUI_COLOR(COLOR_WIN_CHAT_HOST),
                        pos + 1,
                        GUI_COLOR(COLOR_WIN_CHAT_DARK));
        }
        else
            gui_printf (buffer,"%s%s",
                        GUI_COLOR(COLOR_WIN_CHAT_NICK),
                        pos_user);
        datetime = (time_t)(atol (pos_date));
        gui_printf_nolog (buffer, "%s, %s",
                          GUI_COLOR(COLOR_WIN_CHAT),
                          ctime (&datetime));
    }    
    return 0;
}

/*
 * irc_cmd_recv_349: '349' command received (end of channel exception list)
 */

int
irc_cmd_recv_349 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_channel, *pos_msg;
    t_irc_channel *ptr_channel;
    t_gui_buffer *buffer;
    
    /* make gcc happy */
    (void) nick;
    
    pos_channel = strchr (arguments, ' ');
    if (!pos_channel)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s cannot parse \"%s\" command\n"),
                          WEECHAT_ERROR, "349");
        return -1;
    }
    pos_channel[0] = '\0';
    pos_channel++;
    while (pos_channel[0] == ' ')
        pos_channel++;
    
    pos_msg = strchr (pos_channel, ' ');
    if (!pos_msg)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s cannot parse \"%s\" command\n"),
                          WEECHAT_ERROR, "349");
        return -1;
    }
    pos_msg[0] = '\0';
    pos_msg++;
    while (pos_msg[0] == ' ')
        pos_msg++;
    if (pos_msg[0] == ':')
        pos_msg++;
    
    ptr_channel = channel_search (server, pos_channel);
    buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;

    command_ignored |= ignore_check (host, "349", pos_channel, server->name);
    
    if (!command_ignored)
    {
        irc_display_prefix (server, buffer, PREFIX_INFO);
        gui_printf (buffer, "%s[%s%s%s] %s%s\n",
                    GUI_COLOR(COLOR_WIN_CHAT_DARK),
                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                    pos_channel,
                    GUI_COLOR(COLOR_WIN_CHAT_DARK),
                    GUI_COLOR(COLOR_WIN_CHAT),
                    pos_msg);
    }
    return 0;
}

/*
 * irc_cmd_recv_351: '351' command received (server version)
 */

int
irc_cmd_recv_351 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos, *pos2;
    
    /* make gcc happy */
    (void) server;
    (void) host;
    (void) nick;
    
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
    
    if (!command_ignored)
    {
        irc_display_prefix (server, server->buffer, PREFIX_SERVER);
        if (pos2)
            gui_printf (server->buffer, "%s %s\n", pos, pos2);
        else
            gui_printf (server->buffer, "%s\n", pos);
    }
    return 0;
}

/*
 * irc_cmd_recv_352: '352' command (who)
 */

int
irc_cmd_recv_352 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_channel, *pos_user, *pos_host, *pos_server, *pos_nick;
    char *pos_attr, *pos_hopcount, *pos_realname;
    int length;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
    /* make gcc happy */
    (void) nick;
    
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
                                    
                                    command_ignored |= ignore_check (host, "352", pos_channel, server->name);
                                    
                                    ptr_channel = channel_search (server, pos_channel);
                                    if (ptr_channel && (ptr_channel->checking_away > 0))
                                    {
                                        ptr_nick = nick_search (ptr_channel, pos_nick);
                                        if (ptr_nick)
                                        {
                                            if (ptr_nick->host)
                                                free (ptr_nick->host);
                                            length = strlen (pos_user) + 1 + strlen (pos_host) + 1;
                                            ptr_nick->host = (char *) malloc (length);
                                            if (ptr_nick->host)
                                                snprintf (ptr_nick->host, length, "%s@%s", pos_user, pos_host);
                                            nick_set_away (ptr_channel, ptr_nick,
                                                           (pos_attr[0] == 'G') ? 1 : 0);
                                        }
                                        return 0;
                                    }
                                    
                                    if (!command_ignored)
                                    {
                                        irc_display_prefix (server, server->buffer,
                                                            PREFIX_SERVER);
                                        gui_printf (server->buffer,
                                                    "%s%s%s on %s%s%s %s %s %s%s@%s %s(%s%s%s)\n",
                                                    GUI_COLOR(COLOR_WIN_CHAT_NICK),
                                                    pos_nick,
                                                    GUI_COLOR(COLOR_WIN_CHAT),
                                                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                                    pos_channel,
                                                    GUI_COLOR(COLOR_WIN_CHAT),
                                                    pos_attr,
                                                    pos_hopcount,
                                                    GUI_COLOR(COLOR_WIN_CHAT_HOST),
                                                    pos_user,
                                                    pos_host,
                                                    GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                                    GUI_COLOR(COLOR_WIN_CHAT),
                                                    pos_realname,
                                                    GUI_COLOR(COLOR_WIN_CHAT_DARK));
                                    }
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
irc_cmd_recv_353 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos, *pos_nick;
    int is_chanowner, is_chanadmin, is_op, is_halfop, has_voice;
    t_irc_channel *ptr_channel;
    t_gui_buffer *ptr_buffer;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
        
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
        if (ptr_channel)
            ptr_buffer = ptr_channel->buffer;
        else
            ptr_buffer = server->buffer;
        
        pos++;
        while (pos[0] == ' ')
            pos++;
        if (pos[0] != ':')
        {
            irc_display_prefix (server, server->buffer, PREFIX_ERROR);
            gui_printf_nolog (server->buffer,
                              _("%s cannot parse \"%s\" command\n"),
                              WEECHAT_ERROR, "353");
            return -1;
        }
        
        command_ignored |= ignore_check (host, "353",
                                         (ptr_channel) ? ptr_channel->name : NULL,
                                         server->name);
        
        /* channel is not joined => display users on server buffer */
        if (!command_ignored && !ptr_channel)
        {
            /* display users on channel */
            irc_display_prefix (server, ptr_buffer, PREFIX_SERVER);
            gui_printf (ptr_buffer, _("Nicks %s%s%s: %s["),
                        GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                        arguments,
                        GUI_COLOR(COLOR_WIN_CHAT),
                        GUI_COLOR(COLOR_WIN_CHAT_DARK));
        }
        
        pos++;
        if (pos[0])
        {
            while (pos && pos[0])
            {
                is_chanowner = 0;
                is_chanadmin = 0;
                is_op = 0;
                is_halfop = 0;
                has_voice = 0;
                while ((pos[0] == '@') || (pos[0] == '%') || (pos[0] == '+'))
                {
                    if (pos[0] == '@')
                    {
                        is_op = 1;
                        if (!command_ignored && !ptr_channel)
                            gui_printf (ptr_buffer, "%s@",
                                        GUI_COLOR(COLOR_WIN_NICK_OP));
                    }
                    if (pos[0] == '%')
                    {
                        is_halfop = 1;
                        if (!command_ignored && !ptr_channel)
                            gui_printf (ptr_buffer, "%s%%",
                                        GUI_COLOR(COLOR_WIN_NICK_HALFOP));
                    }
                    if (pos[0] == '+')
                    {
                        has_voice = 1;
                        if (!command_ignored && !ptr_channel)
                            gui_printf (ptr_buffer, "%s+",
                                        GUI_COLOR(COLOR_WIN_NICK_VOICE));
                    }
                    pos++;
                }
                if (pos[0] == '~')
                {
                    is_chanowner = 1;
                    pos++;
                    if (!command_ignored && !ptr_channel)
                        gui_printf (ptr_buffer, "%s~",
                                    GUI_COLOR(COLOR_WIN_NICK_OP));
                }
                if (pos[0] == '&')
                {
                    is_chanadmin = 1;
                    pos++;
                    if (!command_ignored && !ptr_channel)
                        gui_printf (ptr_buffer, "%s&",
                                    GUI_COLOR(COLOR_WIN_NICK_OP));
                }
                pos_nick = pos;
                pos = strchr (pos, ' ');
                if (pos)
                {
                    pos[0] = '\0';
                    pos++;
                }
                if (ptr_channel)
                {
                    if (!nick_new (server, ptr_channel, pos_nick, is_chanowner,
                                   is_chanadmin, is_op, is_halfop, has_voice))
                    {
                        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                        gui_printf_nolog (server->buffer,
                                          _("%s cannot create nick \"%s\" for channel \"%s\"\n"),
                                          WEECHAT_ERROR, pos_nick, ptr_channel->name);
                    }
                }
                else
                {
                    if (!command_ignored)
                    {
                        gui_printf (ptr_buffer, "%s%s",
                                    GUI_COLOR(COLOR_WIN_CHAT), pos_nick);
                        if (pos && pos[0])
                            gui_printf (ptr_buffer, " ");
                    }
                }
            }
        }
        if (ptr_channel)
        {
            gui_nicklist_draw (ptr_channel->buffer, 1, 1);
            gui_status_draw (ptr_channel->buffer, 1);
        }
        else
        {
            if (!command_ignored)
                gui_printf (ptr_buffer, "%s]\n",
                            GUI_COLOR(COLOR_WIN_CHAT_DARK));
        }
    }
    else
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
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
irc_cmd_recv_366 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos, *pos2;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    int num_nicks, num_op, num_halfop, num_voice, num_normal;
    
    /* make gcc happy */
    (void) nick;
    
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
                command_ignored |= ignore_check (host, "366", ptr_channel->name, server->name);
                
                if (!command_ignored)
                {
                    /* display users on channel */
                    irc_display_prefix (server, ptr_channel->buffer, PREFIX_SERVER);
                    gui_printf (ptr_channel->buffer, _("Nicks %s%s%s: %s["),
                                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                ptr_channel->name,
                                GUI_COLOR(COLOR_WIN_CHAT),
                                GUI_COLOR(COLOR_WIN_CHAT_DARK));
                    
                    for (ptr_nick = ptr_channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
                    {
                        irc_display_nick (ptr_channel->buffer, ptr_nick, NULL,
                                          MSG_TYPE_MSG, 0, COLOR_WIN_CHAT, 1);
                        if (ptr_nick != ptr_channel->last_nick)
                            gui_printf (ptr_channel->buffer, " ");
                    }
                    gui_printf (ptr_channel->buffer, "%s]\n",
                                GUI_COLOR(COLOR_WIN_CHAT_DARK));
                    
                    /* display number of nicks, ops, halfops & voices on the channel */
                    nick_count (ptr_channel, &num_nicks, &num_op, &num_halfop, &num_voice,
                                &num_normal);
                    irc_display_prefix (server, ptr_channel->buffer, PREFIX_INFO);
                    gui_printf (ptr_channel->buffer,
                                _("Channel %s%s%s: %s%d%s %s %s(%s%d%s %s, "
                                  "%s%d%s %s, %s%d%s %s, %s%d%s %s%s)\n"),
                                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                ptr_channel->name,
                                GUI_COLOR(COLOR_WIN_CHAT),
                                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                num_nicks,
                                GUI_COLOR(COLOR_WIN_CHAT),
                                (num_nicks > 1) ? _("nicks") : _("nick"),
                                GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                num_op,
                                GUI_COLOR(COLOR_WIN_CHAT),
                                (num_op > 1) ? _("ops") : _("op"),
                                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                num_halfop,
                                GUI_COLOR(COLOR_WIN_CHAT),
                                (num_halfop > 1) ? _("halfops") : _("halfop"),
                                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                num_voice,
                                GUI_COLOR(COLOR_WIN_CHAT),
                                (num_voice > 1) ? _("voices") : _("voice"),
                                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                num_normal,
                                GUI_COLOR(COLOR_WIN_CHAT),
                                _("normal"),
                                GUI_COLOR(COLOR_WIN_CHAT_DARK));
                }
                irc_cmd_send_mode (server, NULL, ptr_channel->name);
                channel_check_away (server, ptr_channel, 1);
            }
            else
            {
                if (!command_ignored)
                {
                    irc_display_prefix (server, gui_current_window->buffer, PREFIX_INFO);
                    gui_printf (gui_current_window->buffer, "%s%s%s: %s\n",
                                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                pos,
                                GUI_COLOR(COLOR_WIN_CHAT),
                                pos2);
                }
                return 0;
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_367: '367' command received (banlist)
 */

int
irc_cmd_recv_367 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_channel, *pos_ban, *pos_user, *pos_date, *pos;
    t_irc_channel *ptr_channel;
    t_gui_buffer *buffer;
    time_t datetime;
    
    /* make gcc happy */
    (void) nick;
    
    /* look for channel */
    pos_channel = strchr (arguments, ' ');
    if (!pos_channel)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s cannot parse \"%s\" command\n"),
                          WEECHAT_ERROR, "367");
        return -1;
    }
    pos_channel[0] = '\0';
    pos_channel++;
    while (pos_channel[0] == ' ')
        pos_channel++;
    
    /* look for ban mask */
    pos_ban = strchr (pos_channel, ' ');
    if (!pos_ban)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s cannot parse \"%s\" command\n"),
                          WEECHAT_ERROR, "367");
        return -1;
    }
    pos_ban[0] = '\0';
    pos_ban++;
    while (pos_ban[0] == ' ')
        pos_ban++;
    
    /* look for user who set ban */
    pos_date = NULL;
    pos_user = strchr (pos_ban, ' ');
    if (pos_user)
    {
        pos_user[0] = '\0';
        pos_user++;
        while (pos_user[0] == ' ')
            pos_user++;
        
        /* look for date/time */
        pos_date = strchr (pos_user, ' ');
        if (pos_date)
        {
            pos_date[0] = '\0';
            pos_date++;
            while (pos_date[0] == ' ')
                pos_date++;   
        }
    }
    
    ptr_channel = channel_search (server, pos_channel);
    buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
    
    command_ignored |= ignore_check (host, "367", pos_channel, server->name);
    
    if (!command_ignored)
    {
        irc_display_prefix (server, buffer, PREFIX_INFO);
        if (pos_user)
        {
            gui_printf_nolog (buffer, _("%s[%s%s%s] %s%s%s banned by "),
                              GUI_COLOR(COLOR_WIN_CHAT_DARK),
                              GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                              pos_channel,
                              GUI_COLOR(COLOR_WIN_CHAT_DARK),
                              GUI_COLOR(COLOR_WIN_CHAT_HOST),
                              pos_ban,
                              GUI_COLOR(COLOR_WIN_CHAT));
            pos = strchr (pos_user, '!');
            if (pos)
            {
                pos[0] = '\0';
                gui_printf (buffer, "%s%s %s(%s%s%s)",
                            GUI_COLOR(COLOR_WIN_CHAT_NICK),
                            pos_user,
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT_HOST),
                            pos + 1,
                            GUI_COLOR(COLOR_WIN_CHAT_DARK));
            }
            else
                gui_printf (buffer, "%s%s",
                            GUI_COLOR(COLOR_WIN_CHAT_NICK),
                            pos_user);
            if (pos_date)
            {
                datetime = (time_t)(atol (pos_date));
                gui_printf (buffer, "%s, %s",
                            GUI_COLOR(COLOR_WIN_CHAT),
                            ctime (&datetime));
            }
            else
                gui_printf (buffer, "\n");
        }
        else
            gui_printf_nolog (buffer, _("%s[%s%s%s] %s%s%s banned\n"),
                              GUI_COLOR(COLOR_WIN_CHAT_DARK),
                              GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                              pos_channel,
                              GUI_COLOR(COLOR_WIN_CHAT_DARK),
                              GUI_COLOR(COLOR_WIN_CHAT_HOST),
                              pos_ban,
                              GUI_COLOR(COLOR_WIN_CHAT));
    }    
    return 0;
}

/*
 * irc_cmd_recv_368: '368' command received (end of banlist)
 */

int
irc_cmd_recv_368 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_channel, *pos_msg;
    t_irc_channel *ptr_channel;
    t_gui_buffer *buffer;
    
    /* make gcc happy */
    (void) nick;
    
    pos_channel = strchr (arguments, ' ');
    if (!pos_channel)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s cannot parse \"%s\" command\n"),
                          WEECHAT_ERROR, "368");
        return -1;
    }
    pos_channel[0] = '\0';
    pos_channel++;
    while (pos_channel[0] == ' ')
        pos_channel++;
    
    pos_msg = strchr (pos_channel, ' ');
    if (!pos_msg)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf_nolog (server->buffer,
                          _("%s cannot parse \"%s\" command\n"),
                          WEECHAT_ERROR, "368");
        return -1;
    }
    pos_msg[0] = '\0';
    pos_msg++;
    while (pos_msg[0] == ' ')
        pos_msg++;
    if (pos_msg[0] == ':')
        pos_msg++;
    
    ptr_channel = channel_search (server, pos_channel);
    buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;

    command_ignored |= ignore_check (host, "368", pos_channel, server->name);
    
    if (!command_ignored)
    {
        irc_display_prefix (server, buffer, PREFIX_INFO);
        gui_printf_nolog (buffer, "%s[%s%s%s] %s%s\n",
                          GUI_COLOR(COLOR_WIN_CHAT_DARK),
                          GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                          pos_channel,
                          GUI_COLOR(COLOR_WIN_CHAT_DARK),
                          GUI_COLOR(COLOR_WIN_CHAT),
                          pos_msg);
    }    
    return 0;
}

/*
 * irc_cmd_recv_378: '378' command received (connecting from)
 */

int
irc_cmd_recv_378 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos, *pos2;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
        /* skip nickname if at beginning of server message */
        if (strncmp (server->nick, arguments, strlen (server->nick)) == 0)
        {
            arguments += strlen (server->nick) + 1;
            while (arguments[0] == ' ')
                arguments++;
        }
        
        irc_display_prefix (server, server->buffer, PREFIX_SERVER);
        pos = strchr (arguments, ':');
        if (pos)
        {
            pos[0] = '\0';
            pos2 = pos - 1;
            while (pos2[0] == ' ')
                pos2--;
            pos2[1] = '\0';
            gui_printf (server->buffer, "%s%s %s\n",
                        GUI_COLOR(COLOR_WIN_CHAT), arguments, pos + 1);
        }
        else
            gui_printf (server->buffer, "%s%s\n",
                        GUI_COLOR(COLOR_WIN_CHAT), arguments);
    }
    
    return 0;
}

/*
 * irc_cmd_recv_433: '433' command received (nickname already in use)
 */

int
irc_cmd_recv_433 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char hostname[128];

    /* Note: this IRC command can not be ignored */
    
    if (!server->is_connected)
    {
        if (strcmp (server->nick, server->nick1) == 0)
        {
            irc_display_prefix (server, server->buffer, PREFIX_INFO);
            gui_printf (server->buffer,
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
                irc_display_prefix (server, server->buffer, PREFIX_INFO);
                gui_printf (server->buffer,
                            _("%s: nickname \"%s\" is already in use, "
                            "trying 3rd nickname \"%s\"\n"),
                            PACKAGE_NAME, server->nick, server->nick3);
                free (server->nick);
                server->nick = strdup (server->nick3);
            }
            else
            {
                if (strcmp (server->nick, server->nick3) == 0)
                {
                    irc_display_prefix (server, server->buffer, PREFIX_INFO);
                    gui_printf (server->buffer,
                                _("%s: all declared nicknames are already in use, "
                                "closing connection with server!\n"),
                                PACKAGE_NAME);
                    server_disconnect (server, 1);
                    return 0;
                }
                else
                {
                    irc_display_prefix (server, server->buffer, PREFIX_INFO);
                    gui_printf (server->buffer,
                                _("%s: nickname \"%s\" is already in use, "
                                "trying 1st nickname \"%s\"\n"),
                                PACKAGE_NAME, server->nick, server->nick1);
                    free (server->nick);
                    server->nick = strdup (server->nick1);
                }
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
        return irc_cmd_recv_error (server, host, nick, arguments);
    
    return 0;
}

/*
 * irc_cmd_recv_438: '438' command received (not authorized to change nickname)
 */

int
irc_cmd_recv_438 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos, *pos2;
    
    /* make gcc happy */
    (void) server;
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
        pos = strchr (arguments, ' ');
        irc_display_prefix (server, server->buffer, PREFIX_SERVER);
        if (pos)
        {
            pos[0] = '\0';
            pos++;
            
            pos2 = strstr (pos, " :");
            if (pos2)
            {
                pos2[0] = '\0';
                pos2 += 2;
                gui_printf (server->buffer, "%s (%s => %s)\n", pos2, arguments, pos);
            }
            else
                gui_printf (server->buffer, "%s (%s)\n", pos, arguments);
        }
        else
            gui_printf (server->buffer, "%s\n", arguments);
    }
    return 0;
}

/*
 * irc_cmd_recv_671: '671' command (whois, secure connection)
 */

int
irc_cmd_recv_671 (t_irc_server *server, char *host, char *nick, char *arguments)
{
    char *pos_nick, *pos_message;
    
    /* make gcc happy */
    (void) host;
    (void) nick;
    
    if (!command_ignored)
    {
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
                
                irc_display_prefix (server, server->buffer, PREFIX_SERVER);
                gui_printf (server->buffer, "%s[%s%s%s] %s%s\n",
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT_NICK),
                            pos_nick,
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT),
                            pos_message);
            }
        }
    }
    return 0;
}
