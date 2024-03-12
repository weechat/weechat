/*
 * test-gui-nick.cpp - test nick functions
 *
 * Copyright (C) 2019-2024 Sébastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "src/core/core-config.h"
#include "src/core/core-string.h"
#include "src/gui/gui-color.h"
#include "src/gui/gui-nick.h"

extern void gui_nick_hash_djb2_64 (const char *nickname, uint64_t *color_64);
extern void gui_nick_hash_djb2_32 (const char *nickname, uint32_t *color_32);
extern void gui_nick_hash_sum_64 (const char *nickname, uint64_t *color_64);
extern void gui_nick_hash_sum_32 (const char *nickname, uint32_t *color_32);
extern uint64_t gui_nick_hash_color (const char *nickname, int num_colors);
extern const char *gui_nick_get_forced_color (const char *nickname);
extern char *gui_nick_strdup_for_color (const char *nickname);
}

#define NICK_COLORS "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,"  \
    "21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,"  \
    "44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,"  \
    "67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,"  \
    "90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109," \
    "110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,"   \
    "127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,"   \
    "144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,"   \
    "161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,"   \
    "178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,"   \
    "195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,"   \
    "212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,"   \
    "229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,"   \
    "246,247,248,249,250,251,252,253,254,255"

#define WEE_NICK_STRDUP_FOR_COLOR(__result, __nickname)                 \
    nick = gui_nick_strdup_for_color (__nickname);                      \
    if (__result)                                                       \
    {                                                                   \
        STRCMP_EQUAL(__result, nick);                                   \
    }                                                                   \
    else                                                                \
    {                                                                   \
        POINTERS_EQUAL(NULL, nick);                                     \
    }                                                                   \
    if (nick)                                                           \
        free (nick);

#define WEE_FIND_COLOR(__result, __nickname, __range, __colors)         \
    color = gui_nick_find_color_name (__nickname, __range, __colors);   \
    STRCMP_EQUAL(__result, color);                                      \
    free (color);                                                       \
    result_color = gui_color_get_custom (__result);                     \
    color = gui_nick_find_color (__nickname, __range, __colors);        \
    STRCMP_EQUAL(result_color, color);                                  \
    free (color);


TEST_GROUP(GuiNick)
{
};

/*
 * Tests functions:
 *   gui_nick_hash_djb2_64
 */

TEST(GuiNick, HashDbj264)
{
    uint64_t hash;

    hash = 0;
    gui_nick_hash_djb2_64 (NULL, NULL);
    gui_nick_hash_djb2_64 ("", NULL);
    gui_nick_hash_djb2_64 ("", &hash);
    UNSIGNED_LONGS_EQUAL(UINT64_C(0), hash);

    gui_nick_hash_djb2_64 ("a", &hash);
    UNSIGNED_LONGS_EQUAL(UINT64_C(97), hash);

    hash = 0;
    gui_nick_hash_djb2_64 ("abcdef", &hash);
    UNSIGNED_LONGS_EQUAL(UINT64_C(4013083373), hash);

    hash = 0;
    gui_nick_hash_djb2_64 ("abcdefghijklmnopqrstuvwxyz", &hash);
    UNSIGNED_LONGS_EQUAL(UINT64_C(16315903832110220128), hash);

    hash = 0;
    gui_nick_hash_djb2_64 ("abcdefghijklmnopqrstuvwxyz"
                           "abcdefghijklmnopqrstuvwxyz", &hash);
    UNSIGNED_LONGS_EQUAL(UINT64_C(16109708650384405235), hash);
}

/*
 * Tests functions:
 *   gui_nick_hash_djb2_32
 */

TEST(GuiNick, HashDbj232)
{
    uint32_t hash;

    hash = 0;
    gui_nick_hash_djb2_32 (NULL, NULL);
    gui_nick_hash_djb2_32 ("", NULL);
    gui_nick_hash_djb2_32 ("", &hash);
    UNSIGNED_LONGS_EQUAL(UINT32_C(0), hash);

    gui_nick_hash_djb2_32 ("a", &hash);
    UNSIGNED_LONGS_EQUAL(UINT32_C(97), hash);

    hash = 0;
    gui_nick_hash_djb2_32 ("abcdef", &hash);
    UNSIGNED_LONGS_EQUAL(UINT32_C(4013083373), hash);

    hash = 0;
    gui_nick_hash_djb2_32 ("abcdefghijklmnopqrstuvwxyz", &hash);
    UNSIGNED_LONGS_EQUAL(UINT32_C(3683976572), hash);
}

/*
 * Tests functions:
 *   gui_nick_hash_sum_64
 */

TEST(GuiNick, HashSum64)
{
    uint64_t hash;

    hash = 0;
    gui_nick_hash_sum_64 (NULL, NULL);
    gui_nick_hash_sum_64 ("", NULL);
    gui_nick_hash_sum_64 ("", &hash);
    UNSIGNED_LONGS_EQUAL(UINT64_C(0), hash);

    gui_nick_hash_sum_64 ("a", &hash);
    UNSIGNED_LONGS_EQUAL(UINT64_C(97), hash);

    hash = 0;
    gui_nick_hash_sum_64 ("abcdef", &hash);
    UNSIGNED_LONGS_EQUAL(UINT64_C(597), hash);

    hash = 0;
    gui_nick_hash_sum_64 ("abcdefghijklmnopqrstuvwxyz", &hash);
    UNSIGNED_LONGS_EQUAL(UINT64_C(2847), hash);

    hash = 0;
    gui_nick_hash_sum_64 ("abcdefghijklmnopqrstuvwxyz"
                          "abcdefghijklmnopqrstuvwxyz", &hash);
    UNSIGNED_LONGS_EQUAL(UINT64_C(5694), hash);
}

/*
 * Tests functions:
 *   gui_nick_hash_sum_32
 */

TEST(GuiNick, HashSum32)
{
    uint32_t hash;

    hash = 0;
    gui_nick_hash_sum_32 (NULL, NULL);
    gui_nick_hash_sum_32 ("", NULL);
    gui_nick_hash_sum_32 ("", &hash);
    UNSIGNED_LONGS_EQUAL(UINT32_C(0), hash);

    gui_nick_hash_sum_32 ("a", &hash);
    UNSIGNED_LONGS_EQUAL(UINT32_C(97), hash);

    hash = 0;
    gui_nick_hash_sum_32 ("abcdef", &hash);
    UNSIGNED_LONGS_EQUAL(UINT32_C(597), hash);

    hash = 0;
    gui_nick_hash_sum_32 ("abcdefghijklmnopqrstuvwxyz", &hash);
    UNSIGNED_LONGS_EQUAL(UINT32_C(2847), hash);

    hash = 0;
    gui_nick_hash_sum_32 ("abcdefghijklmnopqrstuvwxyz"
                          "abcdefghijklmnopqrstuvwxyz", &hash);
    UNSIGNED_LONGS_EQUAL(UINT32_C(5694), hash);
}

/*
 * Tests functions:
 *   gui_nick_hash_color
 */

TEST(GuiNick, HashColor)
{
    /* hash without salt */

    /* test hash: djb2 */
    config_file_option_set (config_look_nick_color_hash, "djb2", 1);

    UNSIGNED_LONGS_EQUAL(UINT64_C(0), gui_nick_hash_color (NULL, 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(0), gui_nick_hash_color ("", 256));

    UNSIGNED_LONGS_EQUAL(UINT64_C(0), gui_nick_hash_color ("abcdef", 0));

    UNSIGNED_LONGS_EQUAL(UINT64_C(6006552168338), gui_nick_hash_color ("abcdef", -1));

    UNSIGNED_LONGS_EQUAL(UINT64_C(71), gui_nick_hash_color ("a", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(108), gui_nick_hash_color ("abc", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(146), gui_nick_hash_color ("abcdef", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(73), gui_nick_hash_color ("abcdefghi", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(170), gui_nick_hash_color ("abcdefghijklmnopqrstuvwxyz", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(124), gui_nick_hash_color ("abcdefghijklmnopqrstuvwxyz"
                                                             "abcdefghijklmnopqrstuvwxyz", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(94), gui_nick_hash_color ("zzzzzz", 256));

    /* test hash: sum */
    config_file_option_set (config_look_nick_color_hash, "sum", 1);

    UNSIGNED_LONGS_EQUAL(UINT64_C(0), gui_nick_hash_color (NULL, 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(0), gui_nick_hash_color ("", 256));

    UNSIGNED_LONGS_EQUAL(UINT64_C(97), gui_nick_hash_color ("a", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(38), gui_nick_hash_color ("abc", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(85), gui_nick_hash_color ("abcdef", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(141), gui_nick_hash_color ("abcdefghi", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(31), gui_nick_hash_color ("abcdefghijklmnopqrstuvwxyz", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(62), gui_nick_hash_color ("abcdefghijklmnopqrstuvwxyz"
                                                            "abcdefghijklmnopqrstuvwxyz", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(220), gui_nick_hash_color ("zzzzzz", 256));

    /* test hash: djb2_32 */
    config_file_option_set (config_look_nick_color_hash, "djb2_32", 1);

    UNSIGNED_LONGS_EQUAL(UINT64_C(0), gui_nick_hash_color (NULL, 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(0), gui_nick_hash_color ("", 256));

    UNSIGNED_LONGS_EQUAL(UINT64_C(0), gui_nick_hash_color ("abcdef", 0));

    UNSIGNED_LONGS_EQUAL(UINT64_C(1382582162), gui_nick_hash_color ("abcdef", -1));

    UNSIGNED_LONGS_EQUAL(UINT64_C(71), gui_nick_hash_color ("a", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(108), gui_nick_hash_color ("abc", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(146), gui_nick_hash_color ("abcdef", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(73), gui_nick_hash_color ("abcdefghi", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(209), gui_nick_hash_color ("abcdefghijklmnopqrstuvwxyz", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(116), gui_nick_hash_color ("abcdefghijklmnopqrstuvwxyz"
                                                             "abcdefghijklmnopqrstuvwxyz", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(94), gui_nick_hash_color ("zzzzzz", 256));

    /* test hash: sum_32 */
    config_file_option_set (config_look_nick_color_hash, "sum_32", 1);

    UNSIGNED_LONGS_EQUAL(UINT64_C(0), gui_nick_hash_color (NULL, 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(0), gui_nick_hash_color ("", 256));

    UNSIGNED_LONGS_EQUAL(UINT64_C(0), gui_nick_hash_color ("abcdef", 0));

    UNSIGNED_LONGS_EQUAL(UINT64_C(597), gui_nick_hash_color ("abcdef", -1));

    UNSIGNED_LONGS_EQUAL(UINT64_C(97), gui_nick_hash_color ("a", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(38), gui_nick_hash_color ("abc", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(85), gui_nick_hash_color ("abcdef", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(141), gui_nick_hash_color ("abcdefghi", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(31), gui_nick_hash_color ("abcdefghijklmnopqrstuvwxyz", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(62), gui_nick_hash_color ("abcdefghijklmnopqrstuvwxyz"
                                                            "abcdefghijklmnopqrstuvwxyz", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(220), gui_nick_hash_color ("zzzzzz", 256));

    /* hash with salt */

    config_file_option_set (config_look_nick_color_hash_salt, "abc", 1);

    /* test hash: djb2 */
    config_file_option_set (config_look_nick_color_hash, "djb2", 1);
    UNSIGNED_LONGS_EQUAL(UINT64_C(146), gui_nick_hash_color ("def", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(199603970247853410), gui_nick_hash_color ("abcdef", -1));

    /* test hash: sum */
    config_file_option_set (config_look_nick_color_hash, "sum", 1);
    UNSIGNED_LONGS_EQUAL(UINT64_C(85), gui_nick_hash_color ("def", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(891), gui_nick_hash_color ("abcdef", -1));

    /* test hash: djb2_32 */
    config_file_option_set (config_look_nick_color_hash, "djb2_32", 1);
    UNSIGNED_LONGS_EQUAL(UINT64_C(146), gui_nick_hash_color ("def", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(2988541282), gui_nick_hash_color ("abcdef", -1));

    /* test hash: sum_32 */
    config_file_option_set (config_look_nick_color_hash, "sum_32", 1);
    UNSIGNED_LONGS_EQUAL(UINT64_C(85), gui_nick_hash_color ("def", 256));
    UNSIGNED_LONGS_EQUAL(UINT64_C(891), gui_nick_hash_color ("abcdef", -1));

    config_file_option_reset (config_look_nick_color_hash_salt, 1);
}

/*
 * Tests functions:
 *   gui_nick_get_forced_color
 */

TEST(GuiNick, GetForcedColor)
{
    config_file_option_set (config_look_nick_color_force,
                            "alice:green;bob:cyan", 1);

    POINTERS_EQUAL(NULL, gui_nick_get_forced_color (NULL));
    POINTERS_EQUAL(NULL, gui_nick_get_forced_color (""));

    POINTERS_EQUAL(NULL, gui_nick_get_forced_color ("unknown"));

    STRCMP_EQUAL("green", gui_nick_get_forced_color ("alice"));
    STRCMP_EQUAL("cyan", gui_nick_get_forced_color ("bob"));

    POINTERS_EQUAL(NULL, gui_nick_get_forced_color ("alice2"));
    POINTERS_EQUAL(NULL, gui_nick_get_forced_color ("alice_"));
    POINTERS_EQUAL(NULL, gui_nick_get_forced_color ("bob2"));
    POINTERS_EQUAL(NULL, gui_nick_get_forced_color ("bob_"));

    config_file_option_reset (config_look_nick_color_force, 1);
}

/*
 * Tests functions:
 *   gui_nick_strdup_for_color
 */

TEST(GuiNick, StrdupForColor)
{
    char *nick;

    WEE_NICK_STRDUP_FOR_COLOR(NULL, NULL);
    WEE_NICK_STRDUP_FOR_COLOR("", "");
    WEE_NICK_STRDUP_FOR_COLOR("abcdef", "abcdef");
    WEE_NICK_STRDUP_FOR_COLOR("abcdef", "abcdef_");
    WEE_NICK_STRDUP_FOR_COLOR("abcdef", "abcdef[]");
}

/*
 * Tests functions:
 *   gui_nick_find_color
 *   gui_nick_find_color_name
 */

TEST(GuiNick, FindColor)
{
    const char *result_color;
    char *color;

    WEE_FIND_COLOR("default", NULL, -1, NULL);
    WEE_FIND_COLOR("default", "", -1, NULL);

    WEE_FIND_COLOR("212", "abcdef", -1, NULL);
    WEE_FIND_COLOR("92", "abcdefghi", -1, NULL);

    /* with forced color */
    config_file_option_set (config_look_nick_color_force,
                            "abcdef:green;abcdefghi:125", 1);
    WEE_FIND_COLOR("green", "abcdef", -1, NULL);
    WEE_FIND_COLOR("125", "abcdefghi", -1, NULL);
    config_file_option_reset (config_look_nick_color_force, 1);

    /* with custom colors */
    WEE_FIND_COLOR("214", "abcdef", -1, "red,blue,214,magenta");
    WEE_FIND_COLOR("blue", "abcdefghi", -1, "red,blue,214,magenta");

    /* with forced color and custom colors (forced color is ignored) */
    config_file_option_set (config_look_nick_color_force,
                            "abcdef:green;abcdefghi:125", 1);
    WEE_FIND_COLOR("214", "abcdef", -1, "red,blue,214,magenta");
    WEE_FIND_COLOR("blue", "abcdefghi", -1, "red,blue,214,magenta");
    config_file_option_reset (config_look_nick_color_force, 1);

    /* with case range */
    WEE_FIND_COLOR("176", "ABCDEF]^", -1, NULL);
    WEE_FIND_COLOR("186", "ABCDEF]^", 0, NULL);
    WEE_FIND_COLOR("174", "ABCDEF]^", 30, NULL);
    WEE_FIND_COLOR("148", "ABCDEF]^", 29, NULL);
    WEE_FIND_COLOR("186", "ABCDEF]^", 26, NULL);

    /* with case range and custom colors */
    WEE_FIND_COLOR("214", "ABCDEF]^", -1, "red,blue,214,magenta,yellow");
    WEE_FIND_COLOR("yellow", "ABCDEF]^", 0, "red,blue,214,magenta,yellow");
    WEE_FIND_COLOR("blue", "ABCDEF]^", 30, "red,blue,214,magenta,yellow");
    WEE_FIND_COLOR("magenta", "ABCDEF]^", 29, "red,blue,214,magenta,yellow");
    WEE_FIND_COLOR("yellow", "ABCDEF]^", 26, "red,blue,214,magenta,yellow");
}
