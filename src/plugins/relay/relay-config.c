/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
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

/* relay-config.c: relay configuration options */


#include <stdlib.h>
#include <limits.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-config.h"
#include "relay-client.h"
#include "relay-buffer.h"
#include "relay-network.h"


struct t_config_file *relay_config_file = NULL;

/* relay config, look section */

struct t_config_option *relay_config_look_auto_open_buffer;

/* relay config, color section */

struct t_config_option *relay_config_color_text;
struct t_config_option *relay_config_color_text_bg;
struct t_config_option *relay_config_color_text_selected;
struct t_config_option *relay_config_color_status[RELAY_NUM_STATUS];

/* relay config, network section */

struct t_config_option *relay_config_network_enabled;
struct t_config_option *relay_config_network_listen_port_range;


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
 * relay_config_change_network_enabled_cb: callback called when user
 *                                         enables/disables relay
 */

void
relay_config_change_network_enabled_cb (void *data,
                                        struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    if ((weechat_config_boolean(relay_config_network_enabled) && relay_network_sock < 0)
        || (!weechat_config_boolean(relay_config_network_enabled) && relay_network_sock >= 0))
    {
        relay_network_init ();
    }
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
    
    relay_config_network_enabled = weechat_config_new_option (
        relay_config_file, ptr_section,
        "enabled", "boolean",
        N_("enable relay"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL,
        &relay_config_change_network_enabled_cb, NULL, NULL, NULL);
    relay_config_network_listen_port_range = weechat_config_new_option (
        relay_config_file, ptr_section,
        "listen_port_range", "string",
        N_("port number (or range of ports) that relay plugin listens on "
           "(syntax: a single port, ie. 5000 or a port "
           "range, ie. 5000-5015, it's recommended to use ports greater than "
           "1024, because only root can use ports below 1024)"),
        NULL, 0, 0, "22373-22400", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    
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
