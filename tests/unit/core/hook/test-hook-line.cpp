/*
 * test-hook-line.cpp - test hook line functions
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

TEST_GROUP(HookLine)
{
};

/*
 * Tests functions:
 *   hook_line_get_description
 */

TEST(HookLine, GetDescription)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_line
 */

TEST(HookLine, Line)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_line_exec
 */

TEST(HookLine, Exec)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_line_free_data
 */

TEST(HookLine, FreeData)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_line_add_to_infolist
 */

TEST(HookLine, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_line_print_log
 */

TEST(HookLine, PrintLog)
{
    /* TODO: write tests */
}
