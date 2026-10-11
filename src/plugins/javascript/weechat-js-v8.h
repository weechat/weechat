/*
 * SPDX-FileCopyrightText: 2013 Koka El Kiwi <kokakiwi@kokakiwi.net>
 * SPDX-FileCopyrightText: 2015-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_JS_V8_H
#define WEECHAT_PLUGIN_JS_V8_H

#include <cstdio>
#include <string>
#include <v8.h>
#include <v8-version.h>

#if (V8_MAJOR_VERSION < 11) || ((V8_MAJOR_VERSION == 11) && (V8_MINOR_VERSION < 3))
#error "V8 >= 11.3 is required"
#endif

class WeechatJsV8
{
public:
    WeechatJsV8 (void);
    ~WeechatJsV8 (void);

    bool load (const char *);

    bool execScript (void);
    v8::Local<v8::Context> getContext (void);
    bool functionExists (const char *);
    v8::Local<v8::Value> execFunction (const char *,
                                       int argc, v8::Local<v8::Value> *);

    void loadLibs (v8::Local<v8::ObjectTemplate>);

private:
    v8::Global<v8::Context> context;

    std::string source;
};

extern void weechat_js_v8_pin_library (void);
extern void weechat_js_v8_init (void);
extern void weechat_js_v8_run_pending_tasks (void);
extern void weechat_js_v8_end (void);

#endif /* WEECHAT_PLUGIN_JS_V8_H */
