/*
 * exec-buffer.c - buffers with output of commands
 *
 * Copyright (C) 2014-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "exec.h"
#include "exec-buffer.h"
#include "exec-command.h"
#include "exec-config.h"


/*
 * Callback for user data in an exec buffer.
 */

int
exec_buffer_input_cb (const void *pointer, void *data,
                      struct t_gui_buffer *buffer,
                      const char *input_data)
{
    char **argv, **argv_eol;
    int argc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    /* close buffer */
    if (strcmp (input_data, "q") == 0)
    {
        weechat_buffer_close (buffer);
        return WEECHAT_RC_OK;
    }

    argv = weechat_string_split (input_data, " ", NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                 0, &argc);
    argv_eol = weechat_string_split (input_data, " ", NULL,
                                     WEECHAT_STRING_SPLIT_STRIP_LEFT
                                     | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                     | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS
                                     | WEECHAT_STRING_SPLIT_KEEP_EOL,
                                     0, NULL);

    if (argv && argv_eol)
        exec_command_run (buffer, argc, argv, argv_eol, 0);

    weechat_string_free_split (argv);
    weechat_string_free_split (argv_eol);

    return WEECHAT_RC_OK;
}

/*
 * Callback called when an exec buffer is closed.
 */

int
exec_buffer_close_cb (const void *pointer, void *data,
                      struct t_gui_buffer *buffer)
{
    const char *full_name;
    struct t_exec_cmd *ptr_exec_cmd;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    /* kill any command whose output is on this buffer */
    full_name = weechat_buffer_get_string (buffer, "full_name");
    for (ptr_exec_cmd = exec_cmds; ptr_exec_cmd;
         ptr_exec_cmd = ptr_exec_cmd->next_cmd)
    {
        if (ptr_exec_cmd->hook
            && ptr_exec_cmd->buffer_full_name
            && (strcmp (ptr_exec_cmd->buffer_full_name, full_name) == 0))
        {
            weechat_hook_set (ptr_exec_cmd->hook, "signal", "kill");
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Restore buffer callbacks (input and close) for buffers created by exec
 * plugin.
 */

void
exec_buffer_set_callbacks ()
{
    struct t_infolist *ptr_infolist;
    struct t_gui_buffer *ptr_buffer;
    const char *plugin_name;

    ptr_infolist = weechat_infolist_get ("buffer", NULL, "");
    if (ptr_infolist)
    {
        while (weechat_infolist_next (ptr_infolist))
        {
            ptr_buffer = weechat_infolist_pointer (ptr_infolist, "pointer");
            plugin_name = weechat_infolist_string (ptr_infolist, "plugin_name");
            if (ptr_buffer && plugin_name
                && (strcmp (plugin_name, EXEC_PLUGIN_NAME) == 0))
            {
                weechat_buffer_set_pointer (ptr_buffer, "close_callback",
                                            &exec_buffer_close_cb);
                weechat_buffer_set_pointer (ptr_buffer, "input_callback",
                                            &exec_buffer_input_cb);
            }
        }
        weechat_infolist_free (ptr_infolist);
    }
}

/*
 * Creates a new exec buffer for a command.
 */

struct t_gui_buffer *
exec_buffer_new (const char *name, int free_content, int clear_buffer,
                 int switch_to_buffer)
{
    struct t_gui_buffer *new_buffer;
    struct t_hashtable *buffer_props;
    int buffer_type;

    new_buffer = weechat_buffer_search (EXEC_PLUGIN_NAME, name);
    if (new_buffer)
    {
        buffer_type = weechat_buffer_get_integer (new_buffer, "type");
        if (((buffer_type == 0) && free_content)
            || ((buffer_type == 1) && !free_content))
        {
            /* change the type of buffer */
            weechat_buffer_set (new_buffer,
                                "type",
                                (free_content) ? "free" : "formatted");
        }
        goto end;
    }



    buffer_props = weechat_hashtable_new (32,
                                          WEECHAT_HASHTABLE_STRING,
                                          WEECHAT_HASHTABLE_STRING,
                                          NULL, NULL);
    if (buffer_props)
    {
        if (free_content)
            weechat_hashtable_set (buffer_props, "type", "free");
        weechat_hashtable_set (buffer_props, "clear", "1");
        weechat_hashtable_set (buffer_props, "title", _("Executed commands"));
        weechat_hashtable_set (buffer_props, "localvar_set_type", "exec");
        weechat_hashtable_set (buffer_props, "localvar_set_no_log", "1");
        weechat_hashtable_set (buffer_props, "time_for_each_line", "0");
        weechat_hashtable_set (buffer_props, "input_get_unknown_commands", "0");
    }

    new_buffer = weechat_buffer_new_props (name,
                                           buffer_props,
                                           &exec_buffer_input_cb, NULL, NULL,
                                           &exec_buffer_close_cb, NULL, NULL);

    weechat_hashtable_free (buffer_props);

    /* failed to create buffer ? then return */
    if (!new_buffer)
        return NULL;

end:
    if (clear_buffer)
        weechat_buffer_clear (new_buffer);
    if (switch_to_buffer)
        weechat_buffer_set (new_buffer, "display", "1");

    return new_buffer;
}
