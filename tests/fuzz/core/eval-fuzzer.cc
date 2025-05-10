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

/* Fuzz testing on WeeChat core evaluation functions */

extern "C"
{
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "src/core/core-config.h"
#include "src/core/core-eval.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-string.h"
#include "src/plugins/plugin.h"
}

extern "C" int
LLVMFuzzerInitialize (int *argc, char ***argv)
{
    /* make C++ compiler happy */
    (void) argc;
    (void) argv;

    string_init ();
    config_weechat_init ();

    return 0;
}

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
    char *str, *expr;
    int length;
    struct t_hashtable *options;

    str = (char *)malloc (size + 1);
    memcpy (str, data, size);
    str[size] = '\0';

    length = size + 128 + 1;
    expr = (char *)malloc (length);

    snprintf (expr, length, "${raw_hl:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${raw:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${hl:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${eval:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${eval_cond:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${esc:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${\\%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${chars:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${lower:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${upper:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${hide:*,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${cut:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${cut:5,…,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${cut:+5,…,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${cutscr:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${cutscr:5,…,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${cutscr:+5,…,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${rev:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${revscr:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${repeat:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${repeat:3,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${length:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${lengthscr:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${split:1,,,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${split:-1,,,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${split:3,,,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${split:3, ,,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${split:count,,,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${split:random,,,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${split:3,,strip_items=_+collapse_seps,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${split:3,,strip_left+strip_right,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${split:3,,keep_eol,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${split:3,,max_items=3,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${split_shell:1,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${split_shell:-1,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${split_shell:3,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${split_shell:count,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${split_shell:random,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${color:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${color:%%.*!/_%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${modifier:color_decode,?,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${modifier:color_decode_ansi,1,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${modifier:color_decode_ansi,0,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${modifier:color_encode_ansi,,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${info:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${info:nick_color,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${info:nick_color,%s;red,blue,green}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${info:nick_color_ignore_case,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${info:nick_color_ignore_case,%s;red,blue,green}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${info:nick_color_name,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${info:nick_color_name,%s;red,blue,green}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${info:nick_color_name_ignore_case,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${info:nick_color_name_ignore_case,%s;red,blue,green}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${base_encode:16,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${base_decode:16,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${base_encode:32,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${base_decode:32,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${base_encode:64,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${base_decode:64,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${base_encode:64url,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${base_decode:64url,%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${date:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${env:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${if:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${if:1?%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));
    snprintf (expr, length, "${if:0?%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${calc:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${random:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${translate:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${define:%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    snprintf (expr, length, "${sec.data.%s}", str);
    free (eval_expression (expr, NULL, NULL, NULL));

    options = hashtable_new (32,
                             WEECHAT_HASHTABLE_STRING, WEECHAT_HASHTABLE_STRING,
                             NULL, NULL);

    hashtable_set (options, "regex", ".*/.*");
    hashtable_set (options, "regex_replace", "${re:0},${re:1},${re:+},${re:#},${re:repl_index}");
    free (eval_expression (str, NULL, NULL, options));

    hashtable_remove_all (options);

    hashtable_set (options, "type", "condition");
    free (eval_expression (str, NULL, NULL, options));

    hashtable_free (options);

    free (str);
    free (expr);

    return 0;
}
