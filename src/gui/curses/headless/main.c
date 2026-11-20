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

#include "../../../core/weechat.h"
#include "../../gui-main.h"
#include "../gui-curses.h"
#include "../gui-curses-main.h"


/*
 * Entry point for WeeChat in headless mode (no GUI).
 */

int
main (int argc, char *argv[])
{
    weechat_init_gettext ();

    /*
     * Enable a special "headless" mode, where some things are slightly
     * different, for example:
     * - no read of stdin (keyboard/mouse)
     * - don't catch any terminal related signal.
     */
    weechat_headless = 1;

    /* Initialize, run main loop and terminate. */
    weechat_init (argc, argv, &gui_main_init);
    gui_main_loop ();
    weechat_end (&gui_main_end);

    return EXIT_SUCCESS;
}
