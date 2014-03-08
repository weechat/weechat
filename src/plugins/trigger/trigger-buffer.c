/*
 * trigger-buffer.c - debug buffer for triggers
 *
 * Copyright (C) 2014 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "trigger.h"
#include "trigger-buffer.h"
#include "trigger-config.h"


struct t_gui_buffer *trigger_buffer = NULL;


/*
 * Callback called for each entry in hashtable.
 */

void
trigger_buffer_hashtable_map_cb (void *data,
                                 struct t_hashtable *hashtable,
                                 const void *key, const void *value)
{
    const char *value_type;
    char *value_no_color;

    /* make C compiler happy */
    (void) data;
    (void) hashtable;

    value_type = weechat_hashtable_get_string (hashtable, "type_values");
    if (!value_type)
        return;

    if (strcmp (value_type, "string") == 0)
    {
        value_no_color = (weechat_config_boolean (trigger_config_look_monitor_strip_colors)) ?
            weechat_string_remove_color ((const char *)value, NULL) : NULL;
        weechat_printf_tags (trigger_buffer, "no_trigger",
                             "\t    %s: %s\"%s%s%s\"",
                             (char *)key,
                             weechat_color ("chat_delimiters"),
                             weechat_color ("reset"),
                             (value_no_color) ? value_no_color : (const char *)value,
                             weechat_color ("chat_delimiters"));
        if (value_no_color)
            free (value_no_color);
    }
    else if (strcmp (value_type, "pointer") == 0)
    {
        weechat_printf_tags (trigger_buffer, "no_trigger",
                             "\t    %s: 0x%lx",
                             (char *)key,
                             value);
    }
}

/*
 * Displays a hashtable on trigger buffer.
 */

void
trigger_buffer_display_hashtable (const char *name,
                                  struct t_hashtable *hashtable)
{
    if (!trigger_buffer)
        return;

    weechat_printf_tags (trigger_buffer, "no_trigger", "  %s:", name);

    weechat_hashtable_map (hashtable, &trigger_buffer_hashtable_map_cb, NULL);
}

/*
 * Displays a trigger in trigger buffer.
 */

void
trigger_buffer_display_trigger (struct t_trigger *trigger,
                                struct t_gui_buffer *buffer,
                                struct t_hashtable *pointers,
                                struct t_hashtable *extra_vars)
{
    if (!trigger_buffer)
        return;

    weechat_printf_tags (trigger_buffer, "no_trigger",
                         "%s\t%s%s %s(%s%s%s)",
                         trigger_hook_type_string[weechat_config_integer (trigger->options[TRIGGER_OPTION_HOOK])],
                         weechat_color (weechat_config_string (trigger_config_color_trigger)),
                         trigger->name,
                         weechat_color ("chat_delimiters"),
                         weechat_color ("reset"),
                         weechat_config_string (trigger->options[TRIGGER_OPTION_ARGUMENTS]),
                         weechat_color ("chat_delimiters"));
    if (buffer)
    {
        weechat_printf_tags (trigger_buffer, "no_trigger",
                             "\t  buffer: %s%s",
                             weechat_color ("chat_buffer"),
                             weechat_buffer_get_string (buffer, "full_name"));
    }
    if (pointers)
        trigger_buffer_display_hashtable ("pointers", pointers);
    if (extra_vars)
        trigger_buffer_display_hashtable ("extra_vars", extra_vars);
}

/*
 * Callback for user data in trigger buffer.
 */

int
trigger_buffer_input_cb (void *data, struct t_gui_buffer *buffer,
                        const char *input_data)
{
    /* make C compiler happy */
    (void) data;

    /* close buffer */
    if (strcmp (input_data, "q") == 0)
    {
        weechat_buffer_close (buffer);
        return WEECHAT_RC_OK;
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback called when trigger buffer is closed.
 */

int
trigger_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    /* make C compiler happy */
    (void) data;
    (void) buffer;

    trigger_buffer = NULL;

    return WEECHAT_RC_OK;
}

/*
 * Restore buffer callbacks (input and close) for buffer created by trigger
 * plugin.
 */

void
trigger_buffer_set_callbacks ()
{
    struct t_gui_buffer *ptr_buffer;

    ptr_buffer = weechat_buffer_search (TRIGGER_PLUGIN_NAME,
                                        TRIGGER_BUFFER_NAME);
    if (ptr_buffer)
    {
        trigger_buffer = ptr_buffer;
        weechat_buffer_set_pointer (trigger_buffer, "close_callback",
                                    &trigger_buffer_close_cb);
        weechat_buffer_set_pointer (trigger_buffer, "input_callback",
                                    &trigger_buffer_input_cb);
    }
}

/*
 * Opens trigger buffer.
 */

void
trigger_buffer_open (int switch_to_buffer)
{
    if (!trigger_buffer)
    {
        trigger_buffer = weechat_buffer_new (TRIGGER_BUFFER_NAME,
                                             &trigger_buffer_input_cb, NULL,
                                             &trigger_buffer_close_cb, NULL);

        /* failed to create buffer ? then return */
        if (!trigger_buffer)
            return;

        weechat_buffer_set (trigger_buffer, "title", _("Trigger monitor"));

        if (!weechat_buffer_get_integer (trigger_buffer, "short_name_is_set"))
            weechat_buffer_set (trigger_buffer, "short_name", TRIGGER_BUFFER_NAME);
        weechat_buffer_set (trigger_buffer, "localvar_set_type", "debug");
        weechat_buffer_set (trigger_buffer, "localvar_set_server", TRIGGER_BUFFER_NAME);
        weechat_buffer_set (trigger_buffer, "localvar_set_channel", TRIGGER_BUFFER_NAME);
        weechat_buffer_set (trigger_buffer, "localvar_set_no_log", "1");

        /* disable all highlights on this buffer */
        weechat_buffer_set (trigger_buffer, "highlight_words", "-");
    }

    if (switch_to_buffer)
        weechat_buffer_set (trigger_buffer, "display", "1");
}
