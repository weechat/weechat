/*
 * SPDX-FileCopyrightText: 2019-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC configuration functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "src/plugins/irc/irc-config.h"
}

TEST_GROUP(IrcConfig)
{
};

/*
 * Test functions:
 *   irc_config_notice_nick_notify
 */

TEST(IrcConfig, NoticeNickNotify)
{
    LONGS_EQUAL(0, irc_config_notice_nick_notify (NULL));
    LONGS_EQUAL(1, irc_config_notice_nick_notify (""));

    LONGS_EQUAL(1, irc_config_notice_nick_notify ("test"));
    LONGS_EQUAL(1, irc_config_notice_nick_notify ("memoserv"));

    /* Default list of nicks preventing notification */
    LONGS_EQUAL(0, irc_config_notice_nick_notify ("chanserv"));
    LONGS_EQUAL(0, irc_config_notice_nick_notify ("ChanServ"));
    LONGS_EQUAL(0, irc_config_notice_nick_notify ("nickserv"));
    LONGS_EQUAL(0, irc_config_notice_nick_notify ("NickServ"));
}

/*
 * Test functions:
 *   irc_config_check_autojoin
 */

TEST(IrcConfig, CheckAutojoin)
{
    /* NULL/empty string */
    LONGS_EQUAL(1, irc_config_check_autojoin (NULL));
    LONGS_EQUAL(1, irc_config_check_autojoin (""));

    /* Wrong delimiter: space instead of comma */
    LONGS_EQUAL(0, irc_config_check_autojoin ("#chan1 #chan2 #chan3"));

    /* No spaces allowed around comma */
    LONGS_EQUAL(0, irc_config_check_autojoin ("#chan1, #chan2"));
    LONGS_EQUAL(0, irc_config_check_autojoin ("#chan1 ,#chan2"));
    LONGS_EQUAL(0, irc_config_check_autojoin ("#chan1 , #chan2"));
    LONGS_EQUAL(0, irc_config_check_autojoin ("#chan1, #chan2, #chan3"));
    LONGS_EQUAL(0, irc_config_check_autojoin ("#chan1 ,#chan2 ,#chan3"));
    LONGS_EQUAL(0, irc_config_check_autojoin ("#chan1 , #chan2 , #chan3"));
    LONGS_EQUAL(0, irc_config_check_autojoin ("#chan1,#chan2 key1, key2"));
    LONGS_EQUAL(0, irc_config_check_autojoin ("#chan1,#chan2 key1 ,key2"));
    LONGS_EQUAL(0, irc_config_check_autojoin ("#chan1,#chan2 key1 , key2"));

    /* Too many keys */
    LONGS_EQUAL(0, irc_config_check_autojoin ("#chan1,#chan2 key1,key2,key3"));

    /* Correct values */
    LONGS_EQUAL(1, irc_config_check_autojoin ("#chan1"));
    LONGS_EQUAL(1, irc_config_check_autojoin ("#chan1 "));
    LONGS_EQUAL(1, irc_config_check_autojoin ("#chan1  "));
    LONGS_EQUAL(1, irc_config_check_autojoin (" #chan1"));
    LONGS_EQUAL(1, irc_config_check_autojoin ("  #chan1"));
    LONGS_EQUAL(1, irc_config_check_autojoin (" #chan1 "));
    LONGS_EQUAL(1, irc_config_check_autojoin ("  #chan1  "));
    LONGS_EQUAL(1, irc_config_check_autojoin ("#c1,#c2"));
    LONGS_EQUAL(1, irc_config_check_autojoin ("#c1,#c2,#c3"));
    LONGS_EQUAL(1, irc_config_check_autojoin ("#c1,#c2,#c3 key1"));
    LONGS_EQUAL(1, irc_config_check_autojoin ("#c1,#c2,#c3 key1,key2"));
    LONGS_EQUAL(1, irc_config_check_autojoin ("#c1,#c2,#c3 key1,key2,key3"));
}
