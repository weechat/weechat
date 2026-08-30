/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Entry point for Curses GUI */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "../../../core/weechat.h"
#include "../../gui-main.h"
#include "../gui-curses.h"
#include "../gui-curses-main.h"


/*
 * Entry point for WeeChat (Curses GUI).
 */

int
main (int argc, char *argv[])
{
    /* Initialize, run main loop and terminate. */
    weechat_init_gettext ();
    weechat_init (argc, argv, &gui_main_init);
    gui_main_loop ();
    weechat_end (&gui_main_end);

    return EXIT_SUCCESS;
}
