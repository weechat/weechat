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
#include <string.h>
#include "src/core/weechat.h"
#include "src/core/core-command.h"
#include "src/core/core-debug.h"
#include "src/core/core-input.h"
#include "src/core/core-string.h"
#include "src/core/core-url.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-chat.h"
#include "src/gui/gui-color.h"
#include "src/gui/gui-cursor.h"
#include "src/gui/gui-filter.h"
#include "src/gui/gui-hotlist.h"
#include "src/gui/gui-key.h"
#include "src/gui/gui-mouse.h"
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

#define WEE_CHECK_MSG_REGEX_BUFFER(__buffer_name, __regex)              \
    if (!record_search_msg_regex (__buffer_name, __regex))              \
    {                                                                   \
        char **msg = command_build_error (__buffer_name, NULL,          \
                                          __regex);                     \
        record_dump (msg);                                              \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

#define WEE_CHECK_MSG_REGEX_CORE(__regex)                               \
    WEE_CHECK_MSG_REGEX_BUFFER("core.weechat", __regex);


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
        string_dyn_concat (msg, ": ", -1);
        if (prefix)
        {
            string_dyn_concat (msg, "prefix=\"", -1);
            string_dyn_concat (msg, prefix, -1);
            string_dyn_concat (msg, "\", ", -1);
        }
        string_dyn_concat (msg, "message=\"", -1);
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
    char string[1024];

    WEE_CMD_CORE_ERROR_GENERIC("/buffer xxx");

    /* /buffer, /buffer list */
    WEE_CMD_CORE("/buffer");
    WEE_CHECK_MSG_CORE("", "Buffers list:");
    WEE_CMD_CORE("/buffer list");
    WEE_CHECK_MSG_CORE("", "Buffers list:");

    /* /buffer add, /buffer close */
    WEE_CMD_CORE_MIN_ARGS("/buffer add", "/buffer add");
    WEE_CMD_CORE_ERROR_MSG("/buffer add weechat",
                           "Buffer name \"weechat\" is reserved for WeeChat");
    WEE_CMD_CORE_ERROR_GENERIC("/buffer close 1a-b");
    WEE_CMD_CORE_ERROR_GENERIC("/buffer close 2-b");
    WEE_CMD_CORE_ERROR_GENERIC("/buffer close 1a-5");
    WEE_CMD_CORE("/buffer add test");
    WEE_CMD_CORE("/buffer close core.test");
    WEE_CMD_CORE("/buffer add -free -switch test");
    WEE_CMD_CORE("/buffer close");
    WEE_CMD_CORE("/buffer add test");
    WEE_CMD_CORE("/buffer close 2");
    WEE_CMD_CORE("/buffer add test");
    WEE_CMD_CORE("/buffer close 2-50");
    WEE_CMD_CORE("/buffer add test");
    WEE_CMD_CORE("/buffer close core.test");
    WEE_CMD_CORE("/buffer close xxx");

    /* /buffer clear */
    WEE_CMD_CORE("/buffer clear");
    WEE_CMD_CORE("/buffer clear -all");
    WEE_CMD_CORE("/buffer clear -merged");
    WEE_CMD_CORE("/buffer add test");
    WEE_CMD_CORE("/buffer clear core.test");
    WEE_CMD_CORE("/buffer close core.test");

    /* /buffer move */
    WEE_CMD_CORE_MIN_ARGS("/buffer move", "/buffer move");
    WEE_CMD_CORE_ERROR_MSG("/buffer move xxx", "Invalid buffer number: \"xxx\"");
    WEE_CMD_CORE("/buffer move -");
    WEE_CMD_CORE("/buffer move +");
    WEE_CMD_CORE("/buffer add -switch test");
    WEE_CMD_CORE("/buffer move -1");
    WEE_CMD_CORE("/buffer move +1");
    WEE_CMD_CORE("/buffer close core.test");

    /* /buffer swap */
    WEE_CMD_CORE_MIN_ARGS("/buffer swap", "/buffer swap");
    WEE_CMD_CORE_ERROR_MSG("/buffer swap xxx", "Buffer \"xxx\" not found");
    WEE_CMD_CORE_ERROR_MSG("/buffer swap core.weechat xxx", "Buffer \"xxx\" not found");
    WEE_CMD_CORE("/buffer add test");
    WEE_CMD_CORE("/buffer swap core.test");
    WEE_CMD_CORE("/buffer core.test");
    WEE_CMD_CORE("/buffer swap core.test");
    WEE_CMD_CORE("/buffer swap core.weechat core.test");
    WEE_CMD_CORE("/buffer close core.test");

    /* /buffer cycle */
    WEE_CMD_CORE_MIN_ARGS("/buffer cycle", "/buffer cycle");
    WEE_CMD_CORE("/buffer cycle xxx");
    WEE_CMD_CORE("/buffer cycle core.weechat");
    WEE_CMD_CORE("/buffer add test");
    WEE_CMD_CORE("/buffer cycle core.test");
    WEE_CMD_CORE("/buffer cycle core.weechat core.test");
    WEE_CMD_CORE("/buffer cycle core.weechat core.test");
    WEE_CMD_CORE("/buffer cycle 1 2");
    WEE_CMD_CORE("/buffer cycle 1 2");
    WEE_CMD_CORE("/buffer close core.test");

    /* /buffer merge, /buffer unmerge */
    WEE_CMD_CORE_MIN_ARGS("/buffer merge", "/buffer merge");
    WEE_CMD_CORE_ERROR_MSG("/buffer merge xxx", "Buffer \"xxx\" not found");
    WEE_CMD_CORE_ERROR_MSG("/buffer unmerge xxx", "Invalid buffer number: \"xxx\"");
    WEE_CMD_CORE("/buffer add test");
    WEE_CMD_CORE("/buffer merge 2");
    WEE_CMD_CORE("/buffer unmerge");
    WEE_CMD_CORE("/buffer core.weechat");
    WEE_CMD_CORE("/buffer merge core.test");
    WEE_CMD_CORE("/buffer unmerge 1");
    WEE_CMD_CORE("/buffer core.weechat");
    WEE_CMD_CORE("/buffer merge core.test");
    WEE_CMD_CORE("/buffer unmerge -all");
    WEE_CMD_CORE("/buffer close core.test");

    /* /buffer hide, /buffer unhide */
    WEE_CMD_CORE("/buffer hide");
    WEE_CMD_CORE("/buffer unhide");
    WEE_CMD_CORE("/buffer hide -all");
    WEE_CMD_CORE("/buffer unhide -all");
    WEE_CMD_CORE("/buffer add test");
    WEE_CMD_CORE("/buffer hide core.weechat 2");
    WEE_CMD_CORE("/buffer unhide 1 core.test");
    WEE_CMD_CORE("/buffer close core.test");

    /* /buffer switch */
    WEE_CMD_CORE("/buffer switch -previous");
    WEE_CMD_CORE("/buffer switch");

    /* /buffer zoom */
    WEE_CMD_CORE("/buffer zoom");

    /* /buffer renumber */
    WEE_CMD_CORE_ERROR_MSG(
        "/buffer renumber",
        "Renumbering is allowed only if option weechat.look.buffer_auto_renumber is off");
    WEE_CMD_CORE("/set weechat.look.buffer_auto_renumber off");
    WEE_CMD_CORE_ERROR_MSG("/buffer renumber xxx 2 5", "Invalid buffer number: \"xxx\"");
    WEE_CMD_CORE_ERROR_MSG("/buffer renumber 1 xxx 5", "Invalid buffer number: \"xxx\"");
    WEE_CMD_CORE_ERROR_MSG("/buffer renumber 1 2 xxx", "Invalid buffer number: \"xxx\"");
    snprintf (string, sizeof (string),
              "Buffer number \"-1\" is out of range (it must be between 1 and %d)",
              GUI_BUFFER_NUMBER_MAX);
    WEE_CMD_CORE_ERROR_MSG("/buffer renumber 1 2 -1", string);
    WEE_CMD_CORE("/buffer renumber");
    WEE_CMD_CORE("/buffer add test");
    WEE_CMD_CORE("/buffer add test2");
    WEE_CMD_CORE("/buffer renumber 1 2 5");
    WEE_CMD_CORE("/buffer close core.test core.test2");
    WEE_CMD_CORE("/reset weechat.look.buffer_auto_renumber");

    /* /buffer notify */
    WEE_CMD_CORE("/buffer notify");
    WEE_CHECK_MSG_CORE("", "Notify for \"core.weechat\": \"all\"");
    WEE_CMD_CORE_ERROR_MSG("/buffer notify xxx", "Unable to set notify level \"xxx\"");

    /* /buffer listvar */
    WEE_CMD_CORE("/buffer listvar");
    WEE_CHECK_MSG_CORE("", "Local variables for buffer \"core.weechat\":");
    WEE_CMD_CORE("/buffer localvar");
    WEE_CHECK_MSG_CORE("", "Local variables for buffer \"core.weechat\":");
    WEE_CMD_CORE_ERROR_MSG("/buffer listvar xxx", "Buffer \"xxx\" not found");

    /* /buffer setvar, /buffer delvar */
    WEE_CMD_CORE_MIN_ARGS("/buffer setvar", "/buffer setvar");
    WEE_CMD_CORE_MIN_ARGS("/buffer delvar", "/buffer delvar");
    WEE_CMD_CORE("/buffer setvar test");
    WEE_CMD_CORE("/buffer listvar core.weechat");
    WEE_CHECK_MSG_CORE("", "  test: \"\"");
    WEE_CMD_CORE("/buffer setvar test value");
    WEE_CMD_CORE("/buffer listvar core.weechat");
    WEE_CHECK_MSG_CORE("", "  test: \"value\"");
    WEE_CMD_CORE("/buffer setvar test \"value2\"");
    WEE_CMD_CORE("/buffer listvar core.weechat");
    WEE_CHECK_MSG_CORE("", "  test: \"value2\"");
    WEE_CMD_CORE("/buffer delvar test");

    /* /buffer set, /buffer setauto, /buffer get */
    WEE_CMD_CORE_MIN_ARGS("/buffer set", "/buffer set");
    WEE_CMD_CORE_MIN_ARGS("/buffer setauto", "/buffer setauto");
    WEE_CMD_CORE_MIN_ARGS("/buffer get", "/buffer get");
    WEE_CMD_CORE("/buffer set input");
    WEE_CMD_CORE("/buffer setauto input");
    WEE_CMD_CORE("/buffer set input test");
    WEE_CMD_CORE("/buffer get input");
    WEE_CHECK_MSG_CORE("", "core.weechat: (str) input = test");
    WEE_CMD_CORE("/buffer set input");
    WEE_CMD_CORE("/buffer get localvar_plugin");
    WEE_CHECK_MSG_CORE("", "core.weechat: (str) localvar_plugin = core");
    WEE_CMD_CORE("/buffer setauto short_name weechat2");
    WEE_CMD_CORE("/buffer get short_name");
    WEE_CHECK_MSG_CORE("", "core.weechat: (str) short_name = weechat2");
    WEE_CMD_CORE("/buffer setauto short_name weechat");
    WEE_CMD_CORE("/buffer get plugin");
    snprintf (string, sizeof (string), "core.weechat: (ptr) plugin = %p", (void *)NULL);
    WEE_CHECK_MSG_CORE("", string);

    /* /buffer jump */
    WEE_CMD_CORE_MIN_ARGS("/buffer jump", "/buffer jump");
    WEE_CMD_CORE_ERROR_GENERIC("/buffer jump xxx");
    WEE_CMD_CORE("/buffer jump smart");
    WEE_CMD_CORE("/buffer jump last_displayed");
    WEE_CMD_CORE("/buffer jump prev_visited");
    WEE_CMD_CORE("/buffer jump next_visited");

    /* relative jump */
    WEE_CMD_CORE("/buffer -");
    WEE_CMD_CORE("/buffer +");
    WEE_CMD_CORE("/buffer -10");
    WEE_CMD_CORE("/buffer add test");
    WEE_CMD_CORE("/buffer hide test");
    WEE_CMD_CORE("/buffer add test2");
    WEE_CMD_CORE("/buffer +1");
    WEE_CMD_CORE("/buffer -1");
    WEE_CMD_CORE("/buffer close core.test core.test2");

    /* smart jump */
    WEE_CMD_CORE_ERROR_MSG("/buffer *xxx", "Invalid buffer number: \"xxx\"");
    WEE_CMD_CORE("/buffer *");
    WEE_CMD_CORE("/buffer *2");

    /* jump by id, number or name */
    WEE_CMD_CORE("/buffer add test");
    WEE_CMD_CORE("/buffer 2");
    WEE_CMD_CORE("/buffer 1");
    WEE_CMD_CORE("/buffer core.test");
    WEE_CMD_CORE("/buffer core.weechat");
    snprintf (string, sizeof (string), "/buffer %lld", gui_buffers->id);
    WEE_CMD_CORE(string);
    WEE_CMD_CORE("/buffer close core.test");
}

/*
 * Tests functions:
 *   command_color
 */

TEST(CoreCommand, Color)
{
    char string[1024];

    WEE_CMD_CORE_ERROR_GENERIC("/color xxx");

    /* /color */
    WEE_CMD_CORE("/color");
    WEE_CMD_CORE("/buffer close core.color");
    WEE_CMD_CORE("/buffer add -switch test");

    /* /color -o */
    WEE_CMD_CORE("/color -o");

    /* /color alias, /color unalias */
    WEE_CMD_CORE_MIN_ARGS("/color alias", "/color alias");
    WEE_CMD_CORE_MIN_ARGS("/color alias 214", "/color alias");
    WEE_CMD_CORE_MIN_ARGS("/color unalias", "/color unalias");
    WEE_CMD_CORE("/color alias 214 orange");
    WEE_CMD_CORE("/color unalias 214");
    snprintf (string, sizeof (string),
              "Invalid color number \"-2\" (must be between 0 and %d)",
              gui_color_get_term_colors ());
    WEE_CMD_CORE_ERROR_MSG("/color alias -2 red", string);
    WEE_CMD_CORE_ERROR_MSG("/color unalias -2", string);
    snprintf (string, sizeof (string),
              "Invalid color number \"9999999\" (must be between 0 and %d)",
              gui_color_get_term_colors ());
    WEE_CMD_CORE_ERROR_MSG("/color alias 9999999 red", string);
    WEE_CMD_CORE_ERROR_MSG("/color unalias 9999999", string);
    snprintf (string, sizeof (string),
              "Invalid color number \"xxx\" (must be between 0 and %d)",
              gui_color_get_term_colors ());
    WEE_CMD_CORE_ERROR_MSG("/color alias xxx red", string);
    WEE_CMD_CORE_ERROR_MSG("/color unalias xxx", string);
    WEE_CMD_CORE_ERROR_MSG("/color unalias 214",
                           "Color \"214\" is not defined in palette");
    WEE_CMD_CORE("/color alias 214 orange 255/175/0");
    WEE_CMD_CORE("/color unalias 214");

    /* /color reset */
    WEE_CMD_CORE("/color reset");

    /* /color switch */
    WEE_CMD_CORE("/color");
    WEE_CMD_CORE("/color switch");
    WEE_CMD_CORE("/color switch");
    WEE_CMD_CORE("/buffer close core.color");

    /* /color term2rgb */
    WEE_CMD_CORE_MIN_ARGS("/color term2rgb", "/color term2rgb");
    WEE_CMD_CORE_ERROR_GENERIC("/color term2rgb xxx");
    WEE_CMD_CORE("/color term2rgb 214");
    WEE_CHECK_MSG_CORE("", "214 -> #ffaf00");

    /* /color rgb2term */
    WEE_CMD_CORE_MIN_ARGS("/color rgb2term", "/color rgb2term");
    WEE_CMD_CORE_ERROR_GENERIC("/color rgb2term xxx");
    WEE_CMD_CORE_ERROR_GENERIC("/color rgb2term fffffff");
    WEE_CMD_CORE_ERROR_GENERIC("/color rgb2term ffaf00 1000");
    WEE_CMD_CORE("/color rgb2term ffaf00");
    WEE_CHECK_MSG_CORE("", "#ffaf00 -> 214");
    WEE_CMD_CORE("/color rgb2term #ffaf00");
    WEE_CHECK_MSG_CORE("", "#ffaf00 -> 214");
    WEE_CMD_CORE("/color rgb2term #ffaf00 100");
    WEE_CHECK_MSG_CORE("", "#ffaf00 -> 11");
}

/*
 * Tests functions:
 *   command_command
 */

TEST(CoreCommand, Command)
{
    WEE_CMD_CORE_MIN_ARGS("/command", "/command");
    WEE_CMD_CORE_MIN_ARGS("/command *", "/command");

    /* /command -s */
    WEE_CMD_CORE("/command -s /print test1;/print test2");
    WEE_CHECK_MSG_CORE("", "test1");
    WEE_CHECK_MSG_CORE("", "test2");

    /* /command -buffer */
    WEE_CMD_CORE_ERROR_MSG("/command -buffer xxx * /print test",
                           "Buffer \"xxx\" not found");

    /* /command <extension> <command> */
    WEE_CMD_CORE_ERROR_MSG("/command xxx /print test", "Plugin \"xxx\" not found");
    WEE_CMD_CORE("/command * /print test");
    WEE_CHECK_MSG_CORE("", "test");
    WEE_CMD_CORE("/command * print test");
    WEE_CHECK_MSG_CORE("", "test");
}

/*
 * Tests functions:
 *   command_cursor
 */

TEST(CoreCommand, Cursor)
{
    WEE_CMD_CORE_ERROR_GENERIC("/cursor xxx");

    WEE_CMD_CORE("/window bare");
    WEE_CMD_CORE("/cursor");
    WEE_CMD_CORE("/window bare");
    WEE_CMD_CORE("/cursor");
    WEE_CMD_CORE("/cursor");

    /* /cursor go, /cursor stop */
    WEE_CMD_CORE_MIN_ARGS("/cursor go", "/cursor go");
    WEE_CMD_CORE_ERROR_GENERIC("/cursor go x,y");
    WEE_CMD_CORE_ERROR_GENERIC("/cursor go xxx");
    WEE_CMD_CORE("/cursor go 0,0");
    WEE_CMD_CORE("/cursor stop");
    WEE_CMD_CORE("/cursor go chat");
    WEE_CMD_CORE("/cursor stop");
    WEE_CMD_CORE("/cursor go chat bottom_left");
    WEE_CMD_CORE("/cursor stop");

    /* /cursor move, /cursor stop */
    WEE_CMD_CORE_MIN_ARGS("/cursor move", "/cursor move");
    WEE_CMD_CORE_ERROR_GENERIC("/cursor move xxx");
    WEE_CMD_CORE("/cursor move up");
    WEE_CMD_CORE("/cursor move down");
    WEE_CMD_CORE("/cursor move left");
    WEE_CMD_CORE("/cursor move right");
    WEE_CMD_CORE("/cursor move top_left");
    WEE_CMD_CORE("/cursor move top_right");
    WEE_CMD_CORE("/cursor move bottom_left");
    WEE_CMD_CORE("/cursor move bottom_right");
    WEE_CMD_CORE("/cursor move edge_top");
    WEE_CMD_CORE("/cursor move edge_bottom");
    WEE_CMD_CORE("/cursor move edge_left");
    WEE_CMD_CORE("/cursor move edge_right");
    WEE_CMD_CORE("/cursor move area_up");
    WEE_CMD_CORE("/cursor move area_down");
    WEE_CMD_CORE("/cursor move area_left");
    WEE_CMD_CORE("/cursor move area_right");
    WEE_CMD_CORE("/cursor stop");
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

    WEE_CMD_CORE_ERROR_GENERIC("/debug xxx");

    /* /debug, /debug list */
    WEE_CMD_CORE("/debug set core 1");
    WEE_CMD_CORE("/debug");
    WEE_CHECK_MSG_CORE("", "Debug:");
    WEE_CMD_CORE("/debug list");
    WEE_CHECK_MSG_CORE("", "Debug:");
    WEE_CMD_CORE("/debug set core 0");

    /* /debug buffer */
    WEE_CMD_CORE("/debug buffer");
    WEE_CHECK_MSG_CORE("", "Raw content of buffers has been written in log file");

    /* /debug callbacks */
    WEE_CMD_CORE_MIN_ARGS("/debug callbacks", "/debug callbacks");
    WEE_CMD_CORE_ERROR_GENERIC("/debug callbacks xxx");
    CHECK(debug_long_callbacks == 0);
    WEE_CMD_CORE("/debug callbacks 957ms");
    CHECK(debug_long_callbacks == 957000);
    WEE_CHECK_MSG_CORE("", "Debug enabled for callbacks (threshold: 0:00:00.957000)");
    WEE_CMD_CORE("/debug callbacks 0");
    CHECK(debug_long_callbacks == 0);
    WEE_CHECK_MSG_CORE("", "Debug disabled for callbacks");

    /* /debug certs */
    WEE_CMD_CORE("/debug certs");
    WEE_CHECK_MSG_REGEX_CORE("certificate.*loaded.*system.*user");

    /* /debug color */
    WEE_CMD_CORE("/debug color");
    WEE_CHECK_MSG_REGEX_CORE("TERM=.*COLORS:.*COLOR_PAIRS:.*");
    WEE_CHECK_MSG_REGEX_CORE("WeeChat colors");

    /* /debug cursor */
    LONGS_EQUAL(0, gui_cursor_debug);
    WEE_CMD_CORE("/debug cursor");
    LONGS_EQUAL(1, gui_cursor_debug);
    WEE_CHECK_MSG_CORE("", "Debug enabled for cursor mode (normal)");
    WEE_CMD_CORE("/debug cursor");
    LONGS_EQUAL(0, gui_cursor_debug);
    WEE_CHECK_MSG_CORE("", "Debug disabled for cursor mode");
    WEE_CMD_CORE("/debug cursor verbose");
    LONGS_EQUAL(2, gui_cursor_debug);
    WEE_CHECK_MSG_CORE("", "Debug enabled for cursor mode (verbose)");
    WEE_CMD_CORE("/debug cursor verbose");
    LONGS_EQUAL(0, gui_cursor_debug);
    WEE_CHECK_MSG_CORE("", "Debug disabled for cursor mode");

    /* /debug dirs */
    WEE_CMD_CORE("/debug dirs");
    WEE_CHECK_MSG_CORE("", "  home:");
    WEE_CHECK_MSG_REGEX_CORE("    config: ");
    WEE_CHECK_MSG_REGEX_CORE("    data: ");
    WEE_CHECK_MSG_REGEX_CORE("    state: ");
    WEE_CHECK_MSG_REGEX_CORE("    cache: ");
    WEE_CHECK_MSG_REGEX_CORE("    runtime: ");
    WEE_CHECK_MSG_REGEX_CORE("  lib: ");
    WEE_CHECK_MSG_REGEX_CORE("  lib \\(extra\\): ");
    WEE_CHECK_MSG_REGEX_CORE("  share: ");
    WEE_CHECK_MSG_REGEX_CORE("  locale: ");

    /* /debug dump */
    WEE_CMD_CORE("/debug dump");
    WEE_CMD_CORE("/debug dump irc");

    /* /debug hdata */
    WEE_CMD_CORE("/debug hdata");
    WEE_CHECK_MSG_REGEX_CORE("[0-9]+ hdata in memory");
    WEE_CMD_CORE("/debug hdata free");

    /* /debug hooks */
    WEE_CMD_CORE("/debug hooks");
    WEE_CHECK_MSG_CORE("", "hooks in memory:");
    WEE_CMD_CORE("/debug hooks irc");
    WEE_CHECK_MSG_REGEX_CORE("hooks \\([0-9]+\\):");
    WEE_CMD_CORE("/debug hooks irc timer");
    WEE_CHECK_MSG_REGEX_CORE("hooks \\([0-9]+\\):");

    /* /debug infolists */
    WEE_CMD_CORE("/debug infolists");
    WEE_CHECK_MSG_REGEX_CORE("[0-9]+ infolists in memory");

    /* /debug key */
    LONGS_EQUAL(0, gui_key_debug);
    WEE_CMD_CORE("/debug key");
    LONGS_EQUAL(1, gui_key_debug);
    gui_key_debug = 0;

    /* /debug libs */
    WEE_CMD_CORE("/debug libs");
    WEE_CHECK_MSG_CORE("", "Libs:");

    /* /debug memory */
    WEE_CMD_CORE("/debug memory");
    WEE_CHECK_MSG_REGEX_CORE("Memory usage");

    /* /debug mouse */
    LONGS_EQUAL(0, gui_mouse_debug);
    WEE_CMD_CORE("/debug mouse");
    LONGS_EQUAL(1, gui_mouse_debug);
    WEE_CHECK_MSG_CORE("", "Debug enabled for mouse (normal)");
    WEE_CMD_CORE("/debug mouse");
    LONGS_EQUAL(0, gui_mouse_debug);
    WEE_CHECK_MSG_CORE("", "Debug disabled for mouse");
    WEE_CMD_CORE("/debug mouse verbose");
    LONGS_EQUAL(2, gui_mouse_debug);
    WEE_CHECK_MSG_CORE("", "Debug enabled for mouse (verbose)");
    WEE_CMD_CORE("/debug mouse");
    LONGS_EQUAL(0, gui_mouse_debug);
    WEE_CHECK_MSG_CORE("", "Debug disabled for mouse");

    /* /debug set */
    LONGS_EQUAL(0, weechat_debug_core);
    WEE_CMD_CORE("/debug set core 1");
    WEE_CHECK_MSG_CORE("", "debug: \"core\" => 1");
    LONGS_EQUAL(1, weechat_debug_core);
    WEE_CMD_CORE("/debug set core 2");
    WEE_CHECK_MSG_CORE("", "debug: \"core\" => 2");
    LONGS_EQUAL(2, weechat_debug_core);
    WEE_CMD_CORE("/debug set core 0");
    WEE_CHECK_MSG_CORE("", "Debug disabled for \"core\"");
    LONGS_EQUAL(0, weechat_debug_core);

    /* /debug tags */
    LONGS_EQUAL(0, gui_chat_display_tags);
    WEE_CMD_CORE("/debug tags");
    LONGS_EQUAL(1, gui_chat_display_tags);
    WEE_CMD_CORE("/debug tags");
    LONGS_EQUAL(0, gui_chat_display_tags);

    /* /debug term */
    WEE_CMD_CORE("/debug term");
    WEE_CHECK_MSG_REGEX_CORE("TERM=.*size:");

    /* /debug time */
    WEE_CMD_CORE_MIN_ARGS("/debug time", "/debug time");
    WEE_CMD_CORE("/debug time /print test");
    WEE_CHECK_MSG_CORE("", "test");

    /* /debug unicode */
    WEE_CMD_CORE_MIN_ARGS("/debug unicode", "/debug unicode");
    WEE_CMD_CORE(command_debug_unicode);
    WEE_CHECK_MSG_CORE("", "  \"\u00E9\u26C4\": 5 / 2, 2 / 3, 3, 3");
    WEE_CHECK_MSG_CORE("", "  \"\u00E9\" (U+00E9, 233, 0xC3 0xA9): 2 / 1, 1 / 1, 1, 1, 1");
    WEE_CHECK_MSG_CORE("", "  \"\u26C4\" (U+26C4, 9924, 0xE2 0x9B 0x84): 3 / 1, 1 / 2, 2, 2, 2");

    /* /debug url */
    LONGS_EQUAL(0, url_debug);
    WEE_CMD_CORE("/debug url");
    LONGS_EQUAL(1, url_debug);
    WEE_CHECK_MSG_CORE("", "Debug hook_url: enabled");
    WEE_CMD_CORE("/debug url");
    LONGS_EQUAL(0, url_debug);
    WEE_CHECK_MSG_CORE("", "Debug hook_url: disabled");

    /* /debug windows */
    WEE_CMD_CORE("/debug windows");
    WEE_CHECK_MSG_CORE("", "Windows tree:");

    /* /debug whitespace */
    LONGS_EQUAL(0, gui_chat_whitespace_mode);
    WEE_CMD_CORE("/debug whitespace");
    LONGS_EQUAL(1, gui_chat_whitespace_mode);
    WEE_CMD_CORE("/debug whitespace");
    LONGS_EQUAL(0, gui_chat_whitespace_mode);
}

/*
 * Tests functions:
 *   command_eval
 */

TEST(CoreCommand, Eval)
{
    WEE_CMD_CORE_MIN_ARGS("/eval", "/eval");

    /* /eval */
    WEE_CMD_CORE("/eval /print test");
    WEE_CHECK_MSG_CORE("", "test");

    /* /eval -d */
    WEE_CMD_CORE("/eval -d /print test");
    WEE_CHECK_MSG_REGEX_CORE("eval_expression\\(\"/print test\"\\)");
    WEE_CHECK_MSG_CORE("", "test");

    /* /eval -n */
    WEE_CMD_CORE("/eval -n ${calc:1+1}");
    WEE_CHECK_MSG_CORE("", "== [2]");

    /* /eval -c -n */
    WEE_CMD_CORE("/eval -c -n abc == abc");
    WEE_CHECK_MSG_CORE("", "== [1]");
    WEE_CMD_CORE("/eval -c -n abc != abc");
    WEE_CHECK_MSG_CORE("", "== [0]");

    /* /eval -c -n -d */
    WEE_CMD_CORE("/eval -c -n -d abc == abc");
    WEE_CHECK_MSG_REGEX_CORE("eval_expression\\(\"abc == abc\"\\)");
    WEE_CHECK_MSG_CORE("", "== [1]");

    /* /eval -s */
    WEE_CMD_CORE("/eval -s /print test1;/print test2");
    WEE_CHECK_MSG_CORE("", "test1");
    WEE_CHECK_MSG_CORE("", "test2");

    /* /eval -s -d */
    WEE_CMD_CORE("/eval -s -d /print test1;/print test2");
    WEE_CHECK_MSG_REGEX_CORE("eval_expression\\(\"/print test1\"\\)");
    WEE_CHECK_MSG_REGEX_CORE("eval_expression\\(\"/print test2\"\\)");
    WEE_CHECK_MSG_CORE("", "test1");
    WEE_CHECK_MSG_CORE("", "test2");
}

/*
 * Tests functions:
 *   command_filter
 */

TEST(CoreCommand, Filter)
{
    WEE_CMD_CORE_ERROR_GENERIC("/filter xxx");

    /* /filter, /filter list */
    WEE_CMD_CORE("/filter");
    WEE_CHECK_MSG_CORE("", "Message filtering enabled");
    WEE_CHECK_MSG_CORE("", "No message filter defined");
    WEE_CMD_CORE("/filter list");
    WEE_CHECK_MSG_CORE("", "Message filtering enabled");
    WEE_CHECK_MSG_CORE("", "No message filter defined");
    WEE_CMD_CORE("/filter add test core.weechat * regex example");
    WEE_CMD_CORE("/filter list");
    WEE_CHECK_MSG_CORE("", "Message filtering enabled");
    WEE_CHECK_MSG_CORE("", "Message filters:");
    WEE_CHECK_MSG_CORE("", "  test: buffer: core.weechat / tags: * / regex: regex example");
    WEE_CMD_CORE("/filter del test");

    /* /filter enable, /filter disable, /filter toggle */
    WEE_CMD_CORE("/filter disable");
    WEE_CHECK_MSG_CORE("", "Message filtering disabled");
    WEE_CMD_CORE("/filter enable");
    WEE_CHECK_MSG_CORE("", "Message filtering enabled");
    WEE_CMD_CORE("/filter toggle");
    WEE_CHECK_MSG_CORE("", "Message filtering disabled");
    WEE_CMD_CORE("/filter toggle");
    WEE_CHECK_MSG_CORE("", "Message filtering enabled");
    WEE_CMD_CORE("/filter add test core.weechat * regex example");
    WEE_CMD_CORE("/filter disable test");
    WEE_CHECK_MSG_CORE("", "Filter \"test\" disabled");
    WEE_CMD_CORE("/filter enable test");
    WEE_CHECK_MSG_CORE("", "Filter \"test\" enabled");
    WEE_CMD_CORE("/filter toggle test");
    WEE_CHECK_MSG_CORE("", "Filter \"test\" disabled");
    WEE_CMD_CORE("/filter toggle test");
    WEE_CHECK_MSG_CORE("", "Filter \"test\" enabled");
    LONGS_EQUAL(1, gui_buffers->filter);
    WEE_CMD_CORE("/filter disable @");
    LONGS_EQUAL(0, gui_buffers->filter);
    WEE_CMD_CORE("/filter enable @");
    LONGS_EQUAL(1, gui_buffers->filter);
    WEE_CMD_CORE("/filter toggle @");
    LONGS_EQUAL(0, gui_buffers->filter);
    WEE_CMD_CORE("/filter toggle @");
    LONGS_EQUAL(1, gui_buffers->filter);
    WEE_CMD_CORE("/filter del test");

    /* /filter add, /filter addreplace, /filter recreate */
    WEE_CMD_CORE_MIN_ARGS("/filter add", "/filter add");
    WEE_CMD_CORE_MIN_ARGS("/filter add test", "/filter add");
    WEE_CMD_CORE_MIN_ARGS("/filter add test core.weechat", "/filter add");
    WEE_CMD_CORE_MIN_ARGS("/filter add test core.weechat *", "/filter add");
    WEE_CMD_CORE_ERROR_MSG("/filter add test core.weechat * *",
                           "You must specify at least tags or regex for filter");
    WEE_CMD_CORE("/filter add test core.weechat * regex example");
    WEE_CHECK_MSG_CORE("", "Filter \"test\" added:");
    WEE_CHECK_MSG_CORE("", "  test: buffer: core.weechat / tags: * / regex: regex example");
    WEE_CMD_CORE_MIN_ARGS("/filter addreplace", "/filter addreplace");
    WEE_CMD_CORE_MIN_ARGS("/filter addreplace test", "/filter addreplace");
    WEE_CMD_CORE_MIN_ARGS("/filter addreplace test core.weechat", "/filter addreplace");
    WEE_CMD_CORE_MIN_ARGS("/filter addreplace test core.weechat *", "/filter addreplace");
    WEE_CMD_CORE("/filter addreplace test core.weechat * regex example2");
    WEE_CHECK_MSG_CORE("", "Filter \"test\" updated:");
    WEE_CHECK_MSG_CORE("", "  test: buffer: core.weechat / tags: * / regex: regex example2");
    WEE_CMD_CORE_ERROR_MSG("/filter recreate xxx", "Filter \"xxx\" not found");
    WEE_CMD_CORE("/filter recreate test");
    STRCMP_EQUAL(gui_buffers->input_buffer,
                 "/filter addreplace test core.weechat * regex example2");
    WEE_CMD_CORE("/input delete_line");
    WEE_CMD_CORE("/filter del test");

    /* /filter rename */
    WEE_CMD_CORE_MIN_ARGS("/filter rename", "/filter rename");
    WEE_CMD_CORE_MIN_ARGS("/filter rename xxx", "/filter rename");
    WEE_CMD_CORE_ERROR_MSG("/filter rename xxx yyy", "Filter \"xxx\" not found");
    WEE_CMD_CORE("/filter add test1 core.weechat * regex example");
    WEE_CMD_CORE("/filter add test2 core.weechat * regex example");
    WEE_CMD_CORE_ERROR_MSG("/filter rename test1 test2",
                           "Unable to rename filter \"test1\" to \"test2\"");
    WEE_CMD_CORE("/filter rename test1 test3");
    WEE_CHECK_MSG_CORE("", "Filter \"test1\" renamed to \"test3\"");
    WEE_CMD_CORE("/filter del test2 test3");

    /* /filter del */
    WEE_CMD_CORE("/filter add test1 core.weechat * regex example");
    WEE_CMD_CORE("/filter add test2 core.weechat * regex example2");
    CHECK(gui_filters);
    CHECK(gui_filters->next_filter);
    WEE_CMD_CORE("/filter del test*");
    POINTERS_EQUAL(NULL, gui_filters);
}

/*
 * Tests functions:
 *   command_help
 */

TEST(CoreCommand, Help)
{
    WEE_CMD_CORE_ERROR_MSG(
        "/help xxx",
        "No help available, \"xxx\" is not a command or an option");

    /* /help, /help -list, /help -listfull */
    WEE_CMD_CORE("/help");
    WEE_CHECK_MSG_CORE("", "[core]");
    WEE_CMD_CORE("/help -list");
    WEE_CHECK_MSG_CORE("", "[core]");
    WEE_CMD_CORE("/help -listfull");
    WEE_CHECK_MSG_CORE("", "[core]");
    WEE_CMD_CORE("/help -listfull core irc fset");
    WEE_CHECK_MSG_CORE("", "[core]");
    WEE_CHECK_MSG_CORE("", "[irc]");
    WEE_CHECK_MSG_CORE("", "[fset]");

    /* /help <command> */
    WEE_CMD_CORE("/help help");
    WEE_CHECK_MSG_CORE("", "display help about commands and options");

    /* /help <option> (with defined value) */
    /* boolean */
    WEE_CMD_CORE("/help weechat.look.confirm_quit");
    WEE_CHECK_MSG_CORE("", "Option \"weechat.look.confirm_quit\":");
    /* integer */
    WEE_CMD_CORE("/help weechat.look.color_pairs_auto_reset");
    WEE_CHECK_MSG_CORE("", "Option \"weechat.look.color_pairs_auto_reset\":");
    /* string */
    WEE_CMD_CORE("/help weechat.look.bar_more_down");
    WEE_CHECK_MSG_CORE("", "Option \"weechat.look.bar_more_down\":");
    /* color */
    WEE_CMD_CORE("/help weechat.color.bar_more");
    WEE_CHECK_MSG_CORE("", "Option \"weechat.color.bar_more\":");
    /* enum */
    WEE_CMD_CORE("/help weechat.look.input_share");
    WEE_CHECK_MSG_CORE("", "Option \"weechat.look.input_share\":");

    /* /help <option> (with undefined value: test with a new IRC server) */
    WEE_CMD_CORE("/server add test 127.0.0.1");
    /* boolean */
    WEE_CMD_CORE("/help irc.server.test.autojoin_dynamic");
    WEE_CHECK_MSG_CORE("", "Option \"irc.server.test.autojoin_dynamic\":");
    /* integer */
    WEE_CMD_CORE("/help irc.server.test.autojoin_delay");
    WEE_CHECK_MSG_CORE("", "Option \"irc.server.test.autojoin_delay\":");
    /* string */
    WEE_CMD_CORE("/help irc.server.test.autojoin");
    WEE_CHECK_MSG_CORE("", "Option \"irc.server.test.autojoin\":");
    /* enum */
    WEE_CMD_CORE("/help irc.server.test.sasl_fail");
    WEE_CHECK_MSG_CORE("", "Option \"irc.server.test.sasl_fail\":");
    WEE_CMD_CORE("/server del test");
}

/*
 * Tests functions:
 *   command_history
 */

TEST(CoreCommand, History)
{
    WEE_CMD_CORE("/history");
    WEE_CMD_CORE("/history clear");
    WEE_CMD_CORE_ERROR_GENERIC("/history xxx");
    WEE_CMD_CORE_ERROR_GENERIC("/history -1");
}


/*
 * Tests functions:
 *   command_hotlist
 */

TEST(CoreCommand, Hotlist)
{
    WEE_CMD_CORE_MIN_ARGS("/hotlist", "/hotlist");
    WEE_CMD_CORE_ERROR_GENERIC("/hotlist xxx");

    /* /hotlist add, /hotlist clear, /hotlist remove */
    WEE_CMD_CORE_ERROR_GENERIC("/hotlist add xxx");
    WEE_CMD_CORE("/hotlist clear");
    POINTERS_EQUAL(NULL, gui_buffers->hotlist);
    WEE_CMD_CORE("/hotlist add");
    CHECK(gui_buffers->hotlist);
    LONGS_EQUAL(GUI_HOTLIST_LOW, gui_buffers->hotlist->priority);
    WEE_CMD_CORE("/hotlist remove");
    POINTERS_EQUAL(NULL, gui_buffers->hotlist);
    WEE_CMD_CORE("/hotlist add message");
    CHECK(gui_buffers->hotlist);
    LONGS_EQUAL(GUI_HOTLIST_MESSAGE, gui_buffers->hotlist->priority);
    WEE_CMD_CORE("/hotlist remove");
    POINTERS_EQUAL(NULL, gui_buffers->hotlist);
    WEE_CMD_CORE("/hotlist add private");
    CHECK(gui_buffers->hotlist);
    LONGS_EQUAL(GUI_HOTLIST_PRIVATE, gui_buffers->hotlist->priority);
    WEE_CMD_CORE("/hotlist remove");
    POINTERS_EQUAL(NULL, gui_buffers->hotlist);
    WEE_CMD_CORE("/hotlist add highlight");
    CHECK(gui_buffers->hotlist);
    LONGS_EQUAL(GUI_HOTLIST_HIGHLIGHT, gui_buffers->hotlist->priority);
    WEE_CMD_CORE("/hotlist remove");

    /* /hotlist restore */
    WEE_CMD_CORE("/buffer add test");
    WEE_CMD_CORE("/command -buffer core.test * /hotlist add highlight");
    CHECK(gui_hotlist);
    LONGS_EQUAL(GUI_HOTLIST_HIGHLIGHT, gui_hotlist->priority);
    WEE_CMD_CORE("/hotlist clear 1");
    CHECK(gui_hotlist);
    LONGS_EQUAL(GUI_HOTLIST_HIGHLIGHT, gui_hotlist->priority);
    WEE_CMD_CORE("/hotlist clear");
    POINTERS_EQUAL(NULL, gui_hotlist);
    WEE_CMD_CORE("/hotlist restore -all");
    CHECK(gui_hotlist);
    WEE_CMD_CORE("/hotlist clear");
    POINTERS_EQUAL(NULL, gui_hotlist);
    WEE_CMD_CORE("/command -buffer core.test * /hotlist restore");
    CHECK(gui_hotlist);
    LONGS_EQUAL(GUI_HOTLIST_HIGHLIGHT, gui_hotlist->priority);
    WEE_CMD_CORE("/hotlist clear");
    WEE_CMD_CORE("/buffer close core.test");
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
    WEE_CMD_CORE_ERROR_GENERIC("/print -xxx");

    /* /print */
    WEE_CMD_CORE("/print");
    WEE_CHECK_MSG_CORE("", "");
    WEE_CMD_CORE("/print hello");
    WEE_CHECK_MSG_CORE("", "hello");
    WEE_CMD_CORE("/print \\-hello");
    WEE_CHECK_MSG_CORE("", "-hello");
    WEE_CMD_CORE("/print prefix\\thello");
    WEE_CHECK_MSG_CORE("prefix", "hello");

    /* /print -buffer */
    WEE_CMD_CORE("/buffer add test");
    WEE_CMD_CORE_ERROR_GENERIC("/print -buffer");
    WEE_CMD_CORE_ERROR_GENERIC("/print -buffer xxx");
    WEE_CMD_CORE("/print -buffer core.test hello");
    WEE_CHECK_MSG_BUFFER("core.test", "", "hello");
    WEE_CMD_CORE("/print -buffer core.weechat hello");
    WEE_CHECK_MSG_CORE("", "hello");
    WEE_CMD_CORE("/buffer close test");

    /* /print -core, /print -current */
    WEE_CMD_CORE("/print -core hello");
    WEE_CHECK_MSG_CORE("", "hello");
    WEE_CMD_CORE("/print -current hello");
    WEE_CHECK_MSG_CORE("", "hello");

    /* /print -newbuffer */
    WEE_CMD_CORE_ERROR_GENERIC("/print -newbuffer");
    WEE_CMD_CORE_ERROR_MSG("/print -newbuffer weechat",
                           "Buffer name \"weechat\" is reserved for WeeChat");
    WEE_CMD_CORE("/print -newbuffer test hello");
    WEE_CHECK_MSG_BUFFER("core.test", "", "hello");
    WEE_CMD_CORE("/buffer close test");
    WEE_CMD_CORE("/print -newbuffer test -free -switch hello");
    WEE_CHECK_MSG_BUFFER("core.test", "", "hello");
    WEE_CMD_CORE("/buffer close test");

    /* /print -escape */
    WEE_CMD_CORE("/print -escape hello\\a");
    WEE_CHECK_MSG_CORE("", "hello\a");

    /* /print -y */
    WEE_CMD_CORE("/buffer add -free test");
    WEE_CMD_CORE_ERROR_GENERIC("/print -buffer core.test -y");
    WEE_CMD_CORE_ERROR_GENERIC("/print -buffer core.test -y xxx hello");
    WEE_CMD_CORE("/print -buffer core.test -y 5 hello");
    WEE_CHECK_MSG_BUFFER("core.test", "", "hello");
    WEE_CMD_CORE("/print -buffer core.test -y -1 hello");
    WEE_CHECK_MSG_BUFFER("core.test", "", "hello");
    WEE_CMD_CORE("/buffer close test");

    /* /print -date */
    WEE_CMD_CORE_ERROR_GENERIC("/print -date");
    WEE_CMD_CORE_ERROR_GENERIC("/print -date xxx");
    WEE_CMD_CORE_ERROR_GENERIC("/print -date -x");
    WEE_CMD_CORE_ERROR_GENERIC("/print -date +x");
    WEE_CMD_CORE("/print -date 0 hello");
    WEE_CHECK_MSG_CORE("", "hello");
    WEE_CMD_CORE("/print -date -1 hello");
    WEE_CHECK_MSG_CORE("", "hello");
    WEE_CMD_CORE("/print -date +1 hello");
    WEE_CHECK_MSG_CORE("", "hello");
    WEE_CMD_CORE("/print -date 10:32:05 hello");
    WEE_CHECK_MSG_CORE("", "hello");
    WEE_CMD_CORE("/print -date 2025-10-11T10:32:09.123456Z hello");
    WEE_CHECK_MSG_CORE("", "hello");

    /* /print -tags */
    WEE_CMD_CORE_ERROR_GENERIC("/print -tags");
    WEE_CMD_CORE("/print -tags tag1,tag2,tag3 hello");
    WEE_CHECK_MSG_CORE("", "hello");

    /* /print -action, /print -error, /print -join, /print -network, /print -quit */
    WEE_CMD_CORE("/print -action hello");
    WEE_CHECK_MSG_CORE(GUI_CHAT_PREFIX_ACTION_DEFAULT, "hello");
    WEE_CMD_CORE("/print -error hello");
    WEE_CHECK_MSG_CORE(GUI_CHAT_PREFIX_ERROR_DEFAULT, "hello");
    WEE_CMD_CORE("/print -join hello");
    WEE_CHECK_MSG_CORE(GUI_CHAT_PREFIX_JOIN_DEFAULT, "hello");
    WEE_CMD_CORE("/print -network hello");
    WEE_CHECK_MSG_CORE(GUI_CHAT_PREFIX_NETWORK_DEFAULT, "hello");
    WEE_CMD_CORE("/print -quit hello");
    WEE_CHECK_MSG_CORE(GUI_CHAT_PREFIX_QUIT_DEFAULT, "hello");

    /* /print -stdout, /print -stderr */
    WEE_CMD_CORE("/print -stdout hello");
    WEE_CMD_CORE("/print -stderr hello");

    /* /print -beep */
    WEE_CMD_CORE("/print -beep");
}

/*
 * Tests functions:
 *   command_proxy
 */

TEST(CoreCommand, Proxy)
{
    WEE_CMD_CORE_ERROR_GENERIC("/proxy xxx");

    /* /proxy, /proxy list */
    WEE_CMD_CORE("/proxy");
    WEE_CHECK_MSG_CORE("", "No proxy defined");
    WEE_CMD_CORE("/proxy list");
    WEE_CHECK_MSG_CORE("", "No proxy defined");

    /* /proxy add, /proxy addreplace, /proxy del */
    WEE_CMD_CORE_MIN_ARGS("/proxy add", "/proxy add");
    WEE_CMD_CORE_MIN_ARGS("/proxy add local", "/proxy add");
    WEE_CMD_CORE_MIN_ARGS("/proxy add local http", "/proxy add");
    WEE_CMD_CORE_MIN_ARGS("/proxy add local http 127.0.0.1", "/proxy add");
    WEE_CMD_CORE_MIN_ARGS("/proxy addreplace", "/proxy addreplace");
    WEE_CMD_CORE_MIN_ARGS("/proxy addreplace local", "/proxy addreplace");
    WEE_CMD_CORE_MIN_ARGS("/proxy addreplace local http", "/proxy addreplace");
    WEE_CMD_CORE_MIN_ARGS("/proxy addreplace local http 127.0.0.1", "/proxy addreplace");
    WEE_CMD_CORE_ERROR_MSG("/proxy add local xxx 127.0.0.1 8888",
                           "Invalid type \"xxx\" for proxy \"local\"");
    WEE_CMD_CORE_ERROR_MSG("/proxy add local http 127.0.0.1 xxx",
                           "Invalid port \"xxx\" for proxy \"local\"");
    WEE_CMD_CORE("/proxy add local http 127.0.0.1 8888");
    WEE_CHECK_MSG_CORE("", "Proxy \"local\" added");
    WEE_CMD_CORE("/proxy list");
    WEE_CHECK_MSG_CORE("", "List of proxies:");
    WEE_CMD_CORE_ERROR_MSG("/proxy add local http 127.0.0.1 8888",
                           "Proxy \"local\" already exists");
    WEE_CMD_CORE("/proxy addreplace local http 127.0.0.1 9999");
    WEE_CHECK_MSG_CORE("", "Proxy \"local\" updated");
    WEE_CMD_CORE("/proxy addreplace local http 127.0.0.1 9999 user password");
    WEE_CHECK_MSG_CORE("", "Proxy \"local\" updated");
    WEE_CMD_CORE("/proxy del local");
    WEE_CHECK_MSG_CORE("", "Proxy \"local\" deleted");

    /* /proxy set */
    WEE_CMD_CORE("/proxy add local http 127.0.0.1 9999 user password");
    WEE_CMD_CORE_MIN_ARGS("/proxy set", "/proxy set");
    WEE_CMD_CORE_MIN_ARGS("/proxy set local", "/proxy set");
    WEE_CMD_CORE_MIN_ARGS("/proxy set local name", "/proxy set");
    WEE_CMD_CORE_ERROR_MSG("/proxy set local xxx yyy",
                           "Unable to set option \"xxx\" for proxy \"local\"");
    WEE_CMD_CORE("/proxy set local name local2");
    WEE_CMD_CORE_ERROR_MSG("/proxy set local name local2", "Proxy \"local\" not found");
    WEE_CMD_CORE("/proxy set local2 type socks4");
    WEE_CMD_CORE("/proxy set local2 ipv6 disable");
    WEE_CMD_CORE("/proxy set local2 address localhost");
    WEE_CMD_CORE("/proxy set local2 port 1234");
    WEE_CMD_CORE("/proxy set local2 username user2");
    WEE_CMD_CORE("/proxy set local2 password password2");
    WEE_CMD_CORE("/proxy del local2");
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
    WEE_CMD_CORE_MIN_ARGS("/repeat", "/repeat");
    WEE_CMD_CORE_MIN_ARGS("/repeat 2", "/repeat");

    /* /repeat <count> */
    WEE_CMD_CORE_ERROR_MSG("/repeat xxx /yyy", "Invalid number: \"xxx\"");
    WEE_CMD_CORE("/repeat 2 /print test ${repeat_index}");
    WEE_CHECK_MSG_CORE("", "test 1");
    WEE_CHECK_MSG_CORE("", "test 2");

    /* /repeat -interval */
    WEE_CMD_CORE_MIN_ARGS("/repeat -interval", "/repeat");
    WEE_CMD_CORE_ERROR_GENERIC("/repeat -interval xxx 2 /yyy");
    WEE_CMD_CORE("/repeat -interval 0 2 /print test");
    WEE_CHECK_MSG_CORE("", "test");
    WEE_CMD_CORE("/repeat -interval 0 2 /print test ${repeat_index}");
    WEE_CHECK_MSG_CORE("", "test 1");
    WEE_CHECK_MSG_CORE("", "test 2");
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
