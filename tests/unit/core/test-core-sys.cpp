/*
 * SPDX-FileCopyrightText: 2023-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test system functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

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
 * Test functions:
 *   sys_setrlimit_resource
 */

TEST(CoreSys, SetrlimitResource)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   sys_setrlimit
 */

TEST(CoreSys, Setrlimit)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   sys_display_rlimit
 */

TEST(CoreSys, DisplayRlimit)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   sys_display_rusage
 */

TEST(CoreSys, DisplayRusage)
{
    /* TODO: write tests */
}
