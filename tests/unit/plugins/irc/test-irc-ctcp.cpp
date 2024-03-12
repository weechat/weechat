/*
 * test-irc-ctcp.cpp - test IRC CTCP functions
 *
 * Copyright (C) 2023-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <sys/utsname.h>
#include "src/core/core-config-file.h"
#include "src/core/core-hook.h"
#include "src/plugins/irc/irc-config.h"
#include "src/plugins/irc/irc-ctcp.h"
#include "src/plugins/irc/irc-server.h"

extern char *irc_ctcp_get_supported_ctcp (struct t_irc_server *server);
}

TEST_GROUP(IrcCtcp)
{
};

/*
 * Tests functions:
 *   irc_ctcp_convert_legacy_format
 */

TEST(IrcCtcp, ConvertLegacyFormat)
{
    char *str;

    WEE_TEST_STR(NULL, irc_ctcp_convert_legacy_format (NULL));
    WEE_TEST_STR("", irc_ctcp_convert_legacy_format (""));

    WEE_TEST_STR("abc", irc_ctcp_convert_legacy_format ("abc"));

    WEE_TEST_STR(
        "${clientinfo} ${version} ${git} ${versiongit} ${date} "
        "${osinfo} ${site} ${download} ${time} ${username} ${realname}",
        irc_ctcp_convert_legacy_format (
            "$clientinfo $version $git $versiongit $date "
            "$osinfo $site $download $time $username $realname"));
}

/*
 * Tests functions:
 *   irc_ctcp_get_default_reply
 */

TEST(IrcCtcp, GetDefaultReply)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_ctcp_get_reply
 */

TEST(IrcCtcp, GetReply)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_ctcp_display_request
 */

TEST(IrcCtcp, DisplayRequest)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_ctcp_display_reply_from_nick
 */

TEST(IrcCtcp, DisplayReplyFromNick)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_ctcp_reply_to_nick
 */

TEST(IrcCtcp, ReplyToNick)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_ctcp_get_supported_ctcp
 */

TEST(IrcCtcp, GetSupportedCtcp)
{
    struct t_irc_server *server;
    struct t_config_option *ptr_option;
    char *str;

    server = irc_server_alloc ("server");
    CHECK(server);

    WEE_TEST_STR("ACTION CLIENTINFO DCC PING SOURCE TIME VERSION",
                 irc_ctcp_get_supported_ctcp (server));

    config_file_option_set_with_string ("irc.ctcp.version", "");
    WEE_TEST_STR("ACTION CLIENTINFO DCC PING SOURCE TIME",
                 irc_ctcp_get_supported_ctcp (server));

    config_file_option_set_with_string ("irc.ctcp.time", "");
    WEE_TEST_STR("ACTION CLIENTINFO DCC PING SOURCE",
                 irc_ctcp_get_supported_ctcp (server));

    config_file_option_set_with_string ("irc.ctcp.version", "test");
    WEE_TEST_STR("ACTION CLIENTINFO DCC PING SOURCE VERSION",
                 irc_ctcp_get_supported_ctcp (server));

    config_file_search_with_string ("irc.ctcp.version", NULL, NULL, &ptr_option, NULL);
    config_file_option_unset (ptr_option);

    config_file_search_with_string ("irc.ctcp.time", NULL, NULL, &ptr_option, NULL);
    config_file_option_unset (ptr_option);

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_ctcp_eval_reply
 */

TEST(IrcCtcp, EvalReply)
{
    struct t_irc_server *server;
    char *str, *info_version, *info_version_git, *info_date, *info_site;
    char *info_site_download, *username, *realname, buf[4096];
    struct utsname *buf_uname;

    server = irc_server_alloc ("server");
    CHECK(server);

    info_version = hook_info_get (NULL, "version", "");
    info_version_git = hook_info_get (NULL, "version_git", "");
    info_date = hook_info_get (NULL, "date", "");
    info_site = hook_info_get (NULL, "weechat_site", "");
    info_site_download = hook_info_get (NULL, "weechat_site_download", "");

    WEE_TEST_STR(NULL, irc_ctcp_eval_reply (NULL, NULL));
    WEE_TEST_STR(NULL, irc_ctcp_eval_reply (NULL, ""));

    WEE_TEST_STR(NULL, irc_ctcp_eval_reply (server, NULL));
    WEE_TEST_STR("", irc_ctcp_eval_reply (server, ""));

    WEE_TEST_STR("abc", irc_ctcp_eval_reply (server, "abc"));

    /* ${clientinfo} */
    WEE_TEST_STR("ACTION CLIENTINFO DCC PING SOURCE TIME VERSION",
                 irc_ctcp_eval_reply (server, "${clientinfo}"));

    /* ${version} */
    WEE_TEST_STR(info_version, irc_ctcp_eval_reply (server, "${version}"));

    /* ${git} */
    WEE_TEST_STR(info_version_git, irc_ctcp_eval_reply (server, "${git}"));

    /* ${versiongit} */
    snprintf (buf, sizeof (buf),
              "%s (git: %s)",
              info_version,
              info_version_git);
    WEE_TEST_STR(buf, irc_ctcp_eval_reply (server, "${versiongit}"));

    /* ${compilation} */
    WEE_TEST_STR(info_date, irc_ctcp_eval_reply (server, "${compilation}"));

    /* ${osinfo} */
    buf_uname = (struct utsname *)malloc (sizeof (struct utsname));
    CHECK(buf_uname);
    CHECK(uname (buf_uname) >= 0);
    snprintf (buf, sizeof (buf),
              "%s %s / %s",
              buf_uname->sysname,
              buf_uname->release,
              buf_uname->machine);
    WEE_TEST_STR(buf, irc_ctcp_eval_reply (server, "${osinfo}"));
    free (buf_uname);

    /* ${site} */
    WEE_TEST_STR(info_site, irc_ctcp_eval_reply (server, "${site}"));

    /* ${download} */
    WEE_TEST_STR(info_site_download, irc_ctcp_eval_reply (server, "${download}"));

    /* ${time} */
    str = irc_ctcp_eval_reply (server, "${time}");
    CHECK(strcmp (str, "") != 0);
    free (str);

    /* ${username} */
    username = irc_server_eval_expression (
        server,
        CONFIG_STRING(irc_config_server_default[IRC_SERVER_OPTION_USERNAME]));
    CHECK(username);
    WEE_TEST_STR(username, irc_ctcp_eval_reply (server, "${username}"));
    free (username);

    /* ${realname} */
    realname = irc_server_eval_expression (
        server,
        CONFIG_STRING(irc_config_server_default[IRC_SERVER_OPTION_REALNAME]));
    CHECK(realname);
    WEE_TEST_STR(realname, irc_ctcp_eval_reply (server, "${realname}"));
    free (realname);

    free (info_version);
    free (info_version_git);
    free (info_date);
    free (info_site);
    free (info_site_download);

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_ctcp_dcc_filename_without_quotes
 */

TEST(IrcCtcp, DccFilenameWithoutQuotes)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_ctcp_recv_dcc
 */

TEST(IrcCtcp, RecvDcc)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_ctcp_recv
 */

TEST(IrcCtcp, Recv)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_ctcp_send
 */

TEST(IrcCtcp, Send)
{
    /* TODO: write tests */
}
