/*
 * test-core-dir.cpp - test directory/file functions
 *
 * Copyright (C) 2014-2021 Sébastien Helleu <flashcode@flashtux.org>
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

TEST(CoreDir, LibName)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   dir_file_get_content
 */

TEST(CoreDir, FileGetContent)
{
    char *path, *content, *content_read;
    const char *content_small = "line 1\nline 2\nend";
    int length, i;
    FILE *f;

    /* file not found */
    POINTERS_EQUAL(NULL, dir_file_get_content (NULL));
    POINTERS_EQUAL(NULL, dir_file_get_content (""));
    POINTERS_EQUAL(NULL, dir_file_get_content ("/tmp/does/not/exist.xyz"));

    path = string_eval_path_home ("%h/test_file.txt", NULL, NULL, NULL);

    /* small file */
    length = strlen (content_small);
    f = fopen (path, "wb");
    CHECK(f);
    LONGS_EQUAL(length, fwrite (content_small, 1, length, f));
    fclose (f);
    content_read = dir_file_get_content (path);
    STRCMP_EQUAL(content_small, content_read);
    free (content_read);
    unlink (path);

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
    f = fopen (path, "wb");
    CHECK(f);
    LONGS_EQUAL(length, fwrite (content, 1, length, f));
    fclose (f);
    content_read = dir_file_get_content (path);
    STRCMP_EQUAL(content, content_read);
    free (content_read);
    unlink (path);
    free (content);
}
