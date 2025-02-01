/*
 * test-logger.cpp - test logger functions
 *
 * Copyright (C) 2023-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/gui/gui-buffer.h"
#include "src/plugins/logger/logger.h"
}

TEST_GROUP(Logger)
{
};

/*
 * Tests functions:
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
 * Tests functions:
 *   logger_get_file_path
 */

TEST(Logger, GetFilePath)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   logger_create_directory
 */

TEST(Logger, CreateDirectory)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   logger_build_option_name
 */

TEST(Logger, BuildOptionName)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   logger_get_level_for_buffer
 */

TEST(Logger, GetLevelForBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   logger_get_mask_for_buffer
 */

TEST(Logger, GetMaskForBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   logger_get_mask_expanded
 */

TEST(Logger, GetMaskExpanded)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   logger_get_filename
 */

TEST(Logger, GetFilename)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   logger_buffer_opened_signal_cb
 */

TEST(Logger, BufferOpenedSignalCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   logger_buffer_closing_signal_cb
 */

TEST(Logger, BufferClosingSignalCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   logger_buffer_renamed_signal_cb
 */

TEST(Logger, BufferRenamedSignalCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   logger_start_signal_cb
 */

TEST(Logger, StartSignalCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   logger_stop_signal_cb
 */

TEST(Logger, StopSignalCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   logger_day_changed_signal_cb
 */

TEST(Logger, DayChangedSignalCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   logger_get_line_tag_info
 */

TEST(Logger, GetLineTagInfo)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   logger_print_cb
 */

TEST(Logger, PrintCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   logger_timer_cb
 */

TEST(Logger, TimerCb)
{
    /* TODO: write tests */
}
