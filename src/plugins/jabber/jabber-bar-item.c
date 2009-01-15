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

/* jabber-bar-item.c: bar items for Jabber plugin */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "jabber.h"
#include "jabber-buffer.h"
#include "jabber-config.h"
#include "jabber-server.h"
#include "jabber-muc.h"


/*
 * jabber_bar_item_buffer_name: bar item with buffer name
 */

char *
jabber_bar_item_buffer_name (void *data, struct t_gui_bar_item *item,
                             struct t_gui_window *window)
{
    char buf[512], buf_name[256], modes[128], away[128];
    const char *name;
    int part_from_muc;
    struct t_gui_buffer *buffer;
    struct t_jabber_server *server;
    struct t_jabber_muc *muc;
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    
    if (!window)
        window = weechat_current_window ();
    
    buf_name[0] = '\0';
    modes[0] = '\0';
    away[0] = '\0';
    
    buffer = weechat_window_get_pointer (window, "buffer");
    
    if (buffer)
    {
        jabber_buffer_get_server_muc (buffer, &server, &muc);
        if (server || muc)
        {
            if (server && !muc)
            {
                if (weechat_config_boolean (jabber_config_look_one_server_buffer))
                {
                    snprintf (buf_name, sizeof (buf_name), "%s%s[<%s%s%s>]",
                              _("servers"),
                              JABBER_COLOR_BAR_DELIM,
                              JABBER_COLOR_STATUS_NAME,
                              (jabber_current_server) ? jabber_current_server->name : "-",
                              JABBER_COLOR_BAR_DELIM);
                }
                else
                {
                    snprintf (buf_name, sizeof (buf_name), "%s%s[%s%s%s]",
                              _("server"),
                              JABBER_COLOR_BAR_DELIM,
                              JABBER_COLOR_STATUS_NAME,
                              server->name,
                              JABBER_COLOR_BAR_DELIM);
                }
            }
            else
            {
                if (muc)
                {
                    part_from_muc = ((muc->type == JABBER_MUC_TYPE_MUC)
                                     && !muc->buddies);
                    snprintf (buf_name, sizeof (buf_name),
                              "%s%s%s%s%s/%s%s%s%s",
                              (part_from_muc) ? JABBER_COLOR_BAR_DELIM : "",
                              (part_from_muc) ? "(" : "",
                              JABBER_COLOR_STATUS_NAME,
                              server->name,
                              JABBER_COLOR_BAR_DELIM,
                              JABBER_COLOR_STATUS_NAME,
                              muc->name,
                              (part_from_muc) ? JABBER_COLOR_BAR_DELIM : "",
                              (part_from_muc) ? ")" : "");
                    if (!part_from_muc
                        && weechat_config_boolean (jabber_config_look_display_muc_modes)
                        && muc->modes && muc->modes[0]
                        && (strcmp (muc->modes, "+") != 0))
                    {
                        snprintf (modes, sizeof (modes),
                                  "%s(%s%s%s)",
                                  JABBER_COLOR_BAR_DELIM,
                                  JABBER_COLOR_STATUS_NAME,
                                  muc->modes,
                                  JABBER_COLOR_BAR_DELIM);
                    }
                }
            }
            if (server && server->is_away)
            {
                snprintf (away, sizeof (away), " %s(%s%s%s)",
                          JABBER_COLOR_BAR_DELIM,
                          JABBER_COLOR_BAR_FG,
                          _("away"),
                          JABBER_COLOR_BAR_DELIM);
            }
        }
        else
        {
            name = weechat_buffer_get_string (buffer, "name");
            if (name)
                snprintf (buf_name, sizeof (buf_name), "%s", name);
        }
        
        snprintf (buf, sizeof (buf), "%s%s%s%s",
                  JABBER_COLOR_STATUS_NAME,
                  buf_name,
                  modes,
                  away);
        return strdup (buf);
    }
    
    return NULL;
}

/*
 * jabber_bar_item_input_prompt: bar item with input prompt
 */

char *
jabber_bar_item_input_prompt (void *data, struct t_gui_bar_item *item,
                              struct t_gui_window *window)
{
    struct t_gui_buffer *buffer;
    struct t_jabber_server *server;
    const char *local_name;
    
    /* make C compiler happy */
    (void) data;
    (void) item;
    
    if (!window)
        window = weechat_current_window ();
    
    buffer = weechat_window_get_pointer (window, "buffer");
    
    if (buffer)
    {
        jabber_buffer_get_server_muc (buffer, &server, NULL);
        if (!server)
            return NULL;
        
        local_name = jabber_server_get_local_name (server);
        
        return (local_name) ? strdup (local_name) : NULL;
    }
    
    return NULL;
}

/*
 * jabber_bar_item_init: initialize Jabber bar items
 */

void
jabber_bar_item_init ()
{
    weechat_bar_item_new ("buffer_name", &jabber_bar_item_buffer_name, NULL);
    weechat_bar_item_new ("input_prompt", &jabber_bar_item_input_prompt, NULL);
}
