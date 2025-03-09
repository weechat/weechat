/*
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PROXY_H
#define WEECHAT_PROXY_H

struct t_infolist;

enum t_proxy_option
{
    PROXY_OPTION_TYPE = 0,             /* type: http, socks4, socks5        */
    PROXY_OPTION_IPV6,                 /* ipv6 ? or ipv4 ?                  */
    PROXY_OPTION_ADDRESS,              /* address (IP or hostname)          */
    PROXY_OPTION_PORT,                 /* port                              */
    PROXY_OPTION_USERNAME,             /* username (optional)               */
    PROXY_OPTION_PASSWORD,             /* password (optional)               */
    /* number of proxy options */
    PROXY_NUM_OPTIONS,
};

enum t_proxy_type
{
    PROXY_TYPE_HTTP = 0,
    PROXY_TYPE_SOCKS4,
    PROXY_TYPE_SOCKS5,
    /* number of proxy types */
    PROXY_NUM_TYPES,
};

enum t_proxy_ipv6
{
    PROXY_IPV6_DISABLE = 0,
    PROXY_IPV6_AUTO,
    PROXY_IPV6_FORCE,
    /* number of IPv6 options */
    PROXY_NUM_IPV6,
};

struct t_proxy
{
    char *name;                         /* proxy name                       */
    struct t_config_option *options[PROXY_NUM_OPTIONS];

    struct t_proxy *prev_proxy;         /* link to previous bar             */
    struct t_proxy *next_proxy;         /* link to next bar                 */
};

/* variables */

extern char *proxy_option_string[];
extern char *proxy_type_string[];
extern char *proxy_ipv6_string[];
extern struct t_proxy *weechat_proxies;
extern struct t_proxy *last_weechat_proxy;
extern struct t_proxy *weechat_temp_proxies;
extern struct t_proxy *last_weechat_temp_proxy;

/* functions */

extern int proxy_search_option (const char *option_name);
extern int proxy_search_type (const char *type);
extern int proxy_valid (struct t_proxy *proxy);
extern struct t_proxy *proxy_search (const char *name);
extern int proxy_set (struct t_proxy *bar, const char *property,
                      const char *value);
extern void proxy_create_option_temp (struct t_proxy *temp_proxy,
                                      int index_option, const char *value);
extern struct t_proxy *proxy_alloc (const char *name);
extern struct t_proxy *proxy_new (const char *name,
                                  const char *type,
                                  const char *ipv6,
                                  const char *address,
                                  const char *port,
                                  const char *username,
                                  const char *password);
extern void proxy_use_temp_proxies (void);
extern void proxy_free (struct t_proxy *proxy);
extern void proxy_free_all (void);
extern struct t_hdata *proxy_hdata_proxy_cb (const void *pointer,
                                             void *data,
                                             const char *hdata_name);
extern int proxy_add_to_infolist (struct t_infolist *infolist,
                                  struct t_proxy *proxy);
extern void proxy_print_log (void);

#endif /* WEECHAT_PROXY_H */
