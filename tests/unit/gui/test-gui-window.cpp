/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test window functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-layout.h"
#include "src/gui/gui-line.h"
#include "src/gui/gui-window.h"

extern void gui_buffer_visited_remove_all ();
}

TEST_GROUP(GuiWindow)
{
};

/*
 * Test functions:
 *   gui_window_valid
 */

TEST(GuiWindow, Valid)
{
    int dummy;

    LONGS_EQUAL(0, gui_window_valid (NULL));
    LONGS_EQUAL(0, gui_window_valid ((struct t_gui_window *)0x1));

    LONGS_EQUAL(1, gui_window_valid (gui_windows));
    LONGS_EQUAL(1, gui_window_valid (gui_current_window));

    /* Stack address: it can never be a window (allocated on heap). */
    LONGS_EQUAL(0, gui_window_valid ((struct t_gui_window *)&dummy));
}

/*
 * Test functions:
 *   gui_window_get_integer
 */

TEST(GuiWindow, GetInteger)
{
    struct t_gui_window window;

    LONGS_EQUAL(0, gui_window_get_integer (NULL, NULL));
    LONGS_EQUAL(0, gui_window_get_integer (NULL, "number"));
    LONGS_EQUAL(0, gui_window_get_integer (gui_windows, NULL));
    LONGS_EQUAL(0, gui_window_get_integer (gui_windows, "invalid"));

    LONGS_EQUAL(1, gui_window_get_integer (gui_windows, "number"));

    /* Window not in list: it must not be used. */
    memset (&window, 0, sizeof (window));
    window.number = 42;
    LONGS_EQUAL(0, gui_window_get_integer (&window, "number"));
}

/*
 * Test functions:
 *   gui_window_get_string
 */

TEST(GuiWindow, GetString)
{
    STRCMP_EQUAL(NULL, gui_window_get_string (NULL, NULL));
    STRCMP_EQUAL(NULL, gui_window_get_string (NULL, "invalid"));
    STRCMP_EQUAL(NULL, gui_window_get_string (gui_windows, NULL));
    STRCMP_EQUAL(NULL, gui_window_get_string (gui_windows, "invalid"));
}

/*
 * Test functions:
 *   gui_window_get_pointer
 */

TEST(GuiWindow, GetPointer)
{
    struct t_gui_window window;

    POINTERS_EQUAL(NULL, gui_window_get_pointer (NULL, NULL));
    POINTERS_EQUAL(NULL, gui_window_get_pointer (NULL, "buffer"));
    POINTERS_EQUAL(NULL, gui_window_get_pointer (gui_windows, NULL));
    POINTERS_EQUAL(NULL, gui_window_get_pointer (gui_windows, "invalid"));

    POINTERS_EQUAL(gui_current_window,
                   gui_window_get_pointer (NULL, "current"));
    POINTERS_EQUAL(gui_windows->buffer,
                   gui_window_get_pointer (gui_windows, "buffer"));

    /* Window not in list: it must not be used. */
    memset (&window, 0, sizeof (window));
    window.buffer = gui_buffers;
    POINTERS_EQUAL(gui_current_window,
                   gui_window_get_pointer (&window, "current"));
    POINTERS_EQUAL(NULL, gui_window_get_pointer (&window, "buffer"));
}

/*
 * Test functions:
 *   gui_window_get_context_at_xy
 */

TEST(GuiWindow, GetContextAtXy)
{
    struct t_gui_window window;
    struct t_gui_window_scroll scroll;
    struct t_gui_window_coords coords[3];
    struct t_gui_line line;
    struct t_gui_line_data line_data;
    char message[] = "aaaaaaaaaaaaaaaaaaaaaaaaa test";
    char *word, *focused_line, *focused_line_beginning, *focused_line_end;
    char *beginning, *end;
    int chat, line_x, i;
    struct t_gui_line *ptr_line;

    /*
     * Message displayed on 2 rows in a window with 20 columns, with the
     * long word wrapped in the middle and not aligned on second row
     * (option weechat.look.align_multiline_words = off):
     *
     *   row 0: "12:00 nick aaaaaaaaaa"  (message at x=10)
     *   row 1: "aaaaaaaaaaaaaaa test"   (message at x=0)
     */
    memset (&window, 0, sizeof (window));
    memset (&scroll, 0, sizeof (scroll));
    memset (&line, 0, sizeof (line));
    memset (&line_data, 0, sizeof (line_data));
    line_data.buffer = gui_buffers;
    line_data.message = message;
    line.data = &line_data;
    for (i = 0; i < 3; i++)
    {
        coords[i].line = NULL;
        coords[i].data = NULL;
        coords[i].data_x = -1;
        coords[i].time_x1 = -1;
        coords[i].time_x2 = -1;
        coords[i].buffer_x1 = -1;
        coords[i].buffer_x2 = -1;
        coords[i].prefix_x1 = -1;
        coords[i].prefix_x2 = -1;
    }
    coords[0].line = &line;
    coords[0].data = message;
    coords[0].data_x = 10;
    coords[1].line = &line;
    coords[1].data = message + 10;
    coords[1].data_x = 0;
    window.win_chat_width = 20;
    window.win_chat_height = 3;
    window.scroll = &scroll;
    window.coords = coords;
    window.coords_size = 3;

    /* not in a window */
    gui_window_get_context_at_xy (NULL, 0, 0,
                                  &chat, &ptr_line, &line_x, &word,
                                  &focused_line, &focused_line_beginning,
                                  &focused_line_end, &beginning, &end);
    LONGS_EQUAL(0, chat);
    POINTERS_EQUAL(NULL, ptr_line);

    /* outside chat area */
    gui_window_get_context_at_xy (&window, 20, 0,
                                  &chat, &ptr_line, &line_x, &word,
                                  &focused_line, &focused_line_beginning,
                                  &focused_line_end, &beginning, &end);
    LONGS_EQUAL(0, chat);
    POINTERS_EQUAL(NULL, ptr_line);

    /* empty row */
    gui_window_get_context_at_xy (&window, 0, 2,
                                  &chat, &ptr_line, &line_x, &word,
                                  &focused_line, &focused_line_beginning,
                                  &focused_line_end, &beginning, &end);
    LONGS_EQUAL(1, chat);
    POINTERS_EQUAL(NULL, ptr_line);
    LONGS_EQUAL(0, line_x);
    POINTERS_EQUAL(NULL, word);

    /* before message on first row */
    gui_window_get_context_at_xy (&window, 5, 0,
                                  &chat, &ptr_line, &line_x, &word,
                                  &focused_line, &focused_line_beginning,
                                  &focused_line_end, &beginning, &end);
    LONGS_EQUAL(1, chat);
    POINTERS_EQUAL(&line, ptr_line);
    LONGS_EQUAL(5, line_x);
    POINTERS_EQUAL(NULL, word);
    POINTERS_EQUAL(NULL, beginning);
    POINTERS_EQUAL(NULL, end);

    /* start of message on first row */
    gui_window_get_context_at_xy (&window, 10, 0,
                                  &chat, &ptr_line, &line_x, &word,
                                  &focused_line, &focused_line_beginning,
                                  &focused_line_end, &beginning, &end);
    LONGS_EQUAL(1, chat);
    POINTERS_EQUAL(&line, ptr_line);
    STRCMP_EQUAL("aaaaaaaaaaaaaaaaaaaaaaaaa", word);
    STRCMP_EQUAL("", beginning);
    STRCMP_EQUAL(message, end);
    STRCMP_EQUAL(message, focused_line);
    STRCMP_EQUAL("", focused_line_beginning);
    STRCMP_EQUAL(message, focused_line_end);
    free (word);
    free (focused_line);
    free (focused_line_beginning);
    free (focused_line_end);
    free (beginning);
    free (end);

    /* start of wrapped word on second row (not aligned) */
    gui_window_get_context_at_xy (&window, 0, 1,
                                  &chat, &ptr_line, &line_x, &word,
                                  &focused_line, &focused_line_beginning,
                                  &focused_line_end, &beginning, &end);
    LONGS_EQUAL(1, chat);
    POINTERS_EQUAL(&line, ptr_line);
    STRCMP_EQUAL("aaaaaaaaaaaaaaaaaaaaaaaaa", word);
    STRCMP_EQUAL("aaaaaaaaaa", beginning);
    STRCMP_EQUAL("aaaaaaaaaaaaaaa test", end);
    free (word);
    free (focused_line);
    free (focused_line_beginning);
    free (focused_line_end);
    free (beginning);
    free (end);

    /* word "test" on second row (not aligned) */
    gui_window_get_context_at_xy (&window, 16, 1,
                                  &chat, &ptr_line, &line_x, &word,
                                  &focused_line, &focused_line_beginning,
                                  &focused_line_end, &beginning, &end);
    LONGS_EQUAL(1, chat);
    POINTERS_EQUAL(&line, ptr_line);
    LONGS_EQUAL(16, line_x);
    STRCMP_EQUAL("test", word);
    STRCMP_EQUAL("aaaaaaaaaaaaaaaaaaaaaaaaa ", beginning);
    STRCMP_EQUAL("test", end);
    STRCMP_EQUAL(message, focused_line);
    STRCMP_EQUAL("aaaaaaaaaaaaaaaaaaaaaaaaa ", focused_line_beginning);
    STRCMP_EQUAL("test", focused_line_end);
    free (word);
    free (focused_line);
    free (focused_line_beginning);
    free (focused_line_end);
    free (beginning);
    free (end);

    /* last char of message on second row */
    gui_window_get_context_at_xy (&window, 19, 1,
                                  &chat, &ptr_line, &line_x, &word,
                                  &focused_line, &focused_line_beginning,
                                  &focused_line_end, &beginning, &end);
    LONGS_EQUAL(1, chat);
    POINTERS_EQUAL(&line, ptr_line);
    STRCMP_EQUAL("test", word);
    STRCMP_EQUAL("t", end);
    free (word);
    free (focused_line);
    free (focused_line_beginning);
    free (focused_line_end);
    free (beginning);
    free (end);
}

/*
 * Test functions:
 *   gui_window_zoom
 */

TEST(GuiWindow, Zoom)
{
    struct t_gui_buffer *buffer1, *buffer2;
    struct t_gui_window *ptr_other_window;

    gui_window_zoom (NULL);

    buffer1 = gui_buffer_new (NULL, "test1", NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(buffer1);
    buffer2 = gui_buffer_new (NULL, "test2", NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(buffer2);

    gui_buffer_visited_remove_all ();

    /* Two windows: test1 in top window (current), weechat in bottom window. */
    CHECK(gui_window_split_horizontal (gui_current_window, 50));
    gui_window_switch_to_buffer (gui_current_window, buffer1, 1);

    /* Zoom and switch to test2. */
    gui_window_zoom (gui_current_window);
    POINTERS_EQUAL(NULL, gui_windows->next_window);
    CHECK(gui_layout_search (GUI_LAYOUT_ZOOM));
    gui_window_switch_to_buffer (gui_current_window, buffer2, 1);
    LONGS_EQUAL(3, gui_buffers_visited_count);
    POINTERS_EQUAL(buffer2, last_gui_buffer_visited->buffer);
    POINTERS_EQUAL(buffer1, last_gui_buffer_visited->prev_buffer->buffer);

    /* Unzoom: test2 is kept in the zoomed window, visited buffers unchanged. */
    gui_window_zoom (gui_current_window);
    POINTERS_EQUAL(NULL, gui_layout_search (GUI_LAYOUT_ZOOM));
    CHECK(gui_windows->next_window);
    POINTERS_EQUAL(NULL, gui_windows->next_window->next_window);
    POINTERS_EQUAL(buffer2, gui_current_window->buffer);
    ptr_other_window = (gui_windows == gui_current_window) ?
        gui_windows->next_window : gui_windows;
    POINTERS_EQUAL(gui_buffers, ptr_other_window->buffer);
    LONGS_EQUAL(3, gui_buffers_visited_count);
    POINTERS_EQUAL(buffer2, last_gui_buffer_visited->buffer);
    POINTERS_EQUAL(buffer1, last_gui_buffer_visited->prev_buffer->buffer);

    gui_window_merge_all (gui_current_window);
    gui_window_switch_to_buffer (gui_current_window, gui_buffers, 1);
    gui_buffer_close (buffer1);
    gui_buffer_close (buffer2);
    gui_buffer_visited_remove_all ();
}

/*
 * Test functions:
 *   gui_window_layout_apply
 */

TEST(GuiWindow, LayoutApply)
{
    struct t_gui_layout *layout, *ptr_layout_zoom;

    gui_window_layout_apply (NULL);

    /* Layout with a single window. */
    layout = gui_layout_alloc ("test");
    CHECK(layout);
    gui_layout_add (layout);
    gui_layout_window_store (layout);

    /* No zoom. */
    CHECK(gui_window_split_horizontal (gui_current_window, 50));
    CHECK(gui_windows->next_window);
    gui_window_layout_apply (layout);
    POINTERS_EQUAL(NULL, gui_windows->next_window);
    POINTERS_EQUAL(NULL, gui_layout_search (GUI_LAYOUT_ZOOM));

    /* Zoom: the zoom is left, next zoom must zoom again. */
    CHECK(gui_window_split_horizontal (gui_current_window, 50));
    gui_window_zoom (gui_current_window);
    POINTERS_EQUAL(NULL, gui_windows->next_window);
    CHECK(gui_layout_search (GUI_LAYOUT_ZOOM));
    gui_window_layout_apply (layout);
    POINTERS_EQUAL(NULL, gui_windows->next_window);
    POINTERS_EQUAL(NULL, gui_layout_search (GUI_LAYOUT_ZOOM));
    CHECK(gui_window_split_horizontal (gui_current_window, 50));
    gui_window_zoom (gui_current_window);
    POINTERS_EQUAL(NULL, gui_windows->next_window);
    CHECK(gui_layout_search (GUI_LAYOUT_ZOOM));

    /* Apply the zoom layout: same as unzoom. */
    ptr_layout_zoom = gui_layout_search (GUI_LAYOUT_ZOOM);
    gui_window_layout_apply (ptr_layout_zoom);
    POINTERS_EQUAL(NULL, gui_layout_search (GUI_LAYOUT_ZOOM));
    CHECK(gui_windows->next_window);
    POINTERS_EQUAL(NULL, gui_windows->next_window->next_window);

    gui_window_merge_all (gui_current_window);
    gui_layout_remove (layout);
}
