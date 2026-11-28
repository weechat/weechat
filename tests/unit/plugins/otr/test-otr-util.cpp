/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test OTR utility functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "src/core/core-string.h"
#include "src/core/weechat.h"
#include "src/plugins/otr/otr-util.h"
}

TEST_GROUP(OtrUtil)
{
    /*
     * Read the first line of a file.
     */
    void read_file (const char *filename, char *content, int size)
    {
        FILE *file;

        content[0] = '\0';
        file = fopen (filename, "r");
        CHECK(file);
        if (!fgets (content, size, file))
            content[0] = '\0';
        fclose (file);
    }
};

/*
 * Test functions:
 *   otr_util_wipe
 */

TEST(OtrUtil, Wipe)
{
    char buffer[8], zero[8];

    memset (zero, 0, sizeof (zero));

    otr_util_wipe (NULL, 0);
    otr_util_wipe (NULL, 8);

    memcpy (buffer, "secret!", 8);
    otr_util_wipe (buffer, 0);
    MEMCMP_EQUAL("secret!", buffer, 8);

    otr_util_wipe (buffer, 3);
    MEMCMP_EQUAL("\0\0\0ret!", buffer, 8);

    otr_util_wipe (buffer, sizeof (buffer));
    MEMCMP_EQUAL(zero, buffer, 8);
}

/*
 * Test functions:
 *   otr_util_free_secret
 */

TEST(OtrUtil, FreeSecret)
{
    otr_util_free_secret (NULL);
    otr_util_free_secret (strdup (""));
    otr_util_free_secret (strdup ("secret"));
}

/*
 * Test functions:
 *   otr_util_file_replace_start
 *   otr_util_file_replace_end
 */

TEST(OtrUtil, FileReplace)
{
    char *filename, *link, *target, *tmp_filename, *tmp_target, *abs_filename;
    char content[64];
    struct stat st;
    FILE *file;

    string_asprintf (&filename, "%s/test_otr_util_replace", weechat_data_dir);
    string_asprintf (&link, "%s/test_otr_util_replace_link", weechat_data_dir);
    CHECK(filename);
    CHECK(link);
    unlink (filename);
    unlink (link);

    /* Existing file (permissions 0644). */
    file = fopen (filename, "w");
    CHECK(file);
    fprintf (file, "first\n");
    fclose (file);
    chmod (filename, 0644);
    abs_filename = realpath (filename, NULL);
    CHECK(abs_filename);
    string_asprintf (&tmp_target, "%s.tmp", abs_filename);
    CHECK(tmp_target);

    /* Write error: file not replaced, temporary file removed. */
    file = otr_util_file_replace_start (filename, &target, &tmp_filename);
    CHECK(file);
    STRCMP_EQUAL(abs_filename, target);
    STRCMP_EQUAL(tmp_target, tmp_filename);
    fprintf (file, "second\n");
    LONGS_EQUAL(0, otr_util_file_replace_end (file, target, tmp_filename, 0));
    read_file (filename, content, sizeof (content));
    STRCMP_EQUAL("first\n", content);
    LONGS_EQUAL(-1, access (tmp_target, F_OK));

    /* Symbolic link: target replaced (permissions 0600), link kept. */
    LONGS_EQUAL(0, symlink (abs_filename, link));
    file = otr_util_file_replace_start (link, &target, &tmp_filename);
    CHECK(file);
    STRCMP_EQUAL(abs_filename, target);
    STRCMP_EQUAL(tmp_target, tmp_filename);
    fprintf (file, "new\n");
    LONGS_EQUAL(1, otr_util_file_replace_end (file, target, tmp_filename, 1));
    LONGS_EQUAL(0, lstat (link, &st));
    CHECK(S_ISLNK(st.st_mode));
    read_file (filename, content, sizeof (content));
    STRCMP_EQUAL("new\n", content);
    LONGS_EQUAL(0, stat (filename, &st));
    LONGS_EQUAL(S_IRUSR | S_IWUSR, st.st_mode & 0777);

    /* Directory does not exist: error. */
    target = (char *)0x1;
    tmp_filename = (char *)0x1;
    POINTERS_EQUAL(NULL,
                   otr_util_file_replace_start (
                       "/nonexistent_dir_otr_test/file", &target,
                       &tmp_filename));
    POINTERS_EQUAL(NULL, target);
    POINTERS_EQUAL(NULL, tmp_filename);

    unlink (link);
    unlink (filename);
    free (link);
    free (filename);
    free (abs_filename);
    free (tmp_target);
}

/*
 * Test functions:
 *   otr_util_dupstr
 */

TEST(OtrUtil, Dupstr)
{
    char *str;

    WEE_TEST_STR("", otr_util_dupstr (NULL));
    WEE_TEST_STR("", otr_util_dupstr (""));
    WEE_TEST_STR("abc", otr_util_dupstr ("abc"));
}

/*
 * Test functions:
 *   otr_util_irc_sanitize
 */

TEST(OtrUtil, IrcSanitize)
{
    char *str;

    WEE_TEST_STR("", otr_util_irc_sanitize (NULL));
    WEE_TEST_STR("", otr_util_irc_sanitize (""));
    WEE_TEST_STR("abc", otr_util_irc_sanitize ("abc"));
    WEE_TEST_STR("a b  c ", otr_util_irc_sanitize ("a\rb\r\nc\n"));
}

/*
 * Test functions:
 *   otr_util_peer_text
 */

TEST(OtrUtil, PeerText)
{
    char *str;

    WEE_TEST_STR("", otr_util_peer_text (NULL));
    WEE_TEST_STR("", otr_util_peer_text (""));
    WEE_TEST_STR("abc", otr_util_peer_text ("abc"));
    WEE_TEST_STR("question? SMP verification succeeded.",
                 otr_util_peer_text ("question?\n"
                                     "SMP verification succeeded."));
    WEE_TEST_STR("a b", otr_util_peer_text ("a\rb"));

    /* WeeChat color codes are removed. */
    WEE_TEST_STR("green text",
                 otr_util_peer_text ("\x19" "02" "green text"));
    WEE_TEST_STR("bold", otr_util_peer_text ("\x1A" "\x01" "bold"));
}
