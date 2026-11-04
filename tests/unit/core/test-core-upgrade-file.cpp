/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test upgrade file functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "src/core/weechat.h"
#include "src/core/core-infolist.h"
#include "src/core/core-string.h"
#include "src/core/core-upgrade-file.h"
#include "src/plugins/weechat-plugin.h"
}

#define TEST_UPGRADE_FILE "test_upgrade_file"

int test_upgrade_file_read_count = 0;
int test_upgrade_file_read_object_id = 0;
int test_upgrade_file_read_integer = 0;
char test_upgrade_file_read_string[64];
char test_upgrade_file_read_log[4096];
int test_upgrade_file_read_rc = WEECHAT_RC_OK;

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

    /*
     * Read an object from the upgrade file and append all its variables
     * to the log (callback used in tests).
     */

    static int
    test_upgrade_file_read_log_cb (const void *pointer, void *data,
                                   struct t_upgrade_file *upgrade_file,
                                   int object_id,
                                   struct t_infolist *infolist)
    {
        char **argv, str_value[1024], str_hexa[3];
        const char *ptr_string;
        unsigned char *ptr_buffer;
        int i, j, argc, size;

        /* Make C++ compiler happy. */
        (void) pointer;
        (void) data;
        (void) upgrade_file;

        test_upgrade_file_read_count++;

        snprintf (str_value, sizeof (str_value), "[%d]", object_id);
        strcat (test_upgrade_file_read_log, str_value);

        if (!infolist_next (infolist))
            return test_upgrade_file_read_rc;

        argv = string_split (infolist_fields (infolist), ",", NULL, 0, 0,
                             &argc);
        for (i = 0; i < argc; i++)
        {
            switch (argv[i][0])
            {
                case 'i':
                    snprintf (str_value, sizeof (str_value), " %s=%d",
                              argv[i] + 2,
                              infolist_integer (infolist, argv[i] + 2));
                    break;
                case 's':
                    ptr_string = infolist_string (infolist, argv[i] + 2);
                    snprintf (str_value, sizeof (str_value), " %s=%s",
                              argv[i] + 2,
                              (ptr_string) ? ptr_string : "(null)");
                    break;
                case 'b':
                    ptr_buffer = (unsigned char *)infolist_buffer (
                        infolist, argv[i] + 2, &size);
                    snprintf (str_value, sizeof (str_value), " %s=",
                              argv[i] + 2);
                    for (j = 0; j < size; j++)
                    {
                        snprintf (str_hexa, sizeof (str_hexa),
                                  "%02x", ptr_buffer[j]);
                        strcat (str_value, str_hexa);
                    }
                    break;
                case 't':
                    snprintf (str_value, sizeof (str_value), " %s=%lld",
                              argv[i] + 2,
                              (long long)infolist_time (infolist,
                                                        argv[i] + 2));
                    break;
                case 'l':
                    snprintf (str_value, sizeof (str_value), " %s=%ld",
                              argv[i] + 2,
                              infolist_long (infolist, argv[i] + 2));
                    break;
                case 'L':
                    snprintf (str_value, sizeof (str_value), " %s=%lld",
                              argv[i] + 2,
                              infolist_longlong (infolist, argv[i] + 2));
                    break;
                default:
                    snprintf (str_value, sizeof (str_value), " %s=?",
                              argv[i]);
                    break;
            }
            strcat (test_upgrade_file_read_log, str_value);
        }
        string_free_split (argv);

        return test_upgrade_file_read_rc;
    }

    /*
     * Read the upgrade file with the log callback.
     *
     * Return the value returned by upgrade_file_read.
     */

    static int
    test_upgrade_file_read_with_log ()
    {
        struct t_upgrade_file *upgrade_file;
        int rc;

        upgrade_file = upgrade_file_new (TEST_UPGRADE_FILE,
                                         &test_upgrade_file_read_log_cb,
                                         NULL, NULL);
        CHECK(upgrade_file);
        rc = upgrade_file_read (upgrade_file);
        upgrade_file_close (upgrade_file);

        return rc;
    }

    /*
     * Write raw data (without signature) in the upgrade file.
     */

    static void
    test_upgrade_file_write_raw (const void *data, size_t size)
    {
        char *filename;
        FILE *file;

        CHECK(string_asprintf (&filename, "%s/%s.upgrade",
                               weechat_data_dir, TEST_UPGRADE_FILE) >= 0);
        file = fopen (filename, "wb");
        CHECK(file);
        if (size > 0)
            LONGS_EQUAL(1, fwrite (data, size, 1, file));
        fclose (file);
        free (filename);
    }

    /*
     * Truncate the upgrade file to the given size.
     */

    static void
    test_upgrade_file_truncate (long size)
    {
        char *filename;

        CHECK(string_asprintf (&filename, "%s/%s.upgrade",
                               weechat_data_dir, TEST_UPGRADE_FILE) >= 0);
        LONGS_EQUAL(0, truncate (filename, size));
        free (filename);
    }

    void setup()
    {
        test_upgrade_file_read_count = 0;
        test_upgrade_file_read_object_id = 0;
        test_upgrade_file_read_integer = 0;
        test_upgrade_file_read_string[0] = '\0';
        test_upgrade_file_read_log[0] = '\0';
        test_upgrade_file_read_rc = WEECHAT_RC_OK;
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

/*
 * Test functions:
 *   upgrade_file_write_object
 *   upgrade_file_write_integer
 *   upgrade_file_write_string
 *   upgrade_file_write_buffer
 *   upgrade_file_write_time
 *   upgrade_file_write_long
 *   upgrade_file_write_longlong
 *   upgrade_file_read
 *   upgrade_file_read_object
 *   upgrade_file_read_integer
 *   upgrade_file_read_string
 *   upgrade_file_read_buffer
 *   upgrade_file_read_time
 *   upgrade_file_read_long
 *   upgrade_file_read_longlong
 */

TEST(CoreUpgradeFile, WriteReadAllTypes)
{
    struct t_infolist *infolist;
    struct t_infolist_item *ptr_item;
    struct t_upgrade_file *upgrade_file;
    unsigned char buffer[4] = { 0x00, 0x01, 0xfe, 0xff };

    infolist = infolist_new (NULL);
    CHECK(infolist);

    /* Item 1: all types with min/max values. */
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_integer (ptr_item, "int_min", INT_MIN));
    CHECK(infolist_new_var_integer (ptr_item, "int_max", INT_MAX));
    CHECK(infolist_new_var_string (ptr_item, "str", "test é"));
    CHECK(infolist_new_var_string (ptr_item, "str_empty", ""));
    CHECK(infolist_new_var_string (ptr_item, "str_null", NULL));
    CHECK(infolist_new_var_pointer (ptr_item, "ptr", (void *)0x1234));
    CHECK(infolist_new_var_buffer (ptr_item, "buf", buffer, sizeof (buffer)));
    CHECK(infolist_new_var_time (ptr_item, "time", 1762288000));
    CHECK(infolist_new_var_long (ptr_item, "long_min", LONG_MIN));
    CHECK(infolist_new_var_long (ptr_item, "long_max", LONG_MAX));
    CHECK(infolist_new_var_longlong (ptr_item, "llong_min", LLONG_MIN));
    CHECK(infolist_new_var_longlong (ptr_item, "llong_max", LLONG_MAX));

    /* Item 2: no variables. */
    CHECK(infolist_new_item (infolist));

    /* Item 3: one variable. */
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_integer (ptr_item, "int", -1));

    upgrade_file = upgrade_file_new (TEST_UPGRADE_FILE, NULL, NULL, NULL);
    CHECK(upgrade_file);
    LONGS_EQUAL(1, upgrade_file_write_object (upgrade_file, 7, infolist));
    LONGS_EQUAL(1, upgrade_file_write_object (upgrade_file, 8, infolist));
    upgrade_file_close (upgrade_file);

    infolist_free (infolist);

    /*
     * Each infolist item is read as a separate object; the pointer is not
     * saved, and an empty string is read as NULL.
     */
    LONGS_EQUAL(1, test_upgrade_file_read_with_log ());
    LONGS_EQUAL(6, test_upgrade_file_read_count);
    STRCMP_EQUAL(
        "[7] int_min=-2147483648 int_max=2147483647 str=test é "
        "str_empty=(null) str_null=(null) buf=0001feff time=1762288000 "
        "long_min=-9223372036854775808 long_max=9223372036854775807 "
        "llong_min=-9223372036854775808 llong_max=9223372036854775807"
        "[7][7] int=-1"
        "[8] int_min=-2147483648 int_max=2147483647 str=test é "
        "str_empty=(null) str_null=(null) buf=0001feff time=1762288000 "
        "long_min=-9223372036854775808 long_max=9223372036854775807 "
        "llong_min=-9223372036854775808 llong_max=9223372036854775807"
        "[8][8] int=-1",
        test_upgrade_file_read_log);
}

/*
 * Test functions:
 *   upgrade_file_read
 *   upgrade_file_read_object
 */

TEST(CoreUpgradeFile, ReadEmptyFile)
{
    struct t_upgrade_file *upgrade_file;

    /* Only the signature: no objects. */
    upgrade_file = upgrade_file_new (TEST_UPGRADE_FILE, NULL, NULL, NULL);
    CHECK(upgrade_file);
    upgrade_file_close (upgrade_file);

    LONGS_EQUAL(1, test_upgrade_file_read_with_log ());
    LONGS_EQUAL(0, test_upgrade_file_read_count);
}

/*
 * Test functions:
 *   upgrade_file_read
 *   upgrade_file_error
 */

TEST(CoreUpgradeFile, ReadBadSignature)
{
    int data[2];

    /* Empty file: signature not found. */
    test_upgrade_file_write_raw (NULL, 0);
    LONGS_EQUAL(0, test_upgrade_file_read_with_log ());

    /* Empty signature. */
    data[0] = 0;
    test_upgrade_file_write_raw (data, sizeof (data[0]));
    LONGS_EQUAL(0, test_upgrade_file_read_with_log ());

    /* Signature length greater than file size. */
    data[0] = 1000;
    test_upgrade_file_write_raw (data, sizeof (data[0]));
    LONGS_EQUAL(0, test_upgrade_file_read_with_log ());

    /* Bad signature. */
    data[0] = 4;
    memcpy (&data[1], "test", 4);
    test_upgrade_file_write_raw (data, sizeof (data));
    LONGS_EQUAL(0, test_upgrade_file_read_with_log ());

    LONGS_EQUAL(0, test_upgrade_file_read_count);
}

/*
 * Test functions:
 *   upgrade_file_read
 *   upgrade_file_read_object
 *   upgrade_file_error
 */

TEST(CoreUpgradeFile, ReadBadObject)
{
    struct t_upgrade_file *upgrade_file;
    int data[4];

    /* Bad object type ("object start" expected). */
    upgrade_file = upgrade_file_new (TEST_UPGRADE_FILE, NULL, NULL, NULL);
    CHECK(upgrade_file);
    data[0] = UPGRADE_TYPE_OBJECT_VAR;
    LONGS_EQUAL(1, fwrite (data, sizeof (data[0]), 1, upgrade_file->file));
    upgrade_file_close (upgrade_file);
    LONGS_EQUAL(0, test_upgrade_file_read_with_log ());

    /* Object start without object id. */
    upgrade_file = upgrade_file_new (TEST_UPGRADE_FILE, NULL, NULL, NULL);
    CHECK(upgrade_file);
    data[0] = UPGRADE_TYPE_OBJECT_START;
    LONGS_EQUAL(1, fwrite (data, sizeof (data[0]), 1, upgrade_file->file));
    upgrade_file_close (upgrade_file);
    LONGS_EQUAL(0, test_upgrade_file_read_with_log ());

    /* Object start and id, without object end. */
    upgrade_file = upgrade_file_new (TEST_UPGRADE_FILE, NULL, NULL, NULL);
    CHECK(upgrade_file);
    data[0] = UPGRADE_TYPE_OBJECT_START;
    data[1] = 1;
    LONGS_EQUAL(1, fwrite (data, sizeof (data[0]) * 2, 1, upgrade_file->file));
    upgrade_file_close (upgrade_file);
    LONGS_EQUAL(0, test_upgrade_file_read_with_log ());

    /* Variable with empty name. */
    upgrade_file = upgrade_file_new (TEST_UPGRADE_FILE, NULL, NULL, NULL);
    CHECK(upgrade_file);
    data[0] = UPGRADE_TYPE_OBJECT_START;
    data[1] = 1;
    data[2] = UPGRADE_TYPE_OBJECT_VAR;
    data[3] = 0;
    LONGS_EQUAL(1, fwrite (data, sizeof (data), 1, upgrade_file->file));
    upgrade_file_close (upgrade_file);
    LONGS_EQUAL(0, test_upgrade_file_read_with_log ());

    /* Valid object with no variables. */
    upgrade_file = upgrade_file_new (TEST_UPGRADE_FILE, NULL, NULL, NULL);
    CHECK(upgrade_file);
    data[0] = UPGRADE_TYPE_OBJECT_START;
    data[1] = 1;
    data[2] = UPGRADE_TYPE_OBJECT_END;
    LONGS_EQUAL(1, fwrite (data, sizeof (data[0]) * 3, 1, upgrade_file->file));
    upgrade_file_close (upgrade_file);
    LONGS_EQUAL(1, test_upgrade_file_read_with_log ());

    LONGS_EQUAL(1, test_upgrade_file_read_count);
    STRCMP_EQUAL("[1]", test_upgrade_file_read_log);
}

/*
 * Test functions:
 *   upgrade_file_read
 *   upgrade_file_read_object
 *   upgrade_file_read_integer
 *   upgrade_file_read_string
 *   upgrade_file_read_buffer
 *   upgrade_file_read_time
 *   upgrade_file_read_long
 *   upgrade_file_read_longlong
 */

TEST(CoreUpgradeFile, ReadTruncatedFile)
{
    struct t_infolist *infolist;
    struct t_infolist_item *ptr_item;
    struct t_upgrade_file *upgrade_file;
    unsigned char buffer[4] = { 0x01, 0x02, 0x03, 0x04 };
    long size, i;

    infolist = infolist_new (NULL);
    CHECK(infolist);
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_integer (ptr_item, "int", 1));
    CHECK(infolist_new_var_string (ptr_item, "str", "abc"));
    CHECK(infolist_new_var_buffer (ptr_item, "buf", buffer, sizeof (buffer)));
    CHECK(infolist_new_var_time (ptr_item, "time", 2));
    CHECK(infolist_new_var_long (ptr_item, "long", 3));
    CHECK(infolist_new_var_longlong (ptr_item, "llong", 4));

    upgrade_file = upgrade_file_new (TEST_UPGRADE_FILE, NULL, NULL, NULL);
    CHECK(upgrade_file);
    LONGS_EQUAL(1, upgrade_file_write_object (upgrade_file, 1, infolist));
    fflush (upgrade_file->file);
    size = ftell (upgrade_file->file);
    upgrade_file_close (upgrade_file);

    infolist_free (infolist);

    /* Full file is OK. */
    LONGS_EQUAL(1, test_upgrade_file_read_with_log ());
    STRCMP_EQUAL("[1] int=1 str=abc buf=01020304 time=2 long=3 llong=4",
                 test_upgrade_file_read_log);

    /*
     * Truncate the file at every position after the signature and the
     * first object type: the read must fail and the callback must never
     * be called.
     */
    test_upgrade_file_read_count = 0;
    for (i = size - 1;
         i >= (long)((2 * sizeof (int)) + strlen (UPGRADE_SIGNATURE));
         i--)
    {
        test_upgrade_file_truncate (i);
        LONGS_EQUAL(0, test_upgrade_file_read_with_log ());
    }
    LONGS_EQUAL(0, test_upgrade_file_read_count);
}

/*
 * Test functions:
 *   upgrade_file_read
 *   upgrade_file_read_object
 */

TEST(CoreUpgradeFile, ReadCallbackError)
{
    struct t_infolist *infolist;
    struct t_upgrade_file *upgrade_file;

    infolist = infolist_new (NULL);
    CHECK(infolist);
    CHECK(infolist_new_item (infolist));
    CHECK(infolist_new_item (infolist));

    upgrade_file = upgrade_file_new (TEST_UPGRADE_FILE, NULL, NULL, NULL);
    CHECK(upgrade_file);
    LONGS_EQUAL(1, upgrade_file_write_object (upgrade_file, 1, infolist));
    upgrade_file_close (upgrade_file);

    infolist_free (infolist);

    /* Read stops on first object when callback returns an error. */
    test_upgrade_file_read_rc = WEECHAT_RC_ERROR;
    LONGS_EQUAL(0, test_upgrade_file_read_with_log ());
    LONGS_EQUAL(1, test_upgrade_file_read_count);
}
