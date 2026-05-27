/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

#ifndef WEECHAT_THEME_H
#define WEECHAT_THEME_H

struct t_hashtable;
struct t_arraylist;

struct t_theme
{
    char *name;                           /* "dark", "solarized", ...        */
    char *description;                    /* free-form text                  */
    char *date;                           /* "YYYY-MM-DD HH:MM:SS"           */
    char *weechat_version;                /* version at registration time    */
    struct t_hashtable *overrides;        /* full_option_name -> value       */
    struct t_theme *prev_theme;           /* link to previous theme          */
    struct t_theme *next_theme;           /* link to next theme              */
};

extern struct t_theme *themes;
extern struct t_theme *last_theme;
extern int theme_applying;                /* gate for config_change_color    */

extern struct t_theme *theme_search (const char *name);
extern struct t_theme *theme_register (const char *name,
                                       struct t_hashtable *overrides);
extern struct t_arraylist *theme_list (void);
extern int theme_apply (const char *name);
extern int theme_save (const char *name, int full);
extern int theme_delete (const char *name);
extern char *theme_make_backup (void);
extern char *theme_user_file_path (const char *name);
extern struct t_theme *theme_file_parse (const char *path);
extern void theme_free (struct t_theme *theme);

extern void theme_init (void);
extern void theme_end (void);

/* implemented in core-theme-builtin.c */
extern void theme_builtin_init (void);

#endif /* WEECHAT_THEME_H */
