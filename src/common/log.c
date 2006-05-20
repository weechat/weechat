/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* log.c: WeeChat log file */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/file.h>

#include "weechat.h"
#include "log.h"


FILE *weechat_log_file = NULL; /* WeeChat log file (~/.weechat/weechat.log) */


/*
 * weechat_log_init: initialize log file
 */

void
weechat_log_init ()
{
    int filename_length;
    char *filename;
    
    filename_length = strlen (weechat_home) + 64;
    filename =
        (char *) malloc (filename_length * sizeof (char));
    snprintf (filename, filename_length, "%s/%s", weechat_home, WEECHAT_LOG_NAME);
    
    weechat_log_file = fopen (filename, "wt");
    if (!weechat_log_file
        || (flock (fileno (weechat_log_file), LOCK_EX | LOCK_NB) != 0))
    {
        fprintf (stderr,
                 _("%s unable to create/append to log file (%s/%s)\n"
                   "If another WeeChat process is using this file, try to run WeeChat\n"
                   "with another home using \"--dir\" command line option.\n"),
                 WEECHAT_ERROR, weechat_home, WEECHAT_LOG_NAME);
        exit (1);
    }
    free (filename);
}

/*
 * weechat_log_printf: write a message in WeeChat log (<weechat_home>/weechat.log)
 */

void
weechat_log_printf (char *message, ...)
{
    static char buffer[4096];
    char *ptr_buffer;
    va_list argptr;
    static time_t seconds;
    struct tm *date_tmp;
    
    if (!weechat_log_file)
        return;
    
    va_start (argptr, message);
    vsnprintf (buffer, sizeof (buffer) - 1, message, argptr);
    va_end (argptr);
    
    /* keep only valid chars */
    ptr_buffer = buffer;
    while (ptr_buffer[0])
    {
        if ((ptr_buffer[0] != '\n')
            && (ptr_buffer[0] != '\r')
            && ((unsigned char)(ptr_buffer[0]) < 32))
            ptr_buffer[0] = '.';
        ptr_buffer++;
    }
    
    seconds = time (NULL);
    date_tmp = localtime (&seconds);
    if (date_tmp)
        fprintf (weechat_log_file, "[%04d-%02d-%02d %02d:%02d:%02d] %s",
                 date_tmp->tm_year + 1900, date_tmp->tm_mon + 1, date_tmp->tm_mday,
                 date_tmp->tm_hour, date_tmp->tm_min, date_tmp->tm_sec,
                 buffer);
    else
        fprintf (weechat_log_file, "%s", buffer);
    fflush (weechat_log_file);
}

/*
 * weechat_log_close: close log file
 */

void
weechat_log_close ()
{
    if (weechat_log_file)
    {
        flock (fileno (weechat_log_file), LOCK_UN);
        fclose (weechat_log_file);
    }
}

/*
 * weechat_log_crash_rename: rename log file when crashing
 */

void
weechat_log_crash_rename ()
{
    char *oldname, *newname;

    oldname = (char *) malloc (strlen (weechat_home) + 64);
    newname = (char *) malloc (strlen (weechat_home) + 64);
    if (oldname && newname)
    {
    }
}
