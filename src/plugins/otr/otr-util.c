/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Utility functions for OTR plugin */

#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-util.h"


/*
 * Duplicate a string (NULL is duplicated as an empty string).
 *
 * Note: result must be freed after use.
 */

char *
otr_util_dupstr (const char *string)
{
    return strdup ((string) ? string : "");
}

/*
 * Duplicate a string with CR and LF replaced by spaces, so that it can be sent
 * in a single IRC line.
 *
 * Note: result must be freed after use.
 */

char *
otr_util_irc_sanitize (const char *string)
{
    char *result, *ptr_result;

    result = otr_util_dupstr (string);
    if (!result)
        return NULL;

    for (ptr_result = result; ptr_result[0]; ptr_result++)
    {
        if ((ptr_result[0] == '\r') || (ptr_result[0] == '\n'))
            ptr_result[0] = ' ';
    }

    return result;
}
