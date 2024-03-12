/*
 * test-typing-status.cpp - test typing status functions
 *
 * Copyright (C) 2021-2024 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CppUTest/TestHarness.h"

#include "tests/tests.h"

extern "C"
{
#include "src/core/core-hashtable.h"
#include "src/gui/gui-buffer.h"
#include "src/plugins/typing/typing-status.h"
}

TEST_GROUP(TypingStatus)
{
};

/*
 * Tests functions:
 *   typing_status_search_state
 */

TEST(TypingStatus, SearchState)
{
    int i;

    LONGS_EQUAL(-1, typing_status_search_state (NULL));
    LONGS_EQUAL(-1, typing_status_search_state (""));
    LONGS_EQUAL(-1, typing_status_search_state ("abc"));

    for (i = 0; i < TYPING_STATUS_NUM_STATES; i++)
    {
        LONGS_EQUAL(i, typing_status_search_state (typing_status_state_string[i]));
    }
}

/*
 * Tests functions:
 *   typing_status_self_free_value_cb
 *   typing_status_self_add
 *   typing_status_self_search
 */

TEST(TypingStatus, SelfAddSearch)
{
    struct t_typing_status *ptr_typing_status;

    POINTERS_EQUAL(NULL, typing_status_self_add (NULL, 0, 0));
    POINTERS_EQUAL(NULL, typing_status_self_add (NULL, -1, 0));
    POINTERS_EQUAL(NULL, typing_status_self_add (gui_buffers, -1, 0));
    POINTERS_EQUAL(NULL, typing_status_self_add (gui_buffers, 999999, 0));

    ptr_typing_status = typing_status_self_add (gui_buffers,
                                                TYPING_STATUS_STATE_TYPING,
                                                1625390031);
    CHECK(ptr_typing_status);
    LONGS_EQUAL(TYPING_STATUS_STATE_TYPING, ptr_typing_status->state);
    LONGS_EQUAL(1625390031, ptr_typing_status->last_typed);
    LONGS_EQUAL(1, typing_status_self->items_count);
    POINTERS_EQUAL(gui_buffers, typing_status_self->oldest_item->key);
    POINTERS_EQUAL(ptr_typing_status, typing_status_self->oldest_item->value);
    POINTERS_EQUAL(NULL, typing_status_self->oldest_item->next_created_item);

    POINTERS_EQUAL(NULL, typing_status_self_search (NULL));
    POINTERS_EQUAL(NULL, typing_status_self_search (gui_buffers + 1));
    POINTERS_EQUAL(ptr_typing_status, typing_status_self_search (gui_buffers));

    hashtable_remove_all (typing_status_self);
}

/*
 * Tests functions:
 *   typing_status_nicks_free_value_cb
 *   typing_status_nick_free_value_cb
 *   typing_status_nick_add
 *   typing_status_nick_search
 *   typing_status_nick_remove
 */

TEST(TypingStatus, NickAddSearchRemove)
{
    struct t_typing_status *ptr_typing_status;
    struct t_hashtable *ptr_nicks;

    POINTERS_EQUAL(NULL, typing_status_nick_add (NULL, NULL, 0, 0));
    POINTERS_EQUAL(NULL, typing_status_nick_add (NULL, "test", 0, 0));
    POINTERS_EQUAL(NULL, typing_status_nick_add (NULL, "test", -1, 0));
    POINTERS_EQUAL(NULL, typing_status_nick_add (NULL, "test", 999999, 0));
    POINTERS_EQUAL(NULL, typing_status_nick_add (gui_buffers, NULL, -1, 0));
    POINTERS_EQUAL(NULL, typing_status_nick_add (gui_buffers, "test", -1, 0));
    POINTERS_EQUAL(NULL, typing_status_nick_add (gui_buffers, "test", 999999, 0));

    ptr_typing_status = typing_status_nick_add (gui_buffers,
                                                "alice",
                                                TYPING_STATUS_STATE_TYPING,
                                                1625390031);
    CHECK(ptr_typing_status);
    LONGS_EQUAL(TYPING_STATUS_STATE_TYPING, ptr_typing_status->state);
    LONGS_EQUAL(1625390031, ptr_typing_status->last_typed);
    LONGS_EQUAL(1, typing_status_nicks->items_count);
    POINTERS_EQUAL(gui_buffers, typing_status_nicks->oldest_item->key);
    POINTERS_EQUAL(NULL, typing_status_nicks->oldest_item->next_created_item);
    ptr_nicks = (struct t_hashtable *)hashtable_get (typing_status_nicks,
                                                     gui_buffers);
    LONGS_EQUAL(1, ptr_nicks->items_count);
    STRCMP_EQUAL("alice", (const char *)ptr_nicks->oldest_item->key);
    POINTERS_EQUAL(ptr_typing_status, ptr_nicks->oldest_item->value);
    POINTERS_EQUAL(NULL, ptr_nicks->oldest_item->next_created_item);

    POINTERS_EQUAL(NULL, typing_status_nick_search (NULL, NULL));
    POINTERS_EQUAL(NULL, typing_status_nick_search (gui_buffers, NULL));
    POINTERS_EQUAL(NULL, typing_status_nick_search (gui_buffers, "abc"));
    POINTERS_EQUAL(ptr_typing_status, typing_status_nick_search (gui_buffers,
                                                                 "alice"));

    hashtable_remove_all (typing_status_nicks);
}
