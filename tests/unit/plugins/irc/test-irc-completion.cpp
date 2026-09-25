/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC completion functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include "src/core/core-arraylist.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-completion.h"
#include "src/plugins/weechat-plugin.h"
#include "src/plugins/irc/irc-server.h"

extern int irc_completion_notify_nicks_cb (const void *pointer, void *data,
                                           const char *completion_item,
                                           struct t_gui_buffer *buffer,
                                           struct t_gui_completion *completion);
}

#define IRC_FAKE_SERVER "fake"

#define WEE_CHECK_COMPLETION_WORD(__word, __index)                      \
    STRCMP_EQUAL(__word,                                                \
                 ((struct t_gui_completion_word *)arraylist_get (       \
                     completion->list, __index))->word);

TEST_GROUP(IrcCompletionWithServer)
{
    struct t_irc_server *ptr_server = NULL;

    void setup ()
    {
        /* Create a fake server (no I/O). */
        run_cmd_quiet ("/mute /server add " IRC_FAKE_SERVER " fake:127.0.0.1 "
                       "-nicks=nick1,nick2,nick3");

        /* Connect to the fake server. */
        run_cmd_quiet ("/connect " IRC_FAKE_SERVER);

        /* Get the server pointer. */
        ptr_server = irc_server_search (IRC_FAKE_SERVER);
    }

    void teardown ()
    {
        /* Disconnect and delete the fake server. */
        run_cmd_quiet ("/mute /disconnect " IRC_FAKE_SERVER);
        run_cmd_quiet ("/mute /server del " IRC_FAKE_SERVER);
        ptr_server = NULL;
    }
};

/*
 * Test functions:
 *   irc_completion_notify_nicks_cb
 */

TEST(IrcCompletionWithServer, NotifyNicks)
{
    struct t_gui_completion *completion;

    CHECK(ptr_server);
    CHECK(ptr_server->buffer);

    /* No notify. */
    completion = gui_completion_new (NULL, ptr_server->buffer);
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_completion_notify_nicks_cb (NULL, NULL, "irc_notify_nicks",
                                                ptr_server->buffer,
                                                completion));
    LONGS_EQUAL(0, arraylist_size (completion->list));
    gui_completion_free (completion);

    /* One notify: no list of nicks separated by commas. */
    run_cmd_quiet ("/mute /notify add bob " IRC_FAKE_SERVER);
    completion = gui_completion_new (NULL, ptr_server->buffer);
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_completion_notify_nicks_cb (NULL, NULL, "irc_notify_nicks",
                                                ptr_server->buffer,
                                                completion));
    LONGS_EQUAL(1, arraylist_size (completion->list));
    WEE_CHECK_COMPLETION_WORD("bob", 0);
    gui_completion_free (completion);

    /* Three notify: nicks sorted, then all nicks separated by commas. */
    run_cmd_quiet ("/mute /notify add carol " IRC_FAKE_SERVER);
    run_cmd_quiet ("/mute /notify add alice " IRC_FAKE_SERVER);
    completion = gui_completion_new (NULL, ptr_server->buffer);
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_completion_notify_nicks_cb (NULL, NULL, "irc_notify_nicks",
                                                ptr_server->buffer,
                                                completion));
    LONGS_EQUAL(4, arraylist_size (completion->list));
    WEE_CHECK_COMPLETION_WORD("alice", 0);
    WEE_CHECK_COMPLETION_WORD("bob", 1);
    WEE_CHECK_COMPLETION_WORD("carol", 2);
    WEE_CHECK_COMPLETION_WORD("alice,bob,carol", 3);
    gui_completion_free (completion);

    run_cmd_quiet ("/mute /notify del -all " IRC_FAKE_SERVER);
}
