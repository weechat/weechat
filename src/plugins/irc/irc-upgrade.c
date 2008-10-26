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

/* irc-upgrade.c: save/restore IRC plugin data */


#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-upgrade.h"
#include "irc-buffer.h"
#include "irc-config.h"
#include "irc-input.h"
#include "irc-server.h"
#include "irc-channel.h"
#include "irc-nick.h"


struct t_irc_server *irc_upgrade_current_server = NULL;
struct t_irc_channel *irc_upgrade_current_channel = NULL;


/*
 * irc_upgrade_save_all_data: save servers/channels/nicks info to upgrade file
 */

int
irc_upgrade_save_all_data (struct t_upgrade_file *upgrade_file)
{
    struct t_infolist *infolist;
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    int rc;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        /* save server */
        infolist = weechat_infolist_new ();
        if (!infolist)
            return 0;
        if (!irc_server_add_to_infolist (infolist, ptr_server))
        {
            weechat_infolist_free (infolist);
            return 0;
        }
        rc = weechat_upgrade_write_object (upgrade_file,
                                           IRC_UPGRADE_TYPE_SERVER,
                                           infolist);
        weechat_infolist_free (infolist);
        if (!rc)
            return 0;
        
        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            /* save channel */
            infolist = weechat_infolist_new ();
            if (!infolist)
                return 0;
            if (!irc_channel_add_to_infolist (infolist, ptr_channel))
            {
                weechat_infolist_free (infolist);
                return 0;
            }
            rc = weechat_upgrade_write_object (upgrade_file,
                                               IRC_UPGRADE_TYPE_CHANNEL,
                                               infolist);
            weechat_infolist_free (infolist);
            if (!rc)
                return 0;
            
            for (ptr_nick = ptr_channel->nicks; ptr_nick;
                 ptr_nick = ptr_nick->next_nick)
            {
                /* save nick */
                infolist = weechat_infolist_new ();
                if (!infolist)
                    return 0;
                if (!irc_nick_add_to_infolist (infolist, ptr_nick))
                {
                    weechat_infolist_free (infolist);
                    return 0;
                }
                rc = weechat_upgrade_write_object (upgrade_file,
                                                   IRC_UPGRADE_TYPE_NICK,
                                                   infolist);
                weechat_infolist_free (infolist);
                if (!rc)
                    return 0;
            }
        }
    }
    
    return 1;
}

/*
 * irc_upgrade_save: save upgrade file
 *                   return 1 if ok, 0 if error
 */

int
irc_upgrade_save ()
{
    int rc;
    struct t_upgrade_file *upgrade_file;
    
    upgrade_file = weechat_upgrade_create (IRC_UPGRADE_FILENAME, 1);
    if (!upgrade_file)
        return 0;
    
    rc = irc_upgrade_save_all_data (upgrade_file);
    
    weechat_upgrade_close (upgrade_file);
    
    return rc;
}

/*
 * irc_upgrade_set_buffer_callbacks: restore buffers callbacks (input and
 *                                   close) for buffers created by IRC plugin
 */

void
irc_upgrade_set_buffer_callbacks ()
{
    struct t_infolist *infolist;
    struct t_gui_buffer *ptr_buffer;
    
    infolist = weechat_infolist_get ("buffer", NULL, NULL);
    if (infolist)
    {
        while (weechat_infolist_next (infolist))
        {
            if (weechat_infolist_pointer (infolist, "plugin") == weechat_irc_plugin)
            {
                ptr_buffer = weechat_infolist_pointer (infolist, "pointer");
                weechat_buffer_set_pointer (ptr_buffer, "close_callback", &irc_buffer_close_cb);
                weechat_buffer_set_pointer (ptr_buffer, "input_callback", &irc_input_data_cb);
            }
        }
    }
}

/*
 * irc_upgrade_read_cb: read callback for 
 */

int
irc_upgrade_read_cb (int object_id,
                     struct t_infolist *infolist)
{
    int flags, sock, size, index;
    char *str, *buf, *buffer_name, option_name[64], *nick;
    struct t_irc_nick *ptr_nick;
    struct t_gui_buffer *ptr_buffer;
    
    weechat_infolist_reset_item_cursor (infolist);
    while (weechat_infolist_next (infolist))
    {
        switch (object_id)
        {
            case IRC_UPGRADE_TYPE_SERVER:
                irc_upgrade_current_server = irc_server_search (weechat_infolist_string (infolist, "name"));
                if (irc_upgrade_current_server)
                {
                    irc_upgrade_current_server->buffer = NULL;
                    buffer_name = weechat_infolist_string (infolist, "buffer_name");
                    if (buffer_name && buffer_name[0])
                    {
                        ptr_buffer = weechat_buffer_search (IRC_PLUGIN_NAME,
                                                            buffer_name);
                        if (ptr_buffer)
                        {
                            irc_upgrade_current_server->buffer = ptr_buffer;
                            if (weechat_config_boolean (irc_config_look_one_server_buffer)
                                && !irc_buffer_servers)
                            {
                                irc_buffer_servers = ptr_buffer;
                            }
                            if (weechat_infolist_integer (infolist, "selected"))
                                irc_current_server = irc_upgrade_current_server;
                        }
                    }
                    irc_upgrade_current_server->current_address = weechat_infolist_integer (infolist, "current_address");
                    
                    sock = weechat_infolist_integer (infolist, "sock");
                    if (sock >= 0)
                    {
                        irc_upgrade_current_server->sock = sock;
                        irc_upgrade_current_server->hook_fd = weechat_hook_fd (irc_upgrade_current_server->sock,
                                                                               1, 0, 0,
                                                                               &irc_server_recv_cb,
                                                                               irc_upgrade_current_server);
                    }
                    irc_upgrade_current_server->is_connected = weechat_infolist_integer (infolist, "is_connected");
                    irc_upgrade_current_server->ssl_connected = weechat_infolist_integer (infolist, "ssl_connected");
                    str = weechat_infolist_string (infolist, "unterminated_message");
                    if (str)
                        irc_upgrade_current_server->unterminated_message = strdup (str);
                    str = weechat_infolist_string (infolist, "nick");
                    if (str)
                        irc_server_set_nick (irc_upgrade_current_server, str);
                    str = weechat_infolist_string (infolist, "nick_modes");
                    if (str)
                        irc_upgrade_current_server->nick_modes = strdup (str);
                    str = weechat_infolist_string (infolist, "prefix");
                    if (str)
                        irc_upgrade_current_server->prefix = strdup (str);
                    irc_upgrade_current_server->reconnect_start = weechat_infolist_time (infolist, "reconnect_start");
                    irc_upgrade_current_server->command_time = weechat_infolist_time (infolist, "command_time");
                    irc_upgrade_current_server->reconnect_join = weechat_infolist_integer (infolist, "reconnect_join");
                    irc_upgrade_current_server->disable_autojoin = weechat_infolist_integer (infolist, "disable_autojoin");
                    irc_upgrade_current_server->is_away = weechat_infolist_integer (infolist, "is_away");
                    str = weechat_infolist_string (infolist, "away_message");
                    if (str)
                        irc_upgrade_current_server->away_message = strdup (str);
                    irc_upgrade_current_server->away_time = weechat_infolist_time (infolist, "away_time");
                    irc_upgrade_current_server->lag = weechat_infolist_integer (infolist, "lag");
                    buf = weechat_infolist_buffer (infolist, "lag_check_time", &size);
                    if (buf)
                        memcpy (&(irc_upgrade_current_server->lag_check_time), buf, size);
                    irc_upgrade_current_server->lag_next_check = weechat_infolist_time (infolist, "lag_next_check");
                    irc_upgrade_current_server->queue_msg = weechat_infolist_integer (infolist, "queue_msg");
                    irc_upgrade_current_server->last_user_message = weechat_infolist_time (infolist, "last_user_message");
                }
                break;
            case IRC_UPGRADE_TYPE_CHANNEL:
                if (irc_upgrade_current_server)
                {
                    irc_upgrade_current_channel = irc_channel_new (irc_upgrade_current_server,
                                                                   weechat_infolist_integer (infolist, "type"),
                                                                   weechat_infolist_string (infolist, "name"),
                                                                   0);
                    if (irc_upgrade_current_channel)
                    {
                        str = weechat_infolist_string (infolist, "topic");
                        if (str)
                            irc_channel_set_topic (irc_upgrade_current_channel, str);
                        str = weechat_infolist_string (infolist, "modes");
                        if (str)
                            irc_upgrade_current_channel->modes = strdup (str);
                        irc_upgrade_current_channel->limit = weechat_infolist_integer (infolist, "limit");
                        str = weechat_infolist_string (infolist, "key");
                        if (str)
                            irc_upgrade_current_channel->key = strdup (str);
                        irc_upgrade_current_channel->checking_away = weechat_infolist_integer (infolist, "checking_away");
                        str = weechat_infolist_string (infolist, "away_message");
                        if (str)
                            irc_upgrade_current_channel->away_message = strdup (str);
                        irc_upgrade_current_channel->cycle = weechat_infolist_integer (infolist, "cycle");
                        irc_upgrade_current_channel->display_creation_date = weechat_infolist_integer (infolist, "display_creation_date");
                        irc_upgrade_current_channel->nick_completion_reset = weechat_infolist_integer (infolist, "nick_completion_reset");
                        index = 0;
                        while (1)
                        {
                            snprintf (option_name, sizeof (option_name),
                                      "nick_speaking_%05d", index);
                            nick = weechat_infolist_string (infolist, option_name);
                            if (!nick)
                                break;
                            irc_channel_nick_speaking_add (irc_upgrade_current_channel,
                                                           nick);
                            index++;
                        }
                        index = 0;
                        while (1)
                        {
                            snprintf (option_name, sizeof (option_name),
                                      "nick_speaking_time_nick_%05d", index);
                            nick = weechat_infolist_string (infolist, option_name);
                            if (!nick)
                                break;
                            snprintf (option_name, sizeof (option_name),
                                      "nick_speaking_time_time_%05d", index);
                            irc_channel_nick_speaking_time_add (irc_upgrade_current_channel,
                                                                nick,
                                                                weechat_infolist_time (infolist,
                                                                                       option_name));
                            index++;
                        }
                    }
                }
                break;
            case IRC_UPGRADE_TYPE_NICK:
                if (irc_upgrade_current_server && irc_upgrade_current_channel)
                {
                    flags = weechat_infolist_integer (infolist, "flags");
                    ptr_nick = irc_nick_new (irc_upgrade_current_server,
                                             irc_upgrade_current_channel,
                                             weechat_infolist_string (infolist, "name"),
                                             flags & IRC_NICK_CHANOWNER,
                                             flags & IRC_NICK_CHANADMIN,
                                             flags & IRC_NICK_CHANADMIN2,
                                             flags & IRC_NICK_OP,
                                             flags & IRC_NICK_HALFOP,
                                             flags & IRC_NICK_VOICE,
                                             flags & IRC_NICK_CHANUSER,
                                             flags & IRC_NICK_AWAY);
                    if (ptr_nick)
                    {
                        str = weechat_infolist_string (infolist, "host");
                        if (str)
                            ptr_nick->host = strdup (str);
                    }
                }
                break;
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_upgrade_load: load upgrade file
 *                   return 1 if ok, 0 if error
 */

int
irc_upgrade_load ()
{
    int rc;
    struct t_upgrade_file *upgrade_file;

    irc_upgrade_set_buffer_callbacks ();
    
    upgrade_file = weechat_upgrade_create (IRC_UPGRADE_FILENAME, 0);
    rc = weechat_upgrade_read (upgrade_file, &irc_upgrade_read_cb);
    
    return rc;
}

