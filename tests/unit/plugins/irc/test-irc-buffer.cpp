/*
 * SPDX-FileCopyrightText: 2021-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC buffer functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include "src/gui/gui-buffer.h"
#include "src/plugins/irc/irc-buffer.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-server.h"
}

#define CHANNEL_300 "#this_channel_name_has_300_chars_xxxxxxxxxxxxxxxxxxxxx" \
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" \
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" \
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" \
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx_END"

TEST_GROUP(IrcBuffer)
{
};

/*
 * Test functions:
 *   irc_buffer_get_server_and_channel
 */

TEST(IrcBuffer, GetServerAndChannel)
{
    struct t_irc_server *server, *ptr_server;
    struct t_irc_channel *channel, *ptr_channel;
    struct t_gui_buffer *buffer_chan, *buffer_pv;

    ptr_server = (struct t_irc_server *)0x1;
    ptr_channel = (struct t_irc_channel *)0x1;
    irc_buffer_get_server_and_channel (NULL, &ptr_server, &ptr_channel);
    POINTERS_EQUAL(NULL, ptr_server);
    POINTERS_EQUAL(NULL, ptr_channel);

    irc_buffer_get_server_and_channel (NULL, NULL, NULL);

    /* Core buffer */
    ptr_server = (struct t_irc_server *)0x1;
    ptr_channel = (struct t_irc_channel *)0x1;
    irc_buffer_get_server_and_channel (gui_buffer_search_main (),
                                       &ptr_server, &ptr_channel);
    POINTERS_EQUAL(NULL, ptr_server);
    POINTERS_EQUAL(NULL, ptr_channel);

    run_cmd_quiet ("/mute /server add test_buffer fake:127.0.0.1");
    run_cmd_quiet ("/connect test_buffer");

    server = irc_server_search ("test_buffer");
    CHECK(server);
    CHECK(server->buffer);

    /* Core buffer with local variable "server": ignored */
    gui_buffer_set (gui_buffer_search_main (), "localvar_set_server",
                    "test_buffer");
    gui_buffer_set (gui_buffer_search_main (), "localvar_set_type",
                    "channel");
    irc_buffer_get_server_and_channel (gui_buffer_search_main (),
                                       &ptr_server, &ptr_channel);
    POINTERS_EQUAL(NULL, ptr_server);
    POINTERS_EQUAL(NULL, ptr_channel);
    gui_buffer_set (gui_buffer_search_main (), "localvar_del_server", "");
    gui_buffer_set (gui_buffer_search_main (), "localvar_del_type", "");

    /* Server buffer */
    ptr_channel = (struct t_irc_channel *)0x1;
    irc_buffer_get_server_and_channel (server->buffer,
                                       &ptr_server, &ptr_channel);
    POINTERS_EQUAL(server, ptr_server);
    POINTERS_EQUAL(NULL, ptr_channel);

    /* Channel/private buffers without channel (eg: JOIN not received) */
    buffer_chan = irc_channel_create_buffer (
        server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(buffer_chan);
    buffer_pv = irc_channel_create_buffer (
        server, IRC_CHANNEL_TYPE_PRIVATE, "bob", 0, 0);
    CHECK(buffer_pv);
    ptr_channel = (struct t_irc_channel *)0x1;
    irc_buffer_get_server_and_channel (buffer_chan, &ptr_server, &ptr_channel);
    POINTERS_EQUAL(server, ptr_server);
    POINTERS_EQUAL(NULL, ptr_channel);
    ptr_channel = (struct t_irc_channel *)0x1;
    irc_buffer_get_server_and_channel (buffer_pv, &ptr_server, &ptr_channel);
    POINTERS_EQUAL(server, ptr_server);
    POINTERS_EQUAL(NULL, ptr_channel);
    irc_buffer_get_server_and_channel (buffer_chan, NULL, NULL);

    /* Unknown server in local variable "server" */
    gui_buffer_set (buffer_chan, "localvar_set_server", "unknown_server");
    ptr_server = (struct t_irc_server *)0x1;
    irc_buffer_get_server_and_channel (buffer_chan, &ptr_server, &ptr_channel);
    POINTERS_EQUAL(NULL, ptr_server);
    POINTERS_EQUAL(NULL, ptr_channel);
    gui_buffer_set (buffer_chan, "localvar_set_server", "test_buffer");

    /* Channel buffer with channel */
    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test",
                               0, 0);
    CHECK(channel);
    POINTERS_EQUAL(buffer_chan, channel->buffer);
    irc_buffer_get_server_and_channel (buffer_chan, &ptr_server, &ptr_channel);
    POINTERS_EQUAL(server, ptr_server);
    POINTERS_EQUAL(channel, ptr_channel);
    ptr_server = (struct t_irc_server *)0x1;
    irc_buffer_get_server_and_channel (buffer_chan, NULL, &ptr_channel);
    POINTERS_EQUAL(channel, ptr_channel);
    irc_buffer_get_server_and_channel (buffer_chan, &ptr_server, NULL);
    POINTERS_EQUAL(server, ptr_server);

    gui_buffer_close (buffer_chan);
    gui_buffer_close (buffer_pv);

    run_cmd_quiet ("/mute /disconnect test_buffer");
    run_cmd_quiet ("/mute /server del test_buffer");
}

/*
 * Test functions:
 *   irc_buffer_build_name
 */

TEST(IrcBuffer, BuildName)
{
    char *str;

    WEE_TEST_STR("", irc_buffer_build_name (NULL, NULL));
    WEE_TEST_STR(".", irc_buffer_build_name ("", ""));

    /* Only server */
    WEE_TEST_STR("server.libera", irc_buffer_build_name ("libera", NULL));

    /* Only channel */
    WEE_TEST_STR("#chan1", irc_buffer_build_name (NULL, "#chan1"));

    /* Server and channel */
    WEE_TEST_STR("libera.#chan1", irc_buffer_build_name ("libera", "#chan1"));
    WEE_TEST_STR("libera." CHANNEL_300,
                 irc_buffer_build_name ("libera", CHANNEL_300));
}

/*
 * Test functions:
 *   irc_buffer_close_server_channels
 */

TEST(IrcBuffer, CloseServerChannels)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_buffer_search_server_lowest_number
 */

TEST(IrcBuffer, SearchServerLowestNumber)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_buffer_search_private_lowest_number
 */

TEST(IrcBuffer, SearchPrivateLowestNumber)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_buffer_move_near_server
 */

TEST(IrcBuffer, IrcBufferMoveNearServer)
{
    /* TODO: write tests */
}
