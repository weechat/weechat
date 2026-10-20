/*
 * SPDX-FileCopyrightText: 2024-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test hook signal functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include "src/core/weechat.h"
#include "src/core/core-hook.h"
#include "src/core/hook/hook-signal.h"
#include "src/plugins/weechat-plugin.h"

extern int hook_signal_match (const char *signal, struct t_hook *hook);
}

TEST_GROUP(HookSignal)
{
    /*
     * Callback for the signal hooks (never called in tests).
     */

    static int
    test_signal_cb (const void *pointer, void *data, const char *signal,
                    const char *type_data, void *signal_data)
    {
        /* Make C++ compiler happy. */
        (void) pointer;
        (void) data;
        (void) signal;
        (void) type_data;
        (void) signal_data;

        return WEECHAT_RC_OK;
    }
};

/*
 * Test functions:
 *   hook_signal_get_description
 */

TEST(HookSignal, GetDescription)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_signal
 */

TEST(HookSignal, Signal)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_signal_match
 */

TEST(HookSignal, Match)
{
    struct t_hook *hook;

    /* Masks without wildcard (exact name) and with wildcards. */
    hook = hook_signal (NULL, "test_signal;Test_Upper;irc_in_*;*_end",
                        &test_signal_cb, NULL, NULL);
    CHECK(hook);

    LONGS_EQUAL(0, hook_signal_match ("", hook));
    LONGS_EQUAL(0, hook_signal_match ("test", hook));
    LONGS_EQUAL(0, hook_signal_match ("test_signal2", hook));
    LONGS_EQUAL(0, hook_signal_match ("xtest_signal", hook));
    LONGS_EQUAL(0, hook_signal_match ("irc_out_privmsg", hook));
    LONGS_EQUAL(0, hook_signal_match ("end_buffer", hook));

    LONGS_EQUAL(1, hook_signal_match ("test_signal", hook));
    LONGS_EQUAL(1, hook_signal_match ("TEST_SIGNAL", hook));
    LONGS_EQUAL(1, hook_signal_match ("test_upper", hook));
    LONGS_EQUAL(1, hook_signal_match ("irc_in_privmsg", hook));
    LONGS_EQUAL(1, hook_signal_match ("IRC_IN_PRIVMSG", hook));
    LONGS_EQUAL(1, hook_signal_match ("buffer_end", hook));

    unhook (hook);

    /* Mask matching any signal. */
    hook = hook_signal (NULL, "*", &test_signal_cb, NULL, NULL);
    CHECK(hook);

    LONGS_EQUAL(1, hook_signal_match ("test", hook));
    LONGS_EQUAL(1, hook_signal_match ("irc_in_privmsg", hook));

    unhook (hook);
}

/*
 * Test functions:
 *   hook_signal_extract_flags
 */

TEST(HookSignal, ExtractFlags)
{
    const char *ptr_signal;
    char signal[128];
    int stop_on_error, ignore_eat;

    hook_signal_extract_flags (NULL, NULL, NULL, NULL);

    /* No flags */
    snprintf (signal, sizeof (signal), "test");
    stop_on_error = -1;
    ignore_eat = -1;
    hook_signal_extract_flags (signal, &ptr_signal, &stop_on_error, &ignore_eat);
    STRCMP_EQUAL("test", ptr_signal);
    LONGS_EQUAL(0, stop_on_error);
    LONGS_EQUAL(0, ignore_eat);

    /* Invalid flags (missing "]") */
    snprintf (signal, sizeof (signal), "[flags:test");
    stop_on_error = -1;
    ignore_eat = -1;
    hook_signal_extract_flags (signal, &ptr_signal, &stop_on_error, &ignore_eat);
    STRCMP_EQUAL("[flags:test", ptr_signal);
    LONGS_EQUAL(0, stop_on_error);
    LONGS_EQUAL(0, ignore_eat);

    /* Unknown flag */
    snprintf (signal, sizeof (signal), "[flags:flag1]test");
    stop_on_error = -1;
    ignore_eat = -1;
    hook_signal_extract_flags (signal, &ptr_signal, &stop_on_error, &ignore_eat);
    STRCMP_EQUAL("test", ptr_signal);
    LONGS_EQUAL(0, stop_on_error);
    LONGS_EQUAL(0, ignore_eat);

    /* Unknown flags */
    snprintf (signal, sizeof (signal), "[flags:flag1,flag2]test");
    stop_on_error = -1;
    ignore_eat = -1;
    hook_signal_extract_flags (signal, &ptr_signal, &stop_on_error, &ignore_eat);
    STRCMP_EQUAL("test", ptr_signal);
    LONGS_EQUAL(0, stop_on_error);
    LONGS_EQUAL(0, ignore_eat);

    /* flag "stop_on_error" */
    snprintf (signal, sizeof (signal), "[flags:stop_on_error]test");
    stop_on_error = -1;
    ignore_eat = -1;
    hook_signal_extract_flags (signal, &ptr_signal, &stop_on_error, &ignore_eat);
    STRCMP_EQUAL("test", ptr_signal);
    LONGS_EQUAL(1, stop_on_error);
    LONGS_EQUAL(0, ignore_eat);

    /* flag "ignore_eat" */
    snprintf (signal, sizeof (signal), "[flags:ignore_eat]test");
    stop_on_error = -1;
    ignore_eat = -1;
    hook_signal_extract_flags (signal, &ptr_signal, &stop_on_error, &ignore_eat);
    STRCMP_EQUAL("test", ptr_signal);
    LONGS_EQUAL(0, stop_on_error);
    LONGS_EQUAL(1, ignore_eat);

    /* flags "stop_on_error" and "ignore_eat" */
    snprintf (signal, sizeof (signal), "[flags:stop_on_error,ignore_eat]test");
    stop_on_error = -1;
    ignore_eat = -1;
    hook_signal_extract_flags (signal, &ptr_signal, &stop_on_error, &ignore_eat);
    STRCMP_EQUAL("test", ptr_signal);
    LONGS_EQUAL(1, stop_on_error);
    LONGS_EQUAL(1, ignore_eat);
}

/*
 * Test functions:
 *   hook_signal_send
 */

TEST(HookSignal, Send)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_signal_free_data
 */

TEST(HookSignal, FreeData)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_signal_add_to_infolist
 */

TEST(HookSignal, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_signal_print_log
 */

TEST(HookSignal, PrintLog)
{
    /* TODO: write tests */
}
