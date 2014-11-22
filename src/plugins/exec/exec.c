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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "exec.h"
#include "exec-buffer.h"
#include "exec-command.h"
#include "exec-completion.h"
#include "exec-config.h"


WEECHAT_PLUGIN_NAME(EXEC_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Execution of external commands in " PACKAGE_NAME));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);

struct t_weechat_plugin *weechat_exec_plugin = NULL;

struct t_exec_cmd *exec_cmds = NULL;        /* first executed command       */
struct t_exec_cmd *last_exec_cmd = NULL;    /* last executed command        */
int exec_cmds_count = 0;                    /* number of executed commands  */

char *exec_color_string[EXEC_NUM_COLORS] =
{ "ansi", "auto", "irc", "weechat", "strip" };


/*
 * Searches for a color action name.
 *
 * Returns index of color in enum t_exec_color, -1 if not found.
 */

int
exec_search_color (const char *color)
{
    int i;

    if (!color)
        return -1;

    for (i = 0; i < EXEC_NUM_COLORS; i++)
    {
        if (weechat_strcasecmp (exec_color_string[i], color) == 0)
            return i;
    }

    /* color not found */
    return -1;
}

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
    int i, number;

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
    if (exec_cmds)
        last_exec_cmd->next_cmd = new_exec_cmd;
    else
        exec_cmds = new_exec_cmd;

    last_exec_cmd = new_exec_cmd;

    new_exec_cmd->number = number;
    new_exec_cmd->name = NULL;
    new_exec_cmd->hook = NULL;
    new_exec_cmd->command = NULL;
    new_exec_cmd->pid = 0;
    new_exec_cmd->detached = 0;
    new_exec_cmd->start_time = time (NULL);
    new_exec_cmd->end_time = 0;
    new_exec_cmd->output_to_buffer = 0;
    new_exec_cmd->buffer_full_name = NULL;
    new_exec_cmd->line_numbers = 0;
    new_exec_cmd->display_rc = 0;
    new_exec_cmd->output_line_nb = 0;
    for (i = 0; i < 2; i++)
    {
        new_exec_cmd->output_size[i] = 0;
        new_exec_cmd->output[i] = NULL;
    }
    new_exec_cmd->return_code = -1;
    new_exec_cmd->pipe_command = NULL;
    new_exec_cmd->hsignal = NULL;

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
 * Decodes colors in a string (from stdout/stderr).
 *
 * Returns string with colors as-is, decoded or removed.
 */

char *
exec_decode_color (struct t_exec_cmd *exec_cmd, const char *string)
{
    int irc_color, keep_colors;

    if (!string)
        return NULL;

    irc_color = 0;
    keep_colors = 1;
    switch (exec_cmd->color)
    {
        case EXEC_COLOR_ANSI:
            return strdup (string);
            break;
        case EXEC_COLOR_AUTO:
            irc_color = (exec_cmd->output_to_buffer || exec_cmd->pipe_command);
            break;
        case EXEC_COLOR_IRC:
            irc_color = 1;
            break;
        case EXEC_COLOR_WEECHAT:
            irc_color = 0;
            break;
        case EXEC_COLOR_STRIP:
            keep_colors = 0;
            break;
    }

    return weechat_hook_modifier_exec (
        (irc_color) ? "irc_color_decode_ansi" : "color_decode_ansi",
        (keep_colors) ? "1" : "0",
        string);
}

/*
 * Displays a line of output.
 */

void
exec_display_line (struct t_exec_cmd *exec_cmd, struct t_gui_buffer *buffer,
                   int out, const char *line)
{
    char *line_color, *line2, str_number[32], str_tags[1024];
    int length;

    if (!exec_cmd || !line)
        return;

    /*
     * if output is sent to the buffer, the buffer must exist
     * (we don't send output by default to core buffer)
     */
    if (exec_cmd->output_to_buffer && !exec_cmd->pipe_command && !buffer)
        return;

    /* decode colors */
    line_color = exec_decode_color (exec_cmd, line);
    if (!line_color)
        return;

    exec_cmd->output_line_nb++;

    if (exec_cmd->pipe_command)
    {
        if (strstr (exec_cmd->pipe_command, "$line"))
        {
            /* replace $line by line content */
            line2 = weechat_string_replace (exec_cmd->pipe_command,
                                            "$line", line_color);
            if (line2)
            {
                weechat_command (buffer, line2);
                free (line2);
            }
        }
        else
        {
            /* add line at the end of command, after a space */
            length = strlen (exec_cmd->pipe_command) + 1 + strlen (line_color) + 1;
            line2 = malloc (length);
            if (line2)
            {
                snprintf (line2, length,
                          "%s %s", exec_cmd->pipe_command, line_color);
                weechat_command (buffer, line2);
                free (line2);
            }
        }
    }
    else if (exec_cmd->output_to_buffer)
    {
        if (exec_cmd->line_numbers)
        {
            length = 32 + strlen (line_color) + 1;
            line2 = malloc (length);
            if (line2)
            {
                snprintf (line2, length,
                          "%d. %s", exec_cmd->output_line_nb, line_color);
                weechat_command (buffer, line2);
                free (line2);
            }
        }
        else
            weechat_command (buffer, (line_color[0]) ? line_color : " ");
    }
    else
    {
        snprintf (str_number, sizeof (str_number), "%d", exec_cmd->number);
        snprintf (str_tags, sizeof (str_tags),
                  "exec_%s,exec_cmd_%s",
                  (out == EXEC_STDOUT) ? "stdout" : "stderr",
                  (exec_cmd->name) ? exec_cmd->name : str_number);
        if (weechat_buffer_get_integer (buffer, "type") == 1)
        {
            snprintf (str_number, sizeof (str_number),
                      "%d. ", exec_cmd->output_line_nb);
            weechat_printf_y (buffer, -1,
                              "%s%s",
                              (exec_cmd->line_numbers) ? str_number : " ",
                              line_color);
        }
        else
        {
            snprintf (str_number, sizeof (str_number),
                      "%d\t", exec_cmd->output_line_nb);
            weechat_printf_tags (buffer, str_tags,
                                 "%s%s",
                                 (exec_cmd->line_numbers) ? str_number : " \t",
                                 line_color);
        }
    }
}

/*
 * Concatenates some text to stdout/stderr of a command.
 */

void
exec_concat_output (struct t_exec_cmd *exec_cmd, struct t_gui_buffer *buffer,
                    int out, const char *text)
{
    int length, new_size;
    const char *ptr_text;
    char *new_output, *pos, *line;

    ptr_text = text;

    /* if output is not sent as hsignal, display lines (ending with '\n') */
    if (!exec_cmd->hsignal)
    {
        ptr_text = text;
        while (ptr_text[0])
        {
            pos = strchr (ptr_text, '\n');
            if (!pos)
                break;
            if (exec_cmd->output_size[out] > 0)
            {
                length = exec_cmd->output_size[out] + (pos - ptr_text) + 1;
                line = malloc (length);
                if (line)
                {
                    memcpy (line, exec_cmd->output[out],
                            exec_cmd->output_size[out]);
                    memcpy (line + exec_cmd->output_size[out],
                            ptr_text, pos - ptr_text);
                    line[length - 1] = '\0';
                }
            }
            else
                line = weechat_strndup (ptr_text, pos - ptr_text);
            if (!line)
                break;
            if (exec_cmd->output[out])
            {
                free (exec_cmd->output[out]);
                exec_cmd->output[out] = NULL;
            }
            exec_cmd->output_size[out] = 0;
            exec_display_line (exec_cmd, buffer, out, line);
            free (line);
            ptr_text = pos + 1;
        }
    }

    /* concatenate ptr_text to output buffer */
    length = strlen (ptr_text);
    if (length > 0)
    {
        new_size = exec_cmd->output_size[out] + length;
        new_output = realloc (exec_cmd->output[out], new_size + 1);
        if (!new_output)
            return;
        exec_cmd->output[out] = new_output;
        memcpy (exec_cmd->output[out] + exec_cmd->output_size[out],
                ptr_text, length + 1);
        exec_cmd->output_size[out] = new_size;
    }
}

/*
 * Ends a command.
 */

void
exec_end_command (struct t_exec_cmd *exec_cmd, int return_code)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_hashtable *hashtable;
    char str_number[32], *output;
    int i, buffer_type;

    if (exec_cmd->hsignal)
    {
        hashtable = weechat_hashtable_new (32,
                                           WEECHAT_HASHTABLE_STRING,
                                           WEECHAT_HASHTABLE_STRING,
                                           NULL,
                                           NULL);
        if (hashtable)
        {
            weechat_hashtable_set (hashtable, "command", exec_cmd->command);
            snprintf (str_number, sizeof (str_number), "%d", exec_cmd->number);
            weechat_hashtable_set (hashtable, "number", str_number);
            weechat_hashtable_set (hashtable, "name", exec_cmd->name);
            output = exec_decode_color (exec_cmd, exec_cmd->output[EXEC_STDOUT]);
            weechat_hashtable_set (hashtable, "out", output);
            if (output)
                free (output);
            output = exec_decode_color (exec_cmd, exec_cmd->output[EXEC_STDERR]);
            weechat_hashtable_set (hashtable, "err", output);
            if (output)
                free (output);
            snprintf (str_number, sizeof (str_number), "%d", return_code);
            weechat_hashtable_set (hashtable, "rc", str_number);
            weechat_hook_hsignal_send (exec_cmd->hsignal, hashtable);
            weechat_hashtable_free (hashtable);
        }
    }
    else
    {
        ptr_buffer = weechat_buffer_search ("==", exec_cmd->buffer_full_name);

        /* display the last line of output (if not ending with '\n') */
        exec_display_line (exec_cmd, ptr_buffer, EXEC_STDOUT,
                           exec_cmd->output[EXEC_STDOUT]);
        exec_display_line (exec_cmd, ptr_buffer, EXEC_STDERR,
                           exec_cmd->output[EXEC_STDERR]);

        /*
         * display return code (only if command is not detached, if output is
         * NOT sent to buffer, and if command is not piped)
         */
        if (exec_cmd->display_rc
            && !exec_cmd->detached && !exec_cmd->output_to_buffer
            && !exec_cmd->pipe_command)
        {
            buffer_type = weechat_buffer_get_integer (ptr_buffer, "type");
            if (return_code >= 0)
            {
                if (buffer_type == 1)
                {
                    weechat_printf_y (ptr_buffer, -1,
                                      ("%s: end of command %d (\"%s\"), "
                                       "return code: %d"),
                                      EXEC_PLUGIN_NAME, exec_cmd->number,
                                      exec_cmd->command, return_code);
                }
                else
                {
                    weechat_printf_tags (ptr_buffer, "exec_rc",
                                         _("%s: end of command %d (\"%s\"), "
                                           "return code: %d"),
                                         EXEC_PLUGIN_NAME, exec_cmd->number,
                                         exec_cmd->command, return_code);
                }
            }
            else
            {
                if (buffer_type == 1)
                {
                    weechat_printf_y (ptr_buffer, -1,
                                      _("%s: unexpected end of command %d "
                                        "(\"%s\")"),
                                      EXEC_PLUGIN_NAME, exec_cmd->number,
                                      exec_cmd->command);
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
        }
    }

    /* (re)set some variables after the end of command */
    exec_cmd->hook = NULL;
    exec_cmd->pid = 0;
    exec_cmd->end_time = time (NULL);
    exec_cmd->return_code = return_code;
    for (i = 0; i < 2; i++)
    {
        if (exec_cmd->output[i])
        {
            free (exec_cmd->output[i]);
            exec_cmd->output[i] = NULL;
        }
        exec_cmd->output_size[i] = 0;
    }

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
    struct t_gui_buffer *ptr_buffer;

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

    if (out || err)
    {
        ptr_buffer = weechat_buffer_search ("==",
                                            ptr_exec_cmd->buffer_full_name);
        if (out)
            exec_concat_output (ptr_exec_cmd, ptr_buffer, EXEC_STDOUT, out);
        if (err)
            exec_concat_output (ptr_exec_cmd, ptr_buffer, EXEC_STDERR, err);
    }

    if (return_code == WEECHAT_HOOK_PROCESS_ERROR)
        exec_end_command (ptr_exec_cmd, -1);
    else if (return_code >= 0)
        exec_end_command (ptr_exec_cmd, return_code);

    return WEECHAT_RC_OK;
}

/*
 * Deletes a command.
 */

void
exec_free (struct t_exec_cmd *exec_cmd)
{
    int i;

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
    if (exec_cmd->buffer_full_name)
        free (exec_cmd->buffer_full_name);
    for (i = 0; i < 2; i++)
    {
        if (exec_cmd->output[i])
            free (exec_cmd->output[i]);
    }
    if (exec_cmd->pipe_command)
        free (exec_cmd->pipe_command);
    if (exec_cmd->hsignal)
        free (exec_cmd->hsignal);

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
        weechat_log_printf ("  output_to_buffer. . . . : %d",    ptr_exec_cmd->output_to_buffer);
        weechat_log_printf ("  buffer_full_name. . . . : '%s'",  ptr_exec_cmd->buffer_full_name);
        weechat_log_printf ("  line_numbers. . . . . . : %d",    ptr_exec_cmd->line_numbers);
        weechat_log_printf ("  display_rc. . . . . . . : %d",    ptr_exec_cmd->display_rc);
        weechat_log_printf ("  output_line_nb. . . . . : %d",    ptr_exec_cmd->output_line_nb);
        weechat_log_printf ("  output_size[stdout] . . : %d",    ptr_exec_cmd->output_size[EXEC_STDOUT]);
        weechat_log_printf ("  output[stdout]. . . . . : '%s'",  ptr_exec_cmd->output[EXEC_STDOUT]);
        weechat_log_printf ("  output_size[stderr] . . : %d",    ptr_exec_cmd->output_size[EXEC_STDERR]);
        weechat_log_printf ("  output[stderr]. . . . . : '%s'",  ptr_exec_cmd->output[EXEC_STDERR]);
        weechat_log_printf ("  return_code . . . . . . : %d",    ptr_exec_cmd->return_code);
        weechat_log_printf ("  pipe_command. . . . . . : '%s'",  ptr_exec_cmd->pipe_command);
        weechat_log_printf ("  hsignal . . . . . . . . : '%s'",  ptr_exec_cmd->hsignal);
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
    int i, upgrading;

    weechat_plugin = plugin;

    exec_command_init ();

    if (!exec_config_init ())
        return WEECHAT_RC_ERROR;

    exec_config_read ();

    /* hook some signals */
    weechat_hook_signal ("debug_dump", &exec_debug_dump_cb, NULL);

    /* hook completions */
    exec_completion_init ();

    /* look at arguments */
    upgrading = 0;
    for (i = 0; i < argc; i++)
    {
        if (weechat_strcasecmp (argv[i], "--upgrade") == 0)
        {
            upgrading = 1;
        }
    }

    if (upgrading)
        exec_buffer_set_callbacks ();

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
