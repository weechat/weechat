/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* completion.c: completes words according to context (cmd/nick) */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#include "weechat.h"
#include "completion.h"
#include "alias.h"
#include "command.h"
#include "log.h"
#include "utf8.h"
#include "util.h"
#include "weelist.h"
#include "weeconfig.h"
#include "../irc/irc.h"

#ifdef PLUGINS
#include "../plugins/plugins.h"
#endif


/*
 * completion_init: init completion
 */

void
completion_init (t_completion *completion, void *server, void *channel)
{
    completion->server = server;
    completion->channel = channel;
    completion->context = COMPLETION_NULL;
    completion->base_command = NULL;
    completion->base_command_arg = 0;
    completion->arg_is_nick = 0;
    completion->base_word = NULL;
    completion->base_word_pos = 0;
    completion->position = -1; 
    completion->args = NULL;
    completion->direction = 0;
    completion->add_space = 1;
    
    completion->completion_list = NULL;
    completion->last_completion = NULL;

    completion->word_found = NULL;
    completion->position_replace = 0;
    completion->diff_size = 0;
    completion->diff_length = 0;
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
    
    if (completion->args)
        free (completion->args);
    completion->args = NULL;
    
    weelist_remove_all (&completion->completion_list,
                        &completion->last_completion);
    completion->completion_list = NULL;
    completion->last_completion = NULL;

    if (completion->word_found)
        free (completion->word_found);
    completion->word_found = NULL;
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
 * completion_get_command_infos: return completion template and max arg for command
 */

void
completion_get_command_infos (t_completion *completion,
                              char **template, int *max_arg)
{
    t_weechat_alias *ptr_alias;
    char *ptr_command, *ptr_command2, *pos;
    int i;
#ifdef PLUGINS
    t_weechat_plugin *ptr_plugin;
    t_plugin_handler *ptr_handler;
#endif
    
    *template = NULL;
    *max_arg = MAX_ARGS;
    
    ptr_alias = alias_search (completion->base_command);
    if (ptr_alias)
    {
        ptr_command = alias_get_final_command (ptr_alias);
        if (!ptr_command)
            return;
    }
    else
        ptr_command = completion->base_command;

    ptr_command2 = strdup (ptr_command);
    if (!ptr_command2)
        return;
    
    pos = strchr (ptr_command2, ' ');
    if (pos)
        pos[0] = '\0';
    
#ifdef PLUGINS
    /* look for plugin command handler */
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        for (ptr_handler = ptr_plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if ((ptr_handler->type == PLUGIN_HANDLER_COMMAND)
                && (ascii_strcasecmp (ptr_handler->command,
                                      ptr_command2) == 0))
            {
                *template = ptr_handler->completion_template;
                *max_arg = MAX_ARGS;
                free (ptr_command2);
                return;
            }
        }
    }
#endif
    
    /* look for WeeChat internal command */
    for (i = 0; weechat_commands[i].command_name; i++)
    {
        if (ascii_strcasecmp (weechat_commands[i].command_name,
                              ptr_command2) == 0)
        {
            *template = weechat_commands[i].completion_template;
            *max_arg = weechat_commands[i].max_arg;
            free (ptr_command2);
            return;
        }
    }
    
    /* look for IRC command */
    for (i = 0; irc_commands[i].command_name; i++)
    {
        if ((irc_commands[i].cmd_function_args || irc_commands[i].cmd_function_1arg)
            && (ascii_strcasecmp (irc_commands[i].command_name,
                                  ptr_command2) == 0))
        {
            *template = irc_commands[i].completion_template;
            *max_arg = irc_commands[i].max_arg;
            free (ptr_command2);
            return;
        }
    }
    
    free (ptr_command2);
    return;
}

/*
 * completion_is_only_alphanum: return 1 if there is only alpha/num chars
 *                              in a string
 */

int
completion_is_only_alphanum (char *string)
{
    while (string[0])
    {
        if (strchr (cfg_look_nick_completion_ignore, string[0]))
            return 0;
        string++;
    }
    return 1;
}

/*
 * completion_strdup_alphanum: duplicate alpha/num chars in a string
 */

char *
completion_strdup_alphanum (char *string)
{
    char *result, *pos;
    
    result = (char *)malloc (strlen (string) + 1);
    pos = result;
    while (string[0])
    {
        if (!strchr (cfg_look_nick_completion_ignore, string[0]))
        {
            pos[0] = string[0];
            pos++;
        }
        string++;
    }
    pos[0] = '\0';
    return result;
}

/*
 * completion_nickncmp: locale and case independent string comparison
 *                      with max length for nicks (alpha or digits only)
 */

int
completion_nickncmp (char *base_word, char *nick, int max)
{
    char *base_word2, *nick2;
    int return_cmp;
    
    if (!cfg_look_nick_completion_ignore
        || !cfg_look_nick_completion_ignore[0]
        || !base_word || !nick || !base_word[0] || !nick[0]
        || (!completion_is_only_alphanum (base_word)))
        return ascii_strncasecmp (base_word, nick, max);
    
    base_word2 = completion_strdup_alphanum (base_word);
    nick2 = completion_strdup_alphanum (nick);
    
    return_cmp = ascii_strncasecmp (base_word2, nick2, strlen (base_word2));
    
    free (base_word2);
    free (nick2);
    
    return return_cmp;
}

/*
 * completion_list_add: add a word to completion word list
 */

void
completion_list_add (t_completion *completion, char *word,
                     int nick_completion, int position)
{
    if (!completion->base_word || !completion->base_word[0]
        || (nick_completion && (completion_nickncmp (completion->base_word, word,
                                                            strlen (completion->base_word)) == 0))
        || (!nick_completion && (ascii_strncasecmp (completion->base_word, word,
                                                          strlen (completion->base_word)) == 0)))
    {
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     word,
                     position);
    }
}

/*
 * completion_list_add_alias: add alias to completion list
 */

void
completion_list_add_alias (t_completion *completion)
{
    t_weechat_alias *ptr_alias;
    
    for (ptr_alias = weechat_alias; ptr_alias; ptr_alias = ptr_alias->next_alias)
    {
        completion_list_add (completion, ptr_alias->alias_name,
                             0, WEELIST_POS_SORT);
    }
}

/*
 * completion_list_add_alias_cmd: add alias and comands to completion list
 */

void
completion_list_add_alias_cmd (t_completion *completion)
{
    t_weelist *ptr_list;
    
    for (ptr_list = index_commands; ptr_list; ptr_list = ptr_list->next_weelist)
    {
        completion_list_add (completion, ptr_list->data, 0, WEELIST_POS_SORT);
    }
}

/*
 * completion_list_add_channel: add current channel to completion list
 */

void
completion_list_add_channel (t_completion *completion)
{
    if (completion->channel)
        completion_list_add (completion,
                             ((t_irc_channel *)(completion->channel))->name,
                             0, WEELIST_POS_SORT);
}

/*
 * completion_list_add_server_channels: add server channels to completion list
 */

void
completion_list_add_server_channels (t_completion *completion)
{
    t_irc_channel *ptr_channel;
    
    if (completion->server)
    {
        for (ptr_channel = ((t_irc_server *)(completion->server))->channels;
             ptr_channel; ptr_channel = ptr_channel->next_channel)
        {
            completion_list_add (completion, ptr_channel->name,
                                 0, WEELIST_POS_SORT);
        }
    }
}

/*
 * completion_list_add_filename: add filename to completion list
 */

void
completion_list_add_filename (t_completion *completion)
{
    char *path_d, *path_b, *p, *d_name;
    char *real_prefix, *prefix;
    char *buffer;
    int buffer_len;
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    char home[3] = { '~', DIR_SEPARATOR_CHAR, '\0' };
    
    buffer_len = PATH_MAX;
    buffer = (char *) malloc (buffer_len * sizeof (char));
    if (!buffer)
	return;
    
    completion->add_space = 0;
    
    if ((strncmp (completion->base_word, home, 2) == 0) && getenv("HOME"))
    {
	real_prefix = strdup (getenv("HOME"));
	prefix = strdup (home);
    }
    else if ((strncmp (completion->base_word, DIR_SEPARATOR, 1) != 0)
	     || (strcmp (completion->base_word, "") == 0))
    {
	real_prefix = strdup (weechat_home);
	prefix = strdup ("");
    }
    else
    {
	real_prefix = strdup (DIR_SEPARATOR);
	prefix = strdup (DIR_SEPARATOR);
    }
	
    snprintf (buffer, buffer_len, "%s", completion->base_word + strlen (prefix));
    p = strrchr (buffer, DIR_SEPARATOR_CHAR);
    if (p)
    {
	*p = '\0';
	path_d = strdup (buffer);
	p++;
	path_b = strdup (p);
    }
    else {
	path_d = strdup ("");
	path_b = strdup (buffer);
    }
    
    sprintf (buffer, "%s%s%s", real_prefix, DIR_SEPARATOR, path_d);
    d_name = strdup (buffer);
    dp = opendir(d_name);
    if (dp != NULL)
    {
	while((entry = readdir(dp)) != NULL) 
	{	
	    if (strncmp (entry->d_name, path_b, strlen(path_b)) == 0) {
		
		if (strcmp (entry->d_name, ".") == 0 || strcmp (entry->d_name, "..") == 0)
		    continue;
		
		snprintf(buffer, buffer_len, "%s%s%s", 
			 d_name, DIR_SEPARATOR, entry->d_name);
		if (stat(buffer, &statbuf) == -1)
		    continue;
		
		snprintf(buffer, buffer_len, "%s%s%s%s%s%s",
			 prefix, 
			 ((strcmp(prefix, "") == 0)
			  || strchr(prefix, DIR_SEPARATOR_CHAR)) ? "" : DIR_SEPARATOR, 
			 path_d, 
			 strcmp(path_d, "") == 0 ? "" : DIR_SEPARATOR, 
			 entry->d_name, 
			 S_ISDIR(statbuf.st_mode) ? DIR_SEPARATOR : "");
		
		completion_list_add (completion, buffer,
                                     0, WEELIST_POS_SORT);
	    }
	}
    }
    
    free (d_name);
    free (prefix);
    free (real_prefix);
    free (path_d);
    free (path_b);
    free (buffer);
}

/*
 * completion_list_add_plugin_cmd: add plugin command handlers to completion list
 */

void
completion_list_add_plugin_cmd (t_completion *completion)
{
#ifdef PLUGINS
    t_weechat_plugin *ptr_plugin;
    t_plugin_handler *ptr_handler;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        for (ptr_handler = ptr_plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if (ptr_handler->type == PLUGIN_HANDLER_COMMAND)
                completion_list_add (completion, ptr_handler->command,
                                     0, WEELIST_POS_SORT);
        }
    }
#else
    /* make C compiler happy */
    (void) completion;
#endif
}

/*
 * completion_list_add_irc_cmd_sent: add IRC command (sent) to completion list
 */

void
completion_list_add_irc_cmd_sent (t_completion *completion)
{
    int i;
    
    for (i = 0; irc_commands[i].command_name; i++)
    {
        if (irc_commands[i].cmd_function_args || irc_commands[i].cmd_function_1arg)
            completion_list_add (completion, irc_commands[i].command_name,
                                 0, WEELIST_POS_SORT);
    }
}

/*
 * completion_list_add_irc_cmd_recv: add IRC command (received) to completion list
 */

void
completion_list_add_irc_cmd_recv (t_completion *completion)
{
    int i;
    
    for (i = 0; irc_commands[i].command_name; i++)
    {
        if (irc_commands[i].recv_function)
            completion_list_add(completion, irc_commands[i].command_name,
                                0, WEELIST_POS_SORT);
    }
}

/*
 * completion_list_add_key_cmd: add key commands/functions to completion list
 */

void
completion_list_add_key_cmd (t_completion *completion)
{
    int i;
    
    for (i = 0; gui_key_functions[i].function_name; i++)
    {
        completion_list_add (completion, gui_key_functions[i].function_name,
                             0, WEELIST_POS_SORT);
    }
}

/*
 * completion_list_add_self_nick: add self nick on server to completion list
 */

void
completion_list_add_self_nick (t_completion *completion)
{
    if (completion->server)
    {
        completion_list_add (completion,
                             ((t_irc_server *)(completion->server))->nick,
                             0, WEELIST_POS_SORT);
    }
}

/*
 * completion_list_add_channel_nicks: add channel nicks to completion list
 */

void
completion_list_add_channel_nicks (t_completion *completion)
{
    t_irc_nick *ptr_nick;
    t_weelist *ptr_weelist;
    
    if (completion->channel)
    {
        if (((t_irc_channel *)(completion->channel))->type == CHANNEL_TYPE_CHANNEL)
        {
            /* add channel nicks */
            for (ptr_nick = ((t_irc_channel *)(completion->channel))->nicks;
                 ptr_nick; ptr_nick = ptr_nick->next_nick)
            {
                completion_list_add (completion, ptr_nick->nick,
                                     1, WEELIST_POS_SORT);
            }
            /* add self nick at the end */
            if (completion->server)
                completion_list_add (completion,
                                     ((t_irc_server *)(completion->server))->nick,
                                     1, WEELIST_POS_END);
            /* add nicks speaking recently on this channel */
            if (cfg_look_nick_completion_smart)
            {
                for (ptr_weelist = ((t_irc_channel *)(completion->channel))->nicks_speaking;
                     ptr_weelist; ptr_weelist = ptr_weelist->next_weelist)
                {
                    if (nick_search ((t_irc_channel *)(completion->channel), ptr_weelist->data))
                        completion_list_add (completion, ptr_weelist->data,
                                             1, WEELIST_POS_BEGINNING);
                }
            }
        }
        if ((((t_irc_channel *)(completion->channel))->type == CHANNEL_TYPE_PRIVATE)
            || (((t_irc_channel *)(completion->channel))->type == CHANNEL_TYPE_DCC_CHAT))
        {
            completion_list_add (completion,
                                 ((t_irc_channel *)(completion->channel))->name,
                                 1, WEELIST_POS_SORT);
        }
        completion->arg_is_nick = 1;
    }
}

/*
 * completion_list_add_channel_nicks_hosts: add channel nicks and hosts to completion list
 */

void
completion_list_add_channel_nicks_hosts (t_completion *completion)
{
    t_irc_nick *ptr_nick;
    char *buf;
    int length;
    
    if (completion->channel)
    {
        if (((t_irc_channel *)(completion->channel))->type == CHANNEL_TYPE_CHANNEL)
        {
            for (ptr_nick = ((t_irc_channel *)(completion->channel))->nicks;
                 ptr_nick; ptr_nick = ptr_nick->next_nick)
            {
                completion_list_add (completion, ptr_nick->nick,
                                     1, WEELIST_POS_SORT);
                if (ptr_nick->host)
                {
                    length = strlen (ptr_nick->nick) + 1 +
                        strlen (ptr_nick->host) + 1;
                    buf = (char *) malloc (length);
                    if (buf)
                    {
                        snprintf (buf, length, "%s!%s",
                                  ptr_nick->nick, ptr_nick->host);
                        completion_list_add (completion, buf,
                                             1, WEELIST_POS_SORT);
                        free (buf);
                    }
                }
            }
        }
        if ((((t_irc_channel *)(completion->channel))->type == CHANNEL_TYPE_PRIVATE)
             || (((t_irc_channel *)(completion->channel))->type == CHANNEL_TYPE_PRIVATE))
        {
            completion_list_add (completion,
                                 ((t_irc_channel *)(completion->channel))->name,
                                 1, WEELIST_POS_SORT);
        }
        completion->arg_is_nick = 1;
    }
}

/*
 * completion_list_add_option: add config option to completion list
 */

void
completion_list_add_option (t_completion *completion)
{
    int i, j;
    t_irc_server *ptr_server;
    char option_name[256];
    
    for (i = 0; i < CONFIG_NUMBER_SECTIONS; i++)
    {
        if ((i != CONFIG_SECTION_KEYS) && (i != CONFIG_SECTION_ALIAS)
            && (i != CONFIG_SECTION_IGNORE) && (i != CONFIG_SECTION_SERVER))
        {
            for (j = 0; weechat_options[i][j].option_name; j++)
            {
                completion_list_add (completion,
                                     weechat_options[i][j].option_name,
                                     0, WEELIST_POS_SORT);
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
            completion_list_add (completion, option_name, 0, WEELIST_POS_SORT);
        }
    }
}

/*
 * completion_list_add_plugin_option: add plugin option to completion list
 */

void
completion_list_add_plugin_option (t_completion *completion)
{
#ifdef PLUGINS
    t_plugin_option *ptr_option;
    
    for (ptr_option = plugin_options; ptr_option;
         ptr_option = ptr_option->next_option)
    {
        completion_list_add (completion, ptr_option->name, 0, WEELIST_POS_SORT);
    }
#else
    /* make C compiler happy */
    (void) completion;
#endif
}

/*
 * completion_list_add_part: add part message to completion list
 */

void
completion_list_add_part (t_completion *completion)
{
    if (cfg_irc_default_msg_part && cfg_irc_default_msg_part[0])
        completion_list_add (completion, cfg_irc_default_msg_part,
                             0, WEELIST_POS_SORT);
}

/*
 * completion_list_add_plugin: add plugin name to completion list
 */

void
completion_list_add_plugin (t_completion *completion)
{
#ifdef PLUGINS
    t_weechat_plugin *ptr_plugin;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        completion_list_add (completion, ptr_plugin->name,
                             0, WEELIST_POS_SORT);
    }
#else
    /* make C compiler happy */
    (void) completion;
#endif
}

/*
 * completion_list_add_quit: add quit message to completion list
 */

void
completion_list_add_quit (t_completion *completion)
{
    if (cfg_irc_default_msg_quit && cfg_irc_default_msg_quit[0])
        completion_list_add (completion, cfg_irc_default_msg_quit,
                             0, WEELIST_POS_SORT);
}

/*
 * completion_list_add_server: add current server to completion list
 */

void
completion_list_add_server (t_completion *completion)
{
    if (completion->server)
        completion_list_add (completion,
                             ((t_irc_server *)(completion->server))->name,
                             0, WEELIST_POS_SORT);
}

/*
 * completion_list_add_servers: add all servers to completion list
 */

void
completion_list_add_servers (t_completion *completion)
{
    t_irc_server *ptr_server;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        completion_list_add (completion, ptr_server->name,
                             0, WEELIST_POS_SORT);
    }
}

/*
 * completion_list_add_topic: add topic to completion list
 */

void
completion_list_add_topic (t_completion *completion)
{
    char *string;
    
    if (completion->server && completion->channel
        && ((t_irc_channel *)(completion->channel))->topic
        && ((t_irc_channel *)(completion->channel))->topic[0])
    {
        if (cfg_irc_colors_send)
            string = (char *)gui_color_decode_for_user_entry ((unsigned char *)((t_irc_channel *)(completion->channel))->topic);
        else
            string = (char *)gui_color_decode ((unsigned char *)((t_irc_channel *)(completion->channel))->topic, 0);
        completion_list_add (completion,
                             (string) ?
                             string : ((t_irc_channel *)(completion->channel))->topic,
                             0, WEELIST_POS_SORT);
        if (string)
            free (string);
    }
}

/*
 * completion_list_add_option_value: add option value to completion list
 */

void
completion_list_add_option_value (t_completion *completion)
{
    char *pos;
    t_config_option *option;
    void *option_value;
    char option_string[2048];
    
    if (completion->args)
    {
        pos = strchr (completion->args, ' ');
        if (pos)
            pos[0] = '\0';
        option = NULL;
        option_value = NULL;
        config_option_search_option_value (completion->args, &option, &option_value);
        if (option && option_value)
        {
            switch (option->option_type)
            {
                case OPTION_TYPE_BOOLEAN:
                    if (option_value && (*((int *)(option_value))))
                        completion_list_add (completion, "on",
                                             0, WEELIST_POS_SORT);
                    else
                        completion_list_add (completion, "off",
                                             0, WEELIST_POS_SORT);
                    break;
                case OPTION_TYPE_INT:
                    snprintf (option_string, sizeof (option_string) - 1,
                              "%d", (option_value) ? *((int *)(option_value)) : option->default_int);
                    completion_list_add (completion, option_string,
                                         0, WEELIST_POS_SORT);
                    break;
                case OPTION_TYPE_INT_WITH_STRING:
                    completion_list_add (completion,
                                         (option_value) ?
                                         option->array_values[*((int *)(option_value))] :
                                         option->array_values[option->default_int],
                                         0, WEELIST_POS_SORT);
                    break;
                case OPTION_TYPE_COLOR:
                    completion_list_add (completion,
                                         (option_value) ?
                                         gui_color_get_name (*((int *)(option_value))) :
                                         option->default_string,
                                         0, WEELIST_POS_SORT);
                    break;
                case OPTION_TYPE_STRING:
                    snprintf (option_string, sizeof (option_string) - 1,
                              "\"%s\"",
                              ((option_value) && (*((char **)(option_value)))) ?
                              *((char **)(option_value)) :
                              option->default_string);
                    completion_list_add (completion, option_string,
                                         0, WEELIST_POS_SORT);
                    break;
            }
        }
        if (pos)
            pos[0] = ' ';
    }
}

/*
 * completion_list_add_plugin_option_value: add plugin option value to completion list
 */

void
completion_list_add_plugin_option_value (t_completion *completion)
{
#ifdef PLUGINS
    char *pos;
    t_plugin_option *ptr_option;
    
    if (completion->args)
    {
        pos = strchr (completion->args, ' ');
        if (pos)
            pos[0] = '\0';
        
        ptr_option = plugin_config_search_internal (completion->args);
        if (ptr_option)
            completion_list_add (completion, ptr_option->value,
                                 0, WEELIST_POS_SORT);
        
        if (pos)
            pos[0] = ' ';
    }
#else
    /* make C compiler happy */
    (void) completion;
#endif
}

/*
 * completion_list_add_weechat_cmd: add WeeChat commands to completion list
 */

void
completion_list_add_weechat_cmd (t_completion *completion)
{
    int i;
    
    for (i = 0; weechat_commands[i].command_name; i++)
    {
        completion_list_add (completion, weechat_commands[i].command_name,
                             0, WEELIST_POS_SORT);
    }
}

/*
 * completion_build_list_template: build data list according to a template
 */

void
completion_build_list_template (t_completion *completion, char *template)
{
    char *word, *pos;
    int word_offset;
    
    word = strdup (template);
    word_offset = 0;
    pos = template;
    while (pos)
    {
        switch (pos[0])
        {
            case '\0':
            case ' ':
            case '|':
                if (word_offset > 0)
                {
                    word[word_offset] = '\0';
                    weelist_add (&completion->completion_list,
                                 &completion->last_completion,
                                 word,
                                 WEELIST_POS_SORT);
                }
                word_offset = 0;
                break;
            case '%':
                pos++;
                if (pos && pos[0])
                {
                    switch (pos[0])
                    {
                        case '-': /* stop completion */
                            completion_stop (completion);
                            free (word);
                            return;
                            break;
                        case 'a': /* alias */
                            completion_list_add_alias (completion);
                            break;
                        case 'A': /* alias or any command */
                            completion_list_add_alias_cmd (completion);
                            break;
                        case 'c': /* current channel */
                            completion_list_add_channel (completion);
                            break;
                        case 'C': /* all channels */
                            completion_list_add_server_channels (completion);
                            break;
                        case 'f': /* filename */
                            completion_list_add_filename (completion);
                            break;
                        case 'h': /* plugin command handlers */
                            completion_list_add_plugin_cmd (completion);
                            break;
                        case 'i': /* IRC command (sent) */
                            completion_list_add_irc_cmd_sent (completion);
                            break;
                        case 'I': /* IRC command (received) */
                            completion_list_add_irc_cmd_recv (completion);
                            break;
                        case 'k': /* key cmd/funtcions*/
                            completion_list_add_key_cmd (completion);
                            break;
                        case 'm': /* self nickname */
                            completion_list_add_self_nick (completion);
                            break;
                        case 'n': /* channel nicks */
                            completion_list_add_channel_nicks (completion);
                            completion->context = COMPLETION_NICK;
                            break;
                        case 'N': /* channel nicks and hosts */
                            completion_list_add_channel_nicks_hosts (completion);
                            break;
                        case 'o': /* config option */
                            completion_list_add_option (completion);
                            break;
                        case 'O': /* plugin option */
                            completion_list_add_plugin_option (completion);
                            break;
                        case 'p': /* part message */
                            completion_list_add_part (completion);
                            break;
                        case 'P': /* plugin name */
                            completion_list_add_plugin (completion);
                            break;
                        case 'q': /* quit message */
                            completion_list_add_quit (completion);
                            break;
                        case 's': /* current server */
                            completion_list_add_server (completion);
                            break;
                        case 'S': /* all servers */
                            completion_list_add_servers (completion);
                            break;
                        case 't': /* topic */
                            completion_list_add_topic (completion);
                            break;
                        case 'v': /* value of config option */
                            completion_list_add_option_value (completion);
                            break;
                        case 'V': /* value of plugin option */
                            completion_list_add_plugin_option_value (completion);
                            break;
                        case 'w': /* WeeChat commands */
                            completion_list_add_weechat_cmd (completion);
                            break;
                    }
                }
                break;
            default:
                word[word_offset++] = pos[0];
        }
        /* end of argument in template? */
        if (!pos[0] || (pos[0] == ' '))
            pos = NULL;
        else
            pos++;
    }
    free (word);
}

/*
 * completion_build_list: build data list according to command and argument #
 */

void
completion_build_list (t_completion *completion)
{
    char *template, *pos_space;
    int max_arg, i;
    
    completion_get_command_infos (completion, &template, &max_arg);
    if (!template || (strcmp (template, "-") == 0) ||
        (completion->base_command_arg > max_arg))
    {
        completion_stop (completion);
        return;
    }
    i = 1;
    while (template && template[0])
    {
        pos_space = strchr (template, ' ');
        if (i == completion->base_command_arg)
        {
            completion_build_list_template (completion, template);
            return;
        }
        if (pos_space)
        {
            template = pos_space;
            while (template[0] == ' ')
                template++;
        }
        else
            template = NULL;
        i++;
    }
}

/*
 * completion_find_context: find context for completion
 */

void
completion_find_context (t_completion *completion, char *buffer, int size, int pos)
{
    int i, command, command_arg, pos_start, pos_end;
    
    /* look for context */
    completion_free (completion);
    completion_init (completion, completion->server, completion->channel);
    command = ((buffer[0] == '/') && (buffer[1] != '/')) ? 1 : 0;
    command_arg = 0;
    i = 0;
    while (i < pos)
    {
        if (buffer[i] == ' ')
        {
            command_arg++;
            i++;
            while ((i < pos) && (buffer[i] == ' ')) i++;
            if (!completion->args)
                completion->args = strdup (buffer + i);
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
        completion->context = COMPLETION_AUTO;
    
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
            completion_build_list (completion);
        }
    }
    
    /* auto completion with nothing as base word is disabled,
       in order to prevent completion when pasting messages with [tab] inside */
    if ((completion->context == COMPLETION_AUTO)
        && ((!completion->base_word) || (!completion->base_word[0])))
    {
        completion->context = COMPLETION_NULL;
        return;
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
    if (completion->direction < 0)
        ptr_weelist = last_index_command;
    else
        ptr_weelist = index_commands;
    while (ptr_weelist)
    {
        if (ascii_strncasecmp (ptr_weelist->data, completion->base_word + 1, length) == 0)
        {
            if ((!completion->word_found) || word_found_seen)
            {
                if (completion->word_found)
                    free (completion->word_found);
                completion->word_found = strdup (ptr_weelist->data);

                if (completion->direction < 0)
                    ptr_weelist2 = ptr_weelist->prev_weelist;
                else
                    ptr_weelist2 = ptr_weelist->next_weelist;
                
                while (ptr_weelist2)
                {
                    if (ascii_strncasecmp (ptr_weelist2->data,
                                           completion->base_word + 1, length) == 0)
                        other_completion++;
                    
                    if (completion->direction < 0)
                        ptr_weelist2 = ptr_weelist2->prev_weelist;
                    else
                        ptr_weelist2 = ptr_weelist2->next_weelist;
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
            (ascii_strcasecmp (ptr_weelist->data, completion->word_found) == 0))
            word_found_seen = 1;

        if (completion->direction < 0)
            ptr_weelist = ptr_weelist->prev_weelist;
        else
            ptr_weelist = ptr_weelist->next_weelist;
    }
    if (completion->word_found)
    {
        free (completion->word_found);
        completion->word_found = NULL;
        completion_command (completion);
    }
}

/*
 * completion_command_arg: complete a command argument
 */

void
completion_command_arg (t_completion *completion, int nick_completion)
{
    int length, word_found_seen, other_completion;
    t_weelist *ptr_weelist, *ptr_weelist2;
    
    length = strlen (completion->base_word);
    word_found_seen = 0;
    other_completion = 0;
    if (completion->direction < 0)
        ptr_weelist = completion->last_completion;
    else
        ptr_weelist = completion->completion_list;
    
    while (ptr_weelist)
    {
        if ((nick_completion && (completion_nickncmp (completion->base_word, ptr_weelist->data, length) == 0))
            || ((!nick_completion) && (ascii_strncasecmp (completion->base_word, ptr_weelist->data, length) == 0)))
        {
            if ((!completion->word_found) || word_found_seen)
            {
                if (completion->word_found)
                    free (completion->word_found);
                completion->word_found = strdup (ptr_weelist->data);
                
                if (completion->direction < 0)
                    ptr_weelist2 = ptr_weelist->prev_weelist;
                else
                    ptr_weelist2 = ptr_weelist->next_weelist;
                
                while (ptr_weelist2)
                {
                    if ((nick_completion
                         && (completion_nickncmp (completion->base_word, ptr_weelist2->data, length) == 0))
                        || ((!nick_completion)
                            && (ascii_strncasecmp (completion->base_word, ptr_weelist2->data, length) == 0)))
                        other_completion++;
                    
                    if (completion->direction < 0)
                        ptr_weelist2 = ptr_weelist2->prev_weelist;
                    else
                        ptr_weelist2 = ptr_weelist2->next_weelist;
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
            (ascii_strcasecmp (ptr_weelist->data, completion->word_found) == 0))
            word_found_seen = 1;

        if (completion->direction < 0)
            ptr_weelist = ptr_weelist->prev_weelist;
        else
            ptr_weelist = ptr_weelist->next_weelist;
    }
    if (completion->word_found)
    {
        free (completion->word_found);
        completion->word_found = NULL;
        completion_command_arg (completion, nick_completion);
    }
}

/*
 * completion_nick: complete a nick
 */

void
completion_nick (t_completion *completion)
{
    int length, word_found_seen, other_completion;
    t_irc_nick *ptr_nick;
    t_weelist *ptr_weelist, *ptr_weelist2;

    if (!completion->channel)
        return;
    
    completion->context = COMPLETION_NICK;
    
    if ((((t_irc_channel *)(completion->channel))->type == CHANNEL_TYPE_PRIVATE)
        || (((t_irc_channel *)(completion->channel))->type == CHANNEL_TYPE_DCC_CHAT))
    {
        if (!(completion->completion_list))
        {
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         ((t_irc_channel *)(completion->channel))->name,
                         WEELIST_POS_SORT);
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         ((t_irc_server *)(completion->server))->nick,
                         WEELIST_POS_SORT);
        }
        completion_command_arg (completion, 1);
        return;
    }

    /* rebuild nick list for completion, with nicks speaking at beginning of list */
    if ((((t_irc_channel *)(completion->channel))->nick_completion_reset)
        || (!(completion->completion_list)))
    {
        /* empty completion list */
        if (completion->completion_list)
        {
            weelist_remove_all (&(completion->completion_list),
                                &(completion->last_completion));
            completion->completion_list = NULL;
            completion->last_completion = NULL;
        }
        
        /* add channel nicks */
        for (ptr_nick = ((t_irc_channel *)(completion->channel))->nicks;
             ptr_nick; ptr_nick = ptr_nick->next_nick)
        {
            weelist_add (&(completion->completion_list),
                         &(completion->last_completion),
                         ptr_nick->nick,
                         WEELIST_POS_SORT);
        }
        /* add self nick at the end */
        if (completion->server)
            weelist_add (&(completion->completion_list),
                         &(completion->last_completion),
                         ((t_irc_server *)(completion->server))->nick,
                         WEELIST_POS_END);
        /* add nicks speaking recently on this channel */
        if (cfg_look_nick_completion_smart)
        {
            for (ptr_weelist = ((t_irc_channel *)(completion->channel))->nicks_speaking;
                 ptr_weelist; ptr_weelist = ptr_weelist->next_weelist)
            {
                if (nick_search ((t_irc_channel *)(completion->channel), ptr_weelist->data))
                    weelist_add (&(completion->completion_list),
                                 &(completion->last_completion),
                                 ptr_weelist->data,
                                 WEELIST_POS_BEGINNING);
            }
        }
        ((t_irc_channel *)(completion->channel))->nick_completion_reset = 0;
    }
    
    length = strlen (completion->base_word);
    word_found_seen = 0;
    other_completion = 0;
    
    if (completion->direction < 0)
        ptr_weelist = completion->last_completion;
    else
        ptr_weelist = completion->completion_list;
    
    while (ptr_weelist)
    {
        if (completion_nickncmp (completion->base_word, ptr_weelist->data, length) == 0)
        {
            if ((!completion->word_found) || word_found_seen)
            {
                if (completion->word_found)
                    free (completion->word_found);
                completion->word_found = strdup (ptr_weelist->data);
                if (cfg_look_nick_complete_first)
                {
                    completion->position = -1;
                    return;
                }

                if (completion->direction < 0)
                    ptr_weelist2 = ptr_weelist->prev_weelist;
                else
                    ptr_weelist2 = ptr_weelist->next_weelist;
                
                while (ptr_weelist2)
                {
                    if (completion_nickncmp (completion->base_word,
                                             ptr_weelist2->data,
                                             length) == 0)
                        other_completion++;
                    
                    if (completion->direction < 0)
                        ptr_weelist2 = ptr_weelist2->prev_weelist;
                    else
                        ptr_weelist2 = ptr_weelist2->next_weelist;
                }
                
                if (other_completion == 0)
                    completion->position = -1;
                else
                {
                    if (completion->position < 0)
                        completion->position = 0;
                }
                return;
            }
            other_completion++;
        }
        if (completion->word_found &&
            (ascii_strcasecmp (ptr_weelist->data, completion->word_found) == 0))
            word_found_seen = 1;
        
        if (completion->direction < 0)
            ptr_weelist = ptr_weelist->prev_weelist;
        else
            ptr_weelist = ptr_weelist->next_weelist;
    }
    if (completion->word_found)
    {
        free (completion->word_found);
        completion->word_found = NULL;
        completion_nick (completion);
    }
}

/*
 * completion_auto: auto complete: nick, filename or channel
 */

void
completion_auto (t_completion *completion)
{
    /* filename completion */
    if ((completion->base_word[0] == '/')
        || (completion->base_word[0] == '~'))
    {
        if (!completion->completion_list)
            completion_list_add_filename (completion);
        completion_command_arg (completion, 0);
        return;
    }
    
    /* channel completion */
    if (string_is_channel (completion->base_word))
    {
        if (!completion->completion_list)
            completion_list_add_server_channels (completion);
        completion_command_arg (completion, 0);
        return;
    }
    
    /* default: nick completion (if channel) */
    if (completion->channel)
        completion_nick (completion);
    else
        completion->context = COMPLETION_NULL;
}

/*
 * completion_search: complete word according to context
 */

void
completion_search (t_completion *completion, int direction,
                   char *buffer, int size, int pos)
{
    char *old_word_found;
    
    completion->direction = direction;
    
    /* if new completion => look for base word */
    if (pos != completion->position)
    {
        if (completion->word_found)
            free (completion->word_found);
        completion->word_found = NULL;
        completion_find_context (completion, buffer, size, pos);
    }
    
    /* completion */
    old_word_found = (completion->word_found) ?
        strdup (completion->word_found) : NULL;
    switch (completion->context)
    {
        case COMPLETION_NULL:
            /* should never be executed */
            return;
        case COMPLETION_NICK:
            completion_nick (completion);
            break;
        case COMPLETION_COMMAND:
            completion_command (completion);
            break;
        case COMPLETION_COMMAND_ARG:
            if (completion->completion_list)
                completion_command_arg (completion, completion->arg_is_nick);
            else
            {
                completion->context = COMPLETION_AUTO;
                completion_auto (completion);
            }
            break;
        case COMPLETION_AUTO:
            completion_auto (completion);
            break;
    }
    if (completion->word_found)
    {
        if (old_word_found)
        {
            completion->diff_size = strlen (completion->word_found) -
                strlen (old_word_found);
            completion->diff_length = utf8_strlen (completion->word_found) -
                utf8_strlen (old_word_found);
        }
        else
        {
            completion->diff_size = strlen (completion->word_found) -
                strlen (completion->base_word);
            completion->diff_length = utf8_strlen (completion->word_found) -
                utf8_strlen (completion->base_word);
            if (completion->context == COMPLETION_COMMAND)
            {
                completion->diff_size++;
                completion->diff_length++;
            }
        }
    }
    if (old_word_found)
        free (old_word_found);
}

/*
 * completion_print_log: print completion list in log (usually for crash dump)
 */

void
completion_print_log (t_completion *completion)
{
    weechat_log_printf ("[completion (addr:0x%X)]\n", completion);
    weechat_log_printf ("  server . . . . . . . . : 0x%X\n", completion->server);
    weechat_log_printf ("  channel. . . . . . . . : 0x%X\n", completion->channel);
    weechat_log_printf ("  context. . . . . . . . : %d\n",   completion->context);
    weechat_log_printf ("  base_command . . . . . : '%s'\n", completion->base_command);
    weechat_log_printf ("  base_command_arg . . . : %d\n",   completion->base_command_arg);
    weechat_log_printf ("  arg_is_nick. . . . . . : %d\n",   completion->arg_is_nick);
    weechat_log_printf ("  base_word. . . . . . . : '%s'\n", completion->base_word);
    weechat_log_printf ("  base_word_pos. . . . . : %d\n",   completion->base_word_pos);
    weechat_log_printf ("  position . . . . . . . : %d\n",   completion->position);
    weechat_log_printf ("  args . . . . . . . . . : '%s'\n", completion->args);
    weechat_log_printf ("  direction. . . . . . . : %d\n",   completion->direction);
    weechat_log_printf ("  add_space. . . . . . . : %d\n",   completion->add_space);
    weechat_log_printf ("  completion_list. . . . : 0x%X\n", completion->completion_list);
    weechat_log_printf ("  last_completion. . . . : 0x%X\n", completion->last_completion);
    weechat_log_printf ("  word_found . . . . . . : '%s'\n", completion->word_found);
    weechat_log_printf ("  position_replace . . . : %d\n",   completion->position_replace);
    weechat_log_printf ("  diff_size. . . . . . . : %d\n",   completion->diff_size);
    weechat_log_printf ("  diff_length. . . . . . : %d\n",   completion->diff_length);
    if (completion->completion_list)
    {
        weechat_log_printf ("\n");
        weelist_print_log (completion->completion_list,
                           "completion list element");
    }
}
