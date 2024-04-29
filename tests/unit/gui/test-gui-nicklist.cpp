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
 *   gui_nicklist_find_pos_group
 *   gui_nicklist_insert_group_sorted
 *   gui_nicklist_add_group_with_id
 *   gui_nicklist_add_group
 *   gui_nicklist_search_group_id
 *   gui_nicklist_search_group_name
 *   gui_nicklist_search_group
 *   gui_nicklist_remove_group
 *   gui_nicklist_remove_all
 */

TEST(GuiNicklist, AddGroup)
{
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group1, *group2, *subgroup1, *subgroup2, *subgroup3;
    char str_search_id[128];

    buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                             NULL, NULL, NULL,
                             NULL, NULL, NULL);
    CHECK(buffer);

    /* invalid: NULL buffer */
    POINTERS_EQUAL(NULL, gui_nicklist_add_group (NULL, NULL, "group1", "blue", 1));

    group1 = gui_nicklist_add_group (buffer, NULL, "group1", "blue", 1);
    CHECK(group1);
    CHECK(group1->id > 0);
    STRCMP_EQUAL("group1", group1->name);
    STRCMP_EQUAL("blue", group1->color);
    LONGS_EQUAL(1, group1->visible);
    POINTERS_EQUAL(buffer->nicklist_root, group1->parent);
    POINTERS_EQUAL(NULL, group1->children);
    POINTERS_EQUAL(NULL, group1->last_child);
    POINTERS_EQUAL(NULL, group1->nicks);
    POINTERS_EQUAL(NULL, group1->last_nick);
    POINTERS_EQUAL(NULL, group1->prev_group);
    POINTERS_EQUAL(NULL, group1->next_group);
    POINTERS_EQUAL(group1, buffer->nicklist_root->children);
    POINTERS_EQUAL(group1, buffer->nicklist_root->last_child);

    group2 = gui_nicklist_add_group (buffer, NULL, "group2", "red", 1);
    CHECK(group2);
    CHECK(group2->id > group1->id);
    STRCMP_EQUAL("group2", group2->name);
    STRCMP_EQUAL("red", group2->color);
    LONGS_EQUAL(1, group2->visible);
    POINTERS_EQUAL(buffer->nicklist_root, group2->parent);
    POINTERS_EQUAL(NULL, group2->children);
    POINTERS_EQUAL(NULL, group2->last_child);
    POINTERS_EQUAL(NULL, group2->nicks);
    POINTERS_EQUAL(NULL, group2->last_nick);
    POINTERS_EQUAL(group1, group2->prev_group);
    POINTERS_EQUAL(NULL, group2->next_group);
    POINTERS_EQUAL(group2, group1->next_group);
    POINTERS_EQUAL(group1, buffer->nicklist_root->children);
    POINTERS_EQUAL(group2, buffer->nicklist_root->last_child);

    subgroup1 = gui_nicklist_add_group (buffer, group2, "1|subgroup1", "magenta", 0);
    CHECK(subgroup1);
    CHECK(subgroup1->id > group2->id);
    STRCMP_EQUAL("1|subgroup1", subgroup1->name);
    STRCMP_EQUAL("magenta", subgroup1->color);
    LONGS_EQUAL(0, subgroup1->visible);
    POINTERS_EQUAL(group2, subgroup1->parent);
    POINTERS_EQUAL(NULL, subgroup1->children);
    POINTERS_EQUAL(NULL, subgroup1->last_child);
    POINTERS_EQUAL(NULL, subgroup1->nicks);
    POINTERS_EQUAL(NULL, subgroup1->last_nick);
    POINTERS_EQUAL(NULL, subgroup1->prev_group);
    POINTERS_EQUAL(NULL, subgroup1->next_group);
    POINTERS_EQUAL(subgroup1, group2->children);
    POINTERS_EQUAL(subgroup1, group2->last_child);

    subgroup3 = gui_nicklist_add_group (buffer, group2, "subgroup3", "cyan", 0);
    CHECK(subgroup3);
    CHECK(subgroup3->id > subgroup1->id);
    STRCMP_EQUAL("subgroup3", subgroup3->name);
    STRCMP_EQUAL("cyan", subgroup3->color);
    LONGS_EQUAL(0, subgroup3->visible);
    POINTERS_EQUAL(group2, subgroup3->parent);
    POINTERS_EQUAL(NULL, subgroup3->children);
    POINTERS_EQUAL(NULL, subgroup3->last_child);
    POINTERS_EQUAL(NULL, subgroup3->nicks);
    POINTERS_EQUAL(NULL, subgroup3->last_nick);
    POINTERS_EQUAL(subgroup1, subgroup3->prev_group);
    POINTERS_EQUAL(NULL, subgroup3->next_group);
    POINTERS_EQUAL(subgroup1, group2->children);
    POINTERS_EQUAL(subgroup3, group2->last_child);

    subgroup2 = gui_nicklist_add_group (buffer, group2, "subgroup2", "brown", 0);
    CHECK(subgroup2);
    CHECK(subgroup2->id > subgroup3->id);
    STRCMP_EQUAL("subgroup2", subgroup2->name);
    STRCMP_EQUAL("brown", subgroup2->color);
    LONGS_EQUAL(0, subgroup2->visible);
    POINTERS_EQUAL(group2, subgroup2->parent);
    POINTERS_EQUAL(NULL, subgroup2->children);
    POINTERS_EQUAL(NULL, subgroup2->last_child);
    POINTERS_EQUAL(NULL, subgroup2->nicks);
    POINTERS_EQUAL(NULL, subgroup2->last_nick);
    POINTERS_EQUAL(subgroup1, subgroup2->prev_group);
    POINTERS_EQUAL(subgroup3, subgroup2->next_group);
    POINTERS_EQUAL(subgroup1, group2->children);
    POINTERS_EQUAL(subgroup3, group2->last_child);

    POINTERS_EQUAL(NULL, gui_nicklist_search_group (NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, gui_nicklist_search_group (buffer, NULL, NULL));
    POINTERS_EQUAL(NULL, gui_nicklist_search_group (NULL, NULL, "group1"));
    POINTERS_EQUAL(NULL, gui_nicklist_search_group (buffer, NULL, "invalid_group"));
    POINTERS_EQUAL(NULL, gui_nicklist_search_group (buffer, buffer->nicklist_root, "invalid_group"));

    POINTERS_EQUAL(group1, gui_nicklist_search_group (buffer, NULL, "group1"));
    POINTERS_EQUAL(group1, gui_nicklist_search_group (buffer, buffer->nicklist_root, "group1"));
    snprintf (str_search_id, sizeof (str_search_id), "==id:%lld", group1->id);
    POINTERS_EQUAL(group1, gui_nicklist_search_group (buffer, NULL, str_search_id));
    POINTERS_EQUAL(group1, gui_nicklist_search_group (buffer, buffer->nicklist_root, str_search_id));
    POINTERS_EQUAL(group2, gui_nicklist_search_group (buffer, NULL, "group2"));
    POINTERS_EQUAL(group2, gui_nicklist_search_group (buffer, buffer->nicklist_root, "group2"));
    snprintf (str_search_id, sizeof (str_search_id), "==id:%lld", group2->id);
    POINTERS_EQUAL(group2, gui_nicklist_search_group (buffer, NULL, str_search_id));
    POINTERS_EQUAL(group2, gui_nicklist_search_group (buffer, buffer->nicklist_root, str_search_id));
    POINTERS_EQUAL(subgroup1, gui_nicklist_search_group (buffer, NULL, "subgroup1"));
    POINTERS_EQUAL(subgroup1, gui_nicklist_search_group (buffer, NULL, "1|subgroup1"));
    POINTERS_EQUAL(subgroup1, gui_nicklist_search_group (buffer, buffer->nicklist_root, "subgroup1"));
    POINTERS_EQUAL(subgroup1, gui_nicklist_search_group (buffer, buffer->nicklist_root, "1|subgroup1"));
    POINTERS_EQUAL(NULL, gui_nicklist_search_group (buffer, group1, "subgroup1"));
    POINTERS_EQUAL(NULL, gui_nicklist_search_group (buffer, group1, "1|subgroup1"));
    POINTERS_EQUAL(subgroup1, gui_nicklist_search_group (buffer, group2, "subgroup1"));
    POINTERS_EQUAL(subgroup1, gui_nicklist_search_group (buffer, group2, "1|subgroup1"));
    snprintf (str_search_id, sizeof (str_search_id), "==id:%lld", subgroup1->id);
    POINTERS_EQUAL(subgroup1, gui_nicklist_search_group (buffer, NULL, str_search_id));
    POINTERS_EQUAL(subgroup1, gui_nicklist_search_group (buffer, buffer->nicklist_root, str_search_id));
    POINTERS_EQUAL(NULL, gui_nicklist_search_group (buffer, group1, str_search_id));
    POINTERS_EQUAL(subgroup1, gui_nicklist_search_group (buffer, group2, str_search_id));
    POINTERS_EQUAL(subgroup2, gui_nicklist_search_group (buffer, NULL, "subgroup2"));
    POINTERS_EQUAL(subgroup2, gui_nicklist_search_group (buffer, buffer->nicklist_root, "subgroup2"));
    POINTERS_EQUAL(NULL, gui_nicklist_search_group (buffer, group1, "subgroup2"));
    POINTERS_EQUAL(subgroup2, gui_nicklist_search_group (buffer, group2, "subgroup2"));
    snprintf (str_search_id, sizeof (str_search_id), "==id:%lld", subgroup2->id);
    POINTERS_EQUAL(subgroup2, gui_nicklist_search_group (buffer, NULL, str_search_id));
    POINTERS_EQUAL(subgroup2, gui_nicklist_search_group (buffer, buffer->nicklist_root, str_search_id));
    POINTERS_EQUAL(NULL, gui_nicklist_search_group (buffer, group1, str_search_id));
    POINTERS_EQUAL(subgroup2, gui_nicklist_search_group (buffer, group2, str_search_id));
    POINTERS_EQUAL(subgroup3, gui_nicklist_search_group (buffer, NULL, "subgroup3"));
    POINTERS_EQUAL(subgroup3, gui_nicklist_search_group (buffer, buffer->nicklist_root, "subgroup3"));
    POINTERS_EQUAL(NULL, gui_nicklist_search_group (buffer, group1, "subgroup3"));
    POINTERS_EQUAL(subgroup3, gui_nicklist_search_group (buffer, group2, "subgroup3"));
    snprintf (str_search_id, sizeof (str_search_id), "==id:%lld", subgroup3->id);
    POINTERS_EQUAL(subgroup3, gui_nicklist_search_group (buffer, NULL, str_search_id));
    POINTERS_EQUAL(subgroup3, gui_nicklist_search_group (buffer, buffer->nicklist_root, str_search_id));
    POINTERS_EQUAL(NULL, gui_nicklist_search_group (buffer, group1, str_search_id));
    POINTERS_EQUAL(subgroup3, gui_nicklist_search_group (buffer, group2, str_search_id));

    /* test remove of NULL buffer/group */
    gui_nicklist_remove_group (NULL, NULL);
    gui_nicklist_remove_group (buffer, NULL);
    gui_nicklist_remove_group (NULL, group1);

    gui_nicklist_remove_group (buffer, group1);
    POINTERS_EQUAL(NULL, gui_nicklist_search_group (buffer, NULL, "group1"));
    POINTERS_EQUAL(group2, buffer->nicklist_root->children);
    POINTERS_EQUAL(group2, buffer->nicklist_root->last_child);

    gui_nicklist_remove_group (buffer, subgroup2);
    POINTERS_EQUAL(NULL, gui_nicklist_search_group (buffer, NULL, "subgroup2"));
    POINTERS_EQUAL(subgroup1, group2->children);
    POINTERS_EQUAL(subgroup3, group2->children->next_group);
    POINTERS_EQUAL(NULL, group2->children->next_group->next_group);

    gui_nicklist_remove_all (buffer);
    POINTERS_EQUAL(NULL, gui_nicklist_search_group (buffer, NULL, "group2"));
    POINTERS_EQUAL(NULL, gui_nicklist_search_group (buffer, NULL, "subgroup1"));
    POINTERS_EQUAL(NULL, gui_nicklist_search_group (buffer, NULL, "subgroup2"));
    POINTERS_EQUAL(NULL, gui_nicklist_search_group (buffer, NULL, "subgroup3"));
    POINTERS_EQUAL(NULL, buffer->nicklist_root->children);
    POINTERS_EQUAL(NULL, buffer->nicklist_root->last_child);

    gui_buffer_close (buffer);
}

/*
 * Tests functions:
 *   gui_nicklist_find_pos_nick
 *   gui_nicklist_insert_nick_sorted
 *   gui_nicklist_add_nick_with_id
 *   gui_nicklist_add_nick
 *   gui_nicklist_search_nick_id
 *   gui_nicklist_search_nick_name
 *   gui_nicklist_search_nick
 *   gui_nicklist_remove_nick
 *   gui_nicklist_remove_all
 */

TEST(GuiNicklist, AddNick)
{
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group1, *group2;
    struct t_gui_nick *nick_root, *nick1, *nick2, *nick3;
    char str_search_id[128];

    buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                             NULL, NULL, NULL,
                             NULL, NULL, NULL);
    CHECK(buffer);

    /* invalid: NULL buffer */
    POINTERS_EQUAL(NULL, gui_nicklist_add_nick (NULL, NULL,
                                                "nick_root", "green",
                                                "@", "lightgreen", 1));

    nick_root = gui_nicklist_add_nick (buffer, NULL,
                                       "nick_root", "green",
                                       "@", "lightgreen", 1);
    CHECK(nick_root);
    CHECK(nick_root->id > 0);
    POINTERS_EQUAL(buffer->nicklist_root, nick_root->group);
    STRCMP_EQUAL("nick_root", nick_root->name);
    STRCMP_EQUAL("green", nick_root->color);
    STRCMP_EQUAL("@", nick_root->prefix);
    STRCMP_EQUAL("lightgreen", nick_root->prefix_color);
    LONGS_EQUAL(1, nick_root->visible);
    POINTERS_EQUAL(NULL, nick_root->prev_nick);
    POINTERS_EQUAL(NULL, nick_root->next_nick);
    POINTERS_EQUAL(nick_root, buffer->nicklist_root->nicks);
    POINTERS_EQUAL(nick_root, buffer->nicklist_root->last_nick);

    group1 = gui_nicklist_add_group (buffer, NULL, "group1", "blue", 1);
    CHECK(group1);
    POINTERS_EQUAL(NULL, group1->nicks);
    POINTERS_EQUAL(NULL, group1->last_nick);

    group2 = gui_nicklist_add_group (buffer, NULL, "group2", "lightblue", 1);
    CHECK(group2);
    POINTERS_EQUAL(NULL, group2->nicks);
    POINTERS_EQUAL(NULL, group2->last_nick);

    nick1 = gui_nicklist_add_nick (buffer, group2, "nick1", "cyan", "@", "lightgreen", 1);
    CHECK(nick1);
    CHECK(nick1->id > nick_root->id);
    POINTERS_EQUAL(group2, nick1->group);
    STRCMP_EQUAL("nick1", nick1->name);
    STRCMP_EQUAL("cyan", nick1->color);
    STRCMP_EQUAL("@", nick1->prefix);
    STRCMP_EQUAL("lightgreen", nick1->prefix_color);
    LONGS_EQUAL(1, nick1->visible);
    POINTERS_EQUAL(NULL, nick1->prev_nick);
    POINTERS_EQUAL(NULL, nick1->next_nick);
    POINTERS_EQUAL(nick1, group2->nicks);
    POINTERS_EQUAL(nick1, group2->last_nick);

    nick3 = gui_nicklist_add_nick (buffer, group2, "nick3", "yellow", NULL, NULL, 0);
    CHECK(nick3);
    CHECK(nick3->id > nick1->id);
    POINTERS_EQUAL(group2, nick3->group);
    STRCMP_EQUAL("nick3", nick3->name);
    STRCMP_EQUAL("yellow", nick3->color);
    POINTERS_EQUAL(NULL, nick3->prefix);
    POINTERS_EQUAL(NULL, nick3->prefix_color);
    LONGS_EQUAL(0, nick3->visible);
    POINTERS_EQUAL(nick1, nick3->prev_nick);
    POINTERS_EQUAL(NULL, nick3->next_nick);
    POINTERS_EQUAL(nick1, group2->nicks);
    POINTERS_EQUAL(nick3, group2->last_nick);

    nick2 = gui_nicklist_add_nick (buffer, group2, "nick2", "lightblue", NULL, NULL, 0);
    CHECK(nick2);
    CHECK(nick2->id > nick3->id);
    POINTERS_EQUAL(group2, nick2->group);
    STRCMP_EQUAL("nick2", nick2->name);
    STRCMP_EQUAL("lightblue", nick2->color);
    POINTERS_EQUAL(NULL, nick2->prefix);
    POINTERS_EQUAL(NULL, nick2->prefix_color);
    LONGS_EQUAL(0, nick2->visible);
    POINTERS_EQUAL(nick1, nick2->prev_nick);
    POINTERS_EQUAL(nick3, nick2->next_nick);
    POINTERS_EQUAL(nick1, group2->nicks);
    POINTERS_EQUAL(nick3, group2->last_nick);

    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (buffer, NULL, NULL));
    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (NULL, NULL, "nick_root"));
    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (buffer, NULL, "invalid_nick"));
    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (buffer, buffer->nicklist_root, "invalid_nick"));

    POINTERS_EQUAL(nick_root, gui_nicklist_search_nick (buffer, NULL, "nick_root"));
    POINTERS_EQUAL(nick_root, gui_nicklist_search_nick (buffer, buffer->nicklist_root, "nick_root"));
    POINTERS_EQUAL(nick1, gui_nicklist_search_nick (buffer, NULL, "nick1"));
    POINTERS_EQUAL(nick1, gui_nicklist_search_nick (buffer, buffer->nicklist_root, "nick1"));
    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (buffer, group1, "nick1"));
    POINTERS_EQUAL(nick1, gui_nicklist_search_nick (buffer, group2, "nick1"));
    snprintf (str_search_id, sizeof (str_search_id), "==id:%lld", nick1->id);
    POINTERS_EQUAL(nick1, gui_nicklist_search_nick (buffer, NULL, str_search_id));
    POINTERS_EQUAL(nick1, gui_nicklist_search_nick (buffer, buffer->nicklist_root, str_search_id));
    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (buffer, group1, str_search_id));
    POINTERS_EQUAL(nick1, gui_nicklist_search_nick (buffer, group2, str_search_id));
    POINTERS_EQUAL(nick2, gui_nicklist_search_nick (buffer, NULL, "nick2"));
    POINTERS_EQUAL(nick2, gui_nicklist_search_nick (buffer, buffer->nicklist_root, "nick2"));
    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (buffer, group1, "nick2"));
    POINTERS_EQUAL(nick2, gui_nicklist_search_nick (buffer, group2, "nick2"));
    snprintf (str_search_id, sizeof (str_search_id), "==id:%lld", nick2->id);
    POINTERS_EQUAL(nick2, gui_nicklist_search_nick (buffer, NULL, str_search_id));
    POINTERS_EQUAL(nick2, gui_nicklist_search_nick (buffer, buffer->nicklist_root, str_search_id));
    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (buffer, group1, str_search_id));
    POINTERS_EQUAL(nick2, gui_nicklist_search_nick (buffer, group2, str_search_id));
    POINTERS_EQUAL(nick3, gui_nicklist_search_nick (buffer, NULL, "nick3"));
    POINTERS_EQUAL(nick3, gui_nicklist_search_nick (buffer, buffer->nicklist_root, "nick3"));
    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (buffer, group1, "nick3"));
    POINTERS_EQUAL(nick3, gui_nicklist_search_nick (buffer, group2, "nick3"));
    snprintf (str_search_id, sizeof (str_search_id), "==id:%lld", nick3->id);
    POINTERS_EQUAL(nick3, gui_nicklist_search_nick (buffer, NULL, str_search_id));
    POINTERS_EQUAL(nick3, gui_nicklist_search_nick (buffer, buffer->nicklist_root, str_search_id));
    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (buffer, group1, str_search_id));
    POINTERS_EQUAL(nick3, gui_nicklist_search_nick (buffer, group2, str_search_id));

    /* test remove of NULL buffer/group */
    gui_nicklist_remove_nick (NULL, NULL);
    gui_nicklist_remove_nick (buffer, NULL);
    gui_nicklist_remove_nick (NULL, nick_root);

    gui_nicklist_remove_nick (buffer, nick_root);
    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (buffer, NULL, "nick_root"));
    POINTERS_EQUAL(NULL, buffer->nicklist_root->nicks);
    POINTERS_EQUAL(NULL, buffer->nicklist_root->last_nick);

    gui_nicklist_remove_all (buffer);
    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (buffer, NULL, "nick1"));
    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (buffer, NULL, "nick2"));
    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (buffer, NULL, "nick3"));
    POINTERS_EQUAL(NULL, buffer->nicklist_root->children);
    POINTERS_EQUAL(NULL, buffer->nicklist_root->last_child);

    gui_buffer_close (buffer);
}

/*
 * Tests functions:
 *   gui_nicklist_get_next_item
 */

TEST(GuiNicklist, GetNextItem)
{
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group1, *group2, *group3, *ptr_group;
    struct t_gui_nick *nick_root1, *nick_root2, *nick1, *nick2, *nick3, *ptr_nick;

    buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                             NULL, NULL, NULL,
                             NULL, NULL, NULL);
    CHECK(buffer);

    nick_root1 = gui_nicklist_add_nick (buffer, NULL, "nick_root1", "green", "@", "lightgreen", 1);
    CHECK(nick_root1);
    nick_root2 = gui_nicklist_add_nick (buffer, NULL, "nick_root2", "green", "@", "lightgreen", 1);
    CHECK(nick_root2);
    group1 = gui_nicklist_add_group (buffer, NULL, "group1", "blue", 1);
    CHECK(group1);
    group2 = gui_nicklist_add_group (buffer, NULL, "group2", "lightblue", 1);
    CHECK(group2);
    group3 = gui_nicklist_add_group (buffer, NULL, "group3", "blue", 1);
    CHECK(group3);
    nick1 = gui_nicklist_add_nick (buffer, group2, "nick1", "cyan", "@", "lightgreen", 1);
    CHECK(nick1);
    nick3 = gui_nicklist_add_nick (buffer, group2, "nick3", "yellow", NULL, NULL, 0);
    CHECK(nick3);
    nick2 = gui_nicklist_add_nick (buffer, group2, "nick2", "lightblue", NULL, NULL, 0);
    CHECK(nick2);

    ptr_group = NULL;
    ptr_nick = NULL;

    /* invalid: NULL buffer */
    gui_nicklist_get_next_item (NULL, &ptr_group, &ptr_nick);
    POINTERS_EQUAL(NULL, ptr_group);
    POINTERS_EQUAL(NULL, ptr_nick);

    gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
    POINTERS_EQUAL(buffer->nicklist_root, ptr_group);
    POINTERS_EQUAL(NULL, ptr_nick);

    gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
    POINTERS_EQUAL(group1, ptr_group);
    POINTERS_EQUAL(NULL, ptr_nick);

    gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
    POINTERS_EQUAL(group2, ptr_group);
    POINTERS_EQUAL(NULL, ptr_nick);

    gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
    POINTERS_EQUAL(group2, ptr_group);
    POINTERS_EQUAL(nick1, ptr_nick);

    gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
    POINTERS_EQUAL(group2, ptr_group);
    POINTERS_EQUAL(nick2, ptr_nick);

    gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
    POINTERS_EQUAL(group2, ptr_group);
    POINTERS_EQUAL(nick3, ptr_nick);

    gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
    POINTERS_EQUAL(group3, ptr_group);
    POINTERS_EQUAL(NULL, ptr_nick);

    gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
    POINTERS_EQUAL(buffer->nicklist_root, ptr_group);
    POINTERS_EQUAL(nick_root1, ptr_nick);

    gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
    POINTERS_EQUAL(buffer->nicklist_root, ptr_group);
    POINTERS_EQUAL(nick_root2, ptr_nick);

    gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
    POINTERS_EQUAL(NULL, ptr_group);
    POINTERS_EQUAL(NULL, ptr_nick);

    gui_buffer_close (buffer);
}

/*
 * Tests functions:
 *   gui_nicklist_get_group_start
 */

TEST(GuiNicklist, GetGroupStart)
{
    const char *group_empty = "";
    const char *group1 = "group1";
    const char *group2 = "01|group2";

    POINTERS_EQUAL(NULL, gui_nicklist_get_group_start (NULL));
    STRCMP_EQUAL(group_empty, gui_nicklist_get_group_start (group_empty));
    STRCMP_EQUAL(group1, gui_nicklist_get_group_start (group1));
    STRCMP_EQUAL(group2 + 3, gui_nicklist_get_group_start (group2));
}

/*
 * Tests functions:
 *   gui_nicklist_compute_visible_count
 */

TEST(GuiNicklist, ComputeVisibleCount)
{
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group1, *group2;
    struct t_gui_nick *nick_root, *nick1, *nick2, *nick3;

    gui_nicklist_compute_visible_count (NULL, NULL);

    buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                             NULL, NULL, NULL,
                             NULL, NULL, NULL);
    CHECK(buffer);

    buffer->nicklist_groups_visible_count = 0;
    buffer->nicklist_nicks_visible_count = 0;
    gui_nicklist_compute_visible_count (buffer, buffer->nicklist_root);
    LONGS_EQUAL(0, buffer->nicklist_groups_visible_count);
    LONGS_EQUAL(0, buffer->nicklist_nicks_visible_count);

    nick_root = gui_nicklist_add_nick (buffer, NULL,
                                       "nick_root", "green",
                                       "@", "lightgreen", 1);
    CHECK(nick_root);

    buffer->nicklist_groups_visible_count = 0;
    buffer->nicklist_nicks_visible_count = 0;
    gui_nicklist_compute_visible_count (buffer, buffer->nicklist_root);
    LONGS_EQUAL(0, buffer->nicklist_groups_visible_count);
    LONGS_EQUAL(1, buffer->nicklist_nicks_visible_count);

    group1 = gui_nicklist_add_group (buffer, NULL, "group1", "blue", 1);
    CHECK(group1);

    buffer->nicklist_groups_visible_count = 0;
    buffer->nicklist_nicks_visible_count = 0;
    gui_nicklist_compute_visible_count (buffer, buffer->nicklist_root);
    LONGS_EQUAL(1, buffer->nicklist_groups_visible_count);
    LONGS_EQUAL(1, buffer->nicklist_nicks_visible_count);

    group2 = gui_nicklist_add_group (buffer, NULL, "group2", "lightblue", 1);
    CHECK(group2);

    buffer->nicklist_groups_visible_count = 0;
    buffer->nicklist_nicks_visible_count = 0;
    gui_nicklist_compute_visible_count (buffer, buffer->nicklist_root);
    LONGS_EQUAL(2, buffer->nicklist_groups_visible_count);
    LONGS_EQUAL(1, buffer->nicklist_nicks_visible_count);

    nick1 = gui_nicklist_add_nick (buffer, group2, "nick1", "cyan", "@", "lightgreen", 1);
    CHECK(nick1);

    buffer->nicklist_groups_visible_count = 0;
    buffer->nicklist_nicks_visible_count = 0;
    gui_nicklist_compute_visible_count (buffer, buffer->nicklist_root);
    LONGS_EQUAL(2, buffer->nicklist_groups_visible_count);
    LONGS_EQUAL(2, buffer->nicklist_nicks_visible_count);

    nick3 = gui_nicklist_add_nick (buffer, group2, "nick3", "yellow", NULL, NULL, 0);
    CHECK(nick3);

    buffer->nicklist_groups_visible_count = 0;
    buffer->nicklist_nicks_visible_count = 0;
    gui_nicklist_compute_visible_count (buffer, buffer->nicklist_root);
    LONGS_EQUAL(2, buffer->nicklist_groups_visible_count);
    LONGS_EQUAL(2, buffer->nicklist_nicks_visible_count);

    nick2 = gui_nicklist_add_nick (buffer, group2, "nick2", "lightblue", NULL, NULL, 0);
    CHECK(nick2);

    buffer->nicklist_groups_visible_count = 0;
    buffer->nicklist_nicks_visible_count = 0;
    gui_nicklist_compute_visible_count (buffer, buffer->nicklist_root);
    LONGS_EQUAL(2, buffer->nicklist_groups_visible_count);
    LONGS_EQUAL(2, buffer->nicklist_nicks_visible_count);

    gui_buffer_close (buffer);
}

/*
 * Tests functions:
 *   gui_nicklist_group_get_integer
 */

TEST(GuiNicklist, GroupGetInteger)
{
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group;

    buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                             NULL, NULL, NULL,
                             NULL, NULL, NULL);
    CHECK(buffer);

    group = gui_nicklist_add_group (buffer, NULL, "group", "blue", 1);
    CHECK(group);

    LONGS_EQUAL(0, gui_nicklist_group_get_integer (buffer, NULL, NULL));
    LONGS_EQUAL(0, gui_nicklist_group_get_integer (buffer, NULL, ""));
    LONGS_EQUAL(0, gui_nicklist_group_get_integer (buffer, NULL, "zzz"));
    LONGS_EQUAL(0, gui_nicklist_group_get_integer (buffer, buffer->nicklist_root, ""));
    LONGS_EQUAL(0, gui_nicklist_group_get_integer (buffer, buffer->nicklist_root, "zzz"));

    LONGS_EQUAL(0, gui_nicklist_group_get_integer (buffer, buffer->nicklist_root, "visible"));
    LONGS_EQUAL(1, gui_nicklist_group_get_integer (buffer, group, "visible"));
    LONGS_EQUAL(0, gui_nicklist_group_get_integer (buffer, buffer->nicklist_root, "level"));
    LONGS_EQUAL(1, gui_nicklist_group_get_integer (buffer, group, "level"));

    gui_buffer_close (buffer);
}

/*
 * Tests functions:
 *   gui_nicklist_group_get_string
 */

TEST(GuiNicklist, GroupGetString)
{
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group;

    buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                             NULL, NULL, NULL,
                             NULL, NULL, NULL);
    CHECK(buffer);

    group = gui_nicklist_add_group (buffer, NULL, "group", "blue", 1);
    CHECK(group);

    POINTERS_EQUAL(NULL, gui_nicklist_group_get_string (buffer, NULL, NULL));
    POINTERS_EQUAL(NULL, gui_nicklist_group_get_string (buffer, NULL, ""));
    POINTERS_EQUAL(NULL, gui_nicklist_group_get_string (buffer, NULL, "zzz"));
    POINTERS_EQUAL(NULL, gui_nicklist_group_get_string (buffer, buffer->nicklist_root, ""));
    POINTERS_EQUAL(NULL, gui_nicklist_group_get_string (buffer, buffer->nicklist_root, "zzz"));

    STRCMP_EQUAL("root", gui_nicklist_group_get_string (buffer, buffer->nicklist_root, "name"));
    STRCMP_EQUAL("group", gui_nicklist_group_get_string (buffer, group, "name"));
    POINTERS_EQUAL(NULL, gui_nicklist_group_get_string (buffer, buffer->nicklist_root, "color"));
    STRCMP_EQUAL("blue", gui_nicklist_group_get_string (buffer, group, "color"));

    gui_buffer_close (buffer);
}

/*
 * Tests functions:
 *   gui_nicklist_group_get_pointer
 */

TEST(GuiNicklist, GroupGetPointer)
{
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group;

    buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                             NULL, NULL, NULL,
                             NULL, NULL, NULL);
    CHECK(buffer);

    group = gui_nicklist_add_group (buffer, NULL, "group", "blue", 1);
    CHECK(group);

    POINTERS_EQUAL(NULL, gui_nicklist_group_get_pointer (buffer, NULL, NULL));
    POINTERS_EQUAL(NULL, gui_nicklist_group_get_pointer (buffer, NULL, ""));
    POINTERS_EQUAL(NULL, gui_nicklist_group_get_pointer (buffer, NULL, "zzz"));
    POINTERS_EQUAL(NULL, gui_nicklist_group_get_pointer (buffer, buffer->nicklist_root, ""));
    POINTERS_EQUAL(NULL, gui_nicklist_group_get_pointer (buffer, buffer->nicklist_root, "zzz"));

    POINTERS_EQUAL(NULL, gui_nicklist_group_get_pointer (buffer, buffer->nicklist_root, "parent"));
    POINTERS_EQUAL(buffer->nicklist_root, gui_nicklist_group_get_pointer (buffer, group, "parent"));

    gui_buffer_close (buffer);
}

/*
 * Tests functions:
 *   gui_nicklist_group_set
 */

TEST(GuiNicklist, GroupSet)
{
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group1, *group2;

    buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                             NULL, NULL, NULL,
                             NULL, NULL, NULL);
    CHECK(buffer);

    group1 = gui_nicklist_add_group (buffer, NULL, "group1", "blue", 1);
    CHECK(group1);

    group2 = gui_nicklist_add_group (buffer, NULL, "group2", "magenta", 1);
    CHECK(group2);

    gui_nicklist_group_set (NULL, NULL, NULL, NULL);
    gui_nicklist_group_set (buffer, NULL, NULL, NULL);
    gui_nicklist_group_set (buffer, group1, NULL, NULL);
    gui_nicklist_group_set (buffer, group1, "color", NULL);
    gui_nicklist_group_set (buffer, group1, "zzz", "test");

    gui_nicklist_group_set (buffer, group1, "id", "123");
    CHECK(group1->id == 123);
    gui_nicklist_group_set (buffer, group2, "id", "123");
    CHECK(group2->id != 123);
    gui_nicklist_group_set (buffer, group2, "id", "456");
    CHECK(group2->id == 456);
    gui_nicklist_group_set (buffer, group1, "color", "green");
    STRCMP_EQUAL("green", group1->color);
    gui_nicklist_group_set (buffer, group1, "color", "");
    STRCMP_EQUAL(NULL, group1->color);
    gui_nicklist_group_set (buffer, group1, "visible", "0");
    LONGS_EQUAL(0, group1->visible);

    gui_buffer_close (buffer);
}

/*
 * Tests functions:
 *   gui_nicklist_nick_get_integer
 */

TEST(GuiNicklist, NickGetInteger)
{
    struct t_gui_buffer *buffer;
    struct t_gui_nick *nick;

    buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                             NULL, NULL, NULL,
                             NULL, NULL, NULL);
    CHECK(buffer);

    nick = gui_nicklist_add_nick (buffer, NULL, "nick", "green", "@", "lightgreen", 1);
    CHECK(nick);

    LONGS_EQUAL(0, gui_nicklist_nick_get_integer (buffer, NULL, NULL));
    LONGS_EQUAL(0, gui_nicklist_nick_get_integer (buffer, NULL, ""));
    LONGS_EQUAL(0, gui_nicklist_nick_get_integer (buffer, NULL, "zzz"));
    LONGS_EQUAL(0, gui_nicklist_nick_get_integer (buffer, nick, "zzz"));

    LONGS_EQUAL(1, gui_nicklist_nick_get_integer (buffer, nick, "visible"));

    gui_buffer_close (buffer);
}

/*
 * Tests functions:
 *   gui_nicklist_nick_get_string
 */

TEST(GuiNicklist, NickGetString)
{
    struct t_gui_buffer *buffer;
    struct t_gui_nick *nick;

    buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                             NULL, NULL, NULL,
                             NULL, NULL, NULL);
    CHECK(buffer);

    nick = gui_nicklist_add_nick (buffer, NULL, "nick", "green", "@", "lightgreen", 1);
    CHECK(nick);

    LONGS_EQUAL(0, gui_nicklist_nick_get_string (buffer, NULL, NULL));
    LONGS_EQUAL(0, gui_nicklist_nick_get_string (buffer, NULL, ""));
    LONGS_EQUAL(0, gui_nicklist_nick_get_string (buffer, NULL, "zzz"));
    LONGS_EQUAL(0, gui_nicklist_nick_get_string (buffer, nick, "zzz"));

    STRCMP_EQUAL("nick", gui_nicklist_nick_get_string (buffer, nick, "name"));
    STRCMP_EQUAL("green", gui_nicklist_nick_get_string (buffer, nick, "color"));
    STRCMP_EQUAL("@", gui_nicklist_nick_get_string (buffer, nick, "prefix"));
    STRCMP_EQUAL("lightgreen", gui_nicklist_nick_get_string (buffer, nick, "prefix_color"));

    gui_buffer_close (buffer);
}

/*
 * Tests functions:
 *   gui_nicklist_nick_get_pointer
 */

TEST(GuiNicklist, NickGetPointer)
{
    struct t_gui_buffer *buffer;
    struct t_gui_nick *nick;

    buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                             NULL, NULL, NULL,
                             NULL, NULL, NULL);
    CHECK(buffer);

    nick = gui_nicklist_add_nick (buffer, NULL, "nick", "green", "@", "lightgreen", 1);
    CHECK(nick);

    LONGS_EQUAL(0, gui_nicklist_nick_get_pointer (buffer, NULL, NULL));
    LONGS_EQUAL(0, gui_nicklist_nick_get_pointer (buffer, NULL, ""));
    LONGS_EQUAL(0, gui_nicklist_nick_get_pointer (buffer, NULL, "zzz"));
    LONGS_EQUAL(0, gui_nicklist_nick_get_pointer (buffer, nick, "zzz"));

    POINTERS_EQUAL(buffer->nicklist_root, gui_nicklist_nick_get_pointer (buffer, nick, "group"));

    gui_buffer_close (buffer);
}

/*
 * Tests functions:
 *   gui_nicklist_nick_set
 */

TEST(GuiNicklist, NickSet)
{
    struct t_gui_buffer *buffer;
    struct t_gui_nick *nick1, *nick2;

    buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                             NULL, NULL, NULL,
                             NULL, NULL, NULL);
    CHECK(buffer);

    nick1 = gui_nicklist_add_nick (buffer, NULL, "nick1", "green", "@", "lightgreen", 1);
    CHECK(nick1);

    nick2 = gui_nicklist_add_nick (buffer, NULL, "nick2", "cyan", "+", "lightcyan", 1);
    CHECK(nick2);

    gui_nicklist_nick_set (NULL, NULL, NULL, NULL);
    gui_nicklist_nick_set (buffer, NULL, NULL, NULL);
    gui_nicklist_nick_set (buffer, nick1, NULL, NULL);
    gui_nicklist_nick_set (buffer, nick1, "color", NULL);
    gui_nicklist_nick_set (buffer, nick1, "zzz", "test");

    gui_nicklist_nick_set (buffer, nick1, "id", "123");
    CHECK(nick1->id == 123);
    gui_nicklist_nick_set (buffer, nick2, "id", "123");
    CHECK(nick2->id != 123);
    gui_nicklist_nick_set (buffer, nick2, "id", "456");
    CHECK(nick2->id == 456);
    gui_nicklist_nick_set (buffer, nick1, "color", "red");
    STRCMP_EQUAL("red", nick1->color);
    gui_nicklist_nick_set (buffer, nick1, "color", "");
    STRCMP_EQUAL(NULL, nick1->color);
    gui_nicklist_nick_set (buffer, nick1, "prefix", "+");
    STRCMP_EQUAL("+", nick1->prefix);
    gui_nicklist_nick_set (buffer, nick1, "prefix", "");
    STRCMP_EQUAL(NULL, nick1->prefix);
    gui_nicklist_nick_set (buffer, nick1, "prefix_color", "lightred");
    STRCMP_EQUAL("lightred", nick1->prefix_color);
    gui_nicklist_nick_set (buffer, nick1, "prefix_color", "");
    STRCMP_EQUAL(NULL, nick1->prefix_color);
    gui_nicklist_nick_set (buffer, nick1, "visible", "0");
    LONGS_EQUAL(0, nick1->visible);

    gui_buffer_close (buffer);
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
