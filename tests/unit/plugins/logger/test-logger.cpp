/*
 * SPDX-FileCopyrightText: 2023-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test logger functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include "src/gui/gui-buffer.h"
#include "src/plugins/logger/logger.h"

extern char *logger_get_mask_expanded (struct t_gui_buffer *buffer,
                                       const char *mask);
}

TEST_GROUP(Logger)
{
};

/*
 * Test functions:
 *   logger_check_conditions
 */

TEST(Logger, CheckConditions)
{
    LONGS_EQUAL(0, logger_check_conditions (NULL, NULL));
    LONGS_EQUAL(0, logger_check_conditions (NULL, ""));
    LONGS_EQUAL(0, logger_check_conditions (NULL, "1"));

    LONGS_EQUAL(1, logger_check_conditions (gui_buffers, NULL));
    LONGS_EQUAL(1, logger_check_conditions (gui_buffers, ""));
    LONGS_EQUAL(1, logger_check_conditions (gui_buffers, "1"));
    LONGS_EQUAL(1, logger_check_conditions (gui_buffers, "${name} == weechat"));

    LONGS_EQUAL(0, logger_check_conditions (gui_buffers, "0"));
    LONGS_EQUAL(0, logger_check_conditions (gui_buffers, "${name} == test"));
}

/*
 * Test functions:
 *   logger_get_file_path
 */

TEST(Logger, GetFilePath)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   logger_create_directory
 */

TEST(Logger, CreateDirectory)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   logger_build_option_name
 */

TEST(Logger, BuildOptionName)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   logger_get_level_for_buffer
 */

TEST(Logger, GetLevelForBuffer)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   logger_get_mask_for_buffer
 */

TEST(Logger, GetMaskForBuffer)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   logger_get_mask_expanded
 */

TEST(Logger, GetMaskExpanded)
{
    char *str;

    /* Empty mask */
    WEE_TEST_STR("", logger_get_mask_expanded (gui_buffers, ""));

    /* Mask without any special char */
    WEE_TEST_STR("test.weechatlog",
                 logger_get_mask_expanded (gui_buffers, "test.weechatlog"));

    /* Local variable of buffer is expanded (buffer "name" == "weechat") */
    WEE_TEST_STR("weechat.weechatlog",
                 logger_get_mask_expanded (gui_buffers, "$name.weechatlog"));

    /* Directory separators of the mask itself are kept as directory levels. */
    WEE_TEST_STR("dir1/dir2/weechat.weechatlog",
                 logger_get_mask_expanded (gui_buffers,
                                           "dir1/dir2/$name.weechatlog"));

    gui_buffer_set (gui_buffers, "localvar_set_testmask", "aaa/bbb");

    /*
     * A directory separator inside a local variable value is replaced and can
     * not add a directory level to the path.
     */
    WEE_TEST_STR("aaa_bbb.weechatlog",
                 logger_get_mask_expanded (gui_buffers,
                                           "$testmask.weechatlog"));

    /*
     * Path traversal: the char used internally to protect directory separators
     * (0x01) must not be turned into a directory separator when it comes from a
     * local variable value (it is kept as-is in the file name).
     */
    gui_buffer_set (gui_buffers, "localvar_set_testmask",
                    "aaa\001..\001..\001bbb");
    WEE_TEST_STR("aaa\001..\001..\001bbb.weechatlog",
                 logger_get_mask_expanded (gui_buffers,
                                           "$testmask.weechatlog"));

    gui_buffer_set (gui_buffers, "localvar_del_testmask", "");
}

/*
 * Test functions:
 *   logger_get_filename
 */

TEST(Logger, GetFilename)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   logger_buffer_opened_signal_cb
 */

TEST(Logger, BufferOpenedSignalCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   logger_buffer_closing_signal_cb
 */

TEST(Logger, BufferClosingSignalCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   logger_buffer_renamed_signal_cb
 */

TEST(Logger, BufferRenamedSignalCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   logger_start_signal_cb
 */

TEST(Logger, StartSignalCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   logger_stop_signal_cb
 */

TEST(Logger, StopSignalCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   logger_day_changed_signal_cb
 */

TEST(Logger, DayChangedSignalCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   logger_get_line_tag_info
 */

TEST(Logger, GetLineTagInfo)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   logger_print_cb
 */

TEST(Logger, PrintCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   logger_timer_cb
 */

TEST(Logger, TimerCb)
{
    /* TODO: write tests */
}
