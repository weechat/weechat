/*
 * SPDX-FileCopyrightText: 2013 Koka El Kiwi <kokakiwi@kokakiwi.net>
 * SPDX-FileCopyrightText: 2015-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
