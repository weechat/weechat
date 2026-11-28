/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Utility functions for OTR plugin */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-util.h"


/*
 * Pointer to function memset, called through a volatile pointer so that the
 * C compiler can not remove the wipe of memory freed just after.
 */
void *(*volatile otr_util_memset) (void *, int, size_t) = &memset;


/*
 * Wipe a memory area containing a secret (private key, SMP secret...).
 */

void
otr_util_wipe (void *pointer, size_t size)
{
    if (pointer && (size > 0))
        otr_util_memset (pointer, 0, size);
}

/*
 * Wipe a string containing a secret (private key, SMP secret...), then free
 * it.
 */

void
otr_util_free_secret (char *string)
{
    if (!string)
        return;

    otr_util_wipe (string, strlen (string));
    free (string);
}

/*
 * Open a new temporary file (mode 0600, readable only by the user) to replace
 * a file: the temporary file is "<target>.tmp", where target is the file, or
 * the target of the link if the file is a symbolic link (so that the link is
 * kept).
 *
 * The file must then be closed with function otr_util_file_replace_end.
 *
 * Return the temporary file opened for writing, NULL if error (errno is set).
 */

FILE *
otr_util_file_replace_start (const char *filename, char **target,
                             char **tmp_filename)
{
    FILE *file;
    int fd, saved_errno;

    *target = NULL;
    *tmp_filename = NULL;

    *target = realpath (filename, NULL);
    if (!*target)
        *target = strdup (filename);
    if (!*target)
        goto error;

    weechat_asprintf (tmp_filename, "%s.tmp", *target);
    if (!*tmp_filename)
    {
        errno = ENOMEM;
        goto error;
    }

    /*
     * Always create a new file (an existing file could have other permissions,
     * or be a symbolic link).
     */
    unlink (*tmp_filename);
    fd = open (*tmp_filename, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd < 0)
        goto error;
    file = fdopen (fd, "w");
    if (!file)
    {
        saved_errno = errno;
        close (fd);
        unlink (*tmp_filename);
        errno = saved_errno;
        goto error;
    }

    return file;

error:
    saved_errno = errno;
    free (*target);
    *target = NULL;
    free (*tmp_filename);
    *tmp_filename = NULL;
    errno = saved_errno;
    return NULL;
}

/*
 * Close a temporary file opened with function otr_util_file_replace_start and,
 * if write_ok is 1, replace the target file with it (data is flushed to disk
 * before); if write_ok is 0 or on error, the temporary file is removed.
 *
 * Strings target and tmp_filename are freed.
 *
 * Return 1 if the file has been replaced, 0 if error (errno is set, unless
 * write_ok is 0).
 */

int
otr_util_file_replace_end (FILE *file, char *target, char *tmp_filename,
                           int write_ok)
{
    int rc, saved_errno;

    rc = 0;

    if (write_ok
        && (fflush (file) == 0)
        && (fsync (fileno (file)) == 0))
    {
        rc = 1;
    }
    if (fclose (file) != 0)
        rc = 0;
    if (rc && (rename (tmp_filename, target) != 0))
        rc = 0;

    saved_errno = errno;
    if (!rc)
        unlink (tmp_filename);
    free (target);
    free (tmp_filename);
    errno = saved_errno;

    return rc;
}

/*
 * Duplicate a string (NULL is duplicated as an empty string).
 *
 * Note: result must be freed after use.
 */

char *
otr_util_dupstr (const char *string)
{
    return strdup ((string) ? string : "");
}

/*
 * Duplicate a string with CR and LF replaced by spaces, so that it can be sent
 * in a single IRC line.
 *
 * Note: result must be freed after use.
 */

char *
otr_util_irc_sanitize (const char *string)
{
    char *result, *ptr_result;

    result = otr_util_dupstr (string);
    if (!result)
        return NULL;

    for (ptr_result = result; ptr_result[0]; ptr_result++)
    {
        if ((ptr_result[0] == '\r') || (ptr_result[0] == '\n'))
            ptr_result[0] = ' ';
    }

    return result;
}

/*
 * Duplicate a text received from the peer (for example an SMP question) so
 * that it can be displayed safely in an OTR message: WeeChat color codes are
 * removed and CR/LF are replaced by spaces, so that the peer can not fake
 * other OTR messages (on new lines or with the colors of OTR messages).
 *
 * Note: result must be freed after use.
 */

char *
otr_util_peer_text (const char *text)
{
    char *no_color, *result;

    no_color = weechat_string_remove_color ((text) ? text : "", NULL);
    if (!no_color)
        return NULL;

    result = otr_util_irc_sanitize (no_color);

    free (no_color);

    return result;
}
