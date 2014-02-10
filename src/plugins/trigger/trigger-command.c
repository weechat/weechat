/*
 * trigger-command.c - trigger command
 *
 * Copyright (C) 2014 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "trigger.h"
#include "trigger-buffer.h"
#include "trigger-config.h"


/*
 * Displays one trigger.
 */

void
trigger_command_display_trigger (const char *name, int enabled, const char *hook,
                                 const char *arguments, const char *conditions,
                                 int regex_count, struct t_trigger_regex *regex,
                                 int commands_count, char **commands,
                                 int return_code, int full)
{
    char str_conditions[64], str_regex[64], str_command[64], str_rc[64];
    char spaces[256];
    int i, length;

    if (full)
    {
        weechat_printf_tags (
            NULL, "no_trigger",
            "  %s%s%s: %s%s%s%s%s%s%s",
            (enabled) ?
            weechat_color (weechat_config_string (trigger_config_color_trigger)) :
            weechat_color (weechat_config_string (trigger_config_color_trigger_disabled)),
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
        if (conditions && conditions[0])
        {
            weechat_printf_tags (NULL, "no_trigger",
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
            weechat_printf_tags (NULL, "no_trigger",
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
                weechat_printf_tags (NULL, "no_trigger",
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
            weechat_printf_tags (NULL, "no_trigger",
                                 "%s %s=> %s%s",
                                 spaces,
                                 weechat_color (weechat_config_string (trigger_config_color_flag_return_code)),
                                 weechat_color ("reset"),
                                 trigger_return_code_string[return_code]);
        }
    }
    else
    {
        str_conditions[0] ='\0';
        str_regex[0] = '\0';
        str_command[0] = '\0';
        str_rc[0] = '\0';
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
        weechat_printf_tags (
            NULL, "no_trigger",
            "  %s%s%s: %s%s%s%s%s%s%s%s%s%s%s%s",
            (enabled) ?
            weechat_color (weechat_config_string (trigger_config_color_trigger)) :
            weechat_color (weechat_config_string (trigger_config_color_trigger_disabled)),
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
            str_rc);
    }
}

/*
 * Displays a list of triggers.
 */

void
trigger_command_list (const char *message, int full)
{
    struct t_trigger *ptr_trigger;

    if (!triggers)
    {
        weechat_printf_tags (NULL, "no_trigger", _("No trigger defined"));
        return;
    }

    weechat_printf_tags (NULL, "no_trigger", "");
    weechat_printf_tags (NULL, "no_trigger", message);

    for (ptr_trigger = triggers; ptr_trigger;
         ptr_trigger = ptr_trigger->next_trigger)
    {
        trigger_command_display_trigger (
            ptr_trigger->name,
            weechat_config_boolean (ptr_trigger->options[TRIGGER_OPTION_ENABLED]),
            weechat_config_string (ptr_trigger->options[TRIGGER_OPTION_HOOK]),
            weechat_config_string (ptr_trigger->options[TRIGGER_OPTION_ARGUMENTS]),
            weechat_config_string (ptr_trigger->options[TRIGGER_OPTION_CONDITIONS]),
            ptr_trigger->regex_count,
            ptr_trigger->regex,
            ptr_trigger->commands_count,
            ptr_trigger->commands,
            weechat_config_integer (ptr_trigger->options[TRIGGER_OPTION_RETURN_CODE]),
            full);
    }
}

/*
 * Displays a list of default triggers.
 */

void
trigger_command_list_default (int full)
{
    int i, regex_count, commands_count;
    struct t_trigger_regex *regex;
    char **commands;

    regex_count = 0;
    regex = NULL;
    commands_count = 0;
    commands = NULL;

    weechat_printf_tags (NULL, "no_trigger", "");
    weechat_printf_tags (NULL, "no_trigger", _("List of default triggers:"));

    for (i = 0; trigger_config_default_list[i][0]; i++)
    {
        trigger_split_regex (trigger_config_default_list[i][0],
                             trigger_config_default_list[i][5],
                             &regex_count,
                             &regex);
        trigger_split_command (trigger_config_default_list[i][6],
                               &commands_count,
                               &commands);
        trigger_command_display_trigger (
            trigger_config_default_list[i][0],
            weechat_config_string_to_boolean (trigger_config_default_list[i][1]),
            trigger_config_default_list[i][2],
            trigger_config_default_list[i][3],
            trigger_config_default_list[i][4],
            regex_count,
            regex,
            commands_count,
            commands,
            trigger_search_return_code (trigger_config_default_list[i][7]),
            full);
    }

    trigger_free_regex (&regex_count, &regex);
    if (commands)
        weechat_string_free_split (commands);
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
trigger_command_set_enabled (struct t_trigger *trigger, int enable)
{
    if (enable == 2)
    {
        trigger_unhook (trigger);
        trigger_hook (trigger);
        weechat_printf_tags (NULL, "no_trigger",
                             _("Trigger \"%s\" restarted"),
                             trigger->name);
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
        weechat_printf_tags (NULL, "no_trigger",
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
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sError: invalid name for trigger"),
                                 weechat_prefix ("error"));
            goto end;
        }
        /* check that no trigger already exists with the new name */
        if (trigger_search (name2))
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sError: trigger \"%s\" already "
                                   "exists"),
                                 weechat_prefix ("error"), name2);
            goto end;
        }
        /* rename the trigger */
        if (trigger_rename (trigger, name2))
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("Trigger \"%s\" renamed to \"%s\""),
                                 name, trigger->name);
        }
        else
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sError: failed to rename trigger "
                                   "\"%s\""),
                                 weechat_prefix ("error"), name);
        }
    }

end:
    if (name)
        free (name);
    if (name2)
        free (name2);
}

/*
 * Callback for command "/trigger": manage triggers.
 */

int
trigger_command_trigger (void *data, struct t_gui_buffer *buffer, int argc,
                         char **argv, char **argv_eol)
{
    struct t_trigger *ptr_trigger;
    char *value, **sargv, **items, input[1024];
    int i, type, count, index_option, enable, sargc, num_items;

    /* make C compiler happy */
    (void) data;

    sargv = NULL;

    /* list all triggers */
    if ((argc == 1)
        || ((argc == 2) && (weechat_strcasecmp (argv[1], "list") == 0)))
    {
        trigger_command_list (_("List of triggers:"), 0);
        goto end;
    }

    /* full list of all triggers */
    if ((argc == 2) && (weechat_strcasecmp (argv[1], "listfull") == 0))
    {
        trigger_command_list (_("List of triggers:"), 1);
        goto end;
    }

    /* list of default triggers */
    if ((argc == 2) && (weechat_strcasecmp (argv[1], "listdefault") == 0))
    {
        trigger_command_list_default (1);
        goto end;
    }

    /* add a trigger */
    if (weechat_strcasecmp (argv[1], "add") == 0)
    {
        sargv = weechat_string_split_shell (argv_eol[2], &sargc);
        if (!sargv || (sargc < 2))
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sError: missing arguments for \"%s\" "
                                   "command"),
                                 weechat_prefix ("error"), "trigger");
            goto end;
        }
        if (!trigger_name_valid (sargv[0]))
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sError: invalid name for trigger"),
                                 weechat_prefix ("error"));
            goto end;
        }
        type = trigger_search_hook_type (sargv[1]);
        if (type < 0)
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sError: invalid hook type \"%s\""),
                                 weechat_prefix ("error"), sargv[1]);
            goto end;
        }
        if ((sargc > 6) && (trigger_search_return_code (sargv[6]) < 0))
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sError: invalid return code \"%s\""),
                                 weechat_prefix ("error"), sargv[6]);
            goto end;
        }
        ptr_trigger = trigger_alloc (sargv[0]);
        if (!ptr_trigger)
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sError: failed to create trigger \"%s\""),
                                 weechat_prefix ("error"), sargv[0]);
            goto end;
        }
        ptr_trigger = trigger_new (
            sargv[0],                      /* name */
            "on",                          /* enabled */
            sargv[1],                      /* hook */
            (sargc > 2) ? sargv[2] : "",   /* arguments */
            (sargc > 3) ? sargv[3] : "",   /* conditions */
            (sargc > 4) ? sargv[4] : "",   /* regex */
            (sargc > 5) ? sargv[5] : "",   /* command */
            (sargc > 6) ? sargv[6] : "");  /* return code */
        if (ptr_trigger)
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("Trigger \"%s\" created"), sargv[0]);
        }
        else
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sError: failed to create trigger \"%s\""),
                                 weechat_prefix ("error"), sargv[0]);
        }
        goto end;
    }

    /* add trigger command in input (to help trigger creation) */
    if (weechat_strcasecmp (argv[1], "addinput") == 0)
    {
        type = TRIGGER_HOOK_SIGNAL;
        if (argc >= 3)
        {
            type = trigger_search_hook_type (argv[2]);
            if (type < 0)
            {
                weechat_printf_tags (NULL, "no_trigger",
                                     _("%sError: invalid hook type \"%s\""),
                                     weechat_prefix ("error"), argv[2]);
                goto end;
            }
        }
        items = weechat_string_split (trigger_hook_default_rc[type], ",", 0, 0,
                                      &num_items);
        snprintf (input, sizeof (input),
                  "/trigger add name %s \"%s\" \"%s\" \"%s\" \"%s\"%s%s%s",
                  trigger_hook_type_string[type],
                  trigger_hook_default_arguments[type],
                  trigger_hook_default_conditions[type],
                  trigger_hook_default_regex[type],
                  trigger_hook_default_command[type],
                  (items && (num_items > 0)) ? " \"" : "",
                  (items && (num_items > 0)) ? items[0] : "",
                  (items && (num_items > 0)) ? "\"" : "");
        weechat_buffer_set (buffer, "input", input);
        weechat_buffer_set (buffer, "input_pos", "13");
        goto end;
    }

    /* set option in a trigger */
    if (weechat_strcasecmp (argv[1], "set") == 0)
    {
        if (argc < 5)
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sError: missing arguments for \"%s\" "
                                   "command"),
                                 weechat_prefix ("error"), "trigger");
            goto end;
        }
        ptr_trigger = trigger_search (argv[2]);
        if (!ptr_trigger)
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sError: trigger \"%s\" not found"),
                                 weechat_prefix ("error"), argv[2]);
            goto end;
        }
        if (weechat_strcasecmp (argv[3], "name") == 0)
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
                weechat_printf_tags (NULL, "no_trigger",
                                     _("Trigger \"%s\" updated"),
                                     ptr_trigger->name);
            }
            else
            {
                weechat_printf_tags (NULL, "no_trigger",
                                     _("%sError: trigger option \"%s\" not "
                                       "found"),
                                     weechat_prefix ("error"), argv[3]);
            }
            free (value);
        }
        goto end;
    }

    /* rename a trigger */
    if (weechat_strcasecmp (argv[1], "rename") == 0)
    {
        if (argc < 4)
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sError: missing arguments for \"%s\" "
                                   "command"),
                                 weechat_prefix ("error"), "trigger");
            goto end;
        }
        ptr_trigger = trigger_search (argv[2]);
        if (!ptr_trigger)
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sError: trigger \"%s\" not found"),
                                 weechat_prefix ("error"), argv[2]);
            goto end;
        }
        trigger_command_rename (ptr_trigger, argv[3]);
        goto end;
    }

    /* enable/disable/toggle/restart trigger(s) */
    if ((weechat_strcasecmp (argv[1], "enable") == 0)
        || (weechat_strcasecmp (argv[1], "disable") == 0)
        || (weechat_strcasecmp (argv[1], "toggle") == 0)
        || (weechat_strcasecmp (argv[1], "restart") == 0))
    {
        if (argc < 3)
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sError: missing arguments for \"%s\" "
                                   "command"),
                                 weechat_prefix ("error"), "trigger");
            goto end;
        }
        enable = -1;
        if (weechat_strcasecmp (argv[1], "enable") == 0)
            enable = 1;
        else if (weechat_strcasecmp (argv[1], "disable") == 0)
            enable = 0;
        else if (weechat_strcasecmp (argv[1], "restart") == 0)
            enable = 2;
        if (weechat_strcasecmp (argv[2], "-all") == 0)
        {
            for (ptr_trigger = triggers; ptr_trigger;
                 ptr_trigger = ptr_trigger->next_trigger)
            {
                trigger_command_set_enabled (ptr_trigger, enable);
            }
        }
        else
        {
            for (i = 2; i < argc; i++)
            {
                ptr_trigger = trigger_search (argv[i]);
                if (ptr_trigger)
                    trigger_command_set_enabled (ptr_trigger, enable);
                else
                {
                    weechat_printf_tags (NULL, "no_trigger",
                                         _("%sTrigger \"%s\" not found"),
                                         weechat_prefix ("error"), argv[i]);
                }
            }
        }
        goto end;
    }

    /* delete trigger(s) */
    if (weechat_strcasecmp (argv[1], "del") == 0)
    {
        if (argc < 3)
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sError: missing arguments for \"%s\" "
                                   "command"),
                                 weechat_prefix ("error"), "trigger");
            goto end;
        }
        if (weechat_strcasecmp (argv[2], "-all") == 0)
        {
            count = triggers_count;
            trigger_free_all ();
            if (count > 0)
                weechat_printf_tags (NULL, "no_trigger",
                                     _("%d triggers removed"), count);
        }
        else
        {
            for (i = 2; i < argc; i++)
            {
                ptr_trigger = trigger_search (argv[i]);
                if (ptr_trigger)
                {
                    trigger_free (ptr_trigger);
                    weechat_printf_tags (NULL, "no_trigger",
                                         _("Trigger \"%s\" removed"), argv[i]);
                }
                else
                {
                    weechat_printf_tags (NULL, "no_trigger",
                                         _("%sTrigger \"%s\" not found"),
                                         weechat_prefix ("error"), argv[i]);
                }
            }
        }
        goto end;
    }

    /* restore default triggers */
    if (weechat_strcasecmp (argv[1], "default") == 0)
    {
        if ((argc >= 3) && (weechat_strcasecmp (argv[2], "-yes") == 0))
        {
            trigger_free_all ();
            trigger_create_default ();
            trigger_command_list (_("Default triggers restored:"), 0);
        }
        else
        {
            weechat_printf (NULL,
                            _("%sError: \"-yes\" argument is required for "
                              "restoring default triggers (security reason)"),
                            weechat_prefix ("error"));
        }
        goto end;
    }

    /* open the trigger monitor buffer */
    if (weechat_strcasecmp (argv[1], "monitor") == 0)
    {
        trigger_buffer_open (1);
        goto end;
    }

end:
    if (sargv)
        weechat_string_free_split (sargv);

    return WEECHAT_RC_OK;
}

/*
 * Hooks trigger commands.
 */

void
trigger_command_init ()
{
    weechat_hook_command (
        "trigger",
        N_("manage triggers"),
        N_("list|listfull|listdefault"
           " || add <name> <hook> [\"<arguments>\" [\"<conditions>\" "
           "[\"<regex>\" [\"<command>\" [\"<return_code>\"]]]]]"
           " || addinput [<hook>]"
           " || set <name> <option> <value>"
           " || rename <name> <new_name>"
           " || enable|disable|toggle|restart <name>|-all [<name>...]"
           " || del <name>|-all [<name>...]"
           " || default -yes"
           " || monitor"),
        N_("       list: list triggers (without argument, this list is displayed)\n"
           "   listfull: list triggers with detailed info for each trigger\n"
           "listdefault: list default triggers\n"
           "        add: add a trigger\n"
           "       name: name of trigger\n"
           "       hook: signal, hsignal, modifier, print, timer\n"
           "  arguments: arguments for the hook, depending on hook (separated "
           "by semicolons):\n"
           "             signal: name(s) of signal\n"
           "             hsignal: name(s) of hsignal\n"
           "             modifier: name(s) of modifier\n"
           "             print: buffer, tags, message, strip_colors\n"
           "             timer: interval, align_second, max_calls\n"
           " conditions: evaluated conditions for the trigger\n"
           "      regex: one or more regular expressions to replace strings "
           "in variables\n"
           "    command: command to execute (many commands can be separated by "
           "\";\"\n"
           "return_code: return code in callback (ok (default), ok_eat, error)\n"
           "   addinput: set input with default arguments to create a trigger\n"
           "        set: set an option in a trigger\n"
           "     option: name of option: name, hook, arguments, conditions, "
           "regex, command, return_code\n"
           "             (for help on option, you can do /help "
           "trigger.trigger.<name>.<option>)\n"
           "      value: new value for the option\n"
           "     rename: rename a trigger\n"
           "     enable: enable trigger(s)\n"
           "    disable: disable trigger(s)\n"
           "     toggle: toggle trigger(s)\n"
           "    restart: restart trigger(s) (for timer)\n"
           "        del: delete a trigger\n"
           "       -all: do action on all triggers\n"
           "    default: restore default triggers\n"
           "    monitor: open the trigger monitor buffer\n"
           "\n"
           "When a trigger callback is called, following actions are performed, "
           "in this order:\n"
           "  1. check conditions; if false, exit\n"
           "  2. replace text using POSIX extended regular expression(s) (if "
           "defined in trigger)\n"
           "  3. execute command(s) (if defined in trigger)\n"
           "  4. exit with a return code (except for modifiers)\n"
           "\n"
           "Examples:\n"
           "  send alert (BEL) on highlight or private message:\n"
           "    /trigger add beep print \"\" \"${tg_highlight} || ${tg_msg_pv}\" "
           "\"\" \"/print -stderr \\a\"\n"
           "  replace password with '*' in /oper command (in command line and "
           "command history):\n"
           "    /trigger add oper modifier input_text_display;history_add "
           "\"\" \"==^(/oper +\\S+ +)(.*)==$1$.*2\"\n"
           "  add text attributes *bold*, _underline_ and /italic/ (only in "
           "user messages):\n"
           "    /trigger add effects modifier weechat_print \"${tg_tag_nick}\" "
           "\"==\\*(\\S+)\\*==*${color:bold}$1${color:-bold}*== "
           "==_(\\S+)_==_${color:underline}$1${color:-underline}_== "
           "==/(\\S+)/==/${color:italic}$1${color:-italic}/\"\n"
           "  hide nicklist bar on small terminals:\n"
           "    /trigger add resize_small signal signal_sigwinch "
           "\"${info:term_width} < 100\" \"\" \"/bar hide nicklist\"\n"
           "    /trigger add resize_big signal signal_sigwinch "
           "\"${info:term_width} >= 100\" \"\" \"/bar show nicklist\"\n"
           "  silently save config each hour:\n"
           "    /trigger add cfgsave timer 3600000;0;0 \"\" \"\" \"/mute /save\""),
        "list|listfull|listdefault"
        " || add %(trigger_names) %(trigger_hooks) %(trigger_hook_arguments) "
        "%(trigger_hook_condition) %(trigger_hook_regex) "
        "%(trigger_hook_command) %(trigger_hook_rc)"
        " || addinput %(trigger_hooks)"
        " || set %(trigger_names) %(trigger_options)|name %(trigger_option_value)"
        " || rename %(trigger_names) %(trigger_names)"
        " || enable|disable|toggle|restart|del %(trigger_names)|-all %(trigger_names)|%*"
        " || default"
        " || monitor",
        &trigger_command_trigger, NULL);
}
