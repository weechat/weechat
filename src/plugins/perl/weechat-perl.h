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

#ifndef WEECHAT_PLUGIN_PERL_H
#define WEECHAT_PLUGIN_PERL_H

#define weechat_plugin weechat_perl_plugin
#define PERL_PLUGIN_NAME "perl"
#define PERL_PLUGIN_PRIORITY 4040

#define PERL_CURRENT_SCRIPT_NAME ((perl_current_script) ? perl_current_script->name : "-")

extern struct t_weechat_plugin *weechat_perl_plugin;

extern struct t_plugin_script_data perl_data;

extern int perl_quiet;
extern struct t_plugin_script *perl_scripts;
extern struct t_plugin_script *last_perl_script;
extern struct t_plugin_script *perl_current_script;
extern struct t_plugin_script *perl_registered_script;
extern const char *perl_current_script_filename;
#ifdef MULTIPLICITY
extern PerlInterpreter *perl_current_interpreter;
#endif /* MULTIPLICITY */

extern HV *weechat_perl_hashtable_to_hash (struct t_hashtable *hashtable);
extern struct t_hashtable *weechat_perl_hash_to_hashtable (SV *hash, int size,
                                                           const char *type_keys,
                                                           const char *type_values);
extern XS (weechat_perl_output);
extern void *weechat_perl_exec (struct t_plugin_script *script,
                                int ret_type, const char *function,
                                const char *format, void **argv);

#endif /* WEECHAT_PLUGIN_PERL_H */
