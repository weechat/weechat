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

/* jabber-debug.c: debug functions for Jabber plugin */


#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "jabber.h"
#include "jabber-debug.h"
#include "jabber-server.h"


struct t_gui_buffer *jabber_debug_buffer = NULL;


/*
 * jabber_debug_buffer_close_cb: callback called when Jabber debug buffer is
 *                               closed
 */

int
jabber_debug_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    
    jabber_debug_buffer = NULL;
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_debug_printf: print a message on Jabber debug buffer
 */

void
jabber_debug_printf (struct t_jabber_server *server, int send, int modified,
                     const char *message)
{
    char *buf, *buf2;
    const char *ptr_buf;
    int pos_buf, pos_buf2, char_size, i;
    
    if (!weechat_jabber_plugin->debug || !message)
        return;
    
    if (!jabber_debug_buffer)
    {
        jabber_debug_buffer = weechat_buffer_search ("jabber",
                                                     JABBER_DEBUG_BUFFER_NAME);
        if (!jabber_debug_buffer)
        {
            jabber_debug_buffer = weechat_buffer_new (JABBER_DEBUG_BUFFER_NAME,
                                                      NULL, NULL,
                                                      &jabber_debug_buffer_close_cb, NULL);
            
            /* failed to create buffer ? then return */
            if (!jabber_debug_buffer)
                return;
            
            weechat_buffer_set (jabber_debug_buffer,
                                "title", _("Jabber debug messages"));
            
            weechat_buffer_set (jabber_debug_buffer, "short_name", JABBER_DEBUG_BUFFER_NAME);
            weechat_buffer_set (jabber_debug_buffer, "localvar_set_server", JABBER_DEBUG_BUFFER_NAME);
            weechat_buffer_set (jabber_debug_buffer, "localvar_set_muc", JABBER_DEBUG_BUFFER_NAME);
            weechat_buffer_set (jabber_debug_buffer, "localvar_set_no_log", "1");
            
            /* disabled all highlights on this debug buffer */
            weechat_buffer_set (jabber_debug_buffer, "highlight_words", "-");
        }
    }
    
    buf = weechat_iconv_to_internal (NULL, message);
    buf2 = malloc ((strlen (buf) * 3) + 1);
    if (buf2)
    {
        ptr_buf = (buf) ? buf : message;
        pos_buf = 0;
        pos_buf2 = 0;
        while (ptr_buf[pos_buf])
        {
            if (ptr_buf[pos_buf] < 32)
            {
                buf2[pos_buf2++] = '\\';
                buf2[pos_buf2++] = (ptr_buf[pos_buf] / 10) + '0';
                buf2[pos_buf2++] = (ptr_buf[pos_buf] % 10) + '0';
                pos_buf++;
            }
            else
            {
                char_size = weechat_utf8_char_size (ptr_buf + pos_buf);
                for (i = 0; i < char_size; i++)
                {
                    buf2[pos_buf2++] = ptr_buf[pos_buf++];
                }
            }
        }
        buf2[pos_buf2] = '\0';
    }
    
    weechat_printf (jabber_debug_buffer,
                    "%s%s%s%s%s%s\t%s",
                    (server) ? weechat_color ("chat_server") : "",
                    (iks_is_secure (server->iks_parser)) ? "[S]" : "",
                    (server) ? server->name : "",
                    (server) ? " " : "",
                    (send) ?
                    weechat_color ("chat_prefix_quit") :
                    weechat_color ("chat_prefix_join"),
                    (send) ?
                    ((modified) ? JABBER_DEBUG_PREFIX_SEND_MOD : JABBER_DEBUG_PREFIX_SEND) :
                    ((modified) ? JABBER_DEBUG_PREFIX_RECV_MOD : JABBER_DEBUG_PREFIX_RECV),
                    (buf2) ? buf2 : ((buf) ? buf : message));
    if (buf)
        free (buf);
    if (buf2)
        free (buf2);
}

/*
 * jabber_debug_signal_debug_dump_cb: dump Jabber data in WeeChat log file
 */

int
jabber_debug_signal_debug_dump_cb (void *data, const char *signal,
                                   const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;
    
    weechat_log_printf ("");
    weechat_log_printf ("***** \"%s\" plugin dump *****",
                        weechat_plugin->name);
    
    jabber_server_print_log ();
    
    weechat_log_printf ("");
    weechat_log_printf ("***** End of \"%s\" plugin dump *****",
                        weechat_plugin->name);
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_debug_init: initialize debug for Jabber plugin
 */

void
jabber_debug_init ()
{
    weechat_hook_signal ("debug_dump", &jabber_debug_signal_debug_dump_cb, NULL);
}
