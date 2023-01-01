/*
 * test-gui-bar-item-custom.cpp - test custom bar item functions
 *
 * Copyright (C) 2022-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include <string.h>
#include "src/core/wee-config.h"
#include "src/gui/gui-bar-item.h"
#include "src/gui/gui-bar-item-custom.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-window.h"

extern int gui_bar_item_custom_name_valid (const char *name);
extern struct t_gui_bar_item_custom *gui_bar_item_custom_search_with_option_name (
    const char *option_name);
extern char *gui_bar_item_custom_callback (const void *pointer,
                                           void *data,
                                           struct t_gui_bar_item *item,
                                           struct t_gui_window *window,
                                           struct t_gui_buffer *buffer,
                                           struct t_hashtable *extra_info);
extern void gui_bar_item_custom_create_bar_item (struct t_gui_bar_item_custom *item);
}

TEST_GROUP(GuiBarItemCustom)
{
};

/*
 * Tests functions:
 *   gui_bar_item_custom_name_valid
 */

TEST(GuiBarItemCustom, NameValid)
{
    LONGS_EQUAL(0, gui_bar_item_custom_name_valid (NULL));
    LONGS_EQUAL(0, gui_bar_item_custom_name_valid (""));
    LONGS_EQUAL(0, gui_bar_item_custom_name_valid (" "));
    LONGS_EQUAL(0, gui_bar_item_custom_name_valid ("."));
    LONGS_EQUAL(0, gui_bar_item_custom_name_valid ("abc def"));
    LONGS_EQUAL(0, gui_bar_item_custom_name_valid (" abcdef"));
    LONGS_EQUAL(0, gui_bar_item_custom_name_valid ("abcdef "));
    LONGS_EQUAL(0, gui_bar_item_custom_name_valid ("abc.def"));
    LONGS_EQUAL(0, gui_bar_item_custom_name_valid (".abcdef"));
    LONGS_EQUAL(0, gui_bar_item_custom_name_valid ("abcdef."));

    LONGS_EQUAL(1, gui_bar_item_custom_name_valid ("?"));
    LONGS_EQUAL(1, gui_bar_item_custom_name_valid ("abc-def"));
    LONGS_EQUAL(1, gui_bar_item_custom_name_valid ("abc-def"));
    LONGS_EQUAL(1, gui_bar_item_custom_name_valid ("abc/def/"));
    LONGS_EQUAL(1, gui_bar_item_custom_name_valid ("abcdef"));
}

/*
 * Tests functions:
 *   gui_bar_item_custom_search_option
 */

TEST(GuiBarItemCustom, SearchOption)
{
    LONGS_EQUAL(-1, gui_bar_item_custom_search_option (NULL));
    LONGS_EQUAL(-1, gui_bar_item_custom_search_option (""));
    LONGS_EQUAL(-1, gui_bar_item_custom_search_option ("zzz"));

    LONGS_EQUAL(0, gui_bar_item_custom_search_option ("conditions"));
    LONGS_EQUAL(1, gui_bar_item_custom_search_option ("content"));
}

/*
 * Tests functions:
 *   gui_bar_item_custom_search
 */

TEST(GuiBarItemCustom, Search)
{
    struct t_gui_bar_item_custom *new_item, *new_item2, *ptr_item;

    new_item = gui_bar_item_custom_new ("test", "${buffer.number} == 1",
                                        "some content");
    CHECK(new_item);

    new_item2 = gui_bar_item_custom_new ("test2", "${buffer.number} == 2",
                                         "some content 2");
    CHECK(new_item2);

    POINTERS_EQUAL(NULL, gui_bar_item_custom_search (NULL));
    POINTERS_EQUAL(NULL, gui_bar_item_custom_search (""));
    POINTERS_EQUAL(NULL, gui_bar_item_custom_search ("zzz"));

    ptr_item = gui_bar_item_custom_search ("test");
    POINTERS_EQUAL(new_item, ptr_item);
    STRCMP_EQUAL("test", ptr_item->name);
    STRCMP_EQUAL("${buffer.number} == 1",
                 CONFIG_STRING(ptr_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONDITIONS]));
    STRCMP_EQUAL("some content",
                 CONFIG_STRING(ptr_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONTENT]));
    CHECK(ptr_item->bar_item);
    POINTERS_EQUAL(NULL, ptr_item->bar_item->plugin);
    STRCMP_EQUAL("test", ptr_item->bar_item->name);
    POINTERS_EQUAL(&gui_bar_item_custom_callback, ptr_item->bar_item->build_callback);
    POINTERS_EQUAL(ptr_item, ptr_item->bar_item->build_callback_pointer);
    POINTERS_EQUAL(NULL, ptr_item->bar_item->build_callback_data);

    ptr_item = gui_bar_item_custom_search ("test2");
    POINTERS_EQUAL(new_item2, ptr_item);
    STRCMP_EQUAL("test2", ptr_item->name);
    STRCMP_EQUAL("${buffer.number} == 2",
                 CONFIG_STRING(ptr_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONDITIONS]));
    STRCMP_EQUAL("some content 2",
                 CONFIG_STRING(ptr_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONTENT]));
    CHECK(ptr_item->bar_item);
    POINTERS_EQUAL(NULL, ptr_item->bar_item->plugin);
    STRCMP_EQUAL("test2", ptr_item->bar_item->name);
    POINTERS_EQUAL(&gui_bar_item_custom_callback, ptr_item->bar_item->build_callback);
    POINTERS_EQUAL(ptr_item, ptr_item->bar_item->build_callback_pointer);
    POINTERS_EQUAL(NULL, ptr_item->bar_item->build_callback_data);

    gui_bar_item_custom_free (new_item);
    gui_bar_item_custom_free (new_item2);
}

/*
 * Tests functions:
 *   gui_bar_item_custom_search_with_option_name
 */

TEST(GuiBarItemCustom, SearchWithOptionName)
{
    struct t_gui_bar_item_custom *new_item, *new_item2;

    new_item = gui_bar_item_custom_new ("test", "${buffer.number} == 1",
                                        "some content");
    CHECK(new_item);

    new_item2 = gui_bar_item_custom_new ("test2", "${buffer.number} == 2",
                                         "some content 2");
    CHECK(new_item2);

    POINTERS_EQUAL(NULL, gui_bar_item_custom_search_with_option_name (NULL));
    POINTERS_EQUAL(NULL, gui_bar_item_custom_search_with_option_name (""));
    POINTERS_EQUAL(NULL, gui_bar_item_custom_search_with_option_name ("test"));
    POINTERS_EQUAL(NULL, gui_bar_item_custom_search_with_option_name ("test2"));
    POINTERS_EQUAL(NULL, gui_bar_item_custom_search_with_option_name ("conditions"));
    POINTERS_EQUAL(NULL, gui_bar_item_custom_search_with_option_name ("content"));

    POINTERS_EQUAL(new_item, gui_bar_item_custom_search_with_option_name ("test.conditions"));
    POINTERS_EQUAL(new_item, gui_bar_item_custom_search_with_option_name ("test.content"));

    POINTERS_EQUAL(new_item2, gui_bar_item_custom_search_with_option_name ("test2.conditions"));
    POINTERS_EQUAL(new_item2, gui_bar_item_custom_search_with_option_name ("test2.content"));

    gui_bar_item_custom_free (new_item);
    gui_bar_item_custom_free (new_item2);
}

/*
 * Tests functions:
 *   gui_bar_item_custom_config_change
 */

TEST(GuiBarItemCustom, ConfigChange)
{
    struct t_gui_bar_item_custom *new_item;

    new_item = gui_bar_item_custom_new ("test", "${buffer.number} == 1",
                                        "some content");
    CHECK(new_item);
    STRCMP_EQUAL("${buffer.number} == 1",
                 CONFIG_STRING(new_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONDITIONS]));
    STRCMP_EQUAL("some content",
                 CONFIG_STRING(new_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONTENT]));

    config_file_option_set (new_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONDITIONS],
                            "${buffer.number} == 2", 1);
    STRCMP_EQUAL("${buffer.number} == 2",
                 CONFIG_STRING(new_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONDITIONS]));

    config_file_option_set (new_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONTENT],
                            "new content", 1);
    STRCMP_EQUAL("new content",
                 CONFIG_STRING(new_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONTENT]));

    gui_bar_item_custom_free (new_item);
}

/*
 * Tests functions:
 *   gui_bar_item_custom_create_option_temp
 */

TEST(GuiBarItemCustom, CreateOptionTemp)
{
    struct t_gui_bar_item_custom *new_item;

    POINTERS_EQUAL(NULL, gui_custom_bar_items);
    POINTERS_EQUAL(NULL, last_gui_custom_bar_item);

    new_item = gui_bar_item_custom_alloc ("test");
    CHECK(new_item);

    POINTERS_EQUAL(NULL, new_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONDITIONS]);
    POINTERS_EQUAL(NULL, new_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONTENT]);

    gui_bar_item_custom_create_option_temp (new_item,
                                            GUI_BAR_ITEM_CUSTOM_OPTION_CONDITIONS,
                                            "${buffer.number} == 1");
    STRCMP_EQUAL("${buffer.number} == 1",
                 CONFIG_STRING(new_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONDITIONS]));

    gui_bar_item_custom_create_option_temp (new_item,
                                            GUI_BAR_ITEM_CUSTOM_OPTION_CONTENT,
                                            "some content");
    STRCMP_EQUAL("some content",
                 CONFIG_STRING(new_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONTENT]));

    gui_bar_item_custom_free (new_item);
}

/*
 * Tests functions:
 *   gui_bar_item_custom_callback
 */

TEST(GuiBarItemCustom, Callback)
{
    struct t_gui_bar_item_custom *new_item;
    char *content;

    new_item = gui_bar_item_custom_new (
        "test",
        "${buffer.number} == 1",
        "${buffer.number} >> ${buffer.full_name}");
    CHECK(new_item);

    /* custom bar item is NULL => no content */
    POINTERS_EQUAL(
        NULL,
        gui_bar_item_custom_callback (NULL, NULL, new_item->bar_item,
                                      gui_windows, gui_buffers, NULL));

    content = gui_bar_item_custom_callback (new_item, NULL, new_item->bar_item,
                                            gui_windows, gui_buffers, NULL);
    STRCMP_EQUAL("1 >> core.weechat", content);
    free (content);

    /* change conditions so that it becomes false on first buffer */
    config_file_option_set (new_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONDITIONS],
                            "${buffer.number} == 2", 1);
    POINTERS_EQUAL(NULL,
                   gui_bar_item_custom_callback (new_item, NULL,
                                                 new_item->bar_item,
                                                 gui_windows, gui_buffers,
                                                 NULL));

    gui_bar_item_custom_free (new_item);
}

/*
 * Tests functions:
 *   gui_bar_item_custom_alloc
 *   gui_bar_item_custom_create_bar_item
 */

TEST(GuiBarItemCustom, Alloc)
{
    struct t_gui_bar_item_custom *new_item;
    int i;

    new_item = gui_bar_item_custom_alloc ("test");
    CHECK(new_item);
    STRCMP_EQUAL("test", new_item->name);
    for (i = 0; i < GUI_BAR_ITEM_CUSTOM_NUM_OPTIONS; i++)
    {
        POINTERS_EQUAL(NULL, new_item->options[i]);
    }
    POINTERS_EQUAL(NULL, new_item->bar_item);
    POINTERS_EQUAL(NULL, new_item->prev_item);
    POINTERS_EQUAL(NULL, new_item->next_item);

    gui_bar_item_custom_create_bar_item (new_item);

    /* do it again to free the bar item then reallocate it */
    gui_bar_item_custom_create_bar_item (new_item);

    CHECK(new_item->bar_item);
    POINTERS_EQUAL(NULL, new_item->bar_item->plugin);
    STRCMP_EQUAL("test", new_item->bar_item->name);
    POINTERS_EQUAL(&gui_bar_item_custom_callback,
                   new_item->bar_item->build_callback);
    POINTERS_EQUAL(new_item, new_item->bar_item->build_callback_pointer);
    POINTERS_EQUAL(NULL, new_item->bar_item->build_callback_data);

    gui_bar_item_custom_free (new_item);
}

/*
 * Tests functions:
 *   gui_bar_item_custom_new
 *   gui_bar_item_custom_new_with_options
 *   gui_bar_item_custom_create_option
 */

TEST(GuiBarItemCustom, New)
{
    struct t_gui_bar_item_custom *new_item, *new_item2;

    /* invalid name: contains a space */
    POINTERS_EQUAL(NULL,
                   gui_bar_item_custom_new ("test item",
                                            "${buffer.number} == 1",
                                            "some content"));

    new_item = gui_bar_item_custom_new ("test", "${buffer.number} == 1",
                                        "some content");
    CHECK(new_item);

    STRCMP_EQUAL("test", new_item->name);
    STRCMP_EQUAL(
        "${buffer.number} == 1",
        CONFIG_STRING(new_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONDITIONS]));
    STRCMP_EQUAL(
        "some content",
        CONFIG_STRING(new_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONTENT]));
    CHECK(new_item->bar_item);
    POINTERS_EQUAL(NULL, new_item->bar_item->plugin);
    STRCMP_EQUAL("test", new_item->bar_item->name);
    POINTERS_EQUAL(&gui_bar_item_custom_callback,
                   new_item->bar_item->build_callback);
    POINTERS_EQUAL(new_item, new_item->bar_item->build_callback_pointer);
    POINTERS_EQUAL(NULL, new_item->bar_item->build_callback_data);
    POINTERS_EQUAL(NULL, new_item->prev_item);
    POINTERS_EQUAL(NULL, new_item->next_item);

    /* invalid name: already exists */
    POINTERS_EQUAL(NULL, gui_bar_item_custom_new ("test",
                                                  "${buffer.number} == 1",
                                                  "some content"));

    /* add another item */
    new_item2 = gui_bar_item_custom_new ("test2", "${buffer.number} == 2",
                                         "some content 2");
    CHECK(new_item2);

    POINTERS_EQUAL(NULL, new_item->prev_item);
    POINTERS_EQUAL(new_item2, new_item->next_item);

    STRCMP_EQUAL("test2", new_item2->name);
    STRCMP_EQUAL(
        "${buffer.number} == 2",
        CONFIG_STRING(new_item2->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONDITIONS]));
    STRCMP_EQUAL(
        "some content 2",
        CONFIG_STRING(new_item2->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONTENT]));
    CHECK(new_item2->bar_item);
    POINTERS_EQUAL(NULL, new_item2->bar_item->plugin);
    STRCMP_EQUAL("test2", new_item2->bar_item->name);
    POINTERS_EQUAL(&gui_bar_item_custom_callback,
                   new_item2->bar_item->build_callback);
    POINTERS_EQUAL(new_item2, new_item2->bar_item->build_callback_pointer);
    POINTERS_EQUAL(NULL, new_item2->bar_item->build_callback_data);
    POINTERS_EQUAL(new_item, new_item2->prev_item);
    POINTERS_EQUAL(NULL, new_item2->next_item);

    gui_bar_item_custom_free (new_item);
    gui_bar_item_custom_free (new_item2);
}

/*
 * Tests functions:
 *   gui_bar_item_custom_use_temp_items
 */

TEST(GuiBarItemCustom, UseTempItems)
{
    struct t_gui_bar_item_custom *new_item, *new_item2;

    new_item = gui_bar_item_custom_alloc ("test");
    gui_temp_custom_bar_items = new_item;
    last_gui_temp_custom_bar_item = new_item;

    new_item2 = gui_bar_item_custom_alloc ("test2");
    new_item->next_item = new_item2;
    new_item2->prev_item = new_item;
    last_gui_temp_custom_bar_item = new_item2;

    gui_bar_item_custom_use_temp_items ();

    POINTERS_EQUAL(NULL, gui_temp_custom_bar_items);
    POINTERS_EQUAL(NULL, last_gui_temp_custom_bar_item);

    POINTERS_EQUAL(new_item, gui_custom_bar_items);
    POINTERS_EQUAL(new_item2, last_gui_custom_bar_item);

    gui_bar_item_custom_free (new_item);
    gui_bar_item_custom_free (new_item2);

    POINTERS_EQUAL(NULL, gui_custom_bar_items);
    POINTERS_EQUAL(NULL, last_gui_custom_bar_item);
}

/*
 * Tests functions:
 *   gui_bar_item_custom_rename
 */

TEST(GuiBarItemCustom, Rename)
{
    struct t_gui_bar_item_custom *new_item, *new_item2;

    new_item = gui_bar_item_custom_new ("test", "${buffer.number} == 1",
                                        "some content");
    new_item2 = gui_bar_item_custom_new ("test2", "${buffer.number} == 2",
                                         "some content 2");

    CHECK(new_item);
    STRCMP_EQUAL("test", new_item->name);
    CHECK(new_item->bar_item);
    STRCMP_EQUAL("test", new_item->bar_item->name);

    CHECK(new_item2);
    STRCMP_EQUAL("test2", new_item2->name);
    CHECK(new_item2->bar_item);
    STRCMP_EQUAL("test2", new_item2->bar_item->name);

    /* invalid name: contains a space */
    LONGS_EQUAL (0, gui_bar_item_custom_rename (new_item, "second test"));

    /* invalid name: custom bar item already exists */
    LONGS_EQUAL(0, gui_bar_item_custom_rename (new_item, "test2"));

    /* rename OK */
    LONGS_EQUAL(1, gui_bar_item_custom_rename (new_item, "test3"));

    STRCMP_EQUAL("test3", new_item->name);
    CHECK(new_item->bar_item);
    STRCMP_EQUAL("test3", new_item->bar_item->name);

    gui_bar_item_custom_free (new_item);
    gui_bar_item_custom_free (new_item2);
}

/*
 * Tests functions:
 *   gui_bar_item_custom_free
 *   gui_bar_item_custom_free_all
 */

TEST(GuiBarItemCustom, Free)
{
    struct t_gui_bar_item_custom *new_item, *new_item2;

    POINTERS_EQUAL(NULL, gui_custom_bar_items);
    POINTERS_EQUAL(NULL, last_gui_custom_bar_item);

    gui_bar_item_custom_free (NULL);

    new_item = gui_bar_item_custom_new ("test", "${buffer.number} == 1",
                                        "some content");
    POINTERS_EQUAL(new_item, gui_custom_bar_items);
    POINTERS_EQUAL(new_item, last_gui_custom_bar_item);

    new_item2 = gui_bar_item_custom_new ("test2", "${buffer.number} == 2",
                                         "some content 2");
    POINTERS_EQUAL(new_item, gui_custom_bar_items);
    POINTERS_EQUAL(new_item2, last_gui_custom_bar_item);

    gui_bar_item_custom_free (new_item);
    POINTERS_EQUAL(new_item2, gui_custom_bar_items);
    POINTERS_EQUAL(new_item2, last_gui_custom_bar_item);

    gui_bar_item_custom_free (new_item2);
    POINTERS_EQUAL(NULL, gui_custom_bar_items);
    POINTERS_EQUAL(NULL, last_gui_custom_bar_item);

    new_item = gui_bar_item_custom_new ("test", "${buffer.number} == 1",
                                        "some content");
    new_item2 = gui_bar_item_custom_new ("test2", "${buffer.number} == 2",
                                         "some content 2");
    POINTERS_EQUAL(new_item, gui_custom_bar_items);
    POINTERS_EQUAL(new_item2, last_gui_custom_bar_item);

    gui_bar_item_custom_free_all ();
    POINTERS_EQUAL(NULL, gui_custom_bar_items);
    POINTERS_EQUAL(NULL, last_gui_custom_bar_item);

    /* remove items in reverse order */
    new_item = gui_bar_item_custom_new ("test", "${buffer.number} == 1",
                                        "some content");
    new_item2 = gui_bar_item_custom_new ("test2", "${buffer.number} == 2",
                                         "some content 2");
    gui_bar_item_custom_free (new_item2);
    gui_bar_item_custom_free (new_item);
}
