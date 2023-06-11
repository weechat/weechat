/*
 * fifo.c - fifo plugin for WeeChat: remote control with FIFO pipe
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>

#include "../weechat-plugin.h"
#include "fifo.h"
#include "fifo-command.h"
#include "fifo-config.h"
#include "fifo-info.h"


WEECHAT_PLUGIN_NAME(FIFO_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("FIFO pipe for remote control"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(FIFO_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_fifo_plugin = NULL;
#define weechat_plugin weechat_fifo_plugin

int fifo_quiet = 0;
int fifo_fd = -1;
struct t_hook *fifo_fd_hook = NULL;
char *fifo_filename = NULL;
char *fifo_unterminated = NULL;


int fifo_fd_cb (const void *pointer, void *data, int fd);


/*
 * Creates FIFO pipe for remote control.
 */

void
fifo_create ()
{
    struct stat st;
    struct t_hashtable *options;

    if (!weechat_config_boolean (fifo_config_file_enabled))
        return;

    if (!fifo_filename)
    {
        /* evaluate path */
        options = weechat_hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL, NULL);
        if (options)
            weechat_hashtable_set (options, "directory", "runtime");
        fifo_filename = weechat_string_eval_path_home (
            weechat_config_string (fifo_config_file_path),
            NULL, NULL, options);
        if (options)
            weechat_hashtable_free (options);
    }

    if (!fifo_filename)
    {
        weechat_printf (NULL,
                        _("%s%s: not enough memory (%s)"),
                        weechat_prefix ("error"), FIFO_PLUGIN_NAME,
                        "fifo_filename");
        return;
    }

    /* remove a pipe with same name (if exists) */
    if (stat (fifo_filename, &st) == 0)
    {
        /* if the file is a FIFO pipe, delete it */
        if (S_ISFIFO(st.st_mode))
            unlink (fifo_filename);
    }

    fifo_fd = -1;

    /* create FIFO pipe, writable for user only */
    if (mkfifo (fifo_filename, 0600) == 0)
    {
        /* open FIFO pipe in non-blocking mode */
        if ((fifo_fd = open (fifo_filename,
                             O_RDWR | O_NONBLOCK)) != -1)
        {
            if ((weechat_fifo_plugin->debug >= 1) || !fifo_quiet)
            {
                weechat_printf (NULL,
                                _("%s: pipe opened (file: %s)"),
                                FIFO_PLUGIN_NAME,
                                fifo_filename);
            }
            fifo_fd_hook = weechat_hook_fd (fifo_fd, 1, 0, 0,
                                            &fifo_fd_cb, NULL, NULL);
        }
        else
        {
            weechat_printf (NULL,
                            _("%s%s: unable to open pipe (%s) for "
                              "reading"),
                            weechat_prefix ("error"), FIFO_PLUGIN_NAME,
                            fifo_filename);
        }
    }
    else
    {
        weechat_printf (NULL,
                        _("%s%s: unable to create pipe for remote "
                          "control (%s): error %d %s"),
                        weechat_prefix ("error"), FIFO_PLUGIN_NAME,
                        fifo_filename, errno, strerror (errno));
    }
}

/*
 * Removes FIFO pipe.
 */

void
fifo_remove ()
{
    int fifo_found;

    fifo_found = (fifo_fd != -1);

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

    /* remove any unterminated message */
    if (fifo_unterminated)
    {
        free (fifo_unterminated);
        fifo_unterminated = NULL;
    }

    /* remove FIFO from disk */
    if (fifo_filename)
    {
        unlink (fifo_filename);
        free (fifo_filename);
        fifo_filename = NULL;
    }

    if (fifo_found && !fifo_quiet)
    {
        weechat_printf (NULL,
                        _("%s: pipe closed"),
                        FIFO_PLUGIN_NAME);
    }
}

/*
 * Executes a command/text received in FIFO pipe.
 */

void
fifo_exec (const char *text)
{
    char *text2, *pos_msg, *command_unescaped;
    int escaped;
    struct t_gui_buffer *ptr_buffer;

    text2 = strdup (text);
    if (!text2)
        return;

    pos_msg = NULL;
    command_unescaped = NULL;
    escaped = 0;
    ptr_buffer = NULL;

    /*
     * look for plugin + buffer name at beginning of text
     * text may be: "plugin.buffer *text" or "*text"
     */
    if (text2[0] == '*' || text2[0] == '\\')
    {
        escaped = text2[0] == '\\';
        pos_msg = text2 + 1;
        ptr_buffer = weechat_current_buffer ();
    }
    else
    {
        pos_msg = strstr (text2, " *");
        if (!pos_msg)
            pos_msg = strstr (text2, " \\");

        if (!pos_msg)
        {
            weechat_printf (NULL,
                            _("%s%s: invalid text received in pipe"),
                            weechat_prefix ("error"), FIFO_PLUGIN_NAME);
            free (text2);
            return;
        }

        escaped = pos_msg[1] == '\\';
        pos_msg[0] = '\0';
        pos_msg += 2;
        ptr_buffer = weechat_buffer_search ("==", text2);
        if (!ptr_buffer)
        {
            weechat_printf (NULL,
                            _("%s%s: buffer \"%s\" not found"),
                            weechat_prefix ("error"), FIFO_PLUGIN_NAME,
                            text2);
            free (text2);
            return;
        }
    }

    if (escaped)
    {
        command_unescaped = weechat_string_convert_escaped_chars (pos_msg);
        if (command_unescaped)
            pos_msg = command_unescaped;
    }

    weechat_command (ptr_buffer, pos_msg);

    free (text2);
    if (command_unescaped)
        free(command_unescaped);
}

/*
 * Reads data in FIFO pipe.
 */

int
fifo_fd_cb (const void *pointer, void *data, int fd)
{
    static char buffer[4096 + 2];
    char *buf2, *pos, *ptr_buf, *next_ptr_buf;
    int num_read, check_error;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) fd;

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
    else if (num_read < 0)
    {
        check_error = (errno == EAGAIN);
#ifdef __CYGWIN__
        check_error = check_error || (errno == ECOMM);
#endif /* __CYGWIN__ */
        if (check_error)
            return WEECHAT_RC_OK;

        weechat_printf (NULL,
                        _("%s%s: error reading pipe (%d %s), closing it"),
                        weechat_prefix ("error"), FIFO_PLUGIN_NAME,
                        errno, strerror (errno));
        fifo_remove ();
    }

    return WEECHAT_RC_OK;
}

/*
 * Initializes fifo plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    if (!fifo_config_init ())
        return WEECHAT_RC_ERROR;

    fifo_config_read ();

    fifo_quiet = 1;

    fifo_create ();

    fifo_command_init ();
    fifo_info_init ();

    fifo_quiet = 0;

    return WEECHAT_RC_OK;
}

/*
 * Ends fifo plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    fifo_remove ();

    fifo_config_write ();
    fifo_config_free ();

    return WEECHAT_RC_OK;
}
