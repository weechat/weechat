/*
 * logger-buffer.c - logger buffer list management
 *
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../weechat-plugin.h"
#include "logger.h"
#include "logger-buffer.h"


struct t_logger_buffer *logger_buffers = NULL;
struct t_logger_buffer *last_logger_buffer = NULL;


/*
 * Checks if a logger buffer pointer is valid.
 *
 * Returns:
 *   1: logger buffer exists
 *   0: logger buffer does not exist
 */

int
logger_buffer_valid (struct t_logger_buffer *logger_buffer)
{
    struct t_logger_buffer *ptr_logger_buffer;

    if (!logger_buffer)
        return 0;

    for (ptr_logger_buffer = logger_buffers; ptr_logger_buffer;
         ptr_logger_buffer = ptr_logger_buffer->next_buffer)
    {
        if (ptr_logger_buffer == logger_buffer)
            return 1;
    }

    /* logger_buffer not found */
    return 0;
}

/*
 * Adds a new buffer for logging.
 *
 * Returns pointer to new logger buffer, NULL if error.
 */

struct t_logger_buffer *
logger_buffer_add (struct t_gui_buffer *buffer, int log_level)
{
    struct t_logger_buffer *new_logger_buffer;

    if (!buffer)
        return NULL;

    if (weechat_logger_plugin->debug)
    {
        weechat_printf_tags (NULL,
                             "no_log",
                             "%s: start logging for buffer \"%s\"",
                             LOGGER_PLUGIN_NAME,
                             weechat_buffer_get_string (buffer, "name"));
    }

    new_logger_buffer = malloc (sizeof (*new_logger_buffer));
    if (new_logger_buffer)
    {
        new_logger_buffer->buffer = buffer;
        new_logger_buffer->log_filename = NULL;
        new_logger_buffer->log_file = NULL;
        new_logger_buffer->log_enabled = 1;
        new_logger_buffer->log_level = log_level;
        new_logger_buffer->write_start_info_line = 1;
        new_logger_buffer->flush_needed = 0;

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
 * Searches for logger buffer by buffer pointer.
 *
 * Returns pointer to logger buffer found, NULL if not found.
 */

struct t_logger_buffer *
logger_buffer_search_buffer (struct t_gui_buffer *buffer)
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
 * Searches for a logger buffer by log filename.
 *
 * Returns pointer to logger buffer found, NULL if not found.
 */

struct t_logger_buffer *
logger_buffer_search_log_filename (const char *log_filename)
{
    struct t_logger_buffer *ptr_logger_buffer;

    if (!log_filename)
        return NULL;

    for (ptr_logger_buffer = logger_buffers; ptr_logger_buffer;
         ptr_logger_buffer = ptr_logger_buffer->next_buffer)
    {
        if (ptr_logger_buffer->log_filename)
        {
            if (strcmp (ptr_logger_buffer->log_filename, log_filename) == 0)
                return ptr_logger_buffer;
        }
    }

    /* logger buffer not found */
    return NULL;
}

/*
 * Removes a logger buffer from list.
 */

void
logger_buffer_free (struct t_logger_buffer *logger_buffer)
{
    struct t_logger_buffer *new_logger_buffers;
    struct t_gui_buffer *ptr_buffer;

    ptr_buffer = logger_buffer->buffer;

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
    if (logger_buffer->log_file)
        fclose (logger_buffer->log_file);

    free (logger_buffer);

    logger_buffers = new_logger_buffers;

    if (weechat_logger_plugin->debug)
    {
        weechat_printf_tags (NULL,
                             "no_log",
                             "%s: stop logging for buffer \"%s\"",
                             LOGGER_PLUGIN_NAME,
                             weechat_buffer_get_string (ptr_buffer, "name"));
    }
}

/*
 * Adds a logger buffer in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
logger_buffer_add_to_infolist (struct t_infolist *infolist,
                               struct t_logger_buffer *logger_buffer)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !logger_buffer)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_pointer (ptr_item, "buffer", logger_buffer->buffer))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "log_filename", logger_buffer->log_filename))
        return 0;
    if (!weechat_infolist_new_var_pointer (ptr_item, "log_file", logger_buffer->log_file))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "log_enabled", logger_buffer->log_enabled))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "log_level", logger_buffer->log_level))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "write_start_info_line", logger_buffer->write_start_info_line))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "flush_needed", logger_buffer->flush_needed))
        return 0;

    return 1;
}
