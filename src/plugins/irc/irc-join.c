/*
 * irc-join.c - functions for list of channels to join
 *
 * Copyright (C) 2022 Sébastien Helleu <flashcode@flashtux.org>
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
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-join.h"
#include "irc-channel.h"
#include "irc-config.h"
#include "irc-server.h"


/*
 * Compares two join channels.
 */

int
irc_join_compare_cb (void *data, struct t_arraylist *arraylist,
                     void *pointer1, void *pointer2)
{
    struct t_irc_server *server;
    struct t_irc_join_channel *ptr_join_chan1, *ptr_join_chan2;
    int rc;

    /* make C compiler happy */
    (void) arraylist;

    server = (struct t_irc_server *)data;

    ptr_join_chan1 = (struct t_irc_join_channel *)pointer1;
    ptr_join_chan2 = (struct t_irc_join_channel *)pointer2;

    /*
     * if channel is the same, always consider it's the same, even if the key
     * is different
     */
    rc = irc_server_strcasecmp (server, ptr_join_chan1->name,
                                ptr_join_chan2->name);
    if (rc == 0)
        return 0;

    /* channels with a key are first in list */
    if (ptr_join_chan1->key && !ptr_join_chan2->key)
        return -1;
    if (!ptr_join_chan1->key && ptr_join_chan2->key)
        return 1;

    return 1;
}

/*
 * Frees a join channel.
 */

void
irc_join_free_cb (void *data, struct t_arraylist *arraylist, void *pointer)
{
    struct t_irc_join_channel *ptr_join_chan;

    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    ptr_join_chan = (struct t_irc_join_channel *)pointer;

    if (ptr_join_chan->name)
        free (ptr_join_chan->name);
    if (ptr_join_chan->key)
        free (ptr_join_chan->key);
    free (ptr_join_chan);
}

/*
 * Splits join string and returns an array list with a list of
 * channels/keys.
 *
 * The format of channels/keys is the one specified by RFC 1459 for the JOIN
 * command (channels with key first in list), for example:
 *
 *   #channel1,#channel2,#channel3 key1,key2
 */

struct t_arraylist *
irc_join_split (struct t_irc_server *server, const char *join)
{
    struct t_arraylist *arraylist;
    char **items, **channels, **keys;
    int count_items, count_channels, count_keys, i;
    const char *ptr_channels, *ptr_keys;
    struct t_irc_join_channel *new_channel;

    arraylist = NULL;
    items = NULL;
    count_items = 0;
    ptr_channels = NULL;
    ptr_keys = NULL;
    channels = NULL;
    count_channels = 0;
    keys = NULL;
    count_keys = 0;

    items = weechat_string_split ((join) ? join : "",
                                  " ", NULL,
                                  WEECHAT_STRING_SPLIT_STRIP_LEFT |
                                  WEECHAT_STRING_SPLIT_STRIP_RIGHT |
                                  WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                  0, &count_items);

    if (count_items >= 1)
        ptr_channels = items[0];
    if (count_items >= 2)
        ptr_keys = items[1];

    if (ptr_channels)
    {
        channels = weechat_string_split (ptr_channels, ",", NULL,
                                         WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                         0, &count_channels);
    }

    if (ptr_keys)
    {
        keys = weechat_string_split (ptr_keys, ",", NULL,
                                     WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                     0, &count_keys);
    }

    arraylist = weechat_arraylist_new (
        16, 1, 0,
        &irc_join_compare_cb, server,
        &irc_join_free_cb, NULL);
    if (!arraylist)
        goto end;

    for (i = 0; i < count_channels; i++)
    {
        new_channel = (struct t_irc_join_channel *)malloc (sizeof (*new_channel));
        new_channel->name = strdup (channels[i]);
        new_channel->key = (i < count_keys) ? strdup (keys[i]) : NULL;
        weechat_arraylist_add (arraylist, new_channel);
    }

end:
    if (items)
        weechat_string_free_split (items);
    if (channels)
        weechat_string_free_split (channels);
    if (keys)
        weechat_string_free_split (keys);

    return arraylist;
}

/*
 * Builds string with a list of channels/keys.
 *
 * Note: result must be freed after use.
 */

char *
irc_join_build_string (struct t_arraylist *arraylist)
{
    struct t_irc_join_channel *ptr_join_chan;
    char **channels, **keys, *result;
    int i, size;

    if (!arraylist)
        return strdup ("");

    channels = NULL;
    keys = NULL;
    result = NULL;

    channels = weechat_string_dyn_alloc (1024);
    if (!channels)
        goto end;
    keys = weechat_string_dyn_alloc (1024);
    if (!keys)
        goto end;

    size = weechat_arraylist_size (arraylist);
    for (i = 0; i < size; i++)
    {
        ptr_join_chan = (struct t_irc_join_channel *)weechat_arraylist_get (
            arraylist, i);
        if (*channels[0])
            weechat_string_dyn_concat (channels, ",", -1);
        weechat_string_dyn_concat (channels, ptr_join_chan->name, -1);
        if (ptr_join_chan->key)
        {
            if (*keys[0])
                weechat_string_dyn_concat (keys, ",", -1);
            weechat_string_dyn_concat (keys, ptr_join_chan->key, -1);
        }
    }

    if (*keys[0])
    {
        weechat_string_dyn_concat (channels, " ", -1);
        weechat_string_dyn_concat (channels, *keys, -1);
    }

end:
    if (channels)
    {
        result = *channels;
        weechat_string_dyn_free (channels, 0);
    }
    if (keys)
        weechat_string_dyn_free (keys, 1);

    return (result) ? result : strdup ("");
}

/*
 * Adds a channel with optional key to the join string.
 *
 * Channels with a key are first in list, so for example:
 *
 *           join = "#abc,#def,#ghi key_abc,key_def"
 *   channel_name = "#xyz"
 *            key = "key_xyz"
 *
 *   => returned value: "#abc,#def,#xyz,#ghi key_abc,key_def,key_xyz"
 *
 * Note: result must be freed after use.
 */

char *
irc_join_add_channel (struct t_irc_server *server,
                      const char *join,
                      const char *channel_name, const char *key)
{
    struct t_arraylist *arraylist;
    struct t_irc_join_channel *join_chan;
    char *new_join;

    if (!channel_name)
        return NULL;

    arraylist = irc_join_split (server, join);
    if (!arraylist)
        return NULL;

    join_chan = (struct t_irc_join_channel *)malloc (sizeof (*join_chan));
    join_chan->name = strdup (channel_name);
    join_chan->key = (key && key[0]) ? strdup (key) : NULL;
    weechat_arraylist_add (arraylist, join_chan);

    new_join = irc_join_build_string (arraylist);

    weechat_arraylist_free (arraylist);

    return new_join;
}

/*
 * Adds channels with optional keys to the join string.
 *
 * Channels with a key are first in list, so for example:
 *
 *    join = "#abc,#def,#ghi key_abc,key_def"
 *   join2 = "#xyz,#jkl key_xyz"
 *
 *   => returned value: "#abc,#def,#xyz,#ghi,#jkl key_abc,key_def,key_xyz"
 *
 * Note: result must be freed after use.
 */

char *
irc_join_add_channels (struct t_irc_server *server,
                       const char *join, const char *join2)
{
    struct t_arraylist *arraylist, *arraylist2;
    struct t_irc_join_channel *ptr_join_chan, *join_chan;
    char *new_join;
    int i, size;

    arraylist = irc_join_split (server, join);
    if (!arraylist)
        return NULL;

    arraylist2 = irc_join_split (server, join2);
    if (!arraylist2)
    {
        weechat_arraylist_free (arraylist);
        return NULL;
    }

    size = weechat_arraylist_size (arraylist2);
    for (i = 0; i < size; i++)
    {
        ptr_join_chan = (struct t_irc_join_channel *)weechat_arraylist_get (
            arraylist2, i);
        join_chan = (struct t_irc_join_channel *)malloc (sizeof (*join_chan));
        join_chan->name = strdup (ptr_join_chan->name);
        join_chan->key = (ptr_join_chan->key && ptr_join_chan->key[0]) ?
            strdup (ptr_join_chan->key) : NULL;
        weechat_arraylist_add (arraylist, join_chan);
    }

    new_join = irc_join_build_string (arraylist);

    weechat_arraylist_free (arraylist);
    weechat_arraylist_free (arraylist2);

    return new_join;
}

/*
 * Sets server autojoin option.
 *
 * If verbose == 1, displays a message on the server buffer with old and new
 * autojoin value.
 */

void
irc_join_set_autojoin (struct t_irc_server *server, const char *autojoin,
                       int verbose)
{
    const char *ptr_old_autojoin;
    char *old_autojoin;

    if (!server)
        return;

    ptr_old_autojoin = IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_AUTOJOIN);
    old_autojoin = (ptr_old_autojoin && ptr_old_autojoin[0]) ?
        strdup (ptr_old_autojoin) : NULL;

    weechat_config_option_set (server->options[IRC_SERVER_OPTION_AUTOJOIN],
                               autojoin, 1);

    if (verbose)
    {
        if (old_autojoin)
        {
            weechat_printf (server->buffer,
                            _("Autojoin changed from \"%s\" to \"%s\""),
                            old_autojoin,
                            autojoin);
        }
        else
        {
            weechat_printf (server->buffer,
                            _("Autojoin changed from empty value to \"%s\""),
                            autojoin);
        }
    }

    if (old_autojoin)
        free (old_autojoin);
}

/*
 * Adds a channel with optional key to the autojoin option of a server.
 */

void
irc_join_add_channel_to_autojoin (struct t_irc_server *server,
                                  const char *channel_name, const char *key,
                                  int verbose)
{
    char *new_autojoin;

    if (!channel_name)
        return;

    new_autojoin = irc_join_add_channel (
        server,
        IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_AUTOJOIN),
        channel_name,
        key);
    if (new_autojoin)
    {
        irc_join_set_autojoin (server, new_autojoin, verbose);
        free (new_autojoin);
    }
}

/*
 * Adds channels with optional keys to the autojoin option of a server.
 */

void
irc_join_add_channels_to_autojoin (struct t_irc_server *server,
                                   const char *join, int verbose)
{
    char *new_autojoin;

    new_autojoin = irc_join_add_channels (
        server,
        IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_AUTOJOIN),
        join);
    if (new_autojoin)
    {
        irc_join_set_autojoin (server, new_autojoin, verbose);
        free (new_autojoin);
    }
}

/*
 * Removes a channel from a join string.
 *
 * Channels with a key are first in list, so for example:
 *
 *           join = "#abc,#def,#ghi key_abc,key_def"
 *   channel_name = "#def"
 *
 *   => returned value: "#abc,#ghi key_abc"
 *
 * Note: result must be freed after use.
 */

char *
irc_join_remove_channel (struct t_irc_server *server,
                         const char *join, const char *channel_name)
{
    struct t_arraylist *arraylist;
    struct t_irc_join_channel *ptr_join_chan;
    char *new_join;
    int i;

    if (!channel_name)
        return NULL;

    arraylist = irc_join_split (server, join);
    if (!arraylist)
        return NULL;

    i = 0;
    while (i < weechat_arraylist_size (arraylist))
    {
        ptr_join_chan = (struct t_irc_join_channel *)weechat_arraylist_get (
            arraylist, i);
        if (irc_server_strcasecmp (server, ptr_join_chan->name, channel_name) == 0)
            weechat_arraylist_remove (arraylist, i);
        else
            i++;
    }

    new_join = irc_join_build_string (arraylist);

    weechat_arraylist_free (arraylist);

    return new_join;
}

/*
 * Removes a channel from server autojoin option.
 */

void
irc_join_remove_channel_from_autojoin (struct t_irc_server *server,
                                       const char *channel_name,
                                       int verbose)
{
    char *new_autojoin;

    if (!channel_name)
        return;

    new_autojoin = irc_join_remove_channel (
        server,
        IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_AUTOJOIN),
        channel_name);
    if (new_autojoin)
    {
        irc_join_set_autojoin (server, new_autojoin, verbose);
        free (new_autojoin);
    }
}

/*
 * Saves currently joined channels in the autojoin option of a server.
 */

void
irc_join_save_channels_to_autojoin (struct t_irc_server *server, int verbose)
{
    struct t_arraylist *arraylist;
    struct t_irc_channel *ptr_channel;
    struct t_irc_join_channel *join_chan;
    char *new_autojoin;

    if (!server)
        return;

    arraylist = weechat_arraylist_new (
        16, 1, 0,
        &irc_join_compare_cb, server,
        &irc_join_free_cb, NULL);
    if (!arraylist)
        return;

    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if ((ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
            && !ptr_channel->part)
        {
            join_chan = (struct t_irc_join_channel *)malloc (sizeof (*join_chan));
            join_chan->name = strdup (ptr_channel->name);
            join_chan->key = (ptr_channel->key && ptr_channel->key[0]) ?
                strdup (ptr_channel->key) : NULL;
            weechat_arraylist_add (arraylist, join_chan);
        }
    }

    new_autojoin = irc_join_build_string (arraylist);
    if (new_autojoin)
    {
        irc_join_set_autojoin (server, new_autojoin, verbose);
        free (new_autojoin);
    }

    weechat_arraylist_free (arraylist);
}
