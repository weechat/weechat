/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test upgrade functions */

#include "CppUTest/TestHarness.h"

#include "tests-record.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include "src/core/weechat.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/core/core-infolist.h"
#include "src/core/core-string.h"
#include "src/core/core-upgrade.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-chat.h"
#include "src/gui/gui-history.h"
#include "src/gui/gui-hotlist.h"
#include "src/gui/gui-key.h"
#include "src/gui/gui-layout.h"
#include "src/gui/gui-line.h"
#include "src/gui/gui-nicklist.h"
#include "src/gui/gui-window.h"
#include "src/plugins/plugin.h"

extern struct t_gui_buffer *upgrade_current_buffer;
extern struct t_gui_buffer *upgrade_set_current_buffer;
extern int upgrade_set_current_window;
extern int hotlist_reset;
extern struct t_gui_layout *upgrade_layout;
extern int upgrade_weechat_save_history (struct t_upgrade_file *upgrade_file,
                                         struct t_gui_history *last_history);
extern int upgrade_weechat_save_buffers (struct t_upgrade_file *upgrade_file);
extern int upgrade_weechat_save_misc (struct t_upgrade_file *upgrade_file);
extern int upgrade_weechat_save_hotlist (struct t_upgrade_file *upgrade_file);
extern int upgrade_weechat_save_layout_window_tree (
    struct t_upgrade_file *upgrade_file,
    struct t_gui_layout_window *layout_window);
extern int upgrade_weechat_save_layout_window (struct t_upgrade_file *upgrade_file);
extern void upgrade_weechat_read_buffer (struct t_infolist *infolist);
extern void upgrade_weechat_read_buffer_line (struct t_infolist *infolist);
extern void upgrade_weechat_read_nicklist (struct t_infolist *infolist);
extern void upgrade_weechat_read_hotlist (struct t_infolist *infolist);
extern int upgrade_weechat_read_cb (const void *pointer, void *data,
                                    struct t_upgrade_file *upgrade_file,
                                    int object_id,
                                    struct t_infolist *infolist);
extern void upgrade_weechat_remove_file_cb (void *data, const char *filename);
}

#define TEST_UPGRADE_FILENAME "test_core_upgrade"
#define TEST_BUFFER_NAME "test_upgrade"

/*
 * Read callback for upgrade file: append a short description of each item
 * read to the dynamic string received in "pointer" (not in "data", which is
 * freed by upgrade_file_close()).
 */

int
test_core_upgrade_dump_cb (const void *pointer, void *data,
                           struct t_upgrade_file *upgrade_file,
                           int object_id,
                           struct t_infolist *infolist)
{
    char **dump, str_item[1024];

    (void) data;
    (void) upgrade_file;

    dump = (char **)pointer;

    infolist_reset_item_cursor (infolist);
    while (infolist_next (infolist))
    {
        str_item[0] = '\0';
        switch (object_id)
        {
            case UPGRADE_WEECHAT_TYPE_HISTORY:
                snprintf (str_item, sizeof (str_item), "H:%s;",
                          infolist_string (infolist, "text"));
                break;
            case UPGRADE_WEECHAT_TYPE_BUFFER:
                snprintf (str_item, sizeof (str_item), "B:%s;",
                          infolist_string (infolist, "name"));
                break;
            case UPGRADE_WEECHAT_TYPE_NICKLIST:
                snprintf (str_item, sizeof (str_item), "N:%s:%s;",
                          infolist_string (infolist, "type"),
                          infolist_string (infolist, "name"));
                break;
            case UPGRADE_WEECHAT_TYPE_BUFFER_LINE:
                snprintf (str_item, sizeof (str_item), "L:%s;",
                          infolist_string (infolist, "message"));
                break;
            case UPGRADE_WEECHAT_TYPE_MISC:
                snprintf (str_item, sizeof (str_item), "M:%d;",
                          infolist_integer (infolist, "upgrade_count"));
                break;
            case UPGRADE_WEECHAT_TYPE_HOTLIST:
                snprintf (str_item, sizeof (str_item), "O:%s;",
                          infolist_string (infolist, "buffer_name"));
                break;
            case UPGRADE_WEECHAT_TYPE_LAYOUT_WINDOW:
                snprintf (str_item, sizeof (str_item), "W:%d:%d;",
                          infolist_integer (infolist, "internal_id"),
                          infolist_integer (infolist, "parent_id"));
                break;
        }
        string_dyn_concat (dump, str_item, -1);
    }

    return WEECHAT_RC_OK;
}

/*
 * Read an upgrade file and return a short description of its content.
 *
 * Note: result must be freed after use.
 */

char *
test_core_upgrade_dump_file (const char *filename)
{
    struct t_upgrade_file *upgrade_file;
    char **dump;

    dump = string_dyn_alloc (256);
    if (!dump)
        return NULL;

    upgrade_file = upgrade_file_new (filename,
                                     &test_core_upgrade_dump_cb, dump, NULL);
    if (upgrade_file)
    {
        upgrade_file_read (upgrade_file);
        upgrade_file_close (upgrade_file);
    }

    return string_dyn_free (dump, 0);
}

/*
 * Remove an upgrade file in WeeChat data directory.
 */

void
test_core_upgrade_remove_file (const char *filename)
{
    char *path;

    if (string_asprintf (&path, "%s/%s.upgrade",
                         weechat_data_dir, filename) >= 0)
    {
        unlink (path);
        free (path);
    }
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

    void teardown ()
    {
        upgrade_current_buffer = NULL;
        upgrade_set_current_buffer = NULL;
        upgrade_set_current_window = 0;
        hotlist_reset = 0;
        gui_layout_free (upgrade_layout);
        upgrade_layout = NULL;
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

    /* The id is rebuilt from the date of print. */
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
     * All the lines displayed by a single call to
     * gui_chat_printf_datetime_tags() share the same date of print, so the
     * ids must be forced to stay unique and strictly increasing.
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

    /* The id is read as-is. */
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

    /* On buffers with free content, the id of a line is its number ("y"). */
    ptr_line = buffer->own_lines->last_line;
    CHECK(ptr_line);
    LONGS_EQUAL(2, ptr_line->data->y);
    CHECK(ptr_line->data->id == 2LL);
    CHECK(buffer->lines_last_id_assigned == -1LL);
    STRCMP_EQUAL("line 1", ptr_line->data->message);

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   upgrade_weechat_save_history
 */

TEST(CoreUpgrade, SaveHistory)
{
    struct t_gui_buffer *buffer;
    struct t_upgrade_file *upgrade_file;
    char *dump;

    /* No history: nothing is written. */
    LONGS_EQUAL(1, upgrade_weechat_save_history (NULL, NULL));

    buffer = gui_buffer_new_user (TEST_BUFFER_NAME, GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);
    gui_history_buffer_add (buffer, "cmd1");
    gui_history_buffer_add (buffer, "cmd2");

    upgrade_file = upgrade_file_new (TEST_UPGRADE_FILENAME, NULL, NULL, NULL);
    CHECK(upgrade_file);
    LONGS_EQUAL(1, upgrade_weechat_save_history (upgrade_file,
                                                 buffer->last_history));
    upgrade_file_close (upgrade_file);

    /* History is saved from the oldest to the newest entry. */
    dump = test_core_upgrade_dump_file (TEST_UPGRADE_FILENAME);
    STRCMP_EQUAL("H:cmd1;H:cmd2;", dump);
    free (dump);

    test_core_upgrade_remove_file (TEST_UPGRADE_FILENAME);
    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   upgrade_weechat_save_buffers
 */

TEST(CoreUpgrade, SaveBuffers)
{
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *ptr_group;
    struct t_upgrade_file *upgrade_file;
    char *dump;

    buffer = gui_buffer_new_user (TEST_BUFFER_NAME, GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);
    gui_buffer_set (buffer, "nicklist", "1");
    ptr_group = gui_nicklist_add_group (buffer, NULL, "grp", "blue", 1);
    CHECK(ptr_group);
    CHECK(gui_nicklist_add_nick (buffer, ptr_group, "alice", "green",
                                 "@", "lightgreen", 1));
    gui_chat_printf (buffer, "line 1");
    gui_chat_printf (buffer, "line 2");
    gui_history_buffer_add (buffer, "cmd1");

    upgrade_file = upgrade_file_new (TEST_UPGRADE_FILENAME, NULL, NULL, NULL);
    CHECK(upgrade_file);
    LONGS_EQUAL(1, upgrade_weechat_save_buffers (upgrade_file));
    upgrade_file_close (upgrade_file);

    /* Buffer, then its nicklist, lines and history. */
    dump = test_core_upgrade_dump_file (TEST_UPGRADE_FILENAME);
    CHECK(dump);
    CHECK(strncmp (dump, "B:weechat;", 10) == 0);
    CHECK(strstr (dump,
                  "B:" TEST_BUFFER_NAME ";"
                  "N:group:root;N:group:grp;N:nick:alice;"
                  "L:line 1;L:line 2;"
                  "H:cmd1;"));
    free (dump);

    test_core_upgrade_remove_file (TEST_UPGRADE_FILENAME);
    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   upgrade_weechat_save_misc
 */

TEST(CoreUpgrade, SaveMisc)
{
    struct t_upgrade_file *upgrade_file;
    char *dump, str_expected[64];

    upgrade_file = upgrade_file_new (TEST_UPGRADE_FILENAME, NULL, NULL, NULL);
    CHECK(upgrade_file);
    LONGS_EQUAL(1, upgrade_weechat_save_misc (upgrade_file));
    upgrade_file_close (upgrade_file);

    snprintf (str_expected, sizeof (str_expected),
              "M:%d;", weechat_upgrade_count);
    dump = test_core_upgrade_dump_file (TEST_UPGRADE_FILENAME);
    STRCMP_EQUAL(str_expected, dump);
    free (dump);

    test_core_upgrade_remove_file (TEST_UPGRADE_FILENAME);
}

/*
 * Test functions:
 *   upgrade_weechat_save_hotlist
 */

TEST(CoreUpgrade, SaveHotlist)
{
    struct t_gui_buffer *buffer;
    struct t_upgrade_file *upgrade_file;
    char *dump;

    buffer = gui_buffer_new_user (TEST_BUFFER_NAME, GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);
    CHECK(gui_hotlist_add (buffer, GUI_HOTLIST_MESSAGE, NULL, 0));

    upgrade_file = upgrade_file_new (TEST_UPGRADE_FILENAME, NULL, NULL, NULL);
    CHECK(upgrade_file);
    LONGS_EQUAL(1, upgrade_weechat_save_hotlist (upgrade_file));
    upgrade_file_close (upgrade_file);

    dump = test_core_upgrade_dump_file (TEST_UPGRADE_FILENAME);
    CHECK(dump);
    CHECK(strstr (dump, "O:" TEST_BUFFER_NAME ";"));
    free (dump);

    test_core_upgrade_remove_file (TEST_UPGRADE_FILENAME);
    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   upgrade_weechat_save_layout_window_tree
 *   upgrade_weechat_save_layout_window
 */

TEST(CoreUpgrade, SaveLayoutWindow)
{
    struct t_gui_layout *layout;
    struct t_gui_layout_window *ptr_root, *ptr_child1;
    struct t_upgrade_file *upgrade_file;
    char *dump;

    /* Tree: root (1) split in 2 (2, 3), the first child split in 2 (4, 5). */
    layout = gui_layout_alloc ("test");
    CHECK(layout);
    ptr_root = gui_layout_window_add (&layout->layout_windows, 1, NULL,
                                      50, 0, NULL, NULL);
    CHECK(ptr_root);
    ptr_child1 = gui_layout_window_add (&layout->layout_windows, 2, ptr_root,
                                        50, 1, NULL, NULL);
    CHECK(ptr_child1);
    CHECK(gui_layout_window_add (&layout->layout_windows, 3, ptr_root,
                                 50, 1, "core", "weechat"));
    CHECK(gui_layout_window_add (&layout->layout_windows, 4, ptr_child1,
                                 50, 1, "core", "weechat"));
    CHECK(gui_layout_window_add (&layout->layout_windows, 5, ptr_child1,
                                 50, 1, "core", "weechat"));

    upgrade_file = upgrade_file_new (TEST_UPGRADE_FILENAME, NULL, NULL, NULL);
    CHECK(upgrade_file);
    LONGS_EQUAL(1, upgrade_weechat_save_layout_window_tree (
                    upgrade_file, layout->layout_windows));
    upgrade_file_close (upgrade_file);
    gui_layout_free (layout);

    dump = test_core_upgrade_dump_file (TEST_UPGRADE_FILENAME);
    STRCMP_EQUAL("W:1:0;W:2:1;W:4:2;W:5:2;W:3:1;", dump);
    free (dump);

    /* Layout of current windows. */
    upgrade_file = upgrade_file_new (TEST_UPGRADE_FILENAME, NULL, NULL, NULL);
    CHECK(upgrade_file);
    LONGS_EQUAL(1, upgrade_weechat_save_layout_window (upgrade_file));
    upgrade_file_close (upgrade_file);

    dump = test_core_upgrade_dump_file (TEST_UPGRADE_FILENAME);
    CHECK(dump);
    CHECK(strncmp (dump, "W:", 2) == 0);
    free (dump);

    test_core_upgrade_remove_file (TEST_UPGRADE_FILENAME);
}

/*
 * Test functions:
 *   upgrade_weechat_save
 */

TEST(CoreUpgrade, Save)
{
    struct t_gui_buffer *buffer;
    char *dump;

    buffer = gui_buffer_new_user (TEST_BUFFER_NAME, GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);
    gui_chat_printf (buffer, "line 1");
    CHECK(gui_hotlist_add (buffer, GUI_HOTLIST_MESSAGE, NULL, 0));

    LONGS_EQUAL(1, upgrade_weechat_save ());

    dump = test_core_upgrade_dump_file (WEECHAT_UPGRADE_FILENAME);
    CHECK(dump);
    CHECK(strstr (dump, "B:weechat;"));
    CHECK(strstr (dump, "B:" TEST_BUFFER_NAME ";L:line 1;"));
    CHECK(strstr (dump, "M:"));
    CHECK(strstr (dump, "O:" TEST_BUFFER_NAME ";"));
    CHECK(strstr (dump, "W:"));
    free (dump);

    test_core_upgrade_remove_file (WEECHAT_UPGRADE_FILENAME);
    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   upgrade_weechat_read_buffer (buffer saved by current version)
 */

TEST(CoreUpgrade, ReadBuffer)
{
    struct t_gui_buffer *buffer, *ptr_buffer;
    struct t_infolist *infolist;
    char str_input[301];
    long long id;

    buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                             NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(buffer);
    gui_buffer_set (buffer, "short_name", "short");
    gui_buffer_set (buffer, "title", "the title");
    /* Not with gui_buffer_set(), which creates an option "weechat.notify.*". */
    buffer->notify = 1;
    gui_buffer_set (buffer, "day_change", "0");
    gui_buffer_set (buffer, "clear", "0");
    gui_buffer_set (buffer, "filter", "0");
    gui_buffer_set (buffer, "time_for_each_line", "0");
    gui_buffer_set (buffer, "prefix_for_each_line", "0");
    gui_buffer_set (buffer, "nicklist_case_sensitive", "1");
    gui_buffer_set (buffer, "nicklist_display_groups", "0");
    gui_buffer_set (buffer, "modes", "+nt");
    gui_buffer_set (buffer, "input_prompt", "prompt");
    gui_buffer_set (buffer, "input_get_any_user_data", "1");
    gui_buffer_set (buffer, "input_get_unknown_commands", "1");
    gui_buffer_set (buffer, "input_get_empty", "1");
    gui_buffer_set (buffer, "input_multiline", "1");
    /* Long input: the input buffer is bigger than the default size. */
    memset (str_input, 'a', sizeof (str_input) - 1);
    str_input[sizeof (str_input) - 1] = '\0';
    gui_buffer_set (buffer, "input", str_input);
    gui_buffer_set (buffer, "input_pos", "2");
    gui_buffer_set (buffer, "highlight_words", "word");
    gui_buffer_set (buffer, "highlight_disable_regex", "nohl");
    gui_buffer_set (buffer, "highlight_regex", "hl.*");
    gui_buffer_set (buffer, "highlight_tags_restrict", "irc_privmsg");
    gui_buffer_set (buffer, "highlight_tags", "tag1");
    gui_buffer_set (buffer, "hotlist_max_level_nicks", "alice:2");
    gui_buffer_set (buffer, "key_bind_meta-a", "/print test");
    gui_buffer_set (buffer, "localvar_set_myvar", "value");
    id = buffer->id;

    infolist = infolist_new (NULL);
    CHECK(infolist);
    CHECK(gui_buffer_add_to_infolist (infolist, buffer));
    gui_buffer_close (buffer);
    POINTERS_EQUAL(NULL, gui_buffer_search_by_id (id));

    upgrade_layout = gui_layout_alloc (GUI_LAYOUT_UPGRADE);
    CHECK(upgrade_layout);
    infolist_reset_item_cursor (infolist);
    CHECK(infolist_next (infolist));
    upgrade_weechat_read_buffer (infolist);
    infolist_free (infolist);

    ptr_buffer = gui_buffer_search_by_id (id);
    CHECK(ptr_buffer);
    POINTERS_EQUAL(ptr_buffer, upgrade_current_buffer);
    POINTERS_EQUAL(NULL, upgrade_set_current_buffer);
    CHECK(ptr_buffer->id == id);
    CHECK(gui_buffer_last_id_assigned >= id);

    /* Buffer is added in the layout. */
    CHECK(upgrade_layout->layout_buffers);
    STRCMP_EQUAL("core", upgrade_layout->layout_buffers->plugin_name);
    STRCMP_EQUAL(TEST_BUFFER_NAME, upgrade_layout->layout_buffers->buffer_name);

    STRCMP_EQUAL("core", ptr_buffer->plugin_name_for_upgrade);
    STRCMP_EQUAL("core." TEST_BUFFER_NAME, ptr_buffer->full_name);
    STRCMP_EQUAL("short", ptr_buffer->short_name);
    STRCMP_EQUAL("the title", ptr_buffer->title);
    LONGS_EQUAL(GUI_BUFFER_TYPE_FORMATTED, ptr_buffer->type);
    LONGS_EQUAL(1, ptr_buffer->notify);
    LONGS_EQUAL(0, ptr_buffer->hidden);
    LONGS_EQUAL(0, ptr_buffer->day_change);
    LONGS_EQUAL(0, ptr_buffer->clear);
    LONGS_EQUAL(0, ptr_buffer->filter);
    LONGS_EQUAL(0, ptr_buffer->time_for_each_line);
    LONGS_EQUAL(0, ptr_buffer->prefix_for_each_line);
    LONGS_EQUAL(1, ptr_buffer->nicklist_case_sensitive);
    LONGS_EQUAL(0, ptr_buffer->nicklist_display_groups);
    STRCMP_EQUAL("+nt", ptr_buffer->modes);
    STRCMP_EQUAL("prompt", ptr_buffer->input_prompt);
    LONGS_EQUAL(1, ptr_buffer->input_get_any_user_data);
    LONGS_EQUAL(1, ptr_buffer->input_get_unknown_commands);
    LONGS_EQUAL(1, ptr_buffer->input_get_empty);
    LONGS_EQUAL(1, ptr_buffer->input_multiline);
    STRCMP_EQUAL(str_input, ptr_buffer->input_buffer);
    LONGS_EQUAL(2 * GUI_BUFFER_INPUT_BLOCK_SIZE,
                ptr_buffer->input_buffer_alloc);
    LONGS_EQUAL(300, ptr_buffer->input_buffer_size);
    LONGS_EQUAL(300, ptr_buffer->input_buffer_length);
    LONGS_EQUAL(2, ptr_buffer->input_buffer_pos);
    STRCMP_EQUAL("word", ptr_buffer->highlight_words);
    STRCMP_EQUAL("nohl", ptr_buffer->highlight_disable_regex);
    CHECK(ptr_buffer->highlight_disable_regex_compiled);
    STRCMP_EQUAL("hl.*", ptr_buffer->highlight_regex);
    CHECK(ptr_buffer->highlight_regex_compiled);
    STRCMP_EQUAL("irc_privmsg", ptr_buffer->highlight_tags_restrict);
    STRCMP_EQUAL("tag1", ptr_buffer->highlight_tags);
    LONGS_EQUAL(1, hashtable_get_integer (ptr_buffer->hotlist_max_level_nicks,
                                          "items_count"));
    LONGS_EQUAL(1, ptr_buffer->keys_count);
    STRCMP_EQUAL("meta-a", ptr_buffer->keys->key);
    STRCMP_EQUAL("/print test", ptr_buffer->keys->command);
    STRCMP_EQUAL("value",
                 (const char *)hashtable_get (ptr_buffer->local_variables,
                                              "myvar"));

    gui_buffer_close (ptr_buffer);
}

/*
 * Test functions:
 *   upgrade_weechat_read_buffer (buffer saved by an old version)
 */

TEST(CoreUpgrade, ReadBufferOldFormat)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_infolist *infolist;
    struct t_infolist_item *ptr_item;

    infolist = infolist_new (NULL);
    CHECK(infolist);

    /*
     * WeeChat < 4.11.0: "id" is a string; WeeChat < 1.0: no "hidden",
     * "clear", "filter"; WeeChat < 4.11.0: no "prefix_for_each_line";
     * WeeChat <= 0.4.2: no "highlight_tags_restrict"; no "short_name",
     * "day_change", "input_buffer".
     */
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_string (ptr_item, "id", "1734000000123456"));
    CHECK(infolist_new_var_integer (ptr_item, "current_buffer", 1));
    CHECK(infolist_new_var_string (ptr_item, "plugin_name", "core"));
    CHECK(infolist_new_var_integer (ptr_item, "number", 2));
    CHECK(infolist_new_var_string (ptr_item, "name", TEST_BUFFER_NAME));
    CHECK(infolist_new_var_integer (ptr_item, "type", GUI_BUFFER_TYPE_FREE));
    CHECK(infolist_new_var_integer (ptr_item, "notify", 2));
    CHECK(infolist_new_var_string (ptr_item, "title", NULL));
    CHECK(infolist_new_var_integer (ptr_item, "input", 1));
    CHECK(infolist_new_var_integer (ptr_item, "input_buffer_alloc", 0));
    CHECK(infolist_new_var_string (ptr_item, "highlight_tags", "tag1"));
    CHECK(infolist_new_var_string (ptr_item, "key_00000", "meta-b"));
    CHECK(infolist_new_var_string (ptr_item, "key_command_00000", "/cmd1"));
    CHECK(infolist_new_var_string (ptr_item, "key_00001", "meta-c"));
    CHECK(infolist_new_var_string (ptr_item, "key_command_00001", "/cmd2"));
    CHECK(infolist_new_var_string (ptr_item, "localvar_name_00000", "var1"));
    CHECK(infolist_new_var_string (ptr_item, "localvar_value_00000", "v1"));
    CHECK(infolist_new_var_string (ptr_item, "localvar_name_00001", "var2"));
    CHECK(infolist_new_var_string (ptr_item, "localvar_value_00001", "v2"));

    infolist_reset_item_cursor (infolist);
    CHECK(infolist_next (infolist));
    upgrade_weechat_read_buffer (infolist);
    infolist_free (infolist);

    ptr_buffer = gui_buffer_search_by_full_name ("core." TEST_BUFFER_NAME);
    CHECK(ptr_buffer);
    POINTERS_EQUAL(ptr_buffer, upgrade_current_buffer);
    POINTERS_EQUAL(ptr_buffer, upgrade_set_current_buffer);
    CHECK(ptr_buffer->id == 1734000000123456LL);
    POINTERS_EQUAL(ptr_buffer, gui_buffer_search_by_id (1734000000123456LL));
    STRCMP_EQUAL(TEST_BUFFER_NAME, ptr_buffer->short_name);
    POINTERS_EQUAL(NULL, ptr_buffer->title);
    LONGS_EQUAL(GUI_BUFFER_TYPE_FREE, ptr_buffer->type);
    LONGS_EQUAL(2, ptr_buffer->notify);
    LONGS_EQUAL(0, ptr_buffer->hidden);
    LONGS_EQUAL(1, ptr_buffer->day_change);
    LONGS_EQUAL(0, ptr_buffer->clear);
    LONGS_EQUAL(1, ptr_buffer->filter);
    LONGS_EQUAL(0, ptr_buffer->prefix_for_each_line);
    STRCMP_EQUAL("tag1", ptr_buffer->highlight_tags_restrict);
    POINTERS_EQUAL(NULL, ptr_buffer->highlight_tags);
    LONGS_EQUAL(2, ptr_buffer->keys_count);
    STRCMP_EQUAL("v1",
                 (const char *)hashtable_get (ptr_buffer->local_variables,
                                              "var1"));
    STRCMP_EQUAL("v2",
                 (const char *)hashtable_get (ptr_buffer->local_variables,
                                              "var2"));

    gui_buffer_close (ptr_buffer);
}

/*
 * Test functions:
 *   upgrade_weechat_read_buffer (invalid or missing id, existing buffer)
 */

TEST(CoreUpgrade, ReadBufferInvalid)
{
    struct t_gui_buffer *buffer, *ptr_buffer;
    struct t_infolist *infolist;
    struct t_infolist_item *ptr_item;
    char str_name[64];
    long long id;
    int i;

    infolist = infolist_new (NULL);
    CHECK(infolist);

    /* Invalid id: a new id is generated. */
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_string (ptr_item, "id", "abc"));
    CHECK(infolist_new_var_string (ptr_item, "plugin_name", "core"));
    CHECK(infolist_new_var_string (ptr_item, "name", TEST_BUFFER_NAME "1"));
    CHECK(infolist_new_var_integer (ptr_item, "type",
                                    GUI_BUFFER_TYPE_FORMATTED));

    /* No id (WeeChat < 4.3.0): a new id is generated. */
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_string (ptr_item, "plugin_name", "core"));
    CHECK(infolist_new_var_string (ptr_item, "name", TEST_BUFFER_NAME "2"));
    CHECK(infolist_new_var_integer (ptr_item, "type",
                                    GUI_BUFFER_TYPE_FORMATTED));

    /* Id with an unexpected type: a new id is generated. */
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_integer (ptr_item, "id", 123));
    CHECK(infolist_new_var_string (ptr_item, "plugin_name", "core"));
    CHECK(infolist_new_var_string (ptr_item, "name", TEST_BUFFER_NAME "3"));
    CHECK(infolist_new_var_integer (ptr_item, "type",
                                    GUI_BUFFER_TYPE_FORMATTED));

    infolist_reset_item_cursor (infolist);
    id = gui_buffer_last_id_assigned;
    while (infolist_next (infolist))
    {
        upgrade_weechat_read_buffer (infolist);
        CHECK(upgrade_current_buffer);
        CHECK(upgrade_current_buffer->id > id);
        id = upgrade_current_buffer->id;
        LONGS_EQUAL(1, upgrade_current_buffer->clear);
        LONGS_EQUAL(1, upgrade_current_buffer->prefix_for_each_line);
    }
    infolist_free (infolist);

    for (i = 1; i <= 3; i++)
    {
        snprintf (str_name, sizeof (str_name),
                  "core.%s%d", TEST_BUFFER_NAME, i);
        ptr_buffer = gui_buffer_search_by_full_name (str_name);
        CHECK(ptr_buffer);
        gui_buffer_close (ptr_buffer);
    }

    /* Buffer already exists: it is not created. */
    buffer = gui_buffer_new_user (TEST_BUFFER_NAME, GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);
    infolist = infolist_new (NULL);
    CHECK(infolist);
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_string (ptr_item, "plugin_name", "core"));
    CHECK(infolist_new_var_string (ptr_item, "name", TEST_BUFFER_NAME));
    CHECK(infolist_new_var_string (ptr_item, "title", "new title"));
    infolist_reset_item_cursor (infolist);
    CHECK(infolist_next (infolist));
    upgrade_current_buffer = buffer;
    upgrade_weechat_read_buffer (infolist);
    infolist_free (infolist);
    POINTERS_EQUAL(NULL, upgrade_current_buffer);
    ptr_buffer = gui_buffer_search_by_full_name ("core." TEST_BUFFER_NAME);
    POINTERS_EQUAL(buffer, ptr_buffer);
    POINTERS_EQUAL(NULL, ptr_buffer->title);

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   upgrade_weechat_read_buffer (main buffer)
 */

TEST(CoreUpgrade, ReadBufferMain)
{
    struct t_infolist *infolist;
    char *title;
    long long id;

    id = gui_buffers->id;
    title = (gui_buffers->title) ? strdup (gui_buffers->title) : NULL;

    infolist = infolist_new (NULL);
    CHECK(infolist);
    CHECK(gui_buffer_add_to_infolist (infolist, gui_buffers));
    infolist_reset_item_cursor (infolist);
    CHECK(infolist_next (infolist));

    /* The main buffer is reused (not created). */
    upgrade_weechat_read_buffer (infolist);
    infolist_free (infolist);
    POINTERS_EQUAL(gui_buffers, upgrade_current_buffer);
    CHECK(gui_buffers->id == id);
    POINTERS_EQUAL(gui_buffers, gui_buffer_search_by_id (id));
    STRCMP_EQUAL(title, gui_buffers->title);
    STRCMP_EQUAL("weechat", gui_buffers->name);

    free (title);
}

/*
 * Test functions:
 *   upgrade_weechat_read_nicklist
 */

TEST(CoreUpgrade, ReadNicklist)
{
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *ptr_group, *ptr_group2;
    struct t_gui_nick *ptr_nick;
    struct t_infolist *infolist;
    struct t_infolist_item *ptr_item;

    infolist = infolist_new (NULL);
    CHECK(infolist);

    /* Root group (ignored). */
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_longlong (ptr_item, "id", 100));
    CHECK(infolist_new_var_string (ptr_item, "type", "group"));
    CHECK(infolist_new_var_string (ptr_item, "name", "root"));

    /* Group with id as "long long" (WeeChat ≥ 4.11.0). */
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_longlong (ptr_item, "id", 101));
    CHECK(infolist_new_var_string (ptr_item, "type", "group"));
    CHECK(infolist_new_var_string (ptr_item, "name", "grp"));
    CHECK(infolist_new_var_string (ptr_item, "color", "blue"));
    CHECK(infolist_new_var_integer (ptr_item, "visible", 1));
    CHECK(infolist_new_var_string (ptr_item, "parent_name", "root"));

    /* Sub-group with id as string (WeeChat < 4.11.0). */
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_string (ptr_item, "id", "102"));
    CHECK(infolist_new_var_string (ptr_item, "type", "group"));
    CHECK(infolist_new_var_string (ptr_item, "name", "sub"));
    CHECK(infolist_new_var_integer (ptr_item, "visible", 0));
    CHECK(infolist_new_var_string (ptr_item, "parent_name", "grp"));

    /* Nick in sub-group with invalid id: a new id is generated. */
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_string (ptr_item, "id", "abc"));
    CHECK(infolist_new_var_string (ptr_item, "type", "nick"));
    CHECK(infolist_new_var_string (ptr_item, "group_name", "sub"));
    CHECK(infolist_new_var_string (ptr_item, "name", "alice"));
    CHECK(infolist_new_var_string (ptr_item, "color", "green"));
    CHECK(infolist_new_var_string (ptr_item, "prefix", "@"));
    CHECK(infolist_new_var_string (ptr_item, "prefix_color", "lightgreen"));
    CHECK(infolist_new_var_integer (ptr_item, "visible", 1));

    /* Nick without group and id (WeeChat < 4.3.0): added in root group. */
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_string (ptr_item, "type", "nick"));
    CHECK(infolist_new_var_string (ptr_item, "name", "bob"));
    CHECK(infolist_new_var_integer (ptr_item, "visible", 1));

    /* Item without type, and unknown type (ignored). */
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_string (ptr_item, "name", "carol"));
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_string (ptr_item, "type", "unknown"));
    CHECK(infolist_new_var_string (ptr_item, "name", "dave"));

    /* No current buffer: nothing is done. */
    upgrade_current_buffer = NULL;
    infolist_reset_item_cursor (infolist);
    CHECK(infolist_next (infolist));
    upgrade_weechat_read_nicklist (infolist);

    buffer = gui_buffer_new_user (TEST_BUFFER_NAME, GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);
    LONGS_EQUAL(0, buffer->nicklist);

    upgrade_current_buffer = buffer;
    infolist_reset_item_cursor (infolist);
    while (infolist_next (infolist))
    {
        upgrade_weechat_read_nicklist (infolist);
    }
    infolist_free (infolist);

    LONGS_EQUAL(1, buffer->nicklist);
    LONGS_EQUAL(2, buffer->nicklist_groups_count);
    LONGS_EQUAL(2, buffer->nicklist_nicks_count);

    ptr_group = gui_nicklist_search_group (buffer, NULL, "grp");
    CHECK(ptr_group);
    CHECK(ptr_group->id == 101);
    POINTERS_EQUAL(buffer->nicklist_root, ptr_group->parent);
    STRCMP_EQUAL("blue", ptr_group->color);
    LONGS_EQUAL(1, ptr_group->visible);

    ptr_group2 = gui_nicklist_search_group (buffer, NULL, "sub");
    CHECK(ptr_group2);
    CHECK(ptr_group2->id == 102);
    POINTERS_EQUAL(ptr_group, ptr_group2->parent);
    LONGS_EQUAL(0, ptr_group2->visible);

    ptr_nick = gui_nicklist_search_nick (buffer, NULL, "alice");
    CHECK(ptr_nick);
    CHECK(ptr_nick->id > 102);
    POINTERS_EQUAL(ptr_group2, ptr_nick->group);
    STRCMP_EQUAL("green", ptr_nick->color);
    STRCMP_EQUAL("@", ptr_nick->prefix);
    STRCMP_EQUAL("lightgreen", ptr_nick->prefix_color);

    ptr_nick = gui_nicklist_search_nick (buffer, NULL, "bob");
    CHECK(ptr_nick);
    POINTERS_EQUAL(buffer->nicklist_root, ptr_nick->group);

    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (buffer, NULL, "carol"));
    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (buffer, NULL, "dave"));

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   upgrade_weechat_read_hotlist
 */

TEST(CoreUpgrade, ReadHotlist)
{
    struct t_gui_buffer *buffer, *buffer2;
    struct t_infolist *infolist;
    struct t_infolist_item *ptr_item;
    struct timeval creation_time;
    char option_name[64];
    int i;

    buffer = gui_buffer_new_user (TEST_BUFFER_NAME, GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);
    buffer2 = gui_buffer_new_user (TEST_BUFFER_NAME "2",
                                   GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer2);

    /* This hotlist is cleared when the first hotlist is read. */
    CHECK(gui_hotlist_add (buffer2, GUI_HOTLIST_LOW, NULL, 0));
    CHECK(buffer2->hotlist);

    creation_time.tv_sec = TEST_DATE;
    creation_time.tv_usec = TEST_DATE_USEC;

    infolist = infolist_new (NULL);
    CHECK(infolist);

    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_integer (ptr_item, "priority",
                                    GUI_HOTLIST_MESSAGE));
    CHECK(infolist_new_var_buffer (ptr_item, "creation_time",
                                   &creation_time, sizeof (creation_time)));
    CHECK(infolist_new_var_string (ptr_item, "plugin_name", "core"));
    CHECK(infolist_new_var_string (ptr_item, "buffer_name", TEST_BUFFER_NAME));
    for (i = 0; i < GUI_HOTLIST_NUM_PRIORITIES; i++)
    {
        snprintf (option_name, sizeof (option_name), "count_%02d", i);
        CHECK(infolist_new_var_integer (ptr_item, option_name, i + 1));
    }

    /* Unknown buffer (ignored). */
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_integer (ptr_item, "priority",
                                    GUI_HOTLIST_MESSAGE));
    CHECK(infolist_new_var_buffer (ptr_item, "creation_time",
                                   &creation_time, sizeof (creation_time)));
    CHECK(infolist_new_var_string (ptr_item, "plugin_name", "core"));
    CHECK(infolist_new_var_string (ptr_item, "buffer_name", "xxx"));

    /* No creation time (ignored). */
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_integer (ptr_item, "priority",
                                    GUI_HOTLIST_MESSAGE));
    CHECK(infolist_new_var_string (ptr_item, "plugin_name", "core"));
    CHECK(infolist_new_var_string (ptr_item, "buffer_name",
                                   TEST_BUFFER_NAME "2"));

    /* No buffer name (ignored). */
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_integer (ptr_item, "priority",
                                    GUI_HOTLIST_MESSAGE));
    CHECK(infolist_new_var_string (ptr_item, "plugin_name", "core"));

    hotlist_reset = 0;
    infolist_reset_item_cursor (infolist);
    while (infolist_next (infolist))
    {
        upgrade_weechat_read_hotlist (infolist);
        LONGS_EQUAL(1, hotlist_reset);
    }
    infolist_free (infolist);

    POINTERS_EQUAL(NULL, buffer2->hotlist);
    CHECK(buffer->hotlist);
    LONGS_EQUAL(GUI_HOTLIST_MESSAGE, buffer->hotlist->priority);
    LONGS_EQUAL(TEST_DATE, buffer->hotlist->creation_time.tv_sec);
    LONGS_EQUAL(TEST_DATE_USEC, buffer->hotlist->creation_time.tv_usec);
    for (i = 0; i < GUI_HOTLIST_NUM_PRIORITIES; i++)
    {
        LONGS_EQUAL(i + 1, buffer->hotlist->count[i]);
    }

    gui_buffer_close (buffer);
    gui_buffer_close (buffer2);
}

/*
 * Test functions:
 *   upgrade_weechat_read_cb
 */

TEST(CoreUpgrade, ReadCb)
{
    struct t_gui_buffer *buffer;
    struct t_infolist *infolist;
    struct t_infolist_item *ptr_item;
    struct t_gui_layout_window *ptr_root;
    long long last_id_assigned;
    time_t old_first_start_time;
    int old_upgrade_count;

    old_first_start_time = weechat_first_start_time;
    old_upgrade_count = weechat_upgrade_count;

    /* Buffer, its line and history. */
    infolist = infolist_new (NULL);
    CHECK(infolist);
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_string (ptr_item, "plugin_name", "core"));
    CHECK(infolist_new_var_string (ptr_item, "name", TEST_BUFFER_NAME));
    CHECK(infolist_new_var_integer (ptr_item, "type",
                                    GUI_BUFFER_TYPE_FORMATTED));
    LONGS_EQUAL(WEECHAT_RC_OK,
                upgrade_weechat_read_cb (NULL, NULL, NULL,
                                         UPGRADE_WEECHAT_TYPE_BUFFER,
                                         infolist));
    infolist_free (infolist);
    buffer = gui_buffer_search_by_full_name ("core." TEST_BUFFER_NAME);
    CHECK(buffer);
    POINTERS_EQUAL(buffer, upgrade_current_buffer);

    /* Reading an object does not reset the last buffer id assigned. */
    CHECK(gui_buffer_last_id_assigned >= buffer->id);
    last_id_assigned = gui_buffer_last_id_assigned;
    infolist = infolist_new (NULL);
    CHECK(infolist);
    test_infolist_add_line_new_format (infolist, 1786886000000000LL, "line 1");
    LONGS_EQUAL(WEECHAT_RC_OK,
                upgrade_weechat_read_cb (NULL, NULL, NULL,
                                         UPGRADE_WEECHAT_TYPE_BUFFER_LINE,
                                         infolist));
    infolist_free (infolist);
    CHECK(gui_buffer_last_id_assigned == last_id_assigned);
    CHECK(buffer->own_lines->last_line);
    STRCMP_EQUAL("line 1", buffer->own_lines->last_line->data->message);

    infolist = infolist_new (NULL);
    CHECK(infolist);
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_string (ptr_item, "type", "nick"));
    CHECK(infolist_new_var_string (ptr_item, "name", "alice"));
    LONGS_EQUAL(WEECHAT_RC_OK,
                upgrade_weechat_read_cb (NULL, NULL, NULL,
                                         UPGRADE_WEECHAT_TYPE_NICKLIST,
                                         infolist));
    infolist_free (infolist);
    CHECK(gui_nicklist_search_nick (buffer, NULL, "alice"));

    infolist = infolist_new (NULL);
    CHECK(infolist);
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_string (ptr_item, "text", "cmd1"));
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_string (ptr_item, "text", "cmd2"));
    LONGS_EQUAL(WEECHAT_RC_OK,
                upgrade_weechat_read_cb (NULL, NULL, NULL,
                                         UPGRADE_WEECHAT_TYPE_HISTORY,
                                         infolist));
    infolist_free (infolist);
    CHECK(buffer->history);
    STRCMP_EQUAL("cmd2", buffer->history->text);
    STRCMP_EQUAL("cmd1", buffer->last_history->text);

    /* Hotlist. */
    infolist = infolist_new (NULL);
    CHECK(infolist);
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_integer (ptr_item, "priority",
                                    GUI_HOTLIST_HIGHLIGHT));
    LONGS_EQUAL(WEECHAT_RC_OK,
                upgrade_weechat_read_cb (NULL, NULL, NULL,
                                         UPGRADE_WEECHAT_TYPE_HOTLIST,
                                         infolist));
    infolist_free (infolist);
    LONGS_EQUAL(1, hotlist_reset);

    /* Global history (no current buffer). */
    upgrade_current_buffer = NULL;
    infolist = infolist_new (NULL);
    CHECK(infolist);
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_string (ptr_item, "text", "global_cmd"));
    LONGS_EQUAL(WEECHAT_RC_OK,
                upgrade_weechat_read_cb (NULL, NULL, NULL,
                                         UPGRADE_WEECHAT_TYPE_HISTORY,
                                         infolist));
    infolist_free (infolist);
    CHECK(gui_history);
    STRCMP_EQUAL("global_cmd", gui_history->text);
    gui_history_global_free ();

    /* Misc info. */
    infolist = infolist_new (NULL);
    CHECK(infolist);
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_time (ptr_item, "start_time", TEST_DATE));
    CHECK(infolist_new_var_integer (ptr_item, "upgrade_count", 42));
    CHECK(infolist_new_var_integer (ptr_item, "current_window_number", 3));
    LONGS_EQUAL(WEECHAT_RC_OK,
                upgrade_weechat_read_cb (NULL, NULL, NULL,
                                         UPGRADE_WEECHAT_TYPE_MISC,
                                         infolist));
    infolist_free (infolist);
    LONGS_EQUAL(TEST_DATE, weechat_first_start_time);
    LONGS_EQUAL(42, weechat_upgrade_count);
    LONGS_EQUAL(3, upgrade_set_current_window);
    weechat_first_start_time = old_first_start_time;
    weechat_upgrade_count = old_upgrade_count;

    /* Layout of windows. */
    upgrade_layout = gui_layout_alloc (GUI_LAYOUT_UPGRADE);
    CHECK(upgrade_layout);
    infolist = infolist_new (NULL);
    CHECK(infolist);
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_integer (ptr_item, "internal_id", 1));
    CHECK(infolist_new_var_integer (ptr_item, "parent_id", 0));
    CHECK(infolist_new_var_integer (ptr_item, "split_pct", 40));
    CHECK(infolist_new_var_integer (ptr_item, "split_horiz", 1));
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_integer (ptr_item, "internal_id", 2));
    CHECK(infolist_new_var_integer (ptr_item, "parent_id", 1));
    CHECK(infolist_new_var_string (ptr_item, "plugin_name", "core"));
    CHECK(infolist_new_var_string (ptr_item, "buffer_name", "weechat"));
    LONGS_EQUAL(WEECHAT_RC_OK,
                upgrade_weechat_read_cb (NULL, NULL, NULL,
                                         UPGRADE_WEECHAT_TYPE_LAYOUT_WINDOW,
                                         infolist));
    infolist_free (infolist);
    ptr_root = upgrade_layout->layout_windows;
    CHECK(ptr_root);
    LONGS_EQUAL(1, ptr_root->internal_id);
    LONGS_EQUAL(40, ptr_root->split_pct);
    LONGS_EQUAL(1, ptr_root->split_horiz);
    CHECK(ptr_root->child1);
    LONGS_EQUAL(2, ptr_root->child1->internal_id);
    STRCMP_EQUAL("core", ptr_root->child1->plugin_name);
    STRCMP_EQUAL("weechat", ptr_root->child1->buffer_name);

    /* Unknown object (ignored). */
    infolist = infolist_new (NULL);
    CHECK(infolist);
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    LONGS_EQUAL(WEECHAT_RC_OK,
                upgrade_weechat_read_cb (NULL, NULL, NULL, 999, infolist));
    infolist_free (infolist);

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   upgrade_weechat_load
 */

TEST(CoreUpgrade, Load)
{
    struct t_upgrade_file *upgrade_file;
    struct t_infolist *infolist;
    struct t_infolist_item *ptr_item;
    long long old_last_id_assigned;
    time_t old_first_start_time;
    int old_upgrade_count;

    old_first_start_time = weechat_first_start_time;
    old_upgrade_count = weechat_upgrade_count;
    old_last_id_assigned = gui_buffer_last_id_assigned;

    /* No upgrade file. */
    test_core_upgrade_remove_file (WEECHAT_UPGRADE_FILENAME);
    LONGS_EQUAL(0, upgrade_weechat_load ());
    POINTERS_EQUAL(NULL, upgrade_layout);

    /* Upgrade file with global history and misc info. */
    upgrade_file = upgrade_file_new (WEECHAT_UPGRADE_FILENAME,
                                     NULL, NULL, NULL);
    CHECK(upgrade_file);
    infolist = infolist_new (NULL);
    CHECK(infolist);
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_string (ptr_item, "text", "global_cmd"));
    CHECK(upgrade_file_write_object (upgrade_file,
                                     UPGRADE_WEECHAT_TYPE_HISTORY,
                                     infolist));
    infolist_free (infolist);
    infolist = infolist_new (NULL);
    CHECK(infolist);
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_time (ptr_item, "start_time", TEST_DATE));
    CHECK(infolist_new_var_integer (ptr_item, "upgrade_count", 42));
    CHECK(infolist_new_var_integer (ptr_item, "current_window_number",
                                    gui_current_window->number));
    CHECK(upgrade_file_write_object (upgrade_file,
                                     UPGRADE_WEECHAT_TYPE_MISC,
                                     infolist));
    infolist_free (infolist);
    upgrade_file_close (upgrade_file);

    LONGS_EQUAL(1, upgrade_weechat_load ());
    POINTERS_EQUAL(NULL, upgrade_layout);
    /* No buffer in upgrade file: last buffer id assigned is reset. */
    CHECK(gui_buffer_last_id_assigned == -1);
    LONGS_EQUAL(TEST_DATE, weechat_first_start_time);
    LONGS_EQUAL(42, weechat_upgrade_count);
    CHECK(gui_history);
    STRCMP_EQUAL("global_cmd", gui_history->text);

    gui_history_global_free ();
    weechat_first_start_time = old_first_start_time;
    weechat_upgrade_count = old_upgrade_count;
    gui_buffer_last_id_assigned = old_last_id_assigned;
    test_core_upgrade_remove_file (WEECHAT_UPGRADE_FILENAME);
}

/*
 * Test functions:
 *   upgrade_weechat_remove_file_cb
 *   upgrade_weechat_end
 */

TEST(CoreUpgrade, End)
{
    char *path_upgrade, *path_other;
    FILE *file;

    CHECK(string_asprintf (&path_upgrade, "%s/test_end.upgrade",
                           weechat_data_dir) >= 0);
    CHECK(string_asprintf (&path_other, "%s/test_end.txt",
                           weechat_data_dir) >= 0);

    /* Only "*.upgrade" files are removed. */
    file = fopen (path_upgrade, "w");
    CHECK(file);
    fclose (file);
    file = fopen (path_other, "w");
    CHECK(file);
    fclose (file);
    upgrade_weechat_remove_file_cb (NULL, path_other);
    LONGS_EQUAL(0, access (path_other, F_OK));
    upgrade_weechat_remove_file_cb (NULL, path_upgrade);
    LONGS_EQUAL(-1, access (path_upgrade, F_OK));

    /* End of upgrade: remove "*.upgrade" files and display a message. */
    file = fopen (path_upgrade, "w");
    CHECK(file);
    fclose (file);
    weechat_upgrading = 1;
    record_start ();
    upgrade_weechat_end ();
    record_stop ();
    LONGS_EQUAL(0, weechat_upgrading);
    LONGS_EQUAL(-1, access (path_upgrade, F_OK));
    LONGS_EQUAL(0, access (path_other, F_OK));
    RECORD_CHECK_MSG_REGEX("core.weechat", "^Upgrade done \\(");

    unlink (path_other);
    free (path_upgrade);
    free (path_other);
}

/*
 * Callback for signal "upgrade": return an error, so that the upgrade is
 * aborted before plugins are unloaded.
 */

int
test_core_upgrade_signal_error_cb (const void *pointer, void *data,
                                   const char *signal, const char *type_data,
                                   void *signal_data)
{
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    return WEECHAT_RC_ERROR;
}

/*
 * Test functions:
 *   upgrade_weechat_check_background
 *   upgrade_weechat_schedule
 *   upgrade_weechat_cancel
 *   upgrade_weechat_exec
 */

TEST(CoreUpgrade, Schedule)
{
    LONGS_EQUAL(1, upgrade_weechat_check_background ());

    LONGS_EQUAL(0, upgrade_pending);
    POINTERS_EQUAL(NULL, upgrade_pending_binary);
    LONGS_EQUAL(0, upgrade_pending_quit);

    /* Nothing is done if no upgrade is scheduled. */
    upgrade_weechat_exec ();
    LONGS_EQUAL(0, upgrade_pending);
    LONGS_EQUAL(0, weechat_quit);
    LONGS_EQUAL(0, weechat_upgrading);

    upgrade_weechat_schedule (strdup ("/path/to/weechat"), 0);
    LONGS_EQUAL(1, upgrade_pending);
    STRCMP_EQUAL("/path/to/weechat", upgrade_pending_binary);
    LONGS_EQUAL(0, upgrade_pending_quit);

    /* A new schedule replaces the previous one. */
    upgrade_weechat_schedule (NULL, 1);
    LONGS_EQUAL(1, upgrade_pending);
    POINTERS_EQUAL(NULL, upgrade_pending_binary);
    LONGS_EQUAL(1, upgrade_pending_quit);

    upgrade_weechat_cancel ();
    LONGS_EQUAL(0, upgrade_pending);
    POINTERS_EQUAL(NULL, upgrade_pending_binary);
    LONGS_EQUAL(0, upgrade_pending_quit);
}

/*
 * Test functions:
 *   upgrade_weechat_exec
 */

TEST(CoreUpgrade, ExecAborted)
{
    struct t_hook *ptr_hook;

    /* Signal "upgrade" returns an error: upgrade is aborted. */
    ptr_hook = hook_signal (NULL, "999999|upgrade",
                            &test_core_upgrade_signal_error_cb, NULL, NULL);
    CHECK(ptr_hook);

    upgrade_weechat_schedule (strdup ("/path/to/weechat"), 0);
    record_start ();
    upgrade_weechat_exec ();
    record_stop ();
    LONGS_EQUAL(0, upgrade_pending);
    POINTERS_EQUAL(NULL, upgrade_pending_binary);
    LONGS_EQUAL(0, upgrade_pending_quit);
    LONGS_EQUAL(0, weechat_quit);
    LONGS_EQUAL(0, weechat_upgrading);
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "Unable to save some plugin sessions (files *.upgrade)",
                     NULL);
    RECORD_CHECK_MSG("core.weechat", "=!=", "Upgrade aborted", NULL);

    unhook (ptr_hook);
}
