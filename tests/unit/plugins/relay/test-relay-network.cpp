/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test network functions for Relay plugin */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "src/core/core-config-file.h"
#include "src/plugins/weechat-plugin.h"
#include "src/plugins/relay/relay-config.h"
#include "src/plugins/relay/relay-network.h"
}

#define WEE_CHECK_PARSE_CIDR(__family, __addr, __prefix_len, __string)  \
    memset (&cidr, 0xAA, sizeof (cidr));                                \
    LONGS_EQUAL(1, relay_network_parse_cidr (__string, &cidr));         \
    LONGS_EQUAL(__family, cidr.family);                                 \
    memset (addr, 0, sizeof (addr));                                    \
    LONGS_EQUAL(1, inet_pton (__family, __addr, addr));                 \
    MEMCMP_EQUAL(addr, cidr.addr, sizeof (addr));                       \
    LONGS_EQUAL(__prefix_len, cidr.prefix_len);

#define WEE_CHECK_MATCH(__result, __network, __address)                 \
    LONGS_EQUAL(1, relay_network_parse_cidr (__network, &cidr));        \
    LONGS_EQUAL(1, relay_network_parse_ip (__address, &family, addr));  \
    LONGS_EQUAL(__result, relay_network_cidr_match (&cidr, family, addr));

TEST_GROUP(RelayNetwork)
{
};

/*
 * Test functions:
 *   relay_network_parse_ip
 */

TEST(RelayNetwork, ParseIp)
{
    unsigned char addr[16], addr2[16];
    int family;

    LONGS_EQUAL(0, relay_network_parse_ip (NULL, NULL, NULL));
    LONGS_EQUAL(0, relay_network_parse_ip (NULL, &family, addr));
    LONGS_EQUAL(0, relay_network_parse_ip ("", &family, addr));
    LONGS_EQUAL(0, relay_network_parse_ip ("127.0.0.1", NULL, addr));
    LONGS_EQUAL(0, relay_network_parse_ip ("127.0.0.1", &family, NULL));
    LONGS_EQUAL(0, relay_network_parse_ip ("abc", &family, addr));
    LONGS_EQUAL(0, relay_network_parse_ip ("1.2.3", &family, addr));
    LONGS_EQUAL(0, relay_network_parse_ip ("1.2.3.256", &family, addr));
    LONGS_EQUAL(0, relay_network_parse_ip ("1.2.3.4/32", &family, addr));
    LONGS_EQUAL(0, relay_network_parse_ip ("2001:db8:::1", &family, addr));
    LONGS_EQUAL(0, relay_network_parse_ip (" 127.0.0.1", &family, addr));

    /* IPv4 */
    LONGS_EQUAL(1, relay_network_parse_ip ("192.168.1.2", &family, addr));
    LONGS_EQUAL(AF_INET, family);
    memset (addr2, 0, sizeof (addr2));
    addr2[0] = 192;
    addr2[1] = 168;
    addr2[2] = 1;
    addr2[3] = 2;
    MEMCMP_EQUAL(addr2, addr, sizeof (addr));

    /* IPv6 */
    LONGS_EQUAL(1, relay_network_parse_ip ("2001:db8::1", &family, addr));
    LONGS_EQUAL(AF_INET6, family);
    memset (addr2, 0, sizeof (addr2));
    addr2[0] = 0x20;
    addr2[1] = 0x01;
    addr2[2] = 0x0D;
    addr2[3] = 0xB8;
    addr2[15] = 0x01;
    MEMCMP_EQUAL(addr2, addr, sizeof (addr));

    /* IPv4-mapped IPv6 address: converted to IPv4 */
    LONGS_EQUAL(1, relay_network_parse_ip ("::ffff:10.1.2.3", &family, addr));
    LONGS_EQUAL(AF_INET, family);
    memset (addr2, 0, sizeof (addr2));
    addr2[0] = 10;
    addr2[1] = 1;
    addr2[2] = 2;
    addr2[3] = 3;
    MEMCMP_EQUAL(addr2, addr, sizeof (addr));
}

/*
 * Test functions:
 *   relay_network_parse_cidr
 */

TEST(RelayNetwork, ParseCidr)
{
    struct t_relay_network_cidr cidr;
    unsigned char addr[16];

    LONGS_EQUAL(0, relay_network_parse_cidr (NULL, NULL));
    LONGS_EQUAL(0, relay_network_parse_cidr (NULL, &cidr));
    LONGS_EQUAL(0, relay_network_parse_cidr ("127.0.0.1", NULL));
    LONGS_EQUAL(0, relay_network_parse_cidr ("", &cidr));
    LONGS_EQUAL(0, relay_network_parse_cidr ("abc", &cidr));
    LONGS_EQUAL(0, relay_network_parse_cidr ("/24", &cidr));
    LONGS_EQUAL(0, relay_network_parse_cidr ("1.2.3/24", &cidr));
    LONGS_EQUAL(0, relay_network_parse_cidr ("1.2.3.4/", &cidr));
    LONGS_EQUAL(0, relay_network_parse_cidr ("1.2.3.4/-1", &cidr));
    LONGS_EQUAL(0, relay_network_parse_cidr ("1.2.3.4/+1", &cidr));
    LONGS_EQUAL(0, relay_network_parse_cidr ("1.2.3.4/ 1", &cidr));
    LONGS_EQUAL(0, relay_network_parse_cidr ("1.2.3.4/2x", &cidr));
    LONGS_EQUAL(0, relay_network_parse_cidr ("1.2.3.4/24/8", &cidr));
    LONGS_EQUAL(0, relay_network_parse_cidr ("1.2.3.4/33", &cidr));
    LONGS_EQUAL(0, relay_network_parse_cidr ("1.2.3.4/99999999999999999999", &cidr));
    LONGS_EQUAL(0, relay_network_parse_cidr ("2001:db8::/129", &cidr));
    LONGS_EQUAL(0, relay_network_parse_cidr ("2001:db8::*", &cidr));
    LONGS_EQUAL(0, relay_network_parse_cidr ("::ffff:10.0.0.0/95", &cidr));
    LONGS_EQUAL(0, relay_network_parse_cidr ("::ffff:10.0.0.0/129", &cidr));

    /* IPv4 */
    WEE_CHECK_PARSE_CIDR(AF_INET, "127.0.0.1", 32, "127.0.0.1");
    WEE_CHECK_PARSE_CIDR(AF_INET, "0.0.0.0", 0, "0.0.0.0/0");
    WEE_CHECK_PARSE_CIDR(AF_INET, "192.168.1.0", 24, "192.168.1.0/24");
    WEE_CHECK_PARSE_CIDR(AF_INET, "192.168.1.5", 24, "192.168.1.5/24");
    WEE_CHECK_PARSE_CIDR(AF_INET, "10.0.0.0", 8, "10.0.0.0/08");
    WEE_CHECK_PARSE_CIDR(AF_INET, "1.2.3.4", 32, "1.2.3.4/32");

    /* IPv6 */
    WEE_CHECK_PARSE_CIDR(AF_INET6, "::1", 128, "::1");
    WEE_CHECK_PARSE_CIDR(AF_INET6, "::", 0, "::/0");
    WEE_CHECK_PARSE_CIDR(AF_INET6, "2001:db8::", 32, "2001:db8::/32");
    WEE_CHECK_PARSE_CIDR(AF_INET6, "2601:1c0:5002:5720::", 64,
                         "2601:01C0:5002:5720:0000::/64");
    WEE_CHECK_PARSE_CIDR(AF_INET6, "2001:db8::1", 128, "2001:db8::1/128");

    /* IPv4-mapped IPv6: converted to IPv4 */
    WEE_CHECK_PARSE_CIDR(AF_INET, "10.1.2.3", 32, "::ffff:10.1.2.3");
    WEE_CHECK_PARSE_CIDR(AF_INET, "10.0.0.0", 8, "::ffff:10.0.0.0/104");
    WEE_CHECK_PARSE_CIDR(AF_INET, "0.0.0.0", 0, "::ffff:0.0.0.0/96");
    WEE_CHECK_PARSE_CIDR(AF_INET, "10.1.2.3", 32, "::ffff:10.1.2.3/128");
}

/*
 * Test functions:
 *   relay_network_cidr_match
 */

TEST(RelayNetwork, CidrMatch)
{
    struct t_relay_network_cidr cidr;
    unsigned char addr[16];
    int family;

    LONGS_EQUAL(1, relay_network_parse_cidr ("127.0.0.1", &cidr));
    LONGS_EQUAL(1, relay_network_parse_ip ("127.0.0.1", &family, addr));
    LONGS_EQUAL(0, relay_network_cidr_match (NULL, AF_INET, addr));
    LONGS_EQUAL(0, relay_network_cidr_match (&cidr, AF_INET, NULL));
    LONGS_EQUAL(0, relay_network_cidr_match (&cidr, AF_INET6, addr));

    /* IPv4 */
    WEE_CHECK_MATCH(1, "127.0.0.1", "127.0.0.1");
    WEE_CHECK_MATCH(0, "127.0.0.1", "127.0.0.2");
    WEE_CHECK_MATCH(1, "0.0.0.0/0", "1.2.3.4");
    WEE_CHECK_MATCH(1, "0.0.0.0/0", "255.255.255.255");
    WEE_CHECK_MATCH(1, "192.168.1.0/24", "192.168.1.0");
    WEE_CHECK_MATCH(1, "192.168.1.0/24", "192.168.1.255");
    WEE_CHECK_MATCH(0, "192.168.1.0/24", "192.168.2.1");
    WEE_CHECK_MATCH(1, "192.168.1.5/24", "192.168.1.200");
    WEE_CHECK_MATCH(1, "172.16.0.0/12", "172.31.255.255");
    WEE_CHECK_MATCH(0, "172.16.0.0/12", "172.32.0.0");
    WEE_CHECK_MATCH(0, "172.16.0.0/12", "172.15.255.255");
    WEE_CHECK_MATCH(1, "10.0.0.0/20", "10.0.15.1");
    WEE_CHECK_MATCH(0, "10.0.0.0/20", "10.0.16.1");
    WEE_CHECK_MATCH(1, "10.1.2.3/31", "10.1.2.2");
    WEE_CHECK_MATCH(0, "10.1.2.3/31", "10.1.2.4");

    /* IPv6 */
    WEE_CHECK_MATCH(1, "::1", "::1");
    WEE_CHECK_MATCH(0, "::1", "::2");
    WEE_CHECK_MATCH(1, "::/0", "2001:db8::1");
    WEE_CHECK_MATCH(1, "2001:db8::/32", "2001:db8:ffff::1");
    WEE_CHECK_MATCH(0, "2001:db8::/32", "2001:db9::1");
    WEE_CHECK_MATCH(1, "2601:1c0:5002:5720::/64", "2601:1c0:5002:5720:abcd::1");
    WEE_CHECK_MATCH(1, "2601:1c0:5002:5720::/64",
                    "2601:1c0:5002:5720:ffff:ffff:ffff:ffff");
    WEE_CHECK_MATCH(0, "2601:1c0:5002:5720::/64", "2601:1c0:5002:5721::1");
    WEE_CHECK_MATCH(1, "2001:db8:0:0:1::/80", "2001:db8::1:0:0:5");
    WEE_CHECK_MATCH(0, "2001:db8:0:0:1::/80", "2001:db8::2:0:0:5");
    WEE_CHECK_MATCH(1, "2001:db8::/61", "2001:db8:0:7::1");
    WEE_CHECK_MATCH(0, "2001:db8::/61", "2001:db8:0:8::1");

    /* IPv4 address with IPv6 network and vice versa */
    WEE_CHECK_MATCH(0, "::/0", "1.2.3.4");
    WEE_CHECK_MATCH(0, "0.0.0.0/0", "::1");

    /* IPv4-mapped IPv6 address and network */
    WEE_CHECK_MATCH(1, "10.0.0.0/8", "::ffff:10.1.2.3");
    WEE_CHECK_MATCH(1, "::ffff:10.0.0.0/104", "10.1.2.3");
    WEE_CHECK_MATCH(0, "::ffff:10.0.0.0/104", "11.1.2.3");
}

/*
 * Test functions:
 *   relay_network_address_in_networks
 */

TEST(RelayNetwork, AddressInNetworks)
{
    struct t_relay_network_cidr networks[3];

    LONGS_EQUAL(1, relay_network_parse_cidr ("127.0.0.1", &networks[0]));
    LONGS_EQUAL(1, relay_network_parse_cidr ("192.168.0.0/16", &networks[1]));
    LONGS_EQUAL(1, relay_network_parse_cidr ("2001:db8::/32", &networks[2]));

    LONGS_EQUAL(0, relay_network_address_in_networks (NULL, NULL, 0));
    LONGS_EQUAL(0, relay_network_address_in_networks ("127.0.0.1", NULL, 3));
    LONGS_EQUAL(0, relay_network_address_in_networks ("127.0.0.1", networks, 0));
    LONGS_EQUAL(0, relay_network_address_in_networks (NULL, networks, 3));
    LONGS_EQUAL(0, relay_network_address_in_networks ("", networks, 3));
    LONGS_EQUAL(0, relay_network_address_in_networks ("abc", networks, 3));
    LONGS_EQUAL(0, relay_network_address_in_networks ("/tmp/relay.sock",
                                                      networks, 3));

    LONGS_EQUAL(1, relay_network_address_in_networks ("127.0.0.1", networks, 3));
    LONGS_EQUAL(0, relay_network_address_in_networks ("127.0.0.2", networks, 3));
    LONGS_EQUAL(1, relay_network_address_in_networks ("192.168.42.1", networks, 3));
    LONGS_EQUAL(0, relay_network_address_in_networks ("192.169.0.1", networks, 3));
    LONGS_EQUAL(1, relay_network_address_in_networks ("2001:db8::abcd", networks, 3));
    LONGS_EQUAL(0, relay_network_address_in_networks ("::1", networks, 3));
    LONGS_EQUAL(1, relay_network_address_in_networks ("::ffff:192.168.0.1",
                                                      networks, 3));

    /* only first network */
    LONGS_EQUAL(1, relay_network_address_in_networks ("127.0.0.1", networks, 1));
    LONGS_EQUAL(0, relay_network_address_in_networks ("192.168.42.1", networks, 1));
}

/*
 * Test functions:
 *   relay_config_check_network_allowed_networks
 *   relay_config_change_network_allowed_networks
 */

TEST(RelayNetwork, ConfigAllowedNetworks)
{
    LONGS_EQUAL(0, relay_config_num_allowed_networks);
    POINTERS_EQUAL(NULL, relay_config_allowed_networks);

    /* invalid values */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (relay_config_network_allowed_networks,
                                        "abc", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (relay_config_network_allowed_networks,
                                        "127.0.0.1,1.2.3.4/33", 1));
    STRCMP_EQUAL("", CONFIG_STRING(relay_config_network_allowed_networks));
    LONGS_EQUAL(0, relay_config_num_allowed_networks);
    POINTERS_EQUAL(NULL, relay_config_allowed_networks);

    /* valid value */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (relay_config_network_allowed_networks,
                                        " 127.0.0.1 , 192.168.0.0/16,,"
                                        "2001:db8::/32 ",
                                        1));
    LONGS_EQUAL(3, relay_config_num_allowed_networks);
    CHECK(relay_config_allowed_networks);
    LONGS_EQUAL(AF_INET, relay_config_allowed_networks[0].family);
    LONGS_EQUAL(32, relay_config_allowed_networks[0].prefix_len);
    LONGS_EQUAL(AF_INET, relay_config_allowed_networks[1].family);
    LONGS_EQUAL(16, relay_config_allowed_networks[1].prefix_len);
    LONGS_EQUAL(AF_INET6, relay_config_allowed_networks[2].family);
    LONGS_EQUAL(32, relay_config_allowed_networks[2].prefix_len);

    /* reset to empty value */
    config_file_option_reset (relay_config_network_allowed_networks, 1);
    LONGS_EQUAL(0, relay_config_num_allowed_networks);
    POINTERS_EQUAL(NULL, relay_config_allowed_networks);
}
