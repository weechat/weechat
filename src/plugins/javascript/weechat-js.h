/*
 * Copyright (C) 2013 Koka El Kiwi <kokakiwi@kokakiwi.net>
 * Copyright (C) 2015 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WEECHAT_JS_H
#define WEECHAT_JS_H 1

#include <v8.h>

#ifdef __cplusplus
#ifdef _WIN32
#define EXPORT extern "C" __declspec (dllexport)
#else
#define EXPORT extern "C"
#endif
#else
#define EXPORT
#endif

#define weechat_plugin weechat_js_plugin
#define JS_PLUGIN_NAME "javascript"

#define JS_CURRENT_SCRIPT_NAME ((js_current_script) ? js_current_script->name : "-")

class WeechatJsV8;

extern struct t_weechat_plugin *weechat_js_plugin;

extern int js_quiet;
extern struct t_plugin_script *js_scripts;
extern struct t_plugin_script *last_js_script;
extern struct t_plugin_script *js_current_script;
extern struct t_plugin_script *js_registered_script;
extern const char *js_current_script_filename;
extern WeechatJsV8 *js_current_interpreter;

extern v8::Handle<v8::Object> weechat_js_hashtable_to_object (struct t_hashtable *hashtable);
extern struct t_hashtable *weechat_js_object_to_hashtable (v8::Handle<v8::Object> obj,
                                                           int size,
                                                           const char *type_keys,
                                                           const char *type_values);
extern void *weechat_js_exec (struct t_plugin_script *script,
                              int ret_type, const char *function,
                              const char *format, void **argv);

#endif /* WEECHAT_JS_H */
