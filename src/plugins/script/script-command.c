/*
 * script-command.c - script command
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <string.h>
#include <stdio.h>

#include "../weechat-plugin.h"
#include "script.h"
#include "script-command.h"
#include "script-action.h"
#include "script-buffer.h"
#include "script-config.h"
#include "script-repo.h"


/*
 * Runs an action.
 */

void
script_command_action (struct t_gui_buffer *buffer,
                       const char *action, const char *arguments,
                       int need_repository, int error_repository)
{
    struct t_script_repo *ptr_script;
    char str_action[4096];
    long value;
    char *error;
    int quiet;

    if (arguments)
    {
        /* action with arguments on command line */
        quiet = 0;
        if (strncmp (arguments, "-q ", 3) == 0)
        {
            quiet = 1;
            arguments += 3;
            while (arguments[0] == ' ')
            {
                arguments++;
            }
        }
        error = NULL;
        value = strtol (arguments, &error, 10);
        if (error && !error[0])
        {
            ptr_script = script_repo_search_displayed_by_number (value);
            if (ptr_script)
            {
                snprintf (str_action, sizeof (str_action),
                          "%s%s %s",
                          (quiet) ? "-q " : "",
                          action,
                          ptr_script->name_with_extension);
                script_action_schedule (buffer, str_action, need_repository,
                                        error_repository, quiet);
            }
        }
        else
        {
            snprintf (str_action, sizeof (str_action),
                      "%s%s %s",
                      (quiet) ? "-q " : "",
                      action,
                      arguments);
            script_action_schedule (buffer, str_action, need_repository,
                                    error_repository, quiet);
        }
    }
    else if (script_buffer && (buffer == script_buffer))
    {
        /* action on current line of script buffer */
        if (script_buffer_detail_script
            && ((weechat_strcmp (action, "show") == 0)
                || (weechat_strcmp (action, "showdiff") == 0)))
        {
            /* if detail on script is displayed, back to list */
            snprintf (str_action, sizeof (str_action),
                      "-q %s",
                      action);
            script_action_schedule (buffer, str_action, need_repository,
                                    error_repository, 1);
        }
        else
        {
            /* if list is displayed, execute action on script */
            if (!script_buffer_detail_script)
            {
                ptr_script = script_repo_search_displayed_by_number (script_buffer_selected_line);
                if (ptr_script)
                {
                    snprintf (str_action, sizeof (str_action),
                              "-q %s %s",
                              action,
                              ptr_script->name_with_extension);
                    script_action_schedule (buffer, str_action, need_repository,
                                            error_repository, 1);
                }
            }
        }
    }
}

/*
 * Callback for command "/script": manages scripts.
 */

int
script_command_script (const void *pointer, void *data,
                       struct t_gui_buffer *buffer, int argc,
                       char **argv, char **argv_eol)
{
    char *error, command[128];
    long value;
    int line;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (argc == 1)
    {
        script_action_schedule (buffer, "buffer", 1, 1, 0);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "search") == 0)
    {
        if (scripts_repo)
            script_repo_filter_scripts ((argc > 2) ? argv_eol[2] : NULL);
        else
            script_repo_set_filter ((argc > 2) ? argv_eol[2] : NULL);
        script_action_schedule (buffer, "buffer", 1, 1, 0);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "enable") == 0)
    {
        if (!weechat_config_boolean (script_config_scripts_download_enabled))
        {
            weechat_config_option_set (script_config_scripts_download_enabled, "on", 1);
            weechat_printf (NULL,
                            _("%s: download of scripts enabled"),
                            SCRIPT_PLUGIN_NAME);
        }
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "list") == 0)
    {
        script_action_schedule (buffer, argv_eol[1], 1, 0, 0);
        return WEECHAT_RC_OK;
    }

    if ((weechat_strcmp (argv[1], "load") == 0)
        || (weechat_strcmp (argv[1], "unload") == 0)
        || (weechat_strcmp (argv[1], "reload") == 0)
        || (weechat_strcmp (argv[1], "autoload") == 0)
        || (weechat_strcmp (argv[1], "noautoload") == 0)
        || (weechat_strcmp (argv[1], "toggleautoload") == 0))
    {
        script_command_action (buffer,
                               argv[1],
                               (argc > 2) ? argv_eol[2] : NULL,
                               0,
                               0);
        return WEECHAT_RC_OK;
    }

    if ((weechat_strcmp (argv[1], "install") == 0)
        || (weechat_strcmp (argv[1], "remove") == 0)
        || (weechat_strcmp (argv[1], "installremove") == 0)
        || (weechat_strcmp (argv[1], "hold") == 0)
        || (weechat_strcmp (argv[1], "show") == 0)
        || (weechat_strcmp (argv[1], "showdiff") == 0))
    {
        script_command_action (buffer,
                               argv[1],
                               (argc > 2) ? argv_eol[2] : NULL,
                               1,
                               1);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "upgrade") == 0)
    {
        script_action_schedule (buffer, "upgrade", 1, 1, 0);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "update") == 0)
    {
        script_repo_file_update (0);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "-go") == 0)
    {
        if ((argc > 2) && script_buffer && !script_buffer_detail_script)
        {
            line = -1;
            if (weechat_strcmp (argv[2], "end") == 0)
            {
                line = script_repo_count_displayed - 1;
            }
            else
            {
                error = NULL;
                value = strtol (argv[2], &error, 10);
                if (error && !error[0])
                    line = value;
            }
            if (line >= 0)
            {
                script_buffer_set_current_line (line);
                script_buffer_check_line_outside_window ();
            }
        }
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "-up") == 0)
    {
        if (script_buffer)
        {
            value = 1;
            if (argc > 2)
            {
                error = NULL;
                value = strtol (argv[2], &error, 10);
                if (!error || error[0])
                    value = 1;
            }
            if (script_buffer_detail_script)
            {
                snprintf (command, sizeof (command),
                          "/window scroll -%d", (int)value);
                weechat_command (script_buffer, command);
            }
            else if ((script_buffer_selected_line >= 0)
                     && (script_repo_count_displayed > 0))
            {
                line = script_buffer_selected_line - value;
                if (line < 0)
                    line = 0;
                if (line != script_buffer_selected_line)
                {
                    script_buffer_set_current_line (line);
                    script_buffer_check_line_outside_window ();
                }
            }
        }
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "-down") == 0)
    {
        if (script_buffer)
        {
            value = 1;
            if (argc > 2)
            {
                error = NULL;
                value = strtol (argv[2], &error, 10);
                if (!error || error[0])
                    value = 1;
            }
            if (script_buffer_detail_script)
            {
                snprintf (command, sizeof (command),
                          "/window scroll +%d", (int)value);
                weechat_command (script_buffer, command);
            }
            else if ((script_buffer_selected_line >= 0)
                     && (script_repo_count_displayed > 0))
            {
                line = script_buffer_selected_line + value;
                if (line >= script_repo_count_displayed)
                    line = script_repo_count_displayed - 1;
                if (line != script_buffer_selected_line)
                {
                    script_buffer_set_current_line (line);
                    script_buffer_check_line_outside_window ();
                }
            }
        }
        return WEECHAT_RC_OK;
    }

    WEECHAT_COMMAND_ERROR;
}

/*
 * Hooks script command.
 */

void
script_command_init ()
{
    weechat_hook_command (
        "script",
        N_("WeeChat script manager"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("enable"
           " || list [-o|-ol|-i|-il]"
           " || search <text>"
           " || show <script>"
           " || load|unload|reload <script> [<script>...]"
           " || autoload|noautoload|toggleautoload <script> [<script>...]"
           " || install|remove|installremove|hold [-q] <script> [<script>...]"
           " || upgrade"
           " || update"
           " || -up|-down [<number>]"
           " || -go <line>|end"),
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[enable]: enable download of scripts "
               "(turn on option script.scripts.download_enabled)"),
            N_("raw[list]: list loaded scripts (all languages)"),
            N_("raw[-o]: send list of loaded scripts to buffer "
               "(string in English)"),
            N_("raw[-ol]: send list of loaded scripts to buffer "
               "(translated string)"),
            N_("raw[-i]: copy list of loaded scripts in command line (for "
               "sending to buffer) (string in English)"),
            N_("raw[-il]: copy list of loaded scripts in command line (for "
               "sending to buffer) (translated string)"),
            N_("raw[search]: search scripts by tags, language (python, "
               "perl, ...), filename extension (py, pl, ...) or text; result is "
               "displayed on scripts buffer"),
            N_("raw[show]: show detailed info about a script"),
            N_("raw[load]: load script(s)"),
            N_("raw[unload]: unload script(s)"),
            N_("raw[reload]: reload script(s)"),
            N_("raw[autoload]: autoload the script"),
            N_("raw[noautoload]: do not autoload the script"),
            N_("raw[toggleautoload]: toggle autoload"),
            N_("raw[install]: install/upgrade script(s)"),
            N_("raw[remove]: remove script(s)"),
            N_("raw[installremove]: install or remove script(s), depending on current "
               "state"),
            N_("raw[hold]: hold/unhold script(s) (a script held will not be "
               "upgraded any more and cannot be removed)"),
            N_("raw[-q]: quiet mode: do not display messages"),
            N_("raw[upgrade]: upgrade all installed scripts which are obsolete "
               "(new version available)"),
            N_("raw[update]: update local scripts cache"),
            N_("raw[-up]: move the selected line up by \"number\" lines"),
            N_("raw[-down]: move the selected line down by \"number\" lines"),
            N_("raw[-go]: select a line by number, first line number is 0 "
               "(\"end\" to select the last line)"),
            "",
            N_("Without argument, this command opens a buffer with list of scripts."),
            "",
            N_("On script buffer, the possible status for each script are:"),
            N_("  `*`: popular script"),
            N_("  `i`: installed"),
            N_("  `a`: autoloaded"),
            N_("  `H`: held"),
            N_("  `r`: running (loaded)"),
            N_("  `N`: obsolete (new version available)"),
            "",
            N_("In output of /script list, this additional status can be displayed:"),
            N_("  `?`: unknown script (can not be downloaded/updated)"),
            "",
            N_("In input of script buffer, word(s) are used to filter scripts "
               "on description, tags, ...). The input \"*\" removes the filter."),
            "",
            N_("For keys, input and mouse actions on the buffer, "
               "see key bindings in User's guide."),
            "",
            N_("Examples:"),
            AI("  /script search url"),
            AI("  /script install go.py urlserver.py"),
            AI("  /script remove go.py"),
            AI("  /script hold urlserver.py"),
            AI("  /script reload urlserver"),
            AI("  /script upgrade")),
        "enable"
        " || list -i|-il|-o|-ol"
        " || search %(script_tags)|%(script_languages)|%(script_extensions)"
        " || show %(script_scripts)"
        " || load %(script_files)|%*"
        " || unload %(python_script)|%(perl_script)|%(ruby_script)|"
        "%(tcl_script)|%(lua_script)|%(guile_script)|%(javascript_script)|"
        "%(php_script)|%*"
        " || reload %(python_script)|%(perl_script)|%(ruby_script)|"
        "%(tcl_script)|%(lua_script)|%(guile_script)|%(javascript_script)|"
        "%(php_script)|%*"
        " || autoload %(script_files)|%*"
        " || noautoload %(script_files)|%*"
        " || toggleautoload %(script_files)|%*"
        " || install %(script_scripts)|%*"
        " || remove %(script_scripts_installed)|%*"
        " || installremove %(script_scripts)|%*"
        " || hold %(script_scripts)|%*"
        " || update"
        " || upgrade"
        " || -up 1|2|3|4|5"
        " || -down 1|2|3|4|5"
        " || -go 0|end",
        &script_command_script, NULL, NULL);
}
