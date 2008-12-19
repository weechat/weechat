/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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

/* irc-config.c: IRC configuration options */


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <pwd.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-config.h"
#include "irc-buffer.h"
#include "irc-ignore.h"
#include "irc-nick.h"
#include "irc-server.h"
#include "irc-channel.h"


char *irc_config_server_option_string[IRC_CONFIG_NUM_SERVER_OPTIONS] =
{ "autoconnect", "autoreconnect", "autoreconnect_delay", "proxy", "addresses",
  "ipv6", "ssl", "password", "nicks", "username", "realname", "local_hostname",
  "command", "command_delay", "autojoin", "autorejoin"
};
char *irc_config_server_option_default[IRC_CONFIG_NUM_SERVER_OPTIONS] =
{ "off", "on", "30", "", "", "off", "off", "", "", "", "", "", "", "0", "",
  "off", ""
};

struct t_config_file *irc_config_file = NULL;
struct t_config_section *irc_config_section_server_default = NULL;
struct t_config_section *irc_config_section_server = NULL;

/* IRC config, look section */

struct t_config_option *irc_config_look_color_nicks_in_server_messages;
struct t_config_option *irc_config_look_one_server_buffer;
struct t_config_option *irc_config_look_open_near_server;
struct t_config_option *irc_config_look_nick_prefix;
struct t_config_option *irc_config_look_nick_suffix;
struct t_config_option *irc_config_look_nick_completion_smart;
struct t_config_option *irc_config_look_display_away;
struct t_config_option *irc_config_look_display_channel_modes;
struct t_config_option *irc_config_look_hide_nickserv_pwd;
struct t_config_option *irc_config_look_highlight_tags;
struct t_config_option *irc_config_look_show_away_once;
struct t_config_option *irc_config_look_smart_filter;
struct t_config_option *irc_config_look_smart_filter_delay;
struct t_config_option *irc_config_look_notice_as_pv;

/* IRC config, color section */

struct t_config_option *irc_config_color_message_join;
struct t_config_option *irc_config_color_message_quit;
struct t_config_option *irc_config_color_input_nick;

/* IRC config, network section */

struct t_config_option *irc_config_network_default_msg_part;
struct t_config_option *irc_config_network_default_msg_quit;
struct t_config_option *irc_config_network_away_check;
struct t_config_option *irc_config_network_away_check_max_nicks;
struct t_config_option *irc_config_network_lag_check;
struct t_config_option *irc_config_network_lag_min_show;
struct t_config_option *irc_config_network_lag_disconnect;
struct t_config_option *irc_config_network_anti_flood;
struct t_config_option *irc_config_network_colors_receive;
struct t_config_option *irc_config_network_colors_send;
struct t_config_option *irc_config_network_send_unknown_commands;

/* IRC config, server section */

struct t_config_option *irc_config_server_default[IRC_CONFIG_NUM_SERVER_OPTIONS];

struct t_hook *hook_config_color_nicks_number = NULL;


/*
 * irc_config_search_server_option: search a server option name
 *                                  return index of option in array
 *                                  "irc_config_server_option_str", or -1 if
 *                                  not found
 */

int
irc_config_search_server_option (const char *option_name)
{
    int i;
    
    if (!option_name)
        return -1;
    
    for (i = 0; i < IRC_CONFIG_NUM_SERVER_OPTIONS; i++)
    {
        if (weechat_strcasecmp (irc_config_server_option_string[i],
                                option_name) == 0)
            return i;
    }
    
    /* server option not found */
    return -1;
}

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
                ptr_nick->color = irc_nick_find_color (ptr_nick);
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_config_change_look_one_server_buffer: called when the "one server buffer"
 *                                           option is changed
 */

void
irc_config_change_look_one_server_buffer (void *data,
                                          struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    if (weechat_config_boolean (irc_config_look_one_server_buffer))
        irc_buffer_merge_servers ();
    else
        irc_buffer_split_server ();
}

/*
 * irc_config_change_look_display_channel_modes: called when the "display
 *                                               channel modes" option is
 *                                               changed
 */

void
irc_config_change_look_display_channel_modes (void *data,
                                              struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    weechat_bar_item_update ("buffer_name");
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
    
    if (weechat_config_boolean (irc_config_look_one_server_buffer))
    {
        if (irc_buffer_servers)
        {
            weechat_buffer_set (irc_buffer_servers,
                                "input_get_unknown_commands", value);
        }
    }
    else
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (ptr_server->buffer)
            {
                weechat_buffer_set (ptr_server->buffer,
                                    "input_get_unknown_commands", value);
            }
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
    int index_option, length;
    char *option_full_name;
    struct t_irc_server *ptr_server;
    struct t_config_option *ptr_option;
    
    index_option = irc_config_search_server_option (data);
    if (index_option >= 0)
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            length = strlen (ptr_server->name) + 1 + strlen (data) + 1;
            option_full_name = malloc (length);
            if (option_full_name)
            {
                ptr_option = weechat_config_search_option (irc_config_file,
                                                           irc_config_section_server,
                                                           option_full_name);
                if (!ptr_option)
                {
                    /* option does not exist for server, so we change value
                       with default value */
                    irc_server_set_with_option (ptr_server, index_option,
                                                option);
                }
                free (option_full_name);
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
    
    index_option = irc_config_search_server_option (data);
    if (index_option >= 0)
    {
        name = weechat_config_option_get_pointer (option, "name");
        ptr_server = irc_config_get_server_from_option_name (name);
        if (ptr_server)
        {
            irc_server_set_with_option (ptr_server, index_option, option);
        }
    }
}

/*
 * irc_config_server_delete_cb: callback called when a server option is deleted
 */

void
irc_config_server_delete_cb (void *data, struct t_config_option *option)
{
    int i, index_option, length;
    char *name, *mask;
    struct t_irc_server *ptr_server;
    struct t_infolist *infolist;
    
    index_option = irc_config_search_server_option (data);
    if (index_option >= 0)
    {
        name = weechat_config_option_get_pointer (option, "name");
        ptr_server = irc_config_get_server_from_option_name (name);
        if (ptr_server)
        {
            irc_server_set_with_option (ptr_server, index_option,
                                        irc_config_server_default[index_option]);
            
            /* look if we should remove server (no more option for server) */
            if (!ptr_server->is_connected)
            {
                length = strlen (ptr_server->name) + 64;
                mask = malloc (length);
                if (mask)
                {
                    snprintf (mask, length, "irc.server.%s.*",
                              ptr_server->name);
                    infolist = weechat_infolist_get ("option", NULL, mask);
                    i = 0;
                    while (weechat_infolist_next (infolist))
                    {
                        i++;
                    }
                    if (i <= 1)
                        irc_server_free (ptr_server);
                    weechat_infolist_free (infolist);
                    free (mask);
                }
            }
        }
    }
}

/*
 * irc_config_reload_servers_from_config: create/update servers from options
 *                                        read in config file
 */

void
irc_config_reload_servers_from_config ()
{
    struct t_infolist *infolist;
    struct t_irc_server *ptr_server;
    struct t_config_option *ptr_option;
    const char *full_name, *option_name;
    char *server_name, *pos_option;
    int i, index_option;
    
    infolist = weechat_infolist_get ("option", NULL, "irc.server.*");
    while (weechat_infolist_next (infolist))
    {
        full_name = weechat_infolist_string (infolist, "full_name");
        option_name = weechat_infolist_string (infolist, "option_name");
        if (full_name && option_name)
        {
            pos_option = strrchr (option_name, '.');
            if (pos_option)
            {
                server_name = weechat_strndup (option_name,
                                               pos_option - option_name);
                if (server_name)
                {
                    pos_option++;
                    ptr_server = irc_server_search (server_name);
                    if (!ptr_server)
                    {
                        /* create server, it's first time we see it */
                        ptr_server = irc_server_alloc (server_name);
                        if (!ptr_server)
                        {
                            weechat_printf (NULL,
                                            _("%s%s: error creating server "
                                              "\"%s\""),
                                            weechat_prefix ("error"),
                                            IRC_PLUGIN_NAME, server_name);
                        }
                    }
                    if (ptr_server)
                    {
                        index_option = irc_config_search_server_option (pos_option);
                        if (index_option >= 0)
                        {
                            if (!ptr_server->reloaded_from_config)
                            {
                                /* it's first time we see server, and we are
                                   reloading config, then initialize server
                                   with default values (will be overwritten
                                   by config later in this function
                                */
                                for (i = 0; i < IRC_CONFIG_NUM_SERVER_OPTIONS; i++)
                                {
                                    irc_server_set_with_option (ptr_server,
                                                                i,
                                                                irc_config_server_default[i]);
                                }
                                ptr_server->reloaded_from_config = 1;
                            }
                            weechat_config_search_with_string (full_name, NULL,
                                                               NULL, &ptr_option,
                                                               NULL);
                            if (ptr_option)
                            {
                                irc_server_set_with_option (ptr_server,
                                                            index_option,
                                                            ptr_option);
                            }
                        }
                    }
                    else
                    {
                        weechat_printf (NULL,
                                        _("%s%s: error creating option "
                                          "\"%s\" for server \"%s\" (server "
                                          "not found)"),
                                        weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                        pos_option, server_name);
                    }
                    free (server_name);
                }
            }
        }
    }
    weechat_infolist_free (infolist);
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
    
    weechat_config_section_free_options (irc_config_section_server);
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        ptr_server->reloaded_from_config = 0;
    }
    
    irc_ignore_free_all ();
    
    rc = weechat_config_reload (config_file);
    
    if (rc == WEECHAT_CONFIG_READ_OK)
        irc_config_reload_servers_from_config (1);
    
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
                /* TODO: create options for server in section (options that
                   are not default one */
                // ...
            }
            else
                irc_server_free (ptr_server);
        }
        
        ptr_server = next_server;
    }
    
    return rc;
}

/*
 * irc_config_ignore_read: read ignore option from config file
 *                         return 1 if ok, 0 if error
 */

int
irc_config_ignore_read (void *data,
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
            argv = weechat_string_explode (value, ";", 0, 0, &argc);
            argv_eol = weechat_string_explode (value, ";", 1, 0, NULL);
            if (argv && argv_eol && (argc >= 3))
            {
                irc_ignore_new (argv_eol[2], argv[0], argv[1]);
            }
            if (argv)
                weechat_string_free_exploded (argv);
            if (argv_eol)
                weechat_string_free_exploded (argv_eol);
        }
    }
    
    return 1;
}

/*
 * irc_config_ignore_write: write ignore section in configuration file
 */

void
irc_config_ignore_write (void *data, struct t_config_file *config_file,
                         const char *section_name)
{
    struct t_irc_ignore *ptr_ignore;
    
    /* make C compiler happy */
    (void) data;
    
    weechat_config_write_line (config_file, section_name, NULL);
    
    for (ptr_ignore = irc_ignore_list; ptr_ignore;
         ptr_ignore = ptr_ignore->next_ignore)
    {
        weechat_config_write_line (config_file,
                                   "ignore",
                                   "%s;%s;%s",
                                   (ptr_ignore->server) ? ptr_ignore->server : "*",
                                   (ptr_ignore->channel) ? ptr_ignore->channel : "*",
                                   ptr_ignore->mask);
    }
}

/*
 * irc_config_server_write_default: write default server section in configuration file
 */

void
irc_config_server_write_default (void *data, struct t_config_file *config_file,
                                 const char *section_name)
{
    /* make C compiler happy */
    (void) data;
    
    weechat_config_write_line (config_file, section_name, NULL);
    
    weechat_config_write_line (config_file, "freenode.addresses",
                               "%s", "\"chat.freenode.net/6667\"");
}

/*
 * irc_config_server_new_option: create a new option for a server
 */

struct t_config_option *
irc_config_server_new_option (struct t_config_file *config_file,
                              struct t_config_section *section,
                              int index_option,
                              const char *option_name,
                              const char *value,
                              void *callback_change,
                              void *callback_change_data,
                              void *callback_delete,
                              void *callback_delete_data)
{
    struct t_config_option *new_option;
    
    new_option = NULL;
    
    switch (index_option)
    {
        case IRC_CONFIG_SERVER_AUTOCONNECT:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "boolean",
                N_("automatically connect to server when WeeChat is starting"),
                NULL, 0, 0,
                irc_config_server_option_default[index_option], value,
                NULL, NULL,
                callback_change, callback_change_data,
                callback_delete, callback_delete_data);
            break;
        case IRC_CONFIG_SERVER_AUTORECONNECT:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "boolean",
                N_("automatically reconnect to server when disconnected"),
                NULL, 0, 0,
                irc_config_server_option_default[index_option], value,
                NULL, NULL,
                callback_change, callback_change_data,
                callback_delete, callback_delete_data);
            break;
        case IRC_CONFIG_SERVER_AUTORECONNECT_DELAY:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("delay (in seconds) before trying again to reconnect to server"),
                NULL, 0, 65535,
                irc_config_server_option_default[index_option], value,
                NULL, NULL,
                callback_change, callback_change_data,
                callback_delete, callback_delete_data);
            break;
        case IRC_CONFIG_SERVER_PROXY:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("proxy used for this server (optional)"),
                NULL, 0, 0,
                irc_config_server_option_default[index_option], value,
                NULL, NULL,
                callback_change, callback_change_data,
                callback_delete, callback_delete_data);
            break;
        case IRC_CONFIG_SERVER_ADDRESSES:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("list of IP/port or hostname/port for server (separated by comma)"),
                NULL, 0, 0,
                irc_config_server_option_default[index_option], value,
                NULL, NULL,
                callback_change, callback_change_data,
                callback_delete, callback_delete_data);
            break;
        case IRC_CONFIG_SERVER_IPV6:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "boolean",
                N_("use IPv6 protocol for server communication"),
                NULL, 0, 0,
                irc_config_server_option_default[index_option], value,
                NULL, NULL,
                callback_change, callback_change_data,
                callback_delete, callback_delete_data);
            break;
        case IRC_CONFIG_SERVER_SSL:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "boolean",
                N_("use SSL for server communication"),
                NULL, 0, 0,
                irc_config_server_option_default[index_option], value,
                NULL, NULL,
                callback_change, callback_change_data,
                callback_delete, callback_delete_data);
            break;
        case IRC_CONFIG_SERVER_PASSWORD:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("password for IRC server"),
                NULL, 0, 0,
                irc_config_server_option_default[index_option], value,
                NULL, NULL,
                callback_change, callback_change_data,
                callback_delete, callback_delete_data);
            break;
        case IRC_CONFIG_SERVER_NICKS:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("nicknames to use on IRC server (separated by comma)"),
                NULL, 0, 0,
                irc_config_server_option_default[index_option], value,
                NULL, NULL,
                callback_change, callback_change_data,
                callback_delete, callback_delete_data);
            break;
        case IRC_CONFIG_SERVER_USERNAME:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("user name to use on IRC server"),
                NULL, 0, 0,
                irc_config_server_option_default[index_option], value,
                NULL, NULL,
                callback_change, callback_change_data,
                callback_delete, callback_delete_data);
            break;
        case IRC_CONFIG_SERVER_REALNAME:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("real name to use on IRC server"),
                NULL, 0, 0,
                irc_config_server_option_default[index_option], value,
                NULL, NULL,
                callback_change, callback_change_data,
                callback_delete, callback_delete_data);
            break;
        case IRC_CONFIG_SERVER_LOCAL_HOSTNAME:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("custom local hostname/IP for server (optional, if empty "
                   "local hostname is used)"),
                NULL, 0, 0,
                irc_config_server_option_default[index_option], value,
                NULL, NULL,
                callback_change, callback_change_data,
                callback_delete, callback_delete_data);
            break;
        case IRC_CONFIG_SERVER_COMMAND:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("command(s) to run when connected to server (many commands should "
                   "be separated by ';', use '\\;' for a semicolon, special variables "
                   "$nick, $channel and $server are replaced by their value)"),
                NULL, 0, 0,
                irc_config_server_option_default[index_option], value,
                NULL, NULL,
                callback_change, callback_change_data,
                callback_delete, callback_delete_data);
            break;
        case IRC_CONFIG_SERVER_COMMAND_DELAY:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "integer",
                N_("delay (in seconds) after command was executed (example: give some "
                   "time for authentication)"),
                NULL, 0, 3600,
                irc_config_server_option_default[index_option], value,
                NULL, NULL,
                callback_change, callback_change_data,
                callback_delete, callback_delete_data);
            break;
        case IRC_CONFIG_SERVER_AUTOJOIN:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "string",
                N_("comma separated list of channels to join when connected to server "
                   "(example: \"#chan1,#chan2,#chan3 key1,key2\")"),
                NULL, 0, 0,
                irc_config_server_option_default[index_option], value,
                NULL, NULL,
                callback_change, callback_change_data,
                callback_delete, callback_delete_data);
            break;
        case IRC_CONFIG_SERVER_AUTOREJOIN:
            new_option = weechat_config_new_option (
                config_file, section,
                option_name, "boolean",
                N_("automatically rejoin channels when kicked"),
                NULL, 0, 0,
                irc_config_server_option_default[index_option], value,
                NULL, NULL,
                callback_change, callback_change_data,
                callback_delete, callback_delete_data);
            break;
        case IRC_CONFIG_NUM_SERVER_OPTIONS:
            break;
    }
    
    return new_option;
}

/*
 * irc_config_server_create_option: create a server option
 */

int
irc_config_server_create_option (void *data, struct t_config_file *config_file,
                                 struct t_config_section *section,
                                 const char *option_name, const char *value)
{
    struct t_config_option *ptr_option;
    struct t_irc_server *ptr_server;
    int rc, index_option;
    char *pos_option, *server_name;
    
    /* make C compiler happy */
    (void) data;
    
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
                index_option = irc_config_search_server_option (pos_option);
                if (index_option >= 0)
                {
                    ptr_server = irc_server_search (server_name);
                    if (!ptr_server)
                        ptr_server = irc_server_alloc (server_name);
                    if (!ptr_server)
                    {
                        weechat_printf (NULL,
                                        _("%s%s: error creating server "
                                          "\"%s\""),
                                        weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                        server_name);
                    }
                    ptr_option = weechat_config_search_option (config_file,
                                                               section,
                                                               option_name);
                    if (ptr_option)
                    {
                        rc = weechat_config_option_set (ptr_option, value, 1);
                    }
                    else
                    {
                        if (value && value[0] && (index_option >= 0))
                        {
                            ptr_option = irc_config_server_new_option (config_file,
                                                                       section,
                                                                       index_option,
                                                                       option_name,
                                                                       value,
                                                                       &irc_config_server_change_cb,
                                                                       irc_config_server_option_string[index_option],
                                                                       &irc_config_server_delete_cb,
                                                                       irc_config_server_option_string[index_option]);
                            
                            if (ptr_option)
                            {
                                if (ptr_server)
                                {
                                    irc_server_set_with_option (ptr_server,
                                                                index_option,
                                                                ptr_option);
                                }
                                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                            }
                        }
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
    
    for (i = 0; i < IRC_CONFIG_NUM_SERVER_OPTIONS; i++)
    {
        default_value = NULL;
        if (i == IRC_CONFIG_SERVER_NICKS)
            default_value = nicks;
        else if (i == IRC_CONFIG_SERVER_USERNAME)
            default_value = username;
        else if (i == IRC_CONFIG_SERVER_REALNAME)
            default_value = realname;
        if (!default_value)
            default_value = irc_config_server_option_default[i];
        
        irc_config_server_default[i] = irc_config_server_new_option (
            irc_config_file,
            section,
            i,
            irc_config_server_option_string[i],
            default_value,
            &irc_config_server_default_change_cb,
            irc_config_server_option_string[i],
            NULL,
            NULL);
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
        NULL, 0, 0, "on", NULL, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_look_one_server_buffer = weechat_config_new_option (
        irc_config_file, ptr_section,
        "one_server_buffer", "boolean",
        N_("use same buffer for all servers"),
        NULL, 0, 0, "off", NULL, NULL, NULL,
        &irc_config_change_look_one_server_buffer, NULL, NULL, NULL);
    irc_config_look_open_near_server = weechat_config_new_option (
        irc_config_file, ptr_section,
        "open_near_server", "boolean",
        N_("open new channels/privates near server"),
        NULL, 0, 0, "off", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_nick_prefix = weechat_config_new_option (
        irc_config_file, ptr_section,
        "nick_prefix", "string",
        N_("text to display before nick in chat window"),
        NULL, 0, 0, "", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_nick_suffix = weechat_config_new_option (
        irc_config_file, ptr_section,
        "nick_suffix", "string",
        N_("text to display after nick in chat window"),
        NULL, 0, 0, "", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_nick_completion_smart = weechat_config_new_option (
        irc_config_file, ptr_section,
        "nick_completion_smart", "boolean",
        N_("smart completion for nicks (completes with last speakers first)"),
        NULL, 0, 0, "on", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_display_away = weechat_config_new_option (
        irc_config_file, ptr_section,
        "display_away", "integer",
        N_("display message when (un)marking as away"),
        "off|local|channel", 0, 0, "local", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_display_channel_modes = weechat_config_new_option (
        irc_config_file, ptr_section,
        "display_channel_modes", "boolean",
        N_("display channel modes in \"buffer_name\" bar item"),
        NULL, 0, 0, "on", NULL, NULL, NULL,
        &irc_config_change_look_display_channel_modes, NULL, NULL, NULL);
    irc_config_look_hide_nickserv_pwd = weechat_config_new_option (
        irc_config_file, ptr_section,
        "hide_nickserv_pwd", "boolean",
        N_("hide password displayed by nickserv"),
        NULL, 0, 0, "on", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_highlight_tags = weechat_config_new_option (
        irc_config_file, ptr_section,
        "highlight_tags", "string",
        N_("comma separated list of tags for messages that may produce "
           "highlight (usually any message from another user, not server "
           "messages,..)"),
        NULL, 0, 0, "irc_privmsg,irc_notice", NULL, NULL, NULL,
        &irc_config_change_look_highlight_tags, NULL, NULL, NULL);
    irc_config_look_show_away_once = weechat_config_new_option (
        irc_config_file, ptr_section,
        "show_away_once", "boolean",
        N_("show remote away message only once in private"),
        NULL, 0, 0, "on", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_smart_filter = weechat_config_new_option (
        irc_config_file, ptr_section,
        "smart_filter", "boolean",
        N_("filter join/part/quit messages for a nick if not speaking for "
           "some minutes on channel (you must create a filter on tag "
           "\"irc_smart_filter\")"),
        NULL, 0, 0, "off", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_smart_filter_delay = weechat_config_new_option (
        irc_config_file, ptr_section,
        "smart_filter_delay", "integer",
        N_("delay for filtering join/part/quit messages (in minutes)"),
        NULL, 1, 60*24*7, "5", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_look_notice_as_pv = weechat_config_new_option (
        irc_config_file, ptr_section,
        "notice_as_pv", "boolean",
        N_("display notices as private messages"),
        NULL, 0, 0, "off", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    
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
        NULL, -1, 0, "green", NULL, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_color_message_quit = weechat_config_new_option (
        irc_config_file, ptr_section,
        "message_quit", "color",
        N_("color for text in part/quit messages"),
        NULL, -1, 0, "red", NULL, NULL, NULL,
        NULL, NULL, NULL, NULL);
    irc_config_color_input_nick = weechat_config_new_option (
        irc_config_file, ptr_section,
        "input_nick", "color",
        N_("color for nick in input bar"),
        NULL, -1, 0, "lightcyan", NULL, NULL, NULL,
        &irc_config_change_color_input_nick, NULL, NULL, NULL);
    
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
    
    irc_config_network_default_msg_part = weechat_config_new_option (
        irc_config_file, ptr_section,
        "default_msg_part", "string",
        N_("default part message (leaving channel) ('%v' will be replaced by "
           "WeeChat version in string)"),
        NULL, 0, 0, "WeeChat %v", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_network_default_msg_quit = weechat_config_new_option (
        irc_config_file, ptr_section,
        "default_msg_quit", "string",
        N_("default quit message (disconnecting from server) ('%v' will be "
           "replaced by WeeChat version in string)"),
        NULL, 0, 0, "WeeChat %v", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_network_away_check = weechat_config_new_option (
        irc_config_file, ptr_section,
        "away_check", "integer",
        N_("interval between two checks for away (in minutes, 0 = never "
           "check)"),
        NULL, 0, INT_MAX, "0", NULL, NULL, NULL,
        &irc_config_change_network_away_check, NULL, NULL, NULL);
    irc_config_network_away_check_max_nicks = weechat_config_new_option (
        irc_config_file, ptr_section,
        "away_check_max_nicks", "integer",
        N_("do not check away nicks on channels with high number of nicks "
           "(0 = unlimited)"),
        NULL, 0, INT_MAX, "0", NULL, NULL, NULL,
        &irc_config_change_network_away_check, NULL, NULL, NULL);
    irc_config_network_lag_check = weechat_config_new_option (
        irc_config_file, ptr_section,
        "lag_check", "integer",
        N_("interval between two checks for lag (in seconds, 0 = never "
           "check)"),
        NULL, 0, INT_MAX, "60", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_network_lag_min_show = weechat_config_new_option (
        irc_config_file, ptr_section,
        "lag_min_show", "integer",
        N_("minimum lag to show (in seconds)"),
        NULL, 0, INT_MAX, "1", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_network_lag_disconnect = weechat_config_new_option (
        irc_config_file, ptr_section,
        "lag_disconnect", "integer",
        N_("disconnect after important lag (in minutes, 0 = never "
           "disconnect)"),
        NULL, 0, INT_MAX, "5", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_network_anti_flood = weechat_config_new_option (
        irc_config_file, ptr_section,
        "anti_flood", "integer",
        N_("anti-flood: # seconds between two user messages (0 = no "
           "anti-flood)"),
        NULL, 0, 5, "2", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_network_colors_receive = weechat_config_new_option (
        irc_config_file, ptr_section,
        "colors_receive", "boolean",
        N_("when off, colors codes are ignored in incoming messages"),
        NULL, 0, 0, "on", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_network_colors_send = weechat_config_new_option (
        irc_config_file, ptr_section,
        "colors_send", "boolean",
        N_("allow user to send colors with special codes (^Cb=bold, "
           "^Ccxx=color, ^Ccxx,yy=color+background, ^Cu=underline, "
           "^Cr=reverse)"),
        NULL, 0, 0, "on", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    irc_config_network_send_unknown_commands = weechat_config_new_option (
        irc_config_file, ptr_section,
        "send_unknown_commands", "boolean",
        N_("send unknown commands to IRC server"),
        NULL, 0, 0, "off", NULL, NULL, NULL,
        &irc_config_change_network_send_unknown_commands, NULL, NULL, NULL);
    
    /* filters */
    ptr_section = weechat_config_new_section (irc_config_file, "ignore",
                                              0, 0,
                                              &irc_config_ignore_read, NULL,
                                              &irc_config_ignore_write, NULL,
                                              &irc_config_ignore_write, NULL,
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
                                              1, 1,
                                              NULL, NULL,
                                              NULL, NULL,
                                              &irc_config_server_write_default, NULL,
                                              &irc_config_server_create_option, NULL,
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
irc_config_write ()
{
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
