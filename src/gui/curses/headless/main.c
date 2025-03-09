/*
 * main.c - entry point for headless mode (no GUI)
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
 * Daemonizes the process.
 */

void
daemonize (void)
{
    pid_t pid;
    int fd, i;

    printf (_("Running WeeChat in background..."));
    printf (" ");

    pid = fork ();

    if (pid < 0)
    {
        printf (_("fork error"));
        printf ("\n");
        exit (EXIT_FAILURE);
    }

    if (pid > 0)
    {
        /* parent process */
        printf (_("OK"));
        printf ("\n");
        exit (EXIT_SUCCESS);
    }

    /* child process */

    /* obtain a new process group */
    setsid ();

    /* close all file descriptors */
    for (i = sysconf (_SC_OPEN_MAX); i >= 0; --i)
    {
        close (i);
    }
    fd = open ("/dev/null", O_RDWR);
    (void) dup (fd);
    (void) dup (fd);
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
     * - don't catch any terminal related signal
     */
    weechat_headless = 1;

    /*
     * Parse extra options for headless mode:
     * - "--daemon": daemonize the process
     * - "--stdout": log messages to stdout (instead of log file)
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

    /* init, main loop and end */
    weechat_init (argc, argv, &gui_main_init);
    gui_main_loop ();
    weechat_end (&gui_main_end);

    return EXIT_SUCCESS;
}
