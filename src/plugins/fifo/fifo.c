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
 * fifo.c: fifo plugin for WeeChat: remote control with FIFO pipe
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <fcntl.h>

#include "../weechat-plugin.h"
#include "fifo.h"
#include "fifo-info.h"


WEECHAT_PLUGIN_NAME(FIFO_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("FIFO pipe for remote control"));
WEECHAT_PLUGIN_AUTHOR("Sebastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);

#ifndef PATH_MAX
    #define PATH_MAX 4096
#endif

#define FIFO_FILENAME_PREFIX "weechat_fifo_"

struct t_weechat_plugin *weechat_fifo_plugin = NULL;
#define weechat_plugin weechat_fifo_plugin

int fifo_quiet = 0;
int fifo_fd = -1;
struct t_hook *fifo_fd_hook = NULL;
char *fifo_filename;
char *fifo_unterminated = NULL;


int fifo_read();


/*
 * fifo_remove_old_pipes: remove old fifo pipes in directory
 */

void
fifo_remove_old_pipes ()
{
    char *buf;
    int buf_len, prefix_len;
    const char *weechat_home, *dir_separator;
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    buf_len = PATH_MAX;
    buf = malloc (buf_len);
    if (!buf)
        return;

    weechat_home = weechat_info_get ("weechat_dir", "");
    dir_separator = weechat_info_get ("dir_separator", "");

    prefix_len = strlen (FIFO_FILENAME_PREFIX);

    dp = opendir (weechat_home);
    if (dp != NULL)
    {
        while ((entry = readdir (dp)) != NULL)
        {
            if (strcmp (entry->d_name, ".") == 0 || strcmp (entry->d_name, "..") == 0)
                continue;

            if (strncmp (entry->d_name, FIFO_FILENAME_PREFIX, prefix_len) == 0)
            {
                snprintf (buf, buf_len, "%s%s%s",
                          weechat_home, dir_separator, entry->d_name);
                if (stat (buf, &statbuf) != -1)
                {
                    weechat_printf (NULL,
                                    _("%s: removing old fifo pipe \"%s\""),
                                    FIFO_PLUGIN_NAME, buf);
                    unlink (buf);
                }
            }
        }
        closedir (dp);
    }

    free (buf);
}

/*
 * fifo_create: create FIFO pipe for remote control
 */

void
fifo_create ()
{
    int filename_length;
    const char *fifo_option, *weechat_home;

    fifo_option = weechat_config_get_plugin ("fifo");
    if (!fifo_option)
    {
        weechat_config_set_plugin ("fifo", "on");
        fifo_option = weechat_config_get_plugin ("fifo");
    }

    weechat_home = weechat_info_get ("weechat_dir", "");

    if (fifo_option && weechat_home)
    {
        fifo_remove_old_pipes ();

        if (weechat_strcasecmp (fifo_option, "on") == 0)
        {
            /*
             * build FIFO filename:
             *   "<weechat_home>/weechat_fifo_" + process PID
             */
            if (!fifo_filename)
            {
                filename_length = strlen (weechat_home) + 64;
                fifo_filename = malloc (filename_length);
                snprintf (fifo_filename, filename_length,
                          "%s/%s%d",
                          weechat_home, FIFO_FILENAME_PREFIX, (int) getpid());
            }

            fifo_fd = -1;

            /* create FIFO pipe, writable for user only */
            if (mkfifo (fifo_filename, 0600) == 0)
            {
                /* open FIFO pipe in read-only, non blockingmode */
                if ((fifo_fd = open (fifo_filename,
                                     O_RDONLY | O_NONBLOCK)) != -1)
                {
                    if ((weechat_fifo_plugin->debug >= 1) || !fifo_quiet)
                    {
                        weechat_printf (NULL,
                                        _("%s: pipe opened"),
                                        FIFO_PLUGIN_NAME);
                    }
                    fifo_fd_hook = weechat_hook_fd (fifo_fd, 1, 0, 0,
                                                    &fifo_read, NULL);
                }
                else
                    weechat_printf (NULL,
                                    _("%s%s: unable to open pipe (%s) for "
                                      "reading"),
                                    weechat_prefix ("error"), FIFO_PLUGIN_NAME,
                                    fifo_filename);
            }
            else
                weechat_printf (NULL,
                                _("%s%s: unable to create pipe for remote "
                                  "control (%s)"),
                                weechat_prefix ("error"), FIFO_PLUGIN_NAME,
                                fifo_filename);
        }
    }
}

/*
 * fifo_remove: remove FIFO pipe
 */

void
fifo_remove ()
{
    /* remove fd hook */
    if (fifo_fd_hook)
    {
        weechat_unhook (fifo_fd_hook);
        fifo_fd_hook = NULL;
    }

    /* close FIFO pipe */
    if (fifo_fd != -1)
    {
        close (fifo_fd);
        fifo_fd = -1;
    }

    /* remove FIFO from disk */
    if (fifo_filename)
        unlink (fifo_filename);

    /* remove any unterminated message */
    if (fifo_unterminated)
    {
        free (fifo_unterminated);
        fifo_unterminated = NULL;
    }

    /* free filename */
    if (fifo_filename)
    {
        free (fifo_filename);
        fifo_filename = NULL;
    }

    weechat_printf (NULL,
                    _("%s: pipe closed"),
                    FIFO_PLUGIN_NAME);
}

/*
 * fifo_exec: execute a command/text received by FIFO pipe
 */

void
fifo_exec (const char *text)
{
    char *text2, *pos_msg, *pos_buffer;
    struct t_gui_buffer *ptr_buffer;

    text2 = strdup (text);
    if (!text2)
        return;

    pos_msg = NULL;
    ptr_buffer = NULL;

    /*
     * look for plugin + buffer name at beginning of text
     * text may be: "plugin.buffer *text" or "*text"
     */
    if (text2[0] == '*')
    {
        pos_msg = text2 + 1;
        ptr_buffer = weechat_current_buffer ();
    }
    else
    {
        pos_msg = strstr (text2, " *");
        if (!pos_msg)
        {
            weechat_printf (NULL,
                            _("%s%s: error, invalid text received on pipe"),
                            weechat_prefix ("error"), FIFO_PLUGIN_NAME);
            free (text2);
            return;
        }
        pos_msg[0] = '\0';
        pos_msg += 2;

        pos_buffer = strchr (text2, '.');
        if (!pos_buffer)
        {
            weechat_printf (NULL,
                            _("%s%s: error, invalid text received on pipe"),
                            weechat_prefix ("error"), FIFO_PLUGIN_NAME);
            free (text2);
            return;
        }
        pos_buffer[0] = '\0';
        pos_buffer++;

        if (text2[0] && pos_buffer[0])
            ptr_buffer = weechat_buffer_search (text2, pos_buffer);
    }

    if (!ptr_buffer)
    {
        weechat_printf (NULL,
                        _("%s%s: error, buffer not found for pipe data"),
                        weechat_prefix ("error"), FIFO_PLUGIN_NAME);
        free (text2);
        return;
    }

    weechat_command (ptr_buffer, pos_msg);

    free (text2);
}

/*
 * fifo_read: read data in FIFO pipe
 */

int
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
            buf2 = malloc (strlen (fifo_unterminated) +
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
                            _("%s%s: error reading pipe, closing it"),
                            weechat_prefix ("error"), FIFO_PLUGIN_NAME);
            fifo_remove ();
        }
        else
        {
            weechat_unhook (fifo_fd_hook);
            fifo_fd_hook = NULL;
            close (fifo_fd);
            fifo_fd = open (fifo_filename, O_RDONLY | O_NONBLOCK);
            if (fifo_fd < 0)
            {
                weechat_printf (NULL,
                                _("%s%s: error opening file, closing it"),
                                weechat_prefix ("error"), FIFO_PLUGIN_NAME);
                fifo_remove ();
            }
            else
                fifo_fd_hook = weechat_hook_fd (fifo_fd, 1, 0, 0,
                                                &fifo_read, NULL);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * fifo_config_cb: fifo config callback (called when fifo option is changed)
 */

int
fifo_config_cb (void *data, const char *option, const char *value)
{
    /* make C compiler happy */
    (void) data;
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

    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_init: initialize fifo plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    fifo_quiet = 1;

    fifo_create ();

    weechat_hook_config ("plugins.var.fifo.fifo", &fifo_config_cb, NULL);

    fifo_info_init ();

    fifo_quiet = 0;

    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end fifo plugin
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    fifo_remove ();

    return WEECHAT_RC_OK;
}
