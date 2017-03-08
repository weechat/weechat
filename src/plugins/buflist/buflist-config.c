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

/* buflist config, format section */

struct t_config_option *buflist_config_format_buffer;
struct t_config_option *buflist_config_format_buffer_current;
struct t_config_option *buflist_config_format_hotlist[4];
struct t_config_option *buflist_config_format_hotlist_none;


/*
 * Callback for changes on option "irc.network.lag_min_show".
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
        N_("format of each line with a buffer"),
        NULL, 0, 0,
        "${color:green}${number}.${indent}${color_hotlist}${name}",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_buflist, NULL, NULL,
        NULL, NULL, NULL);
    buflist_config_format_buffer_current = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "buffer_current", "string",
        N_("format for the line with current buffer"),
        NULL, 0, 0,
        "${color:lightgreen,blue}${number}.${indent}${color_hotlist}${name}",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_buflist, NULL, NULL,
        NULL, NULL, NULL);
    buflist_config_format_hotlist[0] = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "hotlist_low", "string",
        N_("format for a buffer with hotlist level \"low\""),
        NULL, 0, 0,
        "${color:white}",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_buflist, NULL, NULL,
        NULL, NULL, NULL);
    buflist_config_format_hotlist[1] = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "hotlist_message", "string",
        N_("format for a buffer with hotlist level \"message\""),
        NULL, 0, 0,
        "${color:brown}",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_buflist, NULL, NULL,
        NULL, NULL, NULL);
    buflist_config_format_hotlist[2] = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "hotlist_private", "string",
        N_("format for a buffer with hotlist level \"private\""),
        NULL, 0, 0,
        "${color:green}",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_buflist, NULL, NULL,
        NULL, NULL, NULL);
    buflist_config_format_hotlist[3] = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "hotlist_highlight", "string",
        N_("format for a buffer with hotlist level \"highlight\""),
        NULL, 0, 0,
        "${color:magenta}",
        NULL, 0,
        NULL, NULL, NULL,
        &buflist_config_change_buflist, NULL, NULL,
        NULL, NULL, NULL);
    buflist_config_format_hotlist_none = weechat_config_new_option (
        buflist_config_file, ptr_section,
        "hotlist_none", "string",
        N_("format for a buffer not in hotlist"),
        NULL, 0, 0,
        "${color:default}",
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
    return weechat_config_read (buflist_config_file);
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
}
