/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test OTR functions to control buffer logging */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include "src/core/core-config-file.h"
#include "src/gui/gui-buffer.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-server.h"
#include "src/plugins/otr/otr.h"
#include "src/plugins/otr/otr-context.h"
#include "src/plugins/otr/otr-log.h"
}

#define OTR_TEST_LOGGER_OPTION "logger.level.irc.test_otr_log.bob"

TEST_GROUP(OtrLog)
{
    struct t_config_option *logger_option ()
    {
        struct t_config_option *ptr_option;

        ptr_option = NULL;
        config_file_search_with_string (OTR_TEST_LOGGER_OPTION,
                                        NULL, NULL, &ptr_option, NULL);
        return ptr_option;
    }

    struct t_config_option *option (const char *name)
    {
        struct t_config_option *ptr_option;

        ptr_option = NULL;
        config_file_search_with_string (name, NULL, NULL, &ptr_option, NULL);
        return ptr_option;
    }
};

/*
 * Test functions:
 *   otr_log_buffer_is_logged
 *   otr_log_buffer_log_level
 *   otr_log_disable
 *   otr_log_restore
 */

TEST(OtrLog, LogLevelDisableRestore)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_config_option *ptr_option;
    struct t_otr_peer_data *ptr_peer_data;
    ConnContext *ptr_context;

    run_cmd_quiet ("/mute /server add test_otr_log fake:127.0.0.1");
    run_cmd_quiet ("/connect test_otr_log");

    server = irc_server_search ("test_otr_log");
    CHECK(server);
    irc_server_set_nick (server, "alice");

    /* Buffer not logged (option logger.file.auto_log is off). */
    run_cmd_quiet ("/mute /set logger.file.auto_log off");
    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_PRIVATE, "bob", 0, 0);
    CHECK(channel);
    LONGS_EQUAL(0, otr_log_buffer_is_logged (channel->buffer));

    /* No specific logger level for the buffer. */
    POINTERS_EQUAL(NULL, logger_option ());
    LONGS_EQUAL(OTR_LOG_DEFAULT, otr_log_buffer_log_level (channel->buffer));

    /* Specific logger level. */
    run_cmd_quiet ("/mute /set " OTR_TEST_LOGGER_OPTION " 5");
    LONGS_EQUAL(5, otr_log_buffer_log_level (channel->buffer));

    /*
     * OTR session while the buffer is not logged: level saved, and set to 0
     * so that logging can not start during the session.
     */
    ptr_context = otrl_context_find (otr_us, "bob", "alice@test_otr_log",
                                     OTR_PROTOCOL, OTRL_INSTAG_MASTER, 1,
                                     NULL, NULL, NULL);
    CHECK(ptr_context);
    ptr_peer_data = otr_context_peer_data_ctx (ptr_context);
    CHECK(ptr_peer_data);
    otr_log_disable (ptr_context, ptr_peer_data);
    LONGS_EQUAL(5, ptr_peer_data->prev_log);
    ptr_option = logger_option ();
    CHECK(ptr_option);
    LONGS_EQUAL(0, CONFIG_INTEGER(ptr_option));

    /*
     * Logging enabled during the session (auto_log on, then all buffers
     * checked again by logger on a level change): buffer not logged.
     */
    run_cmd_quiet ("/mute /set logger.file.auto_log on");
    run_cmd_quiet ("/mute /set logger.level.irc.test_otr_log.other 9");
    LONGS_EQUAL(0, otr_log_buffer_is_logged (channel->buffer));
    run_cmd_quiet ("/mute /unset logger.level.irc.test_otr_log.other");
    run_cmd_quiet ("/mute /set logger.file.auto_log off");

    otr_log_restore (ptr_context, ptr_peer_data, 0);
    LONGS_EQUAL(OTR_LOG_UNTOUCHED, ptr_peer_data->prev_log);
    ptr_option = logger_option ();
    CHECK(ptr_option);
    LONGS_EQUAL(5, CONFIG_INTEGER(ptr_option));

    run_cmd_quiet ("/mute /unset logger.file.auto_log");
    run_cmd_quiet ("/mute /unset " OTR_TEST_LOGGER_OPTION);

    gui_buffer_close (channel->buffer);

    run_cmd_quiet ("/mute /disconnect test_otr_log");
    run_cmd_quiet ("/mute /server del test_otr_log");
}

/*
 * Test functions:
 *   otr_log_save_level
 *   otr_log_buffer_renamed
 *   otr_buffer_renamed_cb
 */

TEST(OtrLog, BufferRenamed)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_config_option *ptr_option;
    struct t_otr_peer_data *ptr_peer_data;
    ConnContext *ptr_context;

    run_cmd_quiet ("/mute /server add test_otr_ren fake:127.0.0.1");
    run_cmd_quiet ("/connect test_otr_ren");
    server = irc_server_search ("test_otr_ren");
    CHECK(server);
    irc_server_set_nick (server, "alice");

    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_PRIVATE, "bob", 0, 0);
    CHECK(channel);
    LONGS_EQUAL(1, otr_log_buffer_is_logged (channel->buffer));

    ptr_context = otrl_context_find (otr_us, "bob", "alice@test_otr_ren",
                                     OTR_PROTOCOL, OTRL_INSTAG_MASTER, 1,
                                     NULL, NULL, NULL);
    CHECK(ptr_context);
    ptr_peer_data = otr_context_peer_data_ctx (ptr_context);
    CHECK(ptr_peer_data);

    /* Logging disabled with the buffer name "bob". */
    otr_log_disable (ptr_context, ptr_peer_data);
    LONGS_EQUAL(OTR_LOG_DEFAULT, ptr_peer_data->prev_log);
    STRCMP_EQUAL("logger.level.irc.test_otr_ren.bob",
                 ptr_peer_data->log_option);
    ptr_option = option ("logger.level.irc.test_otr_ren.bob");
    CHECK(ptr_option);
    LONGS_EQUAL(0, CONFIG_INTEGER(ptr_option));
    LONGS_EQUAL(0, otr_log_buffer_is_logged (channel->buffer));

    /*
     * Buffer renamed by the IRC plugin (case of nick changed): old option
     * restored, logging disabled with the new name.
     */
    irc_channel_pv_rename (server, channel, "Bob");
    POINTERS_EQUAL(NULL, option ("logger.level.irc.test_otr_ren.bob"));
    ptr_option = option ("logger.level.irc.test_otr_ren.Bob");
    CHECK(ptr_option);
    LONGS_EQUAL(0, CONFIG_INTEGER(ptr_option));
    LONGS_EQUAL(0, otr_log_buffer_is_logged (channel->buffer));
    LONGS_EQUAL(OTR_LOG_DEFAULT, ptr_peer_data->prev_log);
    STRCMP_EQUAL("logger.level.irc.test_otr_ren.Bob",
                 ptr_peer_data->log_option);

    /* End of session: logger level restored with the new name. */
    otr_log_restore (ptr_context, ptr_peer_data, 0);
    POINTERS_EQUAL(NULL, option ("logger.level.irc.test_otr_ren.Bob"));
    LONGS_EQUAL(OTR_LOG_UNTOUCHED, ptr_peer_data->prev_log);
    POINTERS_EQUAL(NULL, ptr_peer_data->log_option);
    LONGS_EQUAL(1, otr_log_buffer_is_logged (channel->buffer));

    gui_buffer_close (channel->buffer);

    run_cmd_quiet ("/mute /disconnect test_otr_ren");
    run_cmd_quiet ("/mute /server del test_otr_ren");
}

/*
 * Test functions:
 *   otr_log_disable (level already saved)
 */

TEST(OtrLog, DisableLevelSaved)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_config_option *ptr_option;
    struct t_otr_peer_data *ptr_peer_data;
    ConnContext *ptr_context;

    run_cmd_quiet ("/mute /server add test_otr_saved fake:127.0.0.1");
    run_cmd_quiet ("/connect test_otr_saved");
    server = irc_server_search ("test_otr_saved");
    CHECK(server);
    irc_server_set_nick (server, "alice");

    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_PRIVATE, "bob", 0, 0);
    CHECK(channel);

    ptr_context = otrl_context_find (otr_us, "bob", "alice@test_otr_saved",
                                     OTR_PROTOCOL, OTRL_INSTAG_MASTER, 1,
                                     NULL, NULL, NULL);
    CHECK(ptr_context);
    ptr_peer_data = otr_context_peer_data_ctx (ptr_context);
    CHECK(ptr_peer_data);

    /* First session: level saved, then logging enabled ("/otr log start"). */
    otr_log_save_level (ptr_peer_data, channel->buffer);
    LONGS_EQUAL(OTR_LOG_DEFAULT, ptr_peer_data->prev_log);
    run_cmd_quiet ("/mute /set logger.level.irc.test_otr_saved.bob 9");
    LONGS_EQUAL(1, otr_log_buffer_is_logged (channel->buffer));

    /*
     * New session started by the peer (level still saved): logging is
     * disabled, the level saved is kept.
     */
    otr_log_disable (ptr_context, ptr_peer_data);
    ptr_option = option ("logger.level.irc.test_otr_saved.bob");
    CHECK(ptr_option);
    LONGS_EQUAL(0, CONFIG_INTEGER(ptr_option));
    LONGS_EQUAL(0, otr_log_buffer_is_logged (channel->buffer));
    LONGS_EQUAL(OTR_LOG_DEFAULT, ptr_peer_data->prev_log);

    /* End of session: level before the first session restored. */
    otr_log_restore (ptr_context, ptr_peer_data, 0);
    POINTERS_EQUAL(NULL, option ("logger.level.irc.test_otr_saved.bob"));
    LONGS_EQUAL(1, otr_log_buffer_is_logged (channel->buffer));

    gui_buffer_close (channel->buffer);

    run_cmd_quiet ("/mute /disconnect test_otr_saved");
    run_cmd_quiet ("/mute /server del test_otr_saved");
}
