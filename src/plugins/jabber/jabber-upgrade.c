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

/* jabber-upgrade.c: save/restore Jabber plugin data */


#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "jabber.h"
#include "jabber-upgrade.h"
#include "jabber-buddy.h"
#include "jabber-buffer.h"
#include "jabber-config.h"
#include "jabber-input.h"
#include "jabber-muc.h"
#include "jabber-server.h"


struct t_jabber_server *jabber_upgrade_current_server = NULL;
struct t_jabber_muc *jabber_upgrade_current_muc = NULL;


/*
 * jabber_upgrade_save_all_data: save servers/MUCs/buddies info to upgrade
 *                               file
 */

int
jabber_upgrade_save_all_data (struct t_upgrade_file *upgrade_file)
{
    struct t_infolist *infolist;
    struct t_jabber_server *ptr_server;
    struct t_jabber_muc *ptr_muc;
    struct t_jabber_buddy *ptr_buddy;
    int rc;
    
    for (ptr_server = jabber_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        /* save server */
        infolist = weechat_infolist_new ();
        if (!infolist)
            return 0;
        if (!jabber_server_add_to_infolist (infolist, ptr_server))
        {
            weechat_infolist_free (infolist);
            return 0;
        }
        rc = weechat_upgrade_write_object (upgrade_file,
                                           JABBER_UPGRADE_TYPE_SERVER,
                                           infolist);
        weechat_infolist_free (infolist);
        if (!rc)
            return 0;
        
        for (ptr_muc = ptr_server->mucs; ptr_muc;
             ptr_muc = ptr_muc->next_muc)
        {
            /* save MUC */
            infolist = weechat_infolist_new ();
            if (!infolist)
                return 0;
            if (!jabber_muc_add_to_infolist (infolist, ptr_muc))
            {
                weechat_infolist_free (infolist);
                return 0;
            }
            rc = weechat_upgrade_write_object (upgrade_file,
                                               JABBER_UPGRADE_TYPE_MUC,
                                               infolist);
            weechat_infolist_free (infolist);
            if (!rc)
                return 0;
            
            for (ptr_buddy = ptr_muc->buddies; ptr_buddy;
                 ptr_buddy = ptr_buddy->next_buddy)
            {
                /* save buddy */
                infolist = weechat_infolist_new ();
                if (!infolist)
                    return 0;
                if (!jabber_buddy_add_to_infolist (infolist, ptr_buddy))
                {
                    weechat_infolist_free (infolist);
                    return 0;
                }
                rc = weechat_upgrade_write_object (upgrade_file,
                                                   JABBER_UPGRADE_TYPE_BUDDY,
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
 * jabber_upgrade_save: save upgrade file
 *                      return 1 if ok, 0 if error
 */

int
jabber_upgrade_save ()
{
    int rc;
    struct t_upgrade_file *upgrade_file;
    
    upgrade_file = weechat_upgrade_create (JABBER_UPGRADE_FILENAME, 1);
    if (!upgrade_file)
        return 0;
    
    rc = jabber_upgrade_save_all_data (upgrade_file);
    
    weechat_upgrade_close (upgrade_file);
    
    return rc;
}

/*
 * jabber_upgrade_set_buffer_callbacks: restore buffers callbacks (input and
 *                                      close) for buffers created by Jabber
 *                                      plugin
 */

void
jabber_upgrade_set_buffer_callbacks ()
{
    struct t_infolist *infolist;
    struct t_gui_buffer *ptr_buffer;
    
    infolist = weechat_infolist_get ("buffer", NULL, NULL);
    if (infolist)
    {
        while (weechat_infolist_next (infolist))
        {
            if (weechat_infolist_pointer (infolist, "plugin") == weechat_jabber_plugin)
            {
                ptr_buffer = weechat_infolist_pointer (infolist, "pointer");
                weechat_buffer_set_pointer (ptr_buffer, "close_callback", &jabber_buffer_close_cb);
                weechat_buffer_set_pointer (ptr_buffer, "input_callback", &jabber_input_data_cb);
            }
        }
    }
}

/*
 * jabber_upgrade_read_cb: read callback for upgrade
 */

int
jabber_upgrade_read_cb (int object_id,
                        struct t_infolist *infolist)
{
    int flags, size, i, index;
    char *buf, option_name[64];
    const char *buffer_name, *str, *buddy;
    struct t_jabber_buddy *ptr_buddy;
    struct t_gui_buffer *ptr_buffer;
    
    weechat_infolist_reset_item_cursor (infolist);
    while (weechat_infolist_next (infolist))
    {
        switch (object_id)
        {
            case JABBER_UPGRADE_TYPE_SERVER:
                jabber_upgrade_current_server = jabber_server_search (weechat_infolist_string (infolist, "name"));
                if (jabber_upgrade_current_server)
                {
                    jabber_upgrade_current_server->temp_server =
                        weechat_infolist_integer (infolist, "temp_server");
                    jabber_upgrade_current_server->buffer = NULL;
                    buffer_name = weechat_infolist_string (infolist, "buffer_name");
                    if (buffer_name && buffer_name[0])
                    {
                        ptr_buffer = weechat_buffer_search (JABBER_PLUGIN_NAME,
                                                            buffer_name);
                        if (ptr_buffer)
                        {
                            jabber_upgrade_current_server->buffer = ptr_buffer;
                            if (weechat_config_boolean (jabber_config_look_one_server_buffer)
                                && !jabber_buffer_servers)
                            {
                                jabber_buffer_servers = ptr_buffer;
                            }
                            if (weechat_infolist_integer (infolist, "selected"))
                                jabber_current_server = jabber_upgrade_current_server;
                        }
                    }
                    jabber_upgrade_current_server->reconnect_start = weechat_infolist_time (infolist, "reconnect_start");
                    jabber_upgrade_current_server->command_time = weechat_infolist_time (infolist, "command_time");
                    jabber_upgrade_current_server->reconnect_join = weechat_infolist_integer (infolist, "reconnect_join");
                    jabber_upgrade_current_server->disable_autojoin = weechat_infolist_integer (infolist, "disable_autojoin");
                    jabber_upgrade_current_server->is_away = weechat_infolist_integer (infolist, "is_away");
                    str = weechat_infolist_string (infolist, "away_message");
                    if (str)
                        jabber_upgrade_current_server->away_message = strdup (str);
                    jabber_upgrade_current_server->away_time = weechat_infolist_time (infolist, "away_time");
                    jabber_upgrade_current_server->lag = weechat_infolist_integer (infolist, "lag");
                    buf = weechat_infolist_buffer (infolist, "lag_check_time", &size);
                    if (buf)
                        memcpy (&(jabber_upgrade_current_server->lag_check_time), buf, size);
                    jabber_upgrade_current_server->lag_next_check = weechat_infolist_time (infolist, "lag_next_check");
                }
                break;
            case JABBER_UPGRADE_TYPE_MUC:
                if (jabber_upgrade_current_server)
                {
                    jabber_upgrade_current_muc = jabber_muc_new (jabber_upgrade_current_server,
                                                                 weechat_infolist_integer (infolist, "type"),
                                                                 weechat_infolist_string (infolist, "name"),
                                                                 0, 0);
                    if (jabber_upgrade_current_muc)
                    {
                        str = weechat_infolist_string (infolist, "topic");
                        if (str)
                            jabber_muc_set_topic (jabber_upgrade_current_muc, str);
                        str = weechat_infolist_string (infolist, "modes");
                        if (str)
                            jabber_upgrade_current_muc->modes = strdup (str);
                        jabber_upgrade_current_muc->limit = weechat_infolist_integer (infolist, "limit");
                        str = weechat_infolist_string (infolist, "key");
                        if (str)
                            jabber_upgrade_current_muc->key = strdup (str);
                        str = weechat_infolist_string (infolist, "away_message");
                        if (str)
                            jabber_upgrade_current_muc->away_message = strdup (str);
                        jabber_upgrade_current_muc->nick_completion_reset = weechat_infolist_integer (infolist, "nick_completion_reset");
                        for (i = 0; i < 2; i++)
                        {
                            index = 0;
                            while (1)
                            {
                                snprintf (option_name, sizeof (option_name),
                                          "buddy_speaking%d_%05d", i, index);
                                buddy = weechat_infolist_string (infolist, option_name);
                                if (!buddy)
                                    break;
                                jabber_muc_buddy_speaking_add (jabber_upgrade_current_muc,
                                                               buddy,
                                                               i);
                                index++;
                            }
                        }
                        index = 0;
                        while (1)
                        {
                            snprintf (option_name, sizeof (option_name),
                                      "buddy_speaking_time_buddy_%05d", index);
                            buddy = weechat_infolist_string (infolist, option_name);
                            if (!buddy)
                                break;
                            snprintf (option_name, sizeof (option_name),
                                      "buddy_speaking_time_time_%05d", index);
                            jabber_muc_buddy_speaking_time_add (jabber_upgrade_current_muc,
                                                                buddy,
                                                                weechat_infolist_time (infolist,
                                                                                       option_name));
                            index++;
                        }
                    }
                }
                break;
            case JABBER_UPGRADE_TYPE_BUDDY:
                if (jabber_upgrade_current_server)
                {
                    flags = weechat_infolist_integer (infolist, "flags");
                    ptr_buddy = jabber_buddy_new (jabber_upgrade_current_server,
                                                  jabber_upgrade_current_muc,
                                                  weechat_infolist_string (infolist, "name"),
                                                  flags & JABBER_BUDDY_CHANOWNER,
                                                  flags & JABBER_BUDDY_CHANADMIN,
                                                  flags & JABBER_BUDDY_CHANADMIN2,
                                                  flags & JABBER_BUDDY_OP,
                                                  flags & JABBER_BUDDY_HALFOP,
                                                  flags & JABBER_BUDDY_VOICE,
                                                  flags & JABBER_BUDDY_CHANUSER,
                                                  flags & JABBER_BUDDY_AWAY);
                    if (ptr_buddy)
                    {
                        str = weechat_infolist_string (infolist, "host");
                        if (str)
                            ptr_buddy->host = strdup (str);
                    }
                }
                break;
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_upgrade_load: load upgrade file
 *                      return 1 if ok, 0 if error
 */

int
jabber_upgrade_load ()
{
    int rc;
    struct t_upgrade_file *upgrade_file;

    jabber_upgrade_set_buffer_callbacks ();
    
    upgrade_file = weechat_upgrade_create (JABBER_UPGRADE_FILENAME, 0);
    rc = weechat_upgrade_read (upgrade_file, &jabber_upgrade_read_cb);
    
    return rc;
}
