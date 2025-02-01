/*
 * hook-config.c - WeeChat config hook
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


/*
 * Returns description of hook.
 *
 * Note: result must be freed after use.
 */

char *
hook_config_get_description (struct t_hook *hook)
{
    return strdup (HOOK_CONFIG(hook, option));
}

/*
 * Hooks a configuration option.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_config (struct t_weechat_plugin *plugin, const char *option,
             t_hook_callback_config *callback,
             const void *callback_pointer,
             void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_config *new_hook_config;
    int priority;
    const char *ptr_option;

    if (!callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_config = malloc (sizeof (*new_hook_config));
    if (!new_hook_config)
    {
        free (new_hook);
        return NULL;
    }

    string_get_priority_and_name (option, &priority, &ptr_option,
                                  HOOK_PRIORITY_DEFAULT);
    hook_init_data (new_hook, plugin, HOOK_TYPE_CONFIG, priority,
                    callback_pointer, callback_data);

    new_hook->hook_data = new_hook_config;
    new_hook_config->callback = callback;
    new_hook_config->option = strdup ((ptr_option) ? ptr_option :
                                      ((option) ? option : ""));

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Executes a config hook.
 */

void
hook_config_exec (const char *option, const char *value)
{
    struct t_hook *ptr_hook, *next_hook;
    struct t_hook_exec_cb hook_exec_cb;

    hook_exec_start ();

    ptr_hook = weechat_hooks[HOOK_TYPE_CONFIG];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (!HOOK_CONFIG(ptr_hook, option)
                || (string_match (option, HOOK_CONFIG(ptr_hook, option), 0))))
        {
            hook_callback_start (ptr_hook, &hook_exec_cb);
            (void) (HOOK_CONFIG(ptr_hook, callback))
                (ptr_hook->callback_pointer,
                 ptr_hook->callback_data,
                 option,
                 value);
            hook_callback_end (ptr_hook, &hook_exec_cb);
        }

        ptr_hook = next_hook;
    }

    hook_exec_end ();
}

/*
 * Frees data in a config hook.
 */

void
hook_config_free_data (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    if (HOOK_CONFIG(hook, option))
    {
        free (HOOK_CONFIG(hook, option));
        HOOK_CONFIG(hook, option) = NULL;
    }

    free (hook->hook_data);
    hook->hook_data = NULL;
}

/*
 * Adds config hook data in the infolist item.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_config_add_to_infolist (struct t_infolist_item *item,
                             struct t_hook *hook)
{
    if (!item || !hook || !hook->hook_data)
        return 0;

    if (!infolist_new_var_pointer (item, "callback", HOOK_CONFIG(hook, callback)))
        return 0;
    if (!infolist_new_var_string (item, "option", HOOK_CONFIG(hook, option)))
        return 0;

    return 1;
}

/*
 * Prints config hook data in WeeChat log file (usually for crash dump).
 */

void
hook_config_print_log (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    log_printf ("  config data:");
    log_printf ("    callback. . . . . . . : %p", HOOK_CONFIG(hook, callback));
    log_printf ("    option. . . . . . . . : '%s'", HOOK_CONFIG(hook, option));
}
