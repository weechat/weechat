/*
 * test-list.cpp - test list functions
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
#include "src/core/wee-list.h"
}

TEST_GROUP(List)
{
};

/*
 * Tests functions:
 *   weelist_new
 */

TEST(List, New)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   weelist_add
 */

TEST(List, Add)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   weelist_search
 *   weelist_search_pos
 *   weelist_casesearch
 *   weelist_casesearch_pos
 */

TEST(List, Search)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   weelist_get
 *   weelist_string
 */

TEST(List, Get)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   weelist_set
 */

TEST(List, Set)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   weelist_next
 *   weelist_prev
 */

TEST(List, Move)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   weelist_remove
 *   weelist_remove_all
 *   weelist_free
 */

TEST(List, Free)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   weelist_print_log
 */

TEST(List, PrintLog)
{
    /* TODO: write tests */
}
