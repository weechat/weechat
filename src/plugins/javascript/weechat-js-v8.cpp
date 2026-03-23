/*
 * SPDX-FileCopyrightText: 2013 Koka El Kiwi <kokakiwi@kokakiwi.net>
 * SPDX-FileCopyrightText: 2015-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

/* V8 JavaScript functions */

#undef _
#include <cstdio>

extern "C"
{
#include "../weechat-plugin.h"
#include "../plugin-script.h"
}

#include "weechat-js.h"
#include "weechat-js-v8.h"

#define PRINT_EXCEPTION                                                 \
    Local<Value> exception = trycatch.Exception();                      \
    String::Utf8Value str_exception(exception);                         \
    weechat_printf (NULL,                                               \
                    weechat_gettext ("%s%s: exception: %s"),            \
                    weechat_prefix ("error"), JS_PLUGIN_NAME,           \
                    *str_exception);

using namespace v8;


/*
 * Constructor.
 */

WeechatJsV8::WeechatJsV8()
{
    this->global = ObjectTemplate::New();
}

/*
 * Destructor.
 */

WeechatJsV8::~WeechatJsV8()
{
    this->context.Dispose();
}

/*
 * Load a javascript script.
 */

bool
WeechatJsV8::load(Handle<String> source)
{
    this->source = source;

    return true;
}

/*
 * Load a javascript script.
 */

bool
WeechatJsV8::load(const char *source)
{
    Handle<String> src = String::New(source);

    return this->load(src);
}

/*
 * Execute a javascript script.
 */

bool
WeechatJsV8::execScript()
{
    v8::TryCatch trycatch;

    this->context = Context::New(NULL, this->global);
    Context::Scope context_scope(this->context);
    Handle<Script> script = Script::Compile(this->source);

    if (script.IsEmpty())
    {
        PRINT_EXCEPTION;
        return false;
    }
    else
    {
        Local<Value> value = script->Run();
        if (value.IsEmpty())
        {
            PRINT_EXCEPTION;
            return false;
        }
    }

    return true;
}

/*
 * Check if a function with given name exists in script.
 */

bool
WeechatJsV8::functionExists(const char *function)
{
    Context::Scope context_scope(this->context);

    Handle<Object> global = this->context->Global();
    Handle<Value> value = global->Get(String::New(function));
    return value->IsFunction();
}

/*
 * Execute a function in a javascript script.
 */

Handle<Value>
WeechatJsV8::execFunction(const char *function, int argc, Handle<Value> *argv)
{
    v8::TryCatch trycatch;

    Context::Scope context_scope(this->context);

    Handle<Object> global = this->context->Global();
    Handle<Value> value = global->Get(String::New(function));
    Handle<Function> func = Handle<Function>::Cast(value);

    Handle<Value> res = func->Call(global, argc, argv);
    if (res.IsEmpty())
    {
        PRINT_EXCEPTION;
    }
    return res;
}

/*
 * Set something in the global.
 */

void
WeechatJsV8::addGlobal(Handle<String> key, Handle<Template> val)
{
    this->global->Set(key, val);
}

/*
 * Set something in the global.
 */

void
WeechatJsV8::addGlobal(const char *key, Handle<Template> val)
{
    this->addGlobal(String::New(key), val);
}
