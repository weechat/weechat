/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_THEME_H
#define WEECHAT_THEME_H

struct t_hashtable;
struct t_arraylist;
struct t_weechat_plugin;

/*
 * A contribution is one (owner, overrides) pair attached to a theme.
 * "owner" is identified by a (plugin, script) pair:
 *   - plugin == NULL && script == NULL  =>  core
 *   - plugin != NULL && script == NULL  =>  plugin (e.g. irc, fset)
 *   - plugin != NULL && script != NULL  =>  individual script under that
 *                                           script-language plugin
 */
struct t_theme_contribution
{
    struct t_weechat_plugin *plugin;
    const void *script;
    struct t_hashtable *overrides;        /* full_option_name -> value       */
    struct t_theme_contribution *prev_contribution;
    struct t_theme_contribution *next_contribution;
};

struct t_theme
{
    char *name;                           /* "light", "solarized", ...       */
    char *description;                    /* free-form text                  */
    char *date;                           /* "YYYY-MM-DD HH:MM:SS"           */
    char *weechat_version;                /* version at registration time    */
    struct t_theme_contribution *contributions;
    struct t_theme_contribution *last_contribution;
    struct t_theme *prev_theme;
    struct t_theme *next_theme;
};

extern struct t_theme *themes;
extern struct t_theme *last_theme;
extern int theme_applying;                /* gate for config_change_color    */

extern struct t_theme *theme_search (const char *name);
extern struct t_theme *theme_register (struct t_weechat_plugin *plugin,
                                       const void *script,
                                       const char *name,
                                       struct t_hashtable *overrides);
extern int theme_overrides_count (struct t_theme *theme);
extern const char *theme_get_override (struct t_theme *theme,
                                       const char *option_name);
extern struct t_arraylist *theme_list (void);
extern int theme_apply (const char *name);
extern int theme_reset (void);
extern int theme_save (const char *name);
extern int theme_rename (const char *old_name, const char *new_name);
extern int theme_delete (const char *name);
extern char *theme_make_backup (void);
extern char *theme_user_file_path (const char *name);
extern struct t_theme *theme_file_parse (const char *path);
extern void theme_free (struct t_theme *theme);

/* Lifecycle: drop all contributions owned by a plugin or script. */
extern void theme_unregister_plugin (struct t_weechat_plugin *plugin);
extern void theme_unregister_script (struct t_weechat_plugin *plugin,
                                     const void *script);

extern void theme_init (void);
extern void theme_end (void);

/* Implemented in core-theme-builtin.c */
extern void theme_builtin_init (void);

#endif /* WEECHAT_THEME_H */
