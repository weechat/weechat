/*
 * irc-ignore.c - ignore (nicks/hosts) management for IRC plugin
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

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-ignore.h"
#include "irc-channel.h"
#include "irc-server.h"


struct t_irc_ignore *irc_ignore_list = NULL; /* list of ignore              */
struct t_irc_ignore *last_irc_ignore = NULL; /* last ignore in list         */


/*
 * Checks if an ignore pointer is valid.
 *
 * Returns:
 *   1: ignore exists
 *   0: ignore does not exist
 */

int
irc_ignore_valid (struct t_irc_ignore *ignore)
{
    struct t_irc_ignore *ptr_ignore;

    if (!ignore)
        return 0;

    for (ptr_ignore = irc_ignore_list; ptr_ignore;
         ptr_ignore = ptr_ignore->next_ignore)
    {
        if (ptr_ignore == ignore)
            return 1;
    }

    /* ignore not found */
    return 0;
}

/*
 * Searches for an ignore.
 *
 * Returns pointer to ignore found, NULL if not found.
 */

struct t_irc_ignore *
irc_ignore_search (const char *mask, const char *server, const char *channel)
{
    struct t_irc_ignore *ptr_ignore;
    char any[2] = "*";

    if (!mask)
        return NULL;

    if (!server)
        server = any;
    if (!channel)
        channel = any;

    for (ptr_ignore = irc_ignore_list; ptr_ignore;
         ptr_ignore = ptr_ignore->next_ignore)
    {
        if ((strcmp (ptr_ignore->mask, mask) == 0)
            && (strcmp (ptr_ignore->server, server) == 0)
            && (weechat_strcasecmp (ptr_ignore->channel, channel) == 0))
        {
            return ptr_ignore;
        }
    }

    /* ignore not found */
    return NULL;
}

/*
 * Searches for an ignore by number (first is #1).
 *
 * Returns pointer to ignore found, NULL if not found.
 */

struct t_irc_ignore *
irc_ignore_search_by_number (int number)
{
    struct t_irc_ignore *ptr_ignore;

    for (ptr_ignore = irc_ignore_list; ptr_ignore;
         ptr_ignore = ptr_ignore->next_ignore)
    {
        if (ptr_ignore->number == number)
            return ptr_ignore;
    }

    /* ignore not found */
    return NULL;
}

/*
 * Adds a new ignore.
 *
 * Returns pointer to new ignore, NULL if error.
 */

struct t_irc_ignore *
irc_ignore_new (const char *mask, const char *server, const char *channel)
{
    struct t_irc_ignore *new_ignore;
    regex_t *regex;

    if (!mask || !mask[0])
        return NULL;

    regex = malloc (sizeof (*regex));
    if (!regex)
        return NULL;

    if (weechat_string_regcomp (regex, mask,
                                REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0)
    {
        free (regex);
        return NULL;
    }

    new_ignore = malloc (sizeof (*new_ignore));
    if (new_ignore)
    {
        new_ignore->number = (last_irc_ignore) ? last_irc_ignore->number + 1 : 1;
        new_ignore->mask = strdup (mask);
        new_ignore->regex_mask = regex;
        new_ignore->server = (server) ? strdup (server) : strdup ("*");
        new_ignore->channel = (channel) ? strdup (channel) : strdup ("*");

        /* add ignore to ignore list */
        new_ignore->prev_ignore = last_irc_ignore;
        if (last_irc_ignore)
            last_irc_ignore->next_ignore = new_ignore;
        else
            irc_ignore_list = new_ignore;
        last_irc_ignore = new_ignore;
        new_ignore->next_ignore = NULL;
    }

    return new_ignore;
}

/*
 * Checks if an ignore matches a server name.
 *
 * Returns:
 *   1: ignore matches the server name
 *   0: ignore does not match the server name
 */

int
irc_ignore_check_server (struct t_irc_ignore *ignore, const char *server)
{
    if (strcmp (ignore->server, "*") == 0)
        return 1;

    return (server && (strcmp (ignore->server, server) == 0)) ? 1 : 0;
}

/*
 * Checks if an ignore matches a channel name (or a nick if the channel name
 * is not a valid channel name).
 *
 * Returns:
 *   1: ignore matches the channel name
 *   0: ignore does not match the channel name
 */

int
irc_ignore_check_channel (struct t_irc_ignore *ignore,
                          struct t_irc_server *server,
                          const char *channel, const char *nick)
{
    if (!channel || (strcmp (ignore->channel, "*") == 0))
        return 1;

    if (irc_channel_is_channel (server, channel))
        return (weechat_strcasecmp (ignore->channel, channel) == 0) ? 1 : 0;

    if (nick)
        return (weechat_strcasecmp (ignore->channel, nick) == 0) ? 1 : 0;

    return 0;
}

/*
 * Checks if an ignore matches a host.
 *
 * Returns:
 *   1: ignore matches the host
 *   0: ignore does not match the host
 */

int
irc_ignore_check_host (struct t_irc_ignore *ignore,
                       const char *nick, const char *host)
{
    const char *pos;

    if (nick && (regexec (ignore->regex_mask, nick, 0, NULL, 0) == 0))
        return 1;

    if (host)
    {
        if (regexec (ignore->regex_mask, host, 0, NULL, 0) == 0)
            return 1;

        if (!strchr (ignore->mask, '!'))
        {
            pos = strchr (host, '!');
            if (pos && (regexec (ignore->regex_mask, pos + 1,
                                 0, NULL, 0) == 0))
            {
                return 1;
            }
        }
    }

    return 0;
}

/*
 * Checks if a message (from an IRC server) should be ignored or not.
 *
 * Returns:
 *   1: message must be ignored
 *   0: message must not be ignored
 */

int
irc_ignore_check (struct t_irc_server *server, const char *channel,
                  const char *nick, const char *host)
{
    struct t_irc_ignore *ptr_ignore;

    if (!server)
        return 0;

    /*
     * if nick is the same as server, then we will not ignore
     * (it is possible when connected to an irc proxy)
     */
    if (nick && server->nick
        && (irc_server_strcasecmp (server, server->nick, nick) == 0))
    {
        return 0;
    }

    for (ptr_ignore = irc_ignore_list; ptr_ignore;
         ptr_ignore = ptr_ignore->next_ignore)
    {
        if (irc_ignore_check_server (ptr_ignore, server->name)
            && irc_ignore_check_channel (ptr_ignore, server, channel, nick))
        {
            if (irc_ignore_check_host (ptr_ignore, nick, host))
                return 1;
        }
    }

    return 0;
}

/*
 * Removes an ignore.
 */

void
irc_ignore_free (struct t_irc_ignore *ignore)
{
    struct t_irc_ignore *ptr_ignore;

    if (!ignore)
        return;

    (void) weechat_hook_signal_send ("irc_ignore_removing",
                                     WEECHAT_HOOK_SIGNAL_POINTER, ignore);

    /* decrement number for all ignore after this one */
    for (ptr_ignore = ignore->next_ignore; ptr_ignore;
         ptr_ignore = ptr_ignore->next_ignore)
    {
        ptr_ignore->number--;
    }

    /* free data */
    free (ignore->mask);
    if (ignore->regex_mask)
    {
        regfree (ignore->regex_mask);
        free (ignore->regex_mask);
    }
    free (ignore->server);
    free (ignore->channel);

    /* remove ignore from list */
    if (ignore->prev_ignore)
        (ignore->prev_ignore)->next_ignore = ignore->next_ignore;
    if (ignore->next_ignore)
        (ignore->next_ignore)->prev_ignore = ignore->prev_ignore;
    if (irc_ignore_list == ignore)
        irc_ignore_list = ignore->next_ignore;
    if (last_irc_ignore == ignore)
        last_irc_ignore = ignore->prev_ignore;

    free (ignore);

    (void) weechat_hook_signal_send ("irc_ignore_removed",
                                     WEECHAT_HOOK_SIGNAL_STRING, NULL);
}

/*
 * Removes all ignores.
 */

void
irc_ignore_free_all (void)
{
    while (irc_ignore_list)
    {
        irc_ignore_free (irc_ignore_list);
    }
}

/*
 * Returns hdata for ignore.
 */

struct t_hdata *
irc_ignore_hdata_ignore_cb (const void *pointer, void *data,
                            const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = weechat_hdata_new (hdata_name, "prev_ignore", "next_ignore",
                               0, 0, NULL, NULL);
    if (hdata)
    {
        WEECHAT_HDATA_VAR(struct t_irc_ignore, number, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_ignore, mask, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_ignore, regex_mask, POINTER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_ignore, server, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_ignore, channel, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_ignore, prev_ignore, POINTER, 0, NULL, hdata_name);
        WEECHAT_HDATA_VAR(struct t_irc_ignore, next_ignore, POINTER, 0, NULL, hdata_name);
        WEECHAT_HDATA_LIST(irc_ignore_list, WEECHAT_HDATA_LIST_CHECK_POINTERS);
        WEECHAT_HDATA_LIST(last_irc_ignore, 0);
    }
    return hdata;
}

/*
 * Adds an ignore in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_ignore_add_to_infolist (struct t_infolist *infolist,
                            struct t_irc_ignore *ignore)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !ignore)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_string (ptr_item, "mask", ignore->mask))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "server", ignore->server))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "channel", ignore->channel))
        return 0;

    return 1;
}

/*
 * Prints ignore infos in WeeChat log file (usually for crash dump).
 */

void
irc_ignore_print_log (void)
{
    struct t_irc_ignore *ptr_ignore;

    for (ptr_ignore = irc_ignore_list; ptr_ignore;
         ptr_ignore = ptr_ignore->next_ignore)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[ignore (addr:%p)]", ptr_ignore);
        weechat_log_printf ("  number . . . . . . . : %d", ptr_ignore->number);
        weechat_log_printf ("  mask . . . . . . . . : '%s'", ptr_ignore->mask);
        weechat_log_printf ("  regex_mask . . . . . : %p", ptr_ignore->regex_mask);
        weechat_log_printf ("  server . . . . . . . : '%s'", ptr_ignore->server);
        weechat_log_printf ("  channel. . . . . . . : '%s'", ptr_ignore->channel);
        weechat_log_printf ("  prev_ignore. . . . . : %p", ptr_ignore->prev_ignore);
        weechat_log_printf ("  next_ignore. . . . . : %p", ptr_ignore->next_ignore);
    }
}
