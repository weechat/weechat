/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test OTR echo-message functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include "src/plugins/otr/otr-echo.h"

extern size_t otr_echo_trimmed_len (const char *string);
extern char *otr_echo_pending_take (const char *server, const char *target,
                                    const char *cipher);
}

TEST_GROUP(OtrEcho)
{
    void setup ()
    {
        otr_echo_pending_clear_all ();
    }

    void teardown ()
    {
        otr_echo_pending_clear_all ();
    }
};

/*
 * Test functions:
 *   otr_echo_trimmed_len
 */

TEST(OtrEcho, TrimmedLen)
{
    LONGS_EQUAL(0, otr_echo_trimmed_len (""));
    LONGS_EQUAL(0, otr_echo_trimmed_len (" \t "));
    LONGS_EQUAL(3, otr_echo_trimmed_len ("abc"));
    LONGS_EQUAL(3, otr_echo_trimmed_len ("abc \t  "));
    LONGS_EQUAL(5, otr_echo_trimmed_len (" a bc"));
}

/*
 * Test functions:
 *   otr_echo_pending_add
 *   otr_echo_pending_take
 *   otr_echo_pending_drop
 *   otr_echo_pending_clear_all
 */

TEST(OtrEcho, PendingAddTake)
{
    char *str, cipher[64];
    int i;

    LONGS_EQUAL(0, otr_echo_pending_count);
    POINTERS_EQUAL(NULL, otr_echo_pending_take ("srv", "bob", "?OTR:abc."));

    otr_echo_pending_add ("srv", "Bob", "?OTR:abc.", "hello");
    LONGS_EQUAL(1, otr_echo_pending_count);
    STRCMP_EQUAL("bob", otr_echo_pending_queue[0].peer);

    /* Not found: other server, other peer, other cipher. */
    POINTERS_EQUAL(NULL, otr_echo_pending_take ("srv2", "bob", "?OTR:abc."));
    POINTERS_EQUAL(NULL, otr_echo_pending_take ("srv", "alice", "?OTR:abc."));
    POINTERS_EQUAL(NULL, otr_echo_pending_take ("srv", "bob", "?OTR:xyz."));
    LONGS_EQUAL(1, otr_echo_pending_count);

    /* Found: trailing spaces and case of peer are ignored. */
    WEE_TEST_STR("hello", otr_echo_pending_take ("srv", "BOB", "?OTR:abc.  "));
    LONGS_EQUAL(0, otr_echo_pending_count);
    POINTERS_EQUAL(NULL, otr_echo_pending_take ("srv", "bob", "?OTR:abc."));

    /* Queue full: the oldest message is removed. */
    for (i = 0; i < OTR_ECHO_PENDING_MAX + 1; i++)
    {
        snprintf (cipher, sizeof (cipher), "?OTR:%d.", i);
        otr_echo_pending_add ("srv", "bob", cipher, "msg");
    }
    LONGS_EQUAL(OTR_ECHO_PENDING_MAX, otr_echo_pending_count);
    POINTERS_EQUAL(NULL, otr_echo_pending_take ("srv", "bob", "?OTR:0."));
    WEE_TEST_STR("msg", otr_echo_pending_take ("srv", "bob", "?OTR:1."));
    LONGS_EQUAL(OTR_ECHO_PENDING_MAX - 1, otr_echo_pending_count);

    otr_echo_pending_clear_all ();
    LONGS_EQUAL(0, otr_echo_pending_count);
}

/*
 * Test functions:
 *   otr_echo_replace
 */

TEST(OtrEcho, Replace)
{
    char *str;

    /* Not an OTR message: unchanged. */
    POINTERS_EQUAL(NULL,
                   otr_echo_replace ("srv",
                                     ":alice!u@h PRIVMSG bob :hello",
                                     "alice!u@h", "bob", "hello"));

    /* OTR message without pending message: hidden. */
    WEE_TEST_STR("",
                 otr_echo_replace ("srv",
                                   ":alice!u@h PRIVMSG bob :?OTR:abc.",
                                   "alice!u@h", "bob", "?OTR:abc."));

    /* OTR message replaced by the plain text sent. */
    otr_echo_pending_add ("srv", "bob", "?OTR:abc.", "hello");
    WEE_TEST_STR(":alice!u@h PRIVMSG bob :hello",
                 otr_echo_replace ("srv",
                                   ":alice!u@h PRIVMSG bob :?OTR:abc.",
                                   "alice!u@h", "bob", "?OTR:abc."));
    otr_echo_pending_add ("srv", "bob", "?OTR:def.", "/me waves");
    WEE_TEST_STR("@time=x :alice!u@h PRIVMSG bob :\x01" "ACTION waves\x01",
                 otr_echo_replace ("srv",
                                   "@time=x :alice!u@h PRIVMSG bob :?OTR:def.",
                                   "alice!u@h", "bob", "?OTR:def."));
    LONGS_EQUAL(0, otr_echo_pending_count);
}
