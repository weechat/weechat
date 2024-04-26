/*
 * test-hook-timer.cpp - test hook timer functions
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

TEST_GROUP(HookTimer)
{
};

/*
 * Tests functions:
 *   hook_timer_get_description
 */

TEST(HookTimer, GetDescription)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_timer_init
 */

TEST(HookTimer, Init)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_timer
 */

TEST(HookTimer, Timer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_timer_check_system_clock
 */

TEST(HookTimer, CheckSystemClock)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_timer_get_time_to_next
 */

TEST(HookTimer, GetTimeToNext)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_timer_exec
 */

TEST(HookTimer, Exec)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_timer_free_data
 */

TEST(HookTimer, FreeData)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_timer_add_to_infolist
 */

TEST(HookTimer, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_timer_print_log
 */

TEST(HookTimer, PrintLog)
{
    /* TODO: write tests */
}
