/*
 * typing.c - manage typing status of users
 *
 * Copyright (C) 2021-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <time.h>

#include "../weechat-plugin.h"
#include "typing.h"
#include "typing-bar-item.h"
#include "typing-config.h"
#include "typing-status.h"


WEECHAT_PLUGIN_NAME(TYPING_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Typing status of users"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(TYPING_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_typing_plugin = NULL;

struct t_hook *typing_signal_buffer_closing = NULL;
struct t_hook *typing_signal_input_text_changed = NULL;
struct t_hook *typing_modifier_input_text_for_buffer = NULL;
struct t_hook *typing_timer = NULL;
struct t_hook *typing_signal_typing_set_nick = NULL;
struct t_hook *typing_signal_typing_reset_buffer = NULL;

int typing_update_item = 0;


/*
 * Sends a "typing" signal.
 *
 * Returns code of last callback executed.
 */

int
typing_send_signal (struct t_gui_buffer *buffer, const char *signal_name)
{
    if (weechat_typing_plugin->debug)
    {
        weechat_printf (NULL, "%s: sending signal \"%s\" for buffer %s",
                        TYPING_PLUGIN_NAME,
                        signal_name,
                        weechat_buffer_get_string (buffer, "full_name"));
    }

    return weechat_hook_signal_send (signal_name,
                                     WEECHAT_HOOK_SIGNAL_POINTER,
                                     buffer);
}

/*
 * Callback for signal "buffer_closing".
 */

int
typing_buffer_closing_signal_cb (const void *pointer, void *data,
                                 const char *signal,
                                 const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    weechat_hashtable_remove (typing_status_self, signal_data);
    weechat_hashtable_remove (typing_status_nicks, signal_data);

    return WEECHAT_RC_OK;
}

/*
 * Callback for signal "input_text_changed".
 */

int
typing_input_text_changed_signal_cb (const void *pointer, void *data,
                                     const char *signal,
                                     const char *type_data, void *signal_data)
{
    int text_search, input_valid;
    const char *ptr_input, *ptr_input_for_buffer;
    struct t_gui_buffer *ptr_buffer;
    struct t_typing_status *ptr_typing_status;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) != 0)
        return WEECHAT_RC_OK;

    ptr_buffer = (struct t_gui_buffer *)signal_data;
    if (!ptr_buffer)
        return WEECHAT_RC_OK;

    /* ignore any change in input if the user is searching text in the buffer */
    text_search = weechat_buffer_get_integer (ptr_buffer, "text_search");
    if (text_search != 0)
        return WEECHAT_RC_OK;

    ptr_input = weechat_buffer_get_string (ptr_buffer, "input");
    input_valid = (ptr_input && ptr_input[0]) ?
        weechat_utf8_strlen (ptr_input) >= weechat_config_integer (typing_config_look_input_min_chars) : 0;

    if (input_valid)
    {
        /* input is a command? ignore it */
        ptr_input_for_buffer = weechat_string_input_for_buffer (ptr_input);
        if (!ptr_input_for_buffer)
            return WEECHAT_RC_OK;

        ptr_typing_status = typing_status_self_search (ptr_buffer);
        if (!ptr_typing_status)
        {
            ptr_typing_status = typing_status_self_add (
                ptr_buffer,
                TYPING_STATUS_STATE_TYPING,
                0);
        }
        if (!ptr_typing_status)
            return WEECHAT_RC_OK;
        ptr_typing_status->state = TYPING_STATUS_STATE_TYPING;
        ptr_typing_status->last_typed = time (NULL);
    }
    else
    {
        /* user was typing something? */
        ptr_typing_status = typing_status_self_search (ptr_buffer);
        if (ptr_typing_status
            && ((ptr_typing_status->state == TYPING_STATUS_STATE_TYPING)
                || (ptr_typing_status->state == TYPING_STATUS_STATE_PAUSED)))
        {
            /*
             * input cleared: maybe something was sent, not sure, so we just
             * set the state to "cleared", a signal can be sent later
             * in timer
             */
            ptr_typing_status->state = TYPING_STATUS_STATE_CLEARED;
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for modifier "input_text_for_buffer".
 */

char *
typing_input_text_for_buffer_modifier_cb (const void *pointer,
                                          void *data,
                                          const char *modifier,
                                          const char *modifier_data,
                                          const char *string)
{
    int rc, text_search;
    unsigned long value;
    const char *ptr_input_for_buffer;
    struct t_gui_buffer *ptr_buffer;
    struct t_typing_status *ptr_typing_status;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) modifier;
    (void) string;

    rc = sscanf (modifier_data, "%lx", &value);
    if ((rc == EOF) || (rc == 0))
        return NULL;
    ptr_buffer = (struct t_gui_buffer *)value;

    /* ignore any change in input if the user is searching text in the buffer */
    text_search = weechat_buffer_get_integer (ptr_buffer, "text_search");
    if (text_search != 0)
        return WEECHAT_RC_OK;

    /* string is a command? ignore it */
    ptr_input_for_buffer = weechat_string_input_for_buffer (string);
    if (!ptr_input_for_buffer)
        return NULL;

    ptr_typing_status = typing_status_self_search (ptr_buffer);
    if (!ptr_typing_status)
    {
        ptr_typing_status = typing_status_self_add (ptr_buffer,
                                                    TYPING_STATUS_STATE_OFF,
                                                    0);
    }
    if (!ptr_typing_status)
        return NULL;

    typing_send_signal (ptr_buffer, "typing_self_sent");
    weechat_hashtable_remove (typing_status_self, ptr_buffer);

    return NULL;
}

/*
 * Callback called periodically (via a timer) for each entry in hashtable
 * "typing_status_self".
 */

void
typing_status_self_status_map_cb (void *data,
                                  struct t_hashtable *hashtable,
                                  const void *key, const void *value)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_typing_status *ptr_typing_status;
    const char *ptr_input, *ptr_input_for_buffer;
    time_t current_time;
    int delay_pause;

    current_time = *((time_t *)data);

    ptr_buffer = (struct t_gui_buffer *)key;
    ptr_typing_status = (struct t_typing_status *)value;

    if (!ptr_buffer || !ptr_typing_status)
        return;

    if (ptr_typing_status->state == TYPING_STATUS_STATE_TYPING)
    {
        ptr_input = weechat_buffer_get_string (ptr_buffer, "input");
        ptr_input_for_buffer = weechat_string_input_for_buffer (ptr_input);
        if (ptr_input_for_buffer)
        {
            /* check if typing is paused */
            delay_pause = weechat_config_integer (typing_config_look_delay_set_paused);
            if (ptr_typing_status->last_typed < current_time - delay_pause)
            {
                ptr_typing_status->state = TYPING_STATUS_STATE_PAUSED;
                typing_send_signal (ptr_buffer, "typing_self_paused");
                weechat_hashtable_remove (hashtable, ptr_buffer);
            }
            else
            {
                typing_send_signal (ptr_buffer, "typing_self_typing");
            }
        }
        else
        {
            typing_send_signal (ptr_buffer, "typing_self_cleared");
            weechat_hashtable_remove (hashtable, ptr_buffer);
        }
    }
    else if (ptr_typing_status->state == TYPING_STATUS_STATE_CLEARED)
    {
        typing_send_signal (ptr_buffer, "typing_self_cleared");
        weechat_hashtable_remove (hashtable, ptr_buffer);
    }
}

/*
 * Callback called periodically (via a timer) for each entry in hashtable
 * "typing_status_nicks".
 */

void
typing_status_nicks_status_map_cb (void *data,
                                   struct t_hashtable *hashtable,
                                   const void *key, const void *value)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_typing_status *ptr_typing_status;
    time_t current_time;
    int delay_purge_pause, delay_purge_typing;

    current_time = *((time_t *)data);

    ptr_buffer = (struct t_gui_buffer *)key;
    ptr_typing_status = (struct t_typing_status *)value;

    if (!ptr_buffer || !ptr_typing_status)
        return;

    delay_purge_pause = weechat_config_integer (
        typing_config_look_delay_purge_paused);
    delay_purge_typing = weechat_config_integer (
        typing_config_look_delay_purge_typing);

    if (((ptr_typing_status->state == TYPING_STATUS_STATE_PAUSED)
         && (ptr_typing_status->last_typed < current_time - delay_purge_pause))
        || ((ptr_typing_status->state == TYPING_STATUS_STATE_TYPING)
            && (ptr_typing_status->last_typed < current_time - delay_purge_typing)))
    {
        weechat_hashtable_remove (hashtable, key);
        typing_update_item = 1;
    }
}

/*
 * Callback called periodically (via a timer) for each entry in hashtable
 * "typing_status_nicks".
 */

void
typing_status_nicks_hash_map_cb (void *data,
                                 struct t_hashtable *hashtable,
                                 const void *key, const void *value)
{
    struct t_hashtable *ptr_nicks;

    ptr_nicks = (struct t_hashtable *)value;

    if (!ptr_nicks)
        return;

    weechat_hashtable_map (ptr_nicks,
                           &typing_status_nicks_status_map_cb,
                           data);

    /* no more nicks for the buffer? then remove the buffer */
    if (weechat_hashtable_get_integer (ptr_nicks, "items_count") == 0)
        weechat_hashtable_remove (hashtable, key);
}

/*
 * Typing timer used to send continuously the self typing status.
 */

int
typing_timer_cb (const void *pointer, void *data, int remaining_calls)
{
    time_t current_time;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) remaining_calls;

    typing_update_item = 0;
    current_time = time (NULL);

    weechat_hashtable_map (typing_status_self,
                           &typing_status_self_status_map_cb, &current_time);
    weechat_hashtable_map (typing_status_nicks,
                           &typing_status_nicks_hash_map_cb, &current_time);

    if (typing_update_item)
        weechat_bar_item_update (TYPING_BAR_ITEM_NAME);

    return WEECHAT_RC_OK;
}

/*
 * Callback for signal "typing_set_nick".
 */

int
typing_typing_set_nick_signal_cb (const void *pointer, void *data,
                                 const char *signal,
                                 const char *type_data, void *signal_data)
{
    char **items;
    int num_items, rc, state, updated;
    unsigned long value;
    struct t_gui_buffer *ptr_buffer;
    struct t_typing_status *ptr_typing_status;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    items = weechat_string_split ((const char *)signal_data, ";", NULL,
                                  0, 3, &num_items);
    if (!items || (num_items != 3))
        goto end;

    rc = sscanf (items[0], "%lx", &value);
    if ((rc == EOF) || (rc == 0))
        goto end;
    ptr_buffer = (struct t_gui_buffer *)value;
    if (!ptr_buffer)
        goto end;

    state = typing_status_search_state (items[1]);
    if (state < 0)
        goto end;

    if (!items[2][0])
        goto end;

    updated = 0;
    ptr_typing_status = typing_status_nick_search (ptr_buffer, items[2]);
    if ((state == TYPING_STATUS_STATE_TYPING)
        || (state == TYPING_STATUS_STATE_PAUSED))
    {
        if (ptr_typing_status)
        {
            if (ptr_typing_status->state != state)
                updated = 1;
            ptr_typing_status->state = state;
            ptr_typing_status->last_typed = time (NULL);
        }
        else
        {
            typing_status_nick_add (ptr_buffer, items[2], state, time (NULL));
            updated = 1;
        }
    }
    else
    {
        if (ptr_typing_status)
            updated = 1;
        typing_status_nick_remove (ptr_buffer, items[2]);
    }

    if (updated)
        weechat_bar_item_update (TYPING_BAR_ITEM_NAME);

end:
    weechat_string_free_split (items);

    return WEECHAT_RC_OK;
}

/*
 * Callback for signal "typing_reset_buffer".
 */

int
typing_typing_reset_buffer_signal_cb (const void *pointer, void *data,
                                      const char *signal,
                                      const char *type_data, void *signal_data)
{
    int items_count;
    struct t_gui_buffer *ptr_buffer;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    ptr_buffer = (struct t_gui_buffer *)signal_data;

    if (!typing_status_nicks)
        return WEECHAT_RC_OK;

    items_count = weechat_hashtable_get_integer (typing_status_nicks,
                                                 "items_count");
    weechat_hashtable_remove (typing_status_nicks, ptr_buffer);
    if (items_count > 0)
        weechat_bar_item_update (TYPING_BAR_ITEM_NAME);

    return WEECHAT_RC_OK;
}

/*
 * Creates or removes hooks, according to options "typing.look.enabled_*".
 */

void
typing_setup_hooks ()
{
    if (weechat_config_boolean (typing_config_look_enabled_self))
    {
        if (!typing_signal_buffer_closing)
        {
            if (weechat_typing_plugin->debug >= 2)
            {
                weechat_printf (NULL, "%s: creating hooks (self)",
                                TYPING_PLUGIN_NAME);
            }
            typing_signal_buffer_closing = weechat_hook_signal (
                "buffer_closing",
                &typing_buffer_closing_signal_cb, NULL, NULL);
            typing_signal_input_text_changed = weechat_hook_signal (
                "input_text_changed",
                &typing_input_text_changed_signal_cb, NULL, NULL);
            typing_modifier_input_text_for_buffer = weechat_hook_modifier (
                "input_text_for_buffer",
                &typing_input_text_for_buffer_modifier_cb, NULL, NULL);
            typing_timer = weechat_hook_timer (
                1000, 0, 0,
                &typing_timer_cb, NULL, NULL);
        }
    }
    else
    {
        if (typing_signal_buffer_closing)
        {
            if (weechat_typing_plugin->debug >= 2)
            {
                weechat_printf (NULL, "%s: removing hooks (self)",
                                TYPING_PLUGIN_NAME);
            }
            weechat_unhook (typing_signal_buffer_closing);
            typing_signal_buffer_closing = NULL;
            weechat_unhook (typing_signal_input_text_changed);
            typing_signal_input_text_changed = NULL;
            weechat_unhook (typing_modifier_input_text_for_buffer);
            typing_modifier_input_text_for_buffer = NULL;
            weechat_unhook (typing_timer);
            typing_timer = NULL;
            if (typing_status_self)
            {
                weechat_hashtable_free (typing_status_self);
                typing_status_self = NULL;
            }
        }
    }

    if (weechat_config_boolean (typing_config_look_enabled_nicks))
    {
        if (!typing_signal_typing_set_nick)
        {
            if (weechat_typing_plugin->debug >= 2)
            {
                weechat_printf (NULL, "%s: creating hooks (nicks)",
                                TYPING_PLUGIN_NAME);
            }
            typing_signal_typing_set_nick = weechat_hook_signal (
                "typing_set_nick",
                &typing_typing_set_nick_signal_cb, NULL, NULL);
            typing_signal_typing_reset_buffer = weechat_hook_signal (
                "typing_reset_buffer",
                &typing_typing_reset_buffer_signal_cb, NULL, NULL);
        }
        if (!typing_timer)
        {
            typing_timer = weechat_hook_timer (
                1000, 0, 0,
                &typing_timer_cb, NULL, NULL);
        }
    }
    else
    {
        if (typing_signal_typing_set_nick)
        {
            if (weechat_typing_plugin->debug >= 2)
            {
                weechat_printf (NULL, "%s: removing hooks (nicks)",
                                TYPING_PLUGIN_NAME);
            }
            weechat_unhook (typing_signal_typing_set_nick);
            typing_signal_typing_set_nick = NULL;
            weechat_unhook (typing_signal_typing_reset_buffer);
            typing_signal_typing_reset_buffer = NULL;
            if (typing_status_nicks)
            {
                weechat_hashtable_free (typing_status_nicks);
                typing_status_nicks = NULL;
            }
        }
        weechat_unhook (typing_timer);
        typing_timer = NULL;
    }
}

/*
 * Removes all hooks.
 */

void
typing_remove_hooks ()
{
    if (typing_signal_buffer_closing)
    {
        weechat_unhook (typing_signal_buffer_closing);
        typing_signal_buffer_closing = NULL;
    }
    if (typing_signal_input_text_changed)
    {
        weechat_unhook (typing_signal_input_text_changed);
        typing_signal_input_text_changed = NULL;
    }
    if (typing_modifier_input_text_for_buffer)
    {
        weechat_unhook (typing_modifier_input_text_for_buffer);
        typing_modifier_input_text_for_buffer = NULL;
    }
    if (typing_timer)
    {
        weechat_unhook (typing_timer);
        typing_timer = NULL;
    }
    if (typing_signal_typing_set_nick)
    {
        weechat_unhook (typing_signal_typing_set_nick);
        typing_signal_typing_set_nick = NULL;
    }
    if (typing_signal_typing_reset_buffer)
    {
        weechat_unhook (typing_signal_typing_reset_buffer);
        typing_signal_typing_reset_buffer = NULL;
    }
}

/*
 * Initializes typing plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    if (!typing_config_init ())
        return WEECHAT_RC_ERROR;

    typing_config_read ();

    typing_setup_hooks ();

    typing_bar_item_init ();

    return WEECHAT_RC_OK;
}

/*
 * Ends typing plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    typing_remove_hooks ();

    typing_config_write ();
    typing_config_free ();

    typing_status_end ();

    return WEECHAT_RC_OK;
}
