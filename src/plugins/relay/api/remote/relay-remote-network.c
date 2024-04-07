/*
 * relay-remote-network.c - network functions for relay remote
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


/*
 * Gets URL to an API resource.
 *
 * For example if remote URL is "https://localhost:9000" and the resource is
 * "handshake", it returns: "https://localhost:9000/api/handshake".
 *
 * Note: result must be free after use.
 */

char *
relay_remote_network_get_url_resource (struct t_relay_remote *remote,
                                       const char *resource)
{
    const char *ptr_url;
    char url[4096];

    if (!remote || !resource || !resource[0])
        return NULL;

    ptr_url = weechat_config_string (remote->options[RELAY_REMOTE_OPTION_URL]);
    if (!ptr_url || !ptr_url[0])
        return NULL;

    snprintf (url, sizeof (url),
              "%s%sapi/%s",
              ptr_url,
              (ptr_url[strlen (ptr_url) - 1] == '/') ? "" : "/",
              resource);

    return strdup (url);
}

/*
 * Close connection with remote.
 */

void
relay_remote_network_close_connection (struct t_relay_remote *remote)
{
    if (!remote)
        return;
    if (remote->hook_fd)
    {
        weechat_unhook (remote->hook_fd);
        remote->hook_fd = NULL;
    }
    if (remote->hook_connect)
    {
        weechat_unhook (remote->hook_connect);
        remote->hook_connect = NULL;
    }
    if (remote->sock != -1)
    {
#ifdef _WIN32
        closesocket (remote->sock);
#else
        close (remote->sock);
#endif /* _WIN32 */
        remote->sock = -1;
    }
    relay_websocket_deflate_free (remote->ws_deflate);
    remote->ws_deflate = NULL;
    remote->synced = 0;
}

/*
 * Disconnects from remote.
 */

void
relay_remote_network_disconnect (struct t_relay_remote *remote)
{
    if (!remote)
        return;

    relay_remote_network_close_connection (remote);
    relay_remote_set_status (remote, RELAY_STATUS_DISCONNECTED);
    weechat_printf (NULL, "remote[%s]: disconnected", remote->name);
}

/*
 * Checks if authentication via websocket handshake was successful.
 *
 * Returns:
 *   1: authentication successful
 *   0: authentication has failed
 */

int
relay_remote_network_check_auth (struct t_relay_remote *remote,
                                 const char *buffer)
{
    struct t_relay_http_response *http_resp;
    cJSON *json_body, *json_error;
    const char *msg_error, *msg_resp_error, *ptr_ws_accept;
    char *key, hash[160 / 8], sec_websocket_accept[128];
    int accept_ok, hash_size;

    http_resp = NULL;
    msg_error = NULL;
    msg_resp_error = NULL;
    accept_ok = 0;

    http_resp = relay_http_parse_response (buffer);
    if (!http_resp)
    {
        msg_error = _("invalid response from remote relay");
        goto error;
    }

    if (http_resp->body)
    {
        json_body = cJSON_Parse (http_resp->body);
        if (json_body)
        {
            json_error = cJSON_GetObjectItem (json_body, "error");
            if (json_error && cJSON_IsString (json_error))
                msg_resp_error = cJSON_GetStringValue (json_error);
        }
    }

    if ((http_resp->return_code != 101)
        || (weechat_strcasecmp (http_resp->message, "Switching Protocols") != 0))
    {
        if (http_resp->return_code == 401)
            msg_error = _("authentication failed with remote relay");
        else
            msg_error = _("invalid response from remote relay");
        goto error;
    }

    if (remote->websocket_key)
    {
        ptr_ws_accept = weechat_hashtable_get (http_resp->headers,
                                               "sec-websocket-accept");
        if (ptr_ws_accept)
        {
            if (weechat_asprintf (&key, "%s%s", remote->websocket_key,
                                  WEBSOCKET_GUID) >= 0)
            {
                if (weechat_crypto_hash (key, strlen (key), "sha1",
                                         hash, &hash_size))
                {
                    if (weechat_string_base_encode ("64", hash, hash_size,
                                                    sec_websocket_accept) > 0)
                    {
                        if (strcmp (ptr_ws_accept, sec_websocket_accept) == 0)
                            accept_ok = 1;
                    }
                }
                free (key);
            }
        }
    }

    relay_websocket_parse_extensions (
        weechat_hashtable_get (http_resp->headers, "sec-websocket-extensions"),
        remote->ws_deflate);

    if (!accept_ok)
    {
        msg_error = _("invalid websocket response (handshake error)");
        goto error;
    }

    relay_http_response_free (http_resp);

    return 1;

error:
    weechat_printf (
        NULL,
        _("%sremote[%s]: error: %s%s%s%s"),
        weechat_prefix ("error"),
        remote->name,
        msg_error,
        (msg_resp_error) ? " (" : "",
        (msg_resp_error) ? msg_resp_error : "",
        (msg_resp_error) ? ")" : "");
    if (http_resp)
        relay_http_response_free (http_resp);
    return 0;
}

/*
 * Sends data to the remote.
 *
 * Returns the number of bytes sent to the remote.
 */

int
relay_remote_network_send_data (struct t_relay_remote *remote,
                                const char *data, int data_size)
{
    if (!remote)
        return 0;

    if (remote->tls)
    {
        return (remote->sock >= 0) ?
            gnutls_record_send (remote->gnutls_sess, data, data_size) :
            data_size;
    }
    else
    {
        return (remote->sock >= 0) ?
            send (remote->sock, data, data_size, 0) :
            data_size;
    }

}

/*
 * Sends data to the remote.
 * If the remote is connected, encapsulate data in a websocket frame.
 *
 * Returns the number of bytes sent to the remote.
 */

int
relay_remote_network_send (struct t_relay_remote *remote,
                           enum t_relay_msg_type msg_type,
                           const char *data, int data_size)
{
    const char *ptr_data;
    char *websocket_frame;
    unsigned long long length_frame;
    int opcode, flags, num_sent;

    if (!remote)
        return 0;

    ptr_data = data;
    websocket_frame = NULL;

    if (remote->status == RELAY_STATUS_CONNECTED)
    {
        /* encapsulate data in a websocket frame */
        switch (msg_type)
        {
            case RELAY_MSG_PING:
                opcode = WEBSOCKET_FRAME_OPCODE_PING;
                break;
            case RELAY_MSG_PONG:
                opcode = WEBSOCKET_FRAME_OPCODE_PONG;
                break;
            case RELAY_MSG_CLOSE:
                opcode = WEBSOCKET_FRAME_OPCODE_CLOSE;
                break;
            default:
                opcode = WEBSOCKET_FRAME_OPCODE_TEXT;
                break;
        }
        websocket_frame = relay_websocket_encode_frame (
            remote->ws_deflate, opcode, 1, data, data_size, &length_frame);
        if (websocket_frame)
        {
            ptr_data = websocket_frame;
            data_size = length_frame;
        }
    }

    num_sent = relay_remote_network_send_data (remote, ptr_data, data_size);

    if (websocket_frame)
        free (websocket_frame);

    if (num_sent >= 0)
    {
        flags = RELAY_RAW_FLAG_SEND;
        if ((msg_type == RELAY_MSG_PING)
            || (msg_type == RELAY_MSG_PONG)
            || (msg_type == RELAY_MSG_CLOSE))
        {
            flags |= RELAY_RAW_FLAG_BINARY;
        }
        relay_raw_print_remote (remote, msg_type, flags, data, data_size);
    }

    return num_sent;
}

/*
 * Sends JSON data to the remote.
 *
 * Returns the number of bytes sent to the remote.
 */

int
relay_remote_network_send_json (struct t_relay_remote *remote, cJSON *json)
{
    char *string;
    int num_bytes;

    if (!remote || !json)
        return 0;

    num_bytes = 0;

    string = cJSON_PrintUnformatted (json);
    if (string)
    {
        num_bytes = relay_remote_network_send (remote, RELAY_MSG_STANDARD,
                                               string, strlen (string));
        free (string);
    }

    return num_bytes;
}

/*
 * Reads text buffer from a remote.
 */

void
relay_remote_network_recv_text (struct t_relay_remote *remote,
                                const char *buffer, int buffer_size)
{
    char request[1024];

    relay_raw_print_remote (remote, RELAY_MSG_STANDARD,
                            RELAY_RAW_FLAG_RECV,
                            buffer, buffer_size);

    if (remote->status == RELAY_STATUS_AUTHENTICATING)
    {
        if (!relay_remote_network_check_auth (remote, buffer))
        {
            relay_remote_network_disconnect (remote);
            return;
        }
        relay_remote_set_status (remote, RELAY_STATUS_CONNECTED);
        snprintf (request, sizeof (request),
                  "{\"request\": \"GET /api/version\"}");
        relay_remote_network_send (remote, RELAY_MSG_STANDARD,
                                   request, strlen (request));
        snprintf (request, sizeof (request),
                  "{\"request\": \"GET /api/buffers?lines=-100&colors=weechat\"}");
        relay_remote_network_send (remote, RELAY_MSG_STANDARD,
                                   request, strlen (request));
    }
    else
    {
        relay_remote_event_recv (remote, buffer);
    }
}

/*
 * Reads websocket frames.
 */

void
relay_remote_network_read_websocket_frames (struct t_relay_remote *remote,
                                            struct t_relay_websocket_frame *frames,
                                            int num_frames)
{
    int i;

    if (!frames || (num_frames <= 0))
        return;

    for (i = 0; i < num_frames; i++)
    {
        if (frames[i].payload_size == 0)
        {
            /*
             * When decoded length is 0, assume remote sent a PONG frame.
             *
             * RFC 6455 Section 5.5.3:
             *
             *   "A Pong frame MAY be sent unsolicited.  This serves as a
             *   unidirectional heartbeat.  A response to an unsolicited
             *   Pong frame is not expected."
             */
            continue;
        }
        switch (frames[i].opcode)
        {
            case RELAY_MSG_PING:
                /* print message in raw buffer */
                relay_raw_print_remote (remote, RELAY_MSG_PING,
                                        RELAY_RAW_FLAG_RECV | RELAY_RAW_FLAG_BINARY,
                                        frames[i].payload,
                                        frames[i].payload_size);
                /* answer with a PONG */
                relay_remote_network_send (remote,
                                           RELAY_MSG_PONG,
                                           frames[i].payload,
                                           frames[i].payload_size);
                break;
            case RELAY_MSG_CLOSE:
                /* print message in raw buffer */
                relay_raw_print_remote (remote, RELAY_MSG_CLOSE,
                                        RELAY_RAW_FLAG_RECV | RELAY_RAW_FLAG_BINARY,
                                        frames[i].payload,
                                        frames[i].payload_size);
                /* answer with a CLOSE */
                relay_remote_network_send (remote,
                                           RELAY_MSG_CLOSE,
                                           frames[i].payload,
                                           frames[i].payload_size);
                /* close the connection */
                relay_remote_network_disconnect (remote);
                /* ignore any other message after the close */
                return;
            default:
                if (frames[i].payload)
                {
                    relay_remote_network_recv_text (remote,
                                                    frames[i].payload,
                                                    frames[i].payload_size);
                }
                break;
        }
    }
}

/*
 * Reads a buffer of bytes from a remote.
 */

void
relay_remote_network_recv_buffer (struct t_relay_remote *remote,
                                  const char *buffer, int buffer_size)
{
    struct t_relay_websocket_frame *frames;
    char *partial_ws_frame;
    int rc, i, num_frames, partial_ws_frame_size;

    /* if authenticating is in progress, check if it was successful */
    if (remote->status == RELAY_STATUS_AUTHENTICATING)
    {
        relay_remote_network_recv_text (remote, buffer, buffer_size);
    }
    else if (remote->status == RELAY_STATUS_CONNECTED)
    {
        partial_ws_frame = NULL;
        partial_ws_frame_size = 0;
        rc = relay_websocket_decode_frame (
            (const unsigned char *)buffer,
            buffer_size,
            0,  /* expect_masked_frame */
            remote->ws_deflate,
            &frames,
            &num_frames,
            &partial_ws_frame,
            &partial_ws_frame_size);
        if (!rc)
        {
            /* fatal error when decoding frame: close connection */
            for (i = 0; i < num_frames; i++)
            {
                if (frames[i].payload)
                    free (frames[i].payload);
            }
            free (frames);
            weechat_printf (
                NULL,
                _("%sremote[%s]: error decoding websocket frame"),
                weechat_prefix ("error"),
                remote->name);
            relay_remote_network_disconnect (remote);
            return;
        }
        relay_remote_network_read_websocket_frames (remote, frames, num_frames);
    }
}

/*
 * Callback for fd hook.
 */

int
relay_remote_network_recv_cb (const void *pointer, void *data, int fd)
{
    struct t_relay_remote *remote;
    static char buffer[4096 + 2];
    int num_read, end_recv;

    /* make C compiler happy */
    (void) data;
    (void) fd;

    remote = (struct t_relay_remote *)pointer;
    if (!remote)
        return WEECHAT_RC_ERROR;

    end_recv = 0;
    while (!end_recv)
    {
        end_recv = 1;

        if (remote->tls)
        {
            if (!remote->gnutls_sess)
                return WEECHAT_RC_ERROR;
            num_read = gnutls_record_recv (remote->gnutls_sess, buffer,
                                           sizeof (buffer) - 2);
        }
        else
        {
            num_read = recv (remote->sock, buffer, sizeof (buffer) - 2, 0);
        }

        if (num_read > 0)
        {
            buffer[num_read] = '\0';
            if (remote->tls
                && (gnutls_record_check_pending (remote->gnutls_sess) > 0))
            {
                /*
                 * if there are unread data in the gnutls buffers,
                 * go on with recv
                 */
                end_recv = 0;
            }
            relay_remote_network_recv_buffer (remote, buffer, num_read);
        }
        else
        {
            if (remote->tls)
            {
                if ((num_read == 0)
                    || ((num_read != GNUTLS_E_AGAIN)
                        && (num_read != GNUTLS_E_INTERRUPTED)))
                {
                    weechat_printf (
                        NULL,
                        _("%sremote[%s]: reading data on socket: error %d %s"),
                        weechat_prefix ("error"),
                        remote->name,
                        num_read,
                        (num_read == 0) ? _("(connection closed by peer)") :
                        gnutls_strerror (num_read));
                    relay_remote_network_disconnect (remote);
                }
            }
            else
            {
                if ((num_read == 0)
                    || ((errno != EAGAIN) && (errno != EWOULDBLOCK)))
                {
                    weechat_printf (
                        NULL,
                        _("%sremote[%s]: reading data on socket: error %d %s"),
                        weechat_prefix ("error"),
                        remote->name,
                        errno,
                        (num_read == 0) ? _("(connection closed by peer)") :
                        strerror (errno));
                    relay_remote_network_disconnect (remote);
                }
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Connects to remote using websocket, with authentication.
 */

void
relay_remote_network_connect_ws_auth (struct t_relay_remote *remote)
{
    char *password, *totp_secret, *totp;
    char *salt_password, salt[64], str_auth[4096], str_auth_base64[4096];
    char str_http[8192], str_totp[128];
    char hash[512 / 8], hash_hexa[((512 / 8) * 2) + 1];
    char ws_key[16], ws_key_base64[64];
    int length, hash_size;

    relay_remote_set_status (remote, RELAY_STATUS_AUTHENTICATING);

    password = NULL;
    totp_secret = NULL;
    str_auth[0] = '\0';
    str_totp[0] = '\0';

    password = weechat_string_eval_expression (
        weechat_config_string (remote->options[RELAY_REMOTE_OPTION_PASSWORD]),
        NULL, NULL, NULL);
    if (!password)
        goto end;
    totp_secret = weechat_string_eval_expression (
        weechat_config_string (remote->options[RELAY_REMOTE_OPTION_TOTP_SECRET]),
        NULL, NULL, NULL);
    if (!totp_secret)
        goto end;

    switch (remote->password_hash_algo)
    {
        case RELAY_AUTH_PASSWORD_HASH_PLAIN:
            snprintf (str_auth, sizeof (str_auth), "plain:%s", password);
            break;
        case RELAY_AUTH_PASSWORD_HASH_SHA256:
        case RELAY_AUTH_PASSWORD_HASH_SHA512:
            length = strlen (password) + 64;
            salt_password = malloc (length);
            if (salt_password)
            {
                snprintf (salt_password, length,
                          "%ld%s",
                          time (NULL),
                          password);
                if (weechat_crypto_hash (
                        salt_password, strlen (salt_password),
                        relay_auth_password_hash_algo_name[remote->password_hash_algo],
                        hash, &hash_size))
                {
                    weechat_string_base_encode ("16", hash, hash_size, hash_hexa);
                    snprintf (str_auth, sizeof (str_auth),
                              "hash:%s", hash_hexa);
                }
                free (salt_password);
            }
            break;
        case RELAY_AUTH_PASSWORD_HASH_PBKDF2_SHA256:
        case RELAY_AUTH_PASSWORD_HASH_PBKDF2_SHA512:
            snprintf (salt, sizeof (salt), "%ld", time (NULL));
            if (weechat_crypto_hash_pbkdf2 (
                    password,
                    strlen (password),
                    relay_auth_password_hash_algo_name[remote->password_hash_algo] + 7,
                    salt, strlen (salt),
                    remote->password_hash_iterations,
                    hash, &hash_size))
            {
                weechat_string_base_encode ("16", hash, hash_size, hash_hexa);
                snprintf (str_auth, sizeof (str_auth),
                          "hash:%s:%s:%d:%s",
                          relay_auth_password_hash_algo_name[remote->password_hash_algo],
                          salt,
                          remote->password_hash_iterations,
                          hash_hexa);
            }
            break;
    }

    if (!str_auth[0])
    {
        weechat_printf (NULL, _("%sremote[%s]: failed to build authentication"),
                        weechat_prefix ("error"), remote->name);
        relay_remote_network_disconnect (remote);
        goto end;
    }

    /* generate random websocket key (16 bytes) */
    gcry_create_nonce (ws_key, sizeof (ws_key));
    weechat_string_base_encode ("64", ws_key, sizeof (ws_key), ws_key_base64);
    if (remote->websocket_key)
        free (remote->websocket_key);
    remote->websocket_key = strdup (ws_key_base64);

    weechat_string_base_encode ("64", str_auth, strlen (str_auth), str_auth_base64);

    if (totp_secret && totp_secret[0])
    {
        /* generate the TOTP with the secret */
        totp = weechat_info_get ("totp_generate", totp_secret);
        if (totp)
        {
            snprintf (str_totp, sizeof (str_totp),
                      "x-weechat-totp: %s\r\n",
                      totp);
            free (totp);
        }
    }

    snprintf (
        str_http, sizeof (str_http),
        "GET /api HTTP/1.1\r\n"
        "Authorization: Basic %s\r\n"
        "%s"
        "Sec-WebSocket-Version: 13\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Connection: Upgrade\r\n"
        "Upgrade: websocket\r\n"
        "Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits\r\n"
        "Host: %s:%d\r\n"
        "\r\n",
        str_auth_base64,
        str_totp,
        ws_key_base64,
        remote->address,
        remote->port);
    relay_remote_network_send (remote, RELAY_MSG_STANDARD,
                               str_http, strlen (str_http));

end:
    if (password)
        free (password);
    if (totp_secret)
        free (totp_secret);
}

/*
 * Callback for connect hook.
 */

int
relay_remote_network_connect_cb (const void *pointer, void *data, int status,
                                 int gnutls_rc, int sock, const char *error,
                                 const char *ip_address)
{
    struct t_relay_remote *remote;
    /*int dhkey_size;*/

    /* make C compiler happy */
    (void) data;

    remote = (struct t_relay_remote *)pointer;

    remote->hook_connect = NULL;

    remote->sock = sock;

    switch (status)
    {
        case WEECHAT_HOOK_CONNECT_OK:
            weechat_printf (NULL, _("remote[%s]: connected to %s/%d (%s)"),
                            remote->name, remote->address, remote->port,
                            ip_address);
            remote->hook_fd = weechat_hook_fd (remote->sock, 1, 0, 0,
                                               &relay_remote_network_recv_cb,
                                               remote, NULL);
            /* authenticate with remote relay */
            relay_remote_network_connect_ws_auth (remote);
            break;
        case WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND:
            weechat_printf (NULL, _("%sremote[%s]: address \"%s\" not found"),
                            weechat_prefix ("error"), remote->name,
                            remote->address);
            if (error && error[0])
            {
                weechat_printf (NULL, _("%sremote[%s]: error: %s"),
                                weechat_prefix ("error"), remote->name, error);
            }
            break;
        case WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND:
            weechat_printf (NULL, _("%sremote[%s]: IP address not found"),
                            weechat_prefix ("error"), remote->name);
            if (error && error[0])
            {
                weechat_printf (NULL, _("%sremote[%s]: error: %s"),
                                weechat_prefix ("error"), remote->name, error);
            }
            break;
        case WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED:
            weechat_printf (NULL, _("%sremote[%s]: connection refused"),
                            weechat_prefix ("error"), remote->name);
            if (error && error[0])
            {
                weechat_printf (NULL, _("%sremote[%s]: error: %s"),
                                weechat_prefix ("error"), remote->name, error);
            }
            break;
        case WEECHAT_HOOK_CONNECT_PROXY_ERROR:
            weechat_printf (
                NULL,
                _("%sremote[%s]: proxy fails to establish connection to server (check "
                  "username/password if used and if server address/port is "
                  "allowed by proxy)"),
                weechat_prefix ("error"), remote->name);
            if (error && error[0])
            {
                weechat_printf (NULL, _("%sremote[%s]: error: %s"),
                                weechat_prefix ("error"), remote->name, error);
            }
            break;
        case WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR:
            weechat_printf (NULL, _("%sremote[%s]: unable to set local hostname/IP"),
                            weechat_prefix ("error"), remote->name);
            if (error && error[0])
            {
                weechat_printf (NULL, _("%sremote[%s]: error: %s"),
                                weechat_prefix ("error"), remote->name, error);
            }
            break;
        case WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR:
            weechat_printf (NULL, _("%sremote[%s]: TLS init error"),
                            weechat_prefix ("error"), remote->name);
            if (error && error[0])
            {
                weechat_printf (NULL, _("%sremote[%s]: error: %s"),
                                weechat_prefix ("error"), remote->name, error);
            }
            break;
        case WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR:
            weechat_printf (NULL, _("%sremote[%s]: TLS handshake failed"),
                            weechat_prefix ("error"), remote->name);
            if (error && error[0])
            {
                weechat_printf (NULL, _("%sremote[%s]: error: %s"),
                                weechat_prefix ("error"), remote->name, error);
            }
            if (gnutls_rc == GNUTLS_E_DH_PRIME_UNACCEPTABLE)
            {
                /* dhkey_size = weechat_config_integer ( */
                /*     remote->options[RELAY_REMOTE_OPTION_TLS_DHKEY_SIZE]); */
                /* weechat_printf ( */
                /*     NULL, */
                /*     _("%sremote[%s]: you should play with option " */
                /*       "relay.remote.%s.tls_dhkey_size (current value is %d, try " */
                /*       "a lower value like %d or %d)"), */
                /*     weechat_prefix ("error"), */
                /*     remote->name, */
                /*     remote->name, */
                /*     dhkey_size, */
                /*     dhkey_size / 2, */
                /*     dhkey_size / 4); */
            }
            break;
        case WEECHAT_HOOK_CONNECT_MEMORY_ERROR:
            weechat_printf (NULL, _("%sremote[%s]: not enough memory"),
                            weechat_prefix ("error"), remote->name);
            if (error && error[0])
            {
                weechat_printf (NULL, _("%sremote[%s]: error: %s"),
                                weechat_prefix ("error"), remote->name, error);
            }
            break;
        case WEECHAT_HOOK_CONNECT_TIMEOUT:
            weechat_printf (NULL, _("%sremote[%s]: timeout"),
                            weechat_prefix ("error"), remote->name);
            if (error && error[0])
            {
                weechat_printf (NULL, _("%sremote[%s]: error: %s"),
                                weechat_prefix ("error"), remote->name, error);
            }
            break;
        case WEECHAT_HOOK_CONNECT_SOCKET_ERROR:
            weechat_printf (NULL, _("%sremote[%s]: unable to create socket"),
                            weechat_prefix ("error"), remote->name);
            if (error && error[0])
            {
                weechat_printf (NULL, _("%sremote[%s]: error: %s"),
                                weechat_prefix ("error"), remote->name, error);
            }
            break;
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for handshake URL.
 */

int
relay_remote_network_url_handshake_cb (const void *pointer,
                                       void *data,
                                       const char *url,
                                       struct t_hashtable *options,
                                       struct t_hashtable *output)
{
    struct t_relay_remote *remote;
    const char *ptr_output, *ptr_resp_code, *ptr_error;
    cJSON *json_body, *json_hash_algo, *json_hash_iterations, *json_totp;

    /* make C compiler happy */
    (void) data;
    (void) url;
    (void) options;

    remote = (struct t_relay_remote *)pointer;

    ptr_resp_code = weechat_hashtable_get (output, "response_code");
    if (ptr_resp_code && ptr_resp_code[0] && (strcmp (ptr_resp_code, "200") != 0))
    {
        weechat_printf (NULL,
                        _("%sremote[%s]: handshake failed with URL %s, response code: %s"),
                        weechat_prefix ("error"),
                        remote->name,
                        weechat_config_string (remote->options[RELAY_REMOTE_OPTION_URL]),
                        ptr_resp_code);
        return WEECHAT_RC_OK;
    }

    ptr_error = weechat_hashtable_get (output, "error");
    if (ptr_error && ptr_error[0])
    {
        weechat_printf (NULL,
                        _("%sremote[%s]: handshake failed with URL %s, error: %s"),
                        weechat_prefix ("error"),
                        remote->name,
                        weechat_config_string (remote->options[RELAY_REMOTE_OPTION_URL]),
                        ptr_error);
        return WEECHAT_RC_OK;
    }

    ptr_output = weechat_hashtable_get (output, "output");
    if (ptr_output && ptr_output[0])
    {
        json_body = cJSON_Parse (weechat_hashtable_get (output, "output"));
        if (json_body)
        {
            /* hash algorithm */
            json_hash_algo = cJSON_GetObjectItem (json_body, "password_hash_algo");
            if (json_hash_algo && cJSON_IsString (json_hash_algo))
            {
                remote->password_hash_algo = relay_auth_password_hash_algo_search (
                    cJSON_GetStringValue (json_hash_algo));
            }
            /* hash iterations */
            json_hash_iterations = cJSON_GetObjectItem (json_body, "password_hash_iterations");
            if (json_hash_iterations && cJSON_IsNumber (json_hash_iterations))
                remote->password_hash_iterations = (int)cJSON_GetNumberValue (json_hash_iterations);
            /* TOTP */
            json_totp = cJSON_GetObjectItem (json_body, "totp");
            if (json_totp && cJSON_IsBool (json_totp))
                remote->totp = (cJSON_IsTrue (json_totp)) ? 1 : 0;
        }
    }

    if (remote->password_hash_algo < 0)
    {
        weechat_printf (NULL,
                        _("%sremote[%s]: handshake failed with URL %s, error: %s"),
                        weechat_prefix ("error"),
                        remote->name,
                        weechat_config_string (remote->options[RELAY_REMOTE_OPTION_URL]),
                        _("hash algorithm not found"));
        return WEECHAT_RC_OK;
    }

    if (remote->password_hash_iterations < 0)
    {
        weechat_printf (NULL,
                        _("%sremote[%s]: handshake failed with URL %s, error: %s"),
                        weechat_prefix ("error"),
                        remote->name,
                        weechat_config_string (remote->options[RELAY_REMOTE_OPTION_URL]),
                        _("unknown number of hash iterations"));
        return WEECHAT_RC_OK;
    }

    if (remote->totp < 0)
    {
        weechat_printf (NULL,
                        _("%sremote[%s]: handshake failed with URL %s, error: %s"),
                        weechat_prefix ("error"),
                        remote->name,
                        weechat_config_string (remote->options[RELAY_REMOTE_OPTION_URL]),
                        _("unknown TOTP status"));
        return WEECHAT_RC_OK;
    }

    if (weechat_relay_plugin->debug >= 1)
    {
        weechat_printf (NULL,
                        _("%sremote[%s]: successful handshake with URL %s: "
                          "hash_algo=%s, iterations=%d, totp=%d"),
                        RELAY_PLUGIN_NAME,
                        remote->name,
                        weechat_config_string (remote->options[RELAY_REMOTE_OPTION_URL]),
                        relay_auth_password_hash_algo_name[remote->password_hash_algo],
                        remote->password_hash_iterations,
                        remote->totp);
    }

    remote->hook_connect = weechat_hook_connect (
        NULL,  /* proxy */
        remote->address,
        remote->port,
        1,  /* ipv6 */
        0,  /* retry */
        NULL,  /* gnutls_sess */
        NULL,  /* gnutls_cb */
        0,  /* gnutls_dhkey_size */
        NULL,  /* gnutls_priorities */
        NULL,  /* local_hostname */
        &relay_remote_network_connect_cb,
        remote,
        NULL);

    return WEECHAT_RC_OK;
}

/*
 * Builds a string with the API HTTP handshake request.
 *
 * Note: result must be free after use.
 */

char *
relay_remote_network_get_handshake_request ()
{
    char **body;
    int i;

    body = weechat_string_dyn_alloc (256);
    if (!body)
        return NULL;

    weechat_string_dyn_concat (body, "{\"password_hash_algo\": [", -1);
    /* all password hash algorithms are supported */
    for (i = 0; i < RELAY_NUM_PASSWORD_HASH_ALGOS; i++)
    {
        if (i > 0)
            weechat_string_dyn_concat (body, ", ", -1);
        weechat_string_dyn_concat (body, "\"", -1);
        weechat_string_dyn_concat (body,
                                   relay_auth_password_hash_algo_name[i], -1);
        weechat_string_dyn_concat (body, "\"", -1);
    }
    weechat_string_dyn_concat (body, "]}", -1);
    return weechat_string_dyn_free (body, 0);
}

/*
 * Connects to a remote WeeChat relay/api.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
relay_remote_network_connect (struct t_relay_remote *remote)
{
    char *url, *body;
    struct t_hashtable *options;

    url = NULL;
    body = NULL;
    options = NULL;

    if (!remote)
        return 0;

    if (remote->sock != -1)
    {
        weechat_printf (
            NULL,
            _("%s%s: already connected to remote relay \"%s\"!"),
            weechat_prefix ("error"), RELAY_PLUGIN_NAME, remote->name);
        return 0;
    }

    relay_remote_set_status (remote, RELAY_STATUS_CONNECTING);

    weechat_printf (NULL,
                    _("remote[%s]: connecting to remote relay %s/%d%s..."),
                    remote->name,
                    remote->address,
                    remote->port,
                    (remote->tls) ? " (TLS)" : "");

    url = relay_remote_network_get_url_resource (remote, "handshake");
    if (!url)
        goto error;

    options = weechat_hashtable_new (32,
                                     WEECHAT_HASHTABLE_STRING,
                                     WEECHAT_HASHTABLE_STRING,
                                     NULL, NULL);
    if (!options)
        goto error;

    weechat_hashtable_set (options, "post", "1");
    weechat_hashtable_set (options,
                           "httpheader",
                           "Accept: application/json\n"
                           "Content-Type: application/json; charset=utf-8");
    body = relay_remote_network_get_handshake_request ();
    if (!body)
        goto error;

    weechat_hashtable_set (options, "postfields", body);

    remote->hook_url_handshake = weechat_hook_url (
        url, options, 5 * 1000,
        &relay_remote_network_url_handshake_cb, remote, NULL);

    free (url);
    free (body);
    weechat_hashtable_free (options);

    return 1;

error:
    weechat_printf (NULL,
                    _("remote[%s]: failed to connect, not enough memory"),
                    remote->name);
    if (url)
        free (url);
    if (body)
        free (body);
    if (options)
        weechat_hashtable_free (options);
    return 0;
}
