/*
 * wee-backtrace.c - backtrace after a segfault
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <dlfcn.h>

#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#endif

#include "weechat.h"
#include "wee-backtrace.h"
#include "wee-log.h"
#include "wee-string.h"
#include "wee-version.h"
#include "../plugins/plugin.h"


/*
 * Displays a backtrace line on standard error output and in WeeChat log.
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
 * Displays function name and line with a backtrace address.
 */

void
weechat_backtrace_addr2line (int number, void *address, const char *symbol)
{
#ifdef HAVE_BACKTRACE
    int rc;
    Dl_info info;
    const void *addr;
    FILE *output;
    char cmd_line[1024];
    char line[1024], *ptr_line, *pos;
    char function_name[1024];

    rc = dladdr (address, &info);
    if ((rc == 0) || !info.dli_fname || !info.dli_fname[0])
    {
        weechat_backtrace_printf ("%03d  %s", number, symbol);
        return;
    }

    addr = address;
    if (info.dli_fbase >= (const void *) 0x40000000)
        addr = (void *)((unsigned long)((const char *) addr) -
                        (unsigned long) info.dli_fbase);

    /* use addr2line to find symbols names */
    snprintf (cmd_line, sizeof (cmd_line),
              "addr2line --functions --demangle -e $(which %s) %p",
              info.dli_fname, addr);
    output = popen (cmd_line, "r");
    if (!output)
    {
        weechat_backtrace_printf ("%03d  %s", number, symbol);
        return;
    }
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
                function_name[0] = '\0';
            }
            else
            {
                if (function_name[0])
                    weechat_backtrace_printf ("%03d  %s",
                                              number, function_name);
                snprintf (function_name, sizeof (function_name),
                          "%s", ptr_line);
            }
        }
    }
    if (function_name[0])
        weechat_backtrace_printf ("%03d  %s",
                                  number, function_name);
    pclose (output);
#else
    /* make C compiler happy */
    (void) number;
    (void) address;
    (void) symbol;

    /* no backtrace possible, we display nothing */
#endif /* HAVE_BACKTRACE */
}

/*
 * Displays backtrace (function called when a SIGSEGV is received).
 */

void
weechat_backtrace ()
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
