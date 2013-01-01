/*
 * irc-mode.c - IRC channel/user modes management
 *
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-mode.h"
#include "irc-server.h"
#include "irc-channel.h"
#include "irc-nick.h"


/*
 * Gets type of channel mode, which is a letter from 'A' to 'D':
 *   A = Mode that adds or removes a nick or address to a list. Always has a
 *       parameter.
 *   B = Mode that changes a setting and always has a parameter.
 *   C = Mode that changes a setting and only has a parameter when set.
 *   D = Mode that changes a setting and never has a parameter.
 *
 * Example:
 *   CHANMODES=beI,k,l,imnpstaqr  ==>  A = { b, e, I }
 *                                     B = { k }
 *                                     C = { l }
 *                                     D = { i, m, n, p, s, t, a, q, r }
 *
 * Note1: modes of type A return the list when there is no parameter present.
 * Note2: some clients assumes that any mode not listed is of type D.
 * Note3: modes in PREFIX are not listed but could be considered type B.
 *
 * More info: http://www.irc.org/tech_docs/005.html
 */

char
irc_mode_get_chanmode_type (struct t_irc_server *server, char chanmode)
{
    char chanmode_type, *pos;
    const char *chanmodes, *ptr_chanmodes;

    /*
     * assume it is type 'B' if mode is in prefix
     * (we first check that because some exotic servers like irc.webchat.org
     * include the prefix chars in chanmodes as type 'A', which is wrong)
     */
    if (irc_server_get_prefix_mode_index (server, chanmode) >= 0)
        return 'B';

    chanmodes = irc_server_get_chanmodes (server);
    pos = strchr (chanmodes, chanmode);
    if (pos)
    {
        chanmode_type = 'A';
        for (ptr_chanmodes = chanmodes; ptr_chanmodes < pos; ptr_chanmodes++)
        {
            if (ptr_chanmodes[0] == ',')
            {
                if (chanmode_type == 'D')
                    break;
                chanmode_type++;
            }
        }
        return chanmode_type;
    }

    /* unknown mode, type 'D' by default */
    return 'D';
}

/*
 * Updates channel modes using the mode and argument.
 *
 * Example:
 *   if channel modes are "+tn" and that we have:
 *     - set_flag      = '+'
 *     - chanmode      = 'k'
 *     - chanmode_type = 'B'
 *     - argument      = 'password'
 *   then channel modes become:
 *     "+tnk password"
 */

void
irc_mode_channel_update (struct t_irc_server *server,
                         struct t_irc_channel *channel,
                         char set_flag,
                         char chanmode,
                         const char *argument)
{
    char *pos_args, *str_modes, **argv, *pos, *ptr_arg;
    char *new_modes, *new_args, str_mode[2], *str_temp;
    int argc, current_arg, chanmode_found, length;

    if (!channel->modes)
        channel->modes = strdup ("+");
    if (!channel->modes)
        return;

    argc = 0;
    argv = NULL;
    pos_args = strchr (channel->modes, ' ');
    if (pos_args)
    {
        str_modes = weechat_strndup (channel->modes, pos_args - channel->modes);
        if (!str_modes)
            return;
        pos_args++;
        while (pos_args[0] == ' ')
            pos_args++;
        argv = weechat_string_split (pos_args, " ", 0, 0, &argc);
    }
    else
    {
        str_modes = strdup (channel->modes);
        if (!str_modes)
            return;
    }

    new_modes = malloc (strlen (channel->modes) + 1 + 1);
    new_args = malloc (((pos_args) ? strlen (pos_args) : 0)
                       + ((argument) ? 1 + strlen (argument) : 0) + 1);
    if (new_modes && new_args)
    {
        new_modes[0] = '\0';
        new_args[0] = '\0';

        /* loop on current modes and build "new_modes" + "new_args" */
        current_arg = 0;
        chanmode_found = 0;
        pos = str_modes;
        while (pos && pos[0])
        {
            if ((pos[0] == '+') || (pos[0] == '-'))
            {
                str_mode[0] = pos[0];
                str_mode[1] = '\0';
                strcat (new_modes, str_mode);
            }
            else
            {
                ptr_arg = NULL;
                switch (irc_mode_get_chanmode_type (server, pos[0]))
                {
                    case 'A': /* always argument */
                    case 'B': /* always argument */
                    case 'C': /* argumment if set */
                        ptr_arg = (current_arg < argc) ?
                            argv[current_arg] : NULL;
                        break;
                    case 'D': /* no argument */
                        break;
                }
                if (ptr_arg)
                    current_arg++;
                if (pos[0] == chanmode)
                {
                    chanmode_found = 1;
                    if (set_flag == '+')
                    {
                        str_mode[0] = pos[0];
                        str_mode[1] = '\0';
                        strcat (new_modes, str_mode);
                        if (argument)
                        {
                            if (new_args[0])
                                strcat (new_args, " ");
                            strcat (new_args, argument);
                        }
                    }
                }
                else
                {
                    str_mode[0] = pos[0];
                    str_mode[1] = '\0';
                    strcat (new_modes, str_mode);
                    if (ptr_arg)
                    {
                        if (new_args[0])
                            strcat (new_args, " ");
                        strcat (new_args, ptr_arg);
                    }
                }
            }
            pos++;
        }
        if (!chanmode_found)
        {
            /*
             * chanmode was not in channel modes: if set_flag is '+', add
             * it to channel modes
             */
            if (set_flag == '+')
            {
                if (argument)
                {
                    /* add mode with argument at the end of modes */
                    str_mode[0] = chanmode;
                    str_mode[1] = '\0';
                    strcat (new_modes, str_mode);
                    if (new_args[0])
                        strcat (new_args, " ");
                    strcat (new_args, argument);
                }
                else
                {
                    /* add mode without argument at the beginning of modes */
                    pos = new_modes;
                    while (pos[0] == '+')
                        pos++;
                    memmove (pos + 1, pos, strlen (pos) + 1);
                    pos[0] = chanmode;
                }
            }
        }
        if (new_args[0])
        {
            length = strlen (new_modes) + 1 + strlen (new_args) + 1;
            str_temp = malloc (length);
            if (str_temp)
            {
                snprintf (str_temp, length, "%s %s", new_modes, new_args);
                if (channel->modes)
                    free (channel->modes);
                channel->modes = str_temp;
            }
        }
        else
        {
            if (channel->modes)
                free (channel->modes);
            channel->modes = strdup (new_modes);
        }
    }

    if (new_modes)
        free (new_modes);
    if (new_args)
        free (new_args);
    if (str_modes)
        free (str_modes);
    if (argv)
        weechat_string_free_split (argv);
}

/*
 * Sets channel modes using CHANMODES (from message 005) and update channel
 * modes if needed.
 */

void
irc_mode_channel_set (struct t_irc_server *server,
                      struct t_irc_channel *channel,
                      const char *modes)
{
    char *pos_args, *str_modes, set_flag, **argv, *pos, *ptr_arg, chanmode_type;
    int argc, current_arg, update_channel_modes, channel_modes_updated;
    struct t_irc_nick *ptr_nick;

    if (!server || !channel || !modes)
        return;

    channel_modes_updated = 0;
    argc = 0;
    argv = NULL;
    pos_args = strchr (modes, ' ');
    if (pos_args)
    {
        str_modes = weechat_strndup (modes, pos_args - modes);
        if (!str_modes)
            return;
        pos_args++;
        while (pos_args[0] == ' ')
            pos_args++;
        argv = weechat_string_split (pos_args, " ", 0, 0, &argc);
    }
    else
    {
        str_modes = strdup (modes);
        if (!str_modes)
            return;
    }

    current_arg = 0;

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
                default:
                    update_channel_modes = 1;
                    chanmode_type = irc_mode_get_chanmode_type (server, pos[0]);
                    ptr_arg = NULL;
                    switch (chanmode_type)
                    {
                        case 'A': /* always argument */
                            update_channel_modes = 0;
                            ptr_arg = (current_arg < argc) ?
                                argv[current_arg] : NULL;
                            break;
                        case 'B': /* always argument */
                            ptr_arg = (current_arg < argc) ?
                                argv[current_arg] : NULL;
                            break;
                        case 'C': /* argumment if set */
                            ptr_arg = ((set_flag == '+') && (current_arg < argc)) ?
                                argv[current_arg] : NULL;
                            break;
                        case 'D': /* no argument */
                            break;
                    }
                    if (ptr_arg)
                        current_arg++;

                    if (pos[0] == 'k')
                    {
                        /* channel key */
                        if (set_flag == '-')
                        {
                            if (channel->key)
                            {
                                free (channel->key);
                                channel->key = NULL;
                            }
                        }
                        else if ((set_flag == '+')
                                 && ptr_arg && (strcmp (ptr_arg, "*") != 0))
                        {
                            /* replace key for +k, but ignore "*" as new key */
                            if (channel->key)
                                free (channel->key);
                            channel->key = strdup (ptr_arg);
                        }
                    }
                    else if (pos[0] == 'l')
                    {
                        /* channel limit */
                        if (set_flag == '-')
                            channel->limit = 0;
                        if ((set_flag == '+') && ptr_arg)
                        {
                            channel->limit = atoi (ptr_arg);
                        }
                    }
                    else if ((chanmode_type != 'A')
                             && (irc_server_get_prefix_mode_index (server,
                                                                   pos[0]) >= 0))
                    {
                        /* mode for nick */
                        update_channel_modes = 0;
                        if (ptr_arg)
                        {
                            ptr_nick = irc_nick_search (server, channel,
                                                        ptr_arg);
                            if (ptr_nick)
                            {
                                irc_nick_set_mode (server, channel, ptr_nick,
                                                   (set_flag == '+'), pos[0]);
                            }
                        }
                    }
                    if (update_channel_modes)
                    {
                        irc_mode_channel_update (server, channel, set_flag,
                                                 pos[0], ptr_arg);
                        channel_modes_updated = 1;
                    }
                    break;
            }
            pos++;
        }
    }

    if (str_modes)
        free (str_modes);
    if (argv)
        weechat_string_free_split (argv);

    if (channel_modes_updated)
        weechat_bar_item_update ("buffer_modes");
}

/*
 * Adds a user mode.
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
 * Removes a user mode.
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
 * Sets user modes.
 */

void
irc_mode_user_set (struct t_irc_server *server, const char *modes,
                   int reset_modes)
{
    char set_flag;
    int end;

    if (reset_modes)
    {
        if (server->nick_modes)
        {
            free (server->nick_modes);
            server->nick_modes = NULL;
        }
    }
    set_flag = '+';
    end = 0;
    while (modes && modes[0])
    {
        switch (modes[0])
        {
            case ' ':
                end = 1;
                break;
            case ':':
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
        if (end)
            break;
        modes++;
    }
    weechat_bar_item_update ("input_prompt");
}
