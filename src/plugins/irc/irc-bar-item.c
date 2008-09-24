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

/* irc-bar-item.c: bar items for IRC plugin */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-buffer.h"
#include "irc-config.h"
#include "irc-server.h"
#include "irc-channel.h"


/*
 * irc_bar_item_buffer_name: bar item with buffer name
 */

char *
irc_bar_item_buffer_name (void *data, struct t_gui_bar_item *item,
                          struct t_gui_window *window,
                          int max_width, int max_height)
{
    char buf[256], buf_name[256], away[128], *name;
    int number;
    struct t_gui_buffer *buffer;
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) max_width;
    (void) max_height;
    
    if (!window)
        window = weechat_current_window;
    
    buf_name[0] = '\0';
    away[0] = '\0';
    
    buffer = weechat_window_get_pointer (window, "buffer");

    if (buffer)
    {
        number = weechat_buffer_get_integer (buffer, "number");
        
        irc_buffer_get_server_channel (buffer, &server, &channel);
        if (server || channel)
        {
            if (server && !channel)
            {
                if (weechat_config_boolean (irc_config_look_one_server_buffer))
                {
                    snprintf (buf_name, sizeof (buf_name), "%s%s[<%s%s%s>]",
                              _("servers"),
                              IRC_COLOR_BAR_DELIM,
                              IRC_COLOR_STATUS_NAME,
                              (irc_current_server) ? irc_current_server->name : "-",
                              IRC_COLOR_BAR_DELIM);
                }
                else
                {
                    snprintf (buf_name, sizeof (buf_name), "%s%s[%s%s%s]",
                              _("server"),
                              IRC_COLOR_BAR_DELIM,
                              IRC_COLOR_STATUS_NAME,
                              server->name,
                              IRC_COLOR_BAR_DELIM);
                }
            }
            else
            {
                if (channel)
                {
                    snprintf (buf_name, sizeof (buf_name),
                              "%s%s%s/%s%s%s(%s%s%s)",
                              IRC_COLOR_STATUS_NAME,
                              server->name,
                              IRC_COLOR_BAR_DELIM,
                              IRC_COLOR_STATUS_NAME,
                              channel->name,
                              IRC_COLOR_BAR_DELIM,
                              IRC_COLOR_STATUS_NAME,
                              channel->modes,
                              IRC_COLOR_BAR_DELIM);
                }
            }
            if (server && server->is_away)
            {
                snprintf (away, sizeof (away), " %s(%s%s%s)",
                          IRC_COLOR_BAR_DELIM,
                          IRC_COLOR_BAR_FG,
                          _("away"),
                          IRC_COLOR_BAR_DELIM);
            }
        }
        else
        {
            name = weechat_buffer_get_string (buffer, "name");
            if (name)
                snprintf (buf_name, sizeof (buf_name), "%s", name);
        }
        
        snprintf (buf, sizeof (buf), "%s%d%s:%s%s%s",
                  IRC_COLOR_STATUS_NUMBER,
                  number,
                  IRC_COLOR_BAR_DELIM,
                  IRC_COLOR_STATUS_NAME,
                  buf_name,
                  away);
        return strdup (buf);
    }
    
    return NULL;
}

/*
 * irc_bar_item_lag: bar item with lag value
 */

char *
irc_bar_item_lag (void *data, struct t_gui_bar_item *item,
                  struct t_gui_window *window,
                  int max_width, int max_height)
{
    char buf[32];
    struct t_gui_buffer *buffer;
    struct t_irc_server *server;
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) window;
    (void) max_width;
    (void) max_height;
    
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
 * irc_bar_item_init: initialize IRC bar items
 */

void
irc_bar_item_init ()
{
    weechat_bar_item_new ("buffer_name", &irc_bar_item_buffer_name, NULL);
    weechat_bar_item_new ("lag", &irc_bar_item_lag, NULL);
}
