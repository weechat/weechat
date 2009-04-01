/*
 * Copyright (c) 2009 by FlashCode <flashcode@flashtux.org>
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

/* jabber-muc.c: jabber MUC management */


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "jabber.h"
#include "jabber-muc.h"
#include "jabber-buffer.h"
#include "jabber-config.h"
#include "jabber-buddy.h"
#include "jabber-server.h"
#include "jabber-input.h"


/*
 * jabber_muc_valid: check if a MUC pointer exists for a server
 *                   return 1 if MUC exists
 *                          0 if MUC is not found
 */

int
jabber_muc_valid (struct t_jabber_server *server, struct t_jabber_muc *muc)
{
    struct t_jabber_muc *ptr_muc;
    
    if (!server)
        return 0;
    
    for (ptr_muc = server->mucs; ptr_muc; ptr_muc = ptr_muc->next_muc)
    {
        if (ptr_muc == muc)
            return 1;
    }
    
    /* MUC not found */
    return 0;
}

/*
 * jabber_muc_new: allocate a new MUC for a server and add it to MUC list
 */

struct t_jabber_muc *
jabber_muc_new (struct t_jabber_server *server, int muc_type,
                const char *muc_name, int switch_to_muc, int auto_switch)
{
    struct t_jabber_muc *new_muc;
    struct t_gui_buffer *new_buffer;
    int buffer_created;
    char *buffer_name;
    
    /* alloc memory for new MUCl */
    if ((new_muc = malloc (sizeof (*new_muc))) == NULL)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot allocate new MUC"),
                        weechat_prefix ("error"), JABBER_PLUGIN_NAME);
        return NULL;
    }
    
    /* create buffer for MUC (or use existing one) */
    buffer_created = 0;
    buffer_name = jabber_buffer_build_name (server->name, muc_name);
    new_buffer = weechat_buffer_search (JABBER_PLUGIN_NAME, buffer_name);
    if (new_buffer)
        weechat_nicklist_remove_all (new_buffer);
    else
    {
        new_buffer = weechat_buffer_new (buffer_name,
                                         &jabber_input_data_cb, NULL,
                                         &jabber_buffer_close_cb, NULL);
        if (!new_buffer)
        {
            free (new_muc);
            return NULL;
        }
        buffer_created = 1;
    }
    
    weechat_buffer_set (new_buffer, "short_name", muc_name);
    weechat_buffer_set (new_buffer, "localvar_set_type",
                        (muc_type == JABBER_MUC_TYPE_MUC) ? "channel" : "private");
    weechat_buffer_set (new_buffer, "localvar_set_nick",
                        jabber_server_get_local_name (server));
    weechat_buffer_set (new_buffer, "localvar_set_server", server->name);
    weechat_buffer_set (new_buffer, "localvar_set_muc", muc_name);

    if (buffer_created)
    {
        weechat_hook_signal_send ("logger_backlog",
                                  WEECHAT_HOOK_SIGNAL_POINTER, new_buffer);
    }
    
    if (muc_type == JABBER_MUC_TYPE_MUC)
    {
        weechat_buffer_set (new_buffer, "nicklist", "1");
        weechat_buffer_set (new_buffer, "nicklist_display_groups", "0");
        weechat_nicklist_add_group (new_buffer, NULL, JABBER_BUDDY_GROUP_OP,
                                    "weechat.color.nicklist_group", 1);
        weechat_nicklist_add_group (new_buffer, NULL, JABBER_BUDDY_GROUP_HALFOP,
                                    "weechat.color.nicklist_group", 1);
        weechat_nicklist_add_group (new_buffer, NULL, JABBER_BUDDY_GROUP_VOICE,
                                    "weechat.color.nicklist_group", 1);
        weechat_nicklist_add_group (new_buffer, NULL, JABBER_BUDDY_GROUP_CHANUSER,
                                    "weechat.color.nicklist_group", 1);
        weechat_nicklist_add_group (new_buffer, NULL, JABBER_BUDDY_GROUP_NORMAL,
                                    "weechat.color.nicklist_group", 1);
    }
    
    /* set highlights settings on MUC buffer */
    weechat_buffer_set (new_buffer, "highlight_words",
                        jabber_server_get_local_name (server));
    if (weechat_config_string (jabber_config_look_highlight_tags)
        && weechat_config_string (jabber_config_look_highlight_tags)[0])
    {
        weechat_buffer_set (new_buffer, "highlight_tags",
                            weechat_config_string (jabber_config_look_highlight_tags));
    }
    
    /* initialize new MUC */
    new_muc->type = muc_type;
    new_muc->name = strdup (muc_name);
    new_muc->topic = NULL;
    new_muc->modes = NULL;
    new_muc->limit = 0;
    new_muc->key = NULL;
    new_muc->away_message = NULL;
    new_muc->nick_completion_reset = 0;
    new_muc->buddies_count = 0;
    new_muc->buddies = NULL;
    new_muc->last_buddy = NULL;
    new_muc->buddies_speaking[0] = NULL;
    new_muc->buddies_speaking[1] = NULL;
    new_muc->buddies_speaking_time = NULL;
    new_muc->last_buddy_speaking_time = NULL;
    new_muc->buffer = new_buffer;
    new_muc->buffer_as_string = NULL;
    
    /* add new MUC to MUCs list */
    new_muc->prev_muc = server->last_muc;
    new_muc->next_muc = NULL;
    if (server->mucs)
        (server->last_muc)->next_muc = new_muc;
    else
        server->mucs = new_muc;
    server->last_muc = new_muc;
    
    if (switch_to_muc)
    {
        weechat_buffer_set (new_buffer, "display",
                            (auto_switch) ? "auto" : "1");
    }
    
    /* all is ok, return address of new muc */
    return new_muc;
}

/*
 * jabber_muc_set_topic: set topic for a muc
 */

void
jabber_muc_set_topic (struct t_jabber_muc *muc, const char *topic)
{
    if (muc->topic)
        free (muc->topic);
    
    muc->topic = (topic) ? strdup (topic) : NULL;
    weechat_buffer_set (muc->buffer, "title", (muc->topic) ? muc->topic : "");
}

/*
 * jabber_muc_search: returns pointer on a muc with name
 */

struct t_jabber_muc *
jabber_muc_search (struct t_jabber_server *server, const char *muc_name)
{
    struct t_jabber_muc *ptr_muc;
    
    if (!server || !muc_name)
        return NULL;
    
    for (ptr_muc = server->mucs; ptr_muc;
         ptr_muc = ptr_muc->next_muc)
    {
        if (weechat_strcasecmp (ptr_muc->name, muc_name) == 0)
            return ptr_muc;
    }
    return NULL;
}

/*
 * jabber_muc_remove_away: remove away for all buddies in a MUC
 */

void
jabber_muc_remove_away (struct t_jabber_muc *muc)
{
    struct t_jabber_buddy *ptr_buddy;
    
    if (muc->type == JABBER_MUC_TYPE_MUC)
    {
        for (ptr_buddy = muc->buddies; ptr_buddy;
             ptr_buddy = ptr_buddy->next_buddy)
        {
            jabber_buddy_set (NULL, muc, ptr_buddy, 0, JABBER_BUDDY_AWAY);
        }
    }
}

/*
 * jabber_muc_set_away: set/unset away status for a MUC
 */

void
jabber_muc_set_away (struct t_jabber_muc *muc, const char *buddy_name,
                     int is_away)
{
    struct t_jabber_buddy *ptr_buddy;
    
    if (muc->type == JABBER_MUC_TYPE_MUC)
    {
        ptr_buddy = jabber_buddy_search (NULL, muc, buddy_name);
        if (ptr_buddy)
            jabber_buddy_set_away (NULL, muc, ptr_buddy, is_away);
    }
}

/*
 * jabber_muc_buddy_speaking_add: add a buddy speaking in a MUC
 */

void
jabber_muc_buddy_speaking_add (struct t_jabber_muc *muc, const char *buddy_name,
                               int highlight)
{
    int size, to_remove, i;
    struct t_weelist_item *ptr_item;
    
    if (highlight < 0)
        highlight = 0;
    if (highlight > 1)
        highlight = 1;
    
    /* create list if it does not exist */
    if (!muc->buddies_speaking[highlight])
        muc->buddies_speaking[highlight] = weechat_list_new ();
    
    /* remove item if it was already in list */
    ptr_item = weechat_list_casesearch (muc->buddies_speaking[highlight],
                                        buddy_name);
    if (ptr_item)
        weechat_list_remove (muc->buddies_speaking[highlight], ptr_item);
    
    /* add buddy in list */
    weechat_list_add (muc->buddies_speaking[highlight], buddy_name,
                      WEECHAT_LIST_POS_END, NULL);
    
    /* reduce list size if it's too big */
    size = weechat_list_size (muc->buddies_speaking[highlight]);
    if (size > JABBER_MUC_BUDDIES_SPEAKING_LIMIT)
    {
        to_remove = size - JABBER_MUC_BUDDIES_SPEAKING_LIMIT;
        for (i = 0; i < to_remove; i++)
        {
            weechat_list_remove (muc->buddies_speaking[highlight],
                                 weechat_list_get (muc->buddies_speaking[highlight], 0));
        }
    }
}

/*
 * jabber_muc_buddy_speaking_rename: rename a buddy speaking in a MUC
 */

void
jabber_muc_buddy_speaking_rename (struct t_jabber_muc *muc,
                                  const char *old_nick,
                                  const char *new_nick)
{
    struct t_weelist_item *ptr_item;
    int i;

    for (i = 0; i < 2; i++)
    {
        if (muc->buddies_speaking[i])
        {
            ptr_item = weechat_list_search (muc->buddies_speaking[i], old_nick);
            if (ptr_item)
                weechat_list_set (ptr_item, new_nick);
        }
    }
}

/*
 * jabber_muc_buddy_speaking_time_search: search a buddy speaking time in a MUC
 */

struct t_jabber_muc_speaking *
jabber_muc_buddy_speaking_time_search (struct t_jabber_muc *muc,
                                       const char *buddy_name,
                                       int check_time)
{
    struct t_jabber_muc_speaking *ptr_buddy;
    time_t time_limit;
    
    time_limit = time (NULL) -
        (weechat_config_integer (jabber_config_look_smart_filter_delay) * 60);
    
    for (ptr_buddy = muc->buddies_speaking_time; ptr_buddy;
         ptr_buddy = ptr_buddy->next_buddy)
    {
        if (strcmp (ptr_buddy->buddy, buddy_name) == 0)
        {
            if (check_time && (ptr_buddy->time_last_message < time_limit))
                return NULL;
            return ptr_buddy;
        }
    }
    
    /* buddy speaking time not found */
    return NULL;
}

/*
 * jabber_muc_buddy_speaking_time_free: free a buddy speaking in a MUC
 */

void
jabber_muc_buddy_speaking_time_free (struct t_jabber_muc *muc,
                                     struct t_jabber_muc_speaking *buddy_speaking)
{
    /* free data */
    if (buddy_speaking->buddy)
        free (buddy_speaking->buddy);
    
    /* remove buddy from list */
    if (buddy_speaking->prev_buddy)
        (buddy_speaking->prev_buddy)->next_buddy = buddy_speaking->next_buddy;
    if (buddy_speaking->next_buddy)
        (buddy_speaking->next_buddy)->prev_buddy = buddy_speaking->prev_buddy;
    if (muc->buddies_speaking_time == buddy_speaking)
        muc->buddies_speaking_time = buddy_speaking->next_buddy;
    if (muc->last_buddy_speaking_time == buddy_speaking)
        muc->last_buddy_speaking_time = buddy_speaking->prev_buddy;
    
    free (buddy_speaking);
}

/*
 * jabber_muc_buddy_speaking_time_free_all: free all buddies speaking in a MUC
 */

void
jabber_muc_buddy_speaking_time_free_all (struct t_jabber_muc *muc)
{
    while (muc->buddies_speaking_time)
    {
        jabber_muc_buddy_speaking_time_free (muc,
                                             muc->buddies_speaking_time);
    }
}

/*
 * jabber_muc_buddy_speaking_time_remove_old: remove old buddies speaking
 */

void
jabber_muc_buddy_speaking_time_remove_old (struct t_jabber_muc *muc)
{
    time_t time_limit;
    
    time_limit = time (NULL) -
        (weechat_config_integer (jabber_config_look_smart_filter_delay) * 60);

    while (muc->last_buddy_speaking_time)
    {
        if (muc->last_buddy_speaking_time->time_last_message >= time_limit)
            break;
        
        jabber_muc_buddy_speaking_time_free (muc,
                                             muc->last_buddy_speaking_time);
    }
}

/*
 * jabber_muc_buddy_speaking_time_add: add a buddy speaking time in a MUC
 */

void
jabber_muc_buddy_speaking_time_add (struct t_jabber_muc *muc,
                                    const char *buddy_name,
                                    time_t time_last_message)
{
    struct t_jabber_muc_speaking *ptr_buddy, *new_buddy;
    
    ptr_buddy = jabber_muc_buddy_speaking_time_search (muc, buddy_name, 0);
    if (ptr_buddy)
        jabber_muc_buddy_speaking_time_free (muc, ptr_buddy);
    
    new_buddy = malloc (sizeof (*new_buddy));
    if (new_buddy)
    {
        new_buddy->buddy = strdup (buddy_name);
        new_buddy->time_last_message = time_last_message;
        
        /* insert buddy at beginning of list */
        new_buddy->prev_buddy = NULL;
        new_buddy->next_buddy = muc->buddies_speaking_time;
        if (muc->buddies_speaking_time)
            muc->buddies_speaking_time->prev_buddy = new_buddy;
        else
            muc->last_buddy_speaking_time = new_buddy;
        muc->buddies_speaking_time = new_buddy;
    }
}

/*
 * jabber_muc_buddy_speaking_time_rename: rename a buddy speaking time in a MUC
 */

void
jabber_muc_buddy_speaking_time_rename (struct t_jabber_muc *muc,
                                       const char *old_buddy,
                                       const char *new_buddy)
{
    struct t_jabber_muc_speaking *ptr_buddy;
    
    if (muc->buddies_speaking_time)
    {
        ptr_buddy = jabber_muc_buddy_speaking_time_search (muc, old_buddy, 0);
        if (ptr_buddy)
        {
            free (ptr_buddy->buddy);
            ptr_buddy->buddy = strdup (new_buddy);
        }
    }
}

/*
 * jabber_muc_free: free a muc and remove it from MUCs list
 */

void
jabber_muc_free (struct t_jabber_server *server, struct t_jabber_muc *muc)
{
    struct t_jabber_muc *new_mucs;
    
    if (!server || !muc)
        return;
    
    /* remove muc from MUCs list */
    if (server->last_muc == muc)
        server->last_muc = muc->prev_muc;
    if (muc->prev_muc)
    {
        (muc->prev_muc)->next_muc = muc->next_muc;
        new_mucs = server->mucs;
    }
    else
        new_mucs = muc->next_muc;
    
    if (muc->next_muc)
        (muc->next_muc)->prev_muc = muc->prev_muc;
    
    /* free data */
    if (muc->name)
        free (muc->name);
    if (muc->topic)
        free (muc->topic);
    if (muc->modes)
        free (muc->modes);
    if (muc->key)
        free (muc->key);
    jabber_buddy_free_all (NULL, muc);
    if (muc->away_message)
        free (muc->away_message);
    if (muc->buddies_speaking[0])
        weechat_list_free (muc->buddies_speaking[0]);
    if (muc->buddies_speaking[1])
        weechat_list_free (muc->buddies_speaking[1]);
    jabber_muc_buddy_speaking_time_free_all (muc);
    if (muc->buffer_as_string)
        free (muc->buffer_as_string);
    
    free (muc);
    
    server->mucs = new_mucs;
}

/*
 * jabber_muc_free_all: free all allocated MUCs for a server
 */

void
jabber_muc_free_all (struct t_jabber_server *server)
{
    while (server->mucs)
    {
        jabber_muc_free (server, server->mucs);
    }
}

/*
 * jabber_muc_add_to_infolist: add a muc in an infolist
 *                              return 1 if ok, 0 if error
 */

int
jabber_muc_add_to_infolist (struct t_infolist *infolist,
                             struct t_jabber_muc *muc)
{
    struct t_infolist_item *ptr_item;
    struct t_weelist_item *ptr_list_item;
    struct t_jabber_muc_speaking *ptr_buddy;
    char option_name[64];
    int i, index;
    
    if (!infolist || !muc)
        return 0;
    
    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;
    
    if (!weechat_infolist_new_var_pointer (ptr_item, "buffer", muc->buffer))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "buffer_name",
                                          (muc->buffer) ?
                                          weechat_buffer_get_string (muc->buffer, "name") : ""))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "buffer_short_name",
                                          (muc->buffer) ?
                                          weechat_buffer_get_string (muc->buffer, "short_name") : ""))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "type", muc->type))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "name", muc->name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "topic", muc->topic))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "modes", muc->modes))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "limit", muc->limit))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "key", muc->key))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "buddies_count", muc->buddies_count))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "away_message", muc->away_message))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "nick_completion_reset", muc->nick_completion_reset))
        return 0;
    for (i = 0; i < 2; i++)
    {
        if (muc->buddies_speaking[i])
        {
            index = 0;
            for (ptr_list_item = weechat_list_get (muc->buddies_speaking[i], 0);
                 ptr_list_item;
                 ptr_list_item = weechat_list_next (ptr_list_item))
            {
                snprintf (option_name, sizeof (option_name),
                          "buddy_speaking%d_%05d", i, index);
                if (!weechat_infolist_new_var_string (ptr_item, option_name,
                                                      weechat_list_string (ptr_list_item)))
                    return 0;
                index++;
            }
        }
    }
    if (muc->buddies_speaking_time)
    {
        i = 0;
        for (ptr_buddy = muc->last_buddy_speaking_time; ptr_buddy;
             ptr_buddy = ptr_buddy->prev_buddy)
        {
            snprintf (option_name, sizeof (option_name),
                      "buddy_speaking_time_buddy_%05d", i);
            if (!weechat_infolist_new_var_string (ptr_item, option_name,
                                                  ptr_buddy->buddy))
                return 0;
            snprintf (option_name, sizeof (option_name),
                      "buddy_speaking_time_time_%05d", i);
            if (!weechat_infolist_new_var_time (ptr_item, option_name,
                                                ptr_buddy->time_last_message))
                return 0;
            i++;
        }
    }
    
    return 1;
}

/*
 * jabber_muc_print_log: print muc infos in log (usually for crash dump)
 */

void
jabber_muc_print_log (struct t_jabber_muc *muc)
{
    struct t_weelist_item *ptr_item;
    struct t_jabber_muc_speaking *ptr_buddy_speaking;
    int i, index;
    struct t_jabber_buddy *ptr_buddy;
    
    weechat_log_printf ("");
    weechat_log_printf ("  => muc %s (addr:0x%lx)]", muc->name, muc);
    weechat_log_printf ("       type . . . . . . . . . . : %d",    muc->type);
    weechat_log_printf ("       topic. . . . . . . . . . : '%s'",  muc->topic);
    weechat_log_printf ("       modes. . . . . . . . . . : '%s'",  muc->modes);
    weechat_log_printf ("       limit. . . . . . . . . . : %d",    muc->limit);
    weechat_log_printf ("       key. . . . . . . . . . . : '%s'",  muc->key);
    weechat_log_printf ("       away_message . . . . . . : '%s'",  muc->away_message);
    weechat_log_printf ("       nick_completion_reset. . : %d",    muc->nick_completion_reset);
    weechat_log_printf ("       buddies_count. . . . . . : %d",    muc->buddies_count);
    weechat_log_printf ("       buddies. . . . . . . . . : 0x%lx", muc->buddies);
    weechat_log_printf ("       last_buddy . . . . . . . : 0x%lx", muc->last_buddy);
    weechat_log_printf ("       buddies_speaking[0]. . . : 0x%lx", muc->buddies_speaking[0]);
    weechat_log_printf ("       buddies_speaking[1]. . . : 0x%lx", muc->buddies_speaking[1]);
    weechat_log_printf ("       buddies_speaking_time. . : 0x%lx", muc->buddies_speaking_time);
    weechat_log_printf ("       last_buddy_speaking_time.: 0x%lx", muc->last_buddy_speaking_time);
    weechat_log_printf ("       buffer . . . . . . . . . : 0x%lx", muc->buffer);
    weechat_log_printf ("       buffer_as_string . . . . : '%s'",  muc->buffer_as_string);
    weechat_log_printf ("       prev_muc . . . . . . . . : 0x%lx", muc->prev_muc);
    weechat_log_printf ("       next_muc . . . . . . . . : 0x%lx", muc->next_muc);
    for (i = 0; i < 2; i++)
    {
        if (muc->buddies_speaking[i])
        {
            weechat_log_printf ("");
            index = 0;
            for (ptr_item = weechat_list_get (muc->buddies_speaking[i], 0);
                 ptr_item; ptr_item = weechat_list_next (ptr_item))
            {
                weechat_log_printf ("         buddy speaking[%d][%d]: '%s'",
                                    i, index, weechat_list_string (ptr_item));
                index++;
            }
        }
    }
    if (muc->buddies_speaking_time)
    {
        weechat_log_printf ("");
        for (ptr_buddy_speaking = muc->buddies_speaking_time;
             ptr_buddy_speaking;
             ptr_buddy_speaking = ptr_buddy_speaking->next_buddy)
        {
            weechat_log_printf ("         buddy speaking time: '%s', time: %ld",
                                ptr_buddy_speaking->buddy,
                                ptr_buddy_speaking->time_last_message);
        }
    }
    for (ptr_buddy = muc->buddies; ptr_buddy;
         ptr_buddy = ptr_buddy->next_buddy)
    {
        jabber_buddy_print_log (ptr_buddy);
    }
}
