/*
 * hook-focus.c - WeeChat focus hook
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
#include "../core-hashtable.h"
#include "../core-hook.h"
#include "../core-infolist.h"
#include "../core-log.h"
#include "../core-string.h"
#include "../../plugins/plugin.h"


/*
 * Returns description of hook.
 *
 * Note: result must be freed after use.
 */

char *
hook_focus_get_description (struct t_hook *hook)
{
    return strdup (HOOK_FOCUS(hook, area));
}

/*
 * Hooks a focus.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_focus (struct t_weechat_plugin *plugin,
            const char *area,
            t_hook_callback_focus *callback,
            const void *callback_pointer,
            void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_focus *new_hook_focus;
    int priority;
    const char *ptr_area;

    if (!area || !area[0] || !callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_focus = malloc (sizeof (*new_hook_focus));
    if (!new_hook_focus)
    {
        free (new_hook);
        return NULL;
    }

    string_get_priority_and_name (area, &priority, &ptr_area,
                                  HOOK_PRIORITY_DEFAULT);
    hook_init_data (new_hook, plugin, HOOK_TYPE_FOCUS, priority,
                    callback_pointer, callback_data);

    new_hook->hook_data = new_hook_focus;
    new_hook_focus->callback = callback;
    new_hook_focus->area = strdup ((ptr_area) ? ptr_area : area);

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Adds keys of a hashtable into another.
 */

void
hook_focus_hashtable_map_cb (void *data,
                             struct t_hashtable *hashtable,
                             const void *key, const void *value)
{
    struct t_hashtable *hashtable1;

    /* make C compiler happy */
    (void) hashtable;

    hashtable1 = (struct t_hashtable *)data;

    if (hashtable1 && key && value)
        hashtable_set (hashtable1, (const char *)key, (const char *)value);
}

/*
 * Adds keys of a hashtable into another (adding suffix "2" to keys).
 */

void
hook_focus_hashtable_map2_cb (void *data,
                              struct t_hashtable *hashtable,
                              const void *key, const void *value)
{
    struct t_hashtable *hashtable1;
    char *key2;

    /* make C compiler happy */
    (void) hashtable;

    hashtable1 = (struct t_hashtable *)data;

    if (!hashtable1 || !key || !value)
        return;

    if (string_asprintf (&key2, "%s2", (const char *)key) >= 0)
    {
        hashtable_set (hashtable1, key2, (const char *)value);
        free (key2);
    }
}

/*
 * Gets data for focus on (x,y) on screen.
 *
 * Argument hashtable_focus2 is not NULL only for a mouse gesture (it's for
 * point where mouse button has been released).
 */

struct t_hashtable *
hook_focus_get_data (struct t_hashtable *hashtable_focus1,
                     struct t_hashtable *hashtable_focus2)
{
    struct t_hook *ptr_hook, *next_hook;
    struct t_hook_exec_cb hook_exec_cb;
    struct t_hashtable *hashtable1, *hashtable2, *hashtable_ret;
    const char *focus1_chat, *focus1_bar_item_name, *keys;
    char **list_keys, *new_key;
    int num_keys, i, focus1_is_chat;

    if (!hashtable_focus1)
        return NULL;

    focus1_chat = hashtable_get (hashtable_focus1, "_chat");
    focus1_is_chat = (focus1_chat && (strcmp (focus1_chat, "1") == 0));
    focus1_bar_item_name = hashtable_get (hashtable_focus1, "_bar_item_name");

    hashtable1 = hashtable_dup (hashtable_focus1);
    if (!hashtable1)
        return NULL;
    hashtable2 = (hashtable_focus2) ? hashtable_dup (hashtable_focus2) : NULL;

    hook_exec_start ();

    ptr_hook = weechat_hooks[HOOK_TYPE_FOCUS];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running
            && ((focus1_is_chat
                 && (strcmp (HOOK_FOCUS(ptr_hook, area), "chat") == 0))
                || (focus1_bar_item_name && focus1_bar_item_name[0]
                    && (strcmp (HOOK_FOCUS(ptr_hook, area), focus1_bar_item_name) == 0))))
        {
            /* run callback for focus #1 */
            hook_callback_start (ptr_hook, &hook_exec_cb);
            hashtable_ret = (HOOK_FOCUS(ptr_hook, callback))
                (ptr_hook->callback_pointer,
                 ptr_hook->callback_data,
                 hashtable1);
            hook_callback_end (ptr_hook, &hook_exec_cb);
            if (hashtable_ret)
            {
                if (hashtable_ret != hashtable1)
                {
                    /*
                     * add keys of hashtable_ret into hashtable1
                     * and destroy it
                     */
                    hashtable_map (hashtable_ret,
                                   &hook_focus_hashtable_map_cb,
                                   hashtable1);
                    hashtable_free (hashtable_ret);
                }
            }

            /* run callback for focus #2 */
            if (hashtable2)
            {
                hook_callback_start (ptr_hook, &hook_exec_cb);
                hashtable_ret = (HOOK_FOCUS(ptr_hook, callback))
                    (ptr_hook->callback_pointer,
                     ptr_hook->callback_data,
                     hashtable2);
                hook_callback_end (ptr_hook, &hook_exec_cb);
                if (hashtable_ret)
                {
                    if (hashtable_ret != hashtable2)
                    {
                        /*
                         * add keys of hashtable_ret into hashtable2
                         * and destroy it
                         */
                        hashtable_map (hashtable_ret,
                                       &hook_focus_hashtable_map_cb,
                                       hashtable2);
                        hashtable_free (hashtable_ret);
                    }
                }
            }
        }

        ptr_hook = next_hook;
    }

    if (hashtable2)
    {
        hashtable_map (hashtable2,
                       &hook_focus_hashtable_map2_cb, hashtable1);
        hashtable_free (hashtable2);
    }
    else
    {
        keys = hashtable_get_string (hashtable1, "keys");
        if (keys)
        {
            list_keys = string_split (keys, ",", NULL,
                                      WEECHAT_STRING_SPLIT_STRIP_LEFT
                                      | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                      | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                      0, &num_keys);
            if (list_keys)
            {
                for (i = 0; i < num_keys; i++)
                {
                    if (string_asprintf (&new_key, "%s2", list_keys[i]) >= 0)
                    {
                        hashtable_set (hashtable1, new_key,
                                       hashtable_get (hashtable1,
                                                      list_keys[i]));
                        free (new_key);
                    }
                }
                string_free_split (list_keys);
            }
        }
    }

    hook_exec_end ();

    return hashtable1;
}

/*
 * Frees data in a focus hook.
 */

void
hook_focus_free_data (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    if (HOOK_FOCUS(hook, area))
    {
        free (HOOK_FOCUS(hook, area));
        HOOK_FOCUS(hook, area) = NULL;
    }

    free (hook->hook_data);
    hook->hook_data = NULL;
}

/*
 * Adds focus hook data in the infolist item.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_focus_add_to_infolist (struct t_infolist_item *item,
                            struct t_hook *hook)
{
    if (!item || !hook || !hook->hook_data)
        return 0;

    if (!infolist_new_var_pointer (item, "callback", HOOK_FOCUS(hook, callback)))
        return 0;
    if (!infolist_new_var_string (item, "area", HOOK_FOCUS(hook, area)))
        return 0;

    return 1;
}

/*
 * Prints focus hook data in WeeChat log file (usually for crash dump).
 */

void
hook_focus_print_log (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    log_printf ("  focus data:");
    log_printf ("    callback. . . . . . . : %p", HOOK_FOCUS(hook, callback));
    log_printf ("    area. . . . . . . . . : '%s'", HOOK_FOCUS(hook, area));
}
