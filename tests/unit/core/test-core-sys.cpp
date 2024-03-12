/*
 * test-core-sys.cpp - test system functions
 *
 * Copyright (C) 2023-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "src/core/core-string.h"
#include "src/core/core-util.h"
}

TEST_GROUP(CoreSys)
{
};

/*
 * Tests functions:
 *   sys_setrlimit_resource
 */

TEST(CoreSys, SetrlimitResource)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   sys_setrlimit
 */

TEST(CoreSys, Setrlimit)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   sys_display_rlimit
 */

TEST(CoreSys, DisplayRlimit)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   sys_display_rusage
 */

TEST(CoreSys, DisplayRusage)
{
    /* TODO: write tests */
}
