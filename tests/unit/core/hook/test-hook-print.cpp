/*
 * test-hook-print.cpp - test hook print functions
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

TEST_GROUP(HookPrint)
{
};

/*
 * Tests functions:
 *   hook_print_get_description
 */

TEST(HookPrint, GetDescription)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_print
 */

TEST(HookPrint, Print)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_print_exec
 */

TEST(HookPrint, Exec)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_print_free_data
 */

TEST(HookPrint, FreeData)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_print_add_to_infolist
 */

TEST(HookPrint, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_print_print_log
 */

TEST(HookPrint, PrintLog)
{
    /* TODO: write tests */
}
