/*
 * typing-config.c - typing configuration options (file typing.conf)
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

#include "../weechat-plugin.h"
#include "typing.h"
#include "typing-config.h"


struct t_config_file *typing_config_file = NULL;
struct t_config_section *typing_config_section_cmd = NULL;
struct t_config_section *typing_config_section_completion = NULL;

/* typing config, look section */

struct t_config_option *typing_config_look_enabled;
struct t_config_option *typing_config_look_delay_pause;


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
 * Callback for changes on option "typing.look.enabled".
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
    struct t_config_section *ptr_section;

    typing_config_file = weechat_config_new (
        TYPING_CONFIG_NAME,
        &typing_config_reload, NULL, NULL);
    if (!typing_config_file)
        return 0;

    /* look */
    ptr_section = weechat_config_new_section (typing_config_file, "look",
                                              0, 0,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (typing_config_file);
        typing_config_file = NULL;
        return 0;
    }

    typing_config_look_enabled = weechat_config_new_option (
        typing_config_file, ptr_section,
        "enabled", "boolean",
        N_("typing enabled"),
        NULL, 0, 0, "on", NULL, 0,
        NULL, NULL, NULL,
        &typing_config_change_enabled, NULL, NULL,
        NULL, NULL, NULL);
    typing_config_look_delay_pause = weechat_config_new_option (
        typing_config_file, ptr_section,
        "delay_pause", "integer",
        N_("number of seconds after typing last char: if reached, the typing "
           "status becomes \"paused\" and no more typing signals are sent"),
        NULL, 1, 3600, "10", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

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
