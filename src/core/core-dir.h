/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_DIR_H
#define WEECHAT_DIR_H

extern char *dir_get_temp_dir (void);
extern int dir_mkdir_home (const char *directory, int mode);
extern int dir_mkdir (const char *directory, int mode);
extern int dir_mkdir_parents (const char *directory, int mode);
extern int dir_rmtree (const char *directory);
extern void dir_create_home_dirs (void);
extern void dir_remove_home_dirs (void);
extern char *dir_get_string_home_dirs (void);
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
extern int dir_file_compare (const char *filename1, const char *filename2);

#endif /* WEECHAT_DIR_H */
