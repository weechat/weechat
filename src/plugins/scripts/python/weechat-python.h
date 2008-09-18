/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __WEECHAT_PYTHON_H
#define __WEECHAT_PYTHON_H 1

#define weechat_plugin weechat_python_plugin
#define PYTHON_PLUGIN_NAME "python"

extern struct t_weechat_plugin *weechat_python_plugin;

extern struct t_plugin_script *python_scripts;
extern struct t_plugin_script *python_current_script;
extern const char *python_current_script_filename;

extern void *weechat_python_exec (struct t_plugin_script *script,
                                  int ret_type, const char *function,
                                  char **argv);

#endif /* weechat-perl.h */
