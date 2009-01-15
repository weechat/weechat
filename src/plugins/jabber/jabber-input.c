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

/* jabber-input.c: Jabber input data (read from user) */


#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "jabber.h"
#include "jabber-buffer.h"
#include "jabber-server.h"
#include "jabber-muc.h"
#include "jabber-buddy.h"
#include "jabber-config.h"
#include "jabber-xmpp.h"


/*
 * jabber_input_user_message_display: display user message
 */

void
jabber_input_user_message_display (struct t_gui_buffer *buffer,
                                   const char *text)
{
    struct t_jabber_buddy *ptr_buddy;
    const char *local_name;
    
    JABBER_GET_SERVER_MUC(buffer);
    
    local_name = jabber_server_get_local_name (ptr_server);
    
    if (ptr_muc)
    {
        if (ptr_muc->type == JABBER_MUC_TYPE_MUC)
            ptr_buddy = jabber_buddy_search (NULL, ptr_muc, local_name);
        else
            ptr_buddy = NULL;
        
        weechat_printf_tags (buffer,
                             jabber_xmpp_tags ("chat_msg", "no_highlight"),
                             "%s%s",
                             jabber_buddy_as_prefix ((ptr_buddy) ? ptr_buddy : NULL,
                                                     (ptr_buddy) ? NULL : local_name,
                                                     JABBER_COLOR_CHAT_NICK_SELF),
                             text);
    }
}

/*
 * jabber_input_data_cb: callback for input data in a buffer
 */

int
jabber_input_data_cb (void *data, struct t_gui_buffer *buffer,
                   const char *input_data)
{
    const char *ptr_data;
    char *msg;
    
    /* make C compiler happy */
    (void) data;
    
    JABBER_GET_SERVER_MUC(buffer);
    
    if (ptr_muc)
    {
        ptr_data = ((input_data[0] == '/') && (input_data[1] == '/')) ?
            input_data + 1 : input_data;
        
        msg = strdup (ptr_data);
        if (msg)
        {
            jabber_xmpp_send_chat_message (ptr_server, ptr_muc, msg);
            jabber_input_user_message_display (buffer, msg);
            free (msg);
        }
    }
    else
    {
        weechat_printf (buffer,
                        _("%s: this buffer is not a MUC!"),
                        JABBER_PLUGIN_NAME);
    }
    
    return WEECHAT_RC_OK;
}
