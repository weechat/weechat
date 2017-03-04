/*
 * buflist-bar-item.c - bar item for buflist plugin
 *
 * Copyright (C) 2003-2017 Sébastien Helleu <flashcode@flashtux.org>
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
#include "buflist.h"
#include "buflist-config.h"


/*
 * Returns content of bar item "buffer_plugin": bar item with buffer plugin.
 */

char *
buflist_bar_item_buflist (const void *pointer, void *data,
                          struct t_gui_bar_item *item,
                          struct t_gui_window *window,
                          struct t_gui_buffer *buffer,
                          struct t_hashtable *extra_info)
{
    struct t_hdata *hdata_buffer;
    struct t_gui_buffer *ptr_buffer, *ptr_next_buffer, *ptr_current_buffer;
    char **buflist, *str_buflist;
    char str_format_number[32], str_format_number_empty[32];
    char str_number[32], str_indent_name[4], *line;
    const char *ptr_format, *ptr_format_current, *ptr_name, *ptr_type;
    int length_max_number, current_buffer, number, prev_number;
    struct t_hashtable *pointers, *extra_vars;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) buffer;
    (void) extra_info;

    prev_number = -1;

    pointers = weechat_hashtable_new (8,
                                      WEECHAT_HASHTABLE_STRING,
                                      WEECHAT_HASHTABLE_POINTER,
                                      NULL,
                                      NULL);
    if (!pointers)
        return NULL;
    extra_vars = weechat_hashtable_new (32,
                                        WEECHAT_HASHTABLE_STRING,
                                        WEECHAT_HASHTABLE_POINTER,
                                        NULL,
                                        NULL);
    if (!extra_vars)
    {
        weechat_hashtable_free (pointers);
        return NULL;
    }

    buflist = weechat_string_dyn_alloc (1);

    ptr_format = weechat_config_string (buflist_config_format_buffer);
    ptr_format_current = weechat_config_string (buflist_config_format_buffer_current);

    ptr_current_buffer = weechat_current_buffer ();

    hdata_buffer = weechat_hdata_get ("buffer");
    ptr_buffer = weechat_hdata_get_list (hdata_buffer, "last_gui_buffer");

    length_max_number = snprintf (
        str_number, sizeof (str_number),
        "%d", weechat_hdata_integer (hdata_buffer, ptr_buffer, "number"));
    snprintf (str_format_number, sizeof (str_format_number),
              "%%%dd", length_max_number);
    snprintf (str_format_number_empty, sizeof (str_format_number_empty),
              "%%-%ds", length_max_number);

    ptr_buffer = weechat_hdata_get_list (hdata_buffer, "gui_buffers");

    while (ptr_buffer)
    {
        ptr_next_buffer = weechat_hdata_move (hdata_buffer, ptr_buffer, 1);

        current_buffer = (ptr_buffer == ptr_current_buffer);

        ptr_name = weechat_hdata_string (hdata_buffer, ptr_buffer,
                                         "short_name");
        if (!ptr_name)
            ptr_name = weechat_hdata_string (hdata_buffer, ptr_buffer, "name");

        if (*buflist[0])
        {
            if (!weechat_string_dyn_concat (buflist, "\n"))
                return NULL;
        }

        /* buffer number */
        number = weechat_hdata_integer (hdata_buffer, ptr_buffer, "number");
        if (number != prev_number)
        {
            snprintf (str_number, sizeof (str_number),
                      str_format_number, number);
        }
        else
        {
            snprintf (str_number, sizeof (str_number),
                      str_format_number_empty, " ");
        }
        prev_number = number;

        /* buffer name */
        str_indent_name[0] = '\0';
        ptr_type = weechat_buffer_get_string (ptr_buffer, "localvar_type");
        if (ptr_type
            && ((strcmp (ptr_type, "channel") == 0)
                || (strcmp (ptr_type, "private") == 0)))
        {
            snprintf (str_indent_name, sizeof (str_indent_name), "  ");
        }

        /* set pointers */
        weechat_hashtable_set (pointers, "buffer", ptr_buffer);

        /* set extra variables */
        weechat_hashtable_set (extra_vars, "number", str_number);
        weechat_hashtable_set (extra_vars, "indent", str_indent_name);
        weechat_hashtable_set (extra_vars, "name", ptr_name);

        /* build string */
        line = weechat_string_eval_expression (
            (current_buffer) ? ptr_format_current : ptr_format,
            pointers, extra_vars, NULL);
        if (!weechat_string_dyn_concat (buflist, line))
        {
            free (line);
            weechat_hashtable_free (extra_vars);
            return NULL;
        }
        free (line);

        ptr_buffer = ptr_next_buffer;
    }

    weechat_hashtable_free (pointers);
    weechat_hashtable_free (extra_vars);

    str_buflist = *buflist;
    weechat_string_dyn_free (buflist, 0);

    return str_buflist;
}

/*
 * Initializes buflist bar items.
 */

void
buflist_bar_item_init ()
{
    weechat_bar_item_new ("buflist",
                          &buflist_bar_item_buflist, NULL, NULL);
}
