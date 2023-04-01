/*
 * exec-config.c - exec configuration options (file exec.conf)
 *
 * Copyright (C) 2014-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include "exec.h"
#include "exec-config.h"


struct t_config_file *exec_config_file = NULL;

/* sections */

struct t_config_section *exec_config_section_command = NULL;
struct t_config_section *exec_config_section_color = NULL;

/* exec config, command section */

struct t_config_option *exec_config_command_default_options = NULL;
struct t_config_option *exec_config_command_purge_delay = NULL;
struct t_config_option *exec_config_command_shell = NULL;

/* exec config, color section */

struct t_config_option *exec_config_color_flag_finished = NULL;
struct t_config_option *exec_config_color_flag_running = NULL;

char **exec_config_cmd_options = NULL;
int exec_config_cmd_num_options = 0;


/*
 * Callback for changes on option "exec.command.default_options".
 */

void
exec_config_change_command_default_options (const void *pointer, void *data,
                                            struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (exec_config_cmd_options)
        weechat_string_free_split (exec_config_cmd_options);

    exec_config_cmd_options = weechat_string_split (
        weechat_config_string (exec_config_command_default_options),
        " ",
        NULL,
        WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
        0,
        &exec_config_cmd_num_options);
}

/*
 * Reloads exec configuration file.
 */

int
exec_config_reload_cb (const void *pointer, void *data,
                       struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    return weechat_config_reload (config_file);
}

/*
 * Initializes exec configuration file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
exec_config_init ()
{
    exec_config_file = weechat_config_new (EXEC_CONFIG_PRIO_NAME,
                                           &exec_config_reload_cb, NULL, NULL);
    if (!exec_config_file)
        return 0;

    /* command */
    exec_config_section_command = weechat_config_new_section (
        exec_config_file, "command",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (exec_config_section_command)
    {
        exec_config_command_default_options = weechat_config_new_option (
            exec_config_file, exec_config_section_command,
            "default_options", "string",
            N_("default options for command /exec (see /help exec); example: "
               "\"-nosh -bg\" to run all commands in background (no output), "
               "and without using the shell"),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &exec_config_change_command_default_options, NULL, NULL,
            NULL, NULL, NULL);
        exec_config_command_purge_delay = weechat_config_new_option (
            exec_config_file, exec_config_section_command,
            "purge_delay", "integer",
            N_("delay for purging finished commands (in seconds, 0 = purge "
               "commands immediately, -1 = never purge)"),
            NULL, -1, 36000 * 24 * 30, "0", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        exec_config_command_shell = weechat_config_new_option (
            exec_config_file, exec_config_section_command,
            "shell", "string",
            N_("shell to use with command \"/exec -sh\"; it can be just the "
               "name of shell if it is in PATH (for example \"bash\") or the "
               "absolute path to the shell (for example \"/bin/bash\"); if "
               "value is empty, \"sh\" is used (note: content is evaluated, "
               "see /help eval)"),
            NULL, 0, 0, "${env:SHELL}", NULL, 0,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
    }

    /* color */
    exec_config_section_color = weechat_config_new_section (
        exec_config_file, "color",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (exec_config_section_color)
    {
        exec_config_color_flag_finished = weechat_config_new_option (
            exec_config_file, exec_config_section_color,
            "flag_finished", "color",
            N_("text color for a finished command flag in list of commands"),
            NULL, 0, 0, "lightred", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        exec_config_color_flag_running = weechat_config_new_option (
            exec_config_file, exec_config_section_color,
            "flag_running", "color",
            N_("text color for a running command flag in list of commands"),
            NULL, 0, 0, "lightgreen", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    return 1;
}

/*
 * Reads exec configuration file.
 */

int
exec_config_read ()
{
    return weechat_config_read (exec_config_file);
}

/*
 * Writes exec configuration file.
 */

int
exec_config_write ()
{
    return weechat_config_write (exec_config_file);
}

/*
 * Frees exec configuration.
 */

void
exec_config_free ()
{
    weechat_config_free (exec_config_file);

    if (exec_config_cmd_options)
    {
        weechat_string_free_split (exec_config_cmd_options);
        exec_config_cmd_options = NULL;
        exec_config_cmd_num_options = 0;
    }
}
