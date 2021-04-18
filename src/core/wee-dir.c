/*
 * wee-dir.c - directory/file functions
 *
 * Copyright (C) 2003-2021 Sébastien Helleu <flashcode@flashtux.org>
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

/* for P_tmpdir in stdio.h */
#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

/* for nftw() */
#define _XOPEN_SOURCE 700

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <ftw.h>

#include "weechat.h"
#include "wee-config.h"
#include "wee-string.h"


/*
 * Returns the path to a temporary directory, the first valid directory in
 * this list:
 *   - content of environment variable "TMPDIR"
 *   - P_tmpdir (from stdio.h)
 *   - content of environment variable "HOME" (user home directory)
 *   - "." (current directory)
 */

char *
dir_get_temp_dir()
{
    char *tmpdir;
    struct stat buf;
    int rc;

    /* get directory from $TMPDIR */
    tmpdir = getenv ("TMPDIR");
    if (tmpdir && tmpdir[0])
    {
        rc = stat (tmpdir, &buf);
        if ((rc == 0) && S_ISDIR(buf.st_mode))
            return strdup (tmpdir);
    }

    /* get directory from P_tmpdir */
    rc = stat (P_tmpdir, &buf);
    if ((rc == 0) && S_ISDIR(buf.st_mode))
        return strdup (P_tmpdir);

    /* get directory from $HOME */
    tmpdir = getenv ("HOME");
    if (tmpdir && tmpdir[0])
    {
        rc = stat (tmpdir, &buf);
        if ((rc == 0) && S_ISDIR(buf.st_mode))
            return strdup (tmpdir);
    }

    /* fallback on current directory */
    return strdup (".");
}

/*
 * Creates a directory in WeeChat home.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dir_mkdir_home (const char *directory, int mode)
{
    char *dir_name;
    int dir_length;

    if (!directory)
        return 0;

    /* build directory, adding WeeChat home */
    dir_length = strlen (weechat_home) + strlen (directory) + 2;
    dir_name = malloc (dir_length);
    if (!dir_name)
        return 0;

    snprintf (dir_name, dir_length, "%s/%s", weechat_home, directory);

    if (mkdir (dir_name, mode) < 0)
    {
        if (errno != EEXIST)
        {
            free (dir_name);
            return 0;
        }
    }

    free (dir_name);
    return 1;
}

/*
 * Creates a directory.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dir_mkdir (const char *directory, int mode)
{
    if (!directory)
        return 0;

    if (mkdir (directory, mode) < 0)
    {
        if (errno != EEXIST)
            return 0;
    }

    return 1;
}

/*
 * Creates a directory and makes parent directories as needed.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dir_mkdir_parents (const char *directory, int mode)
{
    char *string, *ptr_string, *pos_sep;
    struct stat buf;
    int rc;

    if (!directory)
        return 0;

    string = strdup (directory);
    if (!string)
        return 0;

    ptr_string = string;
    while (ptr_string[0] == DIR_SEPARATOR_CHAR)
    {
        ptr_string++;
    }

    while (ptr_string && ptr_string[0])
    {
        pos_sep = strchr (ptr_string, DIR_SEPARATOR_CHAR);
        if (pos_sep)
            pos_sep[0] = '\0';

        rc = stat (string, &buf);
        if ((rc < 0) || !S_ISDIR(buf.st_mode))
        {
            /* try to create directory */
            if (!dir_mkdir (string, mode))
            {
                free (string);
                return 0;
            }
        }

        if (pos_sep)
        {
            pos_sep[0] = DIR_SEPARATOR_CHAR;
            ptr_string = pos_sep + 1;
        }
        else
            ptr_string = NULL;
    }

    free (string);

    return 1;
}

/*
 * Unlinks a file or directory; callback called by function dir_rmtree().
 *
 * Returns the return code of remove():
 *   0: OK
 *   -1: error
 */

int
dir_unlink_cb (const char *fpath, const struct stat *sb, int typeflag,
               struct FTW *ftwbuf)
{
    /* make C compiler happy */
    (void) sb;
    (void) typeflag;
    (void) ftwbuf;

    return remove (fpath);
}

/*
 * Removes a directory and all files inside recursively.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dir_rmtree (const char *directory)
{
    int rc;

    if (!directory)
        return 0;

    rc = nftw (directory, dir_unlink_cb, 64, FTW_DEPTH | FTW_PHYS);

    return (rc == 0) ? 1 : 0;
}

/*
 * Finds files in a directory and executes a function on each file.
 */

void
dir_exec_on_files (const char *directory, int recurse_subdirs,
                    int hidden_files,
                    void (*callback)(void *data, const char *filename),
                    void *callback_data)
{
    char complete_filename[PATH_MAX];
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;

    if (!directory || !callback)
        return;

    dir = opendir (directory);
    if (dir)
    {
        while ((entry = readdir (dir)))
        {
            if (hidden_files || (entry->d_name[0] != '.'))
            {
                snprintf (complete_filename, sizeof (complete_filename),
                          "%s/%s", directory, entry->d_name);
                lstat (complete_filename, &statbuf);
                if (S_ISDIR(statbuf.st_mode))
                {
                    if (recurse_subdirs
                        && (strcmp (entry->d_name, ".") != 0)
                        && (strcmp (entry->d_name, "..") != 0))
                    {
                        dir_exec_on_files (complete_filename, 1, hidden_files,
                                           callback, callback_data);
                    }
                }
                else
                {
                    (*callback) (callback_data, complete_filename);
                }
            }
        }
        closedir (dir);
    }
}

/*
 * Searches for the full name of a WeeChat library with name and extension
 * (searches first in WeeChat user's dir, then WeeChat global lib directory).
 *
 * Returns name of library found, NULL if not found.
 *
 * Note: result must be freed after use (if not NULL).
 */

char *
dir_search_full_lib_name_ext (const char *filename, const char *extension,
                              const char *plugins_dir)
{
    char *name_with_ext, *final_name, *extra_libdir;
    int length;
    struct stat st;

    length = strlen (filename) + strlen (extension) + 1;
    name_with_ext = malloc (length);
    if (!name_with_ext)
        return NULL;
    snprintf (name_with_ext, length,
              "%s%s",
              filename,
              (strchr (filename, '.')) ? "" : extension);

    /* try libdir from environment variable WEECHAT_EXTRA_LIBDIR */
    extra_libdir = getenv (WEECHAT_EXTRA_LIBDIR);
    if (extra_libdir && extra_libdir[0])
    {
        length = strlen (extra_libdir) + strlen (name_with_ext) +
            strlen (plugins_dir) + 16;
        final_name = malloc (length);
        if (!final_name)
        {
            free (name_with_ext);
            return NULL;
        }
        snprintf (final_name, length,
                  "%s%s%s%s%s",
                  extra_libdir,
                  DIR_SEPARATOR,
                  plugins_dir,
                  DIR_SEPARATOR,
                  name_with_ext);
        if ((stat (final_name, &st) == 0) && (st.st_size > 0))
        {
            free (name_with_ext);
            return final_name;
        }
        free (final_name);
    }

    /* try WeeChat user's dir */
    length = strlen (weechat_home) + strlen (name_with_ext) +
        strlen (plugins_dir) + 16;
    final_name = malloc (length);
    if (!final_name)
    {
        free (name_with_ext);
        return NULL;
    }
    snprintf (final_name, length,
              "%s%s%s%s%s",
              weechat_home,
              DIR_SEPARATOR,
              plugins_dir,
              DIR_SEPARATOR,
              name_with_ext);
    if ((stat (final_name, &st) == 0) && (st.st_size > 0))
    {
        free (name_with_ext);
        return final_name;
    }
    free (final_name);

    /* try WeeChat global lib dir */
    length = strlen (WEECHAT_LIBDIR) + strlen (name_with_ext) +
        strlen (plugins_dir) + 16;
    final_name = malloc (length);
    if (!final_name)
    {
        free (name_with_ext);
        return NULL;
    }
    snprintf (final_name, length,
              "%s%s%s%s%s",
              WEECHAT_LIBDIR,
              DIR_SEPARATOR,
              plugins_dir,
              DIR_SEPARATOR,
              name_with_ext);
    if ((stat (final_name, &st) == 0) && (st.st_size > 0))
    {
        free (name_with_ext);
        return final_name;
    }
    free (final_name);

    free (name_with_ext);

    return NULL;
}

/*
 * Searches for the full name of a WeeChat library with name.
 *
 * All extensions listed in option "weechat.plugin.extension" are tested.
 *
 * Note: result must be freed after use (if not NULL).
 */

char *
dir_search_full_lib_name (const char *filename, const char *plugins_dir)
{
    char *filename2, *full_name;
    int i;

    /* expand home in filename */
    filename2 = string_expand_home (filename);
    if (!filename2)
        return NULL;

    /* if full path, return it */
    if (strchr (filename2, '/') || strchr (filename2, '\\'))
        return filename2;

    if (config_plugin_extensions)
    {
        for (i = 0; i < config_num_plugin_extensions; i++)
        {
            full_name = dir_search_full_lib_name_ext (
                filename2,
                config_plugin_extensions[i],
                plugins_dir);
            if (full_name)
            {
                free (filename2);
                return full_name;
            }
        }
    }
    else
    {
        full_name = dir_search_full_lib_name_ext (filename2, "", plugins_dir);
        if (full_name)
        {
            free (filename2);
            return full_name;
        }
    }

    free (filename2);

    return strdup (filename);
}

/*
 * Reads content of a file.
 *
 * Returns an allocated buffer with the content of file, NULL if error.
 *
 * Note: result must be freed after use.
 */

char *
dir_file_get_content (const char *filename)
{
    char *buffer, *buffer2;
    FILE *f;
    size_t count, fp;

    if (!filename)
        return NULL;

    buffer = NULL;
    fp = 0;

    f = fopen (filename, "r");
    if (!f)
        goto error;

    while (!feof (f))
    {
        if (fp > SIZE_MAX - (1024 * sizeof (char)))
            goto error;
        buffer2 = (char *) realloc (buffer, (fp + (1024 * sizeof (char))));
        if (!buffer2)
            goto error;
        buffer = buffer2;
        count = fread (&buffer[fp], sizeof (char), 1024, f);
        if (count <= 0)
            goto error;
        fp += count;
    }
    if (fp > SIZE_MAX - sizeof (char))
        goto error;
    buffer2 = (char *) realloc (buffer, fp + sizeof (char));
    if (!buffer2)
        goto error;
    buffer = buffer2;
    buffer[fp] = '\0';
    fclose (f);

    return buffer;

error:
    if (buffer)
	free (buffer);
    if (f)
	fclose (f);
    return NULL;
}

/*
 * Expands and assigns given path to "weechat_home".
 */

void
dir_set_home_path (char *home_path)
{
    char *ptr_home;
    int dir_length;

    if (home_path[0] == '~')
    {
        /* replace leading '~' by $HOME */
        ptr_home = getenv ("HOME");
        if (!ptr_home)
        {
            string_fprintf (stderr, _("Error: unable to get HOME directory\n"));
            weechat_shutdown (EXIT_FAILURE, 0);
            /* make C static analyzer happy (never executed) */
            return;
        }
        dir_length = strlen (ptr_home) + strlen (home_path + 1) + 1;
        weechat_home = malloc (dir_length);
        if (weechat_home)
        {
            snprintf (weechat_home, dir_length,
                      "%s%s", ptr_home, home_path + 1);
        }
    }
    else
    {
        weechat_home = strdup (home_path);
    }

    if (!weechat_home)
    {
        string_fprintf (stderr,
                        _("Error: not enough memory for home directory\n"));
        weechat_shutdown (EXIT_FAILURE, 0);
        /* make C static analyzer happy (never executed) */
        return;
    }
}

/*
 * Creates WeeChat home directory (by default ~/.weechat).
 *
 * Any error in this function is fatal: WeeChat can not run without a home
 * directory.
 */

void
dir_create_home_dir ()
{
    char *temp_dir, *temp_home_template, *ptr_weechat_home;
    char *config_weechat_home;
    int length, add_separator;
    struct stat statinfo;

    /* temporary WeeChat home */
    if (weechat_home_temp)
    {
        temp_dir = dir_get_temp_dir ();
        if (!temp_dir || !temp_dir[0])
        {
            string_fprintf (stderr,
                            _("Error: not enough memory for home "
                              "directory\n"));
            weechat_shutdown (EXIT_FAILURE, 0);
            /* make C static analyzer happy (never executed) */
            return;
        }
        length = strlen (temp_dir) + 32 + 1;
        temp_home_template = malloc (length);
        if (!temp_home_template)
        {
            free (temp_dir);
            string_fprintf (stderr,
                            _("Error: not enough memory for home "
                              "directory\n"));
            weechat_shutdown (EXIT_FAILURE, 0);
            /* make C static analyzer happy (never executed) */
            return;
        }
        add_separator = (temp_dir[strlen (temp_dir) - 1] != DIR_SEPARATOR_CHAR);
        snprintf (temp_home_template, length,
                  "%s%sweechat_temp_XXXXXX",
                  temp_dir,
                  add_separator ? DIR_SEPARATOR : "");
        free (temp_dir);
        ptr_weechat_home = mkdtemp (temp_home_template);
        if (!ptr_weechat_home)
        {
            string_fprintf (stderr,
                            _("Error: unable to create a temporary "
                              "home directory (using template: \"%s\")\n"),
                            temp_home_template);
            free (temp_home_template);
            weechat_shutdown (EXIT_FAILURE, 0);
            /* make C static analyzer happy (never executed) */
            return;
        }
        weechat_home = strdup (ptr_weechat_home);
        free (temp_home_template);
        weechat_home_delete_on_exit = 1;
        return;
    }

    /*
     * weechat_home is not set yet: look for environment variable
     * "WEECHAT_HOME"
     */
    if (!weechat_home)
    {
        ptr_weechat_home = getenv ("WEECHAT_HOME");
        if (ptr_weechat_home && ptr_weechat_home[0])
            dir_set_home_path (ptr_weechat_home);
    }

    /* weechat_home is still not set: try to use compile time default */
    if (!weechat_home)
    {
        config_weechat_home = WEECHAT_HOME;
        dir_set_home_path (
            (config_weechat_home[0] ? config_weechat_home : "~/.weechat"));
    }

    /* if home already exists, it has to be a directory */
    if (stat (weechat_home, &statinfo) == 0)
    {
        if (!S_ISDIR (statinfo.st_mode))
        {
            string_fprintf (stderr,
                            _("Error: home (%s) is not a directory\n"),
                            weechat_home);
            weechat_shutdown (EXIT_FAILURE, 0);
            /* make C static analyzer happy (never executed) */
            return;
        }
    }

    /* create home directory; error is fatal */
    if (!dir_mkdir (weechat_home, 0755))
    {
        string_fprintf (stderr,
                        _("Error: cannot create directory \"%s\"\n"),
                        weechat_home);
        weechat_shutdown (EXIT_FAILURE, 0);
        /* make C static analyzer happy (never executed) */
        return;
    }
}
