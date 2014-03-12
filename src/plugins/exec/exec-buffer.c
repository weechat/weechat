/*
 * exec-buffer.c - buffers with output of commands
 *
 * Copyright (C) 2014 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "exec.h"
#include "exec-buffer.h"
#include "exec-config.h"


/*
 * Callback for user data in an exec buffer.
 */

int
exec_buffer_input_cb (void *data, struct t_gui_buffer *buffer,
                      const char *input_data)
{
    /* make C compiler happy */
    (void) data;

    /* close buffer */
    if (strcmp (input_data, "q") == 0)
    {
        weechat_buffer_close (buffer);
        return WEECHAT_RC_OK;
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback called when an exec buffer is closed.
 */

int
exec_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    /* make C compiler happy */
    (void) data;
    (void) buffer;

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
exec_buffer_new (const char *name, int switch_to_buffer)
{
    struct t_gui_buffer *new_buffer;

    new_buffer = weechat_buffer_search (EXEC_PLUGIN_NAME, name);
    if (new_buffer)
        return new_buffer;

    new_buffer = weechat_buffer_new (name,
                                     &exec_buffer_input_cb, NULL,
                                     &exec_buffer_close_cb, NULL);

    /* failed to create buffer ? then return */
    if (!new_buffer)
        return NULL;

    weechat_buffer_set (new_buffer, "title", _("Executed commands"));
    weechat_buffer_set (new_buffer, "localvar_set_type", "exec");
    weechat_buffer_set (new_buffer, "localvar_set_no_log", "1");
    weechat_buffer_set (new_buffer, "time_for_each_line", "0");
    weechat_buffer_set (new_buffer, "input_get_unknown_commands", "0");

    if (switch_to_buffer)
        weechat_buffer_set (new_buffer, "display", "1");

    return new_buffer;
}
