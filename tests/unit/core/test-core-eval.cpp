/*
 * test-core-eval.cpp - test evaluation functions
 *
 * Copyright (C) 2014-2020 Sébastien Helleu <flashcode@flashtux.org>
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

    /* test with debug */
    hashtable_set (options, "debug", "1");
    WEE_CHECK_EVAL("1", "abc < def");
    ptr_debug_output = (const char *)hashtable_get (options, "debug_output");
    STRCMP_EQUAL("eval_expression(\"abc < def\")\n"
                 "eval_expression_condition(\"abc < def\")\n"
                 "eval_strstr_level(\"abc < def\", \"||\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc < def\", \"&&\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc < def\", \"=~\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc < def\", \"!~\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc < def\", \"==*\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc < def\", \"!!*\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc < def\", \"=*\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc < def\", \"!*\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc < def\", \"==-\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc < def\", \"!!-\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc < def\", \"=-\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc < def\", \"!-\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc < def\", \"==\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc < def\", \"!=\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc < def\", \"<=\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc < def\", \"<\", \"(\", \")\", 0)\n"
                 "eval_expression_condition(\"abc\")\n"
                 "eval_strstr_level(\"abc\", \"||\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc\", \"&&\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc\", \"=~\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc\", \"!~\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc\", \"==*\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc\", \"!!*\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc\", \"=*\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc\", \"!*\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc\", \"==-\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc\", \"!!-\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc\", \"=-\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc\", \"!-\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc\", \"==\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc\", \"!=\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc\", \"<=\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc\", \"<\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc\", \">=\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"abc\", \">\", \"(\", \")\", 0)\n"
                 "eval_replace_vars(\"abc\")\n"
                 "eval_expression_condition(\"def\")\n"
                 "eval_strstr_level(\"def\", \"||\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"def\", \"&&\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"def\", \"=~\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"def\", \"!~\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"def\", \"==*\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"def\", \"!!*\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"def\", \"=*\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"def\", \"!*\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"def\", \"==-\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"def\", \"!!-\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"def\", \"=-\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"def\", \"!-\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"def\", \"==\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"def\", \"!=\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"def\", \"<=\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"def\", \"<\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"def\", \">=\", \"(\", \")\", 0)\n"
                 "eval_strstr_level(\"def\", \">\", \"(\", \")\", 0)\n"
                 "eval_replace_vars(\"def\")\n"
                 "eval_compare(\"abc\", \"<\", \"def\")",
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
    char *value, str_value[256];
    const char *ptr_debug_output;

    pointers = NULL;

    extra_vars = hashtable_new (32,
                                WEECHAT_HASHTABLE_STRING,
                                WEECHAT_HASHTABLE_STRING,
                                NULL, NULL);
    CHECK(extra_vars);
    hashtable_set (extra_vars, "test", "value");

    options = NULL;

    POINTERS_EQUAL(NULL, eval_expression (NULL, NULL, NULL, NULL));

    /* test with simple strings */
    WEE_CHECK_EVAL("", "");
    WEE_CHECK_EVAL("a b c", "a b c");
    WEE_CHECK_EVAL("$", "$");
    WEE_CHECK_EVAL("", "${");
    WEE_CHECK_EVAL("}", "}");
    WEE_CHECK_EVAL("", "${}");
    WEE_CHECK_EVAL("", "${xyz}");

    /* test eval of substring */
    WEE_CHECK_EVAL("\t", "${eval:${\\t}}");

    /* test eval of condition */
    WEE_CHECK_EVAL("0", "${eval_cond:}");
    WEE_CHECK_EVAL("0", "${eval_cond:${buffer.number} == 2}");
    WEE_CHECK_EVAL("1", "${eval_cond:${buffer.number} == 1}");

    /* test value from extra_vars */
    WEE_CHECK_EVAL("value", "${test}");

    /* test escaped chars */
    WEE_CHECK_EVAL("\t", "${\\t}");
    WEE_CHECK_EVAL("\t", "${esc:\t}");

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
              gui_color_get_custom ("lightgreen"));
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
    WEE_CHECK_EVAL("0", "${if:0}");
    WEE_CHECK_EVAL("1", "${if:1}");
    WEE_CHECK_EVAL("0", "${if:abc!=abc}");
    WEE_CHECK_EVAL("1", "${if:abc==abc}");
    WEE_CHECK_EVAL("1", "${if:${if:abc==abc}}");
    WEE_CHECK_EVAL("0", "${if:${rev:${if:42==42?hello:bye}}==eyb}");
    WEE_CHECK_EVAL("1", "${if:${rev:${if:42==42?hello:bye}}==olleh}");

    /* test calc */
    WEE_CHECK_EVAL("0", "${calc:}");
    WEE_CHECK_EVAL("123", "${calc:123}");
    WEE_CHECK_EVAL("4", "${calc:1+3}");
    WEE_CHECK_EVAL("8", "${calc:5+1*3}");
    WEE_CHECK_EVAL("18", "${calc:(5+1)*3}");
    WEE_CHECK_EVAL("123129", "${calc:${repeat:2,123}+2*3}");

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
    WEE_CHECK_EVAL("x", "x${buffer.number");
    WEE_CHECK_EVAL("x${buffer.number}1",
                   "x\\${buffer.number}${buffer.number}");
    WEE_CHECK_EVAL("1", "${buffer.number}");
    WEE_CHECK_EVAL("1", "${window.buffer.number}");
    WEE_CHECK_EVAL("core.weechat", "${buffer.full_name}");
    WEE_CHECK_EVAL("core.weechat", "${window.buffer.full_name}");
    WEE_CHECK_EVAL("", "${buffer[0x0].full_name}");
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

    /* test with another prefix/suffix */
    options = hashtable_new (32,
                             WEECHAT_HASHTABLE_STRING,
                             WEECHAT_HASHTABLE_STRING,
                             NULL, NULL);
    CHECK(options);
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
    hashtable_free (options);

    /* test with debug */
    options = hashtable_new (32,
                             WEECHAT_HASHTABLE_STRING,
                             WEECHAT_HASHTABLE_STRING,
                             NULL, NULL);
    CHECK(options);
    hashtable_set (options, "debug", "1");
    WEE_CHECK_EVAL("fedcba", "${rev:abcdef}");
    ptr_debug_output = (const char *)hashtable_get (options, "debug_output");
    STRCMP_EQUAL("eval_expression(\"${rev:abcdef}\")\n"
                 "eval_replace_vars(\"${rev:abcdef}\")\n"
                 "eval_replace_vars_cb(\"rev:abcdef\")",
                 ptr_debug_output);
    hashtable_free (options);

    hashtable_free (extra_vars);
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

    /* REG_NOTBOL (issue #1521) */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", "^(a|b)");
    hashtable_set (options, "regex_replace", "c");
    WEE_CHECK_EVAL("cb", "ab");

    /* replace removes prefix (issue #1521) */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", "^[^ ]+ ");
    hashtable_set (options, "regex_replace", "");
    WEE_CHECK_EVAL("ca va", "allo ca va");

    hashtable_free (pointers);
    hashtable_free (extra_vars);
    hashtable_free (options);
}
