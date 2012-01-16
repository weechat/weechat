/*
 * Copyright (C) 2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * wee-url.c: URL transfer
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

#include "weechat.h"
#include "wee-url.h"
#include "wee-hashtable.h"
#include "wee-infolist.h"
#include "wee-string.h"


#define URL_DEF_CONST(__prefix, __name)                                 \
    { #__name, CURL##__prefix##_##__name }
#define URL_DEF_OPTION(__name, __type, __constants)                     \
    { #__name, CURLOPT_##__name, URL_TYPE_##__type, __constants }


char *url_type_string[] = { "string", "long", "mask" };

struct t_url_constant url_proxy_types[] =
{
#if LIBCURL_VERSION_NUM >= 0x070A00
    /* libcurl >= 7.10 */
    URL_DEF_CONST(PROXY, HTTP),
#endif
#if LIBCURL_VERSION_NUM >= 0x071304
    /* libcurl >= 7.19.4 */
    URL_DEF_CONST(PROXY, HTTP_1_0),
#endif
#if LIBCURL_VERSION_NUM >= 0x070A00
    /* libcurl >= 7.10 */
    URL_DEF_CONST(PROXY, SOCKS4),
    URL_DEF_CONST(PROXY, SOCKS5),
#endif
#if LIBCURL_VERSION_NUM >= 0x071200
    /* libcurl >= 7.18.0 */
    URL_DEF_CONST(PROXY, SOCKS4A),
    URL_DEF_CONST(PROXY, SOCKS5_HOSTNAME),
#endif
    { NULL, 0 },
};

struct t_url_constant url_protocols[] =
{
#if LIBCURL_VERSION_NUM >= 0x071304
    /* libcurl >= 7.19.4 */
    URL_DEF_CONST(PROTO, HTTP),
    URL_DEF_CONST(PROTO, HTTPS),
    URL_DEF_CONST(PROTO, FTP),
    URL_DEF_CONST(PROTO, FTPS),
    URL_DEF_CONST(PROTO, SCP),
    URL_DEF_CONST(PROTO, SFTP),
    URL_DEF_CONST(PROTO, TELNET),
    URL_DEF_CONST(PROTO, LDAP),
    URL_DEF_CONST(PROTO, LDAPS),
    URL_DEF_CONST(PROTO, DICT),
    URL_DEF_CONST(PROTO, FILE),
    URL_DEF_CONST(PROTO, TFTP),
#endif
#if LIBCURL_VERSION_NUM >= 0x071400
    /* libcurl >= 7.20.0 */
    URL_DEF_CONST(PROTO, IMAP),
    URL_DEF_CONST(PROTO, IMAPS),
    URL_DEF_CONST(PROTO, POP3),
    URL_DEF_CONST(PROTO, POP3S),
    URL_DEF_CONST(PROTO, SMTP),
    URL_DEF_CONST(PROTO, SMTPS),
    URL_DEF_CONST(PROTO, RTSP),
#endif
#if LIBCURL_VERSION_NUM >= 0x071500
    /* libcurl >= 7.21.0 */
    URL_DEF_CONST(PROTO, RTMP),
    URL_DEF_CONST(PROTO, RTMPT),
    URL_DEF_CONST(PROTO, RTMPE),
    URL_DEF_CONST(PROTO, RTMPTE),
    URL_DEF_CONST(PROTO, RTMPS),
    URL_DEF_CONST(PROTO, RTMPTS),
#endif
#if LIBCURL_VERSION_NUM >= 0x071502
    /* libcurl >= 7.21.2 */
    URL_DEF_CONST(PROTO, GOPHER),
#endif
#if LIBCURL_VERSION_NUM >= 0x071304
    /* libcurl >= 7.19.4 */
    URL_DEF_CONST(PROTO, ALL),
#endif
    { NULL, 0 },
};

struct t_url_constant url_netrc[] =
{
#if LIBCURL_VERSION_NUM >= 0x070908
    /* libcurl >= 7.9.8 */
    URL_DEF_CONST(_NETRC, IGNORED),
    URL_DEF_CONST(_NETRC, OPTIONAL),
    URL_DEF_CONST(_NETRC, REQUIRED),
#endif
    { NULL, 0 },
};

struct t_url_constant url_auth[] =
{
#if LIBCURL_VERSION_NUM >= 0x070A06
    /* libcurl >= 7.10.6 */
    URL_DEF_CONST(AUTH, NONE),
    URL_DEF_CONST(AUTH, BASIC),
    URL_DEF_CONST(AUTH, DIGEST),
    URL_DEF_CONST(AUTH, GSSNEGOTIATE),
    URL_DEF_CONST(AUTH, NTLM),
#endif
#if LIBCURL_VERSION_NUM >= 0x071303
    /* libcurl >= 7.19.3 */
    URL_DEF_CONST(AUTH, DIGEST_IE),
#endif
#if LIBCURL_VERSION_NUM >= 0x071600
    /* libcurl >= 7.22.0 */
    URL_DEF_CONST(AUTH, NTLM_WB),
#endif
#if LIBCURL_VERSION_NUM >= 0x071503
    /* libcurl >= 7.21.3 */
    URL_DEF_CONST(AUTH, ONLY),
#endif
#if LIBCURL_VERSION_NUM >= 0x070A06
    /* libcurl >= 7.10.6 */
    URL_DEF_CONST(AUTH, ANY),
    URL_DEF_CONST(AUTH, ANYSAFE),
#endif
    { NULL, 0 },
};

struct t_url_constant url_authtype[] =
{
#if LIBCURL_VERSION_NUM >= 0x071504
    /* libcurl >= 7.21.4 */
    URL_DEF_CONST(_TLSAUTH, NONE),
    URL_DEF_CONST(_TLSAUTH, SRP),
#endif
    { NULL, 0 },
};

struct t_url_constant url_postredir[] =
{
#if LIBCURL_VERSION_NUM >= 0x071301
    /* libcurl >= 7.19.1 */
    URL_DEF_CONST(_REDIR, POST_301),
    URL_DEF_CONST(_REDIR, POST_302),
#endif
    { NULL, 0 },
};

struct t_url_option url_options[] =
{
    /* behavior options */
#if LIBCURL_VERSION_NUM >= 0x070100
    /* libcurl >= 7.1 */
    URL_DEF_OPTION(HEADER,                LONG,   NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071501
    /* libcurl >= 7.21 */
    URL_DEF_OPTION(WILDCARDMATCH,         LONG,   NULL),
#endif
    /* error options */
#if LIBCURL_VERSION_NUM >= 0x070100
    /* libcurl >= 7.1 */
    URL_DEF_OPTION(FAILONERROR,           LONG,   NULL),
#endif
    /* network options */
#if LIBCURL_VERSION_NUM >= 0x071304
    /* libcurl >= 7.19.4 */
    URL_DEF_OPTION(PROTOCOLS,             MASK,   url_protocols),
    URL_DEF_OPTION(REDIR_PROTOCOLS,       MASK,   url_protocols),
#endif
#if LIBCURL_VERSION_NUM >= 0x070100
    /* libcurl >= 7.1 */
    URL_DEF_OPTION(PROXY,                 STRING, NULL),
    URL_DEF_OPTION(PROXYPORT,             LONG,   NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070A00
    /* libcurl >= 7.10 */
    URL_DEF_OPTION(PROXYTYPE,             LONG,   url_proxy_types),
#endif
#if LIBCURL_VERSION_NUM >= 0x071304
    /* libcurl >= 7.19.4 */
    URL_DEF_OPTION(NOPROXY,               STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070300
    /* libcurl >= 7.3 */
    URL_DEF_OPTION(HTTPPROXYTUNNEL,       LONG,   NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071304
    /* libcurl >= 7.19.4 */
    URL_DEF_OPTION(SOCKS5_GSSAPI_SERVICE, STRING, NULL),
    URL_DEF_OPTION(SOCKS5_GSSAPI_NEC,     LONG,   NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070300
    /* libcurl >= 7.3 */
    URL_DEF_OPTION(INTERFACE,             STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070F02
    /* libcurl >= 7.15.2 */
    URL_DEF_OPTION(LOCALPORT,             LONG,   NULL),
    URL_DEF_OPTION(LOCALPORTRANGE,        LONG,   NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070903
    /* libcurl >= 7.9.3 */
    URL_DEF_OPTION(DNS_CACHE_TIMEOUT,     LONG,   NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070A00
    /* libcurl >= 7.10 */
    URL_DEF_OPTION(BUFFERSIZE,            LONG,   NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070100
    /* libcurl >= 7.1 */
    URL_DEF_OPTION(PORT,                  LONG,   NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070B02
    /* libcurl >= 7.11.2 */
    URL_DEF_OPTION(TCP_NODELAY,           LONG,   NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071300
    /* libcurl >= 7.19.0 */
    URL_DEF_OPTION(ADDRESS_SCOPE,         LONG,   NULL),
#endif
    /* name and password options (authentication) */
#if LIBCURL_VERSION_NUM >= 0x070100
    /* libcurl >= 7.1 */
    URL_DEF_OPTION(NETRC,                 LONG,   url_netrc),
#endif
#if LIBCURL_VERSION_NUM >= 0x070B00
    /* libcurl >= 7.11.0 */
    URL_DEF_OPTION(NETRC_FILE,            STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070100
    /* libcurl >= 7.1 */
    URL_DEF_OPTION(USERPWD,               STRING, NULL),
    URL_DEF_OPTION(PROXYUSERPWD,          STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071301
    /* libcurl >= 7.19.1 */
    URL_DEF_OPTION(USERNAME,              STRING, NULL),
    URL_DEF_OPTION(PASSWORD,              STRING, NULL),
    URL_DEF_OPTION(PROXYUSERNAME,         STRING, NULL),
    URL_DEF_OPTION(PROXYPASSWORD,         STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070A06
    /* libcurl >= 7.10.6 */
    URL_DEF_OPTION(HTTPAUTH,              MASK,   url_auth),
#endif
#if LIBCURL_VERSION_NUM >= 0x071504
    /* libcurl >= 7.21.4 */
    URL_DEF_OPTION(TLSAUTH_TYPE,          MASK,   url_authtype),
    URL_DEF_OPTION(TLSAUTH_USERNAME,      STRING, NULL),
    URL_DEF_OPTION(TLSAUTH_PASSWORD,      STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070A07
    /* libcurl >= 7.10.7 */
    URL_DEF_OPTION(PROXYAUTH,             MASK,   url_auth),
#endif
    /* HTTP options */
#if LIBCURL_VERSION_NUM >= 0x070100
    /* libcurl >= 7.1 */
    URL_DEF_OPTION(AUTOREFERER,           LONG,   NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071506
    /* libcurl >= 7.15.6 */
    URL_DEF_OPTION(ACCEPT_ENCODING,       STRING, NULL),
    URL_DEF_OPTION(TRANSFER_ENCODING,     LONG,   NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070100
    /* libcurl >= 7.1 */
    URL_DEF_OPTION(FOLLOWLOCATION,        LONG,   NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070A04
    /* libcurl >= 7.10.4 */
    URL_DEF_OPTION(UNRESTRICTED_AUTH,     LONG,   NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070500
    /* libcurl >= 7.5 */
    URL_DEF_OPTION(MAXREDIRS,             LONG,   NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071301
    /* libcurl >= 7.19.1 */
    URL_DEF_OPTION(POSTREDIR,             MASK,   url_postredir),
#endif
#if LIBCURL_VERSION_NUM >= 0x070100
    /* libcurl >= 7.1 */
    URL_DEF_OPTION(POST,                  LONG,   NULL),
    URL_DEF_OPTION(POSTFIELDS,            STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070200
    /* libcurl >= 7.2 */
    URL_DEF_OPTION(POSTFIELDSIZE,         LONG,   NULL),
#endif
    { NULL, 0, 0, NULL },
};


/*
 * weeurl_search_constant: search a constant in array of constants
 */

int
weeurl_search_constant (struct t_url_constant *constants, const char *name)
{
    int i;

    for (i = 0; constants[i].name; i++)
    {
        if (string_strcasecmp (constants[i].name, name) == 0)
        {
            return i;
        }
    }

    /* constant not found */
    return -1;
}

/*
 * weeurl_get_mask_value: get value of mask using constants
 *                        string_mask has format: "const1+const2+const3"
 */

long
weeurl_get_mask_value (struct t_url_constant *constants,
                       const char *string_mask)
{
    char **items, *item;
    int num_items, i, index;
    long mask;

    mask = 0;

    items = string_split (string_mask, "+", 0, 0, &num_items);
    if (items)
    {
        for (i = 0; i < num_items; i++)
        {
            item = string_remove_quotes(items[i], "'\"");
            if (item)
            {
                index = weeurl_search_constant (constants, item);
                if (index >= 0)
                    mask |= constants[index].value;
                free (item);
            }
        }
        string_free_split (items);
    }

    return mask;
}

/*
 * weeurl_search_option: search an url option in table of options
 */

int
weeurl_search_option (const char *name)
{
    int i;

    for (i = 0; url_options[i].name; i++)
    {
        if (string_strcasecmp (url_options[i].name, name) == 0)
        {
            return i;
        }
    }

    /* option not found */
    return -1;
}

/*
 * weeurl_read: read callback for curl, used to read data from a file
 */

size_t
weeurl_read (void *buffer, size_t size, size_t nmemb, void *stream)
{
    return (stream) ? fread (buffer, size, nmemb, stream) : 0;
}

/*
 * weeurl_write: write callback for curl, used to write data in a file
 */

size_t
weeurl_write (void *buffer, size_t size, size_t nmemb, void *stream)
{
    return (stream) ? fwrite (buffer, size, nmemb, stream) : 0;
}

/*
 * weeurl_set_options: callback called for each option in hashtable "options",
 *                     it sets option in CURL easy handle
 */

void
weeurl_option_map_cb (void *data,
                      struct t_hashtable *hashtable,
                      const void *key, const void *value)
{
    CURL *curl;
    int index, index_constant, rc;
    long long_value;

    /* make C compiler happy */
    (void) hashtable;

    curl = (CURL *)data;
    if (!curl)
        return;

    index = weeurl_search_option ((const char *)key);
    if (index >= 0)
    {
        switch (url_options[index].type)
        {
            case URL_TYPE_STRING:
                curl_easy_setopt (curl, url_options[index].option,
                                  (const char *)value);
                break;
            case URL_TYPE_LONG:
                if (url_options[index].constants)
                {
                    index_constant = weeurl_search_constant (url_options[index].constants,
                                                             (const char *)value);
                    if (index_constant >= 0)
                    {
                        curl_easy_setopt (curl, url_options[index].option,
                                          url_options[index].constants[index_constant].value);
                    }
                }
                else
                {
                    rc = sscanf ((const char *)value, "%ld", &long_value);
                    if (rc != EOF)
                    {
                        curl_easy_setopt (curl, url_options[index].option,
                                          long_value);
                    }
                }
                break;
            case URL_TYPE_MASK:
                if (url_options[index].constants)
                {
                    long_value = weeurl_get_mask_value (url_options[index].constants,
                                                        (const char *)value);
                    curl_easy_setopt (curl, url_options[index].option,
                                      long_value);
                }
                break;
        }
    }

    return;
}

/*
 * weeurl_download: download URL, using options
 *                  return code:
 *                    0: ok
 *                    1: invalid url
 *                    2: error downloading url
 *                    3: memory error
 *                    4: file error
 */

int
weeurl_download (const char *url, struct t_hashtable *options)
{
    CURL *curl;
    struct t_url_file url_file[2];
    char *url_file_option[2] = { "file_in", "file_out" };
    char *url_file_mode[2] = { "rb", "wb" };
    CURLoption url_file_opt_func[2] = { CURLOPT_READFUNCTION, CURLOPT_WRITEFUNCTION };
    CURLoption url_file_opt_data[2] = { CURLOPT_READDATA, CURLOPT_WRITEDATA };
    void *url_file_opt_cb[2] = { &weeurl_read, &weeurl_write };
    int rc, i;

    rc = 0;

    for (i = 0; i < 2; i++)
    {
        url_file[i].filename = NULL;
        url_file[i].stream = NULL;
    }

    if (!url || !url[0])
    {
        rc = 1;
        goto end;
    }

    curl = curl_easy_init();
    if (!curl)
    {
        rc = 3;
        goto end;
    }

    /* set default options */
    curl_easy_setopt (curl, CURLOPT_URL, url);
    curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, "1");

    /* set file in/out from options in hashtable */
    if (options)
    {
        for (i = 0; i < 2; i++)
        {
            url_file[i].filename = hashtable_get (options, url_file_option[i]);
            if (url_file[i].filename)
            {
                url_file[i].stream = fopen (url_file[i].filename, url_file_mode[i]);
                if (!url_file[i].stream)
                {
                    rc = 4;
                    goto end;
                }
                curl_easy_setopt (curl, url_file_opt_func[i], url_file_opt_cb[i]);
                curl_easy_setopt (curl, url_file_opt_data[i], url_file[i].stream);
            }
        }
    }

    /* set other options in hashtable */
    hashtable_map (options, &weeurl_option_map_cb, curl);

    /* perform action! */
    if (curl_easy_perform (curl) != CURLE_OK)
        rc = 2;

    /* cleanup */
    curl_easy_cleanup (curl);

end:
    for (i = 0; i < 2; i++)
    {
        if (url_file[i].stream)
            fclose (url_file[i].stream);
    }
    return rc;
}

/*
 * weeurl_option_add_to_infolist: add an url option in an infolist
 *                                return 1 if ok, 0 if error
 */

int
weeurl_option_add_to_infolist (struct t_infolist *infolist,
                               struct t_url_option *option)
{
    struct t_infolist_item *ptr_item;
    char *constants;
    int i, length;

    if (!infolist || !option)
        return 0;

    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_string (ptr_item, "name", option->name))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "option", option->option))
        return 0;
    if (!infolist_new_var_string (ptr_item, "type", url_type_string[option->type]))
        return 0;
    if (option->constants)
    {
        length = 1;
        for (i = 0; option->constants[i].name; i++)
        {
            length += strlen (option->constants[i].name) + 1;
        }
        constants = malloc (length);
        if (constants)
        {
            constants[0] = '\0';
            for (i = 0; option->constants[i].name; i++)
            {
                if (i > 0)
                    strcat (constants, ",");
                strcat (constants, option->constants[i].name);
            }
            if (!infolist_new_var_string (ptr_item, "constants", constants))
            {
                free (constants);
                return 0;
            }
            free (constants);
        }
    }

    return 1;
}
