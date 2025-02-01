/*
 * test-irc-color.cpp - test IRC color functions
 *
 * Copyright (C) 2019-2025 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <stdio.h>
#include "src/core/core-config-file.h"
#include "src/core/core-hook.h"
#include "src/core/core-infolist.h"
#include "src/gui/gui-color.h"
#include "src/plugins/irc/irc-color.h"
#include "src/plugins/irc/irc-config.h"

extern int irc_color_convert_rgb2term (long rgb);
extern int irc_color_convert_rgb2irc (long rgb);
extern int irc_color_convert_term2irc (int color);
}

/* tests on irc_color_decode(): IRC color -> WeeChat color */
#define STRING_IRC_BOLD                                                 \
    "test_" IRC_COLOR_BOLD_STR "bold" IRC_COLOR_BOLD_STR "_end"
#define STRING_IRC_RESET                                                \
    "test_" IRC_COLOR_RESET_STR "reset" IRC_COLOR_RESET_STR "_end"
#define STRING_IRC_REVERSE                                              \
    "test_" IRC_COLOR_REVERSE_STR "reverse" IRC_COLOR_REVERSE_STR "_end"
#define STRING_IRC_ITALIC                                               \
    "test_" IRC_COLOR_ITALIC_STR "italic" IRC_COLOR_ITALIC_STR "_end"
#define STRING_IRC_UNDERLINE                                            \
    "test_" IRC_COLOR_UNDERLINE_STR "underline" IRC_COLOR_UNDERLINE_STR "_end"
#define STRING_IRC_COLOR_RESET                                          \
    "test_" IRC_COLOR_COLOR_STR "resetcolor"
#define STRING_IRC_COLOR_FG_LIGHTCYAN                                   \
    "test_" IRC_COLOR_COLOR_STR "11" "lightcyan" IRC_COLOR_COLOR_STR "_end"
#define STRING_IRC_COLOR_FG_LIGHTCYAN_BG_RED                            \
    "test_" IRC_COLOR_COLOR_STR "11,05" "lightcyan/red"                 \
    IRC_COLOR_COLOR_STR "_end"
#define STRING_IRC_ONLY_ATTRS_AND_COLORS                                \
    IRC_COLOR_COLOR_STR                                                 \
    IRC_COLOR_RESET_STR                                                 \
    IRC_COLOR_BOLD_STR                                                  \
    IRC_COLOR_REVERSE_STR                                               \
    IRC_COLOR_ITALIC_STR                                                \
    IRC_COLOR_UNDERLINE_STR                                             \
    IRC_COLOR_UNDERLINE_STR                                             \
    IRC_COLOR_ITALIC_STR                                                \
    IRC_COLOR_REVERSE_STR                                               \
    IRC_COLOR_BOLD_STR
#define STRING_IRC_ATTRS_AND_COLORS                                     \
    "test_"                                                             \
    IRC_COLOR_BOLD_STR IRC_COLOR_UNDERLINE_STR                          \
    IRC_COLOR_COLOR_STR "08,02" "bold_underline_yellow/blue"            \
    IRC_COLOR_BOLD_STR IRC_COLOR_UNDERLINE_STR                          \
    "_normal_yellow/blue"
#define STRING_IRC_COLOR_MIRC_REMAPPED                                  \
    "test_"                                                             \
    IRC_COLOR_COLOR_STR "03,02" "remapped"
#define STRING_IRC_COLOR_FG_ORANGE                                      \
    "test_" IRC_COLOR_HEX_COLOR_STR "FF7F00" "orange"                   \
    IRC_COLOR_HEX_COLOR_STR "_end"
#define STRING_IRC_COLOR_FG_YELLOW_BG_DARKMAGENTA                       \
    "test_" IRC_COLOR_HEX_COLOR_STR "FFFF00,8B008B"                     \
    "yellow/darkmagenta"                                                \
    IRC_COLOR_HEX_COLOR_STR "_end"
#define STRING_IRC_COLOR_TERM_REMAPPED                                  \
    "test_"                                                             \
    IRC_COLOR_HEX_COLOR_STR "FFFF00,8B008B" "remapped"

/* tests on irc_color_encode(): command line -> IRC color */
#define STRING_USER_BOLD                                                \
    "test_" "\x02" "bold" "\x02" "_end"
#define STRING_USER_RESET                                               \
    "test_" "\x0F" "reset" "\x0F" "_end"
#define STRING_USER_REVERSE                                             \
    "test_" "\x16" "reverse" "\x16" "_end"
#define STRING_USER_ITALIC                                              \
    "test_" "\x1D" "italic" "\x1D" "_end"
#define STRING_USER_UNDERLINE                                           \
    "test_" "\x1F" "underline" "\x1F" "_end"
#define STRING_USER_FG_LIGHTCYAN                                        \
    "test_" "\x03" "11" "lightcyan" "\x03" "_end"
#define STRING_USER_FG_LIGHTCYAN_BG_RED                                 \
    "test_" "\x03" "11,05" "lightcyan/red" "\x03" "_end"
#define STRING_USER_ONLY_ATTRS_AND_COLORS                               \
    "\x03" "\x0F" "\x02" "\x16" "\x1D" "\x1F"                           \
    "\x1F" "\x1D" "\x16" "\x02"
#define STRING_USER_ATTRS_AND_COLORS                                    \
    "test_" "\x02" "\x1F" "\x03" "08,02" "bold_underline_yellow/blue"   \
    "\x02" "\x1F" "_normal_yellow/blue"
#define STRING_USER_FG_ORANGE                                           \
    "test_" "\x04" "FF7F00" "orange" "\x04" "_end"
#define STRING_USER_FG_YELLOW_BG_DARKMAGENTA                            \
    "test_" "\x04" "FFFF00,8B008B" "yellow/darkmagenta" "\x04" "_end"

/* tests on irc_color_decode_ansi(): ANSI color -> IRC color */
#define STRING_ANSI_RESET "test_\x1B[mreset"
#define STRING_ANSI_RESET_0 "test_\x1B[0mreset"
#define STRING_ANSI_BOLD                                                \
    "test_\x1B[1mbold1\x1B[2m_normal_"                                  \
    "\x1B[1mbold2\x1B[21m_normal_"                                      \
    "\x1B[1mbold3\x1B[22m_normal"
#define STRING_ANSI_ITALIC                                              \
    "test_\x1B[3mitalic\x1B[23m_normal"
#define STRING_ANSI_UNDERLINE                                           \
    "test_\x1B[4munderline\x1B[24m_normal"
#define STRING_ANSI_FG_BLUE                                             \
    "test_\x1B[34mblue"
#define STRING_ANSI_FG_LIGHTCYAN                                        \
    "test_\x1B[96mlightcyan"
#define STRING_ANSI_FG_BLUE_BG_RED                                      \
    "test_\x1B[34m\x1B[41mblue/red"
#define STRING_ANSI_FG_LIGHTCYAN_BG_LIGHTBLUE                           \
    "test_\x1B[96m\x1B[104mlightcyan/lightblue"
#define STRING_ANSI_FG_RGB_IRC_13                                       \
    "test_\x1B[38;2;255;0;255mcolor13"
#define STRING_ANSI_FG_TERM_IRC_13                                      \
    "test_\x1B[38;5;13mcolor13"
#define STRING_ANSI_FG_RGB_IRC_13_BG_RGB_IRC_02                         \
    "test_\x1B[38;2;255;0;255m\x1B[48;2;0;0;128mcolor13/02"
#define STRING_ANSI_FG_TERM_IRC_13_BG_TERM_IRC_02                       \
    "test_\x1B[38;5;13m\x1B[48;5;4mcolor13/02"
#define STRING_ANSI_DEFAULT_FG                                          \
    "test_\x1B[39mdefault_fg"
#define STRING_ANSI_DEFAULT_BG                                          \
    "test_\x1B[49mdefault_bg"

#define WEE_CHECK_DECODE(__result, __string, __keep_colors)             \
    decoded = irc_color_decode (__string, __keep_colors);               \
    STRCMP_EQUAL(__result, decoded);                                    \
    free (decoded);

#define WEE_CHECK_ENCODE(__result, __string, __keep_colors)             \
    encoded = irc_color_encode (__string, __keep_colors);               \
    STRCMP_EQUAL(__result, encoded);                                    \
    free (encoded);

#define WEE_CHECK_DECODE_ANSI(__result, __string, __keep_colors)        \
    decoded = irc_color_decode_ansi (__string, __keep_colors);          \
    STRCMP_EQUAL(__result, decoded);                                    \
    free (decoded);

TEST_GROUP(IrcColor)
{
};

/*
 * Tests functions:
 *   irc_color_convert_rgb2term
 */

TEST(IrcColor, ConvertRgb2Term)
{
    LONGS_EQUAL(-1, irc_color_convert_rgb2term (-1));
    LONGS_EQUAL(0, irc_color_convert_rgb2term (0));
    LONGS_EQUAL(9, irc_color_convert_rgb2term (0xFF0000));    /* red */
    LONGS_EQUAL(10, irc_color_convert_rgb2term (0x00FF00));   /* green */
    LONGS_EQUAL(12, irc_color_convert_rgb2term (0x0000FF));   /* blue */
    LONGS_EQUAL(11, irc_color_convert_rgb2term (0xFFFF00));   /* yellow */
    LONGS_EQUAL(208, irc_color_convert_rgb2term (0xFF7F00));  /* orange */
    LONGS_EQUAL(90, irc_color_convert_rgb2term (0x8B008B));   /* dark magenta */
}

/*
 * Tests functions:
 *   irc_color_convert_rgb2irc
 */

TEST(IrcColor, ConvertRgb2Irc)
{
    LONGS_EQUAL(1, irc_color_convert_rgb2irc (0x000000));
    LONGS_EQUAL(1, irc_color_convert_rgb2irc (0x010203));
    LONGS_EQUAL(4, irc_color_convert_rgb2irc (0xFF0033));
    LONGS_EQUAL(15, irc_color_convert_rgb2irc (0xAABBCC));
}

/*
 * Tests functions:
 *   irc_color_convert_term2irc
 */

TEST(IrcColor, ConvertTerm2Irc)
{
    LONGS_EQUAL(1, irc_color_convert_term2irc (0));
    LONGS_EQUAL(15, irc_color_convert_term2irc (123));
    LONGS_EQUAL(13, irc_color_convert_term2irc (200));
    LONGS_EQUAL(0, irc_color_convert_term2irc (255));
}

/*
 * Tests functions:
 *   irc_color_decode
 */

TEST(IrcColor, Decode)
{
    char string[1024], *decoded;

    /* NULL/empty string */
    STRCMP_EQUAL(NULL, irc_color_decode (NULL, 0));
    STRCMP_EQUAL(NULL, irc_color_decode (NULL, 1));
    WEE_CHECK_DECODE("", "", 0);
    WEE_CHECK_DECODE("", "", 1);

    /* no color codes */
    WEE_CHECK_DECODE("test string", "test string", 0);
    WEE_CHECK_DECODE("test string", "test string", 1);

    /* bold */
    WEE_CHECK_DECODE("test_bold_end", STRING_IRC_BOLD, 0);
    snprintf (string, sizeof (string),
              "test_%sbold%s_end",
              gui_color_get_custom ("bold"),
              gui_color_get_custom ("-bold"));
    WEE_CHECK_DECODE(string, STRING_IRC_BOLD, 1);

    /* reset */
    WEE_CHECK_DECODE("test_reset_end", STRING_IRC_RESET, 0);
    snprintf (string, sizeof (string),
              "test_%sreset%s_end",
              gui_color_get_custom ("reset"),
              gui_color_get_custom ("reset"));
    WEE_CHECK_DECODE(string, STRING_IRC_RESET, 1);

    /* reverse */
    WEE_CHECK_DECODE("test_reverse_end", STRING_IRC_REVERSE, 0);
    snprintf (string, sizeof (string),
              "test_%sreverse%s_end",
              gui_color_get_custom ("reverse"),
              gui_color_get_custom ("-reverse"));
    WEE_CHECK_DECODE(string, STRING_IRC_REVERSE, 1);

    /* italic */
    WEE_CHECK_DECODE("test_italic_end", STRING_IRC_ITALIC, 0);
    snprintf (string, sizeof (string),
              "test_%sitalic%s_end",
              gui_color_get_custom ("italic"),
              gui_color_get_custom ("-italic"));
    WEE_CHECK_DECODE(string, STRING_IRC_ITALIC, 1);

    /* underline */
    WEE_CHECK_DECODE("test_underline_end", STRING_IRC_UNDERLINE, 0);
    snprintf (string, sizeof (string),
              "test_%sunderline%s_end",
              gui_color_get_custom ("underline"),
              gui_color_get_custom ("-underline"));
    WEE_CHECK_DECODE(string, STRING_IRC_UNDERLINE, 1);

    /* color: no color code (reset) */
    WEE_CHECK_DECODE("test_resetcolor", STRING_IRC_COLOR_RESET, 0);
    snprintf (string, sizeof (string),
              "test_%sresetcolor",
              gui_color_get_custom ("resetcolor"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_RESET, 1);

    /* color: lightcyan */
    WEE_CHECK_DECODE("test_lightcyan_end", STRING_IRC_COLOR_FG_LIGHTCYAN, 0);
    snprintf (string, sizeof (string),
              "test_%slightcyan%s_end",
              gui_color_get_custom ("|lightcyan"),
              gui_color_get_custom ("resetcolor"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_FG_LIGHTCYAN, 1);

    /* color: lightcyan on red */
    WEE_CHECK_DECODE("test_lightcyan/red_end",
                     STRING_IRC_COLOR_FG_LIGHTCYAN_BG_RED, 0);
    snprintf (string, sizeof (string),
              "test_%slightcyan/red%s_end",
              gui_color_get_custom ("|lightcyan,red"),
              gui_color_get_custom ("resetcolor"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_FG_LIGHTCYAN_BG_RED, 1);

    /* color: only attributes and colors */
    WEE_CHECK_DECODE("", STRING_IRC_ONLY_ATTRS_AND_COLORS, 0);
    snprintf (string, sizeof (string),
              "%s%s%s%s%s%s%s%s%s%s%s%s",
              gui_color_get_custom ("resetcolor"),
              gui_color_get_custom ("reset"),
              gui_color_get_custom ("bold"),
              "",  /* fixed */
              gui_color_get_custom ("reverse"),
              gui_color_get_custom ("italic"),
              gui_color_get_custom ("underline"),
              gui_color_get_custom ("-underline"),
              gui_color_get_custom ("-italic"),
              gui_color_get_custom ("-reverse"),
              "",  /* fixed */
              gui_color_get_custom ("-bold"));
    WEE_CHECK_DECODE(string, STRING_IRC_ONLY_ATTRS_AND_COLORS, 1);

    /* color: attributes and colors */
    WEE_CHECK_DECODE("test_bold_underline_yellow/blue_normal_yellow/blue",
                     STRING_IRC_ATTRS_AND_COLORS, 0);
    snprintf (string, sizeof (string),
              "test_%s%s%sbold_underline_yellow/blue%s%s_normal_yellow/blue",
              gui_color_get_custom ("bold"),
              gui_color_get_custom ("underline"),
              gui_color_get_custom ("|yellow,blue"),
              gui_color_get_custom ("-bold"),
              gui_color_get_custom ("-underline"));
    WEE_CHECK_DECODE(string, STRING_IRC_ATTRS_AND_COLORS, 1);

    /* color: 03,02 -> green (remapped via option irc.color.mirc_remap) */
    config_file_option_set (irc_config_color_mirc_remap, "3,2:green", 1);
    WEE_CHECK_DECODE("test_remapped", STRING_IRC_COLOR_MIRC_REMAPPED, 0);
    snprintf (string, sizeof (string),
              "test_%sremapped",
              gui_color_get_custom ("|green"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_MIRC_REMAPPED, 1);
    config_file_option_unset (irc_config_color_mirc_remap);

    /* color: hex 0xFF7F00 (orange / 208) */
    WEE_CHECK_DECODE("test_orange_end",
                     STRING_IRC_COLOR_FG_ORANGE, 0);
    snprintf (string, sizeof (string),
              "test_%sorange%s_end",
              gui_color_get_custom ("|208"),
              gui_color_get_custom ("resetcolor"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_FG_ORANGE, 1);

    /* color: hex 0xFFFF00 (yellow / 11) on 0x8B008B (dark magenta / 90) */
    WEE_CHECK_DECODE("test_yellow/darkmagenta_end",
                     STRING_IRC_COLOR_FG_YELLOW_BG_DARKMAGENTA, 0);
    snprintf (string, sizeof (string),
              "test_%syellow/darkmagenta%s_end",
              gui_color_get_custom ("|11,90"),
              gui_color_get_custom ("resetcolor"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_FG_YELLOW_BG_DARKMAGENTA, 1);

    /*
     * color: hex 0xFFFF00 (yellow / 11) on 0x8B008B (dark magenta / 90)
     * -> blue (remapped via option irc.color.term_remap)
     */
    config_file_option_set (irc_config_color_term_remap, "11,90:blue", 1);
    WEE_CHECK_DECODE("test_remapped", STRING_IRC_COLOR_TERM_REMAPPED, 0);
    snprintf (string, sizeof (string),
              "test_%sremapped",
              gui_color_get_custom ("|blue"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_TERM_REMAPPED, 1);
    config_file_option_unset (irc_config_color_term_remap);
}

/*
 * Tests functions:
 *   irc_color_encode
 */

TEST(IrcColor, Encode)
{
    char string[1024], *encoded;

    /* NULL/empty string */
    STRCMP_EQUAL(NULL, irc_color_encode (NULL, 0));
    STRCMP_EQUAL(NULL, irc_color_encode (NULL, 1));
    WEE_CHECK_ENCODE("", "", 0);
    WEE_CHECK_ENCODE("", "", 1);

    /* no color codes */
    WEE_CHECK_ENCODE("test string", "test string", 0);
    WEE_CHECK_ENCODE("test string", "test string", 1);

    /* bold */
    WEE_CHECK_ENCODE("test_bold_end", STRING_USER_BOLD, 0);
    snprintf (string, sizeof (string),
              "test_%sbold%s_end",
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_BOLD_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_BOLD, 1);

    /* reset */
    WEE_CHECK_ENCODE("test_reset_end", STRING_USER_RESET, 0);
    snprintf (string, sizeof (string),
              "test_%sreset%s_end",
              IRC_COLOR_RESET_STR,
              IRC_COLOR_RESET_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_RESET, 1);

    /* reverse */
    WEE_CHECK_ENCODE("test_reverse_end", STRING_USER_REVERSE, 0);
    snprintf (string, sizeof (string),
              "test_%sreverse%s_end",
              IRC_COLOR_REVERSE_STR,
              IRC_COLOR_REVERSE_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_REVERSE, 1);

    /* italic */
    WEE_CHECK_ENCODE("test_italic_end", STRING_USER_ITALIC, 0);
    snprintf (string, sizeof (string),
              "test_%sitalic%s_end",
              IRC_COLOR_ITALIC_STR,
              IRC_COLOR_ITALIC_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_ITALIC, 1);

    /* underline */
    WEE_CHECK_ENCODE("test_underline_end", STRING_USER_UNDERLINE, 0);
    snprintf (string, sizeof (string),
              "test_%sunderline%s_end",
              IRC_COLOR_UNDERLINE_STR,
              IRC_COLOR_UNDERLINE_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_UNDERLINE, 1);

    /* color: lightcyan */
    WEE_CHECK_ENCODE("test_lightcyan_end", STRING_USER_FG_LIGHTCYAN, 0);
    snprintf (string, sizeof (string),
              "test_%s11lightcyan%s_end",
              IRC_COLOR_COLOR_STR,
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_FG_LIGHTCYAN, 1);

    /* color: lightcyan on red */
    WEE_CHECK_ENCODE("test_lightcyan/red_end",
                     STRING_USER_FG_LIGHTCYAN_BG_RED, 0);
    snprintf (string, sizeof (string),
              "test_%s11,05lightcyan/red%s_end",
              IRC_COLOR_COLOR_STR,
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_FG_LIGHTCYAN_BG_RED, 1);

    /* color: only attributes and colors */
    WEE_CHECK_ENCODE("", STRING_USER_ONLY_ATTRS_AND_COLORS, 0);
    snprintf (string, sizeof (string),
              "%s%s%s%s%s%s%s%s%s%s",
              IRC_COLOR_COLOR_STR,
              IRC_COLOR_RESET_STR,
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_REVERSE_STR,
              IRC_COLOR_ITALIC_STR,
              IRC_COLOR_UNDERLINE_STR,
              IRC_COLOR_UNDERLINE_STR,
              IRC_COLOR_ITALIC_STR,
              IRC_COLOR_REVERSE_STR,
              IRC_COLOR_BOLD_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_ONLY_ATTRS_AND_COLORS, 1);

    /* color: attributes and colors */
    WEE_CHECK_ENCODE("test_bold_underline_yellow/blue_normal_yellow/blue",
                     STRING_USER_ATTRS_AND_COLORS, 0);
    snprintf (string, sizeof (string),
              "test_%s%s%s08,02bold_underline_yellow/blue"
              "%s%s_normal_yellow/blue",
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_UNDERLINE_STR,
              IRC_COLOR_COLOR_STR,
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_UNDERLINE_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_ATTRS_AND_COLORS, 1);

    /* color: hex 0xFF7F00 (orange / 208) */
    WEE_CHECK_ENCODE("test_orange_end", STRING_USER_FG_ORANGE, 0);
    snprintf (string, sizeof (string),
              "test_%sFF7F00orange%s_end",
              IRC_COLOR_HEX_COLOR_STR,
              IRC_COLOR_HEX_COLOR_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_FG_ORANGE, 1);

    /* color: hex 0xFFFF00 (yellow / 11) on 0x8B008B (dark magenta / 90) */
    WEE_CHECK_ENCODE("test_yellow/darkmagenta_end",
                     STRING_USER_FG_YELLOW_BG_DARKMAGENTA, 0);
    snprintf (string, sizeof (string),
              "test_%sFFFF00,8B008Byellow/darkmagenta%s_end",
              IRC_COLOR_HEX_COLOR_STR,
              IRC_COLOR_HEX_COLOR_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_FG_YELLOW_BG_DARKMAGENTA, 1);
}

/*
 * Tests functions:
 *   irc_color_decode_ansi
 */

TEST(IrcColor, DecodeAnsi)
{
    char string[1024], *decoded;

    /* NULL/empty string */
    STRCMP_EQUAL(NULL, irc_color_decode_ansi (NULL, 0));
    STRCMP_EQUAL(NULL, irc_color_decode_ansi (NULL, 1));
    WEE_CHECK_DECODE_ANSI ("", "", 0);
    WEE_CHECK_DECODE_ANSI ("", "", 1);

    /* no color codes */
    WEE_CHECK_DECODE_ANSI("test string", "test string", 0);
    WEE_CHECK_DECODE_ANSI("test string", "test string", 1);

    /* sequences not supported (not ending with "m") */
    WEE_CHECK_DECODE_ANSI("", "\x1B[z", 0);
    WEE_CHECK_DECODE_ANSI("", "\x1B[z", 1);
    WEE_CHECK_DECODE_ANSI("test", "\x1B[ztest", 0);
    WEE_CHECK_DECODE_ANSI("test", "\x1B[ztest", 1);

    /* color: reset (implicit) */
    WEE_CHECK_DECODE_ANSI("test_reset", STRING_ANSI_RESET, 0);
    snprintf (string, sizeof (string),
              "test_%sreset",
              IRC_COLOR_RESET_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_RESET, 1);

    /* color: reset (with "0") */
    WEE_CHECK_DECODE_ANSI("test_reset", STRING_ANSI_RESET_0, 0);
    snprintf (string, sizeof (string),
              "test_%sreset",
              IRC_COLOR_RESET_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_RESET_0, 1);

    /* bold */
    WEE_CHECK_DECODE_ANSI("test_bold1_normal_bold2_normal_bold3_normal",
                          STRING_ANSI_BOLD, 0);
    snprintf (string, sizeof (string),
              "test_%sbold1%s_normal_%sbold2%s_normal_%sbold3%s_normal",
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_BOLD_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_BOLD, 1);

    /* italic */
    WEE_CHECK_DECODE_ANSI("test_italic_normal", STRING_ANSI_ITALIC, 0);
    snprintf (string, sizeof (string),
              "test_%sitalic%s_normal",
              IRC_COLOR_ITALIC_STR,
              IRC_COLOR_ITALIC_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_ITALIC, 1);

    /* underline */
    WEE_CHECK_DECODE_ANSI("test_underline_normal", STRING_ANSI_UNDERLINE, 0);
    snprintf (string, sizeof (string),
              "test_%sunderline%s_normal",
              IRC_COLOR_UNDERLINE_STR,
              IRC_COLOR_UNDERLINE_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_UNDERLINE, 1);

    /* color: blue */
    WEE_CHECK_DECODE_ANSI("test_blue", STRING_ANSI_FG_BLUE, 0);
    snprintf (string, sizeof (string),
              "test_%s02blue",
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_FG_BLUE, 1);

    /* color: lightcyan */
    WEE_CHECK_DECODE_ANSI("test_lightcyan", STRING_ANSI_FG_LIGHTCYAN, 0);
    snprintf (string, sizeof (string),
              "test_%s11lightcyan",
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_FG_LIGHTCYAN, 1);

    /* color: blue on red */
    WEE_CHECK_DECODE_ANSI("test_blue/red", STRING_ANSI_FG_BLUE_BG_RED, 0);
    snprintf (string, sizeof (string),
              "test_%s02%s,05blue/red",
              IRC_COLOR_COLOR_STR,
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_FG_BLUE_BG_RED, 1);

    /* color: lightcyan on lightblue */
    WEE_CHECK_DECODE_ANSI("test_lightcyan/lightblue",
                          STRING_ANSI_FG_LIGHTCYAN_BG_LIGHTBLUE, 0);
    snprintf (string, sizeof (string),
              "test_%s11%s,12lightcyan/lightblue",
              IRC_COLOR_COLOR_STR,
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_FG_LIGHTCYAN_BG_LIGHTBLUE, 1);

    /* color: RGB "FF00FF" (term 13 -> IRC 13 -> lightmagenta) */
    WEE_CHECK_DECODE_ANSI("test_color13", STRING_ANSI_FG_RGB_IRC_13, 0);
    snprintf (string, sizeof (string),
              "test_%s13color13",
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_FG_RGB_IRC_13, 1);

    /* color: term 13 -> IRC 13 -> lightmagenta */
    WEE_CHECK_DECODE_ANSI("test_color13", STRING_ANSI_FG_TERM_IRC_13, 0);
    snprintf (string, sizeof (string),
              "test_%s13color13",
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_FG_TERM_IRC_13, 1);

    /*
     * color: RGB "FF00FF" (term 13 -> IRC 13 -> lightmagenta)
     *        on RGB "000080" (term 04 -> IRC 02 -> blue)
     */
    WEE_CHECK_DECODE_ANSI("test_color13/02",
                          STRING_ANSI_FG_RGB_IRC_13_BG_RGB_IRC_02, 0);
    snprintf (string, sizeof (string),
              "test_%s13%s,02color13/02",
              IRC_COLOR_COLOR_STR,
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string,
                          STRING_ANSI_FG_RGB_IRC_13_BG_RGB_IRC_02, 1);

    /*
     * color: term 13 -> IRC 13 -> lightmagenta
     *        on term 04 -> IRC 02 -> blue
     */
    WEE_CHECK_DECODE_ANSI("test_color13/02",
                          STRING_ANSI_FG_TERM_IRC_13_BG_TERM_IRC_02, 0);
    snprintf (string, sizeof (string),
              "test_%s13%s,02color13/02",
              IRC_COLOR_COLOR_STR,
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string,
                          STRING_ANSI_FG_TERM_IRC_13_BG_TERM_IRC_02, 1);

    /* default text color */
    WEE_CHECK_DECODE_ANSI("test_default_fg", STRING_ANSI_DEFAULT_FG, 0);
    snprintf (string, sizeof (string),
              "test_%s15default_fg",
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_DEFAULT_FG, 1);

    /* default background color */
    WEE_CHECK_DECODE_ANSI("test_default_bg", STRING_ANSI_DEFAULT_BG, 0);
    snprintf (string, sizeof (string),
              "test_%s,01default_bg",
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_DEFAULT_BG, 1);
}

/*
 * Tests functions:
 *   irc_color_for_tags
 */

TEST(IrcColor, ForTags)
{
    STRCMP_EQUAL(NULL, irc_color_for_tags (NULL));

    STRCMP_EQUAL("", irc_color_for_tags (""));
    STRCMP_EQUAL("test", irc_color_for_tags ("test"));
    STRCMP_EQUAL("blue:red", irc_color_for_tags ("blue,red"));
}

/*
 * Tests functions:
 *   irc_color_modifier_cb
 */

TEST(IrcColor, ModifierCallback)
{
    char string[1024], *result;

    /* modifier "irc_color_decode" */
    snprintf (string, sizeof (string),
              "test_%sbold%s_end",
              gui_color_get_custom ("bold"),
              gui_color_get_custom ("-bold"));
    result = hook_modifier_exec (NULL, "irc_color_decode",
                                 "1", STRING_IRC_BOLD);
    STRCMP_EQUAL(string, result);
    free (result);

    /* modifier "irc_color_encode" */
    snprintf (string, sizeof (string),
              "test_%sbold%s_end",
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_BOLD_STR);
    result = hook_modifier_exec (NULL, "irc_color_encode",
                                 "1", STRING_USER_BOLD);
    STRCMP_EQUAL(string, result);
    free (result);

    /* modifier "irc_color_decode_ansi" */
    snprintf (string, sizeof (string),
              "test_%sbold1%s_normal_%sbold2%s_normal_%sbold3%s_normal",
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_BOLD_STR);
    result = hook_modifier_exec (NULL, "irc_color_decode_ansi",
                                 "1", STRING_ANSI_BOLD);
    STRCMP_EQUAL(string, result);
    free (result);
}

/*
 * Tests functions:
 *   irc_color_weechat_add_to_infolist
 */

TEST(IrcColor, WeechatAddToInfolist)
{
    struct t_infolist *infolist;
    struct t_infolist_item *ptr_item;
    int num_items;

    LONGS_EQUAL(0, irc_color_weechat_add_to_infolist (NULL));

    infolist = infolist_new (NULL);
    LONGS_EQUAL(1, irc_color_weechat_add_to_infolist (infolist));
    num_items = 0;
    for (ptr_item = infolist->items; ptr_item; ptr_item = ptr_item->next_item)
    {
        num_items++;
    }
    LONGS_EQUAL(IRC_NUM_COLORS, num_items);
}
