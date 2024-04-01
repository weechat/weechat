/*
 * hook-infolist.c - WeeChat infolist hook
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
hook_infolist_get_description (struct t_hook *hook)
{
    return strdup (HOOK_INFOLIST(hook, infolist_name));
}

/*
 * Hooks an infolist.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_infolist (struct t_weechat_plugin *plugin, const char *infolist_name,
               const char *description, const char *pointer_description,
               const char *args_description,
               t_hook_callback_infolist *callback,
               const void *callback_pointer,
               void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_infolist *new_hook_infolist;
    int priority;
    const char *ptr_infolist_name;

    if (!infolist_name || !infolist_name[0] || !callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_infolist = malloc (sizeof (*new_hook_infolist));
    if (!new_hook_infolist)
    {
        free (new_hook);
        return NULL;
    }

    string_get_priority_and_name (infolist_name, &priority, &ptr_infolist_name,
                                  HOOK_PRIORITY_DEFAULT);
    hook_init_data (new_hook, plugin, HOOK_TYPE_INFOLIST, priority,
                    callback_pointer, callback_data);

    new_hook->hook_data = new_hook_infolist;
    new_hook_infolist->callback = callback;
    new_hook_infolist->infolist_name = strdup ((ptr_infolist_name) ?
                                               ptr_infolist_name : infolist_name);
    new_hook_infolist->description = strdup ((description) ? description : "");
    new_hook_infolist->pointer_description = strdup ((pointer_description) ?
                                                     pointer_description : "");
    new_hook_infolist->args_description = strdup ((args_description) ?
                                                  args_description : "");

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Gets an infolist via infolist hook.
 */

struct t_infolist *
hook_infolist_get (struct t_weechat_plugin *plugin, const char *infolist_name,
                   void *pointer, const char *arguments)
{
    struct t_hook *ptr_hook, *next_hook;
    struct t_hook_exec_cb hook_exec_cb;
    struct t_infolist *value;

    /* make C compiler happy */
    (void) plugin;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    hook_exec_start ();

    ptr_hook = weechat_hooks[HOOK_TYPE_INFOLIST];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (strcmp (HOOK_INFOLIST(ptr_hook, infolist_name), infolist_name) == 0))
        {
            hook_callback_start (ptr_hook, &hook_exec_cb);
            value = (HOOK_INFOLIST(ptr_hook, callback))
                (ptr_hook->callback_pointer,
                 ptr_hook->callback_data,
                 infolist_name,
                 pointer,
                 arguments);
            hook_callback_end (ptr_hook, &hook_exec_cb);

            hook_exec_end ();
            return value;
        }

        ptr_hook = next_hook;
    }

    hook_exec_end ();

    /* infolist not found */
    return NULL;
}

/*
 * Frees data in an infolist hook.
 */

void
hook_infolist_free_data (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    if (HOOK_INFOLIST(hook, infolist_name))
    {
        free (HOOK_INFOLIST(hook, infolist_name));
        HOOK_INFOLIST(hook, infolist_name) = NULL;
    }
    if (HOOK_INFOLIST(hook, description))
    {
        free (HOOK_INFOLIST(hook, description));
        HOOK_INFOLIST(hook, description) = NULL;
    }
    if (HOOK_INFOLIST(hook, pointer_description))
    {
        free (HOOK_INFOLIST(hook, pointer_description));
        HOOK_INFOLIST(hook, pointer_description) = NULL;
    }
    if (HOOK_INFOLIST(hook, args_description))
    {
        free (HOOK_INFOLIST(hook, args_description));
        HOOK_INFOLIST(hook, args_description) = NULL;
    }

    free (hook->hook_data);
    hook->hook_data = NULL;
}

/*
 * Adds infolist hook data in the infolist item.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_infolist_add_to_infolist (struct t_infolist_item *item,
                               struct t_hook *hook)
{
    if (!item || !hook || !hook->hook_data)
        return 0;

    if (!infolist_new_var_pointer (item, "callback", HOOK_INFOLIST(hook, callback)))
        return 0;
    if (!infolist_new_var_string (item, "infolist_name", HOOK_INFOLIST(hook, infolist_name)))
        return 0;
    if (!infolist_new_var_string (item, "description", HOOK_INFOLIST(hook, description)))
        return 0;
    if (!infolist_new_var_string (item, "description_nls",
                                  (HOOK_INFOLIST(hook, description)
                                   && HOOK_INFOLIST(hook, description)[0]) ?
                                  _(HOOK_INFOLIST(hook, description)) : ""))
        return 0;
    if (!infolist_new_var_string (item, "pointer_description", HOOK_INFOLIST(hook, pointer_description)))
        return 0;
    if (!infolist_new_var_string (item, "pointer_description_nls",
                                  (HOOK_INFOLIST(hook, pointer_description)
                                   && HOOK_INFOLIST(hook, pointer_description)[0]) ?
                                  _(HOOK_INFOLIST(hook, pointer_description)) : ""))
        return 0;
    if (!infolist_new_var_string (item, "args_description", HOOK_INFOLIST(hook, args_description)))
        return 0;
    if (!infolist_new_var_string (item, "args_description_nls",
                                  (HOOK_INFOLIST(hook, args_description)
                                   && HOOK_INFOLIST(hook, args_description)[0]) ?
                                  _(HOOK_INFOLIST(hook, args_description)) : ""))
        return 0;

    return 1;
}

/*
 * Prints infolist hook data in WeeChat log file (usually for crash dump).
 */

void
hook_infolist_print_log (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    log_printf ("  infolist data:");
    log_printf ("    callback. . . . . . . : %p", HOOK_INFOLIST(hook, callback));
    log_printf ("    infolist_name . . . . : '%s'", HOOK_INFOLIST(hook, infolist_name));
    log_printf ("    description . . . . . : '%s'", HOOK_INFOLIST(hook, description));
    log_printf ("    pointer_description . : '%s'", HOOK_INFOLIST(hook, pointer_description));
    log_printf ("    args_description. . . : '%s'", HOOK_INFOLIST(hook, args_description));
}
