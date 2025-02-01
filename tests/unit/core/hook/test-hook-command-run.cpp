/*
 * test-hook-command-run.cpp - test hook command run functions
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
}

TEST_GROUP(HookCommandRun)
{
};

/*
 * Tests functions:
 *   hook_command_run_get_description
 */

TEST(HookCommandRun, GetDescription)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_command_run
 */

TEST(HookCommandRun, CommandRun)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_command_run_exec
 */

TEST(HookCommandRun, Exec)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_command_run_free_data
 */

TEST(HookCommandRun, FreeData)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_command_run_add_to_infolist
 */

TEST(HookCommandRun, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_command_run_print_log
 */

TEST(HookCommandRun, PrintLog)
{
    /* TODO: write tests */
}
