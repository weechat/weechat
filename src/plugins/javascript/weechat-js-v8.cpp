/*
 * SPDX-FileCopyrightText: 2013 Koka El Kiwi <kokakiwi@kokakiwi.net>
 * SPDX-FileCopyrightText: 2015-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* V8 JavaScript functions */

#undef _
#include <cstdio>
#include <dlfcn.h>
#include <libplatform/libplatform.h>

extern "C"
{
#include "../weechat-plugin.h"
#include "../plugin-script.h"
}

#include "weechat-js.h"
#include "weechat-js-v8.h"

#define PRINT_EXCEPTION                                                 \
    String::Utf8Value str_exception(js_isolate, trycatch.Exception());  \
    weechat_printf (NULL,                                               \
                    weechat_gettext ("%s%s: exception: %s"),            \
                    weechat_prefix ("error"), JS_PLUGIN_NAME,           \
                    (*str_exception) ? *str_exception : "?");

using namespace v8;

/*
 * V8 can be initialized only once in a process (it can not be initialized
 * again after V8::Dispose), so the platform and the isolate are created on
 * first load of the plugin and kept until the end of the process (the plugin
 * library is never unloaded from memory, see function
 * weechat_js_v8_pin_library).
 *
 * V8 runs in single-threaded mode: WeeChat is single-threaded and forks
 * child processes (for example in hook_process), so no background thread
 * must be created by V8.
 */
Platform *js_platform = NULL;
ArrayBuffer::Allocator *js_allocator = NULL;
Isolate *js_isolate = NULL;
int js_isolate_entered = 0;
int js_exec_depth = 0;


/*
 * Prevent the plugin library from being unloaded from memory when the plugin
 * is unloaded, so that V8 data are kept and can be used again if the plugin
 * is loaded again.
 */

void
weechat_js_v8_pin_library (void)
{
    Dl_info info;

    if (dladdr ((void *)&weechat_js_v8_pin_library, &info) && info.dli_fname)
        (void) dlopen (info.dli_fname, RTLD_NOW | RTLD_NOLOAD | RTLD_NODELETE);
}

/*
 * Initialize V8 (on first call only) and enter the isolate.
 *
 * This is done on first load of a script, so that V8 is not initialized if
 * no javascript script is loaded.
 */

void
weechat_js_v8_init (void)
{
    Isolate::CreateParams create_params;

    if (js_isolate_entered)
        return;

    if (!js_isolate)
    {
        if (!js_platform)
        {
            weechat_js_v8_pin_library ();
            V8::SetFlagsFromString("--single-threaded");
            js_platform = platform::NewSingleThreadedDefaultPlatform().release();
            V8::InitializePlatform(js_platform);
            V8::Initialize();
        }
        js_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
        create_params.array_buffer_allocator = js_allocator;
        js_isolate = Isolate::New(create_params);
    }

    js_isolate->Enter();
    js_isolate_entered = 1;
}

/*
 * Exit the isolate.
 *
 * The isolate is not disposed so that it can be used again if the plugin is
 * loaded again.
 */

void
weechat_js_v8_end (void)
{
    if (js_isolate_entered)
    {
        js_isolate->Exit();
        js_isolate_entered = 0;
    }
}

/*
 * Run pending tasks posted by V8 on the platform (for example garbage
 * collection tasks).
 *
 * Tasks are run only when no javascript code is running (a javascript
 * function can call a WeeChat function which runs a callback of another
 * javascript script).
 */

void
weechat_js_v8_run_pending_tasks (void)
{
    if (js_exec_depth > 0)
        return;

    while (platform::PumpMessageLoop(js_platform, js_isolate))
    {
    }
}

/*
 * Constructor.
 */

WeechatJsV8::WeechatJsV8()
{
}

/*
 * Destructor.
 */

WeechatJsV8::~WeechatJsV8()
{
    this->context.Reset();
}

/*
 * Load a javascript script.
 */

bool
WeechatJsV8::load(const char *source)
{
    this->source = source;

    return true;
}

/*
 * Execute a javascript script.
 */

bool
WeechatJsV8::execScript()
{
    HandleScope handle_scope(js_isolate);
    TryCatch trycatch(js_isolate);
    Local<ObjectTemplate> global;
    Local<Context> ctx;
    Local<String> src;
    Local<Script> script;
    Local<Value> value;
    bool rc;

    global = ObjectTemplate::New(js_isolate);
    this->loadLibs(global);

    ctx = Context::New(js_isolate, NULL, global);
    this->context.Reset(js_isolate, ctx);
    Context::Scope context_scope(ctx);

    if (!String::NewFromUtf8(js_isolate, this->source.c_str()).ToLocal(&src))
        return false;

    if (!Script::Compile(ctx, src).ToLocal(&script))
    {
        PRINT_EXCEPTION;
        return false;
    }

    js_exec_depth++;
    rc = script->Run(ctx).ToLocal(&value);
    js_exec_depth--;

    if (!rc)
    {
        PRINT_EXCEPTION;
    }

    weechat_js_v8_run_pending_tasks ();

    return rc;
}

/*
 * Get the context of script.
 */

Local<Context>
WeechatJsV8::getContext(void)
{
    return this->context.Get(js_isolate);
}

/*
 * Check if a function with given name exists in script.
 */

bool
WeechatJsV8::functionExists(const char *function)
{
    HandleScope handle_scope(js_isolate);
    Local<Context> ctx;
    Local<String> name;
    Local<Value> value;

    ctx = this->getContext();
    Context::Scope context_scope(ctx);

    if (!String::NewFromUtf8(js_isolate, function).ToLocal(&name))
        return false;
    if (!ctx->Global()->Get(ctx, name).ToLocal(&value))
        return false;

    return value->IsFunction();
}

/*
 * Execute a function in a javascript script.
 *
 * The returned value is empty if an exception occurred.
 *
 * Note: the caller must create a handle scope and enter the context
 * of script.
 */

Local<Value>
WeechatJsV8::execFunction(const char *function, int argc, Local<Value> *argv)
{
    TryCatch trycatch(js_isolate);
    Local<Context> ctx;
    Local<Object> global;
    Local<String> name;
    Local<Value> value, res;

    ctx = this->getContext();
    global = ctx->Global();

    if (!String::NewFromUtf8(js_isolate, function).ToLocal(&name))
        return Local<Value>();
    if (!global->Get(ctx, name).ToLocal(&value) || !value->IsFunction())
        return Local<Value>();

    js_exec_depth++;
    if (!value.As<Function>()->Call(ctx, global, argc, argv).ToLocal(&res))
    {
        PRINT_EXCEPTION;
        res = Local<Value>();
    }
    js_exec_depth--;

    weechat_js_v8_run_pending_tasks ();

    return res;
}
