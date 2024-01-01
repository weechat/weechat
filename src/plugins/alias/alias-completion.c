/*
 * alias-completion.c - completion for alias commands
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include "alias.h"


/*
 * Adds list of aliases to completion list.
 */

int
alias_completion_alias_cb (const void *pointer, void *data,
                           const char *completion_item,
                           struct t_gui_buffer *buffer,
                           struct t_gui_completion *completion)
{
    struct t_alias *ptr_alias;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_alias = alias_list; ptr_alias;
         ptr_alias = ptr_alias->next_alias)
    {
        weechat_completion_list_add (completion, ptr_alias->name,
                                     0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds value of an alias to completion list.
 */

int
alias_completion_alias_value_cb (const void *pointer, void *data,
                                 const char *completion_item,
                                 struct t_gui_buffer *buffer,
                                 struct t_gui_completion *completion)
{
    const char *args;
    char **argv, *alias_name;
    int argc;
    struct t_alias *ptr_alias;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    args = weechat_completion_get_string (completion, "args");
    if (args)
    {
        argv = weechat_string_split (args, " ", NULL,
                                     WEECHAT_STRING_SPLIT_STRIP_LEFT
                                     | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                     | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                     0, &argc);
        if (argv)
        {
            if (argc > 0)
                alias_name = strdup (argv[argc - 1]);
            else
                alias_name = strdup (args);

            if (alias_name)
            {
                ptr_alias = alias_search (alias_name);
                if (ptr_alias)
                {
                    weechat_completion_list_add (completion,
                                                 ptr_alias->command,
                                                 0,
                                                 WEECHAT_LIST_POS_BEGINNING);
                }
                free (alias_name);
            }
            weechat_string_free_split (argv);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Hooks completions.
 */

void
alias_completion_init ()
{
    weechat_hook_completion ("alias", N_("list of aliases"),
                             &alias_completion_alias_cb, NULL, NULL);
    weechat_hook_completion ("alias_value", N_("value of alias"),
                             &alias_completion_alias_value_cb, NULL, NULL);
}
