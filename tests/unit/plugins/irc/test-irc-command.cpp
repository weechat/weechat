/*
 * SPDX-FileCopyrightText: 2024-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC commands */

#include "CppUTest/TestHarness.h"

#include "tests.h"
#include "tests-record.h"

extern "C"
{
#include <stdio.h>
#include <string.h>
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/gui/gui-buffer.h"
#include "src/core/core-string.h"
#include "src/plugins/plugin.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-command.h"
#include "src/plugins/irc/irc-config.h"
#include "src/plugins/irc/irc-notify.h"
#include "src/plugins/irc/irc-server.h"
#include "src/plugins/logger/logger-config.h"

extern char **irc_command_mode_masks_convert_ranges (char **argv, int arg_start);
extern void irc_command_display_away (struct t_irc_server *server,
                                      const char *string1,
                                      const char *string2);
extern int irc_command_run_away (const void *pointer, void *data,
                                 struct t_gui_buffer *buffer,
                                 const char *command);
extern void irc_send_nick_server (struct t_irc_server *server,
                                  const char *nickname);
}

#define IRC_FAKE_SERVER "fake_cmd"
#define SRV_BUFFER "irc.server." IRC_FAKE_SERVER
#define CHAN_BUFFER "irc." IRC_FAKE_SERVER ".#test"

/* Run an IRC command in the channel buffer. */
#define CMD_CHAN(__command)                                             \
    run_command (CHAN_BUFFER, __command);

/* Run an IRC command in the server buffer. */
#define CMD_SRV(__command)                                              \
    run_command (SRV_BUFFER, __command);

/* Check messages sent to the server (separated by "\n"). */
#define CHECK_SENT(__messages)                                          \
    STRCMP_EQUAL(__messages, *sent);

/* Check an error displayed in the server buffer. */
#define CHECK_SRV_ERROR(__message)                                      \
    if (!record_search (SRV_BUFFER, "=!=", __message, NULL))            \
    {                                                                   \
        char **msg = string_dyn_alloc (256);                            \
        string_dyn_concat (msg, "Error not displayed: ", -1);           \
        string_dyn_concat (msg, __message, -1);                         \
        string_dyn_concat (msg, "\nAll messages displayed:\n", -1);     \
        record_dump (msg);                                              \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

TEST_GROUP(IrcCommand)
{
};

TEST_GROUP(IrcCommandWithServer)
{
    struct t_irc_server *server = NULL;
    struct t_irc_channel *channel = NULL;
    struct t_hook *hook_irc_out = NULL;
    char **sent = NULL;

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
        if ((*ptr_sent)[0])
            string_dyn_concat (ptr_sent, "\n", -1);
        string_dyn_concat (ptr_sent, (const char *)signal_data, -1);
        return WEECHAT_RC_OK;
    }

    void server_recv (const char *message)
    {
        char str_command[8192];

        snprintf (str_command, sizeof (str_command),
                  "/command -buffer " SRV_BUFFER " irc "
                  "/server fakerecv \"%s\"",
                  message);
        run_cmd_quiet (str_command);
    }

    /* Run a command in a buffer, record messages and messages sent. */
    void run_command (const char *buffer, const char *command)
    {
        char str_command[8192];

        string_dyn_copy (sent, NULL);
        snprintf (str_command, sizeof (str_command),
                  "/command -buffer %s irc %s", buffer, command);
        record_start ();
        run_cmd_quiet (str_command);
        record_stop ();
    }

    void setup ()
    {
        sent = string_dyn_alloc (256);
        config_file_option_set (logger_config_look_backlog, "0", 1);
        run_cmd_quiet ("/mute /server add " IRC_FAKE_SERVER " fake:127.0.0.1 "
                       "-nicks=alice -anti_flood=0");
        run_cmd_quiet ("/mute /connect " IRC_FAKE_SERVER);
        server = irc_server_search (IRC_FAKE_SERVER);
        CHECK(server);
        server_recv (":server 001 alice :Welcome");
        server_recv (":server 005 alice MODES=3 PREFIX=(ohv)@%+ "
                     "CHANMODES=beIq,k,l,imnpst CHANTYPES=# "
                     ":are supported");
        server_recv (":alice!user_a@host_a JOIN #test");
        server_recv (":server 353 alice = #test :@alice %helen +carol dave");
        server_recv (":server 366 alice #test :End of /NAMES list");
        server_recv (":bob!user_b@host_b JOIN #test");
        channel = irc_channel_search (server, "#test");
        CHECK(channel);
        hook_irc_out = hook_signal (NULL, IRC_FAKE_SERVER ",irc_out1_*",
                                    &irc_out_cb, sent, NULL);
    }

    void teardown ()
    {
        record_stop ();
        unhook (hook_irc_out);
        hook_irc_out = NULL;
        run_cmd_quiet ("/mute /disconnect " IRC_FAKE_SERVER);
        run_cmd_quiet ("/mute /server del " IRC_FAKE_SERVER);
        server = NULL;
        channel = NULL;
        string_dyn_free (sent, 1);
        sent = NULL;
        config_file_option_reset (logger_config_look_backlog, 1);
    }
};

/*
 * Test functions:
 *   irc_command_mode_nicks
 *   irc_command_op
 *   irc_command_deop
 *   irc_command_voice
 *   irc_command_devoice
 *   irc_command_halfop
 *   irc_command_dehalfop
 */

TEST(IrcCommandWithServer, ModeNicks)
{
    /* "*" requires "-yes". */
    CMD_CHAN("/op *");
    CHECK_SENT("");
    CHECK(record_search ("core.weechat", "=!=",
                         "irc: \"-yes\" argument is required for nick \"*\" "
                         "(security reason), see /help op",
                         NULL));

    /* Nicks already op are ignored, nicks are sent once. */
    CMD_CHAN("/op alice bob bob b*");
    CHECK_SENT("MODE #test +o bob");

    /* Max 3 modes per message (option MODES of server). */
    CMD_CHAN("/op * -yes");
    CHECK_SENT("MODE #test +ooo helen carol dave\n"
               "MODE #test +o bob");

    /* Own nick is not deop with a wildcard. */
    CMD_CHAN("/deop * -yes");
    CHECK_SENT("");
    CMD_CHAN("/deop alice");
    CHECK_SENT("MODE #test -o alice");

    /* Own nick. */
    CMD_CHAN("/op");
    CHECK_SENT("MODE #test +o alice");

    CMD_CHAN("/op bob carol");
    CHECK_SENT("MODE #test +oo carol bob");

    CMD_SRV("/op bob");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"op\" command can only be executed in a channel "
                    "buffer");

    CMD_CHAN("/deop");
    CHECK_SENT("MODE #test -o alice");

    /* Nick without op is ignored. */
    CMD_CHAN("/deop alice bob");
    CHECK_SENT("MODE #test -o alice");

    CMD_SRV("/deop alice");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"deop\" command can only be executed in a channel "
                    "buffer");

    CMD_CHAN("/voice");
    CHECK_SENT("MODE #test +v alice");

    /* Nick already voiced is ignored. */
    CMD_CHAN("/voice bob carol dave");
    CHECK_SENT("MODE #test +vv dave bob");

    CMD_SRV("/voice bob");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"voice\" command can only be executed in a "
                    "channel buffer");

    CMD_CHAN("/devoice");
    CHECK_SENT("MODE #test -v alice");

    CMD_CHAN("/devoice bob carol");
    CHECK_SENT("MODE #test -v carol");

    CMD_SRV("/devoice carol");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"devoice\" command can only be executed in a "
                    "channel buffer");

    CMD_CHAN("/halfop");
    CHECK_SENT("MODE #test +h alice");

    CMD_CHAN("/halfop bob helen");
    CHECK_SENT("MODE #test +h bob");

    CMD_SRV("/halfop bob");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"halfop\" command can only be executed in a "
                    "channel buffer");

    CMD_CHAN("/dehalfop");
    CHECK_SENT("MODE #test -h alice");

    CMD_CHAN("/dehalfop bob helen");
    CHECK_SENT("MODE #test -h helen");

    CMD_SRV("/dehalfop helen");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"dehalfop\" command can only be executed in a "
                    "channel buffer");
}

/*
 * Test functions:
 *   irc_command_mode_masks
 *   irc_command_ban
 *   irc_command_unban
 *   irc_command_quiet
 *   irc_command_unquiet
 */

TEST(IrcCommandWithServer, ModeMasks)
{
    /* Nick is replaced by the default ban mask (if host is known). */
    CMD_CHAN("/ban bob carol x!y@z");
    CHECK_SENT("MODE #test +bbb *!user_b@host_b carol x!y@z");

    /* Max 3 modes per message (option MODES of server). */
    CMD_CHAN("/ban a b c d e");
    CHECK_SENT("MODE #test +bbb a b c\n"
               "MODE #test +bb d e");

    /* List of bans. */
    CMD_CHAN("/ban");
    CHECK_SENT("MODE #test +b");
    CMD_SRV("/ban #test");
    CHECK_SENT("MODE #test +b");

    CMD_CHAN("/ban bob");
    CHECK_SENT("MODE #test +b *!user_b@host_b");
    CMD_SRV("/ban #test x!y@z");
    CHECK_SENT("MODE #test +b x!y@z");

    /* Server buffer without channel. */
    CMD_SRV("/ban");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"ban\" command can only be executed in a channel "
                    "buffer");
    CMD_SRV("/ban bob");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"ban\" command can only be executed in a channel "
                    "buffer");

    /* Bans received from server (numbers are used in /unban). */
    server_recv (":server 367 alice #test ban1!*@* alice 1700000000");
    server_recv (":server 367 alice #test ban2!*@* alice 1700000000");
    server_recv (":server 367 alice #test ban3!*@* alice 1700000000");
    server_recv (":server 368 alice #test :End of channel ban list");

    CMD_CHAN("/unban bob x!y@z");
    CHECK_SENT("MODE #test -bb *!user_b@host_b x!y@z");

    CMD_CHAN("/unban 1 3");
    CHECK_SENT("MODE #test -bb ban1!*@* ban3!*@*");
    CMD_CHAN("/unban 1-3");
    CHECK_SENT("MODE #test -bbb ban1!*@* ban2!*@* ban3!*@*");

    /* Unknown number: sent as-is. */
    CMD_CHAN("/unban 9");
    CHECK_SENT("MODE #test -b 9");

    CMD_SRV("/unban #test x!y@z");
    CHECK_SENT("MODE #test -b x!y@z");

    CMD_SRV("/unban x!y@z");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"unban\" command can only be executed in a "
                    "channel buffer");

    CMD_CHAN("/quiet");
    CHECK_SENT("MODE #test +q");

    CMD_CHAN("/quiet bob x!y@z");
    CHECK_SENT("MODE #test +qq *!user_b@host_b x!y@z");
    CMD_SRV("/quiet #test x!y@z");
    CHECK_SENT("MODE #test +q x!y@z");

    CMD_SRV("/quiet x!y@z");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"quiet\" command can only be executed in a "
                    "channel buffer");

    CMD_CHAN("/unquiet bob x!y@z");
    CHECK_SENT("MODE #test -qq *!user_b@host_b x!y@z");
    CMD_SRV("/unquiet #test x!y@z");
    CHECK_SENT("MODE #test -q x!y@z");

    CMD_SRV("/unquiet x!y@z");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"unquiet\" command can only be executed in a "
                    "channel buffer");

    /* Mode not supported by server. */
    server_recv (":server 005 alice CHANMODES=beI,k,l,imnpst "
                 ":are supported");
    CMD_CHAN("/quiet x");
    CHECK_SENT("");
    CHECK(record_search ("core.weechat", "=!=",
                         "irc: cannot execute command /quiet, channel mode "
                         "\"q\" is not supported by server",
                         NULL));
}

/*
 * Test functions:
 *   irc_command_kick_channel
 *   irc_command_kick
 *   irc_command_kickban
 *   irc_command_send_ban
 */

TEST(IrcCommandWithServer, Kick)
{
    /* Default kick message (option msg_kick). */
    config_file_option_set (server->options[IRC_SERVER_OPTION_MSG_KICK],
                            "${nick} kicks ${target} from ${channel}", 1);
    CMD_CHAN("/kick bob");
    CHECK_SENT("KICK #test bob :alice kicks bob from #test");
    CMD_CHAN("/kick bob you are out");
    CHECK_SENT("KICK #test bob :you are out");
    config_file_option_reset (server->options[IRC_SERVER_OPTION_MSG_KICK], 1);

    CMD_CHAN("/kick bob");
    CHECK_SENT("KICK #test bob");
    CMD_CHAN("/kick bob reason");
    CHECK_SENT("KICK #test bob :reason");
    CMD_SRV("/kick #test bob reason");
    CHECK_SENT("KICK #test bob :reason");

    CMD_SRV("/kick bob");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"kick\" command can only be executed in a channel "
                    "buffer");

    CMD_CHAN("/kickban bob reason");
    CHECK_SENT("MODE #test +b *!user_b@host_b\n"
               "KICK #test bob :reason");

    /* Nick with host: host is banned. */
    CMD_CHAN("/kickban bob!user@host");
    CHECK_SENT("MODE #test +b *!user@host\n"
               "KICK #test bob");
    CMD_SRV("/kickban #test bob@host");
    CHECK_SENT("MODE #test +b *!bob@host\n"
               "KICK #test bob");

    /* Mask must begin with a nick. */
    CMD_CHAN("/kickban *!*@host");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: mask must begin with nick");

    CMD_SRV("/kickban bob");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"kickban\" command can only be executed in a "
                    "channel buffer");

    CMD_CHAN("/kickban bob");
    CHECK_SENT("MODE #test +b *!user_b@host_b\n"
               "KICK #test bob");

    /* Nick without host: nick is used. */
    CMD_CHAN("/kickban carol");
    CHECK_SENT("MODE #test +b carol\n"
               "KICK #test carol");
}

/*
 * Test functions:
 *   irc_command_invite
 *   irc_command_topic
 *   irc_command_knock
 */

TEST(IrcCommandWithServer, InviteTopicKnock)
{
    CMD_CHAN("/invite bob");
    CHECK_SENT("INVITE bob #test");
    CMD_CHAN("/invite bob carol");
    CHECK_SENT("INVITE bob #test\n"
               "INVITE carol #test");
    CMD_SRV("/invite bob carol #other");
    CHECK_SENT("INVITE bob #other\n"
               "INVITE carol #other");

    CMD_SRV("/invite bob");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"invite\" command can only be executed in a "
                    "channel buffer");
    CMD_SRV("/invite bob carol");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"invite\" command can only be executed in a "
                    "channel buffer");

    CMD_CHAN("/topic");
    CHECK_SENT("TOPIC #test");
    CMD_CHAN("/topic new topic");
    CHECK_SENT("TOPIC #test :new topic");
    CMD_CHAN("/topic -delete");
    CHECK_SENT("TOPIC #test :");
    CMD_SRV("/topic #test");
    CHECK_SENT("TOPIC #test");
    CMD_SRV("/topic #test other topic");
    CHECK_SENT("TOPIC #test :other topic");

    CMD_SRV("/topic new topic");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"topic\" command can only be executed in a "
                    "channel buffer");

    CMD_SRV("/knock #secret");
    CHECK_SENT("KNOCK #secret");
    CMD_SRV("/knock #secret please let me in");
    CHECK_SENT("KNOCK #secret :please let me in");
}

/*
 * Test functions:
 *   irc_command_cycle
 *   irc_command_names
 *   irc_command_remove
 */

TEST(IrcCommandWithServer, CycleNamesRemove)
{
    /* Default part message (option msg_part). */
    config_file_option_set (server->options[IRC_SERVER_OPTION_MSG_PART],
                            "bye from ${channel}", 1);
    CMD_CHAN("/cycle");
    CHECK_SENT("PART #test :bye from #test");
    LONGS_EQUAL(1, channel->cycle);
    channel->cycle = 0;

    config_file_option_set (server->options[IRC_SERVER_OPTION_MSG_PART],
                            "", 1);
    CMD_CHAN("/cycle");
    CHECK_SENT("PART #test");
    LONGS_EQUAL(1, channel->cycle);
    channel->cycle = 0;

    CMD_CHAN("/cycle see you");
    CHECK_SENT("PART #test :see you");
    channel->cycle = 0;

    CMD_SRV("/cycle #test,#other bye");
    CHECK_SENT("PART #test,#other :bye");
    LONGS_EQUAL(1, channel->cycle);
    channel->cycle = 0;

    CMD_SRV("/cycle");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"part\" command cannot be executed on a server "
                    "buffer");
    CMD_SRV("/cycle bye");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"cycle\" command cannot be executed on a server "
                    "buffer");

    config_file_option_reset (server->options[IRC_SERVER_OPTION_MSG_PART], 1);

    CMD_CHAN("/names");
    CHECK_SENT("NAMES #test");
    CMD_SRV("/names #test,#other");
    CHECK_SENT("NAMES #test,#other");

    /* Filter on nick prefix. */
    CMD_CHAN("/names -o");
    CHECK_SENT("NAMES #test");
    STRCMP_EQUAL("o", (const char *)hashtable_get (
                     server->names_channel_filter, "#test"));
    CMD_SRV("/names -count #test");
    CHECK_SENT("NAMES #test");
    STRCMP_EQUAL("#", (const char *)hashtable_get (
                     server->names_channel_filter, "#test"));

    CMD_SRV("/names");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"names\" command can only be executed in a "
                    "channel buffer");

    CMD_CHAN("/remove bob");
    CHECK_SENT("REMOVE #test bob");
    CMD_CHAN("/remove bob go away");
    CHECK_SENT("REMOVE #test bob :go away");
    CMD_SRV("/remove #test bob");
    CHECK_SENT("REMOVE #test bob");

    CMD_SRV("/remove bob");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"remove\" command can only be executed in a "
                    "channel buffer");
}

/*
 * Test functions:
 *   irc_command_display_away
 *   irc_command_away_server
 *   irc_command_away
 *   irc_command_run_away
 */

TEST(IrcCommandWithServer, Away)
{
    struct t_gui_buffer *ptr_buffer;

    /* Away with message displayed locally (default). */
    CMD_CHAN("/away gone fishing");
    CHECK_SENT("AWAY :gone fishing");
    LONGS_EQUAL(1, server->is_away);
    STRCMP_EQUAL("gone fishing", server->away_message);
    CHECK(server->away_time > 0);
    CHECK(record_search (CHAN_BUFFER, "", "[alice away: gone fishing]",
                         NULL));

    /* Back. */
    CMD_CHAN("/away");
    CHECK_SENT("AWAY");
    LONGS_EQUAL(0, server->is_away);
    POINTERS_EQUAL(NULL, server->away_message);
    LONGS_EQUAL(0, server->away_time);
    CHECK(record_search_msg_regex (CHAN_BUFFER,
                                   "^\\[alice back: gone 00:00:0[0-9]\\]$"));

    /* Away message sent as action to channels. */
    config_file_option_set (irc_config_look_display_away, "channel", 1);
    CMD_SRV("/away busy");
    CHECK_SENT("AWAY :busy\n"
               "PRIVMSG #test :\001ACTION is away: busy\001");
    CMD_SRV("/away");
    CHECK(strncmp (*sent,
                   "AWAY\nPRIVMSG #test :\001ACTION is back (gone 00:00:0",
                   48) == 0);

    /* Nothing displayed or sent to channels. */
    config_file_option_set (irc_config_look_display_away, "off", 1);
    CMD_SRV("/away busy");
    CHECK_SENT("AWAY :busy");
    CMD_SRV("/away");
    CHECK_SENT("AWAY");
    config_file_option_reset (irc_config_look_display_away, 1);

    /* All servers. */
    CMD_CHAN("/away -all brb");
    CHECK_SENT("AWAY :brb");
    LONGS_EQUAL(1, server->is_away);
    CMD_CHAN("/away -all");
    CHECK_SENT("AWAY");
    LONGS_EQUAL(0, server->is_away);

    /* Callback of hook "command_run" on /away. */
    ptr_buffer = gui_buffer_search ("irc", "server." IRC_FAKE_SERVER);
    CHECK(ptr_buffer);
    string_dyn_copy (sent, NULL);
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_command_run_away (NULL, NULL, ptr_buffer, "/away later"));
    CHECK_SENT("AWAY :later");
    string_dyn_copy (sent, NULL);
    irc_command_away_server (server, NULL, 0);
    CHECK_SENT("AWAY");

    /* Away on server not connected: message stored for next connection. */
    run_cmd_quiet ("/mute /disconnect " IRC_FAKE_SERVER);
    string_dyn_copy (sent, NULL);
    record_start ();
    irc_command_away_server (server, "later", 0);
    record_stop ();
    CHECK_SENT("");
    STRCMP_EQUAL("later", server->away_message);
    CHECK(record_search (SRV_BUFFER, "", "irc: future away: later", NULL));
    record_start ();
    irc_command_away_server (server, NULL, 0);
    record_stop ();
    POINTERS_EQUAL(NULL, server->away_message);
    CHECK(record_search (SRV_BUFFER, "", "irc: future away removed", NULL));

    /* Invalid server. */
    irc_command_away_server (NULL, "test", 0);
}

/*
 * Test functions:
 *   irc_send_nick_server
 *   irc_command_nick
 */

TEST(IrcCommandWithServer, Nick)
{
    CMD_SRV("/nick alice2");
    CHECK_SENT("NICK alice2");
    CMD_CHAN("/nick alice:2");
    CHECK_SENT("NICK :alice:2");
    CMD_CHAN("/nick -all alice3");
    CHECK_SENT("NICK alice3");

    /* Invalid arguments. */
    CMD_CHAN("/nick alice2 alice3");
    CHECK_SENT("");

    irc_send_nick_server (NULL, "test");

    /* Server not connected: nick is changed locally. */
    run_cmd_quiet ("/mute /disconnect " IRC_FAKE_SERVER);
    string_dyn_copy (sent, NULL);
    irc_send_nick_server (server, "alice4");
    CHECK_SENT("");
    STRCMP_EQUAL("alice4", server->nick);
}

/*
 * Test functions:
 *   irc_command_whois
 *   irc_command_whowas
 *   irc_command_who
 *   irc_command_userhost
 *   irc_command_ison
 */

TEST(IrcCommandWithServer, UserInfo)
{
    /* /whois: own nick by default, nick of private buffer. */
    CMD_SRV("/whois");
    CHECK_SENT("WHOIS alice");
    CMD_SRV("/whois bob");
    CHECK_SENT("WHOIS bob");
    CMD_SRV("/whois bob carol");
    CHECK_SENT("WHOIS bob carol");
    server_recv (":bob!user_b@host_b PRIVMSG alice :hi");
    run_command ("irc." IRC_FAKE_SERVER ".bob", "/whois");
    CHECK_SENT("WHOIS bob");

    /* Option irc.network.whois_double_nick. */
    config_file_option_set (irc_config_network_whois_double_nick, "on", 1);
    CMD_SRV("/whois bob");
    CHECK_SENT("WHOIS bob bob");
    CMD_SRV("/whois bob,carol");
    CHECK_SENT("WHOIS bob,carol");
    config_file_option_reset (irc_config_network_whois_double_nick, 1);

    CMD_SRV("/whowas bob");
    CHECK_SENT("WHOWAS bob");
    CMD_SRV("/whowas bob 5");
    CHECK_SENT("WHOWAS bob 5");

    CMD_SRV("/who");
    CHECK_SENT("WHO");
    CMD_SRV("/who #test o");
    CHECK_SENT("WHO #test o");

    CMD_SRV("/userhost bob carol");
    CHECK_SENT("USERHOST bob carol");

    CMD_SRV("/ison bob carol");
    CHECK_SENT("ISON :bob carol");

    /* Missing arguments. */
    CMD_SRV("/whowas");
    CMD_SRV("/userhost");
    CMD_SRV("/ison");
    CHECK_SENT("");
}

/*
 * Test functions:
 *   irc_command_ping
 *   irc_command_pong
 *   irc_command_kill
 *   irc_command_squery
 *   irc_command_setname
 *   irc_command_wallops
 */

TEST(IrcCommandWithServer, MessagesToServer)
{
    CMD_SRV("/ping irc.example.com");
    CHECK_SENT("PING irc.example.com");
    CMD_SRV("/pong irc.example.com");
    CHECK_SENT("PONG irc.example.com");

    CMD_SRV("/kill bob");
    CHECK_SENT("KILL bob");
    CMD_SRV("/kill bob bad behavior");
    CHECK_SENT("KILL bob :bad behavior");

    CMD_SRV("/squery service");
    CHECK_SENT("SQUERY service");
    CMD_SRV("/squery service help me");
    CHECK_SENT("SQUERY service :help me");

    CMD_SRV("/setname Alice Liddell");
    CHECK_SENT("SETNAME :Alice Liddell");

    CMD_SRV("/wallops hello opers");
    CHECK_SENT("WALLOPS :hello opers");

    /* Missing arguments. */
    CMD_SRV("/ping");
    CMD_SRV("/pong");
    CMD_SRV("/kill");
    CMD_SRV("/squery");
    CMD_SRV("/setname");
    CMD_SRV("/wallops");
    CHECK_SENT("");
}

/*
 * Test functions:
 *   irc_command_wallchops
 */

TEST(IrcCommandWithServer, Wallchops)
{
    /* No WALLCHOPS/STATUSMSG: notice sent to each op (except self). */
    server_recv (":server MODE #test +o bob");
    CMD_CHAN("/wallchops hello ops");
    CHECK_SENT("NOTICE bob :hello ops");
    CHECK(record_search_msg_regex (CHAN_BUFFER, "hello ops"));

    /* STATUSMSG with "@": notice sent to @#channel. */
    server_recv (":server 005 alice STATUSMSG=@+ :are supported");
    CMD_SRV("/wallchops #test hello ops");
    CHECK_SENT("NOTICE @#test :hello ops");

    /* Channel not joined. */
    CMD_SRV("/wallchops #other hello");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: you are not on channel \"#other\"");

    CMD_SRV("/wallchops hello");
    CHECK_SENT("");
    CHECK_SRV_ERROR("irc: \"wallchops\" command can only be executed in a "
                    "channel buffer");
}

/*
 * Test functions:
 *   irc_command_mode_masks_convert_ranges
 */

TEST(IrcCommand, ModeMasksConvertRanges)
{
    char **args, **masks;

    POINTERS_EQUAL(NULL, irc_command_mode_masks_convert_ranges (NULL, 0));

    args = string_split ("test", " ", NULL, 0, 0, NULL);
    POINTERS_EQUAL(NULL, irc_command_mode_masks_convert_ranges (args, -1));
    masks = irc_command_mode_masks_convert_ranges (args, 0);
    CHECK(masks);
    STRCMP_EQUAL("test", masks[0]);
    POINTERS_EQUAL(NULL, masks[1]);
    string_free_split (masks);
    string_free_split (args);

    args = string_split ("3 test 6-9 5-1 -2 64-", " ", NULL, 0, 0, NULL);
    masks = irc_command_mode_masks_convert_ranges (args, 0);
    CHECK(masks);
    STRCMP_EQUAL("3", masks[0]);
    STRCMP_EQUAL("test", masks[1]);
    STRCMP_EQUAL("6", masks[2]);
    STRCMP_EQUAL("7", masks[3]);
    STRCMP_EQUAL("8", masks[4]);
    STRCMP_EQUAL("9", masks[5]);
    STRCMP_EQUAL("5-1", masks[6]);
    STRCMP_EQUAL("-2", masks[7]);
    STRCMP_EQUAL("64-", masks[8]);
    POINTERS_EQUAL(NULL, masks[9]);
    string_free_split (masks);
    string_free_split (args);

    args = string_split ("4-10", " ", NULL, 0, 0, NULL);
    masks = irc_command_mode_masks_convert_ranges (args, 0);
    CHECK(masks);
    STRCMP_EQUAL("4", masks[0]);
    STRCMP_EQUAL("5", masks[1]);
    STRCMP_EQUAL("6", masks[2]);
    STRCMP_EQUAL("7", masks[3]);
    STRCMP_EQUAL("8", masks[4]);
    STRCMP_EQUAL("9", masks[5]);
    STRCMP_EQUAL("10", masks[6]);
    POINTERS_EQUAL(NULL, masks[7]);
    string_free_split (masks);
    string_free_split (args);
}

/*
 * Test functions:
 *   irc_command_me_channel_message
 */

TEST(IrcCommand, MeChannelMessage)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_me_channel
 */

TEST(IrcCommand, MeChannel)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_me_all_channels
 */

TEST(IrcCommand, MeAllChannels)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_action
 */

TEST(IrcCommand, Action)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_admin
 */

TEST(IrcCommand, Admin)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_exec_buffers
 */

TEST(IrcCommand, ExecBuffers)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_exec_all_channels
 */

TEST(IrcCommand, ExecAllChannels)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_allchan
 */

TEST(IrcCommand, Allchan)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_allpv
 */

TEST(IrcCommand, Allpv)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_exec_all_servers
 */

TEST(IrcCommand, ExecAllServers)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_allserv
 */

TEST(IrcCommand, Allserv)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_auth
 */

TEST(IrcCommand, Auth)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_autojoin
 */

TEST(IrcCommand, Autojoin)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_cap
 */

TEST(IrcCommand, Cap)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_connect_one_server
 */

TEST(IrcCommand, ConnectOneServer)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_connect
 */

TEST(IrcCommand, Connect)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_ctcp
 */

TEST(IrcCommand, Ctcp)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_dcc
 */

TEST(IrcCommand, Dcc)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_die
 */

TEST(IrcCommand, Die)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_quit_server
 */

TEST(IrcCommand, QuitServer)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_disconnect_one_server
 */

TEST(IrcCommand, DisconnectOneServer)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_disconnect
 */

TEST(IrcCommand, Disconnect)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_ignore_display
 */

TEST(IrcCommand, IgnoreDisplay)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_ignore
 */

TEST(IrcCommand, Ignore)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_info
 */

TEST(IrcCommand, Info)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_join_server
 */

TEST(IrcCommand, JoinServer)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_join
 */

TEST(IrcCommand, Join)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_links
 */

TEST(IrcCommand, Links)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_list_get_int_arg
 */

TEST(IrcCommand, ListGetIntArg)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_list
 */

TEST(IrcCommand, List)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_lusers
 */

TEST(IrcCommand, Lusers)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_map
 */

TEST(IrcCommand, Map)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_me
 */

TEST(IrcCommand, Me)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_mode_server
 */

TEST(IrcCommand, ModeServer)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_mode
 */

TEST(IrcCommand, Mode)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_motd
 */

TEST(IrcCommand, Motd)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_msg
 */

TEST(IrcCommand, Msg)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_notice
 */

TEST(IrcCommand, Notice)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_notify
 */

TEST(IrcCommand, Notify)
{
    struct t_irc_server *server;

    run_cmd_quiet ("/mute /server add test_notify 127.0.0.1");
    server = irc_server_search ("test_notify");
    CHECK(server);

    /* Add a single nick. */
    run_cmd_quiet ("/mute /notify add nick1 test_notify");
    LONGS_EQUAL(1, server->notify_count);
    CHECK(irc_notify_search (server, "nick1"));
    LONGS_EQUAL(0, irc_notify_search (server, "nick1")->check_away);

    /* Add multiple nicks, including one already existing. */
    run_cmd_quiet ("/mute /notify add nick2,nick1,nick3 test_notify");
    LONGS_EQUAL(3, server->notify_count);
    CHECK(irc_notify_search (server, "nick2"));
    CHECK(irc_notify_search (server, "nick3"));
    STRCMP_EQUAL("nick1,nick2,nick3",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_NOTIFY]));

    /* Add multiple nicks with empty items. */
    run_cmd_quiet ("/mute /notify add ,nick4,, test_notify");
    LONGS_EQUAL(4, server->notify_count);
    CHECK(irc_notify_search (server, "nick4"));

    /* Add or replace multiple nicks, including one not existing. */
    run_cmd_quiet ("/mute /notify addreplace nick1,nick5 test_notify -away");
    LONGS_EQUAL(5, server->notify_count);
    LONGS_EQUAL(1, irc_notify_search (server, "nick1")->check_away);
    LONGS_EQUAL(0, irc_notify_search (server, "nick2")->check_away);
    LONGS_EQUAL(0, irc_notify_search (server, "nick3")->check_away);
    LONGS_EQUAL(0, irc_notify_search (server, "nick4")->check_away);
    LONGS_EQUAL(1, irc_notify_search (server, "nick5")->check_away);
    STRCMP_EQUAL("nick2,nick3,nick4,nick1 away,nick5 away",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_NOTIFY]));
    run_cmd_quiet ("/mute /notify del nick5 test_notify");
    LONGS_EQUAL(4, server->notify_count);

    /* Delete a single nick. */
    run_cmd_quiet ("/mute /notify del nick2 test_notify");
    LONGS_EQUAL(3, server->notify_count);
    POINTERS_EQUAL(NULL, irc_notify_search (server, "nick2"));

    /* Delete multiple nicks, including one not found. */
    run_cmd_quiet ("/mute /notify del nick1,unknown,nick4 test_notify");
    LONGS_EQUAL(1, server->notify_count);
    POINTERS_EQUAL(NULL, irc_notify_search (server, "nick1"));
    POINTERS_EQUAL(NULL, irc_notify_search (server, "nick4"));
    CHECK(irc_notify_search (server, "nick3"));
    STRCMP_EQUAL("nick3",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_NOTIFY]));

    run_cmd_quiet ("/mute /notify add nick5 test_notify");
    run_cmd_quiet ("/mute /notify add nick6 test_notify");
    LONGS_EQUAL(3, server->notify_count);

    /* Delete multiple nicks with empty items. */
    run_cmd_quiet ("/mute /notify del ,nick5,,nick6, test_notify");
    LONGS_EQUAL(1, server->notify_count);

    /* Delete all nicks. */
    run_cmd_quiet ("/mute /notify del -all test_notify");
    LONGS_EQUAL(0, server->notify_count);

    run_cmd_quiet ("/mute /server del test_notify");
}

/*
 * Test functions:
 *   irc_command_oper
 */

TEST(IrcCommand, Oper)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_part_channel
 */

TEST(IrcCommand, PartChannel)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_part
 */

TEST(IrcCommand, Part)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_query
 */

TEST(IrcCommand, Query)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_quote
 */

TEST(IrcCommand, Quote)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_reconnect_one_server
 */

TEST(IrcCommand, ReconnectOneServer)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_reconnect
 */

TEST(IrcCommand, Reconnect)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_rehash
 */

TEST(IrcCommand, Rehash)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_restart
 */

TEST(IrcCommand, Restart)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_rules
 */

TEST(IrcCommand, Rules)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_sajoin
 */

TEST(IrcCommand, Sajoin)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_samode
 */

TEST(IrcCommand, Samode)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_sanick
 */

TEST(IrcCommand, Sanick)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_sapart
 */

TEST(IrcCommand, Sapart)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_saquit
 */

TEST(IrcCommand, Saquit)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_display_server
 */

TEST(IrcCommand, DisplayServer)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_server
 */

TEST(IrcCommand, Server)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_service
 */

TEST(IrcCommand, Service)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_servlist
 */

TEST(IrcCommand, Servlist)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_squit
 */

TEST(IrcCommand, Squit)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_stats
 */

TEST(IrcCommand, Stats)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_summon
 */

TEST(IrcCommand, Summon)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_time
 */

TEST(IrcCommand, Time)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_trace
 */

TEST(IrcCommand, Trace)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_users
 */

TEST(IrcCommand, Users)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_version
 */

TEST(IrcCommand, Version)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_command_init
 */

TEST(IrcCommand, Init)
{
    /* TODO: write tests */
}
