/*
 * Copyright (c) 2003 by FlashCode <flashcode@flashtux.org>
 *                       Bounga <bounga@altern.org>
 *                       Xahlexx <xahlexx@tuxisland.org>
 * See README for License detail.
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
#include "../irc/irc.h"
#include "command.h"


/*
 * completion_init: init completion
 */

void
completion_init (t_completion *completion)
{
    completion->position = -1;
    completion->base_word = NULL;
}

/*
 * completion_free: free completion
 */

void
completion_free (t_completion *completion)
{
    if (completion->base_word)
        free (completion->base_word);
}

/*
 * completion_command: complete a command
 */

void
completion_command (t_completion *completion)
{
    int length, word_found_seen;
    t_index_command *ptr_index;
    
    length = strlen (completion->base_word) - 1;
    word_found_seen = 0;
    for (ptr_index = index_commands; ptr_index; ptr_index = ptr_index->next_index)
    {
        if (strncasecmp (ptr_index->command_name, completion->base_word + 1, length) == 0)
        {
            if ((!completion->word_found) || word_found_seen)
            {
                completion->word_found = ptr_index->command_name;
                return;
            }
        }
        if (completion->word_found &&
            (strcasecmp (ptr_index->command_name, completion->word_found) == 0))
            word_found_seen = 1;
    }
    if (completion->word_found)
    {
        completion->word_found = NULL;
        completion_command (completion);
    }
}

/*
 * completion_nick: complete a nick
 */

void
completion_nick (t_completion *completion, t_irc_channel *channel)
{
    int length, word_found_seen;
    t_irc_nick *ptr_nick;

    length = strlen (completion->base_word);
    word_found_seen = 0;
    for (ptr_nick = channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
        if (strncasecmp (ptr_nick->nick, completion->base_word, length) == 0)
        {
            if ((!completion->word_found) || word_found_seen)
            {
                completion->word_found = ptr_nick->nick;
                return;
            }
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
    int i, pos_start, pos_end;
    char *old_word_found;
    
    /* TODO: complete when no word is there with command according to context */
    if (size == 0)
    {
        completion->word_found = NULL;
        return;
    }
    
    /* if new complation => look for base word */
    if (pos != completion->position)
    {
        completion->word_found = NULL;
        
        if ((pos == size) || (buffer[pos-1] != ' '))
            pos--;
        if ((pos > 0) && (buffer[pos] == ' '))
            return;
        
        i = pos;
        while ((i >= 0) && (buffer[i] != ' '))
            i--;
        pos_start = i + 1;
        i = pos;
        while ((i < size) && (buffer[i] != ' '))
            i++;
        pos_end = i - 1;
        
        if (pos_start > pos_end)
            return;
        
        completion->base_word_pos = pos_start;
        
        if (completion->base_word)
            free (completion->base_word);
        completion->base_word = (char *) malloc (pos_end - pos_start + 2);
        
        for (i = pos_start; i <= pos_end; i++)
            completion->base_word[i - pos_start] = buffer[i];
        completion->base_word[pos_end - pos_start + 1] = '\0';
        
        if (completion->base_word[0] == '/')
            completion->position_replace = pos_start + 1;
        else
            completion->position_replace = pos_start;
    }
    
    /* completion */
    old_word_found = completion->word_found;
    if (completion->base_word[0] == '/')
    {
        completion_command (completion);
        if (completion->word_found)
        {
            if (old_word_found)
                completion->diff_size = strlen (completion->word_found) -
                                        strlen (old_word_found);
            else
                completion->diff_size = strlen (completion->word_found) -
                                        strlen (completion->base_word) + 1;
        }
    }
    else
    {
        if (channel)
        {
            completion_nick (completion, (t_irc_channel *)channel);
            if (completion->word_found)
            {
                if (old_word_found)
                    completion->diff_size = strlen (completion->word_found) -
                                            strlen (old_word_found);
                else
                    completion->diff_size = strlen (completion->word_found) -
                                            strlen (completion->base_word);
            }
        }
    }
}
