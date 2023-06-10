/*
 * trigger-command.c - trigger command
 *
 * Copyright (C) 2014-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "trigger.h"
#include "trigger-buffer.h"
#include "trigger-config.h"


/*
 * Displays the status of triggers (global status).
 */

void
trigger_command_display_status ()
{
    weechat_printf_date_tags (NULL, 0, "no_trigger",
                              (trigger_enabled) ?
                              _("Triggers enabled") : _("Triggers disabled"));
}

/*
 * Displays one trigger (internal function, must not be called directly).
 */

void
trigger_command_display_trigger_internal (const char *name,
                                          int enabled,
                                          const char *hook,
                                          const char *arguments,
                                          const char *conditions,
                                          int hooks_count,
                                          int hook_count_cb,
                                          int hook_count_cmd,
                                          int regex_count,
                                          struct t_trigger_regex *regex,
                                          int commands_count,
                                          char **commands,
                                          int return_code,
                                          int post_action,
                                          int verbose)
{
    char str_conditions[64], str_regex[64], str_command[64], str_rc[64];
    char str_post_action[64], spaces[256];
    int i, length;

    if (verbose >= 1)
    {
        weechat_printf_date_tags (
            NULL, 0, "no_trigger",
            _("  %s%s%s: %s%s%s%s%s%s%s"),
            weechat_color ((enabled) ? "chat_status_enabled" : "chat_status_disabled"),
            name,
            weechat_color ("reset"),
            hook,
            weechat_color ("chat_delimiters"),
            (arguments && arguments[0]) ? "(" : "",
            weechat_color ("reset"),
            arguments,
            weechat_color ("chat_delimiters"),
            (arguments && arguments[0]) ? ")" : "");
        length = weechat_strlen_screen (name) + 3;
        if (length >= (int)sizeof (spaces))
            length = sizeof (spaces) - 1;
        memset (spaces, ' ', length);
        spaces[length] = '\0';
        if (verbose >= 2)
        {
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      "%s hooks: %d", spaces, hooks_count);
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      "%s callback: %d",
                                      spaces, hook_count_cb);
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      "%s commands: %d",
                                      spaces, hook_count_cmd);
        }
        if (conditions && conditions[0])
        {
            weechat_printf_date_tags (
                NULL, 0, "no_trigger",
                "%s %s=? %s\"%s%s%s\"",
                spaces,
                weechat_color (weechat_config_string (trigger_config_color_flag_conditions)),
                weechat_color ("chat_delimiters"),
                weechat_color ("reset"),
                conditions,
                weechat_color ("chat_delimiters"));
        }
        for (i = 0; i < regex_count; i++)
        {
            weechat_printf_date_tags (
                NULL, 0, "no_trigger",
                "%s %s~%d %s\"%s%s%s\" --> "
                "\"%s%s%s\"%s%s%s%s",
                spaces,
                weechat_color (weechat_config_string (trigger_config_color_flag_regex)),
                i + 1,
                weechat_color ("chat_delimiters"),
                weechat_color (weechat_config_string (trigger_config_color_regex)),
                regex[i].str_regex,
                weechat_color ("chat_delimiters"),
                weechat_color (weechat_config_string (trigger_config_color_replace)),
                regex[i].replace,
                weechat_color ("chat_delimiters"),
                weechat_color ("reset"),
                (regex[i].variable) ? " (" : "",
                (regex[i].variable) ? regex[i].variable : "",
                (regex[i].variable) ? ")" : "");
        }
        if (commands)
        {
            for (i = 0; commands[i]; i++)
            {
                weechat_printf_date_tags (
                    NULL, 0, "no_trigger",
                    "%s %s/%d %s\"%s%s%s\"",
                    spaces,
                    weechat_color (weechat_config_string (trigger_config_color_flag_command)),
                    i + 1,
                    weechat_color ("chat_delimiters"),
                    weechat_color ("reset"),
                    commands[i],
                    weechat_color ("chat_delimiters"));
            }
        }
        if ((return_code >= 0) && (return_code != TRIGGER_RC_OK))
        {
            weechat_printf_date_tags (
                NULL, 0, "no_trigger",
                "%s %s=> %s%s",
                spaces,
                weechat_color (weechat_config_string (trigger_config_color_flag_return_code)),
                weechat_color ("reset"),
                trigger_return_code_string[return_code]);
        }
        if ((post_action >= 0) && (post_action != TRIGGER_POST_ACTION_NONE))
        {
            weechat_printf_date_tags (
                NULL, 0, "no_trigger",
                "%s %s=1 %s%s",
                spaces,
                weechat_color (weechat_config_string (trigger_config_color_flag_post_action)),
                weechat_color ("reset"),
                trigger_post_action_string[post_action]);
        }
    }
    else
    {
        str_conditions[0] ='\0';
        str_regex[0] = '\0';
        str_command[0] = '\0';
        str_rc[0] = '\0';
        str_post_action[0] = '\0';
        if (conditions && conditions[0])
        {
            snprintf (str_conditions, sizeof (str_conditions),
                      " %s=?%s",
                      weechat_color (weechat_config_string (trigger_config_color_flag_conditions)),
                      weechat_color ("reset"));
        }
        if (regex_count > 0)
        {
            snprintf (str_regex, sizeof (str_regex),
                      " %s~%d%s",
                      weechat_color (weechat_config_string (trigger_config_color_flag_regex)),
                      regex_count,
                      weechat_color ("reset"));
        }
        if (commands_count > 0)
        {
            snprintf (str_command, sizeof (str_command),
                      " %s/%d%s",
                      weechat_color (weechat_config_string (trigger_config_color_flag_command)),
                      commands_count,
                      weechat_color ("reset"));
        }
        if ((return_code >= 0) && (return_code != TRIGGER_RC_OK))
        {
            snprintf (str_rc, sizeof (str_rc),
                      " %s=>%s",
                      weechat_color (weechat_config_string (trigger_config_color_flag_return_code)),
                      weechat_color ("reset"));
        }
        if ((post_action >= 0) && (post_action != TRIGGER_POST_ACTION_NONE))
        {
            snprintf (str_post_action, sizeof (str_post_action),
                      " %s=1%s",
                      weechat_color (weechat_config_string (trigger_config_color_flag_post_action)),
                      weechat_color ("reset"));
        }
        weechat_printf_date_tags (
            NULL, 0, "no_trigger",
            _("  %s%s%s: %s%s%s%s%s%s%s%s%s%s%s%s%s"),
            weechat_color ((enabled) ? "chat_status_enabled": "chat_status_disabled"),
            name,
            weechat_color ("reset"),
            hook,
            weechat_color ("chat_delimiters"),
            (arguments && arguments[0]) ? "(" : "",
            weechat_color ("reset"),
            arguments,
            weechat_color ("chat_delimiters"),
            (arguments && arguments[0]) ? ")" : "",
            weechat_color ("reset"),
            str_conditions,
            str_regex,
            str_command,
            str_rc,
            str_post_action);
    }
}

/*
 * Displays one trigger.
 */

void
trigger_command_display_trigger (struct t_trigger *trigger, int verbose)
{
    trigger_command_display_trigger_internal (
        trigger->name,
        weechat_config_boolean (trigger->options[TRIGGER_OPTION_ENABLED]),
        weechat_config_string (trigger->options[TRIGGER_OPTION_HOOK]),
        weechat_config_string (trigger->options[TRIGGER_OPTION_ARGUMENTS]),
        weechat_config_string (trigger->options[TRIGGER_OPTION_CONDITIONS]),
        trigger->hooks_count,
        trigger->hook_count_cb,
        trigger->hook_count_cmd,
        trigger->regex_count,
        trigger->regex,
        trigger->commands_count,
        trigger->commands,
        weechat_config_integer (trigger->options[TRIGGER_OPTION_RETURN_CODE]),
        weechat_config_integer (trigger->options[TRIGGER_OPTION_POST_ACTION]),
        verbose);
}

/*
 * Displays a list of triggers.
 */

void
trigger_command_list (const char *message, int verbose)
{
    struct t_trigger *ptr_trigger;

    weechat_printf_date_tags (NULL, 0, "no_trigger", "");
    trigger_command_display_status ();

    if (!triggers)
    {
        weechat_printf_date_tags (NULL, 0, "no_trigger",
                                  _("No trigger defined"));
        return;
    }

    weechat_printf_date_tags (NULL, 0, "no_trigger", message);

    for (ptr_trigger = triggers; ptr_trigger;
         ptr_trigger = ptr_trigger->next_trigger)
    {
        trigger_command_display_trigger (ptr_trigger, verbose);
    }
}

/*
 * Displays a list of default triggers.
 */

void
trigger_command_list_default (int verbose)
{
    int i, regex_count, commands_count;
    struct t_trigger_regex *regex;
    char **commands;

    regex_count = 0;
    regex = NULL;
    commands_count = 0;
    commands = NULL;

    weechat_printf_date_tags (NULL, 0, "no_trigger", "");
    weechat_printf_date_tags (NULL, 0, "no_trigger",
                              _("List of default triggers:"));

    for (i = 0; trigger_config_default_list[i][0]; i++)
    {
        if (trigger_regex_split (trigger_config_default_list[i][5],
                                 &regex_count,
                                 &regex) < 0)
            continue;
        trigger_split_command (trigger_config_default_list[i][6],
                               &commands_count,
                               &commands);
        trigger_command_display_trigger_internal (
            trigger_config_default_list[i][0],
            weechat_config_string_to_boolean (trigger_config_default_list[i][1]),
            trigger_config_default_list[i][2],
            trigger_config_default_list[i][3],
            trigger_config_default_list[i][4],
            0,
            0,
            0,
            regex_count,
            regex,
            commands_count,
            commands,
            trigger_search_return_code (trigger_config_default_list[i][7]),
            trigger_search_post_action (trigger_config_default_list[i][8]),
            verbose);
    }

    trigger_regex_free (&regex_count, &regex);
    if (commands)
        weechat_string_free_split (commands);
}

/*
 * Displays an error if a trigger is running.
 */

void
trigger_command_error_running (struct t_trigger *trigger, const char *action)
{
    weechat_printf_date_tags (NULL, 0, "no_trigger",
                              _("%s%s: action \"%s\" can not be executed on "
                                "trigger \"%s\" because it is currently "
                                "running"),
                              weechat_prefix ("error"), TRIGGER_PLUGIN_NAME,
                              action, trigger->name);
}

/*
 * Set "enabled" value in a trigger.
 *
 * Argument "enable" can be:
 *   -1: toggle trigger
 *    0: disable trigger
 *    1: enable trigger
 *    2: restart trigger (recreate hook(s))
 */

void
trigger_command_set_enabled (struct t_trigger *trigger,
                             int enable, const char *enable_string,
                             int display_error)
{
    if (trigger->hook_running)
    {
        trigger_command_error_running (trigger, enable_string);
        return;
    }

    if (enable == 2)
    {
        if (weechat_config_boolean (trigger->options[TRIGGER_OPTION_ENABLED]))
        {
            trigger_hook (trigger);
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      _("Trigger \"%s\" restarted"),
                                      trigger->name);
        }
        else if (display_error)
        {
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      _("%s%s: a disabled trigger can not be "
                                        "restarted"),
                                      weechat_prefix ("error"),
                                      TRIGGER_PLUGIN_NAME);
        }
    }
    else
    {
        if (enable < 0)
        {
            enable = weechat_config_boolean (trigger->options[TRIGGER_OPTION_ENABLED]) ?
                0 : 1;
        }
        weechat_config_option_set (trigger->options[TRIGGER_OPTION_ENABLED],
                                   (enable) ? "on" : "off", 1);
        weechat_printf_date_tags (NULL, 0, "no_trigger",
                                  (enable) ?
                                  _("Trigger \"%s\" enabled") :
                                  _("Trigger \"%s\" disabled"),
                                  trigger->name);
    }
}

/*
 * Renames a trigger and displays error if the rename is not possible.
 *
 * This function is called by commands:
 *   /trigger set OLD name NEW
 *   /trigger rename OLD NEW
 */

void
trigger_command_rename (struct t_trigger *trigger, const char *new_name)
{
    char *name, *name2;

    name = strdup (trigger->name);
    name2 = weechat_string_remove_quotes (new_name, "'\"");

    if (name && name2)
    {
        /* check that new name is valid */
        if (!trigger_name_valid (name2))
        {
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      _("%s%s: invalid trigger name: \"%s\""),
                                      weechat_prefix ("error"),
                                      TRIGGER_PLUGIN_NAME,
                                      name2);
            goto end;
        }
        /* check that no trigger already exists with the new name */
        if (trigger_search (name2))
        {
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      _("%s%s: trigger \"%s\" already "
                                        "exists"),
                                      weechat_prefix ("error"), TRIGGER_PLUGIN_NAME,
                                      name2);
            goto end;
        }
        /* rename the trigger */
        if (trigger_rename (trigger, name2))
        {
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      _("Trigger \"%s\" renamed to \"%s\""),
                                      name, trigger->name);
        }
        else
        {
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      _("%s%s: failed to rename trigger "
                                        "\"%s\""),
                                      weechat_prefix ("error"),
                                      TRIGGER_PLUGIN_NAME,
                                      name);
        }
    }

end:
    if (name)
        free (name);
    if (name2)
        free (name2);
}

/*
 * Builds a string with the command to create the trigger.
 *
 * Note: result must be freed after use.
 */

char *
trigger_command_build_string (const char *format, ...)
{
    weechat_va_format (format);
    return vbuffer;
}

/*
 * Returns escaped argument for input by adding a backslash before each double
 * quote.
 */

char *
trigger_command_escape_argument (const char *argument)
{
    return weechat_string_replace (argument, "\"", "\\\"");
}

/*
 * Callback for command "/trigger": manage triggers.
 */

int
trigger_command_trigger (const void *pointer, void *data,
                         struct t_gui_buffer *buffer, int argc,
                         char **argv, char **argv_eol)
{
    struct t_trigger *ptr_trigger, *ptr_trigger2;
    struct t_trigger_regex *regex;
    char *value, **sargv, **items, *input, str_pos[16];
    char *arg_arguments, *arg_conditions, *arg_regex, *arg_command;
    char *arg_return_code, *arg_post_action;
    int rc, i, j, type, count, index_option, enable, sargc, num_items, add_rc;
    int regex_count, regex_rc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    rc = WEECHAT_RC_OK;
    sargv = NULL;

    /* list all triggers */
    if ((argc == 1)
        || ((argc == 2) && (weechat_strcmp (argv[1], "list") == 0)))
    {
        trigger_command_list (_("List of triggers:"), 0);
        goto end;
    }

    /* full list of all triggers */
    if ((argc == 2) && (weechat_strcmp (argv[1], "listfull") == 0))
    {
        trigger_command_list (_("List of triggers:"), 1);
        goto end;
    }

    /* list of default triggers */
    if ((argc == 2) && (weechat_strcmp (argv[1], "listdefault") == 0))
    {
        trigger_command_list_default (1);
        goto end;
    }

    /* add a trigger */
    if ((weechat_strcmp (argv[1], "add") == 0)
        || (weechat_strcmp (argv[1], "addoff") == 0)
        || (weechat_strcmp (argv[1], "addreplace") == 0))
    {
        sargv = weechat_string_split_shell (argv_eol[2], &sargc);
        if (!sargv || (sargc < 2))
            goto error;
        if (!trigger_name_valid (sargv[0]))
        {
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      _("%s%s: invalid trigger name: \"%s\""),
                                      weechat_prefix ("error"),
                                      TRIGGER_PLUGIN_NAME,
                                      sargv[0]);
            goto end;
        }
        type = trigger_search_hook_type (sargv[1]);
        if (type < 0)
        {
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      _("%s%s: invalid hook type: \"%s\""),
                                      weechat_prefix ("error"),
                                      TRIGGER_PLUGIN_NAME,
                                      sargv[1]);
            goto end;
        }
        if ((sargc > 4) && sargv[4][0])
        {
            regex_count = 0;
            regex = NULL;
            regex_rc = trigger_regex_split (sargv[4], &regex_count, &regex);
            trigger_regex_free (&regex_count, &regex);
            switch (regex_rc)
            {
                case 0: /* OK */
                    break;
                case -1: /* format error */
                    weechat_printf (NULL,
                                    _("%s%s: invalid format for regular "
                                      "expression: \"%s\""),
                                    weechat_prefix ("error"),
                                    TRIGGER_PLUGIN_NAME,
                                    sargv[4]);
                    goto end;
                    break;
                case -2: /* regex compilation error */
                    weechat_printf (NULL,
                                    _("%s%s: invalid regular expression "
                                      "(compilation failed): \"%s\""),
                                    weechat_prefix ("error"),
                                    TRIGGER_PLUGIN_NAME,
                                    sargv[4]);
                    goto end;
                    break;
                case -3: /* memory error */
                    weechat_printf (NULL,
                                    _("%s%s: not enough memory for regular "
                                      "expression: \"%s\""),
                                    weechat_prefix ("error"),
                                    TRIGGER_PLUGIN_NAME,
                                    sargv[4]);
                    goto end;
                    break;
            }
        }
        if ((sargc > 6) && sargv[6][0]
            && (trigger_search_return_code (sargv[6]) < 0))
        {
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      _("%s%s: invalid return code: \"%s\""),
                                      weechat_prefix ("error"),
                                      TRIGGER_PLUGIN_NAME,
                                      sargv[6]);
            goto end;
        }
        if ((sargc > 7) && sargv[7][0]
            && (trigger_search_post_action (sargv[7]) < 0))
        {
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      _("%s%s: invalid post action \"%s\""),
                                      weechat_prefix ("error"),
                                      TRIGGER_PLUGIN_NAME,
                                      sargv[7]);
            goto end;
        }
        ptr_trigger = trigger_search (sargv[0]);
        if (ptr_trigger)
        {
            if (weechat_strcmp (argv[1], "addreplace") == 0)
            {
                if (ptr_trigger)
                {
                    if (ptr_trigger->hook_running)
                    {
                        trigger_command_error_running (ptr_trigger, argv[1]);
                        goto end;
                    }
                    trigger_free (ptr_trigger);
                }
            }
            else
            {
                weechat_printf_date_tags (
                    NULL, 0, "no_trigger",
                    _("%s%s: trigger \"%s\" already exists "
                      "(choose another name or use option "
                      "\"addreplace\" to overwrite it)"),
                    weechat_prefix ("error"),
                    TRIGGER_PLUGIN_NAME, sargv[0]);
                goto end;
            }
        }
        ptr_trigger = trigger_alloc (sargv[0]);
        if (!ptr_trigger)
        {
            weechat_printf_date_tags (
                NULL, 0, "no_trigger",
                _("%s%s: failed to create trigger \"%s\""),
                weechat_prefix ("error"), TRIGGER_PLUGIN_NAME,
                sargv[0]);
            goto end;
        }
        ptr_trigger = trigger_new (
            sargv[0],                      /* name */
            (weechat_strcmp (argv[1], "addoff") == 0) ? "off" : "on",
            sargv[1],                      /* hook */
            (sargc > 2) ? sargv[2] : "",   /* arguments */
            (sargc > 3) ? sargv[3] : "",   /* conditions */
            (sargc > 4) ? sargv[4] : "",   /* regex */
            (sargc > 5) ? sargv[5] : "",   /* command */
            (sargc > 6) ? sargv[6] : "",   /* return code */
            (sargc > 7) ? sargv[7] : "");  /* post action */
        if (ptr_trigger)
        {
            weechat_printf_date_tags (
                NULL, 0, "no_trigger",
                _("Trigger \"%s\" created"), sargv[0]);
        }
        else
        {
            weechat_printf_date_tags (
                NULL, 0, "no_trigger",
                _("%s%s: failed to create trigger \"%s\""),
                weechat_prefix ("error"), TRIGGER_PLUGIN_NAME,
                sargv[0]);
        }
        goto end;
    }

    /* add trigger command in input (to help trigger creation) */
    if (weechat_strcmp (argv[1], "addinput") == 0)
    {
        type = TRIGGER_HOOK_SIGNAL;
        if (argc >= 3)
        {
            type = trigger_search_hook_type (argv[2]);
            if (type < 0)
            {
                weechat_printf_date_tags (NULL, 0, "no_trigger",
                                          _("%s%s: invalid hook type: \"%s\""),
                                          weechat_prefix ("error"),
                                          TRIGGER_PLUGIN_NAME, argv[2]);
                goto end;
            }
        }
        items = weechat_string_split (trigger_hook_default_rc[type],
                                      ",",
                                      NULL,
                                      WEECHAT_STRING_SPLIT_STRIP_LEFT
                                      | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                      | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                      0,
                                      &num_items);
        input = trigger_command_build_string (
            "/trigger add name %s \"%s\" \"%s\" \"%s\" \"%s\"%s%s%s",
            trigger_hook_type_string[type],
            trigger_hook_default_arguments[type],
            TRIGGER_HOOK_DEFAULT_CONDITIONS,
            TRIGGER_HOOK_DEFAULT_REGEX,
            TRIGGER_HOOK_DEFAULT_COMMAND,
            (items && (num_items > 0)) ? " \"" : "",
            (items && (num_items > 0)) ? items[0] : "",
            (items && (num_items > 0)) ? "\"" : "");
        if (input)
        {
            weechat_buffer_set (buffer, "input", input);
            weechat_buffer_set (buffer, "input_pos", "13");
            free (input);
        }
        if (items)
            weechat_string_free_split (items);
        goto end;
    }

    /*
     * get command to create a trigger, and according to option:
     * - input: put the command in input
     * - output: send the command to the buffer
     * - recreate: same as input, but the trigger is first deleted
     */
    if ((weechat_strcmp (argv[1], "input") == 0)
        || (weechat_strcmp (argv[1], "output") == 0)
        || (weechat_strcmp (argv[1], "recreate") == 0))
    {
        if (argc < 3)
            goto error;
        ptr_trigger = trigger_search (argv[2]);
        if (!ptr_trigger)
        {
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      _("%s%s: trigger \"%s\" not found"),
                                      weechat_prefix ("error"),
                                      TRIGGER_PLUGIN_NAME,
                                      argv[2]);
            goto end;
        }
        add_rc = trigger_hook_default_rc[weechat_config_integer (ptr_trigger->options[TRIGGER_OPTION_HOOK])][0];
        arg_arguments = trigger_command_escape_argument (
            weechat_config_string (ptr_trigger->options[TRIGGER_OPTION_ARGUMENTS]));
        arg_conditions = trigger_command_escape_argument (
            weechat_config_string (ptr_trigger->options[TRIGGER_OPTION_CONDITIONS]));
        arg_regex = trigger_command_escape_argument (
            weechat_config_string (ptr_trigger->options[TRIGGER_OPTION_REGEX]));
        arg_command = trigger_command_escape_argument (
            weechat_config_string (ptr_trigger->options[TRIGGER_OPTION_COMMAND]));
        arg_return_code = trigger_command_escape_argument (
            (add_rc) ?
            weechat_config_string (ptr_trigger->options[TRIGGER_OPTION_RETURN_CODE]) : "");
        arg_post_action = trigger_command_escape_argument (
            weechat_config_string (ptr_trigger->options[TRIGGER_OPTION_POST_ACTION]));
        input = trigger_command_build_string (
            "//trigger %s %s %s \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"",
            (weechat_strcmp (argv[1], "recreate") == 0) ? "addreplace" : "add",
            ptr_trigger->name,
            weechat_config_string (ptr_trigger->options[TRIGGER_OPTION_HOOK]),
            arg_arguments,
            arg_conditions,
            arg_regex,
            arg_command,
            arg_return_code,
            arg_post_action);
        if (arg_arguments)
            free (arg_arguments);
        if (arg_conditions)
            free (arg_conditions);
        if (arg_regex)
            free (arg_regex);
        if (arg_command)
            free (arg_command);
        if (arg_return_code)
            free (arg_return_code);
        if (arg_post_action)
            free (arg_post_action);
        if (input)
        {
            if (weechat_strcmp (argv[1], "output") == 0)
            {
                weechat_command (buffer, input);
            }
            else
            {
                weechat_buffer_set (buffer, "input", input + 1);
                snprintf (str_pos, sizeof (str_pos),
                          "%d", weechat_utf8_strlen (input + 1));
                weechat_buffer_set (buffer, "input_pos", str_pos);
            }
            free (input);
        }
        goto end;
    }

    /* set option in a trigger */
    if (weechat_strcmp (argv[1], "set") == 0)
    {
        if (argc < 5)
            goto error;
        ptr_trigger = trigger_search (argv[2]);
        if (!ptr_trigger)
        {
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      _("%s%s: trigger \"%s\" not found"),
                                      weechat_prefix ("error"),
                                      TRIGGER_PLUGIN_NAME,
                                      argv[2]);
            goto end;
        }
        if (ptr_trigger->hook_running)
        {
            trigger_command_error_running (ptr_trigger, argv[1]);
            goto end;
        }
        if (weechat_strcmp (argv[3], "name") == 0)
        {
            trigger_command_rename (ptr_trigger, argv[4]);
            goto end;
        }
        value = weechat_string_remove_quotes (argv_eol[4], "'\"");
        if (value)
        {
            index_option = trigger_search_option (argv[3]);
            if (index_option >= 0)
            {
                weechat_config_option_set (ptr_trigger->options[index_option],
                                           value, 1);
                weechat_printf_date_tags (NULL, 0, "no_trigger",
                                          _("Trigger \"%s\" updated"),
                                          ptr_trigger->name);
            }
            else
            {
                weechat_printf_date_tags (NULL, 0, "no_trigger",
                                          _("%s%s: trigger option \"%s\" not "
                                            "found"),
                                          weechat_prefix ("error"),
                                          TRIGGER_PLUGIN_NAME, argv[3]);
            }
            free (value);
        }
        goto end;
    }

    /* rename a trigger */
    if (weechat_strcmp (argv[1], "rename") == 0)
    {
        if (argc < 4)
            goto error;
        ptr_trigger = trigger_search (argv[2]);
        if (!ptr_trigger)
        {
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      _("%s%s: trigger \"%s\" not found"),
                                      weechat_prefix ("error"),
                                      TRIGGER_PLUGIN_NAME,
                                      argv[2]);
            goto end;
        }
        if (ptr_trigger->hook_running)
        {
            trigger_command_error_running (ptr_trigger, argv[1]);
            goto end;
        }
        trigger_command_rename (ptr_trigger, argv[3]);
        goto end;
    }

    /* copy a trigger */
    if (weechat_strcmp (argv[1], "copy") == 0)
    {
        if (argc < 4)
            goto error;
        ptr_trigger = trigger_search (argv[2]);
        if (!ptr_trigger)
        {
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      _("%s%s: trigger \"%s\" not found"),
                                      weechat_prefix ("error"),
                                      TRIGGER_PLUGIN_NAME,
                                      argv[2]);
            goto end;
        }
        /* check that new name is valid */
        if (!trigger_name_valid (argv[3]))
        {
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      _("%s%s: invalid trigger name: \"%s\""),
                                      weechat_prefix ("error"),
                                      TRIGGER_PLUGIN_NAME,
                                      argv[3]);
            goto end;
        }
        /* check that no trigger already exists with the new name */
        if (trigger_search (argv[3]))
        {
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      _("%s%s: trigger \"%s\" already "
                                        "exists"),
                                      weechat_prefix ("error"),
                                      TRIGGER_PLUGIN_NAME,
                                      argv[3]);
            goto end;
        }
        /* copy the trigger */
        ptr_trigger2 = trigger_copy (ptr_trigger, argv[3]);
        if (ptr_trigger2)
        {
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      _("Trigger \"%s\" copied to \"%s\""),
                                      ptr_trigger->name, ptr_trigger2->name);
        }
        else
        {
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      _("%s%s: failed to copy trigger "
                                        "\"%s\""),
                                      weechat_prefix ("error"),
                                      TRIGGER_PLUGIN_NAME,
                                      ptr_trigger->name);
        }
        goto end;
    }

    /* enable/disable/toggle/restart trigger(s) */
    if ((weechat_strcmp (argv[1], "enable") == 0)
        || (weechat_strcmp (argv[1], "disable") == 0)
        || (weechat_strcmp (argv[1], "toggle") == 0)
        || (weechat_strcmp (argv[1], "restart") == 0))
    {
        if (argc < 3)
        {
            if (weechat_strcmp (argv[1], "restart") == 0)
                goto error;
            if (weechat_strcmp (argv[1], "enable") == 0)
                weechat_config_option_set (trigger_config_look_enabled, "1", 1);
            else if (weechat_strcmp (argv[1], "disable") == 0)
                weechat_config_option_set (trigger_config_look_enabled, "0", 1);
            else if (weechat_strcmp (argv[1], "toggle") == 0)
            {
                weechat_config_option_set (trigger_config_look_enabled,
                                           (trigger_enabled) ? "0" : "1",
                                           1);
            }
            trigger_command_display_status ();
            goto end;
        }
        enable = -1;
        if (weechat_strcmp (argv[1], "enable") == 0)
            enable = 1;
        else if (weechat_strcmp (argv[1], "disable") == 0)
            enable = 0;
        else if (weechat_strcmp (argv[1], "restart") == 0)
            enable = 2;
        if (weechat_strcmp (argv[2], "-all") == 0)
        {
            for (ptr_trigger = triggers; ptr_trigger;
                 ptr_trigger = ptr_trigger->next_trigger)
            {
                trigger_command_set_enabled (ptr_trigger, enable, argv[1], 0);
            }
        }
        else
        {
            for (i = 2; i < argc; i++)
            {
                ptr_trigger = trigger_search (argv[i]);
                if (ptr_trigger)
                    trigger_command_set_enabled (ptr_trigger, enable, argv[1],
                                                 1);
                else
                {
                    weechat_printf_date_tags (NULL, 0, "no_trigger",
                                              _("%sTrigger \"%s\" not found"),
                                              weechat_prefix ("error"),
                                              argv[i]);
                }
            }
        }
        goto end;
    }

    /* delete trigger(s) */
    if (weechat_strcmp (argv[1], "del") == 0)
    {
        if (argc < 3)
            goto error;
        if (weechat_strcmp (argv[2], "-all") == 0)
        {
            count = triggers_count;
            ptr_trigger = triggers;
            while (ptr_trigger)
            {
                ptr_trigger2 = ptr_trigger->next_trigger;
                if (ptr_trigger->hook_running)
                {
                    trigger_command_error_running (ptr_trigger, argv[1]);
                }
                else
                {
                    trigger_free (ptr_trigger);
                }
                ptr_trigger = ptr_trigger2;
            }
            count = count - triggers_count;
            if (count > 0)
                weechat_printf_date_tags (NULL, 0, "no_trigger",
                                          _("%d triggers removed"), count);
        }
        else
        {
            for (i = 2; i < argc; i++)
            {
                ptr_trigger = trigger_search (argv[i]);
                if (ptr_trigger)
                {
                    if (ptr_trigger->hook_running)
                    {
                        trigger_command_error_running (ptr_trigger, argv[1]);
                    }
                    else
                    {
                        trigger_free (ptr_trigger);
                        weechat_printf_date_tags (NULL, 0, "no_trigger",
                                                  _("Trigger \"%s\" removed"),
                                                  argv[i]);
                    }
                }
                else
                {
                    weechat_printf_date_tags (NULL, 0, "no_trigger",
                                              _("%sTrigger \"%s\" not found"),
                                              weechat_prefix ("error"),
                                              argv[i]);
                }
            }
        }
        goto end;
    }

    /* show detailed info on a trigger */
    if (weechat_strcmp (argv[1], "show") == 0)
    {
        if (argc < 3)
            goto error;
        ptr_trigger = trigger_search (argv[2]);
        if (!ptr_trigger)
        {
            weechat_printf_date_tags (NULL, 0, "no_trigger",
                                      _("%s%s: trigger \"%s\" not found"),
                                      weechat_prefix ("error"),
                                      TRIGGER_PLUGIN_NAME,
                                      argv[2]);
            goto end;
        }
        weechat_printf_date_tags (NULL, 0, "no_trigger", "");
        weechat_printf_date_tags (NULL, 0, "no_trigger", _("Trigger:"));
        trigger_command_display_trigger (ptr_trigger, 2);
        goto end;
    }

    /* restore default trigger(s) */
    if (weechat_strcmp (argv[1], "restore") == 0)
    {
        if (argc < 3)
            goto error;
        for (i = 2; i < argc; i++)
        {
            for (j = 0; trigger_config_default_list[j][0]; j++)
            {
                if (strcmp (trigger_config_default_list[j][0], argv[i]) == 0)
                    break;
            }
            if (trigger_config_default_list[j][0])
            {
                ptr_trigger = trigger_search (argv[i]);
                if (ptr_trigger && ptr_trigger->hook_running)
                {
                    trigger_command_error_running (ptr_trigger, argv[1]);
                }
                else
                {
                    if (ptr_trigger)
                        trigger_free (ptr_trigger);
                    trigger_new (
                        trigger_config_default_list[j][0],   /* name */
                        trigger_config_default_list[j][1],   /* enabled */
                        trigger_config_default_list[j][2],   /* hook */
                        trigger_config_default_list[j][3],   /* arguments */
                        trigger_config_default_list[j][4],   /* conditions */
                        trigger_config_default_list[j][5],   /* regex */
                        trigger_config_default_list[j][6],   /* command */
                        trigger_config_default_list[j][7],   /* return code */
                        trigger_config_default_list[j][8]);  /* post action */
                    weechat_printf_date_tags (NULL, 0, "no_trigger",
                                              _("Trigger \"%s\" restored"),
                                              argv[i]);
                }
            }
            else
            {
                weechat_printf_date_tags (
                    NULL, 0, "no_trigger",
                    _("%sDefault trigger \"%s\" not found"),
                    weechat_prefix ("error"), argv[i]);
            }
        }
        goto end;
    }

    /* delete all triggers and restore default ones */
    if (weechat_strcmp (argv[1], "default") == 0)
    {
        if ((argc >= 3) && (weechat_strcmp (argv[2], "-yes") == 0))
        {
            ptr_trigger = triggers;
            while (ptr_trigger)
            {
                ptr_trigger2 = ptr_trigger->next_trigger;
                if (ptr_trigger->hook_running)
                {
                    trigger_command_error_running (ptr_trigger, argv[1]);
                }
                else
                {
                    trigger_free (ptr_trigger);
                }
                ptr_trigger = ptr_trigger2;
            }
            if (triggers_count == 0)
            {
                trigger_create_default ();
                trigger_command_list (_("Default triggers restored:"), 0);
            }
        }
        else
        {
            weechat_printf (NULL,
                            _("%s%s: \"-yes\" argument is required for "
                              "restoring default triggers (security reason)"),
                            weechat_prefix ("error"), TRIGGER_PLUGIN_NAME);
        }
        goto end;
    }

    /* open the trigger monitor buffer */
    if (weechat_strcmp (argv[1], "monitor") == 0)
    {
        trigger_buffer_open ((argc > 2) ? argv_eol[2] : NULL, 1);
        goto end;
    }

error:
    rc = WEECHAT_RC_ERROR;

end:
    if (sargv)
        weechat_string_free_split (sargv);

    if (rc == WEECHAT_RC_ERROR)
        WEECHAT_COMMAND_ERROR;

    return rc;
}

/*
 * Hooks trigger commands.
 */

void
trigger_command_init ()
{
    weechat_hook_command (
        "trigger",
        N_("manage triggers, the Swiss Army knife for WeeChat"),
        N_("list|listfull|listdefault"
           " || add|addoff|addreplace <name> <hook> [\"<arguments>\" "
           "[\"<conditions>\" [\"<regex>\" [\"<command>\" "
           "[\"<return_code>\" [\"<post_action>\"]]]]]]"
           " || addinput [<hook>]"
           " || input|output|recreate <name>"
           " || set <name> <option> <value>"
           " || rename|copy <name> <new_name>"
           " || enable|disable|toggle [<name>|-all [<name>...]]"
           " || restart <name>|-all [<name>...]"
           " || show <name>"
           " || del <name>|-all [<name>...]"
           " || restore <name> [<name>...]"
           " || default -yes"
           " || monitor [<filter>]"),
        N_("       list: list triggers (without argument, this list is displayed)\n"
           "   listfull: list triggers with detailed info for each trigger\n"
           "listdefault: list default triggers\n"
           "        add: add a trigger\n"
           "     addoff: add a trigger (disabled)\n"
           " addreplace: add or replace an existing trigger\n"
           "       name: name of trigger\n"
           "       hook: signal, hsignal, modifier, line, print, command, "
           "command_run, timer, config, focus, info, info_hashtable\n"
           "  arguments: arguments for the hook, depending on hook (separated "
           "by semicolons):\n"
           "             signal: name(s) of signal (required)\n"
           "             hsignal: name(s) of hsignal (required)\n"
           "             modifier: name(s) of modifier (required)\n"
           "             line: buffer type (\"formatted\", \"free\" or \"*\"), "
           "list of buffer masks, tags\n"
           "             print: buffer, tags, message, strip colors\n"
           "             command: command (required), description, arguments, "
           "description of arguments, completion (all arguments except command "
           "are evaluated, \"${tg_trigger_name}\" is replaced by the trigger "
           "name, see /help eval)\n"
           "             command_run: command(s) (required)\n"
           "             timer: interval (required), align on second, max calls\n"
           "             config: name(s) of option (required)\n"
           "             focus: name(s) of area (required)\n"
           "             info: name(s) of info (required)\n"
           "             info_hashtable: name(s) of info (required)\n"
           " conditions: evaluated conditions for the trigger\n"
           "      regex: one or more regular expressions to replace strings "
           "in variables\n"
           "    command: command to execute (many commands can be separated by "
           "\";\")\n"
           "return_code: return code in callback (ok (default), ok_eat, error)\n"
           "post_action: action to take after execution (none (default), "
           "disable, delete)\n"
           "   addinput: set input with default arguments to create a trigger\n"
           "      input: set input with the command used to create the trigger\n"
           "     output: send the command to create the trigger on the buffer\n"
           "   recreate: same as \"input\", with option \"addreplace\" instead "
           "of \"add\"\n"
           "        set: set an option in a trigger\n"
           "     option: name of option: name, hook, arguments, conditions, "
           "regex, command, return_code\n"
           "             (for help on option, you can type: /help "
           "trigger.trigger.<name>.<option>)\n"
           "      value: new value for the option\n"
           "     rename: rename a trigger\n"
           "       copy: copy a trigger\n"
           "     enable: enable trigger(s) (without arguments: enable triggers "
           "globally)\n"
           "    disable: disable trigger(s) (without arguments: disable triggers "
           "globally)\n"
           "     toggle: toggle trigger(s) (without arguments: toggle triggers "
           "globally)\n"
           "    restart: restart trigger(s) (recreate the hooks)\n"
           "       show: show detailed info on a trigger (with some stats)\n"
           "        del: delete a trigger\n"
           "       -all: do action on all triggers\n"
           "    restore: restore trigger(s) with the default values (works "
           "only for default triggers)\n"
           "    default: delete all triggers and restore default ones\n"
           "    monitor: open the trigger monitor buffer, with optional filter:\n"
           "     filter: filter hooks/triggers to display (a hook must start "
           "with \"@\", for example \"@signal\"), many filters can be separated "
           "by commas; wildcard \"*\" is allowed in each trigger name\n"
           "\n"
           "When a trigger callback is called, following actions are performed, "
           "in this order:\n"
           "  1. check conditions; if false, exit\n"
           "  2. replace text using POSIX extended regular expression(s) (if "
           "defined in trigger)\n"
           "  3. execute command(s) (if defined in trigger)\n"
           "  4. exit with a return code (except for modifier, line, focus, "
           "info and info_hashtable)\n"
           "  5. perform post action\n"
           "\n"
           "Examples (you can also look at default triggers with /trigger "
           "listdefault):\n"
           "  add text attributes *bold*, _underline_ and /italic/ (only in "
           "user messages):\n"
           "    /trigger add effects modifier weechat_print \"${tg_tag_nick}\" "
           "\"==\\*([^ ]+)\\*==*${color:bold}${re:1}${color:-bold}*== "
           "==_([^ ]+)_==_${color:underline}${re:1}${color:-underline}_== "
           "==/([^ ]+)/==/${color:italic}${re:1}${color:-italic}/\"\n"
           "  hide nicklist bar on small terminals:\n"
           "    /trigger add resize_small signal signal_sigwinch "
           "\"${info:term_width} < 100\" \"\" \"/bar hide nicklist\"\n"
           "    /trigger add resize_big signal signal_sigwinch "
           "\"${info:term_width} >= 100\" \"\" \"/bar show nicklist\"\n"
           "  silently save config each hour:\n"
           "    /trigger add cfgsave timer 3600000;0;0 \"\" \"\" \"/mute /save\"\n"
           "  silently save WeeChat session at midnight (see /help upgrade):\n"
           "    /trigger add session_save signal day_changed \"\" \"\" "
           "\"/mute /upgrade -save\"\n"
           "  open trigger monitor and show only modifiers and triggers whose "
           "name starts with \"resize\":\n"
           "    /trigger monitor @modifier,resize*"),
        "list|listfull|listdefault"
        " || add|addoff|addreplace %(trigger_add_arguments)|%*"
        " || addinput %(trigger_hooks)"
        " || input|output|recreate %(trigger_names)"
        " || set %(trigger_names) %(trigger_options)|name %(trigger_option_value)"
        " || rename|copy %(trigger_names) %(trigger_names)"
        " || enable %(trigger_names_disabled)|-all %(trigger_names_disabled)|%*"
        " || disable %(trigger_names_enabled)|-all %(trigger_names_enabled)|%*"
        " || toggle|restart|del %(trigger_names)|-all %(trigger_names)|%*"
        " || show %(trigger_names)"
        " || restore %(trigger_names_default)|%*"
        " || default"
        " || monitor %(trigger_names)|%(trigger_hooks_filter)",
        &trigger_command_trigger, NULL, NULL);
}
