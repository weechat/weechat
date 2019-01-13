/*
 * test-irc-protocol.cpp - test IRC protocol functions
 *
 * Copyright (C) 2019 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/plugins/irc/irc-protocol.h"
}

TEST_GROUP(IrcProtocol)
{
};

/*
 * Tests functions:
 *   irc_protocol_parse_time
 */

TEST(IrcProtocol, ParseTime)
{
    /* invalid time formats */
    LONGS_EQUAL(0, irc_protocol_parse_time (NULL));
    LONGS_EQUAL(0, irc_protocol_parse_time (""));
    LONGS_EQUAL(0, irc_protocol_parse_time ("invalid"));

    /* incomplete time formats */
    LONGS_EQUAL(0, irc_protocol_parse_time ("2019-01"));
    LONGS_EQUAL(0, irc_protocol_parse_time ("2019-01-13"));
    LONGS_EQUAL(0, irc_protocol_parse_time ("2019-01-13T14"));
    LONGS_EQUAL(0, irc_protocol_parse_time ("2019-01-13T14:37"));

    /* valid time with ISO 8601 format*/
    LONGS_EQUAL(1547386699, irc_protocol_parse_time ("2019-01-13T13:38:19.123Z"));
    LONGS_EQUAL(1547386699, irc_protocol_parse_time ("2019-01-13T13:38:19.123"));
    LONGS_EQUAL(1547386699, irc_protocol_parse_time ("2019-01-13T13:38:19"));

    /* valid time as timestamp */
    LONGS_EQUAL(1547386699, irc_protocol_parse_time ("1547386699.123"));
    LONGS_EQUAL(1547386699, irc_protocol_parse_time ("1547386699"));
}
