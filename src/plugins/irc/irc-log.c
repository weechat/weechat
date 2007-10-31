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

/* irc-log.c: log IRC buffers to files */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../../core/weechat.h"
#include "irc.h"
#include "../../core/log.h"
#include "../../core/util.h"
#include "../../core/weechat-config.h"


/*
 * irc_log_get_filename: get filename for an IRC buffer log file
 *                       Note: result has to be free() after use
 */

char *
irc_log_fet_filename (char *server_name, char *channel_name, int dcc_chat)
{
    int length;
    char *log_path, *log_path2;
    char *server_name2, *channel_name2;
    char *log_filename;
    
    if (!server_name)
        return NULL;
    
    log_path = weechat_strreplace (cfg_log_path, "~", getenv ("HOME"));
    log_path2 = weechat_strreplace (log_path, "%h", weechat_home);
    
    if (channel_name)
    {
        server_name2 = NULL;
        channel_name2 = weechat_strreplace (channel_name, DIR_SEPARATOR, "_");
    }
    else
    {
        server_name2 = weechat_strreplace (server_name, DIR_SEPARATOR, "_");
        channel_name2 = NULL;
    }
    
    if (!log_path || !log_path2 || (!channel_name && !server_name2)
        || (channel_name && !channel_name2))
    {
        weechat_log_printf (_("Not enough memory to write log file \"%s\"\n"),
                            (log_path2) ? log_path2 : ((log_path) ? log_path : cfg_log_path));
        if (log_path)
            free (log_path);
        if (log_path2)
            free (log_path2);
        if (server_name2)
            free (server_name2);
        if (channel_name2)
            free (channel_name2);
        return NULL;
    }
    
    length = strlen (log_path2) + 128;
    if (channel_name2)
        length += strlen (channel_name2);
    else
        length += strlen (server_name2);
    
    log_filename = (char *) malloc (length);
    if (!log_filename)
    {
        weechat_log_printf (_("Not enough memory to write log file \"%s\"\n"),
                            (log_path2) ? log_path2 : ((log_path) ? log_path : cfg_log_path));
        free (log_path);
        free (log_path2);
        if (server_name2)
            free (server_name2);
        if (channel_name2)
            free (channel_name2);
        return NULL;
    }
    
    strcpy (log_filename, log_path2);
    
    free (log_path);
    free (log_path2);
    
    if (log_filename[strlen (log_filename) - 1] != DIR_SEPARATOR_CHAR)
        strcat (log_filename, DIR_SEPARATOR);
    
    /* server buffer */
    if (!channel_name2)
    {
        strcat (log_filename, server_name2);
        strcat (log_filename, ".");
    }
    
    /* DCC chat buffer */
    if (channel_name2 && dcc_chat)
    {
        strcat (log_filename, "dcc.");
    }
    
    /* channel buffer */
    if (channel_name2)
    {
        strcat (log_filename, channel_name2);
        strcat (log_filename, ".");
    }
    
    /* append WeeChat suffix */
    strcat (log_filename, "weechatlog");
    
    if (server_name2)
        free (server_name2);
    if (channel_name2)
        free (channel_name2);
    
    return log_filename;
}
