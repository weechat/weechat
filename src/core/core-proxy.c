/*
 * core-proxy.c - proxy functions
 *
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "weechat.h"
#include "core-proxy.h"
#include "core-config.h"
#include "core-hdata.h"
#include "core-infolist.h"
#include "core-log.h"
#include "core-string.h"
#include "../plugins/plugin.h"


char *proxy_option_string[PROXY_NUM_OPTIONS] =
{ "type", "ipv6", "address", "port", "username", "password" };
char *proxy_option_default[PROXY_NUM_OPTIONS] =
{ "http", "auto", "127.0.0.1", "3128", "", "" };
char *proxy_type_string[PROXY_NUM_TYPES] =
{ "http", "socks4", "socks5" };
char *proxy_ipv6_string[PROXY_NUM_IPV6] =
{ "disable", "auto", "force" };

struct t_proxy *weechat_proxies = NULL;    /* first proxy                   */
struct t_proxy *last_weechat_proxy = NULL; /* last proxy                    */

struct t_proxy *weechat_temp_proxies = NULL;    /* proxies used when        */
struct t_proxy *last_weechat_temp_proxy = NULL; /* reading configuration    */


/*
 * Searches for a proxy option.
 *
 * Returns index of option in enum t_proxy_option, -1 if option is not found.
 */

int
proxy_search_option (const char *option_name)
{
    int i;

    if (!option_name)
        return -1;

    for (i = 0; i < PROXY_NUM_OPTIONS; i++)
    {
        if (strcmp (proxy_option_string[i], option_name) == 0)
            return i;
    }

    /* proxy option not found */
    return -1;
}

/*
 * Searches for a proxy type.
 *
 * Returns index of option in enum t_proxy_type, -1 if type is not found.
 */

int
proxy_search_type (const char *type)
{
    int i;

    if (!type)
        return -1;

    for (i = 0; i < PROXY_NUM_TYPES; i++)
    {
        if (strcmp (proxy_type_string[i], type) == 0)
            return i;
    }

    /* type not found */
    return -1;
}

/*
 * Checks if a proxy pointer is valid.
 *
 * Returns:
 *   1: proxy exists
 *   0: proxy does not exist
 */

int
proxy_valid (struct t_proxy *proxy)
{
    struct t_proxy *ptr_proxy;

    if (!proxy)
        return 0;

    for (ptr_proxy = weechat_proxies; ptr_proxy;
         ptr_proxy = ptr_proxy->next_proxy)
    {
        if (ptr_proxy == proxy)
            return 1;
    }

    /* proxy not found */
    return 0;
}

/*
 * Searches for a proxy by name.
 *
 * Returns pointer to proxy found, NULL if not found.
 */

struct t_proxy *
proxy_search (const char *name)
{
    struct t_proxy *ptr_proxy;

    if (!name || !name[0])
        return NULL;

    for (ptr_proxy = weechat_proxies; ptr_proxy;
         ptr_proxy = ptr_proxy->next_proxy)
    {
        if (strcmp (ptr_proxy->name, name) == 0)
            return ptr_proxy;
    }

    /* proxy not found */
    return NULL;
}

/*
 * Sets name for a proxy.
 */

void
proxy_set_name (struct t_proxy *proxy, const char *name)
{
    int length;
    char *option_name;

    if (!name || !name[0])
        return;

    length = strlen (name) + 64;
    option_name = malloc (length);
    if (option_name)
    {
        snprintf (option_name, length, "%s.type", name);
        config_file_option_rename (proxy->options[PROXY_OPTION_TYPE], option_name);
        snprintf (option_name, length, "%s.ipv6", name);
        config_file_option_rename (proxy->options[PROXY_OPTION_IPV6], option_name);
        snprintf (option_name, length, "%s.address", name);
        config_file_option_rename (proxy->options[PROXY_OPTION_ADDRESS], option_name);
        snprintf (option_name, length, "%s.port", name);
        config_file_option_rename (proxy->options[PROXY_OPTION_PORT], option_name);
        snprintf (option_name, length, "%s.username", name);
        config_file_option_rename (proxy->options[PROXY_OPTION_USERNAME], option_name);
        snprintf (option_name, length, "%s.password", name);
        config_file_option_rename (proxy->options[PROXY_OPTION_PASSWORD], option_name);

        free (proxy->name);
        proxy->name = strdup (name);

        free (option_name);
    }
}

/*
 * Sets a property for a proxy.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
proxy_set (struct t_proxy *proxy, const char *property, const char *value)
{
    if (!proxy || !property || !value)
        return 0;

    if (strcmp (property, "name") == 0)
    {
        proxy_set_name (proxy, value);
        return 1;
    }
    else if (strcmp (property, "type") == 0)
    {
        config_file_option_set (proxy->options[PROXY_OPTION_TYPE], value, 1);
        return 1;
    }
    else if (strcmp (property, "ipv6") == 0)
    {
        config_file_option_set (proxy->options[PROXY_OPTION_IPV6], value, 1);
        return 1;
    }
    else if (strcmp (property, "address") == 0)
    {
        config_file_option_set (proxy->options[PROXY_OPTION_ADDRESS], value, 1);
        return 1;
    }
    else if (strcmp (property, "port") == 0)
    {
        config_file_option_set (proxy->options[PROXY_OPTION_PORT], value, 1);
        return 1;
    }
    else if (strcmp (property, "username") == 0)
    {
        config_file_option_set (proxy->options[PROXY_OPTION_USERNAME], value, 1);
        return 1;
    }
    else if (strcmp (property, "password") == 0)
    {
        config_file_option_set (proxy->options[PROXY_OPTION_PASSWORD], value, 1);
        return 1;
    }

    return 0;
}

/*
 * Creates an option for a proxy.
 *
 * Returns pointer to new option, NULL if error.
 */

struct t_config_option *
proxy_create_option (const char *proxy_name, int index_option,
                     const char *value)
{
    struct t_config_option *ptr_option;
    char *option_name;

    ptr_option = NULL;

    if (string_asprintf (&option_name,
                         "%s.%s",
                         proxy_name,
                         proxy_option_string[index_option]) < 0)
    {
        return NULL;
    }

    switch (index_option)
    {
        case PROXY_OPTION_TYPE:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_proxy,
                option_name, "enum",
                N_("proxy type (http (default), socks4, socks5)"),
                "http|socks4|socks5", 0, 0, value, NULL, 0,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            break;
        case PROXY_OPTION_IPV6:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_proxy,
                option_name, "enum",
                N_("connect to proxy using IPv6"),
                "disable|auto|force", 0, 0, value, NULL, 0,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            break;
        case PROXY_OPTION_ADDRESS:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_proxy,
                option_name, "string",
                N_("proxy server address (IP or hostname)"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            break;
        case PROXY_OPTION_PORT:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_proxy,
                option_name, "integer",
                N_("port for connecting to proxy server"),
                NULL, 0, 65535, value, NULL, 0,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            break;
        case PROXY_OPTION_USERNAME:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_proxy,
                option_name, "string",
                N_("username for proxy server "
                   "(note: content is evaluated, see /help eval)"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            break;
        case PROXY_OPTION_PASSWORD:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_proxy,
                option_name, "string",
                N_("password for proxy server "
                   "(note: content is evaluated, see /help eval)"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            break;
        case PROXY_NUM_OPTIONS:
            break;
    }

    free (option_name);

    return ptr_option;
}

/*
 * Creates an option for a temporary proxy (when reading configuration file).
 */

void
proxy_create_option_temp (struct t_proxy *temp_proxy, int index_option,
                          const char *value)
{
    struct t_config_option *new_option;

    new_option = proxy_create_option (temp_proxy->name,
                                      index_option,
                                      value);
    if (new_option && (index_option >= 0))
        temp_proxy->options[index_option] = new_option;
}

/*
 * Allocates and initializes a new proxy structure.
 *
 * Returns pointer to new proxy, NULL if error.
 */

struct t_proxy *
proxy_alloc (const char *name)
{
    struct t_proxy *new_proxy;
    int i;

    new_proxy = malloc (sizeof (*new_proxy));
    if (new_proxy)
    {
        new_proxy->name = strdup (name);
        for (i = 0; i < PROXY_NUM_OPTIONS; i++)
        {
            new_proxy->options[i] = NULL;
        }
        new_proxy->prev_proxy = NULL;
        new_proxy->next_proxy = NULL;
    }

    return new_proxy;
}

/*
 * Adds a new proxy with options.
 *
 * Returns pointer to new proxy, NULL if error.
 */

struct t_proxy *
proxy_new_with_options (const char *name,
                        struct t_config_option *type,
                        struct t_config_option *ipv6,
                        struct t_config_option *address,
                        struct t_config_option *port,
                        struct t_config_option *username,
                        struct t_config_option *password)
{
    struct t_proxy *new_proxy;

    /* add proxy */
    new_proxy = proxy_alloc (name);
    if (!new_proxy)
        return NULL;

    new_proxy->options[PROXY_OPTION_TYPE] = type;
    new_proxy->options[PROXY_OPTION_IPV6] = ipv6;
    new_proxy->options[PROXY_OPTION_ADDRESS] = address;
    new_proxy->options[PROXY_OPTION_PORT] = port;
    new_proxy->options[PROXY_OPTION_USERNAME] = username;
    new_proxy->options[PROXY_OPTION_PASSWORD] = password;

    /* add proxy to proxies list */
    new_proxy->prev_proxy = last_weechat_proxy;
    if (last_weechat_proxy)
        last_weechat_proxy->next_proxy = new_proxy;
    else
        weechat_proxies = new_proxy;
    last_weechat_proxy = new_proxy;
    new_proxy->next_proxy = NULL;

    return new_proxy;
}

/*
 * Adds a new proxy.
 *
 * Returns pointer to new proxy, NULL if error.
 */

struct t_proxy *
proxy_new (const char *name, const char *type, const char *ipv6,
           const char *address, const char *port, const char *username,
           const char *password)
{
    struct t_config_option *option_type, *option_ipv6, *option_address;
    struct t_config_option *option_port, *option_username, *option_password;
    struct t_proxy *new_proxy;

    if (!name || !name[0])
        return NULL;

    /* it's not possible to add 2 proxies with same name */
    if (proxy_search (name))
        return NULL;

    /* look for type */
    if (proxy_search_type (type) < 0)
        return NULL;

    option_type = proxy_create_option (name, PROXY_OPTION_TYPE,
                                       type);
    option_ipv6 = proxy_create_option (name, PROXY_OPTION_IPV6,
                                       ipv6);
    option_address = proxy_create_option (name, PROXY_OPTION_ADDRESS,
                                          (address) ? address : "");
    option_port = proxy_create_option (name, PROXY_OPTION_PORT,
                                       port);
    option_username = proxy_create_option (name, PROXY_OPTION_USERNAME,
                                           (username) ? username : "");
    option_password = proxy_create_option (name, PROXY_OPTION_PASSWORD,
                                           (password) ? password : "");

    new_proxy = proxy_new_with_options (name, option_type, option_ipv6,
                                        option_address, option_port,
                                        option_username, option_password);
    if (!new_proxy)
    {
        config_file_option_free (option_type, 0);
        config_file_option_free (option_ipv6, 0);
        config_file_option_free (option_address, 0);
        config_file_option_free (option_port, 0);
        config_file_option_free (option_username, 0);
        config_file_option_free (option_password, 0);
    }

    return new_proxy;
}

/*
 * Uses temporary proxies (added by reading configuration file).
 */

void
proxy_use_temp_proxies (void)
{
    struct t_proxy *ptr_temp_proxy, *next_temp_proxy;
    int i, num_options_ok;

    for (ptr_temp_proxy = weechat_temp_proxies; ptr_temp_proxy;
         ptr_temp_proxy = ptr_temp_proxy->next_proxy)
    {
        num_options_ok = 0;
        for (i = 0; i < PROXY_NUM_OPTIONS; i++)
        {
            if (!ptr_temp_proxy->options[i])
            {
                ptr_temp_proxy->options[i] = proxy_create_option (ptr_temp_proxy->name,
                                                                  i,
                                                                  proxy_option_default[i]);
            }
            if (ptr_temp_proxy->options[i])
                num_options_ok++;
        }

        if (num_options_ok == PROXY_NUM_OPTIONS)
        {
            proxy_new_with_options (ptr_temp_proxy->name,
                                    ptr_temp_proxy->options[PROXY_OPTION_TYPE],
                                    ptr_temp_proxy->options[PROXY_OPTION_IPV6],
                                    ptr_temp_proxy->options[PROXY_OPTION_ADDRESS],
                                    ptr_temp_proxy->options[PROXY_OPTION_PORT],
                                    ptr_temp_proxy->options[PROXY_OPTION_USERNAME],
                                    ptr_temp_proxy->options[PROXY_OPTION_PASSWORD]);
        }
        else
        {
            for (i = 0; i < PROXY_NUM_OPTIONS; i++)
            {
                if (ptr_temp_proxy->options[i])
                {
                    config_file_option_free (ptr_temp_proxy->options[i], 0);
                    ptr_temp_proxy->options[i] = NULL;
                }
            }
        }
    }

    /* free all temp proxies */
    while (weechat_temp_proxies)
    {
        next_temp_proxy = weechat_temp_proxies->next_proxy;

        free (weechat_temp_proxies->name);
        free (weechat_temp_proxies);

        weechat_temp_proxies = next_temp_proxy;
    }
    last_weechat_temp_proxy = NULL;
}

/*
 * Frees a proxy.
 */

void
proxy_free (struct t_proxy *proxy)
{
    int i;

    if (!proxy)
        return;

    /* remove proxy from proxies list */
    if (proxy->prev_proxy)
        (proxy->prev_proxy)->next_proxy = proxy->next_proxy;
    if (proxy->next_proxy)
        (proxy->next_proxy)->prev_proxy = proxy->prev_proxy;
    if (weechat_proxies == proxy)
        weechat_proxies = proxy->next_proxy;
    if (last_weechat_proxy == proxy)
        last_weechat_proxy = proxy->prev_proxy;

    /* free data */
    free (proxy->name);
    for (i = 0; i < PROXY_NUM_OPTIONS; i++)
    {
        config_file_option_free (proxy->options[i], 1);
    }

    free (proxy);
}

/*
 * Frees all proxies.
 */

void
proxy_free_all (void)
{
    while (weechat_proxies)
    {
        proxy_free (weechat_proxies);
    }
}

/*
 * Returns hdata for proxy.
 */

struct t_hdata *
proxy_hdata_proxy_cb (const void *pointer, void *data,
                      const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_proxy", "next_proxy",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_proxy, name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_proxy, options, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_proxy, prev_proxy, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_proxy, next_proxy, POINTER, 0, NULL, hdata_name);
        HDATA_LIST(weechat_proxies, WEECHAT_HDATA_LIST_CHECK_POINTERS);
        HDATA_LIST(last_weechat_proxy, 0);
    }
    return hdata;
}

/*
 * Adds a proxy in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
proxy_add_to_infolist (struct t_infolist *infolist, struct t_proxy *proxy)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !proxy)
        return 0;

    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_string (ptr_item, "name", proxy->name))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "type", CONFIG_ENUM(proxy->options[PROXY_OPTION_TYPE])))
        return 0;
    if (!infolist_new_var_string (ptr_item, "type_string", proxy_type_string[CONFIG_ENUM(proxy->options[PROXY_OPTION_TYPE])]))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "ipv6", CONFIG_INTEGER(proxy->options[PROXY_OPTION_IPV6])))
        return 0;
    if (!infolist_new_var_string (ptr_item, "address", CONFIG_STRING(proxy->options[PROXY_OPTION_ADDRESS])))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "port", CONFIG_INTEGER(proxy->options[PROXY_OPTION_PORT])))
        return 0;
    if (!infolist_new_var_string (ptr_item, "username", CONFIG_STRING(proxy->options[PROXY_OPTION_USERNAME])))
        return 0;
    if (!infolist_new_var_string (ptr_item, "password", CONFIG_STRING(proxy->options[PROXY_OPTION_PASSWORD])))
        return 0;

    return 1;
}

/*
 * Prints proxies in WeeChat log file (usually for crash dump).
 */

void
proxy_print_log (void)
{
    struct t_proxy *ptr_proxy;

    for (ptr_proxy = weechat_proxies; ptr_proxy;
         ptr_proxy = ptr_proxy->next_proxy)
    {
        log_printf ("");
        log_printf ("[proxy (addr:%p)]", ptr_proxy);
        log_printf ("  name . . . . . . . . . : '%s'", ptr_proxy->name);
        log_printf ("  type . . . . . . . . . : %d (%s)",
                    CONFIG_ENUM(ptr_proxy->options[PROXY_OPTION_TYPE]),
                    proxy_type_string[CONFIG_ENUM(ptr_proxy->options[PROXY_OPTION_TYPE])]);
        log_printf ("  ipv6 . . . . . . . . . : %d (%s)",
                    CONFIG_ENUM(ptr_proxy->options[PROXY_OPTION_IPV6]),
                    proxy_ipv6_string[CONFIG_ENUM(ptr_proxy->options[PROXY_OPTION_IPV6])]);
        log_printf ("  address. . . . . . . . : '%s'", CONFIG_STRING(ptr_proxy->options[PROXY_OPTION_ADDRESS]));
        log_printf ("  port . . . . . . . . . : %d", CONFIG_INTEGER(ptr_proxy->options[PROXY_OPTION_PORT]));
        log_printf ("  username . . . . . . . : '%s'", CONFIG_STRING(ptr_proxy->options[PROXY_OPTION_USERNAME]));
        log_printf ("  password . . . . . . . : '%s'", CONFIG_STRING(ptr_proxy->options[PROXY_OPTION_PASSWORD]));
        log_printf ("  prev_proxy . . . . . . : %p", ptr_proxy->prev_proxy);
        log_printf ("  next_proxy . . . . . . : %p", ptr_proxy->next_proxy);
    }
}
