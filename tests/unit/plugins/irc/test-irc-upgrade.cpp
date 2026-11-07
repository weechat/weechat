/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC upgrade functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"
#include "tests-record.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-infolist.h"
#include "src/core/core-list.h"
#include "src/core/core-string.h"
#include "src/gui/gui-buffer.h"
#include "src/plugins/plugin.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-modelist.h"
#include "src/plugins/irc/irc-nick.h"
#include "src/plugins/irc/irc-notify.h"
#include "src/plugins/irc/irc-raw.h"
#include "src/plugins/irc/irc-redirect.h"
#include "src/plugins/irc/irc-server.h"
#include "src/plugins/irc/irc-upgrade.h"
#include "src/plugins/logger/logger-config.h"

extern struct t_irc_server *irc_upgrade_current_server;
extern struct t_irc_channel *irc_upgrade_current_channel;
extern struct t_irc_modelist *irc_upgrade_current_modelist;
extern struct t_irc_redirect_pattern *irc_redirect_pattern_search (const char *name);
extern void irc_redirect_pattern_free (struct t_irc_redirect_pattern *redirect_pattern);
extern void irc_raw_message_free_all (void);
extern int irc_upgrade_read_cb (const void *pointer, void *data,
                                struct t_upgrade_file *upgrade_file,
                                int object_id,
                                struct t_infolist *infolist);
}

#define IRC_FAKE_SERVER "fake_upgrade"
#define SRV_BUFFER "irc.server." IRC_FAKE_SERVER
#define SERVER_ADD_COMMAND                                              \
    "/mute /server add " IRC_FAKE_SERVER " fake:127.0.0.1 "             \
    "-nicks=alice -anti_flood=0 -notify=dave"

#define RECV(__irc_msg)                                                 \
    server_recv (__irc_msg);

TEST_GROUP(IrcUpgrade)
{
    struct t_irc_server *server = NULL;
    char *upgrade_filename = NULL;

    void server_recv (const char *command)
    {
        char str_command[8192];

        snprintf (str_command, sizeof (str_command),
                  "/command -buffer " SRV_BUFFER " irc "
                  "/server fakerecv \"%s\"",
                  command);
        run_cmd_quiet (str_command);
    }

    void setup ()
    {
        config_file_option_set (logger_config_look_backlog, "0", 1);
        run_cmd_quiet (SERVER_ADD_COMMAND);
        server = irc_server_search (IRC_FAKE_SERVER);
        CHECK(server);
        /* Server buffer (needed to close channel buffers with the server). */
        CHECK(irc_server_create_buffer (server));
        upgrade_filename = string_eval_path_home (
            "${weechat_data_dir}/" IRC_UPGRADE_FILENAME ".upgrade",
            NULL, NULL, NULL);
        unlink (upgrade_filename);
        irc_upgrade_current_server = NULL;
        irc_upgrade_current_channel = NULL;
        irc_upgrade_current_modelist = NULL;
    }

    void teardown ()
    {
        struct t_irc_redirect_pattern *ptr_pattern;

        delete_server ();
        ptr_pattern = irc_redirect_pattern_search ("test_upgrade");
        if (ptr_pattern)
            irc_redirect_pattern_free (ptr_pattern);
        irc_upgrade_current_server = NULL;
        irc_upgrade_current_channel = NULL;
        irc_upgrade_current_modelist = NULL;
        unlink (upgrade_filename);
        free (upgrade_filename);
        config_file_option_reset (logger_config_look_backlog, 1);
    }

    /* Delete the fake server (and its buffers). */
    void delete_server ()
    {
        if (irc_server_search (IRC_FAKE_SERVER))
        {
            run_cmd_quiet ("/mute /disconnect " IRC_FAKE_SERVER);
            run_cmd_quiet ("/mute /server del " IRC_FAKE_SERVER);
        }
        server = NULL;
    }

    /*
     * Simulate an upgrade: the server is deleted then created again from
     * configuration (not connected, without channels), with its buffer (on
     * upgrade, buffers are kept).
     */
    void recreate_server ()
    {
        delete_server ();
        run_cmd_quiet (SERVER_ADD_COMMAND);
        server = irc_server_search (IRC_FAKE_SERVER);
        CHECK(server);
        CHECK(irc_server_create_buffer (server));
        server->buffer = NULL;
        LONGS_EQUAL(0, server->is_connected);
        POINTERS_EQUAL(NULL, server->channels);
    }

    /* Connect to the fake server and join channel #test with some nicks. */
    void connect_and_join ()
    {
        run_cmd_quiet ("/connect " IRC_FAKE_SERVER);
        RECV(":server 001 alice :Welcome");
        RECV(":server 005 alice PREFIX=(ohv)@%+ CHANTYPES=#& "
             "CHANMODES=beI,k,l,imnst MONITOR=100 LINELEN=4096 USERLEN=10 "
             "HOSTLEN=64 UTF8MAPPING=rfc8265 UTF8ONLY CLIENTTAGDENY=*,-a "
             ":are supported");
        RECV(":alice!user@host JOIN #test");
        RECV(":server 332 alice #test :the topic");
        RECV(":server 353 alice = #test :@alice +bob carol");
        RECV(":server 366 alice #test :End of /NAMES list");
        RECV(":server 324 alice #test +ntk secret");
        RECV(":server 367 alice #test *!*@spam setter!u@h 1700000000");
        RECV(":server 368 alice #test :End of channel ban list");
        RECV(":bob!user_b@host_b PRIVMSG #test :hello");
        RECV(":server 306 alice :You have been marked as being away");
    }
};

/*
 * Test functions:
 *   irc_upgrade_save
 *   irc_upgrade_save_all_data
 *   irc_upgrade_load
 *   irc_upgrade_read_cb
 *   irc_upgrade_set_buffer_callbacks
 *   irc_upgrade_set_buffer_properties
 */

TEST(IrcUpgrade, SaveLoad)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    struct t_irc_modelist *ptr_modelist;
    struct t_irc_notify *ptr_notify;
    struct t_irc_redirect *ptr_redirect;
    struct t_irc_redirect_pattern *ptr_pattern;
    struct t_gui_buffer *ptr_buffer;
    int raw_count;

    connect_and_join ();
    ptr_notify = irc_notify_search (server, "dave");
    CHECK(ptr_notify);
    ptr_notify->is_on_server = 1;
    ptr_notify->away_message = strdup ("gone");
    CHECK(irc_redirect_new (server, "whois", "test", 1, "bob", 30, NULL));
    CHECK(irc_redirect_pattern_new ("test_upgrade", 1, 30,
                                    "311", "318", "319"));
    raw_count = irc_raw_messages_count;
    CHECK(raw_count > 0);

    LONGS_EQUAL(1, irc_upgrade_save (0));
    CHECK(access (upgrade_filename, F_OK) == 0);

    /* Simulate the upgrade. */
    recreate_server ();
    irc_raw_message_free_all ();
    LONGS_EQUAL(0, irc_raw_messages_count);
    irc_redirect_pattern_free (irc_redirect_pattern_search ("test_upgrade"));

    LONGS_EQUAL(1, irc_upgrade_load ());
    LONGS_EQUAL(0, irc_upgrading);

    /* Server (buffer found by its name). */
    CHECK(server->buffer);
    POINTERS_EQUAL(gui_buffer_search_by_full_name (SRV_BUFFER), server->buffer);
    LONGS_EQUAL(1, server->fake_server);
    LONGS_EQUAL(1, server->is_connected);
    STRCMP_EQUAL("alice", server->nick);
    STRCMP_EQUAL("ohv", server->prefix_modes);
    STRCMP_EQUAL("@%+", server->prefix_chars);
    STRCMP_EQUAL("#&", server->chantypes);
    STRCMP_EQUAL("beI,k,l,imnst", server->chanmodes);
    LONGS_EQUAL(100, server->monitor);
    LONGS_EQUAL(4096, server->msg_max_length);
    LONGS_EQUAL(10, server->user_max_length);
    LONGS_EQUAL(64, server->host_max_length);
    LONGS_EQUAL(1, server->utf8only);
    LONGS_EQUAL(1, server->is_away);
    CHECK(server->isupport);
    CHECK(strstr (server->isupport, "MONITOR=100"));
    STRCMP_EQUAL("127.0.0.1", server->current_address);

    /* Channel. */
    ptr_channel = irc_channel_search (server, "#test");
    CHECK(ptr_channel);
    CHECK(ptr_channel->buffer);
    STRCMP_EQUAL("the topic", ptr_channel->topic);
    STRCMP_EQUAL("+ntk secret", ptr_channel->modes);
    STRCMP_EQUAL("secret", ptr_channel->key);
    LONGS_EQUAL(3, ptr_channel->nicks_count);
    CHECK(ptr_channel->nicks_speaking[0]);
    CHECK(weelist_search (ptr_channel->nicks_speaking[0], "bob"));
    CHECK(ptr_channel->nicks_speaking_time);
    STRCMP_EQUAL("bob", ptr_channel->nicks_speaking_time->nick);

    /* Nicks. */
    ptr_nick = irc_nick_search (server, ptr_channel, "alice");
    CHECK(ptr_nick);
    STRCMP_EQUAL("@  ", ptr_nick->prefixes);
    ptr_nick = irc_nick_search (server, ptr_channel, "bob");
    CHECK(ptr_nick);
    STRCMP_EQUAL("  +", ptr_nick->prefixes);
    STRCMP_EQUAL("user_b@host_b", ptr_nick->host);
    CHECK(irc_nick_search (server, ptr_channel, "carol"));

    /* Modelist "b" (bans). */
    ptr_modelist = irc_modelist_search (ptr_channel, 'b');
    CHECK(ptr_modelist);
    CHECK(ptr_modelist->items);
    STRCMP_EQUAL("*!*@spam", ptr_modelist->items->mask);
    STRCMP_EQUAL("setter!u@h", ptr_modelist->items->setter);
    LONGS_EQUAL(1700000000, ptr_modelist->items->datetime);

    /* Notify, redirect, redirect pattern, raw messages. */
    ptr_notify = irc_notify_search (server, "dave");
    CHECK(ptr_notify);
    LONGS_EQUAL(1, ptr_notify->is_on_server);
    STRCMP_EQUAL("gone", ptr_notify->away_message);
    for (ptr_redirect = server->redirects; ptr_redirect;
         ptr_redirect = ptr_redirect->next_redirect)
    {
        if (strcmp (ptr_redirect->signal, "test") == 0)
            break;
    }
    CHECK(ptr_redirect);
    STRCMP_EQUAL("whois", ptr_redirect->pattern);
    STRCMP_EQUAL("bob", ptr_redirect->string);
    LONGS_EQUAL(30, ptr_redirect->timeout);
    ptr_pattern = irc_redirect_pattern_search ("test_upgrade");
    CHECK(ptr_pattern);
    LONGS_EQUAL(1, ptr_pattern->temp_pattern);
    STRCMP_EQUAL("311", ptr_pattern->cmd_start);
    LONGS_EQUAL(raw_count, irc_raw_messages_count);

    /* Buffer callbacks restored on channel buffer. */
    ptr_buffer = ptr_channel->buffer;
    CHECK(ptr_buffer->close_callback);
    CHECK(ptr_buffer->input_callback);
    CHECK(ptr_buffer->nickcmp_callback);
    POINTERS_EQUAL(server, ptr_buffer->nickcmp_callback_pointer);
}

TEST(IrcUpgrade, SaveLoadDisconnected)
{
    struct t_irc_channel *ptr_channel;

    connect_and_join ();

    /* Save in disconnected state: nicks and modelists are not saved. */
    LONGS_EQUAL(1, irc_upgrade_save (1));

    recreate_server ();
    LONGS_EQUAL(1, irc_upgrade_load ());

    CHECK(server->buffer);
    LONGS_EQUAL(0, server->is_connected);
    ptr_channel = irc_channel_search (server, "#test");
    CHECK(ptr_channel);
    LONGS_EQUAL(0, ptr_channel->nicks_count);
    POINTERS_EQUAL(NULL, irc_modelist_search (ptr_channel, 'b')->items);
}

TEST(IrcUpgrade, LoadNoFile)
{
    /* No upgrade file: error. */
    LONGS_EQUAL(0, irc_upgrade_load ());
}

/*
 * Test functions:
 *   irc_upgrade_read_cb (data saved by old WeeChat versions)
 */

TEST(IrcUpgrade, ReadOldVersion)
{
    struct t_infolist *infolist;
    struct t_infolist_item *item;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    struct t_irc_modelist *ptr_modelist;

    /* Server: old variables, values read from "isupport". */
    infolist = infolist_new (NULL);
    item = infolist_new_item (infolist);
    infolist_new_var_string (item, "name", IRC_FAKE_SERVER);
    infolist_new_var_string (item, "buffer_name", "server." IRC_FAKE_SERVER);
    infolist_new_var_integer (item, "fake_server", 1);
    infolist_new_var_integer (item, "index_current_address", 0);
    infolist_new_var_integer (item, "sock", -1);
    infolist_new_var_integer (item, "is_connected", 1);
    infolist_new_var_integer (item, "ssl_connected", 1);
    infolist_new_var_string (item, "nick", "alice");
    infolist_new_var_integer (item, "cap_away_notify", 1);
    infolist_new_var_integer (item, "cap_account_notify", 1);
    infolist_new_var_integer (item, "cap_extended_join", 1);
    infolist_new_var_string (item, "prefix", "(ov)@+");
    infolist_new_var_string (item, "isupport",
                             "LINELEN=1024 USERLEN=12 HOSTLEN=70 "
                             "UTF8MAPPING=rfc8265 UTF8ONLY CHANMODES=b,k,l,nt "
                             "MONITOR=50 CLIENTTAGDENY=*");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_upgrade_read_cb (NULL, NULL, NULL,
                                     IRC_UPGRADE_TYPE_SERVER, infolist));
    infolist_free (infolist);
    POINTERS_EQUAL(server, irc_upgrade_current_server);
    POINTERS_EQUAL(gui_buffer_search_by_full_name (SRV_BUFFER), server->buffer);
    LONGS_EQUAL(1, server->is_connected);
    LONGS_EQUAL(1, server->tls_connected);
    LONGS_EQUAL(IRC_SERVER_AUTH_METHOD_NONE, server->authentication_method);
    LONGS_EQUAL(-1, server->sasl_mechanism_used);
    STRCMP_EQUAL("127.0.0.1", server->current_address);
    CHECK(hashtable_has_key (server->cap_list, "away-notify"));
    CHECK(hashtable_has_key (server->cap_list, "account-notify"));
    CHECK(hashtable_has_key (server->cap_list, "extended-join"));
    STRCMP_EQUAL("ov", server->prefix_modes);
    STRCMP_EQUAL("@+", server->prefix_chars);
    LONGS_EQUAL(1024, server->msg_max_length);
    LONGS_EQUAL(12, server->user_max_length);
    LONGS_EQUAL(70, server->host_max_length);
    LONGS_EQUAL(1, server->utf8only);
    STRCMP_EQUAL("b,k,l,nt", server->chanmodes);
    LONGS_EQUAL(50, server->monitor);

    /* Channel with nicks speaking and smart filtered joins. */
    infolist = infolist_new (NULL);
    item = infolist_new_item (infolist);
    infolist_new_var_integer (item, "type", IRC_CHANNEL_TYPE_CHANNEL);
    infolist_new_var_string (item, "name", "#old");
    infolist_new_var_string (item, "join_msg_received", "332,353");
    infolist_new_var_string (item, "nick_speaking0_00000", "bob");
    infolist_new_var_string (item, "nick_speaking1_00000", "carol");
    infolist_new_var_string (item, "nick_speaking_time_nick_00000", "bob");
    infolist_new_var_time (item, "nick_speaking_time_time_00000", time (NULL));
    infolist_new_var_string (item, "join_smart_filtered",
                             "bob:1700000000,invalid,carol:abc");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_upgrade_read_cb (NULL, NULL, NULL,
                                     IRC_UPGRADE_TYPE_CHANNEL, infolist));
    infolist_free (infolist);
    ptr_channel = irc_channel_search (server, "#old");
    CHECK(ptr_channel);
    POINTERS_EQUAL(ptr_channel, irc_upgrade_current_channel);
    CHECK(hashtable_has_key (ptr_channel->join_msg_received, "332"));
    CHECK(hashtable_has_key (ptr_channel->join_msg_received, "353"));
    CHECK(weelist_search (ptr_channel->nicks_speaking[0], "bob"));
    CHECK(weelist_search (ptr_channel->nicks_speaking[1], "carol"));
    LONGS_EQUAL(1, ptr_channel->join_smart_filtered->items_count);
    CHECK(hashtable_has_key (ptr_channel->join_smart_filtered, "bob"));

    /* Nick with old flags (op, voice, away). */
    infolist = infolist_new (NULL);
    item = infolist_new_item (infolist);
    infolist_new_var_string (item, "name", "bob");
    infolist_new_var_string (item, "host", "user@host");
    infolist_new_var_string (item, "prefixes", "  ");
    infolist_new_var_integer (item, "flags", 8 | 32 | 64);
    item = infolist_new_item (infolist);
    infolist_new_var_string (item, "name", "carol");
    infolist_new_var_string (item, "prefixes", "  ");
    infolist_new_var_integer (item, "flags", 1 | 2 | 16 | 128);
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_upgrade_read_cb (NULL, NULL, NULL,
                                     IRC_UPGRADE_TYPE_NICK, infolist));
    infolist_free (infolist);
    ptr_nick = irc_nick_search (server, ptr_channel, "bob");
    CHECK(ptr_nick);
    STRCMP_EQUAL("@+", ptr_nick->prefixes);
    LONGS_EQUAL(1, ptr_nick->away);
    CHECK(irc_nick_search (server, ptr_channel, "carol"));

    /* Modelist and item. */
    infolist = infolist_new (NULL);
    item = infolist_new_item (infolist);
    infolist_new_var_string (item, "type", "b");
    infolist_new_var_integer (item, "state", IRC_MODELIST_STATE_RECEIVED);
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_upgrade_read_cb (NULL, NULL, NULL,
                                     IRC_UPGRADE_TYPE_MODELIST, infolist));
    infolist_free (infolist);
    ptr_modelist = irc_modelist_search (ptr_channel, 'b');
    POINTERS_EQUAL(ptr_modelist, irc_upgrade_current_modelist);
    LONGS_EQUAL(IRC_MODELIST_STATE_RECEIVED, ptr_modelist->state);
    infolist = infolist_new (NULL);
    item = infolist_new_item (infolist);
    infolist_new_var_string (item, "mask", "*!*@bad");
    infolist_new_var_integer (item, "number", 5);
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_upgrade_read_cb (NULL, NULL, NULL,
                                     IRC_UPGRADE_TYPE_MODELIST_ITEM,
                                     infolist));
    infolist_free (infolist);
    CHECK(ptr_modelist->items);
    STRCMP_EQUAL("*!*@bad", ptr_modelist->items->mask);
    LONGS_EQUAL(5, ptr_modelist->items->number);

    /* Raw message without server (WeeChat < 2.7): ignored. */
    irc_raw_message_free_all ();
    infolist = infolist_new (NULL);
    item = infolist_new_item (infolist);
    infolist_new_var_string (item, "message", "PING :server");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_upgrade_read_cb (NULL, NULL, NULL,
                                     IRC_UPGRADE_TYPE_RAW_MESSAGE, infolist));
    infolist_free (infolist);
    LONGS_EQUAL(0, irc_raw_messages_count);
}

TEST(IrcUpgrade, ReadUnknownServer)
{
    struct t_infolist *infolist;
    struct t_infolist_item *item;

    /* Server not found: its channels, nicks, notify, redirects are ignored. */
    infolist = infolist_new (NULL);
    item = infolist_new_item (infolist);
    infolist_new_var_string (item, "name", "unknown_server");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_upgrade_read_cb (NULL, NULL, NULL,
                                     IRC_UPGRADE_TYPE_SERVER, infolist));
    POINTERS_EQUAL(NULL, irc_upgrade_current_server);
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_upgrade_read_cb (NULL, NULL, NULL,
                                     IRC_UPGRADE_TYPE_CHANNEL, infolist));
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_upgrade_read_cb (NULL, NULL, NULL,
                                     IRC_UPGRADE_TYPE_NICK, infolist));
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_upgrade_read_cb (NULL, NULL, NULL,
                                     IRC_UPGRADE_TYPE_MODELIST, infolist));
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_upgrade_read_cb (NULL, NULL, NULL,
                                     IRC_UPGRADE_TYPE_MODELIST_ITEM,
                                     infolist));
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_upgrade_read_cb (NULL, NULL, NULL,
                                     IRC_UPGRADE_TYPE_REDIRECT, infolist));
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_upgrade_read_cb (NULL, NULL, NULL,
                                     IRC_UPGRADE_TYPE_NOTIFY, infolist));
    infolist_free (infolist);
    POINTERS_EQUAL(NULL, server->channels);

    /* Raw message of an unknown server: ignored. */
    irc_raw_message_free_all ();
    infolist = infolist_new (NULL);
    item = infolist_new_item (infolist);
    infolist_new_var_string (item, "server", "unknown_server");
    infolist_new_var_string (item, "message", "PING :server");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_upgrade_read_cb (NULL, NULL, NULL,
                                     IRC_UPGRADE_TYPE_RAW_MESSAGE, infolist));
    infolist_free (infolist);
    LONGS_EQUAL(0, irc_raw_messages_count);
}
