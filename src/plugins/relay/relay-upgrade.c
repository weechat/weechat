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

/* relay-upgrade.c: save/restore relay plugin data */


#include <stdlib.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-upgrade.h"
#include "relay-buffer.h"


/*
 * relay_upgrade_save_clients: save clients info to upgrade file
 */

int
relay_upgrade_save_clients (struct t_upgrade_file *upgrade_file)
{
    /* TODO: save relay data */
    (void) upgrade_file;
    return 1;
}

/*
 * relay_upgrade_save: save upgrade file
 *                     return 1 if ok, 0 if error
 */

int
relay_upgrade_save ()
{
    int rc;
    struct t_upgrade_file *upgrade_file;
    
    upgrade_file = weechat_upgrade_create (RELAY_UPGRADE_FILENAME, 1);
    if (!upgrade_file)
        return 0;
    
    rc = relay_upgrade_save_clients (upgrade_file);
    
    weechat_upgrade_close (upgrade_file);
    
    return rc;
}

/*
 * relay_upgrade_set_buffer_callbacks: restore buffers callbacks (input and
 *                                     close) for buffers created by relay plugin
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
            }
        }
    }
}

/*
 * relay_upgrade_read_cb: read callback for relay upgrade file
 */

int
relay_upgrade_read_cb (int object_id,
                      struct t_infolist *infolist)
{
    /* TODO: write relay read cb */
    (void) object_id;
    (void) infolist;
    return WEECHAT_RC_OK;
}

/*
 * relay_upgrade_load: load upgrade file
 *                    return 1 if ok, 0 if error
 */

int
relay_upgrade_load ()
{
    int rc;
    struct t_upgrade_file *upgrade_file;
    
    relay_upgrade_set_buffer_callbacks ();
    
    upgrade_file = weechat_upgrade_create (RELAY_UPGRADE_FILENAME, 0);
    rc = weechat_upgrade_read (upgrade_file, &relay_upgrade_read_cb);
    
    return rc;
}
