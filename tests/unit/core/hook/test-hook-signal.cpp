/*
 * test-hook-signal.cpp - test hook signal functions
 *
 * Copyright (C) 2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/core/weechat.h"
}

TEST_GROUP(HookSignal)
{
};

/*
 * Tests functions:
 *   hook_signal_get_description
 */

TEST(HookSignal, GetDescription)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_signal
 */

TEST(HookSignal, Signal)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_signal_match
 */

TEST(HookSignal, Match)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_signal_send
 */

TEST(HookSignal, Send)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_signal_free_data
 */

TEST(HookSignal, FreeData)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_signal_add_to_infolist
 */

TEST(HookSignal, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_signal_print_log
 */

TEST(HookSignal, PrintLog)
{
    /* TODO: write tests */
}
