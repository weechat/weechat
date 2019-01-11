/*
 * test-core-hdata.cpp - test hdata functions
 *
 * Copyright (C) 2014-2019 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/core/wee-hdata.h"
}

TEST_GROUP(CoreHdata)
{
};

/*
 * Tests functions:
 *   hdata_new
 *   hdata_new_var
 *   hdata_new_list
 */

TEST(CoreHdata, New)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hdata_get_var_offset
 *   hdata_get_var_type
 *   hdata_get_var_type_string
 *   hdata_get_var_array_size
 *   hdata_get_var_array_size_string
 *   hdata_get_var_hdata
 *   hdata_get_var
 *   hdata_get_var_at_offset
 *   hdata_get_list
 */

TEST(CoreHdata, Get)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hdata_check_pointer
 */

TEST(CoreHdata, Check)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hdata_move
 *   hdata_search
 */

TEST(CoreHdata, Move)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hdata_char
 *   hdata_integer
 *   hdata_long
 *   hdata_string
 *   hdata_pointer
 *   hdata_time
 */

TEST(CoreHdata, Read)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hdata_free_all_plugin
 *   hdata_free_all
 */

TEST(CoreHdata, Free)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   hdata_print_log
 */

TEST(CoreHdata, PrintLog)
{
    /* TODO: write tests */
}
