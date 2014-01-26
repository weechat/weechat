/*
 * trigger-callback.c - callbacks for triggers
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
#include <time.h>

#include "../weechat-plugin.h"
#include "trigger.h"
#include "trigger-buffer.h"


/* one hashtable by hook, used in callback to evaluate "conditions" */
struct t_hashtable *trigger_callback_hashtable_options = NULL;


/*
 * Checks conditions for a trigger.
 *
 * Returns:
 *   1: conditions are true (or no condition set in trigger)
 *   0: conditions are false
 */

int
trigger_callback_check_conditions (struct t_trigger *trigger,
                                   struct t_hashtable *pointers,
                                   struct t_hashtable *extra_vars)
{
    const char *conditions;
    char *value;
    int rc;

    conditions = weechat_config_string (trigger->options[TRIGGER_OPTION_CONDITIONS]);
    if (!conditions || !conditions[0])
        return 1;

    value = weechat_string_eval_expression (conditions,
                                            pointers,
                                            extra_vars,
                                            trigger_callback_hashtable_options);
    rc = (value && (strcmp (value, "1") == 0));
    if (value)
        free (value);

    return rc;
}

/*
 * Replaces text using one or more regex in the trigger.
 *
 * Note: result must be freed after use.
 */

char *
trigger_callback_replace_regex (struct t_trigger *trigger, const char *name,
                                const char *value)
{
    char *temp, *res;
    int i;

    if (trigger->regex_count == 0)
        return strdup (value);

    res = NULL;

    for (i = 0; i < trigger->regex_count; i++)
    {
        temp = weechat_string_replace_regex ((res) ? res : value,
                                             trigger->regex[i].regex,
                                             trigger->regex[i].replace_eval);
        if (!temp)
            return res;
        res = temp;

        /* display debug info on trigger buffer */
        if (trigger_buffer)
        {
            weechat_printf_tags (trigger_buffer, "no_trigger",
                                 "\t  %s (regex %d): \"%s%s\"",
                                 name, i + 1, res, weechat_color ("reset"));
        }
    }

    return res;
}

/*
 * Executes the trigger command(s).
 */

void
trigger_callback_run_command (struct t_trigger *trigger,
                              struct t_gui_buffer *buffer,
                              struct t_hashtable *pointers,
                              struct t_hashtable *extra_vars)
{
    const char *command;
    char *command_eval, **commands, **ptr_command;

    command = weechat_config_string (trigger->options[TRIGGER_OPTION_COMMAND]);
    if (!command || !command[0])
        return;

    command_eval = weechat_string_eval_expression (command, pointers,
                                                   extra_vars, NULL);
    if (command_eval)
    {
        commands = weechat_string_split_command (command_eval, ';');
        if (commands)
        {
            for (ptr_command = commands; *ptr_command; ptr_command++)
            {
                /* display debug info on trigger buffer */
                if (trigger_buffer)
                {
                    weechat_printf_tags (trigger_buffer, "no_trigger",
                                         "\t  running command \"%s\"",
                                         *ptr_command);
                }
                weechat_command (buffer, *ptr_command);
            }
            weechat_string_free_split_command (commands);
            trigger->hook_count_cmd++;
        }
        free (command_eval);
    }
}

/*
 * Callback for a signal hooked.
 */

int
trigger_callback_signal_cb (void *data, const char *signal,
                            const char *type_data, void *signal_data)
{
    struct t_trigger *trigger;
    struct t_hashtable *extra_vars;
    const char *command, *ptr_signal_data;
    char str_data[128], *signal_data2;
    int rc;

    /* get trigger pointer, return immediately if not found or trigger running */
    trigger = (struct t_trigger *)data;
    if (!trigger || trigger->hook_running)
        return WEECHAT_RC_OK;

    trigger->hook_count_cb++;
    trigger->hook_running = 1;

    extra_vars = NULL;

    rc = trigger_return_code[weechat_config_integer (trigger->options[TRIGGER_OPTION_RETURN_CODE])];

    /*
     * in a signal, the only possible action is to execute a command;
     * so if the command is empty, just exit without checking conditions
     */
    command = weechat_config_string (trigger->options[TRIGGER_OPTION_COMMAND]);
    if (!command || !command[0])
        goto end;

    /* create hashtable */
    extra_vars = weechat_hashtable_new (32,
                                        WEECHAT_HASHTABLE_STRING,
                                        WEECHAT_HASHTABLE_STRING,
                                        NULL,
                                        NULL);
    if (!extra_vars)
        goto end;

    /* add data in hashtable used for conditions/replace/command */
    weechat_hashtable_set (extra_vars, "tg_signal", signal);
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        ptr_signal_data = (const char *)signal_data;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        snprintf (str_data, sizeof (str_data),
                  "%d", *((int *)signal_data));
        ptr_signal_data = str_data;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        str_data[0] = '\0';
        if (signal_data)
        {
            snprintf (str_data, sizeof (str_data),
                      "0x%lx", (long unsigned int)signal_data);
        }
        ptr_signal_data = str_data;
    }
    weechat_hashtable_set (extra_vars, "tg_signal_data", ptr_signal_data);

    /* display debug info on trigger buffer */
    if (!trigger_buffer && (weechat_trigger_plugin->debug >= 1))
        trigger_buffer_open (0);
    if (trigger_buffer)
    {
        weechat_printf_tags (trigger_buffer, "no_trigger",
                             "signal\t%s%s",
                             weechat_color ("chat_channel"),
                             trigger->name);
        weechat_printf_tags (trigger_buffer, "no_trigger",
                             "\t  signal_data: \"%s%s\"",
                             ptr_signal_data,
                             weechat_color ("reset"));
        trigger_buffer_display_hashtable ("extra_vars", extra_vars);
    }

    /* check conditions */
    if (!trigger_callback_check_conditions (trigger, NULL, extra_vars))
        goto end;

    /* replace text with regex */
    if (trigger->regex_count > 0)
    {
        signal_data2 = trigger_callback_replace_regex (trigger,
                                                       "tg_signal_data",
                                                       ptr_signal_data);
        if (signal_data2)
        {
            weechat_hashtable_set (extra_vars, "tg_signal_data", signal_data2);
            free (signal_data2);
        }
    }

    /* execute command */
    trigger_callback_run_command (trigger, NULL, NULL, extra_vars);

end:
    if (extra_vars)
        weechat_hashtable_free (extra_vars);
    trigger->hook_running = 0;
    return rc;
}

/*
 * Callback for a hsignal hooked.
 */

int
trigger_callback_hsignal_cb (void *data, const char *signal,
                             struct t_hashtable *hashtable)
{
    (void) data;
    (void) signal;
    (void) hashtable;

    return WEECHAT_RC_OK;
}

/*
 * Callback for a modifier hooked.
 */

char *
trigger_callback_modifier_cb (void *data, const char *modifier,
                              const char *modifier_data, const char *string)
{
    struct t_trigger *trigger;
    struct t_hashtable *extra_vars;
    const char *command;
    char *string_modified, *pos, *pos2, *plugin_name, *buffer_name;
    char *buffer_full_name, *tags;
    int no_trigger, length;

    no_trigger = 0;

    /* get trigger pointer, return immediately if not found or trigger running */
    trigger = (struct t_trigger *)data;
    if (!trigger || trigger->hook_running)
        return NULL;

    trigger->hook_count_cb++;
    trigger->hook_running = 1;

    extra_vars = NULL;
    string_modified = NULL;

    /*
     * in a modifier, the only possible actions are regex or command;
     * so if both are empty, just exit without checking conditions
     */
    command = weechat_config_string (trigger->options[TRIGGER_OPTION_COMMAND]);
    if ((!command || !command[0]) && !trigger->regex)
        goto end;

    /* create hashtable */
    extra_vars = weechat_hashtable_new (32,
                                        WEECHAT_HASHTABLE_STRING,
                                        WEECHAT_HASHTABLE_STRING,
                                        NULL,
                                        NULL);
    if (!extra_vars)
        goto end;

    /* add data in hashtable used for conditions/replace/command */
    weechat_hashtable_set (extra_vars, "tg_modifier", modifier);
    weechat_hashtable_set (extra_vars, "tg_modifier_data", modifier_data);
    weechat_hashtable_set (extra_vars, "tg_string", string);

    /* add special variables for a WeeChat message */
    if (strcmp (modifier, "weechat_print") == 0)
    {
        pos = strchr (modifier_data, ';');
        if (pos)
        {
            plugin_name = weechat_strndup (modifier_data, pos - modifier_data);
            if (plugin_name)
            {
                weechat_hashtable_set (extra_vars, "tg_plugin", plugin_name);
                pos++;
                pos2 = strchr (pos, ';');
                if (pos2)
                {
                    buffer_name = weechat_strndup (pos, pos2 - pos);
                    if (buffer_name)
                    {
                        length = strlen (plugin_name) + 1 + strlen (buffer_name) + 1;
                        buffer_full_name = malloc (length);
                        if (buffer_full_name)
                        {
                            snprintf (buffer_full_name, length,
                                      "%s.%s", plugin_name, buffer_name);
                            weechat_hashtable_set (extra_vars, "tg_buffer",
                                                   buffer_full_name);
                            free (buffer_full_name);
                        }
                        free (buffer_name);
                    }
                    pos2++;
                    if (pos2[0])
                    {
                        length = 1 + strlen (pos2) + 1 + 1;
                        tags = malloc (length);
                        if (tags)
                        {
                            snprintf (tags, length, ",%s,", pos2);
                            weechat_hashtable_set (extra_vars, "tg_tags", tags);
                            if (strstr (tags, ",no_trigger,"))
                                no_trigger = 1;
                            free (tags);
                        }
                    }
                }
                free (plugin_name);
            }
        }
    }

    /*
     * ignore this modifier if "no_trigger" was in tags
     * (for modifier "weechat_print")
     */
    if (no_trigger)
        goto end;

    /* display debug info on trigger buffer */
    if (!trigger_buffer && (weechat_trigger_plugin->debug >= 1))
        trigger_buffer_open (0);
    if (trigger_buffer)
    {
        weechat_printf_tags (trigger_buffer, "no_trigger",
                             "modifier\t%s%s",
                             weechat_color ("chat_channel"),
                             trigger->name);
        weechat_printf_tags (trigger_buffer, "no_trigger",
                             "\t  modifier: %s", modifier);
        weechat_printf_tags (trigger_buffer, "no_trigger",
                             "\t  modifier_data: \"%s%s\"",
                             modifier_data,
                             weechat_color ("reset"));
        trigger_buffer_display_hashtable ("extra_vars", extra_vars);
    }

    /* check conditions */
    if (!trigger_callback_check_conditions (trigger, NULL, extra_vars))
        goto end;

    /* replace text with regex */
    if (trigger->regex_count > 0)
    {
        string_modified = trigger_callback_replace_regex (trigger, "tg_string",
                                                          string);
        if (string_modified)
        {
            weechat_hashtable_set (extra_vars, "tg_string", string_modified);
            if (strcmp (string, string_modified) == 0)
            {
                /* regex did not change the string, ignore it */
                free (string_modified);
                string_modified = NULL;
            }
        }
    }

    /* execute command */
    trigger_callback_run_command (trigger, NULL, NULL, extra_vars);

end:
    if (extra_vars)
        weechat_hashtable_free (extra_vars);
    trigger->hook_running = 0;
    return string_modified;
}

/*
 * Callback for a print hooked.
 */

int
trigger_callback_print_cb  (void *data, struct t_gui_buffer *buffer,
                            time_t date, int tags_count, const char **tags,
                            int displayed, int highlight, const char *prefix,
                            const char *message)
{
    struct t_trigger *trigger;
    struct t_hashtable *pointers, *extra_vars;
    const char *command, *localvar_type;
    char *message2, *str_tags, *str_tags2, str_temp[128];
    int i, rc, length, tag_notify_private;
    struct tm *date_tmp;

    /* get trigger pointer, return immediately if not found or trigger running */
    trigger = (struct t_trigger *)data;
    if (!trigger || trigger->hook_running)
        return WEECHAT_RC_OK;

    trigger->hook_count_cb++;
    trigger->hook_running = 1;

    pointers = NULL;
    extra_vars = NULL;

    rc = trigger_return_code[weechat_config_integer (trigger->options[TRIGGER_OPTION_RETURN_CODE])];

    /* return if the buffer does not match buffers defined in the trigger */
    if (trigger->hook_print_buffers
        && !weechat_buffer_match_list (buffer, trigger->hook_print_buffers))
        goto end;

    /*
     * in a print, the only possible action is to execute a command;
     * so if the command is empty, just exit without checking conditions
     */
    command = weechat_config_string (trigger->options[TRIGGER_OPTION_COMMAND]);
    if (!command || !command[0])
        goto end;

    /* create hashtables */
    pointers = weechat_hashtable_new (32,
                                      WEECHAT_HASHTABLE_STRING,
                                      WEECHAT_HASHTABLE_POINTER,
                                      NULL,
                                      NULL);
    if (!pointers)
        goto end;
    extra_vars = weechat_hashtable_new (32,
                                        WEECHAT_HASHTABLE_STRING,
                                        WEECHAT_HASHTABLE_STRING,
                                        NULL,
                                        NULL);
    if (!extra_vars)
        goto end;

    /* add data in hashtable used for conditions/replace/command */
    weechat_hashtable_set (pointers, "buffer", buffer);
    date_tmp = localtime (&date);
    if (date_tmp)
    {
        strftime (str_temp, sizeof (str_temp), "%Y-%m-%d %H:%M:%S", date_tmp);
        weechat_hashtable_set (extra_vars, "tg_date", str_temp);
    }
    snprintf (str_temp, sizeof (str_temp), "%d", tags_count);
    weechat_hashtable_set (extra_vars, "tg_tags_count", str_temp);
    str_tags = weechat_string_build_with_split_string (tags, ",");
    if (str_tags)
    {
        /* build string with tags and commas around: ",tag1,tag2,tag3," */
        length = 1 + strlen (str_tags) + 1 + 1;
        str_tags2 = malloc (length);
        if (str_tags2)
        {
            snprintf (str_tags2, length, ",%s,", str_tags);
            weechat_hashtable_set (extra_vars, "tg_tags", str_tags2);
            free (str_tags2);
        }
        free (str_tags);
    }
    snprintf (str_temp, sizeof (str_temp), "%d", displayed);
    weechat_hashtable_set (extra_vars, "tg_displayed", str_temp);
    snprintf (str_temp, sizeof (str_temp), "%d", highlight);
    weechat_hashtable_set (extra_vars, "tg_highlight", str_temp);
    weechat_hashtable_set (extra_vars, "tg_prefix", prefix);
    weechat_hashtable_set (extra_vars, "tg_message", message);

    localvar_type = weechat_buffer_get_string (buffer, "localvar_type");
    tag_notify_private = 0;

    for (i = 0; i < tags_count; i++)
    {
        if (strcmp (tags[i], "no_trigger") == 0)
        {
            goto end;
        }
        else if (strcmp (tags[i], "notify_private") == 0)
        {
            tag_notify_private = 1;
        }
    }
    snprintf (str_temp, sizeof (str_temp), "%d",
              (tag_notify_private && localvar_type &&
               (strcmp (localvar_type, "private") == 0)) ? 1 : 0);
    weechat_hashtable_set (extra_vars, "tg_msg_pv", str_temp);

    /* display debug info on trigger buffer */
    if (!trigger_buffer && (weechat_trigger_plugin->debug >= 1))
        trigger_buffer_open (0);
    if (trigger_buffer)
    {
        weechat_printf_tags (trigger_buffer, "no_trigger",
                             "print\t%s%s",
                             weechat_color ("chat_channel"),
                             trigger->name);
        weechat_printf_tags (trigger_buffer, "no_trigger",
                             "\t  buffer: %s",
                             weechat_buffer_get_string (buffer, "full_name"));
        trigger_buffer_display_hashtable ("pointers", pointers);
        trigger_buffer_display_hashtable ("extra_vars", extra_vars);
    }

    /* check conditions */
    if (!trigger_callback_check_conditions (trigger, pointers, extra_vars))
        goto end;

    /* replace text with regex */
    if (trigger->regex_count > 0)
    {
        message2 = trigger_callback_replace_regex (trigger, "tg_message",
                                                   message);
        if (message2)
        {
            weechat_hashtable_set (extra_vars, "tg_message", message);
            free (message2);
        }
    }

    /* execute command */
    trigger_callback_run_command (trigger, buffer, pointers, extra_vars);

end:
    if (pointers)
        weechat_hashtable_free (pointers);
    if (extra_vars)
        weechat_hashtable_free (extra_vars);
    trigger->hook_running = 0;
    return rc;
}

/*
 * Initializes trigger callback.
 */

void
trigger_callback_init ()
{
    trigger_callback_hashtable_options = weechat_hashtable_new (32,
                                                                WEECHAT_HASHTABLE_STRING,
                                                                WEECHAT_HASHTABLE_STRING,
                                                                NULL,
                                                                NULL);
    if (trigger_callback_hashtable_options)
    {
        weechat_hashtable_set (trigger_callback_hashtable_options,
                               "type", "condition");
    }
}

/*
 * Ends trigger callback.
 */

void
trigger_callback_end ()
{
    if (trigger_callback_hashtable_options)
        weechat_hashtable_free (trigger_callback_hashtable_options);
}
