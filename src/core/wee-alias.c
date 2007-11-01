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

/* wee-alias.c: WeeChat alias */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "weechat.h"
#include "wee-alias.h"
#include "wee-config.h"
#include "wee-string.h"


struct alias *weechat_alias = NULL;
struct alias *weechat_last_alias = NULL;


/*
 * alias_search: search an alias
 */

struct alias *
alias_search (char *alias_name)
{
    struct alias *ptr_alias;
    
    for (ptr_alias = weechat_alias; ptr_alias; ptr_alias = ptr_alias->next_alias)
    {
        if (string_strcasecmp (alias_name, ptr_alias->name) == 0)
            return ptr_alias;
    }
    return NULL;
}

/*
 * alias_find_pos: find position for an alias (for sorting aliases)
 */

struct alias *
alias_find_pos (char *alias_name)
{
    struct alias *ptr_alias;
    
    for (ptr_alias = weechat_alias; ptr_alias; ptr_alias = ptr_alias->next_alias)
    {
        if (string_strcasecmp (alias_name, ptr_alias->name) < 0)
            return ptr_alias;
    }
    return NULL;
}

/*
 * alias_insert_sorted: insert alias into sorted list
 */

void
alias_insert_sorted (struct alias *alias)
{
    struct alias *pos_alias;
    
    pos_alias = alias_find_pos (alias->name);
    
    if (weechat_alias)
    {
        if (pos_alias)
        {
            /* insert alias into the list (before alias found) */
            alias->prev_alias = pos_alias->prev_alias;
            alias->next_alias = pos_alias;
            if (pos_alias->prev_alias)
                pos_alias->prev_alias->next_alias = alias;
            else
                weechat_alias = alias;
            pos_alias->prev_alias = alias;
        }
        else
        {
            /* add alias to the end */
            alias->prev_alias = weechat_last_alias;
            alias->next_alias = NULL;
            weechat_last_alias->next_alias = alias;
            weechat_last_alias = alias;
        }
    }
    else
    {
        alias->prev_alias = NULL;
        alias->next_alias = NULL;
        weechat_alias = alias;
        weechat_last_alias = alias;
    }
}

/*
 * alias_new: create new alias and add it to alias list
 */

struct alias *
alias_new (char *name, char *command)
{
    struct alias *new_alias, *ptr_alias;

    while (name[0] == '/')
    {
	name++;
    }
    
    if (string_strcasecmp (name, "builtin") == 0)
        return NULL;
    
    ptr_alias = alias_search (name);
    if (ptr_alias)
    {
	if (ptr_alias->command)
	    free (ptr_alias->command);
	ptr_alias->command = strdup (command);
	return ptr_alias;
    }
    
    if ((new_alias = ((struct alias *) malloc (sizeof (struct alias)))))
    {
        new_alias->name = strdup (name);
        new_alias->command = (char *) malloc (strlen (command) + 1);
	new_alias->running = 0;
        if (new_alias->command)
            strcpy (new_alias->command, command);
        alias_insert_sorted (new_alias);
        return new_alias;
    }
    else
        return NULL;
}

/*
 * alias_get_final_command: get final command pointed by an alias
 */

char *
alias_get_final_command (struct alias *alias)
{
    struct alias *ptr_alias;
    char *result;
    
    if (alias->running)
    {
        gui_chat_printf (NULL,
                         _("%sError: circular reference when calling alias "
                           "\"/%s\""),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         alias->name);
        return NULL;
    }
    
    ptr_alias = alias_search ((alias->command[0] == '/') ?
                             alias->command + 1 : alias->command);
    if (ptr_alias)
    {
        alias->running = 1;
        result = alias_get_final_command (ptr_alias);
        alias->running = 0;
        return result;
    }
    return (alias->command[0] == '/') ?
        alias->command + 1 : alias->command;
}

/*
 * alias_add_word: add word to string and increment length
 *                 This function should NOT be called directly.
 */

void
alias_add_word (char **alias, int *length, char *word)
{
    int length_word;

    if (!word)
        return;
    
    length_word = strlen (word);
    if (length_word == 0)
        return;
    
    if (*alias == NULL)
    {
        *alias = (char *) malloc (length_word + 1);
        strcpy (*alias, word);
    }
    else
    {
        *alias = realloc (*alias, strlen (*alias) + length_word + 1);
        strcat (*alias, word);
    }
    *length += length_word;
}

/*
 * alias_replace_args: replace arguments ($1, $2, .. or $*) in alias arguments
 */

char *
alias_replace_args (char *alias_args, char *user_args)
{
    char **argv, *start, *pos, *res;
    int argc, length_res, args_count;
    
    argv = string_explode (user_args, " ", 0, &argc);
    
    res = NULL;
    length_res = 0;
    args_count = 0;
    start = alias_args;
    pos = start;
    while (pos && pos[0])
    {
        if ((pos[0] == '\\') && (pos[1] == '$'))
        {
            pos[0] = '\0';
            alias_add_word (&res, &length_res, start);
            alias_add_word (&res, &length_res, "$");
            pos[0] = '\\';
            start = pos + 2;
            pos = start;
        }
        else
        {
            if (pos[0] == '$')
            {
                if (pos[1] == '*')
                {
                    args_count++;
                    pos[0] = '\0';
                    alias_add_word (&res, &length_res, start);
                    alias_add_word (&res, &length_res, user_args);
                    pos[0] = '$';
                    start = pos + 2;
                    pos = start;
                }
                else
                {
                    if ((pos[1] >= '1') && (pos[1] <= '9'))
                    {
                        args_count++;
                        pos[0] = '\0';
                        alias_add_word (&res, &length_res, start);
                        if (pos[1] - '0' <= argc)
                            alias_add_word (&res, &length_res, argv[pos[1] - '1']);
                        pos[0] = '$';
                        start = pos + 2;
                        pos = start;
                    }
                    else
                        pos++;
                }
            }
            else
                pos++;
        }
    }
    
    if (start < pos)
        alias_add_word (&res, &length_res, start);
    
    if ((args_count == 0) && user_args && user_args[0])
    {
        alias_add_word (&res, &length_res, " ");
        alias_add_word (&res, &length_res, user_args);
    }
    
    if (argv)
        string_free_exploded (argv);
    
    return res;
}

/*
 * alias_replace_vars: replace special vars ($nick, $channel, $server) in a string
 *                     Note: result has to be free() after use
 */

char *
alias_replace_vars (struct t_gui_buffer *buffer, char *string)
{
    /* TODO: call protocol specific function to do this */
    (void) buffer;
    
/*    char *var_nick, *var_channel, *var_server;
    char empty_string[1] = { '\0' };
    char *res, *temp;
    
    var_nick = (server && server->nick) ? server->nick : empty_string;
    var_channel = (channel) ? channel->name : empty_string;
    var_server = (server) ? server->name : empty_string;
    
    temp = weechat_strreplace (string, "$nick", var_nick);
    if (!temp)
        return NULL;
    res = temp;
    
    temp = weechat_strreplace (res, "$channel", var_channel);
    free (res);
    if (!temp)
        return NULL;
    res = temp;
    
    temp = weechat_strreplace (res, "$server", var_server);
    free (res);
    if (!temp)
        return NULL;
    res = temp;
    
    return res;*/
    
    return strdup (string);
}

/*
 * alias_free: free an alias and reomve it from list
 */

void
alias_free (struct alias *alias)
{
    struct alias *new_weechat_alias;

    /* remove alias from list */
    if (weechat_last_alias == alias)
        weechat_last_alias = alias->prev_alias;
    if (alias->prev_alias)
    {
        (alias->prev_alias)->next_alias = alias->next_alias;
        new_weechat_alias = weechat_alias;
    }
    else
        new_weechat_alias = alias->next_alias;
    
    if (alias->next_alias)
        (alias->next_alias)->prev_alias = alias->prev_alias;

    /* free data */
    if (alias->name)
        free (alias->name);
    if (alias->command)
        free (alias->command);
    free (alias);
    weechat_alias = new_weechat_alias;
}

/*
 * alias_free_all: free all alias
 */

void
alias_free_all ()
{
    while (weechat_alias)
        alias_free (weechat_alias);
}
