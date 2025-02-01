/*
 * test-xfer-file.cpp - test xfer file functions
 *
 * Copyright (C) 2022-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/plugins/xfer/xfer-file.h"
}

TEST_GROUP(XferFile)
{
};

/*
 * Tests functions:
 *   xfer_file_search_crc32
 */

TEST(XferFile, SearchCrc32)
{
    STRCMP_EQUAL(NULL, xfer_file_search_crc32 (NULL));
    STRCMP_EQUAL(NULL, xfer_file_search_crc32 (""));
    STRCMP_EQUAL(NULL, xfer_file_search_crc32 ("a"));
    STRCMP_EQUAL(NULL, xfer_file_search_crc32 ("z"));
    STRCMP_EQUAL(NULL, xfer_file_search_crc32 ("123456781234abcd"));
    STRCMP_EQUAL(NULL, xfer_file_search_crc32 ("test_filename"));

    /* valid CRC32 */
    STRCMP_EQUAL("1234abcd", xfer_file_search_crc32 ("test_1234abcd"));
    STRCMP_EQUAL("1234aBCd", xfer_file_search_crc32 ("test_1234aBCd"));
    STRCMP_EQUAL("1234abcd_test", xfer_file_search_crc32 ("1234abcd_test"));
    STRCMP_EQUAL("1234Abcd_test", xfer_file_search_crc32 ("1234Abcd_test"));
    STRCMP_EQUAL("12345678", xfer_file_search_crc32 ("1234abcd_12345678"));
}

/*
 * Tests functions:
 *   xfer_file_resume
 */

TEST(XferFile, Resume)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   xfer_file_check_suffix
 */

TEST(XferFile, CheckSuffix)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   xfer_file_find_suffix
 */

TEST(XferFile, FindSuffix)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   xfer_file_find_filename
 */

TEST(XferFile, FindFilename)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   xfer_file_calculate_speed
 */

TEST(XferFile, CalculateSpeed)
{
    /* TODO: write tests */
}
