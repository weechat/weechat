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

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hook.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../core/wee-list.h"
#include "../plugins/plugin.h"
#include "../plugins/plugin-config.h"
#include "gui-completion.h"
#include "gui-buffer.h"
#include "gui-color.h"
#include "gui-keyboard.h"


/*
 * gui_completion_init: init completion
 */

void
gui_completion_init (struct t_gui_completion *completion,
                     struct t_gui_buffer *buffer)
{
    completion->buffer = buffer;
    completion->context = GUI_COMPLETION_NULL;
    completion->base_command = NULL;
    completion->base_command_arg = 0;
    completion->arg_is_nick = 0;
    completion->base_word = NULL;
    completion->base_word_pos = 0;
    completion->position = -1; 
    completion->args = NULL;
    completion->direction = 0;
    completion->add_space = 1;
    
    completion->completion_list = weelist_new ();
    
    completion->word_found = NULL;
    completion->position_replace = 0;
    completion->diff_size = 0;
    completion->diff_length = 0;
}

/*
 * gui_completion_free_data: free data in completion
 */

void
gui_completion_free_data (struct t_gui_completion *completion)
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

    if (completion->completion_list)
    {
        weelist_free (completion->completion_list);
        completion->completion_list = NULL;
    }
    
    if (completion->word_found)
        free (completion->word_found);
    completion->word_found = NULL;
}

/*
 * gui_completion_free: free completion
 */

void
gui_completion_free (struct t_gui_completion *completion)
{
    gui_completion_free_data (completion);
    free (completion);
}

/*
 * gui_completion_stop: stop completion (for example after 1 arg of command
 *                      with 1 arg)
 */

void
gui_completion_stop (struct t_gui_completion *completion)
{
    completion->context = GUI_COMPLETION_NULL;
    completion->position = -1;
}

/*
 * gui_completion_search_command: search command hook
 */

struct t_hook *
gui_completion_search_command (struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;
    
    for (ptr_hook = weechat_hooks; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if ((ptr_hook->type == HOOK_TYPE_COMMAND)
            && HOOK_COMMAND(ptr_hook, command)
            && HOOK_COMMAND(ptr_hook, command)[0]
            && (HOOK_COMMAND(ptr_hook, level) == 0)
            && (string_strcasecmp (HOOK_COMMAND(ptr_hook, command),
                                   completion->base_command) == 0))
            return ptr_hook;
    }
    
    /* command not found */
    return NULL;
}

/*
 * gui_completion_is_only_alphanum: return 1 if there is only alpha/num chars
 *                                  in a string
 */

int
gui_completion_is_only_alphanum (char *string)
{
    while (string[0])
    {
        if (strchr (CONFIG_STRING(config_look_nick_completion_ignore),
                    string[0]))
            return 0;
        string++;
    }
    return 1;
}

/*
 * gui_completion_strdup_alphanum: duplicate alpha/num chars in a string
 */

char *
gui_completion_strdup_alphanum (char *string)
{
    char *result, *pos;
    
    result = (char *)malloc (strlen (string) + 1);
    pos = result;
    while (string[0])
    {
        if (!strchr (CONFIG_STRING(config_look_nick_completion_ignore),
                     string[0]))
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
 * gui_completion_nickncmp: locale and case independent string comparison
 *                          with max length for nicks (alpha or digits only)
 */

int
gui_completion_nickncmp (char *base_word, char *nick, int max)
{
    char *base_word2, *nick2;
    int return_cmp;
    
    if (!CONFIG_STRING(config_look_nick_completion_ignore)
        || !CONFIG_STRING(config_look_nick_completion_ignore)[0]
        || !base_word || !nick || !base_word[0] || !nick[0]
        || (!gui_completion_is_only_alphanum (base_word)))
        return string_strncasecmp (base_word, nick, max);
    
    base_word2 = gui_completion_strdup_alphanum (base_word);
    nick2 = gui_completion_strdup_alphanum (nick);
    
    return_cmp = string_strncasecmp (base_word2, nick2, strlen (base_word2));
    
    free (base_word2);
    free (nick2);
    
    return return_cmp;
}

/*
 * gui_completion_list_add: add a word to completion word list
 */

void
gui_completion_list_add (struct t_gui_completion *completion, char *word,
                         int nick_completion, int position)
{
    if (!word || !word[0])
        return;
    
    if (!completion->base_word || !completion->base_word[0]
        || (nick_completion && (gui_completion_nickncmp (completion->base_word, word,
                                                         strlen (completion->base_word)) == 0))
        || (!nick_completion && (string_strncasecmp (completion->base_word, word,
                                                     strlen (completion->base_word)) == 0)))
    {
        weelist_add (completion->completion_list,
                     word, position);
    }
}

/*
 * gui_completion_list_add_channel: add current channel to completion list
 */

void
gui_completion_list_add_channel (struct t_gui_completion *completion)
{
    (void) completion;
    /*if (completion->channel)
        gui_completion_list_add (completion,
                                 ((t_irc_channel *)(completion->channel))->name,
                                 0, WEELIST_POS_SORT);*/
}

/*
 * gui_completion_list_add_channels: add server channels to completion list
 */

void
gui_completion_list_add_channels (struct t_gui_completion *completion)
{
    (void) completion;
    /*t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    
    if (completion->server)
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            for (ptr_channel = ptr_server->channels;
                 ptr_channel; ptr_channel = ptr_channel->next_channel)
            {
                gui_completion_list_add (completion, ptr_channel->name,
                                         0, WEELIST_POS_SORT);
            }
        }
        }*/
}

/*
 * gui_completion_list_add_filename: add filename to completion list
 */

void
gui_completion_list_add_filename (struct t_gui_completion *completion)
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
		
		gui_completion_list_add (completion, buffer,
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
 * gui_completion_list_add_command_hooks: add command hooks to completion list
 */

void
gui_completion_list_add_command_hooks (struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;
    
    for (ptr_hook = weechat_hooks; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if ((ptr_hook->type == HOOK_TYPE_COMMAND)
            && (HOOK_COMMAND(ptr_hook, command))
            && (HOOK_COMMAND(ptr_hook, command)[0]))
            gui_completion_list_add (completion,
                                     HOOK_COMMAND(ptr_hook, command),
                                     0, WEELIST_POS_SORT);
    }
}

/*
 * gui_completion_list_add_irc_cmd_recv: add IRC command (received) to
 *                                       completion list
 */

void
gui_completion_list_add_irc_cmd_recv (struct t_gui_completion *completion)
{
    (void) completion;
    /*int i;
    
    for (i = 0; irc_commands[i].name; i++)
    {
        if (irc_commands[i].recv_function)
            gui_completion_list_add(completion, irc_commands[i].name,
                                    0, WEELIST_POS_SORT);
                                    }*/
}

/*
 * gui_completion_list_add_key_cmd: add key commands/functions to completion
 *                                  list
 */

void
gui_completion_list_add_key_cmd (struct t_gui_completion *completion)
{
    int i;
    
    for (i = 0; gui_key_functions[i].function_name; i++)
    {
        gui_completion_list_add (completion, gui_key_functions[i].function_name,
                                 0, WEELIST_POS_SORT);
    }
}

/*
 * gui_completion_list_add_self_nick: add self nick on server to completion list
 */

void
gui_completion_list_add_self_nick (struct t_gui_completion *completion)
{
    if (completion->buffer->input_nick)
        gui_completion_list_add (completion,
                                 completion->buffer->input_nick,
                                 0, WEELIST_POS_SORT);
}

/*
 * gui_completion_list_add_server_nicks: add server nicks to completion list
 */

void
gui_completion_list_add_server_nicks (struct t_gui_completion *completion)
{
    (void) completion;
    /*t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
    if (completion->server)
    {
        for (ptr_server = (t_irc_server *)(completion->server); ptr_server;
             ptr_server = ptr_server->next_server)
        {
            for (ptr_channel = ptr_server->channels; ptr_channel;
                 ptr_channel = ptr_channel->next_channel)
            {
                if ((!completion->channel || (t_irc_channel *)(completion->channel) != ptr_channel)
                    && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
                {
                    for (ptr_nick = ptr_channel->nicks; ptr_nick;
                         ptr_nick = ptr_nick->next_nick)
                    {
                        gui_completion_list_add (completion, ptr_nick->nick,
                                                 1, WEELIST_POS_SORT);
                    }
                }
            }
        }
        
        // add current channel nicks at beginning
        if (completion->channel && (((t_irc_channel *)(completion->channel))->type == IRC_CHANNEL_TYPE_CHANNEL))
        {
            for (ptr_nick = ((t_irc_channel *)(completion->channel))->nicks;
                 ptr_nick; ptr_nick = ptr_nick->next_nick)
            {
                gui_completion_list_add (completion, ptr_nick->nick,
                                         1, WEELIST_POS_BEGINNING);
            }
        }
        
        // add self nick at the end
        if (completion->server)
            gui_completion_list_add (completion,
                                 ((t_irc_server *)(completion->server))->nick,
                                 1, WEELIST_POS_END);
        
        completion->arg_is_nick = 1;
    }*/
}

/*
 * gui_completion_list_add_channel_nicks: add channel nicks to completion list
 */

void
gui_completion_list_add_channel_nicks (struct t_gui_completion *completion)
{
    (void) completion;
    /*t_irc_nick *ptr_nick;
    struct t_weelist *ptr_weelist;
    
    if (completion->channel)
    {
        if (((t_irc_channel *)(completion->channel))->type == IRC_CHANNEL_TYPE_CHANNEL)
        {
            // add channel nicks
            for (ptr_nick = ((t_irc_channel *)(completion->channel))->nicks;
                 ptr_nick; ptr_nick = ptr_nick->next_nick)
            {
                gui_completion_list_add (completion, ptr_nick->nick,
                                         1, WEELIST_POS_SORT);
            }
            
            // add nicks speaking recently on this channel
            if (CONFIG_BOOLEAN(config_look_nick_completion_smart))
            {
                for (ptr_weelist = ((t_irc_channel *)(completion->channel))->nicks_speaking;
                     ptr_weelist; ptr_weelist = ptr_weelist->next_weelist)
                {
                    if (irc_nick_search ((t_irc_channel *)(completion->channel),
                                         ptr_weelist->data))
                        gui_completion_list_add (completion, ptr_weelist->data,
                                                 1, WEELIST_POS_BEGINNING);
                }
            }
            
            // add self nick at the end
            if (completion->server)
                gui_completion_list_add (completion,
                                         ((t_irc_server *)(completion->server))->nick,
                                         1, WEELIST_POS_END);
        }
        if ((((t_irc_channel *)(completion->channel))->type == IRC_CHANNEL_TYPE_PRIVATE)
            || (((t_irc_channel *)(completion->channel))->type == IRC_CHANNEL_TYPE_DCC_CHAT))
        {
            gui_completion_list_add (completion,
                                     ((t_irc_channel *)(completion->channel))->name,
                                     1, WEELIST_POS_SORT);
        }
        completion->arg_is_nick = 1;
        }*/
}

/*
 * gui_completion_list_add_channel_nicks_hosts: add channel nicks and hosts to completion list
 */

void
gui_completion_list_add_channel_nicks_hosts (struct t_gui_completion *completion)
{
    (void) completion;
    /*t_irc_nick *ptr_nick;
    char *buf;
    int length;
    
    if (completion->channel)
    {
        if (((t_irc_channel *)(completion->channel))->type == IRC_CHANNEL_TYPE_CHANNEL)
        {
            for (ptr_nick = ((t_irc_channel *)(completion->channel))->nicks;
                 ptr_nick; ptr_nick = ptr_nick->next_nick)
            {
                gui_completion_list_add (completion, ptr_nick->nick,
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
                        gui_completion_list_add (completion, buf,
                                                 1, WEELIST_POS_SORT);
                        free (buf);
                    }
                }
            }
        }
        if ((((t_irc_channel *)(completion->channel))->type == IRC_CHANNEL_TYPE_PRIVATE)
             || (((t_irc_channel *)(completion->channel))->type == IRC_CHANNEL_TYPE_PRIVATE))
        {
            gui_completion_list_add (completion,
                                     ((t_irc_channel *)(completion->channel))->name,
                                     1, WEELIST_POS_SORT);
        }
        completion->arg_is_nick = 1;
        }*/
}

/*
 * gui_completion_list_add_option: add config option to completion list
 */

void
gui_completion_list_add_option (struct t_gui_completion *completion)
{
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;

    for (ptr_section = weechat_config_file->sections; ptr_section;
         ptr_section = ptr_section->next_section)
    {
        for (ptr_option = ptr_section->options; ptr_option;
             ptr_option = ptr_option->next_option)
        {
            gui_completion_list_add (completion,
                                     ptr_option->name,
                                     0, WEELIST_POS_SORT);
        }
    }
}

/*
 * gui_completion_list_add_plugin_option: add plugin option to completion list
 */

void
gui_completion_list_add_plugin_option (struct t_gui_completion *completion)
{
    struct t_config_option *ptr_option;
    
    for (ptr_option = plugin_options; ptr_option;
         ptr_option = ptr_option->next_option)
    {
        gui_completion_list_add (completion, ptr_option->name,
                                 0, WEELIST_POS_SORT);
    }
}

/*
 * gui_completion_list_add_part: add part message to completion list
 */

void
gui_completion_list_add_part (struct t_gui_completion *completion)
{
    (void) completion;
    /*if (config_irc_default_msg_part && config_irc_default_msg_part[0])
        gui_completion_list_add (completion, config_irc_default_msg_part,
        0, WEELIST_POS_SORT);*/
}

/*
 * gui_completion_list_add_plugin: add plugin name to completion list
 */

void
gui_completion_list_add_plugin (struct t_gui_completion *completion)
{
    struct t_weechat_plugin *ptr_plugin;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        gui_completion_list_add (completion, ptr_plugin->name,
                                 0, WEELIST_POS_SORT);
    }
}

/*
 * gui_completion_list_add_quit: add quit message to completion list
 */

void
gui_completion_list_add_quit (struct t_gui_completion *completion)
{
    (void) completion;
    /*if (config_irc_default_msg_quit && config_irc_default_msg_quit[0])
        gui_completion_list_add (completion, config_irc_default_msg_quit,
        0, WEELIST_POS_SORT);*/
}

/*
 * gui_completion_list_add_server: add current server to completion list
 */

void
gui_completion_list_add_server (struct t_gui_completion *completion)
{
    (void) completion;
    /*if (completion->server)
        gui_completion_list_add (completion,
                             ((t_irc_server *)(completion->server))->name,
                             0, WEELIST_POS_SORT);*/
}

/*
 * gui_completion_list_add_servers: add all servers to completion list
 */

void
gui_completion_list_add_servers (struct t_gui_completion *completion)
{
    (void) completion;
    /*t_irc_server *ptr_server;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        gui_completion_list_add (completion, ptr_server->name,
                                 0, WEELIST_POS_SORT);
                                 }*/
}

/*
 * gui_completion_list_add_topic: add topic to completion list
 */

void
gui_completion_list_add_topic (struct t_gui_completion *completion)
{
    (void) completion;
    /*char *string;
    
    if (completion->server && completion->channel
        && ((t_irc_channel *)(completion->channel))->topic
        && ((t_irc_channel *)(completion->channel))->topic[0])
    {
        if (config_irc_colors_send)
            string = (char *)gui_color_decode_for_user_entry ((unsigned char *)((t_irc_channel *)(completion->channel))->topic);
        else
            string = (char *)gui_color_decode ((unsigned char *)((t_irc_channel *)(completion->channel))->topic, 0, 0);
        gui_completion_list_add (completion,
                                 (string) ?
                                 string : ((t_irc_channel *)(completion->channel))->topic,
                                 0, WEELIST_POS_SORT);
        if (string)
            free (string);
            }*/
}

/*
 * gui_completion_list_add_option_value: add option value to completion list
 */

void
gui_completion_list_add_option_value (struct t_gui_completion *completion)
{
    char *pos, *color_name, option_string[2048];
    struct t_config_option *ptr_option;
    
    if (completion->args)
    {
        pos = strchr (completion->args, ' ');
        if (pos)
            pos[0] = '\0';
        ptr_option = config_file_search_option (weechat_config_file,
                                                NULL,
                                                completion->args);
        if (ptr_option)
        {
            switch (ptr_option->type)
            {
                case CONFIG_OPTION_BOOLEAN:
                    if (CONFIG_BOOLEAN(ptr_option) == CONFIG_BOOLEAN_TRUE)
                        gui_completion_list_add (completion, "on",
                                                 0, WEELIST_POS_SORT);
                    else
                        gui_completion_list_add (completion, "off",
                                                 0, WEELIST_POS_SORT);
                    break;
                case CONFIG_OPTION_INTEGER:
                    if (ptr_option->string_values)
                        snprintf (option_string, sizeof (option_string) - 1,
                                  "%s",
                                  ptr_option->string_values[CONFIG_INTEGER(ptr_option)]);
                    else
                        snprintf (option_string, sizeof (option_string) - 1,
                                  "%d", CONFIG_INTEGER(ptr_option));
                    gui_completion_list_add (completion, option_string,
                                             0, WEELIST_POS_SORT);
                    break;
                case CONFIG_OPTION_STRING:
                    snprintf (option_string, sizeof (option_string) - 1,
                              "\"%s\"",
                              CONFIG_STRING(ptr_option));
                    gui_completion_list_add (completion, option_string,
                                             0, WEELIST_POS_SORT);
                    break;
                case CONFIG_OPTION_COLOR:
                    color_name = gui_color_get_name (CONFIG_INTEGER(ptr_option));
                    if (color_name)
                        gui_completion_list_add (completion,
                                                 color_name,
                                                 0, WEELIST_POS_SORT);
                    break;
            }
        }
        if (pos)
            pos[0] = ' ';
    }
}

/*
 * gui_completion_list_add_plugin_option_value: add plugin option value to completion list
 */

void
gui_completion_list_add_plugin_option_value (struct t_gui_completion *completion)
{
    char *pos;
    struct t_config_option *ptr_option;
    
    if (completion->args)
    {
        pos = strchr (completion->args, ' ');
        if (pos)
            pos[0] = '\0';
        
        ptr_option = plugin_config_search_internal (completion->args);
        if (ptr_option)
            gui_completion_list_add (completion, ptr_option->value,
                                     0, WEELIST_POS_SORT);
        
        if (pos)
            pos[0] = ' ';
    }
}

/*
 * gui_completion_list_add_weechat_cmd: add WeeChat commands to completion list
 */

void
gui_completion_list_add_weechat_cmd (struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;
    
    for (ptr_hook = weechat_hooks; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if ((ptr_hook->type == HOOK_TYPE_COMMAND)
            && !ptr_hook->plugin
            && HOOK_COMMAND(ptr_hook, command)
            && HOOK_COMMAND(ptr_hook, command)[0])
        {
            gui_completion_list_add (completion,
                                     HOOK_COMMAND(ptr_hook, command),
                                     0, WEELIST_POS_SORT);
        }
    }
}

/*
 * gui_completion_custom: custom completion by a plugin
 */

void
gui_completion_custom (struct t_gui_completion *completion,
                       char *custom_completion,
                       struct t_weechat_plugin *plugin)
{
    hook_completion_exec (plugin,
                          custom_completion,
                          completion->completion_list);
}

/*
 * gui_completion_build_list_template: build data list according to a template
 */

void
gui_completion_build_list_template (struct t_gui_completion *completion,
                                    char *template,
                                    struct t_weechat_plugin *plugin)
{
    char *word, *pos, *pos_end, *custom_completion;
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
                    weelist_add (completion->completion_list,
                                 word, WEELIST_POS_SORT);
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
                            gui_completion_stop (completion);
                            free (word);
                            return;
                            break;
                        case '*': /* repeat last completion (do nothing there) */
                            break;
                        case 'c': /* current channel */
                            gui_completion_list_add_channel (completion);
                            break;
                        case 'C': /* all channels */
                            gui_completion_list_add_channels (completion);
                            break;
                        case 'f': /* filename */
                            gui_completion_list_add_filename (completion);
                            break;
                        case 'h': /* command hooks */
                            gui_completion_list_add_command_hooks (completion);
                            break;
                        case 'k': /* key cmd/funtcions*/
                            gui_completion_list_add_key_cmd (completion);
                            break;
                        case 'm': /* self nickname */
                            gui_completion_list_add_self_nick (completion);
                            break;
                        case 'M': /* nicks of current server (all open channels) */
                            gui_completion_list_add_server_nicks (completion);
                            break;
                        case 'n': /* channel nicks */
                            gui_completion_list_add_channel_nicks (completion);
                            completion->context = GUI_COMPLETION_NICK;
                            break;
                        case 'N': /* channel nicks and hosts */
                            gui_completion_list_add_channel_nicks_hosts (completion);
                            break;
                        case 'o': /* config option */
                            gui_completion_list_add_option (completion);
                            break;
                        case 'O': /* plugin option */
                            gui_completion_list_add_plugin_option (completion);
                            break;
                        case 'p': /* part message */
                            gui_completion_list_add_part (completion);
                            break;
                        case 'P': /* plugin name */
                            gui_completion_list_add_plugin (completion);
                            break;
                        case 'q': /* quit message */
                            gui_completion_list_add_quit (completion);
                            break;
                        case 's': /* current server */
                            gui_completion_list_add_server (completion);
                            break;
                        case 'S': /* all servers */
                            gui_completion_list_add_servers (completion);
                            break;
                        case 't': /* topic */
                            gui_completion_list_add_topic (completion);
                            break;
                        case 'v': /* value of config option */
                            gui_completion_list_add_option_value (completion);
                            break;
                        case 'V': /* value of plugin option */
                            gui_completion_list_add_plugin_option_value (completion);
                            break;
                        case 'w': /* WeeChat commands */
                            gui_completion_list_add_weechat_cmd (completion);
                            break;
                        case '(': /* custom completion by a plugin */
                            pos++;
                            pos_end = strchr (pos, ')');
                            if (pos_end)
                            {
                                if (pos_end > pos)
                                {
                                    custom_completion = strndup (pos,
                                                                 pos_end - pos);
                                    if (custom_completion)
                                    {
                                        gui_completion_custom (completion,
                                                               custom_completion,
                                                               plugin);
                                        free (custom_completion);
                                    }
                                }
                                pos = pos_end + 1;
                            }
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
 * gui_completion_build_list: build data list according to command and argument #
 */

void
gui_completion_build_list (struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;
    char *template, *pos_template, *pos_space;
    int repeat_last, i, length;

    repeat_last = 0;
    
    ptr_hook = gui_completion_search_command (completion);
    if (!ptr_hook || !HOOK_COMMAND(ptr_hook, completion)
        || (strcmp (HOOK_COMMAND(ptr_hook, completion), "-") == 0))
    {
        gui_completion_stop (completion);
        return;
    }
    
    length = strlen (HOOK_COMMAND(ptr_hook, completion));
    if (length >= 2)
    {
        if (strcmp (HOOK_COMMAND(ptr_hook, completion) + length - 2,
                    "%*") == 0)
            repeat_last = 1;
    }
    
    i = 1;
    pos_template = HOOK_COMMAND(ptr_hook, completion);
    while (pos_template && pos_template[0])
    {
        pos_space = strchr (pos_template, ' ');
        if (i == completion->base_command_arg)
        {
            gui_completion_build_list_template (completion, pos_template,
                                                ptr_hook->plugin);
            return;
        }
        if (pos_space)
        {
            pos_template = pos_space;
            while (pos_template[0] == ' ')
                pos_template++;
        }
        else
            pos_template = NULL;
        i++;
    }
    if (repeat_last)
    {
        pos_space = rindex (template, ' ');
        gui_completion_build_list_template (completion,
                                            (pos_space) ?
                                            pos_space + 1 : template,
                                            ptr_hook->plugin);
    }
}

/*
 * gui_completion_find_context: find context for completion
 */

void
gui_completion_find_context (struct t_gui_completion *completion, char *data,
                             int size, int pos)
{
    int i, command, command_arg, pos_start, pos_end;
    
    /* look for context */
    gui_completion_free_data (completion);
    gui_completion_init (completion, completion->buffer);
    command = ((data[0] == '/') && (data[1] != '/')) ? 1 : 0;
    command_arg = 0;
    i = 0;
    while (i < pos)
    {
        if (data[i] == ' ')
        {
            command_arg++;
            i++;
            while ((i < pos) && (data[i] == ' ')) i++;
            if (!completion->args)
                completion->args = strdup (data + i);
        }
        else
            i++;
    }
    if (command)
    {
        if (command_arg > 0)
        {
            completion->context = GUI_COMPLETION_COMMAND_ARG;
            completion->base_command_arg = command_arg;
        }
        else
        {
            completion->context = GUI_COMPLETION_COMMAND;
            completion->base_command_arg = 0;
        }
    }
    else
        completion->context = GUI_COMPLETION_AUTO;
    
    /* look for word to complete (base word) */
    completion->base_word_pos = 0;
    completion->position_replace = pos;
    
    if (size > 0)
    {
        i = pos;
        pos_start = i;
        if (data[i] == ' ')
        {
            if ((i > 0) && (data[i-1] != ' '))
            {
                i--;
                while ((i >= 0) && (data[i] != ' '))
                    i--;
                pos_start = i + 1;
            }
        }
        else
        {
            while ((i >= 0) && (data[i] != ' '))
                i--;
            pos_start = i + 1;
        }
        i = pos;
        while ((i < size) && (data[i] != ' '))
            i++;
        pos_end = i - 1;

        if (completion->context == GUI_COMPLETION_COMMAND)
            pos_start++;
        
        completion->base_word_pos = pos_start;
        
        if (pos_start <= pos_end)
        {
            completion->position_replace = pos_start;
            completion->base_word = (char *) malloc (pos_end - pos_start + 2);
            for (i = pos_start; i <= pos_end; i++)
                completion->base_word[i - pos_start] = data[i];
            completion->base_word[pos_end - pos_start + 1] = '\0';
        }
    }
    
    if (!completion->base_word)
        completion->base_word = strdup ("");
    
    /* find command (for command argument completion only) */
    if (completion->context == GUI_COMPLETION_COMMAND_ARG)
    {
        pos_start = 0;
        while ((pos_start < size) && (data[pos_start] != '/'))
            pos_start++;
        if (data[pos_start] == '/')
        {
            pos_start++;
            pos_end = pos_start;
            while ((pos_end < size) && (data[pos_end] != ' '))
                pos_end++;
            if (data[pos_end] == ' ')
                pos_end--;
            
            completion->base_command = (char *) malloc (pos_end - pos_start + 2);
            for (i = pos_start; i <= pos_end; i++)
                completion->base_command[i - pos_start] = data[i];
            completion->base_command[pos_end - pos_start + 1] = '\0';
            gui_completion_build_list (completion);
        }
    }
    
    /* auto completion with nothing as base word is disabled,
       in order to prevent completion when pasting messages with [tab] inside */
    if ((completion->context == GUI_COMPLETION_AUTO)
        && ((!completion->base_word) || (!completion->base_word[0])))
    {
        completion->context = GUI_COMPLETION_NULL;
        return;
    }
}

/*
 * gui_completion_command: complete a command
 */

void
gui_completion_command (struct t_gui_completion *completion)
{
    int length, word_found_seen, other_completion;
    struct t_hook *ptr_hook;
    struct t_weelist_item *ptr_item, *ptr_item2;
    
    length = strlen (completion->base_word);
    word_found_seen = 0;
    other_completion = 0;
    if (!completion->completion_list->items)
    {
        for (ptr_hook = weechat_hooks; ptr_hook;
             ptr_hook = ptr_hook->next_hook)
        {
            if ((ptr_hook->type == HOOK_TYPE_COMMAND)
                && HOOK_COMMAND(ptr_hook, command)
                && HOOK_COMMAND(ptr_hook, command)[0]
                && (HOOK_COMMAND(ptr_hook, level) == 0))
            {
                gui_completion_list_add (completion,
                                         HOOK_COMMAND(ptr_hook, command),
                                         0, WEELIST_POS_SORT);
            }
        }
    }
    if (completion->direction < 0)
        ptr_item = completion->completion_list->last_item;
    else
        ptr_item = completion->completion_list->items;
    
    while (ptr_item)
    {
        if (string_strncasecmp (ptr_item->data, completion->base_word, length) == 0)
        {
            if ((!completion->word_found) || word_found_seen)
            {
                if (completion->word_found)
                    free (completion->word_found);
                completion->word_found = strdup (ptr_item->data);
                
                if (completion->direction < 0)
                    ptr_item2 = ptr_item->prev_item;
                else
                    ptr_item2 = ptr_item->next_item;
                
                while (ptr_item2)
                {
                    if (string_strncasecmp (ptr_item2->data,
                                            completion->base_word, length) == 0)
                        other_completion++;
                    
                    if (completion->direction < 0)
                        ptr_item2 = ptr_item2->prev_item;
                    else
                        ptr_item2 = ptr_item2->next_item;
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
            (string_strcasecmp (ptr_item->data, completion->word_found) == 0))
            word_found_seen = 1;
        
        if (completion->direction < 0)
            ptr_item = ptr_item->prev_item;
        else
            ptr_item = ptr_item->next_item;
    }
    if (completion->word_found)
    {
        free (completion->word_found);
        completion->word_found = NULL;
        gui_completion_command (completion);
    }
}

/*
 * gui_completion_command_arg: complete a command argument
 */

void
gui_completion_command_arg (struct t_gui_completion *completion, int nick_completion)
{
    int length, word_found_seen, other_completion;
    struct t_weelist_item *ptr_item, *ptr_item2;
    
    length = strlen (completion->base_word);
    word_found_seen = 0;
    other_completion = 0;
    if (completion->direction < 0)
        ptr_item = completion->completion_list->last_item;
    else
        ptr_item = completion->completion_list->items;
    
    while (ptr_item)
    {
        if ((nick_completion && (gui_completion_nickncmp (completion->base_word, ptr_item->data, length) == 0))
            || ((!nick_completion) && (string_strncasecmp (completion->base_word, ptr_item->data, length) == 0)))
        {
            if ((!completion->word_found) || word_found_seen)
            {
                if (completion->word_found)
                    free (completion->word_found);
                completion->word_found = strdup (ptr_item->data);
                
                if (completion->direction < 0)
                    ptr_item2 = ptr_item->prev_item;
                else
                    ptr_item2 = ptr_item->next_item;
                
                while (ptr_item2)
                {
                    if ((nick_completion
                         && (gui_completion_nickncmp (completion->base_word, ptr_item2->data, length) == 0))
                        || ((!nick_completion)
                            && (string_strncasecmp (completion->base_word, ptr_item2->data, length) == 0)))
                        other_completion++;
                    
                    if (completion->direction < 0)
                        ptr_item2 = ptr_item2->prev_item;
                    else
                        ptr_item2 = ptr_item2->next_item;
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
            (string_strcasecmp (ptr_item->data, completion->word_found) == 0))
            word_found_seen = 1;

        if (completion->direction < 0)
            ptr_item = ptr_item->prev_item;
        else
            ptr_item = ptr_item->next_item;
    }
    if (completion->word_found)
    {
        free (completion->word_found);
        completion->word_found = NULL;
        gui_completion_command_arg (completion, nick_completion);
    }
}

/*
 * gui_completion_nick: complete a nick
 */

void
gui_completion_nick (struct t_gui_completion *completion)
{
    (void) completion;
    /*int length, word_found_seen, other_completion;
    t_irc_nick *ptr_nick;
    struct t_weelist_item *ptr_item, *ptr_item2;

    if (!completion->channel)
        return;
    
    completion->context = GUI_COMPLETION_NICK;
    
    if ((((t_irc_channel *)(completion->channel))->type == IRC_CHANNEL_TYPE_PRIVATE)
        || (((t_irc_channel *)(completion->channel))->type == IRC_CHANNEL_TYPE_DCC_CHAT))
    {
        if (!(completion->completion_list->items))
        {
            weelist_add (completion->completion_list,
                         ((t_irc_channel *)(completion->channel))->name,
                         WEELIST_POS_SORT);
            weelist_add (completion->completion_list,
                         ((t_irc_server *)(completion->server))->nick,
                         WEELIST_POS_SORT);
        }
        gui_completion_command_arg (completion, 1);
        return;
    }

    // rebuild nick list for completion, with nicks speaking at beginning of list
    if ((((t_irc_channel *)(completion->channel))->nick_completion_reset)
        || (!(completion->completion_list->items)))
    {
        // empty completion list
        if (completion->completion_list)
        {
            weelist_free (completion->completion_list);
            completion->completion_list = NULL;
            completion->last_completion = NULL;
        }
        
        // add channel nicks
        for (ptr_nick = ((t_irc_channel *)(completion->channel))->nicks;
             ptr_nick; ptr_nick = ptr_nick->next_nick)
        {
            weelist_add (completion->completion_list,
                         ptr_nick->nick,
                         WEELIST_POS_SORT);
        }
        
        // add nicks speaking recently on this channel
        if (CONFIG_BOOLEAN(config_look_nick_completion_smart))
        {
            for (ptr_item = ((t_irc_channel *)(completion->channel))->nicks_speaking->items;
                 ptr_item; ptr_item = ptr_item->next_item)
            {
                if (irc_nick_search ((t_irc_channel *)(completion->channel),
                                     ptr_item->data))
                    weelist_add (completion->completion_list,
                                 ptr_item->data,
                                 WEELIST_POS_BEGINNING);
            }
        }
        
        // add self nick at the end
        if (completion->server)
            weelist_add (completion->completion_list,
                         ((t_irc_server *)(completion->server))->nick,
                         WEELIST_POS_END);
        
        ((t_irc_channel *)(completion->channel))->nick_completion_reset = 0;
    }
    
    length = strlen (completion->base_word);
    word_found_seen = 0;
    other_completion = 0;
    
    if (completion->direction < 0)
        ptr_item = completion->completion_list->last_item;
    else
        ptr_item = completion->completion_list->items;
    
    while (ptr_item)
    {
        if (gui_completion_nickncmp (completion->base_word, ptr_item->data, length) == 0)
        {
            if ((!completion->word_found) || word_found_seen)
            {
                if (completion->word_found)
                    free (completion->word_found);
                completion->word_found = strdup (ptr_item->data);
                if (CONFIG_BOOLEAN(config_look_nick_complete_first))
                {
                    completion->position = -1;
                    return;
                }

                if (completion->direction < 0)
                    ptr_item2 = ptr_item->prev_item;
                else
                    ptr_item2 = ptr_item->next_item;
                
                while (ptr_item2)
                {
                    if (gui_completion_nickncmp (completion->base_word,
                                                 ptr_item2->data,
                                                 length) == 0)
                        other_completion++;
                    
                    if (completion->direction < 0)
                        ptr_item2 = ptr_item2->prev_item;
                    else
                        ptr_item2 = ptr_item2->next_item;
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
            (string_strcasecmp (ptr_item->data, completion->word_found) == 0))
            word_found_seen = 1;
        
        if (completion->direction < 0)
            ptr_item = ptr_item->prev_item;
        else
            ptr_item = ptr_item->next_item;
    }
    if (completion->word_found)
    {
        free (completion->word_found);
        completion->word_found = NULL;
        gui_completion_nick (completion);
    }*/
}

/*
 * gui_completion_auto: auto complete: nick, filename or channel
 */

void
gui_completion_auto (struct t_gui_completion *completion)
{
    (void) completion;
    /*
    // filename completion
    if ((completion->base_word[0] == '/')
        || (completion->base_word[0] == '~'))
    {
        if (!completion->completion_list->items)
            gui_completion_list_add_filename (completion);
        gui_completion_command_arg (completion, 0);
        return;
    }
    
    // channel completion
    if (irc_channel_is_channel (completion->base_word))
    {
        if (!completion->completion_list->items)
            gui_completion_list_add_channels (completion);
        gui_completion_command_arg (completion, 0);
        return;
    }
    
    // default: nick completion (if channel)
    if (completion->channel)
        gui_completion_nick (completion);
    else
        completion->context = GUI_COMPLETION_NULL;
    */
}

/*
 * gui_completion_search: complete word according to context
 */

void
gui_completion_search (struct t_gui_completion *completion, int direction,
                       char *data, int size, int pos)
{
    char *old_word_found;
    
    completion->direction = direction;
    
    /* if new completion => look for base word */
    if (pos != completion->position)
    {
        if (completion->word_found)
            free (completion->word_found);
        completion->word_found = NULL;
        gui_completion_find_context (completion, data, size, pos);
    }
    
    /* completion */
    old_word_found = (completion->word_found) ?
        strdup (completion->word_found) : NULL;
    switch (completion->context)
    {
        case GUI_COMPLETION_NULL:
            /* should never be executed */
            return;
        case GUI_COMPLETION_NICK:
            gui_completion_nick (completion);
            break;
        case GUI_COMPLETION_COMMAND:
            gui_completion_command (completion);
            break;
        case GUI_COMPLETION_COMMAND_ARG:
            if (completion->completion_list->items)
                gui_completion_command_arg (completion, completion->arg_is_nick);
            else
            {
                completion->context = GUI_COMPLETION_AUTO;
                gui_completion_auto (completion);
            }
            break;
        case GUI_COMPLETION_AUTO:
            gui_completion_auto (completion);
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
        }
    }
    if (old_word_found)
        free (old_word_found);
}

/*
 * gui_completion_print_log: print completion list in log (usually for crash dump)
 */

void
gui_completion_print_log (struct t_gui_completion *completion)
{
    log_printf ("[completion (addr:0x%X)]", completion);
    log_printf ("  buffer . . . . . . . . : 0x%X", completion->buffer);
    log_printf ("  context. . . . . . . . : %d",   completion->context);
    log_printf ("  base_command . . . . . : '%s'", completion->base_command);
    log_printf ("  base_command_arg . . . : %d",   completion->base_command_arg);
    log_printf ("  arg_is_nick. . . . . . : %d",   completion->arg_is_nick);
    log_printf ("  base_word. . . . . . . : '%s'", completion->base_word);
    log_printf ("  base_word_pos. . . . . : %d",   completion->base_word_pos);
    log_printf ("  position . . . . . . . : %d",   completion->position);
    log_printf ("  args . . . . . . . . . : '%s'", completion->args);
    log_printf ("  direction. . . . . . . : %d",   completion->direction);
    log_printf ("  add_space. . . . . . . : %d",   completion->add_space);
    log_printf ("  completion_list. . . . : 0x%X", completion->completion_list);
    log_printf ("  word_found . . . . . . : '%s'", completion->word_found);
    log_printf ("  position_replace . . . : %d",   completion->position_replace);
    log_printf ("  diff_size. . . . . . . : %d",   completion->diff_size);
    log_printf ("  diff_length. . . . . . : %d",   completion->diff_length);
    if (completion->completion_list)
    {
        log_printf ("");
        weelist_print_log (completion->completion_list,
                           "completion list element");
    }
}
