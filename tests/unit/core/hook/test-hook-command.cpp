/*
 * test-hook-command.cpp - test hook command functions
 *
 * Copyright (C) 2023-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#include "tests/tests.h"

extern "C"
{
#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#endif
#include "src/core/weechat.h"
#include "src/core/hook/hook-command.h"
#include "src/plugins/plugin.h"

extern char *hook_command_remove_raw_markers (const char *string);
}

TEST_GROUP(HookCommand)
{
};

/*
 * Tests functions:
 *   hook_command_get_description
 */

TEST(HookCommand, GetDescription)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_command_search
 */

TEST(HookCommand, Search)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_command_build_completion
 */

TEST(HookCommand, BuildCompletion)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_command_remove_raw_markers
 */

TEST(HookCommand, RemoveRawMarkers)
{
    char *str;

    WEE_TEST_STR(NULL, hook_command_remove_raw_markers (NULL));
    WEE_TEST_STR("", hook_command_remove_raw_markers (""));

    WEE_TEST_STR("test", hook_command_remove_raw_markers ("test"));
    WEE_TEST_STR("[test] raw[x", hook_command_remove_raw_markers ("[test] raw[x"));

    WEE_TEST_STR("", hook_command_remove_raw_markers ("raw[]"));
    WEE_TEST_STR("x", hook_command_remove_raw_markers ("raw[x]"));
    WEE_TEST_STR("x test", hook_command_remove_raw_markers ("raw[x] test"));
    WEE_TEST_STR("[test] ", hook_command_remove_raw_markers ("[test] raw[]"));
    WEE_TEST_STR("[test] x", hook_command_remove_raw_markers ("[test] raw[x]"));
    WEE_TEST_STR("[test] x y", hook_command_remove_raw_markers ("[test] raw[x] raw[y]"));
}

/*
 * Tests functions:
 *   hook_command_arraylist_arg_desc_free
 *   hook_command_format_args_description
 */

TEST(HookCommand, FormatArgsDescription)
{
    char *str;

    WEE_TEST_STR(NULL, hook_command_format_args_description (NULL));
    WEE_TEST_STR("", hook_command_format_args_description (""));

    WEE_TEST_STR("test format args desc",
                 hook_command_format_args_description (
                     "test format args desc"));
    WEE_TEST_STR("raw[list]: list all bars",
                 hook_command_format_args_description (
                     "raw[list]: list all bars"));

    WEE_TEST_STR("        list: list all bars\n"
                 "sub-command2: some other sub-command",
                 hook_command_format_args_description (
                     WEECHAT_HOOK_COMMAND_STR_FORMATTED "\n"
                     "raw[list]: list all bars\n"
                     "raw[sub-command2]: some other sub-command"));

    WEE_TEST_STR("    list: list all things\n"
                 "listfull: list all things (verbose)\n"
                 "    name: name of the thing\n"
                 "    type: the type:\n"
                 "          type1: first type\n"
                 "          type2: second type\n"
                 "\n"
                 "This is another line: test.",
                 hook_command_format_args_description (
                     WEECHAT_HOOK_COMMAND_STR_FORMATTED "\n"
                     "raw[list]: list all things\n"
                     "raw[listfull]: list all things (verbose)\n"
                     "name: name of the thing\n"
                     "type: the type:\n"
                     "> raw[type1]: first type\n"
                     "> raw[type2]: second type\n"
                     "\n"
                     "This is another line: test."));
}

/*
 * Tests functions:
 *   hook_command
 */

TEST(HookCommand, Command)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_command_exec
 */

TEST(HookCommand, CommandExec)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_command_similar_get_relevance
 */

TEST(HookCommand, CommandSimilarGetRelevance)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_command_similar_cmp_cb
 */

TEST(HookCommand, CommandSimilarCmpCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_command_similar_free_cb
 */

TEST(HookCommand, CommandSimilarFreeCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_command_build_list_similar_commands
 */

TEST(HookCommand, CommandBuildListSimilarCommands)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_command_display_error_unknown
 */

TEST(HookCommand, CommandDisplayErrorUnknown)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_command_free_data
 */

TEST(HookCommand, CommandFreeData)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_command_add_to_infolist
 */

TEST(HookCommand, CommandAddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_command_print_log
 */

TEST(HookCommand, CommandPrintLog)
{
    /* TODO: write tests */
}
