/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Fuzz testing on relay JSON parser and writer */

extern "C"
{
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "src/core/core-utf8.h"
#include "src/core/core-util.h"
#include "src/plugins/weechat-plugin.h"
#include "src/plugins/relay/relay-json.h"

struct t_weechat_plugin *weechat_relay_plugin = NULL;
}

struct t_weechat_plugin fuzz_relay_plugin;

extern "C" int
LLVMFuzzerInitialize (int *argc, char ***argv)
{
    /* Make C++ compiler happy. */
    (void) argc;
    (void) argv;

    /* Functions of plugin API used by relay JSON functions. */
    memset (&fuzz_relay_plugin, 0, sizeof (fuzz_relay_plugin));
    fuzz_relay_plugin.utf8_is_valid = &utf8_is_valid;
    fuzz_relay_plugin.utf8_char_size = &utf8_char_size;
    fuzz_relay_plugin.util_parse_longlong = &util_parse_longlong;
    weechat_relay_plugin = &fuzz_relay_plugin;

    return 0;
}

extern "C" int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
    struct t_relay_json *json, *json2;
    char *str, *str2, *str3;

    str = (char *)malloc (size + 1);
    memcpy (str, data, size);
    str[size] = '\0';

    json = relay_json_parse (str);
    if (json)
    {
        /* Output of writer must be valid JSON, and stable. */
        str2 = relay_json_print (json);
        if (!str2)
            abort ();
        json2 = relay_json_parse (str2);
        if (!json2)
            abort ();
        str3 = relay_json_print (json2);
        if (!str3 || (strcmp (str2, str3) != 0))
            abort ();
        free (str3);
        relay_json_free (json2);
        free (str2);
        relay_json_free (json);
    }

    free (str);

    return 0;
}
