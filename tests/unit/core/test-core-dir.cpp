/*
 * test-core-dir.cpp - test directory/file functions
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
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "src/core/wee-dir.h"
#include "src/core/wee-string.h"
}

TEST_GROUP(CoreDir)
{
};

/*
 * Tests functions:
 *   dir_get_temp_dir
 */

TEST(CoreDir, GetTempDir)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   dir_mkdir_home
 *   dir_mkdir
 *   dir_mkdir_parents
 */

TEST(CoreDir, Mkdir)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   dir_rmtree
 */

TEST(CoreDir, Rmtree)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   dir_exec_on_files
 */

TEST(CoreDir, ExecOnFiles)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   dir_search_full_lib_name
 */

TEST(CoreDir, SearchFullLibName)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   dir_file_get_content
 *   dir_file_copy
 */

TEST(CoreDir, FileGetContentCopy)
{
    char *path1, *path2, *content, *content_read1, *content_read2;
    const char *content_small = "line 1\nline 2\nend";
    int length, i;
    FILE *f;

    /* file not found */
    LONGS_EQUAL(0, dir_file_copy (NULL, NULL));
    LONGS_EQUAL(0, dir_file_copy ("", ""));
    LONGS_EQUAL(0, dir_file_copy ("/tmp/does/not/exist.xyz", "/tmp/test.txt"));

    path1 = string_eval_path_home ("${weechat_data_dir}/test_file.txt",
                                   NULL, NULL, NULL);
    path2 = string_eval_path_home ("${weechat_data_dir}/test_file2.txt",
                                   NULL, NULL, NULL);

    /* small file */
    length = strlen (content_small);
    f = fopen (path1, "wb");
    CHECK(f);
    LONGS_EQUAL(length, fwrite (content_small, 1, length, f));
    fclose (f);
    LONGS_EQUAL(1, dir_file_copy (path1, path2));
    content_read1 = dir_file_get_content (path1);
    content_read2 = dir_file_get_content (path2);
    STRCMP_EQUAL(content_small, content_read1);
    MEMCMP_EQUAL(content_read1, content_read2, length);
    free (content_read1);
    free (content_read2);
    unlink (path1);
    unlink (path2);

    /* bigger file: 26 lines of 5000 bytes */
    length = 26 * 5001;
    content = (char *)malloc (length + 1);
    CHECK(content);
    for (i = 0; i < 26; i++)
    {
        memset (content + (i * 5001), 'a' + i, 5000);
        content[(i * 5001) + 5000] = '\n';
    }
    content[26 * 5001] = '\0';
    f = fopen (path1, "wb");
    CHECK(f);
    LONGS_EQUAL(length, fwrite (content, 1, length, f));
    fclose (f);
    LONGS_EQUAL(1, dir_file_copy (path1, path2));
    content_read1 = dir_file_get_content (path1);
    content_read2 = dir_file_get_content (path2);
    STRCMP_EQUAL(content, content_read1);
    MEMCMP_EQUAL(content_read1, content_read2, length);
    free (content_read1);
    free (content_read2);
    unlink (path1);
    unlink (path2);
    free (content);
}

/*
 * Tests functions:
 *   dir_set_home_path
 */

TEST(CoreDir, SetHomePath)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   dir_create_home_temp_dir
 */

TEST(CoreDir, CreateHomeTempDir)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   dir_find_xdg_dirs
 */

TEST(CoreDir, FindXdgDirs)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   dir_find_home_dirs
 */

TEST(CoreDir, FindHomeDirs)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   dir_create_home_dir
 */

TEST(CoreDir, CreateHomeDir)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   dir_create_home_dirs
 */

TEST(CoreDir, CreateHomeDirs)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   dir_remove_home_dirs
 */

TEST(CoreDir, RemoveHomeDirs)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   dir_get_string_home_dirs
 */

TEST(CoreDir, GetStringHomeDirs)
{
    /* TODO: write tests */
}
