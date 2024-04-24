/*
 * logger-buffer.c - logger buffer list management
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "../weechat-plugin.h"
#include "logger.h"
#include "logger-buffer.h"
#include "logger-config.h"


char *logger_buffer_compression_extension[LOGGER_BUFFER_NUM_COMPRESSION_TYPES] =
{ "", ".gz", ".zst" };

struct t_logger_buffer *logger_buffers = NULL;
struct t_logger_buffer *last_logger_buffer = NULL;


/*
 * Checks if a logger buffer pointer is valid.
 *
 * Returns:
 *   1: logger buffer exists
 *   0: logger buffer does not exist
 */

int
logger_buffer_valid (struct t_logger_buffer *logger_buffer)
{
    struct t_logger_buffer *ptr_logger_buffer;

    if (!logger_buffer)
        return 0;

    for (ptr_logger_buffer = logger_buffers; ptr_logger_buffer;
         ptr_logger_buffer = ptr_logger_buffer->next_buffer)
    {
        if (ptr_logger_buffer == logger_buffer)
            return 1;
    }

    /* logger_buffer not found */
    return 0;
}

/*
 * Adds a new buffer for logging.
 *
 * Returns pointer to new logger buffer, NULL if error.
 */

struct t_logger_buffer *
logger_buffer_add (struct t_gui_buffer *buffer, int log_level)
{
    struct t_logger_buffer *new_logger_buffer;

    if (!buffer)
        return NULL;

    if (weechat_logger_plugin->debug)
    {
        weechat_printf_date_tags (NULL, 0, "no_log",
                                  "%s: start logging for buffer \"%s\"",
                                  LOGGER_PLUGIN_NAME,
                                  weechat_buffer_get_string (buffer, "name"));
    }

    new_logger_buffer = malloc (sizeof (*new_logger_buffer));
    if (new_logger_buffer)
    {
        new_logger_buffer->buffer = buffer;
        new_logger_buffer->log_filename = NULL;
        new_logger_buffer->log_file = NULL;
        new_logger_buffer->log_file_inode = 0;
        new_logger_buffer->log_enabled = 1;
        new_logger_buffer->log_level = log_level;
        new_logger_buffer->write_start_info_line = 1;
        new_logger_buffer->flush_needed = 0;
        new_logger_buffer->compressing = 0;

        new_logger_buffer->prev_buffer = last_logger_buffer;
        new_logger_buffer->next_buffer = NULL;
        if (last_logger_buffer)
            last_logger_buffer->next_buffer = new_logger_buffer;
        else
            logger_buffers = new_logger_buffer;
        last_logger_buffer = new_logger_buffer;
    }

    return new_logger_buffer;
}

/*
 * Searches for logger buffer by buffer pointer.
 *
 * Returns pointer to logger buffer found, NULL if not found.
 */

struct t_logger_buffer *
logger_buffer_search_buffer (struct t_gui_buffer *buffer)
{
    struct t_logger_buffer *ptr_logger_buffer;

    for (ptr_logger_buffer = logger_buffers; ptr_logger_buffer;
         ptr_logger_buffer = ptr_logger_buffer->next_buffer)
    {
        if (ptr_logger_buffer->buffer == buffer)
            return ptr_logger_buffer;
    }

    /* logger buffer not found */
    return NULL;
}

/*
 * Searches for a logger buffer by log filename.
 *
 * Returns pointer to logger buffer found, NULL if not found.
 */

struct t_logger_buffer *
logger_buffer_search_log_filename (const char *log_filename)
{
    struct t_logger_buffer *ptr_logger_buffer;

    if (!log_filename)
        return NULL;

    for (ptr_logger_buffer = logger_buffers; ptr_logger_buffer;
         ptr_logger_buffer = ptr_logger_buffer->next_buffer)
    {
        if (ptr_logger_buffer->log_filename)
        {
            if (strcmp (ptr_logger_buffer->log_filename, log_filename) == 0)
                return ptr_logger_buffer;
        }
    }

    /* logger buffer not found */
    return NULL;
}

/*
 * Sets log filename for a logger buffer.
 */

void
logger_buffer_set_log_filename (struct t_logger_buffer *logger_buffer)
{
    char *log_filename, *pos_last_sep;
    char *dir_separator;
    struct t_logger_buffer *ptr_logger_buffer;

    /* get log filename for buffer */
    log_filename = logger_get_filename (logger_buffer->buffer);
    if (!log_filename)
    {
        weechat_printf_date_tags (NULL, 0, "no_log",
                                  _("%s%s: not enough memory"),
                                  weechat_prefix ("error"),
                                  LOGGER_PLUGIN_NAME);
        return;
    }

    /* log file is already used by another buffer? */
    ptr_logger_buffer = logger_buffer_search_log_filename (log_filename);
    if (ptr_logger_buffer)
    {
        weechat_printf_date_tags (
            NULL, 0, "no_log",
            _("%s%s: unable to start logging for buffer "
              "\"%s\": filename \"%s\" is already used by "
              "another buffer (check your log settings)"),
            weechat_prefix ("error"),
            LOGGER_PLUGIN_NAME,
            weechat_buffer_get_string (logger_buffer->buffer, "name"),
            log_filename);
        free (log_filename);
        return;
    }

    /* create directory for path in "log_filename" */
    dir_separator = weechat_info_get ("dir_separator", "");
    if (dir_separator)
    {
        pos_last_sep = strrchr (log_filename, dir_separator[0]);
        if (pos_last_sep)
        {
            pos_last_sep[0] = '\0';
            weechat_mkdir_parents (log_filename, 0700);
            pos_last_sep[0] = dir_separator[0];
        }
        free (dir_separator);
    }

    /* set log filename */
    logger_buffer->log_filename = log_filename;
}

/*
 * Creates a log file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
logger_buffer_create_log_file (struct t_logger_buffer *logger_buffer)
{
    char *charset, *message, buf_time[256], buf_beginning[1024];
    int log_level, rc;
    struct timeval tv_now;
    struct stat statbuf;

    if (logger_buffer->log_file)
    {
        /*
         * check that the inode has not changed, otherwise that means the file
         * was deleted, and we must reopen it
         */
        rc = stat (logger_buffer->log_filename, &statbuf);
        if ((rc == 0) && (statbuf.st_ino == logger_buffer->log_file_inode))
        {
            /* inode has not changed, we can write in this file */
            return 1;
        }
        fclose (logger_buffer->log_file);
        logger_buffer->log_file = NULL;
        logger_buffer->log_file_inode = 0;
    }

    /* get log level */
    log_level = logger_get_level_for_buffer (logger_buffer->buffer);
    if (log_level == 0)
        return 0;

    /* create directory */
    if (!logger_create_directory ())
    {
        weechat_printf_date_tags (
            NULL, 0, "no_log",
            _("%s%s: unable to create directory for logs "
              "(\"%s\")"),
            weechat_prefix ("error"), LOGGER_PLUGIN_NAME,
            weechat_config_string (logger_config_file_path));
        return 0;
    }
    if (!logger_buffer->log_filename)
        logger_buffer_set_log_filename (logger_buffer);
    if (!logger_buffer->log_filename)
        return 0;

    /* create or append to log file */
    logger_buffer->log_file =
        fopen (logger_buffer->log_filename, "a");
    if (!logger_buffer->log_file)
    {
        weechat_printf_date_tags (
            NULL, 0, "no_log",
            _("%s%s: unable to write log file \"%s\": %s"),
            weechat_prefix ("error"), LOGGER_PLUGIN_NAME,
            logger_buffer->log_filename, strerror (errno));
        return 0;
    }

    /* get file inode */
    rc = stat (logger_buffer->log_filename, &statbuf);
    if (rc != 0)
    {
        weechat_printf_date_tags (
            NULL, 0, "no_log",
            _("%s%s: unable to get file status of log file \"%s\": %s"),
            weechat_prefix ("error"), LOGGER_PLUGIN_NAME,
            logger_buffer->log_filename, strerror (errno));
        fclose (logger_buffer->log_file);
        logger_buffer->log_file = NULL;
        logger_buffer->log_file_inode = 0;
        return 0;
    }
    logger_buffer->log_file_inode = statbuf.st_ino;

    /* write info line */
    if (weechat_config_boolean (logger_config_file_info_lines)
        && logger_buffer->write_start_info_line)
    {
        gettimeofday (&tv_now, NULL);
        weechat_util_strftimeval (
            buf_time, sizeof (buf_time),
            weechat_config_string (logger_config_file_time_format),
            &tv_now);
        snprintf (buf_beginning, sizeof (buf_beginning),
                  _("%s\t****  Beginning of log  ****"),
                  buf_time);
        charset = weechat_info_get ("charset_terminal", "");
        message = (charset) ?
            weechat_iconv_from_internal (charset, buf_beginning) : NULL;
        fprintf (logger_buffer->log_file,
                 "%s\n", (message) ? message : buf_beginning);
        free (charset);
        free (message);
        logger_buffer->flush_needed = 1;
    }
    logger_buffer->write_start_info_line = 0;

    return 1;
}

/*
 * Compresses a log file, in the child process.
 */

void
logger_buffer_compress_file (struct t_logger_buffer *logger_buffer)
{
    char filename[PATH_MAX], new_filename[PATH_MAX];
    const char *ptr_extension;
    int compression_type, compression_level;

    compression_type = weechat_config_enum (
        logger_config_file_rotation_compression_type);
    ptr_extension = logger_buffer_compression_extension[compression_type];

    snprintf (filename, sizeof (filename),
              "%s.1",
              logger_buffer->log_filename);
    snprintf (new_filename, sizeof (new_filename),
              "%s.1%s",
              logger_buffer->log_filename,
              ptr_extension);

    compression_level = weechat_config_integer (
        logger_config_file_rotation_compression_level);

    switch (compression_type)
    {
        case LOGGER_BUFFER_COMPRESSION_GZIP:
            if (weechat_file_compress (filename, new_filename,
                                       "gzip", compression_level))
            {
                unlink (filename);
            }
            break;
#ifdef HAVE_ZSTD
        case LOGGER_BUFFER_COMPRESSION_ZSTD:
            if (weechat_file_compress (filename, new_filename,
                                       "zstd", compression_level))
            {
                unlink (filename);
            }
            break;
#endif
        default:
            break;
    }

    exit (0);
}

/*
 * Compression callback.
 */

int
logger_buffer_compress_cb (const void *pointer, void *data,
                           const char *command, int return_code,
                           const char *out, const char *err)
{
    struct t_logger_buffer *logger_buffer;

    /* make C compiler happy */
    (void) data;
    (void) command;
    (void) return_code;
    (void) out;
    (void) err;

    logger_buffer = (struct t_logger_buffer *)pointer;
    if (!logger_buffer_valid (logger_buffer))
        return WEECHAT_RC_OK;

    if (return_code == WEECHAT_HOOK_PROCESS_CHILD)
    {
        logger_buffer_compress_file (logger_buffer);
    }
    else if (return_code >= 0)
    {
        logger_buffer->compressing = 0;
    }

    return WEECHAT_RC_OK;
}

/*
 * Rotates a log file if needed (rotation enabled and max size reached).
 *
 * For example if we have these log files:
 *
 *    irc.libera.#test.weechatlog  (current log file)
 *    irc.libera.#test.weechatlog.1
 *    irc.libera.#test.weechatlog.2
 *
 * The following renames are done in this order:
 *
 *    irc.libera.#test.weechatlog.2 -> irc.libera.#test.weechatlog.3
 *    irc.libera.#test.weechatlog.1 -> irc.libera.#test.weechatlog.2
 *    irc.libera.#test.weechatlog   -> irc.libera.#test.weechatlog.1
 *
 * And in case of compressed log files:
 *
 *    irc.libera.#test.weechatlog.2.gz -> irc.libera.#test.weechatlog.3.gz
 *    irc.libera.#test.weechatlog.1.gz -> irc.libera.#test.weechatlog.2.gz
 *    irc.libera.#test.weechatlog      -> irc.libera.#test.weechatlog.1
 *
 * Then the file irc.libera.#test.weechatlog is created again.
 */

void
logger_buffer_rotate (struct t_logger_buffer *logger_buffer)
{
    int compression_type, extension_index, found_comp, found_not_comp, i;
    char filename[PATH_MAX], new_filename[PATH_MAX];
    const char *ptr_extension;
    struct stat st;

    /* do not rotate if compression of log file is running */
    if (logger_buffer->compressing)
        return;

    /* do not rotate if rotation is disabled */
    if (logger_config_rotation_size_max == 0)
        return;

    /* do not rotate if max size is not reached */
    if (fstat (fileno (logger_buffer->log_file), &st) != 0)
        return;
    if (st.st_size <= (long int)logger_config_rotation_size_max)
        return;

    if (weechat_logger_plugin->debug)
    {
        weechat_log_printf ("logger: rotation for log: \"%s\"",
                            logger_buffer->log_filename);
    }

    compression_type = weechat_config_enum (
        logger_config_file_rotation_compression_type);

#ifndef HAVE_ZSTD
    if (compression_type == LOGGER_BUFFER_COMPRESSION_ZSTD)
        compression_type = LOGGER_BUFFER_COMPRESSION_NONE;
#endif

    ptr_extension = logger_buffer_compression_extension[compression_type];

    /* find the highest existing extension index */
    extension_index = 1;
    while (1)
    {
        found_comp = 0;
        found_not_comp = 0;
        if (ptr_extension[0])
        {
            /* try compressed file */
            snprintf (filename, sizeof (filename),
                      "%s.%d%s",
                      logger_buffer->log_filename,
                      extension_index,
                      ptr_extension);
            found_comp = (access (filename, F_OK) == 0);
        }
        if (!found_comp)
        {
            /* try non-compressed file */
            snprintf (filename, sizeof (filename),
                      "%s.%d",
                      logger_buffer->log_filename,
                      extension_index);
            found_not_comp = (access (filename, F_OK) == 0);
        }
        if (!found_comp && !found_not_comp)
            break;

        extension_index++;
    }
    extension_index--;

    /* close current log file */
    fclose (logger_buffer->log_file);
    logger_buffer->log_file = NULL;
    logger_buffer->log_file_inode = 0;

    /*
     * rename all files with an extension, starting with the higher one
     *
     * example with no compression enabled:
     *   .2" -> ".3" then ".1" -> ".2" then "" -> ".1"
     *
     * example with gzip compression:
     *   ".2.gz" -> ".3.gz" then ".1.gz" -> ".2.gz" then "" -> ".1"
     */
    for (i = extension_index; i >= 0; i--)
    {
        if (i == 0)
        {
            /* rename current log file to ".1" (no compression for now) */
            snprintf (filename, sizeof (filename),
                      "%s",
                      logger_buffer->log_filename);
            snprintf (new_filename, sizeof (new_filename),
                      "%s.%d",
                      logger_buffer->log_filename,
                      i + 1);
        }
        else
        {
            /* rename ".N" to ".N+1" */
            filename[0] = '\0';

            if (ptr_extension[0])
            {
                /* try compressed file */
                snprintf (filename, sizeof (filename),
                          "%s.%d%s",
                          logger_buffer->log_filename,
                          i,
                          ptr_extension);
                if (access (filename, F_OK) == 0)
                {
                    /* compressed file found, go on */
                    snprintf (new_filename, sizeof (new_filename),
                              "%s.%d%s",
                              logger_buffer->log_filename,
                              i + 1,
                              ptr_extension);
                }
                else
                {
                    filename[0] = '\0';
                }
            }
            if (!filename[0])
            {
                /* non-compressed file */
                snprintf (filename, sizeof (filename),
                          "%s.%d",
                          logger_buffer->log_filename,
                          i);
                snprintf (new_filename, sizeof (filename),
                          "%s.%d",
                          logger_buffer->log_filename,
                          i + 1);
            }
        }
        if (weechat_logger_plugin->debug)
        {
            weechat_log_printf ("logger: renaming \"%s\" to \"%s\"",
                                filename,
                                new_filename);
        }
        if (rename (filename, new_filename) != 0)
            break;
    }

    if (compression_type != LOGGER_BUFFER_COMPRESSION_NONE)
    {
        /* compress rotated log file */
        if (weechat_logger_plugin->debug)
        {
            weechat_log_printf ("logger: compressing \"%s.1\" => \"%s.1%s\"",
                                logger_buffer->log_filename,
                                logger_buffer->log_filename,
                                ptr_extension);
        }
        logger_buffer->compressing = 1;
        (void) weechat_hook_process ("func:compress",
                                     0,
                                     &logger_buffer_compress_cb,
                                     logger_buffer,
                                     NULL);
    }
}

/*
 * Writes a line to log file.
 */

void
logger_buffer_write_line (struct t_logger_buffer *logger_buffer,
                          const char *format, ...)
{
    char *charset, *message;

    if (!logger_buffer_create_log_file (logger_buffer))
        return;

    if (!logger_buffer->log_file)
        return;

    weechat_va_format (format);
    if (vbuffer)
    {
        charset = weechat_info_get ("charset_terminal", "");
        message = (charset) ?
            weechat_iconv_from_internal (charset, vbuffer) : NULL;
        fprintf (logger_buffer->log_file,
                 "%s\n", (message) ? message : vbuffer);
        free (charset);
        free (message);
        logger_buffer->flush_needed = 1;
        if (!logger_hook_timer)
        {
            fflush (logger_buffer->log_file);
            if (weechat_config_boolean (logger_config_file_fsync))
                fsync (fileno (logger_buffer->log_file));
            logger_buffer->flush_needed = 0;
            logger_buffer_rotate (logger_buffer);
        }
        free (vbuffer);
    }
}

/*
 * Stops log for a logger buffer.
 */

void
logger_buffer_stop (struct t_logger_buffer *logger_buffer, int write_info_line)
{
    struct timeval tv_now;
    char buf_time[256];

    if (!logger_buffer)
        return;

    if (logger_buffer->log_enabled && logger_buffer->log_file)
    {
        if (write_info_line && weechat_config_boolean (logger_config_file_info_lines))
        {
            gettimeofday (&tv_now, NULL);
            weechat_util_strftimeval (
                buf_time, sizeof (buf_time),
                weechat_config_string (logger_config_file_time_format),
                &tv_now);
            logger_buffer_write_line (logger_buffer,
                                      _("%s\t****  End of log  ****"),
                                      buf_time);
        }
    }

    logger_buffer_free (logger_buffer);
}

/*
 * Ends log for all buffers.
 */

void
logger_buffer_stop_all (int write_info_line)
{
    while (logger_buffers)
    {
        logger_buffer_stop (logger_buffers, write_info_line);
    }
}

/*
 * Starts logging for a buffer.
 */

void
logger_buffer_start (struct t_gui_buffer *buffer, int write_info_line)
{
    struct t_logger_buffer *ptr_logger_buffer;
    int log_level, log_enabled;

    if (!buffer)
        return;

    log_level = logger_get_level_for_buffer (buffer);
    log_enabled =  weechat_config_boolean (logger_config_file_auto_log)
        && (log_level > 0)
        && (logger_check_conditions (
                buffer,
                weechat_config_string (logger_config_file_log_conditions)));

    ptr_logger_buffer = logger_buffer_search_buffer (buffer);

    /* logging is disabled for buffer */
    if (!log_enabled)
    {
        /* stop logger if it is active */
        if (ptr_logger_buffer)
            logger_buffer_stop (ptr_logger_buffer, 1);
    }
    else
    {
        /* logging is enabled for buffer */
        if (ptr_logger_buffer)
            ptr_logger_buffer->log_level = log_level;
        else
        {
            ptr_logger_buffer = logger_buffer_add (buffer, log_level);

            if (ptr_logger_buffer)
            {
                if (ptr_logger_buffer->log_filename)
                {
                    if (ptr_logger_buffer->log_file)
                    {
                        fclose (ptr_logger_buffer->log_file);
                        ptr_logger_buffer->log_file = NULL;
                        ptr_logger_buffer->log_file_inode = 0;
                    }
                }
            }
        }
        if (ptr_logger_buffer)
            ptr_logger_buffer->write_start_info_line = write_info_line;
    }
}

/*
 * Starts logging for all buffers.
 */

void
logger_buffer_start_all (int write_info_line)
{
    struct t_infolist *ptr_infolist;

    ptr_infolist = weechat_infolist_get ("buffer", NULL, NULL);
    if (ptr_infolist)
    {
        while (weechat_infolist_next (ptr_infolist))
        {
            logger_buffer_start (weechat_infolist_pointer (ptr_infolist,
                                                           "pointer"),
                                 write_info_line);
        }
        weechat_infolist_free (ptr_infolist);
    }
}

/*
 * Flushes all log files.
 */

void
logger_buffer_flush ()
{
    struct t_logger_buffer *ptr_logger_buffer;

    for (ptr_logger_buffer = logger_buffers; ptr_logger_buffer;
         ptr_logger_buffer = ptr_logger_buffer->next_buffer)
    {
        if (ptr_logger_buffer->log_file && ptr_logger_buffer->flush_needed)
        {
            if (weechat_logger_plugin->debug >= 2)
            {
                weechat_printf_date_tags (NULL, 0, "no_log",
                                          "%s: flush file %s",
                                          LOGGER_PLUGIN_NAME,
                                          ptr_logger_buffer->log_filename);
            }
            fflush (ptr_logger_buffer->log_file);
            if (weechat_config_boolean (logger_config_file_fsync))
                fsync (fileno (ptr_logger_buffer->log_file));
            ptr_logger_buffer->flush_needed = 0;
            logger_buffer_rotate (ptr_logger_buffer);
        }
    }
}

/*
 * Adjusts log filenames for all buffers.
 *
 * Filename can change if configuration option is changed, or if day of system
 * date has changed.
 */

void
logger_buffer_adjust_log_filenames ()
{
    struct t_infolist *ptr_infolist;
    struct t_logger_buffer *ptr_logger_buffer;
    struct t_gui_buffer *ptr_buffer;
    char *log_filename;

    ptr_infolist = weechat_infolist_get ("buffer", NULL, NULL);
    if (ptr_infolist)
    {
        while (weechat_infolist_next (ptr_infolist))
        {
            ptr_buffer = weechat_infolist_pointer (ptr_infolist, "pointer");
            ptr_logger_buffer = logger_buffer_search_buffer (ptr_buffer);
            if (ptr_logger_buffer && ptr_logger_buffer->log_filename)
            {
                log_filename = logger_get_filename (ptr_logger_buffer->buffer);
                if (log_filename)
                {
                    if (strcmp (log_filename, ptr_logger_buffer->log_filename) != 0)
                    {
                        /*
                         * log filename has changed (probably due to day
                         * change),then we'll use new filename
                         */
                        logger_buffer_stop (ptr_logger_buffer, 1);
                        logger_buffer_start (ptr_buffer, 1);
                    }
                    free (log_filename);
                }
            }
        }
        weechat_infolist_free (ptr_infolist);
    }
}

/*
 * Removes a logger buffer from list.
 */

void
logger_buffer_free (struct t_logger_buffer *logger_buffer)
{
    struct t_logger_buffer *new_logger_buffers;
    struct t_gui_buffer *ptr_buffer;

    if (!logger_buffer)
        return;

    ptr_buffer = logger_buffer->buffer;

    /* remove logger buffer */
    if (last_logger_buffer == logger_buffer)
        last_logger_buffer = logger_buffer->prev_buffer;
    if (logger_buffer->prev_buffer)
    {
        (logger_buffer->prev_buffer)->next_buffer = logger_buffer->next_buffer;
        new_logger_buffers = logger_buffers;
    }
    else
        new_logger_buffers = logger_buffer->next_buffer;

    if (logger_buffer->next_buffer)
        (logger_buffer->next_buffer)->prev_buffer = logger_buffer->prev_buffer;

    /* free data */
    free (logger_buffer->log_filename);
    if (logger_buffer->log_file)
        fclose (logger_buffer->log_file);

    free (logger_buffer);

    logger_buffers = new_logger_buffers;

    if (weechat_logger_plugin->debug)
    {
        weechat_printf_date_tags (
            NULL, 0, "no_log",
            "%s: stop logging for buffer \"%s\"",
            LOGGER_PLUGIN_NAME,
            weechat_buffer_get_string (ptr_buffer, "name"));
    }
}

/*
 * Adds a logger buffer in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
logger_buffer_add_to_infolist (struct t_infolist *infolist,
                               struct t_logger_buffer *logger_buffer)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !logger_buffer)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_pointer (ptr_item, "buffer", logger_buffer->buffer))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "log_filename", logger_buffer->log_filename))
        return 0;
    if (!weechat_infolist_new_var_pointer (ptr_item, "log_file", logger_buffer->log_file))
        return 0;
    if (!weechat_infolist_new_var_buffer (ptr_item, "log_file_inode", &(logger_buffer->log_file_inode), sizeof(logger_buffer->log_file_inode)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "log_enabled", logger_buffer->log_enabled))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "log_level", logger_buffer->log_level))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "write_start_info_line", logger_buffer->write_start_info_line))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "flush_needed", logger_buffer->flush_needed))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "compressing", logger_buffer->compressing))
        return 0;

    return 1;
}
