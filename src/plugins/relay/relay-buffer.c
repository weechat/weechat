/*
 * relay-buffer.c - display clients list on relay buffer
 *
 * Copyright (C) 2003-2021 Sébastien Helleu <flashcode@flashtux.org>
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

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-buffer.h"
#include "relay-client.h"
#include "relay-config.h"
#include "relay-raw.h"


struct t_gui_buffer *relay_buffer = NULL;
int relay_buffer_selected_line = 0;


/*
 * Updates a client in buffer and updates hotlist for relay buffer.
 */

void
relay_buffer_refresh (const char *hotlist)
{
    struct t_relay_client *ptr_client, *client_selected;
    char str_color[256], str_status[64], str_date_start[128], str_date_end[128];
    char *str_recv, *str_sent;
    int i, length, line;
    struct tm *date_tmp;

    if (relay_buffer)
    {
        weechat_buffer_clear (relay_buffer);
        line = 0;
        client_selected = relay_client_search_by_number (relay_buffer_selected_line);
        weechat_printf_y (relay_buffer, 0,
                          "%s%s%s%s%s%s%s",
                          weechat_color ("green"),
                          _("Actions (letter+enter):"),
                          weechat_color ("lightgreen"),
                          /* disconnect */
                          (client_selected
                           && !RELAY_CLIENT_HAS_ENDED(client_selected)) ?
                          _("  [D] Disconnect") : "",
                          /* remove */
                          (client_selected
                           && RELAY_CLIENT_HAS_ENDED(client_selected)) ?
                          _("  [R] Remove") : "",
                          /* purge old */
                          _("  [P] Purge finished"),
                          /* quit */
                          _("  [Q] Close this buffer"));
        for (ptr_client = relay_clients; ptr_client;
             ptr_client = ptr_client->next_client)
        {
            snprintf (str_color, sizeof (str_color),
                      "%s,%s",
                      (line == relay_buffer_selected_line) ?
                      weechat_config_string (relay_config_color_text_selected) :
                      weechat_config_string (relay_config_color_text),
                      weechat_config_string (relay_config_color_text_bg));

            snprintf (str_status, sizeof (str_status),
                      "%s", _(relay_client_status_string[ptr_client->status]));
            length = weechat_utf8_strlen_screen (str_status);
            if (length < 20)
            {
                for (i = 0; i < 20 - length; i++)
                {
                    strcat (str_status, " ");
                }
            }

            str_date_start[0] = '\0';
            date_tmp = localtime (&(ptr_client->start_time));
            if (date_tmp)
            {
                if (strftime (str_date_start, sizeof (str_date_start),
                              "%a, %d %b %Y %H:%M:%S", date_tmp) == 0)
                    str_date_start[0] = '\0';
            }
            str_date_end[0] = '-';
            str_date_end[1] = '\0';
            if (ptr_client->end_time > 0)
            {
                date_tmp = localtime (&(ptr_client->end_time));
                if (date_tmp)
                {
                    if (strftime (str_date_end, sizeof (str_date_end),
                                  "%a, %d %b %Y %H:%M:%S", date_tmp) == 0)
                        str_date_end[0] = '\0';
                }
            }

            str_recv = weechat_string_format_size (ptr_client->bytes_recv);
            str_sent = weechat_string_format_size (ptr_client->bytes_sent);

            /* first line with status, description and bytes recv/sent */
            weechat_printf_y (relay_buffer, (line * 2) + 2,
                              _("%s%s[%s%s%s%s] %s, received: %s, sent: %s"),
                              weechat_color (str_color),
                              (line == relay_buffer_selected_line) ? "*** " : "    ",
                              weechat_color (weechat_config_string (relay_config_color_status[ptr_client->status])),
                              str_status,
                              weechat_color ("reset"),
                              weechat_color (str_color),
                              ptr_client->desc,
                              (str_recv) ? str_recv : "?",
                              (str_sent) ? str_sent : "?");

            /* second line with start/end time */
            weechat_printf_y (relay_buffer, (line * 2) + 3,
                              _("%s%-26s started on: %s, ended on: %s"),
                              weechat_color (str_color),
                              " ",
                              str_date_start,
                              str_date_end);

            if (str_recv)
                free (str_recv);
            if (str_sent)
                free (str_sent);

            line++;
        }
        if (hotlist)
            weechat_buffer_set (relay_buffer, "hotlist", hotlist);
    }
}

/*
 * Callback for input data in relay buffer.
 */

int
relay_buffer_input_cb (const void *pointer, void *data,
                       struct t_gui_buffer *buffer,
                       const char *input_data)
{
    struct t_relay_client *client, *ptr_client, *next_client;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (buffer == relay_raw_buffer)
    {
        if (weechat_strcasecmp (input_data, "q") == 0)
            weechat_buffer_close (buffer);
    }
    else if (buffer == relay_buffer)
    {
        client = relay_client_search_by_number (relay_buffer_selected_line);

        /* disconnect client */
        if (weechat_strcasecmp (input_data, "d") == 0)
        {
            if (client && !RELAY_CLIENT_HAS_ENDED(client))
            {
                relay_client_disconnect (client);
                relay_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
            }
        }
        /* purge old clients */
        else if (weechat_strcasecmp (input_data, "p") == 0)
        {
            ptr_client = relay_clients;
            while (ptr_client)
            {
                next_client = ptr_client->next_client;
                if (RELAY_CLIENT_HAS_ENDED(ptr_client))
                    relay_client_free (ptr_client);
                ptr_client = next_client;
            }
            relay_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
        }
        /* quit relay buffer (close it) */
        else if (weechat_strcasecmp (input_data, "q") == 0)
        {
            weechat_buffer_close (buffer);
        }
        /* remove client */
        else if (weechat_strcasecmp (input_data, "r") == 0)
        {
            if (client && RELAY_CLIENT_HAS_ENDED(client))
            {
                relay_client_free (client);
                relay_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback called when relay buffer is closed.
 */

int
relay_buffer_close_cb (const void *pointer, void *data,
                       struct t_gui_buffer *buffer)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (buffer == relay_raw_buffer)
    {
        relay_raw_buffer = NULL;
    }
    else if (buffer == relay_buffer)
    {
        relay_buffer = NULL;
    }

    return WEECHAT_RC_OK;
}

/*
 * Opens relay buffer.
 */

void
relay_buffer_open ()
{
    if (!relay_buffer)
    {
        relay_buffer = weechat_buffer_new (RELAY_BUFFER_NAME,
                                           &relay_buffer_input_cb, NULL, NULL,
                                           &relay_buffer_close_cb, NULL, NULL);

        /* failed to create buffer ? then exit */
        if (!relay_buffer)
            return;

        weechat_buffer_set (relay_buffer, "type", "free");
        weechat_buffer_set (relay_buffer, "title", _("List of clients for relay"));
        weechat_buffer_set (relay_buffer, "key_bind_meta2-A", "/relay up");
        weechat_buffer_set (relay_buffer, "key_bind_meta2-B", "/relay down");
        weechat_buffer_set (relay_buffer, "localvar_set_type", "relay");
    }
}
