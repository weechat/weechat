/*
 * relay-http.c - HTTP request parser for relay plugin
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
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <zlib.h>
#ifdef HAVE_ZSTD
#include <zstd.h>
#endif

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-auth.h"
#include "relay-client.h"
#include "relay-config.h"
#include "relay-http.h"
#include "relay-raw.h"
#include "relay-websocket.h"
#ifdef HAVE_CJSON
#include "api/relay-api.h"
#endif

#define HEX2DEC(c) (((c >= 'a') && (c <= 'f')) ? c - 'a' + 10 :         \
                    ((c >= 'A') && (c <= 'F')) ? c - 'A' + 10 :         \
                    c - '0')


/*
 * Reinitializes the HTTP request status.
 */

void
relay_http_request_reinit (struct t_relay_http_request *request)
{
    request->status = RELAY_HTTP_METHOD;
    weechat_string_dyn_copy (request->raw, NULL);
    if (request->method)
    {
        free (request->method);
        request->method = NULL;
    }
    if (request->path)
    {
        free (request->path);
        request->path = NULL;
    }
    if (request->path_items)
    {
        weechat_string_free_split (request->path_items);
        request->path_items = NULL;
    }
    request->num_path_items = 0;
    weechat_hashtable_remove_all (request->params);
    if (request->http_version)
    {
        free (request->http_version);
        request->http_version = NULL;
    }
    weechat_hashtable_remove_all (request->headers);
    weechat_hashtable_remove_all (request->accept_encoding);
    if (request->ws_deflate)
    {
        relay_websocket_deflate_free (request->ws_deflate);
        request->ws_deflate = relay_websocket_deflate_alloc ();
    }
    request->content_length = 0;
    request->body_size = 0;
    if (request->body)
    {
        free (request->body);
        request->body = NULL;
    }
}

/*
 * Allocates a t_relay_http_request structure.
 */

struct t_relay_http_request *
relay_http_request_alloc ()
{
    struct t_relay_http_request *new_request;

    new_request = (struct t_relay_http_request *)malloc (sizeof (*new_request));
    if (!new_request)
        return NULL;

    new_request->status = RELAY_HTTP_METHOD;
    new_request->raw = weechat_string_dyn_alloc (64);
    new_request->method = NULL;
    new_request->path = NULL;
    new_request->path_items = NULL;
    new_request->num_path_items = 0;
    new_request->params = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    new_request->http_version = NULL;
    new_request->headers = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    new_request->accept_encoding = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    new_request->ws_deflate = relay_websocket_deflate_alloc ();
    new_request->content_length = 0;
    new_request->body_size = 0;
    new_request->body = NULL;

    return new_request;
}

/*
 * Decode URL: replace "%" chars by their values (eg: "%23" -> "#").
 */

char *
relay_http_url_decode (const char *url)
{
    const char *ptr_url, *ptr_next;
    char **out, str_char[2];
    int length;

    if (!url)
        return NULL;

    length = strlen (url);
    out = weechat_string_dyn_alloc ((length > 0) ? length : 1);
    if (!out)
        return NULL;

    ptr_url = url;
    while (ptr_url && ptr_url[0])
    {
        if ((ptr_url[0] == '%')
            && (isxdigit ((unsigned char)ptr_url[1]))
            && (isxdigit ((unsigned char)ptr_url[2])))
        {
            snprintf (str_char, sizeof (str_char),
                      "%c",
                      (HEX2DEC(ptr_url[1]) << 4) + HEX2DEC(ptr_url[2]));
            weechat_string_dyn_concat (out, str_char, -1);
            ptr_url += 3;
        }
        else
        {
            ptr_next = weechat_utf8_next_char (ptr_url);
            weechat_string_dyn_concat (
                out,
                ptr_url,
                (ptr_next) ? ptr_next - ptr_url : -1);
            ptr_url = ptr_next;
        }
    }

    return weechat_string_dyn_free (out, 0);
}

/*
 * Returns value of an URL parameter as boolean (0 or 1), using a default value
 * if the parameter is not set.
 */

int
relay_http_get_param_boolean (struct t_relay_http_request *request,
                              const char *name, int default_value)
{
    const char *ptr_value;

    ptr_value = weechat_hashtable_get (request->params, name);
    if (!ptr_value)
        return default_value;

    return weechat_config_string_to_boolean (ptr_value);
}

/*
 * Returns value of an URL parameter as long, using a default value if the
 * parameter is not set or if it's not a valid long integer.
 */

long
relay_http_get_param_long (struct t_relay_http_request *request,
                           const char *name, long default_value)
{
    const char *ptr_value;
    char *error;
    long number;

    ptr_value = weechat_hashtable_get (request->params, name);
    if (!ptr_value)
        return default_value;

    number = strtol (ptr_value, &error, 10);
    if (error && !error[0])
        return number;

    return default_value;
}

/*
 * Get decoded path items from path.
 */

void
relay_http_parse_path (const char *path,
                       char ***paths, int *num_paths,
                       struct t_hashtable *params)
{
    char *pos, *str_path, *str_params, **items_path, **items2_path;
    char **items_params, *name, *value;
    int i, num_items_path, num_items_params;

    *paths = NULL;
    *num_paths = 0;

    if (!path)
        return;

    str_path = NULL;
    str_params = NULL;
    items_path = NULL;
    items2_path = NULL;

    pos = strchr (path, '?');
    if (pos)
    {
        str_path = weechat_strndup (path, pos - path);
        str_params = strdup (pos + 1);
    }
    else
    {
        str_path = strdup (path);
    }

    /*
     * decode path items (until '?' or end of string):
     *   "/path/to/irc.libera.%23weechat"
     *   => ["path", "to", "irc.libera.#weechat"]
     */
    items_path = weechat_string_split (
        (str_path[0] == '/') ? str_path + 1 : str_path,
        "/",
        NULL,
        WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
        0, &num_items_path);
    if (items_path && (num_items_path > 0))
    {
        items2_path = malloc (sizeof (*items_path) * (num_items_path + 1));
        if (items2_path)
        {
            for (i = 0; i < num_items_path; i++)
            {
                items2_path[i] = relay_http_url_decode (items_path[i]);
            }
            items2_path[num_items_path] = NULL;
        }
        *paths = items2_path;
        *num_paths = num_items_path;
    }

    /*
     * decode parameters (starting after '?'):
     *   "/path/to/irc.libera.%23weechat?option=2&bool=off&fields=a,b,c"
     *   => {"option": "2", "bool": "off", "fields": "a,b,c"}
     */
    if (str_params)
    {
        items_params = weechat_string_split (str_params, "&", NULL, 0, 0,
                                             &num_items_params);
        if (items_params && (num_items_params > 0))
        {
            for (i = 0; i < num_items_params; i++)
            {
                pos = strchr (items_params[i], '=');
                if (pos)
                {
                    name = weechat_strndup (items_params[i], pos - items_params[i]);
                    value = relay_http_url_decode (pos + 1);
                }
                else
                {
                    name = strdup (items_params[i]);
                    value = strdup ("");
                }
                if (params)
                    weechat_hashtable_set (params, name, value);
                free (name);
                free (value);
            }
        }
    }

    free (str_path);
    free (str_params);
    weechat_string_free_split (items_path);
}

/*
 * Parses and saves method and path.
 *
 * Returns:
 *   1: OK, method and path saved
 *   0: error: invalid format
 */

int
relay_http_parse_method_path (struct t_relay_http_request *request,
                              const char *method_path)
{
    char **items;
    int num_items;

    if (!request || !method_path || !method_path[0])
        return 0;

    weechat_string_dyn_concat (request->raw, method_path, -1);
    weechat_string_dyn_concat (request->raw, "\n", -1);

    items = weechat_string_split (method_path, " ", NULL,
                                  WEECHAT_STRING_SPLIT_STRIP_LEFT
                                  | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                  | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                  0, &num_items);
    if (!items || (num_items < 2))
        goto error;

    free (request->method);
    request->method = strdup (items[0]);

    free (request->path);
    request->path = strdup (items[1]);

    if (num_items > 2)
    {
        free (request->http_version);
        request->http_version = strdup (items[2]);
    }

    relay_http_parse_path (request->path,
                           &(request->path_items),
                           &(request->num_path_items),
                           request->params);

    request->status = RELAY_HTTP_HEADERS;

    weechat_string_free_split (items);

    return 1;

error:
    weechat_string_free_split (items);
    request->status = RELAY_HTTP_END;
    return 0;
}

/*
 * Parses and saves a HTTP header in hashtable "headers".
 *
 * Returns:
 *   1: OK, header saved
 *   0: error: invalid format
 */

int
relay_http_parse_header (struct t_relay_http_request *request,
                         const char *header)
{
    char *pos, *name, *name_lower, *error, **items;
    const char *ptr_value;
    int i, num_items;
    long number;

    weechat_string_dyn_concat (request->raw, header, -1);
    weechat_string_dyn_concat (request->raw, "\n", -1);

    /* empty line => end of headers */
    if (!header || !header[0])
    {
        request->status = (request->content_length > 0) ?
            RELAY_HTTP_BODY : RELAY_HTTP_END;
        return 1;
    }

    pos = strchr (header, ':');

    /* not a valid header */
    if (!pos || (pos == header))
        return 0;

    /* get header name, which is case-insensitive */
    name = weechat_strndup (header, pos - header);
    if (!name)
        return 0;
    name_lower = weechat_string_tolower (name);
    if (!name_lower)
    {
        free (name);
        return 0;
    }

    /* get pointer on header value */
    ptr_value = pos + 1;
    while (ptr_value[0] == ' ')
    {
        ptr_value++;
    }

    /* add header in the hashtable */
    weechat_hashtable_set (request->headers, name_lower, ptr_value);

    /* if header is "Accept-Encoding", save the allowed encoding */
    if (strcmp (name_lower, "accept-encoding") == 0)
    {
        items = weechat_string_split (ptr_value, ",", " ", 0, 0, &num_items);
        if (items)
        {
            for (i = 0; i < num_items; i++)
            {
                weechat_hashtable_set (request->accept_encoding, items[i], NULL);
            }
            weechat_string_free_split (items);
        }
    }

    /* if header is "Content-Length", save the length */
    if (strcmp (name_lower, "content-length") == 0)
    {
        error = NULL;
        number = strtol (ptr_value, &error, 10);
        if (error && !error[0])
            request->content_length = (int)number;
    }

    /*
     * if header is "Sec-WebSocket-Extensions", save supported websocket
     * extensions
     */
    if (strcmp (name_lower, "sec-websocket-extensions") == 0)
        relay_websocket_parse_extensions (ptr_value, request->ws_deflate);

    free (name);
    free (name_lower);

    return 1;
}

/*
 * Adds bytes to HTTP body, changes the status to RELAY_HTTP_END if the body
 * is complete.
 */

void
relay_http_add_to_body (struct t_relay_http_request *request,
                        char **partial_message)
{
    char *new_body, *new_partial;
    int num_bytes_missing, length_msg, length;

    if (!partial_message || !*partial_message)
        return;

    num_bytes_missing = request->content_length
        - request->body_size;
    if (num_bytes_missing <= 0)
    {
        request->status = RELAY_HTTP_END;
        return;
    }

    length_msg = strlen (*partial_message);
    if (num_bytes_missing >= length_msg)
    {
        new_body = realloc (
            request->body,
            request->body_size + length_msg + 1);
        if (new_body)
        {
            request->body = new_body;
            memcpy (request->body + request->body_size,
                    *partial_message,
                    length_msg);
            request->body[request->body_size + length_msg] = '\0';
            request->body_size += length_msg;
            weechat_string_dyn_concat (request->raw, *partial_message, -1);
        }
        free (*partial_message);
        *partial_message = NULL;
    }
    else
    {
        new_body = realloc (
            request->body,
            request->body_size + num_bytes_missing + 1);
        if (new_body)
        {
            request->body = new_body;
            memcpy (request->body + request->body_size,
                    *partial_message,
                    num_bytes_missing);
            request->body[request->body_size + num_bytes_missing] = '\0';
            request->body_size += num_bytes_missing;
            weechat_string_dyn_concat (request->raw,
                                       *partial_message, num_bytes_missing);
            length = strlen (*partial_message + num_bytes_missing);
            new_partial = malloc (length + 1);
            if (new_partial)
            {
                memcpy (new_partial,
                        *partial_message + num_bytes_missing,
                        length + 1);
                free (*partial_message);
                *partial_message = new_partial;
            }
        }
    }

    if (request->body_size >= request->content_length)
        request->status = RELAY_HTTP_END;
}

/*
 * Gets authentication status according to headers in the request.
 *
 * Returns:
 *    0: authentication OK (password + TOTP if enabled)
 *   -1: missing password
 *   -2: invalid password
 *   -3: missing TOTP
 *   -4: invalid TOTP
 *   -5: invalid hash algorithm
 *   -6: invalid timestamp (used as salt)
 *   -7: invalid number of iterations (PBKDF2)
 *   -8: out of memory
 */

int
relay_http_get_auth_status (struct t_relay_client *client)
{
    const char *auth, *client_totp, *pos;
    char *relay_password, *totp_secret, *info_totp_args, *info_totp;
    char *user_pass;
    int rc, length, totp_ok;

    rc = 0;
    relay_password = NULL;
    totp_secret = NULL;
    user_pass = NULL;

    relay_password = weechat_string_eval_expression (
        weechat_config_string (relay_config_network_password),
        NULL, NULL, NULL);
    if (!relay_password)
    {
        rc = -8;
        goto end;
    }

    auth = weechat_hashtable_get (client->http_req->headers, "authorization");
    if (!auth || (weechat_strncasecmp (auth, "basic ", 6) != 0))
    {
        rc = -1;
        goto end;
    }

    pos = auth + 6;
    while (pos[0] == ' ')
    {
        pos++;
    }

    length = strlen (pos);
    user_pass = malloc (length + 1);
    if (!user_pass)
    {
        rc = -8;
        goto end;
    }
    length = weechat_string_base_decode ("64", pos, user_pass);
    if (length < 0)
    {
        rc = -2;
        goto end;
    }
    if (strncmp (user_pass, "plain:", 6) == 0)
    {
        switch (relay_auth_check_password_plain (client, user_pass + 6, relay_password))
        {
            case 0: /* password OK */
                break;
            case -1: /* "plain" is not allowed */
                rc = -5;
                goto end;
            case -2: /* invalid password */
            default:
                rc = -2;
                goto end;
        }
    }
    else if (strncmp (user_pass, "hash:", 5) == 0)
    {
        switch (relay_auth_password_hash (client, user_pass + 5, relay_password))
        {
            case 0: /* password OK */
                break;
            case -1: /* invalid hash algorithm */
                rc = -5;
                goto end;
            case -2: /* invalid timestamp */
                rc = -6;
                goto end;
            case -3: /* invalid iterations */
                rc = -7;
                goto end;
            case -4: /* invalid password */
            default:
                rc = -2;
                goto end;
        }
    }
    else
    {
        rc = -2;
        goto end;
    }

    totp_secret = weechat_string_eval_expression (
        weechat_config_string (relay_config_network_totp_secret),
        NULL, NULL, NULL);
    if (totp_secret && totp_secret[0])
    {
        client_totp = weechat_hashtable_get (client->http_req->headers, "x-weechat-totp");
        if (!client_totp || !client_totp[0])
        {
            rc = -3;
            goto end;
        }
        length = strlen (totp_secret) + strlen (client_totp) + 16 + 1;
        info_totp_args = malloc (length);
        if (info_totp_args)
        {
            /* validate the TOTP received from the client */
            snprintf (info_totp_args, length,
                      "%s,%s,0,%d",
                      totp_secret,  /* the shared secret */
                      client_totp,  /* the TOTP from client */
                      weechat_config_integer (relay_config_network_totp_window));
            info_totp = weechat_info_get ("totp_validate", info_totp_args);
            totp_ok = (info_totp && (strcmp (info_totp, "1") == 0)) ?
                1 : 0;
            free (info_totp);
            free (info_totp_args);
            if (!totp_ok)
            {
                rc = -4;
                goto end;
            }
        }
    }

end:
    free (relay_password);
    free (totp_secret);
    free (user_pass);
    return rc;
}

/*
 * Checks authentication in HTTP request.
 *
 * Returns:
 *   1: authentication OK
 *   0: authentication failed
 */

int
relay_http_check_auth (struct t_relay_client *client)
{
    int rc;

    rc = relay_http_get_auth_status (client);
    switch (rc)
    {
        case 0: /* authentication OK */
            break;
        case -1: /* missing password */
            relay_http_send_error_json (client, RELAY_HTTP_401_UNAUTHORIZED,
                                        NULL,
                                        RELAY_HTTP_ERROR_MISSING_PASSWORD);
            break;
        case -2: /* invalid password */
            relay_http_send_error_json (client, RELAY_HTTP_401_UNAUTHORIZED,
                                        NULL,
                                        RELAY_HTTP_ERROR_INVALID_PASSWORD);
            break;
        case -3: /* missing TOTP */
            relay_http_send_error_json (client, RELAY_HTTP_401_UNAUTHORIZED,
                                        NULL,
                                        RELAY_HTTP_ERROR_MISSING_TOTP);
            break;
        case -4: /* invalid TOTP */
            relay_http_send_error_json (client, RELAY_HTTP_401_UNAUTHORIZED,
                                        NULL,
                                        RELAY_HTTP_ERROR_INVALID_TOTP);
            break;
        case -5: /* invalid hash algorithm */
            relay_http_send_error_json (client, RELAY_HTTP_401_UNAUTHORIZED,
                                        NULL,
                                        RELAY_HTTP_ERROR_INVALID_HASH_ALGO);
            break;
        case -6: /* invalid timestamp */
            relay_http_send_error_json (client, RELAY_HTTP_401_UNAUTHORIZED,
                                        NULL,
                                        RELAY_HTTP_ERROR_INVALID_TIMESTAMP);
            break;
        case -7: /* invalid iterations */
            relay_http_send_error_json (client, RELAY_HTTP_401_UNAUTHORIZED,
                                        NULL,
                                        RELAY_HTTP_ERROR_INVALID_ITERATIONS);
            break;
        case -8: /* out of memory */
            relay_http_send_error_json (client, RELAY_HTTP_401_UNAUTHORIZED,
                                        NULL,
                                        RELAY_HTTP_ERROR_OUT_OF_MEMORY);
            break;
    }
    return (rc == 0) ? 1 : 0;
}

/*
 * Processes HTTP websocket request.
 */

void
relay_http_process_websocket (struct t_relay_client *client)
{
    const char *ptr_real_ip;
    char *handshake;
    int rc;

    rc = relay_websocket_client_handshake_valid (client->http_req);

    if (rc == -1)
    {
        relay_http_send (client, RELAY_HTTP_400_BAD_REQUEST, NULL, NULL, 0);
        if (weechat_relay_plugin->debug >= 1)
        {
            weechat_printf_date_tags (
                NULL, 0, "relay_client",
                _("%s%s: invalid websocket handshake received for client %s%s%s"),
                weechat_prefix ("error"),
                RELAY_PLUGIN_NAME,
                RELAY_COLOR_CHAT_CLIENT,
                client->desc,
                RELAY_COLOR_CHAT);
        }
        relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
        return;
    }

    if (rc == -2)
    {
        relay_http_send (client, RELAY_HTTP_403_FORBIDDEN, NULL, NULL, 0);
        if (weechat_relay_plugin->debug >= 1)
        {
            weechat_printf_date_tags (
                NULL, 0, "relay_client",
                _("%s%s: origin \"%s\" is not allowed for websocket"),
                weechat_prefix ("error"),
                RELAY_PLUGIN_NAME,
                weechat_hashtable_get (client->http_req->headers, "origin"));
        }
        relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
        return;
    }

    /* handshake from client is valid, auth is mandatory for "api" protocol */
    if (client->protocol == RELAY_PROTOCOL_API)
    {
        if (relay_http_check_auth (client))
        {
            relay_client_set_status (client, RELAY_STATUS_CONNECTED);
        }
        else
        {
            relay_client_set_status (client, RELAY_STATUS_AUTH_FAILED);
            return;
        }
    }

    handshake = relay_websocket_build_handshake (client->http_req);
    if (handshake)
    {
        relay_client_send (client,
                           RELAY_MSG_STANDARD,
                           handshake,
                           strlen (handshake), NULL);
        free (handshake);
        client->websocket = RELAY_CLIENT_WEBSOCKET_READY;
        memcpy (client->ws_deflate, client->http_req->ws_deflate,
                sizeof (*(client->ws_deflate)));
        if (client->protocol == RELAY_PROTOCOL_API)
        {
            /* "api" protocol uses JSON in input/output (multi-line text) */
            client->recv_data_type = RELAY_CLIENT_DATA_TEXT_MULTILINE;
            client->send_data_type = RELAY_CLIENT_DATA_TEXT_MULTILINE;
        }
    }

    ptr_real_ip = weechat_hashtable_get (
        client->http_req->headers, "x-real-ip");
    if (ptr_real_ip)
    {
        free (client->real_ip);
        client->real_ip = strdup (ptr_real_ip);
        relay_client_set_desc (client);
        weechat_printf_date_tags (
            NULL, 0, "relay_client",
            _("%s: websocket client %s%s%s has real IP "
              "address \"%s\""),
            RELAY_PLUGIN_NAME,
            RELAY_COLOR_CHAT_CLIENT,
            client->desc,
            RELAY_COLOR_CHAT,
            ptr_real_ip);
    }
}

/*
 * Processes HTTP request.
 */

void
relay_http_process_request (struct t_relay_client *client)
{
    if (client->http_req->raw)
    {
        relay_raw_print_client (client, RELAY_MSG_STANDARD,
                                RELAY_RAW_FLAG_RECV,
                                *(client->http_req->raw),
                                strlen (*(client->http_req->raw)) + 1);
    }

    /* if websocket is initializing */
    if (client->websocket == RELAY_CLIENT_WEBSOCKET_INITIALIZING)
    {
        relay_http_process_websocket (client);
    }
    else
    {
#ifdef HAVE_CJSON
        if (client->protocol == RELAY_PROTOCOL_API)
            relay_api_recv_http (client);
#endif /* HAVE_CJSON */
    }
}

/*
 * Reads HTTP data from a client.
 */

void
relay_http_recv (struct t_relay_client *client, const char *data)
{
    char *new_partial, *pos;
    int length;

    if (client->partial_message)
    {
        new_partial = realloc (client->partial_message,
                               strlen (client->partial_message) +
                               strlen (data) + 1);
        if (!new_partial)
            return;
        client->partial_message = new_partial;
        strcat (client->partial_message, data);
    }
    else
    {
        client->partial_message = strdup (data);
    }

    while (client->partial_message)
    {
        if ((client->http_req->status == RELAY_HTTP_METHOD)
            || (client->http_req->status == RELAY_HTTP_HEADERS))
        {
            pos = strchr (client->partial_message, '\r');
            if (!pos)
                break;
            pos[0] = '\0';
            if (client->http_req->status == RELAY_HTTP_METHOD)
            {
                relay_http_parse_method_path (client->http_req,
                                              client->partial_message);
            }
            else
            {
                relay_http_parse_header (client->http_req,
                                         client->partial_message);
            }
            pos[0] = '\r';
            pos++;
            if (pos[0] == '\n')
                pos++;
            length = strlen (pos);
            if (length > 0)
            {
                new_partial = malloc (length + 1);
                if (new_partial)
                {
                    memcpy (new_partial, pos, length + 1);
                    free (client->partial_message);
                    client->partial_message = new_partial;
                }
            }
            else
            {
                free (client->partial_message);
                client->partial_message = NULL;
            }
        }
        else if (client->http_req->status == RELAY_HTTP_BODY)
        {
            relay_http_add_to_body (client->http_req, &(client->partial_message));
        }

        /* process the request if it's ready to be processed (all parsed) */
        if (client->http_req->status == RELAY_HTTP_END)
        {
            relay_http_process_request (client);
            relay_http_request_reinit (client->http_req);
        }

        /*
         * we continue to process HTTP requests only if websocket is
         * initializing or for "api" relay
         */
        if ((client->websocket != RELAY_CLIENT_WEBSOCKET_INITIALIZING)
            && (client->protocol != RELAY_PROTOCOL_API))
        {
            break;
        }
    }
}

/*
 * Compresses body of HTTP request with gzip or zstd, if all conditions are met:
 *   - body is not empty
 *   - gzip or ztsd is allowed by client (header "Accept-Encoding")
 *     (for zstd, WeeChat must be compiled with zstd support)
 *   - compression is enabled (option relay.network.compression > 0)
 *
 * "compressed_size" is set to the compressed size (0 if error).
 *
 * "http_content_encoding" is a pointer to a string that will be set with
 * the HTTP header "Content-Encoding", if the compression is successful
 * (for example: "Content-Encoding: gzip").
 *
 * Returns pointer to compressed data or NULL if error.
 *
 * Note: result must be freed after use.
 */

char *
relay_http_compress (struct t_relay_http_request *request,
                     const char *data, int data_size,
                     int *compressed_size,
                     char *http_content_encoding,
                     int http_content_encoding_size)
{
    int rc, compression, compression_level, comp_deflate, comp_gzip, comp_zstd;
    Bytef *dest;
    uLongf dest_size;
    z_stream strm;
#ifdef HAVE_ZSTD
    size_t zstd_comp_size;
#endif

    if (!request)
        return NULL;

    if (compressed_size)
        *compressed_size = 0;
    if (http_content_encoding)
        http_content_encoding[0] = '\0';

    if (!data || (data_size <= 0) || !compressed_size
        || !http_content_encoding || (http_content_encoding_size <= 0))
    {
        return NULL;
    }

    compression = weechat_config_integer (relay_config_network_compression);
    if (compression <= 0)
        return NULL;

    /*
     * compression used by priority if allowed:
     *   1. zstd
     *   2. deflate
     *   3. gzip
     */
    comp_deflate = weechat_hashtable_has_key (request->accept_encoding, "deflate");
    comp_gzip = weechat_hashtable_has_key (request->accept_encoding, "gzip");
#ifdef HAVE_ZSTD
    comp_zstd = weechat_hashtable_has_key (request->accept_encoding, "zstd");
#else
    comp_zstd = 0;
#endif /* HAVE_ZSTD */

    if (!comp_deflate && !comp_gzip && !comp_zstd)
        return NULL;

    if (comp_deflate)
        comp_gzip = 0;

    dest = NULL;
    dest_size = 0;

#ifdef HAVE_ZSTD
    /* compress with zstd */
    if (!dest && comp_zstd)
    {
        /* convert % to zstd compression level (1-19) */
        compression_level = (((compression - 1) * 19) / 100) + 1;
        dest_size = ZSTD_compressBound (data_size);
        dest = malloc (dest_size);
        if (dest)
        {
            zstd_comp_size = ZSTD_compress(
                dest,
                dest_size,
                (void *)data,
                data_size,
                compression_level);
            if (zstd_comp_size > 0)
            {
                *compressed_size = zstd_comp_size;
                strcat (http_content_encoding, "Content-Encoding: zstd\r\n");
            }
            else
            {
                free (dest);
                dest = NULL;
            }
        }
    }
#endif /* HAVE_ZSTD */

    /* compress with deflate (zlib) or gzip */
    if (!dest && (comp_deflate || comp_gzip))
    {
        /* convert % to zlib compression level (1-9) */
        compression_level = (((compression - 1) * 9) / 100) + 1;
        dest_size = compressBound (data_size);
        dest = malloc (dest_size);
        if (dest)
        {
            memset (&strm, 0, sizeof (strm));
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            strm.opaque = Z_NULL;
            strm.avail_in = (uInt)data_size;
            strm.next_in = (Bytef *)data;
            strm.avail_out = (uInt)dest_size;
            strm.next_out = (Bytef *)dest;
            rc = deflateInit2 (
                &strm,
                compression_level,
                Z_DEFLATED,                  /* method */
                (comp_gzip) ? 15 + 16 : 15,  /* + 16 = gzip instead of zlib */
                8,                           /* memLevel */
                Z_DEFAULT_STRATEGY);         /* strategy */
            if (rc == Z_OK)
            {
                rc = deflate (&strm, Z_FINISH);
                (void) deflateEnd (&strm);
                if ((rc == Z_STREAM_END) || (rc == Z_OK))
                {
                    *compressed_size = strm.total_out;
                    if (comp_deflate)
                        strcat (http_content_encoding, "Content-Encoding: deflate\r\n");
                    else if (comp_gzip)
                        strcat (http_content_encoding, "Content-Encoding: gzip\r\n");
                }
                else
                {
                    free (dest);
                    dest = NULL;
                }
            }
            else
            {
                free (dest);
                dest = NULL;
            }
        }
    }

    return (char *)dest;
}

/*
 * Sends a HTTP message to client.
 *
 * Argument "http" is a HTTP code + message, for example:
 * "403 Forbidden".
 *
 * Returns number of bytes sent to client, -1 if error.
 */

int
relay_http_send (struct t_relay_client *client,
                 int return_code, const char *message,
                 const char *headers,
                 const char *body, int body_size)
{
    const char *ptr_body;
    char str_header[1024], str_content_encoding[256];
    char *compressed_body, *http_message, *raw_message;
    int length_header, length_msg, num_bytes;
    int *ptr_body_size, compressed_body_size;

    if (!client || !message || (body_size < 0))
        return -1;

    str_content_encoding[0] = '\0';

    ptr_body = body;
    ptr_body_size = &body_size;

    compressed_body = relay_http_compress (client->http_req, body, body_size,
                                           &compressed_body_size,
                                           str_content_encoding,
                                           sizeof (str_content_encoding));
    if (compressed_body)
    {
        ptr_body = compressed_body;
        ptr_body_size = &compressed_body_size;
    }

    snprintf (str_header, sizeof (str_header),
              "HTTP/1.1 %d %s\r\n"
              "%s%s"
              "%s"
              "Content-Length: %d\r\n"
              "\r\n",
              return_code,
              (message) ? message : "",
              (headers) ? headers : "",
              (headers && headers[0]) ? "\r\n" : "",
              str_content_encoding,
              *ptr_body_size);

    length_header = strlen (str_header);

    if (!ptr_body || (*ptr_body_size <= 0))
    {
        num_bytes = relay_client_send (client, RELAY_MSG_STANDARD,
                                       str_header, length_header, NULL);
    }
    else
    {
        length_msg = length_header + (*ptr_body_size);
        http_message = malloc (length_msg + 1);
        if (http_message)
        {
            memcpy (http_message, str_header, length_header);
            memcpy (http_message + length_header, ptr_body, *ptr_body_size);
            http_message[length_msg] = '\0';
            raw_message = NULL;
            if (compressed_body)
            {
                weechat_asprintf (&raw_message,
                                  "%s[%d bytes data]",
                                  str_header,
                                  *ptr_body_size);
            }
            num_bytes = relay_client_send (client, RELAY_MSG_STANDARD,
                                           http_message, length_msg,
                                           raw_message);
            free (raw_message);
            free (http_message);
        }
        else
        {
            num_bytes = -1;
        }
    }

    free (compressed_body);

    return num_bytes;
}

/*
 * Sends JSON string to client.
 *
 * Returns number of bytes sent to client, -1 if error.
 */

int
relay_http_send_json (struct t_relay_client *client,
                      int return_code,
                      const char *message,
                      const char *headers,
                      const char *json_string)
{
    int num_bytes;
    char *headers2;

    if (!client || !message)
        return -1;

    weechat_asprintf (
        &headers2,
        "%s%s%s",
        (headers) ? headers : "",
        (headers && headers[0]) ? "\r\n" : "",
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Type: application/json; charset=utf-8");

    num_bytes = relay_http_send (client,
                                 return_code,
                                 message,
                                 headers2,
                                 json_string,
                                 (json_string) ? strlen (json_string) : 0);

    free (headers2);

    return num_bytes;
}

/*
 * Sends JSON error to client.
 *
 * Returns number of bytes sent to client, -1 if error.
 */

int
relay_http_send_error_json (struct t_relay_client *client,
                            int return_code,
                            const char *message,
                            const char *headers,
                            const char *format, ...)
{
    int num_bytes, length;
    char *error_msg, *json;

    if (!client || !message || !format)
        return -1;

    weechat_va_format (format);
    if (!vbuffer)
        return -1;

    num_bytes = -1;
    error_msg = NULL;
    json = NULL;

    error_msg = weechat_string_replace (vbuffer, "\"", "\\\"");
    if (!error_msg)
        goto end;

    length = strlen (error_msg) + 64;
    json = malloc (length);
    if (!json)
        goto end;
    snprintf (json, length, "{\"error\": \"%s\"}", error_msg);

    num_bytes = relay_http_send_json (client, return_code, message, headers,
                                      json);

end:
    free (vbuffer);
    free (error_msg);
    free (json);
    return num_bytes;
}

/*
 * Frees a HTTP request.
 */

void
relay_http_request_free (struct t_relay_http_request *request)
{
    weechat_string_dyn_free (request->raw, 1);
    free (request->method);
    free (request->path);
    weechat_string_free_split (request->path_items);
    weechat_hashtable_free (request->params);
    free (request->http_version);
    weechat_hashtable_free (request->headers);
    weechat_hashtable_free (request->accept_encoding);
    relay_websocket_deflate_free (request->ws_deflate);
    free (request->body);

    free (request);
}

/*
 * Allocates a t_relay_http_response structure.
 */

struct t_relay_http_response *
relay_http_response_alloc ()
{
    struct t_relay_http_response *new_response;

    new_response = (struct t_relay_http_response *)malloc (sizeof (*new_response));
    if (!new_response)
        return NULL;

    new_response->status = RELAY_HTTP_METHOD;
    new_response->http_version = NULL;
    new_response->return_code = 0;
    new_response->message = NULL;
    new_response->headers = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    new_response->content_length = 0;
    new_response->body_size = 0;
    new_response->body = NULL;

    return new_response;
}

/*
 * Parses and saves response code.
 *
 * Returns:
 *   1: OK, response code and HTTP version saved
 *   0: error: invalid format
 */

int
relay_http_parse_response_code (struct t_relay_http_response *response,
                                const char *response_code)
{
    const char *pos, *pos2;
    char *error, *return_code;
    long value;

    if (!response)
        return 0;

    if (!response_code || !response_code[0])
    {
        response->status = RELAY_HTTP_END;
        return 0;
    }

    pos = strchr (response_code, ' ');
    if (!pos)
        goto error;

    free (response->http_version);
    response->http_version = weechat_strndup (response_code, pos - response_code);

    while (pos[0] == ' ')
    {
        pos++;
    }

    pos2 = strchr (pos, ' ');
    if (pos2)
        return_code = weechat_strndup (pos, pos2 - pos);
    else
        return_code = strdup (pos);
    if (!return_code)
        goto error;

    error = NULL;
    value = strtol (return_code, &error, 10);
    if (error && !error[0])
        response->return_code = (int)value;

    free (return_code);

    if (pos2)
    {
        while (pos2[0] == ' ')
        {
            pos2++;
        }
        free (response->message);
        response->message = strdup (pos2);
    }

    response->status = RELAY_HTTP_HEADERS;

    return 1;

error:
    response->status = RELAY_HTTP_END;
    return 0;
}

/*
 * Parses and saves a header of a HTTP response in hashtable "headers".
 *
 * Returns:
 *   1: OK, header saved
 *   0: error: invalid format
 */

int
relay_http_parse_response_header (struct t_relay_http_response *response,
                                  const char *header)
{
    char *pos, *name, *name_lower, *error;
    const char *ptr_value;
    long number;

    /* empty line => end of headers */
    if (!header || !header[0])
    {
        response->status = (response->content_length > 0) ?
            RELAY_HTTP_BODY : RELAY_HTTP_END;
        return 1;
    }

    pos = strchr (header, ':');

    /* not a valid header */
    if (!pos || (pos == header))
        return 0;

    /* get header name, which is case-insensitive */
    name = weechat_strndup (header, pos - header);
    if (!name)
        return 0;
    name_lower = weechat_string_tolower (name);
    if (!name_lower)
    {
        free (name);
        return 0;
    }

    /* get pointer on header value */
    ptr_value = pos + 1;
    while (ptr_value[0] == ' ')
    {
        ptr_value++;
    }

    /* add header in the hashtable */
    weechat_hashtable_set (response->headers, name_lower, ptr_value);

    /* if header is "Content-Length", save the length */
    if (strcmp (name_lower, "content-length") == 0)
    {
        error = NULL;
        number = strtol (ptr_value, &error, 10);
        if (error && !error[0])
            response->content_length = (int)number;
    }

    free (name);
    free (name_lower);

    return 1;
}

/*
 * Parses HTTP response with a string.
 *
 * Returns HTTP request structure, NULL if error.
 */

struct t_relay_http_response *
relay_http_parse_response (const char *data)
{
    struct t_relay_http_response *http_resp;
    const char *ptr_data, *pos;
    char *line;

    if (!data || !data[0])
        return NULL;

    http_resp = relay_http_response_alloc ();
    if (!http_resp)
        return NULL;

    ptr_data = data;
    while (ptr_data && ptr_data[0])
    {
        if ((http_resp->status == RELAY_HTTP_METHOD)
            || (http_resp->status == RELAY_HTTP_HEADERS))
        {
            pos = strchr (ptr_data, '\r');
            if (!pos)
                break;
            line = weechat_strndup (ptr_data, pos - ptr_data);
            if (!line)
                break;
            if (http_resp->status == RELAY_HTTP_METHOD)
                relay_http_parse_response_code (http_resp, line);
            else
                relay_http_parse_response_header (http_resp, line);
            free (line);
            ptr_data = pos + 1;
            if (ptr_data[0] == '\n')
                ptr_data++;
        }
        else if (http_resp->status == RELAY_HTTP_BODY)
        {
            http_resp->body_size = strlen (ptr_data);
            http_resp->body = malloc (http_resp->body_size + 1);
            if (http_resp->body)
            {
                memcpy (http_resp->body, ptr_data, http_resp->body_size);
                http_resp->body[http_resp->body_size] = '\0';
            }
            http_resp->status = RELAY_HTTP_END;
        }
        else
            break;

        if (http_resp->status == RELAY_HTTP_END)
            break;
    }

    return http_resp;
}

/*
 * Frees a HTTP response.
 */

void
relay_http_response_free (struct t_relay_http_response *response)
{
    if (!response)
        return;

    free (response->http_version);
    free (response->message);
    weechat_hashtable_free (response->headers);
    free (response->body);

    free (response);
}

/*
 * Prints HTTP request in WeeChat log file (usually for crash dump).
 */

void
relay_http_print_log_request (struct t_relay_http_request *request)
{
    int i;

    weechat_log_printf ("  http_request:");
    weechat_log_printf ("    status. . . . . . . . . : %d", request->status);
    weechat_log_printf ("    raw . . . . . . . . . . : '%s'",
                        (request->raw) ? *(request->raw) : NULL);
    weechat_log_printf ("    method. . . . . . . . . : '%s'", request->method);
    weechat_log_printf ("    path. . . . . . . . . . : '%s'", request->path);
    weechat_log_printf ("    path_items. . . . . . . : %p", request->path_items);
    if (request->path_items)
    {
        for (i = 0; request->path_items[0]; i++)
        {
            weechat_log_printf ("      '%s'", request->path_items[i]);
        }
    }
    weechat_log_printf ("    num_path_items. . . . . : %d", request->num_path_items);
    weechat_log_printf ("    params. . . . . . . . . : %p (hashtable: '%s')",
                        request->params,
                        weechat_hashtable_get_string (request->params, "keys_values"));
    weechat_log_printf ("    http_version. . . . . . : '%s'", request->http_version);
    weechat_log_printf ("    headers . . . . . . . . : %p (hashtable: '%s')",
                        request->headers,
                        weechat_hashtable_get_string (request->headers, "keys_values"));
    weechat_log_printf ("    accept_encoding . . . . : %p (hashtable: '%s')",
                        request->accept_encoding,
                        weechat_hashtable_get_string (request->accept_encoding,
                                                      "keys_values"));
    relay_websocket_deflate_print_log (request->ws_deflate, "  ");
    weechat_log_printf ("    content_length. . . . . : %d", request->content_length);
    weechat_log_printf ("    body_size . . . . . . . : %d", request->body_size);
    weechat_log_printf ("    body. . . . . . . . . . : '%s'", request->body);
}

/*
 * Prints HTTP response in WeeChat log file (usually for crash dump).
 */

void
relay_http_print_log_response (struct t_relay_http_response *response)
{
    weechat_log_printf ("  http_response:");
    weechat_log_printf ("    status. . . . . . . . . : %d", response->status);
    weechat_log_printf ("    http_version. . . . . . : '%s'", response->http_version);
    weechat_log_printf ("    return_code . . . . . . : %d", response->return_code);
    weechat_log_printf ("    message . . . . . . . . : '%s'", response->message);
    weechat_log_printf ("    headers . . . . . . . . : 0x%lx (hashtable: '%s')",
                        response->headers,
                        weechat_hashtable_get_string (response->headers, "keys_values"));
    weechat_log_printf ("    content_length. . . . . : %d", response->content_length);
    weechat_log_printf ("    body_size . . . . . . . : %d", response->body_size);
    weechat_log_printf ("    body. . . . . . . . . . : '%s'", response->body);
}
