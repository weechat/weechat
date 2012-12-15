/*
 * wee-input.c - default input callback for buffers
 *
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

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
#include "wee-utf8.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../plugins/plugin.h"


/*
 * Sends data to buffer input callback.
 */

void
input_exec_data (struct t_gui_buffer *buffer, const char *data)
{
    if (buffer->input_callback)
    {
        (void)(buffer->input_callback) (buffer->input_callback_data,
                                        buffer,
                                        data);
    }
    else
        gui_chat_printf (buffer,
                         _("%sYou can not write text in this "
                           "buffer"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
}

/*
 * Executes a command.
 */

void
input_exec_command (struct t_gui_buffer *buffer,
                    int any_plugin,
                    struct t_weechat_plugin *plugin,
                    const char *string)
{
    int rc;
    char *command, *pos, *ptr_args;

    if ((!string) || (!string[0]))
        return;

    command = strdup (string);
    if (!command)
        return ;

    /* look for end of command */
    ptr_args = NULL;

    pos = &command[strlen (command) - 1];
    if (pos[0] == ' ')
    {
        while ((pos > command) && (pos[0] == ' '))
            pos--;
        pos[1] = '\0';
    }

    rc = hook_command_exec (buffer, any_plugin, plugin, command);

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
            gui_chat_printf (NULL,
                             _("%sError with command \"%s\" (try /help %s)"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             command + 1, command + 1);
            break;
        case 1: /* command hooked, OK (executed) */
            break;
        case -2: /* command is ambiguous (exists for other plugins) */
            gui_chat_printf (NULL,
                             _("%sError: ambiguous command \"%s\": it exists "
                               "in many plugins and not in \"%s\" plugin"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             command + 1,
                             plugin_get_name (plugin));
            break;
        case -3: /* command is running */
            gui_chat_printf (NULL,
                             _("%sError: too much calls to command \"%s\" "
                               "(looping)"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             command + 1);
            break;
        default: /* no command hooked */
            /*
             * if unknown commands are accepted by this buffer, just send
             * input text as data to buffer, otherwise display error
             */
            if (buffer->input_get_unknown_commands)
            {
                input_exec_data (buffer, string);
            }
            else
            {
                gui_chat_printf (NULL,
                                 _("%sError: unknown command \"%s\" (type "
                                   "/help for help)"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 command + 1);
            }
            break;
    }
    free (command);
}

/*
 * Reads user input and sends data to buffer's callback.
 */

void
input_data (struct t_gui_buffer *buffer, const char *data)
{
    char *pos, *buf, str_buffer[128], *new_data;
    const char *ptr_data, *ptr_data_for_buffer;
    int length, char_size;

    if (!buffer || !data || !data[0] || (data[0] == '\r') || (data[0] == '\n'))
        return;

    /* execute modifier "input_text_for_buffer" */
    snprintf (str_buffer, sizeof (str_buffer),
              "0x%lx", (long unsigned int)buffer);
    new_data = hook_modifier_exec (NULL,
                                   "input_text_for_buffer",
                                   str_buffer,
                                   data);

    /* data not dropped? */
    if (!new_data || new_data[0])
    {
        ptr_data = (new_data) ? new_data : data;
        while (ptr_data && ptr_data[0])
        {
            pos = strchr (ptr_data, '\n');
            if (pos)
                pos[0] = '\0';

            ptr_data_for_buffer = string_input_for_buffer (ptr_data);
            if (ptr_data_for_buffer)
            {
                /*
                 * input string is NOT a command, send it to buffer input
                 * callback
                 */
                if (string_is_command_char (ptr_data_for_buffer))
                {
                    char_size = utf8_char_size (ptr_data_for_buffer);
                    length = strlen (ptr_data_for_buffer) + char_size + 1;
                    buf = malloc (length);
                    if (buf)
                    {
                        memcpy (buf, ptr_data_for_buffer, char_size);
                        snprintf (buf + char_size, length - char_size,
                                  "%s", ptr_data_for_buffer);
                        input_exec_data (buffer, buf);
                        free (buf);
                    }
                }
                else
                    input_exec_data (buffer, ptr_data_for_buffer);
            }
            else
            {
                /* input string is a command */
                input_exec_command (buffer, 1, buffer->plugin, ptr_data);
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

    if (new_data)
        free (new_data);
}
