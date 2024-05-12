/*
 * irc-channel.c - channel and private chat management for IRC plugin
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include "irc-input.h"
#include "irc-join.h"
#include "irc-modelist.h"
#include "irc-nick.h"
#include "irc-protocol.h"
#include "irc-server.h"
#include "irc-upgrade.h"


char *irc_channel_typing_state_string[IRC_CHANNEL_NUM_TYPING_STATES] =
{ "off", "active", "paused", "done" };

/* default CHANTYPES */
char *irc_channel_default_chantypes = "#&";


/*
 * Checks if a channel pointer is valid for a server.
 *
 * Returns:
 *   1: channel exists for server
 *   0: channel does not exist for server
 */

int
irc_channel_valid (struct t_irc_server *server, struct t_irc_channel *channel)
{
    struct t_irc_channel *ptr_channel;

    if (!server || !channel)
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
 * Searches for a channel by name.
 *
 * Returns pointer to channel found, NULL if not found.
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
 * Searches for a channel buffer by channel name.
 *
 * Returns pointer to buffer found, NULL if not found.
 */

struct t_gui_buffer *
irc_channel_search_buffer (struct t_irc_server *server, int channel_type,
                           const char *channel_name)
{
    struct t_hdata *hdata_buffer;
    struct t_gui_buffer *ptr_buffer;
    const char *ptr_type, *ptr_server_name, *ptr_channel_name;

    if (!channel_name)
        return NULL;

    hdata_buffer = weechat_hdata_get ("buffer");
    ptr_buffer = weechat_hdata_get_list (hdata_buffer, "gui_buffers");

    while (ptr_buffer)
    {
        if (weechat_buffer_get_pointer (ptr_buffer,
                                        "plugin") == weechat_irc_plugin)
        {
            ptr_type = weechat_buffer_get_string (ptr_buffer, "localvar_type");
            ptr_server_name = weechat_buffer_get_string (ptr_buffer,
                                                         "localvar_server");
            ptr_channel_name = weechat_buffer_get_string (ptr_buffer,
                                                          "localvar_channel");
            if (ptr_type && ptr_type[0]
                && ptr_server_name && ptr_server_name[0]
                && ptr_channel_name && ptr_channel_name[0]
                && (((channel_type == IRC_CHANNEL_TYPE_CHANNEL)
                     && (strcmp (ptr_type, "channel") == 0))
                    || ((channel_type == IRC_CHANNEL_TYPE_PRIVATE)
                        && (strcmp (ptr_type, "private") == 0)))
                && (strcmp (ptr_server_name, server->name) == 0)
                && ((irc_server_strcasecmp (server, ptr_channel_name,
                                            channel_name) == 0)))
            {
                return ptr_buffer;
            }
        }

        /* move to next buffer */
        ptr_buffer = weechat_hdata_move (hdata_buffer, ptr_buffer, 1);
    }

    /* buffer not found */
    return NULL;
}

/*
 * Applies properties to a buffer.
 */

void
irc_channel_apply_props (void *data,
                         struct t_hashtable *hashtable,
                         const void *key,
                         const void *value)
{
    /* make C compiler happy */
    (void) hashtable;

    weechat_buffer_set ((struct t_gui_buffer *)data,
                        (const char *)key,
                        (const char *)value);
}

/*
 * Creates a buffer for a channel.
 */

struct t_gui_buffer *
irc_channel_create_buffer (struct t_irc_server *server,
                           int channel_type,
                           const char *channel_name,
                           int switch_to_channel,
                           int auto_switch)
{
    struct t_gui_buffer *ptr_buffer, *ptr_buffer_for_merge;
    struct t_hashtable *buffer_props;
    int buffer_created, current_buffer_number, buffer_position;
    int autojoin_join, manual_join, noswitch;
    char str_number[32], *channel_name_lower, *buffer_name;
    const char *short_name, *localvar_channel;

    buffer_created = 0;
    buffer_props = NULL;
    buffer_name = irc_buffer_build_name (server->name, channel_name);

    ptr_buffer = irc_channel_search_buffer (server, channel_type,
                                            channel_name);
    if (!ptr_buffer && (channel_type == IRC_CHANNEL_TYPE_PRIVATE))
    {
        /*
         * in case of private buffer, we reuse a buffer which has wrong type
         * "channel" (opened by a manual /join or autojoin)
         */
        ptr_buffer = irc_channel_search_buffer (server,
                                                IRC_CHANNEL_TYPE_CHANNEL,
                                                channel_name);
        if (ptr_buffer)
            weechat_bar_item_update ("buffer_name");
    }

    buffer_props = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (buffer_props)
    {
        weechat_hashtable_set (
            buffer_props,
            "input_multiline",
            (weechat_hashtable_has_key (server->cap_list, "batch")
             && weechat_hashtable_has_key (server->cap_list, "draft/multiline")) ?
            "1" : "0");
        weechat_hashtable_set (buffer_props, "name", buffer_name);
        weechat_hashtable_set (
            buffer_props,
            "localvar_set_type",
            (channel_type == IRC_CHANNEL_TYPE_CHANNEL) ? "channel" : "private");
        weechat_hashtable_set (buffer_props, "localvar_set_nick", server->nick);
        weechat_hashtable_set (buffer_props, "localvar_set_host", server->host);
        weechat_hashtable_set (buffer_props, "localvar_set_server", server->name);
        weechat_hashtable_set (buffer_props, "localvar_set_channel", channel_name);
        if (server->is_away && server->away_message)
        {
            weechat_hashtable_set (buffer_props,
                                   "localvar_set_away", server->away_message);
        }
        else
        {
            weechat_hashtable_set (buffer_props, "localvar_del_away", "");
        }
    }

    if (ptr_buffer)
    {
        if (!irc_upgrading)
            weechat_nicklist_remove_all (ptr_buffer);
        weechat_hashtable_map (buffer_props, &irc_channel_apply_props, ptr_buffer);
    }
    else
    {
        ptr_buffer_for_merge = NULL;
        if (channel_type == IRC_CHANNEL_TYPE_PRIVATE)
        {
            switch (weechat_config_enum (irc_config_look_pv_buffer))
            {
                case IRC_CONFIG_LOOK_PV_BUFFER_MERGE_BY_SERVER:
                    /* merge private buffers by server */
                    ptr_buffer_for_merge = irc_buffer_search_private_lowest_number (server);
                    break;
                case IRC_CONFIG_LOOK_PV_BUFFER_MERGE_ALL:
                    /* merge *ALL* private buffers */
                    ptr_buffer_for_merge = irc_buffer_search_private_lowest_number (NULL);
                    break;
            }
        }
        current_buffer_number = weechat_buffer_get_integer (
            weechat_current_buffer (), "number");

        ptr_buffer = weechat_buffer_new_props (
            buffer_name,
            buffer_props,
            &irc_input_data_cb, NULL, NULL,
            &irc_buffer_close_cb, NULL, NULL);
        if (!ptr_buffer)
            goto end;

        if (weechat_buffer_get_integer (ptr_buffer, "layout_number") < 1)
        {
            buffer_position = (channel_type == IRC_CHANNEL_TYPE_CHANNEL) ?
                weechat_config_enum (irc_config_look_new_channel_position) :
                weechat_config_enum (irc_config_look_new_pv_position);
            switch (buffer_position)
            {
                case IRC_CONFIG_LOOK_BUFFER_POSITION_NONE:
                    /* do nothing */
                    break;
                case IRC_CONFIG_LOOK_BUFFER_POSITION_NEXT:
                    /* move buffer to current number + 1 */
                    snprintf (str_number, sizeof (str_number),
                              "%d", current_buffer_number + 1);
                    weechat_buffer_set (ptr_buffer, "number", str_number);
                    break;
                case IRC_CONFIG_LOOK_BUFFER_POSITION_NEAR_SERVER:
                    /* move buffer after last channel/pv of server */
                    irc_buffer_move_near_server (
                        server,
                        0,  /* list_buffer */
                        channel_type,
                        ptr_buffer);
                    break;
            }
            if (ptr_buffer_for_merge)
                weechat_buffer_merge (ptr_buffer, ptr_buffer_for_merge);
        }
        buffer_created = 1;
    }

    if (buffer_created)
    {
        if (!weechat_buffer_get_integer (ptr_buffer, "short_name_is_set"))
            weechat_buffer_set (ptr_buffer, "short_name", channel_name);
    }
    else
    {
        short_name = weechat_buffer_get_string (ptr_buffer, "short_name");
        localvar_channel = weechat_buffer_get_string (ptr_buffer,
                                                      "localvar_channel");
        if (!short_name
            || (localvar_channel
                && (strcmp (localvar_channel, short_name) == 0)))
        {
            /* update the short_name only if it was not changed by the user */
            weechat_buffer_set (ptr_buffer, "short_name", channel_name);
        }
    }

    if (buffer_created)
    {
        (void) weechat_hook_signal_send ("logger_backlog",
                                         WEECHAT_HOOK_SIGNAL_POINTER,
                                         ptr_buffer);
        if (weechat_config_boolean (irc_config_network_send_unknown_commands))
            weechat_buffer_set (ptr_buffer, "input_get_unknown_commands", "1");
        if (channel_type == IRC_CHANNEL_TYPE_CHANNEL)
        {
            weechat_buffer_set (ptr_buffer, "nicklist", "1");
            weechat_buffer_set (ptr_buffer, "nicklist_display_groups", "0");
            weechat_buffer_set_pointer (ptr_buffer, "nickcmp_callback",
                                        &irc_buffer_nickcmp_cb);
            weechat_buffer_set_pointer (ptr_buffer, "nickcmp_callback_pointer",
                                        server);
        }

        /* set highlights settings on channel buffer */
        weechat_buffer_set (
            ptr_buffer,
            "highlight_words_add",
            (channel_type == IRC_CHANNEL_TYPE_CHANNEL) ?
            weechat_config_string (irc_config_look_highlight_channel) :
            weechat_config_string (irc_config_look_highlight_pv));
        if (weechat_config_string (irc_config_look_highlight_tags_restrict)
            && weechat_config_string (irc_config_look_highlight_tags_restrict)[0])
        {
            weechat_buffer_set (
                ptr_buffer,
                "highlight_tags_restrict",
                weechat_config_string (irc_config_look_highlight_tags_restrict));
        }

        /* switch to new buffer (if needed) */
        autojoin_join = irc_join_has_channel (
            server,
            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_AUTOJOIN),
            channel_name);
        manual_join = 0;
        noswitch = 0;
        channel_name_lower = NULL;
        if (channel_type == IRC_CHANNEL_TYPE_CHANNEL)
        {
            channel_name_lower = weechat_string_tolower (channel_name);
            if (channel_name_lower)
            {
                manual_join = weechat_hashtable_has_key (server->join_manual,
                                                         channel_name_lower);
                noswitch = weechat_hashtable_has_key (server->join_noswitch,
                                                      channel_name_lower);
            }
        }
        if (switch_to_channel)
        {
            if (channel_type == IRC_CHANNEL_TYPE_CHANNEL)
            {
                if (noswitch
                    || (!manual_join && !autojoin_join)
                    || (manual_join && !weechat_config_boolean (irc_config_look_buffer_switch_join))
                    || (!manual_join && autojoin_join && !weechat_config_boolean (irc_config_look_buffer_switch_autojoin)))
                {
                    switch_to_channel = 0;
                }
            }
            if (switch_to_channel)
            {
                weechat_buffer_set (ptr_buffer, "display",
                                    (auto_switch && !manual_join) ? "auto" : "1");
            }
        }
        if (channel_name_lower)
        {
            weechat_hashtable_remove (server->join_noswitch, channel_name_lower);
            free (channel_name_lower);
        }
    }

end:
    weechat_hashtable_free (buffer_props);
    free (buffer_name);
    return ptr_buffer;
}

/*
 * Creates a new channel in a server.
 *
 * Returns pointer to new channel, NULL if error.
 */

struct t_irc_channel *
irc_channel_new (struct t_irc_server *server, int channel_type,
                 const char *channel_name, int switch_to_channel,
                 int auto_switch)
{
    struct t_irc_channel *new_channel;
    struct t_gui_buffer *ptr_buffer;
    const char *ptr_chanmode, *ptr_channel_key;
    char *channel_name_lower;

    /* create buffer for channel (or use existing one) */
    ptr_buffer = irc_channel_create_buffer (server, channel_type,
                                            channel_name, switch_to_channel,
                                            auto_switch);
    if (!ptr_buffer)
        return NULL;

    /* alloc memory for new channel */
    if ((new_channel = malloc (sizeof (*new_channel))) == NULL)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot allocate new channel"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME);
        return NULL;
    }

    /* initialize new channel */
    new_channel->type = channel_type;
    new_channel->name = strdup (channel_name);
    new_channel->topic = NULL;
    new_channel->modes = NULL;
    new_channel->limit = 0;
    new_channel->key = NULL;
    channel_name_lower = weechat_string_tolower (channel_name);
    if (channel_name_lower)
    {
        ptr_channel_key = weechat_hashtable_get (server->join_channel_key,
                                                 channel_name_lower);
        if (ptr_channel_key)
            new_channel->key = strdup (ptr_channel_key);
        free (channel_name_lower);
    }
    new_channel->join_msg_received = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    new_channel->checking_whox = 0;
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
    new_channel->modelists = NULL;
    new_channel->last_modelist = NULL;
    for (ptr_chanmode = irc_server_get_chanmodes (server); ptr_chanmode[0];
         ptr_chanmode++)
    {
        if (ptr_chanmode[0] != ',')
            irc_modelist_new (new_channel, ptr_chanmode[0]);
    }
    new_channel->join_smart_filtered = NULL;
    new_channel->typing_state = IRC_CHANNEL_TYPING_STATE_OFF;
    new_channel->typing_status_sent = 0;
    new_channel->buffer = ptr_buffer;
    new_channel->buffer_as_string = NULL;

    /* add new channel to channels list */
    new_channel->prev_channel = server->last_channel;
    new_channel->next_channel = NULL;
    if (server->last_channel)
        (server->last_channel)->next_channel = new_channel;
    else
        server->channels = new_channel;
    server->last_channel = new_channel;

    (void) weechat_hook_signal_send (
        (channel_type == IRC_CHANNEL_TYPE_CHANNEL) ?
        "irc_channel_opened" : "irc_pv_opened",
        WEECHAT_HOOK_SIGNAL_POINTER, ptr_buffer);

    /* all is OK, return address of new channel */
    return new_channel;
}

/*
 * Renames a private buffer.
 */

void
irc_channel_pv_rename (struct t_irc_server *server,
                       struct t_irc_channel *channel,
                       const char *new_name)
{
    char *buffer_name;

    if (!server || !channel || (channel->type != IRC_CHANNEL_TYPE_PRIVATE)
        || !new_name)
    {
        return;
    }

    free (channel->name);
    channel->name = strdup (new_name);
    if (channel->pv_remote_nick_color)
    {
        free (channel->pv_remote_nick_color);
        channel->pv_remote_nick_color = NULL;
    }
    buffer_name = irc_buffer_build_name (server->name, channel->name);
    if (buffer_name)
    {
        weechat_buffer_set (channel->buffer, "name", buffer_name);
        weechat_buffer_set (channel->buffer, "short_name", channel->name);
        weechat_buffer_set (channel->buffer, "localvar_set_channel", channel->name);
        free (buffer_name);
    }
}

/*
 * Adds groups in nicklist for a channel.
 */

void
irc_channel_add_nicklist_groups (struct t_irc_server *server,
                                 struct t_irc_channel *channel)
{
    const char *prefix_modes;
    char str_group[32];
    int i;

    if (channel->type != IRC_CHANNEL_TYPE_CHANNEL)
        return;

    prefix_modes = irc_server_get_prefix_modes (server);
    for (i = 0; prefix_modes[i]; i++)
    {
        snprintf (str_group, sizeof (str_group), "%03d|%c",
                  i, prefix_modes[i]);
        weechat_nicklist_add_group (channel->buffer, NULL, str_group,
                                    "weechat.color.nicklist_group", 1);
    }
    snprintf (str_group, sizeof (str_group), "%03d|%s",
              IRC_NICK_GROUP_OTHER_NUMBER, IRC_NICK_GROUP_OTHER_NAME);
    weechat_nicklist_add_group (channel->buffer, NULL, str_group,
                                "weechat.color.nicklist_group", 1);
}

/*
 * Sets modes on channel buffer.
 */

void
irc_channel_set_buffer_modes (struct t_irc_server *server,
                              struct t_irc_channel *channel)
{
    char *modes_without_args;
    const char *pos_space;

    if (!server || !channel || !channel->buffer)
        return;

    if ((channel->type == IRC_CHANNEL_TYPE_CHANNEL)
        && channel->nicks
        && channel->modes && channel->modes[0]
        && (strcmp (channel->modes, "+") != 0))
    {
        modes_without_args = NULL;
        if (!irc_config_display_channel_modes_arguments (channel->modes))
        {
            pos_space = strchr (channel->modes, ' ');
            if (pos_space)
            {
                modes_without_args = weechat_strndup (
                    channel->modes, pos_space - channel->modes);
            }
        }
        weechat_buffer_set (
            channel->buffer,
            "modes",
            (modes_without_args) ? modes_without_args : channel->modes);
        free (modes_without_args);
    }
    else
    {
        weechat_buffer_set (channel->buffer, "modes", "");
    }
}

/*
 * Sets input prompt on channel buffer.
 */

void
irc_channel_set_buffer_input_prompt (struct t_irc_server *server,
                                     struct t_irc_channel *channel)
{
    struct t_irc_nick *ptr_nick;
    int display_modes;
    char str_prefix[64], *prompt;

    if (!server || !channel || !channel->buffer)
        return;

    if (server->nick)
    {
        /* build prefix */
        str_prefix[0] = '\0';
        if (weechat_config_boolean (irc_config_look_item_nick_prefix)
            && (channel->type == IRC_CHANNEL_TYPE_CHANNEL))
        {
            ptr_nick = irc_nick_search (server, channel, server->nick);
            if (ptr_nick)
            {
                if (weechat_config_boolean (irc_config_look_nick_mode_empty)
                    || (ptr_nick->prefix[0] != ' '))
                {
                    snprintf (str_prefix, sizeof (str_prefix), "%s%s",
                              weechat_color (
                                  irc_nick_get_prefix_color_name (
                                      server, ptr_nick->prefix[0])),
                              ptr_nick->prefix);
                }
            }
        }

        display_modes = (weechat_config_boolean (irc_config_look_item_nick_modes)
                         && server->nick_modes && server->nick_modes[0]);

        weechat_asprintf (&prompt, "%s%s%s%s%s%s%s%s%s",
                          str_prefix,
                          IRC_COLOR_INPUT_NICK,
                          server->nick,
                          (display_modes) ? IRC_COLOR_BAR_DELIM : "",
                          (display_modes) ? "(" : "",
                          (display_modes) ? IRC_COLOR_ITEM_NICK_MODES : "",
                          (display_modes) ? server->nick_modes : "",
                          (display_modes) ? IRC_COLOR_BAR_DELIM : "",
                          (display_modes) ? ")" : "");
        if (prompt)
        {
            weechat_buffer_set (channel->buffer, "input_prompt", prompt);
            free (prompt);
        }
    }
    else
    {
        weechat_buffer_set (channel->buffer, "input_prompt", "");
    }
}

/*
 * Sets the buffer title with the channel topic.
 */

void
irc_channel_set_buffer_title (struct t_irc_channel *channel)
{
    char *title_color;

    if (channel->topic)
    {
        title_color = irc_color_decode (
            channel->topic,
            (weechat_config_boolean (irc_config_look_topic_strip_colors)) ? 0 : 1);
        weechat_buffer_set (channel->buffer, "title", title_color);
        free (title_color);
    }
    else
    {
        weechat_buffer_set (channel->buffer, "title", "");
    }
}

/*
 * Sets topic for a channel.
 */

void
irc_channel_set_topic (struct t_irc_channel *channel, const char *topic)
{
    int display_warning;

    /*
     * display a warning in the private buffer if the address of remote
     * nick has changed (that means you may talk to someone else!)
     */
    display_warning = (
        (channel->type == IRC_CHANNEL_TYPE_PRIVATE)
        && weechat_config_boolean (irc_config_look_display_pv_warning_address)
        && channel->topic && channel->topic[0]
        && topic && topic[0]
        && (strcmp (channel->topic, topic) != 0));

    free (channel->topic);
    channel->topic = (topic) ? strdup (topic) : NULL;

    irc_channel_set_buffer_title (channel);

    if (display_warning)
    {
        weechat_printf_date_tags (
                    channel->buffer,
                    0,
                    "no_log,warning_nick_address",
                    _("%sWarning: the address of remote nick has changed"),
                    weechat_prefix ("error"));
    }
}

/*
 * Sets topic of all private buffers with a nick.
 */

void
irc_channel_set_topic_private_buffers (struct t_irc_server *server,
                                       struct t_irc_nick *nick,
                                       const char *nickname,
                                       const char *topic)
{
    struct t_irc_channel *ptr_channel;

    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if ((ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
            && (irc_server_strcasecmp (server, ptr_channel->name, (nick) ? nick->name : nickname) == 0))
        {
            irc_channel_set_topic (ptr_channel, topic);
        }
    }
}

/*
 * Sets modes for a channel.
 */

void
irc_channel_set_modes (struct t_irc_channel *channel, const char *modes)
{
    free (channel->modes);
    channel->modes = (modes) ? strdup (modes) : NULL;
}

/*
 * Checks if a string is a valid channel name.
 *
 * Returns:
 *   1: string is a channel name
 *   0: string is not a channel name
 */

int
irc_channel_is_channel (struct t_irc_server *server, const char *string)
{
    char first_char[2];
    const char *ptr_chantypes;

    if (!string)
        return 0;

    first_char[0] = string[0];
    first_char[1] = '\0';

    ptr_chantypes = irc_server_get_chantypes (server);

    return (strpbrk (first_char, ptr_chantypes)) ? 1 : 0;
}

/*
 * Returns a string with a channel type to add in front of a channel name,
 * if it doesn't have a valid channel type for the given server.
 *
 * It returns an empty string if the channel already has a valid channel type,
 * or if the option irc.look.join_auto_add_chantype is off.
 */

const char *
irc_channel_get_auto_chantype (struct t_irc_server *server,
                               const char *channel_name)
{
    static char chantype[2];
    const char *ptr_chantypes;

    chantype[0] = '\0';
    chantype[1] = '\0';

    if (weechat_config_boolean (irc_config_look_join_auto_add_chantype)
        && !irc_channel_is_channel (server, channel_name))
    {
        ptr_chantypes = irc_server_get_chantypes (server);
        if (ptr_chantypes && ptr_chantypes[0])
        {
            /*
             * use '#' if it's in chantypes (anywhere in the string), because
             * it is the most common channel type, and fallback on first
             * channel type
             */
            chantype[0] = (strchr (ptr_chantypes, '#')) ?
                '#' : ptr_chantypes[0];
        }
    }

    return chantype;
}

/*
 * Removes account for all nicks on a channel.
 */

void
irc_channel_remove_account (struct t_irc_server *server,
                            struct t_irc_channel *channel)
{
    struct t_irc_nick *ptr_nick;

    /* make C compiler happy */
    (void) server;

    if (channel->type == IRC_CHANNEL_TYPE_CHANNEL)
    {
        for (ptr_nick = channel->nicks; ptr_nick;
             ptr_nick = ptr_nick->next_nick)
        {
            free (ptr_nick->account);
            ptr_nick->account = NULL;
        }
    }
}

/*
 * Removes away for all nicks on a channel.
 */

void
irc_channel_remove_away (struct t_irc_server *server,
                         struct t_irc_channel *channel)
{
    struct t_irc_nick *ptr_nick;

    if (channel->type == IRC_CHANNEL_TYPE_CHANNEL)
    {
        for (ptr_nick = channel->nicks; ptr_nick;
             ptr_nick = ptr_nick->next_nick)
        {
            irc_nick_set_away (server, channel, ptr_nick, 0);
        }
    }
}

/*
 * Checks for WHOX information on a channel.
 */

void
irc_channel_check_whox (struct t_irc_server *server,
                        struct t_irc_channel *channel)
{
    if ((channel->type == IRC_CHANNEL_TYPE_CHANNEL) && channel->nicks)
    {
        if (weechat_hashtable_has_key (server->cap_list, "away-notify")
            || weechat_hashtable_has_key (server->cap_list, "account-notify")
            || ((IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AWAY_CHECK) > 0)
                && ((IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AWAY_CHECK_MAX_NICKS) == 0)
                    || (channel->nicks_count <= IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AWAY_CHECK_MAX_NICKS)))))
        {
            channel->checking_whox++;
            if (irc_server_get_isupport_value (server, "WHOX"))
            {
                /* WHOX is supported */
                irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_LOW, NULL,
                                  "WHO %s %%cuhsnfdar", channel->name);
            }
            else
            {
                /* WHOX is NOT supported */
                irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_LOW, NULL,
                                  "WHO %s", channel->name);
            }
        }
        else
        {
            irc_channel_remove_account (server, channel);
            irc_channel_remove_away (server, channel);
        }
    }
}

/*
 * Sets/unsets away status for a channel.
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
 * Adds a nick speaking on a channel.
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
            weechat_list_remove (
                channel->nicks_speaking[highlight],
                weechat_list_get (channel->nicks_speaking[highlight], 0));
        }
    }
}

/*
 * Adds a nick speaking on a channel.
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
 * Renames a nick speaking on a channel.
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
            ptr_item = weechat_list_search (channel->nicks_speaking[i],
                                            old_nick);
            if (ptr_item)
                weechat_list_set (ptr_item, new_nick);
        }
    }
}

/*
 * Renames a nick speaking on a channel if it is already in list.
 */

void
irc_channel_nick_speaking_rename_if_present (struct t_irc_server *server,
                                             struct t_irc_channel *channel,
                                             const char *nick_name)
{
    struct t_weelist_item *ptr_item;
    int i, j, list_size;

    for (i = 0; i < 2; i++)
    {
        if (channel->nicks_speaking[i])
        {
            list_size = weechat_list_size (channel->nicks_speaking[i]);
            for (j = 0; j < list_size; j++)
            {
                ptr_item = weechat_list_get (channel->nicks_speaking[i], j);
                if (ptr_item
                    && (irc_server_strcasecmp (server,
                                               weechat_list_string (ptr_item),
                                               nick_name) == 0))
                {
                    weechat_list_set (ptr_item, nick_name);
                }
            }
        }
    }
}

/*
 * Searches for a nick speaking time on a channel.
 *
 * Returns pointer to nick speaking time, NULL if not found.
 */

struct t_irc_channel_speaking *
irc_channel_nick_speaking_time_search (struct t_irc_server *server,
                                       struct t_irc_channel *channel,
                                       const char *nick_name,
                                       int check_time)
{
    struct t_irc_channel_speaking *ptr_nick;
    time_t time_limit;

    if (!server || !channel || !nick_name)
        return NULL;

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
 * Frees a nick speaking on a channel.
 */

void
irc_channel_nick_speaking_time_free (struct t_irc_channel *channel,
                                     struct t_irc_channel_speaking *nick_speaking)
{
    if (!channel || !nick_speaking)
        return;

    /* free data */
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
 * Frees all nick speaking on a channel.
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
 * Removes old nicks speaking.
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
 * Adds a nick speaking time on a channel.
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
 * Renames a nick speaking time on a channel.
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
        ptr_nick = irc_channel_nick_speaking_time_search (server, channel,
                                                          old_nick, 0);
        if (ptr_nick)
        {
            free (ptr_nick->nick);
            ptr_nick->nick = strdup (new_nick);
        }
    }
}

/*
 * Adds a nick in hashtable "join_smart_filtered" (creates the hashtable if
 * needed).
 */

void
irc_channel_join_smart_filtered_add (struct t_irc_channel *channel,
                                     const char *nick,
                                     time_t join_time)
{
    /* return if unmasking of smart filtered joins is disabled */
    if (weechat_config_integer (irc_config_look_smart_filter_join_unmask) == 0)
        return;

    /* create hashtable if needed */
    if (!channel->join_smart_filtered)
    {
        channel->join_smart_filtered = weechat_hashtable_new (
            64,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_TIME,
            NULL, NULL);
    }
    if (!channel->join_smart_filtered)
        return;

    weechat_hashtable_set (channel->join_smart_filtered, nick, &join_time);
}

/*
 * Renames a nick in hashtable "join_smart_filtered".
 */

void
irc_channel_join_smart_filtered_rename (struct t_irc_channel *channel,
                                        const char *old_nick,
                                        const char *new_nick)
{
    time_t *ptr_time, join_time;

    /* return if hashtable does not exist in channel */
    if (!channel->join_smart_filtered)
        return;

    /* search old_nick in hashtable */
    ptr_time = weechat_hashtable_get (channel->join_smart_filtered, old_nick);
    if (!ptr_time)
        return;

    /* remove old_nick, add new_nick with time of old_nick */
    join_time = *ptr_time;
    weechat_hashtable_remove (channel->join_smart_filtered, old_nick);
    weechat_hashtable_set (channel->join_smart_filtered, new_nick, &join_time);
}

/*
 * Removes a nick in hashtable "join_smart_filtered".
 */

void
irc_channel_join_smart_filtered_remove (struct t_irc_channel *channel,
                                        const char *nick)
{
    /* return if hashtable does not exist in channel */
    if (!channel->join_smart_filtered)
        return;

    weechat_hashtable_remove (channel->join_smart_filtered, nick);
}

/*
 * Unmasks a smart filtered join if nick is in hashtable "join_smart_filtered",
 * then removes nick from hashtable.
 */

void
irc_channel_join_smart_filtered_unmask (struct t_irc_channel *channel,
                                        const char *nick)
{
    int i, unmask_delay, length_tags, nick_found, join, account;
    int chghost, setname, nick_changed, smart_filtered, remove_smart_filter;
    time_t *ptr_time, date_min;
    struct t_hdata *hdata_line, *hdata_line_data;
    struct t_gui_line *own_lines;
    struct t_gui_line *line;
    struct t_gui_line_data *line_data;
    const char **tags, *irc_nick1, *irc_nick2;
    char *new_tags, *nick_to_search;
    struct t_hashtable *hashtable;

    /* return if hashtable does not exist in channel */
    if (!channel->join_smart_filtered)
        return;

    /* return if unmasking of smart filtered joins is disabled */
    unmask_delay = weechat_config_integer (
        irc_config_look_smart_filter_join_unmask);
    if (unmask_delay == 0)
        return;

    /* check if nick is in hashtable "join_smart_filtered" */
    ptr_time = weechat_hashtable_get (channel->join_smart_filtered, nick);
    if (!ptr_time)
        return;

    /*
     * the min date allowed to unmask a join (a join older than this date will
     * not be unmasked)
     */
    date_min = time (NULL) - (unmask_delay * 60);

    /*
     * if the join is too old (older than current time - unmask delay), just
     * remove nick from hashtable and return
     */
    if (*ptr_time < date_min)
    {
        weechat_hashtable_remove (channel->join_smart_filtered, nick);
        return;
    }

    /* get hdata and pointers on last line in buffer */
    own_lines = weechat_hdata_pointer (weechat_hdata_get ("buffer"),
                                       channel->buffer, "own_lines");
    if (!own_lines)
        return;
    line = weechat_hdata_pointer (weechat_hdata_get ("lines"),
                                  own_lines, "last_line");
    if (!line)
        return;
    hdata_line = weechat_hdata_get ("line");
    hdata_line_data = weechat_hdata_get ("line_data");

    /* the nick to search in messages (track nick changes) */
    nick_to_search = strdup (nick);
    if (!nick_to_search)
        return;

    /* loop on lines until we find the join */
    while (line)
    {
        line_data = weechat_hdata_pointer (hdata_line, line, "data");
        if (!line_data)
            break;

        /* exit loop if we reach the unmask delay */
        if (weechat_hdata_time (hdata_line_data, line_data, "date_printed") < date_min)
            break;

        /* check tags in line */
        tags = weechat_hdata_pointer (hdata_line_data, line_data, "tags_array");
        if (tags)
        {
            length_tags = 0;
            nick_found = 0;
            join = 0;
            account = 0;
            chghost = 0;
            setname = 0;
            nick_changed = 0;
            irc_nick1 = NULL;
            irc_nick2 = NULL;
            smart_filtered = 0;
            for (i = 0; tags[i]; i++)
            {
                if (strncmp (tags[i], "nick_", 5) == 0)
                {
                    if (strcmp (tags[i] + 5, nick_to_search) == 0)
                        nick_found = 1;
                }
                else if (strcmp (tags[i], "irc_join") == 0)
                    join = 1;
                else if (strcmp (tags[i], "irc_account") == 0)
                    account = 1;
                else if (strcmp (tags[i], "irc_chghost") == 0)
                    chghost = 1;
                else if (strcmp (tags[i], "irc_setname") == 0)
                    setname = 1;
                else if (strcmp (tags[i], "irc_nick") == 0)
                    nick_changed = 1;
                else if (strncmp (tags[i], "irc_nick1_", 10) == 0)
                    irc_nick1 = tags[i] + 10;
                else if (strncmp (tags[i], "irc_nick2_", 10) == 0)
                    irc_nick2 = tags[i] + 10;
                else if (strcmp (tags[i], "irc_smart_filter") == 0)
                    smart_filtered = 1;
                length_tags += strlen (tags[i]) + 1;
            }

            /* check if we must remove tag "irc_smart_filter" in line */
            remove_smart_filter = 0;
            if (nick_changed && irc_nick1 && irc_nick2
                && (strcmp (irc_nick2, nick_to_search) == 0))
            {
                /* update the nick to search if the line is a message "nick" */
                free (nick_to_search);
                nick_to_search = strdup (irc_nick1);
                if (!nick_to_search)
                    break;
                remove_smart_filter = 1;
            }
            else if (nick_found && (join || account || chghost || setname)
                     && smart_filtered)
            {
                remove_smart_filter = 1;
            }

            if (remove_smart_filter)
            {
                /*
                 * unmask a "nick" or "join" message: remove the tag
                 * "irc_smart_filter"
                 */
                new_tags = malloc (length_tags);
                if (new_tags)
                {
                    /* build a string with all tags, except "irc_smart_filter" */
                    new_tags[0] = '\0';
                    for (i = 0; tags[i]; i++)
                    {
                        if (strcmp (tags[i], "irc_smart_filter") != 0)
                        {
                            if (new_tags[0])
                                strcat (new_tags, ",");
                            strcat (new_tags, tags[i]);
                        }
                    }
                    hashtable = weechat_hashtable_new (4,
                                                       WEECHAT_HASHTABLE_STRING,
                                                       WEECHAT_HASHTABLE_STRING,
                                                       NULL, NULL);
                    if (hashtable)
                    {
                        /* update tags in line (remove tag "irc_smart_filter") */
                        weechat_hashtable_set (hashtable,
                                               "tags_array", new_tags);
                        weechat_hdata_update (hdata_line_data, line_data,
                                              hashtable);
                        weechat_hashtable_free (hashtable);
                    }
                    free (new_tags);
                }

                /*
                 * exit loop if the message was the join (if it's a nick change,
                 * then we loop until we find the join)
                 */
                if (join)
                    break;
            }
        }

        /* continue with previous line in buffer */
        line = weechat_hdata_move (hdata_line, line, -1);
    }

    free (nick_to_search);

    weechat_hashtable_remove (channel->join_smart_filtered, nick);
}

/*
 * Rejoins a channel (for example after kick).
 */

void
irc_channel_rejoin (struct t_irc_server *server, struct t_irc_channel *channel,
                    int manual_join, int noswitch)
{
    char *join_string;
    int length;

    if (channel->key)
    {
        length = strlen (channel->name) + 1 + strlen (channel->key) + 1;
        join_string = malloc (length);
        if (join_string)
        {
            snprintf (join_string, length, "%s %s",
                      channel->name,
                      channel->key);
            irc_command_join_server (server, join_string,
                                     manual_join, noswitch);
            free (join_string);
        }
        else
        {
            irc_command_join_server (server, channel->name,
                                     manual_join, noswitch);
        }
    }
    else
    {
        irc_command_join_server (server, channel->name,
                                 manual_join, noswitch);
    }
}

/*
 * Callback for autorejoin on a channel.
 */

int
irc_channel_autorejoin_cb (const void *pointer, void *data,
                           int remaining_calls)
{
    struct t_irc_server *ptr_server, *ptr_server_found;
    struct t_irc_channel *ptr_channel_arg, *ptr_channel;

    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    ptr_channel_arg = (struct t_irc_channel *)pointer;

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
        irc_channel_rejoin (ptr_server_found, ptr_channel_arg, 0, 1);
        ptr_channel_arg->hook_autorejoin = NULL;
    }

    return WEECHAT_RC_OK;
}

/*
 * Displays a message in pv buffer if nick is back and if private has flag
 * "has_quit_server".
 */

void
irc_channel_display_nick_back_in_pv (struct t_irc_server *server,
                                     struct t_irc_nick *nick,
                                     const char *nickname)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_protocol_ctxt ctxt;

    if (!server || (!nick && !nickname))
        return;

    memset (&ctxt, 0, sizeof (ctxt));
    ctxt.server = server;
    ctxt.nick = (nick) ? nick->name : NULL;
    ctxt.nick_is_me = (irc_server_strcasecmp (server, ctxt.nick, server->nick) == 0);
    ctxt.address = (nick) ? nick->host : NULL;
    ctxt.command = strdup ("nick_back");

    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if ((ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
            && ptr_channel->has_quit_server
            && (irc_server_strcasecmp (server, ptr_channel->name, (nick) ? nick->name : nickname) == 0))
        {
            if (weechat_config_boolean (irc_config_look_display_pv_back))
            {
                weechat_printf_date_tags (
                    ptr_channel->buffer,
                    0,
                    irc_protocol_tags (&ctxt, NULL),
                    _("%s%s%s %s(%s%s%s)%s is back on server"),
                    weechat_prefix ("join"),
                    irc_nick_color_for_msg (server, 1, nick, nickname),
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

    free (ctxt.command);
}

/*
 * Sets state for modelists in a channel.
 */

void
irc_channel_modelist_set_state (struct t_irc_channel *channel, int state)
{
    struct t_irc_modelist *ptr_modelist;

    for (ptr_modelist = channel->modelists; ptr_modelist;
         ptr_modelist = ptr_modelist->next_modelist)
    {
        ptr_modelist->state = state;
    }
}

/*
 * Frees a channel and remove it from channels list.
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

    /* free linked lists */
    irc_nick_free_all (server, channel);
    irc_modelist_free_all (channel);

    /* free channel data */
    free (channel->name);
    free (channel->topic);
    free (channel->modes);
    free (channel->key);
    weechat_hashtable_free (channel->join_msg_received);
    free (channel->away_message);
    free (channel->pv_remote_nick_color);
    weechat_unhook (channel->hook_autorejoin);
    weechat_list_free (channel->nicks_speaking[0]);
    weechat_list_free (channel->nicks_speaking[1]);
    irc_channel_nick_speaking_time_free_all (channel);
    weechat_hashtable_free (channel->join_smart_filtered);
    free (channel->buffer_as_string);

    free (channel);

    server->channels = new_channels;
}

/*
 * Frees all channels for a server.
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
 * Returns hdata for channel.
 */

struct t_hdata *
irc_channel_hdata_channel_cb (const void *pointer, void *data,
                              const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = weechat_hdata_new (hdata_name, "prev_channel", "next_channel",
                               0, 0, NULL, NULL);
    if (hdata)
    {
        WEECHAT_HDATA_VAR(struct t_irc_channel, type, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, name, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, topic, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, modes, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, limit, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, key, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, join_msg_received, HASHTABLE, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, checking_whox, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, away_message, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, has_quit_server, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, cycle, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, part, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, nick_completion_reset, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, pv_remote_nick_color, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, hook_autorejoin, POINTER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, nicks_count, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, nicks, POINTER, 0, NULL, "irc_nick");
        WEECHAT_HDATA_VAR(struct t_irc_channel, last_nick, POINTER, 0, NULL, "irc_nick");
        WEECHAT_HDATA_VAR(struct t_irc_channel, nicks_speaking, POINTER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, nicks_speaking_time, POINTER, 0, NULL, "irc_channel_speaking");
        WEECHAT_HDATA_VAR(struct t_irc_channel, last_nick_speaking_time, POINTER, 0, NULL, "irc_channel_speaking");
        WEECHAT_HDATA_VAR(struct t_irc_channel, modelists, POINTER, 0, NULL, "irc_modelist");
        WEECHAT_HDATA_VAR(struct t_irc_channel, last_modelist, POINTER, 0, NULL, "irc_modelist");
        WEECHAT_HDATA_VAR(struct t_irc_channel, join_smart_filtered, HASHTABLE, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, typing_state, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, typing_status_sent, TIME, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, buffer, POINTER, 0, NULL, "buffer");
        WEECHAT_HDATA_VAR(struct t_irc_channel, buffer_as_string, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel, prev_channel, POINTER, 0, NULL, hdata_name);
        WEECHAT_HDATA_VAR(struct t_irc_channel, next_channel, POINTER, 0, NULL, hdata_name);
    }
    return hdata;
}

/*
 * Returns hdata for channel_speaking.
 */

struct t_hdata *
irc_channel_hdata_channel_speaking_cb (const void *pointer, void *data,
                                       const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = weechat_hdata_new (hdata_name, "prev_nick", "next_nick",
                               0, 0, NULL, NULL);
    if (hdata)
    {
        WEECHAT_HDATA_VAR(struct t_irc_channel_speaking, nick, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel_speaking, time_last_message, TIME, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_channel_speaking, prev_nick, POINTER, 0, NULL, hdata_name);
        WEECHAT_HDATA_VAR(struct t_irc_channel_speaking, next_nick, POINTER, 0, NULL, hdata_name);
    }
    return hdata;
}

/*
 * Adds a channel in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
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
    if (!weechat_infolist_new_var_string (ptr_item, "join_msg_received",
                                          weechat_hashtable_get_string (channel->join_msg_received, "keys")))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "checking_whox", channel->checking_whox))
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
    if (!weechat_infolist_new_var_string (ptr_item, "join_smart_filtered",
                                          weechat_hashtable_get_string (channel->join_smart_filtered,
                                                                        "keys_values")))
        return 0;

    return 1;
}

/*
 * Prints channel infos in WeeChat log file (usually for crash dump).
 */

void
irc_channel_print_log (struct t_irc_channel *channel)
{
    struct t_weelist_item *ptr_item;
    struct t_irc_channel_speaking *ptr_nick_speaking;
    int i, index;
    struct t_irc_nick *ptr_nick;
    struct t_irc_modelist *ptr_modelist;

    weechat_log_printf ("");
    weechat_log_printf ("  => channel %s (addr:%p):", channel->name, channel);
    weechat_log_printf ("       type . . . . . . . . . . : %d", channel->type);
    weechat_log_printf ("       topic. . . . . . . . . . : '%s'", channel->topic);
    weechat_log_printf ("       modes. . . . . . . . . . : '%s'", channel->modes);
    weechat_log_printf ("       limit. . . . . . . . . . : %d", channel->limit);
    weechat_log_printf ("       key. . . . . . . . . . . : '%s'", channel->key);
    weechat_log_printf ("       join_msg_received. . . . : %p (hashtable: '%s')",
                        channel->join_msg_received,
                        weechat_hashtable_get_string (channel->join_msg_received,
                                                      "keys_values"));
    weechat_log_printf ("       checking_whox. . . . . . : %d", channel->checking_whox);
    weechat_log_printf ("       away_message . . . . . . : '%s'", channel->away_message);
    weechat_log_printf ("       has_quit_server. . . . . : %d", channel->has_quit_server);
    weechat_log_printf ("       cycle. . . . . . . . . . : %d", channel->cycle);
    weechat_log_printf ("       part . . . . . . . . . . : %d", channel->part);
    weechat_log_printf ("       nick_completion_reset. . : %d", channel->nick_completion_reset);
    weechat_log_printf ("       pv_remote_nick_color . . : '%s'", channel->pv_remote_nick_color);
    weechat_log_printf ("       hook_autorejoin. . . . . : %p", channel->hook_autorejoin);
    weechat_log_printf ("       nicks_count. . . . . . . : %d", channel->nicks_count);
    weechat_log_printf ("       nicks. . . . . . . . . . : %p", channel->nicks);
    weechat_log_printf ("       last_nick. . . . . . . . : %p", channel->last_nick);
    weechat_log_printf ("       nicks_speaking[0]. . . . : %p", channel->nicks_speaking[0]);
    weechat_log_printf ("       nicks_speaking[1]. . . . : %p", channel->nicks_speaking[1]);
    weechat_log_printf ("       nicks_speaking_time. . . : %p", channel->nicks_speaking_time);
    weechat_log_printf ("       last_nick_speaking_time. : %p", channel->last_nick_speaking_time);
    weechat_log_printf ("       modelists. . . . . . . . : %p", channel->modelists);
    weechat_log_printf ("       last_modelist. . . . . . : %p", channel->last_modelist);
    weechat_log_printf ("       join_smart_filtered. . . : %p (hashtable: '%s')",
                        channel->join_smart_filtered,
                        weechat_hashtable_get_string (channel->join_smart_filtered,
                                                      "keys_values"));
    weechat_log_printf ("       typing_state . . . . . . : %d", channel->typing_state);
    weechat_log_printf ("       typing_status_sent . . . : %lld", (long long)channel->typing_status_sent);
    weechat_log_printf ("       buffer . . . . . . . . . : %p", channel->buffer);
    weechat_log_printf ("       buffer_as_string . . . . : '%s'", channel->buffer_as_string);
    weechat_log_printf ("       prev_channel . . . . . . : %p", channel->prev_channel);
    weechat_log_printf ("       next_channel . . . . . . : %p", channel->next_channel);
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
            weechat_log_printf ("         nick speaking time: '%s', time: %lld",
                                ptr_nick_speaking->nick,
                                (long long)ptr_nick_speaking->time_last_message);
        }
    }
    for (ptr_nick = channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
        irc_nick_print_log (ptr_nick);
    }
    for (ptr_modelist = channel->modelists; ptr_modelist;
         ptr_modelist = ptr_modelist->next_modelist)
    {
        irc_modelist_print_log (ptr_modelist);
    }
}
