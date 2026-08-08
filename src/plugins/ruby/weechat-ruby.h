/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 * SPDX-FileCopyrightText: 2005-2007 Emmanuel Bouthenot <kolter@openics.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_RUBY_H
#define WEECHAT_PLUGIN_RUBY_H

#define weechat_plugin weechat_ruby_plugin
#define RUBY_PLUGIN_NAME "ruby"
#define RUBY_PLUGIN_PRIORITY 4010

#define RUBY_CURRENT_SCRIPT_NAME ((ruby_current_script) ? ruby_current_script->name : "-")

extern struct t_weechat_plugin *weechat_ruby_plugin;

extern struct t_plugin_script_data ruby_data;

extern int ruby_quiet;
extern struct t_plugin_script *ruby_scripts;
extern struct t_plugin_script *last_ruby_script;
extern struct t_plugin_script *ruby_current_script;
extern struct t_plugin_script *ruby_registered_script;
extern const char *ruby_current_script_filename;
extern VALUE ruby_current_module;

extern VALUE weechat_ruby_hashtable_to_hash (struct t_hashtable *hashtable);
extern struct t_hashtable *weechat_ruby_hash_to_hashtable (VALUE dict,
                                                           int size,
                                                           const char *type_keys,
                                                           const char *type_values);
void *weechat_ruby_exec (struct t_plugin_script *script,
                         int ret_type, const char *function,
                         const char *format, void **argv);

#endif /* WEECHAT_PLUGIN_RUBY_H */
