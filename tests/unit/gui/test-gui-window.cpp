/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test window functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <string.h>
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-window.h"
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
