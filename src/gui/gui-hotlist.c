/*
 * gui-hotlist.c - hotlist management (used by all GUI)
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stddef.h>
#include <string.h>

#include "../core/weechat.h"
#include "../core/core-config.h"
#include "../core/core-eval.h"
#include "../core/core-hashtable.h"
#include "../core/core-hdata.h"
#include "../core/core-hook.h"
#include "../core/core-infolist.h"
#include "../core/core-log.h"
#include "../core/core-string.h"
#include "../core/core-util.h"
#include "../plugins/plugin.h"
#include "gui-hotlist.h"
#include "gui-buffer.h"
#include "gui-color.h"
#include "gui-window.h"


struct t_gui_hotlist *gui_hotlist = NULL;
struct t_gui_hotlist *last_gui_hotlist = NULL;
struct t_gui_buffer *gui_hotlist_initial_buffer = NULL;
struct t_hashtable *gui_hotlist_hashtable_add_conditions_pointers = NULL;
struct t_hashtable *gui_hotlist_hashtable_add_conditions_vars = NULL;
struct t_hashtable *gui_hotlist_hashtable_add_conditions_options = NULL;

int gui_add_hotlist = 1;                    /* 0 is for temporarily disable */
                                            /* hotlist add for all buffers  */

char *gui_hotlist_priority_string[GUI_HOTLIST_NUM_PRIORITIES] =
{ "low", "message", "private", "highlight" };


/*
 * Sends signal "hotlist_changed".
 */

void
gui_hotlist_changed_signal (struct t_gui_buffer *buffer)
{
    (void) hook_signal_send ("hotlist_changed",
                             WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * Searches for hotlist priority.
 *
 * Returns index of hotlist priority found, -1 if not found.
 */

int
gui_hotlist_search_priority (const char *priority)
{
    int i;

    if (!priority)
        return -1;

    for (i = 0; i < GUI_HOTLIST_NUM_PRIORITIES; i++)
    {
        if (strcmp (gui_hotlist_priority_string[i], priority) == 0)
            return i;
    }

    return -1;
}

/*
 * Searches for hotlist with buffer pointer.
 *
 * Returns pointer to hotlist found, NULL if not found.
 */

struct t_gui_hotlist *
gui_hotlist_search (struct t_gui_hotlist *hotlist, struct t_gui_buffer *buffer)
{
    struct t_gui_hotlist *ptr_hotlist;

    for (ptr_hotlist = hotlist; ptr_hotlist;
         ptr_hotlist = ptr_hotlist->next_hotlist)
    {
        if (ptr_hotlist->buffer == buffer)
            return ptr_hotlist;
    }
    return NULL;
}

/*
 * Duplicates a hotlist element.
 *
 * Returns pointer to new hotlist, NULL if error.
 */

struct t_gui_hotlist *
gui_hotlist_dup (struct t_gui_hotlist *hotlist)
{
    struct t_gui_hotlist *new_hotlist;

    new_hotlist = malloc (sizeof (*new_hotlist));
    if (!new_hotlist)
        return NULL;

    new_hotlist->priority = hotlist->priority;
    memcpy (&(new_hotlist->creation_time), &(hotlist->creation_time),
            sizeof (new_hotlist->creation_time));
    new_hotlist->buffer = hotlist->buffer;
    memcpy (new_hotlist->count, hotlist->count, sizeof (hotlist->count));
    new_hotlist->prev_hotlist = NULL;
    new_hotlist->next_hotlist = NULL;

    return new_hotlist;
}

/*
 * Frees a hotlist and removes it from hotlist queue.
 */

void
gui_hotlist_free (struct t_gui_hotlist **hotlist,
                  struct t_gui_hotlist **last_hotlist,
                  struct t_gui_hotlist *ptr_hotlist,
                  int save_removed_hotlist)
{
    struct t_gui_hotlist *new_hotlist;

    if (!ptr_hotlist)
        return;

    if (save_removed_hotlist)
    {
        free (ptr_hotlist->buffer->hotlist_removed);
        ptr_hotlist->buffer->hotlist_removed = gui_hotlist_dup (ptr_hotlist);
        ptr_hotlist->buffer->hotlist_removed->prev_hotlist = NULL;
        ptr_hotlist->buffer->hotlist_removed->next_hotlist = NULL;
    }

    ptr_hotlist->buffer->hotlist = NULL;

    /* remove hotlist from queue */
    if (*last_hotlist == ptr_hotlist)
        *last_hotlist = ptr_hotlist->prev_hotlist;
    if (ptr_hotlist->prev_hotlist)
    {
        (ptr_hotlist->prev_hotlist)->next_hotlist = ptr_hotlist->next_hotlist;
        new_hotlist = *hotlist;
    }
    else
        new_hotlist = ptr_hotlist->next_hotlist;

    if (ptr_hotlist->next_hotlist)
        (ptr_hotlist->next_hotlist)->prev_hotlist = ptr_hotlist->prev_hotlist;

    free (ptr_hotlist);
    *hotlist = new_hotlist;
}

/*
 * Frees all hotlists.
 */

void
gui_hotlist_free_all (struct t_gui_hotlist **hotlist,
                      struct t_gui_hotlist **last_hotlist)
{
    /* remove all hotlists */
    while (*hotlist)
    {
        gui_hotlist_free (hotlist, last_hotlist, *hotlist, 0);
    }
}

/*
 * Checks if a buffer must be added to hotlist, according to its notify level.
 *
 * Returns:
 *   1: buffer must be added to hotlist
 *   0: buffer must not be added to hotlist
 */

int
gui_hotlist_check_buffer_notify (struct t_gui_buffer *buffer,
                                 enum t_gui_hotlist_priority priority)
{
    switch (priority)
    {
        case GUI_HOTLIST_LOW:
            return (buffer->notify >= 3);
        case GUI_HOTLIST_MESSAGE:
            return (buffer->notify >= 2);
        case GUI_HOTLIST_PRIVATE:
        case GUI_HOTLIST_HIGHLIGHT:
            return (buffer->notify >= 1);
        case GUI_HOTLIST_NUM_PRIORITIES:
            /*
             * this constant is used to count hotlist priorities only,
             * it is never used as priority
             */
            break;
    }
    return 1;
}

/*
 * Compares two hotlists in order to add them in the sorted list.
 *
 * The comparison is made using the list of fields defined in the option
 * "weechat.look.hotlist_sort".
 *
 * Returns:
 *   -1: hotlist1 < hotlist2
 *    0: hotlist1 == hotlist2
 *    1: hotlist1 > hotlist2
 */

int
gui_hotlist_compare_hotlists (struct t_hdata *hdata_hotlist,
                              struct t_gui_hotlist *hotlist1,
                              struct t_gui_hotlist *hotlist2)
{
    int i, reverse, case_sensitive, rc;
    const char *ptr_field;

    for (i = 0; i < config_num_hotlist_sort_fields; i++)
    {
        rc = 0;
        reverse = 1;
        case_sensitive = 1;
        ptr_field = config_hotlist_sort_fields[i];
        while ((ptr_field[0] == '-') || (ptr_field[0] == '~'))
        {
            if (ptr_field[0] == '-')
                reverse *= -1;
            else if (ptr_field[0] == '~')
                case_sensitive ^= 1;
            ptr_field++;
        }
        if (!hotlist1 && !hotlist2)
            rc = 0;
        else if (hotlist1 && !hotlist2)
            rc = 1;
        else if (!hotlist1 && hotlist2)
            rc = -1;
        else
        {
            rc = hdata_compare (hdata_hotlist,
                                hotlist1, hotlist2,
                                ptr_field,
                                case_sensitive);
        }
        rc *= reverse;
        if (rc != 0)
            return rc;
    }

    return 0;
}

/*
 * Searches for position of hotlist (to keep hotlist sorted).
 */

struct t_gui_hotlist *
gui_hotlist_find_pos (struct t_gui_hotlist *hotlist,
                      struct t_gui_hotlist *new_hotlist)
{
    struct t_hdata *hdata_hotlist;
    struct t_gui_hotlist *ptr_hotlist;

    hdata_hotlist = hook_hdata_get (NULL, "hotlist");
    for (ptr_hotlist = hotlist; ptr_hotlist;
         ptr_hotlist = ptr_hotlist->next_hotlist)
    {
        if (gui_hotlist_compare_hotlists (hdata_hotlist,
                                          new_hotlist, ptr_hotlist) < 0)
        {
            return ptr_hotlist;
        }
    }

    return NULL;
}

/*
 * Adds new hotlist in list.
 */

void
gui_hotlist_add_hotlist (struct t_gui_hotlist **hotlist,
                         struct t_gui_hotlist **last_hotlist,
                         struct t_gui_hotlist *new_hotlist)
{
    struct t_gui_hotlist *pos_hotlist;

    if (*hotlist)
    {
        pos_hotlist = gui_hotlist_find_pos (*hotlist, new_hotlist);

        if (pos_hotlist)
        {
            /* insert hotlist into the hotlist (before hotlist found) */
            new_hotlist->prev_hotlist = pos_hotlist->prev_hotlist;
            new_hotlist->next_hotlist = pos_hotlist;
            if (pos_hotlist->prev_hotlist)
                (pos_hotlist->prev_hotlist)->next_hotlist = new_hotlist;
            else
                *hotlist = new_hotlist;
            pos_hotlist->prev_hotlist = new_hotlist;
        }
        else
        {
            /* add hotlist to the end */
            new_hotlist->prev_hotlist = *last_hotlist;
            new_hotlist->next_hotlist = NULL;
            (*last_hotlist)->next_hotlist = new_hotlist;
            *last_hotlist = new_hotlist;
        }
    }
    else
    {
        new_hotlist->prev_hotlist = NULL;
        new_hotlist->next_hotlist = NULL;
        *hotlist = new_hotlist;
        *last_hotlist = new_hotlist;
    }
}

/*
 * Adds a buffer to hotlist, with priority.
 *
 * If creation_time is NULL, current time is used.
 *
 * Returns pointer to hotlist created or changed, NULL if no hotlist was
 * created/changed.
 */

struct t_gui_hotlist *
gui_hotlist_add (struct t_gui_buffer *buffer,
                 enum t_gui_hotlist_priority priority,
                 struct timeval *creation_time,
                 int check_conditions)
{
    struct t_gui_hotlist *new_hotlist, *ptr_hotlist;
    int i, count[GUI_HOTLIST_NUM_PRIORITIES], rc;
    char *value, str_value[32];

    if (!buffer || !gui_add_hotlist)
        return NULL;

    /* do not add core buffer if upgrading */
    if (weechat_upgrading && (buffer == gui_buffer_search_main ()))
        return NULL;

    if (priority > GUI_HOTLIST_MAX)
        priority = GUI_HOTLIST_MAX;

    /* check if priority is OK according to buffer notify level value */
    if (!gui_hotlist_check_buffer_notify (buffer, priority))
        return NULL;

    if (check_conditions)
    {
        /* create hashtable if needed (to evaluate conditions) */
        if (!gui_hotlist_hashtable_add_conditions_pointers)
        {
            gui_hotlist_hashtable_add_conditions_pointers = hashtable_new (
                32,
                WEECHAT_HASHTABLE_STRING,
                WEECHAT_HASHTABLE_POINTER,
                NULL, NULL);
            if (!gui_hotlist_hashtable_add_conditions_pointers)
                return NULL;
        }
        if (!gui_hotlist_hashtable_add_conditions_vars)
        {
            gui_hotlist_hashtable_add_conditions_vars = hashtable_new (
                32,
                WEECHAT_HASHTABLE_STRING,
                WEECHAT_HASHTABLE_STRING,
                NULL, NULL);
            if (!gui_hotlist_hashtable_add_conditions_vars)
                return NULL;
        }
        if (!gui_hotlist_hashtable_add_conditions_options)
        {
            gui_hotlist_hashtable_add_conditions_options = hashtable_new (
                32,
                WEECHAT_HASHTABLE_STRING,
                WEECHAT_HASHTABLE_STRING,
                NULL, NULL);
            if (!gui_hotlist_hashtable_add_conditions_options)
                return NULL;
            hashtable_set (gui_hotlist_hashtable_add_conditions_options,
                           "type", "condition");
        }

        /* set data in hashtables */
        hashtable_set (gui_hotlist_hashtable_add_conditions_pointers,
                       "window", gui_current_window);
        hashtable_set (gui_hotlist_hashtable_add_conditions_pointers,
                       "buffer", buffer);
        snprintf (str_value, sizeof (str_value), "%d", priority);
        hashtable_set (gui_hotlist_hashtable_add_conditions_vars,
                       "priority", str_value);

        /* check if conditions are true */
        value = eval_expression (
            CONFIG_STRING(config_look_hotlist_add_conditions),
            gui_hotlist_hashtable_add_conditions_pointers,
            gui_hotlist_hashtable_add_conditions_vars,
            gui_hotlist_hashtable_add_conditions_options);
        rc = (value && (strcmp (value, "1") == 0));
        free (value);
        if (!rc)
            return NULL;
    }

    /* init count */
    for (i = 0; i < GUI_HOTLIST_NUM_PRIORITIES; i++)
    {
        count[i] = 0;
    }

    ptr_hotlist = gui_hotlist_search (gui_hotlist, buffer);
    if (ptr_hotlist)
    {
        /* return if priority is greater or equal than the one to add */
        if (ptr_hotlist->priority >= priority)
        {
            ptr_hotlist->count[priority]++;
            gui_hotlist_changed_signal (buffer);
            return ptr_hotlist;
        }

        /*
         * if buffer is present with lower priority: save counts, remove it
         * and go on
         */
        memcpy (count, ptr_hotlist->count, sizeof (ptr_hotlist->count));
        gui_hotlist_free (&gui_hotlist, &last_gui_hotlist, ptr_hotlist, 1);
    }

    new_hotlist = malloc (sizeof (*new_hotlist));
    if (!new_hotlist)
        return NULL;

    new_hotlist->priority = priority;
    if (creation_time)
    {
        memcpy (&(new_hotlist->creation_time),
                creation_time, sizeof (*creation_time));
    }
    else
        gettimeofday (&(new_hotlist->creation_time), NULL);
    new_hotlist->buffer = buffer;
    buffer->hotlist = new_hotlist;
    memcpy (new_hotlist->count, count, sizeof (new_hotlist->count));
    new_hotlist->count[priority]++;
    new_hotlist->next_hotlist = NULL;
    new_hotlist->prev_hotlist = NULL;

    gui_hotlist_add_hotlist (&gui_hotlist, &last_gui_hotlist, new_hotlist);

    gui_hotlist_changed_signal (buffer);

    return new_hotlist;
}

/*
 * Restores a hotlist that was removed from a buffer.
 */

void
gui_hotlist_restore_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_hotlist *ptr_hotlist;

    if (!buffer->hotlist_removed)
        return;

    /* remove hotlist with buffer from list (if found) */
    ptr_hotlist = gui_hotlist_search (gui_hotlist, buffer);
    if (ptr_hotlist)
        gui_hotlist_free (&gui_hotlist, &last_gui_hotlist, ptr_hotlist, 0);

    /* restore the removed hotlist */
    buffer->hotlist_removed->buffer = buffer;
    ptr_hotlist = gui_hotlist_dup (buffer->hotlist_removed);
    gui_hotlist_add_hotlist (&gui_hotlist, &last_gui_hotlist, ptr_hotlist);

    free (buffer->hotlist_removed);
    buffer->hotlist_removed = NULL;

    gui_hotlist_changed_signal (buffer);
}

/*
 * Restores latest hotlist removed in all buffers.
 */

void
gui_hotlist_restore_all_buffers (void)
{
    struct t_gui_buffer *ptr_buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_hotlist_restore_buffer (ptr_buffer);
    }
}

/*
 * Resorts hotlist.
 */

void
gui_hotlist_resort (void)
{
    struct t_gui_hotlist *new_hotlist, *last_new_hotlist;
    struct t_gui_hotlist *ptr_hotlist, *ptr_next_hotlist, *ptr_prev_hotlist;
    int hotlist_changed;

    /* sort is not needed if hotlist has less than 2 entries */
    if (!gui_hotlist || !gui_hotlist->next_hotlist)
        return;

    /* resort hotlist in new linked list */
    hotlist_changed = 0;
    new_hotlist = NULL;
    last_new_hotlist = NULL;
    ptr_hotlist = gui_hotlist;
    while (ptr_hotlist)
    {
        ptr_next_hotlist = ptr_hotlist->next_hotlist;
        ptr_prev_hotlist = ptr_hotlist->prev_hotlist;
        gui_hotlist_add_hotlist (&new_hotlist, &last_new_hotlist, ptr_hotlist);
        if (ptr_hotlist->prev_hotlist != ptr_prev_hotlist)
            hotlist_changed = 1;
        ptr_hotlist = ptr_next_hotlist;
    }

    /* switch to new sorted hotlist */
    gui_hotlist = new_hotlist;
    last_gui_hotlist = last_new_hotlist;

    if (hotlist_changed)
        gui_hotlist_changed_signal (NULL);
}

/*
 * Clears hotlist with a level mask (integer).
 *
 * Argument "level_mask" is a combination of:
 *   1 = join/part
 *   2 = message
 *   4 = private
 *   8 = highlight
 *
 * So for example :
 *   1 = clear only join/part
 *   12 = clear only private and highlight
 *   15 = clear whole hotlist
 */

void
gui_hotlist_clear (int level_mask)
{
    struct t_gui_hotlist *ptr_hotlist, *ptr_next_hotlist;
    int hotlist_changed;

    hotlist_changed = 0;

    ptr_hotlist = gui_hotlist;
    while (ptr_hotlist)
    {
        ptr_next_hotlist = ptr_hotlist->next_hotlist;
        if (level_mask & (1 << ptr_hotlist->priority))
        {
            gui_hotlist_free (&gui_hotlist, &last_gui_hotlist, ptr_hotlist, 1);
            hotlist_changed = 1;
        }
        ptr_hotlist = ptr_next_hotlist;
    }

    if (hotlist_changed)
        gui_hotlist_changed_signal (NULL);
}

/*
 * Clears hotlist with a level mask (string).
 */

void
gui_hotlist_clear_level_string (struct t_gui_buffer *buffer,
                                const char *str_level_mask)
{
    long level_mask;
    char *error;
    struct t_gui_hotlist *ptr_hotlist;
    int priority;

    if (str_level_mask)
    {
        if (strcmp (str_level_mask, "lowest") == 0)
        {
            /* clear only lowest priority currently in hotlist */
            priority = GUI_HOTLIST_MAX + 1;
            for (ptr_hotlist = gui_hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if ((int)ptr_hotlist->priority < priority)
                    priority = ptr_hotlist->priority;
            }
            if (priority <= GUI_HOTLIST_MAX)
            {
                gui_hotlist_clear (1 << priority);
                gui_hotlist_initial_buffer = buffer;
            }
        }
        else if (strcmp (str_level_mask, "highest") == 0)
        {
            /* clear only highest priority currently in hotlist */
            priority = GUI_HOTLIST_MIN - 1;
            for (ptr_hotlist = gui_hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if ((int)ptr_hotlist->priority > priority)
                    priority = ptr_hotlist->priority;
            }
            if (priority >= GUI_HOTLIST_MIN)
            {
                gui_hotlist_clear (1 << priority);
                gui_hotlist_initial_buffer = buffer;
            }
        }
        else
        {
            /* clear hotlist using a mask of levels */
            error = NULL;
            level_mask = strtol (str_level_mask, &error, 10);
            if (error && !error[0] && (level_mask > 0))
            {
                gui_hotlist_clear ((int)level_mask);
                gui_hotlist_initial_buffer = buffer;
            }
        }
    }
    else
    {
        gui_hotlist_clear (GUI_HOTLIST_MASK_MAX);
        gui_hotlist_initial_buffer = buffer;
    }
}

/*
 * Removes a buffer from hotlist.
 */

void
gui_hotlist_remove_buffer (struct t_gui_buffer *buffer,
                           int force_remove_buffer)
{
    int hotlist_changed, hotlist_remove, buffer_to_remove;
    struct t_gui_hotlist *ptr_hotlist, *next_hotlist;

    if (!buffer)
        return;

    hotlist_changed = 0;

    hotlist_remove = CONFIG_ENUM(config_look_hotlist_remove);

    ptr_hotlist = gui_hotlist;
    while (ptr_hotlist)
    {
        next_hotlist = ptr_hotlist->next_hotlist;

        buffer_to_remove = (force_remove_buffer) ?
            (ptr_hotlist->buffer == buffer) : 0;

        switch (hotlist_remove)
        {
            case CONFIG_LOOK_HOTLIST_REMOVE_BUFFER:
                buffer_to_remove |= (ptr_hotlist->buffer == buffer);
                break;
            case CONFIG_LOOK_HOTLIST_REMOVE_MERGED:
                buffer_to_remove |=
                    ((ptr_hotlist->buffer->number == buffer->number)
                     && (!ptr_hotlist->buffer->zoomed
                         || (ptr_hotlist->buffer->active == 2)));
                break;
        }

        if (buffer_to_remove)
        {
            gui_hotlist_free (&gui_hotlist, &last_gui_hotlist, ptr_hotlist, 1);
            hotlist_changed = 1;
        }

        ptr_hotlist = next_hotlist;
    }

    if (hotlist_changed)
        gui_hotlist_changed_signal (NULL);
}

/*
 * Returns hdata for hotlist.
 */

struct t_hdata *
gui_hotlist_hdata_hotlist_cb (const void *pointer, void *data,
                              const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_hotlist", "next_hotlist",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_hotlist, priority, INTEGER, 0, NULL, NULL);
        HDATA_VAR_NAME(struct t_gui_hotlist, creation_time.tv_sec, "time", TIME, 0, NULL, NULL);
        HDATA_VAR_NAME(struct t_gui_hotlist, creation_time.tv_usec, "time_usec", LONG, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_hotlist, buffer, POINTER, 0, NULL, "buffer");
        HDATA_VAR(struct t_gui_hotlist, count, INTEGER, 0, GUI_HOTLIST_NUM_PRIORITIES_STR, NULL);
        HDATA_VAR(struct t_gui_hotlist, prev_hotlist, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_hotlist, next_hotlist, POINTER, 0, NULL, hdata_name);
        HDATA_LIST(gui_hotlist, WEECHAT_HDATA_LIST_CHECK_POINTERS);
        HDATA_LIST(last_gui_hotlist, 0);
    }
    return hdata;
}

/*
 * Adds a hotlist in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_hotlist_add_to_infolist (struct t_infolist *infolist,
                             struct t_gui_hotlist *hotlist)
{
    struct t_infolist_item *ptr_item;
    int i;
    char option_name[64];

    if (!infolist || !hotlist)
        return 0;

    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_integer (ptr_item, "priority", hotlist->priority))
        return 0;
    switch (hotlist->priority)
    {
        case GUI_HOTLIST_LOW:
            if (!infolist_new_var_string (ptr_item, "color",
                                          gui_color_get_name (CONFIG_COLOR(config_color_status_data_other))))
                return 0;
            break;
        case GUI_HOTLIST_MESSAGE:
            if (!infolist_new_var_string (ptr_item, "color",
                                          gui_color_get_name (CONFIG_COLOR(config_color_status_data_msg))))
                return 0;
            break;
        case GUI_HOTLIST_PRIVATE:
            if (!infolist_new_var_string (ptr_item, "color",
                                          gui_color_get_name (CONFIG_COLOR(config_color_status_data_private))))
                return 0;
            break;
        case GUI_HOTLIST_HIGHLIGHT:
            if (!infolist_new_var_string (ptr_item, "color",
                                          gui_color_get_name (CONFIG_COLOR(config_color_status_data_highlight))))
                return 0;
            break;
        case GUI_HOTLIST_NUM_PRIORITIES:
            /*
             * this constant is used to count hotlist priorities only,
             * it is never used as priority
             */
            break;
    }
    if (!infolist_new_var_buffer (ptr_item, "creation_time", &(hotlist->creation_time), sizeof (struct timeval)))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "buffer_pointer", hotlist->buffer))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "buffer_number", hotlist->buffer->number))
        return 0;
    if (!infolist_new_var_string (ptr_item, "plugin_name", gui_buffer_get_plugin_name (hotlist->buffer)))
        return 0;
    if (!infolist_new_var_string (ptr_item, "buffer_name", hotlist->buffer->name))
        return 0;
    for (i = 0; i < GUI_HOTLIST_NUM_PRIORITIES; i++)
    {
        snprintf (option_name, sizeof (option_name), "count_%02d", i);
        if (!infolist_new_var_integer (ptr_item, option_name, hotlist->count[i]))
            return 0;
    }

    return 1;
}

/*
 * Prints hotlist in WeeChat log file (usually for crash dump).
 */

void
gui_hotlist_print_log (void)
{
    struct t_gui_hotlist *ptr_hotlist;
    int i;

    for (ptr_hotlist = gui_hotlist; ptr_hotlist;
         ptr_hotlist = ptr_hotlist->next_hotlist)
    {
        log_printf ("[hotlist (addr:%p)]", ptr_hotlist);
        log_printf ("  priority . . . . . . . : %d", ptr_hotlist->priority);
        log_printf ("  creation_time. . . . . : tv_sec:%lld, tv_usec:%ld",
                    (long long)(ptr_hotlist->creation_time.tv_sec),
                    (long)(ptr_hotlist->creation_time.tv_usec));
        log_printf ("  buffer . . . . . . . . : %p", ptr_hotlist->buffer);
        for (i = 0; i < GUI_HOTLIST_NUM_PRIORITIES; i++)
        {
            log_printf ("  count[%02d]. . . . . . . : %d", i, ptr_hotlist->count[i]);
        }
        log_printf ("  prev_hotlist . . . . . : %p", ptr_hotlist->prev_hotlist);
        log_printf ("  next_hotlist . . . . . : %p", ptr_hotlist->next_hotlist);
    }
}

/*
 * Ends hotlist.
 */

void
gui_hotlist_end (void)
{
    if (gui_hotlist_hashtable_add_conditions_pointers)
    {
        hashtable_free (gui_hotlist_hashtable_add_conditions_pointers);
        gui_hotlist_hashtable_add_conditions_pointers = NULL;
    }
    if (gui_hotlist_hashtable_add_conditions_vars)
    {
        hashtable_free (gui_hotlist_hashtable_add_conditions_vars);
        gui_hotlist_hashtable_add_conditions_vars = NULL;
    }
    if (gui_hotlist_hashtable_add_conditions_options)
    {
        hashtable_free (gui_hotlist_hashtable_add_conditions_options);
        gui_hotlist_hashtable_add_conditions_options = NULL;
    }
}
