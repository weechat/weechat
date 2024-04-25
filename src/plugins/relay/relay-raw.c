/*
 * relay-raw.c - functions for Relay raw data messages
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
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-raw.h"
#include "relay-buffer.h"
#include "relay-client.h"
#include "relay-config.h"
#include "relay-remote.h"


struct t_gui_buffer *relay_raw_buffer = NULL;

int relay_raw_messages_count = 0;
struct t_relay_raw_message *relay_raw_messages = NULL;
struct t_relay_raw_message *last_relay_raw_message = NULL;


/*
 * Prints a relay raw message.
 */

void
relay_raw_message_print (struct t_relay_raw_message *raw_message)
{
    if (relay_raw_buffer && raw_message)
    {
        weechat_printf_datetime_tags (relay_raw_buffer,
                                      raw_message->date,
                                      raw_message->date_usec,
                                      NULL,
                                      "%s\t%s",
                                      raw_message->prefix,
                                      raw_message->message);
    }
}

/*
 * Opens relay raw buffer.
 */

void
relay_raw_open (int switch_to_buffer)
{
    struct t_relay_raw_message *ptr_raw_message;
    struct t_hashtable *buffer_props;

    if (!relay_raw_buffer)
    {
        relay_raw_buffer = weechat_buffer_search (RELAY_PLUGIN_NAME,
                                                  RELAY_RAW_BUFFER_NAME);
        if (!relay_raw_buffer)
        {
            buffer_props = weechat_hashtable_new (
                32,
                WEECHAT_HASHTABLE_STRING,
                WEECHAT_HASHTABLE_STRING,
                NULL, NULL);
            if (buffer_props)
            {
                weechat_hashtable_set (buffer_props,
                                       "title", _("Relay raw messages"));
                weechat_hashtable_set (buffer_props,
                                       "localvar_set_type", "debug");
                weechat_hashtable_set (buffer_props,
                                       "localvar_set_server", RELAY_RAW_BUFFER_NAME);
                weechat_hashtable_set (buffer_props,
                                       "localvar_set_channel", RELAY_RAW_BUFFER_NAME);
                weechat_hashtable_set (buffer_props,
                                       "localvar_set_no_log", "1");
                /* disable all highlights on this buffer */
                weechat_hashtable_set (buffer_props, "highlight_words", "-");
            }
            relay_raw_buffer = weechat_buffer_new_props (
                RELAY_RAW_BUFFER_NAME,
                buffer_props,
                &relay_buffer_input_cb, NULL, NULL,
                &relay_buffer_close_cb, NULL, NULL);
            weechat_hashtable_free (buffer_props);

            /* failed to create buffer ? then return */
            if (!relay_raw_buffer)
                return;

            if (!weechat_buffer_get_integer (relay_raw_buffer, "short_name_is_set"))
            {
                weechat_buffer_set (relay_raw_buffer, "short_name",
                                    RELAY_RAW_BUFFER_NAME);
            }

            /* print messages in list */
            for (ptr_raw_message = relay_raw_messages; ptr_raw_message;
                 ptr_raw_message = ptr_raw_message->next_message)
            {
                relay_raw_message_print (ptr_raw_message);
            }
        }
    }

    if (relay_raw_buffer && switch_to_buffer)
        weechat_buffer_set (relay_raw_buffer, "display", "1");
}

/*
 * Frees a raw message and remove it from list.
 */

void
relay_raw_message_free (struct t_relay_raw_message *raw_message)
{
    struct t_relay_raw_message *new_raw_messages;

    if (!raw_message)
        return;

    /* remove message from raw messages list */
    if (last_relay_raw_message == raw_message)
        last_relay_raw_message = raw_message->prev_message;
    if (raw_message->prev_message)
    {
        (raw_message->prev_message)->next_message = raw_message->next_message;
        new_raw_messages = relay_raw_messages;
    }
    else
        new_raw_messages = raw_message->next_message;

    if (raw_message->next_message)
        (raw_message->next_message)->prev_message = raw_message->prev_message;

    /* free data */
    free (raw_message->prefix);
    free (raw_message->message);

    free (raw_message);

    relay_raw_messages = new_raw_messages;

    relay_raw_messages_count--;
}

/*
 * Frees all raw messages.
 */

void
relay_raw_message_free_all ()
{
    while (relay_raw_messages)
    {
        relay_raw_message_free (relay_raw_messages);
    }
}

/*
 * Removes old raw messages if limit has been reached.
 */

void
relay_raw_message_remove_old ()
{
    int max_messages;

    max_messages = weechat_config_integer (relay_config_look_raw_messages);
    while (relay_raw_messages && (relay_raw_messages_count >= max_messages))
    {
        relay_raw_message_free (relay_raw_messages);
    }
}

/*
 * Adds a new raw message to list.
 *
 * Returns pointer to new raw message, NULL if error.
 */

struct t_relay_raw_message *
relay_raw_message_add_to_list (time_t date, int date_usec,
                               const char *prefix, const char *message)
{
    struct t_relay_raw_message *new_raw_message;

    if (!prefix || !message)
        return NULL;

    relay_raw_message_remove_old ();

    new_raw_message = malloc (sizeof (*new_raw_message));
    if (new_raw_message)
    {
        new_raw_message->date = date;
        new_raw_message->date_usec = date_usec;
        new_raw_message->prefix = strdup (prefix);
        new_raw_message->message = strdup (message);

        /* add message to list */
        new_raw_message->prev_message = last_relay_raw_message;
        new_raw_message->next_message = NULL;
        if (last_relay_raw_message)
            last_relay_raw_message->next_message = new_raw_message;
        else
            relay_raw_messages = new_raw_message;
        last_relay_raw_message = new_raw_message;

        relay_raw_messages_count++;
    }

    return new_raw_message;
}

/*
 * Converts a binary message for raw display.
 */

char *
relay_raw_convert_binary_message (const char *data, int data_size)
{
    return weechat_string_hex_dump (data, data_size, 16, "  > ", NULL);
}

/*
 * Converts a text message for raw display.
 */

char *
relay_raw_convert_text_message (const char *data)
{
    const unsigned char *ptr_buf;
    const char *hexa = "0123456789ABCDEF";
    char *buf, *buf2;
    int i, pos_buf, pos_buf2, char_size;

    buf = weechat_iconv_to_internal (NULL, data);
    if (!buf)
        return NULL;
    buf2 = weechat_string_replace (buf, "\r", "");
    free (buf);
    if (!buf2)
        return NULL;
    buf = buf2;
    buf2 = malloc ((strlen (buf) * 4) + 1);
    if (buf2)
    {
        ptr_buf = (unsigned char *)buf;
        pos_buf = 0;
        pos_buf2 = 0;
        while (ptr_buf[pos_buf])
        {
            if ((ptr_buf[pos_buf] < 32) && (ptr_buf[pos_buf] != '\n'))
            {
                buf2[pos_buf2++] = '\\';
                buf2[pos_buf2++] = 'x';
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
    free (buf);
    return buf2;
}

/*
 * Adds a new raw message to list.
 */

void
relay_raw_message_add (enum t_relay_msg_type msg_type,
                       int flags,
                       const char *peer_id,
                       const char *data, int data_size)
{
    char *raw_data, *buf, prefix[1024], prefix_arrow[16];
    int length;
    struct t_relay_raw_message *new_raw_message;
    struct timeval tv_now;

    if (flags & RELAY_RAW_FLAG_BINARY)
        raw_data = relay_raw_convert_binary_message (data, data_size);
    else
        raw_data = relay_raw_convert_text_message (data);

    if (!raw_data)
        return;

    snprintf (prefix, sizeof (prefix), " ");

    if (!(flags & RELAY_RAW_FLAG_BINARY)
        || (msg_type == RELAY_MSG_PING)
        || (msg_type == RELAY_MSG_PONG)
        || (msg_type == RELAY_MSG_CLOSE))
    {
        /* build prefix with arrow */
        prefix_arrow[0] = '\0';
        switch (flags & (RELAY_RAW_FLAG_RECV | RELAY_RAW_FLAG_SEND))
        {
            case RELAY_RAW_FLAG_RECV:
                strcpy (prefix_arrow, RELAY_RAW_PREFIX_RECV);
                break;
            case RELAY_RAW_FLAG_SEND:
                strcpy (prefix_arrow, RELAY_RAW_PREFIX_SEND);
                break;
            default:
                if (flags & RELAY_RAW_FLAG_RECV)
                    strcpy (prefix_arrow, RELAY_RAW_PREFIX_RECV);
                else
                    strcpy (prefix_arrow, RELAY_RAW_PREFIX_SEND);
                break;
        }

        snprintf (prefix, sizeof (prefix), "%s%s%s%s",
                  (flags & RELAY_RAW_FLAG_SEND) ?
                  weechat_color ("chat_prefix_quit") :
                  weechat_color ("chat_prefix_join"),
                  prefix_arrow,
                  (peer_id && peer_id[0]) ? " " : "",
                  (peer_id && peer_id[0]) ? peer_id : "");
    }

    length = strlen (relay_msg_type_string[msg_type]) + strlen (raw_data) + 1;
    buf = malloc (length);
    if (buf)
    {
        snprintf (buf, length, "%s%s",
                  relay_msg_type_string[msg_type],
                  raw_data);
    }

    gettimeofday (&tv_now, NULL);
    new_raw_message = relay_raw_message_add_to_list (
        tv_now.tv_sec,
        tv_now.tv_usec,
        prefix,
        (buf) ? buf : raw_data);

    if (new_raw_message)
    {
        if (relay_raw_buffer)
            relay_raw_message_print (new_raw_message);
        if (weechat_config_integer (relay_config_look_raw_messages) == 0)
            relay_raw_message_free (new_raw_message);
    }

    free (buf);
    free (raw_data);
}

/*
 * Prints a message for a client on relay raw buffer.
 */

void
relay_raw_print_client (struct t_relay_client *client,
                        enum t_relay_msg_type msg_type,
                        int flags,
                        const char *data, int data_size)
{
    char peer_id[256];

    /* auto-open relay raw buffer if debug for irc plugin is >= 1 */
    if (!relay_raw_buffer && (weechat_relay_plugin->debug >= 1))
        relay_raw_open (0);

    if (client)
    {
        snprintf (peer_id, sizeof (peer_id), "%s[%s%d%s] %s%s%s%s",
                  weechat_color ("chat_delimiters"),
                  weechat_color ("chat"),
                  client->id,
                  weechat_color ("chat_delimiters"),
                  weechat_color ("chat_server"),
                  relay_protocol_string[client->protocol],
                  (client->protocol_args) ? "." : "",
                  (client->protocol_args) ? client->protocol_args : "");
    }
    else
    {
        peer_id[0] = '\0';
    }

    relay_raw_message_add (msg_type, flags, peer_id, data, data_size);
}

/*
 * Prints a message for a remote on relay raw buffer.
 */

void
relay_raw_print_remote (struct t_relay_remote *remote,
                        enum t_relay_msg_type msg_type,
                        int flags,
                        const char *data, int data_size)
{
    char peer_id[256];

    /* auto-open relay raw buffer if debug for irc plugin is >= 1 */
    if (!relay_raw_buffer && (weechat_relay_plugin->debug >= 1))
        relay_raw_open (0);

    if (remote)
    {
        snprintf (peer_id, sizeof (peer_id), "%s<%sR%s> %s%s",
                  weechat_color ("chat_delimiters"),
                  weechat_color ("chat"),
                  weechat_color ("chat_delimiters"),
                  weechat_color ("chat_server"),
                  remote->name);
    }
    else
    {
        peer_id[0] = '\0';
    }

    relay_raw_message_add (msg_type, flags, peer_id, data, data_size);
}

/*
 * Adds a raw message in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
relay_raw_add_to_infolist (struct t_infolist *infolist,
                           struct t_relay_raw_message *raw_message)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !raw_message)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_time (ptr_item, "date", raw_message->date))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "date_usec", raw_message->date_usec))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "prefix", raw_message->prefix))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "message", raw_message->message))
        return 0;

    return 1;
}
