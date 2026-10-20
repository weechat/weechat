/*
 * SPDX-FileCopyrightText: 2024-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test hook url functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "src/core/weechat.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/core/core-string.h"
#include "src/core/core-util.h"
#include "src/plugins/weechat-plugin.h"
}

int hook_url_test_cb_count = 0;
char *hook_url_test_cb_output = NULL;

TEST_GROUP(HookUrl)
{
    /*
     * Callback for the URL hook: save the output.
     */

    static int
    test_url_cb (const void *pointer, void *data, const char *url,
                 struct t_hashtable *options, struct t_hashtable *output)
    {
        const char *ptr_output;

        /* Make C++ compiler happy. */
        (void) pointer;
        (void) data;
        (void) url;
        (void) options;

        hook_url_test_cb_count++;
        ptr_output = (const char *)hashtable_get (output, "output");
        free (hook_url_test_cb_output);
        hook_url_test_cb_output = (ptr_output) ? strdup (ptr_output) : NULL;

        return WEECHAT_RC_OK;
    }

    void setup ()
    {
        hook_url_test_cb_count = 0;
        hook_url_test_cb_output = NULL;
    }

    void teardown ()
    {
        free (hook_url_test_cb_output);
        hook_url_test_cb_output = NULL;
    }
};

/*
 * Test functions:
 *   hook_url_get_description
 */

TEST(HookUrl, GetDescription)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_url_hashtable_map_cb
 */

TEST(HookUrl, HashtableMapCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_url_run_callback
 */

TEST(HookUrl, RunCallback)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_url_thread_cleanup
 */

TEST(HookUrl, ThreadCleanup)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_url_transfer_thread
 */

TEST(HookUrl, TransferThread)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_url_check_end
 *   hook_url_fd_cb
 */

TEST(HookUrl, FdCb)
{
    struct t_hook *hook;
    struct timeval time_start, time_now;
    char *filename, path[PATH_MAX], url[PATH_MAX + 16];
    FILE *file;

    filename = string_eval_path_home ("${weechat_data_dir}/test_hook_url.txt",
                                      NULL, NULL, NULL);
    CHECK(filename);
    file = fopen (filename, "w");
    CHECK(file);
    fputs ("test hook_url", file);
    fclose (file);
    CHECK(realpath (filename, path));
    snprintf (url, sizeof (url), "file://%s", path);

    hook = hook_url (NULL, url, NULL, 10000, &test_url_cb, NULL, NULL);
    CHECK(hook);
    CHECK(HOOK_URL(hook, hook_fd));

    /* Callback is called only in main thread. */
    LONGS_EQUAL(0, hook_url_test_cb_count);

    /* End of thread is detected with the pipe only, without the timer. */
    gettimeofday (&time_start, NULL);
    while (hook_url_test_cb_count == 0)
    {
        hook_fd_exec ();
        gettimeofday (&time_now, NULL);
        if (util_timeval_diff (&time_start, &time_now) >= 5 * 1000 * 1000)
            break;
    }
    LONGS_EQUAL(1, hook_url_test_cb_count);
    STRCMP_EQUAL("test hook_url", hook_url_test_cb_output);

    unlink (filename);
    free (filename);
}

/*
 * Test functions:
 *   hook_url_timer_cb
 */

TEST(HookUrl, TimerCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_url_transfer
 */

TEST(HookUrl, Transfer)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_url
 */

TEST(HookUrl, Url)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_url_free_data
 */

TEST(HookUrl, FreeData)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_url_add_to_infolist
 */

TEST(HookUrl, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_url_print_log
 */

TEST(HookUrl, PrintLog)
{
    /* TODO: write tests */
}
