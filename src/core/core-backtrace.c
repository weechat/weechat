/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Backtrace after a segfault */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <dlfcn.h>

#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#endif

#include "weechat.h"
#include "core-backtrace.h"
#include "core-log.h"
#include "core-string.h"
#include "core-version.h"
#include "../plugins/plugin.h"


/*
 * Display a backtrace line on standard error output and in WeeChat log.
 */

void
weechat_backtrace_printf (const char *message, ...)
{
    weechat_va_format (message);
    if (vbuffer)
    {
        string_fprintf (stderr, "%s\n", vbuffer);
        log_printf ("%s", vbuffer);
        free (vbuffer);
    }
}

/*
 * Find the file to give to addr2line, using the file name returned by dladdr.
 *
 * If filename contains a "/", it is used as-is, otherwise it is searched
 * in directories of environment variable PATH (like the shell does to find
 * a command).
 *
 * The full path is stored in "path" (its size is "path_size" bytes).
 *
 * No memory is dynamically allocated by this function, because it is called
 * after a crash.
 *
 * Returns:
 *   1: file found (path is set)
 *   0: file not found (path is set to empty string if path_size > 0)
 */

int
weechat_backtrace_find_file (const char *filename, char *path, int path_size)
{
    const char *env_path, *ptr_dir, *pos;
    int length_dir, length_filename;
    struct stat st;

    if (!path || (path_size <= 0))
        return 0;

    path[0] = '\0';

    if (!filename || !filename[0])
        return 0;

    length_filename = strlen (filename);

    if (strchr (filename, '/'))
    {
        if (length_filename >= path_size)
            return 0;
        memcpy (path, filename, length_filename + 1);
        return 1;
    }

    env_path = getenv ("PATH");
    if (!env_path || !env_path[0])
        return 0;

    ptr_dir = env_path;
    while (ptr_dir)
    {
        pos = strchr (ptr_dir, ':');
        length_dir = (pos) ? (int)(pos - ptr_dir) : (int)strlen (ptr_dir);
        /* An empty directory in PATH means the current directory. */
        if (length_dir == 0)
        {
            ptr_dir = ".";
            length_dir = 1;
        }
        if (length_dir + 1 + length_filename < path_size)
        {
            memcpy (path, ptr_dir, length_dir);
            path[length_dir] = '/';
            memcpy (path + length_dir + 1, filename, length_filename + 1);
            if ((stat (path, &st) == 0)
                && S_ISREG(st.st_mode)
                && (access (path, X_OK) == 0))
            {
                return 1;
            }
        }
        ptr_dir = (pos) ? pos + 1 : NULL;
    }

    path[0] = '\0';
    return 0;
}

/*
 * Display function name and line with a backtrace address.
 */

void
weechat_backtrace_addr2line (int number, void *address, const char *symbol)
{
#ifdef HAVE_BACKTRACE
    int rc, fd_pipe[2], lines_displayed;
    pid_t pid;
    Dl_info info;
    const void *addr;
    FILE *output;
    char path[4096], str_addr[64];
    char *args[7];
    char line[1024], *ptr_line, *pos;
    char function_name[1024];

    rc = dladdr (address, &info);
    if ((rc == 0) || !info.dli_fname || !info.dli_fname[0]
        || !weechat_backtrace_find_file (info.dli_fname, path, sizeof (path)))
    {
        weechat_backtrace_printf ("%03d  %s", number, symbol);
        return;
    }

    addr = address;
    if (info.dli_fbase >= (const void *) 0x40000000)
        addr = (void *)((unsigned long)((const char *) addr) -
                        (unsigned long) info.dli_fbase);

    snprintf (str_addr, sizeof (str_addr), "%p", addr);

    /*
     * Use addr2line to find symbols names; it is executed without a shell,
     * so the file name is given as-is, whatever chars it contains.
     */
    if (pipe (fd_pipe) < 0)
    {
        weechat_backtrace_printf ("%03d  %s", number, symbol);
        return;
    }
    pid = fork ();
    if (pid < 0)
    {
        close (fd_pipe[0]);
        close (fd_pipe[1]);
        weechat_backtrace_printf ("%03d  %s", number, symbol);
        return;
    }
    if (pid == 0)
    {
        /* Child process: execute addr2line with output sent to the pipe. */
        close (fd_pipe[0]);
        if (dup2 (fd_pipe[1], STDOUT_FILENO) < 0)
            _exit (EXIT_FAILURE);
        close (fd_pipe[1]);
        args[0] = "addr2line";
        args[1] = "--functions";
        args[2] = "--demangle";
        args[3] = "-e";
        args[4] = path;
        args[5] = str_addr;
        args[6] = NULL;
        execvp (args[0], args);
        _exit (EXIT_FAILURE);
    }

    /* Parent process: read output of addr2line. */
    close (fd_pipe[1]);
    output = fdopen (fd_pipe[0], "r");
    if (!output)
    {
        close (fd_pipe[0]);
        waitpid (pid, NULL, 0);
        weechat_backtrace_printf ("%03d  %s", number, symbol);
        return;
    }
    lines_displayed = 0;
    function_name[0] = '\0';
    while (!feof (output))
    {
        ptr_line = fgets (line, sizeof (line) - 1, output);
        if (ptr_line && ptr_line[0])
        {
            pos = strchr (ptr_line, '\n');
            if (pos)
                pos[0] = '\0';
            if (strchr (ptr_line, ':'))
            {
                weechat_backtrace_printf (
                    "%03d  %s%s%s%s",
                    number,
                    ptr_line,
                    (function_name[0]) ? " [function " : "",
                    function_name,
                    (function_name[0]) ? "]" : "");
                lines_displayed++;
                function_name[0] = '\0';
            }
            else
            {
                if (function_name[0])
                {
                    weechat_backtrace_printf ("%03d  %s",
                                              number, function_name);
                    lines_displayed++;
                }
                snprintf (function_name, sizeof (function_name),
                          "%s", ptr_line);
            }
        }
    }
    if (function_name[0])
    {
        weechat_backtrace_printf ("%03d  %s",
                                  number, function_name);
        lines_displayed++;
    }
    fclose (output);
    waitpid (pid, NULL, 0);

    /* Nothing displayed (addr2line not found?): display the symbol. */
    if (lines_displayed == 0)
        weechat_backtrace_printf ("%03d  %s", number, symbol);
#else
    /* Make C compiler happy. */
    (void) number;
    (void) address;
    (void) symbol;

    /* No backtrace possible, we display nothing. */
#endif /* HAVE_BACKTRACE */
}

/*
 * Display backtrace (function called when a SIGSEGV is received).
 */

void
weechat_backtrace (void)
{
#ifdef HAVE_BACKTRACE
    void *trace[BACKTRACE_MAX];
    int trace_size, i;
    char **symbols;
#endif /* HAVE_BACKTRACE */

    weechat_backtrace_printf ("======= WeeChat backtrace =======");
    weechat_backtrace_printf ("(written by WeeChat %s, compiled on %s %s)",
                              version_get_version_with_git (),
                              version_get_compilation_date (),
                              version_get_compilation_time ());

#ifdef HAVE_BACKTRACE
    trace_size = backtrace (trace, BACKTRACE_MAX);
    symbols = backtrace_symbols (trace, trace_size);

    for (i = 0; i < trace_size; i++)
    {
        weechat_backtrace_addr2line (i + 1, trace[i], symbols[i]);
    }
#else
    weechat_backtrace_printf ("  No backtrace info (no debug info available "
                              "or no backtrace possible on your system).");
#endif /* HAVE_BACKTRACE */

    weechat_backtrace_printf ("======= End of  backtrace =======");
}
