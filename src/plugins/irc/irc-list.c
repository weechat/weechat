/*
 * irc-list.c - functions for IRC list buffer
 *
 * Copyright (C) 2023-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include "irc.h"
#include "irc-list.h"
#include "irc-buffer.h"
#include "irc-color.h"
#include "irc-config.h"
#include "irc-input.h"
#include "irc-message.h"
#include "irc-server.h"


struct t_hdata *irc_list_hdata_list_channel = NULL;
struct t_hashtable *irc_list_filter_hashtable_pointers = NULL;
struct t_hashtable *irc_list_filter_hashtable_extra_vars = NULL;
struct t_hashtable *irc_list_filter_hashtable_options = NULL;


/*
 * Compares two channels in list.
 */

int
irc_list_compare_cb (void *data, struct t_arraylist *arraylist,
                     void *pointer1, void *pointer2)
{
    struct t_irc_server *ptr_server;
    const char *ptr_field;
    int i, reverse, case_sensitive, rc;

    /* make C compiler happy */
    (void) arraylist;

    ptr_server = data;
    if (!ptr_server)
        return 1;

    for (i = 0; i < ptr_server->list->sort_fields_count; i++)
    {
        reverse = 1;
        case_sensitive = 1;
        ptr_field = ptr_server->list->sort_fields[i];
        while ((ptr_field[0] == '-') || (ptr_field[0] == '~'))
        {
            if (ptr_field[0] == '-')
                reverse *= -1;
            else if (ptr_field[0] == '~')
                case_sensitive ^= 1;
            ptr_field++;
        }
        rc = weechat_hdata_compare (irc_list_hdata_list_channel,
                                    pointer1, pointer2,
                                    ptr_field,
                                    case_sensitive);
        rc *= reverse;
        if (rc != 0)
            return rc;
    }

    return 1;
}

/*
 * Frees a channel in list.
 */

void
irc_list_free_cb (void *data, struct t_arraylist *arraylist, void *pointer)
{
    struct t_irc_list_channel *ptr_channel;

    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    ptr_channel = (struct t_irc_list_channel *)pointer;
    if (ptr_channel)
    {
        free (ptr_channel->name);
        free (ptr_channel->name2);
        free (ptr_channel->topic);
        free (ptr_channel);
    }
}

/*
 * Sets the local variable "filter" in the list buffer.
 */

void
irc_list_buffer_set_localvar_filter (struct t_gui_buffer *buffer,
                                     struct t_irc_server *server)
{
    if (!buffer || !server)
        return;

    weechat_buffer_set (buffer, "localvar_set_filter",
                        (server->list->filter) ? server->list->filter : "*");
}

/*
 * Sets filter for list of channels.
 */

void
irc_list_set_filter (struct t_irc_server *server, const char *filter)
{
    if (server->list->filter)
    {
        free (server->list->filter);
        server->list->filter = NULL;
    }

    server->list->filter = (filter && (strcmp (filter, "*") != 0)) ?
        strdup (filter) : NULL;

    irc_list_buffer_set_localvar_filter (server->list->buffer, server);
}

/*
 * Sets sort for list of channels.
 *
 */

void
irc_list_set_sort (struct t_irc_server *server, const char *sort)
{
    if (server->list->sort)
    {
        free (server->list->sort);
        server->list->sort = NULL;
    }
    if (server->list->sort_fields)
    {
        weechat_string_free_split (server->list->sort_fields);
        server->list->sort_fields = NULL;
    }
    server->list->sort_fields_count = 0;

    server->list->sort = strdup (
        (sort && sort[0]) ?
        sort : weechat_config_string (irc_config_look_list_buffer_sort));

    if (server->list->sort)
    {
        server->list->sort_fields = weechat_string_split (
            server->list->sort,
            ",",
            NULL,
            WEECHAT_STRING_SPLIT_STRIP_LEFT
            | WEECHAT_STRING_SPLIT_STRIP_RIGHT
            | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
            0,
            &server->list->sort_fields_count);
    }
}

/*
 * Adds the properties of an irc list channel in a hashtable
 * (keys and values must be strings).
 */

void
irc_list_add_channel_in_hashtable (struct t_hashtable *hashtable,
                                   struct t_irc_list_channel *channel)
{
    char str_number[32];

    weechat_hashtable_set (hashtable, "name", channel->name);
    weechat_hashtable_set (hashtable, "name2", channel->name2);
    snprintf (str_number, sizeof (str_number), "%d", channel->users);
    weechat_hashtable_set (hashtable, "users", str_number);
    weechat_hashtable_set (hashtable, "topic", channel->topic);
}

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
irc_list_string_match (const char *string, const char *mask)
{
    if (strchr (mask, '*'))
        return weechat_string_match (string, mask, 0);
    else
        return (weechat_strcasestr (string, mask)) ? 1 : 0;
}

/*
 * Checks if a channel matches filter.
 *
 * Return:
 *   1: channel matches filter
 *   0: channel does NOT match filter
 */

int
irc_list_channel_match_filter (struct t_irc_server *server,
                               struct t_irc_list_channel *channel)
{
    char *error, *result;
    long number;
    int match;

    /* no filter? then any channel is matching */
    if (!server->list->filter)
        return 1;

    if (strncmp (server->list->filter, "c:", 2) == 0)
    {
        /* filter by evaluated condition */
        weechat_hashtable_set (irc_list_filter_hashtable_pointers,
                               "irc_list_channel", channel);
        irc_list_add_channel_in_hashtable (irc_list_filter_hashtable_extra_vars,
                                           channel);
        result = weechat_string_eval_expression (
            server->list->filter + 2,
            irc_list_filter_hashtable_pointers,
            irc_list_filter_hashtable_extra_vars,
            irc_list_filter_hashtable_options);
        match = (result && (strcmp (result, "1") == 0)) ? 1 : 0;
        free (result);
        return match;
    }

    if (strncmp (server->list->filter, "n:", 2) == 0)
    {
        /* filter by channel name */
        if (channel->name
            && irc_list_string_match (channel->name, server->list->filter + 2))
        {
            return 1;
        }
    }
    else if (strncmp (server->list->filter, "t:", 2) == 0)
    {
        /* filter by topic */
        if (channel->topic
            && irc_list_string_match (channel->topic, server->list->filter + 2))
        {
            return 1;
        }
    }
    else if (strncmp (server->list->filter, "u:>", 3) == 0)
    {
        /* filter by users (> N)*/
        error = NULL;
        number = strtol (server->list->filter + 3, &error, 10);
        if (error && !error[0] && channel->users > (int)number)
            return 1;
    }
    else if (strncmp (server->list->filter, "u:<", 3) == 0)
    {
        /* filter by users (< N)*/
        error = NULL;
        number = strtol (server->list->filter + 3, &error, 10);
        if (error && !error[0] && channel->users < (int)number)
            return 1;
    }
    else if (strncmp (server->list->filter, "u:", 2) == 0)
    {
        /* filter by users */
        error = NULL;
        number = strtol (server->list->filter + 2, &error, 10);
        if (error && !error[0] && channel->users >= (int)number)
            return 1;
    }
    else
    {
        if (channel->name
            && irc_list_string_match (channel->name, server->list->filter))
        {
            return 1;
        }
        if (channel->topic
            && irc_list_string_match (channel->topic, server->list->filter))
        {
            return 1;
        }
    }

    return 0;
}

/*
 * Filters channels: apply filter and use sort to build the list
 * "filter_channels" that are pointers to t_irc_list_channel structs
 * stored in main list "channels".
 */

void
irc_list_filter_channels (struct t_irc_server *server)
{
    struct t_irc_list_channel *ptr_channel;
    int i, list_size;

    if (server->list->filter_channels)
    {
        weechat_arraylist_clear (server->list->filter_channels);
    }
    else
    {
        server->list->filter_channels = weechat_arraylist_new (
            16, 1, 0,
            &irc_list_compare_cb, server,
            NULL, NULL);
    }

    if (!server->list->sort)
    {
        irc_list_set_sort (
            server,
            weechat_config_string (irc_config_look_list_buffer_sort));
    }

    list_size = weechat_arraylist_size (server->list->channels);
    for (i = 0; i < list_size; i++)
    {
        ptr_channel = (struct t_irc_list_channel *)weechat_arraylist_get (
            server->list->channels, i);
        if (!ptr_channel)
            continue;
        if (irc_list_channel_match_filter (server, ptr_channel))
            weechat_arraylist_add (server->list->filter_channels, ptr_channel);
    }
}

/*
 * Parses output of redirected /list (string with raw IRC messages separated
 * by newlines) and returns an arraylist of parsed channels, or NULL if no
 * valid message (322) was found in output.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_list_parse_messages (struct t_irc_server *server, const char *output)
{
    struct t_irc_list_channel *channel;
    char **irc_msgs, *command, **params, *error;
    const char *ptr_name;
    int i, count_irc_msgs, num_params, length, keep_colors;
    long number;

    if (server->list->channels)
    {
        weechat_arraylist_free (server->list->channels);
        server->list->channels = NULL;
    }

    irc_msgs = weechat_string_split (output, "\n", NULL, 0, 0, &count_irc_msgs);
    if (!irc_msgs)
        return 0;

    server->list->channels = weechat_arraylist_new (
        16, 0, 1,
        NULL, NULL,
        &irc_list_free_cb, NULL);
    if (!server->list->channels)
    {
        weechat_string_free_split (irc_msgs);
        return 0;
    }

    server->list->name_max_length = 0;

    keep_colors = (weechat_config_boolean (
                       irc_config_look_list_buffer_topic_strip_colors)) ?
        0 : 1;

    for (i = 0; i < count_irc_msgs; i++)
    {
        irc_message_parse (server, irc_msgs[i],
                           NULL,  /* tags */
                           NULL,  /* message_without_tags */
                           NULL,  /* nick */
                           NULL,  /* user */
                           NULL,  /* host */
                           &command,
                           NULL,  /* channel */
                           NULL,  /* arguments */
                           NULL,  /* text */
                           &params,
                           &num_params,
                           NULL,  /* pos_command */
                           NULL,  /* pos_arguments */
                           NULL,  /* pos_channel */
                           NULL);  /* pos_text */
        if (command
            && (strcmp (command, "322") == 0)
            && params
            && (num_params >= 3))
        {
            channel = malloc (sizeof (*channel));
            if (channel)
            {
                channel->name = strdup (params[1]);
                ptr_name = params[1] + 1;
                while (ptr_name[0] && (ptr_name[0] == params[1][0]))
                {
                    ptr_name++;
                }
                channel->name2 = strdup (ptr_name);
                error = NULL;
                number = strtol (params[2], &error, 10);
                channel->users = (error && !error[0]) ? number : 0;
                channel->topic = (num_params > 3) ?
                    irc_color_decode (params[3], keep_colors) : NULL;
                length = weechat_utf8_strlen_screen (channel->name);
                if (length > server->list->name_max_length)
                    server->list->name_max_length = length;
                weechat_arraylist_add (server->list->channels, channel);
            }
        }
        free (command);
        weechat_string_free_split (params);
    }

    weechat_string_free_split (irc_msgs);

    irc_list_filter_channels (server);

    return 1;
}

/*
 * Sets title of list buffer.
 */

void
irc_list_buffer_set_title (struct t_irc_server *server)
{
    int num_channels, num_channels_total;
    char str_title[8192];

    if (!server || !server->list->buffer)
        return;

    num_channels = (server->list->filter_channels) ?
        weechat_arraylist_size (server->list->filter_channels) : 0;
    num_channels_total = (server->list->channels) ?
        weechat_arraylist_size (server->list->channels) : 0;

    snprintf (str_title, sizeof (str_title),
              _("%d channels (total: %d) | Filter: %s | Sort: %s | "
                "Key(input): "
                "ctrl+j=join channel, "
                "($)=refresh, "
                "(q)=close buffer"),
              num_channels,
              num_channels_total,
              (server->list->filter) ? server->list->filter : "*",
              (server->list->sort) ? server->list->sort : "");

    weechat_buffer_set (server->list->buffer, "title", str_title);
}

/*
 * Displays a line.
 */

void
irc_list_display_line (struct t_irc_server *server, int line)
{
    struct t_irc_list_channel *ptr_channel;
    const char *ptr_color;
    char str_spaces[1024], color[256];
    int num_spaces;

    ptr_channel = (struct t_irc_list_channel *)weechat_arraylist_get (
        server->list->filter_channels, line);

    /* line color */
    if (line == server->list->selected_line)
    {
        snprintf (color, sizeof (color),
                  "%s,%s",
                  weechat_config_string (irc_config_color_list_buffer_line_selected),
                  weechat_config_string (irc_config_color_list_buffer_line_selected_bg));
        ptr_color = weechat_color (color);
    }
    else
    {
        ptr_color = NULL;
    }

    /* channel name */
    str_spaces[0] = '\0';
    num_spaces = server->list->name_max_length
        - weechat_utf8_strlen_screen (ptr_channel->name);
    if (num_spaces > 0)
    {
        if (num_spaces >= (int)sizeof (str_spaces))
            num_spaces = sizeof (str_spaces) - 1;
        memset (str_spaces, ' ', num_spaces);
        str_spaces[num_spaces] = '\0';
    }

    /* display the line */
    weechat_printf_y (
        server->list->buffer,
        line,
        "%s%s%s %7d  %s",
        (ptr_color) ? ptr_color : "",
        ptr_channel->name,
        str_spaces,
        ptr_channel->users,
        ptr_channel->topic);
}

/*
 * Updates list of channels in list buffer.
 */

void
irc_list_buffer_refresh (struct t_irc_server *server, int clear)
{
    int num_channels, i;

    if (!server || !server->list->buffer)
        return;

    num_channels = weechat_arraylist_size (server->list->filter_channels);

    if (clear)
    {
        weechat_buffer_clear (server->list->buffer);
        server->list->selected_line = 0;
    }

    for (i = 0; i < num_channels; i++)
    {
        irc_list_display_line (server, i);
    }

    irc_list_buffer_set_title (server);
}

/*
 * Sets current selected line.
 */

void
irc_list_set_current_line (struct t_irc_server *server, int line)
{
    int old_line;

    if ((line >= 0) && (line < weechat_arraylist_size (server->list->filter_channels)))
    {
        old_line = server->list->selected_line;
        server->list->selected_line = line;

        if (old_line != server->list->selected_line)
            irc_list_display_line (server, old_line);
        irc_list_display_line (server, server->list->selected_line);

        irc_list_buffer_set_title (server);
    }
}

/*
 * Gets info about a window.
 */

void
irc_list_get_window_info (struct t_gui_window *window,
                          int *start_line_y, int *chat_height)
{
    struct t_hdata *hdata_window, *hdata_window_scroll, *hdata_line;
    struct t_hdata *hdata_line_data;
    void *window_scroll, *start_line, *line_data;

    hdata_window = weechat_hdata_get ("window");
    hdata_window_scroll = weechat_hdata_get ("window_scroll");
    hdata_line = weechat_hdata_get ("line");
    hdata_line_data = weechat_hdata_get ("line_data");
    *start_line_y = 0;
    window_scroll = weechat_hdata_pointer (hdata_window, window, "scroll");
    if (window_scroll)
    {
        start_line = weechat_hdata_pointer (hdata_window_scroll, window_scroll,
                                            "start_line");
        if (start_line)
        {
            line_data = weechat_hdata_pointer (hdata_line, start_line, "data");
            if (line_data)
            {
                *start_line_y = weechat_hdata_integer (hdata_line_data,
                                                       line_data, "y");
            }
        }
    }
    *chat_height = weechat_hdata_integer (hdata_window, window,
                                          "win_chat_height");
}

/*
 * Checks if current line is outside window and adjusts scroll if needed.
 */

void
irc_list_check_line_outside_window (struct t_irc_server *server)
{
    struct t_gui_window *window;
    int start_line_y, chat_height;
    int selected_y;
    char str_command[256];

    window = weechat_window_search_with_buffer (server->list->buffer);
    if (!window)
        return;

    irc_list_get_window_info (window, &start_line_y, &chat_height);

    selected_y = server->list->selected_line;

    if ((start_line_y > selected_y)
        || (start_line_y < selected_y - chat_height + 1))
    {
        snprintf (str_command, sizeof (str_command),
                  "/window scroll -window %d %s%d",
                  weechat_window_get_integer (window, "number"),
                  (start_line_y > selected_y) ? "-" : "+",
                  (start_line_y > selected_y) ?
                  start_line_y - selected_y :
                  selected_y - start_line_y - chat_height + 1);
        weechat_command (server->list->buffer, str_command);
    }
}

/*
 * Callback for signal "window_scrolled".
 */

int
irc_list_window_scrolled_cb (const void *pointer, void *data,
                             const char *signal, const char *type_data,
                             void *signal_data)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_irc_server *ptr_server;
    int start_line_y, chat_height, line, num_channels;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    /* search the /list buffer */
    ptr_buffer = weechat_window_get_pointer (signal_data, "buffer");
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->list->buffer == ptr_buffer)
            break;
    }
    if (!ptr_server)
        return WEECHAT_RC_OK;

    irc_list_get_window_info (signal_data, &start_line_y, &chat_height);

    line = ptr_server->list->selected_line;
    while (line < start_line_y)
    {
        line += chat_height;
    }
    while (line >= start_line_y + chat_height)
    {
        line -= chat_height;
    }
    if (line < start_line_y)
        line = start_line_y + 1;

    num_channels = weechat_arraylist_size (ptr_server->list->filter_channels);
    if ((num_channels > 0) && (line >= num_channels))
        line = num_channels - 1;

    irc_list_set_current_line (ptr_server, line);

    return WEECHAT_RC_OK;
}

/*
 * Moves N lines up/down in buffer
 * (negative lines = move up, positive lines = move down).
 */

void
irc_list_move_line_relative (struct t_irc_server *server, int num_lines)
{
    int num_channels, line;

    num_channels = weechat_arraylist_size (server->list->filter_channels);
    if (num_channels == 0)
        return;

    line = server->list->selected_line + num_lines;
    if (line < 0)
        line = 0;
    if ((num_channels > 0) && (line >= num_channels))
        line = num_channels - 1;
    if (line != server->list->selected_line)
    {
        irc_list_set_current_line (server, line);
        irc_list_check_line_outside_window (server);
    }
}

/*
 * Moves to line N (0 = first line, -1 = last line).
 */

void
irc_list_move_line_absolute (struct t_irc_server *server, int line_number)
{
    int num_channels, line;

    num_channels = weechat_arraylist_size (server->list->filter_channels);
    if (num_channels == 0)
        return;

    line = line_number;
    if (line < 0)
        line = (num_channels > 0) ? num_channels - 1 : 0;
    if ((num_channels > 0) && (line >= num_channels))
        line = num_channels - 1;
    if (line != server->list->selected_line)
    {
        irc_list_set_current_line (server, line);
        irc_list_check_line_outside_window (server);
    }
}

/*
 * Scrolls horizontally with percent
 * (negative: scroll to the left, positive: scroll to the right).
 */

void
irc_list_scroll_horizontal (struct t_irc_server *server, int percent)
{
    struct t_gui_window *ptr_window;
    char str_command[512];

    if (percent < -100)
        percent = -100;
    else if (percent > 100)
        percent = 100;

    ptr_window = weechat_window_search_with_buffer (server->list->buffer);
    if (!ptr_window)
        return;

    snprintf (str_command, sizeof (str_command),
              "/window scroll_horiz -window %d %d%%",
              weechat_window_get_integer (ptr_window, "number"),
              percent);
    weechat_command (server->list->buffer, str_command);
}

/*
 * Joins channel on current selected line.
 */

void
irc_list_join_channel (struct t_irc_server *server)
{
    struct t_irc_list_channel *ptr_channel;
    int num_channels;
    char str_command[1024];

    num_channels = weechat_arraylist_size (server->list->filter_channels);

    if ((num_channels == 0) || (server->list->selected_line >= num_channels))
        return;

    ptr_channel = (struct t_irc_list_channel *)weechat_arraylist_get (
        server->list->filter_channels,
        server->list->selected_line);
    if (!ptr_channel)
        return;

    snprintf (str_command, sizeof (str_command),
              "/join %s", ptr_channel->name);

    weechat_command (server->list->buffer, str_command);
}

/*
 * Callback for input data in list buffer.
 */

int
irc_list_buffer_input_data (struct t_gui_buffer *buffer, const char *input_data)
{
    struct t_irc_server *ptr_server;
    const char *ptr_server_name, *ptr_input;
    int i;
    char *actions[][2] = {
        { "<<", "/list -go 0"   },
        { ">>", "/list -go end" },
        { "<",  "/list -left"   },
        { ">",  "/list -right"  },
        { NULL, NULL            },
    };

    /* close buffer */
    if (strcmp (input_data, "q") == 0)
    {
        weechat_buffer_close (buffer);
        return WEECHAT_RC_OK;
    }

    ptr_server_name = weechat_buffer_get_string (buffer, "localvar_server");
    if (!ptr_server_name)
        return WEECHAT_RC_OK;

    ptr_server = irc_server_search (ptr_server_name);
    if (!ptr_server)
        return WEECHAT_RC_OK;

    /* refresh buffer */
    if (strcmp (input_data, "$") == 0)
    {
        weechat_command (ptr_server->list->buffer, "/list");
        return WEECHAT_RC_OK;
    }

    /* join channel */
    if (strcmp (input_data, "j") == 0)
    {
        irc_list_join_channel (ptr_server);
        return WEECHAT_RC_OK;
    }

    /* change sort of channels */
    if (strncmp (input_data, "s:", 2) == 0)
    {
        irc_list_set_sort (ptr_server, input_data + 2);
        irc_list_filter_channels (ptr_server);
        irc_list_buffer_refresh (ptr_server, 1);
        weechat_buffer_set (buffer, "display", "1");
        return WEECHAT_RC_OK;
    }

    /* execute action */
    for (i = 0; actions[i][0]; i++)
    {
        if (strcmp (input_data, actions[i][0]) == 0)
        {
            weechat_command (buffer, actions[i][1]);
            return WEECHAT_RC_OK;
        }
    }

    /* filter channels with given text */
    ptr_input = input_data;
    while (ptr_input[0] == ' ')
    {
        ptr_input++;
    }
    if (ptr_input[0])
    {
        irc_list_set_filter (ptr_server, ptr_input);
        irc_list_filter_channels (ptr_server);
        irc_list_buffer_refresh (ptr_server, 1);
        weechat_buffer_set (buffer, "display", "1");
    }

    return WEECHAT_RC_OK;
}

/*
 * Creates buffer with list of channels for a server.
 *
 * Returns pointer to newly created buffer, NULL if error.
 */

struct t_gui_buffer *
irc_list_create_buffer (struct t_irc_server *server)
{
    struct t_hashtable *buffer_props;
    struct t_gui_buffer *buffer;
    char buffer_name[1024], str_number[32];
    int buffer_position, current_buffer_number;

    buffer_props = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (buffer_props)
    {
        weechat_hashtable_set (buffer_props, "type", "free");
        weechat_hashtable_set (buffer_props, "localvar_set_type", "list");
        weechat_hashtable_set (buffer_props, "localvar_set_server", server->name);
        weechat_hashtable_set (buffer_props, "localvar_set_channel", server->name);
        weechat_hashtable_set (buffer_props, "localvar_set_no_log", "1");
        /* disable all highlights on this buffer */
        weechat_hashtable_set (buffer_props, "highlight_words", "-");
        /* set keys on buffer */
        weechat_hashtable_set (buffer_props, "key_bind_up", "/list -up");
        weechat_hashtable_set (buffer_props, "key_bind_down", "/list -down");
        weechat_hashtable_set (buffer_props, "key_bind_meta-home", "/list -go 0");
        weechat_hashtable_set (buffer_props, "key_bind_meta-end", "/list -go end");
        weechat_hashtable_set (buffer_props, "key_bind_f11", "/list -left");
        weechat_hashtable_set (buffer_props, "key_bind_f12", "/list -right");
        weechat_hashtable_set (buffer_props, "key_bind_ctrl-j", "/list -join");
    }

    current_buffer_number = weechat_buffer_get_integer (
        weechat_current_buffer (), "number");

    snprintf (buffer_name, sizeof (buffer_name), "list_%s", server->name);

    buffer = weechat_buffer_new_props (
        buffer_name,
        buffer_props,
        &irc_input_data_cb, NULL, NULL,
        &irc_buffer_close_cb, NULL, NULL);

    weechat_hashtable_free (buffer_props);

    irc_list_buffer_set_localvar_filter (buffer, server);

    if (weechat_buffer_get_integer (buffer, "layout_number") < 1)
    {
        buffer_position = weechat_config_enum (irc_config_look_new_list_position);
        switch (buffer_position)
        {
            case IRC_CONFIG_LOOK_BUFFER_POSITION_NONE:
                /* do nothing */
                break;
            case IRC_CONFIG_LOOK_BUFFER_POSITION_NEXT:
                /* move buffer to current number + 1 */
                snprintf (str_number, sizeof (str_number),
                          "%d", current_buffer_number + 1);
                weechat_buffer_set (buffer, "number", str_number);
                break;
            case IRC_CONFIG_LOOK_BUFFER_POSITION_NEAR_SERVER:
                /* move buffer after last channel/pv of server */
                irc_buffer_move_near_server (
                    server,
                    1,  /* list_buffer */
                    -1,  /* channel_type */
                    buffer);
                break;
        }
    }

    return buffer;
}

/*
 * Callback for redirected /list command.
 */

int
irc_list_hsignal_redirect_list_cb (const void *pointer,
                                   void *data,
                                   const char *signal,
                                   struct t_hashtable *hashtable)
{
    struct t_irc_server *ptr_server;
    const char *ptr_error, *ptr_server_name, *ptr_output;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;

    ptr_error = weechat_hashtable_get (hashtable, "error");
    if (ptr_error && ptr_error[0])
    {
        weechat_printf (
            NULL,
            _("%s%s: error in redirection of /list: %s"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, ptr_error);
        return WEECHAT_RC_OK;
    }

    ptr_server_name = weechat_hashtable_get (hashtable, "server");
    if (!ptr_server_name)
        return WEECHAT_RC_OK;

    ptr_server = irc_server_search (ptr_server_name);
    if (!ptr_server || !ptr_server->buffer)
        return WEECHAT_RC_OK;

    ptr_output = weechat_hashtable_get (hashtable, "output");
    if (!ptr_output)
        return WEECHAT_RC_OK;

    if (!irc_list_hdata_list_channel)
    {
        irc_list_hdata_list_channel = weechat_hdata_get ("irc_list_channel");
        if (!irc_list_hdata_list_channel)
            return WEECHAT_RC_OK;
    }

    irc_list_parse_messages (ptr_server, ptr_output);
    if (!ptr_server->list->channels)
        return WEECHAT_RC_OK;

    irc_list_buffer_refresh (ptr_server, 1);

    return WEECHAT_RC_OK;
}

/*
 * Resets lists used by list buffer.
 */

void
irc_list_reset (struct t_irc_server *server)
{
    if (!server)
        return;

    if (server->list->channels)
        weechat_arraylist_clear (server->list->channels);
    if (server->list->filter_channels)
        weechat_arraylist_clear (server->list->filter_channels);
    server->list->name_max_length = 0;
    if (!server->list->sort)
    {
        irc_list_set_sort (
            server,
            weechat_config_string (irc_config_look_list_buffer_sort));
    }
    server->list->selected_line = 0;
}

/*
 * Frees a list structure in a server.
 */

struct t_irc_list *
irc_list_alloc ()
{
    struct t_irc_list *list;

    list = malloc (sizeof (*list));
    if (!list)
        return NULL;

    list->buffer = NULL;
    list->channels = NULL;
    list->filter_channels = NULL;
    list->name_max_length = 0;
    list->filter = NULL;
    list->sort = NULL;
    list->sort_fields = NULL;
    list->sort_fields_count = 0;
    list->selected_line = 0;

    return list;
}

/*
 * Frees data in a list structure.
 */

void
irc_list_free_data (struct t_irc_server *server)
{
    if (!server || !server->list)
        return;

    if (server->list->channels)
    {
        weechat_arraylist_free (server->list->channels);
        server->list->channels = NULL;
    }
    if (server->list->filter_channels)
    {
        weechat_arraylist_free (server->list->filter_channels);
        server->list->filter_channels = NULL;
    }
    server->list->name_max_length = 0;
    if (server->list->filter)
    {
        free (server->list->filter);
        server->list->filter = NULL;
    }
    if (server->list->sort)
    {
        free (server->list->sort);
        server->list->sort = NULL;
    }
    if (server->list->sort_fields)
    {
        weechat_string_free_split (server->list->sort_fields);
        server->list->sort_fields = NULL;
    }
    server->list->sort_fields_count = 0;
    server->list->selected_line = 0;
}

/*
 * Frees a list structure in a server.
 */

void
irc_list_free (struct t_irc_server *server)
{
    if (!server || !server->list)
        return;

    if (server->list->buffer)
        weechat_buffer_close (server->list->buffer);

    irc_list_free_data (server);

    free (server->list);
    server->list = NULL;
}

/*
 * Returns hdata for irc_list_channel.
 */

struct t_hdata *
irc_list_hdata_list_channel_cb (const void *pointer, void *data,
                                const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = weechat_hdata_new (hdata_name, NULL, NULL, 0, 0, NULL, NULL);
    if (hdata)
    {
        WEECHAT_HDATA_VAR(struct t_irc_list_channel, name, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_list_channel, name2, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_list_channel, users, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_list_channel, topic, STRING, 0, NULL, NULL);
    }
    return hdata;
}

/*
 * Returns hdata for irc_list.
 */

struct t_hdata *
irc_list_hdata_list_cb (const void *pointer, void *data, const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = weechat_hdata_new (hdata_name, NULL, NULL, 0, 0, NULL, NULL);
    if (hdata)
    {
        WEECHAT_HDATA_VAR(struct t_irc_list, buffer, POINTER, 0, NULL, "buffer");
        WEECHAT_HDATA_VAR(struct t_irc_list, channels, POINTER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_list, filter_channels, POINTER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_list, name_max_length, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_list, filter, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_list, sort, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_list, sort_fields, POINTER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_list, sort_fields_count, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_list, selected_line, INTEGER, 0, NULL, NULL);
    }
    return hdata;
}

/*
 * Callback called when a mouse action occurs in irc list buffer.
 */

int
irc_list_mouse_hsignal_cb (const void *pointer, void *data, const char *signal,
                           struct t_hashtable *hashtable)
{
    const char *ptr_key, *ptr_chat_line_y, *ptr_buffer_pointer;
    struct t_gui_buffer *ptr_buffer;
    unsigned long value;
    char str_command[1024];
    int rc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;

    ptr_key = weechat_hashtable_get (hashtable, "_key");
    ptr_buffer_pointer = weechat_hashtable_get (hashtable, "_buffer");
    ptr_chat_line_y = weechat_hashtable_get (hashtable, "_chat_line_y");

    if (!ptr_key || !ptr_buffer_pointer || !ptr_chat_line_y)
        return WEECHAT_RC_OK;

    rc = sscanf (ptr_buffer_pointer, "%lx", &value);
    if ((rc == EOF) || (rc == 0))
        return WEECHAT_RC_OK;
    ptr_buffer = (struct t_gui_buffer *)value;
    if (!ptr_buffer)
        return WEECHAT_RC_OK;

    snprintf (str_command, sizeof (str_command),
              "/list -go %s",
              ptr_chat_line_y);
    weechat_command (ptr_buffer, str_command);

    if (weechat_string_match (ptr_key, "button2*", 1))
        weechat_command (ptr_buffer, "/list -join");

    return WEECHAT_RC_OK;
}

/*
 * Initializes irc list.
 */

void
irc_list_init ()
{
    struct t_hashtable *keys;

    irc_list_filter_hashtable_pointers = weechat_hashtable_new (
        8,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_POINTER,
        NULL, NULL);
    irc_list_filter_hashtable_extra_vars = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    irc_list_filter_hashtable_options = weechat_hashtable_new (
        8,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (irc_list_filter_hashtable_options)
    {
        weechat_hashtable_set (irc_list_filter_hashtable_options,
                               "type", "condition");
    }

    weechat_hook_hsignal (IRC_LIST_MOUSE_HSIGNAL,
                          &irc_list_mouse_hsignal_cb, NULL, NULL);

    keys = weechat_hashtable_new (32,
                                  WEECHAT_HASHTABLE_STRING,
                                  WEECHAT_HASHTABLE_STRING,
                                  NULL, NULL);
    if (keys)
    {
        weechat_hashtable_set (
            keys,
            "@chat(" IRC_PLUGIN_NAME ".list_*):button1",
            "/window ${_window_number};/list -go ${_chat_line_y}");
        weechat_hashtable_set (
            keys,
            "@chat(" IRC_PLUGIN_NAME ".list_*):button2*",
            "hsignal:" IRC_LIST_MOUSE_HSIGNAL);
        weechat_hashtable_set (
            keys,
            "@chat(" IRC_PLUGIN_NAME ".list_*):wheelup",
            "/list -up 5");
        weechat_hashtable_set (
            keys,
            "@chat(" IRC_PLUGIN_NAME ".list_*):wheeldown",
            "/list -down 5");
        weechat_hashtable_set (keys, "__quiet", "1");
        weechat_key_bind ("mouse", keys);
        weechat_hashtable_free (keys);
    }
}

/*
 * Ends irc list.
 */

void
irc_list_end ()
{
    if (irc_list_filter_hashtable_pointers)
    {
        weechat_hashtable_free (irc_list_filter_hashtable_pointers);
        irc_list_filter_hashtable_pointers = NULL;
    }
    if (irc_list_filter_hashtable_extra_vars)
    {
        weechat_hashtable_free (irc_list_filter_hashtable_extra_vars);
        irc_list_filter_hashtable_extra_vars = NULL;
    }
    if (irc_list_filter_hashtable_options)
    {
        weechat_hashtable_free (irc_list_filter_hashtable_options);
        irc_list_filter_hashtable_options = NULL;
    }
    irc_list_hdata_list_channel = NULL;
}
