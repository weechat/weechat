/*
 * Copyright (C) 2013 Koka El Kiwi <kokakiwi@kokakiwi.net>
 * Copyright (C) 2015-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_JS_V8_H
#define WEECHAT_PLUGIN_JS_V8_H

#include <cstdio>
#include <v8.h>

class WeechatJsV8
{
public:
    WeechatJsV8 (void);
    ~WeechatJsV8 (void);

    bool load (v8::Handle<v8::String>);
    bool load (const char *);

    bool execScript (void);
    bool functionExists (const char *);
    v8::Handle<v8::Value> execFunction (const char *,
                                        int argc, v8::Handle<v8::Value> *);

    void addGlobal (v8::Handle<v8::String>, v8::Handle<v8::Template>);
    void addGlobal (const char *, v8::Handle<v8::Template>);

    void loadLibs (void);

private:
    v8::HandleScope handle_scope;
    v8::Handle<v8::ObjectTemplate> global;
    v8::Persistent<v8::Context> context;

    v8::Handle<v8::String> source;
};

#endif /* WEECHAT_PLUGIN_JS_V8_H */
