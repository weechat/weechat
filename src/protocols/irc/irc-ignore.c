/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

/* irc-ignore.c: manages IRC ignore list */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../../common/weechat.h"
#include "irc.h"
#include "../../common/command.h"
#include "../../common/log.h"
#include "../../common/util.h"


t_irc_ignore *irc_ignore = NULL;
t_irc_ignore *last_irc_ignore = NULL;


/*
 * irc_ignore_check_mask: return 1 is mask1 and mask2 are the same host
 *                        anyone or both strings may have user and/or host after
 */

int
irc_ignore_check_mask (char *mask1, char *mask2)
{
    char *m1, *m2, *pos;
    int match;
    
    if (!mask1 || !mask1[0] || !mask2 || !mask2[0])
        return 0;
    
    m1 = strdup (mask1);
    m2 = strdup (mask2);
    
    pos = strchr (m1, '!');
    if (!pos)
    {
        /* remove '!' from m2 */
        pos = strchr (m2, '!');
        if (pos)
            pos[0] = '\0';
    }
    pos = strchr (m2, '!');
    if (!pos)
    {
        /* remove '!' from m1 */
        pos = strchr (m1, '!');
        if (pos)
            pos[0] = '\0';
    }
    
    /* TODO: use regexp to match both masks */
    match = ascii_strcasecmp (m1, m2);
    
    free (m1);
    free (m2);
    
    return (match == 0);
}

/*
 * irc_ignore_match: check if pointed ignore matches with arguments
 */

int
irc_ignore_match (t_irc_ignore *ptr_ignore, char *mask, char *type,
                  char *channel_name, char *server_name)
{
    /* check mask */
    if ((strcmp (mask, "*") != 0) && (strcmp (ptr_ignore->mask, "*") != 0)
        && (!irc_ignore_check_mask (ptr_ignore->mask, mask)))
                return 0;
    
    /* mask is matching, go on with type */
    if ((strcmp (type, "*") != 0) && (strcmp (ptr_ignore->type, "*") != 0)
        && (ascii_strcasecmp (ptr_ignore->type, type) != 0))
        return 0;
    
    /* mask and type matching, go on with server */
    if (server_name && server_name[0])
    { 
        if ((strcmp (server_name, "*") != 0) && (strcmp (ptr_ignore->server_name, "*") != 0)
            && (ascii_strcasecmp (ptr_ignore->server_name, server_name) != 0))
            return 0;
    }
    else
    {
        if (strcmp (ptr_ignore->server_name, "*") != 0)
            return 0;
    }
    
    /* mask, type and server matching, go on with channel */
    if (channel_name && channel_name[0])
    { 
        if ((strcmp (channel_name, "*") != 0) && (strcmp (ptr_ignore->channel_name, "*") != 0)
            && (ascii_strcasecmp (ptr_ignore->channel_name, channel_name) != 0))
            return 0;
    }
    else
    {
        if (strcmp (ptr_ignore->channel_name, "*") != 0)
            return 0;
    }
    
    /* all is matching => we find a ignore! */
    return 1;
}

/*
 * irc_ignore_check: check if an ignore is set for arguments
 *                   return 1 if at least one ignore exists (message should NOT be displayed)
 *                          0 if no ignore found (message will be displayed)
 */

int
irc_ignore_check (char *mask, char *type, char *channel_name, char *server_name)
{
    t_irc_ignore *ptr_ignore;
    
    if (!mask || !mask[0] || !type || !type[0])
        return 0;
    
    for (ptr_ignore = irc_ignore; ptr_ignore;
         ptr_ignore = ptr_ignore->next_ignore)
    {
        if (irc_ignore_match (ptr_ignore, mask, type, channel_name, server_name))
            return 1;
    }
    
    /* no ignore found */
    return 0;
}

/*
 * irc_ignore_search: search for an ignore
 */

t_irc_ignore *
irc_ignore_search (char *mask, char *type, char *channel_name, char *server_name)
{
    t_irc_ignore *ptr_ignore;
    
    for (ptr_ignore = irc_ignore; ptr_ignore;
         ptr_ignore = ptr_ignore->next_ignore)
    {
        if ((ascii_strcasecmp (ptr_ignore->mask, mask) == 0)
            && (ascii_strcasecmp (ptr_ignore->type, type) == 0)
            && (ascii_strcasecmp (ptr_ignore->channel_name, channel_name) == 0)
            && (ascii_strcasecmp (ptr_ignore->server_name, server_name) == 0))
            return ptr_ignore;
    }
    
    /* ignore not found */
    return NULL;
}

/*
 * irc_ignore_add: add an ignore in list
 */

t_irc_ignore *
irc_ignore_add (char *mask, char *type, char *channel_name, char *server_name)
{
    int type_index;
    t_irc_command *command_ptr;
    t_irc_ignore *new_ignore;

    if (!mask || !mask[0] || !type || !type[0] || !channel_name || !channel_name[0]
        || !server_name || !server_name[0])
    {
        irc_display_prefix (NULL, NULL, GUI_PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s too few arguments for ignore\n"),
                    WEECHAT_ERROR);
        return NULL;
    }
    
#ifdef DEBUG
    weechat_log_printf ("Adding ignore: mask:'%s', type:'%s', channel:'%s', "
                        "server:'%s'\n",
                        mask, type, channel_name, server_name);
#endif
    
    type_index = -1;
    command_ptr = NULL;
    
    if ((strcmp (mask, "*") == 0) && (strcmp (type, "*") == 0))
    {
        irc_display_prefix (NULL, NULL, GUI_PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s mask or type/command should be non generic value for ignore\n"),
                    WEECHAT_ERROR);
        return NULL;
    }
    
    if (irc_ignore_search (mask, type, channel_name, server_name))
    {
        irc_display_prefix (NULL, NULL, GUI_PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s ignore already exists\n"),
                    WEECHAT_ERROR);
        return NULL;
    }
    
    /* create new ignore */
    new_ignore = (t_irc_ignore *) malloc (sizeof (t_irc_ignore));
    if (new_ignore)
    {
        new_ignore->mask = strdup (mask);
        new_ignore->type = strdup (type);
        new_ignore->server_name = strdup (server_name);
        new_ignore->channel_name = strdup (channel_name);
        
        /* add new ignore to queue */
        new_ignore->prev_ignore = last_irc_ignore;
        new_ignore->next_ignore = NULL;
        if (irc_ignore)
            last_irc_ignore->next_ignore = new_ignore;
        else
            irc_ignore = new_ignore;
        last_irc_ignore = new_ignore;
    }
    else
    {
        irc_display_prefix (NULL, NULL, GUI_PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s not enough memory to create ignore\n"),
                    WEECHAT_ERROR);
        return NULL;
    }
    
    return new_ignore;
}

/*
 * irc_ignore_add_from_config: add an ignore to list, read from config file
 *                             (comma serparated values)
 */

t_irc_ignore *
irc_ignore_add_from_config (char *string)
{
    t_irc_ignore *new_ignore;
    char *string2;
    char *pos_mask, *pos_type, *pos_channel, *pos_server;
    
    if (!string || !string[0])
        return NULL;
    
    new_ignore = NULL;
    string2 = strdup (string);

    pos_mask = string2;
    pos_type = strchr (pos_mask, ',');
    if (pos_type)
    {
        pos_type[0] = '\0';
        pos_type++;
        pos_channel = strchr (pos_type, ',');
        if (pos_channel)
        {
            pos_channel[0] = '\0';
            pos_channel++;
            pos_server = strchr (pos_channel, ',');
            if (pos_server)
            {
                pos_server[0] = '\0';
                pos_server++;
                new_ignore = irc_ignore_add (pos_mask, pos_type, pos_channel, pos_server);
            }
        }
    }
    
    free (string2);
    return new_ignore;
}

/*
 * irc_ignore_free: free an ignore
 */

void
irc_ignore_free (t_irc_ignore *ptr_ignore)
{
    t_irc_ignore *new_irc_ignore;

    /* free data */
    if (ptr_ignore->mask)
        free (ptr_ignore->mask);
    if (ptr_ignore->type)
        free (ptr_ignore->type);
    if (ptr_ignore->channel_name)
        free (ptr_ignore->channel_name);
    if (ptr_ignore->server_name)
        free (ptr_ignore->server_name);
    
    /* remove ignore from queue */
    if (last_irc_ignore == ptr_ignore)
        last_irc_ignore = ptr_ignore->prev_ignore;
    if (ptr_ignore->prev_ignore)
    {
        (ptr_ignore->prev_ignore)->next_ignore = ptr_ignore->next_ignore;
        new_irc_ignore = irc_ignore;
    }
    else
        new_irc_ignore = ptr_ignore->next_ignore;
    
    if (ptr_ignore->next_ignore)
        (ptr_ignore->next_ignore)->prev_ignore = ptr_ignore->prev_ignore;
    
    free (ptr_ignore);
    irc_ignore = new_irc_ignore;
}

/*
 * irc_ignore_free_all: free all ignores
 */

void
irc_ignore_free_all ()
{
    while (irc_ignore)
        irc_ignore_free (irc_ignore);
}

/*
 * irc_ignore_search_free: search and free ignore(s)
 *                         return: number of ignore found and deleted
 *                                 0 if no ignore found
 */

int
irc_ignore_search_free (char *mask, char *type,
                        char *channel_name, char *server_name)
{
    int found;
    t_irc_ignore *ptr_ignore, *next_ignore;
    
    found = 0;
    ptr_ignore = irc_ignore;
    while (ptr_ignore)
    {
        if (irc_ignore_match (ptr_ignore, mask, type, channel_name, server_name))
        {
            found++;
            if (found == 1)
                gui_printf (NULL, "\n");
            irc_display_prefix (NULL, NULL, GUI_PREFIX_INFO);
            weechat_cmd_ignore_display (_("Removing ignore:"), ptr_ignore);
            next_ignore = ptr_ignore->next_ignore;
            irc_ignore_free (ptr_ignore);
            ptr_ignore = next_ignore;
        }
        else
            ptr_ignore = ptr_ignore->next_ignore;
    }
    
    return found;
}

/*
 * irc_ignore_search_free_by_number: search and free ignore(s) by number
 *                                   return: 1 if ignore found and deleted
 *                                           0 if ignore not found
 */

int
irc_ignore_search_free_by_number (int number)
{
    int i;
    t_irc_ignore *ptr_ignore;
    
    if (number < 1)
        return 0;
    
    i = 0;
    for (ptr_ignore = irc_ignore; ptr_ignore;
         ptr_ignore = ptr_ignore->next_ignore)
    {
        i++;
        if (i == number)
        {
            gui_printf (NULL, "\n");
            irc_display_prefix (NULL, NULL, GUI_PREFIX_INFO);
            weechat_cmd_ignore_display (_("Removing ignore:"), ptr_ignore);
            irc_ignore_free (ptr_ignore);
            return 1;
        }
    }
    
    /* ignore number not found */
    return 0;
}

/*
 * irc_ignore_print_log: print ignore list in log (usually for crash dump)
 */

void
irc_ignore_print_log ()
{
    t_irc_ignore *ptr_ignore;
    
    weechat_log_printf ("[ignore list]\n");
    
    for (ptr_ignore = irc_ignore; ptr_ignore;
         ptr_ignore = ptr_ignore->next_ignore)
    {
        weechat_log_printf ("\n");
        weechat_log_printf ("  -> ignore at 0x%X:\n", ptr_ignore);
        weechat_log_printf ("     mask. . . . . . . : %s\n",   ptr_ignore->mask);
        weechat_log_printf ("     type. . . . . . . : %s\n",   ptr_ignore->type);
        weechat_log_printf ("     channel_name. . . : %s\n",   ptr_ignore->channel_name);
        weechat_log_printf ("     server_name . . . : %s\n",   ptr_ignore->server_name);
        weechat_log_printf ("     prev_ignore . . . : 0x%X\n", ptr_ignore->prev_ignore);
        weechat_log_printf ("     next_ignore . . . : 0x%X\n", ptr_ignore->next_ignore);
    }
}
