/*
 * irc-join.c - functions for list of channels to join
 *
 * Copyright (C) 2022-2024 Sébastien Helleu <flashcode@flashtux.org>
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
 * Compares two join channels: name and key.
 */

int
irc_join_compare_join_channel (struct t_irc_server *server,
                               struct t_irc_join_channel *join_channel1,
                               struct t_irc_join_channel *join_channel2)
{
    int rc;

    rc = irc_server_strcasecmp (server,
                                join_channel1->name, join_channel2->name);
    if (rc != 0)
        return rc;

    if (!join_channel1->key && !join_channel2->key)
        return 0;
    if (join_channel1->key && !join_channel2->key)
        return -1;
    if (!join_channel1->key && join_channel2->key)
        return 1;

    return strcmp (join_channel1->key, join_channel2->key);
}

/*
 * Compares two join channels: no sort, keyed channels first.
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
 * Compares two join channels: alphabetic sort, keyed channels first.
 */

int
irc_join_compare_sort_alpha_cb (void *data, struct t_arraylist *arraylist,
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

    return rc;
}

/*
 * Compares two join channels: buffer sort, then alphabetic,
 * keyed channels first.
 */

int
irc_join_compare_sort_buffer_cb (void *data, struct t_arraylist *arraylist,
                                 void *pointer1, void *pointer2)
{
    struct t_irc_server *server;
    struct t_irc_channel *ptr_channel1, *ptr_channel2;
    struct t_irc_join_channel *ptr_join_chan1, *ptr_join_chan2;
    int rc, buffer_num1, buffer_num2;

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

    /* search buffer number for each channel */
    ptr_channel1 = irc_channel_search (server, ptr_join_chan1->name);
    buffer_num1 = (ptr_channel1 && ptr_channel1->buffer) ?
        weechat_buffer_get_integer (ptr_channel1->buffer, "number") : INT_MAX;
    ptr_channel2 = irc_channel_search (server, ptr_join_chan2->name);
    buffer_num2 = (ptr_channel2 && ptr_channel2->buffer) ?
        weechat_buffer_get_integer (ptr_channel2->buffer, "number") : INT_MAX;
    if (buffer_num1 < buffer_num2)
        return -1;
    if (buffer_num1 > buffer_num2)
        return 1;

    /* same buffer number: fallback on alphabetic sort */
    return rc;
}

/*
 * Frees a join channel.
 */

void
irc_join_free_join_channel (struct t_irc_join_channel *join_channel)
{
    free (join_channel->name);
    free (join_channel->key);

    free (join_channel);
}

/*
 * Callback called to free a join channel.
 */

void
irc_join_free_cb (void *data, struct t_arraylist *arraylist, void *pointer)
{
    struct t_irc_join_channel *ptr_join_chan;

    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    ptr_join_chan = (struct t_irc_join_channel *)pointer;
    irc_join_free_join_channel (ptr_join_chan);
}

/*
 * Removes all occurrences of a channel from the array list then adds the
 * join channel (channel + key).
 *
 * Returns:
 *   1: join channel added to the list
 *   0: join channel NOT added to the list
 *      (the caller must then free it if needed)
 */

int
irc_join_arraylist_add (struct t_arraylist *arraylist,
                        struct t_irc_server *server,
                        struct t_irc_join_channel *join_channel)
{
    struct t_irc_join_channel *ptr_join_chan, *ptr_join_chan_exact;
    int index, removed;

    index = 0;
    ptr_join_chan_exact = NULL;
    while (index < weechat_arraylist_size (arraylist))
    {
        ptr_join_chan = weechat_arraylist_get (arraylist, index);
        removed = 0;
        if (ptr_join_chan)
        {
            if (irc_join_compare_join_channel (server,
                                               ptr_join_chan,
                                               join_channel) == 0)
            {
                if (ptr_join_chan_exact)
                {
                    weechat_arraylist_remove (arraylist, index);
                    removed = 1;
                }
                else
                {
                    ptr_join_chan_exact = ptr_join_chan;
                }
            }
            else if (irc_server_strcasecmp (server, ptr_join_chan->name,
                                            join_channel->name) == 0)
            {
                weechat_arraylist_remove (arraylist, index);
                removed = 1;
            }
        }
        if (!removed)
            index++;
    }

    if (ptr_join_chan_exact)
    {
        free (ptr_join_chan_exact->name);
        ptr_join_chan_exact->name = strdup (join_channel->name);
        free (ptr_join_chan_exact->key);
        ptr_join_chan_exact->key = (join_channel->key) ?
            strdup (join_channel->key) : NULL;
        return 0;
    }

    weechat_arraylist_add (arraylist, join_channel);

    return 1;
}

/*
 * Splits join string and returns an array list with a list of
 * channels/keys.
 *
 * The format of channels/keys is the one specified by RFC 1459 for the JOIN
 * command (channels with key first in list), for example:
 *
 *   #channel1,#channel2,#channel3 key1,key2
 *
 * Parameter sort can be (in all cases, keyed channels are first in list):
 *   IRC_JOIN_SORT_DISABLED: no sort
 *   IRC_JOIN_SORT_ALPHA: alphabetic sort
 *   IRC_JOIN_SORT_BUFFER: sort by buffer number, then alphabetic
 */

struct t_arraylist *
irc_join_split (struct t_irc_server *server, const char *join,
                enum t_irc_join_sort sort)
{
    struct t_arraylist *arraylist;
    char **items, **channels, **keys;
    int count_items, count_channels, count_keys, i;
    const char *ptr_channels, *ptr_keys;
    struct t_irc_join_channel *new_channel;
    void *sort_cb;

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

    switch (sort)
    {
        case IRC_JOIN_SORT_ALPHA:
            sort_cb = &irc_join_compare_sort_alpha_cb;
            break;
        case IRC_JOIN_SORT_BUFFER:
            sort_cb = &irc_join_compare_sort_buffer_cb;
            break;
        default:
            sort_cb = &irc_join_compare_cb;
            break;
    }

    arraylist = weechat_arraylist_new (16, 1, 0,
                                       sort_cb, server,
                                       &irc_join_free_cb, NULL);
    if (!arraylist)
        goto end;

    for (i = 0; i < count_channels; i++)
    {
        new_channel = (struct t_irc_join_channel *)malloc (sizeof (*new_channel));
        new_channel->name = strdup (channels[i]);
        new_channel->key = (i < count_keys) ? strdup (keys[i]) : NULL;
        if (!irc_join_arraylist_add (arraylist, server, new_channel))
            irc_join_free_join_channel (new_channel);
    }

end:
    weechat_string_free_split (items);
    weechat_string_free_split (channels);
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
        result = weechat_string_dyn_free (channels, 0);
    weechat_string_dyn_free (keys, 1);

    return (result) ? result : strdup ("");
}

/*
 * Checks if a channel is in a join string.
 *
 * Returns:
 *   1: channel found in join string (case insensitive comparison)
 *   0: channel NOT found in join string
 */

int
irc_join_has_channel (struct t_irc_server *server,
                      const char *join, const char *channel_name)
{
    struct t_arraylist *arraylist;
    struct t_irc_join_channel *ptr_join_chan;
    int i, found;

    if (!join || !join[0] || !channel_name || !channel_name[0])
        return 0;

    arraylist = irc_join_split (server, join, 0);
    if (!arraylist)
        return 0;

    found = 0;
    for (i = 0; i < weechat_arraylist_size (arraylist); i++)
    {
        ptr_join_chan = (struct t_irc_join_channel *)weechat_arraylist_get (
            arraylist, i);
        if (irc_server_strcasecmp (server, ptr_join_chan->name,
                                   channel_name) == 0)
        {
            found = 1;
            break;
        }
    }

    weechat_arraylist_free (arraylist);

    return found;
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

    arraylist = irc_join_split (server, join, 0);
    if (!arraylist)
        return NULL;

    join_chan = (struct t_irc_join_channel *)malloc (sizeof (*join_chan));
    join_chan->name = strdup (channel_name);
    join_chan->key = (key && key[0]) ? strdup (key) : NULL;
    if (!irc_join_arraylist_add (arraylist, server, join_chan))
        irc_join_free_join_channel (join_chan);

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

    arraylist = irc_join_split (server, join, 0);
    if (!arraylist)
        return NULL;

    arraylist2 = irc_join_split (server, join2, 0);
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
        if (!irc_join_arraylist_add (arraylist, server, join_chan))
            irc_join_free_join_channel (join_chan);
    }

    new_join = irc_join_build_string (arraylist);

    weechat_arraylist_free (arraylist);
    weechat_arraylist_free (arraylist2);

    return new_join;
}

/*
 * Sets the server autojoin option to a new value.
 *
 * If the autojoin contains a link to secure data (eg: "${sec.data.xxx}" with
 * nothing before "${" and nothing after "}"), then the content of secure data
 * is updated and the server autojoin option is kept as-is.
 */

void
irc_join_set_autojoin_option (struct t_irc_server *server,
                              const char *join)
{
    const char *ptr_autojoin, *pos_option, *pos_closing_brace;
    char *sec_data_name, **command;

    sec_data_name = NULL;
    ptr_autojoin = IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_AUTOJOIN);

    if (ptr_autojoin && ptr_autojoin[0]
        && (strncmp (ptr_autojoin, "${sec.data.", 11) == 0))
    {
        pos_option = ptr_autojoin + 11;
        pos_closing_brace = strchr (pos_option, '}');
        if (pos_closing_brace && !pos_closing_brace[1])
        {
            sec_data_name = weechat_strndup (pos_option,
                                             pos_closing_brace - pos_option);
        }
    }

    if (sec_data_name)
    {
        command = weechat_string_dyn_alloc (128);
        weechat_string_dyn_concat (command, "/mute /secure set ", -1);
        weechat_string_dyn_concat (command, sec_data_name, -1);
        weechat_string_dyn_concat (command, " ", -1);
        weechat_string_dyn_concat (command, join, -1);
        weechat_command (weechat_buffer_search_main (), *command);
        weechat_string_dyn_free (command, 1);
        free (sec_data_name);
    }
    else
    {
        weechat_config_option_set (server->options[IRC_SERVER_OPTION_AUTOJOIN],
                                   join, 1);
    }
}

/*
 * Adds a channel with optional key to the autojoin option of a server.
 */

void
irc_join_add_channel_to_autojoin (struct t_irc_server *server,
                                  const char *channel_name, const char *key)
{
    char *old_autojoin, *new_autojoin;

    if (!channel_name)
        return;

    old_autojoin = irc_server_eval_expression (
        server,
        IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_AUTOJOIN));

    new_autojoin = irc_join_add_channel (server, old_autojoin, channel_name, key);
    if (new_autojoin)
    {
        irc_join_set_autojoin_option (server, new_autojoin);
        free (new_autojoin);
    }

    free (old_autojoin);
}

/*
 * Adds channels with optional keys to the autojoin option of a server.
 */

void
irc_join_add_channels_to_autojoin (struct t_irc_server *server,
                                   const char *join)
{
    char *old_autojoin, *new_autojoin;

    old_autojoin = irc_server_eval_expression (
        server,
        IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_AUTOJOIN));

    new_autojoin = irc_join_add_channels (server, old_autojoin, join);
    if (new_autojoin)
    {
        irc_join_set_autojoin_option (server, new_autojoin);
        free (new_autojoin);
    }

    free (old_autojoin);
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

    arraylist = irc_join_split (server, join, 0);
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
                                       const char *channel_name)
{
    char *old_autojoin, *new_autojoin;

    if (!channel_name)
        return;

    old_autojoin = irc_server_eval_expression (
        server,
        IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_AUTOJOIN));

    new_autojoin = irc_join_remove_channel (server, old_autojoin, channel_name);
    if (new_autojoin)
    {
        irc_join_set_autojoin_option (server, new_autojoin);
        free (new_autojoin);
    }

    free (old_autojoin);
}

/*
 * Renames a channel in a join string: removes the channel then adds it (with
 * its key, if set).
 *
 * Channels with a key are first in list, so for example:
 *
 *           join     = "#abc,#def,#ghi key_abc,key_def"
 *   channel_name     = "#def"
 *   new_channel_name = "#zzz"
 *
 *   => returned value: "#abc,#zzz,#ghi key_abc,key_def"
 *
 * If channel_name == new_channel_name (ignoring case), the function does
 * nothing.
 *
 * If channel_name is not in the list, the function does nothing.
 *
 * If the new_channel_name is already in the list, the channel_name is just
 * removed.
 *
 * Note: result must be freed after use.
 */

char *
irc_join_rename_channel (struct t_irc_server *server,
                         const char *join,
                         const char *channel_name,
                         const char *new_channel_name)
{
    struct t_arraylist *arraylist;
    struct t_irc_join_channel *ptr_join_chan;
    char *new_join;
    int i, to_remove;

    if (!channel_name || !new_channel_name)
        return NULL;

    if (irc_server_strcasecmp (server, channel_name, new_channel_name) == 0)
        return (join) ? strdup (join) : NULL;

    arraylist = irc_join_split (server, join, 0);
    if (!arraylist)
        return NULL;

    /* check if new channel name is already in the list */
    to_remove = 0;
    for (i = 0; i < weechat_arraylist_size (arraylist); i++)
    {
        ptr_join_chan = (struct t_irc_join_channel *)weechat_arraylist_get (
            arraylist, i);
        if (irc_server_strcasecmp (server, ptr_join_chan->name,
                                   new_channel_name) == 0)
        {
            to_remove = 1;
            break;
        }
    }

    i = 0;
    while (i < weechat_arraylist_size (arraylist))
    {
        ptr_join_chan = (struct t_irc_join_channel *)weechat_arraylist_get (
            arraylist, i);
        if (irc_server_strcasecmp (server, ptr_join_chan->name,
                                   channel_name) == 0)
        {
            if (to_remove)
            {
                weechat_arraylist_remove (arraylist, i);
            }
            else
            {
                free (ptr_join_chan->name);
                ptr_join_chan->name = strdup (new_channel_name);
                i++;
            }
        }
        else
        {
            i++;
        }
    }

    new_join = irc_join_build_string (arraylist);

    weechat_arraylist_free (arraylist);

    return new_join;
}

/*
 * Renames a channel in a server autojoin option.
 */

void
irc_join_rename_channel_in_autojoin (struct t_irc_server *server,
                                     const char *channel_name,
                                     const char *new_channel_name)
{
    char *old_autojoin, *new_autojoin;

    if (!channel_name || !new_channel_name)
        return;

    old_autojoin = irc_server_eval_expression (
        server,
        IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_AUTOJOIN));

    new_autojoin = irc_join_rename_channel (server, old_autojoin,
                                            channel_name, new_channel_name);
    if (new_autojoin)
    {
        irc_join_set_autojoin_option (server, new_autojoin);
        free (new_autojoin);
    }

    free (old_autojoin);
}

/*
 * Saves currently joined channels in the autojoin option of a server.
 */

void
irc_join_save_channels_to_autojoin (struct t_irc_server *server)
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
            if (!irc_join_arraylist_add (arraylist, server, join_chan))
                irc_join_free_join_channel (join_chan);
        }
    }

    new_autojoin = irc_join_build_string (arraylist);
    if (new_autojoin)
    {
        irc_join_set_autojoin_option (server, new_autojoin);
        free (new_autojoin);
    }

    weechat_arraylist_free (arraylist);
}

/*
 * Sorts channels.
 */

char *
irc_join_sort_channels (struct t_irc_server *server, const char *join,
                        enum t_irc_join_sort sort)
{
    struct t_arraylist *arraylist;
    char *new_join;

    arraylist = irc_join_split (server, join, sort);
    if (!arraylist)
        return NULL;

    new_join = irc_join_build_string (arraylist);

    weechat_arraylist_free (arraylist);

    return new_join;
}

/*
 * Sorts channels in autojoin option of a server.
 */

void
irc_join_sort_autojoin (struct t_irc_server *server, enum t_irc_join_sort sort)
{
    char *old_autojoin, *new_autojoin;

    if (!server)
        return;

    old_autojoin = irc_server_eval_expression (
        server,
        IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_AUTOJOIN));

    if (!old_autojoin || !old_autojoin[0])
    {
        free (old_autojoin);
        return;
    }

    new_autojoin = irc_join_sort_channels (server, old_autojoin, sort);
    if (new_autojoin)
    {
        irc_join_set_autojoin_option (server, new_autojoin);
        free (new_autojoin);
    }

    free (old_autojoin);
}
