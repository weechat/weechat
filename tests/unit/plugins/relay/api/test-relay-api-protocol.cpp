/*
 * test-relay-api-protocol.cpp - test relay API protocol (protocol)
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/api/relay-api-protocol.h"
}

TEST_GROUP(RelayApiProtocol)
{
};

/*
 * Tests functions:
 *   relay_api_protocol_signal_buffer_cb
 */

TEST(RelayApiProtocol, SignalBufferCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_protocol_hsignal_nicklist_cb
 */

TEST(RelayApiProtocol, HsignalNicklistCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_protocol_signal_upgrade_cb
 */

TEST(RelayApiProtocol, SignalUpgradeCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_protocol_cb_handshake
 */

TEST(RelayApiProtocol, CbHandshake)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_protocol_cb_version
 */

TEST(RelayApiProtocol, CbVersion)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_protocol_cb_buffers
 */

TEST(RelayApiProtocol, CbBuffers)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_protocol_cb_input
 */

TEST(RelayApiProtocol, CbInput)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_protocol_cb_ping
 */

TEST(RelayApiProtocol, CbPing)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_protocol_cb_sync
 */

TEST(RelayApiProtocol, CbSync)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_protocol_recv_json
 */

TEST(RelayApiProtocol, RecvJson)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_protocol_recv_http
 */

TEST(RelayApiProtocol, RecvHttp)
{
    /* TODO: write tests */
}
