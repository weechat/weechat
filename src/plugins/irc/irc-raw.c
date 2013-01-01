/*
 * irc-raw.c - functions for IRC raw data messages
 *
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-raw.h"
#include "irc-buffer.h"
#include "irc-config.h"
#include "irc-input.h"
#include "irc-server.h"


struct t_gui_buffer *irc_raw_buffer = NULL;

int irc_raw_messages_count = 0;
struct t_irc_raw_message *irc_raw_messages = NULL;
struct t_irc_raw_message *last_irc_raw_message = NULL;


/*
 * Prints an irc raw message.
 */

void
irc_raw_message_print (struct t_irc_raw_message *raw_message)
{
    if (irc_raw_buffer && raw_message)
    {
        weechat_printf_date_tags (irc_raw_buffer,
                                  raw_message->date, NULL,
                                  "%s\t%s",
                                  raw_message->prefix,
                                  raw_message->message);
    }
}

/*
 * Opens IRC raw buffer.
 */

void
irc_raw_open (int switch_to_buffer)
{
    struct t_irc_raw_message *ptr_raw_message;

    if (!irc_raw_buffer)
    {
        irc_raw_buffer = weechat_buffer_search (IRC_PLUGIN_NAME,
                                                IRC_RAW_BUFFER_NAME);
        if (!irc_raw_buffer)
        {
            irc_raw_buffer = weechat_buffer_new (IRC_RAW_BUFFER_NAME,
                                                 &irc_input_data_cb, NULL,
                                                 &irc_buffer_close_cb, NULL);

            /* failed to create buffer ? then return */
            if (!irc_raw_buffer)
                return;

            weechat_buffer_set (irc_raw_buffer,
                                "title", _("IRC raw messages"));

            if (!weechat_buffer_get_integer (irc_raw_buffer, "short_name_is_set"))
            {
                weechat_buffer_set (irc_raw_buffer, "short_name",
                                    IRC_RAW_BUFFER_NAME);
            }
            weechat_buffer_set (irc_raw_buffer, "localvar_set_type", "debug");
            weechat_buffer_set (irc_raw_buffer, "localvar_set_server", IRC_RAW_BUFFER_NAME);
            weechat_buffer_set (irc_raw_buffer, "localvar_set_channel", IRC_RAW_BUFFER_NAME);
            weechat_buffer_set (irc_raw_buffer, "localvar_set_no_log", "1");

            /* disable all highlights on this buffer */
            weechat_buffer_set (irc_raw_buffer, "highlight_words", "-");

            /* print messages in list */
            for (ptr_raw_message = irc_raw_messages; ptr_raw_message;
                 ptr_raw_message = ptr_raw_message->next_message)
            {
                irc_raw_message_print (ptr_raw_message);
            }
        }
    }

    if (irc_raw_buffer && switch_to_buffer)
        weechat_buffer_set (irc_raw_buffer, "display", "1");
}

/*
 * Frees a raw message and removes it from list.
 */

void
irc_raw_message_free (struct t_irc_raw_message *raw_message)
{
    struct t_irc_raw_message *new_raw_messages;

    /* remove message from raw messages list */
    if (last_irc_raw_message == raw_message)
        last_irc_raw_message = raw_message->prev_message;
    if (raw_message->prev_message)
    {
        (raw_message->prev_message)->next_message = raw_message->next_message;
        new_raw_messages = irc_raw_messages;
    }
    else
        new_raw_messages = raw_message->next_message;

    if (raw_message->next_message)
        (raw_message->next_message)->prev_message = raw_message->prev_message;

    /* free data */
    if (raw_message->prefix)
        free (raw_message->prefix);
    if (raw_message->message)
        free (raw_message->message);

    free (raw_message);

    irc_raw_messages = new_raw_messages;

    irc_raw_messages_count--;
}

/*
 * Frees all raw messages.
 */

void
irc_raw_message_free_all ()
{
    while (irc_raw_messages)
    {
        irc_raw_message_free (irc_raw_messages);
    }
}

/*
 * Removes old raw messages if limit has been reached.
 */

void
irc_raw_message_remove_old ()
{
    int max_messages;

    max_messages = weechat_config_integer (irc_config_look_raw_messages);
    while (irc_raw_messages && (irc_raw_messages_count >= max_messages))
    {
        irc_raw_message_free (irc_raw_messages);
    }
}

/*
 * Adds a new raw message to list.
 *
 * Returns pointer to new raw message, NULL if error.
 */

struct t_irc_raw_message *
irc_raw_message_add_to_list (time_t date, const char *prefix,
                             const char *message)
{
    struct t_irc_raw_message *new_raw_message;

    if (!prefix || !message)
        return NULL;

    irc_raw_message_remove_old ();

    new_raw_message = malloc (sizeof (*new_raw_message));
    if (new_raw_message)
    {
        new_raw_message->date = date;
        new_raw_message->prefix = strdup (prefix);
        new_raw_message->message = strdup (message);

        /* add message to list */
        new_raw_message->prev_message = last_irc_raw_message;
        new_raw_message->next_message = NULL;
        if (irc_raw_messages)
            last_irc_raw_message->next_message = new_raw_message;
        else
            irc_raw_messages = new_raw_message;
        last_irc_raw_message = new_raw_message;

        irc_raw_messages_count++;
    }

    return new_raw_message;
}

/*
 * Adds a new raw message to list.
 *
 * Returns pointer to new raw message, NULL if error.
 */

struct t_irc_raw_message *
irc_raw_message_add (struct t_irc_server *server, int flags,
                     const char *message)
{
    char *buf, *buf2, prefix[256], prefix_arrow[16];
    const unsigned char *ptr_buf;
    const char *hexa = "0123456789ABCDEF";
    int pos_buf, pos_buf2, char_size, i;
    struct t_irc_raw_message *new_raw_message;

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

    /* build prefix with arrow */
    prefix_arrow[0] = '\0';
    switch (flags & (IRC_RAW_FLAG_RECV | IRC_RAW_FLAG_SEND
                     | IRC_RAW_FLAG_MODIFIED | IRC_RAW_FLAG_REDIRECT))
    {
        case IRC_RAW_FLAG_RECV:
            strcpy (prefix_arrow, IRC_RAW_PREFIX_RECV);
            break;
        case IRC_RAW_FLAG_RECV | IRC_RAW_FLAG_MODIFIED:
            strcpy (prefix_arrow, IRC_RAW_PREFIX_RECV_MODIFIED);
            break;
        case IRC_RAW_FLAG_RECV | IRC_RAW_FLAG_REDIRECT:
            strcpy (prefix_arrow, IRC_RAW_PREFIX_RECV_REDIRECT);
            break;
        case IRC_RAW_FLAG_SEND:
            strcpy (prefix_arrow, IRC_RAW_PREFIX_SEND);
            break;
        case IRC_RAW_FLAG_SEND | IRC_RAW_FLAG_MODIFIED:
            strcpy (prefix_arrow, IRC_RAW_PREFIX_SEND_MODIFIED);
            break;
        default:
            if (flags & IRC_RAW_FLAG_RECV)
                strcpy (prefix_arrow, IRC_RAW_PREFIX_RECV);
            else
                strcpy (prefix_arrow, IRC_RAW_PREFIX_SEND);
            break;
    }

    snprintf (prefix, sizeof (prefix), "%s%s%s%s%s",
              (server) ? weechat_color ("chat_server") : "",
              (server) ? server->name : "",
              (server) ? " " : "",
              (flags & IRC_RAW_FLAG_SEND) ?
              weechat_color ("chat_prefix_quit") :
              weechat_color ("chat_prefix_join"),
              prefix_arrow);

    new_raw_message = irc_raw_message_add_to_list (time (NULL),
                                                   prefix,
                                                   (buf2) ? buf2 : ((buf) ? buf : message));

    if (buf)
        free (buf);
    if (buf2)
        free (buf2);

    return new_raw_message;
}

/*
 * Prints a message on IRC raw buffer.
 */

void
irc_raw_print (struct t_irc_server *server, int flags,
               const char *message)
{
    struct t_irc_raw_message *new_raw_message;

    if (!message)
        return;

    /* auto-open IRC raw buffer if debug for irc plugin is >= 1 */
    if (!irc_raw_buffer && (weechat_irc_plugin->debug >= 1))
        irc_raw_open (0);

    new_raw_message = irc_raw_message_add (server, flags, message);
    if (new_raw_message)
    {
        if (irc_raw_buffer)
            irc_raw_message_print (new_raw_message);
        if (weechat_config_integer (irc_config_look_raw_messages) == 0)
            irc_raw_message_free (new_raw_message);
    }
}

/*
 * Adds a raw message in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_raw_add_to_infolist (struct t_infolist *infolist,
                         struct t_irc_raw_message *raw_message)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !raw_message)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_time (ptr_item, "date", raw_message->date))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "prefix", raw_message->prefix))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "message", raw_message->message))
        return 0;

    return 1;
}
