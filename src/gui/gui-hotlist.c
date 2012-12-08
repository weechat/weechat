/*
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

/*
 * gui-hotlist.c: hotlist management (list of buffers with activity)
 *                (used by all GUI)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hashtable.h"
#include "../core/wee-hdata.h"
#include "../core/wee-hook.h"
#include "../core/wee-infolist.h"
#include "../core/wee-log.h"
#include "../core/wee-util.h"
#include "../plugins/plugin.h"
#include "gui-hotlist.h"
#include "gui-buffer.h"
#include "gui-color.h"
#include "gui-window.h"


struct t_gui_hotlist *gui_hotlist = NULL;
struct t_gui_hotlist *last_gui_hotlist = NULL;
struct t_gui_buffer *gui_hotlist_initial_buffer = NULL;

int gui_add_hotlist = 1;                    /* 0 is for temporarly disable  */
                                            /* hotlist add for all buffers  */


/*
 * gui_hotlist_changed_signal: send signal "hotlist_changed"
 */

void
gui_hotlist_changed_signal ()
{
    hook_signal_send ("hotlist_changed", WEECHAT_HOOK_SIGNAL_STRING, NULL);
}

/*
 * gui_hotlist_search: find hotlist with buffer pointer
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
 * gui_hotlist_free: free a hotlist and remove it from hotlist queue
 */

void
gui_hotlist_free (struct t_gui_hotlist **hotlist,
                  struct t_gui_hotlist **last_hotlist,
                  struct t_gui_hotlist *ptr_hotlist)
{
    struct t_gui_hotlist *new_hotlist;

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
 * gui_hotlist_free_all: free all hotlists
 */

void
gui_hotlist_free_all (struct t_gui_hotlist **hotlist,
                      struct t_gui_hotlist **last_hotlist)
{
    /* remove all hotlists */
    while (*hotlist)
    {
        gui_hotlist_free (hotlist, last_hotlist, *hotlist);
    }
}

/*
 * gui_hotlist_check_buffer_notify: return: 1 if buffer notify is ok according
 *                                            to priority (buffer will be added
 *                                            to hotlist)
 *                                          0 if buffer will not be added to
 *                                            hotlist
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
 * gui_hotlist_find_pos: find position for a inserting in hotlist
 *                       (for sorting hotlist)
 */

struct t_gui_hotlist *
gui_hotlist_find_pos (struct t_gui_hotlist *hotlist,
                      struct t_gui_hotlist *new_hotlist)
{
    struct t_gui_hotlist *ptr_hotlist;

    switch (CONFIG_INTEGER(config_look_hotlist_sort))
    {
        case CONFIG_LOOK_HOTLIST_SORT_GROUP_TIME_ASC:
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if ((new_hotlist->priority > ptr_hotlist->priority)
                    || ((new_hotlist->priority == ptr_hotlist->priority)
                        && (util_timeval_diff (&(new_hotlist->creation_time),
                                               &(ptr_hotlist->creation_time)) > 0)))
                    return ptr_hotlist;
            }
            break;
        case CONFIG_LOOK_HOTLIST_SORT_GROUP_TIME_DESC:
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if ((new_hotlist->priority > ptr_hotlist->priority)
                    || ((new_hotlist->priority == ptr_hotlist->priority)
                        && (util_timeval_diff (&(new_hotlist->creation_time),
                                                   &(ptr_hotlist->creation_time)) < 0)))
                    return ptr_hotlist;
            }
            break;
        case CONFIG_LOOK_HOTLIST_SORT_GROUP_NUMBER_ASC:
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if ((new_hotlist->priority > ptr_hotlist->priority)
                    || ((new_hotlist->priority == ptr_hotlist->priority)
                        && (new_hotlist->buffer->number < ptr_hotlist->buffer->number)))
                    return ptr_hotlist;
            }
            break;
        case CONFIG_LOOK_HOTLIST_SORT_GROUP_NUMBER_DESC:
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if ((new_hotlist->priority > ptr_hotlist->priority)
                    || ((new_hotlist->priority == ptr_hotlist->priority)
                        && (new_hotlist->buffer->number > ptr_hotlist->buffer->number)))
                    return ptr_hotlist;
            }
            break;
        case CONFIG_LOOK_HOTLIST_SORT_NUMBER_ASC:
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if (new_hotlist->buffer->number < ptr_hotlist->buffer->number)
                    return ptr_hotlist;
            }
            break;
        case CONFIG_LOOK_HOTLIST_SORT_NUMBER_DESC:
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if (new_hotlist->buffer->number > ptr_hotlist->buffer->number)
                    return ptr_hotlist;
            }
            break;
    }
    return NULL;
}

/*
 * gui_hotlist_add_hotlist: add new hotlist in list
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
 * gui_hotlist_add: add a buffer to hotlist, with priority
 *                  if creation_time is NULL, current time is used
 *                  return pointer to hotlist created or changed, or NULL if no
 *                  hotlist was created/changed
 */

struct t_gui_hotlist *
gui_hotlist_add (struct t_gui_buffer *buffer,
                 enum t_gui_hotlist_priority priority,
                 struct timeval *creation_time)
{
    struct t_gui_hotlist *new_hotlist, *ptr_hotlist;
    int i, count[GUI_HOTLIST_NUM_PRIORITIES];
    const char *away;

    if (!buffer || !gui_add_hotlist)
        return NULL;

    /* do not add core buffer if upgrading */
    if (weechat_upgrading && (buffer == gui_buffer_search_main ()))
        return NULL;

    /* do not add buffer if it is displayed and away is not set */
    away = hashtable_get (buffer->local_variables, "away");
    if ((buffer->num_displayed > 0)
        && ((!away || !away[0])
            || !CONFIG_BOOLEAN(config_look_hotlist_add_buffer_if_away)))
        return NULL;

    if (priority > GUI_HOTLIST_MAX)
        priority = GUI_HOTLIST_MAX;

    /* check if priority is ok according to buffer notify level value */
    if (!gui_hotlist_check_buffer_notify (buffer, priority))
        return NULL;

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
            gui_hotlist_changed_signal ();
            return ptr_hotlist;
        }

        /*
         * if buffer is present with lower priority: save counts, remove it
         * and go on
         */
        memcpy (count, ptr_hotlist->count, sizeof (ptr_hotlist->count));
        gui_hotlist_free (&gui_hotlist, &last_gui_hotlist, ptr_hotlist);
    }

    new_hotlist = malloc (sizeof (*new_hotlist));
    if (!new_hotlist)
    {
        log_printf (_("Error: not enough memory to add a buffer to "
                      "hotlist"));
        return NULL;
    }

    new_hotlist->priority = priority;
    if (creation_time)
    {
        memcpy (&(new_hotlist->creation_time),
                creation_time, sizeof (*creation_time));
    }
    else
        gettimeofday (&(new_hotlist->creation_time), NULL);
    new_hotlist->buffer = buffer;
    memcpy (new_hotlist->count, count, sizeof (new_hotlist->count));
    new_hotlist->count[priority]++;
    new_hotlist->next_hotlist = NULL;
    new_hotlist->prev_hotlist = NULL;

    gui_hotlist_add_hotlist (&gui_hotlist, &last_gui_hotlist, new_hotlist);

    gui_hotlist_changed_signal ();

    return new_hotlist;
}

/*
 * gui_hotlist_dup: duplicate hotlist element
 */

struct t_gui_hotlist *
gui_hotlist_dup (struct t_gui_hotlist *hotlist)
{
    struct t_gui_hotlist *new_hotlist;

    new_hotlist = malloc (sizeof (*new_hotlist));
    if (new_hotlist)
    {
        new_hotlist->priority = hotlist->priority;
        memcpy (&(new_hotlist->creation_time), &(hotlist->creation_time),
                sizeof (new_hotlist->creation_time));
        new_hotlist->buffer = hotlist->buffer;
        memcpy (new_hotlist->count, hotlist->count, sizeof (hotlist->count));
        new_hotlist->prev_hotlist = NULL;
        new_hotlist->next_hotlist = NULL;
        return new_hotlist;
    }
    return NULL;
}

/*
 * gui_hotlist_resort: resort hotlist with new sort type
 */

void
gui_hotlist_resort ()
{
    struct t_gui_hotlist *new_hotlist, *last_new_hotlist;
    struct t_gui_hotlist *ptr_hotlist, *element;

    /* copy and resort hotlist in new linked list */
    new_hotlist = NULL;
    last_new_hotlist = NULL;
    for (ptr_hotlist = gui_hotlist; ptr_hotlist;
         ptr_hotlist = ptr_hotlist->next_hotlist)
    {
        element = gui_hotlist_dup (ptr_hotlist);
        gui_hotlist_add_hotlist (&new_hotlist, &last_new_hotlist, element);
    }

    gui_hotlist_free_all (&gui_hotlist, &last_gui_hotlist);

    gui_hotlist = new_hotlist;
    last_gui_hotlist = last_new_hotlist;

    gui_hotlist_changed_signal ();
}

/*
 * gui_hotlist_clear: clear hotlist
 */

void
gui_hotlist_clear ()
{
    gui_hotlist_free_all (&gui_hotlist, &last_gui_hotlist);
    gui_hotlist_changed_signal ();
}

/*
 * gui_hotlist_remove_buffer: remove a buffer from hotlist
 */

void
gui_hotlist_remove_buffer (struct t_gui_buffer *buffer)
{
    int hotlist_changed;
    struct t_gui_hotlist *ptr_hotlist, *next_hotlist;

    if (weechat_upgrading)
        return;

    hotlist_changed = 0;

    ptr_hotlist = gui_hotlist;
    while (ptr_hotlist)
    {
        next_hotlist = ptr_hotlist->next_hotlist;

        if (ptr_hotlist->buffer->number == buffer->number)
        {
            gui_hotlist_free (&gui_hotlist, &last_gui_hotlist, ptr_hotlist);
            hotlist_changed = 1;
        }

        ptr_hotlist = next_hotlist;
    }

    if (hotlist_changed)
        gui_hotlist_changed_signal ();
}

/*
 * gui_hotlist_hdata_hotlist_cb: return hdata for hotlist
 */

struct t_hdata *
gui_hotlist_hdata_hotlist_cb (void *data, const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_hotlist", "next_hotlist",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_hotlist, priority, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_hotlist, creation_time.tv_sec, TIME, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_hotlist, creation_time.tv_usec, LONG, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_hotlist, buffer, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_hotlist, count, INTEGER, 0, GUI_HOTLIST_NUM_PRIORITIES_STR, NULL);
        HDATA_VAR(struct t_gui_hotlist, prev_hotlist, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_hotlist, next_hotlist, POINTER, 0, NULL, hdata_name);
        HDATA_LIST(gui_hotlist);
        HDATA_LIST(last_gui_hotlist);
    }
    return hdata;
}

/*
 * gui_hotlist_add_to_infolist: add a hotlist in an infolist
 *                              return 1 if ok, 0 if error
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
 * gui_hotlist_print_log: print hotlist in log (usually for crash dump)
 */

void
gui_hotlist_print_log ()
{
    struct t_gui_hotlist *ptr_hotlist;
    int i;

    for (ptr_hotlist = gui_hotlist; ptr_hotlist;
         ptr_hotlist = ptr_hotlist->next_hotlist)
    {
        log_printf ("[hotlist (addr:0x%lx)]", ptr_hotlist);
        log_printf ("  priority . . . . . . . : %d",    ptr_hotlist->priority);
        log_printf ("  creation_time. . . . . : tv_sec:%d, tv_usec:%d",
                    ptr_hotlist->creation_time.tv_sec,
                    ptr_hotlist->creation_time.tv_usec);
        log_printf ("  buffer . . . . . . . . : 0x%lx", ptr_hotlist->buffer);
        for (i = 0; i < GUI_HOTLIST_NUM_PRIORITIES; i++)
        {
            log_printf ("  count[%02d]. . . . . . . : %d", i, ptr_hotlist->count[i]);
        }
        log_printf ("  prev_hotlist . . . . . : 0x%lx", ptr_hotlist->prev_hotlist);
        log_printf ("  next_hotlist . . . . . : 0x%lx", ptr_hotlist->next_hotlist);
    }
}
