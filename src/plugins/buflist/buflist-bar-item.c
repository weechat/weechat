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
#include "buflist-bar-item.h"
#include "buflist-config.h"


struct t_gui_bar_item *buflist_bar_item_buflist = NULL;
struct t_hashtable *buflist_hashtable_pointers = NULL;
struct t_hashtable *buflist_hashtable_extra_vars = NULL;
struct t_hashtable *buflist_hashtable_options = NULL;


/*
 * Returns content of bar item "buffer_plugin": bar item with buffer plugin.
 */

char *
buflist_bar_item_buflist_cb (const void *pointer, void *data,
                             struct t_gui_bar_item *item,
                             struct t_gui_window *window,
                             struct t_gui_buffer *buffer,
                             struct t_hashtable *extra_info)
{
    struct t_arraylist *buffers;
    struct t_gui_buffer *ptr_buffer, *ptr_current_buffer;
    struct t_gui_hotlist *ptr_hotlist;
    char **buflist, *str_buflist;
    char str_format_number[32], str_format_number_empty[32];
    char str_number[32], str_indent_name[4], *line, **hotlist, *str_hotlist;
    char str_hotlist_count[32];
    const char *ptr_format, *ptr_format_current, *ptr_name, *ptr_type;
    const char *ptr_hotlist_format, *ptr_hotlist_priority;
    const char *hotlist_priority_none = "none";
    const char *hotlist_priority[4] = { "low", "message", "private",
                                        "highlight" };
    const char *ptr_lag;
    int i, j, length_max_number, current_buffer, number, prev_number, priority;
    int rc, count;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) buffer;
    (void) extra_info;

    prev_number = -1;

    buflist = weechat_string_dyn_alloc (256);

    ptr_format = weechat_config_string (buflist_config_format_buffer);
    ptr_format_current = weechat_config_string (buflist_config_format_buffer_current);

    ptr_current_buffer = weechat_current_buffer ();

    ptr_buffer = weechat_hdata_get_list (buflist_hdata_buffer,
                                         "last_gui_buffer");

    length_max_number = snprintf (
        str_number, sizeof (str_number),
        "%d",
        weechat_hdata_integer (buflist_hdata_buffer, ptr_buffer, "number"));
    snprintf (str_format_number, sizeof (str_format_number),
              "%%%dd", length_max_number);
    snprintf (str_format_number_empty, sizeof (str_format_number_empty),
              "%%-%ds", length_max_number);

    buffers = buflist_sort_buffers ();

    for (i = 0; i < weechat_arraylist_size (buffers); i++)
    {
        ptr_buffer = weechat_arraylist_get (buffers, i);

        current_buffer = (ptr_buffer == ptr_current_buffer);

        ptr_hotlist = weechat_hdata_pointer (buflist_hdata_buffer,
                                             ptr_buffer, "hotlist");

        ptr_name = weechat_hdata_string (buflist_hdata_buffer,
                                         ptr_buffer, "short_name");
        if (!ptr_name)
            ptr_name = weechat_hdata_string (buflist_hdata_buffer,
                                             ptr_buffer, "name");

        if (*buflist[0])
        {
            if (!weechat_string_dyn_concat (buflist, "\n"))
                goto error;
        }

        /* buffer number */
        number = weechat_hdata_integer (buflist_hdata_buffer,
                                        ptr_buffer, "number");
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
        weechat_hashtable_set (buflist_hashtable_pointers,
                               "buffer", ptr_buffer);

        /* set extra variables */
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "format_buffer",
                               weechat_config_string (
                                   buflist_config_format_buffer));
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "number", str_number);
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "indent", str_indent_name);
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "name", ptr_name);

        /* hotlist */
        ptr_hotlist_format = weechat_config_string (
            buflist_config_format_hotlist_level_none);
        ptr_hotlist_priority = hotlist_priority_none;
        if (ptr_hotlist)
        {
            priority = weechat_hdata_integer (buflist_hdata_hotlist,
                                              ptr_hotlist, "priority");
            if ((priority >= 0) && (priority < 4))
            {
                ptr_hotlist_format = weechat_config_string (
                    buflist_config_format_hotlist_level[priority]);
                ptr_hotlist_priority = hotlist_priority[priority];
            }
        }
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "color_hotlist", ptr_hotlist_format);
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "hotlist_priority", ptr_hotlist_priority);
        str_hotlist = NULL;
        if (ptr_hotlist)
        {
            hotlist = weechat_string_dyn_alloc (64);
            if (hotlist)
            {
                for (j = 3; j >= 0; j--)
                {
                    snprintf (str_hotlist_count, sizeof (str_hotlist_count),
                              "%02d|count", j);
                    count = weechat_hdata_integer (buflist_hdata_hotlist,
                                                   ptr_hotlist,
                                                   str_hotlist_count);
                    if (count > 0)
                    {
                        if (*hotlist[0])
                        {
                            weechat_string_dyn_concat (
                                hotlist,
                                weechat_config_string (
                                    buflist_config_format_hotlist_separator));
                        }
                        weechat_string_dyn_concat (
                            hotlist,
                            weechat_config_string (
                                buflist_config_format_hotlist_level[j]));
                        snprintf (str_hotlist_count, sizeof (str_hotlist_count),
                                  "%d", count);
                        weechat_string_dyn_concat (hotlist, str_hotlist_count);
                    }
                }
                str_hotlist = *hotlist;
                weechat_string_dyn_free (hotlist, 0);
            }
        }
        weechat_hashtable_set (
            buflist_hashtable_extra_vars,
            "format_hotlist",
            (str_hotlist) ? weechat_config_string (buflist_config_format_hotlist) : "");
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "hotlist",
                               (str_hotlist) ? str_hotlist : "");
        if (str_hotlist)
            free (str_hotlist);

        /* lag */
        ptr_lag = weechat_buffer_get_string (ptr_buffer, "localvar_lag");
        if (ptr_lag && ptr_lag[0])
        {
            weechat_hashtable_set (
                buflist_hashtable_extra_vars,
                "format_lag",
                weechat_config_string (buflist_config_format_lag));
        }
        else
        {
            weechat_hashtable_set (buflist_hashtable_extra_vars,
                                   "format_lag", "");
        }

        /* build string */
        line = weechat_string_eval_expression (
            (current_buffer) ? ptr_format_current : ptr_format,
            buflist_hashtable_pointers,
            buflist_hashtable_extra_vars,
            buflist_hashtable_options);

        /* concatenate string */
        rc = weechat_string_dyn_concat (buflist, line);
        free (line);
        if (!rc)
            goto error;
    }

    str_buflist = *buflist;

    goto end;

error:
    str_buflist = NULL;

end:
    weechat_string_dyn_free (buflist, 0);
    weechat_arraylist_free (buffers);

    return str_buflist;
}

/*
 * Initializes buflist bar items.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
buflist_bar_item_init ()
{
    /* create hashtable used by the bar item callback */
    buflist_hashtable_pointers = weechat_hashtable_new (
        8,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_POINTER,
        NULL,
        NULL);
    if (!buflist_hashtable_pointers)
        return 0;
    buflist_hashtable_extra_vars = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL,
        NULL);
    if (!buflist_hashtable_extra_vars)
    {
        weechat_hashtable_free (buflist_hashtable_pointers);
        return 0;
    }
    buflist_hashtable_options = weechat_hashtable_new (
        8,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL,
        NULL);
    if (!buflist_hashtable_options)
    {
        weechat_hashtable_free (buflist_hashtable_pointers);
        weechat_hashtable_free (buflist_hashtable_extra_vars);
        return 0;
    }
    weechat_hashtable_set (buflist_hashtable_options, "extra", "eval");

    /* bar items */
    buflist_bar_item_buflist = weechat_bar_item_new (
        BUFLIST_BAR_ITEM_NAME,
        &buflist_bar_item_buflist_cb, NULL, NULL);

    return 1;
}

/*
 * Ends buflist bar items.
 */

void
buflist_bar_item_end ()
{
    weechat_bar_item_remove (buflist_bar_item_buflist);

    weechat_hashtable_free (buflist_hashtable_pointers);
    buflist_hashtable_pointers = NULL;

    weechat_hashtable_free (buflist_hashtable_extra_vars);
    buflist_hashtable_extra_vars = NULL;

    weechat_hashtable_free (buflist_hashtable_options);
    buflist_hashtable_options = NULL;
}
