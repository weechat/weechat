/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

/* Test theme functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"
#include "tests-record.h"

extern "C"
{
#include <ctype.h>
#include <string.h>
#include "src/core/core-arraylist.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-theme.h"
#include "src/plugins/plugin.h"

extern char *theme_format_now (void);
extern struct t_theme *theme_alloc (const char *name);
extern void theme_free (struct t_theme *theme);
}

TEST_GROUP(CoreTheme)
{
    void setup ()
    {
        /* start every test with a clean registry */
        theme_end ();
        theme_init ();
    }

    void teardown ()
    {
        theme_end ();
    }

    struct t_hashtable *make_overrides (const char *key1, const char *val1,
                                        const char *key2, const char *val2)
    {
        struct t_hashtable *hashtable;

        hashtable = hashtable_new (8,
                                   WEECHAT_HASHTABLE_STRING,
                                   WEECHAT_HASHTABLE_STRING,
                                   NULL,
                                   NULL);
        if (key1)
            hashtable_set (hashtable, key1, val1);
        if (key2)
            hashtable_set (hashtable, key2, val2);
        return hashtable;
    }
};

/*
 * Test functions:
 *   theme_search
 */

TEST(CoreTheme, Search)
{
    struct t_hashtable *overrides;

    /* empty registry */
    POINTERS_EQUAL(NULL, theme_search ("dark"));
    POINTERS_EQUAL(NULL, theme_search (NULL));

    overrides = make_overrides ("weechat.color.chat", "default", NULL, NULL);
    theme_register ("dark", overrides);
    hashtable_free (overrides);

    /* registered name found */
    CHECK(theme_search ("dark") != NULL);
    STRCMP_EQUAL("dark", theme_search ("dark")->name);

    /* unknown / case mismatch / NULL */
    POINTERS_EQUAL(NULL, theme_search ("light"));
    POINTERS_EQUAL(NULL, theme_search ("Dark"));
    POINTERS_EQUAL(NULL, theme_search (NULL));
}

/*
 * Test functions:
 *   theme_format_now
 */

TEST(CoreTheme, FormatNow)
{
    char *str;
    int i;

    str = theme_format_now ();
    CHECK(str != NULL);
    LONGS_EQUAL(19, (long)strlen (str));

    /* format: YYYY-MM-DD HH:MM:SS */
    for (i = 0; i < 4; i++)
        CHECK(isdigit ((unsigned char)str[i]));
    CHECK(str[4] == '-');
    CHECK(isdigit ((unsigned char)str[5]));
    CHECK(isdigit ((unsigned char)str[6]));
    CHECK(str[7] == '-');
    CHECK(isdigit ((unsigned char)str[8]));
    CHECK(isdigit ((unsigned char)str[9]));
    CHECK(str[10] == ' ');
    CHECK(isdigit ((unsigned char)str[11]));
    CHECK(isdigit ((unsigned char)str[12]));
    CHECK(str[13] == ':');
    CHECK(isdigit ((unsigned char)str[14]));
    CHECK(isdigit ((unsigned char)str[15]));
    CHECK(str[16] == ':');
    CHECK(isdigit ((unsigned char)str[17]));
    CHECK(isdigit ((unsigned char)str[18]));

    free (str);
}

/*
 * Test functions:
 *   theme_alloc
 */

TEST(CoreTheme, Alloc)
{
    struct t_theme *theme;

    theme = theme_alloc ("solarized_light");
    CHECK(theme != NULL);
    STRCMP_EQUAL("solarized_light", theme->name);
    STRCMP_EQUAL("", theme->description);
    CHECK(theme->date != NULL);
    LONGS_EQUAL(19, (long)strlen (theme->date));
    CHECK(theme->weechat_version != NULL);
    CHECK(theme->weechat_version[0] != '\0');
    CHECK(theme->overrides != NULL);
    LONGS_EQUAL(0, theme->overrides->items_count);
    POINTERS_EQUAL(NULL, theme->prev_theme);
    POINTERS_EQUAL(NULL, theme->next_theme);

    theme_free (theme);
}

/*
 * Test functions:
 *   theme_free
 */

TEST(CoreTheme, Free)
{
    struct t_theme *theme;

    /* free(NULL) is a no-op, must not crash */
    theme_free (NULL);

    /* free a valid theme that is NOT in the registry */
    theme = theme_alloc ("unknown");
    theme_free (theme);
}

/*
 * Test functions:
 *   theme_merge_overrides_cb
 *   theme_register
 */

TEST(CoreTheme, Register)
{
    struct t_hashtable *o1, *o2;
    struct t_theme *t1, *t2;

    /* NULL / empty name => NULL */
    POINTERS_EQUAL(NULL, theme_register (NULL, NULL));
    POINTERS_EQUAL(NULL, theme_register ("", NULL));

    /* register a new theme */
    o1 = make_overrides ("weechat.color.chat", "default",
                         "weechat.color.separator", "blue");
    t1 = theme_register ("dark", o1);
    hashtable_free (o1);
    CHECK(t1 != NULL);
    STRCMP_EQUAL("dark", t1->name);
    LONGS_EQUAL(2, t1->overrides->items_count);
    STRCMP_EQUAL("default", (const char *)hashtable_get (t1->overrides,
                                                         "weechat.color.chat"));
    STRCMP_EQUAL("blue", (const char *)hashtable_get (t1->overrides,
                                                      "weechat.color.separator"));

    /* second call with same name merges into the existing theme */
    o2 = make_overrides ("irc.color.input_nick", "lightcyan",
                         "weechat.color.separator", "darkgray");
    t2 = theme_register ("dark", o2);
    hashtable_free (o2);
    POINTERS_EQUAL(t1, t2);  /* same struct, merged into */
    LONGS_EQUAL(3, t1->overrides->items_count);
    /* new key added */
    STRCMP_EQUAL("lightcyan", (const char *)hashtable_get (t1->overrides,
                                                           "irc.color.input_nick"));
    /* duplicate key overridden */
    STRCMP_EQUAL("darkgray", (const char *)hashtable_get (t1->overrides,
                                                          "weechat.color.separator"));

    /* registering with NULL overrides only creates the theme */
    t2 = theme_register ("empty", NULL);
    CHECK(t2 != NULL);
    LONGS_EQUAL(0, t2->overrides->items_count);
}

/*
 * Test functions:
 *   theme_list_cmp_cb
 *   theme_list
 */

TEST(CoreTheme, List)
{
    struct t_arraylist *list;

    /* empty list when nothing registered */
    list = theme_list ();
    CHECK(list != NULL);
    LONGS_EQUAL(0, arraylist_size (list));
    arraylist_free (list);

    /* register three themes in non-alphabetical order */
    theme_register ("solarized", NULL);
    theme_register ("dark", NULL);
    theme_register ("nord", NULL);

    list = theme_list ();
    CHECK(list != NULL);
    LONGS_EQUAL(3, arraylist_size (list));

    /* sorted by name */
    STRCMP_EQUAL("dark",
                 ((struct t_theme *)arraylist_get (list, 0))->name);
    STRCMP_EQUAL("nord",
                 ((struct t_theme *)arraylist_get (list, 1))->name);
    STRCMP_EQUAL("solarized",
                 ((struct t_theme *)arraylist_get (list, 2))->name);

    arraylist_free (list);
}

/*
 * Test functions:
 *   theme_init
 */

TEST(CoreTheme, Init)
{
    theme_init ();
}

/*
 * Test functions:
 *   theme_end
 */

TEST(CoreTheme, End)
{
    theme_register ("dark", NULL);
    theme_register ("light", NULL);
    CHECK(themes != NULL);

    theme_end ();
    POINTERS_EQUAL(NULL, themes);
    POINTERS_EQUAL(NULL, last_theme);
}
