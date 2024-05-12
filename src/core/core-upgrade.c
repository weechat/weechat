/*
 * core-upgrade.c - save/restore session data of WeeChat core
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include "weechat.h"
#include "core-upgrade.h"
#include "core-dir.h"
#include "core-hook.h"
#include "core-infolist.h"
#include "core-secure-buffer.h"
#include "core-string.h"
#include "core-util.h"
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
struct t_gui_layout *upgrade_layout = NULL;


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

    ptr_infolist = infolist_new (NULL);
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
        ptr_infolist = infolist_new (NULL);
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
            ptr_infolist = infolist_new (NULL);
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
            ptr_infolist = infolist_new (NULL);
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

    ptr_infolist = infolist_new (NULL);
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
        ptr_infolist = infolist_new (NULL);
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

    ptr_infolist = infolist_new (NULL);
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
    struct t_gui_layout *ptr_layout;
    int rc;

    rc = 0;

    /* get current layout for windows */
    ptr_layout = gui_layout_alloc (GUI_LAYOUT_UPGRADE);

    if (ptr_layout)
    {
        gui_layout_window_store (ptr_layout);

        /* save tree with layout of windows */
        rc = upgrade_weechat_save_layout_window_tree (upgrade_file, ptr_layout->layout_windows);

        gui_layout_free (ptr_layout);
    }

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

    upgrade_file = upgrade_file_new (WEECHAT_UPGRADE_FILENAME,
                                     NULL, NULL, NULL);
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
 * Reads a buffer from infolist.
 */

void
upgrade_weechat_read_buffer (struct t_infolist *infolist)
{
    struct t_gui_buffer *ptr_buffer;
    const char *key, *var_name, *name, *plugin_name, *ptr_id;
    const char *str;
    char option_name[64], *option_key, *option_var, *error;
    int index, length, main_buffer;
    long long id;

    /* "id" is new in WeeChat 4.3.0 */
    id = -1;
    if (infolist_search_var (infolist, "id"))
    {
        ptr_id = infolist_string (infolist, "id");
        if (ptr_id)
        {
            error = NULL;
            id = strtoll (ptr_id, &error, 10);
            if (!error || error[0])
                id = -1;
        }
    }
    if (id < 0)
        id = gui_buffer_generate_id ();

    plugin_name = infolist_string (infolist, "plugin_name");
    name = infolist_string (infolist, "name");
    gui_layout_buffer_add (upgrade_layout,
                           plugin_name, name,
                           infolist_integer (infolist, "number"));
    main_buffer = gui_buffer_is_main (plugin_name, name);
    if (main_buffer)
    {
        /* use WeeChat main buffer */
        upgrade_current_buffer = gui_buffers;
        upgrade_current_buffer->id = id;
    }
    else
    {
        /* create buffer it it's not main buffer */
        upgrade_current_buffer = gui_buffer_new_props_with_id (
            id,
            NULL,  /* plugin */
            infolist_string (infolist, "name"),
            NULL,  /* properties */
            NULL, NULL, NULL,  /* input callback */
            NULL, NULL, NULL);  /* close callback */
    }
    if (!upgrade_current_buffer)
        return;

    if (upgrade_current_buffer->id > gui_buffer_last_id_assigned)
        gui_buffer_last_id_assigned = upgrade_current_buffer->id;

    ptr_buffer = upgrade_current_buffer;

    if (infolist_integer (infolist, "current_buffer"))
        upgrade_set_current_buffer = ptr_buffer;

    /* name for upgrade */
    free (ptr_buffer->plugin_name_for_upgrade);
    ptr_buffer->plugin_name_for_upgrade =
        strdup (infolist_string (infolist, "plugin_name"));

    /* full name */
    gui_buffer_build_full_name (ptr_buffer);

    /* old full name */
    free (ptr_buffer->old_full_name);
    str = infolist_string (infolist, "old_full_name");
    ptr_buffer->old_full_name = (str) ? strdup (str) : NULL;

    /* short name */
    free (ptr_buffer->short_name);
    str = infolist_string (infolist, "short_name");
    ptr_buffer->short_name = (str) ? strdup (str) : NULL;

    /* buffer type */
    ptr_buffer->type = infolist_integer (infolist, "type");

    /* notify level */
    ptr_buffer->notify = infolist_integer (infolist, "notify");

    /* "hidden" is new in WeeChat 1.0 */
    if (infolist_search_var (infolist, "hidden"))
        ptr_buffer->hidden = infolist_integer (infolist, "hidden");
    else
        ptr_buffer->hidden = 0;

    /* day change */
    if (infolist_search_var (infolist, "day_change"))
        ptr_buffer->day_change = infolist_integer (infolist, "day_change");
    else
        ptr_buffer->day_change = 1;

    /* "clear" is new in WeeChat 1.0 */
    if (infolist_search_var (infolist, "clear"))
        ptr_buffer->clear = infolist_integer (infolist, "clear");
    else
        ptr_buffer->clear = (ptr_buffer->type == GUI_BUFFER_TYPE_FREE) ? 0 : 1;

    /* "filter" is new in WeeChat 1.0 */
    if (infolist_search_var (infolist, "filter"))
        ptr_buffer->filter = infolist_integer (infolist, "filter");
    else
        ptr_buffer->filter = 1;

    /* nicklist */
    ptr_buffer->nicklist_case_sensitive =
        infolist_integer (infolist, "nicklist_case_sensitive");
    ptr_buffer->nicklist_display_groups =
        infolist_integer (infolist, "nicklist_display_groups");

    /* title (not for main buffer, because there's the latest version) */
    if (!main_buffer)
    {
        free (ptr_buffer->title);
        str = infolist_string (infolist, "title");
        ptr_buffer->title = (str) ? strdup (str) : NULL;
    }

    /* modes */
    gui_buffer_set_modes (ptr_buffer, infolist_string (infolist, "modes"));

    /* first line not read */
    ptr_buffer->lines->first_line_not_read =
        infolist_integer (infolist, "first_line_not_read");

    /* next line id */
    ptr_buffer->next_line_id = infolist_integer (infolist, "next_line_id");

    /* time for each line */
    ptr_buffer->time_for_each_line =
        infolist_integer (infolist, "time_for_each_line");

    /* input */
    gui_buffer_set_input_prompt (
        ptr_buffer, infolist_string (infolist, "input_prompt"));
    ptr_buffer->input = infolist_integer (infolist, "input");
    ptr_buffer->input_get_any_user_data =
        infolist_integer (infolist, "input_get_any_user_data");
    ptr_buffer->input_get_unknown_commands =
        infolist_integer (infolist, "input_get_unknown_commands");
    ptr_buffer->input_get_empty =
        infolist_integer (infolist, "input_get_empty");
    ptr_buffer->input_multiline =
        infolist_integer (infolist, "input_multiline");

    if (infolist_integer (infolist, "input_buffer_alloc") > 0)
    {
        ptr_buffer->input_buffer =
            malloc (infolist_integer (infolist, "input_buffer_alloc"));
        if (ptr_buffer->input_buffer)
        {
            ptr_buffer->input_buffer_size =
                infolist_integer (infolist, "input_buffer_size");
            ptr_buffer->input_buffer_length =
                infolist_integer (infolist, "input_buffer_length");
            ptr_buffer->input_buffer_pos =
                infolist_integer (infolist, "input_buffer_pos");
            ptr_buffer->input_buffer_1st_display =
                infolist_integer (infolist, "input_buffer_1st_display");
            if (infolist_string (infolist, "input_buffer"))
                strcpy (ptr_buffer->input_buffer,
                        infolist_string (infolist, "input_buffer"));
            else
                ptr_buffer->input_buffer[0] = '\0';
        }
    }

    /* text search is disabled after upgrade */

    /* highlight options */
    gui_buffer_set_highlight_words (
        ptr_buffer, infolist_string (infolist, "highlight_words"));
    gui_buffer_set_highlight_disable_regex (
        ptr_buffer, infolist_string (infolist, "highlight_disable_regex"));
    gui_buffer_set_highlight_regex (
        ptr_buffer, infolist_string (infolist, "highlight_regex"));
    if (infolist_search_var (infolist,
                             "highlight_tags_restrict"))
    {
        /* WeeChat >= 0.4.3 */
        gui_buffer_set_highlight_tags_restrict (
            ptr_buffer, infolist_string (infolist, "highlight_tags_restrict"));
        gui_buffer_set_highlight_tags (
            ptr_buffer, infolist_string (infolist, "highlight_tags"));
    }
    else
    {
        /* WeeChat <= 0.4.2 */
        gui_buffer_set_highlight_tags_restrict (
            ptr_buffer, infolist_string (infolist, "highlight_tags"));
    }

    /* hotlist max level nicks */
    gui_buffer_set_hotlist_max_level_nicks (
        ptr_buffer, infolist_string (infolist, "hotlist_max_level_nicks"));

    /* local keys */
    index = 0;
    while (1)
    {
        snprintf (option_name, sizeof (option_name), "key_%05d", index);
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
            gui_buffer_set (ptr_buffer, option_key,
                            infolist_string (infolist, option_name));
            free (option_key);
        }
        index++;
    }

    /* local variables */
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
            gui_buffer_set (ptr_buffer, option_var,
                            infolist_string (infolist, option_name));
            free (option_var);
        }
        index++;
    }
}

/*
 * Reads a buffer line from infolist.
 */

void
upgrade_weechat_read_buffer_line (struct t_infolist *infolist)
{
    struct t_gui_line *new_line;

    if (!upgrade_current_buffer)
        return;

    switch (upgrade_current_buffer->type)
    {
        case GUI_BUFFER_TYPE_FORMATTED:
            new_line = gui_line_new (
                upgrade_current_buffer,
                -1,
                infolist_time (infolist, "date"),
                infolist_integer (infolist, "date_usec"),
                infolist_time (infolist, "date_printed"),
                infolist_integer (infolist, "date_usec_printed"),
                infolist_string (infolist, "tags"),
                infolist_string (infolist, "prefix"),
                infolist_string (infolist, "message"));
            if (new_line)
            {
                new_line->data->id = infolist_integer (infolist, "id");
                gui_line_add (new_line);
                new_line->data->highlight = infolist_integer (infolist,
                                                              "highlight");
                if (infolist_integer (infolist, "last_read_line"))
                    upgrade_current_buffer->lines->last_read_line = new_line;
            }
            break;
        case GUI_BUFFER_TYPE_FREE:
            new_line = gui_line_new (
                upgrade_current_buffer,
                infolist_integer (infolist, "y"),
                infolist_time (infolist, "date"),
                infolist_integer (infolist, "date_usec"),
                infolist_time (infolist, "date_printed"),
                infolist_integer (infolist, "date_usec_printed"),
                infolist_string (infolist, "tags"),
                NULL,
                infolist_string (infolist, "message"));
            if (new_line)
            {
                new_line->data->id = infolist_integer (infolist, "id");
                gui_line_add_y (new_line);
            }
            break;
        case GUI_BUFFER_NUM_TYPES:
            break;
    }
}

/*
 * Reads a nicklist from infolist.
 */

void
upgrade_weechat_read_nicklist (struct t_infolist *infolist)
{
    struct t_gui_nick_group *ptr_group;
    const char *type, *name, *group_name, *ptr_id;
    char *error;
    long long id;

    if (!upgrade_current_buffer)
        return;

    upgrade_current_buffer->nicklist = 1;
    ptr_group = NULL;
    type = infolist_string (infolist, "type");
    if (!type)
        return;

    /* "id" is new in WeeChat 4.3.0 */
    id = -1;
    if (infolist_search_var (infolist, "id"))
    {
        ptr_id = infolist_string (infolist, "id");
        if (ptr_id)
        {
            error = NULL;
            id = strtoll (ptr_id, &error, 10);
            if (!error || error[0])
                id = -1;
        }
    }
    if (id < 0)
        id = gui_nicklist_generate_id (upgrade_current_buffer);

    if (strcmp (type, "group") == 0)
    {
        name = infolist_string (infolist, "name");
        if (name && (strcmp (name, "root") != 0))
        {
            group_name = infolist_string (infolist, "parent_name");
            if (group_name)
            {
                ptr_group = gui_nicklist_search_group (
                    upgrade_current_buffer, NULL, group_name);
            }
            gui_nicklist_add_group_with_id (
                upgrade_current_buffer,
                id,
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
        {
            ptr_group = gui_nicklist_search_group (
                upgrade_current_buffer, NULL, group_name);
        }
        gui_nicklist_add_nick_with_id (
            upgrade_current_buffer,
            id,
            ptr_group,
            infolist_string (infolist, "name"),
            infolist_string (infolist, "color"),
            infolist_string (infolist, "prefix"),
            infolist_string (infolist, "prefix_color"),
            infolist_integer (infolist, "visible"));
    }
}

/*
 * Reads hotlist from infolist.
 */

void
upgrade_weechat_read_hotlist (struct t_infolist *infolist)
{
    const char *plugin_name, *buffer_name;
    char option_name[64];
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_hotlist *new_hotlist;
    struct timeval creation_time;
    void *buf;
    int i, size;

    if (!hotlist_reset)
    {
        gui_hotlist_clear (GUI_HOTLIST_MASK_MAX);
        hotlist_reset = 1;
    }
    plugin_name = infolist_string (infolist, "plugin_name");
    buffer_name = infolist_string (infolist, "buffer_name");
    if (plugin_name && buffer_name)
    {
        ptr_buffer = gui_buffer_search (plugin_name, buffer_name);
        if (ptr_buffer)
        {
            buf = infolist_buffer (infolist, "creation_time", &size);
            if (buf)
            {
                memcpy (&creation_time, buf, size);
                new_hotlist = gui_hotlist_add (
                    ptr_buffer,
                    infolist_integer (infolist, "priority"),
                    &creation_time,
                    1);  /* check_conditions */
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
}

/*
 * Reads WeeChat upgrade file.
 */

int
upgrade_weechat_read_cb (const void *pointer, void *data,
                         struct t_upgrade_file *upgrade_file,
                         int object_id,
                         struct t_infolist *infolist)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) upgrade_file;

    gui_buffer_last_id_assigned = -1;

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
                upgrade_weechat_read_buffer (infolist);
                break;
            case UPGRADE_WEECHAT_TYPE_BUFFER_LINE:
                upgrade_weechat_read_buffer_line (infolist);
                break;
            case UPGRADE_WEECHAT_TYPE_NICKLIST:
                upgrade_weechat_read_nicklist (infolist);
                break;
            case UPGRADE_WEECHAT_TYPE_MISC:
                weechat_first_start_time = infolist_time (infolist, "start_time");
                weechat_upgrade_count = infolist_integer (infolist, "upgrade_count");
                upgrade_set_current_window = infolist_integer (infolist, "current_window_number");
                break;
            case UPGRADE_WEECHAT_TYPE_HOTLIST:
                upgrade_weechat_read_hotlist (infolist);
                break;
            case UPGRADE_WEECHAT_TYPE_LAYOUT_WINDOW:
                gui_layout_window_add (&upgrade_layout->layout_windows,
                                       infolist_integer (infolist, "internal_id"),
                                       gui_layout_window_search_by_id (upgrade_layout->layout_windows,
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

    upgrade_layout = gui_layout_alloc (GUI_LAYOUT_UPGRADE);

    upgrade_file = upgrade_file_new (WEECHAT_UPGRADE_FILENAME,
                                     &upgrade_weechat_read_cb, NULL, NULL);
    if (!upgrade_file)
        return 0;

    rc = upgrade_file_read (upgrade_file);

    upgrade_file_close (upgrade_file);

    if (!hotlist_reset)
        gui_hotlist_clear (GUI_HOTLIST_MASK_MAX);

    gui_color_buffer_assign ();
    gui_color_buffer_display ();

    gui_buffer_user_set_callbacks ();

    secure_buffer_assign ();
    secure_buffer_display ();

    if (upgrade_layout->layout_buffers)
        gui_layout_buffer_apply (upgrade_layout);
    if (upgrade_layout->layout_windows)
        gui_layout_window_apply (upgrade_layout, -1);

    gui_layout_free (upgrade_layout);
    upgrade_layout = NULL;

    if (upgrade_set_current_window > 0)
        gui_window_switch_by_number (upgrade_set_current_window);

    if (upgrade_set_current_buffer)
    {
        gui_window_switch_to_buffer (gui_current_window,
                                     upgrade_set_current_buffer, 0);
    }

    gui_layout_buffer_get_number_all (gui_layout_current);

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
    long long time_diff;

    /* remove .upgrade files */
    dir_exec_on_files (weechat_data_dir,
                       0, 0,
                       &upgrade_weechat_remove_file_cb, NULL);

    /* display message for end of /upgrade with duration */
    gettimeofday (&tv_now, NULL);
    time_diff = util_timeval_diff (&weechat_current_start_timeval, &tv_now);
    gui_chat_printf (NULL,
                     /* TRANSLATORS: %.02fs is a float number + "s" ("seconds") */
                     _("Upgrade done (%.02fs)"),
                     ((float)time_diff) / 1000000);

    /* upgrading ended */
    weechat_upgrading = 0;

    /* send signal for end of /upgrade */
    (void) hook_signal_send ("upgrade_ended", WEECHAT_HOOK_SIGNAL_STRING, NULL);
}
