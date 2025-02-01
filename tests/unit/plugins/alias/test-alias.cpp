/*
 * test-alias.cpp - test alias functions
 *
 * Copyright (C) 2023-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#include "tests/tests.h"

extern "C"
{
#include "src/plugins/alias/alias.h"

extern char *alias_replace_args (const char *alias_args, const char *user_args);
}

TEST_GROUP(Alias)
{
};

/*
 * Tests functions:
 *   alias_valid
 */

TEST(Alias, Valid)
{
    struct t_alias *alias;

    alias = alias_new ("test_alias", "/mute", NULL);

    LONGS_EQUAL(0, alias_valid (NULL));
    LONGS_EQUAL(0, alias_valid ((struct t_alias *)0x1));
    LONGS_EQUAL(0, alias_valid (alias + 1));

    LONGS_EQUAL(1, alias_valid (alias));

    alias_free (alias);
}

/*
 * Tests functions:
 *   alias_search
 */

TEST(Alias, Search)
{
    struct t_alias *alias;

    alias = alias_new ("test_alias", "/mute", NULL);

    POINTERS_EQUAL(NULL, alias_search (NULL));
    POINTERS_EQUAL(NULL, alias_search (""));
    POINTERS_EQUAL(NULL, alias_search ("does_not_exist"));

    POINTERS_EQUAL(alias, alias_search ("test_alias"));

    alias_free (alias);
}

/*
 * Tests functions:
 *   alias_string_add_word_range
 *   alias_string_add_arguments
 *   alias_replace_args
 */

TEST(Alias, ReplaceArgs)
{
    char *str;

    STRCMP_EQUAL(NULL, alias_replace_args (NULL, NULL));
    STRCMP_EQUAL(NULL, alias_replace_args (NULL, ""));
    STRCMP_EQUAL(NULL, alias_replace_args ("", NULL));

    WEE_TEST_STR("", alias_replace_args ("", ""));
    WEE_TEST_STR("", alias_replace_args ("", "abc def"));
    WEE_TEST_STR("/test", alias_replace_args ("/test", ""));
    WEE_TEST_STR("/test $1", alias_replace_args ("/test \\$1", "abc def"));

    /* arguments by index: $n */
    WEE_TEST_STR("/test $0", alias_replace_args ("/test $0", ""));
    WEE_TEST_STR("/test $0", alias_replace_args ("/test $0", "abc def"));
    WEE_TEST_STR("/test ", alias_replace_args ("/test $1", ""));
    WEE_TEST_STR("/test ", alias_replace_args ("/test $9", "abc def"));
    WEE_TEST_STR("/test abc", alias_replace_args ("/test $1", "abc def"));
    WEE_TEST_STR("/test def abc", alias_replace_args ("/test $2 $1", "abc def"));
    WEE_TEST_STR("/test def abc", alias_replace_args ("/test $2 $1", "abc def"));

    /* arguments from 1 to m: $-m */
    WEE_TEST_STR("/test $-0", alias_replace_args ("/test $-0", ""));
    WEE_TEST_STR("/test $-0", alias_replace_args ("/test $-0", "abc def"));
    WEE_TEST_STR("/test ", alias_replace_args ("/test $-1", ""));
    WEE_TEST_STR("/test abc", alias_replace_args ("/test $-1", "abc def"));
    WEE_TEST_STR("/test abc def", alias_replace_args ("/test $-2", "abc def"));
    WEE_TEST_STR("/test abc def", alias_replace_args ("/test $-3", "abc def"));
    WEE_TEST_STR("/test abc def", alias_replace_args ("/test $-9", "abc def"));

    /* arguments from n to last: $n- */
    WEE_TEST_STR("/test $0-", alias_replace_args ("/test $0-", ""));
    WEE_TEST_STR("/test $0-", alias_replace_args ("/test $0-", "abc def"));
    WEE_TEST_STR("/test ", alias_replace_args ("/test $1-", ""));
    WEE_TEST_STR("/test abc def", alias_replace_args ("/test $1-", "abc def"));
    WEE_TEST_STR("/test def", alias_replace_args ("/test $2-", "abc def"));
    WEE_TEST_STR("/test ", alias_replace_args ("/test $3-", "abc def"));
    WEE_TEST_STR("/test ", alias_replace_args ("/test $9-", "abc def"));

    /* arguments from n to m: $n-m */
    WEE_TEST_STR("/test $0-0", alias_replace_args ("/test $0-0", ""));
    WEE_TEST_STR("/test $0-0", alias_replace_args ("/test $0-0", "abc def"));
    WEE_TEST_STR("/test $0-1", alias_replace_args ("/test $0-1", ""));
    WEE_TEST_STR("/test $0-1", alias_replace_args ("/test $0-1", "abc def"));
    WEE_TEST_STR("/test ", alias_replace_args ("/test $1-1", ""));
    WEE_TEST_STR("/test abc", alias_replace_args ("/test $1-1", "abc def"));
    WEE_TEST_STR("/test abc def", alias_replace_args ("/test $1-2", "abc def"));
    WEE_TEST_STR("/test def", alias_replace_args ("/test $2-2", "abc def"));
    WEE_TEST_STR("/test def", alias_replace_args ("/test $2-3", "abc def"));
    WEE_TEST_STR("/test def", alias_replace_args ("/test $2-9", "abc def"));

    /* all arguments: $* */
    WEE_TEST_STR("/test ", alias_replace_args ("/test $*", ""));
    WEE_TEST_STR("/test abc \"def\"", alias_replace_args ("/test $*", "abc \"def\""));

    /* all arguments with double quotes escaped: $& */
    WEE_TEST_STR("/test ", alias_replace_args ("/test $&", ""));
    WEE_TEST_STR("/test abc \\\"def\\\"", alias_replace_args ("/test $&", "abc \"def\""));

    /* last argument: $~ */
    WEE_TEST_STR("/test ", alias_replace_args ("/test $~", ""));
    WEE_TEST_STR("/test abc", alias_replace_args ("/test $~", "abc"));
    WEE_TEST_STR("/test def", alias_replace_args ("/test $~", "abc def"));

    /* multiple arguments */
    WEE_TEST_STR("/test def abc 'ghi jkl'",
                 alias_replace_args("/test $2 $1 '$3-'", "abc def ghi jkl"));
}

/*
 * Tests functions:
 *   alias_run_command
 */

TEST(Alias, RunCommand)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   alias_cb
 */

TEST(Alias, Cb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   alias_hook_command
 */

TEST(Alias, HookCommand)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   alias_find_pos
 */

TEST(Alias, FindPos)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   alias_insert
 */

TEST(Alias, Insert)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   alias_remove_from_list
 */

TEST(Alias, RemoveFromList)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   alias_rename
 */

TEST(Alias, Rename)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   alias_free
 */

TEST(Alias, Free)
{
    /* test free of NULL alias */
    alias_free (NULL);
}

/*
 * Tests functions:
 *   alias_free_all
 */

TEST(Alias, FreeAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   alias_update_completion
 */

TEST(Alias, UpdateCompletion)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   alias_name_valid
 */

TEST(Alias, NameValid)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   alias_new
 */

TEST(Alias, New)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   alias_add_to_infolist
 */

TEST(Alias, AddToInfolist)
{
    /* TODO: write tests */
}
