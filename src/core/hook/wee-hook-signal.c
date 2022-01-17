/*
 * wee-hook-signal.c - WeeChat signal hook
 *
 * Copyright (C) 2003-2022 Sébastien Helleu <flashcode@flashtux.org>
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
#include "../../plugins/plugin.h"


/*
 * Hooks a signal.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_signal (struct t_weechat_plugin *plugin, const char *signal,
             t_hook_callback_signal *callback,
             const void *callback_pointer,
             void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_signal *new_hook_signal;
    int priority;
    const char *ptr_signal;

    if (!signal || !signal[0] || !callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_signal = malloc (sizeof (*new_hook_signal));
    if (!new_hook_signal)
    {
        free (new_hook);
        return NULL;
    }

    hook_get_priority_and_name (signal, &priority, &ptr_signal);
    hook_init_data (new_hook, plugin, HOOK_TYPE_SIGNAL, priority,
                    callback_pointer, callback_data);

    new_hook->hook_data = new_hook_signal;
    new_hook_signal->callback = callback;
    new_hook_signal->signal = strdup ((ptr_signal) ? ptr_signal : signal);

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Sends a signal.
 */

int
hook_signal_send (const char *signal, const char *type_data, void *signal_data)
{
    struct t_hook *ptr_hook, *next_hook;
    int rc;

    rc = WEECHAT_RC_OK;

    hook_exec_start ();

    ptr_hook = weechat_hooks[HOOK_TYPE_SIGNAL];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (string_match (signal, HOOK_SIGNAL(ptr_hook, signal), 0)))
        {
            ptr_hook->running = 1;
            rc = (HOOK_SIGNAL(ptr_hook, callback))
                (ptr_hook->callback_pointer,
                 ptr_hook->callback_data,
                 signal,
                 type_data,
                 signal_data);
            ptr_hook->running = 0;

            if (rc == WEECHAT_RC_OK_EAT)
                break;
        }

        ptr_hook = next_hook;
    }

    hook_exec_end ();

    return rc;
}

/*
 * Frees data in a signal hook.
 */

void
hook_signal_free_data (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    if (HOOK_SIGNAL(hook, signal))
    {
        free (HOOK_SIGNAL(hook, signal));
        HOOK_SIGNAL(hook, signal) = NULL;
    }

    free (hook->hook_data);
    hook->hook_data = NULL;
}

/*
 * Adds signal hook data in the infolist item.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_signal_add_to_infolist (struct t_infolist_item *item,
                              struct t_hook *hook)
{
    if (!item || !hook || !hook->hook_data)
        return 0;

    if (!infolist_new_var_pointer (item, "callback", HOOK_SIGNAL(hook, callback)))
        return 0;
    if (!infolist_new_var_string (item, "signal", HOOK_SIGNAL(hook, signal)))
        return 0;

    return 1;
}

/*
 * Prints signal hook data in WeeChat log file (usually for crash dump).
 */

void
hook_signal_print_log (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    log_printf ("  signal data:");
    log_printf ("    callback. . . . . . . : 0x%lx", HOOK_SIGNAL(hook, callback));
    log_printf ("    signal. . . . . . . . : '%s'", HOOK_SIGNAL(hook, signal));
}
