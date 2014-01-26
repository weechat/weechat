/*
 * trigger-config.c - trigger configuration options (file trigger.conf)
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
#include "trigger-config.h"


struct t_config_file *trigger_config_file = NULL;
struct t_config_section *trigger_config_section_trigger = NULL;

/* trigger config, look section */

struct t_config_option *trigger_config_look_test;

/* trigger config, color section */

struct t_config_option *trigger_config_color_regex;
struct t_config_option *trigger_config_color_replace;


/*
 * Callback called when trigger option "enabled" is changed.
 */

void
trigger_config_change_enabled (void *data, struct t_config_option *option)
{
    struct t_trigger *ptr_trigger;

    /* make C compiler happy */
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
 * Callback called when trigger option "arguments" is changed.
 */

void
trigger_config_change_arguments (void *data, struct t_config_option *option)
{
    struct t_trigger *ptr_trigger;

    /* make C compiler happy */
    (void) data;

    ptr_trigger = trigger_search_with_option (option);
    if (!ptr_trigger)
        return;

    trigger_hook (ptr_trigger);
}

/*
 * Callback called when trigger option "regex" is changed.
 */

void
trigger_config_change_regex (void *data, struct t_config_option *option)
{
    struct t_trigger *ptr_trigger;

    /* make C compiler happy */
    (void) data;

    ptr_trigger = trigger_search_with_option (option);
    if (!ptr_trigger)
        return;

    trigger_set_regex (ptr_trigger);
}

/*
 * Creates an option for a trigger.
 *
 * Returns pointer to new option, NULL if error.
 */

struct t_config_option *
trigger_config_create_option (const char *trigger_name, int index_option,
                              const char *value)
{
    struct t_config_option *ptr_option;
    int length;
    char *option_name;

    ptr_option = NULL;

    length = strlen (trigger_name) + 1 +
        strlen (trigger_option_string[index_option]) + 1;
    option_name = malloc (length);
    if (!option_name)
        return NULL;

    snprintf (option_name, length, "%s.%s",
              trigger_name, trigger_option_string[index_option]);

    switch (index_option)
    {
        case TRIGGER_OPTION_ENABLED:
            ptr_option = weechat_config_new_option (
                trigger_config_file, trigger_config_section_trigger,
                option_name, "boolean",
                N_("if disabled, the hooks are removed from trigger, so it is "
                   "not called any more"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, &trigger_config_change_enabled, NULL, NULL, NULL);
            break;
        case TRIGGER_OPTION_HOOK:
            ptr_option = weechat_config_new_option (
                trigger_config_file, trigger_config_section_trigger,
                option_name, "integer",
                N_("type of hook used"),
                "signal|hsignal|modifier|print|timer", 0, 0, value, NULL, 0,
                NULL, NULL, NULL, NULL, NULL, NULL);
            break;
        case TRIGGER_OPTION_ARGUMENTS:
            ptr_option = weechat_config_new_option (
                trigger_config_file, trigger_config_section_trigger,
                option_name, "string",
                N_("arguments for the hook (depend on the hook type); for "
                   "signal/hsignal/modifier: name[;name...], for print: "
                   "buffer;tags;strip_colors;message, for timer: "
                   "interval;align_second;max_calls"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, &trigger_config_change_arguments, NULL, NULL, NULL);
            break;
        case TRIGGER_OPTION_CONDITIONS:
            ptr_option = weechat_config_new_option (
                trigger_config_file, trigger_config_section_trigger,
                option_name, "string",
                N_("condition(s) for running the command (it is checked in "
                   "hook callback) (note: content is evaluated when trigger is "
                   "run, see /help eval)"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, NULL, NULL, NULL, NULL);
            break;
        case TRIGGER_OPTION_REGEX:
            ptr_option = weechat_config_new_option (
                trigger_config_file, trigger_config_section_trigger,
                option_name, "string",
                N_("replace text with a POSIX extended regular expression (it "
                   "is done only if conditions are OK, and before running the "
                   "command) (note: content is evaluated on trigger creation, "
                   "see /help eval)"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, &trigger_config_change_regex, NULL, NULL, NULL);
            break;
        case TRIGGER_OPTION_COMMAND:
            ptr_option = weechat_config_new_option (
                trigger_config_file, trigger_config_section_trigger,
                option_name, "string",
                N_("command run if conditions are OK, after regex replacements"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, NULL, NULL, NULL, NULL);
            break;
        case TRIGGER_OPTION_RETURN_CODE:
            ptr_option = weechat_config_new_option (
                trigger_config_file, trigger_config_section_trigger,
                option_name, "integer",
                N_("return code for hook callback (see plugin API reference to "
                   "know where ok_eat or error can be used efficiently)"),
                "ok|ok_eat|error", 0, 0, value, NULL, 0,
                NULL, NULL, NULL, NULL, NULL, NULL);
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

    new_option = trigger_config_create_option (temp_trigger->name,
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
trigger_config_use_temp_triggers ()
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
                    trigger_config_create_option (ptr_temp_trigger->name,
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

        if (triggers_temp->name)
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
trigger_config_trigger_read_cb (void *data, struct t_config_file *config_file,
                                struct t_config_section *section,
                                const char *option_name, const char *value)
{
    char *pos_option, *trigger_name;
    struct t_trigger *ptr_temp_trigger;
    int index_option;

    /* make C compiler happy */
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
 * Reloads trigger configuration file.
 */

int
trigger_config_reload_cb (void *data, struct t_config_file *config_file)
{
    int rc;

    /* make C compiler happy */
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
trigger_config_init ()
{
    struct t_config_section *ptr_section;

    trigger_config_file = weechat_config_new (TRIGGER_CONFIG_NAME,
                                              &trigger_config_reload_cb, NULL);
    if (!trigger_config_file)
        return 0;

    /* look */
    ptr_section = weechat_config_new_section (trigger_config_file, "look",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (trigger_config_file);
        return 0;
    }

    trigger_config_look_test = weechat_config_new_option (
        trigger_config_file, ptr_section,
        "test", "boolean",
        "",
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        NULL, NULL, NULL, NULL);

    /* color */
    ptr_section = weechat_config_new_section (trigger_config_file, "color",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (trigger_config_file);
        return 0;
    }

    trigger_config_color_regex = weechat_config_new_option (
        trigger_config_file, ptr_section,
        "regex", "color",
        N_("text color for regular expressions"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL);
    trigger_config_color_replace = weechat_config_new_option (
        trigger_config_file, ptr_section,
        "replace", "color",
        N_("text color for replacement text (for regular expressions)"),
        NULL, 0, 0, "cyan", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL);

    /* trigger */
    ptr_section = weechat_config_new_section (trigger_config_file,
                                              TRIGGER_CONFIG_SECTION_TRIGGER,
                                              0, 0,
                                              &trigger_config_trigger_read_cb, NULL,
                                              NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (trigger_config_file);
        return 0;
    }

    trigger_config_section_trigger = ptr_section;

    return 1;
}

/*
 * Reads trigger configuration file.
 */

int
trigger_config_read ()
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
trigger_config_write ()
{
    return weechat_config_write (trigger_config_file);
}

/*
 * Frees trigger configuration.
 */

void
trigger_config_free ()
{
    weechat_config_free (trigger_config_file);
}
