/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

    /* save COLORFGBG to restore it at the end of the test */
    saved_colorfgbg = getenv ("COLORFGBG");
    colorfgbg = (saved_colorfgbg) ? strdup (saved_colorfgbg) : NULL;

    /* dark background ("fg;bg"): indices 0-6 and 8 */
    WEE_CHECK_THEME(0, "15;0");
    WEE_CHECK_THEME(0, "15;1");
    WEE_CHECK_THEME(0, "15;6");
    WEE_CHECK_THEME(0, "15;8");

    /* light background ("fg;bg"): index 7 and 9-15 */
    WEE_CHECK_THEME(1, "0;7");
    WEE_CHECK_THEME(1, "0;9");
    WEE_CHECK_THEME(1, "0;15");

    /* "fg;default;bg" form: last component is the background */
    WEE_CHECK_THEME(0, "0;default;0");
    WEE_CHECK_THEME(1, "0;default;15");

    /* restore COLORFGBG */
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
