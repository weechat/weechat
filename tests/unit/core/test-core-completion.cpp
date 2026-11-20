/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test default completions of WeeChat core */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "src/core/weechat.h"
#include "src/core/core-arraylist.h"
#include "src/core/core-completion.h"
#include "src/core/core-config.h"
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/core/core-secure.h"
#include "src/core/core-string.h"
#include "src/core/core-theme.h"
#include "src/gui/gui-bar-item-custom.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-completion.h"
#include "src/gui/gui-nicklist.h"
#include "src/plugins/plugin.h"

typedef int (t_test_completion_cb)(const void *pointer, void *data,
                                   const char *completion_item,
                                   struct t_gui_buffer *buffer,
                                   struct t_gui_completion *completion);

extern void completion_list_add_quoted_word (
    struct t_gui_completion *completion, const char *word);
extern t_test_completion_cb completion_list_add_bars_names_cb;
extern t_test_completion_cb completion_list_add_bars_items_cb;
extern t_test_completion_cb completion_list_add_custom_bar_items_names_cb;
extern t_test_completion_cb
    completion_list_add_custom_bar_item_conditions_cb;
extern t_test_completion_cb completion_list_add_custom_bar_item_contents_cb;
extern t_test_completion_cb
    completion_list_add_custom_bar_item_add_arguments_cb;
extern t_test_completion_cb completion_list_add_bars_options_cb;
extern t_test_completion_cb completion_list_add_buffers_names_cb;
extern t_test_completion_cb completion_list_add_buffers_numbers_cb;
extern t_test_completion_cb completion_list_add_buffers_plugins_names_cb;
extern t_test_completion_cb completion_list_add_buffer_local_variables_cb;
extern t_test_completion_cb completion_list_add_buffer_local_variable_value_cb;
extern t_test_completion_cb completion_list_add_buffer_properties_set_cb;
extern t_test_completion_cb completion_list_add_buffer_properties_setauto_cb;
extern t_test_completion_cb completion_list_add_buffer_properties_get_cb;
extern t_test_completion_cb completion_list_add_windows_numbers_cb;
extern t_test_completion_cb completion_list_add_colors_cb;
extern t_test_completion_cb completion_list_add_palette_colors_cb;
extern t_test_completion_cb completion_list_add_config_files_cb;
extern t_test_completion_cb completion_list_add_filename_cb;
extern t_test_completion_cb completion_list_add_filters_cb;
extern t_test_completion_cb completion_list_add_filters_disabled_cb;
extern t_test_completion_cb completion_list_add_filters_enabled_cb;
extern t_test_completion_cb completion_list_add_hook_types_cb;
extern t_test_completion_cb completion_list_add_commands_cb;
extern t_test_completion_cb completion_list_add_infos_cb;
extern t_test_completion_cb completion_list_add_infolists_cb;
extern t_test_completion_cb completion_list_add_nicks_cb;
extern t_test_completion_cb completion_list_add_config_options_cb;
extern t_test_completion_cb completion_list_add_plugins_cb;
extern t_test_completion_cb completion_list_add_plugins_installed_cb;
extern t_test_completion_cb completion_list_add_plugins_commands_cb;
extern t_test_completion_cb completion_list_add_config_option_values_cb;
extern t_test_completion_cb completion_list_add_weechat_commands_cb;
extern t_test_completion_cb completion_list_add_proxies_names_cb;
extern t_test_completion_cb completion_list_add_proxies_options_cb;
extern t_test_completion_cb completion_list_add_keys_contexts_cb;
extern t_test_completion_cb completion_list_add_keys_codes_cb;
extern t_test_completion_cb completion_list_add_keys_codes_for_reset_cb;
extern t_test_completion_cb completion_list_add_cursor_areas_cb;
extern t_test_completion_cb completion_list_add_layouts_names_cb;
extern t_test_completion_cb completion_list_add_theme_themes_all_cb;
extern t_test_completion_cb completion_list_add_theme_themes_user_cb;
extern t_test_completion_cb completion_list_add_theme_themes_files_cb;
extern t_test_completion_cb completion_list_add_secured_data_cb;
extern t_test_completion_cb completion_list_add_env_vars_cb;
extern t_test_completion_cb completion_list_add_env_value_cb;
extern t_test_completion_cb completion_list_add_eval_variables_cb;
}

#define TEST_BUFFER_NAME "test_compl_buffer"

/* Check the exact list of words (separated by "|") found by a completion. */
#define WEE_CHECK_COMPL(__result, __callback, __item, __buffer,          \
                        __base_word, __args, __arg_index)               \
    str_list = test_completion_run (&__callback, __item, __buffer,      \
                                    __base_word, __args, __arg_index);  \
    STRCMP_EQUAL(__result, str_list);                                   \
    free (str_list);

/* Check that a word is (or is not) in the list found by a completion. */
#define WEE_CHECK_COMPL_HAS(__found, __word, __callback, __item,        \
                            __buffer, __base_word, __args, __arg_index) \
    str_list = test_completion_run (&__callback, __item, __buffer,      \
                                    __base_word, __args, __arg_index);  \
    LONGS_EQUAL(__found, test_completion_has_word (str_list, __word));  \
    free (str_list);

/*
 * Run a completion callback and return the list of words found, separated by
 * "|" (empty string if no words were found).
 *
 * Note: result must be freed after use.
 */

char *
test_completion_run (t_test_completion_cb *callback,
                     const char *completion_item,
                     struct t_gui_buffer *buffer,
                     const char *base_word,
                     const char *args,
                     int arg_index)
{
    struct t_gui_completion *completion;
    struct t_gui_completion_word *ptr_word;
    char **result;
    int i;

    completion = gui_completion_new (NULL, (buffer) ? buffer : gui_buffers);
    if (!completion)
        return NULL;

    completion->base_word = (base_word) ? strdup (base_word) : NULL;
    completion->base_word_pos = 1;  /* no nick completer added */
    completion->args = (args) ? strdup (args) : NULL;
    completion->base_command_arg_index = arg_index;

    (void) (callback) (NULL, NULL, completion_item,
                       completion->buffer, completion);

    result = string_dyn_alloc (256);
    for (i = 0; i < arraylist_size (completion->list); i++)
    {
        ptr_word = (struct t_gui_completion_word *)arraylist_get (
            completion->list, i);
        if (i > 0)
            string_dyn_concat (result, "|", -1);
        string_dyn_concat (result, ptr_word->word, -1);
    }

    gui_completion_free (completion);

    return string_dyn_free (result, 0);
}

/*
 * Check if a word is in a list of words separated by "|".
 *
 * Return:
 *   1: word found
 *   0: word not found
 */

int
test_completion_has_word (const char *list, const char *word)
{
    char *str_list, *str_word;
    int found;

    if (!list || !word)
        return 0;

    if (string_asprintf (&str_list, "|%s|", list) < 0)
        return 0;
    if (string_asprintf (&str_word, "|%s|", word) < 0)
    {
        free (str_list);
        return 0;
    }

    found = (strstr (str_list, str_word)) ? 1 : 0;

    free (str_list);
    free (str_word);

    return found;
}

/*
 * Create an empty file.
 */

void
test_completion_create_file (const char *path)
{
    FILE *file;

    file = fopen (path, "w");
    if (file)
        fclose (file);
}

TEST_GROUP(CoreCompletion)
{
};

/*
 * Test functions:
 *   completion_list_add_quoted_word
 */

TEST(CoreCompletion, AddQuotedWord)
{
    struct t_gui_completion *completion;
    struct t_gui_completion_word *ptr_word;

    completion = gui_completion_new (NULL, gui_buffers);
    CHECK(completion);
    completion_list_add_quoted_word (completion, "test");
    LONGS_EQUAL(1, arraylist_size (completion->list));
    ptr_word = (struct t_gui_completion_word *)arraylist_get (completion->list,
                                                              0);
    STRCMP_EQUAL("\"test\"", ptr_word->word);
    gui_completion_free (completion);
}

/*
 * Test functions:
 *   completion_list_add_bars_names_cb
 *   completion_list_add_bars_items_cb
 *   completion_list_add_bars_options_cb
 *   completion_list_add_cursor_areas_cb
 */

TEST(CoreCompletion, Bars)
{
    char *str_list;

    WEE_CHECK_COMPL("title", completion_list_add_bars_names_cb,
                    NULL, NULL, "tit", NULL, 0);
    WEE_CHECK_COMPL("", completion_list_add_bars_names_cb,
                    NULL, NULL, "xxx", NULL, 0);

    WEE_CHECK_COMPL_HAS(1, "buffer_name", completion_list_add_bars_items_cb,
                        NULL, NULL, "buffer_", NULL, 0);
    WEE_CHECK_COMPL_HAS(0, "time", completion_list_add_bars_items_cb,
                        NULL, NULL, "buffer_", NULL, 0);

    WEE_CHECK_COMPL("priority", completion_list_add_bars_options_cb,
                    NULL, NULL, "pri", NULL, 0);
    WEE_CHECK_COMPL_HAS(1, "items", completion_list_add_bars_options_cb,
                        NULL, NULL, NULL, NULL, 0);

    WEE_CHECK_COMPL("chat", completion_list_add_cursor_areas_cb,
                    NULL, NULL, "ch", NULL, 0);
    WEE_CHECK_COMPL_HAS(1, "chat", completion_list_add_cursor_areas_cb,
                        NULL, NULL, NULL, NULL, 0);
}

/*
 * Test functions:
 *   completion_list_add_custom_bar_items_names_cb
 *   completion_list_add_custom_bar_item_conditions_cb
 *   completion_list_add_custom_bar_item_contents_cb
 *   completion_list_add_custom_bar_item_add_arguments_cb
 */

TEST(CoreCompletion, CustomBarItems)
{
    char *str_list;

    WEE_CHECK_COMPL("", completion_list_add_custom_bar_items_names_cb,
                    NULL, NULL, "test_compl", NULL, 0);

    WEE_CHECK_COMPL("\"" GUI_BAR_ITEM_CUSTOM_DEFAULT_CONDITIONS "\"|\"\"",
                    completion_list_add_custom_bar_item_conditions_cb,
                    NULL, NULL, NULL, NULL, 0);
    WEE_CHECK_COMPL("\"" GUI_BAR_ITEM_CUSTOM_DEFAULT_CONTENTS "\"|\"\"",
                    completion_list_add_custom_bar_item_contents_cb,
                    NULL, NULL, NULL, NULL, 0);

    run_cmd_quiet ("/mute /item add test_compl_item \"${buffer.number} == 1\" "
                   "\"content\"");

    WEE_CHECK_COMPL("test_compl_item",
                    completion_list_add_custom_bar_items_names_cb,
                    NULL, NULL, "test_compl", NULL, 0);

    /* No args: nothing is completed. */
    WEE_CHECK_COMPL("", completion_list_add_custom_bar_item_add_arguments_cb,
                    NULL, NULL, NULL, NULL, 0);

    /* First argument: name of custom bar item. */
    WEE_CHECK_COMPL("test_compl_item",
                    completion_list_add_custom_bar_item_add_arguments_cb,
                    NULL, NULL, "", "add", 0);
    WEE_CHECK_COMPL("test_compl_item",
                    completion_list_add_custom_bar_item_add_arguments_cb,
                    NULL, NULL, "test_compl", "add test_compl", 0);

    /* Second argument: conditions (of existing item, or default ones). */
    WEE_CHECK_COMPL("\"${buffer.number} == 1\"",
                    completion_list_add_custom_bar_item_add_arguments_cb,
                    NULL, NULL, "", "add test_compl_item", 0);
    WEE_CHECK_COMPL("\"" GUI_BAR_ITEM_CUSTOM_DEFAULT_CONDITIONS "\"|\"\"",
                    completion_list_add_custom_bar_item_add_arguments_cb,
                    NULL, NULL, "", "add new_item", 0);

    /* Third argument: content (of existing item, or default one). */
    WEE_CHECK_COMPL("\"content\"",
                    completion_list_add_custom_bar_item_add_arguments_cb,
                    NULL, NULL, "", "add test_compl_item \"cond\"", 0);
    WEE_CHECK_COMPL("\"" GUI_BAR_ITEM_CUSTOM_DEFAULT_CONTENTS "\"|\"\"",
                    completion_list_add_custom_bar_item_add_arguments_cb,
                    NULL, NULL, "", "add new_item \"cond\"", 0);

    /* Fourth argument: nothing is completed. */
    WEE_CHECK_COMPL("",
                    completion_list_add_custom_bar_item_add_arguments_cb,
                    NULL, NULL, "", "add test_compl_item \"cond\" \"x\"", 0);

    run_cmd_quiet ("/mute /item del test_compl_item");
}

/*
 * Test functions:
 *   completion_list_add_buffers_names_cb
 *   completion_list_add_buffers_numbers_cb
 *   completion_list_add_buffers_plugins_names_cb
 *   completion_list_add_windows_numbers_cb
 */

TEST(CoreCompletion, BuffersWindows)
{
    struct t_gui_buffer *buffer;
    char *str_list, str_number[32];

    buffer = gui_buffer_new_user (TEST_BUFFER_NAME, GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);

    WEE_CHECK_COMPL(TEST_BUFFER_NAME, completion_list_add_buffers_names_cb,
                    NULL, NULL, "test_compl", NULL, 0);
    WEE_CHECK_COMPL_HAS(1, "weechat", completion_list_add_buffers_names_cb,
                        NULL, NULL, NULL, NULL, 0);

    snprintf (str_number, sizeof (str_number), "%d", buffer->number);
    WEE_CHECK_COMPL_HAS(1, "1", completion_list_add_buffers_numbers_cb,
                        NULL, NULL, NULL, NULL, 0);
    WEE_CHECK_COMPL_HAS(1, str_number, completion_list_add_buffers_numbers_cb,
                        NULL, NULL, NULL, NULL, 0);

    WEE_CHECK_COMPL("core." TEST_BUFFER_NAME,
                    completion_list_add_buffers_plugins_names_cb,
                    NULL, NULL, "core.test_compl", NULL, 0);
    WEE_CHECK_COMPL_HAS(1, "core.weechat",
                        completion_list_add_buffers_plugins_names_cb,
                        NULL, NULL, NULL, NULL, 0);

    WEE_CHECK_COMPL("1", completion_list_add_windows_numbers_cb,
                    NULL, NULL, NULL, NULL, 0);

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   completion_list_add_buffer_local_variables_cb
 *   completion_list_add_buffer_local_variable_value_cb
 *   completion_list_add_buffer_properties_set_cb
 *   completion_list_add_buffer_properties_setauto_cb
 *   completion_list_add_buffer_properties_get_cb
 *   completion_list_add_eval_variables_cb
 */

TEST(CoreCompletion, BufferProperties)
{
    struct t_gui_buffer *buffer;
    char *str_list;

    buffer = gui_buffer_new_user (TEST_BUFFER_NAME, GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);
    gui_buffer_set (buffer, "localvar_set_test_var1", "value1");
    gui_buffer_set (buffer, "localvar_set_test_var2", "value2");
    gui_buffer_set (buffer, "key_bind_meta-z", "/print test");

    WEE_CHECK_COMPL("test_var1|test_var2",
                    completion_list_add_buffer_local_variables_cb,
                    NULL, buffer, "test_", NULL, 0);

    /* Value of local variable: name is in previous argument. */
    WEE_CHECK_COMPL("", completion_list_add_buffer_local_variable_value_cb,
                    NULL, buffer, NULL, NULL, 0);
    WEE_CHECK_COMPL("", completion_list_add_buffer_local_variable_value_cb,
                    NULL, buffer, NULL, "", 0);
    WEE_CHECK_COMPL("value1",
                    completion_list_add_buffer_local_variable_value_cb,
                    NULL, buffer, NULL, "setvar test_var1", 3);
    WEE_CHECK_COMPL("value2",
                    completion_list_add_buffer_local_variable_value_cb,
                    NULL, buffer, NULL, "setvar test_var2 xxx", 3);
    /* Invalid argument index: last argument is used. */
    WEE_CHECK_COMPL("value2",
                    completion_list_add_buffer_local_variable_value_cb,
                    NULL, buffer, NULL, "setvar test_var2", 0);
    WEE_CHECK_COMPL("",
                    completion_list_add_buffer_local_variable_value_cb,
                    NULL, buffer, NULL, "setvar unknown", 3);

    WEE_CHECK_COMPL("short_name", completion_list_add_buffer_properties_set_cb,
                    NULL, buffer, "short_na", NULL, 0);

    WEE_CHECK_COMPL("short_name",
                    completion_list_add_buffer_properties_setauto_cb,
                    NULL, buffer, "short_na", NULL, 0);
    WEE_CHECK_COMPL("localvar_set_test_var1|localvar_set_test_var2",
                    completion_list_add_buffer_properties_setauto_cb,
                    NULL, buffer, "localvar_set_test", NULL, 0);
    WEE_CHECK_COMPL("key_bind_meta-z|key_unbind_meta-z",
                    completion_list_add_buffer_properties_setauto_cb,
                    NULL, buffer, "key_", NULL, 0);

    WEE_CHECK_COMPL("short_name", completion_list_add_buffer_properties_get_cb,
                    NULL, buffer, "short_na", NULL, 0);
    WEE_CHECK_COMPL_HAS(1, "number",
                        completion_list_add_buffer_properties_get_cb,
                        NULL, buffer, NULL, NULL, 0);
    WEE_CHECK_COMPL_HAS(1, "plugin",
                        completion_list_add_buffer_properties_get_cb,
                        NULL, buffer, NULL, NULL, 0);

    WEE_CHECK_COMPL("${upper:string}", completion_list_add_eval_variables_cb,
                    NULL, buffer, "${upp", NULL, 0);
    WEE_CHECK_COMPL("${test_var1}|${test_var2}",
                    completion_list_add_eval_variables_cb,
                    NULL, buffer, "${test_", NULL, 0);

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   completion_list_add_colors_cb
 *   completion_list_add_palette_colors_cb
 */

TEST(CoreCompletion, Colors)
{
    char *str_list;

    WEE_CHECK_COMPL("lightblue", completion_list_add_colors_cb,
                    NULL, NULL, "lightbl", NULL, 0);
    WEE_CHECK_COMPL_HAS(1, "default", completion_list_add_colors_cb,
                        NULL, NULL, NULL, NULL, 0);
    WEE_CHECK_COMPL_HAS(1, "0", completion_list_add_colors_cb,
                        NULL, NULL, NULL, NULL, 0);

    WEE_CHECK_COMPL("", completion_list_add_palette_colors_cb,
                    NULL, NULL, "test_compl", NULL, 0);
    run_cmd_quiet ("/mute /color alias 214 test_compl_orange");
    WEE_CHECK_COMPL("214", completion_list_add_palette_colors_cb,
                    NULL, NULL, "214", NULL, 0);
    WEE_CHECK_COMPL_HAS(1, "test_compl_orange", completion_list_add_colors_cb,
                        NULL, NULL, NULL, NULL, 0);
    run_cmd_quiet ("/mute /color unalias 214");
}

/*
 * Test functions:
 *   completion_list_add_config_files_cb
 *   completion_list_add_config_options_cb
 */

TEST(CoreCompletion, ConfigFilesOptions)
{
    char *str_list;

    WEE_CHECK_COMPL("weechat", completion_list_add_config_files_cb,
                    NULL, NULL, "weec", NULL, 0);
    WEE_CHECK_COMPL_HAS(1, "sec", completion_list_add_config_files_cb,
                        NULL, NULL, NULL, NULL, 0);

    WEE_CHECK_COMPL("weechat.look.confirm_quit",
                    completion_list_add_config_options_cb,
                    NULL, NULL, "weechat.look.confirm_q", NULL, 0);
    WEE_CHECK_COMPL("", completion_list_add_config_options_cb,
                    NULL, NULL, "weechat.look.xxx", NULL, 0);
}

/*
 * Test functions:
 *   completion_list_add_config_option_values_cb
 */

TEST(CoreCompletion, ConfigOptionValues)
{
    struct t_config_file *config;
    struct t_config_section *section;
    char *str_list;

    config = config_file_new (NULL, "test_compl", NULL, NULL, NULL);
    CHECK(config);
    section = config_file_new_section (config, "sec", 0, 0,
                                       NULL, NULL, NULL,
                                       NULL, NULL, NULL,
                                       NULL, NULL, NULL,
                                       NULL, NULL, NULL,
                                       NULL, NULL, NULL);
    CHECK(section);
    CHECK(config_file_new_option (config, section, "bool_on", "boolean", "",
                                  NULL, 0, 0, "on", NULL, 0,
                                  NULL, NULL, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL));
    CHECK(config_file_new_option (config, section, "bool_off", "boolean", "",
                                  NULL, 0, 0, "off", "off", 1,
                                  NULL, NULL, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL));
    CHECK(config_file_new_option (config, section, "bool_null", "boolean", "",
                                  NULL, 0, 0, NULL, NULL, 1,
                                  NULL, NULL, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL));
    CHECK(config_file_new_option (config, section, "int_min", "integer", "",
                                  NULL, 0, 10, "0", NULL, 0,
                                  NULL, NULL, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL));
    CHECK(config_file_new_option (config, section, "int_mid", "integer", "",
                                  NULL, 0, 10, "5", NULL, 0,
                                  NULL, NULL, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL));
    CHECK(config_file_new_option (config, section, "int_max", "integer", "",
                                  NULL, 0, 10, "10", NULL, 0,
                                  NULL, NULL, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL));
    CHECK(config_file_new_option (config, section, "int_null", "integer", "",
                                  NULL, 0, 10, NULL, NULL, 1,
                                  NULL, NULL, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL));
    CHECK(config_file_new_option (config, section, "str", "string", "",
                                  NULL, 0, 0, "value", NULL, 0,
                                  NULL, NULL, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL));
    CHECK(config_file_new_option (config, section, "str_null", "string", "",
                                  NULL, 0, 0, NULL, NULL, 1,
                                  NULL, NULL, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL));
    CHECK(config_file_new_option (config, section, "color", "color", "",
                                  NULL, 0, 0, "lightblue", NULL, 0,
                                  NULL, NULL, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL));
    CHECK(config_file_new_option (config, section, "color_null", "color", "",
                                  NULL, 0, 0, NULL, NULL, 1,
                                  NULL, NULL, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL));
    CHECK(config_file_new_option (config, section, "enum", "enum", "",
                                  "v1|v2|v3", 0, 0, "v2", NULL, 0,
                                  NULL, NULL, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL));
    CHECK(config_file_new_option (config, section, "enum_null", "enum", "",
                                  "v1|v2|v3", 0, 0, NULL, NULL, 1,
                                  NULL, NULL, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL));

    /* Invalid or unknown option. */
    WEE_CHECK_COMPL("", completion_list_add_config_option_values_cb,
                    NULL, NULL, NULL, NULL, 0);
    WEE_CHECK_COMPL("", completion_list_add_config_option_values_cb,
                    NULL, NULL, NULL, "test_compl", 0);
    WEE_CHECK_COMPL("", completion_list_add_config_option_values_cb,
                    NULL, NULL, NULL, "test_compl.sec", 0);
    WEE_CHECK_COMPL("", completion_list_add_config_option_values_cb,
                    NULL, NULL, NULL, "xxx.sec.bool_on", 0);
    WEE_CHECK_COMPL("", completion_list_add_config_option_values_cb,
                    NULL, NULL, NULL, "test_compl.xxx.bool_on", 0);
    WEE_CHECK_COMPL("", completion_list_add_config_option_values_cb,
                    NULL, NULL, NULL, "test_compl.sec.xxx", 0);

    /* Boolean. */
    WEE_CHECK_COMPL("on|off|toggle",
                    completion_list_add_config_option_values_cb,
                    NULL, NULL, NULL, "test_compl.sec.bool_on", 0);
    WEE_CHECK_COMPL("off|on|toggle|null",
                    completion_list_add_config_option_values_cb,
                    NULL, NULL, NULL, "test_compl.sec.bool_off ", 0);
    WEE_CHECK_COMPL("null|off|on|toggle",
                    completion_list_add_config_option_values_cb,
                    NULL, NULL, NULL, "test_compl.sec.bool_null", 0);

    /* Integer. */
    WEE_CHECK_COMPL("0|++1",
                    completion_list_add_config_option_values_cb,
                    NULL, NULL, NULL, "test_compl.sec.int_min", 0);
    WEE_CHECK_COMPL("5|++1|--1",
                    completion_list_add_config_option_values_cb,
                    NULL, NULL, NULL, "test_compl.sec.int_mid", 0);
    WEE_CHECK_COMPL("10|--1",
                    completion_list_add_config_option_values_cb,
                    NULL, NULL, NULL, "test_compl.sec.int_max", 0);
    WEE_CHECK_COMPL("null",
                    completion_list_add_config_option_values_cb,
                    NULL, NULL, NULL, "test_compl.sec.int_null", 0);

    /* String. */
    WEE_CHECK_COMPL("\"value\"|\"\"",
                    completion_list_add_config_option_values_cb,
                    NULL, NULL, NULL, "test_compl.sec.str", 0);
    WEE_CHECK_COMPL("null|\"\"",
                    completion_list_add_config_option_values_cb,
                    NULL, NULL, NULL, "test_compl.sec.str_null", 0);

    /* Color. */
    WEE_CHECK_COMPL("lightblue",
                    completion_list_add_config_option_values_cb,
                    NULL, NULL, "lightbl", "test_compl.sec.color", 0);
    WEE_CHECK_COMPL("++1",
                    completion_list_add_config_option_values_cb,
                    NULL, NULL, "++", "test_compl.sec.color", 0);
    WEE_CHECK_COMPL("null",
                    completion_list_add_config_option_values_cb,
                    NULL, NULL, "nu", "test_compl.sec.color_null", 0);

    /* Enum. */
    WEE_CHECK_COMPL("v2|v1|v3|++1|--1",
                    completion_list_add_config_option_values_cb,
                    NULL, NULL, NULL, "test_compl.sec.enum", 0);
    WEE_CHECK_COMPL("null|v1|v2|v3|++1|--1",
                    completion_list_add_config_option_values_cb,
                    NULL, NULL, NULL, "test_compl.sec.enum_null", 0);

    config_file_free (config);
}

/*
 * Test functions:
 *   completion_list_add_filename_cb
 */

TEST(CoreCompletion, Filename)
{
    char *str_list, *path_dir, *real_path_dir, *path, *item, *base_word;
    char *old_home;

    CHECK(string_asprintf (&path_dir, "%s/test_compl_dir",
                           weechat_data_dir) >= 0);
    mkdir (path_dir, 0700);
    CHECK(string_asprintf (&path, "%s/sub", path_dir) >= 0);
    mkdir (path, 0700);
    free (path);
    CHECK(string_asprintf (&path, "%s/file1", path_dir) >= 0);
    test_completion_create_file (path);
    free (path);
    CHECK(string_asprintf (&path, "%s/file2", path_dir) >= 0);
    test_completion_create_file (path);
    free (path);

    /* Relative to WeeChat data directory. */
    WEE_CHECK_COMPL("test_compl_dir/file1|test_compl_dir/file2",
                    completion_list_add_filename_cb,
                    "filename", NULL, "test_compl_dir/f", NULL, 0);
    WEE_CHECK_COMPL("test_compl_dir/file1|test_compl_dir/file2|"
                    "test_compl_dir/sub/",
                    completion_list_add_filename_cb,
                    "filename", NULL, "test_compl_dir/", NULL, 0);
    WEE_CHECK_COMPL("test_compl_dir/",
                    completion_list_add_filename_cb,
                    "filename", NULL, "test_compl_d", NULL, 0);
    WEE_CHECK_COMPL("",
                    completion_list_add_filename_cb,
                    "filename", NULL, "test_compl_dir/xxx", NULL, 0);
    WEE_CHECK_COMPL("",
                    completion_list_add_filename_cb,
                    "filename", NULL, "xxx/yyy", NULL, 0);

    /* Relative to a directory given in the completion item. */
    WEE_CHECK_COMPL("file1|file2",
                    completion_list_add_filename_cb,
                    "filename:${weechat_data_dir}/test_compl_dir",
                    NULL, "f", NULL, 0);
    /* Empty directory in the completion item: WeeChat data dir is used. */
    WEE_CHECK_COMPL("test_compl_dir/",
                    completion_list_add_filename_cb,
                    "filename:${xxx}", NULL, "test_compl_d", NULL, 0);

    /* Absolute path. */
    real_path_dir = realpath (path_dir, NULL);
    CHECK(real_path_dir);
    CHECK(string_asprintf (&base_word, "%s/f", real_path_dir) >= 0);
    CHECK(string_asprintf (&item, "%s/file1|%s/file2",
                           real_path_dir, real_path_dir) >= 0);
    WEE_CHECK_COMPL(item, completion_list_add_filename_cb,
                    "filename", NULL, base_word, NULL, 0);
    free (base_word);
    free (item);
    free (real_path_dir);

    /* Path relative to home directory. */
    old_home = (getenv ("HOME")) ? strdup (getenv ("HOME")) : NULL;
    setenv ("HOME", path_dir, 1);
    WEE_CHECK_COMPL("~/file1|~/file2",
                    completion_list_add_filename_cb,
                    "filename", NULL, "~/f", NULL, 0);
    WEE_CHECK_COMPL("~/sub/",
                    completion_list_add_filename_cb,
                    "filename", NULL, "~/s", NULL, 0);
    if (old_home)
    {
        setenv ("HOME", old_home, 1);
        free (old_home);
    }
    else
    {
        unsetenv ("HOME");
    }

    CHECK(string_asprintf (&path, "%s/file1", path_dir) >= 0);
    unlink (path);
    free (path);
    CHECK(string_asprintf (&path, "%s/file2", path_dir) >= 0);
    unlink (path);
    free (path);
    CHECK(string_asprintf (&path, "%s/sub", path_dir) >= 0);
    rmdir (path);
    free (path);
    rmdir (path_dir);
    free (path_dir);
}

/*
 * Test functions:
 *   completion_list_add_filters_cb
 *   completion_list_add_filters_disabled_cb
 *   completion_list_add_filters_enabled_cb
 */

TEST(CoreCompletion, Filters)
{
    char *str_list;

    WEE_CHECK_COMPL("", completion_list_add_filters_cb,
                    NULL, NULL, "test_compl", NULL, 0);

    run_cmd_quiet ("/mute /filter add test_compl_f1 * * xxx");
    run_cmd_quiet ("/mute /filter add test_compl_f2 * * yyy");
    run_cmd_quiet ("/mute /filter disable test_compl_f2");

    WEE_CHECK_COMPL("test_compl_f1|test_compl_f2",
                    completion_list_add_filters_cb,
                    NULL, NULL, "test_compl", NULL, 0);
    WEE_CHECK_COMPL("test_compl_f2",
                    completion_list_add_filters_disabled_cb,
                    NULL, NULL, "test_compl", NULL, 0);
    WEE_CHECK_COMPL("test_compl_f1",
                    completion_list_add_filters_enabled_cb,
                    NULL, NULL, "test_compl", NULL, 0);

    run_cmd_quiet ("/mute /filter del test_compl_f1");
    run_cmd_quiet ("/mute /filter del test_compl_f2");
}

/*
 * Test functions:
 *   completion_list_add_hook_types_cb
 *   completion_list_add_commands_cb
 *   completion_list_add_weechat_commands_cb
 *   completion_list_add_infos_cb
 *   completion_list_add_infolists_cb
 */

TEST(CoreCompletion, Hooks)
{
    char *str_list;

    WEE_CHECK_COMPL("command|command_run|completion",
                    completion_list_add_hook_types_cb,
                    NULL, NULL, "com", NULL, 0);

    WEE_CHECK_COMPL("upgrade", completion_list_add_commands_cb,
                    "commands", NULL, "upgr", NULL, 0);
    WEE_CHECK_COMPL("/upgrade", completion_list_add_commands_cb,
                    "commands:/", NULL, "/upgr", NULL, 0);
    if (plugin_search ("irc"))
    {
        WEE_CHECK_COMPL("allchan", completion_list_add_commands_cb,
                        "commands", NULL, "allch", NULL, 0);
    }

    WEE_CHECK_COMPL("upgrade", completion_list_add_weechat_commands_cb,
                    "weechat_commands", NULL, "upgr", NULL, 0);
    WEE_CHECK_COMPL("/upgrade", completion_list_add_weechat_commands_cb,
                    "weechat_commands:/", NULL, "/upgr", NULL, 0);
    /* Command of a plugin: not completed. */
    WEE_CHECK_COMPL("", completion_list_add_weechat_commands_cb,
                    "weechat_commands", NULL, "allch", NULL, 0);

    WEE_CHECK_COMPL("version_number", completion_list_add_infos_cb,
                    NULL, NULL, "version_n", NULL, 0);

    WEE_CHECK_COMPL_HAS(1, "buffer", completion_list_add_infolists_cb,
                        NULL, NULL, "buf", NULL, 0);
    WEE_CHECK_COMPL("", completion_list_add_infolists_cb,
                    NULL, NULL, "xxx", NULL, 0);
}

/*
 * Test functions:
 *   completion_list_add_nicks_cb
 */

TEST(CoreCompletion, Nicks)
{
    struct t_gui_buffer *buffer;
    char *str_list;

    buffer = gui_buffer_new_user (TEST_BUFFER_NAME, GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);

    WEE_CHECK_COMPL("", completion_list_add_nicks_cb,
                    NULL, buffer, NULL, NULL, 0);

    CHECK(gui_nicklist_add_nick (buffer, NULL, "bob", NULL, NULL, NULL, 1));
    CHECK(gui_nicklist_add_nick (buffer, NULL, "alice", NULL, NULL, NULL, 1));
    CHECK(gui_nicklist_add_nick (buffer, NULL, "carol", NULL, NULL, NULL, 0));

    /* Nicks in order of nicklist, invisible nicks are ignored. */
    WEE_CHECK_COMPL("alice|bob", completion_list_add_nicks_cb,
                    NULL, buffer, NULL, NULL, 0);
    WEE_CHECK_COMPL("bob", completion_list_add_nicks_cb,
                    NULL, buffer, "b", NULL, 0);

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   completion_list_add_plugins_cb
 *   completion_list_add_plugins_installed_cb
 *   completion_list_add_plugins_commands_cb
 */

TEST(CoreCompletion, Plugins)
{
    char *str_list;

    if (plugin_search ("irc"))
    {
        WEE_CHECK_COMPL("irc", completion_list_add_plugins_cb,
                        NULL, NULL, "ir", NULL, 0);
        WEE_CHECK_COMPL_HAS(1, "irc", completion_list_add_plugins_installed_cb,
                            NULL, NULL, "ir", NULL, 0);
    }
    WEE_CHECK_COMPL("", completion_list_add_plugins_cb,
                    NULL, NULL, "xxx", NULL, 0);

    /* Plugin name is in previous argument. */
    WEE_CHECK_COMPL("", completion_list_add_plugins_commands_cb,
                    "plugins_commands", NULL, NULL, NULL, 0);
    WEE_CHECK_COMPL("", completion_list_add_plugins_commands_cb,
                    "plugins_commands", NULL, NULL, "", 0);
    WEE_CHECK_COMPL("upgrade", completion_list_add_plugins_commands_cb,
                    "plugins_commands", NULL, "upgr", "core", 0);
    WEE_CHECK_COMPL("/upgrade", completion_list_add_plugins_commands_cb,
                    "plugins_commands:/", NULL, "/upgr", "core", 0);
    WEE_CHECK_COMPL("", completion_list_add_plugins_commands_cb,
                    "plugins_commands", NULL, "allch", "core", 0);
    if (plugin_search ("irc"))
    {
        WEE_CHECK_COMPL("allchan", completion_list_add_plugins_commands_cb,
                        "plugins_commands", NULL, "allch", "irc", 0);
        WEE_CHECK_COMPL("allchan", completion_list_add_plugins_commands_cb,
                        "plugins_commands", NULL, "allch", "list irc allch",
                        3);
    }
    WEE_CHECK_COMPL("", completion_list_add_plugins_commands_cb,
                    "plugins_commands", NULL, "allch", "xxx", 0);
}

/*
 * Test functions:
 *   completion_list_add_proxies_names_cb
 *   completion_list_add_proxies_options_cb
 */

TEST(CoreCompletion, Proxies)
{
    char *str_list;

    WEE_CHECK_COMPL("", completion_list_add_proxies_names_cb,
                    NULL, NULL, "test_compl", NULL, 0);
    run_cmd_quiet ("/mute /proxy add test_compl_proxy http 127.0.0.1 8080");
    WEE_CHECK_COMPL("test_compl_proxy", completion_list_add_proxies_names_cb,
                    NULL, NULL, "test_compl", NULL, 0);
    run_cmd_quiet ("/mute /proxy del test_compl_proxy");

    WEE_CHECK_COMPL("type", completion_list_add_proxies_options_cb,
                    NULL, NULL, "ty", NULL, 0);
    WEE_CHECK_COMPL_HAS(1, "password", completion_list_add_proxies_options_cb,
                        NULL, NULL, NULL, NULL, 0);
}

/*
 * Test functions:
 *   completion_list_add_keys_contexts_cb
 *   completion_list_add_keys_codes_cb
 *   completion_list_add_keys_codes_for_reset_cb
 */

TEST(CoreCompletion, Keys)
{
    char *str_list;

    WEE_CHECK_COMPL("default|search|histsearch|cursor|mouse",
                    completion_list_add_keys_contexts_cb,
                    NULL, NULL, NULL, NULL, 0);

    WEE_CHECK_COMPL_HAS(1, "ctrl-l", completion_list_add_keys_codes_cb,
                        NULL, NULL, "ctrl-", NULL, 0);

    WEE_CHECK_COMPL_HAS(0, "meta-z,meta-z,meta-z",
                        completion_list_add_keys_codes_for_reset_cb,
                        NULL, NULL, NULL, NULL, 0);
    WEE_CHECK_COMPL_HAS(0, "ctrl-l",
                        completion_list_add_keys_codes_for_reset_cb,
                        NULL, NULL, NULL, NULL, 0);

    /* Key added and key deleted: they can be reset. */
    run_cmd_quiet ("/mute /key bind meta-z,meta-z,meta-z /print test");
    run_cmd_quiet ("/mute /key unbind ctrl-l");
    WEE_CHECK_COMPL_HAS(1, "meta-z,meta-z,meta-z",
                        completion_list_add_keys_codes_for_reset_cb,
                        NULL, NULL, NULL, NULL, 0);
    WEE_CHECK_COMPL_HAS(1, "ctrl-l",
                        completion_list_add_keys_codes_for_reset_cb,
                        NULL, NULL, NULL, NULL, 0);
    run_cmd_quiet ("/mute /key unbind meta-z,meta-z,meta-z");
    run_cmd_quiet ("/mute /key reset ctrl-l");
    WEE_CHECK_COMPL_HAS(0, "ctrl-l",
                        completion_list_add_keys_codes_for_reset_cb,
                        NULL, NULL, NULL, NULL, 0);
}

/*
 * Test functions:
 *   completion_list_add_layouts_names_cb
 */

TEST(CoreCompletion, Layouts)
{
    char *str_list;

    WEE_CHECK_COMPL("", completion_list_add_layouts_names_cb,
                    NULL, NULL, "test_compl", NULL, 0);
    run_cmd_quiet ("/mute /layout store test_compl_layout");
    WEE_CHECK_COMPL("test_compl_layout", completion_list_add_layouts_names_cb,
                    NULL, NULL, "test_compl", NULL, 0);
    run_cmd_quiet ("/mute /layout del test_compl_layout");
}

/*
 * Test functions:
 *   completion_theme_add_file_cb
 *   completion_list_add_theme_themes_all_cb
 *   completion_list_add_theme_themes_user_cb
 *   completion_list_add_theme_themes_files_cb
 */

TEST(CoreCompletion, Themes)
{
    char *str_list, *path_dir, *path;
    const char *files[] = {
        "test_compl_user.theme",
        "backup-test_compl.theme",
        "test_compl_other.txt",
        ".theme",
        NULL,
    };
    int i, dir_created;

    CHECK(string_asprintf (&path_dir, "%s/themes", weechat_config_dir) >= 0);
    dir_created = (mkdir (path_dir, 0700) == 0);
    for (i = 0; files[i]; i++)
    {
        CHECK(string_asprintf (&path, "%s/%s", path_dir, files[i]) >= 0);
        test_completion_create_file (path);
        free (path);
    }

    if (themes)
    {
        WEE_CHECK_COMPL_HAS(1, themes->name,
                            completion_list_add_theme_themes_all_cb,
                            NULL, NULL, NULL, NULL, 0);
        WEE_CHECK_COMPL_HAS(0, themes->name,
                            completion_list_add_theme_themes_user_cb,
                            NULL, NULL, NULL, NULL, 0);
        WEE_CHECK_COMPL_HAS(0, themes->name,
                            completion_list_add_theme_themes_files_cb,
                            NULL, NULL, NULL, NULL, 0);
    }
    WEE_CHECK_COMPL("test_compl_user",
                    completion_list_add_theme_themes_all_cb,
                    NULL, NULL, "test_compl", NULL, 0);
    WEE_CHECK_COMPL("backup-test_compl",
                    completion_list_add_theme_themes_all_cb,
                    NULL, NULL, "backup-test_compl", NULL, 0);
    WEE_CHECK_COMPL("test_compl_user",
                    completion_list_add_theme_themes_user_cb,
                    NULL, NULL, "test_compl", NULL, 0);
    WEE_CHECK_COMPL("",
                    completion_list_add_theme_themes_user_cb,
                    NULL, NULL, "backup-test_compl", NULL, 0);
    WEE_CHECK_COMPL("test_compl_user",
                    completion_list_add_theme_themes_files_cb,
                    NULL, NULL, "test_compl", NULL, 0);
    WEE_CHECK_COMPL("backup-test_compl",
                    completion_list_add_theme_themes_files_cb,
                    NULL, NULL, "backup-test_compl", NULL, 0);

    for (i = 0; files[i]; i++)
    {
        CHECK(string_asprintf (&path, "%s/%s", path_dir, files[i]) >= 0);
        unlink (path);
        free (path);
    }
    if (dir_created)
        rmdir (path_dir);
    free (path_dir);
}

/*
 * Test functions:
 *   completion_list_add_secured_data_cb
 */

TEST(CoreCompletion, SecuredData)
{
    char *str_list;

    WEE_CHECK_COMPL("", completion_list_add_secured_data_cb,
                    NULL, NULL, "test_compl", NULL, 0);
    hashtable_set (secure_hashtable_data, "test_compl_sec", "value");
    WEE_CHECK_COMPL("test_compl_sec", completion_list_add_secured_data_cb,
                    NULL, NULL, "test_compl", NULL, 0);
    hashtable_remove (secure_hashtable_data, "test_compl_sec");
}

/*
 * Test functions:
 *   completion_list_add_env_vars_cb
 *   completion_list_add_env_value_cb
 */

TEST(CoreCompletion, Env)
{
    char *str_list;

    setenv ("WEECHAT_TEST_COMPL", "test value", 1);

    WEE_CHECK_COMPL("WEECHAT_TEST_COMPL", completion_list_add_env_vars_cb,
                    NULL, NULL, "WEECHAT_TEST_C", NULL, 0);

    /* Name of variable is in previous argument. */
    WEE_CHECK_COMPL("", completion_list_add_env_value_cb,
                    NULL, NULL, NULL, NULL, 0);
    WEE_CHECK_COMPL("", completion_list_add_env_value_cb,
                    NULL, NULL, NULL, "", 0);
    WEE_CHECK_COMPL("test value", completion_list_add_env_value_cb,
                    NULL, NULL, NULL, "set WEECHAT_TEST_COMPL", 3);
    WEE_CHECK_COMPL("test value", completion_list_add_env_value_cb,
                    NULL, NULL, NULL, "set WEECHAT_TEST_COMPL", 0);
    WEE_CHECK_COMPL("", completion_list_add_env_value_cb,
                    NULL, NULL, NULL, "set WEECHAT_TEST_XXX", 3);

    unsetenv ("WEECHAT_TEST_COMPL");

    WEE_CHECK_COMPL("", completion_list_add_env_vars_cb,
                    NULL, NULL, "WEECHAT_TEST_C", NULL, 0);
}
