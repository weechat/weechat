/*
 * test-core-list.cpp - test list functions
 *
 * Copyright (C) 2014-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/core/core-list.h"
#include "src/plugins/plugin.h"
}

#define LIST_VALUE_TEST "test"
#define LIST_VALUE_XYZ  "xyz"
#define LIST_VALUE_ZZZ  "zzz"

int list_value_user_data_test;
int list_value_user_data_xyz;
int list_value_user_data_zzz;

TEST_GROUP(CoreList)
{
};

/*
 * Creates a list for tests.
 */

struct t_weelist *
test_list_new ()
{
    struct t_weelist *list;

    list = weelist_new ();

    weelist_add (list, LIST_VALUE_ZZZ, WEECHAT_LIST_POS_END,
                 &list_value_user_data_zzz);
    weelist_add (list, LIST_VALUE_TEST, WEECHAT_LIST_POS_BEGINNING,
                 &list_value_user_data_test);
    weelist_add (list, LIST_VALUE_XYZ, WEECHAT_LIST_POS_SORT,
                 &list_value_user_data_xyz);

    return list;
}

/*
 * Tests functions:
 *   weelist_new
 */

TEST(CoreList, New)
{
    struct t_weelist *list;

    list = weelist_new();
    CHECK(list);

    /* check initial values */
    POINTERS_EQUAL(NULL, list->items);
    POINTERS_EQUAL(NULL, list->last_item);
    LONGS_EQUAL(0, list->size);

    /* free list */
    weelist_free (list);
}

/*
 * Tests functions:
 *   weelist_add
 *   weelist_free
 */

TEST(CoreList, Add)
{
    struct t_weelist *list;
    struct t_weelist_item *item1, *item2, *item3;
    const char *str_user_data = "some user data";

    list = weelist_new();

    POINTERS_EQUAL(NULL, weelist_add (NULL, NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, weelist_add (list, NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, weelist_add (NULL, LIST_VALUE_TEST, NULL, NULL));
    POINTERS_EQUAL(NULL, weelist_add (NULL, NULL, WEECHAT_LIST_POS_END, NULL));
    POINTERS_EQUAL(NULL, weelist_add (list, LIST_VALUE_TEST, NULL, NULL));
    POINTERS_EQUAL(NULL, weelist_add (list, NULL, WEECHAT_LIST_POS_END, NULL));
    POINTERS_EQUAL(NULL, weelist_add (NULL, LIST_VALUE_TEST,
                                      WEECHAT_LIST_POS_END, NULL));

    /* add an element at the end */
    item1 = weelist_add (list, LIST_VALUE_ZZZ, WEECHAT_LIST_POS_END,
                         (void *)str_user_data);
    CHECK(item1);
    CHECK(item1->data);
    CHECK(item1->user_data);
    STRCMP_EQUAL(LIST_VALUE_ZZZ, item1->data);
    POINTERS_EQUAL(str_user_data, item1->user_data);
    LONGS_EQUAL(1, list->size);  /* list is now: ["zzz"] */
    POINTERS_EQUAL(item1, list->items);

    /* add an element at the beginning */
    item2 = weelist_add (list, LIST_VALUE_TEST, WEECHAT_LIST_POS_BEGINNING,
                         (void *)str_user_data);
    CHECK(item2);
    CHECK(item2->data);
    CHECK(item2->user_data);
    STRCMP_EQUAL(LIST_VALUE_TEST, item2->data);
    POINTERS_EQUAL(str_user_data, item2->user_data);
    LONGS_EQUAL(2, list->size);  /* list is now: ["test", "zzz"] */
    POINTERS_EQUAL(item2, list->items);
    POINTERS_EQUAL(item1, list->items->next_item);

    /* add an element, using sort */
    item3 = weelist_add (list, LIST_VALUE_XYZ, WEECHAT_LIST_POS_SORT,
                         (void *)str_user_data);
    CHECK(item3);
    CHECK(item3->data);
    CHECK(item3->user_data);
    STRCMP_EQUAL(LIST_VALUE_XYZ, item3->data);
    POINTERS_EQUAL(str_user_data, item3->user_data);
    LONGS_EQUAL(3, list->size);  /* list is now: ["test", "xyz", "zzz"] */
    POINTERS_EQUAL(item2, list->items);
    POINTERS_EQUAL(item3, list->items->next_item);
    POINTERS_EQUAL(item1, list->items->next_item->next_item);

    /* free list */
    weelist_free (list);

    /* test free of NULL list */
    weelist_free (NULL);
}

/*
 * Tests functions:
 *   weelist_search
 *   weelist_search_pos
 *   weelist_casesearch
 *   weelist_casesearch_pos
 */

TEST(CoreList, Search)
{
    struct t_weelist *list;
    struct t_weelist_item *ptr_item;

    list = test_list_new ();

    /* search an element */

    POINTERS_EQUAL(NULL, weelist_search (NULL, NULL));
    POINTERS_EQUAL(NULL, weelist_search (list, NULL));
    POINTERS_EQUAL(NULL, weelist_search (NULL, LIST_VALUE_TEST));

    POINTERS_EQUAL(NULL, weelist_search (list, "not found"));
    POINTERS_EQUAL(NULL, weelist_search (list, "TEST"));

    ptr_item = weelist_search (list, LIST_VALUE_TEST);
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_TEST, ptr_item->data);

    ptr_item = weelist_search (list, LIST_VALUE_XYZ);
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_XYZ, ptr_item->data);

    ptr_item = weelist_search (list, LIST_VALUE_ZZZ);
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_ZZZ, ptr_item->data);

    /* search the position of an element */

    LONGS_EQUAL(-1, weelist_search_pos (NULL, NULL));
    LONGS_EQUAL(-1, weelist_search_pos (list, NULL));
    LONGS_EQUAL(-1, weelist_search_pos (NULL, LIST_VALUE_TEST));

    LONGS_EQUAL(-1, weelist_search_pos (list, "not found"));
    LONGS_EQUAL(-1, weelist_search_pos (list, "TEST"));

    LONGS_EQUAL(0, weelist_search_pos (list, LIST_VALUE_TEST));
    LONGS_EQUAL(1, weelist_search_pos (list, LIST_VALUE_XYZ));
    LONGS_EQUAL(2, weelist_search_pos (list, LIST_VALUE_ZZZ));

    /* case-insensitive search of an element */

    POINTERS_EQUAL(NULL, weelist_casesearch (NULL, NULL));
    POINTERS_EQUAL(NULL, weelist_casesearch (list, NULL));
    POINTERS_EQUAL(NULL, weelist_casesearch (NULL, LIST_VALUE_TEST));

    POINTERS_EQUAL(NULL, weelist_casesearch (list, "not found"));

    ptr_item = weelist_casesearch (list, "TEST");
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_TEST, ptr_item->data);

    ptr_item = weelist_casesearch (list, LIST_VALUE_TEST);
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_TEST, ptr_item->data);

    ptr_item = weelist_casesearch (list, LIST_VALUE_XYZ);
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_XYZ, ptr_item->data);

    ptr_item = weelist_casesearch (list, LIST_VALUE_ZZZ);
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_ZZZ, ptr_item->data);

    /* case-insensitive search of an element position */

    LONGS_EQUAL(-1, weelist_casesearch_pos (NULL, NULL));
    LONGS_EQUAL(-1, weelist_casesearch_pos (list, NULL));
    LONGS_EQUAL(-1, weelist_casesearch_pos (NULL, LIST_VALUE_TEST));

    LONGS_EQUAL(-1, weelist_casesearch_pos (list, "not found"));

    LONGS_EQUAL(0, weelist_casesearch_pos (list, "TEST"));
    LONGS_EQUAL(0, weelist_casesearch_pos (list, LIST_VALUE_TEST));
    LONGS_EQUAL(1, weelist_casesearch_pos (list, LIST_VALUE_XYZ));
    LONGS_EQUAL(2, weelist_casesearch_pos (list, LIST_VALUE_ZZZ));

    /* free list */
    weelist_free (list);
}

/*
 * Tests functions:
 *   weelist_get
 *   weelist_string
 *   weelist_user_data
 */

TEST(CoreList, Get)
{
    struct t_weelist *list;
    struct t_weelist_item *ptr_item;

    list = test_list_new ();

    /* get an element by position */

    POINTERS_EQUAL(NULL, weelist_get (NULL, -1));
    POINTERS_EQUAL(NULL, weelist_get (list, -1));
    POINTERS_EQUAL(NULL, weelist_get (NULL, 0));

    POINTERS_EQUAL(NULL, weelist_get (list, 50));

    ptr_item = weelist_get (list, 0);
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_TEST, ptr_item->data);

    ptr_item = weelist_get (list, 1);
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_XYZ, ptr_item->data);

    ptr_item = weelist_get (list, 2);
    CHECK(ptr_item);
    STRCMP_EQUAL(LIST_VALUE_ZZZ, ptr_item->data);

    /* get string value of an element */

    POINTERS_EQUAL(NULL, weelist_string (NULL));

    ptr_item = weelist_get(list, 0);
    STRCMP_EQUAL(LIST_VALUE_TEST, weelist_string (ptr_item));

    ptr_item = weelist_get(list, 1);
    STRCMP_EQUAL(LIST_VALUE_XYZ, weelist_string (ptr_item));

    ptr_item = weelist_get(list, 2);
    STRCMP_EQUAL(LIST_VALUE_ZZZ, weelist_string (ptr_item));

    /* get user_data value of an element */
    POINTERS_EQUAL(NULL, weelist_user_data (NULL));

    ptr_item = weelist_get(list, 0);
    POINTERS_EQUAL(&list_value_user_data_test, weelist_user_data (ptr_item));

    ptr_item = weelist_get(list, 1);
    POINTERS_EQUAL(&list_value_user_data_xyz, weelist_user_data (ptr_item));

    ptr_item = weelist_get(list, 2);
    POINTERS_EQUAL(&list_value_user_data_zzz, weelist_user_data (ptr_item));

    /* free list */
    weelist_free (list);
}

/*
 * Tests functions:
 *   weelist_set
 */

TEST(CoreList, Set)
{
    struct t_weelist *list;
    struct t_weelist_item *ptr_item;
    const char *another_test = "another test";

    list = test_list_new ();

    ptr_item = weelist_get (list, 0);
    STRCMP_EQUAL(LIST_VALUE_TEST, ptr_item->data);

    weelist_set (NULL, NULL);
    weelist_set (ptr_item, NULL);
    weelist_set (NULL, another_test);

    weelist_set (ptr_item, another_test);
    STRCMP_EQUAL(another_test, ptr_item->data);

    /* free list */
    weelist_free (list);
}

/*
 * Tests functions:
 *   weelist_next
 *   weelist_prev
 */

TEST(CoreList, Move)
{
    struct t_weelist *list;
    struct t_weelist_item *ptr_item;

    list = test_list_new ();

    /* get next item */

    ptr_item = weelist_get (list, 0);
    STRCMP_EQUAL(LIST_VALUE_TEST, ptr_item->data);
    ptr_item = weelist_next (ptr_item);
    STRCMP_EQUAL(LIST_VALUE_XYZ, ptr_item->data);
    ptr_item = weelist_next (ptr_item);
    STRCMP_EQUAL(LIST_VALUE_ZZZ, ptr_item->data);
    ptr_item = weelist_next (ptr_item);
    POINTERS_EQUAL(NULL, ptr_item);

    /* get previous item */

    ptr_item = weelist_get(list, 2);
    STRCMP_EQUAL(LIST_VALUE_ZZZ, ptr_item->data);
    ptr_item = weelist_prev (ptr_item);
    STRCMP_EQUAL(LIST_VALUE_XYZ, ptr_item->data);
    ptr_item = weelist_prev (ptr_item);
    STRCMP_EQUAL(LIST_VALUE_TEST, ptr_item->data);

    /* free list */
    weelist_free (list);
}

/*
 * Tests functions:
 *   weelist_remove
 *   weelist_remove_all
 */

TEST(CoreList, Free)
{
    struct t_weelist *list;
    struct t_weelist_item *ptr_item;

    list = test_list_new ();

    /* remove one element */

    ptr_item = weelist_get(list, 1);
    STRCMP_EQUAL(LIST_VALUE_XYZ, ptr_item->data);

    weelist_remove (NULL, NULL);
    weelist_remove (list, NULL);
    weelist_remove (NULL, ptr_item);

    weelist_remove (list, ptr_item);

    ptr_item = weelist_get(list, 1);
    STRCMP_EQUAL(LIST_VALUE_ZZZ, ptr_item->data);
    ptr_item = weelist_get (list, 2);
    POINTERS_EQUAL(NULL, ptr_item);

    /* remove all elements */

    weelist_remove_all (list);
    LONGS_EQUAL(0, list->size);
    POINTERS_EQUAL(NULL, list->items);
    POINTERS_EQUAL(NULL, list->last_item);

    /* free list */
    weelist_free (list);
}

/*
 * Tests functions:
 *   weelist_print_log
 */

TEST(CoreList, PrintLog)
{
    /* TODO: write tests */
}
