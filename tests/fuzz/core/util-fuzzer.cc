/*
 * SPDX-FileCopyrightText: 2025-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Fuzz testing on WeeChat core util functions */

extern "C"
{
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "src/core/core-config.h"
#include "src/core/core-string.h"
#include "src/core/core-util.h"
}

extern "C" int
LLVMFuzzerInitialize (int *argc, char ***argv)
{
    /* Make C++ compiler happy. */
    (void) argc;
    (void) argv;

    string_init ();
    config_weechat_init ();

    return 0;
}

extern "C" int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
    char *str, str_time[32768];
    unsigned long long delay;
    struct timeval tv;

    str = (char *)malloc (size + 1);
    memcpy (str, data, size);
    str[size] = '\0';

    if (size < 256)
    {
        gettimeofday (&tv, NULL);
        util_strftimeval (str_time, sizeof (str_time), str, &tv);
    }

    util_parse_time (str, &tv);

    util_parse_delay (str, 1, &delay);
    util_parse_delay (str, 10, &delay);

    util_version_number (str);

    free (str);

    return 0;
}
