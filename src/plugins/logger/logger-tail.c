/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Return last lines of a file */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "logger.h"
#include "logger-tail.h"


#define LOGGER_TAIL_BUFSIZE 4096


/*
 * Search for last EOL in a string.
 */

const char *
logger_tail_last_eol (const char *string_start, const char *string_ptr)
{
    if (!string_start || !string_ptr)
        return NULL;

    while (string_ptr >= string_start)
    {
        if ((string_ptr[0] == '\n') || (string_ptr[0] == '\r'))
            return string_ptr;
        string_ptr--;
    }

    /* No end-of-line found in string. */
    return NULL;
}

/*
 * Read a line in a file, from offset "start" (included) to offset "end"
 * (excluded).
 *
 * Note: result must be freed after use.
 */

char *
logger_tail_read_line (int fd, off_t start, off_t end)
{
    char *line;
    size_t length, total_read;
    ssize_t bytes_read;

    if ((fd < 0) || (start < 0) || (end < start))
        return NULL;

    length = end - start;
    line = malloc (length + 1);
    if (!line)
        return NULL;

    if (lseek (fd, start, SEEK_SET) < 0)
    {
        free (line);
        return NULL;
    }

    total_read = 0;
    while (total_read < length)
    {
        bytes_read = read (fd, line + total_read, length - total_read);
        if (bytes_read <= 0)
        {
            free (line);
            return NULL;
        }
        total_read += bytes_read;
    }
    line[length] = '\0';

    return line;
}

/*
 * Compare two lines in arraylist.
 */

int
logger_tail_lines_cmp_cb (void *data,
                          struct t_arraylist *arraylist,
                          void *pointer1,
                          void *pointer2)
{
    /* Make C compiler happy. */
    (void) data;
    (void) arraylist;

    return weechat_strcmp ((const char *)pointer1, (const char *)pointer2);
}

/*
 * Free a line in arraylist.
 */

void
logger_tail_lines_free_cb (void *data, struct t_arraylist *arraylist,
                           void *pointer)
{
    /* Make C compiler happy. */
    (void) data;
    (void) arraylist;

    free (pointer);
}

/*
 * Return last lines of a file.
 *
 * Note: result must be freed after use.
 */

struct t_arraylist *
logger_tail_file (const char *filename, int lines)
{
    int fd, count_read;
    off_t file_length, file_pos, part_of_line_end;
    size_t to_read;
    ssize_t bytes_read;
    char buf[LOGGER_TAIL_BUFSIZE + 1];
    char *ptr_buf, *pos_eol, *line;
    struct t_arraylist *list_lines;

    if (!filename || !filename[0] || (lines < 1))
        return NULL;

    fd = -1;
    part_of_line_end = -1;
    list_lines = NULL;

    /* Allocate arraylist. */
    list_lines = weechat_arraylist_new (lines, 0, 1,
                                        &logger_tail_lines_cmp_cb, NULL,
                                        &logger_tail_lines_free_cb, NULL);
    if (!list_lines)
        goto error;

    /* Open file. */
    fd = open (filename, O_RDONLY);
    if (fd == -1)
        goto error;

    /* Seek to the end of file. */
    count_read = 0;
    file_length = lseek (fd, (off_t)0, SEEK_END);
    if (file_length <= 0)
        goto error;
    to_read = file_length;
    file_pos = file_length - LOGGER_TAIL_BUFSIZE;
    if (file_pos < 0)
        file_pos = 0;
    else
        to_read = LOGGER_TAIL_BUFSIZE;
    lseek (fd, file_pos, SEEK_SET);

    /* Loop until we have "lines" lines in list. */
    while (lines > 0)
    {
        lseek (fd, file_pos, SEEK_SET);
        bytes_read = read (fd, buf, to_read);
        if (bytes_read <= 0)
            goto error;
        count_read++;
        buf[bytes_read] = '\0';
        if ((count_read == 1)
            && ((buf[bytes_read - 1] == '\n') || (buf[bytes_read - 1] == '\r')))
        {
            /* Ignore last new line of the file (on first block read only). */
            buf[bytes_read - 1] = '\0';
            bytes_read--;
        }
        ptr_buf = buf + bytes_read - 1;
        while (ptr_buf && (ptr_buf >= buf))
        {
            pos_eol = (char *)logger_tail_last_eol (buf, ptr_buf);
            if (pos_eol)
            {
                ptr_buf = pos_eol - 1;
                pos_eol[0] = '\0';
                pos_eol++;
                if (part_of_line_end >= 0)
                {
                    /*
                     * The line started in a block read before: read the
                     * whole line at once from the file.
                     */
                    line = logger_tail_read_line (fd,
                                                  file_pos + (pos_eol - buf),
                                                  part_of_line_end);
                    if (!line)
                        goto error;
                    part_of_line_end = -1;
                    weechat_arraylist_insert (list_lines, 0, line);
                }
                else
                {
                    weechat_arraylist_insert (list_lines, 0, strdup (pos_eol));
                }
                lines--;
                if (lines <= 0)
                    break;
            }
            else
            {
                /*
                 * Beginning of read buffer reached without EOL, then we
                 * save the position of the end of this line in file, the
                 * line will be read when its beginning is found.
                 */
                if (part_of_line_end < 0)
                    part_of_line_end = file_pos + (ptr_buf - buf) + 1;
                ptr_buf = NULL;
            }
        }
        if (file_pos == 0)
            break;
        to_read = file_pos;
        file_pos -= LOGGER_TAIL_BUFSIZE;
        if (file_pos < 0)
            file_pos = 0;
        else
            to_read = LOGGER_TAIL_BUFSIZE;
    }

    if ((lines > 0) && (part_of_line_end >= 0))
    {
        /* Beginning of file reached: add the first line of file. */
        line = logger_tail_read_line (fd, 0, part_of_line_end);
        if (!line)
            goto error;
        weechat_arraylist_insert (list_lines, 0, line);
    }

    close (fd);

    return list_lines;

error:
    weechat_arraylist_free (list_lines);
    if (fd >= 0)
        close (fd);
    return NULL;
}
