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
#include "../src/core/wee-hashtable.h"
#include "../src/plugins/plugin.h"
}

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

    hash = hashtable_hash_key_djb2 ("test");
    CHECK(hash == 5849825121ULL);
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
 */

TEST(Hashtable, Set)
{
    struct t_hashtable *hashtable;
    struct t_hashtable_item *item;
    const char *value = "this is a string";

    hashtable = hashtable_new (32,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               &test_hashtable_hash_key_cb,
                               &test_hashtable_keycmp_cb);
    POINTERS_EQUAL(NULL, hashtable_set_with_size (NULL, NULL, -1, NULL, -1));
    POINTERS_EQUAL(NULL, hashtable_set_with_size (NULL, NULL, -1, NULL, -1));

    /* TODO: write more tests */

    hashtable_free (hashtable);
}

/*
 * Tests functions:
 *   hashtable_get_item
 *   hashtable_get
 *   hashtable_has_key
 */

TEST(Hashtable, Get)
{
    /* TODO: write tests */
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
 *   hashtable_dup
 */

TEST(Hashtable, Dup)
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
 *   hashtable_remove
 *   hashtable_remove_all
 *   hashtable_free
 */

TEST(Hashtable, Free)
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
