/*
 * relay-api-msg.c - build JSON messages for "api" protocol
 *
 * Copyright (C) 2023-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <limits.h>
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

#define MSG_ADD_HDATA_TIME_USEC(__json_name,                            \
                                __var_name, __var_name_usec)            \
    time_value = weechat_hdata_time (hdata, pointer, __var_name);       \
    local_time = localtime (&time_value);                               \
    time_value -= local_time->tm_gmtoff;                                \
    local_time = localtime (&time_value);                               \
    tv.tv_sec = mktime (local_time);                                    \
    tv.tv_usec = weechat_hdata_integer (hdata, pointer,                 \
                                        __var_name_usec);               \
    weechat_util_strftimeval (str_time, sizeof (str_time),              \
                              "%FT%T.%fZ", &tv);                        \
    MSG_ADD_STR_BUF(__json_name, str_time);

#define MSG_ADD_HDATA_STR(__json_name, __var_name)                      \
    ptr_string = weechat_hdata_string (hdata, pointer, __var_name);     \
    MSG_ADD_STR_PTR(__json_name, ptr_string);

#define MSG_CONVERT_COLORS(__json_name, __string)                       \
    switch (colors)                                                     \
    {                                                                   \
        case RELAY_API_COLORS_ANSI:                                     \
            string = weechat_hook_modifier_exec (                       \
                "color_encode_ansi", NULL,                              \
                (__string) ? __string : "");                            \
            if (string)                                                 \
            {                                                           \
                MSG_ADD_STR_PTR(__json_name, string);                   \
                free (string);                                          \
            }                                                           \
            break;                                                      \
        case RELAY_API_COLORS_WEECHAT:                                  \
            MSG_ADD_STR_PTR(__json_name, __string);                     \
            break;                                                      \
        case RELAY_API_COLORS_STRIP:                                    \
            string = weechat_string_remove_color (                      \
                (__string) ? __string : "", NULL);                      \
            if (string)                                                 \
            {                                                           \
                MSG_ADD_STR_PTR(__json_name, string);                   \
                free (string);                                          \
            }                                                           \
        case RELAY_API_NUM_COLORS:                                      \
            break;                                                      \
    }

#define MSG_ADD_HDATA_STR_COLORS(__json_name, __var_name)               \
    ptr_string = weechat_hdata_string (hdata, pointer, __var_name);     \
    MSG_CONVERT_COLORS(__json_name, ptr_string);

#define MSG_ADD_HDATA_COLOR(__json_name, __var_name)                    \
    ptr_string = weechat_hdata_string (hdata, pointer, __var_name);     \
    ptr_color = (ptr_string && ptr_string[0]) ?                         \
        weechat_color (ptr_string) : NULL;                              \
    MSG_CONVERT_COLORS(__json_name, ptr_color);


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
                                  long long event_buffer_id,
                                  const char *headers,
                                  const char *body_type,
                                  cJSON *json_body)
{
    cJSON *json;
    char *string, *request;
    int num_bytes, length;

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
                cJSON_AddItemToObject (
                    json, "event_name",
                    cJSON_CreateString ((event_name) ? event_name : ""));
                cJSON_AddItemToObject (
                    json, "buffer_id",
                    cJSON_CreateNumber (event_buffer_id));
            }
            else
            {
                length = weechat_asprintf (
                    &request,
                    "%s%s%s",
                    (client->http_req->method) ? client->http_req->method : "",
                    (client->http_req->method) ? " " : "",
                    (client->http_req->path) ? client->http_req->path : "");
                if (length >= 0)
                {
                    cJSON_AddItemToObject (json, "request",
                                           cJSON_CreateString (request));
                    cJSON_AddItemToObject (
                        json, "request_body",
                        (client->http_req->body) ?
                        cJSON_Parse (client->http_req->body) : cJSON_CreateNull ());
                    free (request);
                }
                cJSON_AddItemToObject (
                    json, "request_id",
                    (client->http_req->id) ?
                    cJSON_CreateString (client->http_req->id) : cJSON_CreateNull ());
            }
            cJSON_AddItemToObject (
                json, "body_type",
                (body_type) ?
                cJSON_CreateString (body_type) : cJSON_CreateNull ());
            cJSON_AddItemToObject (
                json, "body",
                (json_body) ? json_body : cJSON_CreateNull ());
            string = cJSON_PrintUnformatted (json);
            num_bytes = relay_client_send (
                client,
                RELAY_MSG_STANDARD,
                string,
                (string) ? strlen (string) : 0,
                NULL);  /* raw_message */
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
                         const char *headers,
                         const char *body_type,
                         cJSON *json_body)
{
    return relay_api_msg_send_json_internal (client,
                                             return_code,
                                             message,
                                             NULL,  /* event_name */
                                             -1,    /* event_buffer_id */
                                             headers,
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
    int num_bytes;
    char *str_json;

    if (!client || !message || !format)
        return -1;

    weechat_va_format (format);
    if (!vbuffer)
        return -1;

    num_bytes = -1;

    json = cJSON_CreateObject ();
    if (!json)
        return -1;

    cJSON_AddItemToObject (json, "error", cJSON_CreateString (vbuffer));

    if (client->websocket == RELAY_CLIENT_WEBSOCKET_READY)
    {
        /*
         * with established websocket, we return JSON string instead of
         * an HTTP response
         */
        num_bytes = relay_api_msg_send_json_internal (
            client,
            return_code,
            message,
            NULL,  /* event_name */
            -1,    /* event_buffer_id */
            headers,
            NULL,  /* body_type */
            json);
    }
    else
    {
        str_json = cJSON_PrintUnformatted (json);
        num_bytes = relay_http_send_json (client, return_code, message,
                                          headers, str_json);
        free (str_json);
    }

    cJSON_Delete (json);
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
                          long long buffer_id,
                          const char *body_type,
                          cJSON *json_body)
{
    return relay_api_msg_send_json_internal (client,
                                             RELAY_API_HTTP_0_EVENT,
                                             name,
                                             buffer_id,
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
                              long lines_free,
                              int nicks,
                              enum t_relay_api_colors colors)
{
    struct t_hdata *hdata;
    struct t_gui_buffer *pointer;
    cJSON *json, *json_local_vars, *json_lines, *json_nicklist_root;
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
    if (weechat_strcmp (ptr_string, "free") == 0)
        lines = lines_free;
    MSG_ADD_STR_PTR("type", ptr_string);
    MSG_ADD_HDATA_VAR(Bool, "hidden", integer, "hidden");
    MSG_ADD_HDATA_STR_COLORS("title", "title");
    MSG_ADD_HDATA_STR_COLORS("modes", "modes");
    MSG_ADD_HDATA_STR_COLORS("input_prompt", "input_prompt");
    MSG_ADD_HDATA_STR("input", "input_buffer");
    MSG_ADD_HDATA_VAR(Number, "input_position", integer, "input_buffer_pos");
    MSG_ADD_HDATA_VAR(Bool, "input_multiline", integer, "input_multiline");
    MSG_ADD_HDATA_VAR(Bool, "nicklist", integer, "nicklist");
    MSG_ADD_HDATA_VAR(Bool, "nicklist_case_sensitive", integer, "nicklist_case_sensitive");
    MSG_ADD_HDATA_VAR(Bool, "nicklist_display_groups", integer, "nicklist_display_groups");
    MSG_ADD_HDATA_VAR(Bool, "time_displayed", integer, "time_for_each_line");

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

    /* keys local to buffer */
    cJSON_AddItemToObject (json, "keys", relay_api_msg_keys_to_json (buffer));

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
        json_nicklist_root = relay_api_msg_nick_group_to_json (
            weechat_hdata_pointer (hdata, buffer, "nicklist_root"),
            colors);
        if (json_nicklist_root)
            cJSON_AddItemToObject (json, "nicklist_root", json_nicklist_root);
    }

    return json;
}

/*
 * Creates a JSON object with a buffer key.
 */

cJSON *
relay_api_msg_key_to_json (struct t_gui_key *key)
{
    struct t_hdata *hdata;
    struct t_gui_key *pointer;
    cJSON *json;
    const char *ptr_string;

    hdata = relay_hdata_key;
    pointer = key;

    json = cJSON_CreateObject ();
    if (!json)
        return NULL;

    if (!key)
        return json;

    MSG_ADD_HDATA_STR("key", "key");
    MSG_ADD_HDATA_STR("command", "command");

    return json;
}

/*
 * Creates a JSON object with an array of buffer keys.
 */

cJSON *
relay_api_msg_keys_to_json (struct t_gui_buffer *buffer)
{
    cJSON *json;
    struct t_gui_key *ptr_key;

    json = cJSON_CreateArray ();
    if (!json)
        return NULL;

    ptr_key = weechat_hdata_pointer (relay_hdata_buffer, buffer, "keys");
    while (ptr_key)
    {
        cJSON_AddItemToArray (json, relay_api_msg_key_to_json (ptr_key));
        ptr_key = weechat_hdata_move (relay_hdata_key, ptr_key, 1);
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
    struct tm *local_time;

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
            for (i = -1; i > lines; i--)
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
relay_api_msg_nick_to_json (struct t_gui_nick *nick,
                            enum t_relay_api_colors colors)
{
    struct t_hdata *hdata;
    struct t_gui_nick *pointer;
    struct t_gui_nick_group *ptr_group;
    cJSON *json;
    const char *ptr_string, *ptr_color;
    char *string;

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
    MSG_ADD_HDATA_STR("prefix_color_name", "prefix_color");
    MSG_ADD_HDATA_COLOR("prefix_color", "prefix_color");
    MSG_ADD_HDATA_STR("name", "name");
    MSG_ADD_HDATA_STR("color_name", "color");
    MSG_ADD_HDATA_COLOR("color", "color");
    MSG_ADD_HDATA_VAR(Bool, "visible", integer, "visible");

    return json;
}

/*
 * Creates a nick group JSON object.
 */

cJSON *
relay_api_msg_nick_group_to_json (struct t_gui_nick_group *nick_group,
                                  enum t_relay_api_colors colors)
{
    struct t_hdata *hdata;
    struct t_gui_nick_group *pointer, *ptr_group;
    struct t_gui_nick *ptr_nick;
    cJSON *json, *json_groups, *json_nicks;
    const char *ptr_string, *ptr_color;
    char *string;

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
    MSG_ADD_HDATA_STR("color_name", "color");
    MSG_ADD_HDATA_COLOR("color", "color");
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
                    relay_api_msg_nick_group_to_json (ptr_group, colors));
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
                    relay_api_msg_nick_to_json (ptr_nick, colors));
                ptr_nick = weechat_hdata_move (relay_hdata_nick, ptr_nick, 1);
            }
        }
        cJSON_AddItemToObject (json, "nicks", json_nicks);
    }

    return json;
}

/*
 * Creates a JSON object with a completion entry.
 */

cJSON *
relay_api_msg_completion_to_json (struct t_gui_completion *completion)
{
    struct t_hdata *hdata;
    struct t_gui_completion *pointer;
    struct t_gui_completion_word *word;
    const char *ptr_string;
    struct t_arraylist *ptr_list;
    cJSON *json, *json_array;
    int context, i, size;

    hdata = relay_hdata_completion;
    pointer = completion;

    json = cJSON_CreateObject ();
    if (!json)
        return NULL;

    if (!completion)
        return json;

    ptr_list = weechat_hdata_pointer (relay_hdata_completion, completion, "list");
    if (!ptr_list)
        return json;

    /* context */
    context = weechat_hdata_integer (relay_hdata_completion, completion, "context");
    switch (context)
    {
        case 0:
            MSG_ADD_STR_PTR("context", "null");
            break;
        case 1:
            MSG_ADD_STR_PTR("context", "command");
            break;
        case 2:
            MSG_ADD_STR_PTR("context", "command_arg");
            break;
        default:
            MSG_ADD_STR_PTR("context", "auto");
            break;
    }

    MSG_ADD_HDATA_STR("base_word", "base_word");
    MSG_ADD_HDATA_VAR(Number, "position_replace", integer, "position_replace");
    MSG_ADD_HDATA_VAR(Bool, "add_space", integer, "add_space");

    json_array = cJSON_CreateArray ();
    size = weechat_arraylist_size (ptr_list);
    for (i = 0; i < size; i++)
    {
        word = (struct t_gui_completion_word *)weechat_arraylist_get (ptr_list, i);
        cJSON_AddItemToArray (
            json_array,
            cJSON_CreateString (
                weechat_hdata_string (relay_hdata_completion_word, word, "word")));
    }
    cJSON_AddItemToObject (json, "list", json_array);

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
    struct tm *local_time;
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
