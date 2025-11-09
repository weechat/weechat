/*
 * SPDX-FileCopyrightText: 2022-2025 Sébastien Helleu <flashcode@flashtux.org>
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

/* Test command functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"
#include "tests-record.h"

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
#include "src/gui/gui-chat.h"
#include "src/plugins/plugin.h"
}

#define WEE_CMD_BUFFER(__buffer_name, __command)                        \
    command_record (__buffer, __command);

#define WEE_CMD_CORE(__command)                                         \
    LONGS_EQUAL(WEECHAT_RC_OK,                                          \
                command_record ("core.weechat", __command));

#define WEE_CMD_CORE_MIN_ARGS(__command, __error_command)               \
    LONGS_EQUAL(WEECHAT_RC_ERROR,                                       \
                command_record ("core.weechat", __command));            \
    command_check_min_args (__command, __error_command);

#define WEE_CMD_CORE_ERROR_GENERIC(__command)                           \
    LONGS_EQUAL(WEECHAT_RC_ERROR,                                       \
                command_record ("core.weechat", __command));            \
    command_check_error_generic (__command);

#define WEE_CMD_CORE_ERROR_MSG(__command, __error_message)              \
    LONGS_EQUAL(WEECHAT_RC_ERROR,                                       \
                command_record ("core.weechat", __command));            \
    WEE_CHECK_MSG_BUFFER(                                               \
        "core.weechat",                                                 \
        GUI_CHAT_PREFIX_ERROR_DEFAULT,                                  \
        __error_message);

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
    int command_record (const char *buffer_name, const char *command)
    {
        struct t_gui_buffer *buffer;
        int rc;

        buffer = gui_buffer_search_by_full_name (buffer_name);
        if (!buffer)
        {
            FAIL("Buffer not found");
        }
        record_start ();
        rc = input_data (buffer, command, NULL, 0, 0);
        record_stop ();
        return rc;
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

    void command_check_min_args (const char *command, const char *error_command)
    {
        char *pos, *command_name, error[1024];

        command_name = strdup (command + 1);
        pos = strchr (command_name, ' ');
        if (pos)
            pos[0] = '\0';
        snprintf (error, sizeof (error),
                  "Too few arguments for command \"%s\" (help on command: /help %s)",
                  error_command,
                  command_name);
        free (command_name);
        WEE_CHECK_MSG_CORE(GUI_CHAT_PREFIX_ERROR_DEFAULT, error);
    }

    void command_check_error_generic (const char *command)
    {
        char *pos, *command_name, error[1024];

        command_name = strdup (command + 1);
        pos = strchr (command_name, ' ');
        if (pos)
            pos[0] = '\0';
        snprintf (error, sizeof (error),
                  "Error with command \"%s\" (help on command: /help %s)",
                  command,
                  command_name);
        free (command_name);
        WEE_CHECK_MSG_CORE(GUI_CHAT_PREFIX_ERROR_DEFAULT, error);
    }
};

/*
 * Tests functions:
 *   command_allbuf
 */

TEST(CoreCommand, Allbuf)
{
    WEE_CMD_CORE_MIN_ARGS("/allbuf", "/allbuf");

    WEE_CMD_CORE("/allbuf /print test allbuf");
    WEE_CHECK_MSG_CORE("", "test allbuf");
}

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
    WEE_CMD_CORE_ERROR_GENERIC("/bar xxx");

    /* /bar, /bar list, /bar listfull, /bar listitems */
    WEE_CMD_CORE("/bar");
    WEE_CHECK_MSG_CORE("", "List of bars:");
    WEE_CMD_CORE("/bar list");
    WEE_CHECK_MSG_CORE("", "List of bars:");
    WEE_CMD_CORE("/bar listfull");
    WEE_CHECK_MSG_CORE("", "List of bars:");
    WEE_CMD_CORE("/bar listitems");
    WEE_CHECK_MSG_CORE("", "List of bar items:");

    /* /bar add, /bar del */
    WEE_CMD_CORE_MIN_ARGS("/bar add", "/bar add");
    WEE_CMD_CORE_MIN_ARGS("/bar del", "/bar del");
    WEE_CMD_CORE_MIN_ARGS("/bar add test", "/bar add");
    WEE_CMD_CORE_MIN_ARGS("/bar add test root", "/bar add");
    WEE_CMD_CORE_MIN_ARGS("/bar add test root top", "/bar add");
    WEE_CMD_CORE_MIN_ARGS("/bar add test root top 1", "/bar add");
    WEE_CMD_CORE_MIN_ARGS("/bar add test root top 1 0", "/bar add");
    WEE_CMD_CORE_ERROR_MSG("/bar add test type1 top 1 0 item1",
                           "Invalid type \"type1\" for bar \"test\"");
    WEE_CMD_CORE_ERROR_MSG("/bar add test root top_top 1 0 item1",
                           "Invalid position \"top_top\" for bar \"test\"");
    WEE_CMD_CORE_ERROR_MSG("/bar add test root top size1 0 item1",
                           "Invalid size \"size1\" for bar \"test\"");
    WEE_CMD_CORE("/bar add test root top 1 0 item1");
    WEE_CHECK_MSG_CORE("", "Bar \"test\" created");
    WEE_CMD_CORE_ERROR_MSG("/bar add test root top 1 0 item1",
                           "Bar \"test\" already exists");
    WEE_CMD_CORE("/bar addreplace test root top 1 0 item2");
    WEE_CHECK_MSG_CORE("", "Bar \"test\" updated");
    WEE_CMD_CORE("/bar addreplace test root,1 top 1 0 item3");
    WEE_CHECK_MSG_CORE("", "Bar \"test\" updated");
    WEE_CMD_CORE("/bar del test");
    WEE_CHECK_MSG_CORE("", "Bar \"test\" deleted");

    /* /bar default */
    WEE_CMD_CORE("/bar default");
    WEE_CMD_CORE("/bar default input title status nicklist");

    /* /bar rename */
    WEE_CMD_CORE_MIN_ARGS("/bar rename", "/bar rename");
    WEE_CMD_CORE_MIN_ARGS("/bar rename status", "/bar rename");
    WEE_CMD_CORE_ERROR_MSG("/bar rename xxx test", "Bar \"xxx\" not found");
    WEE_CMD_CORE_ERROR_MSG("/bar rename status nicklist",
                           "Bar \"nicklist\" already exists for \"bar rename\" command");
    WEE_CMD_CORE("/bar rename status status2");
    WEE_CMD_CORE("/bar rename status2 status");

    /* /bar set */
    WEE_CMD_CORE_MIN_ARGS("/bar set", "/bar set");
    WEE_CMD_CORE_MIN_ARGS("/bar set status", "/bar set");
    WEE_CMD_CORE_MIN_ARGS("/bar set status position", "/bar set");
    WEE_CMD_CORE_ERROR_MSG("/bar set xxx position top", "Bar \"xxx\" not found");
    WEE_CMD_CORE_ERROR_MSG("/bar set status xxx top",
                           "Unable to set option \"xxx\" for bar \"status\"");
    WEE_CMD_CORE("/bar set status position top");
    WEE_CMD_CORE("/bar set status position bottom");

    /* /bar hide, /bar show, /bar toggle */
    WEE_CMD_CORE_MIN_ARGS("/bar hide", "/bar hide");
    WEE_CMD_CORE_MIN_ARGS("/bar show", "/bar show");
    WEE_CMD_CORE_MIN_ARGS("/bar toggle", "/bar toggle");
    WEE_CMD_CORE_ERROR_MSG("/bar hide xxx", "Bar \"xxx\" not found");
    WEE_CMD_CORE_ERROR_MSG("/bar show xxx", "Bar \"xxx\" not found");
    WEE_CMD_CORE_ERROR_MSG("/bar toggle xxx", "Bar \"xxx\" not found");
    WEE_CMD_CORE("/bar toggle status");
    WEE_CMD_CORE("/bar toggle status");
    WEE_CMD_CORE("/bar hide status");
    WEE_CMD_CORE("/bar hide status");
    WEE_CMD_CORE("/bar show status");
    WEE_CMD_CORE("/bar show status");

    /* /bar scroll */
    WEE_CMD_CORE_MIN_ARGS("/bar scroll", "/bar scroll");
    WEE_CMD_CORE_MIN_ARGS("/bar scroll status", "/bar scroll");
    WEE_CMD_CORE_MIN_ARGS("/bar scroll status *", "/bar scroll");
    WEE_CMD_CORE_ERROR_MSG("/bar scroll xxx * +10", "Bar \"xxx\" not found");
    WEE_CMD_CORE_ERROR_MSG("/bar scroll status 999999 +10", "Window not found for \"bar\" command");
    WEE_CMD_CORE_ERROR_MSG("/bar scroll status * +xxx", "Unable to scroll bar \"status\"");
    WEE_CMD_CORE("/bar scroll status * +10");
    WEE_CMD_CORE("/bar scroll status * -10");
    WEE_CMD_CORE("/bar scroll status 1 +10");
    WEE_CMD_CORE("/bar scroll status 1 -10");
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
 *   command_pipe
 */

TEST(CoreCommand, Pipe)
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
