/*
 * irc-info.c - info, infolist and hdata hooks for IRC plugin
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-batch.h"
#include "irc-channel.h"
#include "irc-color.h"
#include "irc-config.h"
#include "irc-ignore.h"
#include "irc-message.h"
#include "irc-modelist.h"
#include "irc-nick.h"
#include "irc-notify.h"
#include "irc-protocol.h"
#include "irc-redirect.h"
#include "irc-server.h"


/*
 * Creates a string with a pointer inside an IRC structure.
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
            snprintf (*string, 64, "0x%lx", (unsigned long)pointer);
        }
    }
}

/*
 * Returns IRC info "irc_is_channel".
 */

char *
irc_info_info_irc_is_channel_cb (const void *pointer, void *data,
                                 const char *info_name,
                                 const char *arguments)
{
    char *pos_comma, *server;
    const char *pos_channel;
    struct t_irc_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    if (!arguments || !arguments[0])
        return NULL;

    ptr_server = NULL;
    pos_channel = arguments;
    pos_comma = strchr (arguments, ',');
    if (pos_comma)
    {
        pos_channel = pos_comma + 1;
        server = weechat_strndup (arguments, pos_comma - arguments);
        if (server)
        {
            ptr_server = irc_server_search (server);
            free (server);
        }
    }

    return (irc_channel_is_channel (ptr_server, pos_channel)) ?
        strdup ("1") : NULL;
}

/*
 * Returns IRC info "irc_is_nick".
 */

char *
irc_info_info_irc_is_nick_cb (const void *pointer, void *data,
                              const char *info_name,
                              const char *arguments)
{
    char *pos_comma, *server;
    const char *pos_nick;
    struct t_irc_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    if (!arguments || !arguments[0])
        return NULL;

    ptr_server = NULL;
    pos_nick = arguments;
    pos_comma = strchr (arguments, ',');
    if (pos_comma)
    {
        pos_nick = pos_comma + 1;
        server = weechat_strndup (arguments, pos_comma - arguments);
        if (server)
        {
            ptr_server = irc_server_search (server);
            free (server);
        }
    }

    return (irc_nick_is_nick (ptr_server, pos_nick)) ? strdup ("1") : NULL;
}

/*
 * Returns IRC info "irc_nick".
 */

char *
irc_info_info_irc_nick_cb (const void *pointer, void *data,
                           const char *info_name,
                           const char *arguments)
{
    struct t_irc_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    if (!arguments || !arguments[0])
        return NULL;

    ptr_server = irc_server_search (arguments);

    return (ptr_server && ptr_server->nick) ?
        strdup (ptr_server->nick) : NULL;
}

/*
 * Returns IRC info "irc_nick_from_host".
 */

char *
irc_info_info_irc_nick_from_host_cb (const void *pointer, void *data,
                                     const char *info_name,
                                     const char *arguments)
{
    const char *ptr_host;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    if (!arguments || !arguments[0])
        return NULL;

    ptr_host = irc_message_get_nick_from_host (arguments);

    return (ptr_host) ? strdup (ptr_host) : NULL;
}

/*
 * Returns IRC info "irc_nick_color".
 */

char *
irc_info_info_irc_nick_color_cb (const void *pointer, void *data,
                                 const char *info_name,
                                 const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    if (!arguments || !arguments[0])
        return NULL;

    return irc_nick_find_color (arguments);
}

/*
 * Returns IRC info "irc_nick_color_name".
 */

char *
irc_info_info_irc_nick_color_name_cb (const void *pointer, void *data,
                                      const char *info_name,
                                      const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    if (!arguments || !arguments[0])
        return NULL;

    return irc_nick_find_color_name (arguments);
}

/*
 * Returns IRC info "irc_buffer".
 */

char *
irc_info_info_irc_buffer_cb (const void *pointer, void *data,
                             const char *info_name,
                             const char *arguments)
{
    char *pos_comma, *pos_comma2, *server, *channel, *host;
    const char *nick;
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    if (!arguments || !arguments[0])
        return NULL;

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
        if (irc_server_search (arguments))
            server = strdup (arguments);
        else
            channel = strdup (arguments);
    }
    if (server)
        ptr_server = irc_server_search (server);

    /*
     * replace channel by nick in host if channel is not a channel
     * (private ?)
     */
    if (channel && host)
    {
        if (!irc_channel_is_channel (ptr_server, channel))
        {
            free (channel);
            channel = NULL;
            nick = irc_message_get_nick_from_host (host);
            if (nick)
                channel = strdup (nick);
        }
    }

    /* search for server or channel buffer */
    if (server && ptr_server && channel)
        ptr_channel = irc_channel_search (ptr_server, channel);

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
        return (ptr_channel->buffer_as_string) ?
            strdup (ptr_channel->buffer_as_string) : NULL;
    }

    if (ptr_server)
    {
        irc_info_create_string_with_pointer (&ptr_server->buffer_as_string,
                                             ptr_server->buffer);
        return (ptr_server->buffer_as_string) ?
            strdup (ptr_server->buffer_as_string) : NULL;
    }

    return NULL;
}

/*
 * Returns IRC info "irc_server_isupport".
 */

char *
irc_info_info_irc_server_isupport_cb (const void *pointer, void *data,
                                      const char *info_name,
                                      const char *arguments)
{
    char *pos_comma, *server;
    const char *isupport_value;
    struct t_irc_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    isupport_value = NULL;
    pos_comma = strchr (arguments, ',');
    if (pos_comma)
    {
        server = weechat_strndup (arguments, pos_comma - arguments);
        if (server)
        {
            ptr_server = irc_server_search (server);
            if (ptr_server)
            {
                isupport_value = irc_server_get_isupport_value (ptr_server,
                                                                pos_comma + 1);
            }
            free (server);
        }
    }

    return (isupport_value) ? strdup ("1") : NULL;
}

/*
 * Returns IRC info "irc_server_isupport_value".
 */

char *
irc_info_info_irc_server_isupport_value_cb (const void *pointer, void *data,
                                            const char *info_name,
                                            const char *arguments)
{
    char *pos_comma, *server;
    const char *isupport_value;
    struct t_irc_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    isupport_value = NULL;
    pos_comma = strchr (arguments, ',');
    if (pos_comma)
    {
        server = weechat_strndup (arguments, pos_comma - arguments);
        if (server)
        {
            ptr_server = irc_server_search (server);
            if (ptr_server)
            {
                isupport_value = irc_server_get_isupport_value (ptr_server,
                                                                pos_comma + 1);
            }
            free (server);
        }
    }

    return (isupport_value) ? strdup (isupport_value) : NULL;
}

/*
 * Returns IRC info "irc_server_cap".
 */

char *
irc_info_info_irc_server_cap_cb (const void *pointer, void *data,
                                 const char *info_name,
                                 const char *arguments)
{
    char *pos_comma, *server;
    int has_cap;
    struct t_irc_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    has_cap = 0;
    pos_comma = strchr (arguments, ',');
    if (pos_comma)
    {
        server = weechat_strndup (arguments, pos_comma - arguments);
        if (server)
        {
            ptr_server = irc_server_search (server);
            if (ptr_server)
            {
                has_cap = weechat_hashtable_has_key (ptr_server->cap_list,
                                                     pos_comma + 1);
            }
            free (server);
        }
    }

    return (has_cap) ? strdup ("1") : NULL;
}

/*
 * Returns IRC info "irc_server_cap_value".
 */

char *
irc_info_info_irc_server_cap_value_cb (const void *pointer, void *data,
                                       const char *info_name,
                                       const char *arguments)
{
    char *pos_comma, *server;
    const char *cap_value;
    struct t_irc_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    cap_value = NULL;
    pos_comma = strchr (arguments, ',');
    if (pos_comma)
    {
        server = weechat_strndup (arguments, pos_comma - arguments);
        if (server)
        {
            ptr_server = irc_server_search (server);
            if (ptr_server)
            {
                cap_value = weechat_hashtable_get (ptr_server->cap_list,
                                                   pos_comma + 1);
            }
            free (server);
        }
    }

    return (cap_value) ? strdup (cap_value) : NULL;
}

/*
 * Returns IRC info "irc_is_message_ignored".
 */

char *
irc_info_info_irc_is_message_ignored_cb (const void *pointer, void *data,
                                         const char *info_name,
                                         const char *arguments)
{
    char *pos_comma, *server;
    const char *pos_message;
    struct t_irc_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    if (!arguments || !arguments[0])
        return NULL;

    pos_comma = strchr (arguments, ',');
    if (!pos_comma)
        return NULL;

    pos_message = pos_comma + 1;

    ptr_server = NULL;
    server = weechat_strndup (arguments, pos_comma - arguments);
    if (server)
    {
        ptr_server = irc_server_search (server);
        free (server);
    }
    if (!ptr_server)
        return NULL;

    return (irc_message_ignored (ptr_server, pos_message)) ?
        strdup ("1") : NULL;
}

/*
 * Returns IRC info with hashtable "irc_message_parse".
 */

struct t_hashtable *
irc_info_info_hashtable_irc_message_parse_cb (const void *pointer, void *data,
                                              const char *info_name,
                                              struct t_hashtable *hashtable)
{
    const char *server, *message;
    struct t_irc_server *ptr_server;
    struct t_hashtable *value;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    if (!hashtable)
        return NULL;

    server = weechat_hashtable_get (hashtable, "server");
    ptr_server = (server) ? irc_server_search (server) : NULL;
    message = weechat_hashtable_get (hashtable, "message");
    if (message)
    {
        value = irc_message_parse_to_hashtable (ptr_server, message);
        return value;
    }

    return NULL;
}

/*
 * Returns IRC info with hashtable "irc_message_split".
 */

struct t_hashtable *
irc_info_info_hashtable_irc_message_split_cb (const void *pointer, void *data,
                                              const char *info_name,
                                              struct t_hashtable *hashtable)
{
    const char *server, *message;
    struct t_irc_server *ptr_server;
    struct t_hashtable *value;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    if (!hashtable)
        return NULL;

    server = weechat_hashtable_get (hashtable, "server");
    ptr_server = (server) ? irc_server_search (server) : NULL;
    message = weechat_hashtable_get (hashtable, "message");
    if (message)
    {
        value = irc_message_split (ptr_server, message);
        return value;
    }

    return NULL;
}

/*
 * Returns IRC infolist "irc_server".
 */

struct t_infolist *
irc_info_infolist_irc_server_cb (const void *pointer, void *data,
                                 const char *infolist_name,
                                 void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_irc_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;

    if (obj_pointer && !irc_server_valid (obj_pointer))
        return NULL;

    ptr_infolist = weechat_infolist_new ();
    if (!ptr_infolist)
        return NULL;

    if (obj_pointer)
    {
        /* build list with only one server */
        if (!irc_server_add_to_infolist (ptr_infolist, obj_pointer, 0))
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
                || weechat_string_match (ptr_server->name, arguments, 1))
            {
                if (!irc_server_add_to_infolist (ptr_infolist, ptr_server, 0))
                {
                    weechat_infolist_free (ptr_infolist);
                    return NULL;
                }
            }
        }
        return ptr_infolist;
    }

    return NULL;
}

/*
 * Returns IRC infolist "irc_channel".
 */

struct t_infolist *
irc_info_infolist_irc_channel_cb (const void *pointer, void *data,
                                  const char *infolist_name,
                                  void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    char **argv;
    int argc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;

    if (!arguments || !arguments[0])
        return NULL;

    ptr_server = NULL;
    ptr_channel = NULL;
    argv = weechat_string_split (arguments, ",", NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                 0, &argc);
    if (!argv)
        return NULL;

    if (argc >= 1)
    {
        ptr_server = irc_server_search (argv[0]);
        if (!ptr_server)
        {
            weechat_string_free_split (argv);
            return NULL;
        }
        if (!obj_pointer && (argc >= 2))
        {
            obj_pointer = irc_channel_search (ptr_server, argv[1]);
            if (!obj_pointer)
            {
                weechat_string_free_split (argv);
                return NULL;
            }
        }
    }
    weechat_string_free_split (argv);

    if (!ptr_server)
        return NULL;

    if (obj_pointer && !irc_channel_valid (ptr_server, obj_pointer))
        return NULL;

    ptr_infolist = weechat_infolist_new ();
    if (!ptr_infolist)
        return NULL;

    if (obj_pointer)
    {
        /* build list with only one channel */
        if (!irc_channel_add_to_infolist (ptr_infolist, obj_pointer))
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
            if (!irc_channel_add_to_infolist (ptr_infolist, ptr_channel))
            {
                weechat_infolist_free (ptr_infolist);
                return NULL;
            }
        }
        return ptr_infolist;
    }

    return NULL;
}

/*
 * Returns IRC infolist "irc_modelist".
 */

struct t_infolist *
irc_info_infolist_irc_modelist_cb (const void *pointer, void *data,
                                   const char *infolist_name,
                                   void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    struct t_irc_modelist *ptr_modelist;
    char **argv;
    int argc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;

    if (!arguments || !arguments[0])
        return NULL;

    ptr_server = NULL;
    ptr_channel = NULL;
    argv = weechat_string_split (arguments, ",", NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                 0, &argc);
    if (!argv)
        return NULL;

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
        if (!obj_pointer && (argc >= 3))
        {
            obj_pointer = irc_modelist_search (ptr_channel, argv[2][0]);

            if (!obj_pointer)
            {
                weechat_string_free_split (argv);
                return NULL;
            }
        }
    }
    weechat_string_free_split (argv);

    if (!ptr_server || !ptr_channel)
        return NULL;

    if (obj_pointer && !irc_modelist_valid (ptr_channel, obj_pointer))
        return NULL;

    ptr_infolist = weechat_infolist_new ();
    if (!ptr_infolist)
        return NULL;

    if (obj_pointer)
    {
        /* build list with only one modelist */
        if (!irc_modelist_add_to_infolist (ptr_infolist, obj_pointer))
        {
            weechat_infolist_free (ptr_infolist);
            return NULL;
        }
        return ptr_infolist;
    }
    else
    {
        /* build list with all modelists of channel */
        for (ptr_modelist = ptr_channel->modelists; ptr_modelist;
             ptr_modelist = ptr_modelist->next_modelist)
        {
            if (!irc_modelist_add_to_infolist (ptr_infolist, ptr_modelist))
            {
                weechat_infolist_free (ptr_infolist);
                return NULL;
            }
        }
        return ptr_infolist;
    }

    return NULL;
}

/*
 * Returns IRC infolist "irc_modelist_item".
 */

struct t_infolist *
irc_info_infolist_irc_modelist_item_cb (const void *pointer, void *data,
                                        const char *infolist_name,
                                        void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    struct t_irc_modelist *ptr_modelist;
    struct t_irc_modelist_item *ptr_item;
    char **argv, *error;
    int argc;
    long number;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;

    if (!arguments || !arguments[0])
        return NULL;

    ptr_server = NULL;
    ptr_channel = NULL;
    argv = weechat_string_split (arguments, ",", NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                 0, &argc);
    if (!argv)
        return NULL;

    if (argc >= 3)
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
        ptr_modelist = irc_modelist_search (ptr_channel, argv[2][0]);
        if (!ptr_modelist)
        {
            weechat_string_free_split (argv);
            return NULL;
        }
        if (!obj_pointer && (argc >= 4))
        {
            error = NULL;
            number = strtol (argv[3], &error, 10);
            if (!error || error[0] || (number < 0))
            {
                weechat_string_free_split (argv);
                return NULL;
            }
            obj_pointer = irc_modelist_item_search_number (ptr_modelist,
                                                           (int)number);
            if (!obj_pointer)
            {
                weechat_string_free_split (argv);
                return NULL;
            }
        }
    }
    weechat_string_free_split (argv);

    if (!ptr_server || !ptr_channel || !ptr_modelist)
        return NULL;

    if (obj_pointer && !irc_modelist_item_valid (ptr_modelist, obj_pointer))
        return NULL;

    ptr_infolist = weechat_infolist_new ();
    if (!ptr_infolist)
        return NULL;

    if (obj_pointer)
    {
        /* build list with only one modelist item */
        if (!irc_modelist_item_add_to_infolist (ptr_infolist, obj_pointer))
        {
            weechat_infolist_free (ptr_infolist);
            return NULL;
        }
        return ptr_infolist;
    }
    else
    {
        /* build list with all modelist items of modelist */
        for (ptr_item = ptr_modelist->items; ptr_item;
             ptr_item = ptr_item->next_item)
        {
            if (!irc_modelist_item_add_to_infolist (ptr_infolist, ptr_item))
            {
                weechat_infolist_free (ptr_infolist);
                return NULL;
            }
        }
        return ptr_infolist;
    }

    return NULL;
}

/*
 * Returns IRC infolist "irc_nick".
 */

struct t_infolist *
irc_info_infolist_irc_nick_cb (const void *pointer, void *data,
                               const char *infolist_name,
                               void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    char **argv;
    int argc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;

    if (!arguments || !arguments[0])
        return NULL;

    ptr_server = NULL;
    ptr_channel = NULL;
    argv = weechat_string_split (arguments, ",", NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                 0, &argc);
    if (!argv)
        return NULL;

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
        if (!obj_pointer && (argc >= 3))
        {
            obj_pointer = irc_nick_search (ptr_server, ptr_channel,
                                       argv[2]);
            if (!obj_pointer)
            {
                weechat_string_free_split (argv);
                return NULL;
            }
        }
    }
    weechat_string_free_split (argv);

    if (!ptr_server || !ptr_channel)
        return NULL;

    if (obj_pointer && !irc_nick_valid (ptr_channel, obj_pointer))
        return NULL;

    ptr_infolist = weechat_infolist_new ();
    if (!ptr_infolist)
        return NULL;

    if (obj_pointer)
    {
        /* build list with only one nick */
        if (!irc_nick_add_to_infolist (ptr_infolist,
                                       obj_pointer))
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

    return NULL;
}

/*
 * Returns IRC infolist "irc_ignore".
 */

struct t_infolist *
irc_info_infolist_irc_ignore_cb (const void *pointer, void *data,
                                 const char *infolist_name,
                                 void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_irc_ignore *ptr_ignore;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;
    (void) arguments;

    if (obj_pointer && !irc_ignore_valid (obj_pointer))
        return NULL;

    ptr_infolist = weechat_infolist_new ();
    if (!ptr_infolist)
        return NULL;

    if (obj_pointer)
    {
        /* build list with only one ignore */
        if (!irc_ignore_add_to_infolist (ptr_infolist, obj_pointer))
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

    return NULL;
}

/*
 * Returns IRC infolist "irc_notify".
 */

struct t_infolist *
irc_info_infolist_irc_notify_cb (const void *pointer, void *data,
                                 const char *infolist_name,
                                 void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_irc_server *ptr_server;
    struct t_irc_notify *ptr_notify;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;

    if (obj_pointer && !irc_notify_valid (NULL, obj_pointer))
        return NULL;

    ptr_infolist = weechat_infolist_new ();
    if (!ptr_infolist)
        return NULL;

    if (obj_pointer)
    {
        /* build list with only one notify */
        if (!irc_notify_add_to_infolist (ptr_infolist, obj_pointer))
        {
            weechat_infolist_free (ptr_infolist);
            return NULL;
        }
        return ptr_infolist;
    }
    else
    {
        /* build list with notify list of all servers matching arguments */
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (!arguments || !arguments[0]
                || weechat_string_match (ptr_server->name, arguments, 1))
            {
                for (ptr_notify = ptr_server->notify_list; ptr_notify;
                     ptr_notify = ptr_notify->next_notify)
                {
                    if (!irc_notify_add_to_infolist (ptr_infolist,
                                                     ptr_notify))
                    {
                        weechat_infolist_free (ptr_infolist);
                        return NULL;
                    }
                }
            }
        }
        return ptr_infolist;
    }

    return NULL;
}

/*
 * Returns IRC infolist "irc_color_weechat".
 */

struct t_infolist *
irc_info_infolist_irc_color_weechat_cb (const void *pointer, void *data,
                                        const char *infolist_name,
                                        void *obj_pointer,
                                        const char *arguments)
{
    struct t_infolist *ptr_infolist;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;
    (void) obj_pointer;
    (void) arguments;

    ptr_infolist = weechat_infolist_new ();
    if (!ptr_infolist)
        return NULL;

    /* build list with all IRC colors */
    if (!irc_color_weechat_add_to_infolist (ptr_infolist))
    {
        weechat_infolist_free (ptr_infolist);
        return NULL;
    }
    return ptr_infolist;
}

/*
 * Hooks info, infolist and hdata for IRC plugin.
 */

void
irc_info_init ()
{
    /* info hooks */
    weechat_hook_info (
        "irc_is_channel",
        N_("1 if string is a valid IRC channel name for server"),
        N_("server,channel (server is optional)"),
        &irc_info_info_irc_is_channel_cb, NULL, NULL);
    weechat_hook_info (
        "irc_is_nick",
        N_("1 if string is a valid IRC nick name"),
        N_("server,nickname (server is optional)"),
        &irc_info_info_irc_is_nick_cb, NULL, NULL);
    weechat_hook_info (
        "irc_nick",
        N_("get current nick on a server"),
        N_("server name"),
        &irc_info_info_irc_nick_cb, NULL, NULL);
    weechat_hook_info (
        "irc_nick_from_host",
        N_("get nick from IRC host"),
        N_("IRC host (like `:nick!name@server.com`)"),
        &irc_info_info_irc_nick_from_host_cb, NULL, NULL);
    weechat_hook_info (
        "irc_nick_color",
        N_("get nick color code "
           "(*deprecated* since version 1.5, replaced by \"nick_color\")"),
        N_("nickname"),
        &irc_info_info_irc_nick_color_cb, NULL, NULL);
    weechat_hook_info (
        "irc_nick_color_name",
        N_("get nick color name "
           "(*deprecated* since version 1.5, replaced by \"nick_color_name\")"),
        N_("nickname"),
        &irc_info_info_irc_nick_color_name_cb, NULL, NULL);
    weechat_hook_info (
        "irc_buffer",
        N_("get buffer pointer for an IRC server/channel/nick"),
        N_("server,channel,nick (channel and nicks are optional)"),
        &irc_info_info_irc_buffer_cb, NULL, NULL);
    weechat_hook_info (
        "irc_server_isupport",
        N_("1 if server supports this feature (from IRC message 005)"),
        N_("server,feature"),
        &irc_info_info_irc_server_isupport_cb, NULL, NULL);
    weechat_hook_info (
        "irc_server_isupport_value",
        N_("value of feature, if supported by server (from IRC message 005)"),
        N_("server,feature"),
        &irc_info_info_irc_server_isupport_value_cb, NULL, NULL);
    weechat_hook_info (
        "irc_server_cap",
        N_("1 if capability is enabled in server"),
        N_("server,capability"),
        &irc_info_info_irc_server_cap_cb, NULL, NULL);
    weechat_hook_info (
        "irc_server_cap_value",
        N_("value of capability, if enabled in server"),
        N_("server,capability"),
        &irc_info_info_irc_server_cap_value_cb, NULL, NULL);
    weechat_hook_info (
        "irc_is_message_ignored",
        N_("1 if the nick is ignored (message is not displayed)"),
        N_("server,message (message is the raw IRC message)"),
        &irc_info_info_irc_is_channel_cb, NULL, NULL);

    /* info_hashtable hooks */
    weechat_hook_info_hashtable (
        "irc_message_parse",
        N_("parse an IRC message"),
        N_("\"message\": IRC message, \"server\": server name (optional)"),
        /* TRANSLATORS: please do not translate key names (enclosed by quotes) */
        N_("\"tags\": tags, "
           "\"tag_xxx\": unescaped value of tag \"xxx\" (one key per tag), "
           "\"message_without_tags\": message without the tags, "
           "\"nick\": nick, "
           "\"user\": user, "
           "\"host\": host, "
           "\"command\": command, "
           "\"channel\": channel, "
           "\"arguments\": arguments (includes channel), "
           "\"text\": text (for example user message), "
           "\"param1\" ... \"paramN\": parsed command parameters, "
           "\"num_params\": number of parsed command parameters, "
           "\"pos_command\": index of \"command\" message (\"-1\" if "
           "\"command\" was not found), "
           "\"pos_arguments\": index of \"arguments\" message (\"-1\" if "
           "\"arguments\" was not found), "
           "\"pos_channel\": index of \"channel\" message (\"-1\" if "
           "\"channel\" was not found), "
           "\"pos_text\": index of \"text\" message (\"-1\" if "
           "\"text\" was not found)"),
        &irc_info_info_hashtable_irc_message_parse_cb, NULL, NULL);
    weechat_hook_info_hashtable (
        "irc_message_split",
        N_("split an IRC message (to fit in 512 bytes by default)"),
        N_("\"message\": IRC message, \"server\": server name (optional)"),
        /* TRANSLATORS: please do not translate key names (enclosed by quotes) */
        N_("\"msg1\" ... \"msgN\": messages to send (without final \"\\r\\n\"), "
           "\"args1\" ... \"argsN\": arguments of messages, \"count\": number "
           "of messages"),
        &irc_info_info_hashtable_irc_message_split_cb, NULL, NULL);

    /* infolist hooks */
    weechat_hook_infolist (
        "irc_server",
        N_("list of IRC servers"),
        N_("server pointer (optional)"),
        N_("server name (wildcard \"*\" is allowed) (optional)"),
        &irc_info_infolist_irc_server_cb, NULL, NULL);
    weechat_hook_infolist (
        "irc_channel",
        N_("list of channels for an IRC server"),
        N_("channel pointer (optional)"),
        N_("server,channel (channel is optional)"),
        &irc_info_infolist_irc_channel_cb, NULL, NULL);
    weechat_hook_infolist (
        "irc_modelist",
        N_("list of channel mode lists for an IRC channel"),
        N_("mode list pointer (optional)"),
        N_("server,channel,type (type is optional)"),
        &irc_info_infolist_irc_modelist_cb, NULL, NULL);
    weechat_hook_infolist (
        "irc_modelist_item",
        N_("list of items in a channel mode list"),
        N_("mode list item pointer (optional)"),
        N_("server,channel,type,number (number is optional)"),
        &irc_info_infolist_irc_modelist_item_cb, NULL, NULL);
    weechat_hook_infolist (
        "irc_nick",
        N_("list of nicks for an IRC channel"),
        N_("nick pointer (optional)"),
        N_("server,channel,nick (nick is optional)"),
        &irc_info_infolist_irc_nick_cb, NULL, NULL);
    weechat_hook_infolist (
        "irc_ignore",
        N_("list of IRC ignores"),
        N_("ignore pointer (optional)"),
        NULL,
        &irc_info_infolist_irc_ignore_cb, NULL, NULL);
    weechat_hook_infolist (
        "irc_notify",
        N_("list of notify"),
        N_("notify pointer (optional)"),
        N_("server name (wildcard \"*\" is allowed) (optional)"),
        &irc_info_infolist_irc_notify_cb, NULL, NULL);
    weechat_hook_infolist (
        "irc_color_weechat",
        N_("mapping between IRC color codes and WeeChat color names"),
        NULL,
        NULL,
        &irc_info_infolist_irc_color_weechat_cb, NULL, NULL);

    /* hdata hooks */
    weechat_hook_hdata (
        "irc_nick", N_("irc nick"),
        &irc_nick_hdata_nick_cb, NULL, NULL);
    weechat_hook_hdata (
        "irc_modelist", N_("irc modelist"),
        &irc_modelist_hdata_modelist_cb, NULL, NULL);
    weechat_hook_hdata (
        "irc_modelist_item", N_("irc modelist item"),
        &irc_modelist_hdata_item_cb, NULL, NULL);
    weechat_hook_hdata (
        "irc_channel", N_("irc channel"),
        &irc_channel_hdata_channel_cb, NULL, NULL);
    weechat_hook_hdata (
        "irc_channel_speaking", N_("irc channel_speaking"),
        &irc_channel_hdata_channel_speaking_cb, NULL, NULL);
    weechat_hook_hdata (
        "irc_ignore", N_("irc ignore"),
        &irc_ignore_hdata_ignore_cb, NULL, NULL);
    weechat_hook_hdata (
        "irc_notify", N_("irc notify"),
        &irc_notify_hdata_notify_cb, NULL, NULL);
    weechat_hook_hdata (
        "irc_redirect_pattern", N_("pattern for irc redirect"),
        &irc_redirect_hdata_redirect_pattern_cb, NULL, NULL);
    weechat_hook_hdata (
        "irc_redirect", N_("irc redirect"),
        &irc_redirect_hdata_redirect_cb, NULL, NULL);
    weechat_hook_hdata (
        "irc_server", N_("irc server"),
        &irc_server_hdata_server_cb, NULL, NULL);
    weechat_hook_hdata (
        "irc_batch", N_("irc batch"),
        &irc_batch_hdata_batch_cb, NULL, NULL);
}
