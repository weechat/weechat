/*
 * xfer-file.c - file functions for xfer plugin
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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>

#include "../weechat-plugin.h"
#include "xfer.h"
#include "xfer-file.h"
#include "xfer-buffer.h"
#include "xfer-config.h"


/*
 * Searches CRC32 in a filename.
 *
 * If more than one CRC32 are found, the last one is returned
 * (with the higher index in filename).
 *
 * The chars before/after CRC32 must be either beginning/end of string or
 * non-hexadecimal chars.
 *
 * Examples:
 *
 *   test_filename     => NULL (not found: no CRC32)
 *   test_1234abcd     => "1234abcd"
 *   1234abcd_test     => "1234abcd"
 *   1234abcd_12345678 => "12345678"
 *   123456781234abcd  => NULL (not found: missing delimiter around CRC32)
 *
 * Returns pointer to last CRC32 in string, NULL if no CRC32 was found.
 */

const char *
xfer_file_search_crc32 (const char *filename)
{
    int length;
    const char *ptr_string, *ptr_crc32;

    length = 0;
    ptr_crc32 = NULL;

    ptr_string = filename;
    while (ptr_string && ptr_string[0])
    {
        if (((ptr_string[0] >= '0') && (ptr_string[0] <= '9'))
            || ((ptr_string[0] >= 'A') && (ptr_string[0] <= 'F'))
            || ((ptr_string[0] >= 'a') && (ptr_string[0] <= 'f')))
        {
            length++;
        }
        else
        {
            if (length == 8)
                ptr_crc32 = ptr_string - 8;
            length = 0;
        }

        ptr_string = weechat_utf8_next_char (ptr_string);
    }
    if (length == 8)
        ptr_crc32 = ptr_string - 8;

    return ptr_crc32;
}

/*
 * Resumes a download.
 *
 * Returns:
 *   1: OK
 *   0: not resumable
 */

int
xfer_file_resume (struct t_xfer *xfer, const char *filename)
{
    struct stat st;

    if (!weechat_config_boolean (xfer_config_file_auto_resume))
        return 0;

    if (access (filename, W_OK) == 0)
    {
        if (stat (filename, &st) != -1)
        {
            if ((unsigned long long) st.st_size < xfer->size)
            {
                xfer->start_resume = (unsigned long long) st.st_size;
                xfer->pos = xfer->start_resume;
                xfer->last_check_pos = xfer->start_resume;
                return 1;
            }
        }
    }

    /* not resumable */
    return 0;
}

/*
 * Checks if file can be downloaded with a given suffix index (if 0 the
 * filename is unchanged, otherwise .1, .2, etc. are added to the filename).
 *
 * Returns 1 if the file can be downloaded with this suffix, 0 if it can not.
 */

int
xfer_file_check_suffix (struct t_xfer *xfer, int suffix)
{
    char *new_filename, *new_temp_filename;
    const char *ptr_suffix;
    int rc, length_suffix, length, filename_exists, temp_filename_exists;
    int same_files;

    rc = 0;
    new_filename = NULL;
    new_temp_filename = NULL;

    ptr_suffix = weechat_config_string (
        xfer_config_file_download_temporary_suffix);
    length_suffix = (ptr_suffix) ? strlen (ptr_suffix) : 0;

    /* build filename with suffix */
    if (suffix == 0)
    {
        new_filename = strdup (xfer->local_filename);
    }
    else
    {
        length = strlen (xfer->local_filename) + 16 + 1;
        new_filename = malloc (length);
        if (new_filename)
        {
            snprintf (new_filename, length, "%s.%d",
                      xfer->local_filename,
                      suffix);
        }
    }
    if (!new_filename)
        goto error;

    /* build temp filename with suffix */
    length = strlen (new_filename) + length_suffix + 1;
    new_temp_filename = malloc (length);
    if (!new_temp_filename)
        goto error;
    snprintf (new_temp_filename, length,
              "%s%s",
              new_filename,
              (ptr_suffix) ? ptr_suffix : "");

    filename_exists = (access (new_filename, F_OK) == 0);
    temp_filename_exists = (access (new_temp_filename, F_OK) == 0);
    same_files = (length_suffix == 0);

    /* if both filenames don't exist, we can use this prefix */
    if (!filename_exists && !temp_filename_exists)
        goto use_prefix;

    /*
     * we try to resume if one of this condition is true:
     *   - filename == temp filename and it exists
     *   - filename != temp filename and only the temp filename exists
     * in any other case, we skip this suffix index
     */

    if ((same_files && filename_exists)
        || (!same_files && !filename_exists && temp_filename_exists))
    {
        if (xfer_file_resume (xfer, new_temp_filename))
            goto use_prefix;
    }

    /* we skip this suffix index */
    goto end;

use_prefix:
    free (xfer->local_filename);
    xfer->local_filename = new_filename;
    xfer->temp_local_filename = new_temp_filename;
    return 1;

error:
    /*
     * in case of error, we remove the local filename and return 1 to stop the
     * infinite loop used to find a suffix index
     */
    free (xfer->local_filename);
    xfer->local_filename = NULL;
    rc = 1;

end:
    if (new_filename)
        free (new_filename);
    if (new_temp_filename)
        free (new_temp_filename);
    return rc;
}

/*
 * Finds the suffix needed for a file, if the file already exists.
 *
 * If no suffix is needed, nothing is changed in the xfer.
 * If a suffix is needed, temp_local_filename and local_filename are changed
 * and filename_suffix is set with the suffix number (starts to 1).
 */

void
xfer_file_find_suffix (struct t_xfer *xfer)
{
    if (xfer_file_check_suffix (xfer, 0))
        return;

    /* if auto rename is not set, then abort xfer */
    if (!weechat_config_boolean (xfer_config_file_auto_rename))
    {
        xfer_close (xfer, XFER_STATUS_FAILED);
        xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
        return;
    }

    /* loop until we find a suffix we can use, starting with suffix == 1 */
    xfer->filename_suffix = 0;
    while (!xfer_file_check_suffix (xfer, ++xfer->filename_suffix))
    {
    }
}

/*
 * Searches for local filename for a xfer.
 *
 * If type is file/recv, adds a suffix (like .1) if needed.
 * If download is resumable, sets "start_resume" to good value.
 */

void
xfer_file_find_filename (struct t_xfer *xfer)
{
    char *dir_separator, *path;
    struct t_hashtable *options;

    if (!XFER_IS_FILE(xfer->type))
        return;

    options = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (options)
        weechat_hashtable_set (options, "directory", "data");
    path = weechat_string_eval_path_home (
        weechat_config_string (xfer_config_file_download_path),
        NULL, NULL, options);
    if (options)
        weechat_hashtable_free (options);
    if (!path)
        return;

    xfer->local_filename = malloc (strlen (path) +
                                   strlen (xfer->remote_nick) +
                                   strlen (xfer->filename) + 4);
    if (!xfer->local_filename)
    {
        free (path);
        return;
    }

    strcpy (xfer->local_filename, path);
    dir_separator = weechat_info_get ("dir_separator", "");
    if (dir_separator
        && (xfer->local_filename[strlen (xfer->local_filename) - 1] != dir_separator[0]))
    {
        strcat (xfer->local_filename, dir_separator);
    }
    if (dir_separator)
        free (dir_separator);
    if (weechat_config_boolean (xfer_config_file_use_nick_in_filename))
    {
        strcat (xfer->local_filename, xfer->remote_nick);
        strcat (xfer->local_filename, ".");
    }
    strcat (xfer->local_filename, xfer->filename);

    free (path);

    xfer_file_find_suffix (xfer);
}

/*
 * Calculates xfer speed (for files only).
 */

void
xfer_file_calculate_speed (struct t_xfer *xfer, int ended)
{
    time_t local_time, elapsed;
    unsigned long long bytes_per_sec_total;

    local_time = time (NULL);
    if (ended || local_time > xfer->last_check_time)
    {
        if (ended)
        {
            /* calculate bytes per second (global) */
            elapsed = local_time - xfer->start_transfer;
            if (elapsed == 0)
                elapsed = 1;
            xfer->bytes_per_sec = (xfer->pos - xfer->start_resume) / elapsed;
            xfer->eta = 0;
        }
        else
        {
            /* calculate ETA */
            elapsed = local_time - xfer->start_transfer;
            if (elapsed == 0)
                elapsed = 1;
            bytes_per_sec_total = (xfer->pos - xfer->start_resume) / elapsed;
            if (bytes_per_sec_total == 0)
                bytes_per_sec_total = 1;
            xfer->eta = (xfer->size - xfer->pos) / bytes_per_sec_total;

            /* calculate bytes per second (since last check time) */
            elapsed = local_time - xfer->last_check_time;
            if (elapsed == 0)
                elapsed = 1;
            xfer->bytes_per_sec = (xfer->pos - xfer->last_check_pos) / elapsed;
        }
        xfer->last_check_time = local_time;
        xfer->last_check_pos = xfer->pos;
    }
}
