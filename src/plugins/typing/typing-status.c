/*
 * typing-status.c - manage self and other users typing status
 *
 * Copyright (C) 2021-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <string.h>
#include <stdio.h>

#include "../weechat-plugin.h"
#include "typing.h"
#include "typing-status.h"


char *typing_status_state_string[TYPING_STATUS_NUM_STATES] =
{ "off", "typing", "paused", "cleared" };

/* hashtable[buffer -> t_typing_status] */
struct t_hashtable *typing_status_self = NULL;

/* hashtable[buffer -> hashtable[nick -> t_typing_status]] */
struct t_hashtable *typing_status_nicks = NULL;


/*
 * Searches a state by name.
 *
 * Returns index of stats in enum t_typing_status_state, -1 if not found.
 */

int
typing_status_search_state (const char *state)
{
    int i;

    if (!state)
        return -1;

    for (i = 0; i < TYPING_STATUS_NUM_STATES; i++)
    {
        if (strcmp (typing_status_state_string[i], state) == 0)
            return i;
    }

    return -1;
}

/*
 * Removes self typing status for a buffer: key is a buffer pointer, value
 * is a t_typing_status pointer.
 */

void
typing_status_self_free_value_cb (struct t_hashtable *hashtable,
                                  const void *key, const void *value)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_typing_status *ptr_typing_status;

    /* make C compiler happy */
    (void) hashtable;

    ptr_buffer = (struct t_gui_buffer *)key;
    ptr_typing_status = (struct t_typing_status *)value;

    if (!ptr_buffer || !ptr_typing_status)
        return;

    if (weechat_typing_plugin->debug)
    {
        weechat_printf_date_tags (
            NULL, 0, "no_log",
            "%s: removing self typing status for buffer \"%s\"",
            TYPING_PLUGIN_NAME,
            weechat_buffer_get_string (ptr_buffer, "name"));
    }

    free (ptr_typing_status);
}

/*
 * Adds a new self typing status.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

struct t_typing_status *
typing_status_self_add (struct t_gui_buffer *buffer, int state, int last_typed)
{
    struct t_typing_status *ptr_typing_status;

    if (!buffer || (state < 0) || (state >= TYPING_STATUS_NUM_STATES))
        return NULL;

    if (!typing_status_self)
    {
        typing_status_self = weechat_hashtable_new (
            64,
            WEECHAT_HASHTABLE_POINTER,  /* buffer */
            WEECHAT_HASHTABLE_POINTER,  /* t_typing_status */
            NULL,
            NULL);
        if (!typing_status_self)
            return NULL;
        weechat_hashtable_set_pointer (typing_status_self,
                                       "callback_free_value",
                                       &typing_status_self_free_value_cb);
    }

    ptr_typing_status = weechat_hashtable_get (typing_status_self, buffer);
    if (!ptr_typing_status)
    {
        if (weechat_typing_plugin->debug)
        {
            weechat_printf_date_tags (
                NULL, 0, "no_log",
                "%s: creating self typing status for buffer \"%s\"",
                TYPING_PLUGIN_NAME,
                weechat_buffer_get_string (buffer, "name"));
        }
        ptr_typing_status = malloc (sizeof (*ptr_typing_status));
        if (!ptr_typing_status)
            return NULL;
    }

    ptr_typing_status->state = state;
    ptr_typing_status->last_typed = last_typed;

    weechat_hashtable_set (typing_status_self, buffer, ptr_typing_status);

    return ptr_typing_status;
}

/*
 * Searches a self typing status for a buffer.
 *
 * Returns pointer to t_typing_status found, NULL if not found.
 */

struct t_typing_status *
typing_status_self_search (struct t_gui_buffer *buffer)
{
    if (!typing_status_self)
        return NULL;

    return weechat_hashtable_get (typing_status_self, buffer);
}

/*
 * Removes nicks typing status: key is a buffer pointer, value is a hashtable
 * pointer.
 */

void
typing_status_nicks_free_value_cb (struct t_hashtable *hashtable,
                                   const void *key, const void *value)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_hashtable *ptr_nicks;

    /* make C compiler happy */
    (void) hashtable;

    ptr_buffer = (struct t_gui_buffer *)key;
    ptr_nicks = (struct t_hashtable *)value;

    if (!ptr_buffer || !ptr_nicks)
        return;

    if (weechat_typing_plugin->debug)
    {
        weechat_printf_date_tags (
            NULL, 0, "no_log",
            "%s: removing nicks typing status for buffer \"%s\"",
            TYPING_PLUGIN_NAME,
            weechat_buffer_get_string (ptr_buffer, "name"));
    }

    weechat_hashtable_free (ptr_nicks);
}

/*
 * Removes a nick typing status: key is a nick (string), value is a
 * t_typing_status pointer.
 */

void
typing_status_nick_free_value_cb (struct t_hashtable *hashtable,
                                   const void *key, const void *value)
{
    const char *ptr_nick;
    struct t_typing_status *ptr_typing_status;

    /* make C compiler happy */
    (void) hashtable;

    ptr_nick = (const char *)key;
    ptr_typing_status = (struct t_typing_status *)value;

    if (!ptr_nick || !ptr_typing_status)
        return;

    free (ptr_typing_status);
}

/*
 * Adds a nick typing status for a buffer.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

struct t_typing_status *
typing_status_nick_add (struct t_gui_buffer *buffer, const char *nick,
                        int state, int last_typed)
{
    struct t_hashtable *ptr_nicks;
    struct t_typing_status *ptr_typing_status;

    if (!buffer || !nick || (state < 0) || (state >= TYPING_STATUS_NUM_STATES))
        return NULL;

    if (!typing_status_nicks)
    {
        typing_status_nicks = weechat_hashtable_new (
            64,
            WEECHAT_HASHTABLE_POINTER,  /* buffer */
            WEECHAT_HASHTABLE_POINTER,  /* hashtable */
            NULL,
            NULL);
        if (!typing_status_nicks)
            return NULL;
        weechat_hashtable_set_pointer (typing_status_nicks,
                                       "callback_free_value",
                                       &typing_status_nicks_free_value_cb);
    }

    ptr_nicks = weechat_hashtable_get (typing_status_nicks, buffer);
    if (!ptr_nicks)
    {
        ptr_nicks = weechat_hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING,   /* nick */
            WEECHAT_HASHTABLE_POINTER,  /* t_typing_status */
            NULL,
            NULL);
        if (!ptr_nicks)
            return NULL;
        weechat_hashtable_set_pointer (ptr_nicks,
                                       "callback_free_value",
                                       &typing_status_nick_free_value_cb);
        weechat_hashtable_set (typing_status_nicks, buffer, ptr_nicks);
    }

    ptr_typing_status = weechat_hashtable_get (ptr_nicks, nick);
    if (!ptr_typing_status)
    {
        if (weechat_typing_plugin->debug)
        {
            weechat_printf_date_tags (
                NULL, 0, "no_log",
                "%s: creating typing status for buffer \"%s\" and nick \"%s\"",
                TYPING_PLUGIN_NAME,
                weechat_buffer_get_string (buffer, "name"),
                nick);
        }
        ptr_typing_status = malloc (sizeof (*ptr_typing_status));
        if (!ptr_typing_status)
            return NULL;
    }

    ptr_typing_status->state = state;
    ptr_typing_status->last_typed = last_typed;

    weechat_hashtable_set (ptr_nicks, nick, ptr_typing_status);

    return ptr_typing_status;
}

/*
 * Searches a nick typing status for a buffer.
 *
 * Returns pointer to t_typing_status found, NULL if not found.
 */

struct t_typing_status *
typing_status_nick_search (struct t_gui_buffer *buffer, const char *nick)
{
    struct t_hashtable *ptr_nicks;

    if (!typing_status_nicks || !buffer || !nick)
        return NULL;

    ptr_nicks = weechat_hashtable_get (typing_status_nicks, buffer);
    if (!ptr_nicks)
        return NULL;

    return weechat_hashtable_get (ptr_nicks, nick);
}

/*
 * Removes a nick typing status from a buffer.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

void
typing_status_nick_remove (struct t_gui_buffer *buffer, const char *nick)
{
    struct t_hashtable *ptr_nicks;

    if (!typing_status_nicks)
        return;

    ptr_nicks = weechat_hashtable_get (typing_status_nicks, buffer);
    if (!ptr_nicks)
        return;

    weechat_hashtable_remove (ptr_nicks, nick);
}

/*
 * Ends typing status.
 */

void
typing_status_end (void)
{
    if (typing_status_self)
    {
        weechat_hashtable_free (typing_status_self);
        typing_status_self = NULL;
    }
    if (typing_status_nicks)
    {
        weechat_hashtable_free (typing_status_nicks);
        typing_status_nicks = NULL;
    }
}
