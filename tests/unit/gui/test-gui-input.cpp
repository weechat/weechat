/*
 * test-gui-input.cpp - test input functions
 *
 * Copyright (C) 2022-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/gui/gui-input.h"

extern void gui_input_clipboard_copy (const char *buffer, int size);
extern void gui_input_delete_range (struct t_gui_buffer *buffer,
                                    char *start, char *end);
}

TEST_GROUP(GuiInput)
{
};

/*
 * Tests functions:
 *   gui_input_optimize_size
 */

TEST(GuiInput, OptimizeSize)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_replace_input
 *   gui_input_set_pos
 */

TEST(GuiInput, ReplaceInputSetPos)
{
    gui_input_replace_input (gui_buffers, NULL);

    gui_input_replace_input (gui_buffers, "noël");
    STRCMP_EQUAL("noël", gui_buffers->input_buffer);
    gui_input_set_pos (gui_buffers, 4);
    LONGS_EQUAL(5, gui_buffers->input_buffer_size);
    LONGS_EQUAL(4, gui_buffers->input_buffer_length);
    LONGS_EQUAL(4, gui_buffers->input_buffer_pos);
    gui_input_set_pos (gui_buffers, 5);
    LONGS_EQUAL(5, gui_buffers->input_buffer_size);
    LONGS_EQUAL(4, gui_buffers->input_buffer_length);
    LONGS_EQUAL(4, gui_buffers->input_buffer_pos);

    gui_input_replace_input (gui_buffers, "");
    STRCMP_EQUAL("", gui_buffers->input_buffer);
    LONGS_EQUAL(0, gui_buffers->input_buffer_size);
    LONGS_EQUAL(0, gui_buffers->input_buffer_length);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);

    gui_input_set_pos (gui_buffers, 10);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
}

/*
 * Tests functions:
 *   gui_input_paste_pending_signal
 */

TEST(GuiInput, PastePendingSignal)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_text_changed_modifier_and_signal
 */

TEST(GuiInput, TextChangedModifierAndSignal)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_text_cursor_moved_signal
 */

TEST(GuiInput, TextCursorMovedSignal)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_search_signal
 */

TEST(GuiInput, SearchSignal)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_insert_string
 */

TEST(GuiInput, InsertString)
{
    gui_input_replace_input (gui_buffers, "");
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_insert_string (gui_buffers, NULL);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_insert_string (gui_buffers, "");
    STRCMP_EQUAL("", gui_buffers->input_buffer);
    LONGS_EQUAL(0, gui_buffers->input_buffer_size);
    LONGS_EQUAL(0, gui_buffers->input_buffer_length);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);

    gui_input_insert_string (gui_buffers, "noël");
    STRCMP_EQUAL("noël", gui_buffers->input_buffer);
    LONGS_EQUAL(5, gui_buffers->input_buffer_size);
    LONGS_EQUAL(4, gui_buffers->input_buffer_length);
    LONGS_EQUAL(4, gui_buffers->input_buffer_pos);

    gui_input_set_pos (gui_buffers, 3);
    LONGS_EQUAL(5, gui_buffers->input_buffer_size);
    LONGS_EQUAL(4, gui_buffers->input_buffer_length);
    LONGS_EQUAL(3, gui_buffers->input_buffer_pos);

    gui_input_insert_string (gui_buffers, "ï");
    STRCMP_EQUAL("noëïl", gui_buffers->input_buffer);
    LONGS_EQUAL(7, gui_buffers->input_buffer_size);
    LONGS_EQUAL(5, gui_buffers->input_buffer_length);
    LONGS_EQUAL(4, gui_buffers->input_buffer_pos);
}

/*
 * Tests functions:
 *   gui_input_clipboard_copy
 */

TEST(GuiInput, ClipboardCopy)
{
    if (gui_input_clipboard)
    {
        free (gui_input_clipboard);
        gui_input_clipboard = NULL;
    }

    gui_input_clipboard_copy (NULL, 1);
    POINTERS_EQUAL(NULL, gui_input_clipboard);

    gui_input_clipboard_copy ("abc", -1);
    POINTERS_EQUAL(NULL, gui_input_clipboard);

    gui_input_clipboard_copy ("abc", 0);
    POINTERS_EQUAL(NULL, gui_input_clipboard);

    gui_input_clipboard_copy ("abc", 1);
    STRCMP_EQUAL("a", gui_input_clipboard);

    gui_input_clipboard_copy ("abc", 3);
    STRCMP_EQUAL("abc", gui_input_clipboard);
}

/*
 * Tests functions:
 *   gui_input_clipboard_paste
 */

TEST(GuiInput, ClipboardPaste)
{
    gui_input_replace_input (gui_buffers, "");

    gui_input_clipboard_copy ("abc", 3);
    gui_input_clipboard_paste (gui_buffers);
    STRCMP_EQUAL("abc", gui_buffers->input_buffer);

    gui_input_set_pos (gui_buffers, 2);
    gui_input_clipboard_copy ("def", 3);
    gui_input_clipboard_paste (gui_buffers);
    STRCMP_EQUAL("abdefc", gui_buffers->input_buffer);
}

/*
 * Tests functions:
 *   gui_input_return
 */

TEST(GuiInput, Return)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_complete
 *   gui_input_complete_next
 *   gui_input_complete_previous
 */

TEST(GuiInput, Complete)
{
    gui_input_replace_input (gui_buffers, "");
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_insert_string (gui_buffers, "/wa");

    gui_input_complete_next (gui_buffers);
    STRCMP_EQUAL("/wait ", gui_buffers->input_buffer);
    gui_input_complete_next (gui_buffers);
    STRCMP_EQUAL("/wallchops ", gui_buffers->input_buffer);
    gui_input_complete_next (gui_buffers);
    STRCMP_EQUAL("/wallops ", gui_buffers->input_buffer);
    gui_input_complete_next (gui_buffers);
    STRCMP_EQUAL("/wait ", gui_buffers->input_buffer);

    gui_input_complete_previous (gui_buffers);
    STRCMP_EQUAL("/wallops ", gui_buffers->input_buffer);
    gui_input_complete_previous (gui_buffers);
    STRCMP_EQUAL("/wallchops ", gui_buffers->input_buffer);
    gui_input_complete_previous (gui_buffers);
    STRCMP_EQUAL("/wait ", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "");
}

/*
 * Tests functions:
 *   gui_input_search_text_here
 */

TEST(GuiInput, SearchTextHere)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_search_text
 */

TEST(GuiInput, SearchText)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_search_compile_regex
 */

TEST(GuiInput, SearchCompileRegex)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_search_switch_case
 */

TEST(GuiInput, SearchSwitchCase)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_search_switch_regex
 */

TEST(GuiInput, SearchSwitchRegex)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_search_switch_where
 */

TEST(GuiInput, SearchSwitchWhere)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_search_previous
 */

TEST(GuiInput, SearchPrevious)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_search_next
 */

TEST(GuiInput, SearchNext)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_search_stop_here
 */

TEST(GuiInput, SearchStopHere)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_search_stop
 */

TEST(GuiInput, SearchStop)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_delete_previous_char
 */

TEST(GuiInput, DeletePreviousChar)
{
    gui_input_replace_input (gui_buffers, "");
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_delete_previous_char (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc");
    gui_input_set_pos (gui_buffers, 0);
    gui_input_delete_previous_char (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("abc", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc");
    gui_input_set_pos (gui_buffers, 1);
    gui_input_delete_previous_char (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("bc", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc");
    gui_input_set_pos (gui_buffers, 2);
    gui_input_delete_previous_char (gui_buffers);
    LONGS_EQUAL(1, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("ac", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc");
    gui_input_set_pos (gui_buffers, 3);
    gui_input_delete_previous_char (gui_buffers);
    LONGS_EQUAL(2, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("ab", gui_buffers->input_buffer);
    gui_input_delete_previous_char (gui_buffers);
    LONGS_EQUAL(1, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("a", gui_buffers->input_buffer);
    gui_input_delete_previous_char (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);
    gui_input_delete_previous_char (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);
}

/*
 * Tests functions:
 *   gui_input_delete_next_char
 */

TEST(GuiInput, DeleteNextChar)
{
    gui_input_replace_input (gui_buffers, "");
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_delete_next_char (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc");
    gui_input_set_pos (gui_buffers, 3);
    gui_input_delete_next_char (gui_buffers);
    LONGS_EQUAL(3, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("abc", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc");
    gui_input_set_pos (gui_buffers, 2);
    gui_input_delete_next_char (gui_buffers);
    LONGS_EQUAL(2, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("ab", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc");
    gui_input_set_pos (gui_buffers, 1);
    gui_input_delete_next_char (gui_buffers);
    LONGS_EQUAL(1, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("ac", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc");
    gui_input_set_pos (gui_buffers, 0);
    gui_input_delete_next_char (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("bc", gui_buffers->input_buffer);
    gui_input_delete_next_char (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("c", gui_buffers->input_buffer);
    gui_input_delete_next_char (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);
    gui_input_delete_next_char (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);
}

/*
 * Tests functions:
 *   gui_input_delete_range
 */

TEST(GuiInput, DeleteRange)
{
    gui_input_replace_input (gui_buffers, "abcdef");
    gui_input_set_pos (gui_buffers, 6);

    gui_input_delete_range (gui_buffers,
                            gui_buffers->input_buffer,
                            gui_buffers->input_buffer);
    STRCMP_EQUAL("bcdef", gui_buffers->input_buffer);
    LONGS_EQUAL(5, gui_buffers->input_buffer_pos);
    gui_input_clipboard_paste (gui_buffers);
    STRCMP_EQUAL("bcdefa", gui_buffers->input_buffer);
    LONGS_EQUAL(6, gui_buffers->input_buffer_pos);

    gui_input_delete_range (gui_buffers,
                            gui_buffers->input_buffer,
                            gui_buffers->input_buffer + 2);
    STRCMP_EQUAL("efa", gui_buffers->input_buffer);
    LONGS_EQUAL(3, gui_buffers->input_buffer_pos);
    gui_input_clipboard_paste (gui_buffers);
    STRCMP_EQUAL("efabcd", gui_buffers->input_buffer);
    LONGS_EQUAL(6, gui_buffers->input_buffer_pos);

    gui_input_delete_range (gui_buffers,
                            gui_buffers->input_buffer,
                            gui_buffers->input_buffer + 5);
    STRCMP_EQUAL("", gui_buffers->input_buffer);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    gui_input_clipboard_paste (gui_buffers);
    STRCMP_EQUAL("efabcd", gui_buffers->input_buffer);
    LONGS_EQUAL(6, gui_buffers->input_buffer_pos);
}

/*
 * Tests functions:
 *   gui_input_delete_previous_word
 */

TEST(GuiInput, DeletePreviousWord)
{
    gui_input_replace_input (gui_buffers, "");
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_delete_previous_word (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc");
    gui_input_set_pos (gui_buffers, 2);
    gui_input_delete_previous_word (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("c", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc");
    gui_input_set_pos (gui_buffers, 3);
    gui_input_delete_previous_word (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc  ");
    gui_input_set_pos (gui_buffers, 5);
    gui_input_delete_previous_word (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc def");
    gui_input_set_pos (gui_buffers, 7);
    gui_input_delete_previous_word (gui_buffers);
    LONGS_EQUAL(4, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("abc ", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc def/ghi/jkl");
    gui_input_set_pos (gui_buffers, 15);
    gui_input_delete_previous_word (gui_buffers);
    LONGS_EQUAL(12, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("abc def/ghi/", gui_buffers->input_buffer);
    gui_input_delete_previous_word (gui_buffers);
    LONGS_EQUAL(8, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("abc def/", gui_buffers->input_buffer);
    gui_input_delete_previous_word (gui_buffers);
    LONGS_EQUAL(4, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("abc ", gui_buffers->input_buffer);
    gui_input_delete_previous_word (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);
}

/*
 * Tests functions:
 *   gui_input_delete_previous_word_whitespace
 */

TEST(GuiInput, DeletePreviousWordWhitespace)
{
    gui_input_replace_input (gui_buffers, "");
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_delete_previous_word_whitespace (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc");
    gui_input_set_pos (gui_buffers, 2);
    gui_input_delete_previous_word_whitespace (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("c", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc");
    gui_input_set_pos (gui_buffers, 3);
    gui_input_delete_previous_word_whitespace (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc  ");
    gui_input_set_pos (gui_buffers, 5);
    gui_input_delete_previous_word_whitespace (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc def");
    gui_input_set_pos (gui_buffers, 7);
    gui_input_delete_previous_word_whitespace (gui_buffers);
    LONGS_EQUAL(4, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("abc ", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc def/ghi/jkl");
    gui_input_set_pos (gui_buffers, 15);
    gui_input_delete_previous_word_whitespace (gui_buffers);
    LONGS_EQUAL(4, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("abc ", gui_buffers->input_buffer);
    gui_input_delete_previous_word_whitespace (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);
}

/*
 * Tests functions:
 *   gui_input_delete_next_word
 */

TEST(GuiInput, DeleteNextWord)
{
    gui_input_replace_input (gui_buffers, "");
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_delete_next_word (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc");
    gui_input_set_pos (gui_buffers, 0);
    gui_input_delete_next_word (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc");
    gui_input_set_pos (gui_buffers, 1);
    gui_input_delete_next_word (gui_buffers);
    LONGS_EQUAL(1, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("a", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "  abc");
    gui_input_set_pos (gui_buffers, 0);
    gui_input_delete_next_word (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc def");
    gui_input_set_pos (gui_buffers, 0);
    gui_input_delete_next_word (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL(" def", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc def/ghi/jkl");
    gui_input_set_pos (gui_buffers, 0);
    gui_input_delete_next_word (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL(" def/ghi/jkl", gui_buffers->input_buffer);
    gui_input_delete_next_word (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("/ghi/jkl", gui_buffers->input_buffer);
    gui_input_delete_next_word (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("/jkl", gui_buffers->input_buffer);
    gui_input_delete_next_word (gui_buffers);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
    STRCMP_EQUAL("", gui_buffers->input_buffer);
}

/*
 * Tests functions:
 *   gui_input_delete_beginning_of_line
 */

TEST(GuiInput, DeleteBeginningOfLine)
{
    gui_input_replace_input (gui_buffers, "");
    gui_input_delete_beginning_of_line (gui_buffers);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abcdef");
    gui_input_set_pos (gui_buffers, 0);
    gui_input_delete_beginning_of_line (gui_buffers);
    STRCMP_EQUAL("abcdef", gui_buffers->input_buffer);

    gui_input_set_pos (gui_buffers, 3);
    gui_input_delete_beginning_of_line (gui_buffers);
    STRCMP_EQUAL("def", gui_buffers->input_buffer);

    gui_input_set_pos (gui_buffers, 3);
    gui_input_delete_beginning_of_line (gui_buffers);
    STRCMP_EQUAL("", gui_buffers->input_buffer);
}

/*
 * Tests functions:
 *   gui_input_delete_end_of_line
 */

TEST(GuiInput, DeleteEndOfLine)
{
    gui_input_replace_input (gui_buffers, "");
    gui_input_delete_end_of_line (gui_buffers);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abcdef");
    gui_input_set_pos (gui_buffers, 6);
    gui_input_delete_end_of_line (gui_buffers);
    STRCMP_EQUAL("abcdef", gui_buffers->input_buffer);

    gui_input_set_pos (gui_buffers, 3);
    gui_input_delete_end_of_line (gui_buffers);
    STRCMP_EQUAL("abc", gui_buffers->input_buffer);

    gui_input_set_pos (gui_buffers, 0);
    gui_input_delete_end_of_line (gui_buffers);
    STRCMP_EQUAL("", gui_buffers->input_buffer);
}

/*
 * Tests functions:
 *   gui_input_delete_line
 */

TEST(GuiInput, DeleteLine)
{
    gui_input_replace_input (gui_buffers, "");
    gui_input_delete_line (gui_buffers);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abcdef");
    gui_input_set_pos (gui_buffers, 6);
    gui_input_delete_line (gui_buffers);
    STRCMP_EQUAL("", gui_buffers->input_buffer);
}

/*
 * Tests functions:
 *   gui_input_transpose_chars
 */

TEST(GuiInput, TransposeChars)
{
    gui_input_replace_input (gui_buffers, "");
    gui_input_transpose_chars (gui_buffers);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "abc");
    gui_input_set_pos (gui_buffers, 0);
    gui_input_transpose_chars (gui_buffers);
    STRCMP_EQUAL("abc", gui_buffers->input_buffer);

    gui_input_set_pos (gui_buffers, 1);
    gui_input_transpose_chars (gui_buffers);
    STRCMP_EQUAL("bac", gui_buffers->input_buffer);

    gui_input_set_pos (gui_buffers, 3);
    gui_input_transpose_chars (gui_buffers);
    STRCMP_EQUAL("bca", gui_buffers->input_buffer);

    gui_input_replace_input (gui_buffers, "");
}

/*
 * Tests functions:
 *   gui_input_move_beginning_of_line
 */

TEST(GuiInput, MoveBeginningOfLine)
{
    gui_input_replace_input (gui_buffers, "");
    gui_input_move_beginning_of_line (gui_buffers);
    STRCMP_EQUAL("", gui_buffers->input_buffer);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);

    gui_input_replace_input (gui_buffers, "abc");
    gui_input_set_pos (gui_buffers, 0);
    gui_input_move_beginning_of_line (gui_buffers);
    STRCMP_EQUAL("abc", gui_buffers->input_buffer);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);

    gui_input_set_pos (gui_buffers, 3);
    gui_input_move_beginning_of_line (gui_buffers);
    STRCMP_EQUAL("abc", gui_buffers->input_buffer);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);

    gui_input_replace_input (gui_buffers, "");
}

/*
 * Tests functions:
 *   gui_input_move_end_of_line
 */

TEST(GuiInput, MoveEndOfLine)
{
    gui_input_replace_input (gui_buffers, "");
    gui_input_move_end_of_line (gui_buffers);
    STRCMP_EQUAL("", gui_buffers->input_buffer);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);

    gui_input_replace_input (gui_buffers, "abc");
    gui_input_set_pos (gui_buffers, 3);
    gui_input_move_end_of_line (gui_buffers);
    STRCMP_EQUAL("abc", gui_buffers->input_buffer);
    LONGS_EQUAL(3, gui_buffers->input_buffer_pos);

    gui_input_set_pos (gui_buffers, 0);
    gui_input_move_end_of_line (gui_buffers);
    STRCMP_EQUAL("abc", gui_buffers->input_buffer);
    LONGS_EQUAL(3, gui_buffers->input_buffer_pos);

    gui_input_replace_input (gui_buffers, "");
}

/*
 * Tests functions:
 *   gui_input_move_previous_char
 */

TEST(GuiInput, MovePreviousChar)
{
    gui_input_replace_input (gui_buffers, "");
    gui_input_move_previous_char (gui_buffers);
    STRCMP_EQUAL("", gui_buffers->input_buffer);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);

    gui_input_replace_input (gui_buffers, "abc");
    gui_input_set_pos (gui_buffers, 3);
    gui_input_move_previous_char (gui_buffers);
    STRCMP_EQUAL("abc", gui_buffers->input_buffer);
    LONGS_EQUAL(2, gui_buffers->input_buffer_pos);

    gui_input_move_previous_char (gui_buffers);
    STRCMP_EQUAL("abc", gui_buffers->input_buffer);
    LONGS_EQUAL(1, gui_buffers->input_buffer_pos);

    gui_input_move_previous_char (gui_buffers);
    STRCMP_EQUAL("abc", gui_buffers->input_buffer);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);

    gui_input_move_previous_char (gui_buffers);
    STRCMP_EQUAL("abc", gui_buffers->input_buffer);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);

    gui_input_replace_input (gui_buffers, "");
}

/*
 * Tests functions:
 *   gui_input_move_next_char
 */

TEST(GuiInput, MoveNextChar)
{
    gui_input_replace_input (gui_buffers, "");
    gui_input_move_next_char (gui_buffers);
    STRCMP_EQUAL("", gui_buffers->input_buffer);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);

    gui_input_replace_input (gui_buffers, "abc");
    gui_input_set_pos (gui_buffers, 0);

    gui_input_move_next_char (gui_buffers);
    STRCMP_EQUAL("abc", gui_buffers->input_buffer);
    LONGS_EQUAL(1, gui_buffers->input_buffer_pos);

    gui_input_move_next_char (gui_buffers);
    STRCMP_EQUAL("abc", gui_buffers->input_buffer);
    LONGS_EQUAL(2, gui_buffers->input_buffer_pos);

    gui_input_move_next_char (gui_buffers);
    STRCMP_EQUAL("abc", gui_buffers->input_buffer);
    LONGS_EQUAL(3, gui_buffers->input_buffer_pos);

    gui_input_move_next_char (gui_buffers);
    STRCMP_EQUAL("abc", gui_buffers->input_buffer);
    LONGS_EQUAL(3, gui_buffers->input_buffer_pos);

    gui_input_replace_input (gui_buffers, "");
}

/*
 * Tests functions:
 *   gui_input_move_previous_word
 */

TEST(GuiInput, MovePreviousWord)
{
    gui_input_replace_input (gui_buffers, "");
    gui_input_move_previous_word (gui_buffers);
    STRCMP_EQUAL("", gui_buffers->input_buffer);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);

    gui_input_replace_input (gui_buffers, "abc/def");
    gui_input_set_pos (gui_buffers, 0);

    gui_input_move_previous_word (gui_buffers);
    STRCMP_EQUAL("abc/def", gui_buffers->input_buffer);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);

    gui_input_set_pos (gui_buffers, 7);

    gui_input_move_previous_word (gui_buffers);
    STRCMP_EQUAL("abc/def", gui_buffers->input_buffer);
    LONGS_EQUAL(4, gui_buffers->input_buffer_pos);

    gui_input_move_previous_word (gui_buffers);
    STRCMP_EQUAL("abc/def", gui_buffers->input_buffer);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);

    gui_input_move_previous_word (gui_buffers);
    STRCMP_EQUAL("abc/def", gui_buffers->input_buffer);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);
}

/*
 * Tests functions:
 *   gui_input_move_next_word
 */

TEST(GuiInput, MoveNextWord)
{
    gui_input_replace_input (gui_buffers, "");
    gui_input_move_next_word (gui_buffers);
    STRCMP_EQUAL("", gui_buffers->input_buffer);
    LONGS_EQUAL(0, gui_buffers->input_buffer_pos);

    gui_input_replace_input (gui_buffers, "abc/def");
    gui_input_set_pos (gui_buffers, 7);

    gui_input_move_next_word (gui_buffers);
    STRCMP_EQUAL("abc/def", gui_buffers->input_buffer);
    LONGS_EQUAL(7, gui_buffers->input_buffer_pos);

    gui_input_set_pos (gui_buffers, 0);

    gui_input_move_next_word (gui_buffers);
    STRCMP_EQUAL("abc/def", gui_buffers->input_buffer);
    LONGS_EQUAL(3, gui_buffers->input_buffer_pos);

    gui_input_move_next_word (gui_buffers);
    STRCMP_EQUAL("abc/def", gui_buffers->input_buffer);
    LONGS_EQUAL(7, gui_buffers->input_buffer_pos);

    gui_input_move_next_word (gui_buffers);
    STRCMP_EQUAL("abc/def", gui_buffers->input_buffer);
    LONGS_EQUAL(7, gui_buffers->input_buffer_pos);
}

/*
 * Tests functions:
 *   gui_input_history_previous
 */

TEST(GuiInput, HistoryPrevious)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_history_next
 */

TEST(GuiInput, HistoryNext)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_history_local_previous
 */

TEST(GuiInput, HistoryLocalPrevious)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_history_local_next
 */

TEST(GuiInput, HistoryLocalNext)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_history_global_previous
 */

TEST(GuiInput, HistoryGlobalPrevious)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_history_global_next
 */

TEST(GuiInput, HistoryGlobalNext)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_grab_key
 */

TEST(GuiInput, GrabKey)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_grab_mouse
 */

TEST(GuiInput, GrabMouse)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_insert
 */

TEST(GuiInput, Insert)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_undo_use
 */

TEST(GuiInput, UndoUse)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_undo
 */

TEST(GuiInput, Undo)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_input_redo
 */

TEST(GuiInput, Redo)
{
    /* TODO: write tests */
}
