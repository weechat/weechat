/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* wee-backtrace.c: display backtrace after a segfault */


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


/*
 * weechat_backtrace_printf: display a backtrace line (on stderr and in WeeChat log)
 */

void
weechat_backtrace_printf (char *message, ...)
{
    static char buffer[4096];
    va_list argptr;
    
    va_start (argptr, message);
    vsnprintf (buffer, sizeof (buffer) - 1, message, argptr);
    va_end (argptr);

    string_iconv_fprintf (stderr, "%s", buffer);
    log_printf ("%s", buffer);
}

/*
 * weechat_backtrace_addr2line: display function name and line with a backtrace address
 */

void
weechat_backtrace_addr2line (int number, void *address, char *symbol)
{
#ifdef HAVE_BACKTRACE
    int rc;
    Dl_info info;
    const void *addr;
    FILE *output;
    char cmd_line[1024];
    char line[1024], *ptr_line, *pos;
    char function_name[1024];
    int file_line;
    
    rc = dladdr (address, &info);
    if ((rc == 0) || !info.dli_fname || !info.dli_fname[0])
    {
        weechat_backtrace_printf ("%03d  %s\n", number, symbol);
        return;
    }
    
    addr = address;
    if (info.dli_fbase >= (const void *) 0x40000000)
        addr = (void *)((unsigned long)((const char *) addr) - (unsigned long) info.dli_fbase);
    
    snprintf (cmd_line, sizeof (cmd_line),
              "addr2line --functions --demangle -e $(which %s) %p",
              info.dli_fname, addr);
    output = popen (cmd_line, "r");
    if (!output)
    {
        weechat_backtrace_printf ("%03d  %s\n", number, symbol);
        return;
    }
    function_name[0] = '\0';
    file_line = 0;
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
                file_line = 1;
                weechat_backtrace_printf ("%03d  %s%s%s%s\n",
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
        weechat_backtrace_printf ("%03d  %s\n",
                                  number, function_name);
    pclose (output);
#else
    /* make C compiler happy */
    (void) number;
    (void) address;
    (void) symbol;
#endif
}

/*
 * weechat_backtrace: display backtrace (called when a SIGSEGV is received)
 */

void
weechat_backtrace ()
{
#ifdef HAVE_BACKTRACE
    void *trace[BACKTRACE_MAX];
    int trace_size, i;
    char **symbols;
#endif

    weechat_backtrace_printf ("======= WeeChat backtrace =======\n");
    weechat_backtrace_printf ("(written by %s, compiled on %s %s)\n",
                              PACKAGE_STRING, __DATE__, __TIME__);

#ifdef HAVE_BACKTRACE
    trace_size = backtrace (trace, BACKTRACE_MAX);
    symbols = backtrace_symbols (trace, trace_size);
    
    for (i = 0; i < trace_size; i++)
    {
        weechat_backtrace_addr2line (i + 1, trace[i], symbols[i]);
    }
#else
    weechat_backtrace_printf ("  No backtrace info (no debug info available "
                              "or no backtrace possible on your system).\n");
#endif
    
    weechat_backtrace_printf ("======= End of  backtrace =======\n");
}
