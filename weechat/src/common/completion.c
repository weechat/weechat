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
#include "utf8.h"
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
completion_init (t_completion *completion)
{
    completion->context = COMPLETION_NULL;
    completion->base_command = NULL;
    completion->base_command_arg = 0;
    completion->position = -1;
    completion->base_word = NULL;
    completion->args = NULL;
    
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
    
    if (completion->args)
        free (completion->args);
    completion->args = NULL;
    
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
completion_build_list (t_completion *completion, void *server, void *channel)
{
    t_weelist *ptr_list;
    int i, j, length;
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    char *pos, option_name[256], *string, *string2;
    t_weechat_alias *ptr_alias;
    t_config_option *option;
    void *option_value;
    char option_string[2048];
#ifdef PLUGINS
    t_weechat_plugin *ptr_plugin;
    t_plugin_handler *ptr_handler;
#endif
    
    /* WeeChat internal commands */
    
    /* no completion for some commands */
    if ((ascii_strcasecmp (completion->base_command, "server") == 0)
        || (ascii_strcasecmp (completion->base_command, "save") == 0))
    {
        completion_stop (completion);
        return;
    }
    if ((ascii_strcasecmp (completion->base_command, "alias") == 0)
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
    if ((ascii_strcasecmp (completion->base_command, "buffer") == 0)
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
    if (ascii_strcasecmp (completion->base_command, "charset") == 0)
    {
        if (completion->base_command_arg == 1)
        {
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "decode_iso");
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "decode_utf");
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "encode");
        }
        else if (completion->base_command_arg == 2)
        {
            if (!server)
            {
                completion_stop (completion);
                return;
            }
            pos = strchr (completion->args, ' ');
            if (pos)
                pos[0] = '\0';
            string2 = NULL;
            if (ascii_strcasecmp (completion->args, "decode_iso") == 0)
            {
                config_option_list_get_value (&(((t_irc_server *)server)->charset_decode_iso),
                                              (channel) ? ((t_irc_channel *)channel)->name : "server",
                                              &string, &length);
                if (string && (length > 0))
                {
                    string2 = strdup (string);
                    string2[length] = '\0';
                }
            }
            else if (ascii_strcasecmp (completion->args, "decode_utf") == 0)
            {
                config_option_list_get_value (&(((t_irc_server *)server)->charset_decode_utf),
                                              (channel) ? ((t_irc_channel *)channel)->name : "server",
                                              &string, &length);
                if (string && (length > 0))
                {
                    string2 = strdup (string);
                    string2[length] = '\0';
                }
            }
            else if (ascii_strcasecmp (completion->args, "encode") == 0)
            {
                config_option_list_get_value (&(((t_irc_server *)server)->charset_encode),
                                              (channel) ? ((t_irc_channel *)channel)->name : "server",
                                              &string, &length);
                if (string && (length > 0))
                {
                    string2 = strdup (string);
                    string2[length] = '\0';
                }
            }
            
            if (string2)
            {
                weelist_add (&completion->completion_list,
                             &completion->last_completion,
                             string2);
                free (string2);
            }
            else
                completion_stop (completion);
            
            if (pos)
                pos[0] = ' ';
        }
        else
            completion_stop (completion);
        return;
    }
    if ((ascii_strcasecmp (completion->base_command, "clear") == 0)
        && (completion->base_command_arg == 1))
    {
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     "-all");
        return;
    }
    if ((ascii_strcasecmp (completion->base_command, "connect") == 0)
        || (ascii_strcasecmp (completion->base_command, "disconnect") == 0))
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
    if (ascii_strcasecmp (completion->base_command, "debug") == 0)
    {
        if (completion->base_command_arg == 1)
        {
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "dump");
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "windows");
        }
        else
            completion_stop (completion);
        return;
    }
    if ((ascii_strcasecmp (completion->base_command, "help") == 0)
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
#ifdef PLUGINS
        for (ptr_plugin = weechat_plugins; ptr_plugin;
             ptr_plugin = ptr_plugin->next_plugin)
        {
            for (ptr_handler = ptr_plugin->handlers;
                 ptr_handler; ptr_handler = ptr_handler->next_handler)
            {
                if (ptr_handler->type == HANDLER_COMMAND)
                    weelist_add (&completion->completion_list,
                                 &completion->last_completion,
                                 ptr_handler->command);
            }
        }
#endif
        return;
    }
    if (ascii_strcasecmp (completion->base_command, "history") == 0)
    {
        if (completion->base_command_arg == 1)
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "clear");
        else
            completion_stop (completion);
        return;
    }
    if (ascii_strcasecmp (completion->base_command, "ignore") == 0)
    {
        /* arg 1: nicks of current channel and "*" */
        if (completion->base_command_arg == 1)
        {
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "*");
            if (channel)
            {
                if (((t_irc_channel *)channel)->type == CHANNEL_TYPE_CHANNEL)
                {
                    for (ptr_nick = ((t_irc_channel *)channel)->nicks; ptr_nick;
                         ptr_nick = ptr_nick->next_nick)
                    {
                        weelist_add (&completion->completion_list,
                                     &completion->last_completion,
                                     ptr_nick->nick);
                    }
                }
                if (((t_irc_channel *)channel)->type == CHANNEL_TYPE_PRIVATE)
                {
                    weelist_add (&completion->completion_list,
                                 &completion->last_completion,
                                 ((t_irc_channel *)channel)->name);
                }
            }
            return;
        }
        
        /* arg 2: type / command and "*" */
        if (completion->base_command_arg == 2)
        {
            weelist_add(&completion->completion_list,
                        &completion->last_completion,
                        "*");
            i = 0;
            while (ignore_types[i])
            {
                weelist_add (&completion->completion_list,
                             &completion->last_completion,
                             ignore_types[i]);
                i++;
            }
            i = 0;
            while (irc_commands[i].command_name)
            {
                if (irc_commands[i].recv_function)
                    weelist_add(&completion->completion_list,
                                &completion->last_completion,
                                irc_commands[i].command_name);
                i++;
            }
            return;
        }
        
        /* arg 3: channel and "*" */
        if (completion->base_command_arg == 3)
        {
            weelist_add(&completion->completion_list,
                        &completion->last_completion,
                        "*");
            if (((t_irc_channel *)channel)->type == CHANNEL_TYPE_CHANNEL)
                weelist_add(&completion->completion_list,
                            &completion->last_completion,
                            ((t_irc_channel *)channel)->name);
            return;
        }
        
        /* arg 4: server */
        if (completion->base_command_arg == 4)
        {
            weelist_add(&completion->completion_list,
                        &completion->last_completion,
                        "*");
            if (SERVER(gui_current_window->buffer))
                weelist_add(&completion->completion_list,
                            &completion->last_completion,
                            SERVER(gui_current_window->buffer)->name);
            return;
        }
    }
    if (ascii_strcasecmp (completion->base_command, "key") == 0)
    {
        if (completion->base_command_arg == 1)
        {
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "unbind");
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "functions");
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "reset");
            return;
        }
        if (completion->base_command_arg == 2)
        {
            i = 0;
            while (gui_key_functions[i].function_name)
            {
                weelist_add (&completion->completion_list,
                             &completion->last_completion,
                             gui_key_functions[i].function_name);
                i++;
            }
            return;
        }
    }
    if ((ascii_strcasecmp (completion->base_command, "plugin") == 0)
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
                     "reload");
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     "unload");
        return;
    }
    if (ascii_strcasecmp (completion->base_command, "set") == 0)
    {
        if (completion->base_command_arg == 1)
        {
            for (i = 0; i < CONFIG_NUMBER_SECTIONS; i++)
            {
                if ((i != CONFIG_SECTION_KEYS) && (i != CONFIG_SECTION_ALIAS)
                    && (i != CONFIG_SECTION_IGNORE) && (i != CONFIG_SECTION_SERVER))
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
        }
        else if (completion->base_command_arg == 3)
        {
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
                                weelist_add (&completion->completion_list,
                                             &completion->last_completion,
                                             "on");
                            else
                                weelist_add (&completion->completion_list,
                                             &completion->last_completion,
                                             "off");
                            break;
                        case OPTION_TYPE_INT:
                            snprintf (option_string, sizeof (option_string) - 1,
                                      "%d", (option_value) ? *((int *)(option_value)) : option->default_int);
                            weelist_add (&completion->completion_list,
                                         &completion->last_completion,
                                         option_string);
                            break;
                        case OPTION_TYPE_INT_WITH_STRING:
                            weelist_add (&completion->completion_list,
                                         &completion->last_completion,
                                         (option_value) ?
                                             option->array_values[*((int *)(option_value))] :
                                             option->array_values[option->default_int]);
                            break;
                        case OPTION_TYPE_COLOR:
                            weelist_add (&completion->completion_list,
                                         &completion->last_completion,
                                         (option_value) ?
                                             gui_get_color_name (*((int *)(option_value))) :
                                             option->default_string);
                            break;
                        case OPTION_TYPE_STRING:
                            snprintf (option_string, sizeof (option_string) - 1,
                                      "\"%s\"",
                                      (option_value) ?
                                      *((char **)(option_value)) :
                                      option->default_string);
                            weelist_add (&completion->completion_list,
                                         &completion->last_completion,
                                         option_string);
                            break;
                    }
                }
                if (pos)
                    pos[0] = ' ';
            }
        }
        else
            completion_stop (completion);
        return;
    }
    if ((ascii_strcasecmp (completion->base_command, "unalias") == 0)
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
    if (ascii_strcasecmp (completion->base_command, "window") == 0)
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
                         "resize");
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "merge");
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "up");
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "down");
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "left");
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         "right");
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
    if ((ascii_strcasecmp (completion->base_command, "admin") == 0)
        || (ascii_strcasecmp (completion->base_command, "die") == 0)
        || (ascii_strcasecmp (completion->base_command, "info") == 0)
        || (ascii_strcasecmp (completion->base_command, "join") == 0)
        || (ascii_strcasecmp (completion->base_command, "links") == 0)
        || (ascii_strcasecmp (completion->base_command, "list") == 0)
        || (ascii_strcasecmp (completion->base_command, "lusers") == 0)
        || (ascii_strcasecmp (completion->base_command, "motd") == 0)
        || (ascii_strcasecmp (completion->base_command, "oper") == 0)
        || (ascii_strcasecmp (completion->base_command, "rehash") == 0)
        || (ascii_strcasecmp (completion->base_command, "restart") == 0)
        || (ascii_strcasecmp (completion->base_command, "service") == 0)
        || (ascii_strcasecmp (completion->base_command, "servlist") == 0)
        || (ascii_strcasecmp (completion->base_command, "squery") == 0)
        || (ascii_strcasecmp (completion->base_command, "squit") == 0)
        || (ascii_strcasecmp (completion->base_command, "stats") == 0)
        || (ascii_strcasecmp (completion->base_command, "summon") == 0)
        || (ascii_strcasecmp (completion->base_command, "time") == 0)
        || (ascii_strcasecmp (completion->base_command, "trace") == 0)
        || (ascii_strcasecmp (completion->base_command, "users") == 0)
        || (ascii_strcasecmp (completion->base_command, "wallops") == 0)
        || (ascii_strcasecmp (completion->base_command, "who") == 0))
    {
        completion_stop (completion);
        return;
    }
    if ((ascii_strcasecmp (completion->base_command, "away") == 0)
        && (completion->base_command_arg == 1))
    {
        if (cfg_irc_default_msg_away && cfg_irc_default_msg_away[0])
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         cfg_irc_default_msg_away);
        return;
    }
    if ((ascii_strcasecmp (completion->base_command, "ctcp") == 0)
        && (completion->base_command_arg == 2))
    {
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     "action");
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     "ping");
        weelist_add (&completion->completion_list,
                     &completion->last_completion,
                     "version");
        return;
    }
    if ((ascii_strcasecmp (completion->base_command, "dcc") == 0)
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
    if (ascii_strcasecmp (completion->base_command, "invite") == 0)
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
    if (ascii_strcasecmp (completion->base_command, "kick") == 0)
    {
        if (completion->base_command_arg != 1)
            completion_stop (completion);
        return;
    }
    if (ascii_strcasecmp (completion->base_command, "kill") == 0)
    {
        if (completion->base_command_arg != 1)
            completion_stop (completion);
        return;
    }
    if (ascii_strcasecmp (completion->base_command, "me") == 0)
    {
        completion->context = COMPLETION_NICK;
        return;
    }
    if (ascii_strcasecmp (completion->base_command, "notice") == 0)
    {
        if (completion->base_command_arg != 1)
            completion_stop (completion);
        return;
    }
    if ((ascii_strcasecmp (completion->base_command, "part") == 0)
        && (completion->base_command_arg == 1))
    {
        if (cfg_irc_default_msg_part && cfg_irc_default_msg_part[0])
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         cfg_irc_default_msg_part);
        return;
    }
    if (ascii_strcasecmp (completion->base_command, "query") == 0)
    {
        if (completion->base_command_arg != 1)
            completion_stop (completion);
        return;
    }
    if ((ascii_strcasecmp (completion->base_command, "quit") == 0)
        && (completion->base_command_arg == 1))
    {
        if (cfg_irc_default_msg_quit && cfg_irc_default_msg_quit[0])
            weelist_add (&completion->completion_list,
                         &completion->last_completion,
                         cfg_irc_default_msg_quit);
        return;
    }
    if (ascii_strcasecmp (completion->base_command, "topic") == 0)
    {
        if (completion->base_command_arg == 1)
        {
            if (!channel || !((t_irc_channel *)channel)->topic
                || !((t_irc_channel *)channel)->topic[0])
                completion_stop (completion);
            else
            {
                if (cfg_irc_colors_send)
                    string = (char *)gui_color_decode_for_user_entry ((unsigned char *)((t_irc_channel *)channel)->topic);
                else
                    string = (char *)gui_color_decode ((unsigned char *)((t_irc_channel *)channel)->topic, 0);
                string2 = channel_iconv_decode ((t_irc_server *)server,
                                                (t_irc_channel *)channel,
                                                (string) ? string : ((t_irc_channel *)channel)->topic);
                weelist_add (&completion->completion_list,
                             &completion->last_completion,
                             (string2) ? string2 : ((t_irc_channel *)channel)->topic);
                if (string)
                    free (string);
                if (string2)
                    free (string2);
            }
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
completion_find_context (t_completion *completion, void *server, void *channel,
                         char *buffer, int size, int pos)
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
            completion_build_list (completion, server, channel);
        }
    }
    
    /* nick completion with nothing as base word is disabled,
       in order to prevent completion when pasting messages with [tab] inside */
    if ((completion->context == COMPLETION_NICK)
        && ((!completion->base_word) || (!completion->base_word[0])))
    {
        completion->context = COMPLETION_NULL;
        return;
    }
    
    if (!completion->completion_list && channel &&
        (((t_irc_channel *)channel)->type == CHANNEL_TYPE_PRIVATE)
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
        if (ascii_strncasecmp (ptr_weelist->data, completion->base_word + 1, length) == 0)
        {
            if ((!completion->word_found) || word_found_seen)
            {
                completion->word_found = ptr_weelist->data;
                for (ptr_weelist2 = ptr_weelist->next_weelist; ptr_weelist2;
                     ptr_weelist2 = ptr_weelist2->next_weelist)
                {
                    if (ascii_strncasecmp (ptr_weelist2->data,
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
            (ascii_strcasecmp (ptr_weelist->data, completion->word_found) == 0))
            word_found_seen = 1;
    }
    if (completion->word_found)
    {
        completion->word_found = NULL;
        completion_command (completion);
    }
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
 * completion_command_arg: complete a command argument
 */

void
completion_command_arg (t_completion *completion, t_irc_channel *channel,
                        int nick_completion)
{
    int length, word_found_seen, other_completion;
    t_weelist *ptr_weelist, *ptr_weelist2;
    
    length = strlen (completion->base_word);
    word_found_seen = 0;
    other_completion = 0;
    for (ptr_weelist = completion->completion_list; ptr_weelist;
         ptr_weelist = ptr_weelist->next_weelist)
    {
        if ((nick_completion && (completion_nickncmp (completion->base_word, ptr_weelist->data, length) == 0))
            || ((!nick_completion) && (ascii_strncasecmp (completion->base_word, ptr_weelist->data, length) == 0)))
        {
            if ((!completion->word_found) || word_found_seen)
            {
                completion->word_found = ptr_weelist->data;
                for (ptr_weelist2 = ptr_weelist->next_weelist; ptr_weelist2;
                     ptr_weelist2 = ptr_weelist2->next_weelist)
                {
                    if ((nick_completion
                         && (completion_nickncmp (completion->base_word, ptr_weelist2->data, length) == 0))
                        || ((!nick_completion)
                            && (ascii_strncasecmp (completion->base_word, ptr_weelist2->data, length) == 0)))
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
            (ascii_strcasecmp (ptr_weelist->data, completion->word_found) == 0))
            word_found_seen = 1;
    }
    if (completion->word_found)
    {
        completion->word_found = NULL;
        completion_command_arg (completion, channel, nick_completion);
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
    
    if (((t_irc_channel *)channel)->type == CHANNEL_TYPE_PRIVATE)
    {
        completion_command_arg (completion, channel, 1);
        return;
    }
    
    length = strlen (completion->base_word);
    word_found_seen = 0;
    other_completion = 0;
    for (ptr_nick = channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
        if (completion_nickncmp (completion->base_word, ptr_nick->nick, length) == 0)
        {
            if ((!completion->word_found) || word_found_seen)
            {
                completion->word_found = ptr_nick->nick;
                for (ptr_nick2 = ptr_nick->next_nick; ptr_nick2;
                     ptr_nick2 = ptr_nick2->next_nick)
                {
                    if (completion_nickncmp (completion->base_word,
                                             ptr_nick2->nick,
                                             length) == 0)
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
            (ascii_strcasecmp (ptr_nick->nick, completion->word_found) == 0))
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
completion_search (t_completion *completion, void *server, void *channel,
                   char *buffer, int size, int pos)
{
    char *old_word_found;
    
    /* if new completion => look for base word */
    if (pos != completion->position)
    {
        completion->word_found = NULL;
        completion_find_context (completion, server, channel, buffer, size, pos);
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
                completion_command_arg (completion, (t_irc_channel *)channel, 0);
            else
                completion_nick (completion, (t_irc_channel *)channel);
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
}
