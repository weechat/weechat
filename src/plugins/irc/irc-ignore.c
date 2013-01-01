/*
 * irc-ignore.c - ignore (nicks/hosts) management for IRC plugin
 *
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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

    if (!server)
        server = any;
    if (!channel)
        channel = any;

    for (ptr_ignore = irc_ignore_list; ptr_ignore;
         ptr_ignore = ptr_ignore->next_ignore)
    {
        if ((strcmp (ptr_ignore->mask, mask) == 0)
            && (weechat_strcasecmp (ptr_ignore->server, server) == 0)
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
    char *complete_mask;

    if (!mask || !mask[0])
        return NULL;

    complete_mask = malloc (1 + strlen (mask) + 1 + 1);
    if (!complete_mask)
        return NULL;

    if (mask[0] == '^')
        strcpy (complete_mask, mask);
    else
    {
        strcpy (complete_mask, "^");
        strcat (complete_mask, mask);
    }
    if (complete_mask[strlen (complete_mask) - 1] != '$')
        strcat (complete_mask, "$");

    regex = malloc (sizeof (*regex));
    if (!regex)
    {
        free (complete_mask);
        return NULL;
    }

    if (weechat_string_regcomp (regex, complete_mask,
                                REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0)
    {
        free (regex);
        free (complete_mask);
        return NULL;
    }

    new_ignore = malloc (sizeof (*new_ignore));
    if (new_ignore)
    {
        new_ignore->number = (last_irc_ignore) ? last_irc_ignore->number + 1 : 1;
        new_ignore->mask = strdup (complete_mask);
        new_ignore->regex_mask = regex;
        new_ignore->server = (server) ? strdup (server) : strdup ("*");
        new_ignore->channel = (channel) ? strdup (channel) : strdup ("*");

        /* add ignore to ignore list */
        new_ignore->prev_ignore = last_irc_ignore;
        if (irc_ignore_list)
            last_irc_ignore->next_ignore = new_ignore;
        else
            irc_ignore_list = new_ignore;
        last_irc_ignore = new_ignore;
        new_ignore->next_ignore = NULL;
    }

    free (complete_mask);

    return new_ignore;
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
    int server_match, channel_match;

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
        server_match = 0;
        channel_match = 0;

        if (strcmp (ptr_ignore->server, "*") == 0)
            server_match = 1;
        else
            server_match = (weechat_strcasecmp (ptr_ignore->server,
                                                server->name) == 0);

        if (!channel || (strcmp (ptr_ignore->channel, "*") == 0))
            channel_match = 1;
        else
        {
            if (irc_channel_is_channel (server, channel))
            {
                channel_match = (weechat_strcasecmp (ptr_ignore->channel,
                                                     channel) == 0);
            }
            else if (nick)
            {
                channel_match = (weechat_strcasecmp (ptr_ignore->channel,
                                                     nick) == 0);
            }
        }

        if (server_match && channel_match)
        {
            if (nick && (regexec (ptr_ignore->regex_mask, nick, 0, NULL, 0) == 0))
                return 1;
            if (host && (regexec (ptr_ignore->regex_mask, host, 0, NULL, 0) == 0))
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

    weechat_hook_signal_send ("irc_ignore_removing",
                              WEECHAT_HOOK_SIGNAL_POINTER, ignore);

    /* decrement number for all ignore after this one */
    for (ptr_ignore = ignore->next_ignore; ptr_ignore;
         ptr_ignore = ptr_ignore->next_ignore)
    {
        ptr_ignore->number--;
    }

    /* free data */
    if (ignore->mask)
        free (ignore->mask);
    if (ignore->regex_mask)
    {
        regfree (ignore->regex_mask);
        free (ignore->regex_mask);
    }
    if (ignore->server)
        free (ignore->server);
    if (ignore->channel)
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

    weechat_hook_signal_send ("irc_ignore_removed",
                              WEECHAT_HOOK_SIGNAL_STRING, NULL);
}

/*
 * Removes all ignores.
 */

void
irc_ignore_free_all ()
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
irc_ignore_hdata_ignore_cb (void *data, const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
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
        WEECHAT_HDATA_LIST(irc_ignore_list);
        WEECHAT_HDATA_LIST(last_irc_ignore);
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
irc_ignore_print_log ()
{
    struct t_irc_ignore *ptr_ignore;

    for (ptr_ignore = irc_ignore_list; ptr_ignore;
         ptr_ignore = ptr_ignore->next_ignore)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[ignore (addr:0x%lx)]", ptr_ignore);
        weechat_log_printf ("  number . . . . . . . : %d",    ptr_ignore->number);
        weechat_log_printf ("  mask . . . . . . . . : '%s'",  ptr_ignore->mask);
        weechat_log_printf ("  regex_mask . . . . . : 0x%lx", ptr_ignore->regex_mask);
        weechat_log_printf ("  server . . . . . . . : '%s'",  ptr_ignore->server);
        weechat_log_printf ("  channel. . . . . . . : '%s'",  ptr_ignore->channel);
        weechat_log_printf ("  prev_ignore. . . . . : 0x%lx", ptr_ignore->prev_ignore);
        weechat_log_printf ("  next_ignore. . . . . : 0x%lx", ptr_ignore->next_ignore);
    }
}
