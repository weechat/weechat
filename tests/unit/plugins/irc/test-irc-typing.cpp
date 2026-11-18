/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC typing functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/gui/gui-buffer.h"
#include "src/plugins/weechat-plugin.h"
#include "src/plugins/irc/irc.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-config.h"
#include "src/plugins/irc/irc-server.h"
#include "src/plugins/irc/irc-typing.h"
}

#define WEE_CHECK_SENT(__message)                                       \
    LONGS_EQUAL(1, test_irc_typing_sent_count);                         \
    STRCMP_EQUAL(__message, test_irc_typing_sent);                      \
    test_irc_typing_sent_count = 0;                                     \
    test_irc_typing_sent[0] = '\0';

#define WEE_CHECK_NOT_SENT                                              \
    LONGS_EQUAL(0, test_irc_typing_sent_count);

#define WEE_SIGNAL(__signal)                                            \
    LONGS_EQUAL(WEECHAT_RC_OK,                                          \
                irc_typing_signal_typing_self_cb (                      \
                    NULL, NULL, __signal,                               \
                    WEECHAT_HOOK_SIGNAL_POINTER, channel->buffer));

int test_irc_typing_sent_count = 0;
char test_irc_typing_sent[1024];

/*
 * Callback for signal "xxx,irc_out1_*": save the message sent to the server.
 */

int
test_irc_typing_signal_irc_out_cb (const void *pointer, void *data,
                                   const char *signal, const char *type_data,
                                   void *signal_data)
{
    /* Make C++ compiler happy. */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    test_irc_typing_sent_count++;
    snprintf (test_irc_typing_sent, sizeof (test_irc_typing_sent),
              "%s", (signal_data) ? (const char *)signal_data : "");

    return WEECHAT_RC_OK;
}

TEST_GROUP(IrcTypingWithServer)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_hook *hook;

    void setup ()
    {
        config_file_option_set (irc_config_look_typing_status_self, "on", 1);

        run_cmd_quiet ("/mute /server add test_typing fake:127.0.0.1");
        run_cmd_quiet ("/connect test_typing");

        server = irc_server_search ("test_typing");
        CHECK(server);
        server->typing_allowed = 1;
        hashtable_set (server->cap_list, "message-tags", NULL);

        channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test",
                                   0, 0);
        CHECK(channel);
        CHECK(channel->buffer);

        test_irc_typing_sent_count = 0;
        test_irc_typing_sent[0] = '\0';
        hook = hook_signal (NULL, "test_typing,irc_out1_*",
                            &test_irc_typing_signal_irc_out_cb, NULL, NULL);
    }

    void teardown ()
    {
        unhook (hook);
        gui_buffer_close (channel->buffer);
        run_cmd_quiet ("/mute /disconnect test_typing");
        run_cmd_quiet ("/mute /server del test_typing");
        config_file_option_reset (irc_config_look_typing_status_self, 1);
    }
};

/*
 * Test functions:
 *   irc_typing_signal_typing_self_cb
 *   irc_typing_send_to_targets
 */

TEST(IrcTypingWithServer, SignalTypingSelfCb)
{
    LONGS_EQUAL(IRC_CHANNEL_TYPING_STATE_OFF, channel->typing_state);

    /* Not an IRC buffer: ignored. */
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_typing_signal_typing_self_cb (
                    NULL, NULL, "typing_self_typing",
                    WEECHAT_HOOK_SIGNAL_POINTER, gui_buffers));
    LONGS_EQUAL(IRC_CHANNEL_TYPING_STATE_OFF, channel->typing_state);

    /* Unknown signal: ignored. */
    WEE_SIGNAL("typing_self_unknown");
    LONGS_EQUAL(IRC_CHANNEL_TYPING_STATE_OFF, channel->typing_state);
    LONGS_EQUAL(0, channel->typing_active_received);

    WEE_SIGNAL("typing_self_typing");
    LONGS_EQUAL(IRC_CHANNEL_TYPING_STATE_ACTIVE, channel->typing_state);
    CHECK(channel->typing_active_received > 0);
    irc_typing_send_to_targets (server);
    WEE_CHECK_SENT("@+typing=active TAGMSG #test");

    WEE_SIGNAL("typing_self_paused");
    LONGS_EQUAL(IRC_CHANNEL_TYPING_STATE_PAUSED, channel->typing_state);
    irc_typing_send_to_targets (server);
    WEE_CHECK_SENT("@+typing=paused TAGMSG #test");
    LONGS_EQUAL(IRC_CHANNEL_TYPING_STATE_OFF, channel->typing_state);

    WEE_SIGNAL("typing_self_typing");
    WEE_SIGNAL("typing_self_cleared");
    LONGS_EQUAL(IRC_CHANNEL_TYPING_STATE_DONE, channel->typing_state);
    irc_typing_send_to_targets (server);
    WEE_CHECK_SENT("@+typing=done TAGMSG #test");
    LONGS_EQUAL(IRC_CHANNEL_TYPING_STATE_OFF, channel->typing_state);

    /* Nothing sent when state is "off". */
    irc_typing_send_to_targets (server);
    WEE_CHECK_NOT_SENT;

    /* Self typing status disabled: signal ignored. */
    config_file_option_set (irc_config_look_typing_status_self, "off", 1);
    WEE_SIGNAL("typing_self_typing");
    LONGS_EQUAL(IRC_CHANNEL_TYPING_STATE_OFF, channel->typing_state);
}

/*
 * Test functions:
 *   irc_typing_send_to_targets (timeout of state "active")
 */

TEST(IrcTypingWithServer, SendToTargetsActiveTimeout)
{
    WEE_SIGNAL("typing_self_typing");
    irc_typing_send_to_targets (server);
    WEE_CHECK_SENT("@+typing=active TAGMSG #test");

    /* Status "active" not sent again before 3 seconds. */
    irc_typing_send_to_targets (server);
    WEE_CHECK_NOT_SENT;

    /* Status "active" refreshed by signal: sent again after 3 seconds. */
    channel->typing_status_sent -= 4;
    channel->typing_active_received -= 4;
    WEE_SIGNAL("typing_self_typing");
    LONGS_EQUAL(IRC_CHANNEL_TYPING_STATE_ACTIVE, channel->typing_state);
    irc_typing_send_to_targets (server);
    WEE_CHECK_SENT("@+typing=active TAGMSG #test");
    LONGS_EQUAL(IRC_CHANNEL_TYPING_STATE_ACTIVE, channel->typing_state);

    /* Status "active" not refreshed, timeout not reached: sent again. */
    channel->typing_status_sent -= 4;
    channel->typing_active_received -= IRC_TYPING_ACTIVE_TIMEOUT - 1;
    irc_typing_send_to_targets (server);
    WEE_CHECK_SENT("@+typing=active TAGMSG #test");
    LONGS_EQUAL(IRC_CHANNEL_TYPING_STATE_ACTIVE, channel->typing_state);

    /* Status "active" not refreshed, timeout reached: "done" sent. */
    channel->typing_active_received -= 2;
    irc_typing_send_to_targets (server);
    WEE_CHECK_SENT("@+typing=done TAGMSG #test");
    LONGS_EQUAL(IRC_CHANNEL_TYPING_STATE_OFF, channel->typing_state);

    /* Nothing else sent. */
    irc_typing_send_to_targets (server);
    WEE_CHECK_NOT_SENT;
}
