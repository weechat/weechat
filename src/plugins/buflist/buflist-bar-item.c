/*
 * buflist-bar-item.c - bar item for buflist plugin
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "buflist.h"
#include "buflist-bar-item.h"
#include "buflist-config.h"


struct t_gui_bar_item *buflist_bar_item_buflist[BUFLIST_BAR_NUM_ITEMS] =
{ NULL, NULL, NULL };
struct t_hashtable *buflist_hashtable_pointers = NULL;
struct t_hashtable *buflist_hashtable_extra_vars = NULL;
struct t_hashtable *buflist_hashtable_options_conditions = NULL;
struct t_arraylist *buflist_list_buffers[BUFLIST_BAR_NUM_ITEMS] =
{ NULL, NULL, NULL };

int old_line_number_current_buffer[BUFLIST_BAR_NUM_ITEMS] = { -1, -1, -1 };


/*
 * Returns the bar item name with an index.
 */

const char *
buflist_bar_item_get_name (int index)
{
    static char item_name[32];

    if (index == 0)
    {
        snprintf (item_name, sizeof (item_name), "%s", BUFLIST_BAR_ITEM_NAME);
    }
    else
    {
        snprintf (item_name, sizeof (item_name),
                  "%s%d",
                  BUFLIST_BAR_ITEM_NAME,
                  index + 1);
    }
    return item_name;
}

/*
 * Returns the bar item index with an item name, -1 if not found.
 */

int
buflist_bar_item_get_index (const char *item_name)
{
    int i;
    const char *ptr_item_name;

    for (i = 0; i < BUFLIST_BAR_NUM_ITEMS; i++)
    {
        ptr_item_name = buflist_bar_item_get_name (i);
        if (strcmp (ptr_item_name, item_name) == 0)
            return i;
    }

    return -1;
}

/*
 * Returns the bar item index with a bar item pointer, -1 if not found.
 */

int
buflist_bar_item_get_index_with_pointer (struct t_gui_bar_item *item)
{
    int i;

    for (i = 0; i < BUFLIST_BAR_NUM_ITEMS; i++)
    {
        if (buflist_bar_item_buflist[i] == item)
            return i;
    }

    return -1;
}

/*
 * Updates buflist bar item if buflist is enabled (or if force argument is 1).
 *
 * If force == 1, all used items are refreshed
 *   (according to option buflist.look.use_items).
 * If force == 2, all items are refreshed.
 */

void
buflist_bar_item_update (int force)
{
    int i, num_items;

    if (force || weechat_config_boolean (buflist_config_look_enabled))
    {
        num_items = (force == 2) ?
            BUFLIST_BAR_NUM_ITEMS :
            weechat_config_integer (buflist_config_look_use_items);
        for (i = 0; i < num_items; i++)
        {
            weechat_bar_item_update (buflist_bar_item_get_name (i));
        }
    }
}

/*
 * Checks if the bar can be scrolled, the bar must have:
 * - a position "left" or "right"
 * - a filling "vertical"
 * - the item_name as first item.
 *
 * Returns:
 *   1: bar can be scrolled
 *   0: bar must not be scrolled
 */

int
buflist_bar_item_bar_can_scroll (struct t_gui_bar *bar, const char *item_name)
{
    const char *ptr_bar_name, *ptr_bar_position, *ptr_bar_filling;
    int items_count, *items_subcount;
    char ***items_name, str_option[1024];

    ptr_bar_name = weechat_hdata_string (buflist_hdata_bar, bar, "name");
    if (!ptr_bar_name)
        return 0;

    /* check that bar option "position" is "left" or "right" */
    snprintf (str_option, sizeof (str_option),
              "weechat.bar.%s.position",
              ptr_bar_name);
    ptr_bar_position = weechat_config_string (weechat_config_get (str_option));
    if (!ptr_bar_position
        || ((strcmp (ptr_bar_position, "left") != 0)
            && (strcmp (ptr_bar_position, "right") != 0)))
    {
        return 0;
    }

    /* check that bar option "filling_left_right" is "vertical" */
    snprintf (str_option, sizeof (str_option),
              "weechat.bar.%s.filling_left_right",
              ptr_bar_name);
    ptr_bar_filling = weechat_config_string (weechat_config_get (str_option));
    if (!ptr_bar_filling || (strcmp (ptr_bar_filling, "vertical") != 0))
    {
        return 0;
    }

    /* check that "buflist" is the first item in bar */
    items_count = weechat_hdata_integer (buflist_hdata_bar, bar,
                                         "items_count");
    if (items_count <= 0)
        return 0;
    items_subcount = weechat_hdata_pointer (buflist_hdata_bar, bar,
                                            "items_subcount");
    if (!items_subcount || (items_subcount[0] <= 0))
        return 0;
    items_name = weechat_hdata_pointer (buflist_hdata_bar, bar, "items_name");
    if (!items_name || !items_name[0] || !items_name[0][0]
        || (strcmp (items_name[0][0], item_name) != 0))
    {
        return 0;
    }

    /* OK, bar can be scrolled! */
    return 1;
}

/*
 * Auto-scrolls a bar window displaying buflist item.
 */

void
buflist_bar_item_auto_scroll_bar_window (struct t_gui_bar_window *bar_window,
                                         int line_number)
{
    int height, scroll_y, new_scroll_y, auto_scroll;
    char str_scroll[64];
    struct t_hashtable *hashtable;

    if (!bar_window || (line_number < 0))
        return;

    height = weechat_hdata_integer (buflist_hdata_bar_window, bar_window,
                                    "height");
    scroll_y = weechat_hdata_integer (buflist_hdata_bar_window, bar_window,
                                      "scroll_y");

    /* no scroll needed if the line_number is already displayed */
    if ((line_number >= scroll_y) && (line_number < scroll_y + height))
        return;

    hashtable = weechat_hashtable_new (8,
                                       WEECHAT_HASHTABLE_STRING,
                                       WEECHAT_HASHTABLE_STRING,
                                       NULL, NULL);
    if (hashtable)
    {
        auto_scroll = weechat_config_integer (buflist_config_look_auto_scroll);
        new_scroll_y = line_number - (((height - 1) * auto_scroll) / 100);
        if (new_scroll_y < 0)
            new_scroll_y = 0;
        snprintf (str_scroll, sizeof (str_scroll),
                  "%d", new_scroll_y);
        weechat_hashtable_set (hashtable, "scroll_y", str_scroll);
        weechat_hdata_update (buflist_hdata_bar_window, bar_window, hashtable);
        weechat_hashtable_free (hashtable);
    }
}

/*
 * Auto-scrolls all bars with a given buflist item as first item.
 */

void
buflist_bar_item_auto_scroll (const char *item_name, int line_number)
{
    struct t_gui_bar *ptr_bar;
    struct t_gui_bar_window *ptr_bar_window;
    struct t_gui_window *ptr_window;

    if (line_number < 0)
        return;

    /* auto-scroll in root bars */
    ptr_bar = weechat_hdata_get_list (buflist_hdata_bar, "gui_bars");
    while (ptr_bar)
    {
        ptr_bar_window = weechat_hdata_pointer (buflist_hdata_bar, ptr_bar,
                                                "bar_window");
        if (ptr_bar_window && buflist_bar_item_bar_can_scroll (ptr_bar,
                                                               item_name))
        {
            buflist_bar_item_auto_scroll_bar_window (ptr_bar_window,
                                                     line_number);
        }
        ptr_bar = weechat_hdata_move (buflist_hdata_bar, ptr_bar, 1);
    }

    /* auto-scroll in window bars */
    ptr_window = weechat_hdata_get_list (buflist_hdata_window, "gui_windows");
    while (ptr_window)
    {
        ptr_bar_window = weechat_hdata_pointer (buflist_hdata_window,
                                                ptr_window, "bar_windows");
        while (ptr_bar_window)
        {
            ptr_bar = weechat_hdata_pointer (buflist_hdata_bar_window,
                                             ptr_bar_window, "bar");
            if (buflist_bar_item_bar_can_scroll (ptr_bar, item_name))
            {
                buflist_bar_item_auto_scroll_bar_window (ptr_bar_window,
                                                         line_number);
            }
            ptr_bar_window = weechat_hdata_move (buflist_hdata_bar_window,
                                                 ptr_bar_window, 1);
        }
        ptr_window = weechat_hdata_move (buflist_hdata_window, ptr_window, 1);
    }
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
    struct t_gui_buffer *ptr_buffer_prev, *ptr_buffer_next;
    struct t_gui_nick *ptr_gui_nick;
    struct t_gui_hotlist *ptr_hotlist;
    void *ptr_server, *ptr_channel;
    char **buflist, *str_buflist, *condition;
    char str_format_number[32], str_format_number_empty[32];
    char str_nick_prefix[32], str_color_nick_prefix[32];
    char str_number[32], str_number2[32], *line, **hotlist, *str_hotlist;
    char str_hotlist_count[32];
    const char *ptr_format, *ptr_format_current, *ptr_format_indent;
    const char *ptr_name, *ptr_type, *ptr_nick, *ptr_nick_prefix;
    const char *ptr_hotlist_format, *ptr_hotlist_priority;
    const char *hotlist_priority_none = "none";
    const char *hotlist_priority[4] = { "low", "message", "private",
                                        "highlight" };
    const char indent_empty[1] = { '\0' };
    const char *ptr_lag, *ptr_item_name, *ptr_tls_version;
    int item_index, num_buffers, is_channel, is_private;
    int i, j, length_max_number, current_buffer, number, prev_number, priority;
    int rc, count, line_number, line_number_current_buffer;
    int hotlist_priority_number;

    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) extra_info;

    if (!weechat_config_boolean (buflist_config_look_enabled))
        return NULL;

    item_index = (int)((unsigned long)pointer);

    if (item_index + 1 > weechat_config_integer (buflist_config_look_use_items))
        return NULL;

    prev_number = -1;
    line_number = 0;
    line_number_current_buffer = 0;

    buflist = weechat_string_dyn_alloc (256);

    weechat_hashtable_set (buflist_hashtable_pointers, "bar_item", item);
    if (window)
        weechat_hashtable_set (buflist_hashtable_pointers, "window", window);

    ptr_format = buflist_config_format_buffer_eval;
    ptr_format_current = buflist_config_format_buffer_current_eval;

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

    if (buflist_list_buffers[item_index])
        weechat_arraylist_free (buflist_list_buffers[item_index]);
    buflist_list_buffers[item_index] = weechat_arraylist_new (
        16, 0, 1,
        NULL, NULL, NULL, NULL);

    buffers = buflist_sort_buffers (item);

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

        /* name / short name */
        ptr_name = weechat_hdata_string (buflist_hdata_buffer,
                                         ptr_buffer, "short_name");
        if (!ptr_name)
        {
            ptr_name = weechat_hdata_string (buflist_hdata_buffer,
                                             ptr_buffer, "name");
        }

        /* current buffer */
        current_buffer = (ptr_buffer == ptr_current_buffer);
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
        snprintf (str_number2, sizeof (str_number2),
                  str_format_number, number);

        /* buffer merged */
        ptr_buffer_prev = weechat_hdata_move (buflist_hdata_buffer,
                                              ptr_buffer, -1);
        ptr_buffer_next = weechat_hdata_move (buflist_hdata_buffer,
                                              ptr_buffer, 1);
        if ((ptr_buffer_prev
             && (weechat_hdata_integer (buflist_hdata_buffer,
                                        ptr_buffer_prev,
                                        "number") == number))
            || (ptr_buffer_next
                && (weechat_hdata_integer (buflist_hdata_buffer,
                                           ptr_buffer_next,
                                           "number") == number)))
        {
            weechat_hashtable_set (buflist_hashtable_extra_vars,
                                   "merged", "1");
        }
        else
        {
            weechat_hashtable_set (buflist_hashtable_extra_vars,
                                   "merged", "0");
        }

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
                               buflist_config_format_buffer_eval);
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "number", str_number);
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "number2", str_number2);
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "format_number",
                               weechat_config_string (
                                   buflist_config_format_number));
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "indent", ptr_format_indent);
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "name", ptr_name);
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "format_name",
                               weechat_config_string (
                                   buflist_config_format_name));

        /* hotlist */
        ptr_hotlist = weechat_hdata_pointer (buflist_hdata_buffer,
                                             ptr_buffer, "hotlist");
        ptr_hotlist_format = weechat_config_string (
            buflist_config_format_hotlist_level_none);
        ptr_hotlist_priority = hotlist_priority_none;
        hotlist_priority_number = -1;
        if (ptr_hotlist)
        {
            priority = weechat_hdata_integer (buflist_hdata_hotlist,
                                              ptr_hotlist, "priority");
            if ((priority >= 0) && (priority < 4))
            {
                ptr_hotlist_format = weechat_config_string (
                    buflist_config_format_hotlist_level[priority]);
                ptr_hotlist_priority = hotlist_priority[priority];
                hotlist_priority_number = priority;
            }
        }
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "color_hotlist", ptr_hotlist_format);
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "hotlist_priority", ptr_hotlist_priority);
        snprintf (str_number, sizeof (str_number),
                  "%d", hotlist_priority_number);
        weechat_hashtable_set (buflist_hashtable_extra_vars,
                               "hotlist_priority_number", str_number);
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
                                    buflist_config_format_hotlist_separator),
                                -1);
                        }
                        weechat_string_dyn_concat (
                            hotlist,
                            weechat_config_string (
                                buflist_config_format_hotlist_level[j]),
                            -1);
                        snprintf (str_hotlist_count, sizeof (str_hotlist_count),
                                  "%d", count);
                        weechat_string_dyn_concat (hotlist,
                                                   str_hotlist_count,
                                                   -1);
                    }
                }
                str_hotlist = weechat_string_dyn_free (hotlist, 0);
            }
        }
        weechat_hashtable_set (
            buflist_hashtable_extra_vars,
            "format_hotlist",
            (str_hotlist) ? buflist_config_format_hotlist_eval : "");
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

        /* tls version */
        ptr_tls_version = weechat_buffer_get_string (ptr_buffer, "localvar_tls_version");
        weechat_hashtable_set (
            buflist_hashtable_extra_vars,
            "format_tls_version",
            (ptr_tls_version && ptr_tls_version[0]) ?
            weechat_config_string (buflist_config_format_tls_version) : "");

        /* check condition: if false, the buffer is not displayed */
        condition = weechat_string_eval_expression (
            weechat_config_string (buflist_config_look_display_conditions),
            buflist_hashtable_pointers,
            buflist_hashtable_extra_vars,
            buflist_hashtable_options_conditions);
        rc = (condition && (strcmp (condition, "1") == 0));
        if (condition)
            free (condition);
        if (!rc)
            continue;

        /* add buffer in list */
        weechat_arraylist_add (buflist_list_buffers[item_index], ptr_buffer);

        /* set some other variables */
        if (current_buffer)
            line_number_current_buffer = line_number;
        prev_number = number;

        /* add newline between each buffer (if needed) */
        if (weechat_config_boolean (buflist_config_look_add_newline)
            && *buflist[0])
        {
            if (!weechat_string_dyn_concat (buflist, "\n", -1))
                goto error;
        }

        /* build string */
        line = weechat_string_eval_expression (
            (current_buffer) ? ptr_format_current : ptr_format,
            buflist_hashtable_pointers,
            buflist_hashtable_extra_vars,
            NULL);

        /* concatenate string */
        rc = weechat_string_dyn_concat (buflist, line, -1);
        free (line);
        if (!rc)
            goto error;

        line_number++;
    }

    str_buflist = weechat_string_dyn_free (buflist, 0);

    goto end;

error:
    weechat_string_dyn_free (buflist, 1);
    str_buflist = NULL;

end:
    weechat_arraylist_free (buffers);

    if ((line_number_current_buffer != old_line_number_current_buffer[item_index])
        && (weechat_config_integer (buflist_config_look_auto_scroll) >= 0))
    {
        ptr_item_name = weechat_hdata_string (buflist_hdata_bar_item,
                                              item, "name");
        buflist_bar_item_auto_scroll (ptr_item_name,
                                      line_number_current_buffer);
    }
    old_line_number_current_buffer[item_index] = line_number_current_buffer;

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
    int i;

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

    buflist_hashtable_options_conditions = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (!buflist_hashtable_options_conditions)
    {
        weechat_hashtable_free (buflist_hashtable_pointers);
        weechat_hashtable_free (buflist_hashtable_extra_vars);
        return 0;
    }
    weechat_hashtable_set (buflist_hashtable_options_conditions,
                           "type", "condition");

    /* bar items */
    for (i = 0; i < BUFLIST_BAR_NUM_ITEMS; i++)
    {
        buflist_list_buffers[i] = NULL;
        old_line_number_current_buffer[i] = -1;
        buflist_bar_item_buflist[i] = weechat_bar_item_new (
            buflist_bar_item_get_name (i),
            &buflist_bar_item_buflist_cb,
            (const void *)((unsigned long)i), NULL);
    }

    return 1;
}

/*
 * Ends buflist bar items.
 */

void
buflist_bar_item_end ()
{
    int i;

    for (i = 0; i < BUFLIST_BAR_NUM_ITEMS; i++)
    {
        weechat_bar_item_remove (buflist_bar_item_buflist[i]);
    }

    weechat_hashtable_free (buflist_hashtable_pointers);
    buflist_hashtable_pointers = NULL;

    weechat_hashtable_free (buflist_hashtable_extra_vars);
    buflist_hashtable_extra_vars = NULL;

    weechat_hashtable_free (buflist_hashtable_options_conditions);
    buflist_hashtable_options_conditions = NULL;

    for (i = 0; i < BUFLIST_BAR_NUM_ITEMS; i++)
    {
        if (buflist_list_buffers[i])
        {
            weechat_arraylist_free (buflist_list_buffers[i]);
            buflist_list_buffers[i] = NULL;
        }
    }
}
