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
 * Callback for body type "line".
 */

RELAY_REMOTE_EVENT_CALLBACK(line)
{
    cJSON *json_obj, *json_tags, *json_tag;
    const char *date, *prefix, *message;
    char **tags;
    struct timeval tv_date;

    if (!event->buffer)
        return WEECHAT_RC_ERROR;

    JSON_GET_STR(event->json, date);
    JSON_GET_STR(event->json, prefix);
    JSON_GET_STR(event->json, message);

    if (!weechat_util_parse_time (date, &tv_date))
    {
        tv_date.tv_sec = 0;
        tv_date.tv_usec = 0;
    }

    tags = weechat_string_dyn_alloc (256);
    if (tags)
    {
        json_tags = cJSON_GetObjectItem (event->json, "tags");
        if (json_tags && cJSON_IsArray (json_tags))
        {
            cJSON_ArrayForEach (json_tag, json_tags)
            {
                if (*tags[0])
                    weechat_string_dyn_concat (tags, ",", -1);
                weechat_string_dyn_concat (
                    tags, cJSON_GetStringValue (json_tag), -1);
            }
        }
    }

    weechat_printf_datetime_tags (
        event->buffer,
        tv_date.tv_sec,
        tv_date.tv_usec,
        (tags) ? *tags : NULL,
        "%s%s%s",
        (prefix && prefix[0]) ? prefix : "",
        (prefix && prefix[0]) ? "\t" : "",
        message);

    if (tags)
        weechat_string_dyn_free (tags, 1);

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
 * Callback for body type "buffer".
 */

RELAY_REMOTE_EVENT_CALLBACK(buffer)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_hashtable *buffer_props;
    struct t_relay_remote_event event_line;
    cJSON *json_obj, *json_lines, *json_line;
    const char *name, *short_name, *type, *title;
    char *full_name, str_number[64];
    long long id;
    int number;

    JSON_GET_NUM(event->json, id, -1);
    JSON_GET_STR(event->json, name);
    JSON_GET_STR(event->json, short_name);
    JSON_GET_NUM(event->json, number, -1);
    JSON_GET_STR(event->json, type);
    JSON_GET_STR(event->json, title);

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
    ptr_buffer = relay_remote_event_search_buffer (event->remote, id);
    if (ptr_buffer)
    {
        weechat_hashtable_map (buffer_props,
                               &relay_remote_event_apply_props, ptr_buffer);
    }
    else
    {
        if (weechat_asprintf (&full_name, "remote.%s.%s", event->remote->name, name) >= 0)
        {
            ptr_buffer = weechat_buffer_new_props (
                full_name, buffer_props,
                &relay_remote_event_buffer_input_cb, event->remote, NULL,
                NULL, NULL, NULL);
            free (full_name);
        }
    }

    if (ptr_buffer)
    {
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
    }

    weechat_hashtable_free (buffer_props);

    return WEECHAT_RC_OK;
}

/*
 * Callback for body type "version".
 */

RELAY_REMOTE_EVENT_CALLBACK(version)
{
    cJSON *json_obj;
    const char *weechat_version, *weechat_version_git, *relay_api_version;

    JSON_GET_STR(event->json, weechat_version);
    JSON_GET_STR(event->json, weechat_version_git);
    JSON_GET_STR(event->json, relay_api_version);

    weechat_printf (NULL,
                    _("remote[%s]: WeeChat: %s (%s), API: %s"),
                    event->remote->name,
                    weechat_version,
                    weechat_version_git,
                    relay_api_version);

    return WEECHAT_RC_OK;
}

/*
 * Synchronizes with remote.
 */

void
relay_remote_event_sync_with_remote (struct t_relay_remote *remote)
{
    cJSON *json, *json_body;

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
    const char *body_type;
    long long buffer_id;
    int i, rc, code;
    struct t_relay_remote_event_cb event_cb[] = {
        /* body_type, callback */
        { "buffer", &relay_remote_event_cb_buffer },
        { "line", &relay_remote_event_cb_line },
        { "version", &relay_remote_event_cb_version },
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
    event.buffer = NULL;
    event.buffer = NULL;

    JSON_GET_NUM(json, code, -1);
    JSON_GET_STR(json, body_type);
    json_event = cJSON_GetObjectItem (json, "event");
    json_body = cJSON_GetObjectItem (json, "body");

    if (!body_type)
    {
        if ((code == 200) || (code == 204))
            return;
        goto error_data;
    }

    if (json_event && cJSON_IsObject (json_event))
    {
        JSON_GET_NUM(json_event, buffer_id, -1);
        event.buffer = relay_remote_event_search_buffer (remote, buffer_id);
    }

    callback = NULL;
    for (i = 0; event_cb[i].body_type; i++)
    {
        if (strcmp (event_cb[i].body_type, body_type) == 0)
        {
            callback = event_cb[i].func;
            break;
        }
    }
    if (!callback)
        return;

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

    if (!remote->synced && (code == 200) && (strcmp (body_type, "buffer") == 0))
        relay_remote_event_sync_with_remote (remote);

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
                    body_type,
                    data);
    return;
}
