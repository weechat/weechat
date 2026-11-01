/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC bar item functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <string.h>
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/core/core-util.h"
#include "src/gui/gui-buffer.h"
#include "src/plugins/plugin.h"
#include "src/plugins/irc/irc.h"
#include "src/plugins/irc/irc-bar-item.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-server.h"

extern struct t_hashtable *irc_bar_item_focus_buflist (const void *pointer,
                                                       void *data,
                                                       struct t_hashtable *info);
}

TEST_GROUP(IrcBarItem)
{
};

/*
 * Test functions:
 *   irc_bar_item_focus_buflist
 */

TEST(IrcBarItem, FocusBuflist)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_gui_buffer *buffer_chan;
    struct t_hashtable *info;
    char str_pointer[128];

    info = hashtable_new (32,
                          WEECHAT_HASHTABLE_STRING,
                          WEECHAT_HASHTABLE_STRING,
                          NULL, NULL);
    CHECK(info);

    /* No buffer pointer */
    POINTERS_EQUAL(NULL, irc_bar_item_focus_buflist (NULL, NULL, info));

    /* Empty buffer pointer */
    hashtable_set (info, "pointer", "");
    POINTERS_EQUAL(NULL, irc_bar_item_focus_buflist (NULL, NULL, info));

    /* Invalid buffer pointer */
    hashtable_set (info, "pointer", "invalid");
    POINTERS_EQUAL(NULL, irc_bar_item_focus_buflist (NULL, NULL, info));

    /* NULL buffer pointer */
    hashtable_set (info, "pointer", "0x0");
    POINTERS_EQUAL(NULL, irc_bar_item_focus_buflist (NULL, NULL, info));

    /* Buffer not found */
    hashtable_set (info, "pointer", "0x1");
    POINTERS_EQUAL(NULL, irc_bar_item_focus_buflist (NULL, NULL, info));

    /* Core buffer */
    snprintf (str_pointer, sizeof (str_pointer),
              "0x%lx", (unsigned long)gui_buffer_search_main ());
    hashtable_set (info, "pointer", str_pointer);
    POINTERS_EQUAL(NULL, irc_bar_item_focus_buflist (NULL, NULL, info));
    POINTERS_EQUAL(NULL, hashtable_get (info, "irc_server"));
    POINTERS_EQUAL(NULL, hashtable_get (info, "irc_channel"));

    run_cmd_quiet ("/mute /server add test_bar_item fake:127.0.0.1");
    run_cmd_quiet ("/connect test_bar_item");

    server = irc_server_search ("test_bar_item");
    CHECK(server);
    CHECK(server->buffer);

    /* Server buffer */
    snprintf (str_pointer, sizeof (str_pointer),
              "0x%lx", (unsigned long)server->buffer);
    hashtable_set (info, "pointer", str_pointer);
    POINTERS_EQUAL(info, irc_bar_item_focus_buflist (NULL, NULL, info));
    snprintf (str_pointer, sizeof (str_pointer),
              "0x%lx", (unsigned long)server);
    STRCMP_EQUAL(str_pointer, (const char *)hashtable_get (info, "irc_server"));
    POINTERS_EQUAL(NULL, hashtable_get (info, "irc_channel"));

    /* Channel buffer */
    hashtable_remove_all (info);
    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test",
                               0, 0);
    CHECK(channel);
    buffer_chan = channel->buffer;
    CHECK(buffer_chan);
    snprintf (str_pointer, sizeof (str_pointer),
              "0x%lx", (unsigned long)buffer_chan);
    hashtable_set (info, "pointer", str_pointer);
    POINTERS_EQUAL(info, irc_bar_item_focus_buflist (NULL, NULL, info));
    snprintf (str_pointer, sizeof (str_pointer),
              "0x%lx", (unsigned long)server);
    STRCMP_EQUAL(str_pointer, (const char *)hashtable_get (info, "irc_server"));
    snprintf (str_pointer, sizeof (str_pointer),
              "0x%lx", (unsigned long)channel);
    STRCMP_EQUAL(str_pointer, (const char *)hashtable_get (info, "irc_channel"));

    gui_buffer_close (buffer_chan);

    run_cmd_quiet ("/mute /disconnect test_bar_item");
    run_cmd_quiet ("/mute /server del test_bar_item");

    hashtable_free (info);
}

/*
 * Test functions:
 *   irc_bar_item_hook_focus_buflist
 */

TEST(IrcBarItem, HookFocusBuflist)
{
    struct t_hook *ptr_hook;
    char *info;
    int count, count_hooks;

    info = hook_info_get (NULL, "buflist_bar_items_count", NULL);
    if (!info)
        return;
    CHECK(util_parse_int (info, 10, &count));
    CHECK(count > 0);
    free (info);

    /* Check that irc hooked focus on all buflist bar items. */
    count_hooks = 0;
    for (ptr_hook = weechat_hooks[HOOK_TYPE_FOCUS]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && (ptr_hook->plugin == weechat_irc_plugin)
            && (strncmp (HOOK_FOCUS(ptr_hook, area), "buflist", 7) == 0))
        {
            count_hooks++;
        }
    }
    LONGS_EQUAL(count, count_hooks);
}
