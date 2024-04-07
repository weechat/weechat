/*
 * relay-api-msg.c - build JSON messages for "api" protocol
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
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <cjson/cJSON.h>

#include "../../weechat-plugin.h"
#include "../relay.h"
#include "../relay-client.h"
#include "../relay-http.h"
#include "../relay-websocket.h"
#include "relay-api.h"
#include "relay-api-msg.h"
#include "relay-api-protocol.h"

#define MSG_ADD_STR_BUF(__json_name, __string)                          \
    cJSON_AddItemToObject(                                              \
        json, __json_name,                                              \
        cJSON_CreateString (__string));

#define MSG_ADD_STR_PTR(__json_name, __string)                          \
    cJSON_AddItemToObject(                                              \
        json, __json_name,                                              \
        cJSON_CreateString ((__string) ? __string : ""));

#define MSG_ADD_HDATA_VAR(__json_type, __json_name,                     \
                          __var_type, __var_name)                       \
    cJSON_AddItemToObject(                                              \
        json, __json_name,                                              \
        cJSON_Create##__json_type (                                     \
            weechat_hdata_##__var_type (hdata, pointer, __var_name)));

#define MSG_ADD_HDATA_TIME(__json_name, __var_name)                     \
    date = weechat_hdata_time (hdata, pointer, __var_name);             \
    strftime (str_time, sizeof (str_time), "%FT%TZ", gmtime (&date));   \
    MSG_ADD_STR_BUF(__json_name, str_time);

#define MSG_ADD_HDATA_TIME_USEC(__json_name,                            \
                                __var_name, __var_name_usec)            \
    time_value = weechat_hdata_time (hdata, pointer, __var_name);       \
    gmtime_r (&time_value, &gm_time);                                   \
    tv.tv_sec = mktime (&gm_time);                                      \
    tv.tv_usec = weechat_hdata_integer (hdata, pointer,                 \
                                        __var_name_usec);               \
    weechat_util_strftimeval (str_time, sizeof (str_time),              \
                              "%FT%T.%fZ", &tv);                        \
    MSG_ADD_STR_BUF(__json_name, str_time);

#define MSG_ADD_HDATA_STR(__json_name, __var_name)                      \
    ptr_string = weechat_hdata_string (hdata, pointer, __var_name);     \
    MSG_ADD_STR_PTR(__json_name, ptr_string);

#define MSG_ADD_HDATA_STR_COLORS(__json_name, __var_name)               \
    ptr_string = weechat_hdata_string (hdata, pointer, __var_name);     \
    switch (colors)                                                     \
    {                                                                   \
        case RELAY_API_COLORS_ANSI:                                     \
            string = weechat_hook_modifier_exec (                       \
                "color_encode_ansi", NULL,                              \
                (ptr_string) ? ptr_string : "");                        \
            if (string)                                                 \
            {                                                           \
                MSG_ADD_STR_PTR(__json_name, string);                   \
                free (string);                                          \
            }                                                           \
            break;                                                      \
        case RELAY_API_COLORS_WEECHAT:                                  \
            MSG_ADD_STR_PTR(__json_name, ptr_string);                   \
            break;                                                      \
        case RELAY_API_COLORS_STRIP:                                    \
            string = weechat_string_remove_color (                      \
                (ptr_string) ? ptr_string : "", NULL);                  \
            if (string)                                                 \
            {                                                           \
                MSG_ADD_STR_PTR(__json_name, string);                   \
                free (string);                                          \
            }                                                           \
        case RELAY_API_NUM_COLORS:                                      \
            break;                                                      \
    }


/*
 * Sends JSON response to client (internal use).
 *
 * Returns number of bytes sent to client, -1 if error.
 */

int
relay_api_msg_send_json_internal (struct t_relay_client *client,
                                  int return_code,
                                  const char *message,
                                  const char *event_name,
                                  struct t_gui_buffer *event_buffer,
                                  const char *headers,
                                  const char *body_type,
                                  cJSON *json_body)
{
    cJSON *json, *json_event;
    int num_bytes, length;
    const char *ptr_id;
    char *string, *error, *request;
    long long id;

    if (!client || !message)
        return -1;

    num_bytes = -1;

    if (client->websocket == RELAY_CLIENT_WEBSOCKET_READY)
    {
        /*
         * with established websocket, we return JSON string instead of
         * an HTTP response
         */
        json = cJSON_CreateObject ();
        if (json)
        {
            cJSON_AddItemToObject (json, "code", cJSON_CreateNumber (return_code));
            cJSON_AddItemToObject (json, "message", cJSON_CreateString (message));
            if (event_name)
            {
                json_event = cJSON_CreateObject ();
                if (json_event)
                {
                    cJSON_AddItemToObject (
                        json_event, "name",
                        cJSON_CreateString ((event_name) ? event_name : ""));
                    id = -1;
                    if (event_buffer)
                    {
                        ptr_id = weechat_buffer_get_string (event_buffer, "id");
                        error = NULL;
                        id = strtoll (ptr_id, &error, 10);
                        if (!error || error[0])
                            id = -1;
                    }
                    cJSON_AddItemToObject (
                        json_event, "buffer_id",
                        cJSON_CreateNumber (id));
                    cJSON_AddItemToObject (json, "event", json_event);
                }
            }
            else
            {
                length = ((client->http_req->method) ? strlen (client->http_req->method) : 0)
                    + 1
                    + ((client->http_req->path) ? strlen (client->http_req->path) : 0)
                    + 1;
                request = malloc (length);
                if (request)
                {
                    snprintf (
                        request, length,
                        "%s%s%s",
                        (client->http_req->method) ? client->http_req->method : "",
                        (client->http_req->method) ? " " : "",
                        (client->http_req->path) ? client->http_req->path : "");
                    cJSON_AddItemToObject (json, "request",
                                           cJSON_CreateString (request));
                    cJSON_AddItemToObject (
                        json, "request_body",
                        (client->http_req->body) ?
                        cJSON_Parse (client->http_req->body) :cJSON_CreateNull ());
                    free (request);
                }
            }
            if (json_body)
            {
                cJSON_AddItemToObject (json, "body_type",
                                       cJSON_CreateString ((body_type) ? body_type : ""));
                cJSON_AddItemToObject (json, "body", json_body);
            }
            string = cJSON_PrintUnformatted (json);
            num_bytes = relay_client_send (
                client,
                RELAY_MSG_STANDARD,
                string,
                (string) ? strlen (string) : 0,
                NULL);  /* raw_message */
            if (string)
                free (string);
            cJSON_DetachItemFromObject (json, "body");
            cJSON_Delete (json);
        }
    }
    else
    {
        string = (json_body) ? cJSON_PrintUnformatted (json_body) : NULL;
        num_bytes = relay_http_send_json (client, return_code, message, headers,
                                          string);
        if (string)
            free (string);
    }

    return num_bytes;
}

/*
 * Sends JSON response to client (internal use).
 *
 * Returns number of bytes sent to client, -1 if error.
 */

int
relay_api_msg_send_json (struct t_relay_client *client,
                         int return_code,
                         const char *message,
                         const char *body_type,
                         cJSON *json_body)
{
    return relay_api_msg_send_json_internal (client,
                                             return_code,
                                             message,
                                             NULL,  /* event_name */
                                             NULL,  /* event_buffer */
                                             NULL,  /* headers */
                                             body_type,
                                             json_body);
}

/*
 * Sends JSON error to client.
 *
 * Returns number of bytes sent to client, -1 if error.
 */

int
relay_api_msg_send_error_json (struct t_relay_client *client,
                               int return_code,
                               const char *message,
                               const char *headers,
                               const char *format, ...)
{
    cJSON *json;
    int num_bytes, length;
    char *error_msg, *str_json;

    if (!client || !message || !format)
        return -1;

    weechat_va_format (format);
    if (!vbuffer)
        return -1;

    num_bytes = -1;

    if (client->websocket == RELAY_CLIENT_WEBSOCKET_READY)
    {
        /*
         * with established websocket, we return JSON string instead of
         * an HTTP response
         */
        json = cJSON_CreateObject ();
        if (json)
        {
            cJSON_AddItemToObject (json, "error", cJSON_CreateString (vbuffer));
            num_bytes = relay_api_msg_send_json_internal (
                client,
                return_code,
                message,
                NULL,  /* event_name */
                NULL,  /* event_buffer */
                headers,
                NULL,  /* body_type */
                json);
            cJSON_Delete (json);
        }
    }
    else
    {
        error_msg = weechat_string_replace (vbuffer, "\"", "\\\"");
        if (error_msg)
        {
            length = strlen (error_msg) + 64;
            str_json = malloc (length);
            if (str_json)
            {
                snprintf (str_json, length, "{\"error\": \"%s\"}", error_msg);
                num_bytes = relay_http_send_json (client, return_code, message,
                                                  headers, str_json);
                free (str_json);
            }
            free (error_msg);
        }
    }

    free (vbuffer);
    return num_bytes;
}

/*
 * Sends event to the client.
 *
 * Returns number of bytes sent to client, -1 if error.
 */

int
relay_api_msg_send_event (struct t_relay_client *client,
                          const char *name,
                          struct t_gui_buffer *buffer,
                          const char *body_type,
                          cJSON *json_body)
{
    return relay_api_msg_send_json_internal (client,
                                             RELAY_API_HTTP_0_EVENT,
                                             name,
                                             buffer,
                                             NULL,  /* headers */
                                             body_type,
                                             json_body);
}

/*
 * Adds a buffer local variable into a JSON object.
 */

void
relay_api_msg_buffer_add_local_vars_cb (void *data,
                                        struct t_hashtable *hashtable,
                                        const void *key,
                                        const void *value)
{
    cJSON *json;

    /* make C compiler happy */
    (void) hashtable;

    json = (cJSON *)data;

    cJSON_AddItemToObject (
        json,
        (const char *)key,
        cJSON_CreateString ((value) ? (const char *)value : ""));
}

/*
 * Creates a JSON object with a buffer.
 */

cJSON *
relay_api_msg_buffer_to_json (struct t_gui_buffer *buffer,
                              long lines,
                              int nicks,
                              enum t_relay_api_colors colors)
{
    struct t_hdata *hdata;
    struct t_gui_buffer *pointer;
    cJSON *json, *json_local_vars, *json_lines, *json_nicks;
    const char *ptr_string;
    char *string;

    hdata = relay_hdata_buffer;
    pointer = buffer;

    json = cJSON_CreateObject ();
    if (!json)
        return NULL;

    if (!buffer)
        return json;

    MSG_ADD_HDATA_VAR(Number, "id", longlong, "id");
    MSG_ADD_HDATA_STR("name", "full_name");
    MSG_ADD_HDATA_STR("short_name", "short_name");
    MSG_ADD_HDATA_VAR(Number, "number", integer, "number");
    ptr_string = weechat_buffer_get_string (buffer, "type");
    MSG_ADD_STR_PTR("type", ptr_string);
    MSG_ADD_HDATA_STR_COLORS("title", "title");

    /* local_variables */
    json_local_vars = cJSON_CreateObject ();
    if (json_local_vars)
    {
        weechat_hashtable_map (
            weechat_hdata_pointer (hdata, buffer, "local_variables"),
            &relay_api_msg_buffer_add_local_vars_cb,
            json_local_vars);
        cJSON_AddItemToObject (json, "local_variables", json_local_vars);
    }

    /* lines */
    if (lines != 0)
    {
        json_lines = relay_api_msg_lines_to_json (buffer, lines, colors);
        if (json_lines)
            cJSON_AddItemToObject (json, "lines", json_lines);
    }

    /* nicks */
    if (nicks)
    {
        json_nicks = relay_api_msg_nick_group_to_json (
            weechat_hdata_pointer (hdata, buffer, "nicklist_root"));
        if (json_nicks)
            cJSON_AddItemToObject (json, "nicks", json_nicks);
    }

    return json;
}

/*
 * Creates a JSON object with a buffer line data.
 */

cJSON *
relay_api_msg_line_data_to_json (struct t_gui_line_data *line_data,
                                 enum t_relay_api_colors colors)
{
    struct t_hdata *hdata;
    struct t_gui_line_data *pointer;
    cJSON *json, *json_tags;
    const char *ptr_string;
    char *string, str_time[256], str_var[64];
    int i, tags_count;
    time_t time_value;
    struct timeval tv;
    struct tm gm_time;

    hdata = relay_hdata_line_data;
    pointer = line_data;

    json = cJSON_CreateObject ();
    if (!json)
        return NULL;

    if (!line_data)
        return json;

    MSG_ADD_HDATA_VAR(Number, "id", integer, "id");
    MSG_ADD_HDATA_VAR(Number, "y", integer, "y");
    MSG_ADD_HDATA_TIME_USEC("date", "date", "date_usec");
    MSG_ADD_HDATA_TIME_USEC("date_printed", "date_printed", "date_usec_printed");
    MSG_ADD_HDATA_VAR(Bool, "displayed", char, "displayed");
    MSG_ADD_HDATA_VAR(Bool, "highlight", char, "highlight");
    MSG_ADD_HDATA_VAR(Number, "notify_level", char, "notify_level");
    MSG_ADD_HDATA_STR_COLORS("prefix", "prefix");
    MSG_ADD_HDATA_STR_COLORS("message", "message");

    /* tags */
    json_tags = cJSON_CreateArray ();
    if (json_tags)
    {
        tags_count = weechat_hdata_integer (hdata, line_data, "tags_count");
        for (i = 0; i < tags_count; i++)
        {
            snprintf (str_var, sizeof (str_var), "%d|tags_array", i);
            cJSON_AddItemToArray (
                json_tags,
                cJSON_CreateString (weechat_hdata_string (hdata, line_data, str_var)));
        }
    }
    cJSON_AddItemToObject(json, "tags", json_tags);

    return json;
}

/*
 * Creates a JSON object with an array of buffer lines.
 */

cJSON *
relay_api_msg_lines_to_json (struct t_gui_buffer *buffer,
                             long lines,
                             enum t_relay_api_colors colors)
{
    cJSON *json;
    struct t_gui_lines *ptr_lines;
    struct t_gui_line *ptr_line;
    struct t_gui_line_data *ptr_line_data;
    long i, count;

    json = cJSON_CreateArray ();
    if (!json)
        return NULL;

    if (lines == 0)
        return json;

    ptr_lines = weechat_hdata_pointer (relay_hdata_buffer, buffer, "own_lines");
    if (!ptr_lines)
        return json;

    if (lines < 0)
    {
        /* search start line from the last line */
        ptr_line = weechat_hdata_pointer (relay_hdata_lines, ptr_lines, "last_line");
        if (ptr_line)
        {
            for (i = 1; i < lines * -1; i++)
            {
                ptr_line = weechat_hdata_move (relay_hdata_line, ptr_line, -1);
                if (!ptr_line)
                    break;
            }
            if (!ptr_line)
                ptr_line = weechat_hdata_pointer (relay_hdata_lines, ptr_lines, "first_line");
        }
    }
    else
    {
        ptr_line = weechat_hdata_pointer (relay_hdata_lines, ptr_lines, "first_line");
    }

    if (!ptr_line)
        return json;

    count = 0;
    while (ptr_line)
    {
        ptr_line_data = weechat_hdata_pointer (relay_hdata_line, ptr_line, "data");
        if (ptr_line_data)
        {
            cJSON_AddItemToArray (
                json,
                relay_api_msg_line_data_to_json (ptr_line_data, colors));
        }
        count++;
        if ((lines > 0) && (count >= lines))
            break;
        ptr_line = weechat_hdata_move (relay_hdata_line, ptr_line, 1);
    }

    return json;
}

/*
 * Creates a nick JSON object.
 */

cJSON *
relay_api_msg_nick_to_json (struct t_gui_nick *nick)
{
    struct t_hdata *hdata;
    struct t_gui_nick *pointer;
    struct t_gui_nick_group *ptr_group;
    cJSON *json;
    const char *ptr_string;

    hdata = relay_hdata_nick;
    pointer = nick;

    json = cJSON_CreateObject ();
    if (!json)
        return NULL;

    if (!nick)
        return json;

    MSG_ADD_HDATA_VAR(Number, "id", longlong, "id");
    ptr_group = weechat_hdata_pointer (relay_hdata_nick, nick, "group");
    cJSON_AddItemToObject (
        json, "parent_group_id",
        cJSON_CreateNumber (
            (ptr_group) ?
            weechat_hdata_longlong (relay_hdata_nick_group, ptr_group, "id") : -1));
    MSG_ADD_HDATA_STR("prefix", "prefix");
    MSG_ADD_HDATA_STR("prefix_color", "prefix_color");
    MSG_ADD_HDATA_STR("name", "name");
    MSG_ADD_HDATA_STR("color", "color");
    MSG_ADD_HDATA_VAR(Bool, "visible", integer, "visible");

    return json;
}

/*
 * Creates a nick group JSON object.
 */

cJSON *
relay_api_msg_nick_group_to_json (struct t_gui_nick_group *nick_group)
{
    struct t_hdata *hdata;
    struct t_gui_nick_group *pointer, *ptr_group;
    struct t_gui_nick *ptr_nick;
    cJSON *json, *json_groups, *json_nicks;
    const char *ptr_string;

    hdata = relay_hdata_nick_group;
    pointer = nick_group;

    json = cJSON_CreateObject ();
    if (!json)
        return NULL;

    if (!nick_group)
        return json;

    MSG_ADD_HDATA_VAR(Number, "id", longlong, "id");
    ptr_group = weechat_hdata_pointer (relay_hdata_nick_group, nick_group, "parent");
    cJSON_AddItemToObject (
        json, "parent_group_id",
        cJSON_CreateNumber (
            (ptr_group) ?
            weechat_hdata_longlong (relay_hdata_nick_group, ptr_group, "id") : -1));
    MSG_ADD_HDATA_STR("name", "name");
    MSG_ADD_HDATA_STR("color", "color");
    MSG_ADD_HDATA_VAR(Bool, "visible", integer, "visible");

    json_groups = cJSON_CreateArray ();
    if (json_groups)
    {
        ptr_group = weechat_hdata_pointer (relay_hdata_nick_group, nick_group, "children");
        if (ptr_group)
        {
            while (ptr_group)
            {
                cJSON_AddItemToArray (
                    json_groups,
                    relay_api_msg_nick_group_to_json (ptr_group));
                ptr_group = weechat_hdata_move (relay_hdata_nick_group, ptr_group, 1);
            }
        }
        cJSON_AddItemToObject (json, "groups", json_groups);
    }

    json_nicks = cJSON_CreateArray ();
    if (json_nicks)
    {
        ptr_nick = weechat_hdata_pointer (relay_hdata_nick_group, nick_group, "nicks");
        if (ptr_nick)
        {
            while (ptr_nick)
            {
                cJSON_AddItemToArray (
                    json_nicks,
                    relay_api_msg_nick_to_json (ptr_nick));
                ptr_nick = weechat_hdata_move (relay_hdata_nick, ptr_nick, 1);
            }
        }
        cJSON_AddItemToObject (json, "nicks", json_nicks);
    }

    return json;
}

/*
 * Creates a JSON object with a hotlist entry.
 */

cJSON *
relay_api_msg_hotlist_to_json (struct t_gui_hotlist *hotlist)
{
    struct t_hdata *hdata;
    struct t_gui_hotlist *pointer;
    struct t_gui_buffer *buffer;
    cJSON *json, *json_count;
    time_t time_value;
    struct timeval tv;
    struct tm gm_time;
    char str_time[256], str_key[32];
    int i, array_size;
    long long buffer_id;

    hdata = relay_hdata_hotlist;
    pointer = hotlist;

    json = cJSON_CreateObject ();
    if (!json)
        return NULL;

    if (!hotlist)
        return json;

    MSG_ADD_HDATA_VAR(Number, "priority", integer, "priority");
    MSG_ADD_HDATA_TIME_USEC("date", "time", "time_usec");
    buffer = weechat_hdata_pointer (hdata, hotlist, "buffer");
    buffer_id = (buffer) ?
        weechat_hdata_longlong (relay_hdata_buffer, buffer, "id") : -1;
    cJSON_AddItemToObject (json, "buffer_id", cJSON_CreateNumber (buffer_id));

    json_count = cJSON_CreateArray ();
    if (json_count)
    {
        array_size = weechat_hdata_get_var_array_size (hdata, hotlist, "count");
        for (i = 0; i < array_size; i++)
        {
            snprintf (str_key, sizeof (str_key), "%d|count", i);
            cJSON_AddItemToArray (
                json_count,
                cJSON_CreateNumber (weechat_hdata_integer (hdata, hotlist, str_key)));
        }
    }
    cJSON_AddItemToObject (json, "count", json_count);

    return json;
}
