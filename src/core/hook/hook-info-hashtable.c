/*
 * hook-info-hashtable.c - WeeChat info_hashtable hook
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
hook_info_hashtable_get_description (struct t_hook *hook)
{
    return strdup (HOOK_INFO_HASHTABLE(hook, info_name));
}

/*
 * Hooks an info using hashtable.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_info_hashtable (struct t_weechat_plugin *plugin, const char *info_name,
                     const char *description, const char *args_description,
                     const char *output_description,
                     t_hook_callback_info_hashtable *callback,
                     const void *callback_pointer,
                     void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_info_hashtable *new_hook_info_hashtable;
    int priority;
    const char *ptr_info_name;

    if (!info_name || !info_name[0] || !callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_info_hashtable = malloc (sizeof (*new_hook_info_hashtable));
    if (!new_hook_info_hashtable)
    {
        free (new_hook);
        return NULL;
    }

    string_get_priority_and_name (info_name, &priority, &ptr_info_name,
                                  HOOK_PRIORITY_DEFAULT);
    hook_init_data (new_hook, plugin, HOOK_TYPE_INFO_HASHTABLE, priority,
                    callback_pointer, callback_data);

    new_hook->hook_data = new_hook_info_hashtable;
    new_hook_info_hashtable->callback = callback;
    new_hook_info_hashtable->info_name = strdup ((ptr_info_name) ?
                                                 ptr_info_name : info_name);
    new_hook_info_hashtable->description = strdup ((description) ? description : "");
    new_hook_info_hashtable->args_description = strdup ((args_description) ?
                                                        args_description : "");
    new_hook_info_hashtable->output_description = strdup ((output_description) ?
                                                          output_description : "");

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Gets info (as hashtable) via info hook.
 */

struct t_hashtable *
hook_info_get_hashtable (struct t_weechat_plugin *plugin, const char *info_name,
                         struct t_hashtable *hashtable)
{
    struct t_hook *ptr_hook, *next_hook;
    struct t_hook_exec_cb hook_exec_cb;
    struct t_hashtable *value;

    /* make C compiler happy */
    (void) plugin;

    if (!info_name || !info_name[0])
        return NULL;

    hook_exec_start ();

    ptr_hook = weechat_hooks[HOOK_TYPE_INFO_HASHTABLE];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (strcmp (HOOK_INFO_HASHTABLE(ptr_hook, info_name), info_name) == 0))
        {
            hook_callback_start (ptr_hook, &hook_exec_cb);
            value = (HOOK_INFO_HASHTABLE(ptr_hook, callback))
                (ptr_hook->callback_pointer,
                 ptr_hook->callback_data,
                 info_name,
                 hashtable);
            hook_callback_end (ptr_hook, &hook_exec_cb);

            hook_exec_end ();
            return value;
        }

        ptr_hook = next_hook;
    }

    hook_exec_end ();

    /* info not found */
    return NULL;
}

/*
 * Frees data in an info_hashtable hook.
 */

void
hook_info_hashtable_free_data (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    if (HOOK_INFO_HASHTABLE(hook, info_name))
    {
        free (HOOK_INFO_HASHTABLE(hook, info_name));
        HOOK_INFO_HASHTABLE(hook, info_name) = NULL;
    }
    if (HOOK_INFO_HASHTABLE(hook, description))
    {
        free (HOOK_INFO_HASHTABLE(hook, description));
        HOOK_INFO_HASHTABLE(hook, description) = NULL;
    }
    if (HOOK_INFO_HASHTABLE(hook, args_description))
    {
        free (HOOK_INFO_HASHTABLE(hook, args_description));
        HOOK_INFO_HASHTABLE(hook, args_description) = NULL;
    }
    if (HOOK_INFO_HASHTABLE(hook, output_description))
    {
        free (HOOK_INFO_HASHTABLE(hook, output_description));
        HOOK_INFO_HASHTABLE(hook, output_description) = NULL;
    }

    free (hook->hook_data);
    hook->hook_data = NULL;
}

/*
 * Adds info_hashtable hook data in the infolist item.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_info_hashtable_add_to_infolist (struct t_infolist_item *item,
                                     struct t_hook *hook)
{
    if (!item || !hook || !hook->hook_data)
        return 0;

    if (!infolist_new_var_pointer (item, "callback", HOOK_INFO_HASHTABLE(hook, callback)))
        return 0;
    if (!infolist_new_var_string (item, "info_name", HOOK_INFO_HASHTABLE(hook, info_name)))
        return 0;
    if (!infolist_new_var_string (item, "description", HOOK_INFO_HASHTABLE(hook, description)))
        return 0;
    if (!infolist_new_var_string (item, "description_nls",
                                  (HOOK_INFO_HASHTABLE(hook, description)
                                   && HOOK_INFO_HASHTABLE(hook, description)[0]) ?
                                  _(HOOK_INFO_HASHTABLE(hook, description)) : ""))
        return 0;
    if (!infolist_new_var_string (item, "args_description", HOOK_INFO_HASHTABLE(hook, args_description)))
        return 0;
    if (!infolist_new_var_string (item, "args_description_nls",
                                  (HOOK_INFO_HASHTABLE(hook, args_description)
                                   && HOOK_INFO_HASHTABLE(hook, args_description)[0]) ?
                                  _(HOOK_INFO_HASHTABLE(hook, args_description)) : ""))
        return 0;
    if (!infolist_new_var_string (item, "output_description", HOOK_INFO_HASHTABLE(hook, output_description)))
        return 0;
    if (!infolist_new_var_string (item, "output_description_nls",
                                  (HOOK_INFO_HASHTABLE(hook, output_description)
                                   && HOOK_INFO_HASHTABLE(hook, output_description)[0]) ?
                                  _(HOOK_INFO_HASHTABLE(hook, output_description)) : ""))
        return 0;

    return 1;
}

/*
 * Prints info_hashtable hook data in WeeChat log file (usually for crash dump).
 */

void
hook_info_hashtable_print_log (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    log_printf ("  info_hashtable data:");
    log_printf ("    callback. . . . . . . : %p", HOOK_INFO_HASHTABLE(hook, callback));
    log_printf ("    info_name . . . . . . : '%s'", HOOK_INFO_HASHTABLE(hook, info_name));
    log_printf ("    description . . . . . : '%s'", HOOK_INFO_HASHTABLE(hook, description));
    log_printf ("    args_description. . . : '%s'", HOOK_INFO_HASHTABLE(hook, args_description));
    log_printf ("    output_description. . : '%s'", HOOK_INFO_HASHTABLE(hook, output_description));
}
