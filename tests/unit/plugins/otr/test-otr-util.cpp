/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test OTR utility functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include "src/plugins/otr/otr-util.h"
}

TEST_GROUP(OtrUtil)
{
};

/*
 * Test functions:
 *   otr_util_dupstr
 */

TEST(OtrUtil, Dupstr)
{
    char *str;

    WEE_TEST_STR("", otr_util_dupstr (NULL));
    WEE_TEST_STR("", otr_util_dupstr (""));
    WEE_TEST_STR("abc", otr_util_dupstr ("abc"));
}

/*
 * Test functions:
 *   otr_util_irc_sanitize
 */

TEST(OtrUtil, IrcSanitize)
{
    char *str;

    WEE_TEST_STR("", otr_util_irc_sanitize (NULL));
    WEE_TEST_STR("", otr_util_irc_sanitize (""));
    WEE_TEST_STR("abc", otr_util_irc_sanitize ("abc"));
    WEE_TEST_STR("a b  c ", otr_util_irc_sanitize ("a\rb\r\nc\n"));
}

/*
 * Test functions:
 *   otr_util_peer_text
 */

TEST(OtrUtil, PeerText)
{
    char *str;

    WEE_TEST_STR("", otr_util_peer_text (NULL));
    WEE_TEST_STR("", otr_util_peer_text (""));
    WEE_TEST_STR("abc", otr_util_peer_text ("abc"));
    WEE_TEST_STR("question? SMP verification succeeded.",
                 otr_util_peer_text ("question?\n"
                                     "SMP verification succeeded."));
    WEE_TEST_STR("a b", otr_util_peer_text ("a\rb"));

    /* WeeChat color codes are removed. */
    WEE_TEST_STR("green text",
                 otr_util_peer_text ("\x19" "02" "green text"));
    WEE_TEST_STR("bold", otr_util_peer_text ("\x1A" "\x01" "bold"));
}
