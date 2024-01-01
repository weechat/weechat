/*
 * test-gui-chat.cpp - test chat functions
 *
 * Copyright (C) 2022-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <string.h>
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-chat.h"
#include "src/gui/gui-color.h"
#include "src/gui/gui-line.h"
#include "src/gui/gui-window.h"
}

#define WEE_GET_WORD_INFO(__result_word_start_offset,                   \
                          __result_word_end_offset,                     \
                          __result_word_length_with_spaces,             \
                          __result_word_length, __string)               \
    word_start_offset = -2;                                             \
    word_end_offset = -2;                                               \
    word_length_with_spaces = -2;                                       \
    word_length = -2;                                                   \
    gui_chat_get_word_info (gui_windows, __string,                      \
                            &word_start_offset, &word_end_offset,       \
                            &word_length_with_spaces, &word_length);    \
    LONGS_EQUAL(__result_word_start_offset, word_start_offset);         \
    LONGS_EQUAL(__result_word_end_offset, word_end_offset);             \
    LONGS_EQUAL(__result_word_length_with_spaces,                       \
                word_length_with_spaces);                               \
    LONGS_EQUAL(__result_word_length, word_length);

TEST_GROUP(GuiChat)
{
};

/*
 * Tests functions:
 *   gui_chat_init
 */

TEST(GuiChat, Init)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_chat_prefix_build
 */

TEST(GuiChat, PrefixBuild)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_chat_strlen
 */

TEST(GuiChat, Strlen)
{
    char string[128];

    LONGS_EQUAL(0, gui_chat_strlen (NULL));
    LONGS_EQUAL(0, gui_chat_strlen (""));

    /* soft hyphen */
    LONGS_EQUAL(1, gui_chat_strlen ("\u00ad"));

    /* zero width space */
    LONGS_EQUAL(1, gui_chat_strlen ("\u200b"));

    /* next line (non printable char */
    LONGS_EQUAL(1, gui_chat_strlen ("\u0085"));

    LONGS_EQUAL(3, gui_chat_strlen ("abc"));
    LONGS_EQUAL(4, gui_chat_strlen ("no\xc3\xabl"));
    LONGS_EQUAL(1, gui_chat_strlen ("\xe2\xbb\xa9"));

    /* "é" + color + "à" */
    snprintf (string, sizeof (string),
              "é%sà", gui_color_get_custom ("red"));
    LONGS_EQUAL(2, gui_chat_strlen (string));

    /* "a" + soft hyphen + color + zero width space + "b" */
    snprintf (string, sizeof (string),
              "a" "\u00ad" "%s" "\u200b" "b", gui_color_get_custom ("red"));
    LONGS_EQUAL(4, gui_chat_strlen (string));
}

/*
 * Tests functions:
 *   gui_chat_strlen_screen
 */

TEST(GuiChat, StrlenScreen)
{
    char string[128];

    LONGS_EQUAL(0, gui_chat_strlen_screen (NULL));
    LONGS_EQUAL(0, gui_chat_strlen_screen (""));

    /* soft hyphen */
    LONGS_EQUAL(0, gui_chat_strlen_screen ("\u00ad"));

    /* zero width space */
    LONGS_EQUAL(0, gui_chat_strlen_screen ("\u200b"));

    /* next line (non printable char) */
    LONGS_EQUAL(0, gui_chat_strlen_screen ("\u0085"));

    LONGS_EQUAL(3, gui_chat_strlen_screen ("abc"));
    LONGS_EQUAL(4, gui_chat_strlen_screen ("no\xc3\xabl"));
    LONGS_EQUAL(2, gui_chat_strlen_screen ("\xe2\xbb\xa9"));

    /* "é" + color + "à" */
    snprintf (string, sizeof (string),
              "é%sà", gui_color_get_custom ("red"));
    LONGS_EQUAL(2, gui_chat_strlen_screen (string));

    /* "a" + soft hyphen + color + zero width space + "b" */
    snprintf (string, sizeof (string),
              "a" "\u00ad" "%s" "\u200b" "b", gui_color_get_custom ("red"));
    LONGS_EQUAL(2, gui_chat_strlen_screen (string));
}

/*
 * Tests functions:
 *   gui_chat_string_add_offset
 */

TEST(GuiChat, StringAddOffset)
{
    const char *str_empty = "";
    const char *str_noel = "no\xc3\xabl";
    const char *str_other = "A\xe2\xbb\xa9Z";
    const char *str_soft_hyphen = "A" "\u00ad" "Z";
    const char *str_zero_width_space = "A" "\u200b" "Z";

    POINTERS_EQUAL(NULL, gui_chat_string_add_offset (NULL, -1));
    POINTERS_EQUAL(NULL, gui_chat_string_add_offset (NULL, 0));
    POINTERS_EQUAL(NULL, gui_chat_string_add_offset (NULL, 1));

    POINTERS_EQUAL(str_empty, gui_chat_string_add_offset (str_empty, -1));
    POINTERS_EQUAL(str_empty, gui_chat_string_add_offset (str_empty, 0));
    POINTERS_EQUAL(str_empty, gui_chat_string_add_offset (str_empty, 1));

    POINTERS_EQUAL(str_noel, gui_chat_string_add_offset (str_noel, -1));
    POINTERS_EQUAL(str_noel, gui_chat_string_add_offset (str_noel, 0));
    POINTERS_EQUAL(str_noel + 1, gui_chat_string_add_offset (str_noel, 1));
    POINTERS_EQUAL(str_noel + 2, gui_chat_string_add_offset (str_noel, 2));
    POINTERS_EQUAL(str_noel + 4, gui_chat_string_add_offset (str_noel, 3));
    POINTERS_EQUAL(str_noel + 5, gui_chat_string_add_offset (str_noel, 4));
    POINTERS_EQUAL(str_noel + 5, gui_chat_string_add_offset (str_noel, 5));

    POINTERS_EQUAL(str_other, gui_chat_string_add_offset (str_other, -1));
    POINTERS_EQUAL(str_other, gui_chat_string_add_offset (str_other, 0));
    POINTERS_EQUAL(str_other + 1, gui_chat_string_add_offset (str_other, 1));
    POINTERS_EQUAL(str_other + 4, gui_chat_string_add_offset (str_other, 2));
    POINTERS_EQUAL(str_other + 5, gui_chat_string_add_offset (str_other, 3));
    POINTERS_EQUAL(str_other + 5, gui_chat_string_add_offset (str_other, 4));
    POINTERS_EQUAL(str_other + 5, gui_chat_string_add_offset (str_other, 5));

    POINTERS_EQUAL(str_soft_hyphen, gui_chat_string_add_offset (str_soft_hyphen, -1));
    POINTERS_EQUAL(str_soft_hyphen, gui_chat_string_add_offset (str_soft_hyphen, 0));
    POINTERS_EQUAL(str_soft_hyphen + 1, gui_chat_string_add_offset (str_soft_hyphen, 1));
    POINTERS_EQUAL(str_soft_hyphen + 3, gui_chat_string_add_offset (str_soft_hyphen, 2));
    POINTERS_EQUAL(str_soft_hyphen + 4, gui_chat_string_add_offset (str_soft_hyphen, 3));
    POINTERS_EQUAL(str_soft_hyphen + 4, gui_chat_string_add_offset (str_soft_hyphen, 4));

    POINTERS_EQUAL(str_zero_width_space, gui_chat_string_add_offset (str_zero_width_space, -1));
    POINTERS_EQUAL(str_zero_width_space, gui_chat_string_add_offset (str_zero_width_space, 0));
    POINTERS_EQUAL(str_zero_width_space + 1, gui_chat_string_add_offset (str_zero_width_space, 1));
    POINTERS_EQUAL(str_zero_width_space + 4, gui_chat_string_add_offset (str_zero_width_space, 2));
    POINTERS_EQUAL(str_zero_width_space + 5, gui_chat_string_add_offset (str_zero_width_space, 3));
    POINTERS_EQUAL(str_zero_width_space + 5, gui_chat_string_add_offset (str_zero_width_space, 4));
}

/*
 * Tests functions:
 *   gui_chat_string_add_offset_screen
 */

TEST(GuiChat, StringAddOffsetScreen)
{
    const char *str_empty = "";
    const char *str_noel = "no\xc3\xabl";
    const char *str_other = "A\xe2\xbb\xa9Z";
    const char *str_soft_hyphen = "A" "\u00ad" "Z";
    const char *str_zero_width_space = "A" "\u200b" "Z";

    POINTERS_EQUAL(NULL, gui_chat_string_add_offset_screen (NULL, -1));
    POINTERS_EQUAL(NULL, gui_chat_string_add_offset_screen (NULL, 0));
    POINTERS_EQUAL(NULL, gui_chat_string_add_offset_screen (NULL, 1));

    POINTERS_EQUAL(str_empty, gui_chat_string_add_offset_screen (str_empty, -1));
    POINTERS_EQUAL(str_empty, gui_chat_string_add_offset_screen (str_empty, 0));
    POINTERS_EQUAL(str_empty, gui_chat_string_add_offset_screen (str_empty, 1));

    POINTERS_EQUAL(str_noel, gui_chat_string_add_offset_screen (str_noel, -1));
    POINTERS_EQUAL(str_noel, gui_chat_string_add_offset_screen (str_noel, 0));
    POINTERS_EQUAL(str_noel + 1, gui_chat_string_add_offset_screen (str_noel, 1));
    POINTERS_EQUAL(str_noel + 2, gui_chat_string_add_offset_screen (str_noel, 2));
    POINTERS_EQUAL(str_noel + 4, gui_chat_string_add_offset_screen (str_noel, 3));
    POINTERS_EQUAL(str_noel + 5, gui_chat_string_add_offset_screen (str_noel, 4));
    POINTERS_EQUAL(str_noel + 5, gui_chat_string_add_offset_screen (str_noel, 5));

    POINTERS_EQUAL(str_other, gui_chat_string_add_offset_screen (str_other, -1));
    POINTERS_EQUAL(str_other, gui_chat_string_add_offset_screen (str_other, 0));
    POINTERS_EQUAL(str_other + 1, gui_chat_string_add_offset_screen (str_other, 1));
    POINTERS_EQUAL(str_other + 1, gui_chat_string_add_offset_screen (str_other, 2));
    POINTERS_EQUAL(str_other + 4, gui_chat_string_add_offset_screen (str_other, 3));
    POINTERS_EQUAL(str_other + 5, gui_chat_string_add_offset_screen (str_other, 4));
    POINTERS_EQUAL(str_other + 5, gui_chat_string_add_offset_screen (str_other, 5));

    POINTERS_EQUAL(str_soft_hyphen, gui_chat_string_add_offset_screen (str_soft_hyphen, -1));
    POINTERS_EQUAL(str_soft_hyphen, gui_chat_string_add_offset_screen (str_soft_hyphen, 0));
    POINTERS_EQUAL(str_soft_hyphen + 3, gui_chat_string_add_offset_screen (str_soft_hyphen, 1));
    POINTERS_EQUAL(str_soft_hyphen + 4, gui_chat_string_add_offset_screen (str_soft_hyphen, 2));
    POINTERS_EQUAL(str_soft_hyphen + 4, gui_chat_string_add_offset_screen (str_soft_hyphen, 3));

    POINTERS_EQUAL(str_zero_width_space, gui_chat_string_add_offset_screen (str_zero_width_space, -1));
    POINTERS_EQUAL(str_zero_width_space, gui_chat_string_add_offset_screen (str_zero_width_space, 0));
    POINTERS_EQUAL(str_zero_width_space + 4, gui_chat_string_add_offset_screen (str_zero_width_space, 1));
    POINTERS_EQUAL(str_zero_width_space + 5, gui_chat_string_add_offset_screen (str_zero_width_space, 2));
    POINTERS_EQUAL(str_zero_width_space + 5, gui_chat_string_add_offset_screen (str_zero_width_space, 3));
}

/*
 * Tests functions:
 *   gui_chat_string_real_pos
 */

TEST(GuiChat, StringRealPos)
{
    LONGS_EQUAL(0, gui_chat_string_real_pos (NULL, -1, 0));
    LONGS_EQUAL(0, gui_chat_string_real_pos (NULL, 0, 0));
    LONGS_EQUAL(0, gui_chat_string_real_pos (NULL, 1, 0));

    LONGS_EQUAL(0, gui_chat_string_real_pos ("", -1, 0));
    LONGS_EQUAL(0, gui_chat_string_real_pos ("", 0, 0));
    LONGS_EQUAL(0, gui_chat_string_real_pos ("", 1, 0));

    LONGS_EQUAL(0, gui_chat_string_real_pos ("abc", 0, 0));
    LONGS_EQUAL(1, gui_chat_string_real_pos ("abc", 1, 0));
    LONGS_EQUAL(2, gui_chat_string_real_pos ("abc", 2, 0));

    LONGS_EQUAL(0, gui_chat_string_real_pos ("no\xc3\xabl", 0, 0));
    LONGS_EQUAL(1, gui_chat_string_real_pos ("no\xc3\xabl", 1, 0));
    LONGS_EQUAL(2, gui_chat_string_real_pos ("no\xc3\xabl", 2, 0));

    LONGS_EQUAL(0, gui_chat_string_real_pos ("\xe2\xbb\xa9", 0, 0));
    LONGS_EQUAL(3, gui_chat_string_real_pos ("\xe2\xbb\xa9", 1, 0));
    LONGS_EQUAL(3, gui_chat_string_real_pos ("\xe2\xbb\xa9", 2, 0));

    LONGS_EQUAL(0, gui_chat_string_real_pos ("\xe2\xbb\xa9", 0, 1));
    LONGS_EQUAL(0, gui_chat_string_real_pos ("\xe2\xbb\xa9", 1, 1));
    LONGS_EQUAL(3, gui_chat_string_real_pos ("\xe2\xbb\xa9", 2, 1));

    /* soft hyphen */
    LONGS_EQUAL(0, gui_chat_string_real_pos ("A" "\u00ad" "Z", 0, 0));
    LONGS_EQUAL(3, gui_chat_string_real_pos ("A" "\u00ad" "Z", 1, 0));
    LONGS_EQUAL(4, gui_chat_string_real_pos ("A" "\u00ad" "Z", 2, 0));

    /* zero width space */
    LONGS_EQUAL(0, gui_chat_string_real_pos ("A" "\u200b" "Z", 0, 1));
    LONGS_EQUAL(4, gui_chat_string_real_pos ("A" "\u200b" "Z", 1, 1));
    LONGS_EQUAL(5, gui_chat_string_real_pos ("A" "\u200b" "Z", 2, 1));
}

/*
 * Tests functions:
 *   gui_chat_string_pos
 */

TEST(GuiChat, StringPos)
{
    LONGS_EQUAL(0, gui_chat_string_pos (NULL, -1));
    LONGS_EQUAL(0, gui_chat_string_pos (NULL, 0));
    LONGS_EQUAL(0, gui_chat_string_pos (NULL, 1));

    LONGS_EQUAL(0, gui_chat_string_pos ("", -1));
    LONGS_EQUAL(0, gui_chat_string_pos ("", 0));
    LONGS_EQUAL(0, gui_chat_string_pos ("", 1));

    LONGS_EQUAL(0, gui_chat_string_pos ("abc", 0));
    LONGS_EQUAL(1, gui_chat_string_pos ("abc", 1));
    LONGS_EQUAL(2, gui_chat_string_pos ("abc", 2));

    LONGS_EQUAL(0, gui_chat_string_pos ("no\xc3\xabl", 0));
    LONGS_EQUAL(1, gui_chat_string_pos ("no\xc3\xabl", 1));
    LONGS_EQUAL(2, gui_chat_string_pos ("no\xc3\xabl", 2));

    LONGS_EQUAL(0, gui_chat_string_pos ("\xe2\xbb\xa9", 0));
    LONGS_EQUAL(1, gui_chat_string_pos ("\xe2\xbb\xa9", 1));
    LONGS_EQUAL(1, gui_chat_string_pos ("\xe2\xbb\xa9", 2));

    /* soft hyphen */
    LONGS_EQUAL(0, gui_chat_string_pos ("A" "\u00ad" "Z", 0));
    LONGS_EQUAL(1, gui_chat_string_pos ("A" "\u00ad" "Z", 1));
    LONGS_EQUAL(2, gui_chat_string_pos ("A" "\u00ad" "Z", 2));

    /* zero width space */
    LONGS_EQUAL(0, gui_chat_string_pos ("A" "\u200b" "Z", 0));
    LONGS_EQUAL(1, gui_chat_string_pos ("A" "\u200b" "Z", 1));
    LONGS_EQUAL(2, gui_chat_string_pos ("A" "\u200b" "Z", 2));
}

/*
 * Tests functions:
 *   gui_chat_get_word_info
 */

TEST(GuiChat, GetWordInfo)
{
    int word_start_offset, word_end_offset, word_length_with_spaces;
    int word_length;
    char string[32];

    WEE_GET_WORD_INFO (0, 0, 0, -1, NULL);
    WEE_GET_WORD_INFO (0, 0, 0, -1, "");
    WEE_GET_WORD_INFO (0, 1, 1, 1, "a");
    WEE_GET_WORD_INFO (0, 3, 3, 3, "abc");
    WEE_GET_WORD_INFO (2, 5, 5, 3, "  abc");
    WEE_GET_WORD_INFO (2, 5, 5, 3, "  abc  ");
    WEE_GET_WORD_INFO (0, 5, 5, 5, "first second");
    WEE_GET_WORD_INFO (1, 6, 6, 5, " first second");

    WEE_GET_WORD_INFO (0, 0, 0, 0, "\nabc");
    WEE_GET_WORD_INFO (0, 1, 1, 0, " \nabc");
    WEE_GET_WORD_INFO (0, 2, 2, 0, "  \nabc");
    WEE_GET_WORD_INFO (0, 5, 5, 5, "first\nsecond");

    snprintf (string, sizeof (string), "%c%c01abc", GUI_COLOR_COLOR_CHAR, GUI_COLOR_FG_CHAR);
    WEE_GET_WORD_INFO (4, 7, 3, 3, string);
    snprintf (string, sizeof (string), "abc%c%c01", GUI_COLOR_COLOR_CHAR, GUI_COLOR_FG_CHAR);
    WEE_GET_WORD_INFO (0, 7, 3, 3, string);
    snprintf (string, sizeof (string), " %c%c01 abc", GUI_COLOR_COLOR_CHAR, GUI_COLOR_FG_CHAR);
    WEE_GET_WORD_INFO (6, 9, 5, 3, string);

    snprintf (string, sizeof (string), "\n%c%c01abc", GUI_COLOR_COLOR_CHAR, GUI_COLOR_FG_CHAR);
    WEE_GET_WORD_INFO (0, 0, 0, 0, string);
    snprintf (string, sizeof (string), "%c%c01\nabc", GUI_COLOR_COLOR_CHAR, GUI_COLOR_FG_CHAR);
    WEE_GET_WORD_INFO (0, 4, 0, 0, string);
    snprintf (string, sizeof (string), " %c%c01 \nabc", GUI_COLOR_COLOR_CHAR, GUI_COLOR_FG_CHAR);
    WEE_GET_WORD_INFO (0, 6, 2, 0, string);
}

/*
 * Tests functions:
 *   gui_chat_get_time_string
 */

TEST(GuiChat, GetTimeString)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_chat_get_time_length
 */

TEST(GuiChat, GetTimeLength)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_chat_change_time_format
 */

TEST(GuiChat, ChangeTimeFormat)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_chat_buffer_valid
 */

TEST(GuiChat, BufferValid)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_chat_add_line_waiting_buffer
 */

TEST(GuiChat, AddLineWaitingBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_chat_print_lines_waiting_buffer
 */

TEST(GuiChat, PrintLinesWaitingBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_chat_printf_datetime_tags_internal
 *   gui_chat_printf_datetime_tags
 */

TEST(GuiChat, PrintDatetimeTags)
{
    struct t_gui_line *ptr_last_line;
    struct t_gui_line_data *ptr_data;

    /* invalid buffer */
    ptr_last_line = gui_buffers->own_lines->last_line;
    gui_chat_printf_datetime_tags ((struct t_gui_buffer *)0x1, 0, 0, NULL, "test");
    POINTERS_EQUAL(ptr_last_line, gui_buffers->own_lines->last_line);

    /* NULL message */
    ptr_last_line = gui_buffers->own_lines->last_line;
    gui_chat_printf_datetime_tags (gui_buffers, 0, 0, NULL, NULL);
    POINTERS_EQUAL(ptr_last_line, gui_buffers->own_lines->last_line);

    /* empty message */
    ptr_last_line = gui_buffers->own_lines->last_line;
    gui_chat_printf_datetime_tags (gui_buffers, 0, 0, NULL, "");
    CHECK(ptr_last_line != gui_buffers->own_lines->last_line);
    ptr_data = gui_buffers->own_lines->last_line->data;
    CHECK(ptr_data);
    POINTERS_EQUAL(gui_buffers, ptr_data->buffer);
    LONGS_EQUAL(-1, ptr_data->y);
    CHECK(ptr_data->date > 0);
    CHECK((ptr_data->date_usec >= 0) && (ptr_data->date_usec <= 999999));
    CHECK(ptr_data->date == ptr_data->date_printed);
    CHECK(ptr_data->date_usec == ptr_data->date_usec_printed);
    CHECK(ptr_data->str_time && ptr_data->str_time[0]);
    LONGS_EQUAL(0, ptr_data->tags_count);
    POINTERS_EQUAL(NULL, ptr_data->tags_array);
    LONGS_EQUAL(1, ptr_data->displayed);
    LONGS_EQUAL(0, ptr_data->notify_level);
    LONGS_EQUAL(0, ptr_data->highlight);
    LONGS_EQUAL(0, ptr_data->refresh_needed);
    STRCMP_EQUAL("", ptr_data->prefix);
    LONGS_EQUAL(0, ptr_data->prefix_length);
    STRCMP_EQUAL("", ptr_data->message);

    /* message (no prefix) */
    ptr_last_line = gui_buffers->own_lines->last_line;
    gui_chat_printf_datetime_tags (gui_buffers, 0, 0, NULL, "this is a test");
    CHECK(ptr_last_line != gui_buffers->own_lines->last_line);
    ptr_data = gui_buffers->own_lines->last_line->data;
    CHECK(ptr_data);
    POINTERS_EQUAL(gui_buffers, ptr_data->buffer);
    LONGS_EQUAL(-1, ptr_data->y);
    CHECK(ptr_data->date > 0);
    CHECK((ptr_data->date_usec >= 0) && (ptr_data->date_usec <= 999999));
    CHECK(ptr_data->date == ptr_data->date_printed);
    CHECK(ptr_data->date_usec == ptr_data->date_usec_printed);
    CHECK(ptr_data->str_time && ptr_data->str_time[0]);
    LONGS_EQUAL(0, ptr_data->tags_count);
    POINTERS_EQUAL(NULL, ptr_data->tags_array);
    LONGS_EQUAL(1, ptr_data->displayed);
    LONGS_EQUAL(0, ptr_data->notify_level);
    LONGS_EQUAL(0, ptr_data->highlight);
    LONGS_EQUAL(0, ptr_data->refresh_needed);
    STRCMP_EQUAL("", ptr_data->prefix);
    LONGS_EQUAL(0, ptr_data->prefix_length);
    STRCMP_EQUAL("this is a test", ptr_data->message);

    /* message with prefix */
    ptr_last_line = gui_buffers->own_lines->last_line;
    gui_chat_printf_datetime_tags (gui_buffers, 0, 0, NULL, "nick\tthis is a test");
    CHECK(ptr_last_line != gui_buffers->own_lines->last_line);
    ptr_data = gui_buffers->own_lines->last_line->data;
    CHECK(ptr_data);
    POINTERS_EQUAL(gui_buffers, ptr_data->buffer);
    LONGS_EQUAL(-1, ptr_data->y);
    CHECK(ptr_data->date > 0);
    CHECK((ptr_data->date_usec >= 0) && (ptr_data->date_usec <= 999999));
    CHECK(ptr_data->date == ptr_data->date_printed);
    CHECK(ptr_data->date_usec == ptr_data->date_usec_printed);
    CHECK(ptr_data->str_time && ptr_data->str_time[0]);
    LONGS_EQUAL(0, ptr_data->tags_count);
    POINTERS_EQUAL(NULL, ptr_data->tags_array);
    LONGS_EQUAL(1, ptr_data->displayed);
    LONGS_EQUAL(0, ptr_data->notify_level);
    LONGS_EQUAL(0, ptr_data->highlight);
    LONGS_EQUAL(0, ptr_data->refresh_needed);
    STRCMP_EQUAL("nick", ptr_data->prefix);
    LONGS_EQUAL(4, ptr_data->prefix_length);
    STRCMP_EQUAL("this is a test", ptr_data->message);

    /* message with prefix */
    ptr_last_line = gui_buffers->own_lines->last_line;
    gui_chat_printf_datetime_tags (gui_buffers, 0, 0, NULL, "nick\tthis is a test");
    CHECK(ptr_last_line != gui_buffers->own_lines->last_line);
    ptr_data = gui_buffers->own_lines->last_line->data;
    CHECK(ptr_data);
    POINTERS_EQUAL(gui_buffers, ptr_data->buffer);
    LONGS_EQUAL(-1, ptr_data->y);
    CHECK(ptr_data->date > 0);
    CHECK((ptr_data->date_usec >= 0) && (ptr_data->date_usec <= 999999));
    CHECK(ptr_data->date == ptr_data->date_printed);
    CHECK(ptr_data->date_usec == ptr_data->date_usec_printed);
    CHECK(ptr_data->str_time && ptr_data->str_time[0]);
    LONGS_EQUAL(0, ptr_data->tags_count);
    POINTERS_EQUAL(NULL, ptr_data->tags_array);
    LONGS_EQUAL(1, ptr_data->displayed);
    LONGS_EQUAL(0, ptr_data->notify_level);
    LONGS_EQUAL(0, ptr_data->highlight);
    LONGS_EQUAL(0, ptr_data->refresh_needed);
    STRCMP_EQUAL("nick", ptr_data->prefix);
    LONGS_EQUAL(4, ptr_data->prefix_length);
    STRCMP_EQUAL("this is a test", ptr_data->message);

    /* message with ignored prefix (space + tab) */
    ptr_last_line = gui_buffers->own_lines->last_line;
    gui_chat_printf_datetime_tags (gui_buffers, 0, 0, NULL, " \tthis is a test");
    CHECK(ptr_last_line != gui_buffers->own_lines->last_line);
    ptr_data = gui_buffers->own_lines->last_line->data;
    CHECK(ptr_data);
    POINTERS_EQUAL(gui_buffers, ptr_data->buffer);
    LONGS_EQUAL(-1, ptr_data->y);
    CHECK(ptr_data->date > 0);
    CHECK((ptr_data->date_usec >= 0) && (ptr_data->date_usec <= 999999));
    CHECK(ptr_data->date == ptr_data->date_printed);
    CHECK(ptr_data->date_usec == ptr_data->date_usec_printed);
    CHECK(ptr_data->str_time && ptr_data->str_time[0]);
    LONGS_EQUAL(0, ptr_data->tags_count);
    POINTERS_EQUAL(NULL, ptr_data->tags_array);
    LONGS_EQUAL(1, ptr_data->displayed);
    LONGS_EQUAL(0, ptr_data->notify_level);
    LONGS_EQUAL(0, ptr_data->highlight);
    LONGS_EQUAL(0, ptr_data->refresh_needed);
    STRCMP_EQUAL("", ptr_data->prefix);
    LONGS_EQUAL(0, ptr_data->prefix_length);
    STRCMP_EQUAL("this is a test", ptr_data->message);

    /* message with no time displayed (two tabs) */
    ptr_last_line = gui_buffers->own_lines->last_line;
    gui_chat_printf_datetime_tags (gui_buffers, 0, 0, NULL, "\t\tthis is a test");
    CHECK(ptr_last_line != gui_buffers->own_lines->last_line);
    ptr_data = gui_buffers->own_lines->last_line->data;
    CHECK(ptr_data);
    POINTERS_EQUAL(gui_buffers, ptr_data->buffer);
    LONGS_EQUAL(-1, ptr_data->y);
    LONGS_EQUAL(0, ptr_data->date);
    LONGS_EQUAL(0, ptr_data->date_usec);
    CHECK(ptr_data->date_printed > 0);
    CHECK((ptr_data->date_usec_printed >= 0) && (ptr_data->date_usec_printed <= 999999));
    POINTERS_EQUAL(NULL, ptr_data->str_time);
    LONGS_EQUAL(0, ptr_data->tags_count);
    POINTERS_EQUAL(NULL, ptr_data->tags_array);
    LONGS_EQUAL(1, ptr_data->displayed);
    LONGS_EQUAL(0, ptr_data->notify_level);
    LONGS_EQUAL(0, ptr_data->highlight);
    LONGS_EQUAL(0, ptr_data->refresh_needed);
    POINTERS_EQUAL(NULL, ptr_data->prefix);
    LONGS_EQUAL(0, ptr_data->prefix_length);
    STRCMP_EQUAL("this is a test", ptr_data->message);

    /* message with past date */
    ptr_last_line = gui_buffers->own_lines->last_line;
    gui_chat_printf_datetime_tags (gui_buffers, 946681200, 123456, NULL,
                                   "nick\tthis is a test");
    CHECK(ptr_last_line != gui_buffers->own_lines->last_line);
    ptr_data = gui_buffers->own_lines->last_line->data;
    CHECK(ptr_data);
    POINTERS_EQUAL(gui_buffers, ptr_data->buffer);
    LONGS_EQUAL(-1, ptr_data->y);
    LONGS_EQUAL(946681200, ptr_data->date);
    LONGS_EQUAL(123456, ptr_data->date_usec);
    CHECK(ptr_data->date < ptr_data->date_printed);
    CHECK(ptr_data->str_time && ptr_data->str_time[0]);
    LONGS_EQUAL(0, ptr_data->tags_count);
    POINTERS_EQUAL(NULL, ptr_data->tags_array);
    LONGS_EQUAL(1, ptr_data->displayed);
    LONGS_EQUAL(0, ptr_data->notify_level);
    LONGS_EQUAL(0, ptr_data->highlight);
    LONGS_EQUAL(0, ptr_data->refresh_needed);
    STRCMP_EQUAL("nick", ptr_data->prefix);
    LONGS_EQUAL(4, ptr_data->prefix_length);
    STRCMP_EQUAL("this is a test", ptr_data->message);

    /* message with empty tags */
    ptr_last_line = gui_buffers->own_lines->last_line;
    gui_chat_printf_datetime_tags (gui_buffers, 0, 0, "",
                                   "nick\tthis is a test");
    CHECK(ptr_last_line != gui_buffers->own_lines->last_line);
    ptr_data = gui_buffers->own_lines->last_line->data;
    CHECK(ptr_data);
    POINTERS_EQUAL(gui_buffers, ptr_data->buffer);
    LONGS_EQUAL(-1, ptr_data->y);
    CHECK(ptr_data->date > 0);
    CHECK((ptr_data->date_usec >= 0) && (ptr_data->date_usec <= 999999));
    CHECK(ptr_data->date == ptr_data->date_printed);
    CHECK(ptr_data->date_usec == ptr_data->date_usec_printed);
    CHECK(ptr_data->str_time && ptr_data->str_time[0]);
    LONGS_EQUAL(0, ptr_data->tags_count);
    POINTERS_EQUAL(NULL, ptr_data->tags_array);
    LONGS_EQUAL(1, ptr_data->displayed);
    LONGS_EQUAL(0, ptr_data->notify_level);
    LONGS_EQUAL(0, ptr_data->highlight);
    LONGS_EQUAL(0, ptr_data->refresh_needed);
    STRCMP_EQUAL("nick", ptr_data->prefix);
    LONGS_EQUAL(4, ptr_data->prefix_length);
    STRCMP_EQUAL("this is a test", ptr_data->message);

    /* message with 3 tags */
    ptr_last_line = gui_buffers->own_lines->last_line;
    gui_chat_printf_datetime_tags (gui_buffers, 0, 0, "tag1,tag2,tag3",
                                   "nick\tthis is a test");
    CHECK(ptr_last_line != gui_buffers->own_lines->last_line);
    ptr_data = gui_buffers->own_lines->last_line->data;
    CHECK(ptr_data);
    POINTERS_EQUAL(gui_buffers, ptr_data->buffer);
    LONGS_EQUAL(-1, ptr_data->y);
    CHECK(ptr_data->date > 0);
    CHECK((ptr_data->date_usec >= 0) && (ptr_data->date_usec <= 999999));
    CHECK(ptr_data->date == ptr_data->date_printed);
    CHECK(ptr_data->date_usec == ptr_data->date_usec_printed);
    CHECK(ptr_data->str_time && ptr_data->str_time[0]);
    LONGS_EQUAL(3, ptr_data->tags_count);
    CHECK(ptr_data->tags_array);
    STRCMP_EQUAL("tag1", ptr_data->tags_array[0]);
    STRCMP_EQUAL("tag2", ptr_data->tags_array[1]);
    STRCMP_EQUAL("tag3", ptr_data->tags_array[2]);
    LONGS_EQUAL(1, ptr_data->displayed);
    LONGS_EQUAL(0, ptr_data->notify_level);
    LONGS_EQUAL(0, ptr_data->highlight);
    LONGS_EQUAL(0, ptr_data->refresh_needed);
    STRCMP_EQUAL("nick", ptr_data->prefix);
    LONGS_EQUAL(4, ptr_data->prefix_length);
    STRCMP_EQUAL("this is a test", ptr_data->message);
}

/*
 * Tests functions:
 *   gui_chat_printf_y_datetime_tags
 */

TEST(GuiChat, PrintYDatetimeTags)
{
    struct t_gui_buffer *buffer;
    struct t_gui_line_data *ptr_data;

    buffer = gui_buffer_new (NULL, "test", NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(buffer);
    gui_buffer_set (buffer, "type", "free");

    /* invalid buffer pointer */
    gui_chat_printf_y_datetime_tags ((struct t_gui_buffer *)0x1, 0, 0, 0, NULL,
                                     "test");
    POINTERS_EQUAL(NULL, buffer->own_lines->last_line);

    /* invalid buffer: not with free content */
    gui_chat_printf_y_datetime_tags (gui_buffers, 0, 0, 0, NULL, "test");
    POINTERS_EQUAL(NULL, buffer->own_lines->last_line);

    /* NULL message */
    gui_chat_printf_y_datetime_tags (buffer, 0, 0, 0, NULL, NULL);
    POINTERS_EQUAL(NULL, buffer->own_lines->last_line);

    /* empty message */
    gui_chat_printf_y_datetime_tags (buffer, 0, 0, 0, NULL, "");
    POINTERS_EQUAL(NULL, buffer->own_lines->last_line);

    /* message on first line */
    gui_chat_printf_y_datetime_tags (buffer, 0, 0, 0, NULL,
                                     "this is a test on line 1");
    CHECK(buffer->own_lines->last_line);
    ptr_data = buffer->own_lines->last_line->data;
    CHECK(ptr_data);
    POINTERS_EQUAL(buffer, ptr_data->buffer);
    LONGS_EQUAL(0, ptr_data->y);
    CHECK(ptr_data->date > 0);
    CHECK((ptr_data->date_usec >= 0) && (ptr_data->date_usec <= 999999));
    CHECK(ptr_data->date == ptr_data->date_printed);
    CHECK(ptr_data->date_usec == ptr_data->date_usec_printed);
    POINTERS_EQUAL(NULL, ptr_data->str_time);
    LONGS_EQUAL(0, ptr_data->tags_count);
    POINTERS_EQUAL(NULL, ptr_data->tags_array);
    LONGS_EQUAL(1, ptr_data->displayed);
    LONGS_EQUAL(0, ptr_data->notify_level);
    LONGS_EQUAL(0, ptr_data->highlight);
    LONGS_EQUAL(1, ptr_data->refresh_needed);
    POINTERS_EQUAL(NULL, ptr_data->prefix);
    LONGS_EQUAL(0, ptr_data->prefix_length);
    STRCMP_EQUAL("this is a test on line 1", ptr_data->message);

    /* message on first line with past date */
    gui_chat_printf_y_datetime_tags (buffer, 0, 946681200, 123456, NULL,
                                     "this is a test on line 1");
    CHECK(buffer->own_lines->last_line);
    ptr_data = buffer->own_lines->last_line->data;
    CHECK(ptr_data);
    POINTERS_EQUAL(buffer, ptr_data->buffer);
    LONGS_EQUAL(0, ptr_data->y);
    LONGS_EQUAL(946681200, ptr_data->date);
    LONGS_EQUAL(123456, ptr_data->date_usec);
    CHECK(ptr_data->date < ptr_data->date_printed);
    CHECK((ptr_data->date_usec_printed >= 0) && (ptr_data->date_usec_printed <= 999999));
    POINTERS_EQUAL(NULL, ptr_data->str_time);
    LONGS_EQUAL(0, ptr_data->tags_count);
    POINTERS_EQUAL(NULL, ptr_data->tags_array);
    LONGS_EQUAL(1, ptr_data->displayed);
    LONGS_EQUAL(0, ptr_data->notify_level);
    LONGS_EQUAL(0, ptr_data->highlight);
    LONGS_EQUAL(1, ptr_data->refresh_needed);
    POINTERS_EQUAL(NULL, ptr_data->prefix);
    LONGS_EQUAL(0, ptr_data->prefix_length);
    STRCMP_EQUAL("this is a test on line 1", ptr_data->message);

    /* message on first line with empty tags */
    gui_chat_printf_y_datetime_tags (buffer, 0, 0, 0, "",
                                     "this is a test on line 1");
    CHECK(buffer->own_lines->last_line);
    ptr_data = buffer->own_lines->last_line->data;
    CHECK(ptr_data);
    POINTERS_EQUAL(buffer, ptr_data->buffer);
    LONGS_EQUAL(0, ptr_data->y);
    CHECK(ptr_data->date > 0);
    CHECK((ptr_data->date_usec >= 0) && (ptr_data->date_usec <= 999999));
    CHECK(ptr_data->date == ptr_data->date_printed);
    CHECK(ptr_data->date_usec == ptr_data->date_usec_printed);
    POINTERS_EQUAL(NULL, ptr_data->str_time);
    LONGS_EQUAL(0, ptr_data->tags_count);
    POINTERS_EQUAL(NULL, ptr_data->tags_array);
    LONGS_EQUAL(1, ptr_data->displayed);
    LONGS_EQUAL(0, ptr_data->notify_level);
    LONGS_EQUAL(0, ptr_data->highlight);
    LONGS_EQUAL(1, ptr_data->refresh_needed);
    POINTERS_EQUAL(NULL, ptr_data->prefix);
    LONGS_EQUAL(0, ptr_data->prefix_length);
    STRCMP_EQUAL("this is a test on line 1", ptr_data->message);

    /* message on first line with 3 tags */
    gui_chat_printf_y_datetime_tags (buffer, 0, 0, 0, "tag1,tag2,tag3",
                                     "this is a test on line 1");
    CHECK(buffer->own_lines->last_line);
    ptr_data = buffer->own_lines->last_line->data;
    CHECK(ptr_data);
    POINTERS_EQUAL(buffer, ptr_data->buffer);
    LONGS_EQUAL(0, ptr_data->y);
    CHECK(ptr_data->date > 0);
    CHECK((ptr_data->date_usec >= 0) && (ptr_data->date_usec <= 999999));
    CHECK(ptr_data->date == ptr_data->date_printed);
    CHECK(ptr_data->date_usec == ptr_data->date_usec_printed);
    POINTERS_EQUAL(NULL, ptr_data->str_time);
    LONGS_EQUAL(3, ptr_data->tags_count);
    CHECK(ptr_data->tags_array);
    STRCMP_EQUAL("tag1", ptr_data->tags_array[0]);
    STRCMP_EQUAL("tag2", ptr_data->tags_array[1]);
    STRCMP_EQUAL("tag3", ptr_data->tags_array[2]);
    LONGS_EQUAL(1, ptr_data->displayed);
    LONGS_EQUAL(0, ptr_data->notify_level);
    LONGS_EQUAL(0, ptr_data->highlight);
    LONGS_EQUAL(1, ptr_data->refresh_needed);
    POINTERS_EQUAL(NULL, ptr_data->prefix);
    LONGS_EQUAL(0, ptr_data->prefix_length);
    STRCMP_EQUAL("this is a test on line 1", ptr_data->message);

    /* message on third line */
    gui_chat_printf_y_datetime_tags (buffer, 2, 0, 0, NULL,
                                     "this is a test on line 3");
    CHECK(buffer->own_lines->last_line);
    ptr_data = buffer->own_lines->last_line->data;
    CHECK(ptr_data);
    POINTERS_EQUAL(buffer, ptr_data->buffer);
    LONGS_EQUAL(2, ptr_data->y);
    CHECK(ptr_data->date > 0);
    CHECK((ptr_data->date_usec >= 0) && (ptr_data->date_usec <= 999999));
    CHECK(ptr_data->date == ptr_data->date_printed);
    CHECK(ptr_data->date_usec == ptr_data->date_usec_printed);
    POINTERS_EQUAL(NULL, ptr_data->str_time);
    LONGS_EQUAL(0, ptr_data->tags_count);
    POINTERS_EQUAL(NULL, ptr_data->tags_array);
    LONGS_EQUAL(1, ptr_data->displayed);
    LONGS_EQUAL(0, ptr_data->notify_level);
    LONGS_EQUAL(0, ptr_data->highlight);
    LONGS_EQUAL(1, ptr_data->refresh_needed);
    POINTERS_EQUAL(NULL, ptr_data->prefix);
    LONGS_EQUAL(0, ptr_data->prefix_length);
    STRCMP_EQUAL("this is a test on line 3", ptr_data->message);

    /* delete first line */
    gui_chat_printf_y_datetime_tags (buffer, 0, 0, 0, NULL, "");
    ptr_data = buffer->own_lines->first_line->data;
    CHECK(ptr_data);
    POINTERS_EQUAL(buffer, ptr_data->buffer);
    LONGS_EQUAL(0, ptr_data->y);
    LONGS_EQUAL(0, ptr_data->date);
    LONGS_EQUAL(0, ptr_data->date_usec);
    LONGS_EQUAL(0, ptr_data->date_printed);
    LONGS_EQUAL(0, ptr_data->date_usec_printed);
    POINTERS_EQUAL(NULL, ptr_data->str_time);
    LONGS_EQUAL(0, ptr_data->tags_count);
    POINTERS_EQUAL(NULL, ptr_data->tags_array);
    LONGS_EQUAL(1, ptr_data->displayed);
    LONGS_EQUAL(0, ptr_data->notify_level);
    LONGS_EQUAL(0, ptr_data->highlight);
    LONGS_EQUAL(1, ptr_data->refresh_needed);
    POINTERS_EQUAL(NULL, ptr_data->prefix);
    LONGS_EQUAL(0, ptr_data->prefix_length);
    STRCMP_EQUAL("", ptr_data->message);

    /* delete third line */
    gui_chat_printf_y_datetime_tags (buffer, 2, 0, 0, NULL, "");
    CHECK(buffer->own_lines->first_line);
    CHECK(buffer->own_lines->first_line->next_line);
    POINTERS_EQUAL(NULL, buffer->own_lines->first_line->next_line->next_line);

    /* delete second line */
    gui_chat_printf_y_datetime_tags (buffer, 1, 0, 0, NULL, "");
    CHECK(buffer->own_lines->first_line);
    POINTERS_EQUAL(NULL, buffer->own_lines->first_line->next_line);

    gui_buffer_close (buffer);
}

/*
 * Tests functions:
 *   gui_chat_hsignal_quote_line_cb
 */

TEST(GuiChat, HsignalQuoteLineCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_chat_end
 */

TEST(GuiChat, ChatEnd)
{
    /* TODO: write tests */
}
