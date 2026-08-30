/*
 * SPDX-FileCopyrightText: 2013-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Info for spell checker plugin */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "spell.h"


/*
 * Return spell info "spell_dict".
 */

char *
spell_info_info_spell_dict_cb (const void *pointer, void *data,
                               const char *info_name,
                               const char *arguments)
{
    int rc;
    unsigned long value;
    struct t_gui_buffer *buffer;
    const char *buffer_full_name, *ptr_dict;

    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) info_name;

    ptr_dict = NULL;

    if (!arguments)
        return NULL;

    buffer_full_name = NULL;
    if (strncmp (arguments, "0x", 2) == 0)
    {
        rc = sscanf (arguments, "%lx", &value);
        if ((rc != EOF) && (rc != 0) && value)
        {
            buffer = (struct t_gui_buffer *)value;
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
 * Hook info for spell plugin.
 */

void
spell_info_init (void)
{
    /* Info hooks */
    weechat_hook_info (
        "spell_dict",
        N_("comma-separated list of dictionaries used in buffer"),
        N_("buffer pointer (\"0x12345678\") or buffer full name "
           "(\"irc.libera.#weechat\")"),
        &spell_info_info_spell_dict_cb, NULL, NULL);
}
