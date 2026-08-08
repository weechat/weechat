/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_SCRIPT_H
#define WEECHAT_PLUGIN_SCRIPT_H

#define weechat_plugin weechat_script_plugin
#define SCRIPT_PLUGIN_NAME "script"
#define SCRIPT_PLUGIN_PRIORITY 3000

#define SCRIPT_NUM_LANGUAGES 8

extern struct t_weechat_plugin *weechat_script_plugin;

extern char *script_language[SCRIPT_NUM_LANGUAGES];
extern char *script_extension[SCRIPT_NUM_LANGUAGES];
extern int script_plugin_loaded[SCRIPT_NUM_LANGUAGES];
extern struct t_hashtable *script_loaded;

extern int script_language_search (const char *language);
extern int script_language_search_by_extension (const char *extension);
extern int script_download_enabled (int display_error);
extern void script_get_loaded_plugins (void);
extern void script_get_scripts (void);

#endif /* WEECHAT_PLUGIN_SCRIPT_H */
