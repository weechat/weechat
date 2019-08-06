/*
 * test-irc-nick.cpp - test IRC nick functions
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
#include "src/plugins/irc/irc-nick.h"
}

TEST_GROUP(IrcNick)
{
};

/*
 * Tests functions:
 *   irc_nick_valid
 */

TEST(IrcNick, Valid)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_is_nick
 */

TEST(IrcNick, IsNick)
{
    /* empty nick */
    LONGS_EQUAL(0, irc_nick_is_nick (NULL));
    LONGS_EQUAL(0, irc_nick_is_nick (""));
    LONGS_EQUAL(0, irc_nick_is_nick (" "));

    /* invalid first char */
    LONGS_EQUAL(0, irc_nick_is_nick ("0abc"));
    LONGS_EQUAL(0, irc_nick_is_nick ("9abc"));
    LONGS_EQUAL(0, irc_nick_is_nick ("-abc"));

    /* invalid chars in nick */
    LONGS_EQUAL(0, irc_nick_is_nick ("noël"));
    LONGS_EQUAL(0, irc_nick_is_nick ("testé"));
    LONGS_EQUAL(0, irc_nick_is_nick ("nick space"));

    /* valid nicks */
    LONGS_EQUAL(1, irc_nick_is_nick ("tester"));
    LONGS_EQUAL(1, irc_nick_is_nick ("bob"));
    LONGS_EQUAL(1, irc_nick_is_nick ("alice"));
    LONGS_EQUAL(1, irc_nick_is_nick ("very_long_nick_which_is_valid"));
}
