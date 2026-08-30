/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test focus functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <string.h>
#include "src/core/core-hashtable.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-focus.h"
#include "src/gui/gui-line.h"
}

TEST_GROUP(GuiFocus)
{
};

/*
 * Test functions:
 *   gui_focus_to_hashtable
 */

TEST(GuiFocus, ToHashtable)
{
    struct t_gui_buffer *buffer;
    struct t_gui_focus_info focus_info;
    struct t_hashtable *hashtable;

    memset (&focus_info, 0, sizeof (focus_info));

    /* No line: all line keys are set to "-1" or an empty string. */
    hashtable = gui_focus_to_hashtable (&focus_info, "button1");
    CHECK(hashtable);
    STRCMP_EQUAL("button1", (const char *)hashtable_get (hashtable, "_key"));
    STRCMP_EQUAL("", (const char *)hashtable_get (hashtable, "_chat_line"));
    STRCMP_EQUAL("-1", (const char *)hashtable_get (hashtable, "_chat_line_id"));
    STRCMP_EQUAL("-1", (const char *)hashtable_get (hashtable, "_chat_line_date"));
    STRCMP_EQUAL("-1", (const char *)hashtable_get (hashtable, "_chat_line_date_usec"));
    STRCMP_EQUAL("-1", (const char *)hashtable_get (hashtable, "_chat_line_date_printed"));
    STRCMP_EQUAL("-1", (const char *)hashtable_get (hashtable, "_chat_line_date_usec_printed"));
    hashtable_free (hashtable);

    /*
     * Buffer with formatted content: the date of print is derived from the
     * identifier of the line.
     */
    buffer = gui_buffer_new_user ("test", GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);
    focus_info.buffer = buffer;
    focus_info.chat = 1;
    focus_info.chat_line = gui_line_new_with_id (buffer, 1786886000123456LL,
                                                 -1, 0, 0, NULL, NULL,
                                                 "message", -1, NULL);
    CHECK(focus_info.chat_line);
    gui_line_add (focus_info.chat_line, 0);

    hashtable = gui_focus_to_hashtable (&focus_info, "button1");
    CHECK(hashtable);
    STRCMP_EQUAL("1786886000123456",
                 (const char *)hashtable_get (hashtable, "_chat_line_id"));
    STRCMP_EQUAL("1786886000",
                 (const char *)hashtable_get (hashtable, "_chat_line_date_printed"));
    STRCMP_EQUAL("123456",
                 (const char *)hashtable_get (hashtable, "_chat_line_date_usec_printed"));
    hashtable_free (hashtable);

    gui_buffer_close (buffer);

    /* Buffer with free content: there is no date of print. */
    buffer = gui_buffer_new_user ("test", GUI_BUFFER_TYPE_FREE);
    CHECK(buffer);
    focus_info.buffer = buffer;
    focus_info.chat = 1;
    focus_info.chat_line = gui_line_new (buffer, 5, 0, 0, NULL, NULL,
                                         "message", -1, NULL);
    CHECK(focus_info.chat_line);
    gui_line_add_y (focus_info.chat_line);

    hashtable = gui_focus_to_hashtable (&focus_info, "button1");
    CHECK(hashtable);
    STRCMP_EQUAL("5", (const char *)hashtable_get (hashtable, "_chat_line_id"));
    STRCMP_EQUAL("0",
                 (const char *)hashtable_get (hashtable, "_chat_line_date_printed"));
    STRCMP_EQUAL("0",
                 (const char *)hashtable_get (hashtable, "_chat_line_date_usec_printed"));
    hashtable_free (hashtable);

    gui_buffer_close (buffer);
}
