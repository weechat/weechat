/*
 * test-infolist.cpp - test infolist functions
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
#include "src/core/wee-infolist.h"
}

TEST_GROUP(Infolist)
{
};

/*
 * Tests functions:
 *   infolist_new
 *   infolist_new_item
 *   infolist_new_var_integer
 *   infolist_new_var_string
 *   infolist_new_var_pointer
 *   infolist_new_var_buffer
 *   infolist_new_var_time
 */

TEST(Infolist, New)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   infolist_valid
 */

TEST(Infolist, Valid)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   infolist_search_var
 */

TEST(Infolist, Search)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   infolist_next
 *   infolist_prev
 *   infolist_reset_item_cursor
 */

TEST(Infolist, Move)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   infolist_integer
 *   infolist_string
 *   infolist_pointer
 *   infolist_buffer
 *   infolist_time
 */

TEST(Infolist, Get)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   infolist_free
 *   infolist_free_all_plugin
 */

TEST(Infolist, Free)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   infolist_print_log
 */

TEST(Infolist, PrintLog)
{
    /* TODO: write tests */
}
