/*
 * Copyright (C) 2008-2010 Dmitry Kobylin <fnfal@academ.tsc.ru>
 * Copyright (C) 2008-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_TCL_H
#define WEECHAT_PLUGIN_TCL_H

#define weechat_plugin weechat_tcl_plugin
#define TCL_PLUGIN_NAME "tcl"
#define TCL_PLUGIN_PRIORITY 4000

#define TCL_CURRENT_SCRIPT_NAME ((tcl_current_script) ? tcl_current_script->name : "-")

extern struct t_weechat_plugin *weechat_tcl_plugin;

extern struct t_plugin_script_data tcl_data;

extern int tcl_quiet;
extern struct t_plugin_script *tcl_scripts;
extern struct t_plugin_script *last_tcl_script;
extern struct t_plugin_script *tcl_current_script;
extern struct t_plugin_script *tcl_registered_script;
extern const char *tcl_current_script_filename;

extern Tcl_Obj *weechat_tcl_hashtable_to_dict (Tcl_Interp *interp,
                                               struct t_hashtable *hashtable);
extern struct t_hashtable *weechat_tcl_dict_to_hashtable (Tcl_Interp *interp,
                                                          Tcl_Obj *dict,
                                                          int size,
                                                          const char *type_keys,
                                                          const char *type_values);
extern void *weechat_tcl_exec (struct t_plugin_script *script,
                               int ret_type, const char *function,
                               const char *format, void **argv);

#endif /* WEECHAT_PLUGIN_TCL_H */
