/*
 * buflist-config.c - buflist configuration options (file buflist.conf)
 *
 * Copyright (C) 2003-2017 Sébastien Helleu <flashcode@flashtux.org>
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

#include "../weechat-plugin.h"
#include "buflist.h"
#include "buflist-config.h"
#include "buflist-bar-item.h"


struct t_config_file *buflist_config_file = NULL;

/* buflist config, look section */

struct t_config_option *buflist_config_look_display_conditions;
struct t_config_option *buflist_config_look_signals_refresh;
struct t_config_option *buflist_config_look_sort;

/* buflist config, format section */

struct t_config_option *buflist_config_format_buffer;
struct t_config_option *buflist_config_format_buffer_current;
struct t_config_option *buflist_config_format_hotlist;
struct t_config_option *buflist_config_format_hotlist_level[4];
struct t_config_option *buflist_config_format_hotlist_level_none;
struct t_config_option *buflist_config_format_hotlist_separator;
struct t_config_option *buflist_config_format_lag;
struct t_config_option *buflist_config_format_number;

struct t_hook **buflist_config_signals_refresh = NULL;
int buflist_config_num_signals_refresh = 0;
char **buflist_config_sort_fields = NULL;
int buflist_config_sort_fields_count = 0;


/*
 * Callback for changes on option "buflist.look.sort".
 */

void
buflist_config_change_sort (const void *pointer, void *data,
                            struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (buflist_config_sort_fields)
        weechat_string_free_split (buflist_config_sort_fields);

    buflist_config_sort_fields = weechat_string_split (
        weechat_config_string (buflist_config_look_sort),
        ",", 0, 0, &buflist_config_sort_fields_count);

    weechat_bar_item_update (BUFLIST_BAR_ITEM_NAME);
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

    weechat_bar_item_update (BUFLIST_BAR_ITEM_NAME);

    return WEECHAT_RC_OK;
}

/*
 * Callback for changes on option "buflist.look.signals_refresh".
 */

void
buflist_config_change_signals_refresh (const void *pointer, void *data,
                                       struct t_config_option *option)
{
    char **signals;
    int count, i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (buflist_config_signals_refresh)
        buflist_config_free_signals_refresh ();

    signals = weechat_string_split (
        weechat_config_string (buflist_config_look_signals_refresh),
        ",", 0, 0, &count);
    if (signals && (count > 0))
    {
        buflist_config_signals_refresh = malloc (
            count * sizeof (*buflist_config_signals_refresh));
        if (buflist_config_signals_refresh)
        {
            buflist_config_num_signals_refresh = count;
            for (i = 0; i < count; i++)
            {
                buflist_config_signals_refresh[i] = weechat_hook_signal (
                    signals[i], &buflist_config_signal_buffer_cb, NULL, NULL);
            }
        }
    }
    if (signals)
        weechat_string_free_split (signals);
}

/*
 * Callback for changes on format options.
 */

void
buflist_config_change_buflist (const void *pointer, void *data,
                               struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    weechat_bar_item_update (BUFLIST_BAR_ITEM_NAME);
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
    struct t_config_section *ptr_section;

    buflist_config_file = weechat_config_new (BUFLIST_CONFIG_NAME,
                                              NULL, NULL, NULL);
    if (!buflist_config_file)
        return 0;

    /* look */
    ptr_section = weechat_config_new_section (buflist_config_file, "look",
                                              0, 0,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (buflist_config_file);
        return 0;
    }

    buflist_config_look_display_conditions = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "display_conditions", "string",
        N_("conditions to display a buffer "
           "(note: content is evaluated, see /help buflist)"),
        NULL, 0, 0,
        "${buffer.hidden}==0",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_signals_refresh, NULL, NULL,
        NULL, NULL, NULL);
    buflist_config_look_signals_refresh = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "signals_refresh", "string",
        N_("comma-separated list of signals that are hooked and trigger the "
           "refresh of buffers list"),
        NULL, 0, 0,
        "buffer_opened,buffer_closed,buffer_merged,buffer_unmerged,"
        "buffer_moved,buffer_renamed,buffer_switch,buffer_hidden,"
        "buffer_unhidden,buffer_localvar_added,buffer_localvar_changed,"
        "window_switch,hotlist_changed",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_signals_refresh, NULL, NULL,
        NULL, NULL, NULL);
    buflist_config_look_sort = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "sort", "string",
        N_("comma-separated list of fields to sort buffers; each field is "
           "a hdata variable of buffer; char \"-\" can be used before field "
           "to reverse order"),
        NULL, 0, 0,
        "number,-active",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_sort, NULL, NULL,
        NULL, NULL, NULL);

    /* format */
    ptr_section = weechat_config_new_section (buflist_config_file, "format",
                                              0, 0,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (buflist_config_file);
        return 0;
    }

    buflist_config_format_buffer = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "buffer", "string",
        N_("format of each line with a buffer "
           "(note: content is evaluated, see /help buflist)"),
        NULL, 0, 0,
        "${format_number}${indent}${color_hotlist}${name}",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_buflist, NULL, NULL,
        NULL, NULL, NULL);
    buflist_config_format_buffer_current = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "buffer_current", "string",
        N_("format for the line with current buffer "
           "(note: content is evaluated, see /help buflist)"),
        NULL, 0, 0,
        "${color:lightgreen,blue}${format_buffer}",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_buflist, NULL, NULL,
        NULL, NULL, NULL);
    buflist_config_format_hotlist = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "hotlist", "string",
        N_("format for hotlist "
           "(note: content is evaluated, see /help buflist)"),
        NULL, 0, 0,
        " ${color:green}(${hotlist}${color:green})",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_buflist, NULL, NULL,
        NULL, NULL, NULL);
    buflist_config_format_hotlist_level[0] = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "hotlist_low", "string",
        N_("format for a buffer with hotlist level \"low\" "
           "(note: content is evaluated, see /help buflist)"),
        NULL, 0, 0,
        "${color:white}",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_buflist, NULL, NULL,
        NULL, NULL, NULL);
    buflist_config_format_hotlist_level[1] = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "hotlist_message", "string",
        N_("format for a buffer with hotlist level \"message\" "
           "(note: content is evaluated, see /help buflist)"),
        NULL, 0, 0,
        "${color:brown}",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_buflist, NULL, NULL,
        NULL, NULL, NULL);
    buflist_config_format_hotlist_level[2] = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "hotlist_private", "string",
        N_("format for a buffer with hotlist level \"private\" "
           "(note: content is evaluated, see /help buflist)"),
        NULL, 0, 0,
        "${color:green}",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_buflist, NULL, NULL,
        NULL, NULL, NULL);
    buflist_config_format_hotlist_level[3] = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "hotlist_highlight", "string",
        N_("format for a buffer with hotlist level \"highlight\" "
           "(note: content is evaluated, see /help buflist)"),
        NULL, 0, 0,
        "${color:magenta}",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_buflist, NULL, NULL,
        NULL, NULL, NULL);
    buflist_config_format_hotlist_level_none = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "hotlist_none", "string",
        N_("format for a buffer not in hotlist "
           "(note: content is evaluated, see /help buflist)"),
        NULL, 0, 0,
        "${color:default}",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_buflist, NULL, NULL,
        NULL, NULL, NULL);
    buflist_config_format_hotlist_separator = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "hotlist_separator", "string",
        N_("separator for counts in hotlist "
           "(note: content is evaluated, see /help buflist)"),
        NULL, 0, 0,
        "${color:default},",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_buflist, NULL, NULL,
        NULL, NULL, NULL);
    buflist_config_format_lag = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "lag", "string",
        N_("format for lag on an IRC server buffer "
           "(note: content is evaluated, see /help buflist)"),
        NULL, 0, 0,
        " ${color:green}[${color:brown}${lag}${color:green}]",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_buflist, NULL, NULL,
        NULL, NULL, NULL);
    buflist_config_format_number = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "number", "string",
        N_("format for buffer number, ${number} is the indented number "
           "(note: content is evaluated, see /help buflist)"),
        NULL, 0, 0,
        "${color:green}${number}${if:${number_displayed}?.: }",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_buflist, NULL, NULL,
        NULL, NULL, NULL);

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
    weechat_config_free (buflist_config_file);

    if (buflist_config_signals_refresh)
        buflist_config_free_signals_refresh ();

    if (buflist_config_sort_fields)
    {
        weechat_string_free_split (buflist_config_sort_fields);
        buflist_config_sort_fields = NULL;
        buflist_config_sort_fields_count = 0;
    }
}
