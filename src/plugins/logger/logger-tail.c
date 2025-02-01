/*
 * logger-tail.c - return last lines of a file
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
 * Searches for last EOL in a string.
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

    /* no end-of-line found in string */
    return NULL;
}

/*
 * Compares two lines in arraylist.
 */

int
logger_tail_lines_cmp_cb (void *data,
                          struct t_arraylist *arraylist,
                          void *pointer1,
                          void *pointer2)
{
    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    return weechat_strcmp ((const char *)pointer1, (const char *)pointer2);
}

/*
 * Frees a line in arraylist.
 */

void
logger_tail_lines_free_cb (void *data, struct t_arraylist *arraylist,
                           void *pointer)
{
    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    free (pointer);
}

/*
 * Returns last lines of a file.
 *
 * Note: result must be freed after use.
 */

struct t_arraylist *
logger_tail_file (const char *filename, int lines)
{
    int fd, count_read;
    off_t file_length, file_pos;
    size_t to_read;
    ssize_t bytes_read;
    char buf[LOGGER_TAIL_BUFSIZE + 1];
    char *ptr_buf, *pos_eol, *part_of_line, *new_part_of_line, *line;
    struct t_arraylist *list_lines;

    if (!filename || !filename[0] || (lines < 1))
        return NULL;

    fd = -1;
    part_of_line = 0;
    list_lines = NULL;

    /* allocate arraylist */
    list_lines = weechat_arraylist_new (lines, 0, 1,
                                        &logger_tail_lines_cmp_cb, NULL,
                                        &logger_tail_lines_free_cb, NULL);
    if (!list_lines)
        goto error;

    /* open file */
    fd = open (filename, O_RDONLY);
    if (fd == -1)
        goto error;

    /* seek to the end of file */
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

    /* loop until we have "lines" lines in list */
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
            /* ignore last new line of the file (on first block read only) */
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
                if (part_of_line)
                {
                    if (weechat_asprintf (&line, "%s%s", pos_eol, part_of_line) < 0)
                        goto error;
                    free (part_of_line);
                    part_of_line = NULL;
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
                 * beginning of read buffer reached without EOL, then we
                 * add string to part_of_line, we'll use that later
                 */
                if (part_of_line)
                {
                    if (weechat_asprintf (&new_part_of_line,
                                          "%s%s", buf, part_of_line) < 0)
                    {
                        goto error;
                    }
                    free (part_of_line);
                    part_of_line = new_part_of_line;
                }
                else
                {
                    part_of_line = strdup (buf);
                    if (!part_of_line)
                        goto error;
                }
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

    if (part_of_line)
    {
        /* add part of line (will be freed when arraylist is destroyed) */
        weechat_arraylist_insert (list_lines, 0, part_of_line);
    }

    close (fd);

    return list_lines;

error:
    free (part_of_line);
    weechat_arraylist_free (list_lines);
    if (fd >= 0)
        close (fd);
    return NULL;
}
