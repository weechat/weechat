/*
 * wee-input.c - default input callback for buffers
 *
 * Copyright (C) 2003-2014 Sébastien Helleu <flashcode@flashtux.org>
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
#include "../gui/gui-filter.h"
#include "../gui/gui-window.h"
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
    char *command, *command_name, *pos;

    if ((!string) || (!string[0]))
        return;

    command = strdup (string);
    if (!command)
        return;

    /* ignore spaces at the end of command */
    pos = &command[strlen (command) - 1];
    if (pos[0] == ' ')
    {
        while ((pos > command) && (pos[0] == ' '))
            pos--;
        pos[1] = '\0';
    }

    /* extract command name */
    pos = strchr (command, ' ');
    command_name = (pos) ?
        string_strndup (command, pos - command) : strdup (command);
    if (!command_name)
    {
        free (command);
        return;
    }

    /* execute command */
    switch (hook_command_exec (buffer, any_plugin, plugin, command))
    {
        case HOOK_COMMAND_EXEC_OK:
            /* command hooked, OK (executed) */
            break;
        case HOOK_COMMAND_EXEC_ERROR:
            /* command hooked, error */
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("%sError with command \"%s\" (help on "
                                         "command: /help %s)"),
                                       gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                       command, command_name + 1);
            break;
        case HOOK_COMMAND_EXEC_NOT_FOUND:
            /*
             * command not found: if unknown commands are accepted by this
             * buffer, just send input text as data to buffer,
             * otherwise display error
             */
            if (buffer->input_get_unknown_commands)
            {
                input_exec_data (buffer, string);
            }
            else
            {
                gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                           _("%sError: unknown command \"%s\" "
                                             "(type /help for help)"),
                                           gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                           command_name);
            }
            break;
        case HOOK_COMMAND_EXEC_AMBIGUOUS_PLUGINS:
            /* command is ambiguous (exists for other plugins) */
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("%sError: ambiguous command \"%s\": "
                                         "it exists in many plugins and not in "
                                         "\"%s\" plugin"),
                                       gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                       command_name,
                                       plugin_get_name (plugin));
            break;
        case HOOK_COMMAND_EXEC_AMBIGUOUS_INCOMPLETE:
            /*
             * command is ambiguous (incomplete command and multiple commands
             * start with this name)
             */
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("%sError: incomplete command \"%s\" "
                                         "and multiple commands start with "
                                         "this name"),
                                       gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                       command_name);
            break;
        case HOOK_COMMAND_EXEC_RUNNING:
            /* command is running */
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("%sError: too many calls to command "
                                         "\"%s\" (looping)"),
                                       gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                       command_name);
            break;
        default:
            break;
    }
    free (command);
    free (command_name);
}

/*
 * Reads user input and sends data to buffer's callback.
 */

void
input_data (struct t_gui_buffer *buffer, const char *data)
{
    char *pos, *buf, str_buffer[128], *new_data, *buffer_full_name;
    const char *ptr_data, *ptr_data_for_buffer;
    int length, char_size, first_command;

    if (!buffer || !gui_buffer_valid (buffer)
        || !data || !data[0] || (data[0] == '\r') || (data[0] == '\n'))
    {
        return;
    }

    buffer_full_name = strdup (buffer->full_name);
    if (!buffer_full_name)
        return;

    /* execute modifier "input_text_for_buffer" */
    snprintf (str_buffer, sizeof (str_buffer),
              "0x%lx", (long unsigned int)buffer);
    new_data = hook_modifier_exec (NULL,
                                   "input_text_for_buffer",
                                   str_buffer,
                                   data);

    /* data was dropped? */
    if (new_data && !new_data[0])
        goto end;

    first_command = 1;
    ptr_data = (new_data) ? new_data : data;
    while (ptr_data && ptr_data[0])
    {
        /*
         * if the buffer pointer is not valid any more (or if it's another
         * buffer), use the current buffer for the next command
         */
        if (!first_command
            && (!gui_buffer_valid (buffer)
                || (strcmp (buffer->full_name, buffer_full_name) != 0)))
        {
            if (!gui_current_window || !gui_current_window->buffer)
                break;
            buffer = gui_current_window->buffer;
            free (buffer_full_name);
            buffer_full_name = strdup (buffer->full_name);
            if (!buffer_full_name)
                break;
        }

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

        first_command = 0;
    }

end:
    if (new_data)
        free (new_data);
    if (buffer_full_name)
        free (buffer_full_name);
}
