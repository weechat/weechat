/*
 * test-irc-tag.cpp - test IRC message tags functions
 *
 * Copyright (C) 2021-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdio.h>
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/plugins/irc/irc-tag.h"
#include "src/plugins/plugin.h"

extern char *irc_tag_hashtable_to_string (struct t_hashtable *tags);
}

#define WEE_CHECK_ESCAPE_VALUE(__result, __string)                      \
    escaped = irc_tag_escape_value (__string);                          \
    STRCMP_EQUAL(__result, escaped);                                    \
    free (escaped);

#define WEE_CHECK_UNESCAPE_VALUE(__result, __string)                    \
    unescaped = irc_tag_unescape_value (__string);                      \
    STRCMP_EQUAL(__result, unescaped);                                  \
    free (unescaped);

TEST_GROUP(IrcTag)
{
};

/*
 * Tests functions:
 *   irc_tag_escape_value
 */

TEST(IrcTag, EscapeValue)
{
    char *escaped;

    /* NULL/empty string */
    POINTERS_EQUAL(NULL, irc_tag_escape_value (NULL));
    WEE_CHECK_ESCAPE_VALUE("", "");

    WEE_CHECK_ESCAPE_VALUE("test", "test");
    WEE_CHECK_ESCAPE_VALUE("test\\:abc", "test;abc");
    WEE_CHECK_ESCAPE_VALUE("test\\sabc", "test abc");
    WEE_CHECK_ESCAPE_VALUE("test_\\\\_abc", "test_\\_abc");
    WEE_CHECK_ESCAPE_VALUE("test_\\r_abc", "test_\r_abc");
    WEE_CHECK_ESCAPE_VALUE("test_\\n_abc", "test_\n_abc");
    WEE_CHECK_ESCAPE_VALUE("test_\xf0\xa4\xad\xa2_abc",
                           "test_\xf0\xa4\xad\xa2_abc");
    WEE_CHECK_ESCAPE_VALUE("\\:\\s\\\\\\r\\n", "; \\\r\n");
}

/*
 * Tests functions:
 *   irc_tag_unescape_value
 */

TEST(IrcTag, UnescapeValue)
{
    char *unescaped;

    /* NULL/empty string */
    POINTERS_EQUAL(NULL, irc_tag_unescape_value (NULL));
    WEE_CHECK_UNESCAPE_VALUE("", "");

    WEE_CHECK_UNESCAPE_VALUE("test", "test");
    WEE_CHECK_UNESCAPE_VALUE("test", "test\\");
    WEE_CHECK_UNESCAPE_VALUE("test;abc", "test\\:abc");
    WEE_CHECK_UNESCAPE_VALUE("test abc", "test\\sabc");
    WEE_CHECK_UNESCAPE_VALUE("test_\\_abc", "test_\\\\_abc");
    WEE_CHECK_UNESCAPE_VALUE("test_\r_abc", "test_\\r_abc");
    WEE_CHECK_UNESCAPE_VALUE("test_\n_abc", "test_\\n_abc");
    WEE_CHECK_UNESCAPE_VALUE("test_a_abc", "test_\\a_abc");
    WEE_CHECK_UNESCAPE_VALUE("test_\xf0\xa4\xad\xa2_abc",
                             "test_\\\xf0\xa4\xad\xa2_abc");
    WEE_CHECK_UNESCAPE_VALUE("; \\\r\n", "\\:\\s\\\\\\r\\n");
}

/*
 * Tests functions:
 *   irc_tag_modifier_cb
 */

TEST(IrcTag, ModifierCallback)
{
    char *result;

    /* modifier "irc_tag_escape_value" */
    result = hook_modifier_exec (NULL, "irc_tag_escape_value", NULL, "test");
    STRCMP_EQUAL("test", result);
    free (result);

    /* modifier "irc_tag_unescape_value" */
    result = hook_modifier_exec (NULL, "irc_tag_unescape_value", NULL, "test");
    STRCMP_EQUAL("test", result);
    free (result);
}

/*
 * Tests functions:
 *   irc_tag_parse
 */

TEST(IrcTag, Parse)
{
    struct t_hashtable *hashtable;

    hashtable = hashtable_new (32,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL, NULL);

    LONGS_EQUAL(0, irc_tag_parse (NULL, hashtable, NULL));
    LONGS_EQUAL(0, irc_tag_parse ("", hashtable, NULL));

    hashtable_remove_all (hashtable);
    LONGS_EQUAL(1, irc_tag_parse ("abc", hashtable, NULL));
    LONGS_EQUAL(1, hashtable->items_count);
    POINTERS_EQUAL(NULL, (const char *)hashtable_get (hashtable, "abc"));

    hashtable_remove_all (hashtable);
    LONGS_EQUAL(1, irc_tag_parse ("abc=def", hashtable, NULL));
    LONGS_EQUAL(1, hashtable->items_count);
    STRCMP_EQUAL("def", (const char *)hashtable_get (hashtable, "abc"));

    hashtable_remove_all (hashtable);
    LONGS_EQUAL(3, irc_tag_parse ("aaa=bbb;ccc;example.com/ddd=value\\sspace",
                                  hashtable, NULL));
    LONGS_EQUAL(3, hashtable->items_count);
    STRCMP_EQUAL("bbb", (const char *)hashtable_get (hashtable, "aaa"));
    POINTERS_EQUAL(NULL, (const char *)hashtable_get (hashtable, "ccc"));
    STRCMP_EQUAL("value space",
                 (const char *)hashtable_get (hashtable, "example.com/ddd"));

    hashtable_remove_all (hashtable);
    LONGS_EQUAL(3, irc_tag_parse ("aaa=bbb;ccc;example.com/ddd=value\\sspace",
                                  hashtable, "tag_"));
    LONGS_EQUAL(3, hashtable->items_count);
    STRCMP_EQUAL("bbb", (const char *)hashtable_get (hashtable, "tag_aaa"));
    POINTERS_EQUAL(NULL, (const char *)hashtable_get (hashtable, "tag_ccc"));
    STRCMP_EQUAL("value space",
                 (const char *)hashtable_get (hashtable, "tag_example.com/ddd"));

    hashtable_free (hashtable);
}

/*
 * Tests functions:
 *   irc_tag_add_to_string_cb
 *   irc_tag_hashtable_to_string
 */

TEST(IrcTag, HashtableToString)
{
    char *str;
    struct t_hashtable *tags;

    POINTERS_EQUAL(NULL, irc_tag_hashtable_to_string (NULL));

    tags = hashtable_new (32,
                          WEECHAT_HASHTABLE_STRING,
                          WEECHAT_HASHTABLE_STRING,
                          NULL, NULL);
    CHECK(tags);

    WEE_TEST_STR("", irc_tag_hashtable_to_string (tags));

    hashtable_set (tags, "time", "2023-08-09T07:43:01.830Z");
    hashtable_set (tags, "msgid", "icqfzy7zdbpix4gy8pvzuv49kw");
    hashtable_set (tags, "test", "value with spaces");

    WEE_TEST_STR("time=2023-08-09T07:43:01.830Z;"
                 "msgid=icqfzy7zdbpix4gy8pvzuv49kw;"
                 "test=value\\swith\\sspaces",
                 irc_tag_hashtable_to_string (tags));

    hashtable_free (tags);
}

/*
 * Tests functions:
 *   irc_tag_add_to_hashtable_cb
 *   irc_tag_add_tags_to_message
 */

TEST(IrcTag, AddTagsToMessage)
{
    char *str;
    struct t_hashtable *tags;

    POINTERS_EQUAL(NULL, irc_tag_add_tags_to_message (NULL, NULL));

    WEE_TEST_STR("", irc_tag_add_tags_to_message ("", NULL));
    WEE_TEST_STR(":nick!user@host PRIVMSG #test :hello",
                 irc_tag_add_tags_to_message (
                     ":nick!user@host PRIVMSG #test :hello", NULL));
    WEE_TEST_STR("@tag1;tag2=value2 :nick!user@host PRIVMSG #test :hello",
                 irc_tag_add_tags_to_message (
                     "@tag1;tag2=value2 :nick!user@host PRIVMSG #test :hello",
                     NULL));

    tags = hashtable_new (32,
                          WEECHAT_HASHTABLE_STRING,
                          WEECHAT_HASHTABLE_STRING,
                          NULL, NULL);
    CHECK(tags);

    WEE_TEST_STR(":nick!user@host PRIVMSG #test :hello",
                 irc_tag_add_tags_to_message (
                     ":nick!user@host PRIVMSG #test :hello", tags));
    WEE_TEST_STR("@tag1;tag2=value2 :nick!user@host PRIVMSG #test :hello",
                 irc_tag_add_tags_to_message (
                     "@tag1;tag2=value2 :nick!user@host PRIVMSG #test :hello",
                     tags));

    hashtable_set (tags, "time", "2023-08-09T07:43:01.830Z");
    hashtable_set (tags, "msgid", "icqfzy7zdbpix4gy8pvzuv49kw");
    hashtable_set (tags, "test", "value with spaces");

    WEE_TEST_STR("@time=2023-08-09T07:43:01.830Z;msgid=icqfzy7zdbpix4gy8pvzuv49kw;"
                 "test=value\\swith\\sspaces :nick!user@host PRIVMSG #test :hello",
                 irc_tag_add_tags_to_message (
                     ":nick!user@host PRIVMSG #test :hello", tags));
    WEE_TEST_STR("@tag1;tag2=value2;time=2023-08-09T07:43:01.830Z;"
                 "msgid=icqfzy7zdbpix4gy8pvzuv49kw;test=value\\swith\\sspaces "
                 ":nick!user@host PRIVMSG #test :hello",
                 irc_tag_add_tags_to_message (
                     "@tag1;tag2=value2 :nick!user@host PRIVMSG #test :hello",
                     tags));

    hashtable_free (tags);
}
