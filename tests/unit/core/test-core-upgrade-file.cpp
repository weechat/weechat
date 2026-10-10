/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test upgrade file functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <string.h>
#include "src/core/core-infolist.h"
#include "src/core/core-upgrade-file.h"
}

#define TEST_UPGRADE_FILE "test_upgrade_file"

int test_upgrade_file_read_count = 0;
int test_upgrade_file_read_object_id = 0;
int test_upgrade_file_read_integer = 0;
char test_upgrade_file_read_string[64];

TEST_GROUP(CoreUpgradeFile)
{
    /*
     * Read an object from the upgrade file (callback used in tests).
     */

    static int
    test_upgrade_file_read_cb (const void *pointer, void *data,
                               struct t_upgrade_file *upgrade_file,
                               int object_id,
                               struct t_infolist *infolist)
    {
        const char *ptr_string;

        /* Make C++ compiler happy. */
        (void) pointer;
        (void) data;
        (void) upgrade_file;

        test_upgrade_file_read_count++;
        test_upgrade_file_read_object_id = object_id;
        if (infolist_next (infolist))
        {
            test_upgrade_file_read_integer = infolist_integer (infolist,
                                                               "integer");
            ptr_string = infolist_string (infolist, "string");
            snprintf (test_upgrade_file_read_string,
                      sizeof (test_upgrade_file_read_string),
                      "%s", (ptr_string) ? ptr_string : "");
        }

        return 1;
    }

    void setup()
    {
        test_upgrade_file_read_count = 0;
        test_upgrade_file_read_object_id = 0;
        test_upgrade_file_read_integer = 0;
        test_upgrade_file_read_string[0] = '\0';
    }
};

/*
 * Test functions:
 *   upgrade_file_new
 *   upgrade_file_write_object
 *   upgrade_file_read
 *   upgrade_file_close
 */

TEST(CoreUpgradeFile, InvalidArguments)
{
    struct t_infolist *infolist;
    struct t_upgrade_file *upgrade_file;

    infolist = infolist_new (NULL);
    CHECK(infolist);
    CHECK(infolist_new_item (infolist));

    POINTERS_EQUAL(NULL, upgrade_file_new (NULL, NULL, NULL, NULL));

    /* Read mode on a file that does not exist. */
    POINTERS_EQUAL(NULL,
                   upgrade_file_new ("test_upgrade_file_missing",
                                     &test_upgrade_file_read_cb, NULL, NULL));

    LONGS_EQUAL(0, upgrade_file_write_object (NULL, 1, NULL));
    LONGS_EQUAL(0, upgrade_file_write_object (NULL, 1, infolist));

    upgrade_file = upgrade_file_new (TEST_UPGRADE_FILE, NULL, NULL, NULL);
    CHECK(upgrade_file);
    LONGS_EQUAL(0, upgrade_file_write_object (upgrade_file, 1, NULL));
    upgrade_file_close (upgrade_file);

    /* No read callback (file opened in write mode). */
    LONGS_EQUAL(0, upgrade_file_read (NULL));
    upgrade_file = upgrade_file_new (TEST_UPGRADE_FILE, NULL, NULL, NULL);
    CHECK(upgrade_file);
    LONGS_EQUAL(0, upgrade_file_read (upgrade_file));
    upgrade_file_close (upgrade_file);

    upgrade_file_close (NULL);

    infolist_free (infolist);
}

/*
 * Test functions:
 *   upgrade_file_new
 *   upgrade_file_write_object
 *   upgrade_file_read
 *   upgrade_file_close
 */

TEST(CoreUpgradeFile, WriteRead)
{
    struct t_infolist *infolist;
    struct t_infolist_item *ptr_item;
    struct t_upgrade_file *upgrade_file;

    infolist = infolist_new (NULL);
    CHECK(infolist);
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_integer (ptr_item, "integer", 123));
    CHECK(infolist_new_var_string (ptr_item, "string", "test string"));

    /* Write the upgrade file. */
    upgrade_file = upgrade_file_new (TEST_UPGRADE_FILE, NULL, NULL, NULL);
    CHECK(upgrade_file);
    LONGS_EQUAL(1, upgrade_file_write_object (upgrade_file, 42, infolist));
    upgrade_file_close (upgrade_file);

    infolist_free (infolist);

    /* Read the upgrade file. */
    upgrade_file = upgrade_file_new (TEST_UPGRADE_FILE,
                                     &test_upgrade_file_read_cb, NULL, NULL);
    CHECK(upgrade_file);
    LONGS_EQUAL(1, upgrade_file_read (upgrade_file));
    upgrade_file_close (upgrade_file);

    LONGS_EQUAL(1, test_upgrade_file_read_count);
    LONGS_EQUAL(42, test_upgrade_file_read_object_id);
    LONGS_EQUAL(123, test_upgrade_file_read_integer);
    STRCMP_EQUAL("test string", test_upgrade_file_read_string);
}
