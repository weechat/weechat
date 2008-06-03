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

/* logger-buffer.c: manages logger buffer list */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "logger-buffer.h"


struct t_logger_buffer *logger_buffers = NULL;
struct t_logger_buffer *last_logger_buffer = NULL;


/*
 * logger_buffer_add: add a new buffer for logging
 */

struct t_logger_buffer *
logger_buffer_add (struct t_gui_buffer *buffer, const char *log_filename)
{
    struct t_logger_buffer *new_logger_buffer;
    
    if (!buffer || !log_filename)
        return NULL;
    
    new_logger_buffer = malloc (sizeof (*new_logger_buffer));
    if (new_logger_buffer)
    {
        new_logger_buffer->buffer = buffer;
        new_logger_buffer->log_filename = strdup (log_filename);
        new_logger_buffer->log_file = NULL;
        new_logger_buffer->log_enabled = 1;
        
        new_logger_buffer->prev_buffer = last_logger_buffer;
        new_logger_buffer->next_buffer = NULL;
        if (logger_buffers)
            last_logger_buffer->next_buffer = new_logger_buffer;
        else
            logger_buffers = new_logger_buffer;
        last_logger_buffer = new_logger_buffer;
    }
    
    return new_logger_buffer;
}

/*
 * logger_buffer_search: search a logger buffer by buffer pointer
 */

struct t_logger_buffer *
logger_buffer_search (struct t_gui_buffer *buffer)
{
    struct t_logger_buffer *ptr_logger_buffer;
    
    for (ptr_logger_buffer = logger_buffers; ptr_logger_buffer;
         ptr_logger_buffer = ptr_logger_buffer->next_buffer)
    {
        if (ptr_logger_buffer->buffer == buffer)
            return ptr_logger_buffer;
    }
    
    /* logger buffer not found */
    return NULL;
}

/*
 * logger_buffer_free: remove a logger buffer from list
 */

void
logger_buffer_free (struct t_logger_buffer *logger_buffer)
{
    struct t_logger_buffer *new_logger_buffers;
    
    /* remove logger buffer */
    if (last_logger_buffer == logger_buffer)
        last_logger_buffer = logger_buffer->prev_buffer;
    if (logger_buffer->prev_buffer)
    {
        (logger_buffer->prev_buffer)->next_buffer = logger_buffer->next_buffer;
        new_logger_buffers = logger_buffers;
    }
    else
        new_logger_buffers = logger_buffer->next_buffer;
    
    if (logger_buffer->next_buffer)
        (logger_buffer->next_buffer)->prev_buffer = logger_buffer->prev_buffer;
    
    /* free data */
    if (logger_buffer->log_filename)
        free (logger_buffer->log_filename);
    
    logger_buffers = new_logger_buffers;
}

/*
 * logger_buffer_free_all: remove all buffers from list
 */

void
logger_buffer_free_all ()
{
    while (logger_buffers)
    {
        logger_buffer_free (logger_buffers);
    }
}
