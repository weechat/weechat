/*
 * SPDX-FileCopyrightText: 2023-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test system functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/resource.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "src/core/core-string.h"
#include "src/core/core-sys.h"
#include "src/core/core-util.h"
}

TEST_GROUP(CoreSys)
{
    /*
     * Test a function closing file descriptors (sys_close_fds or
     * sys_close_fds_loop).
     */

    void check_close_fds (void (*close_fds)(int fd_min))
    {
        pid_t pid, pid_waited;
        struct rlimit rlim;
        struct sigaction sa_default, sa_old;
        int fd, fd_null, fd1, fd2, status;

        /*
         * Use the default action for SIGCHLD while the child runs: if SIGCHLD
         * is ignored (or handled by a plugin), the child could be reaped
         * automatically and waitpid() would fail.
         */
        memset (&sa_default, 0, sizeof (sa_default));
        sa_default.sa_handler = SIG_DFL;
        sigemptyset (&sa_default.sa_mask);
        LONGS_EQUAL(0, sigaction (SIGCHLD, &sa_default, &sa_old));

        /*
         * Run the test in a child process: it can close any file descriptor
         * without closing those used by tests.
         */
        pid = fork ();
        if (pid < 0)
            sigaction (SIGCHLD, &sa_old, NULL);
        CHECK(pid >= 0);
        if (pid == 0)
        {
            /* Lower the limit of open files: faster if the fallback is used. */
            if ((getrlimit (RLIMIT_NOFILE, &rlim) == 0)
                && (rlim.rlim_cur > 1024))
            {
                rlim.rlim_cur = 1024;
                setrlimit (RLIMIT_NOFILE, &rlim);
            }
            /* Open stdin, stdout and stderr if closed in the test runner. */
            for (fd = 0; fd <= 2; fd++)
            {
                if ((fcntl (fd, F_GETFD) == -1)
                    && (open ("/dev/null", O_RDWR) != fd))
                {
                    _exit (1);
                }
            }
            fd_null = open ("/dev/null", O_RDWR);
            if (fd_null < 3)
                _exit (1);
            /* Two other file descriptors: fd_null < fd1 < fd2. */
            fd1 = fcntl (fd_null, F_DUPFD, fd_null + 1);
            fd2 = (fd1 >= 0) ? fcntl (fd_null, F_DUPFD, fd1 + 1) : -1;
            if ((fd1 < 0) || (fd2 < 0))
                _exit (2);
            /* Nothing closed: fd1 and fd2 are lower. */
            close_fds (fd2 + 1);
            if ((fcntl (fd1, F_GETFD) == -1) || (fcntl (fd2, F_GETFD) == -1))
                _exit (3);
            /* Close file descriptors >= fd1 + 1: fd2 is closed, not fd1. */
            close_fds (fd1 + 1);
            if ((fcntl (fd1, F_GETFD) == -1)
                || (fcntl (fd2, F_GETFD) != -1) || (errno != EBADF))
            {
                _exit (4);
            }
            /* Close file descriptors >= 3 (like the daemon). */
            close_fds (3);
            if ((fcntl (fd_null, F_GETFD) != -1) || (errno != EBADF)
                || (fcntl (fd1, F_GETFD) != -1) || (errno != EBADF))
            {
                _exit (5);
            }
            /* stdin, stdout and stderr are not closed. */
            if (fcntl (2, F_GETFD) == -1)
                _exit (6);
            _exit (0);
        }
        pid_waited = waitpid (pid, &status, 0);
        sigaction (SIGCHLD, &sa_old, NULL);
        LONGS_EQUAL(pid, pid_waited);
        CHECK(WIFEXITED(status));
        LONGS_EQUAL(0, WEXITSTATUS(status));
    }
};

/*
 * Test functions:
 *   sys_setrlimit_resource
 */

TEST(CoreSys, SetrlimitResource)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   sys_setrlimit
 */

TEST(CoreSys, Setrlimit)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   sys_display_rlimit
 */

TEST(CoreSys, DisplayRlimit)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   sys_display_rusage
 */

TEST(CoreSys, DisplayRusage)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   sys_close_fds_loop
 *   sys_close_fds
 */

TEST(CoreSys, CloseFds)
{
    check_close_fds (&sys_close_fds);
    check_close_fds (&sys_close_fds_loop);
}

/*
 * Test functions:
 *   sys_daemonize
 */

TEST(CoreSys, Daemonize)
{
    /* TODO: write tests */
}
