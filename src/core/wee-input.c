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

/* wee-input.c: default callback function to read user input */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "weechat.h"
#include "wee-input.h"
#include "wee-config.h"
#include "wee-hook.h"
#include "wee-string.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../plugins/plugin.h"



/*
 * input_is_command: return 1 if line is a command, 0 otherwise
 */

int
input_is_command (char *line)
{
    char *pos_slash, *pos_space;

    if (strncmp (line, "/*", 2) == 0)
        return 0;
    
    pos_slash = strchr (line + 1, '/');
    pos_space = strchr (line + 1, ' ');
    
    return (line[0] == '/')
        && (!pos_slash || (pos_space && pos_slash > pos_space));
}

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
    int rc, argc;
    char *command, *pos, *ptr_args;
    char **argv, **argv_eol;
    
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
    
    rc = hook_command_exec (buffer, command, only_builtin);
    /*vars_replaced = alias_replace_vars (window, ptr_args);
      rc = plugin_cmd_handler_exec (window->buffer->protocol, command + 1,
      (vars_replaced) ? vars_replaced : ptr_args);
      if (vars_replaced)
      free (vars_replaced);*/
    
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
    
    switch (rc)
    {
        case 0: /* command hooked, KO */
            break;
        case 1: /* command hooked, OK (executed) */
            break;
        default: /* no command hooked */
            argv = string_explode (ptr_args, " ", 0, 0, &argc);
            argv_eol = string_explode (ptr_args, " ", 1, 0, NULL);
            
            /* should we send unknown command to IRC server? */
            /*if (cfg_irc_send_unknown_commands)
            {
                if (ptr_args)
                    unknown_command = (char *)malloc ((strlen (command + 1) + 1 + strlen (ptr_args) + 1) * sizeof (char));
                else
                    unknown_command = (char *)malloc ((strlen (command + 1) + 1) * sizeof (char));
                
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
                               "for help)"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             command + 1);
            
            string_free_exploded (argv);
            string_free_exploded (argv_eol);
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
            
            if (input_is_command (ptr_data))
            {
                /* WeeChat or plugin command */
                (void) input_exec_command (buffer, ptr_data,
                                           only_builtin);
            }
            else
            {
                if ((ptr_data[0] == '/') && (ptr_data[1] == '/'))
                    ptr_data++;

                hook_command_exec (buffer, ptr_data, 0);
                
                if (buffer->input_callback)
                {
                    (void)(buffer->input_callback) (buffer->input_callback_data,
                                                    buffer,
                                                    ptr_data);
                }
                else
                    gui_chat_printf (buffer,
                                     _("%sYou can not write text in this "
                                       "buffer"),
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
