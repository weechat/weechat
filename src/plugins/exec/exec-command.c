/*
 * exec-command.c - exec command
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
#include <time.h>

#include "../weechat-plugin.h"
#include "exec.h"
#include "exec-buffer.h"
#include "exec-command.h"
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
                            _("  %s%s%s %ld%s%s%s: %s\"%s%s%s\"%s (pid: %d, "
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
            if (strftime (str_time1, sizeof (str_time1),
                          "%Y-%m-%d %H:%M:%S", local_time) == 0)
                str_time1[0] = '\0';
            local_time = localtime (&ptr_exec_cmd->end_time);
            if (strftime (str_time2, sizeof (str_time2),
                          "%Y-%m-%d %H:%M:%S", local_time) == 0)
                str_time2[0] = '\0';
            weechat_printf (NULL,
                            "  %s%s%s %ld%s%s%s: %s\"%s%s%s\"%s (%s -> %s, %s)",
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

    if (!id)
        return NULL;

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
 * Parse command options.
 *
 * Returns:
 *   1: options parsed successfully
 *   0: error parsing options
 */

int
exec_command_parse_options (struct t_exec_cmd_options *cmd_options,
                            int argc, char **argv, int start_arg,
                            int set_command_index)
{
    int i, j, end, length, length_total;
    char *error;

    for (i = start_arg; i < argc; i++)
    {
        if (weechat_strcmp (argv[i], "-sh") == 0)
        {
            cmd_options->use_shell = 1;
        }
        else if (weechat_strcmp (argv[i], "-nosh") == 0)
        {
            cmd_options->use_shell = 0;
        }
        else if (weechat_strcmp (argv[i], "-bg") == 0)
        {
            cmd_options->detached = 1;
        }
        else if (weechat_strcmp (argv[i], "-nobg") == 0)
        {
            cmd_options->detached = 0;
        }
        else if (weechat_strcmp (argv[i], "-stdin") == 0)
        {
            cmd_options->pipe_stdin = 1;
        }
        else if (weechat_strcmp (argv[i], "-nostdin") == 0)
        {
            cmd_options->pipe_stdin = 0;
        }
        else if (weechat_strcmp (argv[i], "-buffer") == 0)
        {
            if (i + 1 >= argc)
                return 0;
            i++;
            cmd_options->ptr_buffer_name = argv[i];
            cmd_options->ptr_buffer = weechat_buffer_search ("==", argv[i]);
            if (cmd_options->ptr_buffer
                && (weechat_buffer_get_integer (cmd_options->ptr_buffer, "type") != 0))
            {
                /* only a buffer with formatted content is allowed */
                return 0;
            }
            if (!cmd_options->ptr_buffer)
                cmd_options->new_buffer = 1;
        }
        else if (weechat_strcmp (argv[i], "-l") == 0)
        {
            cmd_options->output_to_buffer = 0;
            cmd_options->output_to_buffer_exec_cmd = 0;
            cmd_options->new_buffer = 0;
        }
        else if (weechat_strcmp (argv[i], "-o") == 0)
        {
            cmd_options->output_to_buffer = 1;
            cmd_options->output_to_buffer_exec_cmd = 0;
            cmd_options->new_buffer = 0;
        }
        else if (weechat_strcmp (argv[i], "-oc") == 0)
        {
            cmd_options->output_to_buffer = 1;
            cmd_options->output_to_buffer_exec_cmd = 1;
            cmd_options->new_buffer = 0;
        }
        else if (weechat_strcmp (argv[i], "-oerr") == 0)
        {
            cmd_options->output_to_buffer_stderr = 1;
        }
        else if (weechat_strcmp (argv[i], "-n") == 0)
        {
            cmd_options->output_to_buffer = 0;
            cmd_options->output_to_buffer_exec_cmd = 0;
            cmd_options->new_buffer = 1;
        }
        else if (weechat_strcmp (argv[i], "-nf") == 0)
        {
            cmd_options->output_to_buffer = 0;
            cmd_options->output_to_buffer_exec_cmd = 0;
            cmd_options->new_buffer = 2;
        }
        else if (weechat_strcmp (argv[i], "-cl") == 0)
        {
            cmd_options->new_buffer_clear = 1;
        }
        else if (weechat_strcmp (argv[i], "-nocl") == 0)
        {
            cmd_options->new_buffer_clear = 0;
        }
        else if (weechat_strcmp (argv[i], "-sw") == 0)
        {
            cmd_options->switch_to_buffer = 1;
        }
        else if (weechat_strcmp (argv[i], "-nosw") == 0)
        {
            cmd_options->switch_to_buffer = 0;
        }
        else if (weechat_strcmp (argv[i], "-ln") == 0)
        {
            cmd_options->line_numbers = 1;
        }
        else if (weechat_strcmp (argv[i], "-noln") == 0)
        {
            cmd_options->line_numbers = 0;
        }
        else if (weechat_strcmp (argv[i], "-flush") == 0)
        {
            cmd_options->flush = 1;
        }
        else if (weechat_strcmp (argv[i], "-noflush") == 0)
        {
            cmd_options->flush = 0;
        }
        else if (weechat_strcmp (argv[i], "-color") == 0)
        {
            if (i + 1 >= argc)
                return 0;
            i++;
            cmd_options->color = exec_search_color (argv[i]);
            if (cmd_options->color < 0)
                return 0;
        }
        else if (weechat_strcmp (argv[i], "-rc") == 0)
        {
            cmd_options->display_rc = 1;
        }
        else if (weechat_strcmp (argv[i], "-norc") == 0)
        {
            cmd_options->display_rc = 0;
        }
        else if (weechat_strcmp (argv[i], "-timeout") == 0)
        {
            if (i + 1 >= argc)
                return 0;
            i++;
            error = NULL;
            cmd_options->timeout = strtol (argv[i], &error, 10);
            if (!error || error[0])
                return 0;
        }
        else if (weechat_strcmp (argv[i], "-name") == 0)
        {
            if (i + 1 >= argc)
                return 0;
            i++;
            cmd_options->ptr_command_name = argv[i];
        }
        else if (weechat_strcmp (argv[i], "-pipe") == 0)
        {
            if (i + 1 >= argc)
                return 0;
            i++;
            if (cmd_options->pipe_command)
            {
                free (cmd_options->pipe_command);
                cmd_options->pipe_command = NULL;
            }
            if (argv[i][0] == '"')
            {
                /* search the ending double quote */
                length_total = 2;
                end = i;
                while (end < argc)
                {
                    length = strlen (argv[end]);
                    length_total += length + 1;
                    if ((length > 0) && (argv[end][length - 1] == '"'))
                        break;
                    end++;
                }
                if (end == argc)
                    return 0;
                cmd_options->pipe_command = malloc (length_total);
                if (!cmd_options->pipe_command)
                    return 0;
                cmd_options->pipe_command[0] = '\0';
                for (j = i; j <= end; j++)
                {
                    if (cmd_options->pipe_command[0])
                        strcat (cmd_options->pipe_command, " ");
                    strcat (cmd_options->pipe_command,
                            (j == i) ? argv[j] + 1 : argv[j]);
                }
                length = strlen (cmd_options->pipe_command);
                if (length > 0)
                    cmd_options->pipe_command[length - 1] = '\0';
                i = end;
            }
            else
                cmd_options->pipe_command = strdup (argv[i]);
        }
        else if (weechat_strcmp (argv[i], "-hsignal") == 0)
        {
            if (i + 1 >= argc)
                return 0;
            i++;
            if (cmd_options->hsignal)
            {
                free (cmd_options->hsignal);
                cmd_options->hsignal = NULL;
            }
            cmd_options->hsignal = strdup (argv[i]);
        }
        else
        {
            if (set_command_index)
            {
                cmd_options->command_index = i;
                break;
            }
            else
                return 0;
        }
    }

    return 1;
}

/*
 * Runs a command.
 *
 * Returns:
 *   WEECHAT_RC_OK: command run successfully
 *   WEECHAT_RC_ERROR: error running command
 */

int
exec_command_run (struct t_gui_buffer *buffer,
                  int argc, char **argv, char **argv_eol, int start_arg)
{
    char str_buffer[512], *shell, *default_shell = "sh";
    struct t_exec_cmd *new_exec_cmd;
    struct t_exec_cmd_options cmd_options;
    struct t_hashtable *process_options;
    struct t_infolist *ptr_infolist;
    struct t_gui_buffer *ptr_new_buffer;

    shell = NULL;
    new_exec_cmd = NULL;
    process_options = NULL;

    /* parse command options */
    cmd_options.command_index = -1;
    cmd_options.use_shell = 0;
    cmd_options.detached = 0;
    cmd_options.pipe_stdin = 0;
    cmd_options.timeout = 0;
    cmd_options.ptr_buffer_name = NULL;
    cmd_options.ptr_buffer = buffer;
    cmd_options.output_to_buffer = 0;
    cmd_options.output_to_buffer_exec_cmd = 0;
    cmd_options.output_to_buffer_stderr = 0;
    cmd_options.new_buffer = 0;
    cmd_options.new_buffer_clear = 0;
    cmd_options.switch_to_buffer = 1;
    cmd_options.line_numbers = -1;
    cmd_options.flush = 1;
    cmd_options.color = EXEC_COLOR_AUTO;
    cmd_options.display_rc = 1;
    cmd_options.ptr_command_name = NULL;
    cmd_options.pipe_command = NULL;
    cmd_options.hsignal = NULL;

    /* parse default options */
    if (!exec_command_parse_options (&cmd_options,
                                     exec_config_cmd_num_options,
                                     exec_config_cmd_options,
                                     0, 0))
    {
        weechat_printf (NULL,
                        _("%s%s: invalid options in option "
                          "exec.command.default_options"),
                        weechat_prefix ("error"), EXEC_PLUGIN_NAME);
        goto error;
    }
    if (!exec_command_parse_options (&cmd_options, argc, argv, start_arg, 1))
        goto error;

    /* options "-bg" and "-o"/"-oc"/"-n" are incompatible */
    if (cmd_options.detached
        && (cmd_options.output_to_buffer || cmd_options.new_buffer))
    {
        goto error;
    }

    /* options "-pipe" and "-bg"/"-o"/"-oc"/"-n" are incompatible */
    if (cmd_options.pipe_command
        && (cmd_options.detached || cmd_options.output_to_buffer
            || cmd_options.new_buffer))
    {
        goto error;
    }

    /* command not found? */
    if (cmd_options.command_index < 0)
        goto error;

    new_exec_cmd = exec_add ();
    if (!new_exec_cmd)
        goto error;

    /* create hashtable for weechat_hook_process_hashtable() */
    process_options = weechat_hashtable_new (32,
                                             WEECHAT_HASHTABLE_STRING,
                                             WEECHAT_HASHTABLE_STRING,
                                             NULL, NULL);
    if (!process_options)
        goto error;

    /* automatically disable shell if we are downloading an URL */
    if (strncmp (argv_eol[cmd_options.command_index], "url:", 4) == 0)
        cmd_options.use_shell = 0;

    /* get default shell */
    if (cmd_options.use_shell)
    {
        shell = weechat_string_eval_expression (
            weechat_config_string (exec_config_command_shell),
            NULL, NULL, NULL);
        if (!shell || !shell[0])
        {
            if (shell)
                free (shell);
            shell = strdup (default_shell);
        }
    }

    if (cmd_options.use_shell)
    {
        /* command will be: sh -c "command arguments..." */
        weechat_hashtable_set (process_options, "arg1", "-c");
        weechat_hashtable_set (process_options, "arg2",
                               argv_eol[cmd_options.command_index]);
    }
    if (cmd_options.pipe_stdin)
        weechat_hashtable_set (process_options, "stdin", "1");
    if (cmd_options.detached)
        weechat_hashtable_set (process_options, "detached", "1");
    if (cmd_options.flush)
        weechat_hashtable_set (process_options, "buffer_flush", "1");

    /* set variables in new command (before running the command) */
    new_exec_cmd->name = (cmd_options.ptr_command_name) ?
        strdup (cmd_options.ptr_command_name) : NULL;
    new_exec_cmd->command = strdup (argv_eol[cmd_options.command_index]);
    new_exec_cmd->detached = cmd_options.detached;

    if (!cmd_options.detached && !cmd_options.pipe_command
        && !cmd_options.hsignal)
    {
        if (cmd_options.ptr_buffer_name && !cmd_options.ptr_buffer)
        {
            /* output in a new buffer using given name */
            new_exec_cmd->output_to_buffer = 0;
            new_exec_cmd->output_to_buffer_exec_cmd = 0;
            snprintf (str_buffer, sizeof (str_buffer),
                      "exec.%s", cmd_options.ptr_buffer_name);
            ptr_new_buffer = exec_buffer_new (str_buffer,
                                              (cmd_options.new_buffer == 2),
                                              cmd_options.new_buffer_clear,
                                              cmd_options.switch_to_buffer);
            if (ptr_new_buffer)
            {
                new_exec_cmd->buffer_full_name =
                    strdup (weechat_buffer_get_string (ptr_new_buffer,
                                                       "full_name"));
            }
        }
        else if (cmd_options.new_buffer)
        {
            /* output in a new buffer using automatic name */
            if (new_exec_cmd->name)
            {
                snprintf (str_buffer, sizeof (str_buffer),
                          "exec.%s", new_exec_cmd->name);
            }
            else
            {
                snprintf (str_buffer, sizeof (str_buffer),
                          "exec.%ld", new_exec_cmd->number);
            }
            ptr_new_buffer = exec_buffer_new (str_buffer,
                                              (cmd_options.new_buffer == 2),
                                              cmd_options.new_buffer_clear,
                                              cmd_options.switch_to_buffer);
            if (ptr_new_buffer)
            {
                new_exec_cmd->buffer_full_name =
                    strdup (weechat_buffer_get_string (ptr_new_buffer,
                                                       "full_name"));
            }
        }
        else if (cmd_options.ptr_buffer)
        {
            new_exec_cmd->buffer_full_name =
                strdup (weechat_buffer_get_string (cmd_options.ptr_buffer,
                                                   "full_name"));
            if (cmd_options.switch_to_buffer)
                weechat_buffer_set (cmd_options.ptr_buffer, "display", "1");
        }
        if (cmd_options.ptr_buffer
            && (strcmp (weechat_buffer_get_string (cmd_options.ptr_buffer, "plugin"),
                        EXEC_PLUGIN_NAME) == 0))
        {
            cmd_options.output_to_buffer = 0;
            cmd_options.output_to_buffer_exec_cmd = 0;
            cmd_options.new_buffer = 1;
        }
    }
    new_exec_cmd->output_to_buffer = cmd_options.output_to_buffer;
    new_exec_cmd->output_to_buffer_exec_cmd = cmd_options.output_to_buffer_exec_cmd;
    new_exec_cmd->output_to_buffer_stderr = cmd_options.output_to_buffer_stderr;
    new_exec_cmd->line_numbers = (cmd_options.line_numbers < 0) ?
        cmd_options.new_buffer : cmd_options.line_numbers;
    new_exec_cmd->color = cmd_options.color;
    new_exec_cmd->display_rc = cmd_options.display_rc;
    new_exec_cmd->pipe_command = cmd_options.pipe_command;
    new_exec_cmd->hsignal = cmd_options.hsignal;

    /* execute the command */
    if (weechat_exec_plugin->debug >= 1)
    {
        weechat_printf (NULL, "%s: executing command: \"%s%s%s%s\"",
                        EXEC_PLUGIN_NAME,
                        (cmd_options.use_shell) ? shell : "",
                        (cmd_options.use_shell) ? " -c '" : "",
                        argv_eol[cmd_options.command_index],
                        (cmd_options.use_shell) ? "'" : "");
    }
    new_exec_cmd->hook = weechat_hook_process_hashtable (
        (cmd_options.use_shell) ? shell : argv_eol[cmd_options.command_index],
        process_options,
        cmd_options.timeout * 1000,
        &exec_process_cb,
        new_exec_cmd,
        NULL);

    if (new_exec_cmd->hook)
    {
        /* get PID of command */
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
    }
    else
    {
        exec_free (new_exec_cmd);
        weechat_printf (NULL,
                        _("%s%s: failed to run command \"%s\""),
                        weechat_prefix ("error"), EXEC_PLUGIN_NAME,
                        argv_eol[cmd_options.command_index]);
    }

    if (shell)
        free (shell);
    weechat_hashtable_free (process_options);

    return WEECHAT_RC_OK;

error:
    if (shell)
        free (shell);
    if (new_exec_cmd)
        exec_free (new_exec_cmd);
    if (process_options)
        weechat_hashtable_free (process_options);

    return WEECHAT_RC_ERROR;
}

/*
 * Callback for command "/exec": manage executed commands.
 */

int
exec_command_exec (const void *pointer, void *data,
                   struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    int i, length, count;
    char *text;
    struct t_exec_cmd *ptr_exec_cmd, *ptr_next_exec_cmd;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    /* list running commands */
    if ((argc == 1)
        || ((argc == 2) && (weechat_strcmp (argv[1], "-list") == 0)))
    {
        exec_command_list ();
        return WEECHAT_RC_OK;
    }

    /* send text to a running process */
    if (weechat_strcmp (argv[1], "-in") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(4, "-in");
        ptr_exec_cmd = exec_command_search_running_id (argv[2]);
        if (ptr_exec_cmd && ptr_exec_cmd->hook)
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

    /* send text to a running process (if given), then close stdin */
    if (weechat_strcmp (argv[1], "-inclose") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "-inclose");
        ptr_exec_cmd = exec_command_search_running_id (argv[2]);
        if (ptr_exec_cmd && ptr_exec_cmd->hook)
        {
            if (argc > 3)
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
            weechat_hook_set (ptr_exec_cmd->hook, "stdin_close", "1");
        }
        return WEECHAT_RC_OK;
    }

    /* send a signal to a running process */
    if (weechat_strcmp (argv[1], "-signal") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(4, "-signal");
        ptr_exec_cmd = exec_command_search_running_id (argv[2]);
        if (ptr_exec_cmd)
            weechat_hook_set (ptr_exec_cmd->hook, "signal", argv[3]);
        return WEECHAT_RC_OK;
    }

    /* send a KILL signal to a running process */
    if (weechat_strcmp (argv[1], "-kill") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "-kill");
        ptr_exec_cmd = exec_command_search_running_id (argv[2]);
        if (ptr_exec_cmd)
            weechat_hook_set (ptr_exec_cmd->hook, "signal", "kill");
        return WEECHAT_RC_OK;
    }

    /* send a KILL signal to all running processes */
    if (weechat_strcmp (argv[1], "-killall") == 0)
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
    if (weechat_strcmp (argv[1], "-set") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(5, "-set");
        ptr_exec_cmd = exec_command_search_running_id (argv[2]);
        if (ptr_exec_cmd)
            weechat_hook_set (ptr_exec_cmd->hook, argv[3], argv_eol[4]);
        return WEECHAT_RC_OK;
    }

    /* delete terminated command(s) */
    if (weechat_strcmp (argv[1], "-del") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "-del");
        if (weechat_strcmp (argv[2], "-all") == 0)
        {
            count = 0;
            ptr_exec_cmd = exec_cmds;
            while (ptr_exec_cmd)
            {
                ptr_next_exec_cmd = ptr_exec_cmd->next_cmd;
                if (!ptr_exec_cmd->hook)
                {
                    exec_free (ptr_exec_cmd);
                    count++;
                }
                ptr_exec_cmd = ptr_next_exec_cmd;
            }
            weechat_printf (NULL, _("%d commands removed"), count);
        }
        else
        {
            for (i = 2; i < argc; i++)
            {
                ptr_exec_cmd = exec_search_by_id (argv[i]);
                if (ptr_exec_cmd)
                {
                    if (ptr_exec_cmd->hook)
                    {
                        weechat_printf (NULL,
                                        _("%s%s: command with id \"%s\" is still "
                                          "running"),
                                        weechat_prefix ("error"), EXEC_PLUGIN_NAME,
                                        argv[i]);
                    }
                    else
                    {
                        exec_free (ptr_exec_cmd);
                        weechat_printf (NULL,
                                        _("Command \"%s\" removed"), argv[i]);
                    }
                }
                else
                {
                    weechat_printf (NULL,
                                    _("%s%s: command id \"%s\" not found"),
                                    weechat_prefix ("error"), EXEC_PLUGIN_NAME,
                                    argv[i]);
                }
            }
        }
        return WEECHAT_RC_OK;
    }

    return exec_command_run (buffer, argc, argv, argv_eol, 1);
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
           " || [-sh|-nosh] [-bg|-nobg] [-stdin|-nostdin] [-buffer <name>] "
           "[-l|-o|-oc|-n|-nf] [-oerr] [-cl|-nocl] [-sw|-nosw] [-ln|-noln] "
           "[-flush|-noflush] [-color ansi|auto|irc|weechat|strip] [-rc|-norc] "
           "[-timeout <timeout>] [-name <name>] [-pipe <command>] "
           "[-hsignal <name>] <command>"
           " || -in <id> <text>"
           " || -inclose <id> [<text>]"
           " || -signal <id> <signal>"
           " || -kill <id>"
           " || -killall"
           " || -set <id> <property> <value>"
           " || -del <id>|-all [<id>...]"),
        N_("   -list: list commands\n"
           "     -sh: use the shell to execute the command, many commands can "
           "be piped (WARNING: use this option ONLY if all arguments are "
           "safe, see option -nosh)\n"
           "   -nosh: do not use the shell to execute the command (required if "
           "the command has some unsafe data, for example the content of a "
            "message from another user) (default)\n"
           "     -bg: run process in background: do not display process output "
           "neither return code (not compatible with options "
           "-o/-oc/-n/-nf/-pipe/-hsignal)\n"
           "   -nobg: catch process output and display return code (default)\n"
           "  -stdin: create a pipe for sending data to the process (with "
           "/exec -in/-inclose)\n"
           "-nostdin: do not create a pipe for stdin (default)\n"
           " -buffer: display/send output of command on this buffer (if the "
           "buffer is not found, a new buffer with name \"exec.exec.xxx\" is "
           "created)\n"
           "      -l: display locally output of command on buffer (default)\n"
           "      -o: send output of command to the buffer without executing "
           "commands (not compatible with options -bg/-pipe/-hsignal)\n"
           "     -oc: send output of command to the buffer and execute commands "
           "(lines starting with \"/\" or another custom command char) "
           "(not compatible with options -bg/-pipe/-hsignal)\n"
           "      -n: display output of command in a new buffer (not compatible "
           "with options -bg/-pipe/-hsignal)\n"
           "     -nf: display output of command in a new buffer with free "
           "content (no word-wrap, no limit on number of lines) (not compatible "
           "with options -bg/-pipe/-hsignal)\n"
           "   -oerr: also send stderr (error output) to the buffer (can be "
           "used only with options -o and -oc)\n"
           "     -cl: clear the new buffer before displaying output\n"
           "   -nocl: append to the new buffer without clear (default)\n"
           "     -sw: switch to the output buffer (default)\n"
           "   -nosw: don't switch to the output buffer\n"
           "     -ln: display line numbers (default in new buffer only)\n"
           "   -noln: don't display line numbers\n"
           "  -flush: display output of command in real time (default)\n"
           "-noflush: display output of command after its end\n"
           "  -color: action on ANSI colors in output:\n"
           "             ansi: keep ANSI codes as-is\n"
           "             auto: convert ANSI colors to WeeChat/IRC (default)\n"
           "              irc: convert ANSI colors to IRC colors\n"
           "          weechat: convert ANSI colors to WeeChat colors\n"
           "            strip: remove ANSI colors\n"
           "     -rc: display return code (default)\n"
           "   -norc: don't display return code\n"
           "-timeout: set a timeout for the command (in seconds)\n"
           "   -name: set a name for the command (to name it later with /exec)\n"
           "   -pipe: send the output to a WeeChat/plugin command (line by "
           "line); if there are spaces in command/arguments, enclose them with "
           "double quotes; variable $line is replaced by the line (by default "
           "the line is added after the command, separated by a space) "
           "(not compatible with options -bg/-o/-oc/-n/-nf)\n"
           "-hsignal: send the output as a hsignal (to be used for example in "
           "a trigger) (not compatible with options -bg/-o/-oc/-n/-nf)\n"
           " command: the command to execute; if beginning with \"url:\", the "
           "shell is disabled and the content of URL is downloaded and sent as "
           "output\n"
           "      id: command identifier: either its number or name (if set "
           "with \"-name xxx\")\n"
           "     -in: send text on standard input of process\n"
           "-inclose: same as -in, but stdin is closed after (and text is "
           "optional: without text, the stdin is just closed)\n"
           " -signal: send a signal to the process; the signal can be an integer "
           "or one of these names: hup, int, quit, kill, term, usr1, usr2\n"
           "   -kill: alias of \"-signal <id> kill\"\n"
           "-killall: kill all running processes\n"
           "    -set: set a hook property (see function hook_set in plugin API "
           "reference)\n"
           "property: hook property\n"
           "   value: new value for hook property\n"
           "    -del: delete a terminated command\n"
           "    -all: delete all terminated commands\n"
           "\n"
           "Default options can be set in the option "
           "exec.command.default_options.\n"
           "\n"
           "Examples:\n"
           "  /exec -n ls -l /tmp\n"
           "  /exec -sh -n ps xu | grep weechat\n"
           "  /exec -n -norc url:https://pastebin.com/raw.php?i=xxxxxxxx\n"
           "  /exec -nf -noln links -dump "
           "https://weechat.org/files/doc/weechat/devel/weechat_user.en.html\n"
           "  /exec -o uptime\n"
           "  /exec -pipe \"/print Machine uptime:\" uptime\n"
           "  /exec -n tail -f /var/log/messages\n"
           "  /exec -kill 0"),
        "-list"
        " || -sh|-nosh|-bg|-nobg|-stdin|-nostdin|-buffer|-l|-o|-n|-nf|"
        "-cl|-nocl|-sw|-nosw|-ln|-noln|-flush|-noflush|-color|-timeout|-name|"
        "-pipe|-hsignal|%*"
        " || -in|-inclose|-signal|-kill %(exec_commands_ids)"
        " || -killall"
        " || -set %(exec_commands_ids) stdin|stdin_close|signal"
        " || -del %(exec_commands_ids)|-all %(exec_commands_ids)|%*",
        &exec_command_exec, NULL, NULL);
}
