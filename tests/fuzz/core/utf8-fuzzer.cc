/*
 * SPDX-FileCopyrightText: 2025 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

/* Fuzz testing on WeeChat core UTF-8 functions */

extern "C"
{
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "src/core/core-config.h"
#include "src/core/core-utf8.h"
}

extern "C" int
LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;

    config_weechat_init ();

    return 0;
}

extern "C" int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
    char *str, *str2, utf8_char[5], *error;
    size_t i;

    str = (char *)malloc (size + 1);
    memcpy (str, data, size);
    str[size] = '\0';

    utf8_has_8bits (str);

    utf8_is_valid (str, size, &error);

    str2 = strdup (str);
    utf8_normalize (str2, '?');
    free (str2);

    for (i = 0; i < 5; i++)
    {
        if (size >= i)
        {
            utf8_prev_char (str, str + i);
            utf8_beginning_of_line (str, str + i);
        }
    }

    utf8_next_char (str);

    utf8_end_of_line (str);

    utf8_char_int (str);

    utf8_int_string (utf8_char_int (str), utf8_char);

    utf8_char_size (str);

    utf8_strlen (str);

    utf8_strnlen (str, size / 2);

    utf8_char_size_screen (str);

    utf8_strlen_screen (str);

    if (size > 4)
    {
        utf8_add_offset (str, 1);
        utf8_real_pos (str, 1);
        utf8_pos (str, 1);
    }

    free (utf8_strndup (str, size / 2));

    if (size > 4)
        utf8_strncpy (utf8_char, str, 1);

    free (str);

    return 0;
}
