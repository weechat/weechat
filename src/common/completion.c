/*
 * Copyright (c) 2003-2005 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* completion.c: completes words according to context (cmd/nick) */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "weechat.h"
#include "completion.h"
#include "command.h"
#include "weelist.h"
#include "weeconfig.h"
#include "../irc/irc.h"


/*
 * completion_init: init completion
 */

void
completion_init (t_completion *completion)
{
    completion->context = COMPLETION_NULL;
    completion->base_command = NULL;
    completion->base_command_arg = 0;
    completion->position = -1;
    completion->base_word = NULL;
    
    completion->completion_list = NULL;
    completion->last_completion = NULL;
}

/*
 * completion_free: free completion
 */

void
completion_free (t_completion *completion)
{
    if (completion->base_command)
        free (completion->base_command);
    completion->base_command = NULL;
    
    if (completion->base_word)
        free (completion->base_word);
    completion->base_word = NULL;
    
    while (completion->completion_list)
        weelist_remove (&completion->completion_list,
                        &completion->last_completion,
                        completion->completion_list);
    completion->completion_list = NULL;
    completion->last_completion = NULL;
}

/*
 * completion_stop: stop completion (for example after 1 arg of command with 1 arg)
 */

void
completion_stop (t_completion *completion)
{
    completion->context = COMPLETION_NULL;
    completion->position = -1;
}

/*
 * completion_build_list: build data list according to command and argument #
 */

void
completion_build_list (t_completion *completion, void *channel)
{
    t_weelist *ptr_list;
    int i, j;
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    char option_name[256];
    t_weechat_alias *ptr_alias;
    
    /* WeeChat internal commands */
    
    /* no completion for some commands */
    if ((strcasecmp (completion->base_command, "server") == 0)
        || (strcasecmp (completion->base_command, "save") == 0))
    {
        completion_stop (completion);
        return;
    }
    if ((strcasecmp (completion->base_command, "alias") == 0)
        && (completion->base_command_arg == 1))
    {
        for (ptr_list = index_commands; ptr_list; ptr_list = ptr_list->next_weelist)
        {
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         ptr_list->data);
        }
        return;
    }
    if ((strcasecmp (completion->base_command, "buffer") == 0)
        && (completion->base_command_arg == 1))
    {
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     "close");
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     "list");
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     "move");
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     "notify");
        return;
    }
    if ((strcasecmp (completion->base_command, "clear") == 0)
        && (completion->base_command_arg == 1))
    {
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     "-all");
        return;
    }
    if ((strcasecmp (completion->base_command, "connect") == 0)
        || (strcasecmp (completion->base_command, "disconnect") == 0))
    {
        if (completion->base_command_arg == 1)
        {
            for (ptr_server = irc_servers; ptr_server;
                 ptr_server = ptr_server->next_server)
            {
                weelist_add (&completion->completion_list,
                             &completion->last_completion,
                             ptr_server->name);
            }
            return;
        }
        else
        {
            completion_stop (completion);
            return;
        }
    }
    if (strcasecmp (completion->base_command, "debug") == 0)
    {
        if (completion->base_command_arg == 1)
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "dump");
        else
            completion_stop (completion);
        return;
    }
    if ((strcasecmp (completion->base_command, "help") == 0)
        && (completion->base_command_arg == 1))
    {
        for (i = 0; weechat_commands[i].command_name; i++)
        {
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         weechat_commands[i].command_name);
        }
        for (i = 0; irc_commands[i].command_name; i++)
        {
            if (irc_commands[i].cmd_function_args || irc_commands[i].cmd_function_1arg)
                weelist_add (&completion->completion_list,
                             &completion->last_completion,
                             irc_commands[i].command_name);
        }
        return;
    }
    if ((strcasecmp (completion->base_command, "perl") == 0)
        && (completion->base_command_arg == 1))
    {
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     "load");
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     "autoload");
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     "unload");
        return;
    }
    if ((strcasecmp (completion->base_command, "set") == 0)
        && (completion->base_command_arg == 1))
    {
        for (i = 0; i < CONFIG_NUMBER_SECTIONS; i++)
        {
            if ((i != CONFIG_SECTION_ALIAS) && (i != CONFIG_SECTION_SERVER))
            {
                for (j = 0; weechat_options[i][j].option_name; j++)
                {
                    weelist_add (&completion->completion_list,
                                 &completion->last_completion,
                                 weechat_options[i][j].option_name);
                }
            }
        }
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            for (i = 0; weechat_options[CONFIG_SECTION_SERVER][i].option_name; i++)
            {
                snprintf (option_name, sizeof (option_name), "%s.%s",
                          ptr_server->name, 
                          weechat_options[CONFIG_SECTION_SERVER][i].option_name);
                weelist_add (&completion->completion_list,
                             &completion->last_completion,
                             option_name);
            }
        }
        return;
    }
    if ((strcasecmp (completion->base_command, "unalias") == 0)
        && (completion->base_command_arg == 1))
    {
        for (ptr_alias = weechat_alias; ptr_alias; ptr_alias = ptr_alias->next_alias)
        {
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         ptr_alias->alias_name);
        }
        return;
    }
    if (strcasecmp (completion->base_command, "window") == 0)
    {
        if (completion->base_command_arg == 1)
        {
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "list");
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "splith");
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "splitv");
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "merge");
            return;
        }
        
        if (completion->base_command_arg == 2)
        {
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "down");
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "up");
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "left");
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "right");
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "all");
            return;
        }
        
        completion_stop (completion);
        return;
    }
    
    /* IRC commands */
    
    /* no completion for some commands */
    if ((strcasecmp (completion->base_command, "admin") == 0)
        || (strcasecmp (completion->base_command, "die") == 0)
        || (strcasecmp (completion->base_command, "info") == 0)
        || (strcasecmp (completion->base_command, "join") == 0)
        || (strcasecmp (completion->base_command, "links") == 0)
        || (strcasecmp (completion->base_command, "list") == 0)
        || (strcasecmp (completion->base_command, "lusers") == 0)
        || (strcasecmp (completion->base_command, "motd") == 0)
        || (strcasecmp (completion->base_command, "oper") == 0)
        || (strcasecmp (completion->base_command, "rehash") == 0)
        || (strcasecmp (completion->base_command, "restart") == 0)
        || (strcasecmp (completion->base_command, "service") == 0)
        || (strcasecmp (completion->base_command, "servlist") == 0)
        || (strcasecmp (completion->base_command, "squery") == 0)
        || (strcasecmp (completion->base_command, "squit") == 0)
        || (strcasecmp (completion->base_command, "stats") == 0)
        || (strcasecmp (completion->base_command, "summon") == 0)
        || (strcasecmp (completion->base_command, "time") == 0)
        || (strcasecmp (completion->base_command, "trace") == 0)
        || (strcasecmp (completion->base_command, "users") == 0)
        || (strcasecmp (completion->base_command, "wallops") == 0)
        || (strcasecmp (completion->base_command, "who") == 0))
    {
        completion_stop (completion);
        return;
    }
    if ((strcasecmp (completion->base_command, "away") == 0)
        && (completion->base_command_arg == 1))
    {
        if (cfg_irc_default_msg_away && cfg_irc_default_msg_away[0])
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         cfg_irc_default_msg_away);
        return;
    }
    if ((strcasecmp (completion->base_command, "ctcp") == 0)
        && (completion->base_command_arg == 2))
    {
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     "action");
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     "version");
        return;
    }
    if ((strcasecmp (completion->base_command, "dcc") == 0)
        && (completion->base_command_arg == 1))
    {
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     "chat");
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     "send");
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     "close");
        return;
    }
    if (strcasecmp (completion->base_command, "invite") == 0)
    {
        /* arg1: nickname */
        if (completion->base_command_arg == 1)
            return;
        
        /* arg > 2: not allowed */
        if (completion->base_command_arg > 2)
        {
            completion_stop (completion);
            return;
        }
        
        /* arg2: channel */
        if (SERVER(gui_current_window->buffer))
        {
            for (ptr_channel = SERVER(gui_current_window->buffer)->channels;
                 ptr_channel; ptr_channel = ptr_channel->next_channel)
            {
                weelist_add (&completion->completion_list,
                             &completion->last_completion,
                             ptr_channel->name);
            }
        }
        return;
    }
    if (strcasecmp (completion->base_command, "kick") == 0)
    {
        if (completion->base_command_arg != 1)
            completion_stop (completion);
        return;
    }
    if (strcasecmp (completion->base_command, "kill") == 0)
    {
        if (completion->base_command_arg != 1)
            completion_stop (completion);
        return;
    }
    if (strcasecmp (completion->base_command, "notice") == 0)
    {
        if (completion->base_command_arg != 1)
            completion_stop (completion);
        return;
    }
    if ((strcasecmp (completion->base_command, "part") == 0)
        && (completion->base_command_arg == 1))
    {
        if (cfg_irc_default_msg_part && cfg_irc_default_msg_part[0])
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         cfg_irc_default_msg_part);
        return;
    }
    if (strcasecmp (completion->base_command, "query") == 0)
    {
        if (completion->base_command_arg != 1)
            completion_stop (completion);
        return;
    }
    if ((strcasecmp (completion->base_command, "quit") == 0)
        && (completion->base_command_arg == 1))
    {
        if (cfg_irc_default_msg_quit && cfg_irc_default_msg_quit[0])
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         cfg_irc_default_msg_quit);
        return;
    }
    if (strcasecmp (completion->base_command, "topic") == 0)
    {
        if (completion->base_command_arg == 1)
        {
            if (!channel || !((t_irc_channel *)channel)->topic
                || !((t_irc_channel *)channel)->topic[0])
                completion_stop (completion);
            else
                weelist_add (&completion->completion_list,
                             &completion->last_completion,
                             ((t_irc_channel *)channel)->topic);
        }
        else
            completion_stop (completion);
        return;
    }
}

/*
 * completion_find_context: find context for completion
 */

void
completion_find_context (t_completion *completion, void *channel, char *buffer,
                         int size, int pos)
{
    int i, command, command_arg, pos_start, pos_end;
    
    /* look for context */
    completion_free (completion);
    command = (buffer[0] == '/') ? 1 : 0;
    command_arg = 0;
    i = 0;
    while (i < pos)
    {
        if (buffer[i] == ' ')
        {
            command_arg++;
            i++;
            while ((i < pos) && (buffer[i] == ' ')) i++;
        }
        else
            i++;
    }
    if (command)
    {
        if (command_arg > 0)
        {
            completion->context = COMPLETION_COMMAND_ARG;
            completion->base_command_arg = command_arg;
        }
        else
        {
            completion->context = COMPLETION_COMMAND;
            completion->base_command_arg = 0;
        }
    }
    else
    {
        if (channel)
            completion->context = COMPLETION_NICK;
        else
            completion->context = COMPLETION_NULL;
    }
    
    /* look for word to complete (base word) */
    completion->base_word_pos = 0;
    completion->position_replace = pos;
    
    if (size > 0)
    {
        i = pos;
        pos_start = i;
        if (buffer[i] == ' ')
        {
            if ((i > 0) && (buffer[i-1] != ' '))
            {
                i--;
                while ((i >= 0) && (buffer[i] != ' '))
                    i--;
                pos_start = i + 1;
            }
        }
        else
        {
            while ((i >= 0) && (buffer[i] != ' '))
                i--;
            pos_start = i + 1;
        }
        i = pos;
        while ((i < size) && (buffer[i] != ' '))
            i++;
        pos_end = i - 1;
        
        completion->base_word_pos = pos_start;
        
        if (pos_start <= pos_end)
        {
            if (completion->context == COMPLETION_COMMAND)
                completion->position_replace = pos_start + 1;
            else
                completion->position_replace = pos_start;
            
            completion->base_word = (char *) malloc (pos_end - pos_start + 2);
            for (i = pos_start; i <= pos_end; i++)
                completion->base_word[i - pos_start] = buffer[i];
            completion->base_word[pos_end - pos_start + 1] = '\0';
        }
    }
    
    if (!completion->base_word)
        completion->base_word = strdup ("");
    
    /* find command (for command argument completion only) */
    if (completion->context == COMPLETION_COMMAND_ARG)
    {
        pos_start = 0;
        while ((pos_start < size) && (buffer[pos_start] != '/'))
            pos_start++;
        if (buffer[pos_start] == '/')
        {
            pos_start++;
            pos_end = pos_start;
            while ((pos_end < size) && (buffer[pos_end] != ' '))
                pos_end++;
            if (buffer[pos_end] == ' ')
                pos_end--;
            
            completion->base_command = (char *) malloc (pos_end - pos_start + 2);
            for (i = pos_start; i <= pos_end; i++)
                completion->base_command[i - pos_start] = buffer[i];
            completion->base_command[pos_end - pos_start + 1] = '\0';
            completion_build_list (completion, channel);
        }
    }
    
    if (!completion->completion_list && channel &&
        (((t_irc_channel *)channel)->type == CHAT_PRIVATE)
        && (completion->context == COMPLETION_NICK))
    {
        /* nick completion in private (only other nick and self) */
        completion->context = COMPLETION_NICK;
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     ((t_irc_channel *)channel)->name);
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     SERVER(gui_current_window->buffer)->nick);
    }
}

/*
 * completion_command: complete a command
 */

void
completion_command (t_completion *completion)
{
    int length, word_found_seen, other_completion;
    t_weelist *ptr_weelist, *ptr_weelist2;
    
    length = strlen (completion->base_word) - 1;
    word_found_seen = 0;
    other_completion = 0;
    for (ptr_weelist = index_commands; ptr_weelist; ptr_weelist = ptr_weelist->next_weelist)
    {
        if (strncasecmp (ptr_weelist->data, completion->base_word + 1, length) == 0)
        {
            if ((!completion->word_found) || word_found_seen)
            {
                completion->word_found = ptr_weelist->data;
                for (ptr_weelist2 = ptr_weelist->next_weelist; ptr_weelist2;
                     ptr_weelist2 = ptr_weelist2->next_weelist)
                {
                    if (strncasecmp (ptr_weelist2->data,
                        completion->base_word + 1, length) == 0)
                        other_completion++;
                }
                if (other_completion == 0)
                    completion->position = -1;
                else
                    if (completion->position < 0)
                        completion->position = 0;
                return;
            }
            other_completion++;
        }
        if (completion->word_found &&
            (strcasecmp (ptr_weelist->data, completion->word_found) == 0))
            word_found_seen = 1;
    }
    if (completion->word_found)
    {
        completion->word_found = NULL;
        completion_command (completion);
    }
}

/*
 * completion_command_arg: complete a command argument
 */

void
completion_command_arg (t_completion *completion, t_irc_channel *channel)
{
    int length, word_found_seen, other_completion;
    t_weelist *ptr_weelist, *ptr_weelist2;
    
    length = strlen (completion->base_word);
    word_found_seen = 0;
    other_completion = 0;
    for (ptr_weelist = completion->completion_list; ptr_weelist;
         ptr_weelist = ptr_weelist->next_weelist)
    {
        if (strncasecmp (ptr_weelist->data, completion->base_word, length) == 0)
        {
            if ((!completion->word_found) || word_found_seen)
            {
                completion->word_found = ptr_weelist->data;
                for (ptr_weelist2 = ptr_weelist->next_weelist; ptr_weelist2;
                     ptr_weelist2 = ptr_weelist2->next_weelist)
                {
                    if (strncasecmp (ptr_weelist2->data,
                        completion->base_word, length) == 0)
                        other_completion++;
                }
                if (other_completion == 0)
                    completion->position = -1;
                else
                    if (completion->position < 0)
                        completion->position = 0;
                return;
            }
            other_completion++;
        }
        if (completion->word_found &&
            (strcasecmp (ptr_weelist->data, completion->word_found) == 0))
            word_found_seen = 1;
    }
    if (completion->word_found)
    {
        completion->word_found = NULL;
        completion_command_arg (completion, channel);
    }
}

/*
 * completion_nick: complete a nick
 */

void
completion_nick (t_completion *completion, t_irc_channel *channel)
{
    int length, word_found_seen, other_completion;
    t_irc_nick *ptr_nick, *ptr_nick2;

    if (!channel)
        return;
    
    if (((t_irc_channel *)channel)->type == CHAT_PRIVATE)
    {
        completion_command_arg (completion, channel);
        return;
    }
    
    length = strlen (completion->base_word);
    word_found_seen = 0;
    other_completion = 0;
    for (ptr_nick = channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
        if (strncasecmp (ptr_nick->nick, completion->base_word, length) == 0)
        {
            if ((!completion->word_found) || word_found_seen)
            {
                completion->word_found = ptr_nick->nick;
                for (ptr_nick2 = ptr_nick->next_nick; ptr_nick2;
                     ptr_nick2 = ptr_nick2->next_nick)
                {
                    if (strncasecmp (ptr_nick2->nick,
                        completion->base_word, length) == 0)
                        other_completion++;
                }
                if (other_completion == 0)
                    completion->position = -1;
                else
                    if (completion->position < 0)
                        completion->position = 0;
                return;
            }
            other_completion++;
        }
        if (completion->word_found &&
            (strcasecmp (ptr_nick->nick, completion->word_found) == 0))
            word_found_seen = 1;
    }
    if (completion->word_found)
    {
        completion->word_found = NULL;
        completion_nick (completion, channel);
    }
}

/*
 * completion_search: complete word according to context
 */

void
completion_search (t_completion *completion, void *channel,
                   char *buffer, int size, int pos)
{
    char *old_word_found;
    
    /* if new completion => look for base word */
    if (pos != completion->position)
    {
        completion->word_found = NULL;
        completion_find_context (completion, channel, buffer, size, pos);
    }
    
    /* completion */
    old_word_found = completion->word_found;
    switch (completion->context)
    {
        case COMPLETION_NULL:
            /* should never be executed */
            return;
        case COMPLETION_NICK:
            if (channel)
                completion_nick (completion, (t_irc_channel *)channel);
            else
                return;
            break;
        case COMPLETION_COMMAND:
            completion_command (completion);
            break;
        case COMPLETION_COMMAND_ARG:
            if (completion->completion_list)
                completion_command_arg (completion, (t_irc_channel *)channel);
            else
                completion_nick (completion, (t_irc_channel *)channel);
            break;
    }
    if (completion->word_found)
    {
        if (old_word_found)
            completion->diff_size = strlen (completion->word_found) -
                                    strlen (old_word_found);
        else
        {
            completion->diff_size = strlen (completion->word_found) -
                                    strlen (completion->base_word);
            if (completion->context == COMPLETION_COMMAND)
                completion->diff_size++;
        }
    }
}
