/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test OTR buffer functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include "src/core/core-config-file.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-line.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-server.h"
#include "src/plugins/otr/otr.h"
#include "src/plugins/otr/otr-buffer.h"
#include "src/plugins/otr/otr-config.h"
#include "src/plugins/otr/otr-context.h"
#include "src/plugins/otr/otr-log.h"
}

#define OTR_TEST_LOGGER_OPTION "logger.level.irc.test_otr_pv.bob"

TEST_GROUP(OtrBuffer)
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
 *   otr_buffer_irc_pv_opened_cb
 */

TEST(OtrBuffer, IrcPvOpenedCb)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_config_option *ptr_option;
    struct t_otr_peer_data *ptr_peer_data;
    ConnContext *ptr_context;

    run_cmd_quiet ("/mute /server add test_otr_pv fake:127.0.0.1");
    run_cmd_quiet ("/connect test_otr_pv");

    server = irc_server_search ("test_otr_pv");
    CHECK(server);
    irc_server_set_nick (server, "alice");

    /* No OTR session: logging is unchanged. */
    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_PRIVATE, "bob", 0, 0);
    CHECK(channel);
    POINTERS_EQUAL(NULL, logger_option ());
    gui_buffer_close (channel->buffer);

    /* Encrypted OTR session started before the private buffer is opened. */
    ptr_context = otrl_context_find (otr_us, "bob", "alice@test_otr_pv",
                                     OTR_PROTOCOL, OTRL_INSTAG_MASTER, 1,
                                     NULL, NULL, NULL);
    CHECK(ptr_context);
    ptr_context->msgstate = OTRL_MSGSTATE_ENCRYPTED;
    ptr_peer_data = otr_context_peer_data_ctx (ptr_context);
    CHECK(ptr_peer_data);
    LONGS_EQUAL(OTR_LOG_UNTOUCHED, ptr_peer_data->prev_log);

    /* Logging is disabled when the private buffer is opened. */
    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_PRIVATE, "bob", 0, 0);
    CHECK(channel);
    ptr_option = logger_option ();
    CHECK(ptr_option);
    LONGS_EQUAL(0, CONFIG_INTEGER(ptr_option));
    LONGS_EQUAL(OTR_LOG_DEFAULT, ptr_peer_data->prev_log);

    /* End of session: logging is restored. */
    ptr_context->msgstate = OTRL_MSGSTATE_PLAINTEXT;
    otr_log_restore (ptr_context, ptr_peer_data, 0);
    POINTERS_EQUAL(NULL, logger_option ());
    LONGS_EQUAL(OTR_LOG_UNTOUCHED, ptr_peer_data->prev_log);

    gui_buffer_close (channel->buffer);

    run_cmd_quiet ("/mute /disconnect test_otr_pv");
    run_cmd_quiet ("/mute /server del test_otr_pv");
}

/*
 * Test functions:
 *   otr_buffer_print_str_tags
 *   otr_buffer_peer_print
 */

TEST(OtrBuffer, PeerPrint)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_gui_line *ptr_line;

    run_cmd_quiet ("/mute /server add test_otr_print fake:127.0.0.1");
    run_cmd_quiet ("/connect test_otr_print");
    server = irc_server_search ("test_otr_print");
    CHECK(server);
    irc_server_set_nick (server, "alice");

    /* No private buffer: message in core buffer, not logged. */
    otr_buffer_peer_print ("test_otr_print", "bob", OTR_CONFIG_LEVEL_INFO,
                           "secret question");
    ptr_line = gui_buffers->own_lines->last_line;
    CHECK(strstr (ptr_line->data->message, "[bob] secret question"));
    STRCMP_EQUAL("no_log", gui_line_search_tag_starting_with (ptr_line,
                                                              "no_log"));

    /* Private buffer: message in this buffer, without tags. */
    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_PRIVATE, "bob", 0, 0);
    CHECK(channel);
    otr_buffer_peer_print ("test_otr_print", "bob", OTR_CONFIG_LEVEL_INFO,
                           "message in private");
    ptr_line = channel->buffer->own_lines->last_line;
    STRCMP_EQUAL("message in private", ptr_line->data->message
                 + strlen (ptr_line->data->message)
                 - strlen ("message in private"));
    LONGS_EQUAL(0, ptr_line->data->tags_count);

    gui_buffer_close (channel->buffer);

    run_cmd_quiet ("/mute /disconnect test_otr_print");
    run_cmd_quiet ("/mute /server del test_otr_print");
}
