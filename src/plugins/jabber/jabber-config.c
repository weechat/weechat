/*
 * Copyright (c) 2009 by FlashCode <flashcode@flashtux.org>
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

/* jabber-config.c: Jabber configuration options */


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <pwd.h>

#include "../weechat-plugin.h"
#include "jabber.h"
#include "jabber-config.h"
#include "jabber-buffer.h"
#include "jabber-buddy.h"
#include "jabber-server.h"
#include "jabber-muc.h"


struct t_config_file *jabber_config_file = NULL;
struct t_config_section *jabber_config_section_server_default = NULL;
struct t_config_section *jabber_config_section_server = NULL;

/* Jabber config, look section */

struct t_config_option *jabber_config_look_color_nicks_in_server_messages;
struct t_config_option *jabber_config_look_one_server_buffer;
struct t_config_option *jabber_config_look_open_near_server;
struct t_config_option *jabber_config_look_nick_prefix;
struct t_config_option *jabber_config_look_nick_suffix;
struct t_config_option *jabber_config_look_nick_completion_smart;
struct t_config_option *jabber_config_look_display_away;
struct t_config_option *jabber_config_look_display_muc_modes;
struct t_config_option *jabber_config_look_highlight_tags;
struct t_config_option *jabber_config_look_show_away_once;
struct t_config_option *jabber_config_look_smart_filter;
struct t_config_option *jabber_config_look_smart_filter_delay;

/* Jabber config, color section */

struct t_config_option *jabber_config_color_message_join;
struct t_config_option *jabber_config_color_message_quit;
struct t_config_option *jabber_config_color_input_nick;

/* Jabber config, network section */

struct t_config_option *jabber_config_network_default_msg_part;
struct t_config_option *jabber_config_network_default_msg_quit;
struct t_config_option *jabber_config_network_lag_check;
struct t_config_option *jabber_config_network_lag_min_show;
struct t_config_option *jabber_config_network_lag_disconnect;
struct t_config_option *jabber_config_network_anti_flood;
struct t_config_option *jabber_config_network_colors_receive;
struct t_config_option *jabber_config_network_colors_send;

/* Jabber config, server section */

struct t_config_option *jabber_config_server_default[JABBER_SERVER_NUM_OPTIONS];

struct t_hook *hook_config_color_nicks_number = NULL;

int jabber_config_write_temp_servers = 0;


struct t_jabber_server *
jabber_config_get_server_from_option_name (const char *name)
{
    struct t_jabber_server *ptr_server;
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
                ptr_server = jabber_server_search (server_name);
                free (server_name);
            }
        }
    }
    
    return ptr_server;
}

/*
 * jabber_config_change_look_color_nicks_number: called when the
 *                                               "weechat.look.color_nicks_number"
 *                                               option is changed
 */

int
jabber_config_change_look_color_nicks_number (void *data, const char *option,
                                              const char *value)
{
    struct t_jabber_server *ptr_server;
    struct t_jabber_muc *ptr_muc;
    struct t_jabber_buddy *ptr_buddy;
    
    /* make C compiler happy */
    (void) data;
    (void) option;
    (void) value;
    
    for (ptr_server = jabber_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        /* buddies in roster */
        for (ptr_buddy = ptr_server->buddies; ptr_buddy;
             ptr_buddy = ptr_buddy->next_buddy)
        {
            ptr_buddy->color = jabber_buddy_find_color (ptr_buddy);
        }
        /* buddies in MUCs */
        for (ptr_muc = ptr_server->mucs; ptr_muc; ptr_muc = ptr_muc->next_muc)
        {
            for (ptr_buddy = ptr_muc->buddies; ptr_buddy;
                 ptr_buddy = ptr_buddy->next_buddy)
            {
                ptr_buddy->color = jabber_buddy_find_color (ptr_buddy);
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_config_change_look_one_server_buffer: called when the "one server buffer"
 *                                              option is changed
 */

void
jabber_config_change_look_one_server_buffer (void *data,
                                             struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    if (weechat_config_boolean (jabber_config_look_one_server_buffer))
        jabber_buffer_merge_servers ();
    else
        jabber_buffer_split_server ();
}

/*
 * jabber_config_change_look_display_muc_modes: called when the "display
 *                                              MUC modes" option is changed
 */

void
jabber_config_change_look_display_muc_modes (void *data,
                                             struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    weechat_bar_item_update ("buffer_name");
}

/*
 * jabber_config_change_look_highlight_tags: called when the "highlight tags"
 *                                           option is changed
 */

void
jabber_config_change_look_highlight_tags (void *data,
                                          struct t_config_option *option)
{
    struct t_jabber_server *ptr_server;
    struct t_jabber_muc *ptr_muc;
    
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    for (ptr_server = jabber_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->buffer)
        {
            weechat_buffer_set (ptr_server->buffer, "highlight_tags",
                                weechat_config_string (jabber_config_look_highlight_tags));
        }
        for (ptr_muc = ptr_server->mucs; ptr_muc; ptr_muc = ptr_muc->next_muc)
        {
            if (ptr_muc->buffer)
            {
                weechat_buffer_set (ptr_muc->buffer, "highlight_tags",
                                    weechat_config_string (jabber_config_look_highlight_tags));
            }
        }
    }
}

/*
 * jabber_config_change_color_input_nick: called when the color of input nick
 *                                        ischanged
 */

void
jabber_config_change_color_input_nick (void *data,
                                       struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    weechat_bar_item_update ("input_prompt");
}

/*
 * jabber_config_server_default_change_cb: callback called when a default server
 *                                         option is modified
 */

void
jabber_config_server_default_change_cb (void *data,
                                        struct t_config_option *option)
{
    int index_option;
    struct t_jabber_server *ptr_server;
    
    index_option = jabber_server_search_option (data);
    if (index_option >= 0)
    {
        for (ptr_server = jabber_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (weechat_config_option_is_null (ptr_server->options[index_option]))
            {
                switch (index_option)
                {
                    case JABBER_SERVER_OPTION_SERVER:
                        jabber_server_set_server (ptr_server,
                                                  weechat_config_string (option));
                        break;
                }
            }
        }
    }
}

/*
 * jabber_config_server_change_cb: callback called when a server option is
 *                                 modified
 */

void
jabber_config_server_change_cb (void *data, struct t_config_option *option)
{
    int index_option;
    char *name;
    struct t_jabber_server *ptr_server;
    
    index_option = jabber_server_search_option (data);
    if (index_option >= 0)
    {
        name = weechat_config_option_get_pointer (option, "name");
        ptr_server = jabber_config_get_server_from_option_name (name);
        if (ptr_server)
        {
            switch (index_option)
            {
                case JABBER_SERVER_OPTION_SERVER:
                    jabber_server_set_server (ptr_server,
                                              JABBER_SERVER_OPTION_STRING(ptr_server,
                                                                          JABBER_SERVER_OPTION_SERVER));
                    break;
            }
        }
    }
}

/*
 * jabber_config_reload: reload Jabber configuration file
 */

int
jabber_config_reload (void *data, struct t_config_file *config_file)
{
    int rc;
    struct t_jabber_server *ptr_server, *next_server;
    
    /* make C compiler happy */
    (void) data;
    
    for (ptr_server = jabber_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        ptr_server->reloading_from_config = 1;
        ptr_server->reloaded_from_config = 0;
    }
    
    rc = weechat_config_reload (config_file);
    
    ptr_server = jabber_servers;
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
                                weechat_prefix ("error"), JABBER_PLUGIN_NAME,
                                ptr_server->name);
            }
            else
                jabber_server_free (ptr_server);
        }
        
        ptr_server = next_server;
    }
    
    return rc;
}

/*
 * jabber_config_server_new_option: create a new option for a server
 */

struct t_config_option *
jabber_config_server_new_option (struct t_config_file *config_file,
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
        case JABBER_SERVER_OPTION_USERNAME:
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
        case JABBER_SERVER_OPTION_SERVER:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("hostname/port or IP/port for server"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case JABBER_SERVER_OPTION_PROXY:
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
        case JABBER_SERVER_OPTION_IPV6:
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
        case JABBER_SERVER_OPTION_TLS:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "boolean",
                N_("use TLS cryptographic protocol for server communication"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case JABBER_SERVER_OPTION_SASL:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "boolean",
                N_("use SASL for authentication"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case JABBER_SERVER_OPTION_RESOURCE:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("resource (for example: Home or Work)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case JABBER_SERVER_OPTION_PASSWORD:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("password"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case JABBER_SERVER_OPTION_LOCAL_ALIAS:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("local alias"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case JABBER_SERVER_OPTION_AUTOCONNECT:
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
        case JABBER_SERVER_OPTION_AUTORECONNECT:
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
        case JABBER_SERVER_OPTION_AUTORECONNECT_DELAY:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("delay (in seconds) before trying again to reconnect to "
                   "server"),
                NULL, 0, 65535,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case JABBER_SERVER_OPTION_LOCAL_HOSTNAME:
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
        case JABBER_SERVER_OPTION_COMMAND:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("command(s) to run when connected to server (many commands "
                   "should be separated by ';', use '\\;' for a semicolon, "
                   "special variables $nick, $muc and $server are "
                   "replaced by their value)"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case JABBER_SERVER_OPTION_COMMAND_DELAY:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("delay (in seconds) after command was executed (example: "
                   "give some time for authentication)"),
                NULL, 0, 3600,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case JABBER_SERVER_OPTION_AUTOJOIN:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("comma separated list of MUCs to join when connected "
                   "to server (example: \"#chan1,#chan2,#chan3 key1,key2\")"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case JABBER_SERVER_OPTION_AUTOREJOIN:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "boolean",
                N_("automatically rejoin MUCs when kicked"),
                NULL, 0, 0,
                default_value, value,
                null_value_allowed,
                NULL, NULL,
                callback_change, callback_change_data,
                NULL, NULL);
            break;
        case JABBER_SERVER_NUM_OPTIONS:
            break;
    }
    
    return new_option;
}

/*
 * jabber_config_server_read_cb: read server option in configuration file
 */

int
jabber_config_server_read_cb (void *data, struct t_config_file *config_file,
                              struct t_config_section *section,
                              const char *option_name, const char *value)
{
    struct t_jabber_server *ptr_server;
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
                index_option = jabber_server_search_option (pos_option);
                if (index_option >= 0)
                {
                    ptr_server = jabber_server_search (server_name);
                    if (!ptr_server)
                        ptr_server = jabber_server_alloc (server_name);
                    if (ptr_server)
                    {
                        if (ptr_server->reloading_from_config
                            && !ptr_server->reloaded_from_config)
                        {
                            for (i = 0; i < JABBER_SERVER_NUM_OPTIONS; i++)
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
                                        weechat_prefix ("error"), JABBER_PLUGIN_NAME,
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
                        weechat_prefix ("error"), JABBER_PLUGIN_NAME,
                        option_name);
    }
    
    return rc;
}

/*
 * jabber_config_server_write_cb: write server section in configuration file
 */

void
jabber_config_server_write_cb (void *data, struct t_config_file *config_file,
                               const char *section_name)
{
    struct t_jabber_server *ptr_server;
    int i;
    
    /* make C compiler happy */
    (void) data;
    
    weechat_config_write_line (config_file, section_name, NULL);
    
    for (ptr_server = jabber_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (!ptr_server->temp_server || jabber_config_write_temp_servers)
        {
            for (i = 0; i < JABBER_SERVER_NUM_OPTIONS; i++)
            {
                weechat_config_write_option (config_file,
                                             ptr_server->options[i]);
            }
        }
    }
}

/*
 * jabber_config_server_create_default_options: create default options for
 *                                              servers
 */

void
jabber_config_server_create_default_options (struct t_config_section *section)
{
    int i;
    
    for (i = 0; i < JABBER_SERVER_NUM_OPTIONS; i++)
    {        
        jabber_config_server_default[i] = jabber_config_server_new_option (
            jabber_config_file,
            section,
            i,
            jabber_server_option_string[i],
            jabber_server_option_default[i],
            jabber_server_option_default[i],
            0,
            &jabber_config_server_default_change_cb,
            jabber_server_option_string[i]);
    }
}

/*
 * jabber_config_init: init Jabber configuration file
 *                     return: 1 if ok, 0 if error
 */

int
jabber_config_init ()
{
    struct t_config_section *ptr_section;
    
    jabber_config_file = weechat_config_new (JABBER_CONFIG_NAME,
                                             &jabber_config_reload, NULL);
    if (!jabber_config_file)
        return 0;
    
    /* look */
    ptr_section = weechat_config_new_section (jabber_config_file, "look",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (jabber_config_file);
        return 0;
    }
    
    jabber_config_look_color_nicks_in_server_messages = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "color_nicks_in_server_messages", "boolean",
        N_("use nick color in messages from server"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    jabber_config_look_one_server_buffer = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "one_server_buffer", "boolean",
        N_("use same buffer for all servers"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL,
        &jabber_config_change_look_one_server_buffer, NULL, NULL, NULL);
    jabber_config_look_open_near_server = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "open_near_server", "boolean",
        N_("open new MUCs/privates near server"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    jabber_config_look_nick_prefix = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "nick_prefix", "string",
        N_("text to display before nick in chat window"),
        NULL, 0, 0, "", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    jabber_config_look_nick_suffix = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "nick_suffix", "string",
        N_("text to display after nick in chat window"),
        NULL, 0, 0, "", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    jabber_config_look_nick_completion_smart = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "nick_completion_smart", "boolean",
        N_("smart completion for nicks (completes with last speakers first)"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    jabber_config_look_display_away = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "display_away", "integer",
        N_("display message when (un)marking as away"),
        "off|local|muc", 0, 0, "local", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    jabber_config_look_display_muc_modes = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "display_muc_modes", "boolean",
        N_("display MUC modes in \"buffer_name\" bar item"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        &jabber_config_change_look_display_muc_modes, NULL, NULL, NULL);
    jabber_config_look_highlight_tags = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "highlight_tags", "string",
        N_("comma separated list of tags for messages that may produce "
           "highlight (usually any message from another user, not server "
           "messages,..)"),
        NULL, 0, 0, "jabber_chat_msg,jabber_notice", NULL, 0, NULL, NULL,
        &jabber_config_change_look_highlight_tags, NULL, NULL, NULL);
    jabber_config_look_show_away_once = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "show_away_once", "boolean",
        N_("show remote away message only once in private"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    jabber_config_look_smart_filter = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "smart_filter", "boolean",
        N_("filter join/part/quit messages for a nick if not speaking for "
           "some minutes on MUC (you must create a filter on tag "
           "\"jabber_smart_filter\")"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    jabber_config_look_smart_filter_delay = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "smart_filter_delay", "integer",
        N_("delay for filtering join/part/quit messages (in minutes)"),
        NULL, 1, 60*24*7, "5", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    
    /* color */
    ptr_section = weechat_config_new_section (jabber_config_file, "color",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (jabber_config_file);
        return 0;
    }
    
    jabber_config_color_message_join = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "message_join", "color",
        N_("color for text in join messages"),
        NULL, -1, 0, "green", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    jabber_config_color_message_quit = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "message_quit", "color",
        N_("color for text in part/quit messages"),
        NULL, -1, 0, "red", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);
    jabber_config_color_input_nick = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "input_nick", "color",
        N_("color for nick in input bar"),
        NULL, -1, 0, "lightcyan", NULL, 0, NULL, NULL,
        &jabber_config_change_color_input_nick, NULL, NULL, NULL);
    
    /* network */
    ptr_section = weechat_config_new_section (jabber_config_file, "network",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (jabber_config_file);
        return 0;
    }
    
    jabber_config_network_default_msg_part = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "default_msg_part", "string",
        N_("default part message (leaving MUC) ('%v' will be replaced by "
           "WeeChat version in string)"),
        NULL, 0, 0, "WeeChat %v", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    jabber_config_network_default_msg_quit = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "default_msg_quit", "string",
        N_("default quit message (disconnecting from server) ('%v' will be "
           "replaced by WeeChat version in string)"),
        NULL, 0, 0, "WeeChat %v", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    jabber_config_network_lag_check = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "lag_check", "integer",
        N_("interval between two checks for lag (in seconds, 0 = never "
           "check)"),
        NULL, 0, INT_MAX, "60", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    jabber_config_network_lag_min_show = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "lag_min_show", "integer",
        N_("minimum lag to show (in seconds)"),
        NULL, 0, INT_MAX, "1", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    jabber_config_network_lag_disconnect = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "lag_disconnect", "integer",
        N_("disconnect after important lag (in minutes, 0 = never "
           "disconnect)"),
        NULL, 0, INT_MAX, "5", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    jabber_config_network_anti_flood = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "anti_flood", "integer",
        N_("anti-flood: # seconds between two user messages (0 = no "
           "anti-flood)"),
        NULL, 0, 5, "2", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    jabber_config_network_colors_receive = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "colors_receive", "boolean",
        N_("when off, colors codes are ignored in incoming messages"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    jabber_config_network_colors_send = weechat_config_new_option (
        jabber_config_file, ptr_section,
        "colors_send", "boolean",
        N_("allow user to send colors with special codes (^Cb=bold, "
           "^Ccxx=color, ^Ccxx,yy=color+background, ^Cu=underline, "
           "^Cr=reverse)"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    
    /* server_default */
    ptr_section = weechat_config_new_section (jabber_config_file, "server_default",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (jabber_config_file);
        return 0;
    }
    
    jabber_config_section_server_default = ptr_section;
    
    jabber_config_server_create_default_options (ptr_section);
    
    /* server */
    ptr_section = weechat_config_new_section (jabber_config_file, "server",
                                              0, 0,
                                              &jabber_config_server_read_cb, NULL,
                                              &jabber_config_server_write_cb, NULL,
                                              NULL, NULL,
                                              NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (jabber_config_file);
        return 0;
    }
    
    jabber_config_section_server = ptr_section;
    
    hook_config_color_nicks_number = weechat_hook_config ("weechat.look.color_nicks_number",
                                                          &jabber_config_change_look_color_nicks_number, NULL);
    
    return 1;
}

/*
 * jabber_config_read: read Jabber configuration file
 */

int
jabber_config_read ()
{
    return weechat_config_read (jabber_config_file);
}

/*
 * jabber_config_write: write Jabber configuration file
 */

int
jabber_config_write (int write_temp_servers)
{
    jabber_config_write_temp_servers = write_temp_servers;
    
    return weechat_config_write (jabber_config_file);
}

/*
 * jabber_config_free: free Jabber configuration
 */

void
jabber_config_free ()
{
    weechat_config_free (jabber_config_file);

    if (hook_config_color_nicks_number)
    {
        weechat_unhook (hook_config_color_nicks_number);
        hook_config_color_nicks_number = NULL;
    }
}
