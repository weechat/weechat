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

/* irc-nick.c: manages nick list for channels */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "irc.h"
#include "irc-nick.h"
#include "irc-config.h"
#include "irc-server.h"
#include "irc-channel.h"


/*
 * irc_nick_find_color: find a color for a nick (according to nick letters)
 */

int
irc_nick_find_color (struct t_irc_nick *nick)
{
    int i, color;
    
    color = 0;
    for (i = strlen (nick->nick) - 1; i >= 0; i--)
    {
        color += (int)(nick->nick[i]);
    }
    color = (color %
             weechat_config_integer (weechat_config_get_weechat ("look_color_nicks_number")));
    
    return color;
}

/*
 * irc_nick_get_gui_infos: get GUI infos for a nick (sort_index, prefix,
 *                         prefix color)
 */

void
irc_nick_get_gui_infos (struct t_irc_nick *nick,
                        int *sort_index, char *prefix, int *color_prefix)
{
    if (nick->flags & IRC_NICK_CHANOWNER)
    {
        *sort_index = 1;
        *prefix = '~';
        *color_prefix = 1;
    }
    else if (nick->flags & IRC_NICK_CHANADMIN)
    {
        *sort_index = 2;
        *prefix = '&';
        *color_prefix = 1;
    }
    else if (nick->flags & IRC_NICK_CHANADMIN2)
    {
        *sort_index = 3;
        *prefix = '!';
        *color_prefix = 1;
    }
    else if (nick->flags & IRC_NICK_OP)
    {
        *sort_index = 4;
        *prefix = '@';
        *color_prefix = 1;
    }
    else if (nick->flags & IRC_NICK_HALFOP)
    {
        *sort_index = 5;
        *prefix = '%';
        *color_prefix = 2;
    }
    else if (nick->flags & IRC_NICK_VOICE)
    {
        *sort_index = 6;
        *prefix = '+';
        *color_prefix = 3;
    }
    else if (nick->flags & IRC_NICK_CHANUSER)
    {
        *sort_index = 7;
        *prefix = '-';
        *color_prefix = 4;
    }
    else
    {
        *sort_index = 8;
        *prefix = ' ';
        *color_prefix = 0;
    }
}

/*
 * irc_nick_new: allocate a new nick for a channel and add it to the nick list
 */

struct t_irc_nick *
irc_nick_new (struct t_irc_server *server, struct t_irc_channel *channel,
              char *nick_name, int is_chanowner, int is_chanadmin,
              int is_chanadmin2, int is_op, int is_halfop, int has_voice,
              int is_chanuser)
{
    (void) server;
    (void) channel;
    (void) nick_name;
    (void) is_chanowner;
    (void) is_chanadmin;
    (void) is_chanadmin2;
    (void) is_op;
    (void) is_halfop;
    (void) has_voice;
    (void) is_chanuser;
    
    /*
    struct t_irc_nick *new_nick;
    struct t_gui_nick *ptr_gui_nick;
    int sort_index, color_prefix;
    char prefix;
    
    // nick already exists on this channel?
    if ((new_nick = irc_nick_search (channel, nick_name)))
    {
        // update nick
        IRC_NICK_SET_FLAG(new_nick, is_chanowner, IRC_NICK_CHANOWNER);
        IRC_NICK_SET_FLAG(new_nick, is_chanadmin, IRC_NICK_CHANADMIN);
        IRC_NICK_SET_FLAG(new_nick, is_chanadmin2, IRC_NICK_CHANADMIN2);
        IRC_NICK_SET_FLAG(new_nick, is_op, IRC_NICK_OP);
        IRC_NICK_SET_FLAG(new_nick, is_halfop, IRC_NICK_HALFOP);
        IRC_NICK_SET_FLAG(new_nick, has_voice, IRC_NICK_VOICE);
        IRC_NICK_SET_FLAG(new_nick, is_chanuser, IRC_NICK_CHANUSER);
        irc_nick_get_gui_infos (new_nick, &sort_index, &prefix, &color_prefix);
        ptr_gui_nick = gui_nicklist_search (channel->buffer, new_nick->nick);
        if (ptr_gui_nick)
            gui_nicklist_update (channel->buffer, ptr_gui_nick, NULL,
                                 sort_index,
                                 ptr_gui_nick->color_nick,
                                 prefix,
                                 color_prefix);
                             
        return new_nick;
    }
    
    // alloc memory for new nick
    if ((new_nick = (struct t_irc_nick *)malloc (sizeof (struct t_irc_nick))) == NULL)
        return NULL;
    
    // initialize new nick
    new_nick->nick = strdup (nick_name);
    new_nick->host = NULL;
    new_nick->flags = 0;
    IRC_NICK_SET_FLAG(new_nick, is_chanowner, IRC_NICK_CHANOWNER);
    IRC_NICK_SET_FLAG(new_nick, is_chanadmin, IRC_NICK_CHANADMIN);
    IRC_NICK_SET_FLAG(new_nick, is_chanadmin2, IRC_NICK_CHANADMIN2);
    IRC_NICK_SET_FLAG(new_nick, is_op, IRC_NICK_OP);
    IRC_NICK_SET_FLAG(new_nick, is_halfop, IRC_NICK_HALFOP);
    IRC_NICK_SET_FLAG(new_nick, has_voice, IRC_NICK_VOICE);
    IRC_NICK_SET_FLAG(new_nick, is_chanuser, IRC_NICK_CHANUSER);
    if (weechat_strcasecmp (new_nick->nick, server->nick) == 0)
        new_nick->color = GUI_COLOR_CHAT_NICK_SELF;
    else
        new_nick->color = irc_nick_find_color (new_nick);

    // add nick to end of list
    new_nick->prev_nick = channel->last_nick;
    if (channel->nicks)
        channel->last_nick->next_nick = new_nick;
    else
        channel->nicks = new_nick;
    channel->last_nick = new_nick;
    new_nick->next_nick = NULL;
    
    channel->nicks_count++;
    
    channel->nick_completion_reset = 1;
    
    // add nick to buffer nicklist
    irc_nick_get_gui_infos (new_nick, &sort_index, &prefix, &color_prefix);
    gui_nicklist_add (channel->buffer, new_nick->nick, sort_index,
                      GUI_COLOR_NICKLIST, prefix, color_prefix);
    
    // all is ok, return address of new nick
    return new_nick;
    */
    return NULL;
}

/*
 * irc_nick_change: change nickname
 */

void
irc_nick_change (struct t_irc_server *server, struct t_irc_channel *channel,
                 struct t_irc_nick *nick, char *new_nick)
{
    (void) server;
    (void) channel;
    (void) nick;
    (void) new_nick;
    
    /*
    int nick_is_me;
    struct t_weelist *ptr_weelist;
    struct t_gui_nick *ptr_nick;
    
    // update buffer nick
    ptr_nick = gui_nicklist_search (channel->buffer, nick->nick);
    if (ptr_nick)
        gui_nicklist_update (channel->buffer, ptr_nick, new_nick,
                             ptr_nick->sort_index,
                             ptr_nick->color_nick,
                             ptr_nick->prefix,
                             ptr_nick->color_prefix);
    
    nick_is_me = (strcmp (nick->nick, server->nick) == 0) ? 1 : 0;
    
    if (!nick_is_me && channel->nicks_speaking)
    {
        ptr_weelist = weelist_search (channel->nicks_speaking, nick->nick);
        if (ptr_weelist && ptr_weelist->data)
        {
            free (ptr_weelist->data);
            ptr_weelist->data = strdup (new_nick);
        }
    }
    
    // change nickname
    if (nick->nick)
        free (nick->nick);
    nick->nick = strdup (new_nick);
    if (nick_is_me)
        nick->color = GUI_COLOR_CHAT_NICK_SELF;
    else
        nick->color = irc_nick_find_color (nick);
    */
}

/*
 * irc_nick_free: free a nick and remove it from nicks list
 */

void
irc_nick_free (struct t_irc_channel *channel, struct t_irc_nick *nick)
{
    (void) channel;
    (void) nick;
    
    /*
    struct t_irc_nick *new_nicks;
    
    if (!channel || !nick)
        return;
    
    // remove nick from buffer nicklist
    (void) gui_nicklist_remove (channel->buffer, nick->nick);
    
    // remove nick from nicks list
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
    
    // free data
    if (nick->nick)
        free (nick->nick);
    if (nick->host)
        free (nick->host);
    free (nick);
    channel->nicks = new_nicks;
    
    channel->nick_completion_reset = 1;
    */
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
irc_nick_search (struct t_irc_channel *channel, char *nickname)
{
    struct t_irc_nick *ptr_nick;
    
    if (!nickname)
        return NULL;
    
    for (ptr_nick = channel->nicks; ptr_nick;
         ptr_nick = ptr_nick->next_nick)
    {
        if (weechat_strcasecmp (ptr_nick->nick, nickname) == 0)
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
    (void) channel;
    (void) nick;
    (void) is_away;
    
    /*
    t_gui_nick *ptr_nick;
    
    if ((irc_cfg_irc_away_check > 0)
        && ((irc_cfg_irc_away_check_max_nicks == 0) ||
            (channel->nicks_count <= irc_cfg_irc_away_check_max_nicks)))
    {
        if (((is_away) && (!(nick->flags & IRC_NICK_AWAY))) ||
            ((!is_away) && (nick->flags & IRC_NICK_AWAY)))
        {
            IRC_NICK_SET_FLAG(nick, is_away, IRC_NICK_AWAY);
            ptr_nick = gui_nicklist_search (channel->buffer, nick->nick);
            if (ptr_nick)
                gui_nicklist_update (channel->buffer, ptr_nick, NULL,
                                     ptr_nick->sort_index,
                                     (is_away) ? GUI_COLOR_NICKLIST_AWAY :
                                     GUI_COLOR_NICKLIST,
                                     ptr_nick->prefix,
                                     ptr_nick->color_prefix);
            gui_nicklist_draw (channel->buffer, 0, 0);
        }
    }
    */
}

/*
 * irc_nick_as_prefix: return string with nick to display as prefix on buffer
 *                     (string will end by a tab)
 */

char *
irc_nick_as_prefix (struct t_irc_nick *nick, char *nickname, char *force_color)
{
    static char result[256];
    
    snprintf (result, sizeof (result), "%s%s%s%s%s%s\t",
              (weechat_config_string (irc_config_irc_nick_prefix)
               && weechat_config_string (irc_config_irc_nick_prefix)[0]) ?
              IRC_COLOR_CHAT_DELIMITERS : "",
              (weechat_config_string (irc_config_irc_nick_prefix)
               && weechat_config_string (irc_config_irc_nick_prefix)[0]) ?
              weechat_config_string (irc_config_irc_nick_prefix) : "",
              (force_color) ? force_color : ((nick) ? nick->color : IRC_COLOR_CHAT_NICK),
              (nick) ? nick->nick : nickname,
              (weechat_config_string (irc_config_irc_nick_suffix)
               && weechat_config_string (irc_config_irc_nick_suffix)[0]) ?
              IRC_COLOR_CHAT_DELIMITERS : "",
              (weechat_config_string (irc_config_irc_nick_suffix)
               && weechat_config_string (irc_config_irc_nick_suffix)[0]) ?
              weechat_config_string (irc_config_irc_nick_suffix) : "");
    
    return result;
}

/*
 * irc_nick_print_log: print nick infos in log (usually for crash dump)
 */

void
irc_nick_print_log (struct t_irc_nick *nick)
{
    weechat_log_printf ("=> nick %s (addr:0x%X):",    nick->nick, nick);
    weechat_log_printf ("     host . . . . . : %s",   nick->host);
    weechat_log_printf ("     flags. . . . . : %d",   nick->flags);
    weechat_log_printf ("     color. . . . . : %d",   nick->color);
    weechat_log_printf ("     prev_nick. . . : 0x%X", nick->prev_nick);
    weechat_log_printf ("     next_nick. . . : 0x%X", nick->next_nick);
}
