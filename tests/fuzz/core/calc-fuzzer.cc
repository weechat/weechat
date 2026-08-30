/*
 * SPDX-FileCopyrightText: 2025-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Fuzz testing on WeeChat core calc functions */

extern "C"
{
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "src/core/core-calc.h"
}

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
    char *str;

    /* Ignore huge data. */
    if (size > 65536)
        return 0;

    str = (char *)malloc (size + 1);
    memcpy (str, data, size);
    str[size] = '\0';

    free (calc_expression (str));

    free (str);

    return 0;
}
