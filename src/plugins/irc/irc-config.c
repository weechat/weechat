/*
 * irc-config.c - IRC configuration options (file irc.conf)
 *
 * Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
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
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include <pwd.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-config.h"
#include "irc-buffer.h"
#include "irc-channel.h"
#include "irc-ctcp.h"
#include "irc-ignore.h"
#include "irc-msgbuffer.h"
#include "irc-nick.h"
#include "irc-notify.h"
#include "irc-server.h"


struct t_config_file *irc_config_file = NULL;
struct t_config_section *irc_config_section_msgbuffer = NULL;
struct t_config_section *irc_config_section_ctcp = NULL;
struct t_config_section *irc_config_section_server_default = NULL;
struct t_config_section *irc_config_section_server = NULL;

int irc_config_loading = 0;

/* IRC config, look section */

struct t_config_option *irc_config_look_buffer_open_before_autojoin;
struct t_config_option *irc_config_look_buffer_open_before_join;
struct t_config_option *irc_config_look_buffer_switch_autojoin;
struct t_config_option *irc_config_look_buffer_switch_join;
struct t_config_option *irc_config_look_color_nicks_in_names;
struct t_config_option *irc_config_look_color_nicks_in_nicklist;
struct t_config_option *irc_config_look_color_nicks_in_server_messages;
struct t_config_option *irc_config_look_color_pv_nick_like_channel;
struct t_config_option *irc_config_look_ctcp_time_format;
struct t_config_option *irc_config_look_display_away;
struct t_config_option *irc_config_look_display_ctcp_blocked;
struct t_config_option *irc_config_look_display_ctcp_reply;
struct t_config_option *irc_config_look_display_ctcp_unknown;
struct t_config_option *irc_config_look_display_host_join;
struct t_config_option *irc_config_look_display_host_join_local;
struct t_config_option *irc_config_look_display_host_quit;
struct t_config_option *irc_config_look_display_join_message;
struct t_config_option *irc_config_look_display_old_topic;
struct t_config_option *irc_config_look_display_pv_away_once;
struct t_config_option *irc_config_look_display_pv_back;
struct t_config_option *irc_config_look_highlight_channel;
struct t_config_option *irc_config_look_highlight_pv;
struct t_config_option *irc_config_look_highlight_server;
struct t_config_option *irc_config_look_highlight_tags_restrict;
struct t_config_option *irc_config_look_item_away_message;
struct t_config_option *irc_config_look_item_channel_modes_hide_args;
struct t_config_option *irc_config_look_item_display_server;
struct t_config_option *irc_config_look_item_nick_modes;
struct t_config_option *irc_config_look_item_nick_prefix;
struct t_config_option *irc_config_look_join_auto_add_chantype;
struct t_config_option *irc_config_look_msgbuffer_fallback;
struct t_config_option *irc_config_look_new_channel_position;
struct t_config_option *irc_config_look_new_pv_position;
struct t_config_option *irc_config_look_nick_color_force;
struct t_config_option *irc_config_look_nick_color_hash;
struct t_config_option *irc_config_look_nick_color_stop_chars;
struct t_config_option *irc_config_look_nick_completion_smart;
struct t_config_option *irc_config_look_nick_mode;
struct t_config_option *irc_config_look_nick_mode_empty;
struct t_config_option *irc_config_look_nicks_hide_password;
struct t_config_option *irc_config_look_notice_as_pv;
struct t_config_option *irc_config_look_notice_welcome_redirect;
struct t_config_option *irc_config_look_notice_welcome_tags;
struct t_config_option *irc_config_look_notify_tags_ison;
struct t_config_option *irc_config_look_notify_tags_whois;
struct t_config_option *irc_config_look_part_closes_buffer;
struct t_config_option *irc_config_look_pv_buffer;
struct t_config_option *irc_config_look_pv_tags;
struct t_config_option *irc_config_look_raw_messages;
struct t_config_option *irc_config_look_server_buffer;
struct t_config_option *irc_config_look_smart_filter;
struct t_config_option *irc_config_look_smart_filter_delay;
struct t_config_option *irc_config_look_smart_filter_join;
struct t_config_option *irc_config_look_smart_filter_join_unmask;
struct t_config_option *irc_config_look_smart_filter_mode;
struct t_config_option *irc_config_look_smart_filter_nick;
struct t_config_option *irc_config_look_smart_filter_quit;
struct t_config_option *irc_config_look_temporary_servers;
struct t_config_option *irc_config_look_topic_strip_colors;

/* IRC config, color section */

struct t_config_option *irc_config_color_input_nick;
struct t_config_option *irc_config_color_item_away;
struct t_config_option *irc_config_color_item_channel_modes;
struct t_config_option *irc_config_color_item_lag_counting;
struct t_config_option *irc_config_color_item_lag_finished;
struct t_config_option *irc_config_color_item_nick_modes;
struct t_config_option *irc_config_color_message_join;
struct t_config_option *irc_config_color_message_quit;
struct t_config_option *irc_config_color_mirc_remap;
struct t_config_option *irc_config_color_nick_prefixes;
struct t_config_option *irc_config_color_notice;
struct t_config_option *irc_config_color_reason_quit;
struct t_config_option *irc_config_color_topic_current;
struct t_config_option *irc_config_color_topic_new;
struct t_config_option *irc_config_color_topic_old;

/* IRC config, network section */

struct t_config_option *irc_config_network_autoreconnect_delay_growing;
struct t_config_option *irc_config_network_autoreconnect_delay_max;
struct t_config_option *irc_config_network_ban_mask_default;
struct t_config_option *irc_config_network_colors_receive;
struct t_config_option *irc_config_network_colors_send;
struct t_config_option *irc_config_network_lag_check;
struct t_config_option *irc_config_network_lag_max;
struct t_config_option *irc_config_network_lag_min_show;
struct t_config_option *irc_config_network_lag_reconnect;
struct t_config_option *irc_config_network_lag_refresh_interval;
struct t_config_option *irc_config_network_notify_check_ison;
struct t_config_option *irc_config_network_notify_check_whois;
struct t_config_option *irc_config_network_send_unknown_commands;
struct t_config_option *irc_config_network_whois_double_nick;

/* IRC config, server section */

struct t_config_option *irc_config_server_default[IRC_SERVER_NUM_OPTIONS];

struct t_hook *irc_config_hook_config_nick_colors = NULL;
char **irc_config_nick_colors = NULL;
int irc_config_num_nick_colors = 0;
struct t_hashtable *irc_config_hashtable_display_join_message = NULL;
struct t_hashtable *irc_config_hashtable_nick_color_force = NULL;
struct t_hashtable *irc_config_hashtable_nick_prefixes = NULL;
struct t_hashtable *irc_config_hashtable_color_mirc_remap = NULL;
char **irc_config_nicks_hide_password = NULL;
int irc_config_num_nicks_hide_password = 0;

int irc_config_write_temp_servers = 0;


/*
 * Gets server pointer with name of option.
 */

struct t_irc_server *
irc_config_get_server_from_option_name (const char *name)
{
    struct t_irc_server *ptr_server;
    char *pos_option, *server_name;

    ptr_server = NULL;

    if (name)
    {
        pos_option = strrchr (name, '.');
        if (pos_option)
        {
            server_name = weechat_strndup (name, pos_option - name);
            if (server_name)
            {
                ptr_server = irc_server_search (server_name);
                free (server_name);
            }
        }
    }

    return ptr_server;
}

/*
 * Computes nick colors for all servers and channels.
 */

void
irc_config_compute_nick_colors ()
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            for (ptr_nick = ptr_channel->nicks; ptr_nick;
                 ptr_nick = ptr_nick->next_nick)
            {
                if (irc_server_strcasecmp (ptr_server, ptr_nick->name,
                                           ptr_server->nick) != 0)
                {
                    if (ptr_nick->color)
                        free (ptr_nick->color);
                    ptr_nick->color = strdup (irc_nick_find_color (ptr_nick->name));
                }
            }
            if (ptr_channel->pv_remote_nick_color)
            {
                free (ptr_channel->pv_remote_nick_color);
                ptr_channel->pv_remote_nick_color = NULL;
            }
        }
    }

    /* if colors are displayed for nicks in nicklist, refresh them */
    if (weechat_config_boolean(irc_config_look_color_nicks_in_nicklist))
        irc_nick_nicklist_set_color_all ();
}

/*
 * Sets nick colors using option "weechat.color.chat_nick_colors".
 */

void
irc_config_set_nick_colors ()
{
    if (irc_config_nick_colors)
    {
        weechat_string_free_split (irc_config_nick_colors);
        irc_config_nick_colors = NULL;
        irc_config_num_nick_colors = 0;
    }

    irc_config_nick_colors =
        weechat_string_split (
            weechat_config_string (
                weechat_config_get ("weechat.color.chat_nick_colors")),
            ",", 0, 0,
            &irc_config_num_nick_colors);
}

/*
 * Checks if channel modes arguments must be displayed or hidden
 * (according to option irc.look.item_channel_modes_hide_args).
 *
 * Returns:
 *   1: channel modes arguments must be displayed
 *   0: channel modes arguments must be hidden
 */

int
irc_config_display_channel_modes_arguments (const char *modes)
{
    char *pos_space, *pos;
    const char *ptr_mode;

    pos_space = strchr (modes, ' ');
    if (!pos_space)
        return 1;

    ptr_mode = weechat_config_string (irc_config_look_item_channel_modes_hide_args);
    if (!ptr_mode)
        return 1;

    /* "*" means hide all arguments */
    if (strcmp (ptr_mode, "*") == 0)
        return 0;

    while (ptr_mode[0])
    {
        pos = strchr (modes, ptr_mode[0]);
        if (pos && (pos < pos_space))
            return 0;
        ptr_mode++;
    }

    /* arguments are displayed by default */
    return 1;
}

/*
 * Callback for changes on option "weechat.color.chat_nick_colors".
 */

int
irc_config_change_nick_colors_cb (void *data, const char *option,
                                  const char *value)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    (void) value;

    irc_config_set_nick_colors ();
    irc_config_compute_nick_colors ();

    return WEECHAT_RC_OK;
}

/*
 * Callback for changes on option "irc.look.color_nicks_in_nicklist".
 */

void
irc_config_change_look_color_nicks_in_nicklist (void *data,
                                                struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    irc_nick_nicklist_set_color_all ();
}

/*
 * Callback for changes on option "irc.look.display_away".
 */

void
irc_config_change_look_display_away (void *data,
                                     struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (!irc_config_loading
        && (weechat_config_integer (irc_config_look_display_away) == IRC_CONFIG_DISPLAY_AWAY_CHANNEL))
    {
        weechat_printf (
            NULL,
            _("%sWARNING: the value \"channel\" for option "
              "\"irc.look.display_away\" will send all your away changes to "
              "the channels, which is often considered as spam; therefore you "
              "could be banned from channels, you are warned!"),
            weechat_prefix ("error"));
    }
}

/*
 * Callback for changes on option "irc.look.display_join_message".
 */

void
irc_config_change_look_display_join_message (void *data,
                                             struct t_config_option *option)
{
    char **items;
    int num_items, i;

    /* make C compiler happy */
    (void) data;
    (void) option;

    if (!irc_config_hashtable_display_join_message)
    {
        irc_config_hashtable_display_join_message = weechat_hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL,
            NULL);
    }
    else
        weechat_hashtable_remove_all (irc_config_hashtable_display_join_message);

    items = weechat_string_split (
        weechat_config_string (irc_config_look_display_join_message),
        ",", 0, 0, &num_items);
    if (items)
    {
        for (i = 0; i < num_items; i++)
        {
            weechat_hashtable_set (irc_config_hashtable_display_join_message,
                                   items[i], "1");
        }
        weechat_string_free_split (items);
    }
}

/*
 * Callback for changes on option "irc.look.server_buffer".
 */

void
irc_config_change_look_server_buffer (void *data,
                                      struct t_config_option *option)
{
    struct t_irc_server *ptr_server;
    struct t_gui_buffer *ptr_buffer;

    /* make C compiler happy */
    (void) data;
    (void) option;

    /* first unmerge all IRC server buffers */
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->buffer)
            weechat_buffer_unmerge (ptr_server->buffer, -1);
    }

    /* merge IRC server buffers with core buffer or another buffer */
    if ((weechat_config_integer (irc_config_look_server_buffer) ==
         IRC_CONFIG_LOOK_SERVER_BUFFER_MERGE_WITH_CORE)
        || (weechat_config_integer (irc_config_look_server_buffer) ==
            IRC_CONFIG_LOOK_SERVER_BUFFER_MERGE_WITHOUT_CORE))
    {
        ptr_buffer =
            (weechat_config_integer (irc_config_look_server_buffer) ==
             IRC_CONFIG_LOOK_SERVER_BUFFER_MERGE_WITH_CORE) ?
            weechat_buffer_search_main () : irc_buffer_search_server_lowest_number ();

        if (ptr_buffer)
        {
            for (ptr_server = irc_servers; ptr_server;
                 ptr_server = ptr_server->next_server)
            {
                if (ptr_server->buffer && (ptr_server->buffer != ptr_buffer))
                    weechat_buffer_merge (ptr_server->buffer, ptr_buffer);
            }
        }
    }
}

/*
 * Callback for changes on option "irc.look.pv_buffer".
 */

void
irc_config_change_look_pv_buffer (void *data,
                                  struct t_config_option *option)
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;

    /* make C compiler happy */
    (void) data;
    (void) option;

    /* first unmerge all IRC private buffers */
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if ((ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
                && ptr_channel->buffer)
            {
                weechat_buffer_unmerge (ptr_channel->buffer, -1);
            }
        }
    }

    /* merge IRC private buffers */
    if ((weechat_config_integer (irc_config_look_pv_buffer) == IRC_CONFIG_LOOK_PV_BUFFER_MERGE_BY_SERVER)
        || (weechat_config_integer (irc_config_look_pv_buffer) == IRC_CONFIG_LOOK_PV_BUFFER_MERGE_ALL))
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            for (ptr_channel = ptr_server->channels; ptr_channel;
                 ptr_channel = ptr_channel->next_channel)
            {
                if ((ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
                    && ptr_channel->buffer)
                {
                    ptr_buffer = NULL;
                    switch (weechat_config_integer (irc_config_look_pv_buffer))
                    {
                        case IRC_CONFIG_LOOK_PV_BUFFER_MERGE_BY_SERVER:
                            /* merge private buffers by server */
                            ptr_buffer = irc_buffer_search_private_lowest_number (ptr_server);
                            break;
                        case IRC_CONFIG_LOOK_PV_BUFFER_MERGE_ALL:
                            /* merge *ALL* private buffers */
                            ptr_buffer = irc_buffer_search_private_lowest_number (NULL);
                            break;
                    }
                    if (ptr_buffer && (ptr_channel->buffer != ptr_buffer))
                        weechat_buffer_merge (ptr_channel->buffer, ptr_buffer);
                }
            }
        }
    }
}

/*
 * Callback for changes on option "irc.look.item_away_message".
 */

void
irc_config_change_look_item_away_message (void *data,
                                          struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    weechat_bar_item_update ("away");
}

/*
 * Callback for changes on option "irc.look.item_channel_modes_hide_args".
 */

void
irc_config_change_look_item_channel_modes_hide_args (void *data,
                                                     struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    weechat_bar_item_update ("buffer_modes");
}

/*
 * Callback for changes on option "irc.look.highlight_tags_restrict".
 */

void
irc_config_change_look_highlight_tags_restrict (void *data,
                                                struct t_config_option *option)
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;

    /* make C compiler happy */
    (void) data;
    (void) option;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->buffer)
        {
            weechat_buffer_set (
                ptr_server->buffer, "highlight_tags_restrict",
                weechat_config_string (irc_config_look_highlight_tags_restrict));
        }
        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if (ptr_channel->buffer)
            {
                weechat_buffer_set (
                    ptr_channel->buffer, "highlight_tags_restrict",
                    weechat_config_string (irc_config_look_highlight_tags_restrict));
            }
        }
    }
}

/*
 * Callback for changes on option "irc.look.nick_color_force".
 */

void
irc_config_change_look_nick_color_force (void *data,
                                         struct t_config_option *option)
{
    char **items, *pos;
    int num_items, i;

    /* make C compiler happy */
    (void) data;
    (void) option;

    if (!irc_config_hashtable_nick_color_force)
    {
        irc_config_hashtable_nick_color_force = weechat_hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL,
            NULL);
    }
    else
        weechat_hashtable_remove_all (irc_config_hashtable_nick_color_force);

    items = weechat_string_split (
        weechat_config_string (irc_config_look_nick_color_force),
        ";", 0, 0, &num_items);
    if (items)
    {
        for (i = 0; i < num_items; i++)
        {
            pos = strchr (items[i], ':');
            if (pos)
            {
                pos[0] = '\0';
                weechat_hashtable_set (irc_config_hashtable_nick_color_force,
                                       items[i],
                                       pos + 1);
            }
        }
        weechat_string_free_split (items);
    }

    irc_config_compute_nick_colors ();
}

/*
 * Callback for changes on options that change nick colors.
 */

void
irc_config_change_look_nick_colors (void *data,
                                    struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    irc_config_compute_nick_colors ();
}

/*
 * Callback for changes on option "irc.look.item_display_server".
 */

void
irc_config_change_look_item_display_server (void *data,
                                            struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    weechat_bar_item_update ("buffer_plugin");
    weechat_bar_item_update ("buffer_name");
    weechat_bar_item_update ("buffer_short_name");
}

/*
 * Callback for changes on option "irc.look.nicks_hide_password".
 */

void
irc_config_change_look_nicks_hide_password (void *data,
                                            struct t_config_option *option)
{
    const char *nicks_hide_password;

    /* make C compiler happy */
    (void) data;
    (void) option;

    if (irc_config_nicks_hide_password)
    {
        weechat_string_free_split (irc_config_nicks_hide_password);
        irc_config_nicks_hide_password = NULL;
    }
    irc_config_num_nicks_hide_password = 0;

    nicks_hide_password = weechat_config_string (irc_config_look_nicks_hide_password);
    if (nicks_hide_password && nicks_hide_password[0])
    {
        irc_config_nicks_hide_password = weechat_string_split (
            nicks_hide_password, ",", 0, 0,
            &irc_config_num_nicks_hide_password);
    }
}

/*
 * Callback for changes on option "irc.look.topic_strip_colors".
 */

void
irc_config_change_look_topic_strip_colors (void *data,
                                           struct t_config_option *option)
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;

    /* make C compiler happy */
    (void) data;
    (void) option;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if (ptr_channel->buffer)
                irc_channel_set_buffer_title (ptr_channel);
        }
    }
}

/*
 * Callback for changes on an option affecting bar item "input_prompt".
 */

void
irc_config_change_bar_item_input_prompt (void *data,
                                         struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    weechat_bar_item_update ("input_prompt");
}

/*
 * Callback for changes on option "irc.color.item_away".
 */

void
irc_config_change_color_item_away (void *data,
                                   struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    weechat_bar_item_update ("away");
}

/*
 * Callback for changes on option "irc.color.item_channel_modes".
 */

void
irc_config_change_color_item_channel_modes (void *data,
                                            struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    weechat_bar_item_update ("buffer_modes");
}

/*
 * Callback for changes on options "irc.color.item_lag_counting" and
 * "irc.color.item_lag_finished".
 */

void
irc_config_change_color_item_lag (void *data,
                                  struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    weechat_bar_item_update ("lag");
}

/*
 * Callback for changes on option "irc.color.item_nick_modes".
 */

void
irc_config_change_color_item_nick_modes (void *data,
                                         struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    weechat_bar_item_update ("input_prompt");
    weechat_bar_item_update ("irc_nick_modes");
}

/*
 * Callback for changes on option "irc.color.mirc_remap".
 */

void
irc_config_change_color_mirc_remap (void *data, struct t_config_option *option)
{
    char **items, *pos;
    int num_items, i;

    /* make C compiler happy */
    (void) data;
    (void) option;

    if (!irc_config_hashtable_color_mirc_remap)
    {
        irc_config_hashtable_color_mirc_remap = weechat_hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL,
            NULL);
    }
    else
        weechat_hashtable_remove_all (irc_config_hashtable_color_mirc_remap);

    items = weechat_string_split (
        weechat_config_string (irc_config_color_mirc_remap),
        ";", 0, 0, &num_items);
    if (items)
    {
        for (i = 0; i < num_items; i++)
        {
            pos = strchr (items[i], ':');
            if (pos)
            {
                pos[0] = '\0';
                weechat_hashtable_set (irc_config_hashtable_color_mirc_remap,
                                       items[i],
                                       pos + 1);
            }
        }
        weechat_string_free_split (items);
    }
}

/*
 * Callback for changes on option "irc.color.nick_prefixes".
 */

void
irc_config_change_color_nick_prefixes (void *data,
                                       struct t_config_option *option)
{
    char **items, *pos;
    int num_items, i;

    /* make C compiler happy */
    (void) data;
    (void) option;

    if (!irc_config_hashtable_nick_prefixes)
    {
        irc_config_hashtable_nick_prefixes = weechat_hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL,
            NULL);
    }
    else
        weechat_hashtable_remove_all (irc_config_hashtable_nick_prefixes);

    items = weechat_string_split (
        weechat_config_string (irc_config_color_nick_prefixes),
        ";", 0, 0, &num_items);
    if (items)
    {
        for (i = 0; i < num_items; i++)
        {
            pos = strchr (items[i], ':');
            if (pos)
            {
                pos[0] = '\0';
                weechat_hashtable_set (irc_config_hashtable_nick_prefixes,
                                       items[i],
                                       pos + 1);
            }
        }
        weechat_string_free_split (items);
    }

    irc_nick_nicklist_set_prefix_color_all ();

    weechat_bar_item_update ("input_prompt");
}

/*
 * Callback for changes on option "irc.network.lag_check".
 */

void
irc_config_change_network_lag_check (void *data,
                                     struct t_config_option *option)
{
    time_t time_next_check;
    struct t_irc_server *ptr_server;

    /* make C compiler happy */
    (void) data;
    (void) option;

    time_next_check = (weechat_config_integer (irc_config_network_lag_check) > 0) ?
        time (NULL) : 0;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->is_connected)
            ptr_server->lag_next_check = time_next_check;
    }
}

/*
 * Callback for changes on option "irc.network.lag_min_show".
 */

void
irc_config_change_network_lag_min_show (void *data,
                                        struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    weechat_bar_item_update ("lag");
}

/*
 * Callback for changes on option "irc.network.notify_check_ison".
 */

void
irc_config_change_network_notify_check_ison (void *data,
                                             struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    irc_notify_hook_timer_ison ();
}

/*
 * Callback for changes on option "irc.network.notify_check_whois".
 */

void
irc_config_change_network_notify_check_whois (void *data,
                                             struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    irc_notify_hook_timer_whois ();
}

/*
 * Callback for changes on option "irc.network.send_unknown_commands".
 */

void
irc_config_change_network_send_unknown_commands (void *data,
                                                 struct t_config_option *option)
{
    char value[2];
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;

    /* make C compiler happy */
    (void) data;
    (void) option;

    strcpy (value,
            (weechat_config_boolean (irc_config_network_send_unknown_commands)) ?
            "1" : "0");

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->buffer)
        {
            weechat_buffer_set (ptr_server->buffer,
                                "input_get_unknown_commands", value);
        }
    }

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if (ptr_channel->buffer)
            {
                weechat_buffer_set (ptr_channel->buffer,
                                    "input_get_unknown_commands", value);
            }
        }
    }
}

/*
 * Callback called when a default server option is modified.
 */

void
irc_config_server_default_change_cb (void *data, struct t_config_option *option)
{
    int index_option;
    struct t_irc_server *ptr_server;

    index_option = irc_server_search_option (data);
    if (index_option >= 0)
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            /*
             * when default value for a server option is changed, we apply it
             * on all servers where value is "null" (inherited from default
             * value)
             */
            if (weechat_config_option_is_null (ptr_server->options[index_option]))
            {
                switch (index_option)
                {
                    case IRC_SERVER_OPTION_ADDRESSES:
                        irc_server_set_addresses (ptr_server,
                                                  weechat_config_string (option));
                        break;
                    case IRC_SERVER_OPTION_NICKS:
                        irc_server_set_nicks (ptr_server,
                                              weechat_config_string (option));
                        break;
                    case IRC_SERVER_OPTION_AWAY_CHECK:
                    case IRC_SERVER_OPTION_AWAY_CHECK_MAX_NICKS:
                        if (IRC_SERVER_OPTION_INTEGER(ptr_server, IRC_SERVER_OPTION_AWAY_CHECK) > 0)
                            irc_server_check_away (ptr_server);
                        else
                            irc_server_remove_away (ptr_server);
                        break;
                }
            }
        }
    }
}

/*
 * Checks string with GnuTLS priorities.
 *
 * Returns NULL if OK, or pointer to char with error in string.
 */

const char *
irc_config_check_gnutls_priorities (const char *priorities)
{
#ifdef HAVE_GNUTLS
    gnutls_priority_t priority_cache;
    const char *pos_error;
    int rc;

    if (!priorities || !priorities[0])
        return NULL;

    rc = gnutls_priority_init (&priority_cache, priorities, &pos_error);
    if (rc == GNUTLS_E_SUCCESS)
    {
        gnutls_priority_deinit (priority_cache);
        return NULL;
    }
    if (pos_error)
        return pos_error;
    return priorities;
#else
    /* make C compiler happy */
    (void) priorities;

    return NULL;
#endif /* HAVE_GNUTLS */
}

/*
 * Callback called to check a server option when it is modified.
 */

int
irc_config_server_check_value_cb (void *data,
                                  struct t_config_option *option,
                                  const char *value)
{
    int index_option, proxy_found;
    const char *pos_error, *proxy_name;
    struct t_infolist *infolist;
#ifdef HAVE_GNUTLS
    char **fingerprints, *str_sizes;
    int i, j, rc, algo, length;
#endif /* HAVE_GNUTLS */

    /* make C compiler happy */
    (void) option;

    index_option = irc_server_search_option (data);
    if (index_option >= 0)
    {
        switch (index_option)
        {
            case IRC_SERVER_OPTION_PROXY:
                if (value && value[0])
                {
                    proxy_found = 0;
                    infolist = weechat_infolist_get ("proxy", NULL, value);
                    if (infolist)
                    {
                        while (weechat_infolist_next (infolist))
                        {
                            proxy_name = weechat_infolist_string (infolist,
                                                                  "name");
                            if (proxy_name && (strcmp (value, proxy_name) == 0))
                            {
                                proxy_found = 1;
                                break;
                            }
                        }
                        weechat_infolist_free (infolist);
                    }
                    if (!proxy_found)
                    {
                        weechat_printf (
                            NULL,
                            _("%s%s: warning: proxy \"%s\" does not exist "
                              "(you can add it with command /proxy)"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME, value);
                    }
                }
                break;
            case IRC_SERVER_OPTION_SSL_PRIORITIES:
                pos_error = irc_config_check_gnutls_priorities (value);
                if (pos_error)
                {
                    weechat_printf (
                        NULL,
                        _("%s%s: invalid priorities string, error at this "
                          "position in string: \"%s\""),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME, pos_error);
                    return 0;
                }
                break;
            case IRC_SERVER_OPTION_SSL_FINGERPRINT:
#ifdef HAVE_GNUTLS
                if (value && value[0])
                {
                    fingerprints = weechat_string_split (value, ",", 0, 0,
                                                         NULL);
                    if (fingerprints)
                    {
                        rc = 0;
                        for (i = 0; fingerprints[i]; i++)
                        {
                            length = strlen (fingerprints[i]);
                            algo = irc_server_fingerprint_search_algo_with_size (
                                length * 4);
                            if (algo < 0)
                            {
                                rc = -1;
                                break;
                            }
                            for (j = 0; j < length; j++)
                            {
                                if (!isxdigit ((unsigned char)fingerprints[i][j]))
                                {
                                    rc = -2;
                                    break;
                                }
                            }
                            if (rc < 0)
                                break;
                        }
                        weechat_string_free_split (fingerprints);
                        switch (rc)
                        {
                            case -1:  /* invalid size */
                                str_sizes = irc_server_fingerprint_str_sizes ();
                                weechat_printf (
                                    NULL,
                                    _("%s%s: invalid fingerprint size, the "
                                      "number of hexadecimal digits must be "
                                      "one of: %s"),
                                    weechat_prefix ("error"),
                                    IRC_PLUGIN_NAME,
                                    (str_sizes) ? str_sizes : "?");
                                if (str_sizes)
                                    free (str_sizes);
                                return 0;
                            case -2:  /* invalid content */
                                weechat_printf (
                                    NULL,
                                    _("%s%s: invalid fingerprint, it must "
                                      "contain only hexadecimal digits (0-9, "
                                      "a-f)"),
                                    weechat_prefix ("error"), IRC_PLUGIN_NAME);
                                return 0;
                        }
                    }
                }
#endif /* HAVE_GNUTLS */
                break;
        }
    }

    return 1;
}

/*
 * Callback called when a server option is modified.
 */

void
irc_config_server_change_cb (void *data, struct t_config_option *option)
{
    int index_option;
    char *name;
    struct t_irc_server *ptr_server;

    index_option = irc_server_search_option (data);
    if (index_option >= 0)
    {
        name = weechat_config_option_get_pointer (option, "name");
        ptr_server = irc_config_get_server_from_option_name (name);
        if (ptr_server)
        {
            switch (index_option)
            {
                case IRC_SERVER_OPTION_ADDRESSES:
                    irc_server_set_addresses (
                        ptr_server,
                        IRC_SERVER_OPTION_STRING(ptr_server,
                                                 IRC_SERVER_OPTION_ADDRESSES));
                    break;
                case IRC_SERVER_OPTION_NICKS:
                    irc_server_set_nicks (
                        ptr_server,
                        IRC_SERVER_OPTION_STRING(ptr_server,
                                                 IRC_SERVER_OPTION_NICKS));
                    break;
                case IRC_SERVER_OPTION_AWAY_CHECK:
                case IRC_SERVER_OPTION_AWAY_CHECK_MAX_NICKS:
                    if (IRC_SERVER_OPTION_INTEGER(ptr_server, IRC_SERVER_OPTION_AWAY_CHECK) > 0)
                        irc_server_check_away (ptr_server);
                    else
                        irc_server_remove_away (ptr_server);
                    break;
                case IRC_SERVER_OPTION_NOTIFY:
                    irc_notify_new_for_server (ptr_server);
                    break;
            }
        }
    }
}

/*
 * Callback called when "notify" option from "server_default" section is
 * changed.
 *
 * This function is used to reject any value in the option (this option is not
 * used, only values in servers are used for notify).
 *
 * Returns:
 *   1: value is not set
 *   0: value is set
 */

int
irc_config_server_default_check_notify (void *data,
                                        struct t_config_option *option,
                                        const char *value)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (value && value[0])
        return 0;

    return 1;
}

/*
 * Reloads IRC configuration file.
 */

int
irc_config_reload (void *data, struct t_config_file *config_file)
{
    int rc;
    struct t_irc_server *ptr_server, *next_server;

    /* make C compiler happy */
    (void) data;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        ptr_server->reloading_from_config = 1;
        ptr_server->reloaded_from_config = 0;
    }

    irc_ignore_free_all ();

    irc_config_loading = 1;
    rc = weechat_config_reload (config_file);
    irc_config_loading = 0;

    ptr_server = irc_servers;
    while (ptr_server)
    {
        next_server = ptr_server->next_server;

        /*
         * if server existed before reload, but was not read in irc.conf:
         * - if connected to server: display a warning, keep server in memory
         * - if not connected: delete server
         */
        if (ptr_server->reloading_from_config
            && !ptr_server->reloaded_from_config)
        {
            if (ptr_server->is_connected)
            {
                weechat_printf (
                    NULL,
                    _("%s%s: warning: server \"%s\" not found in "
                      "configuration file, not deleted in memory because it's "
                      "currently used"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME,
                    ptr_server->name);
            }
            else
                irc_server_free (ptr_server);
        }

        ptr_server = next_server;
    }

    return rc;
}

/*
 * Sets a message target buffer.
 */

int
irc_config_msgbuffer_create_option (void *data,
                                    struct t_config_file *config_file,
                                    struct t_config_section *section,
                                    const char *option_name, const char *value)
{
    struct t_config_option *ptr_option;
    int rc;

    /* make C compiler happy */
    (void) data;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    if (option_name)
    {
        ptr_option = weechat_config_search_option (config_file, section,
                                                   option_name);
        if (ptr_option)
        {
            if (value)
                rc = weechat_config_option_set (ptr_option, value, 1);
            else
            {
                weechat_config_option_free (ptr_option);
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            }
        }
        else
        {
            if (value)
            {
                ptr_option = weechat_config_new_option (
                    config_file, section,
                    option_name, "integer",
                    _("buffer used to display message received from IRC "
                      "server"),
                    "weechat|server|current|private", 0, 0, value, value, 0,
                    NULL, NULL, NULL, NULL, NULL, NULL);
                rc = (ptr_option) ?
                    WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE :
                    WEECHAT_CONFIG_OPTION_SET_ERROR;
            }
            else
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
    }

    if (rc == WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        weechat_printf (
            NULL,
            _("%s%s: error creating \"%s\" => \"%s\""),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, option_name, value);
    }

    return rc;
}

/*
 * Sets a ctcp reply format.
 */

int
irc_config_ctcp_create_option (void *data, struct t_config_file *config_file,
                               struct t_config_section *section,
                               const char *option_name, const char *value)
{
    struct t_config_option *ptr_option;
    int rc;
    const char *default_value;
    static char empty_value[1] = { '\0' };
    const char *pos_name;

    /* make C compiler happy */
    (void) data;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    if (option_name)
    {
        ptr_option = weechat_config_search_option (config_file, section,
                                                   option_name);
        if (ptr_option)
        {
            if (value)
                rc = weechat_config_option_set (ptr_option, value, 1);
            else
            {
                weechat_config_option_free (ptr_option);
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            }
        }
        else
        {
            if (value)
            {
                pos_name = strchr (option_name, '.');
                pos_name = (pos_name) ? pos_name + 1 : option_name;

                default_value = irc_ctcp_get_default_reply (pos_name);
                if (!default_value)
                    default_value = empty_value;

                ptr_option = weechat_config_new_option (
                    config_file, section,
                    option_name, "string",
                    _("format for CTCP reply or empty string for blocking "
                      "CTCP (no reply), following variables are replaced: "
                      "$version (WeeChat version), "
                      "$compilation (compilation date), "
                      "$osinfo (info about OS), "
                      "$site (WeeChat site), "
                      "$download (WeeChat site, download page), "
                      "$time (current date and time as text), "
                      "$username (username on server), "
                      "$realname (realname on server)"),
                    NULL, 0, 0, default_value, value, 0,
                    NULL, NULL, NULL, NULL, NULL, NULL);
                rc = (ptr_option) ?
                    WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE :
                    WEECHAT_CONFIG_OPTION_SET_ERROR;
            }
            else
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
    }

    if (rc == WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        weechat_printf (
            NULL,
            _("%s%s: error creating CTCP \"%s\" => \"%s\""),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, option_name, value);
    }

    return rc;
}

/*
 * Reads ignore option from configuration file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_config_ignore_read_cb (void *data,
                           struct t_config_file *config_file,
                           struct t_config_section *section,
                           const char *option_name, const char *value)
{
    char **argv, **argv_eol;
    int argc;

    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;

    if (option_name)
    {
        if (value && value[0])
        {
            argv = weechat_string_split (value, ";", 0, 0, &argc);
            argv_eol = weechat_string_split (value, ";", 1, 0, NULL);
            if (argv && argv_eol && (argc >= 3))
            {
                irc_ignore_new (argv_eol[2], argv[0], argv[1]);
            }
            if (argv)
                weechat_string_free_split (argv);
            if (argv_eol)
                weechat_string_free_split (argv_eol);
        }
    }

    return 1;
}

/*
 * Writes ignore section in IRC configuration file.
 */

int
irc_config_ignore_write_cb (void *data, struct t_config_file *config_file,
                            const char *section_name)
{
    struct t_irc_ignore *ptr_ignore;

    /* make C compiler happy */
    (void) data;

    if (!weechat_config_write_line (config_file, section_name, NULL))
        return WEECHAT_CONFIG_WRITE_ERROR;

    for (ptr_ignore = irc_ignore_list; ptr_ignore;
         ptr_ignore = ptr_ignore->next_ignore)
    {
        if (!weechat_config_write_line (config_file,
                                        "ignore",
                                        "%s;%s;%s",
                                        (ptr_ignore->server) ? ptr_ignore->server : "*",
                                        (ptr_ignore->channel) ? ptr_ignore->channel : "*",
                                        ptr_ignore->mask))
            return WEECHAT_CONFIG_WRITE_ERROR;
    }

    return WEECHAT_CONFIG_WRITE_OK;
}

/*
 * Creates a new option for a server.
 *
 * Returns pointer to new option, NULL if error.
 */

struct t_config_option *
irc_config_server_new_option (struct t_config_file *config_file,
                              struct t_config_section *section,
                              int index_option,
                              const char *option_name,
                              const char *default_value,
                              const char *value,
                              int null_value_allowed,
                              int (*callback_check_value)(void *data,
                                                          struct t_config_option *option,
                                                          const char *value),
                              void *callback_check_value_data,
                              void (*callback_change)(void *data,
                                                      struct t_config_option *option),
                              void *callback_change_data)
{
    struct t_config_option *new_option;

    new_option = NULL;

    switch (index_option)
    {
        case IRC_SERVER_OPTION_ADDRESSES:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("list of hostname/port or IP/port for server (separated by "
                   "comma) "
                   "(note: content is evaluated, see /help eval)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_PROXY:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("name of proxy used for this server (optional, proxy must "
                   "be defined with command /proxy)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_IPV6:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "boolean",
                N_("use IPv6 protocol for server communication (try IPv6 then "
                   "fallback to IPv4); if disabled, only IPv4 is used"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_SSL:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "boolean",
                N_("use SSL for server communication"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_SSL_CERT:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("SSL certificate file used to automatically identify your "
                   "nick (\"%h\" will be replaced by WeeChat home, "
                   "\"~/.weechat\" by default)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_SSL_PRIORITIES:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("string with priorities for gnutls (for syntax, see "
                   "documentation of function gnutls_priority_init in gnutls "
                   "manual, common strings are: \"PERFORMANCE\", \"NORMAL\", "
                    "\"SECURE128\", \"SECURE256\", \"EXPORT\", \"NONE\")"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_SSL_DHKEY_SIZE:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("size of the key used during the Diffie-Hellman Key "
                   "Exchange"),
                NULL, 0, INT_MAX,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_SSL_FINGERPRINT:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("fingerprint of certificate which is trusted and accepted "
                   "for the server; only hexadecimal digits are allowed (0-9, "
                   "a-f): 64 chars for SHA-512, 32 chars for SHA-256, "
                   "20 chars for SHA-1 (insecure, not recommended); many "
                   "fingerprints can be separated by commas; if this option "
                   "is set, the other checks on certificates are NOT "
                   "performed (option \"ssl_verify\")"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_SSL_VERIFY:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "boolean",
                N_("check that the SSL connection is fully trusted"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_PASSWORD:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("password for server "
                   "(note: content is evaluated, see /help eval)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_CAPABILITIES:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                /* TRANSLATORS: please keep words "client capabilities" between brackets if translation is different (see fr.po) */
                N_("comma-separated list of client capabilities to enable for "
                   "server if they are available (see /help cap for a list of "
                   "capabilities supported by WeeChat) "
                   "(example: \"away-notify,multi-prefix\")"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_SASL_MECHANISM:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("mechanism for SASL authentication: "
                   "\"plain\" for plain text password, "
                   "\"ecdsa-nist256p-challenge\" for key-based "
                   "challenge authentication, "
                   "\"external\" for authentication using client side SSL "
                   "cert, "
                   "\"dh-blowfish\" for blowfish crypted password "
                   "(insecure, not recommended), "
                   "\"dh-aes\" for AES crypted password "
                   "(insecure, not recommended)"),
                "plain|ecdsa-nist256p-challenge|external|dh-blowfish|dh-aes",
                0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_SASL_USERNAME:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("username for SASL authentication; this option is not used "
                   "for mechanism \"external\" "
                   "(note: content is evaluated, see /help eval)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_SASL_PASSWORD:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("password for SASL authentication; this option is not used "
                   "for mechanisms \"ecdsa-nist256p-challenge\" and "
                   "\"external\" "
                   "(note: content is evaluated, see /help eval)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_SASL_KEY:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("file with ECC private key for mechanism "
                   "\"ecdsa-nist256p-challenge\" "
                   "(\"%h\" will be replaced by WeeChat home, "
                   "\"~/.weechat\" by default)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_SASL_TIMEOUT:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("timeout (in seconds) before giving up SASL "
                   "authentication"),
                NULL, 1, 3600,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_SASL_FAIL:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("action to perform if SASL authentication fails: "
                   "\"continue\" to ignore the authentication problem, "
                   "\"reconnect\" to schedule a reconnection to the server, "
                   "\"disconnect\" to disconnect from server"),
                "continue|reconnect|disconnect", 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_AUTOCONNECT:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "boolean",
                N_("automatically connect to server when WeeChat is starting"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_AUTORECONNECT:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "boolean",
                N_("automatically reconnect to server when disconnected"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_AUTORECONNECT_DELAY:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("delay (in seconds) before trying again to reconnect to "
                   "server"),
                NULL, 1, 65535,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_NICKS:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("nicknames to use on server (separated by comma) "
                   "(note: content is evaluated, see /help eval)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_NICKS_ALTERNATE:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "boolean",
                N_("get an alternate nick when all the declared nicks are "
                   "already used on server: add some \"_\" until the nick has "
                   "a length of 9, and then replace last char (or the two "
                   "last chars) by a number from 1 to 99, until we find "
                   "a nick not used on server"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_USERNAME:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("user name to use on server "
                   "(note: content is evaluated, see /help eval)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_REALNAME:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("real name to use on server "
                   "(note: content is evaluated, see /help eval)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_LOCAL_HOSTNAME:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("custom local hostname/IP for server (optional, if empty "
                   "local hostname is used)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_COMMAND:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("command(s) to run after connection to server and before "
                   "auto-join of channels (many commands can be separated by "
                   "\";\", use \"\\;\" for a semicolon, special variables "
                   "$nick, $channel and $server are replaced by their value) "
                   "(note: content is evaluated, see /help eval)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_COMMAND_DELAY:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("delay (in seconds) after execution of command and before "
                   "auto-join of channels (example: give some time for "
                   "authentication before joining channels)"),
                NULL, 0, 3600,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_AUTOJOIN:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("comma separated list of channels to join after connection "
                   "to server (and after executing command + delay if they are "
                   "set); the channels that require a key must be at beginning "
                   "of the list, and all the keys must be given after the "
                   "channels (separated by a space) (example: \"#channel1,"
                   "#channel2,#channel3 key1,key2\" where #channel1 and "
                   "#channel2 are protected by key1 and key2) "
                   "(note: content is evaluated, see /help eval)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_AUTOREJOIN:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "boolean",
                N_("automatically rejoin channels after kick; you can define "
                   "a buffer local variable on a channel to override this "
                   "value (name of variable: \"autorejoin\", value: \"on\" or "
                   "\"off\")"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_AUTOREJOIN_DELAY:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("delay (in seconds) before autorejoin (after kick)"),
                NULL, 0, 3600*24,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_CONNECTION_TIMEOUT:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("timeout (in seconds) between TCP connection to server and "
                   "message 001 received, if this timeout is reached before "
                   "001 message is received, WeeChat will disconnect from "
                   "server"),
                NULL, 1, 3600,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_ANTI_FLOOD_PRIO_HIGH:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("anti-flood for high priority queue: number of seconds "
                   "between two user messages or commands sent to IRC server "
                   "(0 = no anti-flood)"),
                NULL, 0, 60,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_ANTI_FLOOD_PRIO_LOW:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("anti-flood for low priority queue: number of seconds "
                   "between two messages sent to IRC server (messages like "
                   "automatic CTCP replies) (0 = no anti-flood)"),
                NULL, 0, 60,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_AWAY_CHECK:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("interval between two checks for away (in minutes, "
                   "0 = never check)"),
                NULL, 0, 60 * 24 * 7,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_AWAY_CHECK_MAX_NICKS:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("do not check away nicks on channels with high number of "
                   "nicks (0 = unlimited)"),
                NULL, 0, 1000000,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_DEFAULT_MSG_KICK:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("default kick message used by commands \"/kick\" and "
                   "\"/kickban\" (special variables $nick, $channel and $server "
                   "are replaced by their value)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_DEFAULT_MSG_PART:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("default part message (leaving channel) (\"%v\" will be "
                   "replaced by WeeChat version in string)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_DEFAULT_MSG_QUIT:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("default quit message (disconnecting from server) (\"%v\" "
                   "will be replaced by WeeChat version in string)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                callback_check_value, callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_NOTIFY:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("notify list for server (you should not change this option "
                    "but use /notify command instead)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                (section == irc_config_section_server_default) ?
                &irc_config_server_default_check_notify : callback_check_value,
                callback_check_value_data,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_NUM_OPTIONS:
            break;
    }

    return new_option;
}

/*
 * Reads server option in IRC configuration file.
 */

int
irc_config_server_read_cb (void *data, struct t_config_file *config_file,
                           struct t_config_section *section,
                           const char *option_name, const char *value)
{
    struct t_irc_server *ptr_server;
    int index_option, rc, i;
    char *pos_option, *server_name;

    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    if (option_name)
    {
        pos_option = strrchr (option_name, '.');
        if (pos_option)
        {
            server_name = weechat_strndup (option_name,
                                           pos_option - option_name);
            pos_option++;
            if (server_name)
            {
                index_option = irc_server_search_option (pos_option);
                if (index_option >= 0)
                {
                    ptr_server = irc_server_search (server_name);
                    if (!ptr_server)
                        ptr_server = irc_server_alloc (server_name);
                    if (ptr_server)
                    {
                        if (ptr_server->reloading_from_config
                            && !ptr_server->reloaded_from_config)
                        {
                            for (i = 0; i < IRC_SERVER_NUM_OPTIONS; i++)
                            {
                                weechat_config_option_set (
                                    ptr_server->options[i], NULL, 1);
                            }
                            ptr_server->reloaded_from_config = 1;
                        }
                        rc = weechat_config_option_set (
                            ptr_server->options[index_option], value, 1);
                    }
                    else
                    {
                        weechat_printf (
                            NULL,
                            _("%s%s: error adding server \"%s\""),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            server_name);
                    }
                }
                free (server_name);
            }
        }
    }

    if (rc == WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        weechat_printf (
            NULL,
            _("%s%s: error creating server option \"%s\""),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, option_name);
    }

    return rc;
}

/*
 * Writes server section in IRC configuration file.
 */

int
irc_config_server_write_cb (void *data, struct t_config_file *config_file,
                            const char *section_name)
{
    struct t_irc_server *ptr_server;
    int i;

    /* make C compiler happy */
    (void) data;

    if (!weechat_config_write_line (config_file, section_name, NULL))
        return WEECHAT_CONFIG_WRITE_ERROR;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (!ptr_server->temp_server || irc_config_write_temp_servers)
        {
            for (i = 0; i < IRC_SERVER_NUM_OPTIONS; i++)
            {
                if (!weechat_config_write_option (config_file,
                                                  ptr_server->options[i]))
                    return WEECHAT_CONFIG_WRITE_ERROR;
            }
        }
    }

    return WEECHAT_CONFIG_WRITE_OK;
}

/*
 * Creates default options for servers.
 */

void
irc_config_server_create_default_options (struct t_config_section *section)
{
    int i, length;
    char *nicks, *username, *realname, *default_value;
    struct passwd *my_passwd;

    nicks = NULL;
    username = NULL;
    realname = strdup ("");

    /* Get the user's name from /etc/passwd */
    if ((my_passwd = getpwuid (geteuid ())) != NULL)
    {
        length = (strlen (my_passwd->pw_name) + 4) * 5;
        nicks = malloc (length);
        if (nicks)
        {
            snprintf (nicks, length, "%s,%s1,%s2,%s3,%s4",
                      my_passwd->pw_name,
                      my_passwd->pw_name,
                      my_passwd->pw_name,
                      my_passwd->pw_name,
                      my_passwd->pw_name);
        }
        username = strdup (my_passwd->pw_name);
    }
    else
    {
        /* default values if /etc/passwd can't be read */
        nicks = strdup (IRC_SERVER_DEFAULT_NICKS);
        username = strdup ("weechat");
    }

    for (i = 0; i < IRC_SERVER_NUM_OPTIONS; i++)
    {
        default_value = NULL;
        switch (i)
        {
            case IRC_SERVER_OPTION_NICKS:
                default_value = nicks;
                break;
            case IRC_SERVER_OPTION_USERNAME:
                default_value = username;
                break;
            case IRC_SERVER_OPTION_REALNAME:
                default_value = realname;
                break;
        }
        if (!default_value)
            default_value = irc_server_options[i][1];

        irc_config_server_default[i] = irc_config_server_new_option (
            irc_config_file,
            section,
            i,
            irc_server_options[i][0],
            irc_server_options[i][1],
            default_value,
            0,
            &irc_config_server_check_value_cb,
            irc_server_options[i][0],
            &irc_config_server_default_change_cb,
            irc_server_options[i][0]);
    }

    if (nicks)
        free (nicks);
    if (username)
        free (username);
    if (realname)
        free (realname);
}

/*
 * Initializes IRC configuration file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_config_init ()
{
    struct t_config_section *ptr_section;

    irc_config_hashtable_display_join_message = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL,
        NULL);
    irc_config_hashtable_nick_color_force = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL,
        NULL);
    irc_config_hashtable_nick_prefixes = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL,
        NULL);
    irc_config_hashtable_color_mirc_remap = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL,
        NULL);

    irc_config_file = weechat_config_new (IRC_CONFIG_NAME,
                                          &irc_config_reload, NULL);
    if (!irc_config_file)
        return 0;

    /* look */
    ptr_section = weechat_config_new_section (irc_config_file, "look",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (irc_config_file);
        return 0;
    }

    irc_config_look_buffer_open_before_autojoin = weechat_config_new_option (
        irc_config_file, ptr_section,
        "buffer_open_before_autojoin", "boolean",
        N_("open channel buffer before the JOIN is received from server "
           "when it is auto joined (with server option \"autojoin\"); "
           "this is useful to open channels with always the same buffer "
           "numbers on startup"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_buffer_open_before_join = weechat_config_new_option (
        irc_config_file, ptr_section,
        "buffer_open_before_join", "boolean",
        N_("open channel buffer before the JOIN is received from server "
           "when it is manually joined (with /join command)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_buffer_switch_autojoin = weechat_config_new_option (
        irc_config_file, ptr_section,
        "buffer_switch_autojoin", "boolean",
        N_("auto switch to channel buffer when it is auto joined (with "
           "server option \"autojoin\")"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_buffer_switch_join = weechat_config_new_option (
        irc_config_file, ptr_section,
        "buffer_switch_join", "boolean",
        N_("auto switch to channel buffer when it is manually joined "
           "(with /join command)"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_color_nicks_in_names = weechat_config_new_option (
        irc_config_file, ptr_section,
        "color_nicks_in_names", "boolean",
        N_("use nick color in output of /names (or list of nicks displayed "
           "when joining a channel)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_color_nicks_in_nicklist = weechat_config_new_option (
        irc_config_file, ptr_section,
        "color_nicks_in_nicklist", "boolean",
        N_("use nick color in nicklist"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL,
        &irc_config_change_look_color_nicks_in_nicklist, NULL, NULL, NULL);
    irc_config_look_color_nicks_in_server_messages = weechat_config_new_option (
        irc_config_file, ptr_section,
        "color_nicks_in_server_messages", "boolean",
        N_("use nick color in messages from server"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_color_pv_nick_like_channel = weechat_config_new_option (
        irc_config_file, ptr_section,
        "color_pv_nick_like_channel", "boolean",
        N_("use same nick color for channel and private"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_ctcp_time_format = weechat_config_new_option (
        irc_config_file, ptr_section,
        "ctcp_time_format", "string",
        N_("time format used in answer to message CTCP TIME (see man strftime "
           "for date/time specifiers)"),
        NULL, 0, 0, "%a, %d %b %Y %T %z", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_display_away = weechat_config_new_option (
        irc_config_file, ptr_section,
        "display_away", "integer",
        N_("display message when (un)marking as away (off: do not display/send "
           "anything, local: display locally, channel: send action to channels)"),
        "off|local|channel", 0, 0, "local", NULL, 0, NULL, NULL,
        &irc_config_change_look_display_away, NULL,
        NULL, NULL);
    irc_config_look_display_ctcp_blocked = weechat_config_new_option (
        irc_config_file, ptr_section,
        "display_ctcp_blocked", "boolean",
        N_("display CTCP message even if it is blocked"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_display_ctcp_reply = weechat_config_new_option (
        irc_config_file, ptr_section,
        "display_ctcp_reply", "boolean",
        N_("display CTCP reply sent by WeeChat"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_display_ctcp_unknown = weechat_config_new_option (
        irc_config_file, ptr_section,
        "display_ctcp_unknown", "boolean",
        N_("display CTCP message even if it is unknown CTCP"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_display_host_join = weechat_config_new_option (
        irc_config_file, ptr_section,
        "display_host_join", "boolean",
        N_("display host in join messages"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_display_host_join_local = weechat_config_new_option (
        irc_config_file, ptr_section,
        "display_host_join_local", "boolean",
        N_("display host in join messages from local client"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_display_host_quit = weechat_config_new_option (
        irc_config_file, ptr_section,
        "display_host_quit", "boolean",
        N_("display host in part/quit messages"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_display_join_message = weechat_config_new_option (
        irc_config_file, ptr_section,
        "display_join_message", "string",
        N_("comma-separated list of messages to display after joining a "
           "channel: 324 = channel modes, 329 = channel creation date, "
           "332 = topic, 333 = nick/date for topic, 353 = names on channel, "
           "366 = names count"),
        NULL, 0, 0, "329,332,333,366", NULL, 0, NULL, NULL,
        &irc_config_change_look_display_join_message, NULL, NULL, NULL);
    irc_config_look_display_old_topic = weechat_config_new_option (
        irc_config_file, ptr_section,
        "display_old_topic", "boolean",
        N_("display old topic when channel topic is changed"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_display_pv_away_once = weechat_config_new_option (
        irc_config_file, ptr_section,
        "display_pv_away_once", "boolean",
        N_("display remote away message only once in private"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_display_pv_back = weechat_config_new_option (
        irc_config_file, ptr_section,
        "display_pv_back", "boolean",
        N_("display a message in private when user is back (after quit on "
           "server)"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_highlight_channel = weechat_config_new_option (
        irc_config_file, ptr_section,
        "highlight_channel", "string",
        N_("comma separated list of words to highlight in channel buffers "
           "(case insensitive, use \"(?-i)\" at beginning of words to "
           "make them case sensitive; special variables $nick, $channel and "
           "$server are replaced by their value), these words are added to "
           "buffer property \"highlight_words\" only when buffer is created "
           "(it does not affect current buffers), an empty string disables "
            "default highlight on nick, examples: \"$nick\", \"(?-i)$nick\""),
        NULL, 0, 0, "$nick", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_highlight_pv = weechat_config_new_option (
        irc_config_file, ptr_section,
        "highlight_pv", "string",
        N_("comma separated list of words to highlight in private buffers "
           "(case insensitive, use \"(?-i)\" at beginning of words to "
           "make them case sensitive; special variables $nick, $channel and "
           "$server are replaced by their value), these words are added to "
           "buffer property \"highlight_words\" only when buffer is created "
           "(it does not affect current buffers), an empty string disables "
            "default highlight on nick, examples: \"$nick\", \"(?-i)$nick\""),
        NULL, 0, 0, "$nick", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_highlight_server = weechat_config_new_option (
        irc_config_file, ptr_section,
        "highlight_server", "string",
        N_("comma separated list of words to highlight in server buffers "
           "(case insensitive, use \"(?-i)\" at beginning of words to "
           "make them case sensitive; special variables $nick, $channel and "
           "$server are replaced by their value), these words are added to "
           "buffer property \"highlight_words\" only when buffer is created "
           "(it does not affect current buffers), an empty string disables "
           "default highlight on nick, examples: \"$nick\", \"(?-i)$nick\""),
        NULL, 0, 0, "$nick", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_highlight_tags_restrict = weechat_config_new_option (
        irc_config_file, ptr_section,
        "highlight_tags_restrict", "string",
        N_("restrict highlights to these tags on irc buffers (to have "
           "highlight on user messages but not server messages); tags "
           "must be separated by a comma and \"+\" can be used to make a "
           "logical \"and\" between tags; wildcard \"*\" is allowed in tags; "
           "an empty value allows highlight on any tag"),
        NULL, 0, 0, "irc_privmsg,irc_notice", NULL, 0, NULL, NULL,
        &irc_config_change_look_highlight_tags_restrict, NULL, NULL, NULL);
    irc_config_look_item_away_message = weechat_config_new_option (
        irc_config_file, ptr_section,
        "item_away_message", "boolean",
        N_("display server away message in away bar item"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        &irc_config_change_look_item_away_message, NULL, NULL, NULL);
    irc_config_look_item_channel_modes_hide_args = weechat_config_new_option (
        irc_config_file, ptr_section,
        "item_channel_modes_hide_args", "string",
        N_("hide channel modes arguments if at least one of these modes is in "
           "channel modes (\"*\" to always hide all arguments, empty value to "
           "never hide arguments); example: \"kf\" to hide arguments if \"k\" "
           "or \"f\" are in channel modes"),
        NULL, 0, 0, "k", NULL, 0, NULL, NULL,
        &irc_config_change_look_item_channel_modes_hide_args, NULL, NULL, NULL);
    irc_config_look_item_display_server = weechat_config_new_option (
        irc_config_file, ptr_section,
        "item_display_server", "integer",
        N_("name of bar item where IRC server is displayed (for status bar)"),
        "buffer_plugin|buffer_name", 0, 0, "buffer_plugin", NULL, 0, NULL, NULL,
        &irc_config_change_look_item_display_server, NULL, NULL, NULL);
    irc_config_look_item_nick_modes = weechat_config_new_option (
        irc_config_file, ptr_section,
        "item_nick_modes", "boolean",
        N_("display nick modes in bar item \"input_prompt\""),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        &irc_config_change_bar_item_input_prompt, NULL, NULL, NULL);
    irc_config_look_item_nick_prefix = weechat_config_new_option (
        irc_config_file, ptr_section,
        "item_nick_prefix", "boolean",
        N_("display nick prefix in bar item \"input_prompt\""),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        &irc_config_change_bar_item_input_prompt, NULL, NULL, NULL);
    irc_config_look_join_auto_add_chantype = weechat_config_new_option (
        irc_config_file, ptr_section,
        "join_auto_add_chantype", "boolean",
        N_("automatically add channel type in front of channel name on "
           "command /join if the channel name does not start with a valid "
           "channel type for the server; for example: \"/join weechat\" will "
           "in fact send: \"/join #weechat\""),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_msgbuffer_fallback = weechat_config_new_option (
        irc_config_file, ptr_section,
        "msgbuffer_fallback", "integer",
        N_("default target buffer for msgbuffer options when target is "
           "private and that private buffer is not found"),
        "current|server", 0, 0, "current", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_new_channel_position = weechat_config_new_option (
        irc_config_file, ptr_section,
        "new_channel_position", "integer",
        N_("force position of new channel in list of buffers "
           "(none = default position (should be last buffer), "
           "next = current buffer + 1, near_server = after last channel/pv "
           "of server)"),
        "none|next|near_server", 0, 0, "none",
        NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_new_pv_position = weechat_config_new_option (
        irc_config_file, ptr_section,
        "new_pv_position", "integer",
        N_("force position of new private in list of buffers "
           "(none = default position (should be last buffer), "
           "next = current buffer + 1, near_server = after last channel/pv "
           "of server)"),
        "none|next|near_server", 0, 0, "none",
        NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_nick_color_force = weechat_config_new_option (
        irc_config_file, ptr_section,
        "nick_color_force", "string",
        N_("force color for some nicks: hash computed with nickname "
           "to find color will not be used for these nicks (format is: "
           "\"nick1:color1;nick2:color2\"); look up for nicks is with "
           "exact case then lower case, so it's possible to use only lower "
           "case for nicks in this option"),
        NULL, 0, 0, "", NULL, 0, NULL, NULL,
        &irc_config_change_look_nick_color_force, NULL, NULL, NULL);
    irc_config_look_nick_color_hash = weechat_config_new_option (
        irc_config_file, ptr_section,
        "nick_color_hash", "integer",
        N_("hash algorithm used to find the color for a nick: djb2 = variant "
           "of djb2 (position of letters matters: anagrams of a nick have "
           "different color), sum = sum of letters"),
        "djb2|sum", 0, 0, "sum", NULL, 0, NULL, NULL,
        &irc_config_change_look_nick_colors, NULL, NULL, NULL);
    irc_config_look_nick_color_stop_chars = weechat_config_new_option (
        irc_config_file, ptr_section,
        "nick_color_stop_chars", "string",
        N_("chars used to stop in nick when computing color with letters of "
           "nick (at least one char outside this list must be in string before "
           "stopping) (example: nick \"|nick|away\" with \"|\" in chars will "
           "return color of nick \"|nick\")"),
        NULL, 0, 0, "_|[", NULL, 0, NULL, NULL,
        &irc_config_change_look_nick_colors, NULL, NULL, NULL);
    irc_config_look_nick_completion_smart = weechat_config_new_option (
        irc_config_file, ptr_section,
        "nick_completion_smart", "integer",
        N_("smart completion for nicks (completes first with last speakers): "
           "speakers = all speakers (including highlights), "
           "speakers_highlights = only speakers with highlight"),
        "off|speakers|speakers_highlights", 0, 0, "speakers", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_nick_mode = weechat_config_new_option (
        irc_config_file, ptr_section,
        "nick_mode", "integer",
        N_("display nick mode (op, voice, ...) before nick (none = never, "
           "prefix = in prefix only (default), action = in action messages "
           "only, both = prefix + action messages)"),
        "none|prefix|action|both", 0, 0, "prefix",
        NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_nick_mode_empty = weechat_config_new_option (
        irc_config_file, ptr_section,
        "nick_mode_empty", "boolean",
        N_("display a space if nick mode is enabled but nick has no mode (not "
           "op, voice, ...)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL,
        &irc_config_change_bar_item_input_prompt, NULL, NULL, NULL);
    irc_config_look_nicks_hide_password = weechat_config_new_option (
        irc_config_file, ptr_section,
        "nicks_hide_password", "string",
        N_("comma separated list of nicks for which passwords will be hidden "
           "when a message is sent, for example to hide password in message "
           "displayed by \"/msg nickserv identify password\", example: "
           "\"nickserv,nickbot\""),
        NULL, 0, 0, "nickserv", NULL, 0, NULL, NULL,
        &irc_config_change_look_nicks_hide_password, NULL, NULL, NULL);
    irc_config_look_notice_as_pv = weechat_config_new_option (
        irc_config_file, ptr_section,
        "notice_as_pv", "integer",
        N_("display notices as private messages (if auto, use private buffer "
           "if found)"),
        "auto|never|always", 0, 0, "auto", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_notice_welcome_redirect = weechat_config_new_option (
        irc_config_file, ptr_section,
        "notice_welcome_redirect", "boolean",
        N_("automatically redirect channel welcome notices to the channel "
           "buffer; such notices have the nick as target but a channel name in "
           "beginning of notice message, for example notices sent by freenode "
           "server which look like: \"[#channel] Welcome to this channel...\""),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_notice_welcome_tags = weechat_config_new_option (
        irc_config_file, ptr_section,
        "notice_welcome_tags", "string",
        N_("comma separated list of tags used in a welcome notices redirected "
           "to a channel, for example: \"notify_private\""),
        NULL, 0, 0, "", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_notify_tags_ison = weechat_config_new_option (
        irc_config_file, ptr_section,
        "notify_tags_ison", "string",
        N_("comma separated list of tags used in messages displayed by notify "
           "when a nick joins or quits server (result of command ison or "
           "monitor), for example: \"notify_message\", \"notify_private\" or "
           "\"notify_highlight\""),
        NULL, 0, 0, "notify_message", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_notify_tags_whois = weechat_config_new_option (
        irc_config_file, ptr_section,
        "notify_tags_whois", "string",
        N_("comma separated list of tags used in messages displayed by notify "
           "when a nick away status changes (result of command whois), "
           "for example: \"notify_message\", \"notify_private\" or "
           "\"notify_highlight\""),
        NULL, 0, 0, "notify_message", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_part_closes_buffer = weechat_config_new_option (
        irc_config_file, ptr_section,
        "part_closes_buffer", "boolean",
        N_("close buffer when /part is issued on a channel"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_pv_buffer = weechat_config_new_option (
        irc_config_file, ptr_section,
        "pv_buffer", "integer",
        N_("merge private buffers"),
        "independent|merge_by_server|merge_all", 0, 0, "independent",
        NULL, 0, NULL, NULL,
        &irc_config_change_look_pv_buffer, NULL, NULL, NULL);
    irc_config_look_pv_tags = weechat_config_new_option (
        irc_config_file, ptr_section,
        "pv_tags", "string",
        N_("comma separated list of tags used in private messages, for example: "
           "\"notify_message\", \"notify_private\" or \"notify_highlight\""),
        NULL, 0, 0, "notify_private", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_raw_messages = weechat_config_new_option (
        irc_config_file, ptr_section,
        "raw_messages", "integer",
        N_("number of raw messages to save in memory when raw data buffer is "
           "closed (messages will be displayed when opening raw data buffer)"),
        NULL, 0, 65535, "256", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_server_buffer = weechat_config_new_option (
        irc_config_file, ptr_section,
        "server_buffer", "integer",
        N_("merge server buffers"),
        "merge_with_core|merge_without_core|independent", 0, 0, "merge_with_core",
        NULL, 0, NULL, NULL,
        &irc_config_change_look_server_buffer, NULL, NULL, NULL);
    irc_config_look_smart_filter = weechat_config_new_option (
        irc_config_file, ptr_section,
        "smart_filter", "boolean",
        N_("filter join/part/quit/nick messages for a nick if not speaking "
           "for some minutes on channel (you must create a filter on tag "
           "\"irc_smart_filter\")"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_smart_filter_delay = weechat_config_new_option (
        irc_config_file, ptr_section,
        "smart_filter_delay", "integer",
        N_("delay for filtering join/part/quit messages (in minutes): if the "
           "nick did not speak during the last N minutes, the join/part/quit is "
           "filtered"),
        NULL, 1, 60*24*7, "5", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_smart_filter_join = weechat_config_new_option (
        irc_config_file, ptr_section,
        "smart_filter_join", "boolean",
        /* TRANSLATORS: please do not translate "join" */
        N_("enable smart filter for \"join\" messages"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_smart_filter_join_unmask = weechat_config_new_option (
        irc_config_file, ptr_section,
        "smart_filter_join_unmask", "integer",
        N_("delay for unmasking a join message that was filtered with tag "
           "\"irc_smart_filter\" (in minutes): if a nick has joined max N "
           "minutes ago and then says something on channel (message, notice or "
           "update on topic), the join is unmasked, as well as nick changes "
           "after this join (0 = disable: never unmask a join)"),
        NULL, 0, 60*24*7, "30", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_smart_filter_mode = weechat_config_new_option (
        irc_config_file, ptr_section,
        "smart_filter_mode", "string",
        /* TRANSLATORS: please do not translate "mode" */
        N_("enable smart filter for \"mode\" messages: \"*\" to filter all "
           "modes, \"+\" to filter all modes in server prefixes (for example "
           "\"ovh\"), \"xyz\" to filter only modes x/y/z, \"-xyz\" to filter "
           "all modes but not x/y/z; examples: \"ovh\": filter modes o/v/h, "
           "\"-bkl\": filter all modes but not b/k/l"),
        NULL, 0, 0, "+", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_smart_filter_nick = weechat_config_new_option (
        irc_config_file, ptr_section,
        "smart_filter_nick", "boolean",
        /* TRANSLATORS: please do not translate "nick" */
        N_("enable smart filter for \"nick\" messages (nick changes)"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_smart_filter_quit = weechat_config_new_option (
        irc_config_file, ptr_section,
        "smart_filter_quit", "boolean",
        /* TRANSLATORS: please do not translate "part" and "quit" */
        N_("enable smart filter for \"part\" and \"quit\" messages"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_temporary_servers = weechat_config_new_option (
        irc_config_file, ptr_section,
        "temporary_servers", "boolean",
        N_("enable automatic addition of temporary servers with command "
           "/connect"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_topic_strip_colors = weechat_config_new_option (
        irc_config_file, ptr_section,
        "topic_strip_colors", "boolean",
        N_("strip colors in topic (used only when displaying buffer title)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL,
        &irc_config_change_look_topic_strip_colors, NULL, NULL, NULL);

    /* color */
    ptr_section = weechat_config_new_section (irc_config_file, "color",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (irc_config_file);
        return 0;
    }

    irc_config_color_input_nick = weechat_config_new_option (
        irc_config_file, ptr_section,
        "input_nick", "color",
        N_("color for nick in input bar"),
        NULL, -1, 0, "lightcyan", NULL, 0, NULL, NULL,
        &irc_config_change_bar_item_input_prompt, NULL, NULL, NULL);
    irc_config_color_item_away = weechat_config_new_option (
        irc_config_file, ptr_section,
        "item_away", "color",
        N_("color for away item"),
        NULL, -1, 0, "yellow", NULL, 0, NULL, NULL,
        &irc_config_change_color_item_away, NULL, NULL, NULL);
    irc_config_color_item_channel_modes = weechat_config_new_option (
        irc_config_file, ptr_section,
        "item_channel_modes", "color",
        N_("color for channel modes, near channel name"),
        NULL, -1, 0, "default", NULL, 0, NULL, NULL,
        &irc_config_change_color_item_channel_modes, NULL, NULL, NULL);
    irc_config_color_item_lag_counting = weechat_config_new_option (
        irc_config_file, ptr_section,
        "item_lag_counting", "color",
        N_("color for lag indicator, when counting (pong not received from "
           "server, lag is increasing)"),
        NULL, -1, 0, "default", NULL, 0, NULL, NULL,
        &irc_config_change_color_item_lag, NULL, NULL, NULL);
    irc_config_color_item_lag_finished = weechat_config_new_option (
        irc_config_file, ptr_section,
        "item_lag_finished", "color",
        N_("color for lag indicator, when pong has been received from server"),
        NULL, -1, 0, "yellow", NULL, 0, NULL, NULL,
        &irc_config_change_color_item_lag, NULL, NULL, NULL);
    irc_config_color_item_nick_modes = weechat_config_new_option (
        irc_config_file, ptr_section,
        "item_nick_modes", "color",
        N_("color for nick modes in bar item \"input_prompt\""),
        NULL, -1, 0, "default", NULL, 0, NULL, NULL,
        &irc_config_change_color_item_nick_modes, NULL, NULL, NULL);
    irc_config_color_message_join = weechat_config_new_option (
        irc_config_file, ptr_section,
        "message_join", "color",
        N_("color for text in join messages"),
        NULL, -1, 0, "green", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_color_message_quit = weechat_config_new_option (
        irc_config_file, ptr_section,
        "message_quit", "color",
        N_("color for text in part/quit messages"),
        NULL, -1, 0, "red", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_color_mirc_remap = weechat_config_new_option (
        irc_config_file, ptr_section,
        "mirc_remap", "string",
        /* TRANSLATORS: please do not translate the list of WeeChat color names at the end of string */
        N_("remap mirc colors in messages using a hashtable: keys are \"fg,bg\" "
           "as integers between -1 (not specified) and 15, values are WeeChat "
           "color names or numbers (format is: \"1,-1:color1;2,7:color2\"), "
           "example: \"1,-1:darkgray;1,2:white,blue\" to remap black to "
           "\"darkgray\" and black on blue to \"white,blue\"; default "
           "WeeChat colors for IRC codes: 0=white, 1=black, 2=blue, 3=green, "
           "4=lightred, 5=red, 6=magenta, 7=brown, 8=yellow, 9=lightgreen, "
           "10=cyan, 11=lightcyan, 12=lightblue, 13=lightmagenta, 14=gray, "
           "15=white"),
        NULL, 0, 0, "1,-1:darkgray", NULL, 0, NULL, NULL,
        &irc_config_change_color_mirc_remap, NULL, NULL, NULL);
    irc_config_color_nick_prefixes = weechat_config_new_option (
        irc_config_file, ptr_section,
        "nick_prefixes", "string",
        N_("color for nick prefixes using mode char (o=op, h=halfop, v=voice, "
           "..), format is: \"o:color1;h:color2;v:color3\" (if a mode is not "
           "found, WeeChat will try with next modes received from server "
           "(\"PREFIX\"); a special mode \"*\" can be used as default color "
           "if no mode has been found in list)"),
        NULL, 0, 0, "y:lightred;q:lightred;a:lightcyan;o:lightgreen;"
        "h:lightmagenta;v:yellow;*:lightblue", NULL, 0, NULL, NULL,
        &irc_config_change_color_nick_prefixes, NULL, NULL, NULL);
    irc_config_color_notice = weechat_config_new_option (
        irc_config_file, ptr_section,
        "notice", "color",
        N_("color for text \"Notice\" in notices"),
        NULL, -1, 0, "green", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_color_reason_quit = weechat_config_new_option (
        irc_config_file, ptr_section,
        "reason_quit", "color",
        N_("color for reason in part/quit messages"),
        NULL, -1, 0, "default", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_color_topic_current = weechat_config_new_option (
        irc_config_file, ptr_section,
        "topic_current", "color",
        N_("color for current channel topic (when joining a channel or "
           "using /topic)"),
        NULL, -1, 0, "default", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_color_topic_new = weechat_config_new_option (
        irc_config_file, ptr_section,
        "topic_new", "color",
        N_("color for new channel topic (when topic is changed)"),
        NULL, -1, 0, "white", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_color_topic_old = weechat_config_new_option (
        irc_config_file, ptr_section,
        "topic_old", "color",
        N_("color for old channel topic (when topic is changed)"),
        NULL, -1, 0, "default", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);

    /* network */
    ptr_section = weechat_config_new_section (irc_config_file, "network",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (irc_config_file);
        return 0;
    }

    irc_config_network_autoreconnect_delay_growing = weechat_config_new_option (
        irc_config_file, ptr_section,
        "autoreconnect_delay_growing", "integer",
        N_("growing factor for autoreconnect delay to server (1 = always same "
           "delay, 2 = delay*2 for each retry, ..)"),
        NULL, 1, 100, "2", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_network_autoreconnect_delay_max = weechat_config_new_option (
        irc_config_file, ptr_section,
        "autoreconnect_delay_max", "integer",
        N_("maximum autoreconnect delay to server (in seconds, 0 = no maximum)"),
        NULL, 0, 3600 * 24 * 7, "600", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_network_ban_mask_default = weechat_config_new_option (
        irc_config_file, ptr_section,
        "ban_mask_default", "string",
        N_("default ban mask for commands /ban, /unban and /kickban; variables "
           "$nick, $user, $ident and $host are replaced by their values "
           "(extracted from \"nick!user@host\"); $ident is the same as $user if "
           "$user does not start with \"~\", otherwise it is set to \"*\"; this "
            "default mask is used only if WeeChat knows the host for the nick"),
        NULL, 0, 0, "*!$ident@$host", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_network_colors_receive = weechat_config_new_option (
        irc_config_file, ptr_section,
        "colors_receive", "boolean",
        N_("when off, colors codes are ignored in incoming messages"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_network_colors_send = weechat_config_new_option (
        irc_config_file, ptr_section,
        "colors_send", "boolean",
        N_("allow user to send colors with special codes (ctrl-c + a code and "
           "optional color: b=bold, cxx=color, cxx,yy=color+background, "
           "i=italic, o=disable color/attributes, r=reverse, u=underline)"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_network_lag_check = weechat_config_new_option (
        irc_config_file, ptr_section,
        "lag_check", "integer",
        N_("interval between two checks for lag (in seconds, 0 = never "
           "check)"),
        NULL, 0, 3600 * 24 * 7, "60", NULL, 0, NULL, NULL,
        &irc_config_change_network_lag_check, NULL, NULL, NULL);
    irc_config_network_lag_max = weechat_config_new_option (
        irc_config_file, ptr_section,
        "lag_max", "integer",
        N_("maximum lag (in seconds): if this lag is reached, WeeChat will "
           "consider that the answer from server (pong) will never be received "
           "and will give up counting the lag (0 = never give up)"),
        NULL, 0, 3600 * 24 * 7, "1800", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_network_lag_min_show = weechat_config_new_option (
        irc_config_file, ptr_section,
        "lag_min_show", "integer",
        N_("minimum lag to show (in milliseconds)"),
        NULL, 0, 1000 * 3600 * 24, "500", NULL, 0, NULL, NULL,
        &irc_config_change_network_lag_min_show, NULL, NULL, NULL);
    irc_config_network_lag_reconnect = weechat_config_new_option (
        irc_config_file, ptr_section,
        "lag_reconnect", "integer",
        N_("reconnect to server if lag is greater than or equal to this value "
           "(in seconds, 0 = never reconnect); this value must be less than or "
           "equal to irc.network.lag_max"),
        NULL, 0, 3600 * 24 * 7, "0", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_network_lag_refresh_interval = weechat_config_new_option (
        irc_config_file, ptr_section,
        "lag_refresh_interval", "integer",
        N_("interval between two refreshs of lag item, when lag is increasing "
           "(in seconds)"),
        NULL, 1, 3600, "1", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_network_notify_check_ison = weechat_config_new_option (
        irc_config_file, ptr_section,
        "notify_check_ison", "integer",
        N_("interval between two checks for notify with IRC command \"ison\" "
           "(in minutes)"),
        NULL, 1, 60 * 24 * 7, "1", NULL, 0, NULL, NULL,
        &irc_config_change_network_notify_check_ison, NULL, NULL, NULL);
    irc_config_network_notify_check_whois = weechat_config_new_option (
        irc_config_file, ptr_section,
        "notify_check_whois", "integer",
        N_("interval between two checks for notify with IRC command \"whois\" "
           "(in minutes)"),
        NULL, 1, 60 * 24 * 7, "5", NULL, 0, NULL, NULL,
        &irc_config_change_network_notify_check_whois, NULL, NULL, NULL);
    irc_config_network_send_unknown_commands = weechat_config_new_option (
        irc_config_file, ptr_section,
        "send_unknown_commands", "boolean",
        N_("send unknown commands to server"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL,
        &irc_config_change_network_send_unknown_commands, NULL, NULL, NULL);
    irc_config_network_whois_double_nick = weechat_config_new_option (
        irc_config_file, ptr_section,
        "whois_double_nick", "boolean",
        N_("double the nick in /whois command (if only one nick is given), to "
           "get idle time in answer; for example: \"/whois nick\" will send "
           "\"whois nick nick\""),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    /* msgbuffer */
    ptr_section = weechat_config_new_section (irc_config_file, "msgbuffer",
                                              1, 1,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL,
                                              &irc_config_msgbuffer_create_option, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (irc_config_file);
        return 0;
    }
    irc_config_section_msgbuffer = ptr_section;

    /* CTCP */
    ptr_section = weechat_config_new_section (irc_config_file, "ctcp",
                                              1, 1,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL,
                                              &irc_config_ctcp_create_option, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (irc_config_file);
        return 0;
    }
    irc_config_section_ctcp = ptr_section;

    /* ignore */
    ptr_section = weechat_config_new_section (irc_config_file, "ignore",
                                              0, 0,
                                              &irc_config_ignore_read_cb, NULL,
                                              &irc_config_ignore_write_cb, NULL,
                                              &irc_config_ignore_write_cb, NULL,
                                              NULL, NULL, NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (irc_config_file);
        return 0;
    }

    /* server_default */
    ptr_section = weechat_config_new_section (irc_config_file, "server_default",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (irc_config_file);
        return 0;
    }
    irc_config_section_server_default = ptr_section;

    irc_config_server_create_default_options (ptr_section);

    /* server */
    ptr_section = weechat_config_new_section (irc_config_file, "server",
                                              0, 0,
                                              &irc_config_server_read_cb, NULL,
                                              &irc_config_server_write_cb, NULL,
                                              NULL, NULL,
                                              NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (irc_config_file);
        return 0;
    }
    irc_config_section_server = ptr_section;

    irc_config_hook_config_nick_colors = weechat_hook_config (
        "weechat.color.chat_nick_colors",
        &irc_config_change_nick_colors_cb, NULL);

    return 1;
}

/*
 * Reads IRC configuration file.
 */

int
irc_config_read ()
{
    int rc;

    irc_config_loading = 1;
    rc = weechat_config_read (irc_config_file);
    irc_config_loading = 0;

    if (rc == WEECHAT_CONFIG_READ_OK)
    {
        irc_notify_new_for_all_servers ();
        irc_config_change_look_display_join_message (NULL, NULL);
        irc_config_change_look_nick_color_force (NULL, NULL);
        irc_config_change_look_nicks_hide_password (NULL, NULL);
        irc_config_change_color_nick_prefixes (NULL, NULL);
        irc_config_change_color_mirc_remap (NULL, NULL);
        irc_config_change_network_notify_check_ison (NULL, NULL);
        irc_config_change_network_notify_check_whois (NULL, NULL);
    }

    return rc;
}

/*
 * Writes IRC configuration file.
 */

int
irc_config_write (int write_temp_servers)
{
    irc_config_write_temp_servers = write_temp_servers;

    return weechat_config_write (irc_config_file);
}

/*
 * Frees IRC configuration.
 */

void
irc_config_free ()
{
    weechat_config_free (irc_config_file);

    if (irc_config_hook_config_nick_colors)
    {
        weechat_unhook (irc_config_hook_config_nick_colors);
        irc_config_hook_config_nick_colors = NULL;
    }
    if (irc_config_nick_colors)
    {
        weechat_string_free_split (irc_config_nick_colors);
        irc_config_nick_colors = NULL;
        irc_config_num_nick_colors = 0;
    }

    if (irc_config_nicks_hide_password)
    {
        weechat_string_free_split (irc_config_nicks_hide_password);
        irc_config_nicks_hide_password = NULL;
        irc_config_num_nicks_hide_password = 0;
    }

    if (irc_config_hashtable_display_join_message)
    {
        weechat_hashtable_free (irc_config_hashtable_display_join_message);
        irc_config_hashtable_display_join_message = NULL;
    }

    if (irc_config_hashtable_nick_color_force)
    {
        weechat_hashtable_free (irc_config_hashtable_nick_color_force);
        irc_config_hashtable_nick_color_force = NULL;
    }

    if (irc_config_hashtable_nick_prefixes)
    {
        weechat_hashtable_free (irc_config_hashtable_nick_prefixes);
        irc_config_hashtable_nick_prefixes = NULL;
    }

    if (irc_config_hashtable_color_mirc_remap)
    {
        weechat_hashtable_free (irc_config_hashtable_color_mirc_remap);
        irc_config_hashtable_color_mirc_remap = NULL;
    }
}
