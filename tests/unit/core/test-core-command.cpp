/*
 * test-core-command.cpp - test command functions
 *
 * Copyright (C) 2022-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include "tests/tests-record.h"

extern "C"
{
#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#endif
#include "src/core/weechat.h"
#include "src/core/core-command.h"
#include "src/core/core-input.h"
#include "src/core/core-string.h"
#include "src/gui/gui-buffer.h"
}

#define WEE_CMD_BUFFER(__buffer_name, __command)                        \
    command_record (__buffer, __command);

#define WEE_CMD_CORE(__command)                                         \
    command_record ("core.weechat", __command);

#define WEE_CHECK_MSG_BUFFER(__buffer_name, __prefix, __message)        \
    if (!record_search (__buffer_name, __prefix, __message, NULL))      \
    {                                                                   \
        char **msg = command_build_error (__buffer_name, __prefix,      \
                                          __message);                   \
        record_dump (msg);                                              \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

#define WEE_CHECK_MSG_CORE(__prefix, __message)                         \
    WEE_CHECK_MSG_BUFFER("core.weechat", __prefix, __message);


TEST_GROUP(CoreCommand)
{
    void command_record (const char *buffer_name, const char *command)
    {
        struct t_gui_buffer *buffer;

        buffer = gui_buffer_search_by_full_name (buffer_name);
        if (!buffer)
        {
            FAIL("Buffer not found");
        }
        record_start ();
        input_data (buffer, command, NULL, 0, 0);
        record_stop ();
    }

    char **command_build_error (const char *buffer_name, const char *prefix,
                                const char *message)
    {
        char **msg;

        msg = string_dyn_alloc (1024);
        string_dyn_concat (msg, "Message not displayed on buffer ", -1);
        string_dyn_concat (msg, buffer_name, -1);
        string_dyn_concat (msg, ": prefix=\"", -1);
        string_dyn_concat (msg, prefix, -1);
        string_dyn_concat (msg, "\", message=\"", -1);
        string_dyn_concat (msg, message, -1);
        string_dyn_concat (msg, "\"\n", -1);
        string_dyn_concat (msg, "All messages displayed:\n", -1);
        return msg;
    }
};

/*
 * Tests functions:
 *   command_away
 */

TEST(CoreCommand, Away)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_bar
 */

TEST(CoreCommand, Bar)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_buffer
 */

TEST(CoreCommand, Buffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_color
 */

TEST(CoreCommand, Color)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_command
 */

TEST(CoreCommand, Command)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_cursor
 */

TEST(CoreCommand, Cursor)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_debug
 */

TEST(CoreCommand, Debug)
{
    const char *command_debug_unicode =
        "/debug unicode "
        "\u00E9"  /* é */
        "\u26C4"  /* ⛄ (snowman without snow) */
        "";

    /* test command "/debug list" */
    /* TODO: write tests */

    /* test command "/debug buffer" */
    /* TODO: write tests */

    /* test command "/debug certs" */
    /* TODO: write tests */

    /* test command "/debug color" */
    /* TODO: write tests */

    /* test command "/debug cursor" */
    /* TODO: write tests */

    /* test command "/debug dirs" */
    /* TODO: write tests */

    /* test command "/debug dump" */
    /* TODO: write tests */

    /* test command "/debug hdata" */
    /* TODO: write tests */

    /* test command "/debug hooks" */
    /* TODO: write tests */

    /* test command "/debug infolists" */
    /* TODO: write tests */

    /* test command "/debug libs" */
    /* TODO: write tests */

    /* test command "/debug memory" */
    /* TODO: write tests */

    /* test command "/debug mouse" */
    /* TODO: write tests */

    /* test command "/debug set" */
    LONGS_EQUAL(0, weechat_debug_core);
    WEE_CMD_CORE("/debug set core 1");
    LONGS_EQUAL(1, weechat_debug_core);
    WEE_CMD_CORE("/debug set core 2");
    LONGS_EQUAL(2, weechat_debug_core);
    WEE_CMD_CORE("/debug set core 0");
    LONGS_EQUAL(0, weechat_debug_core);

    /* test command "/debug tags" */
    /* TODO: write tests */

    /* test command "/debug term" */
    /* TODO: write tests */

    /* test command "/debug time" */
    /* TODO: write tests */

    /* test command "/debug unicode" */
    WEE_CMD_CORE(command_debug_unicode);
    WEE_CHECK_MSG_CORE("", "  \"\u00E9\u26C4\": 5 / 2, 2 / 3, 3, 3");
    WEE_CHECK_MSG_CORE("", "  \"\u00E9\" (U+00E9, 233, 0xC3 0xA9): 2 / 1, 1 / 1, 1, 1, 1");
    WEE_CHECK_MSG_CORE("", "  \"\u26C4\" (U+26C4, 9924, 0xE2 0x9B 0x84): 3 / 1, 1 / 2, 2, 2, 2");

    /* test command "/debug windows" */
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_eval
 */

TEST(CoreCommand, Eval)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_filter
 */

TEST(CoreCommand, Filter)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_help
 */

TEST(CoreCommand, Help)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_history
 */

TEST(CoreCommand, History)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_input
 */

TEST(CoreCommand, Input)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_item
 */

TEST(CoreCommand, Item)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_key
 */

TEST(CoreCommand, Key)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_layout
 */

TEST(CoreCommand, Layout)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_mouse
 */

TEST(CoreCommand, Mouse)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_mute
 */

TEST(CoreCommand, Mute)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_plugin
 */

TEST(CoreCommand, Plugin)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_print
 */

TEST(CoreCommand, Print)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_proxy
 */

TEST(CoreCommand, Proxy)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_quit
 */

TEST(CoreCommand, Quit)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_reload
 */

TEST(CoreCommand, Reload)
{
    WEE_CMD_CORE("/save");
    WEE_CMD_CORE("/reload");
    WEE_CHECK_MSG_CORE("", "Options reloaded from sec.conf");
    WEE_CHECK_MSG_CORE("", "Options reloaded from weechat.conf");
    WEE_CHECK_MSG_CORE("", "Options reloaded from plugins.conf");
    WEE_CHECK_MSG_CORE("", "Options reloaded from charset.conf");
}

/*
 * Tests functions:
 *   command_repeat
 */

TEST(CoreCommand, Repeat)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_reset
 */

TEST(CoreCommand, Reset)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_save
 */

TEST(CoreCommand, Save)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_secure
 */

TEST(CoreCommand, Secure)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_set
 */

TEST(CoreCommand, Set)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_toggle
 */

TEST(CoreCommand, Toggle)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_unset
 */

TEST(CoreCommand, Unset)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_upgrade
 */

TEST(CoreCommand, Upgrade)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_uptime
 */

TEST(CoreCommand, Uptime)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_version
 */

TEST(CoreCommand, Version)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_wait
 */

TEST(CoreCommand, Wait)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   command_window
 */

TEST(CoreCommand, Window)
{
    /* TODO: write tests */
}
