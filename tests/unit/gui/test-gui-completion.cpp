/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test completion functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <string.h>
#include "src/core/core-hashtable.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-completion.h"
}

TEST_GROUP(GuiCompletion)
{
};

/*
 * Test functions:
 *   gui_completion_valid
 */

TEST(GuiCompletion, Valid)
{
    struct t_gui_completion *completion;
    int dummy;

    LONGS_EQUAL(0, gui_completion_valid (NULL));
    LONGS_EQUAL(0, gui_completion_valid ((struct t_gui_completion *)0x1));

    /* Stack address: it can never be a completion (allocated on heap). */
    LONGS_EQUAL(0, gui_completion_valid ((struct t_gui_completion *)&dummy));

    /* Completion of core buffer. */
    LONGS_EQUAL(1, gui_completion_valid (gui_buffers->completion));

    completion = gui_completion_new (NULL, gui_buffers);
    CHECK(completion);
    LONGS_EQUAL(1, gui_completion_valid (completion));
    gui_completion_free (completion);
    LONGS_EQUAL(0, gui_completion_valid (completion));
}

/*
 * Test functions:
 *   gui_completion_new
 *   gui_completion_free
 */

TEST(GuiCompletion, NewFree)
{
    struct t_gui_completion *completion;

    POINTERS_EQUAL(NULL, gui_completion_new (NULL, NULL));

    completion = gui_completion_new (NULL, gui_buffers);
    CHECK(completion);
    POINTERS_EQUAL(NULL, completion->plugin);
    POINTERS_EQUAL(gui_buffers, completion->buffer);
    LONGS_EQUAL(1, hashtable_has_key (gui_completion_pointers, completion));
    LONGS_EQUAL(1, hashtable_has_key (gui_completion_pointers,
                                      gui_buffers->completion));

    gui_completion_free (completion);
    LONGS_EQUAL(0, hashtable_has_key (gui_completion_pointers, completion));
    LONGS_EQUAL(1, hashtable_has_key (gui_completion_pointers,
                                      gui_buffers->completion));

    /* Test free of NULL completion. */
    gui_completion_free (NULL);
}

/*
 * Test functions with an invalid completion:
 *   gui_completion_free
 *   gui_completion_list_add
 *   gui_completion_search
 *   gui_completion_get_string
 *   gui_completion_set
 */

TEST(GuiCompletion, InvalidCompletion)
{
    struct t_gui_completion *completion, completion_not_in_list;

    /* Completion not in list: it must not be used. */
    memset (&completion_not_in_list, 0, sizeof (completion_not_in_list));
    completion_not_in_list.base_word = (char *)"test";
    gui_completion_list_add (&completion_not_in_list, "test", 0, "end");
    LONGS_EQUAL(0,
                gui_completion_search (&completion_not_in_list, "/help", 5, 1));
    STRCMP_EQUAL(NULL,
                 gui_completion_get_string (&completion_not_in_list,
                                            "base_word"));
    gui_completion_set (&completion_not_in_list, "add_space", "1");
    LONGS_EQUAL(0, completion_not_in_list.add_space);
    gui_completion_free (&completion_not_in_list);

    completion = gui_completion_new (NULL, gui_buffers);
    CHECK(completion);
    gui_completion_set (completion, "add_space", "0");
    LONGS_EQUAL(0, completion->add_space);
    gui_completion_set (completion, "add_space", "1");
    LONGS_EQUAL(1, completion->add_space);
    gui_completion_free (completion);

    /* Completion freed: it must not be used. */
    gui_completion_list_add (completion, "test", 0, "end");
    LONGS_EQUAL(0, gui_completion_search (completion, "/help", 5, 1));
    STRCMP_EQUAL(NULL, gui_completion_get_string (completion, "base_word"));
    gui_completion_set (completion, "add_space", "0");
    gui_completion_free (completion);
}
