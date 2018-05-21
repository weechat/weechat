/*
 * wee-url.c - URL transfer
 *
 * Copyright (C) 2012-2018 Sébastien Helleu <flashcode@flashtux.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

#include "weechat.h"
#include "wee-url.h"
#include "wee-config.h"
#include "wee-hashtable.h"
#include "wee-infolist.h"
#include "wee-proxy.h"
#include "wee-string.h"


#define URL_DEF_CONST(__prefix, __name)                                 \
    { #__name, CURL##__prefix##_##__name }
#define URL_DEF_OPTION(__name, __type, __constants)                     \
    { #__name, CURLOPT_##__name, URL_TYPE_##__type, __constants }


char *url_type_string[] = { "string", "long", "long long", "mask", "list" };

/*
 * Constants/options for Curl 7.52.0
 * (this list of options must be updated on every new Curl release)
 */

struct t_url_constant url_proxy_types[] =
{
#if LIBCURL_VERSION_NUM >= 0x070A00 /* 7.10.0 */
    URL_DEF_CONST(PROXY, HTTP),
    URL_DEF_CONST(PROXY, SOCKS4),
    URL_DEF_CONST(PROXY, SOCKS5),
#endif
#if LIBCURL_VERSION_NUM >= 0x071200 /* 7.18.0 */
    URL_DEF_CONST(PROXY, SOCKS4A),
    URL_DEF_CONST(PROXY, SOCKS5_HOSTNAME),
#endif
#if LIBCURL_VERSION_NUM >= 0x071304 /* 7.19.4 */
    URL_DEF_CONST(PROXY, HTTP_1_0),
#endif
#if LIBCURL_VERSION_NUM >= 0x073400 /* 7.52.0 */
    URL_DEF_CONST(PROXY, HTTPS),
#endif
    { NULL, 0 },
};

struct t_url_constant url_protocols[] =
{
#if LIBCURL_VERSION_NUM >= 0x071304 /* 7.19.4 */
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
    URL_DEF_CONST(PROTO, ALL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071400 /* 7.20.0 */
    URL_DEF_CONST(PROTO, IMAP),
    URL_DEF_CONST(PROTO, IMAPS),
    URL_DEF_CONST(PROTO, POP3),
    URL_DEF_CONST(PROTO, POP3S),
    URL_DEF_CONST(PROTO, SMTP),
    URL_DEF_CONST(PROTO, SMTPS),
    URL_DEF_CONST(PROTO, RTSP),
#endif
#if LIBCURL_VERSION_NUM >= 0x071500 /* 7.21.0 */
    URL_DEF_CONST(PROTO, RTMP),
    URL_DEF_CONST(PROTO, RTMPT),
    URL_DEF_CONST(PROTO, RTMPE),
    URL_DEF_CONST(PROTO, RTMPTE),
    URL_DEF_CONST(PROTO, RTMPS),
    URL_DEF_CONST(PROTO, RTMPTS),
#endif
#if LIBCURL_VERSION_NUM >= 0x071502 /* 7.21.2 */
    URL_DEF_CONST(PROTO, GOPHER),
#endif
#if LIBCURL_VERSION_NUM >= 0x072800 /* 7.40.0 */
    URL_DEF_CONST(PROTO, SMB),
    URL_DEF_CONST(PROTO, SMBS),
#endif
    { NULL, 0 },
};

struct t_url_constant url_netrc[] =
{
#if LIBCURL_VERSION_NUM >= 0x070908 /* 7.9.8 */
    URL_DEF_CONST(_NETRC, IGNORED),
    URL_DEF_CONST(_NETRC, OPTIONAL),
    URL_DEF_CONST(_NETRC, REQUIRED),
#endif
    { NULL, 0 },
};

struct t_url_constant url_auth[] =
{
#if LIBCURL_VERSION_NUM >= 0x070A06 /* 7.10.6 */
    URL_DEF_CONST(AUTH, NONE),
    URL_DEF_CONST(AUTH, BASIC),
    URL_DEF_CONST(AUTH, DIGEST),
    URL_DEF_CONST(AUTH, NTLM),
    URL_DEF_CONST(AUTH, ANY),
    URL_DEF_CONST(AUTH, ANYSAFE),
#endif
#if LIBCURL_VERSION_NUM >= 0x071303 /* 7.19.3 */
    URL_DEF_CONST(AUTH, DIGEST_IE),
#endif
#if LIBCURL_VERSION_NUM >= 0x071503 /* 7.21.3 */
    URL_DEF_CONST(AUTH, ONLY),
#endif
#if LIBCURL_VERSION_NUM >= 0x071600 /* 7.22.0 */
    URL_DEF_CONST(AUTH, NTLM_WB),
#endif
#if LIBCURL_VERSION_NUM >= 0x070A06 && LIBCURL_VERSION_NUM < 0x072600 /* 7.10.6 - 7.38.0 */
    URL_DEF_CONST(AUTH, GSSNEGOTIATE),
#endif
#if LIBCURL_VERSION_NUM >= 0x072600 /* 7.38.0 */
    URL_DEF_CONST(AUTH, NEGOTIATE),
#endif
    { NULL, 0 },
};

struct t_url_constant url_authtype[] =
{
#if LIBCURL_VERSION_NUM >= 0x071504 /* 7.21.4 */
    URL_DEF_CONST(_TLSAUTH, NONE),
    URL_DEF_CONST(_TLSAUTH, SRP),
#endif
    { NULL, 0 },
};

struct t_url_constant url_postredir[] =
{
#if LIBCURL_VERSION_NUM >= 0x071301 /* 7.19.1 */
    URL_DEF_CONST(_REDIR, POST_301),
    URL_DEF_CONST(_REDIR, POST_302),
#endif
    { NULL, 0 },
};

struct t_url_constant url_http_version[] =
{
#if LIBCURL_VERSION_NUM >= 0x070901 /* 7.9.1 */
    URL_DEF_CONST(_HTTP_VERSION, NONE),
    URL_DEF_CONST(_HTTP_VERSION, 1_0),
    URL_DEF_CONST(_HTTP_VERSION, 1_1),
#endif
#if LIBCURL_VERSION_NUM >= 0x072100 /* 7.33.0 */
    URL_DEF_CONST(_HTTP_VERSION, 2_0),
#endif
#if LIBCURL_VERSION_NUM >= 0x072B00 /* 7.43.0 */
    URL_DEF_CONST(_HTTP_VERSION, 2),
#endif
#if LIBCURL_VERSION_NUM >= 0x072F00 /* 7.47.0 */
    URL_DEF_CONST(_HTTP_VERSION, 2TLS),
#endif
#if LIBCURL_VERSION_NUM >= 0x073100 /* 7.49.0 */
    URL_DEF_CONST(_HTTP_VERSION, 2_PRIOR_KNOWLEDGE),
#endif
    { NULL, 0 },
};

struct t_url_constant url_ftp_auth[] =
{
#if LIBCURL_VERSION_NUM >= 0x070C02 /* 7.12.2 */
    URL_DEF_CONST(FTPAUTH, DEFAULT),
    URL_DEF_CONST(FTPAUTH, SSL),
    URL_DEF_CONST(FTPAUTH, TLS),
#endif
    { NULL, 0 },
};

struct t_url_constant url_ftp_ssl_ccc[] =
{
#if LIBCURL_VERSION_NUM >= 0x071002 /* 7.16.2 */
    URL_DEF_CONST(FTPSSL, CCC_NONE),
    URL_DEF_CONST(FTPSSL, CCC_ACTIVE),
#endif
#if LIBCURL_VERSION_NUM >= 0x071001 /* 7.16.1 */
    URL_DEF_CONST(FTPSSL, CCC_PASSIVE),
#endif
    { NULL, 0 },
};

struct t_url_constant url_ftp_file_method[] =
{
#if LIBCURL_VERSION_NUM >= 0x070F03 /* 7.15.3 */
    URL_DEF_CONST(FTPMETHOD, MULTICWD),
    URL_DEF_CONST(FTPMETHOD, NOCWD),
    URL_DEF_CONST(FTPMETHOD, SINGLECWD),
#endif
    { NULL, 0 },
};

struct t_url_constant url_rtsp_request[] =
{
#if LIBCURL_VERSION_NUM >= 0x071400 /* 7.20.0 */
    URL_DEF_CONST(_RTSPREQ, OPTIONS),
    URL_DEF_CONST(_RTSPREQ, DESCRIBE),
    URL_DEF_CONST(_RTSPREQ, ANNOUNCE),
    URL_DEF_CONST(_RTSPREQ, SETUP),
    URL_DEF_CONST(_RTSPREQ, PLAY),
    URL_DEF_CONST(_RTSPREQ, PAUSE),
    URL_DEF_CONST(_RTSPREQ, TEARDOWN),
    URL_DEF_CONST(_RTSPREQ, GET_PARAMETER),
    URL_DEF_CONST(_RTSPREQ, SET_PARAMETER),
    URL_DEF_CONST(_RTSPREQ, RECORD),
    URL_DEF_CONST(_RTSPREQ, RECEIVE),
#endif
    { NULL, 0 },
};

struct t_url_constant url_time_condition[] =
{
#if LIBCURL_VERSION_NUM >= 0x070907 /* 7.9.7 */
    URL_DEF_CONST(_TIMECOND, NONE),
    URL_DEF_CONST(_TIMECOND, IFMODSINCE),
    URL_DEF_CONST(_TIMECOND, IFUNMODSINCE),
    URL_DEF_CONST(_TIMECOND, LASTMOD),
#endif
    { NULL, 0 },
};

struct t_url_constant url_ip_resolve[] =
{
#if LIBCURL_VERSION_NUM >= 0x070A08 /* 7.10.8 */
    URL_DEF_CONST(_IPRESOLVE, WHATEVER),
    URL_DEF_CONST(_IPRESOLVE, V4),
    URL_DEF_CONST(_IPRESOLVE, V6),
#endif
    { NULL, 0 },
};

struct t_url_constant url_use_ssl[] =
{
#if LIBCURL_VERSION_NUM >= 0x071100 /* 7.17.0 */
    URL_DEF_CONST(USESSL, NONE),
    URL_DEF_CONST(USESSL, TRY),
    URL_DEF_CONST(USESSL, CONTROL),
    URL_DEF_CONST(USESSL, ALL),
#endif
    { NULL, 0 },
};

struct t_url_constant url_ssl_version[] =
{
#if LIBCURL_VERSION_NUM >= 0x070902 /* 7.9.2 */
    URL_DEF_CONST(_SSLVERSION, DEFAULT),
    URL_DEF_CONST(_SSLVERSION, TLSv1),
    URL_DEF_CONST(_SSLVERSION, SSLv2),
    URL_DEF_CONST(_SSLVERSION, SSLv3),
#endif
#if LIBCURL_VERSION_NUM >= 0x072200 /* 7.34.0 */
    URL_DEF_CONST(_SSLVERSION, TLSv1_0),
    URL_DEF_CONST(_SSLVERSION, TLSv1_1),
    URL_DEF_CONST(_SSLVERSION, TLSv1_2),
#endif
#if LIBCURL_VERSION_NUM >= 0x073400 /* 7.52.0 */
    URL_DEF_CONST(_SSLVERSION, TLSv1_3),
#endif
    { NULL, 0 },
};

struct t_url_constant url_ssl_options[] =
{
#if LIBCURL_VERSION_NUM >= 0x071900 /* 7.25.0 */
    URL_DEF_CONST(SSLOPT, ALLOW_BEAST),
#endif
#if LIBCURL_VERSION_NUM >= 0x072C00 /* 7.44.0 */
    URL_DEF_CONST(SSLOPT, NO_REVOKE),
#endif
    { NULL, 0 },
};

struct t_url_constant url_gssapi_delegation[] =
{
#if LIBCURL_VERSION_NUM >= 0x071600 /* 7.22.0 */
    URL_DEF_CONST(GSSAPI_DELEGATION, NONE),
    URL_DEF_CONST(GSSAPI_DELEGATION, POLICY_FLAG),
    URL_DEF_CONST(GSSAPI_DELEGATION, FLAG),
#endif
    { NULL, 0 },
};

struct t_url_constant url_ssh_auth[] =
{
#if LIBCURL_VERSION_NUM >= 0x071001 /* 7.16.1 */
    URL_DEF_CONST(SSH_AUTH, NONE),
    URL_DEF_CONST(SSH_AUTH, PUBLICKEY),
    URL_DEF_CONST(SSH_AUTH, PASSWORD),
    URL_DEF_CONST(SSH_AUTH, HOST),
    URL_DEF_CONST(SSH_AUTH, KEYBOARD),
    URL_DEF_CONST(SSH_AUTH, DEFAULT),
    URL_DEF_CONST(SSH_AUTH, ANY),
#endif
#if LIBCURL_VERSION_NUM >= 0x071C00 /* 7.28.0 */
    URL_DEF_CONST(SSH_AUTH, AGENT),
#endif
    { NULL, 0 },
};

struct t_url_constant url_header[] =
{
#if LIBCURL_VERSION_NUM >= 0x072500 /* 7.37.0 */
    URL_DEF_CONST(HEADER, UNIFIED),
    URL_DEF_CONST(HEADER, SEPARATE),
#endif
    { NULL, 0 },
};

struct t_url_option url_options[] =
{
    /*
     * behavior options
     */
#if LIBCURL_VERSION_NUM >= 0x070100 /* 7.1.0 */
    URL_DEF_OPTION(VERBOSE, LONG, NULL),
    URL_DEF_OPTION(HEADER, LONG, NULL),
    URL_DEF_OPTION(NOPROGRESS, LONG, NULL),
#endif
    #if LIBCURL_VERSION_NUM >= 0x070A00 /* 7.10.0 */
    URL_DEF_OPTION(NOSIGNAL, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071500 /* 7.21.0 */
    URL_DEF_OPTION(WILDCARDMATCH, LONG, NULL),
#endif

    /*
     * error options
     */
#if LIBCURL_VERSION_NUM >= 0x070100 /* 7.1.0 */
    URL_DEF_OPTION(FAILONERROR, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x073300 /* 7.51.0 */
    URL_DEF_OPTION(KEEP_SENDING_ON_ERROR, LONG, NULL),
#endif

    /*
     * network options
     */
#if LIBCURL_VERSION_NUM >= 0x070100 /* 7.1.0 */
    URL_DEF_OPTION(PROXY, STRING, NULL),
    URL_DEF_OPTION(PROXYPORT, LONG, NULL),
    URL_DEF_OPTION(PORT, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x073400 /* 7.52.0 */
    URL_DEF_OPTION(PRE_PROXY, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070300 /* 7.3.0 */
    URL_DEF_OPTION(HTTPPROXYTUNNEL, LONG, NULL),
    URL_DEF_OPTION(INTERFACE, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070903 /* 7.9.3 */
    URL_DEF_OPTION(DNS_CACHE_TIMEOUT, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070A00 /* 7.10.0 */
    URL_DEF_OPTION(PROXYTYPE, LONG, url_proxy_types),
    URL_DEF_OPTION(BUFFERSIZE, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070B02 /* 7.11.2 */
    URL_DEF_OPTION(TCP_NODELAY, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070F02 /* 7.15.2 */
    URL_DEF_OPTION(LOCALPORT, LONG, NULL),
    URL_DEF_OPTION(LOCALPORTRANGE, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071300 /* 7.19.0 */
    URL_DEF_OPTION(ADDRESS_SCOPE, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071304 /* 7.19.4 */
    URL_DEF_OPTION(PROTOCOLS, MASK, url_protocols),
    URL_DEF_OPTION(REDIR_PROTOCOLS, MASK, url_protocols),
    URL_DEF_OPTION(NOPROXY, STRING, NULL),
    URL_DEF_OPTION(SOCKS5_GSSAPI_NEC, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071900 /* 7.25.0 */
    URL_DEF_OPTION(TCP_KEEPALIVE, LONG, NULL),
    URL_DEF_OPTION(TCP_KEEPIDLE, LONG, NULL),
    URL_DEF_OPTION(TCP_KEEPINTVL, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x072800 /* 7.40.0 */
    URL_DEF_OPTION(UNIX_SOCKET_PATH, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x072A00 /* 7.42.0 */
    URL_DEF_OPTION(PATH_AS_IS, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x072B00 /* 7.43.0 */
    URL_DEF_OPTION(PROXY_SERVICE_NAME, STRING, NULL),
    URL_DEF_OPTION(SERVICE_NAME, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x072D00 /* 7.45.0 */
    URL_DEF_OPTION(DEFAULT_PROTOCOL, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071304 && LIBCURL_VERSION_NUM < 0x073100 /* 7.19.4 - 7.49.0 */
    URL_DEF_OPTION(SOCKS5_GSSAPI_SERVICE, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x073100 /* 7.49.0 */
    URL_DEF_OPTION(TCP_FASTOPEN, LONG, NULL),
#endif

    /*
     * name and password options (authentication)
     */
#if LIBCURL_VERSION_NUM >= 0x070100 /* 7.1.0 */
    URL_DEF_OPTION(NETRC, LONG, url_netrc),
    URL_DEF_OPTION(USERPWD, STRING, NULL),
    URL_DEF_OPTION(PROXYUSERPWD, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070A06 /* 7.10.6 */
    URL_DEF_OPTION(HTTPAUTH, MASK, url_auth),
#endif
#if LIBCURL_VERSION_NUM >= 0x070A07 /* 7.10.7 */
    URL_DEF_OPTION(PROXYAUTH, MASK, url_auth),
#endif
#if LIBCURL_VERSION_NUM >= 0x070B00 /* 7.11.0 */
    URL_DEF_OPTION(NETRC_FILE, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071301 /* 7.19.1 */
    URL_DEF_OPTION(USERNAME, STRING, NULL),
    URL_DEF_OPTION(PASSWORD, STRING, NULL),
    URL_DEF_OPTION(PROXYUSERNAME, STRING, NULL),
    URL_DEF_OPTION(PROXYPASSWORD, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071504 /* 7.21.4 */
    URL_DEF_OPTION(TLSAUTH_TYPE, MASK, url_authtype),
    URL_DEF_OPTION(TLSAUTH_USERNAME, STRING, NULL),
    URL_DEF_OPTION(TLSAUTH_PASSWORD, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071F00 /* 7.31.0 */
    URL_DEF_OPTION(SASL_IR, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x072100 /* 7.33.0 */
    URL_DEF_OPTION(XOAUTH2_BEARER, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x072200 /* 7.34.0 */
    URL_DEF_OPTION(LOGIN_OPTIONS, STRING, NULL),
#endif

    /*
     * HTTP options
     */
#if LIBCURL_VERSION_NUM >= 0x070100 /* 7.1.0 */
    URL_DEF_OPTION(AUTOREFERER, LONG, NULL),
    URL_DEF_OPTION(FOLLOWLOCATION, LONG, NULL),
    URL_DEF_OPTION(PUT, LONG, NULL),
    URL_DEF_OPTION(POST, LONG, NULL),
    URL_DEF_OPTION(POSTFIELDS, STRING, NULL),
    URL_DEF_OPTION(HTTPPOST, LIST, NULL),
    URL_DEF_OPTION(REFERER, STRING, NULL),
    URL_DEF_OPTION(USERAGENT, STRING, NULL),
    URL_DEF_OPTION(HTTPHEADER, LIST, NULL),
    URL_DEF_OPTION(COOKIE, STRING, NULL),
    URL_DEF_OPTION(COOKIEFILE, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070200 /* 7.2.0 */
    URL_DEF_OPTION(POSTFIELDSIZE, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070500 /* 7.5.0 */
    URL_DEF_OPTION(MAXREDIRS, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070801 /* 7.8.1 */
    URL_DEF_OPTION(HTTPGET, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070900 /* 7.9.0 */
    URL_DEF_OPTION(COOKIEJAR, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070901 /* 7.9.1 */
    URL_DEF_OPTION(HTTP_VERSION, LONG, url_http_version),
#endif
#if LIBCURL_VERSION_NUM >= 0x070907 /* 7.9.7 */
    URL_DEF_OPTION(COOKIESESSION, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070A03 /* 7.10.3 */
    URL_DEF_OPTION(HTTP200ALIASES, LIST, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070A04 /* 7.10.4 */
    URL_DEF_OPTION(UNRESTRICTED_AUTH, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070B01 /* 7.11.1 */
    URL_DEF_OPTION(POSTFIELDSIZE_LARGE, LONGLONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070E01 /* 7.14.1 */
    URL_DEF_OPTION(COOKIELIST, STRING, NULL),
    URL_DEF_OPTION(IGNORE_CONTENT_LENGTH, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071506 /* 7.15.6 */
    URL_DEF_OPTION(ACCEPT_ENCODING, STRING, NULL),
    URL_DEF_OPTION(TRANSFER_ENCODING, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071002 /* 7.16.2 */
    URL_DEF_OPTION(HTTP_CONTENT_DECODING, LONG, NULL),
    URL_DEF_OPTION(HTTP_TRANSFER_DECODING, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071101 /* 7.17.1 */
    URL_DEF_OPTION(COPYPOSTFIELDS, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071301 /* 7.19.1 */
    URL_DEF_OPTION(POSTREDIR, MASK, url_postredir),
#endif
#if LIBCURL_VERSION_NUM >= 0x072400 /* 7.36.0 */
    URL_DEF_OPTION(EXPECT_100_TIMEOUT_MS, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x072500 /* 7.37.0 */
    URL_DEF_OPTION(HEADEROPT, MASK, url_header),
    URL_DEF_OPTION(PROXYHEADER, LIST, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x072B00 /* 7.43.0 */
    URL_DEF_OPTION(PIPEWAIT, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x072E00 /* 7.46.0 */
    URL_DEF_OPTION(STREAM_WEIGHT, LONG, NULL),
#endif

    /*
     * SMTP options
     */
#if LIBCURL_VERSION_NUM >= 0x071400 /* 7.20.0 */
    URL_DEF_OPTION(MAIL_FROM, STRING, NULL),
    URL_DEF_OPTION(MAIL_RCPT, LIST, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071900 /* 7.25.0 */
    URL_DEF_OPTION(MAIL_AUTH, STRING, NULL),
#endif

    /*
     * TFTP options
     */
#if LIBCURL_VERSION_NUM >= 0x071304 /* 7.19.4 */
    URL_DEF_OPTION(TFTP_BLKSIZE, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x073000 /* 7.48.0 */
    URL_DEF_OPTION(TFTP_NO_OPTIONS, LONG, NULL),
#endif

    /*
     * FTP options
     */
#if LIBCURL_VERSION_NUM >= 0x070100 /* 7.1.0 */
    URL_DEF_OPTION(FTPPORT, STRING, NULL),
    URL_DEF_OPTION(QUOTE, LIST, NULL),
    URL_DEF_OPTION(POSTQUOTE, LIST, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070902 /* 7.9.2 */
    URL_DEF_OPTION(FTP_USE_EPSV, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070905 /* 7.9.5 */
    URL_DEF_OPTION(PREQUOTE, LIST, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070A05 /* 7.10.5 */
    URL_DEF_OPTION(FTP_USE_EPRT, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070A07 /* 7.10.7 */
    URL_DEF_OPTION(FTP_CREATE_MISSING_DIRS, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070A08 /* 7.10.8 */
    URL_DEF_OPTION(FTP_RESPONSE_TIMEOUT, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070C02 /* 7.12.2 */
    URL_DEF_OPTION(FTPSSLAUTH, LONG, url_ftp_auth),
#endif
#if LIBCURL_VERSION_NUM >= 0x070D00 /* 7.13.0 */
    URL_DEF_OPTION(FTP_ACCOUNT, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070F00 /* 7.15.0 */
    URL_DEF_OPTION(FTP_SKIP_PASV_IP, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070F01 /* 7.15.1 */
    URL_DEF_OPTION(FTP_FILEMETHOD, LONG, url_ftp_file_method),
#endif
#if LIBCURL_VERSION_NUM >= 0x070F05 /* 7.15.5 */
    URL_DEF_OPTION(FTP_ALTERNATIVE_TO_USER, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071001 /* 7.16.1 */
    URL_DEF_OPTION(FTP_SSL_CCC, LONG, url_ftp_ssl_ccc),
#endif
#if LIBCURL_VERSION_NUM >= 0x071100 /* 7.17.0 */
    URL_DEF_OPTION(DIRLISTONLY, LONG, NULL),
    URL_DEF_OPTION(APPEND, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071400 /* 7.20.0 */
    URL_DEF_OPTION(FTP_USE_PRET, LONG, NULL),
#endif

    /*
     * RTSP options
     */
#if LIBCURL_VERSION_NUM >= 0x071400 /* 7.20.0 */
    URL_DEF_OPTION(RTSP_REQUEST, LONG, url_rtsp_request),
    URL_DEF_OPTION(RTSP_SESSION_ID, STRING, NULL),
    URL_DEF_OPTION(RTSP_STREAM_URI, STRING, NULL),
    URL_DEF_OPTION(RTSP_TRANSPORT, STRING, NULL),
    URL_DEF_OPTION(RTSP_CLIENT_CSEQ, LONG, NULL),
    URL_DEF_OPTION(RTSP_SERVER_CSEQ, LONG, NULL),
#endif

    /*
     * protocol options
     */
#if LIBCURL_VERSION_NUM >= 0x070100 /* 7.1.0 */
    URL_DEF_OPTION(CRLF, LONG, NULL),
    URL_DEF_OPTION(RANGE, STRING, NULL),
    URL_DEF_OPTION(RESUME_FROM, LONG, NULL),
    URL_DEF_OPTION(CUSTOMREQUEST, STRING, NULL),
    URL_DEF_OPTION(NOBODY, LONG, NULL),
    URL_DEF_OPTION(INFILESIZE, LONG, NULL),
    URL_DEF_OPTION(UPLOAD, LONG, NULL),
    URL_DEF_OPTION(TIMECONDITION, LONG, url_time_condition),
    URL_DEF_OPTION(TIMEVALUE, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070101 /* 7.1.1 */
    URL_DEF_OPTION(TRANSFERTEXT, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070500 /* 7.5.0 */
    URL_DEF_OPTION(FILETIME, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070A08 /* 7.10.8 */
    URL_DEF_OPTION(MAXFILESIZE, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071200 /* 7.18.0 */
    URL_DEF_OPTION(PROXY_TRANSFER_MODE, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070B00 /* 7.11.0 */
    URL_DEF_OPTION(RESUME_FROM_LARGE, LONGLONG, NULL),
    URL_DEF_OPTION(INFILESIZE_LARGE, LONGLONG, NULL),
    URL_DEF_OPTION(MAXFILESIZE_LARGE, LONGLONG, NULL),
#endif

    /*
     * connection options
     */
#if LIBCURL_VERSION_NUM >= 0x070100 /* 7.1.0 */
    URL_DEF_OPTION(TIMEOUT, LONG, NULL),
    URL_DEF_OPTION(LOW_SPEED_LIMIT, LONG, NULL),
    URL_DEF_OPTION(LOW_SPEED_TIME, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070700 /* 7.7.0 */
    URL_DEF_OPTION(FRESH_CONNECT, LONG, NULL),
    URL_DEF_OPTION(FORBID_REUSE, LONG, NULL),
    URL_DEF_OPTION(CONNECTTIMEOUT, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070A08 /* 7.10.8 */
    URL_DEF_OPTION(IPRESOLVE, LONG, url_ip_resolve),
#endif
#if LIBCURL_VERSION_NUM >= 0x070F02 /* 7.15.2 */
    URL_DEF_OPTION(CONNECT_ONLY, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070F05 /* 7.15.5 */
    URL_DEF_OPTION(MAX_SEND_SPEED_LARGE, LONGLONG, NULL),
    URL_DEF_OPTION(MAX_RECV_SPEED_LARGE, LONGLONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071002 /* 7.16.2 */
    URL_DEF_OPTION(TIMEOUT_MS, LONG, NULL),
    URL_DEF_OPTION(CONNECTTIMEOUT_MS, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071003 /* 7.16.3 */
    URL_DEF_OPTION(MAXCONNECTS, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071100 /* 7.17.0 */
    URL_DEF_OPTION(USE_SSL, LONG, url_use_ssl),
#endif
#if LIBCURL_VERSION_NUM >= 0x071503 /* 7.21.3 */
    URL_DEF_OPTION(RESOLVE, LIST, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071800 /* 7.24.0 */
    URL_DEF_OPTION(DNS_SERVERS, STRING, NULL),
    URL_DEF_OPTION(ACCEPTTIMEOUT_MS, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x072100 /* 7.33.0 */
    URL_DEF_OPTION(DNS_INTERFACE, STRING, NULL),
    URL_DEF_OPTION(DNS_LOCAL_IP4, STRING, NULL),
    URL_DEF_OPTION(DNS_LOCAL_IP6, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x073100 /* 7.49.0 */
    URL_DEF_OPTION(CONNECT_TO, LIST, NULL),
#endif

    /*
     * SSL and security options
     */
#if LIBCURL_VERSION_NUM >= 0x070100 /* 7.1.0 */
    URL_DEF_OPTION(SSLCERT, STRING, NULL),
    URL_DEF_OPTION(SSLVERSION, LONG, url_ssl_version),
#endif
#if LIBCURL_VERSION_NUM >= 0x070402 /* 7.4.2 */
    URL_DEF_OPTION(SSL_VERIFYPEER, LONG, NULL),
    URL_DEF_OPTION(CAINFO, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070700 /* 7.7.0 */
    URL_DEF_OPTION(RANDOM_FILE, STRING, NULL),
    URL_DEF_OPTION(EGDSOCKET, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070801 /* 7.8.1 */
    URL_DEF_OPTION(SSL_VERIFYHOST, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070900 /* 7.9.0 */
    URL_DEF_OPTION(SSL_CIPHER_LIST, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070903 /* 7.9.3 */
    URL_DEF_OPTION(SSLCERTTYPE, STRING, NULL),
    URL_DEF_OPTION(SSLKEY, STRING, NULL),
    URL_DEF_OPTION(SSLKEYTYPE, STRING, NULL),
    URL_DEF_OPTION(SSLENGINE, STRING, NULL),
    URL_DEF_OPTION(SSLENGINE_DEFAULT, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x070908 /* 7.9.8 */
    URL_DEF_OPTION(CAPATH, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071000 /* 7.16.0 */
    URL_DEF_OPTION(SSL_SESSIONID_CACHE, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071004 /* 7.16.4 */
    URL_DEF_OPTION(KRBLEVEL, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071100 /* 7.17.0 */
    URL_DEF_OPTION(KEYPASSWD, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071300 /* 7.19.0 */
    URL_DEF_OPTION(ISSUERCERT, STRING, NULL),
    URL_DEF_OPTION(CRLFILE, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071301 /* 7.19.1 */
    URL_DEF_OPTION(CERTINFO, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071600 /* 7.22.0 */
    URL_DEF_OPTION(GSSAPI_DELEGATION, LONG, url_gssapi_delegation),
#endif
#if LIBCURL_VERSION_NUM >= 0x071900 /* 7.25.0 */
    URL_DEF_OPTION(SSL_OPTIONS, LONG, url_ssl_options),
#endif
#if LIBCURL_VERSION_NUM >= 0x072400 /* 7.36.0 */
    URL_DEF_OPTION(SSL_ENABLE_ALPN, LONG, NULL),
    URL_DEF_OPTION(SSL_ENABLE_NPN, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x072700 /* 7.39.0 */
    URL_DEF_OPTION(PINNEDPUBLICKEY, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x072900 /* 7.41.0 */
    URL_DEF_OPTION(SSL_VERIFYSTATUS, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x072A00 /* 7.42.0 */
    URL_DEF_OPTION(SSL_FALSESTART, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x073400 /* 7.52.0 */
    URL_DEF_OPTION(PROXY_CAINFO, STRING, NULL),
    URL_DEF_OPTION(PROXY_CAPATH, STRING, NULL),
    URL_DEF_OPTION(PROXY_CRLFILE, STRING, NULL),
    URL_DEF_OPTION(PROXY_KEYPASSWD, STRING, NULL),
    URL_DEF_OPTION(PROXY_PINNEDPUBLICKEY, STRING, NULL),
    URL_DEF_OPTION(PROXY_SSLCERT, STRING, NULL),
    URL_DEF_OPTION(PROXY_SSLCERTTYPE, STRING, NULL),
    URL_DEF_OPTION(PROXY_SSLKEY, STRING, NULL),
    URL_DEF_OPTION(PROXY_SSLKEYTYPE, STRING, NULL),
    URL_DEF_OPTION(PROXY_SSLVERSION, LONG, url_ssl_version),
    /*URL_DEF_OPTION(PROXY_SSL_CIPHER_LIST, LIST, NULL),*/
    URL_DEF_OPTION(PROXY_SSL_OPTIONS, LONG, url_ssl_options),
    URL_DEF_OPTION(PROXY_SSL_VERIFYHOST, LONG, NULL),
    URL_DEF_OPTION(PROXY_SSL_VERIFYPEER, LONG, NULL),
    URL_DEF_OPTION(PROXY_TLSAUTH_PASSWORD, STRING, NULL),
    URL_DEF_OPTION(PROXY_TLSAUTH_TYPE, STRING, NULL),
    URL_DEF_OPTION(PROXY_TLSAUTH_USERNAME, STRING, NULL),
#endif

    /*
     * SSH options
     */
#if LIBCURL_VERSION_NUM >= 0x071001 /* 7.16.1 */
    URL_DEF_OPTION(SSH_AUTH_TYPES, MASK, url_gssapi_delegation),
    URL_DEF_OPTION(SSH_PUBLIC_KEYFILE, STRING, NULL),
    URL_DEF_OPTION(SSH_PRIVATE_KEYFILE, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071101 /* 7.17.1 */
    URL_DEF_OPTION(SSH_HOST_PUBLIC_KEY_MD5, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x071306 /* 7.19.6 */
    URL_DEF_OPTION(SSH_KNOWNHOSTS, STRING, NULL),
#endif

    /*
     * other options
     */
#if LIBCURL_VERSION_NUM >= 0x071004 /* 7.16.4 */
    URL_DEF_OPTION(NEW_FILE_PERMS, LONG, NULL),
    URL_DEF_OPTION(NEW_DIRECTORY_PERMS, LONG, NULL),
#endif

    /*
     * telnet options
     */
#if LIBCURL_VERSION_NUM >= 0x070700 /* 7.7 */
    URL_DEF_OPTION(TELNETOPTIONS, LIST, NULL),
#endif
    { NULL, 0, 0, NULL },
};

char url_error[CURL_ERROR_SIZE + 1];


/*
 * Searches for a constant in array of constants.
 *
 * Returns index of constant, -1 if not found.
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
 * Gets value of mask using constants.
 *
 * Argument "string_mask" has format: "const1+const2+const3".
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
            item = string_remove_quotes (items[i], "'\"");
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
 * Searches for an URL option in table of options.
 *
 * Returns index of option, -1 if not found.
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
 * Reads data from a file (callback called to read a file).
 */

size_t
weeurl_read (void *buffer, size_t size, size_t nmemb, void *stream)
{
    return (stream) ? fread (buffer, size, nmemb, stream) : 0;
}

/*
 * Writes data in a file (callback called to write a file).
 */

size_t
weeurl_write (void *buffer, size_t size, size_t nmemb, void *stream)
{
    return (stream) ? fwrite (buffer, size, nmemb, stream) : 0;
}

/*
 * Sets option in CURL easy handle (callback called for each option in hashtable
 * "options").
 */

void
weeurl_option_map_cb (void *data,
                      struct t_hashtable *hashtable,
                      const void *key, const void *value)
{
    CURL *curl;
    int index, index_constant, rc;
    long long_value;
    long long long_long_value;
    struct curl_slist *slist;
    char *list_copy, *list_entry_walker;
    const char *list_entry_start;

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
            case URL_TYPE_LONGLONG:
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
                    rc = sscanf ((const char *)value, "%lld", &long_long_value);
                    if (rc != EOF)
                    {
                        curl_easy_setopt (curl, url_options[index].option,
                                          (curl_off_t)long_long_value);
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
            case URL_TYPE_LIST:
                slist = NULL;
                list_copy = malloc (strlen ((const char *)value) + 1);
                if (list_copy)
                {
                    strcpy (list_copy, (const char *)value);
                    list_entry_start = list_copy;
                    list_entry_walker = list_copy;
                    while (*list_entry_walker)
                    {
                        if (*list_entry_walker == '\n')
                        {
                            *list_entry_walker = '\0';
                            slist = curl_slist_append (slist, list_entry_start);
                            list_entry_start = list_entry_walker + 1;
                        }
                        ++list_entry_walker;
                    }
                    slist = curl_slist_append (slist, list_entry_start);
                    free (list_copy);
                    curl_easy_setopt (curl, url_options[index].option,
                                      slist);
                }
                break;
        }
    }
}

/*
 * Sets proxy in CURL easy handle.
 */

void
weeurl_set_proxy (CURL *curl, struct t_proxy *proxy)
{
    if (!proxy)
        return;

    /* set proxy type */
    switch (CONFIG_INTEGER(proxy->options[PROXY_OPTION_TYPE]))
    {
        case PROXY_TYPE_HTTP:
            curl_easy_setopt (curl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
            break;
        case PROXY_TYPE_SOCKS4:
#if LIBCURL_VERSION_NUM < 0x070A00 /* 7.10.0 */
            /* proxy socks4 not supported in Curl < 7.10 */
            return;
#endif /* LIBCURL_VERSION_NUM < 0x070A00 */
            curl_easy_setopt (curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS4);
            break;
        case PROXY_TYPE_SOCKS5:
#if LIBCURL_VERSION_NUM < 0x070A00 /* 7.10.0 */
            /* proxy socks5 not supported in Curl < 7.10 */
            return;
#endif /* LIBCURL_VERSION_NUM < 0x070A00 */
#if LIBCURL_VERSION_NUM >= 0x071200 /* 7.18.0 */
            curl_easy_setopt (curl, CURLOPT_PROXYTYPE,
                              CURLPROXY_SOCKS5_HOSTNAME);
#else
            curl_easy_setopt (curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5);
#endif /* LIBCURL_VERSION_NUM >= 0x071200 */
            break;
    }

    /* set proxy address */
    curl_easy_setopt (curl, CURLOPT_PROXY,
                      CONFIG_STRING(proxy->options[PROXY_OPTION_ADDRESS]));

    /* set proxy port */
    curl_easy_setopt (curl, CURLOPT_PROXYPORT,
                      CONFIG_INTEGER(proxy->options[PROXY_OPTION_PORT]));

    /* set username/password */
#if LIBCURL_VERSION_NUM >= 0x071301 /* 7.19.1 */
    if (CONFIG_STRING(proxy->options[PROXY_OPTION_USERNAME])
        && CONFIG_STRING(proxy->options[PROXY_OPTION_USERNAME])[0])
    {
        curl_easy_setopt (curl, CURLOPT_PROXYUSERNAME,
                          CONFIG_STRING(proxy->options[PROXY_OPTION_USERNAME]));
    }
    if (CONFIG_STRING(proxy->options[PROXY_OPTION_PASSWORD])
        && CONFIG_STRING(proxy->options[PROXY_OPTION_PASSWORD])[0])
    {
        curl_easy_setopt (curl, CURLOPT_PROXYPASSWORD,
                          CONFIG_STRING(proxy->options[PROXY_OPTION_PASSWORD]));
    }
#endif /* LIBCURL_VERSION_NUM >= 0x071301 */
}

/*
 * Downloads URL using options.
 *
 * Returns:
 *   0: OK
 *   1: invalid URL
 *   2: error downloading URL
 *   3: not enough memory
 *   4: file error
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
    struct t_proxy *ptr_proxy;
    int rc, curl_rc, i;

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

    curl = curl_easy_init ();
    if (!curl)
    {
        rc = 3;
        goto end;
    }

    /* set default options */
    curl_easy_setopt (curl, CURLOPT_URL, url);
    curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1L);

    /* set proxy (if option weechat.network.proxy_curl is set) */
    if (CONFIG_STRING(config_network_proxy_curl)
        && CONFIG_STRING(config_network_proxy_curl)[0])
    {
        ptr_proxy = proxy_search (CONFIG_STRING(config_network_proxy_curl));
        if (ptr_proxy)
            weeurl_set_proxy (curl, ptr_proxy);
    }

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

    /* set error buffer */
    curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, url_error);

    /* perform action! */
    curl_rc = curl_easy_perform (curl);
    if (curl_rc != CURLE_OK)
    {
        fprintf (stderr,
                 _("curl error %d (%s) (URL: \"%s\")\n"),
                 curl_rc, url_error, url);
        rc = 2;
    }

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
 * Adds an URL option in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
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
