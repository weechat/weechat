/*
 * SPDX-FileCopyrightText: 2022-2026 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Test xfer file functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include "src/core/core-config-file.h"
#include "src/plugins/xfer/xfer.h"
#include "src/plugins/xfer/xfer-config.h"
#include "src/plugins/xfer/xfer-file.h"
}

TEST_GROUP(XferFile)
{
};

/*
 * Build a "file recv" xfer with the given remote nick (and a fixed filename),
 * call xfer_file_find_filename and return a copy of the basename of the local
 * filename (the part after the last directory separator).
 *
 * Note: result must be freed after use.
 */

static char *
test_find_filename_basename (const char *remote_nick)
{
    struct t_xfer xfer;
    char *pos, *result;

    memset (&xfer, 0, sizeof (xfer));
    xfer.type = XFER_TYPE_FILE_RECV_ACTIVE;
    xfer.remote_nick = strdup (remote_nick);
    xfer.filename = strdup ("test.txt");

    xfer_file_find_filename (&xfer);

    result = NULL;
    if (xfer.local_filename)
    {
        pos = strrchr (xfer.local_filename, DIR_SEPARATOR_CHAR);
        result = strdup ((pos) ? pos + 1 : xfer.local_filename);
    }

    free (xfer.remote_nick);
    free (xfer.filename);
    free (xfer.local_filename);
    free (xfer.temp_local_filename);

    return result;
}

/*
 * Test functions:
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
 * Test functions:
 *   xfer_file_resume
 */

TEST(XferFile, Resume)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   xfer_file_check_suffix
 */

TEST(XferFile, CheckSuffix)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   xfer_file_find_suffix
 */

TEST(XferFile, FindSuffix)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   xfer_file_find_filename
 */

TEST(XferFile, FindFilename)
{
    char *basename;

    config_file_option_set (xfer_config_file_download_path, "/tmp/weechat_test_xfer", 1);

    /* remote nick without directory separator: used as-is */
    basename = test_find_filename_basename ("alice");
    STRCMP_EQUAL("alice.test.txt", basename);
    free (basename);

    /*
     * remote nick with a directory separator: the separator is replaced by
     * "_" so the nick cannot make the file be written outside the download
     * directory
     */
    basename = test_find_filename_basename ("../foo");
    STRCMP_EQUAL(".._foo.test.txt", basename);
    free (basename);

    /* all directory separators in the nick are replaced */
    basename = test_find_filename_basename ("a/b/c");
    STRCMP_EQUAL("a_b_c.test.txt", basename);
    free (basename);

    config_file_option_unset (xfer_config_file_download_path);
}

/*
 * Test functions:
 *   xfer_file_calculate_speed
 */

TEST(XferFile, CalculateSpeed)
{
    /* TODO: write tests */
}
