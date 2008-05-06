/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* xfer-command.c: xfer command */


#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "xfer.h"
#include "xfer-buffer.h"


/*
 * xfer_command_xfer: command /xfer
 */

int
xfer_command_xfer (void *data, struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argc;
    (void) argv;
    (void) argv_eol;
    
    if (!xfer_buffer)
        xfer_buffer_open ();

    if (argc > 1)
    {
        if (strcmp (argv[1], "up") == 0)
        {
            if (xfer_buffer_selected_line > 0)
            {
                xfer_buffer_selected_line--;
                xfer_buffer_refresh (NULL);
            }
        }
        else if (strcmp (argv[1], "down") == 0)
        {
            if (xfer_buffer_selected_line < xfer_count - 1)
            {
                xfer_buffer_selected_line++;
                xfer_buffer_refresh (NULL);
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * xfer_command: xfer command
 */

void
xfer_command_init ()
{
    weechat_hook_command ("xfer",
                          N_("xfer control"),
                          "",
                          _("Open buffer with xfer list"),
                          NULL, &xfer_command_xfer, NULL);
}
