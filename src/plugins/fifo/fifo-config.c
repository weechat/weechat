/*
 * fifo-config.c - fifo configuration options (file fifo.conf)
 *
 * Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
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
#include "fifo.h"
#include "fifo-config.h"


struct t_config_file *fifo_config_file = NULL;

/* fifo config, file section */

struct t_config_option *fifo_config_file_enabled;
struct t_config_option *fifo_config_file_path;


/*
 * Callback for changes on option "enabled".
 */

void
fifo_config_change_file_enabled (const void *pointer, void *data,
                                 struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    fifo_remove ();

    if (weechat_config_boolean (fifo_config_file_enabled))
        fifo_create();
}

/*
 * Callback for changes on option "path".
 */

void
fifo_config_change_file_path (const void *pointer, void *data,
                              struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    fifo_quiet = 1;

    fifo_remove ();
    fifo_create ();

    fifo_quiet = 0;
}

/*
 * Initializes fifo configuration file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
fifo_config_init ()
{
    struct t_config_section *ptr_section;

    fifo_config_file = weechat_config_new (FIFO_CONFIG_NAME,
                                           NULL, NULL, NULL);
    if (!fifo_config_file)
        return 0;

    /* file */
    ptr_section = weechat_config_new_section (fifo_config_file, "file",
                                              0, 0,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (fifo_config_file);
        return 0;
    }

    fifo_config_file_enabled = weechat_config_new_option (
        fifo_config_file, ptr_section,
        "enabled", "boolean",
        N_("enable FIFO pipe"),
        NULL, 0, 0, "on", NULL, 0,
        NULL, NULL, NULL,
        &fifo_config_change_file_enabled, NULL, NULL,
        NULL, NULL, NULL);
    fifo_config_file_path = weechat_config_new_option (
        fifo_config_file, ptr_section,
        "path", "string",
        N_("path for FIFO file; \"%h\" at beginning of string is "
           "replaced by WeeChat home (\"~/.weechat\" by default); "
           "WeeChat PID can be used in path with ${info:pid} "
           "(note: content is evaluated, see /help eval)"),
        NULL, 0, 0, "%h/weechat_fifo", NULL, 0,
        NULL, NULL, NULL,
        fifo_config_change_file_path, NULL, NULL,
        NULL, NULL, NULL);

    return 1;
}

/*
 * Reads fifo configuration file.
 */

int
fifo_config_read ()
{
    return weechat_config_read (fifo_config_file);
}

/*
 * Writes fifo configuration file.
 */

int
fifo_config_write ()
{
    return weechat_config_write (fifo_config_file);
}

/*
 * Frees fifo configuration.
 */

void
fifo_config_free ()
{
    weechat_config_free (fifo_config_file);
}
