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

/* Fuzz testing on WeeChat core calc functions */

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "src/core/core-calc.h"

int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
    char *str;

    str = (char *)malloc (size + 1);
    memcpy (str, data, size);
    str[size] = '\0';

    free (calc_expression (str));

    free (str);

    return 0;
}
