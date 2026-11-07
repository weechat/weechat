/*
 * SPDX-FileCopyrightText: 2019-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC color functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <string.h>
#include <stdio.h>
#include "src/core/core-config-file.h"
#include "src/core/core-hook.h"
#include "src/core/core-infolist.h"
#include "src/core/core-string.h"
#include "src/gui/gui-color.h"
#include "src/plugins/irc/irc-color.h"
#include "src/plugins/irc/irc-config.h"

extern int irc_color_convert_rgb2term (long rgb);
extern int irc_color_convert_rgb2irc (long rgb);
extern int irc_color_convert_term2irc (int color);
extern int irc_color_ansi_next_param (const char **ptr_param,
                                      const char *ptr_end, int *value);
extern int irc_color_ansi_next_color (const char **ptr_param,
                                      const char *ptr_end);
extern int irc_color_ansi_sequence_size (const char *string);
extern void irc_color_decode_ansi_sequence (char **output,
                                            struct t_irc_color_ansi_state *ansi_state,
                                            const char *params, int length);
}

/* Tests on irc_color_decode(): IRC color -> WeeChat color */
#define STRING_IRC_BOLD                                                 \
    "test_" IRC_COLOR_BOLD_STR "bold" IRC_COLOR_BOLD_STR "_end"
#define STRING_IRC_RESET                                                \
    "test_" IRC_COLOR_RESET_STR "reset" IRC_COLOR_RESET_STR "_end"
#define STRING_IRC_REVERSE                                              \
    "test_" IRC_COLOR_REVERSE_STR "reverse" IRC_COLOR_REVERSE_STR "_end"
#define STRING_IRC_ITALIC                                               \
    "test_" IRC_COLOR_ITALIC_STR "italic" IRC_COLOR_ITALIC_STR "_end"
#define STRING_IRC_STRIKETHROUGH                                        \
    "test_" IRC_COLOR_STRIKETHROUGH_STR "strikethrough" IRC_COLOR_STRIKETHROUGH_STR "_end"
#define STRING_IRC_UNDERLINE                                            \
    "test_" IRC_COLOR_UNDERLINE_STR "underline" IRC_COLOR_UNDERLINE_STR "_end"
#define STRING_IRC_COLOR_RESET                                          \
    "test_" IRC_COLOR_COLOR_STR "resetcolor"
#define STRING_IRC_COLOR_FG_LIGHTCYAN                                   \
    "test_" IRC_COLOR_COLOR_STR "11" "lightcyan" IRC_COLOR_COLOR_STR "_end"
#define STRING_IRC_COLOR_FG_LIGHTCYAN_BG_RED                            \
    "test_" IRC_COLOR_COLOR_STR "11,05" "lightcyan/red"                 \
    IRC_COLOR_COLOR_STR "_end"
#define STRING_IRC_COLOR_BG_WITHOUT_FG                                  \
    "test_" IRC_COLOR_COLOR_STR ",05" "_end"
#define STRING_IRC_COLOR_FG_LIGHTCYAN_COMMA                             \
    "test_" IRC_COLOR_COLOR_STR "11," "lightcyan_end"
#define STRING_IRC_ONLY_ATTRS_AND_COLORS                                \
    IRC_COLOR_COLOR_STR                                                 \
    IRC_COLOR_RESET_STR                                                 \
    IRC_COLOR_BOLD_STR                                                  \
    IRC_COLOR_REVERSE_STR                                               \
    IRC_COLOR_ITALIC_STR                                                \
    IRC_COLOR_STRIKETHROUGH_STR                                         \
    IRC_COLOR_UNDERLINE_STR                                             \
    IRC_COLOR_UNDERLINE_STR                                             \
    IRC_COLOR_STRIKETHROUGH_STR                                         \
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
#define STRING_IRC_COLOR_MIRC_REMAPPED_CURRENT_BG                       \
    "test_"                                                             \
    IRC_COLOR_COLOR_STR "01,95" "black/gray_"                           \
    IRC_COLOR_COLOR_STR "01" "black_"                                   \
    IRC_COLOR_COLOR_STR IRC_COLOR_COLOR_STR "01" "remapped1_"           \
    IRC_COLOR_COLOR_STR "01,05" "black/red_"                            \
    IRC_COLOR_RESET_STR IRC_COLOR_COLOR_STR "01" "remapped2_"           \
    IRC_COLOR_COLOR_STR "01,99" "black/default_"                        \
    IRC_COLOR_COLOR_STR "01" "remapped3_"                               \
    IRC_COLOR_HEX_COLOR_STR "FFFF00,8B008B" "yellow/darkmagenta_"       \
    IRC_COLOR_COLOR_STR "01" "black"
#define STRING_IRC_COLOR_FG_ORANGE                                      \
    "test_" IRC_COLOR_HEX_COLOR_STR "FF7F00" "orange"                   \
    IRC_COLOR_HEX_COLOR_STR "_end"
#define STRING_IRC_COLOR_FG_YELLOW_BG_DARKMAGENTA                       \
    "test_" IRC_COLOR_HEX_COLOR_STR "FFFF00,8B008B"                     \
    "yellow/darkmagenta"                                                \
    IRC_COLOR_HEX_COLOR_STR "_end"
#define STRING_IRC_COLOR_HEX_BG_WITHOUT_FG                              \
    "test_" IRC_COLOR_HEX_COLOR_STR ",8B008B" "_end"
#define STRING_IRC_COLOR_FG_ORANGE_COMMA                                \
    "test_" IRC_COLOR_HEX_COLOR_STR "FF7F00," "orange_end"
#define STRING_IRC_COLOR_HEX_SHORT                                      \
    "test_" IRC_COLOR_HEX_COLOR_STR "12" " apples"
#define STRING_IRC_COLOR_HEX_SHORT_BG                                   \
    "test_" IRC_COLOR_HEX_COLOR_STR "FF7F00,8B00" " orange"
#define STRING_IRC_COLOR_HEX_LONG                                       \
    "test_" IRC_COLOR_HEX_COLOR_STR "FF7F00" "ABCD" " orange"
#define STRING_IRC_COLOR_TERM_REMAPPED                                  \
    "test_"                                                             \
    IRC_COLOR_HEX_COLOR_STR "FFFF00,8B008B" "remapped"
#define STRING_IRC_COLOR_TERM_REMAPPED_CURRENT_BG                       \
    "test_"                                                             \
    IRC_COLOR_HEX_COLOR_STR "000000,8B008B" "black/darkmagenta_"        \
    IRC_COLOR_HEX_COLOR_STR "000000" "black_"                           \
    IRC_COLOR_HEX_COLOR_STR IRC_COLOR_HEX_COLOR_STR "000000"            \
    "remapped1_"                                                        \
    IRC_COLOR_HEX_COLOR_STR "000000,8B008B" "black/darkmagenta_"        \
    IRC_COLOR_RESET_STR IRC_COLOR_HEX_COLOR_STR "000000" "remapped2_"   \
    IRC_COLOR_COLOR_STR "01,05" "black/red_"                            \
    IRC_COLOR_HEX_COLOR_STR "000000" "black"

/* Tests on irc_color_encode(): command line -> IRC color */
#define STRING_USER_BOLD                                                \
    "test_" "\x02" "bold" "\x02" "_end"
#define STRING_USER_RESET                                               \
    "test_" "\x0F" "reset" "\x0F" "_end"
#define STRING_USER_REVERSE                                             \
    "test_" "\x16" "reverse" "\x16" "_end"
#define STRING_USER_ITALIC                                              \
    "test_" "\x1D" "italic" "\x1D" "_end"
#define STRING_USER_STRIKETHROUGH                                       \
    "test_" "\x1E" "strikethrough" "\x1E" "_end"
#define STRING_USER_UNDERLINE                                           \
    "test_" "\x1F" "underline" "\x1F" "_end"
#define STRING_USER_FG_LIGHTCYAN                                        \
    "test_" "\x03" "11" "lightcyan" "\x03" "_end"
#define STRING_USER_FG_LIGHTCYAN_BG_RED                                 \
    "test_" "\x03" "11,05" "lightcyan/red" "\x03" "_end"
#define STRING_USER_BG_WITHOUT_FG                                       \
    "test_" "\x03" ",05" "_end"
#define STRING_USER_FG_LIGHTCYAN_COMMA                                  \
    "test_" "\x03" "11," "lightcyan_end"
#define STRING_USER_ONLY_ATTRS_AND_COLORS                               \
    "\x03" "\x0F" "\x02" "\x16" "\x1D" "\x1E" "\x1F" "\x1F" "\x1E"      \
    "\x1D" "\x16" "\x02"
#define STRING_USER_ATTRS_AND_COLORS                                    \
    "test_" "\x02" "\x1F" "\x03" "08,02" "bold_underline_yellow/blue"   \
    "\x02" "\x1F" "_normal_yellow/blue"
#define STRING_USER_FG_ORANGE                                           \
    "test_" "\x04" "FF7F00" "orange" "\x04" "_end"
#define STRING_USER_FG_YELLOW_BG_DARKMAGENTA                            \
    "test_" "\x04" "FFFF00,8B008B" "yellow/darkmagenta" "\x04" "_end"
#define STRING_USER_HEX_BG_WITHOUT_FG                                   \
    "test_" "\x04" ",8B008B" "_end"
#define STRING_USER_FG_ORANGE_COMMA                                     \
    "test_" "\x04" "FF7F00," "orange_end"
#define STRING_USER_HEX_SHORT                                           \
    "test_" "\x04" "12" " apples"
#define STRING_USER_HEX_SHORT_BG                                        \
    "test_" "\x04" "FF7F00,8B00" " orange"

/* Tests on irc_color_decode_ansi(): ANSI color -> IRC color */
#define STRING_ANSI_RESET "test_\x1B[mreset"
#define STRING_ANSI_RESET_0 "test_\x1B[0mreset"
#define STRING_ANSI_BOLD                                                \
    "test_\x1B[1mbold1\x1B[2m_normal_"                                  \
    "\x1B[1mbold2\x1B[21m_normal_"                                      \
    "\x1B[1mbold3\x1B[22m_normal"
#define STRING_ANSI_ITALIC                                              \
    "test_\x1B[3mitalic\x1B[23m_normal"
#define STRING_ANSI_STRIKETHROUGH                                       \
    "test_\x1B[9mstrikethrough\x1B[29m_normal"
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
#define STRING_ANSI_EMPTY_PARAM_ONLY                                    \
    "A\x1B[1mX\x1B[;mB"
#define STRING_ANSI_EMPTY_PARAM_FIRST                                   \
    "A\x1B[;1mB"
#define STRING_ANSI_EMPTY_PARAM_MIDDLE                                  \
    "A\x1B[1;;4mB"
#define STRING_ANSI_EMPTY_PARAM_LAST                                    \
    "A\x1B[1;mB"
#define STRING_ANSI_FG_TERM_EMPTY                                       \
    "test_\x1B[38;5;mcolor01"
#define STRING_ANSI_BG_TERM_EMPTY                                       \
    "test_\x1B[48;5;;1mcolor01"
#define STRING_ANSI_FG_RGB_EMPTY                                        \
    "test_\x1B[38;2;255;;255mcolor13"
#define STRING_ANSI_BG_RGB_EMPTY                                        \
    "test_\x1B[48;2;255;;255;4mcolor13"
#define STRING_ANSI_UNDERLINE_COLOR_TERM                                \
    "test_\x1B[58;5;1mul_1"
#define STRING_ANSI_UNDERLINE_COLOR_RGB                                 \
    "test_\x1B[58;2;255;0;0mul_red"
#define STRING_ANSI_UNDERLINE_COLOR_DEFAULT                             \
    "test_\x1B[59mul_default"
#define STRING_ANSI_UNDERLINE_FG_RGB_UNDERLINE_COLOR_RGB                \
    "test_\x1B[4m\x1B[38;2;0;128;128m\x1B[58;2;255;0;0mul_color10"
#define STRING_ANSI_UNDERLINE_COLOR_MIXED                               \
    "test_\x1B[4;58;5;1;34;59;1mul_blue"
#define STRING_ANSI_SUB_PARAMS                                          \
    "test_\x1B[1;4:3;38:2::255:0:0;34mbold_blue"
#define STRING_ANSI_FG_RGB_COMPONENT_256                                \
    "test_\x1B[38;2;0;256;0mcolor09"
#define STRING_ANSI_FG_RGB_COMPONENT_40000                              \
    "test_\x1B[38;2;40000;0;0mcolor04"
#define STRING_ANSI_BG_RGB_COMPONENTS_TOO_BIG                           \
    "test_\x1B[48;2;0;256;99999999999mcolor11"

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
 * Test functions:
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
 * Test functions:
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
 * Test functions:
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
 * Test functions:
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

    /* Byte which is not valid UTF-8 before a color code */
    WEE_CHECK_DECODE("A\xC3" "red", "A\xC3\x03" "04red", 0);

    /* No color codes */
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

    /* strikethrough */
    WEE_CHECK_DECODE("test_strikethrough_end", STRING_IRC_STRIKETHROUGH, 0);
    snprintf (string, sizeof (string),
              "test_%sstrikethrough%s_end",
              gui_color_get_custom ("dim"),
              gui_color_get_custom ("-dim"));
    WEE_CHECK_DECODE(string, STRING_IRC_STRIKETHROUGH, 1);

    /* underline */
    WEE_CHECK_DECODE("test_underline_end", STRING_IRC_UNDERLINE, 0);
    snprintf (string, sizeof (string),
              "test_%sunderline%s_end",
              gui_color_get_custom ("underline"),
              gui_color_get_custom ("-underline"));
    WEE_CHECK_DECODE(string, STRING_IRC_UNDERLINE, 1);

    /* Color: no color code (reset) */
    WEE_CHECK_DECODE("test_resetcolor", STRING_IRC_COLOR_RESET, 0);
    snprintf (string, sizeof (string),
              "test_%sresetcolor",
              gui_color_get_custom ("resetcolor"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_RESET, 1);

    /* Color: lightcyan */
    WEE_CHECK_DECODE("test_lightcyan_end", STRING_IRC_COLOR_FG_LIGHTCYAN, 0);
    snprintf (string, sizeof (string),
              "test_%slightcyan%s_end",
              gui_color_get_custom ("|lightcyan"),
              gui_color_get_custom ("resetcolor"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_FG_LIGHTCYAN, 1);

    /* Color: lightcyan on red */
    WEE_CHECK_DECODE("test_lightcyan/red_end",
                     STRING_IRC_COLOR_FG_LIGHTCYAN_BG_RED, 0);
    snprintf (string, sizeof (string),
              "test_%slightcyan/red%s_end",
              gui_color_get_custom ("|lightcyan,red"),
              gui_color_get_custom ("resetcolor"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_FG_LIGHTCYAN_BG_RED, 1);

    /* Color: background without foreground (color reset + text ",05") */
    WEE_CHECK_DECODE("test_,05_end", STRING_IRC_COLOR_BG_WITHOUT_FG, 0);
    snprintf (string, sizeof (string),
              "test_%s,05_end",
              gui_color_get_custom ("resetcolor"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_BG_WITHOUT_FG, 1);

    /* Color: lightcyan followed by a comma without background */
    WEE_CHECK_DECODE("test_,lightcyan_end",
                     STRING_IRC_COLOR_FG_LIGHTCYAN_COMMA, 0);
    snprintf (string, sizeof (string),
              "test_%s,lightcyan_end",
              gui_color_get_custom ("|lightcyan"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_FG_LIGHTCYAN_COMMA, 1);

    /* Color: only attributes and colors */
    WEE_CHECK_DECODE("", STRING_IRC_ONLY_ATTRS_AND_COLORS, 0);
    snprintf (string, sizeof (string),
              "%s%s%s%s%s%s%s%s%s%s%s%s",
              gui_color_get_custom ("resetcolor"),
              gui_color_get_custom ("reset"),
              gui_color_get_custom ("bold"),
              gui_color_get_custom ("reverse"),
              gui_color_get_custom ("italic"),
              gui_color_get_custom ("dim"),
              gui_color_get_custom ("underline"),
              gui_color_get_custom ("-underline"),
              gui_color_get_custom ("-dim"),
              gui_color_get_custom ("-italic"),
              gui_color_get_custom ("-reverse"),
              gui_color_get_custom ("-bold"));
    WEE_CHECK_DECODE(string, STRING_IRC_ONLY_ATTRS_AND_COLORS, 1);

    /* Color: attributes and colors */
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

    /* Color: 03,02 -> green (remapped via option irc.color.mirc_remap) */
    config_file_option_set (irc_config_color_mirc_remap, "3,2:green", 1);
    WEE_CHECK_DECODE("test_remapped", STRING_IRC_COLOR_MIRC_REMAPPED, 0);
    snprintf (string, sizeof (string),
              "test_%sremapped",
              gui_color_get_custom ("|green"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_MIRC_REMAPPED, 1);
    config_file_option_unset (irc_config_color_mirc_remap);

    /*
     * Color: 01 -> darkgray (remapped via option irc.color.mirc_remap)
     * only if the current background is the default one.
     */
    config_file_option_set (irc_config_color_mirc_remap, "1,-1:darkgray", 1);
    WEE_CHECK_DECODE("test_black/gray_black_remapped1_black/red_remapped2_"
                     "black/default_remapped3_yellow/darkmagenta_black",
                     STRING_IRC_COLOR_MIRC_REMAPPED_CURRENT_BG, 0);
    snprintf (string, sizeof (string),
              "test_%sblack/gray_%sblack_%s%sremapped1_%sblack/red_"
              "%s%sremapped2_%sblack/default_%sremapped3_"
              "%syellow/darkmagenta_%sblack",
              gui_color_get_custom ("|black,247"),
              gui_color_get_custom ("|black"),
              gui_color_get_custom ("resetcolor"),
              gui_color_get_custom ("|darkgray"),
              gui_color_get_custom ("|black,red"),
              gui_color_get_custom ("reset"),
              gui_color_get_custom ("|darkgray"),
              gui_color_get_custom ("|black,default"),
              gui_color_get_custom ("|darkgray"),
              gui_color_get_custom ("|#ffff00,#8b008b"),
              gui_color_get_custom ("|black"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_MIRC_REMAPPED_CURRENT_BG, 1);
    config_file_option_unset (irc_config_color_mirc_remap);

    /* Color: hex 0xFF7F00 (orange, RGB kept) */
    WEE_CHECK_DECODE("test_orange_end",
                     STRING_IRC_COLOR_FG_ORANGE, 0);
    snprintf (string, sizeof (string),
              "test_%sorange%s_end",
              gui_color_get_custom ("|#ff7f00"),
              gui_color_get_custom ("resetcolor"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_FG_ORANGE, 1);

    /* Color: hex 0xFFFF00 (yellow) on 0x8B008B (dark magenta), RGB kept */
    WEE_CHECK_DECODE("test_yellow/darkmagenta_end",
                     STRING_IRC_COLOR_FG_YELLOW_BG_DARKMAGENTA, 0);
    snprintf (string, sizeof (string),
              "test_%syellow/darkmagenta%s_end",
              gui_color_get_custom ("|#ffff00,#8b008b"),
              gui_color_get_custom ("resetcolor"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_FG_YELLOW_BG_DARKMAGENTA, 1);

    /* Color: hex background without foreground (reset + text ",8B008B") */
    WEE_CHECK_DECODE("test_,8B008B_end",
                     STRING_IRC_COLOR_HEX_BG_WITHOUT_FG, 0);
    snprintf (string, sizeof (string),
              "test_%s,8B008B_end",
              gui_color_get_custom ("resetcolor"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_HEX_BG_WITHOUT_FG, 1);

    /* Color: hex 0xFF7F00 (orange, RGB kept) followed by a comma */
    WEE_CHECK_DECODE("test_,orange_end",
                     STRING_IRC_COLOR_FG_ORANGE_COMMA, 0);
    snprintf (string, sizeof (string),
              "test_%s,orange_end",
              gui_color_get_custom ("|#ff7f00"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_FG_ORANGE_COMMA, 1);

    /* Color: hex with less than 6 digits (reset + text "12 apples") */
    WEE_CHECK_DECODE("test_12 apples", STRING_IRC_COLOR_HEX_SHORT, 0);
    snprintf (string, sizeof (string),
              "test_%s12 apples",
              gui_color_get_custom ("resetcolor"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_HEX_SHORT, 1);

    /* Color: hex background with less than 6 digits (text ",8B00") */
    WEE_CHECK_DECODE("test_,8B00 orange", STRING_IRC_COLOR_HEX_SHORT_BG, 0);
    snprintf (string, sizeof (string),
              "test_%s,8B00 orange",
              gui_color_get_custom ("|#ff7f00"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_HEX_SHORT_BG, 1);

    /* Color: hex followed by more hexadecimal digits (text "ABCD") */
    WEE_CHECK_DECODE("test_ABCD orange", STRING_IRC_COLOR_HEX_LONG, 0);
    snprintf (string, sizeof (string),
              "test_%sABCD orange",
              gui_color_get_custom ("|#ff7f00"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_HEX_LONG, 1);

    /*
     * Color: hex 0xFFFF00 (yellow / 11) on 0x8B008B (dark magenta / 90)
     * -> blue (remapped via option irc.color.term_remap).
     */
    config_file_option_set (irc_config_color_term_remap, "11,90:blue", 1);
    WEE_CHECK_DECODE("test_remapped", STRING_IRC_COLOR_TERM_REMAPPED, 0);
    snprintf (string, sizeof (string),
              "test_%sremapped",
              gui_color_get_custom ("|blue"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_TERM_REMAPPED, 1);
    config_file_option_unset (irc_config_color_term_remap);

    /*
     * Color: hex 0x000000 (black / 0) -> darkgray (remapped via option
     * irc.color.term_remap) only if the current background is the default
     * one.
     */
    config_file_option_set (irc_config_color_term_remap, "0,-1:darkgray", 1);
    WEE_CHECK_DECODE("test_black/darkmagenta_black_remapped1_"
                     "black/darkmagenta_remapped2_black/red_black",
                     STRING_IRC_COLOR_TERM_REMAPPED_CURRENT_BG, 0);
    snprintf (string, sizeof (string),
              "test_%sblack/darkmagenta_%sblack_%s%sremapped1_"
              "%sblack/darkmagenta_%s%sremapped2_%sblack/red_%sblack",
              gui_color_get_custom ("|#000000,#8b008b"),
              gui_color_get_custom ("|#000000"),
              gui_color_get_custom ("resetcolor"),
              gui_color_get_custom ("|darkgray"),
              gui_color_get_custom ("|#000000,#8b008b"),
              gui_color_get_custom ("reset"),
              gui_color_get_custom ("|darkgray"),
              gui_color_get_custom ("|black,red"),
              gui_color_get_custom ("|#000000"));
    WEE_CHECK_DECODE(string, STRING_IRC_COLOR_TERM_REMAPPED_CURRENT_BG, 1);
    config_file_option_unset (irc_config_color_term_remap);
}

/*
 * Test functions:
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

    /* No color codes */
    WEE_CHECK_ENCODE("test string", "test string", 0);
    WEE_CHECK_ENCODE("test string", "test string", 1);

    /* Byte which is not valid UTF-8 before a color code */
    WEE_CHECK_ENCODE("A\xC3" "red", "A\xC3\x03" "04red", 0);
    WEE_CHECK_ENCODE("A\xC3\x03" "04red", "A\xC3\x03" "04red", 1);

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

    /* strikethrough */
    WEE_CHECK_ENCODE("test_strikethrough_end", STRING_USER_STRIKETHROUGH, 0);
    snprintf (string, sizeof (string),
              "test_%sstrikethrough%s_end",
              IRC_COLOR_STRIKETHROUGH_STR,
              IRC_COLOR_STRIKETHROUGH_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_STRIKETHROUGH, 1);

    /* underline */
    WEE_CHECK_ENCODE("test_underline_end", STRING_USER_UNDERLINE, 0);
    snprintf (string, sizeof (string),
              "test_%sunderline%s_end",
              IRC_COLOR_UNDERLINE_STR,
              IRC_COLOR_UNDERLINE_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_UNDERLINE, 1);

    /* Color: lightcyan */
    WEE_CHECK_ENCODE("test_lightcyan_end", STRING_USER_FG_LIGHTCYAN, 0);
    snprintf (string, sizeof (string),
              "test_%s11lightcyan%s_end",
              IRC_COLOR_COLOR_STR,
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_FG_LIGHTCYAN, 1);

    /* Color: lightcyan on red */
    WEE_CHECK_ENCODE("test_lightcyan/red_end",
                     STRING_USER_FG_LIGHTCYAN_BG_RED, 0);
    snprintf (string, sizeof (string),
              "test_%s11,05lightcyan/red%s_end",
              IRC_COLOR_COLOR_STR,
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_FG_LIGHTCYAN_BG_RED, 1);

    /* Color: background without foreground (color reset + text ",05") */
    WEE_CHECK_ENCODE("test_,05_end", STRING_USER_BG_WITHOUT_FG, 0);
    snprintf (string, sizeof (string),
              "test_%s,05_end",
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_BG_WITHOUT_FG, 1);

    /* Color: lightcyan followed by a comma without background */
    WEE_CHECK_ENCODE("test_,lightcyan_end",
                     STRING_USER_FG_LIGHTCYAN_COMMA, 0);
    snprintf (string, sizeof (string),
              "test_%s11,lightcyan_end",
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_FG_LIGHTCYAN_COMMA, 1);

    /* Color: only attributes and colors */
    WEE_CHECK_ENCODE("", STRING_USER_ONLY_ATTRS_AND_COLORS, 0);
    snprintf (string, sizeof (string),
              "%s%s%s%s%s%s%s%s%s%s%s%s",
              IRC_COLOR_COLOR_STR,
              IRC_COLOR_RESET_STR,
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_REVERSE_STR,
              IRC_COLOR_ITALIC_STR,
              IRC_COLOR_STRIKETHROUGH_STR,
              IRC_COLOR_UNDERLINE_STR,
              IRC_COLOR_UNDERLINE_STR,
              IRC_COLOR_STRIKETHROUGH_STR,
              IRC_COLOR_ITALIC_STR,
              IRC_COLOR_REVERSE_STR,
              IRC_COLOR_BOLD_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_ONLY_ATTRS_AND_COLORS, 1);

    /* Color: attributes and colors */
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

    /* Color: hex 0xFF7F00 (orange, RGB kept) */
    WEE_CHECK_ENCODE("test_orange_end", STRING_USER_FG_ORANGE, 0);
    snprintf (string, sizeof (string),
              "test_%sFF7F00orange%s_end",
              IRC_COLOR_HEX_COLOR_STR,
              IRC_COLOR_HEX_COLOR_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_FG_ORANGE, 1);

    /* Color: hex 0xFFFF00 (yellow) on 0x8B008B (dark magenta), RGB kept */
    WEE_CHECK_ENCODE("test_yellow/darkmagenta_end",
                     STRING_USER_FG_YELLOW_BG_DARKMAGENTA, 0);
    snprintf (string, sizeof (string),
              "test_%sFFFF00,8B008Byellow/darkmagenta%s_end",
              IRC_COLOR_HEX_COLOR_STR,
              IRC_COLOR_HEX_COLOR_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_FG_YELLOW_BG_DARKMAGENTA, 1);

    /* Color: hex background without foreground (reset + text ",8B008B") */
    WEE_CHECK_ENCODE("test_,8B008B_end", STRING_USER_HEX_BG_WITHOUT_FG, 0);
    snprintf (string, sizeof (string),
              "test_%s,8B008B_end",
              IRC_COLOR_HEX_COLOR_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_HEX_BG_WITHOUT_FG, 1);

    /* Color: hex 0xFF7F00 (orange, RGB kept) followed by a comma */
    WEE_CHECK_ENCODE("test_,orange_end", STRING_USER_FG_ORANGE_COMMA, 0);
    snprintf (string, sizeof (string),
              "test_%sFF7F00,orange_end",
              IRC_COLOR_HEX_COLOR_STR);
    WEE_CHECK_ENCODE(string, STRING_USER_FG_ORANGE_COMMA, 1);

    /* Color: hex with less than 6 digits (reset + text "12 apples") */
    WEE_CHECK_ENCODE("test_12 apples", STRING_USER_HEX_SHORT, 0);
    WEE_CHECK_ENCODE(STRING_USER_HEX_SHORT, STRING_USER_HEX_SHORT, 1);

    /* Color: hex background with less than 6 digits (text ",8B00") */
    WEE_CHECK_ENCODE("test_,8B00 orange", STRING_USER_HEX_SHORT_BG, 0);
    WEE_CHECK_ENCODE(STRING_USER_HEX_SHORT_BG, STRING_USER_HEX_SHORT_BG, 1);
}

/*
 * Test functions:
 *   irc_color_code_size
 */

TEST(IrcColor, CodeSize)
{
    LONGS_EQUAL(0, irc_color_code_size (NULL));
    LONGS_EQUAL(0, irc_color_code_size (""));
    LONGS_EQUAL(0, irc_color_code_size ("abc"));
    LONGS_EQUAL(0, irc_color_code_size ("\x02" "abc"));
    LONGS_EQUAL(1, irc_color_code_size ("\x03"));
    LONGS_EQUAL(1, irc_color_code_size ("\x03" "abc"));
    LONGS_EQUAL(1, irc_color_code_size ("\x03" ",05"));
    LONGS_EQUAL(2, irc_color_code_size ("\x03" "4abc"));
    LONGS_EQUAL(3, irc_color_code_size ("\x03" "04abc"));
    LONGS_EQUAL(3, irc_color_code_size ("\x03" "045"));
    LONGS_EQUAL(3, irc_color_code_size ("\x03" "04,abc"));
    LONGS_EQUAL(5, irc_color_code_size ("\x03" "04,5abc"));
    LONGS_EQUAL(6, irc_color_code_size ("\x03" "04,05abc"));
    LONGS_EQUAL(6, irc_color_code_size ("\x03" "04,056"));
    LONGS_EQUAL(1, irc_color_code_size ("\x04"));
    LONGS_EQUAL(1, irc_color_code_size ("\x04" "12 apples"));
    LONGS_EQUAL(7, irc_color_code_size ("\x04" "FF7F00abc"));
    LONGS_EQUAL(7, irc_color_code_size ("\x04" "FF7F00,8B00"));
    LONGS_EQUAL(14, irc_color_code_size ("\x04" "FF7F00,8B008Babc"));
}

/*
 * Test functions:
 *   irc_color_hex_color_value
 *   irc_color_code_parse
 */

TEST(IrcColor, CodeParse)
{
    int fg, bg;

    LONGS_EQUAL(0x000000, irc_color_hex_color_value ((const unsigned char *)"000000"));
    LONGS_EQUAL(0xFF7F00, irc_color_hex_color_value ((const unsigned char *)"FF7F00abc"));
    LONGS_EQUAL(0xFF7F00, irc_color_hex_color_value ((const unsigned char *)"ff7f00"));

    fg = 1;
    bg = 1;
    LONGS_EQUAL(0, irc_color_code_parse (NULL, &fg, &bg));
    LONGS_EQUAL(IRC_COLOR_FORMATTING_NONE, fg);
    LONGS_EQUAL(IRC_COLOR_FORMATTING_NONE, bg);
    LONGS_EQUAL(0, irc_color_code_parse ("abc", &fg, &bg));
    LONGS_EQUAL(IRC_COLOR_FORMATTING_NONE, fg);

    LONGS_EQUAL(1, irc_color_code_parse ("\x03" ",05", &fg, &bg));
    LONGS_EQUAL(IRC_COLOR_FORMATTING_NONE, fg);
    LONGS_EQUAL(IRC_COLOR_FORMATTING_NONE, bg);
    LONGS_EQUAL(2, irc_color_code_parse ("\x03" "4abc", &fg, &bg));
    LONGS_EQUAL(4, fg);
    LONGS_EQUAL(IRC_COLOR_FORMATTING_NONE, bg);
    LONGS_EQUAL(3, irc_color_code_parse ("\x03" "99,abc", &fg, &bg));
    LONGS_EQUAL(99, fg);
    LONGS_EQUAL(IRC_COLOR_FORMATTING_NONE, bg);
    LONGS_EQUAL(5, irc_color_code_parse ("\x03" "04,5abc", &fg, &bg));
    LONGS_EQUAL(4, fg);
    LONGS_EQUAL(5, bg);
    LONGS_EQUAL(6, irc_color_code_parse ("\x03" "04,056", &fg, &bg));
    LONGS_EQUAL(4, fg);
    LONGS_EQUAL(5, bg);

    LONGS_EQUAL(1, irc_color_code_parse ("\x04" "12 apples", &fg, &bg));
    LONGS_EQUAL(IRC_COLOR_FORMATTING_NONE, fg);
    LONGS_EQUAL(7, irc_color_code_parse ("\x04" "ff7f00,8B00", &fg, &bg));
    LONGS_EQUAL(0xFF7F00 | IRC_COLOR_RGB_FLAG, fg);
    LONGS_EQUAL(IRC_COLOR_FORMATTING_NONE, bg);
    LONGS_EQUAL(14, irc_color_code_parse ("\x04" "FF7F00,8B008Babc", &fg, &bg));
    LONGS_EQUAL(0xFF7F00 | IRC_COLOR_RGB_FLAG, fg);
    LONGS_EQUAL(0x8B008B | IRC_COLOR_RGB_FLAG, bg);

    /* Colors are optional. */
    LONGS_EQUAL(3, irc_color_code_parse ("\x03" "04", NULL, NULL));
}

/*
 * Test functions:
 *   irc_color_formatting_transition
 *   irc_color_formatting_add_separator
 */

TEST(IrcColor, FormattingTransition)
{
    struct t_irc_color_formatting from, to;
    char **output;
    int fg_only;

#define WEE_CHECK_TRANSITION(__codes, __fg_only, __colors_only)         \
    output = string_dyn_alloc (32);                                     \
    fg_only = irc_color_formatting_transition (output, &from, &to,      \
                                               __colors_only);          \
    STRCMP_EQUAL(__codes, *output);                                     \
    LONGS_EQUAL(__fg_only, fg_only);                                    \
    string_dyn_free (output, 1);

    output = string_dyn_alloc (32);
    LONGS_EQUAL(0, irc_color_formatting_transition (NULL, &from, &to, 0));
    LONGS_EQUAL(0, irc_color_formatting_transition (output, NULL, &to, 0));
    LONGS_EQUAL(0, irc_color_formatting_transition (output, &from, NULL, 0));
    string_dyn_free (output, 1);

    /* Same formatting: no code. */
    irc_color_formatting_init (&from);
    irc_color_formatting_init (&to);
    WEE_CHECK_TRANSITION("", 0, 0);

    /* Attributes are toggled, or ignored with colors only. */
    from.bold = 1;
    to.underline = 1;
    to.monospace = 1;
    WEE_CHECK_TRANSITION("\x02\x1F\x11", 0, 0);
    WEE_CHECK_TRANSITION("", 0, 1);
    irc_color_formatting_init (&from);
    irc_color_formatting_init (&to);

    /* Text color only. */
    to.fg = 4;
    WEE_CHECK_TRANSITION("\x03" "04", 1, 0);
    to.fg = 0xFF7F00 | IRC_COLOR_RGB_FLAG;
    WEE_CHECK_TRANSITION("\x04" "FF7F00", 2, 0);

    /* Text color and background of the same kind. */
    to.fg = 4;
    to.bg = 2;
    WEE_CHECK_TRANSITION("\x03" "04,02", 0, 0);
    to.fg = 0xFF7F00 | IRC_COLOR_RGB_FLAG;
    to.bg = 0x000080 | IRC_COLOR_RGB_FLAG;
    WEE_CHECK_TRANSITION("\x04" "FF7F00,000080", 0, 0);

    /* Colors of different kinds: background first, then text color. */
    to.fg = 4;
    to.bg = 0x000080 | IRC_COLOR_RGB_FLAG;
    WEE_CHECK_TRANSITION("\x04" "FFFFFF,000080" "\x03" "04", 1, 0);
    to.fg = 0xFF7F00 | IRC_COLOR_RGB_FLAG;
    to.bg = 2;
    WEE_CHECK_TRANSITION("\x03" "99,02" "\x04" "FF7F00", 2, 0);

    /* Background without text color: default text color. */
    to.fg = IRC_COLOR_FORMATTING_NONE;
    to.bg = 2;
    WEE_CHECK_TRANSITION("\x03" "99,02", 0, 0);

    /* Only colors which differ are sent. */
    from.fg = 4;
    from.bg = 2;
    to.fg = 12;
    to.bg = 2;
    WEE_CHECK_TRANSITION("\x03" "12", 1, 0);
    to.fg = 4;
    WEE_CHECK_TRANSITION("", 0, 0);

    /* Background reset to default: colors reset, then text color. */
    to.bg = IRC_COLOR_DEFAULT;
    WEE_CHECK_TRANSITION("\x03\x03" "04", 1, 0);
    to.fg = IRC_COLOR_DEFAULT;
    WEE_CHECK_TRANSITION("\x03" "99,99", 0, 0);

    /* Background without text color: sent with the default color. */
    from.fg = 4;
    from.bg = 2;
    to.fg = IRC_COLOR_FORMATTING_NONE;
    to.bg = 5;
    WEE_CHECK_TRANSITION("\x03" "99,05", 0, 0);

#undef WEE_CHECK_TRANSITION
}

TEST(IrcColor, FormattingSeparator)
{
    char **output;

#define WEE_CHECK_SEPARATOR(__codes, __fg_only, __text, __length)       \
    output = string_dyn_alloc (32);                                     \
    irc_color_formatting_add_separator (output, __fg_only, __text,      \
                                        __length);                      \
    STRCMP_EQUAL(__codes, *output);                                     \
    string_dyn_free (output, 1);

    irc_color_formatting_add_separator (NULL, 1, ",5", -1);

    WEE_CHECK_SEPARATOR("", 0, ",5", -1);
    WEE_CHECK_SEPARATOR("", 1, NULL, -1);
    WEE_CHECK_SEPARATOR("", 1, "5", -1);

    /*
     * Complete text: the color after "," is checked (for "^Dfg", any
     * hexadecimal digit: some IRC clients read 1 to 6 digits).
     */
    WEE_CHECK_SEPARATOR("\x02\x02", 1, ",5 text", -1);
    WEE_CHECK_SEPARATOR("", 1, ", text", -1);
    WEE_CHECK_SEPARATOR("", 1, ",", -1);
    WEE_CHECK_SEPARATOR("\x02\x02", 2, ",000080 text", -1);
    WEE_CHECK_SEPARATOR("\x02\x02", 2, ",5 text", -1);
    WEE_CHECK_SEPARATOR("", 2, ",g text", -1);

    /* Partial text: the rest may come later. */
    WEE_CHECK_SEPARATOR("\x02\x02", 1, ",", 1);
    WEE_CHECK_SEPARATOR("\x02\x02", 1, ",5", 2);
    WEE_CHECK_SEPARATOR("", 1, ",a", 2);
    WEE_CHECK_SEPARATOR("\x02\x02", 2, ",a", 2);
    WEE_CHECK_SEPARATOR("", 2, ",g", 2);

#undef WEE_CHECK_SEPARATOR
}

/*
 * Test functions:
 *   irc_color_formatting_init
 *   irc_color_formatting_update
 *   irc_color_formatting_codes
 */

TEST(IrcColor, Formatting)
{
    struct t_irc_color_formatting formatting;
    const char *string;
    char *codes;

#define WEE_CHECK_FORMATTING(__codes, __string, __next_text)            \
    irc_color_formatting_init (&formatting);                            \
    string = __string;                                                  \
    irc_color_formatting_update (&formatting, string, strlen (string)); \
    codes = irc_color_formatting_codes (&formatting, __next_text, 0);   \
    STRCMP_EQUAL(__codes, codes);                                       \
    free (codes);

    irc_color_formatting_init (NULL);
    irc_color_formatting_update (NULL, "\x02", 1);
    POINTERS_EQUAL(NULL, irc_color_formatting_codes (NULL, NULL, 0));

    /* No formatting. */
    WEE_CHECK_FORMATTING("", "", NULL);
    WEE_CHECK_FORMATTING("", "test", "next");

    /* Attributes. */
    WEE_CHECK_FORMATTING("\x02", "\x02" "bold", NULL);
    WEE_CHECK_FORMATTING("", "\x02" "bold" "\x02", NULL);
    WEE_CHECK_FORMATTING("\x02\x16\x1D\x1E\x1F\x11",
                         "\x02\x16\x1D\x1E\x1F\x11" "all", NULL);
    WEE_CHECK_FORMATTING("", "\x02\x16\x1D\x1E\x1F\x11" "all" "\x0F", NULL);

    /* Colors: text color alone keeps the background. */
    WEE_CHECK_FORMATTING("\x03" "04", "\x03" "4red", NULL);
    WEE_CHECK_FORMATTING("\x03" "04,02", "\x03" "04,02" "red/blue", NULL);
    WEE_CHECK_FORMATTING("\x03" "12,02",
                         "\x03" "04,02" "red/blue" "\x03" "12" "lightblue",
                         NULL);
    WEE_CHECK_FORMATTING("", "\x03" "04,02" "red/blue" "\x03" "reset", NULL);
    WEE_CHECK_FORMATTING("\x02\x03" "04",
                         "\x02\x03" "04" "bold red", NULL);

    /* Hex colors. */
    WEE_CHECK_FORMATTING("\x04" "FF7F00", "\x04" "FF7F00" "orange", NULL);
    WEE_CHECK_FORMATTING("\x04" "FF7F00,000080",
                         "\x04" "FF7F00,000080" "orange/navy", NULL);
    WEE_CHECK_FORMATTING("", "\x04" "FF7F00" "orange" "\x04" "reset", NULL);
    WEE_CHECK_FORMATTING("\x04" "FF7F00", "\x04" "ff7f00" "orange", NULL);

    /* Mixed kinds of colors: background first, then text color. */
    WEE_CHECK_FORMATTING("\x03" "99,02" "\x04" "FF7F00",
                         "\x03" "04,02" "red/blue" "\x04" "FF7F00" "orange",
                         NULL);
    WEE_CHECK_FORMATTING("\x04" "FFFFFF,000080" "\x03" "04",
                         "\x04" "FF7F00,000080" "orange" "\x03" "04" "red",
                         NULL);
    WEE_CHECK_FORMATTING("\x04" "FFFFFF,000080" "\x03" "99",
                         "\x04" "FF7F00,000080" "orange" "\x03" "99"
                         "default/navy",
                         NULL);

    /* Text color followed by "," + digits: "^B^B" is added. */
    WEE_CHECK_FORMATTING("\x03" "04" "\x02\x02", "\x03" "04" "red", ",5 text");
    WEE_CHECK_FORMATTING("\x03" "04", "\x03" "04" "red", ", text");
    WEE_CHECK_FORMATTING("\x03" "04,02", "\x03" "04,02" "red", ",5 text");
    WEE_CHECK_FORMATTING("\x04" "FF7F00" "\x02\x02",
                         "\x04" "FF7F00" "orange", ",000080 text");
    WEE_CHECK_FORMATTING("\x04" "FF7F00" "\x02\x02",
                         "\x04" "FF7F00" "orange", ",5 text");
    WEE_CHECK_FORMATTING("\x04" "FF7F00", "\x04" "FF7F00" "orange", ",g text");

    /* Attributes only (no separator after attributes). */
    irc_color_formatting_init (&formatting);
    string = "\x02\x1F" "\x03" "04,02" "bold underline red/blue";
    irc_color_formatting_update (&formatting, string, strlen (string));
    codes = irc_color_formatting_codes (&formatting, NULL, 1);
    STRCMP_EQUAL("\x02\x1F", codes);
    free (codes);
    irc_color_formatting_init (&formatting);
    string = "\x03" "04" "red";
    irc_color_formatting_update (&formatting, string, strlen (string));
    codes = irc_color_formatting_codes (&formatting, ",5 text", 1);
    STRCMP_EQUAL("", codes);
    free (codes);

    /* Code cut at the end of the string: ignored. */
    irc_color_formatting_init (&formatting);
    string = "\x03" "04,02" "red/blue";
    irc_color_formatting_update (&formatting, string, 4);
    codes = irc_color_formatting_codes (&formatting, NULL, 0);
    STRCMP_EQUAL("", codes);
    free (codes);
}

/*
 * Test functions:
 *   irc_color_is_hex_color
 */

TEST(IrcColor, IsHexColor)
{
    LONGS_EQUAL(0, irc_color_is_hex_color ((const unsigned char *)""));
    LONGS_EQUAL(0, irc_color_is_hex_color ((const unsigned char *)"12"));
    LONGS_EQUAL(0, irc_color_is_hex_color ((const unsigned char *)"FF7F0"));
    LONGS_EQUAL(0, irc_color_is_hex_color ((const unsigned char *)"FF7F0G"));
    LONGS_EQUAL(1, irc_color_is_hex_color ((const unsigned char *)"FF7F00"));
    LONGS_EQUAL(1, irc_color_is_hex_color ((const unsigned char *)"ff7f00"));
    LONGS_EQUAL(1, irc_color_is_hex_color ((const unsigned char *)"FF7F00AB"));
}

/*
 * Test functions:
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

    /* No color codes */
    WEE_CHECK_DECODE_ANSI("test string", "test string", 0);
    WEE_CHECK_DECODE_ANSI("test string", "test string", 1);

    /* Sequences not supported (not ending with "m") */
    WEE_CHECK_DECODE_ANSI("", "\x1B[z", 0);
    WEE_CHECK_DECODE_ANSI("", "\x1B[z", 1);
    WEE_CHECK_DECODE_ANSI("test", "\x1B[ztest", 0);
    WEE_CHECK_DECODE_ANSI("test", "\x1B[ztest", 1);

    /* Color: reset (implicit) */
    WEE_CHECK_DECODE_ANSI("test_reset", STRING_ANSI_RESET, 0);
    snprintf (string, sizeof (string),
              "test_%sreset",
              IRC_COLOR_RESET_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_RESET, 1);

    /* Color: reset (with "0") */
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

    /* strikethrough */
    WEE_CHECK_DECODE_ANSI("test_strikethrough_normal", STRING_ANSI_STRIKETHROUGH, 0);
    snprintf (string, sizeof (string),
              "test_%sstrikethrough%s_normal",
              IRC_COLOR_STRIKETHROUGH_STR,
              IRC_COLOR_STRIKETHROUGH_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_STRIKETHROUGH, 1);

    /* underline */
    WEE_CHECK_DECODE_ANSI("test_underline_normal", STRING_ANSI_UNDERLINE, 0);
    snprintf (string, sizeof (string),
              "test_%sunderline%s_normal",
              IRC_COLOR_UNDERLINE_STR,
              IRC_COLOR_UNDERLINE_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_UNDERLINE, 1);

    /* Color: blue */
    WEE_CHECK_DECODE_ANSI("test_blue", STRING_ANSI_FG_BLUE, 0);
    snprintf (string, sizeof (string),
              "test_%s02blue",
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_FG_BLUE, 1);

    /* Color: lightcyan */
    WEE_CHECK_DECODE_ANSI("test_lightcyan", STRING_ANSI_FG_LIGHTCYAN, 0);
    snprintf (string, sizeof (string),
              "test_%s11lightcyan",
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_FG_LIGHTCYAN, 1);

    /* Color: blue on red */
    WEE_CHECK_DECODE_ANSI("test_blue/red", STRING_ANSI_FG_BLUE_BG_RED, 0);
    snprintf (string, sizeof (string),
              "test_%s02%s,05blue/red",
              IRC_COLOR_COLOR_STR,
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_FG_BLUE_BG_RED, 1);

    /* Color: lightcyan on lightblue */
    WEE_CHECK_DECODE_ANSI("test_lightcyan/lightblue",
                          STRING_ANSI_FG_LIGHTCYAN_BG_LIGHTBLUE, 0);
    snprintf (string, sizeof (string),
              "test_%s11%s,12lightcyan/lightblue",
              IRC_COLOR_COLOR_STR,
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_FG_LIGHTCYAN_BG_LIGHTBLUE, 1);

    /* Color: RGB "FF00FF" (term 13 -> IRC 13 -> lightmagenta) */
    WEE_CHECK_DECODE_ANSI("test_color13", STRING_ANSI_FG_RGB_IRC_13, 0);
    snprintf (string, sizeof (string),
              "test_%s13color13",
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_FG_RGB_IRC_13, 1);

    /* Color: term 13 -> IRC 13 -> lightmagenta */
    WEE_CHECK_DECODE_ANSI("test_color13", STRING_ANSI_FG_TERM_IRC_13, 0);
    snprintf (string, sizeof (string),
              "test_%s13color13",
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_FG_TERM_IRC_13, 1);

    /*
     * Color: RGB "FF00FF" (term 13 -> IRC 13 -> lightmagenta)
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
     * Color: term 13 -> IRC 13 -> lightmagenta
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

    /* Default text color */
    WEE_CHECK_DECODE_ANSI("test_default_fg", STRING_ANSI_DEFAULT_FG, 0);
    snprintf (string, sizeof (string),
              "test_%s15default_fg",
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_DEFAULT_FG, 1);

    /* Default background color */
    WEE_CHECK_DECODE_ANSI("test_default_bg", STRING_ANSI_DEFAULT_BG, 0);
    snprintf (string, sizeof (string),
              "test_%s,01default_bg",
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_DEFAULT_BG, 1);

    /* Empty parameters (reset) */
    WEE_CHECK_DECODE_ANSI("AXB", STRING_ANSI_EMPTY_PARAM_ONLY, 0);
    snprintf (string, sizeof (string),
              "A%sX%s%sB",
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_RESET_STR,
              IRC_COLOR_RESET_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_EMPTY_PARAM_ONLY, 1);
    WEE_CHECK_DECODE_ANSI("AB", STRING_ANSI_EMPTY_PARAM_FIRST, 0);
    snprintf (string, sizeof (string),
              "A%s%sB",
              IRC_COLOR_RESET_STR,
              IRC_COLOR_BOLD_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_EMPTY_PARAM_FIRST, 1);
    WEE_CHECK_DECODE_ANSI("AB", STRING_ANSI_EMPTY_PARAM_MIDDLE, 0);
    snprintf (string, sizeof (string),
              "A%s%s%sB",
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_RESET_STR,
              IRC_COLOR_UNDERLINE_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_EMPTY_PARAM_MIDDLE, 1);
    WEE_CHECK_DECODE_ANSI("AB", STRING_ANSI_EMPTY_PARAM_LAST, 0);
    snprintf (string, sizeof (string),
              "A%s%sB",
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_RESET_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_EMPTY_PARAM_LAST, 1);

    /* Empty arguments in text/background colors (0 -> IRC 01 -> black) */
    WEE_CHECK_DECODE_ANSI("test_color01", STRING_ANSI_FG_TERM_EMPTY, 0);
    snprintf (string, sizeof (string),
              "test_%s01color01",
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_FG_TERM_EMPTY, 1);
    snprintf (string, sizeof (string),
              "test_%s,01%scolor01",
              IRC_COLOR_COLOR_STR,
              IRC_COLOR_BOLD_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_BG_TERM_EMPTY, 1);

    /* Empty arguments in RGB colors: "FF00FF" (IRC 13 -> lightmagenta) */
    WEE_CHECK_DECODE_ANSI("test_color13", STRING_ANSI_FG_RGB_EMPTY, 0);
    snprintf (string, sizeof (string),
              "test_%s13color13",
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_FG_RGB_EMPTY, 1);
    snprintf (string, sizeof (string),
              "test_%s,13%scolor13",
              IRC_COLOR_COLOR_STR,
              IRC_COLOR_UNDERLINE_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_BG_RGB_EMPTY, 1);

    /* Underline terminal color (ignored) */
    WEE_CHECK_DECODE_ANSI("test_ul_1", STRING_ANSI_UNDERLINE_COLOR_TERM, 0);
    WEE_CHECK_DECODE_ANSI("test_ul_1", STRING_ANSI_UNDERLINE_COLOR_TERM, 1);

    /* Underline RGB color (ignored) */
    WEE_CHECK_DECODE_ANSI("test_ul_red", STRING_ANSI_UNDERLINE_COLOR_RGB, 0);
    WEE_CHECK_DECODE_ANSI("test_ul_red", STRING_ANSI_UNDERLINE_COLOR_RGB, 1);

    /* Default underline color (ignored) */
    WEE_CHECK_DECODE_ANSI("test_ul_default",
                          STRING_ANSI_UNDERLINE_COLOR_DEFAULT, 0);
    WEE_CHECK_DECODE_ANSI("test_ul_default",
                          STRING_ANSI_UNDERLINE_COLOR_DEFAULT, 1);

    /*
     * Underline + RGB "008080" (IRC 10 -> cyan)
     * + underline RGB color (ignored)
     */
    WEE_CHECK_DECODE_ANSI("test_ul_color10",
                          STRING_ANSI_UNDERLINE_FG_RGB_UNDERLINE_COLOR_RGB, 0);
    snprintf (string, sizeof (string),
              "test_%s%s10ul_color10",
              IRC_COLOR_UNDERLINE_STR,
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string,
                          STRING_ANSI_UNDERLINE_FG_RGB_UNDERLINE_COLOR_RGB, 1);

    /* Underline + underline colors (ignored) + blue + bold */
    WEE_CHECK_DECODE_ANSI("test_ul_blue", STRING_ANSI_UNDERLINE_COLOR_MIXED, 0);
    snprintf (string, sizeof (string),
              "test_%s%s02%sul_blue",
              IRC_COLOR_UNDERLINE_STR,
              IRC_COLOR_COLOR_STR,
              IRC_COLOR_BOLD_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_UNDERLINE_COLOR_MIXED, 1);

    /* Other escape sequences (removed) */
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B(BB", 0);
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B(BB", 1);
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B(mB", 0);
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B(mB", 1);
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B>B", 0);
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B>B", 1);

    /* Control sequences other than SGR (removed) */
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B[2 qB", 0);
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B[2 qB", 1);
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B[=5uB", 0);
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B[=5uB", 1);
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B[3~B", 0);
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B[3~B", 1);

    /* Private control sequences ending with "m" (removed) */
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B[>4;2mB", 0);
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B[>4;2mB", 1);
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B[?4mB", 0);
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B[?4mB", 1);

    /* Private bytes after the first parameter (removed) */
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B[1;>4mB", 1);
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B[1<mB", 1);
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B[5=mB", 1);

    /* Control sequence ending with "m" with intermediate bytes (removed) */
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B[1 mB", 0);
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B[1 mB", 1);

    /* Parameters with sub-parameters (ignored) */
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B[38:2::255:0:0mB", 0);
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B[38:2::255:0:0mB", 1);
    WEE_CHECK_DECODE_ANSI("AB", "A\x1B[4:0mB", 1);
    WEE_CHECK_DECODE_ANSI("test_bold_blue", STRING_ANSI_SUB_PARAMS, 0);
    snprintf (string, sizeof (string),
              "test_%s%s02bold_blue",
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_SUB_PARAMS, 1);

    /* RGB color with components greater than 255 (clamped to 255) */
    WEE_CHECK_DECODE_ANSI("test_color09",
                          STRING_ANSI_FG_RGB_COMPONENT_256, 0);
    snprintf (string, sizeof (string),
              "test_%s09color09",
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_FG_RGB_COMPONENT_256, 1);
    WEE_CHECK_DECODE_ANSI("test_color04",
                          STRING_ANSI_FG_RGB_COMPONENT_40000, 0);
    snprintf (string, sizeof (string),
              "test_%s04color04",
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_FG_RGB_COMPONENT_40000, 1);
    WEE_CHECK_DECODE_ANSI("test_color11",
                          STRING_ANSI_BG_RGB_COMPONENTS_TOO_BIG, 0);
    snprintf (string, sizeof (string),
              "test_%s,11color11",
              IRC_COLOR_COLOR_STR);
    WEE_CHECK_DECODE_ANSI(string, STRING_ANSI_BG_RGB_COMPONENTS_TOO_BIG, 1);
}

/*
 * Test functions:
 *   irc_color_ansi_next_param
 *   irc_color_ansi_next_color
 *   irc_color_ansi_sequence_size
 *   irc_color_decode_ansi_sequence
 */

TEST(IrcColor, DecodeAnsiSequence)
{
    struct t_irc_color_ansi_state ansi_state;
    const char *ptr_param, *params;
    char **output;
    int value;

    params = "1;38;5;196";
    ptr_param = params;
    LONGS_EQUAL(1, irc_color_ansi_next_param (&ptr_param, params + 10, &value));
    LONGS_EQUAL(1, value);
    LONGS_EQUAL(1, irc_color_ansi_next_param (&ptr_param, params + 10, &value));
    LONGS_EQUAL(38, value);
    LONGS_EQUAL(196, irc_color_ansi_next_color (&ptr_param, params + 10));
    LONGS_EQUAL(0, irc_color_ansi_next_param (&ptr_param, params + 10, &value));

    params = "2;255;136;0";
    ptr_param = params;
    LONGS_EQUAL(IRC_COLOR_RGB_FLAG | 0xFF8800,
                irc_color_ansi_next_color (&ptr_param, params + 11));
    params = "2;256;0;0";
    ptr_param = params;
    LONGS_EQUAL(-1, irc_color_ansi_next_color (&ptr_param, params + 9));
    params = "x";
    ptr_param = params;
    LONGS_EQUAL(0, irc_color_ansi_next_param (&ptr_param, params + 1, &value));

    LONGS_EQUAL(0, irc_color_ansi_sequence_size ("test"));
    LONGS_EQUAL(0, irc_color_ansi_sequence_size ("\x1B[m"));
    LONGS_EQUAL(0, irc_color_ansi_sequence_size ("\x1B[1"));
    LONGS_EQUAL(0, irc_color_ansi_sequence_size ("\x1B[?1m"));
    LONGS_EQUAL(4, irc_color_ansi_sequence_size ("\x1B[1mtest"));
    LONGS_EQUAL(11, irc_color_ansi_sequence_size ("\x1B[38;5;196m"));

    /* Invalid parameters (for example changed by a script) are ignored. */
    irc_color_formatting_init (&ansi_state.formatting);
    output = string_dyn_alloc (32);
    irc_color_decode_ansi_sequence (output, &ansi_state, "1;x;4", 5);
    STRCMP_EQUAL("\x02", *output);
    irc_color_decode_ansi_sequence (output, &ansi_state, "38;5", 4);
    STRCMP_EQUAL("\x02", *output);
    string_dyn_free (output, 1);
}

/*
 * Test functions:
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
 * Test functions:
 *   irc_color_modifier_cb
 */

TEST(IrcColor, ModifierCallback)
{
    char string[1024], *result;

    /* Modifier "irc_color_decode" */
    snprintf (string, sizeof (string),
              "test_%sbold%s_end",
              gui_color_get_custom ("bold"),
              gui_color_get_custom ("-bold"));
    result = hook_modifier_exec (NULL, "irc_color_decode",
                                 "1", STRING_IRC_BOLD);
    STRCMP_EQUAL(string, result);
    free (result);

    /* Modifier "irc_color_encode" */
    snprintf (string, sizeof (string),
              "test_%sbold%s_end",
              IRC_COLOR_BOLD_STR,
              IRC_COLOR_BOLD_STR);
    result = hook_modifier_exec (NULL, "irc_color_encode",
                                 "1", STRING_USER_BOLD);
    STRCMP_EQUAL(string, result);
    free (result);

    /* Modifier "irc_color_decode_ansi" */
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
 * Test functions:
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
