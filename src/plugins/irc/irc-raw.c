/*
 * irc-raw.c - functions for IRC raw data messages
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
#include <time.h>
#include <sys/time.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-raw.h"
#include "irc-buffer.h"
#include "irc-config.h"
#include "irc-message.h"
#include "irc-input.h"
#include "irc-server.h"


struct t_gui_buffer *irc_raw_buffer = NULL;

int irc_raw_messages_count = 0;
struct t_irc_raw_message *irc_raw_messages = NULL;
struct t_irc_raw_message *last_irc_raw_message = NULL;

char *irc_raw_filter = NULL;
struct t_hashtable *irc_raw_filter_hashtable_options = NULL;


/*
 * Checks if a string matches a mask.
 *
 * If mask has no "*" inside, it just checks if "mask" is inside the "string".
 * If mask has at least one "*" inside, the function weechat_string_match is
 * used.
 *
 * Returns:
 *   1: string matches mask
 *   0: string does not match mask
 */

int
irc_raw_message_string_match (const char *string, const char *mask)
{
    if (strchr (mask, '*'))
        return weechat_string_match (string, mask, 0);
    else
        return (weechat_strcasestr (string, mask)) ? 1 : 0;
}

/*
 * Checks if a messages is matching current filter(s).
 *
 * Returns:
 *   1: message is matching filter(s)
 *   0: message does not match filter(s)
 */

int
irc_raw_message_match_filter (struct t_irc_raw_message *raw_message,
                              const char *filter)
{
    int match;
    char *command, *result, str_date[128];
    struct t_hashtable *hashtable;
    struct timeval tv;

    if (!filter || !filter[0])
        return 1;

    if (strncmp (filter, "c:", 2) == 0)
    {
        /* filter by evaluated condition */
        hashtable = irc_message_parse_to_hashtable (raw_message->server,
                                                    raw_message->message);
        if (hashtable)
        {
            tv.tv_sec = raw_message->date;
            tv.tv_usec = raw_message->date_usec;
            weechat_util_strftimeval (str_date, sizeof (str_date),
                                      "%FT%T.%f", &tv);
            weechat_hashtable_set (hashtable, "date", str_date);
            weechat_hashtable_set (hashtable,
                                   "server", raw_message->server->name);
            weechat_hashtable_set (
                hashtable,
                "recv",
                (raw_message->flags & IRC_RAW_FLAG_RECV) ? "1" : "0");
            weechat_hashtable_set (
                hashtable,
                "sent",
                (raw_message->flags & IRC_RAW_FLAG_SEND) ? "1" : "0");
            weechat_hashtable_set (
                hashtable,
                "modified",
                (raw_message->flags & IRC_RAW_FLAG_MODIFIED) ? "1" : "0");
            weechat_hashtable_set (
                hashtable,
                "redirected",
                (raw_message->flags & IRC_RAW_FLAG_REDIRECT) ? "1" : "0");
        }
        result = weechat_string_eval_expression (
            filter + 2,
            NULL,
            hashtable,
            irc_raw_filter_hashtable_options);
        match = (result && (strcmp (result, "1") == 0)) ? 1 : 0;
        weechat_hashtable_free (hashtable);
        free (result);
        return match;
    }
    else if (strncmp (filter, "s:", 2) == 0)
    {
        /* filter by server name */
        return (strcmp (raw_message->server->name, filter + 2) == 0) ? 1 : 0;
    }
    else if (strncmp (filter, "f:", 2) == 0)
    {
        /* filter by message flag */
        if (strcmp (filter + 2, "recv") == 0)
        {
            return (raw_message->flags & IRC_RAW_FLAG_RECV) ? 1 : 0;
        }
        else if (strcmp (filter + 2, "sent") == 0)
        {
            return (raw_message->flags & IRC_RAW_FLAG_SEND) ? 1 : 0;
        }
        else if (strcmp (filter + 2, "modified") == 0)
        {
            return (raw_message->flags & IRC_RAW_FLAG_MODIFIED) ? 1 : 0;
        }
        else if (strcmp (filter + 2, "redirected") == 0)
        {
            return (raw_message->flags & IRC_RAW_FLAG_REDIRECT) ? 1 : 0;
        }
        return 0;
    }
    else if (strncmp (filter, "m:", 2) == 0)
    {
        /* filter by IRC command */
        irc_message_parse (raw_message->server,
                           raw_message->message,
                           NULL,  /* tags */
                           NULL,  /* message_without_tags */
                           NULL,  /* nick */
                           NULL,  /* user */
                           NULL,  /* host */
                           &command,
                           NULL,  /* channel */
                           NULL,  /* arguments */
                           NULL,  /* text */
                           NULL,  /* params */
                           NULL,  /* num_params */
                           NULL,  /* pos_command */
                           NULL,  /* pos_arguments */
                           NULL,  /* pos_channel */
                           NULL);  /* pos_text */
        match = (command && (weechat_strcasecmp (command, filter + 2) == 0)) ?
            1 : 0;
        free (command);
        return match;
    }
    else
    {
        /* filter by text in message */
        return (irc_raw_message_string_match (raw_message->message,
                                              filter)) ? 1 : 0;
    }
}

/*
 * Prints an irc raw message.
 */

void
irc_raw_message_print (struct t_irc_raw_message *raw_message)
{
    char *buf, *buf2, prefix[512], prefix_arrow[16];
    const unsigned char *ptr_buf;
    const char *hexa = "0123456789ABCDEF";
    int pos_buf, pos_buf2, char_size, i;

    if (!irc_raw_buffer || !raw_message)
        return;

    if (!irc_raw_message_match_filter (raw_message, irc_raw_filter))
        return;

    buf = NULL;
    buf2 = NULL;

    if (raw_message->flags & IRC_RAW_FLAG_BINARY)
    {
        buf = weechat_string_hex_dump (
            raw_message->message,
            strlen (raw_message->message),
            16,
            "  > ",
            NULL);
        snprintf (prefix, sizeof (prefix), " ");
    }
    else
    {
        buf = weechat_iconv_to_internal (NULL, raw_message->message);
        buf2 = malloc ((strlen (buf) * 4) + 1);
        if (buf2)
        {
            ptr_buf = (buf) ?
                (unsigned char *)buf : (unsigned char *)(raw_message->message);
            pos_buf = 0;
            pos_buf2 = 0;
            while (ptr_buf[pos_buf])
            {
                if ((ptr_buf[pos_buf] < 32)
                    || !weechat_utf8_is_valid ((const char *)(ptr_buf + pos_buf),
                                               1, NULL))
                {
                    buf2[pos_buf2++] = '\\';
                    buf2[pos_buf2++] = 'x';
                    buf2[pos_buf2++] = hexa[ptr_buf[pos_buf] / 16];
                    buf2[pos_buf2++] = hexa[ptr_buf[pos_buf] % 16];
                    pos_buf++;
                }
                else if (ptr_buf[pos_buf] == '\\')
                {
                    buf2[pos_buf2++] = '\\';
                    buf2[pos_buf2++] = '\\';
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
        switch (raw_message->flags & (IRC_RAW_FLAG_RECV
                                      | IRC_RAW_FLAG_SEND
                                      | IRC_RAW_FLAG_MODIFIED
                                      | IRC_RAW_FLAG_REDIRECT))
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
                if (raw_message->flags & IRC_RAW_FLAG_RECV)
                    strcpy (prefix_arrow, IRC_RAW_PREFIX_RECV);
                else
                    strcpy (prefix_arrow, IRC_RAW_PREFIX_SEND);
                break;
        }

        snprintf (prefix, sizeof (prefix), "%s%s%s%s%s",
                  (raw_message->flags & IRC_RAW_FLAG_SEND) ?
                  weechat_color ("chat_prefix_quit") :
                  weechat_color ("chat_prefix_join"),
                  prefix_arrow,
                  (raw_message->server) ? weechat_color ("chat_server") : "",
                  (raw_message->server) ? " " : "",
                  (raw_message->server) ? (raw_message->server)->name : "");
    }

    weechat_printf_datetime_tags (
        irc_raw_buffer,
        raw_message->date,
        raw_message->date_usec,
        NULL,
        "%s\t%s",
        prefix,
        (buf2) ? buf2 : ((buf) ? buf : raw_message->message));

    free (buf);
    free (buf2);
}

/*
 * Sets the local variable "filter" in the irc raw buffer.
 */

void
irc_raw_set_localvar_filter ()
{
    if (!irc_raw_buffer)
        return;

    weechat_buffer_set (irc_raw_buffer, "localvar_set_filter",
                        (irc_raw_filter) ? irc_raw_filter : "*");
}

/*
 * Sets title of irc raw buffer.
 */

void
irc_raw_set_title ()
{
    char str_title[1024];

    if (!irc_raw_buffer)
        return;

    snprintf (str_title, sizeof (str_title),
              _("IRC raw messages | Filter: %s"),
              (irc_raw_filter) ? irc_raw_filter : "*");

    weechat_buffer_set (irc_raw_buffer, "title", str_title);
}

/*
 * Updates list of messages in raw buffer.
 */

void
irc_raw_refresh (int clear)
{
    struct t_irc_raw_message *ptr_raw_message;

    if (!irc_raw_buffer)
        return;

    if (clear)
        weechat_buffer_clear (irc_raw_buffer);

    /* print messages in list */
    for (ptr_raw_message = irc_raw_messages; ptr_raw_message;
         ptr_raw_message = ptr_raw_message->next_message)
    {
        irc_raw_message_print (ptr_raw_message);
    }

    irc_raw_set_title ();
}

/*
 * Opens IRC raw buffer.
 */

void
irc_raw_open (int switch_to_buffer)
{
    struct t_hashtable *buffer_props;

    if (!irc_raw_buffer)
    {
        irc_raw_buffer = weechat_buffer_search (IRC_PLUGIN_NAME,
                                                IRC_RAW_BUFFER_NAME);
        if (!irc_raw_buffer)
        {
            buffer_props = weechat_hashtable_new (
                32,
                WEECHAT_HASHTABLE_STRING,
                WEECHAT_HASHTABLE_STRING,
                NULL, NULL);
            if (buffer_props)
            {
                weechat_hashtable_set (buffer_props,
                                       "localvar_set_type", "debug");
                weechat_hashtable_set (buffer_props,
                                       "localvar_set_server", IRC_RAW_BUFFER_NAME);
                weechat_hashtable_set (buffer_props,
                                       "localvar_set_channel", IRC_RAW_BUFFER_NAME);
                weechat_hashtable_set (buffer_props,
                                       "localvar_set_no_log", "1");
                /* disable all highlights on this buffer */
                weechat_hashtable_set (buffer_props, "highlight_words", "-");
            }
            irc_raw_buffer = weechat_buffer_new_props (
                IRC_RAW_BUFFER_NAME,
                buffer_props,
                &irc_input_data_cb, NULL, NULL,
                &irc_buffer_close_cb, NULL, NULL);
            weechat_hashtable_free (buffer_props);

            /* failed to create buffer ? then return */
            if (!irc_raw_buffer)
                return;

            if (!weechat_buffer_get_integer (irc_raw_buffer, "short_name_is_set"))
            {
                weechat_buffer_set (irc_raw_buffer, "short_name",
                                    IRC_RAW_BUFFER_NAME);
            }

            irc_raw_set_localvar_filter ();

            irc_raw_refresh (0);
        }
    }

    if (irc_raw_buffer && switch_to_buffer)
        weechat_buffer_set (irc_raw_buffer, "display", "1");
}

/*
 * Sets the raw messages filter.
 */

void
irc_raw_set_filter (const char *filter)
{
    free (irc_raw_filter);
    irc_raw_filter = (filter && (strcmp (filter, "*") != 0)) ?
        strdup (filter) : NULL;
    irc_raw_set_localvar_filter ();
}

/*
 * Filters raw messages.
 */

void
irc_raw_filter_options (const char *filter)
{
    irc_raw_set_filter (filter);
    irc_raw_set_localvar_filter ();
    irc_raw_refresh (1);
}

/*
 * Frees a raw message and removes it from list.
 */

void
irc_raw_message_free (struct t_irc_raw_message *raw_message)
{
    struct t_irc_raw_message *new_raw_messages;

    if (!raw_message)
        return;

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
irc_raw_message_add_to_list (time_t date, int date_usec,
                             struct t_irc_server *server,
                             int flags, const char *message)
{
    struct t_irc_raw_message *new_raw_message;

    if (!message)
        return NULL;

    irc_raw_message_remove_old ();

    new_raw_message = malloc (sizeof (*new_raw_message));
    if (new_raw_message)
    {
        new_raw_message->date = date;
        new_raw_message->date_usec = date_usec;
        new_raw_message->server = server;
        new_raw_message->flags = flags;
        new_raw_message->message = strdup (message);

        /* add message to list */
        new_raw_message->prev_message = last_irc_raw_message;
        new_raw_message->next_message = NULL;
        if (last_irc_raw_message)
            last_irc_raw_message->next_message = new_raw_message;
        else
            irc_raw_messages = new_raw_message;
        last_irc_raw_message = new_raw_message;

        irc_raw_messages_count++;
    }

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
    struct timeval tv_now;

    if (!message)
        return;

    /* auto-open IRC raw buffer if debug for irc plugin is >= 1 */
    if (!irc_raw_buffer && (weechat_irc_plugin->debug >= 1))
        irc_raw_open (0);

    gettimeofday (&tv_now, NULL);
    new_raw_message = irc_raw_message_add_to_list (
        tv_now.tv_sec, tv_now.tv_usec, server, flags, message);
    if (new_raw_message)
    {
        if (irc_raw_buffer)
            irc_raw_message_print (new_raw_message);
        if (weechat_config_integer (irc_config_look_raw_messages) == 0)
            irc_raw_message_free (new_raw_message);
    }

    if (weechat_irc_plugin->debug >= 2)
    {
        new_raw_message = irc_raw_message_add_to_list (
            tv_now.tv_sec,
            tv_now.tv_usec,
            server,
            flags | IRC_RAW_FLAG_BINARY,
            message);
        if (new_raw_message)
        {
            if (irc_raw_buffer)
                irc_raw_message_print (new_raw_message);
            if (weechat_config_integer (irc_config_look_raw_messages) == 0)
                irc_raw_message_free (new_raw_message);
        }
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
    if (!weechat_infolist_new_var_integer (ptr_item, "date_usec", raw_message->date_usec))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "server", raw_message->server->name))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "flags", raw_message->flags))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "message", raw_message->message))
        return 0;

    return 1;
}

/*
 * Initializes irc raw.
 */

void
irc_raw_init ()
{
    irc_raw_filter_hashtable_options = weechat_hashtable_new (
        8,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (irc_raw_filter_hashtable_options)
    {
        weechat_hashtable_set (irc_raw_filter_hashtable_options,
                               "type", "condition");
    }
}

/*
 * Ends irc raw.
 */

void
irc_raw_end ()
{
    irc_raw_message_free_all ();

    if (irc_raw_buffer)
    {
        weechat_buffer_close (irc_raw_buffer);
        irc_raw_buffer = NULL;
    }
    if (irc_raw_filter)
    {
        free (irc_raw_filter);
        irc_raw_filter = NULL;
    }
    if (irc_raw_filter_hashtable_options)
    {
        weechat_hashtable_free (irc_raw_filter_hashtable_options);
        irc_raw_filter_hashtable_options = NULL;
    }
}
