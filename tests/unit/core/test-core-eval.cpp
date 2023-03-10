/*
 * test-core-eval.cpp - test evaluation functions
 *
 * Copyright (C) 2014-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include "src/core/wee-eval.h"
#include "src/core/wee-config.h"
#include "src/core/wee-config-file.h"
#include "src/core/wee-hashtable.h"
#include "src/core/wee-secure.h"
#include "src/core/wee-string.h"
#include "src/core/wee-version.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-line.h"
#include "src/gui/gui-color.h"
#include "src/plugins/plugin.h"
}

#define WEE_CHECK_EVAL(__result, __expr)                                \
    value = eval_expression (__expr, pointers, extra_vars, options);    \
    STRCMP_EQUAL(__result, value);                                      \
    free (value);

TEST_GROUP(CoreEval)
{
};

/*
 * Tests functions:
 *   eval_is_true
 */

TEST(CoreEval, IsTrue)
{
    /* false */
    LONGS_EQUAL(0, eval_is_true (NULL));
    LONGS_EQUAL(0, eval_is_true (""));
    LONGS_EQUAL(0, eval_is_true ("0"));

    /* true */
    LONGS_EQUAL(1, eval_is_true ("00"));
    LONGS_EQUAL(1, eval_is_true ("1"));
    LONGS_EQUAL(1, eval_is_true ("A"));
    LONGS_EQUAL(1, eval_is_true ("abcdef"));
}

/*
 * Tests functions:
 *   eval_expression (condition)
 */

TEST(CoreEval, EvalCondition)
{
    struct t_hashtable *pointers, *extra_vars, *options;
    const char *ptr_debug_output;
    char *value;

    pointers = NULL;

    extra_vars = hashtable_new (32,
                                WEECHAT_HASHTABLE_STRING,
                                WEECHAT_HASHTABLE_STRING,
                                NULL, NULL);
    CHECK(extra_vars);
    hashtable_set (extra_vars, "test", "value");
    hashtable_set (extra_vars, "var_abc", "abc");
    hashtable_set (extra_vars, "var_cond_true", "a==a");
    hashtable_set (extra_vars, "var_cond_false", "a==b");

    options = hashtable_new (32,
                             WEECHAT_HASHTABLE_STRING,
                             WEECHAT_HASHTABLE_STRING,
                             NULL, NULL);
    CHECK(options);
    hashtable_set (options, "type", "condition");

    POINTERS_EQUAL(NULL, eval_expression (NULL, NULL, NULL, options));

    /* conditions evaluated as false */
    WEE_CHECK_EVAL("0", "");
    WEE_CHECK_EVAL("0", "0");
    WEE_CHECK_EVAL("0", "1 == 2");
    WEE_CHECK_EVAL("0", "  1 == 2  ");
    WEE_CHECK_EVAL("0", "==1");
    WEE_CHECK_EVAL("0", "1 >= 2");
    WEE_CHECK_EVAL("0", "2 <= 1");
    WEE_CHECK_EVAL("0", "2 != 2");
    WEE_CHECK_EVAL("0", "18 < 5");
    WEE_CHECK_EVAL("0", "5 > 18");
    WEE_CHECK_EVAL("0", "18 < -5");
    WEE_CHECK_EVAL("0", "-5 > 18");
    WEE_CHECK_EVAL("0", "-18 > 5");
    WEE_CHECK_EVAL("0", "5 < -18");
    WEE_CHECK_EVAL("0", "18.2 < 5");
    WEE_CHECK_EVAL("0", "5 > 18.2");
    WEE_CHECK_EVAL("0", "18.2 < -5");
    WEE_CHECK_EVAL("0", "-5 > 18.2");
    WEE_CHECK_EVAL("0", "-18.2 > 5");
    WEE_CHECK_EVAL("0", "5 < -18.2");
    WEE_CHECK_EVAL("0", "2.3e-2 != 0.023");
    WEE_CHECK_EVAL("0", "0xA3 < 2");
    WEE_CHECK_EVAL("0", "-0xA3 > 2");
    WEE_CHECK_EVAL("0", "1 == 5 > 18");
    WEE_CHECK_EVAL("0", ">1");
    WEE_CHECK_EVAL("0", "abc == def");
    WEE_CHECK_EVAL("0", "()");
    WEE_CHECK_EVAL("0", "(5 > 26)");
    WEE_CHECK_EVAL("0", "((5 > 26))");
    WEE_CHECK_EVAL("0", "(26 < 5)");
    WEE_CHECK_EVAL("0", "abc > def");
    WEE_CHECK_EVAL("0", "1 && 0");
    WEE_CHECK_EVAL("0", "abc && 0");
    WEE_CHECK_EVAL("0", "0 || 0");
    WEE_CHECK_EVAL("0", "0 || 0 || 0");
    WEE_CHECK_EVAL("0", "0 || 1 && 0");
    WEE_CHECK_EVAL("0", "0 || (1 && 0)");
    WEE_CHECK_EVAL("0", "0 || (0 || (1 && 0))");
    WEE_CHECK_EVAL("0", "1 && (0 || 0)");
    WEE_CHECK_EVAL("0", "(0 || 1) && 0");
    WEE_CHECK_EVAL("0", "((0 || 1) && 1) && 0");
    WEE_CHECK_EVAL("0", "(0) ");
    WEE_CHECK_EVAL("0", "abcd =~ (?-i)^ABC");
    WEE_CHECK_EVAL("0", "abcd =~ \\(abcd\\)");
    WEE_CHECK_EVAL("0", "=~abcd");
    WEE_CHECK_EVAL("0", "(abcd) =~ \\(\\(abcd\\)\\)");
    WEE_CHECK_EVAL("0", "abcd !~ ^ABC");
    WEE_CHECK_EVAL("0", "abcd !~ (?-i)^abc");
    WEE_CHECK_EVAL("0", "abcd!~abc");
    WEE_CHECK_EVAL("0", "abcd ==* abce");
    WEE_CHECK_EVAL("0", "abcd ==* ABCD");
    WEE_CHECK_EVAL("0", "abcd ==* a*e");
    WEE_CHECK_EVAL("0", "abcd ==* A*E");
    WEE_CHECK_EVAL("0", "abcd !!* *bc*");
    WEE_CHECK_EVAL("0", "abcd !!* *");
    WEE_CHECK_EVAL("0", "abcd =* abce");
    WEE_CHECK_EVAL("0", "abcd =* a*e");
    WEE_CHECK_EVAL("0", "abcd =* A*E");
    WEE_CHECK_EVAL("0", "abcd !* *bc*");
    WEE_CHECK_EVAL("0", "abcd !* *BC*");
    WEE_CHECK_EVAL("0", "abcd !* *");
    WEE_CHECK_EVAL("0", "abcd ==- abce");
    WEE_CHECK_EVAL("0", "abcd ==- ABCD");
    WEE_CHECK_EVAL("0", "abcd ==- BC");
    WEE_CHECK_EVAL("0", "abcd !!- bc");
    WEE_CHECK_EVAL("0", "abcd =- abce");
    WEE_CHECK_EVAL("0", "abcd !- bc");
    WEE_CHECK_EVAL("0", "abcd !- BC");
    WEE_CHECK_EVAL("0", "${test} == test");
    WEE_CHECK_EVAL("0", "${test2} == value2");
    WEE_CHECK_EVAL("0", "${buffer.number} == 2");
    WEE_CHECK_EVAL("0", "${window.buffer.number} == 2");
    WEE_CHECK_EVAL("0", "${calc:2+3} < 5");
    WEE_CHECK_EVAL("0", "${calc:1.5*3} < 4.5");
    WEE_CHECK_EVAL("0", "${if:${buffer.number}==2?yes:}");
    WEE_CHECK_EVAL("0", "${if:${buffer.number}==2?yes:no} == yes");
    WEE_CHECK_EVAL("0", "yes == ${if:${buffer.number}==2?yes:no}");
    WEE_CHECK_EVAL("0", "${if:\\$==A?yes:}");
    WEE_CHECK_EVAL("0", "${var_does_not_exist}");
    WEE_CHECK_EVAL("0", "${var_abc} == def");

    /* conditions evaluated as true */
    WEE_CHECK_EVAL("1", "1");
    WEE_CHECK_EVAL("1", "123");
    WEE_CHECK_EVAL("1", "abc");
    WEE_CHECK_EVAL("1", "2 == 2");
    WEE_CHECK_EVAL("1", "  2 == 2  ");
    WEE_CHECK_EVAL("1", "==0");
    WEE_CHECK_EVAL("1", "2 >= 1");
    WEE_CHECK_EVAL("1", "1 <= 2");
    WEE_CHECK_EVAL("1", "1 != 2");
    WEE_CHECK_EVAL("1", "18 > 5");
    WEE_CHECK_EVAL("1", "5 < 18");
    WEE_CHECK_EVAL("1", "18 > -5");
    WEE_CHECK_EVAL("1", "-5 < 18");
    WEE_CHECK_EVAL("1", "-18 < 5");
    WEE_CHECK_EVAL("1", "5 > -18");
    WEE_CHECK_EVAL("1", "18.2 > 5");
    WEE_CHECK_EVAL("1", "5 < 18.2");
    WEE_CHECK_EVAL("1", "18.2 > -5");
    WEE_CHECK_EVAL("1", "-5 < 18.2");
    WEE_CHECK_EVAL("1", "-18.2 < 5");
    WEE_CHECK_EVAL("1", "5 > -18.2");
    WEE_CHECK_EVAL("1", "2.3e-2 == 0.023");
    WEE_CHECK_EVAL("1", "0xA3 > 2");
    WEE_CHECK_EVAL("1", "-0xA3 < 2");
    WEE_CHECK_EVAL("1", "1 == 18 > 5");
    WEE_CHECK_EVAL("1", "abc == abc");
    WEE_CHECK_EVAL("1", "'abc' == 'abc'");
    WEE_CHECK_EVAL("1", "\"abc\" == \"abc\"");
    WEE_CHECK_EVAL("1", "(26 > 5)");
    WEE_CHECK_EVAL("1", "((26 > 5))");
    WEE_CHECK_EVAL("1", "(5 < 26)");
    WEE_CHECK_EVAL("1", "<1");
    WEE_CHECK_EVAL("1", "def > abc");
    WEE_CHECK_EVAL("1", "1 && 1");
    WEE_CHECK_EVAL("1", "abc && 1");
    WEE_CHECK_EVAL("1", "0 || 1");
    WEE_CHECK_EVAL("1", "0 || 0 || 1");
    WEE_CHECK_EVAL("1", "1 || 1 && 0");
    WEE_CHECK_EVAL("1", "0 || (1 && 1)");
    WEE_CHECK_EVAL("1", "0 || (0 || (1 && 1))");
    WEE_CHECK_EVAL("1", "1 && (0 || 1)");
    WEE_CHECK_EVAL("1", "(0 || 1) && 1");
    WEE_CHECK_EVAL("1", "((0 || 1) && 1) && 1");
    WEE_CHECK_EVAL("1", "(1)1");
    WEE_CHECK_EVAL("1", "abcd =~ ^ABC");
    WEE_CHECK_EVAL("1", "abcd =~ (?-i)^abc");
    WEE_CHECK_EVAL("1", "abcd=~abc");
    WEE_CHECK_EVAL("1", "=~");
    WEE_CHECK_EVAL("1", "abc=~");
    WEE_CHECK_EVAL("1", "(abcd) =~ (abcd)");
    WEE_CHECK_EVAL("1", "(abcd) =~ \\(abcd\\)");
    WEE_CHECK_EVAL("1", "((abcd)) =~ \\(\\(abcd\\)\\)");
    WEE_CHECK_EVAL("1", "abcd !~ (?-i)^ABC");
    WEE_CHECK_EVAL("1", "abcd !~ \\(abcd\\)");
    WEE_CHECK_EVAL("1", "!~abcd");
    WEE_CHECK_EVAL("1", "abcd !!* abce");
    WEE_CHECK_EVAL("1", "abcd !!* ABCD");
    WEE_CHECK_EVAL("1", "abcd !!* a*e");
    WEE_CHECK_EVAL("1", "abcd !!* A*E");
    WEE_CHECK_EVAL("1", "abcd !!* *BC*");
    WEE_CHECK_EVAL("1", "abcd ==* *bc*");
    WEE_CHECK_EVAL("1", "abcd ==* *");
    WEE_CHECK_EVAL("1", "abcd !* abce");
    WEE_CHECK_EVAL("1", "abcd !* a*e");
    WEE_CHECK_EVAL("1", "abcd !* A*E");
    WEE_CHECK_EVAL("1", "abcd =* *bc*");
    WEE_CHECK_EVAL("1", "abcd =* *BC*");
    WEE_CHECK_EVAL("1", "abcd =* *");
    WEE_CHECK_EVAL("1", "abcd !!- abce");
    WEE_CHECK_EVAL("1", "abcd !!- ABCD");
    WEE_CHECK_EVAL("1", "abcd !!- BC");
    WEE_CHECK_EVAL("1", "abcd ==- bc");
    WEE_CHECK_EVAL("1", "abcd !- abce");
    WEE_CHECK_EVAL("1", "abcd =- bc");
    WEE_CHECK_EVAL("1", "abcd =- BC");
    WEE_CHECK_EVAL("1", "${test} == value");
    WEE_CHECK_EVAL("1", "${test2} ==");
    WEE_CHECK_EVAL("1", "${buffer.number} == 1");
    WEE_CHECK_EVAL("1", "${window.buffer.number} == 1");
    WEE_CHECK_EVAL("1", "${calc:2+3} >= 5");
    WEE_CHECK_EVAL("1", "${calc:1.5*3} >= 4.5");
    WEE_CHECK_EVAL("1", "${if:${buffer.number}==1?yes:}");
    WEE_CHECK_EVAL("1", "${if:${buffer.number}==1?yes:no} == yes");
    WEE_CHECK_EVAL("1", "yes == ${if:${buffer.number}==1?yes:no}");
    WEE_CHECK_EVAL("1", "${if:\\$==\\$?yes:}");
    WEE_CHECK_EVAL("1", "${var_abc}");
    WEE_CHECK_EVAL("1", "${var_abc} == abc");
    WEE_CHECK_EVAL("1", "${var_cond_true}");
    WEE_CHECK_EVAL("0", "${var_cond_true} == zzz");
    WEE_CHECK_EVAL("1", "${var_cond_true} != zzz");
    WEE_CHECK_EVAL("1", "${var_cond_false}");
    WEE_CHECK_EVAL("0", "${var_cond_false} == zzz");
    WEE_CHECK_EVAL("1", "${var_cond_false} != zzz");

    /* evaluation of extra_vars */
    hashtable_set (options, "extra", "eval");
    hashtable_set (extra_vars, "test", "${buffer.number}");
    WEE_CHECK_EVAL("1", "${test} == 1");

    /* test with another prefix/suffix */
    hashtable_set (options, "prefix", "%(");
    WEE_CHECK_EVAL("0", "${buffer.number} == 1");
    WEE_CHECK_EVAL("1", "%(buffer.number} == 1");
    hashtable_set (options, "suffix", ")%");
    WEE_CHECK_EVAL("0", "${buffer.number} == 1");
    WEE_CHECK_EVAL("1", "%(buffer.number)% == 1");
    hashtable_remove (options, "prefix");
    hashtable_remove (options, "suffix");

    /* test with debug level 1 */
    hashtable_set (options, "debug", "1");
    WEE_CHECK_EVAL("1", "abc < def");
    ptr_debug_output = (const char *)hashtable_get (options, "debug_output");
    STRCMP_EQUAL("1:eval_expression(\"abc < def\")\n"
                 "  2:eval_expression_condition(\"abc < def\")\n"
                 "    3:eval_expression_condition(\"abc\")\n"
                 "      4:eval_replace_vars(\"abc\")\n"
                 "      4:== \"abc\"\n"
                 "    3:== \"abc\"\n"
                 "    5:eval_expression_condition(\"def\")\n"
                 "      6:eval_replace_vars(\"def\")\n"
                 "      6:== \"def\"\n"
                 "    5:== \"def\"\n"
                 "    7:eval_compare(\"abc\", \"<\", \"def\")\n"
                 "    7:== \"1\"\n"
                 "  2:== \"1\"\n"
                 "1:== \"1\"",
                 ptr_debug_output);
    hashtable_remove (options, "debug");
    hashtable_remove (options, "debug_output");

    /* test with debug level 2 */
    hashtable_set (options, "debug", "2");
    WEE_CHECK_EVAL("1", "abc < def");
    ptr_debug_output = (const char *)hashtable_get (options, "debug_output");
    STRCMP_EQUAL(
        "1:eval_expression(\"abc < def\")\n"
        "  2:eval_expression_condition(\"abc < def\")\n"
        "    3:eval_strstr_level(\"abc < def\", \"||\", \"(\", \")\", 0)\n"
        "    3:== null\n"
        "    4:eval_strstr_level(\"abc < def\", \"&&\", \"(\", \")\", 0)\n"
        "    4:== null\n"
        "    5:eval_strstr_level(\"abc < def\", \"=~\", \"(\", \")\", 0)\n"
        "    5:== null\n"
        "    6:eval_strstr_level(\"abc < def\", \"!~\", \"(\", \")\", 0)\n"
        "    6:== null\n"
        "    7:eval_strstr_level(\"abc < def\", \"==*\", \"(\", \")\", 0)\n"
        "    7:== null\n"
        "    8:eval_strstr_level(\"abc < def\", \"!!*\", \"(\", \")\", 0)\n"
        "    8:== null\n"
        "    9:eval_strstr_level(\"abc < def\", \"=*\", \"(\", \")\", 0)\n"
        "    9:== null\n"
        "    10:eval_strstr_level(\"abc < def\", \"!*\", \"(\", \")\", 0)\n"
        "    10:== null\n"
        "    11:eval_strstr_level(\"abc < def\", \"==-\", \"(\", \")\", 0)\n"
        "    11:== null\n"
        "    12:eval_strstr_level(\"abc < def\", \"!!-\", \"(\", \")\", 0)\n"
        "    12:== null\n"
        "    13:eval_strstr_level(\"abc < def\", \"=-\", \"(\", \")\", 0)\n"
        "    13:== null\n"
        "    14:eval_strstr_level(\"abc < def\", \"!-\", \"(\", \")\", 0)\n"
        "    14:== null\n"
        "    15:eval_strstr_level(\"abc < def\", \"==\", \"(\", \")\", 0)\n"
        "    15:== null\n"
        "    16:eval_strstr_level(\"abc < def\", \"!=\", \"(\", \")\", 0)\n"
        "    16:== null\n"
        "    17:eval_strstr_level(\"abc < def\", \"<=\", \"(\", \")\", 0)\n"
        "    17:== null\n"
        "    18:eval_strstr_level(\"abc < def\", \"<\", \"(\", \")\", 0)\n"
        "    18:== \"< def\"\n"
        "    19:eval_expression_condition(\"abc\")\n"
        "      20:eval_strstr_level(\"abc\", \"||\", \"(\", \")\", 0)\n"
        "      20:== null\n"
        "      21:eval_strstr_level(\"abc\", \"&&\", \"(\", \")\", 0)\n"
        "      21:== null\n"
        "      22:eval_strstr_level(\"abc\", \"=~\", \"(\", \")\", 0)\n"
        "      22:== null\n"
        "      23:eval_strstr_level(\"abc\", \"!~\", \"(\", \")\", 0)\n"
        "      23:== null\n"
        "      24:eval_strstr_level(\"abc\", \"==*\", \"(\", \")\", 0)\n"
        "      24:== null\n"
        "      25:eval_strstr_level(\"abc\", \"!!*\", \"(\", \")\", 0)\n"
        "      25:== null\n"
        "      26:eval_strstr_level(\"abc\", \"=*\", \"(\", \")\", 0)\n"
        "      26:== null\n"
        "      27:eval_strstr_level(\"abc\", \"!*\", \"(\", \")\", 0)\n"
        "      27:== null\n"
        "      28:eval_strstr_level(\"abc\", \"==-\", \"(\", \")\", 0)\n"
        "      28:== null\n"
        "      29:eval_strstr_level(\"abc\", \"!!-\", \"(\", \")\", 0)\n"
        "      29:== null\n"
        "      30:eval_strstr_level(\"abc\", \"=-\", \"(\", \")\", 0)\n"
        "      30:== null\n"
        "      31:eval_strstr_level(\"abc\", \"!-\", \"(\", \")\", 0)\n"
        "      31:== null\n"
        "      32:eval_strstr_level(\"abc\", \"==\", \"(\", \")\", 0)\n"
        "      32:== null\n"
        "      33:eval_strstr_level(\"abc\", \"!=\", \"(\", \")\", 0)\n"
        "      33:== null\n"
        "      34:eval_strstr_level(\"abc\", \"<=\", \"(\", \")\", 0)\n"
        "      34:== null\n"
        "      35:eval_strstr_level(\"abc\", \"<\", \"(\", \")\", 0)\n"
        "      35:== null\n"
        "      36:eval_strstr_level(\"abc\", \">=\", \"(\", \")\", 0)\n"
        "      36:== null\n"
        "      37:eval_strstr_level(\"abc\", \">\", \"(\", \")\", 0)\n"
        "      37:== null\n"
        "      38:eval_replace_vars(\"abc\")\n"
        "      38:== \"abc\"\n"
        "    19:== \"abc\"\n"
        "    39:eval_expression_condition(\"def\")\n"
        "      40:eval_strstr_level(\"def\", \"||\", \"(\", \")\", 0)\n"
        "      40:== null\n"
        "      41:eval_strstr_level(\"def\", \"&&\", \"(\", \")\", 0)\n"
        "      41:== null\n"
        "      42:eval_strstr_level(\"def\", \"=~\", \"(\", \")\", 0)\n"
        "      42:== null\n"
        "      43:eval_strstr_level(\"def\", \"!~\", \"(\", \")\", 0)\n"
        "      43:== null\n"
        "      44:eval_strstr_level(\"def\", \"==*\", \"(\", \")\", 0)\n"
        "      44:== null\n"
        "      45:eval_strstr_level(\"def\", \"!!*\", \"(\", \")\", 0)\n"
        "      45:== null\n"
        "      46:eval_strstr_level(\"def\", \"=*\", \"(\", \")\", 0)\n"
        "      46:== null\n"
        "      47:eval_strstr_level(\"def\", \"!*\", \"(\", \")\", 0)\n"
        "      47:== null\n"
        "      48:eval_strstr_level(\"def\", \"==-\", \"(\", \")\", 0)\n"
        "      48:== null\n"
        "      49:eval_strstr_level(\"def\", \"!!-\", \"(\", \")\", 0)\n"
        "      49:== null\n"
        "      50:eval_strstr_level(\"def\", \"=-\", \"(\", \")\", 0)\n"
        "      50:== null\n"
        "      51:eval_strstr_level(\"def\", \"!-\", \"(\", \")\", 0)\n"
        "      51:== null\n"
        "      52:eval_strstr_level(\"def\", \"==\", \"(\", \")\", 0)\n"
        "      52:== null\n"
        "      53:eval_strstr_level(\"def\", \"!=\", \"(\", \")\", 0)\n"
        "      53:== null\n"
        "      54:eval_strstr_level(\"def\", \"<=\", \"(\", \")\", 0)\n"
        "      54:== null\n"
        "      55:eval_strstr_level(\"def\", \"<\", \"(\", \")\", 0)\n"
        "      55:== null\n"
        "      56:eval_strstr_level(\"def\", \">=\", \"(\", \")\", 0)\n"
        "      56:== null\n"
        "      57:eval_strstr_level(\"def\", \">\", \"(\", \")\", 0)\n"
        "      57:== null\n"
        "      58:eval_replace_vars(\"def\")\n"
        "      58:== \"def\"\n"
        "    39:== \"def\"\n"
        "    59:eval_compare(\"abc\", \"<\", \"def\")\n"
        "    59:== \"1\"\n"
        "  2:== \"1\"\n"
        "1:== \"1\"",
        ptr_debug_output);
    hashtable_remove (options, "debug");
    hashtable_remove (options, "debug_output");

    hashtable_free (extra_vars);
    hashtable_free (options);
}

/*
 * Tests functions:
 *   eval_expression (expression)
 */

TEST(CoreEval, EvalExpression)
{
    struct t_hashtable *pointers, *extra_vars, *options;
    struct t_config_option *ptr_option;
    char *value, str_value[256], str_expr[256];
    const char *ptr_debug_output;

    pointers = hashtable_new (32,
                              WEECHAT_HASHTABLE_STRING,
                              WEECHAT_HASHTABLE_POINTER,
                              NULL, NULL);
    CHECK(pointers);

    extra_vars = hashtable_new (32,
                                WEECHAT_HASHTABLE_STRING,
                                WEECHAT_HASHTABLE_STRING,
                                NULL, NULL);
    CHECK(extra_vars);
    hashtable_set (extra_vars, "test", "value");
    hashtable_set (extra_vars, "var_abc", "abc");
    hashtable_set (extra_vars, "var_cond_true", "a==a");
    hashtable_set (extra_vars, "var_cond_false", "a==b");

    options = hashtable_new (32,
                             WEECHAT_HASHTABLE_STRING,
                             WEECHAT_HASHTABLE_STRING,
                             NULL, NULL);
    CHECK(options);

    POINTERS_EQUAL(NULL, eval_expression (NULL, NULL, NULL, NULL));

    /* test with simple strings */
    WEE_CHECK_EVAL("", "");
    WEE_CHECK_EVAL("a b c", "a b c");
    WEE_CHECK_EVAL("$", "$");
    WEE_CHECK_EVAL("", "${");
    WEE_CHECK_EVAL("}", "}");
    WEE_CHECK_EVAL("", "${}");
    WEE_CHECK_EVAL("", "${xyz}");

    /* test raw string */
    WEE_CHECK_EVAL("${info:version}", "${raw:${info:version}}");
    WEE_CHECK_EVAL("yes", "${if:${raw:test?}==${raw:test?}?yes:no}");
    WEE_CHECK_EVAL("no", "${if:${raw:test?}==${raw:test}?yes:no}");
    WEE_CHECK_EVAL("16", "${length:${raw:${buffer.number}}}");

    /* test eval of substring */
    WEE_CHECK_EVAL("\t", "${eval:${\\t}}");

    /* test eval of condition */
    WEE_CHECK_EVAL("0", "${eval_cond:}");
    WEE_CHECK_EVAL("0", "${eval_cond:${buffer.number} == 2}");
    WEE_CHECK_EVAL("1", "${eval_cond:${buffer.number} == 1}");
    WEE_CHECK_EVAL("1", "${eval_cond:${var_abc}}");
    WEE_CHECK_EVAL("1", "${eval_cond:${var_abc} == abc}");
    WEE_CHECK_EVAL("0", "${eval_cond:${var_abc} == def}");
    WEE_CHECK_EVAL("1", "${eval_cond:${var_cond_true}}");
    WEE_CHECK_EVAL("1", "${eval_cond:${var_cond_true}}");
    WEE_CHECK_EVAL("0", "${eval_cond:${var_cond_false}}");

    /* test value from extra_vars */
    WEE_CHECK_EVAL("value", "${test}");

    /* test escaped chars */
    WEE_CHECK_EVAL("\t", "${\\t}");
    WEE_CHECK_EVAL("\t", "${esc:\t}");

    /* test range of chars */
    WEE_CHECK_EVAL("0123456789", "${chars:digit}");
    WEE_CHECK_EVAL("0123456789abcdefABCDEF", "${chars:xdigit}");
    WEE_CHECK_EVAL("abcdefghijklmnopqrstuvwxyz", "${chars:lower}");
    WEE_CHECK_EVAL("ABCDEFGHIJKLMNOPQRSTUVWXYZ", "${chars:upper}");
    WEE_CHECK_EVAL("abcdefghijklmnopqrstuvwxyz"
                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ", "${chars:alpha}");
    WEE_CHECK_EVAL("abcdefghijklmnopqrstuvwxyz"
                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                   "0123456789", "${chars:alnum}");
    WEE_CHECK_EVAL("0123456789", "${chars:0-9}");
    WEE_CHECK_EVAL("abcdefgh", "${chars:a-h}");
    WEE_CHECK_EVAL("JKLMNOPQRSTUV", "${chars:J-V}");
    WEE_CHECK_EVAL("é", "${chars:é-é}");
    WEE_CHECK_EVAL("àáâãäåæçèé", "${chars:à-é}");
    WEE_CHECK_EVAL("←↑→↓", "${chars:←-↓}");  /* U+2190 - U+2193 */
    WEE_CHECK_EVAL("▁▂▃▄▅▆▇█▉▊▋▌▍▎▏", "${chars:▁-▏}");  /* U+2581 - U+258F */
    WEE_CHECK_EVAL("", "${chars:Z-A}");  /* invalid (reverse) */

    /* test case conversion: to lower case */
    WEE_CHECK_EVAL("", "${lower:}");
    WEE_CHECK_EVAL("this is a test", "${lower:This is a TEST}");
    WEE_CHECK_EVAL("testé testé", "${lower:TESTÉ Testé}");

    /* test case conversion: to upper case */
    WEE_CHECK_EVAL("", "${upper:}");
    WEE_CHECK_EVAL("THIS IS A TEST", "${upper:This is a TEST}");
    WEE_CHECK_EVAL("TESTÉ TESTÉ", "${upper:TESTÉ Testé}");

    /* test hidden chars */
    WEE_CHECK_EVAL("", "${hide:invalid}");
    WEE_CHECK_EVAL("********", "${hide:*,password}");
    WEE_CHECK_EVAL("\u2603\u2603\u2603", "${hide:${esc:\u2603},abc}");

    /* test cut of chars (invalid values) */
    WEE_CHECK_EVAL("", "${cut:}");
    WEE_CHECK_EVAL("", "${cut:0,}");
    WEE_CHECK_EVAL("", "${cut:a,,}");
    WEE_CHECK_EVAL("", "${cutscr:}");
    WEE_CHECK_EVAL("", "${cutscr:0,}");
    WEE_CHECK_EVAL("", "${cutscr:a,,}");

    /* test cut of chars */
    WEE_CHECK_EVAL("", "${cut:0,,}");
    WEE_CHECK_EVAL("", "${cutscr:0,,}");

    WEE_CHECK_EVAL("", "${cut:0,+,}");
    WEE_CHECK_EVAL("", "${cutscr:0,+,}");

    WEE_CHECK_EVAL("", "${cut:0,,test}");
    WEE_CHECK_EVAL("", "${cutscr:0,,test}");

    WEE_CHECK_EVAL("+", "${cut:0,+,test}");
    WEE_CHECK_EVAL("+", "${cutscr:0,+,test}");

    WEE_CHECK_EVAL("te", "${cut:2,,test}");
    WEE_CHECK_EVAL("te", "${cutscr:2,,test}");

    WEE_CHECK_EVAL("te+", "${cut:2,+,test}");
    WEE_CHECK_EVAL("te+", "${cutscr:2,+,test}");

    WEE_CHECK_EVAL("tes", "${cut:3,,test}");
    WEE_CHECK_EVAL("tes", "${cutscr:3,,test}");
    WEE_CHECK_EVAL("tes", "${cut:+3,,test}");
    WEE_CHECK_EVAL("tes", "${cutscr:+3,,test}");

    WEE_CHECK_EVAL("tes+", "${cut:3,+,test}");
    WEE_CHECK_EVAL("tes+", "${cutscr:3,+,test}");
    WEE_CHECK_EVAL("tes++", "${cut:3,++,test}");
    WEE_CHECK_EVAL("tes++", "${cutscr:3,++,test}");
    WEE_CHECK_EVAL("tes+++", "${cut:3,+++,test}");
    WEE_CHECK_EVAL("tes+++", "${cutscr:3,+++,test}");
    WEE_CHECK_EVAL("tes++++", "${cut:3,++++,test}");
    WEE_CHECK_EVAL("tes++++", "${cutscr:3,++++,test}");
    WEE_CHECK_EVAL("tes…", "${cut:3,…,test}");
    WEE_CHECK_EVAL("tes…", "${cutscr:3,…,test}");
    WEE_CHECK_EVAL("te+", "${cut:+3,+,test}");
    WEE_CHECK_EVAL("te+", "${cutscr:+3,+,test}");
    WEE_CHECK_EVAL("te…", "${cut:+3,…,test}");
    WEE_CHECK_EVAL("te…", "${cutscr:+3,…,test}");
    WEE_CHECK_EVAL("t++", "${cut:+3,++,test}");
    WEE_CHECK_EVAL("t++", "${cutscr:+3,++,test}");
    WEE_CHECK_EVAL("+++", "${cut:+3,+++,test}");
    WEE_CHECK_EVAL("+++", "${cutscr:+3,+++,test}");
    WEE_CHECK_EVAL("", "${cut:+3,++++,test}");
    WEE_CHECK_EVAL("", "${cutscr:+3,++++,test}");

    WEE_CHECK_EVAL("test", "${cut:4,,test}");
    WEE_CHECK_EVAL("test", "${cutscr:4,,test}");
    WEE_CHECK_EVAL("test", "${cut:+4,,test}");
    WEE_CHECK_EVAL("test", "${cutscr:+4,,test}");

    WEE_CHECK_EVAL("test", "${cut:4,+,test}");
    WEE_CHECK_EVAL("test", "${cutscr:4,+,test}");
    WEE_CHECK_EVAL("test", "${cut:+4,+,test}");
    WEE_CHECK_EVAL("test", "${cutscr:+4,+,test}");

    WEE_CHECK_EVAL("éà", "${cut:2,,éàô}");
    WEE_CHECK_EVAL("éà", "${cutscr:2,,éàô}");

    WEE_CHECK_EVAL("éà+", "${cut:2,+,éàô}");
    WEE_CHECK_EVAL("éà+", "${cutscr:2,+,éàô}");

    WEE_CHECK_EVAL("こ+", "${cut:1,+,こんにちは世界}");
    WEE_CHECK_EVAL("+", "${cutscr:1,+,こんにちは世界}");

    WEE_CHECK_EVAL("こん+", "${cut:2,+,こんにちは世界}");
    WEE_CHECK_EVAL("こ+", "${cutscr:2,+,こんにちは世界}");

    WEE_CHECK_EVAL("こんに+", "${cut:3,+,こんにちは世界}");
    WEE_CHECK_EVAL("こ+", "${cutscr:3,+,こんにちは世界}");

    WEE_CHECK_EVAL("こんにち+", "${cut:4,+,こんにちは世界}");
    WEE_CHECK_EVAL("こん+", "${cutscr:4,+,こんにちは世界}");

    WEE_CHECK_EVAL("こんにちは+", "${cut:5,+,こんにちは世界}");
    WEE_CHECK_EVAL("こん+", "${cutscr:4,+,こんにちは世界}");

    WEE_CHECK_EVAL("a+", "${cut:1,+,a${\\u0308}}");
    WEE_CHECK_EVAL("a\u0308", "${cutscr:1,+,a${\\u0308}}");

    /* test reverse of string */
    WEE_CHECK_EVAL("!dlrow ,olleH", "${rev:Hello, world!}");
    WEE_CHECK_EVAL("界世はちにんこ", "${rev:こんにちは世界}");
    WEE_CHECK_EVAL("!dlrow30F\x19 ,olleH",
                   "${rev:Hello, ${color:red}world!}");
    WEE_CHECK_EVAL("Hello, \x19" "F03world!",
                   "${rev:${rev:Hello, ${color:red}world!}}");

    /* test reverse of string (for screen) */
    WEE_CHECK_EVAL("!dlrow ,olleH", "${revscr:Hello, world!}");
    WEE_CHECK_EVAL("界世はちにんこ", "${revscr:こんにちは世界}");
    WEE_CHECK_EVAL("!dlrow\x19" "F03 ,olleH",
                   "${revscr:Hello, ${color:red}world!}");
    WEE_CHECK_EVAL("Hello, \x19" "F03world!",
                   "${revscr:${revscr:Hello, ${color:red}world!}}");

    /* test repeat of string (invalid values) */
    WEE_CHECK_EVAL("", "${repeat:}");
    WEE_CHECK_EVAL("", "${repeat:0}");
    WEE_CHECK_EVAL("", "${repeat:a,x}");

    /* test repeat of string */
    WEE_CHECK_EVAL("", "${repeat:-1,x}");
    WEE_CHECK_EVAL("", "${repeat:0,x}");
    WEE_CHECK_EVAL("x", "${repeat:1,x}");
    WEE_CHECK_EVAL("xxxxx", "${repeat:5,x}");
    WEE_CHECK_EVAL("cbacbacba", "${repeat:3,${rev:abc}}");
    WEE_CHECK_EVAL("cbacba", "${repeat:${rev:20},${rev:abc}}");

    /* test length of string */
    WEE_CHECK_EVAL("0", "${length:}");
    WEE_CHECK_EVAL("4", "${length:test}");
    WEE_CHECK_EVAL("7", "${length:こんにちは世界}");
    WEE_CHECK_EVAL("7", "${length:${color:green}こんにちは世界}");

    WEE_CHECK_EVAL("0", "${lengthscr:}");
    WEE_CHECK_EVAL("4", "${lengthscr:test}");
    WEE_CHECK_EVAL("14", "${lengthscr:こんにちは世界}");
    WEE_CHECK_EVAL("14", "${lengthscr:${color:green}こんにちは世界}");

    /* test split of string */
    WEE_CHECK_EVAL("", "${split:}");
    WEE_CHECK_EVAL("", "${split:1}");
    WEE_CHECK_EVAL("", "${split:1,}");
    WEE_CHECK_EVAL("", "${split:1,,}");
    WEE_CHECK_EVAL("", "${split:count}");
    WEE_CHECK_EVAL("", "${split:count,}");
    WEE_CHECK_EVAL("", "${split:count,,}");
    WEE_CHECK_EVAL("0", "${split:count,,,}");

    WEE_CHECK_EVAL("abc", "${split:random,,,abc}");

    WEE_CHECK_EVAL("", "${split:0,,,abc,def,ghi}");
    WEE_CHECK_EVAL("abc", "${split:1,,,abc}");
    WEE_CHECK_EVAL("abc", "${split:1,,,abc,def}");
    WEE_CHECK_EVAL("abc", "${split:1,,,abc,def,ghi}");
    WEE_CHECK_EVAL("def", "${split:2,,,abc,def,ghi}");
    WEE_CHECK_EVAL("ghi", "${split:3,,,abc,def,ghi}");
    WEE_CHECK_EVAL("", "${split:4,,,abc,def,ghi}");

    WEE_CHECK_EVAL("ghi", "${split:-1,,,abc,def,ghi}");
    WEE_CHECK_EVAL("def", "${split:-2,,,abc,def,ghi}");
    WEE_CHECK_EVAL("abc", "${split:-3,,,abc,def,ghi}");
    WEE_CHECK_EVAL("", "${split:-4,,,abc,def,ghi}");
    WEE_CHECK_EVAL("3", "${split:count,,,abc,def,ghi}");

    WEE_CHECK_EVAL("abc", "${split:1, ,,abc def ghi}");
    WEE_CHECK_EVAL("def", "${split:2, ,,abc def ghi}");
    WEE_CHECK_EVAL("ghi", "${split:3, ,,abc def ghi}");
    WEE_CHECK_EVAL("", "${split:4, ,,abc def ghi}");
    WEE_CHECK_EVAL("3", "${split:count, ,,abc def ghi}");

    WEE_CHECK_EVAL("abc", "${split:1,_-,,abc-def_ghi}");
    WEE_CHECK_EVAL("def", "${split:2,_-,,abc-def_ghi}");
    WEE_CHECK_EVAL("ghi", "${split:3,_-,,abc-def_ghi}");
    WEE_CHECK_EVAL("", "${split:4,_-,,abc-def_ghi}");
    WEE_CHECK_EVAL("3", "${split:count,_-,,abc-def_ghi}");

    WEE_CHECK_EVAL("abc,def,ghi", "${split:1,,keep_eol,abc,def,ghi}");
    WEE_CHECK_EVAL("def,ghi", "${split:2,,keep_eol,abc,def,ghi}");
    WEE_CHECK_EVAL("ghi", "${split:3,,keep_eol,abc,def,ghi}");

    WEE_CHECK_EVAL("abc,def,ghi", "${split:1,,keep_eol+strip_left,,,abc,def,ghi}");
    WEE_CHECK_EVAL("def,ghi", "${split:2,,keep_eol+strip_left,,,abc,def,ghi}");
    WEE_CHECK_EVAL("ghi", "${split:3,,keep_eol+strip_left,,,abc,def,ghi}");

    WEE_CHECK_EVAL("abc,def,ghi", "${split:1,,keep_eol+strip_left+strip_right,,,abc,def,ghi,,}");
    WEE_CHECK_EVAL("def,ghi", "${split:2,,keep_eol+strip_left+strip_right,,,abc,def,ghi,,}");
    WEE_CHECK_EVAL("ghi", "${split:3,,keep_eol+strip_left+strip_right,,,abc,def,ghi,,}");

    WEE_CHECK_EVAL("abc", "${split:1,,collapse_seps,abc,,,def,,ghi}");
    WEE_CHECK_EVAL("def", "${split:2,,collapse_seps,abc,,,def,,ghi}");
    WEE_CHECK_EVAL("ghi", "${split:3,,collapse_seps,abc,,,def,,ghi}");

    WEE_CHECK_EVAL("abc", "${split:1,,strip_items=_,_abc_,_def_,_ghi_}");
    WEE_CHECK_EVAL("def", "${split:2,,strip_items=_,_abc_,_def_,_ghi_}");
    WEE_CHECK_EVAL("ghi", "${split:3,,strip_items=_,_abc_,_def_,_ghi_}");

    WEE_CHECK_EVAL("def", "${split:2,,max_items=0,abc,def,ghi}");
    WEE_CHECK_EVAL("def", "${split:2,,max_items=2,abc,def,ghi}");
    WEE_CHECK_EVAL("", "${split:2,,max_items=1,abc,def,ghi}");

    /* test split of shell arguments */
    WEE_CHECK_EVAL("", "${split_shell:}");
    WEE_CHECK_EVAL("", "${split_shell:1}");
    WEE_CHECK_EVAL("", "${split_shell:1,}");
    WEE_CHECK_EVAL("", "${split_shell:count}");
    WEE_CHECK_EVAL("0", "${split_shell:count,}");

    WEE_CHECK_EVAL("first arg", "${split_shell:random,\"first arg\"}");

    WEE_CHECK_EVAL("", "${split_shell:0,\"first arg\" arg2}");
    WEE_CHECK_EVAL("first arg", "${split_shell:1,\"first arg\" arg2}");
    WEE_CHECK_EVAL("arg2", "${split_shell:2,\"first arg\" arg2}");

    WEE_CHECK_EVAL("arg2", "${split_shell:-1,\"first arg\" arg2}");
    WEE_CHECK_EVAL("first arg", "${split_shell:-2,\"first arg\" arg2}");
    WEE_CHECK_EVAL("", "${split_shell:-3,\"first arg\" arg2}");

    /* test color */
    WEE_CHECK_EVAL(gui_color_get_custom ("green"), "${color:green}");
    WEE_CHECK_EVAL(gui_color_get_custom ("*214"), "${color:*214}");
    snprintf (str_value, sizeof (str_value),
              "%s-test-",
              gui_color_from_option (config_color_chat_delimiters));
    WEE_CHECK_EVAL(str_value, "${color:chat_delimiters}-test-");
    config_file_search_with_string ("weechat.color.chat_host", NULL, NULL,
                                    &ptr_option, NULL);
    if (!ptr_option)
    {
        FAIL("ERROR: option weechat.color.chat_host not found.");
    }
    snprintf (str_value, sizeof (str_value),
              "%s-test-", gui_color_from_option (ptr_option));
    WEE_CHECK_EVAL(str_value, "${color:weechat.color.chat_host}-test-");
    WEE_CHECK_EVAL("test", "${option.not.found}test");

    /* test modifier (invalid values) */
    WEE_CHECK_EVAL("test_", "test_${modifier:}");
    WEE_CHECK_EVAL("test_", "test_${modifier:xxx}");
    WEE_CHECK_EVAL("test_", "test_${modifier:xxx,data}");

    /* test modifier */
    WEE_CHECK_EVAL("test_string", "test_${modifier:xxx,data,string}");
    WEE_CHECK_EVAL("test_no_color",
                   "${modifier:color_decode_ansi,0,test_\x1B[92mno_color}");
    snprintf (str_value, sizeof (str_value),
              "test_%slightgreen",
              gui_color_get_custom ("|lightgreen"));
    WEE_CHECK_EVAL(str_value,
                   "${modifier:color_decode_ansi,1,test_\x1B[92mlightgreen}");
    snprintf (str_value, sizeof (str_value),
              "${modifier:color_encode_ansi,,test_%slightgreen}",
              gui_color_get_custom ("lightgreen"));
    WEE_CHECK_EVAL("test_\x1B[92mlightgreen", str_value);

    /* test info */
    WEE_CHECK_EVAL(version_get_version (), "${info:version}");

    /* test base_encode */
    WEE_CHECK_EVAL("", "${base_encode:}");
    WEE_CHECK_EVAL("", "${base_encode:0,xxx}");
    WEE_CHECK_EVAL("", "${base_encode:100,test string}");
    WEE_CHECK_EVAL("7465737420737472696E67", "${base_encode:16,test string}");
    WEE_CHECK_EVAL("ORSXG5BAON2HE2LOM4======", "${base_encode:32,test string}");
    WEE_CHECK_EVAL("dGVzdCBzdHJpbmc=", "${base_encode:64,test string}");

    /* test base_decode */
    WEE_CHECK_EVAL("", "${base_decode:}");
    WEE_CHECK_EVAL("", "${base_decode:0,xxx}");
    WEE_CHECK_EVAL("", "${base_decode:100,test string}");
    WEE_CHECK_EVAL("test string", "${base_decode:16,7465737420737472696E67}");
    WEE_CHECK_EVAL("test string", "${base_decode:32,ORSXG5BAON2HE2LOM4======}");
    WEE_CHECK_EVAL("test string", "${base_decode:64,dGVzdCBzdHJpbmc=}");

    /* test date */
    WEE_CHECK_EVAL("", "${date:}");
    value = eval_expression ("${date}", pointers, extra_vars, options);
    LONGS_EQUAL(19, strlen (value));
    free (value);
    value = eval_expression ("${date:%H:%M:%S}",
                             pointers, extra_vars, options);
    LONGS_EQUAL(8, strlen (value));
    free (value);

    /* test ternary operator */
    WEE_CHECK_EVAL("1", "${if:5>2}");
    WEE_CHECK_EVAL("0", "${if:1>7}");
    WEE_CHECK_EVAL("yes", "${if:5>2?yes:no}");
    WEE_CHECK_EVAL("no", "${if:1>7?yes:no}");
    WEE_CHECK_EVAL("yes", "${if:5>2 && 6>3?yes:no}");
    WEE_CHECK_EVAL("yes-yes", "${if:5>2?${if:6>3?yes-yes:yes-no}:${if:9>4?no-yes:no-no}}");
    WEE_CHECK_EVAL("yes-no", "${if:5>2?${if:1>7?yes-yes:yes-no}:${if:9>4?no-yes:no-no}}");
    WEE_CHECK_EVAL("no-yes", "${if:1>7?${if:6>3?yes-yes:yes-no}:${if:9>4?no-yes:no-no}}");
    WEE_CHECK_EVAL("no-no", "${if:1>7?${if:1>7?yes-yes:yes-no}:${if:1>7?no-yes:no-no}}");
    WEE_CHECK_EVAL("0", "${if:}");
    WEE_CHECK_EVAL("0", "${if:0}");
    WEE_CHECK_EVAL("1", "${if:1}");
    WEE_CHECK_EVAL("1", "${if:abc}");
    WEE_CHECK_EVAL("0", "${if:abc!=abc}");
    WEE_CHECK_EVAL("1", "${if:abc==abc}");
    WEE_CHECK_EVAL("1", "${if:${if:abc==abc}}");
    WEE_CHECK_EVAL("0", "${if:${rev:${if:42==42?hello:bye}}==eyb}");
    WEE_CHECK_EVAL("1", "${if:${rev:${if:42==42?hello:bye}}==olleh}");
    WEE_CHECK_EVAL("1", "${if:${var_abc}}");
    WEE_CHECK_EVAL("1", "${if:${var_abc} == abc}");
    WEE_CHECK_EVAL("0", "${if:${var_abc} == def}");
    WEE_CHECK_EVAL("1", "${if:${var_cond_true}}");
    WEE_CHECK_EVAL("0", "${if:${var_cond_true} == zzz}");
    WEE_CHECK_EVAL("1", "${if:${var_cond_true} != zzz}");
    WEE_CHECK_EVAL("1", "${if:${var_cond_false}}");
    WEE_CHECK_EVAL("0", "${if:${var_cond_false} == zzz}");
    WEE_CHECK_EVAL("1", "${if:${var_cond_false} != zzz}");

    /* test calc */
    WEE_CHECK_EVAL("0", "${calc:}");
    WEE_CHECK_EVAL("123", "${calc:123}");
    WEE_CHECK_EVAL("4", "${calc:1+3}");
    WEE_CHECK_EVAL("8", "${calc:5+1*3}");
    WEE_CHECK_EVAL("18", "${calc:(5+1)*3}");
    WEE_CHECK_EVAL("123129", "${calc:${repeat:2,123}+2*3}");

    /* test random */
    WEE_CHECK_EVAL("0", "${random:}");
    WEE_CHECK_EVAL("0", "${random:1}");
    WEE_CHECK_EVAL("0", "${random:1,}");
    WEE_CHECK_EVAL("0", "${random:5,1}");
    WEE_CHECK_EVAL("2", "${random:2,2}");
    snprintf (str_expr, sizeof (str_expr),
              "${random:-1,%ld}",
              (long)RAND_MAX);
    WEE_CHECK_EVAL("0", str_expr);
    snprintf (str_expr, sizeof (str_expr),
              "${random:%ld,%ld}",
              (long)RAND_MAX,
              (long)RAND_MAX);
    snprintf (str_value, sizeof (str_value),
              "%ld",
              (long)RAND_MAX);
    WEE_CHECK_EVAL(str_value, str_expr);

    /* test translation */
    WEE_CHECK_EVAL("", "${translate:}");
    WEE_CHECK_EVAL("abcdef", "${translate:abcdef}");

    /* test user variables */
    WEE_CHECK_EVAL("", "${define:}");
    WEE_CHECK_EVAL("", "${define:test}");
    WEE_CHECK_EVAL("", "${define:test,value}");
    WEE_CHECK_EVAL("value", "${define:test,value}${test}");
    WEE_CHECK_EVAL("8", "${define:test,${calc:5+3}}${test}");
    WEE_CHECK_EVAL("value", "${define:buffer,value}${buffer}");

    /* test option */
    hashtable_set (secure_hashtable_data, "sec_option", "sec_value");
    WEE_CHECK_EVAL("sec_value", "${sec.data.sec_option}");
    hashtable_remove (secure_hashtable_data, "sec_option");
    snprintf (str_value, sizeof (str_value),
              "%d", CONFIG_INTEGER(config_look_scroll_amount));
    WEE_CHECK_EVAL(str_value, "${weechat.look.scroll_amount}");
    WEE_CHECK_EVAL(str_value, "${${window.buffer.name}.look.scroll_amount}");
    WEE_CHECK_EVAL("right", "${weechat.look.prefix_align}");
    WEE_CHECK_EVAL("1", "${weechat.startup.display_logo}");
    WEE_CHECK_EVAL("=!=", "${weechat.look.prefix_error}");
    WEE_CHECK_EVAL("lightcyan", "${weechat.color.chat_nick}");

    /* test buffer local variable */
    WEE_CHECK_EVAL("core", "${plugin}");
    WEE_CHECK_EVAL("weechat", "${name}");

    /* test hdata */
    hashtable_set (pointers, "my_null_pointer", (const void *)0x0);
    hashtable_set (pointers, "my_buffer_pointer", gui_buffers);
    hashtable_set (pointers, "my_other_pointer", (const void *)0x1234abcd);
    WEE_CHECK_EVAL("x", "x${buffer.number");
    WEE_CHECK_EVAL("x${buffer.number}1",
                   "x\\${buffer.number}${buffer.number}");
    WEE_CHECK_EVAL("1", "${buffer.number}");
    WEE_CHECK_EVAL("1", "${window.buffer.number}");
    WEE_CHECK_EVAL("core.weechat", "${buffer.full_name}");
    WEE_CHECK_EVAL("core.weechat", "${window.buffer.full_name}");
    WEE_CHECK_EVAL("", "${buffer[].full_name}");
    WEE_CHECK_EVAL("", "${buffer[0x0].full_name}");
    WEE_CHECK_EVAL("", "${buffer[0x1].full_name}");
    WEE_CHECK_EVAL("", "${buffer[unknown_list].full_name}");
    WEE_CHECK_EVAL("", "${unknown_pointer}");
    WEE_CHECK_EVAL("", "${my_null_pointer}");
    snprintf (str_value, sizeof (str_value),
              "0x%lx", (long unsigned int)gui_buffers);
    WEE_CHECK_EVAL(str_value, "${my_buffer_pointer}");
    WEE_CHECK_EVAL("0x1234abcd", "${my_other_pointer}");
    WEE_CHECK_EVAL("", "${buffer[unknown_pointer].full_name}");
    WEE_CHECK_EVAL("", "${buffer[my_null_pointer].full_name}");
    WEE_CHECK_EVAL("core.weechat", "${buffer[my_buffer_pointer].full_name}");
    WEE_CHECK_EVAL("", "${buffer[my_other_pointer].full_name}");
    WEE_CHECK_EVAL("core.weechat", "${buffer[gui_buffers].full_name}");
    snprintf (str_value, sizeof (str_value),
              "${buffer[0x%lx].full_name}", (long unsigned int)gui_buffers);
    WEE_CHECK_EVAL("core.weechat", str_value);
    snprintf (str_value, sizeof (str_value), "%c", 1);
    WEE_CHECK_EVAL(str_value,
                   "${window.buffer.own_lines.first_line.data.displayed}");
    WEE_CHECK_EVAL("1", "${window.buffer.num_displayed}");
    snprintf (str_value, sizeof (str_value),
              "%lld",
              (long long)(gui_buffers->own_lines->first_line->data->date));
    WEE_CHECK_EVAL(str_value,
                   "${window.buffer.own_lines.first_line.data.date}");
    snprintf (str_value, sizeof (str_value),
              "0x%lx", (long unsigned int)(gui_buffers->local_variables));
    WEE_CHECK_EVAL(str_value, "${window.buffer.local_variables}");
    WEE_CHECK_EVAL("core", "${window.buffer.local_variables.plugin}");
    WEE_CHECK_EVAL("weechat", "${window.buffer.local_variables.name}");
    WEE_CHECK_EVAL("name,plugin", "${window.buffer.local_variables.keys_sorted()}");
    WEE_CHECK_EVAL("name:weechat,plugin:core", "${window.buffer.local_variables.keys_values_sorted()}");
    WEE_CHECK_EVAL("", "${window.buffer.local_variables.nonexisting_func()}");
    WEE_CHECK_EVAL("", "${window.buffer.local_variables.nonexisting_func(}");
    WEE_CHECK_EVAL("", "${window.buffer.local_variables.nonexisting_func)}");
    WEE_CHECK_EVAL("", "${window.buffer.local_variables.keys( )}");
    WEE_CHECK_EVAL("", "${window.buffer.local_variables.()}");
    hashtable_remove_all (pointers);

    /* test with another prefix/suffix */
    hashtable_set (options, "prefix", "<<<");
    WEE_CHECK_EVAL("${info:version}", "${info:version}");
    WEE_CHECK_EVAL("<info:version}", "<info:version}");
    WEE_CHECK_EVAL("<<info:version}", "<<info:version}");
    WEE_CHECK_EVAL(version_get_version (), "<<<info:version}");
    WEE_CHECK_EVAL("1", "<<<buffer.number}");
    hashtable_set (options, "suffix", ">>>");
    WEE_CHECK_EVAL("${info:version}", "${info:version}");
    WEE_CHECK_EVAL("<info:version>", "<info:version>");
    WEE_CHECK_EVAL("<<info:version>>", "<<info:version>>");
    WEE_CHECK_EVAL(version_get_version (), "<<<info:version>>>");
    WEE_CHECK_EVAL("1", "<<<buffer.number>>>");
    hashtable_remove_all (options);

    /* test with debug level 1 */
    hashtable_set (options, "debug", "1");
    WEE_CHECK_EVAL("fedcba", "${rev:abcdef}");
    ptr_debug_output = (const char *)hashtable_get (options, "debug_output");
    STRCMP_EQUAL("1:eval_expression(\"${rev:abcdef}\")\n"
                 "  2:eval_replace_vars(\"${rev:abcdef}\")\n"
                 "    3:eval_replace_vars_cb(\"rev:abcdef\")\n"
                 "    3:== \"fedcba\"\n"
                 "  2:== \"fedcba\"\n"
                 "1:== \"fedcba\"",
                 ptr_debug_output);
    hashtable_remove_all (options);

    /* test with debug level 2 */
    hashtable_set (options, "debug", "2");
    WEE_CHECK_EVAL("fedcba", "${rev:abcdef}");
    ptr_debug_output = (const char *)hashtable_get (options, "debug_output");
    STRCMP_EQUAL("1:eval_expression(\"${rev:abcdef}\")\n"
                 "  2:eval_replace_vars(\"${rev:abcdef}\")\n"
                 "    3:eval_replace_vars_cb(\"rev:abcdef\")\n"
                 "    3:== \"fedcba\"\n"
                 "  2:== \"fedcba\"\n"
                 "1:== \"fedcba\"",
                 ptr_debug_output);
    hashtable_remove_all (options);

    hashtable_free (pointers);
    hashtable_free (extra_vars);
    hashtable_free (options);
}

/*
 * Tests functions:
 *   eval_expression (replace with regex)
 */

TEST(CoreEval, EvalReplaceRegex)
{
    struct t_hashtable *pointers, *extra_vars, *options;
    char *value;
    regex_t regex;

    pointers = hashtable_new (32,
                              WEECHAT_HASHTABLE_STRING,
                              WEECHAT_HASHTABLE_POINTER,
                              NULL, NULL);
    CHECK(pointers);

    extra_vars = hashtable_new (32,
                                WEECHAT_HASHTABLE_STRING,
                                WEECHAT_HASHTABLE_STRING,
                                NULL, NULL);
    CHECK(extra_vars);
    hashtable_set (extra_vars, "test", "value");

    options = hashtable_new (32,
                             WEECHAT_HASHTABLE_STRING,
                             WEECHAT_HASHTABLE_STRING,
                             NULL, NULL);
    CHECK(options);

    /* replace regex by empty string (on empty string) */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", ".*");
    hashtable_set (options, "regex_replace", "");
    WEE_CHECK_EVAL("", "");

    /* replace regex (on empty string) */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", ".*");
    hashtable_set (options, "regex_replace", "test");
    WEE_CHECK_EVAL("test", "");

    /* replace regex by empty string */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", ".*");
    hashtable_set (options, "regex_replace", "");
    WEE_CHECK_EVAL("", "test");

    /* replace empty regex */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", "");
    hashtable_set (options, "regex_replace", "abc");
    WEE_CHECK_EVAL("test", "test");

    /* replace empty regex by empty string */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", "");
    hashtable_set (options, "regex_replace", "");
    WEE_CHECK_EVAL("test", "test");

    /* add brackets around URLs (regex as string) */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", "[a-zA-Z0-9_]+://[^ ]+");
    hashtable_set (options, "regex_replace", "[ ${re:0} ]");
    WEE_CHECK_EVAL("test: [ https://weechat.org/ ]",
                   "test: https://weechat.org/");

    /* add brackets around URLs (compiled regex) */
    LONGS_EQUAL(0, string_regcomp (&regex, "[a-zA-Z0-9_]+://[^ ]+",
                                   REG_EXTENDED | REG_ICASE));
    hashtable_set (pointers, "regex", &regex);
    hashtable_remove (options, "regex");
    hashtable_set (options, "regex_replace", "[ ${re:0} ]");
    WEE_CHECK_EVAL("test: [ https://weechat.org/ ]",
                   "test: https://weechat.org/");
    regfree (&regex);

    /* hide passwords (regex as string) */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", "(password=)([^ ]+)");
    hashtable_set (options, "regex_replace", "${re:1}${hide:*,${re:2}}");
    WEE_CHECK_EVAL("password=*** password=***",
                   "password=abc password=def");

    /* hide passwords (compiled regex) */
    LONGS_EQUAL(0, string_regcomp (&regex, "(password=)([^ ]+)",
                                   REG_EXTENDED | REG_ICASE));
    hashtable_set (pointers, "regex", &regex);
    hashtable_remove (options, "regex");
    hashtable_set (options, "regex_replace", "${re:1}${hide:*,${re:2}}");
    WEE_CHECK_EVAL("password=*** password=***",
                   "password=abc password=def");
    regfree (&regex);

    /* regex groups */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", "([a-z]+) ([a-z]+) ([a-z]+) ([a-z]+)");
    hashtable_set (options, "regex_replace",
                   "${re:0} -- ${re:1} ${re:+} (${re:#})");
    WEE_CHECK_EVAL("abc def ghi jkl -- abc jkl (4)", "abc def ghi jkl");

    /* invalid regex group */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", "abc");
    hashtable_set (options, "regex_replace", "${re:z}");
    WEE_CHECK_EVAL("", "abc");

    /* use replace index: add number before each item */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", "[^,]+");
    hashtable_set (options, "regex_replace", "${re:repl_index}.${re:0}");
    WEE_CHECK_EVAL("1.item1,2.item2,3.item3", "item1,item2,item3");

    /* use replace index: replace each letter by its position */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", ".");
    hashtable_set (options, "regex_replace", "${re:repl_index}");
    WEE_CHECK_EVAL("1234", "test");

    hashtable_free (pointers);
    hashtable_free (extra_vars);
    hashtable_free (options);
}
