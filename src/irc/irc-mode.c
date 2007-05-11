/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

/* irc-mode.c: IRC channel/user modes management */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../common/weechat.h"
#include "irc.h"
#include "../common/util.h"
#include "../gui/gui.h"


/*
 * irc_mode_channel_set_nick: set a mode for a nick on a channel
 */

void
irc_mode_channel_set_nick (t_irc_channel *channel, char *nick,
                           char set_flag, int flag)
{
    t_irc_nick *ptr_nick;
    
    if (nick)
    {
        ptr_nick = nick_search (channel, nick);
        if (ptr_nick)
        {
            NICK_SET_FLAG(ptr_nick, (set_flag == '+'), flag);
            nick_resort (channel, ptr_nick);
            gui_nicklist_draw (channel->buffer, 1, 1);
        }
    }
}

/*
 * irc_mode_channel_get_flag: search for flag before current position
 */

char
irc_mode_channel_get_flag (char *str, char *pos)
{
    char set_flag;

    set_flag = '+';
    pos--;
    while (pos >= str)
    {
        if (pos[0] == '-')
            return '-';
        if (pos[0] == '+')
            return '+';
        pos--;
    }
    return set_flag;
}

/*
 * irc_mode_channel_set: set channel modes
 */

void
irc_mode_channel_set (t_irc_channel *channel, char *modes)
{
    char *pos_args, set_flag, **argv, *pos, *ptr_arg;
    int argc, current_arg;
    
    argc = 0;
    argv = NULL;
    current_arg = 0;
    pos_args = strchr (modes, ' ');
    if (pos_args)
    {
        pos_args[0] = '\0';
        pos_args++;
        while (pos_args[0] == ' ')
            pos_args++;
        argv = explode_string (pos_args, " ", 0, &argc);
        if (argc > 0)
            current_arg = argc - 1;
    }
    
    if (modes && modes[0])
    {
        set_flag = '+';
        pos = modes + strlen (modes) - 1;
        while (pos >= modes)
        {
            switch (pos[0])
            {
                case ':':
                case ' ':
                case '+':
                case '-':
                    break;
                default:
                    set_flag = irc_mode_channel_get_flag (modes, pos);
                    ptr_arg = ((argc > 0) && (current_arg >= 0)) ?
                        argv[current_arg--] : NULL;
                    switch (pos[0])
                    {
                        case 'a': /* unrealircd specific flag */
                            irc_mode_channel_set_nick (channel, ptr_arg,
                                                       set_flag, NICK_CHANADMIN);
                            break;
                        case 'h':
                            irc_mode_channel_set_nick (channel, ptr_arg,
                                                       set_flag, NICK_HALFOP);
                            break;
                        case 'k':
                            if (channel->key)
                            {
                                free (channel->key);
                                channel->key = NULL;
                            }
                            if ((set_flag == '+') && ptr_arg)
                                channel->key = strdup (ptr_arg);
                            break;
                        case 'l':
                            if (set_flag == '-')
                                channel->limit = 0;
                            if ((set_flag == '+') && ptr_arg)
                                channel->limit = atoi (ptr_arg);
                            break;
                        case 'o':
                            irc_mode_channel_set_nick (channel, ptr_arg,
                                                       set_flag, NICK_OP);
                            break;
                        case 'q': /* unrealircd specific flag */
                            irc_mode_channel_set_nick (channel, ptr_arg,
                                                       set_flag, NICK_CHANOWNER);
                            break;
                        case 'v':
                            irc_mode_channel_set_nick (channel, ptr_arg,
                                                       set_flag, NICK_VOICE);
                            break;
                    }
                    break;
            }
            pos--;
        }
    }
    
    if (argv)
        free_exploded_string (argv);
}

/*
 * irc_mode_user_add: add a user mode
 */

void
irc_mode_user_add (t_irc_server *server, char mode)
{
    char str_mode[2];

    str_mode[0] = mode;
    str_mode[1] = '\0';
    
    if (server->nick_modes)
    {
        if (!strchr (server->nick_modes, mode))
        {
            server->nick_modes = (char *) realloc (server->nick_modes,
                                                   strlen (server->nick_modes) + 1 + 1);
            strcat (server->nick_modes, str_mode);
            gui_status_draw (gui_current_window->buffer, 1);
            gui_input_draw (gui_current_window->buffer, 1);
        }
    }
    else
    {
        server->nick_modes = (char *) malloc (2);
        strcpy (server->nick_modes, str_mode);
        gui_status_draw (gui_current_window->buffer, 1);
        gui_input_draw (gui_current_window->buffer, 1);
    }
}

/*
 * irc_mode_user_remove: remove a user mode
 */

void
irc_mode_user_remove (t_irc_server *server, char mode)
{
    char *pos;
    int new_size;
    
    if (server->nick_modes)
    {
        pos = strchr (server->nick_modes, mode);
        if (pos)
        {
            new_size = strlen (server->nick_modes);
            memmove (pos, pos + 1, strlen (pos + 1) + 1);
            server->nick_modes = (char *) realloc (server->nick_modes,
                                                   new_size);
            gui_status_draw (gui_current_window->buffer, 1);
            gui_input_draw (gui_current_window->buffer, 1);
        }
    }
}

/*
 * irc_mode_user_set: set user modes
 */

void
irc_mode_user_set (t_irc_server *server, char *modes)
{
    char set_flag;
    
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
}

/*
 * irc_mode_nick_prefix_allowed: return <> 0 if nick prefix is allowed by server
 *                               for example :
 *                                 IRC:  005 (...) PREFIX=(ov)@+
 *                               => allowed prefixes: @+
 */

int
irc_mode_nick_prefix_allowed (t_irc_server *server, char prefix)
{
    /* if server did not send any prefix info, then consider this prefix is allowed */
    if (!server->prefix)
        return 1;

    return (strchr (server->prefix, prefix) != NULL);
}
