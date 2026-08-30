/*
 * SPDX-FileCopyrightText: 2018-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test hook modifier functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "src/core/weechat.h"
#include "src/core/core-string.h"
#include "src/core/core-hook.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-chat.h"
#include "src/gui/gui-line.h"
#include "src/plugins/plugin.h"
}

#define TEST_BUFFER_NAME "test"

TEST_GROUP(HookModifier)
{
};

/*
 * Test functions:
 *   hook_modifier_get_description
 */

TEST(HookModifier, GetDescription)
{
    /* TODO: write tests */
}

char *
test_modifier_cb (const void *pointer, void *data,
                  const char *modifier, const char *modifier_data,
                  const char *string)
{
    char **items, *new_string;
    const char *ptr_plugin, *ptr_tags, *ptr_msg;
    int num_items, length, rc;
    unsigned long value;
    struct t_gui_buffer *ptr_buffer;

    /* Make C++ compiler happy. */
    (void) pointer;
    (void) data;
    (void) modifier;

    new_string = NULL;

    /* Split modifier_data, which is: "buffer_pointer;tags". */
    items = string_split (modifier_data, ";", NULL,
                          WEECHAT_STRING_SPLIT_STRIP_LEFT
                          | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                          | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                          2, &num_items);
    if (!items || (num_items < 1))
        goto error;

    ptr_tags = (num_items >= 2) ? items[1] : NULL;

    rc = sscanf (items[0], "%lx", &value);
    if ((rc == EOF) || (rc == 0))
        goto error;

    ptr_buffer = (struct t_gui_buffer *)value;

    ptr_plugin = gui_buffer_get_plugin_name (ptr_buffer);
    if (!ptr_plugin)
        goto error;

    /* Do nothing on a buffer different from "core.test". */
    if ((strcmp (ptr_plugin, "core") != 0)
        || (strcmp (ptr_buffer->name, TEST_BUFFER_NAME) != 0))
    {
        goto error;
    }

    if (strncmp (string, "\t\t", 2) == 0)
    {
        ptr_msg = string + 2;
    }
    else
    {
        ptr_msg = strchr (string, '\t');
        if (!ptr_msg)
            goto error;
        ptr_msg++;
    }

    length = strlen (string) + 128;
    new_string = (char *)malloc (length);
    if (!new_string)
        goto error;
    new_string[0] = '\0';

    if (ptr_tags && strstr (ptr_tags, "add_prefix"))
    {
        /* Add a prefix in message. */
        snprintf (new_string, length, "new prefix\t%s (modified)", ptr_msg);
    }
    else if (ptr_tags && strstr (ptr_tags, "add_date_prefix"))
    {
        /* Add a date/prefix in message. */
        snprintf (new_string, length, "new prefix\t%s (modified)", ptr_msg);
    }
    else if (ptr_tags && strstr (ptr_tags, "update_prefix"))
    {
        /* Update the prefix. */
        snprintf (new_string, length, "new prefix\t%s (modified)", ptr_msg);
    }
    else if (ptr_tags && strstr (ptr_tags, "remove_prefix"))
    {
        /* Remove the prefix. */
        snprintf (new_string, length, " \t%s (modified)", ptr_msg);
    }
    else if (ptr_tags && strstr (ptr_tags, "remove_date_prefix"))
    {
        /* Remove the date/prefix. */
        snprintf (new_string, length, "\t\t%s (modified)", ptr_msg);
    }

    if (!new_string[0])
    {
        /* Default message returned: just add " (modified)" to the string. */
        snprintf (new_string, length, "%s (modified)", string);
    }

    return new_string;

error:
    if (items)
        string_free_split (items);
    if (new_string)
        free (new_string);
    return NULL;
}

/*
 * Test functions:
 *   hook_modifier
 */

TEST(HookModifier, Modifier)
{
    struct t_gui_buffer *test_buffer;
    struct t_gui_line *ptr_line;
    struct t_hook *hook;

    /* Create/open a test buffer. */
    test_buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                                  NULL, NULL, NULL,
                                  NULL, NULL, NULL);
    CHECK(test_buffer);

    hook = hook_modifier (NULL, "weechat_print", &test_modifier_cb, NULL, NULL);

    /* Check hook contents. */
    CHECK(hook);
    POINTERS_EQUAL(NULL, hook->plugin);
    STRCMP_EQUAL(NULL, hook->subplugin);
    LONGS_EQUAL(HOOK_TYPE_MODIFIER, hook->type);
    LONGS_EQUAL(0, hook->deleted);
    LONGS_EQUAL(0, hook->running);
    LONGS_EQUAL(HOOK_PRIORITY_DEFAULT, hook->priority);
    POINTERS_EQUAL(NULL, hook->callback_pointer);
    POINTERS_EQUAL(NULL, hook->callback_data);
    CHECK(hook->hook_data);
    POINTERS_EQUAL(&test_modifier_cb, HOOK_MODIFIER(hook, callback));
    STRCMP_EQUAL("weechat_print", HOOK_MODIFIER(hook, modifier));

    /* Message without prefix: unchanged. */
    gui_chat_printf_date_tags (test_buffer, 0, NULL, " \tmessage");
    ptr_line = test_buffer->own_lines->last_line;
    CHECK(ptr_line->data->date > 0);
    STRCMP_EQUAL("", ptr_line->data->prefix);
    STRCMP_EQUAL("message (modified)", ptr_line->data->message);

    /* Message without prefix: add a prefix. */
    gui_chat_printf_date_tags (test_buffer, 0, "add_prefix", " \tmessage");
    ptr_line = test_buffer->own_lines->last_line;
    CHECK(ptr_line->data->date > 0);
    STRCMP_EQUAL("new prefix", ptr_line->data->prefix);
    STRCMP_EQUAL("message (modified)", ptr_line->data->message);

    /* Message without date: unchanged */
    gui_chat_printf_date_tags (test_buffer, 0, NULL, "\t\tmessage");
    ptr_line = test_buffer->own_lines->last_line;
    LONGS_EQUAL(0, ptr_line->data->date);
    STRCMP_EQUAL(NULL, ptr_line->data->prefix);
    STRCMP_EQUAL("message (modified)", ptr_line->data->message);

    /* Message without date: add a date/prefix. */
    gui_chat_printf_date_tags (test_buffer, 0, "add_date_prefix",
                               "\t\tmessage");
    ptr_line = test_buffer->own_lines->last_line;
    CHECK(ptr_line->data->date > 0);
    STRCMP_EQUAL("new prefix", ptr_line->data->prefix);
    STRCMP_EQUAL("message (modified)", ptr_line->data->message);

    /* Standard message: unchanged */
    gui_chat_printf_date_tags (test_buffer, 0, NULL, "prefix\tmessage");
    ptr_line = test_buffer->own_lines->last_line;
    CHECK(ptr_line->data->date > 0);
    STRCMP_EQUAL("prefix", ptr_line->data->prefix);
    STRCMP_EQUAL("message (modified)", ptr_line->data->message);

    /* Standard message: update the prefix. */
    gui_chat_printf_date_tags (test_buffer, 0, "update_prefix",
                               "prefix\tmessage");
    ptr_line = test_buffer->own_lines->last_line;
    CHECK(ptr_line->data->date > 0);
    STRCMP_EQUAL("new prefix", ptr_line->data->prefix);
    STRCMP_EQUAL("message (modified)", ptr_line->data->message);

    /* Standard message: remove the prefix. */
    gui_chat_printf_date_tags (test_buffer, 0, "remove_prefix",
                               "prefix\tmessage");
    ptr_line = test_buffer->own_lines->last_line;
    CHECK(ptr_line->data->date > 0);
    STRCMP_EQUAL("", ptr_line->data->prefix);
    STRCMP_EQUAL("message (modified)", ptr_line->data->message);

    /* Standard message: remove the date/prefix. */
    gui_chat_printf_date_tags (test_buffer, 0, "remove_date_prefix",
                               "prefix\tmessage");
    ptr_line = test_buffer->own_lines->last_line;
    LONGS_EQUAL(0, ptr_line->data->date);
    STRCMP_EQUAL(NULL, ptr_line->data->prefix);
    STRCMP_EQUAL("message (modified)", ptr_line->data->message);

    /* Close the test buffer. */
    gui_buffer_close (test_buffer);
}

/*
 * Test functions:
 *   hook_modifier_exec
 */

TEST(HookModifier, Exec)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_modifier_free_data
 */

TEST(HookModifier, FreeData)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_modifier_add_to_infolist
 */

TEST(HookModifier, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_modifier_print_log
 */

TEST(HookModifier, PrintLog)
{
    /* TODO: write tests */
}
