/*
 * SPDX-FileCopyrightText: 2022-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test input functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <ctype.h>
#include <regex.h>
#include <string.h>
#include "src/core/core-hook.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-history.h"
#include "src/gui/gui-input.h"
#include "src/gui/gui-key.h"
#include "src/gui/gui-mouse.h"
#include "src/gui/gui-window.h"
#include "src/plugins/weechat-plugin.h"

extern void gui_input_clipboard_copy (const char *buffer, int size);
extern void gui_input_delete_range (struct t_gui_buffer *buffer,
                                    char *start, char *end);
extern int gui_input_optimize_size (struct t_gui_buffer *buffer,
                                    int new_size, int new_length);
extern void gui_input_text_cursor_moved_signal (struct t_gui_buffer *buffer);
extern void gui_input_history_use_get_next (struct t_gui_buffer *buffer);
extern int gui_key_grab_command;
extern int gui_key_grab_delay;
}

TEST_GROUP(GuiInput)
{
};

/*
 * Test functions:
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
 * Test functions:
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
 * Test functions:
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
    STRCMP_EQUAL(NULL, gui_input_clipboard);

    gui_input_clipboard_copy ("abc", -1);
    STRCMP_EQUAL(NULL, gui_input_clipboard);

    gui_input_clipboard_copy ("abc", 0);
    STRCMP_EQUAL(NULL, gui_input_clipboard);

    gui_input_clipboard_copy ("abc", 1);
    STRCMP_EQUAL("a", gui_input_clipboard);

    gui_input_clipboard_copy ("abc", 3);
    STRCMP_EQUAL("abc", gui_input_clipboard);
}

/*
 * Test functions:
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
 * Test functions:
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
 * Test functions:
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
 * Test functions:
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
 * Test functions:
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
 * Test functions:
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
 * Test functions:
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
 * Test functions:
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
 * Test functions:
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
 * Test functions:
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
 * Test functions:
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
 * Test functions:
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
 * Test functions:
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
 * Test functions:
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
 * Test functions:
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
 * Test functions:
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
 * Test functions:
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
 * Test functions:
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
 * Test functions:
 *   gui_input_insert
 */

TEST(GuiInput, Insert)
{
    gui_input_replace_input (gui_buffers, "");

    gui_input_insert (gui_buffers, NULL, 1);
    STRCMP_EQUAL("", gui_buffers->input_buffer);
    gui_input_insert (gui_buffers, NULL, 0);
    STRCMP_EQUAL("", gui_buffers->input_buffer);

    gui_input_insert (gui_buffers, "a\\nb\\x41", 1);
    STRCMP_EQUAL("a\nbA", gui_buffers->input_buffer);
    LONGS_EQUAL(4, gui_buffers->input_buffer_pos);

    gui_input_replace_input (gui_buffers, "");

    gui_input_insert (gui_buffers, "a\\nb\\x41", 0);
    STRCMP_EQUAL("a\\nb\\x41", gui_buffers->input_buffer);
    LONGS_EQUAL(8, gui_buffers->input_buffer_pos);

    gui_input_replace_input (gui_buffers, "");
}

/*
 * Modifier "input_text_content": convert input to upper case.
 */

char *
test_input_modifier_upper_cb (const void *pointer, void *data,
                              const char *modifier, const char *modifier_data,
                              const char *string)
{
    char *result;
    int i;

    (void) pointer;
    (void) data;
    (void) modifier;
    (void) modifier_data;

    result = strdup (string);
    for (i = 0; result && result[i]; i++)
    {
        result[i] = toupper ((unsigned char)result[i]);
    }
    return result;
}

#define TEST_BUFFER_NAME "test_input"

/* Data received by the input callback of the test buffer. */
char test_input_data_received[4096];
int test_input_data_count = 0;

/* Number of signals received, and name of last signal. */
int test_input_signal_count = 0;
char test_input_signal[128];

TEST_GROUP(GuiInputWithBuffer)
{
    struct t_gui_buffer *buffer;
    struct t_gui_buffer *old_buffer;
    struct t_hook *hook_signal_input;

    /*
     * Input callback of the test buffer: store data received.
     */

    static int
    input_cb (const void *pointer, void *data,
              struct t_gui_buffer *buffer, const char *input_data)
    {
        (void) pointer;
        (void) data;
        (void) buffer;

        if (test_input_data_received[0])
        {
            strncat (test_input_data_received, "|",
                     sizeof (test_input_data_received)
                     - strlen (test_input_data_received) - 1);
        }
        strncat (test_input_data_received, input_data,
                 sizeof (test_input_data_received)
                 - strlen (test_input_data_received) - 1);
        test_input_data_count++;

        return WEECHAT_RC_OK;
    }

    /*
     * Callback for signals "input_*": count signals received.
     */

    static int
    signal_cb (const void *pointer, void *data, const char *signal,
               const char *type_data, void *signal_data)
    {
        (void) pointer;
        (void) data;
        (void) type_data;
        (void) signal_data;

        test_input_signal_count++;
        snprintf (test_input_signal, sizeof (test_input_signal),
                  "%s", signal);

        return WEECHAT_RC_OK;
    }

    void reset ()
    {
        test_input_data_received[0] = '\0';
        test_input_data_count = 0;
        test_input_signal_count = 0;
        test_input_signal[0] = '\0';
    }

    /*
     * Set input content and cursor position, then reset counters.
     */

    void set_input (const char *input, int pos)
    {
        gui_input_replace_input (buffer, input);
        buffer->input_buffer_pos = pos;
        reset ();
    }

    void setup ()
    {
        reset ();
        old_buffer = gui_windows->buffer;
        buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                                 &input_cb, NULL, NULL,
                                 NULL, NULL, NULL);
        CHECK(buffer);
        gui_window_switch_to_buffer (gui_windows, buffer, 0);
        POINTERS_EQUAL(buffer, gui_windows->buffer);
        hook_signal_input = hook_signal (NULL, "input_*", &signal_cb,
                                         NULL, NULL);
    }

    void teardown ()
    {
        unhook (hook_signal_input);
        gui_window_switch_to_buffer (gui_windows, old_buffer, 0);
        gui_buffer_close (buffer);
    }
};

/*
 * Test functions:
 *   gui_input_optimize_size
 */

TEST(GuiInputWithBuffer, OptimizeSize)
{
    /* Buffer without input: nothing changed. */
    buffer->input = 0;
    LONGS_EQUAL(0, gui_input_optimize_size (buffer, 10, 10));
    buffer->input = 1;

    LONGS_EQUAL(1, gui_input_optimize_size (buffer, 10, 8));
    LONGS_EQUAL(10, buffer->input_buffer_size);
    LONGS_EQUAL(8, buffer->input_buffer_length);
    LONGS_EQUAL(GUI_BUFFER_INPUT_BLOCK_SIZE, buffer->input_buffer_alloc);

    /* Allocated size is a multiple of the block size, with room for '\0'. */
    LONGS_EQUAL(1, gui_input_optimize_size (buffer,
                                            GUI_BUFFER_INPUT_BLOCK_SIZE,
                                            GUI_BUFFER_INPUT_BLOCK_SIZE));
    LONGS_EQUAL(2 * GUI_BUFFER_INPUT_BLOCK_SIZE, buffer->input_buffer_alloc);

    LONGS_EQUAL(1, gui_input_optimize_size (buffer, 0, 0));
    LONGS_EQUAL(GUI_BUFFER_INPUT_BLOCK_SIZE, buffer->input_buffer_alloc);
    LONGS_EQUAL(0, buffer->input_buffer_size);
}

/*
 * Test functions:
 *   gui_input_paste_pending_signal
 *   gui_input_text_cursor_moved_signal
 *   gui_input_search_signal
 */

TEST(GuiInputWithBuffer, Signals)
{
    gui_input_paste_pending_signal ();
    LONGS_EQUAL(1, test_input_signal_count);
    STRCMP_EQUAL("input_paste_pending", test_input_signal);

    gui_input_text_cursor_moved_signal (buffer);
    LONGS_EQUAL(2, test_input_signal_count);
    STRCMP_EQUAL("input_text_cursor_moved", test_input_signal);

    gui_input_search_signal (buffer);
    LONGS_EQUAL(3, test_input_signal_count);
    STRCMP_EQUAL("input_search", test_input_signal);
}

/*
 * Test functions:
 *   gui_input_text_changed_modifier_and_signal
 */

TEST(GuiInputWithBuffer, TextChangedModifierAndSignal)
{
    struct t_hook *hook;

    set_input ("test", 4);

    /* Signal sent, undo saved. */
    POINTERS_EQUAL(NULL, buffer->input_undo);
    gui_input_text_changed_modifier_and_signal (buffer, 1, 1);
    LONGS_EQUAL(1, test_input_signal_count);
    STRCMP_EQUAL("input_text_changed", test_input_signal);
    CHECK(buffer->input_undo);
    STRCMP_EQUAL("test", buffer->last_input_undo->data);

    /* Input changed by modifier "input_text_content". */
    hook = hook_modifier (NULL, "input_text_content",
                          &test_input_modifier_upper_cb, NULL, NULL);
    CHECK(hook);
    gui_input_text_changed_modifier_and_signal (buffer, 0, 1);
    STRCMP_EQUAL("TEST", buffer->input_buffer);
    unhook (hook);
}

/*
 * Test functions:
 *   gui_input_return
 *   gui_input_send_data_to_buffer
 */

TEST(GuiInputWithBuffer, Return)
{
    /* Empty input: nothing sent. */
    set_input ("", 0);
    gui_input_return (buffer);
    LONGS_EQUAL(0, test_input_data_count);

    /* Empty input sent if buffer accepts it. */
    gui_buffer_set (buffer, "input_get_empty", "1");
    gui_input_return (buffer);
    LONGS_EQUAL(1, test_input_data_count);
    STRCMP_EQUAL("", test_input_data_received);
    gui_buffer_set (buffer, "input_get_empty", "0");

    /* Input sent and cleared, added in history. */
    set_input ("hello", 3);
    gui_input_return (buffer);
    LONGS_EQUAL(1, test_input_data_count);
    STRCMP_EQUAL("hello", test_input_data_received);
    STRCMP_EQUAL("", buffer->input_buffer);
    LONGS_EQUAL(0, buffer->input_buffer_pos);
    CHECK(buffer->history);
    STRCMP_EQUAL("hello", buffer->history->text);

    /* Multi-line input: one line sent at a time. */
    set_input ("line1\nline2", 0);
    gui_input_return (buffer);
    LONGS_EQUAL(2, test_input_data_count);
    STRCMP_EQUAL("line1|line2", test_input_data_received);

    /* Multi-line input sent as-is if buffer accepts multi-line input. */
    gui_buffer_set (buffer, "input_multiline", "1");
    set_input ("line1\nline2", 0);
    gui_input_return (buffer);
    LONGS_EQUAL(1, test_input_data_count);
    STRCMP_EQUAL("line1\nline2", test_input_data_received);
    gui_buffer_set (buffer, "input_multiline", "0");

    /* Buffer without input: nothing sent. */
    set_input ("hello", 0);
    buffer->input = 0;
    gui_input_return (buffer);
    LONGS_EQUAL(0, test_input_data_count);
    buffer->input = 1;
}

/*
 * Test functions:
 *   gui_input_split_return
 */

TEST(GuiInputWithBuffer, SplitReturn)
{
    set_input ("", 0);
    gui_input_split_return (buffer);
    LONGS_EQUAL(0, test_input_data_count);

    /* Each line is sent separately. */
    set_input ("line1\nline2\nline3", 0);
    gui_input_split_return (buffer);
    LONGS_EQUAL(3, test_input_data_count);
    STRCMP_EQUAL("line1|line2|line3", test_input_data_received);
    STRCMP_EQUAL("", buffer->input_buffer);
    STRCMP_EQUAL("line3", buffer->history->text);
}

/*
 * Test functions:
 *   gui_input_search_text
 *   gui_input_search_text_here
 *   gui_input_search_history
 *   gui_input_search_switch_case
 *   gui_input_search_switch_regex
 *   gui_input_search_switch_where
 *   gui_input_search_stop
 *   gui_input_search_stop_here
 */

TEST(GuiInputWithBuffer, Search)
{
    /* No search: nothing done. */
    gui_input_search_switch_case (buffer);
    gui_input_search_switch_regex (buffer);
    gui_input_search_switch_where (buffer);
    gui_input_search_stop (buffer);
    gui_input_search_stop_here (buffer);
    LONGS_EQUAL(GUI_BUFFER_SEARCH_DISABLED, buffer->text_search);
    LONGS_EQUAL(0, test_input_signal_count);

    /* Search in lines. */
    gui_input_search_text (buffer);
    LONGS_EQUAL(GUI_BUFFER_SEARCH_LINES, buffer->text_search);
    STRCMP_EQUAL("input_search", test_input_signal);

    /* Search already started: nothing done. */
    reset ();
    gui_input_search_text (buffer);
    gui_input_search_text_here (buffer);
    gui_input_search_history (buffer);
    LONGS_EQUAL(GUI_BUFFER_SEARCH_LINES, buffer->text_search);
    LONGS_EQUAL(0, test_input_signal_count);

    /* Switch case and regex. */
    buffer->text_search_exact = 0;
    gui_input_search_switch_case (buffer);
    LONGS_EQUAL(1, buffer->text_search_exact);
    gui_input_search_switch_case (buffer);
    LONGS_EQUAL(0, buffer->text_search_exact);
    buffer->text_search_regex = 0;
    gui_input_search_switch_regex (buffer);
    LONGS_EQUAL(1, buffer->text_search_regex);
    gui_input_search_switch_regex (buffer);
    LONGS_EQUAL(0, buffer->text_search_regex);

    /* Switch where: message -> prefix -> both -> message. */
    buffer->text_search_where = GUI_BUFFER_SEARCH_IN_MESSAGE;
    gui_input_search_switch_where (buffer);
    LONGS_EQUAL(GUI_BUFFER_SEARCH_IN_PREFIX, buffer->text_search_where);
    gui_input_search_switch_where (buffer);
    LONGS_EQUAL(GUI_BUFFER_SEARCH_IN_MESSAGE | GUI_BUFFER_SEARCH_IN_PREFIX,
                buffer->text_search_where);
    gui_input_search_switch_where (buffer);
    LONGS_EQUAL(GUI_BUFFER_SEARCH_IN_MESSAGE, buffer->text_search_where);

    /* Stop search (input is restored, then signal "input_search" sent). */
    reset ();
    gui_input_search_stop (buffer);
    LONGS_EQUAL(GUI_BUFFER_SEARCH_DISABLED, buffer->text_search);
    STRCMP_EQUAL("input_search", test_input_signal);

    /* Search in lines from current position, stop here. */
    gui_input_search_text_here (buffer);
    LONGS_EQUAL(GUI_BUFFER_SEARCH_LINES, buffer->text_search);
    gui_input_search_stop_here (buffer);
    LONGS_EQUAL(GUI_BUFFER_SEARCH_DISABLED, buffer->text_search);

    /* Search in history: switch local/global history. */
    gui_input_search_history (buffer);
    LONGS_EQUAL(GUI_BUFFER_SEARCH_HISTORY, buffer->text_search);
    buffer->text_search_history = GUI_BUFFER_SEARCH_HISTORY_LOCAL;
    gui_input_search_switch_where (buffer);
    LONGS_EQUAL(GUI_BUFFER_SEARCH_HISTORY_GLOBAL, buffer->text_search_history);
    gui_input_search_switch_where (buffer);
    LONGS_EQUAL(GUI_BUFFER_SEARCH_HISTORY_LOCAL, buffer->text_search_history);
    gui_input_search_stop (buffer);
    LONGS_EQUAL(GUI_BUFFER_SEARCH_DISABLED, buffer->text_search);

    /* Buffer with free content: search where can not be changed. */
    gui_buffer_set (buffer, "type", "free");
    gui_input_search_text (buffer);
    buffer->text_search_where = GUI_BUFFER_SEARCH_IN_MESSAGE;
    gui_input_search_switch_where (buffer);
    LONGS_EQUAL(GUI_BUFFER_SEARCH_IN_MESSAGE, buffer->text_search_where);
    gui_input_search_stop (buffer);
}

/*
 * Test functions:
 *   gui_input_search_previous
 *   gui_input_search_next
 */

TEST(GuiInputWithBuffer, SearchPreviousNext)
{
    /* No search: nothing done. */
    gui_input_search_previous (buffer);
    gui_input_search_next (buffer);
    LONGS_EQUAL(GUI_BUFFER_SEARCH_DISABLED, buffer->text_search);

    gui_input_search_text (buffer);
    gui_input_search_previous (buffer);
    LONGS_EQUAL(GUI_BUFFER_SEARCH_DIR_BACKWARD, buffer->text_search_direction);
    gui_input_search_next (buffer);
    LONGS_EQUAL(GUI_BUFFER_SEARCH_DIR_FORWARD, buffer->text_search_direction);
    gui_input_search_stop (buffer);

    gui_input_search_history (buffer);
    gui_input_search_previous (buffer);
    LONGS_EQUAL(GUI_BUFFER_SEARCH_DIR_BACKWARD, buffer->text_search_direction);
    gui_input_search_next (buffer);
    LONGS_EQUAL(GUI_BUFFER_SEARCH_DIR_FORWARD, buffer->text_search_direction);
    gui_input_search_stop (buffer);
}

/*
 * Test functions:
 *   gui_input_search_compile_regex
 */

TEST(GuiInputWithBuffer, SearchCompileRegex)
{
    /* Not a regex search: nothing compiled. */
    buffer->text_search_regex = 0;
    set_input ("abc", 0);
    gui_input_search_compile_regex (buffer);
    POINTERS_EQUAL(NULL, buffer->text_search_regex_compiled);

    /* Invalid regex. */
    buffer->text_search_regex = 1;
    set_input ("a(b", 0);
    gui_input_search_compile_regex (buffer);
    POINTERS_EQUAL(NULL, buffer->text_search_regex_compiled);

    /* Valid regex, case insensitive. */
    buffer->text_search_exact = 0;
    set_input ("^a.c$", 0);
    gui_input_search_compile_regex (buffer);
    CHECK(buffer->text_search_regex_compiled);
    LONGS_EQUAL(0, regexec (buffer->text_search_regex_compiled, "AbC", 0,
                            NULL, 0));

    /* Valid regex, case sensitive (previous regex is freed). */
    buffer->text_search_exact = 1;
    gui_input_search_compile_regex (buffer);
    CHECK(buffer->text_search_regex_compiled);
    CHECK(regexec (buffer->text_search_regex_compiled, "AbC", 0,
                   NULL, 0) != 0);
    LONGS_EQUAL(0, regexec (buffer->text_search_regex_compiled, "abc", 0,
                            NULL, 0));

    /* Not a regex search any more: compiled regex is freed. */
    buffer->text_search_regex = 0;
    gui_input_search_compile_regex (buffer);
    POINTERS_EQUAL(NULL, buffer->text_search_regex_compiled);
}

/*
 * Test functions:
 *   gui_input_delete_beginning_of_input
 *   gui_input_delete_end_of_input
 *   gui_input_delete_input
 */

TEST(GuiInputWithBuffer, DeleteInput)
{
    /* Delete beginning of input (multiple lines). */
    set_input ("ab\ncdé\nf", 5);
    gui_input_delete_beginning_of_input (buffer);
    STRCMP_EQUAL("é\nf", buffer->input_buffer);
    LONGS_EQUAL(0, buffer->input_buffer_pos);
    LONGS_EQUAL(3, buffer->input_buffer_length);
    STRCMP_EQUAL("input_text_changed", test_input_signal);

    /* Cursor at beginning: nothing deleted. */
    reset ();
    gui_input_delete_beginning_of_input (buffer);
    STRCMP_EQUAL("é\nf", buffer->input_buffer);
    LONGS_EQUAL(0, test_input_signal_count);

    /* Delete end of input. */
    set_input ("ab\ncdé\nf", 4);
    gui_input_delete_end_of_input (buffer);
    STRCMP_EQUAL("ab\nc", buffer->input_buffer);
    LONGS_EQUAL(4, buffer->input_buffer_pos);
    LONGS_EQUAL(4, buffer->input_buffer_length);

    /* Cursor at end: nothing deleted. */
    gui_input_delete_end_of_input (buffer);
    STRCMP_EQUAL("ab\nc", buffer->input_buffer);

    /* Delete whole input. */
    set_input ("ab\ncd", 2);
    gui_input_delete_input (buffer);
    STRCMP_EQUAL("", buffer->input_buffer);
    LONGS_EQUAL(0, buffer->input_buffer_pos);
    LONGS_EQUAL(0, buffer->input_buffer_length);
}

/*
 * Test functions:
 *   gui_input_move_beginning_of_input
 *   gui_input_move_end_of_input
 */

TEST(GuiInputWithBuffer, MoveBeginningEndOfInput)
{
    set_input ("ab\ncdé", 4);
    gui_input_move_beginning_of_input (buffer);
    LONGS_EQUAL(0, buffer->input_buffer_pos);
    LONGS_EQUAL(1, test_input_signal_count);
    STRCMP_EQUAL("input_text_cursor_moved", test_input_signal);

    /* Already at beginning: no signal. */
    gui_input_move_beginning_of_input (buffer);
    LONGS_EQUAL(1, test_input_signal_count);

    gui_input_move_end_of_input (buffer);
    LONGS_EQUAL(6, buffer->input_buffer_pos);
    LONGS_EQUAL(2, test_input_signal_count);

    /* Already at end: no signal. */
    gui_input_move_end_of_input (buffer);
    LONGS_EQUAL(2, test_input_signal_count);
}

/*
 * Test functions:
 *   gui_input_move_previous_line
 *   gui_input_move_next_line
 */

TEST(GuiInputWithBuffer, MovePreviousNextLine)
{
    /* Lines: "abcd" (pos 0-4), "é" (5-6), "fghij" (7-12). */
    set_input ("abcd\né\nfghij", 10);

    /* Previous line is shorter: cursor at end of line. */
    gui_input_move_previous_line (buffer);
    LONGS_EQUAL(6, buffer->input_buffer_pos);
    gui_input_move_previous_line (buffer);
    LONGS_EQUAL(1, buffer->input_buffer_pos);

    /* First line: no move. */
    reset ();
    gui_input_move_previous_line (buffer);
    LONGS_EQUAL(1, buffer->input_buffer_pos);
    LONGS_EQUAL(0, test_input_signal_count);

    /* Next lines: same column if possible. */
    gui_input_move_next_line (buffer);
    LONGS_EQUAL(6, buffer->input_buffer_pos);
    gui_input_move_next_line (buffer);
    LONGS_EQUAL(8, buffer->input_buffer_pos);
    STRCMP_EQUAL("input_text_cursor_moved", test_input_signal);

    /* Last line: no move. */
    reset ();
    gui_input_move_next_line (buffer);
    LONGS_EQUAL(8, buffer->input_buffer_pos);
    LONGS_EQUAL(0, test_input_signal_count);

    /* Same column kept when lines are long enough. */
    set_input ("abcd\nefgh", 7);
    gui_input_move_previous_line (buffer);
    LONGS_EQUAL(2, buffer->input_buffer_pos);
    gui_input_move_next_line (buffer);
    LONGS_EQUAL(7, buffer->input_buffer_pos);

    /* Single line: no move. */
    set_input ("abcd", 2);
    gui_input_move_previous_line (buffer);
    LONGS_EQUAL(2, buffer->input_buffer_pos);
    gui_input_move_next_line (buffer);
    LONGS_EQUAL(2, buffer->input_buffer_pos);
}

/*
 * Test functions:
 *   gui_input_history_local_previous
 *   gui_input_history_local_next
 *   gui_input_history_previous
 *   gui_input_history_next
 */

TEST(GuiInputWithBuffer, HistoryLocal)
{
    /* Empty history: nothing done. */
    set_input ("", 0);
    gui_input_history_local_previous (buffer);
    STRCMP_EQUAL("", buffer->input_buffer);

    gui_history_add (buffer, "cmd1");
    gui_history_add (buffer, "cmd2");

    /* Recall previous commands, cursor at end. */
    gui_input_history_local_previous (buffer);
    STRCMP_EQUAL("cmd2", buffer->input_buffer);
    LONGS_EQUAL(4, buffer->input_buffer_pos);
    gui_input_history_local_previous (buffer);
    STRCMP_EQUAL("cmd1", buffer->input_buffer);

    /* Oldest command: stay on it. */
    gui_input_history_local_previous (buffer);
    STRCMP_EQUAL("cmd1", buffer->input_buffer);

    /* Next commands, then empty input. */
    gui_input_history_local_next (buffer);
    STRCMP_EQUAL("cmd2", buffer->input_buffer);
    gui_input_history_local_next (buffer);
    STRCMP_EQUAL("", buffer->input_buffer);
    POINTERS_EQUAL(NULL, buffer->ptr_history);

    /* Input typed then "down": input saved in history and cleared. */
    set_input ("typed", 5);
    gui_input_history_local_next (buffer);
    STRCMP_EQUAL("", buffer->input_buffer);
    STRCMP_EQUAL("typed", buffer->history->text);

    /* Input typed then "up": input saved, then recalled with "down". */
    set_input ("draft", 5);
    gui_input_history_local_previous (buffer);
    STRCMP_EQUAL("typed", buffer->input_buffer);
    gui_input_history_local_next (buffer);
    STRCMP_EQUAL("draft", buffer->input_buffer);

    /* An edited command is kept in history (like bash/readline). */
    set_input ("", 0);
    buffer->ptr_history = NULL;
    gui_input_history_local_previous (buffer);
    STRCMP_EQUAL("draft", buffer->input_buffer);
    gui_input_replace_input (buffer, "draft edited");
    gui_input_history_local_previous (buffer);
    gui_input_history_local_next (buffer);
    STRCMP_EQUAL("draft edited", buffer->input_buffer);

    /* Buffer without input: nothing done. */
    buffer->input = 0;
    gui_input_history_local_previous (buffer);
    gui_input_history_local_next (buffer);
    buffer->input = 1;
}

/*
 * Test functions:
 *   gui_input_history_global_previous
 *   gui_input_history_global_next
 */

TEST(GuiInputWithBuffer, HistoryGlobal)
{
    gui_history_global_free ();

    gui_history_add (buffer, "global1");
    gui_history_add (buffer, "global2");
    set_input ("", 0);

    gui_input_history_global_previous (buffer);
    STRCMP_EQUAL("global2", buffer->input_buffer);
    gui_input_history_global_previous (buffer);
    STRCMP_EQUAL("global1", buffer->input_buffer);
    gui_input_history_global_next (buffer);
    STRCMP_EQUAL("global2", buffer->input_buffer);
    gui_input_history_global_next (buffer);
    STRCMP_EQUAL("", buffer->input_buffer);
    POINTERS_EQUAL(NULL, gui_history_ptr);

    gui_history_global_free ();
}

/*
 * Test functions:
 *   gui_input_history_use_get_next
 */

TEST(GuiInputWithBuffer, HistoryUseGetNext)
{
    gui_history_add (buffer, "cmd1");
    gui_history_add (buffer, "cmd2");
    gui_history_add (buffer, "cmd3");
    set_input ("", 0);

    /* No history recalled: input is sent, nothing inserted. */
    set_input ("test", 4);
    buffer->ptr_history = NULL;
    gui_history_ptr = NULL;
    gui_input_history_use_get_next (buffer);
    STRCMP_EQUAL("test", test_input_data_received);
    STRCMP_EQUAL("", buffer->input_buffer);

    /* Recall "cmd1", send it and insert the next one ("cmd2"). */
    set_input ("", 0);
    buffer->ptr_history = NULL;
    gui_input_history_local_previous (buffer);
    gui_input_history_local_previous (buffer);
    gui_input_history_local_previous (buffer);
    gui_input_history_local_previous (buffer);
    STRCMP_EQUAL("cmd1", buffer->input_buffer);
    reset ();
    gui_input_history_use_get_next (buffer);
    STRCMP_EQUAL("cmd1", test_input_data_received);
    STRCMP_EQUAL("cmd2", buffer->input_buffer);
}

/*
 * Test functions:
 *   gui_input_grab_key
 *   gui_input_grab_mouse
 */

TEST(GuiInputWithBuffer, Grab)
{
    /* Buffer without input: no grab. */
    buffer->input = 0;
    gui_input_grab_key (buffer, 0, NULL);
    LONGS_EQUAL(0, gui_key_grab);
    gui_input_grab_mouse (buffer, 0);
    LONGS_EQUAL(0, gui_mouse_grab);
    buffer->input = 1;

    gui_input_grab_key (buffer, 1, "100");
    LONGS_EQUAL(1, gui_key_grab);
    LONGS_EQUAL(1, gui_key_grab_command);
    LONGS_EQUAL(100, gui_key_grab_delay);
    gui_key_grab = 0;
    gui_key_grab_command = 0;

    gui_input_grab_mouse (buffer, 0);
    LONGS_EQUAL(1, gui_mouse_grab);
    gui_mouse_grab = 0;

    /* Grab with area. */
    gui_input_grab_mouse (buffer, 1);
    LONGS_EQUAL(2, gui_mouse_grab);
    gui_mouse_grab = 0;
}

/*
 * Test functions:
 *   gui_input_undo
 *   gui_input_redo
 *   gui_input_undo_use
 */

TEST(GuiInputWithBuffer, UndoRedo)
{
    /* No undo: nothing done. */
    set_input ("", 0);
    gui_input_undo (buffer);
    gui_input_redo (buffer);
    STRCMP_EQUAL("", buffer->input_buffer);

    /* Three changes. */
    gui_input_insert (buffer, "a", 0);
    gui_input_insert (buffer, "b", 0);
    gui_input_insert (buffer, "c", 0);
    STRCMP_EQUAL("abc", buffer->input_buffer);

    gui_input_undo (buffer);
    STRCMP_EQUAL("ab", buffer->input_buffer);
    LONGS_EQUAL(2, buffer->input_buffer_pos);
    gui_input_undo (buffer);
    STRCMP_EQUAL("a", buffer->input_buffer);
    gui_input_undo (buffer);
    STRCMP_EQUAL("", buffer->input_buffer);

    /* Nothing more to undo. */
    gui_input_undo (buffer);
    STRCMP_EQUAL("", buffer->input_buffer);

    gui_input_redo (buffer);
    STRCMP_EQUAL("a", buffer->input_buffer);
    gui_input_redo (buffer);
    STRCMP_EQUAL("ab", buffer->input_buffer);
    gui_input_redo (buffer);
    STRCMP_EQUAL("abc", buffer->input_buffer);

    /* Nothing more to redo. */
    gui_input_redo (buffer);
    STRCMP_EQUAL("abc", buffer->input_buffer);

    /* Undo after a change not saved yet: current input is saved first. */
    gui_input_replace_input (buffer, "abcd");
    gui_input_undo (buffer);
    STRCMP_EQUAL("abc", buffer->input_buffer);
    gui_input_redo (buffer);
    STRCMP_EQUAL("abcd", buffer->input_buffer);
}
