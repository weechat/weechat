/*
 * Copyright (C) 2003-2010 Sebastien Helleu <flashcode@flashtux.org>
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
 * relay-config.c: relay configuration options (file relay.conf)
 */

#include <stdlib.h>
#include <limits.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-config.h"
#include "relay-client.h"
#include "relay-buffer.h"
#include "relay-server.h"


struct t_config_file *relay_config_file = NULL;
struct t_config_section *relay_config_section_port = NULL;

/* relay config, look section */

struct t_config_option *relay_config_look_auto_open_buffer;

/* relay config, color section */

struct t_config_option *relay_config_color_text;
struct t_config_option *relay_config_color_text_bg;
struct t_config_option *relay_config_color_text_selected;
struct t_config_option *relay_config_color_status[RELAY_NUM_STATUS];

/* relay config, network section */

struct t_config_option *relay_config_network_max_clients;


/*
 * relay_config_refresh_cb: callback called when user changes relay option that
 *                          needs a refresh of relay list
 */

void
relay_config_refresh_cb (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    if (relay_buffer)
        relay_buffer_refresh (NULL);
}

/*
 * relay_config_change_port_cb: callback called when relay port option is
 *                              modified
 */

int
relay_config_check_port_cb (void *data, struct t_config_option *option,
                            const char *value)
{
    char *error;
    long port;
    struct t_relay_server *ptr_server;
    
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    error = NULL;
    port = strtol (value, &error, 10);
    ptr_server = relay_server_search_port ((int)port);
    if (ptr_server)
    {
        weechat_printf (NULL, _("%s%s: error: port \"%d\" is already used"),
                        weechat_prefix ("error"),
                        RELAY_PLUGIN_NAME, (int)port);
        return 0;
    }
    
    return 1;
}

/*
 * relay_config_change_port_cb: callback called when relay port option is
 *                              modified
 */

void
relay_config_change_port_cb (void *data, struct t_config_option *option)
{
    struct t_relay_server *ptr_server;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_server = relay_server_search (weechat_config_option_get_pointer (option, "name"));
    if (ptr_server)
    {
        relay_server_update_port (ptr_server,
                                  *((int *)weechat_config_option_get_pointer (option, "value")));
    }
}

/*
 * relay_config_delete_port_cb: callback called when relay port option is
 *                              deleted
 */

void
relay_config_delete_port_cb (void *data, struct t_config_option *option)
{
    struct t_relay_server *ptr_server;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_server = relay_server_search (weechat_config_option_get_pointer (option, "name"));
    if (ptr_server)
        relay_server_free (ptr_server);
}

/*
 * relay_config_create_option_port: create a relay for a port
 */

int
relay_config_create_option_port (void *data,
                                 struct t_config_file *config_file,
                                 struct t_config_section *section,
                                 const char *option_name,
                                 const char *value)
{
    int rc, protocol_number;
    char *error, *protocol, *protocol_string;
    long port;
    struct t_relay_server *ptr_server;
    
    /* make C compiler happy */
    (void) data;
    
    rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
    
    relay_server_get_protocol_string (option_name,
                                      &protocol, &protocol_string);
    
    protocol_number = -1;
    port = -1;
    
    if (protocol && protocol_string)
        protocol_number = relay_protocol_search (protocol);
    
    if (protocol_number < 0)
    {
        weechat_printf (NULL, _("%s%s: error: unknown protocol \"%s\""),
                        weechat_prefix ("error"),
                        RELAY_PLUGIN_NAME, protocol);
        rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
    }
    
    if (rc != WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        error = NULL;
        port = strtol (value, &error, 10);
        ptr_server = relay_server_search_port ((int)port);
        if (ptr_server)
        {
            weechat_printf (NULL, _("%s%s: error: port \"%d\" is already used"),
                            weechat_prefix ("error"),
                            RELAY_PLUGIN_NAME, (int)port);
            rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
        }
    }
    
    if (rc != WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        /* create config option */
        weechat_config_new_option (
            config_file, section,
            option_name, "integer", NULL,
            NULL, 0, 65535, "", value, 0,
            &relay_config_check_port_cb, NULL,
            &relay_config_change_port_cb, NULL,
            &relay_config_delete_port_cb, NULL);
        
        relay_server_new (protocol_number, protocol_string, port);
        
        rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
    }
    
    if (protocol)
        free (protocol);
    if (protocol_string)
        free (protocol_string);
    
    return rc;
}

/*
 * relay_config_reload: reload relay configuration file
 */

int
relay_config_reload (void *data, struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) data;
    
    return  weechat_config_reload (config_file);
}

/*
 * relay_config_init: init relay configuration file
 *                    return: 1 if ok, 0 if error
 */

int
relay_config_init ()
{
    struct t_config_section *ptr_section;
    
    relay_config_file = weechat_config_new (RELAY_CONFIG_NAME,
                                            &relay_config_reload, NULL);
    if (!relay_config_file)
        return 0;
    
    ptr_section = weechat_config_new_section (relay_config_file, "look",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (relay_config_file);
        return 0;
    }
    
    relay_config_look_auto_open_buffer = weechat_config_new_option (
        relay_config_file, ptr_section,
        "auto_open_buffer", "boolean",
        N_("auto open relay buffer when a new client is connecting"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    
    ptr_section = weechat_config_new_section (relay_config_file, "color",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (relay_config_file);
        return 0;
    }
    
    relay_config_color_text = weechat_config_new_option (
        relay_config_file, ptr_section,
        "text", "color",
        N_("text color"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, &relay_config_refresh_cb, NULL, NULL, NULL);
    relay_config_color_text_bg = weechat_config_new_option (
        relay_config_file, ptr_section,
        "text_bg", "color",
        N_("background color"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, &relay_config_refresh_cb, NULL, NULL, NULL);
    relay_config_color_text_selected = weechat_config_new_option (
        relay_config_file, ptr_section,
        "text_selected", "color",
        N_("text color of selected client line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, &relay_config_refresh_cb, NULL, NULL, NULL);
    relay_config_color_status[RELAY_STATUS_CONNECTING] = weechat_config_new_option (
        relay_config_file, ptr_section,
        "status_connecting", "color",
        N_("text color for \"connecting\" status"),
        NULL, 0, 0, "yellow", NULL, 0,
        NULL, NULL, &relay_config_refresh_cb, NULL, NULL, NULL);
    relay_config_color_status[RELAY_STATUS_WAITING_AUTH] = weechat_config_new_option (
        relay_config_file, ptr_section,
        "status_waiting_auth", "color",
        N_("text color for \"waiting authentication\" status"),
        NULL, 0, 0, "brown", NULL, 0,
        NULL, NULL, &relay_config_refresh_cb, NULL, NULL, NULL);
    relay_config_color_status[RELAY_STATUS_CONNECTED] = weechat_config_new_option (
        relay_config_file, ptr_section,
        "status_active", "color",
        N_("text color for \"connected\" status"),
        NULL, 0, 0, "lightblue", NULL, 0,
        NULL, NULL, &relay_config_refresh_cb, NULL, NULL, NULL);
    relay_config_color_status[RELAY_STATUS_AUTH_FAILED] = weechat_config_new_option (
        relay_config_file, ptr_section,
        "status_auth_failed", "color",
        N_("text color for \"authentication failed\" status"),
        NULL, 0, 0, "lightred", NULL, 0,
        NULL, NULL, &relay_config_refresh_cb, NULL, NULL, NULL);
    relay_config_color_status[RELAY_STATUS_DISCONNECTED] = weechat_config_new_option (
        relay_config_file, ptr_section,
        "status_disconnected", "color",
        N_("text color for \"disconnected\" status"),
        NULL, 0, 0, "lightred", NULL, 0,
        NULL, NULL, &relay_config_refresh_cb, NULL, NULL, NULL);
    
    ptr_section = weechat_config_new_section (relay_config_file, "network",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (relay_config_file);
        return 0;
    }
    
    relay_config_network_max_clients = weechat_config_new_option (
        relay_config_file, ptr_section,
        "max_clients", "integer",
        N_("maximum number of clients connecting to a port"),
        NULL, 1, 1024, "5", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL);
    
    ptr_section = weechat_config_new_section (relay_config_file, "port",
                                              1, 1,
                                              NULL, NULL,
                                              NULL, NULL,
                                              NULL, NULL,
                                              &relay_config_create_option_port, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (relay_config_file);
        return 0;
    }
    
    relay_config_section_port = ptr_section;
    
    return 1;
}

/*
 * relay_config_read: read relay configuration file
 */

int
relay_config_read ()
{
    return weechat_config_read (relay_config_file);
}

/*
 * relay_config_write: write relay configuration file
 */

int
relay_config_write ()
{
    return weechat_config_write (relay_config_file);
}
