/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test trigger callback functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"
#include "tests-record.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/core/core-string.h"
#include "src/core/hook/hook-hsignal.h"
#include "src/core/hook/hook-info.h"
#include "src/core/hook/hook-info-hashtable.h"
#include "src/core/hook/hook-modifier.h"
#include "src/core/hook/hook-signal.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-chat.h"
#include "src/gui/gui-color.h"
#include "src/gui/gui-line.h"
#include "src/gui/gui-window.h"
#include "src/plugins/plugin.h"
#include "src/plugins/trigger/trigger.h"
#include "src/plugins/trigger/trigger-buffer.h"
#include "src/plugins/trigger/trigger-callback.h"

extern struct t_weechat_plugin *weechat_trigger_plugin;
}

TEST_GROUP(TriggerCallback)
{
    struct t_trigger *trigger;

    void setup ()
    {
        trigger = NULL;
        record_start ();
    }

    void teardown ()
    {
        record_stop ();
        if (trigger && trigger_search (trigger->name))
            trigger_free (trigger);
        trigger_enabled = 1;
    }

    /* Create a trigger (enabled). */
    struct t_trigger *new_trigger (const char *name, const char *hook,
                                   const char *arguments,
                                   const char *conditions,
                                   const char *regex,
                                   const char *command,
                                   const char *return_code,
                                   const char *post_action)
    {
        trigger = trigger_new (name, "on", hook, arguments, conditions,
                               regex, command, return_code, post_action);
        CHECK(trigger);
        return trigger;
    }

    /* Return the message of last line in core buffer (without colors). */
    char *last_line_core ()
    {
        struct t_gui_line *ptr_line;

        ptr_line = gui_buffers->own_lines->last_line;
        return (ptr_line) ?
            gui_color_decode (ptr_line->data->message, NULL) : NULL;
    }
};

/*
 * Test functions:
 *   trigger_callback_signal_cb
 *   trigger_callback_set_common_vars
 *   trigger_callback_check_conditions
 *   trigger_callback_regex
 *   trigger_callback_regex_replace
 *   trigger_callback_regex_translate_chars
 *   trigger_callback_run_command
 *   trigger_callback_execute
 */

TEST(TriggerCallback, Signal)
{
    int value;

    new_trigger ("test_signal", "signal", "test_trigger_signal",
                 "${tg_signal_data} != skip",
                 "/a/b/ y/d/D/",
                 "/print -core sig:${tg_trigger_name}:${tg_hook_type}:"
                 "${tg_signal}:${tg_signal_data}",
                 "ok_eat", "none");

    /* String. */
    LONGS_EQUAL(WEECHAT_RC_OK_EAT,
                hook_signal_send ("test_trigger_signal",
                                  WEECHAT_HOOK_SIGNAL_STRING,
                                  (void *)"data"));
    RECORD_CHECK_MSG("core.weechat", "",
                     "sig:test_signal:signal:test_trigger_signal:Dbtb",
                     NULL);
    LONGS_EQUAL(1, trigger->hook_count_cb);
    LONGS_EQUAL(1, trigger->hook_count_cmd);

    /* Conditions are false: nothing done, return code is OK. */
    record_start ();
    LONGS_EQUAL(WEECHAT_RC_OK,
                hook_signal_send ("test_trigger_signal",
                                  WEECHAT_HOOK_SIGNAL_STRING,
                                  (void *)"skip"));
    LONGS_EQUAL(0, record_count_messages ());
    LONGS_EQUAL(2, trigger->hook_count_cb);
    LONGS_EQUAL(1, trigger->hook_count_cmd);

    /* Integer. */
    value = 42;
    hook_signal_send ("test_trigger_signal", WEECHAT_HOOK_SIGNAL_INT, &value);
    RECORD_CHECK_MSG("core.weechat", "",
                     "sig:test_signal:signal:test_trigger_signal:42", NULL);
    record_start ();
    hook_signal_send ("test_trigger_signal", WEECHAT_HOOK_SIGNAL_INT, NULL);
    RECORD_CHECK_MSG("core.weechat", "",
                     "sig:test_signal:signal:test_trigger_signal:", NULL);

    /* Pointer. */
    record_start ();
    hook_signal_send ("test_trigger_signal", WEECHAT_HOOK_SIGNAL_POINTER,
                      (void *)0x1234);
    RECORD_CHECK_MSG("core.weechat", "",
                     "sig:test_signal:signal:test_trigger_signal:0x1234",
                     NULL);
    record_start ();
    hook_signal_send ("test_trigger_signal", WEECHAT_HOOK_SIGNAL_POINTER,
                      NULL);
    RECORD_CHECK_MSG("core.weechat", "",
                     "sig:test_signal:signal:test_trigger_signal:", NULL);
}

TEST(TriggerCallback, SignalIrc)
{
    new_trigger ("test_signal_irc", "signal",
                 "*,irc_in2_privmsg;*,irc_outtags_privmsg", "", "",
                 "/print -core irc:${server}:${nick}:${channel}:${text}:"
                 "${irc_server}",
                 "ok", "none");

    /* IRC message received. */
    hook_signal_send ("test_srv,irc_in2_privmsg", WEECHAT_HOOK_SIGNAL_STRING,
                      (void *)":alice!user@host PRIVMSG #test :hello");
    RECORD_CHECK_MSG("core.weechat", "", "irc:test_srv:alice:#test:hello:",
                     NULL);

    /* IRC message sent, with tags. */
    record_start ();
    hook_signal_send ("test_srv,irc_outtags_privmsg",
                      WEECHAT_HOOK_SIGNAL_STRING,
                      (void *)"tag1,tag2;PRIVMSG #chan :hi");
    RECORD_CHECK_MSG("core.weechat", "", "irc:test_srv::#chan:hi:", NULL);

    /* Tags without message. */
    record_start ();
    hook_signal_send ("test_srv,irc_outtags_privmsg",
                      WEECHAT_HOOK_SIGNAL_STRING,
                      (void *)"no message");
    RECORD_CHECK_MSG("core.weechat", "", "irc:::::", NULL);
}

TEST(TriggerCallback, Disabled)
{
    new_trigger ("test_disabled", "signal", "test_trigger_signal", "", "",
                 "/print -core signal received", "ok_eat", "none");

    /* Triggers disabled globally. */
    trigger_enabled = 0;
    LONGS_EQUAL(WEECHAT_RC_OK,
                hook_signal_send ("test_trigger_signal",
                                  WEECHAT_HOOK_SIGNAL_STRING,
                                  (void *)"data"));
    LONGS_EQUAL(0, record_count_messages ());
    trigger_enabled = 1;

    /* No trigger or trigger already running. */
    LONGS_EQUAL(WEECHAT_RC_OK,
                trigger_callback_signal_cb (NULL, NULL, "test",
                                            WEECHAT_HOOK_SIGNAL_STRING,
                                            (void *)"data"));
    trigger->hook_running = 1;
    LONGS_EQUAL(WEECHAT_RC_OK,
                trigger_callback_signal_cb (trigger, NULL, "test",
                                            WEECHAT_HOOK_SIGNAL_STRING,
                                            (void *)"data"));
    trigger->hook_running = 0;
    LONGS_EQUAL(0, record_count_messages ());
    LONGS_EQUAL(0, trigger->hook_count_cb);
}

TEST(TriggerCallback, PostAction)
{
    /* Disable the trigger after execution. */
    new_trigger ("test_post_disable", "signal", "test_trigger_signal", "", "",
                 "", "ok", "disable");
    hook_signal_send ("test_trigger_signal", WEECHAT_HOOK_SIGNAL_STRING,
                      (void *)"data");
    LONGS_EQUAL(0, config_file_option_boolean (
                    trigger->options[TRIGGER_OPTION_ENABLED]));
    trigger_free (trigger);

    /* Delete the trigger after execution. */
    new_trigger ("test_post_delete", "signal", "test_trigger_signal", "", "",
                 "", "ok", "delete");
    hook_signal_send ("test_trigger_signal", WEECHAT_HOOK_SIGNAL_STRING,
                      (void *)"data");
    POINTERS_EQUAL(NULL, trigger_search ("test_post_delete"));
    trigger = NULL;
}

TEST(TriggerCallback, Monitor)
{
    int old_debug;

    new_trigger ("test_monitor", "signal", "test_trigger_signal", "",
                 "/a/b/ /x/y/tg_new_var /c/d/tg_signal",
                 "/print -core monitor:${tg_signal_data}:${tg_new_var}",
                 "ok", "none");

    /* Open monitor buffer and enable debug (to display elapsed time). */
    trigger_buffer_open (NULL, 0);
    CHECK(trigger_buffer);
    old_debug = weechat_trigger_plugin->debug;
    weechat_trigger_plugin->debug = 1;

    hook_signal_send ("test_trigger_signal", WEECHAT_HOOK_SIGNAL_STRING,
                      (void *)"data");
    RECORD_CHECK_MSG("core.weechat", "", "monitor:dbtb:", NULL);
    RECORD_CHECK_MSG_REGEX("trigger.monitor", "regex 1");
    RECORD_CHECK_MSG_REGEX("trigger.monitor", "creating variable");
    RECORD_CHECK_MSG_REGEX("trigger.monitor", "running command");
    RECORD_CHECK_MSG_REGEX("trigger.monitor", "elapsed: init=");

    weechat_trigger_plugin->debug = old_debug;
    gui_buffer_close (trigger_buffer);
}

/*
 * Test functions:
 *   trigger_callback_hsignal_cb
 */

TEST(TriggerCallback, Hsignal)
{
    struct t_hashtable *hashtable;

    new_trigger ("test_hsignal", "hsignal", "test_trigger_hsignal", "",
                 "", "/print -core hsig:${tg_signal}:${key1}:${buffer[my_buffer].name}",
                 "ok", "none");

    /* Hashtable with strings. */
    hashtable = hashtable_new (32, WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING, NULL, NULL);
    hashtable_set (hashtable, "key1", "value1");
    hook_hsignal_send ("test_trigger_hsignal", hashtable);
    hashtable_free (hashtable);
    RECORD_CHECK_MSG("core.weechat", "",
                     "hsig:test_trigger_hsignal:value1:", NULL);

    /* Hashtable with pointers. */
    record_start ();
    hashtable = hashtable_new (32, WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_POINTER, NULL, NULL);
    hashtable_set (hashtable, "my_buffer", gui_buffers);
    hook_hsignal_send ("test_trigger_hsignal", hashtable);
    hashtable_free (hashtable);
    RECORD_CHECK_MSG("core.weechat", "",
                     "hsig:test_trigger_hsignal::weechat", NULL);

    /* Hashtable with other types: ignored. */
    record_start ();
    hashtable = hashtable_new (32, WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_INTEGER, NULL, NULL);
    hook_hsignal_send ("test_trigger_hsignal", hashtable);
    hashtable_free (hashtable);
    RECORD_CHECK_MSG("core.weechat", "", "hsig:test_trigger_hsignal::",
                     NULL);

    /* No hashtable. */
    record_start ();
    hook_hsignal_send ("test_trigger_hsignal", NULL);
    RECORD_CHECK_MSG("core.weechat", "", "hsig:test_trigger_hsignal::",
                     NULL);
}

/*
 * Test functions:
 *   trigger_callback_modifier_cb
 *   trigger_callback_set_tags
 */

TEST(TriggerCallback, Modifier)
{
    char *str;

    new_trigger ("test_modifier", "modifier", "test_trigger_modifier",
                 "${tg_modifier_data} != skip", "/abc/xyz/", "",
                 "ok", "none");

    WEE_TEST_STR("xyz def",
                 hook_modifier_exec (NULL, "test_trigger_modifier", "data",
                                     "abc def"));
    WEE_TEST_STR("abc def",
                 hook_modifier_exec (NULL, "test_trigger_modifier", "skip",
                                     "abc def"));
    WEE_TEST_STR("no match",
                 hook_modifier_exec (NULL, "test_trigger_modifier", "data",
                                     "no match"));
}

TEST(TriggerCallback, ModifierIrc)
{
    char *str;

    new_trigger ("test_modifier_irc", "modifier", "irc_in2_privmsg",
                 "${nick} == alice && ${channel} == #test && ${server} == srv",
                 "/hello/bye/", "", "ok", "none");

    WEE_TEST_STR(":alice!user@host PRIVMSG #test :bye",
                 hook_modifier_exec (NULL, "irc_in2_privmsg", "srv",
                                     ":alice!user@host PRIVMSG #test :hello"));
    WEE_TEST_STR(":bob!user@host PRIVMSG #test :hello",
                 hook_modifier_exec (NULL, "irc_in2_privmsg", "srv",
                                     ":bob!user@host PRIVMSG #test :hello"));
}

TEST(TriggerCallback, ModifierWeechatPrint)
{
    char *str;

    new_trigger ("test_modifier_print", "modifier", "weechat_print",
                 "${tg_buffer} == core.weechat && ${tg_plugin} == core "
                 "&& ${tg_tags} =~ ,test_modifier_print, "
                 "&& ${tg_tag_nick} == alice "
                 "&& ${tg_prefix_nocolor} == alice "
                 "&& ${tg_message_nocolor} == secret",
                 "/secret/******/", "", "ok", "none");

    /* Message modified. */
    gui_chat_printf_date_tags (NULL, 0, "test_modifier_print,nick_alice",
                               "%salice\tsecret", gui_color_get_custom ("red"));
    WEE_TEST_STR("******", last_line_core ());

    /* Tag "no_trigger": message not modified. */
    gui_chat_printf_date_tags (NULL, 0,
                               "test_modifier_print,nick_alice,no_trigger",
                               "alice\tsecret");
    WEE_TEST_STR("secret", last_line_core ());

    /* Conditions are false: message not modified. */
    gui_chat_printf_date_tags (NULL, 0, "test_modifier_print,nick_bob",
                               "alice\tsecret");
    WEE_TEST_STR("secret", last_line_core ());

    /* No prefix, invalid buffer pointer: not modified. */
    WEE_TEST_STR("secret",
                 hook_modifier_exec (NULL, "weechat_print", "xyz;tag",
                                     "secret"));
    WEE_TEST_STR("\tsecret",
                 hook_modifier_exec (NULL, "weechat_print", "",
                                     "\tsecret"));
}

/*
 * Test functions:
 *   trigger_callback_line_cb
 */

TEST(TriggerCallback, Line)
{
    struct t_hashtable *hashtable, *result;
    struct t_gui_line *ptr_line;
    char *str, str_ptr[64];

    new_trigger ("test_line", "line", "formatted;core.weechat;test_line",
                 "${tg_tag_nick} == alice",
                 "/hello/world/ /,test_line,/,test_line,added,/tags",
                 "", "ok", "none");

    gui_chat_printf_date_tags (NULL, 0, "test_line,nick_alice",
                               "alice\thello");
    ptr_line = gui_buffers->own_lines->last_line;
    WEE_TEST_STR("world", last_line_core ());
    LONGS_EQUAL(3, ptr_line->data->tags_count);
    STRCMP_EQUAL("test_line", ptr_line->data->tags_array[0]);
    STRCMP_EQUAL("added", ptr_line->data->tags_array[1]);
    STRCMP_EQUAL("nick_alice", ptr_line->data->tags_array[2]);

    /* Conditions are false: line not modified. */
    gui_chat_printf_date_tags (NULL, 0, "test_line,nick_bob", "bob\thello");
    WEE_TEST_STR("hello", last_line_core ());

    /* Invalid buffer pointer: nothing done. */
    hashtable = hashtable_new (32, WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING, NULL, NULL);
    hashtable_set (hashtable, "buffer", "xyz");
    POINTERS_EQUAL(NULL, trigger_callback_line_cb (trigger, NULL, hashtable));
    hashtable_set (hashtable, "buffer", "0xzzz");
    POINTERS_EQUAL(NULL, trigger_callback_line_cb (trigger, NULL, hashtable));

    /* Tag "no_trigger": nothing done. */
    snprintf (str_ptr, sizeof (str_ptr), "%p", (void *)gui_buffers);
    hashtable_set (hashtable, "buffer", str_ptr);
    hashtable_set (hashtable, "tags", "nick_alice,no_trigger");
    POINTERS_EQUAL(NULL, trigger_callback_line_cb (trigger, NULL, hashtable));

    /* Tags emptied by regex. */
    trigger_free (trigger);
    new_trigger ("test_line", "line", "formatted;core.weechat;test_line", "",
                 "/.*//tags", "", "ok", "none");
    hashtable_set (hashtable, "tags", "test_line");
    hashtable_set (hashtable, "message", "msg");
    result = trigger_callback_line_cb (trigger, NULL, hashtable);
    CHECK(result);
    STRCMP_EQUAL("", (const char *)hashtable_get (result, "tags"));
    hashtable_free (result);

    hashtable_free (hashtable);
}

/*
 * Test functions:
 *   trigger_callback_print_cb
 */

TEST(TriggerCallback, Print)
{
    new_trigger ("test_print", "print", "core.weechat;test_print", "", "",
                 "/print -core print:${tg_tags_count}:${tg_tag_nick}:"
                 "${tg_tag_prefix_nick}:${tg_tag_host}:${tg_tag_notify}:"
                 "${tg_notify}:${tg_msg_pv}:${tg_tag_irc_time}:"
                 "${tg_tag_irc_flag}:${tg_displayed}:${tg_highlight}:"
                 "${tg_prefix_nocolor}:${tg_message_nocolor}:${tg_tags}",
                 "ok", "none");

    gui_chat_printf_date_tags (
        NULL, 0,
        "test_print,nick_alice,prefix_nick_red,host_user@host,"
        "notify_private,irc_tag_time=2021,irc_tag_flag,irc_tag_=x",
        "%salice\thello", gui_color_get_custom ("red"));
    RECORD_CHECK_MSG("core.weechat", "",
                     "print:8:alice:red:user@host:private:private:0:2021::1:0:"
                     "alice:hello:,test_print,nick_alice,prefix_nick_red,"
                     "host_user@host,notify_private,irc_tag_time=2021,"
                     "irc_tag_flag,irc_tag_=x,",
                     NULL);

    /* Notify "none": "tg_notify" is not set. */
    record_start ();
    gui_chat_printf_date_tags (NULL, 0, "test_print,notify_none", "msg");
    RECORD_CHECK_MSG("core.weechat", "",
                     "print:2::::none:::::1:0::msg:,test_print,notify_none,",
                     NULL);

    /* Tag "no_trigger": nothing done. */
    record_start ();
    gui_chat_printf_date_tags (NULL, 0, "test_print,no_trigger", "msg");
    LONGS_EQUAL(1, record_count_messages ());

    /* Buffer not matching: nothing done. */
    LONGS_EQUAL(WEECHAT_RC_OK,
                trigger_callback_print_cb (trigger, NULL, NULL, 0, 0,
                                           0, NULL, 1, 0, "", "msg"));
}

/*
 * Test functions:
 *   trigger_callback_command_cb
 */

TEST(TriggerCallback, Command)
{
    new_trigger ("test_command", "command", "test_trigger_cmd;desc", "", "",
                 "/print -core cmd:${tg_argc}:${tg_argv0}:${tg_argv1}:"
                 "${tg_argv_eol1}:${tg_shell_argc}:${tg_shell_argv2}",
                 "ok", "none");

    run_cmd ("/test_trigger_cmd a \"b c\"");
    RECORD_CHECK_MSG("core.weechat", "",
                     "cmd:4:/test_trigger_cmd:a:a \"b c\":3:b c", NULL);

    /* Unterminated quote. */
    record_start ();
    run_cmd ("/test_trigger_cmd a \"b");
    RECORD_CHECK_MSG("core.weechat", "",
                     "cmd:3:/test_trigger_cmd:a:a \"b:3:b", NULL);
}

/*
 * Test functions:
 *   trigger_callback_command_run_cb
 */

TEST(TriggerCallback, CommandRun)
{
    new_trigger ("test_command_run", "command_run", "/test_trigger_run*", "",
                 "", "/print -core run:${tg_command}:${buffer.full_name}",
                 "ok_eat", "none");

    /* Command eaten: no error about unknown command. */
    run_cmd ("/test_trigger_run arg");
    RECORD_CHECK_MSG("core.weechat", "",
                     "run:/test_trigger_run arg:core.weechat", NULL);
    LONGS_EQUAL(1, record_count_messages ());
}

/*
 * Test functions:
 *   trigger_callback_timer_cb
 */

TEST(TriggerCallback, Timer)
{
    struct t_hook *ptr_hook;

    new_trigger ("test_timer", "timer", "3600000;0;2", "", "",
                 "/print -core timer:${tg_remaining_calls}:"
                 "${tg_date}", "ok", "none");
    CHECK(trigger->hooks[0]);

    LONGS_EQUAL(WEECHAT_RC_OK, trigger_callback_timer_cb (trigger, NULL, 1));
    RECORD_CHECK_MSG_REGEX("core.weechat", "^timer:1:[0-9]{4}-[0-9]{2}-");
    CHECK(trigger->hooks[0]);

    /* Last call: the hook pointer is removed from trigger. */
    ptr_hook = trigger->hooks[0];
    LONGS_EQUAL(WEECHAT_RC_OK, trigger_callback_timer_cb (trigger, NULL, 0));
    RECORD_CHECK_MSG_REGEX("core.weechat", "^timer:0:");
    POINTERS_EQUAL(NULL, trigger->hooks[0]);
    unhook (ptr_hook);
}

/*
 * Test functions:
 *   trigger_callback_config_cb
 */

TEST(TriggerCallback, Config)
{
    new_trigger ("test_config", "config", "plugins.var.test_trigger.*", "",
                 "", "/print -core cfg:${tg_option}:${tg_value}",
                 "ok", "none");

    run_cmd_quiet ("/set plugins.var.test_trigger.opt value1");
    RECORD_CHECK_MSG("core.weechat", "",
                     "cfg:plugins.var.test_trigger.opt:value1", NULL);
    run_cmd_quiet ("/unset plugins.var.test_trigger.opt");
}

/*
 * Test functions:
 *   trigger_callback_focus_cb
 */

TEST(TriggerCallback, Focus)
{
    struct t_hashtable *info, *result;
    char str_window[64], str_buffer[64];

    new_trigger ("test_focus", "focus", "chat", "", "",
                 "/print -core focus:${tg_trigger_name}:${tg_hook_type}:"
                 "${key}:${window.number}:${buffer.name}",
                 "ok", "none");

    info = hashtable_new (32, WEECHAT_HASHTABLE_STRING,
                          WEECHAT_HASHTABLE_STRING, NULL, NULL);
    snprintf (str_window, sizeof (str_window), "%p", (void *)gui_windows);
    snprintf (str_buffer, sizeof (str_buffer), "%p", (void *)gui_buffers);
    hashtable_set (info, "_window", str_window);
    hashtable_set (info, "_buffer", str_buffer);
    hashtable_set (info, "key", "value");

    result = trigger_callback_focus_cb (trigger, NULL, info);
    POINTERS_EQUAL(info, result);
    RECORD_CHECK_MSG("core.weechat", "",
                     "focus:test_focus:focus:value:1:weechat", NULL);

    /* Hashtable received is not changed. */
    LONGS_EQUAL(3, info->items_count);
    LONGS_EQUAL(0, hashtable_has_key (info, "tg_trigger_name"));
    LONGS_EQUAL(0, hashtable_has_key (info, "tg_hook_type"));

    /* Invalid pointers: ignored (current window/buffer are used). */
    record_start ();
    hashtable_set (info, "_window", "xyz");
    hashtable_set (info, "_buffer", "0xzzz");
    trigger_callback_focus_cb (trigger, NULL, info);
    RECORD_CHECK_MSG_REGEX("core.weechat", "^focus:test_focus:focus:value:");

    hashtable_free (info);
}

/*
 * Test functions:
 *   trigger_callback_info_cb
 */

TEST(TriggerCallback, Info)
{
    char *str;

    new_trigger ("test_info", "info", "test_trigger_info",
                 "${tg_arguments} != skip",
                 "/.*/${tg_info_name}:${tg_arguments}/", "", "ok", "none");

    WEE_TEST_STR("test_trigger_info:args",
                 hook_info_get (NULL, "test_trigger_info", "args"));
    WEE_TEST_STR("", hook_info_get (NULL, "test_trigger_info", "skip"));
}

/*
 * Test functions:
 *   trigger_callback_info_hashtable_cb
 */

TEST(TriggerCallback, InfoHashtable)
{
    struct t_hashtable *hashtable, *result;

    new_trigger ("test_info_hashtable", "info_hashtable",
                 "test_trigger_info_hashtable", "",
                 "/x/y/key1 /.*/${tg_info_name}/key2", "", "ok", "none");

    hashtable = hashtable_new (32, WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING, NULL, NULL);
    hashtable_set (hashtable, "key1", "xx");
    hashtable_set (hashtable, "key3", "unchanged");
    result = hook_info_get_hashtable (NULL, "test_trigger_info_hashtable",
                                      hashtable);
    CHECK(result);
    LONGS_EQUAL(2, result->items_count);
    STRCMP_EQUAL("yy", (const char *)hashtable_get (result, "key1"));
    STRCMP_EQUAL("test_trigger_info_hashtable",
                 (const char *)hashtable_get (result, "key2"));
    hashtable_free (result);
    hashtable_free (hashtable);
}
