/*
 * logger-tail.c - return last lines of a file
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

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "logger.h"
#include "logger-tail.h"


#define LOGGER_TAIL_BUFSIZE 4096


/*
 * Searches for last EOL in a string.
 */

const char *
logger_tail_last_eol (const char *string_start, const char *string_ptr)
{
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
 * Returns last lines of a file.
 *
 * Note: result must be freed after use with function logger_tail_free().
 */

struct t_logger_line *
logger_tail_file (const char *filename, int n_lines)
{
    int fd;
    off_t file_length, file_pos;
    size_t to_read;
    ssize_t bytes_read;
    char buf[LOGGER_TAIL_BUFSIZE + 1];
    char *ptr_buf, *pos_eol, *part_of_line, *new_part_of_line;
    struct t_logger_line *ptr_line, *new_line;

    /* open file */
    fd = open (filename, O_RDONLY);
    if (fd == -1)
        return NULL;

    /* seek to the end of file */
    file_length = lseek (fd, (off_t)0, SEEK_END);
    if (file_length <= 0)
    {
        close (fd);
        return NULL;
    }
    to_read = file_length;
    file_pos = file_length - LOGGER_TAIL_BUFSIZE;
    if (file_pos < 0)
        file_pos = 0;
    else
        to_read = LOGGER_TAIL_BUFSIZE;
    lseek (fd, file_pos, SEEK_SET);

    /* loop until we have "n_lines" lines in list */
    part_of_line = NULL;
    ptr_line = NULL;
    while (n_lines > 0)
    {
        lseek (fd, file_pos, SEEK_SET);
        bytes_read = read (fd, buf, to_read);
        if (bytes_read <= 0)
        {
            if (part_of_line)
                free (part_of_line);
            logger_tail_free (ptr_line);
            close (fd);
            return NULL;
        }
        buf[bytes_read] = '\0';
        ptr_buf = buf + bytes_read - 1;
        while (ptr_buf && (ptr_buf >= buf))
        {
            pos_eol = (char *)logger_tail_last_eol (buf, ptr_buf);
            if ((pos_eol && (pos_eol[1] || part_of_line)) || (file_pos == 0))
            {
                /* use data and part_of_line (if existing) to build a new line */
                if (!pos_eol)
                {
                    ptr_buf = NULL;
                    pos_eol = buf;
                }
                else
                {
                    ptr_buf = pos_eol - 1;
                    pos_eol[0] = '\0';
                    pos_eol++;
                }
                if (part_of_line || pos_eol[0])
                {
                    new_line = malloc (sizeof (*new_line));
                    if (!new_line)
                    {
                        logger_tail_free (ptr_line);
                        ptr_line = NULL;
                        break;
                    }
                    if (part_of_line)
                    {
                        new_line->data = malloc ((strlen (pos_eol) +
                                                  strlen (part_of_line) + 1));
                        if (!new_line->data)
                        {
                            free (part_of_line);
                            free (new_line);
                            logger_tail_free (ptr_line);
                            close (fd);
                            return NULL;
                        }
                        strcpy (new_line->data, pos_eol);
                        strcat (new_line->data, part_of_line);
                        free (part_of_line);
                        part_of_line = NULL;
                    }
                    else
                    {
                        new_line->data = strdup (pos_eol);
                    }
                    new_line->next_line = ptr_line;
                    ptr_line = new_line;
                    n_lines--;
                    if (n_lines <= 0)
                        break;
                }
            }
            else if (!pos_eol)
            {
                /*
                 * beginning of read buffer reached without EOL, then we
                 * add string to part_of_line, we'll use that later
                 */
                if (part_of_line)
                {
                    new_part_of_line = malloc (strlen (buf) + strlen (part_of_line) + 1);
                    if (!new_part_of_line)
                    {
                        free (part_of_line);
                        logger_tail_free (ptr_line);
                        close (fd);
                        return NULL;
                    }
                    strcpy (new_part_of_line, buf);
                    strcat (new_part_of_line, part_of_line);
                    free (part_of_line);
                    part_of_line = new_part_of_line;
                }
                else
                {
                    part_of_line = malloc (strlen (buf) + 1);
                    strcpy (part_of_line, buf);
                }
                ptr_buf = NULL;
            }
            else
                ptr_buf = pos_eol - 1;
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
        free (part_of_line);

    close (fd);

    return ptr_line;
}

/*
 * Frees structure returned by function "logger_tail_file".
 */

void
logger_tail_free (struct t_logger_line *lines)
{
    struct t_logger_line *ptr_line, *next_line;

    if (!lines)
        return;

    ptr_line = lines;
    while (ptr_line)
    {
        next_line = ptr_line->next_line;

        if (ptr_line->data)
            free (ptr_line->data);
        free (ptr_line);

        ptr_line = next_line;
    }
}
