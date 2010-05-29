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

/*
 * irc-config.c: IRC configuration options (file irc.conf)
 */


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <pwd.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-config.h"
#include "irc-ctcp.h"
#include "irc-buffer.h"
#include "irc-ignore.h"
#include "irc-msgbuffer.h"
#include "irc-nick.h"
#include "irc-server.h"
#include "irc-channel.h"


struct t_config_file *irc_config_file = NULL;
struct t_config_section *irc_config_section_msgbuffer = NULL;
struct t_config_section *irc_config_section_ctcp = NULL;
struct t_config_section *irc_config_section_server_default = NULL;
struct t_config_section *irc_config_section_server = NULL;

/* IRC config, look section */

struct t_config_option *irc_config_look_color_nicks_in_server_messages;
struct t_config_option *irc_config_look_color_pv_nick_like_channel;
struct t_config_option *irc_config_look_server_buffer;
struct t_config_option *irc_config_look_open_channel_near_server;
struct t_config_option *irc_config_look_open_pv_near_server;
struct t_config_option *irc_config_look_nick_prefix;
struct t_config_option *irc_config_look_nick_suffix;
struct t_config_option *irc_config_look_nick_completion_smart;
struct t_config_option *irc_config_look_display_away;
struct t_config_option *irc_config_look_display_ctcp_blocked;
struct t_config_option *irc_config_look_display_ctcp_reply;
struct t_config_option *irc_config_look_display_ctcp_unknown;
struct t_config_option *irc_config_look_display_old_topic;
struct t_config_option *irc_config_look_display_pv_away_once;
struct t_config_option *irc_config_look_display_pv_back;
struct t_config_option *irc_config_look_item_channel_modes;
struct t_config_option *irc_config_look_item_channel_modes_hide_key;
struct t_config_option *irc_config_look_item_nick_modes;
struct t_config_option *irc_config_look_item_nick_prefix;
struct t_config_option *irc_config_look_hide_nickserv_pwd;
struct t_config_option *irc_config_look_highlight_tags;
struct t_config_option *irc_config_look_item_display_server;
struct t_config_option *irc_config_look_msgbuffer_fallback;
struct t_config_option *irc_config_look_notice_as_pv;
struct t_config_option *irc_config_look_part_closes_buffer;
struct t_config_option *irc_config_look_raw_messages;
struct t_config_option *irc_config_look_smart_filter;
struct t_config_option *irc_config_look_smart_filter_delay;
struct t_config_option *irc_config_look_smart_filter_join;
struct t_config_option *irc_config_look_smart_filter_quit;
struct t_config_option *irc_config_look_topic_strip_colors;

/* IRC config, color section */

struct t_config_option *irc_config_color_message_join;
struct t_config_option *irc_config_color_message_quit;
struct t_config_option *irc_config_color_notice;
struct t_config_option *irc_config_color_input_nick;
struct t_config_option *irc_config_color_item_away;
struct t_config_option *irc_config_color_item_channel_modes;

/* IRC config, network section */

struct t_config_option *irc_config_network_autoreconnect_delay_growing;
struct t_config_option *irc_config_network_autoreconnect_delay_max;
struct t_config_option *irc_config_network_connection_timeout;
struct t_config_option *irc_config_network_default_msg_part;
struct t_config_option *irc_config_network_default_msg_quit;
struct t_config_option *irc_config_network_away_check;
struct t_config_option *irc_config_network_away_check_max_nicks;
struct t_config_option *irc_config_network_lag_check;
struct t_config_option *irc_config_network_lag_min_show;
struct t_config_option *irc_config_network_lag_disconnect;
struct t_config_option *irc_config_network_anti_flood[2];
struct t_config_option *irc_config_network_colors_receive;
struct t_config_option *irc_config_network_colors_send;
struct t_config_option *irc_config_network_send_unknown_commands;

/* IRC config, server section */

struct t_config_option *irc_config_server_default[IRC_SERVER_NUM_OPTIONS];

struct t_hook *hook_config_color_nicks_number = NULL;

int irc_config_write_temp_servers = 0;


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
 * irc_config_change_look_color_nicks_number: called when the
 *                                            "weechat.look.color_nicks_number"
 *                                            option is changed
 */

int
irc_config_change_look_color_nicks_number (void *data, const char *option,
                                           const char *value)
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    
    /* make C compiler happy */
    (void) data;
    (void) option;
    (void) value;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            for (ptr_nick = ptr_channel->nicks; ptr_nick;
                 ptr_nick = ptr_nick->next_nick)
            {
                ptr_nick->color = irc_nick_find_color (ptr_nick->name);
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_config_change_look_server_buffer: called when the "one server buffer"
 *                                       option is changed
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
            weechat_buffer_search_main () : irc_buffer_search_first_for_all_servers ();
        
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
 * irc_config_change_look_item_channel_modes: called when the "display
 *                                            channel modes" option is changed
 */

void
irc_config_change_look_item_channel_modes (void *data,
                                           struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    weechat_bar_item_update ("buffer_name");
}

/*
 * irc_config_change_look_item_channel_modes_hide_key: called when the
 *                                                     "display channel modes
 *                                                     hide key" option is
 *                                                     changed
 */

void
irc_config_change_look_item_channel_modes_hide_key (void *data,
                                                    struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    weechat_bar_item_update ("buffer_name");
}

/*
 * irc_config_change_look_item_nick_modes: called when the "display nick modes"
 *                                         option is changed
 */

void
irc_config_change_look_item_nick_modes (void *data,
                                        struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    weechat_bar_item_update ("input_prompt");
}

/*
 * irc_config_change_look_item_nick_prefix: called when the "display nick
 *                                          prefix" option is changed
 */

void
irc_config_change_look_item_nick_prefix (void *data,
                                         struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    weechat_bar_item_update ("input_prompt");
}

/*
 * irc_config_change_look_highlight_tags: called when the "highlight tags"
 *                                        option is changed
 */

void
irc_config_change_look_highlight_tags (void *data,
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
            weechat_buffer_set (ptr_server->buffer, "highlight_tags",
                                weechat_config_string (irc_config_look_highlight_tags));
        }
        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if (ptr_channel->buffer)
            {
                weechat_buffer_set (ptr_channel->buffer, "highlight_tags",
                                    weechat_config_string (irc_config_look_highlight_tags));
            }
        }
    }
}

/*
 * irc_config_change_look_item_display_server: called when the
 *                                             "item_display_server" option is
 *                                             changed
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
}

/*
 * irc_config_change_look_topic_strip_colors: called when the "topic strip colors"
 *                                            option is changed
 */

void
irc_config_change_look_topic_strip_colors (void *data,
                                           struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    weechat_bar_item_update ("buffer_title");
}

/*
 * irc_config_change_color_input_nick: called when the color of input nick is
 *                                     changed
 */

void
irc_config_change_color_input_nick (void *data,
                                    struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    weechat_bar_item_update ("input_prompt");
}

/*
 * irc_config_change_color_item_away: called when the color of away item is
 *                                    changed
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
 * irc_config_change_color_item_buffer_name: called when the color of buffer
 *                                           name is changed
 */

void
irc_config_change_color_item_buffer_name (void *data,
                                          struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    weechat_bar_item_update ("buffer_name");
}

/*
 * irc_config_change_network_away_check: called when away check is changed
 */

void
irc_config_change_network_away_check (void *data,
                                      struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    if (irc_hook_timer_check_away)
    {
        weechat_unhook (irc_hook_timer_check_away);
        irc_hook_timer_check_away = NULL;
    }
    if (weechat_config_integer (irc_config_network_away_check) > 0)
    {
        irc_hook_timer_check_away = weechat_hook_timer (weechat_config_integer (irc_config_network_away_check) * 60 * 1000,
                                                        0, 0,
                                                        &irc_server_timer_check_away_cb,
                                                        NULL);
    }
    else
    {
        /* reset away flag for all nicks/chans/servers */
        irc_server_remove_away ();
    }
}

/*
 * irc_config_change_network_lag_check: called when lag check is changed
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
 * irc_config_change_network_send_unknown_commands: called when "send_unknown_commands"
 *                                                  is changed
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
 * irc_config_server_default_change_cb: callback called when a default server
 *                                      option is modified
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
                }
            }
        }
    }
}

/*
 * irc_config_server_change_cb: callback called when a server option is modified
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
                    irc_server_set_addresses (ptr_server,
                                              IRC_SERVER_OPTION_STRING(ptr_server,
                                                                       IRC_SERVER_OPTION_ADDRESSES));
                    break;
                case IRC_SERVER_OPTION_NICKS:
                    irc_server_set_nicks (ptr_server,
                                          IRC_SERVER_OPTION_STRING(ptr_server,
                                                                   IRC_SERVER_OPTION_NICKS));
                    break;
            }
        }
    }
}

/*
 * irc_config_reload: reload IRC configuration file
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
    
    rc = weechat_config_reload (config_file);
    
    ptr_server = irc_servers;
    while (ptr_server)
    {
        next_server = ptr_server->next_server;
        
        if (!ptr_server->reloaded_from_config)
        {
            if (ptr_server->is_connected)
            {
                weechat_printf (NULL,
                                _("%s%s: warning: server \"%s\" not found "
                                  "in configuration file, not deleted in "
                                  "memory because it's currently used"),
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
 * irc_config_msgbuffer_create_option: set a message target buffer
 */

int
irc_config_msgbuffer_create_option (void *data,
                                    struct t_config_file *config_file,
                                    struct t_config_section *section,
                                    const char *option_name, const char *value)
{
    struct t_config_option *ptr_option;
    int rc;
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
                
                ptr_option = weechat_config_new_option (
                    config_file, section,
                    option_name, "integer",
                    _("buffer used to display message received from IRC "
                      "server"),
                    "weechat|current|private", 0, 0, value, value, 0,
                    NULL, NULL, NULL, NULL, NULL, NULL);
                rc = (ptr_option) ?
                    WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
            }
            else
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
    }
    
    if (rc == WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        weechat_printf (NULL,
                        _("%s%s: error creating \"%s\" => \"%s\""),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME,
                        option_name, value);
    }
    
    return rc;
}

/*
 * irc_config_ctcp_create_option: set a ctcp reply format
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
                    WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
            }
            else
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
    }
    
    if (rc == WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        weechat_printf (NULL,
                        _("%s%s: error creating CTCP \"%s\" => \"%s\""),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME,
                        option_name, value);
    }
    
    return rc;
}

/*
 * irc_config_ignore_read_cb: read ignore option from configuration file
 *                            return 1 if ok, 0 if error
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
 * irc_config_ignore_write_cb: write ignore section in configuration file
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
 * irc_config_server_write_default_cb: write default server section in
 *                                     configuration file
 */

int
irc_config_server_write_default_cb (void *data,
                                    struct t_config_file *config_file,
                                    const char *section_name)
{
    int i;
    char option_name[128];
    
    /* make C compiler happy */
    (void) data;
    
    if (!weechat_config_write_line (config_file, section_name, NULL))
        return WEECHAT_CONFIG_WRITE_ERROR;
    
    for (i = 0; i < IRC_SERVER_NUM_OPTIONS; i++)
    {
        snprintf (option_name, sizeof (option_name),
                  "freenode.%s",
                  irc_server_option_string[i]);
        switch (i)
        {
            case IRC_SERVER_OPTION_ADDRESSES:
                if (!weechat_config_write_line (config_file,
                                                option_name,
                                                "%s", "\"chat.freenode.net/6667\""))
                    return WEECHAT_CONFIG_WRITE_ERROR;
                break;
            default:
                if (!weechat_config_write_line (config_file,
                                                option_name,
                                                WEECHAT_CONFIG_OPTION_NULL))
                    return WEECHAT_CONFIG_WRITE_ERROR;
                break;
        }
    }
    
    return WEECHAT_CONFIG_WRITE_OK;
}

/*
 * irc_config_server_new_option: create a new option for a server
 */

struct t_config_option *
irc_config_server_new_option (struct t_config_file *config_file,
                              struct t_config_section *section,
                              int index_option,
                              const char *option_name,
                              const char *default_value,
                              const char *value,
                              int null_value_allowed,
                              void *callback_change,
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
                N_("list of hostname/port or IP/port for server (separated by comma)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_PROXY:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("proxy used for this server (optional)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_IPV6:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "boolean",
                N_("use IPv6 protocol for server communication"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
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
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_SSL_CERT:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("ssl certificate file used to automatically identify your nick"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_SSL_DHKEY_SIZE:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("size of the key used during the Diffie-Hellman Key Exchange"),
                NULL, 0, INT_MAX,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_SSL_VERIFY:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "boolean",
                N_("check that the ssl connection is fully trusted"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_PASSWORD:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("password for server"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_SASL_MECHANISM:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("mechanism for SASL authentication"),
                "plain|dh-blowfish", 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_SASL_USERNAME:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("username for SASL authentication"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_SASL_PASSWORD:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("password for SASL authentication"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
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
                NULL, NULL,
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
                NULL, NULL,
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
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_AUTORECONNECT_DELAY:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("delay (in seconds) before trying again to reconnect to server"),
                NULL, 1, 65535,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_NICKS:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("nicknames to use on server (separated by comma)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_USERNAME:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("user name to use on server"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_REALNAME:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("real name to use on server"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
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
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_COMMAND:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("command(s) to run when connected to server (many commands "
                   "should be separated by \";\", use \"\\;\" for a semicolon, "
                   "special variables $nick, $channel and $server are replaced "
                   "by their value)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_COMMAND_DELAY:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("delay (in seconds) after command was executed (example: give some "
                   "time for authentication)"),
                NULL, 0, 3600,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_AUTOJOIN:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("comma separated list of channels to join when connected to server "
                   "(example: \"#chan1,#chan2,#chan3 key1,key2\")"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_OPTION_AUTOREJOIN:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "boolean",
                N_("automatically rejoin channels after kick"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
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
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case IRC_SERVER_NUM_OPTIONS:
            break;
    }
    
    return new_option;
}

/*
 * irc_config_server_read_cb: read server option in configuration file
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
                                weechat_config_option_set (ptr_server->options[i],
                                                           NULL, 1);
                            }
                            ptr_server->reloaded_from_config = 1;
                        }
                        rc = weechat_config_option_set (ptr_server->options[index_option],
                                                        value, 1);
                    }
                    else
                    {
                        weechat_printf (NULL,
                                        _("%s%s: error creating server "
                                          "\"%s\""),
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
        weechat_printf (NULL,
                        _("%s%s: error creating server option \"%s\""),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME,
                        option_name);
    }
    
    return rc;
}

/*
 * irc_config_server_write_cb: write server section in configuration file
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
 * irc_config_server_create_default_options: create default options for servers
 */

void
irc_config_server_create_default_options (struct t_config_section *section)
{
    int i, length;
    char *nicks, *username, *realname, *pos, *default_value;
    struct passwd *my_passwd;
    
    nicks = NULL;
    username = NULL;
    realname = NULL;
    
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
        if ((!my_passwd->pw_gecos)
            || (my_passwd->pw_gecos[0] == '\0')
            || (my_passwd->pw_gecos[0] == ',')
            || (my_passwd->pw_gecos[0] == ' '))
            realname = strdup (my_passwd->pw_name);
        else
        {
            realname = strdup (my_passwd->pw_gecos);
            pos = strchr (realname, ',');
            if (pos)
                pos[0] = '\0';
        }
    }
    else
    {
        /* default values if /etc/passwd can't be read */
        nicks = strdup (IRC_SERVER_DEFAULT_NICKS);
        username = strdup ("weechat");
        realname = strdup ("weechat");
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
            default_value = irc_server_option_default[i];
        
        irc_config_server_default[i] = irc_config_server_new_option (
            irc_config_file,
            section,
            i,
            irc_server_option_string[i],
            irc_server_option_default[i],
            default_value,
            0,
            &irc_config_server_default_change_cb,
            irc_server_option_string[i]);
    }
    
    if (nicks)
        free (nicks);
    if (username)
        free (username);
    if (realname)
        free (realname);
}

/*
 * irc_config_init: init IRC configuration file
 *                  return: 1 if ok, 0 if error
 */

int
irc_config_init ()
{
    struct t_config_section *ptr_section;
    
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
    irc_config_look_server_buffer = weechat_config_new_option (
        irc_config_file, ptr_section,
        "server_buffer", "integer",
        N_("merge server buffers"),
        "merge_with_core|merge_without_core|independent", 0, 0, "merge_with_core",
        NULL, 0, NULL, NULL,
        &irc_config_change_look_server_buffer, NULL, NULL, NULL);
    irc_config_look_open_channel_near_server = weechat_config_new_option (
        irc_config_file, ptr_section,
        "open_channel_near_server", "boolean",
        N_("open new channels near server"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_open_pv_near_server = weechat_config_new_option (
        irc_config_file, ptr_section,
        "open_pv_near_server", "boolean",
        N_("open new privates near server"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_nick_prefix = weechat_config_new_option (
        irc_config_file, ptr_section,
        "nick_prefix", "string",
        N_("text to display before nick in chat window"),
        NULL, 0, 0, "", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_nick_suffix = weechat_config_new_option (
        irc_config_file, ptr_section,
        "nick_suffix", "string",
        N_("text to display after nick in chat window"),
        NULL, 0, 0, "", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_nick_completion_smart = weechat_config_new_option (
        irc_config_file, ptr_section,
        "nick_completion_smart", "integer",
        N_("smart completion for nicks (completes first with last speakers)"),
        "off|speakers|speakers_highlights", 0, 0, "speakers", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_display_away = weechat_config_new_option (
        irc_config_file, ptr_section,
        "display_away", "integer",
        N_("display message when (un)marking as away"),
        "off|local|channel", 0, 0, "local", NULL, 0, NULL, NULL, NULL, NULL,
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
    irc_config_look_item_channel_modes = weechat_config_new_option (
        irc_config_file, ptr_section,
        "item_channel_modes", "boolean",
        N_("display channel modes in \"buffer_name\" bar item"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        &irc_config_change_look_item_channel_modes, NULL, NULL, NULL);
    irc_config_look_item_channel_modes_hide_key = weechat_config_new_option (
        irc_config_file, ptr_section,
        "item_channel_modes_hide_key", "boolean",
        N_("hide channel key if modes are displayed in \"buffer_name\" bar "
           "item (this will hide all channel modes arguments if mode +k is "
           "set on channel)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL,
        &irc_config_change_look_item_channel_modes_hide_key, NULL, NULL, NULL);
    irc_config_look_item_nick_modes = weechat_config_new_option (
        irc_config_file, ptr_section,
        "item_nick_modes", "boolean",
        N_("display nick modes in \"input_prompt\" bar item"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        &irc_config_change_look_item_nick_modes, NULL, NULL, NULL);
    irc_config_look_item_nick_prefix = weechat_config_new_option (
        irc_config_file, ptr_section,
        "item_nick_prefix", "boolean",
        N_("display nick prefix in \"input_prompt\" bar item"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        &irc_config_change_look_item_nick_prefix, NULL, NULL, NULL);
    irc_config_look_hide_nickserv_pwd = weechat_config_new_option (
        irc_config_file, ptr_section,
        "hide_nickserv_pwd", "boolean",
        N_("hide password displayed by nickserv"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_highlight_tags = weechat_config_new_option (
        irc_config_file, ptr_section,
        "highlight_tags", "string",
        N_("comma separated list of tags for messages that may produce "
           "highlight (usually any message from another user, not server "
           "messages,..)"),
        NULL, 0, 0, "irc_privmsg,irc_notice", NULL, 0, NULL, NULL,
        &irc_config_change_look_highlight_tags, NULL, NULL, NULL);
    irc_config_look_item_display_server = weechat_config_new_option (
        irc_config_file, ptr_section,
        "item_display_server", "integer",
        N_("name of bar item where IRC server is displayed (for status bar)"),
        "buffer_plugin|buffer_name", 0, 0, "buffer_plugin", NULL, 0, NULL, NULL,
        &irc_config_change_look_item_display_server, NULL, NULL, NULL);
    irc_config_look_msgbuffer_fallback = weechat_config_new_option (
        irc_config_file, ptr_section,
        "msgbuffer_fallback", "integer",
        N_("default target buffer for msgbuffer options when target is "
           "private and that private buffer is not found"),
        "current|server", 0, 0, "current", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_raw_messages = weechat_config_new_option (
        irc_config_file, ptr_section,
        "raw_messages", "integer",
        N_("number of IRC raw messages to save in memory when raw data buffer "
           "is closed (messages will be displayed when opening raw data buffer)"),
        NULL, 0, 65535, "256", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_smart_filter = weechat_config_new_option (
        irc_config_file, ptr_section,
        "smart_filter", "boolean",
        N_("filter join/part/quit messages for a nick if not speaking for "
           "some minutes on channel (you must create a filter on tag "
           "\"irc_smart_filter\")"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_smart_filter_delay = weechat_config_new_option (
        irc_config_file, ptr_section,
        "smart_filter_delay", "integer",
        N_("delay for filtering join/part/quit messages (in minutes)"),
        NULL, 1, 60*24*7, "5", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_smart_filter_join = weechat_config_new_option (
        irc_config_file, ptr_section,
        "smart_filter_join", "boolean",
        N_("enable smart filter for \"join\" messages"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_smart_filter_quit = weechat_config_new_option (
        irc_config_file, ptr_section,
        "smart_filter_quit", "boolean",
        N_("enable smart filter for \"part\" and \"quit\" messages"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_notice_as_pv = weechat_config_new_option (
        irc_config_file, ptr_section,
        "notice_as_pv", "integer",
        N_("display notices as private messages (if auto, use private buffer "
           "if found)"),
        "auto|never|always", 0, 0, "auto", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_part_closes_buffer = weechat_config_new_option (
        irc_config_file, ptr_section,
        "part_closes_buffer", "boolean",
        N_("close buffer when /part is issued on a channel"),
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
    irc_config_color_notice = weechat_config_new_option (
        irc_config_file, ptr_section,
        "notice", "color",
        N_("color for text \"Notice\" in notices"),
        NULL, -1, 0, "green", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_color_input_nick = weechat_config_new_option (
        irc_config_file, ptr_section,
        "input_nick", "color",
        N_("color for nick in input bar"),
        NULL, -1, 0, "lightcyan", NULL, 0, NULL, NULL,
        &irc_config_change_color_input_nick, NULL, NULL, NULL);
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
        &irc_config_change_color_item_buffer_name, NULL, NULL, NULL);
    
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
        NULL, 0, 3600 * 24, "1800", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_network_connection_timeout = weechat_config_new_option (
        irc_config_file, ptr_section,
        "connection_timeout", "integer",
        N_("timeout (in seconds) between TCP connection to server and message "
           "001 received, if this timeout is reached before 001 message is "
           "received, WeeChat will disconnect from server"),
        NULL, 1, 3600, "60", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_network_default_msg_part = weechat_config_new_option (
        irc_config_file, ptr_section,
        "default_msg_part", "string",
        N_("default part message (leaving channel) (\"%v\" will be replaced "
           "by WeeChat version in string)"),
        NULL, 0, 0, "WeeChat %v", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_network_default_msg_quit = weechat_config_new_option (
        irc_config_file, ptr_section,
        "default_msg_quit", "string",
        N_("default quit message (disconnecting from server) (\"%v\" will be "
           "replaced by WeeChat version in string)"),
        NULL, 0, 0, "WeeChat %v", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_network_away_check = weechat_config_new_option (
        irc_config_file, ptr_section,
        "away_check", "integer",
        N_("interval between two checks for away (in minutes, 0 = never "
           "check)"),
        NULL, 0, INT_MAX, "0", NULL, 0, NULL, NULL,
        &irc_config_change_network_away_check, NULL, NULL, NULL);
    irc_config_network_away_check_max_nicks = weechat_config_new_option (
        irc_config_file, ptr_section,
        "away_check_max_nicks", "integer",
        N_("do not check away nicks on channels with high number of nicks "
           "(0 = unlimited)"),
        NULL, 0, INT_MAX, "25", NULL, 0, NULL, NULL,
        &irc_config_change_network_away_check, NULL, NULL, NULL);
    irc_config_network_lag_check = weechat_config_new_option (
        irc_config_file, ptr_section,
        "lag_check", "integer",
        N_("interval between two checks for lag (in seconds, 0 = never "
           "check)"),
        NULL, 0, INT_MAX, "60", NULL, 0, NULL, NULL,
        &irc_config_change_network_lag_check, NULL, NULL, NULL);
    irc_config_network_lag_min_show = weechat_config_new_option (
        irc_config_file, ptr_section,
        "lag_min_show", "integer",
        N_("minimum lag to show (in seconds)"),
        NULL, 0, INT_MAX, "1", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_network_lag_disconnect = weechat_config_new_option (
        irc_config_file, ptr_section,
        "lag_disconnect", "integer",
        N_("disconnect after important lag (in minutes, 0 = never "
           "disconnect)"),
        NULL, 0, INT_MAX, "0", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_network_anti_flood[0] = weechat_config_new_option (
        irc_config_file, ptr_section,
        "anti_flood_prio_high", "integer",
        N_("anti-flood for high priority queue: number of seconds between two "
           "user messages or commands sent to IRC server (0 = no anti-flood)"),
        NULL, 0, 60, "2", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_network_anti_flood[1] = weechat_config_new_option (
        irc_config_file, ptr_section,
        "anti_flood_prio_low", "integer",
        N_("anti-flood for low priority queue: number of seconds between two "
           "messages sent to IRC server (messages like automatic CTCP replies) "
           "(0 = no anti-flood)"),
        NULL, 0, 60, "2", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
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
           "u=underline, r=reverse)"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_network_send_unknown_commands = weechat_config_new_option (
        irc_config_file, ptr_section,
        "send_unknown_commands", "boolean",
        N_("send unknown commands to server"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL,
        &irc_config_change_network_send_unknown_commands, NULL, NULL, NULL);
    
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
                                              &irc_config_server_write_default_cb, NULL,
                                              NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (irc_config_file);
        return 0;
    }
    irc_config_section_server = ptr_section;
    
    hook_config_color_nicks_number = weechat_hook_config ("weechat.look.color_nicks_number",
                                                          &irc_config_change_look_color_nicks_number, NULL);
    
    return 1;
}

/*
 * irc_config_read: read IRC configuration file
 */

int
irc_config_read ()
{
    int rc;
    
    rc = weechat_config_read (irc_config_file);
    if (rc == WEECHAT_CONFIG_READ_OK)
    {
        irc_config_change_network_away_check (NULL, NULL);
    }
    
    return rc;
}

/*
 * irc_config_write: write IRC configuration file
 */

int
irc_config_write (int write_temp_servers)
{
    irc_config_write_temp_servers = write_temp_servers;
    
    return weechat_config_write (irc_config_file);
}

/*
 * irc_config_free: free IRC configuration
 */

void
irc_config_free ()
{
    weechat_config_free (irc_config_file);

    if (hook_config_color_nicks_number)
    {
        weechat_unhook (hook_config_color_nicks_number);
        hook_config_color_nicks_number = NULL;
    }
}
