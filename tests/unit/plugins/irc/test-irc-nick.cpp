/*
 * test-irc-nick.cpp - test IRC nick functions
 *
 * Copyright (C) 2019-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <string.h>
#include "src/gui/gui-color.h"
#include "src/plugins/irc/irc-nick.h"
#include "src/plugins/irc/irc-server.h"
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
    struct t_irc_server *server;

    /* no server, default utf8mapping = none */

    /* empty nick */
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, NULL));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, ""));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, " "));

    /* invalid first char (rfc1459) */
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "0abc"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "9abc"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "-abc"));

    /* invalid first char: prefix char */
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "@abc"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "+abc"));

    /* invalid first char: chantypes */
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "#abc"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "&abc"));

    /* invalid chars in nick */
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "nick test"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "nick,test"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "nick?test"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "nick!test"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "nick@test"));

    /* UTF-8 wide chars in nick */
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "noël"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "testé"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "\xf0\xa4\xad\xa2"));  /* han char */

    /* valid nicks */
    LONGS_EQUAL(1, irc_nick_is_nick (NULL, "tester"));
    LONGS_EQUAL(1, irc_nick_is_nick (NULL, "bob"));
    LONGS_EQUAL(1, irc_nick_is_nick (NULL, "alice"));
    LONGS_EQUAL(1, irc_nick_is_nick (NULL, "very_long_nick_which_is_valid"));

    /*
     * server with:
     *   utf8mapping = rfc8265
     *   nicklen = 20
     *   prefix = (qaohv)~&@%+
     *   chantypes = #
     */
    server = irc_server_alloc ("my_ircd");
    CHECK(server);
    if (server->chantypes)
        free (server->chantypes);
    server->utf8mapping = IRC_SERVER_UTF8MAPPING_RFC8265;
    server->nick_max_length = 20;
    irc_server_set_prefix_modes_chars (server, "(qaohv)~&@%+");
    if (server->chantypes)
        free (server->chantypes);
    server->chantypes = strdup ("#");

    /* empty nick */
    LONGS_EQUAL(0, irc_nick_is_nick (server, NULL));
    LONGS_EQUAL(0, irc_nick_is_nick (server, ""));
    LONGS_EQUAL(0, irc_nick_is_nick (server, " "));

    /* nick too long */
    LONGS_EQUAL(0, irc_nick_is_nick (server, "long_nick___length_21"));

    /* valid nicks: first char allowed with utf8mapping = rfc8265 */
    LONGS_EQUAL(1, irc_nick_is_nick (server, "0abc"));
    LONGS_EQUAL(1, irc_nick_is_nick (server, "9abc"));
    LONGS_EQUAL(1, irc_nick_is_nick (server, "-abc"));

    /* invalid first char: prefix char */
    LONGS_EQUAL(0, irc_nick_is_nick (server, "~abc"));
    LONGS_EQUAL(0, irc_nick_is_nick (server, "&abc"));
    LONGS_EQUAL(0, irc_nick_is_nick (server, "@abc"));
    LONGS_EQUAL(0, irc_nick_is_nick (server, "%abc"));
    LONGS_EQUAL(0, irc_nick_is_nick (server, "+abc"));

    /* invalid first char: chantypes */
    LONGS_EQUAL(0, irc_nick_is_nick (server, "#abc"));

    /* invalid chars in nick */
    LONGS_EQUAL(0, irc_nick_is_nick (server, "nick test"));
    LONGS_EQUAL(0, irc_nick_is_nick (server, "nick,test"));
    LONGS_EQUAL(0, irc_nick_is_nick (server, "nick?test"));
    LONGS_EQUAL(0, irc_nick_is_nick (server, "nick!test"));
    LONGS_EQUAL(0, irc_nick_is_nick (server, "nick@test"));

    /* invalid UTF-8 */
    LONGS_EQUAL(0, irc_nick_is_nick (server, "no\xc3l"));

    /* valid nicks: UTF-8 */
    LONGS_EQUAL(1, irc_nick_is_nick (server, "noël"));
    LONGS_EQUAL(1, irc_nick_is_nick (server, "testé"));
    LONGS_EQUAL(1, irc_nick_is_nick (server, "\xf0\xa4\xad\xa2")); /* han char */

    /* valid nicks with UTF-8 wide chars */
    LONGS_EQUAL(1, irc_nick_is_nick (server, "noël"));
    LONGS_EQUAL(1, irc_nick_is_nick (server, "testé"));

    /* valid nicks */
    LONGS_EQUAL(1, irc_nick_is_nick (server, "tester"));
    LONGS_EQUAL(1, irc_nick_is_nick (server, "bob"));
    LONGS_EQUAL(1, irc_nick_is_nick (server, "alice"));
    LONGS_EQUAL(1, irc_nick_is_nick (server, "long_nick__length_20"));

    /* max length: 4 bytes */
    server->nick_max_length = 4;

    /* invalid nick: 8 bytes */
    LONGS_EQUAL(0, irc_nick_is_nick (server, "\xf0\xa4\xad\xa2\xf0\xa4\xad\xa2"));

    /* valid nick: 4 bytes */
    LONGS_EQUAL(1, irc_nick_is_nick (server, "\xf0\xa4\xad\xa2")); /* han char */

    /* max length: 3 bytes */
    server->nick_max_length = 3;

    /* invalid nick: 4 bytes */
    LONGS_EQUAL(0, irc_nick_is_nick (server, "\xf0\xa4\xad\xa2")); /* han char */

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_nick_find_color
 *   irc_nick_find_color_name
 */

TEST(IrcNick, FindColor)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_set_current_prefix
 */

TEST(IrcNick, SetCurrentPrefix)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_set_prefix
 */

TEST(IrcNick, SetPrefix)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_set_prefixes
 */

TEST(IrcNick, SetPrefixes)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_set_host
 */

TEST(IrcNick, SetHost)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_is_op_or_higher
 */

TEST(IrcNick, IsOpOrHigher)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_has_prefix_mode
 */

TEST(IrcNick, HasPrefixMode)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_get_nicklist_group
 */

TEST(IrcNick, GetNicklistGroup)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_get_prefix_color_name
 */

TEST(IrcNick, GetPrefixColorName)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_get_color_for_nicklist
 */

TEST(IrcNick, GetColorForNicklist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_nicklist_add
 */

TEST(IrcNick, NicklistAdd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_nicklist_remove
 */

TEST(IrcNick, NicklistRemove)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_nicklist_set
 */

TEST(IrcNick, NicklistSet)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_nicklist_set_prefix_color_all
 */

TEST(IrcNick, NicklistSetPrefixColorAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_nicklist_set_color_all
 */

TEST(IrcNick, NicklistSetColorAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_new
 */

TEST(IrcNick, New)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_change
 */

TEST(IrcNick, Change)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_set_mode
 */

TEST(IrcNick, SetMode)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_realloc_prefixes
 */

TEST(IrcNick, ReallocPrefixes)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_free
 */

TEST(IrcNick, Free)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_free_all
 */

TEST(IrcNick, FreeAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_search
 */

TEST(IrcNick, Search)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_count
 */

TEST(IrcNick, Count)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_set_away
 */

TEST(IrcNick, SetAway)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_mode_for_display
 */

TEST(IrcNick, ModeForDisplay)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_as_prefix
 */

TEST(IrcNick, AsPrefix)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_color_for_msg
 */

TEST(IrcNick, ColorForMsg)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_color_for_pv
 */

TEST(IrcNick, ColorForPv)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_default_ban_mask
 */

TEST(IrcNick, DefaultBanMask)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_hdata_nick_cb
 */

TEST(IrcNick, HdataNickCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_add_to_infolist
 */

TEST(IrcNick, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_nick_print_log
 */

TEST(IrcNick, PrintLog)
{
    /* TODO: write tests */
}
