/*
 * test-hook-fd.cpp - test hook fd functions
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

TEST_GROUP(HookFd)
{
};

/*
 * Tests functions:
 *   hook_fd_get_description
 */

TEST(HookFd, GetDescription)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_fd_search
 */

TEST(HookFd, Search)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_fd_realloc_pollfd
 */

TEST(HookFd, ReallocPollfd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_fd_add_cb
 */

TEST(HookFd, AddCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_fd_remove_cb
 */

TEST(HookFd, RemoveCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_fd
 */

TEST(HookFd, Fd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_fd_exec
 */

TEST(HookFd, Exec)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_fd_free_data
 */

TEST(HookFd, FreeData)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_fd_add_to_infolist
 */

TEST(HookFd, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_fd_print_log
 */

TEST(HookFd, PrintLog)
{
    /* TODO: write tests */
}
