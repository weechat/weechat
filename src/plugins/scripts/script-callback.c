/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * script-callback.c: script callbacks management
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "script.h"
#include "script-callback.h"


/*
 * script_callback_alloc: allocate a new callback and initializes it
 */

struct t_script_callback *
script_callback_alloc ()
{
    struct t_script_callback *new_script_callback;

    new_script_callback = malloc (sizeof (*new_script_callback));
    if (new_script_callback)
    {
        new_script_callback->script = NULL;
        new_script_callback->function = NULL;
        new_script_callback->data = NULL;
        new_script_callback->config_file = NULL;
        new_script_callback->config_section = NULL;
        new_script_callback->config_option = NULL;
        new_script_callback->hook = NULL;
        new_script_callback->buffer = NULL;
        new_script_callback->bar_item = NULL;
        new_script_callback->upgrade_file = NULL;
        return new_script_callback;
    }

    return NULL;
}

/*
 * script_callback_add: allocate a new callback, initialize it
 *                      (script/function/data) and add it to list
 *                      return pointer to new callback or NULL if error
 */

struct t_script_callback *
script_callback_add (struct t_plugin_script *script,
                     const char *function,
                     const char *data)
{
    struct t_script_callback *script_cb;
    if (!script)
        return NULL;

    script_cb = script_callback_alloc ();
    if (!script_cb)
        return NULL;

    /* initialize callback */
    script_cb->script = script;
    script_cb->function = (function) ? strdup (function) : NULL;
    script_cb->data = (data) ? strdup (data) : NULL;

    /* add callback to list */
    if (script->callbacks)
        script->callbacks->prev_callback = script_cb;
    script_cb->prev_callback = NULL;
    script_cb->next_callback = script->callbacks;
    script->callbacks = script_cb;

    return script_cb;
}

/*
 * script_callback_free_data: free data of a script callback
 */

void
script_callback_free_data (struct t_script_callback *script_callback)
{
    if (script_callback->function)
        free (script_callback->function);
    if (script_callback->data)
        free (script_callback->data);
}

/*
 * script_callback_remove: remove a callback from a script
 */

void
script_callback_remove (struct t_plugin_script *script,
                        struct t_script_callback *script_callback)
{
    /* remove callback from list */
    if (script_callback->prev_callback)
        (script_callback->prev_callback)->next_callback =
            script_callback->next_callback;
    if (script_callback->next_callback)
        (script_callback->next_callback)->prev_callback =
            script_callback->prev_callback;
    if (script->callbacks == script_callback)
        script->callbacks = script_callback->next_callback;

    script_callback_free_data (script_callback);

    free (script_callback);
}

/*
 * script_callback_remove_all: remove all callbacks from a script
 */

void
script_callback_remove_all (struct t_plugin_script *script)
{
    while (script->callbacks)
    {
        script_callback_remove (script, script->callbacks);
    }
}

/*
 * script_callback_print_log: print callback infos in log (usually for crash dump)
 */

void
script_callback_print_log (struct t_weechat_plugin *weechat_plugin,
                           struct t_script_callback *script_callback)
{
    weechat_log_printf ("");
    weechat_log_printf ("  [callback (addr:0x%lx)]",       script_callback);
    weechat_log_printf ("    script. . . . . . . : 0x%lx", script_callback->script);
    weechat_log_printf ("    function. . . . . . : '%s'",  script_callback->function);
    weechat_log_printf ("    data. . . . . . . . : '%s'",  script_callback->data);
    weechat_log_printf ("    config_file . . . . : 0x%lx", script_callback->config_file);
    weechat_log_printf ("    config_section. . . : 0x%lx", script_callback->config_section);
    weechat_log_printf ("    config_option . . . : 0x%lx", script_callback->config_option);
    weechat_log_printf ("    hook. . . . . . . . : 0x%lx", script_callback->hook);
    weechat_log_printf ("    buffer. . . . . . . : 0x%lx", script_callback->buffer);
    weechat_log_printf ("    bar_item. . . . . . : 0x%lx", script_callback->bar_item);
    weechat_log_printf ("    upgrade_file. . . . : 0x%lx", script_callback->upgrade_file);
    weechat_log_printf ("    prev_callback . . . : 0x%lx", script_callback->prev_callback);
    weechat_log_printf ("    next_callback . . . : 0x%lx", script_callback->next_callback);
}
