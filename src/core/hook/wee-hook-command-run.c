/*
 * wee-hook-command-run.c - WeeChat command_run hook
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include "../wee-hook.h"
#include "../wee-infolist.h"
#include "../wee-log.h"
#include "../wee-string.h"
#include "../wee-utf8.h"
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
    int rc, hook_matching, length;
    char *command2;
    const char *ptr_string, *ptr_command;

    if (!weechat_hooks[HOOK_TYPE_COMMAND_RUN])
        return WEECHAT_RC_OK;

    ptr_command = command;
    command2 = NULL;

    if (command[0] != '/')
    {
        ptr_string = utf8_next_char (command);
        length = 1 + strlen (ptr_string) + 1;
        command2 = malloc (length);
        if (command2)
        {
            snprintf (command2, length, "/%s", ptr_string);
            ptr_command = command2;
        }
    }

    ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND_RUN];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running
            && HOOK_COMMAND_RUN(ptr_hook, command))
        {
            hook_matching = string_match (ptr_command,
                                          HOOK_COMMAND_RUN(ptr_hook, command),
                                          1);

            if (!hook_matching
                && !strchr (HOOK_COMMAND_RUN(ptr_hook, command), ' '))
            {
                length = strlen (HOOK_COMMAND_RUN(ptr_hook, command));
                hook_matching = ((string_strncmp (ptr_command,
                                                  HOOK_COMMAND_RUN(ptr_hook, command),
                                                  utf8_strlen (HOOK_COMMAND_RUN(ptr_hook, command))) == 0)
                                 && ((ptr_command[length] == ' ')
                                     || (ptr_command[length] == '\0')));
            }

            if (hook_matching)
            {
                ptr_hook->running = 1;
                rc = (HOOK_COMMAND_RUN(ptr_hook, callback)) (
                    ptr_hook->callback_pointer,
                    ptr_hook->callback_data,
                    buffer,
                    ptr_command);
                ptr_hook->running = 0;
                if (rc == WEECHAT_RC_OK_EAT)
                {
                    if (command2)
                        free (command2);
                    return rc;
                }
            }
        }

        ptr_hook = next_hook;
    }

    if (command2)
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
    log_printf ("    callback. . . . . . . : 0x%lx", HOOK_COMMAND_RUN(hook, callback));
    log_printf ("    command . . . . . . . : '%s'",  HOOK_COMMAND_RUN(hook, command));
}
