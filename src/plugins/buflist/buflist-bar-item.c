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
struct t_hashtable *buflist_hashtable_options_conditions = NULL;
struct t_arraylist *buflist_list_buffers = NULL;


/*
 * Updates buflist bar item if buflist is enabled.
 */

void
buflist_bar_item_update ()
{
    if (weechat_config_boolean (buflist_config_look_enabled))
        weechat_bar_item_update (BUFLIST_BAR_ITEM_NAME);
}

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
    struct t_gui_nick *ptr_gui_nick;
    struct t_gui_hotlist *ptr_hotlist;
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    char **buflist, *str_buflist, *condition;
    char str_format_number[32], str_format_number_empty[32];
    char str_nick_prefix[32], str_color_nick_prefix[32];
    char str_number[32], *line, **hotlist, *str_hotlist;
    char str_hotlist_count[32];
    const char *ptr_format, *ptr_format_current, *ptr_format_indent;
    const char *ptr_name, *ptr_type, *ptr_nick, *ptr_nick_prefix;
    const char *ptr_hotlist_format, *ptr_hotlist_priority;
    const char *hotlist_priority_none = "none";
    const char *hotlist_priority[4] = { "low", "message", "private",
                                        "highlight" };
    const char indent_empty[1] = { '\0' };
    const char *ptr_lag;
    int num_buffers, is_channel, is_private;
    int i, j, length_max_number, current_buffer, number, prev_number, priority;
    int rc, count;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) buffer;
    (void) extra_info;

    if (!weechat_config_boolean (buflist_config_look_enabled))
        return NULL;

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

    if (buflist_list_buffers)
        weechat_arraylist_free (buflist_list_buffers);
    buflist_list_buffers = weechat_arraylist_new (16, 0, 1,
                                                  NULL, NULL, NULL, NULL);

    buffers = buflist_sort_buffers ();

    num_buffers = weechat_arraylist_size (buffers);
    for (i = 0; i < num_buffers; i++)
    {
        ptr_buffer = weechat_arraylist_get (buffers, i);

        /* set pointers */
        weechat_hashtable_set (buflist_hashtable_pointers,
                               "buffer", ptr_buffer);

        /* set IRC server/channel pointers */
        buflist_buffer_get_irc_pointers (ptr_buffer, &ptr_server, &ptr_channel);
        weechat_hashtable_set (buflist_hashtable_pointers,
                               "irc_server", ptr_server);
        weechat_hashtable_set (buflist_hashtable_pointers,
                               "irc_channel", ptr_channel);

        /* check condition: if false, the buffer is not displayed */
        condition = weechat_string_eval_expression (
            weechat_config_string (buflist_config_look_display_conditions),
            buflist_hashtable_pointers,
            NULL,  /* extra vars */
            buflist_hashtable_options_conditions);
        rc = (condition && (strcmp (condition, "1") == 0));
        if (condition)
            free (condition);
        if (!rc)
            continue;

        weechat_arraylist_add (buflist_list_buffers, ptr_buffer);

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

        /* current buffer */
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "current_buffer",
                               (current_buffer) ? "1" : "0");

        /* buffer number */
        number = weechat_hdata_integer (buflist_hdata_buffer,
                                        ptr_buffer, "number");
        if (number != prev_number)
        {
            snprintf (str_number, sizeof (str_number),
                      str_format_number, number);
            weechat_hashtable_set (buflist_hashtable_extra_vars,
                                   "number_displayed", "1");
        }
        else
        {
            snprintf (str_number, sizeof (str_number),
                      str_format_number_empty, " ");
            weechat_hashtable_set (buflist_hashtable_extra_vars,
                                   "number_displayed", "0");
        }
        prev_number = number;

        /* buffer name */
        ptr_type = weechat_buffer_get_string (ptr_buffer, "localvar_type");
        is_channel = (ptr_type && (strcmp (ptr_type, "channel") == 0));
        is_private = (ptr_type && (strcmp (ptr_type, "private") == 0));
        ptr_format_indent = (is_channel || is_private) ?
            weechat_config_string (buflist_config_format_indent) : indent_empty;

        /* nick prefix */
        str_nick_prefix[0] = '\0';
        str_color_nick_prefix[0] = '\0';
        if (is_channel
            && weechat_config_boolean (buflist_config_look_nick_prefix))
        {
            snprintf (str_nick_prefix, sizeof (str_nick_prefix),
                      "%s",
                      (weechat_config_boolean (buflist_config_look_nick_prefix_empty)) ?
                      " " : "");
            ptr_nick = weechat_buffer_get_string (ptr_buffer, "localvar_nick");
            if (ptr_nick)
            {
                ptr_gui_nick = weechat_nicklist_search_nick (ptr_buffer, NULL,
                                                             ptr_nick);
                if (ptr_gui_nick)
                {
                    ptr_nick_prefix = weechat_nicklist_nick_get_string (
                        ptr_buffer, ptr_gui_nick, "prefix");
                    if (ptr_nick_prefix && (ptr_nick_prefix[0] != ' '))
                    {
                        snprintf (str_color_nick_prefix,
                                  sizeof (str_color_nick_prefix),
                                  "%s",
                                  weechat_color (
                                      weechat_nicklist_nick_get_string (
                                          ptr_buffer, ptr_gui_nick,
                                          "prefix_color")));
                        snprintf (str_nick_prefix, sizeof (str_nick_prefix),
                                  "%s",
                                  ptr_nick_prefix);
                    }
                }
            }
        }
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "nick_prefix", str_nick_prefix);
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "color_nick_prefix", str_color_nick_prefix);
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "format_nick_prefix",
                               weechat_config_string (
                                   buflist_config_format_nick_prefix));

        /* set extra variables */
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "format_buffer",
                               weechat_config_string (
                                   buflist_config_format_buffer));
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "number", str_number);
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "format_number",
                               weechat_config_string (
                                   buflist_config_format_number));
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "indent", ptr_format_indent);
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
    /* create hashtables used by the bar item callback */
    buflist_hashtable_pointers = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_POINTER,
        NULL,
        NULL);
    if (!buflist_hashtable_pointers)
        return 0;

    buflist_hashtable_extra_vars = weechat_hashtable_new (
        128,
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
        32,
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

    buflist_hashtable_options_conditions = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (!buflist_hashtable_options_conditions)
    {
        weechat_hashtable_free (buflist_hashtable_pointers);
        weechat_hashtable_free (buflist_hashtable_extra_vars);
        weechat_hashtable_free (buflist_hashtable_options);
        return 0;
    }
    weechat_hashtable_set (buflist_hashtable_options_conditions,
                           "type", "condition");

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

    weechat_hashtable_free (buflist_hashtable_options_conditions);
    buflist_hashtable_options_conditions = NULL;

    if (buflist_list_buffers)
    {
        weechat_arraylist_free (buflist_list_buffers);
        buflist_list_buffers = NULL;
    }
}
