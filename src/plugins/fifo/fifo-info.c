/*
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

/*
 * fifo-info.c: info and infolist hooks for fifo plugin
 */

#include <stdlib.h>

#include "../weechat-plugin.h"
#include "fifo.h"


/*
 * Returns info about FIFO pipe.
 */

const char *
fifo_info_get_info_cb (void *data, const char *info_name,
                       const char *arguments)
{
    /* make C compiler happy */
    (void) data;
    (void) arguments;

    if (weechat_strcasecmp (info_name, "fifo_filename") == 0)
    {
        return fifo_filename;
    }

    return NULL;
}

/*
 * Hooks info for fifo plugin.
 */

void
fifo_info_init ()
{
    weechat_hook_info ("fifo_filename", N_("name of FIFO pipe"), NULL,
                       &fifo_info_get_info_cb, NULL);
}
