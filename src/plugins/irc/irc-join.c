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
    struct t_irc_join_channel *channel1, *channel2;
    int rc;

    /* make C compiler happy */
    (void) arraylist;

    server = (struct t_irc_server *)data;

    channel1 = (struct t_irc_join_channel *)pointer1;
    channel2 = (struct t_irc_join_channel *)pointer2;

    /*
     * if channel is the same, always consider it's the same, even if the key
     * is different
     */
    rc = irc_server_strcasecmp (server, channel1->name, channel2->name);
    if (rc == 0)
        return 0;

    /* channels with a key are first in list */
    if (channel1->key && !channel2->key)
        return -1;
    if (!channel1->key && channel2->key)
        return 1;

    return 1;
}

/*
 * Frees a join channel.
 */

void
irc_join_free_cb (void *data, struct t_arraylist *arraylist, void *pointer)
{
    struct t_irc_join_channel *channel;

    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    channel = (struct t_irc_join_channel *)pointer;

    if (channel->name)
        free (channel->name);
    if (channel->key)
        free (channel->key);
    free (channel);
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
    struct t_irc_join_channel *channel;
    char **channels, **keys, *result;
    int size, i;

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
        channel = (struct t_irc_join_channel *)weechat_arraylist_get (
            arraylist, i);
        if (*channels[0])
            weechat_string_dyn_concat (channels, ",", -1);
        weechat_string_dyn_concat (channels, channel->name, -1);
        if (channel->key)
        {
            if (*keys[0])
                weechat_string_dyn_concat (keys, ",", -1);
            weechat_string_dyn_concat (keys, channel->key, -1);
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
    struct t_irc_join_channel *channel;
    char *new_join;

    if (!channel_name)
        return NULL;

    arraylist = irc_join_split (server, join);
    if (!arraylist)
        return NULL;

    channel = (struct t_irc_join_channel *)malloc (sizeof (*channel));
    channel->name = strdup (channel_name);
    channel->key = (key && key[0]) ? strdup (key) : NULL;
    weechat_arraylist_add (arraylist, channel);

    new_join = irc_join_build_string (arraylist);

    weechat_arraylist_free (arraylist);

    return new_join;
}

/*
 * Adds a channel with optional key to the autojoin option of a server.
 */

void
irc_join_add_channel_to_autojoin (struct t_irc_server *server,
                                  const char *channel_name, const char *key)
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
        weechat_config_option_set (
            server->options[IRC_SERVER_OPTION_AUTOJOIN],
            new_autojoin,
            1);
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
    struct t_irc_join_channel *channel;
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
        channel = (struct t_irc_join_channel *)weechat_arraylist_get (
            arraylist, i);
        if (irc_server_strcasecmp (server, channel->name, channel_name) == 0)
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
                                       const char *channel_name)
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
        weechat_config_option_set (
            server->options[IRC_SERVER_OPTION_AUTOJOIN],
            new_autojoin,
            1);
        free (new_autojoin);
    }
}
