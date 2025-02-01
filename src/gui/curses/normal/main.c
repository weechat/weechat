/*
 * main.c - entry point for Curses GUI
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
    /* init, main loop and end */
    weechat_init_gettext ();
    weechat_init (argc, argv, &gui_main_init);
    gui_main_loop ();
    weechat_end (&gui_main_end);

    return EXIT_SUCCESS;
}
