/*
 * SPDX-FileCopyrightText: 2024-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test hook process functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include "src/core/weechat.h"
#include "src/core/core-hook.h"
#include "src/plugins/weechat-plugin.h"

extern void hook_process_child_read (struct t_hook *hook_process, int fd,
                                     int index_buffer,
                                     struct t_hook **hook_fd);
extern int hook_process_check_end (struct t_hook *hook_process);
}

int hook_process_test_cb_count = 0;
int hook_process_test_cb_rc = 0;
char *hook_process_test_cb_out = NULL;

TEST_GROUP(HookProcess)
{
    /*
     * Callback for the process hook: save the return code and output.
     */

    static int
    test_process_cb (const void *pointer, void *data, const char *command,
                     int return_code, const char *out, const char *err)
    {
        /* Make C++ compiler happy. */
        (void) pointer;
        (void) data;
        (void) command;
        (void) err;

        hook_process_test_cb_count++;
        hook_process_test_cb_rc = return_code;
        if (out)
        {
            free (hook_process_test_cb_out);
            hook_process_test_cb_out = strdup (out);
        }

        return WEECHAT_RC_OK;
    }

    /*
     * Create a process hook and fork the child.
     */

    struct t_hook *
    test_run_process (const char *command)
    {
        struct t_hook *hook;

        hook = hook_process (NULL, command, 0, &test_process_cb, NULL, NULL);
        CHECK(hook);
        hook_process_exec ();
        CHECK(HOOK_PROCESS(hook, child_pid) > 0);

        return hook;
    }

    /*
     * Wait for the end of the child process, without reaping it.
     */

    void
    test_wait_child (struct t_hook *hook)
    {
        siginfo_t info;
        int rc;

        do
        {
            memset (&info, 0, sizeof (info));
            rc = waitid (P_PID, HOOK_PROCESS(hook, child_pid), &info,
                         WEXITED | WNOWAIT);
        } while ((rc < 0) && (errno == EINTR));
        LONGS_EQUAL(0, rc);
    }

    void setup ()
    {
        hook_process_test_cb_count = 0;
        hook_process_test_cb_rc = 0;
        hook_process_test_cb_out = NULL;
    }

    void teardown ()
    {
        free (hook_process_test_cb_out);
        hook_process_test_cb_out = NULL;
    }
};

/*
 * Test functions:
 *   hook_process_get_description
 */

TEST(HookProcess, GetDescription)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_process_hashtable
 */

TEST(HookProcess, Hashtable)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_process
 */

TEST(HookProcess, Process)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_process_child
 */

TEST(HookProcess, Child)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_process_send_buffers
 */

TEST(HookProcess, SendBuffers)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_process_add_to_buffer
 */

TEST(HookProcess, AddToBuffer)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_process_child_read
 */

TEST(HookProcess, ChildRead)
{
    struct t_hook *hook;
    int i;

    /* EOF on stdout and stderr ends the process, without the timer. */
    hook = test_run_process ("sh -c 'echo test; exit 3'");
    test_wait_child (hook);
    for (i = 0; (hook_process_test_cb_count == 0) && (i < 100); i++)
    {
        if (HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDOUT]))
        {
            hook_process_child_read (
                hook,
                HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDOUT]),
                HOOK_PROCESS_STDOUT,
                &(HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDOUT])));
        }
        if ((hook_process_test_cb_count == 0)
            && HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDERR]))
        {
            hook_process_child_read (
                hook,
                HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDERR]),
                HOOK_PROCESS_STDERR,
                &(HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDERR])));
        }
    }
    LONGS_EQUAL(1, hook_process_test_cb_count);
    LONGS_EQUAL(3, hook_process_test_cb_rc);
    STRCMP_EQUAL("test\n", hook_process_test_cb_out);
}

/*
 * Test functions:
 *   hook_process_child_read_stdout_cb
 */

TEST(HookProcess, ChildReadStdoutCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_process_child_read_stderr_cb
 */

TEST(HookProcess, ChildReadStderrCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_process_child_read_until_eof
 */

TEST(HookProcess, ChildReadUntilEof)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_process_check_end
 */

TEST(HookProcess, CheckEnd)
{
    struct t_hook *hook;

    /* Child not yet forked. */
    hook = hook_process (NULL, "sh -c 'exit 0'", 0, &test_process_cb,
                         NULL, NULL);
    CHECK(hook);
    LONGS_EQUAL(0, hook_process_check_end (hook));
    unhook (hook);
    LONGS_EQUAL(0, hook_process_test_cb_count);

    /* Child still running. */
    hook = test_run_process ("sh -c 'sleep 10'");
    LONGS_EQUAL(0, hook_process_check_end (hook));
    unhook (hook);
    LONGS_EQUAL(0, hook_process_test_cb_count);

    /* Child ended: remaining output is read and callback is called. */
    hook = test_run_process ("sh -c 'echo test; exit 3'");
    test_wait_child (hook);
    LONGS_EQUAL(1, hook_process_check_end (hook));
    LONGS_EQUAL(1, hook_process_test_cb_count);
    LONGS_EQUAL(3, hook_process_test_cb_rc);
    STRCMP_EQUAL("test\n", hook_process_test_cb_out);
}

/*
 * Test functions:
 *   hook_process_timer_cb
 */

TEST(HookProcess, TimerCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_process_run
 */

TEST(HookProcess, Run)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_process_exec
 */

TEST(HookProcess, Exec)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_process_free_data
 */

TEST(HookProcess, FreeData)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_process_add_to_infolist
 */

TEST(HookProcess, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   hook_process_print_log
 */

TEST(HookProcess, PrintLog)
{
    /* TODO: write tests */
}
