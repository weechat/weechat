/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

/* gui-log.c: log buffers to files */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "gui-log.h"


/*
 * gui_log_write_date: writes date to log file
 */

void
gui_log_write_date (struct t_gui_buffer *buffer)
{
    static char buf_time[256];
    static time_t seconds;
    struct tm *date_tmp;
    
    if (buffer->log_file)
    {    
        seconds = time (NULL);
        date_tmp = localtime (&seconds);
        if (date_tmp)
        {
            strftime (buf_time, sizeof (buf_time) - 1,
                      cfg_log_time_format, date_tmp);
            fprintf (buffer->log_file, "%s  ", buf_time);
            fflush (buffer->log_file);
        }
    }
}

/*
 * gui_log_write_line: writes a line to log file
 */

void
gui_log_write_line (struct t_gui_buffer *buffer, char *message)
{
    char *msg_no_color;
    
    if (buffer->log_file)
    {
        msg_no_color = (char *)gui_color_decode ((unsigned char *)message);
        string_iconv_fprintf (buffer->log_file,
                              "%s\n", (msg_no_color) ? msg_no_color : message);
        fflush (buffer->log_file);
        if (msg_no_color)
            free (msg_no_color);
    }
}

/*
 * gui_log_write: writes a message to log file
 */

void
gui_log_write (struct t_gui_buffer *buffer, char *message)
{
    char *msg_no_color;
    
    if (buffer->log_file)
    {
        msg_no_color = (char *)gui_color_decode ((unsigned char *)message);
        string_iconv_fprintf (buffer->log_file,
                              "%s", (msg_no_color) ? msg_no_color : message);
        fflush (buffer->log_file);
        if (msg_no_color)
            free (msg_no_color);
    }
}

/*
 * gui_log_start: starts a log
 */

void
gui_log_start (struct t_gui_buffer *buffer)
{
    if (buffer->log_filename)
    {
        buffer->log_file = fopen (buffer->log_filename, "a");
        if (!buffer->log_file)
        {
            weechat_log_printf (_("Unable to write log file \"%s\"\n"),
                                buffer->log_filename);
            gui_chat_printf (NULL,
                             _("%sError: Unable to write log file \"%s\"\n"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             buffer->log_filename);
            free (buffer->log_filename);
            return;
        }
        
        gui_log_write (buffer, _("****  Beginning of log  "));
        gui_log_write_date (buffer);
        gui_log_write (buffer, "****\n");
    }
}

/*
 * gui_log_end: ends a log
 */

void
gui_log_end (struct t_gui_buffer *buffer)
{
    if (buffer->log_file)
    {
        gui_log_write (buffer, _("****  End of log  "));
        gui_log_write_date (buffer);
        gui_log_write (buffer, "****\n");
        fclose (buffer->log_file);
        buffer->log_file = NULL;
    }
    if (buffer->log_filename)
    {
        free (buffer->log_filename);
        buffer->log_filename = NULL;
    }
}
