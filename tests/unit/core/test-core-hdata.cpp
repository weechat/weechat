/*
 * test-core-hdata.cpp - test hdata functions
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
#include "src/core/wee-hdata.h"
#include "src/core/wee-config.h"
#include "src/core/wee-hashtable.h"
#include "src/core/wee-hook.h"
#include "src/core/wee-string.h"
#include "src/gui/gui-buffer.h"
#include "src/plugins/plugin.h"
}

struct t_test_item
{
    /* char */
    char test_char;
    char test_count_char;
    char test_array_2_char_fixed_size[2];
    char *test_ptr_2_char;

    /* integer */
    int test_int;
    int test_count_int;
    int test_array_2_int_fixed_size[2];
    int *test_ptr_3_int;
    int *test_ptr_1_int_fixed_size;

    /* long */
    long test_long;
    long test_count_long;
    long test_array_2_long_fixed_size[2];
    long *test_ptr_2_long;

    /* string */
    char *test_string;
    char *test_string2;
    char *test_string3;
    char *test_string_null;
    const char *test_shared_string;
    int test_count_words;
    char test_array_2_words_fixed_size[2][32];
    char **test_ptr_words;
    char **test_ptr_words_dyn;
    char **test_ptr_words_dyn_shared;

    /* pointer */
    void *test_pointer;
    int test_count_pointer;
    void *test_array_2_pointer_fixed_size[2];
    void **test_ptr_3_pointer;
    void **test_ptr_0_pointer_dyn;
    void **test_ptr_1_pointer_dyn;

    /* time */
    time_t test_time;
    int test_count_time;
    time_t test_array_2_time_fixed_size[2];
    time_t *test_ptr_2_time;

    /* hashtable */
    struct t_hashtable *test_hashtable;
    int test_count_hashtable;
    struct t_hashtable *test_array_2_hashtable_fixed_size[2];
    struct t_hashtable **test_ptr_2_hashtable;
    struct t_hashtable **test_ptr_1_hashtable_dyn;

    /* other */
    void *test_other;
    int test_count_other;
    void *test_ptr_3_other[3];

    /* invalid */
    char *test_count_invalid;
    int *test_ptr_invalid;

    struct t_test_item *prev_item;
    struct t_test_item *next_item;
};

struct t_test_item *items = NULL;
struct t_test_item *last_item = NULL;
struct t_test_item *ptr_item1 = NULL;
struct t_test_item *ptr_item2 = NULL;
struct t_hdata *ptr_hdata = NULL;

/*
 * Test of update callback.
 */

int
callback_update_dummy (void *data,
                       struct t_hdata *hdata,
                       void *pointer,
                       struct t_hashtable *hashtable)
{
    /* make C++ compiler happy */
    (void) data;
    (void) hdata;
    (void) pointer;
    (void) hashtable;

    return 0;
}

TEST_GROUP(CoreHdata)
{
};

/*
 * Tests functions:
 *   hdata_new
 */

TEST(CoreHdata, New)
{
    struct t_hdata *hdata;

    POINTERS_EQUAL(NULL, hdata_new (NULL, NULL, NULL, NULL, 0, 0, NULL, NULL));
    POINTERS_EQUAL(NULL, hdata_new (NULL, "", NULL, NULL, 0, 0, NULL, NULL));

    hdata = hdata_new (NULL, "test_hdata", NULL, NULL, 1, 0, NULL, NULL);
    CHECK(hdata);
    POINTERS_EQUAL(hdata, hashtable_get (weechat_hdata, "test_hdata"));
    STRCMP_EQUAL("test_hdata", hdata->name);
    POINTERS_EQUAL(NULL, hdata->plugin);
    POINTERS_EQUAL(NULL, hdata->var_prev);
    POINTERS_EQUAL(NULL, hdata->var_next);
    CHECK(hdata->hash_var);
    LONGS_EQUAL(0, hdata->hash_var->items_count);
    CHECK(hdata->hash_list);
    LONGS_EQUAL(0, hdata->hash_list->items_count);
    LONGS_EQUAL(1, hdata->create_allowed);
    LONGS_EQUAL(0, hdata->delete_allowed);
    POINTERS_EQUAL(NULL, hdata->callback_update);
    POINTERS_EQUAL(NULL, hdata->callback_update_data);
    LONGS_EQUAL(0, hdata->update_pending);
    hashtable_remove (weechat_hdata, "test_hdata");
    POINTERS_EQUAL(NULL, hashtable_get (weechat_hdata, "test_hdata"));

    hdata = hdata_new (NULL, "test_hdata", "prev", "next", 1, 0,
                       &callback_update_dummy, (void *)0x123);
    CHECK(hdata);
    POINTERS_EQUAL(hdata, hashtable_get (weechat_hdata, "test_hdata"));
    STRCMP_EQUAL("test_hdata", hdata->name);
    POINTERS_EQUAL(NULL, hdata->plugin);
    STRCMP_EQUAL("prev", hdata->var_prev);
    STRCMP_EQUAL("next", hdata->var_next);
    CHECK(hdata->hash_var);
    LONGS_EQUAL(0, hdata->hash_var->items_count);
    CHECK(hdata->hash_list);
    LONGS_EQUAL(0, hdata->hash_list->items_count);
    LONGS_EQUAL(1, hdata->create_allowed);
    LONGS_EQUAL(0, hdata->delete_allowed);
    POINTERS_EQUAL(&callback_update_dummy, hdata->callback_update);
    POINTERS_EQUAL(0x123, hdata->callback_update_data);
    LONGS_EQUAL(0, hdata->update_pending);
    hashtable_remove (weechat_hdata, "test_hdata");
    POINTERS_EQUAL(NULL, hashtable_get (weechat_hdata, "test_hdata"));
}

/*
 * Tests functions:
 *   hdata_new_var
 */

TEST(CoreHdata, NewVar)
{
    struct t_hdata *hdata;
    struct t_hdata_var *var;

    hdata = hdata_new (NULL, "test_hdata", "prev", "next", 1, 0,
                       &callback_update_dummy, (void *)0x123);
    CHECK(hdata);
    CHECK(hdata->hash_var);
    LONGS_EQUAL(0, hdata->hash_var->items_count);

    hdata_new_var (NULL, NULL, 0, 0, 0, NULL, NULL);
    hdata_new_var (hdata, NULL, 0, 0, 0, NULL, NULL);
    hdata_new_var (NULL, "var", 0, 0, 0, NULL, NULL);

    LONGS_EQUAL(0, hdata->hash_var->items_count);

    /* simple variable */
    hdata_new_var (hdata, "var1", 0, WEECHAT_HDATA_STRING, 0, NULL, NULL);
    LONGS_EQUAL(1, hdata->hash_var->items_count);
    var = (struct t_hdata_var *)hashtable_get (hdata->hash_var, "var1");
    CHECK(var);
    LONGS_EQUAL(0, var->offset);
    LONGS_EQUAL(WEECHAT_HDATA_STRING, var->type);
    LONGS_EQUAL(0, var->update_allowed);
    POINTERS_EQUAL(NULL, var->array_size);
    POINTERS_EQUAL(NULL, var->hdata_name);

    /* variable with size as variable name */
    hdata_new_var (hdata, "var2", 8, WEECHAT_HDATA_INTEGER, 1,
                   "size", "other_hdata");
    LONGS_EQUAL(2, hdata->hash_var->items_count);
    var = (struct t_hdata_var *)hashtable_get (hdata->hash_var, "var2");
    CHECK(var);
    LONGS_EQUAL(8, var->offset);
    LONGS_EQUAL(WEECHAT_HDATA_INTEGER, var->type);
    LONGS_EQUAL(1, var->update_allowed);
    STRCMP_EQUAL("size", var->array_size);
    STRCMP_EQUAL("other_hdata", var->hdata_name);

    /* variable with size as integer (fixed size) */
    hdata_new_var (hdata, "var3", 16, WEECHAT_HDATA_INTEGER, 1,
                   "8", "other_hdata");
    LONGS_EQUAL(3, hdata->hash_var->items_count);
    var = (struct t_hdata_var *)hashtable_get (hdata->hash_var, "var3");
    CHECK(var);
    LONGS_EQUAL(16, var->offset);
    LONGS_EQUAL(WEECHAT_HDATA_INTEGER, var->type);
    LONGS_EQUAL(1, var->update_allowed);
    STRCMP_EQUAL("8", var->array_size);
    STRCMP_EQUAL("other_hdata", var->hdata_name);

    /* variable with size "*" (automatic) */
    hdata_new_var (hdata, "var4", 24, WEECHAT_HDATA_INTEGER, 1,
                   "*", "other_hdata");
    LONGS_EQUAL(4, hdata->hash_var->items_count);
    var = (struct t_hdata_var *)hashtable_get (hdata->hash_var, "var4");
    CHECK(var);
    LONGS_EQUAL(24, var->offset);
    LONGS_EQUAL(WEECHAT_HDATA_INTEGER, var->type);
    LONGS_EQUAL(1, var->update_allowed);
    STRCMP_EQUAL("*", var->array_size);
    STRCMP_EQUAL("other_hdata", var->hdata_name);

    hashtable_remove (weechat_hdata, "test_hdata");
}

/*
 * Tests functions:
 *   hdata_new_list
 */

TEST(CoreHdata, NewList)
{
    struct t_hdata *hdata;
    struct t_hdata_list *list;

    hdata = hdata_new (NULL, "test_hdata", "prev", "next", 1, 0,
                       &callback_update_dummy, (void *)0x123);
    CHECK(hdata);
    CHECK(hdata->hash_list);
    LONGS_EQUAL(0, hdata->hash_list->items_count);

    hdata_new_list (NULL, NULL, NULL, 0);
    hdata_new_list (hdata, NULL, NULL, 0);
    hdata_new_list (NULL, "list", NULL, 0);

    LONGS_EQUAL(0, hdata->hash_list->items_count);

    hdata_new_list (hdata, "list1", (void *)0x123, 0);
    LONGS_EQUAL(1, hdata->hash_list->items_count);
    list = (struct t_hdata_list *)hashtable_get (hdata->hash_list, "list1");
    CHECK(list);
    POINTERS_EQUAL(0x123, list->pointer);
    LONGS_EQUAL(0, list->flags);

    hdata_new_list (hdata, "list2", (void *)0x456,
                    WEECHAT_HDATA_LIST_CHECK_POINTERS);
    LONGS_EQUAL(2, hdata->hash_list->items_count);
    list = (struct t_hdata_list *)hashtable_get (hdata->hash_list, "list2");
    CHECK(list);
    POINTERS_EQUAL(0x456, list->pointer);
    LONGS_EQUAL(WEECHAT_HDATA_LIST_CHECK_POINTERS, list->flags);

    hashtable_remove (weechat_hdata, "test_hdata");
}

TEST_GROUP(CoreHdataWithList)
{
    static int callback_update (void *data,
                                struct t_hdata *hdata,
                                void *pointer,
                                struct t_hashtable *hashtable)
    {
        const char *ptr_keys;
        char **keys;
        int rc, i, num_keys;

        /* make C++ compiler happy */
        (void) data;
        (void) hdata;
        (void) pointer;

        rc = 0;

        ptr_keys = hashtable_get_string (hashtable, "keys");
        if (!ptr_keys || !ptr_keys[0])
            return rc;

        keys = string_split (ptr_keys, ",", NULL, 0, 0, &num_keys);
        if (!keys)
            return rc;

        for (i = 0; i < num_keys; i++)
        {
            if (hashtable_has_key (hashtable, keys[i]))
            {
                rc += hdata_set (
                    hdata, pointer, keys[i],
                    (const char *)hashtable_get (hashtable, keys[i]));
            }
        }

        string_free_split (keys);

        return rc;
    }

    struct t_test_item *get_item1 ()
    {
        struct t_test_item *item;

        item = (struct t_test_item *)malloc (sizeof (*item));

        /* char */
        item->test_char = 'A';
        item->test_count_char = 2;
        item->test_array_2_char_fixed_size[0] = 'A';
        item->test_array_2_char_fixed_size[1] = 'B';
        item->test_ptr_2_char = (char *)malloc (
            2 * sizeof (item->test_ptr_2_char[0]));
        item->test_ptr_2_char[0] = 'B';
        item->test_ptr_2_char[1] = 'C';

        /* integer */
        item->test_int = 123;
        item->test_count_int = 3;
        item->test_array_2_int_fixed_size[0] = 111;
        item->test_array_2_int_fixed_size[1] = 222;
        item->test_ptr_3_int = (int *)malloc (
            3 * sizeof (item->test_ptr_3_int[0]));
        item->test_ptr_3_int[0] = 1;
        item->test_ptr_3_int[1] = 2;
        item->test_ptr_3_int[2] = 3;
        item->test_ptr_1_int_fixed_size = (int *)malloc (
            1 * sizeof (item->test_ptr_1_int_fixed_size[0]));
        item->test_ptr_1_int_fixed_size[0] = 111;

        /* long */
        item->test_long = 123456789L;
        item->test_count_long = 2;
        item->test_array_2_long_fixed_size[0] = 111L;
        item->test_array_2_long_fixed_size[1] = 222L;
        item->test_ptr_2_long = (long *)malloc (
            2 * sizeof (item->test_ptr_2_long[0]));
        item->test_ptr_2_long[0] = 123456L;
        item->test_ptr_2_long[1] = 234567L;

        /* string */
        item->test_string = strdup ("item1");
        item->test_string2 = strdup ("STRING2");
        item->test_string3 = strdup ("test");
        item->test_string_null = NULL;
        item->test_shared_string = string_shared_get ("item1_shared");
        strcpy (item->test_array_2_words_fixed_size[0], "item1-word1");
        strcpy (item->test_array_2_words_fixed_size[1], "item1-word2");
        item->test_ptr_words = string_split (
            "a,b,c", ",", NULL, 0, 0, &(item->test_count_words));
        item->test_ptr_words_dyn = string_split (
            "aa,bb,cc", ",", NULL, 0, 0, NULL);
        item->test_ptr_words_dyn_shared = string_split_shared (
            "aaa,bbb,ccc", ",", NULL, 0, 0, NULL);

        /* pointer */
        item->test_pointer = (void *)0x123;
        item->test_count_pointer = 3;
        item->test_array_2_pointer_fixed_size[0] = (void *)0x112233;
        item->test_array_2_pointer_fixed_size[1] = (void *)0x445566;
        item->test_ptr_3_pointer = (void **)malloc (
            3 * sizeof (item->test_ptr_3_pointer[0]));
        item->test_ptr_3_pointer[0] = (void *)0x123;
        item->test_ptr_3_pointer[1] = (void *)0x456;
        item->test_ptr_3_pointer[2] = (void *)0x789;
        item->test_ptr_0_pointer_dyn = NULL;
        item->test_ptr_1_pointer_dyn = (void **)malloc (
            2 * sizeof (item->test_ptr_1_pointer_dyn[0]));
        item->test_ptr_1_pointer_dyn[0] = (void *)0x123;
        item->test_ptr_1_pointer_dyn[1] = NULL;

        /* time */
        item->test_time = 123456;
        item->test_count_time = 2;
        item->test_array_2_time_fixed_size[0] = 112;
        item->test_array_2_time_fixed_size[1] = 334;
        item->test_ptr_2_time = (time_t *)malloc (
            2 * sizeof (item->test_ptr_2_time[0]));
        item->test_ptr_2_time[0] = 1234;
        item->test_ptr_2_time[1] = 5678;

        /* hashtable */
        item->test_hashtable = hashtable_new (
            8,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL,
            NULL);
        hashtable_set (item->test_hashtable, "key1", "value1");
        item->test_count_hashtable = 2;
        item->test_array_2_hashtable_fixed_size[0] = hashtable_new (
            8,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL,
            NULL);
        hashtable_set (item->test_array_2_hashtable_fixed_size[0],
                       "key_array_1.1", "value_array_1.1");
        item->test_array_2_hashtable_fixed_size[1] = hashtable_new (
            8,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL,
            NULL);
        hashtable_set (item->test_array_2_hashtable_fixed_size[1],
                       "key_array_1.2", "value_array_1.2");
        item->test_ptr_2_hashtable = (struct t_hashtable **)malloc (
            2 * sizeof (item->test_ptr_2_hashtable[0]));
        item->test_ptr_2_hashtable[0] = hashtable_new (
            8,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL,
            NULL);
        hashtable_set (item->test_ptr_2_hashtable[0],
                       "key1.1", "value1.1");
        item->test_ptr_2_hashtable[1] = hashtable_new (
            8,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL,
            NULL);
        hashtable_set (item->test_ptr_2_hashtable[1],
                       "key1.2", "value1.2");
        item->test_ptr_1_hashtable_dyn = (struct t_hashtable **)malloc (
            2 * sizeof (item->test_ptr_1_hashtable_dyn[0]));
        item->test_ptr_1_hashtable_dyn[0] = hashtable_new (
            8,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL,
            NULL);
        item->test_ptr_1_hashtable_dyn[1] = NULL;

        /* other */
        item->test_other = (void *)0x123abc;
        item->test_count_other = 3;
        item->test_ptr_3_other[0] = (void *)0x1a;
        item->test_ptr_3_other[1] = (void *)0x2b;
        item->test_ptr_3_other[2] = (void *)0x3c;

        /* invalid */
        item->test_count_invalid = NULL;
        item->test_ptr_invalid = NULL;

        return item;
    }

    struct t_test_item *get_item2 ()
    {
        struct t_test_item *item;

        item = (struct t_test_item *)malloc (sizeof (*item));

        /* char */
        item->test_char = 'a';
        item->test_count_char = 2;
        item->test_array_2_char_fixed_size[0] = 'a';
        item->test_array_2_char_fixed_size[1] = 'b';
        item->test_ptr_2_char = (char *)malloc (
            2 * sizeof (item->test_ptr_2_char[0]));
        item->test_ptr_2_char[0] = 'b';
        item->test_ptr_2_char[1] = 'c';

        /* integer */
        item->test_int = 456;
        item->test_count_int = 3;
        item->test_array_2_int_fixed_size[0] = 444;
        item->test_array_2_int_fixed_size[1] = 555;
        item->test_ptr_3_int = (int *)malloc (
            3 * sizeof (item->test_ptr_3_int[0]));
        item->test_ptr_3_int[0] = 4;
        item->test_ptr_3_int[1] = 5;
        item->test_ptr_3_int[2] = 6;
        item->test_ptr_1_int_fixed_size = (int *)malloc (
            1 * sizeof (item->test_ptr_1_int_fixed_size[0]));
        item->test_ptr_1_int_fixed_size[0] = 222;

        /* long */
        item->test_long = 987654321L;
        item->test_count_long = 2;
        item->test_array_2_long_fixed_size[0] = 333L;
        item->test_array_2_long_fixed_size[1] = 444L;
        item->test_ptr_2_long = (long *)malloc (
            2 * sizeof (item->test_ptr_2_long[0]));
        item->test_ptr_2_long[0] = 789123L;
        item->test_ptr_2_long[1] = 891234L;

        /* string */
        item->test_string = strdup ("item2");
        item->test_string2 = strdup ("string2");
        item->test_string3 = NULL;
        ptr_item1->test_string_null = NULL;
        item->test_shared_string = string_shared_get ("item2_shared");
        strcpy (item->test_array_2_words_fixed_size[0], "item2-word1");
        strcpy (item->test_array_2_words_fixed_size[1], "item2-word2");
        item->test_ptr_words = string_split (
            "e,f,g,h", ",", NULL, 0, 0, &(item->test_count_words));
        item->test_ptr_words_dyn = string_split (
            "ee,ff,gg,hh", ",", NULL, 0, 0, NULL);
        item->test_ptr_words_dyn_shared = string_split_shared (
            "eee,fff,ggg,hhh", ",", NULL, 0, 0, NULL);

        /* pointer */
        item->test_pointer = (void *)0x456;
        item->test_count_pointer = 3;
        item->test_array_2_pointer_fixed_size[0] = (void *)0x778899;
        item->test_array_2_pointer_fixed_size[1] = (void *)0xaabbcc;
        item->test_ptr_3_pointer = (void **)malloc (
            3 * sizeof (item->test_ptr_3_pointer[0]));
        item->test_ptr_3_pointer[0] = (void *)0x123abc;
        item->test_ptr_3_pointer[1] = (void *)0x456def;
        item->test_ptr_3_pointer[2] = (void *)0x789abc;
        item->test_ptr_0_pointer_dyn = NULL;
        item->test_ptr_1_pointer_dyn = (void **)malloc (
            2 * sizeof (item->test_ptr_1_pointer_dyn[0]));
        item->test_ptr_1_pointer_dyn[0] = (void *)0x456;
        item->test_ptr_1_pointer_dyn[1] = NULL;

        /* time */
        item->test_time = 789123;
        item->test_count_time = 2;
        item->test_array_2_time_fixed_size[0] = 556;
        item->test_array_2_time_fixed_size[1] = 778;
        item->test_ptr_2_time = (time_t *)malloc (
            2 * sizeof (item->test_ptr_2_time[0]));
        item->test_ptr_2_time[0] = 123456;
        item->test_ptr_2_time[1] = 789123;

        /* hashtable */
        item->test_hashtable = hashtable_new (
            8,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL,
            NULL);
        hashtable_set (item->test_hashtable, "key2", "value2");
        item->test_count_hashtable = 2;
        item->test_array_2_hashtable_fixed_size[0] = hashtable_new (
            8,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL,
            NULL);
        hashtable_set (item->test_array_2_hashtable_fixed_size[0],
                       "key_array_2.1", "value_array_2.1");
        item->test_array_2_hashtable_fixed_size[1] = hashtable_new (
            8,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL,
            NULL);
        hashtable_set (item->test_array_2_hashtable_fixed_size[1],
                       "key_array_2.2", "value_array_2.2");
        item->test_ptr_2_hashtable = (struct t_hashtable **)malloc (
            2 * sizeof (item->test_ptr_2_hashtable[0]));
        item->test_ptr_2_hashtable[0] = hashtable_new (
            8,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL,
            NULL);
        hashtable_set (item->test_ptr_2_hashtable[0],
                       "key2.1", "value2.1");
        item->test_ptr_2_hashtable[1] = hashtable_new (
            8,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL,
            NULL);
        hashtable_set (item->test_ptr_2_hashtable[1],
                       "key2.2", "value2.2");
        item->test_ptr_1_hashtable_dyn = (struct t_hashtable **)malloc (
            2 * sizeof (item->test_ptr_1_hashtable_dyn[0]));
        item->test_ptr_1_hashtable_dyn[0] = hashtable_new (
            8,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL,
            NULL);
        item->test_ptr_1_hashtable_dyn[1] = NULL;

        /* other */
        item->test_other = (void *)0x456def;
        item->test_count_other = 3;
        item->test_ptr_3_other[0] = (void *)0x4d;
        item->test_ptr_3_other[1] = (void *)0x5e;
        item->test_ptr_3_other[2] = (void *)0x6f;

        /* invalid */
        item->test_count_invalid = NULL;
        item->test_ptr_invalid = NULL;

        return item;
    }

    struct t_hdata *get_hdata ()
    {
        struct t_hdata *hdata;

        hdata = hdata_new (NULL, "test_item", "prev_item", "next_item",
                           1, 1,
                           &callback_update, NULL);

        /* char */
        HDATA_VAR(struct t_test_item, test_char, CHAR, 1, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_count_char, CHAR, 0, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_array_2_char_fixed_size, CHAR, 0, "2", NULL);
        HDATA_VAR(struct t_test_item, test_ptr_2_char, CHAR, 0, "*,test_count_char", NULL);

        /* integer */
        HDATA_VAR(struct t_test_item, test_int, INTEGER, 1, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_count_int, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_array_2_int_fixed_size, INTEGER, 0, "2", NULL);
        HDATA_VAR(struct t_test_item, test_ptr_3_int, INTEGER, 0, "*,test_count_int", NULL);
        HDATA_VAR(struct t_test_item, test_ptr_1_int_fixed_size, INTEGER, 0, "*,1", NULL);

        /* long */
        HDATA_VAR(struct t_test_item, test_long, LONG, 1, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_count_long, LONG, 0, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_array_2_long_fixed_size, LONG, 0, "2", NULL);
        HDATA_VAR(struct t_test_item, test_ptr_2_long, LONG, 0, "*,test_count_long", NULL);

        /* string */
        HDATA_VAR(struct t_test_item, test_string, STRING, 1, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_string2, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_string3, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_string_null, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_shared_string, SHARED_STRING, 1, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_count_words, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_array_2_words_fixed_size, STRING, 0, "2", NULL);
        HDATA_VAR(struct t_test_item, test_ptr_words, STRING, 0, "*,test_count_words", NULL);
        HDATA_VAR(struct t_test_item, test_ptr_words_dyn, STRING, 0, "*,*", NULL);
        HDATA_VAR(struct t_test_item, test_ptr_words_dyn_shared, SHARED_STRING, 0, "*,*", NULL);

        /* pointer */
        HDATA_VAR(struct t_test_item, test_pointer, POINTER, 1, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_count_pointer, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_array_2_pointer_fixed_size, POINTER, 0, "2", NULL);
        HDATA_VAR(struct t_test_item, test_ptr_3_pointer, POINTER, 0, "*,test_count_pointer", NULL);
        HDATA_VAR(struct t_test_item, test_ptr_0_pointer_dyn, POINTER, 0, "*,*", NULL);
        HDATA_VAR(struct t_test_item, test_ptr_1_pointer_dyn, POINTER, 0, "*,*", NULL);

        /* time */
        HDATA_VAR(struct t_test_item, test_time, TIME, 1, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_count_time, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_array_2_time_fixed_size, TIME, 0, "2", NULL);
        HDATA_VAR(struct t_test_item, test_ptr_2_time, TIME, 0, "*,test_count_time", NULL);

        /* hashtable */
        HDATA_VAR(struct t_test_item, test_hashtable, HASHTABLE, 1, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_count_hashtable, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_array_2_hashtable_fixed_size, HASHTABLE, 0, "2", NULL);
        HDATA_VAR(struct t_test_item, test_ptr_2_hashtable, HASHTABLE, 0, "*,test_count_hashtable", NULL);
        HDATA_VAR(struct t_test_item, test_ptr_1_hashtable_dyn, HASHTABLE, 0, "*,*", NULL);

        /* other */
        HDATA_VAR(struct t_test_item, test_other, OTHER, 1, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_count_other, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_ptr_3_other, OTHER, 0, "test_count_other", NULL);

        /* invalid */
        HDATA_VAR(struct t_test_item, test_count_invalid, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_test_item, test_ptr_invalid, STRING, 0, "test_count_invalid", NULL);

        /* prev/next item */
        HDATA_VAR(struct t_test_item, prev_item, POINTER, 0, NULL, "test_item");
        HDATA_VAR(struct t_test_item, next_item, POINTER, 0, NULL, "test_item");

        /* lists */
        HDATA_LIST(items, WEECHAT_HDATA_LIST_CHECK_POINTERS);
        HDATA_LIST(last_item, 0);

        return hdata;
    }

    void setup ()
    {
        /* build items */
        ptr_item1 = get_item1 ();
        ptr_item2 = get_item2 ();

        /* build a linked list: items: ptr_item1 -> ptr_item2 -> NULL */
        ptr_item1->prev_item = NULL;
        ptr_item1->next_item = ptr_item2;
        ptr_item2->prev_item = ptr_item1;
        ptr_item2->next_item = NULL;
        items = ptr_item1;
        last_item = ptr_item2;

        /* build hdata */
        ptr_hdata = get_hdata ();
    }

    void free_item (struct t_test_item *item)
    {
        /* char */
        free (item->test_ptr_2_char);

        /* integer */
        free (item->test_ptr_3_int);
        free (item->test_ptr_1_int_fixed_size);

        /* long */
        free (item->test_ptr_2_long);

        /* string */
        free (item->test_string);
        free (item->test_string2);
        free (item->test_string3);
        string_shared_free (item->test_shared_string);
        hashtable_free (item->test_hashtable);
        string_free_split (item->test_ptr_words);
        string_free_split (item->test_ptr_words_dyn);
        string_free_split_shared (item->test_ptr_words_dyn_shared);

        /* pointer */
        free (item->test_ptr_3_pointer);

        /* time */
        free (item->test_ptr_2_time);

        /* hashtable */
        hashtable_free (item->test_array_2_hashtable_fixed_size[0]);
        hashtable_free (item->test_array_2_hashtable_fixed_size[1]);
        hashtable_free (item->test_ptr_2_hashtable[0]);
        hashtable_free (item->test_ptr_2_hashtable[1]);
        free (item->test_ptr_2_hashtable);
        hashtable_free (item->test_ptr_1_hashtable_dyn[0]);
        free (item->test_ptr_1_hashtable_dyn);

        free (item);
    }

    void teardown ()
    {
        free_item (ptr_item1);
        free_item (ptr_item2);

        ptr_item1 = NULL;
        ptr_item2 = NULL;

        items = NULL;
        last_item = NULL;

        hashtable_remove (weechat_hdata, "test_item");
    }
};

/*
 * Tests functions:
 *   hdata_get_var_offset
 */

TEST(CoreHdataWithList, GetVarOffset)
{
    LONGS_EQUAL(-1, hdata_get_var_offset (NULL, NULL));
    LONGS_EQUAL(-1, hdata_get_var_offset (ptr_hdata, NULL));
    LONGS_EQUAL(-1, hdata_get_var_offset (NULL, "test_char"));
    LONGS_EQUAL(-1, hdata_get_var_offset (ptr_hdata, "zzz"));

    LONGS_EQUAL(offsetof (struct t_test_item, test_char),
                hdata_get_var_offset (ptr_hdata, "test_char"));
    LONGS_EQUAL(offsetof (struct t_test_item, test_int),
                hdata_get_var_offset (ptr_hdata, "test_int"));
    LONGS_EQUAL(offsetof (struct t_test_item, test_string),
                hdata_get_var_offset (ptr_hdata, "test_string"));
}

/*
 * Tests functions:
 *   hdata_get_var_type
 */

TEST(CoreHdataWithList, GetVarType)
{
    LONGS_EQUAL(-1, hdata_get_var_type (NULL, NULL));
    LONGS_EQUAL(-1, hdata_get_var_type (ptr_hdata, NULL));
    LONGS_EQUAL(-1, hdata_get_var_type (NULL, "test_char"));
    LONGS_EQUAL(-1, hdata_get_var_type (ptr_hdata, "zzz"));

    LONGS_EQUAL(WEECHAT_HDATA_CHAR,
                hdata_get_var_type (ptr_hdata, "test_char"));
    LONGS_EQUAL(WEECHAT_HDATA_INTEGER,
                hdata_get_var_type (ptr_hdata, "test_int"));
    LONGS_EQUAL(WEECHAT_HDATA_LONG,
                hdata_get_var_type (ptr_hdata, "test_long"));
    LONGS_EQUAL(WEECHAT_HDATA_STRING,
                hdata_get_var_type (ptr_hdata, "test_string"));
    LONGS_EQUAL(WEECHAT_HDATA_SHARED_STRING,
                hdata_get_var_type (ptr_hdata, "test_shared_string"));
    LONGS_EQUAL(WEECHAT_HDATA_POINTER,
                hdata_get_var_type (ptr_hdata, "test_pointer"));
    LONGS_EQUAL(WEECHAT_HDATA_TIME,
                hdata_get_var_type (ptr_hdata, "test_time"));
    LONGS_EQUAL(WEECHAT_HDATA_HASHTABLE,
                hdata_get_var_type (ptr_hdata, "test_hashtable"));
    LONGS_EQUAL(WEECHAT_HDATA_OTHER,
                hdata_get_var_type (ptr_hdata, "test_other"));
}

/*
 * Tests functions:
 *   hdata_get_var_type_string
 */

TEST(CoreHdataWithList, GetVarTypeString)
{
    POINTERS_EQUAL(NULL, hdata_get_var_type_string (NULL, NULL));
    POINTERS_EQUAL(NULL, hdata_get_var_type_string (ptr_hdata, NULL));
    POINTERS_EQUAL(NULL, hdata_get_var_type_string (NULL, "test_char"));
    POINTERS_EQUAL(NULL, hdata_get_var_type_string (ptr_hdata, "zzz"));

    STRCMP_EQUAL("char",
                 hdata_get_var_type_string (ptr_hdata, "test_char"));
    STRCMP_EQUAL("integer",
                 hdata_get_var_type_string (ptr_hdata, "test_int"));
    STRCMP_EQUAL("long",
                 hdata_get_var_type_string (ptr_hdata, "test_long"));
    STRCMP_EQUAL("string",
                 hdata_get_var_type_string (ptr_hdata, "test_string"));
    STRCMP_EQUAL("shared_string",
                 hdata_get_var_type_string (ptr_hdata, "test_shared_string"));
    STRCMP_EQUAL("pointer",
                 hdata_get_var_type_string (ptr_hdata, "test_pointer"));
    STRCMP_EQUAL("time",
                 hdata_get_var_type_string (ptr_hdata, "test_time"));
    STRCMP_EQUAL("hashtable",
                 hdata_get_var_type_string (ptr_hdata, "test_hashtable"));
    STRCMP_EQUAL("other",
                 hdata_get_var_type_string (ptr_hdata, "test_other"));
}

/*
 * Tests functions:
 *   hdata_get_var_array_size
 */

TEST(CoreHdataWithList, GetVarArraySize)
{
    LONGS_EQUAL(-1, hdata_get_var_array_size (NULL, NULL, NULL));
    LONGS_EQUAL(-1, hdata_get_var_array_size (ptr_hdata, NULL, NULL));
    LONGS_EQUAL(-1, hdata_get_var_array_size (NULL, NULL, "test_char"));
    LONGS_EQUAL(-1, hdata_get_var_array_size (ptr_hdata, NULL, "zzz"));
    LONGS_EQUAL(-1, hdata_get_var_array_size (ptr_hdata, ptr_item1, "zzz"));

    /* not an array */
    LONGS_EQUAL(-1, hdata_get_var_array_size (ptr_hdata, ptr_item1, "test_char"));

    /* item 1 */
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_array_2_char_fixed_size"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_ptr_2_char"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_array_2_int_fixed_size"));
    LONGS_EQUAL(3, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_ptr_3_int"));
    LONGS_EQUAL(1, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_ptr_1_int_fixed_size"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_array_2_long_fixed_size"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_ptr_2_long"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_array_2_words_fixed_size"));
    LONGS_EQUAL(3, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_ptr_words"));
    LONGS_EQUAL(3, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_ptr_words_dyn"));
    LONGS_EQUAL(3, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_ptr_words_dyn_shared"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_array_2_pointer_fixed_size"));
    LONGS_EQUAL(3, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_ptr_3_pointer"));
    LONGS_EQUAL(0, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_ptr_0_pointer_dyn"));
    LONGS_EQUAL(1, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_ptr_1_pointer_dyn"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_array_2_time_fixed_size"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_ptr_2_time"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_array_2_hashtable_fixed_size"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_ptr_2_hashtable"));
    LONGS_EQUAL(1, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_ptr_1_hashtable_dyn"));
    LONGS_EQUAL(3, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                             "test_ptr_3_other"));
    LONGS_EQUAL(-1, hdata_get_var_array_size (ptr_hdata, ptr_item1,
                                              "test_ptr_invalid"));

    /* item 2 */
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_array_2_char_fixed_size"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_ptr_2_char"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_array_2_int_fixed_size"));
    LONGS_EQUAL(3, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_ptr_3_int"));
    LONGS_EQUAL(1, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_ptr_1_int_fixed_size"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_array_2_long_fixed_size"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_ptr_2_long"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_array_2_words_fixed_size"));
    LONGS_EQUAL(4, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_ptr_words"));
    LONGS_EQUAL(4, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_ptr_words_dyn"));
    LONGS_EQUAL(4, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_ptr_words_dyn_shared"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_array_2_pointer_fixed_size"));
    LONGS_EQUAL(3, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_ptr_3_pointer"));
    LONGS_EQUAL(0, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_ptr_0_pointer_dyn"));
    LONGS_EQUAL(1, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_ptr_1_pointer_dyn"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_array_2_time_fixed_size"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_ptr_2_time"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_array_2_hashtable_fixed_size"));
    LONGS_EQUAL(2, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_ptr_2_hashtable"));
    LONGS_EQUAL(1, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_ptr_1_hashtable_dyn"));
    LONGS_EQUAL(3, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                             "test_ptr_3_other"));
    LONGS_EQUAL(-1, hdata_get_var_array_size (ptr_hdata, ptr_item2,
                                              "test_ptr_invalid"));
}

/*
 * Tests functions:
 *   hdata_get_var_array_size_string
 */

TEST(CoreHdataWithList, GetVarArraySizeString)
{
    POINTERS_EQUAL(NULL, hdata_get_var_array_size_string (NULL, NULL,
                                                          NULL));
    POINTERS_EQUAL(NULL, hdata_get_var_array_size_string (ptr_hdata, NULL,
                                                          NULL));
    POINTERS_EQUAL(NULL, hdata_get_var_array_size_string (NULL, NULL,
                                                          "test_char"));
    POINTERS_EQUAL(NULL, hdata_get_var_array_size_string (ptr_hdata, NULL,
                                                          "zzz"));
    POINTERS_EQUAL(NULL, hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                          "zzz"));

    /* not an array */
    POINTERS_EQUAL(NULL, hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                          "test_char"));

    /* item 1 */
    STRCMP_EQUAL("2",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                  "test_array_2_char_fixed_size"));
    STRCMP_EQUAL("test_count_char",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                  "test_ptr_2_char"));
    STRCMP_EQUAL("2",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                  "test_array_2_int_fixed_size"));
    STRCMP_EQUAL("test_count_int",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                  "test_ptr_3_int"));
    STRCMP_EQUAL("1",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                  "test_ptr_1_int_fixed_size"));
    STRCMP_EQUAL("2",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                  "test_array_2_long_fixed_size"));
    STRCMP_EQUAL("test_count_long",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                  "test_ptr_2_long"));
    STRCMP_EQUAL("2",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                  "test_array_2_words_fixed_size"));
    STRCMP_EQUAL("test_count_words",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                  "test_ptr_words"));
    STRCMP_EQUAL("*", hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                       "test_ptr_words_dyn"));
    STRCMP_EQUAL("*", hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                       "test_ptr_words_dyn_shared"));
    STRCMP_EQUAL("2",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                  "test_array_2_pointer_fixed_size"));
    STRCMP_EQUAL("test_count_pointer",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                  "test_ptr_3_pointer"));
    STRCMP_EQUAL("*", hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                       "test_ptr_0_pointer_dyn"));
    STRCMP_EQUAL("*", hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                       "test_ptr_1_pointer_dyn"));
    STRCMP_EQUAL("2",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                  "test_array_2_time_fixed_size"));
    STRCMP_EQUAL("test_count_time",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                  "test_ptr_2_time"));
    STRCMP_EQUAL("2",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                  "test_array_2_hashtable_fixed_size"));
    STRCMP_EQUAL("test_count_hashtable",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                  "test_ptr_2_hashtable"));
    STRCMP_EQUAL("*",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                  "test_ptr_1_hashtable_dyn"));
    STRCMP_EQUAL("test_count_other",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                  "test_ptr_3_other"));
    STRCMP_EQUAL("test_count_invalid",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item1,
                                                  "test_ptr_invalid"));

    /* item 2 */
    STRCMP_EQUAL("2",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                  "test_array_2_char_fixed_size"));
    STRCMP_EQUAL("test_count_char",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                  "test_ptr_2_char"));
    STRCMP_EQUAL("2",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                  "test_array_2_int_fixed_size"));
    STRCMP_EQUAL("test_count_int",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                  "test_ptr_3_int"));
    STRCMP_EQUAL("1",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                  "test_ptr_1_int_fixed_size"));
    STRCMP_EQUAL("2",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                  "test_array_2_long_fixed_size"));
    STRCMP_EQUAL("test_count_long",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                  "test_ptr_2_long"));
    STRCMP_EQUAL("2",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                  "test_array_2_words_fixed_size"));
    STRCMP_EQUAL("test_count_words",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                  "test_ptr_words"));
    STRCMP_EQUAL("*", hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                       "test_ptr_words_dyn"));
    STRCMP_EQUAL("*", hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                       "test_ptr_words_dyn_shared"));
    STRCMP_EQUAL("2",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                  "test_array_2_pointer_fixed_size"));
    STRCMP_EQUAL("test_count_pointer",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                  "test_ptr_3_pointer"));
    STRCMP_EQUAL("*", hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                       "test_ptr_0_pointer_dyn"));
    STRCMP_EQUAL("*", hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                       "test_ptr_1_pointer_dyn"));
    STRCMP_EQUAL("2",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                  "test_array_2_time_fixed_size"));
    STRCMP_EQUAL("test_count_time",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                  "test_ptr_2_time"));
    STRCMP_EQUAL("2",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                  "test_array_2_hashtable_fixed_size"));
    STRCMP_EQUAL("test_count_hashtable",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                  "test_ptr_2_hashtable"));
    STRCMP_EQUAL("*",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                  "test_ptr_1_hashtable_dyn"));
    STRCMP_EQUAL("test_count_other",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                  "test_ptr_3_other"));
    STRCMP_EQUAL("test_count_invalid",
                 hdata_get_var_array_size_string (ptr_hdata, ptr_item2,
                                                  "test_ptr_invalid"));
}

/*
 * Tests functions:
 *   hdata_get_var_hdata
 */

TEST(CoreHdataWithList, GetVarHdata)
{
    POINTERS_EQUAL(NULL, hdata_get_var_hdata (NULL, NULL));
    POINTERS_EQUAL(NULL, hdata_get_var_hdata (ptr_hdata, NULL));
    POINTERS_EQUAL(NULL, hdata_get_var_hdata (NULL, "test_char"));
    POINTERS_EQUAL(NULL, hdata_get_var_hdata (ptr_hdata, "zzz"));

    /* no reference to hdata */
    POINTERS_EQUAL(NULL, hdata_get_var_hdata (ptr_hdata, "test_char"));

    /* check prev/next item variables */
    STRCMP_EQUAL("test_item", hdata_get_var_hdata (ptr_hdata, "prev_item"));
    STRCMP_EQUAL("test_item", hdata_get_var_hdata (ptr_hdata, "next_item"));
}

/*
 * Tests functions:
 *   hdata_get_var
 */

TEST(CoreHdataWithList, GetVar)
{
    POINTERS_EQUAL(NULL, hdata_get_var (NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, hdata_get_var (ptr_hdata, NULL, NULL));
    POINTERS_EQUAL(NULL, hdata_get_var (NULL, ptr_item1, NULL));
    POINTERS_EQUAL(NULL, hdata_get_var (NULL, NULL, "test_char"));
    POINTERS_EQUAL(NULL, hdata_get_var (ptr_hdata, ptr_item1, NULL));
    POINTERS_EQUAL(NULL, hdata_get_var (NULL, ptr_item1, "test_char"));
    POINTERS_EQUAL(NULL, hdata_get_var (ptr_hdata, NULL, "test_char"));
    POINTERS_EQUAL(NULL, hdata_get_var (ptr_hdata, ptr_item1, "zzz"));

    /* item 1 */
    POINTERS_EQUAL(&(ptr_item1->test_char),
                   hdata_get_var (ptr_hdata, ptr_item1, "test_char"));
    POINTERS_EQUAL(&(ptr_item1->test_int),
                   hdata_get_var (ptr_hdata, ptr_item1, "test_int"));
    POINTERS_EQUAL(&(ptr_item1->test_string),
                   hdata_get_var (ptr_hdata, ptr_item1, "test_string"));

    /* item 2 */
    POINTERS_EQUAL(&(ptr_item2->test_char),
                   hdata_get_var (ptr_hdata, ptr_item2, "test_char"));
    POINTERS_EQUAL(&(ptr_item2->test_int),
                   hdata_get_var (ptr_hdata, ptr_item2, "test_int"));
    POINTERS_EQUAL(&(ptr_item2->test_string),
                   hdata_get_var (ptr_hdata, ptr_item2, "test_string"));
}

/*
 * Tests functions:
 *   hdata_get_var_at_offset
 */

TEST(CoreHdataWithList, GetVarAtOffset)
{
    POINTERS_EQUAL(NULL, hdata_get_var_at_offset (NULL, NULL, 0));
    POINTERS_EQUAL(NULL, hdata_get_var_at_offset (ptr_hdata, NULL, 0));
    POINTERS_EQUAL(NULL, hdata_get_var_at_offset (NULL, ptr_item1, 0));

    /* item 1 */
    POINTERS_EQUAL(&(ptr_item1->test_char),
                   hdata_get_var_at_offset (
                       ptr_hdata,
                       ptr_item1,
                       offsetof (struct t_test_item, test_char)));
    POINTERS_EQUAL(&(ptr_item1->test_int),
                   hdata_get_var_at_offset (
                       ptr_hdata,
                       ptr_item1,
                       offsetof (struct t_test_item, test_int)));
    POINTERS_EQUAL(&(ptr_item1->test_string),
                   hdata_get_var_at_offset (
                       ptr_hdata,
                       ptr_item1,
                       offsetof (struct t_test_item, test_string)));

    /* item 2 */
    POINTERS_EQUAL(&(ptr_item2->test_char),
                   hdata_get_var_at_offset (
                       ptr_hdata,
                       ptr_item2,
                       offsetof (struct t_test_item, test_char)));
    POINTERS_EQUAL(&(ptr_item2->test_int),
                   hdata_get_var_at_offset (
                       ptr_hdata,
                       ptr_item2,
                       offsetof (struct t_test_item, test_int)));
    POINTERS_EQUAL(&(ptr_item2->test_string),
                   hdata_get_var_at_offset (
                       ptr_hdata,
                       ptr_item2,
                       offsetof (struct t_test_item, test_string)));
}

/*
 * Tests functions:
 *   hdata_get_list
 */

TEST(CoreHdataWithList, GetList)
{
    POINTERS_EQUAL(NULL, hdata_get_list (NULL, NULL));
    POINTERS_EQUAL(NULL, hdata_get_list (ptr_hdata, NULL));
    POINTERS_EQUAL(NULL, hdata_get_list (NULL, "items"));
    POINTERS_EQUAL(NULL, hdata_get_list (ptr_hdata, "zzz"));

    POINTERS_EQUAL(ptr_item1, hdata_get_list (ptr_hdata, "items"));
    POINTERS_EQUAL(ptr_item2, hdata_get_list (ptr_hdata, "last_item"));
}

/*
 * Tests functions:
 *   hdata_check_pointer
 */

TEST(CoreHdataWithList, Check)
{
    LONGS_EQUAL(0, hdata_check_pointer (NULL, NULL, NULL));
    LONGS_EQUAL(0, hdata_check_pointer (ptr_hdata, NULL, NULL));
    LONGS_EQUAL(0, hdata_check_pointer (NULL, NULL, ptr_item1));

    /* pointer not found */
    LONGS_EQUAL(0, hdata_check_pointer (ptr_hdata, NULL, (void *)0x1));
    LONGS_EQUAL(0, hdata_check_pointer (ptr_hdata, items, (void *)0x1));

    /* valid pointer */
    LONGS_EQUAL(1, hdata_check_pointer (ptr_hdata, NULL, ptr_item1));
    LONGS_EQUAL(1, hdata_check_pointer (ptr_hdata, NULL, ptr_item2));
    LONGS_EQUAL(1, hdata_check_pointer (ptr_hdata, items, ptr_item1));
    LONGS_EQUAL(1, hdata_check_pointer (ptr_hdata, items, ptr_item2));
}

/*
 * Tests functions:
 *   hdata_move
 */

TEST(CoreHdataWithList, Move)
{
    POINTERS_EQUAL(NULL, hdata_move (NULL, NULL, 0));
    POINTERS_EQUAL(NULL, hdata_move (ptr_hdata, NULL, 0));
    POINTERS_EQUAL(NULL, hdata_move (NULL, ptr_item1, 0));
    POINTERS_EQUAL(NULL, hdata_move (NULL, NULL, 1));
    POINTERS_EQUAL(NULL, hdata_move (ptr_hdata, ptr_item1, 0));
    POINTERS_EQUAL(NULL, hdata_move (ptr_hdata, NULL, 1));
    POINTERS_EQUAL(NULL, hdata_move (NULL, ptr_item1, 1));

    /* move from item1 */
    POINTERS_EQUAL(NULL, hdata_move (ptr_hdata, ptr_item1, -1));
    POINTERS_EQUAL(NULL, hdata_move (ptr_hdata, ptr_item1, -42));
    POINTERS_EQUAL(ptr_item2, hdata_move (ptr_hdata, ptr_item1, 1));
    POINTERS_EQUAL(NULL, hdata_move (ptr_hdata, ptr_item1, 42));

    /* move from item2 */
    POINTERS_EQUAL(ptr_item1, hdata_move (ptr_hdata, ptr_item2, -1));
    POINTERS_EQUAL(NULL, hdata_move (ptr_hdata, ptr_item2, -42));
    POINTERS_EQUAL(NULL, hdata_move (ptr_hdata, ptr_item2, 1));
    POINTERS_EQUAL(NULL, hdata_move (ptr_hdata, ptr_item2, 42));
}

/*
 * Tests functions:
 *   hdata_search
 */

TEST(CoreHdataWithList, Search)
{
    struct t_hashtable *pointers, *extra_vars;

    pointers = hashtable_new (8,
                              WEECHAT_HASHTABLE_STRING,
                              WEECHAT_HASHTABLE_POINTER,
                              NULL,
                              NULL);

    extra_vars = hashtable_new (8,
                                WEECHAT_HASHTABLE_STRING,
                                WEECHAT_HASHTABLE_STRING,
                                NULL,
                                NULL);

    POINTERS_EQUAL(NULL, hdata_search (NULL, NULL, NULL,
                                       NULL, NULL, NULL, 0));
    POINTERS_EQUAL(NULL, hdata_search (ptr_hdata, NULL, NULL,
                                       NULL, NULL, NULL, 0));
    POINTERS_EQUAL(NULL, hdata_search (NULL, items, NULL,
                                       NULL, NULL, NULL, 0));
    POINTERS_EQUAL(NULL, hdata_search (NULL, NULL, "${test_char} == A",
                                       NULL, NULL, NULL, 0));
    POINTERS_EQUAL(NULL, hdata_search (NULL, NULL, NULL,
                                       NULL, NULL, NULL, 1));
    POINTERS_EQUAL(NULL, hdata_search (ptr_hdata, items, NULL,
                                       NULL, NULL, NULL, 0));
    POINTERS_EQUAL(NULL, hdata_search (ptr_hdata, NULL, "${test_char} == A",
                                       NULL, NULL, NULL, 0));
    POINTERS_EQUAL(NULL, hdata_search (ptr_hdata, NULL, NULL, NULL, NULL, NULL, 1));
    POINTERS_EQUAL(NULL, hdata_search (NULL, items, "${test_char} == A",
                                       NULL, NULL, NULL, 0));
    POINTERS_EQUAL(NULL, hdata_search (NULL, items, NULL, NULL, NULL, NULL, 1));
    POINTERS_EQUAL(NULL, hdata_search (NULL, NULL, "${test_char} == A",
                                       NULL, NULL, NULL, 1));
    POINTERS_EQUAL(NULL, hdata_search (ptr_hdata, items, "${test_char} == A",
                                       NULL, NULL, NULL, 0));
    POINTERS_EQUAL(NULL, hdata_search (ptr_hdata, items, NULL,
                                       NULL, NULL, NULL, 1));
    POINTERS_EQUAL(NULL, hdata_search (ptr_hdata, NULL, "${test_char} == A",
                                       NULL, NULL, NULL, 1));
    POINTERS_EQUAL(NULL, hdata_search (NULL, items, "${test_char} == A",
                                       NULL, NULL, NULL, 1));

    /* search char */
    POINTERS_EQUAL(
        NULL,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_char} == Z",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        NULL,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_char} == X",
                      NULL, NULL, NULL, 2));
    POINTERS_EQUAL(
        ptr_item1,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_char} == A",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        ptr_item2,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_char} == a",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        ptr_item1,
        hdata_search (ptr_hdata, last_item,
                      "${test_item.test_char} == A",
                      NULL, NULL, NULL, -1));
    hashtable_set (extra_vars, "value", "a");
    POINTERS_EQUAL(
        ptr_item2,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_char} == ${value}",
                      NULL, extra_vars, NULL, 1));

    /* search integer */
    POINTERS_EQUAL(
        NULL,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_int} == 999",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        NULL,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_int} == 456",
                      NULL, NULL, NULL, 2));
    POINTERS_EQUAL(
        ptr_item1,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_int} == 123",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        ptr_item2,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_int} == 456",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        ptr_item1,
        hdata_search (ptr_hdata, last_item,
                      "${test_item.test_int} == 123",
                      NULL, NULL, NULL, -1));
    hashtable_set (extra_vars, "value", "456");
    POINTERS_EQUAL(
        ptr_item2,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_int} == ${value}",
                      NULL, extra_vars, NULL, 1));

    /* search long */
    POINTERS_EQUAL(
        NULL,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_long} == 999",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        NULL,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_long} == 987654321",
                      NULL, NULL, NULL, 2));
    POINTERS_EQUAL(
        ptr_item1,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_long} == 123456789",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        ptr_item2,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_long} == 987654321",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        ptr_item1,
        hdata_search (ptr_hdata, last_item,
                      "${test_item.test_long} == 123456789",
                      NULL, NULL, NULL, -1));
    hashtable_set (extra_vars, "value", "987654321");
    POINTERS_EQUAL(
        ptr_item2,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_long} == ${value}",
                      NULL, extra_vars, NULL, 1));

    /* search string */
    POINTERS_EQUAL(
        NULL,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_string} == zzz",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        NULL,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_string} == item2",
                      NULL, NULL, NULL, 2));
    POINTERS_EQUAL(
        ptr_item1,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_string} == item1",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        ptr_item2,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_string} == item2",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        ptr_item1,
        hdata_search (ptr_hdata, last_item,
                      "${test_item.test_string} == item1",
                      NULL, NULL, NULL, -1));
    hashtable_set (extra_vars, "value", "item2");
    POINTERS_EQUAL(
        ptr_item2,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_string} == item2",
                      NULL, extra_vars, NULL, 1));

    /* search shared string */
    POINTERS_EQUAL(
        NULL,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_shared_string} == zzz",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        NULL,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_shared_string} == item2_shared",
                      NULL, NULL, NULL, 2));
    POINTERS_EQUAL(
        ptr_item1,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_shared_string} == item1_shared",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        ptr_item2,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_shared_string} == item2_shared",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        ptr_item1,
        hdata_search (ptr_hdata, last_item,
                      "${test_item.test_shared_string} == item1_shared",
                      NULL, NULL, NULL, -1));
    hashtable_set (extra_vars, "value", "item2_shared");
    POINTERS_EQUAL(
        ptr_item2,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_shared_string} == item2_shared",
                      NULL, NULL, NULL, 1));

    /* search pointer */
    POINTERS_EQUAL(
        NULL,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_pointer} == 0x999",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        NULL,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_pointer} == 0x456",
                      NULL, NULL, NULL, 2));
    POINTERS_EQUAL(
        ptr_item1,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_pointer} == 0x123",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        ptr_item2,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_pointer} == 0x456",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        ptr_item1,
        hdata_search (ptr_hdata, last_item,
                      "${test_item.test_pointer} == 0x123",
                      NULL, NULL, NULL, -1));
    hashtable_set (extra_vars, "value", "0x456");
    POINTERS_EQUAL(
        ptr_item2,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_pointer} == ${value}",
                      NULL, extra_vars, NULL, 1));
    hashtable_set (pointers, "value", (void *)0x456);
    POINTERS_EQUAL(
        ptr_item2,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_pointer} == ${value}",
                      pointers, NULL, NULL, 1));

    /* search time */
    POINTERS_EQUAL(
        NULL,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_time} == 999",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        NULL,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_time} == 789123",
                      NULL, NULL, NULL, 2));
    POINTERS_EQUAL(
        ptr_item1,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_time} == 123456",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        ptr_item2,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_time} == 789123",
                      NULL, NULL, NULL, 1));
    POINTERS_EQUAL(
        ptr_item1,
        hdata_search (ptr_hdata, last_item,
                      "${test_item.test_time} == 123456",
                      NULL, NULL, NULL, -1));
    hashtable_set (extra_vars, "value", "789123");
    POINTERS_EQUAL(
        ptr_item2,
        hdata_search (ptr_hdata, items,
                      "${test_item.test_time} == ${value}",
                      NULL, extra_vars, NULL, 1));

    hashtable_free (pointers);
    hashtable_free (extra_vars);
}

/*
 * Tests functions:
 *   hdata_get_index_and_name
 */

TEST(CoreHdataWithList, GetIndexAndName)
{
    int index;
    const char *ptr_name;

    hdata_get_index_and_name (NULL, NULL, NULL);
    hdata_get_index_and_name ("test", NULL, NULL);
    hdata_get_index_and_name ("123|test", NULL, NULL);

    index = -999;
    ptr_name = (const char *)0x1;
    hdata_get_index_and_name (NULL, &index, &ptr_name);
    LONGS_EQUAL(-1, index);
    POINTERS_EQUAL(NULL, ptr_name);

    index = -999;
    ptr_name = (const char *)0x1;
    hdata_get_index_and_name ("test", &index, &ptr_name);
    LONGS_EQUAL(-1, index);
    STRCMP_EQUAL("test", ptr_name);

    index = -999;
    ptr_name = (const char *)0x1;
    hdata_get_index_and_name ("abc|test", &index, &ptr_name);
    LONGS_EQUAL(-1, index);
    STRCMP_EQUAL("abc|test", ptr_name);

    index = -999;
    ptr_name = (const char *)0x1;
    hdata_get_index_and_name ("123|test", &index, &ptr_name);
    LONGS_EQUAL(123, index);
    STRCMP_EQUAL("test", ptr_name);

    index = -999;
    ptr_name = (const char *)0x1;
    hdata_get_index_and_name ("123|", &index, &ptr_name);
    LONGS_EQUAL(123, index);
    STRCMP_EQUAL("", ptr_name);
}

/*
 * Tests functions:
 *   hdata_char
 */

TEST(CoreHdataWithList, Char)
{
    LONGS_EQUAL('\0', hdata_char (NULL, NULL, NULL));
    LONGS_EQUAL('\0', hdata_char (ptr_hdata, NULL, NULL));
    LONGS_EQUAL('\0', hdata_char (NULL, ptr_item1, NULL));
    LONGS_EQUAL('\0', hdata_char (NULL, NULL, "test_char"));
    LONGS_EQUAL('\0', hdata_char (ptr_hdata, ptr_item1, NULL));
    LONGS_EQUAL('\0', hdata_char (ptr_hdata, NULL, "test_char"));
    LONGS_EQUAL('\0', hdata_char (NULL, ptr_item1, "test_char"));

    /* variable not found */
    LONGS_EQUAL('\0', hdata_char (ptr_hdata, ptr_item1, "zzz"));
    LONGS_EQUAL('\0', hdata_char (ptr_hdata, ptr_item1, "1|zzz"));

    /* item 1 */
    LONGS_EQUAL('A', hdata_char (ptr_hdata, ptr_item1, "test_char"));
    LONGS_EQUAL('A', hdata_char (ptr_hdata, ptr_item1,
                                 "0|test_array_2_char_fixed_size"));
    LONGS_EQUAL('B', hdata_char (ptr_hdata, ptr_item1,
                                 "1|test_array_2_char_fixed_size"));
    LONGS_EQUAL('B', hdata_char (ptr_hdata, ptr_item1, "0|test_ptr_2_char"));
    LONGS_EQUAL('C', hdata_char (ptr_hdata, ptr_item1, "1|test_ptr_2_char"));

    /* item 2 */
    LONGS_EQUAL('a', hdata_char (ptr_hdata, ptr_item2, "test_char"));
    LONGS_EQUAL('b', hdata_char (ptr_hdata, ptr_item2, "0|test_ptr_2_char"));
    LONGS_EQUAL('c', hdata_char (ptr_hdata, ptr_item2, "1|test_ptr_2_char"));
}

/*
 * Tests functions:
 *   hdata_integer
 */

TEST(CoreHdataWithList, Integer)
{
    LONGS_EQUAL(0, hdata_integer (NULL, NULL, NULL));
    LONGS_EQUAL(0, hdata_integer (ptr_hdata, NULL, NULL));
    LONGS_EQUAL(0, hdata_integer (NULL, ptr_item1, NULL));
    LONGS_EQUAL(0, hdata_integer (NULL, NULL, "test_int"));
    LONGS_EQUAL(0, hdata_integer (ptr_hdata, ptr_item1, NULL));
    LONGS_EQUAL(0, hdata_integer (ptr_hdata, NULL, "test_int"));
    LONGS_EQUAL(0, hdata_integer (NULL, ptr_item1, "test_int"));

    /* variable not found */
    LONGS_EQUAL(0, hdata_integer (ptr_hdata, ptr_item1, "zzz"));
    LONGS_EQUAL(0, hdata_integer (ptr_hdata, ptr_item1, "1|zzz"));

    /* item 1 */
    LONGS_EQUAL(123, hdata_integer (ptr_hdata, ptr_item1, "test_int"));
    LONGS_EQUAL(111, hdata_integer (ptr_hdata, ptr_item1,
                                    "0|test_array_2_int_fixed_size"));
    LONGS_EQUAL(222, hdata_integer (ptr_hdata, ptr_item1,
                                    "1|test_array_2_int_fixed_size"));
    LONGS_EQUAL(1, hdata_integer (ptr_hdata, ptr_item1, "0|test_ptr_3_int"));
    LONGS_EQUAL(2, hdata_integer (ptr_hdata, ptr_item1, "1|test_ptr_3_int"));
    LONGS_EQUAL(3, hdata_integer (ptr_hdata, ptr_item1, "2|test_ptr_3_int"));
    LONGS_EQUAL(111, hdata_integer (ptr_hdata, ptr_item1,
                                    "0|test_ptr_1_int_fixed_size"));

    /* item 2 */
    LONGS_EQUAL(456, hdata_integer (ptr_hdata, ptr_item2, "test_int"));
    LONGS_EQUAL(444, hdata_integer (ptr_hdata, ptr_item2,
                                    "0|test_array_2_int_fixed_size"));
    LONGS_EQUAL(555, hdata_integer (ptr_hdata, ptr_item2,
                                    "1|test_array_2_int_fixed_size"));
    LONGS_EQUAL(4, hdata_integer (ptr_hdata, ptr_item2, "0|test_ptr_3_int"));
    LONGS_EQUAL(5, hdata_integer (ptr_hdata, ptr_item2, "1|test_ptr_3_int"));
    LONGS_EQUAL(6, hdata_integer (ptr_hdata, ptr_item2, "2|test_ptr_3_int"));
    LONGS_EQUAL(222, hdata_integer (ptr_hdata, ptr_item2,
                                    "0|test_ptr_1_int_fixed_size"));
}

/*
 * Tests functions:
 *   hdata_long
 */

TEST(CoreHdataWithList, Long)
{
    LONGS_EQUAL(0, hdata_long (NULL, NULL, NULL));
    LONGS_EQUAL(0, hdata_long (ptr_hdata, NULL, NULL));
    LONGS_EQUAL(0, hdata_long (NULL, ptr_item1, NULL));
    LONGS_EQUAL(0, hdata_long (NULL, NULL, "test_long"));
    LONGS_EQUAL(0, hdata_long (ptr_hdata, ptr_item1, NULL));
    LONGS_EQUAL(0, hdata_long (ptr_hdata, NULL, "test_long"));
    LONGS_EQUAL(0, hdata_long (NULL, ptr_item1, "test_long"));

    /* variable not found */
    LONGS_EQUAL(0, hdata_long (ptr_hdata, ptr_item1, "zzz"));
    LONGS_EQUAL(0, hdata_long (ptr_hdata, ptr_item1, "1|zzz"));

    /* item 1 */
    LONGS_EQUAL(123456789L, hdata_long (ptr_hdata, ptr_item1, "test_long"));
    LONGS_EQUAL(111L, hdata_long (ptr_hdata, ptr_item1,
                                  "0|test_array_2_long_fixed_size"));
    LONGS_EQUAL(222L, hdata_long (ptr_hdata, ptr_item1,
                                  "1|test_array_2_long_fixed_size"));
    LONGS_EQUAL(123456L, hdata_long (ptr_hdata, ptr_item1,
                                     "0|test_ptr_2_long"));
    LONGS_EQUAL(234567L, hdata_long (ptr_hdata, ptr_item1,
                                     "1|test_ptr_2_long"));

    /* item 2 */
    LONGS_EQUAL(987654321L, hdata_long (ptr_hdata, ptr_item2, "test_long"));
    LONGS_EQUAL(333L, hdata_long (ptr_hdata, ptr_item2,
                                  "0|test_array_2_long_fixed_size"));
    LONGS_EQUAL(444L, hdata_long (ptr_hdata, ptr_item2,
                                  "1|test_array_2_long_fixed_size"));
    LONGS_EQUAL(789123L, hdata_long (ptr_hdata, ptr_item2,
                                     "0|test_ptr_2_long"));
    LONGS_EQUAL(891234L, hdata_long (ptr_hdata, ptr_item2,
                                     "1|test_ptr_2_long"));
}

/*
 * Tests functions:
 *   hdata_string
 */

TEST(CoreHdataWithList, String)
{
    POINTERS_EQUAL(NULL, hdata_string (NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, hdata_string (ptr_hdata, NULL, NULL));
    POINTERS_EQUAL(NULL, hdata_string (NULL, ptr_item1, NULL));
    POINTERS_EQUAL(NULL, hdata_string (NULL, NULL, "test_string"));
    POINTERS_EQUAL(NULL, hdata_string (ptr_hdata, ptr_item1, NULL));
    POINTERS_EQUAL(NULL, hdata_string (ptr_hdata, NULL, "test_string"));
    POINTERS_EQUAL(NULL, hdata_string (NULL, ptr_item1, "test_string"));

    /* variable not found */
    POINTERS_EQUAL(NULL, hdata_string (ptr_hdata, ptr_item1, "zzz"));
    POINTERS_EQUAL(NULL, hdata_string (ptr_hdata, ptr_item1, "1|zzz"));

    /* item 1 */
    STRCMP_EQUAL("item1", hdata_string (ptr_hdata, ptr_item1, "test_string"));
    STRCMP_EQUAL("item1_shared", hdata_string (ptr_hdata, ptr_item1,
                                               "test_shared_string"));
    POINTERS_EQUAL(
        NULL,
        hdata_string (ptr_hdata, ptr_item1, "0|test_array_2_words_fixed_size"));
    POINTERS_EQUAL(
        NULL,
        hdata_string (ptr_hdata, ptr_item1, "1|test_array_2_words_fixed_size"));
    STRCMP_EQUAL(
        "a",
        hdata_string (ptr_hdata, ptr_item1, "0|test_ptr_words"));
    STRCMP_EQUAL(
        "b",
        hdata_string (ptr_hdata, ptr_item1, "1|test_ptr_words"));
    STRCMP_EQUAL(
        "c",
        hdata_string (ptr_hdata, ptr_item1, "2|test_ptr_words"));
    POINTERS_EQUAL(
        NULL,
        hdata_string (ptr_hdata, ptr_item1, "3|test_ptr_words"));
    STRCMP_EQUAL(
        "aa",
        hdata_string (ptr_hdata, ptr_item1, "0|test_ptr_words_dyn"));
    STRCMP_EQUAL(
        "bb",
        hdata_string (ptr_hdata, ptr_item1, "1|test_ptr_words_dyn"));
    STRCMP_EQUAL(
        "cc",
        hdata_string (ptr_hdata, ptr_item1, "2|test_ptr_words_dyn"));
    POINTERS_EQUAL(
        NULL,
        hdata_string (ptr_hdata, ptr_item1, "3|test_ptr_words_dyn"));
    STRCMP_EQUAL(
        "aaa",
        hdata_string (ptr_hdata, ptr_item1, "0|test_ptr_words_dyn_shared"));
    STRCMP_EQUAL(
        "bbb",
        hdata_string (ptr_hdata, ptr_item1, "1|test_ptr_words_dyn_shared"));
    STRCMP_EQUAL(
        "ccc",
        hdata_string (ptr_hdata, ptr_item1, "2|test_ptr_words_dyn_shared"));
    POINTERS_EQUAL(
        NULL,
        hdata_string (ptr_hdata, ptr_item1, "3|test_ptr_words_dyn_shared"));

    /* item 2 */
    STRCMP_EQUAL("item2", hdata_string (ptr_hdata, ptr_item2, "test_string"));
    STRCMP_EQUAL("item2_shared", hdata_string (ptr_hdata, ptr_item2,
                                               "test_shared_string"));
    POINTERS_EQUAL(
        NULL,
        hdata_string (ptr_hdata, ptr_item2, "0|test_array_2_words_fixed_size"));
    POINTERS_EQUAL(
        NULL,
        hdata_string (ptr_hdata, ptr_item2, "1|test_array_2_words_fixed_size"));
    STRCMP_EQUAL(
        "e",
        hdata_string (ptr_hdata, ptr_item2, "0|test_ptr_words"));
    STRCMP_EQUAL(
        "f",
        hdata_string (ptr_hdata, ptr_item2, "1|test_ptr_words"));
    STRCMP_EQUAL(
        "g",
        hdata_string (ptr_hdata, ptr_item2, "2|test_ptr_words"));
    STRCMP_EQUAL(
        "h",
        hdata_string (ptr_hdata, ptr_item2, "3|test_ptr_words"));
    POINTERS_EQUAL(
        NULL,
        hdata_string (ptr_hdata, ptr_item2, "4|test_ptr_words"));
    STRCMP_EQUAL(
        "ee",
        hdata_string (ptr_hdata, ptr_item2, "0|test_ptr_words_dyn"));
    STRCMP_EQUAL(
        "ff",
        hdata_string (ptr_hdata, ptr_item2, "1|test_ptr_words_dyn"));
    STRCMP_EQUAL(
        "gg",
        hdata_string (ptr_hdata, ptr_item2, "2|test_ptr_words_dyn"));
    STRCMP_EQUAL(
        "hh",
        hdata_string (ptr_hdata, ptr_item2, "3|test_ptr_words_dyn"));
    POINTERS_EQUAL(
        NULL,
        hdata_string (ptr_hdata, ptr_item2, "4|test_ptr_words_dyn"));
    STRCMP_EQUAL(
        "eee",
        hdata_string (ptr_hdata, ptr_item2, "0|test_ptr_words_dyn_shared"));
    STRCMP_EQUAL(
        "fff",
        hdata_string (ptr_hdata, ptr_item2, "1|test_ptr_words_dyn_shared"));
    STRCMP_EQUAL(
        "ggg",
        hdata_string (ptr_hdata, ptr_item2, "2|test_ptr_words_dyn_shared"));
    STRCMP_EQUAL(
        "hhh",
        hdata_string (ptr_hdata, ptr_item2, "3|test_ptr_words_dyn_shared"));
    POINTERS_EQUAL(
        NULL,
        hdata_string (ptr_hdata, ptr_item2, "4|test_ptr_words_dyn_shared"));
}

/*
 * Tests functions:
 *   hdata_pointer
 */

TEST(CoreHdataWithList, Pointer)
{
    POINTERS_EQUAL(NULL, hdata_pointer (NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, hdata_pointer (ptr_hdata, NULL, NULL));
    POINTERS_EQUAL(NULL, hdata_pointer (NULL, ptr_item1, NULL));
    POINTERS_EQUAL(NULL, hdata_pointer (NULL, NULL, "test_pointer"));
    POINTERS_EQUAL(NULL, hdata_pointer (ptr_hdata, ptr_item1, NULL));
    POINTERS_EQUAL(NULL, hdata_pointer (ptr_hdata, NULL, "test_pointer"));
    POINTERS_EQUAL(NULL, hdata_pointer (NULL, ptr_item1, "test_pointer"));

    /* variable not found */
    POINTERS_EQUAL(NULL, hdata_pointer (ptr_hdata, ptr_item1, "zzz"));
    POINTERS_EQUAL(NULL, hdata_pointer (ptr_hdata, ptr_item1, "1|zzz"));

    /* item 1 */
    LONGS_EQUAL(0x123, hdata_pointer (ptr_hdata, ptr_item1, "test_pointer"));
    LONGS_EQUAL(
        0x112233,
        hdata_pointer (ptr_hdata, ptr_item1, "0|test_array_2_pointer_fixed_size"));
    LONGS_EQUAL(
        0x445566,
        hdata_pointer (ptr_hdata, ptr_item1, "1|test_array_2_pointer_fixed_size"));
    LONGS_EQUAL(
        0x123,
        hdata_pointer (ptr_hdata, ptr_item1, "0|test_ptr_3_pointer"));
    LONGS_EQUAL(
        0x456,
        hdata_pointer (ptr_hdata, ptr_item1, "1|test_ptr_3_pointer"));
    LONGS_EQUAL(
        0x789,
        hdata_pointer (ptr_hdata, ptr_item1, "2|test_ptr_3_pointer"));

    /* item 2 */
    LONGS_EQUAL(0x456, hdata_pointer (ptr_hdata, ptr_item2, "test_pointer"));
    LONGS_EQUAL(
        0x778899,
        hdata_pointer (ptr_hdata, ptr_item2, "0|test_array_2_pointer_fixed_size"));
    LONGS_EQUAL(
        0xaabbcc,
        hdata_pointer (ptr_hdata, ptr_item2, "1|test_array_2_pointer_fixed_size"));
    LONGS_EQUAL(
        0x123abc,
        hdata_pointer (ptr_hdata, ptr_item2, "0|test_ptr_3_pointer"));
    LONGS_EQUAL(
        0x456def,
        hdata_pointer (ptr_hdata, ptr_item2, "1|test_ptr_3_pointer"));
    LONGS_EQUAL(
        0x789abc,
        hdata_pointer (ptr_hdata, ptr_item2, "2|test_ptr_3_pointer"));
}

/*
 * Tests functions:
 *   hdata_time
 */

TEST(CoreHdataWithList, Time)
{
    LONGS_EQUAL(0, hdata_time (NULL, NULL, NULL));
    LONGS_EQUAL(0, hdata_time (ptr_hdata, NULL, NULL));
    LONGS_EQUAL(0, hdata_time (NULL, ptr_item1, NULL));
    LONGS_EQUAL(0, hdata_time (NULL, NULL, "test_time"));
    LONGS_EQUAL(0, hdata_time (ptr_hdata, ptr_item1, NULL));
    LONGS_EQUAL(0, hdata_time (ptr_hdata, NULL, "test_time"));
    LONGS_EQUAL(0, hdata_time (NULL, ptr_item1, "test_time"));

    /* variable not found */
    LONGS_EQUAL(0, hdata_time (ptr_hdata, ptr_item1, "zzz"));
    LONGS_EQUAL(0, hdata_time (ptr_hdata, ptr_item1, "1|zzz"));

    /* item 1 */
    LONGS_EQUAL(123456, hdata_time (ptr_hdata, ptr_item1, "test_time"));
    LONGS_EQUAL(112,
                hdata_time (ptr_hdata, ptr_item1, "0|test_array_2_time_fixed_size"));
    LONGS_EQUAL(334,
                hdata_time (ptr_hdata, ptr_item1, "1|test_array_2_time_fixed_size"));
    LONGS_EQUAL(1234,
                hdata_time (ptr_hdata, ptr_item1, "0|test_ptr_2_time"));
    LONGS_EQUAL(5678,
                hdata_time (ptr_hdata, ptr_item1, "1|test_ptr_2_time"));

    /* item 2 */
    LONGS_EQUAL(789123, hdata_time (ptr_hdata, ptr_item2, "test_time"));
    LONGS_EQUAL(556,
                hdata_time (ptr_hdata, ptr_item2, "0|test_array_2_time_fixed_size"));
    LONGS_EQUAL(778,
                hdata_time (ptr_hdata, ptr_item2, "1|test_array_2_time_fixed_size"));
    LONGS_EQUAL(123456,
                hdata_time (ptr_hdata, ptr_item2, "0|test_ptr_2_time"));
    LONGS_EQUAL(789123,
                hdata_time (ptr_hdata, ptr_item2, "1|test_ptr_2_time"));
}

/*
 * Tests functions:
 *   hdata_hashtable
 */

TEST(CoreHdataWithList, Hashtable)
{
    struct t_hashtable *ptr_hashtable;

    POINTERS_EQUAL(NULL, hdata_hashtable (NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, hdata_hashtable (ptr_hdata, NULL, NULL));
    POINTERS_EQUAL(NULL, hdata_hashtable (NULL, ptr_item1, NULL));
    POINTERS_EQUAL(NULL, hdata_hashtable (NULL, NULL, "test_hashtable"));
    POINTERS_EQUAL(NULL, hdata_hashtable (ptr_hdata, ptr_item1, NULL));
    POINTERS_EQUAL(NULL, hdata_hashtable (ptr_hdata, NULL, "test_hashtable"));
    POINTERS_EQUAL(NULL, hdata_hashtable (NULL, ptr_item1, "test_hashtable"));

    /* variable not found */
    POINTERS_EQUAL(NULL, hdata_hashtable (ptr_hdata, ptr_item1, "zzz"));
    POINTERS_EQUAL(NULL, hdata_hashtable (ptr_hdata, ptr_item1, "1|zzz"));

    /* item 1 */
    ptr_hashtable = hdata_hashtable (ptr_hdata, ptr_item1, "test_hashtable");
    CHECK(ptr_hashtable);
    STRCMP_EQUAL("value1", (const char *)hashtable_get (ptr_hashtable, "key1"));
    ptr_hashtable = hdata_hashtable (ptr_hdata, ptr_item1,
                                     "0|test_array_2_hashtable_fixed_size");
    CHECK(ptr_hashtable);
    STRCMP_EQUAL("value_array_1.1",
                 (const char *)hashtable_get (ptr_hashtable, "key_array_1.1"));
    ptr_hashtable = hdata_hashtable (ptr_hdata, ptr_item1,
                                     "1|test_array_2_hashtable_fixed_size");
    CHECK(ptr_hashtable);
    STRCMP_EQUAL("value_array_1.2",
                 (const char *)hashtable_get (ptr_hashtable, "key_array_1.2"));
    ptr_hashtable = hdata_hashtable (ptr_hdata, ptr_item1,
                                     "0|test_ptr_2_hashtable");
    CHECK(ptr_hashtable);
    STRCMP_EQUAL("value1.1",
                 (const char *)hashtable_get (ptr_hashtable, "key1.1"));
    ptr_hashtable = hdata_hashtable (ptr_hdata, ptr_item1,
                                     "1|test_ptr_2_hashtable");
    CHECK(ptr_hashtable);
    STRCMP_EQUAL("value1.2",
                 (const char *)hashtable_get (ptr_hashtable, "key1.2"));

    /* item 2 */
    ptr_hashtable = hdata_hashtable (ptr_hdata, ptr_item2, "test_hashtable");
    CHECK(ptr_hashtable);
    STRCMP_EQUAL("value2", (const char *)hashtable_get (ptr_hashtable, "key2"));
    ptr_hashtable = hdata_hashtable (ptr_hdata, ptr_item2,
                                     "0|test_array_2_hashtable_fixed_size");
    CHECK(ptr_hashtable);
    STRCMP_EQUAL("value_array_2.1",
                 (const char *)hashtable_get (ptr_hashtable, "key_array_2.1"));
    ptr_hashtable = hdata_hashtable (ptr_hdata, ptr_item2,
                                     "1|test_array_2_hashtable_fixed_size");
    CHECK(ptr_hashtable);
    STRCMP_EQUAL("value_array_2.2",
                 (const char *)hashtable_get (ptr_hashtable, "key_array_2.2"));
    ptr_hashtable = hdata_hashtable (ptr_hdata, ptr_item2,
                                     "0|test_ptr_2_hashtable");
    CHECK(ptr_hashtable);
    STRCMP_EQUAL("value2.1",
                 (const char *)hashtable_get (ptr_hashtable, "key2.1"));
    ptr_hashtable = hdata_hashtable (ptr_hdata, ptr_item2,
                                     "1|test_ptr_2_hashtable");
    CHECK(ptr_hashtable);
    STRCMP_EQUAL("value2.2",
                 (const char *)hashtable_get (ptr_hashtable, "key2.2"));
}

/*
 * Tests functions:
 *   hdata_compare
 */

TEST(CoreHdataWithList, Compare)
{
    LONGS_EQUAL(0, hdata_compare (NULL, NULL, NULL, NULL, 0));
    LONGS_EQUAL(0, hdata_compare (ptr_hdata, NULL, NULL, NULL, 0));

    /* one or two pointers are missing */
    LONGS_EQUAL(0, hdata_compare (ptr_hdata, NULL, NULL, "test_char", 0));
    LONGS_EQUAL(1, hdata_compare (ptr_hdata, ptr_item1, NULL, "test_char", 0));
    LONGS_EQUAL(-1, hdata_compare (ptr_hdata, NULL, ptr_item2, "test_char", 0));

    /* compare chars: 'A' and 'a' */
    LONGS_EQUAL(0, hdata_compare (ptr_hdata, ptr_item1, ptr_item1,
                                  "test_char", 0));
    LONGS_EQUAL(-1, hdata_compare (ptr_hdata, ptr_item1, ptr_item2,
                                   "test_char", 0));
    LONGS_EQUAL(1, hdata_compare (ptr_hdata, ptr_item2, ptr_item1,
                                  "test_char", 0));

    /* compare strings: "STRING2" and "string2" */
    LONGS_EQUAL(0, hdata_compare (ptr_hdata, ptr_item1, ptr_item2,
                                  "test_string2", 0));
    LONGS_EQUAL(-1, hdata_compare (ptr_hdata, ptr_item1, ptr_item2,
                                   "test_string2", 1));
    LONGS_EQUAL(1, hdata_compare (ptr_hdata, ptr_item2, ptr_item1,
                                  "test_string2", 1));

    /* compare strings: "test" and NULL */
    LONGS_EQUAL(1, hdata_compare (ptr_hdata, ptr_item1, ptr_item2,
                                  "test_string3", 0));
    LONGS_EQUAL(-1, hdata_compare (ptr_hdata, ptr_item2, ptr_item1,
                                   "test_string3", 0));

    /* compare strings: NULL and NULL */
    LONGS_EQUAL(0, hdata_compare (ptr_hdata, ptr_item2, ptr_item1,
                                  "test_string_null", 0));

    /* compare integers: 123 and 456 */
    LONGS_EQUAL(0, hdata_compare (ptr_hdata, ptr_item1, ptr_item1,
                                  "test_int", 0));
    LONGS_EQUAL(-1, hdata_compare (ptr_hdata, ptr_item1, ptr_item2,
                                   "test_int", 0));
    LONGS_EQUAL(1, hdata_compare (ptr_hdata, ptr_item2, ptr_item1,
                                  "test_int", 0));

    /* compare long integers: 123456789L and 987654321L */
    LONGS_EQUAL(0, hdata_compare (ptr_hdata, ptr_item1, ptr_item1,
                                  "test_long", 0));
    LONGS_EQUAL(-1, hdata_compare (ptr_hdata, ptr_item1, ptr_item2,
                                   "test_long", 0));
    LONGS_EQUAL(1, hdata_compare (ptr_hdata, ptr_item2, ptr_item1,
                                  "test_long", 0));

    /* compare pointers: 0x123 and 0x456 */
    LONGS_EQUAL(0, hdata_compare (ptr_hdata, ptr_item1, ptr_item1,
                                  "test_pointer", 0));
    LONGS_EQUAL(-1, hdata_compare (ptr_hdata, ptr_item1, ptr_item2,
                                   "test_pointer", 0));
    LONGS_EQUAL(1, hdata_compare (ptr_hdata, ptr_item2, ptr_item1,
                                  "test_pointer", 0));

    /* compare times: 123456 and 789123 */
    LONGS_EQUAL(0, hdata_compare (ptr_hdata, ptr_item1, ptr_item1,
                                  "test_time", 0));
    LONGS_EQUAL(-1, hdata_compare (ptr_hdata, ptr_item1, ptr_item2,
                                   "test_time", 0));
    LONGS_EQUAL(1, hdata_compare (ptr_hdata, ptr_item2, ptr_item1,
                                  "test_time", 0));

    /* compare hashtables: not possible */
    LONGS_EQUAL(0, hdata_compare (ptr_hdata, ptr_item1, ptr_item2,
                                  "test_hashtable", 0));

    /* compare "other" type: not possible */
    LONGS_EQUAL(0, hdata_compare (ptr_hdata, ptr_item1, ptr_item2,
                                  "test_other", 0));
}

/*
 * Tests functions:
 *   hdata_set
 *   hdata_update
 */

TEST(CoreHdataWithList, Update)
{
    struct t_hashtable *hashtable, *ptr_old_hashtable;
    void *ptr_old_pointer;

    hashtable = hashtable_new (8,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL,
                               NULL);

    LONGS_EQUAL(0, hdata_update (NULL, NULL, NULL));
    LONGS_EQUAL(0, hdata_update (ptr_hdata, NULL, NULL));
    LONGS_EQUAL(0, hdata_update (NULL, ptr_item1, NULL));
    LONGS_EQUAL(0, hdata_update (NULL, NULL, hashtable));
    LONGS_EQUAL(0, hdata_update (ptr_hdata, ptr_item1, NULL));
    LONGS_EQUAL(0, hdata_update (ptr_hdata, NULL, hashtable));
    LONGS_EQUAL(0, hdata_update (NULL, ptr_item1, hashtable));

    /* check update without update callback */
    hdata_new (NULL, "test_item2", "prev_item", "next_item", 1, 1, NULL, NULL);
    LONGS_EQUAL(0, hdata_update (ptr_hdata, ptr_item1, hashtable));
    hashtable_remove (weechat_hdata, "test_item2");

    /* check if create is allowed */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "__create_allowed", "1");
    LONGS_EQUAL(1, hdata_update (ptr_hdata, ptr_item1, hashtable));

    /* check if delete is allowed */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "__delete_allowed", "1");
    LONGS_EQUAL(1, hdata_update (ptr_hdata, ptr_item1, hashtable));

    /* check if update is allowed on a variable */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "__update_allowed", "zzz");
    LONGS_EQUAL(0, hdata_update (ptr_hdata, ptr_item1, hashtable));
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "__update_allowed", "test_string");
    LONGS_EQUAL(1, hdata_update (ptr_hdata, ptr_item1, hashtable));
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "__update_allowed", "test_string2");
    LONGS_EQUAL(0, hdata_update (ptr_hdata, ptr_item1, hashtable));

    /* variable not found */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "zzz", "test");
    LONGS_EQUAL(0, hdata_update (ptr_hdata, ptr_item1, hashtable));

    /* update not allowed on the variable */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_string2", "test");
    LONGS_EQUAL(0, hdata_update (ptr_hdata, ptr_item1, hashtable));
    STRCMP_EQUAL("STRING2", ptr_item1->test_string2);

    /* set empty char */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_char", "");
    LONGS_EQUAL(1, hdata_update (ptr_hdata, ptr_item1, hashtable));
    LONGS_EQUAL('\0', ptr_item1->test_char);

    /* set char to 'M' */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_char", "M");
    LONGS_EQUAL(1, hdata_update (ptr_hdata, ptr_item1, hashtable));
    LONGS_EQUAL('M', ptr_item1->test_char);

    /* set string to NULL */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_string", NULL);
    LONGS_EQUAL(1, hdata_update (ptr_hdata, ptr_item1, hashtable));
    POINTERS_EQUAL(NULL, ptr_item1->test_string);

    /* set string to empty string */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_string", "");
    LONGS_EQUAL(1, hdata_update (ptr_hdata, ptr_item1, hashtable));
    STRCMP_EQUAL("", ptr_item1->test_string);

    /* set string to "test" */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_string", "test");
    LONGS_EQUAL(1, hdata_update (ptr_hdata, ptr_item1, hashtable));
    STRCMP_EQUAL("test", ptr_item1->test_string);

    /* set shared string to NULL */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_shared_string", NULL);
    LONGS_EQUAL(1, hdata_update (ptr_hdata, ptr_item1, hashtable));
    POINTERS_EQUAL(NULL, ptr_item1->test_shared_string);

    /* set shared string to empty string */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_shared_string", "");
    LONGS_EQUAL(1, hdata_update (ptr_hdata, ptr_item1, hashtable));
    STRCMP_EQUAL("", ptr_item1->test_shared_string);

    /* set shared string to "test_shared" */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_shared_string", "test_shared");
    LONGS_EQUAL(1, hdata_update (ptr_hdata, ptr_item1, hashtable));
    STRCMP_EQUAL("test_shared", ptr_item1->test_shared_string);

    /* set int to invalid value */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_int", "abc");
    LONGS_EQUAL(0, hdata_update (ptr_hdata, ptr_item1, hashtable));
    LONGS_EQUAL(123, ptr_item1->test_int);

    /* set int to -5 */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_int", "-5");
    LONGS_EQUAL(1, hdata_update (ptr_hdata, ptr_item1, hashtable));
    LONGS_EQUAL(-5, ptr_item1->test_int);

    /* set int to 77 */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_int", "77");
    LONGS_EQUAL(1, hdata_update (ptr_hdata, ptr_item1, hashtable));
    LONGS_EQUAL(77, ptr_item1->test_int);

    /* set long to invalid value */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_long", "abc");
    LONGS_EQUAL(0, hdata_update (ptr_hdata, ptr_item1, hashtable));
    LONGS_EQUAL(123456789L, ptr_item1->test_long);

    /* set long to -55 */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_long", "-55");
    LONGS_EQUAL(1, hdata_update (ptr_hdata, ptr_item1, hashtable));
    LONGS_EQUAL(-55, ptr_item1->test_long);

    /* set long to 777 */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_long", "777");
    LONGS_EQUAL(1, hdata_update (ptr_hdata, ptr_item1, hashtable));
    LONGS_EQUAL(777, ptr_item1->test_long);

    /* set pointer to invalid value */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_pointer", "zzz");
    LONGS_EQUAL(0, hdata_update (ptr_hdata, ptr_item1, hashtable));
    POINTERS_EQUAL(0x123, ptr_item1->test_pointer);

    /* set pointer to NULL */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_pointer", NULL);
    LONGS_EQUAL(1, hdata_update (ptr_hdata, ptr_item1, hashtable));
    POINTERS_EQUAL(NULL, ptr_item1->test_pointer);

    /* set pointer to 0x1a2b3c */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_pointer", "0x1a2b3c");
    LONGS_EQUAL(1, hdata_update (ptr_hdata, ptr_item1, hashtable));
    POINTERS_EQUAL(0x1a2b3c, ptr_item1->test_pointer);

    /* set time to invalid value */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_time", "-10");
    LONGS_EQUAL(0, hdata_update (ptr_hdata, ptr_item1, hashtable));
    LONGS_EQUAL(123456, ptr_item1->test_time);

    /* set time to 112233 */
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_time", "112233");
    LONGS_EQUAL(1, hdata_update (ptr_hdata, ptr_item1, hashtable));
    LONGS_EQUAL(112233, ptr_item1->test_time);

    /* set hashtable to NULL (not possible) */
    ptr_old_hashtable = ptr_item1->test_hashtable;
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_hashtable", NULL);
    LONGS_EQUAL(0, hdata_update (ptr_hdata, ptr_item1, hashtable));
    POINTERS_EQUAL(ptr_old_hashtable, ptr_item1->test_hashtable);

    /* set other to NULL (not possible) */
    ptr_old_pointer = ptr_item1->test_other;
    hashtable_remove_all (hashtable);
    hashtable_set (hashtable, "test_other", NULL);
    LONGS_EQUAL(0, hdata_update (ptr_hdata, ptr_item1, hashtable));
    POINTERS_EQUAL(ptr_old_pointer, ptr_item1->test_other);
}

/*
 * Tests functions:
 *   hdata_get_string
 */

TEST(CoreHdataWithList, GetString)
{
    const char *prop;
    char **items;
    int num_items;

    POINTERS_EQUAL(NULL, hdata_get_string (NULL, NULL));
    POINTERS_EQUAL(NULL, hdata_get_string (ptr_hdata, NULL));
    POINTERS_EQUAL(NULL, hdata_get_string (NULL, "var_keys"));

    POINTERS_EQUAL(NULL, hdata_get_string (ptr_hdata, "zzz"));

    STRCMP_EQUAL(
        "test_char,test_count_char,test_array_2_char_fixed_size,"
        "test_ptr_2_char,test_int,test_count_int,test_array_2_int_fixed_size,"
        "test_ptr_3_int,test_ptr_1_int_fixed_size,test_long,test_count_long,"
        "test_array_2_long_fixed_size,test_ptr_2_long,test_string,"
        "test_string2,test_string3,test_string_null,test_shared_string,"
        "test_count_words,test_array_2_words_fixed_size,test_ptr_words,"
        "test_ptr_words_dyn,test_ptr_words_dyn_shared,test_pointer,"
        "test_count_pointer,test_array_2_pointer_fixed_size,"
        "test_ptr_3_pointer,test_ptr_0_pointer_dyn,test_ptr_1_pointer_dyn,"
        "test_time,test_count_time,test_array_2_time_fixed_size,"
        "test_ptr_2_time,test_hashtable,test_count_hashtable,"
        "test_array_2_hashtable_fixed_size,test_ptr_2_hashtable,"
        "test_ptr_1_hashtable_dyn,test_other,test_count_other,"
        "test_ptr_3_other,test_count_invalid,test_ptr_invalid,prev_item,"
        "next_item",
        hdata_get_string (ptr_hdata, "var_keys"));

    prop = hdata_get_string (ptr_hdata, "var_values");
    items = string_split (prop, ",", NULL, 0, 0, &num_items);
    LONGS_EQUAL(45, num_items);
    string_free_split (items);

    prop = hdata_get_string (ptr_hdata, "var_keys_values");
    items = string_split (prop, ",", NULL, 0, 0, &num_items);
    LONGS_EQUAL(45, num_items);
    string_free_split (items);

    STRCMP_EQUAL("prev_item", hdata_get_string (ptr_hdata, "var_prev"));
    STRCMP_EQUAL("next_item", hdata_get_string (ptr_hdata, "var_next"));

    STRCMP_EQUAL("items,last_item", hdata_get_string (ptr_hdata, "list_keys"));
    prop = hdata_get_string (ptr_hdata, "list_values");
    items = string_split (prop, ",", NULL, 0, 0, &num_items);
    LONGS_EQUAL(2, num_items);
    string_free_split (items);
    prop = hdata_get_string (ptr_hdata, "list_keys_values");
    items = string_split (prop, ",", NULL, 0, 0, &num_items);
    LONGS_EQUAL(2, num_items);
    string_free_split (items);
}

/*
 * Tests functions:
 *   hdata_free
 *   hdata_free_all_plugin
 *   hdata_free_all
 */

TEST(CoreHdataWithList, Free)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hdata_print_log
 */

TEST(CoreHdataWithList, PrintLog)
{
    /* TODO: write tests */
}
