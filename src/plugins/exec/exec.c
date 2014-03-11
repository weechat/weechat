/*
 * exec.c - execution of external commands in WeeChat
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
#include "exec-command.h"
#include "exec-completion.h"
#include "exec-config.h"


WEECHAT_PLUGIN_NAME(EXEC_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Execution of external commands in WeeChat"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);

struct t_weechat_plugin *weechat_exec_plugin = NULL;

struct t_exec_cmd *exec_cmds = NULL;        /* first executed command       */
struct t_exec_cmd *last_exec_cmd = NULL;    /* last executed command        */
int exec_cmds_count = 0;                    /* number of executed commands  */


/*
 * Searches for an executed command by id, which can be a number or a name.
 *
 * Returns pointer to executed command found, NULL if not found.
 */

struct t_exec_cmd *
exec_search_by_id (const char *id)
{
    struct t_exec_cmd* ptr_exec_cmd;
    char *error;
    long number;

    error = NULL;
    number = strtol (id, &error, 10);
    if (!error || error[0])
        number = -1;

    for (ptr_exec_cmd = exec_cmds; ptr_exec_cmd;
         ptr_exec_cmd = ptr_exec_cmd->next_cmd)
    {
        /* check if number is matching */
        if ((number >= 0) && (ptr_exec_cmd->number == (int)number))
            return ptr_exec_cmd;

        /* check if name is matching */
        if (ptr_exec_cmd->name && (strcmp (ptr_exec_cmd->name, id) == 0))
            return ptr_exec_cmd;
    }

    /* executed command not found */
    return NULL;
}

/*
 * Adds a command in list of executed commands.
 */

struct t_exec_cmd *
exec_add ()
{
    struct t_exec_cmd *new_exec_cmd, *ptr_exec_cmd;
    int number;

    /* find first available number */
    number = (last_exec_cmd) ? last_exec_cmd->number + 1 : 0;
    for (ptr_exec_cmd = exec_cmds; ptr_exec_cmd;
         ptr_exec_cmd = ptr_exec_cmd->next_cmd)
    {
        if (ptr_exec_cmd->prev_cmd
            && (ptr_exec_cmd->number > ptr_exec_cmd->prev_cmd->number + 1))
        {
            number = ptr_exec_cmd->prev_cmd->number + 1;
            break;
        }
    }

    new_exec_cmd = malloc (sizeof (*new_exec_cmd));
    if (!new_exec_cmd)
        return NULL;

    new_exec_cmd->prev_cmd = last_exec_cmd;
    new_exec_cmd->next_cmd = NULL;
    if (!exec_cmds)
        exec_cmds = new_exec_cmd;
    else
        last_exec_cmd->next_cmd = new_exec_cmd;
    last_exec_cmd = new_exec_cmd;

    new_exec_cmd->number = number;
    new_exec_cmd->name = NULL;
    new_exec_cmd->hook = NULL;
    new_exec_cmd->command = NULL;
    new_exec_cmd->pid = 0;
    new_exec_cmd->detached = 0;
    new_exec_cmd->start_time = time (NULL);
    new_exec_cmd->end_time = 0;
    new_exec_cmd->buffer_plugin = NULL;
    new_exec_cmd->buffer_name = NULL;
    new_exec_cmd->output_to_buffer = 0;
    new_exec_cmd->stdout_size = 0;
    new_exec_cmd->stdout = NULL;
    new_exec_cmd->stderr_size = 0;
    new_exec_cmd->stderr = NULL;
    new_exec_cmd->return_code = -1;

    exec_cmds_count++;

    return new_exec_cmd;
}

/*
 * Timer callback to delete a command.
 */

int
exec_timer_delete_cb (void *data, int remaining_calls)
{
    struct t_exec_cmd *exec_cmd, *ptr_exec_cmd;

    /* make C compiler happy */
    (void) remaining_calls;

    exec_cmd = (struct t_exec_cmd *)data;
    if (!exec_cmd)
        return WEECHAT_RC_OK;

    for (ptr_exec_cmd = exec_cmds; ptr_exec_cmd;
         ptr_exec_cmd = ptr_exec_cmd->next_cmd)
    {
        if (ptr_exec_cmd == exec_cmd)
        {
            exec_free (ptr_exec_cmd);
            break;
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Concatenates some text to stdout/stderr of a command.
 */

void
exec_command_concat_output (int *size, char **output, const char *text)
{
    int length, new_size;
    char *new_output;

    length = strlen (text);
    new_size = *size + length;
    new_output = realloc (*output, new_size + 1);
    if (!new_output)
        return;

    *output = new_output;
    memcpy (*output + *size, text, length + 1);
    *size = new_size;
}

/*
 * Displays output of a command.
 */

void
exec_command_display_output (struct t_exec_cmd *exec_cmd,
                             struct t_gui_buffer *buffer, int stdout)
{
    char *ptr_output, **lines, str_number[32], str_tags[1024];
    int i, num_lines;

    ptr_output = (stdout) ? exec_cmd->stdout : exec_cmd->stderr;
    if (!ptr_output)
        return;

    /*
     * if output is sent to the buffer, the buffer must exist
     * (we don't send output by default to core buffer)
     */
    if (exec_cmd->output_to_buffer && !buffer)
        return;

    lines = weechat_string_split (ptr_output, "\n", 0, 0, &num_lines);
    if (!lines)
        return;

    for (i = 0; i < num_lines; i++)
    {
        if (exec_cmd->output_to_buffer)
            weechat_command (buffer, lines[i]);
        else
        {
            snprintf (str_number, sizeof (str_number), "%d", exec_cmd->number);
            snprintf (str_tags, sizeof (str_tags),
                      "exec_%s,exec_cmd_%s",
                      (stdout) ? "stdout" : "stderr",
                      (exec_cmd->name) ? exec_cmd->name : str_number);
            weechat_printf_tags (buffer, str_tags,
                                 "%s%s",
                                 (stdout) ? " \t" : weechat_prefix ("error"),
                                 lines[i]);
        }
    }

    weechat_string_free_split (lines);
}

/*
 * Ends a command.
 */

void
exec_end_command (struct t_exec_cmd *exec_cmd, int return_code)
{
    struct t_gui_buffer *ptr_buffer;

    ptr_buffer = weechat_buffer_search (exec_cmd->buffer_plugin,
                                        exec_cmd->buffer_name);

    /* display stdout/stderr (if output to buffer, the buffer must exist) */
    exec_command_display_output (exec_cmd, ptr_buffer, 1);
    exec_command_display_output (exec_cmd, ptr_buffer, 0);

    /*
     * display return code (only if command is not detached and if output is
     * NOT sent to buffer)
     */
    if (!exec_cmd->detached && !exec_cmd->output_to_buffer)
    {
        if (return_code >= 0)
        {
            weechat_printf_tags (ptr_buffer, "exec_rc",
                                 _("%s: end of command %d (\"%s\"), "
                                   "return code: %d"),
                                 EXEC_PLUGIN_NAME, exec_cmd->number,
                                 exec_cmd->command, return_code);
        }
        else
        {
            weechat_printf_tags (ptr_buffer, "exec_rc",
                                 _("%s: unexpected end of command %d "
                                   "(\"%s\")"),
                                 EXEC_PLUGIN_NAME, exec_cmd->number,
                                 exec_cmd->command);
        }
    }

    /* (re)set some variables after the end of command */
    exec_cmd->hook = NULL;
    exec_cmd->pid = 0;
    exec_cmd->end_time = time (NULL);
    exec_cmd->return_code = return_code;

    /* schedule a timer to remove the executed command */
    if (weechat_config_integer (exec_config_command_purge_delay) >= 0)
    {
        weechat_hook_timer (1 + (1000 * weechat_config_integer (exec_config_command_purge_delay)),
                            0, 1,
                            &exec_timer_delete_cb, exec_cmd);
    }
}

/*
 * Callback for hook process.
 */

int
exec_process_cb (void *data, const char *command, int return_code,
                 const char *out, const char *err)
{
    struct t_exec_cmd *ptr_exec_cmd;

    /* make C compiler happy */
    (void) command;

    ptr_exec_cmd = (struct t_exec_cmd *)data;
    if (!ptr_exec_cmd)
        return WEECHAT_RC_ERROR;

    if (weechat_exec_plugin->debug >= 2)
    {
        weechat_printf (NULL,
                        "%s: process_cb: command=\"%s\", rc=%d, "
                        "out: %d bytes, err: %d bytes",
                        EXEC_PLUGIN_NAME,
                        ptr_exec_cmd->command,
                        return_code,
                        (out) ? strlen (out) : 0,
                        (err) ? strlen (err) : 0);
    }

    if (return_code == WEECHAT_HOOK_PROCESS_ERROR)
    {
        exec_end_command (ptr_exec_cmd, -1);
        return WEECHAT_RC_OK;
    }

    if (out)
    {
        exec_command_concat_output (&ptr_exec_cmd->stdout_size,
                                    &ptr_exec_cmd->stdout,
                                    out);
    }
    if (err)
    {
        exec_command_concat_output (&ptr_exec_cmd->stderr_size,
                                    &ptr_exec_cmd->stderr,
                                    err);
    }

    if (return_code >= 0)
        exec_end_command (ptr_exec_cmd, return_code);

    return WEECHAT_RC_OK;
}

/*
 * Deletes a command.
 */

void
exec_free (struct t_exec_cmd *exec_cmd)
{
    if (!exec_cmd)
        return;

    /* remove command from commands list */
    if (exec_cmd->prev_cmd)
        (exec_cmd->prev_cmd)->next_cmd = exec_cmd->next_cmd;
    if (exec_cmd->next_cmd)
        (exec_cmd->next_cmd)->prev_cmd = exec_cmd->prev_cmd;
    if (exec_cmds == exec_cmd)
        exec_cmds = exec_cmd->next_cmd;
    if (last_exec_cmd == exec_cmd)
        last_exec_cmd = exec_cmd->prev_cmd;

    /* free data */
    if (exec_cmd->hook)
        weechat_unhook (exec_cmd->hook);
    if (exec_cmd->name)
        free (exec_cmd->name);
    if (exec_cmd->command)
        free (exec_cmd->command);
    if (exec_cmd->buffer_plugin)
        free (exec_cmd->buffer_plugin);
    if (exec_cmd->buffer_name)
        free (exec_cmd->buffer_name);
    if (exec_cmd->stdout)
        free (exec_cmd->stdout);
    if (exec_cmd->stderr)
        free (exec_cmd->stderr);

    free (exec_cmd);

    exec_cmds_count--;
}

/*
 * Deletes all commands.
 */

void
exec_free_all ()
{
    while (exec_cmds)
    {
        exec_free (exec_cmds);
    }
}

/*
 * Prints exec infos in WeeChat log file (usually for crash dump).
 */

void
exec_print_log ()
{
    struct t_exec_cmd *ptr_exec_cmd;

    for (ptr_exec_cmd = exec_cmds; ptr_exec_cmd;
         ptr_exec_cmd = ptr_exec_cmd->next_cmd)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[exec command (addr:0x%lx)]", ptr_exec_cmd);
        weechat_log_printf ("  number. . . . . . . . . : %d",    ptr_exec_cmd->number);
        weechat_log_printf ("  name. . . . . . . . . . : '%s'",  ptr_exec_cmd->name);
        weechat_log_printf ("  hook. . . . . . . . . . : 0x%lx", ptr_exec_cmd->hook);
        weechat_log_printf ("  command . . . . . . . . : '%s'",  ptr_exec_cmd->command);
        weechat_log_printf ("  pid . . . . . . . . . . : %d",    ptr_exec_cmd->pid);
        weechat_log_printf ("  detached. . . . . . . . : %d",    ptr_exec_cmd->detached);
        weechat_log_printf ("  start_time. . . . . . . : %ld",   ptr_exec_cmd->start_time);
        weechat_log_printf ("  end_time. . . . . . . . : %ld",   ptr_exec_cmd->end_time);
        weechat_log_printf ("  buffer_plugin . . . . . : '%s'",  ptr_exec_cmd->buffer_plugin);
        weechat_log_printf ("  buffer_name . . . . . . : '%s'",  ptr_exec_cmd->buffer_name);
        weechat_log_printf ("  output_to_buffer. . . . : %d",    ptr_exec_cmd->output_to_buffer);
        weechat_log_printf ("  stdout_size . . . . . . : %d",    ptr_exec_cmd->stdout_size);
        weechat_log_printf ("  stdout. . . . . . . . . : '%s'",  ptr_exec_cmd->stdout);
        weechat_log_printf ("  stderr_size . . . . . . : %d",    ptr_exec_cmd->stderr_size);
        weechat_log_printf ("  stderr. . . . . . . . . : '%s'",  ptr_exec_cmd->stderr);
        weechat_log_printf ("  return_code . . . . . . : %d",    ptr_exec_cmd->return_code);
        weechat_log_printf ("  prev_cmd. . . . . . . . : 0x%lx", ptr_exec_cmd->prev_cmd);
        weechat_log_printf ("  next_cmd. . . . . . . . : 0x%lx", ptr_exec_cmd->next_cmd);
    }
}

/*
 * Callback for signal "debug_dump".
 */

int
exec_debug_dump_cb (void *data, const char *signal, const char *type_data,
                    void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data
        || (weechat_strcasecmp ((char *)signal_data, EXEC_PLUGIN_NAME) == 0))
    {
        weechat_log_printf ("");
        weechat_log_printf ("***** \"%s\" plugin dump *****",
                            weechat_plugin->name);

        exec_print_log ();

        weechat_log_printf ("");
        weechat_log_printf ("***** End of \"%s\" plugin dump *****",
                            weechat_plugin->name);
    }

    return WEECHAT_RC_OK;
}

/*
 * Initializes exec plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    exec_command_init ();

    if (!exec_config_init ())
        return WEECHAT_RC_ERROR;

    exec_config_read ();

    /* hook some signals */
    weechat_hook_signal ("debug_dump", &exec_debug_dump_cb, NULL);

    /* hook completions */
    exec_completion_init ();

    return WEECHAT_RC_OK;
}

/*
 * Ends exec plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    exec_config_write ();
    exec_free_all ();
    exec_config_free ();

    return WEECHAT_RC_OK;
}
