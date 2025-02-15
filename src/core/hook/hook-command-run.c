/*
 * hook-command-run.c - WeeChat command_run hook
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../weechat.h"
#include "../core-hook.h"
#include "../core-infolist.h"
#include "../core-log.h"
#include "../core-string.h"
#include "../core-utf8.h"
#include "../../plugins/plugin.h"


/*
 * Returns description of hook.
 *
 * Note: result must be freed after use.
 */

char *
hook_command_run_get_description (struct t_hook *hook)
{
    return strdup (HOOK_COMMAND_RUN(hook, command));
}

/*
 * Hooks a command when it's run by WeeChat.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_command_run (struct t_weechat_plugin *plugin,
                  const char *command,
                  t_hook_callback_command_run *callback,
                  const void *callback_pointer,
                  void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_command_run *new_hook_command_run;
    int priority;
    const char *ptr_command;

    if (!callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_command_run = malloc (sizeof (*new_hook_command_run));
    if (!new_hook_command_run)
    {
        free (new_hook);
        return NULL;
    }

    string_get_priority_and_name (command, &priority, &ptr_command,
                                  HOOK_PRIORITY_DEFAULT);
    hook_init_data (new_hook, plugin, HOOK_TYPE_COMMAND_RUN, priority,
                    callback_pointer, callback_data);

    new_hook->hook_data = new_hook_command_run;
    new_hook_command_run->callback = callback;
    new_hook_command_run->command = strdup ((ptr_command) ? ptr_command :
                                            ((command) ? command : ""));
    new_hook_command_run->keep_spaces_right = 0;

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Executes a command_run hook.
 */

int
hook_command_run_exec (struct t_gui_buffer *buffer, const char *command)
{
    struct t_hook *ptr_hook, *next_hook;
    struct t_hook_exec_cb hook_exec_cb;
    int rc, hook_matching, i, length;
    char *command2;
    const char *ptr_string;

    if (!weechat_hooks[HOOK_TYPE_COMMAND_RUN])
        return WEECHAT_RC_OK;

    if (command[0] != '/')
    {
        ptr_string = utf8_next_char (command);
        if (string_asprintf (&command2, "/%s", ptr_string) < 0)
            return WEECHAT_RC_ERROR;
    }
    else
    {
        command2 = strdup (command);
    }

    ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND_RUN];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running
            && HOOK_COMMAND_RUN(ptr_hook, command))
        {
            hook_matching = string_match (command2,
                                          HOOK_COMMAND_RUN(ptr_hook, command),
                                          1);

            if (!hook_matching
                && !strchr (HOOK_COMMAND_RUN(ptr_hook, command), ' '))
            {
                length = strlen (HOOK_COMMAND_RUN(ptr_hook, command));
                hook_matching = ((string_strncmp (command2,
                                                  HOOK_COMMAND_RUN(ptr_hook, command),
                                                  utf8_strlen (HOOK_COMMAND_RUN(ptr_hook, command))) == 0)
                                 && ((command2[length] == ' ')
                                     || (command2[length] == '\0')));
            }

            if (hook_matching)
            {
                /* remove trailing spaces */
                if (!HOOK_COMMAND_RUN(ptr_hook, keep_spaces_right))
                {
                    i = strlen (command2) - 1;
                    while ((i >= 0) && (command2[i] == ' '))
                    {
                        command2[i] = '\0';
                        i--;
                    }
                }
                /* execute the command_run hook */
                hook_callback_start (ptr_hook, &hook_exec_cb);
                rc = (HOOK_COMMAND_RUN(ptr_hook, callback)) (
                    ptr_hook->callback_pointer,
                    ptr_hook->callback_data,
                    buffer,
                    command2);
                hook_callback_end (ptr_hook, &hook_exec_cb);
                if (rc == WEECHAT_RC_OK_EAT)
                {
                    free (command2);
                    return rc;
                }
            }
        }

        ptr_hook = next_hook;
    }

    free (command2);

    return WEECHAT_RC_OK;
}

/*
 * Frees data in a command_run hook.
 */

void
hook_command_run_free_data (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    if (HOOK_COMMAND_RUN(hook, command))
    {
        free (HOOK_COMMAND_RUN(hook, command));
        HOOK_COMMAND_RUN(hook, command) = NULL;
    }

    free (hook->hook_data);
    hook->hook_data = NULL;
}

/*
 * Adds command_run hook data in the infolist item.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_command_run_add_to_infolist (struct t_infolist_item *item,
                                  struct t_hook *hook)
{
    if (!item || !hook || !hook->hook_data)
        return 0;

    if (!infolist_new_var_pointer (item, "callback", HOOK_COMMAND_RUN(hook, callback)))
        return 0;
    if (!infolist_new_var_string (item, "command", HOOK_COMMAND_RUN(hook, command)))
        return 0;
    if (!infolist_new_var_integer (item, "keep_spaces_right", HOOK_COMMAND_RUN(hook, keep_spaces_right)))
        return 0;

    return 1;
}

/*
 * Prints command_run hook data in WeeChat log file (usually for crash dump).
 */

void
hook_command_run_print_log (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    log_printf ("  command_run data:");
    log_printf ("    callback. . . . . . . : %p", HOOK_COMMAND_RUN(hook, callback));
    log_printf ("    command . . . . . . . : '%s'", HOOK_COMMAND_RUN(hook, command));
    log_printf ("    keep_spaces_right . . : %d", HOOK_COMMAND_RUN(hook, keep_spaces_right));
}
