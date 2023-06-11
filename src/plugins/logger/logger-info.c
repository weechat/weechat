/*
 * logger-info.c - info and infolist hooks for logger plugin
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
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "logger.h"
#include "logger-buffer.h"


/*
 * Returns info "logger_log_file".
 */

char *
logger_info_log_file_cb (const void *pointer, void *data,
                         const char *info_name,
                         const char *arguments)
{
    int rc;
    unsigned long value;
    struct t_gui_buffer *buffer;
    struct t_logger_buffer *logger_buffer;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    if (!arguments)
        return NULL;

    buffer = NULL;
    if (strncmp (arguments, "0x", 2) == 0)
    {
        rc = sscanf (arguments, "%lx", &value);
        if ((rc != EOF) && (rc != 0) && value)
        {
            if (weechat_hdata_check_pointer (weechat_hdata_get ("buffer"),
                                             NULL,
                                             (struct t_gui_buffer *)value))
            {
                buffer = (struct t_gui_buffer *)value;
            }
        }
    }
    else
    {
        buffer = weechat_buffer_search ("==", arguments);
    }

    if (buffer)
    {
        logger_buffer = logger_buffer_search_buffer (buffer);
        if (logger_buffer && logger_buffer->log_filename)
            return strdup (logger_buffer->log_filename);
    }

    return NULL;
}

/*
 * Returns logger infolist "logger_buffer".
 */

struct t_infolist *
logger_info_infolist_logger_buffer_cb (const void *pointer, void *data,
                                       const char *infolist_name,
                                       void *obj_pointer,
                                       const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_logger_buffer *ptr_logger_buffer;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;
    (void) arguments;

    if (obj_pointer && !logger_buffer_valid (obj_pointer))
        return NULL;

    ptr_infolist = weechat_infolist_new ();
    if (!ptr_infolist)
        return NULL;

    if (obj_pointer)
    {
        /* build list with only one logger buffer */
        if (!logger_buffer_add_to_infolist (ptr_infolist, obj_pointer))
        {
            weechat_infolist_free (ptr_infolist);
            return NULL;
        }
        return ptr_infolist;
    }
    else
    {
        /* build list with all logger buffers */
        for (ptr_logger_buffer = logger_buffers; ptr_logger_buffer;
             ptr_logger_buffer = ptr_logger_buffer->next_buffer)
        {
            if (!logger_buffer_add_to_infolist (ptr_infolist,
                                                ptr_logger_buffer))
            {
                weechat_infolist_free (ptr_infolist);
                return NULL;
            }
        }
        return ptr_infolist;
    }

    return NULL;
}


/*
 * Hooks infolist for logger plugin.
 */

void
logger_info_init ()
{
    /* info hooks */
    weechat_hook_info (
        "logger_log_file",
        N_("path to current log filename for the buffer"),
        N_("buffer pointer (\"0x12345678\") or buffer full name "
           "(\"irc.libera.#weechat\")"),
        &logger_info_log_file_cb, NULL, NULL);

    /* infolist hooks */
    weechat_hook_infolist (
        "logger_buffer", N_("list of logger buffers"),
        N_("logger pointer (optional)"),
        NULL,
        &logger_info_infolist_logger_buffer_cb, NULL, NULL);
}
