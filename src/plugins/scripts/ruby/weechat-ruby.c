/*
 * Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
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

/* weechat-ruby.c: Ruby plugin for WeeChat */


#undef _

#include <ruby.h>
#if defined(RUBY_VERSION) && RUBY_VERSION >=19
#include <ruby/encoding.h>
#endif

#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../../weechat-plugin.h"
#include "../script.h"
#include "weechat-ruby.h"
#include "weechat-ruby-api.h"

#ifndef StringValuePtr
#define StringValuePtr(s) STR2CSTR(s)
#endif
#ifndef RARRAY_LEN
#define RARRAY_LEN(s) RARRAY(s)->len
#endif
#ifndef RARRAY_PTR
#define RARRAY_PTR(s) RARRAY(s)->ptr
#endif
#ifndef RSTRING_LEN
#define RSTRING_LEN(s) RSTRING(s)->len
#endif
#ifndef RSTRING_PTR
#define RSTRING_PTR(s) RSTRING(s)->ptr
#endif


WEECHAT_PLUGIN_NAME(RUBY_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION("Ruby plugin for WeeChat");
WEECHAT_PLUGIN_AUTHOR("FlashCode <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE("GPL3");

struct t_weechat_plugin *weechat_ruby_plugin = NULL;

int ruby_quiet = 0;
struct t_plugin_script *ruby_scripts = NULL;
struct t_plugin_script *last_ruby_script = NULL;
struct t_plugin_script *ruby_current_script = NULL;
const char *ruby_current_script_filename = NULL;

/* string used to execute action "install":
   when signal "ruby_install_script" is received, name of string
   is added to this string, to be installed later by a timer (when nothing is
   running in script)
*/
char *ruby_action_install_list = NULL;

/* string used to execute action "remove":
   when signal "ruby_remove_script" is received, name of string
   is added to this string, to be removed later by a timer (when nothing is
   running in script)
*/
char *ruby_action_remove_list = NULL;

VALUE ruby_mWeechat, ruby_mWeechatOutputs;

#define MOD_NAME_PREFIX "WeechatRubyModule"
int ruby_num = 0;

char ruby_buffer_output[128];

typedef struct protect_call_arg {
    VALUE recv;
    ID mid;
    int argc;
    VALUE *argv;
} protect_call_arg_t;


/* 
 * protect_funcall0 : used to protect a function call
 */

static VALUE 
protect_funcall0 (VALUE arg)
{
    return rb_funcall2 (((protect_call_arg_t *)arg)->recv,
                        ((protect_call_arg_t *)arg)->mid,
                        ((protect_call_arg_t *)arg)->argc,
                        ((protect_call_arg_t *)arg)->argv);
}

/* 
 * rb_protect_funcall : function call in protect mode
 */

VALUE
rb_protect_funcall (VALUE recv, ID mid, int *state, int argc, ...)
{
    va_list ap;
    VALUE *argv;
    struct protect_call_arg arg;

    if (argc > 0)
    {
        int i;
        argv = ALLOCA_N(VALUE, argc);
        va_start(ap, argc);
        for (i = 0; i < argc; i++)
            argv[i] = va_arg(ap, VALUE);
        va_end(ap);
    }
    else
        argv = 0;
    arg.recv = recv;
    arg.mid = mid;
    arg.argc = argc;
    arg.argv = argv;
    return rb_protect(protect_funcall0, (VALUE) &arg, state);
}

/*
 *  weechat_ruby_print_exception: display ruby exception
 */

int
weechat_ruby_print_exception (VALUE err)
{
    VALUE backtrace;
    int i;
    int ruby_error;
    char* line;
    char* cline;
    char* err_msg;
    char* err_class;
    
    backtrace = rb_protect_funcall (err, rb_intern("backtrace"),
                                    &ruby_error, 0);
    err_msg = STR2CSTR(rb_protect_funcall(err, rb_intern("message"),
                                          &ruby_error, 0));
    err_class = STR2CSTR(rb_protect_funcall(rb_protect_funcall(err,
                                                               rb_intern("class"),
                                                               &ruby_error, 0),
                                            rb_intern("name"), &ruby_error, 0));
    
    if (strcmp (err_class, "SyntaxError") == 0)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error: %s"),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME,
                        STR2CSTR(rb_inspect(err)));
    }
    else
    {
        for (i = 0; i < RARRAY_LEN(backtrace); i++)
        {
            line = STR2CSTR(RARRAY_PTR(backtrace)[i]);
            cline = NULL;
            if (i == 0)
            {
                cline = (char *)calloc (strlen (line) + 2 + strlen (err_msg) +
                                        3 + strlen (err_class) + 1,
                                        sizeof (char));
                if (cline)
                {
                    strcat (cline, line);
                    strcat (cline, ": ");
                    strcat (cline, err_msg);
                    strcat (cline, " (");
                    strcat (cline, err_class);
                    strcat (cline, ")");
                }
            }
            else
            {
                cline = (char *)calloc(strlen (line) + strlen ("     from ") + 1,
                                       sizeof (char));
                if (cline)
                {
                    strcat (cline, "     from ");
                    strcat (cline, line);
                }
            }
            if (cline)
            {
                weechat_printf (NULL,
                                weechat_gettext ("%s%s: error: %s"),
                                weechat_prefix ("error"), RUBY_PLUGIN_NAME,
                                cline);
            }
            
            if (cline)
                free (cline);
        }
    }
    
    return 0;
}

/*
 * weechat_ruby_exec: call a ruby command
 */

void *
weechat_ruby_exec (struct t_plugin_script *script,
                   int ret_type, const char *function, char **argv)
{
    VALUE rc, err;
    int ruby_error, *ret_i;
    void *ret_value;
    struct t_plugin_script *old_ruby_current_script;

    old_ruby_current_script = ruby_current_script;
    ruby_current_script = script;
    
    if (argv && argv[0])
    {
        if (argv[1])
        {
            if (argv[2])
            {
                if (argv[3])
                {
                    if (argv[4])
                    {
                        if (argv[5])
                        {
                            if (argv[6])
                            {
                                if (argv[7])
                                {
                                    rc = rb_protect_funcall ((VALUE) script->interpreter, rb_intern(function),
                                                             &ruby_error, 8,
                                                             rb_str_new2(argv[0]),
                                                             rb_str_new2(argv[1]),
                                                             rb_str_new2(argv[2]),
                                                             rb_str_new2(argv[3]),
                                                             rb_str_new2(argv[4]),
                                                             rb_str_new2(argv[5]),
                                                             rb_str_new2(argv[6]),
                                                             rb_str_new2(argv[7]));
                                }
                                else
                                {
                                    rc = rb_protect_funcall ((VALUE) script->interpreter, rb_intern(function),
                                                             &ruby_error, 7,
                                                             rb_str_new2(argv[0]),
                                                             rb_str_new2(argv[1]),
                                                             rb_str_new2(argv[2]),
                                                             rb_str_new2(argv[3]),
                                                             rb_str_new2(argv[4]),
                                                             rb_str_new2(argv[5]),
                                                             rb_str_new2(argv[6]));
                                }
                            }
                            else
                            {
                                rc = rb_protect_funcall ((VALUE) script->interpreter, rb_intern(function),
                                                         &ruby_error, 6,
                                                         rb_str_new2(argv[0]),
                                                         rb_str_new2(argv[1]),
                                                         rb_str_new2(argv[2]),
                                                         rb_str_new2(argv[3]),
                                                         rb_str_new2(argv[4]),
                                                         rb_str_new2(argv[5]));
                            }
                        }
                        else
                        {
                            rc = rb_protect_funcall ((VALUE) script->interpreter, rb_intern(function),
                                                     &ruby_error, 5,
                                                     rb_str_new2(argv[0]),
                                                     rb_str_new2(argv[1]),
                                                     rb_str_new2(argv[2]),
                                                     rb_str_new2(argv[3]),
                                                     rb_str_new2(argv[4]));
                        }
                    }
                    else
                    {
                        rc = rb_protect_funcall ((VALUE) script->interpreter, rb_intern(function),
                                                 &ruby_error, 4,
                                                 rb_str_new2(argv[0]),
                                                 rb_str_new2(argv[1]),
                                                 rb_str_new2(argv[2]),
                                                 rb_str_new2(argv[3]));
                    }
                }
                else
                {
                    rc = rb_protect_funcall ((VALUE) script->interpreter, rb_intern(function),
                                             &ruby_error, 3,
                                             rb_str_new2(argv[0]),
                                             rb_str_new2(argv[1]),
                                             rb_str_new2(argv[2]));
                }
            }
            else
            {
                rc = rb_protect_funcall ((VALUE) script->interpreter, rb_intern(function),
                                         &ruby_error, 2,
                                         rb_str_new2(argv[0]),
                                         rb_str_new2(argv[1]));
            }
        }
        else
        {
            rc = rb_protect_funcall ((VALUE) script->interpreter, rb_intern(function),
                                     &ruby_error, 1,
                                     rb_str_new2(argv[0]));
        }
    }
    else
    {
        rc = rb_protect_funcall ((VALUE) script->interpreter, rb_intern(function),
                                 &ruby_error, 0);
    }
    
    if (ruby_error)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to run function \"%s\""),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME, function);
        
        err = rb_gv_get("$!");
        weechat_ruby_print_exception(err);
        
        return NULL;
    }
    
    if ((TYPE(rc) == T_STRING) && (ret_type == WEECHAT_SCRIPT_EXEC_STRING))
    {
        if (STR2CSTR (rc))
            ret_value = strdup (STR2CSTR (rc));
        else
            ret_value = NULL;
    }
    else if ((TYPE(rc) == T_FIXNUM) && (ret_type == WEECHAT_SCRIPT_EXEC_INT))
    {
        ret_i = malloc (sizeof (*ret_i));
        if (ret_i)
            *ret_i = NUM2INT(rc);
        ret_value = ret_i;
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"%s\" must return a "
                                         "valid value"),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME, function);
        ruby_current_script = old_ruby_current_script;
        return WEECHAT_RC_OK;
    }
    
    if (ret_value == NULL)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: not enough memory in function "
                                         "\"%s\""),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME, function);
        ruby_current_script = old_ruby_current_script;
        return NULL;
    }
    
    ruby_current_script = old_ruby_current_script;
    
    return ret_value;
}

/*
 * weechat_ruby_output: redirection for stdout and stderr
 */

static VALUE 
weechat_ruby_output (VALUE self, VALUE str)
{
    char *msg, *p, *m;
    
    /* make C compiler happy */
    (void) self;
    
    msg = strdup(STR2CSTR(str));
    
    m = msg;
    while ((p = strchr (m, '\n')) != NULL)
    {
        *p = '\0';
        if (strlen (m) + strlen (ruby_buffer_output) > 0)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: stdout/stderr: %s%s"),
                            weechat_prefix ("error"), RUBY_PLUGIN_NAME,
                            ruby_buffer_output, m);
        }
        *p = '\n';
        ruby_buffer_output[0] = '\0';
        m = ++p;
    }
    
    if (strlen(m) + strlen(ruby_buffer_output) > sizeof(ruby_buffer_output))
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: stdout/stderr: %s%s"),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME,
                        ruby_buffer_output, m);
        ruby_buffer_output[0] = '\0';
    }
    else
        strcat (ruby_buffer_output, m);

    if (msg)
        free (msg);
    
    return Qnil;
}

/*
 * weechat_ruby_output_flush: just for compatibility
 */

static VALUE 
weechat_ruby_output_flush (VALUE self)
{
    /* make C compiler happy */
    (void) self;
    
    return Qnil;
}


/*
 * weechat_ruby_load: load a Ruby script
 */

int
weechat_ruby_load (const char *filename)
{
    char modname[64];
    VALUE curModule, ruby_retcode, err;
    int ruby_error;
    struct stat buf;
    
    if (stat (filename, &buf) != 0)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not found"),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME, filename);
        return 0;
    }
    
    if ((weechat_ruby_plugin->debug >= 1) || !ruby_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: loading script \"%s\""),
                        RUBY_PLUGIN_NAME, filename);
    }
    
    ruby_current_script = NULL;
    
    snprintf (modname, sizeof(modname), "%s%d", MOD_NAME_PREFIX, ruby_num);
    ruby_num++;

    curModule = rb_define_module(modname);

    ruby_current_script_filename = filename;
    
    ruby_retcode = rb_protect_funcall (curModule, rb_intern("load_eval_file"),
                                       &ruby_error, 1, rb_str_new2(filename));
    
    if (ruby_retcode == Qnil)
    {
        err = rb_gv_get("$!");
        weechat_ruby_print_exception(err);
        return 0;
    }
    
    if (NUM2INT(ruby_retcode) != 0)
    {
        switch (NUM2INT(ruby_retcode))
        {
            case 1:
                weechat_printf (NULL,
                                weechat_gettext ("%s%s: unable to read file "
                                                 "\"%s\""),
                                weechat_prefix ("error"), RUBY_PLUGIN_NAME,
                                filename);
                break;
            case 2:
                weechat_printf (NULL,
                                weechat_gettext ("%s%s: error while loading "
                                                 "file \"%s\""),
                                weechat_prefix ("error"), RUBY_PLUGIN_NAME,
                                filename);
                break;
            case 3:
                weechat_printf (NULL,
                                weechat_gettext ("%s%s: function "
                                                 "\"weechat_init\" is missing "
                                                 "in file \"%s\""),
                                weechat_prefix ("error"), RUBY_PLUGIN_NAME,
                                filename);
                break;
        }

        if (NUM2INT(ruby_retcode) == 1 || NUM2INT(ruby_retcode) == 2)
        {
            weechat_ruby_print_exception(rb_iv_get(curModule, "@load_eval_file_error"));
        }
        
        return 0;
    }
    
    ruby_retcode = rb_protect_funcall (curModule, rb_intern("weechat_init"),
                                       &ruby_error, 0);
    
    if (ruby_error)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to eval function "
                                         "\"weechat_init\" in file \"%s\""),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME, filename);
        
        err = rb_gv_get("$!");
        weechat_ruby_print_exception(err);

        if (ruby_current_script != NULL)
        {
            script_remove (weechat_ruby_plugin,
                           &ruby_scripts, &last_ruby_script,
                           ruby_current_script);
        }

        return 0;
    }
    
    if (ruby_current_script == NULL)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME, filename);
        return 0;
    }
    
    ruby_current_script->interpreter = (VALUE *) curModule;
    rb_gc_register_address (ruby_current_script->interpreter);
    
    return 1;
}

/*
 * weechat_ruby_load_cb: callback for weechat_script_auto_load() function
 */

void
weechat_ruby_load_cb (void *data, const char *filename)
{
    /* make C compiler happy */
    (void) data;
    
    weechat_ruby_load (filename);
}

/*
 * weechat_ruby_unload: unload a Ruby script
 */

void
weechat_ruby_unload (struct t_plugin_script *script)
{
    int *r;
    char *ruby_argv[1] = { NULL };
    void *interpreter;
    
    weechat_printf (NULL,
                    weechat_gettext ("%s: unloading script \"%s\""),
                    RUBY_PLUGIN_NAME, script->name);
    
    if (script->shutdown_func && script->shutdown_func[0])
    {
        r = (int *) weechat_ruby_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script->shutdown_func,
                                       ruby_argv);
        if (r)
            free (r);
    }
    
    interpreter = script->interpreter;
    
    if (ruby_current_script == script)
        ruby_current_script = (ruby_current_script->prev_script) ?
            ruby_current_script->prev_script : ruby_current_script->next_script;
    
    script_remove (weechat_ruby_plugin, &ruby_scripts, &last_ruby_script,
                   script);
    
    if (interpreter)
        rb_gc_unregister_address (interpreter);
}

/*
 * weechat_ruby_unload_name: unload a Ruby script by name
 */

void
weechat_ruby_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    
    ptr_script = script_search (weechat_ruby_plugin, ruby_scripts, name);
    if (ptr_script)
    {
        weechat_ruby_unload (ptr_script);
        weechat_printf (NULL,
                        weechat_gettext ("%s: script \"%s\" unloaded"),
                        RUBY_PLUGIN_NAME, name);
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME, name);
    }
}

/*
 * weechat_ruby_reload_name: reload a Ruby script by name
 */

void
weechat_ruby_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;
    
    ptr_script = script_search (weechat_ruby_plugin, ruby_scripts, name);
    if (ptr_script)
    {
        filename = strdup (ptr_script->filename);
        if (filename)
        {
            weechat_ruby_unload (ptr_script);
            weechat_printf (NULL,
                            weechat_gettext ("%s: script \"%s\" unloaded"),
                            RUBY_PLUGIN_NAME, name);
            weechat_ruby_load (filename);
            free (filename);
        }
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME, name);
    }
}

/*
 * weechat_ruby_unload_all: unload all Ruby scripts
 */

void
weechat_ruby_unload_all ()
{
    while (ruby_scripts)
    {
        weechat_ruby_unload (ruby_scripts);
    }
}

/*
 * weechat_ruby_command_cb: callback for "/ruby" command
 */

int
weechat_ruby_command_cb (void *data, struct t_gui_buffer *buffer,
                         int argc, char **argv, char **argv_eol)
{
    char *path_script;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    
    if (argc == 1)
    {
        script_display_list (weechat_ruby_plugin, ruby_scripts,
                             NULL, 0);
    }
    else if (argc == 2)
    {
        if (weechat_strcasecmp (argv[1], "list") == 0)
        {
            script_display_list (weechat_ruby_plugin, ruby_scripts,
                                 NULL, 0);
        }
        else if (weechat_strcasecmp (argv[1], "listfull") == 0)
        {
            script_display_list (weechat_ruby_plugin, ruby_scripts,
                                 NULL, 1);
        }
        else if (weechat_strcasecmp (argv[1], "autoload") == 0)
        {
            script_auto_load (weechat_ruby_plugin, &weechat_ruby_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "reload") == 0)
        {
            weechat_ruby_unload_all ();
            script_auto_load (weechat_ruby_plugin, &weechat_ruby_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "unload") == 0)
        {
            weechat_ruby_unload_all ();
        }
    }
    else
    {
        if (weechat_strcasecmp (argv[1], "list") == 0)
        {
            script_display_list (weechat_ruby_plugin, ruby_scripts,
                                 argv_eol[2], 0);
        }
        else if (weechat_strcasecmp (argv[1], "listfull") == 0)
        {
            script_display_list (weechat_ruby_plugin, ruby_scripts,
                                 argv_eol[2], 1);
        }
        else if (weechat_strcasecmp (argv[1], "load") == 0)
        {
            /* load Ruby script */
            path_script = script_search_path (weechat_ruby_plugin,
                                              argv_eol[2]);
            weechat_ruby_load ((path_script) ? path_script : argv_eol[2]);
            if (path_script)
                free (path_script);
        }
        else if (weechat_strcasecmp (argv[1], "reload") == 0)
        {
            /* reload one Ruby script */
            weechat_ruby_reload_name (argv_eol[2]);
        }
        else if (weechat_strcasecmp (argv[1], "unload") == 0)
        {
            /* unload Ruby script */
            weechat_ruby_unload_name (argv_eol[2]);
        }
        else
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unknown option for "
                                             "command \"%s\""),
                            weechat_prefix ("error"), RUBY_PLUGIN_NAME, "ruby");
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_ruby_completion_cb: callback for script completion
 */

int
weechat_ruby_completion_cb (void *data, const char *completion_item,
                            struct t_gui_buffer *buffer,
                            struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    script_completion (weechat_ruby_plugin, completion, ruby_scripts);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_ruby_infolist_cb: callback for infolist
 */

struct t_infolist *
weechat_ruby_infolist_cb (void *data, const char *infolist_name,
                          void *pointer, const char *arguments)
{
    /* make C compiler happy */
    (void) data;
    
    if (!infolist_name || !infolist_name[0])
        return NULL;
    
    if (weechat_strcasecmp (infolist_name, "ruby_script") == 0)
    {
        return script_infolist_list_scripts (weechat_ruby_plugin,
                                             ruby_scripts, pointer,
                                             arguments);
    }
    
    return NULL;
}

/*
 * weechat_ruby_signal_debug_dump_cb: dump Ruby plugin data in WeeChat log file
 */

int
weechat_ruby_signal_debug_dump_cb (void *data, const char *signal,
                                   const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;
    
    script_print_log (weechat_ruby_plugin, ruby_scripts);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_ruby_signal_buffer_closed_cb: callback called when a buffer is closed
 */

int
weechat_ruby_signal_buffer_closed_cb (void *data, const char *signal,
                                      const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    
    if (signal_data)
        script_remove_buffer_callbacks (ruby_scripts, signal_data);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_ruby_timer_action_cb: timer for executing actions
 */

int
weechat_ruby_timer_action_cb (void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) remaining_calls;

    if (data)
    {
        if (data == &ruby_action_install_list)
        {
            script_action_install (weechat_ruby_plugin,
                                   ruby_scripts,
                                   &weechat_ruby_unload,
                                   &weechat_ruby_load,
                                   &ruby_action_install_list);
        }
        else if (data == &ruby_action_remove_list)
        {
            script_action_remove (weechat_ruby_plugin,
                                  ruby_scripts,
                                  &weechat_ruby_unload,
                                  &ruby_action_remove_list);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_ruby_signal_script_action_cb: callback called when a script action
 *                                       is asked (install/remove a script)
 */

int
weechat_ruby_signal_script_action_cb (void *data, const char *signal,
                                      const char *type_data,
                                      void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "ruby_script_install") == 0)
        {
            script_action_add (&ruby_action_install_list,
                               (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_ruby_timer_action_cb,
                                &ruby_action_install_list);
        }
        else if (strcmp (signal, "ruby_script_remove") == 0)
        {
            script_action_add (&ruby_action_remove_list,
                               (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_ruby_timer_action_cb,
                                &ruby_action_remove_list);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_init: initialize Ruby plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    int ruby_error;
    char *weechat_ruby_code =
        {
            "$stdout = WeechatOutputs\n"
            "$stderr = WeechatOutputs\n"
            "begin"
            "  if RUBY_VERSION.split('.')[1] == '9'\n"
            "    require 'enc/encdb.so'\n"
            "    require 'enc/trans/transdb.so'\n"
            "\n"
            "    require 'thread'\n"
            "    class ::Mutex\n"
            "      def synchronize(*args)\n"
            "        yield\n"
            "      end\n"
            "    end\n"
            "    require 'rubygems'\n"
            "    $LOAD_PATH.concat Gem.latest_load_paths\n"
            "  else\n"
            "    require 'rubygems'\n"
            "  end\n"
            "rescue LoadError\n"
            "end\n"
            "\n"
            "class Module\n"
            "\n"
            "  def load_eval_file (file)\n"
            "    lines = ''\n"
            "    begin\n"
            "      lines = File.read(file)\n"
            "    rescue => e\n"
            "      return 1\n"
            "    end\n"
            "\n"
            "    begin\n"
            "      module_eval(lines)\n"
            "    rescue Exception => e\n"
            "      @load_eval_file_error = e\n"
            "      return 2\n"
            "    end\n"
            "\n"
            "    has_init = false\n"
            "\n"
            "    instance_methods.each do |meth|\n"
            "      if meth.to_s == 'weechat_init'\n"
            "        has_init = true\n"
            "      end\n"
            "      module_eval('module_function :' + meth)\n"
            "    end\n"
            "\n"
            "    unless has_init\n"
            "      return 3\n"
            "    end\n"
            "\n"
            "    return 0\n"
            "  end\n"
            "end\n"
        };
    
    weechat_ruby_plugin = plugin;
    
    ruby_error = 0;
    
    /* init stdout/stderr buffer */
    ruby_buffer_output[0] = '\0';
    
#if defined(RUBY_VERSION) && RUBY_VERSION >= 19
    RUBY_INIT_STACK;
#endif
    
    ruby_init ();
    ruby_init_loadpath ();
    ruby_script ("__weechat_plugin__");
    
    ruby_mWeechat = rb_define_module("Weechat");
    weechat_ruby_api_init (ruby_mWeechat);
    
    /* redirect stdin and stdout */
    ruby_mWeechatOutputs = rb_define_module("WeechatOutputs");
    rb_define_singleton_method(ruby_mWeechatOutputs, "write",
                               weechat_ruby_output, 1);
    rb_define_singleton_method(ruby_mWeechatOutputs, "puts",
                               weechat_ruby_output, 1);
    rb_define_singleton_method(ruby_mWeechatOutputs, "p",
                               weechat_ruby_output, 1);
    rb_define_singleton_method(ruby_mWeechatOutputs, "flush",
                               weechat_ruby_output_flush, 0);

    rb_eval_string_protect(weechat_ruby_code, &ruby_error);
    if (ruby_error)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to eval WeeChat ruby "
                                         "internal code"),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME);
        VALUE err = rb_gv_get("$!");
        weechat_ruby_print_exception(err);
        return WEECHAT_RC_ERROR;
    }
    
    ruby_quiet = 1;
    script_init (weechat_ruby_plugin,
                 argc,
                 argv,
                 &ruby_scripts,
                 &weechat_ruby_command_cb,
                 &weechat_ruby_completion_cb,
                 &weechat_ruby_infolist_cb,
                 &weechat_ruby_signal_debug_dump_cb,
                 &weechat_ruby_signal_buffer_closed_cb,
                 &weechat_ruby_signal_script_action_cb,
                 &weechat_ruby_load_cb,
                 &weechat_ruby_api_buffer_input_data_cb,
                 &weechat_ruby_api_buffer_close_cb);
    ruby_quiet = 0;
    
    script_display_short_list (weechat_ruby_plugin,
                               ruby_scripts);
    
    /* init ok */
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: shutdown Ruby interface
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;
    
    /* unload all scripts */
    weechat_ruby_unload_all ();
    
    ruby_cleanup (0);
    
    return WEECHAT_RC_OK;
}
