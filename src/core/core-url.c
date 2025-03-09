/*
 * core-url.c - URL transfer
 *
 * Copyright (C) 2012-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

#include "weechat.h"
#include "core-url.h"
#include "core-config.h"
#include "core-hashtable.h"
#include "core-infolist.h"
#include "core-proxy.h"
#include "core-string.h"
#include "../plugins/plugin.h"


#define URL_DEF_CONST(__prefix, __name)                                 \
    { #__name, CURL##__prefix##_##__name }
#define URL_DEF_OPTION(__name, __type, __constants)                     \
    { #__name, CURLOPT_##__name, URL_TYPE_##__type, __constants }


int url_debug = 0;
char *url_type_string[] = { "string", "long", "long long", "mask", "list" };

/*
 * Constants/options for Curl 7.87.0
 * (this list of options must be updated on every new Curl release)
 */

struct t_url_constant url_proxy_types[] =
{
    URL_DEF_CONST(PROXY, HTTP),
    URL_DEF_CONST(PROXY, SOCKS4),
    URL_DEF_CONST(PROXY, SOCKS5),
    URL_DEF_CONST(PROXY, SOCKS4A),
    URL_DEF_CONST(PROXY, SOCKS5_HOSTNAME),
    URL_DEF_CONST(PROXY, HTTP_1_0),
#if LIBCURL_VERSION_NUM >= 0x073400 /* 7.52.0 */
    URL_DEF_CONST(PROXY, HTTPS),
#endif
    { NULL, 0 },
};

struct t_url_constant url_protocols[] =
{
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
    URL_DEF_CONST(PROTO, IMAP),
    URL_DEF_CONST(PROTO, IMAPS),
    URL_DEF_CONST(PROTO, POP3),
    URL_DEF_CONST(PROTO, POP3S),
    URL_DEF_CONST(PROTO, SMTP),
    URL_DEF_CONST(PROTO, SMTPS),
    URL_DEF_CONST(PROTO, RTSP),
    URL_DEF_CONST(PROTO, RTMP),
    URL_DEF_CONST(PROTO, RTMPT),
    URL_DEF_CONST(PROTO, RTMPE),
    URL_DEF_CONST(PROTO, RTMPTE),
    URL_DEF_CONST(PROTO, RTMPS),
    URL_DEF_CONST(PROTO, RTMPTS),
    URL_DEF_CONST(PROTO, GOPHER),
    URL_DEF_CONST(PROTO, SMB),
    URL_DEF_CONST(PROTO, SMBS),
#if LIBCURL_VERSION_NUM >= 0x074700 /* 7.71.0 */
    URL_DEF_CONST(PROTO, MQTT),
#endif
#if LIBCURL_VERSION_NUM >= 0x074B00 /* 7.75.0 */
    URL_DEF_CONST(PROTO, GOPHERS),
#endif
    { NULL, 0 },
};

struct t_url_constant url_netrc[] =
{
    URL_DEF_CONST(_NETRC, IGNORED),
    URL_DEF_CONST(_NETRC, OPTIONAL),
    URL_DEF_CONST(_NETRC, REQUIRED),
    { NULL, 0 },
};

struct t_url_constant url_auth[] =
{
    URL_DEF_CONST(AUTH, NONE),
    URL_DEF_CONST(AUTH, BASIC),
    URL_DEF_CONST(AUTH, DIGEST),
    URL_DEF_CONST(AUTH, NTLM),
    URL_DEF_CONST(AUTH, ANY),
    URL_DEF_CONST(AUTH, ANYSAFE),
    URL_DEF_CONST(AUTH, DIGEST_IE),
    URL_DEF_CONST(AUTH, ONLY),
#if LIBCURL_VERSION_NUM < 0x080800 /* < 8.8.0 */
    URL_DEF_CONST(AUTH, NTLM_WB),
#endif
    URL_DEF_CONST(AUTH, NEGOTIATE),
#if LIBCURL_VERSION_NUM >= 0x073700 /* 7.55.0 */
    URL_DEF_CONST(AUTH, GSSAPI),
#endif
#if LIBCURL_VERSION_NUM >= 0x073D00 /* 7.61.0 */
    URL_DEF_CONST(AUTH, BEARER),
#endif
#if LIBCURL_VERSION_NUM >= 0x074B00 /* 7.75.0 */
    URL_DEF_CONST(AUTH, AWS_SIGV4),
#endif
    { NULL, 0 },
};

struct t_url_constant url_authtype[] =
{
    URL_DEF_CONST(_TLSAUTH, NONE),
    URL_DEF_CONST(_TLSAUTH, SRP),
    { NULL, 0 },
};

struct t_url_constant url_postredir[] =
{
    URL_DEF_CONST(_REDIR, POST_301),
    URL_DEF_CONST(_REDIR, POST_302),
    { NULL, 0 },
};

struct t_url_constant url_http_version[] =
{
    URL_DEF_CONST(_HTTP_VERSION, NONE),
    URL_DEF_CONST(_HTTP_VERSION, 1_0),
    URL_DEF_CONST(_HTTP_VERSION, 1_1),
    URL_DEF_CONST(_HTTP_VERSION, 2_0),
    URL_DEF_CONST(_HTTP_VERSION, 2),
    URL_DEF_CONST(_HTTP_VERSION, 2TLS),
#if LIBCURL_VERSION_NUM >= 0x073100 /* 7.49.0 */
    URL_DEF_CONST(_HTTP_VERSION, 2_PRIOR_KNOWLEDGE),
#endif
#if LIBCURL_VERSION_NUM >= 0x074200 /* 7.66.0 */
    URL_DEF_CONST(_HTTP_VERSION, 3),
#endif
    { NULL, 0 },
};

struct t_url_constant url_ftp_auth[] =
{
    URL_DEF_CONST(FTPAUTH, DEFAULT),
    URL_DEF_CONST(FTPAUTH, SSL),
    URL_DEF_CONST(FTPAUTH, TLS),
    { NULL, 0 },
};

struct t_url_constant url_ftp_ssl_ccc[] =
{
    URL_DEF_CONST(FTPSSL, CCC_NONE),
    URL_DEF_CONST(FTPSSL, CCC_ACTIVE),
    URL_DEF_CONST(FTPSSL, CCC_PASSIVE),
    { NULL, 0 },
};

struct t_url_constant url_ftp_file_method[] =
{
    URL_DEF_CONST(FTPMETHOD, MULTICWD),
    URL_DEF_CONST(FTPMETHOD, NOCWD),
    URL_DEF_CONST(FTPMETHOD, SINGLECWD),
    { NULL, 0 },
};

struct t_url_constant url_rtsp_request[] =
{
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
    { NULL, 0 },
};

struct t_url_constant url_time_condition[] =
{
    URL_DEF_CONST(_TIMECOND, NONE),
    URL_DEF_CONST(_TIMECOND, IFMODSINCE),
    URL_DEF_CONST(_TIMECOND, IFUNMODSINCE),
    URL_DEF_CONST(_TIMECOND, LASTMOD),
    { NULL, 0 },
};

struct t_url_constant url_ip_resolve[] =
{
    URL_DEF_CONST(_IPRESOLVE, WHATEVER),
    URL_DEF_CONST(_IPRESOLVE, V4),
    URL_DEF_CONST(_IPRESOLVE, V6),
    { NULL, 0 },
};

struct t_url_constant url_use_ssl[] =
{
    URL_DEF_CONST(USESSL, NONE),
    URL_DEF_CONST(USESSL, TRY),
    URL_DEF_CONST(USESSL, CONTROL),
    URL_DEF_CONST(USESSL, ALL),
    { NULL, 0 },
};

struct t_url_constant url_ssl_version[] =
{
    URL_DEF_CONST(_SSLVERSION, DEFAULT),
    URL_DEF_CONST(_SSLVERSION, TLSv1),
    URL_DEF_CONST(_SSLVERSION, SSLv2),
    URL_DEF_CONST(_SSLVERSION, SSLv3),
    URL_DEF_CONST(_SSLVERSION, TLSv1_0),
    URL_DEF_CONST(_SSLVERSION, TLSv1_1),
    URL_DEF_CONST(_SSLVERSION, TLSv1_2),
#if LIBCURL_VERSION_NUM >= 0x073400 /* 7.52.0 */
    URL_DEF_CONST(_SSLVERSION, TLSv1_3),
#endif
#if LIBCURL_VERSION_NUM >= 0x073600 /* 7.54.0 */
    URL_DEF_CONST(_SSLVERSION, MAX_DEFAULT),
    URL_DEF_CONST(_SSLVERSION, MAX_NONE),
    URL_DEF_CONST(_SSLVERSION, MAX_TLSv1_0),
    URL_DEF_CONST(_SSLVERSION, MAX_TLSv1_1),
    URL_DEF_CONST(_SSLVERSION, MAX_TLSv1_2),
    URL_DEF_CONST(_SSLVERSION, MAX_TLSv1_3),
#endif
    { NULL, 0 },
};

struct t_url_constant url_ssl_options[] =
{
    URL_DEF_CONST(SSLOPT, ALLOW_BEAST),
    URL_DEF_CONST(SSLOPT, NO_REVOKE),
#if LIBCURL_VERSION_NUM >= 0x073800 /* 7.56.0 */
    URL_DEF_CONST(SSLSET, NO_BACKENDS),
    URL_DEF_CONST(SSLSET, OK),
    URL_DEF_CONST(SSLSET, TOO_LATE),
    URL_DEF_CONST(SSLSET, UNKNOWN_BACKEND),
#endif
#if LIBCURL_VERSION_NUM >= 0x074400 /* 7.68.0 */
    URL_DEF_CONST(SSLOPT, NO_PARTIALCHAIN),
#endif
#if LIBCURL_VERSION_NUM >= 0x074600 /* 7.70.0 */
    URL_DEF_CONST(SSLOPT, REVOKE_BEST_EFFORT),
#endif
#if LIBCURL_VERSION_NUM >= 0x074700 /* 7.71.0 */
    URL_DEF_CONST(SSLOPT, NATIVE_CA),
#endif
#if LIBCURL_VERSION_NUM >= 0x074D00 /* 7.77.0 */
    URL_DEF_CONST(SSLOPT, AUTO_CLIENT_CERT),
#endif
    { NULL, 0 },
};

struct t_url_constant url_gssapi_delegation[] =
{
    URL_DEF_CONST(GSSAPI_DELEGATION, NONE),
    URL_DEF_CONST(GSSAPI_DELEGATION, POLICY_FLAG),
    URL_DEF_CONST(GSSAPI_DELEGATION, FLAG),
    { NULL, 0 },
};

struct t_url_constant url_ssh_auth[] =
{
    URL_DEF_CONST(SSH_AUTH, NONE),
    URL_DEF_CONST(SSH_AUTH, PUBLICKEY),
    URL_DEF_CONST(SSH_AUTH, PASSWORD),
    URL_DEF_CONST(SSH_AUTH, HOST),
    URL_DEF_CONST(SSH_AUTH, KEYBOARD),
    URL_DEF_CONST(SSH_AUTH, DEFAULT),
    URL_DEF_CONST(SSH_AUTH, ANY),
    URL_DEF_CONST(SSH_AUTH, AGENT),
#if LIBCURL_VERSION_NUM >= 0x073A00 /* 7.58.0 */
    URL_DEF_CONST(SSH_AUTH, GSSAPI),
#endif
    { NULL, 0 },
};

struct t_url_constant url_header[] =
{
    URL_DEF_CONST(HEADER, UNIFIED),
    URL_DEF_CONST(HEADER, SEPARATE),
    { NULL, 0 },
};

struct t_url_constant url_hsts[] =
{
#if LIBCURL_VERSION_NUM >= 0x074A00 /* 7.74.0 */
    URL_DEF_CONST(HSTS, ENABLE),
    URL_DEF_CONST(HSTS, READONLYFILE),
#endif
    { NULL, 0 },
};

struct t_url_constant url_mime[] =
{
#if LIBCURL_VERSION_NUM >= 0x075100 /* 7.81.0 */
    URL_DEF_CONST(MIMEOPT, FORMESCAPE),
#endif
    { NULL, 0 },
};

struct t_url_constant url_websocket[] =
{
#if LIBCURL_VERSION_NUM >= 0x075600 /* 7.86.0 */
    URL_DEF_CONST(WS, BINARY),
    URL_DEF_CONST(WS, CLOSE),
    URL_DEF_CONST(WS, CONT),
    URL_DEF_CONST(WS, OFFSET),
    URL_DEF_CONST(WS, PING),
    URL_DEF_CONST(WS, PONG),
    URL_DEF_CONST(WS, RAW_MODE),
    URL_DEF_CONST(WS, TEXT),
#endif
    { NULL, 0 },
};

struct t_url_option url_options[] =
{
    /*
     * behavior options
     */
    URL_DEF_OPTION(VERBOSE, LONG, NULL),
    URL_DEF_OPTION(HEADER, LONG, NULL),
    URL_DEF_OPTION(NOPROGRESS, LONG, NULL),
    URL_DEF_OPTION(NOSIGNAL, LONG, NULL),
    URL_DEF_OPTION(WILDCARDMATCH, LONG, NULL),

    /*
     * error options
     */
    URL_DEF_OPTION(FAILONERROR, LONG, NULL),
#if LIBCURL_VERSION_NUM >= 0x073300 /* 7.51.0 */
    URL_DEF_OPTION(KEEP_SENDING_ON_ERROR, LONG, NULL),
#endif

    /*
     * network options
     */
    URL_DEF_OPTION(PROXY, STRING, NULL),
    URL_DEF_OPTION(PROXYPORT, LONG, NULL),
    URL_DEF_OPTION(PORT, LONG, NULL),
#if LIBCURL_VERSION_NUM >= 0x073400 /* 7.52.0 */
    URL_DEF_OPTION(PRE_PROXY, STRING, NULL),
#endif
    URL_DEF_OPTION(HTTPPROXYTUNNEL, LONG, NULL),
    URL_DEF_OPTION(INTERFACE, STRING, NULL),
    URL_DEF_OPTION(DNS_CACHE_TIMEOUT, LONG, NULL),
    URL_DEF_OPTION(PROXYTYPE, LONG, url_proxy_types),
    URL_DEF_OPTION(BUFFERSIZE, LONG, NULL),
    URL_DEF_OPTION(TCP_NODELAY, LONG, NULL),
    URL_DEF_OPTION(LOCALPORT, LONG, NULL),
    URL_DEF_OPTION(LOCALPORTRANGE, LONG, NULL),
    URL_DEF_OPTION(ADDRESS_SCOPE, LONG, NULL),
#if LIBCURL_VERSION_NUM < 0x075500 /* < 7.85.0 */
    URL_DEF_OPTION(PROTOCOLS, MASK, url_protocols),
#endif
#if LIBCURL_VERSION_NUM < 0x075500 /* < 7.85.0 */
    URL_DEF_OPTION(REDIR_PROTOCOLS, MASK, url_protocols),
#endif
    URL_DEF_OPTION(NOPROXY, STRING, NULL),
    URL_DEF_OPTION(SOCKS5_GSSAPI_NEC, LONG, NULL),
    URL_DEF_OPTION(TCP_KEEPALIVE, LONG, NULL),
    URL_DEF_OPTION(TCP_KEEPIDLE, LONG, NULL),
    URL_DEF_OPTION(TCP_KEEPINTVL, LONG, NULL),
    URL_DEF_OPTION(UNIX_SOCKET_PATH, STRING, NULL),
#if LIBCURL_VERSION_NUM >= 0x073500 /* 7.53.0 */
    URL_DEF_OPTION(ABSTRACT_UNIX_SOCKET, STRING, NULL),
#endif
    URL_DEF_OPTION(PATH_AS_IS, LONG, NULL),
    URL_DEF_OPTION(PROXY_SERVICE_NAME, STRING, NULL),
    URL_DEF_OPTION(SERVICE_NAME, STRING, NULL),
    URL_DEF_OPTION(DEFAULT_PROTOCOL, STRING, NULL),
#if LIBCURL_VERSION_NUM < 0x073100 /* < 7.49.0 */
    URL_DEF_OPTION(SOCKS5_GSSAPI_SERVICE, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x073100 /* 7.49.0 */
    URL_DEF_OPTION(TCP_FASTOPEN, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x073700 /* 7.55.0 */
    URL_DEF_OPTION(SOCKS5_AUTH, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x073C00 /* 7.60.0 */
    URL_DEF_OPTION(HAPROXYPROTOCOL, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x073E00 /* 7.62.0 */
    URL_DEF_OPTION(DOH_URL, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x075500 /* 7.85.0 */
    URL_DEF_OPTION(PROTOCOLS_STR, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x075500 /* 7.85.0 */
    URL_DEF_OPTION(REDIR_PROTOCOLS_STR, STRING, NULL),
#endif

    /*
     * names and password options (authentication)
     */
    URL_DEF_OPTION(NETRC, LONG, url_netrc),
    URL_DEF_OPTION(USERPWD, STRING, NULL),
    URL_DEF_OPTION(PROXYUSERPWD, STRING, NULL),
    URL_DEF_OPTION(HTTPAUTH, MASK, url_auth),
    URL_DEF_OPTION(PROXYAUTH, MASK, url_auth),
    URL_DEF_OPTION(NETRC_FILE, STRING, NULL),
    URL_DEF_OPTION(USERNAME, STRING, NULL),
    URL_DEF_OPTION(PASSWORD, STRING, NULL),
    URL_DEF_OPTION(PROXYUSERNAME, STRING, NULL),
    URL_DEF_OPTION(PROXYPASSWORD, STRING, NULL),
    URL_DEF_OPTION(TLSAUTH_TYPE, MASK, url_authtype),
    URL_DEF_OPTION(TLSAUTH_USERNAME, STRING, NULL),
    URL_DEF_OPTION(TLSAUTH_PASSWORD, STRING, NULL),
#if LIBCURL_VERSION_NUM >= 0x074200 /* 7.66.0 */
    URL_DEF_OPTION(SASL_AUTHZID, STRING, NULL),
#endif

    URL_DEF_OPTION(SASL_IR, LONG, NULL),
    URL_DEF_OPTION(XOAUTH2_BEARER, STRING, NULL),
    URL_DEF_OPTION(LOGIN_OPTIONS, STRING, NULL),
#if LIBCURL_VERSION_NUM >= 0x073D00 /* 7.61.0 */
    URL_DEF_OPTION(DISALLOW_USERNAME_IN_URL, LONG, NULL),
#endif

    /*
     * HTTP options
     */
    URL_DEF_OPTION(AUTOREFERER, LONG, NULL),
    URL_DEF_OPTION(FOLLOWLOCATION, LONG, NULL),
    URL_DEF_OPTION(POST, LONG, NULL),
    URL_DEF_OPTION(POSTFIELDS, STRING, NULL),
    URL_DEF_OPTION(REFERER, STRING, NULL),
    URL_DEF_OPTION(USERAGENT, STRING, NULL),
    URL_DEF_OPTION(HTTPHEADER, LIST, NULL),
    URL_DEF_OPTION(COOKIE, STRING, NULL),
    URL_DEF_OPTION(COOKIEFILE, STRING, NULL),
    URL_DEF_OPTION(POSTFIELDSIZE, LONG, NULL),
    URL_DEF_OPTION(MAXREDIRS, LONG, NULL),
    URL_DEF_OPTION(HTTPGET, LONG, NULL),
    URL_DEF_OPTION(COOKIEJAR, STRING, NULL),
    URL_DEF_OPTION(HTTP_VERSION, LONG, url_http_version),
    URL_DEF_OPTION(COOKIESESSION, LONG, NULL),
    URL_DEF_OPTION(HTTP200ALIASES, LIST, NULL),
    URL_DEF_OPTION(UNRESTRICTED_AUTH, LONG, NULL),
    URL_DEF_OPTION(POSTFIELDSIZE_LARGE, LONGLONG, NULL),
    URL_DEF_OPTION(COOKIELIST, STRING, NULL),
    URL_DEF_OPTION(IGNORE_CONTENT_LENGTH, LONG, NULL),
    URL_DEF_OPTION(ACCEPT_ENCODING, STRING, NULL),
    URL_DEF_OPTION(TRANSFER_ENCODING, LONG, NULL),
    URL_DEF_OPTION(HTTP_CONTENT_DECODING, LONG, NULL),
    URL_DEF_OPTION(HTTP_TRANSFER_DECODING, LONG, NULL),
    URL_DEF_OPTION(COPYPOSTFIELDS, STRING, NULL),
    URL_DEF_OPTION(POSTREDIR, MASK, url_postredir),
    URL_DEF_OPTION(EXPECT_100_TIMEOUT_MS, LONG, NULL),
    URL_DEF_OPTION(HEADEROPT, MASK, url_header),
    URL_DEF_OPTION(PROXYHEADER, LIST, NULL),
    URL_DEF_OPTION(PIPEWAIT, LONG, NULL),
    URL_DEF_OPTION(STREAM_WEIGHT, LONG, NULL),
#if LIBCURL_VERSION_NUM >= 0x073700 /* 7.55.0 */
    URL_DEF_OPTION(REQUEST_TARGET, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM < 0x073800 /* < 7.56.0 */
    URL_DEF_OPTION(HTTPPOST, LIST, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x074000 /* 7.64.0 */
    URL_DEF_OPTION(HTTP09_ALLOWED, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x074A00 /* 7.74.0 */
    URL_DEF_OPTION(HSTS, STRING, NULL),
    URL_DEF_OPTION(HSTS_CTRL, MASK, url_hsts),
#endif

    /*
     * SMTP options
     */
    URL_DEF_OPTION(MAIL_FROM, STRING, NULL),
    URL_DEF_OPTION(MAIL_RCPT, LIST, NULL),
    URL_DEF_OPTION(MAIL_AUTH, STRING, NULL),
#if LIBCURL_VERSION_NUM >= 0x074500 && LIBCURL_VERSION_NUM < 0x080200 /* 7.69.0 < 8.2.0 */
    URL_DEF_OPTION(MAIL_RCPT_ALLLOWFAILS, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x080200 /* 8.2.0 */
    URL_DEF_OPTION(MAIL_RCPT_ALLOWFAILS, LONG, NULL),
#endif

    /*
     * TFTP options
     */
    URL_DEF_OPTION(TFTP_BLKSIZE, LONG, NULL),
#if LIBCURL_VERSION_NUM >= 0x073000 /* 7.48.0 */
    URL_DEF_OPTION(TFTP_NO_OPTIONS, LONG, NULL),
#endif

    /*
     * FTP options
     */
    URL_DEF_OPTION(FTPPORT, STRING, NULL),
    URL_DEF_OPTION(QUOTE, LIST, NULL),
    URL_DEF_OPTION(POSTQUOTE, LIST, NULL),
    URL_DEF_OPTION(FTP_USE_EPSV, LONG, NULL),
    URL_DEF_OPTION(PREQUOTE, LIST, NULL),
    URL_DEF_OPTION(FTP_USE_EPRT, LONG, NULL),
    URL_DEF_OPTION(FTP_CREATE_MISSING_DIRS, LONG, NULL),
#if LIBCURL_VERSION_NUM < 0x075500 /* < 7.85.0 */
    URL_DEF_OPTION(FTP_RESPONSE_TIMEOUT, LONG, NULL),
#endif
    URL_DEF_OPTION(FTPSSLAUTH, LONG, url_ftp_auth),
    URL_DEF_OPTION(FTP_ACCOUNT, STRING, NULL),
    URL_DEF_OPTION(FTP_SKIP_PASV_IP, LONG, NULL),
    URL_DEF_OPTION(FTP_FILEMETHOD, LONG, url_ftp_file_method),
    URL_DEF_OPTION(FTP_ALTERNATIVE_TO_USER, STRING, NULL),
    URL_DEF_OPTION(FTP_SSL_CCC, LONG, url_ftp_ssl_ccc),
    URL_DEF_OPTION(DIRLISTONLY, LONG, NULL),
    URL_DEF_OPTION(APPEND, LONG, NULL),
    URL_DEF_OPTION(FTP_USE_PRET, LONG, NULL),

    /*
     * RTSP options
     */
    URL_DEF_OPTION(RTSP_REQUEST, LONG, url_rtsp_request),
    URL_DEF_OPTION(RTSP_SESSION_ID, STRING, NULL),
    URL_DEF_OPTION(RTSP_STREAM_URI, STRING, NULL),
    URL_DEF_OPTION(RTSP_TRANSPORT, STRING, NULL),
    URL_DEF_OPTION(RTSP_CLIENT_CSEQ, LONG, NULL),
    URL_DEF_OPTION(RTSP_SERVER_CSEQ, LONG, NULL),
#if LIBCURL_VERSION_NUM >= 0x074B00 /* 7.75.0 */
    URL_DEF_OPTION(AWS_SIGV4, STRING, NULL),
#endif

    /*
     * protocol options
     */
    URL_DEF_OPTION(CRLF, LONG, NULL),
    URL_DEF_OPTION(RANGE, STRING, NULL),
    URL_DEF_OPTION(RESUME_FROM, LONG, NULL),
    URL_DEF_OPTION(CUSTOMREQUEST, STRING, NULL),
    URL_DEF_OPTION(NOBODY, LONG, NULL),
    URL_DEF_OPTION(INFILESIZE, LONG, NULL),
    URL_DEF_OPTION(UPLOAD, LONG, NULL),
    URL_DEF_OPTION(TIMECONDITION, LONG, url_time_condition),
    URL_DEF_OPTION(TIMEVALUE, LONG, NULL),
    URL_DEF_OPTION(TRANSFERTEXT, LONG, NULL),
    URL_DEF_OPTION(FILETIME, LONG, NULL),
    URL_DEF_OPTION(MAXFILESIZE, LONG, NULL),
    URL_DEF_OPTION(PROXY_TRANSFER_MODE, LONG, NULL),
    URL_DEF_OPTION(RESUME_FROM_LARGE, LONGLONG, NULL),
    URL_DEF_OPTION(INFILESIZE_LARGE, LONGLONG, NULL),
    URL_DEF_OPTION(MAXFILESIZE_LARGE, LONGLONG, NULL),
#if LIBCURL_VERSION_NUM >= 0x073B00 /* 7.59.0 */
    URL_DEF_OPTION(TIMEVALUE_LARGE, LONGLONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x073E00 /* 7.62.0 */
    URL_DEF_OPTION(UPLOAD_BUFFERSIZE, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x075100 /* 7.81.0 */
    URL_DEF_OPTION(MIME_OPTIONS, MASK, url_mime),
#endif

    /*
     * connection options
     */
    URL_DEF_OPTION(TIMEOUT, LONG, NULL),
    URL_DEF_OPTION(LOW_SPEED_LIMIT, LONG, NULL),
    URL_DEF_OPTION(LOW_SPEED_TIME, LONG, NULL),
    URL_DEF_OPTION(FRESH_CONNECT, LONG, NULL),
    URL_DEF_OPTION(FORBID_REUSE, LONG, NULL),
    URL_DEF_OPTION(CONNECTTIMEOUT, LONG, NULL),
    URL_DEF_OPTION(IPRESOLVE, LONG, url_ip_resolve),
    URL_DEF_OPTION(CONNECT_ONLY, LONG, NULL),
    URL_DEF_OPTION(MAX_SEND_SPEED_LARGE, LONGLONG, NULL),
    URL_DEF_OPTION(MAX_RECV_SPEED_LARGE, LONGLONG, NULL),
    URL_DEF_OPTION(TIMEOUT_MS, LONG, NULL),
    URL_DEF_OPTION(CONNECTTIMEOUT_MS, LONG, NULL),
#if LIBCURL_VERSION_NUM >= 0x074100 /* 7.65.0 */
    URL_DEF_OPTION(MAXAGE_CONN, LONG, NULL),
#endif
    URL_DEF_OPTION(MAXCONNECTS, LONG, NULL),
    URL_DEF_OPTION(USE_SSL, LONG, url_use_ssl),
    URL_DEF_OPTION(RESOLVE, LIST, NULL),
    URL_DEF_OPTION(DNS_SERVERS, STRING, NULL),
    URL_DEF_OPTION(ACCEPTTIMEOUT_MS, LONG, NULL),
    URL_DEF_OPTION(DNS_INTERFACE, STRING, NULL),
    URL_DEF_OPTION(DNS_LOCAL_IP4, STRING, NULL),
    URL_DEF_OPTION(DNS_LOCAL_IP6, STRING, NULL),
#if LIBCURL_VERSION_NUM >= 0x073100 /* 7.49.0 */
    URL_DEF_OPTION(CONNECT_TO, LIST, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x073B00 /* 7.59.0 */
    URL_DEF_OPTION(HAPPY_EYEBALLS_TIMEOUT_MS, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x073C00 /* 7.60.0 */
    URL_DEF_OPTION(DNS_SHUFFLE_ADDRESSES, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x073E00 /* 7.62.0 */
    URL_DEF_OPTION(UPKEEP_INTERVAL_MS, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x075000 /* 7.80.0 */
    URL_DEF_OPTION(MAXLIFETIME_CONN, LONG, NULL),
#endif

    /*
     * SSL and security options
     */
    URL_DEF_OPTION(SSLCERT, STRING, NULL),
    URL_DEF_OPTION(SSLVERSION, LONG, url_ssl_version),
    URL_DEF_OPTION(SSL_VERIFYPEER, LONG, NULL),
    URL_DEF_OPTION(CAINFO, STRING, NULL),
#if LIBCURL_VERSION_NUM < 0x075400 /* < 7.84.0 */
    URL_DEF_OPTION(RANDOM_FILE, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM < 0x075400 /* < 7.84.0 */
    URL_DEF_OPTION(EGDSOCKET, STRING, NULL),
#endif
    URL_DEF_OPTION(SSL_VERIFYHOST, LONG, NULL),
    URL_DEF_OPTION(SSL_CIPHER_LIST, STRING, NULL),
    URL_DEF_OPTION(SSLCERTTYPE, STRING, NULL),
    URL_DEF_OPTION(SSLKEY, STRING, NULL),
    URL_DEF_OPTION(SSLKEYTYPE, STRING, NULL),
    URL_DEF_OPTION(SSLENGINE, STRING, NULL),
    URL_DEF_OPTION(SSLENGINE_DEFAULT, LONG, NULL),
    URL_DEF_OPTION(CAPATH, STRING, NULL),
    URL_DEF_OPTION(SSL_SESSIONID_CACHE, LONG, NULL),
    URL_DEF_OPTION(KRBLEVEL, STRING, NULL),
    URL_DEF_OPTION(KEYPASSWD, STRING, NULL),
    URL_DEF_OPTION(ISSUERCERT, STRING, NULL),
    URL_DEF_OPTION(CRLFILE, STRING, NULL),
    URL_DEF_OPTION(CERTINFO, LONG, NULL),
    URL_DEF_OPTION(GSSAPI_DELEGATION, LONG, url_gssapi_delegation),
    URL_DEF_OPTION(SSL_OPTIONS, LONG, url_ssl_options),
    URL_DEF_OPTION(SSL_ENABLE_ALPN, LONG, NULL),
#if LIBCURL_VERSION_NUM < 0x075600 /* < 7.86.0 */
    URL_DEF_OPTION(SSL_ENABLE_NPN, LONG, NULL),
#endif
    URL_DEF_OPTION(PINNEDPUBLICKEY, STRING, NULL),
    URL_DEF_OPTION(SSL_VERIFYSTATUS, LONG, NULL),
    URL_DEF_OPTION(SSL_FALSESTART, LONG, NULL),
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
    URL_DEF_OPTION(PROXY_SSL_CIPHER_LIST, LIST, NULL),
    URL_DEF_OPTION(PROXY_SSL_OPTIONS, LONG, url_ssl_options),
    URL_DEF_OPTION(PROXY_SSL_VERIFYHOST, LONG, NULL),
    URL_DEF_OPTION(PROXY_SSL_VERIFYPEER, LONG, NULL),
    URL_DEF_OPTION(PROXY_TLSAUTH_PASSWORD, STRING, NULL),
    URL_DEF_OPTION(PROXY_TLSAUTH_TYPE, STRING, NULL),
    URL_DEF_OPTION(PROXY_TLSAUTH_USERNAME, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x073D00 /* 7.61.0 */
    URL_DEF_OPTION(TLS13_CIPHERS, LIST, NULL),
    URL_DEF_OPTION(PROXY_TLS13_CIPHERS, LIST, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x074700 /* 7.71.0 */
    URL_DEF_OPTION(PROXY_ISSUERCERT, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x074900 /* 7.73.0 */
    URL_DEF_OPTION(SSL_EC_CURVES, STRING, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x074C00 /* 7.76.0 */
    URL_DEF_OPTION(DOH_SSL_VERIFYHOST, LONG, NULL),
    URL_DEF_OPTION(DOH_SSL_VERIFYPEER, LONG, NULL),
    URL_DEF_OPTION(DOH_SSL_VERIFYSTATUS, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x075700 /* 7.87.0 */
    URL_DEF_OPTION(CA_CACHE_TIMEOUT, LONG, NULL),
#endif

    /*
     * SSH options
     */
    URL_DEF_OPTION(SSH_AUTH_TYPES, MASK, url_gssapi_delegation),
    URL_DEF_OPTION(SSH_PUBLIC_KEYFILE, STRING, NULL),
    URL_DEF_OPTION(SSH_PRIVATE_KEYFILE, STRING, NULL),
    URL_DEF_OPTION(SSH_HOST_PUBLIC_KEY_MD5, STRING, NULL),
    URL_DEF_OPTION(SSH_KNOWNHOSTS, STRING, NULL),
#if LIBCURL_VERSION_NUM >= 0x073800 /* 7.56.0 */
    URL_DEF_OPTION(SSH_COMPRESSION, LONG, NULL),
#endif
#if LIBCURL_VERSION_NUM >= 0x075000 /* 7.80.0 */
    URL_DEF_OPTION(SSH_HOST_PUBLIC_KEY_SHA256, STRING, NULL),
#endif

    /*
     * telnet options
     */
    URL_DEF_OPTION(TELNETOPTIONS, LIST, NULL),

    /*
     * websocket options
     */
#if LIBCURL_VERSION_NUM >= 0x075600 /* 7.86.0 */
    URL_DEF_OPTION(WS_OPTIONS, MASK, url_websocket),
#endif

    /*
     * other options
     */
    URL_DEF_OPTION(NEW_FILE_PERMS, LONG, NULL),
    URL_DEF_OPTION(NEW_DIRECTORY_PERMS, LONG, NULL),
#if LIBCURL_VERSION_NUM >= 0x075700 /* 7.87.0 */
    URL_DEF_OPTION(QUICK_EXIT, LONG, NULL),
#endif

    { NULL, 0, 0, NULL },
};


/*
 * Searches for a constant in array of constants.
 *
 * Returns index of constant, -1 if not found.
 */

int
weeurl_search_constant (struct t_url_constant *constants, const char *name)
{
    int i;

    if (!constants || !name)
        return -1;

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

    if (!constants || !string_mask)
        return 0;

    mask = 0;

    items = string_split (string_mask, "+", NULL,
                          WEECHAT_STRING_SPLIT_STRIP_LEFT
                          | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                          | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                          0, &num_items);
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

    if (!name)
        return -1;

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
weeurl_read_stream (void *buffer, size_t size, size_t nmemb, void *stream)
{
    return (stream) ? fread (buffer, size, nmemb, stream) : 0;
}

/*
 * Writes data in a file (callback called to write a file).
 */

size_t
weeurl_write_stream (void *buffer, size_t size, size_t nmemb, void *stream)
{
    return (stream) ? fwrite (buffer, size, nmemb, stream) : 0;
}

/*
 * Adds data to a dynamic string (callback called to catch stdout).
 */

size_t
weeurl_write_string (void *buffer, size_t size, size_t nmemb, void *string)
{
    if (!string)
        return 0;

    string_dyn_concat ((char **)string, buffer, size * nmemb);

    return size * nmemb;
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
    int i, index, index_constant, rc, num_items;
    long long_value;
    long long long_long_value;
    struct curl_slist *slist;
    char **items;

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
                items = string_split (value, "\n", NULL,
                                      WEECHAT_STRING_SPLIT_STRIP_LEFT
                                      | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                      | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                      0, &num_items);
                if (items)
                {
                    slist = NULL;
                    for (i = 0; i < num_items; i++)
                    {
                        slist = curl_slist_append (slist, items[i]);
                    }
                    curl_easy_setopt (curl,
                                      url_options[index].option,
                                      slist);
                    string_free_split (items);
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
    switch (CONFIG_ENUM(proxy->options[PROXY_OPTION_TYPE]))
    {
        case PROXY_TYPE_HTTP:
            curl_easy_setopt (curl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
            break;
        case PROXY_TYPE_SOCKS4:
            curl_easy_setopt (curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS4);
            break;
        case PROXY_TYPE_SOCKS5:
            curl_easy_setopt (curl, CURLOPT_PROXYTYPE,
                              CURLPROXY_SOCKS5_HOSTNAME);
            break;
    }

    /* set proxy address */
    curl_easy_setopt (curl, CURLOPT_PROXY,
                      CONFIG_STRING(proxy->options[PROXY_OPTION_ADDRESS]));

    /* set proxy port */
    curl_easy_setopt (curl, CURLOPT_PROXYPORT,
                      CONFIG_INTEGER(proxy->options[PROXY_OPTION_PORT]));

    /* set username/password */
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
}

/*
 * Downloads URL using options.
 *
 * If output is not NULL, it must be a hashtable with keys and values of type
 * "string". The following keys may be added in the hashtable,
 * depending on the success or error of the URL transfer:
 *
 *   key           | description
 *   --------------|--------------------------------------------------------
 *   response_code | HTTP response code (as string)
 *   headers       | HTTP headers in response
 *   output        | stdout (set only if "file_out" was not set in options)
 *   error         | error message (set only in case of error)
 *
 * Returns:
 *   0: OK
 *   1: invalid URL
 *   2: error downloading URL
 *   3: not enough memory
 *   4: file error
 */

int
weeurl_download (const char *url, struct t_hashtable *options,
                 struct t_hashtable *output)
{
    CURL *curl;
    struct t_url_file url_file[2];
    char *url_file_option[2] = { "file_in", "file_out" };
    char *url_file_mode[2] = { "rb", "wb" };
    char url_error[CURL_ERROR_SIZE + 1], url_error_code[12];
    char **string_headers, **string_output;
    char str_response_code[32];
    CURLoption url_file_opt_func[2] = { CURLOPT_READFUNCTION, CURLOPT_WRITEFUNCTION };
    CURLoption url_file_opt_data[2] = { CURLOPT_READDATA, CURLOPT_WRITEDATA };
    void *url_file_opt_cb[2] = { &weeurl_read_stream, &weeurl_write_stream };
    struct t_proxy *ptr_proxy;
    int rc, curl_rc, i, output_to_file;
    long response_code;

    rc = 0;

    string_headers = NULL;
    string_output = NULL;
    url_error[0] = '\0';
    url_error_code[0] = '\0';

    for (i = 0; i < 2; i++)
    {
        url_file[i].filename = NULL;
        url_file[i].stream = NULL;
    }

    if (!url || !url[0])
    {
        snprintf (url_error, sizeof (url_error), "%s", _("invalid URL"));
        rc = 1;
        goto end;
    }

    curl = curl_easy_init ();
    if (!curl)
    {
        snprintf (url_error, sizeof (url_error), "%s", _("not enough memory"));
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

    /* set callback to retrieve HTTP headers */
    if (output)
    {
        string_headers = string_dyn_alloc (1024);
        if (string_headers)
        {
            curl_easy_setopt (curl, CURLOPT_HEADERFUNCTION, &weeurl_write_string);
            curl_easy_setopt (curl, CURLOPT_HEADERDATA, string_headers);
        }
    }

    /* set file in/out from options in hashtable */
    output_to_file = 0;
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
                    snprintf (url_error, sizeof (url_error),
                              (i == 0) ?
                              _("file \"%s\" not found") :
                              _("cannot write file \"%s\""),
                              url_file[i].filename);
                    rc = 4;
                    goto end;
                }
                curl_easy_setopt (curl, url_file_opt_func[i], url_file_opt_cb[i]);
                curl_easy_setopt (curl, url_file_opt_data[i], url_file[i].stream);
                if (i == 1)
                    output_to_file = 1;
            }
        }
    }

    /* redirect stdout if no filename was given (via key "file_out") */
    if (output && !output_to_file)
    {
        string_output = string_dyn_alloc (1024);
        if (string_output)
        {
            curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, &weeurl_write_string);
            curl_easy_setopt (curl, CURLOPT_WRITEDATA, string_output);
        }
    }

    /* set other options in hashtable */
    hashtable_map (options, &weeurl_option_map_cb, curl);

    /* set error buffer */
    curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, url_error);

    /* perform action! */
    curl_rc = curl_easy_perform (curl);
    if (curl_rc == CURLE_OK)
    {
        if (output)
        {
            curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &response_code);
            snprintf (str_response_code, sizeof (str_response_code),
                      "%ld", response_code);
            hashtable_set (output, "response_code", str_response_code);
        }
    }
    else
    {
        if (output)
        {
            snprintf (url_error_code, sizeof (url_error_code), "%d", curl_rc);
            if (!url_error[0])
            {
                snprintf (url_error, sizeof (url_error),
                          "%s", _("transfer error"));
            }
        }
        else
        {
            /*
             * URL transfer done in a forked process: display error on stderr,
             * which will be sent to the hook_process callback
             */
            fprintf (stderr,
                     _("curl error %d (%s) (URL: \"%s\")\n"),
                     curl_rc, url_error, url);
        }
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
    if (output)
    {
        if (string_headers)
        {
            hashtable_set (output, "headers", *string_headers);
            string_dyn_free (string_headers, 1);
        }
        if (string_output)
        {
            hashtable_set (output, "output", *string_output);
            string_dyn_free (string_output, 1);
        }
        if (url_error[0])
            hashtable_set (output, "error", url_error);
        if (url_error_code[0])
            hashtable_set (output, "error_code_curl", url_error_code);
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

/*
 * Initializes URL.
 */

void
weeurl_init (void)
{
    curl_global_init (CURL_GLOBAL_ALL);
}

/*
 * Ends URL.
 */

void
weeurl_end (void)
{
    curl_global_cleanup ();
}
