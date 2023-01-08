/*
 * wee-dir.c - directory/file functions
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

/* for P_tmpdir in stdio.h */
#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

/* for nftw() */
#define _XOPEN_SOURCE 700
#if defined(__APPLE__)
#define _DARWIN_C_SOURCE
#endif

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
#include <zlib.h>
#include <zstd.h>

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
dir_get_temp_dir ()
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
    char *dir, *dir1, *dir2, *dir3, *dir4;
    int rc, dir_length;

    rc = 0;
    dir = NULL;
    dir1 = NULL;
    dir2 = NULL;
    dir3 = NULL;
    dir4 = NULL;

    if (!directory)
        goto end;

    if (strncmp (directory, "${", 2) == 0)
    {
        dir = strdup (directory);
    }
    else
    {
        /* build directory in data dir by default */
        dir_length = strlen (weechat_data_dir) + strlen (directory) + 2;
        dir = malloc (dir_length);
        if (!dir)
            goto end;
        snprintf (dir, dir_length, "%s/%s", weechat_data_dir, directory);
    }

    dir1 = string_replace (dir, "${weechat_config_dir}", weechat_config_dir);
    if (!dir1)
        goto end;

    dir2 = string_replace (dir1, "${weechat_data_dir}", weechat_data_dir);
    if (!dir2)
        goto end;

    dir3 = string_replace (dir2, "${weechat_cache_dir}", weechat_cache_dir);
    if (!dir3)
        goto end;

    dir4 = string_replace (dir3, "${weechat_runtime_dir}", weechat_runtime_dir);
    if (!dir4)
        goto end;

    /* build directory, adding WeeChat home */
    if (mkdir (dir4, mode) < 0)
    {
        if (errno != EEXIST)
            goto end;
    }

    rc = 1;

end:
    if (dir)
        free (dir);
    if (dir1)
        free (dir1);
    if (dir2)
        free (dir2);
    if (dir3)
        free (dir3);
    if (dir4)
        free (dir4);
    return rc;
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
 * Uses one or four different paths for WeeChat home directories.
 *
 * If 4 paths are given, they must be separated by colons and given in this
 * order: config, data, cache, runtime.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dir_set_home_path (char *path)
{
    char **paths;
    int rc, num_paths;

    rc = 0;
    paths = NULL;

    if (!path)
        goto end;

    paths = string_split (path, ":", NULL, 0, 0, &num_paths);
    if (!paths)
    {
        string_fprintf (stderr, _("Error: not enough memory\n"));
        goto end;
    }

    if (num_paths == 1)
    {
        weechat_config_dir = string_expand_home (paths[0]);
        weechat_data_dir = string_expand_home (paths[0]);
        weechat_cache_dir = string_expand_home (paths[0]);
        weechat_runtime_dir = string_expand_home (paths[0]);
    }
    else if (num_paths == 4)
    {
        weechat_config_dir = string_expand_home (paths[0]);
        weechat_data_dir = string_expand_home (paths[1]);
        weechat_cache_dir = string_expand_home (paths[2]);
        weechat_runtime_dir = string_expand_home (paths[3]);
    }
    else
    {
        string_fprintf (stderr,
                        _("Error: wrong number of paths for home directories "
                          "(expected: 1 or 4, received: %d)\n"),
                        num_paths);
        goto end;
    }

    rc = 1;

end:
    if (paths)
        string_free_split (paths);
    return rc;
}

/*
 * Creates WeeChat temporary home directory (deleted on exit).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dir_create_home_temp_dir ()
{
    char *temp_dir, *temp_home_template, *ptr_weechat_home;
    int rc, length, add_separator;

    rc = 0;
    temp_dir = NULL;
    temp_home_template = NULL;

    temp_dir = dir_get_temp_dir ();
    if (!temp_dir || !temp_dir[0])
        goto memory_error;

    length = strlen (temp_dir) + 32 + 1;
    temp_home_template = malloc (length);
    if (!temp_home_template)
        goto memory_error;

    add_separator = (temp_dir[strlen (temp_dir) - 1] != DIR_SEPARATOR_CHAR);
    snprintf (temp_home_template, length,
              "%s%sweechat_temp_XXXXXX",
              temp_dir,
              add_separator ? DIR_SEPARATOR : "");
    ptr_weechat_home = mkdtemp (temp_home_template);
    if (!ptr_weechat_home)
    {
        string_fprintf (stderr,
                        _("Error: unable to create a temporary "
                          "home directory (using template: \"%s\")\n"),
                        temp_home_template);
        goto end;
    }

    weechat_config_dir = strdup (ptr_weechat_home);
    weechat_data_dir = strdup (ptr_weechat_home);
    weechat_cache_dir = strdup (ptr_weechat_home);
    weechat_runtime_dir = strdup (ptr_weechat_home);

    weechat_home_delete_on_exit = 1;

    rc = 1;
    goto end;

memory_error:
    string_fprintf (stderr, _("Error: not enough memory\n"));

end:
    if (temp_dir)
        free (temp_dir);
    if (temp_home_template)
        free (temp_home_template);
    return rc;
}

/*
 * Finds XDG directories.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dir_find_xdg_dirs (char **config_dir, char **data_dir, char **cache_dir,
                   char **runtime_dir)
{
    char *ptr_home, path[PATH_MAX];
    char *xdg_config_home, *xdg_data_home, *xdg_cache_home, *xdg_runtime_dir;

    *config_dir = NULL;
    *data_dir = NULL;
    *cache_dir = NULL;
    *runtime_dir = NULL;

    ptr_home = getenv ("HOME");
    xdg_config_home = getenv ("XDG_CONFIG_HOME");
    xdg_data_home = getenv ("XDG_DATA_HOME");
    xdg_cache_home = getenv ("XDG_CACHE_HOME");
    xdg_runtime_dir = getenv ("XDG_RUNTIME_DIR");

    /* set config dir: $XDG_CONFIG_HOME/weechat or $HOME/.config/weechat */
    if (xdg_config_home && xdg_config_home[0])
    {
        snprintf (path, sizeof (path),
                  "%s%s%s",
                  xdg_config_home, DIR_SEPARATOR, "weechat");
    }
    else
    {
        snprintf (path, sizeof (path),
                  "%s%s%s%s%s",
                  ptr_home, DIR_SEPARATOR, ".config", DIR_SEPARATOR, "weechat");
    }
    *config_dir = strdup (path);
    if (!*config_dir)
        goto error;

    /* set data dir: $XDG_DATA_HOME/weechat or $HOME/.local/share/weechat */
    if (xdg_data_home && xdg_data_home[0])
    {
        snprintf (path, sizeof (path),
                  "%s%s%s",
                  xdg_data_home, DIR_SEPARATOR, "weechat");
    }
    else
    {
        snprintf (path, sizeof (path),
                  "%s%s%s%s%s%s%s",
                  ptr_home, DIR_SEPARATOR, ".local", DIR_SEPARATOR, "share",
                  DIR_SEPARATOR, "weechat");
    }
    *data_dir = strdup (path);
    if (!*data_dir)
        goto error;

    /* set cache dir: $XDG_CACHE_HOME/weechat or $HOME/.cache/weechat */
    if (xdg_cache_home && xdg_cache_home[0])
    {
        snprintf (path, sizeof (path),
                  "%s%s%s",
                  xdg_cache_home, DIR_SEPARATOR, "weechat");
    }
    else
    {
        snprintf (path, sizeof (path),
                  "%s%s%s%s%s",
                  ptr_home, DIR_SEPARATOR, ".cache", DIR_SEPARATOR, "weechat");
    }
    *cache_dir = strdup (path);
    if (!*cache_dir)
        goto error;

    /* set runtime dir: $XDG_RUNTIME_DIR/weechat or same as cache dir */
    if (xdg_runtime_dir && xdg_runtime_dir[0])
    {
        snprintf (path, sizeof (path),
                  "%s%s%s",
                  xdg_runtime_dir, DIR_SEPARATOR, "weechat");
        *runtime_dir = strdup (path);
    }
    else
    {
        *runtime_dir = strdup (*cache_dir);
    }
    if (!*runtime_dir)
        goto error;

    return 1;

error:
    if (*config_dir)
    {
        free (*config_dir);
        *config_dir = NULL;
    }
    if (*data_dir)
    {
        free (*data_dir);
        *data_dir = NULL;
    }
    if (*cache_dir)
    {
        free (*cache_dir);
        *cache_dir = NULL;
    }
    if (*runtime_dir)
    {
        free (*runtime_dir);
        *runtime_dir = NULL;
    }
    string_fprintf (stderr, _("Error: not enough memory\n"));
    return 0;
}

/*
 * Finds WeeChat home directories: it can be either XDG directories or the
 * same directory for all files (like the legacy directory ~/.weechat).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dir_find_home_dirs ()
{
    char *ptr_home, *ptr_weechat_home, *config_weechat_home;
    char *config_dir, *data_dir, *cache_dir, *runtime_dir;
    char path[PATH_MAX];

    /* temporary WeeChat home */
    if (weechat_home_temp)
        return dir_create_home_temp_dir ();

    /* use a forced home with -d/--dir */
    if (weechat_home_force)
        return dir_set_home_path (weechat_home_force);

    /* use environment variable "WEECHAT_HOME" (if set) */
    ptr_weechat_home = getenv ("WEECHAT_HOME");
    if (ptr_weechat_home && ptr_weechat_home[0])
        return dir_set_home_path (ptr_weechat_home);

    /* use the home forced at compilation time (if set) */
    config_weechat_home = WEECHAT_HOME;
    if (config_weechat_home[0])
        return dir_set_home_path (config_weechat_home);

    if (!dir_find_xdg_dirs (&config_dir, &data_dir, &cache_dir, &runtime_dir))
        return 0;

    /* check if {weechat_config_dir}/weechat.conf exists */
    snprintf (path, sizeof (path),
              "%s%s%s",
              config_dir, DIR_SEPARATOR, "weechat.conf");
    if (access (path, F_OK) == 0)
        goto use_xdg;

    /*
     * check if $HOME/.weechat/weechat.conf exists
     * (compatibility with old releases not supporting XDG directories)
     */
    ptr_home = getenv ("HOME");
    snprintf (path, sizeof (path),
              "%s%s%s%s%s",
              ptr_home, DIR_SEPARATOR, ".weechat", DIR_SEPARATOR, "weechat.conf");
    if (access (path, F_OK) == 0)
    {
        snprintf (path, sizeof (path),
                  "%s%s%s",
                  ptr_home, DIR_SEPARATOR, ".weechat");
        weechat_config_dir = strdup (path);
        weechat_data_dir = strdup (path);
        weechat_cache_dir = strdup (path);
        weechat_runtime_dir = strdup (path);
        free (config_dir);
        free (data_dir);
        free (cache_dir);
        free (runtime_dir);
        return 1;
    }

    /* use XDG directories */
use_xdg:
    weechat_config_dir = config_dir;
    weechat_data_dir = data_dir;
    weechat_cache_dir = cache_dir;
    weechat_runtime_dir = runtime_dir;
    return 1;
}

/*
 * Creates a home directory.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dir_create_home_dir (char *path)
{
    struct stat statinfo;

    /* if home already exists, it has to be a directory */
    if (stat (path, &statinfo) == 0)
    {
        if (!S_ISDIR (statinfo.st_mode))
        {
            string_fprintf (stderr,
                            _("Error: \"%s\" is not a directory\n"),
                            path);
            return 0;
        }
    }

    /* create home directory; error is fatal */
    if (!dir_mkdir_parents (path, 0700))
    {
        string_fprintf (stderr,
                        _("Error: cannot create directory \"%s\"\n"),
                        path);
        return 0;
    }

    return 1;
}

/*
 * Creates WeeChat home directories.
 *
 * Any error in this function (or a sub function called) is fatal: WeeChat
 * can not run at all without the home directories.
 */

void
dir_create_home_dirs ()
{
    int rc;

    if (!dir_find_home_dirs ())
        goto error;

    rc = dir_create_home_dir (weechat_config_dir);
    if (rc && (strcmp (weechat_config_dir, weechat_data_dir) != 0))
        rc = dir_create_home_dir (weechat_data_dir);
    if (rc && (strcmp (weechat_config_dir, weechat_cache_dir) != 0))
        rc = dir_create_home_dir (weechat_cache_dir);
    if (rc && (strcmp (weechat_config_dir, weechat_runtime_dir) != 0))
        rc = dir_create_home_dir (weechat_runtime_dir);
    if (rc)
        return;

error:
    weechat_shutdown (EXIT_FAILURE, 0);
}

/*
 * Removes WeeChat home directories (called when -t / --temp-dir is given).
 */

void
dir_remove_home_dirs ()
{
    dir_rmtree (weechat_config_dir);
    if (strcmp (weechat_config_dir, weechat_data_dir) != 0)
        dir_rmtree (weechat_data_dir);
    if (strcmp (weechat_config_dir, weechat_cache_dir) != 0)
        dir_rmtree (weechat_cache_dir);
    if (strcmp (weechat_config_dir, weechat_runtime_dir) != 0)
        dir_rmtree (weechat_runtime_dir);
}

/*
 * Returns a string with home directories separated by colons, in this order:
 * config_dir, data_dir, cache_dir, runtime_dir.
 *
 * Example of value returned:
 *   /home/user/.config/weechat:/home/user/.local/share/weechat:
 *     /home/user/.cache/weechat:/run/user/1000/weechat
 *
 * Note: result must be freed after use.
 */

char *
dir_get_string_home_dirs ()
{
    char *dirs[5];

    dirs[0] = weechat_config_dir;
    dirs[1] = weechat_data_dir;
    dirs[2] = weechat_cache_dir;
    dirs[3] = weechat_runtime_dir;
    dirs[4] = NULL;

    return string_rebuild_split_string ((const char **)dirs, ":", 0, -1);
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
    char name_with_ext[256], final_name[PATH_MAX], *extra_libdir;
    struct stat st;
    int rc;

    rc = snprintf (name_with_ext, sizeof (name_with_ext),
                   "%s%s",
                   filename,
                   (strchr (filename, '.')) ? "" : extension);
    if ((rc < 0) || (rc >= (int)sizeof (name_with_ext)))
        return NULL;

    /* try libdir from environment variable WEECHAT_EXTRA_LIBDIR */
    extra_libdir = getenv (WEECHAT_EXTRA_LIBDIR);
    if (extra_libdir && extra_libdir[0])
    {
        rc = snprintf (final_name, sizeof (final_name),
                       "%s%s%s%s%s",
                       extra_libdir,
                       DIR_SEPARATOR,
                       plugins_dir,
                       DIR_SEPARATOR,
                       name_with_ext);
        if ((rc < 0) || (rc >= (int)sizeof (final_name)))
            return NULL;
        if ((stat (final_name, &st) == 0) && (st.st_size > 0))
            return strdup (final_name);
    }

    /* try WeeChat user's dir */
    rc = snprintf (final_name, sizeof (final_name),
                   "%s%s%s%s%s",
                   weechat_data_dir,
                   DIR_SEPARATOR,
                   plugins_dir,
                   DIR_SEPARATOR,
                   name_with_ext);
    if ((rc < 0) || (rc >= (int)sizeof (final_name)))
        return NULL;
    if ((stat (final_name, &st) == 0) && (st.st_size > 0))
        return strdup (final_name);

    /* try WeeChat global lib dir */
    rc = snprintf (final_name, sizeof (final_name),
                   "%s%s%s%s%s",
                   WEECHAT_LIBDIR,
                   DIR_SEPARATOR,
                   plugins_dir,
                   DIR_SEPARATOR,
                   name_with_ext);
    if ((rc < 0) || (rc >= (int)sizeof (final_name)))
        return NULL;
    if ((stat (final_name, &st) == 0) && (st.st_size > 0))
        return strdup (final_name);

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
 * Copies a file to another location.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dir_file_copy (const char *from, const char *to)
{
    FILE *src, *dst;
    char *buffer;
    int rc;
    size_t count;

    rc = 0;
    buffer = NULL;
    src = NULL;
    dst = NULL;

    if (!from || !from[0] || !to || !to[0])
        goto end;

    buffer = malloc (65536);
    if (!buffer)
        goto end;

    src = fopen (from, "rb");
    if (!src)
        goto end;
    dst = fopen (to, "wb");
    if (!dst)
        goto end;

    while (!feof (src))
    {
        count = fread (buffer, 1, 65536, src);
        if (count <= 0)
            goto end;
        if (fwrite (buffer, 1, count, dst) <= 0)
            goto end;
    }

    rc = 1;

end:
    if (buffer)
	free (buffer);
    if (src)
        fclose (src);
    if (dst)
        fclose (dst);
    return rc;
}

/*
 * Compresses a file with gzip.
 *
 * Notes:
 *   - the output file must not exist
 *   - compression_level is an integer between 1 and 9
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dir_file_compress_gzip (const char *from, const char *to,
                        int compression_level)
{
    FILE *source, *dest;
    z_stream strm;
    unsigned char *buffer_in, *buffer_out;
    int rc, ret, flush;
    size_t buffer_size, have;

    source = NULL;
    dest = NULL;
    buffer_size = 256 * 1024;
    buffer_in = NULL;
    buffer_out = NULL;
    rc = 0;

    if (!from || !to || (compression_level < 1) || (compression_level > 9))
        goto end;

    if (access (to, F_OK) == 0)
        goto end;

    buffer_in = malloc (buffer_size);
    if (!buffer_in)
        goto end;
    buffer_out = malloc (buffer_size);
    if (!buffer_out)
        goto end;

    source = fopen (from, "rb");
    if (!source)
        goto end;
    dest = fopen (to, "wb");
    if (!dest)
        goto end;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    ret = deflateInit2 (
        &strm,
        compression_level,
        Z_DEFLATED,           /* method */
        15 + 16,              /* + 16 = gzip instead of zlib */
        8,                    /* memLevel */
        Z_DEFAULT_STRATEGY);  /* strategy */
    if (ret != Z_OK)
        goto end;

    do
    {
        strm.avail_in = fread (buffer_in, 1, buffer_size, source);
        if (ferror (source))
            goto error;

        flush = feof (source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = buffer_in;

        do
        {
            strm.avail_out = buffer_size;
            strm.next_out = buffer_out;
            ret = deflate (&strm, flush);
            if (ret == Z_STREAM_ERROR)
                goto error;
            have = buffer_size - strm.avail_out;
            if ((fwrite (buffer_out, 1, have, dest) != have) || ferror (dest))
                goto error;
        } while (strm.avail_out == 0);
        if (strm.avail_in != 0)
            goto error;
    } while (flush != Z_FINISH);

    if (ret != Z_STREAM_END)
        goto error;

    (void) deflateEnd (&strm);

    rc = 1;
    goto end;

error:
    (void) deflateEnd (&strm);
    unlink (to);

end:
    if (buffer_in)
        free (buffer_in);
    if (buffer_out)
        free (buffer_out);
    if (source)
        fclose (source);
    if (dest)
        fclose (dest);

    return rc;
}

/*
 * Compresses a file with zstandard.
 *
 * Notes:
 *   - the output file must not exist
 *   - compression_level is an integer between 1 and 19
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dir_file_compress_zstd (const char *from, const char *to,
                        int compression_level)
{
    FILE *source, *dest;
    void *buffer_in, *buffer_out;
    size_t buffer_in_size, buffer_out_size, num_read, remaining;
    int rc;
#if ZSTD_VERSION_NUMBER >= 10400  /* zstd ≥ 1.4.0 */
    ZSTD_CCtx *cctx = NULL;
    ZSTD_EndDirective mode;
    int finished, last_chunk;
#else  /* zstd < 1.4.0 */
    ZSTD_CStream *cstream = NULL;
    size_t result, to_read;
#endif
    ZSTD_inBuffer input;
    ZSTD_outBuffer output;

    source = NULL;
    dest = NULL;
    buffer_in = NULL;
    buffer_out = NULL;
    rc = 0;

    if (!from || !to || (compression_level < 1) || (compression_level > 19))
        goto end;

    if (access (to, F_OK) == 0)
        goto end;

    buffer_in_size = ZSTD_CStreamInSize ();
    buffer_in = malloc (buffer_in_size);
    if (!buffer_in)
        goto end;
    buffer_out_size = ZSTD_CStreamOutSize ();
    buffer_out = malloc (buffer_out_size);
    if (!buffer_out)
        goto end;

    source = fopen (from, "rb");
    if (!source)
        goto end;
    dest = fopen (to, "wb");
    if (!dest)
        goto end;

#if ZSTD_VERSION_NUMBER >= 10400  /* zstd ≥ 1.4.0 */
    cctx = ZSTD_createCCtx ();
    if (!cctx)
        goto error;
    ZSTD_CCtx_setParameter (cctx, ZSTD_c_compressionLevel, compression_level);

    while (1)
    {
        num_read = fread (buffer_in, 1, buffer_in_size, source);
        if (ferror (source))
            goto error;
        last_chunk = (num_read < buffer_in_size);
        mode = (last_chunk) ? ZSTD_e_end : ZSTD_e_continue;
        input.src = buffer_in;
        input.size = num_read;
        input.pos = 0;
        finished = 0;
        while (!finished)
        {
            output.dst = buffer_out;
            output.size = buffer_out_size;
            output.pos = 0;
            remaining = ZSTD_compressStream2(cctx, &output , &input, mode);
            if (ZSTD_isError (remaining))
                goto error;
            if ((fwrite (buffer_out, 1, output.pos, dest) != output.pos)
                || ferror (dest))
            {
                goto error;
            }
            finished = (last_chunk) ? (remaining == 0) : (input.pos == input.size);
        };
        if (input.pos != input.size)
            goto error;
        if (last_chunk)
            break;
    }
#else  /* zstd < 1.4.0 */
    cstream = ZSTD_createCStream ();
    if (!cstream)
        goto error;
    result = ZSTD_initCStream (cstream, compression_level);
    if (ZSTD_isError (result))
        goto error;
    to_read = buffer_in_size;
    while ((num_read = fread (buffer_in, 1, buffer_in_size, source)))
    {
        input.src = buffer_in;
        input.size = num_read;
        input.pos = 0;
        while (input.pos < input.size)
        {
            output.dst = buffer_out;
            output.size = buffer_out_size;
            output.pos = 0;
            to_read = ZSTD_compressStream (cstream, &output , &input);
            if (ZSTD_isError (to_read))
                goto error;
            if (to_read > buffer_in_size)
                to_read = buffer_in_size;
            if ((fwrite (buffer_out, 1, output.pos, dest) != output.pos)
                || ferror (dest))
            {
                goto error;
            }
        }
    }

    output.dst = buffer_out;
    output.size = buffer_out_size;
    output.pos = 0;
    remaining = ZSTD_endStream (cstream, &output);
    if (remaining)
        goto error;
    if ((fwrite (buffer_out, 1, output.pos, dest) != output.pos)
        || ferror (dest))
    {
        goto error;
    }
#endif

    rc = 1;
    goto end;

error:
#if ZSTD_VERSION_NUMBER >= 10400  /* zstd ≥ 1.4.0 */
    if (cctx)
    {
        ZSTD_freeCCtx (cctx);
        cctx = NULL;
    }
#else  /* zstd < 1.4.0 */
    if (cstream)
    {
        ZSTD_freeCStream (cstream);
        cstream = NULL;
    }
#endif
    unlink (to);

end:
#if ZSTD_VERSION_NUMBER >= 10400  /* zstd ≥ 1.4.0 */
    if (cctx)
        ZSTD_freeCCtx (cctx);
#else  /* zstd < 1.4.0 */
    if (cstream)
        ZSTD_freeCStream (cstream);
#endif

    if (buffer_in)
        free (buffer_in);
    if (buffer_out)
        free (buffer_out);
    if (source)
        fclose (source);
    if (dest)
        fclose (dest);

    return rc;
}

/*
 * Compresses a file with gzip or zstandard.
 *
 * The output file must not exist.
 *
 * Supported values for parameter "compressor":
 *   - "gzip": gzip compression (via zlib)
 *   - "zstd": zstandard compression
 *
 * Parameter "compression_level" is the compression level as percentage:
 * from 1 (fast, low compression) to 100 (slow, best compression).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
dir_file_compress (const char *filename_input,
                   const char *filename_output,
                   const char *compressor,
                   int compression_level)
{
    int level;

    if (!compressor || (compression_level < 1) || (compression_level > 100))
        return 0;

    if (strcmp (compressor, "gzip") == 0)
    {
        /* convert percent to zlib compression level (1-9) */
        level = (((compression_level - 1) * 9) / 100) + 1;
        return dir_file_compress_gzip (filename_input, filename_output, level);
    }
    else if (strcmp (compressor, "zstd") == 0)
    {
        /* convert percent to zstd compression level (1-19) */
        level = (((compression_level - 1) * 19) / 100) + 1;
        return dir_file_compress_zstd (filename_input, filename_output, level);
    }
    else
    {
        return 0;
    }
}
