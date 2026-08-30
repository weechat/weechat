/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Info and infolist hooks for logger plugin */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "logger.h"
#include "logger-buffer.h"


/*
 * Return info "logger_log_file".
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

    /* Make C compiler happy. */
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
 * Return logger infolist "logger_buffer".
 */

struct t_infolist *
logger_info_infolist_logger_buffer_cb (const void *pointer, void *data,
                                       const char *infolist_name,
                                       void *obj_pointer,
                                       const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_logger_buffer *ptr_logger_buffer;

    /* Make C compiler happy. */
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
        /* Build list with only one logger buffer. */
        if (!logger_buffer_add_to_infolist (ptr_infolist, obj_pointer))
        {
            weechat_infolist_free (ptr_infolist);
            return NULL;
        }
        return ptr_infolist;
    }
    else
    {
        /* Build list with all logger buffers. */
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
 * Hook infolist for logger plugin.
 */

void
logger_info_init (void)
{
    /* Info hooks */
    weechat_hook_info (
        "logger_log_file",
        N_("path to current log filename for the buffer"),
        N_("buffer pointer (\"0x12345678\") or buffer full name "
           "(\"irc.libera.#weechat\")"),
        &logger_info_log_file_cb, NULL, NULL);

    /* Infolist hooks */
    weechat_hook_infolist (
        "logger_buffer", N_("list of logger buffers"),
        N_("logger pointer (optional)"),
        NULL,
        &logger_info_infolist_logger_buffer_cb, NULL, NULL);
}
