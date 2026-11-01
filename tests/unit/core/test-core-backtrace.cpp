/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test backtrace functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "src/core/core-backtrace.h"
#include "src/core/core-string.h"
}

TEST_GROUP(CoreBacktrace)
{
};

/*
 * Test functions:
 *   weechat_backtrace_find_file
 */

TEST(CoreBacktrace, FindFile)
{
    char path[4096], small_path[8], dir[] = "/tmp/weechat_test_bt_XXXXXX";
    char file_exec[4096], file_noexec[4096], env_path[16384];
    const char *ptr_old_path;
    char *old_path;
    FILE *file;

    /* Invalid arguments. */
    LONGS_EQUAL(0, weechat_backtrace_find_file ("sh", NULL, 0));
    LONGS_EQUAL(0, weechat_backtrace_find_file ("sh", path, 0));
    strcpy (path, "test");
    LONGS_EQUAL(0, weechat_backtrace_find_file (NULL, path, sizeof (path)));
    STRCMP_EQUAL("", path);
    strcpy (path, "test");
    LONGS_EQUAL(0, weechat_backtrace_find_file ("", path, sizeof (path)));
    STRCMP_EQUAL("", path);

    /* File with a "/": used as-is, even if it does not exist. */
    LONGS_EQUAL(1, weechat_backtrace_find_file ("/path/to/lib.so",
                                                path, sizeof (path)));
    STRCMP_EQUAL("/path/to/lib.so", path);
    LONGS_EQUAL(1, weechat_backtrace_find_file ("./weechat",
                                                path, sizeof (path)));
    STRCMP_EQUAL("./weechat", path);

    /* File with a "/", path too small. */
    LONGS_EQUAL(0, weechat_backtrace_find_file ("/path/to/lib.so",
                                                small_path,
                                                sizeof (small_path)));
    STRCMP_EQUAL("", small_path);

    /* Search in PATH: create an executable file and a non-executable one. */
    CHECK(mkdtemp (dir));
    snprintf (file_exec, sizeof (file_exec), "%s/test_exec", dir);
    snprintf (file_noexec, sizeof (file_noexec), "%s/test_noexec", dir);
    file = fopen (file_exec, "w");
    CHECK(file);
    fclose (file);
    chmod (file_exec, 0700);
    file = fopen (file_noexec, "w");
    CHECK(file);
    fclose (file);
    chmod (file_noexec, 0600);

    ptr_old_path = getenv ("PATH");
    old_path = (ptr_old_path) ? strdup (ptr_old_path) : NULL;

    snprintf (env_path, sizeof (env_path),
              "/nonexistent_dir_weechat:%s:/usr/bin", dir);
    setenv ("PATH", env_path, 1);
    LONGS_EQUAL(1, weechat_backtrace_find_file ("test_exec",
                                                path, sizeof (path)));
    STRCMP_EQUAL(file_exec, path);
    strcpy (path, "test");
    LONGS_EQUAL(0, weechat_backtrace_find_file ("test_noexec",
                                                path, sizeof (path)));
    STRCMP_EQUAL("", path);
    LONGS_EQUAL(0, weechat_backtrace_find_file ("test_does_not_exist",
                                                path, sizeof (path)));
    STRCMP_EQUAL("", path);

    /* Search in PATH, directory itself is not a regular file. */
    setenv ("PATH", "/", 1);
    LONGS_EQUAL(0, weechat_backtrace_find_file ("tmp", path, sizeof (path)));
    STRCMP_EQUAL("", path);

    /* Search in PATH, path too small. */
    setenv ("PATH", env_path, 1);
    LONGS_EQUAL(0, weechat_backtrace_find_file ("test_exec",
                                                small_path,
                                                sizeof (small_path)));
    STRCMP_EQUAL("", small_path);

    /* Empty PATH. */
    setenv ("PATH", "", 1);
    LONGS_EQUAL(0, weechat_backtrace_find_file ("test_exec",
                                                path, sizeof (path)));

    /* No PATH. */
    unsetenv ("PATH");
    LONGS_EQUAL(0, weechat_backtrace_find_file ("test_exec",
                                                path, sizeof (path)));

    if (old_path)
    {
        setenv ("PATH", old_path, 1);
        free (old_path);
    }

    unlink (file_exec);
    unlink (file_noexec);
    rmdir (dir);
}

/*
 * Test functions:
 *   weechat_backtrace_addr2line
 *   weechat_backtrace
 */

TEST(CoreBacktrace, Backtrace)
{
    /* TODO: write tests */
}
