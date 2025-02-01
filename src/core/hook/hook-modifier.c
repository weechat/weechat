/*
 * hook-modifier.c - WeeChat modifier hook
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
hook_modifier_get_description (struct t_hook *hook)
{
    return strdup (HOOK_MODIFIER(hook, modifier));
}

/*
 * Hooks a modifier.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_modifier (struct t_weechat_plugin *plugin, const char *modifier,
               t_hook_callback_modifier *callback,
               const void *callback_pointer,
               void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_modifier *new_hook_modifier;
    int priority;
    const char *ptr_modifier;

    if (!modifier || !modifier[0] || !callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_modifier = malloc (sizeof (*new_hook_modifier));
    if (!new_hook_modifier)
    {
        free (new_hook);
        return NULL;
    }

    string_get_priority_and_name (modifier, &priority, &ptr_modifier,
                                  HOOK_PRIORITY_DEFAULT);
    hook_init_data (new_hook, plugin, HOOK_TYPE_MODIFIER, priority,
                    callback_pointer, callback_data);

    new_hook->hook_data = new_hook_modifier;
    new_hook_modifier->callback = callback;
    new_hook_modifier->modifier = strdup ((ptr_modifier) ? ptr_modifier : modifier);

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Executes a modifier hook.
 *
 * Note: result must be freed after use.
 */

char *
hook_modifier_exec (struct t_weechat_plugin *plugin, const char *modifier,
                    const char *modifier_data, const char *string)
{
    struct t_hook *ptr_hook, *next_hook;
    struct t_hook_exec_cb hook_exec_cb;
    char *new_msg, *message_modified;

    /* make C compiler happy */
    (void) plugin;

    if (!modifier || !modifier[0] || !string)
        return NULL;

    new_msg = NULL;
    message_modified = strdup (string);
    if (!message_modified)
        return NULL;

    hook_exec_start ();

    ptr_hook = weechat_hooks[HOOK_TYPE_MODIFIER];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (string_strcasecmp (HOOK_MODIFIER(ptr_hook, modifier),
                                   modifier) == 0))
        {
            hook_callback_start (ptr_hook, &hook_exec_cb);
            new_msg = (HOOK_MODIFIER(ptr_hook, callback))
                (ptr_hook->callback_pointer,
                 ptr_hook->callback_data,
                 modifier,
                 modifier_data,
                 message_modified);
            hook_callback_end (ptr_hook, &hook_exec_cb);

            /* empty string returned => message dropped */
            if (new_msg && !new_msg[0])
            {
                free (message_modified);
                hook_exec_end ();
                return new_msg;
            }

            /* new message => keep it as base for next modifier */
            if (new_msg)
            {
                free (message_modified);
                message_modified = new_msg;
            }
        }

        ptr_hook = next_hook;
    }

    hook_exec_end ();

    return message_modified;
}

/*
 * Frees data in a modifier hook.
 */

void
hook_modifier_free_data (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    if (HOOK_MODIFIER(hook, modifier))
    {
        free (HOOK_MODIFIER(hook, modifier));
        HOOK_MODIFIER(hook, modifier) = NULL;
    }

    free (hook->hook_data);
    hook->hook_data = NULL;
}

/*
 * Adds modifier hook data in the infolist item.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_modifier_add_to_infolist (struct t_infolist_item *item,
                               struct t_hook *hook)
{
    if (!item || !hook || !hook->hook_data)
        return 0;

    if (!infolist_new_var_pointer (item, "callback", HOOK_MODIFIER(hook, callback)))
        return 0;
    if (!infolist_new_var_string (item, "modifier", HOOK_MODIFIER(hook, modifier)))
        return 0;

    return 1;
}

/*
 * Prints modifier hook data in WeeChat log file (usually for crash dump).
 */

void
hook_modifier_print_log (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    log_printf ("  modifier data:");
    log_printf ("    callback. . . . . . . : %p", HOOK_MODIFIER(hook, callback));
    log_printf ("    modifier. . . . . . . : '%s'", HOOK_MODIFIER(hook, modifier));
}
