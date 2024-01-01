/*
 * test-xfer-network.cpp - test xfer network functions
 *
 * Copyright (C) 2022-2024 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CppUTest/TestHarness.h"

#include "tests/tests.h"

extern "C"
{
#include "src/plugins/xfer/xfer-network.h"

extern char *xfer_network_convert_integer_to_ipv4 (const char *str_address);
}

TEST_GROUP(XferNetwork)
{
};

/*
 * Tests functions:
 *   xfer_network_convert_integer_to_ipv4
 */

TEST(XferNetwork, ConvertIntegerToIpv4)
{
    char *str;

    POINTERS_EQUAL(NULL, xfer_network_convert_integer_to_ipv4 (NULL));
    POINTERS_EQUAL(NULL, xfer_network_convert_integer_to_ipv4 (""));
    POINTERS_EQUAL(NULL, xfer_network_convert_integer_to_ipv4 ("abc"));
    POINTERS_EQUAL(NULL, xfer_network_convert_integer_to_ipv4 ("0"));
    POINTERS_EQUAL(NULL, xfer_network_convert_integer_to_ipv4 ("-1"));

    /* too big: UINT32_MAX + 1 = 4294967296 */
    POINTERS_EQUAL(NULL, xfer_network_convert_integer_to_ipv4 ("4294967296"));

    WEE_TEST_STR("0.0.0.1", xfer_network_convert_integer_to_ipv4 ("1"));
    WEE_TEST_STR("0.0.1.0", xfer_network_convert_integer_to_ipv4 ("256"));
    WEE_TEST_STR("0.1.0.0", xfer_network_convert_integer_to_ipv4 ("65536"));
    WEE_TEST_STR("1.0.0.0", xfer_network_convert_integer_to_ipv4 ("16777216"));
    WEE_TEST_STR("127.0.0.1", xfer_network_convert_integer_to_ipv4 ("2130706433"));
    WEE_TEST_STR("192.168.1.2", xfer_network_convert_integer_to_ipv4 ("3232235778"));
}
