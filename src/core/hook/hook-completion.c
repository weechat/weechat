/*
 * hook-completion.c - WeeChat completion hook
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
#include "../../gui/gui-completion.h"


/*
 * Returns description of hook.
 *
 * Note: result must be freed after use.
 */

char *
hook_completion_get_description (struct t_hook *hook)
{
    return strdup (HOOK_COMPLETION(hook, completion_item));
}

/*
 * Hooks a completion.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_completion (struct t_weechat_plugin *plugin, const char *completion_item,
                 const char *description,
                 t_hook_callback_completion *callback,
                 const void *callback_pointer,
                 void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_completion *new_hook_completion;
    int priority;
    const char *ptr_completion_item;

    if (!completion_item || !completion_item[0]
        || strchr (completion_item, ' ') || !callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_completion = malloc (sizeof (*new_hook_completion));
    if (!new_hook_completion)
    {
        free (new_hook);
        return NULL;
    }

    string_get_priority_and_name (completion_item,
                                  &priority, &ptr_completion_item,
                                  HOOK_PRIORITY_DEFAULT);
    hook_init_data (new_hook, plugin, HOOK_TYPE_COMPLETION, priority,
                    callback_pointer, callback_data);

    new_hook->hook_data = new_hook_completion;
    new_hook_completion->callback = callback;
    new_hook_completion->completion_item = strdup ((ptr_completion_item) ?
                                                   ptr_completion_item : completion_item);
    new_hook_completion->description = strdup ((description) ?
                                               description : "");

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Executes a completion hook.
 */

void
hook_completion_exec (struct t_weechat_plugin *plugin,
                      const char *completion_item,
                      struct t_gui_buffer *buffer,
                      struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook, *next_hook;
    struct t_hook_exec_cb hook_exec_cb;
    const char *pos;
    char *item;

    /* make C compiler happy */
    (void) plugin;

    if (!weechat_hooks[HOOK_TYPE_COMPLETION])
        return;

    hook_exec_start ();

    pos = strchr (completion_item, ':');
    item = (pos) ?
        string_strndup (completion_item, pos - completion_item) :
        strdup (completion_item);
    if (!item)
        return;

    ptr_hook = weechat_hooks[HOOK_TYPE_COMPLETION];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (strcmp (HOOK_COMPLETION(ptr_hook, completion_item), item) == 0))
        {
            hook_callback_start (ptr_hook, &hook_exec_cb);
            (void) (HOOK_COMPLETION(ptr_hook, callback))
                (ptr_hook->callback_pointer,
                 ptr_hook->callback_data,
                 completion_item,
                 buffer,
                 completion);
            hook_callback_end (ptr_hook, &hook_exec_cb);
        }

        ptr_hook = next_hook;
    }

    free (item);

    hook_exec_end ();
}

/*
 * Frees data in a completion hook.
 */

void
hook_completion_free_data (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    if (HOOK_COMPLETION(hook, completion_item))
    {
        free (HOOK_COMPLETION(hook, completion_item));
        HOOK_COMPLETION(hook, completion_item) = NULL;
    }
    if (HOOK_COMPLETION(hook, description))
    {
        free (HOOK_COMPLETION(hook, description));
        HOOK_COMPLETION(hook, description) = NULL;
    }

    free (hook->hook_data);
    hook->hook_data = NULL;
}

/*
 * Adds completion hook data in the infolist item.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_completion_add_to_infolist (struct t_infolist_item *item,
                                 struct t_hook *hook)
{
    if (!item || !hook || !hook->hook_data)
        return 0;

    if (!infolist_new_var_pointer (item, "callback", HOOK_COMPLETION(hook, callback)))
        return 0;
    if (!infolist_new_var_string (item, "completion_item", HOOK_COMPLETION(hook, completion_item)))
        return 0;
    if (!infolist_new_var_string (item, "description", HOOK_COMPLETION(hook, description)))
        return 0;
    if (!infolist_new_var_string (item, "description_nls",
                                  (HOOK_COMPLETION(hook, description)
                                   && HOOK_COMPLETION(hook, description)[0]) ?
                                  _(HOOK_COMPLETION(hook, description)) : ""))
        return 0;

    return 1;
}

/*
 * Prints completion hook data in WeeChat log file (usually for crash dump).
 */

void
hook_completion_print_log (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    log_printf ("  completion data:");
    log_printf ("    callback. . . . . . . : %p", HOOK_COMPLETION(hook, callback));
    log_printf ("    completion_item . . . : '%s'", HOOK_COMPLETION(hook, completion_item));
    log_printf ("    description . . . . . : '%s'", HOOK_COMPLETION(hook, description));
}
