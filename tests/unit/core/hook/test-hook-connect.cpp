/*
 * test-hook-connect.cpp - test hook connect functions
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

TEST_GROUP(HookConnect)
{
};

/*
 * Tests functions:
 *   hook_connect_get_description
 */

TEST(HookConnect, GetDescription)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_connect
 */

TEST(HookConnect, Connect)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_connect_gnutls_verify_certificates
 */

TEST(HookConnect, GnutlsVerifyCertificates)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_connect_gnutls_set_certificates
 */

TEST(HookConnect, GnutlsSetCertificates)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_connect_free_data
 */

TEST(HookConnect, FreeData)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_connect_add_to_infolist
 */

TEST(HookConnect, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hook_connect_print_log
 */

TEST(HookConnect, PrintLog)
{
    /* TODO: write tests */
}
