/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test trigger command functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include "src/core/core-string.h"
#include "src/plugins/trigger/trigger-command.h"

extern char *trigger_command_escape_argument (const char *argument);
}

#define WEE_CHECK_ESCAPE_ARGUMENT(__result, __argument)                 \
    WEE_TEST_STR(__result, trigger_command_escape_argument (__argument));

#define WEE_CHECK_ESCAPE_ARGUMENT_SPLIT(__argument)                     \
    escaped = trigger_command_escape_argument (__argument);             \
    CHECK(escaped);                                                     \
    snprintf (command, sizeof (command), "\"%s\"", escaped);            \
    free (escaped);                                                     \
    argv = string_split_shell (command, &argc);                         \
    LONGS_EQUAL(1, argc);                                               \
    CHECK(argv);                                                        \
    STRCMP_EQUAL(__argument, argv[0]);                                  \
    string_free_split (argv);

TEST_GROUP(TriggerCommand)
{
};

/*
 * Test functions:
 *   trigger_command_escape_argument
 */

TEST(TriggerCommand, EscapeArgument)
{
    char *str, *escaped, command[1024], **argv;
    int argc;

    WEE_CHECK_ESCAPE_ARGUMENT(NULL, NULL);
    WEE_CHECK_ESCAPE_ARGUMENT("", "");
    WEE_CHECK_ESCAPE_ARGUMENT("test", "test");
    WEE_CHECK_ESCAPE_ARGUMENT("\\\"test\\\"", "\"test\"");
    WEE_CHECK_ESCAPE_ARGUMENT("\\\\", "\\");
    WEE_CHECK_ESCAPE_ARGUMENT("\\\\n", "\\n");
    WEE_CHECK_ESCAPE_ARGUMENT("\\\\\\\"", "\\\"");
    WEE_CHECK_ESCAPE_ARGUMENT("/\\\\\\\\\\\\\\\\/\\\\\\\\\\\\\\\\/tg_argv_eol1",
                              "/\\\\\\\\/\\\\\\\\/tg_argv_eol1");

    /* escaped argument must be split back to the original argument */
    WEE_CHECK_ESCAPE_ARGUMENT_SPLIT("");
    WEE_CHECK_ESCAPE_ARGUMENT_SPLIT("test");
    WEE_CHECK_ESCAPE_ARGUMENT_SPLIT("\"test\"");
    WEE_CHECK_ESCAPE_ARGUMENT_SPLIT("\\");
    WEE_CHECK_ESCAPE_ARGUMENT_SPLIT("\\n");
    WEE_CHECK_ESCAPE_ARGUMENT_SPLIT("\\\"");
    WEE_CHECK_ESCAPE_ARGUMENT_SPLIT("ab\\\\\\c");
    WEE_CHECK_ESCAPE_ARGUMENT_SPLIT("/\\\\\\\\/\\\\\\\\/tg_argv_eol1");
    WEE_CHECK_ESCAPE_ARGUMENT_SPLIT("${tg_message} \"test\" \\\" \\\\\"");
}
