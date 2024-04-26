/*
 * test-hook-config.cpp - test hook config functions
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

TEST_GROUP(HookConfig)
{
};

/*
 * Tests functions:
 *   hook_config_get_description
 */

TEST(HookConfig, GetDescription)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_config
 */

TEST(HookConfig, Config)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_config_exec
 */

TEST(HookConfig, Exec)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_config_free_data
 */

TEST(HookConfig, FreeData)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_config_add_to_infolist
 */

TEST(HookConfig, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_config_print_log
 */

TEST(HookConfig, PrintLog)
{
    /* TODO: write tests */
}
