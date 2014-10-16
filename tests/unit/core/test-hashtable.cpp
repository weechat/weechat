/*
 * test-hashtable.cpp - test hashtable functions
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
#include "src/core/wee-hashtable.h"
#include "src/plugins/plugin.h"
}

#define HASHTABLE_TEST_KEY      "test"
#define HASHTABLE_TEST_KEY_HASH 5849825121ULL
#define HASHTABLE_TEST_VALUE    "this is a value"

TEST_GROUP(Hashtable)
{
};

/*
 * Tests functions:
 *   hashtable_hash_key_djb2
 */

TEST(Hashtable, HashDbj2)
{
    unsigned long long hash;

    hash = hashtable_hash_key_djb2 (HASHTABLE_TEST_KEY);
    CHECK(hash == HASHTABLE_TEST_KEY_HASH);
}

/*
 * Test callback hashing a key.
 *
 * It returns the djb2 hash + 1.
 */

unsigned long long
test_hashtable_hash_key_cb (struct t_hashtable *hashtable, const void *key)
{
    return hashtable_hash_key_djb2 ((const char *)key) + 1;
}

/*
 * Test callback comparing two keys.
 *
 * It just makes a string comparison (strcmp) between both keys.
 */

int
test_hashtable_keycmp_cb (struct t_hashtable *hashtable,
                          const void *key1, const void *key2)
{
    return strcmp ((const char *)key1, (const char *)key2);
}

/*
 * Tests functions:
 *   hashtable_new
 */

TEST(Hashtable, New)
{
    struct t_hashtable *hashtable;

    hashtable = hashtable_new (-1, NULL, NULL, NULL, NULL);
    POINTERS_EQUAL(NULL, hashtable);

    /* test invalid size */
    POINTERS_EQUAL(NULL,
                   hashtable_new (-1,
                                  WEECHAT_HASHTABLE_STRING,
                                  WEECHAT_HASHTABLE_STRING,
                                  NULL, NULL));

    /* test invalid type for keys/values */
    POINTERS_EQUAL(NULL,
                   hashtable_new (32,
                                  "xxxxx",  /* invalid */
                                  "yyyyy",  /* invalid */
                                  NULL, NULL));

    /* valid hashtable */
    hashtable = hashtable_new (32,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_INTEGER,
                               &test_hashtable_hash_key_cb,
                               &test_hashtable_keycmp_cb);
    CHECK(hashtable);
    LONGS_EQUAL(32, hashtable->size);
    CHECK(hashtable->htable);
    LONGS_EQUAL(0, hashtable->items_count);
    LONGS_EQUAL(HASHTABLE_STRING, hashtable->type_keys);
    LONGS_EQUAL(HASHTABLE_INTEGER, hashtable->type_values);
    POINTERS_EQUAL(&test_hashtable_hash_key_cb, hashtable->callback_hash_key);
    POINTERS_EQUAL(&test_hashtable_keycmp_cb, hashtable->callback_keycmp);
    POINTERS_EQUAL(NULL, hashtable->callback_free_key);
    POINTERS_EQUAL(NULL, hashtable->callback_free_value);
    hashtable_free (hashtable);
}

/*
 * Tests functions:
 *   hashtable_set_with_size
 *   hashtable_set
 *   hashtable_get_item
 *   hashtable_get
 *   hashtable_has_key
 *   hashtable_dup
 *   hashtable_remove
 *   hashtable_remove_all
 *   hashtable_free
 */

TEST(Hashtable, SetGetRemove)
{
    struct t_hashtable *hashtable, *hashtable2;
    struct t_hashtable_item *item, *ptr_item, *ptr_item2;
    const char *str_key = HASHTABLE_TEST_KEY;
    const char *str_value = HASHTABLE_TEST_VALUE;
    const char *ptr_value;
    unsigned long long hash;
    int i, j;

    hashtable = hashtable_new (32,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               &test_hashtable_hash_key_cb,
                               &test_hashtable_keycmp_cb);
    LONGS_EQUAL(32, hashtable->size);
    LONGS_EQUAL(0, hashtable->items_count);

    /* invalid set of items */
    POINTERS_EQUAL(NULL, hashtable_set_with_size (NULL, NULL, -1, NULL, -1));
    POINTERS_EQUAL(NULL, hashtable_set_with_size (NULL, NULL, -1, NULL, -1));

    /* add an item in hashtable with NULL value */
    item = hashtable_set (hashtable, str_key, NULL);
    CHECK(item);
    LONGS_EQUAL(1, hashtable->items_count);
    STRCMP_EQUAL(str_key, (const char *)item->key);
    LONGS_EQUAL(strlen (str_key) + 1, item->key_size);
    POINTERS_EQUAL(NULL, item->value);
    LONGS_EQUAL(0, item->value_size);
    POINTERS_EQUAL(NULL, item->prev_item);
    POINTERS_EQUAL(NULL, item->next_item);

    /* set a string value for the same key */
    item = hashtable_set (hashtable, str_key, str_value);
    CHECK(item);
    LONGS_EQUAL(1, hashtable->items_count);
    STRCMP_EQUAL(str_key, (const char *)item->key);
    LONGS_EQUAL(strlen (str_key) + 1, item->key_size);
    STRCMP_EQUAL(str_value, (const char *)item->value);
    LONGS_EQUAL(strlen (str_value) + 1, item->value_size);
    POINTERS_EQUAL(NULL, item->prev_item);
    POINTERS_EQUAL(NULL, item->next_item);

    /* get item */
    item = hashtable_get_item (hashtable, str_key, &hash);
    CHECK(item);
    STRCMP_EQUAL(str_key, (const char *)item->key);
    STRCMP_EQUAL(str_value, (const char *)item->value);
    LONGS_EQUAL(2, hash);

    /* get value */
    ptr_value = (const char *)hashtable_get (hashtable, str_key);
    CHECK(ptr_value);
    STRCMP_EQUAL(ptr_value, str_value);

    /* check if key is in hashtable */
    LONGS_EQUAL(0, hashtable_has_key (hashtable, NULL));
    LONGS_EQUAL(0, hashtable_has_key (hashtable, ""));
    LONGS_EQUAL(0, hashtable_has_key (hashtable, "xxx"));
    LONGS_EQUAL(1, hashtable_has_key (hashtable, str_key));

    /* delete an item */
    hashtable_remove (hashtable, str_key);
    LONGS_EQUAL(0, hashtable->items_count);

    /* add an item with size in hashtable */
    item = hashtable_set_with_size (hashtable,
                                    str_key, strlen (str_key) + 1,
                                    str_value, strlen (str_value) + 1);
    CHECK(item);
    LONGS_EQUAL(1, hashtable->items_count);
    STRCMP_EQUAL(str_key, (const char *)item->key);
    LONGS_EQUAL(strlen (str_key) + 1, item->key_size);
    STRCMP_EQUAL(str_value, (const char *)item->value);
    LONGS_EQUAL(strlen (str_value) + 1, item->value_size);

    /* add another item */
    hashtable_set (hashtable, "xxx", "zzz");
    LONGS_EQUAL(2, hashtable->items_count);

    /*
     * test duplication of hashtable and check that duplicated content is
     * exactly the same as initial hashtable
     */
    hashtable2 = hashtable_dup (hashtable);
    CHECK(hashtable2);
    LONGS_EQUAL(hashtable->size, hashtable2->size);
    LONGS_EQUAL(hashtable->items_count, hashtable2->items_count);
    for (i = 0; i < hashtable->size; i++)
    {
        if (hashtable->htable[i])
        {
            ptr_item = hashtable->htable[i];
            ptr_item2 = hashtable2->htable[i];
            while (ptr_item && ptr_item2)
            {
                LONGS_EQUAL(ptr_item->key_size, ptr_item2->key_size);
                LONGS_EQUAL(ptr_item->value_size, ptr_item2->value_size);
                if (ptr_item->key)
                {
                    STRCMP_EQUAL((const char *)ptr_item->key,
                                 (const char *)ptr_item2->key);
                }
                else
                {
                    POINTERS_EQUAL(ptr_item->key, ptr_item2->key);
                }
                if (ptr_item->value)
                {
                    STRCMP_EQUAL((const char *)ptr_item->value,
                                 (const char *)ptr_item2->value);
                }
                else
                {
                    POINTERS_EQUAL(ptr_item->value, ptr_item2->value);
                }
                ptr_item = ptr_item->next_item;
                ptr_item2 = ptr_item2->next_item;
                CHECK((ptr_item && ptr_item2) || (!ptr_item && !ptr_item2));
            }
        }
        else
        {
            POINTERS_EQUAL(hashtable->htable[i], hashtable2->htable[i]);
        }
    }

    /* remove all items */
    hashtable_remove_all (hashtable);
    LONGS_EQUAL(0, hashtable->items_count);

    /* free hashtable */
    hashtable_free (hashtable);
    hashtable_free (hashtable2);
}

/*
 * Tests functions:
 *   hashtable_map
 *   hashtable_map_string
 */

TEST(Hashtable, Map)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hashtable_get_list_keys
 *   hashtable_get_integer
 *   hashtable_get_string
 *   hashtable_set_pointer
 */

TEST(Hashtable, Properties)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hashtable_add_to_infolist
 */

TEST(Hashtable, Infolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hashtable_print_log
 */

TEST(Hashtable, PrintLog)
{
    /* TODO: write tests */
}
