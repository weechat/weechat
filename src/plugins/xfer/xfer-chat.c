/*
 * xfer-chat.c - chat with direct connection to remote host
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "../weechat-plugin.h"
#include "xfer.h"
#include "xfer-chat.h"
#include "xfer-buffer.h"
#include "xfer-config.h"


/*
 * Returns color name for tags (replace "," by ":").
 *
 * Note: result must be freed after use.
 */

char *
xfer_chat_color_for_tags (const char *color)
{
    if (!color)
        return NULL;

    return weechat_string_replace (color, ",", ":");
}

/*
 * Sends data to remote host via xfer chat.
 */

int
xfer_chat_send (struct t_xfer *xfer, const char *buffer, int size_buf)
{
    if (!xfer)
        return -1;

    return send (xfer->sock, buffer, size_buf, 0);
}

/*
 * Sends formatted data to remote host via DCC CHAT.
 */

void
xfer_chat_sendf (struct t_xfer *xfer, const char *format, ...)
{
    char *ptr_msg, *msg_encoded;

    if (!xfer || (xfer->sock < 0))
        return;

    weechat_va_format (format);
    if (!vbuffer)
        return;

    msg_encoded = (xfer->charset_modifier) ?
        weechat_hook_modifier_exec ("charset_encode",
                                    xfer->charset_modifier,
                                    vbuffer) : NULL;

    ptr_msg = (msg_encoded) ? msg_encoded : vbuffer;

    if (xfer_chat_send (xfer, ptr_msg, strlen (ptr_msg)) <= 0)
    {
        weechat_printf (NULL,
                        _("%s%s: error sending data to \"%s\" via xfer chat"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME,
                        xfer->remote_nick);
        xfer_close (xfer, XFER_STATUS_FAILED);
    }

    if (msg_encoded)
        free (msg_encoded);

    free (vbuffer);
}

/*
 * Receives data from xfer chat remote host.
 */

int
xfer_chat_recv_cb (const void *pointer, void *data, int fd)
{
    struct t_xfer *xfer;
    static char buffer[4096 + 2];
    char *buf2, *pos, *ptr_buf, *ptr_buf2, *next_ptr_buf;
    char *ptr_buf_decoded, *ptr_buf_without_weechat_colors, *ptr_buf_color;
    char str_tags[256], *str_color;
    const char *pv_tags;
    int num_read, length, ctcp_action;

    /* make C compiler happy */
    (void) data;
    (void) fd;

    xfer = (struct t_xfer *)pointer;

    num_read = recv (xfer->sock, buffer, sizeof (buffer) - 2, 0);
    if (num_read > 0)
    {
        buffer[num_read] = '\0';

        buf2 = NULL;
        ptr_buf = buffer;
        if (xfer->unterminated_message)
        {
            buf2 = malloc (strlen (xfer->unterminated_message) +
                                   strlen (buffer) + 1);
            if (buf2)
            {
                strcpy (buf2, xfer->unterminated_message);
                strcat (buf2, buffer);
            }
            ptr_buf = buf2;
            free (xfer->unterminated_message);
            xfer->unterminated_message = NULL;
        }

        while (ptr_buf && ptr_buf[0])
        {
            next_ptr_buf = NULL;
            pos = strstr (ptr_buf, "\n");
            if (pos)
            {
                pos[0] = '\0';
                next_ptr_buf = pos + 1;
            }
            else
            {
                xfer->unterminated_message = strdup (ptr_buf);
                ptr_buf = NULL;
                next_ptr_buf = NULL;
            }

            if (ptr_buf)
            {
                ctcp_action = 0;
                length = strlen (ptr_buf);
                if (ptr_buf[length - 1] == '\r')
                {
                    ptr_buf[length - 1] = '\0';
                    length--;
                }

                if ((ptr_buf[0] == '\01')
                    && (ptr_buf[length - 1] == '\01'))
                {
                    ptr_buf[length - 1] = '\0';
                    ptr_buf++;
                    if (strncmp (ptr_buf, "ACTION ", 7) == 0)
                    {
                        ptr_buf += 7;
                        ctcp_action = 1;
                    }
                }

                ptr_buf_decoded = (xfer->charset_modifier) ?
                    weechat_hook_modifier_exec ("charset_decode",
                                                xfer->charset_modifier,
                                                ptr_buf) : NULL;
                ptr_buf_without_weechat_colors = weechat_string_remove_color ((ptr_buf_decoded) ? ptr_buf_decoded : ptr_buf,
                                                                              "?");
                ptr_buf_color = weechat_hook_modifier_exec ("irc_color_decode",
                                                            "1",
                                                            (ptr_buf_without_weechat_colors) ?
                                                            ptr_buf_without_weechat_colors : ((ptr_buf_decoded) ? ptr_buf_decoded : ptr_buf));
                ptr_buf2 = (ptr_buf_color) ?
                    ptr_buf_color : ((ptr_buf_without_weechat_colors) ?
                                     ptr_buf_without_weechat_colors : ((ptr_buf_decoded) ? ptr_buf_decoded : ptr_buf));
                pv_tags = weechat_config_string (xfer_config_look_pv_tags);
                if (ctcp_action)
                {
                    snprintf (str_tags, sizeof (str_tags),
                              "irc_privmsg,irc_action,%s%snick_%s,log1",
                              (pv_tags && pv_tags[0]) ? pv_tags : "",
                              (pv_tags && pv_tags[0]) ? "," : "",
                              xfer->remote_nick);
                    weechat_printf_date_tags (
                        xfer->buffer,
                        0,
                        str_tags,
                        "%s%s%s%s%s%s",
                        weechat_prefix ("action"),
                        weechat_color ((xfer->remote_nick_color) ?
                                       xfer->remote_nick_color : "chat_nick_other"),
                        xfer->remote_nick,
                        weechat_color ("chat"),
                        (ptr_buf2[0]) ? " " : "",
                        ptr_buf2);
                }
                else
                {
                    str_color = xfer_chat_color_for_tags (
                        (xfer->remote_nick_color) ?
                        xfer->remote_nick_color : weechat_config_color (weechat_config_get ("weechat.color.chat_nick_other")));
                    snprintf (str_tags, sizeof (str_tags),
                              "irc_privmsg,%s%sprefix_nick_%s,nick_%s,log1",
                              (pv_tags && pv_tags[0]) ? pv_tags : "",
                              (pv_tags && pv_tags[0]) ? "," : "",
                              (str_color) ? str_color : "default",
                              xfer->remote_nick);
                    if (str_color)
                        free (str_color);
                    weechat_printf_date_tags (
                        xfer->buffer,
                        0,
                        str_tags,
                        "%s%s\t%s",
                        weechat_color (
                            (xfer->remote_nick_color) ?
                            xfer->remote_nick_color : "chat_nick_other"),
                        xfer->remote_nick,
                        ptr_buf2);
                }
                if (ptr_buf_decoded)
                    free (ptr_buf_decoded);
                if (ptr_buf_without_weechat_colors)
                    free (ptr_buf_without_weechat_colors);
                if (ptr_buf_color)
                    free (ptr_buf_color);
            }

            ptr_buf = next_ptr_buf;
        }

        if (buf2)
            free (buf2);
    }
    else
    {
        xfer_close (xfer, XFER_STATUS_ABORTED);
        xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback called when user sends data to xfer chat buffer.
 */

int
xfer_chat_buffer_input_cb (const void *pointer, void *data,
                           struct t_gui_buffer *buffer,
                           const char *input_data)
{
    struct t_xfer *ptr_xfer;
    char *input_data_color, str_tags[256], *str_color;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_xfer = xfer_search_by_buffer (buffer);

    if (ptr_xfer)
    {
        if (!XFER_HAS_ENDED(ptr_xfer->status))
        {
            xfer_chat_sendf (ptr_xfer, "%s\r\n", input_data);
            if (!XFER_HAS_ENDED(ptr_xfer->status))
            {
                str_color = xfer_chat_color_for_tags (weechat_config_color (weechat_config_get ("weechat.color.chat_nick_self")));
                snprintf (str_tags, sizeof (str_tags),
                          "irc_privmsg,no_highlight,prefix_nick_%s,nick_%s,log1",
                          (str_color) ? str_color : "default",
                          ptr_xfer->local_nick);
                if (str_color)
                    free (str_color);
                input_data_color = weechat_hook_modifier_exec ("irc_color_decode",
                                                               "1",
                                                               input_data);
                weechat_printf_date_tags (
                    buffer,
                    0,
                    str_tags,
                    "%s%s\t%s",
                    weechat_color ("chat_nick_self"),
                    ptr_xfer->local_nick,
                    (input_data_color) ? input_data_color : input_data);
                if (input_data_color)
                    free (input_data_color);
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback called when a buffer with direct chat is closed.
 */

int
xfer_chat_buffer_close_cb (const void *pointer, void *data,
                           struct t_gui_buffer *buffer)
{
    struct t_xfer *ptr_xfer;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    for (ptr_xfer = xfer_list; ptr_xfer; ptr_xfer = ptr_xfer->next_xfer)
    {
        if (ptr_xfer->buffer == buffer)
        {
            if (!XFER_HAS_ENDED(ptr_xfer->status))
            {
                xfer_close (ptr_xfer, XFER_STATUS_ABORTED);
                xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
            }
            ptr_xfer->buffer = NULL;
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Creates buffer for DCC chat.
 */

void
xfer_chat_open_buffer (struct t_xfer *xfer)
{
    char *name;
    int length, buffer_created;

    buffer_created = 0;

    length = strlen (xfer->plugin_name) + 8 + strlen (xfer->plugin_id) + 1
        + strlen (xfer->remote_nick) + 1;
    name = malloc (length);
    if (!name)
        return;

    snprintf (name, length, "%s_dcc.%s.%s",
              xfer->plugin_name, xfer->plugin_id, xfer->remote_nick);
    xfer->buffer = weechat_buffer_search (XFER_PLUGIN_NAME, name);
    if (!xfer->buffer)
    {
        xfer->buffer = weechat_buffer_new (
            name,
            &xfer_chat_buffer_input_cb, NULL, NULL,
            &xfer_chat_buffer_close_cb, NULL, NULL);
        buffer_created = 1;

        /* failed to create buffer ? then return */
        if (!xfer->buffer)
        {
            free (name);
            return;
        }
    }

    if (buffer_created)
    {
        weechat_buffer_set (xfer->buffer, "title", _("xfer chat"));
        if (!weechat_buffer_get_integer (xfer->buffer, "short_name_is_set"))
        {
            weechat_buffer_set (xfer->buffer, "short_name",
                                xfer->remote_nick);
        }
        weechat_buffer_set (xfer->buffer, "localvar_set_type", "private");
        weechat_buffer_set (xfer->buffer, "localvar_set_nick", xfer->local_nick);
        weechat_buffer_set (xfer->buffer, "localvar_set_channel", xfer->remote_nick);
        weechat_buffer_set (xfer->buffer, "localvar_set_tls_version", "cleartext");
        weechat_buffer_set (xfer->buffer, "highlight_words_add", "$nick");
    }

    weechat_printf (xfer->buffer,
                    _("%s%s: connected to %s (%s) via xfer chat"),
                    weechat_prefix ("network"),
                    XFER_PLUGIN_NAME,
                    xfer->remote_nick,
                    xfer->remote_address_str);

    free (name);
}
