/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* wee-upgrade.c: save/restore session data */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "weechat.h"
#include "wee-upgrade.h"
#include "wee-infolist.h"
#include "wee-string.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-history.h"
#include "../gui/gui-hotlist.h"
#include "../gui/gui-nicklist.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"


struct t_gui_buffer *upgrade_current_buffer = NULL;
struct t_gui_buffer *upgrade_set_current_buffer = NULL;
int hotlist_reset = 0;


/*
 * upgrade_weechat_save_history: save history info to upgrade file
 *                               (from last to first, to restore it in good order)
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
 * upgrade_weechat_save_buffers: save buffers info to upgrade file
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
            if (!gui_nicklist_add_to_infolist (ptr_infolist, ptr_buffer))
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
        for (ptr_line = ptr_buffer->lines; ptr_line;
             ptr_line = ptr_line->next_line)
        {
            ptr_infolist = infolist_new ();
            if (!ptr_infolist)
                return 0;
            if (!gui_buffer_line_add_to_infolist (ptr_infolist, ptr_line))
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
 * upgrade_weechat_save_uptime: save uptime info to upgrade file
 */

int
upgrade_weechat_save_uptime (struct t_upgrade_file *upgrade_file)
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
    if (!infolist_new_var_time (ptr_item, "start_time", weechat_start_time))
    {
        infolist_free (ptr_infolist);
        return 0;
    }
    
    rc = upgrade_file_write_object (upgrade_file,
                                    UPGRADE_WEECHAT_TYPE_UPTIME,
                                    ptr_infolist);
    infolist_free (ptr_infolist);
    
    return rc;
}

/*
 * upgrade_weechat_save_hotlist: save hotlist info to upgrade file
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
 * upgrade_weechat_save: save upgrade file
 *                       return 1 if ok, 0 if error
 */

int
upgrade_weechat_save ()
{
    int rc;
    struct t_upgrade_file *upgrade_file;
    
    upgrade_file = upgrade_file_create (WEECHAT_UPGRADE_FILENAME, 1);
    if (!upgrade_file)
        return 0;

    rc = 1;
    rc &= upgrade_weechat_save_history (upgrade_file, last_history_global);
    rc &= upgrade_weechat_save_buffers (upgrade_file);
    rc &= upgrade_weechat_save_uptime (upgrade_file);
    rc &= upgrade_weechat_save_hotlist (upgrade_file);
    
    upgrade_file_close (upgrade_file);
    
    return rc;
}

/*
 * upgrade_weechat_read_cb: callback for reading upgrade file
 */

int
upgrade_weechat_read_cb (int object_id,
                         struct t_infolist *infolist)
{
    char *type, *name, *prefix, *group_name, option_name[32], *key;
    char *option_key;
    struct t_gui_nick_group *ptr_group;
    struct t_gui_buffer *ptr_buffer;
    struct timeval creation_time;
    void *buf;
    int size, key_index, length;
    
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
                /* create buffer if it was created by a plugin (ie not weechat
                   main buffer) */
                if (infolist_string (infolist, "plugin_name"))
                {
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
                        upgrade_current_buffer->nicklist_case_sensitive =
                            infolist_integer (infolist, "nicklist_case_sensitive");
                        upgrade_current_buffer->nicklist_display_groups =
                            infolist_integer (infolist, "nicklist_display_groups");
                        gui_buffer_set_highlight_words (upgrade_current_buffer,
                                                        infolist_string (infolist, "highlight_words"));
                        gui_buffer_set_highlight_tags (upgrade_current_buffer,
                                                       infolist_string (infolist, "highlight_tags"));
                        key_index = 0;
                        while (1)
                        {
                            snprintf (option_name, sizeof (option_name),
                                      "key_%05d", key_index);
                            key = infolist_string (infolist, option_name);
                            if (!key)
                                break;
                            length = 16 + strlen (key) + 1;
                            option_key = malloc (length);
                            if (option_key)
                            {
                                snprintf (option_key, length, "key_bind_%s", key);
                                snprintf (option_name, sizeof (option_name),
                                          "key_command_%05d", key_index);
                                gui_buffer_set (upgrade_current_buffer,
                                                option_key,
                                                infolist_string (infolist, option_name));
                                free (option_key);
                            }
                            key_index++;
                        }
                    }
                }
                else
                    upgrade_current_buffer = gui_buffers;
                break;
            case UPGRADE_WEECHAT_TYPE_BUFFER_LINE:
                /* add line to current buffer */
                if (upgrade_current_buffer)
                {
                    gui_chat_line_add (
                        upgrade_current_buffer,
                        infolist_time (infolist, "date"),
                        infolist_time (infolist, "date_printed"),
                        infolist_string (infolist, "tags"),
                        infolist_string (infolist, "prefix"),
                        infolist_string (infolist, "message"));
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
                                ptr_group = gui_nicklist_add_group (
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
                            prefix = infolist_string (infolist, "prefix");
                            gui_nicklist_add_nick (
                                upgrade_current_buffer,
                                ptr_group,
                                infolist_string (infolist, "name"),
                                infolist_string (infolist, "color"),
                                (prefix) ? prefix[0] : ' ',
                                infolist_string (infolist, "prefix_color"),
                                infolist_integer (infolist, "visible"));
                        }
                    }
                }
                break;
            case UPGRADE_WEECHAT_TYPE_UPTIME:
                weechat_start_time = infolist_time (infolist, "start_time");
                break;
            case UPGRADE_WEECHAT_TYPE_HOTLIST:
                if (!hotlist_reset)
                {
                    gui_hotlist_clear ();
                    hotlist_reset = 1;
                }
                ptr_buffer = gui_buffer_search_by_number (infolist_integer (infolist, "buffer_number"));
                if (ptr_buffer)
                {
                    buf = infolist_buffer (infolist, "creation_time", &size);
                    if (buf)
                    {
                        memcpy (&creation_time, buf, size);
                        gui_hotlist_add (ptr_buffer,
                                         infolist_integer (infolist, "priority"),
                                         &creation_time,
                                         1);
                    }
                }
                break;
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * upgrade_weechat_load: load upgrade file
 *                       return 1 if ok, 0 if error
 */

int
upgrade_weechat_load ()
{
    int rc;
    struct t_upgrade_file *upgrade_file;
    
    upgrade_file = upgrade_file_create (WEECHAT_UPGRADE_FILENAME, 0);
    rc = upgrade_file_read (upgrade_file, &upgrade_weechat_read_cb);

    if (upgrade_set_current_buffer)
        gui_window_switch_to_buffer (gui_current_window,
                                     upgrade_set_current_buffer);
    
    return rc;
}
