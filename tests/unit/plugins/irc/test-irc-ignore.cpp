/*
 * test-irc-ignore.cpp - test IRC ignore functions
 *
 * Copyright (C) 2019-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/plugins/irc/irc-ignore.h"
#include "src/plugins/irc/irc-server.h"
}

TEST_GROUP(IrcIgnore)
{
};

/*
 * Tests functions:
 *   irc_ignore_new
 */

TEST(IrcIgnore, New)
{
    struct t_irc_ignore *ignore;

    POINTERS_EQUAL(NULL, irc_ignore_new (NULL, NULL, NULL));

    ignore = irc_ignore_new ("^user@host$", NULL, NULL);
    CHECK(ignore);
    LONGS_EQUAL(1, ignore->number);
    STRCMP_EQUAL("^user@host$", ignore->mask);
    CHECK(ignore->regex_mask);
    STRCMP_EQUAL("*", ignore->server);
    STRCMP_EQUAL("*", ignore->channel);
    irc_ignore_free (ignore);

    ignore = irc_ignore_new ("^user@host$", "libera", "#weechat");
    CHECK(ignore);
    LONGS_EQUAL(1, ignore->number);
    STRCMP_EQUAL("^user@host$", ignore->mask);
    CHECK(ignore->regex_mask);
    STRCMP_EQUAL("libera", ignore->server);
    STRCMP_EQUAL("#weechat", ignore->channel);
    irc_ignore_free (ignore);
}

/*
 * Tests functions:
 *   irc_ignore_free
 *   irc_ignore_free_all
 */

TEST(IrcIgnore, Free)
{
    struct t_irc_ignore *ignore1, *ignore2, *ignore3;

    ignore1 = irc_ignore_new ("^user1@host$", NULL, NULL);
    CHECK(ignore1);
    LONGS_EQUAL(1, ignore1->number);
    POINTERS_EQUAL(ignore1, irc_ignore_list);
    POINTERS_EQUAL(ignore1, last_irc_ignore);

    ignore2 = irc_ignore_new ("^user2@host$", NULL, NULL);
    CHECK(ignore2);
    LONGS_EQUAL(2, ignore2->number);
    POINTERS_EQUAL(ignore1, irc_ignore_list);
    POINTERS_EQUAL(ignore2, last_irc_ignore);

    ignore3 = irc_ignore_new ("^user3@host$", NULL, NULL);
    CHECK(ignore3);
    LONGS_EQUAL(3, ignore3->number);
    POINTERS_EQUAL(ignore1, irc_ignore_list);
    POINTERS_EQUAL(ignore3, last_irc_ignore);

    irc_ignore_free (ignore1);

    LONGS_EQUAL(1, ignore2->number);
    LONGS_EQUAL(2, ignore3->number);
    POINTERS_EQUAL(ignore2, irc_ignore_list);
    POINTERS_EQUAL(ignore3, last_irc_ignore);

    irc_ignore_free_all ();

    POINTERS_EQUAL(NULL, irc_ignore_list);
    POINTERS_EQUAL(NULL, last_irc_ignore);
}

/*
 * Tests functions:
 *   irc_ignore_valid
 */

TEST(IrcIgnore, Valid)
{
    struct t_irc_ignore *ignore, *ignore_invalid;

    ignore = irc_ignore_new ("^user@host$", NULL, NULL);
    CHECK(ignore);
    LONGS_EQUAL(1, ignore->number);

    LONGS_EQUAL(0, irc_ignore_valid (NULL));

    ignore_invalid = (struct t_irc_ignore *)(((unsigned long)ignore) ^ 1);
    LONGS_EQUAL(0, irc_ignore_valid (ignore_invalid));

    LONGS_EQUAL(1, irc_ignore_valid (ignore));

    irc_ignore_free_all ();
}

/*
 * Tests functions:
 *   irc_ignore_search
 *   irc_ignore_search_by_number
 */

TEST(IrcIgnore, Search)
{
    struct t_irc_ignore *ignore1, *ignore2;

    ignore1 = irc_ignore_new ("^user1@host$", "libera", "#weechat");
    CHECK(ignore1);
    LONGS_EQUAL(1, ignore1->number);

    ignore2 = irc_ignore_new ("^user2@host$", "server2", "#channel2");
    CHECK(ignore2);
    LONGS_EQUAL(2, ignore2->number);

    POINTERS_EQUAL(NULL, irc_ignore_search ("not_found", NULL, NULL));
    POINTERS_EQUAL(NULL, irc_ignore_search ("not_found",
                                            "libera", "#weechat"));
    POINTERS_EQUAL(NULL, irc_ignore_search ("^user1@host$",
                                            "server1", "#weechat"));
    POINTERS_EQUAL(NULL, irc_ignore_search ("^user1@host$",
                                            "libera", "#channel1"));
    POINTERS_EQUAL(NULL, irc_ignore_search ("^user1@host$", NULL, NULL));
    POINTERS_EQUAL(NULL, irc_ignore_search ("^user2@host$", NULL, NULL));

    POINTERS_EQUAL(ignore1, irc_ignore_search ("^user1@host$",
                                               "libera", "#weechat"));
    POINTERS_EQUAL(ignore2, irc_ignore_search ("^user2@host$",
                                               "server2", "#channel2"));

    POINTERS_EQUAL(NULL, irc_ignore_search_by_number (-1));
    POINTERS_EQUAL(NULL, irc_ignore_search_by_number (0));
    POINTERS_EQUAL(NULL, irc_ignore_search_by_number (3));

    POINTERS_EQUAL(ignore1, irc_ignore_search_by_number (1));
    POINTERS_EQUAL(ignore2, irc_ignore_search_by_number (2));

    irc_ignore_free_all ();
}

/*
 * Tests functions:
 *   irc_ignore_check_host
 */

TEST(IrcIgnore, CheckHost)
{
    struct t_irc_server *server;
    struct t_irc_ignore *ignore1, *ignore2;

    server = irc_server_alloc ("test_ignore");
    CHECK(server);

    ignore1 = irc_ignore_new ("^user1@host$", "libera", "#weechat");
    CHECK(ignore1);
    LONGS_EQUAL(1, ignore1->number);

    ignore2 = irc_ignore_new ("^nick2$", NULL, NULL);
    CHECK(ignore2);
    LONGS_EQUAL(2, ignore2->number);

    /* check server */
    LONGS_EQUAL(0, irc_ignore_check_server (ignore1, "test"));
    LONGS_EQUAL(1, irc_ignore_check_server (ignore1, "libera"));
    LONGS_EQUAL(1, irc_ignore_check_server (ignore2, "test"));
    LONGS_EQUAL(1, irc_ignore_check_server (ignore2, "libera"));

    /* check channel */
    LONGS_EQUAL(0, irc_ignore_check_channel (ignore1, server, "#test", "nick"));
    LONGS_EQUAL(1, irc_ignore_check_channel (ignore1, server, "#weechat", "nick"));
    LONGS_EQUAL(0, irc_ignore_check_channel (ignore1, server, "test", "nick"));
    LONGS_EQUAL(0, irc_ignore_check_channel (ignore1, server, "weechat", "nick"));
    LONGS_EQUAL(1, irc_ignore_check_channel (ignore2, server, "#test", "nick"));
    LONGS_EQUAL(1, irc_ignore_check_channel (ignore2, server, "#weechat", "nick"));

    /* check host */
    LONGS_EQUAL(0, irc_ignore_check_host (ignore1, "nick1", "nick!aaa@bbb"));
    LONGS_EQUAL(0, irc_ignore_check_host (ignore1, "nick1", "test"));
    LONGS_EQUAL(1, irc_ignore_check_host (ignore1, "nick1", "user1@host"));
    LONGS_EQUAL(1, irc_ignore_check_host (ignore1, "nick1", "nick1!user1@host"));
    LONGS_EQUAL(0, irc_ignore_check_host (ignore2, "nick1", "nick1!aaa@bbb"));
    LONGS_EQUAL(1, irc_ignore_check_host (ignore2, "nick2", "nick2!aaa@bbb"));

    irc_ignore_free_all ();
    irc_server_free (server);
}
