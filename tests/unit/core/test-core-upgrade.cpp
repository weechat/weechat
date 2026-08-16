/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test upgrade functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <stdlib.h>
#include <time.h>
#include "src/core/core-infolist.h"
#include "src/core/core-upgrade.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-line.h"

extern struct t_gui_buffer *upgrade_current_buffer;
extern void upgrade_weechat_read_buffer_line (struct t_infolist *infolist);
}

#define TEST_DATE 1734000000
#define TEST_DATE_USEC 123456
#define TEST_DATE_PRINTED 1734000001
#define TEST_DATE_USEC_PRINTED 654321
#define TEST_ID_FROM_DATE_PRINTED \
    (((long long)TEST_DATE_PRINTED * 1000000LL) + TEST_DATE_USEC_PRINTED)

TEST_GROUP(CoreUpgrade)
{
    /*
     * Add an item in infolist with a buffer line, in the format used by
     * WeeChat < 4.11.0 ("id" is an integer, the date of print is saved in
     * "date_printed"/"date_usec_printed").
     *
     * The variables are added in the same order as in
     * gui_line_add_to_infolist(), because the read of variables relies on
     * this order.
     */

    static void test_infolist_add_line_old_format (struct t_infolist *infolist,
                                                   int id,
                                                   time_t date_printed,
                                                   int date_usec_printed,
                                                   const char *message)
    {
        struct t_infolist_item *ptr_item;

        ptr_item = infolist_new_item (infolist);
        CHECK(ptr_item);

        CHECK(infolist_new_var_integer (ptr_item, "id", id));
        CHECK(infolist_new_var_integer (ptr_item, "y", -1));
        CHECK(infolist_new_var_time (ptr_item, "date", TEST_DATE));
        CHECK(infolist_new_var_integer (ptr_item, "date_usec", TEST_DATE_USEC));
        CHECK(infolist_new_var_time (ptr_item, "date_printed", date_printed));
        CHECK(infolist_new_var_integer (ptr_item, "date_usec_printed",
                                        date_usec_printed));
        CHECK(infolist_new_var_string (ptr_item, "str_time", ""));
        CHECK(infolist_new_var_integer (ptr_item, "tags_count", 0));
        CHECK(infolist_new_var_string (ptr_item, "tags", ""));
        CHECK(infolist_new_var_integer (ptr_item, "displayed", 1));
        CHECK(infolist_new_var_integer (ptr_item, "notify_level", 0));
        CHECK(infolist_new_var_integer (ptr_item, "highlight", 0));
        CHECK(infolist_new_var_string (ptr_item, "prefix", ""));
        CHECK(infolist_new_var_string (ptr_item, "message", message));
        CHECK(infolist_new_var_integer (ptr_item, "last_read_line", 0));
    }

    /*
     * Add an item in infolist with a buffer line, in the format used by
     * WeeChat ≥ 4.11.0 ("id" is a long long with the date of print, and
     * there is no "date_printed"/"date_usec_printed").
     */

    static void test_infolist_add_line_new_format (struct t_infolist *infolist,
                                                   long long id,
                                                   const char *message)
    {
        struct t_infolist_item *ptr_item;

        ptr_item = infolist_new_item (infolist);
        CHECK(ptr_item);

        CHECK(infolist_new_var_longlong (ptr_item, "id", id));
        CHECK(infolist_new_var_integer (ptr_item, "y", -1));
        CHECK(infolist_new_var_time (ptr_item, "date", TEST_DATE));
        CHECK(infolist_new_var_integer (ptr_item, "date_usec", TEST_DATE_USEC));
        CHECK(infolist_new_var_string (ptr_item, "str_time", ""));
        CHECK(infolist_new_var_integer (ptr_item, "tags_count", 0));
        CHECK(infolist_new_var_string (ptr_item, "tags", ""));
        CHECK(infolist_new_var_integer (ptr_item, "displayed", 1));
        CHECK(infolist_new_var_integer (ptr_item, "notify_level", 0));
        CHECK(infolist_new_var_integer (ptr_item, "highlight", 0));
        CHECK(infolist_new_var_string (ptr_item, "prefix", ""));
        CHECK(infolist_new_var_string (ptr_item, "message", message));
        CHECK(infolist_new_var_integer (ptr_item, "last_read_line", 0));
    }

    /* Read all lines of the infolist into the buffer received. */

    static void test_read_lines (struct t_gui_buffer *buffer,
                                 struct t_infolist *infolist)
    {
        upgrade_current_buffer = buffer;
        infolist_reset_item_cursor (infolist);
        while (infolist_next (infolist))
        {
            upgrade_weechat_read_buffer_line (infolist);
        }
        upgrade_current_buffer = NULL;
    }
};

/*
 * Test functions:
 *   upgrade_weechat_read_buffer_line (buffer with formatted content, format
 *   of WeeChat < 4.11.0)
 */

TEST(CoreUpgrade, ReadBufferLineOldFormat)
{
    struct t_gui_buffer *buffer;
    struct t_infolist *infolist;
    struct t_gui_line *ptr_line;

    buffer = gui_buffer_new_user ("test", GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);

    infolist = infolist_new (NULL);
    CHECK(infolist);

    test_infolist_add_line_old_format (infolist, 0,
                                       TEST_DATE_PRINTED,
                                       TEST_DATE_USEC_PRINTED,
                                       "line 1");
    test_read_lines (buffer, infolist);
    infolist_free (infolist);

    /* the id is rebuilt from the date of print */
    ptr_line = buffer->own_lines->last_line;
    CHECK(ptr_line);
    CHECK(ptr_line->data->id == TEST_ID_FROM_DATE_PRINTED);
    CHECK(buffer->lines_last_id_assigned == TEST_ID_FROM_DATE_PRINTED);
    STRCMP_EQUAL("line 1", ptr_line->data->message);

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   upgrade_weechat_read_buffer_line (buffer with formatted content, format
 *   of WeeChat < 4.11.0, with several lines sharing the same date of print)
 */

TEST(CoreUpgrade, ReadBufferLineOldFormatSameDatePrinted)
{
    struct t_gui_buffer *buffer;
    struct t_infolist *infolist;
    struct t_gui_line *ptr_line;

    buffer = gui_buffer_new_user ("test", GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);

    infolist = infolist_new (NULL);
    CHECK(infolist);

    /*
     * all the lines displayed by a single call to
     * gui_chat_printf_datetime_tags() share the same date of print, so the
     * ids must be forced to stay unique and strictly increasing
     */
    test_infolist_add_line_old_format (infolist, 0,
                                       TEST_DATE_PRINTED,
                                       TEST_DATE_USEC_PRINTED,
                                       "line 1");
    test_infolist_add_line_old_format (infolist, 1,
                                       TEST_DATE_PRINTED,
                                       TEST_DATE_USEC_PRINTED,
                                       "line 2");
    test_infolist_add_line_old_format (infolist, 2,
                                       TEST_DATE_PRINTED,
                                       TEST_DATE_USEC_PRINTED,
                                       "line 3");
    test_read_lines (buffer, infolist);
    infolist_free (infolist);

    ptr_line = buffer->own_lines->first_line;
    CHECK(ptr_line);
    CHECK(ptr_line->data->id == TEST_ID_FROM_DATE_PRINTED);
    STRCMP_EQUAL("line 1", ptr_line->data->message);

    ptr_line = ptr_line->next_line;
    CHECK(ptr_line);
    CHECK(ptr_line->data->id == TEST_ID_FROM_DATE_PRINTED + 1);
    STRCMP_EQUAL("line 2", ptr_line->data->message);

    ptr_line = ptr_line->next_line;
    CHECK(ptr_line);
    CHECK(ptr_line->data->id == TEST_ID_FROM_DATE_PRINTED + 2);
    STRCMP_EQUAL("line 3", ptr_line->data->message);

    POINTERS_EQUAL(NULL, ptr_line->next_line);
    CHECK(buffer->lines_last_id_assigned == TEST_ID_FROM_DATE_PRINTED + 2);

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   upgrade_weechat_read_buffer_line (buffer with formatted content, format
 *   of WeeChat ≥ 4.11.0)
 */

TEST(CoreUpgrade, ReadBufferLineNewFormat)
{
    struct t_gui_buffer *buffer;
    struct t_infolist *infolist;
    struct t_gui_line *ptr_line;

    buffer = gui_buffer_new_user ("test", GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);

    infolist = infolist_new (NULL);
    CHECK(infolist);

    test_infolist_add_line_new_format (infolist, 1786886000000000LL, "line 1");
    test_infolist_add_line_new_format (infolist, 1786886000000123LL, "line 2");
    test_read_lines (buffer, infolist);
    infolist_free (infolist);

    /* the id is read as-is */
    ptr_line = buffer->own_lines->first_line;
    CHECK(ptr_line);
    CHECK(ptr_line->data->id == 1786886000000000LL);
    STRCMP_EQUAL("line 1", ptr_line->data->message);

    ptr_line = ptr_line->next_line;
    CHECK(ptr_line);
    CHECK(ptr_line->data->id == 1786886000000123LL);
    STRCMP_EQUAL("line 2", ptr_line->data->message);

    POINTERS_EQUAL(NULL, ptr_line->next_line);
    CHECK(buffer->lines_last_id_assigned == 1786886000000123LL);

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   upgrade_weechat_read_buffer_line (buffer with free content)
 */

TEST(CoreUpgrade, ReadBufferLineFreeContent)
{
    struct t_gui_buffer *buffer;
    struct t_infolist *infolist;
    struct t_infolist_item *ptr_item;
    struct t_gui_line *ptr_line;

    buffer = gui_buffer_new_user ("test", GUI_BUFFER_TYPE_FREE);
    CHECK(buffer);

    infolist = infolist_new (NULL);
    CHECK(infolist);

    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_longlong (ptr_item, "id", 1786886000000000LL));
    CHECK(infolist_new_var_integer (ptr_item, "y", 2));
    CHECK(infolist_new_var_time (ptr_item, "date", TEST_DATE));
    CHECK(infolist_new_var_integer (ptr_item, "date_usec", TEST_DATE_USEC));
    CHECK(infolist_new_var_string (ptr_item, "str_time", ""));
    CHECK(infolist_new_var_integer (ptr_item, "tags_count", 0));
    CHECK(infolist_new_var_string (ptr_item, "tags", ""));
    CHECK(infolist_new_var_string (ptr_item, "message", "line 1"));

    test_read_lines (buffer, infolist);
    infolist_free (infolist);

    /* on buffers with free content, the id of a line is its number ("y") */
    ptr_line = buffer->own_lines->last_line;
    CHECK(ptr_line);
    LONGS_EQUAL(2, ptr_line->data->y);
    CHECK(ptr_line->data->id == 2);
    CHECK(buffer->lines_last_id_assigned == -1);
    STRCMP_EQUAL("line 1", ptr_line->data->message);

    gui_buffer_close (buffer);
}
