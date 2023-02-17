/*
 * wee-hook-print.c - WeeChat print hook
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
#include "../../gui/gui-buffer.h"
#include "../../gui/gui-color.h"
#include "../../gui/gui-line.h"


/*
 * Returns description of hook.
 *
 * Note: result must be freed after use.
 */

char *
hook_print_get_description (struct t_hook *hook)
{
    char str_desc[1024];

    if (HOOK_PRINT(hook, buffer))
    {
        snprintf (str_desc, sizeof (str_desc),
                  "buffer: %s, message: %s%s%s",
                  HOOK_PRINT(hook, buffer)->name,
                  (HOOK_PRINT(hook, message)) ? "\"" : "",
                  (HOOK_PRINT(hook, message)) ? HOOK_PRINT(hook, message) : "(none)",
                  (HOOK_PRINT(hook, message)) ? "\"" : "");
    }
    else
    {
        snprintf (str_desc, sizeof (str_desc),
                  "message: %s%s%s",
                  (HOOK_PRINT(hook, message)) ? "\"" : "",
                  (HOOK_PRINT(hook, message)) ? HOOK_PRINT(hook, message) : "(none)",
                  (HOOK_PRINT(hook, message)) ? "\"" : "");
    }

    return strdup (str_desc);
}

/*
 * Hooks a message printed by WeeChat.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_print (struct t_weechat_plugin *plugin, struct t_gui_buffer *buffer,
            const char *tags, const char *message, int strip_colors,
            t_hook_callback_print *callback,
            const void *callback_pointer,
            void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_print *new_hook_print;

    if (!callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_print = malloc (sizeof (*new_hook_print));
    if (!new_hook_print)
    {
        free (new_hook);
        return NULL;
    }

    hook_init_data (new_hook, plugin, HOOK_TYPE_PRINT, HOOK_PRIORITY_DEFAULT,
                    callback_pointer, callback_data);

    new_hook->hook_data = new_hook_print;
    new_hook_print->callback = callback;
    new_hook_print->buffer = buffer;
    new_hook_print->tags_array = string_split_tags (tags,
                                                    &new_hook_print->tags_count);
    new_hook_print->message = (message) ? strdup (message) : NULL;
    new_hook_print->strip_colors = strip_colors;

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Executes a print hook.
 */

void
hook_print_exec (struct t_gui_buffer *buffer, struct t_gui_line *line)
{
    struct t_hook *ptr_hook, *next_hook;
    char *prefix_no_color, *message_no_color;

    if (!weechat_hooks[HOOK_TYPE_PRINT])
        return;

    if (!line->data->message)
        return;

    prefix_no_color = (line->data->prefix) ?
        gui_color_decode (line->data->prefix, NULL) : NULL;

    message_no_color = gui_color_decode (line->data->message, NULL);
    if (!message_no_color)
    {
        if (prefix_no_color)
            free (prefix_no_color);
        return;
    }

    hook_exec_start ();

    ptr_hook = weechat_hooks[HOOK_TYPE_PRINT];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (!HOOK_PRINT(ptr_hook, buffer)
                || (buffer == HOOK_PRINT(ptr_hook, buffer)))
            && (!HOOK_PRINT(ptr_hook, message)
                || !HOOK_PRINT(ptr_hook, message)[0]
                || string_strcasestr (prefix_no_color, HOOK_PRINT(ptr_hook, message))
                || string_strcasestr (message_no_color, HOOK_PRINT(ptr_hook, message)))
            && (!HOOK_PRINT(ptr_hook, tags_array)
                || gui_line_match_tags (line->data,
                                        HOOK_PRINT(ptr_hook, tags_count),
                                        HOOK_PRINT(ptr_hook, tags_array))))
        {
            /* run callback */
            ptr_hook->running = 1;
            (void) (HOOK_PRINT(ptr_hook, callback))
                (ptr_hook->callback_pointer,
                 ptr_hook->callback_data,
                 buffer,
                 line->data->date,
                 line->data->tags_count,
                 (const char **)line->data->tags_array,
                 (int)line->data->displayed, (int)line->data->highlight,
                 (HOOK_PRINT(ptr_hook, strip_colors)) ? prefix_no_color : line->data->prefix,
                 (HOOK_PRINT(ptr_hook, strip_colors)) ? message_no_color : line->data->message);
            ptr_hook->running = 0;
        }

        ptr_hook = next_hook;
    }

    if (prefix_no_color)
        free (prefix_no_color);
    if (message_no_color)
        free (message_no_color);

    hook_exec_end ();
}

/*
 * Frees data in a print hook.
 */

void
hook_print_free_data (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    if (HOOK_PRINT(hook, tags_array))
    {
        string_free_split_tags (HOOK_PRINT(hook, tags_array));
        HOOK_PRINT(hook, tags_array) = NULL;
    }
    if (HOOK_PRINT(hook, message))
    {
        free (HOOK_PRINT(hook, message));
        HOOK_PRINT(hook, message) = NULL;
    }

    free (hook->hook_data);
    hook->hook_data = NULL;
}

/*
 * Adds print hook data in the infolist item.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_print_add_to_infolist (struct t_infolist_item *item,
                            struct t_hook *hook)
{
    if (!item || !hook || !hook->hook_data)
        return 0;

    if (!infolist_new_var_pointer (item, "callback", HOOK_PRINT(hook, callback)))
        return 0;
    if (!infolist_new_var_pointer (item, "buffer", HOOK_PRINT(hook, buffer)))
        return 0;
    if (!infolist_new_var_integer (item, "tags_count", HOOK_PRINT(hook, tags_count)))
        return 0;
    if (!infolist_new_var_pointer (item, "tags_array", HOOK_PRINT(hook, tags_array)))
        return 0;
    if (!infolist_new_var_string (item, "message", HOOK_PRINT(hook, message)))
        return 0;
    if (!infolist_new_var_integer (item, "strip_colors", HOOK_PRINT(hook, strip_colors)))
        return 0;

    return 1;
}

/*
 * Prints print hook data in WeeChat log file (usually for crash dump).
 */

void
hook_print_print_log (struct t_hook *hook)
{
    int i, j;

    if (!hook || !hook->hook_data)
        return;

    log_printf ("  print data:");
    log_printf ("    callback. . . . . . . : 0x%lx", HOOK_PRINT(hook, callback));
    log_printf ("    buffer. . . . . . . . : 0x%lx", HOOK_PRINT(hook, buffer));
    log_printf ("    tags_count. . . . . . : %d", HOOK_PRINT(hook, tags_count));
    log_printf ("    tags_array. . . . . . : 0x%lx", HOOK_PRINT(hook, tags_array));
    if (HOOK_PRINT(hook, tags_array))
    {
        for (i = 0; i < HOOK_PRINT(hook, tags_count); i++)
        {
            for (j = 0; HOOK_PRINT(hook, tags_array)[i][j]; j++)
            {
                log_printf ("      tags_array[%03d][%03d]: '%s'",
                            i,
                            j,
                            HOOK_PRINT(hook, tags_array)[i][j]);
            }
        }
    }
    log_printf ("    message . . . . . . . : '%s'", HOOK_PRINT(hook, message));
    log_printf ("    strip_colors. . . . . : %d", HOOK_PRINT(hook, strip_colors));
}
