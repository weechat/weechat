/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Info and infolist hooks for fifo plugin */

#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "fifo.h"


/*
 * Return FIFO info "fifo_filename".
 */

char *
fifo_info_info_fifo_filename_cb (const void *pointer, void *data,
                                 const char *info_name,
                                 const char *arguments)
{
    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    return (fifo_filename) ? strdup (fifo_filename) : NULL;
}

/*
 * Hook info for fifo plugin.
 */

void
fifo_info_init (void)
{
    weechat_hook_info ("fifo_filename", N_("name of FIFO pipe"), NULL,
                       &fifo_info_info_fifo_filename_cb, NULL, NULL);
}
