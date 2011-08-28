/*
 * Copyright (C) 2003-2011 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * irc-mode.c: IRC channel/user modes management
 */

#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-mode.h"
#include "irc-server.h"
#include "irc-channel.h"
#include "irc-nick.h"


/*
 * irc_mode_channel_set: set channel modes
 *                       return: 1 if channel modes are updated
 *                               0 if channel modes are NOT updated
 *                                 (no update or on nicks only)
 */

int
irc_mode_channel_set (struct t_irc_server *server,
                      struct t_irc_channel *channel, const char *modes)
{
    char *pos_args, *str_modes, set_flag, **argv, *pos, *ptr_arg;
    int modes_count, channel_modes_updated, argc, current_arg;
    struct t_irc_nick *ptr_nick;
    
    if (!server || !channel || !modes)
        return 0;
    
    channel_modes_updated = 0;
    
    argc = 0;
    argv = NULL;
    pos_args = strchr (modes, ' ');
    if (pos_args)
    {
        str_modes = weechat_strndup (modes, pos_args - modes);
        if (!str_modes)
            return 0;
        pos_args++;
        while (pos_args[0] == ' ')
            pos_args++;
        argv = weechat_string_split (pos_args, " ", 0, 0, &argc);
    }
    else
    {
        str_modes = strdup (modes);
        if (!str_modes)
            return 0;
    }

    /* count number of mode chars */
    modes_count = 0;
    pos = str_modes;
    while (pos && pos[0])
    {
        if ((pos[0] != ':') && (pos[0] != ' ') && (pos[0] != '+')
            && (pos[0] != '-'))
        {
            modes_count++;
        }
        pos++;
    }
    current_arg = argc - modes_count;
    
    if (str_modes && str_modes[0])
    {
        set_flag = '+';
        pos = str_modes;
        while (pos && pos[0])
        {
            switch (pos[0])
            {
                case ':':
                case ' ':
                    break;
                case '+':
                    set_flag = '+';
                    break;
                case '-':
                    set_flag = '-';
                    break;
                case 'b': /* ban (ignored) */
                    current_arg++;
                    break;
                case 'k': /* channel key */
                    if (channel->key)
                    {
                        free (channel->key);
                        channel->key = NULL;
                    }
                    if (set_flag == '+')
                    {
                        ptr_arg = ((current_arg >= 0) && (current_arg < argc)) ?
                            argv[current_arg] : NULL;
                        if (ptr_arg)
                            channel->key = strdup (ptr_arg);
                    }
                    channel_modes_updated = 1;
                    current_arg++;
                    break;
                case 'l': /* channel limit */
                    if (set_flag == '-')
                        channel->limit = 0;
                    if (set_flag == '+')
                    {
                        ptr_arg = ((current_arg >= 0) && (current_arg < argc)) ?
                            argv[current_arg] : NULL;
                        if (ptr_arg)
                            channel->limit = atoi (ptr_arg);
                    }
                    channel_modes_updated = 1;
                    current_arg++;
                    break;
                default:
                    if (irc_server_get_prefix_mode_index (server, pos[0]) >= 0)
                    {
                        ptr_arg = ((current_arg >= 0) && (current_arg < argc)) ?
                            argv[current_arg] : NULL;
                        if (ptr_arg)
                        {
                            ptr_nick = irc_nick_search (channel, ptr_arg);
                            if (ptr_nick)
                            {
                                irc_nick_set_mode (server, channel, ptr_nick,
                                                   (set_flag == '+'), pos[0]);
                            }
                        }
                    }
                    else
                        channel_modes_updated = 1;
                    current_arg++;
                    break;
            }
            pos++;
        }
    }
    
    if (str_modes)
        free (str_modes);
    if (argv)
        weechat_string_free_split (argv);
    
    weechat_bar_item_update ("buffer_name");
    
    return channel_modes_updated;
}

/*
 * irc_mode_user_add: add a user mode
 */

void
irc_mode_user_add (struct t_irc_server *server, char mode)
{
    char str_mode[2], *nick_modes2;

    str_mode[0] = mode;
    str_mode[1] = '\0';
    
    if (server->nick_modes)
    {
        if (!strchr (server->nick_modes, mode))
        {
            nick_modes2 = realloc (server->nick_modes,
                                   strlen (server->nick_modes) + 1 + 1);
            if (!nick_modes2)
            {
                if (server->nick_modes)
                {
                    free (server->nick_modes);
                    server->nick_modes = NULL;
                }
                return;
            }
            server->nick_modes = nick_modes2;
            strcat (server->nick_modes, str_mode);
            weechat_bar_item_update ("input_prompt");
        }
    }
    else
    {
        server->nick_modes = malloc (2);
        strcpy (server->nick_modes, str_mode);
        weechat_bar_item_update ("input_prompt");
    }
}

/*
 * irc_mode_user_remove: remove a user mode
 */

void
irc_mode_user_remove (struct t_irc_server *server, char mode)
{
    char *pos, *nick_modes2;
    int new_size;
    
    if (server->nick_modes)
    {
        pos = strchr (server->nick_modes, mode);
        if (pos)
        {
            new_size = strlen (server->nick_modes);
            memmove (pos, pos + 1, strlen (pos + 1) + 1);
            nick_modes2 = realloc (server->nick_modes, new_size);
            if (nick_modes2)
                server->nick_modes = nick_modes2;
            weechat_bar_item_update ("input_prompt");
        }
    }
}

/*
 * irc_mode_user_set: set user modes
 */

void
irc_mode_user_set (struct t_irc_server *server, const char *modes,
                   int reset_modes)
{
    char set_flag;

    if (reset_modes)
    {
        if (server->nick_modes)
        {
            free (server->nick_modes);
            server->nick_modes = NULL;
        }
    }
    set_flag = '+';
    while (modes && modes[0])
    {
        switch (modes[0])
        {
            case ':':
            case ' ':
                break;
            case '+':
                set_flag = '+';
                break;
            case '-':
                set_flag = '-';
                break;
            default:
                if (set_flag == '+')
                    irc_mode_user_add (server, modes[0]);
                else
                    irc_mode_user_remove (server, modes[0]);
                break;
        }
        modes++;
    }
    weechat_bar_item_update ("input_prompt");
}
