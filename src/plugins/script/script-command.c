/*
 * script-command.c - script command
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
                script_action_schedule (str_action, need_repository,
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
            script_action_schedule (str_action, need_repository,
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
            script_action_schedule (str_action, need_repository,
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
                    script_action_schedule (str_action, need_repository,
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
        script_action_schedule ("buffer", 1, 1, 0);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "go") == 0)
    {
        if ((argc > 2) && script_buffer && !script_buffer_detail_script)
        {
            error = NULL;
            value = strtol (argv[2], &error, 10);
            if (error && !error[0])
            {
                script_buffer_set_current_line (value);
            }
        }
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "search") == 0)
    {
        if (scripts_repo)
            script_repo_filter_scripts ((argc > 2) ? argv_eol[2] : NULL);
        else
            script_repo_set_filter ((argc > 2) ? argv_eol[2] : NULL);
        script_action_schedule ("buffer", 1, 1, 0);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "list") == 0)
    {
        script_action_schedule (argv_eol[1], 1, 0, 0);
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
        script_action_schedule ("upgrade", 1, 1, 0);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "update") == 0)
    {
        script_repo_file_update (0);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "up") == 0)
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

    if (weechat_strcmp (argv[1], "down") == 0)
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
        N_("list [-o|-ol|-i|-il]"
           " || search <text>"
           " || show <script>"
           " || load|unload|reload <script> [<script>...]"
           " || autoload|noautoload|toggleautoload <script> [<script>...]"
           " || install|remove|installremove|hold [-q] <script> [<script>...]"
           " || upgrade"
           " || update"),
        N_("          list: list loaded scripts (all languages)\n"
           "            -o: send list of loaded scripts to buffer "
           "(string in English)\n"
           "           -ol: send list of loaded scripts to buffer "
           "(translated string)\n"
           "            -i: copy list of loaded scripts in command line (for "
           "sending to buffer) (string in English)\n"
           "           -il: copy list of loaded scripts in command line (for "
           "sending to buffer) (translated string)\n"
           "        search: search scripts by tags, language (python, "
           "perl, ...), filename extension (py, pl, ...) or text; result is "
           "displayed on scripts buffer\n"
           "          show: show detailed info about a script\n"
           "          load: load script(s)\n"
           "        unload: unload script(s)\n"
           "        reload: reload script(s)\n"
           "      autoload: autoload the script\n"
           "    noautoload: do not autoload the script\n"
           "toggleautoload: toggle autoload\n"
           "       install: install/upgrade script(s)\n"
           "        remove: remove script(s)\n"
           " installremove: install or remove script(s), depending on current "
           "state\n"
           "          hold: hold/unhold script(s) (a script held will not be "
           "upgraded any more and cannot be removed)\n"
           "            -q: quiet mode: do not display messages\n"
           "       upgrade: upgrade all installed scripts which are obsolete "
           "(new version available)\n"
           "        update: update local scripts cache\n"
           "\n"
           "Without argument, this command opens a buffer with list of scripts.\n"
           "\n"
           "On script buffer, the possible status for each script are:\n"
           "  * i a H r N\n"
           "  | | | | | |\n"
           "  | | | | | obsolete (new version available)\n"
           "  | | | | running (loaded)\n"
           "  | | | held\n"
           "  | | autoloaded\n"
           "  | installed\n"
           "  popular script\n"
           "\n"
           "In output of /script list, the possible status for each script are:\n"
           "  * ? i a H N\n"
           "  | | | | | |\n"
           "  | | | | | obsolete (new version available)\n"
           "  | | | | held\n"
           "  | | | autoloaded\n"
           "  | | installed\n"
           "  | unknown script (can not be downloaded/updated)\n"
           "  popular script\n"
           "\n"
           "Keys on script buffer:\n"
           "  alt+i  install script\n"
           "  alt+r  remove script\n"
           "  alt+l  load script\n"
           "  alt+L  reload script\n"
           "  alt+u  unload script\n"
           "  alt+A  autoload script\n"
           "  alt+h  (un)hold script\n"
           "  alt+v  view script\n"
           "\n"
           "Input allowed on script buffer:\n"
           "  i/r/l/L/u/A/h/v  action on script (same as keys above)\n"
           "  q                close buffer\n"
           "  $                refresh buffer\n"
           "  s:x,y            sort buffer using keys x and y (see /help "
           "script.look.sort)\n"
           "  s:               reset sort (use default sort)\n"
           "  word(s)          filter scripts: search word(s) in scripts "
           "(description, tags, ...)\n"
           "  *                remove filter\n"
           "\n"
           "Mouse actions on script buffer:\n"
           "  wheel         scroll list\n"
           "  left button   select script\n"
           "  right button  install/remove script\n"
           "\n"
           "Examples:\n"
           "  /script search url\n"
           "  /script install go.py urlserver.py\n"
           "  /script remove go.py\n"
           "  /script hold urlserver.py\n"
           "  /script reload urlserver\n"
           "  /script upgrade"),
        "list -i|-il|-o|-ol"
        " || search %(script_tags)|%(script_languages)|%(script_extensions)"
        " || show %(script_scripts)"
        " || load %(script_files)|%*"
        " || unload %(python_script)|%(perl_script)|%(ruby_script)|"
        "%(tcl_script)|%(lua_script)|%(guile_script)|%(javascript_script)|"
        "%(php_script)|%*"
        " || reload %(python_script)|%(perl_script)|%(ruby_script)|"
        "%(tcl_script)|%(lua_script)|%(guile_script)|%(javascript_script)|"
        "%(php_script)|%*"
        " || autoload %(script_scripts_installed)|%*"
        " || noautoload %(script_scripts_installed)|%*"
        " || toggleautoload %(script_scripts_installed)|%*"
        " || install %(script_scripts)|%*"
        " || remove %(script_scripts_installed)|%*"
        " || installremove %(script_scripts)|%*"
        " || hold %(script_scripts)|%*"
        " || update"
        " || upgrade",
        &script_command_script, NULL, NULL);
}
