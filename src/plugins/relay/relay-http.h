/*
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

#ifndef WEECHAT_PLUGIN_RELAY_HTTP_H
#define WEECHAT_PLUGIN_RELAY_HTTP_H

struct t_relay_client;

enum t_relay_client_http_status
{
    RELAY_HTTP_METHOD = 0,             /* reading method (eg: GET, POST)    */
    RELAY_HTTP_HEADERS,                /* reading headers                   */
    RELAY_HTTP_BODY,                   /* reading body                      */
    RELAY_HTTP_END,                    /* end of HTTP request               */
    /* number of HTTP status */
    RELAY_NUM_HTTP_STATUS,
};

#define RELAY_HTTP_200_OK                     200, "OK"
#define RELAY_HTTP_204_NO_CONTENT             204, "No Content"
#define RELAY_HTTP_400_BAD_REQUEST            400, "Bad Request"
#define RELAY_HTTP_401_UNAUTHORIZED           401, "Unauthorized"
#define RELAY_HTTP_403_FORBIDDEN              403, "Forbidden"
#define RELAY_HTTP_404_NOT_FOUND              404, "Not Found"
#define RELAY_HTTP_500_INTERNAL_SERVER_ERROR  500, "Internal Server Error"
#define RELAY_HTTP_503_SERVICE_UNAVAILABLE    503, "Service Unvavailable"

#define RELAY_HTTP_ERROR_MISSING_PASSWORD     "Missing password"
#define RELAY_HTTP_ERROR_INVALID_PASSWORD     "Invalid password"
#define RELAY_HTTP_ERROR_MISSING_TOTP         "Missing TOTP"
#define RELAY_HTTP_ERROR_INVALID_TOTP         "Invalid TOTP"
#define RELAY_HTTP_ERROR_INVALID_HASH_ALGO    "Invalid hash algorithm " \
    "(not found or not supported)"
#define RELAY_HTTP_ERROR_INVALID_TIMESTAMP    "Invalid timestamp"
#define RELAY_HTTP_ERROR_INVALID_ITERATIONS   "Invalid number of iterations"
#define RELAY_HTTP_ERROR_OUT_OF_MEMORY        "Out of memory"

struct t_relay_http_request
{
    enum t_relay_client_http_status status; /* HTTP status                  */
    char **raw;                        /* raw request                       */
    char *method;                      /* method (GET, POST, etc.)          */
    char *path;                        /* path after method                 */
    char **path_items;                 /* list of items in path, eg:        */
                                       /* "/api/a/b" => ["api", "a", "b"]   */
    int num_path_items;                /* number of path items              */
    struct t_hashtable *params;        /* optional parameters ("?p=a&q=b")  */
    char *http_version;                /* HTTP version (eg: "HTTP/1.1")     */
    struct t_hashtable *headers;       /* HTTP headers for websocket        */
                                       /* and API protocol                  */
    struct t_hashtable *accept_encoding; /* allowed encoding for response   */
    struct t_relay_websocket_deflate *ws_deflate; /* websocket deflate data */
    int content_length;                /* value of header "Content-Length"  */
    int body_size;                     /* size of HTTP body read so far     */
    char *body;                        /* HTTP body (can be NULL)           */
};

struct t_relay_http_response
{
    enum t_relay_client_http_status status; /* HTTP status                  */
    char *http_version;                /* HTTP version (eg: "HTTP/1.1")     */
    int return_code;                   /* HTTP return code (eg: 200, 401)   */
    char *message;                     /* message after return code         */
    struct t_hashtable *headers;       /* HTTP headers for websocket        */
                                       /* and API protocol                  */
    int content_length;                /* value of header "Content-Length"  */
    int body_size;                     /* size of HTTP body read so far     */
    char *body;                        /* HTTP body (can be NULL)           */
};

extern void relay_http_request_reinit (struct t_relay_http_request *request);
extern struct t_relay_http_request *relay_http_request_alloc ();
extern int relay_http_get_param_boolean (struct t_relay_http_request *request,
                                         const char *name, int default_value);
extern long relay_http_get_param_long (struct t_relay_http_request *request,
                                       const char *name, long default_value);
extern int relay_http_parse_method_path (struct t_relay_http_request *request,
                                         const char *method_path);
extern int relay_http_check_auth (struct t_relay_client *client);
extern void relay_http_recv (struct t_relay_client *client, const char *data);
extern int relay_http_send (struct t_relay_client *client,
                            int return_code, const char *message,
                            const char *headers,
                            const char *body, int body_size);
extern int relay_http_send_json (struct t_relay_client *client,
                                 int return_code,
                                 const char *message,
                                 const char *headers,
                                 const char *json_string);
extern int relay_http_send_error_json (struct t_relay_client *client,
                                       int return_code,
                                       const char *message,
                                       const char *headers,
                                       const char *format, ...);
extern void relay_http_request_free (struct t_relay_http_request *request);
extern struct t_relay_http_response *relay_http_response_alloc ();
extern struct t_relay_http_response *relay_http_parse_response (const char *data);
extern void relay_http_response_free (struct t_relay_http_response *response);
extern void relay_http_print_log_request (struct t_relay_http_request *request);
extern void relay_http_print_log_response (struct t_relay_http_response *response);

#endif /* WEECHAT_PLUGIN_RELAY_HTTP_H */
