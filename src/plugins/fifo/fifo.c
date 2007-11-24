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

/* fifo.c: FIFO pipe plugin for WeeChat remote control */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../weechat-plugin.h"
#include "fifo.h"


static struct t_weechat_plugin *weechat_plugin = NULL;
static int fifo_fd = -1;
static struct t_hook *fifo_fd_hook = NULL;
static char *fifo_filename;
static char *fifo_unterminated = NULL;


/*
 * fifo_create: create FIFO pipe for remote control
 */

static void
fifo_create ()
{
    int filename_length;
    char *fifo_option, *weechat_home;

    fifo_option = weechat_plugin_config_get ("fifo");
    if (!fifo_option)
    {
        weechat_plugin_config_set ("fifo", "on");
        fifo_option = weechat_plugin_config_get ("fifo");
    }
    
    weechat_home = weechat_info_get ("weechat_dir");
    
    if (fifo_option && weechat_home)
    {
        if (weechat_strcasecmp (fifo_option, "on") == 0)
        {
            /* build FIFO filename: "<weechat_home>/weechat_fifo_" + process
               PID */
            if (!fifo_filename)
            {
                filename_length = strlen (weechat_home) + 64;
                fifo_filename = (char *) malloc (filename_length *
                                                 sizeof (char));
                snprintf (fifo_filename, filename_length,
                          "%s/weechat_fifo_%d",
                          weechat_home, (int) getpid());
            }
            
            fifo_fd = -1;
            
            /* create FIFO pipe, writable for user only */
            if (mkfifo (fifo_filename, 0600) == 0)
            {
                /* open FIFO pipe in read-only, non blockingmode */
                if ((fifo_fd = open (fifo_filename,
                                     O_RDONLY | O_NONBLOCK)) != -1)
                    weechat_printf (NULL,
                                    _("%sFifo: pipe is open\n"),
                                    weechat_prefix ("info"));
                else
                    weechat_printf (NULL,
                                    _("%sFifo: unable to open pipe (%s) for "
                                      "reading"),
                                    weechat_prefix ("error"),
                                    fifo_filename);
            }
            else
                weechat_printf (NULL,
                                _("%sFifo: unable to create pipe for remote "
                                  "control (%s)"),
                                weechat_prefix ("error"),
                                fifo_filename);
        }
    }
    if (fifo_option)
        free (fifo_option);
    if (weechat_home)
        free (weechat_home);
}

/*
 * fifo_remove: remove FIFO pipe
 */

static void
fifo_remove ()
{
    if (fifo_fd != -1)
    {
        /* close FIFO pipe */
        close (fifo_fd);
        fifo_fd = -1;
    }
    
    /* remove FIFO from disk */
    if (fifo_filename)
        unlink (fifo_filename);
    
    if (fifo_unterminated)
    {
        free (fifo_unterminated);
        fifo_unterminated = NULL;
    }
    
    if (fifo_filename)
    {
        free (fifo_filename);
        fifo_filename = NULL;
    }
    
    weechat_printf (NULL,
                    _("%sFifo: pipe is closed"),
                    weechat_prefix ("info"));
}

/*
 * fifo_exec: execute a command/text received by FIFO pipe
 */

static void
fifo_exec (char *text)
{
    char *pos_msg, *pos;
    struct t_gui_buffer *ptr_buffer;
    
    pos = NULL;
    ptr_buffer = NULL;
    
    /* look for category/name at beginning of text
       text may be: "category,name *text" or "name *text" or "*text" */
    if (text[0] == '*')
    {
        pos_msg = text + 1;
        ptr_buffer = weechat_buffer_search (NULL, NULL);
    }
    else
    {
        pos_msg = strstr (text, " *");
        if (!pos_msg)
        {
            weechat_printf (NULL,
                            _("%sFifo error: invalid text received on pipe"),
                            weechat_prefix ("error"));
            return;
        }
        pos_msg[0] = '\0';
        pos = pos_msg - 1;
        pos_msg += 2;
        while ((pos >= text) && (pos[0] == ' '))
        {
            pos[0] = '\0';
            pos--;
        }
        
        if (text[0])
        {
            pos = strchr (text, ',');
            if (pos)
            {
                pos[0] = '\0';
                ptr_buffer = weechat_buffer_search (text, pos + 1);
            }
            else
                ptr_buffer = weechat_buffer_search (NULL, text);
        }
    }
    
    if (!ptr_buffer)
    {
        weechat_printf (NULL,
                        _("%sFifo error: buffer not found for pipe data"),
                        weechat_prefix ("error"));
        return;
    }
    
    weechat_command (ptr_buffer, pos_msg);
}

/*
 * fifo_read: read data in FIFO pipe
 */

static int
fifo_read ()
{
    static char buffer[4096 + 2];
    char *buf2, *pos, *ptr_buf, *next_ptr_buf;
    int num_read;
    
    num_read = read (fifo_fd, buffer, sizeof (buffer) - 2);
    if (num_read > 0)
    {
        buffer[num_read] = '\0';
        
        buf2 = NULL;
        ptr_buf = buffer;
        if (fifo_unterminated)
        {
            buf2 = (char *) malloc (strlen (fifo_unterminated) +
                strlen (buffer) + 1);
            if (buf2)
            {
                strcpy (buf2, fifo_unterminated);
                strcat (buf2, buffer);
            }
            ptr_buf = buf2;
            free (fifo_unterminated);
            fifo_unterminated = NULL;
        }
        
        while (ptr_buf && ptr_buf[0])
        {
            next_ptr_buf = NULL;
            pos = strstr (ptr_buf, "\r\n");
            if (pos)
            {
                pos[0] = '\0';
                next_ptr_buf = pos + 2;
            }
            else
            {
                pos = strstr (ptr_buf, "\n");
                if (pos)
                {
                    pos[0] = '\0';
                    next_ptr_buf = pos + 1;
                }
                else
                {
                    fifo_unterminated = strdup (ptr_buf);
                    ptr_buf = NULL;
                    next_ptr_buf = NULL;
                }
            }
            
            if (ptr_buf)
                fifo_exec (ptr_buf);
            
            ptr_buf = next_ptr_buf;
        }
        
        if (buf2)
            free (buf2);
    }
    else
    {
        if (num_read < 0)
        {
            weechat_printf (NULL,
                            _("%sFifo: error reading pipe, closing it"),
                            weechat_prefix ("error"));
            fifo_remove ();
        }
        else
        {
            weechat_unhook (fifo_fd_hook);
            close (fifo_fd);
            fifo_fd = open (fifo_filename, O_RDONLY | O_NONBLOCK);
            fifo_fd_hook = weechat_hook_fd (fifo_fd, 1, 0, 0, fifo_read, NULL);
        }
    }

    return PLUGIN_RC_SUCCESS;
}

/*
 * fifo_config_cb: fifo config callback (called when fifo option is changed)
 */

static int
fifo_config_cb (void *data, char *type, char *option, char *value)
{
    /* make C compiler happy */
    (void) data;
    (void) type;
    (void) option;
    
    if (weechat_strcasecmp (value, "on") == 0)
    {
        if (fifo_fd < 0)
            fifo_create ();
    }
    else
    {
        if (fifo_fd >= 0)
            fifo_remove ();
    }
    
    return PLUGIN_RC_SUCCESS;
}

/*
 * weechat_plugin_init: initialize fifo plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin)
{
    weechat_plugin = plugin;
    
    fifo_create ();
    
    fifo_fd_hook = weechat_hook_fd (fifo_fd, 1, 0, 0, fifo_read, NULL);
    
    weechat_hook_config ("plugin", "fifo.fifo", fifo_config_cb, NULL);
    
    return PLUGIN_RC_SUCCESS;
}

/*
 * weechat_plugin_end: end fifo plugin
 */

int
weechat_plugin_end ()
{
    fifo_remove ();
    
    return PLUGIN_RC_SUCCESS;
}
