/*
 * test-core-input.cpp - test input functions
 *
 * Copyright (C) 2024-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#define TEST_INPUT_DATA(__rc, __buffer, __data, __commands_allowed,     \
                        __split_newline, __user_data)                   \
    record_start ();                                                    \
    LONGS_EQUAL(__rc,                                                   \
        input_data (__buffer, __data, __commands_allowed,               \
                    __split_newline, __user_data));                     \
    record_stop ();

extern "C"
{
#include <string.h>
#include "src/core/core-input.h"
#include "src/core/core-string.h"
#include "src/gui/gui-buffer.h"
#include "src/plugins/plugin.h"
}

TEST_GROUP(CoreInput)
{
};

/*
 * Tests functions:
 *   input_exec_data
 */

TEST(CoreInput, ExecData)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   input_exec_command
 */

TEST(CoreInput, ExecCommand)
{
    /* TODO: write tests */
}

/*
 * Test callback for buffer input.
 */

int
test_core_input_buffer_input_cb (const void *pointer, void *data,
                                 struct t_gui_buffer *buffer,
                                 const char *input_data)
{
    /* make C++ compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;
    (void) input_data;

    return WEECHAT_RC_OK;
}

/*
 * Tests functions:
 *   input_data
 */

TEST(CoreInput, Data)
{
    struct t_gui_buffer *buffer;

    buffer = gui_buffer_new (NULL, "test",
                             &test_core_input_buffer_input_cb, NULL, NULL,
                             NULL, NULL, NULL);
    CHECK(buffer);
    gui_buffer_set (buffer, "input_get_any_user_data", "1");

    LONGS_EQUAL(WEECHAT_RC_ERROR, input_data (NULL, NULL, NULL, 0, 0));
    LONGS_EQUAL(WEECHAT_RC_ERROR, input_data ((struct t_gui_buffer *)0x1,
                                              NULL, NULL, 0, 0));
    LONGS_EQUAL(WEECHAT_RC_ERROR, input_data (gui_buffers, NULL, NULL, 0, 0));

    /* on core buffer: command not found */
    TEST_INPUT_DATA(WEECHAT_RC_ERROR, gui_buffers, "/xxx", NULL, 0, 0);
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "Unknown command \"xxx\" (type /help for help), "
                     "commands with similar name: -", NULL);

    /* on test buffer: command not found */
    TEST_INPUT_DATA(WEECHAT_RC_ERROR, buffer, "/xxx", NULL, 0, 0);
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "Unknown command \"xxx\" (type /help for help), "
                     "commands with similar name: -", NULL);

    /* on core buffer: command not found, but user_data == 1 */
    TEST_INPUT_DATA(WEECHAT_RC_ERROR, gui_buffers, "/xxx", NULL, 0, 1);
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "Unknown command \"xxx\" (type /help for help), "
                     "commands with similar name: -", NULL);

    /* on test buffer: command not found, but user_data == 1 */
    TEST_INPUT_DATA(WEECHAT_RC_OK, buffer, "/xxx", NULL, 0, 1);
    RECORD_CHECK_NO_MSG();

    /* on core buffer: empty text to buffer */
    TEST_INPUT_DATA(WEECHAT_RC_ERROR, gui_buffers, "", NULL, 0, 0);
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "You cannot write text in this buffer", NULL);

    /* on test buffer: empty text to buffer */
    TEST_INPUT_DATA(WEECHAT_RC_OK, buffer, "", NULL, 0, 0);
    RECORD_CHECK_NO_MSG();

    /* on core buffer: text to buffer */
    TEST_INPUT_DATA(WEECHAT_RC_ERROR, gui_buffers, "test", NULL, 0, 0);
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "You cannot write text in this buffer", NULL);

    /* on test buffer: text to buffer */
    TEST_INPUT_DATA(WEECHAT_RC_OK, buffer, "test", NULL, 0, 0);
    RECORD_CHECK_NO_MSG();

    /* on core buffer: text to buffer (with two command chars) */
    TEST_INPUT_DATA(WEECHAT_RC_ERROR, gui_buffers, "//test", NULL, 0, 0);
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "You cannot write text in this buffer", NULL);

    /* on test buffer: text to buffer (with two command chars) */
    TEST_INPUT_DATA(WEECHAT_RC_OK, buffer, "//test", NULL, 0, 0);
    RECORD_CHECK_NO_MSG();

    /* on core buffer: valid command */
    TEST_INPUT_DATA(WEECHAT_RC_OK, gui_buffers, "/print core\n/print line2", NULL, 0, 0);
    RECORD_CHECK_MSG("core.weechat", "", "core", NULL);
    LONGS_EQUAL(1, record_count_messages ());

    /* on test buffer: valid command */
    TEST_INPUT_DATA(WEECHAT_RC_OK, buffer, "/print test\n/print line2", NULL, 0, 0);
    RECORD_CHECK_MSG("core.test", "", "test", NULL);
    LONGS_EQUAL(1, record_count_messages ());

    /* on core buffer: forbidden command */
    TEST_INPUT_DATA(WEECHAT_RC_ERROR, gui_buffers, "/print core\n/print line2", "*,!print", 0, 0);
    RECORD_CHECK_NO_MSG();

    /* on test buffer: forbidden command */
    TEST_INPUT_DATA(WEECHAT_RC_ERROR, buffer, "/print test\n/print line2", "*,!print", 0, 0);
    RECORD_CHECK_NO_MSG();

    /* on core buffer: valid command with split_newline */
    TEST_INPUT_DATA(WEECHAT_RC_OK, gui_buffers, "/print core\n/print line2", NULL, 1, 0);
    RECORD_CHECK_MSG("core.weechat", "", "core", NULL);
    RECORD_CHECK_MSG("core.weechat", "", "line2", NULL);
    LONGS_EQUAL(2, record_count_messages ());

    /* on test buffer: valid command with split_newline */
    TEST_INPUT_DATA(WEECHAT_RC_OK, buffer, "/print test\n/print line2", NULL, 1, 0);
    RECORD_CHECK_MSG("core.test", "", "test", NULL);
    RECORD_CHECK_MSG("core.test", "", "line2", NULL);
    LONGS_EQUAL(2, record_count_messages ());

    /* on core buffer: valid command but with commands disabled */
    TEST_INPUT_DATA(WEECHAT_RC_ERROR, gui_buffers, "/print core\n/print line2", "-", 0, 0);
    RECORD_CHECK_MSG("core.weechat", "=!=", "You cannot write text in this buffer", NULL);
    LONGS_EQUAL(1, record_count_messages ());

    /* on test buffer: valid command but with commands disabled */
    TEST_INPUT_DATA(WEECHAT_RC_OK, buffer, "/print core\n/print line2", "-", 0, 0);
    RECORD_CHECK_NO_MSG();

    gui_buffer_close (buffer);
}

/*
 * Tests functions:
 *   input_data_timer_cb
 */

TEST(CoreInput, DataTimerCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   input_data_delayed
 */

TEST(CoreInput, DataDelayed)
{
    /* TODO: write tests */
}
