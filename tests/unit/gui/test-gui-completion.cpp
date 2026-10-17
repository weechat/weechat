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
#include "src/core/weechat.h"
#include "src/core/core-config.h"
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-completion.h"
#include "src/plugins/plugin.h"
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

int
test_completion_command_cb (const void *pointer, void *data,
                            struct t_gui_buffer *buffer,
                            int argc, char **argv, char **argv_eol)
{
    /* Make C++ compiler happy. */
    (void) pointer;
    (void) data;
    (void) buffer;
    (void) argc;
    (void) argv;
    (void) argv_eol;

    return WEECHAT_RC_OK;
}

int
test_completion_empty_cb (const void *pointer, void *data,
                          const char *completion_item,
                          struct t_gui_buffer *buffer,
                          struct t_gui_completion *completion)
{
    /* Make C++ compiler happy. */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;
    (void) completion;

    return WEECHAT_RC_OK;
}

int
test_completion_words_cb (const void *pointer, void *data,
                          const char *completion_item,
                          struct t_gui_buffer *buffer,
                          struct t_gui_completion *completion)
{
    /* Make C++ compiler happy. */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    gui_completion_list_add (completion, "abc_word", 0, WEECHAT_LIST_POS_SORT);

    return WEECHAT_RC_OK;
}

/*
 * Test functions:
 *   gui_completion_search (command argument with custom completion)
 */

TEST(GuiCompletion, SearchCustomCompletion)
{
    struct t_gui_completion *completion;
    struct t_hook *hook_cmd, *hook_cplt_empty, *hook_cplt_words;

    config_file_option_set (config_completion_default_template,
                            "abc_default", 1);

    hook_cmd = hook_command (NULL, "test_cplt", "", "", "",
                             "%(test_cplt_empty) %(test_cplt_words)",
                             &test_completion_command_cb, NULL, NULL);
    CHECK(hook_cmd);
    hook_cplt_empty = hook_completion (NULL, "test_cplt_empty", "",
                                       &test_completion_empty_cb, NULL, NULL);
    CHECK(hook_cplt_empty);
    hook_cplt_words = hook_completion (NULL, "test_cplt_words", "",
                                       &test_completion_words_cb, NULL, NULL);
    CHECK(hook_cplt_words);

    completion = gui_completion_new (NULL, gui_buffers);
    CHECK(completion);

    /* Custom completion without words: nothing is completed. */
    gui_completion_search (completion, "/test_cplt abc", 14, 1);
    LONGS_EQUAL(1, completion->template_custom_completion);
    STRCMP_EQUAL(NULL, completion->word_found);

    /* Same completion again: still nothing completed. */
    gui_completion_search (completion, "/test_cplt abc", 14, 1);
    STRCMP_EQUAL(NULL, completion->word_found);

    /* Custom completion with words. */
    gui_completion_search (completion, "/test_cplt x abc", 16, 1);
    LONGS_EQUAL(1, completion->template_custom_completion);
    STRCMP_EQUAL("abc_word", completion->word_found);

    /* No template for argument: fallback on default template. */
    gui_completion_search (completion, "/test_cplt x y abc", 18, 1);
    LONGS_EQUAL(0, completion->template_custom_completion);
    STRCMP_EQUAL("abc_default", completion->word_found);

    gui_completion_free (completion);

    unhook (hook_cplt_words);
    unhook (hook_cplt_empty);
    unhook (hook_cmd);

    config_file_option_reset (config_completion_default_template, 1);
}
