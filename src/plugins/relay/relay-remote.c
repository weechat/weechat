/*
 * relay-remote.c - remote relay server functions for relay plugin
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
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <gnutls/gnutls.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-config.h"
#include "relay-remote.h"


char *relay_remote_option_string[RELAY_REMOTE_NUM_OPTIONS] =
{ "url", "proxy", "password", "totp_secret" };
char *relay_remote_option_default[RELAY_REMOTE_NUM_OPTIONS] =
{ "", "", "", "" };

struct t_relay_remote *relay_remotes = NULL;
struct t_relay_remote *last_relay_remote = NULL;
int relay_remotes_count = 0;            /* number of remotes                */

struct t_relay_remote *relay_remotes_temp = NULL;     /* first temp. remote */
struct t_relay_remote *last_relay_remote_temp = NULL; /* last temp. remote  */


/*
 * Searches for a remote option name.
 *
 * Returns index of option in enum t_relay_remote_option, -1 if not found.
 */

int
relay_remote_search_option (const char *option_name)
{
    int i;

    if (!option_name)
        return -1;

    for (i = 0; i < RELAY_REMOTE_NUM_OPTIONS; i++)
    {
        if (strcmp (relay_remote_option_string[i], option_name) == 0)
            return i;
    }

    /* remote option not found */
    return -1;
}

/*
 * Checks if a remote pointer is valid.
 *
 * Returns:
 *   1: remote exists
 *   0: remote does not exist
 */

int
relay_remote_valid (struct t_relay_remote *remote)
{
    struct t_relay_remote *ptr_remote;

    if (!remote)
        return 0;

    for (ptr_remote = relay_remotes; ptr_remote;
         ptr_remote = ptr_remote->next_remote)
    {
        if (ptr_remote == remote)
            return 1;
    }

    /* remote not found */
    return 0;
}

/*
 * Searches for a remote by name.
 *
 * Returns pointer to remote found, NULL if not found.
 */

struct t_relay_remote *
relay_remote_search (const char *name)
{
    struct t_relay_remote *ptr_remote;

    if (!name || !name[0])
        return NULL;

    for (ptr_remote = relay_remotes; ptr_remote;
         ptr_remote = ptr_remote->next_remote)
    {
        if (strcmp (ptr_remote->name, name) == 0)
            return ptr_remote;
    }

    /* remote not found */
    return NULL;
}

/*
 * Searches for a remote by number (first remote is 0).
 *
 * Returns pointer to remote found, NULL if not found.
 */

struct t_relay_remote *
relay_remote_search_by_number (int number)
{
    struct t_relay_remote *ptr_remote;
    int i;

    i = 0;
    for (ptr_remote = relay_remotes; ptr_remote;
         ptr_remote = ptr_remote->next_remote)
    {
        if (i == number)
            return ptr_remote;
        i++;
    }

    /* remote not found */
    return NULL;
}

/*
 * Checks if a remote name is valid: it must contain only alphabetic chars
 * or digits.
 *
 * Returns:
 *   1: name is valid
 *   0: name is invalid
 */

int
relay_remote_name_valid (const char *name)
{
    const char *ptr_name;

    if (!name || !name[0])
        return 0;

    ptr_name = name;
    while (ptr_name[0])
    {
        if (!isalnum ((unsigned char)ptr_name[0]))
            return 0;
        ptr_name++;
    }

    /* name is valid */
    return 1;
}

/*
 * Checks if a remote URL is valid;
 *
 * Returns:
 *   1: URL is valid
 *   0: URL is invalid
 */

int
relay_remote_url_valid (const char *url)
{
    const char *pos;

    if (!url || !url[0])
        return 0;

    /* URL must start with "https://" or "http://" */
    if ((strncmp (url, "https://", 8) != 0) && (strncmp (url, "http://", 7) != 0))
        return 0;

    pos = strchr (url + 7, ':');

    /* invalid port? */
    if (pos && !isdigit ((unsigned char)pos[1]))
        return 0;

    /* URL is valid */
    return 1;
}

/*
 * Sends a signal with the status of remote ("relay_remote_xxx").
 */

void
relay_remote_send_signal (struct t_relay_remote *remote)
{
    char signal[128];

    snprintf (signal, sizeof (signal),
              "relay_remote_%s",
              relay_status_name[remote->status]);
    weechat_hook_signal_send (signal, WEECHAT_HOOK_SIGNAL_POINTER, remote);
}

/*
 * Allocates and initializes new remote structure.
 *
 * Returns pointer to new remote, NULL if error.
 */

struct t_relay_remote *
relay_remote_alloc (const char *name)
{
    struct t_relay_remote *new_remote;
    int i;

    if (!relay_remote_name_valid (name))
        return NULL;

    if (relay_remote_search (name))
        return NULL;

    new_remote = malloc (sizeof (*new_remote));
    if (!new_remote)
        return NULL;

    new_remote->name = strdup (name);
    for (i = 0; i < RELAY_REMOTE_NUM_OPTIONS; i++)
    {
        new_remote->options[i] = NULL;
    }
    new_remote->address = NULL;
    new_remote->port = 0;
    new_remote->tls = 0;
    new_remote->status = RELAY_STATUS_DISCONNECTED;
    new_remote->sock = -1;
    new_remote->gnutls_sess = NULL;
    new_remote->prev_remote = NULL;
    new_remote->next_remote = NULL;

    return new_remote;
}

/*
 * Searches for position of remote in list (to keep remotes sorted by name).
 */

struct t_relay_remote *
relay_remote_find_pos (struct t_relay_remote *remote,
                       struct t_relay_remote *list_remotes)
{
    struct t_relay_remote *ptr_remote;

    for (ptr_remote = list_remotes; ptr_remote;
         ptr_remote = ptr_remote->next_remote)
    {
        if (weechat_strcmp (remote->name, ptr_remote->name) < 0)
            return ptr_remote;
    }

    /* position not found */
    return NULL;
}

/*
 * Adds a remote in a linked list.
 */

void
relay_remote_add (struct t_relay_remote *remote,
                  struct t_relay_remote **list_remotes,
                  struct t_relay_remote **last_list_remote)
{
    struct t_relay_remote *pos_remote;

    pos_remote = relay_remote_find_pos (remote, *list_remotes);
    if (pos_remote)
    {
        /* add remote before "pos_remote" */
        remote->prev_remote = pos_remote->prev_remote;
        remote->next_remote = pos_remote;
        if (pos_remote->prev_remote)
            (pos_remote->prev_remote)->next_remote = remote;
        else
            *list_remotes = remote;
        pos_remote->prev_remote = remote;
    }
    else
    {
        /* add remote to end of list */
        remote->prev_remote = *last_list_remote;
        remote->next_remote = NULL;
        if (*last_list_remote)
            (*last_list_remote)->next_remote = remote;
        else
            *list_remotes = remote;
        *last_list_remote = remote;
    }
}

/*
 * Creates a new remote with options.
 *
 * Returns pointer to new remote, NULL if error.
 */

struct t_relay_remote *
relay_remote_new_with_options (const char *name, struct t_config_option **options)
{
    struct t_relay_remote *new_remote;
    int i;

    new_remote = relay_remote_alloc (name);
    if (!new_remote)
        return NULL;

    if (!relay_remote_url_valid (weechat_config_string (options[RELAY_REMOTE_OPTION_URL])))
    {
        free (new_remote);
        return NULL;
    }

    for (i = 0; i < RELAY_REMOTE_NUM_OPTIONS; i++)
    {
        new_remote->options[i] = options[i];
    }
    relay_remote_add (new_remote, &relay_remotes, &last_relay_remote);
    relay_remotes_count++;

    relay_remote_send_signal (new_remote);

    return new_remote;
}

/*
 * Creates a new remote.
 *
 * Returns pointer to new remote, NULL if error.
 */

struct t_relay_remote *
relay_remote_new (const char *name, const char *url, const char *proxy,
                  const char *password, const char *totp_secret)
{
    struct t_config_option *option[RELAY_REMOTE_NUM_OPTIONS];
    const char *value[RELAY_REMOTE_NUM_OPTIONS];
    struct t_relay_remote *new_remote;
    int i;

    if (!name || !name[0] || !url || !url[0])
        return NULL;

    new_remote = malloc (sizeof (*new_remote));
    if (!new_remote)
        return NULL;

    value[RELAY_REMOTE_OPTION_URL] = url;
    value[RELAY_REMOTE_OPTION_PROXY] = proxy;
    value[RELAY_REMOTE_OPTION_PASSWORD] = password;
    value[RELAY_REMOTE_OPTION_TOTP_SECRET] = totp_secret;

    for (i = 0; i < RELAY_REMOTE_NUM_OPTIONS; i++)
    {
        option[i] = relay_config_create_remote_option (name, i, value[i]);
    }

    new_remote = relay_remote_new_with_options (name, option);
    if (!new_remote)
    {
        for (i = 0; i < RELAY_REMOTE_NUM_OPTIONS; i++)
        {
            weechat_config_option_free (option[i]);
        }
    }

    return new_remote;
}

/*
 * Creates a new remote using an infolist.
 *
 * This is called to restore remotes after /upgrade.
 */

struct t_relay_remote *
relay_remote_new_with_infolist (struct t_infolist *infolist)
{
    struct t_relay_remote *new_remote;

    new_remote = malloc (sizeof (*new_remote));
    if (!new_remote)
        return NULL;

    new_remote->address = strdup (weechat_infolist_string (infolist, "name"));
    new_remote->address = strdup (weechat_infolist_string (infolist, "address"));
    new_remote->port = weechat_infolist_integer (infolist, "port");
    new_remote->tls = weechat_infolist_integer (infolist, "tls");
    new_remote->status = weechat_infolist_integer (infolist, "status");
    new_remote->sock = weechat_infolist_integer (infolist, "sock");
    new_remote->gnutls_sess = NULL;
    new_remote->prev_remote = NULL;
    new_remote->next_remote = relay_remotes;
    if (relay_remotes)
        relay_remotes->prev_remote = new_remote;
    else
        last_relay_remote = new_remote;
    relay_remotes = new_remote;

    relay_remotes_count++;

    return new_remote;
}

/*
 * Sets status for a remote.
 */

void
relay_remote_set_status (struct t_relay_remote *remote,
                         enum t_relay_status status)
{
    /*
     * IMPORTANT: if changes are made in this function or sub-functions called,
     * please also update the function relay_remote_add_to_infolist:
     * when the flag force_disconnected_state is set to 1 we simulate
     * a disconnected state for remote in infolist (used on /upgrade -save)
     */

    remote->status = status;

    relay_remote_send_signal (remote);
}

/*
 * Renames a remote.
 *
 * Returns:
 *   1: OK
 *   0: error (remote not renamed)
 */

int
relay_remote_rename (struct t_relay_remote *remote, const char *name)
{
    int length, i;
    char *option_name;

    if (!remote || !name || !name[0] || !relay_remote_name_valid (name)
        || relay_remote_search (name))
    {
        return 0;
    }

    length = strlen (name) + 64;
    option_name = malloc (length);
    if (!option_name)
        return 0;

    for (i = 0; i < RELAY_REMOTE_NUM_OPTIONS; i++)
    {
        if (remote->options[i])
        {
            snprintf (option_name, length,
                      "%s.%s",
                      name,
                      relay_remote_option_string[i]);
            weechat_config_option_rename (remote->options[i], option_name);
        }
    }

    if (remote->name)
        free (remote->name);
    remote->name = strdup (name);

    free (option_name);

    /* re-insert remote in list (for sorting remotes by name) */
    if (remote->prev_remote)
        (remote->prev_remote)->next_remote = remote->next_remote;
    else
        relay_remotes = remote->next_remote;
    if (remote->next_remote)
        (remote->next_remote)->prev_remote = remote->prev_remote;
    else
        last_relay_remote = remote->prev_remote;
    relay_remote_add (remote, &relay_remotes, &last_relay_remote);

    return 1;
}

/*
 * Deletes a remote.
 */

void
relay_remote_free (struct t_relay_remote *remote)
{
    int i;

    if (!remote)
        return;

    /* remove remote from list */
    if (remote->prev_remote)
        (remote->prev_remote)->next_remote = remote->next_remote;
    if (remote->next_remote)
        (remote->next_remote)->prev_remote = remote->prev_remote;
    if (relay_remotes == remote)
        relay_remotes = remote->next_remote;
    if (last_relay_remote == remote)
        last_relay_remote = remote->prev_remote;

    /* free data */
    if (remote->name)
        free (remote->name);
    for (i = 0; i < RELAY_REMOTE_NUM_OPTIONS; i++)
    {
        if (remote->options[i])
            weechat_config_option_free (remote->options[i]);
    }
    if (remote->address)
        free (remote->address);

    free (remote);

    relay_remotes_count--;
}

/*
 * Removes all remotes.
 */

void
relay_remote_free_all ()
{
    while (relay_remotes)
    {
        relay_remote_free (relay_remotes);
    }
}

/*
 * Disconnects one remote.
 */

void
relay_remote_disconnect (struct t_relay_remote *remote)
{
    if (remote->sock >= 0)
    {
        relay_remote_set_status (remote, RELAY_STATUS_DISCONNECTED);
    }
}

/*
 * Disconnects all remotes.
 */

void
relay_remote_disconnect_all ()
{
    struct t_relay_remote *ptr_remote;

    for (ptr_remote = relay_remotes; ptr_remote;
         ptr_remote = ptr_remote->next_remote)
    {
        relay_remote_disconnect (ptr_remote);
    }
}

/*
 * Adds a remote in an infolist.
 *
 * If force_disconnected_state == 1, the infolist contains the remote
 * in a disconnected state (but the remote is unchanged, still connected if it
 * was).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
relay_remote_add_to_infolist (struct t_infolist *infolist,
                              struct t_relay_remote *remote,
                              int force_disconnected_state)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !remote)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_string (ptr_item, "name", remote->name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "address", remote->address))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "port", remote->port))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "tls", remote->tls))
        return 0;
    if (!RELAY_STATUS_HAS_ENDED(remote->status) && force_disconnected_state)
    {
        if (!weechat_infolist_new_var_integer (ptr_item, "status", RELAY_STATUS_DISCONNECTED))
            return 0;
        if (!weechat_infolist_new_var_integer (ptr_item, "sock", -1))
            return 0;
    }
    else
    {
        if (!weechat_infolist_new_var_integer (ptr_item, "status", remote->status))
            return 0;
        if (!weechat_infolist_new_var_integer (ptr_item, "sock", remote->sock))
            return 0;
    }

    return 1;
}

/*
 * Prints remotes in WeeChat log file (usually for crash dump).
 */

void
relay_remote_print_log ()
{
    struct t_relay_remote *ptr_remote;

    for (ptr_remote = relay_remotes; ptr_remote;
         ptr_remote = ptr_remote->next_remote)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[relay remote (addr:0x%lx)]", ptr_remote);
        weechat_log_printf ("  name. . . . . . . . . : '%s'", ptr_remote->name);
        weechat_log_printf ("  url . . . . . . . . . : '%s'",
                            weechat_config_string (ptr_remote->options[RELAY_REMOTE_OPTION_URL]));
        weechat_log_printf ("  proxy . . . . . . . . : '%s'",
                            weechat_config_string (ptr_remote->options[RELAY_REMOTE_OPTION_PROXY]));
        weechat_log_printf ("  password. . . . . . . : '%s'",
                            weechat_config_string (ptr_remote->options[RELAY_REMOTE_OPTION_PASSWORD]));
        weechat_log_printf ("  totp_secret . . . . . : '%s'",
                            weechat_config_string (ptr_remote->options[RELAY_REMOTE_OPTION_TOTP_SECRET]));
        weechat_log_printf ("  address . . . . . . . : '%s'", ptr_remote->address);
        weechat_log_printf ("  port. . . . . . . . . : %d", ptr_remote->port);
        weechat_log_printf ("  tls . . . . . . . . . : %d", ptr_remote->tls);
        weechat_log_printf ("  status. . . . . . . . : %d (%s)",
                            ptr_remote->status,
                            relay_status_string[ptr_remote->status]);
        weechat_log_printf ("  sock. . . . . . . . . : %d", ptr_remote->sock);
        weechat_log_printf ("  gnutls_sess . . . . . : 0x%lx", ptr_remote->gnutls_sess);
        weechat_log_printf ("  prev_remote . . . . . : 0x%lx", ptr_remote->prev_remote);
        weechat_log_printf ("  next_remote . . . . . : 0x%lx", ptr_remote->next_remote);
    }
}
