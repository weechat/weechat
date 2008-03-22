/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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

/* wee-util.c: some useful functions for WeeChat */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>

#include "weechat.h"
#include "wee-util.h"
#include "wee-config.h"
#include "wee-string.h"


/*
 * util_timeval_cmp: compare two timeval structures
 *                   return: -1 if tv1 < tv2
 *                            0 if tv1 == tv2
 *                            1 if tv1 > tv2
 */

int
util_timeval_cmp (struct timeval *tv1, struct timeval *tv2)
{
    if (tv1->tv_sec < tv2->tv_sec)
        return -1;
    if (tv1->tv_sec > tv2->tv_sec)
        return 1;
    if (tv1->tv_usec < tv2->tv_usec)
        return -1;
    if (tv1->tv_usec > tv2->tv_usec)
        return 1;
    return 0;
}

/*
 * util_timeval_diff: calculates difference between two times (return in
 *                    milliseconds)
 */

long
util_timeval_diff (struct timeval *tv1, struct timeval *tv2)
{
    long diff_sec, diff_usec;
    
    diff_sec = tv2->tv_sec - tv1->tv_sec;
    diff_usec = tv2->tv_usec - tv1->tv_usec;
    
    if (diff_usec < 0)
    {
        diff_usec += 1000000;
        diff_sec--;
    }
    return ((diff_usec / 1000) + (diff_sec * 1000));
}

/*
 * util_timeval_add: add interval (in milliseconds) to a timeval struct 
 */

void
util_timeval_add (struct timeval *tv, long interval)
{
    long usec;
    
    tv->tv_sec += (interval / 1000);
    usec = tv->tv_usec + ((interval % 1000) * 1000);
    if (usec > 1000000)
    {
        tv->tv_usec = usec % 1000000;
        tv->tv_sec++;
    }
    else
        tv->tv_usec = usec;
}

/*
 * util_get_time_length: calculates time length with a time format
 */

int
util_get_time_length (char *time_format)
{
    time_t date;
    struct tm *local_time;
    char text_time[1024];
    
    if (!time_format || !time_format[0])
        return 0;
    
    date = time (NULL);
    local_time = localtime (&date);
    strftime (text_time, sizeof (text_time),
              time_format, local_time);
    return strlen (text_time);
}

/*
 * util_catch_signal: catch a signal
 */

void
util_catch_signal (int signum, void (*handler)(int))
{
    struct sigaction act;
    
    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = handler;
    sigaction(signum, &act, NULL);
}

/*
 * util_create_dir: create a directory
 *                  return: 1 if ok (or directory already exists)
 *                          0 if error
 */

int
util_create_dir (char *directory, int permissions)
{
    if (mkdir (directory, 0755) < 0)
    {
        /* exit if error (except if directory already exists) */
        if (errno != EEXIST)
        {
            string_iconv_fprintf (stderr,
                                  _("Error: cannot create directory \"%s\"\n"),
                                  directory);
            return 0;
        }
        return 1;
    }
    if ((permissions != 0) && (strcmp (directory, getenv ("HOME")) != 0))
        chmod (directory, permissions);
    return 1;
}

/*
 * util_exec_on_files: find files in a directory and execute a
 *                     function on each file
 */

void
util_exec_on_files (char *directory, void *data,
                    int (*callback)(void *data, char *filename))
{
    char complete_filename[1024];
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    int rc;
    
    if (!directory || !callback)
        return;
    
    dir = opendir (directory);
    if (dir)
    {
        while ((entry = readdir (dir)))
        {
            snprintf (complete_filename, sizeof (complete_filename) - 1,
                      "%s/%s", directory, entry->d_name);
            lstat (complete_filename, &statbuf);
            if (!S_ISDIR(statbuf.st_mode))
            {
                rc = (*callback) (data, complete_filename);
            }
        }
        closedir (dir);
    }
}

/*
 * util_search_full_lib_name: search the full name of a WeeChat library
 *                            file with a part of name
 *                            - look in WeeChat user's dir, then WeeChat
 *                              global lib dir
 *                            - sys_directory is the system directory under
 *                              WeeChat lib prefix, for example "plugins"
 *                            - result has to be free() after use
 */

char *
util_search_full_lib_name (char *filename, char *sys_directory)
{
    char *name_with_ext, *final_name;
    int length;
    struct stat st;
    
    /* filename is already a full path */
    if (strchr (filename, '/') || strchr (filename, '\\'))
        return strdup (filename);
    
    length = strlen (filename) + 16;
    if (CONFIG_STRING(config_plugins_extension)
        && CONFIG_STRING(config_plugins_extension)[0])
        length += strlen (CONFIG_STRING(config_plugins_extension));
    name_with_ext = (char *)malloc (length * sizeof (char));
    if (!name_with_ext)
        return strdup (filename);
    strcpy (name_with_ext, filename);
    if (!strchr (filename, '.')
        && CONFIG_STRING(config_plugins_extension)
        && CONFIG_STRING(config_plugins_extension)[0])
        strcat (name_with_ext, CONFIG_STRING(config_plugins_extension));
    
    /* try WeeChat user's dir */
    length = strlen (weechat_home) + strlen (name_with_ext) +
        strlen (sys_directory) + 16;
    final_name = (char *)malloc (length * sizeof (char));
    if (!final_name)
    {
        free (name_with_ext);
        return strdup (filename);
    }
    snprintf (final_name, length,
              "%s/%s/%s", weechat_home, sys_directory, name_with_ext);
    if ((stat (final_name, &st) == 0) && (st.st_size > 0))
    {
        free (name_with_ext);
        return final_name;
    }
    free (final_name);
    
    /* try WeeChat global lib dir */
    length = strlen (WEECHAT_LIBDIR) + strlen (name_with_ext) +
        strlen (sys_directory) + 16;
    final_name = (char *)malloc (length * sizeof (char));
    if (!final_name)
    {
        free (name_with_ext);
        return strdup (filename);
    }
    snprintf (final_name, length,
              "%s/%s/%s", WEECHAT_LIBDIR, sys_directory, name_with_ext);
    if ((stat (final_name, &st) == 0) && (st.st_size > 0))
    {
        free (name_with_ext);
        return final_name;
    }
    free (final_name);

    return name_with_ext;
}
