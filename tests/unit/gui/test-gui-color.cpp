/*
 * test-gui-color.cpp - test color functions
 *
 * Copyright (C) 2019-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/core/wee-config.h"
#include "src/core/wee-string.h"
#include "src/gui/gui-color.h"
}

#define WEE_CHECK_DECODE(__result, __string, __replacement)             \
    decoded = gui_color_decode (__string, __replacement);               \
    STRCMP_EQUAL(__result, decoded);                                    \
    free (decoded);

#define WEE_CHECK_DECODE_ANSI(__result, __string, __keep_colors)        \
    decoded = gui_color_decode_ansi (__string, __keep_colors);          \
    STRCMP_EQUAL(__result, decoded);                                    \
    free (decoded);

#define WEE_CHECK_ENCODE_ANSI(__result, __string)                       \
    encoded = gui_color_encode_ansi (__string);                         \
    STRCMP_EQUAL(__result, encoded);                                    \
    free (encoded);

#define WEE_CHECK_EMPHASIZE(__result, __string, __search,               \
                            __case_sensitive, __regex)                  \
    emphasized = gui_color_emphasize (__string, __search,               \
                                      __case_sensitive, __regex);       \
    STRCMP_EQUAL(__result, emphasized);                                 \
    free (emphasized);

TEST_GROUP(GuiColor)
{
};

/*
 * Tests functions:
 *   gui_color_get_custom
 */

TEST(GuiColor, GetCustom)
{
    char string[32];

    STRCMP_EQUAL("", gui_color_get_custom (NULL));
    STRCMP_EQUAL("", gui_color_get_custom (""));

    /* reset */
    snprintf (string, sizeof (string), "%c", GUI_COLOR_RESET_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("reset"));

    /* resetcolor */
    snprintf (string, sizeof (string),
              "%c%c",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_RESET_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("resetcolor"));

    /* emphasis */
    snprintf (string, sizeof (string),
              "%c%c",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_EMPHASIS_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("emphasis"));

    /* blink */
    snprintf (string, sizeof (string),
              "%c%c",
              GUI_COLOR_SET_ATTR_CHAR,
              GUI_COLOR_ATTR_BLINK_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("blink"));

    /* -blink */
    snprintf (string, sizeof (string),
              "%c%c",
              GUI_COLOR_REMOVE_ATTR_CHAR,
              GUI_COLOR_ATTR_BLINK_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("-blink"));

    /* dim */
    snprintf (string, sizeof (string),
              "%c%c",
              GUI_COLOR_SET_ATTR_CHAR,
              GUI_COLOR_ATTR_DIM_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("dim"));

    /* -dim */
    snprintf (string, sizeof (string),
              "%c%c",
              GUI_COLOR_REMOVE_ATTR_CHAR,
              GUI_COLOR_ATTR_DIM_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("-dim"));

    /* bold */
    snprintf (string, sizeof (string),
              "%c%c",
              GUI_COLOR_SET_ATTR_CHAR,
              GUI_COLOR_ATTR_BOLD_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("bold"));

    /* -bold */
    snprintf (string, sizeof (string),
              "%c%c",
              GUI_COLOR_REMOVE_ATTR_CHAR,
              GUI_COLOR_ATTR_BOLD_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("-bold"));

    /* reverse */
    snprintf (string, sizeof (string),
              "%c%c",
              GUI_COLOR_SET_ATTR_CHAR,
              GUI_COLOR_ATTR_REVERSE_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("reverse"));

    /* -reverse */
    snprintf (string, sizeof (string),
              "%c%c",
              GUI_COLOR_REMOVE_ATTR_CHAR,
              GUI_COLOR_ATTR_REVERSE_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("-reverse"));

    /* italic */
    snprintf (string, sizeof (string),
              "%c%c",
              GUI_COLOR_SET_ATTR_CHAR,
              GUI_COLOR_ATTR_ITALIC_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("italic"));

    /* -italic */
    snprintf (string, sizeof (string),
              "%c%c",
              GUI_COLOR_REMOVE_ATTR_CHAR,
              GUI_COLOR_ATTR_ITALIC_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("-italic"));

    /* underline */
    snprintf (string, sizeof (string),
              "%c%c",
              GUI_COLOR_SET_ATTR_CHAR,
              GUI_COLOR_ATTR_UNDERLINE_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("underline"));

    /* -underline */
    snprintf (string, sizeof (string),
              "%c%c",
              GUI_COLOR_REMOVE_ATTR_CHAR,
              GUI_COLOR_ATTR_UNDERLINE_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("-underline"));

    /* bar_fg */
    snprintf (string, sizeof (string),
              "%c%c%c",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_BAR_CHAR,
              GUI_COLOR_BAR_FG_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("bar_fg"));

    /* bar_delim */
    snprintf (string, sizeof (string),
              "%c%c%c",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_BAR_CHAR,
              GUI_COLOR_BAR_DELIM_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("bar_delim"));

    /* bar_bg */
    snprintf (string, sizeof (string),
              "%c%c%c",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_BAR_CHAR,
              GUI_COLOR_BAR_BG_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("bar_bg"));

    /* only delimiter (no-op) */
    STRCMP_EQUAL("", gui_color_get_custom (","));
    STRCMP_EQUAL("", gui_color_get_custom (":"));

    /* fg color */
    snprintf (string, sizeof (string),
              "%c%c09",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_FG_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("blue"));

    /* fg color, delimiter and no bg color */
    snprintf (string, sizeof (string),
              "%c%c09",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_FG_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("blue,"));
    STRCMP_EQUAL(string, gui_color_get_custom ("blue:"));

    /* bg color */
    snprintf (string, sizeof (string),
              "%c%c09",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_BG_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom (",blue"));
    STRCMP_EQUAL(string, gui_color_get_custom (":blue"));

    /* fg+bg color */
    snprintf (string, sizeof (string),
              "%c%c08~09",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_FG_BG_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("yellow,blue"));
    STRCMP_EQUAL(string, gui_color_get_custom ("yellow:blue"));

    /* fg terminal color */
    snprintf (string, sizeof (string),
              "%c%c%c00214",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_FG_CHAR,
              GUI_COLOR_EXTENDED_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("214"));

    /* bg terminal color */
    snprintf (string, sizeof (string),
              "%c%c%c00214",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_BG_CHAR,
              GUI_COLOR_EXTENDED_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom (",214"));
    STRCMP_EQUAL(string, gui_color_get_custom (":214"));

    /* fg+bg terminal color */
    snprintf (string, sizeof (string),
              "%c%c%c00227~%c00240",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_FG_BG_CHAR,
              GUI_COLOR_EXTENDED_CHAR,
              GUI_COLOR_EXTENDED_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("227,240"));
    STRCMP_EQUAL(string, gui_color_get_custom ("227:240"));

    /* fg terminal color + bg color */
    snprintf (string, sizeof (string),
              "%c%c%c00227~09",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_FG_BG_CHAR,
              GUI_COLOR_EXTENDED_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("227,blue"));
    STRCMP_EQUAL(string, gui_color_get_custom ("227:blue"));

    /* fg color with attributes + bg terminal color */
    snprintf (string, sizeof (string),
              "%c%c%c_/00227~09",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_FG_BG_CHAR,
              GUI_COLOR_EXTENDED_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("_/227,blue"));
    STRCMP_EQUAL(string, gui_color_get_custom ("_/227:blue"));
}

/*
 * Tests functions:
 *   gui_color_code_size
 */

TEST(GuiColor, CodeSize)
{
    char string[256];

    /* NULL/empty string */
    LONGS_EQUAL(0, gui_color_code_size (NULL));
    LONGS_EQUAL(0, gui_color_code_size (""));

    /* no color code */
    LONGS_EQUAL(0, gui_color_code_size ("test"));

    /* reset */
    LONGS_EQUAL(1, gui_color_code_size (gui_color_get_custom ("reset")));

    /* reset (×2) */
    snprintf (string, sizeof (string),
              "%s%s",
              gui_color_get_custom ("reset"),
              gui_color_get_custom ("reset"));
    LONGS_EQUAL(1, gui_color_code_size (string));

    /* resetcolor */
    LONGS_EQUAL(2, gui_color_code_size (gui_color_get_custom ("resetcolor")));

    /* emphasis */
    LONGS_EQUAL(2, gui_color_code_size (gui_color_get_custom ("emphasis")));

    /* blink */
    LONGS_EQUAL(2, gui_color_code_size (gui_color_get_custom ("blink")));
    LONGS_EQUAL(2, gui_color_code_size (gui_color_get_custom ("-blink")));

    /* dim */
    LONGS_EQUAL(2, gui_color_code_size (gui_color_get_custom ("dim")));
    LONGS_EQUAL(2, gui_color_code_size (gui_color_get_custom ("-dim")));

    /* bold */
    LONGS_EQUAL(2, gui_color_code_size (gui_color_get_custom ("bold")));
    LONGS_EQUAL(2, gui_color_code_size (gui_color_get_custom ("-bold")));

    /* reverse */
    LONGS_EQUAL(2, gui_color_code_size (gui_color_get_custom ("reverse")));
    LONGS_EQUAL(2, gui_color_code_size (gui_color_get_custom ("-reverse")));

    /* italic */
    LONGS_EQUAL(2, gui_color_code_size (gui_color_get_custom ("italic")));
    LONGS_EQUAL(2, gui_color_code_size (gui_color_get_custom ("-italic")));

    /* underline */
    LONGS_EQUAL(2, gui_color_code_size (gui_color_get_custom ("underline")));
    LONGS_EQUAL(2, gui_color_code_size (gui_color_get_custom ("-underline")));

    /* bar_fg */
    LONGS_EQUAL(3, gui_color_code_size (gui_color_get_custom ("bar_fg")));

    /* bar_delim */
    LONGS_EQUAL(3, gui_color_code_size (gui_color_get_custom ("bar_delim")));

    /* bar_bg */
    LONGS_EQUAL(3, gui_color_code_size (gui_color_get_custom ("bar_bg")));

    /* fg color */
    LONGS_EQUAL(4, gui_color_code_size (gui_color_get_custom ("blue")));

    /* bg color */
    LONGS_EQUAL(4, gui_color_code_size (gui_color_get_custom (",blue")));

    /* fg+bg color */
    LONGS_EQUAL(7, gui_color_code_size (gui_color_get_custom ("yellow,blue")));

    /* fg+bg color (×2) */
    snprintf (string, sizeof (string),
              "%s%s",
              gui_color_get_custom ("yellow,blue"),
              gui_color_get_custom ("yellow,blue"));
    LONGS_EQUAL(7, gui_color_code_size (string));

    /* fg terminal color */
    LONGS_EQUAL(8, gui_color_code_size (gui_color_get_custom ("214")));

    /* bg terminal color */
    LONGS_EQUAL(8, gui_color_code_size (gui_color_get_custom (",214")));

    /* fg+bg terminal color */
    LONGS_EQUAL(15, gui_color_code_size (gui_color_get_custom ("227,240")));

    /* fg terminal color + bg color */
    LONGS_EQUAL(11, gui_color_code_size (gui_color_get_custom ("227,blue")));

    /* WeeChat color */
    LONGS_EQUAL(3, gui_color_code_size (GUI_COLOR(GUI_COLOR_CHAT_HOST)));
}

/*
 * Tests functions:
 *   gui_color_decode
 */

TEST(GuiColor, Decode)
{
    char string[256], *decoded;

    /* NULL/empty string */
    POINTERS_EQUAL(NULL, gui_color_decode (NULL, NULL));
    POINTERS_EQUAL(NULL, gui_color_decode (NULL, ""));
    POINTERS_EQUAL(NULL, gui_color_decode (NULL, "?"));
    WEE_CHECK_DECODE("", "", NULL);
    WEE_CHECK_DECODE("", "", "");
    WEE_CHECK_DECODE("", "", "?");

    /* no color codes */
    WEE_CHECK_DECODE("test string", "test string", NULL);
    WEE_CHECK_DECODE("test string", "test string", "");
    WEE_CHECK_DECODE("test string", "test string", "?");

    /* reset */
    snprintf (string, sizeof (string),
              "test_%sreset",
              gui_color_get_custom ("reset"));
    WEE_CHECK_DECODE("test_reset", string, NULL);
    WEE_CHECK_DECODE("test_reset", string, "");
    WEE_CHECK_DECODE("test_?reset", string, "?");

    /* resetcolor */
    snprintf (string, sizeof (string),
              "test_%sresetcolor",
              gui_color_get_custom ("resetcolor"));
    WEE_CHECK_DECODE("test_resetcolor", string, NULL);
    WEE_CHECK_DECODE("test_resetcolor", string, "");
    WEE_CHECK_DECODE("test_?resetcolor", string, "?");

    /* emphasis */
    snprintf (string, sizeof (string),
              "test_%semphasis",
              gui_color_get_custom ("emphasis"));
    WEE_CHECK_DECODE("test_emphasis", string, NULL);
    WEE_CHECK_DECODE("test_emphasis", string, "");
    WEE_CHECK_DECODE("test_?emphasis", string, "?");

    /* blink */
    snprintf (string, sizeof (string),
              "test_%sblink%s_end",
              gui_color_get_custom ("blink"),
              gui_color_get_custom ("-blink"));
    WEE_CHECK_DECODE("test_blink_end", string, NULL);
    WEE_CHECK_DECODE("test_blink_end", string, "");
    WEE_CHECK_DECODE("test_?blink?_end", string, "?");

    /* dim */
    snprintf (string, sizeof (string),
              "test_%sdim%s_end",
              gui_color_get_custom ("dim"),
              gui_color_get_custom ("-dim"));
    WEE_CHECK_DECODE("test_dim_end", string, NULL);
    WEE_CHECK_DECODE("test_dim_end", string, "");
    WEE_CHECK_DECODE("test_?dim?_end", string, "?");

    /* bold */
    snprintf (string, sizeof (string),
              "test_%sbold%s_end",
              gui_color_get_custom ("bold"),
              gui_color_get_custom ("-bold"));
    WEE_CHECK_DECODE("test_bold_end", string, NULL);
    WEE_CHECK_DECODE("test_bold_end", string, "");
    WEE_CHECK_DECODE("test_?bold?_end", string, "?");

    /* reverse */
    snprintf (string, sizeof (string),
              "test_%sreverse%s_end",
              gui_color_get_custom ("reverse"),
              gui_color_get_custom ("-reverse"));
    WEE_CHECK_DECODE("test_reverse_end", string, NULL);
    WEE_CHECK_DECODE("test_reverse_end", string, "");
    WEE_CHECK_DECODE("test_?reverse?_end", string, "?");

    /* italic */
    snprintf (string, sizeof (string),
              "test_%sitalic%s_end",
              gui_color_get_custom ("italic"),
              gui_color_get_custom ("-italic"));
    WEE_CHECK_DECODE("test_italic_end", string, NULL);
    WEE_CHECK_DECODE("test_italic_end", string, "");
    WEE_CHECK_DECODE("test_?italic?_end", string, "?");

    /* underline */
    snprintf (string, sizeof (string),
              "test_%sunderline%s_end",
              gui_color_get_custom ("underline"),
              gui_color_get_custom ("-underline"));
    WEE_CHECK_DECODE("test_underline_end", string, NULL);
    WEE_CHECK_DECODE("test_underline_end", string, "");
    WEE_CHECK_DECODE("test_?underline?_end", string, "?");

    /* bar_fg */
    snprintf (string, sizeof (string),
              "test_%sbar_fg",
              gui_color_get_custom ("bar_fg"));
    WEE_CHECK_DECODE("test_bar_fg", string, NULL);
    WEE_CHECK_DECODE("test_bar_fg", string, "");
    WEE_CHECK_DECODE("test_?bar_fg", string, "?");

    /* bar_delim */
    snprintf (string, sizeof (string),
              "test_%sbar_delim",
              gui_color_get_custom ("bar_delim"));
    WEE_CHECK_DECODE("test_bar_delim", string, NULL);
    WEE_CHECK_DECODE("test_bar_delim", string, "");
    WEE_CHECK_DECODE("test_?bar_delim", string, "?");

    /* bar_bg */
    snprintf (string, sizeof (string),
              "test_%sbar_bg",
              gui_color_get_custom ("bar_bg"));
    WEE_CHECK_DECODE("test_bar_bg", string, NULL);
    WEE_CHECK_DECODE("test_bar_bg", string, "");
    WEE_CHECK_DECODE("test_?bar_bg", string, "?");

    /* fg color */
    snprintf (string, sizeof (string),
              "test_%sblue",
              gui_color_get_custom ("blue"));
    WEE_CHECK_DECODE("test_blue", string, NULL);
    WEE_CHECK_DECODE("test_blue", string, "");
    WEE_CHECK_DECODE("test_?blue", string, "?");

    /* bg color */
    snprintf (string, sizeof (string),
              "test_%sblue",
              gui_color_get_custom (",blue"));
    WEE_CHECK_DECODE("test_blue", string, NULL);
    WEE_CHECK_DECODE("test_blue", string, "");
    WEE_CHECK_DECODE("test_?blue", string, "?");

    /* fg+bg color */
    snprintf (string, sizeof (string),
              "test_%syellow_blue",
              gui_color_get_custom ("yellow,blue"));
    WEE_CHECK_DECODE("test_yellow_blue", string, NULL);
    WEE_CHECK_DECODE("test_yellow_blue", string, "");
    WEE_CHECK_DECODE("test_?yellow_blue", string, "?");

    /* fg terminal color */
    snprintf (string, sizeof (string),
              "test_%s214",
              gui_color_get_custom ("214"));
    WEE_CHECK_DECODE("test_214", string, NULL);
    WEE_CHECK_DECODE("test_214", string, "");
    WEE_CHECK_DECODE("test_?214", string, "?");

    /* bg terminal color */
    snprintf (string, sizeof (string),
              "test_%s,214",
              gui_color_get_custom (",214"));
    WEE_CHECK_DECODE("test_,214", string, NULL);
    WEE_CHECK_DECODE("test_,214", string, "");
    WEE_CHECK_DECODE("test_?,214", string, "?");

    /* fg+bg terminal color */
    snprintf (string, sizeof (string),
              "test_%s227,240",
              gui_color_get_custom ("227,240"));
    WEE_CHECK_DECODE("test_227,240", string, NULL);
    WEE_CHECK_DECODE("test_227,240", string, "");
    WEE_CHECK_DECODE("test_?227,240", string, "?");

    /* fg terminal color + bg color */
    snprintf (string, sizeof (string),
              "test_%s227,blue",
              gui_color_get_custom ("227,blue"));
    WEE_CHECK_DECODE("test_227,blue", string, NULL);
    WEE_CHECK_DECODE("test_227,blue", string, "");
    WEE_CHECK_DECODE("test_?227,blue", string, "?");

    /* WeeChat color */
    snprintf (string, sizeof (string),
              "test_%soption_weechat.color.chat_host",
              GUI_COLOR(GUI_COLOR_CHAT_HOST));
    WEE_CHECK_DECODE("test_option_weechat.color.chat_host", string, NULL);
    WEE_CHECK_DECODE("test_option_weechat.color.chat_host", string, "");
    WEE_CHECK_DECODE("test_?option_weechat.color.chat_host", string, "?");
}

/*
 * Tests functions:
 *   gui_color_decode_ansi
 */

TEST(GuiColor, DecodeAnsi)
{
    char string[256], *decoded;

    /* NULL/empty string */
    POINTERS_EQUAL(NULL, gui_color_decode_ansi (NULL, 0));
    POINTERS_EQUAL(NULL, gui_color_decode_ansi (NULL, 1));
    WEE_CHECK_DECODE_ANSI("", "", 0);
    WEE_CHECK_DECODE_ANSI("", "", 1);

    /* no color codes */
    WEE_CHECK_DECODE_ANSI("test string", "test string", 0);
    WEE_CHECK_DECODE_ANSI("test string", "test string", 1);

    /* invalid ANSI color */
    WEE_CHECK_DECODE_ANSI("test_invalid", "test_\x1B[12zinvalid", 0);
    WEE_CHECK_DECODE_ANSI("test_invalid", "test_\x1B[12zinvalid", 1);

    /* reset */
    WEE_CHECK_DECODE_ANSI("test_reset", "test_\x1B[mreset", 0);
    snprintf (string, sizeof (string),
              "test_%sreset", gui_color_get_custom ("reset"));
    WEE_CHECK_DECODE_ANSI(string, "test_\x1B[mreset", 1);

    /* reset with 0 */
    WEE_CHECK_DECODE_ANSI("test_reset", "test_\x1B[0mreset", 0);
    snprintf (string, sizeof (string),
              "test_%sreset", gui_color_get_custom ("reset"));
    WEE_CHECK_DECODE_ANSI(string, "test_\x1B[0mreset", 1);

    /* blink */
    WEE_CHECK_DECODE_ANSI("test_blink_end", "test_\x1B[5mblink\x1B[25m_end", 0);
    snprintf (string, sizeof (string),
              "test_%sblink%s_end",
              gui_color_get_custom ("blink"),
              gui_color_get_custom ("-blink"));
    WEE_CHECK_DECODE_ANSI(string, "test_\x1B[5mblink\x1B[25m_end", 1);

    /* dim */
    WEE_CHECK_DECODE_ANSI("test_dim_end", "test_\x1B[2mdim\x1B[22m_end", 0);
    snprintf (string, sizeof (string),
              "test_%sdim%s_end",
              gui_color_get_custom ("dim"),
              gui_color_get_custom ("-dim"));
    WEE_CHECK_DECODE_ANSI(string, "test_\x1B[2mdim\x1B[22m_end", 1);

    /* bold */
    WEE_CHECK_DECODE_ANSI("test_bold_end", "test_\x1B[1mbold\x1B[21m_end", 0);
    snprintf (string, sizeof (string),
              "test_%sbold%s_end",
              gui_color_get_custom ("bold"),
              gui_color_get_custom ("-bold"));
    WEE_CHECK_DECODE_ANSI(string, "test_\x1B[1mbold\x1B[21m_end", 1);

    /* reverse */
    WEE_CHECK_DECODE_ANSI("test_reverse_end",
                          "test_\x1B[7mreverse\x1B[27m_end", 0);
    snprintf (string, sizeof (string),
              "test_%sreverse%s_end",
              gui_color_get_custom ("reverse"),
              gui_color_get_custom ("-reverse"));
    WEE_CHECK_DECODE_ANSI(string, "test_\x1B[7mreverse\x1B[27m_end", 1);

    /* italic */
    WEE_CHECK_DECODE_ANSI("test_italic_end",
                          "test_\x1B[3mitalic\x1B[23m_end", 0);
    snprintf (string, sizeof (string),
              "test_%sitalic%s_end",
              gui_color_get_custom ("italic"),
              gui_color_get_custom ("-italic"));
    WEE_CHECK_DECODE_ANSI(string, "test_\x1B[3mitalic\x1B[23m_end", 1);

    /* underline */
    WEE_CHECK_DECODE_ANSI("test_underline_end",
                          "test_\x1B[4munderline\x1B[24m_end", 0);
    snprintf (string, sizeof (string),
              "test_%sunderline%s_end",
              gui_color_get_custom ("underline"),
              gui_color_get_custom ("-underline"));
    WEE_CHECK_DECODE_ANSI(string, "test_\x1B[4munderline\x1B[24m_end", 1);

    /* default text color */
    WEE_CHECK_DECODE_ANSI("test_default", "test_\x1B[39mdefault", 0);
    snprintf (string, sizeof (string),
              "test_%sdefault",
              gui_color_get_custom ("default"));
    WEE_CHECK_DECODE_ANSI(string, "test_\x1B[39mdefault", 1);

    /* default text color */
    WEE_CHECK_DECODE_ANSI("test_bg_default", "test_\x1B[49mbg_default", 0);
    snprintf (string, sizeof (string),
              "test_%sbg_default",
              gui_color_get_custom (",default"));
    WEE_CHECK_DECODE_ANSI(string, "test_\x1B[49mbg_default", 1);

    /* text color */
    WEE_CHECK_DECODE_ANSI("test_blue", "test_\x1B[34mblue", 0);
    snprintf (string, sizeof (string),
              "test_%sblue",
              gui_color_get_custom ("|blue"));
    WEE_CHECK_DECODE_ANSI(string, "test_\x1B[34mblue", 1);

    /* bright text color */
    WEE_CHECK_DECODE_ANSI("test_lightgreen", "test_\x1B[92mlightgreen", 0);
    snprintf (string, sizeof (string),
              "test_%slightgreen",
              gui_color_get_custom ("|lightgreen"));
    WEE_CHECK_DECODE_ANSI(string, "test_\x1B[92mlightgreen", 1);

    /* text terminal color */
    WEE_CHECK_DECODE_ANSI("test_214", "test_\x1B[38;5;214m214", 0);
    snprintf (string, sizeof (string),
              "test_%s214",
              gui_color_get_custom ("|214"));
    WEE_CHECK_DECODE_ANSI(string, "test_\x1B[38;5;214m214", 1);

    /* text RGB color */
    WEE_CHECK_DECODE_ANSI("test_13", "test_\x1B[38;2;255;0;255m13", 0);
    snprintf (string, sizeof (string),
              "test_%s13",
              gui_color_get_custom ("|13"));
    WEE_CHECK_DECODE_ANSI(string, "test_\x1B[38;2;255;0;255m13", 1);

    /* background color */
    WEE_CHECK_DECODE_ANSI("test_bg_red", "test_\x1B[41mbg_red", 0);
    snprintf (string, sizeof (string),
              "test_%sbg_red",
              gui_color_get_custom ("|,red"));
    WEE_CHECK_DECODE_ANSI(string, "test_\x1B[41mbg_red", 1);

    /* bright background color */
    WEE_CHECK_DECODE_ANSI("test_bg_lightgreen",
                          "test_\x1B[102mbg_lightgreen", 0);
    snprintf (string, sizeof (string),
              "test_%sbg_lightgreen",
              gui_color_get_custom ("|,lightgreen"));
    WEE_CHECK_DECODE_ANSI(string, "test_\x1B[102mbg_lightgreen", 1);

    /* background terminal color */
    WEE_CHECK_DECODE_ANSI("test_bg_240", "test_\x1B[48;5;214mbg_240", 0);
    snprintf (string, sizeof (string),
              "test_%sbg_240",
              gui_color_get_custom ("|,240"));
    WEE_CHECK_DECODE_ANSI(string, "test_\x1B[48;5;240mbg_240", 1);

    /* background RGB color */
    WEE_CHECK_DECODE_ANSI("test_bg_13", "test_\x1B[48;2;255;0;255mbg_13", 0);
    snprintf (string, sizeof (string),
              "test_%sbg_13",
              gui_color_get_custom ("|,13"));
    WEE_CHECK_DECODE_ANSI(string, "test_\x1B[48;2;255;0;255mbg_13", 1);

    /* text RGB + background RGB color */
    WEE_CHECK_DECODE_ANSI(
        "test_fg_13_bg_04",
        "test_\x1B[38;2;255;0;255m\x1B[48;2;0;0;128mfg_13_bg_04",
        0);
    snprintf (string, sizeof (string),
              "test_%s%sfg_13_bg_04",
              gui_color_get_custom ("|13"),
              gui_color_get_custom ("|,04"));
    WEE_CHECK_DECODE_ANSI(
        string,
        "test_\x1B[38;2;255;0;255m\x1B[48;2;0;0;128mfg_13_bg_04",
        1);
}

/*
 * Tests functions:
 *   gui_color_encode_ansi
 */

TEST(GuiColor, EncodeAnsi)
{
    char string[256], *encoded;

    /* NULL/empty string */
    POINTERS_EQUAL(NULL, gui_color_encode_ansi (NULL));
    WEE_CHECK_ENCODE_ANSI("", "");

    /* reset */
    snprintf (string, sizeof (string),
              "test_%sreset", gui_color_get_custom ("reset"));
    WEE_CHECK_ENCODE_ANSI("test_\x1B[0mreset", string);

    /* blink */
    snprintf (string, sizeof (string),
              "test_%sblink%s_end",
              gui_color_get_custom ("blink"),
              gui_color_get_custom ("-blink"));
    WEE_CHECK_ENCODE_ANSI("test_\x1B[5mblink\x1B[25m_end", string);

    /* dim */
    snprintf (string, sizeof (string),
              "test_%sdim%s_end",
              gui_color_get_custom ("dim"),
              gui_color_get_custom ("-dim"));
    WEE_CHECK_ENCODE_ANSI("test_\x1B[2mdim\x1B[22m_end", string);

    /* bold */
    snprintf (string, sizeof (string),
              "test_%sbold%s_end",
              gui_color_get_custom ("bold"),
              gui_color_get_custom ("-bold"));
    WEE_CHECK_ENCODE_ANSI("test_\x1B[1mbold\x1B[21m_end", string);

    /* reverse */
    snprintf (string, sizeof (string),
              "test_%sreverse%s_end",
              gui_color_get_custom ("reverse"),
              gui_color_get_custom ("-reverse"));
    WEE_CHECK_ENCODE_ANSI("test_\x1B[7mreverse\x1B[27m_end", string);

    /* italic */
    snprintf (string, sizeof (string),
              "test_%sitalic%s_end",
              gui_color_get_custom ("italic"),
              gui_color_get_custom ("-italic"));
    WEE_CHECK_ENCODE_ANSI("test_\x1B[3mitalic\x1B[23m_end", string);

    /* underline */
    snprintf (string, sizeof (string),
              "test_%sunderline%s_end",
              gui_color_get_custom ("underline"),
              gui_color_get_custom ("-underline"));
    WEE_CHECK_ENCODE_ANSI("test_\x1B[4munderline\x1B[24m_end", string);

    /* text color */
    snprintf (string, sizeof (string),
              "test_%sblue",
              gui_color_get_custom ("blue"));
    WEE_CHECK_ENCODE_ANSI("test_\x1B[34mblue", string);

    /* bright text color */
    snprintf (string, sizeof (string),
              "test_%slightgreen",
              gui_color_get_custom ("lightgreen"));
    WEE_CHECK_ENCODE_ANSI("test_\x1B[92mlightgreen", string);

    /* text terminal color */
    snprintf (string, sizeof (string),
              "test_%s214",
              gui_color_get_custom ("214"));
    WEE_CHECK_ENCODE_ANSI("test_\x1B[38;5;214m214", string);

    /* background color */
    snprintf (string, sizeof (string),
              "test_%sbg_red",
              gui_color_get_custom (",red"));
    WEE_CHECK_ENCODE_ANSI("test_\x1B[41mbg_red", string);

    /* bright background color */
    snprintf (string, sizeof (string),
              "test_%sbg_lightgreen",
              gui_color_get_custom (",lightgreen"));
    WEE_CHECK_ENCODE_ANSI("test_\x1B[102mbg_lightgreen", string);

    /* background terminal color */
    snprintf (string, sizeof (string),
              "test_%sbg_240",
              gui_color_get_custom (",240"));
    WEE_CHECK_ENCODE_ANSI("test_\x1B[48;5;240mbg_240", string);

    /* WeeChat color */
    snprintf (string, sizeof (string),
              "test_%soption_weechat.color.chat_host",
              GUI_COLOR(GUI_COLOR_CHAT_HOST));
    WEE_CHECK_ENCODE_ANSI(
        "test_\x1B[0m\x1B[38;5;6m\x1B[49m"
        "option_weechat.color.chat_host",
        string);

    /* WeeChat bright color */
    snprintf (string, sizeof (string),
              "test_%soption_weechat.color.chat_nick",
              GUI_COLOR(GUI_COLOR_CHAT_NICK));
    WEE_CHECK_ENCODE_ANSI(
        "test_\x1B[0m\x1B[38;5;14m\x1B[49m"
        "option_weechat.color.chat_nick",
        string);

    /* WeeChat color with attributes */
    config_file_option_set (config_color_chat_host, "_green", 1);
    snprintf (string, sizeof (string),
              "test_%soption_weechat.color.chat_host",
              GUI_COLOR(GUI_COLOR_CHAT_HOST));
    WEE_CHECK_ENCODE_ANSI(
        "test_\x1B[0m\x1B[4m\x1B[38;5;2m\x1B[49m"
        "option_weechat.color.chat_host",
        string);
    config_file_option_reset (config_color_chat_host, 1);

    /* multiple colors/attributes */
    snprintf (string, sizeof (string),
              "%shello, %sthis is%s %sblink%s %sdim%s a test %sblue %sreset "
              "%syellow,red here!",
              gui_color_get_custom (",blue"),
              gui_color_get_custom ("bold"),
              gui_color_get_custom ("-bold"),
              gui_color_get_custom ("blink"),
              gui_color_get_custom ("-blink"),
              gui_color_get_custom ("dim"),
              gui_color_get_custom ("-dim"),
              gui_color_get_custom ("blue"),
              gui_color_get_custom ("reset"),
              gui_color_get_custom ("yellow,red"));
    WEE_CHECK_ENCODE_ANSI(
        "\x1B[44mhello, \x1B[1mthis is\x1B[21m \x1B[5mblink\x1B[25m "
        "\x1B[2mdim\x1B[22m a test \x1B[34mblue \x1B[0mreset "
        "\x1B[93m\x1B[41myellow,red here!",
        string);
}

/*
 * Tests functions:
 *   gui_color_emphasize
 */

TEST(GuiColor, Emphasize)
{
    char string1[256], string2[256], *emphasized;
    regex_t regex;

    /* NULL/empty string, search or regex */
    POINTERS_EQUAL(NULL, gui_color_emphasize (NULL, NULL, 0, NULL));
    POINTERS_EQUAL(NULL, gui_color_emphasize ("test", NULL, 0, NULL));
    POINTERS_EQUAL(NULL, gui_color_emphasize (NULL, "test", 0, NULL));

    /* build strings for tests */
    snprintf (string1, sizeof (string1),
              "%shello, %sthis is%s a test here!",
              gui_color_get_custom (",blue"),
              gui_color_get_custom ("bold"),
              gui_color_get_custom ("bold"));
    snprintf (string2, sizeof (string2),
              "%shello, %s%sthis is%s a test%s here!",
              gui_color_get_custom (",blue"),
              gui_color_get_custom ("emphasis"),
              gui_color_get_custom ("bold"),
              gui_color_get_custom ("bold"),
              gui_color_get_custom ("emphasis"));

    /* search string (found) */
    WEE_CHECK_EMPHASIZE(string2, string1, "this is a test", 0, NULL);
    WEE_CHECK_EMPHASIZE(string2, string1, "this IS A TesT", 0, NULL);

    /* search string (not found) */
    WEE_CHECK_EMPHASIZE(string1, string1, "this IS A TesT", 1, NULL);

    /* search regex (found) */
    string_regcomp (&regex, "this.*test", 0);
    WEE_CHECK_EMPHASIZE(string2, string1, NULL, 0, &regex);
    regfree (&regex);

    /* search regex (not found) */
    string_regcomp (&regex, "this.*failed", 0);
    WEE_CHECK_EMPHASIZE(string1, string1, NULL, 0, &regex);
    regfree (&regex);
}
