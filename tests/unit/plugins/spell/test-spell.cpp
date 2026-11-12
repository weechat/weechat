/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test spell functions */

#include "CppUTest/TestHarness.h"
#include "CppUTest/MemoryLeakWarningPlugin.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include "src/core/core-string.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-color.h"
#include "src/gui/gui-nicklist.h"
#include "src/plugins/spell/spell.h"

extern int spell_command_authorized (const char *command);
extern int spell_string_is_url (const char *word);
extern int spell_string_is_nick (struct t_gui_buffer *buffer, char *word);
extern int spell_string_is_simili_number (const char *word);
extern int spell_is_hex_color (const char *string);
extern void spell_skip_color_codes (char **string, char **result);
}

#define WEE_CHECK_SKIP_COLOR_CODES(__skipped, __remaining, __string)    \
    str = strdup (__string);                                            \
    ptr_str = str;                                                      \
    result = string_dyn_alloc (64);                                     \
    spell_skip_color_codes (&ptr_str, result);                          \
    STRCMP_EQUAL(__skipped, *result);                                   \
    STRCMP_EQUAL(__remaining, ptr_str);                                 \
    string_dyn_free (result, 1);                                        \
    free (str);

#define WEE_CHECK_IS_NICK(__result, __buffer, __word)                  \
    snprintf (word, sizeof (word), "%s", __word);                       \
    LONGS_EQUAL(__result, spell_string_is_nick (__buffer, word));       \
    STRCMP_EQUAL(__word, word);

#define TEST_BUFFER_NAME "test"

TEST_GROUP(Spell)
{
    /*
     * Run a command that sets a dictionary option, without memory leak
     * detection: the spell library checks if dictionaries exist, and keeps
     * some memory allocated by this check.
     */

    static void
    run_cmd_dict (const char *command)
    {
        MemoryLeakWarningPlugin::saveAndDisableNewDeleteOverloads();
        run_cmd_quiet (command);
        MemoryLeakWarningPlugin::restoreNewDeleteOverloads();
    }
};

/*
 * Test functions:
 *   spell_build_option_name
 *   spell_get_dict
 */

TEST(Spell, BuildOptionNameGetDict)
{
    struct t_gui_buffer *buffer;
    char *str;

    POINTERS_EQUAL(NULL, spell_build_option_name (NULL));
    POINTERS_EQUAL(NULL, spell_get_dict (NULL));

    buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                             NULL, NULL, NULL,
                             NULL, NULL, NULL);
    CHECK(buffer);

    WEE_TEST_STR("core." TEST_BUFFER_NAME, spell_build_option_name (buffer));

    POINTERS_EQUAL(NULL, spell_get_dict (buffer));

    run_cmd_quiet ("/set spell.check.default_dict \"en\"");
    STRCMP_EQUAL("en", spell_get_dict (buffer));

    run_cmd_dict ("/set spell.dict.core." TEST_BUFFER_NAME " \"en,de\"");
    STRCMP_EQUAL("en,de", spell_get_dict (buffer));

    run_cmd_dict ("/unset spell.dict.core." TEST_BUFFER_NAME);
    run_cmd_quiet ("/unset spell.check.default_dict");

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   spell_get_dict_with_buffer_name
 */

TEST(Spell, GetDictWithBufferName)
{
    POINTERS_EQUAL(NULL, spell_get_dict_with_buffer_name (NULL));
    POINTERS_EQUAL(NULL, spell_get_dict_with_buffer_name (""));
    POINTERS_EQUAL(NULL, spell_get_dict_with_buffer_name ("plugtest"));
    POINTERS_EQUAL(NULL,
                   spell_get_dict_with_buffer_name ("plugtest.srv.#chan"));

    /* default dictionary */
    run_cmd_quiet ("/set spell.check.default_dict \"en\"");
    STRCMP_EQUAL("en", spell_get_dict_with_buffer_name (""));
    STRCMP_EQUAL("en", spell_get_dict_with_buffer_name ("plugtest.srv.#chan"));

    /* dictionary for a plugin */
    run_cmd_dict ("/set spell.dict.plugtest \"de\"");
    STRCMP_EQUAL("en", spell_get_dict_with_buffer_name ("other.srv.#chan"));
    STRCMP_EQUAL("de", spell_get_dict_with_buffer_name ("plugtest"));
    STRCMP_EQUAL("de", spell_get_dict_with_buffer_name ("plugtest.srv"));
    STRCMP_EQUAL("de", spell_get_dict_with_buffer_name ("plugtest.srv.#chan"));

    /* dictionary for a server: more specific than the plugin */
    run_cmd_dict ("/set spell.dict.plugtest.srv \"de,en\"");
    STRCMP_EQUAL("de", spell_get_dict_with_buffer_name ("plugtest"));
    STRCMP_EQUAL("de", spell_get_dict_with_buffer_name ("plugtest.srv2"));
    STRCMP_EQUAL("de,en", spell_get_dict_with_buffer_name ("plugtest.srv"));
    STRCMP_EQUAL("de,en",
                 spell_get_dict_with_buffer_name ("plugtest.srv.#chan"));

    /* name with a dot in the channel */
    STRCMP_EQUAL("de,en",
                 spell_get_dict_with_buffer_name ("plugtest.srv.#a.b.c"));

    run_cmd_dict ("/unset spell.dict.plugtest.srv");
    run_cmd_dict ("/unset spell.dict.plugtest");
    run_cmd_quiet ("/unset spell.check.default_dict");

    POINTERS_EQUAL(NULL,
                   spell_get_dict_with_buffer_name ("plugtest.srv.#chan"));
}

/*
 * Test functions:
 *   spell_command_authorized
 */

TEST(Spell, CommandAuthorized)
{
    /* no command: authorized */
    LONGS_EQUAL(1, spell_command_authorized (NULL));

    /* commands in the default list */
    LONGS_EQUAL(1, spell_command_authorized ("msg"));
    LONGS_EQUAL(1, spell_command_authorized ("me"));
    LONGS_EQUAL(1, spell_command_authorized ("topic"));

    /* commands not in the default list (case sensitive, exact match) */
    LONGS_EQUAL(0, spell_command_authorized (""));
    LONGS_EQUAL(0, spell_command_authorized ("m"));
    LONGS_EQUAL(0, spell_command_authorized ("ms"));
    LONGS_EQUAL(0, spell_command_authorized ("msgx"));
    LONGS_EQUAL(0, spell_command_authorized ("MSG"));
    LONGS_EQUAL(0, spell_command_authorized ("join"));

    /* custom list */
    run_cmd_quiet ("/set spell.check.commands \"join,msg\"");
    LONGS_EQUAL(1, spell_command_authorized ("join"));
    LONGS_EQUAL(1, spell_command_authorized ("msg"));
    LONGS_EQUAL(0, spell_command_authorized ("me"));

    /* empty list */
    run_cmd_quiet ("/set spell.check.commands \"\"");
    LONGS_EQUAL(1, spell_command_authorized (NULL));
    LONGS_EQUAL(0, spell_command_authorized ("msg"));

    run_cmd_quiet ("/unset spell.check.commands");
    LONGS_EQUAL(1, spell_command_authorized ("me"));
    LONGS_EQUAL(0, spell_command_authorized ("join"));
}

/*
 * Test functions:
 *   spell_string_is_url
 */

TEST(Spell, StringIsUrl)
{
    LONGS_EQUAL(0, spell_string_is_url (""));
    LONGS_EQUAL(0, spell_string_is_url ("word"));
    LONGS_EQUAL(0, spell_string_is_url ("http"));
    LONGS_EQUAL(0, spell_string_is_url ("https//example.com"));
    LONGS_EQUAL(0, spell_string_is_url ("www.example.com"));
    LONGS_EQUAL(0, spell_string_is_url ("mailto:user@example.com"));
    LONGS_EQUAL(0, spell_string_is_url (" https://example.com"));

    LONGS_EQUAL(1, spell_string_is_url ("http:"));
    LONGS_EQUAL(1, spell_string_is_url ("http://example.com"));
    LONGS_EQUAL(1, spell_string_is_url ("https://example.com/path?a=b"));
    LONGS_EQUAL(1, spell_string_is_url ("HTTPS://EXAMPLE.COM"));
    LONGS_EQUAL(1, spell_string_is_url ("ftp://example.com"));
    LONGS_EQUAL(1, spell_string_is_url ("ircs://irc.libera.chat:6697"));
    LONGS_EQUAL(1, spell_string_is_url ("irc6s://irc.example.com"));
    LONGS_EQUAL(1, spell_string_is_url ("svn+ssh://example.com/repo"));
    LONGS_EQUAL(1, spell_string_is_url ("git://example.com/repo.git"));
    LONGS_EQUAL(1, spell_string_is_url ("file:///tmp/test"));
}

/*
 * Test functions:
 *   spell_string_is_nick
 */

TEST(Spell, StringIsNick)
{
    struct t_gui_buffer *buffer;
    char word[256];

    buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                             NULL, NULL, NULL,
                             NULL, NULL, NULL);
    CHECK(buffer);

    CHECK(gui_nicklist_add_nick (buffer, NULL, "alice", NULL,
                                 NULL, NULL, 1));
    CHECK(gui_nicklist_add_nick (buffer, NULL, "bob", NULL,
                                 NULL, NULL, 1));

    /* not a nick */
    WEE_CHECK_IS_NICK(0, buffer, "");
    WEE_CHECK_IS_NICK(0, buffer, "carol");
    WEE_CHECK_IS_NICK(0, buffer, "ali");
    WEE_CHECK_IS_NICK(0, buffer, "alicex");
    WEE_CHECK_IS_NICK(0, buffer, "alice:x");
    WEE_CHECK_IS_NICK(0, buffer, "alice,");

    /* nick */
    WEE_CHECK_IS_NICK(1, buffer, "alice");
    WEE_CHECK_IS_NICK(1, buffer, "bob");

    /* nick followed by nick completer and/or space */
    WEE_CHECK_IS_NICK(1, buffer, "alice:");
    WEE_CHECK_IS_NICK(1, buffer, "alice: ");
    WEE_CHECK_IS_NICK(1, buffer, "alice: hello");
    WEE_CHECK_IS_NICK(1, buffer, "alice hello");
    WEE_CHECK_IS_NICK(1, buffer, "bob :x");

    /* self and remote nick are checked only in a private buffer */
    gui_buffer_set (buffer, "localvar_set_nick", "me");
    gui_buffer_set (buffer, "localvar_set_channel", "carol");
    WEE_CHECK_IS_NICK(0, buffer, "me");
    WEE_CHECK_IS_NICK(0, buffer, "carol");

    gui_buffer_set (buffer, "localvar_set_type", "private");
    WEE_CHECK_IS_NICK(1, buffer, "me");
    WEE_CHECK_IS_NICK(1, buffer, "ME");
    WEE_CHECK_IS_NICK(1, buffer, "me:");
    WEE_CHECK_IS_NICK(1, buffer, "carol");
    WEE_CHECK_IS_NICK(1, buffer, "Carol: hi");
    WEE_CHECK_IS_NICK(1, buffer, "alice");
    WEE_CHECK_IS_NICK(0, buffer, "dave");

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   spell_string_is_simili_number
 */

TEST(Spell, StringIsSimiliNumber)
{
    LONGS_EQUAL(0, spell_string_is_simili_number (NULL));
    LONGS_EQUAL(0, spell_string_is_simili_number (""));
    LONGS_EQUAL(0, spell_string_is_simili_number ("a"));
    LONGS_EQUAL(0, spell_string_is_simili_number ("123a"));
    LONGS_EQUAL(0, spell_string_is_simili_number ("a123"));
    LONGS_EQUAL(0, spell_string_is_simili_number ("12 34"));
    LONGS_EQUAL(0, spell_string_is_simili_number ("1é"));

    LONGS_EQUAL(1, spell_string_is_simili_number ("0"));
    LONGS_EQUAL(1, spell_string_is_simili_number ("1234567890"));
    LONGS_EQUAL(1, spell_string_is_simili_number ("3.14"));
    LONGS_EQUAL(1, spell_string_is_simili_number ("-42"));
    LONGS_EQUAL(1, spell_string_is_simili_number ("1,000,000"));
    LONGS_EQUAL(1, spell_string_is_simili_number ("12:30"));
    LONGS_EQUAL(1, spell_string_is_simili_number ("2026-11-04"));
    LONGS_EQUAL(1, spell_string_is_simili_number ("(555)"));
    LONGS_EQUAL(1, spell_string_is_simili_number ("!?"));
    LONGS_EQUAL(1, spell_string_is_simili_number ("50%"));
}

/*
 * Test functions:
 *   spell_skip_color_codes
 */

TEST(Spell, SkipColorCodes)
{
    char *str, *ptr_str, **result, string[256];

    WEE_CHECK_SKIP_COLOR_CODES("", "", "");
    WEE_CHECK_SKIP_COLOR_CODES("", "word", "word");
    WEE_CHECK_SKIP_COLOR_CODES("", ",05word", ",05word");

    /* WeeChat color code */
    snprintf (string, sizeof (string), "%sword", gui_color_get_custom ("red"));
    WEE_CHECK_SKIP_COLOR_CODES(gui_color_get_custom ("red"), "word", string);

    /* IRC attributes */
    WEE_CHECK_SKIP_COLOR_CODES("\x02\x0F\x11\x16\x1D\x1E\x1F", "word",
                               "\x02\x0F\x11\x16\x1D\x1E\x1Fword");

    /* IRC color: reset */
    WEE_CHECK_SKIP_COLOR_CODES("\x03", "word", "\x03word");

    /* IRC color: foreground */
    WEE_CHECK_SKIP_COLOR_CODES("\x03" "5", "word", "\x03" "5word");
    WEE_CHECK_SKIP_COLOR_CODES("\x03" "05", "word", "\x03" "05word");
    WEE_CHECK_SKIP_COLOR_CODES("\x03" "05", "1word", "\x03" "051word");

    /* IRC color: foreground followed by a comma without background */
    WEE_CHECK_SKIP_COLOR_CODES("\x03" "05", ",word", "\x03" "05,word");

    /* IRC color: foreground and background */
    WEE_CHECK_SKIP_COLOR_CODES("\x03" "5,2", "word", "\x03" "5,2word");
    WEE_CHECK_SKIP_COLOR_CODES("\x03" "05,02", "word", "\x03" "05,02word");
    WEE_CHECK_SKIP_COLOR_CODES("\x03" "05,02", "1word", "\x03" "05,021word");

    /* IRC color: background without foreground (reset + text ",02") */
    WEE_CHECK_SKIP_COLOR_CODES("\x03", ",02word", "\x03" ",02word");

    /* IRC hex color: reset */
    WEE_CHECK_SKIP_COLOR_CODES("\x04", "word", "\x04word");

    LONGS_EQUAL(0, spell_is_hex_color (""));
    LONGS_EQUAL(0, spell_is_hex_color ("FF7F0"));
    LONGS_EQUAL(0, spell_is_hex_color ("FF7F0G"));
    LONGS_EQUAL(1, spell_is_hex_color ("ff7f00"));
    LONGS_EQUAL(1, spell_is_hex_color ("FF7F00AB"));

    /* IRC hex color: less than 6 digits (reset + text) */
    WEE_CHECK_SKIP_COLOR_CODES("\x04", "Fword", "\x04" "Fword");
    WEE_CHECK_SKIP_COLOR_CODES("\x04", "facde text", "\x04" "facde text");

    /* IRC hex color: foreground */
    WEE_CHECK_SKIP_COLOR_CODES("\x04" "FF7F00", "word", "\x04" "FF7F00word");
    WEE_CHECK_SKIP_COLOR_CODES("\x04" "FF7F00", "1word",
                               "\x04" "FF7F001word");

    /* IRC hex color: foreground followed by a comma without background */
    WEE_CHECK_SKIP_COLOR_CODES("\x04" "FF7F00", ",word",
                               "\x04" "FF7F00,word");

    /* IRC hex color: foreground and background */
    WEE_CHECK_SKIP_COLOR_CODES("\x04", "F,8word", "\x04" "F,8word");
    WEE_CHECK_SKIP_COLOR_CODES("\x04" "FFFFFF", ",abc word",
                               "\x04" "FFFFFF,abc word");
    WEE_CHECK_SKIP_COLOR_CODES("\x04" "FFFF00,8B008B", "word",
                               "\x04" "FFFF00,8B008Bword");
    WEE_CHECK_SKIP_COLOR_CODES("\x04" "FFFF00,8B008B", "1word",
                               "\x04" "FFFF00,8B008B1word");

    /* IRC hex color: background without foreground (reset + ",8B008B") */
    WEE_CHECK_SKIP_COLOR_CODES("\x04", ",8B008Bword", "\x04" ",8B008Bword");

    /* multiple color codes */
    WEE_CHECK_SKIP_COLOR_CODES("\x02\x03" "05,02\x1F\x03", "word",
                               "\x02\x03" "05,02\x1F\x03word");
    WEE_CHECK_SKIP_COLOR_CODES("\x1E\x04" "FFFF00,8B008B\x03" "05\x04",
                               "word",
                               "\x1E\x04" "FFFF00,8B008B\x03" "05\x04word");
}
