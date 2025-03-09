/*
 * hook-process.c - WeeChat process hook
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>

#include "../weechat.h"
#include "../core-hashtable.h"
#include "../core-hook.h"
#include "../core-infolist.h"
#include "../core-log.h"
#include "../core-string.h"
#include "../core-url.h"
#include "../../gui/gui-chat.h"
#include "../../plugins/plugin.h"


int hook_process_pending = 0;          /* 1 if there are some process to    */
                                       /* run (via fork)                    */


void hook_process_run (struct t_hook *hook_process);


/*
 * Returns description of hook.
 *
 * Note: result must be freed after use.
 */

char *
hook_process_get_description (struct t_hook *hook)
{
    char str_desc[1024];

    snprintf (str_desc, sizeof (str_desc),
              "command: \"%s\", child pid: %d",
              HOOK_PROCESS(hook, command),
              HOOK_PROCESS(hook, child_pid));

    return strdup (str_desc);
}

/*
 * Hooks a process (using fork) with options in hashtable.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_process_hashtable (struct t_weechat_plugin *plugin,
                        const char *command,
                        struct t_hashtable *options,
                        int timeout,
                        t_hook_callback_process *callback,
                        const void *callback_pointer,
                        void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_process *new_hook_process;
    char *stdout_buffer, *stderr_buffer, *error;
    const char *ptr_value;
    long number;

    stdout_buffer = NULL;
    stderr_buffer = NULL;
    new_hook = NULL;
    new_hook_process = NULL;

    if (!command || !command[0] || !callback)
        goto error;

    stdout_buffer = malloc (HOOK_PROCESS_BUFFER_SIZE + 1);
    if (!stdout_buffer)
        goto error;

    stderr_buffer = malloc (HOOK_PROCESS_BUFFER_SIZE + 1);
    if (!stderr_buffer)
        goto error;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        goto error;

    new_hook_process = malloc (sizeof (*new_hook_process));
    if (!new_hook_process)
        goto error;

    hook_init_data (new_hook, plugin, HOOK_TYPE_PROCESS, HOOK_PRIORITY_DEFAULT,
                    callback_pointer, callback_data);

    new_hook->hook_data = new_hook_process;
    new_hook_process->callback = callback;
    new_hook_process->command = strdup (command);
    new_hook_process->options = (options) ? hashtable_dup (options) : NULL;
    new_hook_process->detached = (options && hashtable_has_key (options,
                                                                "detached"));
    new_hook_process->timeout = timeout;
    new_hook_process->child_read[HOOK_PROCESS_STDIN] = -1;
    new_hook_process->child_read[HOOK_PROCESS_STDOUT] = -1;
    new_hook_process->child_read[HOOK_PROCESS_STDERR] = -1;
    new_hook_process->child_write[HOOK_PROCESS_STDIN] = -1;
    new_hook_process->child_write[HOOK_PROCESS_STDOUT] = -1;
    new_hook_process->child_write[HOOK_PROCESS_STDERR] = -1;
    new_hook_process->child_pid = 0;
    new_hook_process->hook_fd[HOOK_PROCESS_STDIN] = NULL;
    new_hook_process->hook_fd[HOOK_PROCESS_STDOUT] = NULL;
    new_hook_process->hook_fd[HOOK_PROCESS_STDERR] = NULL;
    new_hook_process->hook_timer = NULL;
    new_hook_process->buffer[HOOK_PROCESS_STDIN] = NULL;
    new_hook_process->buffer[HOOK_PROCESS_STDOUT] = stdout_buffer;
    new_hook_process->buffer[HOOK_PROCESS_STDERR] = stderr_buffer;
    new_hook_process->buffer_size[HOOK_PROCESS_STDIN] = 0;
    new_hook_process->buffer_size[HOOK_PROCESS_STDOUT] = 0;
    new_hook_process->buffer_size[HOOK_PROCESS_STDERR] = 0;
    new_hook_process->buffer_flush = HOOK_PROCESS_BUFFER_SIZE;
    if (options)
    {
        ptr_value = hashtable_get (options, "buffer_flush");
        if (ptr_value && ptr_value[0])
        {
            error = NULL;
            number = strtol (ptr_value, &error, 10);
            if (error && !error[0]
                && (number >= 1) && (number <= HOOK_PROCESS_BUFFER_SIZE))
            {
                new_hook_process->buffer_flush = (int)number;
            }
        }
    }

    hook_add_to_list (new_hook);

    if (weechat_debug_core >= 1)
    {
        gui_chat_printf (NULL,
                         "debug: hook_process: command=\"%s\", "
                         "options=\"%s\", timeout=%d",
                         new_hook_process->command,
                         hashtable_get_string (new_hook_process->options,
                                               "keys_values"),
                         new_hook_process->timeout);
    }

    if (strncmp (new_hook_process->command, "func:", 5) == 0)
        hook_process_pending = 1;
    else
        hook_process_run (new_hook);

    return new_hook;

error:
    free (stdout_buffer);
    free (stderr_buffer);
    free (new_hook);
    free (new_hook_process);
    return NULL;
}

/*
 * Hooks a process (using fork).
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_process (struct t_weechat_plugin *plugin,
              const char *command,
              int timeout,
              t_hook_callback_process *callback,
              const void *callback_pointer,
              void *callback_data)
{
    return hook_process_hashtable (plugin, command, NULL, timeout,
                                   callback, callback_pointer, callback_data);
}

/*
 * Child process for hook process: executes command and returns string result
 * into pipe for WeeChat process.
 */

void
hook_process_child (struct t_hook *hook_process)
{
    char **exec_args, *arg0, str_arg[64], **ptr_exec_arg;
    const char *ptr_url, *ptr_arg;
    int rc, i, num_args;
    FILE *f;

    /* read stdin from parent, if a pipe was defined */
    if (HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDIN]) >= 0)
    {
        if (dup2 (HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDIN]),
                  STDIN_FILENO) < 0)
        {
            _exit (EXIT_FAILURE);
        }
    }
    else
    {
        /* no stdin pipe from parent, use "/dev/null" for stdin stream */
        f = freopen ("/dev/null", "r", stdin);
        (void) f;
    }
    if (HOOK_PROCESS(hook_process, child_write[HOOK_PROCESS_STDIN]) >= 0)
        close (HOOK_PROCESS(hook_process, child_write[HOOK_PROCESS_STDIN]));

    /* redirect stdout/stderr to pipe (so that parent process can read them) */
    if (HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDOUT]) >= 0)
    {
        close (HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDOUT]));
        if (dup2 (HOOK_PROCESS(hook_process, child_write[HOOK_PROCESS_STDOUT]),
              STDOUT_FILENO) < 0)
        {
            _exit (EXIT_FAILURE);
        }
    }
    else
    {
        /* detached mode: write stdout in /dev/null */
        f = freopen ("/dev/null", "w", stdout);
        (void) f;
    }
    if (HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDERR]) >= 0)
    {
        close (HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDERR]));
        if (dup2 (HOOK_PROCESS(hook_process, child_write[HOOK_PROCESS_STDERR]),
                  STDERR_FILENO) < 0)
        {
            _exit (EXIT_FAILURE);
        }
    }
    else
    {
        /* detached mode: write stderr in /dev/null */
        f = freopen ("/dev/null", "w", stderr);
        (void) f;
    }

    rc = EXIT_FAILURE;

    if (strncmp (HOOK_PROCESS(hook_process, command), "url:", 4) == 0)
    {
        /* get URL output (on stdout or file, depending on options) */
        ptr_url = HOOK_PROCESS(hook_process, command) + 4;
        while (ptr_url[0] == ' ')
        {
            ptr_url++;
        }
        rc = weeurl_download (ptr_url,
                              HOOK_PROCESS(hook_process, options),
                              NULL);  /* output */
    }
    else if (strncmp (HOOK_PROCESS(hook_process, command), "func:", 5) == 0)
    {
        /* run a function (via the hook callback) */
        rc = (int) (HOOK_PROCESS(hook_process, callback))
            (hook_process->callback_pointer,
             hook_process->callback_data,
             HOOK_PROCESS(hook_process, command),
             WEECHAT_HOOK_PROCESS_CHILD,
             NULL, NULL);
    }
    else
    {
        /* launch command */
        num_args = 0;
        if (HOOK_PROCESS(hook_process, options))
        {
            /*
             * count number of arguments given in the hashtable options,
             * keys are: "arg1", "arg2", ...
             */
            while (1)
            {
                snprintf (str_arg, sizeof (str_arg), "arg%d", num_args + 1);
                ptr_arg = hashtable_get (HOOK_PROCESS(hook_process, options),
                                         str_arg);
                if (!ptr_arg)
                    break;
                num_args++;
            }
        }
        if (num_args > 0)
        {
            /*
             * if at least one argument was found in hashtable option, the
             * "command" contains only path to binary (without arguments), and
             * the arguments are in hashtable
             */
            exec_args = malloc ((num_args + 2) * sizeof (exec_args[0]));
            if (exec_args)
            {
                exec_args[0] = strdup (HOOK_PROCESS(hook_process, command));
                for (i = 1; i <= num_args; i++)
                {
                    snprintf (str_arg, sizeof (str_arg), "arg%d", i);
                    ptr_arg = hashtable_get (HOOK_PROCESS(hook_process, options),
                                             str_arg);
                    exec_args[i] = (ptr_arg) ? strdup (ptr_arg) : NULL;
                }
                exec_args[num_args + 1] = NULL;
            }
        }
        else
        {
            /*
             * if no arguments were found in hashtable, make an automatic split
             * of command, like the shell does
             */
            exec_args = string_split_shell (HOOK_PROCESS(hook_process, command),
                                            NULL);
        }

        if (exec_args)
        {
            arg0 = string_expand_home (exec_args[0]);
            if (arg0)
            {
                free (exec_args[0]);
                exec_args[0] = arg0;
            }
            if (weechat_debug_core >= 1)
            {
                log_printf ("hook_process, command='%s'",
                            HOOK_PROCESS(hook_process, command));
                for (ptr_exec_arg = exec_args, i = 0; *ptr_exec_arg;
                     ptr_exec_arg++, i++)
                {
                    log_printf ("  args[%d] == '%s'", i, *ptr_exec_arg);
                }
            }
            execvp (exec_args[0], exec_args);
        }

        /* should not be executed if execvp was OK */
        string_free_split (exec_args);
        fprintf (stderr, "Error with command '%s'\n",
                 HOOK_PROCESS(hook_process, command));
    }

    fflush (stdout);
    fflush (stderr);

    _exit (rc);
}

/*
 * Sends buffers (stdout/stderr) to callback.
 */

void
hook_process_send_buffers (struct t_hook *hook_process, int callback_rc)
{
    int size;

    /* add '\0' at end of stdout and stderr */
    size = HOOK_PROCESS(hook_process, buffer_size[HOOK_PROCESS_STDOUT]);
    if (size > 0)
        HOOK_PROCESS(hook_process, buffer[HOOK_PROCESS_STDOUT])[size] = '\0';
    size = HOOK_PROCESS(hook_process, buffer_size[HOOK_PROCESS_STDERR]);
    if (size > 0)
        HOOK_PROCESS(hook_process, buffer[HOOK_PROCESS_STDERR])[size] = '\0';

    /* send buffers to callback */
    (void) (HOOK_PROCESS(hook_process, callback))
        (hook_process->callback_pointer,
         hook_process->callback_data,
         HOOK_PROCESS(hook_process, command),
         callback_rc,
         (HOOK_PROCESS(hook_process, buffer_size[HOOK_PROCESS_STDOUT]) > 0) ?
         HOOK_PROCESS(hook_process, buffer[HOOK_PROCESS_STDOUT]) : NULL,
         (HOOK_PROCESS(hook_process, buffer_size[HOOK_PROCESS_STDERR]) > 0) ?
         HOOK_PROCESS(hook_process, buffer[HOOK_PROCESS_STDERR]) : NULL);

    /* reset size for stdout and stderr */
    HOOK_PROCESS(hook_process, buffer_size[HOOK_PROCESS_STDOUT]) = 0;
    HOOK_PROCESS(hook_process, buffer_size[HOOK_PROCESS_STDERR]) = 0;
}

/*
 * Adds some data to buffer (stdout or stderr).
 */

void
hook_process_add_to_buffer (struct t_hook *hook_process, int index_buffer,
                            const char *buffer, int size)
{
    if (HOOK_PROCESS(hook_process, buffer_size[index_buffer]) + size > HOOK_PROCESS_BUFFER_SIZE)
        hook_process_send_buffers (hook_process, WEECHAT_HOOK_PROCESS_RUNNING);

    memcpy (HOOK_PROCESS(hook_process, buffer[index_buffer]) +
            HOOK_PROCESS(hook_process, buffer_size[index_buffer]),
            buffer, size);
    HOOK_PROCESS(hook_process, buffer_size[index_buffer]) += size;
}

/*
 * Reads process output (stdout or stderr) from child process.
 */

void
hook_process_child_read (struct t_hook *hook_process, int fd,
                         int index_buffer, struct t_hook **hook_fd)
{
    char buffer[HOOK_PROCESS_BUFFER_SIZE / 8];
    int num_read;

    if (hook_process->deleted)
        return;

    num_read = read (fd, buffer, sizeof (buffer) - 1);
    if (num_read > 0)
    {
        hook_process_add_to_buffer (hook_process, index_buffer,
                                    buffer, num_read);
        if (HOOK_PROCESS(hook_process, buffer_size[index_buffer]) >=
            HOOK_PROCESS(hook_process, buffer_flush))
        {
            hook_process_send_buffers (hook_process,
                                       WEECHAT_HOOK_PROCESS_RUNNING);
        }
    }
    else if (num_read == 0)
    {
        unhook (*hook_fd);
        *hook_fd = NULL;
    }
}

/*
 * Reads process output (stdout) from child process.
 */

int
hook_process_child_read_stdout_cb (const void *pointer, void *data, int fd)
{
    struct t_hook *hook_process;

    /* make C compiler happy */
    (void) data;

    hook_process = (struct t_hook *)pointer;

    hook_process_child_read (hook_process, fd, HOOK_PROCESS_STDOUT,
                             &(HOOK_PROCESS(hook_process, hook_fd[HOOK_PROCESS_STDOUT])));
    return WEECHAT_RC_OK;
}

/*
 * Reads process output (stderr) from child process.
 */

int
hook_process_child_read_stderr_cb (const void *pointer, void *data, int fd)
{
    struct t_hook *hook_process;

    /* make C compiler happy */
    (void) data;

    hook_process = (struct t_hook *)pointer;

    hook_process_child_read (hook_process, fd, HOOK_PROCESS_STDERR,
                             &(HOOK_PROCESS(hook_process, hook_fd[HOOK_PROCESS_STDERR])));
    return WEECHAT_RC_OK;
}

/*
 * Reads process output from child process until EOF
 * (called when the child process has ended).
 */

void
hook_process_child_read_until_eof (struct t_hook *hook_process)
{
    struct pollfd poll_fd[2];
    int count, fd_stdout, fd_stderr, num_fd, ready, i;

    fd_stdout = HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDOUT]);
    fd_stderr = HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDERR]);

    /*
     * use a counter to prevent any infinite loop
     * (if child's output is very very long...)
     */
    count = 0;
    while (count < 1024)
    {
        num_fd = 0;

        if (HOOK_PROCESS(hook_process, hook_fd[HOOK_PROCESS_STDOUT])
            && ((fcntl (fd_stdout, F_GETFD) != -1) || (errno != EBADF)))
        {
            poll_fd[num_fd].fd = fd_stdout;
            poll_fd[num_fd].events = POLLIN;
            poll_fd[num_fd].revents = 0;
            num_fd++;
        }

        if (HOOK_PROCESS(hook_process, hook_fd[HOOK_PROCESS_STDERR])
            && ((fcntl (fd_stderr, F_GETFD) != -1) || (errno != EBADF)))
        {
            poll_fd[num_fd].fd = fd_stderr;
            poll_fd[num_fd].events = POLLIN;
            poll_fd[num_fd].revents = 0;
            num_fd++;
        }

        if (num_fd == 0)
            break;

        ready = poll (poll_fd, num_fd, 0);

        if (ready <= 0)
            break;

        for (i = 0; i < num_fd; i++)
        {
            if (poll_fd[i].revents & POLLIN)
            {
                if (poll_fd[i].fd == fd_stdout)
                {
                    (void) hook_process_child_read_stdout_cb (
                        hook_process,
                        NULL,
                        HOOK_PROCESS(hook_process,
                                     child_read[HOOK_PROCESS_STDOUT]));
                }
                else
                {
                    (void) hook_process_child_read_stderr_cb (
                        hook_process,
                        NULL,
                        HOOK_PROCESS(hook_process,
                                     child_read[HOOK_PROCESS_STDERR]));
                }
            }
        }

        count++;
    }
}

/*
 * Checks if child process is still alive.
 */

int
hook_process_timer_cb (const void *pointer, void *data, int remaining_calls)
{
    struct t_hook *hook_process;
    int status, rc;

    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    hook_process = (struct t_hook *)pointer;

    if (hook_process->deleted)
        return WEECHAT_RC_OK;

    if (remaining_calls == 0)
    {
        hook_process_send_buffers (hook_process, WEECHAT_HOOK_PROCESS_ERROR);
        if (weechat_debug_core >= 1)
        {
            gui_chat_printf (NULL,
                             _("End of command '%s', timeout reached (%.3fs)"),
                             HOOK_PROCESS(hook_process, command),
                             ((float)HOOK_PROCESS(hook_process, timeout)) / 1000);
        }
        kill (HOOK_PROCESS(hook_process, child_pid), SIGKILL);
        unhook (hook_process);
    }
    else
    {
        if (waitpid (HOOK_PROCESS(hook_process, child_pid),
                     &status, WNOHANG) > 0)
        {
            if (WIFEXITED(status))
            {
                /* child terminated normally */
                rc = WEXITSTATUS(status);
                hook_process_child_read_until_eof (hook_process);
                hook_process_send_buffers (hook_process, rc);
                unhook (hook_process);
            }
            else if (WIFSIGNALED(status))
            {
                /* child terminated by a signal */
                hook_process_child_read_until_eof (hook_process);
                hook_process_send_buffers (hook_process,
                                           WEECHAT_HOOK_PROCESS_ERROR);
                unhook (hook_process);
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Executes process command in child, and read data in current process,
 * with fd hook.
 */

void
hook_process_run (struct t_hook *hook_process)
{
    int pipes[3][2], timeout, max_calls, rc, i;
    char str_error[1024];
    long interval;
    pid_t pid;

    for (i = 0; i < 3; i++)
    {
        pipes[i][0] = -1;
        pipes[i][1] = -1;
    }

    /* create pipe for stdin (only if stdin was given in options) */
    if (HOOK_PROCESS(hook_process, options)
        && hashtable_has_key (HOOK_PROCESS(hook_process, options), "stdin"))
    {
        if (pipe (pipes[HOOK_PROCESS_STDIN]) < 0)
            goto error;
    }

    /* create pipes for stdout/err (if not running in detached mode) */
    if (!HOOK_PROCESS(hook_process, detached))
    {
        if (pipe (pipes[HOOK_PROCESS_STDOUT]) < 0)
            goto error;
        if (pipe (pipes[HOOK_PROCESS_STDERR]) < 0)
            goto error;
    }

    /* assign pipes to variables in hook */
    for (i = 0; i < 3; i++)
    {
        HOOK_PROCESS(hook_process, child_read[i]) = pipes[i][0];
        HOOK_PROCESS(hook_process, child_write[i]) = pipes[i][1];
    }

    /* flush stdout and stderr before forking */
    fflush (stdout);
    fflush (stderr);

    /* fork */
    switch (pid = fork ())
    {
        /* fork failed */
        case -1:
            snprintf (str_error, sizeof (str_error),
                      "fork error: %s",
                      strerror (errno));
            (void) (HOOK_PROCESS(hook_process, callback))
                (hook_process->callback_pointer,
                 hook_process->callback_data,
                 HOOK_PROCESS(hook_process, command),
                 WEECHAT_HOOK_PROCESS_ERROR,
                 NULL, str_error);
            unhook (hook_process);
            return;
        /* child process */
        case 0:
            rc = setuid (getuid ());
            (void) rc;
            hook_process_child (hook_process);
            /* never executed */
            _exit (EXIT_SUCCESS);
            break;
    }

    /* parent process */
    HOOK_PROCESS(hook_process, child_pid) = pid;
    if (HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDIN]) >= 0)
    {
        close (HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDIN]));
        HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDIN]) = -1;
    }
    if (HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDOUT]) >= 0)
    {
        close (HOOK_PROCESS(hook_process, child_write[HOOK_PROCESS_STDOUT]));
        HOOK_PROCESS(hook_process, child_write[HOOK_PROCESS_STDOUT]) = -1;
    }
    if (HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDERR]) >= 0)
    {
        close (HOOK_PROCESS(hook_process, child_write[HOOK_PROCESS_STDERR]));
        HOOK_PROCESS(hook_process, child_write[HOOK_PROCESS_STDERR]) = -1;
    }

    if (HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDOUT]) >= 0)
    {
        HOOK_PROCESS(hook_process, hook_fd[HOOK_PROCESS_STDOUT]) =
            hook_fd (hook_process->plugin,
                     HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDOUT]),
                     1, 0, 0,
                     &hook_process_child_read_stdout_cb,
                     hook_process, NULL);
    }

    if (HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDERR]) >= 0)
    {
        HOOK_PROCESS(hook_process, hook_fd[HOOK_PROCESS_STDERR]) =
            hook_fd (hook_process->plugin,
                     HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDERR]),
                     1, 0, 0,
                     &hook_process_child_read_stderr_cb,
                     hook_process, NULL);
    }

    timeout = HOOK_PROCESS(hook_process, timeout);
    interval = 100;
    max_calls = 0;
    if (timeout > 0)
    {
        if (timeout <= 100)
        {
            interval = timeout;
            max_calls = 1;
        }
        else
        {
            interval = 100;
            max_calls = timeout / 100;
            if (timeout % 100 == 0)
                max_calls++;
        }
    }
    HOOK_PROCESS(hook_process, hook_timer) = hook_timer (hook_process->plugin,
                                                         interval, 0, max_calls,
                                                         &hook_process_timer_cb,
                                                         hook_process,
                                                         NULL);
    return;

error:
    for (i = 0; i < 3; i++)
    {
        if (pipes[i][0] >= 0)
            close (pipes[i][0]);
        if (pipes[i][1] >= 0)
            close (pipes[i][1]);
    }
    (void) (HOOK_PROCESS(hook_process, callback))
        (hook_process->callback_pointer,
         hook_process->callback_data,
         HOOK_PROCESS(hook_process, command),
         WEECHAT_HOOK_PROCESS_ERROR,
         NULL, NULL);
    unhook (hook_process);
}

/*
 * Executes all process commands pending.
 */

void
hook_process_exec (void)
{
    struct t_hook *ptr_hook, *next_hook;
    struct t_hook_exec_cb hook_exec_cb;

    hook_exec_start ();

    ptr_hook = weechat_hooks[HOOK_TYPE_PROCESS];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (HOOK_PROCESS(ptr_hook, child_pid) == 0))
        {
            hook_callback_start (ptr_hook, &hook_exec_cb);
            hook_process_run (ptr_hook);
            hook_callback_end (ptr_hook, &hook_exec_cb);
        }

        ptr_hook = next_hook;
    }

    hook_exec_end ();

    hook_process_pending = 0;
}

/*
 * Frees data in a process hook.
 */

void
hook_process_free_data (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    if (HOOK_PROCESS(hook, command))
    {
        free (HOOK_PROCESS(hook, command));
        HOOK_PROCESS(hook, command) = NULL;
    }
    if (HOOK_PROCESS(hook, options))
    {
        hashtable_free (HOOK_PROCESS(hook, options));
        HOOK_PROCESS(hook, options) = NULL;
    }
    if (HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDIN]))
    {
        unhook (HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDIN]));
        HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDIN]) = NULL;
    }
    if (HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDOUT]))
    {
        unhook (HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDOUT]));
        HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDOUT]) = NULL;
    }
    if (HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDERR]))
    {
        unhook (HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDERR]));
        HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDERR]) = NULL;
    }
    if (HOOK_PROCESS(hook, hook_timer))
    {
        unhook (HOOK_PROCESS(hook, hook_timer));
        HOOK_PROCESS(hook, hook_timer) = NULL;
    }
    if (HOOK_PROCESS(hook, child_pid) > 0)
    {
        kill (HOOK_PROCESS(hook, child_pid), SIGKILL);
        hook_schedule_clean_process (HOOK_PROCESS(hook, child_pid));
        HOOK_PROCESS(hook, child_pid) = 0;
    }
    if (HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDIN]) != -1)
    {
        close (HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDIN]));
        HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDIN]) = -1;
    }
    if (HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDIN]) != -1)
    {
        close (HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDIN]));
        HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDIN]) = -1;
    }
    if (HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDOUT]) != -1)
    {
        close (HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDOUT]));
        HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDOUT]) = -1;
    }
    if (HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDOUT]) != -1)
    {
        close (HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDOUT]));
        HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDOUT]) = -1;
    }
    if (HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDERR]) != -1)
    {
        close (HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDERR]));
        HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDERR]) = -1;
    }
    if (HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDERR]) != -1)
    {
        close (HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDERR]));
        HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDERR]) = -1;
    }
    if (HOOK_PROCESS(hook, buffer[HOOK_PROCESS_STDIN]))
    {
        free (HOOK_PROCESS(hook, buffer[HOOK_PROCESS_STDIN]));
        HOOK_PROCESS(hook, buffer[HOOK_PROCESS_STDIN]) = NULL;
    }
    if (HOOK_PROCESS(hook, buffer[HOOK_PROCESS_STDOUT]))
    {
        free (HOOK_PROCESS(hook, buffer[HOOK_PROCESS_STDOUT]));
        HOOK_PROCESS(hook, buffer[HOOK_PROCESS_STDOUT]) = NULL;
    }
    if (HOOK_PROCESS(hook, buffer[HOOK_PROCESS_STDERR]))
    {
        free (HOOK_PROCESS(hook, buffer[HOOK_PROCESS_STDERR]));
        HOOK_PROCESS(hook, buffer[HOOK_PROCESS_STDERR]) = NULL;
    }

    free (hook->hook_data);
    hook->hook_data = NULL;
}

/*
 * Adds process hook data in the infolist item.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_process_add_to_infolist (struct t_infolist_item *item,
                              struct t_hook *hook)
{
    if (!item || !hook || !hook->hook_data)
        return 0;

    if (!infolist_new_var_pointer (item, "callback", HOOK_PROCESS(hook, callback)))
        return 0;
    if (!infolist_new_var_string (item, "command", HOOK_PROCESS(hook, command)))
        return 0;
    if (!infolist_new_var_string (item, "options", hashtable_get_string (HOOK_PROCESS(hook, options), "keys_values")))
        return 0;
    if (!infolist_new_var_integer (item, "detached", HOOK_PROCESS(hook, detached)))
        return 0;
    if (!infolist_new_var_integer (item, "timeout", (int)(HOOK_PROCESS(hook, timeout))))
        return 0;
    if (!infolist_new_var_integer (item, "child_read_stdin", HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDIN])))
        return 0;
    if (!infolist_new_var_integer (item, "child_write_stdin", HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDIN])))
        return 0;
    if (!infolist_new_var_integer (item, "child_read_stdout", HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDOUT])))
        return 0;
    if (!infolist_new_var_integer (item, "child_write_stdout", HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDOUT])))
        return 0;
    if (!infolist_new_var_integer (item, "child_read_stderr", HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDERR])))
        return 0;
    if (!infolist_new_var_integer (item, "child_write_stderr", HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDERR])))
        return 0;
    if (!infolist_new_var_integer (item, "child_pid", HOOK_PROCESS(hook, child_pid)))
        return 0;
    if (!infolist_new_var_pointer (item, "hook_fd_stdin", HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDIN])))
        return 0;
    if (!infolist_new_var_pointer (item, "hook_fd_stdout", HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDOUT])))
        return 0;
    if (!infolist_new_var_pointer (item, "hook_fd_stderr", HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDERR])))
        return 0;
    if (!infolist_new_var_pointer (item, "hook_timer", HOOK_PROCESS(hook, hook_timer)))
        return 0;

    return 1;
}

/*
 * Prints process hook data in WeeChat log file (usually for crash dump).
 */

void
hook_process_print_log (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    log_printf ("  process data:");
    log_printf ("    callback. . . . . . . : %p", HOOK_PROCESS(hook, callback));
    log_printf ("    command . . . . . . . : '%s'", HOOK_PROCESS(hook, command));
    log_printf ("    options . . . . . . . : %p (hashtable: '%s')",
                HOOK_PROCESS(hook, options),
                hashtable_get_string (HOOK_PROCESS(hook, options),
                                      "keys_values"));
    log_printf ("    detached. . . . . . . : %d", HOOK_PROCESS(hook, detached));
    log_printf ("    timeout . . . . . . . : %ld", HOOK_PROCESS(hook, timeout));
    log_printf ("    child_read[stdin] . . : %d", HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDIN]));
    log_printf ("    child_write[stdin]. . : %d", HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDIN]));
    log_printf ("    child_read[stdout]. . : %d", HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDOUT]));
    log_printf ("    child_write[stdout] . : %d", HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDOUT]));
    log_printf ("    child_read[stderr]. . : %d", HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDERR]));
    log_printf ("    child_write[stderr] . : %d", HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDERR]));
    log_printf ("    child_pid . . . . . . : %d", HOOK_PROCESS(hook, child_pid));
    log_printf ("    hook_fd[stdin]. . . . : %p", HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDIN]));
    log_printf ("    hook_fd[stdout] . . . : %p", HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDOUT]));
    log_printf ("    hook_fd[stderr] . . . : %p", HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDERR]));
    log_printf ("    hook_timer. . . . . . : %p", HOOK_PROCESS(hook, hook_timer));
}
