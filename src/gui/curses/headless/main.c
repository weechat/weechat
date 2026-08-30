/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Entry point for headless mode (no GUI) */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "../../../core/weechat.h"
#include "../../gui-main.h"
#include "../gui-curses.h"
#include "../gui-curses-main.h"


/*
 * Daemonize the process.
 */

void
daemonize (void)
{
    pid_t pid;
    int fd, i, rc;

    printf ("%s ", _("Running WeeChat in background..."));

    pid = fork ();

    if (pid < 0)
    {
        printf ("%s\n", _("fork error"));
        exit (EXIT_FAILURE);
    }

    if (pid > 0)
    {
        /* Parent process */
        printf ("%s\n", _("OK"));
        exit (EXIT_SUCCESS);
    }

    /* Child process */

    /* Obtain a new process group. */
    setsid ();

    /* Close all file descriptors. */
    for (i = sysconf (_SC_OPEN_MAX); i >= 0; --i)
    {
        close (i);
    }
    fd = open ("/dev/null", O_RDWR);
    rc = dup (fd);
    rc = dup (fd);
    (void) rc;
}

/*
 * Entry point for WeeChat in headless mode (no GUI).
 */

int
main (int argc, char *argv[])
{
    int i;

    weechat_init_gettext ();

    /*
     * Enable a special "headless" mode, where some things are slightly
     * different, for example:
     * - no read of stdin (keyboard/mouse)
     * - don't catch any terminal related signal.
     */
    weechat_headless = 1;

    /*
     * Parse extra options for headless mode:
     * - "--daemon": daemonize the process
     * - "--stdout": log messages to stdout (instead of log file).
     */
    weechat_daemon = 0;
    for (i = 1; i < argc; i++)
    {
        if (strcmp (argv[i], "--daemon") == 0)
        {
            weechat_daemon = 1;
        }
        else if (strcmp (argv[i], "--stdout") == 0)
        {
            weechat_log_stdout = 1;
        }
    }
    if (weechat_daemon)
    {
        weechat_log_stdout = 0;
        daemonize ();
    }

    /* Initialize, run main loop and terminate. */
    weechat_init (argc, argv, &gui_main_init);
    gui_main_loop ();
    weechat_end (&gui_main_end);

    return EXIT_SUCCESS;
}
