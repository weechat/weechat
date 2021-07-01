/*
 * typing.c - manage typing status of users
 *
 * Copyright (C) 2021 Sébastien Helleu <flashcode@flashtux.org>
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
#include "typing-config.h"
#include "typing-status.h"


WEECHAT_PLUGIN_NAME(TYPING_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Typing status of users"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(8000);

struct t_weechat_plugin *weechat_typing_plugin = NULL;

struct t_hook *typing_signal_buffer_closing = NULL;
struct t_hook *typing_signal_input_text_changed = NULL;
struct t_hook *typing_modifier_input_text_for_buffer = NULL;
struct t_hook *typing_timer = NULL;


/*
 * Sends a "typing" signal.
 *
 * Returns code of last callback executed.
 */

int
typing_send_signal (struct t_gui_buffer *buffer,
                    struct t_typing_status *typing_status,
                    const char *signal_name)
{
    if (weechat_typing_plugin->debug)
    {
        weechat_printf (NULL, "%s: sending signal \"%s\" for buffer %s",
                        TYPING_PLUGIN_NAME,
                        signal_name,
                        weechat_buffer_get_string (buffer, "full_name"));
    }

    typing_status->last_signal_sent = time (NULL);
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

    return WEECHAT_RC_OK;
}

/*
 * Callback for signal "buffer_closing".
 */

int
typing_input_text_changed_signal_cb (const void *pointer, void *data,
                                     const char *signal,
                                     const char *type_data, void *signal_data)
{
    int text_search;
    const char *ptr_input, *ptr_input_for_buffer;
    struct t_gui_buffer *ptr_buffer;
    struct t_typing_status *ptr_typing_status;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    ptr_buffer = (struct t_gui_buffer *)signal_data;

    /* ignore any change in input if the user is searching text in the buffer */
    text_search = weechat_buffer_get_integer (ptr_buffer, "text_search");
    if (text_search != 0)
        return WEECHAT_RC_OK;

    ptr_input = weechat_buffer_get_string (ptr_buffer, "input");

    if (ptr_input && ptr_input[0])
    {
        /* input is a command? ignore it */
        ptr_input_for_buffer = weechat_string_input_for_buffer (ptr_input);
        if (!ptr_input_for_buffer)
            return WEECHAT_RC_OK;

        ptr_typing_status = weechat_hashtable_get (typing_status_self,
                                                   ptr_buffer);
        if (!ptr_typing_status)
            ptr_typing_status = typing_status_add (ptr_buffer);
        if (!ptr_typing_status)
            return WEECHAT_RC_OK;
        ptr_typing_status->status = TYPING_STATUS_STATUS_TYPING;
        ptr_typing_status->last_typed = time (NULL);
    }
    else
    {
        /* user was typing something? */
        ptr_typing_status = weechat_hashtable_get (typing_status_self,
                                                   ptr_buffer);
        if (ptr_typing_status
            && ((ptr_typing_status->status == TYPING_STATUS_STATUS_TYPING)
                || (ptr_typing_status->status == TYPING_STATUS_STATUS_PAUSED)))
        {
            /*
             * input cleared: maybe something was sent, not sure, so we just
             * set the status to "cleared", a signal can be sent later
             * in timer
             */
            ptr_typing_status->status = TYPING_STATUS_STATUS_CLEARED;
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

    ptr_typing_status = weechat_hashtable_get (typing_status_self, ptr_buffer);
    if (!ptr_typing_status)
        ptr_typing_status = typing_status_add (ptr_buffer);
    if (!ptr_typing_status)
        return NULL;

    typing_send_signal (ptr_buffer, ptr_typing_status, "typing_sent");
    weechat_hashtable_remove (typing_status_self, ptr_buffer);

    return NULL;
}

/*
 * Callback called periodically (via a timer) for each entry in hashtable
 * "typing_status_self".
 */

void
typing_status_self_map_cb (void *data,
                           struct t_hashtable *hashtable,
                           const void *key, const void *value)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_typing_status *ptr_typing_status;
    const char *ptr_input, *ptr_input_for_buffer;
    int delay_pause;

    /* make C compiler happy */
    (void) data;

    ptr_buffer = (struct t_gui_buffer *)key;
    ptr_typing_status = (struct t_typing_status *)value;

    if (ptr_typing_status->status == TYPING_STATUS_STATUS_TYPING)
    {
        ptr_input = weechat_buffer_get_string (ptr_buffer, "input");
        ptr_input_for_buffer = weechat_string_input_for_buffer (ptr_input);
        if (ptr_input_for_buffer)
        {
            /* check if typing is paused */
            delay_pause = weechat_config_integer (typing_config_look_delay_pause);
            if (ptr_typing_status->last_typed < time (NULL) - delay_pause)
            {
                ptr_typing_status->status = TYPING_STATUS_STATUS_PAUSED;
                typing_send_signal (ptr_buffer, ptr_typing_status,
                                    "typing_paused");
                weechat_hashtable_remove (hashtable, ptr_buffer);
            }
            else
            {
                typing_send_signal (ptr_buffer, ptr_typing_status,
                                    "typing_active");
            }
        }
        else
        {
            typing_send_signal (ptr_buffer, ptr_typing_status,
                                "typing_cleared");
            weechat_hashtable_remove (hashtable, ptr_buffer);
        }
    }
    else if (ptr_typing_status->status == TYPING_STATUS_STATUS_CLEARED)
    {
        typing_send_signal (ptr_buffer, ptr_typing_status,
                            "typing_cleared");
        weechat_hashtable_remove (hashtable, ptr_buffer);
    }
}

/*
 * Callback for modifier "input_text_for_buffer".
 */

int
typing_timer_cb (const void *pointer,
                 void *data,
                 int remaining_calls)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) remaining_calls;

    weechat_hashtable_map (typing_status_self,
                           &typing_status_self_map_cb, NULL);

    return WEECHAT_RC_OK;
}

/*
 * Creates or removes hooks, according to option typing.look.enabled.
 */

void
typing_setup_hooks ()
{
    if (weechat_config_boolean (typing_config_look_enabled))
    {
        if (!typing_signal_buffer_closing)
        {
            if (weechat_typing_plugin->debug >= 2)
                weechat_printf (NULL, "%s: creating hooks", TYPING_PLUGIN_NAME);
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
                weechat_printf (NULL, "%s: removing hooks", TYPING_PLUGIN_NAME);
            weechat_unhook (typing_signal_buffer_closing);
            typing_signal_buffer_closing = NULL;
            weechat_unhook (typing_signal_input_text_changed);
            typing_signal_input_text_changed = NULL;
            weechat_unhook (typing_modifier_input_text_for_buffer);
            typing_modifier_input_text_for_buffer = NULL;
            weechat_unhook (typing_timer);
            typing_timer = NULL;
        }
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

    typing_config_write ();
    typing_config_free ();

    typing_status_end ();

    return WEECHAT_RC_OK;
}
