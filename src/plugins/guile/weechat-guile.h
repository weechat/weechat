/*
 * Copyright (C) 2011-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_GUILE_H
#define WEECHAT_PLUGIN_GUILE_H

#include <libguile.h>

#define weechat_plugin weechat_guile_plugin
#define GUILE_PLUGIN_NAME "guile"
#define GUILE_PLUGIN_PRIORITY 4070

#define GUILE_CURRENT_SCRIPT_NAME ((guile_current_script) ? guile_current_script->name : "-")

extern struct t_weechat_plugin *weechat_guile_plugin;

extern struct t_plugin_script_data guile_data;

extern int guile_quiet;
extern struct t_plugin_script *guile_scripts;
extern struct t_plugin_script *last_guile_script;
extern struct t_plugin_script *guile_current_script;
extern struct t_plugin_script *guile_registered_script;
extern const char *guile_current_script_filename;
extern SCM guile_port;

extern SCM weechat_guile_hashtable_to_alist (struct t_hashtable *hashtable);
extern struct t_hashtable *weechat_guile_alist_to_hashtable (SCM dict,
                                                             int size,
                                                             const char *type_keys,
                                                             const char *type_values);
extern void *weechat_guile_exec (struct t_plugin_script *script,
                                  int ret_type, const char *function,
                                  char *format, void **argv);
#if SCM_MAJOR_VERSION >= 3 || (SCM_MAJOR_VERSION == 2 && SCM_MINOR_VERSION >= 2)
/* Guile >= 2.2 */
extern size_t weechat_guile_port_fill_input (SCM port, SCM dst,
                                             size_t start, size_t count);
extern size_t weechat_guile_port_write (SCM port, SCM src,
                                        size_t start, size_t count);
#else
/* Guile < 2.2 */
extern int weechat_guile_port_fill_input (SCM port);
extern void weechat_guile_port_write (SCM port, const void *data, size_t size);
#endif

#endif /* WEECHAT_PLUGIN_GUILE_H */
