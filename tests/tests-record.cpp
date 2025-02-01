/*
 * tests-record.cpp - record and search in messages displayed
 *
 * Copyright (C) 2023-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <unistd.h>
#include <string.h>

#include "tests-record.h"

extern "C"
{
#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#endif
#include "src/core/weechat.h"
#include "src/core/core-arraylist.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/core/core-string.h"
#include "src/gui/gui-color.h"
}

/* recording of messages: to test if a message is actually displayed */
int record_messages = 0;
struct t_arraylist *recorded_messages = NULL;
struct t_hook *record_hook_line = NULL;


/*
 * Callback used to compare two recorded messages.
 */

int
record_cmp_cb (void *data, struct t_arraylist *arraylist,
               void *pointer1, void *pointer2)
{
    /* make C++ compiler happy */
    (void) data;
    (void) arraylist;

    return (pointer1 < pointer2) ?
        -1 : ((pointer1 > pointer2) ? 1 : 0);
}

/*
 * Callback used to free a recorded message.
 */

void
record_free_cb (void *data, struct t_arraylist *arraylist, void *pointer)
{
    /* make C++ compiler happy */
    (void) data;
    (void) arraylist;

    if (pointer)
        hashtable_free ((struct t_hashtable *)pointer);
}

/*
 * Callback for hook line, used when message are recorded.
 */

struct t_hashtable *
record_hook_line_cb (const void *pointer, void *data, struct t_hashtable *line)
{
    struct t_hashtable *hashtable;
    const char *ptr_string;

    /* make C++ compiler happy */
    (void) pointer;
    (void) data;

    hashtable = hashtable_dup (line);

    ptr_string = (const char *)hashtable_get (hashtable, "prefix");
    hashtable_set (
        hashtable,
        "prefix_no_color",
        (ptr_string) ? gui_color_decode (ptr_string, NULL) : NULL);

    ptr_string = (const char *)hashtable_get (hashtable, "message");
    hashtable_set (
        hashtable,
        "message_no_color",
        (ptr_string) ? gui_color_decode (ptr_string, NULL) : NULL);

    arraylist_add (recorded_messages, hashtable);

    return NULL;
}

/*
 * Starts recording of messages displayed.
 */

void
record_start ()
{
    record_messages = 1;

    if (recorded_messages)
    {
        arraylist_clear (recorded_messages);
    }
    else
    {
        recorded_messages = arraylist_new (16, 0, 1,
                                           &record_cmp_cb, NULL,
                                           &record_free_cb, NULL);
    }
    if (!record_hook_line)
    {
        record_hook_line = hook_line (NULL, "*", NULL, NULL,
                                      &record_hook_line_cb, NULL, NULL);
    }
}

/*
 * Stops recording of messages displayed.
 */

void
record_stop ()
{
    record_messages = 0;

    if (record_hook_line)
    {
        unhook (record_hook_line);
        record_hook_line = NULL;
    }
}

/*
 * Checks if a recorded message field matches a value.
 *
 * Returns:
 *   1: value matches
 *   0: value does NOT match
 */

int
record_match (struct t_hashtable *recorded_msg,
              const char *field, const char *value)
{
    const char *ptr_value;

    ptr_value = (const char *)hashtable_get (recorded_msg, field);

    return ((!ptr_value && !value)
            || (ptr_value && value && (strcmp (ptr_value, value) == 0)));
}

/*
 * Searches if a prefix/message has been displayed in a buffer.
 *
 * Returns pointer to hashtable with the message found, NULL if the message
 * has NOT been displayed.
 */

struct t_hashtable *
record_search (const char *buffer, const char *prefix, const char *message,
               const char *tags)
{
    int i, size;
    struct t_hashtable *rec_msg;

    size = arraylist_size (recorded_messages);

    for (i = 0; i < size; i++)
    {
        rec_msg = (struct t_hashtable *)arraylist_get (recorded_messages, i);
        if (!rec_msg)
            continue;
        if (record_match (rec_msg, "buffer_name", buffer)
            && record_match (rec_msg, "prefix_no_color", prefix)
            && record_match (rec_msg, "message_no_color", message)
            && (!tags || !tags[0] || record_match (rec_msg, "tags", tags)))
        {
            return rec_msg;
        }
    }

    /* message not displayed */
    return NULL;
}

/*
 * Returns the number of messages displayed during the recording.
 */

int
record_count_messages ()
{
    return arraylist_size (recorded_messages);
}

/*
 * Adds all recorded messages to the dynamic string "msg".
 */

void
record_dump (char **msg)
{
    struct t_hashtable *rec_msg;
    const char *ptr_buffer_name, *ptr_prefix, *ptr_msg, *ptr_tags;
    int i, size;

    size = arraylist_size (recorded_messages);

    for (i = 0; i < size; i++)
    {
        rec_msg = (struct t_hashtable *)arraylist_get (recorded_messages, i);
        if (!rec_msg)
            continue;
        ptr_buffer_name = (const char *)hashtable_get (rec_msg, "buffer_name");
        ptr_prefix = (const char *)hashtable_get (rec_msg, "prefix_no_color");
        ptr_msg = (const char *)hashtable_get (rec_msg, "message_no_color");
        ptr_tags = (const char *)hashtable_get (rec_msg, "tags");
        string_dyn_concat (msg, "  ", -1);
        string_dyn_concat (msg, ptr_buffer_name, -1);
        string_dyn_concat (msg, ": prefix=\"", -1);
        string_dyn_concat (msg, ptr_prefix, -1);
        string_dyn_concat (msg, "\", message=\"", -1);
        string_dyn_concat (msg, ptr_msg, -1);
        string_dyn_concat (msg, "\", tags=\"", -1);
        string_dyn_concat (msg, ptr_tags, -1);
        string_dyn_concat (msg, "\"\n", -1);
    }
}
