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

#include "../common/weechat.h"
#include "gui.h"
#include "../common/log.h"
#include "../common/util.h"
#include "../common/weeconfig.h"


/*
 * gui_log_write_date: writes date to log file
 */

void
gui_log_write_date (t_gui_buffer *buffer)
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
 * gui_log_write_line: writes a line to log file
 */

void
gui_log_write_line (t_gui_buffer *buffer, char *message)
{
    char *msg_no_color;
    
    if (buffer->log_file)
    {
        msg_no_color = (char *)gui_color_decode ((unsigned char *)message, 0, 0);
        weechat_iconv_fprintf (buffer->log_file,
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
gui_log_write (t_gui_buffer *buffer, char *message)
{
    char *msg_no_color;
    
    if (buffer->log_file)
    {
        msg_no_color = (char *)gui_color_decode ((unsigned char *)message, 0, 0);
        weechat_iconv_fprintf (buffer->log_file,
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
gui_log_start (t_gui_buffer *buffer)
{
    int length;
    char *log_path, *log_path2;
    char *server_name, *channel_name;
    
    log_path = weechat_strreplace (cfg_log_path, "~", getenv ("HOME"));
    log_path2 = weechat_strreplace (log_path, "%h", weechat_home);
    
    if (SERVER(buffer))
        server_name = weechat_strreplace (SERVER(buffer)->name, DIR_SEPARATOR, "_");
    else
        server_name = NULL;
    if (CHANNEL(buffer))
        channel_name = weechat_strreplace (CHANNEL(buffer)->name, DIR_SEPARATOR, "_");
    else
        channel_name = NULL;
    
    if (!log_path || !log_path2 || (SERVER(buffer) && !server_name) ||
        (CHANNEL(buffer) && !channel_name))
    {
        weechat_log_printf (_("Not enough memory to write log file \"%s\"\n"),
                            (log_path2) ? log_path2 : ((log_path) ? log_path : cfg_log_path));
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf_nolog (NULL, _("Not enough memory to write log file \"%s\"\n"),
                          (log_path2) ? log_path2 : ((log_path) ? log_path : cfg_log_path));
        if (log_path)
            free (log_path);
        if (log_path2)
            free (log_path2);
        if (server_name)
            free (server_name);
        if (channel_name)
            free (channel_name);
        return;
    }
    
    length = strlen (log_path2) + 128;
    if (SERVER(buffer))
        length += strlen (server_name);
    if (CHANNEL(buffer))
        length += strlen (channel_name);
    
    buffer->log_filename = (char *) malloc (length);
    if (!buffer->log_filename)
    {
        weechat_log_printf (_("Not enough memory to write log file \"%s\"\n"),
                            (log_path2) ? log_path2 : ((log_path) ? log_path : cfg_log_path));
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf_nolog (NULL, _("Not enough memory to write log file \"%s\"\n"),
                          (log_path2) ? log_path2 : ((log_path) ? log_path : cfg_log_path));
        free (log_path);
        free (log_path2);
        if (server_name)
            free (server_name);
        if (channel_name)
            free (channel_name);
        return;
    }
    
    strcpy (buffer->log_filename, log_path2);
    
    free (log_path);
    free (log_path2);
    
    if (buffer->log_filename[strlen (buffer->log_filename) - 1] != DIR_SEPARATOR_CHAR)
        strcat (buffer->log_filename, DIR_SEPARATOR);
    
    if (SERVER(buffer))
    {
        strcat (buffer->log_filename, server_name);
        strcat (buffer->log_filename, ".");
    }
    if (CHANNEL(buffer)
        && (CHANNEL(buffer)->type == CHANNEL_TYPE_DCC_CHAT))
    {
        strcat (buffer->log_filename, "dcc.");
    }
    if (CHANNEL(buffer))
    {
        strcat (buffer->log_filename, channel_name);
        strcat (buffer->log_filename, ".");
    }
    strcat (buffer->log_filename, "weechatlog");
    
    if (server_name)
        free (server_name);
    if (channel_name)
        free (channel_name);
    
    buffer->log_file = fopen (buffer->log_filename, "a");
    if (!buffer->log_file)
    {
        weechat_log_printf (_("Unable to write log file \"%s\"\n"),
                            buffer->log_filename);
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL, _("Unable to write log file \"%s\"\n"),
                    buffer->log_filename);
        free (buffer->log_filename);
        return;
    }
    
    gui_log_write (buffer, _("****  Beginning of log  "));
    gui_log_write_date (buffer);
    gui_log_write (buffer, "****\n");
    
    return;
}

/*
 * gui_log_end: ends a log
 */

void
gui_log_end (t_gui_buffer *buffer)
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
