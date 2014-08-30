/*
 * test-arraylist.cpp - test arraylist functions
 *
 * Copyright (C) 2014 Sébastien Helleu <flashcode@flashtux.org>
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

extern "C"
{
#include <string.h>
#include "src/core/wee-arraylist.h"
#include "src/core/wee-string.h"
}

#define TEST_ARRAYLIST_ADD(__result, __value)                           \
    LONGS_EQUAL(__result,                                               \
                arraylist_add (arraylist, (void *)(__value)));

TEST_GROUP(Arraylist)
{
};

/*
 * Test callback comparing two arraylist elements.
 *
 * Returns:
 *   -1: element(pointer1) < element(pointer2)
 *    0: element(pointer1) == element(pointer2)
 *    1: element(pointer1) > element(pointer2)
 */

int
test_cmp_cb (void *data, struct t_arraylist *arraylist,
             void *pointer1, void *pointer2)
{
    if (!pointer1 || !pointer2)
        return (pointer1) ? 1 : ((pointer2) ? -1 : 0);

    return string_strcasecmp ((const char *)pointer1, (const char *)pointer2);
}

void
test_arraylist (int initial_size, int sorted, int allow_duplicates)
{
    struct t_arraylist *arraylist;
    int i, index, index_insert, expected_pos;
    const char *item_aaa = "aaa";
    const char *item_abc = "abc";
    const char *item_DEF = "DEF";
    const char *item_def = "def";
    const char *item_xxx = "xxx";

    /* create arraylist */
    arraylist = arraylist_new (initial_size,
                               sorted,
                               allow_duplicates,
                               &test_cmp_cb, NULL,
                               NULL, NULL);

    /* check values after creation */
    CHECK(arraylist);
    LONGS_EQUAL(0, arraylist->size);
    LONGS_EQUAL(initial_size, arraylist->size_alloc);
    LONGS_EQUAL(initial_size, arraylist->size_alloc_min);
    if (initial_size > 0)
    {
        CHECK(arraylist->data);
        for (i = 0; i < initial_size; i++)
        {
            POINTERS_EQUAL(NULL, arraylist->data[i]);
        }
    }
    else
    {
        POINTERS_EQUAL(NULL, arraylist->data);
    }
    LONGS_EQUAL(sorted, arraylist->sorted);
    LONGS_EQUAL(allow_duplicates, arraylist->allow_duplicates);

    /* check size */
    LONGS_EQUAL(0, arraylist_size (arraylist));

    /* get element (this should always fail, the list is empty!) */
    POINTERS_EQUAL(NULL, arraylist_get (NULL, -1));
    POINTERS_EQUAL(NULL, arraylist_get (NULL, 0));
    POINTERS_EQUAL(NULL, arraylist_get (NULL, 1));
    POINTERS_EQUAL(NULL, arraylist_get (arraylist, -1));
    POINTERS_EQUAL(NULL, arraylist_get (arraylist, 0));
    POINTERS_EQUAL(NULL, arraylist_get (arraylist, 1));

    /* search element (this should always fail, the list is empty!) */
    POINTERS_EQUAL(NULL, arraylist_search (NULL, NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, arraylist_search (arraylist, NULL, NULL, NULL));
    POINTERS_EQUAL(NULL,
                   arraylist_search (NULL, (void *)item_abc, NULL, NULL));
    POINTERS_EQUAL(NULL,
                   arraylist_search (arraylist, (void *)item_abc, NULL, NULL));

    /* invalid add of element */
    LONGS_EQUAL(-1, arraylist_add (NULL, NULL));

    /* add some elements */
    if (sorted)
    {
        TEST_ARRAYLIST_ADD(0, item_xxx);
        TEST_ARRAYLIST_ADD(0, NULL);
        TEST_ARRAYLIST_ADD(1, item_def);
        TEST_ARRAYLIST_ADD(1, item_DEF);
        TEST_ARRAYLIST_ADD(1, item_abc);
    }
    else
    {
        TEST_ARRAYLIST_ADD(0, item_xxx);
        TEST_ARRAYLIST_ADD(1, NULL);
        TEST_ARRAYLIST_ADD(2, item_def);
        TEST_ARRAYLIST_ADD((allow_duplicates) ? 3 : 2, item_DEF);
        TEST_ARRAYLIST_ADD((allow_duplicates) ? 4 : 3, item_abc);
    }

    /*
     * arraylist is now:
     *   sorted:
     *     allow dup: [NULL, "abc", "DEF", "def", "xxx", (NULL)]
     *     no dup   : [NULL, "abc", "DEF", "xxx"]
     *   not sorted:
     *     allow dup: ["xxx", NULL, "def", "DEF", "abc", (NULL)]
     *     no dup   : ["xxx", NULL, "DEF", "abc"]
     */

    /* check size after adds */
    LONGS_EQUAL((allow_duplicates) ? 5 : 4, arraylist->size);
    LONGS_EQUAL((allow_duplicates) ? 5 : 4, arraylist_size (arraylist));
    LONGS_EQUAL((allow_duplicates) ? 6 : 4, arraylist->size_alloc);

    /* check content after adds */
    if (sorted)
    {
        POINTERS_EQUAL(NULL, arraylist->data[0]);
        STRCMP_EQUAL(item_abc, (const char *)arraylist->data[1]);
        if (allow_duplicates)
        {
            STRCMP_EQUAL(item_DEF, (const char *)arraylist->data[2]);
            STRCMP_EQUAL(item_def, (const char *)arraylist->data[3]);
            STRCMP_EQUAL(item_xxx, (const char *)arraylist->data[4]);
            POINTERS_EQUAL(NULL, arraylist->data[5]);
        }
        else
        {
            STRCMP_EQUAL(item_DEF, (const char *)arraylist->data[2]);
            STRCMP_EQUAL(item_xxx, (const char *)arraylist->data[3]);
        }
    }
    else
    {
        STRCMP_EQUAL(item_xxx, (const char *)arraylist->data[0]);
        POINTERS_EQUAL(NULL, arraylist->data[1]);
        if (allow_duplicates)
        {
            STRCMP_EQUAL(item_def, (const char *)arraylist->data[2]);
            STRCMP_EQUAL(item_DEF, (const char *)arraylist->data[3]);
            STRCMP_EQUAL(item_abc, (const char *)arraylist->data[4]);
            POINTERS_EQUAL(NULL, arraylist->data[5]);
        }
        else
        {
            STRCMP_EQUAL(item_DEF, (const char *)arraylist->data[2]);
            STRCMP_EQUAL(item_abc, (const char *)arraylist->data[3]);
        }
    }

    /* search elements */
    if (sorted)
    {
        /* search first element */
        POINTERS_EQUAL(NULL, arraylist_search (arraylist, NULL,
                                               &index, &index_insert));
        LONGS_EQUAL(0, index);
        LONGS_EQUAL(0, index_insert);

        /* search second element */
        POINTERS_EQUAL(item_abc, arraylist_search (arraylist, (void *)item_abc,
                                                   &index, &index_insert));
        LONGS_EQUAL(1, index);
        LONGS_EQUAL(1, index_insert);

        /* search last element */
        POINTERS_EQUAL(item_xxx,
                       arraylist_search (arraylist, (void *)item_xxx,
                                         &index, &index_insert));
        LONGS_EQUAL((allow_duplicates) ? 4 : 3, index);
        LONGS_EQUAL(-1, index_insert);

        /* search non-existing element */
        POINTERS_EQUAL(NULL,
                       arraylist_search (arraylist, (void *)item_aaa,
                                         &index, &index_insert));
        LONGS_EQUAL(-1, index);
        LONGS_EQUAL(1, index_insert);
    }
    else
    {
        /* search first element */
        POINTERS_EQUAL(item_xxx, arraylist_search (arraylist, (void *)item_xxx,
                                                   &index, &index_insert));
        LONGS_EQUAL(0, index);
        LONGS_EQUAL(-1, index_insert);

        /* search second element */
        POINTERS_EQUAL(NULL, arraylist_search (arraylist, NULL,
                                               &index, &index_insert));
        LONGS_EQUAL(1, index);
        LONGS_EQUAL(-1, index_insert);

        /* search last element */
        POINTERS_EQUAL(item_abc,
                       arraylist_search (arraylist, (void *)item_abc,
                                         &index, &index_insert));
        LONGS_EQUAL((allow_duplicates) ? 4 : 3, index);
        LONGS_EQUAL(-1, index_insert);

        /* search non-existing element */
        POINTERS_EQUAL(NULL,
                       arraylist_search (arraylist, (void *)item_aaa,
                                         &index, &index_insert));
        LONGS_EQUAL(-1, index);
        LONGS_EQUAL(-1, index_insert);
    }

    /* invalid remove of elements */
    LONGS_EQUAL(-1, arraylist_remove (NULL, -1));
    LONGS_EQUAL(-1, arraylist_remove (arraylist, -1));
    LONGS_EQUAL(-1, arraylist_remove (NULL, 0));

    /* remove the 3 first elements and check size after each remove */
    LONGS_EQUAL(0, arraylist_remove (arraylist, 0));
    LONGS_EQUAL((allow_duplicates) ? 4 : 3, arraylist->size);
    LONGS_EQUAL((allow_duplicates) ? 4 : 3, arraylist_size (arraylist));
    LONGS_EQUAL((allow_duplicates) ? 6 : 4, arraylist->size_alloc);
    LONGS_EQUAL(0, arraylist_remove (arraylist, 0));
    LONGS_EQUAL((allow_duplicates) ? 3 : 2, arraylist->size);
    LONGS_EQUAL((allow_duplicates) ? 3 : 2, arraylist_size (arraylist));
    LONGS_EQUAL((allow_duplicates) ? 6 : 4, arraylist->size_alloc);
    LONGS_EQUAL(0, arraylist_remove (arraylist, 0));
    LONGS_EQUAL((allow_duplicates) ? 2 : 1, arraylist->size);
    LONGS_EQUAL((allow_duplicates) ? 2 : 1, arraylist_size (arraylist));
    LONGS_EQUAL((allow_duplicates) ? 3 : 2, arraylist->size_alloc);

    /*
     * arraylist is now:
     *   sorted:
     *     allow dup: ["def", "xxx", (NULL)]
     *     no dup   : ["xxx"]
     *   not sorted:
     *     allow dup: ["DEF", "abc", (NULL)]
     *     no dup   : ["abc"]
     */

    /* check content after the 3 deletions */
    if (sorted)
    {
        if (allow_duplicates)
        {
            STRCMP_EQUAL(item_def, (const char *)arraylist->data[0]);
            STRCMP_EQUAL(item_xxx, (const char *)arraylist->data[1]);
            POINTERS_EQUAL(NULL, arraylist->data[2]);
        }
        else
        {
            STRCMP_EQUAL(item_xxx, (const char *)arraylist->data[0]);
        }
    }
    else
    {
        if (allow_duplicates)
        {
            STRCMP_EQUAL(item_DEF, (const char *)arraylist->data[0]);
            STRCMP_EQUAL(item_abc, (const char *)arraylist->data[1]);
            POINTERS_EQUAL(NULL, arraylist->data[2]);
        }
        else
        {
            STRCMP_EQUAL(item_abc, (const char *)arraylist->data[0]);
        }
    }

    /* invalid insert of element */
    LONGS_EQUAL(-1, arraylist_insert (NULL, 0, NULL));

    /* insert of one element */
    LONGS_EQUAL(0, arraylist_insert (arraylist, 0, (void *)item_aaa));

    /*
     * arraylist is now:
     *   sorted:
     *     allow dup: ["aaa", "def", "xxx", (NULL)]
     *     no dup   : ["aaa", "xxx"]
     *   not sorted:
     *     allow dup: ["aaa", "DEF", "abc", (NULL)]
     *     no dup   : ["aaa", "abc"]
     */

    /* check size after insert */
    LONGS_EQUAL((allow_duplicates) ? 3 : 2, arraylist->size);
    LONGS_EQUAL((allow_duplicates) ? 3 : 2, arraylist_size (arraylist));
    LONGS_EQUAL((allow_duplicates) ? 3 : 2, arraylist->size_alloc);

    /* check content after the insert */
    if (sorted)
    {
        if (allow_duplicates)
        {
            STRCMP_EQUAL(item_aaa, (const char *)arraylist->data[0]);
            STRCMP_EQUAL(item_def, (const char *)arraylist->data[1]);
            STRCMP_EQUAL(item_xxx, (const char *)arraylist->data[2]);
        }
        else
        {
            STRCMP_EQUAL(item_aaa, (const char *)arraylist->data[0]);
            STRCMP_EQUAL(item_xxx, (const char *)arraylist->data[1]);
        }
    }
    else
    {
        if (allow_duplicates)
        {
            STRCMP_EQUAL(item_aaa, (const char *)arraylist->data[0]);
            STRCMP_EQUAL(item_DEF, (const char *)arraylist->data[1]);
            STRCMP_EQUAL(item_abc, (const char *)arraylist->data[2]);
        }
        else
        {
            STRCMP_EQUAL(item_aaa, (const char *)arraylist->data[0]);
            STRCMP_EQUAL(item_abc, (const char *)arraylist->data[1]);
        }
    }

    /* clear arraylist */
    LONGS_EQUAL(0, arraylist_clear (NULL));
    LONGS_EQUAL(1, arraylist_clear (arraylist));

    /* check size and data after clear */
    LONGS_EQUAL(0, arraylist->size);
    LONGS_EQUAL(0, arraylist_size (arraylist));
    LONGS_EQUAL(initial_size, arraylist->size_alloc);
    if (initial_size > 0)
    {
        CHECK(arraylist->data);
        for (i = 0; i < initial_size; i++)
        {
            POINTERS_EQUAL(NULL, arraylist->data[i]);
        }
    }
    else
    {
        POINTERS_EQUAL(NULL, arraylist->data);
    }

    /* free arraylist */
    arraylist_free (arraylist);
}

/*
 * Tests functions:
 *   arraylist_new
 *   arraylist_size
 *   arraylist_get
 *   arraylist_search
 *   arraylist_insert
 *   arraylist_add
 *   arraylist_remove
 *   arraylist_clear
 *   arraylist_free
 */

TEST(Arraylist, New)
{
    int initial_size, sorted, allow_duplicates;

    /*
     * in order to create an arraylist, initial_size must be >= 0 and a
     * comparison callback must be given
     */
    POINTERS_EQUAL(NULL,
                   arraylist_new (-1, 0, 0, NULL, NULL, NULL, NULL));
    POINTERS_EQUAL(NULL,
                   arraylist_new (-1, 0, 0, &test_cmp_cb, NULL, NULL, NULL));
    POINTERS_EQUAL(NULL,
                   arraylist_new (0, 0, 0, NULL, NULL, NULL, NULL));

    /* tests on arraylists */
    for (initial_size = 0; initial_size < 2; initial_size++)
    {
        for (sorted = 0; sorted < 2; sorted++)
        {
            for (allow_duplicates = 0; allow_duplicates < 2;
                 allow_duplicates++)
            {
                test_arraylist (initial_size, sorted, allow_duplicates);
            }
        }
    }
}
