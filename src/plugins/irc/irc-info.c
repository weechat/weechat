/*
 * Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
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

/* irc-info.c: info and infolist hooks for IRC plugin */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-channel.h"
#include "irc-config.h"
#include "irc-ignore.h"
#include "irc-nick.h"
#include "irc-protocol.h"
#include "irc-server.h"


/*
 * irc_info_create_string_with_pointer: create a string with a pointer inside
 *                                      an IRC structure
 */

void
irc_info_create_string_with_pointer (char **string, void *pointer)
{
    if (*string)
    {
        free (*string);
        *string = NULL;
    }
    if (pointer)
    {
        *string = malloc (64);
        if (*string)
        {
            snprintf (*string, 64 - 1, "0x%lx", (long unsigned int)pointer);
        }
    }
}

/*
 * irc_info_get_info_cb: callback called when IRC info is asked
 */

const char *
irc_info_get_info_cb (void *data, const char *info_name,
                      const char *arguments)
{
    char *pos_comma, *pos_comma2, *server, *channel, *host;
    const char *nick;
    static char str_true[2] = "1";
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    
    /* make C compiler happy */
    (void) data;
    
    if (weechat_strcasecmp (info_name, "irc_is_channel") == 0)
    {
        if (irc_channel_is_channel (arguments))
            return str_true;
        return NULL;
    }
    else if (weechat_strcasecmp (info_name, "irc_nick") == 0)
    {
        ptr_server = irc_server_search (arguments);
        if (ptr_server)
            return ptr_server->nick;
        return NULL;
    }
    else if (weechat_strcasecmp (info_name, "irc_nick_from_host") == 0)
    {
        return irc_protocol_get_nick_from_host (arguments);
    }
    else if (weechat_strcasecmp (info_name, "irc_nick_color") == 0)
    {
        return irc_nick_find_color (arguments);
    }
    else if (weechat_strcasecmp (info_name, "irc_buffer") == 0)
    {
        if (arguments && arguments[0])
        {
            server = NULL;
            channel = NULL;
            host = NULL;
            ptr_server = NULL;
            ptr_channel = NULL;
            
            pos_comma = strchr (arguments, ',');
            if (pos_comma)
            {
                server = weechat_strndup (arguments, pos_comma - arguments);
                pos_comma2 = strchr (pos_comma + 1, ',');
                if (pos_comma2)
                {
                    channel = weechat_strndup (pos_comma + 1,
                                               pos_comma2 - pos_comma - 1);
                    host = strdup (pos_comma2 + 1);
                }
                else
                    channel = strdup (pos_comma + 1);
            }
            else
            {
                if (irc_channel_is_channel (arguments))
                    channel = strdup (arguments);
                else
                    server = strdup (arguments);
            }
            
            /* replace channel by nick in host if channel is not a channel
               (private ?) */
            if (channel && host)
            {
                if (!irc_channel_is_channel (channel))
                {
                    free (channel);
                    channel = NULL;
                    nick = irc_protocol_get_nick_from_host (host);
                    if (nick)
                        channel = strdup (nick);
                    
                }
            }
            
            /* search for server or channel buffer */
            if (server)
            {
                ptr_server = irc_server_search (server);
                if (ptr_server && channel)
                    ptr_channel = irc_channel_search (ptr_server, channel);
            }
            
            if (server)
                free (server);
            if (channel)
                free (channel);
            if (host)
                free (host);
            
            if (ptr_channel)
            {
                irc_info_create_string_with_pointer (&ptr_channel->buffer_as_string,
                                                     ptr_channel->buffer);
                return ptr_channel->buffer_as_string;
            }
            if (ptr_server)
            {
                irc_info_create_string_with_pointer (&ptr_server->buffer_as_string,
                                                     ptr_server->buffer);
                return ptr_server->buffer_as_string;
            }
        }
    }
    
    return NULL;
}

/*
 * irc_info_get_infolist_cb: callback called when IRC infolist is asked
 */

struct t_infolist *
irc_info_get_infolist_cb (void *data, const char *infolist_name,
                          void *pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    struct t_irc_ignore *ptr_ignore;
    char **argv;
    int argc;
    
    /* make C compiler happy */
    (void) data;
    
    if (!infolist_name || !infolist_name[0])
        return NULL;
    
    if (weechat_strcasecmp (infolist_name, "irc_server") == 0)
    {
        if (pointer && !irc_server_valid (pointer))
            return NULL;
        
        ptr_infolist = weechat_infolist_new ();
        if (ptr_infolist)
        {
            if (pointer)
            {
                /* build list with only one server */
                if (!irc_server_add_to_infolist (ptr_infolist, pointer))
                {
                    weechat_infolist_free (ptr_infolist);
                    return NULL;
                }
                return ptr_infolist;
            }
            else
            {
                /* build list with all servers matching arguments */
                for (ptr_server = irc_servers; ptr_server;
                     ptr_server = ptr_server->next_server)
                {
                    if (!arguments || !arguments[0]
                        || weechat_string_match (ptr_server->name, arguments, 0))
                    {
                        if (!irc_server_add_to_infolist (ptr_infolist, ptr_server))
                        {
                            weechat_infolist_free (ptr_infolist);
                            return NULL;
                        }
                    }
                }
                return ptr_infolist;
            }
        }
    }
    else if (weechat_strcasecmp (infolist_name, "irc_channel") == 0)
    {
        if (arguments && arguments[0])
        {
            ptr_server = irc_server_search (arguments);
            if (ptr_server)
            {
                if (pointer && !irc_channel_valid (ptr_server, pointer))
                    return NULL;
                
                ptr_infolist = weechat_infolist_new ();
                if (ptr_infolist)
                {
                    if (pointer)
                    {
                        /* build list with only one channel */
                        if (!irc_channel_add_to_infolist (ptr_infolist, pointer))
                        {
                            weechat_infolist_free (ptr_infolist);
                            return NULL;
                        }
                        return ptr_infolist;
                    }
                    else
                    {
                        /* build list with all channels of server */
                        for (ptr_channel = ptr_server->channels; ptr_channel;
                             ptr_channel = ptr_channel->next_channel)
                        {
                            if (!irc_channel_add_to_infolist (ptr_infolist,
                                                              ptr_channel))
                            {
                                weechat_infolist_free (ptr_infolist);
                                return NULL;
                            }
                        }
                        return ptr_infolist;
                    }
                }
            }
        }
    }
    else if (weechat_strcasecmp (infolist_name, "irc_nick") == 0)
    {
        if (arguments && arguments[0])
        {
            ptr_server = NULL;
            ptr_channel = NULL;
            argv = weechat_string_split (arguments, ",", 0, 0, &argc);
            if (argv)
            {
                if (argc >= 2)
                {
                    ptr_server = irc_server_search (argv[0]);
                    if (!ptr_server)
                    {
                        weechat_string_free_split (argv);
                        return NULL;
                    }
                    ptr_channel = irc_channel_search (ptr_server, argv[1]);
                    if (!ptr_channel)
                    {
                        weechat_string_free_split (argv);
                        return NULL;
                    }
                    if (!pointer && (argc >= 3))
                    {
                        pointer = irc_nick_search (ptr_channel, argv[2]);
                        if (!pointer)
                        {
                            weechat_string_free_split (argv);
                            return NULL;
                        }
                    }
                }
                weechat_string_free_split (argv);
                if (ptr_server && ptr_channel)
                {
                    if (pointer && !irc_nick_valid (ptr_channel, pointer))
                        return NULL;
                    
                    ptr_infolist = weechat_infolist_new ();
                    if (ptr_infolist)
                    {
                        if (pointer)
                        {
                            /* build list with only one nick */
                            if (!irc_nick_add_to_infolist (ptr_infolist, pointer))
                            {
                                weechat_infolist_free (ptr_infolist);
                                return NULL;
                            }
                            return ptr_infolist;
                        }
                        else
                        {
                            /* build list with all nicks of channel */
                            for (ptr_nick = ptr_channel->nicks; ptr_nick;
                                 ptr_nick = ptr_nick->next_nick)
                            {
                                if (!irc_nick_add_to_infolist (ptr_infolist,
                                                               ptr_nick))
                                {
                                    weechat_infolist_free (ptr_infolist);
                                    return NULL;
                                }
                            }
                            return ptr_infolist;
                        }
                    }
                }
            }
        }
    }
    else if (weechat_strcasecmp (infolist_name, "irc_ignore") == 0)
    {
        if (pointer && !irc_ignore_valid (pointer))
            return NULL;
        
        ptr_infolist = weechat_infolist_new ();
        if (ptr_infolist)
        {
            if (pointer)
            {
                /* build list with only one ignore */
                if (!irc_ignore_add_to_infolist (ptr_infolist, pointer))
                {
                    weechat_infolist_free (ptr_infolist);
                    return NULL;
                }
                return ptr_infolist;
            }
            else
            {
                /* build list with all ignore */
                for (ptr_ignore = irc_ignore_list; ptr_ignore;
                     ptr_ignore = ptr_ignore->next_ignore)
                {
                    if (!irc_ignore_add_to_infolist (ptr_infolist, ptr_ignore))
                    {
                        weechat_infolist_free (ptr_infolist);
                        return NULL;
                    }
                }
                return ptr_infolist;
            }
        }
    }
    
    return NULL;
}

/*
 * irc_info_init: initialize info and infolist hooks for IRC plugin
 */

void
irc_info_init ()
{
    /* info hooks */
    weechat_hook_info ("irc_is_channel", N_("1 if string is an IRC channel"),
                       &irc_info_get_info_cb, NULL);
    weechat_hook_info ("irc_nick", N_("get current nick on a server"),
                       &irc_info_get_info_cb, NULL);
    weechat_hook_info ("irc_nick_from_host", N_("get nick from IRC host"),
                       &irc_info_get_info_cb, NULL);
    weechat_hook_info ("irc_nick_color", N_("get nick color"),
                       &irc_info_get_info_cb, NULL);
    weechat_hook_info ("irc_buffer", N_("get buffer pointer for an IRC server/channel"),
                       &irc_info_get_info_cb, NULL);
    
    /* infolist hooks */
    weechat_hook_infolist ("irc_server", N_("list of IRC servers"),
                           &irc_info_get_infolist_cb, NULL);
    weechat_hook_infolist ("irc_channel", N_("list of channels for an IRC server"),
                           &irc_info_get_infolist_cb, NULL);
    weechat_hook_infolist ("irc_nick", N_("list of nicks for an IRC channel"),
                           &irc_info_get_infolist_cb, NULL);
    weechat_hook_infolist ("irc_ignore", N_("list of IRC ignores"),
                           &irc_info_get_infolist_cb, NULL);
}
