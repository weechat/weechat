/*
 * typing-config.c - typing configuration options (file typing.conf)
 *
 * Copyright (C) 2021-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include <limits.h>

#include "../weechat-plugin.h"
#include "typing.h"
#include "typing-config.h"
#include "typing-bar-item.h"


struct t_config_file *typing_config_file = NULL;

/* sections */

struct t_config_section *typing_config_section_look = NULL;

/* typing config, look section */

struct t_config_option *typing_config_look_delay_purge_paused = NULL;
struct t_config_option *typing_config_look_delay_purge_typing = NULL;
struct t_config_option *typing_config_look_delay_set_paused = NULL;
struct t_config_option *typing_config_look_enabled_nicks = NULL;
struct t_config_option *typing_config_look_enabled_self = NULL;
struct t_config_option *typing_config_look_input_min_chars = NULL;
struct t_config_option *typing_config_look_item_max_length = NULL;


/*
 * Reloads typing configuration file.
 */

int
typing_config_reload (const void *pointer, void *data,
                      struct t_config_file *config_file)
{
    int rc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    rc = weechat_config_reload (config_file);

    typing_setup_hooks ();

    return rc;
}

/*
 * Callback for changes on options "typing.look.enabled_*".
 */

void
typing_config_change_enabled (const void *pointer, void *data,
                              struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    typing_setup_hooks ();
    weechat_bar_item_update (TYPING_BAR_ITEM_NAME);
}

/*
 * Callback for changes on options "typing.look.item_max_length".
 */

void
typing_config_change_item_max_length (const void *pointer, void *data,
                                      struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    weechat_bar_item_update (TYPING_BAR_ITEM_NAME);
}

/*
 * Initializes typing configuration file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
typing_config_init ()
{
    typing_config_file = weechat_config_new (
        TYPING_CONFIG_PRIO_NAME,
        &typing_config_reload, NULL, NULL);
    if (!typing_config_file)
        return 0;

    /* look */
    typing_config_section_look = weechat_config_new_section (
        typing_config_file, "look",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (typing_config_section_look)
    {
        typing_config_look_delay_purge_paused = weechat_config_new_option (
            typing_config_file, typing_config_section_look,
            "delay_purge_paused", "integer",
            N_("number of seconds after paused status has been set: if reached, "
               "the typing status is removed"),
            NULL, 1, INT_MAX, "30", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        typing_config_look_delay_purge_typing = weechat_config_new_option (
            typing_config_file, typing_config_section_look,
            "delay_purge_typing", "integer",
            N_("number of seconds after typing status has been set: if reached, "
               "the typing status is removed"),
            NULL, 1, INT_MAX, "6", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        typing_config_look_delay_set_paused = weechat_config_new_option (
            typing_config_file, typing_config_section_look,
            "delay_set_paused", "integer",
            N_("number of seconds after typing last char: if reached, the "
               "typing status becomes \"paused\" and no more typing signals "
               "are sent"),
            NULL, 1, INT_MAX, "10", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        typing_config_look_enabled_nicks = weechat_config_new_option (
            typing_config_file, typing_config_section_look,
            "enabled_nicks", "boolean",
            N_("typing enabled for other nicks (display typing info for nicks "
               "typing in the current buffer)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL,
            &typing_config_change_enabled, NULL, NULL,
            NULL, NULL, NULL);
        typing_config_look_enabled_self = weechat_config_new_option (
            typing_config_file, typing_config_section_look,
            "enabled_self", "boolean",
            N_("typing enabled for self messages (send typing info to other "
               "users)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL,
            &typing_config_change_enabled, NULL, NULL,
            NULL, NULL, NULL);
        typing_config_look_input_min_chars = weechat_config_new_option (
            typing_config_file, typing_config_section_look,
            "input_min_chars", "integer",
            N_("min number of chars in message to trigger send of typing "
               "signals"),
            NULL, 1, INT_MAX, "4", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        typing_config_look_item_max_length = weechat_config_new_option (
            typing_config_file, typing_config_section_look,
            "item_max_length", "integer",
            N_("max number of chars displayed in the bar item \"typing\" "
               "(0 = do not truncate content)"),
            NULL, 0, INT_MAX, "0", NULL, 0,
            NULL, NULL, NULL,
            &typing_config_change_item_max_length, NULL, NULL,
            NULL, NULL, NULL);
    }

    return 1;
}

/*
 * Reads typing configuration file.
 */

int
typing_config_read ()
{
    return weechat_config_read (typing_config_file);
}

/*
 * Writes typing configuration file.
 */

int
typing_config_write ()
{
    return weechat_config_write (typing_config_file);
}

/*
 * Frees typing configuration.
 */

void
typing_config_free ()
{
    weechat_config_free (typing_config_file);
}
