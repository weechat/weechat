/*
 * SPDX-FileCopyrightText: 2018-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test hook functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "src/core/core-hook.h"
#include "src/core/core-infolist.h"
#include "src/core/hook/hook-command.h"
#include "src/plugins/weechat-plugin.h"

extern int hook_add_to_infolist_type (struct t_infolist *infolist, int type,
                                      const char *arguments);
}

TEST_GROUP(CoreHook)
{
    /*
     * Command callback used in tests.
     */

    static int
    test_hook_command_cb (const void *pointer, void *data,
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

    /*
     * Count items in an infolist.
     */

    static int
    test_infolist_count_items (struct t_infolist *infolist)
    {
        int count;

        count = 0;
        infolist_reset_item_cursor (infolist);
        while (infolist_next (infolist))
        {
            count++;
        }
        return count;
    }
};

/*
 * Test functions:
 *   hook_init
 */

TEST(CoreHook, Init)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_search_type
 */

TEST(CoreHook, SearchType)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_find_pos
 */

TEST(CoreHook, FindPos)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_add_to_list
 */

TEST(CoreHook, AddToList)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_remove_from_list
 */

TEST(CoreHook, RemoveFromList)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_remove_deleted
 */

TEST(CoreHook, RemoveDeleted)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_init_data
 */

TEST(CoreHook, InitData)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_valid
 */

TEST(CoreHook, Valid)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_exec_start
 */

TEST(CoreHook, ExecStart)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_exec_end
 */

TEST(CoreHook, ExecEnd)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_callback_start
 */

TEST(CoreHook, CallbackStart)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_callback_end
 */

TEST(CoreHook, CallbackEnd)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_get_description
 */

TEST(CoreHook, GetDescription)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_set
 */

TEST(CoreHook, Set)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_timer_clean_process_cb
 */

TEST(CoreHook, TimerCleanProcessCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_schedule_clean_process
 */

TEST(CoreHook, ScheduleCleanProcess)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   unhook
 */

TEST(CoreHook, Unhook)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   unhook_all_plugin
 */

TEST(CoreHook, UnhookAllPlugin)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   unhook_all
 */

TEST(CoreHook, UnhookAll)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_add_to_infolist_pointer
 */

TEST(CoreHook, AddToInfolistPointer)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_add_to_infolist_type
 */

TEST(CoreHook, AddToInfolistType)
{
    struct t_hook *hook1, *hook2;
    struct t_infolist *infolist;

    hook1 = hook_command (NULL, "test_infolist_cmd1", "", "", "", "",
                          &test_hook_command_cb, NULL, NULL);
    CHECK(hook1);
    hook2 = hook_command (NULL, "test_infolist_cmd2", "", "", "", "",
                          &test_hook_command_cb, NULL, NULL);
    CHECK(hook2);

    infolist = infolist_new (NULL);
    LONGS_EQUAL(1, hook_add_to_infolist_type (infolist, HOOK_TYPE_COMMAND,
                                              "test_infolist_cmd*"));
    LONGS_EQUAL(2, test_infolist_count_items (infolist));
    infolist_free (infolist);

    infolist = infolist_new (NULL);
    LONGS_EQUAL(1, hook_add_to_infolist_type (infolist, HOOK_TYPE_COMMAND,
                                              "test_infolist_cmd2"));
    LONGS_EQUAL(1, test_infolist_count_items (infolist));
    infolist_reset_item_cursor (infolist);
    CHECK(infolist_next (infolist));
    POINTERS_EQUAL(hook2, infolist_pointer (infolist, "pointer"));
    infolist_free (infolist);

    /* Hook deleted during a hook exec: it is not returned. */
    hook_exec_start ();
    unhook (hook1);
    LONGS_EQUAL(1, hook1->deleted);
    infolist = infolist_new (NULL);
    LONGS_EQUAL(1, hook_add_to_infolist_type (infolist, HOOK_TYPE_COMMAND,
                                              "test_infolist_cmd*"));
    LONGS_EQUAL(1, test_infolist_count_items (infolist));
    infolist_reset_item_cursor (infolist);
    CHECK(infolist_next (infolist));
    POINTERS_EQUAL(hook2, infolist_pointer (infolist, "pointer"));
    infolist_free (infolist);
    infolist = infolist_new (NULL);
    LONGS_EQUAL(1, hook_add_to_infolist_type (infolist, HOOK_TYPE_COMMAND,
                                              NULL));
    infolist_reset_item_cursor (infolist);
    while (infolist_next (infolist))
    {
        CHECK(infolist_pointer (infolist, "pointer") != hook1);
    }
    infolist_free (infolist);
    hook_exec_end ();

    unhook (hook2);
}

/*
 * Test functions:
 *   hook_add_to_infolist
 */

TEST(CoreHook, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_print_log
 */

TEST(CoreHook, PrintLog)
{
    /* TODO: write tests */
}
