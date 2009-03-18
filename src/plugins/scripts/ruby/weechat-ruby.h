/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_RUBY_H
#define __WEECHAT_RUBY_H 1

#define weechat_plugin weechat_ruby_plugin
#define RUBY_PLUGIN_NAME "ruby"

extern struct t_weechat_plugin *weechat_ruby_plugin;

extern int ruby_quiet;
extern struct t_plugin_script *ruby_scripts;
extern struct t_plugin_script *last_ruby_script;
extern struct t_plugin_script *ruby_current_script;
extern const char *ruby_current_script_filename;

extern void *weechat_ruby_exec (struct t_plugin_script *script,
                                int ret_type, const char *function,
                                char **argv);

#endif /* weechat-perl.h */
