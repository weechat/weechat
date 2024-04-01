/*
 * hook-hdata.c - WeeChat hdata hook
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../weechat.h"
#include "../core-hashtable.h"
#include "../core-hdata.h"
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
hook_hdata_get_description (struct t_hook *hook)
{
    return strdup (HOOK_HDATA(hook, hdata_name));
}

/*
 * Hooks a hdata.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_hdata (struct t_weechat_plugin *plugin, const char *hdata_name,
            const char *description,
            t_hook_callback_hdata *callback,
            const void *callback_pointer,
            void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_hdata *new_hook_hdata;
    int priority;
    const char *ptr_hdata_name;

    if (!hdata_name || !hdata_name[0] || !callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_hdata = malloc (sizeof (*new_hook_hdata));
    if (!new_hook_hdata)
    {
        free (new_hook);
        return NULL;
    }

    string_get_priority_and_name (hdata_name, &priority, &ptr_hdata_name,
                                  HOOK_PRIORITY_DEFAULT);
    hook_init_data (new_hook, plugin, HOOK_TYPE_HDATA, priority,
                    callback_pointer, callback_data);

    new_hook->hook_data = new_hook_hdata;
    new_hook_hdata->callback = callback;
    new_hook_hdata->hdata_name = strdup ((ptr_hdata_name) ?
                                         ptr_hdata_name : hdata_name);
    new_hook_hdata->description = strdup ((description) ? description : "");

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Gets hdata via hdata hook.
 */

struct t_hdata *
hook_hdata_get (struct t_weechat_plugin *plugin, const char *hdata_name)
{
    struct t_hook *ptr_hook, *next_hook;
    struct t_hook_exec_cb hook_exec_cb;
    struct t_hdata *value;

    /* make C compiler happy */
    (void) plugin;

    if (!hdata_name || !hdata_name[0])
        return NULL;

    if (weechat_hdata)
    {
        value = hashtable_get (weechat_hdata, hdata_name);
        if (value)
            return value;
    }

    hook_exec_start ();

    ptr_hook = weechat_hooks[HOOK_TYPE_HDATA];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (strcmp (HOOK_HDATA(ptr_hook, hdata_name), hdata_name) == 0))
        {
            hook_callback_start (ptr_hook, &hook_exec_cb);
            value = (HOOK_HDATA(ptr_hook, callback))
                (ptr_hook->callback_pointer,
                 ptr_hook->callback_data,
                 HOOK_HDATA(ptr_hook, hdata_name));
            hook_callback_end (ptr_hook, &hook_exec_cb);

            hook_exec_end ();
            return value;
        }

        ptr_hook = next_hook;
    }

    hook_exec_end ();

    /* hdata not found */
    return NULL;
}

/*
 * Frees data in a hdata hook.
 */

void
hook_hdata_free_data (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    if (HOOK_HDATA(hook, hdata_name))
    {
        free (HOOK_HDATA(hook, hdata_name));
        HOOK_HDATA(hook, hdata_name) = NULL;
    }
    if (HOOK_HDATA(hook, description))
    {
        free (HOOK_HDATA(hook, description));
        HOOK_HDATA(hook, description) = NULL;
    }

    free (hook->hook_data);
    hook->hook_data = NULL;
}

/*
 * Adds hdata hook data in the infolist item.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_hdata_add_to_infolist (struct t_infolist_item *item,
                            struct t_hook *hook)
{
    if (!item || !hook || !hook->hook_data)
        return 0;

    if (!infolist_new_var_pointer (item, "callback", HOOK_HDATA(hook, callback)))
        return 0;
    if (!infolist_new_var_string (item, "hdata_name", HOOK_HDATA(hook, hdata_name)))
        return 0;
    if (!infolist_new_var_string (item, "description", HOOK_HDATA(hook, description)))
        return 0;
    if (!infolist_new_var_string (item, "description_nls",
                                  (HOOK_HDATA(hook, description)
                                   && HOOK_HDATA(hook, description)[0]) ?
                                  _(HOOK_HDATA(hook, description)) : ""))
        return 0;

    return 1;
}

/*
 * Prints hdata hook data in WeeChat log file (usually for crash dump).
 */

void
hook_hdata_print_log (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    log_printf ("  hdata data:");
    log_printf ("    callback. . . . . . . : %p", HOOK_HDATA(hook, callback));
    log_printf ("    hdata_name. . . . . . : '%s'", HOOK_HDATA(hook, hdata_name));
    log_printf ("    description . . . . . : '%s'", HOOK_HDATA(hook, description));
}
