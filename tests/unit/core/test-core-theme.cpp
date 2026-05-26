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
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "src/core/core-arraylist.h"
#include "src/core/core-config.h"
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-string.h"
#include "src/core/core-theme.h"
#include "src/core/weechat.h"
#include "src/plugins/plugin.h"

extern char *theme_format_now (void);
extern struct t_theme *theme_alloc (const char *name);
extern void theme_free (struct t_theme *theme);
extern char *theme_user_file_path (const char *name);
extern char *theme_make_backup_name (void);
extern int theme_write_file_full (const char *name, const char *description);
extern char *theme_file_strip_quotes (char *value);
extern struct t_theme *theme_file_parse (const char *path);
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
 *   theme_user_file_path
 */

TEST(CoreTheme, UserFilePath)
{
    char *path, *expected;

    /* NULL / empty => NULL */
    POINTERS_EQUAL(NULL, theme_user_file_path (NULL));
    POINTERS_EQUAL(NULL, theme_user_file_path (""));

    /* "name" => "<weechat_config_dir>/themes/name.theme" */
    expected = NULL;
    string_asprintf (&expected, "%s/themes/dark.theme", weechat_config_dir);
    path = theme_user_file_path ("dark");
    CHECK(path != NULL);
    STRCMP_EQUAL(expected, path);
    free (path);
    free (expected);
}

/*
 * Test functions:
 *   theme_make_backup_name
 */

TEST(CoreTheme, MakeBackupName)
{
    char *name;
    int i;

    name = theme_make_backup_name ();
    CHECK(name != NULL);

    /* format: "backup-YYYYMMDD-HHMMSS-uuuuuu" (29 chars) */
    LONGS_EQUAL(29, (long)strlen (name));
    STRNCMP_EQUAL("backup-", name, 7);

    /* 8 digits for date */
    for (i = 7; i < 15; i++)
        CHECK(isdigit ((unsigned char)name[i]));
    CHECK(name[15] == '-');
    /* 6 digits for time */
    for (i = 16; i < 22; i++)
        CHECK(isdigit ((unsigned char)name[i]));
    CHECK(name[22] == '-');
    /* 6 digits for microseconds */
    for (i = 23; i < 29; i++)
        CHECK(isdigit ((unsigned char)name[i]));

    free (name);
}

/*
 * Test functions:
 *   theme_write_file_full
 */

TEST(CoreTheme, WriteFileFull)
{
    char *path, line[8192];
    FILE *file;
    int saw_info, saw_name, saw_description, saw_date, saw_weechat;
    int saw_options_section, saw_an_option;

    /* refuse empty/NULL */
    LONGS_EQUAL(0, theme_write_file_full (NULL, NULL));
    LONGS_EQUAL(0, theme_write_file_full ("", NULL));

    /* write a valid file */
    LONGS_EQUAL(1, theme_write_file_full ("test_wrt", "a description"));

    path = theme_user_file_path ("test_wrt");
    CHECK(path != NULL);

    file = fopen (path, "r");
    CHECK(file != NULL);

    saw_info = saw_name = saw_description = saw_date = saw_weechat = 0;
    saw_options_section = saw_an_option = 0;
    while (fgets (line, sizeof (line) - 1, file))
    {
        if (strncmp (line, "[info]", 6) == 0)
            saw_info = 1;
        else if (strncmp (line, "[options]", 9) == 0)
            saw_options_section = 1;
        else if (strncmp (line, "name = \"test_wrt\"", 17) == 0)
            saw_name = 1;
        else if (strncmp (line, "description = \"a description\"", 29) == 0)
            saw_description = 1;
        else if (strncmp (line, "date = \"", 8) == 0)
            saw_date = 1;
        else if (strncmp (line, "weechat = \"", 11) == 0)
            saw_weechat = 1;
        else if (saw_options_section
                 && (strchr (line, '=') != NULL)
                 && (strchr (line, '.') != NULL))
            saw_an_option = 1;
    }
    fclose (file);

    LONGS_EQUAL(1, saw_info);
    LONGS_EQUAL(1, saw_name);
    LONGS_EQUAL(1, saw_description);
    LONGS_EQUAL(1, saw_date);
    LONGS_EQUAL(1, saw_weechat);
    LONGS_EQUAL(1, saw_options_section);
    LONGS_EQUAL(1, saw_an_option);

    unlink (path);
    free (path);
}

/*
 * Test functions:
 *   theme_make_backup
 */

TEST(CoreTheme, MakeBackup)
{
    char *name, *path;
    struct stat st;

    name = theme_make_backup ();
    CHECK(name != NULL);
    STRNCMP_EQUAL("backup-", name, 7);
    LONGS_EQUAL(29, (long)strlen (name));

    /* the backup file must exist on disk */
    path = theme_user_file_path (name);
    CHECK(path != NULL);
    LONGS_EQUAL(0, stat (path, &st));
    CHECK(st.st_size > 0);

    unlink (path);
    free (path);
    free (name);
}

/*
 * Test functions:
 *   theme_apply_set_option_cb
 *   theme_apply
 */

TEST(CoreTheme, Apply)
{
    struct t_hashtable *overrides;
    struct t_config_option *opt_prefix_error;
    char *saved_prefix_error, *saved_theme_label;
    int saved_backup;

    /* NULL / empty / missing name => error */
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_apply (NULL));
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_apply (""));
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_apply ("does_not_exist"));

    /* snapshot the option we will mutate + supporting state */
    opt_prefix_error = NULL;
    config_file_search_with_string ("weechat.look.prefix_error",
                                    NULL, NULL, &opt_prefix_error, NULL);
    CHECK(opt_prefix_error != NULL);
    saved_prefix_error = strdup (CONFIG_STRING(opt_prefix_error));
    saved_theme_label = strdup (CONFIG_STRING(config_look_theme));
    saved_backup = CONFIG_BOOLEAN(config_look_theme_backup);

    /* disable backup so the test does not touch the filesystem */
    config_file_option_set (config_look_theme_backup, "off", 1);

    /* register a theme that flips one themable option, then apply */
    overrides = make_overrides ("weechat.look.prefix_error", "TEST!",
                                NULL, NULL);
    theme_register ("apply_test", overrides);
    hashtable_free (overrides);

    LONGS_EQUAL(WEECHAT_RC_OK, theme_apply ("apply_test"));

    /* override took effect */
    STRCMP_EQUAL("TEST!", CONFIG_STRING(opt_prefix_error));
    /* active label persisted */
    STRCMP_EQUAL("apply_test", CONFIG_STRING(config_look_theme));

    /* restore previous state */
    config_file_option_set (opt_prefix_error, saved_prefix_error, 1);
    config_file_option_set (config_look_theme, saved_theme_label, 1);
    config_file_option_set (config_look_theme_backup,
                            (saved_backup) ? "on" : "off", 1);

    free (saved_prefix_error);
    free (saved_theme_label);
}

/*
 * Test functions:
 *   theme_file_strip_quotes
 */

TEST(CoreTheme, FileStripQuotes)
{
    char buf[64];

    /* NULL passes through */
    POINTERS_EQUAL(NULL, theme_file_strip_quotes (NULL));

    /* len < 2: too short to be a matched quote pair */
    strcpy (buf, "");
    STRCMP_EQUAL("", theme_file_strip_quotes (buf));
    strcpy (buf, "a");
    STRCMP_EQUAL("a", theme_file_strip_quotes (buf));
    strcpy (buf, "\"");
    STRCMP_EQUAL("\"", theme_file_strip_quotes (buf));

    /* no quotes: returned as-is */
    strcpy (buf, "hello");
    STRCMP_EQUAL("hello", theme_file_strip_quotes (buf));

    /* matched double quotes are stripped */
    strcpy (buf, "\"hello\"");
    STRCMP_EQUAL("hello", theme_file_strip_quotes (buf));

    /* matched single quotes are stripped */
    strcpy (buf, "'world'");
    STRCMP_EQUAL("world", theme_file_strip_quotes (buf));

    /* mismatched: unchanged */
    strcpy (buf, "\"unmatched'");
    STRCMP_EQUAL("\"unmatched'", theme_file_strip_quotes (buf));
    strcpy (buf, "'unmatched\"");
    STRCMP_EQUAL("'unmatched\"", theme_file_strip_quotes (buf));

    /* exactly two quotes => empty string after stripping */
    strcpy (buf, "\"\"");
    STRCMP_EQUAL("", theme_file_strip_quotes (buf));

    /* internal quotes only on one side: unchanged */
    strcpy (buf, "no\"quote");
    STRCMP_EQUAL("no\"quote", theme_file_strip_quotes (buf));
}

/*
 * Test functions:
 *   theme_file_parse
 */

TEST(CoreTheme, FileParse)
{
    const char *path = "/tmp/weechat_test_theme_parse.theme";
    FILE *file;
    struct t_theme *theme;

    /* NULL and missing file => NULL */
    POINTERS_EQUAL(NULL, theme_file_parse (NULL));
    unlink (path);  /* belt-and-suspenders */
    POINTERS_EQUAL(NULL, theme_file_parse (path));

    /* write a well-formed file: [info] + [options], mixed quoting,
       blanks and comments scattered around */
    file = fopen (path, "w");
    CHECK(file != NULL);
    fprintf (file, "# leading comment\n");
    fprintf (file, "\n");
    fprintf (file, "[info]\n");
    fprintf (file, "name = \"solarized_light\"\n");
    fprintf (file, "description = \"Light-bg theme\"\n");
    fprintf (file, "date = \"2026-05-26 09:42:10\"\n");
    fprintf (file, "weechat = \"4.10.0-dev\"\n");
    fprintf (file, "unknown_info_key = \"ignored\"\n");
    fprintf (file, "\n");
    fprintf (file, "[options]\n");
    fprintf (file, "weechat.color.chat = default\n");                 /* unquoted */
    fprintf (file, "  weechat.color.separator   =   \"blue\"\n");     /* whitespace + quotes */
    fprintf (file, "irc.color.input_nick = 'lightcyan'\n");           /* single quotes */
    fclose (file);

    theme = theme_file_parse (path);
    CHECK(theme != NULL);

    /* [info] fields populated */
    STRCMP_EQUAL("solarized_light", theme->name);
    STRCMP_EQUAL("Light-bg theme", theme->description);
    STRCMP_EQUAL("2026-05-26 09:42:10", theme->date);
    STRCMP_EQUAL("4.10.0-dev", theme->weechat_version);

    /* [options] entries: three known keys, "unknown_info_key" must NOT
       leak in (it lives under [info]) */
    LONGS_EQUAL(3, theme->overrides->items_count);
    STRCMP_EQUAL("default",
                 (const char *)hashtable_get (theme->overrides,
                                              "weechat.color.chat"));
    STRCMP_EQUAL("blue",
                 (const char *)hashtable_get (theme->overrides,
                                              "weechat.color.separator"));
    STRCMP_EQUAL("lightcyan",
                 (const char *)hashtable_get (theme->overrides,
                                              "irc.color.input_nick"));
    POINTERS_EQUAL(NULL, hashtable_get (theme->overrides,
                                        "unknown_info_key"));

    theme_free (theme);
    unlink (path);

    /* parse a file that has only [info]: overrides hashtable empty,
       missing [info] keys default to empty string */
    file = fopen (path, "w");
    CHECK(file != NULL);
    fprintf (file, "[info]\n");
    fprintf (file, "name = \"only_info\"\n");
    fclose (file);

    theme = theme_file_parse (path);
    CHECK(theme != NULL);
    STRCMP_EQUAL("only_info", theme->name);
    STRCMP_EQUAL("", theme->description);
    STRCMP_EQUAL("", theme->date);
    STRCMP_EQUAL("", theme->weechat_version);
    LONGS_EQUAL(0, theme->overrides->items_count);
    theme_free (theme);
    unlink (path);

    /* malformed lines must not crash; a missing-'=' line and a stray
       section header are tolerated, the rest of the file still parses */
    file = fopen (path, "w");
    CHECK(file != NULL);
    fprintf (file, "[info]\n");
    fprintf (file, "name = \"robust\"\n");
    fprintf (file, "broken line without equals\n");
    fprintf (file, "[unknown_section]\n");
    fprintf (file, "ignored = value\n");
    fprintf (file, "[options]\n");
    fprintf (file, "weechat.color.chat = red\n");
    fclose (file);

    theme = theme_file_parse (path);
    CHECK(theme != NULL);
    STRCMP_EQUAL("robust", theme->name);
    LONGS_EQUAL(1, theme->overrides->items_count);
    STRCMP_EQUAL("red",
                 (const char *)hashtable_get (theme->overrides,
                                              "weechat.color.chat"));
    POINTERS_EQUAL(NULL, hashtable_get (theme->overrides, "ignored"));
    theme_free (theme);
    unlink (path);
}

/*
 * Test functions:
 *   theme_init
 */

TEST(CoreTheme, Init)
{
    /* register something so we can prove init wipes it */
    theme_register ("dark", NULL);
    CHECK(themes != NULL);

    theme_init ();
    POINTERS_EQUAL(NULL, themes);
    POINTERS_EQUAL(NULL, last_theme);
    LONGS_EQUAL(0, theme_applying);
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
