/*
 * buflist-config.c - buflist configuration options (file buflist.conf)
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include "buflist.h"
#include "buflist-config.h"
#include "buflist-bar-item.h"


struct t_config_file *buflist_config_file = NULL;

/* sections */

struct t_config_section *buflist_config_section_look = NULL;
struct t_config_section *buflist_config_section_format = NULL;

/* buflist config, look section */

struct t_config_option *buflist_config_look_add_newline = NULL;
struct t_config_option *buflist_config_look_auto_scroll = NULL;
struct t_config_option *buflist_config_look_display_conditions = NULL;
struct t_config_option *buflist_config_look_enabled = NULL;
struct t_config_option *buflist_config_look_mouse_jump_visited_buffer = NULL;
struct t_config_option *buflist_config_look_mouse_move_buffer = NULL;
struct t_config_option *buflist_config_look_mouse_wheel = NULL;
struct t_config_option *buflist_config_look_nick_prefix = NULL;
struct t_config_option *buflist_config_look_nick_prefix_empty = NULL;
struct t_config_option *buflist_config_look_signals_refresh = NULL;
struct t_config_option *buflist_config_look_sort = NULL;
struct t_config_option *buflist_config_look_use_items = NULL;

/* buflist config, format section */

struct t_config_option *buflist_config_format_buffer = NULL;
struct t_config_option *buflist_config_format_buffer_current = NULL;
struct t_config_option *buflist_config_format_hotlist = NULL;
struct t_config_option *buflist_config_format_hotlist_level[4] = {
    NULL, NULL, NULL, NULL,
};
struct t_config_option *buflist_config_format_hotlist_level_none = NULL;
struct t_config_option *buflist_config_format_hotlist_separator = NULL;
struct t_config_option *buflist_config_format_indent = NULL;
struct t_config_option *buflist_config_format_lag = NULL;
struct t_config_option *buflist_config_format_name = NULL;
struct t_config_option *buflist_config_format_nick_prefix = NULL;
struct t_config_option *buflist_config_format_number = NULL;
struct t_config_option *buflist_config_format_tls_version = NULL;

struct t_hook **buflist_config_signals_refresh = NULL;
int buflist_config_num_signals_refresh = 0;
char **buflist_config_sort_fields[BUFLIST_BAR_NUM_ITEMS] = { NULL, NULL, NULL };
int buflist_config_sort_fields_count[BUFLIST_BAR_NUM_ITEMS] = { 0, 0, 0 };
char *buflist_config_format_buffer_eval = NULL;
char *buflist_config_format_buffer_current_eval = NULL;
char *buflist_config_format_hotlist_eval = NULL;


/*
 * Reloads buflist configuration file.
 */

int
buflist_config_reload (const void *pointer, void *data,
                       struct t_config_file *config_file)
{
    int rc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    rc = weechat_config_reload (config_file);

    buflist_add_bar ();

    return rc;
}

/*
 * Frees the signals hooked for refresh.
 */

void
buflist_config_free_signals_refresh ()
{
    int i;

    if (!buflist_config_signals_refresh)
        return;

    for (i = 0; i < buflist_config_num_signals_refresh; i++)
    {
        weechat_unhook (buflist_config_signals_refresh[i]);
    }

    free (buflist_config_signals_refresh);
    buflist_config_signals_refresh = NULL;

    buflist_config_num_signals_refresh = 0;
}

/*
 * Compares two signals to add them in the sorted arraylist.
 *
 * Returns:
 *   -1: signal1 < signal2
 *    0: signal1 == signal2
 *    1: signal1 > signal2
 */

int
buflist_config_compare_signals (void *data, struct t_arraylist *arraylist,
                                void *pointer1, void *pointer2)
{
    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    return strcmp ((const char *)pointer1, (const char *)pointer2);
}

/*
 * Callback for a signal on a buffer.
 */

int
buflist_config_signal_buffer_cb (const void *pointer, void *data,
                                 const char *signal, const char *type_data,
                                 void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    buflist_bar_item_update (0);

    return WEECHAT_RC_OK;
}

/*
 * Hooks the signals for refresh.
 */

void
buflist_config_hook_signals_refresh ()
{
    char **all_signals, **signals;
    const char *ptr_signals_refresh;
    struct t_arraylist *signals_list;
    int count, list_size, i;

    all_signals = weechat_string_dyn_alloc (256);
    if (!all_signals)
        return;

    ptr_signals_refresh = weechat_config_string (
        buflist_config_look_signals_refresh);

    weechat_string_dyn_concat (all_signals, BUFLIST_CONFIG_SIGNALS_REFRESH, -1);
    if (ptr_signals_refresh && ptr_signals_refresh[0])
    {
        weechat_string_dyn_concat (all_signals, ",", -1);
        weechat_string_dyn_concat (all_signals, ptr_signals_refresh, -1);
    }
    if (weechat_config_boolean (buflist_config_look_nick_prefix))
    {
        weechat_string_dyn_concat (all_signals, ",", -1);
        weechat_string_dyn_concat (
            all_signals,
            BUFLIST_CONFIG_SIGNALS_REFRESH_NICK_PREFIX,
            -1);
    }

    signals = weechat_string_split (*all_signals, ",", NULL,
                                    WEECHAT_STRING_SPLIT_STRIP_LEFT
                                    | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                    | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                    0, &count);
    if (signals)
    {
        signals_list = weechat_arraylist_new (
            32, 1, 0,
            &buflist_config_compare_signals,
            NULL, NULL, NULL);
        if (signals_list)
        {
            for (i = 0; i < count; i++)
            {
                weechat_arraylist_add (signals_list, signals[i]);
            }
            list_size = weechat_arraylist_size (signals_list);
            buflist_config_signals_refresh = malloc (
                list_size * sizeof (*buflist_config_signals_refresh));
            if (buflist_config_signals_refresh)
            {
                buflist_config_num_signals_refresh = list_size;
                for (i = 0; i < list_size; i++)
                {
                    buflist_config_signals_refresh[i] = weechat_hook_signal (
                        weechat_arraylist_get (signals_list, i),
                        &buflist_config_signal_buffer_cb, NULL, NULL);
                }
                if (weechat_buflist_plugin->debug >= 1)
                {
                    weechat_printf (NULL, _("%s: %d signals hooked"),
                                    BUFLIST_PLUGIN_NAME, list_size);
                }
            }
            weechat_arraylist_free (signals_list);
        }
        weechat_string_free_split (signals);
    }

    weechat_string_dyn_free (all_signals, 1);
}

/*
 * Callback for changes on option "buflist.look.enabled".
 */

void
buflist_config_change_enabled (const void *pointer, void *data,
                               struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    buflist_config_free_signals_refresh ();

    if (weechat_config_boolean (buflist_config_look_enabled))
    {
        /* buflist enabled */
        buflist_config_hook_signals_refresh ();
        weechat_command (NULL, "/mute /bar show buflist");
        buflist_bar_item_update (0);
    }
    else
    {
        /* buflist disabled */
        weechat_command (NULL, "/mute /bar hide buflist");
        buflist_bar_item_update (1);
    }
}

/*
 * Callback for changes on option "buflist.look.sort".
 */

void
buflist_config_change_sort (const void *pointer, void *data,
                            struct t_config_option *option)
{
    int i;
    struct t_hashtable *hashtable_pointers;
    char *sort;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    for (i = 0; i < BUFLIST_BAR_NUM_ITEMS; i++)
    {
        if (buflist_config_sort_fields[i])
        {
            weechat_string_free_split (buflist_config_sort_fields[i]);
            buflist_config_sort_fields[i] = NULL;
            buflist_config_sort_fields_count[i] = 0;
        }
    }

    hashtable_pointers = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_POINTER,
        NULL,
        NULL);
    if (!hashtable_pointers)
        return;

    for (i = 0; i < BUFLIST_BAR_NUM_ITEMS; i++)
    {
        weechat_hashtable_set (hashtable_pointers,
                               "bar_item", buflist_bar_item_buflist[i]);

        sort = weechat_string_eval_expression (
            weechat_config_string (buflist_config_look_sort),
            hashtable_pointers,
            NULL,
            NULL);

        buflist_config_sort_fields[i] = weechat_string_split (
            (sort) ? sort : "",
            ",",
            NULL,
            WEECHAT_STRING_SPLIT_STRIP_LEFT
            | WEECHAT_STRING_SPLIT_STRIP_RIGHT
            | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
            0,
            &buflist_config_sort_fields_count[i]);

        if (sort)
            free (sort);
    }

    weechat_hashtable_free (hashtable_pointers);

    buflist_bar_item_update (0);
}

/*
 * Callback for changes on option "buflist.look.signals_refresh".
 */

void
buflist_config_change_signals_refresh (const void *pointer, void *data,
                                       struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    buflist_config_free_signals_refresh ();
    buflist_config_hook_signals_refresh ();
}

/*
 * Callback for changes on option "buflist.look.nick_prefix".
 */

void
buflist_config_change_nick_prefix (const void *pointer, void *data,
                                   struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    buflist_config_change_signals_refresh (NULL, NULL, NULL);
    buflist_bar_item_update (0);
}

/*
 * Callback for changes on option "buflist.look.use_items".
 */

void
buflist_config_change_use_items (const void *pointer, void *data,
                                 struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    buflist_bar_item_update (2);
}

/*
 * Callback for changes on options needing bar item refresh.
 */

void
buflist_config_change_buflist (const void *pointer, void *data,
                               struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    buflist_bar_item_update (0);
}

/*
 * Replace formats like ${format_xxx} by evaluated form: ${eval:${format_xxx}}.
 *
 * Note: result must be freed after use.
 */

char *
buflist_config_add_eval_for_formats (const char *string)
{
    char *formats[] = { "format_buffer", "format_number", "indent",
                        "format_nick_prefix", "format_name",
                        "format_hotlist", "hotlist", "format_lag",
                        "color_hotlist", "format_tls_version", NULL };
    char *result, *tmp, format[512], format_eval[512];
    int i;

    result = strdup (string);
    for (i = 0; formats[i]; i++)
    {
        snprintf (format, sizeof (format),
                  "${%s}", formats[i]);
        snprintf (format_eval, sizeof (format_eval),
                  "${eval:${%s}}", formats[i]);
        tmp = weechat_string_replace (result, format, format_eval);
        free (result);
        result = tmp;
    }
    return result;
}

/*
 * Callback for changes on some format options.
 */

void
buflist_config_change_format (const void *pointer, void *data,
                              struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (buflist_config_format_buffer_eval)
        free (buflist_config_format_buffer_eval);
    buflist_config_format_buffer_eval = buflist_config_add_eval_for_formats (
        weechat_config_string (buflist_config_format_buffer));

    if (buflist_config_format_buffer_current_eval)
        free (buflist_config_format_buffer_current_eval);
    buflist_config_format_buffer_current_eval = buflist_config_add_eval_for_formats (
        weechat_config_string (buflist_config_format_buffer_current));

    if (buflist_config_format_hotlist_eval)
        free (buflist_config_format_hotlist_eval);
    buflist_config_format_hotlist_eval = buflist_config_add_eval_for_formats (
        weechat_config_string (buflist_config_format_hotlist));

    buflist_bar_item_update (0);
}

/*
 * Initializes buflist configuration file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
buflist_config_init ()
{
    buflist_config_file = weechat_config_new (
        BUFLIST_CONFIG_PRIO_NAME,
        &buflist_config_reload, NULL, NULL);
    if (!buflist_config_file)
        return 0;

    /* look */
    buflist_config_section_look = weechat_config_new_section (
        buflist_config_file, "look",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (buflist_config_section_look)
    {
        buflist_config_look_add_newline = weechat_config_new_option (
            buflist_config_file, buflist_config_section_look,
            "add_newline", "boolean",
            N_("add newline between the buffers displayed, so each buffer is "
               "displayed on a separate line (recommended); if disabled, "
               "newlines must be manually added in the formats with \"${\\n}\", "
               "and the mouse actions are not possible any more"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_buflist, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_look_auto_scroll = weechat_config_new_option (
            buflist_config_file, buflist_config_section_look,
            "auto_scroll", "integer",
            N_("automatically scroll the buflist bar to always see the current "
               "buffer (this works only with a bar on the left/right position "
               "with a \"vertical\" filling); this value is the percent number "
               "of lines displayed before the current buffer when scrolling "
               "(-1 = disable scroll); for example 50 means that after a scroll, "
               "the current buffer is at the middle of bar, 0 means on top of "
               "bar, 100 means at bottom of bar"),
            NULL, -1, 100, "50", NULL, 0,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_look_display_conditions = weechat_config_new_option (
            buflist_config_file, buflist_config_section_look,
            "display_conditions", "string",
            N_("conditions to display a buffer "
               "(note: content is evaluated, see /help buflist); for example "
               "to hide server buffers if they are merged with core buffer: "
               "\"${buffer.hidden}==0 && ((${type}!=server && "
               "${buffer.full_name}!=core.weechat) || ${buffer.active}==1)\""),
            NULL, 0, 0, "${buffer.hidden}==0", NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_buflist, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_look_enabled = weechat_config_new_option (
            buflist_config_file, buflist_config_section_look,
            "enabled", "boolean",
            N_("enable buflist; it is recommended to use this option instead of "
               "just hiding the bar because it also removes some internal hooks "
               "that are not needed any more when the bar is hidden; you can "
               "also use the command \"/buflist toggle\" or use the default key "
               "alt+shift+b"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_enabled, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_look_mouse_jump_visited_buffer = weechat_config_new_option (
            buflist_config_file, buflist_config_section_look,
            "mouse_jump_visited_buffer", "boolean",
            N_("if enabled, clicks with left/right buttons on the line with "
               "current buffer jump to previous/next visited buffer"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_look_mouse_move_buffer = weechat_config_new_option (
            buflist_config_file, buflist_config_section_look,
            "mouse_move_buffer", "boolean",
            N_("if enabled, mouse gestures (drag & drop) move buffers in list"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_look_mouse_wheel = weechat_config_new_option (
            buflist_config_file, buflist_config_section_look,
            "mouse_wheel", "boolean",
            N_("if enabled, mouse wheel up/down actions jump to previous/next "
               "buffer in list"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_look_nick_prefix = weechat_config_new_option (
            buflist_config_file, buflist_config_section_look,
            "nick_prefix", "boolean",
            N_("get the nick prefix and its color from nicklist so that "
               "${nick_prefix} can be used in format; this can be slow on "
               "buffers with lot of nicks in nicklist, so this option is "
               "disabled by default"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_nick_prefix, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_look_nick_prefix_empty = weechat_config_new_option (
            buflist_config_file, buflist_config_section_look,
            "nick_prefix_empty", "boolean",
            N_("when the nick prefix is enabled, display a space instead if "
               "there is no nick prefix on the buffer"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_buflist, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_look_signals_refresh = weechat_config_new_option (
            buflist_config_file, buflist_config_section_look,
            "signals_refresh", "string",
            N_("comma-separated list of extra signals that are hooked and "
               "trigger the refresh of buffers list; this can be useful if "
               "some custom variables are used in formats and need specific "
               "refresh"),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_signals_refresh, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_look_sort = weechat_config_new_option (
            buflist_config_file, buflist_config_section_look,
            "sort", "string",
            N_("comma-separated list of fields to sort buffers; each field is "
               "a hdata variable of buffer (\"var\"), a hdata variable of "
               "IRC server (\"irc_server.var\") or a hdata variable of "
               "IRC channel (\"irc_channel.var\"); "
               "char \"-\" can be used before field to reverse order, "
               "char \"~\" can be used to do a case insensitive comparison; "
               "examples: \"-~short_name\" for case insensitive and reverse "
               "sort on buffer short name, "
               "\"-hotlist.priority,hotlist.creation_time.tv_sec,number,-active\" "
               "for sort like the hotlist then by buffer number for buffers "
               "without activity "
               "(note: the content is evaluated, before being split into "
               "fields, but at that time \"bar_item\" is the only variable "
               "that can be used, to distinguish between different buflist "
               "items, for example \"${bar_item.name}\")"),
            NULL, 0, 0, "number,-active", NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_sort, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_look_use_items = weechat_config_new_option (
            buflist_config_file, buflist_config_section_look,
            "use_items", "integer",
            N_("number of buflist bar items that can be used; the item names "
               "are: \"buflist\", \"buflist2\", \"buflist3\"; be careful, "
               "using more than one bar item slows down the display of "
               "buffers list"),
            NULL, 1, BUFLIST_BAR_NUM_ITEMS, "1", NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_use_items, NULL, NULL,
            NULL, NULL, NULL);
    }

    /* format */
    buflist_config_section_format = weechat_config_new_section (
        buflist_config_file, "format",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (buflist_config_section_format)
    {
        buflist_config_format_buffer = weechat_config_new_option (
            buflist_config_file, buflist_config_section_format,
            "buffer", "string",
            N_("format of each line with a buffer "
               "(note: content is evaluated, see /help buflist); "
               "example: standard format for bar item \"buflist\" and only the "
               "buffer number between square brackets for other bar items "
               "(\"buflist2\" and \"buflist3\"): "
               "\"${if:${bar_item.name}==buflist?${format_number}${indent}"
               "${format_nick_prefix}${color_hotlist}${format_name}:"
               "[${number}]}\""),
            NULL, 0, 0,
            "${format_number}${indent}${format_nick_prefix}${color_hotlist}"
            "${format_name}",
            NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_format, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_format_buffer_current = weechat_config_new_option (
            buflist_config_file, buflist_config_section_format,
            "buffer_current", "string",
            N_("format for the line with current buffer "
               "(note: content is evaluated, see /help buflist)"),
            NULL, 0, 0, "${color:,17}${format_buffer}", NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_format, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_format_hotlist = weechat_config_new_option (
            buflist_config_file, buflist_config_section_format,
            "hotlist", "string",
            N_("format for hotlist "
               "(note: content is evaluated, see /help buflist)"),
            NULL, 0, 0,
            " ${color:green}(${hotlist}${color:green})",
            NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_format, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_format_hotlist_level[3] = weechat_config_new_option (
            buflist_config_file, buflist_config_section_format,
            "hotlist_highlight", "string",
            N_("format for a buffer with hotlist level \"highlight\" "
               "(note: content is evaluated, see /help buflist)"),
            NULL, 0, 0, "${color:magenta}", NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_buflist, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_format_hotlist_level[0] = weechat_config_new_option (
            buflist_config_file, buflist_config_section_format,
            "hotlist_low", "string",
            N_("format for a buffer with hotlist level \"low\" "
               "(note: content is evaluated, see /help buflist)"),
            NULL, 0, 0, "${color:white}", NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_buflist, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_format_hotlist_level[1] = weechat_config_new_option (
            buflist_config_file, buflist_config_section_format,
            "hotlist_message", "string",
            N_("format for a buffer with hotlist level \"message\" "
               "(note: content is evaluated, see /help buflist)"),
            NULL, 0, 0, "${color:brown}", NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_buflist, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_format_hotlist_level_none = weechat_config_new_option (
            buflist_config_file, buflist_config_section_format,
            "hotlist_none", "string",
            N_("format for a buffer not in hotlist "
               "(note: content is evaluated, see /help buflist)"),
            NULL, 0, 0, "${color:default}", NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_buflist, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_format_hotlist_level[2] = weechat_config_new_option (
            buflist_config_file, buflist_config_section_format,
            "hotlist_private", "string",
            N_("format for a buffer with hotlist level \"private\" "
               "(note: content is evaluated, see /help buflist)"),
            NULL, 0, 0, "${color:green}", NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_buflist, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_format_hotlist_separator = weechat_config_new_option (
            buflist_config_file, buflist_config_section_format,
            "hotlist_separator", "string",
            N_("separator for counts in hotlist "
               "(note: content is evaluated, see /help buflist)"),
            NULL, 0, 0, "${color:default},", NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_buflist, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_format_indent = weechat_config_new_option (
            buflist_config_file, buflist_config_section_format,
            "indent", "string",
            N_("string displayed to indent channel and private buffers "
               "(note: content is evaluated, see /help buflist)"),
            NULL, 0, 0, "  ", NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_buflist, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_format_lag = weechat_config_new_option (
            buflist_config_file, buflist_config_section_format,
            "lag", "string",
            N_("format for lag on an IRC server buffer "
               "(note: content is evaluated, see /help buflist)"),
            NULL, 0, 0,
            " ${color:green}[${color:brown}${lag}${color:green}]",
            NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_buflist, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_format_name = weechat_config_new_option (
            buflist_config_file, buflist_config_section_format,
            "name", "string",
            N_("format for buffer name "
               "(note: content is evaluated, see /help buflist)"),
            NULL, 0, 0, "${name}", NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_buflist, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_format_nick_prefix = weechat_config_new_option (
            buflist_config_file, buflist_config_section_format,
            "nick_prefix", "string",
            N_("format for nick prefix on a channel "
               "(note: content is evaluated, see /help buflist)"),
            NULL, 0, 0, "${color_nick_prefix}${nick_prefix}", NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_buflist, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_format_number = weechat_config_new_option (
            buflist_config_file, buflist_config_section_format,
            "number", "string",
            N_("format for buffer number, ${number} is the indented number "
               "(note: content is evaluated, see /help buflist)"),
            NULL, 0, 0,
            "${color:green}${number}${if:${number_displayed}?.: }",
            NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_buflist, NULL, NULL,
            NULL, NULL, NULL);
        buflist_config_format_tls_version = weechat_config_new_option (
            buflist_config_file, buflist_config_section_format,
            "tls_version", "string",
            N_("format for TLS version on an IRC server buffer "
               "(note: content is evaluated, see /help buflist)"),
            NULL, 0, 0,
            " ${color:default}(${if:${tls_version}==TLS1.3?${color:green}:"
            "${if:${tls_version}==TLS1.2?${color:yellow}:${color:red}}}"
            "${translate:${tls_version}}${color:default})",
            NULL, 0,
            NULL, NULL, NULL,
            &buflist_config_change_buflist, NULL, NULL,
            NULL, NULL, NULL);
    }

    return 1;
}

/*
 * Reads buflist configuration file.
 */

int
buflist_config_read ()
{
    int rc;

    rc = weechat_config_read (buflist_config_file);

    if (rc == WEECHAT_CONFIG_READ_OK)
    {
        buflist_config_change_sort (NULL, NULL, NULL);
        buflist_config_change_signals_refresh (NULL, NULL, NULL);
        buflist_config_change_format (NULL, NULL, NULL);
    }

    return rc;
}

/*
 * Writes buflist configuration file.
 */

int
buflist_config_write ()
{
    return weechat_config_write (buflist_config_file);
}

/*
 * Frees buflist configuration.
 */

void
buflist_config_free ()
{
    int i;

    weechat_config_free (buflist_config_file);

    if (buflist_config_signals_refresh)
        buflist_config_free_signals_refresh ();

    for (i = 0; i < BUFLIST_BAR_NUM_ITEMS; i++)
    {
        if (buflist_config_sort_fields[i])
        {
            weechat_string_free_split (buflist_config_sort_fields[i]);
            buflist_config_sort_fields[i] = NULL;
            buflist_config_sort_fields_count[i] = 0;
        }
    }

    if (buflist_config_format_buffer_eval)
        free (buflist_config_format_buffer_eval);
    if (buflist_config_format_buffer_current_eval)
        free (buflist_config_format_buffer_current_eval);
    if (buflist_config_format_hotlist_eval)
        free (buflist_config_format_hotlist_eval);
}
