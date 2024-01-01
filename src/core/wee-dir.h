/*
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

#ifndef WEECHAT_DIR_H
#define WEECHAT_DIR_H

extern char *dir_get_temp_dir();
extern int dir_mkdir_home (const char *directory, int mode);
extern int dir_mkdir (const char *directory, int mode);
extern int dir_mkdir_parents (const char *directory, int mode);
extern int dir_rmtree (const char *directory);
extern void dir_create_home_dirs ();
extern void dir_remove_home_dirs ();
extern char *dir_get_string_home_dirs ();
extern void dir_exec_on_files (const char *directory, int recurse_subdirs,
                               int hidden_files,
                               void (*callback)(void *data,
                                                const char *filename),
                               void *callback_data);
extern char *dir_search_full_lib_name (const char *filename,
                                       const char *sys_directory);
extern char *dir_file_get_content (const char *filename);
extern int dir_file_copy (const char *from, const char *to);
extern int dir_file_compress (const char *from, const char *to,
                              const char *compressor, int compression_level);

#endif /* WEECHAT_DIR_H */
