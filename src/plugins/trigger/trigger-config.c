/*
 * trigger-config.c - trigger configuration options (file trigger.conf)
 *
 * Copyright (C) 2014-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <string.h>

#include "../weechat-plugin.h"
#include "trigger.h"
#include "trigger-config.h"


struct t_config_file *trigger_config_file = NULL;

/* sections */

struct t_config_section *trigger_config_section_look = NULL;
struct t_config_section *trigger_config_section_color = NULL;
struct t_config_section *trigger_config_section_trigger = NULL;

/* trigger config, look section */

struct t_config_option *trigger_config_look_enabled = NULL;
struct t_config_option *trigger_config_look_monitor_strip_colors = NULL;

/* trigger config, color section */

struct t_config_option *trigger_config_color_flag_command = NULL;
struct t_config_option *trigger_config_color_flag_conditions = NULL;
struct t_config_option *trigger_config_color_flag_regex = NULL;
struct t_config_option *trigger_config_color_flag_return_code = NULL;
struct t_config_option *trigger_config_color_flag_post_action = NULL;
struct t_config_option *trigger_config_color_identifier = NULL;
struct t_config_option *trigger_config_color_regex = NULL;
struct t_config_option *trigger_config_color_replace = NULL;
struct t_config_option *trigger_config_color_trigger = NULL;
struct t_config_option *trigger_config_color_trigger_disabled = NULL;

char *trigger_config_default_list[][1 + TRIGGER_NUM_OPTIONS] =
{
    /*
     * beep on highlight/private message, on following conditions:
     *   - message is displayed (not filtered)
     *     AND:
     *   - message does not have tag "notify_none"
     *     AND:
     *       - message is a highlight
     *         OR:
     *       - message is a message in a private buffer
     *     AND:
     *   - buffer notify is NOT "none"
     */
    { "beep", "on",
      "print",
      "",
      "${tg_displayed} "
      "&& ${tg_tags} !!- ,notify_none, "
      "&& (${tg_highlight} || ${tg_msg_pv}) "
      "&& ${buffer.notify} > 0",
      "",
      "/print -beep",
      "ok",
      "" },
    /*
     * hide passwords in commands:
     *   - /msg [-server <name>] nickserv id <password>
     *   - /msg [-server <name>] nickserv identify <password>
     *   - /msg [-server <name>] nickserv ghost <nick> <password>
     *   - /msg [-server <name>] nickserv release <nick> <password>
     *   - /msg [-server <name>] nickserv regain <nick> <password>
     *   - /msg [-server <name>] nickserv recover <nick> <password>
     *   - /msg [-server <name>] nickserv setpass <nick> <key> <password>
     *   - /oper <nick> <password>
     *   - /quote pass <password>
     *   - /secure passphrase <passphrase>
     *   - /secure decrypt <passphrase>
     *   - /secure set <name> <value>
     */
    { "cmd_pass", "on",
      "modifier",
      "5000|input_text_display;5000|history_add;5000|irc_command_auth",
      "",
      "s==^("
      "(/(msg|m|quote) +(-server +[^ \\n]+ +)?nickserv +("
      "id|"
      "identify|"
      "set +password|"
      "ghost +[^ \\n]+|"
      "release +[^ \\n]+|"
      "regain +[^ \\n]+|"
      "recover +[^ \\n]+|"
      "setpass +[^ \\n]+"
      ") +)|"
      "/oper +[^ \\n]+ +|"
      "/quote +pass +|"
      "/secure +(passphrase|decrypt|set +[^ \\n]+) +"
      ")"
      "([^\\n]*)"
      "==${re:1}${hide:*,${re:+}}",
      "",
      "",
      "" },
    /*
     * hide passwords in commands:
     *   - /msg [-server <name>] nickserv register <password> <email>
     */
    { "cmd_pass_register", "on",
      "modifier",
      "5000|input_text_display;5000|history_add;5000|irc_command_auth",
      "",
      "s==^(/(msg|m|quote) +(-server +[^ \\n]+ +)?nickserv +register +)"
      "([^ \\n]+)([^\\n]*)"
      "==${re:1}${hide:*,${re:4}}${re:5}",
      "",
      "",
      "" },
    /*
     * hide password in IRC auth message displayed (message received from
     * server after the user issued the command):
     *   - id <password>
     *   - identify <password>
     *   - set password <password>
     *   - register <password>
     *   - ghost <nick> <password>
     *   - release <nick> <password>
     *   - regain <nick> <password>
     *   - recover <nick> <password>
     */
    { "msg_auth", "on",
      "modifier",
      "5000|irc_message_auth",
      "",
      "s==^(.*("
      "id|"
      "identify|"
      "set +password|"
      "register|"
      "ghost +[^ ]+|"
      "release +[^ ]+|"
      "regain +[^ ]+|"
      "recover +[^ ]+"
      ") +)(.*)"
      "==${re:1}${hide:*,${re:+}}",
      "",
      "",
      "" },
    /*
     * hide server password in commands:
     *   - /server add <name> <address> -password=<password>
     *   - /server add <name> <address> -sasl_password=<password>
     *   - /connect <address> -password=<password>
     *   - /connect <address> -sasl_password=<password>
     */
    { "server_pass", "on",
      "modifier",
      "5000|input_text_display;5000|history_add",
      "",
      "s==^(/(server|connect) [^\\n]*-(sasl_)?password=)([^ \\n]+)([^\\n]*)"
      "==${re:1}${hide:*,${re:4}}${re:5}"
      "",
      "",
      "" },
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
};


/*
 * Callback for changes on option "trigger.look.enabled".
 */

void
trigger_config_change_enabled (const void *pointer, void *data,
                               struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    trigger_enabled = weechat_config_boolean (option);
}

/*
 * Callback for changes on option "trigger.trigger.xxx.enabled".
 */

void
trigger_config_change_trigger_enabled (const void *pointer, void *data,
                                       struct t_config_option *option)
{
    struct t_trigger *ptr_trigger;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_trigger = trigger_search_with_option (option);
    if (!ptr_trigger)
        return;

    if (weechat_config_boolean (option))
        trigger_hook (ptr_trigger);
    else
        trigger_unhook (ptr_trigger);
}

/*
 * Callback for changes on option "trigger.trigger.xxx.hook".
 */

void
trigger_config_change_trigger_hook (const void *pointer, void *data,
                                    struct t_config_option *option)
{
    struct t_trigger *ptr_trigger;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_trigger = trigger_search_with_option (option);
    if (!ptr_trigger)
        return;

    if (ptr_trigger->options[TRIGGER_OPTION_ARGUMENTS])
        trigger_hook (ptr_trigger);
}

/*
 * Callback for changes on option "trigger.trigger.xxx.arguments".
 */

void
trigger_config_change_trigger_arguments (const void *pointer, void *data,
                                         struct t_config_option *option)
{
    struct t_trigger *ptr_trigger;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_trigger = trigger_search_with_option (option);
    if (!ptr_trigger)
        return;

    if (ptr_trigger->options[TRIGGER_OPTION_HOOK])
        trigger_hook (ptr_trigger);
}

/*
 * Callback for changes on option "trigger.trigger.xxx.regex".
 */

void
trigger_config_change_trigger_regex (const void *pointer, void *data,
                                     struct t_config_option *option)
{
    struct t_trigger *ptr_trigger;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_trigger = trigger_search_with_option (option);
    if (!ptr_trigger)
        return;

    switch (trigger_regex_split (weechat_config_string (option),
                                 &ptr_trigger->regex_count,
                                 &ptr_trigger->regex))
    {
        case 0: /* OK */
            break;
        case -1: /* format error */
            weechat_printf (NULL,
                            _("%s%s: invalid format for option \"regex\", "
                              "see /help trigger.trigger.%s.regex"),
                            weechat_prefix ("error"), TRIGGER_PLUGIN_NAME,
                            ptr_trigger->name);
            break;
        case -2: /* regex compilation error */
            weechat_printf (NULL,
                            _("%s%s: invalid regular expression in option "
                              "\"regex\", see /help trigger.trigger.%s.regex"),
                            weechat_prefix ("error"), TRIGGER_PLUGIN_NAME,
                            ptr_trigger->name);
            break;
        case -3: /* memory error */
            weechat_printf (NULL,
                            _("%s%s: not enough memory"),
                            weechat_prefix ("error"), TRIGGER_PLUGIN_NAME);
            break;
    }
}

/*
 * Callback for changes on option "trigger.trigger.xxx.command".
 */

void
trigger_config_change_trigger_command (const void *pointer, void *data,
                                       struct t_config_option *option)
{
    struct t_trigger *ptr_trigger;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_trigger = trigger_search_with_option (option);
    if (!ptr_trigger)
        return;

    trigger_split_command (weechat_config_string (option),
                           &ptr_trigger->commands_count,
                           &ptr_trigger->commands);
}

/*
 * Creates an option for a trigger.
 *
 * Returns pointer to new option, NULL if error.
 */

struct t_config_option *
trigger_config_create_trigger_option (const char *trigger_name, int index_option,
                                      const char *value)
{
    struct t_config_option *ptr_option;
    char *option_name;

    ptr_option = NULL;

    if (weechat_asprintf (&option_name,
                          "%s.%s",
                          trigger_name,
                          trigger_option_string[index_option]) < 0)
    {
        return NULL;
    }

    switch (index_option)
    {
        case TRIGGER_OPTION_ENABLED:
            ptr_option = weechat_config_new_option (
                trigger_config_file, trigger_config_section_trigger,
                option_name, "boolean",
                N_("if disabled, the hooks are removed from trigger, so it is "
                   "not called anymore"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, NULL,
                &trigger_config_change_trigger_enabled, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case TRIGGER_OPTION_HOOK:
            ptr_option = weechat_config_new_option (
                trigger_config_file, trigger_config_section_trigger,
                option_name, "enum",
                N_("type of hook used"),
                trigger_hook_option_values,
                0, 0, value, NULL, 0,
                NULL, NULL, NULL,
                &trigger_config_change_trigger_hook, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case TRIGGER_OPTION_ARGUMENTS:
            ptr_option = weechat_config_new_option (
                trigger_config_file, trigger_config_section_trigger,
                option_name, "string",
                N_("arguments for the hook (depend on the hook type, see /help "
                   "trigger)"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, NULL,
                &trigger_config_change_trigger_arguments, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case TRIGGER_OPTION_CONDITIONS:
            ptr_option = weechat_config_new_option (
                trigger_config_file, trigger_config_section_trigger,
                option_name, "string",
                N_("condition(s) for running the command (it is checked in "
                   "hook callback) (note: content is evaluated when trigger is "
                   "run, see /help eval)"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            break;
        case TRIGGER_OPTION_REGEX:
            ptr_option = weechat_config_new_option (
                trigger_config_file, trigger_config_section_trigger,
                option_name, "string",
                N_("replace text with a POSIX extended regular expression (it "
                   "is done only if conditions are OK, and before running the "
                   "command) (note: content is evaluated when trigger is run, "
                   "see /help eval); format is: \"/regex/replace/var\" (var "
                   "is the hashtable variable to replace, it is optional), "
                   "many regex can be separated by a space, for example: "
                   "\"/regex1/replace1/var1 /regex2/replace2/var2\"; escaped "
                   "chars are interpreted in the regex (for example \"\\n\"); "
                   "the separator \"/\" can be replaced by any char (one or "
                   "more identical chars); matching groups can be used in "
                   "replace: ${re:0} to ${re:99}, ${re:+} for last match and "
                   "${hide:c,${re:N}} to replace all chars of group N by "
                   "char 'c'"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, NULL,
                &trigger_config_change_trigger_regex, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case TRIGGER_OPTION_COMMAND:
            ptr_option = weechat_config_new_option (
                trigger_config_file, trigger_config_section_trigger,
                option_name, "string",
                N_("command(s) to run if conditions are OK, after regex "
                   "replacements (many commands can be separated by semicolons)"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, NULL,
                &trigger_config_change_trigger_command, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case TRIGGER_OPTION_RETURN_CODE:
            ptr_option = weechat_config_new_option (
                trigger_config_file, trigger_config_section_trigger,
                option_name, "enum",
                N_("return code for hook callback (see plugin API reference to "
                   "know where ok_eat/error can be used efficiently)"),
                "ok|ok_eat|error", 0, 0, value, NULL, 0,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            break;
        case TRIGGER_OPTION_POST_ACTION:
            ptr_option = weechat_config_new_option (
                trigger_config_file, trigger_config_section_trigger,
                option_name, "enum",
                N_("action to take on the trigger after execution"),
                "none|disable|delete", 0, 0, value, NULL, 0,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            break;
        case TRIGGER_NUM_OPTIONS:
            break;
    }

    free (option_name);

    return ptr_option;
}

/*
 * Creates option for a temporary trigger (when reading configuration file).
 */

void
trigger_config_create_option_temp (struct t_trigger *temp_trigger,
                                   int index_option, const char *value)
{
    struct t_config_option *new_option;

    new_option = trigger_config_create_trigger_option (temp_trigger->name,
                                                       index_option, value);
    if (new_option
        && (index_option >= 0) && (index_option < TRIGGER_NUM_OPTIONS))
    {
        temp_trigger->options[index_option] = new_option;
    }
}

/*
 * Uses temporary triggers (created by reading configuration file).
 */

void
trigger_config_use_temp_triggers (void)
{
    struct t_trigger *ptr_temp_trigger, *next_temp_trigger;
    int i, num_options_ok;

    for (ptr_temp_trigger = triggers_temp; ptr_temp_trigger;
         ptr_temp_trigger = ptr_temp_trigger->next_trigger)
    {
        num_options_ok = 0;
        for (i = 0; i < TRIGGER_NUM_OPTIONS; i++)
        {
            if (!ptr_temp_trigger->options[i])
            {
                ptr_temp_trigger->options[i] =
                    trigger_config_create_trigger_option (ptr_temp_trigger->name,
                                                          i,
                                                          trigger_option_default[i]);
            }
            if (ptr_temp_trigger->options[i])
                num_options_ok++;
        }

        if (num_options_ok == TRIGGER_NUM_OPTIONS)
        {
            trigger_new_with_options (ptr_temp_trigger->name,
                                      ptr_temp_trigger->options);
        }
        else
        {
            for (i = 0; i < TRIGGER_NUM_OPTIONS; i++)
            {
                if (ptr_temp_trigger->options[i])
                {
                    weechat_config_option_free (ptr_temp_trigger->options[i]);
                    ptr_temp_trigger->options[i] = NULL;
                }
            }
        }
    }

    /* free all temporary triggers */
    while (triggers_temp)
    {
        next_temp_trigger = triggers_temp->next_trigger;

        free (triggers_temp->name);
        free (triggers_temp);

        triggers_temp = next_temp_trigger;
    }
    last_trigger_temp = NULL;
}

/*
 * Reads a trigger option in trigger configuration file.
 */

int
trigger_config_trigger_read_cb (const void *pointer, void *data,
                                struct t_config_file *config_file,
                                struct t_config_section *section,
                                const char *option_name, const char *value)
{
    char *pos_option, *trigger_name;
    struct t_trigger *ptr_temp_trigger;
    int index_option;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;
    (void) section;

    if (!option_name)
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    pos_option = strchr (option_name, '.');
    if (!pos_option)
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    trigger_name = weechat_strndup (option_name, pos_option - option_name);
    if (!trigger_name)
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    pos_option++;

    /* search temporary trigger */
    for (ptr_temp_trigger = triggers_temp; ptr_temp_trigger;
         ptr_temp_trigger = ptr_temp_trigger->next_trigger)
    {
        if (strcmp (ptr_temp_trigger->name, trigger_name) == 0)
            break;
    }
    if (!ptr_temp_trigger)
    {
        /* create new temporary trigger */
        ptr_temp_trigger = trigger_alloc (trigger_name);
        if (ptr_temp_trigger)
            trigger_add (ptr_temp_trigger, &triggers_temp, &last_trigger_temp);
    }

    if (ptr_temp_trigger)
    {
        index_option = trigger_search_option (pos_option);
        if (index_option >= 0)
        {
            trigger_config_create_option_temp (ptr_temp_trigger, index_option,
                                               value);
        }
        else
        {
            weechat_printf (NULL,
                            _("%sWarning: unknown option for section \"%s\": "
                              "%s (value: \"%s\")"),
                            weechat_prefix ("error"),
                            TRIGGER_CONFIG_SECTION_TRIGGER,
                            option_name, value);
        }
    }

    free (trigger_name);

    return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * Writes default triggers in trigger configuration file.
 */

int
trigger_config_trigger_write_default_cb (const void *pointer, void *data,
                                         struct t_config_file *config_file,
                                         const char *section_name)
{
    int i, j, quotes;
    char option_name[512];

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (!weechat_config_write_line (config_file, section_name, NULL))
        return WEECHAT_CONFIG_WRITE_ERROR;

    for (i = 0; trigger_config_default_list[i][0]; i++)
    {
        for (j = 0; j < TRIGGER_NUM_OPTIONS; j++)
        {
            snprintf (option_name, sizeof (option_name),
                      "%s.%s",
                      trigger_config_default_list[i][0],
                      trigger_option_string[j]);
            quotes = (j & (TRIGGER_OPTION_ARGUMENTS | TRIGGER_OPTION_CONDITIONS |
                           TRIGGER_OPTION_REGEX | TRIGGER_OPTION_COMMAND));
            if (!weechat_config_write_line (config_file, option_name, "%s%s%s",
                                            (quotes) ? "\"" : "",
                                            trigger_config_default_list[i][j + 1],
                                            (quotes) ? "\"" : ""))
            {
                return WEECHAT_CONFIG_WRITE_ERROR;
            }
        }
    }

    return WEECHAT_CONFIG_WRITE_OK;
}

/*
 * Reloads trigger configuration file.
 */

int
trigger_config_reload_cb (const void *pointer, void *data,
                          struct t_config_file *config_file)
{
    int rc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    trigger_free_all ();

    rc = weechat_config_reload (config_file);

    trigger_config_use_temp_triggers ();

    return rc;
}

/*
 * Initializes trigger configuration file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
trigger_config_init (void)
{
    trigger_config_file = weechat_config_new (
        TRIGGER_CONFIG_PRIO_NAME,
        &trigger_config_reload_cb, NULL, NULL);
    if (!trigger_config_file)
        return 0;

    /* look */
    trigger_config_section_look = weechat_config_new_section (
        trigger_config_file, "look",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (trigger_config_section_look)
    {
        trigger_config_look_enabled = weechat_config_new_option (
            trigger_config_file, trigger_config_section_look,
            "enabled", "boolean",
            N_("enable trigger support"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &trigger_config_change_enabled, NULL, NULL,
            NULL, NULL, NULL);
        trigger_config_look_monitor_strip_colors = weechat_config_new_option (
            trigger_config_file, trigger_config_section_look,
            "monitor_strip_colors", "boolean",
            N_("strip colors in hashtable values displayed on monitor buffer"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    /* color */
    trigger_config_section_color = weechat_config_new_section (
        trigger_config_file, "color",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (trigger_config_section_color)
    {
        trigger_config_color_flag_command = weechat_config_new_option (
            trigger_config_file, trigger_config_section_color,
            "flag_command", "color",
            N_("text color for command flag (in /trigger list)"),
            NULL, 0, 0, "lightgreen", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        trigger_config_color_flag_conditions = weechat_config_new_option (
            trigger_config_file, trigger_config_section_color,
            "flag_conditions", "color",
            N_("text color for conditions flag (in /trigger list)"),
            NULL, 0, 0, "yellow", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        trigger_config_color_flag_regex = weechat_config_new_option (
            trigger_config_file, trigger_config_section_color,
            "flag_regex", "color",
            N_("text color for regex flag (in /trigger list)"),
            NULL, 0, 0, "lightcyan", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        trigger_config_color_flag_return_code = weechat_config_new_option (
            trigger_config_file, trigger_config_section_color,
            "flag_return_code", "color",
            N_("text color for return code flag (in /trigger list)"),
            NULL, 0, 0, "lightmagenta", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        trigger_config_color_flag_post_action = weechat_config_new_option (
            trigger_config_file, trigger_config_section_color,
            "flag_post_action", "color",
            N_("text color for post action flag (in /trigger list)"),
            NULL, 0, 0, "lightblue", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        trigger_config_color_identifier = weechat_config_new_option (
            trigger_config_file, trigger_config_section_color,
            "identifier", "color",
            N_("text color for trigger context identifier in monitor buffer"),
            NULL, 0, 0, "cyan", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        trigger_config_color_regex = weechat_config_new_option (
            trigger_config_file, trigger_config_section_color,
            "regex", "color",
            N_("text color for regular expressions"),
            NULL, 0, 0, "white", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        trigger_config_color_replace = weechat_config_new_option (
            trigger_config_file, trigger_config_section_color,
            "replace", "color",
            N_("text color for replacement text (for regular expressions)"),
            NULL, 0, 0, "cyan", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    /* trigger */
    trigger_config_section_trigger = weechat_config_new_section (
        trigger_config_file,
        TRIGGER_CONFIG_SECTION_TRIGGER,
        0, 0,
        &trigger_config_trigger_read_cb, NULL, NULL,
        NULL, NULL, NULL,
        &trigger_config_trigger_write_default_cb, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);

    return 1;
}

/*
 * Reads trigger configuration file.
 */

int
trigger_config_read (void)
{
    int rc;

    rc = weechat_config_read (trigger_config_file);

    trigger_config_use_temp_triggers ();

    return rc;
}

/*
 * Writes trigger configuration file.
 */

int
trigger_config_write (void)
{
    return weechat_config_write (trigger_config_file);
}

/*
 * Frees trigger configuration.
 */

void
trigger_config_free (void)
{
    weechat_config_free (trigger_config_file);
    trigger_config_file = NULL;
}
