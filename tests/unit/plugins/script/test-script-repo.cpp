/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test script repository functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>
#include <sys/stat.h>
#include <zlib.h>
#include "src/core/core-config-file.h"
#include "src/core/core-dir.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/core/core-string.h"
#include "src/gui/gui-color.h"
#include "src/plugins/script/script.h"
#include "src/plugins/script/script-config.h"
#include "src/plugins/script/script-repo.h"
#include "src/plugins/weechat-plugin.h"

extern int script_repo_script_is_held (struct t_script_repo *script);
extern char *script_repo_sha512sum_file (const char *filename);
extern void script_repo_remove (struct t_script_repo *script);
extern int script_repo_file_update_url_cb (const void *pointer, void *data,
                                           const char *url,
                                           struct t_hashtable *options,
                                           struct t_hashtable *output);
}

/* Content of script "go.py" and its SHA-512 checksum. */
#define TEST_SCRIPT_CONTENT "print('test')\n"
#define TEST_SCRIPT_SHA512                                              \
    "f716f155e9ae59caa9a7d5928e0592b39c8e582d1aa44782514d9aa5c1535f77"  \
    "38c4e0a26507ef336e55d7a56ad924e76ef7fa8a0bd4e0b133d448d843b81151"

/* Repository file: 3 valid scripts and some invalid ones. */
const char *test_script_repo_xml =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<plugins>\n"
    "  <name>ignored outside of a plugin</name>\n"
    /* Valid script: go.py */
    "  <plugin id=\"1\">\n"
    "    <name>go</name>\n"
    "    <version>3.0</version>\n"
    "    <url>https://weechat.org/files/scripts/go.py</url>\n"
    "    <language>python</language>\n"
    "    <license>GPL3</license>\n"
    "    <md5sum>ignored</md5sum>\n"
    "    <sha512sum>" TEST_SCRIPT_SHA512 "</sha512sum>\n"
    "    <tags>buffer,go</tags>\n"
    "    <desc_en>Quick jump to buffers</desc_en>\n"
    "    <desc_en_US>Jump to buffers (US)</desc_en_US>\n"
    "    <desc_fr>Saut rapide vers les tampons</desc_fr>\n"
    "    <requirements>python3</requirements>\n"
    "    <min_weechat>0.3.0</min_weechat>\n"
    "    <author>alice</author>\n"
    "    <mail>alice@example.com</mail>\n"
    "    <popularity>10</popularity>\n"
    "    <added>2009-04-05 18:22:11</added>\n"
    "    <updated>2026-01-02 03:04:05</updated>\n"
    "    <unknown_tag>ignored</unknown_tag>\n"
    "    <name_without_end>\n"
    "    no tag\n"
    "  </plugin>\n"
    /* Valid script: buffers.pl (with XML entities). */
    "  <plugin id=\"2\">\n"
    "    <name>buffers</name>\n"
    "    <version>5.0</version>\n"
    "    <language>perl</language>\n"
    "    <tags>buffers,bar</tags>\n"
    "    <desc_en>Sidebar &amp; list of buffers &lt;b&gt;</desc_en>\n"
    "    <author>bob</author>\n"
    "    <popularity>5</popularity>\n"
    "    <added>2015-06-07 08:09:10</added>\n"
    "    <updated>2020-02-03 04:05:06</updated>\n"
    "  </plugin>\n"
    /* Valid script: autosort.py (invalid popularity and dates). */
    "  <plugin id=\"3\">\n"
    "    <name>autosort</name>\n"
    "    <version>1.0</version>\n"
    "    <language>python</language>\n"
    "    <tags>buffers,sort</tags>\n"
    "    <desc_en>Automatically keep buffers grouped</desc_en>\n"
    "    <author>carol</author>\n"
    "    <popularity>abc</popularity>\n"
    "    <added>invalid date</added>\n"
    "    <updated>2020-02-03 04:05:06 extra</updated>\n"
    "  </plugin>\n"
    /* Invalid: name with a directory separator. */
    "  <plugin id=\"4\">\n"
    "    <name>../evil</name>\n"
    "    <language>python</language>\n"
    "    <desc_en>Evil script</desc_en>\n"
    "    <author>eve</author>\n"
    "  </plugin>\n"
    /* Invalid: unknown language. */
    "  <plugin id=\"5\">\n"
    "    <name>cobol_script</name>\n"
    "    <language>cobol</language>\n"
    "    <desc_en>Script in COBOL</desc_en>\n"
    "    <author>eve</author>\n"
    "  </plugin>\n"
    /* Invalid: WeeChat too old. */
    "  <plugin id=\"6\">\n"
    "    <name>future</name>\n"
    "    <language>python</language>\n"
    "    <desc_en>Script for the future</desc_en>\n"
    "    <author>eve</author>\n"
    "    <min_weechat>99.0.0</min_weechat>\n"
    "  </plugin>\n"
    /* Invalid: WeeChat too recent. */
    "  <plugin id=\"7\">\n"
    "    <name>past</name>\n"
    "    <language>python</language>\n"
    "    <desc_en>Script from the past</desc_en>\n"
    "    <author>eve</author>\n"
    "    <max_weechat>0.1.0</max_weechat>\n"
    "  </plugin>\n"
    /* Invalid: no English description. */
    "  <plugin id=\"8\">\n"
    "    <name>french_only</name>\n"
    "    <language>python</language>\n"
    "    <desc_fr>Script en français</desc_fr>\n"
    "    <author>eve</author>\n"
    "  </plugin>\n"
    /* Invalid: no name. */
    "  <plugin id=\"9\">\n"
    "    <language>python</language>\n"
    "    <desc_en>Script without name</desc_en>\n"
    "    <author>eve</author>\n"
    "  </plugin>\n"
    /* Invalid: plugin not closed, replaced by the next one. */
    "  <plugin id=\"10\">\n"
    "    <name>not_closed</name>\n"
    "    <language>python</language>\n"
    "    <desc_en>Script not closed</desc_en>\n"
    "    <author>eve</author>\n"
    "  <plugin id=\"11\">\n"
    "  </plugin>\n"
    "</plugins>\n";

TEST_GROUP(ScriptRepo)
{
};

TEST_GROUP(ScriptRepoWithFile)
{
    char *xml_filename;
    char *data_dir;

    /*
     * Write the repository file (gzip compressed).
     */

    void write_repo_file (const char *content)
    {
        gzFile file;

        file = gzopen (xml_filename, "w");
        CHECK(file);
        if (content[0])
            CHECK(gzputs (file, content) > 0);
        gzclose (file);
    }

    /*
     * Write a file in WeeChat data directory.
     *
     * Note: result must be freed after use.
     */

    char *write_data_file (const char *path, const char *content)
    {
        char *filename, *pos;
        FILE *file;

        CHECK(string_asprintf (&filename, "%s/%s", data_dir, path) >= 0);
        pos = strrchr (filename, '/');
        pos[0] = '\0';
        dir_mkdir_parents (filename, 0700);
        pos[0] = '/';
        file = fopen (filename, "w");
        CHECK(file);
        fputs (content, file);
        fclose (file);
        return filename;
    }

    /*
     * Remove a file in WeeChat data directory.
     */

    void remove_data_file (const char *path)
    {
        char *filename;

        CHECK(string_asprintf (&filename, "%s/%s", data_dir, path) >= 0);
        unlink (filename);
        free (filename);
    }

    /*
     * Return names of all scripts, in order, separated by commas
     * (with a "-" prefix if the script is not displayed).
     */

    void get_scripts (char *result, int size)
    {
        struct t_script_repo *ptr_script;

        result[0] = '\0';
        for (ptr_script = scripts_repo; ptr_script;
             ptr_script = ptr_script->next_script)
        {
            if (result[0])
                strncat (result, ",", size - strlen (result) - 1);
            if (!ptr_script->displayed)
                strncat (result, "-", size - strlen (result) - 1);
            strncat (result, ptr_script->name_with_extension,
                     size - strlen (result) - 1);
        }
    }

    void setup ()
    {
        xml_filename = script_config_get_xml_filename ();
        CHECK(xml_filename);
        data_dir = hook_info_get (NULL, "weechat_data_dir", NULL);
        CHECK(data_dir);
        dir_mkdir_parents (xml_filename, 0700);
        rmdir (xml_filename);
        write_repo_file (test_script_repo_xml);
    }

    void teardown ()
    {
        script_repo_remove_all ();
        script_repo_set_filter (NULL);
        unlink (xml_filename);
        free (xml_filename);
        remove_data_file ("python/autoload/go.py");
        remove_data_file ("python/go.py");
        free (data_dir);
        hashtable_remove (script_loaded, "go.py");
        config_file_option_reset (script_config_look_sort, 1);
        config_file_option_reset (script_config_look_translate_description, 1);
        config_file_option_reset (script_config_scripts_cache_expire, 1);
        config_file_option_reset (script_config_scripts_hold, 1);
    }
};

/*
 * Test functions:
 *   script_repo_script_name_valid
 */

TEST(ScriptRepo, ScriptNameValid)
{
    /* Invalid script name */
    LONGS_EQUAL(0, script_repo_script_name_valid (NULL));
    LONGS_EQUAL(0, script_repo_script_name_valid (""));

    /* Directory separator */
    LONGS_EQUAL(0, script_repo_script_name_valid ("/"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("/etc/passwd"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("../test"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("../../test"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("dir/test"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("test/"));

    /* Windows directory separator */
    LONGS_EQUAL(0, script_repo_script_name_valid ("\\"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("..\\test"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("dir\\test"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("test\\"));

    /* Windows drive separator */
    LONGS_EQUAL(0, script_repo_script_name_valid (":"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("c:"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("c:test"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("test:"));

    /*
     * "." and ".." are accepted: they are single path components and the
     * script extension is always appended to the name, so they can not be
     * used to escape the scripts directory.
     */
    LONGS_EQUAL(1, script_repo_script_name_valid ("."));
    LONGS_EQUAL(1, script_repo_script_name_valid (".."));

    /* Valid script name */
    LONGS_EQUAL(1, script_repo_script_name_valid ("a"));
    LONGS_EQUAL(1, script_repo_script_name_valid ("go"));
    LONGS_EQUAL(1, script_repo_script_name_valid ("iset"));
    LONGS_EQUAL(1, script_repo_script_name_valid (".test"));
    LONGS_EQUAL(1, script_repo_script_name_valid ("test.py"));
    LONGS_EQUAL(1, script_repo_script_name_valid ("buffer_autoset"));
    LONGS_EQUAL(1, script_repo_script_name_valid ("weechat-script"));
    LONGS_EQUAL(1, script_repo_script_name_valid ("noël"));
}

/*
 * Test functions:
 *   script_repo_file_read
 */

TEST(ScriptRepoWithFile, FileReadErrors)
{
    /* File not found. */
    unlink (xml_filename);
    LONGS_EQUAL(0, script_repo_file_read (1));
    POINTERS_EQUAL(NULL, scripts_repo);

    /* Empty file. */
    write_repo_file ("");
    LONGS_EQUAL(1, script_repo_file_read (1));
    POINTERS_EQUAL(NULL, scripts_repo);
    LONGS_EQUAL(0, script_repo_count);

    /* Not a list of scripts. */
    write_repo_file ("test\n<html>\n</html>\n");
    LONGS_EQUAL(1, script_repo_file_read (1));
    POINTERS_EQUAL(NULL, scripts_repo);
}

/*
 * Test functions:
 *   script_repo_file_read
 *   script_repo_alloc
 *   script_repo_add
 *   script_repo_free
 *   script_repo_remove
 *   script_repo_remove_all
 */

TEST(ScriptRepoWithFile, FileRead)
{
    struct t_script_repo *ptr_script;
    char scripts[1024];

    LONGS_EQUAL(1, script_repo_file_read (1));

    /* Default sort: "i,p,n" (installed, popularity, name). */
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("go.py,buffers.pl,autosort.py", scripts);
    LONGS_EQUAL(3, script_repo_count);
    LONGS_EQUAL(3, script_repo_count_displayed);

    /* Script "go.py": description translated for locale "en_US". */
    ptr_script = scripts_repo;
    STRCMP_EQUAL("go", ptr_script->name);
    STRCMP_EQUAL("go.py", ptr_script->name_with_extension);
    STRCMP_EQUAL("python", script_language[ptr_script->language]);
    STRCMP_EQUAL("alice", ptr_script->author);
    STRCMP_EQUAL("alice@example.com", ptr_script->mail);
    STRCMP_EQUAL("3.0", ptr_script->version);
    STRCMP_EQUAL("GPL3", ptr_script->license);
    STRCMP_EQUAL("Jump to buffers (US)", ptr_script->description);
    STRCMP_EQUAL("buffer,go", ptr_script->tags);
    STRCMP_EQUAL("python3", ptr_script->requirements);
    STRCMP_EQUAL("0.3.0", ptr_script->min_weechat);
    POINTERS_EQUAL(NULL, ptr_script->max_weechat);
    STRCMP_EQUAL(TEST_SCRIPT_SHA512, ptr_script->sha512sum);
    STRCMP_EQUAL("https://weechat.org/files/scripts/go.py", ptr_script->url);
    LONGS_EQUAL(10, ptr_script->popularity);
    LONGS_EQUAL(1238955731, ptr_script->date_added);
    LONGS_EQUAL(1767323045, ptr_script->date_updated);
    LONGS_EQUAL(0, ptr_script->status);
    LONGS_EQUAL(1, ptr_script->displayed);

    /* Script "buffers.pl": XML entities are decoded. */
    ptr_script = ptr_script->next_script;
    STRCMP_EQUAL("buffers.pl", ptr_script->name_with_extension);
    STRCMP_EQUAL("perl", script_language[ptr_script->language]);
    STRCMP_EQUAL("Sidebar & list of buffers <b>", ptr_script->description);
    LONGS_EQUAL(1433664550, ptr_script->date_added);
    LONGS_EQUAL(1580702706, ptr_script->date_updated);

    /* Script "autosort.py": invalid popularity and dates are ignored. */
    ptr_script = ptr_script->next_script;
    STRCMP_EQUAL("autosort.py", ptr_script->name_with_extension);
    LONGS_EQUAL(0, ptr_script->popularity);
    LONGS_EQUAL(0, ptr_script->date_added);
    LONGS_EQUAL(0, ptr_script->date_updated);
    POINTERS_EQUAL(NULL, ptr_script->next_script);
    POINTERS_EQUAL(ptr_script, last_script_repo);

    /* Max length of fields. */
    LONGS_EQUAL(8, *((int *)hashtable_get (script_repo_max_length_field, "N")));
    LONGS_EQUAL(11, *((int *)hashtable_get (script_repo_max_length_field, "n")));
    LONGS_EQUAL(6, *((int *)hashtable_get (script_repo_max_length_field, "l")));
    LONGS_EQUAL(5, *((int *)hashtable_get (script_repo_max_length_field, "a")));

    /* Description not translated. */
    config_file_option_set (script_config_look_translate_description,
                            "off", 1);
    LONGS_EQUAL(1, script_repo_file_read (1));
    STRCMP_EQUAL("Quick jump to buffers", scripts_repo->description);

    /* Read again: scripts are replaced. */
    LONGS_EQUAL(1, script_repo_file_read (1));
    LONGS_EQUAL(3, script_repo_count);

    /* Remove scripts. */
    script_repo_remove (scripts_repo->next_script);
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("go.py,autosort.py", scripts);
    LONGS_EQUAL(2, script_repo_count);
    LONGS_EQUAL(2, script_repo_count_displayed);
    script_repo_remove (last_script_repo);
    STRCMP_EQUAL("go.py", last_script_repo->name_with_extension);
    script_repo_remove (scripts_repo);
    POINTERS_EQUAL(NULL, scripts_repo);
    POINTERS_EQUAL(NULL, last_script_repo);
    LONGS_EQUAL(0, script_repo_count);
    LONGS_EQUAL(0, script_repo_count_displayed);

    script_repo_remove_all ();
    POINTERS_EQUAL(NULL, script_repo_max_length_field);
}

/*
 * Test functions:
 *   script_repo_compare_scripts
 *   script_repo_find_pos
 */

TEST(ScriptRepoWithFile, Sort)
{
    char scripts[1024];

    /* Name. */
    config_file_option_set (script_config_look_sort, "n", 1);
    script_repo_file_read (1);
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("autosort.py,buffers.pl,go.py", scripts);

    /* Name, reverse order. */
    config_file_option_set (script_config_look_sort, "-n", 1);
    script_repo_file_read (1);
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("go.py,buffers.pl,autosort.py", scripts);

    /* Author. */
    config_file_option_set (script_config_look_sort, "a", 1);
    script_repo_file_read (1);
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("go.py,buffers.pl,autosort.py", scripts);

    /* Date added (newest first). */
    config_file_option_set (script_config_look_sort, "d", 1);
    script_repo_file_read (1);
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("buffers.pl,go.py,autosort.py", scripts);

    /* Date updated (newest first). */
    config_file_option_set (script_config_look_sort, "u", 1);
    script_repo_file_read (1);
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("go.py,buffers.pl,autosort.py", scripts);

    /* Language, then name. */
    config_file_option_set (script_config_look_sort, "l,n", 1);
    script_repo_file_read (1);
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("buffers.pl,autosort.py,go.py", scripts);

    /* Extension, then name reversed. */
    config_file_option_set (script_config_look_sort, "e,-n", 1);
    script_repo_file_read (1);
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("buffers.pl,go.py,autosort.py", scripts);

    /* Popularity, reversed. */
    config_file_option_set (script_config_look_sort, "-p", 1);
    script_repo_file_read (1);
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("autosort.py,buffers.pl,go.py", scripts);

    /* Status "installed" first. */
    free (write_data_file ("python/autosort.py", "test"));
    config_file_option_set (script_config_look_sort, "i,n", 1);
    script_repo_file_read (1);
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("autosort.py,buffers.pl,go.py", scripts);
    remove_data_file ("python/autosort.py");

    /* Unknown sort keys are ignored. */
    config_file_option_set (script_config_look_sort, "z,x,n", 1);
    script_repo_file_read (1);
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("autosort.py,buffers.pl,go.py", scripts);
}

/*
 * Test functions:
 *   script_repo_script_valid
 *   script_repo_search_displayed_by_number
 *   script_repo_search_by_name
 *   script_repo_search_by_name_ext
 */

TEST(ScriptRepoWithFile, Search)
{
    struct t_script_repo script;

    LONGS_EQUAL(1, script_repo_file_read (1));

    LONGS_EQUAL(0, script_repo_script_valid (NULL));
    LONGS_EQUAL(0, script_repo_script_valid (&script));
    LONGS_EQUAL(1, script_repo_script_valid (scripts_repo));
    LONGS_EQUAL(1, script_repo_script_valid (last_script_repo));

    POINTERS_EQUAL(NULL, script_repo_search_by_name (NULL));
    POINTERS_EQUAL(NULL, script_repo_search_by_name (""));
    POINTERS_EQUAL(NULL, script_repo_search_by_name ("Go"));
    POINTERS_EQUAL(NULL, script_repo_search_by_name ("go.py"));
    POINTERS_EQUAL(NULL, script_repo_search_by_name ("evil"));
    POINTERS_EQUAL(scripts_repo, script_repo_search_by_name ("go"));
    POINTERS_EQUAL(last_script_repo, script_repo_search_by_name ("autosort"));

    POINTERS_EQUAL(NULL, script_repo_search_by_name_ext (NULL));
    POINTERS_EQUAL(NULL, script_repo_search_by_name_ext ("go"));
    POINTERS_EQUAL(NULL, script_repo_search_by_name_ext ("go.pl"));
    POINTERS_EQUAL(scripts_repo, script_repo_search_by_name_ext ("go.py"));
    POINTERS_EQUAL(scripts_repo->next_script,
                   script_repo_search_by_name_ext ("buffers.pl"));

    POINTERS_EQUAL(NULL, script_repo_search_displayed_by_number (-1));
    POINTERS_EQUAL(NULL, script_repo_search_displayed_by_number (3));
    POINTERS_EQUAL(scripts_repo, script_repo_search_displayed_by_number (0));
    POINTERS_EQUAL(last_script_repo,
                   script_repo_search_displayed_by_number (2));

    /* Only displayed scripts are counted. */
    scripts_repo->displayed = 0;
    POINTERS_EQUAL(scripts_repo->next_script,
                   script_repo_search_displayed_by_number (0));
    POINTERS_EQUAL(NULL, script_repo_search_displayed_by_number (2));
    scripts_repo->displayed = 1;
}

/*
 * Test functions:
 *   script_repo_set_filter
 *   script_repo_match_filter
 *   script_repo_filter_scripts
 */

TEST(ScriptRepoWithFile, Filter)
{
    char scripts[1024];

    LONGS_EQUAL(1, script_repo_file_read (1));

    /* No filter or "*": all scripts. */
    script_repo_filter_scripts (NULL);
    POINTERS_EQUAL(NULL, script_repo_filter);
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("go.py,buffers.pl,autosort.py", scripts);
    LONGS_EQUAL(3, script_repo_count_displayed);
    script_repo_filter_scripts ("*");
    STRCMP_EQUAL("*", script_repo_filter);
    LONGS_EQUAL(3, script_repo_count_displayed);

    /* Tag (case insensitive). */
    script_repo_filter_scripts ("BAR");
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("-go.py,buffers.pl,-autosort.py", scripts);
    LONGS_EQUAL(1, script_repo_count_displayed);

    /* Name with extension. */
    script_repo_filter_scripts ("AUTO");
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("-go.py,-buffers.pl,autosort.py", scripts);

    /* Language and extension. */
    script_repo_filter_scripts ("perl");
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("-go.py,buffers.pl,-autosort.py", scripts);
    script_repo_filter_scripts ("pl");
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("-go.py,buffers.pl,-autosort.py", scripts);
    script_repo_filter_scripts ("py");
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("go.py,-buffers.pl,autosort.py", scripts);

    /* Description. */
    script_repo_filter_scripts ("grouped");
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("-go.py,-buffers.pl,autosort.py", scripts);

    /* Tag, name or description. */
    script_repo_filter_scripts ("buffers");
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("go.py,buffers.pl,autosort.py", scripts);

    /* All words must match. */
    script_repo_filter_scripts ("buffers  jump");
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("go.py,-buffers.pl,-autosort.py", scripts);
    LONGS_EQUAL(1, script_repo_count_displayed);
    script_repo_filter_scripts ("python perl");
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("-go.py,-buffers.pl,-autosort.py", scripts);
    LONGS_EQUAL(0, script_repo_count_displayed);

    /* No match. */
    script_repo_filter_scripts ("xyz");
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("-go.py,-buffers.pl,-autosort.py", scripts);

    /* Filter is applied when the file is read. */
    script_repo_set_filter ("sort");
    STRCMP_EQUAL("sort", script_repo_filter);
    LONGS_EQUAL(1, script_repo_file_read (1));
    get_scripts (scripts, sizeof (scripts));
    STRCMP_EQUAL("-go.py,-buffers.pl,autosort.py", scripts);
    LONGS_EQUAL(1, script_repo_count_displayed);
}

/*
 * Test functions:
 *   script_repo_script_is_held
 */

TEST(ScriptRepoWithFile, ScriptIsHeld)
{
    struct t_script_repo *ptr_script;

    LONGS_EQUAL(1, script_repo_file_read (1));
    ptr_script = script_repo_search_by_name_ext ("go.py");
    CHECK(ptr_script);

    LONGS_EQUAL(0, script_repo_script_is_held (ptr_script));

    config_file_option_set (script_config_scripts_hold, "go.py", 1);
    LONGS_EQUAL(1, script_repo_script_is_held (ptr_script));
    config_file_option_set (script_config_scripts_hold, "a.py,go.py", 1);
    LONGS_EQUAL(1, script_repo_script_is_held (ptr_script));
    config_file_option_set (script_config_scripts_hold, "go.py,b.pl", 1);
    LONGS_EQUAL(1, script_repo_script_is_held (ptr_script));
    config_file_option_set (script_config_scripts_hold, "xgo.py,go.py", 1);
    LONGS_EQUAL(1, script_repo_script_is_held (ptr_script));

    /* Partial names are not held. */
    config_file_option_set (script_config_scripts_hold, "xgo.py", 1);
    LONGS_EQUAL(0, script_repo_script_is_held (ptr_script));
    config_file_option_set (script_config_scripts_hold, "go.py2", 1);
    LONGS_EQUAL(0, script_repo_script_is_held (ptr_script));
    config_file_option_set (script_config_scripts_hold, "go", 1);
    LONGS_EQUAL(0, script_repo_script_is_held (ptr_script));
    config_file_option_set (script_config_scripts_hold, "a.go.py,go.pyc", 1);
    LONGS_EQUAL(0, script_repo_script_is_held (ptr_script));
}

/*
 * Test functions:
 *   script_repo_sha512sum_file
 */

TEST(ScriptRepoWithFile, Sha512sumFile)
{
    char *filename, *str;

    POINTERS_EQUAL(NULL, script_repo_sha512sum_file ("/tmp/does/not/exist"));

    filename = write_data_file ("python/go.py", TEST_SCRIPT_CONTENT);
    WEE_TEST_STR(TEST_SCRIPT_SHA512, script_repo_sha512sum_file (filename));
    free (filename);

    filename = write_data_file ("python/go.py", "");
    WEE_TEST_STR("cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d"
                 "36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a5"
                 "38327af927da3e",
                 script_repo_sha512sum_file (filename));
    free (filename);
}

/*
 * Test functions:
 *   script_repo_update_status
 *   script_repo_update_status_all
 *   script_repo_get_filename_loaded
 *   script_repo_get_status_for_display
 *   script_repo_get_status_desc_for_display
 */

TEST(ScriptRepoWithFile, UpdateStatus)
{
    struct t_script_repo *ptr_script;
    char *filename, *filename2, *str, *resolved;

    LONGS_EQUAL(1, script_repo_file_read (1));
    ptr_script = script_repo_search_by_name_ext ("go.py");
    CHECK(ptr_script);

    /* Not installed. */
    script_repo_update_status (ptr_script);
    LONGS_EQUAL(0, ptr_script->status);
    POINTERS_EQUAL(NULL, script_repo_get_filename_loaded (ptr_script));
    STRCMP_EQUAL("", script_repo_get_status_desc_for_display (ptr_script,
                                                              "iaHrN"));

    /* Installed and autoloaded, same version as repository. */
    filename = write_data_file ("python/autoload/go.py", TEST_SCRIPT_CONTENT);
    script_repo_update_status (ptr_script);
    LONGS_EQUAL(SCRIPT_STATUS_INSTALLED | SCRIPT_STATUS_AUTOLOADED,
                ptr_script->status);
    WEE_TEST_STR(filename, script_repo_get_filename_loaded (ptr_script));
    free (filename);

    /* Installed and autoloaded, different version: script is obsolete. */
    free (write_data_file ("python/autoload/go.py", "print('old')\n"));
    script_repo_update_status (ptr_script);
    LONGS_EQUAL(SCRIPT_STATUS_INSTALLED | SCRIPT_STATUS_AUTOLOADED
                | SCRIPT_STATUS_NEW_VERSION,
                ptr_script->status);

    /* No checksum in repository: script is not obsolete. */
    free (ptr_script->sha512sum);
    ptr_script->sha512sum = NULL;
    script_repo_update_status (ptr_script);
    LONGS_EQUAL(SCRIPT_STATUS_INSTALLED | SCRIPT_STATUS_AUTOLOADED,
                ptr_script->status);
    ptr_script->sha512sum = strdup (TEST_SCRIPT_SHA512);

    /* Autoload is a symbolic link: real file is returned. */
    remove_data_file ("python/autoload/go.py");
    filename = write_data_file ("python/go.py", TEST_SCRIPT_CONTENT);
    CHECK(string_asprintf (&filename2, "%s/python/autoload/go.py",
                           data_dir) >= 0);
    LONGS_EQUAL(0, symlink ("../go.py", filename2));
    script_repo_update_status (ptr_script);
    LONGS_EQUAL(SCRIPT_STATUS_INSTALLED | SCRIPT_STATUS_AUTOLOADED,
                ptr_script->status);
    resolved = realpath (filename, NULL);
    CHECK(resolved);
    WEE_TEST_STR(resolved, script_repo_get_filename_loaded (ptr_script));
    free (resolved);
    free (filename2);
    free (filename);

    /* Installed, not autoloaded. */
    remove_data_file ("python/autoload/go.py");
    script_repo_update_status (ptr_script);
    LONGS_EQUAL(SCRIPT_STATUS_INSTALLED, ptr_script->status);

    /* Held. */
    config_file_option_set (script_config_scripts_hold, "go.py", 1);
    script_repo_update_status (ptr_script);
    LONGS_EQUAL(SCRIPT_STATUS_INSTALLED | SCRIPT_STATUS_HELD,
                ptr_script->status);

    /* Running. */
    hashtable_set (script_loaded, "go.py", "2.9");
    script_repo_update_status (ptr_script);
    LONGS_EQUAL(SCRIPT_STATUS_INSTALLED | SCRIPT_STATUS_HELD
                | SCRIPT_STATUS_RUNNING,
                ptr_script->status);
    STRCMP_EQUAL("2.9", ptr_script->version_loaded);
    LONGS_EQUAL(3, *((int *)hashtable_get (script_repo_max_length_field, "V")));
    STRCMP_EQUAL("popular installed held running",
                 script_repo_get_status_desc_for_display (ptr_script,
                                                          "*iaHrN"));
    STRCMP_EQUAL("running popular",
                 script_repo_get_status_desc_for_display (ptr_script, "r*"));
    str = gui_color_decode (
        script_repo_get_status_for_display (ptr_script, "*iaHrN", 0), NULL);
    STRCMP_EQUAL("*i Hr ", str);
    free (str);
    str = gui_color_decode (
        script_repo_get_status_for_display (ptr_script, "*iaHrN", 1), NULL);
    STRCMP_EQUAL("*iHr", str);
    free (str);

    /* Not running any more. */
    hashtable_remove (script_loaded, "go.py");
    config_file_option_reset (script_config_scripts_hold, 1);
    script_repo_update_status_all ();
    LONGS_EQUAL(SCRIPT_STATUS_INSTALLED, ptr_script->status);
    POINTERS_EQUAL(NULL, ptr_script->version_loaded);
    LONGS_EQUAL(0, *((int *)hashtable_get (script_repo_max_length_field, "V")));

    /* Status of an unknown script. */
    str = gui_color_decode (
        script_repo_get_status_for_display (NULL, "*iaHrN?", 0), NULL);
    STRCMP_EQUAL("      ?", str);
    free (str);
    str = gui_color_decode (
        script_repo_get_status_for_display (NULL, "*iaHrN?", 1), NULL);
    STRCMP_EQUAL("?", str);
    free (str);
    STRCMP_EQUAL("", script_repo_get_status_desc_for_display (NULL, "*iaHrN"));
}

/*
 * Test functions:
 *   script_repo_file_exists
 *   script_repo_file_is_uptodate
 */

TEST(ScriptRepoWithFile, FileExistsIsUptodate)
{
    struct utimbuf times;

    /* File exists, with default cache expiration (1 day). */
    LONGS_EQUAL(1, script_repo_file_exists ());
    LONGS_EQUAL(1, script_repo_file_is_uptodate ());

    /* Cache always expires. */
    config_file_option_set (script_config_scripts_cache_expire, "0", 1);
    LONGS_EQUAL(0, script_repo_file_is_uptodate ());

    /* Cache never expires. */
    config_file_option_set (script_config_scripts_cache_expire, "-1", 1);
    LONGS_EQUAL(1, script_repo_file_is_uptodate ());

    /* File modified 2 hours ago: expired with a cache of 60 minutes. */
    times.actime = time (NULL) - 7200;
    times.modtime = times.actime;
    LONGS_EQUAL(0, utime (xml_filename, &times));
    config_file_option_set (script_config_scripts_cache_expire, "60", 1);
    LONGS_EQUAL(0, script_repo_file_is_uptodate ());
    config_file_option_set (script_config_scripts_cache_expire, "180", 1);
    LONGS_EQUAL(1, script_repo_file_is_uptodate ());
    config_file_option_set (script_config_scripts_cache_expire, "-1", 1);
    LONGS_EQUAL(1, script_repo_file_is_uptodate ());

    /* Empty file: not up-to-date. */
    unlink (xml_filename);
    fclose (fopen (xml_filename, "w"));
    LONGS_EQUAL(1, script_repo_file_exists ());
    LONGS_EQUAL(0, script_repo_file_is_uptodate ());

    /* File does not exist. */
    unlink (xml_filename);
    LONGS_EQUAL(0, script_repo_file_exists ());
    LONGS_EQUAL(0, script_repo_file_is_uptodate ());
}

/*
 * Test functions:
 *   script_repo_file_update_url_cb
 */

TEST(ScriptRepoWithFile, FileUpdateUrlCb)
{
    struct t_hashtable *output;

    output = hashtable_new (32, WEECHAT_HASHTABLE_STRING,
                            WEECHAT_HASHTABLE_STRING, NULL, NULL);
    CHECK(output);

    /* Download error: file is not read. */
    hashtable_set (output, "error", "connection refused");
    LONGS_EQUAL(WEECHAT_RC_OK,
                script_repo_file_update_url_cb ((void *)1, NULL, "url",
                                                NULL, output));
    POINTERS_EQUAL(NULL, scripts_repo);

    /* Empty error: file is read. */
    hashtable_set (output, "error", "");
    LONGS_EQUAL(WEECHAT_RC_OK,
                script_repo_file_update_url_cb ((void *)1, NULL, "url",
                                                NULL, output));
    LONGS_EQUAL(3, script_repo_count);
    script_repo_remove_all ();

    /* No error: file is read. */
    hashtable_remove (output, "error");
    LONGS_EQUAL(WEECHAT_RC_OK,
                script_repo_file_update_url_cb ((void *)1, NULL, "url",
                                                NULL, output));
    LONGS_EQUAL(3, script_repo_count);

    hashtable_free (output);
}
