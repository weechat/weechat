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

/* xfer-buffer.c: display xfer list on xfer buffer */


#include <stdlib.h>
#include <stdio.h>

#include "../weechat-plugin.h"
#include "xfer.h"
#include "xfer-buffer.h"
#include "xfer-config.h"


struct t_gui_buffer *xfer_buffer = NULL;
int xfer_buffer_selected_line = 0;


/*
 * xfer_buffer_close_cb: callback called when xfer buffer is closed
 */

int
xfer_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    
    xfer_buffer = NULL;
    
    return WEECHAT_RC_OK;
}

/*
 * xfer_buffer_open: open xfer buffer (to display list of xfer)
 */

void
xfer_buffer_open ()
{
    if (!xfer_buffer)
    {
        xfer_buffer = weechat_buffer_new ("xfer", "xfer",
                                          NULL, NULL,
                                          &xfer_buffer_close_cb, NULL);
        
        /* failed to create buffer ? then exit */
        if (!xfer_buffer)
            return;

        weechat_buffer_set (xfer_buffer, "type", "free");
        weechat_buffer_set (xfer_buffer, "title", _("Xfer list"));
    }
}

/*
 * xfer_buffer_refresh: update a xfer in buffer and update hotlist for xfer buffer
 */

void
xfer_buffer_refresh (char *hotlist)
{
    struct t_xfer *ptr_xfer;
    char str_color[256];
    int line;
    
    if (xfer_buffer)
    {
        line = 0;
        for (ptr_xfer = xfer_list; ptr_xfer; ptr_xfer = ptr_xfer->next_xfer)
        {
            if (XFER_IS_FILE(ptr_xfer->type))
            {
                snprintf (str_color, sizeof (str_color),
                          "%s,%s",
                          weechat_config_string (xfer_config_color_text),
                          weechat_config_string (xfer_config_color_text_bg));
                weechat_printf_y (xfer_buffer, line * 2,
                                  "%s%s%-20s \"%s\"",
                                  weechat_color(str_color),
                                  (line == xfer_buffer_selected_line) ?
                                  "*** " : "    ",
                                  ptr_xfer->nick, ptr_xfer->filename);
                weechat_printf_y (xfer_buffer, (line * 2) + 1,
                                  "%s%s%s %s%-15s ",
                                  weechat_color(str_color),
                                  (line == xfer_buffer_selected_line) ?
                                  "*** " : "    ",
                                  (XFER_IS_SEND(ptr_xfer->type)) ?
                                  "<<--" : "-->>",
                                  weechat_color(
                                      weechat_config_string (
                                          xfer_config_color_status[ptr_xfer->status])),
                                  _(xfer_status_string[ptr_xfer->status]));
            }
            line++;
        }
        weechat_buffer_set (xfer_buffer, "hotlist", hotlist);
    }
}
