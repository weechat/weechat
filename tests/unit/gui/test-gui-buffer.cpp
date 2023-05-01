/*
 * test-gui-buffer.cpp - test buffer functions
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
#include "src/core/wee-hashtable.h"
#include "src/core/wee-hook.h"
#include "src/core/wee-input.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-key.h"
#include "src/gui/gui-line.h"
#include "src/gui/gui-nicklist.h"
#include "src/plugins/plugin.h"

extern int gui_buffer_user_input_cb (const void *pointer, void *data,
                                     struct t_gui_buffer *buffer,
                                     const char *input_data);
extern int gui_buffer_user_close_cb (const void *pointer, void *data,
                                     struct t_gui_buffer *buffer);
}

#define TEST_BUFFER_NAME "test"

char signal_buffer_user_input[256];
int signal_buffer_user_closing = 0;

TEST_GROUP(GuiBuffer)
{
    static int signal_buffer_user_input_cb (const void *pointer, void *data,
                                            const char *signal,
                                            const char *type_data,
                                            void *signal_data)
    {
        /* make C++ compiler happy */
        (void) pointer;
        (void) data;
        (void) signal;
        (void) type_data;

        if (signal_data)
        {
            snprintf (signal_buffer_user_input,
                      sizeof (signal_buffer_user_input),
                      "%s",
                      (const char *)signal_data);
        }
        return WEECHAT_RC_OK;
    }

    static int signal_buffer_user_input_eat_cb (const void *pointer, void *data,
                                                const char *signal,
                                                const char *type_data,
                                                void *signal_data)
    {
        /* make C++ compiler happy */
        (void) pointer;
        (void) data;
        (void) signal;
        (void) type_data;

        if (signal_data)
        {
            snprintf (signal_buffer_user_input,
                      sizeof (signal_buffer_user_input),
                      "%s",
                      (const char *)signal_data);
        }
        return WEECHAT_RC_OK_EAT;
    }

    static int signal_buffer_user_closing_cb (const void *pointer, void *data,
                                              const char *signal,
                                              const char *type_data,
                                              void *signal_data)
    {
        /* make C++ compiler happy */
        (void) pointer;
        (void) data;
        (void) signal;
        (void) type_data;
        (void) signal_data;

        signal_buffer_user_closing = 1;
        return WEECHAT_RC_OK_EAT;
    }
};

/*
 * Tests functions:
 *   gui_buffer_search_type
 */

TEST(GuiBuffer, SearchType)
{
    LONGS_EQUAL(-1, gui_buffer_search_type (NULL));
    LONGS_EQUAL(-1, gui_buffer_search_type (""));
    LONGS_EQUAL(-1, gui_buffer_search_type ("invalid"));

    LONGS_EQUAL(GUI_BUFFER_TYPE_FORMATTED, gui_buffer_search_type ("formatted"));
    LONGS_EQUAL(GUI_BUFFER_TYPE_FREE, gui_buffer_search_type ("free"));
}

/*
 * Tests functions:
 *   gui_buffer_search_notify
 */

TEST(GuiBuffer, SearchNotify)
{
    LONGS_EQUAL(-1, gui_buffer_search_notify (NULL));
    LONGS_EQUAL(-1, gui_buffer_search_notify (""));
    LONGS_EQUAL(-1, gui_buffer_search_notify ("invalid"));

    LONGS_EQUAL(GUI_BUFFER_NOTIFY_NONE, gui_buffer_search_notify ("none"));
    LONGS_EQUAL(GUI_BUFFER_NOTIFY_HIGHLIGHT, gui_buffer_search_notify ("highlight"));
    LONGS_EQUAL(GUI_BUFFER_NOTIFY_MESSAGE, gui_buffer_search_notify ("message"));
    LONGS_EQUAL(GUI_BUFFER_NOTIFY_ALL, gui_buffer_search_notify ("all"));
}

/*
 * Tests functions:
 *   gui_buffer_get_plugin_name
 */

TEST(GuiBuffer, GetPluginName)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_get_short_name
 */

TEST(GuiBuffer, GetShortName)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_build_full_name
 */

TEST(GuiBuffer, BuildFullName)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_local_var_add
 */

TEST(GuiBuffer, LocalVarAdd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_local_var_remove
 */

TEST(GuiBuffer, LocalVarRemove)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_local_var_remove_all
 */

TEST(GuiBuffer, LocalVarRemoveAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_notify_get
 */

TEST(GuiBuffer, NotifyGet)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_notify_set
 */

TEST(GuiBuffer, NotifySet)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_notify_set_all
 */

TEST(GuiBuffer, NotifySetAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_find_pos
 */

TEST(GuiBuffer, FindPos)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_shift_numbers
 */

TEST(GuiBuffer, ShiftNumbers)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_insert
 */

TEST(GuiBuffer, Insert)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_input_buffer_init
 */

TEST(GuiBuffer, InputBufferInit)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_is_reserved_name
 */

TEST(GuiBuffer, IsReservedName)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_apply_properties_cb
 */

TEST(GuiBuffer, ApplyPropertiesCb)
{
    /* TODO: write tests */
}

/*
 * Test callback for buffer input.
 */


int
test_buffer_input_cb (const void *pointer, void *data,
                      struct t_gui_buffer *buffer, const char *input_data)
{
    /* make C++ compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;
    (void) input_data;

    return WEECHAT_RC_OK;
}

/*
 * Test callback for buffer close.
 */

int
test_buffer_close_cb (const void *pointer, void *data,
                      struct t_gui_buffer *buffer)
{
    /* make C++ compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    return WEECHAT_RC_OK;
}

/*
 * Tests functions:
 *   gui_buffer_new_props
 */

TEST(GuiBuffer, NewProps)
{
    struct t_hashtable *properties;
    struct t_gui_buffer *buffer;

    properties = hashtable_new (32,
                                WEECHAT_HASHTABLE_STRING,
                                WEECHAT_HASHTABLE_STRING,
                                NULL,
                                NULL);
    hashtable_set (properties, "type", "free");
    buffer = gui_buffer_new_props (NULL, TEST_BUFFER_NAME, properties,
                                   &test_buffer_input_cb, NULL, NULL,
                                   &test_buffer_close_cb, NULL, NULL);
    CHECK(buffer);
    POINTERS_EQUAL(NULL, buffer->plugin);
    POINTERS_EQUAL(NULL, buffer->plugin_name_for_upgrade);
    LONGS_EQUAL(2, buffer->number);
    LONGS_EQUAL(0, buffer->layout_number);
    LONGS_EQUAL(0, buffer->layout_number_merge_order);
    STRCMP_EQUAL(TEST_BUFFER_NAME, buffer->name);
    STRCMP_EQUAL("core." TEST_BUFFER_NAME, buffer->full_name);
    POINTERS_EQUAL(NULL, buffer->old_full_name);
    POINTERS_EQUAL(NULL, buffer->short_name);
    LONGS_EQUAL(GUI_BUFFER_TYPE_FREE, buffer->type);
    LONGS_EQUAL(GUI_BUFFER_NOTIFY_ALL, buffer->notify);
    LONGS_EQUAL(0, buffer->num_displayed);
    LONGS_EQUAL(1, buffer->active);
    LONGS_EQUAL(0, buffer->hidden);
    LONGS_EQUAL(0, buffer->zoomed);
    LONGS_EQUAL(1, buffer->print_hooks_enabled);
    LONGS_EQUAL(1, buffer->day_change);
    LONGS_EQUAL(0, buffer->clear);
    LONGS_EQUAL(1, buffer->filter);
    POINTERS_EQUAL(&test_buffer_close_cb, buffer->close_callback);
    POINTERS_EQUAL(NULL, buffer->close_callback_pointer);
    POINTERS_EQUAL(NULL, buffer->close_callback_data);
    LONGS_EQUAL(0, buffer->closing);
    POINTERS_EQUAL(NULL, buffer->title);
    CHECK(buffer->own_lines);
    POINTERS_EQUAL(NULL, buffer->own_lines->first_line);
    POINTERS_EQUAL(NULL, buffer->own_lines->last_line);
    POINTERS_EQUAL(NULL, buffer->own_lines->last_read_line);
    LONGS_EQUAL(0, buffer->next_line_id);
    LONGS_EQUAL(0, buffer->time_for_each_line);
    LONGS_EQUAL(2, buffer->chat_refresh_needed);
    LONGS_EQUAL(0, buffer->nicklist);
    LONGS_EQUAL(0, buffer->nicklist_case_sensitive);
    CHECK(buffer->nicklist_root);
    STRCMP_EQUAL("root", buffer->nicklist_root->name);
    LONGS_EQUAL(0, buffer->nicklist_max_length);
    LONGS_EQUAL(1, buffer->nicklist_display_groups);
    LONGS_EQUAL(0, buffer->nicklist_count);
    LONGS_EQUAL(0, buffer->nicklist_visible_count);
    LONGS_EQUAL(0, buffer->nicklist_groups_count);
    LONGS_EQUAL(0, buffer->nicklist_groups_visible_count);
    LONGS_EQUAL(0, buffer->nicklist_nicks_count);
    LONGS_EQUAL(0, buffer->nicklist_nicks_visible_count);
    POINTERS_EQUAL(NULL, buffer->nickcmp_callback);
    POINTERS_EQUAL(NULL, buffer->nickcmp_callback_pointer);
    POINTERS_EQUAL(NULL, buffer->nickcmp_callback_data);
    LONGS_EQUAL(1, buffer->input);
    POINTERS_EQUAL(&test_buffer_input_cb, buffer->input_callback);
    POINTERS_EQUAL(NULL, buffer->input_callback_pointer);
    POINTERS_EQUAL(NULL, buffer->input_callback_data);
    LONGS_EQUAL(0, buffer->input_get_unknown_commands);
    LONGS_EQUAL(0, buffer->input_get_empty);
    LONGS_EQUAL(0, buffer->input_multiline);
    STRCMP_EQUAL("", buffer->input_buffer);
    CHECK(buffer->input_buffer_alloc > 0);
    LONGS_EQUAL(0, buffer->input_buffer_size);
    LONGS_EQUAL(0, buffer->input_buffer_length);
    LONGS_EQUAL(0, buffer->input_buffer_pos);
    LONGS_EQUAL(0, buffer->input_buffer_1st_display);
    CHECK(buffer->input_undo_snap);
    POINTERS_EQUAL(NULL, buffer->input_undo_snap->data);
    LONGS_EQUAL(0, buffer->input_undo_snap->pos);
    POINTERS_EQUAL(NULL, buffer->input_undo_snap->prev_undo);
    POINTERS_EQUAL(NULL, buffer->input_undo_snap->next_undo);
    POINTERS_EQUAL(NULL, buffer->input_undo);
    POINTERS_EQUAL(NULL, buffer->last_input_undo);
    POINTERS_EQUAL(NULL, buffer->ptr_input_undo);
    LONGS_EQUAL(0, buffer->input_undo_count);
    CHECK(buffer->completion);
    POINTERS_EQUAL(NULL, buffer->history);
    POINTERS_EQUAL(NULL, buffer->last_history);
    POINTERS_EQUAL(NULL, buffer->ptr_history);
    LONGS_EQUAL(0, buffer->num_history);
    LONGS_EQUAL(GUI_TEXT_SEARCH_DISABLED, buffer->text_search);
    LONGS_EQUAL(0, buffer->text_search_exact);
    LONGS_EQUAL(0, buffer->text_search_regex);
    POINTERS_EQUAL(NULL, buffer->text_search_regex_compiled);
    LONGS_EQUAL(0, buffer->text_search_where);
    LONGS_EQUAL(0, buffer->text_search_found);
    POINTERS_EQUAL(NULL, buffer->text_search_input);
    POINTERS_EQUAL(NULL, buffer->highlight_words);
    POINTERS_EQUAL(NULL, buffer->highlight_regex);
    POINTERS_EQUAL(NULL, buffer->highlight_regex_compiled);
    POINTERS_EQUAL(NULL, buffer->highlight_tags_restrict);
    LONGS_EQUAL(0, buffer->highlight_tags_restrict_count);
    POINTERS_EQUAL(NULL, buffer->highlight_tags_restrict_array);
    POINTERS_EQUAL(NULL, buffer->highlight_tags);
    LONGS_EQUAL(0, buffer->highlight_tags_count);
    POINTERS_EQUAL(NULL, buffer->highlight_tags_array);
    POINTERS_EQUAL(NULL, buffer->hotlist);
    POINTERS_EQUAL(NULL, buffer->hotlist_removed);
    CHECK(buffer->hotlist_max_level_nicks);
    POINTERS_EQUAL(NULL, buffer->keys);
    POINTERS_EQUAL(NULL, buffer->last_key);
    LONGS_EQUAL(0, buffer->keys_count);
    CHECK(buffer->local_variables);
    LONGS_EQUAL(2, buffer->local_variables->items_count);
    STRCMP_EQUAL("core",
                 (const char *)hashtable_get (buffer->local_variables, "plugin"));
    STRCMP_EQUAL(TEST_BUFFER_NAME,
                 (const char *)hashtable_get (buffer->local_variables, "name"));
    POINTERS_EQUAL(gui_buffers, buffer->prev_buffer);
    POINTERS_EQUAL(NULL, buffer->next_buffer);
    LONGS_EQUAL(2, gui_buffers_count);
    gui_buffer_close (buffer);

    hashtable_set (properties, "title", "the buffer title");
    hashtable_set (properties, "short_name", "the_short_name");
    hashtable_set (properties, "localvar_set_test", "value");
    hashtable_set (properties, "key_bind_meta-y", "/test_y arg1 arg2");
    hashtable_set (properties, "key_bind_meta-z", "/test_z arg1 arg2");
    buffer = gui_buffer_new_props (NULL, TEST_BUFFER_NAME, properties,
                                   &test_buffer_input_cb, NULL, NULL,
                                   &test_buffer_close_cb, NULL, NULL);
    STRCMP_EQUAL("the buffer title", buffer->title);
    STRCMP_EQUAL("the_short_name", buffer->short_name);
    LONGS_EQUAL(3, buffer->local_variables->items_count);
    STRCMP_EQUAL("value",
                 (const char *)hashtable_get (buffer->local_variables, "test"));
    CHECK(buffer->keys);
    STRCMP_EQUAL("meta-y", buffer->keys->key);
    STRCMP_EQUAL("/test_y arg1 arg2", buffer->keys->command);
    POINTERS_EQUAL(NULL, buffer->keys->prev_key);
    CHECK(buffer->keys->next_key);
    STRCMP_EQUAL("meta-z", buffer->keys->next_key->key);
    STRCMP_EQUAL("/test_z arg1 arg2", buffer->keys->next_key->command);
    POINTERS_EQUAL(buffer->keys, buffer->keys->next_key->prev_key);
    POINTERS_EQUAL(NULL, buffer->keys->next_key->next_key);
    LONGS_EQUAL(2, buffer->keys_count);
    gui_buffer_close (buffer);

    hashtable_free (properties);
}

/*
 * Tests functions:
 *   gui_buffer_new
 */

TEST(GuiBuffer, New)
{
    struct t_gui_buffer *buffer;

    buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                             &test_buffer_input_cb, NULL, NULL,
                             &test_buffer_close_cb, NULL, NULL);
    CHECK(buffer);
    POINTERS_EQUAL(NULL, buffer->plugin);
    POINTERS_EQUAL(NULL, buffer->plugin_name_for_upgrade);
    LONGS_EQUAL(2, buffer->number);
    LONGS_EQUAL(0, buffer->layout_number);
    LONGS_EQUAL(0, buffer->layout_number_merge_order);
    STRCMP_EQUAL(TEST_BUFFER_NAME, buffer->name);
    STRCMP_EQUAL("core." TEST_BUFFER_NAME, buffer->full_name);
    POINTERS_EQUAL(NULL, buffer->old_full_name);
    POINTERS_EQUAL(NULL, buffer->short_name);
    gui_buffer_close (buffer);
}

/*
 * Tests functions:
 *   gui_buffer_user_input_cb
 *   gui_buffer_user_close_cb
 *   gui_buffer_new_user
 */

TEST(GuiBuffer, NewUser)
{
    int type;
    struct t_gui_buffer *buffer;
    struct t_hook *signal_input, *signal_closing;

    for (type = 0; type < GUI_BUFFER_NUM_TYPES; type++)
    {
        signal_input = hook_signal (NULL,
                                    "buffer_user_input_" TEST_BUFFER_NAME,
                                    &signal_buffer_user_input_cb, NULL, NULL);
        signal_closing = hook_signal (NULL,
                                      "buffer_user_closing_" TEST_BUFFER_NAME,
                                      &signal_buffer_user_closing_cb, NULL, NULL);

        /* test creation of user buffer */
        buffer = gui_buffer_new_user (TEST_BUFFER_NAME,
                                      (enum t_gui_buffer_type)type);
        CHECK(buffer);
        STRCMP_EQUAL(TEST_BUFFER_NAME, buffer->name);
        STRCMP_EQUAL("core." TEST_BUFFER_NAME, buffer->full_name);
        POINTERS_EQUAL(&gui_buffer_user_input_cb, buffer->input_callback);
        POINTERS_EQUAL(&gui_buffer_user_close_cb, buffer->close_callback);

        /* test signal "buffer_user_input_test" */
        signal_buffer_user_input[0] = '\0';
        input_data (buffer, "something", NULL, 0);
        STRCMP_EQUAL("something", signal_buffer_user_input);

        /* test signal "buffer_user_closing_test" */
        signal_buffer_user_closing = 0;
        gui_buffer_close (buffer);
        LONGS_EQUAL(1, signal_buffer_user_closing);

        /* create the buffer again */
        buffer = gui_buffer_new_user (TEST_BUFFER_NAME,
                                      (enum t_gui_buffer_type)type);

        /* close the buffer by sending "q" */
        signal_buffer_user_input[0] = '\0';
        signal_buffer_user_closing = 0;
        input_data (buffer, "q", NULL, 0);
        STRCMP_EQUAL("q", signal_buffer_user_input);
        LONGS_EQUAL(1, signal_buffer_user_closing);

        /* create the buffer again */
        buffer = gui_buffer_new_user (TEST_BUFFER_NAME,
                                      (enum t_gui_buffer_type)type);

        /* hook a signal that eats the input */
        unhook (signal_input);
        signal_input = hook_signal (NULL,
                                    "buffer_user_input_" TEST_BUFFER_NAME,
                                    &signal_buffer_user_input_eat_cb, NULL, NULL);

        /*
         * try to close the buffer by sending "q": it should not close it
         * because the input signal callback as returned WEECHAT_RC_OK_EAT
         */
        signal_buffer_user_input[0] = '\0';
        signal_buffer_user_closing = 0;
        input_data (buffer, "q", NULL, 0);
        STRCMP_EQUAL("q", signal_buffer_user_input);
        LONGS_EQUAL(0, signal_buffer_user_closing);

        gui_buffer_close (buffer);

        unhook (signal_input);
        unhook (signal_closing);
    }
}

/*
 * Tests functions:
 *   gui_buffer_user_set_callbacks
 */

TEST(GuiBuffer, UserSetCallbacks)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_valid
 */

TEST(GuiBuffer, Valid)
{
    struct t_gui_buffer *buffer;

    buffer = gui_buffer_new (NULL, TEST_BUFFER_NAME,
                             &test_buffer_input_cb, NULL, NULL,
                             &test_buffer_close_cb, NULL, NULL);

    LONGS_EQUAL(0, gui_buffer_valid ((struct t_gui_buffer *)0x1));
    LONGS_EQUAL(0, gui_buffer_valid (buffer + 1));

    /* NULL pointer is considered valid: to print on core buffer */
    LONGS_EQUAL(1, gui_buffer_valid (NULL));

    LONGS_EQUAL(1, gui_buffer_valid (gui_buffers));
    LONGS_EQUAL(1, gui_buffer_valid (buffer));

    gui_buffer_close (buffer);
}

/*
 * Tests functions:
 *   gui_buffer_string_replace_local_var
 */

TEST(GuiBuffer, StringReplaceLocalVar)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_match_list
 */

TEST(GuiBuffer, MatchList)
{
    LONGS_EQUAL(0, gui_buffer_match_list (NULL, NULL));
    LONGS_EQUAL(0, gui_buffer_match_list (gui_buffers, NULL));
    LONGS_EQUAL(0, gui_buffer_match_list (gui_buffers, ""));
    LONGS_EQUAL(0, gui_buffer_match_list (NULL, "*"));

    LONGS_EQUAL(1, gui_buffer_match_list (gui_buffers, "*"));
    LONGS_EQUAL(1, gui_buffer_match_list (gui_buffers, "core.*"));
    LONGS_EQUAL(1, gui_buffer_match_list (gui_buffers, "*.wee*"));
    LONGS_EQUAL(1, gui_buffer_match_list (gui_buffers, "*,!*test*"));
    LONGS_EQUAL(1, gui_buffer_match_list (gui_buffers, "*,!*test*,!*abc*"));

    LONGS_EQUAL(0, gui_buffer_match_list (gui_buffers, "*,!*wee*"));
    LONGS_EQUAL(0, gui_buffer_match_list (gui_buffers, "*,!*abc*,!*wee*"));
}

/*
 * Tests functions:
 *   gui_buffer_set_plugin_for_upgrade
 */

TEST(GuiBuffer, SetPluginForUpgrade)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_property_in_list
 */

TEST(GuiBuffer, PropertyInList)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_get_integer
 */

TEST(GuiBuffer, GetInteger)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_get_string
 */

TEST(GuiBuffer, GetString)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_get_pointer
 */

TEST(GuiBuffer, GetPointer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_ask_chat_refresh
 */

TEST(GuiBuffer, AskChatRefresh)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_name
 */

TEST(GuiBuffer, SetName)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_short_name
 */

TEST(GuiBuffer, SetShortName)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_type
 */

TEST(GuiBuffer, SetType)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_title
 */

TEST(GuiBuffer, SetTitle)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_time_for_each_line
 */

TEST(GuiBuffer, SetTimeForEachLine)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_nicklist
 */

TEST(GuiBuffer, SetNicklist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_nicklist_case_sensitive
 */

TEST(GuiBuffer, SetNicklistCaseSensitive)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_nicklist_display_groups
 */

TEST(GuiBuffer, SetNicklistDisplayGroups)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_highlight_words
 */

TEST(GuiBuffer, SetHighlightWords)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_highlight_words_list
 */

TEST(GuiBuffer, SetHighlightWordsList)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_add_highlight_words
 */

TEST(GuiBuffer, AddHighlightWords)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_remove_highlight_words
 */

TEST(GuiBuffer, RemoveHighlightWords)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_highlight_regex
 */

TEST(GuiBuffer, SetHighlightRegex)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_highlight_tags_restrict
 */

TEST(GuiBuffer, SetHighlightTagsRestrict)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_highlight_tags
 */

TEST(GuiBuffer, SetHighlightTags)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_hotlist_max_level_nicks
 */

TEST(GuiBuffer, SetHotlistMaxLevelNicks)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_add_hotlist_max_level_nicks
 */

TEST(GuiBuffer, AddHotlistMaxLevelNicks)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_remove_hotlist_max_level_nicks
 */

TEST(GuiBuffer, RemoveHotlistMaxLevelNicks)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_input_get_unknown_commands
 */

TEST(GuiBuffer, SetInputGetUnknownCommands)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_input_get_empty
 */

TEST(GuiBuffer, SetInputGetEmpty)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_input_multiline
 */

TEST(GuiBuffer, SetInputMultiline)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_unread
 */

TEST(GuiBuffer, SetUnread)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set
 */

TEST(GuiBuffer, Set)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_pointer
 */

TEST(GuiBuffer, SetPointer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_compute_num_displayed
 */

TEST(GuiBuffer, ComputeNumDisplayed)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_add_value_num_displayed
 */

TEST(GuiBuffer, AddValueNumDisplayed)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_is_main
 */

TEST(GuiBuffer, IsMain)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_search_main
 */

TEST(GuiBuffer, SearchMain)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_search_by_full_name
 */

TEST(GuiBuffer, SearchByFullName)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_search_by_name
 */

TEST(GuiBuffer, SearchByName)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_search_by_partial_name
 */

TEST(GuiBuffer, SearchByPartialName)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_search_by_number
 */

TEST(GuiBuffer, SearchByNumber)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_search_by_number_or_name
 */

TEST(GuiBuffer, SearchByNumberOrName)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_search_range
 */

TEST(GuiBuffer, SearchRange)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_count_merged_buffers
 */

TEST(GuiBuffer, CountMergedBuffers)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_clear
 */

TEST(GuiBuffer, Clear)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_clear_all
 */

TEST(GuiBuffer, ClearAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_get_next_active_buffer
 */

TEST(GuiBuffer, GetNextActiveBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_get_previous_active_buffer
 */

TEST(GuiBuffer, GetPreviousActiveBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_visited_get_index_previous
 */

TEST(GuiBuffer, VisitedGetIndexPrevious)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_visited_get_index_next
 */

TEST(GuiBuffer, VisitedGetIndexNext)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_visited_search
 */

TEST(GuiBuffer, VisitedSearch)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_visited_search_by_number
 */

TEST(GuiBuffer, VisitedSearchByNumber)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_visited_remove
 */

TEST(GuiBuffer, VisitedRemove)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_visited_remove_by_buffer
 */

TEST(GuiBuffer, VisitedRemoveByBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_visited_remove_all
 */

TEST(GuiBuffer, VisitedRemoveAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_visited_add
 */

TEST(GuiBuffer, VisitedAdd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_close
 */

TEST(GuiBuffer, Close)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_switch_by_number
 */

TEST(GuiBuffer, SwitchByNumber)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_set_active_buffer
 */

TEST(GuiBuffer, SetActiveBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_switch_active_buffer
 */

TEST(GuiBuffer, SwitchActiveBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_switch_active_buffer_previous
 */

TEST(GuiBuffer, SwitchActiveBufferPrevious)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_zoom
 */

TEST(GuiBuffer, Zoom)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_renumber
 */

TEST(GuiBuffer, Renumber)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_move_to_number
 */

TEST(GuiBuffer, MoveToNumber)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_swap
 */

TEST(GuiBuffer, Swap)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_merge
 */

TEST(GuiBuffer, Merge)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_unmerge
 */

TEST(GuiBuffer, Unmerge)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_unmerge_all
 */

TEST(GuiBuffer, UnmergeAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_hide
 */

TEST(GuiBuffer, Hide)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_hide_all
 */

TEST(GuiBuffer, HideAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_unhide
 */

TEST(GuiBuffer, Unhide)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_unhide_all
 */

TEST(GuiBuffer, UnhideAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_sort_by_layout_number
 */

TEST(GuiBuffer, SortByLayoutNumber)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_undo_snap
 */

TEST(GuiBuffer, UndoSnap)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_undo_snap_free
 */

TEST(GuiBuffer, UndoSnapFree)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_undo_add
 */

TEST(GuiBuffer, UndoAdd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_undo_free
 */

TEST(GuiBuffer, UndoFree)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_undo_free_all
 */

TEST(GuiBuffer, UndoFreeAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_input_move_to_buffer
 */

TEST(GuiBuffer, InputMoveToBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_jump_smart
 */

TEST(GuiBuffer, JumpSmart)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_jump_last_visible_number
 */

TEST(GuiBuffer, JumpLastVisibleNumber)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_jump_last_buffer_displayed
 */

TEST(GuiBuffer, JumpLastBufferDisplayed)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_jump_visited_by_index
 */

TEST(GuiBuffer, JumpVisitedByIndex)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_jump_previously_visited_buffer
 */

TEST(GuiBuffer, JumpPreviouslyVisitedBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_jump_next_visited_buffer
 */

TEST(GuiBuffer, JumpNextVisitedBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_hdata_buffer_cb
 */

TEST(GuiBuffer, HdataBufferCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_hdata_input_undo_cb
 */

TEST(GuiBuffer, HdataInputUndoCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_hdata_buffer_visited_cb
 */

TEST(GuiBuffer, HdataBufferVisitedCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_add_to_infolist
 */

TEST(GuiBuffer, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_dump_hexa
 */

TEST(GuiBuffer, DumpHexa)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_buffer_print_log
 */

TEST(GuiBuffer, PrintLog)
{
    /* TODO: write tests */
}
