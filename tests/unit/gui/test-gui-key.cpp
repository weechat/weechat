/*
 * test-gui-key.cpp - test key functions
 *
 * Copyright (C) 2023 Sébastien Helleu <flashcode@flashtux.org>
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

#include "tests/tests.h"

extern "C"
{
#include "src/gui/gui-key.h"

extern char *gui_key_legacy_internal_code (const char *key);
}

#define WEE_CHECK_EXP_KEY(__rc, __key_name, __key_name_alias, __key)    \
    key_name = NULL;                                                    \
    key_name_alias = NULL;                                              \
    LONGS_EQUAL(__rc,                                                   \
                gui_key_expand (__key, &key_name, &key_name_alias));    \
    if (__key_name == NULL)                                             \
    {                                                                   \
        POINTERS_EQUAL(NULL, key_name);                                 \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__key_name, key_name);                             \
    }                                                                   \
    if (__key_name_alias == NULL)                                       \
    {                                                                   \
        POINTERS_EQUAL(NULL, key_name_alias);                           \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__key_name_alias, key_name_alias);                 \
    }                                                                   \
    if (key_name)                                                       \
        free (key_name);                                                \
    if (key_name_alias)                                                 \
        free (key_name_alias);

TEST_GROUP(GuiKey)
{
};

/*
 * Tests functions:
 *   gui_key_init
 */

TEST(GuiKey, Init)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_search_context
 */

TEST(GuiKey, SearchContext)
{
    LONGS_EQUAL(-1, gui_key_search_context (NULL));
    LONGS_EQUAL(-1, gui_key_search_context (""));
    LONGS_EQUAL(-1, gui_key_search_context ("invalid"));

    LONGS_EQUAL(0, gui_key_search_context ("default"));
    LONGS_EQUAL(1, gui_key_search_context ("search"));
    LONGS_EQUAL(2, gui_key_search_context ("cursor"));
    LONGS_EQUAL(3, gui_key_search_context ("mouse"));
}

/*
 * Tests functions:
 *   gui_key_get_current_context
 */

TEST(GuiKey, GetCurrentContext)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_grab_init
 */

TEST(GuiKey, GrabInit)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_grab_end_timer_cb
 */

TEST(GuiKey, GrabEndTimerCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_legacy_internal_code
 */

TEST(GuiKey, LegacyInternalCode)
{
    char *str;

    WEE_TEST_STR(NULL, gui_key_legacy_internal_code (NULL));
    WEE_TEST_STR("", gui_key_legacy_internal_code (""));
    WEE_TEST_STR("A", gui_key_legacy_internal_code ("A"));
    WEE_TEST_STR("a", gui_key_legacy_internal_code ("a"));

    WEE_TEST_STR("@chat:t", gui_key_legacy_internal_code ("@chat:t"));

    WEE_TEST_STR("\001[A", gui_key_legacy_internal_code ("meta-A"));
    WEE_TEST_STR("\001[a", gui_key_legacy_internal_code ("meta-a"));

    WEE_TEST_STR("\001[[A", gui_key_legacy_internal_code ("meta2-A"));
    WEE_TEST_STR("\001[[a", gui_key_legacy_internal_code ("meta2-a"));

    /* ctrl-letter keys are forced to lower case */
    WEE_TEST_STR("\001a", gui_key_legacy_internal_code ("ctrl-A"));
    WEE_TEST_STR("\001a", gui_key_legacy_internal_code ("ctrl-a"));

    WEE_TEST_STR(" ", gui_key_legacy_internal_code ("space"));
}

/*
 * Tests functions:
 *   gui_key_expand
 */

TEST(GuiKey, Expand)
{
    char *key_name, *key_name_alias;

    /* NULL key */
    WEE_CHECK_EXP_KEY(0, NULL, NULL, NULL);

    /* empty key */
    WEE_CHECK_EXP_KEY(1, "", "", "");

    /* NULL key names */
    LONGS_EQUAL(1, gui_key_expand ("a", NULL, NULL));

    /* invalid keys: incomplete */
    WEE_CHECK_EXP_KEY(0, NULL, NULL, "\001");
    WEE_CHECK_EXP_KEY(0, NULL, NULL, "\001[");
    WEE_CHECK_EXP_KEY(0, NULL, NULL, "\001[O");
    WEE_CHECK_EXP_KEY(0, NULL, NULL, "\001[[");
    WEE_CHECK_EXP_KEY(0, NULL, NULL, "\001[[O");
    WEE_CHECK_EXP_KEY(0, NULL, NULL, "\001[[1");
    WEE_CHECK_EXP_KEY(0, NULL, NULL, "\001[[12");
    WEE_CHECK_EXP_KEY(0, NULL, NULL, "\001[[123");
    WEE_CHECK_EXP_KEY(0, NULL, NULL, "\001[[1;");
    WEE_CHECK_EXP_KEY(0, NULL, NULL, "\001[[1;2");
    WEE_CHECK_EXP_KEY(0, NULL, NULL, "\001[[2;3");
    WEE_CHECK_EXP_KEY(0, NULL, NULL, "\001[[15;");
    WEE_CHECK_EXP_KEY(0, NULL, NULL, "\001[[15;1");
    WEE_CHECK_EXP_KEY(0, NULL, NULL, "\001[[[");

    /* unknown sequence: kept as-is */
    WEE_CHECK_EXP_KEY(1, "meta2-x", "meta2-x", "\001[[x");
    WEE_CHECK_EXP_KEY(1, "meta2-é", "meta2-é", "\001[[é");

    WEE_CHECK_EXP_KEY(1, "A", "A", "A");
    WEE_CHECK_EXP_KEY(1, "a", "a", "a");
    WEE_CHECK_EXP_KEY(1, "space", "space", " ");
    WEE_CHECK_EXP_KEY(1, "comma", "comma", ",");

    /* ctrl + key */
    WEE_CHECK_EXP_KEY(1, "ctrl-a", "ctrl-a", "\001a");
    WEE_CHECK_EXP_KEY(1, "ctrl-h", "backspace", "\001h");
    WEE_CHECK_EXP_KEY(1, "ctrl-?", "backspace", "\001?");
    WEE_CHECK_EXP_KEY(1, "ctrl-i", "tab", "\001i");
    WEE_CHECK_EXP_KEY(1, "ctrl-j", "return", "\001j");
    WEE_CHECK_EXP_KEY(1, "ctrl-m", "return", "\001m");
    WEE_CHECK_EXP_KEY(1, "ctrl-z", "ctrl-z", "\001z");
    WEE_CHECK_EXP_KEY(1, "ctrl-_", "ctrl-_", "\001_");

    /* ctrl + key with upper case letter (auto-converted to lower case) */
    WEE_CHECK_EXP_KEY(1, "ctrl-a", "ctrl-a", "\001A");
    WEE_CHECK_EXP_KEY(1, "ctrl-h", "backspace", "\001H");
    WEE_CHECK_EXP_KEY(1, "ctrl-i", "tab", "\001I");
    WEE_CHECK_EXP_KEY(1, "ctrl-j", "return", "\001J");
    WEE_CHECK_EXP_KEY(1, "ctrl-m", "return", "\001M");
    WEE_CHECK_EXP_KEY(1, "ctrl-z", "ctrl-z", "\001Z");

    /* ctrl + key then other letter */
    WEE_CHECK_EXP_KEY(1, "ctrl-c,b", "ctrl-c,b", "\001cb");
    WEE_CHECK_EXP_KEY(1, "ctrl-c,_", "ctrl-c,_", "\001c_");

    /* alt + ctrl + key */
    WEE_CHECK_EXP_KEY(1, "meta-ctrl-a", "meta-ctrl-a", "\001[\001a");
    WEE_CHECK_EXP_KEY(1, "meta-ctrl-h", "meta-backspace", "\001[\001h");
    WEE_CHECK_EXP_KEY(1, "meta-ctrl-?", "meta-backspace", "\001[\001?");
    WEE_CHECK_EXP_KEY(1, "meta-ctrl-i", "meta-tab", "\001[\001i");
    WEE_CHECK_EXP_KEY(1, "meta-ctrl-j", "meta-return", "\001[\001j");
    WEE_CHECK_EXP_KEY(1, "meta-ctrl-m", "meta-return", "\001[\001m");
    WEE_CHECK_EXP_KEY(1, "meta-ctrl-z", "meta-ctrl-z", "\001[\001z");
    WEE_CHECK_EXP_KEY(1, "meta-ctrl-_", "meta-ctrl-_", "\001[\001_");

    /* alt + key */
    WEE_CHECK_EXP_KEY(1, "meta-A", "meta-A", "\001[A");
    WEE_CHECK_EXP_KEY(1, "meta-a", "meta-a", "\001[a");
    WEE_CHECK_EXP_KEY(1, "meta-É", "meta-É", "\001[É");
    WEE_CHECK_EXP_KEY(1, "meta-é", "meta-é", "\001[é");
    WEE_CHECK_EXP_KEY(1, "meta-Z", "meta-Z", "\001[Z");
    WEE_CHECK_EXP_KEY(1, "meta-z", "meta-z", "\001[z");
    WEE_CHECK_EXP_KEY(1, "meta-_", "meta-_", "\001[_");

    /* 2 * alt + key */
    WEE_CHECK_EXP_KEY(1, "meta-meta-A", "meta-meta-A", "\001[\001[A");
    WEE_CHECK_EXP_KEY(1, "meta-meta-a", "meta-meta-a", "\001[\001[a");
    WEE_CHECK_EXP_KEY(1, "meta-meta-É", "meta-meta-É", "\001[\001[É");
    WEE_CHECK_EXP_KEY(1, "meta-meta-é", "meta-meta-é", "\001[\001[é");
    WEE_CHECK_EXP_KEY(1, "meta-meta-Z", "meta-meta-Z", "\001[\001[Z");
    WEE_CHECK_EXP_KEY(1, "meta-meta-z", "meta-meta-z", "\001[\001[z");
    WEE_CHECK_EXP_KEY(1, "meta-meta-_", "meta-meta-_", "\001[\001[_");

    /* 3 * alt + key */
    WEE_CHECK_EXP_KEY(1, "meta-meta-meta-A", "meta-meta-meta-A", "\001[\001[\001[A");
    WEE_CHECK_EXP_KEY(1, "meta-meta-meta-a", "meta-meta-meta-a", "\001[\001[\001[a");
    WEE_CHECK_EXP_KEY(1, "meta-meta-meta-É", "meta-meta-meta-É", "\001[\001[\001[É");
    WEE_CHECK_EXP_KEY(1, "meta-meta-meta-é", "meta-meta-meta-é", "\001[\001[\001[é");
    WEE_CHECK_EXP_KEY(1, "meta-meta-meta-Z", "meta-meta-meta-Z", "\001[\001[\001[Z");
    WEE_CHECK_EXP_KEY(1, "meta-meta-meta-z", "meta-meta-meta-z", "\001[\001[\001[z");
    WEE_CHECK_EXP_KEY(1, "meta-meta-meta-_", "meta-meta-meta-_", "\001[\001[\001[_");

    /* alt + key (meta2) */
    WEE_CHECK_EXP_KEY(1, "meta2-Z", "shift-tab", "\001[[Z");

    /* arrows */
    WEE_CHECK_EXP_KEY(1, "meta2-A", "up", "\001[[A");
    WEE_CHECK_EXP_KEY(1, "meta2-B", "down", "\001[[B");
    WEE_CHECK_EXP_KEY(1, "meta2-C", "right", "\001[[C");
    WEE_CHECK_EXP_KEY(1, "meta2-D", "left", "\001[[D");

    /* shift + arrows, modifier = 2: 1 + 1=shift */
    WEE_CHECK_EXP_KEY(1, "meta2-1;2A", "shift-up", "\001[[1;2A");
    WEE_CHECK_EXP_KEY(1, "meta2-1;2B", "shift-down", "\001[[1;2B");
    WEE_CHECK_EXP_KEY(1, "meta2-1;2C", "shift-right", "\001[[1;2C");
    WEE_CHECK_EXP_KEY(1, "meta2-1;2D", "shift-left", "\001[[1;2D");

    /* alt + arrows, modifier = 3: 1 + 2=alt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;3A", "meta-up", "\001[[1;3A");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-A", "meta-up", "\001[\001[[A");
    WEE_CHECK_EXP_KEY(1, "meta2-1;3B", "meta-down", "\001[[1;3B");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-B", "meta-down", "\001[\001[[B");
    WEE_CHECK_EXP_KEY(1, "meta2-1;3C", "meta-right", "\001[[1;3C");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-C", "meta-right", "\001[\001[[C");
    WEE_CHECK_EXP_KEY(1, "meta2-1;3D", "meta-left", "\001[[1;3D");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-D", "meta-left", "\001[\001[[D");

    /* 2 * alt + arrows, modifier = 3: 1 + 2=alt */
    WEE_CHECK_EXP_KEY(1, "meta-meta2-1;3A", "meta-meta-up", "\001[\001[[1;3A");
    WEE_CHECK_EXP_KEY(1, "meta-meta-meta2-A", "meta-meta-up", "\001[\001[\001[[A");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-1;3B", "meta-meta-down", "\001[\001[[1;3B");
    WEE_CHECK_EXP_KEY(1, "meta-meta-meta2-B", "meta-meta-down", "\001[\001[\001[[B");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-1;3C", "meta-meta-right", "\001[\001[[1;3C");
    WEE_CHECK_EXP_KEY(1, "meta-meta-meta2-C", "meta-meta-right", "\001[\001[\001[[C");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-1;3D", "meta-meta-left", "\001[\001[[1;3D");
    WEE_CHECK_EXP_KEY(1, "meta-meta-meta2-D", "meta-meta-left", "\001[\001[\001[[D");

    /* 3 * alt + arrows, modifier = 3: 1 + 2=alt */
    WEE_CHECK_EXP_KEY(1, "meta-meta-meta2-1;3A", "meta-meta-meta-up", "\001[\001[\001[[1;3A");
    WEE_CHECK_EXP_KEY(1, "meta-meta-meta-meta2-A", "meta-meta-meta-up", "\001[\001[\001[\001[[A");
    WEE_CHECK_EXP_KEY(1, "meta-meta-meta2-1;3B", "meta-meta-meta-down", "\001[\001[\001[[1;3B");
    WEE_CHECK_EXP_KEY(1, "meta-meta-meta-meta2-B", "meta-meta-meta-down", "\001[\001[\001[\001[[B");
    WEE_CHECK_EXP_KEY(1, "meta-meta-meta2-1;3C", "meta-meta-meta-right", "\001[\001[\001[[1;3C");
    WEE_CHECK_EXP_KEY(1, "meta-meta-meta-meta2-C", "meta-meta-meta-right", "\001[\001[\001[\001[[C");
    WEE_CHECK_EXP_KEY(1, "meta-meta-meta2-1;3D", "meta-meta-meta-left", "\001[\001[\001[[1;3D");
    WEE_CHECK_EXP_KEY(1, "meta-meta-meta-meta2-D", "meta-meta-meta-left", "\001[\001[\001[\001[[D");

    /* alt + shift + arrows, modifier = 4: 1 + 1=shift + 2=alt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;4A", "meta-shift-up", "\001[[1;4A");
    WEE_CHECK_EXP_KEY(1, "meta2-1;4B", "meta-shift-down", "\001[[1;4B");
    WEE_CHECK_EXP_KEY(1, "meta2-1;4C", "meta-shift-right", "\001[[1;4C");
    WEE_CHECK_EXP_KEY(1, "meta2-1;4D", "meta-shift-left", "\001[[1;4D");

    /* ctrl + arrows, modifier = 5: 1 + 4=ctrl */
    WEE_CHECK_EXP_KEY(1, "meta2-1;5A", "ctrl-up", "\001[[1;5A");
    WEE_CHECK_EXP_KEY(1, "meta2-a", "ctrl-up", "\001[Oa");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;5B", "ctrl-down", "\001[[1;5B");
    WEE_CHECK_EXP_KEY(1, "meta2-b", "ctrl-down", "\001[Ob");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;5C", "ctrl-right", "\001[[1;5C");
    WEE_CHECK_EXP_KEY(1, "meta2-c", "ctrl-right", "\001[Oc");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;5D", "ctrl-left", "\001[[1;5D");
    WEE_CHECK_EXP_KEY(1, "meta2-d", "ctrl-left", "\001[Od");  /* urxvt */

    /* ctrl + shift + arrows, modifier = 6: 1 + 1=shift + 4=ctrl */
    WEE_CHECK_EXP_KEY(1, "meta2-1;6A", "ctrl-shift-up", "\001[[1;6A");
    WEE_CHECK_EXP_KEY(1, "meta2-1;6B", "ctrl-shift-down", "\001[[1;6B");
    WEE_CHECK_EXP_KEY(1, "meta2-1;6C", "ctrl-shift-right", "\001[[1;6C");
    WEE_CHECK_EXP_KEY(1, "meta2-1;6D", "ctrl-shift-left", "\001[[1;6D");

    /* ctrl + alt + arrows, modifier = 7: 1 + 2=alt + 4=ctrl */
    WEE_CHECK_EXP_KEY(1, "meta2-1;7A", "meta-ctrl-up", "\001[[1;7A");
    WEE_CHECK_EXP_KEY(1, "meta2-1;7B", "meta-ctrl-down", "\001[[1;7B");
    WEE_CHECK_EXP_KEY(1, "meta2-1;7C", "meta-ctrl-right", "\001[[1;7C");
    WEE_CHECK_EXP_KEY(1, "meta2-1;7D", "meta-ctrl-left", "\001[[1;7D");

    /* ctrl + alt + arrows, modifier = 8: 1 + 1=shift + 2=alt + 4=ctrl */
    WEE_CHECK_EXP_KEY(1, "meta2-1;8A", "meta-ctrl-shift-up", "\001[[1;8A");
    WEE_CHECK_EXP_KEY(1, "meta2-1;8B", "meta-ctrl-shift-down", "\001[[1;8B");
    WEE_CHECK_EXP_KEY(1, "meta2-1;8C", "meta-ctrl-shift-right", "\001[[1;8C");
    WEE_CHECK_EXP_KEY(1, "meta2-1;8D", "meta-ctrl-shift-left", "\001[[1;8D");

    /* home */
    WEE_CHECK_EXP_KEY(1, "meta2-H", "home", "\001[[H");
    WEE_CHECK_EXP_KEY(1, "meta2-1~", "home", "\001[[1~");
    WEE_CHECK_EXP_KEY(1, "meta2-7~", "home", "\001[[7~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;2H", "shift-home", "\001[[1;2H");
    WEE_CHECK_EXP_KEY(1, "meta2-7$", "shift-home", "\001[[7$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;3H", "meta-home", "\001[[1;3H");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-7~", "meta-home", "\001[\001[[7~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;4H", "meta-shift-home", "\001[[1;4H");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-7$", "meta-shift-home", "\001[\001[[7$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;5H", "ctrl-home", "\001[[1;5H");
    WEE_CHECK_EXP_KEY(1, "meta2-7^", "ctrl-home", "\001[[7^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;6H", "ctrl-shift-home", "\001[[1;6H");
    WEE_CHECK_EXP_KEY(1, "meta2-7@", "ctrl-shift-home", "\001[[7@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;7H", "meta-ctrl-home", "\001[[1;7H");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-7^", "meta-ctrl-home", "\001[\001[[7^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;8H", "meta-ctrl-shift-home", "\001[[1;8H");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-7@", "meta-ctrl-shift-home", "\001[\001[[7@");  /* urxvt */

    /* end */
    WEE_CHECK_EXP_KEY(1, "meta2-F", "end", "\001[[F");
    WEE_CHECK_EXP_KEY(1, "meta2-4~", "end", "\001[[4~");
    WEE_CHECK_EXP_KEY(1, "meta2-8~", "end", "\001[[8~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;2F", "shift-end", "\001[[1;2F");
    WEE_CHECK_EXP_KEY(1, "meta2-4;2~", "shift-end", "\001[[4;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-8;2~", "shift-end", "\001[[8;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;3F", "meta-end", "\001[[1;3F");
    WEE_CHECK_EXP_KEY(1, "meta2-4;3~", "meta-end", "\001[[4;3~");
    WEE_CHECK_EXP_KEY(1, "meta2-8;3~", "meta-end", "\001[[8;3~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;4F", "meta-shift-end", "\001[[1;4F");
    WEE_CHECK_EXP_KEY(1, "meta2-4;4~", "meta-shift-end", "\001[[4;4~");
    WEE_CHECK_EXP_KEY(1, "meta2-8;4~", "meta-shift-end", "\001[[8;4~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;5F", "ctrl-end", "\001[[1;5F");
    WEE_CHECK_EXP_KEY(1, "meta2-4;5~", "ctrl-end", "\001[[4;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-8;5~", "ctrl-end", "\001[[8;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;6F", "ctrl-shift-end", "\001[[1;6F");
    WEE_CHECK_EXP_KEY(1, "meta2-4;6~", "ctrl-shift-end", "\001[[4;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-8;6~", "ctrl-shift-end", "\001[[8;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;7F", "meta-ctrl-end", "\001[[1;7F");
    WEE_CHECK_EXP_KEY(1, "meta2-4;7~", "meta-ctrl-end", "\001[[4;7~");
    WEE_CHECK_EXP_KEY(1, "meta2-8;7~", "meta-ctrl-end", "\001[[8;7~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;8F", "meta-ctrl-shift-end", "\001[[1;8F");
    WEE_CHECK_EXP_KEY(1, "meta2-4;8~", "meta-ctrl-shift-end", "\001[[4;8~");
    WEE_CHECK_EXP_KEY(1, "meta2-8;8~", "meta-ctrl-shift-end", "\001[[8;8~");

    /* insert */
    WEE_CHECK_EXP_KEY(1, "meta2-2~", "insert", "\001[[2~");
    WEE_CHECK_EXP_KEY(1, "meta2-2;2~", "shift-insert", "\001[[2;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-2$", "shift-insert", "\001[[2$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-2;3~", "meta-insert", "\001[[2;3~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-2~", "meta-insert", "\001[\001[[2~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-2;4~", "meta-shift-insert", "\001[[2;4~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-2$", "meta-shift-insert", "\001[\001[[2$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-2;5~", "ctrl-insert", "\001[[2;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-2^", "ctrl-insert", "\001[[2^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-2;6~", "ctrl-shift-insert", "\001[[2;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-2@", "ctrl-shift-insert", "\001[[2@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-2;7~", "meta-ctrl-insert", "\001[[2;7~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-2^", "meta-ctrl-insert", "\001[\001[[2^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-2;8~", "meta-ctrl-shift-insert", "\001[[2;8~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-2@", "meta-ctrl-shift-insert", "\001[\001[[2@");  /* urxvt */

    /* delete */
    WEE_CHECK_EXP_KEY(1, "meta2-3~", "delete", "\001[[3~");
    WEE_CHECK_EXP_KEY(1, "meta2-3;2~", "shift-delete", "\001[[3;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-3$", "shift-delete", "\001[[3$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-3;3~", "meta-delete", "\001[[3;3~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-3~", "meta-delete", "\001[\001[[3~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-3;4~", "meta-shift-delete", "\001[[3;4~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-3$", "meta-shift-delete", "\001[\001[[3$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-3;5~", "ctrl-delete", "\001[[3;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-3^", "ctrl-delete", "\001[[3^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-3;6~", "ctrl-shift-delete", "\001[[3;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-3@", "ctrl-shift-delete", "\001[[3@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-3;7~", "meta-ctrl-delete", "\001[[3;7~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-3^", "meta-ctrl-delete", "\001[\001[[3^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-3;8~", "meta-ctrl-shift-delete", "\001[[3;8~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-3@", "meta-ctrl-shift-delete", "\001[\001[[3@");  /* urxvt */

    /* pgup */
    WEE_CHECK_EXP_KEY(1, "meta2-5~", "pgup", "\001[[5~");
    WEE_CHECK_EXP_KEY(1, "meta2-I", "pgup", "\001[[I");
    WEE_CHECK_EXP_KEY(1, "meta2-5;2~", "shift-pgup", "\001[[5;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;2I", "shift-pgup", "\001[[1;2I");
    WEE_CHECK_EXP_KEY(1, "meta2-5;3~", "meta-pgup", "\001[[5;3~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;3I", "meta-pgup", "\001[[1;3I");
    WEE_CHECK_EXP_KEY(1, "meta2-5;4~", "meta-shift-pgup", "\001[[5;4~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;4I", "meta-shift-pgup", "\001[[1;4I");
    WEE_CHECK_EXP_KEY(1, "meta2-5;5~", "ctrl-pgup", "\001[[5;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;5I", "ctrl-pgup", "\001[[1;5I");
    WEE_CHECK_EXP_KEY(1, "meta2-5;6~", "ctrl-shift-pgup", "\001[[5;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;6I", "ctrl-shift-pgup", "\001[[1;6I");
    WEE_CHECK_EXP_KEY(1, "meta2-5;7~", "meta-ctrl-pgup", "\001[[5;7~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;7I", "meta-ctrl-pgup", "\001[[1;7I");
    WEE_CHECK_EXP_KEY(1, "meta2-5;8~", "meta-ctrl-shift-pgup", "\001[[5;8~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;8I", "meta-ctrl-shift-pgup", "\001[[1;8I");

    /* pgdn */
    WEE_CHECK_EXP_KEY(1, "meta2-6~", "pgdn", "\001[[6~");
    WEE_CHECK_EXP_KEY(1, "meta2-G", "pgdn", "\001[[G");
    WEE_CHECK_EXP_KEY(1, "meta2-6;2~", "shift-pgdn", "\001[[6;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;2G", "shift-pgdn", "\001[[1;2G");
    WEE_CHECK_EXP_KEY(1, "meta2-6;3~", "meta-pgdn", "\001[[6;3~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;3G", "meta-pgdn", "\001[[1;3G");
    WEE_CHECK_EXP_KEY(1, "meta2-6;4~", "meta-shift-pgdn", "\001[[6;4~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;4G", "meta-shift-pgdn", "\001[[1;4G");
    WEE_CHECK_EXP_KEY(1, "meta2-6;5~", "ctrl-pgdn", "\001[[6;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;5G", "ctrl-pgdn", "\001[[1;5G");
    WEE_CHECK_EXP_KEY(1, "meta2-6;6~", "ctrl-shift-pgdn", "\001[[6;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;6G", "ctrl-shift-pgdn", "\001[[1;6G");
    WEE_CHECK_EXP_KEY(1, "meta2-6;7~", "meta-ctrl-pgdn", "\001[[6;7~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;7G", "meta-ctrl-pgdn", "\001[[1;7G");
    WEE_CHECK_EXP_KEY(1, "meta2-6;8~", "meta-ctrl-shift-pgdn", "\001[[6;8~");
    WEE_CHECK_EXP_KEY(1, "meta2-1;8G", "meta-ctrl-shift-pgdn", "\001[[1;8G");

    /* f0 */
    WEE_CHECK_EXP_KEY(1, "meta2-10~", "f0", "\001[[10~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-10$", "shift-f0", "\001[[10$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta-meta2-10~", "meta-f0", "\001[\001[[10~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta-meta2-10$", "meta-shift-f0", "\001[\001[[10$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-10^", "ctrl-f0", "\001[[10^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-10@", "ctrl-shift-f0", "\001[[10@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta-meta2-10^", "meta-ctrl-f0", "\001[\001[[10^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta-meta2-10@", "meta-ctrl-shift-f0", "\001[\001[[10@");  /* urxvt */

    /* f1 */
    WEE_CHECK_EXP_KEY(1, "meta2-P", "f1", "\001[OP");
    WEE_CHECK_EXP_KEY(1, "meta2-11~", "f1", "\001[[11~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-[A", "f1", "\001[[[A");  /* Linux console */
    WEE_CHECK_EXP_KEY(1, "meta2-1;2P", "shift-f1", "\001[[1;2P");
    WEE_CHECK_EXP_KEY(1, "meta2-11$", "shift-f1", "\001[[11$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;3P", "meta-f1", "\001[[1;3P");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-11~", "meta-f1", "\001[\001[[11~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;4P", "meta-shift-f1", "\001[[1;4P");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-11$", "meta-shift-f1", "\001[\001[[11$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;5P", "ctrl-f1", "\001[[1;5P");
    WEE_CHECK_EXP_KEY(1, "meta2-11^", "ctrl-f1", "\001[[11^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;6P", "ctrl-shift-f1", "\001[[1;6P");
    WEE_CHECK_EXP_KEY(1, "meta2-11@", "ctrl-shift-f1", "\001[[11@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;7P", "meta-ctrl-f1", "\001[[1;7P");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-11^", "meta-ctrl-f1", "\001[\001[[11^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;8P", "meta-ctrl-shift-f1", "\001[[1;8P");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-11@", "meta-ctrl-shift-f1", "\001[\001[[11@");  /* urxvt */

    /* f2 */
    WEE_CHECK_EXP_KEY(1, "meta2-Q", "f2", "\001[OQ");
    WEE_CHECK_EXP_KEY(1, "meta2-12~", "f2", "\001[[12~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-[B", "f2", "\001[[[B");  /* Linux console */
    WEE_CHECK_EXP_KEY(1, "meta2-1;2Q", "shift-f2", "\001[[1;2Q");
    WEE_CHECK_EXP_KEY(1, "meta2-12$", "shift-f2", "\001[[12$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;3Q", "meta-f2", "\001[[1;3Q");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-12~", "meta-f2", "\001[\001[[12~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;4Q", "meta-shift-f2", "\001[[1;4Q");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-12$", "meta-shift-f2", "\001[\001[[12$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;5Q", "ctrl-f2", "\001[[1;5Q");
    WEE_CHECK_EXP_KEY(1, "meta2-12^", "ctrl-f2", "\001[[12^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;6Q", "ctrl-shift-f2", "\001[[1;6Q");
    WEE_CHECK_EXP_KEY(1, "meta2-12@", "ctrl-shift-f2", "\001[[12@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;7Q", "meta-ctrl-f2", "\001[[1;7Q");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-12^", "meta-ctrl-f2", "\001[\001[[12^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;8Q", "meta-ctrl-shift-f2", "\001[[1;8Q");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-12@", "meta-ctrl-shift-f2", "\001[\001[[12@");  /* urxvt */

    /* f3 */
    WEE_CHECK_EXP_KEY(1, "meta2-R", "f3", "\001[OR");
    WEE_CHECK_EXP_KEY(1, "meta2-13~", "f3", "\001[[13~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-[C", "f3", "\001[[[C");  /* Linux console */
    WEE_CHECK_EXP_KEY(1, "meta2-1;2R", "shift-f3", "\001[[1;2R");
    WEE_CHECK_EXP_KEY(1, "meta2-13$", "shift-f3", "\001[[13$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;3R", "meta-f3", "\001[[1;3R");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-13~", "meta-f3", "\001[\001[[13~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;4R", "meta-shift-f3", "\001[[1;4R");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-13$", "meta-shift-f3", "\001[\001[[13$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;5R", "ctrl-f3", "\001[[1;5R");
    WEE_CHECK_EXP_KEY(1, "meta2-13^", "ctrl-f3", "\001[[13^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;6R", "ctrl-shift-f3", "\001[[1;6R");
    WEE_CHECK_EXP_KEY(1, "meta2-13@", "ctrl-shift-f3", "\001[[13@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;7R", "meta-ctrl-f3", "\001[[1;7R");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-13^", "meta-ctrl-f3", "\001[\001[[13^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;8R", "meta-ctrl-shift-f3", "\001[[1;8R");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-13@", "meta-ctrl-shift-f3", "\001[\001[[13@");  /* urxvt */

    /* f4 */
    WEE_CHECK_EXP_KEY(1, "meta2-S", "f4", "\001[OS");
    WEE_CHECK_EXP_KEY(1, "meta2-14~", "f4", "\001[[14~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-[D", "f4", "\001[[[D");  /* Linux console */
    WEE_CHECK_EXP_KEY(1, "meta2-1;2S", "shift-f4", "\001[[1;2S");
    WEE_CHECK_EXP_KEY(1, "meta2-14$", "shift-f4", "\001[[14$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;3S", "meta-f4", "\001[[1;3S");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-14~", "meta-f4", "\001[\001[[14~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;4S", "meta-shift-f4", "\001[[1;4S");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-14$", "meta-shift-f4", "\001[\001[[14$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;5S", "ctrl-f4", "\001[[1;5S");
    WEE_CHECK_EXP_KEY(1, "meta2-14^", "ctrl-f4", "\001[[14^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;6S", "ctrl-shift-f4", "\001[[1;6S");
    WEE_CHECK_EXP_KEY(1, "meta2-14@", "ctrl-shift-f4", "\001[[14@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;7S", "meta-ctrl-f4", "\001[[1;7S");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-14^", "meta-ctrl-f4", "\001[\001[[14^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-1;8S", "meta-ctrl-shift-f4", "\001[[1;8S");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-14@", "meta-ctrl-shift-f4", "\001[\001[[14@");  /* urxvt */

    /* f5 */
    WEE_CHECK_EXP_KEY(1, "meta2-15~", "f5", "\001[[15~");
    WEE_CHECK_EXP_KEY(1, "meta2-[E", "f5", "\001[[[E");  /* Linux console */
    WEE_CHECK_EXP_KEY(1, "meta2-15;2~", "shift-f5", "\001[[15;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-15$", "shift-f5", "\001[[15$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-15;3~", "meta-f5", "\001[[15;3~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-15~", "meta-f5", "\001[\001[[15~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-15;4~", "meta-shift-f5", "\001[[15;4~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-15$", "meta-shift-f5", "\001[\001[[15$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-15;5~", "ctrl-f5", "\001[[15;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-15^", "ctrl-f5", "\001[[15^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-15;6~", "ctrl-shift-f5", "\001[[15;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-15@", "ctrl-shift-f5", "\001[[15@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-15;7~", "meta-ctrl-f5", "\001[[15;7~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-15^", "meta-ctrl-f5", "\001[\001[[15^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-15;8~", "meta-ctrl-shift-f5", "\001[[15;8~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-15@", "meta-ctrl-shift-f5", "\001[\001[[15@");  /* urxvt */

    /* f6 */
    WEE_CHECK_EXP_KEY(1, "meta2-17~", "f6", "\001[[17~");
    WEE_CHECK_EXP_KEY(1, "meta2-17;2~", "shift-f6", "\001[[17;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-17$", "shift-f6", "\001[[17$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-17;3~", "meta-f6", "\001[[17;3~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-17~", "meta-f6", "\001[\001[[17~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-17;4~", "meta-shift-f6", "\001[[17;4~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-17$", "meta-shift-f6", "\001[\001[[17$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-17;5~", "ctrl-f6", "\001[[17;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-17^", "ctrl-f6", "\001[[17^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-17;6~", "ctrl-shift-f6", "\001[[17;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-17@", "ctrl-shift-f6", "\001[[17@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-17;7~", "meta-ctrl-f6", "\001[[17;7~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-17^", "meta-ctrl-f6", "\001[\001[[17^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-17;8~", "meta-ctrl-shift-f6", "\001[[17;8~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-17@", "meta-ctrl-shift-f6", "\001[\001[[17@");  /* urxvt */

    /* f7 */
    WEE_CHECK_EXP_KEY(1, "meta2-18~", "f7", "\001[[18~");
    WEE_CHECK_EXP_KEY(1, "meta2-18;2~", "shift-f7", "\001[[18;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-18$", "shift-f7", "\001[[18$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-18;3~", "meta-f7", "\001[[18;3~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-18~", "meta-f7", "\001[\001[[18~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-18;4~", "meta-shift-f7", "\001[[18;4~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-18$", "meta-shift-f7", "\001[\001[[18$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-18;5~", "ctrl-f7", "\001[[18;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-18^", "ctrl-f7", "\001[[18^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-18;6~", "ctrl-shift-f7", "\001[[18;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-18@", "ctrl-shift-f7", "\001[[18@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-18;7~", "meta-ctrl-f7", "\001[[18;7~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-18^", "meta-ctrl-f7", "\001[\001[[18^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-18;8~", "meta-ctrl-shift-f7", "\001[[18;8~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-18@", "meta-ctrl-shift-f7", "\001[\001[[18@");  /* urxvt */

    /* f8 */
    WEE_CHECK_EXP_KEY(1, "meta2-19~", "f8", "\001[[19~");
    WEE_CHECK_EXP_KEY(1, "meta2-19;2~", "shift-f8", "\001[[19;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-19$", "shift-f8", "\001[[19$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-19;3~", "meta-f8", "\001[[19;3~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-19~", "meta-f8", "\001[\001[[19~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-19;4~", "meta-shift-f8", "\001[[19;4~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-19$", "meta-shift-f8", "\001[\001[[19$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-19;5~", "ctrl-f8", "\001[[19;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-19^", "ctrl-f8", "\001[[19^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-19;6~", "ctrl-shift-f8", "\001[[19;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-19@", "ctrl-shift-f8", "\001[[19@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-19;7~", "meta-ctrl-f8", "\001[[19;7~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-19^", "meta-ctrl-f8", "\001[\001[[19^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-19;8~", "meta-ctrl-shift-f8", "\001[[19;8~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-19@", "meta-ctrl-shift-f8", "\001[\001[[19@");  /* urxvt */

    /* f9 */
    WEE_CHECK_EXP_KEY(1, "meta2-20~", "f9", "\001[[20~");
    WEE_CHECK_EXP_KEY(1, "meta2-20;2~", "shift-f9", "\001[[20;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-20$", "shift-f9", "\001[[20$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-20;3~", "meta-f9", "\001[[20;3~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-20~", "meta-f9", "\001[\001[[20~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-20;4~", "meta-shift-f9", "\001[[20;4~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-20$", "meta-shift-f9", "\001[\001[[20$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-20;5~", "ctrl-f9", "\001[[20;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-20^", "ctrl-f9", "\001[[20^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-20;6~", "ctrl-shift-f9", "\001[[20;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-20@", "ctrl-shift-f9", "\001[[20@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-20;7~", "meta-ctrl-f9", "\001[[20;7~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-20^", "meta-ctrl-f9", "\001[\001[[20^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-20;8~", "meta-ctrl-shift-f9", "\001[[20;8~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-20@", "meta-ctrl-shift-f9", "\001[\001[[20@");  /* urxvt */

    /* f10 */
    WEE_CHECK_EXP_KEY(1, "meta2-21~", "f10", "\001[[21~");
    WEE_CHECK_EXP_KEY(1, "meta2-21;2~", "shift-f10", "\001[[21;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-21$", "shift-f10", "\001[[21$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-21;3~", "meta-f10", "\001[[21;3~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-21~", "meta-f10", "\001[\001[[21~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-21;4~", "meta-shift-f10", "\001[[21;4~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-21$", "meta-shift-f10", "\001[\001[[21$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-21;5~", "ctrl-f10", "\001[[21;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-21^", "ctrl-f10", "\001[[21^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-21;6~", "ctrl-shift-f10", "\001[[21;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-21@", "ctrl-shift-f10", "\001[[21@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-21;7~", "meta-ctrl-f10", "\001[[21;7~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-21^", "meta-ctrl-f10", "\001[\001[[21^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-21;8~", "meta-ctrl-shift-f10", "\001[[21;8~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-21@", "meta-ctrl-shift-f10", "\001[\001[[21@");  /* urxvt */

    /* f11 */
    WEE_CHECK_EXP_KEY(1, "meta2-23~", "f11", "\001[[23~");
    WEE_CHECK_EXP_KEY(1, "meta2-23;2~", "shift-f11", "\001[[23;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-23$", "shift-f11", "\001[[23$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-23;3~", "meta-f11", "\001[[23;3~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-23~", "meta-f11", "\001[\001[[23~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-23;4~", "meta-shift-f11", "\001[[23;4~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-23$", "meta-shift-f11", "\001[\001[[23$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-23;5~", "ctrl-f11", "\001[[23;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-23^", "ctrl-f11", "\001[[23^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-23;6~", "ctrl-shift-f11", "\001[[23;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-23@", "ctrl-shift-f11", "\001[[23@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-23;7~", "meta-ctrl-f11", "\001[[23;7~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-23^", "meta-ctrl-f11", "\001[\001[[23^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-23;8~", "meta-ctrl-shift-f11", "\001[[23;8~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-23@", "meta-ctrl-shift-f11", "\001[\001[[23@");  /* urxvt */

    /* f12 */
    WEE_CHECK_EXP_KEY(1, "meta2-24~", "f12", "\001[[24~");
    WEE_CHECK_EXP_KEY(1, "meta2-24;2~", "shift-f12", "\001[[24;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-24$", "shift-f12", "\001[[24$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-24;3~", "meta-f12", "\001[[24;3~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-24~", "meta-f12", "\001[\001[[24~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-24;4~", "meta-shift-f12", "\001[[24;4~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-24$", "meta-shift-f12", "\001[\001[[24$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-24;5~", "ctrl-f12", "\001[[24;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-24^", "ctrl-f12", "\001[[24^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-24;6~", "ctrl-shift-f12", "\001[[24;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-24@", "ctrl-shift-f12", "\001[[24@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-24;7~", "meta-ctrl-f12", "\001[[24;7~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-24^", "meta-ctrl-f12", "\001[\001[[24^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-24;8~", "meta-ctrl-shift-f12", "\001[[24;8~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-24@", "meta-ctrl-shift-f12", "\001[\001[[24@");  /* urxvt */

    /* f13 */
    WEE_CHECK_EXP_KEY(1, "meta2-25~", "f13", "\001[[25~");
    WEE_CHECK_EXP_KEY(1, "meta2-25;2~", "shift-f13", "\001[[25;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-25$", "shift-f13", "\001[[25$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-25;3~", "meta-f13", "\001[[25;3~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-25~", "meta-f13", "\001[\001[[25~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-25;4~", "meta-shift-f13", "\001[[25;4~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-25$", "meta-shift-f13", "\001[\001[[25$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-25;5~", "ctrl-f13", "\001[[25;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-25^", "ctrl-f13", "\001[[25^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-25;6~", "ctrl-shift-f13", "\001[[25;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-25@", "ctrl-shift-f13", "\001[[25@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-25;7~", "meta-ctrl-f13", "\001[[25;7~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-25^", "meta-ctrl-f13", "\001[\001[[25^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-25;8~", "meta-ctrl-shift-f13", "\001[[25;8~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-25@", "meta-ctrl-shift-f13", "\001[\001[[25@");  /* urxvt */

    /* f14 */
    WEE_CHECK_EXP_KEY(1, "meta2-26~", "f14", "\001[[26~");
    WEE_CHECK_EXP_KEY(1, "meta2-26;2~", "shift-f14", "\001[[26;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-26$", "shift-f14", "\001[[26$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-26;3~", "meta-f14", "\001[[26;3~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-26~", "meta-f14", "\001[\001[[26~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-26;4~", "meta-shift-f14", "\001[[26;4~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-26$", "meta-shift-f14", "\001[\001[[26$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-26;5~", "ctrl-f14", "\001[[26;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-26^", "ctrl-f14", "\001[[26^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-26;6~", "ctrl-shift-f14", "\001[[26;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-26@", "ctrl-shift-f14", "\001[[26@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-26;7~", "meta-ctrl-f14", "\001[[26;7~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-26^", "meta-ctrl-f14", "\001[\001[[26^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-26;8~", "meta-ctrl-shift-f14", "\001[[26;8~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-26@", "meta-ctrl-shift-f14", "\001[\001[[26@");  /* urxvt */

    /* f15 */
    WEE_CHECK_EXP_KEY(1, "meta2-28~", "f15", "\001[[28~");
    WEE_CHECK_EXP_KEY(1, "meta2-28;2~", "shift-f15", "\001[[28;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-28$", "shift-f15", "\001[[28$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-28;3~", "meta-f15", "\001[[28;3~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-28~", "meta-f15", "\001[\001[[28~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-28;4~", "meta-shift-f15", "\001[[28;4~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-28$", "meta-shift-f15", "\001[\001[[28$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-28;5~", "ctrl-f15", "\001[[28;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-28^", "ctrl-f15", "\001[[28^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-28;6~", "ctrl-shift-f15", "\001[[28;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-28@", "ctrl-shift-f15", "\001[[28@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-28;7~", "meta-ctrl-f15", "\001[[28;7~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-28^", "meta-ctrl-f15", "\001[\001[[28^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-28;8~", "meta-ctrl-shift-f15", "\001[[28;8~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-28@", "meta-ctrl-shift-f15", "\001[\001[[28@");  /* urxvt */

    /* f16 */
    WEE_CHECK_EXP_KEY(1, "meta2-29~", "f16", "\001[[29~");
    WEE_CHECK_EXP_KEY(1, "meta2-29;2~", "shift-f16", "\001[[29;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-29$", "shift-f16", "\001[[29$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-29;3~", "meta-f16", "\001[[29;3~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-29~", "meta-f16", "\001[\001[[29~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-29;4~", "meta-shift-f16", "\001[[29;4~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-29$", "meta-shift-f16", "\001[\001[[29$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-29;5~", "ctrl-f16", "\001[[29;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-29^", "ctrl-f16", "\001[[29^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-29;6~", "ctrl-shift-f16", "\001[[29;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-29@", "ctrl-shift-f16", "\001[[29@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-29;7~", "meta-ctrl-f16", "\001[[29;7~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-29^", "meta-ctrl-f16", "\001[\001[[29^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-29;8~", "meta-ctrl-shift-f16", "\001[[29;8~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-29@", "meta-ctrl-shift-f16", "\001[\001[[29@");  /* urxvt */

    /* f17 */
    WEE_CHECK_EXP_KEY(1, "meta2-31~", "f17", "\001[[31~");
    WEE_CHECK_EXP_KEY(1, "meta2-31;2~", "shift-f17", "\001[[31;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-31$", "shift-f17", "\001[[31$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-31;3~", "meta-f17", "\001[[31;3~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-31~", "meta-f17", "\001[\001[[31~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-31;4~", "meta-shift-f17", "\001[[31;4~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-31$", "meta-shift-f17", "\001[\001[[31$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-31;5~", "ctrl-f17", "\001[[31;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-31^", "ctrl-f17", "\001[[31^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-31;6~", "ctrl-shift-f17", "\001[[31;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-31@", "ctrl-shift-f17", "\001[[31@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-31;7~", "meta-ctrl-f17", "\001[[31;7~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-31^", "meta-ctrl-f17", "\001[\001[[31^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-31;8~", "meta-ctrl-shift-f17", "\001[[31;8~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-31@", "meta-ctrl-shift-f17", "\001[\001[[31@");  /* urxvt */

    /* f18 */
    WEE_CHECK_EXP_KEY(1, "meta2-32~", "f18", "\001[[32~");
    WEE_CHECK_EXP_KEY(1, "meta2-32;2~", "shift-f18", "\001[[32;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-32$", "shift-f18", "\001[[32$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-32;3~", "meta-f18", "\001[[32;3~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-32~", "meta-f18", "\001[\001[[32~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-32;4~", "meta-shift-f18", "\001[[32;4~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-32$", "meta-shift-f18", "\001[\001[[32$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-32;5~", "ctrl-f18", "\001[[32;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-32^", "ctrl-f18", "\001[[32^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-32;6~", "ctrl-shift-f18", "\001[[32;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-32@", "ctrl-shift-f18", "\001[[32@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-32;7~", "meta-ctrl-f18", "\001[[32;7~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-32^", "meta-ctrl-f18", "\001[\001[[32^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-32;8~", "meta-ctrl-shift-f18", "\001[[32;8~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-32@", "meta-ctrl-shift-f18", "\001[\001[[32@");  /* urxvt */

    /* f19 */
    WEE_CHECK_EXP_KEY(1, "meta2-33~", "f19", "\001[[33~");
    WEE_CHECK_EXP_KEY(1, "meta2-33;2~", "shift-f19", "\001[[33;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-33$", "shift-f19", "\001[[33$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-33;3~", "meta-f19", "\001[[33;3~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-33~", "meta-f19", "\001[\001[[33~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-33;4~", "meta-shift-f19", "\001[[33;4~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-33$", "meta-shift-f19", "\001[\001[[33$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-33;5~", "ctrl-f19", "\001[[33;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-33^", "ctrl-f19", "\001[[33^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-33;6~", "ctrl-shift-f19", "\001[[33;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-33@", "ctrl-shift-f19", "\001[[33@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-33;7~", "meta-ctrl-f19", "\001[[33;7~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-33^", "meta-ctrl-f19", "\001[\001[[33^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-33;8~", "meta-ctrl-shift-f19", "\001[[33;8~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-33@", "meta-ctrl-shift-f19", "\001[\001[[33@");  /* urxvt */

    /* f20 */
    WEE_CHECK_EXP_KEY(1, "meta2-34~", "f20", "\001[[34~");
    WEE_CHECK_EXP_KEY(1, "meta2-34;2~", "shift-f20", "\001[[34;2~");
    WEE_CHECK_EXP_KEY(1, "meta2-34$", "shift-f20", "\001[[34$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-34;3~", "meta-f20", "\001[[34;3~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-34~", "meta-f20", "\001[\001[[34~");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-34;4~", "meta-shift-f20", "\001[[34;4~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-34$", "meta-shift-f20", "\001[\001[[34$");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-34;5~", "ctrl-f20", "\001[[34;5~");
    WEE_CHECK_EXP_KEY(1, "meta2-34^", "ctrl-f20", "\001[[34^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-34;6~", "ctrl-shift-f20", "\001[[34;6~");
    WEE_CHECK_EXP_KEY(1, "meta2-34@", "ctrl-shift-f20", "\001[[34@");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-34;7~", "meta-ctrl-f20", "\001[[34;7~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-34^", "meta-ctrl-f20", "\001[\001[[34^");  /* urxvt */
    WEE_CHECK_EXP_KEY(1, "meta2-34;8~", "meta-ctrl-shift-f20", "\001[[34;8~");
    WEE_CHECK_EXP_KEY(1, "meta-meta2-34@", "meta-ctrl-shift-f20", "\001[\001[[34@");  /* urxvt */
}

/*
 * Tests functions:
 *   gui_key_legacy_to_alias
 */

TEST(GuiKey, LegacyToAlias)
{
    char *str;

    WEE_TEST_STR(NULL, gui_key_legacy_to_alias (NULL));
    WEE_TEST_STR("", gui_key_legacy_to_alias (""));

    WEE_TEST_STR("@chat:button1", gui_key_legacy_to_alias ("@chat:button1"));

    WEE_TEST_STR("", gui_key_legacy_to_alias ("ctrl-"));
    WEE_TEST_STR("", gui_key_legacy_to_alias ("meta-"));
    WEE_TEST_STR("", gui_key_legacy_to_alias ("meta2-"));

    WEE_TEST_STR("ctrl-a", gui_key_legacy_to_alias ("ctrl-A"));
    WEE_TEST_STR("ctrl-a", gui_key_legacy_to_alias ("ctrl-a"));
    WEE_TEST_STR("return", gui_key_legacy_to_alias ("ctrl-j"));
    WEE_TEST_STR("return", gui_key_legacy_to_alias ("ctrl-m"));
    WEE_TEST_STR("ctrl-c,b", gui_key_legacy_to_alias ("ctrl-cb"));
    WEE_TEST_STR("meta-space", gui_key_legacy_to_alias ("meta-space"));
    WEE_TEST_STR("meta-c,o,m,m,a", gui_key_legacy_to_alias ("meta-comma"));
    WEE_TEST_STR("meta-comma", gui_key_legacy_to_alias ("meta-,"));
    WEE_TEST_STR("meta-comma,x", gui_key_legacy_to_alias ("meta-,x"));
    WEE_TEST_STR("meta-left", gui_key_legacy_to_alias ("meta2-1;3D"));
    WEE_TEST_STR("meta-w,meta-up", gui_key_legacy_to_alias ("meta-wmeta2-1;3A"));
    WEE_TEST_STR("meta-w,comma,meta-u,p", gui_key_legacy_to_alias ("meta-w,meta-up"));
}

/*
 * Tests functions:
 *   gui_key_find_pos
 */

TEST(GuiKey, FindPos)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_insert_sorted
 */

TEST(GuiKey, InsertSorted)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_set_area_type_name
 */

TEST(GuiKey, SetAreaTypeName)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_set_areas
 */

TEST(GuiKey, SetAreas)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_set_score
 */

TEST(GuiKey, SetScore)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_is_safe
 */

TEST(GuiKey, IsSafe)
{
    /* NOT safe: NULL or empty string */
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, NULL));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, ""));

    /* NOT safe: simple keys */
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "a"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "A"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "é"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "/"));

    /* NOT safe: "@" in default/search context */
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "@"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_SEARCH, "@"));

    /* NOT safe: partial modifier */
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "ctrl"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "meta"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "meta2"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "shift"));

    /* NOT safe: starts with capital letter (keys are case sensitive) */
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "Ctrl-a"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "Meta-a"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "Meta2-a"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "Shift-home"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "F1"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "Home"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "Insert"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "Delete"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "End"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "Pgup"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "Pgdn"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "Up"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "Down"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "Right"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "Left"));
    LONGS_EQUAL(0, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "Tab"));

    /* safe keys */
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "ctrl-a"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "meta-a"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "meta-A"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "shift-home"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f0"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f1"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f2"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f3"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f4"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f5"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f6"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f7"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f8"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f9"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f10"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f11"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f12"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f13"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f14"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f15"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f16"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f17"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f18"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f19"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "f20"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "home"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "insert"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "delete"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "end"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "pgup"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "pgdn"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "up"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "down"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "right"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "left"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_DEFAULT, "tab"));

    /* safe keys: "@" in cursor/mouse context */
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_CURSOR, "@"));
    LONGS_EQUAL(1, gui_key_is_safe (GUI_KEY_CONTEXT_MOUSE, "@"));
}

/*
 * Tests functions:
 *   gui_key_new
 */

TEST(GuiKey, New)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_search
 */

TEST(GuiKey, Search)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_search_part
 */

TEST(GuiKey, SearchPart)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_bind
 */

TEST(GuiKey, Bind)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_bind_plugin_hashtable_map_cb
 */

TEST(GuiKey, BindPluginHashtableMapCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_bind_plugin
 */

TEST(GuiKey, BindPlugin)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_unbind
 */

TEST(GuiKey, Unbind)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_unbind_plugin
 */

TEST(GuiKey, UnbindPlugin)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_focus_matching
 */

TEST(GuiKey, FocusMatching)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_focus_command
 */

TEST(GuiKey, FocusCommand)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_focus
 */

TEST(GuiKey, Focus)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_pressed
 */

TEST(GuiKey, Pressed)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_free
 */

TEST(GuiKey, Free)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_free_all
 */

TEST(GuiKey, FreeAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_buffer_optimize
 */

TEST(GuiKey, BufferOptimize)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_buffer_reset
 */

TEST(GuiKey, BufferReset)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_buffer_add
 */

TEST(GuiKey, BufferAdd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_buffer_search
 */

TEST(GuiKey, BufferSearch)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_buffer_remove
 */

TEST(GuiKey, BufferRemove)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_remove_newline
 */

TEST(GuiKey, PasteRemoveNewline)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_replace_tabs
 */

TEST(GuiKey, PasteReplaceTabs)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_start
 */

TEST(GuiKey, PasteStart)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_get_paste_lines
 */

TEST(GuiKey, GetPasteLines)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_check
 */

TEST(GuiKey, PasteCheck)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_bracketed_timer_cb
 */

TEST(GuiKey, PasteBracketedTimerCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_bracketed_timer_remove
 */

TEST(GuiKey, PasteBracketedTimerRemove)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_bracketed_timer_add
 */

TEST(GuiKey, PasteBracketedTimerAdd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_bracketed_start
 */

TEST(GuiKey, PasteBracketedStart)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_bracketed_stop
 */

TEST(GuiKey, PasteBracketedStop)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_accept
 */

TEST(GuiKey, PasteAccept)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_cancel
 */

TEST(GuiKey, PasteCancel)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_end
 */

TEST(GuiKey, End)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_hdata_key_cb
 */

TEST(GuiKey, HdataKeyCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_add_to_infolist
 */

TEST(GuiKey, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_print_log_key
 */

TEST(GuiKey, PrintLogKey)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_print_log
 */

TEST(GuiKey, PrintLog)
{
    /* TODO: write tests */
}
