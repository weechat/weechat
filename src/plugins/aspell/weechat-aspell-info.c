/*
 * weechat-aspell-info.c - info for aspell plugin
 *
 * Copyright (C) 2013 Sebastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "weechat-aspell.h"


/*
 * Returns aspell info.
 */

const char *
weechat_aspell_info_get_info_cb (void *data, const char *info_name,
                                 const char *arguments)
{
    int rc;
    long unsigned int value;
    struct t_gui_buffer *buffer;
    const char *buffer_full_name;

    /* make C compiler happy */
    (void) data;

    if (weechat_strcasecmp (info_name, "aspell_dict") == 0)
    {
        if (arguments)
        {
            buffer_full_name = NULL;
            if (strncmp (arguments, "0x", 2) == 0)
            {
                rc = sscanf (arguments, "%lx", &value);
                if ((rc != EOF) && (rc != 0))
                {
                    buffer = (struct t_gui_buffer *)value;
                    if (buffer)
                    {
                        buffer_full_name = weechat_buffer_get_string (buffer,
                                                                      "full_name");
                    }
                }
            }
            else
                buffer_full_name = arguments;

            if (buffer_full_name)
                return weechat_aspell_get_dict_with_buffer_name (buffer_full_name);
        }
        return NULL;
    }

    return NULL;
}

/*
 * Hooks info for aspell plugin.
 */

void
weechat_aspell_info_init ()
{
    /* info hooks */
    weechat_hook_info ("aspell_dict",
                       N_("comma-separated list of dictionaries used in buffer"),
                       N_("buffer pointer (\"0x12345678\") or buffer full name "
                          "(\"irc.freenode.#weechat\")"),
                       &weechat_aspell_info_get_info_cb, NULL);
}
