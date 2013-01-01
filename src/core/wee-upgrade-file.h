/*
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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

#ifndef __WEECHAT_UPGRADE_FILE_H
#define __WEECHAT_UPGRADE_FILE_H 1

#define UPGRADE_SIGNATURE "===== WeeChat Upgrade file v2.2 - binary, do not edit! ====="

#define UPGRADE_ERROR(msg1, msg2)                                       \
    upgrade_file_error(upgrade_file, msg1, msg2, __FILE__, __LINE__)

struct t_infolist;

enum t_upgrade_type
{
    UPGRADE_TYPE_OBJECT_START = 0,
    UPGRADE_TYPE_OBJECT_END,
    UPGRADE_TYPE_OBJECT_VAR,
};

struct t_upgrade_file
{
    char *filename;                        /* filename with path            */
    FILE *file;                            /* file pointer                  */
    long last_read_pos;                    /* last read position            */
    int last_read_length;                  /* last read length              */
    int (*callback_read)                   /* callback called when reading  */
    (void *data,                           /* file                          */
     struct t_upgrade_file *upgrade_file,
     int object_id,
     struct t_infolist *infolist);
    void *callback_read_data;              /* data sent to callback         */
    struct t_upgrade_file *prev_upgrade;   /* link to previous upgrade file */
    struct t_upgrade_file *next_upgrade;   /* link to next upgrade file     */
};

extern struct t_upgrade_file *upgrade_file_new (const char *filename,
                                                int write);
extern int upgrade_file_write_object (struct t_upgrade_file *upgrade_file,
                                      int object_id,
                                      struct t_infolist *infolist);
extern int upgrade_file_read (struct t_upgrade_file *upgrade_file,
                              int (*callback_read)(void *data,
                                                   struct t_upgrade_file *upgrade_file,
                                                   int object_id,
                                                   struct t_infolist *infolist),
                              void *callback_read_data);
extern void upgrade_file_close (struct t_upgrade_file *upgrade_file);

#endif /* __WEECHAT_UPGRADE_FILE_H */
