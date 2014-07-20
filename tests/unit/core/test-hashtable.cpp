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
#include "../src/core/wee-hashtable.h"
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
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hashtable_new
 */

TEST(Hashtable, New)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hashtable_set_with_size
 *   hashtable_set
 */

TEST(Hashtable, Set)
{
    /* TODO: write tests */
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
