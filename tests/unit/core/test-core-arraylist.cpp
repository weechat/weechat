/*
 * test-core-arraylist.cpp - test arraylist functions
 *
 * Copyright (C) 2014-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/core/wee-arraylist.h"
#include "src/core/wee-string.h"
}

#define TEST_ARRAYLIST_ADD(__result, __value)                           \
    LONGS_EQUAL(__result,                                               \
                arraylist_add (arraylist, (void *)(__value)));

#define TEST_ARRAYLIST_SEARCH(__result_ptr, __result_index,             \
                              __result_index_insert, __value)           \
    pointer = arraylist_search (arraylist, (void *)(__value),           \
                                &index, &index_insert);                 \
    if (__result_ptr != NULL && pointer)                                \
    {                                                                   \
        STRCMP_EQUAL(__result_ptr, (const char *)pointer);              \
    }                                                                   \
    else                                                                \
    {                                                                   \
        POINTERS_EQUAL(__result_ptr, pointer);                          \
    }                                                                   \
    LONGS_EQUAL(__result_index, index);                                 \
    LONGS_EQUAL(__result_index_insert, index_insert);

TEST_GROUP(CoreArraylist)
{
};

/*
 * Test callback comparing two arraylist elements.
 * Note: NULL element is considered lower than any other.
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
    /* make C++ compiler happy */
    (void) data;
    (void) arraylist;

    if (!pointer1 || !pointer2)
        return (pointer1) ? 1 : ((pointer2) ? -1 : 0);

    return string_strcasecmp ((const char *)pointer1, (const char *)pointer2);
}

void
test_arraylist (int initial_size, int sorted, int allow_duplicates)
{
    struct t_arraylist *arraylist;
    int i, index, index_insert;
    void *pointer;
    const char *item_aaa = "aaa";
    const char *item_abc = "abc";
    const char *item_DEF = "DEF";
    const char *item_Def = "Def";
    const char *item_def = "def";
    const char *item_xxx = "xxx";
    const char *item_zzz = "zzz";

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
        TEST_ARRAYLIST_ADD(0, item_zzz);
        TEST_ARRAYLIST_ADD(0, item_xxx);
        TEST_ARRAYLIST_ADD(0, NULL);
        TEST_ARRAYLIST_ADD(1, item_DEF);
        TEST_ARRAYLIST_ADD((allow_duplicates) ? 2 : 1, item_def);
        TEST_ARRAYLIST_ADD((allow_duplicates) ? 3 : 1, item_Def);
        TEST_ARRAYLIST_ADD(1, item_abc);
    }
    else
    {
        TEST_ARRAYLIST_ADD(0, item_zzz);
        TEST_ARRAYLIST_ADD(1, item_xxx);
        TEST_ARRAYLIST_ADD(2, NULL);
        TEST_ARRAYLIST_ADD(3, item_DEF);
        TEST_ARRAYLIST_ADD((allow_duplicates) ? 4 : 3, item_def);
        TEST_ARRAYLIST_ADD((allow_duplicates) ? 5 : 3, item_Def);
        TEST_ARRAYLIST_ADD((allow_duplicates) ? 6 : 4, item_abc);
    }

    /*
     * arraylist is now:
     *   sorted:
     *     dup   : [NULL, "abc", "DEF", "def", "Def", "xxx", "zzz"] + 2 NULL
     *     no dup: [NULL, "abc", "Def", "xxx", "zzz"] + 1 NULL
     *   not sorted:
     *     dup   : ["zzz", "xxx", NULL, "DEF", "def", "Def", "abc"] + 2 NULL
     *     no dup: ["zzz", "xxx", NULL, "Def", "abc"] + 1 NULL
     */

    /* check size after adds */
    LONGS_EQUAL((allow_duplicates) ? 7 : 5, arraylist->size);
    LONGS_EQUAL((allow_duplicates) ? 7 : 5, arraylist_size (arraylist));
    LONGS_EQUAL((allow_duplicates) ? 9 : 6, arraylist->size_alloc);

    /* check content after adds */
    if (sorted)
    {
        if (allow_duplicates)
        {
            POINTERS_EQUAL(NULL, arraylist->data[0]);
            STRCMP_EQUAL(item_abc, (const char *)arraylist->data[1]);
            STRCMP_EQUAL(item_DEF, (const char *)arraylist->data[2]);
            STRCMP_EQUAL(item_def, (const char *)arraylist->data[3]);
            STRCMP_EQUAL(item_Def, (const char *)arraylist->data[4]);
            STRCMP_EQUAL(item_xxx, (const char *)arraylist->data[5]);
            STRCMP_EQUAL(item_zzz, (const char *)arraylist->data[6]);
            for (i = 7; i < arraylist->size_alloc; i++)
            {
                POINTERS_EQUAL(NULL, arraylist->data[i]);
            }
        }
        else
        {
            POINTERS_EQUAL(NULL, arraylist->data[0]);
            STRCMP_EQUAL(item_abc, (const char *)arraylist->data[1]);
            STRCMP_EQUAL(item_Def, (const char *)arraylist->data[2]);
            STRCMP_EQUAL(item_xxx, (const char *)arraylist->data[3]);
            STRCMP_EQUAL(item_zzz, (const char *)arraylist->data[4]);
            for (i = 5; i < arraylist->size_alloc; i++)
            {
                POINTERS_EQUAL(NULL, arraylist->data[i]);
            }
        }
    }
    else
    {
        if (allow_duplicates)
        {
            STRCMP_EQUAL(item_zzz, (const char *)arraylist->data[0]);
            STRCMP_EQUAL(item_xxx, (const char *)arraylist->data[1]);
            POINTERS_EQUAL(NULL, arraylist->data[2]);
            STRCMP_EQUAL(item_DEF, (const char *)arraylist->data[3]);
            STRCMP_EQUAL(item_def, (const char *)arraylist->data[4]);
            STRCMP_EQUAL(item_Def, (const char *)arraylist->data[5]);
            STRCMP_EQUAL(item_abc, (const char *)arraylist->data[6]);
            for (i = 7; i < arraylist->size_alloc; i++)
            {
                POINTERS_EQUAL(NULL, arraylist->data[i]);
            }
        }
        else
        {
            STRCMP_EQUAL(item_zzz, (const char *)arraylist->data[0]);
            STRCMP_EQUAL(item_xxx, (const char *)arraylist->data[1]);
            POINTERS_EQUAL(NULL, arraylist->data[2]);
            STRCMP_EQUAL(item_Def, (const char *)arraylist->data[3]);
            STRCMP_EQUAL(item_abc, (const char *)arraylist->data[4]);
            for (i = 5; i < arraylist->size_alloc; i++)
            {
                POINTERS_EQUAL(NULL, arraylist->data[i]);
            }
        }
    }

    /* search elements */
    if (sorted)
    {
        if (allow_duplicates)
        {
            TEST_ARRAYLIST_SEARCH(NULL, 0, 1, NULL);
            TEST_ARRAYLIST_SEARCH(item_abc, 1, 2, item_abc);
            TEST_ARRAYLIST_SEARCH(item_DEF, 2, 5, item_DEF);
            TEST_ARRAYLIST_SEARCH(item_DEF, 2, 5, item_def);
            TEST_ARRAYLIST_SEARCH(item_DEF, 2, 5, item_Def);
            TEST_ARRAYLIST_SEARCH(item_xxx, 5, 6, item_xxx);
            TEST_ARRAYLIST_SEARCH(item_zzz, 6, 7, item_zzz);
        }
        else
        {
            TEST_ARRAYLIST_SEARCH(NULL, 0, 1, NULL);
            TEST_ARRAYLIST_SEARCH(item_abc, 1, 2, item_abc);
            TEST_ARRAYLIST_SEARCH(item_Def, 2, 3, item_DEF);
            TEST_ARRAYLIST_SEARCH(item_Def, 2, 3, item_def);
            TEST_ARRAYLIST_SEARCH(item_Def, 2, 3, item_Def);
            TEST_ARRAYLIST_SEARCH(item_xxx, 3, 4, item_xxx);
            TEST_ARRAYLIST_SEARCH(item_zzz, 4, 5, item_zzz);
        }

        /* search non-existing element */
        TEST_ARRAYLIST_SEARCH(NULL, -1, 1, item_aaa);
    }
    else
    {
        if (allow_duplicates)
        {
            TEST_ARRAYLIST_SEARCH(item_zzz, 0, -1, item_zzz);
            TEST_ARRAYLIST_SEARCH(item_xxx, 1, -1, item_xxx);
            TEST_ARRAYLIST_SEARCH(NULL, 2, -1, NULL);
            TEST_ARRAYLIST_SEARCH(item_DEF, 3, -1, item_DEF);
            TEST_ARRAYLIST_SEARCH(item_DEF, 3, -1, item_def);
            TEST_ARRAYLIST_SEARCH(item_DEF, 3, -1, item_Def);
            TEST_ARRAYLIST_SEARCH(item_abc, 6, -1, item_abc);
        }
        else
        {
            TEST_ARRAYLIST_SEARCH(item_zzz, 0, -1, item_zzz);
            TEST_ARRAYLIST_SEARCH(item_xxx, 1, -1, item_xxx);
            TEST_ARRAYLIST_SEARCH(NULL, 2, -1, NULL);
            TEST_ARRAYLIST_SEARCH(item_Def, 3, -1, item_DEF);
            TEST_ARRAYLIST_SEARCH(item_Def, 3, -1, item_def);
            TEST_ARRAYLIST_SEARCH(item_Def, 3, -1, item_Def);
            TEST_ARRAYLIST_SEARCH(item_abc, 4, -1, item_abc);
        }

        /* search non-existing element */
        TEST_ARRAYLIST_SEARCH(NULL, -1, -1, item_aaa);
    }

    /* invalid remove of elements */
    LONGS_EQUAL(-1, arraylist_remove (NULL, -1));
    LONGS_EQUAL(-1, arraylist_remove (arraylist, -1));
    LONGS_EQUAL(-1, arraylist_remove (NULL, 0));

    /* remove the 3 first elements and check size after each remove */
    LONGS_EQUAL(0, arraylist_remove (arraylist, 0));
    LONGS_EQUAL((allow_duplicates) ? 6 : 4, arraylist->size);
    LONGS_EQUAL((allow_duplicates) ? 6 : 4, arraylist_size (arraylist));
    LONGS_EQUAL((allow_duplicates) ? 9 : 6, arraylist->size_alloc);
    LONGS_EQUAL(0, arraylist_remove (arraylist, 0));
    LONGS_EQUAL((allow_duplicates) ? 5 : 3, arraylist->size);
    LONGS_EQUAL((allow_duplicates) ? 5 : 3, arraylist_size (arraylist));
    LONGS_EQUAL((allow_duplicates) ? 9 : 6, arraylist->size_alloc);
    LONGS_EQUAL(0, arraylist_remove (arraylist, 0));
    LONGS_EQUAL((allow_duplicates) ? 4 : 2, arraylist->size);
    LONGS_EQUAL((allow_duplicates) ? 4 : 2, arraylist_size (arraylist));
    LONGS_EQUAL((allow_duplicates) ? 5 : 3, arraylist->size_alloc);

    /*
     * arraylist is now:
     *   sorted:
     *     dup   : ["def", "Def", "xxx", "zzz"] + 1 NULL
     *     no dup: ["xxx", "zzz"] + 1 NULL
     *   not sorted:
     *     dup   : ["DEF", "def", "Def", "abc"] + 1 NULL
     *     no dup: ["Def", "abc"] + 1 NULL
     */

    /* check content after the 3 deletions */
    if (sorted)
    {
        if (allow_duplicates)
        {
            STRCMP_EQUAL(item_def, (const char *)arraylist->data[0]);
            STRCMP_EQUAL(item_Def, (const char *)arraylist->data[1]);
            STRCMP_EQUAL(item_xxx, (const char *)arraylist->data[2]);
            STRCMP_EQUAL(item_zzz, (const char *)arraylist->data[3]);
            for (i = 4; i < arraylist->size_alloc; i++)
            {
                POINTERS_EQUAL(NULL, arraylist->data[i]);
            }
        }
        else
        {
            STRCMP_EQUAL(item_xxx, (const char *)arraylist->data[0]);
            STRCMP_EQUAL(item_zzz, (const char *)arraylist->data[1]);
            for (i = 2; i < arraylist->size_alloc; i++)
            {
                POINTERS_EQUAL(NULL, arraylist->data[i]);
            }
        }
    }
    else
    {
        if (allow_duplicates)
        {
            STRCMP_EQUAL(item_DEF, (const char *)arraylist->data[0]);
            STRCMP_EQUAL(item_def, (const char *)arraylist->data[1]);
            STRCMP_EQUAL(item_Def, (const char *)arraylist->data[2]);
            STRCMP_EQUAL(item_abc, (const char *)arraylist->data[3]);
            for (i = 4; i < arraylist->size_alloc; i++)
            {
                POINTERS_EQUAL(NULL, arraylist->data[i]);
            }
        }
        else
        {
            STRCMP_EQUAL(item_Def, (const char *)arraylist->data[0]);
            STRCMP_EQUAL(item_abc, (const char *)arraylist->data[1]);
            for (i = 2; i < arraylist->size_alloc; i++)
            {
                POINTERS_EQUAL(NULL, arraylist->data[i]);
            }
        }
    }

    /* invalid insert of element */
    LONGS_EQUAL(-1, arraylist_insert (NULL, 0, NULL));

    /* insert of one element */
    LONGS_EQUAL(0, arraylist_insert (arraylist, 0, (void *)item_aaa));

    /*
     * arraylist is now:
     *   sorted:
     *     dup   : ["aaa", "def", "Def", "xxx", "zzz"]
     *     no dup: ["aaa", "xxx", "zzz"]
     *   not sorted:
     *     dup   : ["aaa", "DEF", "def", "Def", "abc"]
     *     no dup: ["aaa", "Def", "abc"]
     */

    /* check size after insert */
    LONGS_EQUAL((allow_duplicates) ? 5 : 3, arraylist->size);
    LONGS_EQUAL((allow_duplicates) ? 5 : 3, arraylist_size (arraylist));
    LONGS_EQUAL((allow_duplicates) ? 5 : 3, arraylist->size_alloc);

    /* check content after the insert */
    if (sorted)
    {
        if (allow_duplicates)
        {
            STRCMP_EQUAL(item_aaa, (const char *)arraylist->data[0]);
            STRCMP_EQUAL(item_def, (const char *)arraylist->data[1]);
            STRCMP_EQUAL(item_Def, (const char *)arraylist->data[2]);
            STRCMP_EQUAL(item_xxx, (const char *)arraylist->data[3]);
            STRCMP_EQUAL(item_zzz, (const char *)arraylist->data[4]);
            for (i = 5; i < arraylist->size_alloc; i++)
            {
                POINTERS_EQUAL(NULL, arraylist->data[i]);
            }
        }
        else
        {
            STRCMP_EQUAL(item_aaa, (const char *)arraylist->data[0]);
            STRCMP_EQUAL(item_xxx, (const char *)arraylist->data[1]);
            STRCMP_EQUAL(item_zzz, (const char *)arraylist->data[2]);
            for (i = 3; i < arraylist->size_alloc; i++)
            {
                POINTERS_EQUAL(NULL, arraylist->data[i]);
            }
        }
    }
    else
    {
        if (allow_duplicates)
        {
            STRCMP_EQUAL(item_aaa, (const char *)arraylist->data[0]);
            STRCMP_EQUAL(item_DEF, (const char *)arraylist->data[1]);
            STRCMP_EQUAL(item_def, (const char *)arraylist->data[2]);
            STRCMP_EQUAL(item_Def, (const char *)arraylist->data[3]);
            STRCMP_EQUAL(item_abc, (const char *)arraylist->data[4]);
            for (i = 5; i < arraylist->size_alloc; i++)
            {
                POINTERS_EQUAL(NULL, arraylist->data[i]);
            }
        }
        else
        {
            STRCMP_EQUAL(item_aaa, (const char *)arraylist->data[0]);
            STRCMP_EQUAL(item_Def, (const char *)arraylist->data[1]);
            STRCMP_EQUAL(item_abc, (const char *)arraylist->data[2]);
            for (i = 3; i < arraylist->size_alloc; i++)
            {
                POINTERS_EQUAL(NULL, arraylist->data[i]);
            }
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

TEST(CoreArraylist, New)
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
