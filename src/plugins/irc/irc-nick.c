/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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

/* irc-nick.c: manages nick list for channels for IRC plugin */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-nick.h"
#include "irc-config.h"
#include "irc-server.h"
#include "irc-channel.h"


/*
 * irc_nick_valid: check if a nick pointer exists for a channel
 *                       return 1 if nick exists
 *                              0 if nick is not found
 */

int
irc_nick_valid (struct t_irc_channel *channel, struct t_irc_nick *nick)
{
    struct t_irc_nick *ptr_nick;
    
    if (!channel)
        return 0;
    
    for (ptr_nick = channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
        if (ptr_nick == nick)
            return 1;
    }
    
    /* nick not found */
    return 0;
}

/*
 * irc_nick_find_color: find a color for a nick (according to nick letters)
 */

char *
irc_nick_find_color (struct t_irc_nick *nick)
{
    int i, color;
    char color_name[64];
    
    color = 0;
    for (i = strlen (nick->name) - 1; i >= 0; i--)
    {
        color += (int)(nick->name[i]);
    }
    color = (color %
             weechat_config_integer (weechat_config_get ("weechat.look.color_nicks_number")));

    snprintf (color_name, sizeof (color_name),
              "chat_nick_color%02d", color + 1);
    
    return weechat_color (color_name);
}

/*
 * irc_nick_get_gui_infos: get GUI infos for a nick (sort_index, prefix,
 *                         prefix color)
 */

void
irc_nick_get_gui_infos (struct t_irc_nick *nick,
                        char *prefix, int *prefix_color,
                        struct t_gui_buffer *buffer,
                        struct t_gui_nick_group **group)
{
    if (nick->flags & IRC_NICK_CHANOWNER)
    {
        if (prefix)
            *prefix = '~';
        if (prefix_color)
            *prefix_color = 1;
        if (buffer && group)
            *group = weechat_nicklist_search_group (buffer, NULL,
                                                    IRC_NICK_GROUP_OP);
    }
    else if (nick->flags & IRC_NICK_CHANADMIN)
    {
        if (prefix)
            *prefix = '&';
        if (prefix_color)
            *prefix_color = 1;
        if (buffer && group)
            *group = weechat_nicklist_search_group (buffer, NULL,
                                                    IRC_NICK_GROUP_OP);
    }
    else if (nick->flags & IRC_NICK_CHANADMIN2)
    {
        if (prefix)
            *prefix = '!';
        if (prefix_color)
            *prefix_color = 1;
        if (buffer && group)
            *group = weechat_nicklist_search_group (buffer, NULL,
                                                    IRC_NICK_GROUP_OP);
    }
    else if (nick->flags & IRC_NICK_OP)
    {
        if (prefix)
            *prefix = '@';
        if (prefix_color)
            *prefix_color = 1;
        if (buffer && group)
            *group = weechat_nicklist_search_group (buffer, NULL,
                                                    IRC_NICK_GROUP_OP);
    }
    else if (nick->flags & IRC_NICK_HALFOP)
    {
        if (prefix)
            *prefix = '%';
        if (prefix_color)
            *prefix_color = 2;
        if (buffer && group)
            *group = weechat_nicklist_search_group (buffer, NULL,
                                                    IRC_NICK_GROUP_HALFOP);
    }
    else if (nick->flags & IRC_NICK_VOICE)
    {
        if (prefix)
            *prefix = '+';
        if (prefix_color)
            *prefix_color = 3;
        if (buffer && group)
            *group = weechat_nicklist_search_group (buffer, NULL,
                                                    IRC_NICK_GROUP_VOICE);
    }
    else if (nick->flags & IRC_NICK_CHANUSER)
    {
        if (prefix)
            *prefix = '-';
        if (prefix_color)
            *prefix_color = 4;
        if (buffer && group)
            *group = weechat_nicklist_search_group (buffer, NULL,
                                                    IRC_NICK_GROUP_CHANUSER);
    }
    else
    {
        if (prefix)
            *prefix = ' ';
        if (prefix_color)
            *prefix_color = 0;
        if (buffer && group)
            *group = weechat_nicklist_search_group (buffer, NULL,
                                                    IRC_NICK_GROUP_NORMAL);
    }
}

/*
 * irc_nick_new: allocate a new nick for a channel and add it to the nick list
 */

struct t_irc_nick *
irc_nick_new (struct t_irc_server *server, struct t_irc_channel *channel,
              const char *nick_name, int is_chanowner, int is_chanadmin,
              int is_chanadmin2, int is_op, int is_halfop, int has_voice,
              int is_chanuser, int is_away)
{
    struct t_irc_nick *new_nick, *ptr_nick;
    char prefix, str_prefix_color[64];
    int prefix_color;
    struct t_gui_nick_group *ptr_group;
    
    /* nick already exists on this channel? */
    ptr_nick = irc_nick_search (channel, nick_name);
    if (ptr_nick)
    {
        /* remove old nick from nicklist */
        irc_nick_get_gui_infos (ptr_nick, &prefix,
                                &prefix_color, channel->buffer, &ptr_group);
        weechat_nicklist_remove_nick (channel->buffer,
                                      weechat_nicklist_search_nick (channel->buffer,
                                                                    ptr_group,
                                                                    ptr_nick->name));
        
        /* update nick */
        IRC_NICK_SET_FLAG(ptr_nick, is_chanowner, IRC_NICK_CHANOWNER);
        IRC_NICK_SET_FLAG(ptr_nick, is_chanadmin, IRC_NICK_CHANADMIN);
        IRC_NICK_SET_FLAG(ptr_nick, is_chanadmin2, IRC_NICK_CHANADMIN2);
        IRC_NICK_SET_FLAG(ptr_nick, is_op, IRC_NICK_OP);
        IRC_NICK_SET_FLAG(ptr_nick, is_halfop, IRC_NICK_HALFOP);
        IRC_NICK_SET_FLAG(ptr_nick, has_voice, IRC_NICK_VOICE);
        IRC_NICK_SET_FLAG(ptr_nick, is_chanuser, IRC_NICK_CHANUSER);
        IRC_NICK_SET_FLAG(ptr_nick, is_away, IRC_NICK_AWAY);
        
        /* add new nick in nicklist */
        irc_nick_get_gui_infos (ptr_nick, &prefix,
                                &prefix_color, channel->buffer, &ptr_group);
        snprintf (str_prefix_color, sizeof (str_prefix_color),
                  "weechat.color.nicklist_prefix%d",
                  prefix_color);
        weechat_nicklist_add_nick (channel->buffer, ptr_group,
                                   ptr_nick->name,
                                   (is_away) ?
                                   "weechat.color.nicklist_away" : "weechat.color.nicklist",
                                   prefix, str_prefix_color, 1);
        
        return ptr_nick;
    }
    
    /* alloc memory for new nick */
    if ((new_nick = malloc (sizeof (*new_nick))) == NULL)
        return NULL;
    
    /* initialize new nick */
    new_nick->name = strdup (nick_name);
    new_nick->host = NULL;
    new_nick->flags = 0;
    IRC_NICK_SET_FLAG(new_nick, is_chanowner, IRC_NICK_CHANOWNER);
    IRC_NICK_SET_FLAG(new_nick, is_chanadmin, IRC_NICK_CHANADMIN);
    IRC_NICK_SET_FLAG(new_nick, is_chanadmin2, IRC_NICK_CHANADMIN2);
    IRC_NICK_SET_FLAG(new_nick, is_op, IRC_NICK_OP);
    IRC_NICK_SET_FLAG(new_nick, is_halfop, IRC_NICK_HALFOP);
    IRC_NICK_SET_FLAG(new_nick, has_voice, IRC_NICK_VOICE);
    IRC_NICK_SET_FLAG(new_nick, is_chanuser, IRC_NICK_CHANUSER);
    IRC_NICK_SET_FLAG(new_nick, is_away, IRC_NICK_AWAY);
    if (weechat_strcasecmp (new_nick->name, server->nick) == 0)
        new_nick->color = IRC_COLOR_CHAT_NICK_SELF;
    else
        new_nick->color = irc_nick_find_color (new_nick);
    
    /* add nick to end of list */
    new_nick->prev_nick = channel->last_nick;
    if (channel->nicks)
        channel->last_nick->next_nick = new_nick;
    else
        channel->nicks = new_nick;
    channel->last_nick = new_nick;
    new_nick->next_nick = NULL;
    
    channel->nicks_count++;
    
    channel->nick_completion_reset = 1;
    
    /* add nick to buffer nicklist */
    irc_nick_get_gui_infos (new_nick, &prefix, &prefix_color,
                            channel->buffer, &ptr_group);
    snprintf (str_prefix_color, sizeof (str_prefix_color),
              "weechat.color.nicklist_prefix%d",
              prefix_color);
    weechat_nicklist_add_nick (channel->buffer, ptr_group,
                               new_nick->name,
                               (is_away) ?
                               "weechat.color.nicklist_away" : "weechat.color.nicklist",
                               prefix, str_prefix_color, 1);
    
    /* all is ok, return address of new nick */
    return new_nick;
}

/*
 * irc_nick_change: change nickname
 */

void
irc_nick_change (struct t_irc_server *server, struct t_irc_channel *channel,
                 struct t_irc_nick *nick, const char *new_nick)
{
    int nick_is_me, prefix_color;
    struct t_gui_nick_group *ptr_group;
    char prefix, str_prefix_color[64];
    
    /* remove nick from nicklist */
    irc_nick_get_gui_infos (nick, &prefix, &prefix_color,
                            channel->buffer, &ptr_group);
    weechat_nicklist_remove_nick (channel->buffer,
                                  weechat_nicklist_search_nick (channel->buffer,
                                                                ptr_group,
                                                                nick->name));
    
    /* update nicks speaking */
    nick_is_me = (strcmp (nick->name, server->nick) == 0) ? 1 : 0;
    if (!nick_is_me && channel->nicks_speaking)
    {
        weechat_list_set (weechat_list_search (channel->nicks_speaking,
                                               nick->name),
                          new_nick);
    }
    
    /* change nickname */
    if (nick->name)
        free (nick->name);
    nick->name = strdup (new_nick);
    if (nick_is_me)
        nick->color = IRC_COLOR_CHAT_NICK_SELF;
    else
        nick->color = irc_nick_find_color (nick);
    
    /* add nick in nicklist */
    irc_nick_get_gui_infos (nick, &prefix, &prefix_color,
                            channel->buffer, &ptr_group);
    snprintf (str_prefix_color, sizeof (str_prefix_color),
              "weechat.color.nicklist_prefix%d",
              prefix_color);
    weechat_nicklist_add_nick (channel->buffer, ptr_group,
                               nick->name, "weechat.color.nicklist",
                               prefix, str_prefix_color, 1);
}

/*
 * irc_nick_set: set a flag for a nick
 */

void
irc_nick_set (struct t_irc_channel *channel,
              struct t_irc_nick *nick, int set, int flag)
{
    char prefix, str_prefix_color[64];
    int prefix_color;
    struct t_gui_nick_group *ptr_group;
    
    /* remove nick from nicklist */
    irc_nick_get_gui_infos (nick, &prefix, &prefix_color,
                            channel->buffer, &ptr_group);
    weechat_nicklist_remove_nick (channel->buffer,
                                  weechat_nicklist_search_nick (channel->buffer,
                                                                ptr_group,
                                                                nick->name));
    
    /* set flag */
    IRC_NICK_SET_FLAG(nick, set, flag);
    
    /* add nick in nicklist */
    irc_nick_get_gui_infos (nick, &prefix, &prefix_color,
                            channel->buffer, &ptr_group);
    snprintf (str_prefix_color, sizeof (str_prefix_color),
              "weechat.color.nicklist_prefix%d",
              prefix_color);
    weechat_nicklist_add_nick (channel->buffer, ptr_group,
                               nick->name,
                               (nick->flags & IRC_NICK_AWAY) ?
                               "weechat.color.nicklist_away" : "weechat.color.nicklist",
                               prefix, str_prefix_color, 1);
}

/*
 * irc_nick_free: free a nick and remove it from nicks list
 */

void
irc_nick_free (struct t_irc_channel *channel, struct t_irc_nick *nick)
{
    struct t_irc_nick *new_nicks;
    char prefix;
    int prefix_color;
    struct t_gui_nick_group *ptr_group;
    
    if (!channel || !nick)
        return;
    
    /* remove nick from nicklist */
    irc_nick_get_gui_infos (nick, &prefix, &prefix_color,
                            channel->buffer, &ptr_group);
    weechat_nicklist_remove_nick (channel->buffer,
                                  weechat_nicklist_search_nick (channel->buffer,
                                                                ptr_group,
                                                                nick->name));
    
    /* remove nick */
    if (channel->last_nick == nick)
        channel->last_nick = nick->prev_nick;
    if (nick->prev_nick)
    {
        (nick->prev_nick)->next_nick = nick->next_nick;
        new_nicks = channel->nicks;
    }
    else
        new_nicks = nick->next_nick;
    
    if (nick->next_nick)
        (nick->next_nick)->prev_nick = nick->prev_nick;
    
    channel->nicks_count--;
    
    /* free data */
    if (nick->name)
        free (nick->name);
    if (nick->host)
        free (nick->host);
    
    free (nick);
    
    channel->nicks = new_nicks;
    channel->nick_completion_reset = 1;
}

/*
 * irc_nick_free_all: free all allocated nicks for a channel
 */

void
irc_nick_free_all (struct t_irc_channel *channel)
{
    if (!channel)
        return;
    
    /* remove all nicks for the channel */
    while (channel->nicks)
        irc_nick_free (channel, channel->nicks);
    
    /* sould be zero, but prevent any bug :D */
    channel->nicks_count = 0;
}

/*
 * irc_nick_search: returns pointer on a nick
 */

struct t_irc_nick *
irc_nick_search (struct t_irc_channel *channel, const char *nickname)
{
    struct t_irc_nick *ptr_nick;
    
    if (!nickname)
        return NULL;
    
    for (ptr_nick = channel->nicks; ptr_nick;
         ptr_nick = ptr_nick->next_nick)
    {
        if (weechat_strcasecmp (ptr_nick->name, nickname) == 0)
            return ptr_nick;
    }
    return NULL;
}

/*
 * irc_nick_count: returns number of nicks (total, op, halfop, voice) on a channel
 */

void
irc_nick_count (struct t_irc_channel *channel, int *total, int *count_op,
                int *count_halfop, int *count_voice, int *count_normal)
{
    struct t_irc_nick *ptr_nick;
    
    (*total) = 0;
    (*count_op) = 0;
    (*count_halfop) = 0;
    (*count_voice) = 0;
    (*count_normal) = 0;
    for (ptr_nick = channel->nicks; ptr_nick;
         ptr_nick = ptr_nick->next_nick)
    {
        (*total)++;
        if ((ptr_nick->flags & IRC_NICK_CHANOWNER) ||
            (ptr_nick->flags & IRC_NICK_CHANADMIN) ||
            (ptr_nick->flags & IRC_NICK_CHANADMIN2) ||
            (ptr_nick->flags & IRC_NICK_OP))
            (*count_op)++;
        else
        {
            if (ptr_nick->flags & IRC_NICK_HALFOP)
                (*count_halfop)++;
            else
            {
                if (ptr_nick->flags & IRC_NICK_VOICE)
                    (*count_voice)++;
                else
                    (*count_normal)++;
            }
        }
    }
}

/*
 * irc_nick_set_away: set/unset away status for a channel
 */

void
irc_nick_set_away (struct t_irc_channel *channel, struct t_irc_nick *nick,
                   int is_away)
{
    if ((weechat_config_integer (irc_config_network_away_check) > 0)
        && ((weechat_config_integer (irc_config_network_away_check_max_nicks) == 0) ||
            (channel->nicks_count <= weechat_config_integer (irc_config_network_away_check_max_nicks))))
    {
        if (((is_away) && (!(nick->flags & IRC_NICK_AWAY))) ||
            ((!is_away) && (nick->flags & IRC_NICK_AWAY)))
        {
            irc_nick_set (channel, nick, is_away, IRC_NICK_AWAY);
        }
    }
}

/*
 * irc_nick_as_prefix: return string with nick to display as prefix on buffer
 *                     (string will end by a tab)
 */

char *
irc_nick_as_prefix (struct t_irc_nick *nick, const char *nickname,
                    const char *force_color)
{
    static char result[256];
    char prefix[2], str_prefix_color[64];
    int prefix_color;

    prefix[0] = '\0';
    prefix[1] = '\0';
    if (weechat_config_boolean (weechat_config_get ("weechat.look.nickmode")))
    {
        if (nick)
        {
            irc_nick_get_gui_infos (nick, &prefix[0], &prefix_color, NULL, NULL);
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
              (weechat_config_string (irc_config_look_nick_prefix)
               && weechat_config_string (irc_config_look_nick_prefix)[0]) ?
              IRC_COLOR_CHAT_DELIMITERS : "",
              (weechat_config_string (irc_config_look_nick_prefix)
               && weechat_config_string (irc_config_look_nick_prefix)[0]) ?
              weechat_config_string (irc_config_look_nick_prefix) : "",
              weechat_color(weechat_config_string(weechat_config_get(str_prefix_color))),
              prefix,
              (force_color) ? force_color : ((nick) ? nick->color : IRC_COLOR_CHAT_NICK),
              (nick) ? nick->name : nickname,
              (weechat_config_string (irc_config_look_nick_suffix)
               && weechat_config_string (irc_config_look_nick_suffix)[0]) ?
              IRC_COLOR_CHAT_DELIMITERS : "",
              (weechat_config_string (irc_config_look_nick_suffix)
               && weechat_config_string (irc_config_look_nick_suffix)[0]) ?
              weechat_config_string (irc_config_look_nick_suffix) : "");
    
    return result;
}

/*
 * irc_nick_add_to_infolist: add a nick in an infolist
 *                           return 1 if ok, 0 if error
 */

int
irc_nick_add_to_infolist (struct t_infolist *infolist,
                          struct t_irc_nick *nick)
{
    struct t_infolist_item *ptr_item;
    
    if (!infolist || !nick)
        return 0;
    
    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;
    
    if (!weechat_infolist_new_var_string (ptr_item, "name", nick->name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "host", nick->host))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "flags", nick->flags))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "color", nick->color))
        return 0;
    
    return 1;
}

/*
 * irc_nick_print_log: print nick infos in log (usually for crash dump)
 */

void
irc_nick_print_log (struct t_irc_nick *nick)
{
    weechat_log_printf ("");
    weechat_log_printf ("    => nick %s (addr:0x%lx):",    nick->name, nick);
    weechat_log_printf ("         host . . . . . : %s",    nick->host);
    weechat_log_printf ("         flags. . . . . : %d",    nick->flags);
    weechat_log_printf ("         color. . . . . : '%s'",  nick->color);
    weechat_log_printf ("         prev_nick. . . : 0x%lx", nick->prev_nick);
    weechat_log_printf ("         next_nick. . . : 0x%lx", nick->next_nick);
}
