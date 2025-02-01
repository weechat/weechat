/*
 * test-core-network.cpp - test network functions
 *
 * Copyright (C) 2021-2025 Sébastien Helleu <flashcode@flashtux.org>
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

extern "C"
{
#include "src/core/core-network.h"

extern int network_is_ip_address (const char *address);
}

TEST_GROUP(CoreNetwork)
{
};

/*
 * Tests functions:
 *   network_init_gcrypt
 */

TEST(CoreNetwork, InitGcrypt)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_load_system_ca_file
 */

TEST(CoreNetwork, LoadSystemCaFile)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_load_user_ca_files
 */

TEST(CoreNetwork, LoadUserCaFiles)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_load_ca_files
 */

TEST(CoreNetwork, LoadCaFiles)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_reload_ca_files
 */

TEST(CoreNetwork, ReloadCaFiles)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_init_gnutls
 */

TEST(CoreNetwork, InitGnutls)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_end
 */

TEST(CoreNetwork, End)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_is_ip_address
 */

TEST(CoreNetwork, IsIpAddress)
{
    /* invalid address */
    LONGS_EQUAL(0, network_is_ip_address (NULL));
    LONGS_EQUAL(0, network_is_ip_address (""));
    LONGS_EQUAL(0, network_is_ip_address ("abc"));
    LONGS_EQUAL(0, network_is_ip_address ("1"));
    LONGS_EQUAL(0, network_is_ip_address ("1.2"));
    LONGS_EQUAL(0, network_is_ip_address ("1.2.3"));
    LONGS_EQUAL(0, network_is_ip_address ("1.2.3.a"));
    LONGS_EQUAL(0, network_is_ip_address ("1.2.3.4.5"));
    LONGS_EQUAL(0, network_is_ip_address ("001.002.003.004"));

    /* valid IPv4 */
    LONGS_EQUAL(1, network_is_ip_address ("127.0.0.1"));
    LONGS_EQUAL(1, network_is_ip_address ("1.2.3.4"));

    /* valid IPv6 */
    LONGS_EQUAL(1, network_is_ip_address ("::1"));
    LONGS_EQUAL(1, network_is_ip_address ("2001:0db8:0000:85a3:0000:0000:ac1f:8001"));
    LONGS_EQUAL(1, network_is_ip_address ("2001:db8:0:85a3:0:0:ac1f:8001"));
    LONGS_EQUAL(1, network_is_ip_address ("2001:db8:0:85a3::ac1f:8001"));
}

/*
 * Tests functions:
 *   network_send_with_retry
 */

TEST(CoreNetwork, SendWithRetry)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_recv_with_retry
 */

TEST(CoreNetwork, RecvWithRetry)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_pass_httpproxy
 */

TEST(CoreNetwork, PassHttpproxy)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_resolve
 */

TEST(CoreNetwork, Resolve)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_pass_socks4proxy
 */

TEST(CoreNetwork, PassSock4proxy)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_pass_socks5proxy
 */

TEST(CoreNetwork, PassSocks5proxy)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_pass_proxy
 */

TEST(CoreNetwork, PassProxy)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_connect
 */

TEST(CoreNetwork, Connect)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_connect_to
 */

TEST(CoreNetwork, ConnectTo)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_connect_child
 */

TEST(CoreNetwork, ConnectChild)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_connect_child_timer_cb
 */

TEST(CoreNetwork, ConnectChildTimerCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_connect_gnutls_handshake_fd_cb
 */

TEST(CoreNetwork, ConnectGnutlsHandshakeFdCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_connect_gnutls_handshake_timer_cb
 */

TEST(CoreNetwork, ConnectGnutlsHandshakeTimerCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_connect_child_read_cb
 */

TEST(CoreNetwork, ConnectChildReadCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   network_connect_with_fork
 */

TEST(CoreNetwork, ConnectWithFork)
{
    /* TODO: write tests */
}
