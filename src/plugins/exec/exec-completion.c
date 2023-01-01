/*
 * exec-completion.c - completion for exec commands
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
#include "exec.h"


/*
 * Adds executed commands ids to completion list.
 */

int
exec_completion_commands_ids_cb (const void *pointer, void *data,
                                 const char *completion_item,
                                 struct t_gui_buffer *buffer,
                                 struct t_gui_completion *completion)
{
    struct t_exec_cmd *ptr_exec_cmd;
    char str_number[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_exec_cmd = exec_cmds; ptr_exec_cmd;
         ptr_exec_cmd = ptr_exec_cmd->next_cmd)
    {
        snprintf (str_number, sizeof (str_number),
                  "%ld", ptr_exec_cmd->number);
        weechat_completion_list_add (completion, str_number,
                                     0, WEECHAT_LIST_POS_SORT);
        if (ptr_exec_cmd->name)
        {
            weechat_completion_list_add (completion, ptr_exec_cmd->name,
                                         0, WEECHAT_LIST_POS_SORT);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Hooks completions.
 */

void
exec_completion_init ()
{
    weechat_hook_completion ("exec_commands_ids",
                             N_("ids (numbers and names) of executed commands"),
                             &exec_completion_commands_ids_cb, NULL, NULL);
}
