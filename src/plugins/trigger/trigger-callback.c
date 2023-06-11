/*
 * trigger-callback.c - callbacks for triggers
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
#include <limits.h>
#include <time.h>
#include <sys/time.h>

#include "../weechat-plugin.h"
#include "trigger.h"
#include "trigger-callback.h"
#include "trigger-buffer.h"
#include "trigger-config.h"


/*
 * trigger context id to correlate messages in monitor buffer with
 * the running trigger
 */
unsigned long trigger_context_id = 0;

/* hashtable used to evaluate "conditions" */
struct t_hashtable *trigger_callback_hashtable_options_conditions = NULL;


/*
 * Parses an IRC message.
 *
 * Returns a hashtable with the parsed message, or NULL if error.
 *
 * Note: hashtable must be freed after use.
 */

struct t_hashtable *
trigger_callback_irc_message_parse (const char *irc_message,
                                    const char *irc_server_name)
{
    struct t_hashtable *hashtable_in, *hashtable_out;

    hashtable_out = NULL;

    hashtable_in = weechat_hashtable_new (32,
                                          WEECHAT_HASHTABLE_STRING,
                                          WEECHAT_HASHTABLE_STRING,
                                          NULL, NULL);
    if (hashtable_in)
    {
        weechat_hashtable_set (hashtable_in, "message", irc_message);
        weechat_hashtable_set (hashtable_in, "server", irc_server_name);
        hashtable_out = weechat_info_get_hashtable ("irc_message_parse",
                                                    hashtable_in);
        weechat_hashtable_free (hashtable_in);
    }

    return hashtable_out;
}

/*
 * Gets the pointer to IRC server with its name.
 *
 * Returns pointer to IRC server, or NULL if not found.
 */

void
trigger_callback_get_irc_server_channel (const char *irc_server_name,
                                         const char *irc_channel_name,
                                         void **irc_server, void **irc_channel)
{
    struct t_hdata *hdata_irc_server, *hdata_irc_channel;
    const char *ptr_name;

    *irc_server = NULL;
    *irc_channel = NULL;

    if (!irc_server_name)
        return;

    hdata_irc_server = weechat_hdata_get ("irc_server");
    if (!hdata_irc_server)
        return;

    /* search the server by name in list of servers */
    *irc_server = weechat_hdata_get_list (hdata_irc_server, "irc_servers");
    while (*irc_server)
    {
        ptr_name = weechat_hdata_string (hdata_irc_server, *irc_server, "name");
        if (strcmp (ptr_name, irc_server_name) == 0)
            break;
        *irc_server = weechat_hdata_move (hdata_irc_server, *irc_server, 1);
    }
    if (!*irc_server)
        return;

    if (!irc_channel_name)
        return;

    hdata_irc_channel = weechat_hdata_get ("irc_channel");
    if (!hdata_irc_channel)
        return;

    /* search the channel by name in list of channels on the server */
    *irc_channel = weechat_hdata_pointer (hdata_irc_server,
                                          *irc_server, "channels");
    while (*irc_channel)
    {
        ptr_name = weechat_hdata_string (hdata_irc_channel,
                                         *irc_channel, "name");
        if (strcmp (ptr_name, irc_channel_name) == 0)
            break;
        *irc_channel = weechat_hdata_move (hdata_irc_channel, *irc_channel, 1);
    }
}

/*
 * Sets variables common to all triggers in a hashtable:
 * - tg_trigger_name
 */

void
trigger_callback_set_common_vars (struct t_trigger *trigger,
                                  struct t_hashtable *hashtable)
{
    if (!trigger || !hashtable)
        return;

    weechat_hashtable_set (hashtable, "tg_trigger_name", trigger->name);
    weechat_hashtable_set (
        hashtable, "tg_hook_type",
        trigger_hook_type_string[
            weechat_config_integer (trigger->options[TRIGGER_OPTION_HOOK])]);
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
    const char *localvar_type, *pos;
    char str_temp[1024], *key;
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
        else if (strncmp (tags[i], "irc_tag_", 8) == 0)
        {
            /*
             * example:
             *   tag: "irc_tag_time=2021-12-30T21:02:50.038Z"
             * is added in the hashtable like this:
             *   key  : "tg_tag_irc_time"
             *   value: "2021-12-30T21:02:50.038Z"
             */
            pos = strchr (tags[i] + 8, '=');
            if (!pos || (pos > tags[i] + 8))
            {
                if (pos)
                {
                    /* tag with value */
                    key = weechat_strndup (tags[i] + 8, pos - tags[i] - 8);
                    if (key)
                    {
                        snprintf (str_temp, sizeof (str_temp),
                                  "tg_tag_irc_%s", key);
                        weechat_hashtable_set (extra_vars, str_temp, pos + 1);
                        free (key);
                    }
                }
                else
                {
                    /* tag without value */
                    snprintf (str_temp, sizeof (str_temp),
                              "tg_tag_irc_%s", tags[i] + 8);
                    weechat_hashtable_set (extra_vars, str_temp, NULL);
                }
            }
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
 * Replaces text using regex.
 *
 * Returns: text replaced.
 *
 * Note: result must be freed after use.
 */

char *
trigger_callback_regex_replace (struct t_trigger_context *context,
                                const char *text,
                                regex_t *regex,
                                const char *replace)
{
    char *value;
    struct t_hashtable *hashtable_options_regex;

    if (!regex)
        return NULL;

    hashtable_options_regex = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);

    weechat_hashtable_set (context->pointers, "regex", regex);
    weechat_hashtable_set (hashtable_options_regex,
                           "regex_replace", replace);

    value = weechat_string_eval_expression (
        text,
        context->pointers,
        context->extra_vars,
        hashtable_options_regex);

    weechat_hashtable_free (hashtable_options_regex);

    return value;
}

/*
 * Translates chars.
 *
 * Returns: text with translated chars.
 *
 * Note: result must be freed after use.
 */

char *
trigger_callback_regex_translate_chars (struct t_trigger_context *context,
                                        const char *text,
                                        const char *chars1,
                                        const char *chars2)
{
    char *value, *chars1_eval, *chars2_eval;

    chars1_eval = weechat_string_eval_expression (
        chars1,
        context->pointers,
        context->extra_vars,
        NULL);
    chars2_eval = weechat_string_eval_expression (
        chars2,
        context->pointers,
        context->extra_vars,
        NULL);

    value = weechat_string_translate_chars (text, chars1_eval, chars2_eval);

    if (chars1_eval)
        free (chars1_eval);
    if (chars2_eval)
        free (chars2_eval);

    return value;
}

/*
 * Executes regex commands.
 */

void
trigger_callback_regex (struct t_trigger *trigger,
                        struct t_trigger_context *context,
                        int display_monitor)
{
    char *value;
    const char *ptr_key, *ptr_value;
    int i, pointers_allocated;

    value = NULL;
    pointers_allocated = 0;

    if (trigger->regex_count == 0)
        return;

    if (!context->pointers)
    {
        context->pointers = weechat_hashtable_new (32,
                                                   WEECHAT_HASHTABLE_STRING,
                                                   WEECHAT_HASHTABLE_POINTER,
                                                   NULL, NULL);
        if (!context->pointers)
            return;
        pointers_allocated = 1;
    }

    for (i = 0; i < trigger->regex_count; i++)
    {
        /* if regex is not set (invalid) for command "regex replace", skip it */
        if ((trigger->regex[i].command == TRIGGER_REGEX_COMMAND_REPLACE)
            && !trigger->regex[i].regex)
        {
            continue;
        }

        ptr_key = (trigger->regex[i].variable) ?
            trigger->regex[i].variable :
            trigger_hook_regex_default_var[weechat_config_integer (trigger->options[TRIGGER_OPTION_HOOK])];
        if (!ptr_key || !ptr_key[0])
        {
            if (trigger_buffer && display_monitor)
            {
                weechat_printf_date_tags (trigger_buffer, 0, "no_trigger",
                                          "%s%lu\t  regex %d: %s",
                                          weechat_color (weechat_config_string (trigger_config_color_identifier)),
                                          context->id,
                                          i + 1,
                                          _("no variable"));
            }
            continue;
        }

        ptr_value = weechat_hashtable_get (context->extra_vars, ptr_key);
        if (!ptr_value)
        {
            if (trigger_buffer && display_monitor)
            {
                weechat_printf_date_tags (trigger_buffer, 0, "no_trigger",
                                          "%s%lu\t  regex %d (%s): %s",
                                          weechat_color (weechat_config_string (trigger_config_color_identifier)),
                                          context->id,
                                          i + 1,
                                          ptr_key,
                                          _("creating variable"));
            }
            weechat_hashtable_set (context->extra_vars, ptr_key, "");
            ptr_value = weechat_hashtable_get (context->extra_vars, ptr_key);
        }

        switch (trigger->regex[i].command)
        {
            case TRIGGER_REGEX_COMMAND_REPLACE:
                value = trigger_callback_regex_replace (
                    context,
                    ptr_value,
                    trigger->regex[i].regex,
                    trigger->regex[i].replace_escaped);
                break;
            case TRIGGER_REGEX_COMMAND_TRANSLATE_CHARS:
                value = trigger_callback_regex_translate_chars (
                    context,
                    ptr_value,
                    trigger->regex[i].str_regex,
                    trigger->regex[i].replace);
                break;
            case TRIGGER_NUM_REGEX_COMMANDS:
                break;
        }

        if (value)
        {
            /* display debug info on trigger buffer */
            if (trigger_buffer && display_monitor)
            {
                weechat_printf_date_tags (trigger_buffer, 0, "no_trigger",
                                          "%s%lu\t  regex %d %s(%s%s%s)%s: "
                                          "%s\"%s%s%s\"",
                                          weechat_color (weechat_config_string (trigger_config_color_identifier)),
                                          context->id,
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
            weechat_hashtable_set (context->extra_vars, ptr_key, value);
            if (context->vars_updated)
            {
                weechat_list_add (context->vars_updated,
                                  ptr_key, WEECHAT_LIST_POS_END,
                                  NULL);
            }
            free (value);
        }
    }

    if (pointers_allocated)
    {
        weechat_hashtable_free (context->pointers);
        context->pointers = NULL;
    }
    else
    {
        weechat_hashtable_remove (context->pointers, "regex");
    }
}

/*
 * Executes the trigger command(s).
 */

void
trigger_callback_run_command (struct t_trigger *trigger,
                              struct t_trigger_context *context,
                              int display_monitor)
{
    struct t_gui_buffer *buffer;
    char *command_eval;
    int i;

    if (!trigger->commands)
        return;

    buffer = context->buffer;
    if (!buffer)
    {
        buffer = weechat_buffer_search_main ();
        if (!buffer)
            return;
    }

    for (i = 0; trigger->commands[i]; i++)
    {
        command_eval = weechat_string_eval_expression (trigger->commands[i],
                                                       context->pointers,
                                                       context->extra_vars,
                                                       NULL);
        if (command_eval)
        {
            /* display debug info on trigger buffer */
            if (trigger_buffer && display_monitor)
            {
                weechat_printf_date_tags (
                    trigger_buffer, 0, "no_trigger",
                    _("%s%lu%s  running command %s\"%s%s%s\"%s "
                      "on buffer %s%s%s"),
                    weechat_color (weechat_config_string (trigger_config_color_identifier)),
                    context->id,
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
                          struct t_trigger_context *context)
{
    int rc, display_monitor;
    long long time_init, time_cond, time_regex, time_cmd, time_total;

    rc = 0;

    trigger_context_id = (trigger_context_id < ULONG_MAX) ?
        trigger_context_id + 1 : 0;
    context->id = trigger_context_id;

    /* display debug info on trigger buffer */
    if (!trigger_buffer && (weechat_trigger_plugin->debug >= 1))
        trigger_buffer_open (NULL, 0);
    display_monitor = trigger_buffer_display_trigger (trigger, context);

    if (weechat_trigger_plugin->debug >= 1)
    {
        gettimeofday (&(context->start_check_conditions), NULL);
        context->start_regex = context->start_check_conditions;
        context->start_run_command = context->start_check_conditions;
    }

    /* check conditions */
    if (trigger_callback_check_conditions (trigger,
                                           context->pointers,
                                           context->extra_vars))
    {
        /* replace text with regex */
        if (weechat_trigger_plugin->debug >= 1)
            gettimeofday (&(context->start_check_conditions), NULL);
        trigger_callback_regex (trigger, context, display_monitor);

        /* execute command(s) */
        if (weechat_trigger_plugin->debug >= 1)
            gettimeofday (&(context->start_run_command), NULL);
        trigger_callback_run_command (trigger, context, display_monitor);

        rc = 1;
    }

    if (weechat_trigger_plugin->debug >= 1)
        gettimeofday (&(context->end_exec), NULL);

    if (trigger_buffer && display_monitor
        && (weechat_trigger_plugin->debug >= 1))
    {
        time_init = weechat_util_timeval_diff (&(context->start_exec),
                                               &(context->start_check_conditions));
        time_cond = weechat_util_timeval_diff (&(context->start_check_conditions),
                                               &(context->start_regex));
        time_regex = weechat_util_timeval_diff (&(context->start_regex),
                                                &(context->start_run_command));
        time_cmd = weechat_util_timeval_diff (&(context->start_run_command),
                                              &(context->end_exec));
        time_total = time_init + time_cond + time_regex + time_cmd;

        weechat_printf_date_tags (trigger_buffer, 0, "no_trigger",
                                  _("%s%lu%s  elapsed: init=%.6fs, "
                                    "conditions=%.6fs, regex=%.6fs, "
                                    "command=%.6fs, total=%.6fs"),
                                  weechat_color (weechat_config_string (trigger_config_color_identifier)),
                                  context->id,
                                  "\t",
                                  (float)time_init / 1000000,
                                  (float)time_cond / 1000000,
                                  (float)time_regex / 1000000,
                                  (float)time_cmd / 1000000,
                                  (float)time_total / 1000000);
    }

    return rc;
}

/*
 * Callback for a signal hooked.
 */

int
trigger_callback_signal_cb (const void *pointer, void *data,
                            const char *signal, const char *type_data,
                            void *signal_data)
{
    const char *ptr_signal_data;
    char str_data[128], *irc_server_name;
    const char *pos, *ptr_irc_message;
    void *ptr_irc_server, *ptr_irc_channel;

    TRIGGER_CALLBACK_CB_INIT(WEECHAT_RC_OK);

    TRIGGER_CALLBACK_CB_NEW_POINTERS;

    /* split IRC message (if signal_data is an IRC message) */
    irc_server_name = NULL;
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
                irc_server_name = weechat_strndup (signal, pos - signal);
                ptr_irc_message = (const char *)signal_data;
            }
        }
        else
        {
            pos = strstr (signal, ",irc_outtags_");
            if (pos)
            {
                irc_server_name = weechat_strndup (signal, pos - signal);
                pos = strchr ((const char *)signal_data, ';');
                if (pos)
                    ptr_irc_message = pos + 1;
            }
        }
    }
    if (irc_server_name && ptr_irc_message)
    {
        ctx.extra_vars = trigger_callback_irc_message_parse (
            ptr_irc_message,
            irc_server_name);
        if (ctx.extra_vars)
        {
            weechat_hashtable_set (ctx.extra_vars, "server", irc_server_name);
            trigger_callback_get_irc_server_channel (
                irc_server_name,
                weechat_hashtable_get (ctx.extra_vars, "channel"),
                &ptr_irc_server,
                &ptr_irc_channel);
            weechat_hashtable_set (ctx.pointers, "irc_server", ptr_irc_server);
            weechat_hashtable_set (ctx.pointers, "irc_channel", ptr_irc_channel);
        }
    }
    if (irc_server_name)
        free (irc_server_name);

    /* create hashtable (if not already created) */
    if (!ctx.extra_vars)
    {
        TRIGGER_CALLBACK_CB_NEW_EXTRA_VARS;
    }

    /* add data in hashtable used for conditions/replace/command */
    trigger_callback_set_common_vars (trigger, ctx.extra_vars);
    weechat_hashtable_set (ctx.extra_vars, "tg_signal", signal);
    ptr_signal_data = NULL;
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
                      "0x%lx", (unsigned long)signal_data);
        }
        ptr_signal_data = str_data;
    }
    weechat_hashtable_set (ctx.extra_vars, "tg_signal_data", ptr_signal_data);

    /* execute the trigger (conditions, regex, command) */
    if (!trigger_callback_execute (trigger, &ctx))
        trigger_rc = WEECHAT_RC_OK;

end:
    TRIGGER_CALLBACK_CB_END(trigger_rc);
}

/*
 * Callback for a hsignal hooked.
 */

int
trigger_callback_hsignal_cb (const void *pointer, void *data,
                             const char *signal,
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
            ctx.pointers = weechat_hashtable_dup (hashtable);
            if (!ctx.pointers)
                goto end;
        }
        else if (strcmp (type_values, "string") == 0)
        {
            ctx.extra_vars = weechat_hashtable_dup (hashtable);
            if (!ctx.extra_vars)
                goto end;
        }
    }

    /* create hashtable (if not already created) */
    if (!ctx.extra_vars)
    {
        TRIGGER_CALLBACK_CB_NEW_EXTRA_VARS;
    }

    /* add data in hashtable used for conditions/replace/command */
    trigger_callback_set_common_vars (trigger, ctx.extra_vars);
    weechat_hashtable_set (ctx.extra_vars, "tg_signal", signal);

    /* execute the trigger (conditions, regex, command) */
    if (!trigger_callback_execute (trigger, &ctx))
        trigger_rc = WEECHAT_RC_OK;

end:
    TRIGGER_CALLBACK_CB_END(trigger_rc);
}

/*
 * Callback for a modifier hooked.
 */

char *
trigger_callback_modifier_cb (const void *pointer, void *data,
                              const char *modifier, const char *modifier_data,
                              const char *string)
{
    const char *ptr_string;
    char *string_modified, *pos, *buffer_pointer;
    char *str_tags, **tags, *prefix, *string_no_color;
    int length, num_tags, rc;
    void *ptr_irc_server, *ptr_irc_channel;
    unsigned long value;

    TRIGGER_CALLBACK_CB_INIT(NULL);

    ctx.buffer = NULL;
    tags = NULL;
    num_tags = 0;
    string_no_color = NULL;

    TRIGGER_CALLBACK_CB_NEW_POINTERS;

    /* split IRC message (if string is an IRC message) */
    if ((strncmp (modifier, "irc_in_", 7) == 0)
        || (strncmp (modifier, "irc_in2_", 8) == 0)
        || (strncmp (modifier, "irc_out1_", 9) == 0)
        || (strncmp (modifier, "irc_out_", 8) == 0))
    {
        ctx.extra_vars = trigger_callback_irc_message_parse (string,
                                                             modifier_data);
        if (ctx.extra_vars)
        {
            weechat_hashtable_set (ctx.extra_vars, "server", modifier_data);
            trigger_callback_get_irc_server_channel (
                modifier_data,
                weechat_hashtable_get (ctx.extra_vars, "channel"),
                &ptr_irc_server,
                &ptr_irc_channel);
            weechat_hashtable_set (ctx.pointers, "irc_server", ptr_irc_server);
            weechat_hashtable_set (ctx.pointers, "irc_channel", ptr_irc_channel);
        }
    }

    if (!ctx.extra_vars)
    {
        TRIGGER_CALLBACK_CB_NEW_EXTRA_VARS;
    }

    /* add data in hashtable used for conditions/replace/command */
    trigger_callback_set_common_vars (trigger, ctx.extra_vars);
    weechat_hashtable_set (ctx.extra_vars, "tg_modifier", modifier);
    weechat_hashtable_set (ctx.extra_vars, "tg_modifier_data", modifier_data);
    weechat_hashtable_set (ctx.extra_vars, "tg_string", string);
    string_no_color = weechat_string_remove_color (string, NULL);
    if (string_no_color)
    {
        weechat_hashtable_set (ctx.extra_vars,
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
                    weechat_hashtable_set (ctx.extra_vars, "tg_prefix", prefix);
                    free (prefix);
                }
            }
            pos++;
            if (pos[0] == '\t')
                pos++;
            weechat_hashtable_set (ctx.extra_vars, "tg_message", pos);
        }
        else
            weechat_hashtable_set (ctx.extra_vars, "tg_message", string);

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
                        weechat_hashtable_set (ctx.extra_vars,
                                               "tg_prefix_nocolor", prefix);
                        free (prefix);
                    }
                }
                pos++;
                if (pos[0] == '\t')
                    pos++;
                weechat_hashtable_set (ctx.extra_vars, "tg_message_nocolor", pos);
            }
            else
            {
                weechat_hashtable_set (ctx.extra_vars,
                                       "tg_message_nocolor", string_no_color);
            }
        }

        /*
         * extract buffer/tags from modifier data
         * (format: "buffer_pointer;tags")
         */
        pos = strchr (modifier_data, ';');
        if (pos)
        {
            buffer_pointer = weechat_strndup (modifier_data,
                                              pos - modifier_data);
            if (buffer_pointer)
            {
                rc = sscanf (buffer_pointer, "0x%lx", &value);
                if ((rc != EOF) && (rc != 0))
                {
                    ctx.buffer = (struct t_gui_buffer *)value;
                    weechat_hashtable_set (
                        ctx.extra_vars,
                        "tg_plugin",
                        weechat_buffer_get_string (ctx.buffer, "plugin"));
                    weechat_hashtable_set (
                        ctx.extra_vars,
                        "tg_buffer",
                        weechat_buffer_get_string (ctx.buffer, "full_name"));
                    pos++;
                    if (pos[0])
                    {
                        tags = weechat_string_split (
                            pos,
                            ",",
                            NULL,
                            WEECHAT_STRING_SPLIT_STRIP_LEFT
                            | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                            | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                            0,
                            &num_tags);
                        length = 1 + strlen (pos) + 1 + 1;
                        str_tags = malloc (length);
                        if (str_tags)
                        {
                            snprintf (str_tags, length, ",%s,", pos);
                            weechat_hashtable_set (ctx.extra_vars, "tg_tags",
                                                   str_tags);
                            free (str_tags);
                        }
                    }
                }
                free (buffer_pointer);
            }
        }
        weechat_hashtable_set (ctx.pointers, "buffer", ctx.buffer);
    }

    if (tags)
    {
        if (!trigger_callback_set_tags (ctx.buffer, (const char **)tags, num_tags,
                                        ctx.extra_vars))
        {
            goto end;
        }
    }

    /* execute the trigger (conditions, regex, command) */
    (void) trigger_callback_execute (trigger, &ctx);

end:
    ptr_string = weechat_hashtable_get (ctx.extra_vars, "tg_string");
    string_modified = (ptr_string && (strcmp (ptr_string, string) != 0)) ?
        strdup (ptr_string) : NULL;

    if (tags)
        weechat_string_free_split (tags);
    if (string_no_color)
        free (string_no_color);

    TRIGGER_CALLBACK_CB_END(string_modified);
}

/*
 * Callback for a line hooked.
 */

struct t_hashtable *
trigger_callback_line_cb (const void *pointer, void *data,
                          struct t_hashtable *line)
{
    struct t_hashtable *hashtable;
    struct t_weelist_item *ptr_item;
    unsigned long value;
    const char *ptr_key, *ptr_value;
    char **tags, *str_tags, *string_no_color;
    int rc, num_tags, length;

    TRIGGER_CALLBACK_CB_INIT(NULL);

    hashtable = NULL;
    tags = NULL;

    TRIGGER_CALLBACK_CB_NEW_POINTERS;
    TRIGGER_CALLBACK_CB_NEW_VARS_UPDATED;

    ctx.extra_vars = weechat_hashtable_dup (line);

    weechat_hashtable_remove (ctx.extra_vars, "buffer");
    weechat_hashtable_remove (ctx.extra_vars, "tags_count");
    weechat_hashtable_remove (ctx.extra_vars, "tags");

    /* add data in hashtables used for conditions/replace/command */
    trigger_callback_set_common_vars (trigger, ctx.extra_vars);
    ptr_value = weechat_hashtable_get (line, "buffer");
    if (!ptr_value || (ptr_value[0] != '0') || (ptr_value[1] != 'x'))
        goto end;
    rc = sscanf (ptr_value + 2, "%lx", &value);
    if ((rc == EOF) || (rc < 1))
        goto end;
    ctx.buffer = (void *)value;

    weechat_hashtable_set (ctx.pointers, "buffer", ctx.buffer);
    ptr_value = weechat_hashtable_get (line, "tags");
    tags = weechat_string_split ((ptr_value) ? ptr_value : "",
                                 ",",
                                 NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                 0,
                                 &num_tags);

    /* build string with tags and commas around: ",tag1,tag2,tag3," */
    length = 1 + strlen ((ptr_value) ? ptr_value : "") + 1 + 1;
    str_tags = malloc (length);
    if (str_tags)
    {
        snprintf (str_tags, length, ",%s,",
                  (ptr_value) ? ptr_value : "");
        weechat_hashtable_set (ctx.extra_vars, "tags", str_tags);
        free (str_tags);
    }

    /* build prefix without colors */
    ptr_value = weechat_hashtable_get (line, "prefix");
    string_no_color = weechat_string_remove_color (ptr_value, NULL);
    weechat_hashtable_set (ctx.extra_vars, "tg_prefix_nocolor", string_no_color);
    if (string_no_color)
        free (string_no_color);

    /* build message without colors */
    ptr_value = weechat_hashtable_get (line, "message");
    string_no_color = weechat_string_remove_color (ptr_value, NULL);
    weechat_hashtable_set (ctx.extra_vars, "tg_message_nocolor", string_no_color);
    if (string_no_color)
        free (string_no_color);

    if (!trigger_callback_set_tags (ctx.buffer, (const char **)tags, num_tags,
                                    ctx.extra_vars))
    {
        goto end;
    }

    /* execute the trigger (conditions, regex, command) */
    (void) trigger_callback_execute (trigger, &ctx);

    hashtable = weechat_hashtable_new (32,
                                       WEECHAT_HASHTABLE_STRING,
                                       WEECHAT_HASHTABLE_STRING,
                                       NULL, NULL);
    if (hashtable)
    {
        /* copy updated variables into the result "hashtable" */
        for (ptr_item = weechat_list_get (ctx.vars_updated, 0); ptr_item;
             ptr_item = weechat_list_next (ptr_item))
        {
            ptr_key = weechat_list_string (ptr_item);
            if (weechat_hashtable_has_key (ctx.extra_vars, ptr_key))
            {
                if (strcmp (ptr_key, "tags") == 0)
                {
                    /* remove commas at the beginning/end of tags */
                    ptr_value = weechat_hashtable_get (ctx.extra_vars, ptr_key);
                    if (ptr_value && ptr_value[0])
                    {
                        str_tags = strdup (
                            (ptr_value[0] == ',') ? ptr_value + 1 : ptr_value);
                        if (str_tags)
                        {
                            if (str_tags[0]
                                && (str_tags[strlen (str_tags) - 1] == ','))
                            {
                                str_tags[strlen (str_tags) - 1] = '\0';
                            }
                            weechat_hashtable_set (hashtable,
                                                   ptr_key, str_tags);
                            free (str_tags);
                        }
                    }
                    else
                    {
                        weechat_hashtable_set (hashtable, ptr_key, ptr_value);
                    }
                }
                else
                {
                    weechat_hashtable_set (
                        hashtable,
                        ptr_key,
                        weechat_hashtable_get (ctx.extra_vars, ptr_key));
                }
            }
        }
    }

end:
    if (tags)
        weechat_string_free_split (tags);

    TRIGGER_CALLBACK_CB_END(hashtable);
}

/*
 * Callback for a print hooked.
 */

int
trigger_callback_print_cb  (const void *pointer, void *data,
                            struct t_gui_buffer *buffer,
                            time_t date, int tags_count, const char **tags,
                            int displayed, int highlight, const char *prefix,
                            const char *message)
{
    char *str_tags, *str_tags2, str_temp[128], *str_no_color;
    int length;
    struct tm *date_tmp;

    TRIGGER_CALLBACK_CB_INIT(WEECHAT_RC_OK);

    ctx.buffer = buffer;

    /* do nothing if the buffer does not match buffers defined in the trigger */
    if (trigger->hook_print_buffers
        && !weechat_buffer_match_list (buffer, trigger->hook_print_buffers))
        goto end;

    TRIGGER_CALLBACK_CB_NEW_POINTERS;
    TRIGGER_CALLBACK_CB_NEW_EXTRA_VARS;

    /* add data in hashtables used for conditions/replace/command */
    trigger_callback_set_common_vars (trigger, ctx.extra_vars);
    weechat_hashtable_set (ctx.pointers, "buffer", buffer);
    date_tmp = localtime (&date);
    if (date_tmp)
    {
        if (strftime (str_temp, sizeof (str_temp),
                      "%Y-%m-%d %H:%M:%S", date_tmp) == 0)
            str_temp[0] = '\0';
        weechat_hashtable_set (ctx.extra_vars, "tg_date", str_temp);
    }
    snprintf (str_temp, sizeof (str_temp), "%d", displayed);
    weechat_hashtable_set (ctx.extra_vars, "tg_displayed", str_temp);
    snprintf (str_temp, sizeof (str_temp), "%d", highlight);
    weechat_hashtable_set (ctx.extra_vars, "tg_highlight", str_temp);
    weechat_hashtable_set (ctx.extra_vars, "tg_prefix", prefix);
    str_no_color = weechat_string_remove_color (prefix, NULL);
    if (str_no_color)
    {
        weechat_hashtable_set (ctx.extra_vars, "tg_prefix_nocolor", str_no_color);
        free (str_no_color);
    }
    weechat_hashtable_set (ctx.extra_vars, "tg_message", message);
    str_no_color = weechat_string_remove_color (message, NULL);
    if (str_no_color)
    {
        weechat_hashtable_set (ctx.extra_vars, "tg_message_nocolor", str_no_color);
        free (str_no_color);
    }

    str_tags = weechat_string_rebuild_split_string (tags, ",", 0, -1);
    if (str_tags)
    {
        /* build string with tags and commas around: ",tag1,tag2,tag3," */
        length = 1 + strlen (str_tags) + 1 + 1;
        str_tags2 = malloc (length);
        if (str_tags2)
        {
            snprintf (str_tags2, length, ",%s,", str_tags);
            weechat_hashtable_set (ctx.extra_vars, "tg_tags", str_tags2);
            free (str_tags2);
        }
        free (str_tags);
    }
    if (!trigger_callback_set_tags (buffer, tags, tags_count, ctx.extra_vars))
        goto end;

    /* execute the trigger (conditions, regex, command) */
    if (!trigger_callback_execute (trigger, &ctx))
        trigger_rc = WEECHAT_RC_OK;

end:
    TRIGGER_CALLBACK_CB_END(trigger_rc);
}

/*
 * Callback for a command hooked.
 */

int
trigger_callback_command_cb  (const void *pointer, void *data,
                              struct t_gui_buffer *buffer,
                              int argc, char **argv, char **argv_eol)
{
    char str_name[64], str_value[128], **shell_argv;
    int i, shell_argc;

    TRIGGER_CALLBACK_CB_INIT(WEECHAT_RC_OK);

    TRIGGER_CALLBACK_CB_NEW_POINTERS;
    TRIGGER_CALLBACK_CB_NEW_EXTRA_VARS;

    ctx.buffer = buffer;

    /* add data in hashtables used for conditions/replace/command */
    trigger_callback_set_common_vars (trigger, ctx.extra_vars);
    weechat_hashtable_set (ctx.pointers, "buffer", buffer);
    snprintf (str_value, sizeof (str_value), "%d", argc);
    weechat_hashtable_set (ctx.extra_vars, "tg_argc", str_value);
    for (i = 0; i < argc; i++)
    {
        snprintf (str_name, sizeof (str_name), "tg_argv%d", i);
        weechat_hashtable_set (ctx.extra_vars, str_name, argv[i]);
        snprintf (str_name, sizeof (str_name), "tg_argv_eol%d", i);
        weechat_hashtable_set (ctx.extra_vars, str_name, argv_eol[i]);
    }
    shell_argv = weechat_string_split_shell (argv_eol[0], &shell_argc);
    if (shell_argv)
    {
        snprintf (str_value, sizeof (str_value), "%d", shell_argc);
        weechat_hashtable_set (ctx.extra_vars, "tg_shell_argc", str_value);
        for (i = 0; i < shell_argc; i++)
        {
            snprintf (str_name, sizeof (str_name), "tg_shell_argv%d", i);
            weechat_hashtable_set (ctx.extra_vars, str_name, shell_argv[i]);
        }
        weechat_string_free_split (shell_argv);
    }
    else
    {
        weechat_hashtable_set (ctx.extra_vars, "tg_shell_argc", "0");
    }

    /* execute the trigger (conditions, regex, command) */
    if (!trigger_callback_execute (trigger, &ctx))
        trigger_rc = WEECHAT_RC_OK;

end:
    TRIGGER_CALLBACK_CB_END(trigger_rc);
}

/*
 * Callback for a command_run hooked.
 */

int
trigger_callback_command_run_cb  (const void *pointer, void *data,
                                  struct t_gui_buffer *buffer,
                                  const char *command)
{
    TRIGGER_CALLBACK_CB_INIT(WEECHAT_RC_OK);

    TRIGGER_CALLBACK_CB_NEW_POINTERS;
    TRIGGER_CALLBACK_CB_NEW_EXTRA_VARS;

    ctx.buffer = buffer;

    /* add data in hashtables used for conditions/replace/command */
    trigger_callback_set_common_vars (trigger, ctx.extra_vars);
    weechat_hashtable_set (ctx.pointers, "buffer", buffer);
    weechat_hashtable_set (ctx.extra_vars, "tg_command", command);

    /* execute the trigger (conditions, regex, command) */
    if (!trigger_callback_execute (trigger, &ctx))
        trigger_rc = WEECHAT_RC_OK;

end:
    TRIGGER_CALLBACK_CB_END(trigger_rc);
}

/*
 * Callback for a timer hooked.
 */

int
trigger_callback_timer_cb  (const void *pointer, void *data,
                            int remaining_calls)
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
    trigger_callback_set_common_vars (trigger, ctx.extra_vars);
    snprintf (str_temp, sizeof (str_temp), "%d", remaining_calls);
    weechat_hashtable_set (ctx.extra_vars, "tg_remaining_calls", str_temp);
    date = time (NULL);
    date_tmp = localtime (&date);
    if (date_tmp)
    {
        if (strftime (str_temp, sizeof (str_temp),
                      "%Y-%m-%d %H:%M:%S", date_tmp) == 0)
            str_temp[0] = '\0';
        weechat_hashtable_set (ctx.extra_vars, "tg_date", str_temp);
    }

    /* execute the trigger (conditions, regex, command) */
    if (!trigger_callback_execute (trigger, &ctx))
        trigger_rc = WEECHAT_RC_OK;

end:
    TRIGGER_CALLBACK_CB_END(trigger_rc);
}

/*
 * Callback for a config hooked.
 */

int
trigger_callback_config_cb  (const void *pointer, void *data,
                             const char *option, const char *value)
{
    TRIGGER_CALLBACK_CB_INIT(WEECHAT_RC_OK);

    TRIGGER_CALLBACK_CB_NEW_EXTRA_VARS;

    /* add data in hashtable used for conditions/replace/command */
    trigger_callback_set_common_vars (trigger, ctx.extra_vars);
    weechat_hashtable_set (ctx.extra_vars, "tg_option", option);
    weechat_hashtable_set (ctx.extra_vars, "tg_value", value);

    /* execute the trigger (conditions, regex, command) */
    if (!trigger_callback_execute (trigger, &ctx))
        trigger_rc = WEECHAT_RC_OK;

end:
    TRIGGER_CALLBACK_CB_END(trigger_rc);
}

/*
 * Callback for a focus hooked.
 */

struct t_hashtable *
trigger_callback_focus_cb (const void *pointer, void *data,
                           struct t_hashtable *info)
{
    const char *ptr_value;
    unsigned long value;
    int rc;

    TRIGGER_CALLBACK_CB_INIT(info);

    TRIGGER_CALLBACK_CB_NEW_POINTERS;

    ctx.extra_vars = weechat_hashtable_dup (info);

    /* add data in hashtables used for conditions/replace/command */
    trigger_callback_set_common_vars (trigger, info);
    ptr_value = weechat_hashtable_get (info, "_window");
    if (ptr_value && ptr_value[0] && (strncmp (ptr_value, "0x", 2) == 0))
    {
        rc = sscanf (ptr_value + 2, "%lx", &value);
        if ((rc != EOF) && (rc >= 1))
            weechat_hashtable_set (ctx.pointers, "window", (void *)value);
    }
    ptr_value = weechat_hashtable_get (info, "_buffer");
    if (ptr_value && ptr_value[0] && (strncmp (ptr_value, "0x", 2) == 0))
    {
        rc = sscanf (ptr_value + 2, "%lx", &value);
        if ((rc != EOF) && (rc >= 1))
            weechat_hashtable_set (ctx.pointers, "buffer", (void *)value);
    }

    /* execute the trigger (conditions, regex, command) */
    (void) trigger_callback_execute (trigger, &ctx);

end:
    TRIGGER_CALLBACK_CB_END(info);
}

/*
 * Callback for an info hooked.
 */

char *
trigger_callback_info_cb (const void *pointer, void *data,
                          const char *info_name, const char *arguments)
{
    const char *ptr_info;
    char *info;

    TRIGGER_CALLBACK_CB_INIT(NULL);

    TRIGGER_CALLBACK_CB_NEW_EXTRA_VARS;

    /* add data in hashtable used for conditions/replace/command */
    trigger_callback_set_common_vars (trigger, ctx.extra_vars);
    weechat_hashtable_set (ctx.extra_vars, "tg_info_name", info_name);
    weechat_hashtable_set (ctx.extra_vars, "tg_arguments", arguments);
    weechat_hashtable_set (ctx.extra_vars, "tg_info", "");

    /* execute the trigger (conditions, regex, command) */
    (void) trigger_callback_execute (trigger, &ctx);

end:
    ptr_info = weechat_hashtable_get (ctx.extra_vars, "tg_info");
    info = (ptr_info) ? strdup (ptr_info) : NULL;

    TRIGGER_CALLBACK_CB_END(info);
}

/*
 * Callback for an info_hashtable hooked.
 */

struct t_hashtable *
trigger_callback_info_hashtable_cb (const void *pointer, void *data,
                                    const char *info_name,
                                    struct t_hashtable *hashtable)
{
    struct t_hashtable *ret_hashtable;
    struct t_weelist_item *ptr_item;
    const char *ptr_key;

    TRIGGER_CALLBACK_CB_INIT(NULL);

    ret_hashtable = NULL;

    TRIGGER_CALLBACK_CB_NEW_POINTERS;
    TRIGGER_CALLBACK_CB_NEW_VARS_UPDATED;

    ctx.extra_vars = weechat_hashtable_dup (hashtable);

    /* add data in hashtable used for conditions/replace/command */
    trigger_callback_set_common_vars (trigger, ctx.extra_vars);
    weechat_hashtable_set (ctx.extra_vars, "tg_info_name", info_name);

    /* execute the trigger (conditions, regex, command) */
    (void) trigger_callback_execute (trigger, &ctx);

    ret_hashtable = weechat_hashtable_new (32,
                                           WEECHAT_HASHTABLE_STRING,
                                           WEECHAT_HASHTABLE_STRING,
                                           NULL, NULL);
    if (ret_hashtable)
    {
        /* copy updated variables into the result "ret_hashtable" */
        for (ptr_item = weechat_list_get (ctx.vars_updated, 0); ptr_item;
             ptr_item = weechat_list_next (ptr_item))
        {
            ptr_key = weechat_list_string (ptr_item);
            if (weechat_hashtable_has_key (ctx.extra_vars, ptr_key))
            {
                weechat_hashtable_set (
                    ret_hashtable,
                    ptr_key,
                    weechat_hashtable_get (ctx.extra_vars, ptr_key));
            }
        }
    }

end:
    TRIGGER_CALLBACK_CB_END(ret_hashtable);
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
        NULL, NULL);
    if (trigger_callback_hashtable_options_conditions)
    {
        weechat_hashtable_set (trigger_callback_hashtable_options_conditions,
                               "type", "condition");
    }
}

/*
 * Ends trigger callback.
 */

void
trigger_callback_end ()
{
    if (trigger_callback_hashtable_options_conditions)
        weechat_hashtable_free (trigger_callback_hashtable_options_conditions);
}
