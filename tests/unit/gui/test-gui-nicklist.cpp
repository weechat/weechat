/*
 * test-gui-nicklist.cpp - test nicklist functions
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
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-nicklist.h"
}

#define TEST_BUFFER_NAME "test"

TEST_GROUP(GuiNicklist)
{
};

/*
 * Tests functions:
 *   gui_nicklist_send_signal
 */

TEST(GuiNicklist, SendSignal)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_send_hsignal
 */

TEST(GuiNicklist, SendHsignal)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_find_pos_group
 */

TEST(GuiNicklist, FindPosGroup)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_insert_group_sorted
 */

TEST(GuiNicklist, InsertGroupSorted)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_search_group_internal
 */

TEST(GuiNicklist, SearchGroupInternal)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_search_group
 */

TEST(GuiNicklist, SearchGroup)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_generate_id
 */

TEST(GuiNicklist, GenerateId)
{
    struct t_gui_buffer *buffer;
    long long id;

    buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                             NULL, NULL, NULL,
                             NULL, NULL, NULL);
    CHECK(buffer);

    CHECK(buffer->nicklist_last_id_assigned == 0);

    id = gui_nicklist_generate_id (buffer);
    CHECK(id > buffer->nicklist_last_id_assigned);
    id = gui_buffer_generate_id ();
    CHECK(id > buffer->nicklist_last_id_assigned);
    id = gui_buffer_generate_id ();
    CHECK(id > buffer->nicklist_last_id_assigned);

    gui_buffer_close (buffer);
}

/*
 * Tests functions:
 *   gui_nicklist_add_group_with_id
 */

TEST(GuiNicklist, AddGroupWithId)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_add_group
 */

TEST(GuiNicklist, AddGroup)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_find_pos_nick
 */

TEST(GuiNicklist, FindPosNick)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_insert_nick_sorted
 */

TEST(GuiNicklist, InsertNickSorted)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_search_nick
 */

TEST(GuiNicklist, SearchNick)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_add_nick_with_id
 */

TEST(GuiNicklist, AddNickWithId)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_add_nick
 */

TEST(GuiNicklist, AddNick)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_remove_nick
 */

TEST(GuiNicklist, RemoveNick)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_remove_group
 */

TEST(GuiNicklist, RemoveGroup)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_remove_all
 */

TEST(GuiNicklist, RemoveAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_get_next_item
 */

TEST(GuiNicklist, GetNextItem)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_get_group_start
 */

TEST(GuiNicklist, GetGroupStart)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_get_max_length
 */

TEST(GuiNicklist, GetMaxLength)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_compute_visible_count
 */

TEST(GuiNicklist, ComputeVisibleCount)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_group_get_integer
 */

TEST(GuiNicklist, GroupGetInteger)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_group_get_string
 */

TEST(GuiNicklist, GroupGetString)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_group_get_pointer
 */

TEST(GuiNicklist, GroupGetPointer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_group_set
 */

TEST(GuiNicklist, GroupSet)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_nick_get_integer
 */

TEST(GuiNicklist, NickGetInteger)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_nick_get_string
 */

TEST(GuiNicklist, NickGetString)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_nick_get_pointer
 */

TEST(GuiNicklist, NickGetPointer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_nick_set
 */

TEST(GuiNicklist, NickSet)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_hdata_nick_group_cb
 */

TEST(GuiNicklist, HdataNickGroupCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_hdata_nick_cb
 */

TEST(GuiNicklist, HdataNickCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_add_group_to_infolist
 */

TEST(GuiNicklist, AddGroupToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_add_nick_to_infolist
 */

TEST(GuiNicklist, AddNickToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_add_to_infolist
 */

TEST(GuiNicklist, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_print_log
 */

TEST(GuiNicklist, PrintLog)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_nicklist_end
 */

TEST(GuiNicklist, End)
{
    /* TODO: write tests */
}
