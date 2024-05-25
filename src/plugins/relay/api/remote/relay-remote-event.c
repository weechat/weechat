/*
 * relay-remote-event.c - process events received from relay remote
 *
 * Copyright (C) 2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <gcrypt.h>
#include <gnutls/gnutls.h>
#include <cjson/cJSON.h>

#include "../../../weechat-plugin.h"
#include "../../relay.h"
#include "../../relay-auth.h"
#include "../../relay-config.h"
#include "../../relay-http.h"
#include "../../relay-raw.h"
#include "../../relay-remote.h"
#include "../../relay-websocket.h"
#include "../relay-api.h"
#include "relay-remote-event.h"
#include "relay-remote-network.h"

#define JSON_GET_NUM(__json, __var, __default)                     \
    json_obj = cJSON_GetObjectItem (__json, #__var);               \
    if (json_obj && cJSON_IsNumber (json_obj))                     \
        __var = cJSON_GetNumberValue (json_obj);                   \
    else                                                           \
        __var = __default;

#define JSON_GET_STR(__json, __var)                                \
    json_obj = cJSON_GetObjectItem (__json, #__var);               \
    if (json_obj && cJSON_IsString (json_obj))                     \
        __var = cJSON_GetStringValue (json_obj);                   \
    else                                                           \
        __var = NULL;

#define JSON_GET_BOOL(__json, __var)                               \
    json_obj = cJSON_GetObjectItem (__json, #__var);               \
    __var = cJSON_IsTrue (json_obj) ? 1 : 0;


/*
 * Searches a buffer used for a remote.
 *
 * Returns pointer to buffer, NULL if not found.
 */

struct t_gui_buffer *
relay_remote_event_search_buffer (struct t_relay_remote *remote, long long id)
{
    struct t_gui_buffer *ptr_buffer, *ptr_buffer_found;
    const char *ptr_name, *ptr_id;
    char str_id[64];

    if (!remote || (id < 0))
        return NULL;

    ptr_buffer_found = NULL;

    snprintf (str_id, sizeof (str_id), "%lld", id);

    ptr_buffer = weechat_hdata_get_list (relay_hdata_buffer, "gui_buffers");
    while (ptr_buffer)
    {
        ptr_name = weechat_buffer_get_string (ptr_buffer, "localvar_relay_remote");
        ptr_id = weechat_buffer_get_string (ptr_buffer, "localvar_relay_remote_id");
        if (ptr_name
            && ptr_id
            && (strcmp (ptr_name, remote->name) == 0)
            && (strcmp (ptr_id, str_id) == 0))
        {
            ptr_buffer_found = ptr_buffer;
            break;
        }
        ptr_buffer = weechat_hdata_move (relay_hdata_buffer, ptr_buffer, 1);
    }

    return ptr_buffer_found;
}

/*
 * Gets the remote buffer id.
 *
 * Returns id found, -1 if error.
 */

long long
relay_remote_event_get_buffer_id (struct t_gui_buffer *buffer)
{
    const char *ptr_id;
    char *error;
    long long buffer_id;

    if (!buffer)
        return -1;

    ptr_id = weechat_buffer_get_string (buffer, "localvar_relay_remote_id");
    if (!ptr_id)
        return -1;

    error = NULL;
    buffer_id = strtoll (ptr_id, &error, 10);
    if (!error || error[0])
        return -1;

    return buffer_id;
}

/*
 * Callback for a line event.
 */

RELAY_REMOTE_EVENT_CALLBACK(line)
{
    cJSON *json_obj, *json_tags, *json_tag;
    const char *date, *prefix, *message, *ptr_tag;
    char **tags;
    int y, highlight, tag_notify_highlight;
    struct timeval tv_date;

    if (!event->buffer || !event->json)
        return WEECHAT_RC_OK;

    JSON_GET_NUM(event->json, y, -1);
    JSON_GET_STR(event->json, date);
    JSON_GET_BOOL(event->json, highlight);
    JSON_GET_STR(event->json, prefix);
    JSON_GET_STR(event->json, message);

    if (!weechat_util_parse_time (date, &tv_date))
    {
        tv_date.tv_sec = 0;
        tv_date.tv_usec = 0;
    }

    tag_notify_highlight = 0;

    tags = weechat_string_dyn_alloc (256);
    if (tags)
    {
        json_tags = cJSON_GetObjectItem (event->json, "tags");
        if (json_tags && cJSON_IsArray (json_tags))
        {
            cJSON_ArrayForEach (json_tag, json_tags)
            {
                ptr_tag = cJSON_GetStringValue (json_tag);
                if (ptr_tag)
                {
                    if (*tags[0])
                        weechat_string_dyn_concat (tags, ",", -1);
                    if (highlight && (strncmp (ptr_tag, "notify_", 7) == 0))
                    {
                        weechat_string_dyn_concat (tags, "notify_highlight", -1);
                        tag_notify_highlight = 1;
                    }
                    else
                    {
                        weechat_string_dyn_concat (tags, ptr_tag, -1);
                    }
                }
            }
        }
        if (highlight && !tag_notify_highlight)
        {
            if (*tags[0])
                weechat_string_dyn_concat (tags, ",", -1);
            weechat_string_dyn_concat (tags, "notify_highlight", -1);
        }
    }

    if (y >= 0)
    {
        /* buffer with free content */
        weechat_printf_y_datetime_tags (
            event->buffer,
            y,
            tv_date.tv_sec,
            tv_date.tv_usec,
            (tags) ? *tags : NULL,
            "%s%s%s",
            (prefix && prefix[0]) ? prefix : "",
            (prefix && prefix[0]) ? "\t" : "",
            message);
    }
    else
    {
        /* buffer with formatted content */
        weechat_printf_datetime_tags (
            event->buffer,
            tv_date.tv_sec,
            tv_date.tv_usec,
            (tags) ? *tags : NULL,
            "%s%s%s",
            (prefix && prefix[0]) ? prefix : "",
            (prefix && prefix[0]) ? "\t" : "",
            message);
    }

    weechat_string_dyn_free (tags, 1);

    return WEECHAT_RC_OK;
}

/*
 * Adds or updates a nick on a buffer using JSON object.
 */

void
relay_remote_event_handle_nick (struct t_gui_buffer *buffer, cJSON *json)
{
    cJSON *json_obj;
    struct t_gui_nick *ptr_nick;
    struct t_gui_nick_group *ptr_parent_group;
    const char *name, *color_name, *prefix, *prefix_color_name;
    char str_id[128];
    long long id, parent_group_id;
    int visible;

    if (!buffer || !json)
        return;

    JSON_GET_NUM(json, id, -1);
    JSON_GET_NUM(json, parent_group_id, -1);
    JSON_GET_STR(json, name);
    JSON_GET_STR(json, color_name);
    JSON_GET_STR(json, prefix);
    JSON_GET_STR(json, prefix_color_name);
    JSON_GET_BOOL(json, visible);

    snprintf (str_id, sizeof (str_id), "==id:%lld", id);
    ptr_nick = weechat_nicklist_search_nick (buffer, NULL, str_id);
    if (ptr_nick)
    {
        /* update existing nick */
        snprintf (str_id, sizeof (str_id), "%lld", id);
        weechat_nicklist_nick_set (buffer, ptr_nick, "id", str_id);
        weechat_nicklist_nick_set (buffer, ptr_nick, "color", color_name);
        weechat_nicklist_nick_set (buffer, ptr_nick, "prefix", prefix);
        weechat_nicklist_nick_set (buffer, ptr_nick, "prefix_color", prefix_color_name);
        weechat_nicklist_nick_set (buffer, ptr_nick,
                                   "visible", (visible) ? "1" : "0");
    }
    else
    {
        /* create a new nick */
        if (parent_group_id < 0)
            return;
        snprintf (str_id, sizeof (str_id), "==id:%lld", parent_group_id);
        ptr_parent_group = weechat_nicklist_search_group (buffer, NULL, str_id);
        if (!ptr_parent_group)
            return;
        ptr_nick = weechat_nicklist_add_nick (buffer, ptr_parent_group,
                                              name, color_name,
                                              prefix, prefix_color_name,
                                              visible);
        if (ptr_nick)
        {
            snprintf (str_id, sizeof (str_id), "%lld", id);
            weechat_nicklist_nick_set (buffer, ptr_nick, "id", str_id);
        }
    }
}

/*
 * Adds or updates a nick group on a buffer using JSON object.
 */

void
relay_remote_event_handle_nick_group (struct t_gui_buffer *buffer, cJSON *json)
{
    cJSON *json_obj, *json_groups, *json_group, *json_nicks, *json_nick;
    struct t_gui_nick_group *ptr_group, *ptr_parent_group;
    const char *name, *color_name;
    char str_id[128];
    long long id, parent_group_id;
    int visible;

    if (!buffer || !json)
        return;

    JSON_GET_NUM(json, id, -1);
    JSON_GET_NUM(json, parent_group_id, -1);
    JSON_GET_STR(json, name);
    JSON_GET_STR(json, color_name);
    JSON_GET_BOOL(json, visible);

    snprintf (str_id, sizeof (str_id), "==id:%lld", id);
    ptr_group = weechat_nicklist_search_group (buffer, NULL, str_id);
    if (ptr_group)
    {
        /* update existing group */
        snprintf (str_id, sizeof (str_id), "%lld", id);
        weechat_nicklist_group_set (buffer, ptr_group, "id", str_id);
        weechat_nicklist_group_set (buffer, ptr_group, "color", color_name);
        weechat_nicklist_group_set (buffer, ptr_group,
                                    "visible", (visible) ? "1" : "0");
    }
    else
    {
        /* create a new group */
        if (parent_group_id < 0)
            return;
        snprintf (str_id, sizeof (str_id), "==id:%lld", parent_group_id);
        ptr_parent_group = weechat_nicklist_search_group (buffer, NULL, str_id);
        if (!ptr_parent_group)
            return;
        ptr_group = weechat_nicklist_add_group (buffer, ptr_parent_group,
                                                name, color_name, visible);
        if (ptr_group)
        {
            snprintf (str_id, sizeof (str_id), "%lld", id);
            weechat_nicklist_group_set (buffer, ptr_group, "id", str_id);
        }
    }

    /* add subgroups */
    json_groups = cJSON_GetObjectItem (json, "groups");
    if (json_groups && cJSON_IsArray (json_groups))
    {
        cJSON_ArrayForEach (json_group, json_groups)
        {
            relay_remote_event_handle_nick_group (buffer, json_group);
        }
    }

    /* add nicks */
    json_nicks = cJSON_GetObjectItem (json, "nicks");
    if (json_nicks && cJSON_IsArray (json_nicks))
    {
        cJSON_ArrayForEach (json_nick, json_nicks)
        {
            relay_remote_event_handle_nick (buffer, json_nick);
        }
    }
}

/*
 * Callback for a nick group event.
 */

RELAY_REMOTE_EVENT_CALLBACK(nick_group)
{
    struct t_gui_nick_group *ptr_group;
    char str_id[128];
    cJSON *json_obj;
    long long id;

    if (!event->buffer || !event->json)
        return WEECHAT_RC_OK;

    if (weechat_strcmp (event->name, "nicklist_group_removing") == 0)
    {
        JSON_GET_NUM(event->json, id, -1);
        snprintf (str_id, sizeof (str_id), "==id:%lld", id);
        ptr_group = weechat_nicklist_search_group (event->buffer, NULL, str_id);
        if (ptr_group)
            weechat_nicklist_remove_group (event->buffer, ptr_group);
    }
    else
    {
        relay_remote_event_handle_nick_group (event->buffer, event->json);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for a nick event.
 */

RELAY_REMOTE_EVENT_CALLBACK(nick)
{
    struct t_gui_nick *ptr_nick;
    char str_id[128];
    cJSON *json_obj;
    long long id;

    if (!event->buffer || !event->json)
        return WEECHAT_RC_OK;

    if (weechat_strcmp (event->name, "nicklist_nick_removing") == 0)
    {
        JSON_GET_NUM(event->json, id, -1);
        snprintf (str_id, sizeof (str_id), "==id:%lld", id);
        ptr_nick = weechat_nicklist_search_nick (event->buffer, NULL, str_id);
        if (ptr_nick)
            weechat_nicklist_remove_nick (event->buffer, ptr_nick);
    }
    else
    {
        relay_remote_event_handle_nick (event->buffer, event->json);
    }

    return WEECHAT_RC_OK;
}

/*
 * Applies properties to a buffer.
 */

void
relay_remote_event_apply_props (void *data,
                                struct t_hashtable *hashtable,
                                const void *key,
                                const void *value)
{
    /* make C compiler happy */
    (void) hashtable;

    weechat_buffer_set ((struct t_gui_buffer *)data,
                        (const char *)key,
                        (const char *)value);
}

/*
 * Callback for remote buffer input.
 */

int
relay_remote_event_buffer_input_cb (const void *pointer,
                                    void *data,
                                    struct t_gui_buffer *buffer,
                                    const char *input_data)
{
    struct t_relay_remote *remote;
    cJSON *json, *json_body;
    long long buffer_id;

    /* make C compiler happy */
    (void) data;

    remote = (struct t_relay_remote *)pointer;

    json = NULL;

    buffer_id = relay_remote_event_get_buffer_id (buffer);
    if (buffer_id < 0)
        goto error;

    json = cJSON_CreateObject ();
    if (!json)
        goto error;

    cJSON_AddItemToObject (json, "request",
                           cJSON_CreateString ("POST /api/input"));
    json_body = cJSON_CreateObject ();
    if (!json_body)
        goto error;

    cJSON_AddItemToObject (json_body, "buffer_id",
                           cJSON_CreateNumber (buffer_id));
    cJSON_AddItemToObject (json_body, "command",
                           cJSON_CreateString (input_data));
    cJSON_AddItemToObject (json, "body", json_body);

    relay_remote_network_send_json (remote, json);

    cJSON_Delete (json);

    return WEECHAT_RC_OK;

error:
    if (json)
        cJSON_Delete (json);
    return WEECHAT_RC_OK;
}

/*
 * Checks if a local variable received in JSON object "local_variables" can
 * be used.
 *
 * The following variables are ignored and must NOT be used:
 *   - "plugin": contains the plugin name
 *   - "name": contains the buffer name
 *   - "relay_remote*": variables reserved for relay remote (in case of nested
 *     remotes, the variables are not propagated)
 *
 * Returns:
 *   1: variable can be used
 *   0: variable must NOT be used (reserved)
 */

int
relay_remote_event_check_local_var (const char *name)
{
    if (!name)
        return 0;

    return ((strcmp (name, "plugin") != 0)
            && (strcmp (name, "name") != 0)
            && (strncmp (name, "relay_remote", 12) != 0)) ?
        1 : 0;
}

/*
 * Remove local variables in buffer that are not in the JSON object
 * "local_variables".
 */

void
relay_remote_event_remove_localvar_cb (void *data,
                                       struct t_hashtable *hashtable,
                                       const void *key,
                                       const void *value)
{
    void **pointers;
    struct t_gui_buffer *buffer;
    cJSON *json;
    char str_local_var[1024];

    /* make C compiler happy */
    (void) hashtable;
    (void) value;

    pointers = (void **)data;
    buffer = pointers[0];
    json = pointers[1];

    if (!relay_remote_event_check_local_var (key))
        return;

    if (!cJSON_GetObjectItem (json, key))
    {
        /* local variable removed on remote? => remove it locally */
        snprintf (str_local_var, sizeof (str_local_var),
                  "localvar_del_%s", (const char *)key);
        weechat_buffer_set (buffer, str_local_var, "");
    }
}

/*
 * Callback for a buffer event or response to GET /api/buffers.
 */

RELAY_REMOTE_EVENT_CALLBACK(buffer)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_hashtable *buffer_props;
    struct t_relay_remote_event event_line;
    struct t_hashtable *local_variables;
    cJSON *json_obj, *json_keys, *json_key, *json_key_name, *json_key_command;
    cJSON *json_vars, *json_var, *json_lines, *json_line, *json_nicklist_root;
    void *pointers[2];
    const char *name, *short_name, *type, *title, *modes, *input_prompt, *input;
    const char *ptr_key, *ptr_command;
    char *full_name, str_number[64], str_local_var[1024], *property;
    long long id;
    int number, nicklist, nicklist_case_sensitive, nicklist_display_groups;
    int apply_props, input_position, input_multiline;

    if (!event->json)
        return WEECHAT_RC_OK;

    JSON_GET_NUM(event->json, id, -1);
    JSON_GET_STR(event->json, name);
    JSON_GET_STR(event->json, short_name);
    JSON_GET_NUM(event->json, number, -1);
    JSON_GET_STR(event->json, type);
    JSON_GET_STR(event->json, title);
    JSON_GET_STR(event->json, modes);
    JSON_GET_STR(event->json, input_prompt);
    JSON_GET_STR(event->json, input);
    JSON_GET_NUM(event->json, input_position, 0);
    JSON_GET_BOOL(event->json, input_multiline);
    JSON_GET_BOOL(event->json, nicklist);
    JSON_GET_BOOL(event->json, nicklist_case_sensitive);
    JSON_GET_BOOL(event->json, nicklist_display_groups);

    buffer_props = weechat_hashtable_new (32,
                                          WEECHAT_HASHTABLE_STRING,
                                          WEECHAT_HASHTABLE_STRING,
                                          NULL,
                                          NULL);
    if (!buffer_props)
        return WEECHAT_RC_ERROR;

    /* buffer base properties */
    weechat_hashtable_set (buffer_props, "type", type);
    weechat_hashtable_set (buffer_props, "short_name", short_name);
    weechat_hashtable_set (buffer_props, "title", title);
    weechat_hashtable_set (buffer_props, "modes", modes);
    weechat_hashtable_set (buffer_props, "input_prompt", input_prompt);
    if (!event->buffer)
    {
        /*
         * set input content and position only when the buffer is created;
         * subsequent updates will be handled via the "input" callback
         */
        weechat_hashtable_set (buffer_props, "input", input);
        snprintf (str_number, sizeof (str_number), "%d", input_position);
        weechat_hashtable_set (buffer_props, "input_pos", str_number);
    }
    weechat_hashtable_set (buffer_props, "input_multiline",
                           (input_multiline) ? "1" : "0");
    weechat_hashtable_set (buffer_props, "nicklist", (nicklist) ? "1" : "0");
    weechat_hashtable_set (buffer_props, "nicklist_case_sensitive",
                           (nicklist_case_sensitive) ? "1" : "0");
    weechat_hashtable_set (buffer_props, "nicklist_display_groups",
                           (nicklist_display_groups) ? "1" : "0");

    /* extra properties for relay */
    weechat_hashtable_set (buffer_props,
                           "localvar_set_relay_remote", event->remote->name);
    snprintf (str_number, sizeof (str_number), "%lld", id);
    weechat_hashtable_set (buffer_props,
                           "localvar_set_relay_remote_id", str_number);
    snprintf (str_number, sizeof (str_number), "%d", number);
    weechat_hashtable_set (buffer_props,
                           "localvar_set_relay_remote_number", str_number);
    weechat_hashtable_set (buffer_props, "input_get_any_user_data", "1");

    /* if buffer exists, set properties, otherwise create buffer */
    apply_props = 1;
    ptr_buffer = event->buffer;
    if (!ptr_buffer)
    {
        if (weechat_asprintf (&full_name, "remote.%s.%s", event->remote->name, name) >= 0)
        {
            ptr_buffer = weechat_buffer_search ("relay", full_name);
            if (!ptr_buffer)
            {
                ptr_buffer = weechat_buffer_new_props (
                    full_name, buffer_props,
                    &relay_remote_event_buffer_input_cb, event->remote, NULL,
                    NULL, NULL, NULL);
                apply_props = 0;
            }
            free (full_name);
        }
    }

    if (!ptr_buffer)
        goto end;

    if (apply_props)
    {
        weechat_hashtable_map (buffer_props,
                               &relay_remote_event_apply_props, ptr_buffer);
    }

    json_vars = cJSON_GetObjectItem (event->json, "local_variables");
    if (json_vars && cJSON_IsObject (json_vars))
    {
        if (weechat_strcmp (event->name, "buffer_localvar_removed") == 0)
        {
            /*
             * we don't know which variables have been removed, so we remove any
             * local variable in the buffer that is not defined in the JSON
             * object "local_variables" received
             */
            pointers[0] = ptr_buffer;
            pointers[1] = json_vars;
            local_variables = weechat_hdata_pointer (relay_hdata_buffer,
                                                     ptr_buffer,
                                                     "local_variables");
            if (local_variables)
            {
                weechat_hashtable_map (local_variables,
                                       &relay_remote_event_remove_localvar_cb,
                                       pointers);
            }
        }
        else
        {
            /* add/update local variables */
            cJSON_ArrayForEach (json_var, json_vars)
            {
                if (json_var->string
                    && cJSON_IsString (json_var)
                    && relay_remote_event_check_local_var (json_var->string))
                {
                    snprintf (str_local_var, sizeof (str_local_var),
                              "localvar_set_%s",
                              json_var->string);
                    weechat_buffer_set (ptr_buffer,
                                        str_local_var,
                                        cJSON_GetStringValue (json_var));
                }
            }
        }
    }

    /* add keys */
    json_keys = cJSON_GetObjectItem (event->json, "keys");
    if (json_keys && cJSON_IsArray (json_keys))
    {
        cJSON_ArrayForEach (json_key, json_keys)
        {
            json_key_name = cJSON_GetObjectItem (json_key, "key");
            json_key_command = cJSON_GetObjectItem (json_key, "command");
            if (json_key_name && cJSON_IsString (json_key_name)
                && json_key_command && cJSON_IsString (json_key_command))
            {
                ptr_key = cJSON_GetStringValue (json_key_name);
                ptr_command = cJSON_GetStringValue (json_key_command);
                if (ptr_key && ptr_command)
                {
                    if (weechat_asprintf (&property, "key_bind_%s", ptr_key) >= 0)
                    {
                        weechat_buffer_set (ptr_buffer, property, ptr_command);
                        free (property);
                    }
                }
            }
        }
    }

    /* add lines */
    json_lines = cJSON_GetObjectItem (event->json, "lines");
    if (json_lines && cJSON_IsArray (json_lines))
    {
        event_line.remote = event->remote;
        event_line.buffer = ptr_buffer;
        cJSON_ArrayForEach (json_line, json_lines)
        {
            event_line.json = json_line;
            relay_remote_event_cb_line (&event_line);
        }
    }

    /* add nicklist groups and nicks */
    json_nicklist_root = cJSON_GetObjectItem (event->json, "nicklist_root");
    if (json_nicklist_root && cJSON_IsObject (json_nicklist_root))
        relay_remote_event_handle_nick_group (ptr_buffer, json_nicklist_root);

end:
    weechat_hashtable_free (buffer_props);

    return WEECHAT_RC_OK;
}

/*
 * Callback for event "buffer_cleared".
 */

RELAY_REMOTE_EVENT_CALLBACK(buffer_cleared)
{
    weechat_buffer_clear (event->buffer);
    return WEECHAT_RC_OK;
}

/*
 * Callback for event "buffer_closed".
 */

RELAY_REMOTE_EVENT_CALLBACK(buffer_closed)
{
    weechat_buffer_close (event->buffer);
    return WEECHAT_RC_OK;
}

/*
 * Callback for an input event.
 */

RELAY_REMOTE_EVENT_CALLBACK(input)
{
    cJSON *json_obj;
    const char *input_prompt, *input;
    char str_pos[64];
    int input_position;

    if (!event->buffer || !event->json)
        return WEECHAT_RC_OK;

    JSON_GET_STR(event->json, input_prompt);
    JSON_GET_STR(event->json, input);
    JSON_GET_NUM(event->json, input_position, 0);

    weechat_buffer_set (event->buffer, "input_prompt", input_prompt);
    weechat_buffer_set (event->buffer, "input", input);
    snprintf (str_pos, sizeof (str_pos), "%d", input_position);
    weechat_buffer_set (event->buffer, "input_pos", str_pos);

    return WEECHAT_RC_OK;
}

/*
 * Callback for response to GET /api/version.
 */

RELAY_REMOTE_EVENT_CALLBACK(version)
{
    cJSON *json_obj;
    const char *weechat_version, *weechat_version_git, *relay_api_version;
    char *weechat_version_local, request[1024];

    if (!event->json)
        return WEECHAT_RC_OK;

    JSON_GET_STR(event->json, weechat_version);
    JSON_GET_STR(event->json, weechat_version_git);
    JSON_GET_STR(event->json, relay_api_version);

    weechat_printf (NULL,
                    _("remote[%s]: WeeChat: %s (%s), API: %s"),
                    event->remote->name,
                    weechat_version,
                    weechat_version_git,
                    relay_api_version);

    if (!event->remote->version_ok)
    {
        /* check version: the remote API must be exactly the same as local API */
        if (weechat_strcmp (relay_api_version, RELAY_API_VERSION_STR) != 0)
        {
                weechat_version_local = weechat_info_get ("version", NULL);
                weechat_printf (
                    NULL,
                    _("%sremote[%s]: API version mismatch: "
                      "remote API is %s (WeeChat %s), "
                      "local API %s (WeeChat %s)"),
                    weechat_prefix ("error"),
                    event->remote->name,
                    relay_api_version,
                    weechat_version,
                    RELAY_API_VERSION_STR,
                    weechat_version_local);
                free (weechat_version_local);
                relay_remote_network_disconnect (event->remote);
                return WEECHAT_RC_OK;
        }

        event->remote->version_ok = 1;
        snprintf (request, sizeof (request),
                  "{\"request\": \"GET /api/buffers?"
                  "lines=-%d"
                  "&nicks=true"
                  "&colors=weechat"
                  "\"}",
                  weechat_config_integer (relay_config_api_remote_get_lines));
        relay_remote_network_send (event->remote, RELAY_MSG_STANDARD,
                                   request, strlen (request));
    }

    return WEECHAT_RC_OK;
}

/*
 * Synchronizes with remote.
 */

void
relay_remote_event_sync_with_remote (struct t_relay_remote *remote)
{
    cJSON *json, *json_body;

    if (!remote)
        return;

    json = cJSON_CreateObject ();
    if (!json)
        goto end;

    cJSON_AddItemToObject (json, "request",
                           cJSON_CreateString ("POST /api/sync"));
    json_body = cJSON_CreateObject ();
    if (!json_body)
        goto end;

    cJSON_AddItemToObject (json_body, "colors",
                           cJSON_CreateString ("weechat"));
    cJSON_AddItemToObject (json, "body", json_body);

    relay_remote_network_send_json (remote, json);

    remote->synced = 1;

end:
    cJSON_Delete (json);
}

/*
 * Reads an event from a remote.
 */

void
relay_remote_event_recv (struct t_relay_remote *remote, const char *data)
{
    cJSON *json, *json_body, *json_event, *json_obj;
    const char *body_type, *name;
    long long buffer_id;
    int i, rc, code;
    struct t_relay_remote_event_cb event_cb[] = {
        /* event (mask), callback (NULL = event ignored) */
        /* note: order is important */
        { "buffer_line_*", &relay_remote_event_cb_line },
        { "buffer_closing", NULL },
        { "buffer_cleared", &relay_remote_event_cb_buffer_cleared },
        { "buffer_closed", &relay_remote_event_cb_buffer_closed },
        { "buffer_*", &relay_remote_event_cb_buffer },
        { "input_*", &relay_remote_event_cb_input },
        { "nicklist_group_*", &relay_remote_event_cb_nick_group },
        { "nicklist_nick_*", &relay_remote_event_cb_nick },
        { NULL, NULL },
    };
    t_relay_remote_event_func *callback;
    struct t_relay_remote_event event;

    if (!remote || !data)
        return;

    /* display debug message */
    if (weechat_relay_plugin->debug >= 2)
    {
        weechat_printf (NULL,
                        "%s: recv from remote %s: \"%s\"",
                        RELAY_PLUGIN_NAME, remote->name, data);
    }

    json = cJSON_Parse (data);
    if (!json)
        goto error_data;

    event.remote = remote;
    event.name = NULL;
    event.buffer = NULL;
    event.json = NULL;

    JSON_GET_NUM(json, code, -1);
    JSON_GET_STR(json, body_type);
    json_event = cJSON_GetObjectItem (json, "event");
    json_body = cJSON_GetObjectItem (json, "body");

    if (!body_type && ((code == 200) || (code == 204)))
        return;

    if (json_event && cJSON_IsObject (json_event))
    {
        JSON_GET_STR(json_event, name);
        event.name = name;
        JSON_GET_NUM(json_event, buffer_id, -1);
        event.buffer = relay_remote_event_search_buffer (remote, buffer_id);
    }

    callback = NULL;

    if (code == 200)
    {
        if (weechat_strcmp (body_type, "buffer") == 0)
            callback = &relay_remote_event_cb_buffer;
        else if (weechat_strcmp (body_type, "version") == 0)
            callback = &relay_remote_event_cb_version;
    }
    else if (event.name)
    {
        for (i = 0; event_cb[i].event_mask; i++)
        {
            if (weechat_string_match (event.name, event_cb[i].event_mask, 1))
            {
                callback = event_cb[i].func;
                break;
            }
        }
    }

    if (callback)
    {
        rc = WEECHAT_RC_OK;
        if (cJSON_IsArray (json_body))
        {
            cJSON_ArrayForEach (json_obj, json_body)
            {
                event.json = json_obj;
                rc = (callback) (&event);
            }
        }
        else
        {
            event.json = json_body;
            rc = (callback) (&event);
        }
        if (rc == WEECHAT_RC_ERROR)
            goto error_cb;
    }

    if (!remote->synced
        && (code == 200)
        && (weechat_strcmp (body_type, "buffer") == 0))
    {
        relay_remote_event_sync_with_remote (remote);
    }

    return;

error_data:
    weechat_printf (NULL,
                    "%sremote[%s]: invalid data received from remote: \"%s\"",
                    weechat_prefix ("error"),
                    remote->name,
                    data);
    return;

error_cb:
    weechat_printf (NULL,
                    "%sremote[%s]: callback failed for body type \"%s\"",
                    weechat_prefix ("error"),
                    remote->name,
                    body_type);
    return;
}
