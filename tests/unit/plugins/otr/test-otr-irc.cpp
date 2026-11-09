/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test OTR IRC functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include "src/plugins/otr/otr-irc.h"
}

TEST_GROUP(OtrIrc)
{
};

/*
 * Test functions:
 *   otr_irc_target_is_channel
 */

TEST(OtrIrc, TargetIsChannel)
{
    LONGS_EQUAL(0, otr_irc_target_is_channel ("test", NULL));
    LONGS_EQUAL(0, otr_irc_target_is_channel ("test", ""));
    LONGS_EQUAL(0, otr_irc_target_is_channel ("test", "bob"));
    LONGS_EQUAL(1, otr_irc_target_is_channel ("test", "#chan"));
}

/*
 * Test functions:
 *   otr_irc_make_account
 *   otr_irc_peer_account
 */

TEST(OtrIrc, MakeAccount)
{
    /* Not connected to the server. */
    POINTERS_EQUAL(NULL, otr_irc_make_account ("unknown_server"));
    POINTERS_EQUAL(NULL, otr_irc_peer_account ("unknown_server", "bob"));
}

/*
 * Test functions:
 *   otr_irc_account_server
 *   otr_irc_account_nick
 */

TEST(OtrIrc, AccountServerNick)
{
    char nick[128];

    STRCMP_EQUAL("", otr_irc_account_server (""));
    STRCMP_EQUAL("", otr_irc_account_server ("alice"));
    STRCMP_EQUAL("libera", otr_irc_account_server ("alice@libera"));

    otr_irc_account_nick ("", nick, sizeof (nick));
    STRCMP_EQUAL("", nick);
    otr_irc_account_nick ("alice", nick, sizeof (nick));
    STRCMP_EQUAL("alice", nick);
    otr_irc_account_nick ("alice@libera", nick, sizeof (nick));
    STRCMP_EQUAL("alice", nick);
    otr_irc_account_nick ("alice@libera", nick, 3);
    STRCMP_EQUAL("al", nick);
}
