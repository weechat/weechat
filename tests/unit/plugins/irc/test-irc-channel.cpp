/*
 * SPDX-FileCopyrightText: 2019-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC channel functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <string.h>
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-color.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-nick.h"
#include "src/plugins/irc/irc-server.h"

extern void irc_nick_set_prefixes (struct t_irc_server *server,
                                   struct t_irc_nick *nick,
                                   const char *prefixes);
}

TEST_GROUP(IrcChannel)
{
};

/*
 * Test functions:
 *   irc_channel_valid
 */

TEST(IrcChannel, Valid)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_channel_is_channel
 */

TEST(IrcChannel, IsChannel)
{
    struct t_irc_server *server;

    /* No server, default chantypes = "#&+!" */

    /* Empty channel */
    LONGS_EQUAL(0, irc_channel_is_channel (NULL, NULL));
    LONGS_EQUAL(0, irc_channel_is_channel (NULL, ""));

    /* Not a channel */
    LONGS_EQUAL(0, irc_channel_is_channel (NULL, "abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (NULL, "/abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (NULL, ":abc"));

    /* Valid channel */
    LONGS_EQUAL(1, irc_channel_is_channel (NULL, "#abc"));
    LONGS_EQUAL(1, irc_channel_is_channel (NULL, "##abc"));
    LONGS_EQUAL(1, irc_channel_is_channel (NULL, "&abc"));
    LONGS_EQUAL(1, irc_channel_is_channel (NULL, "&&abc"));

    /* Server with chantypes = "#" */
    server = irc_server_alloc ("my_ircd");
    CHECK(server);
    if (server->chantypes)
        free (server->chantypes);
    server->chantypes = strdup ("#");

    /* Empty channel */
    LONGS_EQUAL(0, irc_channel_is_channel (server, NULL));
    LONGS_EQUAL(0, irc_channel_is_channel (server, ""));

    /* Not a channel */
    LONGS_EQUAL(0, irc_channel_is_channel (server, "abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (server, "/abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (server, ":abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (server, "&abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (server, "+abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (server, "!abc"));

    /* Valid channel */
    LONGS_EQUAL(1, irc_channel_is_channel (server, "#abc"));
    LONGS_EQUAL(1, irc_channel_is_channel (server, "##abc"));

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_channel_refresh_self_prefix
 */

TEST(IrcChannel, RefreshSelfPrefix)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_irc_nick *nick;
    char *prompt;

    irc_channel_refresh_self_prefix (NULL, NULL);

    run_cmd_quiet ("/mute /server add test_prefix fake:127.0.0.1");
    run_cmd_quiet ("/connect test_prefix");

    server = irc_server_search ("test_prefix");
    CHECK(server);
    irc_server_set_nick (server, "alice");

    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(channel);
    CHECK(channel->buffer);

    irc_channel_refresh_self_prefix (server, NULL);
    irc_channel_refresh_self_prefix (NULL, channel);

    nick = irc_nick_new (server, channel, "alice", "user@host", "@", 0, NULL, NULL);
    CHECK(nick);

    /* Prompt is refreshed with our own nick prefix. */
    gui_buffer_set (channel->buffer, "input_prompt", "");
    STRCMP_EQUAL(NULL, channel->buffer->input_prompt);
    irc_channel_refresh_self_prefix (server, channel);
    prompt = gui_color_decode (channel->buffer->input_prompt, NULL);
    STRCMP_EQUAL("@alice", prompt);
    free (prompt);

    /* Prompt is refreshed after our own nick prefix changed. */
    irc_nick_set_prefixes (server, nick, NULL);
    prompt = gui_color_decode (channel->buffer->input_prompt, NULL);
    STRCMP_EQUAL("@alice", prompt);
    free (prompt);
    irc_channel_refresh_self_prefix (server, channel);
    prompt = gui_color_decode (channel->buffer->input_prompt, NULL);
    STRCMP_EQUAL("alice", prompt);
    free (prompt);

    /* Prompt is refreshed when our own nick is removed from channel. */
    irc_nick_set_prefixes (server, nick, "@");
    irc_channel_refresh_self_prefix (server, channel);
    prompt = gui_color_decode (channel->buffer->input_prompt, NULL);
    STRCMP_EQUAL("@alice", prompt);
    free (prompt);
    irc_nick_free (server, channel, nick);
    prompt = gui_color_decode (channel->buffer->input_prompt, NULL);
    STRCMP_EQUAL("alice", prompt);
    free (prompt);

    gui_buffer_close (channel->buffer);

    run_cmd_quiet ("/mute /disconnect test_prefix");
    run_cmd_quiet ("/mute /server del test_prefix");
}
