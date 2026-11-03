/*
 * SPDX-FileCopyrightText: 2024-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test input functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"
#include "tests-record.h"

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
#include "src/core/core-config.h"
#include "src/core/core-config-file.h"
#include "src/core/core-input.h"
#include "src/core/core-string.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-window.h"
#include "src/plugins/plugin.h"
}

TEST_GROUP(CoreInput)
{
};

/*
 * Test functions:
 *   input_exec_data
 */

TEST(CoreInput, ExecData)
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
    /* Make C++ compiler happy. */
    (void) pointer;
    (void) data;
    (void) buffer;
    (void) input_data;

    return WEECHAT_RC_OK;
}

/*
 * Test functions:
 *   input_exec_command
 */

TEST(CoreInput, ExecCommand)
{
    struct t_gui_buffer *buffer;
    char str_command[64], str_error[128];

    buffer = gui_buffer_new (NULL, "test",
                             &test_core_input_buffer_input_cb, NULL, NULL,
                             NULL, NULL, NULL);
    CHECK(buffer);

    snprintf (str_command, sizeof (str_command), "/%d", buffer->number);

    LONGS_EQUAL(WEECHAT_RC_ERROR,
                input_exec_command (gui_buffers, 1, NULL, NULL, NULL));
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                input_exec_command (gui_buffers, 1, NULL, "", NULL));

    POINTERS_EQUAL(gui_buffers, gui_current_window->buffer);

    /* Jump to buffer number. */
    LONGS_EQUAL(WEECHAT_RC_OK,
                input_exec_command (gui_buffers, 1, NULL, str_command, NULL));
    POINTERS_EQUAL(buffer, gui_current_window->buffer);
    LONGS_EQUAL(WEECHAT_RC_OK,
                input_exec_command (buffer, 1, NULL, "/1", NULL));
    POINTERS_EQUAL(gui_buffers, gui_current_window->buffer);

    /* Jump to buffer number: multiple numbers, the last one is used. */
    snprintf (str_command, sizeof (str_command), "/1/%d", buffer->number);
    LONGS_EQUAL(WEECHAT_RC_OK,
                input_exec_command (gui_buffers, 1, NULL, str_command, NULL));
    POINTERS_EQUAL(buffer, gui_current_window->buffer);
    snprintf (str_command, sizeof (str_command), "/%d/1", buffer->number);
    LONGS_EQUAL(WEECHAT_RC_OK,
                input_exec_command (buffer, 1, NULL, str_command, NULL));
    POINTERS_EQUAL(gui_buffers, gui_current_window->buffer);

    /* Jump to buffer number with custom command char. */
    config_file_option_set (config_look_command_chars, "%", 1);
    snprintf (str_command, sizeof (str_command), "%%%d", buffer->number);
    LONGS_EQUAL(WEECHAT_RC_OK,
                input_exec_command (gui_buffers, 1, NULL, str_command, NULL));
    POINTERS_EQUAL(buffer, gui_current_window->buffer);
    snprintf (str_command, sizeof (str_command), "%%%d%%1", buffer->number);
    LONGS_EQUAL(WEECHAT_RC_OK,
                input_exec_command (buffer, 1, NULL, str_command, NULL));
    POINTERS_EQUAL(gui_buffers, gui_current_window->buffer);
    config_file_option_reset (config_look_command_chars, 1);

    snprintf (str_command, sizeof (str_command), "/%d", buffer->number);

    /* Jump to buffer number with command "buffer" forbidden. */
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                input_exec_command (gui_buffers, 1, NULL, str_command,
                                    "*,!buffer"));
    POINTERS_EQUAL(gui_buffers, gui_current_window->buffer);

    /* Buffer number with arguments: unknown command. */
    snprintf (str_command, sizeof (str_command), "/%d test", buffer->number);
    snprintf (str_error, sizeof (str_error),
              "^Unknown command \"%d\" \\(type /help for help\\)",
              buffer->number);
    record_start ();
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                input_exec_command (gui_buffers, 1, NULL, str_command, NULL));
    record_stop ();
    RECORD_CHECK_MSG_REGEX("core.weechat", str_error);
    POINTERS_EQUAL(gui_buffers, gui_current_window->buffer);

    /* Jump to buffer number disabled: unknown command. */
    config_file_option_set (config_look_command_buffer_number, "off", 1);
    snprintf (str_command, sizeof (str_command), "/%d", buffer->number);
    record_start ();
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                input_exec_command (gui_buffers, 1, NULL, str_command, NULL));
    record_stop ();
    RECORD_CHECK_MSG_REGEX("core.weechat", str_error);
    POINTERS_EQUAL(gui_buffers, gui_current_window->buffer);
    config_file_option_reset (config_look_command_buffer_number, 1);

    gui_buffer_close (buffer);
}

/*
 * Test functions:
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

    /* On core buffer: command not found */
    TEST_INPUT_DATA(WEECHAT_RC_ERROR, gui_buffers, "/xxx", NULL, 0, 0);
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "Unknown command \"xxx\" (type /help for help), "
                     "commands with similar name: -", NULL);

    /* On test buffer: command not found. */
    TEST_INPUT_DATA(WEECHAT_RC_ERROR, buffer, "/xxx", NULL, 0, 0);
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "Unknown command \"xxx\" (type /help for help), "
                     "commands with similar name: -", NULL);

    /* On core buffer: command not found, but user_data == 1 */
    TEST_INPUT_DATA(WEECHAT_RC_ERROR, gui_buffers, "/xxx", NULL, 0, 1);
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "Unknown command \"xxx\" (type /help for help), "
                     "commands with similar name: -", NULL);

    /* On test buffer: command not found, but user_data == 1. */
    TEST_INPUT_DATA(WEECHAT_RC_OK, buffer, "/xxx", NULL, 0, 1);
    RECORD_CHECK_NO_MSG();

    /* On core buffer: empty text to buffer. */
    TEST_INPUT_DATA(WEECHAT_RC_ERROR, gui_buffers, "", NULL, 0, 0);
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "You cannot write text in this buffer", NULL);

    /* On test buffer: empty text to buffer. */
    TEST_INPUT_DATA(WEECHAT_RC_OK, buffer, "", NULL, 0, 0);
    RECORD_CHECK_NO_MSG();

    /* On core buffer: text to buffer */
    TEST_INPUT_DATA(WEECHAT_RC_ERROR, gui_buffers, "test", NULL, 0, 0);
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "You cannot write text in this buffer", NULL);

    /* On test buffer: text to buffer. */
    TEST_INPUT_DATA(WEECHAT_RC_OK, buffer, "test", NULL, 0, 0);
    RECORD_CHECK_NO_MSG();

    /* On core buffer: text to buffer (with two command chars) */
    TEST_INPUT_DATA(WEECHAT_RC_ERROR, gui_buffers, "//test", NULL, 0, 0);
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "You cannot write text in this buffer", NULL);

    /* On test buffer: text to buffer (with two command chars). */
    TEST_INPUT_DATA(WEECHAT_RC_OK, buffer, "//test", NULL, 0, 0);
    RECORD_CHECK_NO_MSG();

    /* On core buffer: valid command */
    TEST_INPUT_DATA(WEECHAT_RC_OK, gui_buffers, "/print core\n/print line2", NULL, 0, 0);
    RECORD_CHECK_MSG("core.weechat", "", "core", NULL);
    LONGS_EQUAL(1, record_count_messages ());

    /* On test buffer: valid command. */
    TEST_INPUT_DATA(WEECHAT_RC_OK, buffer, "/print test\n/print line2", NULL, 0, 0);
    RECORD_CHECK_MSG("core.test", "", "test", NULL);
    LONGS_EQUAL(1, record_count_messages ());

    /* On core buffer: forbidden command */
    TEST_INPUT_DATA(WEECHAT_RC_ERROR, gui_buffers, "/print core\n/print line2", "*,!print", 0, 0);
    RECORD_CHECK_NO_MSG();

    /* On test buffer: forbidden command. */
    TEST_INPUT_DATA(WEECHAT_RC_ERROR, buffer, "/print test\n/print line2", "*,!print", 0, 0);
    RECORD_CHECK_NO_MSG();

    /* On core buffer: valid command with split_newline. */
    TEST_INPUT_DATA(WEECHAT_RC_OK, gui_buffers, "/print core\n/print line2", NULL, 1, 0);
    RECORD_CHECK_MSG("core.weechat", "", "core", NULL);
    RECORD_CHECK_MSG("core.weechat", "", "line2", NULL);
    LONGS_EQUAL(2, record_count_messages ());

    /* On test buffer: valid command with split_newline. */
    TEST_INPUT_DATA(WEECHAT_RC_OK, buffer, "/print test\n/print line2", NULL, 1, 0);
    RECORD_CHECK_MSG("core.test", "", "test", NULL);
    RECORD_CHECK_MSG("core.test", "", "line2", NULL);
    LONGS_EQUAL(2, record_count_messages ());

    /* On core buffer: valid command but with commands disabled. */
    TEST_INPUT_DATA(WEECHAT_RC_ERROR, gui_buffers, "/print core\n/print line2", "-", 0, 0);
    RECORD_CHECK_MSG("core.weechat", "=!=", "You cannot write text in this buffer", NULL);
    LONGS_EQUAL(1, record_count_messages ());

    /* On test buffer: valid command but with commands disabled. */
    TEST_INPUT_DATA(WEECHAT_RC_OK, buffer, "/print core\n/print line2", "-", 0, 0);
    RECORD_CHECK_NO_MSG();

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   input_data (jump to buffer number)
 */

TEST(CoreInput, DataBufferNumber)
{
    struct t_gui_buffer *buffer;
    char str_command[64];

    buffer = gui_buffer_new (NULL, "test",
                             &test_core_input_buffer_input_cb, NULL, NULL,
                             NULL, NULL, NULL);
    CHECK(buffer);

    POINTERS_EQUAL(gui_buffers, gui_current_window->buffer);

    snprintf (str_command, sizeof (str_command), "/%d", buffer->number);
    TEST_INPUT_DATA(WEECHAT_RC_OK, gui_buffers, str_command, NULL, 0, 1);
    RECORD_CHECK_NO_MSG();
    POINTERS_EQUAL(buffer, gui_current_window->buffer);

    TEST_INPUT_DATA(WEECHAT_RC_OK, buffer, "/1", NULL, 0, 1);
    RECORD_CHECK_NO_MSG();
    POINTERS_EQUAL(gui_buffers, gui_current_window->buffer);

    /* Multiple numbers: not considered as a path, the last one is used. */
    snprintf (str_command, sizeof (str_command), "/1/%d", buffer->number);
    TEST_INPUT_DATA(WEECHAT_RC_OK, gui_buffers, str_command, NULL, 0, 1);
    RECORD_CHECK_NO_MSG();
    POINTERS_EQUAL(buffer, gui_current_window->buffer);

    snprintf (str_command, sizeof (str_command), "/%d/1", buffer->number);
    TEST_INPUT_DATA(WEECHAT_RC_OK, buffer, str_command, NULL, 0, 1);
    RECORD_CHECK_NO_MSG();
    POINTERS_EQUAL(gui_buffers, gui_current_window->buffer);

    /* Custom command char. */
    config_file_option_set (config_look_command_chars, "%", 1);
    snprintf (str_command, sizeof (str_command), "%%1%%%d", buffer->number);
    TEST_INPUT_DATA(WEECHAT_RC_OK, gui_buffers, str_command, NULL, 0, 1);
    RECORD_CHECK_NO_MSG();
    POINTERS_EQUAL(buffer, gui_current_window->buffer);
    snprintf (str_command, sizeof (str_command), "%%%d%%1", buffer->number);
    TEST_INPUT_DATA(WEECHAT_RC_OK, buffer, str_command, NULL, 0, 1);
    RECORD_CHECK_NO_MSG();
    POINTERS_EQUAL(gui_buffers, gui_current_window->buffer);
    config_file_option_reset (config_look_command_chars, 1);

    /* Option disabled: "/1/N" is a path sent to the buffer. */
    config_file_option_set (config_look_command_buffer_number, "off", 1);
    snprintf (str_command, sizeof (str_command), "/1/%d", buffer->number);
    TEST_INPUT_DATA(WEECHAT_RC_ERROR, gui_buffers, str_command, NULL, 0, 1);
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "You cannot write text in this buffer", NULL);
    POINTERS_EQUAL(gui_buffers, gui_current_window->buffer);
    config_file_option_reset (config_look_command_buffer_number, 1);

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   input_data_timer_cb
 */

TEST(CoreInput, DataTimerCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   input_data_delayed
 */

TEST(CoreInput, DataDelayed)
{
    /* TODO: write tests */
}
