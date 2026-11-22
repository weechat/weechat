/*
 * SPDX-FileCopyrightText: 2014-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test directory/file functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "src/core/core-dir.h"
#include "src/core/core-string.h"

extern void dir_remove_trailing_separators (char *path);
}

TEST_GROUP(CoreDir)
{
};

/*
 * Test functions:
 *   dir_get_temp_dir
 */

TEST(CoreDir, GetTempDir)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   dir_mkdir_home
 *   dir_mkdir
 *   dir_mkdir_parents
 */

TEST(CoreDir, Mkdir)
{
    char *path, *path_file, *path_sub, *path_file_sub;
    struct stat st;
    FILE *file;

    path = string_eval_path_home ("${weechat_data_dir}/test_mkdir",
                                  NULL, NULL, NULL);
    CHECK(path);
    dir_rmtree (path);
    string_asprintf (&path_sub, "%s/a/b", path);
    string_asprintf (&path_file, "%s/file", path);
    string_asprintf (&path_file_sub, "%s/file/sub", path);
    CHECK(path_sub);
    CHECK(path_file);
    CHECK(path_file_sub);

    LONGS_EQUAL(0, dir_mkdir_home (NULL, 0700));
    LONGS_EQUAL(0, dir_mkdir (NULL, 0700));
    LONGS_EQUAL(0, dir_mkdir_parents (NULL, 0700));

    /* Create directory, then directory already exists. */
    LONGS_EQUAL(1, dir_mkdir (path, 0700));
    LONGS_EQUAL(0, stat (path, &st));
    CHECK(S_ISDIR(st.st_mode));
    LONGS_EQUAL(1, dir_mkdir (path, 0700));

    /* Create directory in WeeChat home, then directory already exists. */
    LONGS_EQUAL(1, dir_mkdir_home ("test_mkdir/home", 0700));
    LONGS_EQUAL(1, dir_mkdir_home ("${weechat_data_dir}/test_mkdir/home",
                                   0700));
    LONGS_EQUAL(1, dir_mkdir_home ("test_mkdir/home", 0700));

    /* Create directory and parent directories. */
    LONGS_EQUAL(1, dir_mkdir_parents (path_sub, 0700));
    LONGS_EQUAL(0, stat (path_sub, &st));
    CHECK(S_ISDIR(st.st_mode));

    /* Directory already exists. */
    LONGS_EQUAL(1, dir_mkdir_parents (path_sub, 0700));

    /* A file exists with the name of the directory or of a parent. */
    file = fopen (path_file, "w");
    CHECK(file);
    fclose (file);
    LONGS_EQUAL(0, dir_mkdir_home ("test_mkdir/file", 0700));
    LONGS_EQUAL(0, dir_mkdir_home ("${weechat_data_dir}/test_mkdir/file",
                                   0700));
    LONGS_EQUAL(0, dir_mkdir (path_file, 0700));
    LONGS_EQUAL(0, dir_mkdir_parents (path_file, 0700));
    LONGS_EQUAL(0, dir_mkdir_parents (path_file_sub, 0700));
    LONGS_EQUAL(0, stat (path_file, &st));
    CHECK(S_ISREG(st.st_mode));

    dir_rmtree (path);
    free (path);
    free (path_sub);
    free (path_file);
    free (path_file_sub);
}

/*
 * Test functions:
 *   dir_unlink_cb
 *   dir_rmtree
 */

TEST(CoreDir, Rmtree)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   dir_set_home_path
 */

TEST(CoreDir, SetHomePath)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   dir_create_home_temp_dir
 */

TEST(CoreDir, CreateHomeTempDir)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   dir_find_xdg_dirs
 */

TEST(CoreDir, FindXdgDirs)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   dir_find_home_dirs
 */

TEST(CoreDir, FindHomeDirs)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   dir_remove_trailing_separators
 */

TEST(CoreDir, RemoveTrailingSeparators)
{
    char path[128];

    dir_remove_trailing_separators (NULL);

    path[0] = '\0';
    dir_remove_trailing_separators (path);
    STRCMP_EQUAL("", path);

    snprintf (path, sizeof (path), "/");
    dir_remove_trailing_separators (path);
    STRCMP_EQUAL("/", path);

    snprintf (path, sizeof (path), "///");
    dir_remove_trailing_separators (path);
    STRCMP_EQUAL("/", path);

    snprintf (path, sizeof (path), "/tmp");
    dir_remove_trailing_separators (path);
    STRCMP_EQUAL("/tmp", path);

    snprintf (path, sizeof (path), "/tmp/");
    dir_remove_trailing_separators (path);
    STRCMP_EQUAL("/tmp", path);

    snprintf (path, sizeof (path), "/tmp///////");
    dir_remove_trailing_separators (path);
    STRCMP_EQUAL("/tmp", path);
}

/*
 * Test functions:
 *   dir_create_home_dir
 */

TEST(CoreDir, CreateHomeDir)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   dir_create_home_dirs
 */

TEST(CoreDir, CreateHomeDirs)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   dir_remove_home_dirs
 */

TEST(CoreDir, RemoveHomeDirs)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   dir_get_string_home_dirs
 */

TEST(CoreDir, GetStringHomeDirs)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   dir_exec_on_files
 */

TEST(CoreDir, ExecOnFiles)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   dir_search_full_lib_name_ext
 *   dir_search_full_lib_name
 */

TEST(CoreDir, SearchFullLibName)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   dir_file_get_content
 *   dir_file_copy
 */

TEST(CoreDir, FileGetContentCopy)
{
    char *path1, *path2, *content, *content_read1, *content_read2;
    const char *content_small = "line 1\nline 2\nend";
    int length, i;
    FILE *f;

    /* File not found */
    LONGS_EQUAL(0, dir_file_copy (NULL, NULL));
    LONGS_EQUAL(0, dir_file_copy ("", ""));
    LONGS_EQUAL(0, dir_file_copy ("/tmp/does/not/exist.xyz", "/tmp/test.txt"));

    path1 = string_eval_path_home ("${weechat_data_dir}/test_content_file1.txt",
                                   NULL, NULL, NULL);
    path2 = string_eval_path_home ("${weechat_data_dir}/test_content_file2.txt",
                                   NULL, NULL, NULL);

    /* Small file */
    length = strlen (content_small);
    f = fopen (path1, "wb");
    fwrite (content_small, 1, length, f);
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

    /* Bigger file: 26 lines of 5000 bytes */
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
    fwrite (content, 1, length, f);
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
 * Test functions:
 *   dir_file_compress_gzip
 */

TEST(CoreDir, FileCompressGzip)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   dir_file_compress_zstd
 */

TEST(CoreDir, FileCompressZstd)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   dir_file_compress
 */

TEST(CoreDir, FileCompress)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   dir_file_compare
 */

TEST(CoreDir, FileCompare)
{
    char *path1, *path2, *content;
    const char *content_small = "line 1\nline 2\nend";
    int length, i;
    FILE *f;

    /* File not found */
    LONGS_EQUAL(2, dir_file_compare (NULL, NULL));
    LONGS_EQUAL(2, dir_file_compare ("", ""));
    LONGS_EQUAL(2, dir_file_compare ("/tmp/does/not/exist1.xyz", "/tmp/does/not/exist2.xyz"));

    path1 = string_eval_path_home ("${weechat_data_dir}/test_compare_file1.txt",
                                   NULL, NULL, NULL);
    path2 = string_eval_path_home ("${weechat_data_dir}/test_compare_file2.txt",
                                   NULL, NULL, NULL);

    /* Small file */
    length = strlen (content_small);
    f = fopen (path1, "wb");
    fwrite (content_small, 1, length, f);
    fclose (f);
    f = fopen (path2, "wb");
    fwrite (content_small, 1, length, f);
    fclose (f);
    LONGS_EQUAL(0, dir_file_compare (path1, path2));
    f = fopen (path1, "ab");
    fwrite ("A", 1, 1, f);
    fclose (f);
    f = fopen (path2, "ab");
    fwrite ("B", 1, 1, f);
    fclose (f);
    LONGS_EQUAL(1, dir_file_compare (path1, path2));

    /* Bigger file: 26 lines of 5000 bytes + 1 byte */
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
    fwrite (content, 1, length, f);
    fclose (f);
    f = fopen (path2, "wb");
    fwrite (content, 1, length, f);
    fclose (f);
    LONGS_EQUAL(0, dir_file_compare (path1, path2));
    f = fopen (path1, "ab");
    fwrite ("A", 1, 1, f);
    fclose (f);
    f = fopen (path2, "ab");
    fwrite ("B", 1, 1, f);
    fclose (f);
    LONGS_EQUAL(1, dir_file_compare (path1, path2));

    unlink (path1);
    unlink (path2);
}
