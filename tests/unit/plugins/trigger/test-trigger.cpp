/*
 * test-trigger.cpp - test trigger functions
 *
 * Copyright (C) 2021-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdio.h>
#include "src/core/wee-config.h"
#include "src/core/wee-config-file.h"
#include "src/plugins/plugin.h"
#include "src/plugins/trigger/trigger.h"
}

#define WEE_CHECK_REGEX_SPLIT(__rc, __ret_regex_count, __str_regex)     \
    trigger_regex_free (&regex_count, &regex);                          \
    LONGS_EQUAL(__rc, trigger_regex_split (__str_regex,                 \
                                           &regex_count,                \
                                           &regex));                    \
    LONGS_EQUAL(__ret_regex_count, regex_count);                        \
    if (regex_count > 0)                                                \
    {                                                                   \
        CHECK(regex);                                                   \
    }                                                                   \
    else                                                                \
    {                                                                   \
        POINTERS_EQUAL(NULL, regex);                                    \
    }


TEST_GROUP(Trigger)
{
};

/*
 * Tests functions:
 *   trigger_search_option
 */

TEST(Trigger, SearchOption)
{
    int i;

    LONGS_EQUAL(-1, trigger_search_option (NULL));
    LONGS_EQUAL(-1, trigger_search_option (""));
    LONGS_EQUAL(-1, trigger_search_option ("abc"));

    for (i = 0; i < TRIGGER_NUM_OPTIONS; i++)
    {
        LONGS_EQUAL(i, trigger_search_option (trigger_option_string[i]));
    }
}

/*
 * Tests functions:
 *   trigger_search_hook_type
 */

TEST(Trigger, SearchHookType)
{
    int i;

    LONGS_EQUAL(-1, trigger_search_hook_type (NULL));
    LONGS_EQUAL(-1, trigger_search_hook_type (""));
    LONGS_EQUAL(-1, trigger_search_hook_type ("abc"));

    for (i = 0; i < TRIGGER_NUM_HOOK_TYPES; i++)
    {
        LONGS_EQUAL(i,
                    trigger_search_hook_type (trigger_hook_type_string[i]));
    }
}

/*
 * Tests functions:
 *   trigger_search_regex_command
 */

TEST(Trigger, SearchRegexCommand)
{
    int i;

    LONGS_EQUAL(-1, trigger_search_regex_command ('a'));
    LONGS_EQUAL(-1, trigger_search_regex_command ('z'));
    LONGS_EQUAL(-1, trigger_search_regex_command ('/'));
    LONGS_EQUAL(-1, trigger_search_regex_command ('*'));
    LONGS_EQUAL(-1, trigger_search_regex_command (' '));

    for (i = 0; i < TRIGGER_NUM_REGEX_COMMANDS; i++)
    {
        LONGS_EQUAL(i,
                    trigger_search_regex_command (trigger_regex_command[i]));
    }
}

/*
 * Tests functions:
 *   trigger_search_return_code
 */

TEST(Trigger, SearchReturnCode)
{
    int i;

    LONGS_EQUAL(-1, trigger_search_return_code (NULL));
    LONGS_EQUAL(-1, trigger_search_return_code (""));
    LONGS_EQUAL(-1, trigger_search_return_code ("abc"));

    for (i = 0; i < TRIGGER_NUM_RETURN_CODES; i++)
    {
        LONGS_EQUAL(i,
                    trigger_search_return_code (trigger_return_code_string[i]));
    }
}

/*
 * Tests functions:
 *   trigger_search_post_action
 */

TEST(Trigger, SearchPostAction)
{
    int i;

    LONGS_EQUAL(-1, trigger_search_post_action (NULL));
    LONGS_EQUAL(-1, trigger_search_post_action (""));
    LONGS_EQUAL(-1, trigger_search_post_action ("abc"));

    for (i = 0; i < TRIGGER_NUM_POST_ACTIONS; i++)
    {
        LONGS_EQUAL(i,
                    trigger_search_post_action (trigger_post_action_string[i]));
    }
}

/*
 * Tests functions:
 *   trigger_unhook
 */

TEST(Trigger, Unhook)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_hook
 */

TEST(Trigger, Hook)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_regex_split
 *   trigger_regex_free
 */

TEST(Trigger, RegexSplit)
{
    int regex_count;
    struct t_trigger_regex *regex;

    regex_count = 0;
    regex = NULL;

    /* free of regex with invalid arguments */
    trigger_regex_free (NULL, NULL);
    trigger_regex_free (&regex_count, NULL);
    trigger_regex_free (NULL, &regex);

    /* no regex_count / regex */
    LONGS_EQUAL(0, trigger_regex_split (NULL, NULL, NULL));
    LONGS_EQUAL(0, trigger_regex_split (NULL, &regex_count, NULL));
    LONGS_EQUAL(0, trigger_regex_split (NULL, NULL, &regex));

    /* NULL/empty regex */
    WEE_CHECK_REGEX_SPLIT(0, 0, NULL);
    WEE_CHECK_REGEX_SPLIT(0, 0, "");

    /* regex too short (default command "s") */
    WEE_CHECK_REGEX_SPLIT(-1, 0, "/");
    WEE_CHECK_REGEX_SPLIT(-1, 0, "/a");

    /* regex too short with command "s" (regex replace) */
    WEE_CHECK_REGEX_SPLIT(-1, 0, "s/");
    WEE_CHECK_REGEX_SPLIT(-1, 0, "s///");
    WEE_CHECK_REGEX_SPLIT(-1, 0, "s/a");

    /* regex too short with command "y" (translate chars) */
    WEE_CHECK_REGEX_SPLIT(-1, 0, "y/");
    WEE_CHECK_REGEX_SPLIT(-1, 0, "y///");
    WEE_CHECK_REGEX_SPLIT(-1, 0, "y/a");

    /* missing second delimiter */
    WEE_CHECK_REGEX_SPLIT(-1, 0, "/abc");
    WEE_CHECK_REGEX_SPLIT(-1, 0, "s/abc");
    WEE_CHECK_REGEX_SPLIT(-1, 0, "y/abc");

    /* invalid command */
    WEE_CHECK_REGEX_SPLIT(-1, 0, "a/a/b");
    WEE_CHECK_REGEX_SPLIT(-1, 0, "z/a/b");

    /* invalid regex */
    WEE_CHECK_REGEX_SPLIT(-2, 0, "/*/a");
    WEE_CHECK_REGEX_SPLIT(-2, 0, "s/*/a");

    /* simple regex (implicit command "s") */
    WEE_CHECK_REGEX_SPLIT(0, 1, "/a/b");
    POINTERS_EQUAL(NULL, regex[0].variable);
    STRCMP_EQUAL("a", regex[0].str_regex);
    CHECK(regex[0].regex);
    STRCMP_EQUAL("b", regex[0].replace);
    STRCMP_EQUAL("b", regex[0].replace_escaped);

    /* simple regex replace (command "s") */
    WEE_CHECK_REGEX_SPLIT(0, 1, "s/a/b");
    POINTERS_EQUAL(NULL, regex[0].variable);
    STRCMP_EQUAL("a", regex[0].str_regex);
    CHECK(regex[0].regex);
    STRCMP_EQUAL("b", regex[0].replace);
    STRCMP_EQUAL("b", regex[0].replace_escaped);

    /* simple translate chars (command "y") */
    WEE_CHECK_REGEX_SPLIT(0, 1, "y/${chars:a-h}/${chars:A-H}");
    POINTERS_EQUAL(NULL, regex[0].variable);
    STRCMP_EQUAL("${chars:a-h}", regex[0].str_regex);
    POINTERS_EQUAL(NULL, regex[0].regex);
    STRCMP_EQUAL("${chars:A-H}", regex[0].replace);
    POINTERS_EQUAL(NULL, regex[0].replace_escaped);

    /* simple regex replace with variable (implicit command "s") */
    WEE_CHECK_REGEX_SPLIT(0, 1, "/a/b/var");
    STRCMP_EQUAL("var", regex[0].variable);
    STRCMP_EQUAL("a", regex[0].str_regex);
    CHECK(regex[0].regex);
    STRCMP_EQUAL("b", regex[0].replace);
    STRCMP_EQUAL("b", regex[0].replace_escaped);

    /* simple regex replace with variable (command "s") */
    WEE_CHECK_REGEX_SPLIT(0, 1, "s/a/b/var");
    STRCMP_EQUAL("var", regex[0].variable);
    STRCMP_EQUAL("a", regex[0].str_regex);
    CHECK(regex[0].regex);
    STRCMP_EQUAL("b", regex[0].replace);
    STRCMP_EQUAL("b", regex[0].replace_escaped);

    /* simple translate chars with variable (command "y") */
    WEE_CHECK_REGEX_SPLIT(0, 1, "y/${chars:a-h}/${chars:A-H}/var");
    STRCMP_EQUAL("var", regex[0].variable);
    STRCMP_EQUAL("${chars:a-h}", regex[0].str_regex);
    POINTERS_EQUAL(NULL, regex[0].regex);
    STRCMP_EQUAL("${chars:A-H}", regex[0].replace);
    POINTERS_EQUAL(NULL, regex[0].replace_escaped);

    /* 2 regex replace separated by 3 spaces, without variables, implicit command "s" */
    WEE_CHECK_REGEX_SPLIT(0, 2, "/abc/def/   /ghi/jkl/");
    POINTERS_EQUAL(NULL, regex[0].variable);
    STRCMP_EQUAL("abc", regex[0].str_regex);
    CHECK(regex[0].regex);
    STRCMP_EQUAL("def", regex[0].replace);
    STRCMP_EQUAL("def", regex[0].replace_escaped);
    POINTERS_EQUAL(NULL, regex[1].variable);
    STRCMP_EQUAL("ghi", regex[1].str_regex);
    CHECK(regex[1].regex);
    STRCMP_EQUAL("jkl", regex[1].replace);
    STRCMP_EQUAL("jkl", regex[1].replace_escaped);

    /* 2 regex replace separated by 3 spaces, without variables, command "s" */
    WEE_CHECK_REGEX_SPLIT(0, 2, "s/abc/def/   s/ghi/jkl/");
    POINTERS_EQUAL(NULL, regex[0].variable);
    STRCMP_EQUAL("abc", regex[0].str_regex);
    CHECK(regex[0].regex);
    STRCMP_EQUAL("def", regex[0].replace);
    STRCMP_EQUAL("def", regex[0].replace_escaped);
    POINTERS_EQUAL(NULL, regex[1].variable);
    STRCMP_EQUAL("ghi", regex[1].str_regex);
    CHECK(regex[1].regex);
    STRCMP_EQUAL("jkl", regex[1].replace);
    STRCMP_EQUAL("jkl", regex[1].replace_escaped);

    /* 2 translate chars separated by 3 spaces, without variables, command "y" */
    WEE_CHECK_REGEX_SPLIT(0, 2, "y/abc/ABC/   y/ghi/GHI/");
    POINTERS_EQUAL(NULL, regex[0].variable);
    STRCMP_EQUAL("abc", regex[0].str_regex);
    POINTERS_EQUAL(NULL, regex[0].regex);
    STRCMP_EQUAL("ABC", regex[0].replace);
    POINTERS_EQUAL(NULL, regex[0].replace_escaped);
    POINTERS_EQUAL(NULL, regex[1].variable);
    STRCMP_EQUAL("ghi", regex[1].str_regex);
    POINTERS_EQUAL(NULL, regex[1].regex);
    STRCMP_EQUAL("GHI", regex[1].replace);
    POINTERS_EQUAL(NULL, regex[1].replace_escaped);

    /* 3 regex replace with variables and escaped replace, implicit command "s" */
    WEE_CHECK_REGEX_SPLIT(0, 3,
                          "/abc/def/var1 /ghi/jkl/var2 /mno/pqr\\x20stu/var3");
    STRCMP_EQUAL("var1", regex[0].variable);
    STRCMP_EQUAL("abc", regex[0].str_regex);
    CHECK(regex[0].regex);
    STRCMP_EQUAL("def", regex[0].replace);
    STRCMP_EQUAL("def", regex[0].replace_escaped);
    STRCMP_EQUAL("var2", regex[1].variable);
    STRCMP_EQUAL("ghi", regex[1].str_regex);
    CHECK(regex[1].regex);
    STRCMP_EQUAL("jkl", regex[1].replace);
    STRCMP_EQUAL("jkl", regex[1].replace_escaped);
    STRCMP_EQUAL("var3", regex[2].variable);
    STRCMP_EQUAL("mno", regex[2].str_regex);
    CHECK(regex[2].regex);
    STRCMP_EQUAL("pqr\\x20stu", regex[2].replace);
    STRCMP_EQUAL("pqr stu", regex[2].replace_escaped);

    /* 3 regex replace with variables and escaped replace, command "s" */
    WEE_CHECK_REGEX_SPLIT(
        0, 3,
        "s/abc/def/var1 s/ghi/jkl/var2 s/mno/pqr\\x20stu/var3");
    STRCMP_EQUAL("var1", regex[0].variable);
    STRCMP_EQUAL("abc", regex[0].str_regex);
    CHECK(regex[0].regex);
    STRCMP_EQUAL("def", regex[0].replace);
    STRCMP_EQUAL("def", regex[0].replace_escaped);
    STRCMP_EQUAL("var2", regex[1].variable);
    STRCMP_EQUAL("ghi", regex[1].str_regex);
    CHECK(regex[1].regex);
    STRCMP_EQUAL("jkl", regex[1].replace);
    STRCMP_EQUAL("jkl", regex[1].replace_escaped);
    STRCMP_EQUAL("var3", regex[2].variable);
    STRCMP_EQUAL("mno", regex[2].str_regex);
    CHECK(regex[2].regex);
    STRCMP_EQUAL("pqr\\x20stu", regex[2].replace);
    STRCMP_EQUAL("pqr stu", regex[2].replace_escaped);

    /* 3 translate chars with variables, command "y" */
    WEE_CHECK_REGEX_SPLIT(0, 3, "y/abc/ABC/var1 y/ghi/GHI/var2 y/mno/MNO/var3");
    STRCMP_EQUAL("var1", regex[0].variable);
    STRCMP_EQUAL("abc", regex[0].str_regex);
    POINTERS_EQUAL(NULL, regex[0].regex);
    STRCMP_EQUAL("ABC", regex[0].replace);
    POINTERS_EQUAL(NULL, regex[0].replace_escaped);
    STRCMP_EQUAL("var2", regex[1].variable);
    STRCMP_EQUAL("ghi", regex[1].str_regex);
    POINTERS_EQUAL(NULL, regex[1].regex);
    STRCMP_EQUAL("GHI", regex[1].replace);
    POINTERS_EQUAL(NULL, regex[1].replace_escaped);
    STRCMP_EQUAL("var3", regex[2].variable);
    STRCMP_EQUAL("mno", regex[2].str_regex);
    POINTERS_EQUAL(NULL, regex[2].regex);
    STRCMP_EQUAL("MNO", regex[2].replace);
    POINTERS_EQUAL(NULL, regex[2].replace_escaped);

    /* mixed regex replace and translate chars */
    WEE_CHECK_REGEX_SPLIT(0, 2,
                          "s/abc/defghi/var1 y/${chars:x-z}/${chars:X-Z}/var2");
    STRCMP_EQUAL("var1", regex[0].variable);
    STRCMP_EQUAL("abc", regex[0].str_regex);
    CHECK(regex[0].regex);
    STRCMP_EQUAL("defghi", regex[0].replace);
    STRCMP_EQUAL("defghi", regex[0].replace_escaped);
    STRCMP_EQUAL("var2", regex[1].variable);
    STRCMP_EQUAL("${chars:x-z}", regex[1].str_regex);
    POINTERS_EQUAL(NULL, regex[1].regex);
    STRCMP_EQUAL("${chars:X-Z}", regex[1].replace);
    POINTERS_EQUAL(NULL, regex[1].replace_escaped);

    trigger_regex_free (&regex_count, &regex);
}

/*
 * Tests functions:
 *   trigger_split_command
 */

TEST(Trigger, SplitCommand)
{
    int commands_count;
    char **commands;

    commands_count = 0;
    commands = NULL;

    /* no commands_count / commands */
    trigger_split_command (NULL, NULL, NULL);
    LONGS_EQUAL(0, commands_count);
    POINTERS_EQUAL(NULL, commands);
    trigger_split_command (NULL, &commands_count, NULL);
    LONGS_EQUAL(0, commands_count);
    POINTERS_EQUAL(NULL, commands);
    trigger_split_command (NULL, NULL, &commands);
    LONGS_EQUAL(0, commands_count);
    POINTERS_EQUAL(NULL, commands);

    /* NULL/empty command */
    trigger_split_command (NULL, &commands_count, &commands);
    LONGS_EQUAL(0, commands_count);
    POINTERS_EQUAL(NULL, commands);
    trigger_split_command (NULL, &commands_count, &commands);
    LONGS_EQUAL(0, commands_count);
    POINTERS_EQUAL(NULL, commands);

    /* one command */
    trigger_split_command ("/test", &commands_count, &commands);
    LONGS_EQUAL(1, commands_count);
    CHECK(commands);
    STRCMP_EQUAL("/test", commands[0]);

    /* one command with an escaped semicolon */
    trigger_split_command ("/test arg\\;test", &commands_count, &commands);
    LONGS_EQUAL(1, commands_count);
    CHECK(commands);
    STRCMP_EQUAL("/test arg;test", commands[0]);

    /* two commands */
    trigger_split_command ("/test1;/test2", &commands_count, &commands);
    LONGS_EQUAL(2, commands_count);
    CHECK(commands);
    STRCMP_EQUAL("/test1", commands[0]);
    STRCMP_EQUAL("/test2", commands[1]);
}

/*
 * Tests functions:
 *   trigger_name_valid
 */

TEST(Trigger, NameValid)
{
    LONGS_EQUAL(0, trigger_name_valid (NULL));
    LONGS_EQUAL(0, trigger_name_valid (""));
    LONGS_EQUAL(0, trigger_name_valid ("-"));
    LONGS_EQUAL(0, trigger_name_valid ("-abc"));
    LONGS_EQUAL(0, trigger_name_valid ("abc def"));
    LONGS_EQUAL(0, trigger_name_valid (" abc"));
    LONGS_EQUAL(0, trigger_name_valid ("abc.def"));
    LONGS_EQUAL(0, trigger_name_valid (".abc"));

    LONGS_EQUAL(1, trigger_name_valid ("abc-def"));
    LONGS_EQUAL(1, trigger_name_valid ("abc-def-"));
    LONGS_EQUAL(1, trigger_name_valid ("abc/def/"));
    LONGS_EQUAL(1, trigger_name_valid ("abcdef"));
}

/*
 * Tests functions:
 *   trigger_search
 *   trigger_search_with_option
 *   trigger_alloc
 *   trigger_find_pos
 *   trigger_add
 *   trigger_new_with_options
 *   trigger_new
 *   trigger_free
 */

TEST(Trigger, New)
{
    struct t_trigger *trigger;
    int hook_type, enabled;

    /* invalid name */
    POINTERS_EQUAL(NULL, trigger_new ("-test", "on", "signal", "test", "", "",
                                      "/print test", "ok", "none"));

    /* invalid hook type */
    POINTERS_EQUAL(NULL, trigger_new ("test", "on", "abc", "test", "", "",
                                      "/print test", "ok", "none"));

    /* invalid return code */
    POINTERS_EQUAL(NULL, trigger_new ("test", "on", "signal", "test", "", "",
                                      "/print test", "abc", "none"));

    /* invalid post action */
    POINTERS_EQUAL(NULL, trigger_new ("test", "on", "signal", "test", "", "",
                                      "/print test", "ok", "abc"));

    /* name already used */
    trigger = trigger_new ("test", "on", "signal", "test", "", "", "", "", "");
    POINTERS_EQUAL(NULL, trigger_new ("test", "on", "signal", "test", "", "",
                                      "", "", ""));
    trigger_free (trigger);

    /* test a trigger of each type */
    for (hook_type = 0; hook_type < TRIGGER_NUM_HOOK_TYPES; hook_type++)
    {
        /* test enabled/disabled trigger */
        for (enabled = 0; enabled < 2; enabled++)
        {
            printf ("Creating %s trigger with hook \"%s\"\n",
                    (enabled) ? "enabled" : "disabled",
                    trigger_hook_type_string[hook_type]);
            trigger = trigger_new (
                "test",
                (enabled) ? "on" : "off",
                trigger_hook_type_string[hook_type],
                (hook_type == TRIGGER_HOOK_TIMER) ? "60000" : "args",
                "conditions",
                "/abc/def",
                "/print test",
                "ok",
                "none");
            CHECK(trigger);
            STRCMP_EQUAL("test", trigger->name);
            LONGS_EQUAL(
                enabled,
                CONFIG_BOOLEAN(trigger->options[TRIGGER_OPTION_ENABLED]));
            LONGS_EQUAL(
                hook_type,
                CONFIG_INTEGER(trigger->options[TRIGGER_OPTION_HOOK]));
            STRCMP_EQUAL(
                (hook_type == TRIGGER_HOOK_TIMER) ? "60000" : "args",
                CONFIG_STRING(trigger->options[TRIGGER_OPTION_ARGUMENTS]));
            STRCMP_EQUAL(
                "conditions",
                CONFIG_STRING(trigger->options[TRIGGER_OPTION_CONDITIONS]));
            STRCMP_EQUAL(
                "/abc/def",
                CONFIG_STRING(trigger->options[TRIGGER_OPTION_REGEX]));
            STRCMP_EQUAL(
                "/print test",
                CONFIG_STRING(trigger->options[TRIGGER_OPTION_COMMAND]));
            LONGS_EQUAL(
                TRIGGER_RC_OK,
                CONFIG_INTEGER(trigger->options[TRIGGER_OPTION_RETURN_CODE]));
            LONGS_EQUAL(
                TRIGGER_POST_ACTION_NONE,
                CONFIG_INTEGER(trigger->options[TRIGGER_OPTION_POST_ACTION]));
            if (enabled)
            {
                LONGS_EQUAL(1, trigger->hooks_count);
                CHECK(trigger->hooks);
                CHECK(trigger->hooks[0]);
            }
            else
            {
                LONGS_EQUAL(0, trigger->hooks_count);
                POINTERS_EQUAL(NULL, trigger->hooks);
            }
            LONGS_EQUAL(0, trigger->hook_count_cb);
            LONGS_EQUAL(0, trigger->hook_count_cmd);
            LONGS_EQUAL(0, trigger->hook_running);
            if (enabled && (hook_type == TRIGGER_HOOK_PRINT))
            {
                STRCMP_EQUAL("args", trigger->hook_print_buffers);
            }
            else
            {
                POINTERS_EQUAL(NULL, trigger->hook_print_buffers);
            }
            LONGS_EQUAL(1, trigger->regex_count);
            CHECK(trigger->regex);
            POINTERS_EQUAL(NULL, trigger->regex[0].variable);
            STRCMP_EQUAL("abc", trigger->regex[0].str_regex);
            CHECK(trigger->regex[0].regex);
            STRCMP_EQUAL("def", trigger->regex[0].replace);
            STRCMP_EQUAL("def", trigger->regex[0].replace_escaped);
            LONGS_EQUAL(1, trigger->commands_count);
            CHECK(trigger->commands);
            STRCMP_EQUAL("/print test", trigger->commands[0]);
            POINTERS_EQUAL(trigger, trigger_search ("test"));
            POINTERS_EQUAL(trigger,
                           trigger_search_with_option (
                               trigger->options[TRIGGER_OPTION_HOOK]));
            trigger_free (trigger);
        }
    }

    /* trigger with multiple regex */
    trigger = trigger_new (
            "test",
            "on",
            "signal",
            "args",
            "conditions",
            "/abc/def/var1 /ghi/jkl/var2 /mno/pqr\\x20stu/var3",
            "/print test",
            "ok",
            "none");
    CHECK(trigger);
    LONGS_EQUAL(3, trigger->regex_count);
    CHECK(trigger->regex);
    /* regex 1 */
    STRCMP_EQUAL("var1", trigger->regex[0].variable);
    STRCMP_EQUAL("abc", trigger->regex[0].str_regex);
    CHECK(trigger->regex[0].regex);
    STRCMP_EQUAL("def", trigger->regex[0].replace);
    STRCMP_EQUAL("def", trigger->regex[0].replace_escaped);
    /* regex 2 */
    STRCMP_EQUAL("var2", trigger->regex[1].variable);
    STRCMP_EQUAL("ghi", trigger->regex[1].str_regex);
    CHECK(trigger->regex[0].regex);
    STRCMP_EQUAL("jkl", trigger->regex[1].replace);
    STRCMP_EQUAL("jkl", trigger->regex[1].replace_escaped);
    /* regex 3 */
    STRCMP_EQUAL("var3", trigger->regex[2].variable);
    STRCMP_EQUAL("mno", trigger->regex[2].str_regex);
    CHECK(trigger->regex[2].regex);
    STRCMP_EQUAL("pqr\\x20stu", trigger->regex[2].replace);
    STRCMP_EQUAL("pqr stu", trigger->regex[2].replace_escaped);
    /* free trigger */
    trigger_free (trigger);

    /* search trigger */
    POINTERS_EQUAL(NULL, trigger_search (NULL));
    POINTERS_EQUAL(NULL, trigger_search (""));
    POINTERS_EQUAL(NULL, trigger_search ("abc"));
    POINTERS_EQUAL(NULL, trigger_search_with_option (NULL));
    POINTERS_EQUAL(NULL, trigger_search_with_option (config_look_day_change));

    /* invalid free */
    trigger_free (NULL);
}

/*
 * Tests functions:
 *   trigger_create_default
 */

TEST(Trigger, CreateDefault)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_rename
 */

TEST(Trigger, Rename)
{
    struct t_trigger *trigger;

    trigger = trigger_new ("test", "on", "signal", "test", "", "", "", "", "");

    LONGS_EQUAL(0, trigger_rename (NULL, NULL));
    LONGS_EQUAL(0, trigger_rename (NULL, ""));
    LONGS_EQUAL(0, trigger_rename (trigger, NULL));
    LONGS_EQUAL(0, trigger_rename (trigger, ""));
    LONGS_EQUAL(0, trigger_rename (trigger, "-test2"));
    LONGS_EQUAL(0, trigger_rename (trigger, "test"));

    LONGS_EQUAL(1, trigger_rename (trigger, "test2"));
    POINTERS_EQUAL(trigger, trigger_search ("test2"));

    trigger_free (trigger);
}

/*
 * Tests functions:
 *   trigger_copy
 */

TEST(Trigger, Copy)
{
    struct t_trigger *trigger, *trigger2;

    trigger = trigger_new ("test", "on", "signal", "test", "", "", "", "", "");

    LONGS_EQUAL(0, trigger_copy (NULL, NULL));
    LONGS_EQUAL(0, trigger_copy (NULL, ""));
    LONGS_EQUAL(0, trigger_copy (trigger, NULL));
    LONGS_EQUAL(0, trigger_copy (trigger, ""));
    LONGS_EQUAL(0, trigger_copy (trigger, "-test2"));
    LONGS_EQUAL(0, trigger_copy (trigger, "test"));

    trigger2 = trigger_copy (trigger, "test2");
    CHECK(trigger2);
    CHECK(trigger != trigger2);
    STRCMP_EQUAL("test", trigger->name);
    STRCMP_EQUAL("test2", trigger2->name);

    trigger_free (trigger);
    trigger_free (trigger2);
}

/*
 * Tests functions:
 *   trigger_free_all
 */

TEST(Trigger, FreeAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_print_log
 */

TEST(Trigger, PrintLog)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_debug_dump_cb
 */

TEST(Trigger, DebugDumpCb)
{
    /* TODO: write tests */
}
