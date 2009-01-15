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

/* jabber-buddy.c: manages buddies list for servers and MUCs */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "../weechat-plugin.h"
#include "jabber.h"
#include "jabber-buddy.h"
#include "jabber-config.h"
#include "jabber-server.h"
#include "jabber-muc.h"


/*
 * jabber_buddy_valid: check if a buddy pointer exists for a server or a muc
 *                     return 1 if buddy exists
 *                            0 if buddy is not found
 */

int
jabber_buddy_valid (struct t_jabber_server *server, struct t_jabber_muc *muc,
                    struct t_jabber_buddy *buddy)
{
    struct t_jabber_buddy *ptr_buddy;
    
    if (server)
    {
        for (ptr_buddy = server->buddies; ptr_buddy;
             ptr_buddy = ptr_buddy->next_buddy)
        {
            if (ptr_buddy == buddy)
                return 1;
        }
    }
    
    if (muc)
    {
        for (ptr_buddy = muc->buddies; ptr_buddy;
             ptr_buddy = ptr_buddy->next_buddy)
        {
            if (ptr_buddy == buddy)
                return 1;
        }
    }
    
    /* buddy not found */
    return 0;
}

/*
 * jabber_buddy_find_color: find a color for a buddy (according to buddy letters)
 */

const char *
jabber_buddy_find_color (struct t_jabber_buddy *buddy)
{
    int i, color;
    char color_name[64];
    
    color = 0;
    for (i = strlen (buddy->name) - 1; i >= 0; i--)
    {
        color += (int)(buddy->name[i]);
    }
    color = (color %
             weechat_config_integer (weechat_config_get ("weechat.look.color_nicks_number")));

    snprintf (color_name, sizeof (color_name),
              "chat_buddy_color%02d", color + 1);
    
    return weechat_color (color_name);
}

/*
 * jabber_buddy_get_gui_infos: get GUI infos for a buddy (sort_index, prefix,
 *                             prefix color)
 */

void
jabber_buddy_get_gui_infos (struct t_jabber_buddy *buddy,
                            char *prefix, int *prefix_color,
                            struct t_gui_buffer *buffer,
                            struct t_gui_nick_group **group)
{
    if (buddy->flags & JABBER_BUDDY_CHANOWNER)
    {
        if (prefix)
            *prefix = '~';
        if (prefix_color)
            *prefix_color = 1;
        if (buffer && group)
            *group = weechat_nicklist_search_group (buffer, NULL,
                                                    JABBER_BUDDY_GROUP_OP);
    }
    else if (buddy->flags & JABBER_BUDDY_CHANADMIN)
    {
        if (prefix)
            *prefix = '&';
        if (prefix_color)
            *prefix_color = 1;
        if (buffer && group)
            *group = weechat_nicklist_search_group (buffer, NULL,
                                                    JABBER_BUDDY_GROUP_OP);
    }
    else if (buddy->flags & JABBER_BUDDY_CHANADMIN2)
    {
        if (prefix)
            *prefix = '!';
        if (prefix_color)
            *prefix_color = 1;
        if (buffer && group)
            *group = weechat_nicklist_search_group (buffer, NULL,
                                                    JABBER_BUDDY_GROUP_OP);
    }
    else if (buddy->flags & JABBER_BUDDY_OP)
    {
        if (prefix)
            *prefix = '@';
        if (prefix_color)
            *prefix_color = 1;
        if (buffer && group)
            *group = weechat_nicklist_search_group (buffer, NULL,
                                                    JABBER_BUDDY_GROUP_OP);
    }
    else if (buddy->flags & JABBER_BUDDY_HALFOP)
    {
        if (prefix)
            *prefix = '%';
        if (prefix_color)
            *prefix_color = 2;
        if (buffer && group)
            *group = weechat_nicklist_search_group (buffer, NULL,
                                                    JABBER_BUDDY_GROUP_HALFOP);
    }
    else if (buddy->flags & JABBER_BUDDY_VOICE)
    {
        if (prefix)
            *prefix = '+';
        if (prefix_color)
            *prefix_color = 3;
        if (buffer && group)
            *group = weechat_nicklist_search_group (buffer, NULL,
                                                    JABBER_BUDDY_GROUP_VOICE);
    }
    else if (buddy->flags & JABBER_BUDDY_CHANUSER)
    {
        if (prefix)
            *prefix = '-';
        if (prefix_color)
            *prefix_color = 4;
        if (buffer && group)
            *group = weechat_nicklist_search_group (buffer, NULL,
                                                    JABBER_BUDDY_GROUP_CHANUSER);
    }
    else
    {
        if (prefix)
            *prefix = ' ';
        if (prefix_color)
            *prefix_color = 0;
        if (buffer && group)
            *group = weechat_nicklist_search_group (buffer, NULL,
                                                    JABBER_BUDDY_GROUP_NORMAL);
    }
}

/*
 * jabber_buddy_new: allocate a new buddy for a muc and add it to the buddy list
 */

struct t_jabber_buddy *
jabber_buddy_new (struct t_jabber_server *server, struct t_jabber_muc *muc,
                  const char *buddy_name, int is_chanowner, int is_chanadmin,
                  int is_chanadmin2, int is_op, int is_halfop, int has_voice,
                  int is_chanuser, int is_away)
{
    struct t_jabber_buddy *new_buddy, *ptr_buddy;
    char prefix, str_prefix_color[64];
    const char *local_name;
    int prefix_color;
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_nick_group *ptr_group;
    
    ptr_buffer = (muc) ? muc->buffer : server->buffer;
    
    /* buddy already exists on this muc? */
    if (muc)
        ptr_buddy = jabber_buddy_search (NULL, muc, buddy_name);
    else
        ptr_buddy = jabber_buddy_search (server, NULL, buddy_name);
    if (ptr_buddy)
    {
        /* remove old buddy from buddylist */
        jabber_buddy_get_gui_infos (ptr_buddy, &prefix,
                                    &prefix_color, ptr_buffer, &ptr_group);
        weechat_nicklist_remove_nick (ptr_buffer,
                                      weechat_nicklist_search_nick (ptr_buffer,
                                                                    ptr_group,
                                                                    ptr_buddy->name));
        
        /* update buddy */
        JABBER_BUDDY_SET_FLAG(ptr_buddy, is_chanowner, JABBER_BUDDY_CHANOWNER);
        JABBER_BUDDY_SET_FLAG(ptr_buddy, is_chanadmin, JABBER_BUDDY_CHANADMIN);
        JABBER_BUDDY_SET_FLAG(ptr_buddy, is_chanadmin2, JABBER_BUDDY_CHANADMIN2);
        JABBER_BUDDY_SET_FLAG(ptr_buddy, is_op, JABBER_BUDDY_OP);
        JABBER_BUDDY_SET_FLAG(ptr_buddy, is_halfop, JABBER_BUDDY_HALFOP);
        JABBER_BUDDY_SET_FLAG(ptr_buddy, has_voice, JABBER_BUDDY_VOICE);
        JABBER_BUDDY_SET_FLAG(ptr_buddy, is_chanuser, JABBER_BUDDY_CHANUSER);
        JABBER_BUDDY_SET_FLAG(ptr_buddy, is_away, JABBER_BUDDY_AWAY);
        
        /* add new buddy in buddylist */
        jabber_buddy_get_gui_infos (ptr_buddy, &prefix,
                                &prefix_color, ptr_buffer, &ptr_group);
        snprintf (str_prefix_color, sizeof (str_prefix_color),
                  "weechat.color.nicklist_prefix%d",
                  prefix_color);
        weechat_nicklist_add_nick (ptr_buffer, ptr_group,
                                   ptr_buddy->name,
                                   (is_away) ?
                                   "weechat.color.nicklist_away" : "bar_fg",
                                   prefix, str_prefix_color, 1);
        
        return ptr_buddy;
    }
    
    /* alloc memory for new buddy */
    if ((new_buddy = malloc (sizeof (*new_buddy))) == NULL)
        return NULL;
    
    /* initialize new buddy */
    new_buddy->name = strdup (buddy_name);
    new_buddy->host = NULL;
    new_buddy->flags = 0;
    JABBER_BUDDY_SET_FLAG(new_buddy, is_chanowner, JABBER_BUDDY_CHANOWNER);
    JABBER_BUDDY_SET_FLAG(new_buddy, is_chanadmin, JABBER_BUDDY_CHANADMIN);
    JABBER_BUDDY_SET_FLAG(new_buddy, is_chanadmin2, JABBER_BUDDY_CHANADMIN2);
    JABBER_BUDDY_SET_FLAG(new_buddy, is_op, JABBER_BUDDY_OP);
    JABBER_BUDDY_SET_FLAG(new_buddy, is_halfop, JABBER_BUDDY_HALFOP);
    JABBER_BUDDY_SET_FLAG(new_buddy, has_voice, JABBER_BUDDY_VOICE);
    JABBER_BUDDY_SET_FLAG(new_buddy, is_chanuser, JABBER_BUDDY_CHANUSER);
    JABBER_BUDDY_SET_FLAG(new_buddy, is_away, JABBER_BUDDY_AWAY);
    local_name = jabber_server_get_local_name (server);
    if (weechat_strcasecmp (new_buddy->name, local_name) == 0)
        new_buddy->color = JABBER_COLOR_CHAT_NICK_SELF;
    else
        new_buddy->color = jabber_buddy_find_color (new_buddy);
    
    /* add buddy to end of list */
    if (muc)
    {
        new_buddy->prev_buddy = muc->last_buddy;
        if (muc->buddies)
            muc->last_buddy->next_buddy = new_buddy;
        else
            muc->buddies = new_buddy;
        muc->last_buddy = new_buddy;
        new_buddy->next_buddy = NULL;
        
        muc->buddies_count++;
        
        muc->nick_completion_reset = 1;
    }
    else
    {
        new_buddy->prev_buddy = server->last_buddy;
        if (server->buddies)
            server->last_buddy->next_buddy = new_buddy;
        else
            server->buddies = new_buddy;
        server->last_buddy = new_buddy;
        new_buddy->next_buddy = NULL;
        
        server->buddies_count++;
    }
    
    /* add buddy to buffer buddylist */
    jabber_buddy_get_gui_infos (new_buddy, &prefix, &prefix_color,
                                ptr_buffer, &ptr_group);
    snprintf (str_prefix_color, sizeof (str_prefix_color),
              "weechat.color.nicklist_prefix%d",
              prefix_color);
    weechat_nicklist_add_nick (ptr_buffer, ptr_group,
                               new_buddy->name,
                               (is_away) ?
                               "weechat.color.nicklist_away" : "bar_fg",
                               prefix, str_prefix_color, 1);
    
    /* all is ok, return address of new buddy */
    return new_buddy;
}

/*
 * jabber_buddy_change: change buddyname
 */

void
jabber_buddy_change (struct t_jabber_server *server, struct t_jabber_muc *muc,
                     struct t_jabber_buddy *buddy, const char *new_buddy)
{
    int buddy_is_me, prefix_color;
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_nick_group *ptr_group;
    char prefix, str_prefix_color[64];
    const char *local_name;
    
    ptr_buffer = (muc) ? muc->buffer : server->buffer;
    
    /* remove buddy from buddylist */
    jabber_buddy_get_gui_infos (buddy, &prefix, &prefix_color,
                                ptr_buffer, &ptr_group);
    weechat_nicklist_remove_nick (ptr_buffer,
                                  weechat_nicklist_search_nick (ptr_buffer,
                                                                ptr_group,
                                                                buddy->name));
    
    /* update buddies speaking */
    local_name = jabber_server_get_local_name (server);
    buddy_is_me = (strcmp (buddy->name, local_name) == 0) ? 1 : 0;
    if (muc && !buddy_is_me)
        jabber_muc_buddy_speaking_rename (muc, buddy->name, new_buddy);
    
    /* change buddyname */
    if (buddy->name)
        free (buddy->name);
    buddy->name = strdup (new_buddy);
    if (buddy_is_me)
        buddy->color = JABBER_COLOR_CHAT_NICK_SELF;
    else
        buddy->color = jabber_buddy_find_color (buddy);
    
    /* add buddy in buddylist */
    jabber_buddy_get_gui_infos (buddy, &prefix, &prefix_color,
                                ptr_buffer, &ptr_group);
    snprintf (str_prefix_color, sizeof (str_prefix_color),
              "weechat.color.nicklist_prefix%d",
              prefix_color);
    weechat_nicklist_add_nick (ptr_buffer, ptr_group,
                               buddy->name, "bar_fg",
                               prefix, str_prefix_color, 1);
}

/*
 * jabber_buddy_set: set a flag for a buddy
 */

void
jabber_buddy_set (struct t_jabber_server *server, struct t_jabber_muc *muc,
                  struct t_jabber_buddy *buddy, int set, int flag)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_nick_group *ptr_group;
    char prefix, str_prefix_color[64];
    int prefix_color;

    if (server || muc)
    {
        ptr_buffer = (muc) ? muc->buffer : server->buffer;
        
        /* remove buddy from buddylist */
        jabber_buddy_get_gui_infos (buddy, &prefix, &prefix_color,
                                    ptr_buffer, &ptr_group);
        weechat_nicklist_remove_nick (ptr_buffer,
                                      weechat_nicklist_search_nick (ptr_buffer,
                                                                    ptr_group,
                                                                    buddy->name));
        
        /* set flag */
        JABBER_BUDDY_SET_FLAG(buddy, set, flag);
        
        /* add buddy in buddylist */
        jabber_buddy_get_gui_infos (buddy, &prefix, &prefix_color,
                                    ptr_buffer, &ptr_group);
        snprintf (str_prefix_color, sizeof (str_prefix_color),
                  "weechat.color.nicklist_prefix%d",
                  prefix_color);
        weechat_nicklist_add_nick (ptr_buffer, ptr_group,
                                   buddy->name,
                                   (buddy->flags & JABBER_BUDDY_AWAY) ?
                                   "weechat.color.nicklist_away" : "bar_fg",
                                   prefix, str_prefix_color, 1);
    }
}

/*
 * jabber_buddy_free: free a buddy and remove it from buddies list
 */

void
jabber_buddy_free (struct t_jabber_server *server, struct t_jabber_muc *muc,
                   struct t_jabber_buddy *buddy)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_nick_group *ptr_group;
    struct t_jabber_buddy *new_buddies;
    char prefix;
    int prefix_color;
    
    if ((!server && !muc) || !buddy)
        return;
    
    ptr_buffer = (muc) ? muc->buffer : server->buffer;
    
    /* remove buddy from buddylist */
    jabber_buddy_get_gui_infos (buddy, &prefix, &prefix_color,
                                ptr_buffer, &ptr_group);
    weechat_nicklist_remove_nick (ptr_buffer,
                                  weechat_nicklist_search_nick (ptr_buffer,
                                                                ptr_group,
                                                                buddy->name));
    
    /* remove buddy */
    if (muc)
    {
        if (muc->last_buddy == buddy)
            muc->last_buddy = buddy->prev_buddy;
        if (buddy->prev_buddy)
        {
            (buddy->prev_buddy)->next_buddy = buddy->next_buddy;
            new_buddies = muc->buddies;
        }
        else
            new_buddies = buddy->next_buddy;
        if (buddy->next_buddy)
            (buddy->next_buddy)->prev_buddy = buddy->prev_buddy;
        muc->buddies_count--;
    }
    else
    {
        if (server->last_buddy == buddy)
            server->last_buddy = buddy->prev_buddy;
        if (buddy->prev_buddy)
        {
            (buddy->prev_buddy)->next_buddy = buddy->next_buddy;
            new_buddies = server->buddies;
        }
        else
            new_buddies = buddy->next_buddy;
        if (buddy->next_buddy)
            (buddy->next_buddy)->prev_buddy = buddy->prev_buddy;
        server->buddies_count--;
    }
    
    /* free data */
    if (buddy->name)
        free (buddy->name);
    if (buddy->host)
        free (buddy->host);
    
    free (buddy);
    
    if (muc)
    {
        muc->buddies = new_buddies;
        muc->nick_completion_reset = 1;
    }
    else
    {
        server->buddies = new_buddies;
    }
}

/*
 * jabber_buddy_free_all: free all allocated buddies for a muc
 */

void
jabber_buddy_free_all (struct t_jabber_server *server,
                       struct t_jabber_muc *muc)
{
    if (server)
    {
        while (server->buddies)
        {
            jabber_buddy_free (server, NULL, server->buddies);
        }
        /* sould be zero, but prevent any bug :D */
        server->buddies_count = 0;
    }
    
    if (muc)
    {
        while (muc->buddies)
        {
            jabber_buddy_free (NULL, muc, muc->buddies);
        }
        /* sould be zero, but prevent any bug :D */
        muc->buddies_count = 0;
    }
}

/*
 * jabber_buddy_search: returns pointer on a buddy
 */

struct t_jabber_buddy *
jabber_buddy_search (struct t_jabber_server *server, struct t_jabber_muc *muc,
                     const char *buddyname)
{
    struct t_jabber_buddy *ptr_buddy;
    
    if (!buddyname)
        return NULL;
    
    if (server)
    {
        for (ptr_buddy = server->buddies; ptr_buddy;
             ptr_buddy = ptr_buddy->next_buddy)
        {
            if (weechat_strcasecmp (ptr_buddy->name, buddyname) == 0)
                return ptr_buddy;
        }
    }
    
    if (muc)
    {
        for (ptr_buddy = muc->buddies; ptr_buddy;
             ptr_buddy = ptr_buddy->next_buddy)
        {
            if (weechat_strcasecmp (ptr_buddy->name, buddyname) == 0)
                return ptr_buddy;
        }
    }
    
    /* buddy not found */
    return NULL;
}

/*
 * jabber_buddy_count: returns number of buddies (total, op, halfop, voice) on
 *                     a server or MUC
 */

void
jabber_buddy_count (struct t_jabber_server *server, struct t_jabber_muc *muc,
                    int *total, int *count_op, int *count_halfop,
                    int *count_voice, int *count_normal)
{
    struct t_jabber_buddy *ptr_buddy;
    
    (*total) = 0;
    (*count_op) = 0;
    (*count_halfop) = 0;
    (*count_voice) = 0;
    (*count_normal) = 0;
    
    if (server || muc)
    {
        for (ptr_buddy = (muc) ? muc->buddies : server->buddies; ptr_buddy;
             ptr_buddy = ptr_buddy->next_buddy)
        {
            (*total)++;
            if ((ptr_buddy->flags & JABBER_BUDDY_CHANOWNER) ||
                (ptr_buddy->flags & JABBER_BUDDY_CHANADMIN) ||
                (ptr_buddy->flags & JABBER_BUDDY_CHANADMIN2) ||
                (ptr_buddy->flags & JABBER_BUDDY_OP))
                (*count_op)++;
            else
            {
                if (ptr_buddy->flags & JABBER_BUDDY_HALFOP)
                    (*count_halfop)++;
                else
                {
                    if (ptr_buddy->flags & JABBER_BUDDY_VOICE)
                        (*count_voice)++;
                    else
                        (*count_normal)++;
                }
            }
        }
    }
}

/*
 * jabber_buddy_set_away: set/unset away status for a muc
 */

void
jabber_buddy_set_away (struct t_jabber_server *server,
                       struct t_jabber_muc *muc,
                       struct t_jabber_buddy *buddy, int is_away)
{
    if (((is_away) && (!(buddy->flags & JABBER_BUDDY_AWAY))) ||
        ((!is_away) && (buddy->flags & JABBER_BUDDY_AWAY)))
    {
        if (muc)
            jabber_buddy_set (NULL, muc, buddy, is_away, JABBER_BUDDY_AWAY);
        else
            jabber_buddy_set (server, NULL, buddy, is_away, JABBER_BUDDY_AWAY);
    }
}

/*
 * jabber_buddy_as_prefix: return string with buddy to display as prefix on
 *                         buffer (string will end by a tab)
 */

char *
jabber_buddy_as_prefix (struct t_jabber_buddy *buddy, const char *buddyname,
                        const char *force_color)
{
    static char result[256];
    char prefix[2], str_prefix_color[64];
    int prefix_color;
    
    prefix[0] = '\0';
    prefix[1] = '\0';
    if (weechat_config_boolean (weechat_config_get ("weechat.look.nickmode")))
    {
        if (buddy)
        {
            jabber_buddy_get_gui_infos (buddy, &prefix[0], &prefix_color, NULL, NULL);
            if ((prefix[0] == ' ')
                && !weechat_config_boolean (weechat_config_get ("weechat.look.nickmode_empty")))
                prefix[0] = '\0';
            snprintf (str_prefix_color, sizeof (str_prefix_color),
                      "weechat.color.nicklist_prefix%d",
                      prefix_color);
        }
        else
        {
            prefix[0] = (weechat_config_boolean (weechat_config_get ("weechat.look.nickmode_empty"))) ?
                ' ' : '\0';
            snprintf (str_prefix_color, sizeof (str_prefix_color),
                      "weechat.color.chat");
        }
    }
    else
    {
        prefix[0] = '\0';
        snprintf (str_prefix_color, sizeof (str_prefix_color), "chat");
    }
    
    snprintf (result, sizeof (result), "%s%s%s%s%s%s%s%s\t",
              (weechat_config_string (jabber_config_look_nick_prefix)
               && weechat_config_string (jabber_config_look_nick_prefix)[0]) ?
              JABBER_COLOR_CHAT_DELIMITERS : "",
              (weechat_config_string (jabber_config_look_nick_prefix)
               && weechat_config_string (jabber_config_look_nick_prefix)[0]) ?
              weechat_config_string (jabber_config_look_nick_prefix) : "",
              weechat_color(weechat_config_string(weechat_config_get(str_prefix_color))),
              prefix,
              (force_color) ? force_color : ((buddy) ? buddy->color : JABBER_COLOR_CHAT_NICK),
              (buddy) ? buddy->name : buddyname,
              (weechat_config_string (jabber_config_look_nick_suffix)
               && weechat_config_string (jabber_config_look_nick_suffix)[0]) ?
              JABBER_COLOR_CHAT_DELIMITERS : "",
              (weechat_config_string (jabber_config_look_nick_suffix)
               && weechat_config_string (jabber_config_look_nick_suffix)[0]) ?
              weechat_config_string (jabber_config_look_nick_suffix) : "");
    
    return result;
}

/*
 * jabber_buddy_add_to_infolist: add a buddy in an infolist
 *                               return 1 if ok, 0 if error
 */

int
jabber_buddy_add_to_infolist (struct t_infolist *infolist,
                              struct t_jabber_buddy *buddy)
{
    struct t_infolist_item *ptr_item;
    
    if (!infolist || !buddy)
        return 0;
    
    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;
    
    if (!weechat_infolist_new_var_string (ptr_item, "name", buddy->name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "host", buddy->host))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "flags", buddy->flags))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "color", buddy->color))
        return 0;
    
    return 1;
}

/*
 * jabber_buddy_print_log: print buddy infos in log (usually for crash dump)
 */

void
jabber_buddy_print_log (struct t_jabber_buddy *buddy)
{
    weechat_log_printf ("");
    weechat_log_printf ("    => buddy %s (addr:0x%lx):",   buddy->name, buddy);
    weechat_log_printf ("         host . . . . . : %s",    buddy->host);
    weechat_log_printf ("         flags. . . . . : %d",    buddy->flags);
    weechat_log_printf ("         color. . . . . : '%s'",  buddy->color);
    weechat_log_printf ("         prev_buddy . . : 0x%lx", buddy->prev_buddy);
    weechat_log_printf ("         next_buddy . . : 0x%lx", buddy->next_buddy);
}
