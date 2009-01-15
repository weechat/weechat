/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* irc-ignore.c: manages ignore list (nicks/hosts) on IRC servers/channels */


#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-ignore.h"
#include "irc-channel.h"
#include "irc-server.h"


struct t_irc_ignore *irc_ignore_list = NULL; /* list of ignore              */
struct t_irc_ignore *last_irc_ignore = NULL; /* last ignore in list         */


/*
 * irc_ignore_valid: check if a ignore pointer exists
 *                   return 1 if ignore exists
 *                          0 if ignore is not found
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
 * irc_ignore_search: search a ignore
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
 * irc_ignore_search_by_number: search a ignore by number (first is #1)
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
 * irc_ignore_new: add new ignore
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
    
    if (regcomp (regex, complete_mask, REG_NOSUB | REG_ICASE) != 0)
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
 * irc_ignore_check: check if a message (from an IRC server) should be ignored
 *                   or not
 *                   return: 1 if message will be ignored
 *                           0 if message will be displayed (NOT ignored)
 */

int
irc_ignore_check (struct t_irc_server *server, struct t_irc_channel *channel,
                  const char *nick, const char *host)
{
    struct t_irc_ignore *ptr_ignore;
    int server_match, channel_match, regex_match;
    
    if (!server)
        return 0;
    
    for (ptr_ignore = irc_ignore_list; ptr_ignore;
         ptr_ignore = ptr_ignore->next_ignore)
    {
        server_match = 0;
        channel_match = 0;
        regex_match = 0;
        
        if (!server || (strcmp (ptr_ignore->server, "*") == 0))
            server_match = 1;
        else
            server_match = (weechat_strcasecmp (ptr_ignore->server,
                                                server->name) == 0);
        
        if (!channel || (strcmp (ptr_ignore->channel, "*") == 0))
            channel_match = 1;
        else
        {
            channel_match = (weechat_strcasecmp (ptr_ignore->channel,
                                                 channel->name) == 0);
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
 * irc_ignore_free: remove a ignore
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
    
    /* remove filter from filters list */
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
 * irc_ignore_free_all: remove all ignore
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
 * irc_ignore_add_to_infolist: add a ignore in an infolist
 *                             return 1 if ok, 0 if error
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
