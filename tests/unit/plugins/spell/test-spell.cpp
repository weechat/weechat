/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test spell functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include "src/core/core-string.h"
#include "src/gui/gui-color.h"
#include "src/plugins/spell/spell.h"

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

TEST_GROUP(Spell)
{
};

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

    /* IRC hex color: foreground */
    WEE_CHECK_SKIP_COLOR_CODES("\x04" "F", "word", "\x04" "Fword");
    WEE_CHECK_SKIP_COLOR_CODES("\x04" "FF7F00", "word", "\x04" "FF7F00word");
    WEE_CHECK_SKIP_COLOR_CODES("\x04" "FF7F00", "1word",
                               "\x04" "FF7F001word");

    /* IRC hex color: foreground followed by a comma without background */
    WEE_CHECK_SKIP_COLOR_CODES("\x04" "FF7F00", ",word",
                               "\x04" "FF7F00,word");

    /* IRC hex color: foreground and background */
    WEE_CHECK_SKIP_COLOR_CODES("\x04" "F,8", "word", "\x04" "F,8word");
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
