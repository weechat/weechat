/*
 * trigger-callback.c - callbacks for triggers
 *
 * Copyright (C) 2014-2015 Sébastien Helleu <flashcode@flashtux.org>
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
#include "trigger-callback.h"
#include "trigger-buffer.h"


/* hashtable used to evaluate "conditions" */
struct t_hashtable *trigger_callback_hashtable_options_conditions = NULL;

/* hashtable used to replace with regex */
struct t_hashtable *trigger_callback_hashtable_options_regex = NULL;


/*
 * Parses an IRC message.
 *
 * Returns a hashtable with the parsed message, or NULL if error.
 *
 * Note: hashtable must be freed after use.
 */

struct t_hashtable *
trigger_callback_irc_message_parse (const char *irc_message,
                                    const char *irc_server)
{
    struct t_hashtable *hashtable_in, *hashtable_out;

    hashtable_out = NULL;

    hashtable_in = weechat_hashtable_new (32,
                                          WEECHAT_HASHTABLE_STRING,
                                          WEECHAT_HASHTABLE_STRING,
                                          NULL,
                                          NULL);
    if (hashtable_in)
    {
        weechat_hashtable_set (hashtable_in, "message", irc_message);
        weechat_hashtable_set (hashtable_in, "server", irc_server);
        hashtable_out = weechat_info_get_hashtable ("irc_message_parse",
                                                    hashtable_in);
        weechat_hashtable_free (hashtable_in);
    }

    return hashtable_out;
}
/*
 * Sets variables in "extra_vars" hashtable using tags from message.
 *
 * Returns:
 *   0: tag "no_trigger" was in tags, callback must NOT be executed
 *   1: no tag "no_trigger", callback can be executed
 */

int
trigger_callback_set_tags (struct t_gui_buffer *buffer,
                           const char **tags, int tags_count,
                           struct t_hashtable *extra_vars)
{
    const char *localvar_type;
    char str_temp[128];
    int i;

    snprintf (str_temp, sizeof (str_temp), "%d", tags_count);
    weechat_hashtable_set (extra_vars, "tg_tags_count", str_temp);
    localvar_type = (buffer) ?
        weechat_buffer_get_string (buffer, "localvar_type") : NULL;

    for (i = 0; i < tags_count; i++)
    {
        if (strcmp (tags[i], "no_trigger") == 0)
        {
            return 0;
        }
        else if (strncmp (tags[i], "notify_", 7) == 0)
        {
            weechat_hashtable_set (extra_vars, "tg_tag_notify", tags[i] + 7);
            if (strcmp (tags[i] + 7, "none") != 0)
            {
                weechat_hashtable_set (extra_vars, "tg_notify", tags[i] + 7);
                if (strcmp (tags[i] + 7, "private") == 0)
                {
                    snprintf (str_temp, sizeof (str_temp), "%d",
                              (localvar_type
                               && (strcmp (localvar_type, "private") == 0)) ? 1 : 0);
                    weechat_hashtable_set (extra_vars, "tg_msg_pv", str_temp);
                }
            }
        }
        else if (strncmp (tags[i], "nick_", 5) == 0)
        {
            weechat_hashtable_set (extra_vars, "tg_tag_nick", tags[i] + 5);
        }
        else if (strncmp (tags[i], "prefix_nick_", 12) == 0)
        {
            weechat_hashtable_set (extra_vars, "tg_tag_prefix_nick",
                                   tags[i] + 12);
        }
        else if (strncmp (tags[i], "host_", 5) == 0)
        {
            weechat_hashtable_set (extra_vars, "tg_tag_host", tags[i] + 5);
        }
    }

    return 1;
}

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

    value = weechat_string_eval_expression (
        conditions,
        pointers,
        extra_vars,
        trigger_callback_hashtable_options_conditions);
    rc = (value && (strcmp (value, "1") == 0));
    if (value)
        free (value);

    return rc;
}

/*
 * Replaces text using one or more regex in the trigger.
 */

void
trigger_callback_replace_regex (struct t_trigger *trigger,
                                struct t_hashtable *pointers,
                                struct t_hashtable *extra_vars,
                                int display_monitor)
{
    char *value;
    const char *ptr_key, *ptr_value;
    int i, pointers_allocated;

    pointers_allocated = 0;

    if (trigger->regex_count == 0)
        return;

    if (!pointers)
    {
        pointers = weechat_hashtable_new (32,
                                          WEECHAT_HASHTABLE_STRING,
                                          WEECHAT_HASHTABLE_POINTER,
                                          NULL,
                                          NULL);
        if (!pointers)
            return;
        pointers_allocated = 1;
    }

    for (i = 0; i < trigger->regex_count; i++)
    {
        /* if regex is not set (invalid), skip it */
        if (!trigger->regex[i].regex)
            continue;

        ptr_key = (trigger->regex[i].variable) ?
            trigger->regex[i].variable :
            trigger_hook_regex_default_var[weechat_config_integer (trigger->options[TRIGGER_OPTION_HOOK])];
        if (!ptr_key || !ptr_key[0])
        {
            if (trigger_buffer && display_monitor)
            {
                weechat_printf_tags (trigger_buffer, "no_trigger",
                                     "\t  regex %d: %s",
                                     i + 1, _("no variable"));
            }
            continue;
        }

        ptr_value = weechat_hashtable_get (extra_vars, ptr_key);
        if (!ptr_value)
        {
            if (trigger_buffer && display_monitor)
            {
                weechat_printf_tags (trigger_buffer, "no_trigger",
                                     "\t  regex %d (%s): %s",
                                     i + 1, ptr_key, _("empty variable"));
            }
            continue;
        }

        weechat_hashtable_set (pointers, "regex", trigger->regex[i].regex);
        weechat_hashtable_set (trigger_callback_hashtable_options_regex,
                               "regex_replace",
                               trigger->regex[i].replace_escaped);

        value = weechat_string_eval_expression (
            ptr_value,
            pointers,
            extra_vars,
            trigger_callback_hashtable_options_regex);

        if (value)
        {
            /* display debug info on trigger buffer */
            if (trigger_buffer && display_monitor)
            {
                weechat_printf_tags (trigger_buffer, "no_trigger",
                                     "\t  regex %d %s(%s%s%s)%s: "
                                     "%s\"%s%s%s\"",
                                     i + 1,
                                     weechat_color ("chat_delimiters"),
                                     weechat_color ("reset"),
                                     ptr_key,
                                     weechat_color ("chat_delimiters"),
                                     weechat_color ("reset"),
                                     weechat_color ("chat_delimiters"),
                                     weechat_color ("reset"),
                                     value,
                                     weechat_color ("chat_delimiters"));
            }
            weechat_hashtable_set (extra_vars, ptr_key, value);
            free (value);
        }
    }

    if (pointers_allocated)
        weechat_hashtable_free (pointers);
    else
        weechat_hashtable_remove (pointers, "regex");
}

/*
 * Executes the trigger command(s).
 */

void
trigger_callback_run_command (struct t_trigger *trigger,
                              struct t_gui_buffer *buffer,
                              struct t_hashtable *pointers,
                              struct t_hashtable *extra_vars,
                              int display_monitor)
{
    char *command_eval;
    int i;

    if (!trigger->commands)
        return;

    if (!buffer)
    {
        buffer = weechat_buffer_search_main ();
        if (!buffer)
            return;
    }

    for (i = 0; trigger->commands[i]; i++)
    {
        command_eval = weechat_string_eval_expression (trigger->commands[i],
                                                       pointers, extra_vars,
                                                       NULL);
        if (command_eval)
        {
            /* display debug info on trigger buffer */
            if (trigger_buffer && display_monitor)
            {
                weechat_printf_tags (trigger_buffer, "no_trigger",
                                     _("%s  running command %s\"%s%s%s\"%s "
                                       "on buffer %s%s%s"),
                                     "\t",
                                     weechat_color ("chat_delimiters"),
                                     weechat_color ("reset"),
                                     command_eval,
                                     weechat_color ("chat_delimiters"),
                                     weechat_color ("reset"),
                                     weechat_color ("chat_buffer"),
                                     weechat_buffer_get_string (buffer,
                                                                "full_name"),
                                     weechat_color ("reset"));
            }
            weechat_command (buffer, command_eval);
            trigger->hook_count_cmd++;
        }
        free (command_eval);
    }
}

/*
 * Executes a trigger.
 *
 * Following actions are executed:
 *   1. display debug info on trigger buffer
 *   2. check conditions (if false, exit)
 *   3. replace text with regex
 *   4. execute command(s)
 *
 * Returns:
 *   1: conditions were true (or no condition set in trigger)
 *   0: conditions were false
 */

int
trigger_callback_execute (struct t_trigger *trigger,
                          struct t_gui_buffer *buffer,
                          struct t_hashtable *pointers,
                          struct t_hashtable *extra_vars)
{
    int display_monitor;

    /* display debug info on trigger buffer */
    if (!trigger_buffer && (weechat_trigger_plugin->debug >= 1))
        trigger_buffer_open (NULL, 0);
    display_monitor = trigger_buffer_display_trigger (trigger,
                                                      buffer,
                                                      pointers,
                                                      extra_vars);

    /* check conditions */
    if (trigger_callback_check_conditions (trigger, pointers, extra_vars))
    {
        /* replace text with regex */
        trigger_callback_replace_regex (trigger, pointers, extra_vars,
                                        display_monitor);

        /* execute command(s) */
        trigger_callback_run_command (trigger, buffer, pointers, extra_vars,
                                      display_monitor);

        return 1;
    }

    return 0;
}

/*
 * Callback for a signal hooked.
 */

int
trigger_callback_signal_cb (void *data, const char *signal,
                            const char *type_data, void *signal_data)
{
    const char *ptr_signal_data;
    char str_data[128], *irc_server;
    const char *pos, *ptr_irc_message;

    TRIGGER_CALLBACK_CB_INIT(WEECHAT_RC_OK);

    /* split IRC message (if signal_data is an IRC message) */
    irc_server = NULL;
    ptr_irc_message = NULL;
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strstr (signal, ",irc_in_")
            || strstr (signal, ",irc_in2_")
            || strstr (signal, ",irc_raw_in_")
            || strstr (signal, ",irc_raw_in2_")
            || strstr (signal, ",irc_out1_")
            || strstr (signal, ",irc_out_"))
        {
            pos = strchr (signal, ',');
            if (pos)
            {
                irc_server = weechat_strndup (signal, pos - signal);
                ptr_irc_message = (const char *)signal_data;
            }
        }
        else
        {
            pos = strstr (signal, ",irc_outtags_");
            if (pos)
            {
                irc_server = weechat_strndup (signal, pos - signal);
                pos = strchr ((const char *)signal_data, ';');
                if (pos)
                    ptr_irc_message = pos + 1;
            }
        }
    }
    if (irc_server && ptr_irc_message)
    {
        extra_vars = trigger_callback_irc_message_parse (ptr_irc_message,
                                                         irc_server);
        if (extra_vars)
            weechat_hashtable_set (extra_vars, "server", irc_server);
    }
    if (irc_server)
        free (irc_server);

    /* create hashtable (if not already created) */
    if (!extra_vars)
    {
        TRIGGER_CALLBACK_CB_NEW_EXTRA_VARS;
    }

    /* add data in hashtable used for conditions/replace/command */
    ptr_signal_data = NULL;
    weechat_hashtable_set (extra_vars, "tg_signal", signal);
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        ptr_signal_data = (const char *)signal_data;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        str_data[0] = '\0';
        if (signal_data)
        {
            snprintf (str_data, sizeof (str_data),
                      "%d", *((int *)signal_data));
        }
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

    /* execute the trigger (conditions, regex, command) */
    if (!trigger_callback_execute (trigger, NULL, pointers, extra_vars))
        trigger_rc = WEECHAT_RC_OK;

end:
    TRIGGER_CALLBACK_CB_END(trigger_rc);
}

/*
 * Callback for a hsignal hooked.
 */

int
trigger_callback_hsignal_cb (void *data, const char *signal,
                             struct t_hashtable *hashtable)
{
    const char *type_values;

    TRIGGER_CALLBACK_CB_INIT(WEECHAT_RC_OK);

    /* duplicate hashtable */
    if (hashtable
        && (strcmp (weechat_hashtable_get_string (hashtable, "type_keys"), "string") == 0))
    {
        type_values = weechat_hashtable_get_string (hashtable, "type_values");
        if (strcmp (type_values, "pointer") == 0)
        {
            pointers = weechat_hashtable_dup (hashtable);
            if (!pointers)
                goto end;
        }
        else if (strcmp (type_values, "string") == 0)
        {
            extra_vars = weechat_hashtable_dup (hashtable);
            if (!extra_vars)
                goto end;
        }
    }

    /* create hashtable (if not already created) */
    if (!extra_vars)
    {
        TRIGGER_CALLBACK_CB_NEW_EXTRA_VARS;
    }

    /* add data in hashtable used for conditions/replace/command */
    weechat_hashtable_set (extra_vars, "tg_signal", signal);

    /* execute the trigger (conditions, regex, command) */
    if (!trigger_callback_execute (trigger, NULL, pointers, extra_vars))
        trigger_rc = WEECHAT_RC_OK;

end:
    TRIGGER_CALLBACK_CB_END(trigger_rc);
}

/*
 * Callback for a modifier hooked.
 */

char *
trigger_callback_modifier_cb (void *data, const char *modifier,
                              const char *modifier_data, const char *string)
{
    struct t_gui_buffer *buffer;
    const char *ptr_string;
    char *string_modified, *pos, *pos2, *plugin_name, *buffer_name;
    char *buffer_full_name, *str_tags, **tags, *prefix, *string_no_color;
    int length, num_tags;

    TRIGGER_CALLBACK_CB_INIT(NULL);

    buffer = NULL;
    tags = NULL;
    num_tags = 0;
    string_no_color = NULL;

    /* split IRC message (if string is an IRC message) */
    if ((strncmp (modifier, "irc_in_", 7) == 0)
        || (strncmp (modifier, "irc_in2_", 8) == 0)
        || (strncmp (modifier, "irc_out1_", 9) == 0)
        || (strncmp (modifier, "irc_out_", 8) == 0))
    {
        extra_vars = trigger_callback_irc_message_parse (string,
                                                         modifier_data);
        if (extra_vars)
            weechat_hashtable_set (extra_vars, "server", modifier_data);
    }

    TRIGGER_CALLBACK_CB_NEW_POINTERS;
    if (!extra_vars)
    {
        TRIGGER_CALLBACK_CB_NEW_EXTRA_VARS;
    }

    /* add data in hashtable used for conditions/replace/command */
    weechat_hashtable_set (extra_vars, "tg_modifier", modifier);
    weechat_hashtable_set (extra_vars, "tg_modifier_data", modifier_data);
    weechat_hashtable_set (extra_vars, "tg_string", string);
    string_no_color = weechat_string_remove_color (string, NULL);
    if (string_no_color)
    {
        weechat_hashtable_set (extra_vars,
                               "tg_string_nocolor", string_no_color);
    }

    /* add special variables for a WeeChat message */
    if (strcmp (modifier, "weechat_print") == 0)
    {
        /* set "tg_prefix" and "tg_message" */
        pos = strchr (string, '\t');
        if (pos)
        {
            if (pos > string)
            {
                prefix = weechat_strndup (string, pos - string);
                if (prefix)
                {
                    weechat_hashtable_set (extra_vars, "tg_prefix", prefix);
                    free (prefix);
                }
            }
            pos++;
            if (pos[0] == '\t')
                pos++;
            weechat_hashtable_set (extra_vars, "tg_message", pos);
        }
        else
            weechat_hashtable_set (extra_vars, "tg_message", string);

        /* set "tg_prefix_nocolor" and "tg_message_nocolor" */
        if (string_no_color)
        {
            pos = strchr (string_no_color, '\t');
            if (pos)
            {
                if (pos > string_no_color)
                {
                    prefix = weechat_strndup (string_no_color,
                                              pos - string_no_color);
                    if (prefix)
                    {
                        weechat_hashtable_set (extra_vars,
                                               "tg_prefix_nocolor", prefix);
                        free (prefix);
                    }
                }
                pos++;
                if (pos[0] == '\t')
                    pos++;
                weechat_hashtable_set (extra_vars, "tg_message_nocolor", pos);
            }
            else
            {
                weechat_hashtable_set (extra_vars,
                                       "tg_message_nocolor", string_no_color);
            }
        }

        /*
         * extract buffer/tags from modifier data
         * (format: "plugin;buffer_name;tags")
         */
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
                        buffer = weechat_buffer_search (plugin_name,
                                                        buffer_name);
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
                        tags = weechat_string_split (pos2, ",", 0, 0, &num_tags);
                        length = 1 + strlen (pos2) + 1 + 1;
                        str_tags = malloc (length);
                        if (str_tags)
                        {
                            snprintf (str_tags, length, ",%s,", pos2);
                            weechat_hashtable_set (extra_vars, "tg_tags",
                                                   str_tags);
                            free (str_tags);
                        }
                    }
                }
                free (plugin_name);
            }
        }
        weechat_hashtable_set (pointers, "buffer", buffer);
    }

    if (tags)
    {
        if (!trigger_callback_set_tags (buffer, (const char **)tags, num_tags,
                                        extra_vars))
        {
            goto end;
        }
    }

    /* execute the trigger (conditions, regex, command) */
    trigger_callback_execute (trigger, buffer, pointers, extra_vars);

end:
    ptr_string = weechat_hashtable_get (extra_vars, "tg_string");
    string_modified = (ptr_string && (strcmp (ptr_string, string) != 0)) ?
        strdup (ptr_string) : NULL;

    if (tags)
        weechat_string_free_split (tags);
    if (string_no_color)
        free (string_no_color);

    TRIGGER_CALLBACK_CB_END(string_modified);
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
    char *str_tags, *str_tags2, str_temp[128], *str_no_color;
    int length;
    struct tm *date_tmp;

    TRIGGER_CALLBACK_CB_INIT(WEECHAT_RC_OK);

    /* do nothing if the buffer does not match buffers defined in the trigger */
    if (trigger->hook_print_buffers
        && !weechat_buffer_match_list (buffer, trigger->hook_print_buffers))
        goto end;

    TRIGGER_CALLBACK_CB_NEW_POINTERS;
    TRIGGER_CALLBACK_CB_NEW_EXTRA_VARS;

    /* add data in hashtables used for conditions/replace/command */
    weechat_hashtable_set (pointers, "buffer", buffer);
    date_tmp = localtime (&date);
    if (date_tmp)
    {
        strftime (str_temp, sizeof (str_temp), "%Y-%m-%d %H:%M:%S", date_tmp);
        weechat_hashtable_set (extra_vars, "tg_date", str_temp);
    }
    snprintf (str_temp, sizeof (str_temp), "%d", displayed);
    weechat_hashtable_set (extra_vars, "tg_displayed", str_temp);
    snprintf (str_temp, sizeof (str_temp), "%d", highlight);
    weechat_hashtable_set (extra_vars, "tg_highlight", str_temp);
    weechat_hashtable_set (extra_vars, "tg_prefix", prefix);
    str_no_color = weechat_string_remove_color (prefix, NULL);
    if (str_no_color)
    {
        weechat_hashtable_set (extra_vars, "tg_prefix_nocolor", str_no_color);
        free (str_no_color);
    }
    weechat_hashtable_set (extra_vars, "tg_message", message);
    str_no_color = weechat_string_remove_color (message, NULL);
    if (str_no_color)
    {
        weechat_hashtable_set (extra_vars, "tg_message_nocolor", str_no_color);
        free (str_no_color);
    }

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
    if (!trigger_callback_set_tags (buffer, tags, tags_count, extra_vars))
        goto end;

    /* execute the trigger (conditions, regex, command) */
    if (!trigger_callback_execute (trigger, buffer, pointers, extra_vars))
        trigger_rc = WEECHAT_RC_OK;

end:
    TRIGGER_CALLBACK_CB_END(trigger_rc);
}

/*
 * Callback for a command hooked.
 */

int
trigger_callback_command_cb  (void *data, struct t_gui_buffer *buffer,
                              int argc, char **argv, char **argv_eol)
{
    char str_name[32];
    int i;

    TRIGGER_CALLBACK_CB_INIT(WEECHAT_RC_OK);

    TRIGGER_CALLBACK_CB_NEW_POINTERS;
    TRIGGER_CALLBACK_CB_NEW_EXTRA_VARS;

    /* add data in hashtables used for conditions/replace/command */
    weechat_hashtable_set (pointers, "buffer", buffer);
    for (i = 0; i < argc; i++)
    {
        snprintf (str_name, sizeof (str_name), "tg_argv%d", i);
        weechat_hashtable_set (extra_vars, str_name, argv[i]);
        snprintf (str_name, sizeof (str_name), "tg_argv_eol%d", i);
        weechat_hashtable_set (extra_vars, str_name, argv_eol[i]);
    }

    /* execute the trigger (conditions, regex, command) */
    if (!trigger_callback_execute (trigger, buffer, pointers, extra_vars))
        trigger_rc = WEECHAT_RC_OK;

end:
    TRIGGER_CALLBACK_CB_END(trigger_rc);
}

/*
 * Callback for a command_run hooked.
 */

int
trigger_callback_command_run_cb  (void *data, struct t_gui_buffer *buffer,
                                  const char *command)
{
    TRIGGER_CALLBACK_CB_INIT(WEECHAT_RC_OK);

    TRIGGER_CALLBACK_CB_NEW_POINTERS;
    TRIGGER_CALLBACK_CB_NEW_EXTRA_VARS;

    /* add data in hashtables used for conditions/replace/command */
    weechat_hashtable_set (pointers, "buffer", buffer);
    weechat_hashtable_set (extra_vars, "tg_command", command);

    /* execute the trigger (conditions, regex, command) */
    if (!trigger_callback_execute (trigger, buffer, pointers, extra_vars))
        trigger_rc = WEECHAT_RC_OK;

end:
    TRIGGER_CALLBACK_CB_END(trigger_rc);
}

/*
 * Callback for a timer hooked.
 */

int
trigger_callback_timer_cb  (void *data, int remaining_calls)
{
    char str_temp[128];
    int i;
    time_t date;
    struct tm *date_tmp;

    TRIGGER_CALLBACK_CB_INIT(WEECHAT_RC_OK);

    /*
     * remove the hook if this is the last call to timer
     * (because WeeChat will remove the hook after this call, so the pointer
     * will become invalid)
     */
    if ((remaining_calls == 0) && trigger->hooks)
    {
        for (i = 0; i < trigger->hooks_count; i++)
        {
            trigger->hooks[i] = NULL;
        }
    }

    TRIGGER_CALLBACK_CB_NEW_EXTRA_VARS;

    /* add data in hashtable used for conditions/replace/command */
    snprintf (str_temp, sizeof (str_temp), "%d", remaining_calls);
    weechat_hashtable_set (extra_vars, "tg_remaining_calls", str_temp);
    date = time (NULL);
    date_tmp = localtime (&date);
    if (date_tmp)
    {
        strftime (str_temp, sizeof (str_temp), "%Y-%m-%d %H:%M:%S", date_tmp);
        weechat_hashtable_set (extra_vars, "tg_date", str_temp);
    }

    /* execute the trigger (conditions, regex, command) */
    if (!trigger_callback_execute (trigger, NULL, pointers, extra_vars))
        trigger_rc = WEECHAT_RC_OK;

end:
    TRIGGER_CALLBACK_CB_END(trigger_rc);
}

/*
 * Callback for a config hooked.
 */

int
trigger_callback_config_cb  (void *data, const char *option, const char *value)
{
    TRIGGER_CALLBACK_CB_INIT(WEECHAT_RC_OK);

    TRIGGER_CALLBACK_CB_NEW_EXTRA_VARS;

    /* add data in hashtable used for conditions/replace/command */
    weechat_hashtable_set (extra_vars, "tg_option", option);
    weechat_hashtable_set (extra_vars, "tg_value", value);

    /* execute the trigger (conditions, regex, command) */
    if (!trigger_callback_execute (trigger, NULL, pointers, extra_vars))
        trigger_rc = WEECHAT_RC_OK;

end:
    TRIGGER_CALLBACK_CB_END(trigger_rc);
}

/*
 * Callback for a focus hooked.
 */

struct t_hashtable *
trigger_callback_focus_cb (void *data, struct t_hashtable *info)
{
    const char *ptr_value;
    long unsigned int value;
    int rc;

    TRIGGER_CALLBACK_CB_INIT(info);

    TRIGGER_CALLBACK_CB_NEW_POINTERS;

    /* add data in hashtables used for conditions/replace/command */
    ptr_value = weechat_hashtable_get (info, "_window");
    if (ptr_value && ptr_value[0] && (strncmp (ptr_value, "0x", 2) == 0))
    {
        rc = sscanf (ptr_value + 2, "%lx", &value);
        if ((rc != EOF) && (rc >= 1))
            weechat_hashtable_set (pointers, "window", (void *)value);
    }
    ptr_value = weechat_hashtable_get (info, "_buffer");
    if (ptr_value && ptr_value[0] && (strncmp (ptr_value, "0x", 2) == 0))
    {
        rc = sscanf (ptr_value + 2, "%lx", &value);
        if ((rc != EOF) && (rc >= 1))
            weechat_hashtable_set (pointers, "buffer", (void *)value);
    }

    /* execute the trigger (conditions, regex, command) */
    trigger_callback_execute (trigger, NULL, pointers, info);

end:
    TRIGGER_CALLBACK_CB_END(info);
}

/*
 * Initializes trigger callback.
 */

void
trigger_callback_init ()
{
    trigger_callback_hashtable_options_conditions = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL,
        NULL);
    if (trigger_callback_hashtable_options_conditions)
    {
        weechat_hashtable_set (trigger_callback_hashtable_options_conditions,
                               "type", "condition");
    }

    trigger_callback_hashtable_options_regex = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL,
        NULL);
}

/*
 * Ends trigger callback.
 */

void
trigger_callback_end ()
{
    if (trigger_callback_hashtable_options_conditions)
        weechat_hashtable_free (trigger_callback_hashtable_options_conditions);
    if (trigger_callback_hashtable_options_regex)
        weechat_hashtable_free (trigger_callback_hashtable_options_regex);
}
