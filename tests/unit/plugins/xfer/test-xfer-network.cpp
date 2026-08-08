/*
 * SPDX-FileCopyrightText: 2022-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test xfer network functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include "src/plugins/xfer/xfer-network.h"

extern char *xfer_network_convert_integer_to_ipv4 (const char *str_address);
}

TEST_GROUP(XferNetwork)
{
};

/*
 * Test functions:
 *   xfer_network_convert_integer_to_ipv4
 */

TEST(XferNetwork, ConvertIntegerToIpv4)
{
    char *str;

    STRCMP_EQUAL(NULL, xfer_network_convert_integer_to_ipv4 (NULL));
    STRCMP_EQUAL(NULL, xfer_network_convert_integer_to_ipv4 (""));
    STRCMP_EQUAL(NULL, xfer_network_convert_integer_to_ipv4 ("abc"));
    STRCMP_EQUAL(NULL, xfer_network_convert_integer_to_ipv4 ("0"));
    STRCMP_EQUAL(NULL, xfer_network_convert_integer_to_ipv4 ("-1"));

    /* too big: UINT32_MAX + 1 = 4294967296 */
    STRCMP_EQUAL(NULL, xfer_network_convert_integer_to_ipv4 ("4294967296"));

    WEE_TEST_STR("0.0.0.1", xfer_network_convert_integer_to_ipv4 ("1"));
    WEE_TEST_STR("0.0.1.0", xfer_network_convert_integer_to_ipv4 ("256"));
    WEE_TEST_STR("0.1.0.0", xfer_network_convert_integer_to_ipv4 ("65536"));
    WEE_TEST_STR("1.0.0.0", xfer_network_convert_integer_to_ipv4 ("16777216"));
    WEE_TEST_STR("127.0.0.1", xfer_network_convert_integer_to_ipv4 ("2130706433"));
    WEE_TEST_STR("192.168.1.2", xfer_network_convert_integer_to_ipv4 ("3232235778"));
}
