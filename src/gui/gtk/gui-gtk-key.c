/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * gui-gtk-keyboard.c: keyboard functions for Gtk GUI
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../../core/weechat.h"
#include "../../core/wee-utf8.h"
#include "../../plugins/plugin.h"
#include "../gui-key.h"
#include "../gui-buffer.h"
#include "gui-gtk.h"


/*
 * Creates default key bindings.
 */

void
gui_key_default_bindings (int context)
{
    /* TODO: write this function for Gtk */
    (void) context;
}

/*
 * Reads keyboard chars.
 */

void
gui_key_read ()
{
    /* TODO: write this function for Gtk */
}
