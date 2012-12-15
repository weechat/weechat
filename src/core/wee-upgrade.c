/*
 * wee-upgrade.c - save/restore session data of WeeChat core
 *
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include "weechat.h"
#include "wee-upgrade.h"
#include "wee-hook.h"
#include "wee-infolist.h"
#include "wee-string.h"
#include "wee-util.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-history.h"
#include "../gui/gui-hotlist.h"
#include "../gui/gui-layout.h"
#include "../gui/gui-line.h"
#include "../gui/gui-nicklist.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"


struct t_gui_buffer *upgrade_current_buffer = NULL;
struct t_gui_buffer *upgrade_set_current_buffer = NULL;
int upgrade_set_current_window = 0;
int hotlist_reset = 0;
struct t_gui_layout_buffer *upgrade_layout_buffers = NULL;
struct t_gui_layout_buffer *last_upgrade_layout_buffer = NULL;
struct t_gui_layout_window *upgrade_layout_windows = NULL;


/*
 * Saves history in WeeChat upgrade file (from last to first, to restore it in
 * good order).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
upgrade_weechat_save_history (struct t_upgrade_file *upgrade_file,
                              struct t_gui_history *last_history)
{
    struct t_infolist *ptr_infolist;
    struct t_infolist_item *ptr_item;
    struct t_gui_history *ptr_history;
    int rc;

    if (!last_history)
        return 1;

    ptr_infolist = infolist_new ();
    if (!ptr_infolist)
        return 0;

    ptr_history = last_history;
    while (ptr_history)
    {
        ptr_item = infolist_new_item (ptr_infolist);
        if (!ptr_item)
        {
            infolist_free (ptr_infolist);
            return 0;
        }

        if (!infolist_new_var_string (ptr_item, "text", ptr_history->text))
        {
            infolist_free (ptr_infolist);
            return 0;
        }

        ptr_history = ptr_history->prev_history;
    }

    rc = upgrade_file_write_object (upgrade_file,
                                    UPGRADE_WEECHAT_TYPE_HISTORY,
                                    ptr_infolist);
    infolist_free (ptr_infolist);
    if (!rc)
        return 0;

    return 1;
}

/*
 * Saves buffers in WeeChat upgrade file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
upgrade_weechat_save_buffers (struct t_upgrade_file *upgrade_file)
{
    struct t_infolist *ptr_infolist;
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_line *ptr_line;
    int rc;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        /* save buffer */
        ptr_infolist = infolist_new ();
        if (!ptr_infolist)
            return 0;
        if (!gui_buffer_add_to_infolist (ptr_infolist, ptr_buffer))
        {
            infolist_free (ptr_infolist);
            return 0;
        }
        rc = upgrade_file_write_object (upgrade_file,
                                        UPGRADE_WEECHAT_TYPE_BUFFER,
                                        ptr_infolist);
        infolist_free (ptr_infolist);
        if (!rc)
            return 0;

        /* save nicklist */
        if (ptr_buffer->nicklist)
        {
            ptr_infolist = infolist_new ();
            if (!ptr_infolist)
                return 0;
            if (!gui_nicklist_add_to_infolist (ptr_infolist, ptr_buffer, NULL))
            {
                infolist_free (ptr_infolist);
                return 0;
            }
            rc = upgrade_file_write_object (upgrade_file,
                                            UPGRADE_WEECHAT_TYPE_NICKLIST,
                                            ptr_infolist);
            infolist_free (ptr_infolist);
            if (!rc)
                return 0;
        }

        /* save buffer lines */
        for (ptr_line = ptr_buffer->own_lines->first_line; ptr_line;
             ptr_line = ptr_line->next_line)
        {
            ptr_infolist = infolist_new ();
            if (!ptr_infolist)
                return 0;
            if (!gui_line_add_to_infolist (ptr_infolist,
                                           ptr_buffer->own_lines,
                                           ptr_line))
            {
                infolist_free (ptr_infolist);
                return 0;
            }
            rc = upgrade_file_write_object (upgrade_file,
                                            UPGRADE_WEECHAT_TYPE_BUFFER_LINE,
                                            ptr_infolist);
            infolist_free (ptr_infolist);
            if (!rc)
                return 0;
        }

        /* save command/text history of buffer */
        if (ptr_buffer->history)
        {
            rc = upgrade_weechat_save_history (upgrade_file,
                                               ptr_buffer->last_history);
            if (!rc)
                return 0;
        }
    }

    return 1;
}

/*
 * Saves miscellaneous info in WeeChat upgrade file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
upgrade_weechat_save_misc (struct t_upgrade_file *upgrade_file)
{
    struct t_infolist *ptr_infolist;
    struct t_infolist_item *ptr_item;
    int rc;

    ptr_infolist = infolist_new ();
    if (!ptr_infolist)
        return 0;

    ptr_item = infolist_new_item (ptr_infolist);
    if (!ptr_item)
    {
        infolist_free (ptr_infolist);
        return 0;
    }
    if (!infolist_new_var_time (ptr_item, "start_time", weechat_first_start_time))
    {
        infolist_free (ptr_infolist);
        return 0;
    }
    if (!infolist_new_var_integer (ptr_item, "upgrade_count", weechat_upgrade_count))
    {
        infolist_free (ptr_infolist);
        return 0;
    }
    if (!infolist_new_var_integer (ptr_item, "current_window_number", gui_current_window->number))
    {
        infolist_free (ptr_infolist);
        return 0;
    }

    rc = upgrade_file_write_object (upgrade_file,
                                    UPGRADE_WEECHAT_TYPE_MISC,
                                    ptr_infolist);
    infolist_free (ptr_infolist);

    return rc;
}

/*
 * Saves hotlist in WeeChat upgrade file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
upgrade_weechat_save_hotlist (struct t_upgrade_file *upgrade_file)
{
    struct t_infolist *ptr_infolist;
    struct t_gui_hotlist *ptr_hotlist;
    int rc;

    for (ptr_hotlist = gui_hotlist; ptr_hotlist;
         ptr_hotlist = ptr_hotlist->next_hotlist)
    {
        ptr_infolist = infolist_new ();
        if (!ptr_infolist)
            return 0;
        if (!gui_hotlist_add_to_infolist (ptr_infolist, ptr_hotlist))
        {
            infolist_free (ptr_infolist);
            return 0;
        }
        rc = upgrade_file_write_object (upgrade_file,
                                        UPGRADE_WEECHAT_TYPE_HOTLIST,
                                        ptr_infolist);
        infolist_free (ptr_infolist);
        if (!rc)
            return 0;
    }

    return 1;
}

/*
 * Saves tree with layout for windows in WeeChat upgrade file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
upgrade_weechat_save_layout_window_tree (struct t_upgrade_file *upgrade_file,
                                         struct t_gui_layout_window *layout_window)
{
    struct t_infolist *ptr_infolist;
    int rc;

    ptr_infolist = infolist_new ();
    if (!ptr_infolist)
        return 0;

    if (!gui_layout_window_add_to_infolist (ptr_infolist, layout_window))
    {
        infolist_free (ptr_infolist);
        return 0;
    }

    rc = upgrade_file_write_object (upgrade_file,
                                    UPGRADE_WEECHAT_TYPE_LAYOUT_WINDOW,
                                    ptr_infolist);

    infolist_free (ptr_infolist);
    if (!rc)
        return 0;

    if (layout_window->child1)
    {
        if (!upgrade_weechat_save_layout_window_tree (upgrade_file,
                                                      layout_window->child1))
            return 0;
    }

    if (layout_window->child2)
    {
        if (!upgrade_weechat_save_layout_window_tree (upgrade_file,
                                                      layout_window->child2))
            return 0;
    }

    return 1;
}

/*
 * Saves layout for windows in WeeChat upgrade file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
upgrade_weechat_save_layout_window (struct t_upgrade_file *upgrade_file)
{
    struct t_gui_layout_window *layout_windows;
    int rc;

    /* get current layout for windows */
    layout_windows = NULL;
    gui_layout_window_save (&layout_windows);

    /* save tree with layout of windows */
    rc = upgrade_weechat_save_layout_window_tree (upgrade_file, layout_windows);

    gui_layout_window_remove_all (&layout_windows);

    return rc;
}

/*
 * Saves WeeChat upgrade file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
upgrade_weechat_save ()
{
    int rc;
    struct t_upgrade_file *upgrade_file;

    upgrade_file = upgrade_file_new (WEECHAT_UPGRADE_FILENAME, 1);
    if (!upgrade_file)
        return 0;

    rc = 1;
    rc &= upgrade_weechat_save_history (upgrade_file, last_gui_history);
    rc &= upgrade_weechat_save_buffers (upgrade_file);
    rc &= upgrade_weechat_save_misc (upgrade_file);
    rc &= upgrade_weechat_save_hotlist (upgrade_file);
    rc &= upgrade_weechat_save_layout_window (upgrade_file);

    upgrade_file_close (upgrade_file);

    return rc;
}

/*
 * Reads WeeChat upgrade file.
 */

int
upgrade_weechat_read_cb (void *data,
                         struct t_upgrade_file *upgrade_file,
                         int object_id,
                         struct t_infolist *infolist)
{
    const char *key, *var_name, *type, *name, *group_name, *plugin_name;
    const char *buffer_name;
    char option_name[64], *option_key, *option_var;
    struct t_gui_nick_group *ptr_group;
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_line *new_line;
    struct t_gui_hotlist *new_hotlist;
    struct timeval creation_time;
    void *buf;
    int i, size, index, length;

    /* make C compiler happy */
    (void) data;
    (void) upgrade_file;

    infolist_reset_item_cursor (infolist);
    while (infolist_next (infolist))
    {
        switch (object_id)
        {
            case UPGRADE_WEECHAT_TYPE_HISTORY:
                if (upgrade_current_buffer)
                {
                    gui_history_buffer_add (upgrade_current_buffer,
                                            infolist_string (infolist, "text"));
                }
                else
                {
                    gui_history_global_add (infolist_string (infolist, "text"));
                }
                break;
            case UPGRADE_WEECHAT_TYPE_BUFFER:
                plugin_name = infolist_string (infolist, "plugin_name");
                name = infolist_string (infolist, "name");
                gui_layout_buffer_add (&upgrade_layout_buffers,
                                       &last_upgrade_layout_buffer,
                                       plugin_name, name,
                                       infolist_integer (infolist, "number"));
                if (gui_buffer_is_main (plugin_name, name))
                {
                    /* use WeeChat main buffer */
                    upgrade_current_buffer = gui_buffers;
                }
                else
                {
                    /*
                     * create buffer if it was created by a plugin (ie not
                     * WeeChat main buffer)
                     */
                    upgrade_current_buffer = gui_buffer_new (
                        NULL,
                        infolist_string (infolist, "name"),
                        NULL, NULL,
                        NULL, NULL);
                    if (upgrade_current_buffer)
                    {
                        if (infolist_integer (infolist, "current_buffer"))
                            upgrade_set_current_buffer = upgrade_current_buffer;
                        upgrade_current_buffer->plugin_name_for_upgrade =
                            strdup (infolist_string (infolist, "plugin_name"));
                        gui_buffer_build_full_name (upgrade_current_buffer);
                        upgrade_current_buffer->short_name =
                            (infolist_string (infolist, "short_name")) ?
                            strdup (infolist_string (infolist, "short_name")) : NULL;
                        upgrade_current_buffer->type =
                            infolist_integer (infolist, "type");
                        upgrade_current_buffer->notify =
                            infolist_integer (infolist, "notify");
                        upgrade_current_buffer->nicklist_case_sensitive =
                            infolist_integer (infolist, "nicklist_case_sensitive");
                        upgrade_current_buffer->nicklist_display_groups =
                            infolist_integer (infolist, "nicklist_display_groups");
                        upgrade_current_buffer->title =
                            (infolist_string (infolist, "title")) ?
                            strdup (infolist_string (infolist, "title")) : NULL;
                        upgrade_current_buffer->lines->first_line_not_read =
                            infolist_integer (infolist, "first_line_not_read");
                        upgrade_current_buffer->time_for_each_line =
                            infolist_integer (infolist, "time_for_each_line");
                        upgrade_current_buffer->input =
                            infolist_integer (infolist, "input");
                        upgrade_current_buffer->input_get_unknown_commands =
                            infolist_integer (infolist, "input_get_unknown_commands");
                        if (infolist_integer (infolist, "input_buffer_alloc") > 0)
                        {
                            upgrade_current_buffer->input_buffer =
                                malloc (infolist_integer (infolist, "input_buffer_alloc"));
                            if (upgrade_current_buffer->input_buffer)
                            {
                                upgrade_current_buffer->input_buffer_size =
                                    infolist_integer (infolist, "input_buffer_size");
                                upgrade_current_buffer->input_buffer_length =
                                    infolist_integer (infolist, "input_buffer_length");
                                upgrade_current_buffer->input_buffer_pos =
                                    infolist_integer (infolist, "input_buffer_pos");
                                upgrade_current_buffer->input_buffer_1st_display =
                                    infolist_integer (infolist, "input_buffer_1st_display");
                                if (infolist_string (infolist, "input_buffer"))
                                    strcpy (upgrade_current_buffer->input_buffer,
                                            infolist_string (infolist, "input_buffer"));
                                else
                                    upgrade_current_buffer->input_buffer[0] = '\0';
                            }
                        }
                        upgrade_current_buffer->text_search =
                            infolist_integer (infolist, "text_search");
                        upgrade_current_buffer->text_search_exact =
                            infolist_integer (infolist, "text_search_exact");
                        upgrade_current_buffer->text_search_found =
                            infolist_integer (infolist, "text_search_found");
                        if (infolist_string (infolist, "text_search_input"))
                            upgrade_current_buffer->text_search_input =
                                strdup (infolist_string (infolist, "text_search_input"));
                        gui_buffer_set_highlight_words (upgrade_current_buffer,
                                                        infolist_string (infolist, "highlight_words"));
                        gui_buffer_set_highlight_regex (upgrade_current_buffer,
                                                        infolist_string (infolist, "highlight_regex"));
                        gui_buffer_set_highlight_tags (upgrade_current_buffer,
                                                       infolist_string (infolist, "highlight_tags"));
                        gui_buffer_set_hotlist_max_level_nicks (upgrade_current_buffer,
                                                                infolist_string (infolist, "hotlist_max_level_nicks"));
                        index = 0;
                        while (1)
                        {
                            snprintf (option_name, sizeof (option_name),
                                      "key_%05d", index);
                            key = infolist_string (infolist, option_name);
                            if (!key)
                                break;
                            length = 16 + strlen (key) + 1;
                            option_key = malloc (length);
                            if (option_key)
                            {
                                snprintf (option_key, length, "key_bind_%s", key);
                                snprintf (option_name, sizeof (option_name),
                                          "key_command_%05d", index);
                                gui_buffer_set (upgrade_current_buffer,
                                                option_key,
                                                infolist_string (infolist, option_name));
                                free (option_key);
                            }
                            index++;
                        }
                        index = 0;
                        while (1)
                        {
                            snprintf (option_name, sizeof (option_name),
                                      "localvar_name_%05d", index);
                            var_name = infolist_string (infolist, option_name);
                            if (!var_name)
                                break;
                            length = 32 + strlen (var_name) + 1;
                            option_var = malloc (length);
                            if (option_var)
                            {
                                snprintf (option_var, length, "localvar_set_%s", var_name);
                                snprintf (option_name, sizeof (option_name),
                                          "localvar_value_%05d", index);
                                gui_buffer_set (upgrade_current_buffer,
                                                option_var,
                                                infolist_string (infolist, option_name));
                                free (option_var);
                            }
                            index++;
                        }
                    }
                }
                break;
            case UPGRADE_WEECHAT_TYPE_BUFFER_LINE:
                /* add line to current buffer */
                if (upgrade_current_buffer)
                {
                    switch (upgrade_current_buffer->type)
                    {
                        case GUI_BUFFER_TYPE_FORMATTED:
                            new_line = gui_line_add (
                                upgrade_current_buffer,
                                infolist_time (infolist, "date"),
                                infolist_time (infolist, "date_printed"),
                                infolist_string (infolist, "tags"),
                                infolist_string (infolist, "prefix"),
                                infolist_string (infolist, "message"));
                            if (new_line)
                            {
                                new_line->data->highlight = infolist_integer (infolist, "highlight");
                                if (infolist_integer (infolist, "last_read_line"))
                                    upgrade_current_buffer->lines->last_read_line = new_line;
                            }
                            break;
                        case GUI_BUFFER_TYPE_FREE:
                            gui_line_add_y (
                                upgrade_current_buffer,
                                infolist_integer (infolist, "y"),
                                infolist_string (infolist, "message"));
                            break;
                        case GUI_BUFFER_NUM_TYPES:
                            break;
                    }
                }
                break;
            case UPGRADE_WEECHAT_TYPE_NICKLIST:
                if (upgrade_current_buffer)
                {
                    upgrade_current_buffer->nicklist = 1;
                    ptr_group = NULL;
                    type = infolist_string (infolist, "type");
                    if (type)
                    {
                        if (strcmp (type, "group") == 0)
                        {
                            name = infolist_string (infolist, "name");
                            if (name && (strcmp (name, "root") != 0))
                            {
                                group_name = infolist_string (infolist, "parent_name");
                                if (group_name)
                                    ptr_group = gui_nicklist_search_group (upgrade_current_buffer,
                                                                           NULL,
                                                                           group_name);
                                gui_nicklist_add_group (
                                    upgrade_current_buffer,
                                    ptr_group,
                                    name,
                                    infolist_string (infolist, "color"),
                                    infolist_integer (infolist, "visible"));
                            }
                        }
                        else if (strcmp (type, "nick") == 0)
                        {
                            group_name = infolist_string (infolist, "group_name");
                            if (group_name)
                                ptr_group = gui_nicklist_search_group (upgrade_current_buffer,
                                                                       NULL,
                                                                       group_name);
                            gui_nicklist_add_nick (
                                upgrade_current_buffer,
                                ptr_group,
                                infolist_string (infolist, "name"),
                                infolist_string (infolist, "color"),
                                infolist_string (infolist, "prefix"),
                                infolist_string (infolist, "prefix_color"),
                                infolist_integer (infolist, "visible"));
                        }
                    }
                }
                break;
            case UPGRADE_WEECHAT_TYPE_MISC:
                weechat_first_start_time = infolist_time (infolist, "start_time");
                weechat_upgrade_count = infolist_integer (infolist, "upgrade_count");
                upgrade_set_current_window = infolist_integer (infolist, "current_window_number");
                break;
            case UPGRADE_WEECHAT_TYPE_HOTLIST:
                if (!hotlist_reset)
                {
                    gui_hotlist_clear ();
                    hotlist_reset = 1;
                }
                plugin_name = infolist_string (infolist, "plugin_name");
                buffer_name = infolist_string (infolist, "buffer_name");
                if (plugin_name && buffer_name)
                {
                    ptr_buffer = gui_buffer_search_by_name (plugin_name,
                                                            buffer_name);
                    if (ptr_buffer)
                    {
                        buf = infolist_buffer (infolist, "creation_time", &size);
                        if (buf)
                        {
                            memcpy (&creation_time, buf, size);
                            new_hotlist = gui_hotlist_add (ptr_buffer,
                                                           infolist_integer (infolist, "priority"),
                                                           &creation_time);
                            if (new_hotlist)
                            {
                                for (i = 0; i < GUI_HOTLIST_NUM_PRIORITIES; i++)
                                {
                                    snprintf (option_name, sizeof (option_name),
                                              "count_%02d", i);
                                    new_hotlist->count[i] = infolist_integer (infolist,
                                                                              option_name);
                                }
                            }
                        }
                    }
                }
                break;
            case UPGRADE_WEECHAT_TYPE_LAYOUT_WINDOW:
                gui_layout_window_add (&upgrade_layout_windows,
                                       infolist_integer (infolist, "internal_id"),
                                       gui_layout_window_search_by_id (upgrade_layout_windows,
                                                                       infolist_integer (infolist, "parent_id")),
                                       infolist_integer (infolist, "split_pct"),
                                       infolist_integer (infolist, "split_horiz"),
                                       infolist_string (infolist, "plugin_name"),
                                       infolist_string (infolist, "buffer_name"));
                break;
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Loads WeeChat upgrade file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
upgrade_weechat_load ()
{
    int rc;
    struct t_upgrade_file *upgrade_file;

    upgrade_file = upgrade_file_new (WEECHAT_UPGRADE_FILENAME, 0);
    rc = upgrade_file_read (upgrade_file, &upgrade_weechat_read_cb, NULL);

    if (!hotlist_reset)
        gui_hotlist_clear ();

    gui_color_buffer_assign ();
    gui_color_buffer_display ();

    if (upgrade_layout_buffers)
    {
        gui_layout_buffer_apply (upgrade_layout_buffers);
        gui_layout_buffer_remove_all (&upgrade_layout_buffers,
                                      &last_upgrade_layout_buffer);
    }

    if (upgrade_layout_windows)
    {
        gui_layout_window_apply (upgrade_layout_windows, -1);
        gui_layout_window_remove_all (&upgrade_layout_windows);
    }

    if (upgrade_set_current_window > 0)
        gui_window_switch_by_number (upgrade_set_current_window);

    if (upgrade_set_current_buffer)
    {
        gui_window_switch_to_buffer (gui_current_window,
                                     upgrade_set_current_buffer, 0);
    }

    gui_layout_buffer_get_number_all (gui_layout_buffers);

    return rc;
}

/*
 * Removes a .upgrade file (callback called for each .upgrade file in WeeChat
 * home directory).
 */

void
upgrade_weechat_remove_file_cb (void *data, const char *filename)
{
    /* make C compiler happy */
    (void) data;

    if (string_match (filename, "*.upgrade", 1))
    {
        if (weechat_debug_core >= 2)
            gui_chat_printf (NULL, _("debug: removing file: %s"), filename);
        unlink (filename);
    }
}

/*
 * Removes *.upgrade files after upgrade and send signal "weechat_upgrade_done".
 */

void
upgrade_weechat_end ()
{
    struct timeval tv_now;
    long time_diff;

    /* remove .upgrade files */
    util_exec_on_files (weechat_home,
                        0,
                        NULL,
                        &upgrade_weechat_remove_file_cb);

    /* display message for end of /upgrade with duration */
    gettimeofday (&tv_now, NULL);
    time_diff = util_timeval_diff (&weechat_current_start_timeval, &tv_now);
    gui_chat_printf (NULL,
                     /* TRANSLATORS: "%s" is translation of "second" or "seconds" */
                     _("Upgrade done (%.02f %s)"),
                     ((float)time_diff) / 1000,
                     NG_("second", "seconds", time_diff / 1000));

    /* upgrading ended */
    weechat_upgrading = 0;

    /* send signal for end of /upgrade */
    hook_signal_send ("upgrade_ended", WEECHAT_HOOK_SIGNAL_STRING, NULL);
}
