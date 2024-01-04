/*
 * relay-upgrade.c - save/restore relay plugin data when upgrading WeeChat
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-upgrade.h"
#include "relay-buffer.h"
#include "relay-client.h"
#include "relay-raw.h"
#include "relay-server.h"


/*
 * Saves relay data in relay upgrade file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
relay_upgrade_save_all_data (struct t_upgrade_file *upgrade_file,
                             int force_disconnected_state)
{
    struct t_infolist *infolist;
    struct t_relay_server *ptr_server;
    struct t_relay_client *ptr_client;
    struct t_relay_raw_message *ptr_raw_message;
    int rc;

    /* save servers */
    for (ptr_server = relay_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        infolist = weechat_infolist_new ();
        if (!infolist)
            return 0;
        if (!relay_server_add_to_infolist (infolist, ptr_server))
        {
            weechat_infolist_free (infolist);
            return 0;
        }
        rc = weechat_upgrade_write_object (upgrade_file,
                                           RELAY_UPGRADE_TYPE_SERVER,
                                           infolist);
        weechat_infolist_free (infolist);
        if (!rc)
            return 0;
    }

    /* save clients */
    for (ptr_client = last_relay_client; ptr_client;
         ptr_client = ptr_client->prev_client)
    {
        infolist = weechat_infolist_new ();
        if (!infolist)
            return 0;
        if (!relay_client_add_to_infolist (infolist, ptr_client,
                                           force_disconnected_state))
        {
            weechat_infolist_free (infolist);
            return 0;
        }
        rc = weechat_upgrade_write_object (upgrade_file,
                                           RELAY_UPGRADE_TYPE_CLIENT,
                                           infolist);
        weechat_infolist_free (infolist);
        if (!rc)
            return 0;
    }

    /* save raw messages */
    for (ptr_raw_message = relay_raw_messages; ptr_raw_message;
         ptr_raw_message = ptr_raw_message->next_message)
    {
        infolist = weechat_infolist_new ();
        if (!infolist)
            return 0;
        if (!relay_raw_add_to_infolist (infolist, ptr_raw_message))
        {
            weechat_infolist_free (infolist);
            return 0;
        }
        rc = weechat_upgrade_write_object (upgrade_file,
                                           RELAY_UPGRADE_TYPE_RAW_MESSAGE,
                                           infolist);
        weechat_infolist_free (infolist);
        if (!rc)
            return 0;
    }

    return 1;
}

/*
 * Saves relay upgrade file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
relay_upgrade_save (int force_disconnected_state)
{
    int rc;
    struct t_upgrade_file *upgrade_file;

    upgrade_file = weechat_upgrade_new (RELAY_UPGRADE_FILENAME,
                                        NULL, NULL, NULL);
    if (!upgrade_file)
        return 0;

    rc = relay_upgrade_save_all_data (upgrade_file, force_disconnected_state);

    weechat_upgrade_close (upgrade_file);

    return rc;
}

/*
 * Restores buffer callbacks (input and close) for buffers created by relay
 * plugin.
 */

void
relay_upgrade_set_buffer_callbacks ()
{
    struct t_infolist *infolist;
    struct t_gui_buffer *ptr_buffer;

    infolist = weechat_infolist_get ("buffer", NULL, NULL);
    if (infolist)
    {
        while (weechat_infolist_next (infolist))
        {
            if (weechat_infolist_pointer (infolist, "plugin") == weechat_relay_plugin)
            {
                ptr_buffer = weechat_infolist_pointer (infolist, "pointer");
                weechat_buffer_set_pointer (ptr_buffer, "close_callback", &relay_buffer_close_cb);
                weechat_buffer_set_pointer (ptr_buffer, "input_callback", &relay_buffer_input_cb);
                if (strcmp (weechat_infolist_string (infolist, "name"),
                            RELAY_BUFFER_NAME) == 0)
                {
                    relay_buffer = ptr_buffer;
                }
                if (strcmp (weechat_infolist_string (infolist, "name"),
                            RELAY_RAW_BUFFER_NAME) == 0)
                {
                    relay_raw_buffer = ptr_buffer;
                }
            }
        }
        weechat_infolist_free (infolist);
    }
}

/*
 * Reads relay upgrade file.
 */

int
relay_upgrade_read_cb (const void *pointer, void *data,
                       struct t_upgrade_file *upgrade_file,
                       int object_id,
                       struct t_infolist *infolist)
{
    const char *str;
    struct t_relay_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) upgrade_file;

    weechat_infolist_reset_item_cursor (infolist);
    while (weechat_infolist_next (infolist))
    {
        switch (object_id)
        {
            case RELAY_UPGRADE_TYPE_SERVER:
                str = weechat_infolist_string (infolist, "protocol_string");
                if (str)
                {
                    ptr_server = relay_server_search (str);
                    if (ptr_server)
                    {
                        ptr_server->last_client_disconnect =
                            weechat_infolist_time (infolist,
                                                   "last_client_disconnect");
                    }
                }
                break;
            case RELAY_UPGRADE_TYPE_CLIENT:
                relay_client_new_with_infolist (infolist);
                break;
            case RELAY_UPGRADE_TYPE_RAW_MESSAGE:
                relay_raw_message_add_to_list (
                    weechat_infolist_time (infolist, "date"),
                    weechat_infolist_integer (infolist, "date_usec"),
                    weechat_infolist_string (infolist, "prefix"),
                    weechat_infolist_string (infolist, "message"));
                break;
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Loads relay upgrade file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
relay_upgrade_load ()
{
    int rc;
    struct t_upgrade_file *upgrade_file;

    relay_upgrade_set_buffer_callbacks ();

    upgrade_file = weechat_upgrade_new (RELAY_UPGRADE_FILENAME,
                                        &relay_upgrade_read_cb, NULL, NULL);
    if (!upgrade_file)
        return 0;

    rc = weechat_upgrade_read (upgrade_file);

    weechat_upgrade_close (upgrade_file);

    relay_buffer_refresh (NULL);

    return rc;
}
