/*
 * exec-command.c - exec command
 *
 * Copyright (C) 2014 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "exec.h"
#include "exec-config.h"


/*
 * Displays a list of executed commands.
 */

void
exec_command_list ()
{
    struct t_exec_cmd *ptr_exec_cmd;
    char str_elapsed[32], str_time1[256], str_time2[256];
    time_t elapsed_time;
    struct tm *local_time;

    weechat_printf (NULL, "");

    if (!exec_cmds)
    {
        weechat_printf (NULL, _("No command is running"));
        return;
    }

    weechat_printf (NULL, _("Commands:"));

    for (ptr_exec_cmd = exec_cmds; ptr_exec_cmd;
         ptr_exec_cmd = ptr_exec_cmd->next_cmd)
    {
        elapsed_time = (ptr_exec_cmd->end_time == 0) ?
            time (NULL) - ptr_exec_cmd->start_time :
            ptr_exec_cmd->end_time - ptr_exec_cmd->start_time;
        if (elapsed_time >= 3600)
        {
            snprintf (str_elapsed, sizeof (str_elapsed),
                      /* TRANSLATORS: format: hours + minutes, for example: 3h59 */
                      _("%dh%02d"),
                      elapsed_time / 3600,
                      elapsed_time % 3600);
        }
        else if (elapsed_time >= 60)
        {
            snprintf (str_elapsed, sizeof (str_elapsed),
                      /* TRANSLATORS: format: minutes + seconds, for example: 3m59 */
                      _("%dm%02d"),
                      elapsed_time / 60,
                      elapsed_time % 60);
        }
        else
        {
            snprintf (str_elapsed, sizeof (str_elapsed),
                      /* TRANSLATORS: format: seconds, for example: 59s */
                      _("%ds"),
                      elapsed_time);
        }
        if (ptr_exec_cmd->end_time == 0)
        {
            /* running command */
            weechat_printf (NULL,
                            /* TRANSLATORS: %s before "ago" is elapsed time, for example: "3m59" */
                            _("  %s%s%s %d%s%s%s: %s\"%s%s%s\"%s (pid: %d, "
                              "started %s ago)"),
                            weechat_color (weechat_config_string (exec_config_color_flag_running)),
                            ">>",
                            weechat_color ("reset"),
                            ptr_exec_cmd->number,
                            (ptr_exec_cmd->name) ? " (" : "",
                            (ptr_exec_cmd->name) ? ptr_exec_cmd->name : "",
                            (ptr_exec_cmd->name) ? ")" : "",
                            weechat_color ("chat_delimiters"),
                            weechat_color ("reset"),
                            ptr_exec_cmd->command,
                            weechat_color ("chat_delimiters"),
                            weechat_color ("reset"),
                            ptr_exec_cmd->pid,
                            str_elapsed);
        }
        else
        {
            /* process has ended */
            local_time = localtime (&ptr_exec_cmd->start_time);
            strftime (str_time1, sizeof (str_time1),
                      "%Y-%m-%d %H:%M:%S", local_time);
            local_time = localtime (&ptr_exec_cmd->end_time);
            strftime (str_time2, sizeof (str_time2),
                      "%Y-%m-%d %H:%M:%S", local_time);
            weechat_printf (NULL,
                            "  %s%s%s %d%s%s%s: %s\"%s%s%s\"%s (%s -> %s, %s)",
                            weechat_color (weechat_config_string (exec_config_color_flag_finished)),
                            "[]",
                            weechat_color ("reset"),
                            ptr_exec_cmd->number,
                            (ptr_exec_cmd->name) ? " (" : "",
                            (ptr_exec_cmd->name) ? ptr_exec_cmd->name : "",
                            (ptr_exec_cmd->name) ? ")" : "",
                            weechat_color ("chat_delimiters"),
                            weechat_color ("reset"),
                            ptr_exec_cmd->command,
                            weechat_color ("chat_delimiters"),
                            weechat_color ("reset"),
                            str_time1,
                            str_time2,
                            str_elapsed);
        }
    }
}

/*
 * Searches a running command by id, and displays an error if command is not
 * found or not running any more.
 *
 * Returns the command found, or NULL if not found or not running.
 */

struct t_exec_cmd *
exec_command_search_running_id (const char *id)
{
    struct t_exec_cmd *ptr_exec_cmd;

    ptr_exec_cmd = exec_search_by_id (id);
    if (!ptr_exec_cmd)
    {
        weechat_printf (NULL, _("%s%s: command id \"%s\" not found"),
                        weechat_prefix ("error"), EXEC_PLUGIN_NAME, id);
        return NULL;
    }

    if (!ptr_exec_cmd->hook)
    {
        weechat_printf (NULL,
                        _("%s%s: command with id \"%s\" is not running any "
                          "more"),
                        weechat_prefix ("error"), EXEC_PLUGIN_NAME, id);
        return NULL;
    }

    return ptr_exec_cmd;
}

/*
 * Callback for command "/exec": manage executed commands.
 */

int
exec_command_exec (void *data, struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    int i, command_index, use_shell, pipe_stdin, output_to_buffer, length;
    long timeout;
    char *error, *ptr_name, *text;
    struct t_exec_cmd *ptr_exec_cmd, *new_exec_cmd;
    struct t_hashtable *options_cmd;
    struct t_infolist *ptr_infolist;

    /* make C compiler happy */
    (void) data;
    (void) buffer;

    /* list running commands */
    if ((argc == 1)
        || ((argc == 2) && (weechat_strcasecmp (argv[1], "-list") == 0)))
    {
        exec_command_list ();
        return WEECHAT_RC_OK;
    }

    /* send text to a running process */
    if (weechat_strcasecmp (argv[1], "-in") == 0)
    {
        if (argc < 4)
            return WEECHAT_RC_ERROR;
        ptr_exec_cmd = exec_command_search_running_id (argv[2]);
        if (ptr_exec_cmd->hook)
        {
            length = strlen (argv_eol[3]) + 1 + 1;
            text = malloc (length);
            if (text)
            {
                snprintf (text, length, "%s\n", argv_eol[3]);
                weechat_hook_set (ptr_exec_cmd->hook, "stdin", text);
                free (text);
            }
        }
        return WEECHAT_RC_OK;
    }

    /* send a signal to a running process */
    if (weechat_strcasecmp (argv[1], "-signal") == 0)
    {
        if (argc < 4)
            return WEECHAT_RC_ERROR;
        ptr_exec_cmd = exec_command_search_running_id (argv[2]);
        if (ptr_exec_cmd)
            weechat_hook_set (ptr_exec_cmd->hook, "signal", argv[3]);
        return WEECHAT_RC_OK;
    }

    /* send a KILL signal to a running process */
    if (weechat_strcasecmp (argv[1], "-kill") == 0)
    {
        if (argc < 3)
            return WEECHAT_RC_ERROR;
        ptr_exec_cmd = exec_command_search_running_id (argv[2]);
        if (ptr_exec_cmd)
            weechat_hook_set (ptr_exec_cmd->hook, "signal", "kill");
        return WEECHAT_RC_OK;
    }

    /* send a KILL signal to all running processes */
    if (weechat_strcasecmp (argv[1], "-killall") == 0)
    {
        for (ptr_exec_cmd = exec_cmds; ptr_exec_cmd;
             ptr_exec_cmd = ptr_exec_cmd->next_cmd)
        {
            if (ptr_exec_cmd->hook)
            {
                weechat_hook_set (ptr_exec_cmd->hook, "signal", "kill");
            }
        }
        return WEECHAT_RC_OK;
    }

    /* set a hook property */
    if (weechat_strcasecmp (argv[1], "-set") == 0)
    {
        if (argc < 5)
            return WEECHAT_RC_ERROR;
        ptr_exec_cmd = exec_command_search_running_id (argv[2]);
        if (ptr_exec_cmd)
            weechat_hook_set (ptr_exec_cmd->hook, argv[3], argv_eol[4]);
        return WEECHAT_RC_OK;
    }

    /* parse command options */
    command_index = -1;
    use_shell = 1;
    pipe_stdin = 0;
    timeout = 0;
    output_to_buffer = 0;
    ptr_name = NULL;

    for (i = 1; i < argc; i++)
    {
        if (weechat_strcasecmp (argv[i], "-nosh") == 0)
        {
            use_shell = 0;
        }
        else if (weechat_strcasecmp (argv[i], "-stdin") == 0)
        {
            pipe_stdin = 1;
        }
        else if (weechat_strcasecmp (argv[i], "-o") == 0)
        {
            output_to_buffer = 1;
        }
        else if (weechat_strcasecmp (argv[i], "-timeout") == 0)
        {
            if (i + 1 >= argc)
                return WEECHAT_RC_ERROR;
            i++;
            error = NULL;
            timeout = strtol (argv[i], &error, 10);
            if (!error || error[0])
                return WEECHAT_RC_ERROR;
        }
        else if (weechat_strcasecmp (argv[i], "-name") == 0)
        {
            if (i + 1 >= argc)
                return WEECHAT_RC_ERROR;
            i++;
            ptr_name = argv[i];
        }
        else
        {
            command_index = i;
            break;
        }
    }
    if (command_index < 0)
        return WEECHAT_RC_ERROR;

    new_exec_cmd = exec_add ();
    if (!new_exec_cmd)
        return WEECHAT_RC_ERROR;

    /* create hashtable for weechat_hook_process_hashtable() */
    options_cmd = weechat_hashtable_new (32,
                                         WEECHAT_HASHTABLE_STRING,
                                         WEECHAT_HASHTABLE_STRING,
                                         NULL,
                                         NULL);
    if (!options_cmd)
    {
        exec_free (new_exec_cmd);
        return WEECHAT_RC_ERROR;
    }

    /* run the command in background */
    if (use_shell)
    {
        /* command will be: sh -c "command arguments..." */
        weechat_hashtable_set (options_cmd, "arg1", "-c");
        weechat_hashtable_set (options_cmd, "arg2", argv_eol[command_index]);
    }
    if (weechat_exec_plugin->debug >= 1)
    {
        weechat_printf (NULL, "%s: executing command: \"%s%s%s\"",
                        EXEC_PLUGIN_NAME,
                        (use_shell) ? "" : "sh -c '",
                        argv_eol[command_index],
                        (use_shell) ? "" : "'");
    }
    if (pipe_stdin)
        weechat_hashtable_set (options_cmd, "stdin", "1");
    new_exec_cmd->hook = weechat_hook_process_hashtable (
        (use_shell) ? "sh" : argv_eol[command_index],
        options_cmd,
        (int)(timeout * 1000),
        &exec_process_cb,
        new_exec_cmd);

    weechat_hashtable_free (options_cmd);

    if (!new_exec_cmd->hook)
    {
        exec_free (new_exec_cmd);
        weechat_printf (NULL,
                        _("%s%s: failed to run command \"%s\""),
                        weechat_prefix ("error"), EXEC_PLUGIN_NAME,
                        argv_eol[command_index]);
        return WEECHAT_RC_OK;
    }

    new_exec_cmd->name = (ptr_name) ? strdup (ptr_name) : NULL;
    new_exec_cmd->command = strdup (argv_eol[command_index]);
    new_exec_cmd->buffer_plugin = strdup (weechat_buffer_get_string (buffer,
                                                                     "plugin"));
    new_exec_cmd->buffer_name = strdup (weechat_buffer_get_string (buffer,
                                                                   "name"));
    new_exec_cmd->output_to_buffer = output_to_buffer;
    ptr_infolist = weechat_infolist_get ("hook", new_exec_cmd->hook, NULL);
    if (ptr_infolist)
    {
        if (weechat_infolist_next (ptr_infolist))
        {
            new_exec_cmd->pid = weechat_infolist_integer (ptr_infolist,
                                                          "child_pid");
        }
        weechat_infolist_free (ptr_infolist);
    }

    return WEECHAT_RC_OK;
}

/*
 * Hooks exec commands.
 */

void
exec_command_init ()
{
    weechat_hook_command (
        "exec",
        N_("execute external commands"),
        N_("-list"
           " || [-nosh] [-stdin] [-o] [-timeout <timeout>] [-name <name>] <id>"
           " || -in <id> <text>"
           " || -signal <id> <signal>"
           " || -kill <id>"
           " || -killall"
           " || -set <id> <property> <value>"),
        N_("   -list: list commands\n"
           "   -nosh: do not use the shell to execute the command (required if "
           "the command has some unsafe data, for example the content of a "
            "message from another user)\n"
           "  -stdin: create a pipe for sending data to the process (with "
           "/exec -in)\n"
           "      -o: send output of command to the current buffer\n"
           "-timeout: set a timeout for the command (in seconds)\n"
           "   -name: set a name for the command (to name it later with /exec)\n"
           " command: the command to execute\n"
           "      id: command identifier: either its number or name (if set "
           "with \"-name xxx\")\n"
           "     -in: send text on standard input of process\n"
           " -signal: send a signal to the process; the signal can be an integer "
           "or one of these names: hup, int, quit, kill, term, usr1, usr2\n"
           "   -kill: alias of \"-signal <id> kill\"\n"
           "-killall: kill all running processes\n"
           "    -set: set a hook property (see function hook_set in plugin API "
           "reference)\n"
           "property: hook property\n"
           "   value: new value for hook property"),
        "-list"
        " || -nosh|-stdin|-o|-timeout|-name|%*"
        " || -in|-signal|-kill %(exec_commands_ids)"
        " || -killall"
        " || -set %(exec_commands_ids) stdin|stdin_close|signal",
        &exec_command_exec, NULL);
}
