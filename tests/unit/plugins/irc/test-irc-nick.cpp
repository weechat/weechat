/*
 * SPDX-FileCopyrightText: 2019-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC nick functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <string.h>
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-color.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-config.h"
#include "src/plugins/irc/irc-nick.h"
#include "src/plugins/irc/irc-server.h"

extern void irc_nick_set_current_prefix (struct t_irc_nick *nick);
extern void irc_nick_set_prefix (struct t_irc_server *server,
                                 struct t_irc_nick *nick,
                                 int set, char prefix);
extern void irc_nick_set_prefixes (struct t_irc_server *server,
                                   struct t_irc_nick *nick,
                                   const char *prefixes);
}

TEST_GROUP(IrcNick)
{
};

/*
 * Test functions:
 *   irc_nick_valid
 */

TEST(IrcNick, Valid)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_is_nick
 */

TEST(IrcNick, IsNick)
{
    struct t_irc_server *server;

    /* No server, default utf8mapping = none */

    /* Empty nick */
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, NULL));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, ""));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, " "));

    /* Invalid first char (rfc1459) */
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "0abc"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "9abc"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "-abc"));

    /* Invalid first char: prefix char */
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "@abc"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "+abc"));

    /* Invalid first char: chantypes */
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "#abc"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "&abc"));

    /* Invalid chars in nick */
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "nick test"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "nick,test"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "nick?test"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "nick!test"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "nick@test"));

    /* UTF-8 wide chars in nick */
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "noël"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "testé"));
    LONGS_EQUAL(0, irc_nick_is_nick (NULL, "\xf0\xa4\xad\xa2"));  /* Han char */

    /* Valid nicks */
    LONGS_EQUAL(1, irc_nick_is_nick (NULL, "tester"));
    LONGS_EQUAL(1, irc_nick_is_nick (NULL, "bob"));
    LONGS_EQUAL(1, irc_nick_is_nick (NULL, "alice"));
    LONGS_EQUAL(1, irc_nick_is_nick (NULL, "very_long_nick_which_is_valid"));

    /*
     * Server with:
     *   utf8mapping = rfc8265
     *   nicklen = 20
     *   prefix = (qaohv)~&@%+
     *   chantypes = #
     */
    server = irc_server_alloc ("my_ircd");
    CHECK(server);
    if (server->chantypes)
        free (server->chantypes);
    server->utf8mapping = IRC_SERVER_UTF8MAPPING_RFC8265;
    server->nick_max_length = 20;
    irc_server_set_prefix_modes_chars (server, "(qaohv)~&@%+");
    if (server->chantypes)
        free (server->chantypes);
    server->chantypes = strdup ("#");

    /* Empty nick */
    LONGS_EQUAL(0, irc_nick_is_nick (server, NULL));
    LONGS_EQUAL(0, irc_nick_is_nick (server, ""));
    LONGS_EQUAL(0, irc_nick_is_nick (server, " "));

    /* Nick too long */
    LONGS_EQUAL(0, irc_nick_is_nick (server, "long_nick___length_21"));

    /* Valid nicks: first char allowed with utf8mapping = rfc8265 */
    LONGS_EQUAL(1, irc_nick_is_nick (server, "0abc"));
    LONGS_EQUAL(1, irc_nick_is_nick (server, "9abc"));
    LONGS_EQUAL(1, irc_nick_is_nick (server, "-abc"));

    /* Invalid first char: prefix char */
    LONGS_EQUAL(0, irc_nick_is_nick (server, "~abc"));
    LONGS_EQUAL(0, irc_nick_is_nick (server, "&abc"));
    LONGS_EQUAL(0, irc_nick_is_nick (server, "@abc"));
    LONGS_EQUAL(0, irc_nick_is_nick (server, "%abc"));
    LONGS_EQUAL(0, irc_nick_is_nick (server, "+abc"));

    /* Invalid first char: chantypes */
    LONGS_EQUAL(0, irc_nick_is_nick (server, "#abc"));

    /* Invalid chars in nick */
    LONGS_EQUAL(0, irc_nick_is_nick (server, "nick test"));
    LONGS_EQUAL(0, irc_nick_is_nick (server, "nick,test"));
    LONGS_EQUAL(0, irc_nick_is_nick (server, "nick?test"));
    LONGS_EQUAL(0, irc_nick_is_nick (server, "nick!test"));
    LONGS_EQUAL(0, irc_nick_is_nick (server, "nick@test"));

    /* Invalid UTF-8 */
    LONGS_EQUAL(0, irc_nick_is_nick (server, "no\xc3l"));

    /* Valid nicks: UTF-8 */
    LONGS_EQUAL(1, irc_nick_is_nick (server, "noël"));
    LONGS_EQUAL(1, irc_nick_is_nick (server, "testé"));
    LONGS_EQUAL(1, irc_nick_is_nick (server, "\xf0\xa4\xad\xa2")); /* Han char */

    /* Valid nicks with UTF-8 wide chars */
    LONGS_EQUAL(1, irc_nick_is_nick (server, "noël"));
    LONGS_EQUAL(1, irc_nick_is_nick (server, "testé"));

    /* Valid nicks */
    LONGS_EQUAL(1, irc_nick_is_nick (server, "tester"));
    LONGS_EQUAL(1, irc_nick_is_nick (server, "bob"));
    LONGS_EQUAL(1, irc_nick_is_nick (server, "alice"));
    LONGS_EQUAL(1, irc_nick_is_nick (server, "long_nick__length_20"));

    /* Max length: 4 bytes */
    server->nick_max_length = 4;

    /* Invalid nick: 8 bytes */
    LONGS_EQUAL(0, irc_nick_is_nick (server, "\xf0\xa4\xad\xa2\xf0\xa4\xad\xa2"));

    /* Valid nick: 4 bytes */
    LONGS_EQUAL(1, irc_nick_is_nick (server, "\xf0\xa4\xad\xa2")); /* Han char */

    /* Max length: 3 bytes */
    server->nick_max_length = 3;

    /* Invalid nick: 4 bytes */
    LONGS_EQUAL(0, irc_nick_is_nick (server, "\xf0\xa4\xad\xa2")); /* Han char */

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_nick_find_color
 *   irc_nick_find_color_name
 */

TEST(IrcNick, FindColor)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_set_current_prefix
 */

TEST(IrcNick, SetCurrentPrefix)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_irc_nick *nick;

    irc_nick_set_current_prefix (NULL);

    run_cmd_quiet ("/mute /server add test_prefix fake:127.0.0.1");
    run_cmd_quiet ("/connect test_prefix");

    server = irc_server_search ("test_prefix");
    CHECK(server);

    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(channel);

    nick = irc_nick_new (server, channel, "bob", "user@host", NULL, 0, NULL, NULL);
    CHECK(nick);
    STRCMP_EQUAL("  ", nick->prefixes);
    STRCMP_EQUAL(" ", nick->prefix);
    STRCMP_EQUAL(" ", nick->all_prefixes);

    strcpy (nick->prefixes, "@ ");
    irc_nick_set_current_prefix (nick);
    STRCMP_EQUAL("@", nick->prefix);
    STRCMP_EQUAL("@", nick->all_prefixes);

    strcpy (nick->prefixes, " +");
    irc_nick_set_current_prefix (nick);
    STRCMP_EQUAL("+", nick->prefix);
    STRCMP_EQUAL("+", nick->all_prefixes);

    strcpy (nick->prefixes, "@+");
    irc_nick_set_current_prefix (nick);
    STRCMP_EQUAL("@", nick->prefix);
    STRCMP_EQUAL("@+", nick->all_prefixes);

    strcpy (nick->prefixes, "  ");
    irc_nick_set_current_prefix (nick);
    STRCMP_EQUAL(" ", nick->prefix);
    STRCMP_EQUAL(" ", nick->all_prefixes);

    nick->prefixes[0] = '\0';
    irc_nick_set_current_prefix (nick);
    STRCMP_EQUAL(" ", nick->prefix);
    STRCMP_EQUAL(" ", nick->all_prefixes);

    irc_nick_free (server, channel, nick);
    if (channel->buffer)
        gui_buffer_close (channel->buffer);

    run_cmd_quiet ("/mute /disconnect test_prefix");
    run_cmd_quiet ("/mute /server del test_prefix");
}

/*
 * Test functions:
 *   irc_nick_set_prefix
 */

TEST(IrcNick, SetPrefix)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_irc_nick *nick;

    irc_nick_set_prefix (NULL, NULL, 1, '@');

    run_cmd_quiet ("/mute /server add test_prefix fake:127.0.0.1");
    run_cmd_quiet ("/connect test_prefix");

    server = irc_server_search ("test_prefix");
    CHECK(server);

    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(channel);

    nick = irc_nick_new (server, channel, "bob", "user@host", NULL, 0, NULL, NULL);
    CHECK(nick);
    STRCMP_EQUAL("  ", nick->prefixes);
    STRCMP_EQUAL(" ", nick->prefix);
    STRCMP_EQUAL(" ", nick->all_prefixes);

    irc_nick_set_prefix (server, NULL, 1, '@');

    /* Unknown prefix: ignored. */
    irc_nick_set_prefix (server, nick, 1, '~');
    STRCMP_EQUAL("  ", nick->prefixes);
    STRCMP_EQUAL(" ", nick->prefix);
    STRCMP_EQUAL(" ", nick->all_prefixes);

    irc_nick_set_prefix (server, nick, 1, '+');
    STRCMP_EQUAL(" +", nick->prefixes);
    STRCMP_EQUAL("+", nick->prefix);
    STRCMP_EQUAL("+", nick->all_prefixes);

    irc_nick_set_prefix (server, nick, 1, '@');
    STRCMP_EQUAL("@+", nick->prefixes);
    STRCMP_EQUAL("@", nick->prefix);
    STRCMP_EQUAL("@+", nick->all_prefixes);

    irc_nick_set_prefix (server, nick, 0, '@');
    STRCMP_EQUAL(" +", nick->prefixes);
    STRCMP_EQUAL("+", nick->prefix);
    STRCMP_EQUAL("+", nick->all_prefixes);

    irc_nick_set_prefix (server, nick, 0, '+');
    STRCMP_EQUAL("  ", nick->prefixes);
    STRCMP_EQUAL(" ", nick->prefix);
    STRCMP_EQUAL(" ", nick->all_prefixes);

    /* Index of prefix beyond nick prefixes: ignored. */
    nick->prefixes[1] = '\0';
    irc_nick_set_prefix (server, nick, 1, '+');
    STRCMP_EQUAL(" ", nick->prefixes);
    STRCMP_EQUAL(" ", nick->prefix);
    STRCMP_EQUAL(" ", nick->all_prefixes);
    nick->prefixes[0] = '\0';
    irc_nick_set_prefix (server, nick, 1, '@');
    STRCMP_EQUAL("", nick->prefixes);
    STRCMP_EQUAL(" ", nick->prefix);
    STRCMP_EQUAL(" ", nick->all_prefixes);

    irc_nick_free (server, channel, nick);
    if (channel->buffer)
        gui_buffer_close (channel->buffer);

    run_cmd_quiet ("/mute /disconnect test_prefix");
    run_cmd_quiet ("/mute /server del test_prefix");
}

/*
 * Test functions:
 *   irc_nick_sort_prefixes
 */

TEST(IrcNick, SortPrefixes)
{
    struct t_irc_server *server;
    char prefixes[16];

    irc_nick_sort_prefixes (NULL, NULL);

    server = irc_server_alloc ("server1");
    CHECK(server);

    irc_nick_sort_prefixes (server, NULL);

    /* Default prefixes: "@+". */
    strcpy (prefixes, "");
    irc_nick_sort_prefixes (server, prefixes);
    STRCMP_EQUAL("", prefixes);
    strcpy (prefixes, "+");
    irc_nick_sort_prefixes (server, prefixes);
    STRCMP_EQUAL("+", prefixes);
    strcpy (prefixes, "@+");
    irc_nick_sort_prefixes (server, prefixes);
    STRCMP_EQUAL("@+", prefixes);
    strcpy (prefixes, "+@");
    irc_nick_sort_prefixes (server, prefixes);
    STRCMP_EQUAL("@+", prefixes);

    /* Prefixes: "~&@%+". */
    irc_server_set_prefix_modes_chars (server, "(qaohv)~&@%+");
    strcpy (prefixes, "+%@&~");
    irc_nick_sort_prefixes (server, prefixes);
    STRCMP_EQUAL("~&@%+", prefixes);
    strcpy (prefixes, "%~+");
    irc_nick_sort_prefixes (server, prefixes);
    STRCMP_EQUAL("~%+", prefixes);

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_nick_set_prefixes
 */

TEST(IrcNick, SetPrefixes)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_irc_nick *nick;

    irc_nick_set_prefixes (NULL, NULL, "@");

    run_cmd_quiet ("/mute /server add test_prefix fake:127.0.0.1");
    run_cmd_quiet ("/connect test_prefix");

    server = irc_server_search ("test_prefix");
    CHECK(server);

    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(channel);

    nick = irc_nick_new (server, channel, "bob", "user@host", "@+", 0, NULL, NULL);
    CHECK(nick);
    STRCMP_EQUAL("@+", nick->prefixes);
    STRCMP_EQUAL("@", nick->prefix);
    STRCMP_EQUAL("@+", nick->all_prefixes);

    irc_nick_set_prefixes (server, NULL, "@");

    irc_nick_set_prefixes (server, nick, NULL);
    STRCMP_EQUAL("  ", nick->prefixes);
    STRCMP_EQUAL(" ", nick->prefix);
    STRCMP_EQUAL(" ", nick->all_prefixes);

    irc_nick_set_prefixes (server, nick, "+");
    STRCMP_EQUAL(" +", nick->prefixes);
    STRCMP_EQUAL("+", nick->prefix);
    STRCMP_EQUAL("+", nick->all_prefixes);

    irc_nick_set_prefixes (server, nick, "+@~");
    STRCMP_EQUAL("@+", nick->prefixes);
    STRCMP_EQUAL("@", nick->prefix);
    STRCMP_EQUAL("@+", nick->all_prefixes);

    irc_nick_set_prefixes (server, nick, "");
    STRCMP_EQUAL("  ", nick->prefixes);
    STRCMP_EQUAL(" ", nick->prefix);
    STRCMP_EQUAL(" ", nick->all_prefixes);

    /* Index of prefix beyond nick prefixes: ignored. */
    nick->prefixes[1] = '\0';
    irc_nick_set_prefixes (server, nick, "@+");
    STRCMP_EQUAL("@", nick->prefixes);
    STRCMP_EQUAL("@", nick->prefix);
    STRCMP_EQUAL("@", nick->all_prefixes);

    irc_nick_free (server, channel, nick);
    if (channel->buffer)
        gui_buffer_close (channel->buffer);

    run_cmd_quiet ("/mute /disconnect test_prefix");
    run_cmd_quiet ("/mute /server del test_prefix");
}

/*
 * Test functions:
 *   irc_nick_set_host
 */

TEST(IrcNick, SetHost)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_is_op_or_higher
 */

TEST(IrcNick, IsOpOrHigher)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_has_prefix_mode
 */

TEST(IrcNick, HasPrefixMode)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_get_nicklist_group
 */

TEST(IrcNick, GetNicklistGroup)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_display_all_prefixes
 *   irc_nick_get_prefixes_to_display
 */

TEST(IrcNick, GetPrefixesToDisplay)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_irc_nick *nick;
    struct t_config_option *option;

    option = irc_config_look_multi_prefix_in_prompt;

    LONGS_EQUAL(0, irc_nick_display_all_prefixes (NULL, NULL));
    LONGS_EQUAL(0, irc_nick_display_all_prefixes (NULL, option));
    POINTERS_EQUAL(NULL, irc_nick_get_prefixes_to_display (NULL, NULL, NULL));

    run_cmd_quiet ("/mute /server add test_prefix fake:127.0.0.1");
    run_cmd_quiet ("/connect test_prefix");

    server = irc_server_search ("test_prefix");
    CHECK(server);

    POINTERS_EQUAL(NULL, irc_nick_get_prefixes_to_display (server, NULL, option));

    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(channel);

    nick = irc_nick_new (server, channel, "bob", "user@host", "@+", 0, NULL, NULL);
    CHECK(nick);

    /* Capability "multi-prefix" not enabled. */
    LONGS_EQUAL(0, irc_nick_display_all_prefixes (server, option));
    STRCMP_EQUAL("@", irc_nick_get_prefixes_to_display (server, nick, option));

    /* Capability "multi-prefix" enabled. */
    hashtable_set (server->cap_list, "multi-prefix", NULL);
    LONGS_EQUAL(0, irc_nick_display_all_prefixes (server, NULL));
    LONGS_EQUAL(1, irc_nick_display_all_prefixes (server, option));
    STRCMP_EQUAL("@", irc_nick_get_prefixes_to_display (server, nick, NULL));
    STRCMP_EQUAL("@+", irc_nick_get_prefixes_to_display (server, nick, option));

    /* Capability "multi-prefix" enabled, option disabled. */
    config_file_option_set (option, "off", 1);
    LONGS_EQUAL(0, irc_nick_display_all_prefixes (server, option));
    STRCMP_EQUAL("@", irc_nick_get_prefixes_to_display (server, nick, option));
    config_file_option_unset (option);

    /* No prefix. */
    irc_nick_set_prefixes (server, nick, NULL);
    STRCMP_EQUAL(" ", irc_nick_get_prefixes_to_display (server, nick, option));
    hashtable_remove (server->cap_list, "multi-prefix");
    STRCMP_EQUAL(" ", irc_nick_get_prefixes_to_display (server, nick, option));

    irc_nick_free (server, channel, nick);
    if (channel->buffer)
        gui_buffer_close (channel->buffer);

    run_cmd_quiet ("/mute /disconnect test_prefix");
    run_cmd_quiet ("/mute /server del test_prefix");
}

/*
 * Test functions:
 *   irc_nick_get_prefix_for_prompt
 */

TEST(IrcNick, GetPrefixForPrompt)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_irc_nick *nick;
    char *str, *str_decoded;

    POINTERS_EQUAL(NULL, irc_nick_get_prefix_for_prompt (NULL, NULL));

    run_cmd_quiet ("/mute /server add test_prefix fake:127.0.0.1");
    run_cmd_quiet ("/connect test_prefix");

    server = irc_server_search ("test_prefix");
    CHECK(server);

    POINTERS_EQUAL(NULL, irc_nick_get_prefix_for_prompt (server, NULL));

    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(channel);

    nick = irc_nick_new (server, channel, "bob", "user@host", NULL, 0, NULL, NULL);
    CHECK(nick);

    /* No prefix: NULL, or a space with option irc.look.nick_mode_empty. */
    POINTERS_EQUAL(NULL, irc_nick_get_prefix_for_prompt (server, nick));
    config_file_option_set (irc_config_look_nick_mode_empty, "on", 1);
    str = irc_nick_get_prefix_for_prompt (server, nick);
    str_decoded = gui_color_decode (str, NULL);
    STRCMP_EQUAL(" ", str_decoded);
    free (str_decoded);
    free (str);
    config_file_option_unset (irc_config_look_nick_mode_empty);

    /* Capability "multi-prefix" not enabled: only the highest prefix. */
    irc_nick_set_prefixes (server, nick, "@+");
    str = irc_nick_get_prefix_for_prompt (server, nick);
    str_decoded = gui_color_decode (str, NULL);
    STRCMP_EQUAL("@", str_decoded);
    free (str_decoded);
    free (str);

    /* Capability "multi-prefix" enabled: all prefixes. */
    hashtable_set (server->cap_list, "multi-prefix", NULL);
    str = irc_nick_get_prefix_for_prompt (server, nick);
    str_decoded = gui_color_decode (str, NULL);
    STRCMP_EQUAL("@+", str_decoded);
    free (str_decoded);
    free (str);
    hashtable_remove (server->cap_list, "multi-prefix");

    irc_nick_free (server, channel, nick);
    if (channel->buffer)
        gui_buffer_close (channel->buffer);

    run_cmd_quiet ("/mute /disconnect test_prefix");
    run_cmd_quiet ("/mute /server del test_prefix");
}

/*
 * Test functions:
 *   irc_nick_get_prefix_color_name
 */

TEST(IrcNick, GetPrefixColorName)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_get_color_for_nicklist
 */

TEST(IrcNick, GetColorForNicklist)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_nicklist_add
 */

TEST(IrcNick, NicklistAdd)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_nicklist_remove
 */

TEST(IrcNick, NicklistRemove)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_nicklist_set
 */

TEST(IrcNick, NicklistSet)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_nicklist_set_prefix_color_all
 */

TEST(IrcNick, NicklistSetPrefixColorAll)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_nicklist_set_color_all
 */

TEST(IrcNick, NicklistSetColorAll)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_new
 */

TEST(IrcNick, New)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_change
 */

TEST(IrcNick, Change)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_set_mode
 */

TEST(IrcNick, SetMode)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_realloc_prefixes
 */

TEST(IrcNick, ReallocPrefixes)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_irc_nick *nick;

    run_cmd_quiet ("/mute /server add test_prefix fake:127.0.0.1");
    run_cmd_quiet ("/connect test_prefix");

    server = irc_server_search ("test_prefix");
    CHECK(server);

    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(channel);

    nick = irc_nick_new (server, channel, "bob", "user@host", "@+", 0, NULL, NULL);
    CHECK(nick);
    STRCMP_EQUAL("@+", nick->prefixes);
    STRCMP_EQUAL("@", nick->prefix);
    STRCMP_EQUAL("@+", nick->all_prefixes);

    /* Grow prefixes: new prefixes are empty. */
    irc_nick_realloc_prefixes (server, 2, 5);
    STRCMP_EQUAL("@+   ", nick->prefixes);
    STRCMP_EQUAL("@", nick->prefix);
    STRCMP_EQUAL("@+", nick->all_prefixes);
    strcpy (nick->prefixes, "~&@%+");
    irc_nick_set_current_prefix (nick);
    STRCMP_EQUAL("~", nick->prefix);
    STRCMP_EQUAL("~&@%+", nick->all_prefixes);

    /* Shrink prefixes: prefix is updated. */
    irc_nick_realloc_prefixes (server, 5, 2);
    STRCMP_EQUAL("~&", nick->prefixes);
    STRCMP_EQUAL("~", nick->prefix);
    STRCMP_EQUAL("~&", nick->all_prefixes);

    /* No prefixes at all. */
    irc_nick_realloc_prefixes (server, 2, 0);
    STRCMP_EQUAL("", nick->prefixes);
    STRCMP_EQUAL(" ", nick->prefix);
    STRCMP_EQUAL(" ", nick->all_prefixes);

    irc_nick_free (server, channel, nick);
    if (channel->buffer)
        gui_buffer_close (channel->buffer);

    run_cmd_quiet ("/mute /disconnect test_prefix");
    run_cmd_quiet ("/mute /server del test_prefix");
}

/*
 * Test functions:
 *   irc_nick_free
 */

TEST(IrcNick, Free)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_free_all
 */

TEST(IrcNick, FreeAll)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_search
 */

TEST(IrcNick, Search)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_hashtable_by_name
 */

TEST(IrcNick, HashtableByName)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_irc_nick *nick1, *nick2;
    struct t_hashtable *hashtable;

    POINTERS_EQUAL(NULL, irc_nick_hashtable_by_name (NULL));

    run_cmd_quiet ("/mute /server add test_hashtable fake:127.0.0.1");
    run_cmd_quiet ("/connect test_hashtable");

    server = irc_server_search ("test_hashtable");
    CHECK(server);

    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(channel);

    hashtable = irc_nick_hashtable_by_name (channel);
    CHECK(hashtable);
    LONGS_EQUAL(0, hashtable->items_count);
    hashtable_free (hashtable);

    nick1 = irc_nick_new (server, channel, "alice", "user@host", NULL, 0, NULL, NULL);
    CHECK(nick1);
    nick2 = irc_nick_new (server, channel, "Bob", "user@host", "@", 0, NULL, NULL);
    CHECK(nick2);

    hashtable = irc_nick_hashtable_by_name (channel);
    CHECK(hashtable);
    LONGS_EQUAL(2, hashtable->items_count);
    POINTERS_EQUAL(nick1, hashtable_get (hashtable, "alice"));
    POINTERS_EQUAL(nick2, hashtable_get (hashtable, "Bob"));
    POINTERS_EQUAL(NULL, hashtable_get (hashtable, "bob"));
    POINTERS_EQUAL(NULL, hashtable_get (hashtable, "carol"));
    hashtable_free (hashtable);

    irc_nick_free_all (server, channel);
    if (channel->buffer)
        gui_buffer_close (channel->buffer);

    run_cmd_quiet ("/mute /disconnect test_hashtable");
    run_cmd_quiet ("/mute /server del test_hashtable");
}

/*
 * Test functions:
 *   irc_nick_count
 */

TEST(IrcNick, Count)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_set_away
 */

TEST(IrcNick, SetAway)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_mode_for_display
 */

TEST(IrcNick, ModeForDisplay)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_as_prefix
 */

TEST(IrcNick, AsPrefix)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_color_for_msg
 */

TEST(IrcNick, ColorForMsg)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_color_for_pv
 */

TEST(IrcNick, ColorForPv)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_default_ban_mask
 */

TEST(IrcNick, DefaultBanMask)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_hdata_nick_cb
 */

TEST(IrcNick, HdataNickCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_add_to_infolist
 */

TEST(IrcNick, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_nick_print_log
 */

TEST(IrcNick, PrintLog)
{
    /* TODO: write tests */
}
