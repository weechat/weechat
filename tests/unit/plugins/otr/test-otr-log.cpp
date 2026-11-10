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

    /* OTR session while the buffer is not logged: level is kept. */
    ptr_context = otrl_context_find (otr_us, "bob", "alice@test_otr_log",
                                     OTR_PROTOCOL, OTRL_INSTAG_MASTER, 1,
                                     NULL, NULL, NULL);
    CHECK(ptr_context);
    ptr_peer_data = otr_context_peer_data_ctx (ptr_context);
    CHECK(ptr_peer_data);
    otr_log_disable (ptr_context, ptr_peer_data);
    LONGS_EQUAL(5, ptr_peer_data->prev_log);
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
