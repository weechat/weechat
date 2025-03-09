/*
 * spell-info.c - info for spell checker plugin
 *
 * Copyright (C) 2013-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include "spell.h"


/*
 * Returns spell info "spell_dict".
 */

char *
spell_info_info_spell_dict_cb (const void *pointer, void *data,
                               const char *info_name,
                               const char *arguments)
{
    int rc;
    struct t_gui_buffer *buffer;
    const char *buffer_full_name, *ptr_dict;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    ptr_dict = NULL;

    if (!arguments)
        return NULL;

    buffer_full_name = NULL;
    if (strncmp (arguments, "0x", 2) == 0)
    {
        rc = sscanf (arguments, "%p", &buffer);
        if ((rc != EOF) && (rc != 0) && buffer)
        {
            if (weechat_hdata_check_pointer (weechat_hdata_get ("buffer"),
                                             NULL, buffer))
            {
                buffer_full_name = weechat_buffer_get_string (buffer,
                                                              "full_name");
            }
        }
    }
    else
    {
        buffer_full_name = arguments;
    }

    if (buffer_full_name)
        ptr_dict = spell_get_dict_with_buffer_name (buffer_full_name);

    return (ptr_dict) ? strdup (ptr_dict) : NULL;
}

/*
 * Hooks info for spell plugin.
 */

void
spell_info_init (void)
{
    /* info hooks */
    weechat_hook_info (
        "spell_dict",
        N_("comma-separated list of dictionaries used in buffer"),
        N_("buffer pointer (\"0x12345678\") or buffer full name "
           "(\"irc.libera.#weechat\")"),
        &spell_info_info_spell_dict_cb, NULL, NULL);
}
