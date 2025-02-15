/*
 * core-input.c - default input callback for buffers
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "weechat.h"
#include "core-input.h"
#include "core-config.h"
#include "core-hook.h"
#include "core-string.h"
#include "core-utf8.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-filter.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"


char **input_commands_allowed = NULL;


/*
 * Sends data to buffer input callback.
 *
 * Returns the return code of buffer callback, or WEECHAT_RC_ERROR if the
 * buffer has no input callback.
 */

int
input_exec_data (struct t_gui_buffer *buffer, const char *data)
{
    if (buffer->input_callback)
    {
        return (buffer->input_callback) (buffer->input_callback_pointer,
                                         buffer->input_callback_data,
                                         buffer,
                                         data);
    }
    else
    {
        gui_chat_printf (buffer,
                         _("%sYou cannot write text in this "
                           "buffer"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
        return WEECHAT_RC_ERROR;
    }
}

/*
 * Executes a command.
 *
 * Returns:
 *   WEECHAT_RC_OK: command executed
 *   WEECHAT_RC_ERROR: error, command not executed
 */

int
input_exec_command (struct t_gui_buffer *buffer,
                    int any_plugin,
                    struct t_weechat_plugin *plugin,
                    const char *string,
                    const char *commands_allowed)
{
    char *command, *command_name, *pos;
    char **old_commands_allowed, **new_commands_allowed;
    const char *ptr_command_name;
    int rc;

    if (!string || (!string[0]))
        return WEECHAT_RC_ERROR;

    rc = WEECHAT_RC_OK;

    command = NULL;
    command_name = NULL;

    old_commands_allowed = input_commands_allowed;
    new_commands_allowed = NULL;

    command = strdup (string);
    if (!command)
    {
        rc = WEECHAT_RC_ERROR;
        goto end;
    }

    if (commands_allowed)
    {
        new_commands_allowed = string_split (
            commands_allowed,
            ",",
            NULL,
            WEECHAT_STRING_SPLIT_STRIP_LEFT
            | WEECHAT_STRING_SPLIT_STRIP_RIGHT
            | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
            0,
            NULL);
        input_commands_allowed = new_commands_allowed;
    }

    /* extract command name */
    pos = strchr (command, ' ');
    command_name = (pos) ?
        string_strndup (command, pos - command) : strdup (command);
    if (!command_name)
    {
        rc = WEECHAT_RC_ERROR;
        goto end;
    }

    ptr_command_name = utf8_next_char (command_name);

    /* check if command is allowed */
    if (input_commands_allowed
        && !string_match_list (ptr_command_name,
                               (const char **)input_commands_allowed, 1))
    {
        if (weechat_debug_core >= 1)
        {
            gui_chat_printf_date_tags (
                NULL, 0, "command_forbidden," GUI_FILTER_TAG_NO_FILTER,
                _("warning: the command \"%s\" is not currently allowed "
                  "(command: \"%s\", buffer: \"%s\")"),
                command_name,
                command,
                buffer->full_name);
        }
        rc = WEECHAT_RC_ERROR;
        goto end;
    }

    /* execute command */
    switch (hook_command_exec (buffer, any_plugin, plugin, command))
    {
        case HOOK_COMMAND_EXEC_OK:
            /* command hooked, OK (executed) */
            break;
        case HOOK_COMMAND_EXEC_ERROR:
            /* command hooked, error */
            rc = WEECHAT_RC_ERROR;
            break;
        case HOOK_COMMAND_EXEC_NOT_FOUND:
            /*
             * command not found: if unknown commands are accepted by this
             * buffer, just send input text as data to buffer,
             * otherwise display error
             */
            if (buffer->input_get_unknown_commands)
            {
                rc = input_exec_data (buffer, string);
            }
            else
            {
                hook_command_display_error_unknown (ptr_command_name);
                rc = WEECHAT_RC_ERROR;
            }
            break;
        case HOOK_COMMAND_EXEC_AMBIGUOUS_PLUGINS:
            /* command is ambiguous (exists for other plugins) */
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("%sAmbiguous command \"%s\": "
                                         "it exists in many plugins and not in "
                                         "\"%s\" plugin"),
                                       gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                       command_name,
                                       plugin_get_name (plugin));
            rc = WEECHAT_RC_ERROR;
            break;
        case HOOK_COMMAND_EXEC_AMBIGUOUS_INCOMPLETE:
            /*
             * command is ambiguous (incomplete command and multiple commands
             * start with this name)
             */
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("%sIncomplete command \"%s\" "
                                         "and multiple commands start with "
                                         "this name"),
                                       gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                       command_name);
            rc = WEECHAT_RC_ERROR;
            break;
        case HOOK_COMMAND_EXEC_RUNNING:
            /* command is running */
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("%sToo many calls to command "
                                         "\"%s\" (looping)"),
                                       gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                       command_name);
            rc = WEECHAT_RC_ERROR;
            break;
        default:
            break;
    }

end:
    free (command);
    free (command_name);

    string_free_split (new_commands_allowed);
    input_commands_allowed = old_commands_allowed;

    return rc;
}

/*
 * Sends data to a buffer's callback.
 *
 * If split_newline = 1 and if buffer input_multiline = 0, the string
 * is split on "\n" and multiple commands can then be executed.
 *
 * Returns:
 *   WEECHAT_RC_OK: data properly sent (or command executed successfully)
 *   WEECHAT_RC_ERROR: error
 */

int
input_data (struct t_gui_buffer *buffer, const char *data,
            const char *commands_allowed, int split_newline, int user_data)
{
    char *pos, str_buffer[128], *new_data, *buffer_full_name;
    const char *ptr_data, *ptr_data_for_buffer;
    int first_command, rc;

    if (!buffer || !gui_buffer_valid (buffer) || !data)
        return WEECHAT_RC_ERROR;

    rc = WEECHAT_RC_OK;

    buffer_full_name = NULL;
    new_data = NULL;

    buffer_full_name = strdup (buffer->full_name);
    if (!buffer_full_name)
    {
        rc = WEECHAT_RC_ERROR;
        goto end;
    }

    /* execute modifier "input_text_for_buffer" */
    snprintf (str_buffer, sizeof (str_buffer), "0x%lx", (unsigned long)buffer);
    new_data = hook_modifier_exec (NULL,
                                   "input_text_for_buffer",
                                   str_buffer,
                                   data);

    /* data was dropped? */
    if (data[0] && new_data && !new_data[0])
        goto end;

    first_command = 1;
    ptr_data = (new_data) ? new_data : data;
    while (ptr_data)
    {
        /*
         * if the buffer pointer is not valid anymore (or if it's another
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

        pos = (buffer->input_multiline) ? NULL : strchr (ptr_data, '\n');

        if (pos)
            pos[0] = '\0';

        ptr_data_for_buffer = string_input_for_buffer (ptr_data);
        if (ptr_data_for_buffer)
        {
            /*
             * input string is NOT a command, send it as-is to the buffer
             * input callback
             */
            rc = input_exec_data (buffer, ptr_data);
        }
        else
        {
            /* input string is a command */
            if (user_data && buffer->input_get_any_user_data)
            {
                /*
                 * if data is sent from user and buffer catches any user data:
                 * send it to callback
                 */
                rc = input_exec_data (buffer, ptr_data);
            }
            else
            {
                /*
                 * if commands_allowed has special value "-", send data as-is
                 * to the buffer input callback, otherwise execute the command
                 * on the buffer
                 */
                if (commands_allowed && (strcmp (commands_allowed, "-") == 0))
                {
                    rc = input_exec_data (buffer, ptr_data);
                }
                else
                {
                    rc = input_exec_command (buffer, 1, buffer->plugin,
                                             ptr_data, commands_allowed);
                }
            }
        }

        if (pos)
        {
            pos[0] = '\n';
            ptr_data = (split_newline) ? pos + 1 : NULL;
        }
        else
            ptr_data = NULL;

        first_command = 0;
    }

end:
    free (buffer_full_name);
    free (new_data);

    return rc;
}

/*
 * Callback for timer set by input_data_delayed.
 */

int
input_data_timer_cb (const void *pointer, void *data, int remaining_calls)
{
    char **timer_args;
    int i;
    struct t_gui_buffer *ptr_buffer;

    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    timer_args = (char **)pointer;

    if (!timer_args)
        return WEECHAT_RC_ERROR;

    if (timer_args[0] && timer_args[1])
    {
        ptr_buffer = gui_buffer_search_by_full_name (timer_args[0]);
        if (ptr_buffer)
        {
            (void) input_data (
                ptr_buffer,
                timer_args[1],
                timer_args[2],
                (string_strcmp (timer_args[3], "1") == 0) ? 1 : 0,
                0);  /* user_data */
        }
    }

    for (i = 0; i < 4; i++)
    {
        free (timer_args[i]);
    }
    free (timer_args);

    return WEECHAT_RC_OK;
}

/*
 * Sends data to a buffer's callback with an optional delay (in milliseconds).
 *
 * If delay < 1, the command is executed immediately.
 * If delay >= 1, the command is scheduled for execution in this number of ms.
 *
 * Returns:
 *   WEECHAT_RC_OK: data properly sent or scheduled for execution
 *   WEECHAT_RC_ERROR: error
 */

int
input_data_delayed (struct t_gui_buffer *buffer, const char *data,
                    const char *commands_allowed, int split_newline,
                    long delay)
{
    char **timer_args, *new_commands_allowed;

    if (delay < 1)
        return input_data (buffer, data, commands_allowed, split_newline, 0);

    timer_args = malloc (4 * sizeof (*timer_args));
    if (!timer_args)
    {
        gui_chat_printf (NULL,
                         _("%sNot enough memory (%s)"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "/wait");
        return WEECHAT_RC_ERROR;
    }

    if (commands_allowed)
    {
        new_commands_allowed = strdup (commands_allowed);
    }
    else if (input_commands_allowed)
    {
        new_commands_allowed = string_rebuild_split_string (
            (const char **)input_commands_allowed, ",", 0, -1);
    }
    else
    {
        new_commands_allowed = NULL;
    }

    timer_args[0] = strdup (buffer->full_name);
    timer_args[1] = strdup (data);
    timer_args[2] = new_commands_allowed;
    timer_args[3] = strdup ((split_newline) ? "1" : "0");

    /* schedule command, execute it after "delay" milliseconds */
    hook_timer (NULL,
                (delay >= 1) ? delay : 1,
                0,
                1,
                &input_data_timer_cb,
                timer_args, NULL);

    return WEECHAT_RC_OK;
}
