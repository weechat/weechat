/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test OTR message functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include "src/core/core-string.h"
#include "src/plugins/otr/otr-message.h"

extern void otr_message_utf8_append (char **string, unsigned long code_point);
}

TEST_GROUP(OtrMessage)
{
};

/*
 * Test functions:
 *   otr_message_is_ctcp
 *   otr_message_is_action
 */

TEST(OtrMessage, IsCtcpIsAction)
{
    LONGS_EQUAL(0, otr_message_is_ctcp (""));
    LONGS_EQUAL(0, otr_message_is_ctcp ("abc"));
    LONGS_EQUAL(0, otr_message_is_ctcp ("\x01"));
    LONGS_EQUAL(0, otr_message_is_ctcp ("\x01VERSION"));
    LONGS_EQUAL(1, otr_message_is_ctcp ("\x01\x01"));
    LONGS_EQUAL(1, otr_message_is_ctcp ("\x01VERSION\x01"));
    LONGS_EQUAL(1, otr_message_is_ctcp ("\x01" "ACTION waves\x01"));

    LONGS_EQUAL(0, otr_message_is_action (""));
    LONGS_EQUAL(0, otr_message_is_action ("ACTION waves"));
    LONGS_EQUAL(0, otr_message_is_action ("\x01VERSION\x01"));
    LONGS_EQUAL(0, otr_message_is_action ("\x01" "ACTION waves"));
    LONGS_EQUAL(1, otr_message_is_action ("\x01" "ACTION waves\x01"));
}

/*
 * Test functions:
 *   otr_message_plain_from_irc
 */

TEST(OtrMessage, PlainFromIrc)
{
    char *str;

    WEE_TEST_STR("", otr_message_plain_from_irc (""));
    WEE_TEST_STR("hello", otr_message_plain_from_irc ("hello"));
    WEE_TEST_STR("\x01VERSION\x01",
                 otr_message_plain_from_irc ("\x01VERSION\x01"));
    WEE_TEST_STR("/me ", otr_message_plain_from_irc ("\x01" "ACTION \x01"));
    WEE_TEST_STR("/me waves",
                 otr_message_plain_from_irc ("\x01" "ACTION waves\x01"));
}

/*
 * Test functions:
 *   otr_message_html_escape
 */

TEST(OtrMessage, HtmlEscape)
{
    char *str;

    WEE_TEST_STR("", otr_message_html_escape (""));
    WEE_TEST_STR("abc", otr_message_html_escape ("abc"));
    WEE_TEST_STR("a &lt;b&gt; &amp; c",
                 otr_message_html_escape ("a <b> & c"));
}

/*
 * Test functions:
 *   otr_message_utf8_append
 */

TEST(OtrMessage, Utf8Append)
{
    char **string;

    string = string_dyn_alloc (64);
    CHECK(string);

    otr_message_utf8_append (string, 0);
    STRCMP_EQUAL("", *string);
    otr_message_utf8_append (string, 0x41);
    STRCMP_EQUAL("A", *string);
    otr_message_utf8_append (string, 0xE9);
    STRCMP_EQUAL("Aé", *string);
    otr_message_utf8_append (string, 0x20AC);
    STRCMP_EQUAL("Aé€", *string);
    otr_message_utf8_append (string, 0x1F600);
    STRCMP_EQUAL("Aé€😀", *string);
    otr_message_utf8_append (string, 0x110000);
    STRCMP_EQUAL("Aé€😀", *string);

    string_dyn_free (string, 1);
}

/*
 * Test functions:
 *   otr_message_html_filter
 */

TEST(OtrMessage, HtmlFilter)
{
    char *str;

    WEE_TEST_STR("", otr_message_html_filter (""));
    WEE_TEST_STR("hello", otr_message_html_filter ("hello"));
    WEE_TEST_STR("a < b", otr_message_html_filter ("a < b"));
    WEE_TEST_STR("a\nb", otr_message_html_filter ("a<br>b"));
    WEE_TEST_STR("a\nb", otr_message_html_filter ("a<BR/>b"));
    WEE_TEST_STR("bold", otr_message_html_filter ("<b>bold</b>"));
    WEE_TEST_STR("[link](https://x.org)",
                 otr_message_html_filter ("<a href=\"https://x.org\">link</a>"));
    WEE_TEST_STR("[link](https://x.org)",
                 otr_message_html_filter ("<a href='https://x.org'>link</a>"));
    WEE_TEST_STR("[https://y.org]",
                 otr_message_html_filter (
                     "<a href=\"https://y.org\">https://y.org</a>"));
    WEE_TEST_STR("&<>\"' ",
                 otr_message_html_filter ("&amp;&lt;&gt;&quot;&apos;&nbsp;"));
    WEE_TEST_STR("éé", otr_message_html_filter ("&#233;&#xE9;"));
    WEE_TEST_STR("&unknown;", otr_message_html_filter ("&unknown;"));
    WEE_TEST_STR("&;", otr_message_html_filter ("&;"));
    WEE_TEST_STR("&#0;", otr_message_html_filter ("&#0;"));
}

/*
 * Test functions:
 *   otr_message_add_decrypted_line
 *   otr_message_build_in_privmsgs
 */

TEST(OtrMessage, BuildInPrivmsgs)
{
    char *str;

    /* Plain text, not decrypted. */
    WEE_TEST_STR("",
                 otr_message_build_in_privmsgs ("", "bob!u@h", "alice", "",
                                                "", 0));
    WEE_TEST_STR(":bob!u@h PRIVMSG alice :hello",
                 otr_message_build_in_privmsgs ("", "bob!u@h", "alice",
                                                "hello", "", 0));
    WEE_TEST_STR("@time=x :bob!u@h PRIVMSG alice :prefix: hello",
                 otr_message_build_in_privmsgs ("@time=x ", "bob!u@h",
                                                "alice", "hello", "prefix: ",
                                                0));
    WEE_TEST_STR(":bob!u@h PRIVMSG alice :l1\r\n"
                 ":bob!u@h PRIVMSG alice :l2",
                 otr_message_build_in_privmsgs ("", "bob!u@h", "alice",
                                                "l1\r\nl2", "", 0));

    /* Decrypted: "/me" becomes a CTCP ACTION, CTCP markers are removed. */
    WEE_TEST_STR(":bob!u@h PRIVMSG alice :hello",
                 otr_message_build_in_privmsgs ("", "bob!u@h", "alice",
                                                "hello", "", 1));
    WEE_TEST_STR(":bob!u@h PRIVMSG alice :\x01" "ACTION waves\x01",
                 otr_message_build_in_privmsgs ("", "bob!u@h", "alice",
                                                "/me waves", "", 1));
    WEE_TEST_STR(":bob!u@h PRIVMSG alice :\x01" "ACTION waves\x01",
                 otr_message_build_in_privmsgs ("", "bob!u@h", "alice",
                                                "\x01" "ACTION waves\x01", "",
                                                1));
    WEE_TEST_STR(":bob!u@h PRIVMSG alice :VERSION",
                 otr_message_build_in_privmsgs ("", "bob!u@h", "alice",
                                                "\x01VERSION\x01", "", 1));
    WEE_TEST_STR(":bob!u@h PRIVMSG alice :l1\r\n"
                 ":bob!u@h PRIVMSG alice :\x01" "ACTION l2\x01",
                 otr_message_build_in_privmsgs ("", "bob!u@h", "alice",
                                                "l1\n/me l2", "", 1));
}

/*
 * Test functions:
 *   otr_message_get_tags
 */

TEST(OtrMessage, GetTags)
{
    char *str;

    POINTERS_EQUAL(NULL, otr_message_get_tags (""));
    POINTERS_EQUAL(NULL, otr_message_get_tags (":bob!u@h PRIVMSG alice :hi"));
    POINTERS_EQUAL(NULL, otr_message_get_tags ("@time=x"));
    WEE_TEST_STR("@time=x ",
                 otr_message_get_tags ("@time=x :bob!u@h PRIVMSG alice :hi"));
}
