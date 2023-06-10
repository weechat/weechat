/*
 * trigger-completion.c - completion for trigger commands
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

#include "../weechat-plugin.h"
#include "trigger.h"
#include "trigger-config.h"


/*
 * Adds triggers to completion list.
 */

int
trigger_completion_triggers_cb (const void *pointer, void *data,
                                const char *completion_item,
                                struct t_gui_buffer *buffer,
                                struct t_gui_completion *completion)
{
    struct t_trigger *ptr_trigger;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_trigger = triggers; ptr_trigger;
         ptr_trigger = ptr_trigger->next_trigger)
    {
        weechat_completion_list_add (completion, ptr_trigger->name,
                                     0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds default triggers to completion list.
 */

int
trigger_completion_triggers_default_cb (const void *pointer, void *data,

                                        const char *completion_item,
                                        struct t_gui_buffer *buffer,
                                        struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; trigger_config_default_list[i][0]; i++)
    {
        weechat_completion_list_add (completion,
                                     trigger_config_default_list[i][0],
                                     0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds disabled triggers to completion list.
 */

int
trigger_completion_triggers_disabled_cb (const void *pointer, void *data,
                                         const char *completion_item,
                                         struct t_gui_buffer *buffer,
                                         struct t_gui_completion *completion)
{
    struct t_trigger *ptr_trigger;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_trigger = triggers; ptr_trigger;
         ptr_trigger = ptr_trigger->next_trigger)
    {
        if (!weechat_config_boolean (ptr_trigger->options[TRIGGER_OPTION_ENABLED]))
        {
            weechat_completion_list_add (completion, ptr_trigger->name,
                                         0, WEECHAT_LIST_POS_SORT);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds enabled triggers to completion list.
 */

int
trigger_completion_triggers_enabled_cb (const void *pointer, void *data,
                                        const char *completion_item,
                                        struct t_gui_buffer *buffer,
                                        struct t_gui_completion *completion)
{
    struct t_trigger *ptr_trigger;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_trigger = triggers; ptr_trigger;
         ptr_trigger = ptr_trigger->next_trigger)
    {
        if (weechat_config_boolean (ptr_trigger->options[TRIGGER_OPTION_ENABLED]))
        {
            weechat_completion_list_add (completion, ptr_trigger->name,
                                         0, WEECHAT_LIST_POS_SORT);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds options for triggers to completion list.
 */

int
trigger_completion_options_cb (const void *pointer, void *data,
                               const char *completion_item,
                               struct t_gui_buffer *buffer,
                               struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; i < TRIGGER_NUM_OPTIONS; i++)
    {
        weechat_completion_list_add (completion,
                                     trigger_option_string[i],
                                     0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds value of a trigger option to completion list.
 */

int
trigger_completion_option_value_cb (const void *pointer, void *data,
                                    const char *completion_item,
                                    struct t_gui_buffer *buffer,
                                    struct t_gui_completion *completion)
{
    const char *args;
    char **argv;
    int argc, index_option;
    struct t_trigger *ptr_trigger;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    args = weechat_completion_get_string (completion, "args");
    if (!args)
        return WEECHAT_RC_OK;

    argv = weechat_string_split (args, " ", NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                 0, &argc);
    if (!argv)
        return WEECHAT_RC_OK;

    if (argc >= 3)
    {
        ptr_trigger = trigger_search (argv[1]);
        if (ptr_trigger)
        {
            if (weechat_strcasecmp (argv[2], "name") == 0)
            {
                weechat_completion_list_add (completion,
                                             ptr_trigger->name,
                                             0,
                                             WEECHAT_LIST_POS_BEGINNING);
            }
            else
            {
                index_option = trigger_search_option (argv[2]);
                if (index_option >= 0)
                {
                    weechat_completion_list_add (
                        completion,
                        weechat_config_string (ptr_trigger->options[index_option]),
                        0,
                        WEECHAT_LIST_POS_BEGINNING);
                }
            }
        }
    }

    weechat_string_free_split (argv);

    return WEECHAT_RC_OK;
}

/*
 * Adds hooks for triggers to completion list.
 */

int
trigger_completion_hooks_cb (const void *pointer, void *data,
                             const char *completion_item,
                             struct t_gui_buffer *buffer,
                             struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; i < TRIGGER_NUM_HOOK_TYPES; i++)
    {
        weechat_completion_list_add (completion,
                                     trigger_hook_type_string[i],
                                     0, WEECHAT_LIST_POS_END);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds hooks for filtering triggers to completion list.
 */

int
trigger_completion_hooks_filter_cb (const void *pointer, void *data,
                                    const char *completion_item,
                                    struct t_gui_buffer *buffer,
                                    struct t_gui_completion *completion)
{
    int i;
    char str_hook[128];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; i < TRIGGER_NUM_HOOK_TYPES; i++)
    {
        snprintf (str_hook, sizeof (str_hook),
                  "@%s", trigger_hook_type_string[i]);
        weechat_completion_list_add (completion, str_hook,
                                     0, WEECHAT_LIST_POS_END);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds a word with quotes around to completion list.
 */

void
trigger_completion_add_quoted_word (struct t_gui_completion *completion,
                                    const char *word)
{
    char *temp;
    int length;

    length = 1 + strlen (word) + 1 + 1;
    temp = malloc (length);
    if (!temp)
        return;

    snprintf (temp, length, "\"%s\"", word);
    weechat_completion_list_add (completion, temp, 0, WEECHAT_LIST_POS_END);

    free (temp);
}

/*
 * Adds a default string to completion list, depending on hook type.
 *
 * If split is not NULL, the default string found is split using this separator,
 * and therefore many words can be added to completion list.
 */

void
trigger_completion_add_default_for_hook (struct t_gui_completion *completion,
                                         char *default_strings[], char *split)
{
    const char *args;
    char **argv, **items;
    int argc, num_items, type, i;

    args = weechat_completion_get_string (completion, "args");
    if (!args)
        return;

    argv = weechat_string_split (args, " ", NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                 0, &argc);
    if (!argv)
        return;

    if (argc >= 3)
    {
        type = trigger_search_hook_type (argv[2]);
        if (type >= 0)
        {
            if (default_strings[type][0] && split && split[0])
            {
                items = weechat_string_split (
                    default_strings[type],
                    split,
                    NULL,
                    WEECHAT_STRING_SPLIT_STRIP_LEFT
                    | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                    | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                    0,
                    &num_items);
                if (items)
                {
                    for (i = 0; i < num_items; i++)
                    {
                        trigger_completion_add_quoted_word (completion,
                                                            items[i]);
                    }
                    weechat_string_free_split (items);
                }
            }
            else
            {
                trigger_completion_add_quoted_word (completion,
                                                    default_strings[type]);
            }
        }
    }

    weechat_string_free_split (argv);
}

/*
 * Adds default arguments for hook to completion list.
 */

int
trigger_completion_hook_arguments_cb (const void *pointer, void *data,
                                      const char *completion_item,
                                      struct t_gui_buffer *buffer,
                                      struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    trigger_completion_add_default_for_hook (completion,
                                             trigger_hook_default_arguments,
                                             NULL);
    weechat_completion_list_add (completion, "\"\"", 0, WEECHAT_LIST_POS_END);

    return WEECHAT_RC_OK;
}

/*
 * Adds default conditions for hook to completion list.
 */

int
trigger_completion_hook_conditions_cb (const void *pointer, void *data,
                                       const char *completion_item,
                                       struct t_gui_buffer *buffer,
                                       struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    weechat_completion_list_add (completion,
                                 "\"" TRIGGER_HOOK_DEFAULT_CONDITIONS "\"",
                                 0,
                                 WEECHAT_LIST_POS_END);
    weechat_completion_list_add (completion, "\"\"", 0, WEECHAT_LIST_POS_END);

    return WEECHAT_RC_OK;
}

/*
 * Adds default regular expression for hook to completion list.
 */

int
trigger_completion_hook_regex_cb (const void *pointer, void *data,
                                  const char *completion_item,
                                  struct t_gui_buffer *buffer,
                                  struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    weechat_completion_list_add (completion,
                                 "\"" TRIGGER_HOOK_DEFAULT_REGEX "\"",
                                 0,
                                 WEECHAT_LIST_POS_END);
    weechat_completion_list_add (completion, "\"\"", 0, WEECHAT_LIST_POS_END);

    return WEECHAT_RC_OK;
}

/*
 * Adds default command for hook to completion list.
 */

int
trigger_completion_hook_command_cb (const void *pointer, void *data,
                                    const char *completion_item,
                                    struct t_gui_buffer *buffer,
                                    struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    weechat_completion_list_add (completion,
                                 "\"" TRIGGER_HOOK_DEFAULT_COMMAND "\"",
                                 0,
                                 WEECHAT_LIST_POS_END);
    weechat_completion_list_add (completion, "\"\"", 0, WEECHAT_LIST_POS_END);

    return WEECHAT_RC_OK;
}

/*
 * Adds default return code(s) for hook to completion list.
 */

int
trigger_completion_hook_rc_cb (const void *pointer, void *data,
                               const char *completion_item,
                               struct t_gui_buffer *buffer,
                               struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    trigger_completion_add_default_for_hook (completion,
                                             trigger_hook_default_rc,
                                             ",");

    return WEECHAT_RC_OK;
}

/*
 * Adds default post actions to completion list.
 */

int
trigger_completion_post_action_cb (const void *pointer, void *data,
                                   const char *completion_item,
                                   struct t_gui_buffer *buffer,
                                   struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; i < TRIGGER_NUM_POST_ACTIONS; i++)
    {
        trigger_completion_add_quoted_word (completion,
                                            trigger_post_action_string[i]);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds arguments for commands that add a trigger.
 */

int
trigger_completion_add_arguments_cb (const void *pointer, void *data,
                                     const char *completion_item,
                                     struct t_gui_buffer *buffer,
                                     struct t_gui_completion *completion)
{
    const char *args, *base_word;
    char **sargv;
    int sargc, arg_complete;

    args = weechat_completion_get_string (completion, "args");
    if (!args)
        return WEECHAT_RC_OK;

    base_word = weechat_completion_get_string (completion, "base_word");

    sargv = weechat_string_split_shell (args, &sargc);
    if (!sargv)
        return WEECHAT_RC_OK;

    arg_complete = sargc;
    if (base_word && base_word[0])
        arg_complete--;

    switch (arg_complete)
    {
        case 1:
            trigger_completion_triggers_cb (pointer, data, completion_item,
                                            buffer, completion);
            break;
        case 2:
            trigger_completion_hooks_cb (pointer, data, completion_item,
                                         buffer, completion);
            break;
        case 3:
            trigger_completion_hook_arguments_cb (pointer, data,
                                                  completion_item, buffer,
                                                  completion);
            break;
        case 4:
            trigger_completion_hook_conditions_cb (pointer, data,
                                                   completion_item, buffer,
                                                   completion);
            break;
        case 5:
            trigger_completion_hook_regex_cb (pointer, data, completion_item,
                                              buffer, completion);
            break;
        case 6:
            trigger_completion_hook_command_cb (pointer, data, completion_item,
                                                buffer, completion);
            break;
        case 7:
            trigger_completion_hook_rc_cb (pointer, data, completion_item,
                                           buffer, completion);
            break;
        case 8:
            trigger_completion_post_action_cb (pointer, data, completion_item,
                                               buffer, completion);
            break;
    }

    weechat_string_free_split (sargv);

    return WEECHAT_RC_OK;
}

/*
 * Hooks completions.
 */

void
trigger_completion_init ()
{
    weechat_hook_completion ("trigger_names",
                             N_("triggers"),
                             &trigger_completion_triggers_cb, NULL, NULL);
    weechat_hook_completion ("trigger_names_default",
                             N_("default triggers"),
                             &trigger_completion_triggers_default_cb, NULL, NULL);
    weechat_hook_completion ("trigger_names_disabled",
                             N_("disabled triggers"),
                             &trigger_completion_triggers_disabled_cb, NULL, NULL);
    weechat_hook_completion ("trigger_names_enabled",
                             N_("enabled triggers"),
                             &trigger_completion_triggers_enabled_cb, NULL, NULL);
    weechat_hook_completion ("trigger_options",
                             N_("options for triggers"),
                             &trigger_completion_options_cb, NULL, NULL);
    weechat_hook_completion ("trigger_option_value",
                             N_("value of a trigger option"),
                             &trigger_completion_option_value_cb, NULL, NULL);
    weechat_hook_completion ("trigger_hooks",
                             N_("hooks for triggers"),
                             &trigger_completion_hooks_cb, NULL, NULL);
    weechat_hook_completion ("trigger_hooks_filter",
                             N_("hooks for triggers (for filter in monitor buffer)"),
                             &trigger_completion_hooks_filter_cb, NULL, NULL);
    weechat_hook_completion ("trigger_hook_arguments",
                             N_("default arguments for a hook"),
                             &trigger_completion_hook_arguments_cb, NULL, NULL);
    weechat_hook_completion ("trigger_hook_conditions",
                             N_("default conditions for a hook"),
                             &trigger_completion_hook_conditions_cb, NULL, NULL);
    weechat_hook_completion ("trigger_hook_regex",
                             N_("default regular expression for a hook"),
                             &trigger_completion_hook_regex_cb, NULL, NULL);
    weechat_hook_completion ("trigger_hook_command",
                             N_("default command for a hook"),
                             &trigger_completion_hook_command_cb, NULL, NULL);
    weechat_hook_completion ("trigger_hook_rc",
                             N_("default return codes for hook callback"),
                             &trigger_completion_hook_rc_cb, NULL, NULL);
    weechat_hook_completion ("trigger_post_action",
                             N_("trigger post actions"),
                             &trigger_completion_post_action_cb, NULL, NULL);
    weechat_hook_completion ("trigger_add_arguments",
                             N_("arguments for command that adds a trigger: "
                                "trigger name, hooks, hook arguments, "
                                "hook conditions, hook regex, hook command, "
                                "hook return code, post actions"),
                             &trigger_completion_add_arguments_cb, NULL, NULL);
}
