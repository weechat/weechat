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

/* irc-bar-item.c: bar items for IRC plugin */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-buffer.h"
#include "irc-color.h"
#include "irc-config.h"
#include "irc-server.h"
#include "irc-channel.h"


/*
 * irc_bar_item_away: bar item with away indicator
 */

char *
irc_bar_item_away (void *data, struct t_gui_bar_item *item,
                   struct t_gui_window *window)
{
    struct t_gui_buffer *buffer;
    struct t_irc_server *server;
    char *buf;
    int length;
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    
    buffer = weechat_window_get_pointer (window, "buffer");
    
    if (buffer)
    {
        irc_buffer_get_server_channel (buffer, &server, NULL);
        
        if (server && server->is_away)
        {
            length = strlen (_("away")) + 64 + 1;
            buf = malloc (length);
            if (buf)
            {
                snprintf (buf, length, "%s%s",
                          IRC_COLOR_ITEM_AWAY,
                          _("away"));
                return buf;
            }
        }
    }
    
    return NULL;
}

/*
 * irc_bar_item_buffer_title: bar item with buffer title
 */

char *
irc_bar_item_buffer_title (void *data, struct t_gui_bar_item *item,
                           struct t_gui_window *window)
{
    struct t_gui_buffer *buffer;
    const char *title;
    char *title_color;
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    
    if (!window)
        window = weechat_current_window ();
    
    buffer = weechat_window_get_pointer (window, "buffer");
    
    if (buffer)
    {
        title = weechat_buffer_get_string (buffer, "title");
        if (!title)
            return NULL;
        
        title_color = irc_color_decode (title,
                                        (weechat_config_boolean (irc_config_look_topic_strip_colors)) ?
                                         0 : 1);
        
        return (title_color) ? title_color : strdup (title);
    }
    
    return NULL;
}

/*
 * irc_bar_item_buffer_plugin: bar item with buffer plugin
 */

char *
irc_bar_item_buffer_plugin (void *data, struct t_gui_bar_item *item,
                            struct t_gui_window *window)
{
    char buf[512];
    struct t_gui_buffer *buffer;
    struct t_weechat_plugin *ptr_plugin;
    const char *name;
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    
    if (!window)
        window = weechat_current_window ();
    
    buffer = weechat_window_get_pointer (window, "buffer");
    
    if (buffer)
    {
        ptr_plugin = weechat_buffer_get_pointer (buffer, "plugin");
        name = weechat_plugin_get_name (ptr_plugin);
        if (ptr_plugin == weechat_irc_plugin)
        {
            irc_buffer_get_server_channel (buffer, &server, &channel);
            if (server && channel
                && (weechat_config_integer (irc_config_look_item_display_server) == IRC_CONFIG_LOOK_ITEM_DISPLAY_SERVER_PLUGIN))
            {
                snprintf (buf, sizeof (buf), "%s%s/%s%s",
                          name,
                          IRC_COLOR_BAR_DELIM,
                          IRC_COLOR_BAR_FG,
                          server->name);
            }
            else
            {
                snprintf (buf, sizeof (buf), "%s", name);
            }
        }
        else
        {
            snprintf (buf, sizeof (buf), "%s", name);
        }
        return strdup (buf);
    }
    
    return NULL;
}

/*
 * irc_bar_item_buffer_name: bar item with buffer name
 */

char *
irc_bar_item_buffer_name (void *data, struct t_gui_bar_item *item,
                          struct t_gui_window *window)
{
    char buf[512], buf_name[256], modes[128], *modes_without_args;
    const char *name, *pos_space, *pos_key;
    int part_from_channel, display_server;
    struct t_gui_buffer *buffer;
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    
    if (!window)
        window = weechat_current_window ();
    
    buf_name[0] = '\0';
    modes[0] = '\0';
    
    display_server = (weechat_config_integer (irc_config_look_item_display_server) == IRC_CONFIG_LOOK_ITEM_DISPLAY_SERVER_NAME);
    
    buffer = weechat_window_get_pointer (window, "buffer");
    
    if (buffer)
    {
        irc_buffer_get_server_channel (buffer, &server, &channel);
        if (server || channel)
        {
            if (server && !channel)
            {
                snprintf (buf_name, sizeof (buf_name), "%s%s[%s%s%s]",
                          _("server"),
                          IRC_COLOR_BAR_DELIM,
                          IRC_COLOR_STATUS_NAME,
                          server->name,
                          IRC_COLOR_BAR_DELIM);
            }
            else
            {
                if (channel)
                {
                    part_from_channel = ((channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                                         && !channel->nicks);
                    snprintf (buf_name, sizeof (buf_name),
                              "%s%s%s%s%s%s%s%s%s%s",
                              (part_from_channel) ? IRC_COLOR_BAR_DELIM : "",
                              (part_from_channel) ? "(" : "",
                              IRC_COLOR_STATUS_NAME,
                              (display_server) ? server->name : "",
                              (display_server) ? IRC_COLOR_BAR_DELIM : "",
                              (display_server) ? "/" : "",
                              IRC_COLOR_STATUS_NAME,
                              channel->name,
                              (part_from_channel) ? IRC_COLOR_BAR_DELIM : "",
                              (part_from_channel) ? ")" : "");
                    if (!part_from_channel
                        && weechat_config_boolean (irc_config_look_display_channel_modes)
                        && channel->modes && channel->modes[0]
                        && (strcmp (channel->modes, "+") != 0))
                    {
                        modes_without_args = NULL;
                        if (weechat_config_boolean (irc_config_look_display_channel_modes_hide_key))
                        {
                            pos_space = strchr(channel->modes, ' ');
                            if (pos_space)
                            {
                                pos_key = strchr(channel->modes, 'k');
                                if (pos_key && (pos_key < pos_space))
                                {
                                    modes_without_args = weechat_strndup(channel->modes,
                                                                         pos_space - channel->modes);
                                }
                            }
                        }
                        snprintf (modes, sizeof (modes),
                                  "%s(%s%s%s)",
                                  IRC_COLOR_BAR_DELIM,
                                  IRC_COLOR_ITEM_CHANNEL_MODES,
                                  (modes_without_args) ? modes_without_args : channel->modes,
                                  IRC_COLOR_BAR_DELIM);
                        if (modes_without_args)
                            free (modes_without_args);
                    }
                }
            }
        }
        else
        {
            name = weechat_buffer_get_string (buffer, "name");
            if (name)
                snprintf (buf_name, sizeof (buf_name), "%s", name);
        }
        
        snprintf (buf, sizeof (buf), "%s%s%s",
                  IRC_COLOR_STATUS_NAME,
                  buf_name,
                  modes);
        return strdup (buf);
    }
    
    return NULL;
}

/*
 * irc_bar_item_lag: bar item with lag value
 */

char *
irc_bar_item_lag (void *data, struct t_gui_bar_item *item,
                  struct t_gui_window *window)
{
    char buf[32];
    struct t_gui_buffer *buffer;
    struct t_irc_server *server;
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    
    buffer = weechat_window_get_pointer (window, "buffer");
    
    if (buffer)
    {
        irc_buffer_get_server_channel (buffer, &server, NULL);
        
        if (server
            && (server->lag >= weechat_config_integer (irc_config_network_lag_min_show) * 1000))
        {
            snprintf (buf, sizeof (buf),
                      "%s: %.1f",
                      _("Lag"),
                      ((float)(server->lag)) / 1000);
            return strdup (buf);
        }
    }

    return NULL;
}

/*
 * irc_bar_item_input_prompt: bar item with input prompt
 */

char *
irc_bar_item_input_prompt (void *data, struct t_gui_bar_item *item,
                           struct t_gui_window *window)
{
    struct t_gui_buffer *buffer;
    struct t_irc_server *server;
    char *buf;
    int length;
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    
    if (!window)
        window = weechat_current_window ();
    
    buffer = weechat_window_get_pointer (window, "buffer");
    
    if (buffer)
    {
        irc_buffer_get_server_channel (buffer, &server, NULL);
        if (!server || !server->nick)
            return NULL;
        
        length = strlen (server->nick) + 64 +
            ((server->nick_modes) ? strlen (server->nick_modes) : 0) + 64 + 1;
        
        buf = malloc (length);
        if (buf)
        {
            if (weechat_config_boolean (irc_config_look_display_nick_modes)
                && server->nick_modes && server->nick_modes[0])
            {
                snprintf (buf, length, "%s%s%s(%s%s%s)",
                          IRC_COLOR_INPUT_NICK,
                          server->nick,
                          IRC_COLOR_BAR_DELIM,
                          IRC_COLOR_BAR_FG,
                          server->nick_modes,
                          IRC_COLOR_BAR_DELIM);
            }
            else
            {
                snprintf (buf, length, "%s%s",
                          IRC_COLOR_INPUT_NICK,
                          server->nick);
            }
        }
        
        return buf;
    }
    
    return NULL;
}

/*
 * irc_bar_item_init: initialize IRC bar items
 */

void
irc_bar_item_init ()
{
    weechat_bar_item_new ("away", &irc_bar_item_away, NULL);
    weechat_bar_item_new ("buffer_title", &irc_bar_item_buffer_title, NULL);
    weechat_bar_item_new ("buffer_plugin", &irc_bar_item_buffer_plugin, NULL);
    weechat_bar_item_new ("buffer_name", &irc_bar_item_buffer_name, NULL);
    weechat_bar_item_new ("lag", &irc_bar_item_lag, NULL);
    weechat_bar_item_new ("input_prompt", &irc_bar_item_input_prompt, NULL);
}
