/*
 * trigger.c - trigger plugin for WeeChat
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
#include <string.h>
#include <ctype.h>
#include <regex.h>

#include "../weechat-plugin.h"
#include "trigger.h"
#include "trigger-buffer.h"
#include "trigger-callback.h"
#include "trigger-command.h"
#include "trigger-completion.h"
#include "trigger-config.h"


WEECHAT_PLUGIN_NAME(TRIGGER_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Text replacement and command execution on events triggered by WeeChat/plugins"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(TRIGGER_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_trigger_plugin = NULL;

char *trigger_option_string[TRIGGER_NUM_OPTIONS] =
{ "enabled", "hook", "arguments", "conditions", "regex", "command",
  "return_code", "post_action" };
char *trigger_option_default[TRIGGER_NUM_OPTIONS] =
{ "on", "signal", "", "", "", "", "ok", "none" };

char *trigger_hook_type_string[TRIGGER_NUM_HOOK_TYPES] =
{ "signal", "hsignal", "modifier", "line", "print", "command", "command_run",
  "timer", "config", "focus", "info", "info_hashtable" };
char *trigger_hook_option_values =
    "signal|hsignal|modifier|line|print|command|command_run|timer|config|"
    "focus|info|info_hashtable";
char *trigger_hook_default_arguments[TRIGGER_NUM_HOOK_TYPES] =
{ "xxx", "xxx", "xxx", "", "", "cmd;desc;args;args_desc;%(buffers_names)",
  "/cmd", "60000;0;0", "xxx", "chat", "xxx", "xxx" };
char *trigger_hook_default_rc[TRIGGER_NUM_HOOK_TYPES] =
{ "ok,ok_eat,error", "ok,ok_eat,error", "", "", "ok,error", "ok,error",
  "ok,ok_eat,error", "ok", "ok", "", "", "" };

char trigger_regex_command[TRIGGER_NUM_REGEX_COMMANDS] =
{ 's', 'y' };
char *trigger_hook_regex_default_var[TRIGGER_NUM_HOOK_TYPES] =
{ "tg_signal_data", "", "tg_string", "message", "tg_message", "tg_argv_eol1",
  "tg_command", "tg_remaining_calls", "tg_value", "", "tg_info", "" };

char *trigger_return_code_string[TRIGGER_NUM_RETURN_CODES] =
{ "ok", "ok_eat", "error" };
int trigger_return_code[TRIGGER_NUM_RETURN_CODES] =
{ WEECHAT_RC_OK, WEECHAT_RC_OK_EAT, WEECHAT_RC_ERROR };

char *trigger_post_action_string[TRIGGER_NUM_POST_ACTIONS] =
{ "none", "disable", "delete" };

struct t_trigger *triggers = NULL;          /* first trigger                */
struct t_trigger *last_trigger = NULL;      /* last trigger                 */
int triggers_count = 0;                     /* number of triggers           */

struct t_trigger *triggers_temp = NULL;     /* first temporary trigger      */
struct t_trigger *last_trigger_temp = NULL; /* last temporary trigger       */

int trigger_enabled = 1;                    /* 0 if triggers are disabled   */


/*
 * Searches for a trigger option name.
 *
 * Returns index of option in enum t_trigger_option, -1 if not found.
 */

int
trigger_search_option (const char *option_name)
{
    int i;

    if (!option_name)
        return -1;

    for (i = 0; i < TRIGGER_NUM_OPTIONS; i++)
    {
        if (strcmp (trigger_option_string[i], option_name) == 0)
            return i;
    }

    /* trigger option not found */
    return -1;
}

/*
 * Searches for trigger hook type.
 *
 * Returns index of hook type in enum t_trigger_hook_type, -1 if not found.
 */

int
trigger_search_hook_type (const char *type)
{
    int i;

    if (!type)
        return -1;

    for (i = 0; i < TRIGGER_NUM_HOOK_TYPES; i++)
    {
        if (strcmp (trigger_hook_type_string[i], type) == 0)
            return i;
    }

    /* hook type not found */
    return -1;
}

/*
 * Searches for a regex command.
 *
 * Returns index of option in enum t_trigger_regex_command, -1 if not found.
 */

int
trigger_search_regex_command (char command)
{
    int i;

    for (i = 0; i < TRIGGER_NUM_REGEX_COMMANDS; i++)
    {
        if (trigger_regex_command[i] == command)
            return i;
    }

    /* regex command not found */
    return -1;
}

/*
 * Searches for trigger return code.
 *
 * Returns index of return code in enum t_trigger_return_code, -1 if not found.
 */

int
trigger_search_return_code (const char *return_code)
{
    int i;

    if (!return_code)
        return -1;

    for (i = 0; i < TRIGGER_NUM_RETURN_CODES; i++)
    {
        if (strcmp (trigger_return_code_string[i], return_code) == 0)
            return i;
    }

    /* return code not found */
    return -1;
}

/*
 * Searches for trigger post action.
 *
 * Returns index of post action in enum t_trigger_post_action, -1 if not found.
 */

int
trigger_search_post_action (const char *post_action)
{
    int i;

    if (!post_action)
        return -1;

    for (i = 0; i < TRIGGER_NUM_POST_ACTIONS; i++)
    {
        if (strcmp (trigger_post_action_string[i], post_action) == 0)
            return i;
    }

    /* post action not found */
    return -1;
}

/*
 * Searches for a trigger by name.
 *
 * Returns pointer to trigger found, NULL if not found.
 */

struct t_trigger *
trigger_search (const char *name)
{
    struct t_trigger *ptr_trigger;

    if (!name || !name[0])
        return NULL;

    for (ptr_trigger = triggers; ptr_trigger;
         ptr_trigger = ptr_trigger->next_trigger)
    {
        if (strcmp (ptr_trigger->name, name) == 0)
            return ptr_trigger;
    }

    /* trigger not found */
    return NULL;
}

/*
 * Searches for a trigger with a pointer to a trigger option.
 *
 * Returns pointer to trigger found, NULL if not found.
 */

struct t_trigger *
trigger_search_with_option (struct t_config_option *option)
{
    const char *ptr_name;
    char *pos_option;
    struct t_trigger *ptr_trigger;
    int num_chars;

    if (!option)
        return NULL;

    ptr_name = weechat_hdata_string (weechat_hdata_get ("config_option"),
                                     option, "name");
    if (!ptr_name)
        return NULL;

    pos_option = strchr (ptr_name, '.');
    if (!pos_option)
        return NULL;

    num_chars = weechat_utf8_pos (ptr_name, pos_option - ptr_name);

    for (ptr_trigger = triggers; ptr_trigger;
         ptr_trigger = ptr_trigger->next_trigger)
    {
        if (weechat_strncmp (ptr_trigger->name, ptr_name, num_chars) == 0)
            break;
    }

    return ptr_trigger;
}

/*
 * Unhooks things hooked in a trigger.
 */

void
trigger_unhook (struct t_trigger *trigger)
{
    int i;

    if (trigger->hooks)
    {
        for (i = 0; i < trigger->hooks_count; i++)
        {
            if (trigger->hooks[i])
                weechat_unhook (trigger->hooks[i]);
        }
        free (trigger->hooks);
        trigger->hooks = NULL;
        trigger->hooks_count = 0;
    }
    trigger->hook_count_cb = 0;
    trigger->hook_count_cmd = 0;
    if (trigger->hook_print_buffers)
    {
        free (trigger->hook_print_buffers);
        trigger->hook_print_buffers = NULL;
    }
}

/*
 * Creates hook(s) in a trigger.
 */

void
trigger_hook (struct t_trigger *trigger)
{
    char **argv, **argv_eol, *buffer_type, *buffer_name, *tags, *message;
    char *error1, *error2, *error3;
    char *eval_desc, *eval_args, *eval_desc_args, *eval_completion;
    int i, argc, strip_colors;
    long interval, align_second, max_calls;
    struct t_hashtable *extra_vars;

    if (!weechat_config_boolean (trigger->options[TRIGGER_OPTION_ENABLED]))
        return;

    trigger_unhook (trigger);

    argv = weechat_string_split (
        weechat_config_string (trigger->options[TRIGGER_OPTION_ARGUMENTS]),
        ";",
        NULL,
        0,
        0,
        &argc);
    argv_eol = weechat_string_split (
        weechat_config_string (trigger->options[TRIGGER_OPTION_ARGUMENTS]),
        ";",
        NULL,
        WEECHAT_STRING_SPLIT_KEEP_EOL,
        0,
        NULL);

    switch (weechat_config_integer (trigger->options[TRIGGER_OPTION_HOOK]))
    {
        case TRIGGER_HOOK_SIGNAL:
            if (argv && (argc >= 1))
            {
                trigger->hooks = malloc (sizeof (trigger->hooks[0]));
                if (trigger->hooks)
                {
                    trigger->hooks_count = 1;
                    trigger->hooks[0] = weechat_hook_signal (
                        weechat_config_string (trigger->options[TRIGGER_OPTION_ARGUMENTS]),
                        &trigger_callback_signal_cb,
                        trigger, NULL);
                }
            }
            break;
        case TRIGGER_HOOK_HSIGNAL:
            if (argv && (argc >= 1))
            {
                trigger->hooks = malloc (sizeof (trigger->hooks[0]));
                if (trigger->hooks)
                {
                    trigger->hooks_count = 1;
                    trigger->hooks[0] = weechat_hook_hsignal (
                        weechat_config_string (trigger->options[TRIGGER_OPTION_ARGUMENTS]),
                        &trigger_callback_hsignal_cb,
                        trigger, NULL);
                }
            }
            break;
        case TRIGGER_HOOK_MODIFIER:
            if (argv && (argc >= 1))
            {
                trigger->hooks = malloc (argc * sizeof (trigger->hooks[0]));
                if (trigger->hooks)
                {
                    trigger->hooks_count = argc;
                    for (i = 0; i < argc; i++)
                    {
                        trigger->hooks[i] = weechat_hook_modifier (
                            argv[i],
                            &trigger_callback_modifier_cb,
                            trigger, NULL);
                    }
                }
            }
            break;
        case TRIGGER_HOOK_LINE:
            buffer_type = NULL;
            buffer_name = NULL;
            tags = NULL;
            if (argv && (argc >= 1))
            {
                buffer_type = argv[0];
                if ((argc >= 2) && (strcmp (argv[1], "*") != 0))
                    buffer_name = argv[1];
                if ((argc >= 3) && (strcmp (argv[2], "*") != 0))
                    tags = argv[2];
            }
            trigger->hooks = malloc (sizeof (trigger->hooks[0]));
            if (trigger->hooks)
            {
                trigger->hooks_count = 1;
                trigger->hooks[0] = weechat_hook_line (
                    buffer_type,
                    buffer_name,
                    tags,
                    &trigger_callback_line_cb,
                    trigger, NULL);
            }
            break;
        case TRIGGER_HOOK_PRINT:
            tags = NULL;
            message = NULL;
            strip_colors = 0;
            if (argv && (argc >= 1))
            {
                if (strcmp (argv[0], "*") != 0)
                    trigger->hook_print_buffers = strdup (argv[0]);
                if ((argc >= 2) && (strcmp (argv[1], "*") != 0))
                    tags = argv[1];
                if ((argc >= 3) && (strcmp (argv[2], "*") != 0))
                    message = argv[2];
                if (argc >= 4)
                    strip_colors = (strcmp (argv[3], "0") != 0) ? 1 : 0;
            }
            trigger->hooks = malloc (sizeof (trigger->hooks[0]));
            if (trigger->hooks)
            {
                trigger->hooks_count = 1;
                trigger->hooks[0] = weechat_hook_print (
                    NULL,
                    tags,
                    message,
                    strip_colors,
                    &trigger_callback_print_cb,
                    trigger, NULL);
            }
            break;
        case TRIGGER_HOOK_COMMAND:
            if (argv && (argc >= 1))
            {
                trigger->hooks = malloc (sizeof (trigger->hooks[0]));
                if (trigger->hooks)
                {
                    extra_vars = weechat_hashtable_new (32,
                                                        WEECHAT_HASHTABLE_STRING,
                                                        WEECHAT_HASHTABLE_STRING,
                                                        NULL, NULL);
                    if (extra_vars)
                    {
                        weechat_hashtable_set (extra_vars, "tg_trigger_name",
                                               trigger->name);
                    }
                    trigger->hooks_count = 1;
                    eval_desc = (argc > 1) ? weechat_string_eval_expression (
                        argv[1], NULL, extra_vars, NULL) : NULL;
                    eval_args = (argc > 2) ? weechat_string_eval_expression (
                        argv[2], NULL, extra_vars, NULL) : NULL;
                    eval_desc_args = (argc > 3) ? weechat_string_eval_expression (
                        argv[3], NULL, extra_vars, NULL) : NULL;
                    eval_completion = (argc > 4) ? weechat_string_eval_expression (
                        argv[4], NULL, extra_vars, NULL) : NULL;
                    trigger->hooks[0] = weechat_hook_command (
                        argv[0],  /* command */
                        (eval_desc) ? eval_desc : "",
                        (eval_args) ? eval_args : "",
                        (eval_desc_args) ? eval_desc_args : "",
                        (eval_completion) ? eval_completion : "",
                        &trigger_callback_command_cb,
                        trigger, NULL);
                    if (eval_desc)
                        free (eval_desc);
                    if (eval_args)
                        free (eval_args);
                    if (eval_desc_args)
                        free (eval_desc_args);
                    if (eval_completion)
                        free (eval_completion);
                    if (extra_vars)
                        weechat_hashtable_free (extra_vars);
                }
            }
            break;
        case TRIGGER_HOOK_COMMAND_RUN:
            if (argv && (argc >= 1))
            {
                trigger->hooks = malloc (argc * sizeof (trigger->hooks[0]));
                if (trigger->hooks)
                {
                    trigger->hooks_count = argc;
                    for (i = 0; i < argc; i++)
                    {
                        trigger->hooks[i] = weechat_hook_command_run (
                            argv[i],
                            &trigger_callback_command_run_cb,
                            trigger, NULL);
                    }
                }
            }
            break;
        case TRIGGER_HOOK_TIMER:
            if (argv && (argc >= 1))
            {
                error1 = NULL;
                error2 = NULL;
                error3 = NULL;
                interval = strtol (argv[0], &error1, 10);
                align_second = strtol ((argc >= 2) ? argv[1] : "0", &error2, 10);
                max_calls = strtol ((argc >= 3) ? argv[2] : "0", &error3, 10);
                if (error1 && !error1[0]
                    && error2 && !error2[0]
                    && error3 && !error3[0]
                    && (interval > 0)
                    && (align_second >= 0)
                    && (max_calls >= 0))
                {
                    trigger->hooks = malloc (sizeof (trigger->hooks[0]));
                    if (trigger->hooks)
                    {
                        trigger->hooks_count = 1;
                        trigger->hooks[0] = weechat_hook_timer (
                            interval,
                            (int)align_second,
                            (int)max_calls,
                            &trigger_callback_timer_cb,
                            trigger, NULL);
                    }
                }
            }
            break;
        case TRIGGER_HOOK_CONFIG:
            if (argv && (argc >= 1))
            {
                trigger->hooks = malloc (argc * sizeof (trigger->hooks[0]));
                if (trigger->hooks)
                {
                    trigger->hooks_count = argc;
                    for (i = 0; i < argc; i++)
                    {
                        trigger->hooks[i] = weechat_hook_config (
                            argv[i],
                            &trigger_callback_config_cb,
                            trigger, NULL);
                    }
                }
            }
            break;
        case TRIGGER_HOOK_FOCUS:
            if (argv && (argc >= 1))
            {
                trigger->hooks = malloc (argc * sizeof (trigger->hooks[0]));
                if (trigger->hooks)
                {
                    trigger->hooks_count = argc;
                    for (i = 0; i < argc; i++)
                    {
                        trigger->hooks[i] = weechat_hook_focus (
                            argv[i],
                            &trigger_callback_focus_cb,
                            trigger, NULL);
                    }
                }
            }
            break;
        case TRIGGER_HOOK_INFO:
            if (argv && (argc >= 1))
            {
                trigger->hooks = malloc (argc * sizeof (trigger->hooks[0]));
                if (trigger->hooks)
                {
                    trigger->hooks_count = argc;
                    for (i = 0; i < argc; i++)
                    {
                        trigger->hooks[i] = weechat_hook_info (
                            argv[i],
                            NULL,
                            NULL,
                            &trigger_callback_info_cb,
                            trigger, NULL);
                    }
                }
            }
            break;
        case TRIGGER_HOOK_INFO_HASHTABLE:
            if (argv && (argc >= 1))
            {
                trigger->hooks = malloc (argc * sizeof (trigger->hooks[0]));
                if (trigger->hooks)
                {
                    trigger->hooks_count = argc;
                    for (i = 0; i < argc; i++)
                    {
                        trigger->hooks[i] = weechat_hook_info_hashtable (
                            argv[i],
                            NULL,
                            NULL,
                            NULL,
                            &trigger_callback_info_hashtable_cb,
                            trigger, NULL);
                    }
                }
            }
            break;
    }

    if (!trigger->hooks)
    {
        weechat_printf (NULL,
                        _("%s%s: unable to create hook for trigger \"%s\" "
                          "(bad arguments)"),
                        weechat_prefix ("error"), TRIGGER_PLUGIN_NAME,
                        trigger->name);
    }

    if (argv)
        weechat_string_free_split (argv);
    if (argv_eol)
        weechat_string_free_split (argv_eol);
}

/*
 * Frees all the regex in a trigger.
 */

void
trigger_regex_free (int *regex_count, struct t_trigger_regex **regex)
{
    int i;

    if (!regex_count || !regex)
        return;

    if (*regex_count > 0)
    {
        for (i = 0; i < *regex_count; i++)
        {
            if ((*regex)[i].variable)
                free ((*regex)[i].variable);
            if ((*regex)[i].str_regex)
                free ((*regex)[i].str_regex);
            if ((*regex)[i].regex)
            {
                regfree ((*regex)[i].regex);
                free ((*regex)[i].regex);
            }
            if ((*regex)[i].replace)
                free ((*regex)[i].replace);
            if ((*regex)[i].replace_escaped)
                free ((*regex)[i].replace_escaped);
        }
        free (*regex);
        *regex = NULL;
        *regex_count = 0;
    }
}

/*
 * Splits the regex in structures, with regex and replacement text.
 *
 * Returns:
 *    0: OK
 *   -1: format error
 *   -2: regex compilation error
 *   -3: not enough memory
 */

int
trigger_regex_split (const char *str_regex,
                     int *regex_count, struct t_trigger_regex **regex)
{
    const char *ptr_regex, *pos, *pos_replace, *pos_replace_end;
    const char *pos_next_regex;
    char *delimiter, *str_regex_escaped;
    int rc, index, length_delimiter, command;
    struct t_trigger_regex *new_regex;

    rc = 0;
    delimiter = NULL;
    str_regex_escaped = NULL;

    if (!regex_count || !regex)
        goto end;

    /* remove any existing regex */
    trigger_regex_free (regex_count, regex);

    if (!str_regex || !str_regex[0])
        goto end;

    /* min 3 chars, for example: "/a/" */
    if (strlen (str_regex) < 3)
        goto format_error;

    /* parse regular expressions in the option */
    ptr_regex = str_regex;
    while (ptr_regex && ptr_regex[0])
    {
        if (delimiter)
        {
            free (delimiter);
            delimiter = NULL;
        }

        /* extract command */
        if (isalpha ((unsigned char)ptr_regex[0]))
        {
            command = trigger_search_regex_command (ptr_regex[0]);
            if (command < 0)
                goto format_error;
            /* skip the command */
            ptr_regex = weechat_utf8_next_char (ptr_regex);
        }
        else
        {
            /* default command is "s" (replace regex) */
            command = TRIGGER_REGEX_COMMAND_REPLACE;
        }

        /* search the delimiter (which can be more than one char) */
        pos = weechat_utf8_next_char (ptr_regex);
        while (pos[0] && (weechat_string_charcmp (ptr_regex, pos) == 0))
        {
            pos = weechat_utf8_next_char (pos);
        }
        if (!pos[0])
            goto format_error;
        delimiter = weechat_strndup (ptr_regex, pos - ptr_regex);
        if (!delimiter)
            goto memory_error;

        length_delimiter = strlen (delimiter);

        ptr_regex = pos;

        /* search the start of replacement string */
        pos_replace = strstr (ptr_regex, delimiter);
        if (!pos_replace)
            goto format_error;

        /* search the end of replacement string */
        pos_replace_end = strstr (pos_replace + length_delimiter, delimiter);

        new_regex = realloc (*regex,
                             (*regex_count + 1) * sizeof ((*regex)[0]));
        if (!new_regex)
            goto memory_error;

        *regex = new_regex;
        (*regex_count)++;
        index = *regex_count - 1;

        /* initialize new regex */
        (*regex)[index].command = command;
        (*regex)[index].variable = NULL;
        (*regex)[index].str_regex = NULL;
        (*regex)[index].regex = NULL;
        (*regex)[index].replace = NULL;
        (*regex)[index].replace_escaped = NULL;

        /* set string with regex */
        (*regex)[index].str_regex = weechat_strndup (ptr_regex,
                                                     pos_replace - ptr_regex);
        if (!(*regex)[index].str_regex)
            goto memory_error;

        /* set regex (command "s" only) */
        if (command == TRIGGER_REGEX_COMMAND_REPLACE)
        {
            if (str_regex_escaped)
                free (str_regex_escaped);
            str_regex_escaped = weechat_string_convert_escaped_chars ((*regex)[index].str_regex);
            if (!str_regex_escaped)
                goto memory_error;
            (*regex)[index].regex = malloc (sizeof (*(*regex)[index].regex));
            if (!(*regex)[index].regex)
                goto memory_error;
            if (weechat_string_regcomp ((*regex)[index].regex,
                                        str_regex_escaped,
                                        REG_EXTENDED | REG_ICASE) != 0)
            {
                free ((*regex)[index].regex);
                (*regex)[index].regex = NULL;
                goto compile_error;
            }
        }

        /* set replace */
        (*regex)[index].replace = (pos_replace_end) ?
            weechat_strndup (pos_replace + length_delimiter,
                             pos_replace_end - pos_replace - length_delimiter) :
            strdup (pos_replace + length_delimiter);
        if (!(*regex)[index].replace)
            goto memory_error;

        /* set replace_escaped (command "s" only) */
        if (command == TRIGGER_REGEX_COMMAND_REPLACE)
        {
            (*regex)[index].replace_escaped =
                weechat_string_convert_escaped_chars ((*regex)[index].replace);
            if (!(*regex)[index].replace_escaped)
                goto memory_error;
        }

        if (!pos_replace_end)
            break;

        /* set variable (optional) */
        ptr_regex = pos_replace_end + length_delimiter;
        if (!ptr_regex[0])
            break;
        if (ptr_regex[0] == ' ')
            pos_next_regex = ptr_regex;
        else
        {
            pos_next_regex = strchr (ptr_regex, ' ');
            (*regex)[index].variable = (pos_next_regex) ?
                weechat_strndup (ptr_regex, pos_next_regex - ptr_regex) :
                strdup (ptr_regex);
            if (!(*regex)[index].variable)
                goto memory_error;
        }
        if (!pos_next_regex)
            break;

        /* skip spaces before next regex */
        ptr_regex = pos_next_regex + 1;
        while (ptr_regex[0] == ' ')
        {
            ptr_regex++;
        }
    }

    goto end;

format_error:
    rc = -1;
    goto end;

compile_error:
    rc = -2;
    goto end;

memory_error:
    rc = -3;
    goto end;

end:
    if (delimiter)
        free (delimiter);
    if (str_regex_escaped)
        free (str_regex_escaped);
    if (rc < 0)
        trigger_regex_free (regex_count, regex);

    return rc;
}

/*
 * Splits command of a trigger.
 */

void
trigger_split_command (const char *command,
                       int *commands_count, char ***commands)
{
    int i;

    if (!commands_count || !commands)
        return;

    if (*commands)
    {
        weechat_string_free_split (*commands);
        *commands = NULL;
    }
    *commands_count = 0;

    if (command && command[0])
    {
        *commands = weechat_string_split_command (command, ';');
        if (*commands)
        {
            for (i = 0; (*commands)[i]; i++)
            {
            }
            *commands_count = i;
        }
    }
}

/*
 * Checks if a trigger name is valid:
 *   - it must not start with "-"
 *   - it must not have any space/period.
 *
 * Returns:
 *   1: name is valid
 *   0: name is invalid
 */

int
trigger_name_valid (const char *name)
{
    if (!name || !name[0] || (name[0] == '-'))
        return 0;

    /* no spaces allowed */
    if (strchr (name, ' '))
        return 0;

    /* no periods allowed */
    if (strchr (name, '.'))
        return 0;

    /* name is valid */
    return 1;
}

/*
 * Allocates and initializes new trigger structure.
 *
 * Returns pointer to new trigger, NULL if error.
 */

struct t_trigger *
trigger_alloc (const char *name)
{
    struct t_trigger *new_trigger;
    int i;

    if (!trigger_name_valid (name))
        return NULL;

    if (trigger_search (name))
        return NULL;

    new_trigger = malloc (sizeof (*new_trigger));
    if (!new_trigger)
        return NULL;

    new_trigger->name = strdup (name);
    for (i = 0; i < TRIGGER_NUM_OPTIONS; i++)
    {
        new_trigger->options[i] = NULL;
    }
    new_trigger->hooks_count = 0;
    new_trigger->hooks = NULL;
    new_trigger->hook_count_cb = 0;
    new_trigger->hook_count_cmd = 0;
    new_trigger->hook_running = 0;
    new_trigger->hook_print_buffers = NULL;
    new_trigger->regex_count = 0;
    new_trigger->regex = NULL;
    new_trigger->commands_count = 0;
    new_trigger->commands = NULL;
    new_trigger->prev_trigger = NULL;
    new_trigger->next_trigger = NULL;

    return new_trigger;
}

/*
 * Searches for position of trigger in list (to keep triggers sorted by name).
 */

struct t_trigger *
trigger_find_pos (struct t_trigger *trigger, struct t_trigger *list_triggers)
{
    struct t_trigger *ptr_trigger;

    for (ptr_trigger = list_triggers; ptr_trigger;
         ptr_trigger = ptr_trigger->next_trigger)
    {
        if (weechat_strcmp (trigger->name, ptr_trigger->name) < 0)
            return ptr_trigger;
    }

    /* position not found */
    return NULL;
}

/*
 * Adds a trigger in a linked list.
 */

void
trigger_add (struct t_trigger *trigger,
             struct t_trigger **list_triggers,
             struct t_trigger **last_list_trigger)
{
    struct t_trigger *pos_trigger;

    pos_trigger = trigger_find_pos (trigger, *list_triggers);
    if (pos_trigger)
    {
        /* add trigger before "pos_trigger" */
        trigger->prev_trigger = pos_trigger->prev_trigger;
        trigger->next_trigger = pos_trigger;
        if (pos_trigger->prev_trigger)
            (pos_trigger->prev_trigger)->next_trigger = trigger;
        else
            *list_triggers = trigger;
        pos_trigger->prev_trigger = trigger;
    }
    else
    {
        /* add trigger to end of list */
        trigger->prev_trigger = *last_list_trigger;
        trigger->next_trigger = NULL;
        if (*last_list_trigger)
            (*last_list_trigger)->next_trigger = trigger;
        else
            *list_triggers = trigger;
        *last_list_trigger = trigger;
    }
}

/*
 * Creates a new trigger with options.
 *
 * Returns pointer to new trigger, NULL if error.
 */

struct t_trigger *
trigger_new_with_options (const char *name, struct t_config_option **options)
{
    struct t_trigger *new_trigger;
    int i;

    new_trigger = trigger_alloc (name);
    if (!new_trigger)
        return NULL;

    for (i = 0; i < TRIGGER_NUM_OPTIONS; i++)
    {
        new_trigger->options[i] = options[i];
    }
    trigger_add (new_trigger, &triggers, &last_trigger);
    triggers_count++;

    if (trigger_regex_split (weechat_config_string (new_trigger->options[TRIGGER_OPTION_REGEX]),
                             &new_trigger->regex_count,
                             &new_trigger->regex) < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: invalid regular expression in trigger: "
                          "\"%s\""),
                        weechat_prefix ("error"), TRIGGER_PLUGIN_NAME,
                        name);
    }

    trigger_split_command (weechat_config_string (new_trigger->options[TRIGGER_OPTION_COMMAND]),
                           &new_trigger->commands_count,
                           &new_trigger->commands);

    trigger_hook (new_trigger);

    return new_trigger;
}

/*
 * Creates a new trigger.
 *
 * Returns pointer to new trigger, NULL if error.
 */

struct t_trigger *
trigger_new (const char *name, const char *enabled, const char *hook,
             const char *arguments, const char *conditions, const char *regex,
             const char *command, const char *return_code,
             const char *post_action)
{
    struct t_config_option *option[TRIGGER_NUM_OPTIONS];
    const char *value[TRIGGER_NUM_OPTIONS];
    struct t_trigger *new_trigger;
    int i;

    /* look for type */
    if (trigger_search_hook_type (hook) < 0)
        return NULL;

    /* look for return code */
    if (return_code && return_code[0]
        && (trigger_search_return_code (return_code) < 0))
    {
        return NULL;
    }

    /* look for post action */
    if (post_action && post_action[0]
        && (trigger_search_post_action (post_action) < 0))
    {
        return NULL;
    }

    value[TRIGGER_OPTION_ENABLED] = enabled;
    value[TRIGGER_OPTION_HOOK] = hook;
    value[TRIGGER_OPTION_ARGUMENTS] = arguments;
    value[TRIGGER_OPTION_CONDITIONS] = conditions;
    value[TRIGGER_OPTION_REGEX] = regex;
    value[TRIGGER_OPTION_COMMAND] = command;
    value[TRIGGER_OPTION_RETURN_CODE] = return_code;
    value[TRIGGER_OPTION_POST_ACTION] = post_action;

    for (i = 0; i < TRIGGER_NUM_OPTIONS; i++)
    {
        option[i] = trigger_config_create_trigger_option (name, i, value[i]);
    }

    new_trigger = trigger_new_with_options (name, option);
    if (!new_trigger)
    {
        for (i = 0; i < TRIGGER_NUM_OPTIONS; i++)
        {
            weechat_config_option_free (option[i]);
        }
    }

    return new_trigger;
}

/*
 * Creates default triggers.
 */

void
trigger_create_default ()
{
    int i;

    for (i = 0; trigger_config_default_list[i][0]; i++)
    {
        trigger_new (trigger_config_default_list[i][0],   /* name */
                     trigger_config_default_list[i][1],   /* enabled */
                     trigger_config_default_list[i][2],   /* hook */
                     trigger_config_default_list[i][3],   /* arguments */
                     trigger_config_default_list[i][4],   /* conditions */
                     trigger_config_default_list[i][5],   /* regex */
                     trigger_config_default_list[i][6],   /* command */
                     trigger_config_default_list[i][7],   /* return code */
                     trigger_config_default_list[i][8]);  /* post action */
    }
}

/*
 * Renames a trigger.
 *
 * Returns:
 *   1: OK
 *   0: error (trigger not renamed)
 */

int
trigger_rename (struct t_trigger *trigger, const char *name)
{
    int length, i;
    char *option_name;

    if (!trigger || !name || !name[0] || !trigger_name_valid (name)
        || trigger_search (name))
    {
        return 0;
    }

    length = strlen (name) + 64;
    option_name = malloc (length);
    if (!option_name)
        return 0;

    for (i = 0; i < TRIGGER_NUM_OPTIONS; i++)
    {
        if (trigger->options[i])
        {
            snprintf (option_name, length,
                      "%s.%s",
                      name,
                      trigger_option_string[i]);
            weechat_config_option_rename (trigger->options[i], option_name);
        }
    }

    if (trigger->name)
        free (trigger->name);
    trigger->name = strdup (name);

    free (option_name);

    /* re-insert trigger in list (for sorting triggers by name) */
    if (trigger->prev_trigger)
        (trigger->prev_trigger)->next_trigger = trigger->next_trigger;
    else
        triggers = trigger->next_trigger;
    if (trigger->next_trigger)
        (trigger->next_trigger)->prev_trigger = trigger->prev_trigger;
    else
        last_trigger = trigger->prev_trigger;
    trigger_add (trigger, &triggers, &last_trigger);

    return 1;
}

/*
 * Copies a trigger.
 *
 * Returns a pointer to the new trigger, NULL if error.
 */

struct t_trigger *
trigger_copy (struct t_trigger *trigger, const char *name)
{
    if (!trigger || !name || !name[0] || !trigger_name_valid (name)
        || trigger_search (name))
    {
        return NULL;
    }

    return trigger_new (
        name,
        weechat_config_string (trigger->options[TRIGGER_OPTION_ENABLED]),
        weechat_config_string (trigger->options[TRIGGER_OPTION_HOOK]),
        weechat_config_string (trigger->options[TRIGGER_OPTION_ARGUMENTS]),
        weechat_config_string (trigger->options[TRIGGER_OPTION_CONDITIONS]),
        weechat_config_string (trigger->options[TRIGGER_OPTION_REGEX]),
        weechat_config_string (trigger->options[TRIGGER_OPTION_COMMAND]),
        weechat_config_string (trigger->options[TRIGGER_OPTION_RETURN_CODE]),
        weechat_config_string (trigger->options[TRIGGER_OPTION_POST_ACTION]));
}

/*
 * Deletes a trigger.
 */

void
trigger_free (struct t_trigger *trigger)
{
    int i;

    if (!trigger)
        return;

    /* remove trigger from triggers list */
    if (trigger->prev_trigger)
        (trigger->prev_trigger)->next_trigger = trigger->next_trigger;
    if (trigger->next_trigger)
        (trigger->next_trigger)->prev_trigger = trigger->prev_trigger;
    if (triggers == trigger)
        triggers = trigger->next_trigger;
    if (last_trigger == trigger)
        last_trigger = trigger->prev_trigger;

    /* free data */
    trigger_unhook (trigger);
    trigger_regex_free (&trigger->regex_count, &trigger->regex);
    if (trigger->name)
        free (trigger->name);
    for (i = 0; i < TRIGGER_NUM_OPTIONS; i++)
    {
        if (trigger->options[i])
            weechat_config_option_free (trigger->options[i]);
    }
    if (trigger->commands)
        weechat_string_free_split (trigger->commands);

    free (trigger);

    triggers_count--;
}

/*
 * Deletes all triggers.
 */

void
trigger_free_all ()
{
    while (triggers)
    {
        trigger_free (triggers);
    }
}

/*
 * Prints trigger infos in WeeChat log file (usually for crash dump).
 */

void
trigger_print_log ()
{
    struct t_trigger *ptr_trigger;
    int i;

    for (ptr_trigger = triggers; ptr_trigger;
         ptr_trigger = ptr_trigger->next_trigger)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[trigger (addr:0x%lx)]", ptr_trigger);
        weechat_log_printf ("  name. . . . . . . . . . : '%s'",  ptr_trigger->name);
        weechat_log_printf ("  enabled . . . . . . . . : %d",
                            weechat_config_integer (ptr_trigger->options[TRIGGER_OPTION_ENABLED]));
        weechat_log_printf ("  hook . .  . . . . . . . : %d ('%s')",
                            weechat_config_integer (ptr_trigger->options[TRIGGER_OPTION_HOOK]),
                            trigger_hook_type_string[weechat_config_integer (ptr_trigger->options[TRIGGER_OPTION_HOOK])]);
        weechat_log_printf ("  arguments . . . . . . . : '%s'",
                            weechat_config_string (ptr_trigger->options[TRIGGER_OPTION_ARGUMENTS]));
        weechat_log_printf ("  conditions. . . . . . . : '%s'",
                            weechat_config_string (ptr_trigger->options[TRIGGER_OPTION_CONDITIONS]));
        weechat_log_printf ("  regex . . . . . . . . . : '%s'",
                            weechat_config_string (ptr_trigger->options[TRIGGER_OPTION_REGEX]));
        weechat_log_printf ("  command . . . . . . . . : '%s'",
                            weechat_config_string (ptr_trigger->options[TRIGGER_OPTION_COMMAND]));
        weechat_log_printf ("  return_code . . . . . . : %d ('%s')",
                            weechat_config_integer (ptr_trigger->options[TRIGGER_OPTION_RETURN_CODE]),
                            trigger_return_code_string[weechat_config_integer (ptr_trigger->options[TRIGGER_OPTION_RETURN_CODE])]);
        weechat_log_printf ("  post_action . . . . . . : %d ('%s')",
                            weechat_config_integer (ptr_trigger->options[TRIGGER_OPTION_POST_ACTION]),
                            trigger_post_action_string[weechat_config_integer (ptr_trigger->options[TRIGGER_OPTION_POST_ACTION])]);
        weechat_log_printf ("  hooks_count . . . . . . : %d",    ptr_trigger->hooks_count);
        weechat_log_printf ("  hooks . . . . . . . . . : 0x%lx", ptr_trigger->hooks);
        for (i = 0; i < ptr_trigger->hooks_count; i++)
        {
            weechat_log_printf ("    hooks[%03d]. . . . . . : 0x%lx",
                                i, ptr_trigger->hooks[i]);
        }
        weechat_log_printf ("  hook_count_cb . . . . . : %llu",  ptr_trigger->hook_count_cb);
        weechat_log_printf ("  hook_count_cmd. . . . . : %llu",  ptr_trigger->hook_count_cmd);
        weechat_log_printf ("  hook_running. . . . . . : %d",    ptr_trigger->hook_running);
        weechat_log_printf ("  hook_print_buffers. . . : '%s'",  ptr_trigger->hook_print_buffers);
        weechat_log_printf ("  regex_count . . . . . . : %d",    ptr_trigger->regex_count);
        weechat_log_printf ("  regex . . . . . . . . . : 0x%lx", ptr_trigger->regex);
        for (i = 0; i < ptr_trigger->regex_count; i++)
        {
            weechat_log_printf ("    regex[%03d].variable . . . : '%s'",
                                i, ptr_trigger->regex[i].variable);
            weechat_log_printf ("    regex[%03d].str_regex. . . : '%s'",
                                i, ptr_trigger->regex[i].str_regex);
            weechat_log_printf ("    regex[%03d].regex. . . . . : 0x%lx",
                                i, ptr_trigger->regex[i].regex);
            weechat_log_printf ("    regex[%03d].replace. . . . : '%s'",
                                i, ptr_trigger->regex[i].replace);
            weechat_log_printf ("    regex[%03d].replace_escaped: '%s'",
                                i, ptr_trigger->regex[i].replace_escaped);
        }
        weechat_log_printf ("  commands_count. . . . . : %d",    ptr_trigger->commands_count);
        weechat_log_printf ("  commands. . . . . . . . : 0x%lx", ptr_trigger->commands);
        if (ptr_trigger->commands)
        {
            for (i = 0; ptr_trigger->commands[i]; i++)
            {
                weechat_log_printf ("    commands[%03d] . . . . : '%s'",
                                    i, ptr_trigger->commands[i]);
            }
        }
        weechat_log_printf ("  prev_trigger. . . . . . : 0x%lx", ptr_trigger->prev_trigger);
        weechat_log_printf ("  next_trigger. . . . . . : 0x%lx", ptr_trigger->next_trigger);
    }
}

/*
 * Callback for signal "debug_dump".
 */

int
trigger_debug_dump_cb (const void *pointer, void *data,
                       const char *signal, const char *type_data,
                       void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data || (strcmp ((char *)signal_data, TRIGGER_PLUGIN_NAME) == 0))
    {
        weechat_log_printf ("");
        weechat_log_printf ("***** \"%s\" plugin dump *****",
                            weechat_plugin->name);

        trigger_print_log ();

        weechat_log_printf ("");
        weechat_log_printf ("***** End of \"%s\" plugin dump *****",
                            weechat_plugin->name);
    }

    return WEECHAT_RC_OK;
}

/*
 * Initializes trigger plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    trigger_callback_init ();

    trigger_command_init ();

    if (!trigger_config_init ())
        return WEECHAT_RC_ERROR;

    trigger_config_read ();

    /* hook some signals */
    weechat_hook_signal ("debug_dump", &trigger_debug_dump_cb, NULL, NULL);

    /* hook completions */
    trigger_completion_init ();

    if (weechat_trigger_plugin->upgrading)
        trigger_buffer_set_callbacks ();

    return WEECHAT_RC_OK;
}

/*
 * Ends trigger plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    trigger_buffer_end ();
    trigger_config_write ();
    trigger_free_all ();
    trigger_config_free ();
    trigger_callback_end ();

    return WEECHAT_RC_OK;
}
