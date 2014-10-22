/*
 * test-eval.cpp - test evaluation functions
 *
 * Copyright (C) 2014 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <stdio.h>
#include <regex.h>
#include "src/core/wee-eval.h"
#include "src/core/wee-config.h"
#include "src/core/wee-hashtable.h"
#include "src/core/wee-string.h"
#include "src/core/wee-version.h"
#include "src/gui/gui-color.h"
#include "src/plugins/plugin.h"
}

#define WEE_CHECK_EVAL(__result, __expr)                                \
    value = eval_expression (__expr, pointers, extra_vars, options);    \
    STRCMP_EQUAL(__result, value);                                      \
    free (value);
TEST_GROUP(Eval)
{
};

/*
 * Tests functions:
 *   eval_is_true
 */

TEST(Eval, Boolean)
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

TEST(Eval, EvalCondition)
{
    struct t_hashtable *pointers, *extra_vars, *options;
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
    WEE_CHECK_EVAL("0", "1 >= 2");
    WEE_CHECK_EVAL("0", "2 <= 1");
    WEE_CHECK_EVAL("0", "2 != 2");
    WEE_CHECK_EVAL("0", "18 < 5");
    WEE_CHECK_EVAL("0", "5 > 18");
    WEE_CHECK_EVAL("0", "1 == 5 > 18");
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
    WEE_CHECK_EVAL("0", "abcd =~ (?-i)^ABC");
    WEE_CHECK_EVAL("0", "abcd =~ \\(abcd\\)");
    WEE_CHECK_EVAL("0", "(abcd) =~ \\(\\(abcd\\)\\)");
    WEE_CHECK_EVAL("0", "${test} == test");
    WEE_CHECK_EVAL("0", "${test2} == value2");
    WEE_CHECK_EVAL("0", "${buffer.number} == 2");
    WEE_CHECK_EVAL("0", "${window.buffer.number} == 2");

    /* conditions evaluated as true */
    WEE_CHECK_EVAL("1", "1");
    WEE_CHECK_EVAL("1", "123");
    WEE_CHECK_EVAL("1", "abc");
    WEE_CHECK_EVAL("1", "2 == 2");
    WEE_CHECK_EVAL("1", "2 >= 1");
    WEE_CHECK_EVAL("1", "1 <= 2");
    WEE_CHECK_EVAL("1", "1 != 2");
    WEE_CHECK_EVAL("1", "18 > 5");
    WEE_CHECK_EVAL("1", "5 < 18");
    WEE_CHECK_EVAL("1", "1 == 18 > 5");
    WEE_CHECK_EVAL("1", "abc == abc");
    WEE_CHECK_EVAL("1", "(26 > 5)");
    WEE_CHECK_EVAL("1", "((26 > 5))");
    WEE_CHECK_EVAL("1", "(5 < 26)");
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
    WEE_CHECK_EVAL("1", "abcd =~ ^ABC");
    WEE_CHECK_EVAL("1", "abcd =~ (?-i)^abc");
    WEE_CHECK_EVAL("1", "(abcd) =~ (abcd)");
    WEE_CHECK_EVAL("1", "(abcd) =~ \\(abcd\\)");
    WEE_CHECK_EVAL("1", "((abcd)) =~ \\(\\(abcd\\)\\)");
    WEE_CHECK_EVAL("1", "${test} == value");
    WEE_CHECK_EVAL("1", "${test2} ==");
    WEE_CHECK_EVAL("1", "${buffer.number} == 1");
    WEE_CHECK_EVAL("1", "${window.buffer.number} == 1");

    hashtable_free (extra_vars);
    hashtable_free (options);
}

/*
 * Tests functions:
 *   eval_expression (expression)
 */

TEST(Eval, EvalExpression)
{
    struct t_hashtable *pointers, *extra_vars, *options;
    char *value, str_value[256];
    void *toto;

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

    /* test value from extra_vars */
    WEE_CHECK_EVAL("value", "${test}");

    /* test escaped chars */
    WEE_CHECK_EVAL("\t", "${\\t}");
    WEE_CHECK_EVAL("\t", "${esc:\t}");

    /* test hidden chars */
    WEE_CHECK_EVAL("********", "${hide:*,password}");
    WEE_CHECK_EVAL("\u2603\u2603\u2603", "${hide:${esc:\u2603},abc}");

    /* test color */
    WEE_CHECK_EVAL(gui_color_get_custom ("green"), "${color:green}");
    WEE_CHECK_EVAL(gui_color_get_custom ("*214"), "${color:*214}");

    /* test info */
    WEE_CHECK_EVAL(version_get_version (), "${info:version}");

    /* test option */
    snprintf (str_value, sizeof (str_value),
              "%d", CONFIG_INTEGER(config_look_scroll_amount));
    WEE_CHECK_EVAL(str_value, "${weechat.look.scroll_amount}");
    WEE_CHECK_EVAL(str_value, "${${window.buffer.name}.look.scroll_amount}");

    /* test hdata */
    WEE_CHECK_EVAL("x", "x${buffer.number");
    WEE_CHECK_EVAL("x${buffer.number}1",
                   "x\\${buffer.number}${buffer.number}");
    WEE_CHECK_EVAL("1", "${buffer.number}");
    WEE_CHECK_EVAL("1", "${window.buffer.number}");
    WEE_CHECK_EVAL("core.weechat", "${buffer.full_name}");
    WEE_CHECK_EVAL("core.weechat", "${window.buffer.full_name}");

    hashtable_free (extra_vars);
}

/*
 * Tests functions:
 *   eval_expression (replace with regex)
 */

TEST(Eval, EvalReplaceRegex)
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

    /* add brackets around URLs (regex as string) */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", "\\w+://\\S+");
    hashtable_set (options, "regex_replace", "[ ${re:0} ]");
    WEE_CHECK_EVAL("test: [ http://weechat.org ]",
                   "test: http://weechat.org");

    /* add brackets around URLs (compiled regex) */
    LONGS_EQUAL(0, string_regcomp (&regex, "\\w+://\\S+",
                                   REG_EXTENDED | REG_ICASE));
    hashtable_set (pointers, "regex", &regex);
    hashtable_remove (options, "regex");
    hashtable_set (options, "regex_replace", "[ ${re:0} ]");
    WEE_CHECK_EVAL("test: [ http://weechat.org ]",
                   "test: http://weechat.org");
    regfree (&regex);

    /* hide passwords (regex as string) */
    hashtable_remove (pointers, "regex");
    hashtable_set (options, "regex", "(password=)(\\S+)");
    hashtable_set (options, "regex_replace", "${re:1}${hide:*,${re:2}}");
    WEE_CHECK_EVAL("password=*** password=***",
                   "password=abc password=def");

    /* hide passwords (compiled regex) */
    LONGS_EQUAL(0, string_regcomp (&regex, "(password=)(\\S+)",
                                   REG_EXTENDED | REG_ICASE));
    hashtable_set (pointers, "regex", &regex);
    hashtable_remove (options, "regex");
    hashtable_set (options, "regex_replace", "${re:1}${hide:*,${re:2}}");
    WEE_CHECK_EVAL("password=*** password=***",
                   "password=abc password=def");
    regfree (&regex);

    hashtable_free (pointers);
    hashtable_free (extra_vars);
    hashtable_free (options);
}
