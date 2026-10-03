/*
 * SPDX-FileCopyrightText: 2023-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Build JSON messages for "api" protocol */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>

#include "../../weechat-plugin.h"
#include "../relay.h"
#include "../relay-client.h"
#include "../relay-http.h"
#include "../relay-json.h"
#include "../relay-websocket.h"
#include "relay-api.h"
#include "relay-api-msg.h"
#include "relay-api-protocol.h"

#define MSG_ADD_STR_BUF(__json_name, __string)                          \
    relay_json_object_add(                                              \
        json, __json_name,                                              \
        relay_json_new_string (__string));

#define MSG_ADD_STR_PTR(__json_name, __string)                          \
    relay_json_object_add(                                              \
        json, __json_name,                                              \
        relay_json_new_string ((__string) ? __string : ""));

#define MSG_ADD_HDATA_VAR(__json_type, __json_name,                     \
                          __var_type, __var_name)                       \
    relay_json_object_add(                                              \
        json, __json_name,                                              \
        relay_json_new_##__json_type (                                  \
            weechat_hdata_##__var_type (hdata, pointer, __var_name)));

#define MSG_ADD_HDATA_TIME_USEC(__json_name,                            \
                                __var_name, __var_name_usec)            \
    tv.tv_sec = weechat_hdata_time (hdata, pointer, __var_name);        \
    tv.tv_usec = weechat_hdata_integer (hdata, pointer,                 \
                                        __var_name_usec);               \
    weechat_util_strftimeval (str_time, sizeof (str_time),              \
                              "%@%FT%T.%fZ", &tv);                      \
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
 * Send JSON response to client (internal use).
 *
 * Return number of bytes sent to client, -1 if error.
 */

int
relay_api_msg_send_json_internal (struct t_relay_client *client,
                                  int return_code,
                                  const char *message,
                                  const char *event_name,
                                  long long event_buffer_id,
                                  const char *headers,
                                  const char *body_type,
                                  struct t_relay_json *json_body)
{
    struct t_relay_json *json, *ptr_body;
    char *string, *request;
    int num_bytes, length;

    if (!client || !message)
        return -1;

    num_bytes = -1;

    if (client->websocket == RELAY_CLIENT_WEBSOCKET_READY)
    {
        /*
         * With established websocket, we return JSON string instead of
         * an HTTP response.
         */
        json = relay_json_new_object ();
        if (json)
        {
            relay_json_object_add (json, "code", relay_json_new_number (return_code));
            relay_json_object_add (json, "message", relay_json_new_string (message));
            if (event_name)
            {
                relay_json_object_add (
                    json, "event_name",
                    relay_json_new_string ((event_name) ? event_name : ""));
                relay_json_object_add (
                    json, "buffer_id",
                    relay_json_new_number (event_buffer_id));
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
                    relay_json_object_add (json, "request",
                                           relay_json_new_string (request));
                    relay_json_object_add (
                        json, "request_body",
                        (client->http_req->body) ?
                        relay_json_parse (client->http_req->body) : relay_json_new_null ());
                    free (request);
                }
                relay_json_object_add (
                    json, "request_id",
                    (client->http_req->id) ?
                    relay_json_new_string (client->http_req->id) : relay_json_new_null ());
            }
            relay_json_object_add (
                json, "body_type",
                (body_type) ?
                relay_json_new_string (body_type) : relay_json_new_null ());
            relay_json_object_add (
                json, "body",
                (json_body) ? json_body : relay_json_new_null ());
            string = relay_json_print (json);
            num_bytes = relay_client_send (
                client,
                RELAY_MSG_STANDARD,
                string,
                (string) ? strlen (string) : 0,
                NULL);  /* raw_message */
            free (string);
            /*
             * Detach the body, which is owned by the caller (if the body
             * is the JSON null created above, it is freed here).
             */
            ptr_body = relay_json_object_detach (json, "body");
            if (ptr_body != json_body)
                relay_json_free (ptr_body);
            relay_json_free (json);
        }
    }
    else
    {
        string = (json_body) ? relay_json_print (json_body) : NULL;
        num_bytes = relay_http_send_json (client, return_code, message, headers,
                                          string);
        free (string);
    }

    return num_bytes;
}

/*
 * Send JSON response to client (internal use).
 *
 * Return number of bytes sent to client, -1 if error.
 */

int
relay_api_msg_send_json (struct t_relay_client *client,
                         int return_code,
                         const char *message,
                         const char *headers,
                         const char *body_type,
                         struct t_relay_json *json_body)
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
 * Send JSON error to client.
 *
 * Return number of bytes sent to client, -1 if error.
 */

int
relay_api_msg_send_error_json (struct t_relay_client *client,
                               int return_code,
                               const char *message,
                               const char *headers,
                               const char *format, ...)
{
    struct t_relay_json *json;
    int num_bytes;
    char *str_json;

    if (!client || !message || !format)
        return -1;

    weechat_va_format (format);
    if (!vbuffer)
        return -1;

    num_bytes = -1;

    json = relay_json_new_object ();
    if (!json)
        return -1;

    relay_json_object_add (json, "error", relay_json_new_string (vbuffer));

    if (client->websocket == RELAY_CLIENT_WEBSOCKET_READY)
    {
        /*
         * With established websocket, we return JSON string instead of
         * an HTTP response.
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
        str_json = relay_json_print (json);
        num_bytes = relay_http_send_json (client, return_code, message,
                                          headers, str_json);
        free (str_json);
    }

    relay_json_free (json);
    free (vbuffer);
    return num_bytes;
}

/*
 * Send event to the client.
 *
 * Return number of bytes sent to client, -1 if error.
 */

int
relay_api_msg_send_event (struct t_relay_client *client,
                          const char *name,
                          long long buffer_id,
                          const char *body_type,
                          struct t_relay_json *json_body)
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
 * Add a buffer local variable into a JSON object.
 */

void
relay_api_msg_buffer_add_local_vars_cb (void *data,
                                        struct t_hashtable *hashtable,
                                        const void *key,
                                        const void *value)
{
    struct t_relay_json *json;

    /* Make C compiler happy. */
    (void) hashtable;

    json = (struct t_relay_json *)data;

    relay_json_object_add (
        json,
        (const char *)key,
        relay_json_new_string ((value) ? (const char *)value : ""));
}

/*
 * Create a JSON object with a buffer.
 */

struct t_relay_json *
relay_api_msg_buffer_to_json (struct t_gui_buffer *buffer,
                              long lines,
                              long lines_free,
                              int nicks,
                              enum t_relay_api_colors colors)
{
    struct t_hdata *hdata;
    struct t_gui_buffer *pointer;
    struct t_gui_lines *ptr_lines;
    struct t_gui_line *ptr_line;
    struct t_gui_line_data *ptr_line_data;
    struct t_relay_json *json, *json_local_vars, *json_lines;
    struct t_relay_json *json_nicklist_root;
    const char *ptr_string;
    char *string;
    long long last_read_line_id;
    int first_line_not_read;

    hdata = relay_hdata_buffer;
    pointer = buffer;

    json = relay_json_new_object ();
    if (!json)
        return NULL;

    if (!buffer)
        return json;

    MSG_ADD_HDATA_VAR(number, "id", longlong, "id");
    MSG_ADD_HDATA_STR("name", "full_name");
    MSG_ADD_HDATA_STR("short_name", "short_name");
    MSG_ADD_HDATA_VAR(number, "number", integer, "number");
    ptr_string = weechat_buffer_get_string (buffer, "type");
    if (weechat_strcmp (ptr_string, "free") == 0)
        lines = lines_free;
    MSG_ADD_STR_PTR("type", ptr_string);
    MSG_ADD_HDATA_VAR(bool, "hidden", integer, "hidden");
    MSG_ADD_HDATA_STR_COLORS("title", "title");
    MSG_ADD_HDATA_STR_COLORS("modes", "modes");
    MSG_ADD_HDATA_STR_COLORS("input_prompt", "input_prompt");
    MSG_ADD_HDATA_STR("input", "input_buffer");
    MSG_ADD_HDATA_VAR(number, "input_position", integer, "input_buffer_pos");
    MSG_ADD_HDATA_VAR(bool, "input_multiline", integer, "input_multiline");
    MSG_ADD_HDATA_VAR(bool, "nicklist", integer, "nicklist");
    MSG_ADD_HDATA_VAR(bool, "nicklist_case_sensitive", integer, "nicklist_case_sensitive");
    MSG_ADD_HDATA_VAR(bool, "nicklist_display_groups", integer, "nicklist_display_groups");
    MSG_ADD_HDATA_VAR(bool, "time_displayed", integer, "time_for_each_line");
    MSG_ADD_HDATA_VAR(bool, "prefix_displayed", integer, "prefix_for_each_line");

    /* Local variables */
    json_local_vars = relay_json_new_object ();
    if (json_local_vars)
    {
        weechat_hashtable_map (
            weechat_hdata_pointer (hdata, buffer, "local_variables"),
            &relay_api_msg_buffer_add_local_vars_cb,
            json_local_vars);
        relay_json_object_add (json, "local_variables", json_local_vars);
    }

    /* Keys local to buffer */
    relay_json_object_add (json, "keys", relay_api_msg_keys_to_json (buffer));

    /* Lines */
    if (lines != 0)
    {
        json_lines = relay_api_msg_lines_to_json (buffer, lines, colors);
        if (json_lines)
            relay_json_object_add (json, "lines", json_lines);
    }
    /*
     * "last_read_line_id" is the id of the last line read, or -1 if there is no
     * read marker in the buffer; in this case, "first_line_not_read" tells if
     * the marker is before the first line (nothing read) or if it has been
     * removed because all the lines have been read.
     */
    last_read_line_id = -1;
    first_line_not_read = 0;
    ptr_lines = weechat_hdata_pointer (relay_hdata_buffer, buffer, "own_lines");
    if (ptr_lines)
    {
        first_line_not_read = weechat_hdata_integer (relay_hdata_lines,
                                                     ptr_lines,
                                                     "first_line_not_read");
        ptr_line = weechat_hdata_pointer (relay_hdata_lines, ptr_lines, "last_read_line");
        if (ptr_line)
        {
            ptr_line_data = weechat_hdata_pointer (relay_hdata_line, ptr_line, "data");
            if (ptr_line_data)
            {
                last_read_line_id = weechat_hdata_longlong (relay_hdata_line_data,
                                                            ptr_line_data, "id");
            }
        }
    }
    relay_json_object_add (
        json, "last_read_line_id",
        relay_json_new_number (last_read_line_id));
    relay_json_object_add (
        json, "first_line_not_read",
        relay_json_new_bool (first_line_not_read));

    /* Nicks */
    if (nicks)
    {
        json_nicklist_root = relay_api_msg_nick_group_to_json (
            weechat_hdata_pointer (hdata, buffer, "nicklist_root"),
            colors);
        if (json_nicklist_root)
            relay_json_object_add (json, "nicklist_root", json_nicklist_root);
    }

    return json;
}

/*
 * Create a JSON object with a buffer key.
 */

struct t_relay_json *
relay_api_msg_key_to_json (struct t_gui_key *key)
{
    struct t_hdata *hdata;
    struct t_gui_key *pointer;
    struct t_relay_json *json;
    const char *ptr_string;

    hdata = relay_hdata_key;
    pointer = key;

    json = relay_json_new_object ();
    if (!json)
        return NULL;

    if (!key)
        return json;

    MSG_ADD_HDATA_STR("key", "key");
    MSG_ADD_HDATA_STR("command", "command");

    return json;
}

/*
 * Create a JSON object with an array of buffer keys.
 */

struct t_relay_json *
relay_api_msg_keys_to_json (struct t_gui_buffer *buffer)
{
    struct t_relay_json *json;
    struct t_gui_key *ptr_key;

    json = relay_json_new_array ();
    if (!json)
        return NULL;

    ptr_key = weechat_hdata_pointer (relay_hdata_buffer, buffer, "keys");
    while (ptr_key)
    {
        relay_json_array_add (json, relay_api_msg_key_to_json (ptr_key));
        ptr_key = weechat_hdata_move (relay_hdata_key, ptr_key, 1);
    }

    return json;
}

/*
 * Create a JSON object with a buffer line data.
 */

struct t_relay_json *
relay_api_msg_line_data_to_json (struct t_gui_line_data *line_data,
                                 enum t_relay_api_colors colors)
{
    struct t_hdata *hdata;
    struct t_gui_line_data *pointer;
    struct t_relay_json *json, *json_tags;
    const char *ptr_string;
    char *string, str_time[256], str_var[64];
    int i, tags_count;
    struct timeval tv;

    hdata = relay_hdata_line_data;
    pointer = line_data;

    json = relay_json_new_object ();
    if (!json)
        return NULL;

    if (!line_data)
        return json;

    MSG_ADD_HDATA_VAR(number, "id", longlong, "id");
    MSG_ADD_HDATA_VAR(number, "y", integer, "y");
    MSG_ADD_HDATA_TIME_USEC("date", "date", "date_usec");
    MSG_ADD_HDATA_VAR(bool, "displayed", char, "displayed");
    MSG_ADD_HDATA_VAR(bool, "highlight", char, "highlight");
    MSG_ADD_HDATA_VAR(number, "notify_level", char, "notify_level");
    MSG_ADD_HDATA_STR_COLORS("prefix", "prefix");
    MSG_ADD_HDATA_STR_COLORS("message", "message");

    /* Tags */
    json_tags = relay_json_new_array ();
    if (json_tags)
    {
        tags_count = weechat_hdata_integer (hdata, line_data, "tags_count");
        for (i = 0; i < tags_count; i++)
        {
            snprintf (str_var, sizeof (str_var), "%d|tags_array", i);
            relay_json_array_add (
                json_tags,
                relay_json_new_string (weechat_hdata_string (hdata, line_data, str_var)));
        }
    }
    relay_json_object_add(json, "tags", json_tags);

    return json;
}

/*
 * Create a JSON object with an array of buffer lines.
 */

struct t_relay_json *
relay_api_msg_lines_to_json (struct t_gui_buffer *buffer,
                             long lines,
                             enum t_relay_api_colors colors)
{
    struct t_relay_json *json;
    struct t_gui_lines *ptr_lines;
    struct t_gui_line *ptr_line;
    struct t_gui_line_data *ptr_line_data;
    long i, count;

    json = relay_json_new_array ();
    if (!json)
        return NULL;

    if (lines == 0)
        return json;

    ptr_lines = weechat_hdata_pointer (relay_hdata_buffer, buffer, "own_lines");
    if (!ptr_lines)
        return json;

    if (lines < 0)
    {
        /* Search start line from the last line. */
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
            relay_json_array_add (
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
 * Create a nick JSON object.
 */

struct t_relay_json *
relay_api_msg_nick_to_json (struct t_gui_nick *nick,
                            enum t_relay_api_colors colors)
{
    struct t_hdata *hdata;
    struct t_gui_nick *pointer;
    struct t_gui_nick_group *ptr_group;
    struct t_relay_json *json;
    const char *ptr_string, *ptr_color;
    char *string;

    hdata = relay_hdata_nick;
    pointer = nick;

    json = relay_json_new_object ();
    if (!json)
        return NULL;

    if (!nick)
        return json;

    MSG_ADD_HDATA_VAR(number, "id", longlong, "id");
    ptr_group = weechat_hdata_pointer (relay_hdata_nick, nick, "group");
    relay_json_object_add (
        json, "parent_group_id",
        relay_json_new_number (
            (ptr_group) ?
            weechat_hdata_longlong (relay_hdata_nick_group, ptr_group, "id") : -1));
    MSG_ADD_HDATA_STR("prefix", "prefix");
    MSG_ADD_HDATA_STR("prefix_color_name", "prefix_color");
    MSG_ADD_HDATA_COLOR("prefix_color", "prefix_color");
    MSG_ADD_HDATA_STR("name", "name");
    MSG_ADD_HDATA_STR("color_name", "color");
    MSG_ADD_HDATA_COLOR("color", "color");
    MSG_ADD_HDATA_VAR(bool, "visible", integer, "visible");

    return json;
}

/*
 * Create a nick group JSON object.
 */

struct t_relay_json *
relay_api_msg_nick_group_to_json (struct t_gui_nick_group *nick_group,
                                  enum t_relay_api_colors colors)
{
    struct t_hdata *hdata;
    struct t_gui_nick_group *pointer, *ptr_group;
    struct t_gui_nick *ptr_nick;
    struct t_relay_json *json, *json_groups, *json_nicks;
    const char *ptr_string, *ptr_color;
    char *string;

    hdata = relay_hdata_nick_group;
    pointer = nick_group;

    json = relay_json_new_object ();
    if (!json)
        return NULL;

    if (!nick_group)
        return json;

    MSG_ADD_HDATA_VAR(number, "id", longlong, "id");
    ptr_group = weechat_hdata_pointer (relay_hdata_nick_group, nick_group, "parent");
    relay_json_object_add (
        json, "parent_group_id",
        relay_json_new_number (
            (ptr_group) ?
            weechat_hdata_longlong (relay_hdata_nick_group, ptr_group, "id") : -1));
    MSG_ADD_HDATA_STR("name", "name");
    MSG_ADD_HDATA_STR("color_name", "color");
    MSG_ADD_HDATA_COLOR("color", "color");
    MSG_ADD_HDATA_VAR(bool, "visible", integer, "visible");

    json_groups = relay_json_new_array ();
    if (json_groups)
    {
        ptr_group = weechat_hdata_pointer (relay_hdata_nick_group, nick_group, "children");
        if (ptr_group)
        {
            while (ptr_group)
            {
                relay_json_array_add (
                    json_groups,
                    relay_api_msg_nick_group_to_json (ptr_group, colors));
                ptr_group = weechat_hdata_move (relay_hdata_nick_group, ptr_group, 1);
            }
        }
        relay_json_object_add (json, "groups", json_groups);
    }

    json_nicks = relay_json_new_array ();
    if (json_nicks)
    {
        ptr_nick = weechat_hdata_pointer (relay_hdata_nick_group, nick_group, "nicks");
        if (ptr_nick)
        {
            while (ptr_nick)
            {
                relay_json_array_add (
                    json_nicks,
                    relay_api_msg_nick_to_json (ptr_nick, colors));
                ptr_nick = weechat_hdata_move (relay_hdata_nick, ptr_nick, 1);
            }
        }
        relay_json_object_add (json, "nicks", json_nicks);
    }

    return json;
}

/*
 * Create a JSON object with a completion entry.
 */

struct t_relay_json *
relay_api_msg_completion_to_json (struct t_gui_completion *completion)
{
    struct t_hdata *hdata;
    struct t_gui_completion *pointer;
    struct t_gui_completion_word *word;
    const char *ptr_string;
    struct t_arraylist *ptr_list;
    struct t_relay_json *json, *json_array;
    int context, i, size;

    hdata = relay_hdata_completion;
    pointer = completion;

    json = relay_json_new_object ();
    if (!json)
        return NULL;

    if (!completion)
        return json;

    ptr_list = weechat_hdata_pointer (relay_hdata_completion, completion, "list");
    if (!ptr_list)
        return json;

    /* Context */
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
    MSG_ADD_HDATA_VAR(number, "position_replace", integer, "position_replace");
    MSG_ADD_HDATA_VAR(bool, "add_space", integer, "add_space");

    json_array = relay_json_new_array ();
    size = weechat_arraylist_size (ptr_list);
    for (i = 0; i < size; i++)
    {
        word = (struct t_gui_completion_word *)weechat_arraylist_get (ptr_list, i);
        relay_json_array_add (
            json_array,
            relay_json_new_string (
                weechat_hdata_string (relay_hdata_completion_word, word, "word")));
    }
    relay_json_object_add (json, "list", json_array);

    return json;
}

/*
 * Create a JSON object with a hotlist entry.
 */

struct t_relay_json *
relay_api_msg_hotlist_to_json (struct t_gui_hotlist *hotlist)
{
    struct t_hdata *hdata;
    struct t_gui_hotlist *pointer;
    struct t_gui_buffer *buffer;
    struct t_relay_json *json, *json_count;
    struct timeval tv;
    char str_time[256], str_key[32];
    int i, array_size;
    long long buffer_id;

    hdata = relay_hdata_hotlist;
    pointer = hotlist;

    json = relay_json_new_object ();
    if (!json)
        return NULL;

    if (!hotlist)
        return json;

    MSG_ADD_HDATA_VAR(number, "priority", integer, "priority");
    MSG_ADD_HDATA_TIME_USEC("date", "time", "time_usec");
    buffer = weechat_hdata_pointer (hdata, hotlist, "buffer");
    buffer_id = (buffer) ?
        weechat_hdata_longlong (relay_hdata_buffer, buffer, "id") : -1;
    relay_json_object_add (json, "buffer_id",
                           relay_json_new_number (buffer_id));

    json_count = relay_json_new_array ();
    if (json_count)
    {
        array_size = weechat_hdata_get_var_array_size (hdata, hotlist, "count");
        for (i = 0; i < array_size; i++)
        {
            snprintf (str_key, sizeof (str_key), "%d|count", i);
            relay_json_array_add (
                json_count,
                relay_json_new_number (weechat_hdata_integer (hdata, hotlist, str_key)));
        }
    }
    relay_json_object_add (json, "count", json_count);

    return json;
}

/*
 * Create a JSON object with a script.
 */

struct t_relay_json *
relay_api_msg_script_to_json (struct t_hdata *hdata, void *script, const char *extension)
{
    struct t_relay_json *json;
    void *pointer;
    const char *ptr_string;
    char name[1024];

    if (!hdata)
        return NULL;

    pointer = script;

    json = relay_json_new_object ();
    if (!json)
        return NULL;

    if (!script)
        return json;

    snprintf (name, sizeof (name),
              "%s.%s",
              weechat_hdata_string (hdata, script, "name"),
              extension);
    MSG_ADD_STR_BUF("name", name);
    MSG_ADD_HDATA_STR("version", "version");
    MSG_ADD_HDATA_STR("description", "description");
    MSG_ADD_HDATA_STR("author", "author");
    MSG_ADD_HDATA_STR("license", "license");

    return json;
}
