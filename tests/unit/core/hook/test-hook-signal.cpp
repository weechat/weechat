/*
 * test-hook-signal.cpp - test hook signal functions
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

extern "C"
{
#include "src/core/weechat.h"
#include "src/core/hook/hook-signal.h"
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
 *   hook_signal_extract_flags
 */

TEST(HookSignal, ExtractFlags)
{
    const char *ptr_signal;
    char signal[128];
    int stop_on_error, ignore_eat;

    hook_signal_extract_flags (NULL, NULL, NULL, NULL);

    /* no flags */
    snprintf (signal, sizeof (signal), "test");
    stop_on_error = -1;
    ignore_eat = -1;
    hook_signal_extract_flags (signal, &ptr_signal, &stop_on_error, &ignore_eat);
    STRCMP_EQUAL("test", ptr_signal);
    LONGS_EQUAL(0, stop_on_error);
    LONGS_EQUAL(0, ignore_eat);

    /* invalid flags (missing "]") */
    snprintf (signal, sizeof (signal), "[flags:test");
    stop_on_error = -1;
    ignore_eat = -1;
    hook_signal_extract_flags (signal, &ptr_signal, &stop_on_error, &ignore_eat);
    STRCMP_EQUAL("[flags:test", ptr_signal);
    LONGS_EQUAL(0, stop_on_error);
    LONGS_EQUAL(0, ignore_eat);

    /* unknown flag */
    snprintf (signal, sizeof (signal), "[flags:flag1]test");
    stop_on_error = -1;
    ignore_eat = -1;
    hook_signal_extract_flags (signal, &ptr_signal, &stop_on_error, &ignore_eat);
    STRCMP_EQUAL("test", ptr_signal);
    LONGS_EQUAL(0, stop_on_error);
    LONGS_EQUAL(0, ignore_eat);

    /* unknown flags */
    snprintf (signal, sizeof (signal), "[flags:flag1,flag2]test");
    stop_on_error = -1;
    ignore_eat = -1;
    hook_signal_extract_flags (signal, &ptr_signal, &stop_on_error, &ignore_eat);
    STRCMP_EQUAL("test", ptr_signal);
    LONGS_EQUAL(0, stop_on_error);
    LONGS_EQUAL(0, ignore_eat);

    /* flag "stop_on_error" */
    snprintf (signal, sizeof (signal), "[flags:stop_on_error]test");
    stop_on_error = -1;
    ignore_eat = -1;
    hook_signal_extract_flags (signal, &ptr_signal, &stop_on_error, &ignore_eat);
    STRCMP_EQUAL("test", ptr_signal);
    LONGS_EQUAL(1, stop_on_error);
    LONGS_EQUAL(0, ignore_eat);

    /* flag "ignore_eat" */
    snprintf (signal, sizeof (signal), "[flags:ignore_eat]test");
    stop_on_error = -1;
    ignore_eat = -1;
    hook_signal_extract_flags (signal, &ptr_signal, &stop_on_error, &ignore_eat);
    STRCMP_EQUAL("test", ptr_signal);
    LONGS_EQUAL(0, stop_on_error);
    LONGS_EQUAL(1, ignore_eat);

    /* flags "stop_on_error" and "ignore_eat" */
    snprintf (signal, sizeof (signal), "[flags:stop_on_error,ignore_eat]test");
    stop_on_error = -1;
    ignore_eat = -1;
    hook_signal_extract_flags (signal, &ptr_signal, &stop_on_error, &ignore_eat);
    STRCMP_EQUAL("test", ptr_signal);
    LONGS_EQUAL(1, stop_on_error);
    LONGS_EQUAL(1, ignore_eat);
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
