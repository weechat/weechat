/*
 * test-gui-hotlist.cpp - test hotlist functions
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
#include <time.h>
#include <sys/time.h>
#include "src/core/core-config.h"
#include "src/core/core-config-file.h"
#include "src/core/core-hook.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-hotlist.h"

extern struct t_gui_hotlist *gui_hotlist_dup (struct t_gui_hotlist *hotlist);
extern int gui_hotlist_compare_hotlists (struct t_hdata *hdata_hotlist,
                                         struct t_gui_hotlist *hotlist1,
                                         struct t_gui_hotlist *hotlist2);
extern void gui_hotlist_free_all (struct t_gui_hotlist **hotlist,
                                  struct t_gui_hotlist **last_hotlist);
}

/*
 * test buffers:
 *   buffer_test[0] ("test1"):
 *     low: 1, message: 0, private: 0, highlight: 0
 *     local variable "priority": 6
 *   buffer_test[1] ("test2"):
 *     low: 1, message: 2, private: 0, highlight: 3
 *     local variable "priority": 4
 *   buffer_test[2] ("Test3"):
 *     low: 0, message: 0, private: 1, highlight: 0
 *     local variable "priority": 8
 *
 * with default hotlist sort:
 *   [test2, test3, test1]
 */
struct t_gui_buffer *buffer_test[3];
const char *buffer_names[3] = { "test1", "test2", "Test3" };

TEST_GROUP(GuiHotlist)
{
    void setup ()
    {
        int i;

        for (i = 0; i < 3; i++)
        {
            buffer_test[i] = gui_buffer_new (NULL, buffer_names[i],
                                             NULL, NULL, NULL,
                                             NULL, NULL, NULL);
        }
        /* buffer "test1": 1 low */
        gui_hotlist_add (buffer_test[0], GUI_HOTLIST_LOW, NULL, 0);
        gui_buffer_set (buffer_test[0], "localvar_set_priority", "6");
        /* buffer "test2": 1 low, 2 messages, 3 highlights */
        gui_hotlist_add (buffer_test[1], GUI_HOTLIST_PRIVATE, NULL, 0);
        gui_hotlist_add (buffer_test[1], GUI_HOTLIST_MESSAGE, NULL, 0);
        gui_hotlist_add (buffer_test[1], GUI_HOTLIST_MESSAGE, NULL, 0);
        gui_hotlist_add (buffer_test[1], GUI_HOTLIST_HIGHLIGHT, NULL, 0);
        gui_hotlist_add (buffer_test[1], GUI_HOTLIST_HIGHLIGHT, NULL, 0);
        gui_hotlist_add (buffer_test[1], GUI_HOTLIST_HIGHLIGHT, NULL, 0);
        gui_buffer_set (buffer_test[1], "localvar_set_priority", "4");
        /* buffer "Test3": 2 private */
        gui_hotlist_add (buffer_test[2], GUI_HOTLIST_PRIVATE, NULL, 0);
        gui_hotlist_add (buffer_test[2], GUI_HOTLIST_PRIVATE, NULL, 0);
        gui_buffer_set (buffer_test[2], "localvar_set_priority", "8");
    }

    void teardown ()
    {
        int i;

        for (i = 0; i < 3; i++)
        {
            gui_buffer_close (buffer_test[i]);
            buffer_test[i] = NULL;
        }
    }
};

/*
 * Tests functions:
 *   gui_hotlist_changed_signal
 */

TEST(GuiHotlist, ChangedSignal)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_hotlist_search_priority
 */

TEST(GuiHotlist, SearchPriority)
{
    LONGS_EQUAL(-1, gui_hotlist_search_priority (NULL));
    LONGS_EQUAL(-1, gui_hotlist_search_priority (""));
    LONGS_EQUAL(-1, gui_hotlist_search_priority ("invalid"));

    LONGS_EQUAL(0, gui_hotlist_search_priority ("low"));
    LONGS_EQUAL(1, gui_hotlist_search_priority ("message"));
    LONGS_EQUAL(2, gui_hotlist_search_priority ("private"));
    LONGS_EQUAL(3, gui_hotlist_search_priority ("highlight"));
}

/*
 * Tests functions:
 *   gui_hotlist_search
 */

TEST(GuiHotlist, Search)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_hotlist_dup
 */

TEST(GuiHotlist, Dup)
{
    struct t_gui_hotlist *hotlist, *hotlist_dup;

    hotlist = (struct t_gui_hotlist *)malloc (sizeof (*hotlist));
    CHECK(hotlist);
    hotlist->priority = GUI_HOTLIST_HIGHLIGHT;
    hotlist->creation_time.tv_sec = 1710623372;
    hotlist->creation_time.tv_usec = 123456;
    hotlist->buffer = gui_buffers;
    hotlist->count[0] = 12;
    hotlist->count[1] = 34;
    hotlist->count[2] = 56;
    hotlist->count[3] = 78;
    hotlist->prev_hotlist = NULL;
    hotlist->next_hotlist = NULL;

    hotlist_dup = gui_hotlist_dup (hotlist);
    CHECK(hotlist_dup);
    LONGS_EQUAL(GUI_HOTLIST_HIGHLIGHT, hotlist_dup->priority);
    LONGS_EQUAL(1710623372, hotlist_dup->creation_time.tv_sec);
    LONGS_EQUAL(123456, hotlist_dup->creation_time.tv_usec);
    LONGS_EQUAL(12, hotlist_dup->count[0]);
    LONGS_EQUAL(34, hotlist_dup->count[1]);
    LONGS_EQUAL(56, hotlist_dup->count[2]);
    LONGS_EQUAL(78, hotlist_dup->count[3]);
    POINTERS_EQUAL(NULL, hotlist_dup->prev_hotlist);
    POINTERS_EQUAL(NULL, hotlist_dup->next_hotlist);

    free (hotlist);
    free (hotlist_dup);
}

/*
 * Tests functions:
 *   gui_hotlist_free
 */

TEST(GuiHotlist, Free)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_hotlist_free_all
 */

TEST(GuiHotlist, FreeAll)
{
    CHECK(gui_hotlist);
    CHECK(last_gui_hotlist);

    gui_hotlist_free_all (&gui_hotlist, &last_gui_hotlist);

    POINTERS_EQUAL(NULL, gui_hotlist);
    POINTERS_EQUAL(NULL, last_gui_hotlist);
}

/*
 * Tests functions:
 *   gui_hotlist_check_buffer_notify
 */

TEST(GuiHotlist, CheckBufferNotify)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_hotlist_compare_hotlists
 */

TEST(GuiHotlist, CompareHotlists)
{
    struct t_hdata *hdata_hotlist;

    hdata_hotlist = hook_hdata_get (NULL, "hotlist");

    config_file_option_set (config_look_hotlist_sort, "buffer.number", 1);

    LONGS_EQUAL(0, gui_hotlist_compare_hotlists (hdata_hotlist, NULL, NULL));
    LONGS_EQUAL(1, gui_hotlist_compare_hotlists (hdata_hotlist, gui_hotlist, NULL));
    LONGS_EQUAL(-1, gui_hotlist_compare_hotlists (hdata_hotlist, NULL, gui_hotlist));

    config_file_option_set (config_look_hotlist_sort, "-buffer.number", 1);

    LONGS_EQUAL(0, gui_hotlist_compare_hotlists (hdata_hotlist, NULL, NULL));
    LONGS_EQUAL(-1, gui_hotlist_compare_hotlists (hdata_hotlist, gui_hotlist, NULL));
    LONGS_EQUAL(1, gui_hotlist_compare_hotlists (hdata_hotlist, NULL, gui_hotlist));

    config_file_option_reset (config_look_hotlist_sort, 1);
}

/*
 * Tests functions:
 *   gui_hotlist_find_pos
 */

TEST(GuiHotlist, FindPos)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_hotlist_add_hotlist
 */

TEST(GuiHotlist, AddHotlist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_hotlist_add
 */

TEST(GuiHotlist, Add)
{
    struct t_gui_hotlist *hotlist;
    struct timeval tv;

    tv.tv_sec = 1710683593;
    tv.tv_usec = 123456;

    hotlist = gui_hotlist_add (gui_buffers, GUI_HOTLIST_LOW, &tv, 0);
    CHECK(hotlist);

    LONGS_EQUAL(GUI_HOTLIST_LOW, hotlist->priority);
    LONGS_EQUAL(1710683593, hotlist->creation_time.tv_sec);
    LONGS_EQUAL(123456, hotlist->creation_time.tv_usec);
    POINTERS_EQUAL(gui_buffers, hotlist->buffer);
    LONGS_EQUAL(1, hotlist->count[0]);
    LONGS_EQUAL(0, hotlist->count[1]);
    LONGS_EQUAL(0, hotlist->count[2]);
    LONGS_EQUAL(0, hotlist->count[3]);

    gui_hotlist_remove_buffer (gui_buffers, 1);
}

/*
 * Tests functions:
 *   gui_hotlist_remove_buffer
 *   gui_hotlist_restore_buffer
 */

TEST(GuiHotlist, RestoreBuffer)
{
    gui_hotlist_remove_buffer (buffer_test[1], 0);

    POINTERS_EQUAL(buffer_test[2], gui_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[0], gui_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(NULL, gui_hotlist->next_hotlist->next_hotlist);

    gui_hotlist_remove_buffer (buffer_test[2], 0);

    POINTERS_EQUAL(buffer_test[0], gui_hotlist->buffer);
    POINTERS_EQUAL(NULL, gui_hotlist->next_hotlist);

    gui_hotlist_restore_buffer (buffer_test[1]);

    POINTERS_EQUAL(buffer_test[1], gui_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[0], gui_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(NULL, gui_hotlist->next_hotlist->next_hotlist);

    gui_hotlist_restore_buffer (buffer_test[2]);

    POINTERS_EQUAL(buffer_test[1], gui_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[2], gui_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[0], gui_hotlist->next_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(NULL, gui_hotlist->next_hotlist->next_hotlist->next_hotlist);

    gui_hotlist_remove_buffer (buffer_test[0], 0);
    gui_hotlist_remove_buffer (buffer_test[1], 0);
    gui_hotlist_remove_buffer (buffer_test[2], 0);

    POINTERS_EQUAL(NULL, gui_hotlist);
}

/*
 * Tests functions:
 *   gui_hotlist_restore_all_buffers
 */

TEST(GuiHotlist, RestoreAllBuffers)
{
    if (gui_buffers->hotlist_removed)
    {
        free (gui_buffers->hotlist_removed);
        gui_buffers->hotlist_removed = NULL;
    }

    gui_hotlist_remove_buffer (buffer_test[0], 1);
    gui_hotlist_remove_buffer (buffer_test[1], 1);
    gui_hotlist_remove_buffer (buffer_test[2], 1);

    POINTERS_EQUAL(NULL, gui_hotlist);

    gui_hotlist_restore_all_buffers ();

    POINTERS_EQUAL(buffer_test[1], gui_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[2], gui_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[0], gui_hotlist->next_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(NULL, gui_hotlist->next_hotlist->next_hotlist->next_hotlist);
}

/*
 * Tests functions:
 *   gui_hotlist_resort
 */

TEST(GuiHotlist, Resort)
{
    /* with default sort */
    POINTERS_EQUAL(buffer_test[1], gui_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[2], gui_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[0], gui_hotlist->next_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(NULL, gui_hotlist->next_hotlist->next_hotlist->next_hotlist);

    /* sort by buffer number */
    config_file_option_set (config_look_hotlist_sort, "buffer.number", 1);
    POINTERS_EQUAL(buffer_test[0], gui_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[1], gui_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[2], gui_hotlist->next_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(NULL, gui_hotlist->next_hotlist->next_hotlist->next_hotlist);

    /* sort by buffer number (descending) */
    config_file_option_set (config_look_hotlist_sort, "-buffer.number", 1);
    POINTERS_EQUAL(buffer_test[2], gui_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[1], gui_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[0], gui_hotlist->next_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(NULL, gui_hotlist->next_hotlist->next_hotlist->next_hotlist);

    /* sort by buffer name (case sensitive) */
    config_file_option_set (config_look_hotlist_sort, "buffer.name", 1);
    POINTERS_EQUAL(buffer_test[2], gui_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[0], gui_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[1], gui_hotlist->next_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(NULL, gui_hotlist->next_hotlist->next_hotlist->next_hotlist);

    /* sort by buffer name (case insensitive) */
    config_file_option_set (config_look_hotlist_sort, "~buffer.name", 1);
    POINTERS_EQUAL(buffer_test[0], gui_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[1], gui_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[2], gui_hotlist->next_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(NULL, gui_hotlist->next_hotlist->next_hotlist->next_hotlist);

    /* sort by local variable "priority" (descending) */
    config_file_option_set (config_look_hotlist_sort, "-buffer.local_variables.priority", 1);
    POINTERS_EQUAL(buffer_test[2], gui_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[0], gui_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[1], gui_hotlist->next_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(NULL, gui_hotlist->next_hotlist->next_hotlist->next_hotlist);

    config_file_option_reset (config_look_hotlist_sort, 1);
}

/*
 * Tests functions:
 *   gui_hotlist_clear
 */

TEST(GuiHotlist, Clear)
{
    CHECK(gui_hotlist);

    /* clear only low join/part (1) */
    gui_hotlist_clear (1);

    POINTERS_EQUAL(buffer_test[1], gui_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[2], gui_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(NULL, gui_hotlist->next_hotlist->next_hotlist);

    /* clear low join/part (1) + private (4) */
    gui_hotlist_clear (5);

    POINTERS_EQUAL(buffer_test[1], gui_hotlist->buffer);
    POINTERS_EQUAL(NULL, gui_hotlist->next_hotlist);

    /* clear whole hotlist (1=join/part + 2=msg + 4=private + 5=highlight) */
    gui_hotlist_clear (15);

    POINTERS_EQUAL(NULL, gui_hotlist);
}

/*
 * Tests functions:
 *   gui_hotlist_clear_level_string
 */

TEST(GuiHotlist, ClearLevelString)
{
    POINTERS_EQUAL(NULL, gui_hotlist_initial_buffer);

    gui_hotlist_clear_level_string (buffer_test[0], "lowest");

    POINTERS_EQUAL(buffer_test[1], gui_hotlist->buffer);
    POINTERS_EQUAL(buffer_test[2], gui_hotlist->next_hotlist->buffer);
    POINTERS_EQUAL(NULL, gui_hotlist->next_hotlist->next_hotlist);
    POINTERS_EQUAL(buffer_test[0], gui_hotlist_initial_buffer);

    gui_hotlist_clear_level_string (buffer_test[1], "highest");

    POINTERS_EQUAL(buffer_test[2], gui_hotlist->buffer);
    POINTERS_EQUAL(NULL, gui_hotlist->next_hotlist);

    POINTERS_EQUAL(buffer_test[1], gui_hotlist_initial_buffer);
    gui_hotlist_clear_level_string (buffer_test[2], "4");

    POINTERS_EQUAL(NULL, gui_hotlist);
    POINTERS_EQUAL(buffer_test[2], gui_hotlist_initial_buffer);

    gui_hotlist_add (buffer_test[0], GUI_HOTLIST_PRIVATE, NULL, 0);
    gui_hotlist_add (buffer_test[1], GUI_HOTLIST_MESSAGE, NULL, 0);
    gui_hotlist_add (buffer_test[2], GUI_HOTLIST_HIGHLIGHT, NULL, 0);

    gui_hotlist_clear_level_string (buffer_test[1], NULL);

    POINTERS_EQUAL(NULL, gui_hotlist);
    POINTERS_EQUAL(buffer_test[1], gui_hotlist_initial_buffer);
}

/*
 * Tests functions:
 *   gui_hotlist_remove_buffer
 */

TEST(GuiHotlist, RemoveBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_hotlist_hdata_hotlist_cb
 */

TEST(GuiHotlist, HdataHotlistCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_hotlist_add_to_infolist
 */

TEST(GuiHotlist, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_hotlist_print_log
 */

TEST(GuiHotlist, PrintLog)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_hotlist_end
 */

TEST(GuiHotlist, End)
{
    /* TODO: write tests */
}
