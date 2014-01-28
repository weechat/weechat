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
 * Set "enabled" value in a trigger.
 *
 * Argument "enable" can be:
 *   -1: toggle trigger
 *    0: disable trigger
 *    1: enable trigger
 */

void
trigger_command_set_enabled (struct t_trigger *trigger, int enable)
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

/*
 * Callback for command "/trigger": manage triggers.
 */

int
trigger_command_trigger (void *data, struct t_gui_buffer *buffer, int argc,
                         char **argv, char **argv_eol)
{
    struct t_trigger *ptr_trigger;
    const char *option;
    char *name, *value;
    int i, type, count, index_option, enable;

    /* make C compiler happy */
    (void) data;
    (void) buffer;

    /* list all triggers */
    if ((argc == 1)
        || ((argc == 2) && (weechat_strcasecmp (argv[1], "list") == 0)))
    {
        if (triggers)
        {
            weechat_printf_tags (NULL, "no_trigger", "");
            weechat_printf_tags (NULL, "no_trigger", _("List of triggers:"));
            for (ptr_trigger = triggers; ptr_trigger;
                 ptr_trigger = ptr_trigger->next_trigger)
            {
                weechat_printf_tags (NULL, "no_trigger",
                                     "  %s: %s, \"%s\" (%d hooks, %d/%d) %s",
                                     ptr_trigger->name,
                                     weechat_config_string (ptr_trigger->options[TRIGGER_OPTION_HOOK]),
                                     weechat_config_string (ptr_trigger->options[TRIGGER_OPTION_ARGUMENTS]),
                                     ptr_trigger->hooks_count,
                                     ptr_trigger->hook_count_cb,
                                     ptr_trigger->hook_count_cmd,
                                     weechat_config_boolean (ptr_trigger->options[TRIGGER_OPTION_ENABLED]) ?
                                     "" : _("(disabled)"));
                option = weechat_config_string (ptr_trigger->options[TRIGGER_OPTION_CONDITIONS]);
                if (option && option[0])
                {
                    weechat_printf_tags (NULL, "no_trigger",
                                         "      conditions: \"%s\"", option);
                }
                for (i = 0; i < ptr_trigger->regex_count; i++)
                {
                    weechat_printf_tags (NULL, "no_trigger",
                                         "      regex %d%s%s%s: "
                                         "%s\"%s%s%s\" --> \"%s%s%s\"",
                                         i + 1,
                                         (ptr_trigger->regex[i].variable) ? " (" : "",
                                         (ptr_trigger->regex[i].variable) ? ptr_trigger->regex[i].variable : "",
                                         (ptr_trigger->regex[i].variable) ? ")" : "",
                                         weechat_color ("chat_delimiters"),
                                         weechat_color (weechat_config_string (trigger_config_color_regex)),
                                         ptr_trigger->regex[i].str_regex,
                                         weechat_color ("chat_delimiters"),
                                         weechat_color (weechat_config_string (trigger_config_color_replace)),
                                         ptr_trigger->regex[i].replace,
                                         weechat_color ("chat_delimiters"));
                }
                option = weechat_config_string (ptr_trigger->options[TRIGGER_OPTION_COMMAND]);
                if (option && option[0])
                {
                    weechat_printf_tags (NULL, "no_trigger",
                                         "      command: \"%s\"", option);
                }
            }
        }
        else
        {
            weechat_printf_tags (NULL, "no_trigger", _("No trigger defined"));
        }
        return WEECHAT_RC_OK;
    }

    /* add a trigger */
    if (weechat_strcasecmp (argv[1], "add") == 0)
    {
        if (argc < 4)
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sError: missing arguments for \"%s\" "
                                   "command"),
                                 weechat_prefix ("error"), "trigger");
            return WEECHAT_RC_OK;
        }
        if (argv[2][0] == '-')
        {
            weechat_printf (NULL,
                            _("%s%s: name can not start with \"-\""),
                            weechat_prefix ("error"), TRIGGER_PLUGIN_NAME);
            return WEECHAT_RC_OK;
        }
        type = trigger_search_hook_type (argv[3]);
        if (type < 0)
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%s%s: invalid hook type \"%s\""),
                                 weechat_prefix ("error"), TRIGGER_PLUGIN_NAME,
                                 argv[3]);
            return WEECHAT_RC_OK;
        }
        ptr_trigger = trigger_alloc (argv[2]);
        if (!ptr_trigger)
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%s%s: error creating trigger \"%s\""),
                                 weechat_prefix ("error"), TRIGGER_PLUGIN_NAME,
                                 argv[2]);
            return WEECHAT_RC_OK;
        }
        if (trigger_new (argv[2], "on", argv[3],
                         (argc > 4) ? argv_eol[4] : "",
                         "", "", "", "ok"))
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("Trigger \"%s\" created"), argv[2]);
        }
        else
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sError: failed to create trigger \"%s\""),
                                 weechat_prefix ("error"), argv[2]);
        }
        return WEECHAT_RC_OK;
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
            return WEECHAT_RC_OK;
        }
        ptr_trigger = trigger_search (argv[2]);
        if (!ptr_trigger)
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sTrigger \"%s\" not found"),
                                 weechat_prefix ("error"), argv[2]);
            return WEECHAT_RC_OK;
        }
        if (weechat_strcasecmp (argv[3], "name") == 0)
        {
            value = weechat_string_remove_quotes (argv[4], "'\"");
            name = strdup (ptr_trigger->name);
            if (value && name)
            {
                if (trigger_rename (ptr_trigger, value))
                {
                    weechat_printf_tags (NULL, "no_trigger",
                                         _("Trigger \"%s\" renamed to \"%s\""),
                                         name, ptr_trigger->name);
                }
            }
            if (name)
                free (name);
            if (value)
                free (value);
            return WEECHAT_RC_OK;
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
                                     _("%sTrigger option \"%s\" not found"),
                                     weechat_prefix ("error"), argv[3]);
            }
            free (value);
        }
        return WEECHAT_RC_OK;
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
            return WEECHAT_RC_OK;
        }
        ptr_trigger = trigger_search (argv[2]);
        if (!ptr_trigger)
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sTrigger \"%s\" not found"),
                                 weechat_prefix ("error"), argv[2]);
            return WEECHAT_RC_OK;
        }
        name = strdup (ptr_trigger->name);
        if (name)
        {
            if (trigger_rename (ptr_trigger, argv[3]))
            {
                weechat_printf_tags (NULL, "no_trigger",
                                     _("Trigger \"%s\" renamed to \"%s\""),
                                     name, ptr_trigger->name);
            }
            free (name);
        }
        return WEECHAT_RC_OK;
    }

    /* enable/disable/toggle trigger(s) */
    if ((weechat_strcasecmp (argv[1], "enable") == 0)
        || (weechat_strcasecmp (argv[1], "disable") == 0)
        || (weechat_strcasecmp (argv[1], "toggle") == 0))
    {
        if (argc < 3)
        {
            weechat_printf_tags (NULL, "no_trigger",
                                 _("%sError: missing arguments for \"%s\" "
                                   "command"),
                                 weechat_prefix ("error"), "trigger");
            return WEECHAT_RC_OK;
        }
        enable = -1;
        if (weechat_strcasecmp (argv[1], "enable") == 0)
            enable = 1;
        else if (weechat_strcasecmp (argv[1], "disable") == 0)
            enable = 0;
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
        return WEECHAT_RC_OK;
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
            return WEECHAT_RC_OK;
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
        return WEECHAT_RC_OK;
    }

    /* open the trigger monitor buffer */
    if (weechat_strcasecmp (argv[1], "monitor") == 0)
    {
        trigger_buffer_open (1);
        return WEECHAT_RC_OK;
    }

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
        N_("list"
           " || add <name> <hook> [<arguments>]"
           " || set <name> <option> <value>"
           " || rename <name> <new_name>"
           " || enable|disable|toggle <name>|-all [<name>...]"
           " || del <name>|-all [<name>...]"
           " || monitor"),
        N_("      add: add a trigger\n"
           "     name: name of trigger\n"
           "     hook: signal, hsignal, modifier, print, timer\n"
           "arguments: arguments for the hook, depending on hook:\n"
           "           signal: name of signal\n"
           "           hsignal: name of hsignal\n"
           "           modifier: name of modifier\n"
           "           print: buffer, tags, message, strip_colors\n"
           "           timer: interval, align_second, max_calls\n"
           "      set: set an option in a trigger\n"
           "   option: name of option: name, hook, arguments, conditions, regex, "
           "command, return_code\n"
           "           (for help on option, you can do /help "
           "trigger.trigger.<name>.<option>)\n"
           "    value: new value for the option\n"
           "   rename: rename a trigger\n"
           "   enable: enable trigger(s)\n"
           "  disable: disable trigger(s)\n"
           "   toggle: toggle trigger(s)\n"
           "      del: delete a trigger\n"
           "     -all: do action on all triggers\n"
           "  monitor: open the trigger monitor buffer\n"
           "\n"
           "When a trigger callback is called, following actions are performed, "
           "in this order:\n"
           "  1. if no regex/command is defined, exit\n"
           "  2. check conditions; if false, exit\n"
           "  3. replace text using regex (if defined in trigger)\n"
           "  4. execute command (if defined in trigger)\n"
           "Note: on steps 1 and 2, the exit is made with the return code "
           "defined in trigger (or NULL for a modifier).\n"
           "\n"
           "Examples:\n"
           "  send alert (BEL) on highlight or private message:\n"
           "    /trigger add beep print\n"
           "    /trigger set beep conditions ${tg_highlight} || ${tg_msg_pv}\n"
           "    /trigger set beep command /print -stderr \\a\n"
           "  replace password with '*' in /oper command (in command line and "
           "command history):\n"
           "    /trigger add oper modifier input_text_display;history_add\n"
           "    /trigger set oper regex ==^(/oper +\\S+ +)(.*)==\\1\\*2\n"
           "  add text attributes in *bold*, _underline_ and /italic/:\n"
           "    /trigger add effects modifier weechat_print\n"
           "    /trigger set effects regex "
           "==\\*(\\S+)\\*==*${color:bold}\\1${color:-bold}*== "
           "==_(\\S+)_==_${color:underline}\\1${color:-underline}_== "
           "==/(\\S+)/==/${color:italic}\\1${color:-italic}/"),
        "list"
        " || add %(trigger_names) %(trigger_hooks)"
        " || set %(trigger_names) %(trigger_options)|name %(trigger_option_value)"
        " || rename %(trigger_names) %(trigger_names)"
        " || enable|disable|toggle|del %(trigger_names)|-all %(trigger_names)|%*"
        " || monitor",
        &trigger_command_trigger, NULL);
}
