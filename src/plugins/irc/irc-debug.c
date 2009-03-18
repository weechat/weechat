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

/* irc-debug.c: debug functions for IRC plugin */


#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-debug.h"
#include "irc-server.h"


struct t_gui_buffer *irc_debug_buffer = NULL;


/*
 * irc_debug_buffer_close_cb: callback called when IRC debug buffer is closed
 */

int
irc_debug_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    
    irc_debug_buffer = NULL;
    
    return WEECHAT_RC_OK;
}

/*
 * irc_debug_printf: print a message on IRC debug buffer
 */

void
irc_debug_printf (struct t_irc_server *server, int send, int modified,
                  const char *message)
{
    char *buf, *buf2;
    const unsigned char *ptr_buf;
    const char *hexa = "0123456789ABCDEF";
    int pos_buf, pos_buf2, char_size, i;
    
    if (!weechat_irc_plugin->debug || !message)
        return;
    
    if (!irc_debug_buffer)
    {
        irc_debug_buffer = weechat_buffer_search ("irc", IRC_DEBUG_BUFFER_NAME);
        if (!irc_debug_buffer)
        {
            irc_debug_buffer = weechat_buffer_new (IRC_DEBUG_BUFFER_NAME,
                                                   NULL, NULL,
                                                   &irc_debug_buffer_close_cb, NULL);

            /* failed to create buffer ? then return */
            if (!irc_debug_buffer)
                return;
            
            weechat_buffer_set (irc_debug_buffer,
                                "title", _("IRC debug messages"));
            
            weechat_buffer_set (irc_debug_buffer, "short_name", IRC_DEBUG_BUFFER_NAME);
            weechat_buffer_set (irc_debug_buffer, "localvar_set_type", "debug");
            weechat_buffer_set (irc_debug_buffer, "localvar_set_server", IRC_DEBUG_BUFFER_NAME);
            weechat_buffer_set (irc_debug_buffer, "localvar_set_channel", IRC_DEBUG_BUFFER_NAME);
            weechat_buffer_set (irc_debug_buffer, "localvar_set_no_log", "1");
            
            /* disabled all highlights on this debug buffer */
            weechat_buffer_set (irc_debug_buffer, "highlight_words", "-");
        }
    }
    
    buf = weechat_iconv_to_internal (NULL, message);
    buf2 = malloc ((strlen (buf) * 3) + 1);
    if (buf2)
    {
        ptr_buf = (buf) ? (unsigned char *)buf : (unsigned char *)message;
        pos_buf = 0;
        pos_buf2 = 0;
        while (ptr_buf[pos_buf])
        {
            if (ptr_buf[pos_buf] < 32)
            {
                buf2[pos_buf2++] = '\\';
                buf2[pos_buf2++] = hexa[ptr_buf[pos_buf] / 16];
                buf2[pos_buf2++] = hexa[ptr_buf[pos_buf] % 16];
                pos_buf++;
            }
            else
            {
                char_size = weechat_utf8_char_size ((const char *)(ptr_buf + pos_buf));
                for (i = 0; i < char_size; i++)
                {
                    buf2[pos_buf2++] = ptr_buf[pos_buf++];
                }
            }
        }
        buf2[pos_buf2] = '\0';
    }
    
    weechat_printf (irc_debug_buffer,
                    "%s%s%s%s%s\t%s",
                    (server) ? weechat_color ("chat_server") : "",
                    (server) ? server->name : "",
                    (server) ? " " : "",
                    (send) ?
                    weechat_color ("chat_prefix_quit") :
                    weechat_color ("chat_prefix_join"),
                    (send) ?
                    ((modified) ? IRC_DEBUG_PREFIX_SEND_MOD : IRC_DEBUG_PREFIX_SEND) :
                    ((modified) ? IRC_DEBUG_PREFIX_RECV_MOD : IRC_DEBUG_PREFIX_RECV),
                    (buf2) ? buf2 : ((buf) ? buf : message));
    if (buf)
        free (buf);
    if (buf2)
        free (buf2);
}

/*
 * irc_debug_signal_debug_dump_cb: dump IRC data in WeeChat log file
 */

int
irc_debug_signal_debug_dump_cb (void *data, const char *signal,
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
    
    irc_server_print_log ();
    
    weechat_log_printf ("");
    weechat_log_printf ("***** End of \"%s\" plugin dump *****",
                        weechat_plugin->name);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_debug_init: initialize debug for IRC plugin
 */

void
irc_debug_init ()
{
    weechat_hook_signal ("debug_dump", &irc_debug_signal_debug_dump_cb, NULL);
}
