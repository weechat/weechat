/*
 * wee-hook-hsignal.c - WeeChat hsignal hook
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
#include "../../plugins/plugin.h"


/*
 * Returns description of hook.
 *
 * Note: result must be freed after use.
 */

char *
hook_hsignal_get_description (struct t_hook *hook)
{
    return string_rebuild_split_string (
        (const char **)(HOOK_HSIGNAL(hook, signals)), ";", 0, -1);
}

/*
 * Hooks a hsignal (signal with hashtable).
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_hsignal (struct t_weechat_plugin *plugin, const char *signal,
              t_hook_callback_hsignal *callback,
              const void *callback_pointer,
              void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_hsignal *new_hook_hsignal;
    int priority;
    const char *ptr_signal;

    if (!signal || !signal[0] || !callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_hsignal = malloc (sizeof (*new_hook_hsignal));
    if (!new_hook_hsignal)
    {
        free (new_hook);
        return NULL;
    }

    string_get_priority_and_name (signal, &priority, &ptr_signal,
                                  HOOK_PRIORITY_DEFAULT);
    hook_init_data (new_hook, plugin, HOOK_TYPE_HSIGNAL, priority,
                    callback_pointer, callback_data);

    new_hook->hook_data = new_hook_hsignal;
    new_hook_hsignal->callback = callback;
    new_hook_hsignal->signals = string_split (
        (ptr_signal) ? ptr_signal : signal,
        ";",
        NULL,
        WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
        0, &new_hook_hsignal->num_signals);

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Checks if a hooked hsignal matches a signal sent: it matches if at least
 * one of the signal masks are matching the signal sent.
 *
 * Returns:
 *   1: hook matches signal sent
 *   0: hook does not match signal sent
 */

int
hook_hsignal_match (const char *signal, struct t_hook *hook)
{
    int i;

    for (i = 0; i < HOOK_HSIGNAL(hook, num_signals); i++)
    {
        if (string_match (signal, HOOK_HSIGNAL(hook, signals)[i], 0))
            return 1;
    }

    return 0;
}

/*
 * Sends a hsignal (signal with hashtable).
 */

int
hook_hsignal_send (const char *signal, struct t_hashtable *hashtable)
{
    struct t_hook *ptr_hook, *next_hook;
    int rc;

    rc = WEECHAT_RC_OK;

    hook_exec_start ();

    ptr_hook = weechat_hooks[HOOK_TYPE_HSIGNAL];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (hook_hsignal_match (signal, ptr_hook)))
        {
            ptr_hook->running = 1;
            rc = (HOOK_HSIGNAL(ptr_hook, callback))
                (ptr_hook->callback_pointer,
                 ptr_hook->callback_data,
                 signal,
                 hashtable);
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
 * Frees data in a hsignal hook.
 */

void
hook_hsignal_free_data (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    if (HOOK_HSIGNAL(hook, signals))
    {
        string_free_split (HOOK_HSIGNAL(hook, signals));
        HOOK_HSIGNAL(hook, signals) = NULL;
    }
    HOOK_HSIGNAL(hook, num_signals) = 0;

    free (hook->hook_data);
    hook->hook_data = NULL;
}

/*
 * Adds hsignal hook data in the infolist item.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_hsignal_add_to_infolist (struct t_infolist_item *item,
                              struct t_hook *hook)
{
    int i;
    char option_name[64];

    if (!item || !hook || !hook->hook_data)
        return 0;

    if (!infolist_new_var_pointer (item, "callback", HOOK_HSIGNAL(hook, callback)))
        return 0;
    for (i = 0; i < HOOK_HSIGNAL(hook, num_signals); i++)
    {
        snprintf (option_name, sizeof (option_name), "signal_%05d", i);
        if (!infolist_new_var_string (item, option_name,
                                      HOOK_HSIGNAL(hook, signals)[i]))
            return 0;
    }
    if (!infolist_new_var_integer (item, "num_signals",
                                   HOOK_HSIGNAL(hook, num_signals)))
        return 0;

    return 1;
}

/*
 * Prints hsignal hook data in WeeChat log file (usually for crash dump).
 */

void
hook_hsignal_print_log (struct t_hook *hook)
{
    int i;

    if (!hook || !hook->hook_data)
        return;

    log_printf ("  signal data:");
    log_printf ("    callback. . . . . . . : 0x%lx", HOOK_HSIGNAL(hook, callback));
    log_printf ("    signals:");
    for (i = 0; i < HOOK_HSIGNAL(hook, num_signals); i++)
    {
        log_printf ("      '%s'", HOOK_HSIGNAL(hook, signals)[i]);
    }
}
