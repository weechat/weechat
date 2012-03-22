/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * irc-channel.c: channel and private chat management for IRC plugin
 */

#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-channel.h"
#include "irc-buffer.h"
#include "irc-color.h"
#include "irc-command.h"
#include "irc-config.h"
#include "irc-nick.h"
#include "irc-server.h"
#include "irc-input.h"


/*
 * irc_channel_valid: check if a channel pointer exists for a server
 *                    return 1 if channel exists
 *                           0 if channel is not found
 */

int
irc_channel_valid (struct t_irc_server *server, struct t_irc_channel *channel)
{
    struct t_irc_channel *ptr_channel;

    if (!server)
        return 0;

    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ptr_channel == channel)
            return 1;
    }

    /* channel not found */
    return 0;
}

/*
 * irc_channel_move_near_server: move new channel/pv buffer near server
 */

void
irc_channel_move_near_server (struct t_irc_server *server, int channel_type,
                              struct t_gui_buffer *buffer)
{
    int number, number_channel, number_last_channel, number_last_private;
    int number_found;
    char str_number[32];
    struct t_irc_channel *ptr_channel;

    number = weechat_buffer_get_integer (buffer, "number");
    number_last_channel = 0;
    number_last_private = 0;
    number_found = 0;

    if (server->channels)
    {
        /* search last channel/pv number for server */
        for (ptr_channel = server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if (ptr_channel->buffer)
            {
                number_channel = weechat_buffer_get_integer (ptr_channel->buffer,
                                                             "number");
                switch (ptr_channel->type)
                {
                    case IRC_CHANNEL_TYPE_CHANNEL:
                        if (number_channel > number_last_channel)
                            number_last_channel = number_channel;
                        break;
                    case IRC_CHANNEL_TYPE_PRIVATE:
                        if (number_channel > number_last_private)
                            number_last_private = number_channel;
                        break;
                }
            }
        }

        /* use last channel/pv number + 1 */
        switch (channel_type)
        {
            case IRC_CHANNEL_TYPE_CHANNEL:
                if (number_last_channel > 0)
                    number_found = number_last_channel + 1;
                break;
            case IRC_CHANNEL_TYPE_PRIVATE:
                if (number_last_private > 0)
                    number_found = number_last_private + 1;
                else if (number_last_channel > 0)
                    number_found = number_last_channel + 1;
                break;
        }
    }
    else
    {
        if (weechat_config_integer (irc_config_look_server_buffer) ==
            IRC_CONFIG_LOOK_SERVER_BUFFER_INDEPENDENT)
        {
            number_found = weechat_buffer_get_integer (server->buffer, "number") + 1;
        }
    }

    /* switch to number found */
    if ((number_found >= 1) && (number_found != number))
    {
        snprintf (str_number, sizeof (str_number), "%d", number_found);
        weechat_buffer_set (buffer, "number", str_number);
    }
}

/*
 * irc_channel_new: allocate a new channel for a server and add it to channels
 *                  list
 */

struct t_irc_channel *
irc_channel_new (struct t_irc_server *server, int channel_type,
                 const char *channel_name, int switch_to_channel,
                 int auto_switch)
{
    struct t_irc_channel *new_channel;
    struct t_gui_buffer *new_buffer;
    int i, buffer_created, current_buffer_number, buffer_position, manual_join;
    char *buffer_name, str_number[32], str_group[32], *channel_name_lower;
    const char *prefix_modes;

    /* alloc memory for new channel */
    if ((new_channel = malloc (sizeof (*new_channel))) == NULL)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot allocate new channel"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME);
        return NULL;
    }

    /* create buffer for channel (or use existing one) */
    buffer_created = 0;
    buffer_name = irc_buffer_build_name (server->name, channel_name);
    new_buffer = weechat_buffer_search (IRC_PLUGIN_NAME, buffer_name);
    if (new_buffer)
        weechat_nicklist_remove_all (new_buffer);
    else
    {
        current_buffer_number = weechat_buffer_get_integer (weechat_current_buffer (),
                                                            "number");
        new_buffer = weechat_buffer_new (buffer_name,
                                         &irc_input_data_cb, NULL,
                                         &irc_buffer_close_cb, NULL);
        if (!new_buffer)
        {
            free (new_channel);
            return NULL;
        }
        if (weechat_buffer_get_integer (new_buffer, "layout_number") < 1)
        {
            buffer_position = (channel_type == IRC_CHANNEL_TYPE_CHANNEL) ?
                weechat_config_integer (irc_config_look_new_channel_position) :
                weechat_config_integer (irc_config_look_new_pv_position);
            switch (buffer_position)
            {
                case IRC_CONFIG_LOOK_BUFFER_POSITION_NONE:
                    /* do nothing */
                    break;
                case IRC_CONFIG_LOOK_BUFFER_POSITION_NEXT:
                    /* move buffer to current number + 1 */
                    snprintf (str_number, sizeof (str_number),
                              "%d", current_buffer_number + 1);
                    weechat_buffer_set (new_buffer, "number", str_number);
                    break;
                case IRC_CONFIG_LOOK_BUFFER_POSITION_NEAR_SERVER:
                    /* move buffer after last channel/pv of server */
                    irc_channel_move_near_server (server, channel_type, new_buffer);
                    break;
            }
        }
        buffer_created = 1;
    }

    if (buffer_created)
    {
        if (!weechat_buffer_get_integer (new_buffer, "short_name_is_set"))
            weechat_buffer_set (new_buffer, "short_name", channel_name);
        weechat_buffer_set (new_buffer, "localvar_set_type",
                            (channel_type == IRC_CHANNEL_TYPE_CHANNEL) ? "channel" : "private");
        weechat_buffer_set (new_buffer, "localvar_set_nick", server->nick);
        weechat_buffer_set (new_buffer, "localvar_set_server", server->name);
        weechat_buffer_set (new_buffer, "localvar_set_channel", channel_name);
        if (server->is_away && server->away_message)
            weechat_buffer_set (new_buffer, "localvar_set_away", server->away_message);
        weechat_hook_signal_send ("logger_backlog",
                                  WEECHAT_HOOK_SIGNAL_POINTER, new_buffer);
        if (weechat_config_boolean (irc_config_network_send_unknown_commands))
            weechat_buffer_set (new_buffer, "input_get_unknown_commands", "1");
        if (channel_type == IRC_CHANNEL_TYPE_CHANNEL)
        {
            weechat_buffer_set (new_buffer, "nicklist", "1");
            weechat_buffer_set (new_buffer, "nicklist_display_groups", "0");
        }

        /* set highlights settings on channel buffer */
        weechat_buffer_set(new_buffer, "highlight_words_add",
                           (channel_type == IRC_CHANNEL_TYPE_CHANNEL) ?
                           weechat_config_string (irc_config_look_highlight_channel) :
                           weechat_config_string (irc_config_look_highlight_pv));
        if (weechat_config_string (irc_config_look_highlight_tags)
            && weechat_config_string (irc_config_look_highlight_tags)[0])
        {
            weechat_buffer_set (new_buffer, "highlight_tags",
                                weechat_config_string (irc_config_look_highlight_tags));
        }
    }

    if (channel_type == IRC_CHANNEL_TYPE_CHANNEL)
    {
        prefix_modes = irc_server_get_prefix_modes (server);
        for (i = 0; prefix_modes[i]; i++)
        {
            snprintf (str_group, sizeof (str_group), "%03d|%c",
                      i, prefix_modes[i]);
            weechat_nicklist_add_group (new_buffer, NULL, str_group,
                                        "weechat.color.nicklist_group", 1);
        }
        snprintf (str_group, sizeof (str_group), "%03d|%s",
                  IRC_NICK_GROUP_OTHER_NUMBER, IRC_NICK_GROUP_OTHER_NAME);
        weechat_nicklist_add_group (new_buffer, NULL, str_group,
                                    "weechat.color.nicklist_group", 1);
    }

    /* initialize new channel */
    new_channel->type = channel_type;
    new_channel->name = strdup (channel_name);
    new_channel->topic = NULL;
    new_channel->modes = NULL;
    new_channel->limit = 0;
    new_channel->key = NULL;
    new_channel->names_received = 0;
    new_channel->checking_away = 0;
    new_channel->away_message = NULL;
    new_channel->has_quit_server = 0;
    new_channel->cycle = 0;
    new_channel->part = 0;
    new_channel->nick_completion_reset = 0;
    new_channel->pv_remote_nick_color = NULL;
    new_channel->hook_autorejoin = NULL;
    new_channel->nicks_count = 0;
    new_channel->nicks = NULL;
    new_channel->last_nick = NULL;
    new_channel->nicks_speaking[0] = NULL;
    new_channel->nicks_speaking[1] = NULL;
    new_channel->nicks_speaking_time = NULL;
    new_channel->last_nick_speaking_time = NULL;
    new_channel->buffer = new_buffer;
    new_channel->buffer_as_string = NULL;

    /* add new channel to channels list */
    new_channel->prev_channel = server->last_channel;
    new_channel->next_channel = NULL;
    if (server->channels)
        (server->last_channel)->next_channel = new_channel;
    else
        server->channels = new_channel;
    server->last_channel = new_channel;

    manual_join = 0;
    channel_name_lower = NULL;
    if (channel_type == IRC_CHANNEL_TYPE_CHANNEL)
    {
        channel_name_lower = strdup (channel_name);
        if (channel_name_lower)
        {
            weechat_string_tolower (channel_name_lower);
            manual_join = weechat_hashtable_has_key (server->manual_joins,
                                                     channel_name_lower);
        }
    }

    if (switch_to_channel)
    {
        if (channel_type == IRC_CHANNEL_TYPE_CHANNEL)
        {
            if ((manual_join && !weechat_config_boolean (irc_config_look_buffer_switch_join))
                || (!manual_join && !weechat_config_boolean (irc_config_look_buffer_switch_autojoin)))
                switch_to_channel = 0;
        }

        if (switch_to_channel)
        {
            weechat_buffer_set (new_buffer, "display",
                                (auto_switch && !manual_join) ? "auto" : "1");
        }
    }

    if (manual_join)
        weechat_hashtable_remove (server->manual_joins, channel_name_lower);
    if (channel_name_lower)
        free (channel_name_lower);

    weechat_hook_signal_send ((channel_type == IRC_CHANNEL_TYPE_CHANNEL) ?
                              "irc_channel_opened" : "irc_pv_opened",
                              WEECHAT_HOOK_SIGNAL_POINTER, new_buffer);

    /* all is ok, return address of new channel */
    return new_channel;
}

/*
 * irc_channel_set_topic: set topic for a channel
 */

void
irc_channel_set_topic (struct t_irc_channel *channel, const char *topic)
{
    if (channel->topic)
        free (channel->topic);

    channel->topic = (topic) ? strdup (topic) : NULL;
    weechat_buffer_set (channel->buffer, "title",
                        (channel->topic) ? channel->topic : "");
}

/*
 * irc_channel_set_modes: set modes for a channel
 */

void
irc_channel_set_modes (struct t_irc_channel *channel, const char *modes)
{
    if (channel->modes)
        free (channel->modes);

    channel->modes = (modes) ? strdup (modes) : NULL;
}

/*
 * irc_channel_search: returns pointer on a channel with name
 */

struct t_irc_channel *
irc_channel_search (struct t_irc_server *server, const char *channel_name)
{
    struct t_irc_channel *ptr_channel;

    if (!server || !channel_name)
        return NULL;

    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (irc_server_strcasecmp (server, ptr_channel->name, channel_name) == 0)
            return ptr_channel;
    }
    return NULL;
}

/*
 * irc_channel_is_channel: returns 1 if string is a channel for given server
 */

int
irc_channel_is_channel (struct t_irc_server *server, const char *string)
{
    char first_char[2];

    if (!string)
        return 0;

    first_char[0] = string[0];
    first_char[1] = '\0';
    return (strpbrk (first_char,
                     (server && server->chantypes) ?
                     server->chantypes : IRC_CHANNEL_DEFAULT_CHANTYPES)) ?
        1 : 0;
}

/*
 * irc_channel_remove_away: remove away for all nicks on a channel
 */

void
irc_channel_remove_away (struct t_irc_server *server,
                         struct t_irc_channel *channel)
{
    struct t_irc_nick *ptr_nick;

    if (channel->type == IRC_CHANNEL_TYPE_CHANNEL)
    {
        for (ptr_nick = channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
        {
            irc_nick_set_away (server, channel, ptr_nick, 0);
        }
    }
}

/*
 * irc_channel_check_away: check for away on a channel
 */

void
irc_channel_check_away (struct t_irc_server *server,
                        struct t_irc_channel *channel)
{
    if ((channel->type == IRC_CHANNEL_TYPE_CHANNEL) && channel->nicks)
    {
        if ((IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AWAY_CHECK) > 0)
            && ((IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AWAY_CHECK_MAX_NICKS) == 0)
                || (channel->nicks_count <= IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AWAY_CHECK_MAX_NICKS))))
        {
            channel->checking_away++;
            irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_LOW, NULL,
                              "WHO %s", channel->name);
        }
        else
            irc_channel_remove_away (server, channel);
    }
}

/*
 * irc_channel_set_away: set/unset away status for a channel
 */

void
irc_channel_set_away (struct t_irc_server *server,
                      struct t_irc_channel *channel, const char *nick_name,
                      int is_away)
{
    struct t_irc_nick *ptr_nick;

    if (channel->type == IRC_CHANNEL_TYPE_CHANNEL)
    {
        ptr_nick = irc_nick_search (server, channel, nick_name);
        if (ptr_nick)
            irc_nick_set_away (server, channel, ptr_nick, is_away);
    }
}

/*
 * irc_channel_nick_speaking_add_to_list: add a nick speaking on a channel
 */

void
irc_channel_nick_speaking_add_to_list (struct t_irc_channel *channel,
                                       const char *nick_name,
                                       int highlight)
{
    int size, to_remove, i;
    struct t_weelist_item *ptr_item;

    /* create list if it does not exist */
    if (!channel->nicks_speaking[highlight])
        channel->nicks_speaking[highlight] = weechat_list_new ();

    /* remove item if it was already in list */
    ptr_item = weechat_list_casesearch (channel->nicks_speaking[highlight],
                                        nick_name);
    if (ptr_item)
        weechat_list_remove (channel->nicks_speaking[highlight], ptr_item);

    /* add nick in list */
    weechat_list_add (channel->nicks_speaking[highlight], nick_name,
                      WEECHAT_LIST_POS_END, NULL);

    /* reduce list size if it's too big */
    size = weechat_list_size (channel->nicks_speaking[highlight]);
    if (size > IRC_CHANNEL_NICKS_SPEAKING_LIMIT)
    {
        to_remove = size - IRC_CHANNEL_NICKS_SPEAKING_LIMIT;
        for (i = 0; i < to_remove; i++)
        {
            weechat_list_remove (channel->nicks_speaking[highlight],
                                 weechat_list_get (channel->nicks_speaking[highlight], 0));
        }
    }
}

/*
 * irc_channel_nick_speaking_add: add a nick speaking on a channel
 */

void
irc_channel_nick_speaking_add (struct t_irc_channel *channel,
                               const char *nick_name, int highlight)
{
    if (highlight < 0)
        highlight = 0;
    if (highlight > 1)
        highlight = 1;
    if (highlight)
        irc_channel_nick_speaking_add_to_list (channel, nick_name, 1);

    irc_channel_nick_speaking_add_to_list (channel, nick_name, 0);
}

/*
 * irc_channel_nick_speaking_rename: rename a nick speaking on a channel
 */

void
irc_channel_nick_speaking_rename (struct t_irc_channel *channel,
                                  const char *old_nick,
                                  const char *new_nick)
{
    struct t_weelist_item *ptr_item;
    int i;

    for (i = 0; i < 2; i++)
    {
        if (channel->nicks_speaking[i])
        {
            ptr_item = weechat_list_search (channel->nicks_speaking[i], old_nick);
            if (ptr_item)
                weechat_list_set (ptr_item, new_nick);
        }
    }
}

/*
 * irc_channel_nick_speaking_time_search: search a nick speaking time on a
 *                                        channel
 */

struct t_irc_channel_speaking *
irc_channel_nick_speaking_time_search (struct t_irc_server *server,
                                       struct t_irc_channel *channel,
                                       const char *nick_name,
                                       int check_time)
{
    struct t_irc_channel_speaking *ptr_nick;
    time_t time_limit;

    time_limit = time (NULL) -
        (weechat_config_integer (irc_config_look_smart_filter_delay) * 60);

    for (ptr_nick = channel->nicks_speaking_time; ptr_nick;
         ptr_nick = ptr_nick->next_nick)
    {
        if (irc_server_strcasecmp (server, ptr_nick->nick, nick_name) == 0)
        {
            if (check_time && (ptr_nick->time_last_message < time_limit))
                return NULL;
            return ptr_nick;
        }
    }

    /* nick speaking time not found */
    return NULL;
}

/*
 * irc_channel_nick_speaking_time_free: free a nick speaking on a channel
 */

void
irc_channel_nick_speaking_time_free (struct t_irc_channel *channel,
                                     struct t_irc_channel_speaking *nick_speaking)
{
    /* free data */
    if (nick_speaking->nick)
        free (nick_speaking->nick);

    /* remove nick from list */
    if (nick_speaking->prev_nick)
        (nick_speaking->prev_nick)->next_nick = nick_speaking->next_nick;
    if (nick_speaking->next_nick)
        (nick_speaking->next_nick)->prev_nick = nick_speaking->prev_nick;
    if (channel->nicks_speaking_time == nick_speaking)
        channel->nicks_speaking_time = nick_speaking->next_nick;
    if (channel->last_nick_speaking_time == nick_speaking)
        channel->last_nick_speaking_time = nick_speaking->prev_nick;

    free (nick_speaking);
}

/*
 * irc_channel_nick_speaking_time_free_all: free all nick speaking on a channel
 */

void
irc_channel_nick_speaking_time_free_all (struct t_irc_channel *channel)
{
    while (channel->nicks_speaking_time)
    {
        irc_channel_nick_speaking_time_free (channel,
                                             channel->nicks_speaking_time);
    }
}

/*
 * irc_channel_nick_speaking_time_remove_old: remove old nicks speaking
 */

void
irc_channel_nick_speaking_time_remove_old (struct t_irc_channel *channel)
{
    time_t time_limit;

    time_limit = time (NULL) -
        (weechat_config_integer (irc_config_look_smart_filter_delay) * 60);

    while (channel->last_nick_speaking_time)
    {
        if (channel->last_nick_speaking_time->time_last_message >= time_limit)
            break;

        irc_channel_nick_speaking_time_free (channel,
                                             channel->last_nick_speaking_time);
    }
}

/*
 * irc_channel_nick_speaking_time_add: add a nick speaking time on a channel
 */

void
irc_channel_nick_speaking_time_add (struct t_irc_server *server,
                                    struct t_irc_channel *channel,
                                    const char *nick_name,
                                    time_t time_last_message)
{
    struct t_irc_channel_speaking *ptr_nick, *new_nick;

    ptr_nick = irc_channel_nick_speaking_time_search (server, channel,
                                                      nick_name, 0);
    if (ptr_nick)
        irc_channel_nick_speaking_time_free (channel, ptr_nick);

    new_nick = malloc (sizeof (*new_nick));
    if (new_nick)
    {
        new_nick->nick = strdup (nick_name);
        new_nick->time_last_message = time_last_message;

        /* insert nick at beginning of list */
        new_nick->prev_nick = NULL;
        new_nick->next_nick = channel->nicks_speaking_time;
        if (channel->nicks_speaking_time)
            channel->nicks_speaking_time->prev_nick = new_nick;
        else
            channel->last_nick_speaking_time = new_nick;
        channel->nicks_speaking_time = new_nick;
    }
}

/*
 * irc_channel_nick_speaking_time_rename: rename a nick speaking time on a
 *                                        channel
 */

void
irc_channel_nick_speaking_time_rename (struct t_irc_server *server,
                                       struct t_irc_channel *channel,
                                       const char *old_nick,
                                       const char *new_nick)
{
    struct t_irc_channel_speaking *ptr_nick;

    if (channel->nicks_speaking_time)
    {
        ptr_nick = irc_channel_nick_speaking_time_search (server, channel, old_nick, 0);
        if (ptr_nick)
        {
            free (ptr_nick->nick);
            ptr_nick->nick = strdup (new_nick);
        }
    }
}

/*
 * irc_channel_rejoin: rejoin a channel (for example after kick)
 */

void
irc_channel_rejoin (struct t_irc_server *server, struct t_irc_channel *channel)
{
    char join_args[256];

    snprintf (join_args, sizeof (join_args), "%s%s%s",
              channel->name,
              (channel->key) ? " " : "",
              (channel->key) ? channel->key : "");

    irc_command_join_server (server, join_args, 0);
}

/*
 * irc_channel_autorejoin_cb: callback for autorejoin on a channel
 */

int
irc_channel_autorejoin_cb (void *data, int remaining_calls)
{
    struct t_irc_server *ptr_server, *ptr_server_found;
    struct t_irc_channel *ptr_channel_arg, *ptr_channel;

    /* make C compiler happy */
    (void) remaining_calls;

    ptr_channel_arg = (struct t_irc_channel *)data;

    ptr_server_found = NULL;
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if (ptr_channel == ptr_channel_arg)
            {
                ptr_server_found = ptr_server;
                break;
            }
        }
    }

    if (ptr_server_found && (ptr_channel_arg->hook_autorejoin))
    {
        irc_channel_rejoin (ptr_server_found, ptr_channel_arg);
        ptr_channel_arg->hook_autorejoin = NULL;
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_channel_display_nick_back_in_pv: display a message in pv buffer if nick
 *                                      is back and if private has flag
 *                                      "has_quit_server"
 */

void
irc_channel_display_nick_back_in_pv (struct t_irc_server *server,
                                     struct t_irc_nick *nick,
                                     const char *nickname)
{
    struct t_irc_channel *ptr_channel;

    if (!server || (!nick && !nickname))
        return;

    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if ((ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
            && ptr_channel->has_quit_server
            && (irc_server_strcasecmp (server, ptr_channel->name, (nick) ? nick->name : nickname) == 0))
        {
            if (weechat_config_boolean (irc_config_look_display_pv_back))
            {
                weechat_printf (ptr_channel->buffer,
                                _("%s%s%s %s(%s%s%s)%s is back on server"),
                                weechat_prefix ("join"),
                                irc_nick_color_for_server_message (server,
                                                                   nick,
                                                                   nickname),
                                (nick) ? nick->name : nickname,
                                IRC_COLOR_CHAT_DELIMITERS,
                                IRC_COLOR_CHAT_HOST,
                                (nick && nick->host) ? nick->host : "",
                                IRC_COLOR_CHAT_DELIMITERS,
                                IRC_COLOR_MESSAGE_JOIN);
            }
            ptr_channel->has_quit_server = 0;
        }
    }
}

/*
 * irc_channel_free: free a channel and remove it from channels list
 */

void
irc_channel_free (struct t_irc_server *server, struct t_irc_channel *channel)
{
    struct t_irc_channel *new_channels;

    if (!server || !channel)
        return;

    /* remove channel from channels list */
    if (server->last_channel == channel)
        server->last_channel = channel->prev_channel;
    if (channel->prev_channel)
    {
        (channel->prev_channel)->next_channel = channel->next_channel;
        new_channels = server->channels;
    }
    else
        new_channels = channel->next_channel;

    if (channel->next_channel)
        (channel->next_channel)->prev_channel = channel->prev_channel;

    /* free data */
    if (channel->name)
        free (channel->name);
    if (channel->topic)
        free (channel->topic);
    if (channel->modes)
        free (channel->modes);
    if (channel->key)
        free (channel->key);
    irc_nick_free_all (server, channel);
    if (channel->away_message)
        free (channel->away_message);
    if (channel->pv_remote_nick_color)
        free (channel->pv_remote_nick_color);
    if (channel->hook_autorejoin)
        weechat_unhook (channel->hook_autorejoin);
    if (channel->nicks_speaking[0])
        weechat_list_free (channel->nicks_speaking[0]);
    if (channel->nicks_speaking[1])
        weechat_list_free (channel->nicks_speaking[1]);
    irc_channel_nick_speaking_time_free_all (channel);
    if (channel->buffer_as_string)
        free (channel->buffer_as_string);

    free (channel);

    server->channels = new_channels;
}

/*
 * irc_channel_free_all: free all allocated channels for a server
 */

void
irc_channel_free_all (struct t_irc_server *server)
{
    while (server->channels)
    {
        irc_channel_free (server, server->channels);
    }
}

/*
 * irc_channel_hdata_channel_cb: return hdata for channel
 */

struct t_hdata *
irc_channel_hdata_channel_cb (void *data, const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) data;

    hdata = weechat_hdata_new (hdata_name, "prev_channel", "next_channel");
    if (hdata)
    {
        WEECHAT_HDATA_VAR(struct t_irc_channel, type, INTEGER, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, name, STRING, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, topic, STRING, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, modes, STRING, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, limit, INTEGER, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, key, STRING, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, names_received, INTEGER, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, checking_away, INTEGER, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, away_message, STRING, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, has_quit_server, INTEGER, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, cycle, INTEGER, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, part, INTEGER, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, nick_completion_reset, INTEGER, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, pv_remote_nick_color, STRING, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, hook_autorejoin, POINTER, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, nicks_count, INTEGER, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, nicks, POINTER, "irc_nick");
        WEECHAT_HDATA_VAR(struct t_irc_channel, last_nick, POINTER, "irc_nick");
        WEECHAT_HDATA_VAR(struct t_irc_channel, nicks_speaking, POINTER, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, nicks_speaking_time, POINTER, "irc_channel_speaking");
        WEECHAT_HDATA_VAR(struct t_irc_channel, last_nick_speaking_time, POINTER, "irc_channel_speaking");
        WEECHAT_HDATA_VAR(struct t_irc_channel, buffer, POINTER, "buffer");
        WEECHAT_HDATA_VAR(struct t_irc_channel, buffer_as_string, STRING, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, prev_channel, POINTER, hdata_name);
        WEECHAT_HDATA_VAR(struct t_irc_channel, next_channel, POINTER, hdata_name);
    }
    return hdata;
}

/*
 * irc_channel_hdata_channel_speaking_cb: return hdata for channel_speaking
 */

struct t_hdata *
irc_channel_hdata_channel_speaking_cb (void *data, const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) data;

    hdata = weechat_hdata_new (hdata_name, "prev_nick", "next_nick");
    if (hdata)
    {
        WEECHAT_HDATA_VAR(struct t_irc_channel_speaking, nick, STRING, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel_speaking, time_last_message, TIME, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel_speaking, prev_nick, POINTER, hdata_name);
        WEECHAT_HDATA_VAR(struct t_irc_channel_speaking, next_nick, POINTER, hdata_name);
    }
    return hdata;
}

/*
 * irc_channel_add_to_infolist: add a channel in an infolist
 *                              return 1 if ok, 0 if error
 */

int
irc_channel_add_to_infolist (struct t_infolist *infolist,
                             struct t_irc_channel *channel)
{
    struct t_infolist_item *ptr_item;
    struct t_weelist_item *ptr_list_item;
    struct t_irc_channel_speaking *ptr_nick;
    char option_name[64];
    int i, index;

    if (!infolist || !channel)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_pointer (ptr_item, "buffer", channel->buffer))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "buffer_name",
                                          (channel->buffer) ?
                                          weechat_buffer_get_string (channel->buffer, "name") : ""))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "buffer_short_name",
                                          (channel->buffer) ?
                                          weechat_buffer_get_string (channel->buffer, "short_name") : ""))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "type", channel->type))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "name", channel->name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "topic", channel->topic))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "modes", channel->modes))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "limit", channel->limit))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "key", channel->key))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "names_received", channel->names_received))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "checking_away", channel->checking_away))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "away_message", channel->away_message))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "has_quit_server", channel->has_quit_server))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "cycle", channel->cycle))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "part", channel->part))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "nick_completion_reset", channel->nick_completion_reset))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "pv_remote_nick_color", channel->pv_remote_nick_color))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "nicks_count", channel->nicks_count))
        return 0;
    for (i = 0; i < 2; i++)
    {
        if (channel->nicks_speaking[i])
        {
            index = 0;
            for (ptr_list_item = weechat_list_get (channel->nicks_speaking[i], 0);
                 ptr_list_item;
                 ptr_list_item = weechat_list_next (ptr_list_item))
            {
                snprintf (option_name, sizeof (option_name),
                          "nick_speaking%d_%05d", i, index);
                if (!weechat_infolist_new_var_string (ptr_item, option_name,
                                                      weechat_list_string (ptr_list_item)))
                    return 0;
                index++;
            }
        }
    }
    if (channel->nicks_speaking_time)
    {
        i = 0;
        for (ptr_nick = channel->last_nick_speaking_time; ptr_nick;
             ptr_nick = ptr_nick->prev_nick)
        {
            snprintf (option_name, sizeof (option_name),
                      "nick_speaking_time_nick_%05d", i);
            if (!weechat_infolist_new_var_string (ptr_item, option_name,
                                                  ptr_nick->nick))
                return 0;
            snprintf (option_name, sizeof (option_name),
                      "nick_speaking_time_time_%05d", i);
            if (!weechat_infolist_new_var_time (ptr_item, option_name,
                                                ptr_nick->time_last_message))
                return 0;
            i++;
        }
    }

    return 1;
}

/*
 * irc_channel_print_log: print channel infos in log (usually for crash dump)
 */

void
irc_channel_print_log (struct t_irc_channel *channel)
{
    struct t_weelist_item *ptr_item;
    struct t_irc_channel_speaking *ptr_nick_speaking;
    int i, index;
    struct t_irc_nick *ptr_nick;

    weechat_log_printf ("");
    weechat_log_printf ("  => channel %s (addr:0x%lx):", channel->name, channel);
    weechat_log_printf ("       type . . . . . . . . . . : %d",    channel->type);
    weechat_log_printf ("       topic. . . . . . . . . . : '%s'",  channel->topic);
    weechat_log_printf ("       modes. . . . . . . . . . : '%s'",  channel->modes);
    weechat_log_printf ("       limit. . . . . . . . . . : %d",    channel->limit);
    weechat_log_printf ("       key. . . . . . . . . . . : '%s'",  channel->key);
    weechat_log_printf ("       names_received . . . . . : %d",    channel->names_received);
    weechat_log_printf ("       checking_away. . . . . . : %d",    channel->checking_away);
    weechat_log_printf ("       away_message . . . . . . : '%s'",  channel->away_message);
    weechat_log_printf ("       has_quit_server. . . . . : %d",    channel->has_quit_server);
    weechat_log_printf ("       cycle. . . . . . . . . . : %d",    channel->cycle);
    weechat_log_printf ("       part . . . . . . . . . . : %d",    channel->part);
    weechat_log_printf ("       nick_completion_reset. . : %d",    channel->nick_completion_reset);
    weechat_log_printf ("       pv_remote_nick_color . . : '%s'",  channel->pv_remote_nick_color);
    weechat_log_printf ("       hook_autorejoin. . . . . : 0x%lx", channel->hook_autorejoin);
    weechat_log_printf ("       nicks_count. . . . . . . : %d",    channel->nicks_count);
    weechat_log_printf ("       nicks. . . . . . . . . . : 0x%lx", channel->nicks);
    weechat_log_printf ("       last_nick. . . . . . . . : 0x%lx", channel->last_nick);
    weechat_log_printf ("       nicks_speaking[0]. . . . : 0x%lx", channel->nicks_speaking[0]);
    weechat_log_printf ("       nicks_speaking[1]. . . . : 0x%lx", channel->nicks_speaking[1]);
    weechat_log_printf ("       nicks_speaking_time. . . : 0x%lx", channel->nicks_speaking_time);
    weechat_log_printf ("       last_nick_speaking_time. : 0x%lx", channel->last_nick_speaking_time);
    weechat_log_printf ("       buffer . . . . . . . . . : 0x%lx", channel->buffer);
    weechat_log_printf ("       buffer_as_string . . . . : '%s'",  channel->buffer_as_string);
    weechat_log_printf ("       prev_channel . . . . . . : 0x%lx", channel->prev_channel);
    weechat_log_printf ("       next_channel . . . . . . : 0x%lx", channel->next_channel);
    for (i = 0; i < 2; i++)
    {
        if (channel->nicks_speaking[i])
        {
            weechat_log_printf ("");
            index = 0;
            for (ptr_item = weechat_list_get (channel->nicks_speaking[i], 0);
                 ptr_item; ptr_item = weechat_list_next (ptr_item))
            {
                weechat_log_printf ("         nick speaking[%d][%d]: '%s'",
                                    i, index, weechat_list_string (ptr_item));
                index++;
            }
        }
    }
    if (channel->nicks_speaking_time)
    {
        weechat_log_printf ("");
        for (ptr_nick_speaking = channel->nicks_speaking_time;
             ptr_nick_speaking;
             ptr_nick_speaking = ptr_nick_speaking->next_nick)
        {
            weechat_log_printf ("         nick speaking time: '%s', time: %ld",
                                ptr_nick_speaking->nick,
                                ptr_nick_speaking->time_last_message);
        }
    }
    for (ptr_nick = channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
        irc_nick_print_log (ptr_nick);
    }
}
