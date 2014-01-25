/*
 * trigger-completion.c - completion for trigger commands
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


/*
 * Adds triggers to completion list.
 */

int
trigger_completion_triggers_cb (void *data, const char *completion_item,
                                struct t_gui_buffer *buffer,
                                struct t_gui_completion *completion)
{
    struct t_trigger *ptr_trigger;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_trigger = triggers; ptr_trigger;
         ptr_trigger = ptr_trigger->next_trigger)
    {
        weechat_hook_completion_list_add (completion, ptr_trigger->name,
                                          0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds options for triggers to completion list.
 */

int
trigger_completion_options_cb (void *data, const char *completion_item,
                               struct t_gui_buffer *buffer,
                               struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; i < TRIGGER_NUM_OPTIONS; i++)
    {
        weechat_hook_completion_list_add (completion,
                                          trigger_option_string[i],
                                          0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds value of a trigger option to completion list.
 */

int
trigger_completion_option_value_cb (void *data, const char *completion_item,
                                    struct t_gui_buffer *buffer,
                                    struct t_gui_completion *completion)
{
    const char *args;
    char **argv;
    int argc, index_option;
    struct t_trigger *ptr_trigger;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    args = weechat_hook_completion_get_string (completion, "args");
    if (!args)
        return WEECHAT_RC_OK;

    argv = weechat_string_split (args, " ", 0, 0, &argc);
    if (!argv)
        return WEECHAT_RC_OK;

    if (argc >= 3)
    {
        ptr_trigger = trigger_search (argv[1]);
        if (ptr_trigger)
        {
            if (weechat_strcasecmp (argv[2], "name") == 0)
            {
                weechat_hook_completion_list_add (completion,
                                                  ptr_trigger->name,
                                                  0,
                                                  WEECHAT_LIST_POS_BEGINNING);
            }
            else
            {
                index_option = trigger_search_option (argv[2]);
                if (index_option >= 0)
                {
                    weechat_hook_completion_list_add (completion,
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
trigger_completion_hooks_cb (void *data, const char *completion_item,
                             struct t_gui_buffer *buffer,
                             struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; i < TRIGGER_NUM_HOOK_TYPES; i++)
    {
        weechat_hook_completion_list_add (completion,
                                          trigger_hook_type_string[i],
                                          0, WEECHAT_LIST_POS_SORT);
    }

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
                             &trigger_completion_triggers_cb, NULL);
    weechat_hook_completion ("trigger_options",
                             N_("options for triggers"),
                             &trigger_completion_options_cb, NULL);
    weechat_hook_completion ("trigger_option_value",
                             N_("value of a trigger option"),
                             &trigger_completion_option_value_cb, NULL);
    weechat_hook_completion ("trigger_hooks",
                             N_("hooks for triggers"),
                             &trigger_completion_hooks_cb, NULL);
}
