/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test terminal functions (Curses interface) */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>

extern int gui_term_theme_is_light (void);
}

/*
 * Asserts the value returned by gui_term_theme_is_light() when COLORFGBG
 * is set to the given value (a definitive background index, so detection
 * returns without falling back to the terminal query on /dev/tty).
 */

#define WEE_CHECK_THEME(__result, __colorfgbg)                          \
    setenv ("COLORFGBG", __colorfgbg, 1);                               \
    LONGS_EQUAL(__result, gui_term_theme_is_light ());

TEST_GROUP(GuiCursesTerm)
{
};

/*
 * Test functions:
 *   gui_term_theme_is_light
 */

TEST(GuiCursesTerm, ThemeIsLight)
{
    const char *saved_colorfgbg;
    char *colorfgbg;

    /* Save COLORFGBG to restore it at the end of the test. */
    saved_colorfgbg = getenv ("COLORFGBG");
    colorfgbg = (saved_colorfgbg) ? strdup (saved_colorfgbg) : NULL;

    /* Dark background ("fg;bg"): indices 0-6 and 8 */
    WEE_CHECK_THEME(0, "15;0");
    WEE_CHECK_THEME(0, "15;1");
    WEE_CHECK_THEME(0, "15;6");
    WEE_CHECK_THEME(0, "15;8");

    /* Light background ("fg;bg"): index 7 and 9-15 */
    WEE_CHECK_THEME(1, "0;7");
    WEE_CHECK_THEME(1, "0;9");
    WEE_CHECK_THEME(1, "0;15");

    /* "fg;default;bg" form: last component is the background */
    WEE_CHECK_THEME(0, "0;default;0");
    WEE_CHECK_THEME(1, "0;default;15");

    /* Restore COLORFGBG. */
    if (colorfgbg)
    {
        setenv ("COLORFGBG", colorfgbg, 1);
        free (colorfgbg);
    }
    else
    {
        unsetenv ("COLORFGBG");
    }
}
