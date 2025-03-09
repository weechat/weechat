/*
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_SCRIPT_REPO_H
#define WEECHAT_PLUGIN_SCRIPT_REPO_H

#include <time.h>

/* status for script */
#define SCRIPT_STATUS_INSTALLED   (1 << 0)
#define SCRIPT_STATUS_AUTOLOADED  (1 << 1)
#define SCRIPT_STATUS_HELD        (1 << 2)
#define SCRIPT_STATUS_RUNNING     (1 << 3)
#define SCRIPT_STATUS_NEW_VERSION (1 << 4)

struct t_script_repo
{
    char *name;                          /* script name                     */
    char *name_with_extension;           /* script name with extension      */
    int language;                        /* language index                  */
    char *author;                        /* author                          */
    char *mail;                          /* author's mail                   */
    char *version;                       /* plugin version                  */
    char *license;                       /* license                         */
    char *description;                   /* description                     */
    char *tags;                          /* comma-separated list of tags    */
    char *requirements;                  /* requirements                    */
    char *min_weechat;                   /* min WeeChat version             */
    char *max_weechat;                   /* max WeeChat version             */
    char *sha512sum;                     /* SHA-512 checksum of script      */
    char *url;                           /* URL to download script          */
    int popularity;                      /* >0 for popular scripts only     */
    time_t date_added;                   /* date added                      */
    time_t date_updated;                 /* date updated                    */
    int status;                          /* installed/running/new version   */
    char *version_loaded;                /* version of script loaded        */
    int displayed;                       /* script displayed?               */
    int install_order;                   /* order for install script (if >0)*/
    struct t_script_repo *prev_script;   /* link to previous script         */
    struct t_script_repo *next_script;   /* link to next script             */
};

extern struct t_script_repo *scripts_repo;
extern struct t_script_repo *last_script_repo;
extern int script_repo_count, script_repo_count_displayed;
extern struct t_hashtable *script_repo_max_length_field;
extern char *script_repo_filter;

extern int script_repo_script_valid (struct t_script_repo *script);
extern struct t_script_repo *script_repo_search_displayed_by_number (int number);
extern struct t_script_repo *script_repo_search_by_name (const char *name);
extern struct t_script_repo *script_repo_search_by_name_ext (const char *name_with_extension);
extern char *script_repo_get_filename_loaded (struct t_script_repo *script);
extern const char *script_repo_get_status_for_display (struct t_script_repo *script,
                                                       const char *list,
                                                       int collapse);
extern const char *script_repo_get_status_desc_for_display (struct t_script_repo *script,
                                                            const char *list);
extern void script_repo_remove_all (void);
extern void script_repo_update_status (struct t_script_repo *script);
extern void script_repo_update_status_all (void);
extern void script_repo_set_filter (const char *filter);
extern void script_repo_filter_scripts (const char *search);
extern int script_repo_file_exists (void);
extern int script_repo_file_is_uptodate (void);
extern int script_repo_file_read (int quiet);
extern int script_repo_file_update (int quiet);
extern struct t_hdata *script_repo_hdata_script_cb (const void *pointer,
                                                    void *data,
                                                    const char *hdata_name);
extern int script_repo_add_to_infolist (struct t_infolist *infolist,
                                        struct t_script_repo *script);
extern void script_repo_print_log (void);

#endif /* WEECHAT_PLUGIN_SCRIPT_REPO_H */
