/*
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

#ifndef WEECHAT_PLUGIN_LOGGER_BUFFER_H
#define WEECHAT_PLUGIN_LOGGER_BUFFER_H

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

enum t_logger_buffer_compression
{
    LOGGER_BUFFER_COMPRESSION_NONE = 0,
    LOGGER_BUFFER_COMPRESSION_GZIP,
    LOGGER_BUFFER_COMPRESSION_ZSTD,
    /* number of compression types */
    LOGGER_BUFFER_NUM_COMPRESSION_TYPES,
};

struct t_infolist;

struct t_logger_buffer
{
    struct t_gui_buffer *buffer;          /* pointer to buffer              */
    char *log_filename;                   /* log filename                   */
    FILE *log_file;                       /* log file                       */
    ino_t log_file_inode;                 /* inode of log file              */
    int log_enabled;                      /* log enabled ?                  */
    int log_level;                        /* log level (0..9)               */
    int write_start_info_line;            /* 1 if start info line must be   */
                                          /* written in file                */
    int flush_needed;                     /* flush needed?                  */
    int compressing;                      /* compressing rotated log, this  */
                                          /* prevents any new rotation      */
                                          /* before the end of compression  */
    struct t_logger_buffer *prev_buffer;  /* link to previous buffer        */
    struct t_logger_buffer *next_buffer;  /* link to next buffer            */
};

extern struct t_logger_buffer *logger_buffers;
extern struct t_logger_buffer *last_logger_buffer;

extern int logger_buffer_valid (struct t_logger_buffer *logger_buffer);
extern struct t_logger_buffer *logger_buffer_add (struct t_gui_buffer *buffer,
                                                  int log_level);
extern struct t_logger_buffer *logger_buffer_search_buffer (struct t_gui_buffer *buffer);
extern struct t_logger_buffer *logger_buffer_search_log_filename (const char *log_filename);
extern void logger_buffer_set_log_filename (struct t_logger_buffer *logger_buffer);
extern int logger_buffer_create_log_file (struct t_logger_buffer *logger_buffer);
extern void logger_buffer_rotate (struct t_logger_buffer *logger_buffer);
extern void logger_buffer_write_line (struct t_logger_buffer *logger_buffer,
                                      const char *format, ...);
extern void logger_buffer_stop (struct t_logger_buffer *logger_buffer,
                                int write_info_line);
extern void logger_buffer_stop_all (int write_info_line);
extern void logger_buffer_start (struct t_gui_buffer *buffer, int write_info_line);
extern void logger_buffer_start_all (int write_info_line);
extern void logger_buffer_flush ();
extern void logger_buffer_adjust_log_filenames ();
extern void logger_buffer_free (struct t_logger_buffer *logger_buffer);
extern int logger_buffer_add_to_infolist (struct t_infolist *infolist,
                                          struct t_logger_buffer *logger_buffer);

#endif /* WEECHAT_PLUGIN_LOGGER_BUFFER_H */
