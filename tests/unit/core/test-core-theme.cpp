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
#include <dirent.h>
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
extern char *theme_write_file (const char *name, const char *description);
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
    theme_register (NULL, NULL, "dark", overrides);
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
    POINTERS_EQUAL(NULL, theme->contributions);
    POINTERS_EQUAL(NULL, theme->last_contribution);
    LONGS_EQUAL(0, theme_overrides_count (theme));
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
    POINTERS_EQUAL(NULL, theme_register (NULL, NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, theme_register (NULL, NULL, "", NULL));

    /* register a new theme */
    o1 = make_overrides ("weechat.color.chat", "default",
                         "weechat.color.separator", "blue");
    t1 = theme_register (NULL, NULL, "dark", o1);
    hashtable_free (o1);
    CHECK(t1 != NULL);
    STRCMP_EQUAL("dark", t1->name);
    LONGS_EQUAL(2, theme_overrides_count (t1));
    STRCMP_EQUAL("default", theme_get_override (t1,
                                                         "weechat.color.chat"));
    STRCMP_EQUAL("blue", theme_get_override (t1,
                                                      "weechat.color.separator"));

    /* second call with same name merges into the existing theme */
    o2 = make_overrides ("irc.color.input_nick", "lightcyan",
                         "weechat.color.separator", "darkgray");
    t2 = theme_register (NULL, NULL, "dark", o2);
    hashtable_free (o2);
    POINTERS_EQUAL(t1, t2);  /* same struct, merged into */
    LONGS_EQUAL(3, theme_overrides_count (t1));
    /* new key added */
    STRCMP_EQUAL("lightcyan", theme_get_override (t1,
                                                           "irc.color.input_nick"));
    /* duplicate key overridden */
    STRCMP_EQUAL("darkgray", theme_get_override (t1,
                                                          "weechat.color.separator"));

    /* registering with NULL overrides only creates the theme */
    t2 = theme_register (NULL, NULL, "empty", NULL);
    CHECK(t2 != NULL);
    LONGS_EQUAL(0, theme_overrides_count (t2));
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
    theme_register (NULL, NULL, "solarized", NULL);
    theme_register (NULL, NULL, "dark", NULL);
    theme_register (NULL, NULL, "nord", NULL);

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
 *   theme_write_file
 */

TEST(CoreTheme, WriteFile)
{
    char *path, *expected_path, line[8192];
    FILE *file;
    int saw_info, saw_name, saw_description, saw_date, saw_weechat;
    int saw_options_section, saw_an_option, full_options;
    int saw_color_code, saw_string_option, string_option_quoted;

    /* refuse empty/NULL */
    POINTERS_EQUAL(NULL, theme_write_file (NULL, NULL));
    POINTERS_EQUAL(NULL, theme_write_file ("", NULL));

    /* full snapshot: every themable option is written; the returned
       path matches the expected theme file path */
    expected_path = theme_user_file_path ("test_wrt");
    CHECK(expected_path != NULL);

    path = theme_write_file ("test_wrt", "a description");
    CHECK(path != NULL);
    STRCMP_EQUAL(expected_path, path);
    free (path);

    path = expected_path;

    file = fopen (path, "r");
    CHECK(file != NULL);

    saw_info = saw_name = saw_description = saw_date = saw_weechat = 0;
    saw_options_section = saw_an_option = 0;
    saw_color_code = saw_string_option = string_option_quoted = 0;
    full_options = 0;
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
        {
            saw_an_option = 1;
            full_options++;
            /*
             * values must be stored verbatim: no WeeChat color code
             * (byte 0x19) may leak into the file
             */
            if (strchr (line, 0x19) != NULL)
                saw_color_code = 1;
            /* a string option value must be wrapped in double quotes */
            if (strncmp (line, "weechat.look.prefix_error = ", 28) == 0)
            {
                saw_string_option = 1;
                string_option_quoted = (line[28] == '"') ? 1 : 0;
            }
        }
    }
    fclose (file);

    LONGS_EQUAL(1, saw_info);
    LONGS_EQUAL(1, saw_name);
    LONGS_EQUAL(1, saw_description);
    LONGS_EQUAL(1, saw_date);
    LONGS_EQUAL(1, saw_weechat);
    LONGS_EQUAL(1, saw_options_section);
    LONGS_EQUAL(1, saw_an_option);
    /* no color codes leaked into option values */
    LONGS_EQUAL(0, saw_color_code);
    /* the string option was found and its value is quoted */
    LONGS_EQUAL(1, saw_string_option);
    LONGS_EQUAL(1, string_option_quoted);
    CHECK(full_options > 10);  /* core has many themable options */

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
    theme_register (NULL, NULL, "apply_test", overrides);
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
 * Test that /theme apply with a "backup-" prefix skips the backup
 * recursion guard (no backup file is written).
 */

TEST(CoreTheme, ApplyBackupRecursionGuard)
{
    struct t_hashtable *overrides;
    int saved_backup;
    char *path;
    struct stat st;

    /* enable backup so the guard's effect is observable */
    saved_backup = CONFIG_BOOLEAN(config_look_theme_backup);
    config_file_option_set (config_look_theme_backup, "on", 1);

    /* register a theme whose name begins with "backup-" */
    overrides = make_overrides ("weechat.color.separator", "default",
                                NULL, NULL);
    theme_register (NULL, NULL, "backup-recursion-test", overrides);
    hashtable_free (overrides);

    /* before apply: count *.theme files in the themes directory */
    /* (we just verify no NEW backup-2* file appears for this apply) */
    LONGS_EQUAL(WEECHAT_RC_OK, theme_apply ("backup-recursion-test"));

    /* the only file that should exist is the one we are restoring;
       no fresh backup of state-before-apply should have been made */
    path = NULL;
    string_asprintf (&path, "%s/themes", weechat_config_dir);
    if (path)
    {
        int new_backups = 0;
        DIR *d = opendir (path);
        struct dirent *ent;
        if (d)
        {
            while ((ent = readdir (d)))
            {
                /* count entries that look like fresh backups
                   (any backup-* file other than our test theme) */
                if ((strncmp (ent->d_name, "backup-", 7) == 0)
                    && (strcmp (ent->d_name,
                                "backup-recursion-test.theme") != 0)
                    && (strncmp (ent->d_name, ".", 1) != 0))
                {
                    new_backups++;
                }
            }
            closedir (d);
        }
        LONGS_EQUAL(0, new_backups);
        free (path);
    }

    /* clean up the test theme file if it was written by the recursion
       guard test (which only happens if test_themes/<name>.theme was
       created earlier in this run) */
    path = theme_user_file_path ("backup-recursion-test");
    if (path && stat (path, &st) == 0)
        unlink (path);
    free (path);

    /* restore option */
    config_file_option_set (config_look_theme_backup,
                            (saved_backup) ? "on" : "off", 1);
}

/*
 * Test that a user file with the same name as a built-in theme
 * shadows the built-in at /theme apply time.
 */

TEST(CoreTheme, ApplyFileShadowsBuiltin)
{
    struct t_hashtable *overrides;
    struct t_config_option *opt_prefix_error;
    char *saved_prefix_error, *saved_theme_label, *path;
    int saved_backup;
    FILE *f;

    /* snapshot mutable state */
    opt_prefix_error = NULL;
    config_file_search_with_string ("weechat.look.prefix_error",
                                    NULL, NULL, &opt_prefix_error, NULL);
    CHECK(opt_prefix_error != NULL);
    saved_prefix_error = strdup (CONFIG_STRING(opt_prefix_error));
    saved_theme_label = strdup (CONFIG_STRING(config_look_theme));
    saved_backup = CONFIG_BOOLEAN(config_look_theme_backup);
    config_file_option_set (config_look_theme_backup, "off", 1);

    /* register an in-memory theme "shadow_test" with one value */
    overrides = make_overrides ("weechat.look.prefix_error", "FROM_REG",
                                NULL, NULL);
    theme_register (NULL, NULL, "shadow_test", overrides);
    hashtable_free (overrides);

    /* drop a same-named user file with a DIFFERENT value */
    LONGS_EQUAL(WEECHAT_RC_OK, theme_save ("user_throwaway"));  /* ensures themes dir exists */
    path = theme_user_file_path ("shadow_test");
    CHECK(path != NULL);
    f = fopen (path, "w");
    CHECK(f != NULL);
    fprintf (f,
             "[info]\nname = \"shadow_test\"\n\n"
             "[options]\nweechat.look.prefix_error = \"FROM_FILE\"\n");
    fclose (f);

    /* apply: the file value must win over the registry value */
    LONGS_EQUAL(WEECHAT_RC_OK, theme_apply ("shadow_test"));
    STRCMP_EQUAL("FROM_FILE", CONFIG_STRING(opt_prefix_error));

    /* clean up */
    unlink (path);
    free (path);
    path = theme_user_file_path ("user_throwaway");
    unlink (path);
    free (path);

    config_file_option_set (opt_prefix_error, saved_prefix_error, 1);
    config_file_option_set (config_look_theme, saved_theme_label, 1);
    config_file_option_set (config_look_theme_backup,
                            (saved_backup) ? "on" : "off", 1);
    free (saved_prefix_error);
    free (saved_theme_label);
}

/*
 * Test that multiple contributions to the same theme are applied in
 * insertion order; later contributions override earlier ones for the
 * same key.
 */

TEST(CoreTheme, ApplyMergeAcrossContributions)
{
    struct t_weechat_plugin fake_plugin_a, fake_plugin_b;
    struct t_hashtable *o1, *o2;
    struct t_config_option *opt;
    char *saved_value;
    int saved_backup;

    opt = NULL;
    config_file_search_with_string ("weechat.look.prefix_error",
                                    NULL, NULL, &opt, NULL);
    CHECK(opt != NULL);
    saved_value = strdup (CONFIG_STRING(opt));
    saved_backup = CONFIG_BOOLEAN(config_look_theme_backup);
    config_file_option_set (config_look_theme_backup, "off", 1);

    /* plugin A contributes first */
    o1 = make_overrides ("weechat.look.prefix_error", "FROM_A",
                         NULL, NULL);
    theme_register (&fake_plugin_a, NULL, "merge_test", o1);
    hashtable_free (o1);

    /* plugin B contributes after */
    o2 = make_overrides ("weechat.look.prefix_error", "FROM_B",
                         NULL, NULL);
    theme_register (&fake_plugin_b, NULL, "merge_test", o2);
    hashtable_free (o2);

    LONGS_EQUAL(WEECHAT_RC_OK, theme_apply ("merge_test"));
    /* later contribution wins */
    STRCMP_EQUAL("FROM_B", CONFIG_STRING(opt));

    config_file_option_set (opt, saved_value, 1);
    config_file_option_set (config_look_theme_backup,
                            (saved_backup) ? "on" : "off", 1);
    free (saved_value);
}

/*
 * Test functions:
 *   theme_reset
 */

TEST(CoreTheme, Reset)
{
    struct t_hashtable *overrides;
    struct t_config_option *opt_prefix_error;
    char *saved_prefix_error, *saved_theme_label, *default_prefix_error;
    int saved_backup;

    opt_prefix_error = NULL;
    config_file_search_with_string ("weechat.look.prefix_error",
                                    NULL, NULL, &opt_prefix_error, NULL);
    CHECK(opt_prefix_error != NULL);
    saved_prefix_error = strdup (CONFIG_STRING(opt_prefix_error));
    saved_theme_label = strdup (CONFIG_STRING(config_look_theme));
    saved_backup = CONFIG_BOOLEAN(config_look_theme_backup);
    default_prefix_error = strdup (CONFIG_STRING_DEFAULT(opt_prefix_error));

    config_file_option_set (config_look_theme_backup, "off", 1);

    /* set up a non-default state: apply a theme that flips one option
       and sets weechat.look.theme as a side effect */
    overrides = make_overrides ("weechat.look.prefix_error", "RESET_ME!",
                                NULL, NULL);
    theme_register (NULL, NULL, "reset_test", overrides);
    hashtable_free (overrides);
    LONGS_EQUAL(WEECHAT_RC_OK, theme_apply ("reset_test"));
    STRCMP_EQUAL("RESET_ME!", CONFIG_STRING(opt_prefix_error));
    STRCMP_EQUAL("reset_test", CONFIG_STRING(config_look_theme));

    /* reset: themable option goes back to its default, label is cleared */
    LONGS_EQUAL(WEECHAT_RC_OK, theme_reset ());
    STRCMP_EQUAL(default_prefix_error, CONFIG_STRING(opt_prefix_error));
    STRCMP_EQUAL(CONFIG_STRING_DEFAULT(config_look_theme),
                 CONFIG_STRING(config_look_theme));

    /* restore */
    config_file_option_set (opt_prefix_error, saved_prefix_error, 1);
    config_file_option_set (config_look_theme, saved_theme_label, 1);
    config_file_option_set (config_look_theme_backup,
                            (saved_backup) ? "on" : "off", 1);

    free (saved_prefix_error);
    free (saved_theme_label);
    free (default_prefix_error);
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
    LONGS_EQUAL(3, theme_overrides_count (theme));
    STRCMP_EQUAL("default",
                 theme_get_override (theme,
                                              "weechat.color.chat"));
    STRCMP_EQUAL("blue",
                 theme_get_override (theme,
                                              "weechat.color.separator"));
    STRCMP_EQUAL("lightcyan",
                 theme_get_override (theme,
                                              "irc.color.input_nick"));
    POINTERS_EQUAL(NULL, theme_get_override (theme, "unknown_info_key"));

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
    LONGS_EQUAL(0, theme_overrides_count (theme));
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
    LONGS_EQUAL(1, theme_overrides_count (theme));
    STRCMP_EQUAL("red",
                 theme_get_override (theme,
                                              "weechat.color.chat"));
    POINTERS_EQUAL(NULL, theme_get_override (theme, "ignored"));
    theme_free (theme);
    unlink (path);
}

/*
 * Test functions:
 *   theme_save
 */

TEST(CoreTheme, Save)
{
    char *path;
    struct stat st;

    /* NULL / empty => error, no file */
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_save (NULL));
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_save (""));

    /* reserved "backup-" prefix => error */
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_save ("backup-anything"));

    /* name colliding with a built-in is refused */
    theme_register (NULL, NULL, "dark", NULL);
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_save ("dark"));

    /* happy path: full snapshot => file exists, with options written */
    LONGS_EQUAL(WEECHAT_RC_OK, theme_save ("save_test"));
    path = theme_user_file_path ("save_test");
    CHECK(path != NULL);
    LONGS_EQUAL(0, stat (path, &st));
    CHECK(st.st_size > 0);
    unlink (path);
    free (path);
}

/*
 * Test functions:
 *   theme_delete
 */

TEST(CoreTheme, Delete)
{
    char *path;
    struct stat st;

    /* NULL / empty => error */
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_delete (NULL));
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_delete (""));

    /* refuses to delete a built-in (no file to delete) */
    theme_register (NULL, NULL, "dark", NULL);
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_delete ("dark"));

    /* missing file => error */
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_delete ("does_not_exist"));

    /* happy path: write a file via theme_save (also ensures the themes
       directory exists), delete it, confirm it is gone */
    LONGS_EQUAL(WEECHAT_RC_OK, theme_save ("del_test"));
    path = theme_user_file_path ("del_test");
    CHECK(path != NULL);
    LONGS_EQUAL(0, stat (path, &st));

    LONGS_EQUAL(WEECHAT_RC_OK, theme_delete ("del_test"));
    LONGS_EQUAL(-1, stat (path, &st));
    free (path);
}

/*
 * Test functions:
 *   theme_rename
 */

TEST(CoreTheme, Rename)
{
    char *src_path, *dst_path;
    struct stat st;
    FILE *file;
    char buf[2048];
    size_t len;

    /* NULL / empty arguments => error */
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_rename (NULL, "dst"));
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_rename ("src", NULL));
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_rename ("", "dst"));
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_rename ("src", ""));

    /* refuses to rename a built-in (no file to rename) */
    theme_register (NULL, NULL, "dark", NULL);
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_rename ("dark", "renamed"));

    /* refuses target == reserved "backup-" prefix */
    LONGS_EQUAL(WEECHAT_RC_OK, theme_save ("rn_src"));
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_rename ("rn_src", "backup-foo"));

    /* refuses target == built-in name */
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_rename ("rn_src", "dark"));

    /* refuses same name */
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_rename ("rn_src", "rn_src"));

    /* source missing => error */
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_rename ("does_not_exist", "rn_dst"));

    /* refuses target that already exists */
    LONGS_EQUAL(WEECHAT_RC_OK, theme_save ("rn_dst"));
    LONGS_EQUAL(WEECHAT_RC_ERROR, theme_rename ("rn_src", "rn_dst"));
    LONGS_EQUAL(WEECHAT_RC_OK, theme_delete ("rn_dst"));

    /* happy path: rename moves the file and rewrites the [info] name */
    src_path = theme_user_file_path ("rn_src");
    dst_path = theme_user_file_path ("rn_dst");
    CHECK(src_path != NULL);
    CHECK(dst_path != NULL);

    LONGS_EQUAL(WEECHAT_RC_OK, theme_rename ("rn_src", "rn_dst"));

    /* old file gone, new file exists */
    LONGS_EQUAL(-1, stat (src_path, &st));
    LONGS_EQUAL(0, stat (dst_path, &st));

    /* [info] name field inside the renamed file is updated */
    file = fopen (dst_path, "r");
    CHECK(file != NULL);
    len = fread (buf, 1, sizeof (buf) - 1, file);
    buf[len] = '\0';
    fclose (file);
    CHECK(strstr (buf, "name = \"rn_dst\"") != NULL);
    CHECK(strstr (buf, "name = \"rn_src\"") == NULL);

    /* if weechat.look.theme pointed at the old name, the label moves too */
    LONGS_EQUAL(WEECHAT_RC_OK, theme_save ("rn_active"));
    config_file_option_set (config_look_theme, "rn_active", 1);
    LONGS_EQUAL(WEECHAT_RC_OK, theme_rename ("rn_active", "rn_moved"));
    STRCMP_EQUAL("rn_moved", CONFIG_STRING(config_look_theme));

    /* cleanup */
    config_file_option_reset (config_look_theme, 1);
    theme_delete ("rn_dst");
    theme_delete ("rn_moved");
    free (src_path);
    free (dst_path);
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
    theme_register (NULL, NULL, "dark", NULL);
    theme_register (NULL, NULL, "light", NULL);
    CHECK(themes != NULL);

    theme_end ();
    POINTERS_EQUAL(NULL, themes);
    POINTERS_EQUAL(NULL, last_theme);
}

/*
 * Test functions:
 *   theme_unregister_plugin
 *   theme_unregister_script
 *   theme_register (per-contributor identity)
 */

TEST(CoreTheme, UnregisterByOwner)
{
    struct t_weechat_plugin fake_plugin_a, fake_plugin_b;
    int script_a = 0, script_b = 0;
    struct t_hashtable *o1, *o2, *o3, *o4;
    struct t_theme *theme;

    /* four contributors register against the same theme:
       core (NULL, NULL), plugin_a (no script), plugin_b (no script),
       and an individual script under plugin_a */
    o1 = make_overrides ("weechat.color.chat",      "default", NULL, NULL);
    o2 = make_overrides ("irc.color.input_nick",    "cyan",    NULL, NULL);
    o3 = make_overrides ("fset.color.title_filter", "18",      NULL, NULL);
    o4 = make_overrides ("weechat.color.separator", "251",     NULL, NULL);

    theme_register (NULL,           NULL,       "light", o1);
    theme_register (&fake_plugin_a, NULL,       "light", o2);
    theme_register (&fake_plugin_b, NULL,       "light", o3);
    theme_register (&fake_plugin_a, &script_a,  "light", o4);

    hashtable_free (o1);
    hashtable_free (o2);
    hashtable_free (o3);
    hashtable_free (o4);

    theme = theme_search ("light");
    CHECK(theme != NULL);
    LONGS_EQUAL(4, theme_overrides_count (theme));

    /* dropping plugin_a's plugin-level contribution leaves core,
       plugin_b, and plugin_a's script contributions intact */
    theme_unregister_plugin (&fake_plugin_a);
    LONGS_EQUAL(3, theme_overrides_count (theme));
    STRCMP_EQUAL("default",
                 theme_get_override (theme, "weechat.color.chat"));
    POINTERS_EQUAL(NULL,
                   theme_get_override (theme, "irc.color.input_nick"));
    STRCMP_EQUAL("18",
                 theme_get_override (theme, "fset.color.title_filter"));
    STRCMP_EQUAL("251",
                 theme_get_override (theme, "weechat.color.separator"));

    /* dropping the script contribution leaves only core and plugin_b */
    theme_unregister_script (&fake_plugin_a, &script_a);
    LONGS_EQUAL(2, theme_overrides_count (theme));
    POINTERS_EQUAL(NULL,
                   theme_get_override (theme, "weechat.color.separator"));

    /* unrelated owners are no-ops */
    theme_unregister_plugin (&fake_plugin_a);   /* already gone */
    theme_unregister_script (&fake_plugin_b, &script_b);  /* never registered */
    LONGS_EQUAL(2, theme_overrides_count (theme));
}

TEST(CoreTheme, RegisterMergesPerContributor)
{
    struct t_weechat_plugin fake_plugin;
    struct t_hashtable *a, *b;
    struct t_theme *theme;

    /* two successive registrations from the same (plugin, script)
       merge into a single contribution */
    a = make_overrides ("k1", "v1", "k2", "v2");
    b = make_overrides ("k2", "newv2", "k3", "v3");

    theme_register (&fake_plugin, NULL, "X", a);
    theme = theme_register (&fake_plugin, NULL, "X", b);
    hashtable_free (a);
    hashtable_free (b);

    CHECK(theme != NULL);
    /* one contribution, 3 keys (k1, k2, k3) */
    CHECK(theme->contributions != NULL);
    POINTERS_EQUAL(NULL, theme->contributions->next_contribution);
    LONGS_EQUAL(3, theme->contributions->overrides->items_count);
    STRCMP_EQUAL("v1",    theme_get_override (theme, "k1"));
    STRCMP_EQUAL("newv2", theme_get_override (theme, "k2"));
    STRCMP_EQUAL("v3",    theme_get_override (theme, "k3"));
}

/*
 * Test functions:
 *   theme_builtin_init
 *   theme_builtin_register_entries
 */

TEST(CoreTheme, BuiltinInit)
{
    struct t_theme *theme;

    /* registry is empty after setup() */
    POINTERS_EQUAL(NULL, theme_search ("light"));

    theme_builtin_init ();

    /* the "light" theme is registered */
    theme = theme_search ("light");
    CHECK(theme != NULL);

    /* sanity check: many core color overrides (>= 30) */
    CHECK(theme_overrides_count (theme) >= 30);

    /* spot-check a few known entries from the core light table */
    STRCMP_EQUAL("cyan",
                 theme_get_override (theme,
                                              "weechat.color.chat_nick"));
    STRCMP_EQUAL("251",
                 theme_get_override (theme,
                                              "weechat.color.separator"));
    STRCMP_EQUAL("254",
                 theme_get_override (theme,
                                              "weechat.bar.status.color_bg"));

    /* idempotency: a second call merges (no duplicate themes, count
       stays the same because the same keys are re-inserted) */
    int count_before = theme_overrides_count (theme);
    theme_builtin_init ();
    theme = theme_search ("light");
    CHECK(theme != NULL);
    LONGS_EQUAL(count_before, theme_overrides_count (theme));
}
