/*
 * SPDX-FileCopyrightText: 2026 Thomas Matthijs <github@selckin.be>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test exec functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <string.h>
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-line.h"
#include "src/plugins/weechat-plugin.h"
#include "src/plugins/exec/exec.h"
#include "src/plugins/exec/exec-config.h"

extern char *exec_decode_color (struct t_exec_cmd *exec_cmd,
                                struct t_gui_buffer *buffer,
                                const char *string);
extern void exec_display_line (struct t_exec_cmd *exec_cmd,
                               struct t_gui_buffer *buffer,
                               int out, const char *line);
extern void exec_end_command (struct t_exec_cmd *exec_cmd, int return_code);
}

#define WEE_CHECK_DECODE_COLOR(__result, __exec_cmd, __buffer, __string) \
    result = exec_decode_color (__exec_cmd, __buffer, __string);        \
    STRCMP_EQUAL(__result, result);                                     \
    free (result);

/* Output of the command received with a hsignal. */
char *test_exec_hsignal_out = NULL;

/*
 * Callback for hsignal "test_exec": save the output of the command.
 */

int
test_exec_hsignal_cb (const void *pointer, void *data, const char *signal,
                      struct t_hashtable *hashtable)
{
    const char *ptr_out;

    /* Make C++ compiler happy. */
    (void) pointer;
    (void) data;
    (void) signal;

    free (test_exec_hsignal_out);
    ptr_out = (const char *)hashtable_get (hashtable, "out");
    test_exec_hsignal_out = (ptr_out) ? strdup (ptr_out) : NULL;

    return WEECHAT_RC_OK;
}

TEST_GROUP(Exec)
{
};

/*
 * Test functions:
 *   exec_decode_color
 */

TEST(Exec, DecodeColor)
{
    struct t_exec_cmd *exec_cmd;
    char *result;

    exec_cmd = exec_add ();
    CHECK(exec_cmd);

    exec_cmd->color = EXEC_COLOR_ANSI;
    POINTERS_EQUAL(NULL, exec_decode_color (exec_cmd, NULL, NULL));

    /* Colors kept as-is. */
    WEE_CHECK_DECODE_COLOR("\x1b[31m" "red", exec_cmd, NULL, "\x1b[31m" "red");

    /* Colors removed. */
    exec_cmd->color = EXEC_COLOR_STRIP;
    WEE_CHECK_DECODE_COLOR("red", exec_cmd, NULL, "\x1b[31m" "red");

    /* WeeChat colors. */
    exec_cmd->color = EXEC_COLOR_WEECHAT;
    WEE_CHECK_DECODE_COLOR("\x19" "F|03" "red", exec_cmd, NULL,
                           "\x1b[31m" "red");
    exec_cmd->color = EXEC_COLOR_AUTO;
    WEE_CHECK_DECODE_COLOR("\x19" "F|03" "red", exec_cmd, NULL,
                           "\x1b[31m" "red");

    /* IRC colors, without buffer. */
    exec_cmd->color = EXEC_COLOR_IRC;
    WEE_CHECK_DECODE_COLOR("\x03" "05" "red", exec_cmd, NULL,
                           "\x1b[31m" "red");
    exec_cmd->color = EXEC_COLOR_AUTO;
    exec_cmd->output_to_buffer = 1;
    WEE_CHECK_DECODE_COLOR("\x03" "05" "red", exec_cmd, NULL,
                           "\x1b[31m" "red");

    /* IRC colors, buffer given to the IRC plugin. */
    WEE_CHECK_DECODE_COLOR("\x03" "05" "red", exec_cmd, gui_buffers,
                           "\x1b[31m" "red");
    exec_cmd->color = EXEC_COLOR_STRIP;
    WEE_CHECK_DECODE_COLOR("red", exec_cmd, gui_buffers, "\x1b[31m" "red");

    exec_free (exec_cmd);
}

/*
 * Test functions:
 *   exec_display_line
 */

TEST(Exec, DisplayLine)
{
    struct t_exec_cmd *exec_cmd;

    exec_cmd = exec_add ();
    CHECK(exec_cmd);

    /*
     * Output piped to a command, run on the current buffer (colors decoded
     * with the current buffer).
     */
    exec_cmd->color = EXEC_COLOR_IRC;
    exec_cmd->pipe_command = strdup ("/print -core test_exec:");
    exec_display_line (exec_cmd, NULL, EXEC_STDOUT, "\x1b[31m" "red");
    STRCMP_EQUAL("test_exec: \x03" "05" "red",
                 gui_buffers->own_lines->last_line->data->message);

    exec_free (exec_cmd);
}

/*
 * Test functions:
 *   exec_end_command
 */

TEST(Exec, EndCommand)
{
    struct t_exec_cmd *exec_cmd;
    struct t_hook *hook;

    hook = hook_hsignal (NULL, "test_exec", &test_exec_hsignal_cb, NULL, NULL);
    CHECK(hook);

    /* Do not remove the command with a timer. */
    config_file_option_set (exec_config_command_purge_delay, "-1", 1);

    /* Output sent with a hsignal, colors decoded without buffer. */
    exec_cmd = exec_add ();
    CHECK(exec_cmd);
    exec_cmd->command = strdup ("test");
    exec_cmd->color = EXEC_COLOR_IRC;
    exec_cmd->hsignal = strdup ("test_exec");
    exec_cmd->output[EXEC_STDOUT] = strdup ("\x1b[31m" "red");
    exec_cmd->output_size[EXEC_STDOUT] = strlen (exec_cmd->output[EXEC_STDOUT]);
    exec_end_command (exec_cmd, 0);
    STRCMP_EQUAL("\x03" "05" "red", test_exec_hsignal_out);
    LONGS_EQUAL(0, exec_cmd->return_code);
    POINTERS_EQUAL(NULL, exec_cmd->output[EXEC_STDOUT]);
    exec_free (exec_cmd);

    config_file_option_reset (exec_config_command_purge_delay, 1);
    unhook (hook);
    free (test_exec_hsignal_out);
    test_exec_hsignal_out = NULL;
}
