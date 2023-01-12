/*
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005-2007 Emmanuel Bouthenot <kolter@openics.org>
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

#ifndef WEECHAT_PLUGIN_PYTHON_H
#define WEECHAT_PLUGIN_PYTHON_H

#define weechat_plugin weechat_python_plugin
#define PYTHON_PLUGIN_NAME "python"
#define PYTHON_PLUGIN_PRIORITY 4020

#define PYTHON_CURRENT_SCRIPT_NAME ((python_current_script) ? python_current_script->name : "-")

#define PY_INTEGER_CHECK(x) (PyLong_Check(x))

extern struct t_weechat_plugin *weechat_python_plugin;

extern struct t_plugin_script_data python_data;

extern int python_quiet;
extern struct t_plugin_script *python_scripts;
extern struct t_plugin_script *last_python_script;
extern struct t_plugin_script *python_current_script;
extern struct t_plugin_script *python_registered_script;
extern const char *python_current_script_filename;
extern PyThreadState *python_current_interpreter;

extern PyObject *weechat_python_hashtable_to_dict (struct t_hashtable *hashtable);
extern struct t_hashtable *weechat_python_dict_to_hashtable (PyObject *dict,
                                                             int size,
                                                             const char *type_keys,
                                                             const char *type_values);
extern void *weechat_python_exec (struct t_plugin_script *script,
                                  int ret_type, const char *function,
                                  const char *format, void **argv);

#endif /* WEECHAT_PLUGIN_PYTHON_H */
