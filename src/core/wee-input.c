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

/* wee-input.c: default callback function to read user input */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "weechat.h"
#include "wee-input.h"
#include "wee-alias.h"
#include "wee-command.h"
#include "wee-config.h"
#include "wee-hook.h"
#include "wee-string.h"
#include "../plugins/plugin.h"


/*
 * input_exec_command: executes a command (WeeChat internal or a
 *                     plugin command)
 *                     if only_builtin == 1, then try only
 *                     WeeChat commands (not plugins neither aliases)
 *                     returns: 1 if command was executed succesfully
 *                              0 if error (command not executed)
 */

int
input_exec_command (struct t_gui_buffer *buffer, char *string,
                    int only_builtin)
{
    int i, rc, argc, return_code, length1, length2;
    char *command, *pos, *ptr_args;
    char **argv, *alias_command;
    char **commands, **ptr_cmd, **ptr_next_cmd;
    char *args_replaced, *vars_replaced, *new_ptr_cmd;
    int some_args_replaced;
    struct alias *ptr_alias;
    
    if ((!string) || (!string[0]) || (string[0] != '/'))
        return 0;
    
    command = strdup (string);
    
    /* look for end of command */
    ptr_args = NULL;
    
    pos = &command[strlen (command) - 1];
    if (pos[0] == ' ')
    {
        while ((pos > command) && (pos[0] == ' '))
            pos--;
        pos[1] = '\0';
    }
    
    pos = strchr (command, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
        ptr_args = pos;
        if (!ptr_args[0])
            ptr_args = NULL;
    }

    rc = -1;
    if (!only_builtin)
    {
        rc = hook_command_exec (buffer->plugin, command + 1, ptr_args);
        /*vars_replaced = alias_replace_vars (window, ptr_args);
        rc = plugin_cmd_handler_exec (window->buffer->protocol, command + 1,
                                      (vars_replaced) ? vars_replaced : ptr_args);
        if (vars_replaced)
            free (vars_replaced);*/
    }
    
    switch (rc)
    {
        case 0: /* plugin handler KO */
            gui_chat_printf (NULL,
                             _("%sError: command \"%s\" failed"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             command + 1);
            break;
        case 1: /* plugin handler OK, executed */
            break;
        default: /* plugin handler not found */
            argv = string_explode (ptr_args, " ", 0, &argc);
            
            /* look for alias */
            if (!only_builtin)
            {
                for (ptr_alias = weechat_alias; ptr_alias;
                     ptr_alias = ptr_alias->next_alias)
                {
                    if (string_strcasecmp (ptr_alias->name, command + 1) == 0)
                    {
                        if (ptr_alias->running == 1)
                        {
                            gui_chat_printf (NULL,
                                             _("%sError: circular reference when "
                                               "calling alias \"/%s\""),
                                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                             ptr_alias->name);
                        }
                        else
                        {		
                            /* an alias can contain many commands separated by ';' */
                            commands = string_split_multi_command (ptr_alias->command,
                                                                   ';');
                            if (commands)
                            {
                                some_args_replaced = 0;
                                ptr_alias->running = 1;
                                for (ptr_cmd=commands; *ptr_cmd; ptr_cmd++)
                                {
                                    ptr_next_cmd = ptr_cmd;
                                    ptr_next_cmd++;
                                    
                                    vars_replaced = alias_replace_vars (buffer,
                                                                        *ptr_cmd);
                                    new_ptr_cmd = (vars_replaced) ? vars_replaced : *ptr_cmd;
                                    args_replaced = alias_replace_args (new_ptr_cmd,
                                                                        ptr_args);
                                    if (args_replaced)
                                    {
                                        some_args_replaced = 1;
                                        if (*ptr_cmd[0] == '/')
                                            (void) input_exec_command (buffer,
                                                                       args_replaced,
                                                                       only_builtin);
                                        else
                                        {
                                            alias_command = (char *) malloc (1 + strlen(args_replaced) + 1);
                                            if (alias_command)
                                            {
                                                strcpy (alias_command, "/");
                                                strcat (alias_command, args_replaced);
                                                (void) input_exec_command (buffer,
                                                                           alias_command,
                                                                           only_builtin);
                                                free (alias_command);
                                            }
                                        }
                                        free (args_replaced);
                                    }
                                    else
                                    {
                                        /* if alias has arguments, they are now
                                           arguments of the last command in the list (if no $1,$2,..$*) was found */
                                        if ((*ptr_next_cmd == NULL) && ptr_args && (!some_args_replaced))
                                        {
                                            length1 = strlen (new_ptr_cmd);
                                            length2 = strlen (ptr_args);
                                            
                                            alias_command = (char *) malloc ( 1 + length1 + 1 + length2 + 1);
                                            if (alias_command)
                                            {
                                                if (*ptr_cmd[0] != '/')
                                                    strcpy (alias_command, "/");
                                                else
                                                    strcpy (alias_command, "");
                                                
                                                strcat (alias_command, new_ptr_cmd);
                                                strcat (alias_command, " ");
                                                strcat (alias_command, ptr_args);
                                                
                                                (void) input_exec_command (buffer,
                                                                           alias_command,
                                                                           only_builtin);
                                                free (alias_command);
                                            }
                                        }
                                        else
                                        {
                                            if (*ptr_cmd[0] == '/')
                                                (void) input_exec_command (buffer,
                                                                           new_ptr_cmd,
                                                                           only_builtin);
                                            else
                                            {
                                                alias_command = (char *) malloc (1 + strlen (new_ptr_cmd) + 1);
                                                if (alias_command)
                                                {
                                                    strcpy (alias_command, "/");
                                                    strcat (alias_command, new_ptr_cmd);
                                                    (void) input_exec_command (buffer,
                                                                               alias_command,
                                                                               only_builtin);
                                                    free (alias_command);
                                                }
                                            }
                                        }
                                    }
                                    if (vars_replaced)
                                        free (vars_replaced);
                                }
                                ptr_alias->running = 0;
                                string_free_multi_command (commands);
                            }
                        }
                        string_free_exploded (argv);
                        free (command);
                        return 1;
                    }
                }
            }
            
            /* look for WeeChat command */
            for (i = 0; weechat_commands[i].name; i++)
            {
                if (string_strcasecmp (weechat_commands[i].name, command + 1) == 0)
                {
                    if ((argc < weechat_commands[i].min_arg)
                        || (argc > weechat_commands[i].max_arg))
                    {
                        if (weechat_commands[i].min_arg ==
                            weechat_commands[i].max_arg)
                        {
                            gui_chat_printf (NULL,
                                             NG_("%sError: wrong argument count "
                                                 "for %s command \"%s\" "
                                                 "(expected: %d arg)",
                                                 "%s%s wrong argument count "
                                                 "for %s command \"%s\" "
                                                 "(expected: %d args)",
                                                 weechat_commands[i].max_arg),
                                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                             PACKAGE_NAME,
                                             command + 1,
                                             weechat_commands[i].max_arg);
                        }
                        else
                        {
                            gui_chat_printf (NULL,
                                             NG_("%sError: wrong argument count "
                                                 "for %s command \"%s\" "
                                                 "(expected: between %d and "
                                                 "%d arg)",
                                                 "%s%s wrong argument count "
                                                 "for %s command \"%s\" "
                                                 "(expected: between %d and "
                                                 "%d args)",
                                                 weechat_commands[i].max_arg),
                                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                             PACKAGE_NAME,
                                             command + 1,
                                             weechat_commands[i].min_arg,
                                             weechat_commands[i].max_arg);
                        }
                    }
                    else
                    {
                        /*ptr_args2 = (ptr_args) ? (char *)gui_color_encode ((unsigned char *)ptr_args,
                                                                           (weechat_commands[i].conversion
                                                                           && cfg_irc_colors_send)) : NULL;*/
                        return_code = (int) (weechat_commands[i].cmd_function)
                            (buffer, ptr_args, argc, argv);
                        if (return_code < 0)
                        {
                            gui_chat_printf (NULL,
                                             _("%sError: command \"%s\" failed"),
                                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                             command + 1);
                        }
                    }
                    string_free_exploded (argv);
                    free (command);
                    return 1;
                }
            }
            
            /* should we send unknown command to IRC server? */
            /*if (cfg_irc_send_unknown_commands)
            {
                if (ptr_args)
                    unknown_command = (char *)malloc (strlen (command + 1) + 1 + strlen (ptr_args) + 1);
                else
                    unknown_command = (char *)malloc (strlen (command + 1) + 1);
                
                if (unknown_command)
                {
                    strcpy (unknown_command, command + 1);
                    if (ptr_args)
                    {
                        strcat (unknown_command, " ");
                        strcat (unknown_command, ptr_args);
                    }
                    irc_send_cmd_quote (server, channel, unknown_command);
                    free (unknown_command);
                }
            }
            else
            {
                gui_chat_printf_error (NULL,
                                       _("Error: unknown command \"%s\" (type /help for help). "
                                         "To send unknown commands to IRC server, enable option "
                                         "irc_send_unknown_commands."),
                                       command + 1);
            }*/

            gui_chat_printf (NULL,
                             _("%sError: unknown command \"%s\" (type /help "
                               "for help)."),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             command + 1);
            
            string_free_exploded (argv);
    }
    free (command);
    return 0;
}

/*
 * input_data: read user input and send data to protocol
 */

void
input_data (struct t_gui_buffer *buffer, char *data, int only_builtin)
{
    char *new_data, *ptr_data, *pos;
    
    if (!buffer || !data || !data[0] || (data[0] == '\r') || (data[0] == '\n'))
        return;

    /* TODO: modifier for input */
    /*new_data = plugin_modifier_exec (PLUGIN_MODIFIER_IRC_USER,
      "", data);*/
    new_data = strdup (data);
    
    /* no changes in new data */
    if (new_data && (strcmp (data, new_data) == 0))
    {
        free (new_data);
        new_data = NULL;
    }
    
    /* message not dropped? */
    if (!new_data || new_data[0])
    {
        /* use new data (returned by plugin) */
        ptr_data = (new_data) ? new_data : data;
        
        while (ptr_data && ptr_data[0])
        {
            pos = strchr (ptr_data, '\n');
            if (pos)
                pos[0] = '\0';
            
            if (command_is_command (ptr_data))
            {
                /* WeeChat or protocol command */
                (void) input_exec_command (buffer, ptr_data,
                                           only_builtin);
            }
            else
            {
                if ((ptr_data[0] == '/') && (ptr_data[1] == '/'))
                    ptr_data++;

                hook_command_exec (buffer->plugin, "", ptr_data);
                
                if (buffer->input_data_cb)
                {
                    (void)(buffer->input_data_cb) (buffer, ptr_data);
                }
                else
                    gui_chat_printf (buffer,
                                     _("%sYou can not write text in this "
                                       "buffer!"),
                                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            }

            if (pos)
            {
                pos[0] = '\n';
                ptr_data = pos + 1;
            }
            else
                ptr_data = NULL;
        }
    }
}
