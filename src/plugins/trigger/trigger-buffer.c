/*
 * trigger-buffer.c - debug buffer for triggers
 *
 * Copyright (C) 2014-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#include "../weechat-plugin.h"
#include "trigger.h"
#include "trigger-buffer.h"
#include "trigger-callback.h"
#include "trigger-config.h"


struct t_gui_buffer *trigger_buffer = NULL;
char **trigger_buffer_filters = NULL;


/*
 * Checks if a trigger matches the filters.
 *
 * Returns:
 *   1: trigger matches the filters
 *   0: trigger does NOT match the filters
 */

int
trigger_buffer_match_filters (struct t_trigger *trigger)
{
    int i;

    /* trigger is matching if there are no filters at all */
    if (!trigger_buffer_filters)
        return 1;

    for (i = 0; trigger_buffer_filters[i]; i++)
    {
        if (trigger_buffer_filters[i][0] == '@')
        {
            /* check if the hook matches the filter */
            if (weechat_strcasecmp (
                    trigger_hook_type_string[weechat_config_enum (trigger->options[TRIGGER_OPTION_HOOK])],
                    trigger_buffer_filters[i] + 1) == 0)
            {
                return 1;
            }
        }
        else
        {
            /* check if the name matches the filter */
            if (weechat_string_match (trigger->name, trigger_buffer_filters[i], 0))
                return 1;
        }
    }

    /* trigger does not match the filters */
    return 0;
}

/*
 * Sets filter for trigger monitor buffer.
 */

void
trigger_buffer_set_filter (const char *filter)
{
    if (trigger_buffer_filters)
    {
        weechat_string_free_split (trigger_buffer_filters);
        trigger_buffer_filters = NULL;
    }

    if (filter && filter[0])
        trigger_buffer_filters = weechat_string_split (
            filter,
            ",",
            NULL,
            WEECHAT_STRING_SPLIT_STRIP_LEFT
            | WEECHAT_STRING_SPLIT_STRIP_RIGHT
            | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
            0,
            NULL);
}

/*
 * Sets title for trigger monitor buffer.
 */

void
trigger_buffer_set_title (void)
{
    const char *ptr_filter;
    char title[1024];

    ptr_filter = weechat_buffer_get_string (trigger_buffer, "localvar_filter");
    snprintf (title, sizeof (title),
              _("Trigger monitor (filter: %s) | Input: q=close, words=filter"),
              (ptr_filter) ? ptr_filter : "*");

    weechat_buffer_set (trigger_buffer, "title", title);
}

/*
 * Callback for user data in trigger buffer.
 */

int
trigger_buffer_input_cb (const void *pointer, void *data,
                         struct t_gui_buffer *buffer,
                         const char *input_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    /* close buffer */
    if (strcmp (input_data, "q") == 0)
    {
        weechat_buffer_close (buffer);
        return WEECHAT_RC_OK;
    }

    /* set filters */
    if (strcmp (input_data, "*") == 0)
        weechat_buffer_set (buffer, "localvar_del_filter", "");
    else
        weechat_buffer_set (buffer, "localvar_set_filter", input_data);
    trigger_buffer_set_filter (weechat_buffer_get_string (buffer,
                                                          "localvar_filter"));
    trigger_buffer_set_title ();

    return WEECHAT_RC_OK;
}

/*
 * Callback called when trigger buffer is closed.
 */

int
trigger_buffer_close_cb (const void *pointer, void *data,
                         struct t_gui_buffer *buffer)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    trigger_buffer = NULL;

    return WEECHAT_RC_OK;
}

/*
 * Restores buffer callbacks (input and close) for buffer created by trigger
 * plugin.
 */

void
trigger_buffer_set_callbacks (void)
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
        trigger_buffer_set_filter (weechat_buffer_get_string (trigger_buffer,
                                                              "localvar_filter"));
    }
}

/*
 * Opens trigger buffer.
 */

void
trigger_buffer_open (const char *filter, int switch_to_buffer)
{
    struct t_hashtable *buffer_props;

    if (!trigger_buffer)
    {
        if (!weechat_buffer_search (TRIGGER_PLUGIN_NAME, TRIGGER_BUFFER_NAME))
        {
            buffer_props = weechat_hashtable_new (32,
                                                  WEECHAT_HASHTABLE_STRING,
                                                  WEECHAT_HASHTABLE_STRING,
                                                  NULL, NULL);
            if (buffer_props)
            {
                weechat_hashtable_set (
                    buffer_props, "short_name", TRIGGER_BUFFER_NAME);
                weechat_hashtable_set (
                    buffer_props, "localvar_set_type", "debug");
                weechat_hashtable_set (
                    buffer_props, "localvar_set_server", TRIGGER_BUFFER_NAME);
                weechat_hashtable_set (
                    buffer_props, "localvar_set_channel", TRIGGER_BUFFER_NAME);
                weechat_hashtable_set (
                    buffer_props, "localvar_set_no_log", "1");
                /* disable all highlights on this buffer */
                weechat_hashtable_set (buffer_props, "highlight_words", "-");
            }
            trigger_buffer = weechat_buffer_new_props (
                TRIGGER_BUFFER_NAME,
                buffer_props,
                &trigger_buffer_input_cb, NULL, NULL,
                &trigger_buffer_close_cb, NULL, NULL);
            weechat_hashtable_free (buffer_props);
        }

        /* failed to create buffer ? then return */
        if (!trigger_buffer)
            return;
    }

    if (filter && filter[0])
        weechat_buffer_set (trigger_buffer, "localvar_set_filter", filter);
    else
        weechat_buffer_set (trigger_buffer, "localvar_del_filter", "");
    trigger_buffer_set_filter (filter);

    trigger_buffer_set_title ();

    if (switch_to_buffer)
        weechat_buffer_set (trigger_buffer, "display", "1");
}

/*
 * Callback called for each entry in hashtable.
 */

void
trigger_buffer_hashtable_map_cb (void *data,
                                 struct t_hashtable *hashtable,
                                 const void *key, const void *value)
{
    struct t_trigger_context *context;
    const char *value_type;
    char *value_no_color;

    /* make C compiler happy */
    (void) hashtable;

    context = (struct t_trigger_context *)data;

    value_type = weechat_hashtable_get_string (hashtable, "type_values");
    if (!value_type)
        return;

    if (strcmp (value_type, "string") == 0)
    {
        value_no_color = (weechat_config_boolean (trigger_config_look_monitor_strip_colors)) ?
            weechat_string_remove_color ((const char *)value, NULL) : NULL;
        weechat_printf_date_tags (
            trigger_buffer, 0, "no_trigger",
            "%s%lu\t    %s: %s\"%s%s%s\"",
            weechat_color (weechat_config_string (trigger_config_color_identifier)),
            context->id,
            (char *)key,
            weechat_color ("chat_delimiters"),
            weechat_color ("reset"),
            (value_no_color) ? value_no_color : (const char *)value,
            weechat_color ("chat_delimiters"));
        free (value_no_color);
    }
    else if (strcmp (value_type, "pointer") == 0)
    {
        weechat_printf_date_tags (
            trigger_buffer, 0, "no_trigger",
            "%s%lu\t    %s: %p",
            weechat_color (weechat_config_string (trigger_config_color_identifier)),
            context->id,
            (char *)key,
            value);
    }
}

/*
 * Displays a hashtable on trigger buffer.
 */

void
trigger_buffer_display_hashtable (struct t_trigger_context *context,
                                  const char *name,
                                  struct t_hashtable *hashtable)
{
    if (!trigger_buffer)
        return;

    weechat_printf_date_tags (
        trigger_buffer, 0, "no_trigger",
        "%s%lu\t  %s:",
        weechat_color (weechat_config_string (trigger_config_color_identifier)),
        context->id,
        name);

    weechat_hashtable_map (hashtable,
                           &trigger_buffer_hashtable_map_cb, context);
}

/*
 * Displays a trigger in trigger buffer.
 *
 * Returns:
 *   1: the trigger has been displayed
 *   0: the trigger has NOT been displayed (no buffer, or does not match filter)
 */

int
trigger_buffer_display_trigger (struct t_trigger *trigger,
                                struct t_trigger_context *context)
{
    if (!trigger_buffer)
        return 0;

    /* check if trigger matches the filter(s) */
    if (!trigger_buffer_match_filters (trigger))
        return 0;

    weechat_printf_date_tags (
        trigger_buffer, 0, "no_trigger",
        "--> %s%lu\t%s: %s%s %s(%s%s%s)%s",
        weechat_color (weechat_config_string (trigger_config_color_identifier)),
        context->id,
        trigger_hook_type_string[weechat_config_enum (trigger->options[TRIGGER_OPTION_HOOK])],
        weechat_color ("chat_status_enabled"),
        trigger->name,
        weechat_color ("chat_delimiters"),
        weechat_color ("reset"),
        weechat_config_string (trigger->options[TRIGGER_OPTION_ARGUMENTS]),
        weechat_color ("chat_delimiters"),
        weechat_color ("reset"));
    if (context->buffer)
    {
        weechat_printf_date_tags (
            trigger_buffer, 0, "no_trigger",
            "%s%lu\t  buffer: %s%s",
            weechat_color (weechat_config_string (trigger_config_color_identifier)),
            context->id,
            weechat_color ("chat_buffer"),
            weechat_buffer_get_string (context->buffer, "full_name"));
    }
    if (context->pointers)
    {
        trigger_buffer_display_hashtable (context,
                                          "pointers", context->pointers);
    }
    if (context->extra_vars)
    {
        trigger_buffer_display_hashtable (context,
                                          "extra_vars", context->extra_vars);
    }

    return 1;
}

/*
 * Ends trigger buffer.
 */

void
trigger_buffer_end (void)
{
    if (trigger_buffer)
    {
        weechat_buffer_close (trigger_buffer);
        trigger_buffer = NULL;
    }

    if (trigger_buffer_filters)
    {
        weechat_string_free_split (trigger_buffer_filters);
        trigger_buffer_filters = NULL;
    }
}
