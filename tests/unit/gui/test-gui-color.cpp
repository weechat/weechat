/*
 * SPDX-FileCopyrightText: 2019-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test color functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <string.h>
#include "src/core/core-config.h"
#include "src/core/core-config-file.h"
#include "src/core/core-hook.h"
#include "src/plugins/weechat-plugin.h"
#include "src/core/core-string.h"
#include "src/gui/gui-color.h"

extern int gui_color_hash_seed_set;
extern int gui_color_term_colors;
extern int gui_color_term_direct_colors;
extern int gui_color_pairs_used;
extern int gui_color_get_pair (int fg, int bg);
extern void gui_color_pair_direct_key_get_colors (const void *key, int *fg,
                                                  int *bg);
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
 * Test functions:
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

    /* Only delimiter (no-op) */
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
 * Test functions:
 *   gui_color_convert_rgb_to_term
 *   gui_color_convert_rgb_to_term_display
 *   gui_color_convert_rgb_to_term_range
 */

TEST(GuiColor, ConvertRgbToTerm)
{
    LONGS_EQUAL(0, gui_color_convert_rgb_to_term (0x000000, 256));
    LONGS_EQUAL(8, gui_color_convert_rgb_to_term (0x808080, 256));
    LONGS_EQUAL(154, gui_color_convert_rgb_to_term (0xafff00, 256));
    LONGS_EQUAL(11, gui_color_convert_rgb_to_term (0xafff00, 64));

    /* Terminal with 88 colors: its own palette after the 16 basic colors. */
    LONGS_EQUAL(0x000000, gui_color_palette_rgb (16, 88));
    LONGS_EQUAL(0x8b0000, gui_color_palette_rgb (32, 88));
    LONGS_EQUAL(0xffffff, gui_color_palette_rgb (79, 88));
    LONGS_EQUAL(0x2e2e2e, gui_color_palette_rgb (80, 88));
    LONGS_EQUAL(0xe7e7e7, gui_color_palette_rgb (87, 88));
    LONGS_EQUAL(-1, gui_color_palette_rgb (88, 88));
    LONGS_EQUAL(gui_color_term256[52], gui_color_palette_rgb (52, 256));
    LONGS_EQUAL(81, gui_color_convert_rgb_to_term (0x5f5f5f, 88));
    LONGS_EQUAL(15, gui_color_convert_rgb_to_term (0xffffff, 88));
    LONGS_EQUAL(52, gui_color_convert_rgb_to_term (0xcd8b00, 88));
    LONGS_EQUAL(1, gui_color_convert_rgb_to_term (0x5f0000, 88));

    /* invalid limit */
    LONGS_EQUAL(0, gui_color_convert_rgb_to_term (0x808080, 0));
    LONGS_EQUAL(0, gui_color_convert_rgb_to_term (0x808080, -1));
    LONGS_EQUAL(8, gui_color_convert_rgb_to_term (0x808080, 257));
    LONGS_EQUAL(0, gui_color_convert_rgb_to_term (0x010203, 100000000));

    /* For display: colors 0-15 (theme of terminal) are not used. */
    LONGS_EQUAL(244, gui_color_convert_rgb_to_term_display (0x808080, 256));
    LONGS_EQUAL(196, gui_color_convert_rgb_to_term_display (0xff0000, 256));
    LONGS_EQUAL(79, gui_color_convert_rgb_to_term_display (0xffffff, 88));
    LONGS_EQUAL(8, gui_color_convert_rgb_to_term_display (0x808080, 16));
    LONGS_EQUAL(0, gui_color_convert_rgb_to_term_display (0x808080, 0));
    LONGS_EQUAL(8, gui_color_convert_rgb_to_term_range (0x808080, 300, 256));
}

/*
 * Test functions:
 *   gui_color_convert_term_to_direct
 */

TEST(GuiColor, ConvertTermToDirect)
{
    /* default color and basic ANSI colors: unchanged */
    LONGS_EQUAL(-1, gui_color_convert_term_to_direct (-1));
    LONGS_EQUAL(0, gui_color_convert_term_to_direct (0));
    LONGS_EQUAL(1, gui_color_convert_term_to_direct (1));
    LONGS_EQUAL(7, gui_color_convert_term_to_direct (7));

    /* out of range: unchanged */
    LONGS_EQUAL(-2, gui_color_convert_term_to_direct (-2));
    LONGS_EQUAL(256, gui_color_convert_term_to_direct (256));
    LONGS_EQUAL(0x123456, gui_color_convert_term_to_direct (0x123456));

    /* colors converted to RGB */
    LONGS_EQUAL(0x808080, gui_color_convert_term_to_direct (8));
    LONGS_EQUAL(0xff0000, gui_color_convert_term_to_direct (9));
    LONGS_EQUAL(0xffffff, gui_color_convert_term_to_direct (15));
    LONGS_EQUAL(0x00015f, gui_color_convert_term_to_direct (17));
    LONGS_EQUAL(0xff0000, gui_color_convert_term_to_direct (196));
    LONGS_EQUAL(0xeeeeee, gui_color_convert_term_to_direct (255));

    /*
     * RGB values < 256 (blue only, including black): green set to 1 so that
     * it is not a palette index ("xterm-direct256" uses indexes 0-255)
     */
    LONGS_EQUAL(0x000100, gui_color_convert_term_to_direct (16));
}

/*
 * Test functions:
 *   gui_color_rgb_parse
 *   gui_color_rgb_to_direct
 *   gui_color_rgb_to_number
 *   gui_color_rgb_from_number
 *   gui_color_rgb_to_term_cached
 *   gui_color_extended_string
 */

TEST(GuiColor, Rgb)
{
    int number1, number2;

    /* parse */
    LONGS_EQUAL(-1, gui_color_rgb_parse (NULL));
    LONGS_EQUAL(-1, gui_color_rgb_parse (""));
    LONGS_EQUAL(-1, gui_color_rgb_parse ("#"));
    LONGS_EQUAL(-1, gui_color_rgb_parse ("#12345"));
    LONGS_EQUAL(-1, gui_color_rgb_parse ("#12345g"));
    LONGS_EQUAL(-1, gui_color_rgb_parse ("123456"));
    LONGS_EQUAL(0x000000, gui_color_rgb_parse ("#000000"));
    LONGS_EQUAL(0xff8800, gui_color_rgb_parse ("#ff8800"));
    LONGS_EQUAL(0xff8800, gui_color_rgb_parse ("#FF8800"));
    LONGS_EQUAL(0xabcdef, gui_color_rgb_parse ("#abcdef_text"));

    /* RGB to direct color */
    LONGS_EQUAL(-1, gui_color_rgb_to_direct (-1));
    LONGS_EQUAL(0x000100, gui_color_rgb_to_direct (0x000000));
    LONGS_EQUAL(0x0001ff, gui_color_rgb_to_direct (0x0000ff));
    LONGS_EQUAL(0x000100, gui_color_rgb_to_direct (0x000100));
    LONGS_EQUAL(0xff8800, gui_color_rgb_to_direct (0xff8800));

    /* RGB to color number (interned) */
    number1 = gui_color_rgb_to_number (0x123456);
    CHECK(number1 >= GUI_COLOR_RGB_FIRST);
    CHECK(number1 <= GUI_COLOR_RGB_LAST);
    LONGS_EQUAL(number1, gui_color_rgb_to_number (0x123456));
    number2 = gui_color_rgb_to_number (0x654321);
    CHECK(number2 >= GUI_COLOR_RGB_FIRST);
    CHECK(number1 != number2);

    /* color number to RGB */
    LONGS_EQUAL(0x123456, gui_color_rgb_from_number (number1));
    LONGS_EQUAL(0x654321, gui_color_rgb_from_number (number2));
    LONGS_EQUAL(-1, gui_color_rgb_from_number (-1));
    LONGS_EQUAL(-1, gui_color_rgb_from_number (214));
    LONGS_EQUAL(-1, gui_color_rgb_from_number (GUI_COLOR_EXTENDED_MAX));
    LONGS_EQUAL(-1, gui_color_rgb_from_number (GUI_COLOR_RGB_LAST));

    /* nearest terminal color */
    LONGS_EQUAL(-1, gui_color_rgb_to_term_cached (214, 256));
    LONGS_EQUAL(gui_color_convert_rgb_to_term (0x123456, 256),
                gui_color_rgb_to_term_cached (number1, 256));
    LONGS_EQUAL(gui_color_convert_rgb_to_term (0x123456, 16),
                gui_color_rgb_to_term_cached (number1, 16));
    LONGS_EQUAL(gui_color_convert_rgb_to_term (0x123456, 256),
                gui_color_rgb_to_term_cached (number1, 256));

    /* string for an extended color */
    STRCMP_EQUAL("00000", gui_color_extended_string (0));
    STRCMP_EQUAL("00214", gui_color_extended_string (214));
    STRCMP_EQUAL("#123456", gui_color_extended_string (number1));
    STRCMP_EQUAL("#654321", gui_color_extended_string (number2));
}

/*
 * Test functions:
 *   gui_color_rgb_to_number
 *   gui_color_rgb_to_number_option
 *   gui_color_rgb_display_reset
 *   gui_color_hash_key_cb
 */

TEST(GuiColor, RgbTables)
{
    int number_option, number_display, i, number, key1, key2, old_term_colors;

    /* RGB color in option: number never changes, reused for display */
    number_option = gui_color_rgb_to_number_option (0x111111);
    CHECK(number_option >= GUI_COLOR_RGB_OPTIONS_FIRST);
    CHECK(number_option < GUI_COLOR_RGB_DISPLAY_FIRST);
    LONGS_EQUAL(number_option, gui_color_rgb_to_number_option (0x111111));
    LONGS_EQUAL(number_option, gui_color_rgb_to_number (0x111111));

    /* RGB color for display */
    number_display = gui_color_rgb_to_number (0x222222);
    CHECK(number_display >= GUI_COLOR_RGB_DISPLAY_FIRST);
    CHECK(number_display <= GUI_COLOR_RGB_LAST);
    LONGS_EQUAL(0x222222, gui_color_rgb_from_number (number_display));

    /* reset of RGB colors for display (options are kept) */
    gui_color_rgb_display_reset ();
    LONGS_EQUAL(-1, gui_color_rgb_from_number (number_display));
    LONGS_EQUAL(0x111111, gui_color_rgb_from_number (number_option));
    LONGS_EQUAL(0, gui_color_rgb_display.count);

    /* table for display full: nearest terminal color and reset requested */
    gui_color_rgb_display_reset_last = 0;
    number = -1;
    for (i = 0; i < GUI_COLOR_RGB_DISPLAY_MAX; i++)
    {
        number = gui_color_rgb_to_number (0x300000 + i);
    }
    LONGS_EQUAL(GUI_COLOR_RGB_LAST, number);
    LONGS_EQUAL(GUI_COLOR_RGB_DISPLAY_MAX, gui_color_rgb_display.count);
    LONGS_EQUAL(0, gui_color_rgb_display_reset_needed);
    LONGS_EQUAL(gui_color_convert_rgb_to_term_display (0xabc123,
                                                       gui_color_term_colors),
                gui_color_rgb_to_number (0xabc123));
    LONGS_EQUAL(1, gui_color_rgb_display_reset_needed);
    /*
     * terminal with 88 colors: nearest color of the 88-color palette, colors
     * 0-15 (theme of terminal) are not used
     */
    old_term_colors = gui_color_term_colors;
    gui_color_term_colors = 88;
    LONGS_EQUAL(32, gui_color_rgb_to_number (0x5f0000));
    LONGS_EQUAL(19, gui_color_rgb_to_number (0x0000ff));
    gui_color_term_colors = old_term_colors;
    gui_color_rgb_display_reset ();
    LONGS_EQUAL(0, gui_color_rgb_display_reset_needed);

    /* table for display full, automatic reset disabled: no reset */
    gui_color_rgb_display_reset_last = 0;
    for (i = 0; i < GUI_COLOR_RGB_DISPLAY_MAX; i++)
    {
        gui_color_rgb_to_number (0x300000 + i);
    }
    config_file_option_set (config_look_color_pairs_auto_reset, "-1", 1);
    LONGS_EQUAL(gui_color_convert_rgb_to_term_display (0xabc123, 256),
                gui_color_rgb_to_number (0xabc123));
    LONGS_EQUAL(0, gui_color_rgb_display_reset_needed);
    config_file_option_reset (config_look_color_pairs_auto_reset, 1);
    gui_color_rgb_display_reset ();

    /* hash of keys: same hash for same key (random seed set once) */
    key1 = 0x000abc;
    LONGS_EQUAL(gui_color_hash_key_cb (gui_color_rgb_options.index, &key1),
                gui_color_hash_key_cb (gui_color_rgb_options.index, &key1));
    LONGS_EQUAL(1, gui_color_hash_seed_set);

    /*
     * hash of keys: different hashes for keys which differ in high bits (the
     * low bits depend on the random seed, the mix of bits is tested with
     * function hashtable_hash_mix64)
     */
    key1 = 0x000abc;
    key2 = 0x100abc;
    CHECK(gui_color_hash_key_cb (gui_color_rgb_options.index, &key1)
          != gui_color_hash_key_cb (gui_color_rgb_options.index, &key2));
}

/*
 * Test functions:
 *   gui_color_extended_size
 *   gui_color_extended_parse
 */

TEST(GuiColor, Extended)
{
    int color;

    LONGS_EQUAL(0, gui_color_extended_size (NULL));
    LONGS_EQUAL(0, gui_color_extended_size (""));
    LONGS_EQUAL(0, gui_color_extended_size ("0021"));
    LONGS_EQUAL(0, gui_color_extended_size ("#12"));
    LONGS_EQUAL(5, gui_color_extended_size ("00214"));
    LONGS_EQUAL(5, gui_color_extended_size ("00214_text"));
    LONGS_EQUAL(5, gui_color_extended_size ("#1234"));
    LONGS_EQUAL(7, gui_color_extended_size ("#123456"));
    LONGS_EQUAL(7, gui_color_extended_size ("#123456_text"));

    LONGS_EQUAL(0, gui_color_extended_parse ("", &color));
    LONGS_EQUAL(-1, color);
    LONGS_EQUAL(0, gui_color_extended_parse ("#12", &color));
    LONGS_EQUAL(-1, color);
    LONGS_EQUAL(5, gui_color_extended_parse ("abcde", &color));
    LONGS_EQUAL(-1, color);
    LONGS_EQUAL(5, gui_color_extended_parse ("00214", &color));
    LONGS_EQUAL(214, color);
    LONGS_EQUAL(7, gui_color_extended_parse ("#123456", &color));
    LONGS_EQUAL(gui_color_rgb_to_number (0x123456), color);
}

/*
 * Test functions:
 *   gui_color_get_custom (RGB colors)
 *   gui_color_assign (RGB colors)
 *   gui_color_get_name (RGB colors)
 */

TEST(GuiColor, GetCustomAssignRgb)
{
    char string[64];
    int color, count, old_max;

    /* foreground */
    snprintf (string, sizeof (string), "%c%c%c#ff8800",
              GUI_COLOR_COLOR_CHAR, GUI_COLOR_FG_CHAR,
              GUI_COLOR_EXTENDED_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("#ff8800"));
    STRCMP_EQUAL(string, gui_color_get_custom ("#FF8800"));

    /* foreground with attributes */
    snprintf (string, sizeof (string), "%c%c%c*_#ff8800",
              GUI_COLOR_COLOR_CHAR, GUI_COLOR_FG_CHAR,
              GUI_COLOR_EXTENDED_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("*_#ff8800"));

    /* background */
    snprintf (string, sizeof (string), "%c%c%c#000080",
              GUI_COLOR_COLOR_CHAR, GUI_COLOR_BG_CHAR,
              GUI_COLOR_EXTENDED_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom (",#000080"));

    /* foreground + background (RGB and mixed) */
    snprintf (string, sizeof (string), "%c%c%c#ff8800~%c#000080",
              GUI_COLOR_COLOR_CHAR, GUI_COLOR_FG_BG_CHAR,
              GUI_COLOR_EXTENDED_CHAR, GUI_COLOR_EXTENDED_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("#ff8800,#000080"));
    snprintf (string, sizeof (string), "%c%c%c00214~%c#000080",
              GUI_COLOR_COLOR_CHAR, GUI_COLOR_FG_BG_CHAR,
              GUI_COLOR_EXTENDED_CHAR, GUI_COLOR_EXTENDED_CHAR);
    STRCMP_EQUAL(string, gui_color_get_custom ("214,#000080"));

    /* invalid RGB colors */
    STRCMP_EQUAL("", gui_color_get_custom ("#ff88"));
    STRCMP_EQUAL("", gui_color_get_custom ("#ff880000"));

    /* assign / get name */
    color = 0;
    LONGS_EQUAL(1, gui_color_assign (&color, "#ff8800"));
    CHECK(color & GUI_COLOR_EXTENDED_FLAG);
    LONGS_EQUAL(0xff8800,
                gui_color_rgb_from_number (color & GUI_COLOR_EXTENDED_MASK));
    STRCMP_EQUAL("#ff8800", gui_color_get_name (color));
    LONGS_EQUAL(1, gui_color_assign (&color, "*_#FF8800"));
    CHECK(color & GUI_COLOR_EXTENDED_BOLD_FLAG);
    CHECK(color & GUI_COLOR_EXTENDED_UNDERLINE_FLAG);
    STRCMP_EQUAL("*_#ff8800", gui_color_get_name (color));
    LONGS_EQUAL(0, gui_color_assign (&color, "#ff88"));
    LONGS_EQUAL(0, gui_color_assign (&color, "#ff880000"));

    /* RGB colors in strings are not added to the table of RGB colors */
    count = gui_color_rgb_display.count;
    gui_color_get_custom ("#abcdef,#fedcba");
    LONGS_EQUAL(count, gui_color_rgb_display.count);

    /* RGB colors in options: table of options */
    LONGS_EQUAL(1, gui_color_assign (&color, "#123abc"));
    CHECK((color & GUI_COLOR_EXTENDED_MASK) >= GUI_COLOR_RGB_OPTIONS_FIRST);
    CHECK((color & GUI_COLOR_EXTENDED_MASK) < GUI_COLOR_RGB_DISPLAY_FIRST);

    /* table of options full: error */
    old_max = gui_color_rgb_options.max;
    gui_color_rgb_options.max = gui_color_rgb_options.count;
    LONGS_EQUAL(1, gui_color_assign (&color, "#123abc"));
    LONGS_EQUAL(0, gui_color_assign (&color, "#cba321"));
    gui_color_rgb_options.max = old_max;
}

/*
 * Test functions:
 *   gui_color_get_pair (RGB colors)
 */

TEST(GuiColor, GetPairRgb)
{
    int old_direct_colors, number1, number2, pairs_used;
    int pair1, pair2, pair3;

    old_direct_colors = gui_color_term_direct_colors;

    /* terminal with direct colors: one pair by fg/bg */
    gui_color_reset_pairs ();
    number1 = gui_color_rgb_to_number (0x123456);
    number2 = gui_color_rgb_to_number (0x654321);
    gui_color_term_direct_colors = 1;
    pairs_used = gui_color_pairs_used;
    pair1 = gui_color_get_pair (number1, -1);
    CHECK(pair1 > 0);
    LONGS_EQUAL(pairs_used + 1, gui_color_pairs_used);
    LONGS_EQUAL(pair1, gui_color_get_pair (number1, -1));
    LONGS_EQUAL(pairs_used + 1, gui_color_pairs_used);
    pair2 = gui_color_get_pair (number1, number2);
    CHECK((pair2 > 0) && (pair2 != pair1));
    pair3 = gui_color_get_pair (214, number2);
    CHECK((pair3 > 0) && (pair3 != pair1) && (pair3 != pair2));
    LONGS_EQUAL(pairs_used + 3, gui_color_pairs_used);

    /* terminal without direct colors: nearest terminal color */
    gui_color_reset_pairs ();
    number1 = gui_color_rgb_to_number (0x123456);
    gui_color_term_direct_colors = 0;
    LONGS_EQUAL(
        gui_color_get_pair (
            gui_color_convert_rgb_to_term_display (0x123456,
                                                   gui_color_term_colors),
            -1),
        gui_color_get_pair (number1, -1));

    gui_color_term_direct_colors = old_direct_colors;
    gui_color_reset_pairs ();
}

int test_gui_color_dump_lines_direct = 0;
int test_gui_color_dump_last_line_empty = 0;

/*
 * Callback of print hook used in test GuiColor.DumpPairsDirect: reset color
 * pairs while pairs are displayed.
 */

int
test_gui_color_dump_print_cb (const void *pointer, void *data,
                              struct t_gui_buffer *buffer,
                              time_t date, int date_usec,
                              int tags_count, const char **tags,
                              int displayed, int highlight,
                              const char *prefix, const char *message)
{
    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) buffer;
    (void) date;
    (void) date_usec;
    (void) tags_count;
    (void) tags;
    (void) displayed;
    (void) highlight;
    (void) prefix;

    test_gui_color_dump_last_line_empty = (message && !message[0]) ? 1 : 0;

    if (message && strstr (message, "direct fg:"))
    {
        test_gui_color_dump_lines_direct++;
        gui_color_reset_pairs ();
    }

    return WEECHAT_RC_OK;
}

/*
 * Test functions:
 *   gui_color_dump
 */

TEST(GuiColor, DumpPairsDirect)
{
    struct t_hook *hook;
    int old_direct_colors;

    old_direct_colors = gui_color_term_direct_colors;

    gui_color_reset_pairs ();
    gui_color_term_direct_colors = 1;
    CHECK(gui_color_get_pair (gui_color_rgb_to_number (0x123456), -1) > 0);
    CHECK(gui_color_get_pair (gui_color_rgb_to_number (0x654321), -1) > 0);
    CHECK(gui_color_get_pair (gui_color_rgb_to_number (0xabcdef), -1) > 0);

    /* A print hook resetting pairs does not change the pairs displayed. */
    hook = hook_print (NULL, NULL, NULL, NULL, 1,
                       &test_gui_color_dump_print_cb, NULL, NULL);
    CHECK(hook);
    test_gui_color_dump_lines_direct = 0;
    gui_color_dump ();
    LONGS_EQUAL(3, test_gui_color_dump_lines_direct);
    /* No empty line after the pairs. */
    LONGS_EQUAL(0, test_gui_color_dump_last_line_empty);
    unhook (hook);

    gui_color_term_direct_colors = old_direct_colors;
    gui_color_reset_pairs ();
}

/*
 * Test functions:
 *   gui_color_pair_direct_key_get_colors
 */

TEST(GuiColor, PairDirectKey)
{
    long long key;
    int fg, bg;

    key = (((long long)(0xff8800 + 1)) << 32) | (long long)(-1 + 1);
    gui_color_pair_direct_key_get_colors (&key, &fg, &bg);
    LONGS_EQUAL(0xff8800, fg);
    LONGS_EQUAL(-1, bg);

    key = (((long long)(-1 + 1)) << 32) | (long long)(0xffffff + 1);
    gui_color_pair_direct_key_get_colors (&key, &fg, &bg);
    LONGS_EQUAL(-1, fg);
    LONGS_EQUAL(0xffffff, bg);
}

/*
 * Test functions:
 *   config_weechat_palette_create_option_cb (number of palette color)
 */

TEST(GuiColor, PaletteNumber)
{
    struct t_config_option *ptr_option;

    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_with_string ("weechat.palette.100000",
                                                    "orange"));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_with_string ("weechat.palette.-1",
                                                    "orange"));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_with_string ("weechat.palette.99999",
                                                    "test_palette_max"));
    LONGS_EQUAL(99999, gui_color_palette_get_alias ("test_palette_max"));
    config_file_search_with_string ("weechat.palette.99999", NULL, NULL,
                                    &ptr_option, NULL);
    CHECK(ptr_option);
    config_file_option_unset (ptr_option);
    LONGS_EQUAL(-1, gui_color_palette_get_alias ("test_palette_max"));
}

/*
 * Test functions:
 *   gui_color_code_size
 */

TEST(GuiColor, CodeSize)
{
    char string[256];

    /* NULL/empty string */
    LONGS_EQUAL(0, gui_color_code_size (NULL));
    LONGS_EQUAL(0, gui_color_code_size (""));

    /* No color code */
    LONGS_EQUAL(0, gui_color_code_size ("test"));

    /*
     * Set/remove attribute: the next char is part of the code only if it is
     * an attribute, like on screen.
     */
    LONGS_EQUAL(2, gui_color_code_size ("\x1A" "\x01" "x"));
    LONGS_EQUAL(2, gui_color_code_size ("\x1B" "\x06" "x"));
    LONGS_EQUAL(1, gui_color_code_size ("\x1A" "x"));
    LONGS_EQUAL(1, gui_color_code_size ("\x1B" "\x07"));
    LONGS_EQUAL(1, gui_color_code_size ("\x1A"));

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

    /* RGB colors */
    LONGS_EQUAL(10, gui_color_code_size ("\x19" "F@#ff8800"));
    LONGS_EQUAL(11, gui_color_code_size ("\x19" "F@*#ff8800"));
    LONGS_EQUAL(10, gui_color_code_size ("\x19" "B@#000080"));
    LONGS_EQUAL(19,
                gui_color_code_size ("\x19" "*@#ff8800~@#000080"));
}

/*
 * Test functions:
 *   gui_color_decode
 */

TEST(GuiColor, Decode)
{
    char string[256], *decoded;

    /* NULL/empty string */
    STRCMP_EQUAL(NULL, gui_color_decode (NULL, NULL));
    STRCMP_EQUAL(NULL, gui_color_decode (NULL, ""));
    STRCMP_EQUAL(NULL, gui_color_decode (NULL, "?"));
    WEE_CHECK_DECODE("", "", NULL);
    WEE_CHECK_DECODE("", "", "");
    WEE_CHECK_DECODE("", "", "?");

    /* No color codes */
    WEE_CHECK_DECODE("test string", "test string", NULL);
    WEE_CHECK_DECODE("test string", "test string", "");
    WEE_CHECK_DECODE("test string", "test string", "?");

    /* Set/remove attribute: the char after it is kept if not an attribute. */
    WEE_CHECK_DECODE("ab", "a" "\x1A" "\x01" "b", NULL);
    WEE_CHECK_DECODE("axb", "a" "\x1A" "xb", NULL);
    WEE_CHECK_DECODE("a?xb", "a" "\x1B" "xb", "?");

    /* reset */
    snprintf (string, sizeof (string),
              "test_" "%s" "reset",
              gui_color_get_custom ("reset"));
    WEE_CHECK_DECODE("test_reset", string, NULL);
    WEE_CHECK_DECODE("test_reset", string, "");
    WEE_CHECK_DECODE("test_[color]reset", string, "[color]");

    /* resetcolor */
    snprintf (string, sizeof (string),
              "test_" "%s" "resetcolor",
              gui_color_get_custom ("resetcolor"));
    WEE_CHECK_DECODE("test_resetcolor", string, NULL);
    WEE_CHECK_DECODE("test_resetcolor", string, "");
    WEE_CHECK_DECODE("test_[color]resetcolor", string, "[color]");

    /* emphasis */
    snprintf (string, sizeof (string),
              "test_" "%s" "emphasis",
              gui_color_get_custom ("emphasis"));
    WEE_CHECK_DECODE("test_emphasis", string, NULL);
    WEE_CHECK_DECODE("test_emphasis", string, "");
    WEE_CHECK_DECODE("test_[color]emphasis", string, "[color]");

    /* blink */
    snprintf (string, sizeof (string),
              "test_" "%s" "blink%s_end",
              gui_color_get_custom ("blink"),
              gui_color_get_custom ("-blink"));
    WEE_CHECK_DECODE("test_blink_end", string, NULL);
    WEE_CHECK_DECODE("test_blink_end", string, "");
    WEE_CHECK_DECODE("test_[color]blink[color]_end", string, "[color]");

    /* dim */
    snprintf (string, sizeof (string),
              "test_" "%s" "dim" "%s" "_end",
              gui_color_get_custom ("dim"),
              gui_color_get_custom ("-dim"));
    WEE_CHECK_DECODE("test_dim_end", string, NULL);
    WEE_CHECK_DECODE("test_dim_end", string, "");
    WEE_CHECK_DECODE("test_[color]dim[color]_end", string, "[color]");

    /* bold */
    snprintf (string, sizeof (string),
              "test_" "%s" "bold" "%s" "_end",
              gui_color_get_custom ("bold"),
              gui_color_get_custom ("-bold"));
    WEE_CHECK_DECODE("test_bold_end", string, NULL);
    WEE_CHECK_DECODE("test_bold_end", string, "");
    WEE_CHECK_DECODE("test_[color]bold[color]_end", string, "[color]");

    /* reverse */
    snprintf (string, sizeof (string),
              "test_" "%s" "reverse" "%s" "_end",
              gui_color_get_custom ("reverse"),
              gui_color_get_custom ("-reverse"));
    WEE_CHECK_DECODE("test_reverse_end", string, NULL);
    WEE_CHECK_DECODE("test_reverse_end", string, "");
    WEE_CHECK_DECODE("test_[color]reverse[color]_end", string, "[color]");

    /* italic */
    snprintf (string, sizeof (string),
              "test_" "%s" "italic" "%s" "_end",
              gui_color_get_custom ("italic"),
              gui_color_get_custom ("-italic"));
    WEE_CHECK_DECODE("test_italic_end", string, NULL);
    WEE_CHECK_DECODE("test_italic_end", string, "");
    WEE_CHECK_DECODE("test_[color]italic[color]_end", string, "[color]");

    /* underline */
    snprintf (string, sizeof (string),
              "test_" "%s" "underline" "%s" "_end",
              gui_color_get_custom ("underline"),
              gui_color_get_custom ("-underline"));
    WEE_CHECK_DECODE("test_underline_end", string, NULL);
    WEE_CHECK_DECODE("test_underline_end", string, "");
    WEE_CHECK_DECODE("test_[color]underline[color]_end", string, "[color]");

    /* bar_fg */
    snprintf (string, sizeof (string),
              "test_" "%s" "bar_fg",
              gui_color_get_custom ("bar_fg"));
    WEE_CHECK_DECODE("test_bar_fg", string, NULL);
    WEE_CHECK_DECODE("test_bar_fg", string, "");
    WEE_CHECK_DECODE("test_[color]bar_fg", string, "[color]");

    /* bar_delim */
    snprintf (string, sizeof (string),
              "test_" "%s" "bar_delim",
              gui_color_get_custom ("bar_delim"));
    WEE_CHECK_DECODE("test_bar_delim", string, NULL);
    WEE_CHECK_DECODE("test_bar_delim", string, "");
    WEE_CHECK_DECODE("test_[color]bar_delim", string, "[color]");

    /* bar_bg */
    snprintf (string, sizeof (string),
              "test_" "%s" "bar_bg",
              gui_color_get_custom ("bar_bg"));
    WEE_CHECK_DECODE("test_bar_bg", string, NULL);
    WEE_CHECK_DECODE("test_bar_bg", string, "");
    WEE_CHECK_DECODE("test_[color]bar_bg", string, "[color]");

    /* fg color */
    snprintf (string, sizeof (string),
              "test_" "%s" "blue",
              gui_color_get_custom ("blue"));
    WEE_CHECK_DECODE("test_blue", string, NULL);
    WEE_CHECK_DECODE("test_blue", string, "");
    WEE_CHECK_DECODE("test_[color]blue", string, "[color]");

    /* bg color */
    snprintf (string, sizeof (string),
              "test_" "%s" "blue",
              gui_color_get_custom (",blue"));
    WEE_CHECK_DECODE("test_blue", string, NULL);
    WEE_CHECK_DECODE("test_blue", string, "");
    WEE_CHECK_DECODE("test_[color]blue", string, "[color]");

    /* fg+bg color */
    snprintf (string, sizeof (string),
              "test_" "%s" "yellow_blue",
              gui_color_get_custom ("yellow,blue"));
    WEE_CHECK_DECODE("test_yellow_blue", string, NULL);
    WEE_CHECK_DECODE("test_yellow_blue", string, "");
    WEE_CHECK_DECODE("test_[color]yellow_blue", string, "[color]");

    /* fg terminal color */
    snprintf (string, sizeof (string),
              "test_" "%s" "214",
              gui_color_get_custom ("214"));
    WEE_CHECK_DECODE("test_214", string, NULL);
    WEE_CHECK_DECODE("test_214", string, "");
    WEE_CHECK_DECODE("test_[color]214", string, "[color]");

    /* bg terminal color */
    snprintf (string, sizeof (string),
              "test_" "%s" ",214",
              gui_color_get_custom (",214"));
    WEE_CHECK_DECODE("test_,214", string, NULL);
    WEE_CHECK_DECODE("test_,214", string, "");
    WEE_CHECK_DECODE("test_[color],214", string, "[color]");

    /* fg+bg terminal color */
    snprintf (string, sizeof (string),
              "test_" "%s" "227,240",
              gui_color_get_custom ("227,240"));
    WEE_CHECK_DECODE("test_227,240", string, NULL);
    WEE_CHECK_DECODE("test_227,240", string, "");
    WEE_CHECK_DECODE("test_[color]227,240", string, "[color]");

    /* fg terminal color + bg color */
    snprintf (string, sizeof (string),
              "test_" "%s" "227,blue",
              gui_color_get_custom ("227,blue"));
    WEE_CHECK_DECODE("test_227,blue", string, NULL);
    WEE_CHECK_DECODE("test_227,blue", string, "");
    WEE_CHECK_DECODE("test_[color]227,blue", string, "[color]");

    /* WeeChat color */
    snprintf (string, sizeof (string),
              "test_" "%s" "option_weechat.color.chat_host",
              GUI_COLOR(GUI_COLOR_CHAT_HOST));
    WEE_CHECK_DECODE("test_option_weechat.color.chat_host", string, NULL);
    WEE_CHECK_DECODE("test_option_weechat.color.chat_host", string, "");
    WEE_CHECK_DECODE("test_[color]option_weechat.color.chat_host", string, "[color]");

    /* RGB colors */
    snprintf (string, sizeof (string),
              "test_%sfg%sbg%sfg_bg_end",
              "\x19" "F@*#ff8800",
              "\x19" "B@#000080",
              "\x19" "*@#ff8800~@#000080");
    WEE_CHECK_DECODE("test_fgbgfg_bg_end", string, NULL);
    WEE_CHECK_DECODE("test_[C]fg[C]bg[C]fg_bg_end", string, "[C]");
}

/*
 * Test functions:
 *   gui_color_downgrade_rgb_term
 *   gui_color_downgrade_rgb
 */

TEST(GuiColor, DowngradeRgb)
{
    char string[256], expected[256], str_fg[32], str_fg_bg[32], str_bg[32];
    char *str, *result;
    int fg, bg, i, rgb;

    /* Like on screen, colors 0-15 (theme of terminal) are not used. */
    LONGS_EQUAL(196, gui_color_downgrade_rgb_term (0xff0000));
    LONGS_EQUAL(244, gui_color_downgrade_rgb_term (0x808080));

    /* Cached conversion: same result, also for colors in the same slot. */
    for (i = 0; i < 300; i++)
    {
        rgb = (i * 0x10305) & 0xFFFFFF;
        LONGS_EQUAL(gui_color_convert_rgb_to_term_display (rgb, 256),
                    gui_color_downgrade_rgb_term (rgb));
        LONGS_EQUAL(gui_color_convert_rgb_to_term_display (rgb, 256),
                    gui_color_downgrade_rgb_term (rgb));
    }

    POINTERS_EQUAL(NULL, gui_color_downgrade_rgb (NULL));
    WEE_TEST_STR("", gui_color_downgrade_rgb (""));
    WEE_TEST_STR("test", gui_color_downgrade_rgb ("test"));

    /* RGB color in text (not in a color code): unchanged */
    WEE_TEST_STR("text @#ff8800", gui_color_downgrade_rgb ("text @#ff8800"));

    /* colors without RGB: unchanged */
    snprintf (string, sizeof (string), "%sa%sb%sc",
              gui_color_get_custom ("*214"),
              gui_color_get_custom ("214,blue"),
              gui_color_get_custom ("bold"));
    WEE_TEST_STR(string, gui_color_downgrade_rgb (string));

    /* RGB colors replaced by nearest terminal colors */
    fg = gui_color_convert_rgb_to_term_display (0xff8800, 256);
    bg = gui_color_convert_rgb_to_term_display (0x000080, 256);
    snprintf (string, sizeof (string), "%sa%sb%sc @#ff8800",
              "\x19" "F@*#ff8800",
              "\x19" "*@#ff8800~@#000080",
              "\x19" "B@#000080");
    snprintf (str_fg, sizeof (str_fg), "*%d", fg);
    snprintf (str_fg_bg, sizeof (str_fg_bg), "%d,%d", fg, bg);
    snprintf (str_bg, sizeof (str_bg), ",%d", bg);
    snprintf (expected, sizeof (expected), "%sa%sb%sc @#ff8800",
              gui_color_get_custom (str_fg),
              gui_color_get_custom (str_fg_bg),
              gui_color_get_custom (str_bg));
    WEE_TEST_STR(expected, gui_color_downgrade_rgb (string));

    /*
     * RGB color after an attribute code: the next char is part of the
     * attribute code only if it is an attribute.
     */
    snprintf (string, sizeof (string), "%s%sa%s%sb",
              "\x1B", "\x19" "F@#ff8800", "\x1A\x01", "\x19" "F@#ff8800");
    snprintf (expected, sizeof (expected), "%s%s%05da%s%s%05db",
              "\x1B", "\x19" "F@", fg, "\x1A\x01", "\x19" "F@", fg);
    WEE_TEST_STR(expected, gui_color_downgrade_rgb (string));

    /* Many color codes after a RGB color (linear time). */
    str = (char *)malloc (10 + (8 * 20000) + 1);
    CHECK(str);
    memcpy (str, "\x19" "F@#ff8800", 10);
    for (i = 0; i < 20000; i++)
        memcpy (str + 10 + (8 * i), "\x19" "F@00214", 8);
    str[10 + (8 * 20000)] = '\0';
    result = gui_color_downgrade_rgb (str);
    CHECK(result);
    LONGS_EQUAL(8 + (8 * 20000), strlen (result));
    free (result);
    free (str);
}

/*
 * Test functions:
 *   gui_color_downgrade_rgb_name
 */

TEST(GuiColor, DowngradeRgbName)
{
    char *str, expected[64];
    int fg, bg;

    fg = gui_color_convert_rgb_to_term_display (0xff8800, 256);
    bg = gui_color_convert_rgb_to_term_display (0x000080, 256);

    POINTERS_EQUAL(NULL, gui_color_downgrade_rgb_name (NULL));
    WEE_TEST_STR("", gui_color_downgrade_rgb_name (""));
    WEE_TEST_STR("red", gui_color_downgrade_rgb_name ("red"));
    WEE_TEST_STR("*214,blue", gui_color_downgrade_rgb_name ("*214,blue"));
    WEE_TEST_STR("#ff88", gui_color_downgrade_rgb_name ("#ff88"));
    snprintf (expected, sizeof (expected), "%d", fg);
    WEE_TEST_STR(expected, gui_color_downgrade_rgb_name ("#ff8800"));
    snprintf (expected, sizeof (expected), "*_%d:%d", fg, bg);
    WEE_TEST_STR(expected, gui_color_downgrade_rgb_name ("*_#FF8800:#000080"));
    snprintf (expected, sizeof (expected), "%d,red", fg);
    WEE_TEST_STR(expected, gui_color_downgrade_rgb_name ("#ff8800,red"));
}

/*
 * Test functions:
 *   modifier "color_downgrade_rgb"
 */

TEST(GuiColor, ModifierDowngradeRgb)
{
    char *str, string[64], expected[64];
    int fg;

    fg = gui_color_convert_rgb_to_term_display (0xff8800, 256);

    /* WeeChat color codes */
    snprintf (string, sizeof (string), "%s" "test #ff8800", "\x19" "F@|#ff8800");
    snprintf (expected, sizeof (expected), "%s%05d" "test #ff8800",
              "\x19" "F@|", fg);
    WEE_TEST_STR(expected,
                 hook_modifier_exec (NULL, "color_downgrade_rgb", NULL, string));
    WEE_TEST_STR(expected,
                 hook_modifier_exec (NULL, "color_downgrade_rgb", "", string));

    /* color name */
    snprintf (expected, sizeof (expected), "*%d", fg);
    WEE_TEST_STR(expected,
                 hook_modifier_exec (NULL, "color_downgrade_rgb", "name",
                                     "*#ff8800"));
}

/*
 * Test functions:
 *   gui_color_decode_ansi
 */

TEST(GuiColor, DecodeAnsi)
{
    char string[256], *decoded;

    /* NULL/empty string */
    STRCMP_EQUAL(NULL, gui_color_decode_ansi (NULL, 0));
    STRCMP_EQUAL(NULL, gui_color_decode_ansi (NULL, 1));
    WEE_CHECK_DECODE_ANSI("", "", 0);
    WEE_CHECK_DECODE_ANSI("", "", 1);

    /* No color codes */
    WEE_CHECK_DECODE_ANSI("test string", "test string", 0);
    WEE_CHECK_DECODE_ANSI("test string", "test string", 1);

    /* Invalid ANSI color */
    WEE_CHECK_DECODE_ANSI("test_invalid", "test_" "\x1B[12z" "invalid", 0);
    WEE_CHECK_DECODE_ANSI("test_invalid", "test_\x1B[12z" "invalid", 1);

    /* reset */
    WEE_CHECK_DECODE_ANSI("test_reset", "test_" "\x1B[m" "reset", 0);
    snprintf (string, sizeof (string),
              "test_" "%s" "reset", gui_color_get_custom ("reset"));
    WEE_CHECK_DECODE_ANSI(string, "test_" "\x1B[m" "reset", 1);

    /* reset with 0 */
    WEE_CHECK_DECODE_ANSI("test_reset", "test_" "\x1B[0m" "reset", 0);
    snprintf (string, sizeof (string),
              "test_" "%s" "reset", gui_color_get_custom ("reset"));
    WEE_CHECK_DECODE_ANSI(string, "test_" "\x1B[0m" "reset", 1);

    /* blink */
    WEE_CHECK_DECODE_ANSI("test_blink_end", "test_" "\x1B[5m" "blink" "\x1B[25m" "_end", 0);
    snprintf (string, sizeof (string),
              "test_" "%s" "blink" "%s" "_end",
              gui_color_get_custom ("blink"),
              gui_color_get_custom ("-blink"));
    WEE_CHECK_DECODE_ANSI(string, "test_" "\x1B[5m" "blink" "\x1B[25m" "_end", 1);

    /* dim */
    WEE_CHECK_DECODE_ANSI("test_dim_end", "test_" "\x1B[2m" "dim" "\x1B[22m" "_end", 0);
    snprintf (string, sizeof (string),
              "test_" "%s" "dim" "%s" "_end",
              gui_color_get_custom ("dim"),
              gui_color_get_custom ("-dim"));
    WEE_CHECK_DECODE_ANSI(string, "test_" "\x1B[2m" "dim" "\x1B[22m" "_end", 1);

    /* bold */
    WEE_CHECK_DECODE_ANSI("test_bold_end", "test_" "\x1B[1m" "bold" "\x1B[21m" "_end", 0);
    snprintf (string, sizeof (string),
              "test_" "%s" "bold" "%s" "_end",
              gui_color_get_custom ("bold"),
              gui_color_get_custom ("-bold"));
    WEE_CHECK_DECODE_ANSI(string, "test_" "\x1B[1m" "bold" "\x1B[21m" "_end", 1);

    /* reverse */
    WEE_CHECK_DECODE_ANSI("test_reverse_end",
                          "test_" "\x1B[7m" "reverse" "\x1B[27m" "_end", 0);
    snprintf (string, sizeof (string),
              "test_" "%s" "reverse" "%s" "_end",
              gui_color_get_custom ("reverse"),
              gui_color_get_custom ("-reverse"));
    WEE_CHECK_DECODE_ANSI(string, "test_" "\x1B[7m" "reverse" "\x1B[27m" "_end", 1);

    /* italic */
    WEE_CHECK_DECODE_ANSI("test_italic_end",
                          "test_" "\x1B[3m" "italic" "\x1B[23m" "_end", 0);
    snprintf (string, sizeof (string),
              "test_" "%s" "italic" "%s" "_end",
              gui_color_get_custom ("italic"),
              gui_color_get_custom ("-italic"));
    WEE_CHECK_DECODE_ANSI(string, "test_" "\x1B[3m" "italic" "\x1B[23m" "_end", 1);

    /* underline */
    WEE_CHECK_DECODE_ANSI("test_underline_end",
                          "test_" "\x1B[4m" "underline" "\x1B[24m" "_end", 0);
    snprintf (string, sizeof (string),
              "test_" "%s" "underline" "%s" "_end",
              gui_color_get_custom ("underline"),
              gui_color_get_custom ("-underline"));
    WEE_CHECK_DECODE_ANSI(string, "test_" "\x1B[4m" "underline" "\x1B[24m" "_end", 1);

    /* Default text color */
    WEE_CHECK_DECODE_ANSI("test_default", "test_" "\x1B[39m" "default", 0);
    snprintf (string, sizeof (string),
              "test_" "%s" "default",
              gui_color_get_custom ("default"));
    WEE_CHECK_DECODE_ANSI(string, "test_" "\x1B[39m" "default", 1);

    /* Default text color */
    WEE_CHECK_DECODE_ANSI("test_bg_default", "test_" "\x1B[49m" "bg_default", 0);
    snprintf (string, sizeof (string),
              "test_" "%s" "bg_default",
              gui_color_get_custom (",default"));
    WEE_CHECK_DECODE_ANSI(string, "test_" "\x1B[49m" "bg_default", 1);

    /* Text color */
    WEE_CHECK_DECODE_ANSI("test_blue", "test_" "\x1B[34m" "blue", 0);
    snprintf (string, sizeof (string),
              "test_" "%s" "blue",
              gui_color_get_custom ("|blue"));
    WEE_CHECK_DECODE_ANSI(string, "test_" "\x1B[34m" "blue", 1);

    /* Bright text color */
    WEE_CHECK_DECODE_ANSI("test_lightgreen", "test_" "\x1B[92m" "lightgreen", 0);
    snprintf (string, sizeof (string),
              "test_" "%s" "lightgreen",
              gui_color_get_custom ("|lightgreen"));
    WEE_CHECK_DECODE_ANSI(string, "test_" "\x1B[92m" "lightgreen", 1);

    /* Text terminal color */
    WEE_CHECK_DECODE_ANSI("test_214", "test_" "\x1B[38;5;214m" "214", 0);
    snprintf (string, sizeof (string),
              "test_" "%s" "214",
              gui_color_get_custom ("|214"));
    WEE_CHECK_DECODE_ANSI(string, "test_" "\x1B[38;5;214m" "214", 1);

    /* Text RGB color */
    WEE_CHECK_DECODE_ANSI("test_13", "test_" "\x1B[38;2;255;0;255m" "13", 0);
    snprintf (string, sizeof (string),
              "test_" "%s" "13",
              gui_color_get_custom ("|13"));
    WEE_CHECK_DECODE_ANSI(string, "test_" "\x1B[38;2;255;0;255m" "13", 1);

    /* Background color */
    WEE_CHECK_DECODE_ANSI("test_bg_red", "test_" "\x1B[41m" "bg_red", 0);
    snprintf (string, sizeof (string),
              "test_" "%s" "bg_red",
              gui_color_get_custom ("|,red"));
    WEE_CHECK_DECODE_ANSI(string, "test_" "\x1B[41m" "bg_red", 1);

    /* Bright background color */
    WEE_CHECK_DECODE_ANSI("test_bg_lightgreen",
                          "test_" "\x1B[102m" "bg_lightgreen", 0);
    snprintf (string, sizeof (string),
              "test_" "%s" "bg_lightgreen",
              gui_color_get_custom ("|,lightgreen"));
    WEE_CHECK_DECODE_ANSI(string, "test_" "\x1B[102m" "bg_lightgreen", 1);

    /* Background terminal color */
    WEE_CHECK_DECODE_ANSI("test_bg_240", "test_" "\x1B[48;5;214m" "bg_240", 0);
    snprintf (string, sizeof (string),
              "test_" "%s" "bg_240",
              gui_color_get_custom ("|,240"));
    WEE_CHECK_DECODE_ANSI(string, "test_" "\x1B[48;5;240m" "bg_240", 1);

    /* Background RGB color */
    WEE_CHECK_DECODE_ANSI("test_bg_13", "test_" "\x1B[48;2;255;0;255m" "bg_13", 0);
    snprintf (string, sizeof (string),
              "test_" "%s" "bg_13",
              gui_color_get_custom ("|,13"));
    WEE_CHECK_DECODE_ANSI(string, "test_" "\x1B[48;2;255;0;255m" "bg_13", 1);

    /* Text RGB + background RGB color */
    WEE_CHECK_DECODE_ANSI(
        "test_fg_13_bg_04",
        "test_" "\x1B[38;2;255;0;255m" "\x1B[48;2;0;0;128m" "fg_13_bg_04",
        0);
    snprintf (string, sizeof (string),
              "test_" "%s" "%s" "fg_13_bg_04",
              gui_color_get_custom ("|13"),
              gui_color_get_custom ("|,04"));
    WEE_CHECK_DECODE_ANSI(
        string,
        "test_\x1B[38;2;255;0;255m" "\x1B[48;2;0;0;128m" "fg_13_bg_04",
        1);
}

/*
 * Test functions:
 *   gui_color_encode_ansi
 */

TEST(GuiColor, EncodeAnsi)
{
    char string[256], expected[256], *encoded;
    int pair, fg, bg;

    /* NULL/empty string */
    STRCMP_EQUAL(NULL, gui_color_encode_ansi (NULL));
    WEE_CHECK_ENCODE_ANSI("", "");

    /* reset */
    snprintf (string, sizeof (string),
              "test_" "%s" "reset", gui_color_get_custom ("reset"));
    WEE_CHECK_ENCODE_ANSI("test_" "\x1B[0m" "reset", string);

    /* blink */
    snprintf (string, sizeof (string),
              "test_" "%s" "blink" "%s" "_end",
              gui_color_get_custom ("blink"),
              gui_color_get_custom ("-blink"));
    WEE_CHECK_ENCODE_ANSI("test_" "\x1B[5m" "blink" "\x1B[25m" "_end", string);

    /* dim */
    snprintf (string, sizeof (string),
              "test_" "%s" "dim" "%s" "_end",
              gui_color_get_custom ("dim"),
              gui_color_get_custom ("-dim"));
    WEE_CHECK_ENCODE_ANSI("test_" "\x1B[2m" "dim" "\x1B[22m" "_end", string);

    /* bold */
    snprintf (string, sizeof (string),
              "test_" "%s" "bold" "%s" "_end",
              gui_color_get_custom ("bold"),
              gui_color_get_custom ("-bold"));
    WEE_CHECK_ENCODE_ANSI("test_" "\x1B[1m" "bold" "\x1B[21m" "_end", string);

    /* reverse */
    snprintf (string, sizeof (string),
              "test_" "%s" "reverse" "%s" "_end",
              gui_color_get_custom ("reverse"),
              gui_color_get_custom ("-reverse"));
    WEE_CHECK_ENCODE_ANSI("test_" "\x1B[7m" "reverse" "\x1B[27m" "_end", string);

    /* italic */
    snprintf (string, sizeof (string),
              "test_" "%s" "italic" "%s" "_end",
              gui_color_get_custom ("italic"),
              gui_color_get_custom ("-italic"));
    WEE_CHECK_ENCODE_ANSI("test_" "\x1B[3m" "italic" "\x1B[23m" "_end", string);

    /* underline */
    snprintf (string, sizeof (string),
              "test_" "%s" "underline" "%s" "_end",
              gui_color_get_custom ("underline"),
              gui_color_get_custom ("-underline"));
    WEE_CHECK_ENCODE_ANSI("test_" "\x1B[4m" "underline" "\x1B[24m" "_end", string);

    /* Text color: WeeChat: "default" -> ANSI: 39 (default fg color) */
    snprintf (string, sizeof (string),
              "test_" "%s" "default",
              gui_color_get_custom ("default"));
    WEE_CHECK_ENCODE_ANSI("test_" "\x1B[39m" "default", string);

    /* Text color: bar foreground -> ANSI: 39 (default fg color) */
    snprintf (string, sizeof (string),
              "test_" "%s" "bar_fg",
              gui_color_get_custom ("bar_fg"));
    WEE_CHECK_ENCODE_ANSI("test_" "\x1B[39m" "bar_fg", string);

    /* Text color: bar background -> ANSI: 49 (default bg color) */
    snprintf (string, sizeof (string),
              "test_" "%s" "bar_bg",
              gui_color_get_custom ("bar_bg"));
    WEE_CHECK_ENCODE_ANSI("test_" "\x1B[49m" "bar_bg", string);

    /* Text color: bar delimiter -> color in option weechat.color.chat_delimiters */
    snprintf (string, sizeof (string),
              "test_" "%s" "bar_delim",
              gui_color_get_custom ("bar_delim"));
    WEE_CHECK_ENCODE_ANSI("test_" "\x1B[48;5;22m" "bar_delim", string);

    /* Text color */
    snprintf (string, sizeof (string),
              "test_" "%s" "blue",
              gui_color_get_custom ("blue"));
    WEE_CHECK_ENCODE_ANSI("test_" "\x1B[34m" "blue", string);

    /* Bright text color */
    snprintf (string, sizeof (string),
              "test_" "%s" "lightgreen",
              gui_color_get_custom ("lightgreen"));
    WEE_CHECK_ENCODE_ANSI("test_" "\x1B[92m" "lightgreen", string);

    /* Text terminal color */
    snprintf (string, sizeof (string),
              "test_" "%s" "214",
              gui_color_get_custom ("214"));
    WEE_CHECK_ENCODE_ANSI("test_" "\x1B[38;5;214m" "214", string);

    /* Background color: WeeChat: "default" -> ANSI: 49 (default bg color) */
    snprintf (string, sizeof (string),
              "test_" "%s" "bg_default",
              gui_color_get_custom (",default"));
    WEE_CHECK_ENCODE_ANSI("test_" "\x1B[49m" "bg_default", string);

    /* Background color */
    snprintf (string, sizeof (string),
              "test_" "%s" "bg_red",
              gui_color_get_custom (",red"));
    WEE_CHECK_ENCODE_ANSI("test_" "\x1B[41m" "bg_red", string);

    /* Bright background color */
    snprintf (string, sizeof (string),
              "test_" "%s" "bg_lightgreen",
              gui_color_get_custom (",lightgreen"));
    WEE_CHECK_ENCODE_ANSI("test_" "\x1B[102m" "bg_lightgreen", string);

    /* Background terminal color */
    snprintf (string, sizeof (string),
              "test_" "%s" "bg_240",
              gui_color_get_custom (",240"));
    WEE_CHECK_ENCODE_ANSI("test_" "\x1B[48;5;240m" "bg_240", string);

    /* WeeChat color */
    snprintf (string, sizeof (string),
              "test_" "%s" "option_weechat.color.chat_host",
              GUI_COLOR(GUI_COLOR_CHAT_HOST));
    WEE_CHECK_ENCODE_ANSI(
        "test_\x1B[0m" "\x1B[38;5;6m" "\x1B[49m"
        "option_weechat.color.chat_host",
        string);

    /* WeeChat bright color */
    snprintf (string, sizeof (string),
              "test_" "%s" "option_weechat.color.chat_nick",
              GUI_COLOR(GUI_COLOR_CHAT_NICK));
    WEE_CHECK_ENCODE_ANSI(
        "test_\x1B[0m" "\x1B[38;5;14m" "\x1B[49m"
        "option_weechat.color.chat_nick",
        string);

    /* WeeChat color with attributes */
    config_file_option_set (config_color_chat_host, "_green", 1);
    snprintf (string, sizeof (string),
              "test_" "%s" "option_weechat.color.chat_host",
              GUI_COLOR(GUI_COLOR_CHAT_HOST));
    WEE_CHECK_ENCODE_ANSI(
        "test_\x1B[0m" "\x1B[4m" "\x1B[38;5;2m" "\x1B[49m"
        "option_weechat.color.chat_host",
        string);
    config_file_option_reset (config_color_chat_host, 1);

    /*
     * Color pair: colors of the pair (not the pair number), no color for an
     * unknown pair.
     */
    pair = gui_color_get_pair (3, 5);
    CHECK(pair > 0);
    LONGS_EQUAL(1, gui_color_get_pair_colors (pair, &fg, &bg));
    LONGS_EQUAL(3, fg);
    LONGS_EQUAL(5, bg);
    LONGS_EQUAL(0, gui_color_get_pair_colors (32000, &fg, &bg));
    snprintf (string, sizeof (string), "a" "\x19" "@%05d" "b", pair);
    WEE_CHECK_ENCODE_ANSI("a" "\x1B[38;5;3m" "\x1B[48;5;5m" "b", string);
    pair = gui_color_get_pair (-1, 2);
    snprintf (string, sizeof (string), "a" "\x19" "@%05d" "b", pair);
    WEE_CHECK_ENCODE_ANSI("a" "\x1B[39m" "\x1B[48;5;2m" "b", string);
    WEE_CHECK_ENCODE_ANSI("a" "b", "a" "\x19" "@32000" "b");

    /* Multiple colors/attributes */
    snprintf (string, sizeof (string),
              "%s" "hello, " "%s" "this is" "%s %s" "blink" "%s %s" "dim"
              "%s a test %s" "blue %s" "reset %s" "yellow,red here!",
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
        "\x1B[44m" "hello, \x1B[1m" "this is" "\x1B[21m \x1B[5m" "blink"
        "\x1B[25m \x1B[2m" "dim" "\x1B[22m a test \x1B[34m" "blue \x1B[0m"
        "reset \x1B[93m" "\x1B[41m" "yellow,red here!",
        string);

    /* RGB colors */
    snprintf (string, sizeof (string),
              "test_%sfg%sbg%sfg_bg_end",
              "\x19" "F@*#ff8800",
              "\x19" "B@#000080",
              "\x19" "*@#ff8800~@#000080");
    WEE_CHECK_ENCODE_ANSI("test_"
                          "\x1B[1m" "\x1B[38;2;255;136;0m" "fg"
                          "\x1B[48;2;0;0;128m" "bg"
                          "\x1B[38;2;255;136;0m" "\x1B[48;2;0;0;128m"
                          "fg_bg_end",
                          string);

    /* RGB colors replaced by colors of the 256-color palette */
    snprintf (expected, sizeof (expected),
              "test_" "\x1B[1m" "\x1B[38;5;%dm" "fg" "\x1B[48;5;%dm" "bg"
              "\x1B[38;5;%dm" "\x1B[48;5;%dm" "fg_bg_end",
              gui_color_convert_rgb_to_term_display (0xff8800, 256),
              gui_color_convert_rgb_to_term_display (0x000080, 256),
              gui_color_convert_rgb_to_term_display (0xff8800, 256),
              gui_color_convert_rgb_to_term_display (0x000080, 256));
    encoded = gui_color_encode_ansi_rgb (string, 0);
    STRCMP_EQUAL(expected, encoded);
    free (encoded);
    encoded = hook_modifier_exec (NULL, "color_encode_ansi", "256", string);
    STRCMP_EQUAL(expected, encoded);
    free (encoded);
    encoded = hook_modifier_exec (NULL, "color_encode_ansi", NULL, string);
    CHECK(strstr (encoded, "\x1B[38;2;255;136;0m"));
    free (encoded);

    /* option color and bar delimiter with RGB colors */
    config_file_option_set (config_color_chat_nick_self, "#ff8800", 1);
    config_file_option_set (config_color_chat_delimiters, "#00ff80", 1);
    snprintf (string, sizeof (string), "%s" "nick" "%c%c%c" "delim",
              GUI_COLOR(GUI_COLOR_CHAT_NICK_SELF),
              GUI_COLOR_COLOR_CHAR, GUI_COLOR_BAR_CHAR, GUI_COLOR_BAR_DELIM_CHAR);
    encoded = gui_color_encode_ansi (string);
    CHECK(strstr (encoded, "\x1B[38;2;255;136;0m"));
    CHECK(strstr (encoded, "\x1B[48;2;0;255;128m" "delim"));
    CHECK(!strstr (encoded, ";5;1000"));
    free (encoded);
    encoded = gui_color_encode_ansi_rgb (string, 0);
    snprintf (expected, sizeof (expected), "\x1B[38;5;%dm",
              gui_color_convert_rgb_to_term_display (0xff8800, 256));
    CHECK(strstr (encoded, expected));
    snprintf (expected, sizeof (expected), "\x1B[48;5;%dm" "delim",
              gui_color_convert_rgb_to_term_display (0x00ff80, 256));
    CHECK(strstr (encoded, expected));
    CHECK(!strstr (encoded, ";2;"));
    free (encoded);
    config_file_option_reset (config_color_chat_nick_self, 1);
    config_file_option_reset (config_color_chat_delimiters, 1);
}

/*
 * Test functions:
 *   gui_color_emphasize
 */

TEST(GuiColor, Emphasize)
{
    char string1[256], string2[256], *emphasized;
    regex_t regex;

    /* NULL/empty string, search or regex */
    STRCMP_EQUAL(NULL, gui_color_emphasize (NULL, NULL, 0, NULL));
    STRCMP_EQUAL(NULL, gui_color_emphasize ("test", NULL, 0, NULL));
    STRCMP_EQUAL(NULL, gui_color_emphasize (NULL, "test", 0, NULL));

    /* Build strings for tests. */
    snprintf (string1, sizeof (string1),
              "%s" "hello, %s" "this is" "%s a test here!",
              gui_color_get_custom (",blue"),
              gui_color_get_custom ("bold"),
              gui_color_get_custom ("bold"));
    snprintf (string2, sizeof (string2),
              "%s" "hello, %s%s" "this is" "%s a test" "%s here!",
              gui_color_get_custom (",blue"),
              gui_color_get_custom ("emphasis"),
              gui_color_get_custom ("bold"),
              gui_color_get_custom ("bold"),
              gui_color_get_custom ("emphasis"));

    /* Search string (found). */
    WEE_CHECK_EMPHASIZE(string2, string1, "this is a test", 0, NULL);
    WEE_CHECK_EMPHASIZE(string2, string1, "this IS A TesT", 0, NULL);

    /* Search string (not found). */
    WEE_CHECK_EMPHASIZE(string1, string1, "this IS A TesT", 1, NULL);

    /* Search regex (found). */
    string_regcomp (&regex, "this.*test", 0);
    WEE_CHECK_EMPHASIZE(string2, string1, NULL, 0, &regex);
    regfree (&regex);

    /* Search regex (not found). */
    string_regcomp (&regex, "this.*failed", 0);
    WEE_CHECK_EMPHASIZE(string1, string1, NULL, 0, &regex);
    regfree (&regex);

    /* Search regex with "^" (found only at beginning of string). */
    snprintf (string1, sizeof (string1), "ab");
    snprintf (string2, sizeof (string2),
              "%s" "a" "%s" "b",
              gui_color_get_custom ("emphasis"),
              gui_color_get_custom ("emphasis"));
    string_regcomp (&regex, "^(a|b)", REG_EXTENDED);
    WEE_CHECK_EMPHASIZE(string2, string1, NULL, 0, &regex);
    regfree (&regex);
}
