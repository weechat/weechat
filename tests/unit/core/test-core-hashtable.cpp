/*
 * SPDX-FileCopyrightText: 2014-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

/* Test hashtable functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "src/core/core-hashtable.h"
#include "src/core/core-infolist.h"
#include "src/core/core-list.h"
#include "src/plugins/plugin.h"
}

#define HASHTABLE_TEST_KEY           "test"
#define HASHTABLE_TEST_KEY_HASH      5849825121ULL
#define HASHTABLE_TEST_KEY_LONG      "abcdefghijklmnopqrstuvwxyz"
#define HASHTABLE_TEST_KEY_LONG_HASH 11232856562070989738ULL
#define HASHTABLE_TEST_VALUE         "this is a value"

char *test_map_string = NULL;

TEST_GROUP(CoreHashtable)
{
    struct t_hashtable *get_weechat_hashtable ()
    {
        struct t_hashtable *hashtable;

        hashtable = hashtable_new (8,
                                   WEECHAT_HASHTABLE_STRING,
                                   WEECHAT_HASHTABLE_STRING,
                                   NULL,
                                   NULL);
        hashtable_set (hashtable, "weechat", "the first item");
        hashtable_set (hashtable, "light", "item2");
        hashtable_set (hashtable, "fast", "item3");
        hashtable_set (hashtable, "extensible", "item4");
        hashtable_set (hashtable, "chat", "item5");
        hashtable_set (hashtable, "client", "last item");
        return hashtable;
    }
};

/*
 * Test functions:
 *   hashtable_hash_key_djb2
 */

TEST(CoreHashtable, HashDbj2)
{
    unsigned long long hash;

    hash = hashtable_hash_key_djb2 (HASHTABLE_TEST_KEY);
    CHECK(hash == HASHTABLE_TEST_KEY_HASH);

    hash = hashtable_hash_key_djb2 (HASHTABLE_TEST_KEY_LONG);
    CHECK(hash == HASHTABLE_TEST_KEY_LONG_HASH);
}

/*
 * Test callback hashing a key.
 *
 * It returns the djb2 hash + 1.
 */

unsigned long long
test_hashtable_hash_key_cb (struct t_hashtable *hashtable, const void *key)
{
    /* make C++ compiler happy */
    (void) hashtable;

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
    /* make C++ compiler happy */
    (void) hashtable;

    return strcmp ((const char *)key1, (const char *)key2);
}

/*
 * Test functions:
 *   hashtable_new
 */

TEST(CoreHashtable, New)
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
    POINTERS_EQUAL(NULL, hashtable->oldest_item);
    POINTERS_EQUAL(NULL, hashtable->newest_item);
    LONGS_EQUAL(HASHTABLE_STRING, hashtable->type_keys);
    LONGS_EQUAL(HASHTABLE_INTEGER, hashtable->type_values);
    POINTERS_EQUAL(&test_hashtable_hash_key_cb, hashtable->callback_hash_key);
    POINTERS_EQUAL(&test_hashtable_keycmp_cb, hashtable->callback_keycmp);
    POINTERS_EQUAL(NULL, hashtable->callback_free_key);
    POINTERS_EQUAL(NULL, hashtable->callback_free_value);
    hashtable_free (hashtable);
}

/*
 * Test functions:
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

TEST(CoreHashtable, SetGetRemove)
{
    struct t_hashtable *hashtable, *hashtable2;
    struct t_hashtable_item *item, *ptr_item, *ptr_item2;
    const char *str_key = HASHTABLE_TEST_KEY;
    const char *str_value = HASHTABLE_TEST_VALUE;
    const char *ptr_value;
    unsigned long long hash;
    int i, value_int, value_int2, *ptr_int;
    long long value_longlong;
    time_t value_time;

    /* free hashtable with NULL pointer */
    hashtable_free (NULL);

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
    CHECK(hashtable->oldest_item);
    CHECK(hashtable->newest_item);
    STRCMP_EQUAL(str_key, (const char *)item->key);
    LONGS_EQUAL(strlen (str_key) + 1, item->key_size);
    POINTERS_EQUAL(NULL, item->value);
    LONGS_EQUAL(0, item->value_size);
    POINTERS_EQUAL(NULL, item->prev_item);
    POINTERS_EQUAL(NULL, item->next_item);
    POINTERS_EQUAL(NULL, item->prev_created_item);
    POINTERS_EQUAL(NULL, item->next_created_item);

    /* set a string value for the same key */
    item = hashtable_set (hashtable, str_key, str_value);
    CHECK(item);
    LONGS_EQUAL(1, hashtable->items_count);
    CHECK(hashtable->oldest_item);
    CHECK(hashtable->newest_item);
    STRCMP_EQUAL(str_key, (const char *)item->key);
    LONGS_EQUAL(strlen (str_key) + 1, item->key_size);
    STRCMP_EQUAL(str_value, (const char *)item->value);
    LONGS_EQUAL(strlen (str_value) + 1, item->value_size);
    POINTERS_EQUAL(NULL, item->prev_item);
    POINTERS_EQUAL(NULL, item->next_item);
    POINTERS_EQUAL(NULL, item->prev_created_item);
    POINTERS_EQUAL(NULL, item->next_created_item);

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
    POINTERS_EQUAL(NULL, hashtable->oldest_item);
    POINTERS_EQUAL(NULL, hashtable->newest_item);

    /* add an item with size in hashtable */
    item = hashtable_set_with_size (hashtable,
                                    str_key, strlen (str_key) + 1,
                                    str_value, strlen (str_value) + 1);
    CHECK(item);
    LONGS_EQUAL(1, hashtable->items_count);
    CHECK(hashtable->oldest_item);
    CHECK(hashtable->newest_item);
    STRCMP_EQUAL(str_key, (const char *)item->key);
    LONGS_EQUAL(strlen (str_key) + 1, item->key_size);
    STRCMP_EQUAL(str_value, (const char *)item->value);
    LONGS_EQUAL(strlen (str_value) + 1, item->value_size);

    /* add another item */
    hashtable_set (hashtable, "xxx", "zzz");
    LONGS_EQUAL(2, hashtable->items_count);
    CHECK(hashtable->oldest_item);
    CHECK(hashtable->newest_item);
    CHECK(hashtable->oldest_item != hashtable->newest_item);
    CHECK(hashtable->oldest_item->next_created_item == hashtable->newest_item);
    STRCMP_EQUAL(str_key, (const char *)hashtable->oldest_item->key);
    STRCMP_EQUAL("xxx",
                 (const char *)hashtable->oldest_item->next_created_item->key);
    STRCMP_EQUAL("xxx",
                 (const char *)hashtable->newest_item->key);

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
    POINTERS_EQUAL(NULL, hashtable->htable[0]);
    POINTERS_EQUAL(NULL, hashtable->htable[1]);
    POINTERS_EQUAL(NULL, hashtable->htable[2]);
    POINTERS_EQUAL(NULL, hashtable->htable[3]);
    POINTERS_EQUAL(NULL, hashtable->htable[4]);
    POINTERS_EQUAL(NULL, hashtable->htable[5]);
    POINTERS_EQUAL(NULL, hashtable->htable[6]);
    POINTERS_EQUAL(NULL, hashtable->htable[7]);
    LONGS_EQUAL(0, hashtable->items_count);
    POINTERS_EQUAL(NULL, hashtable->oldest_item);
    POINTERS_EQUAL(NULL, hashtable->newest_item);

    /* free hashtables */
    hashtable_free (hashtable);
    hashtable_free (hashtable2);

    /*
     * create a hashtable with size 8, and add 6 items,
     * to check if many items with same hashed key work fine,
     * the expected htable inside hashtable is:
     *   +-----+
     *   |   0 |
     *   +-----+
     *   |   1 |
     *   +-----+
     *   |   2 | --> "extensible"
     *   +-----+
     *   |   3 | --> "fast" --> "light"
     *   +-----+
     *   |   4 |
     *   +-----+
     *   |   5 | --> "chat"
     *   +-----+
     *   |   6 | --> "client"
     *   +-----+
     *   |   7 | --> "weechat"
     *   +-----+
     */
    hashtable = hashtable_new (8,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL,
                               NULL);
    LONGS_EQUAL(8, hashtable->size);
    LONGS_EQUAL(0, hashtable->items_count);

    item = hashtable_set (hashtable, "weechat", NULL);
    CHECK(item);
    POINTERS_EQUAL(item, hashtable->htable[7]);

    item = hashtable_set (hashtable, "light", NULL);
    CHECK(item);
    POINTERS_EQUAL(item, hashtable->htable[3]);

    item = hashtable_set (hashtable, "fast", NULL);
    CHECK(item);
    POINTERS_EQUAL(item, hashtable->htable[3]);

    item = hashtable_set (hashtable, "extensible", NULL);
    CHECK(item);
    POINTERS_EQUAL(item, hashtable->htable[2]);

    item = hashtable_set (hashtable, "chat", NULL);
    CHECK(item);
    POINTERS_EQUAL(item, hashtable->htable[5]);

    item = hashtable_set (hashtable, "client", NULL);
    CHECK(item);
    POINTERS_EQUAL(item, hashtable->htable[6]);

    /* check items by order of creation */
    ptr_item = hashtable->oldest_item;
    STRCMP_EQUAL("weechat", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("light", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("fast", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("extensible", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("chat", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("client", (const char *)ptr_item->key);
    STRCMP_EQUAL("client", (const char *)hashtable->newest_item->key);
    ptr_item = ptr_item->next_created_item;
    POINTERS_EQUAL(NULL, ptr_item);

    /* remove items and check again by order of creation */
    hashtable_remove (hashtable, "fast");
    LONGS_EQUAL(5, hashtable->items_count);
    ptr_item = hashtable->oldest_item;
    STRCMP_EQUAL("weechat", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("light", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("extensible", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("chat", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("client", (const char *)ptr_item->key);
    STRCMP_EQUAL("client", (const char *)hashtable->newest_item->key);
    ptr_item = ptr_item->next_created_item;
    POINTERS_EQUAL(NULL, ptr_item);

    hashtable_remove (hashtable, "light");
    LONGS_EQUAL(4, hashtable->items_count);
    ptr_item = hashtable->oldest_item;
    STRCMP_EQUAL("weechat", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("extensible", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("chat", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("client", (const char *)ptr_item->key);
    STRCMP_EQUAL("client", (const char *)hashtable->newest_item->key);
    ptr_item = ptr_item->next_created_item;
    POINTERS_EQUAL(NULL, ptr_item);

    hashtable_remove (hashtable, "weechat");
    LONGS_EQUAL(3, hashtable->items_count);
    ptr_item = hashtable->oldest_item;
    STRCMP_EQUAL("extensible", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("chat", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("client", (const char *)ptr_item->key);
    STRCMP_EQUAL("client", (const char *)hashtable->newest_item->key);
    ptr_item = ptr_item->next_created_item;
    POINTERS_EQUAL(NULL, ptr_item);

    hashtable_remove (hashtable, "client");
    LONGS_EQUAL(2, hashtable->items_count);
    ptr_item = hashtable->oldest_item;
    STRCMP_EQUAL("extensible", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("chat", (const char *)ptr_item->key);
    STRCMP_EQUAL("chat", (const char *)hashtable->newest_item->key);
    ptr_item = ptr_item->next_created_item;
    POINTERS_EQUAL(NULL, ptr_item);

    /* check current content of hashtable */
    POINTERS_EQUAL(NULL, hashtable->htable[0]);
    POINTERS_EQUAL(NULL, hashtable->htable[1]);
    STRCMP_EQUAL("extensible", (const char *)hashtable->htable[2]->key);
    POINTERS_EQUAL(NULL, hashtable->htable[3]);
    POINTERS_EQUAL(NULL, hashtable->htable[4]);
    STRCMP_EQUAL("chat", (const char *)hashtable->htable[5]->key);
    POINTERS_EQUAL(NULL, hashtable->htable[6]);
    POINTERS_EQUAL(NULL, hashtable->htable[7]);

    /* free hashtable */
    hashtable_free (hashtable);

    /* test hashtable: integer -> integer */
    hashtable = hashtable_new (8,
                               WEECHAT_HASHTABLE_INTEGER,
                               WEECHAT_HASHTABLE_INTEGER,
                               NULL,
                               NULL);
    value_int = -1;
    value_int2 = -500;
    hashtable_set (hashtable, &value_int, &value_int2);
    value_int = 123;
    value_int2 = 500;
    hashtable_set (hashtable, &value_int, &value_int2);
    value_int = -2;
    POINTERS_EQUAL(NULL, hashtable_get (hashtable, &value_int));
    value_int = -1;
    ptr_int = (int *)hashtable_get (hashtable, &value_int);
    CHECK(ptr_int);
    LONGS_EQUAL(-500, *ptr_int);
    value_int = 123;
    ptr_int = (int *)hashtable_get (hashtable, &value_int);
    CHECK(ptr_int);
    LONGS_EQUAL(500, *ptr_int);
    hashtable_free (hashtable);

    /* test hashtable: pointer -> integer */
    hashtable = hashtable_new (8,
                               WEECHAT_HASHTABLE_POINTER,
                               WEECHAT_HASHTABLE_INTEGER,
                               NULL,
                               NULL);
    value_int = -500;
    hashtable_set (hashtable, (void *)0x0, &value_int);
    value_int = 500;
    hashtable_set (hashtable, (void *)0x1, &value_int);
    POINTERS_EQUAL(NULL, hashtable_get (hashtable, (void *)0x2));
    ptr_int = (int *)hashtable_get (hashtable, (void *)0x0);
    CHECK(ptr_int);
    LONGS_EQUAL(-500, *ptr_int);
    ptr_int = (int *)hashtable_get (hashtable, (void *)0x1);
    CHECK(ptr_int);
    LONGS_EQUAL(500, *ptr_int);
    hashtable_free (hashtable);

    /* test hashtable: time -> string */
    hashtable = hashtable_new (8,
                               WEECHAT_HASHTABLE_TIME,
                               WEECHAT_HASHTABLE_STRING,
                               NULL,
                               NULL);
    value_time = 0;
    hashtable_set (hashtable, &value_time, "test_0");
    value_time = 1718036808;
    hashtable_set (hashtable, &value_time, "test_1718036808");
    value_time = 1;
    STRCMP_EQUAL(NULL, (const char *)hashtable_get (hashtable, &value_time));
    value_time = 0;
    STRCMP_EQUAL("test_0", (const char *)hashtable_get (hashtable, &value_time));
    value_time = 1718036808;
    STRCMP_EQUAL("test_1718036808", (const char *)hashtable_get (hashtable, &value_time));
    hashtable_free (hashtable);

    /* test hashtable: long long -> string */
    hashtable = hashtable_new (8,
                               WEECHAT_HASHTABLE_LONGLONG,
                               WEECHAT_HASHTABLE_STRING,
                               NULL,
                               NULL);
    value_longlong = -1;
    hashtable_set (hashtable, &value_longlong, "test_-1");
    value_longlong = 123;
    hashtable_set (hashtable, &value_longlong, "test_123");
    value_longlong = 0;
    STRCMP_EQUAL(NULL, (const char *)hashtable_get (hashtable, &value_longlong));
    value_longlong = -1;
    STRCMP_EQUAL("test_-1", (const char *)hashtable_get (hashtable, &value_longlong));
    value_longlong = 123;
    STRCMP_EQUAL("test_123", (const char *)hashtable_get (hashtable, &value_longlong));
    hashtable_free (hashtable);
}

/*
 * Test functions:
 *   hashtable_rehash (via hashtable_set_with_size, growing on high load factor)
 */

TEST(CoreHashtable, Rehash)
{
    struct t_hashtable *hashtable;
    struct t_hashtable_item *ptr_item;
    char key[32], value[32];
    int i;

    hashtable = hashtable_new (4,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL,
                               NULL);
    LONGS_EQUAL(4, hashtable->size);

    /* fill up to the load factor limit: no rehash must happen yet */
    for (i = 0; i < 8; i++)
    {
        snprintf (key, sizeof (key), "key%d", i);
        snprintf (value, sizeof (value), "value%d", i);
        hashtable_set (hashtable, key, value);
    }
    LONGS_EQUAL(8, hashtable->items_count);
    LONGS_EQUAL(4, hashtable->size);

    /* one more item pushes items_count above size * load factor: size doubles */
    hashtable_set (hashtable, "key8", "value8");
    LONGS_EQUAL(9, hashtable->items_count);
    LONGS_EQUAL(8, hashtable->size);

    /* all items must still be reachable with their original value after the rehash */
    for (i = 0; i < 9; i++)
    {
        snprintf (key, sizeof (key), "key%d", i);
        snprintf (value, sizeof (value), "value%d", i);
        STRCMP_EQUAL(value, (const char *)hashtable_get (hashtable, key));
    }

    /* each bucket must keep its items sorted by key after the rehash */
    for (i = 0; i < hashtable->size; i++)
    {
        for (ptr_item = hashtable->htable[i];
             ptr_item && ptr_item->next_item;
             ptr_item = ptr_item->next_item)
        {
            CHECK(strcmp ((const char *)ptr_item->key,
                          (const char *)ptr_item->next_item->key) < 0);
        }
    }

    /* fill up to the next load factor limit to trigger a second rehash */
    for (i = 9; i < 17; i++)
    {
        snprintf (key, sizeof (key), "key%d", i);
        snprintf (value, sizeof (value), "value%d", i);
        hashtable_set (hashtable, key, value);
    }
    LONGS_EQUAL(17, hashtable->items_count);
    LONGS_EQUAL(16, hashtable->size);

    for (i = 0; i < 17; i++)
    {
        snprintf (key, sizeof (key), "key%d", i);
        snprintf (value, sizeof (value), "value%d", i);
        STRCMP_EQUAL(value, (const char *)hashtable_get (hashtable, key));
    }

    for (i = 0; i < hashtable->size; i++)
    {
        for (ptr_item = hashtable->htable[i];
             ptr_item && ptr_item->next_item;
             ptr_item = ptr_item->next_item)
        {
            CHECK(strcmp ((const char *)ptr_item->key,
                          (const char *)ptr_item->next_item->key) < 0);
        }
    }

    hashtable_free (hashtable);
}

/*
 * Test functions:
 *   hashtable_remove (shrink)
 */

TEST(CoreHashtable, ShrinkBasic)
{
    struct t_hashtable *hashtable;
    struct t_hashtable_item *ptr_item;
    char key[32], value[32];
    int i;

    hashtable = hashtable_new (32,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL,
                               NULL);

    /* fill with enough items to stay below the grow threshold */
    for (i = 0; i < 20; i++)
    {
        snprintf (key, sizeof (key), "key%d", i);
        snprintf (value, sizeof (value), "value%d", i);
        hashtable_set (hashtable, key, value);
    }
    LONGS_EQUAL(20, hashtable->items_count);
    LONGS_EQUAL(32, hashtable->size);

    /* remove down to 8 items: still above the shrink threshold */
    for (i = 19; i >= 8; i--)
    {
        snprintf (key, sizeof (key), "key%d", i);
        hashtable_remove (hashtable, key);
    }
    LONGS_EQUAL(8, hashtable->items_count);
    LONGS_EQUAL(32, hashtable->size);

    /* one more removal pushes items_count below size / load factor: size halves */
    hashtable_remove (hashtable, "key7");
    LONGS_EQUAL(7, hashtable->items_count);
    LONGS_EQUAL(16, hashtable->size);

    /* all remaining items must still be reachable with their original value */
    for (i = 0; i < 7; i++)
    {
        snprintf (key, sizeof (key), "key%d", i);
        snprintf (value, sizeof (value), "value%d", i);
        STRCMP_EQUAL(value, (const char *)hashtable_get (hashtable, key));
    }

    /* each bucket must keep its items sorted by key after the shrink */
    for (i = 0; i < hashtable->size; i++)
    {
        for (ptr_item = hashtable->htable[i];
             ptr_item && ptr_item->next_item;
             ptr_item = ptr_item->next_item)
        {
            CHECK(strcmp ((const char *)ptr_item->key,
                          (const char *)ptr_item->next_item->key) < 0);
        }
    }

    hashtable_free (hashtable);
}

TEST(CoreHashtable, ShrinkNoneAboveThreshold)
{
    struct t_hashtable *hashtable;
    char key[32], value[32];
    int i;

    hashtable = hashtable_new (32,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL,
                               NULL);

    for (i = 0; i < 10; i++)
    {
        snprintf (key, sizeof (key), "key%d", i);
        snprintf (value, sizeof (value), "value%d", i);
        hashtable_set (hashtable, key, value);
    }
    LONGS_EQUAL(10, hashtable->items_count);
    LONGS_EQUAL(32, hashtable->size);

    /* removing one item stays above the shrink threshold: no rehash */
    hashtable_remove (hashtable, "key9");
    LONGS_EQUAL(9, hashtable->items_count);
    LONGS_EQUAL(32, hashtable->size);

    hashtable_free (hashtable);
}

TEST(CoreHashtable, ShrinkNoThrashNearBoundary)
{
    struct t_hashtable *hashtable;
    char key[32], value[32];
    int i;

    hashtable = hashtable_new (32,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL,
                               NULL);

    /* fill to exactly half load: the boundary a naive design would react to */
    for (i = 0; i < 16; i++)
    {
        snprintf (key, sizeof (key), "key%d", i);
        snprintf (value, sizeof (value), "value%d", i);
        hashtable_set (hashtable, key, value);
    }
    LONGS_EQUAL(16, hashtable->items_count);
    LONGS_EQUAL(32, hashtable->size);

    /* toggling a single extra item near that boundary must never resize */
    for (i = 0; i < 3; i++)
    {
        hashtable_set (hashtable, "extra", "extra_value");
        LONGS_EQUAL(17, hashtable->items_count);
        LONGS_EQUAL(32, hashtable->size);

        hashtable_remove (hashtable, "extra");
        LONGS_EQUAL(16, hashtable->items_count);
        LONGS_EQUAL(32, hashtable->size);
    }

    /* remove real items down past the actual shrink threshold */
    for (i = 15; i >= 7; i--)
    {
        snprintf (key, sizeof (key), "key%d", i);
        hashtable_remove (hashtable, key);
    }
    LONGS_EQUAL(7, hashtable->items_count);
    LONGS_EQUAL(16, hashtable->size);

    /* the same toggle test must also be stable at the new (halved) size */
    for (i = 0; i < 3; i++)
    {
        hashtable_set (hashtable, "extra2", "extra_value2");
        LONGS_EQUAL(8, hashtable->items_count);
        LONGS_EQUAL(16, hashtable->size);

        hashtable_remove (hashtable, "extra2");
        LONGS_EQUAL(7, hashtable->items_count);
        LONGS_EQUAL(16, hashtable->size);
    }

    hashtable_free (hashtable);
}

TEST(CoreHashtable, ShrinkMinSize)
{
    struct t_hashtable *hashtable;
    char key[32], value[32];
    int i;

    hashtable = hashtable_new (32,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL,
                               NULL);

    for (i = 0; i < 5; i++)
    {
        snprintf (key, sizeof (key), "key%d", i);
        snprintf (value, sizeof (value), "value%d", i);
        hashtable_set (hashtable, key, value);
    }
    LONGS_EQUAL(5, hashtable->items_count);
    LONGS_EQUAL(32, hashtable->size);

    /* first removal: size halves once (32 -> 16) */
    hashtable_remove (hashtable, "key0");
    LONGS_EQUAL(4, hashtable->items_count);
    LONGS_EQUAL(16, hashtable->size);

    /* second removal: size halves again (16 -> 8, the minimum size) */
    hashtable_remove (hashtable, "key1");
    LONGS_EQUAL(3, hashtable->items_count);
    LONGS_EQUAL(8, hashtable->size);

    /* further removals down to zero items must never shrink below the minimum */
    hashtable_remove (hashtable, "key2");
    LONGS_EQUAL(2, hashtable->items_count);
    LONGS_EQUAL(8, hashtable->size);

    hashtable_remove (hashtable, "key3");
    LONGS_EQUAL(1, hashtable->items_count);
    LONGS_EQUAL(8, hashtable->size);

    hashtable_remove (hashtable, "key4");
    LONGS_EQUAL(0, hashtable->items_count);
    LONGS_EQUAL(8, hashtable->size);

    hashtable_free (hashtable);

    /* a hashtable created at the minimum size must never attempt to shrink */
    hashtable = hashtable_new (8,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL,
                               NULL);
    hashtable_set (hashtable, "key0", "value0");
    hashtable_set (hashtable, "key1", "value1");
    hashtable_set (hashtable, "key2", "value2");
    LONGS_EQUAL(3, hashtable->items_count);
    LONGS_EQUAL(8, hashtable->size);

    hashtable_remove (hashtable, "key0");
    hashtable_remove (hashtable, "key1");
    hashtable_remove (hashtable, "key2");
    LONGS_EQUAL(0, hashtable->items_count);
    LONGS_EQUAL(8, hashtable->size);

    hashtable_free (hashtable);
}

TEST(CoreHashtable, ShrinkRemoveAllKeepsSize)
{
    struct t_hashtable *hashtable;
    char key[32], value[32];
    int i, grown_size;

    hashtable = hashtable_new (8,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL,
                               NULL);

    /* fill enough to force at least one grow */
    for (i = 0; i < 20; i++)
    {
        snprintf (key, sizeof (key), "key%d", i);
        snprintf (value, sizeof (value), "value%d", i);
        hashtable_set (hashtable, key, value);
    }
    LONGS_EQUAL(20, hashtable->items_count);
    CHECK(hashtable->size > 8);
    grown_size = hashtable->size;

    /* removing everything at once must not shrink the table */
    hashtable_remove_all (hashtable);
    LONGS_EQUAL(0, hashtable->items_count);
    POINTERS_EQUAL(NULL, hashtable->oldest_item);
    POINTERS_EQUAL(NULL, hashtable->newest_item);
    LONGS_EQUAL(grown_size, hashtable->size);

    /* the hashtable must still work correctly at the retained size */
    hashtable_set (hashtable, "foo", "bar");
    hashtable_set (hashtable, "baz", "qux");
    LONGS_EQUAL(2, hashtable->items_count);
    LONGS_EQUAL(grown_size, hashtable->size);
    STRCMP_EQUAL("bar", (const char *)hashtable_get (hashtable, "foo"));
    STRCMP_EQUAL("qux", (const char *)hashtable_get (hashtable, "baz"));

    hashtable_free (hashtable);
}

void
test_hashtable_map_string_cb (void *data,
                              struct t_hashtable *hashtable,
                              const char *key, const char *value)
{
    /* make C++ compiler happy */
    (void) hashtable;
    (void) data;

    if (test_map_string[0])
        strcat (test_map_string, ";");
    strcat (test_map_string, key);
    strcat (test_map_string, ":");
    strcat (test_map_string, value);
}

/*
 * Test functions:
 *   hashtable_map_string
 */

TEST(CoreHashtable, MapString)
{
    struct t_hashtable *hashtable;
    int value_int;
    void *value_ptr;
    time_t value_time;
    char result[1024], value_buffer[3] = { 0x01, 0x05, 0x09 };

    test_map_string = (char *)malloc (1024);

    /* string -> string */
    test_map_string[0] = '\0';
    hashtable = get_weechat_hashtable ();
    hashtable_map_string (hashtable, &test_hashtable_map_string_cb, NULL);
    STRCMP_EQUAL("weechat:the first item;light:item2;fast:item3;"
                 "extensible:item4;chat:item5;client:last item",
                 test_map_string);

    /* integer -> pointer */
    test_map_string[0] = '\0';
    hashtable = hashtable_new (8,
                               WEECHAT_HASHTABLE_INTEGER,
                               WEECHAT_HASHTABLE_POINTER,
                               NULL,
                               NULL);
    value_int = 123;
    value_ptr = (void *)0x123abc;
    hashtable_set (hashtable, &value_int, value_ptr);
    value_int = 45678;
    value_ptr = (void *)0xdef789;
    hashtable_set (hashtable, &value_int, value_ptr);
    hashtable_map_string (hashtable, &test_hashtable_map_string_cb, NULL);
    STRCMP_EQUAL("123:0x123abc;45678:0xdef789", test_map_string);

    /* time -> buffer */
    test_map_string[0] = '\0';
    hashtable = hashtable_new (8,
                               WEECHAT_HASHTABLE_TIME,
                               WEECHAT_HASHTABLE_BUFFER,
                               NULL,
                               NULL);
    value_time = 1624693124;
    hashtable_set_with_size (hashtable,
                             &value_time, 0,
                             value_buffer, sizeof (value_buffer));
    hashtable_map_string (hashtable, &test_hashtable_map_string_cb, NULL);
    snprintf (result, sizeof (result),
              "1624693124:%p",
              hashtable->newest_item->value);
    STRCMP_EQUAL(result, test_map_string);

    free (test_map_string);
}

/*
 * Test functions:
 *   hashtable_map
 *   hashtable_dup
 */

TEST(CoreHashtable, Dup)
{
    struct t_hashtable *hashtable, *hashtable2;
    struct t_hashtable_item *ptr_item;

    hashtable = get_weechat_hashtable ();

    POINTERS_EQUAL(NULL, hashtable_dup (NULL));

    hashtable2 = hashtable_dup (hashtable);

    LONGS_EQUAL(6, hashtable2->items_count);
    ptr_item = hashtable2->oldest_item;
    STRCMP_EQUAL("weechat", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("light", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("fast", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("extensible", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("chat", (const char *)ptr_item->key);
    ptr_item = ptr_item->next_created_item;
    STRCMP_EQUAL("client", (const char *)ptr_item->key);
    STRCMP_EQUAL("client", (const char *)hashtable2->newest_item->key);
    ptr_item = ptr_item->next_created_item;
    POINTERS_EQUAL(NULL, ptr_item);

    hashtable_free (hashtable);
    hashtable_free (hashtable2);
}

/*
 * Test functions:
 *   hashtable_get_list_keys
 */

TEST(CoreHashtable, GetListKeys)
{
    struct t_hashtable *hashtable;
    struct t_weelist *list_keys;

    hashtable = get_weechat_hashtable ();

    POINTERS_EQUAL(NULL, hashtable_get_list_keys (NULL));

    list_keys = hashtable_get_list_keys (hashtable);
    CHECK(list_keys);
    STRCMP_EQUAL("chat", weelist_string (weelist_get (list_keys, 0)));
    STRCMP_EQUAL("client", weelist_string (weelist_get (list_keys, 1)));
    STRCMP_EQUAL("extensible", weelist_string (weelist_get (list_keys, 2)));
    STRCMP_EQUAL("fast", weelist_string (weelist_get (list_keys, 3)));
    STRCMP_EQUAL("light", weelist_string (weelist_get (list_keys, 4)));
    STRCMP_EQUAL("weechat", weelist_string (weelist_get (list_keys, 5)));

    hashtable_free (hashtable);
}

/*
 * Test functions:
 *   hashtable_get_integer
 */

TEST(CoreHashtable, GetInteger)
{
    struct t_hashtable *hashtable;

    hashtable = get_weechat_hashtable ();

    LONGS_EQUAL(0, hashtable_get_integer (NULL, NULL));
    LONGS_EQUAL(0, hashtable_get_integer (hashtable, NULL));
    LONGS_EQUAL(0, hashtable_get_integer (hashtable, ""));
    LONGS_EQUAL(0, hashtable_get_integer (hashtable, "unknown"));

    LONGS_EQUAL(8, hashtable_get_integer (hashtable, "size"));
    LONGS_EQUAL(6, hashtable_get_integer (hashtable, "items_count"));

    hashtable_free (hashtable);
}

/*
 * Test functions:
 *   hashtable_get_string
 */

TEST(CoreHashtable, GetString)
{
    struct t_hashtable *hashtable;

    hashtable = get_weechat_hashtable ();

    STRCMP_EQUAL(NULL, hashtable_get_string (NULL, NULL));
    STRCMP_EQUAL(NULL, hashtable_get_string (hashtable, NULL));
    STRCMP_EQUAL(NULL, hashtable_get_string (hashtable, ""));
    STRCMP_EQUAL(NULL, hashtable_get_string (hashtable, "unknown"));

    STRCMP_EQUAL("string", hashtable_get_string (hashtable, "type_keys"));
    STRCMP_EQUAL("string", hashtable_get_string (hashtable, "type_values"));
    STRCMP_EQUAL("weechat,light,fast,extensible,chat,client",
                 hashtable_get_string (hashtable, "keys"));
    STRCMP_EQUAL("chat,client,extensible,fast,light,weechat",
                 hashtable_get_string (hashtable, "keys_sorted"));
    STRCMP_EQUAL("the first item,item2,item3,item4,item5,last item",
                 hashtable_get_string (hashtable, "values"));
    STRCMP_EQUAL("weechat:the first item,light:item2,fast:item3,"
                 "extensible:item4,chat:item5,client:last item",
                 hashtable_get_string (hashtable, "keys_values"));
    STRCMP_EQUAL("chat:item5,client:last item,extensible:item4,fast:item3,"
                 "light:item2,weechat:the first item",
                 hashtable_get_string (hashtable, "keys_values_sorted"));

    hashtable_free (hashtable);
}

/*
 * Test callback freeing key (it does nothing).
 */

void
test_hashtable_free_key (struct t_hashtable *hashtable, void *key)
{
    /* make C++ compiler happy */
    (void) hashtable;
    (void) key;
}

/*
 * Test callback freeing value (it does nothing).
 */

void
test_hashtable_free_value (struct t_hashtable *hashtable,
                           const void *key, void *value)
{
    /* make C++ compiler happy */
    (void) hashtable;
    (void) key;
    (void) value;
}

/*
 * Test functions:
 *   hashtable_set_pointer
 */

TEST(CoreHashtable, SetPointer)
{
    struct t_hashtable *hashtable;

    hashtable = get_weechat_hashtable ();

    hashtable_set_pointer (NULL, NULL, NULL);
    hashtable_set_pointer (hashtable, NULL, NULL);
    hashtable_set_pointer (hashtable, "", NULL);
    hashtable_set_pointer (hashtable, "unknown", NULL);

    hashtable_set_pointer (hashtable,
                           "callback_free_key", (void *)&test_hashtable_free_key);
    POINTERS_EQUAL(&test_hashtable_free_key, hashtable->callback_free_key);
    hashtable_set_pointer (hashtable,
                           "callback_free_value", (void *)&test_hashtable_free_value);
    POINTERS_EQUAL(&test_hashtable_free_value, hashtable->callback_free_value);

    hashtable_free (hashtable);
}

/*
 * Test functions:
 *   hashtable_add_to_infolist
 *   hashtable_add_from_infolist
 */

TEST(CoreHashtable, Infolist)
{
    struct t_hashtable *hashtable;
    struct t_infolist *infolist;
    struct t_infolist_item *infolist_item;

    hashtable = get_weechat_hashtable ();

    infolist = infolist_new (NULL);
    infolist_item = infolist_new_item (infolist);

    LONGS_EQUAL(0, hashtable_add_to_infolist (NULL, NULL, NULL));
    LONGS_EQUAL(0, hashtable_add_to_infolist (hashtable, NULL, NULL));
    LONGS_EQUAL(0, hashtable_add_to_infolist (hashtable, infolist_item, NULL));

    LONGS_EQUAL(1, hashtable_add_to_infolist (hashtable, infolist_item, "test"));

    infolist_reset_item_cursor (infolist);
    infolist_next (infolist);

    STRCMP_EQUAL("weechat", infolist_string (infolist, "test_name_00000"));
    STRCMP_EQUAL("the first item", infolist_string (infolist, "test_value_00000"));
    STRCMP_EQUAL("light", infolist_string (infolist, "test_name_00001"));
    STRCMP_EQUAL("item2", infolist_string (infolist, "test_value_00001"));
    STRCMP_EQUAL("fast", infolist_string (infolist, "test_name_00002"));
    STRCMP_EQUAL("item3", infolist_string (infolist, "test_value_00002"));
    STRCMP_EQUAL("extensible", infolist_string (infolist, "test_name_00003"));
    STRCMP_EQUAL("item4", infolist_string (infolist, "test_value_00003"));
    STRCMP_EQUAL("chat", infolist_string (infolist, "test_name_00004"));
    STRCMP_EQUAL("item5", infolist_string (infolist, "test_value_00004"));
    STRCMP_EQUAL("client", infolist_string (infolist, "test_name_00005"));
    STRCMP_EQUAL("last item", infolist_string (infolist, "test_value_00005"));
}

/*
 * Test functions:
 *   hashtable_print_log
 */

TEST(CoreHashtable, PrintLog)
{
    /* TODO: write tests */
}
