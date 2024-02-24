/*
 * test-gui-curses-mouse.cpp - test mouse functions (Curses interface)
 *
 * Copyright (C) 2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/gui/gui-mouse.h"

extern const char *gui_mouse_event_concat_gesture (const char *key);
extern const char *gui_mouse_event_name_sgr (const char *key);
extern const char *gui_mouse_event_name_utf8 (const char *key);
}

#define WEE_CHECK_GESTURE(__result, __x1, __y1, __x2, __y2)             \
    key[0] = '\0';                                                      \
    gui_mouse_event_x[0] = __x1;                                        \
    gui_mouse_event_y[0] = __y1;                                        \
    gui_mouse_event_x[1] = __x2;                                        \
    gui_mouse_event_y[1] = __y2;                                        \
    gui_mouse_event_concat_gesture (key);                               \
    STRCMP_EQUAL(__result, key);

#define WEE_CHECK_EVENT_SGR(__event, __index, __x1, __y1, __x2, __y2,   \
                            __key)                                      \
    STRCMP_EQUAL(__event, gui_mouse_event_name_sgr (__key));            \
    LONGS_EQUAL(__index, gui_mouse_event_index);                        \
    LONGS_EQUAL(__x1, gui_mouse_event_x[0]);                            \
    LONGS_EQUAL(__y1, gui_mouse_event_y[0]);                            \
    LONGS_EQUAL(__x2, gui_mouse_event_x[1]);                            \
    LONGS_EQUAL(__y2, gui_mouse_event_y[1]);

#define WEE_CHECK_EVENT_UTF8(__event, __index, __x1, __y1, __x2, __y2,  \
                             __key)                                     \
    STRCMP_EQUAL(__event, gui_mouse_event_name_utf8 (__key));           \
    LONGS_EQUAL(__index, gui_mouse_event_index);                        \
    LONGS_EQUAL(__x1, gui_mouse_event_x[0]);                            \
    LONGS_EQUAL(__y1, gui_mouse_event_y[0]);                            \
    LONGS_EQUAL(__x2, gui_mouse_event_x[1]);                            \
    LONGS_EQUAL(__y2, gui_mouse_event_y[1]);

TEST_GROUP(GuiCursesMouse)
{
};


/*
 * Tests functions:
 *   gui_mouse_enable
 */

TEST(GuiCursesMouse, Enable)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_mouse_disable
 */

TEST(GuiCursesMouse, Disable)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_mouse_display_state
 */

TEST(GuiCursesMouse, DisplayState)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_mouse_grab_init
 */

TEST(GuiCursesMouse, GrabInit)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_mouse_grab_event2input
 */

TEST(GuiCursesMouse, GrabEvent2input)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_mouse_grab_end
 */

TEST(GuiCursesMouse, GrabEnd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_mouse_event_size
 */

TEST(GuiCursesMouse, EventSize)
{
    LONGS_EQUAL(-1, gui_mouse_event_size (NULL));
    LONGS_EQUAL(-1, gui_mouse_event_size (""));
    LONGS_EQUAL(-1, gui_mouse_event_size ("a"));
    LONGS_EQUAL(-1, gui_mouse_event_size ("test"));

    /* SGR event */
    LONGS_EQUAL(0, gui_mouse_event_size ("\x01[[<"));
    LONGS_EQUAL(0, gui_mouse_event_size ("\x01[[<0"));
    LONGS_EQUAL(0, gui_mouse_event_size ("\x01[[<0;12"));
    LONGS_EQUAL(0, gui_mouse_event_size ("\x01[[<0;12;34"));
    LONGS_EQUAL(12, gui_mouse_event_size ("\x01[[<0;12;34M"));
    LONGS_EQUAL(12, gui_mouse_event_size ("\x01[[<0;12;34m"));
    LONGS_EQUAL(12, gui_mouse_event_size ("\x01[[<0;12;34MABC"));
    LONGS_EQUAL(12, gui_mouse_event_size ("\x01[[<0;12;34M\x01[[<0;12;34m"));

    /* UTF-8 event */
    LONGS_EQUAL(0, gui_mouse_event_size ("\x01[[M"));
    LONGS_EQUAL(0, gui_mouse_event_size ("\x01[[M@"));
    LONGS_EQUAL(0, gui_mouse_event_size ("\x01[[M@?"));
    LONGS_EQUAL(7, gui_mouse_event_size ("\x01[[M@?E"));
    LONGS_EQUAL(7, gui_mouse_event_size ("\x01[[M@?EABC"));
    LONGS_EQUAL(7, gui_mouse_event_size ("\x01[[M@?E\x01[[M@?E"));
}

/*
 * Tests functions:
 *   gui_mouse_event_concat_gesture
 */

TEST(GuiCursesMouse, EventConcatGesture)
{
    char key[128];

    WEE_CHECK_GESTURE("", 0, 0, 0, 0);
    WEE_CHECK_GESTURE("", 0, 0, 1, 0);
    WEE_CHECK_GESTURE("", 0, 0, 2, 0);

    WEE_CHECK_GESTURE("", 50, 50, 50, 48);
    WEE_CHECK_GESTURE("-gesture-up", 50, 50, 50, 47);
    WEE_CHECK_GESTURE("-gesture-up", 50, 50, 50, 31);
    WEE_CHECK_GESTURE("-gesture-up-long", 50, 50, 65, 31);
    WEE_CHECK_GESTURE("-gesture-up-long", 50, 50, 50, 30);

    WEE_CHECK_GESTURE("", 50, 50, 50, 52);
    WEE_CHECK_GESTURE("-gesture-down", 50, 50, 50, 53);
    WEE_CHECK_GESTURE("-gesture-down", 50, 50, 50, 69);
    WEE_CHECK_GESTURE("-gesture-down-long", 50, 50, 65, 69);
    WEE_CHECK_GESTURE("-gesture-down-long", 50, 50, 50, 70);

    WEE_CHECK_GESTURE("", 50, 50, 48, 50);
    WEE_CHECK_GESTURE("-gesture-left", 50, 50, 47, 50);
    WEE_CHECK_GESTURE("-gesture-left", 50, 50, 11, 50);
    WEE_CHECK_GESTURE("-gesture-left-long", 50, 50, 11, 65);
    WEE_CHECK_GESTURE("-gesture-left-long", 50, 50, 10, 50);

    WEE_CHECK_GESTURE("", 50, 50, 52, 50);
    WEE_CHECK_GESTURE("-gesture-right", 50, 50, 53, 50);
    WEE_CHECK_GESTURE("-gesture-right", 50, 50, 89, 50);
    WEE_CHECK_GESTURE("-gesture-right-long", 50, 50, 89, 65);
    WEE_CHECK_GESTURE("-gesture-right-long", 50, 50, 90, 50);
}

/*
 * Tests functions:
 *   gui_mouse_event_name_sgr
 */

TEST(GuiCursesMouse, EventNameSgr)
{
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR(NULL, 0, 0, 0, 0, 0, NULL);
    WEE_CHECK_EVENT_SGR(NULL, 0, 0, 0, 0, 0, "");
    WEE_CHECK_EVENT_SGR(NULL, 0, 0, 0, 0, 0, "invalid");
    WEE_CHECK_EVENT_SGR(NULL, 0, 0, 0, 0, 0, "invalid;no;digits");

    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("button1-event-down", 1, 19, 5, 19, 5, "0;20;6M");
    WEE_CHECK_EVENT_SGR("button1", 1, 19, 5, 19, 5, "0;20;6m");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("shift-button1-event-down", 1, 19, 5, 19, 5, "4;20;6M");
    WEE_CHECK_EVENT_SGR("shift-button1", 1, 19, 5, 19, 5, "4;20;6m");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("alt-button1-event-down", 1, 19, 5, 19, 5, "8;20;6M");
    WEE_CHECK_EVENT_SGR("alt-button1", 1, 19, 5, 19, 5, "8;20;6m");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("ctrl-button1-event-down", 1, 19, 5, 19, 5, "16;20;6M");
    WEE_CHECK_EVENT_SGR("ctrl-button1", 1, 19, 5, 19, 5, "16;20;6m");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("alt-ctrl-shift-button1-event-down", 1, 19, 5, 19, 5, "28;20;6M");
    WEE_CHECK_EVENT_SGR("alt-ctrl-shift-button1", 1, 19, 5, 19, 5, "28;20;6m");

    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("button2-event-down", 1, 19, 5, 19, 5, "2;20;6M");
    WEE_CHECK_EVENT_SGR("button2", 1, 19, 5, 19, 5, "2;20;6m");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("shift-button2-event-down", 1, 19, 5, 19, 5, "6;20;6M");
    WEE_CHECK_EVENT_SGR("shift-button2", 1, 19, 5, 19, 5, "6;20;6m");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("alt-button2-event-down", 1, 19, 5, 19, 5, "10;20;6M");
    WEE_CHECK_EVENT_SGR("alt-button2", 1, 19, 5, 19, 5, "10;20;6m");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("ctrl-button2-event-down", 1, 19, 5, 19, 5, "18;20;6M");
    WEE_CHECK_EVENT_SGR("ctrl-button2", 1, 19, 5, 19, 5, "18;20;6m");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("alt-ctrl-shift-button2-event-down", 1, 19, 5, 19, 5, "30;20;6M");
    WEE_CHECK_EVENT_SGR("alt-ctrl-shift-button2", 1, 19, 5, 19, 5, "30;20;6m");

    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("button3-event-down", 1, 19, 5, 19, 5, "1;20;6M");
    WEE_CHECK_EVENT_SGR("button3", 1, 19, 5, 19, 5, "1;20;6m");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("shift-button3-event-down", 1, 19, 5, 19, 5, "5;20;6M");
    WEE_CHECK_EVENT_SGR("shift-button3", 1, 19, 5, 19, 5, "5;20;6m");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("alt-button3-event-down", 1, 19, 5, 19, 5, "9;20;6M");
    WEE_CHECK_EVENT_SGR("alt-button3", 1, 19, 5, 19, 5, "9;20;6m");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("ctrl-button3-event-down", 1, 19, 5, 19, 5, "17;20;6M");
    WEE_CHECK_EVENT_SGR("ctrl-button3", 1, 19, 5, 19, 5, "17;20;6m");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("alt-ctrl-shift-button3-event-down", 1, 19, 5, 19, 5, "29;20;6M");
    WEE_CHECK_EVENT_SGR("alt-ctrl-shift-button3", 1, 19, 5, 19, 5, "29;20;6m");

    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("wheelup", 1, 19, 5, 19, 5, "64;20;6M");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("shift-wheelup", 1, 19, 5, 19, 5, "68;20;6M");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("alt-wheelup", 1, 19, 5, 19, 5, "72;20;6M");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("ctrl-wheelup", 1, 19, 5, 19, 5, "80;20;6M");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("alt-ctrl-shift-wheelup", 1, 19, 5, 19, 5, "92;20;6M");

    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("wheeldown", 1, 19, 5, 19, 5, "65;20;6M");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("shift-wheeldown", 1, 19, 5, 19, 5, "69;20;6M");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("alt-wheeldown", 1, 19, 5, 19, 5, "73;20;6M");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("ctrl-wheeldown", 1, 19, 5, 19, 5, "81;20;6M");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("alt-ctrl-shift-wheeldown", 1, 19, 5, 19, 5, "93;20;6M");

    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("button1-event-down", 1, 19, 5, 19, 5, "0;20;6M");
    WEE_CHECK_EVENT_SGR("button1-event-drag", 1, 19, 5, 20, 5, "32;21;6M");
    WEE_CHECK_EVENT_SGR("button1-event-drag", 1, 19, 5, 21, 5, "32;22;6M");
    WEE_CHECK_EVENT_SGR("button1-gesture-right", 1, 19, 5, 22, 5, "0;23;6m");

    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_SGR("button1-event-down", 1, 19, 5, 19, 5, "0;20;6M");
    WEE_CHECK_EVENT_SGR("button1-event-drag", 1, 19, 5, 20, 5, "32;21;6M");
    WEE_CHECK_EVENT_SGR("button1-event-drag", 1, 19, 5, 21, 5, "32;22;6M");
    WEE_CHECK_EVENT_SGR("button1-gesture-right-long", 1, 19, 5, 69, 5, "0;70;6m");
}

/*
 * Tests functions:
 *   gui_mouse_event_name_utf8
 */

TEST(GuiCursesMouse, EventNameUtf8)
{
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8(NULL, 0, 0, 0, 0, 0, NULL);
    WEE_CHECK_EVENT_UTF8(NULL, 0, 0, 0, 0, 0, "");

    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("button1-event-down", 1, 19, 5, 19, 5, " 4&");
    WEE_CHECK_EVENT_UTF8("button1", 1, 19, 5, 19, 5, "#4&");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("alt-button1-event-down", 1, 19, 5, 19, 5, "(4&");
    WEE_CHECK_EVENT_UTF8("alt-button1", 1, 19, 5, 19, 5, "+4&");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("ctrl-button1-event-down", 1, 19, 5, 19, 5, "04&");
    WEE_CHECK_EVENT_UTF8("ctrl-button1", 1, 19, 5, 19, 5, "34&");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("alt-ctrl-button1-event-down", 1, 19, 5, 19, 5, "84&");
    WEE_CHECK_EVENT_UTF8("alt-ctrl-button1", 1, 19, 5, 19, 5, ";4&");

    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("button2-event-down", 1, 19, 5, 19, 5, "\"4&");
    WEE_CHECK_EVENT_UTF8("button2", 1, 19, 5, 19, 5, "#4&");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("alt-button2-event-down", 1, 19, 5, 19, 5, "*4&");
    WEE_CHECK_EVENT_UTF8("alt-button2", 1, 19, 5, 19, 5, "+4&");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("ctrl-button2-event-down", 1, 19, 5, 19, 5, "24&");
    WEE_CHECK_EVENT_UTF8("ctrl-button2", 1, 19, 5, 19, 5, "34&");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("alt-ctrl-button2-event-down", 1, 19, 5, 19, 5, ":4&");
    WEE_CHECK_EVENT_UTF8("alt-ctrl-button2", 1, 19, 5, 19, 5, ";4&");

    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("button3-event-down", 1, 19, 5, 19, 5, "!4&");
    WEE_CHECK_EVENT_UTF8("button3", 1, 19, 5, 19, 5, "#4&");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("alt-button3-event-down", 1, 19, 5, 19, 5, ")4&");
    WEE_CHECK_EVENT_UTF8("alt-button3", 1, 19, 5, 19, 5, "+4&");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("ctrl-button3-event-down", 1, 19, 5, 19, 5, "14&");
    WEE_CHECK_EVENT_UTF8("ctrl-button3", 1, 19, 5, 19, 5, "34&");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("alt-ctrl-button3-event-down", 1, 19, 5, 19, 5, "94&");
    WEE_CHECK_EVENT_UTF8("alt-ctrl-button3", 1, 19, 5, 19, 5, ";4&");

    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("wheelup", 1, 19, 5, 19, 5, "`4&");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("alt-wheelup", 1, 19, 5, 19, 5, "h4&");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("ctrl-wheelup", 1, 19, 5, 19, 5, "p4&");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("alt-ctrl-wheelup", 1, 19, 5, 19, 5, "x4&");

    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("wheeldown", 1, 19, 5, 19, 5, "a4&");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("alt-wheeldown", 1, 19, 5, 19, 5, "i4&");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("ctrl-wheeldown", 1, 19, 5, 19, 5, "q4&");
    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("alt-ctrl-wheeldown", 1, 19, 5, 19, 5, "y4&");

    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("button1-event-down", 1, 19, 5, 19, 5, " 4&");
    WEE_CHECK_EVENT_UTF8("button1-event-drag", 1, 19, 5, 20, 5, "@5&");
    WEE_CHECK_EVENT_UTF8("button1-event-drag", 1, 19, 5, 21, 5, "@6&");
    WEE_CHECK_EVENT_UTF8("button1-gesture-right", 1, 19, 5, 22, 5, "#7&");

    gui_mouse_event_reset ();
    WEE_CHECK_EVENT_UTF8("button1-event-down", 1, 19, 5, 19, 5, " 4&");
    WEE_CHECK_EVENT_UTF8("button1-event-drag", 1, 19, 5, 20, 5, "@5&");
    WEE_CHECK_EVENT_UTF8("button1-event-drag", 1, 19, 5, 21, 5, "@6&");
    WEE_CHECK_EVENT_UTF8("button1-gesture-right-long", 1, 19, 5, 69, 5, "#f&");
}

/*
 * Tests functions:
 *   gui_mouse_event_process
 */

TEST(GuiCursesMouse, EventProcess)
{
    /* TODO: write tests */
}
