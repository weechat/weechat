/*
 * Copyright (c) 2003-2005 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* log.c: log buffers to files */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "weechat.h"
#include "log.h"
#include "weeconfig.h"
#include "../gui/gui.h"


/*
 * log_write_date: writes date to log file
 */

void
log_write_date (t_gui_buffer *buffer)
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
            strftime (buf_time, sizeof (buf_time) - 1, cfg_log_timestamp, date_tmp);
            fprintf (buffer->log_file, "%s  ", buf_time);
            fflush (buffer->log_file);
        }
    }
}

/*
 * log_write: writes a message to log file
 */

void
log_write (t_gui_buffer *buffer, char *message)
{
    if (buffer->log_file)
    {    
        fprintf (buffer->log_file, "%s", message);
        fflush (buffer->log_file);
    }
}

/*
 * log_start: starts a log
 */

void
log_start (t_gui_buffer *buffer)
{
    int length;
    char *ptr_home;
    
    ptr_home = getenv ("HOME");
    length = strlen (cfg_log_path) +
        ((cfg_log_path[0] == '~') ? strlen (ptr_home) : 0) +
        64;
    if (SERVER(buffer))
        length += strlen (SERVER(buffer)->name);
    if (CHANNEL(buffer))
        length += strlen (CHANNEL(buffer)->name);
    buffer->log_filename = (char *) malloc (length);
    if (!buffer->log_filename)
    {
        wee_log_printf (_("Not enough memory to write log file for a buffer\n"));
        return;
    }
    if (cfg_log_path[0] == '~')
    {
        strcpy (buffer->log_filename, ptr_home);
        strcat (buffer->log_filename, cfg_log_path + 1);
    }
    else
        strcpy (buffer->log_filename, cfg_log_path);
    if (buffer->log_filename[strlen (buffer->log_filename) - 1] != DIR_SEPARATOR_CHAR)
        strcat (buffer->log_filename, DIR_SEPARATOR);
    
    if (SERVER(buffer))
    {
        strcat (buffer->log_filename, SERVER(buffer)->name);
        strcat (buffer->log_filename, ".");
    }
    if (CHANNEL(buffer))
    {
        strcat (buffer->log_filename, CHANNEL(buffer)->name);
        strcat (buffer->log_filename, ".");
    }
    strcat (buffer->log_filename, "weechatlog");
    
    buffer->log_file = fopen (buffer->log_filename, "a");
    if (!buffer->log_file)
    {
        wee_log_printf (_("Unable to write log file for a buffer\n"));
        free (buffer->log_filename);
        return;
    }
    log_write (buffer, _("****  Beginning of log  "));
    log_write_date (buffer);
    log_write (buffer, "****\n");
}

/*
 * log_end: ends a log
 */

void
log_end (t_gui_buffer *buffer)
{
    if (buffer->log_file)
    {
        log_write (buffer, _("****  End of log  "));
        log_write_date (buffer);
        log_write (buffer, "****\n");
        fclose (buffer->log_file);
        buffer->log_file = NULL;
    }
    if (buffer->log_filename)
        free (buffer->log_filename);
}
