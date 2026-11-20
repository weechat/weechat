/*
 * SPDX-FileCopyrightText: 2023-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC CTCP functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"
#include "tests-record.h"

extern "C"
{
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include "src/core/core-config-file.h"
#include "src/core/core-hook.h"
#include "src/core/core-infolist.h"
#include "src/core/core-string.h"
#include "src/plugins/plugin.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-config.h"
#include "src/plugins/irc/irc-ctcp.h"
#include "src/plugins/irc/irc-server.h"
#include "src/plugins/logger/logger-config.h"

extern char *irc_ctcp_get_supported_ctcp (struct t_irc_server *server);
extern char *irc_ctcp_dcc_filename_without_quotes (const char *filename);
}

#define IRC_FAKE_SERVER "fake_ctcp"
#define SRV_BUFFER "irc.server." IRC_FAKE_SERVER
#define CHAN_BUFFER "irc." IRC_FAKE_SERVER ".#test"
#define PV_BUFFER "irc." IRC_FAKE_SERVER ".bob"

#define RECV(__irc_msg)                                                 \
    server_recv (__irc_msg);

/* Check that a message is displayed in a buffer. */
#define CHECK_MSG(__buffer, __prefix, __message)                        \
    if (!record_search (__buffer, __prefix, __message, NULL))           \
    {                                                                   \
        char **msg = string_dyn_alloc (256);                            \
        string_dyn_concat (msg, "Message not displayed in ", -1);       \
        string_dyn_concat (msg, __buffer, -1);                          \
        string_dyn_concat (msg, ": \"", -1);                            \
        string_dyn_concat (msg, __message, -1);                         \
        string_dyn_concat (msg, "\"\nAll messages displayed:\n", -1);   \
        record_dump (msg);                                              \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

/* Check that no message is displayed in a buffer. */
#define CHECK_NO_MSG(__buffer, __prefix, __message)                     \
    POINTERS_EQUAL(NULL, record_search (__buffer, __prefix, __message, \
                                        NULL));

TEST_GROUP(IrcCtcp)
{
};

TEST_GROUP(IrcCtcpWithServer)
{
    struct t_irc_server *server = NULL;
    struct t_hook *hook_irc_out = NULL;
    struct t_hook *hook_xfer[3] = { NULL, NULL, NULL };
    char **sent = NULL;
    char **xfer = NULL;

    /* Save messages sent to the server (separated by "\n"). */
    static int irc_out_cb (const void *pointer, void *data,
                           const char *signal, const char *type_data,
                           void *signal_data)
    {
        char **ptr_sent;

        (void) data;
        (void) signal;
        (void) type_data;

        ptr_sent = (char **)pointer;
        string_dyn_concat (ptr_sent, (const char *)signal_data, -1);
        string_dyn_concat (ptr_sent, "\n", -1);
        return WEECHAT_RC_OK;
    }

    /*
     * Save some variables of the infolist sent to xfer plugin, and "eat" the
     * signal, so that no xfer is created.
     */
    static int xfer_cb (const void *pointer, void *data,
                        const char *signal, const char *type_data,
                        void *signal_data)
    {
        struct t_infolist *ptr_infolist;
        char **ptr_xfer, str_port[32];
        const char *vars[] = { "type_string", "filename", "remote_address",
                               "size", "start_resume", "token",
                               "remote_nick", NULL };
        const char *value;
        int i;

        (void) data;
        (void) type_data;

        ptr_xfer = (char **)pointer;
        ptr_infolist = (struct t_infolist *)signal_data;
        string_dyn_concat (ptr_xfer, signal, -1);
        infolist_reset_item_cursor (ptr_infolist);
        if (infolist_next (ptr_infolist))
        {
            for (i = 0; vars[i]; i++)
            {
                if (!infolist_search_var (ptr_infolist, vars[i]))
                    continue;
                value = infolist_string (ptr_infolist, vars[i]);
                string_dyn_concat (ptr_xfer, " ", -1);
                string_dyn_concat (ptr_xfer, vars[i], -1);
                string_dyn_concat (ptr_xfer, "=", -1);
                string_dyn_concat (ptr_xfer, (value) ? value : "(null)", -1);
            }
            snprintf (str_port, sizeof (str_port), " port=%d",
                      infolist_integer (ptr_infolist, "port"));
            string_dyn_concat (ptr_xfer, str_port, -1);
        }
        return WEECHAT_RC_OK_EAT;
    }

    void server_recv (const char *command)
    {
        char str_command[8192];

        record_start ();
        string_dyn_copy (sent, NULL);
        string_dyn_copy (xfer, NULL);
        snprintf (str_command, sizeof (str_command),
                  "/command -buffer " SRV_BUFFER " irc "
                  "/server fakerecv \"%s\"",
                  command);
        run_cmd_quiet (str_command);
        record_stop ();
    }

    void setup ()
    {
        sent = string_dyn_alloc (256);
        xfer = string_dyn_alloc (256);
        hook_irc_out = hook_signal (NULL, IRC_FAKE_SERVER ",irc_out1_*",
                                    &irc_out_cb, sent, NULL);
        hook_xfer[0] = hook_signal (NULL, "2000|xfer_add",
                                    &xfer_cb, xfer, NULL);
        hook_xfer[1] = hook_signal (NULL, "2000|xfer_accept_resume",
                                    &xfer_cb, xfer, NULL);
        hook_xfer[2] = hook_signal (NULL, "2000|xfer_start_resume",
                                    &xfer_cb, xfer, NULL);
        config_file_option_set (logger_config_look_backlog, "0", 1);
        run_cmd_quiet ("/mute /server add " IRC_FAKE_SERVER " fake:127.0.0.1 "
                       "-nicks=alice -anti_flood=0");
        run_cmd_quiet ("/mute /connect " IRC_FAKE_SERVER);
        server = irc_server_search (IRC_FAKE_SERVER);
        CHECK(server);
        RECV(":server 001 alice :Welcome");
        RECV(":alice!user_a@host_a JOIN #test");
        RECV(":bob!user_b@host_b JOIN #test");
    }

    void teardown ()
    {
        int i;

        record_stop ();
        unhook (hook_irc_out);
        for (i = 0; i < 3; i++)
        {
            unhook (hook_xfer[i]);
        }
        run_cmd_quiet ("/mute /disconnect " IRC_FAKE_SERVER);
        run_cmd_quiet ("/mute /server del " IRC_FAKE_SERVER);
        server = NULL;
        string_dyn_free (sent, 1);
        string_dyn_free (xfer, 1);
        config_file_option_reset (logger_config_look_backlog, 1);
        config_file_option_reset (irc_config_look_display_ctcp_blocked, 1);
        config_file_option_reset (irc_config_look_display_ctcp_reply, 1);
        config_file_option_reset (irc_config_look_display_ctcp_unknown, 1);
    }
};

/*
 * Test functions:
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
 * Test functions:
 *   irc_ctcp_get_default_reply
 */

TEST(IrcCtcp, GetDefaultReply)
{
    STRCMP_EQUAL("${clientinfo}", irc_ctcp_get_default_reply ("clientinfo"));
    STRCMP_EQUAL("${download}", irc_ctcp_get_default_reply ("source"));
    STRCMP_EQUAL("${time}", irc_ctcp_get_default_reply ("time"));
    STRCMP_EQUAL("WeeChat ${version}", irc_ctcp_get_default_reply ("version"));
    STRCMP_EQUAL("WeeChat ${version}", irc_ctcp_get_default_reply ("VERSION"));

    POINTERS_EQUAL(NULL, irc_ctcp_get_default_reply (""));
    POINTERS_EQUAL(NULL, irc_ctcp_get_default_reply ("ping"));
    POINTERS_EQUAL(NULL, irc_ctcp_get_default_reply ("xxx"));
}

/*
 * Test functions:
 *   irc_ctcp_get_reply
 */

TEST(IrcCtcp, GetReply)
{
    struct t_irc_server *server;
    struct t_config_option *ptr_option;

    server = irc_server_alloc ("server");
    CHECK(server);

    /* Default replies. */
    STRCMP_EQUAL("WeeChat ${version}", irc_ctcp_get_reply (server, "version"));
    STRCMP_EQUAL("WeeChat ${version}", irc_ctcp_get_reply (server, "VERSION"));
    STRCMP_EQUAL("${time}", irc_ctcp_get_reply (server, "time"));
    POINTERS_EQUAL(NULL, irc_ctcp_get_reply (server, "xxx"));

    /* Reply defined for all servers. */
    config_file_option_set_with_string ("irc.ctcp.version", "global");
    config_file_option_set_with_string ("irc.ctcp.xxx", "custom");
    STRCMP_EQUAL("global", irc_ctcp_get_reply (server, "VERSION"));
    STRCMP_EQUAL("custom", irc_ctcp_get_reply (server, "XXX"));

    /* Reply defined for the server (it has priority). */
    config_file_option_set_with_string ("irc.ctcp.server.version", "");
    STRCMP_EQUAL("", irc_ctcp_get_reply (server, "VERSION"));

    config_file_search_with_string ("irc.ctcp.server.version", NULL, NULL,
                                    &ptr_option, NULL);
    config_file_option_unset (ptr_option);
    config_file_search_with_string ("irc.ctcp.version", NULL, NULL,
                                    &ptr_option, NULL);
    config_file_option_unset (ptr_option);
    config_file_search_with_string ("irc.ctcp.xxx", NULL, NULL,
                                    &ptr_option, NULL);
    config_file_option_unset (ptr_option);

    STRCMP_EQUAL("WeeChat ${version}", irc_ctcp_get_reply (server, "version"));
    POINTERS_EQUAL(NULL, irc_ctcp_get_reply (server, "xxx"));

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_ctcp_parse_type_arguments
 */

TEST(IrcCtcp, ParseTypeArguments)
{
    char *type, *args;

    /* Invalid arguments: nothing is done. */
    irc_ctcp_parse_type_arguments (NULL, &type, &args);
    irc_ctcp_parse_type_arguments ("\001VERSION\001", NULL, &args);
    irc_ctcp_parse_type_arguments ("\001VERSION\001", &type, NULL);

    irc_ctcp_parse_type_arguments ("", &type, &args);
    POINTERS_EQUAL(NULL, type);
    POINTERS_EQUAL(NULL, args);

    irc_ctcp_parse_type_arguments ("VERSION", &type, &args);
    POINTERS_EQUAL(NULL, type);
    POINTERS_EQUAL(NULL, args);

    /* Missing "\001" at the end. */
    irc_ctcp_parse_type_arguments ("\001VERSION", &type, &args);
    POINTERS_EQUAL(NULL, type);
    POINTERS_EQUAL(NULL, args);

    irc_ctcp_parse_type_arguments ("\001\001", &type, &args);
    STRCMP_EQUAL("", type);
    POINTERS_EQUAL(NULL, args);
    free (type);

    irc_ctcp_parse_type_arguments ("\001VERSION\001", &type, &args);
    STRCMP_EQUAL("VERSION", type);
    POINTERS_EQUAL(NULL, args);
    free (type);

    irc_ctcp_parse_type_arguments ("\001PING 123 456\001", &type, &args);
    STRCMP_EQUAL("PING", type);
    STRCMP_EQUAL("123 456", args);
    free (type);
    free (args);

    irc_ctcp_parse_type_arguments ("\001ACTION \001", &type, &args);
    STRCMP_EQUAL("ACTION", type);
    STRCMP_EQUAL("", args);
    free (type);
    free (args);

    /* Text after the end of CTCP is ignored. */
    irc_ctcp_parse_type_arguments ("\001VERSION\001 extra", &type, &args);
    STRCMP_EQUAL("VERSION", type);
    POINTERS_EQUAL(NULL, args);
    free (type);

    irc_ctcp_parse_type_arguments ("\001PING 123\001 extra", &type, &args);
    STRCMP_EQUAL("PING", type);
    STRCMP_EQUAL("123", args);
    free (type);
    free (args);
}

/*
 * Test functions:
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
 * Test functions:
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
 * Test functions:
 *   irc_ctcp_dcc_filename_without_quotes
 */

TEST(IrcCtcp, DccFilenameWithoutQuotes)
{
    char *str;

    WEE_TEST_STR("", irc_ctcp_dcc_filename_without_quotes (""));
    WEE_TEST_STR("\"", irc_ctcp_dcc_filename_without_quotes ("\""));
    WEE_TEST_STR("", irc_ctcp_dcc_filename_without_quotes ("\"\""));
    WEE_TEST_STR("file.txt",
                 irc_ctcp_dcc_filename_without_quotes ("file.txt"));
    WEE_TEST_STR("file.txt",
                 irc_ctcp_dcc_filename_without_quotes ("\"file.txt\""));
    WEE_TEST_STR("my file.txt",
                 irc_ctcp_dcc_filename_without_quotes ("\"my file.txt\""));
    WEE_TEST_STR("\"file.txt",
                 irc_ctcp_dcc_filename_without_quotes ("\"file.txt"));
    WEE_TEST_STR("file.txt\"",
                 irc_ctcp_dcc_filename_without_quotes ("file.txt\""));
}


/*
 * Test functions:
 *   irc_ctcp_recv
 *   irc_ctcp_display_request
 *   irc_ctcp_reply_to_nick
 *   irc_ctcp_display_reply_to_nick
 *   irc_ctcp_display_reply_to_nick_internal
 */

TEST(IrcCtcpWithServer, RecvReply)
{
    /* CTCP VERSION in private. */
    RECV(":bob!user_b@host_b PRIVMSG alice :\001VERSION\001");
    CHECK(strstr (*sent, "NOTICE bob :\001VERSION WeeChat "));
    CHECK_MSG(SRV_BUFFER, "--", "CTCP requested by bob: VERSION");
    CHECK(record_search_msg_regex (SRV_BUFFER,
                                   "^CTCP reply to bob: VERSION WeeChat "));

    /* CTCP VERSION in channel. */
    RECV(":bob!user_b@host_b PRIVMSG #test :\001VERSION\001");
    CHECK(strstr (*sent, "NOTICE bob :\001VERSION WeeChat "));
    CHECK_MSG(CHAN_BUFFER, "--", "CTCP requested by bob: VERSION");

    /* CTCP with arguments (ignored), lower case. */
    RECV(":bob!user_b@host_b PRIVMSG alice :\001time some args\001");
    CHECK(strstr (*sent, "NOTICE bob :\001TIME ") == *sent);
    CHECK_MSG(SRV_BUFFER, "--", "CTCP requested by bob: time some args");

    /* Custom reply. */
    config_file_option_set_with_string ("irc.ctcp.xxx", "custom ${version}");
    RECV(":bob!user_b@host_b PRIVMSG alice :\001XXX\001");
    CHECK(strstr (*sent, "NOTICE bob :\001XXX custom ") == *sent);
    run_cmd_quiet ("/mute /unset irc.ctcp.xxx");

    /* Text after the end of CTCP is ignored: only one reply is sent. */
    RECV(":bob!user_b@host_b PRIVMSG alice :\001VERSION\001 VERSION");
    CHECK(strstr (*sent, "NOTICE bob :\001VERSION WeeChat ") == *sent);
    POINTERS_EQUAL(NULL, strstr (*sent + 1, "NOTICE"));
    LONGS_EQUAL(2, record_count_messages ());

    /* Reply not displayed. */
    config_file_option_set (irc_config_look_display_ctcp_reply, "off", 1);
    RECV(":bob!user_b@host_b PRIVMSG alice :\001VERSION\001");
    CHECK(strstr (*sent, "NOTICE bob :\001VERSION WeeChat "));
    LONGS_EQUAL(1, record_count_messages ());
}

/*
 * Test functions:
 *   irc_ctcp_recv (CTCP blocked)
 *   irc_ctcp_display_request
 */

TEST(IrcCtcpWithServer, RecvBlocked)
{
    config_file_option_set_with_string ("irc.ctcp.version", "");

    RECV(":bob!user_b@host_b PRIVMSG alice :\001VERSION\001");
    STRCMP_EQUAL("", *sent);
    CHECK_MSG(SRV_BUFFER, "--", "CTCP requested by bob: VERSION (blocked)");

    config_file_option_set (irc_config_look_display_ctcp_blocked, "off", 1);
    RECV(":bob!user_b@host_b PRIVMSG alice :\001VERSION\001");
    STRCMP_EQUAL("", *sent);
    LONGS_EQUAL(0, record_count_messages ());

    run_cmd_quiet ("/mute /unset irc.ctcp.version");
}

/*
 * Test functions:
 *   irc_ctcp_recv (CTCP PING)
 */

TEST(IrcCtcpWithServer, RecvPing)
{
    RECV(":bob!user_b@host_b PRIVMSG alice :\001PING 1234 5678\001");
    STRCMP_EQUAL("NOTICE bob :\001PING 1234 5678\001\n", *sent);
    CHECK_MSG(SRV_BUFFER, "--", "CTCP requested by bob: PING 1234 5678");
    CHECK_MSG(SRV_BUFFER, "--", "CTCP reply to bob: PING 1234 5678");

    RECV(":bob!user_b@host_b PRIVMSG alice :\001PING\001");
    STRCMP_EQUAL("NOTICE bob :\001PING\001\n", *sent);
    CHECK_MSG(SRV_BUFFER, "--", "CTCP requested by bob: PING");

    /* "\001" in arguments is replaced by a space (CVE-2022-2663). */
    RECV(":bob!user_b@host_b PRIVMSG alice :\001PING a\001b\001");
    STRCMP_EQUAL("NOTICE bob :\001PING a b\001\n", *sent);

    /* Custom reply. */
    config_file_option_set_with_string ("irc.ctcp.ping", "pong");
    RECV(":bob!user_b@host_b PRIVMSG alice :\001PING 1234\001");
    STRCMP_EQUAL("NOTICE bob :\001PING pong\001\n", *sent);

    /* Blocked. */
    config_file_option_set_with_string ("irc.ctcp.ping", "");
    RECV(":bob!user_b@host_b PRIVMSG alice :\001PING 1234\001");
    STRCMP_EQUAL("", *sent);
    CHECK_MSG(SRV_BUFFER, "--", "CTCP requested by bob: PING 1234 (blocked)");

    run_cmd_quiet ("/mute /unset irc.ctcp.ping");
}

/*
 * Test functions:
 *   irc_ctcp_recv (unknown CTCP)
 */

TEST(IrcCtcpWithServer, RecvUnknown)
{
    RECV(":bob!user_b@host_b PRIVMSG alice :\001UNKNOWN some args\001");
    STRCMP_EQUAL("", *sent);
    CHECK_MSG(SRV_BUFFER, "--",
              "Unknown CTCP requested by bob: UNKNOWN some args");

    RECV(":bob!user_b@host_b PRIVMSG #test :\001UNKNOWN\001");
    STRCMP_EQUAL("", *sent);
    CHECK_MSG(CHAN_BUFFER, "--", "Unknown CTCP requested by bob: UNKNOWN");

    config_file_option_set (irc_config_look_display_ctcp_unknown, "off", 1);
    RECV(":bob!user_b@host_b PRIVMSG alice :\001UNKNOWN\001");
    STRCMP_EQUAL("", *sent);
    LONGS_EQUAL(0, record_count_messages ());
}

/*
 * Test functions:
 *   irc_ctcp_recv (CTCP ACTION)
 */

TEST(IrcCtcpWithServer, RecvAction)
{
    struct t_irc_channel *ptr_channel;

    /* Action in channel. */
    RECV(":bob!user_b@host_b PRIVMSG #test :\001ACTION waves\001");
    STRCMP_EQUAL("", *sent);
    CHECK_MSG(CHAN_BUFFER, " *", "bob waves");

    /* Text after the end of CTCP is ignored. */
    RECV(":bob!user_b@host_b PRIVMSG #test :\001ACTION waves\001 extra");
    CHECK_MSG(CHAN_BUFFER, " *", "bob waves");
    LONGS_EQUAL(1, record_count_messages ());

    /* Action without text. */
    RECV(":bob!user_b@host_b PRIVMSG #test :\001ACTION\001");
    CHECK_MSG(CHAN_BUFFER, " *", "bob");

    /* Action from a nick not in channel. */
    RECV(":carol!user_c@host_c PRIVMSG #test :\001ACTION waves\001");
    CHECK_MSG(CHAN_BUFFER, " *", "carol waves");

    /* Action for channel ops (STATUSMSG). */
    RECV(":bob!user_b@host_b PRIVMSG @#test :\001ACTION waves\001");
    CHECK_MSG(CHAN_BUFFER, "--", "Action -> @#test: bob waves");

    /* Action in private: a private buffer is opened. */
    RECV(":bob!user_b@host_b PRIVMSG alice :\001ACTION waves\001");
    CHECK_MSG(PV_BUFFER, " *", "bob waves");
    ptr_channel = irc_channel_search (server, "bob");
    CHECK(ptr_channel);
    STRCMP_EQUAL("user_b@host_b", ptr_channel->topic);

    /* Second action in private: same buffer. */
    RECV(":bob!user_b@host_b PRIVMSG alice :\001ACTION smiles\001");
    CHECK_MSG(PV_BUFFER, " *", "bob smiles");
    POINTERS_EQUAL(ptr_channel, irc_channel_search (server, "bob"));
}

/*
 * Test functions:
 *   irc_ctcp_display_reply_from_nick
 */

TEST(IrcCtcpWithServer, DisplayReplyFromNick)
{
    struct timeval tv_now;
    char str_msg[256];

    RECV(":bob!user_b@host_b NOTICE alice :\001VERSION WeeChat 1.0\001");
    STRCMP_EQUAL("", *sent);
    CHECK_MSG(SRV_BUFFER, "--", "CTCP reply from bob: VERSION WeeChat 1.0");
    CHECK(record_search (SRV_BUFFER, "--",
                         "CTCP reply from bob: VERSION WeeChat 1.0",
                         "irc_notice,irc_ctcp,nick_bob,host_user_b@host_b,"
                         "log1"));

    /* Extra spaces before arguments are ignored. */
    RECV(":bob!user_b@host_b NOTICE alice :\001VERSION   WeeChat 1.0\001");
    CHECK_MSG(SRV_BUFFER, "--", "CTCP reply from bob: VERSION WeeChat 1.0");

    RECV(":bob!user_b@host_b NOTICE alice :\001VERSION\001");
    CHECK_MSG(SRV_BUFFER, "--", "CTCP reply from bob: VERSION");
    CHECK(record_search (SRV_BUFFER, "--", "CTCP reply from bob: VERSION",
                         "irc_notice,irc_ctcp,nick_bob,host_user_b@host_b,"
                         "log1"));

    /* Text after the end of CTCP is ignored. */
    RECV(":bob!user_b@host_b NOTICE alice :\001VERSION 1.0\001 extra");
    CHECK_MSG(SRV_BUFFER, "--", "CTCP reply from bob: VERSION 1.0");
    LONGS_EQUAL(1, record_count_messages ());

    /* Missing "\001" at the end. */
    RECV(":bob!user_b@host_b NOTICE alice :\001VERSION WeeChat 1.0");
    CHECK_MSG(SRV_BUFFER, "--", "CTCP reply from bob: VERSION WeeChat 1.0");

    /* Reply to CTCP PING with a single argument: displayed as-is. */
    RECV(":bob!user_b@host_b NOTICE alice :\001PING 1234\001");
    CHECK_MSG(SRV_BUFFER, "--", "CTCP reply from bob: PING 1234");

    /* Reply to CTCP PING: the lag is displayed. */
    gettimeofday (&tv_now, NULL);
    snprintf (str_msg, sizeof (str_msg),
              ":bob!user_b@host_b NOTICE alice :\001PING %lld %lld\001",
              (long long)tv_now.tv_sec - 2, (long long)tv_now.tv_usec);
    RECV(str_msg);
    STRCMP_EQUAL("", *sent);
    CHECK(record_search_msg_regex (
              SRV_BUFFER, "^CTCP reply from bob: PING 2\\.[0-9]{3}s$"));
}

/*
 * Test functions:
 *   irc_ctcp_recv_dcc
 */

TEST(IrcCtcpWithServer, RecvDcc)
{
    /* DCC SEND (active). */
    RECV(":bob!user_b@host_b PRIVMSG alice :"
         "\001DCC SEND file.txt 3232235777 5000 1234\001");
    STRCMP_EQUAL("xfer_add type_string=file_recv_active filename=file.txt "
                 "remote_address=3232235777 size=1234 token=(null) "
                 "remote_nick=bob port=5000",
                 *xfer);

    /* DCC SEND (passive), with token and extra spaces. */
    RECV(":bob!user_b@host_b PRIVMSG alice :"
         "\001DCC SEND  file.txt  3232235777  0  1234  42\001");
    STRCMP_EQUAL("xfer_add type_string=file_recv_passive filename=file.txt "
                 "remote_address=3232235777 size=1234 token=42 "
                 "remote_nick=bob port=0",
                 *xfer);

    /* DCC SEND with file name between double quotes. */
    RECV(":bob!user_b@host_b PRIVMSG alice :"
         "\001DCC SEND \"my file.txt\" 3232235777 5000 1234\001");
    STRCMP_EQUAL("xfer_add type_string=file_recv_active "
                 "filename=my file.txt "
                 "remote_address=3232235777 size=1234 token=(null) "
                 "remote_nick=bob port=5000",
                 *xfer);

    /* DCC RESUME. */
    RECV(":bob!user_b@host_b PRIVMSG alice :"
         "\001DCC RESUME \"my file.txt\" 5000 100\001");
    STRCMP_EQUAL("xfer_accept_resume type_string=file_recv_active "
                 "filename=my file.txt start_resume=100 token=(null) "
                 "port=5000",
                 *xfer);
    RECV(":bob!user_b@host_b PRIVMSG alice :"
         "\001DCC RESUME file.txt 0 100 42\001");
    STRCMP_EQUAL("xfer_accept_resume type_string=file_recv_active "
                 "filename=file.txt start_resume=100 token=42 port=0",
                 *xfer);

    /* DCC ACCEPT. */
    RECV(":bob!user_b@host_b PRIVMSG alice :"
         "\001DCC ACCEPT \"my file.txt\" 5000 100\001");
    STRCMP_EQUAL("xfer_start_resume type_string=file_recv_active "
                 "filename=my file.txt start_resume=100 token=(null) "
                 "port=5000",
                 *xfer);
    RECV(":bob!user_b@host_b PRIVMSG alice :"
         "\001DCC ACCEPT file.txt 0 100 42\001");
    STRCMP_EQUAL("xfer_start_resume type_string=file_recv_active "
                 "filename=file.txt start_resume=100 token=42 port=0",
                 *xfer);

    /* DCC CHAT. */
    RECV(":bob!user_b@host_b PRIVMSG alice :"
         "\001DCC CHAT chat 3232235777 5000\001");
    STRCMP_EQUAL("xfer_add type_string=chat_recv "
                 "remote_address=3232235777 remote_nick=bob port=5000",
                 *xfer);
    RECV(":bob!user_b@host_b PRIVMSG alice :"
         "\001DCC CHAT xyz 3232235777 5000\001");
    STRCMP_EQUAL("", *xfer);
    CHECK_MSG(SRV_BUFFER, "=!=",
              "irc: unknown DCC CHAT type received from bob: \"xyz\"");

    /* No arguments or unknown DCC type: ignored. */
    RECV(":bob!user_b@host_b PRIVMSG alice :\001DCC\001");
    STRCMP_EQUAL("", *xfer);
    LONGS_EQUAL(0, record_count_messages ());
    RECV(":bob!user_b@host_b PRIVMSG alice :\001DCC XYZ a b c\001");
    STRCMP_EQUAL("", *xfer);
    LONGS_EQUAL(0, record_count_messages ());
}

/*
 * Test functions:
 *   irc_ctcp_recv_dcc (invalid DCC)
 */

TEST(IrcCtcpWithServer, RecvDccInvalid)
{
    const char *messages[] = {
        "\001DCC SEND file.txt\001",
        "\001DCC SEND \"file.txt 1 2 3\001",
        "\001DCC SEND \"file.txt\"\001",
        "\001DCC SEND file.txt 3232235777\001",
        "\001DCC SEND file.txt 3232235777 5000\001",
        "\001DCC RESUME file.txt\001",
        "\001DCC RESUME \"file.txt\001",
        "\001DCC RESUME \"file.txt\"\001",
        "\001DCC RESUME file.txt 5000\001",
        "\001DCC ACCEPT file.txt\001",
        "\001DCC ACCEPT \"file.txt\001",
        "\001DCC ACCEPT \"file.txt\"\001",
        "\001DCC ACCEPT file.txt 5000\001",
        "\001DCC CHAT chat\001",
        "\001DCC CHAT chat 3232235777\001",
        NULL,
    };
    char str_msg[256];
    int i;

    for (i = 0; messages[i]; i++)
    {
        snprintf (str_msg, sizeof (str_msg),
                  ":bob!user_b@host_b PRIVMSG alice :%s", messages[i]);
        RECV(str_msg);
        STRCMP_EQUAL("", *xfer);
        CHECK_MSG(SRV_BUFFER, "=!=", "irc: cannot parse \"privmsg\" command");
    }
}

/*
 * Test functions:
 *   irc_ctcp_send
 */

TEST(IrcCtcpWithServer, Send)
{
    string_dyn_copy (sent, NULL);
    irc_ctcp_send (server, "bob", "VERSION", NULL);
    STRCMP_EQUAL("PRIVMSG bob :\001VERSION\001\n", *sent);

    string_dyn_copy (sent, NULL);
    irc_ctcp_send (server, "#test", "PING", "1234 5678");
    STRCMP_EQUAL("PRIVMSG #test :\001PING 1234 5678\001\n", *sent);
}
