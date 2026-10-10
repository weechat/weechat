/*
 * SPDX-FileCopyrightText: 2021-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test configuration file functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"
#include "tests-record.h"

extern "C"
{
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "src/core/weechat.h"
#include "src/core/core-arraylist.h"
#include "src/core/core-config-file.h"
#include "src/core/core-config.h"
#include "src/core/core-dir.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hdata.h"
#include "src/core/core-hook.h"
#include "src/core/core-infolist.h"
#include "src/core/core-secure-config.h"
#include "src/core/core-string.h"
#include "src/gui/gui-color.h"
#include "src/plugins/plugin.h"
#include "src/plugins/plugin-config.h"

extern struct t_config_file *config_file_find_pos (const char *name);
extern void config_file_config_insert (struct t_config_file *config_file);
extern struct t_config_section *config_file_section_find_pos (struct t_config_file *config_file,
                                                              const char *name);
extern char *config_file_option_full_name (struct t_config_option *option);
extern void config_file_hook_config_exec (struct t_config_option *option);
extern struct t_config_option *config_file_option_find_pos (struct t_config_section *section,
                                                            const char *name);
extern void config_file_option_insert_in_section (struct t_config_option *option);
extern struct t_config_option *config_file_option_malloc (void);
extern struct t_config_option *config_file_get_parent_option (struct t_config_option *option);
extern int config_file_string_boolean_is_valid (const char *text);
extern const char *config_file_option_escape (const char *name);
extern int config_file_write_internal (struct t_config_file *config_file,
                                       int default_options);
extern int config_file_parse_version (const char *version);
extern void config_file_backup (const char *filename);
extern void config_file_update_data_read (struct t_config_file *config_file,
                                          const char *filename,
                                          const char *section,
                                          const char *option,
                                          const char *value,
                                          char **ret_section,
                                          char **ret_option,
                                          char **ret_value,
                                          int *warning_update_displayed);
extern int config_file_read_internal (struct t_config_file *config_file,
                                      int reload);
extern void config_file_option_free_data (struct t_config_option *option);
extern int config_file_add_option_to_infolist (struct t_infolist *infolist,
                                               struct t_config_file *config_file,
                                               struct t_config_section *section,
                                               struct t_config_option *option,
                                               const char *option_name);
}

#define TEST_CONFIG_NAME "test_cfg"

#define TEST_CHECK_MSG_REGEX(__regex)                                   \
    if (!record_search_msg_regex ("core.weechat", __regex))             \
    {                                                                   \
        char **msg = string_dyn_alloc (256);                            \
        string_dyn_concat (msg, "Message not displayed: ", -1);         \
        string_dyn_concat (msg, __regex, -1);                           \
        string_dyn_concat (msg, "\nAll messages displayed:\n", -1);     \
        record_dump (msg);                                              \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

struct t_config_file *config_test = NULL;
struct t_config_section *section_test = NULL;

int test_reload_cb_count = 0;
int test_section_read_cb_count = 0;
char *test_section_read_cb_option = NULL;
char *test_section_read_cb_value = NULL;
int test_section_delete_option_cb_count = 0;
int test_option_change_cb_count = 0;
int test_option_delete_cb_count = 0;
int test_update_cb_count = 0;
int test_hook_config_cb_count = 0;
char *test_hook_config_cb_option = NULL;
char *test_hook_config_cb_value = NULL;

struct t_config_option *ptr_option_bool = NULL;
struct t_config_option *ptr_option_bool_child = NULL;
struct t_config_option *ptr_option_int = NULL;
struct t_config_option *ptr_option_int_child = NULL;
struct t_config_option *ptr_option_int_str = NULL;
struct t_config_option *ptr_option_int_str_child = NULL;
struct t_config_option *ptr_option_str = NULL;
struct t_config_option *ptr_option_str_child = NULL;
struct t_config_option *ptr_option_col = NULL;
struct t_config_option *ptr_option_col_child = NULL;
struct t_config_option *ptr_option_enum = NULL;
struct t_config_option *ptr_option_enum_child = NULL;

TEST_GROUP(CoreConfigFile)
{
};

TEST_GROUP(CoreConfigFileWithNewOptions)
{
    static int option_str_check_cb (const void *pointer,
                                    void *data,
                                    struct t_config_option *option,
                                    const char *value)
    {
        (void) pointer;
        (void) data;
        (void) option;

        return ((strcmp (value, "xxx") == 0) || (strcmp (value, "zzz") == 0)) ?
            0 : 1;
    }

    void setup ()
    {
        ptr_option_bool = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_boolean", "boolean", "", NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_bool_child = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_boolean_child << weechat.look.test_boolean",
            "boolean", "", NULL, 0, 0, NULL, NULL, 1,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_int = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_integer", "integer", "", NULL, 0, 123456, "100", NULL, 0,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_int_child = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_integer_child << weechat.look.test_integer",
            "integer", "", NULL, 0, 123456, NULL, NULL, 1,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        /* Auto-created as enum with WeeChat >= 4.1.0 */
        ptr_option_int_str = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_integer_values", "integer", "", "v1|v2|v3", 0, 0, "v2", NULL, 0,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_int_str_child = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_integer_values_child << weechat.look.test_integer_values",
            "integer", "", "v1|v2|v3", 0, 0, NULL, NULL, 1,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_str = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_string", "string", "", NULL, 0, 0, "value", NULL, 0,
            &option_str_check_cb, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_str_child = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_string_child << weechat.look.test_string",
            "string", "", NULL, 0, 0, NULL, NULL, 1,
            &option_str_check_cb, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_col = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "test_color", "color", "", NULL, 0, 0, "blue", NULL, 0,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_col_child = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "test_color_child << weechat.color.test_color",
            "color", "", NULL, 0, 0, NULL, NULL, 1,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_enum = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_enum", "enum", "", "v1|v2|v3", 0, 0, "v2", NULL, 0,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_enum_child = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_enum_child << weechat.look.test_enum",
            "enum", "", "v1|v2|v3", 0, 0, NULL, NULL, 1,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
    }

    void teardown ()
    {
        config_file_option_free (ptr_option_bool, 0);
        ptr_option_bool = NULL;
        config_file_option_free (ptr_option_bool_child, 0);
        ptr_option_bool_child = NULL;
        config_file_option_free (ptr_option_int, 0);
        ptr_option_int = NULL;
        config_file_option_free (ptr_option_int_child, 0);
        ptr_option_int_child = NULL;
        config_file_option_free (ptr_option_int_str, 0);
        ptr_option_int_str = NULL;
        config_file_option_free (ptr_option_int_str_child, 0);
        ptr_option_int_str_child = NULL;
        config_file_option_free (ptr_option_str, 0);
        ptr_option_str = NULL;
        config_file_option_free (ptr_option_str_child, 0);
        ptr_option_str_child = NULL;
        config_file_option_free (ptr_option_col, 0);
        ptr_option_col = NULL;
        config_file_option_free (ptr_option_col_child, 0);
        ptr_option_col_child = NULL;
        config_file_option_free (ptr_option_enum, 0);
        ptr_option_enum = NULL;
        config_file_option_free (ptr_option_enum_child, 0);
        ptr_option_enum_child = NULL;
    }
};

/*
 * Reset variables updated by test callbacks.
 */

void
test_config_reset_cb_vars (void)
{
    test_reload_cb_count = 0;
    test_section_read_cb_count = 0;
    free (test_section_read_cb_option);
    test_section_read_cb_option = NULL;
    free (test_section_read_cb_value);
    test_section_read_cb_value = NULL;
    test_section_delete_option_cb_count = 0;
    test_option_change_cb_count = 0;
    test_option_delete_cb_count = 0;
    test_update_cb_count = 0;
    test_hook_config_cb_count = 0;
    free (test_hook_config_cb_option);
    test_hook_config_cb_option = NULL;
    free (test_hook_config_cb_value);
    test_hook_config_cb_value = NULL;
}

/*
 * Callback for reloading a test configuration file.
 */

int
test_config_reload_cb (const void *pointer, void *data,
                       struct t_config_file *config_file)
{
    /* Make C++ compiler happy. */
    (void) pointer;
    (void) data;
    (void) config_file;

    test_reload_cb_count++;

    return WEECHAT_CONFIG_READ_OK;
}

/*
 * Callback for reading an option in a test section.
 */

int
test_section_read_cb (const void *pointer, void *data,
                      struct t_config_file *config_file,
                      struct t_config_section *section,
                      const char *option_name, const char *value)
{
    /* Make C++ compiler happy. */
    (void) pointer;
    (void) data;
    (void) config_file;
    (void) section;

    test_section_read_cb_count++;
    free (test_section_read_cb_option);
    test_section_read_cb_option = (option_name) ? strdup (option_name) : NULL;
    free (test_section_read_cb_value);
    test_section_read_cb_value = (value) ? strdup (value) : NULL;

    return WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
}

/*
 * Callback for writing a test section.
 *
 * If pointer is not NULL, an error is returned.
 */

int
test_section_write_cb (const void *pointer, void *data,
                       struct t_config_file *config_file,
                       const char *section_name)
{
    /* Make C++ compiler happy. */
    (void) data;

    if (pointer)
        return WEECHAT_CONFIG_WRITE_ERROR;

    if (!config_file_write_line (config_file, section_name, NULL))
        return WEECHAT_CONFIG_WRITE_ERROR;
    if (!config_file_write_line (config_file, "custom", "\"%s\"", "value"))
        return WEECHAT_CONFIG_WRITE_ERROR;

    return WEECHAT_CONFIG_WRITE_OK;
}

/*
 * Callback for writing default values of a test section.
 */

int
test_section_write_default_cb (const void *pointer, void *data,
                               struct t_config_file *config_file,
                               const char *section_name)
{
    /* Make C++ compiler happy. */
    (void) pointer;
    (void) data;

    if (!config_file_write_line (config_file, section_name, NULL))
        return WEECHAT_CONFIG_WRITE_ERROR;
    if (!config_file_write_line (config_file, "custom_default", "\"%s\"",
                                 "default"))
    {
        return WEECHAT_CONFIG_WRITE_ERROR;
    }

    return WEECHAT_CONFIG_WRITE_OK;
}

/*
 * Callback for creating an option in a test section (always a string).
 */

int
test_section_create_option_cb (const void *pointer, void *data,
                               struct t_config_file *config_file,
                               struct t_config_section *section,
                               const char *option_name, const char *value)
{
    struct t_config_option *ptr_option;

    /* Make C++ compiler happy. */
    (void) pointer;
    (void) data;

    ptr_option = config_file_new_option (
        config_file, section,
        option_name, "string", NULL, NULL, 0, 0, value, value, 1,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);

    return (ptr_option) ?
        WEECHAT_CONFIG_OPTION_SET_OK_CHANGED : WEECHAT_CONFIG_OPTION_SET_ERROR;
}

/*
 * Callback for deleting an option in a test section.
 */

int
test_section_delete_option_cb (const void *pointer, void *data,
                               struct t_config_file *config_file,
                               struct t_config_section *section,
                               struct t_config_option *option)
{
    /* Make C++ compiler happy. */
    (void) pointer;
    (void) data;
    (void) config_file;
    (void) section;

    test_section_delete_option_cb_count++;
    config_file_option_free (option, 1);

    return WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
}

/*
 * Callback for a change of value in a test option.
 */

void
test_option_change_cb (const void *pointer, void *data,
                       struct t_config_option *option)
{
    /* Make C++ compiler happy. */
    (void) pointer;
    (void) data;
    (void) option;

    test_option_change_cb_count++;
}

/*
 * Callback for deletion of a test option.
 */

void
test_option_delete_cb (const void *pointer, void *data,
                       struct t_config_option *option)
{
    /* Make C++ compiler happy. */
    (void) pointer;
    (void) data;
    (void) option;

    test_option_delete_cb_count++;
}

/*
 * Callback for updating data read in a test configuration file
 * (version 1 to 2):
 *   - section "old_sec" is renamed to "sec"
 *   - option "old_name" is renamed to "int"
 *   - option "drop" is ignored (empty name)
 *   - option "str_null" gets a null value
 *   - option "set_value" gets value "xyz" (in a new hashtable)
 */

struct t_hashtable *
test_config_update_cb (const void *pointer, void *data,
                       struct t_config_file *config_file,
                       int version_read,
                       struct t_hashtable *data_read)
{
    struct t_hashtable *hashtable;
    const char *ptr_section, *ptr_option;

    /* Make C++ compiler happy. */
    (void) pointer;
    (void) data;
    (void) config_file;

    test_update_cb_count++;

    if (version_read >= 2)
        return NULL;

    ptr_section = (const char *)hashtable_get (data_read, "section");
    ptr_option = (const char *)hashtable_get (data_read, "option");

    if (!ptr_option)
    {
        if (ptr_section && (strcmp (ptr_section, "old_sec") == 0))
        {
            hashtable_set (data_read, "section", "sec");
            return data_read;
        }
        return NULL;
    }

    if (strcmp (ptr_option, "old_name") == 0)
    {
        hashtable_set (data_read, "option", "int");
        return data_read;
    }
    if (strcmp (ptr_option, "drop") == 0)
    {
        hashtable_set (data_read, "option", "");
        return data_read;
    }
    if (strcmp (ptr_option, "str_null") == 0)
    {
        hashtable_set (data_read, "value_null", "1");
        return data_read;
    }
    if (strcmp (ptr_option, "set_value") == 0)
    {
        hashtable = hashtable_dup (data_read);
        hashtable_set (hashtable, "value", "xyz");
        return hashtable;
    }

    return NULL;
}

/*
 * Callback for hook_config.
 */

int
test_hook_config_cb (const void *pointer, void *data,
                     const char *option, const char *value)
{
    /* Make C++ compiler happy. */
    (void) pointer;
    (void) data;

    test_hook_config_cb_count++;
    free (test_hook_config_cb_option);
    test_hook_config_cb_option = (option) ? strdup (option) : NULL;
    free (test_hook_config_cb_value);
    test_hook_config_cb_value = (value) ? strdup (value) : NULL;

    return WEECHAT_RC_OK;
}

/*
 * Create a section without callbacks in a configuration file.
 */

struct t_config_section *
test_new_section (struct t_config_file *config_file, const char *name,
                  int user_can_add_options, int user_can_delete_options)
{
    return config_file_new_section (
        config_file, name, user_can_add_options, user_can_delete_options,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
}

/*
 * Create an option without callbacks in a section (integers are between
 * 0 and 1000).
 */

struct t_config_option *
test_new_option (struct t_config_section *section, const char *name,
                 const char *type, const char *string_values,
                 const char *default_value, const char *value,
                 int null_value_allowed)
{
    return config_file_new_option (
        section->config_file, section,
        name, type, "", string_values, 0, 1000, default_value, value,
        null_value_allowed,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
}

/*
 * Build path to a file in WeeChat config directory.
 *
 * Note: result must be freed after use.
 */

char *
test_config_get_path (const char *filename)
{
    char *path;

    string_asprintf (&path, "%s/%s", weechat_config_dir, filename);

    return path;
}

/*
 * Write a file in WeeChat config directory.
 */

void
test_config_write_file (const char *filename, const char *content)
{
    char *path;
    FILE *file;

    path = test_config_get_path (filename);
    file = fopen (path, "w");
    if (file)
    {
        fputs (content, file);
        fclose (file);
    }
    free (path);
}

/*
 * Read content of a file in WeeChat config directory.
 *
 * Note: result must be freed after use.
 */

char *
test_config_read_file (const char *filename)
{
    char *path, *content;

    path = test_config_get_path (filename);
    content = dir_file_get_content (path);
    free (path);

    return content;
}

/*
 * Remove a file in WeeChat config directory.
 */

void
test_config_remove_file (const char *filename)
{
    char *path;

    path = test_config_get_path (filename);
    unlink (path);
    free (path);
}

int test_backup_files_count = 0;

/*
 * Count and remove a backup file (callback of dir_exec_on_files).
 */

void
test_config_backup_file_cb (void *data, const char *filename)
{
    const char *prefix, *pos;

    prefix = (const char *)data;

    pos = strrchr (filename, '/');
    pos = (pos) ? pos + 1 : filename;
    if (strncmp (pos, prefix, strlen (prefix)) == 0)
    {
        test_backup_files_count++;
        unlink (filename);
    }
}

/*
 * Remove backup files of a file in WeeChat config directory.
 *
 * Return the number of backup files removed.
 */

int
test_config_remove_backup_files (const char *filename)
{
    char prefix[256];

    snprintf (prefix, sizeof (prefix), "%s.backup", filename);
    test_backup_files_count = 0;
    dir_exec_on_files (weechat_config_dir, 0, 1,
                       &test_config_backup_file_cb, prefix);

    return test_backup_files_count;
}

TEST_GROUP(CoreConfigFileWithTestConfig)
{
    void setup ()
    {
        test_config_reset_cb_vars ();
        config_test = config_file_new (NULL, TEST_CONFIG_NAME,
                                       NULL, NULL, NULL);
        section_test = test_new_section (config_test, "sec", 0, 0);
    }

    void teardown ()
    {
        config_file_free (config_test);
        config_test = NULL;
        section_test = NULL;
        test_config_remove_file (TEST_CONFIG_NAME ".conf");
        test_config_remove_backup_files (TEST_CONFIG_NAME ".conf");
        test_config_reset_cb_vars ();
    }
};

/*
 * Test functions:
 *   config_file_valid
 */

TEST(CoreConfigFile, Valid)
{
    LONGS_EQUAL(0, config_file_valid (NULL));
    LONGS_EQUAL(0, config_file_valid ((struct t_config_file *)0x1));

    LONGS_EQUAL(1, config_file_valid (config_file_search ("weechat")));
    LONGS_EQUAL(1, config_file_valid (config_file_search ("sec")));
}

/*
 * Test functions:
 *   config_file_search
 */

TEST(CoreConfigFile, Search)
{
    POINTERS_EQUAL(NULL, config_file_search (NULL));
    POINTERS_EQUAL(NULL, config_file_search (""));
    POINTERS_EQUAL(NULL, config_file_search ("zzz"));

    POINTERS_EQUAL(weechat_config_file, config_file_search ("weechat"));
    POINTERS_EQUAL(secure_config_file, config_file_search ("sec"));
}

/*
 * Test functions:
 *   config_file_find_pos
 */

TEST(CoreConfigFile, FindPos)
{
    POINTERS_EQUAL(NULL, config_file_find_pos (NULL));
    POINTERS_EQUAL(config_files, config_file_find_pos (""));
    POINTERS_EQUAL(weechat_config_file->next_config, config_file_find_pos ("weechat2"));
    POINTERS_EQUAL(config_files, config_file_find_pos ("WEECHAT2"));
}

/*
 * Test functions:
 *   config_file_config_insert
 */

TEST(CoreConfigFile, ConfigInsert)
{
    struct t_config_file *config_first, *config_middle, *config_last;
    struct t_config_file *old_first, *old_last, *old_next_weechat;

    old_first = config_files;
    old_last = last_config_file;
    old_next_weechat = weechat_config_file->next_config;

    /* Insert of NULL config does nothing. */
    config_file_config_insert (NULL);
    POINTERS_EQUAL(old_first, config_files);
    POINTERS_EQUAL(old_last, last_config_file);

    /* Insert at the end of list. */
    config_last = config_file_new (NULL, "zzz_test", NULL, NULL, NULL);
    CHECK(config_last);
    POINTERS_EQUAL(config_last, last_config_file);
    POINTERS_EQUAL(old_last, config_last->prev_config);
    POINTERS_EQUAL(NULL, config_last->next_config);
    POINTERS_EQUAL(config_last, old_last->next_config);

    /* Insert at the beginning of list. */
    config_first = config_file_new (NULL, "AAA_test", NULL, NULL, NULL);
    CHECK(config_first);
    POINTERS_EQUAL(config_first, config_files);
    POINTERS_EQUAL(NULL, config_first->prev_config);
    POINTERS_EQUAL(old_first, config_first->next_config);
    POINTERS_EQUAL(config_first, old_first->prev_config);

    /* Insert in the middle of list. */
    config_middle = config_file_new (NULL, "weechat2", NULL, NULL, NULL);
    CHECK(config_middle);
    POINTERS_EQUAL(weechat_config_file, config_middle->prev_config);
    POINTERS_EQUAL(old_next_weechat, config_middle->next_config);
    POINTERS_EQUAL(config_middle, weechat_config_file->next_config);
    POINTERS_EQUAL(config_middle, old_next_weechat->prev_config);

    config_file_free (config_first);
    config_file_free (config_middle);
    config_file_free (config_last);

    POINTERS_EQUAL(old_first, config_files);
    POINTERS_EQUAL(old_last, last_config_file);
    POINTERS_EQUAL(old_next_weechat, weechat_config_file->next_config);
}

/*
 * Test functions:
 *   config_file_new
 */

TEST(CoreConfigFile, New)
{
    struct t_config_file *config;

    POINTERS_EQUAL(NULL, config_file_new (NULL, NULL, NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, config_file_new (NULL, "", NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, config_file_new (NULL, "500|", NULL, NULL, NULL));

    /* Two configuration files cannot have same name. */
    POINTERS_EQUAL(NULL, config_file_new (NULL, "weechat", NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, config_file_new (NULL, "500|weechat",
                                          NULL, NULL, NULL));

    /* Default priority */
    config = config_file_new (NULL, "test_new", &test_config_reload_cb,
                              (void *)0x123, NULL);
    CHECK(config);
    POINTERS_EQUAL(config, config_file_search ("test_new"));
    POINTERS_EQUAL(NULL, config->plugin);
    LONGS_EQUAL(CONFIG_PRIORITY_DEFAULT, config->priority);
    STRCMP_EQUAL("test_new", config->name);
    STRCMP_EQUAL("test_new.conf", config->filename);
    POINTERS_EQUAL(NULL, config->file);
    LONGS_EQUAL(1, config->version);
    POINTERS_EQUAL(NULL, config->callback_update);
    POINTERS_EQUAL(NULL, config->callback_update_pointer);
    POINTERS_EQUAL(NULL, config->callback_update_data);
    POINTERS_EQUAL(&test_config_reload_cb, config->callback_reload);
    POINTERS_EQUAL(0x123, config->callback_reload_pointer);
    POINTERS_EQUAL(NULL, config->callback_reload_data);
    POINTERS_EQUAL(NULL, config->sections);
    POINTERS_EQUAL(NULL, config->last_section);
    config_file_free (config);
    POINTERS_EQUAL(NULL, config_file_search ("test_new"));

    /* Custom priority */
    config = config_file_new (NULL, "123|test_new", NULL, NULL, NULL);
    CHECK(config);
    POINTERS_EQUAL(config, config_file_search ("test_new"));
    LONGS_EQUAL(123, config->priority);
    STRCMP_EQUAL("test_new", config->name);
    STRCMP_EQUAL("test_new.conf", config->filename);
    POINTERS_EQUAL(NULL, config->callback_reload);
    config_file_free (config);
}

/*
 * Test functions:
 *   config_file_set_version
 */

TEST(CoreConfigFileWithTestConfig, SetVersion)
{
    LONGS_EQUAL(1, config_test->version);

    LONGS_EQUAL(0, config_file_set_version (NULL, 2, NULL, NULL, NULL));
    LONGS_EQUAL(0, config_file_set_version (config_test, -1, NULL, NULL, NULL));
    LONGS_EQUAL(0, config_file_set_version (config_test, 0, NULL, NULL, NULL));
    LONGS_EQUAL(1, config_test->version);
    POINTERS_EQUAL(NULL, config_test->callback_update);

    LONGS_EQUAL(1, config_file_set_version (config_test, 3,
                                            &test_config_update_cb,
                                            (void *)0x123, NULL));
    LONGS_EQUAL(3, config_test->version);
    POINTERS_EQUAL(&test_config_update_cb, config_test->callback_update);
    POINTERS_EQUAL(0x123, config_test->callback_update_pointer);
    POINTERS_EQUAL(NULL, config_test->callback_update_data);

    LONGS_EQUAL(1, config_file_set_version (config_test, 2, NULL, NULL, NULL));
    LONGS_EQUAL(2, config_test->version);
    POINTERS_EQUAL(NULL, config_test->callback_update);
    POINTERS_EQUAL(NULL, config_test->callback_update_pointer);
}

/*
 * Test functions:
 *   config_file_arraylist_cmp_config_cb
 *   config_file_get_configs_by_priority
 */

TEST(CoreConfigFile, GetConfigsByPriority)
{
    struct t_config_file *ptr_config;
    struct t_arraylist *all_configs;
    int config_count;

    /* Count number of configuration files. */
    config_count = 0;
    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        config_count++;
    }

    /* Get list of configuration files by priority (highest to lowest). */
    all_configs = config_file_get_configs_by_priority ();
    CHECK(all_configs);

    /* Ensure we have all files in the list (and not more). */
    LONGS_EQUAL(config_count, arraylist_size (all_configs));

    /* Check core configuration files (they have higher priority). */
    POINTERS_EQUAL(secure_config_file, arraylist_get (all_configs, 0));
    POINTERS_EQUAL(weechat_config_file, arraylist_get (all_configs, 1));
    POINTERS_EQUAL(plugin_config_file, arraylist_get (all_configs, 2));

    /* Check first plugin configuration file (with highest priority). */
    ptr_config = (struct t_config_file *)arraylist_get (all_configs, 3);
    CHECK(ptr_config);
    STRCMP_EQUAL("charset", ptr_config->name);

    /* Check last plugin configuration file (with lowest priority). */
    ptr_config = (struct t_config_file *)arraylist_get (all_configs,
                                                        config_count - 1);
    CHECK(ptr_config);
    STRCMP_EQUAL("fset", ptr_config->name);

    arraylist_free (all_configs);
}

/*
 * Test functions:
 *   config_file_section_find_pos
 */

TEST(CoreConfigFileWithTestConfig, SectionFindPos)
{
    struct t_config_section *section_zzz;

    section_zzz = test_new_section (config_test, "zzz", 0, 0);
    CHECK(section_zzz);

    POINTERS_EQUAL(NULL, config_file_section_find_pos (NULL, NULL));
    POINTERS_EQUAL(NULL, config_file_section_find_pos (NULL, "sec"));
    POINTERS_EQUAL(NULL, config_file_section_find_pos (config_test, NULL));

    POINTERS_EQUAL(section_test, config_file_section_find_pos (config_test, ""));
    POINTERS_EQUAL(section_test, config_file_section_find_pos (config_test, "aaa"));
    POINTERS_EQUAL(section_zzz, config_file_section_find_pos (config_test, "sec"));
    POINTERS_EQUAL(section_zzz, config_file_section_find_pos (config_test, "xxx"));
    POINTERS_EQUAL(NULL, config_file_section_find_pos (config_test, "zzz"));
    POINTERS_EQUAL(NULL, config_file_section_find_pos (config_test, "zzzz"));
}

/*
 * Test functions:
 *   config_file_new_section
 */

TEST(CoreConfigFileWithTestConfig, NewSection)
{
    struct t_config_section *section;

    POINTERS_EQUAL(NULL, test_new_section (NULL, "sec2", 0, 0));
    POINTERS_EQUAL(NULL, test_new_section (config_test, NULL, 0, 0));

    /* Two sections cannot have same name. */
    POINTERS_EQUAL(NULL, test_new_section (config_test, "sec", 0, 0));

    section = config_file_new_section (
        config_test, "sec2", 1, 1,
        &test_section_read_cb, (void *)0x1, NULL,
        &test_section_write_cb, (void *)0x2, NULL,
        &test_section_write_default_cb, (void *)0x3, NULL,
        &test_section_create_option_cb, (void *)0x4, NULL,
        &test_section_delete_option_cb, (void *)0x5, NULL);
    CHECK(section);
    POINTERS_EQUAL(section, config_file_search_section (config_test, "sec2"));
    POINTERS_EQUAL(config_test, section->config_file);
    STRCMP_EQUAL("sec2", section->name);
    LONGS_EQUAL(1, section->user_can_add_options);
    LONGS_EQUAL(1, section->user_can_delete_options);
    POINTERS_EQUAL(&test_section_read_cb, section->callback_read);
    POINTERS_EQUAL(0x1, section->callback_read_pointer);
    POINTERS_EQUAL(NULL, section->callback_read_data);
    POINTERS_EQUAL(&test_section_write_cb, section->callback_write);
    POINTERS_EQUAL(0x2, section->callback_write_pointer);
    POINTERS_EQUAL(NULL, section->callback_write_data);
    POINTERS_EQUAL(&test_section_write_default_cb,
                   section->callback_write_default);
    POINTERS_EQUAL(0x3, section->callback_write_default_pointer);
    POINTERS_EQUAL(NULL, section->callback_write_default_data);
    POINTERS_EQUAL(&test_section_create_option_cb,
                   section->callback_create_option);
    POINTERS_EQUAL(0x4, section->callback_create_option_pointer);
    POINTERS_EQUAL(NULL, section->callback_create_option_data);
    POINTERS_EQUAL(&test_section_delete_option_cb,
                   section->callback_delete_option);
    POINTERS_EQUAL(0x5, section->callback_delete_option_pointer);
    POINTERS_EQUAL(NULL, section->callback_delete_option_data);
    POINTERS_EQUAL(NULL, section->options);
    POINTERS_EQUAL(NULL, section->last_option);

    /* Section is added at the end of list. */
    POINTERS_EQUAL(section_test, config_test->sections);
    POINTERS_EQUAL(section, config_test->last_section);
    POINTERS_EQUAL(section_test, section->prev_section);
    POINTERS_EQUAL(NULL, section->next_section);
    POINTERS_EQUAL(section, section_test->next_section);
}

/*
 * Test functions:
 *   config_file_search_section
 */

TEST(CoreConfigFile, SearchSection)
{
    POINTERS_EQUAL(NULL, config_file_search_section (NULL, NULL));
    POINTERS_EQUAL(NULL, config_file_search_section (weechat_config_file, NULL));
    POINTERS_EQUAL(NULL, config_file_search_section (weechat_config_file, "zzz"));

    POINTERS_EQUAL(weechat_config_section_proxy,
                   config_file_search_section (weechat_config_file, "proxy"));
}

/*
 * Test functions:
 *   config_file_option_full_name
 */

TEST(CoreConfigFile, OptionFullName)
{
    char *str;

    STRCMP_EQUAL(NULL, config_file_option_full_name (NULL));

    WEE_TEST_STR("weechat.look.buffer_time_format",
                 config_file_option_full_name (config_look_buffer_time_format));
}

/*
 * Test functions:
 *   config_file_hook_config_exec
 */

TEST(CoreConfigFileWithTestConfig, HookConfigExec)
{
    struct t_config_option *opt_bool, *opt_int, *opt_str, *opt_str_null;
    struct t_config_option *opt_col, *opt_enum, *opt_no_section;
    struct t_hook *hook;

    opt_bool = test_new_option (section_test, "bool", "boolean", NULL,
                                "on", NULL, 0);
    opt_int = test_new_option (section_test, "int", "integer", NULL,
                               "42", NULL, 0);
    opt_str = test_new_option (section_test, "str", "string", NULL,
                               "value", NULL, 0);
    opt_str_null = test_new_option (section_test, "str_null", "string", NULL,
                                    NULL, NULL, 1);
    opt_col = test_new_option (section_test, "col", "color", NULL,
                               "red", NULL, 0);
    opt_enum = test_new_option (section_test, "enum", "enum", "v1|v2|v3",
                                "v2", NULL, 0);
    opt_no_section = config_file_new_option (
        NULL, NULL, "no_section", "string", "", NULL, 0, 0, "value", NULL, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    CHECK(opt_bool);
    CHECK(opt_int);
    CHECK(opt_str);
    CHECK(opt_str_null);
    CHECK(opt_col);
    CHECK(opt_enum);
    CHECK(opt_no_section);

    hook = hook_config (NULL, TEST_CONFIG_NAME ".sec.*",
                        &test_hook_config_cb, NULL, NULL);
    CHECK(hook);

    config_file_hook_config_exec (NULL);
    LONGS_EQUAL(0, test_hook_config_cb_count);

    /* Option without section: hooks are not called. */
    config_file_hook_config_exec (opt_no_section);
    LONGS_EQUAL(0, test_hook_config_cb_count);

    config_file_hook_config_exec (opt_bool);
    LONGS_EQUAL(1, test_hook_config_cb_count);
    STRCMP_EQUAL(TEST_CONFIG_NAME ".sec.bool", test_hook_config_cb_option);
    STRCMP_EQUAL("on", test_hook_config_cb_value);

    config_file_hook_config_exec (opt_int);
    LONGS_EQUAL(2, test_hook_config_cb_count);
    STRCMP_EQUAL(TEST_CONFIG_NAME ".sec.int", test_hook_config_cb_option);
    STRCMP_EQUAL("42", test_hook_config_cb_value);

    config_file_hook_config_exec (opt_str);
    LONGS_EQUAL(3, test_hook_config_cb_count);
    STRCMP_EQUAL(TEST_CONFIG_NAME ".sec.str", test_hook_config_cb_option);
    STRCMP_EQUAL("value", test_hook_config_cb_value);

    config_file_hook_config_exec (opt_str_null);
    LONGS_EQUAL(4, test_hook_config_cb_count);
    STRCMP_EQUAL(TEST_CONFIG_NAME ".sec.str_null", test_hook_config_cb_option);
    POINTERS_EQUAL(NULL, test_hook_config_cb_value);

    config_file_hook_config_exec (opt_col);
    LONGS_EQUAL(5, test_hook_config_cb_count);
    STRCMP_EQUAL(TEST_CONFIG_NAME ".sec.col", test_hook_config_cb_option);
    STRCMP_EQUAL("red", test_hook_config_cb_value);

    config_file_hook_config_exec (opt_enum);
    LONGS_EQUAL(6, test_hook_config_cb_count);
    STRCMP_EQUAL(TEST_CONFIG_NAME ".sec.enum", test_hook_config_cb_option);
    STRCMP_EQUAL("v2", test_hook_config_cb_value);

    unhook (hook);

    config_file_option_free (opt_no_section, 0);
}

/*
 * Test functions:
 *   config_file_option_find_pos
 */

TEST(CoreConfigFileWithTestConfig, OptionFindPos)
{
    struct t_config_option *opt_b, *opt_d;

    POINTERS_EQUAL(NULL, config_file_option_find_pos (NULL, NULL));
    POINTERS_EQUAL(NULL, config_file_option_find_pos (NULL, "a"));
    POINTERS_EQUAL(NULL, config_file_option_find_pos (section_test, NULL));

    /* Empty section */
    POINTERS_EQUAL(NULL, config_file_option_find_pos (section_test, "a"));

    opt_b = test_new_option (section_test, "b", "string", NULL, "", NULL, 0);
    opt_d = test_new_option (section_test, "d", "string", NULL, "", NULL, 0);
    CHECK(opt_b);
    CHECK(opt_d);

    POINTERS_EQUAL(opt_b, config_file_option_find_pos (section_test, ""));
    POINTERS_EQUAL(opt_b, config_file_option_find_pos (section_test, "a"));
    POINTERS_EQUAL(opt_d, config_file_option_find_pos (section_test, "b"));
    POINTERS_EQUAL(opt_d, config_file_option_find_pos (section_test, "c"));
    POINTERS_EQUAL(NULL, config_file_option_find_pos (section_test, "d"));
    POINTERS_EQUAL(NULL, config_file_option_find_pos (section_test, "e"));
}

/*
 * Test functions:
 *   config_file_option_insert_in_section
 */

TEST(CoreConfigFileWithTestConfig, OptionInsertInSection)
{
    struct t_config_option *opt_a, *opt_m, *opt_z, *opt_no_section;

    config_file_option_insert_in_section (NULL);

    /* Option without section: nothing is done. */
    opt_no_section = config_file_option_malloc ();
    CHECK(opt_no_section);
    opt_no_section->name = strdup ("test");
    config_file_option_insert_in_section (opt_no_section);
    POINTERS_EQUAL(NULL, opt_no_section->prev_option);
    POINTERS_EQUAL(NULL, opt_no_section->next_option);
    config_file_option_free (opt_no_section, 0);

    opt_a = config_file_option_malloc ();
    opt_m = config_file_option_malloc ();
    opt_z = config_file_option_malloc ();
    CHECK(opt_a);
    CHECK(opt_m);
    CHECK(opt_z);
    opt_a->config_file = config_test;
    opt_a->section = section_test;
    opt_a->name = strdup ("a");
    opt_m->config_file = config_test;
    opt_m->section = section_test;
    opt_m->name = strdup ("m");
    opt_z->config_file = config_test;
    opt_z->section = section_test;
    opt_z->name = strdup ("z");

    /* First option in section */
    config_file_option_insert_in_section (opt_m);
    POINTERS_EQUAL(opt_m, section_test->options);
    POINTERS_EQUAL(opt_m, section_test->last_option);
    POINTERS_EQUAL(NULL, opt_m->prev_option);
    POINTERS_EQUAL(NULL, opt_m->next_option);

    /* Insert at the end of section */
    config_file_option_insert_in_section (opt_z);
    POINTERS_EQUAL(opt_m, section_test->options);
    POINTERS_EQUAL(opt_z, section_test->last_option);
    POINTERS_EQUAL(opt_m, opt_z->prev_option);
    POINTERS_EQUAL(NULL, opt_z->next_option);
    POINTERS_EQUAL(opt_z, opt_m->next_option);

    /* Insert at the beginning of section */
    config_file_option_insert_in_section (opt_a);
    POINTERS_EQUAL(opt_a, section_test->options);
    POINTERS_EQUAL(opt_z, section_test->last_option);
    POINTERS_EQUAL(NULL, opt_a->prev_option);
    POINTERS_EQUAL(opt_m, opt_a->next_option);
    POINTERS_EQUAL(opt_a, opt_m->prev_option);
}

/*
 * Test functions:
 *   config_file_option_malloc
 */

TEST(CoreConfigFile, OptionMalloc)
{
    struct t_config_option *option;

    option = config_file_option_malloc ();
    CHECK(option);
    POINTERS_EQUAL(NULL, option->config_file);
    POINTERS_EQUAL(NULL, option->section);
    POINTERS_EQUAL(NULL, option->name);
    POINTERS_EQUAL(NULL, option->parent_name);
    LONGS_EQUAL(0, option->type);
    LONGS_EQUAL(0, option->themable);
    POINTERS_EQUAL(NULL, option->description);
    POINTERS_EQUAL(NULL, option->string_values);
    LONGS_EQUAL(0, option->min);
    LONGS_EQUAL(0, option->max);
    POINTERS_EQUAL(NULL, option->default_value);
    POINTERS_EQUAL(NULL, option->value);
    LONGS_EQUAL(0, option->null_value_allowed);
    POINTERS_EQUAL(NULL, option->callback_check_value);
    POINTERS_EQUAL(NULL, option->callback_check_value_pointer);
    POINTERS_EQUAL(NULL, option->callback_check_value_data);
    POINTERS_EQUAL(NULL, option->callback_change);
    POINTERS_EQUAL(NULL, option->callback_change_pointer);
    POINTERS_EQUAL(NULL, option->callback_change_data);
    POINTERS_EQUAL(NULL, option->callback_delete);
    POINTERS_EQUAL(NULL, option->callback_delete_pointer);
    POINTERS_EQUAL(NULL, option->callback_delete_data);
    LONGS_EQUAL(0, option->loaded);
    POINTERS_EQUAL(NULL, option->prev_option);
    POINTERS_EQUAL(NULL, option->next_option);
    free (option);
}

/*
 * Create an option with the given type, read its "themable" flag, then free
 * it.
 *
 * Return the value of the "themable" flag (0 or 1), or -1 if the option could
 * not be created (invalid type).
 */

int
test_new_option_themable (struct t_config_section *section,
                          const char *name, const char *type,
                          const char *string_values,
                          const char *default_value)
{
    struct t_config_option *option;
    int themable;

    option = config_file_new_option (
        weechat_config_file, section,
        name, type, "", string_values, 0, 123456, default_value, NULL, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (!option)
        return -1;
    themable = option->themable;
    config_file_option_free (option, 0);
    return themable;
}

/*
 * Test functions:
 *   config_file_new_option
 */

TEST(CoreConfigFile, NewOption)
{
    struct t_config_option *ptr_option;
    int *ptr_themable;

    /* Plain types are not themable. */
    LONGS_EQUAL(0, test_new_option_themable (
                    weechat_config_section_look,
                    "test_themable", "boolean", NULL, "off"));
    LONGS_EQUAL(0, test_new_option_themable (
                    weechat_config_section_look,
                    "test_themable", "integer", NULL, "100"));
    LONGS_EQUAL(0, test_new_option_themable (
                    weechat_config_section_look,
                    "test_themable", "string", NULL, "value"));
    LONGS_EQUAL(0, test_new_option_themable (
                    weechat_config_section_look,
                    "test_themable", "enum", "v1|v2|v3", "v2"));

    /* Color options are always themable, even without the suffix. */
    LONGS_EQUAL(1, test_new_option_themable (
                    weechat_config_section_color,
                    "test_themable", "color", NULL, "blue"));

    /* The "|themable" suffix marks an option of any type as themable. */
    LONGS_EQUAL(1, test_new_option_themable (
                    weechat_config_section_look,
                    "test_themable", "boolean|themable", NULL, "off"));
    LONGS_EQUAL(1, test_new_option_themable (
                    weechat_config_section_look,
                    "test_themable", "integer|themable", NULL, "100"));
    LONGS_EQUAL(1, test_new_option_themable (
                    weechat_config_section_look,
                    "test_themable", "string|themable", NULL, "value"));
    LONGS_EQUAL(1, test_new_option_themable (
                    weechat_config_section_look,
                    "test_themable", "enum|themable", "v1|v2|v3", "v2"));
    LONGS_EQUAL(1, test_new_option_themable (
                    weechat_config_section_color,
                    "test_themable", "color|themable", NULL, "blue"));

    /* An invalid type or unknown suffix is refused (option not created). */
    LONGS_EQUAL(-1, test_new_option_themable (
                    weechat_config_section_look,
                    "test_themable", "string|xxx", NULL, "value"));
    LONGS_EQUAL(-1, test_new_option_themable (
                    weechat_config_section_look,
                    "test_themable", "string|", NULL, "value"));
    LONGS_EQUAL(-1, test_new_option_themable (
                    weechat_config_section_look,
                    "test_themable", "xxx", NULL, "value"));

    /* The flag is reachable via config_file_option_get_pointer. */
    ptr_option = config_file_new_option (
        weechat_config_file, weechat_config_section_look,
        "test_themable", "string|themable", "", NULL, 0, 0, "value", NULL, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    CHECK(ptr_option);
    ptr_themable = (int *)config_file_option_get_pointer (ptr_option,
                                                          "themable");
    CHECK(ptr_themable);
    POINTERS_EQUAL(&ptr_option->themable, ptr_themable);
    LONGS_EQUAL(1, *ptr_themable);
    config_file_option_free (ptr_option, 0);
}

/*
 * Test functions:
 *   config_file_new_option (values)
 */

TEST(CoreConfigFileWithTestConfig, NewOptionValues)
{
    struct t_config_option *option;

    /* Missing name or type */
    POINTERS_EQUAL(NULL, test_new_option (section_test, NULL, "string", NULL,
                                          "value", NULL, 0));
    POINTERS_EQUAL(NULL, test_new_option (section_test, "opt", NULL, NULL,
                                          "value", NULL, 0));

    /* Two options cannot have same name in a section. */
    option = test_new_option (section_test, "opt", "string", NULL,
                              "value", NULL, 0);
    CHECK(option);
    POINTERS_EQUAL(NULL, test_new_option (section_test, "opt", "string", NULL,
                                          "value", NULL, 0));
    config_file_option_free (option, 0);

    /* Enum without string values */
    POINTERS_EQUAL(NULL, test_new_option (section_test, "opt", "enum", NULL,
                                          "v1", NULL, 0));
    POINTERS_EQUAL(NULL, test_new_option (section_test, "opt", "enum", "",
                                          "v1", NULL, 0));

    /* No value and null value not allowed */
    POINTERS_EQUAL(NULL, test_new_option (section_test, "opt", "string", NULL,
                                          NULL, NULL, 0));

    /* Default value used as value */
    option = test_new_option (section_test, "opt", "string", NULL,
                              "default", NULL, 0);
    CHECK(option);
    STRCMP_EQUAL("default", CONFIG_STRING_DEFAULT(option));
    STRCMP_EQUAL("default", CONFIG_STRING(option));
    config_file_option_free (option, 0);

    /* Value used as default value */
    option = test_new_option (section_test, "opt", "string", NULL,
                              NULL, "value", 0);
    CHECK(option);
    STRCMP_EQUAL("value", CONFIG_STRING_DEFAULT(option));
    STRCMP_EQUAL("value", CONFIG_STRING(option));
    config_file_option_free (option, 0);

    /* Null value allowed */
    option = test_new_option (section_test, "opt", "string", NULL,
                              NULL, NULL, 1);
    CHECK(option);
    POINTERS_EQUAL(NULL, option->default_value);
    POINTERS_EQUAL(NULL, option->value);
    LONGS_EQUAL(1, option->null_value_allowed);
    config_file_option_free (option, 0);

    /* Option with all fields */
    option = config_file_new_option (
        config_test, section_test,
        "opt << " TEST_CONFIG_NAME ".sec.parent", "string", "description",
        NULL, 0, 100, "default", "value", 1,
        NULL, (void *)0x1, NULL,
        &test_option_change_cb, (void *)0x2, NULL,
        &test_option_delete_cb, (void *)0x3, NULL);
    CHECK(option);
    POINTERS_EQUAL(option, config_file_search_option (config_test,
                                                      section_test, "opt"));
    POINTERS_EQUAL(config_test, option->config_file);
    POINTERS_EQUAL(section_test, option->section);
    STRCMP_EQUAL("opt", option->name);
    STRCMP_EQUAL(TEST_CONFIG_NAME ".sec.parent", option->parent_name);
    LONGS_EQUAL(CONFIG_OPTION_TYPE_STRING, option->type);
    LONGS_EQUAL(0, option->themable);
    STRCMP_EQUAL("description", option->description);
    POINTERS_EQUAL(NULL, option->string_values);
    LONGS_EQUAL(0, option->min);
    LONGS_EQUAL(100, option->max);
    STRCMP_EQUAL("default", CONFIG_STRING_DEFAULT(option));
    STRCMP_EQUAL("value", CONFIG_STRING(option));
    LONGS_EQUAL(1, option->null_value_allowed);
    POINTERS_EQUAL(NULL, option->callback_check_value);
    POINTERS_EQUAL(0x1, option->callback_check_value_pointer);
    POINTERS_EQUAL(&test_option_change_cb, option->callback_change);
    POINTERS_EQUAL(0x2, option->callback_change_pointer);
    POINTERS_EQUAL(&test_option_delete_cb, option->callback_delete);
    POINTERS_EQUAL(0x3, option->callback_delete_pointer);
    LONGS_EQUAL(1, option->loaded);
    config_file_option_free (option, 0);

    /* Boolean */
    option = test_new_option (section_test, "opt", "boolean", NULL,
                              "on", "off", 0);
    CHECK(option);
    LONGS_EQUAL(CONFIG_OPTION_TYPE_BOOLEAN, option->type);
    LONGS_EQUAL(CONFIG_BOOLEAN_FALSE, option->min);
    LONGS_EQUAL(CONFIG_BOOLEAN_TRUE, option->max);
    LONGS_EQUAL(1, CONFIG_BOOLEAN_DEFAULT(option));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(option));
    config_file_option_free (option, 0);

    /* Integer: values are clamped to min/max, invalid value is 0 */
    option = test_new_option (section_test, "opt", "integer", NULL,
                              "5000", "-5", 0);
    CHECK(option);
    LONGS_EQUAL(CONFIG_OPTION_TYPE_INTEGER, option->type);
    LONGS_EQUAL(0, option->min);
    LONGS_EQUAL(1000, option->max);
    LONGS_EQUAL(1000, CONFIG_INTEGER_DEFAULT(option));
    LONGS_EQUAL(0, CONFIG_INTEGER(option));
    config_file_option_free (option, 0);
    option = test_new_option (section_test, "opt", "integer", NULL,
                              "abc", "123", 0);
    CHECK(option);
    LONGS_EQUAL(0, CONFIG_INTEGER_DEFAULT(option));
    LONGS_EQUAL(123, CONFIG_INTEGER(option));
    config_file_option_free (option, 0);
    option = test_new_option (section_test, "opt", "integer", NULL,
                              "-5", "5000", 0);
    CHECK(option);
    LONGS_EQUAL(0, CONFIG_INTEGER_DEFAULT(option));
    LONGS_EQUAL(1000, CONFIG_INTEGER(option));
    config_file_option_free (option, 0);
    option = test_new_option (section_test, "opt", "integer", NULL,
                              "123", "abc", 0);
    CHECK(option);
    LONGS_EQUAL(123, CONFIG_INTEGER_DEFAULT(option));
    LONGS_EQUAL(0, CONFIG_INTEGER(option));
    config_file_option_free (option, 0);

    /* Color: invalid color is 0 */
    option = test_new_option (section_test, "opt", "color", NULL,
                              "red", "invalid_color", 0);
    CHECK(option);
    LONGS_EQUAL(CONFIG_OPTION_TYPE_COLOR, option->type);
    LONGS_EQUAL(1, option->themable);
    LONGS_EQUAL(gui_color_get_weechat_colors_number () - 1, option->max);
    LONGS_EQUAL(3, CONFIG_COLOR_DEFAULT(option));
    LONGS_EQUAL(0, CONFIG_COLOR(option));
    config_file_option_free (option, 0);
    option = test_new_option (section_test, "opt", "color", NULL,
                              "invalid_color", "red", 0);
    CHECK(option);
    LONGS_EQUAL(0, CONFIG_COLOR_DEFAULT(option));
    LONGS_EQUAL(3, CONFIG_COLOR(option));
    config_file_option_free (option, 0);

    /* Enum: unknown value is first value (index 0) */
    option = test_new_option (section_test, "opt", "enum", "|v1|v2||v3|",
                              "v3", "zzz", 0);
    CHECK(option);
    LONGS_EQUAL(CONFIG_OPTION_TYPE_ENUM, option->type);
    CHECK(option->string_values);
    STRCMP_EQUAL("v1", option->string_values[0]);
    STRCMP_EQUAL("v2", option->string_values[1]);
    STRCMP_EQUAL("v3", option->string_values[2]);
    POINTERS_EQUAL(NULL, option->string_values[3]);
    LONGS_EQUAL(0, option->min);
    LONGS_EQUAL(2, option->max);
    LONGS_EQUAL(2, CONFIG_ENUM_DEFAULT(option));
    LONGS_EQUAL(0, CONFIG_ENUM(option));
    config_file_option_free (option, 0);

    /* Integer with string values is converted to enum */
    option = test_new_option (section_test, "opt", "integer", "v1|v2",
                              "v2", NULL, 0);
    CHECK(option);
    LONGS_EQUAL(CONFIG_OPTION_TYPE_ENUM, option->type);
    LONGS_EQUAL(1, CONFIG_ENUM(option));
    config_file_option_free (option, 0);

    /* Option without section */
    option = config_file_new_option (
        NULL, NULL, "opt", "string", NULL, NULL, 0, 0, "value", NULL, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    CHECK(option);
    POINTERS_EQUAL(NULL, option->config_file);
    POINTERS_EQUAL(NULL, option->section);
    POINTERS_EQUAL(NULL, option->description);
    POINTERS_EQUAL(NULL, option->prev_option);
    POINTERS_EQUAL(NULL, option->next_option);
    POINTERS_EQUAL(NULL, section_test->options);
    config_file_option_free (option, 0);
}

/*
 * Test functions:
 *   config_file_search_option
 */

TEST(CoreConfigFile, SearchOption)
{
    POINTERS_EQUAL(NULL, config_file_search_option (NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, config_file_search_option (weechat_config_file,
                                                    NULL, NULL));
    POINTERS_EQUAL(NULL,
                   config_file_search_option (weechat_config_file,
                                              weechat_config_section_color,
                                              NULL));

    POINTERS_EQUAL(NULL,
                   config_file_search_option (weechat_config_file,
                                              weechat_config_section_color,
                                              "xxx"));
    POINTERS_EQUAL(NULL,
                   config_file_search_option (weechat_config_file,
                                              NULL,
                                              "xxx"));
    POINTERS_EQUAL(NULL,
                   config_file_search_option (NULL,
                                              weechat_config_section_color,
                                              "xxx"));

    POINTERS_EQUAL(config_color_chat_channel,
                   config_file_search_option (weechat_config_file,
                                              weechat_config_section_color,
                                              "chat_channel"));
    POINTERS_EQUAL(config_color_chat_channel,
                   config_file_search_option (weechat_config_file,
                                              NULL,
                                              "chat_channel"));
    POINTERS_EQUAL(config_color_chat_channel,
                   config_file_search_option (NULL,
                                              weechat_config_section_color,
                                              "chat_channel"));
}

/*
 * Test functions:
 *   config_file_search_section_option
 */

TEST(CoreConfigFile, SearchSectionOption)
{
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;

    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    config_file_search_section_option (NULL, NULL, NULL,
                                       &ptr_section, &ptr_option);
    POINTERS_EQUAL(NULL, ptr_section);
    POINTERS_EQUAL(NULL, ptr_option);

    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    config_file_search_section_option (weechat_config_file, NULL, NULL,
                                       &ptr_section, &ptr_option);
    POINTERS_EQUAL(NULL, ptr_section);
    POINTERS_EQUAL(NULL, ptr_option);

    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    config_file_search_section_option (weechat_config_file,
                                       weechat_config_section_color,
                                       NULL,
                                       &ptr_section, &ptr_option);
    POINTERS_EQUAL(NULL, ptr_section);
    POINTERS_EQUAL(NULL, ptr_option);

    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    config_file_search_section_option (weechat_config_file,
                                       weechat_config_section_color,
                                       "xxx",
                                       &ptr_section, &ptr_option);
    POINTERS_EQUAL(NULL, ptr_section);
    POINTERS_EQUAL(NULL, ptr_option);

    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    config_file_search_section_option (weechat_config_file,
                                       weechat_config_section_color,
                                       "chat_channel",
                                       &ptr_section, &ptr_option);
    POINTERS_EQUAL(weechat_config_section_color, ptr_section);
    POINTERS_EQUAL(config_color_chat_channel, ptr_option);

    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    config_file_search_section_option (weechat_config_file,
                                       NULL,
                                       "chat_channel",
                                       &ptr_section, &ptr_option);
    POINTERS_EQUAL(weechat_config_section_color, ptr_section);
    POINTERS_EQUAL(config_color_chat_channel, ptr_option);

    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    config_file_search_section_option (NULL,
                                       weechat_config_section_color,
                                       "chat_channel",
                                       &ptr_section, &ptr_option);
    POINTERS_EQUAL(weechat_config_section_color, ptr_section);
    POINTERS_EQUAL(config_color_chat_channel, ptr_option);
}

/*
 * Test functions:
 *   config_file_search_with_string
 */

TEST(CoreConfigFile, SearchWithString)
{
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    const char *pos_option_name;

    ptr_config = (struct t_config_file *)0x1;
    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    pos_option_name = (char *)0x1;
    config_file_search_with_string (NULL, NULL, NULL, NULL, NULL);
    POINTERS_EQUAL(0x1, ptr_config);
    POINTERS_EQUAL(0x1, ptr_section);
    POINTERS_EQUAL(0x1, ptr_option);
    POINTERS_EQUAL(0x1, pos_option_name);

    ptr_config = (struct t_config_file *)0x1;
    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    pos_option_name = (char *)0x1;
    config_file_search_with_string (NULL, &ptr_config, &ptr_section,
                                    &ptr_option, &pos_option_name);
    POINTERS_EQUAL(NULL, ptr_config);
    POINTERS_EQUAL(NULL, ptr_section);
    POINTERS_EQUAL(NULL, ptr_option);
    POINTERS_EQUAL(NULL, pos_option_name);

    ptr_config = (struct t_config_file *)0x1;
    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    pos_option_name = (char *)0x1;
    config_file_search_with_string ("", &ptr_config, &ptr_section,
                                    &ptr_option, &pos_option_name);
    POINTERS_EQUAL(NULL, ptr_config);
    POINTERS_EQUAL(NULL, ptr_section);
    POINTERS_EQUAL(NULL, ptr_option);
    POINTERS_EQUAL(NULL, pos_option_name);

    ptr_config = (struct t_config_file *)0x1;
    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    pos_option_name = (char *)0x1;
    config_file_search_with_string ("zzz", &ptr_config, &ptr_section,
                                    &ptr_option, &pos_option_name);
    POINTERS_EQUAL(NULL, ptr_config);
    POINTERS_EQUAL(NULL, ptr_section);
    POINTERS_EQUAL(NULL, ptr_option);
    POINTERS_EQUAL(NULL, pos_option_name);

    ptr_config = (struct t_config_file *)0x1;
    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    pos_option_name = (char *)0x1;
    config_file_search_with_string ("weechat.color.chat_channel",
                                    &ptr_config, &ptr_section,
                                    &ptr_option, &pos_option_name);
    POINTERS_EQUAL(weechat_config_file, ptr_config);
    POINTERS_EQUAL(weechat_config_section_color, ptr_section);
    POINTERS_EQUAL(config_color_chat_channel, ptr_option);
    STRCMP_EQUAL("chat_channel", pos_option_name);
}

/*
 * Test functions:
 *   config_file_get_parent_option
 */

TEST(CoreConfigFileWithNewOptions, GetParentOption)
{
    struct t_config_option *option;

    POINTERS_EQUAL(NULL, config_file_get_parent_option (NULL));

    /* Options without parent */
    POINTERS_EQUAL(NULL, config_file_get_parent_option (ptr_option_bool));
    POINTERS_EQUAL(NULL, config_file_get_parent_option (ptr_option_int));
    POINTERS_EQUAL(NULL, config_file_get_parent_option (ptr_option_str));
    POINTERS_EQUAL(NULL, config_file_get_parent_option (ptr_option_col));
    POINTERS_EQUAL(NULL, config_file_get_parent_option (ptr_option_enum));

    /* Options with parent */
    POINTERS_EQUAL(ptr_option_bool,
                   config_file_get_parent_option (ptr_option_bool_child));
    POINTERS_EQUAL(ptr_option_int,
                   config_file_get_parent_option (ptr_option_int_child));
    POINTERS_EQUAL(ptr_option_int_str,
                   config_file_get_parent_option (ptr_option_int_str_child));
    POINTERS_EQUAL(ptr_option_str,
                   config_file_get_parent_option (ptr_option_str_child));
    POINTERS_EQUAL(ptr_option_col,
                   config_file_get_parent_option (ptr_option_col_child));
    POINTERS_EQUAL(ptr_option_enum,
                   config_file_get_parent_option (ptr_option_enum_child));

    /* Parent option not found */
    option = config_file_new_option (
        weechat_config_file, weechat_config_section_look,
        "test_orphan << weechat.look.zzz",
        "string", "", NULL, 0, 0, NULL, NULL, 1,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    CHECK(option);
    POINTERS_EQUAL(NULL, config_file_get_parent_option (option));
    config_file_option_free (option, 0);
}

/*
 * Test functions:
 *   config_file_string_boolean_is_valid
 */

TEST(CoreConfigFile, StringBooleanIsValid)
{
    LONGS_EQUAL(0, config_file_string_boolean_is_valid (NULL));
    LONGS_EQUAL(0, config_file_string_boolean_is_valid (""));
    LONGS_EQUAL(0, config_file_string_boolean_is_valid ("zzz"));

    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("on"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("yes"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("y"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("true"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("t"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("1"));

    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("off"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("no"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("n"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("false"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("f"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("0"));
}

/*
 * Test functions:
 *   config_file_string_to_boolean
 */

TEST(CoreConfigFile, StringToBoolean)
{
    LONGS_EQUAL(0, config_file_string_to_boolean (NULL));
    LONGS_EQUAL(0, config_file_string_to_boolean (""));
    LONGS_EQUAL(0, config_file_string_to_boolean ("zzz"));

    LONGS_EQUAL(1, config_file_string_to_boolean ("on"));
    LONGS_EQUAL(1, config_file_string_to_boolean ("yes"));
    LONGS_EQUAL(1, config_file_string_to_boolean ("y"));
    LONGS_EQUAL(1, config_file_string_to_boolean ("true"));
    LONGS_EQUAL(1, config_file_string_to_boolean ("t"));
    LONGS_EQUAL(1, config_file_string_to_boolean ("1"));

    LONGS_EQUAL(0, config_file_string_to_boolean ("off"));
    LONGS_EQUAL(0, config_file_string_to_boolean ("no"));
    LONGS_EQUAL(0, config_file_string_to_boolean ("n"));
    LONGS_EQUAL(0, config_file_string_to_boolean ("false"));
    LONGS_EQUAL(0, config_file_string_to_boolean ("f"));
    LONGS_EQUAL(0, config_file_string_to_boolean ("0"));
}

/*
 * Test functions:
 *   config_file_option_set
 *   config_file_option_reset
 */

TEST(CoreConfigFileWithNewOptions, OptionSetReset)
{
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_reset (NULL, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (NULL, NULL, 1));

    /* Boolean */
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_bool, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_bool, "on", 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_bool, "toggle", 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_bool, "toggle", 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_bool, 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option_bool));

    /* Integer */
    LONGS_EQUAL(100, CONFIG_INTEGER(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_int, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_int, "-500", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_int, "99999999", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_int, "50", 1));
    LONGS_EQUAL(50, CONFIG_INTEGER(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_int, "++15", 1));
    LONGS_EQUAL(65, CONFIG_INTEGER(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_int, "--3", 1));
    LONGS_EQUAL(62, CONFIG_INTEGER(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_int, 1));
    LONGS_EQUAL(100, CONFIG_INTEGER(ptr_option_int));

    /* Integer with string values (enum with WeeChat >= 4.1.0) */
    LONGS_EQUAL(1, CONFIG_INTEGER(ptr_option_int_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_int_str, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_int_str, "v3", 1));
    LONGS_EQUAL(2, CONFIG_INTEGER(ptr_option_int_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_int_str, 1));
    LONGS_EQUAL(1, CONFIG_INTEGER(ptr_option_int_str));

    /* String */
    STRCMP_EQUAL("value", CONFIG_STRING(ptr_option_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_str, "xxx", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_str, "test", 1));
    STRCMP_EQUAL("test", CONFIG_STRING(ptr_option_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_str, 1));
    STRCMP_EQUAL("value", CONFIG_STRING(ptr_option_str));

    /* Color */
    LONGS_EQUAL(9, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_col, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "red", 1));
    LONGS_EQUAL(3, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "++5", 1));
    LONGS_EQUAL(8, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "--3", 1));
    LONGS_EQUAL(5, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "%red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_BLINK_FLAG, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, ".red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_DIM_FLAG, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "*red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_BOLD_FLAG, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "!red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_REVERSE_FLAG, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "/red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_ITALIC_FLAG, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "_red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_UNDERLINE_FLAG, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "|red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_KEEPATTR_FLAG, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "%.*!/_|red", 1));
    LONGS_EQUAL(3
                | GUI_COLOR_EXTENDED_BLINK_FLAG
                | GUI_COLOR_EXTENDED_DIM_FLAG
                | GUI_COLOR_EXTENDED_BOLD_FLAG
                | GUI_COLOR_EXTENDED_REVERSE_FLAG
                | GUI_COLOR_EXTENDED_ITALIC_FLAG
                | GUI_COLOR_EXTENDED_UNDERLINE_FLAG
                | GUI_COLOR_EXTENDED_KEEPATTR_FLAG,
                CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_col, 1));
    LONGS_EQUAL(9, CONFIG_COLOR(ptr_option_col));

    /* Enum */
    LONGS_EQUAL(1, CONFIG_ENUM(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_enum, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_enum, "v3", 1));
    LONGS_EQUAL(2, CONFIG_ENUM(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_enum, 1));
    LONGS_EQUAL(1, CONFIG_INTEGER(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_enum, "++abc", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_enum, "--abc", 1));
    LONGS_EQUAL(1, CONFIG_ENUM(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_enum, "++1", 1));
    LONGS_EQUAL(2, CONFIG_ENUM(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_enum, "++2", 1));
    LONGS_EQUAL(1, CONFIG_ENUM(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_enum, "--1", 1));
    LONGS_EQUAL(0, CONFIG_ENUM(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_enum, "--4", 1));
    LONGS_EQUAL(2, CONFIG_ENUM(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set (ptr_option_enum, "++3", 1));
    LONGS_EQUAL(2, CONFIG_ENUM(ptr_option_enum));

    /* Null value: value is not changed if null is not allowed */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set (ptr_option_bool, NULL, 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option_bool));
}

/*
 * Test functions:
 *   config_file_option_set (option with null value)
 *   config_file_option_reset (option with null value)
 */

TEST(CoreConfigFileWithNewOptions, OptionSetResetNullValue)
{
    /* Boolean */
    POINTERS_EQUAL(NULL, ptr_option_bool_child->value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_bool_child, "zzz", 1));
    POINTERS_EQUAL(NULL, ptr_option_bool_child->value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_bool_child, "toggle", 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(ptr_option_bool_child));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_null (ptr_option_bool_child, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_bool_child, "off", 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option_bool_child));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_bool_child, 1));
    POINTERS_EQUAL(NULL, ptr_option_bool_child->value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_reset (ptr_option_bool_child, 1));
    POINTERS_EQUAL(NULL, ptr_option_bool_child->value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set (ptr_option_bool_child, NULL, 1));
    POINTERS_EQUAL(NULL, ptr_option_bool_child->value);
    config_file_option_set_default (ptr_option_bool_child, "on", 1);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_bool_child, 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(ptr_option_bool_child));

    /* Integer */
    POINTERS_EQUAL(NULL, ptr_option_int_child->value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_int_child, "zzz", 1));
    POINTERS_EQUAL(NULL, ptr_option_int_child->value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_int_child, "50", 1));
    LONGS_EQUAL(50, CONFIG_INTEGER(ptr_option_int_child));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_int_child, 1));
    POINTERS_EQUAL(NULL, ptr_option_int_child->value);
    config_file_option_set_default (ptr_option_int_child, "0", 1);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_int_child, 1));
    LONGS_EQUAL(0, CONFIG_INTEGER(ptr_option_int_child));

    /* Integer with string values (enum with WeeChat >= 4.1.0) */
    POINTERS_EQUAL(NULL, ptr_option_int_str_child->value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_int_str_child, "zzz", 1));
    POINTERS_EQUAL(NULL, ptr_option_int_str_child->value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_int_str_child, "v3", 1));
    LONGS_EQUAL(2, CONFIG_ENUM(ptr_option_int_str_child));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_int_str_child, 1));
    POINTERS_EQUAL(NULL, ptr_option_int_str_child->value);
    config_file_option_set_default (ptr_option_int_str_child, "v3", 1);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_int_str_child, 1));
    LONGS_EQUAL(2, CONFIG_ENUM(ptr_option_int_str_child));

    /* String */
    POINTERS_EQUAL(NULL, ptr_option_str_child->value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_str_child, "test", 1));
    STRCMP_EQUAL("test", CONFIG_STRING(ptr_option_str_child));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_str_child, 1));
    POINTERS_EQUAL(NULL, ptr_option_str_child->value);

    /* Color */
    POINTERS_EQUAL(NULL, ptr_option_col_child->value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_col_child, "zzz", 1));
    POINTERS_EQUAL(NULL, ptr_option_col_child->value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col_child, "red", 1));
    LONGS_EQUAL(3, CONFIG_COLOR(ptr_option_col_child));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_col_child, 1));
    POINTERS_EQUAL(NULL, ptr_option_col_child->value);
    config_file_option_set_default (ptr_option_col_child, "red", 1);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_col_child, 1));
    LONGS_EQUAL(3, CONFIG_COLOR(ptr_option_col_child));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_reset (ptr_option_col_child, 1));

    /* Enum */
    POINTERS_EQUAL(NULL, ptr_option_enum_child->value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_enum_child, "zzz", 1));
    POINTERS_EQUAL(NULL, ptr_option_enum_child->value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_enum_child, "++1", 1));
    LONGS_EQUAL(1, CONFIG_ENUM(ptr_option_enum_child));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_null (ptr_option_enum_child, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_enum_child, "--1", 1));
    LONGS_EQUAL(2, CONFIG_ENUM(ptr_option_enum_child));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_enum_child, 1));
    POINTERS_EQUAL(NULL, ptr_option_enum_child->value);
    config_file_option_set_default (ptr_option_enum_child, "v1", 1);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_enum_child, 1));
    LONGS_EQUAL(0, CONFIG_ENUM(ptr_option_enum_child));
}

/*
 * Test functions:
 *   config_file_option_set (callback "change")
 *   config_file_option_reset (callback "change")
 *   config_file_option_set_default (callback "change")
 */

TEST(CoreConfigFileWithTestConfig, OptionSetCallbackChange)
{
    struct t_config_option *option;

    option = config_file_new_option (
        config_test, section_test,
        "int", "integer", "", NULL, 0, 100, "10", NULL, 0,
        NULL, NULL, NULL,
        &test_option_change_cb, NULL, NULL,
        NULL, NULL, NULL);
    CHECK(option);
    LONGS_EQUAL(0, test_option_change_cb_count);

    /* Set */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (option, "20", 0));
    LONGS_EQUAL(0, test_option_change_cb_count);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (option, "30", 1));
    LONGS_EQUAL(1, test_option_change_cb_count);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set (option, "30", 1));
    LONGS_EQUAL(1, test_option_change_cb_count);

    /* Reset */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (option, 0));
    LONGS_EQUAL(1, test_option_change_cb_count);
    config_file_option_set (option, "40", 0);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (option, 1));
    LONGS_EQUAL(2, test_option_change_cb_count);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_reset (option, 1));
    LONGS_EQUAL(2, test_option_change_cb_count);

    /* Set default */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (option, "50", 0));
    LONGS_EQUAL(2, test_option_change_cb_count);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (option, "60", 1));
    LONGS_EQUAL(3, test_option_change_cb_count);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_default (option, "60", 1));
    LONGS_EQUAL(3, test_option_change_cb_count);
}

/*
 * Test functions:
 *   config_file_option_toggle
 */

TEST(CoreConfigFileWithNewOptions, OptionToggle)
{
    const char *value_boolean_ok[] = { "on", NULL };
    const char *values_boolean_ok[] = { "on", "off", NULL };
    const char *values_boolean_error[] = { "xxx", "zzz", NULL };
    const char *value_integer_ok[] = { "50", NULL };
    const char *values_integer_ok[] = { "75", "92", NULL };
    const char *values_integer_error[] = { "-500", "99999999", NULL };
    const char *value_integer_str_ok[] = { "v3", NULL };
    const char *values_integer_str_ok[] = { "v1", "v3", NULL };
    const char *values_integer_str_error[] = { "xxx", "zzz", NULL };
    const char *value_string_ok[] = { "+", NULL };
    const char *values_string_ok[] = { "$", "*", NULL };
    const char *values_string_error[] = { "xxx", "zzz", NULL };
    const char *value_color_ok[] = { "red", NULL };
    const char *values_color_ok[] = { "green", "cyan", NULL };
    const char *values_color_error[] = { "xxx", "zzz", NULL };

    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (NULL, NULL, 0, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_bool, NULL, -1, 1));

    /* Boolean */
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_bool,
                                           values_boolean_error, 2, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_bool, NULL, 0, 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_bool, NULL, 0, 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_bool, value_boolean_ok, 1, 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_bool, value_boolean_ok, 1, 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_bool, values_boolean_ok, 2, 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_bool, values_boolean_ok, 2, 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_reset (ptr_option_bool, 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option_bool));

    /* Integer */
    LONGS_EQUAL(100, CONFIG_INTEGER(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_int,
                                           values_integer_error, 2, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_int,
                                           NULL, 0, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_int,
                                           value_integer_ok, 1, 1));
    LONGS_EQUAL(50, CONFIG_INTEGER(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_int,
                                           value_integer_ok, 1, 1));
    LONGS_EQUAL(100, CONFIG_INTEGER(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_int,
                                           values_integer_ok, 2, 1));
    LONGS_EQUAL(75, CONFIG_INTEGER(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_int,
                                           values_integer_ok, 2, 1));
    LONGS_EQUAL(92, CONFIG_INTEGER(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_int, 1));
    LONGS_EQUAL(100, CONFIG_INTEGER(ptr_option_int));

    /* Integer with string values (enum with WeeChat >= 4.1.0) */
    LONGS_EQUAL(1, CONFIG_INTEGER(ptr_option_int_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_int_str,
                                           values_integer_str_error, 2, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_int_str,
                                           NULL, 0, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_int_str,
                                           value_integer_str_ok, 1, 1));
    LONGS_EQUAL(2, CONFIG_INTEGER(ptr_option_int_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_int_str,
                                           values_integer_str_ok, 2, 1));
    LONGS_EQUAL(0, CONFIG_INTEGER(ptr_option_int_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_int_str,
                                           values_integer_str_ok, 2, 1));
    LONGS_EQUAL(2, CONFIG_INTEGER(ptr_option_int_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_int_str, 1));
    LONGS_EQUAL(1, CONFIG_INTEGER(ptr_option_int_str));

    /* String */
    STRCMP_EQUAL("value", CONFIG_STRING(ptr_option_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_str,
                                           values_string_error, 2, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_str,
                                           NULL, 0, 1));
    STRCMP_EQUAL("", CONFIG_STRING(ptr_option_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_str,
                                           NULL, 0, 1));
    STRCMP_EQUAL("value", CONFIG_STRING(ptr_option_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_str,
                                           value_string_ok, 1, 1));
    STRCMP_EQUAL("+", CONFIG_STRING(ptr_option_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_str,
                                           values_string_ok, 2, 1));
    STRCMP_EQUAL("$", CONFIG_STRING(ptr_option_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_str,
                                           values_string_ok, 2, 1));
    STRCMP_EQUAL("*", CONFIG_STRING(ptr_option_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_str, 1));
    STRCMP_EQUAL("value", CONFIG_STRING(ptr_option_str));

    /* Color */
    LONGS_EQUAL(9, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_col,
                                           values_color_error, 2, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_col, NULL, 0, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_col,
                                           value_color_ok, 1, 1));
    LONGS_EQUAL(3, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_col,
                                           values_color_ok, 2, 1));
    LONGS_EQUAL(5, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_col,
                                           values_color_ok, 2, 1));
    LONGS_EQUAL(13, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_col, 1));
    LONGS_EQUAL(9, CONFIG_COLOR(ptr_option_col));

    /* Enum */
    LONGS_EQUAL(1, CONFIG_ENUM(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_enum,
                                           values_integer_str_error, 2, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_enum,
                                           NULL, 0, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_enum,
                                           value_integer_str_ok, 1, 1));
    LONGS_EQUAL(2, CONFIG_ENUM(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_enum,
                                           values_integer_str_ok, 2, 1));
    LONGS_EQUAL(0, CONFIG_ENUM(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_enum,
                                           values_integer_str_ok, 2, 1));
    LONGS_EQUAL(2, CONFIG_ENUM(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_enum, 1));
    LONGS_EQUAL(1, CONFIG_ENUM(ptr_option_enum));
}

/*
 * Test functions:
 *   config_file_option_set_null
 */

TEST(CoreConfigFileWithTestConfig, OptionSetNull)
{
    struct t_config_option *opt_str, *opt_str_null;

    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_null (NULL, 1));

    /* Null value not allowed */
    opt_str = test_new_option (section_test, "str", "string", NULL,
                               "value", NULL, 0);
    CHECK(opt_str);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_null (opt_str, 1));
    STRCMP_EQUAL("value", CONFIG_STRING(opt_str));

    /* Null value allowed */
    opt_str_null = config_file_new_option (
        config_test, section_test,
        "str_null", "string", "", NULL, 0, 0, "default", "value", 1,
        NULL, NULL, NULL,
        &test_option_change_cb, NULL, NULL,
        NULL, NULL, NULL);
    CHECK(opt_str_null);
    STRCMP_EQUAL("value", CONFIG_STRING(opt_str_null));

    /* Without callback */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_null (opt_str_null, 0));
    POINTERS_EQUAL(NULL, opt_str_null->value);
    STRCMP_EQUAL("default", CONFIG_STRING_DEFAULT(opt_str_null));
    LONGS_EQUAL(0, test_option_change_cb_count);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_null (opt_str_null, 1));
    LONGS_EQUAL(0, test_option_change_cb_count);

    /* With callback */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (opt_str_null, "test", 0));
    STRCMP_EQUAL("test", CONFIG_STRING(opt_str_null));
    LONGS_EQUAL(0, test_option_change_cb_count);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_null (opt_str_null, 1));
    POINTERS_EQUAL(NULL, opt_str_null->value);
    LONGS_EQUAL(1, test_option_change_cb_count);
}

/*
 * Test functions:
 *   config_file_option_set_default
 */

TEST(CoreConfigFileWithNewOptions, OptionSetDefault)
{
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (NULL, NULL, 1));

    /* Boolean */
    LONGS_EQUAL(0, CONFIG_BOOLEAN_DEFAULT(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_default (ptr_option_bool, NULL, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (ptr_option_bool, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_bool, "on", 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN_DEFAULT(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_bool, "toggle", 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN_DEFAULT(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_bool, "toggle", 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN_DEFAULT(ptr_option_bool));

    /* Integer */
    LONGS_EQUAL(100, CONFIG_INTEGER_DEFAULT(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_default (ptr_option_int, NULL, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (ptr_option_int, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (ptr_option_int, "-500", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (ptr_option_int, "99999999", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_int, "50", 1));
    LONGS_EQUAL(50, CONFIG_INTEGER_DEFAULT(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_int, "++15", 1));
    LONGS_EQUAL(65, CONFIG_INTEGER_DEFAULT(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_int, "--3", 1));
    LONGS_EQUAL(62, CONFIG_INTEGER_DEFAULT(ptr_option_int));

    /* Integer with string values (enum with WeeChat >= 4.1.0) */
    LONGS_EQUAL(1, CONFIG_INTEGER_DEFAULT(ptr_option_int_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_default (ptr_option_int_str, NULL, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (ptr_option_int_str, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_int_str, "v3", 1));
    LONGS_EQUAL(2, CONFIG_INTEGER_DEFAULT(ptr_option_int_str));

    /* String */
    STRCMP_EQUAL("value", CONFIG_STRING_DEFAULT(ptr_option_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_str, "xxx", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_str, "test", 1));
    STRCMP_EQUAL("test", CONFIG_STRING_DEFAULT(ptr_option_str));

    /* Color */
    LONGS_EQUAL(9, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (ptr_option_col, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "red", 1));
    LONGS_EQUAL(3, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "++5", 1));
    LONGS_EQUAL(8, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "--3", 1));
    LONGS_EQUAL(5, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "%red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_BLINK_FLAG, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, ".red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_DIM_FLAG, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "*red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_BOLD_FLAG, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "!red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_REVERSE_FLAG, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "/red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_ITALIC_FLAG, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "_red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_UNDERLINE_FLAG, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "|red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_KEEPATTR_FLAG, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "%.*!/_|red", 1));
    LONGS_EQUAL(3
                | GUI_COLOR_EXTENDED_BLINK_FLAG
                | GUI_COLOR_EXTENDED_DIM_FLAG
                | GUI_COLOR_EXTENDED_BOLD_FLAG
                | GUI_COLOR_EXTENDED_REVERSE_FLAG
                | GUI_COLOR_EXTENDED_ITALIC_FLAG
                | GUI_COLOR_EXTENDED_UNDERLINE_FLAG
                | GUI_COLOR_EXTENDED_KEEPATTR_FLAG,
                CONFIG_COLOR_DEFAULT(ptr_option_col));

    /* Enum */
    LONGS_EQUAL(1, CONFIG_ENUM_DEFAULT(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_default (ptr_option_enum, NULL, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (ptr_option_enum, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_enum, "v3", 1));
    LONGS_EQUAL(2, CONFIG_INTEGER_DEFAULT(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_enum, "++2", 1));
    LONGS_EQUAL(1, CONFIG_ENUM_DEFAULT(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_enum, "--2", 1));
    LONGS_EQUAL(2, CONFIG_ENUM_DEFAULT(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_default (ptr_option_enum, "++3", 1));
    LONGS_EQUAL(2, CONFIG_ENUM_DEFAULT(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (ptr_option_enum, "++abc", 1));
    LONGS_EQUAL(2, CONFIG_ENUM_DEFAULT(ptr_option_enum));
}

/*
 * Test functions:
 *   config_file_option_set_default (option with null default value)
 */

TEST(CoreConfigFileWithNewOptions, OptionSetDefaultNullValue)
{
    /* Null value: default value is not changed if null is not allowed */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_default (ptr_option_bool, NULL, 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN_DEFAULT(ptr_option_bool));

    /* Boolean */
    POINTERS_EQUAL(NULL, ptr_option_bool_child->default_value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (ptr_option_bool_child,
                                                "zzz", 1));
    POINTERS_EQUAL(NULL, ptr_option_bool_child->default_value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_bool_child,
                                                "toggle", 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN_DEFAULT(ptr_option_bool_child));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_bool_child,
                                                NULL, 1));
    POINTERS_EQUAL(NULL, ptr_option_bool_child->default_value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_default (ptr_option_bool_child,
                                                NULL, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_bool_child,
                                                "on", 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN_DEFAULT(ptr_option_bool_child));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_default (ptr_option_bool_child,
                                                "on", 1));

    /* Integer */
    POINTERS_EQUAL(NULL, ptr_option_int_child->default_value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (ptr_option_int_child,
                                                "zzz", 1));
    POINTERS_EQUAL(NULL, ptr_option_int_child->default_value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_int_child,
                                                "50", 1));
    LONGS_EQUAL(50, CONFIG_INTEGER_DEFAULT(ptr_option_int_child));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_default (ptr_option_int_child,
                                                "50", 1));

    /* Color */
    POINTERS_EQUAL(NULL, ptr_option_col_child->default_value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (ptr_option_col_child,
                                                "zzz", 1));
    POINTERS_EQUAL(NULL, ptr_option_col_child->default_value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col_child,
                                                "red", 1));
    LONGS_EQUAL(3, CONFIG_COLOR_DEFAULT(ptr_option_col_child));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_default (ptr_option_col_child,
                                                "red", 1));

    /* Enum */
    POINTERS_EQUAL(NULL, ptr_option_enum_child->default_value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (ptr_option_enum_child,
                                                "zzz", 1));
    POINTERS_EQUAL(NULL, ptr_option_enum_child->default_value);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_enum_child,
                                                "++1", 1));
    LONGS_EQUAL(1, CONFIG_ENUM_DEFAULT(ptr_option_enum_child));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_enum_child,
                                                NULL, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_enum_child,
                                                "--1", 1));
    LONGS_EQUAL(2, CONFIG_ENUM_DEFAULT(ptr_option_enum_child));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_default (ptr_option_enum_child,
                                                "v3", 1));
}

/*
 * Test functions:
 *   config_file_option_unset
 */

TEST(CoreConfigFileWithTestConfig, OptionUnset)
{
    struct t_config_section *section_del, *section_del_cb;
    struct t_config_option *opt_int, *opt_str, *opt_del, *opt_del_cb;

    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_UNSET_ERROR,
                config_file_option_unset (NULL));

    /* Section where options can not be deleted: option is reset. */
    opt_int = test_new_option (section_test, "int", "integer", NULL,
                               "42", NULL, 0);
    CHECK(opt_int);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET,
                config_file_option_unset (opt_int));
    config_file_option_set (opt_int, "50", 1);
    LONGS_EQUAL(50, CONFIG_INTEGER(opt_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_UNSET_OK_RESET,
                config_file_option_unset (opt_int));
    LONGS_EQUAL(42, CONFIG_INTEGER(opt_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET,
                config_file_option_unset (opt_int));
    POINTERS_EQUAL(opt_int, config_file_search_option (config_test,
                                                       section_test, "int"));

    /*
     * Option with null default value where null is not allowed (forced
     * here, it can not be created like this): reset fails.
     */
    opt_str = test_new_option (section_test, "str", "string", NULL,
                               NULL, "value", 1);
    CHECK(opt_str);
    opt_str->null_value_allowed = 0;
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_UNSET_ERROR,
                config_file_option_unset (opt_str));
    STRCMP_EQUAL("value", CONFIG_STRING(opt_str));

    /* Section where options can be deleted (without callback). */
    section_del = test_new_section (config_test, "del", 0, 1);
    CHECK(section_del);
    opt_del = config_file_new_option (
        config_test, section_del,
        "opt", "string", "", NULL, 0, 0, "value", NULL, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        &test_option_delete_cb, NULL, NULL);
    CHECK(opt_del);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED,
                config_file_option_unset (opt_del));
    LONGS_EQUAL(1, test_option_delete_cb_count);
    POINTERS_EQUAL(NULL, config_file_search_option (config_test,
                                                    section_del, "opt"));
    POINTERS_EQUAL(NULL, section_del->options);

    /* Section where options can be deleted (with callback). */
    section_del_cb = config_file_new_section (
        config_test, "del_cb", 0, 1,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        &test_section_delete_option_cb, NULL, NULL);
    CHECK(section_del_cb);
    opt_del_cb = test_new_option (section_del_cb, "opt", "string", NULL,
                                  "value", NULL, 0);
    CHECK(opt_del_cb);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED,
                config_file_option_unset (opt_del_cb));
    LONGS_EQUAL(1, test_section_delete_option_cb_count);
    POINTERS_EQUAL(NULL, config_file_search_option (config_test,
                                                    section_del_cb, "opt"));
}

/*
 * Test functions:
 *   config_file_option_rename
 */

TEST(CoreConfigFileWithTestConfig, OptionRename)
{
    struct t_config_option *opt_aaa, *opt_ccc, *opt_child;

    config_file_option_rename (NULL, NULL);
    config_file_option_rename (NULL, "test");

    opt_aaa = test_new_option (section_test, "aaa", "string", NULL,
                               "value", NULL, 0);
    opt_ccc = test_new_option (section_test, "ccc", "string", NULL,
                               "value", NULL, 0);
    opt_child = test_new_option (section_test,
                                 "child << " TEST_CONFIG_NAME ".sec.aaa",
                                 "string", NULL, NULL, NULL, 1);
    CHECK(opt_aaa);
    CHECK(opt_ccc);
    CHECK(opt_child);
    POINTERS_EQUAL(opt_aaa, section_test->options);
    POINTERS_EQUAL(opt_child, section_test->last_option);

    /* Invalid new name or option already existing: nothing is done. */
    config_file_option_rename (opt_aaa, NULL);
    STRCMP_EQUAL("aaa", opt_aaa->name);
    config_file_option_rename (opt_aaa, "");
    STRCMP_EQUAL("aaa", opt_aaa->name);
    config_file_option_rename (opt_aaa, "ccc");
    STRCMP_EQUAL("aaa", opt_aaa->name);
    STRCMP_EQUAL("ccc", opt_ccc->name);

    /* Rename option: it is moved in section and parent name is updated. */
    config_file_option_rename (opt_aaa, "zzz");
    STRCMP_EQUAL("zzz", opt_aaa->name);
    POINTERS_EQUAL(NULL, config_file_search_option (config_test,
                                                    section_test, "aaa"));
    POINTERS_EQUAL(opt_aaa, config_file_search_option (config_test,
                                                       section_test, "zzz"));
    POINTERS_EQUAL(opt_ccc, section_test->options);
    POINTERS_EQUAL(opt_aaa, section_test->last_option);
    POINTERS_EQUAL(NULL, opt_ccc->prev_option);
    POINTERS_EQUAL(opt_child, opt_ccc->next_option);
    POINTERS_EQUAL(opt_ccc, opt_child->prev_option);
    POINTERS_EQUAL(opt_aaa, opt_child->next_option);
    POINTERS_EQUAL(opt_child, opt_aaa->prev_option);
    POINTERS_EQUAL(NULL, opt_aaa->next_option);
    STRCMP_EQUAL(TEST_CONFIG_NAME ".sec.zzz", opt_child->parent_name);
    POINTERS_EQUAL(opt_aaa, config_file_get_parent_option (opt_child));

    /* Rename option in the middle of section (order: ccc, child, zzz) */
    config_file_option_rename (opt_child, "aab");
    STRCMP_EQUAL("aab", opt_child->name);
    POINTERS_EQUAL(opt_child, section_test->options);
    POINTERS_EQUAL(opt_aaa, section_test->last_option);
    POINTERS_EQUAL(NULL, opt_child->prev_option);
    POINTERS_EQUAL(opt_ccc, opt_child->next_option);
    POINTERS_EQUAL(opt_child, opt_ccc->prev_option);
    POINTERS_EQUAL(opt_aaa, opt_ccc->next_option);
    POINTERS_EQUAL(opt_ccc, opt_aaa->prev_option);

    /* Rename last option of section (order: aab, ccc, zzz) */
    config_file_option_rename (opt_aaa, "bbb");
    STRCMP_EQUAL("bbb", opt_aaa->name);
    POINTERS_EQUAL(opt_child, section_test->options);
    POINTERS_EQUAL(opt_ccc, section_test->last_option);
    POINTERS_EQUAL(opt_aaa, opt_child->next_option);
    POINTERS_EQUAL(opt_child, opt_aaa->prev_option);
    POINTERS_EQUAL(opt_ccc, opt_aaa->next_option);
    POINTERS_EQUAL(opt_aaa, opt_ccc->prev_option);
    POINTERS_EQUAL(NULL, opt_ccc->next_option);
    STRCMP_EQUAL(TEST_CONFIG_NAME ".sec.bbb", opt_child->parent_name);
}

/*
 * Test functions:
 *   config_file_option_value_to_string
 */

TEST(CoreConfigFileWithTestConfig, OptionValueToString)
{
    struct t_config_option *opt_bool, *opt_int, *opt_str, *opt_str_null;
    struct t_config_option *opt_col, *opt_enum;
    char *str, expected[1024];

    opt_bool = test_new_option (section_test, "bool", "boolean", NULL,
                                "off", "on", 0);
    opt_int = test_new_option (section_test, "int", "integer", NULL,
                               "10", "42", 0);
    opt_str = test_new_option (section_test, "str", "string", NULL,
                               "def", "val", 0);
    opt_str_null = test_new_option (section_test, "str_null", "string", NULL,
                                    NULL, NULL, 1);
    opt_col = test_new_option (section_test, "col", "color", NULL,
                               "blue", "red", 0);
    opt_enum = test_new_option (section_test, "enum", "enum", "v1|v2|v3",
                                "v1", "v3", 0);
    CHECK(opt_bool);
    CHECK(opt_int);
    CHECK(opt_str);
    CHECK(opt_str_null);
    CHECK(opt_col);
    CHECK(opt_enum);

    POINTERS_EQUAL(NULL, config_file_option_value_to_string (NULL, 0, 0, 0));

    /* Without colors */
    WEE_TEST_STR("on", config_file_option_value_to_string (opt_bool, 0, 0, 0));
    WEE_TEST_STR("off", config_file_option_value_to_string (opt_bool, 1, 0, 0));
    WEE_TEST_STR("42", config_file_option_value_to_string (opt_int, 0, 0, 0));
    WEE_TEST_STR("10", config_file_option_value_to_string (opt_int, 1, 0, 0));
    WEE_TEST_STR("val", config_file_option_value_to_string (opt_str, 0, 0, 0));
    WEE_TEST_STR("def", config_file_option_value_to_string (opt_str, 1, 0, 0));
    WEE_TEST_STR("\"val\"",
                 config_file_option_value_to_string (opt_str, 0, 0, 1));
    WEE_TEST_STR("\"def\"",
                 config_file_option_value_to_string (opt_str, 1, 0, 1));
    WEE_TEST_STR("null",
                 config_file_option_value_to_string (opt_str_null, 0, 0, 0));
    WEE_TEST_STR("null",
                 config_file_option_value_to_string (opt_str_null, 1, 0, 1));
    WEE_TEST_STR("red", config_file_option_value_to_string (opt_col, 0, 0, 0));
    WEE_TEST_STR("blue", config_file_option_value_to_string (opt_col, 1, 0, 0));
    WEE_TEST_STR("v3", config_file_option_value_to_string (opt_enum, 0, 0, 0));
    WEE_TEST_STR("v1", config_file_option_value_to_string (opt_enum, 1, 0, 0));

    /* Delimiters are used only for strings. */
    WEE_TEST_STR("on", config_file_option_value_to_string (opt_bool, 0, 0, 1));
    WEE_TEST_STR("42", config_file_option_value_to_string (opt_int, 0, 0, 1));

    /* With colors */
    snprintf (expected, sizeof (expected), "%s%s",
              GUI_COLOR(GUI_COLOR_CHAT_VALUE), "on");
    WEE_TEST_STR(expected,
                 config_file_option_value_to_string (opt_bool, 0, 1, 0));
    snprintf (expected, sizeof (expected), "%s%s",
              GUI_COLOR(GUI_COLOR_CHAT_VALUE), "42");
    WEE_TEST_STR(expected,
                 config_file_option_value_to_string (opt_int, 0, 1, 0));
    snprintf (expected, sizeof (expected), "%s%s",
              GUI_COLOR(GUI_COLOR_CHAT_VALUE), "val");
    WEE_TEST_STR(expected,
                 config_file_option_value_to_string (opt_str, 0, 1, 0));
    snprintf (expected, sizeof (expected), "%s\"%s%s%s\"",
              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
              GUI_COLOR(GUI_COLOR_CHAT_VALUE),
              "val",
              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
    WEE_TEST_STR(expected,
                 config_file_option_value_to_string (opt_str, 0, 1, 1));
    snprintf (expected, sizeof (expected), "%s%s",
              GUI_COLOR(GUI_COLOR_CHAT_VALUE_NULL), "null");
    WEE_TEST_STR(expected,
                 config_file_option_value_to_string (opt_str_null, 0, 1, 0));
    snprintf (expected, sizeof (expected), "%s%s",
              GUI_COLOR(GUI_COLOR_CHAT_VALUE), "red");
    WEE_TEST_STR(expected,
                 config_file_option_value_to_string (opt_col, 0, 1, 0));
    snprintf (expected, sizeof (expected), "%s%s",
              GUI_COLOR(GUI_COLOR_CHAT_VALUE), "v3");
    WEE_TEST_STR(expected,
                 config_file_option_value_to_string (opt_enum, 0, 1, 0));
}

/*
 * Test functions:
 *   config_file_option_get_string
 */

TEST(CoreConfigFileWithNewOptions, OptionGetString)
{
    POINTERS_EQUAL(NULL, config_file_option_get_string (NULL, NULL));
    POINTERS_EQUAL(NULL, config_file_option_get_string (NULL, "name"));
    POINTERS_EQUAL(NULL, config_file_option_get_string (ptr_option_bool, NULL));
    POINTERS_EQUAL(NULL, config_file_option_get_string (ptr_option_bool, ""));
    POINTERS_EQUAL(NULL, config_file_option_get_string (ptr_option_bool, "zzz"));

    STRCMP_EQUAL("weechat",
                 config_file_option_get_string (ptr_option_bool_child,
                                                "config_name"));
    STRCMP_EQUAL("look",
                 config_file_option_get_string (ptr_option_bool_child,
                                                "section_name"));
    STRCMP_EQUAL("test_boolean_child",
                 config_file_option_get_string (ptr_option_bool_child,
                                                "name"));
    STRCMP_EQUAL("weechat.look.test_boolean",
                 config_file_option_get_string (ptr_option_bool_child,
                                                "parent_name"));
    POINTERS_EQUAL(NULL,
                   config_file_option_get_string (ptr_option_bool,
                                                  "parent_name"));
    STRCMP_EQUAL("",
                 config_file_option_get_string (ptr_option_bool_child,
                                                "description"));

    STRCMP_EQUAL("boolean",
                 config_file_option_get_string (ptr_option_bool, "type"));
    STRCMP_EQUAL("integer",
                 config_file_option_get_string (ptr_option_int, "type"));
    STRCMP_EQUAL("enum",
                 config_file_option_get_string (ptr_option_int_str, "type"));
    STRCMP_EQUAL("string",
                 config_file_option_get_string (ptr_option_str, "type"));
    STRCMP_EQUAL("color",
                 config_file_option_get_string (ptr_option_col, "type"));
    STRCMP_EQUAL("enum",
                 config_file_option_get_string (ptr_option_enum, "type"));
}

/*
 * Test functions:
 *   config_file_option_get_pointer
 */

TEST(CoreConfigFileWithNewOptions, OptionGetPointer)
{
    POINTERS_EQUAL(NULL, config_file_option_get_pointer (NULL, NULL));
    POINTERS_EQUAL(NULL, config_file_option_get_pointer (NULL, "name"));
    POINTERS_EQUAL(NULL, config_file_option_get_pointer (ptr_option_int, NULL));
    POINTERS_EQUAL(NULL, config_file_option_get_pointer (ptr_option_int, ""));
    POINTERS_EQUAL(NULL, config_file_option_get_pointer (ptr_option_int, "zzz"));

    POINTERS_EQUAL(weechat_config_file,
                   config_file_option_get_pointer (ptr_option_int,
                                                   "config_file"));
    POINTERS_EQUAL(weechat_config_section_look,
                   config_file_option_get_pointer (ptr_option_int, "section"));
    POINTERS_EQUAL(ptr_option_int->name,
                   config_file_option_get_pointer (ptr_option_int, "name"));
    POINTERS_EQUAL(NULL,
                   config_file_option_get_pointer (ptr_option_int,
                                                   "parent_name"));
    POINTERS_EQUAL(ptr_option_int_child->parent_name,
                   config_file_option_get_pointer (ptr_option_int_child,
                                                   "parent_name"));
    POINTERS_EQUAL(&ptr_option_int->type,
                   config_file_option_get_pointer (ptr_option_int, "type"));
    POINTERS_EQUAL(&ptr_option_int->themable,
                   config_file_option_get_pointer (ptr_option_int,
                                                   "themable"));
    POINTERS_EQUAL(ptr_option_int->description,
                   config_file_option_get_pointer (ptr_option_int,
                                                   "description"));
    POINTERS_EQUAL(NULL,
                   config_file_option_get_pointer (ptr_option_int,
                                                   "string_values"));
    POINTERS_EQUAL(ptr_option_enum->string_values,
                   config_file_option_get_pointer (ptr_option_enum,
                                                   "string_values"));
    POINTERS_EQUAL(&ptr_option_int->min,
                   config_file_option_get_pointer (ptr_option_int, "min"));
    LONGS_EQUAL(0,
                *((int *)config_file_option_get_pointer (ptr_option_int,
                                                         "min")));
    POINTERS_EQUAL(&ptr_option_int->max,
                   config_file_option_get_pointer (ptr_option_int, "max"));
    LONGS_EQUAL(123456,
                *((int *)config_file_option_get_pointer (ptr_option_int,
                                                         "max")));
    POINTERS_EQUAL(ptr_option_int->default_value,
                   config_file_option_get_pointer (ptr_option_int,
                                                   "default_value"));
    LONGS_EQUAL(100,
                *((int *)config_file_option_get_pointer (ptr_option_int,
                                                         "default_value")));
    POINTERS_EQUAL(ptr_option_int->value,
                   config_file_option_get_pointer (ptr_option_int, "value"));
    LONGS_EQUAL(100,
                *((int *)config_file_option_get_pointer (ptr_option_int,
                                                         "value")));
    POINTERS_EQUAL(NULL,
                   config_file_option_get_pointer (ptr_option_int_child,
                                                   "value"));
    POINTERS_EQUAL(ptr_option_int->prev_option,
                   config_file_option_get_pointer (ptr_option_int,
                                                   "prev_option"));
    POINTERS_EQUAL(ptr_option_int->next_option,
                   config_file_option_get_pointer (ptr_option_int,
                                                   "next_option"));
}

/*
 * Test functions:
 *   config_file_option_is_null
 */

TEST(CoreConfigFileWithTestConfig, OptionIsNull)
{
    struct t_config_option *opt_str, *opt_str_null;

    LONGS_EQUAL(1, config_file_option_is_null (NULL));

    opt_str = test_new_option (section_test, "str", "string", NULL,
                               "value", NULL, 0);
    opt_str_null = test_new_option (section_test, "str_null", "string", NULL,
                                    "value", NULL, 1);
    CHECK(opt_str);
    CHECK(opt_str_null);

    LONGS_EQUAL(0, config_file_option_is_null (opt_str));
    LONGS_EQUAL(1, config_file_option_is_null (opt_str_null));
    config_file_option_set (opt_str_null, "test", 1);
    LONGS_EQUAL(0, config_file_option_is_null (opt_str_null));
    config_file_option_set_null (opt_str_null, 1);
    LONGS_EQUAL(1, config_file_option_is_null (opt_str_null));
}

/*
 * Test functions:
 *   config_file_option_default_is_null
 */

TEST(CoreConfigFileWithTestConfig, OptionDefaultIsNull)
{
    struct t_config_option *opt_str, *opt_str_null;

    LONGS_EQUAL(1, config_file_option_default_is_null (NULL));

    opt_str = test_new_option (section_test, "str", "string", NULL,
                               "value", NULL, 0);
    opt_str_null = test_new_option (section_test, "str_null", "string", NULL,
                                    NULL, "value", 1);
    CHECK(opt_str);
    CHECK(opt_str_null);

    LONGS_EQUAL(0, config_file_option_default_is_null (opt_str));
    LONGS_EQUAL(1, config_file_option_default_is_null (opt_str_null));
    LONGS_EQUAL(0, config_file_option_is_null (opt_str_null));
    config_file_option_set_default (opt_str_null, "test", 1);
    LONGS_EQUAL(0, config_file_option_default_is_null (opt_str_null));
}

/*
 * Test functions:
 *   config_file_option_has_changed
 */

TEST(CoreConfigFileWithNewOptions, OptionHasChanged)
{
    struct t_config_option *option;

    LONGS_EQUAL(0, config_file_option_has_changed (NULL));

    /* Boolean */
    LONGS_EQUAL(0, config_file_option_has_changed (ptr_option_bool));
    config_file_option_set (ptr_option_bool, "on", 1);
    LONGS_EQUAL(1, config_file_option_has_changed (ptr_option_bool));
    config_file_option_reset (ptr_option_bool, 1);
    LONGS_EQUAL(0, config_file_option_has_changed (ptr_option_bool));

    /* Integer */
    LONGS_EQUAL(0, config_file_option_has_changed (ptr_option_int));
    config_file_option_set (ptr_option_int, "123", 1);
    LONGS_EQUAL(1, config_file_option_has_changed (ptr_option_int));
    config_file_option_reset (ptr_option_int, 1);
    LONGS_EQUAL(0, config_file_option_has_changed (ptr_option_int));

    /* String */
    LONGS_EQUAL(0, config_file_option_has_changed (ptr_option_str));
    config_file_option_set (ptr_option_str, "test", 1);
    LONGS_EQUAL(1, config_file_option_has_changed (ptr_option_str));
    config_file_option_reset (ptr_option_str, 1);
    LONGS_EQUAL(0, config_file_option_has_changed (ptr_option_str));

    /* Color */
    LONGS_EQUAL(0, config_file_option_has_changed (ptr_option_col));
    config_file_option_set (ptr_option_col, "red", 1);
    LONGS_EQUAL(1, config_file_option_has_changed (ptr_option_col));
    config_file_option_reset (ptr_option_col, 1);
    LONGS_EQUAL(0, config_file_option_has_changed (ptr_option_col));

    /* Enum */
    LONGS_EQUAL(0, config_file_option_has_changed (ptr_option_enum));
    config_file_option_set (ptr_option_enum, "v3", 1);
    LONGS_EQUAL(1, config_file_option_has_changed (ptr_option_enum));
    config_file_option_reset (ptr_option_enum, 1);
    LONGS_EQUAL(0, config_file_option_has_changed (ptr_option_enum));

    /* Null default value and value */
    LONGS_EQUAL(0, config_file_option_has_changed (ptr_option_str_child));
    config_file_option_set (ptr_option_str_child, "test", 1);
    LONGS_EQUAL(1, config_file_option_has_changed (ptr_option_str_child));
    config_file_option_set_null (ptr_option_str_child, 1);
    LONGS_EQUAL(0, config_file_option_has_changed (ptr_option_str_child));

    /* Default value not null, value null */
    option = config_file_new_option (
        weechat_config_file, weechat_config_section_look,
        "test_null", "string", "", NULL, 0, 0, "value", "value", 1,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    CHECK(option);
    LONGS_EQUAL(0, config_file_option_has_changed (option));
    config_file_option_set_null (option, 1);
    LONGS_EQUAL(1, config_file_option_has_changed (option));
    config_file_option_free (option, 0);
}

/*
 * Test functions:
 *   config_file_option_set_with_string
 */

TEST(CoreConfigFileWithTestConfig, OptionSetWithString)
{
    struct t_config_section *section_add, *section_add_no_cb;
    struct t_config_option *opt_str, *ptr_option;

    opt_str = test_new_option (section_test, "str", "string", NULL,
                               "value", NULL, 0);
    CHECK(opt_str);
    section_add = config_file_new_section (
        config_test, "add", 1, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        &test_section_create_option_cb, NULL, NULL,
        NULL, NULL, NULL);
    CHECK(section_add);
    section_add_no_cb = test_new_section (config_test, "add_no_cb", 1, 0);
    CHECK(section_add_no_cb);

    /* Option not found */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND,
                config_file_option_set_with_string (NULL, NULL));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND,
                config_file_option_set_with_string ("", "value"));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND,
                config_file_option_set_with_string ("zzz", "value"));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND,
                config_file_option_set_with_string (TEST_CONFIG_NAME ".sec",
                                                    "value"));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND,
                config_file_option_set_with_string ("zzz.sec.str", "value"));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND,
                config_file_option_set_with_string (TEST_CONFIG_NAME ".zzz.str",
                                                    "value"));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND,
                config_file_option_set_with_string (TEST_CONFIG_NAME ".sec.zzz",
                                                    "value"));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND,
                config_file_option_set_with_string (
                    TEST_CONFIG_NAME ".add_no_cb.zzz", "value"));

    /* Existing option */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_with_string (TEST_CONFIG_NAME ".sec.str",
                                                    "test"));
    STRCMP_EQUAL("test", CONFIG_STRING(opt_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_with_string (TEST_CONFIG_NAME ".sec.str",
                                                    "test"));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_with_string (TEST_CONFIG_NAME ".sec.str",
                                                    NULL));
    STRCMP_EQUAL("test", CONFIG_STRING(opt_str));

    /* New option created by the section callback */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_with_string (TEST_CONFIG_NAME ".add.new",
                                                    "abc"));
    ptr_option = config_file_search_option (config_test, section_add, "new");
    CHECK(ptr_option);
    STRCMP_EQUAL("abc", CONFIG_STRING(ptr_option));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_with_string (TEST_CONFIG_NAME ".add.new",
                                                    "abc"));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_with_string (TEST_CONFIG_NAME ".add.new",
                                                    NULL));
    POINTERS_EQUAL(NULL, ptr_option->value);
}

/*
 * Test functions:
 *   config_file_option_boolean
 *   config_file_option_boolean_default
 */

TEST(CoreConfigFileWithNewOptions, OptionBoolean)
{
    LONGS_EQUAL(0, config_file_option_boolean (NULL));
    LONGS_EQUAL(0, config_file_option_boolean_default (NULL));

    LONGS_EQUAL(0, config_file_option_boolean (ptr_option_bool));
    LONGS_EQUAL(0, config_file_option_boolean_default (ptr_option_bool));

    config_file_option_set (ptr_option_bool, "on", 1);
    LONGS_EQUAL(1, config_file_option_boolean (ptr_option_bool));
    LONGS_EQUAL(0, config_file_option_boolean_default (ptr_option_bool));
    config_file_option_reset (ptr_option_bool, 1);
    LONGS_EQUAL(0, config_file_option_boolean (ptr_option_bool));
    LONGS_EQUAL(0, config_file_option_boolean_default (ptr_option_bool));

    LONGS_EQUAL(0, config_file_option_boolean (ptr_option_int));
    LONGS_EQUAL(0, config_file_option_boolean_default (ptr_option_int));
    LONGS_EQUAL(0, config_file_option_boolean (ptr_option_int_str));
    LONGS_EQUAL(0, config_file_option_boolean_default (ptr_option_int_str));
    LONGS_EQUAL(0, config_file_option_boolean (ptr_option_str));
    LONGS_EQUAL(0, config_file_option_boolean_default (ptr_option_str));
    LONGS_EQUAL(0, config_file_option_boolean (ptr_option_col));
    LONGS_EQUAL(0, config_file_option_boolean_default (ptr_option_col));
    LONGS_EQUAL(0, config_file_option_boolean (ptr_option_enum));
    LONGS_EQUAL(0, config_file_option_boolean_default (ptr_option_enum));
}

/*
 * Test functions:
 *   config_file_option_boolean_inherited
 */

TEST(CoreConfigFileWithNewOptions, OptionBooleanInherited)
{
    LONGS_EQUAL(0, config_file_option_boolean_inherited (NULL));

    LONGS_EQUAL(0, config_file_option_boolean_inherited (ptr_option_bool_child));
    config_file_option_set (ptr_option_bool, "on", 1);
    LONGS_EQUAL(1, config_file_option_boolean_inherited (ptr_option_bool_child));
    config_file_option_reset (ptr_option_bool, 1);
    LONGS_EQUAL(0, config_file_option_boolean_inherited (ptr_option_bool_child));

    /* Child option with its own value */
    config_file_option_set (ptr_option_bool_child, "on", 1);
    LONGS_EQUAL(1, config_file_option_boolean_inherited (ptr_option_bool_child));
    config_file_option_set_null (ptr_option_bool_child, 1);
    LONGS_EQUAL(0, config_file_option_boolean_inherited (ptr_option_bool_child));
}

/*
 * Test functions:
 *   config_file_option_integer
 *   config_file_option_integer_default
 */

TEST(CoreConfigFileWithNewOptions, OptionInteger)
{
    LONGS_EQUAL(0, config_file_option_integer (NULL));
    LONGS_EQUAL(0, config_file_option_integer_default (NULL));

    LONGS_EQUAL(100, config_file_option_integer (ptr_option_int));
    LONGS_EQUAL(100, config_file_option_integer_default (ptr_option_int));

    config_file_option_set (ptr_option_int, "123", 1);
    LONGS_EQUAL(123, config_file_option_integer (ptr_option_int));
    LONGS_EQUAL(100, config_file_option_integer_default (ptr_option_int));
    config_file_option_reset (ptr_option_int, 1);
    LONGS_EQUAL(100, config_file_option_integer (ptr_option_int));
    LONGS_EQUAL(100, config_file_option_integer_default (ptr_option_int));

    LONGS_EQUAL(0, config_file_option_integer (ptr_option_bool));
    LONGS_EQUAL(0, config_file_option_integer_default (ptr_option_bool));
    config_file_option_set (ptr_option_bool, "on", 1);
    LONGS_EQUAL(1, config_file_option_integer (ptr_option_bool));
    LONGS_EQUAL(0, config_file_option_integer_default (ptr_option_bool));
    config_file_option_reset (ptr_option_bool, 1);
    LONGS_EQUAL(1, config_file_option_integer (ptr_option_int_str));
    LONGS_EQUAL(1, config_file_option_integer_default (ptr_option_int_str));
    LONGS_EQUAL(0, config_file_option_integer (ptr_option_str));
    LONGS_EQUAL(0, config_file_option_integer_default (ptr_option_str));
    LONGS_EQUAL(9, config_file_option_integer (ptr_option_col));
    LONGS_EQUAL(9, config_file_option_integer_default (ptr_option_col));
    LONGS_EQUAL(1, config_file_option_integer (ptr_option_enum));
    LONGS_EQUAL(1, config_file_option_integer_default (ptr_option_enum));

    config_file_option_set_default (ptr_option_bool, "on", 1);
    LONGS_EQUAL(1, config_file_option_integer_default (ptr_option_bool));
}

/*
 * Test functions:
 *   config_file_option_integer_inherited
 */

TEST(CoreConfigFileWithNewOptions, OptionIntegerInherited)
{
    LONGS_EQUAL(0, config_file_option_integer_inherited (NULL));

    LONGS_EQUAL(100, config_file_option_integer_inherited (ptr_option_int_child));
    config_file_option_set (ptr_option_int, "123", 1);
    LONGS_EQUAL(123, config_file_option_integer_inherited (ptr_option_int_child));
    config_file_option_reset (ptr_option_int, 1);
    LONGS_EQUAL(100, config_file_option_integer_inherited (ptr_option_int_child));

    /* Child option with its own value */
    config_file_option_set (ptr_option_int_child, "50", 1);
    LONGS_EQUAL(50, config_file_option_integer_inherited (ptr_option_int_child));
    config_file_option_set_null (ptr_option_int_child, 1);
    LONGS_EQUAL(100, config_file_option_integer_inherited (ptr_option_int_child));
}

/*
 * Test functions:
 *   config_file_option_string
 *   config_file_option_string_default
 */

TEST(CoreConfigFileWithNewOptions, OptionString)
{
    STRCMP_EQUAL(NULL, config_file_option_string (NULL));
    STRCMP_EQUAL(NULL, config_file_option_string_default (NULL));

    STRCMP_EQUAL("v2", config_file_option_string (ptr_option_int_str));
    STRCMP_EQUAL("v2", config_file_option_string_default (ptr_option_int_str));

    STRCMP_EQUAL("value", config_file_option_string (ptr_option_str));
    STRCMP_EQUAL("value", config_file_option_string_default (ptr_option_str));

    config_file_option_set (ptr_option_int_str, "v3", 1);
    STRCMP_EQUAL("v3", config_file_option_string (ptr_option_int_str));
    STRCMP_EQUAL("v2", config_file_option_string_default (ptr_option_int_str));
    config_file_option_reset (ptr_option_int_str, 1);
    STRCMP_EQUAL("v2", config_file_option_string (ptr_option_int_str));
    STRCMP_EQUAL("v2", config_file_option_string_default (ptr_option_int_str));

    config_file_option_set (ptr_option_str, "test", 1);
    STRCMP_EQUAL("test", config_file_option_string (ptr_option_str));
    STRCMP_EQUAL("value", config_file_option_string_default (ptr_option_str));
    config_file_option_reset (ptr_option_str, 1);
    STRCMP_EQUAL("value", config_file_option_string (ptr_option_str));
    STRCMP_EQUAL("value", config_file_option_string_default (ptr_option_str));

    STRCMP_EQUAL("off", config_file_option_string (ptr_option_bool));
    STRCMP_EQUAL("off", config_file_option_string_default (ptr_option_bool));
    STRCMP_EQUAL(NULL, config_file_option_string (ptr_option_int));
    STRCMP_EQUAL(NULL, config_file_option_string_default (ptr_option_int));
    STRCMP_EQUAL("v2", config_file_option_string (ptr_option_int_str));
    STRCMP_EQUAL("v2", config_file_option_string_default (ptr_option_int_str));
    STRCMP_EQUAL("blue", config_file_option_string (ptr_option_col));
    STRCMP_EQUAL("blue", config_file_option_string_default (ptr_option_col));
    STRCMP_EQUAL("v2", config_file_option_string (ptr_option_enum));
    STRCMP_EQUAL("v2", config_file_option_string_default (ptr_option_enum));

    config_file_option_set (ptr_option_bool, "on", 1);
    config_file_option_set_default (ptr_option_bool, "on", 1);
    STRCMP_EQUAL("on", config_file_option_string (ptr_option_bool));
    STRCMP_EQUAL("on", config_file_option_string_default (ptr_option_bool));
}

/*
 * Test functions:
 *   config_file_option_string_inherited
 */

TEST(CoreConfigFileWithNewOptions, OptionStringInherited)
{
    STRCMP_EQUAL(NULL, config_file_option_string_inherited (NULL));

    STRCMP_EQUAL("v2", config_file_option_string_inherited (ptr_option_int_str_child));
    config_file_option_set (ptr_option_int_str, "v3", 1);
    STRCMP_EQUAL("v3", config_file_option_string_inherited (ptr_option_int_str_child));
    config_file_option_reset (ptr_option_int_str, 1);
    STRCMP_EQUAL("v2", config_file_option_string_inherited (ptr_option_int_str_child));

    STRCMP_EQUAL("value", config_file_option_string_inherited (ptr_option_str_child));
    config_file_option_set (ptr_option_str, "test", 1);
    STRCMP_EQUAL("test", config_file_option_string_inherited (ptr_option_str_child));
    config_file_option_reset (ptr_option_str, 1);
    STRCMP_EQUAL("value", config_file_option_string_inherited (ptr_option_str_child));

    /* Child option with its own value */
    config_file_option_set (ptr_option_str_child, "test", 1);
    STRCMP_EQUAL("test", config_file_option_string_inherited (ptr_option_str_child));
    config_file_option_set_null (ptr_option_str_child, 1);
    STRCMP_EQUAL("value", config_file_option_string_inherited (ptr_option_str_child));
}

/*
 * Test functions:
 *   config_file_option_color
 *   config_file_option_color_default
 */

TEST(CoreConfigFileWithNewOptions, OptionColor)
{
    STRCMP_EQUAL(NULL, config_file_option_color (NULL));
    STRCMP_EQUAL(NULL, config_file_option_color_default (NULL));

    STRCMP_EQUAL("blue", config_file_option_color (ptr_option_col));
    STRCMP_EQUAL("blue", config_file_option_color_default (ptr_option_col));

    config_file_option_set (ptr_option_col, "red", 1);
    STRCMP_EQUAL("red", config_file_option_color (ptr_option_col));
    STRCMP_EQUAL("blue", config_file_option_color_default (ptr_option_col));
    config_file_option_reset (ptr_option_col, 1);
    STRCMP_EQUAL("blue", config_file_option_color (ptr_option_col));
    STRCMP_EQUAL("blue", config_file_option_color_default (ptr_option_col));

    STRCMP_EQUAL(NULL, config_file_option_color (ptr_option_bool));
    STRCMP_EQUAL(NULL, config_file_option_color_default (ptr_option_bool));
    STRCMP_EQUAL(NULL, config_file_option_color (ptr_option_int));
    STRCMP_EQUAL(NULL, config_file_option_color_default (ptr_option_int));
    STRCMP_EQUAL(NULL, config_file_option_color (ptr_option_int_str));
    STRCMP_EQUAL(NULL, config_file_option_color_default (ptr_option_int_str));
    STRCMP_EQUAL(NULL, config_file_option_color (ptr_option_str));
    STRCMP_EQUAL(NULL, config_file_option_color_default (ptr_option_str));
    STRCMP_EQUAL(NULL, config_file_option_color (ptr_option_enum));
    STRCMP_EQUAL(NULL, config_file_option_color_default (ptr_option_enum));
}

/*
 * Test functions:
 *   config_file_option_color_inherited
 */

TEST(CoreConfigFileWithNewOptions, OptionColorInherited)
{
    STRCMP_EQUAL(NULL, config_file_option_color_inherited (NULL));

    STRCMP_EQUAL("blue", config_file_option_color_inherited (ptr_option_col_child));
    config_file_option_set (ptr_option_col, "red", 1);
    STRCMP_EQUAL("red", config_file_option_color_inherited (ptr_option_col_child));
    config_file_option_reset (ptr_option_col, 1);
    STRCMP_EQUAL("blue", config_file_option_color_inherited (ptr_option_col_child));

    /* Child option with its own value */
    config_file_option_set (ptr_option_col_child, "red", 1);
    STRCMP_EQUAL("red", config_file_option_color_inherited (ptr_option_col_child));
    config_file_option_set_null (ptr_option_col_child, 1);
    STRCMP_EQUAL("blue", config_file_option_color_inherited (ptr_option_col_child));
}

/*
 * Test functions:
 *   config_file_option_enum
 *   config_file_option_enum_default
 */

TEST(CoreConfigFileWithNewOptions, OptionEnum)
{
    LONGS_EQUAL(0, config_file_option_enum (NULL));
    LONGS_EQUAL(0, config_file_option_enum_default (NULL));

    LONGS_EQUAL(1, config_file_option_enum (ptr_option_enum));
    LONGS_EQUAL(1, config_file_option_enum_default (ptr_option_enum));

    config_file_option_set (ptr_option_enum, "v3", 1);
    LONGS_EQUAL(2, config_file_option_enum (ptr_option_enum));
    LONGS_EQUAL(1, config_file_option_enum_default (ptr_option_enum));
    config_file_option_reset (ptr_option_enum, 1);
    LONGS_EQUAL(1, config_file_option_enum (ptr_option_enum));
    LONGS_EQUAL(1, config_file_option_enum_default (ptr_option_enum));

    LONGS_EQUAL(0, config_file_option_enum (ptr_option_bool));
    LONGS_EQUAL(0, config_file_option_enum_default (ptr_option_bool));
    config_file_option_set (ptr_option_bool, "on", 1);
    LONGS_EQUAL(1, config_file_option_enum (ptr_option_bool));
    LONGS_EQUAL(0, config_file_option_enum_default (ptr_option_bool));
    config_file_option_reset (ptr_option_bool, 1);
    LONGS_EQUAL(100, config_file_option_enum (ptr_option_int));
    LONGS_EQUAL(100, config_file_option_enum_default (ptr_option_int));
    LONGS_EQUAL(1, config_file_option_enum (ptr_option_int_str));
    LONGS_EQUAL(1, config_file_option_enum_default (ptr_option_int_str));
    LONGS_EQUAL(0, config_file_option_enum (ptr_option_str));
    LONGS_EQUAL(0, config_file_option_enum_default (ptr_option_str));
    LONGS_EQUAL(9, config_file_option_enum (ptr_option_col));
    LONGS_EQUAL(9, config_file_option_enum_default (ptr_option_col));
    LONGS_EQUAL(1, config_file_option_enum (ptr_option_enum));
    LONGS_EQUAL(1, config_file_option_enum_default (ptr_option_enum));

    config_file_option_set_default (ptr_option_bool, "on", 1);
    LONGS_EQUAL(1, config_file_option_enum_default (ptr_option_bool));
}

/*
 * Test functions:
 *   config_file_option_enum_inherited
 */

TEST(CoreConfigFileWithNewOptions, OptionEnumInherited)
{
    LONGS_EQUAL(0, config_file_option_enum_inherited (NULL));

    LONGS_EQUAL(1, config_file_option_enum_inherited (ptr_option_enum_child));
    config_file_option_set (ptr_option_enum, "v3", 1);
    LONGS_EQUAL(2, config_file_option_enum_inherited (ptr_option_enum_child));
    config_file_option_reset (ptr_option_enum, 1);
    LONGS_EQUAL(1, config_file_option_enum_inherited (ptr_option_enum_child));

    /* Child option with its own value */
    config_file_option_set (ptr_option_enum_child, "v3", 1);
    LONGS_EQUAL(2, config_file_option_enum_inherited (ptr_option_enum_child));
    config_file_option_set_null (ptr_option_enum_child, 1);
    LONGS_EQUAL(1, config_file_option_enum_inherited (ptr_option_enum_child));
}

/*
 * Test functions:
 *   config_file_option_boolean_inherited (parent with null value)
 *   config_file_option_integer_inherited (parent with null value)
 *   config_file_option_string_inherited (parent with null value)
 *   config_file_option_color_inherited (parent with null value)
 *   config_file_option_enum_inherited (parent with null value)
 */

TEST(CoreConfigFileWithTestConfig, OptionInheritedNullParentValue)
{
    struct t_config_option *opt_bool, *opt_int, *opt_str, *opt_col, *opt_enum;
    struct t_config_option *opt_bool_child, *opt_int_child, *opt_str_child;
    struct t_config_option *opt_col_child, *opt_enum_child;

    /* Parent options: default value but null value */
    opt_bool = test_new_option (section_test, "bool", "boolean", NULL,
                                "on", NULL, 1);
    opt_int = test_new_option (section_test, "int", "integer", NULL,
                               "42", NULL, 1);
    opt_str = test_new_option (section_test, "str", "string", NULL,
                               "def", NULL, 1);
    opt_col = test_new_option (section_test, "col", "color", NULL,
                               "red", NULL, 1);
    opt_enum = test_new_option (section_test, "enum", "enum", "v1|v2|v3",
                                "v3", NULL, 1);
    CHECK(opt_bool);
    CHECK(opt_int);
    CHECK(opt_str);
    CHECK(opt_col);
    CHECK(opt_enum);
    POINTERS_EQUAL(NULL, opt_bool->value);
    POINTERS_EQUAL(NULL, opt_int->value);
    POINTERS_EQUAL(NULL, opt_str->value);
    POINTERS_EQUAL(NULL, opt_col->value);
    POINTERS_EQUAL(NULL, opt_enum->value);

    /* Child options: null default value and value */
    opt_bool_child = test_new_option (
        section_test, "bool_child << " TEST_CONFIG_NAME ".sec.bool",
        "boolean", NULL, NULL, NULL, 1);
    opt_int_child = test_new_option (
        section_test, "int_child << " TEST_CONFIG_NAME ".sec.int",
        "integer", NULL, NULL, NULL, 1);
    opt_str_child = test_new_option (
        section_test, "str_child << " TEST_CONFIG_NAME ".sec.str",
        "string", NULL, NULL, NULL, 1);
    opt_col_child = test_new_option (
        section_test, "col_child << " TEST_CONFIG_NAME ".sec.col",
        "color", NULL, NULL, NULL, 1);
    opt_enum_child = test_new_option (
        section_test, "enum_child << " TEST_CONFIG_NAME ".sec.enum",
        "enum", "v1|v2|v3", NULL, NULL, 1);
    CHECK(opt_bool_child);
    CHECK(opt_int_child);
    CHECK(opt_str_child);
    CHECK(opt_col_child);
    CHECK(opt_enum_child);

    /* Default value of parent is returned */
    LONGS_EQUAL(1, config_file_option_boolean_inherited (opt_bool_child));
    LONGS_EQUAL(42, config_file_option_integer_inherited (opt_int_child));
    STRCMP_EQUAL("def", config_file_option_string_inherited (opt_str_child));
    STRCMP_EQUAL("red", config_file_option_color_inherited (opt_col_child));
    LONGS_EQUAL(2, config_file_option_enum_inherited (opt_enum_child));
}

/*
 * Test functions:
 *   config_file_option_escape
 */

TEST(CoreConfigFile, OptionEscape)
{
    STRCMP_EQUAL("\\", config_file_option_escape (NULL));

    STRCMP_EQUAL("", config_file_option_escape (""));
    STRCMP_EQUAL("", config_file_option_escape ("test"));
    STRCMP_EQUAL("", config_file_option_escape ("|test"));
    STRCMP_EQUAL("", config_file_option_escape ("]test"));

    STRCMP_EQUAL("\\", config_file_option_escape ("#test"));
    STRCMP_EQUAL("\\", config_file_option_escape ("[test"));
    STRCMP_EQUAL("\\", config_file_option_escape ("\\test"));
}

/*
 * Test functions:
 *   config_file_write_option
 */

TEST(CoreConfigFileWithTestConfig, WriteOption)
{
    struct t_config_option *opt_bool, *opt_int, *opt_str, *opt_str_null;
    struct t_config_option *opt_col, *opt_enum, *opt_escaped;
    char *path, *content;

    opt_bool = test_new_option (section_test, "bool", "boolean", NULL,
                                "on", NULL, 0);
    opt_int = test_new_option (section_test, "int", "integer", NULL,
                               "42", NULL, 0);
    opt_str = test_new_option (section_test, "str", "string", NULL,
                               "value", NULL, 0);
    opt_str_null = test_new_option (section_test, "str_null", "string", NULL,
                                    NULL, NULL, 1);
    opt_col = test_new_option (section_test, "col", "color", NULL,
                               "red", NULL, 0);
    opt_enum = test_new_option (section_test, "enum", "enum", "v1|v2|v3",
                                "v2", NULL, 0);
    opt_escaped = test_new_option (section_test, "#escaped", "string", NULL,
                                   "x", NULL, 0);
    CHECK(opt_bool);
    CHECK(opt_int);
    CHECK(opt_str);
    CHECK(opt_str_null);
    CHECK(opt_col);
    CHECK(opt_enum);
    CHECK(opt_escaped);

    LONGS_EQUAL(0, config_file_write_option (NULL, NULL));
    LONGS_EQUAL(0, config_file_write_option (NULL, opt_bool));
    LONGS_EQUAL(0, config_file_write_option (config_test, NULL));

    /* File not opened */
    LONGS_EQUAL(0, config_file_write_option (config_test, opt_bool));

    path = test_config_get_path (TEST_CONFIG_NAME ".conf");
    config_test->file = fopen (path, "w");
    CHECK(config_test->file);
    LONGS_EQUAL(1, config_file_write_option (config_test, opt_bool));
    LONGS_EQUAL(1, config_file_write_option (config_test, opt_int));
    LONGS_EQUAL(1, config_file_write_option (config_test, opt_str));
    LONGS_EQUAL(1, config_file_write_option (config_test, opt_str_null));
    LONGS_EQUAL(1, config_file_write_option (config_test, opt_col));
    LONGS_EQUAL(1, config_file_write_option (config_test, opt_enum));
    LONGS_EQUAL(1, config_file_write_option (config_test, opt_escaped));
    fclose (config_test->file);
    config_test->file = NULL;
    free (path);

    content = test_config_read_file (TEST_CONFIG_NAME ".conf");
    STRCMP_EQUAL("bool = on\n"
                 "int = 42\n"
                 "str = \"value\"\n"
                 "str_null\n"
                 "col = red\n"
                 "enum = v2\n"
                 "\\#escaped = \"x\"\n",
                 content);
    free (content);
}

/*
 * Test functions:
 *   config_file_write_line
 */

TEST(CoreConfigFileWithTestConfig, WriteLine)
{
    char *path, *content;

    LONGS_EQUAL(0, config_file_write_line (NULL, NULL, NULL));
    LONGS_EQUAL(0, config_file_write_line (NULL, "sec", NULL));
    LONGS_EQUAL(0, config_file_write_line (config_test, NULL, NULL));

    /* File not opened */
    LONGS_EQUAL(0, config_file_write_line (config_test, "sec", NULL));
    LONGS_EQUAL(0, config_file_write_line (config_test, "opt", "value"));

    path = test_config_get_path (TEST_CONFIG_NAME ".conf");
    config_test->file = fopen (path, "w");
    CHECK(config_test->file);
    LONGS_EQUAL(1, config_file_write_line (config_test, "sec1", NULL));
    LONGS_EQUAL(1, config_file_write_line (config_test, "sec2", ""));
    LONGS_EQUAL(1, config_file_write_line (config_test, "sec3", "%s", ""));
    LONGS_EQUAL(1, config_file_write_line (config_test, "opt1", "%d", 42));
    LONGS_EQUAL(1, config_file_write_line (config_test, "opt2", "\"%s\"",
                                           "value"));
    LONGS_EQUAL(1, config_file_write_line (config_test, "[opt3", "on"));
    fclose (config_test->file);
    config_test->file = NULL;
    free (path);

    content = test_config_read_file (TEST_CONFIG_NAME ".conf");
    STRCMP_EQUAL("\n[sec1]\n"
                 "\n[sec2]\n"
                 "\n[sec3]\n"
                 "opt1 = 42\n"
                 "opt2 = \"value\"\n"
                 "\\[opt3 = on\n",
                 content);
    free (content);
}

/*
 * Test functions:
 *   config_file_write_internal
 */

TEST(CoreConfigFileWithTestConfig, WriteInternal)
{
    struct t_config_section *section_custom, *section_error;
    struct t_config_option *opt_bool, *opt_int;
    char *path, *path_tmp, *path_target, *content;
    struct stat st;

    LONGS_EQUAL(WEECHAT_CONFIG_WRITE_ERROR,
                config_file_write_internal (NULL, 0));
    LONGS_EQUAL(WEECHAT_CONFIG_WRITE_ERROR,
                config_file_write_internal (NULL, 1));

    opt_bool = test_new_option (section_test, "bool", "boolean", NULL,
                                "on", NULL, 0);
    opt_int = test_new_option (section_test, "int", "integer", NULL,
                               "42", NULL, 0);
    CHECK(opt_bool);
    CHECK(opt_int);
    section_custom = config_file_new_section (
        config_test, "custom", 0, 0,
        NULL, NULL, NULL,
        &test_section_write_cb, NULL, NULL,
        &test_section_write_default_cb, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    CHECK(section_custom);

    path = test_config_get_path (TEST_CONFIG_NAME ".conf");
    string_asprintf (&path_tmp, "%s.weechattmp", path);

    /* Default options, version 1 (no config version written) */
    LONGS_EQUAL(WEECHAT_CONFIG_WRITE_OK,
                config_file_write_internal (config_test, 1));
    POINTERS_EQUAL(NULL, config_test->file);
    LONGS_EQUAL(-1, access (path_tmp, F_OK));
    content = test_config_read_file (TEST_CONFIG_NAME ".conf");
    CHECK(content);
    CHECK(strncmp (content, "#\n# ", 4) == 0);
    CHECK(strstr (content, " -- " TEST_CONFIG_NAME ".conf\n"));
    POINTERS_EQUAL(NULL, strstr (content, "config_version"));
    CHECK(strstr (content, "\n[sec]\nbool = on\nint = 42\n"));
    CHECK(strstr (content, "\n[custom]\ncustom_default = \"default\"\n"));
    POINTERS_EQUAL(NULL, strstr (content, "custom = "));
    free (content);

    /* Permissions are set on file (default is "600") */
    LONGS_EQUAL(0, stat (path, &st));
    LONGS_EQUAL(0600, st.st_mode & 0777);

    /* Current options, version 2 */
    config_file_set_version (config_test, 2, NULL, NULL, NULL);
    config_file_option_set (opt_int, "50", 1);
    LONGS_EQUAL(WEECHAT_CONFIG_WRITE_OK,
                config_file_write_internal (config_test, 0));
    content = test_config_read_file (TEST_CONFIG_NAME ".conf");
    CHECK(content);
    CHECK(strstr (content, "\nconfig_version = 2\n"));
    CHECK(strstr (content, "\n[sec]\nbool = on\nint = 50\n"));
    CHECK(strstr (content, "\n[custom]\ncustom = \"value\"\n"));
    POINTERS_EQUAL(NULL, strstr (content, "custom_default"));
    free (content);

    /* No changes: temporary file is removed */
    LONGS_EQUAL(WEECHAT_CONFIG_WRITE_OK,
                config_file_write_internal (config_test, 0));
    POINTERS_EQUAL(NULL, config_test->file);
    LONGS_EQUAL(-1, access (path_tmp, F_OK));

    /* Error in write callback of a section: file is not updated */
    config_file_option_set (opt_int, "60", 1);
    section_error = config_file_new_section (
        config_test, "error", 0, 0,
        NULL, NULL, NULL,
        &test_section_write_cb, (void *)0x1, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    CHECK(section_error);
    record_start ();
    LONGS_EQUAL(WEECHAT_CONFIG_WRITE_ERROR,
                config_file_write_internal (config_test, 0));
    record_stop ();
    TEST_CHECK_MSG_REGEX("Error writing configuration file");
    POINTERS_EQUAL(NULL, config_test->file);
    LONGS_EQUAL(-1, access (path_tmp, F_OK));
    content = test_config_read_file (TEST_CONFIG_NAME ".conf");
    CHECK(content);
    CHECK(strstr (content, "\nint = 50\n"));
    free (content);
    config_file_section_free (section_error);

    /* Temporary file can not be created (a directory has the same name) */
    LONGS_EQUAL(0, mkdir (path_tmp, 0700));
    record_start ();
    LONGS_EQUAL(WEECHAT_CONFIG_WRITE_ERROR,
                config_file_write_internal (config_test, 0));
    record_stop ();
    TEST_CHECK_MSG_REGEX("^Cannot create file \".*" TEST_CONFIG_NAME
                         "\\.conf\\.weechattmp\"$");
    TEST_CHECK_MSG_REGEX("Error writing configuration file");
    POINTERS_EQUAL(NULL, config_test->file);
    LONGS_EQUAL(0, rmdir (path_tmp));

    /* Custom permissions and write with fsync */
    config_file_option_set (config_look_config_permissions, "640", 1);
    config_file_option_set (config_look_save_config_with_fsync, "on", 1);
    LONGS_EQUAL(WEECHAT_CONFIG_WRITE_OK,
                config_file_write_internal (config_test, 0));
    config_file_option_reset (config_look_config_permissions, 1);
    config_file_option_reset (config_look_save_config_with_fsync, 1);
    LONGS_EQUAL(0, stat (path, &st));
    LONGS_EQUAL(0640, st.st_mode & 0777);
    content = test_config_read_file (TEST_CONFIG_NAME ".conf");
    CHECK(content);
    CHECK(strstr (content, "\nint = 60\n"));
    free (content);

    /* Config file is a symbolic link: the target is written */
    path_target = test_config_get_path (TEST_CONFIG_NAME "_target.conf");
    test_config_write_file (TEST_CONFIG_NAME "_target.conf", "");
    unlink (path);
    LONGS_EQUAL(0, symlink (TEST_CONFIG_NAME "_target.conf", path));
    LONGS_EQUAL(WEECHAT_CONFIG_WRITE_OK,
                config_file_write_internal (config_test, 0));
    LONGS_EQUAL(0, lstat (path, &st));
    CHECK(S_ISLNK(st.st_mode));
    content = test_config_read_file (TEST_CONFIG_NAME "_target.conf");
    CHECK(content);
    CHECK(strstr (content, "\nint = 60\n"));
    free (content);
    unlink (path_target);
    free (path_target);

    free (path);
    free (path_tmp);
}

/*
 * Test functions:
 *   config_file_write
 */

TEST(CoreConfigFileWithTestConfig, Write)
{
    struct t_config_option *opt_int;
    char *content;

    LONGS_EQUAL(WEECHAT_CONFIG_WRITE_ERROR, config_file_write (NULL));

    opt_int = test_new_option (section_test, "int", "integer", NULL,
                               "42", NULL, 0);
    CHECK(opt_int);
    config_file_option_set (opt_int, "50", 1);

    LONGS_EQUAL(WEECHAT_CONFIG_WRITE_OK, config_file_write (config_test));
    content = test_config_read_file (TEST_CONFIG_NAME ".conf");
    CHECK(content);
    CHECK(strstr (content, "\n[sec]\nint = 50\n"));
    free (content);
}

/*
 * Test functions:
 *   config_file_parse_version
 */

TEST(CoreConfigFile, ParseVersion)
{
    LONGS_EQUAL(-1, config_file_parse_version (NULL));
    LONGS_EQUAL(-1, config_file_parse_version (""));
    LONGS_EQUAL(-1, config_file_parse_version ("abc"));
    LONGS_EQUAL(-1, config_file_parse_version ("1abc"));
    LONGS_EQUAL(-1, config_file_parse_version ("0"));
    LONGS_EQUAL(-1, config_file_parse_version ("-1"));

    LONGS_EQUAL(1, config_file_parse_version ("1"));
    LONGS_EQUAL(2, config_file_parse_version ("2"));
    LONGS_EQUAL(123, config_file_parse_version ("123"));
}

/*
 * Test functions:
 *   config_file_backup
 */

TEST(CoreConfigFile, Backup)
{
    char *path;

    config_file_backup (NULL);

    /* Backup of an existing file (twice) */
    test_config_write_file (TEST_CONFIG_NAME "_backup.conf", "test\n");
    path = test_config_get_path (TEST_CONFIG_NAME "_backup.conf");
    record_start ();
    config_file_backup (path);
    config_file_backup (path);
    record_stop ();
    TEST_CHECK_MSG_REGEX("^File .*" TEST_CONFIG_NAME "_backup\\.conf "
                         "has been backed up as .*" TEST_CONFIG_NAME
                         "_backup\\.conf\\.backup\\.[0-9]{8}\\.[0-9]{6}$");
    LONGS_EQUAL(2, test_config_remove_backup_files (TEST_CONFIG_NAME
                                                    "_backup.conf"));
    unlink (path);
    free (path);

    /* Backup of a file that does not exist */
    path = test_config_get_path (TEST_CONFIG_NAME "_missing.conf");
    record_start ();
    config_file_backup (path);
    record_stop ();
    TEST_CHECK_MSG_REGEX("^Error: unable to backup file .*"
                         TEST_CONFIG_NAME "_missing\\.conf$");
    LONGS_EQUAL(0, test_config_remove_backup_files (TEST_CONFIG_NAME
                                                    "_missing.conf"));
    free (path);
}

/*
 * Test functions:
 *   config_file_update_data_read
 */

TEST(CoreConfigFileWithTestConfig, UpdateDataRead)
{
    char *section, *option, *value;
    int warning;

    section = strdup ("old_sec");
    warning = 0;

    /* Config is already the latest version: nothing is done */
    config_test->version_read = 1;
    record_start ();
    config_file_update_data_read (config_test, "test.conf",
                                  "old_sec", NULL, NULL,
                                  &section, NULL, NULL, &warning);
    record_stop ();
    RECORD_CHECK_NO_MSG();
    LONGS_EQUAL(0, warning);
    STRCMP_EQUAL("old_sec", section);

    /* Newer version without update callback: warning only */
    config_file_set_version (config_test, 2, NULL, NULL, NULL);
    record_start ();
    config_file_update_data_read (config_test, "test.conf",
                                  "old_sec", NULL, NULL,
                                  &section, NULL, NULL, &warning);
    record_stop ();
    TEST_CHECK_MSG_REGEX("^Important: file test\\.conf has been updated "
                         "from version 1 to 2");
    LONGS_EQUAL(1, warning);
    STRCMP_EQUAL("old_sec", section);

    /* Warning is displayed only once */
    record_start ();
    config_file_update_data_read (config_test, "test.conf",
                                  "old_sec", NULL, NULL,
                                  &section, NULL, NULL, &warning);
    record_stop ();
    RECORD_CHECK_NO_MSG();

    config_file_set_version (config_test, 2, &test_config_update_cb,
                             NULL, NULL);

    /* Section renamed by the callback */
    config_file_update_data_read (config_test, "test.conf",
                                  "old_sec", NULL, NULL,
                                  &section, NULL, NULL, &warning);
    LONGS_EQUAL(1, test_update_cb_count);
    STRCMP_EQUAL("sec", section);
    free (section);

    /* Section not changed by the callback */
    section = strdup ("other");
    config_file_update_data_read (config_test, "test.conf",
                                  "other", NULL, NULL,
                                  &section, NULL, NULL, &warning);
    LONGS_EQUAL(2, test_update_cb_count);
    STRCMP_EQUAL("other", section);
    free (section);

    /* Option renamed by the callback */
    option = strdup ("old_name");
    value = strdup ("5");
    config_file_update_data_read (config_test, "test.conf",
                                  "sec", "old_name", "5",
                                  NULL, &option, &value, &warning);
    LONGS_EQUAL(3, test_update_cb_count);
    STRCMP_EQUAL("int", option);
    STRCMP_EQUAL("5", value);
    free (option);
    free (value);

    /* Option ignored by the callback (empty name) */
    option = strdup ("drop");
    value = strdup ("1");
    config_file_update_data_read (config_test, "test.conf",
                                  "sec", "drop", "1",
                                  NULL, &option, &value, &warning);
    STRCMP_EQUAL("", option);
    STRCMP_EQUAL("1", value);
    free (option);
    free (value);

    /* Value set to null by the callback */
    option = strdup ("str_null");
    value = strdup ("abc");
    config_file_update_data_read (config_test, "test.conf",
                                  "sec", "str_null", "abc",
                                  NULL, &option, &value, &warning);
    STRCMP_EQUAL("str_null", option);
    POINTERS_EQUAL(NULL, value);
    free (option);

    /* Null value replaced by the callback (in a new hashtable) */
    option = strdup ("set_value");
    value = NULL;
    config_file_update_data_read (config_test, "test.conf",
                                  "sec", "set_value", NULL,
                                  NULL, &option, &value, &warning);
    STRCMP_EQUAL("set_value", option);
    STRCMP_EQUAL("xyz", value);
    free (option);
    free (value);

    /* Option not changed by the callback */
    option = strdup ("other");
    value = strdup ("1");
    config_file_update_data_read (config_test, "test.conf",
                                  "sec", "other", "1",
                                  NULL, &option, &value, &warning);
    STRCMP_EQUAL("other", option);
    STRCMP_EQUAL("1", value);
    free (option);
    free (value);

    /* Version read is the latest: callback is not called */
    config_test->version_read = 2;
    option = strdup ("old_name");
    value = strdup ("5");
    config_file_update_data_read (config_test, "test.conf",
                                  "sec", "old_name", "5",
                                  NULL, &option, &value, &warning);
    LONGS_EQUAL(7, test_update_cb_count);
    STRCMP_EQUAL("old_name", option);
    free (option);
    free (value);
}

/*
 * Test functions:
 *   config_file_read_internal
 *   config_file_read
 */

TEST(CoreConfigFileWithTestConfig, Read)
{
    struct t_config_section *section_cb;
    struct t_config_option *opt_bool, *opt_int, *opt_str, *opt_str_null;
    struct t_config_option *opt_col, *opt_enum, *opt_escaped;
    char *path;

    LONGS_EQUAL(WEECHAT_CONFIG_READ_FILE_NOT_FOUND, config_file_read (NULL));
    LONGS_EQUAL(WEECHAT_CONFIG_READ_FILE_NOT_FOUND,
                config_file_read_internal (NULL, 0));

    opt_bool = test_new_option (section_test, "bool", "boolean", NULL,
                                "off", NULL, 0);
    opt_int = test_new_option (section_test, "int", "integer", NULL,
                               "10", NULL, 0);
    opt_str = test_new_option (section_test, "str", "string", NULL,
                               "value", NULL, 0);
    opt_str_null = test_new_option (section_test, "str_null", "string", NULL,
                                    "def", "val", 1);
    opt_col = test_new_option (section_test, "col", "color", NULL,
                               "blue", NULL, 0);
    opt_enum = test_new_option (section_test, "enum", "enum", "v1|v2|v3",
                                "v1", NULL, 0);
    opt_escaped = test_new_option (section_test, "#escaped", "string", NULL,
                                   "x", NULL, 0);
    CHECK(opt_bool);
    CHECK(opt_int);
    CHECK(opt_str);
    CHECK(opt_str_null);
    CHECK(opt_col);
    CHECK(opt_enum);
    CHECK(opt_escaped);
    section_cb = config_file_new_section (
        config_test, "cb", 0, 0,
        &test_section_read_cb, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    CHECK(section_cb);

    /* File not found: it is created with default options */
    path = test_config_get_path (TEST_CONFIG_NAME ".conf");
    LONGS_EQUAL(-1, access (path, F_OK));
    LONGS_EQUAL(WEECHAT_CONFIG_READ_OK, config_file_read (config_test));
    LONGS_EQUAL(0, access (path, F_OK));

    /* File can not be opened (skipped as root, who can read the file) */
    if (geteuid () != 0)
    {
        LONGS_EQUAL(0, chmod (path, 0000));
        record_start ();
        LONGS_EQUAL(WEECHAT_CONFIG_READ_FILE_NOT_FOUND,
                    config_file_read (config_test));
        record_stop ();
        LONGS_EQUAL(0, chmod (path, 0600));
        TEST_CHECK_MSG_REGEX("^WARNING: failed to read configuration file "
                             "\".*" TEST_CONFIG_NAME "\\.conf\" "
                             "\\(Permission denied\\)$");
        TEST_CHECK_MSG_REGEX("^WARNING: file \".*" TEST_CONFIG_NAME
                             "\\.conf\" will be overwritten on exit");
    }
    free (path);

    /* Read file with valid and invalid lines */
    test_config_write_file (
        TEST_CONFIG_NAME ".conf",
        "#\n"                                   /* 1 */
        "# comment\n"                           /* 2 */
        "#\n"                                   /* 3 */
        "\n"                                    /* 4 */
        "config_version = 1\n"                  /* 5 */
        "outside = 1\n"                         /* 6 */
        "\n"                                    /* 7 */
        "[sec]\n"                               /* 8 */
        "bool = on\n"                           /* 9 */
        "  int   =   123  \n"                   /* 10 */
        "str = \"quoted value\"\n"              /* 11 */
        "str_null = null\n"                     /* 12 */
        "col = 'red'\n"                         /* 13 */
        "enum = v3\r\n"                         /* 14 */
        "\\#escaped = \"y\"\n"                  /* 15 */
        "unknown = 1\n"                         /* 16 */
        "bool = invalid\n"                      /* 17 */
        "[zzz]\n"                               /* 18 */
        "foo = bar\n"                           /* 19 */
        "[sec\n"                                /* 20 */
        "[cb]\n"                                /* 21 */
        "opt1 = val1\n"                         /* 22 */
        "opt2\n");                              /* 23 */
    record_start ();
    LONGS_EQUAL(WEECHAT_CONFIG_READ_OK, config_file_read (config_test));
    record_stop ();
    LONGS_EQUAL(1, config_test->version_read);
    LONGS_EQUAL(1, CONFIG_BOOLEAN(opt_bool));
    LONGS_EQUAL(123, CONFIG_INTEGER(opt_int));
    STRCMP_EQUAL("quoted value", CONFIG_STRING(opt_str));
    POINTERS_EQUAL(NULL, opt_str_null->value);
    STRCMP_EQUAL("red", config_file_option_color (opt_col));
    LONGS_EQUAL(2, CONFIG_ENUM(opt_enum));
    STRCMP_EQUAL("y", CONFIG_STRING(opt_escaped));
    LONGS_EQUAL(2, test_section_read_cb_count);
    STRCMP_EQUAL("opt2", test_section_read_cb_option);
    POINTERS_EQUAL(NULL, test_section_read_cb_value);
    TEST_CHECK_MSG_REGEX("line 6: ignoring option outside section: "
                         "outside = 1$");
    TEST_CHECK_MSG_REGEX("line 16: ignoring unknown option for section "
                         "\"sec\": unknown = 1$");
    TEST_CHECK_MSG_REGEX("line 17: ignoring invalid value for option in "
                         "section \"sec\": bool = invalid$");
    TEST_CHECK_MSG_REGEX("line 18: ignoring unknown section identifier "
                         "\\(\"zzz\"\\)$");
    TEST_CHECK_MSG_REGEX("line 19: ignoring option outside section: "
                         "foo = bar$");
    TEST_CHECK_MSG_REGEX("line 20: invalid syntax, missing");
    LONGS_EQUAL(6, record_count_messages ());

    /* Version read is newer than supported version: rest of file ignored */
    config_file_option_reset (opt_int, 1);
    config_file_set_version (config_test, 2, NULL, NULL, NULL);
    test_config_write_file (TEST_CONFIG_NAME ".conf",
                            "config_version = 3\n"
                            "[sec]\n"
                            "int = 500\n");
    record_start ();
    LONGS_EQUAL(WEECHAT_CONFIG_READ_OK, config_file_read (config_test));
    record_stop ();
    TEST_CHECK_MSG_REGEX("version read \\(3\\) is newer than supported "
                         "version \\(2\\)");
    LONGS_EQUAL(10, CONFIG_INTEGER(opt_int));
    LONGS_EQUAL(1, test_config_remove_backup_files (TEST_CONFIG_NAME ".conf"));

    /* Invalid version: rest of file ignored */
    test_config_write_file (TEST_CONFIG_NAME ".conf",
                            "config_version = abc\n"
                            "[sec]\n"
                            "int = 500\n");
    record_start ();
    LONGS_EQUAL(WEECHAT_CONFIG_READ_OK, config_file_read (config_test));
    record_stop ();
    TEST_CHECK_MSG_REGEX("line 1: invalid config version: "
                         "\"config_version = abc\"");
    LONGS_EQUAL(10, CONFIG_INTEGER(opt_int));
    LONGS_EQUAL(1, test_config_remove_backup_files (TEST_CONFIG_NAME ".conf"));

    /* Older version: data is updated by the callback */
    config_file_option_set (opt_str_null, "val", 1);
    config_file_set_version (config_test, 2, &test_config_update_cb,
                             NULL, NULL);
    test_config_write_file (TEST_CONFIG_NAME ".conf",
                            "[old_sec]\n"
                            "old_name = 77\n"
                            "drop = 1\n"
                            "str_null = abc\n");
    record_start ();
    LONGS_EQUAL(WEECHAT_CONFIG_READ_OK, config_file_read (config_test));
    record_stop ();
    TEST_CHECK_MSG_REGEX("^Important: file .*" TEST_CONFIG_NAME "\\.conf "
                         "has been updated from version 1 to 2");
    LONGS_EQUAL(1, record_count_messages ());
    LONGS_EQUAL(4, test_update_cb_count);
    LONGS_EQUAL(77, CONFIG_INTEGER(opt_int));
    POINTERS_EQUAL(NULL, opt_str_null->value);
}

/*
 * Test functions:
 *   config_file_reload
 */

TEST(CoreConfigFileWithTestConfig, Reload)
{
    struct t_config_section *section_cb;
    struct t_config_option *opt_int, *opt_str, *opt_cb;

    LONGS_EQUAL(WEECHAT_CONFIG_READ_FILE_NOT_FOUND, config_file_reload (NULL));

    opt_int = test_new_option (section_test, "int", "integer", NULL,
                               "10", NULL, 0);
    opt_str = test_new_option (section_test, "str", "string", NULL,
                               "value", NULL, 0);
    CHECK(opt_int);
    CHECK(opt_str);
    section_cb = config_file_new_section (
        config_test, "cb", 0, 0,
        &test_section_read_cb, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    CHECK(section_cb);
    opt_cb = test_new_option (section_cb, "opt", "string", NULL,
                              "value", NULL, 0);
    CHECK(opt_cb);

    config_file_option_set (opt_int, "20", 1);
    config_file_option_set (opt_str, "changed", 1);
    config_file_option_set (opt_cb, "changed", 1);

    test_config_write_file (TEST_CONFIG_NAME ".conf",
                            "[sec]\n"
                            "int = 50\n"
                            "[cb]\n"
                            "opt = test\n");

    LONGS_EQUAL(WEECHAT_CONFIG_READ_OK, config_file_reload (config_test));

    /* Option read in file */
    LONGS_EQUAL(50, CONFIG_INTEGER(opt_int));
    LONGS_EQUAL(1, opt_int->loaded);

    /* Option not found in file: reset to its default value */
    STRCMP_EQUAL("value", CONFIG_STRING(opt_str));
    LONGS_EQUAL(0, opt_str->loaded);

    /* Option in section with read callback: not reset */
    LONGS_EQUAL(1, test_section_read_cb_count);
    STRCMP_EQUAL("opt", test_section_read_cb_option);
    STRCMP_EQUAL("test", test_section_read_cb_value);
    STRCMP_EQUAL("changed", CONFIG_STRING(opt_cb));
}

/*
 * Test functions:
 *   config_file_option_free_data
 */

TEST(CoreConfigFile, OptionFreeData)
{
    struct t_config_option *option;
    int argc;

    /* Option without data */
    option = config_file_option_malloc ();
    CHECK(option);
    config_file_option_free_data (option);
    free (option);

    /* Option with all data allocated */
    option = config_file_option_malloc ();
    CHECK(option);
    option->name = strdup ("name");
    option->parent_name = strdup ("parent_name");
    option->description = strdup ("description");
    option->string_values = string_split ("v1|v2", "|", NULL, 0, 0, &argc);
    option->default_value = malloc (sizeof (int));
    option->value = malloc (sizeof (int));
    option->callback_check_value_data = malloc (16);
    option->callback_change_data = malloc (16);
    option->callback_delete_data = malloc (16);
    config_file_option_free_data (option);
    free (option);
}

/*
 * Test functions:
 *   config_file_option_free
 */

TEST(CoreConfigFileWithTestConfig, OptionFree)
{
    struct t_config_option *opt_a, *opt_b, *opt_c;
    struct t_hook *hook;

    /* Test free of NULL option. */
    config_file_option_free (NULL, 1);

    opt_a = test_new_option (section_test, "a", "string", NULL, "", NULL, 0);
    opt_b = test_new_option (section_test, "b", "string", NULL, "", NULL, 0);
    opt_c = test_new_option (section_test, "c", "string", NULL, "", NULL, 0);
    CHECK(opt_a);
    CHECK(opt_b);
    CHECK(opt_c);

    hook = hook_config (NULL, TEST_CONFIG_NAME ".sec.*",
                        &test_hook_config_cb, NULL, NULL);
    CHECK(hook);

    /* Free option in the middle of section (with callback) */
    config_file_option_free (opt_b, 1);
    LONGS_EQUAL(1, test_hook_config_cb_count);
    STRCMP_EQUAL(TEST_CONFIG_NAME ".sec.b", test_hook_config_cb_option);
    POINTERS_EQUAL(NULL, test_hook_config_cb_value);
    POINTERS_EQUAL(opt_a, section_test->options);
    POINTERS_EQUAL(opt_c, section_test->last_option);
    POINTERS_EQUAL(opt_c, opt_a->next_option);
    POINTERS_EQUAL(opt_a, opt_c->prev_option);

    /* Free first option (without callback) */
    config_file_option_free (opt_a, 0);
    LONGS_EQUAL(1, test_hook_config_cb_count);
    POINTERS_EQUAL(opt_c, section_test->options);
    POINTERS_EQUAL(opt_c, section_test->last_option);
    POINTERS_EQUAL(NULL, opt_c->prev_option);

    /* Free last option */
    config_file_option_free (opt_c, 0);
    POINTERS_EQUAL(NULL, section_test->options);
    POINTERS_EQUAL(NULL, section_test->last_option);

    unhook (hook);
}

/*
 * Test functions:
 *   config_file_section_free_options
 */

TEST(CoreConfigFileWithTestConfig, SectionFreeOptions)
{
    config_file_section_free_options (NULL);

    /* Section without options */
    config_file_section_free_options (section_test);
    POINTERS_EQUAL(NULL, section_test->options);
    POINTERS_EQUAL(NULL, section_test->last_option);

    CHECK(test_new_option (section_test, "a", "string", NULL, "", NULL, 0));
    CHECK(test_new_option (section_test, "b", "string", NULL, "", NULL, 0));
    CHECK(test_new_option (section_test, "c", "string", NULL, "", NULL, 0));

    config_file_section_free_options (section_test);
    POINTERS_EQUAL(NULL, section_test->options);
    POINTERS_EQUAL(NULL, section_test->last_option);
    POINTERS_EQUAL(section_test, config_file_search_section (config_test,
                                                             "sec"));
}

/*
 * Test functions:
 *   config_file_section_free
 */

TEST(CoreConfigFileWithTestConfig, SectionFree)
{
    struct t_config_section *section2, *section3, *section4;

    config_file_section_free (NULL);

    section2 = test_new_section (config_test, "sec2", 0, 0);
    section3 = test_new_section (config_test, "sec3", 0, 0);
    section4 = test_new_section (config_test, "sec4", 0, 0);
    CHECK(section2);
    CHECK(section3);
    CHECK(section4);
    CHECK(test_new_option (section3, "a", "string", NULL, "", NULL, 0));

    /* Free section in the middle of list */
    config_file_section_free (section3);
    POINTERS_EQUAL(NULL, config_file_search_section (config_test, "sec3"));
    POINTERS_EQUAL(section4, section2->next_section);
    POINTERS_EQUAL(section2, section4->prev_section);

    /* Free first section */
    config_file_section_free (section_test);
    section_test = NULL;
    POINTERS_EQUAL(section2, config_test->sections);
    POINTERS_EQUAL(NULL, section2->prev_section);

    /* Free last section */
    config_file_section_free (section4);
    POINTERS_EQUAL(section2, config_test->sections);
    POINTERS_EQUAL(section2, config_test->last_section);
    POINTERS_EQUAL(NULL, section2->next_section);

    /* Free the only section */
    config_file_section_free (section2);
    POINTERS_EQUAL(NULL, config_test->sections);
    POINTERS_EQUAL(NULL, config_test->last_section);
}

/*
 * Test functions:
 *   config_file_free
 */

TEST(CoreConfigFile, Free)
{
    struct t_config_file *config_first, *config_middle, *config_last;
    struct t_config_file *old_first, *old_last, *old_next_weechat;
    struct t_config_section *section;

    config_file_free (NULL);

    old_first = config_files;
    old_last = last_config_file;
    old_next_weechat = weechat_config_file->next_config;

    config_first = config_file_new (NULL, "AAA_test", NULL, NULL, NULL);
    config_middle = config_file_new (NULL, "weechat2", NULL, NULL, NULL);
    config_last = config_file_new (NULL, "zzz_test", NULL, NULL, NULL);
    CHECK(config_first);
    CHECK(config_middle);
    CHECK(config_last);
    section = test_new_section (config_middle, "sec", 0, 0);
    CHECK(section);
    CHECK(test_new_option (section, "opt", "string", NULL, "", NULL, 0));

    /* Free configuration file in the middle of list */
    config_file_free (config_middle);
    POINTERS_EQUAL(NULL, config_file_search ("weechat2"));
    POINTERS_EQUAL(old_next_weechat, weechat_config_file->next_config);
    POINTERS_EQUAL(weechat_config_file, old_next_weechat->prev_config);

    /* Free first configuration file */
    config_file_free (config_first);
    POINTERS_EQUAL(NULL, config_file_search ("AAA_test"));
    POINTERS_EQUAL(old_first, config_files);
    POINTERS_EQUAL(NULL, old_first->prev_config);

    /* Free last configuration file */
    config_file_free (config_last);
    POINTERS_EQUAL(NULL, config_file_search ("zzz_test"));
    POINTERS_EQUAL(old_last, last_config_file);
    POINTERS_EQUAL(NULL, old_last->next_config);
}

/*
 * Test functions:
 *   config_file_free_all
 */

TEST(CoreConfigFile, FreeAll)
{
    struct t_config_file *old_config_files, *old_last_config_file;

    /* Use an empty list to not free WeeChat configuration files. */
    old_config_files = config_files;
    old_last_config_file = last_config_file;
    config_files = NULL;
    last_config_file = NULL;

    CHECK(config_file_new (NULL, "test1", NULL, NULL, NULL));
    CHECK(config_file_new (NULL, "test2", NULL, NULL, NULL));
    CHECK(config_files);

    config_file_free_all ();
    POINTERS_EQUAL(NULL, config_files);
    POINTERS_EQUAL(NULL, last_config_file);

    config_files = old_config_files;
    last_config_file = old_last_config_file;
}

/*
 * Test functions:
 *   config_file_free_all_plugin
 */

TEST(CoreConfigFile, FreeAllPlugin)
{
    struct t_weechat_plugin *plugin1, *plugin2;
    struct t_config_file *config1a, *config1b, *config2;

    plugin1 = (struct t_weechat_plugin *)0x1;
    plugin2 = (struct t_weechat_plugin *)0x2;

    config1a = config_file_new (plugin1, "test_plugin1a", NULL, NULL, NULL);
    config1b = config_file_new (plugin1, "test_plugin1b", NULL, NULL, NULL);
    config2 = config_file_new (plugin2, "test_plugin2", NULL, NULL, NULL);
    CHECK(config1a);
    CHECK(config1b);
    CHECK(config2);

    config_file_free_all_plugin (plugin1);
    POINTERS_EQUAL(NULL, config_file_search ("test_plugin1a"));
    POINTERS_EQUAL(NULL, config_file_search ("test_plugin1b"));
    POINTERS_EQUAL(config2, config_file_search ("test_plugin2"));
    POINTERS_EQUAL(weechat_config_file, config_file_search ("weechat"));

    config_file_free_all_plugin (plugin2);
    POINTERS_EQUAL(NULL, config_file_search ("test_plugin2"));
    POINTERS_EQUAL(weechat_config_file, config_file_search ("weechat"));
}

/*
 * Test functions:
 *   config_file_hdata_config_file_cb
 */

TEST(CoreConfigFile, HdataConfigFileCb)
{
    struct t_hdata *hdata;

    hdata = config_file_hdata_config_file_cb (NULL, NULL, "test_config_file");
    CHECK(hdata);
    POINTERS_EQUAL(hdata, hashtable_get (weechat_hdata, "test_config_file"));

    LONGS_EQUAL(offsetof (struct t_config_file, name),
                hdata_get_var_offset (hdata, "name"));
    LONGS_EQUAL(WEECHAT_HDATA_POINTER, hdata_get_var_type (hdata, "plugin"));
    STRCMP_EQUAL("plugin", hdata_get_var_hdata (hdata, "plugin"));
    LONGS_EQUAL(WEECHAT_HDATA_INTEGER, hdata_get_var_type (hdata, "priority"));
    LONGS_EQUAL(WEECHAT_HDATA_STRING, hdata_get_var_type (hdata, "name"));
    LONGS_EQUAL(WEECHAT_HDATA_STRING, hdata_get_var_type (hdata, "filename"));
    LONGS_EQUAL(WEECHAT_HDATA_POINTER, hdata_get_var_type (hdata, "file"));
    LONGS_EQUAL(WEECHAT_HDATA_INTEGER, hdata_get_var_type (hdata, "version"));
    LONGS_EQUAL(WEECHAT_HDATA_POINTER,
                hdata_get_var_type (hdata, "callback_reload"));
    LONGS_EQUAL(WEECHAT_HDATA_POINTER, hdata_get_var_type (hdata, "sections"));
    STRCMP_EQUAL("config_section", hdata_get_var_hdata (hdata, "sections"));
    STRCMP_EQUAL("config_section",
                 hdata_get_var_hdata (hdata, "last_section"));
    STRCMP_EQUAL("test_config_file",
                 hdata_get_var_hdata (hdata, "prev_config"));
    STRCMP_EQUAL("test_config_file",
                 hdata_get_var_hdata (hdata, "next_config"));
    LONGS_EQUAL(-1, hdata_get_var_type (hdata, "zzz"));

    POINTERS_EQUAL(config_files, hdata_get_list (hdata, "config_files"));
    POINTERS_EQUAL(last_config_file,
                   hdata_get_list (hdata, "last_config_file"));

    STRCMP_EQUAL("weechat",
                 hdata_string (hdata, weechat_config_file, "name"));

    hashtable_remove (weechat_hdata, "test_config_file");
}

/*
 * Test functions:
 *   config_file_hdata_config_section_cb
 */

TEST(CoreConfigFile, HdataConfigSectionCb)
{
    struct t_hdata *hdata;

    hdata = config_file_hdata_config_section_cb (NULL, NULL,
                                                 "test_config_section");
    CHECK(hdata);
    POINTERS_EQUAL(hdata, hashtable_get (weechat_hdata,
                                         "test_config_section"));

    LONGS_EQUAL(offsetof (struct t_config_section, name),
                hdata_get_var_offset (hdata, "name"));
    LONGS_EQUAL(WEECHAT_HDATA_POINTER,
                hdata_get_var_type (hdata, "config_file"));
    STRCMP_EQUAL("config_file", hdata_get_var_hdata (hdata, "config_file"));
    LONGS_EQUAL(WEECHAT_HDATA_STRING, hdata_get_var_type (hdata, "name"));
    LONGS_EQUAL(WEECHAT_HDATA_INTEGER,
                hdata_get_var_type (hdata, "user_can_add_options"));
    LONGS_EQUAL(WEECHAT_HDATA_INTEGER,
                hdata_get_var_type (hdata, "user_can_delete_options"));
    LONGS_EQUAL(WEECHAT_HDATA_POINTER,
                hdata_get_var_type (hdata, "callback_read"));
    LONGS_EQUAL(WEECHAT_HDATA_POINTER,
                hdata_get_var_type (hdata, "callback_delete_option_data"));
    STRCMP_EQUAL("config_option", hdata_get_var_hdata (hdata, "options"));
    STRCMP_EQUAL("config_option", hdata_get_var_hdata (hdata, "last_option"));
    STRCMP_EQUAL("test_config_section",
                 hdata_get_var_hdata (hdata, "prev_section"));
    STRCMP_EQUAL("test_config_section",
                 hdata_get_var_hdata (hdata, "next_section"));
    LONGS_EQUAL(-1, hdata_get_var_type (hdata, "zzz"));

    STRCMP_EQUAL("look",
                 hdata_string (hdata, weechat_config_section_look, "name"));

    hashtable_remove (weechat_hdata, "test_config_section");
}

/*
 * Test functions:
 *   config_file_hdata_config_option_cb
 */

TEST(CoreConfigFile, HdataConfigOptionCb)
{
    struct t_hdata *hdata;

    hdata = config_file_hdata_config_option_cb (NULL, NULL,
                                                "test_config_option");
    CHECK(hdata);
    POINTERS_EQUAL(hdata, hashtable_get (weechat_hdata,
                                         "test_config_option"));

    LONGS_EQUAL(offsetof (struct t_config_option, name),
                hdata_get_var_offset (hdata, "name"));
    STRCMP_EQUAL("config_file", hdata_get_var_hdata (hdata, "config_file"));
    STRCMP_EQUAL("config_section", hdata_get_var_hdata (hdata, "section"));
    LONGS_EQUAL(WEECHAT_HDATA_STRING, hdata_get_var_type (hdata, "name"));
    LONGS_EQUAL(WEECHAT_HDATA_STRING,
                hdata_get_var_type (hdata, "parent_name"));
    LONGS_EQUAL(WEECHAT_HDATA_INTEGER, hdata_get_var_type (hdata, "type"));
    LONGS_EQUAL(WEECHAT_HDATA_INTEGER, hdata_get_var_type (hdata, "themable"));
    LONGS_EQUAL(WEECHAT_HDATA_STRING,
                hdata_get_var_type (hdata, "description"));
    LONGS_EQUAL(WEECHAT_HDATA_STRING,
                hdata_get_var_type (hdata, "string_values"));
    STRCMP_EQUAL("*",
                 hdata_get_var_array_size_string (hdata, NULL,
                                                  "string_values"));
    LONGS_EQUAL(WEECHAT_HDATA_INTEGER, hdata_get_var_type (hdata, "min"));
    LONGS_EQUAL(WEECHAT_HDATA_INTEGER, hdata_get_var_type (hdata, "max"));
    LONGS_EQUAL(WEECHAT_HDATA_POINTER,
                hdata_get_var_type (hdata, "default_value"));
    LONGS_EQUAL(WEECHAT_HDATA_POINTER, hdata_get_var_type (hdata, "value"));
    LONGS_EQUAL(WEECHAT_HDATA_INTEGER,
                hdata_get_var_type (hdata, "null_value_allowed"));
    LONGS_EQUAL(WEECHAT_HDATA_POINTER,
                hdata_get_var_type (hdata, "callback_delete_data"));
    LONGS_EQUAL(WEECHAT_HDATA_INTEGER, hdata_get_var_type (hdata, "loaded"));
    STRCMP_EQUAL("test_config_option",
                 hdata_get_var_hdata (hdata, "prev_option"));
    STRCMP_EQUAL("test_config_option",
                 hdata_get_var_hdata (hdata, "next_option"));
    LONGS_EQUAL(-1, hdata_get_var_type (hdata, "zzz"));

    STRCMP_EQUAL("buffer_time_format",
                 hdata_string (hdata, config_look_buffer_time_format, "name"));

    hashtable_remove (weechat_hdata, "test_config_option");
}

/*
 * Test functions:
 *   config_file_add_option_to_infolist
 */

TEST(CoreConfigFileWithNewOptions, AddOptionToInfolist)
{
    struct t_infolist *infolist;

    infolist = infolist_new (NULL);
    CHECK(infolist);

    /* Invalid option */
    LONGS_EQUAL(0, config_file_add_option_to_infolist (
                    infolist, weechat_config_file,
                    weechat_config_section_look, NULL, NULL));
    POINTERS_EQUAL(NULL, infolist->items);

    /* Option name not matching */
    LONGS_EQUAL(1, config_file_add_option_to_infolist (
                    infolist, weechat_config_file,
                    weechat_config_section_look, ptr_option_int, "zzz*"));
    POINTERS_EQUAL(NULL, infolist->items);

    /* Option with a parent and null value */
    LONGS_EQUAL(1, config_file_add_option_to_infolist (
                    infolist, weechat_config_file,
                    weechat_config_section_look, ptr_option_int_child,
                    NULL));
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("weechat.look.test_integer_child",
                 infolist_string (infolist, "full_name"));
    STRCMP_EQUAL("weechat", infolist_string (infolist, "config_name"));
    STRCMP_EQUAL("look", infolist_string (infolist, "section_name"));
    STRCMP_EQUAL("test_integer_child",
                 infolist_string (infolist, "option_name"));
    STRCMP_EQUAL("weechat.look.test_integer",
                 infolist_string (infolist, "parent_name"));
    STRCMP_EQUAL("", infolist_string (infolist, "description"));
    STRCMP_EQUAL("", infolist_string (infolist, "description_nls"));
    POINTERS_EQUAL(NULL, infolist_string (infolist, "string_values"));
    LONGS_EQUAL(0, infolist_integer (infolist, "min"));
    LONGS_EQUAL(123456, infolist_integer (infolist, "max"));
    LONGS_EQUAL(1, infolist_integer (infolist, "null_value_allowed"));
    LONGS_EQUAL(1, infolist_integer (infolist, "value_is_null"));
    LONGS_EQUAL(1, infolist_integer (infolist, "default_value_is_null"));
    STRCMP_EQUAL("integer", infolist_string (infolist, "type"));
    LONGS_EQUAL(0, infolist_integer (infolist, "themable"));
    POINTERS_EQUAL(NULL, infolist_string (infolist, "value"));
    POINTERS_EQUAL(NULL, infolist_string (infolist, "default_value"));
    STRCMP_EQUAL("100", infolist_string (infolist, "parent_value"));

    /* Enum option matching the option name */
    config_file_option_set (ptr_option_enum, "v3", 1);
    LONGS_EQUAL(1, config_file_add_option_to_infolist (
                    infolist, weechat_config_file,
                    weechat_config_section_look, ptr_option_enum,
                    "weechat.look.test_enum"));
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("weechat.look.test_enum",
                 infolist_string (infolist, "full_name"));
    POINTERS_EQUAL(NULL, infolist_string (infolist, "parent_name"));
    STRCMP_EQUAL("v1|v2|v3", infolist_string (infolist, "string_values"));
    LONGS_EQUAL(0, infolist_integer (infolist, "min"));
    LONGS_EQUAL(2, infolist_integer (infolist, "max"));
    LONGS_EQUAL(0, infolist_integer (infolist, "null_value_allowed"));
    LONGS_EQUAL(0, infolist_integer (infolist, "value_is_null"));
    LONGS_EQUAL(0, infolist_integer (infolist, "default_value_is_null"));
    STRCMP_EQUAL("enum", infolist_string (infolist, "type"));
    STRCMP_EQUAL("v3", infolist_string (infolist, "value"));
    STRCMP_EQUAL("v2", infolist_string (infolist, "default_value"));
    POINTERS_EQUAL(NULL, infolist_string (infolist, "parent_value"));

    /* Color option (themable) */
    LONGS_EQUAL(1, config_file_add_option_to_infolist (
                    infolist, weechat_config_file,
                    weechat_config_section_color, ptr_option_col, NULL));
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("weechat.color.test_color",
                 infolist_string (infolist, "full_name"));
    STRCMP_EQUAL("color", infolist_string (infolist, "type"));
    LONGS_EQUAL(1, infolist_integer (infolist, "themable"));
    STRCMP_EQUAL("blue", infolist_string (infolist, "value"));
    STRCMP_EQUAL("blue", infolist_string (infolist, "default_value"));

    POINTERS_EQUAL(NULL, infolist_next (infolist));

    infolist_free (infolist);
}

/*
 * Test functions:
 *   config_file_add_to_infolist
 */

TEST(CoreConfigFileWithNewOptions, AddToInfolist)
{
    struct t_infolist *infolist;
    struct t_infolist_item *ptr_item;
    int count;

    LONGS_EQUAL(0, config_file_add_to_infolist (NULL, NULL));

    /* No option matching */
    infolist = infolist_new (NULL);
    CHECK(infolist);
    LONGS_EQUAL(1, config_file_add_to_infolist (infolist, "zzz.*"));
    POINTERS_EQUAL(NULL, infolist->items);
    infolist_free (infolist);

    /* Some options matching */
    infolist = infolist_new (NULL);
    CHECK(infolist);
    LONGS_EQUAL(1, config_file_add_to_infolist (infolist,
                                                "weechat.look.test_*"));
    count = 0;
    for (ptr_item = infolist->items; ptr_item; ptr_item = ptr_item->next_item)
    {
        count++;
    }
    LONGS_EQUAL(10, count);
    infolist_free (infolist);

    /* All options */
    infolist = infolist_new (NULL);
    CHECK(infolist);
    LONGS_EQUAL(1, config_file_add_to_infolist (infolist, NULL));
    count = 0;
    for (ptr_item = infolist->items; ptr_item; ptr_item = ptr_item->next_item)
    {
        count++;
    }
    CHECK(count > 10);
    infolist_free (infolist);
}

/*
 * Test functions:
 *   config_file_print_log
 */

TEST(CoreConfigFileWithNewOptions, PrintLog)
{
    struct t_config_option *option;

    /* Option with null default value and value */
    option = config_file_new_option (
        weechat_config_file, weechat_config_section_look,
        "test_null", "string", "", NULL, 0, 0, NULL, NULL, 1,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    CHECK(option);

    config_file_print_log ();

    config_file_option_free (option, 0);
}
