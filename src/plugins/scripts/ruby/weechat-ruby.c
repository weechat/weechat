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

/* weechat-ruby.c: Ruby plugin for WeeChat */

#undef _

#include <ruby.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../../weechat-plugin.h"
#include "../script.h"
#include "weechat-ruby.h"
#include "weechat-ruby-api.h"


WEECHAT_PLUGIN_NAME("ruby");
WEECHAT_PLUGIN_DESCRIPTION("Ruby plugin for WeeChat");
WEECHAT_PLUGIN_AUTHOR("FlashCode <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION("0.1");
WEECHAT_PLUGIN_LICENSE("GPL");

struct t_weechat_plugin *weechat_ruby_plugin = NULL;

struct t_plugin_script *ruby_scripts = NULL;
struct t_plugin_script *ruby_current_script = NULL;
char *ruby_current_script_filename = NULL;

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
protect_funcall0(VALUE arg)
{
    return rb_funcall2(((protect_call_arg_t *) arg)->recv,
                       ((protect_call_arg_t *) arg)->mid,
                       ((protect_call_arg_t *) arg)->argc,
                       ((protect_call_arg_t *) arg)->argv);
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
 * weechat_ruby_exec: execute a Ruby script
 */

void *
weechat_ruby_exec (struct t_plugin_script *script,
		   int ret_type, char *function, char **argv)
{
    VALUE rc, err;
    int ruby_error, *ret_i;
    void *ret_value;
    
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
                            rc = rb_protect_funcall ((VALUE) script->interpreter, rb_intern(function),
                                                     &ruby_error, 6,
                                                     rb_str_new2(argv[0]),
                                                     rb_str_new2(argv[1]),
                                                     rb_str_new2(argv[2]),
                                                     rb_str_new2(argv[3]),
                                                     rb_str_new2(argv[4]),
                                                     rb_str_new2(argv[5]));
                        }
                        else
                            rc = rb_protect_funcall ((VALUE) script->interpreter, rb_intern(function),
                                                     &ruby_error, 5,
                                                     rb_str_new2(argv[0]),
                                                     rb_str_new2(argv[1]),
                                                     rb_str_new2(argv[2]),
                                                     rb_str_new2(argv[3]),
                                                     rb_str_new2(argv[4]));
                    }
                    else
                        rc = rb_protect_funcall ((VALUE) script->interpreter, rb_intern(function),
                                                 &ruby_error, 4,
                                                 rb_str_new2(argv[0]),
                                                 rb_str_new2(argv[1]),
                                                 rb_str_new2(argv[2]),
                                                 rb_str_new2(argv[3]));
                }
                else
                    rc = rb_protect_funcall ((VALUE) script->interpreter, rb_intern(function),
                                             &ruby_error, 3,
                                             rb_str_new2(argv[0]),
                                             rb_str_new2(argv[1]),
                                             rb_str_new2(argv[2]));
            }
            else
                rc = rb_protect_funcall ((VALUE) script->interpreter, rb_intern(function),
                                         &ruby_error, 2,
                                         rb_str_new2(argv[0]),
                                         rb_str_new2(argv[1]));
        }
        else
            rc = rb_protect_funcall ((VALUE) script->interpreter, rb_intern(function),
                                     &ruby_error, 1,
                                     rb_str_new2(argv[0]));
    }
    else
        rc = rb_protect_funcall ((VALUE) script->interpreter, rb_intern(function),
                                 &ruby_error, 0);
    
    if (ruby_error)
    {
	weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to run function \"%s\""),
                        weechat_prefix ("error"), "ruby", function);
	
	err = rb_inspect(rb_gv_get("$!"));
	weechat_printf (NULL,
                        weechat_gettext ("%s%s: error: \"%s\""),
                        weechat_prefix ("error"), "ruby", STR2CSTR(err));
        
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
	ret_i = (int *)malloc (sizeof (int));
	if (ret_i)
	    *ret_i = NUM2INT(rc);
	ret_value = ret_i;
    }
    else
    {
	weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"%s\" must return a "
                                         "valid value"),
                        weechat_prefix ("error"), "ruby", function);
	return WEECHAT_RC_OK;
    }
    
    if (ret_value == NULL)
    {
	weechat_printf (NULL,
                        weechat_gettext ("%s%s: not enough memory in function "
                                         "\"%s\""),
                        weechat_prefix ("error"), "ruby", function);
	return NULL;
    }
    
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
                            weechat_prefix ("error"), "ruby",
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
                        weechat_prefix ("error"), "ruby",
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
weechat_ruby_load (char *filename)
{
    char modname[64];
    VALUE curModule, ruby_retcode, err;
    int ruby_error;
    struct stat buf;
    
    if (stat (filename, &buf) != 0)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not found"),
                        weechat_prefix ("error"), "ruby", filename);
        return 0;
    }
    
    weechat_printf (NULL,
                    weechat_gettext ("%s%s: loading script \"%s\""),
                    weechat_prefix ("info"), "ruby", filename);
    
    ruby_current_script = NULL;
    
    snprintf (modname, sizeof(modname), "%s%d", MOD_NAME_PREFIX, ruby_num);
    ruby_num++;

    curModule = rb_define_module(modname);

    ruby_current_script_filename = filename;
    
    ruby_retcode = rb_protect_funcall (curModule, rb_intern("load_eval_file"),
				       &ruby_error, 1, rb_str_new2(filename));
    
    if (ruby_retcode == Qnil)
    {
	err = rb_inspect(rb_gv_get("$!"));
	weechat_printf (NULL,
                        weechat_gettext ("%s%s: error: \"%s\""),
                        weechat_prefix ("error"), "ruby", STR2CSTR(err));
	return 0;
    }
    
    if (NUM2INT(ruby_retcode) != 0)
    {
	VALUE ruby_eval_error;
	
	switch (NUM2INT(ruby_retcode))
	{
            case 1:
                weechat_printf (NULL,
                                weechat_gettext ("%s%s: unable to read file "
                                                 "\"%s\""),
                                weechat_prefix ("error"), "ruby", filename);
                break;
            case 2:
                weechat_printf (NULL,
                                weechat_gettext ("%s%s: error while loading "
                                                 "file \"%s\""),
                                weechat_prefix ("error"), "ruby", filename);
                break;
            case 3:
                weechat_printf (NULL,
                                weechat_gettext ("%s%s: function "
                                                 "\"weechat_init\" is missing "
                                                 "in file \"%s\""),
                                weechat_prefix ("error"), "ruby", filename);
                break;
	}
	
	if (NUM2INT(ruby_retcode) == 1 || NUM2INT(ruby_retcode) == 2)
	{
	    ruby_eval_error = rb_iv_get(curModule, "@load_eval_file_error");
	    if (ruby_eval_error)
            {
		weechat_printf (NULL,
                                weechat_gettext ("%s%s: error: %s"),
                                weechat_prefix ("error"), "ruby",
                                STR2CSTR(ruby_eval_error));
            }
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
                        weechat_prefix ("error"), "ruby", filename);
	
	err = rb_inspect(rb_gv_get("$!"));
	weechat_printf (NULL,
                        weechat_gettext ("%s%s: error: \"%s\""),
                        weechat_prefix ("error"), "ruby", STR2CSTR(err));
	
	if (ruby_current_script != NULL)
        {
	    script_remove (weechat_ruby_plugin, &ruby_scripts,
                           ruby_current_script);
        }
        
	return 0;
    }
    
    if (ruby_current_script == NULL)
    {
	weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        weechat_prefix ("error"), "ruby", filename);
        return 0;
    }
    
    ruby_current_script->interpreter = (VALUE *) curModule;
    rb_gc_register_address (ruby_current_script->interpreter);
    
    return 1;
}

/*
 * weechat_ruby_load_cb: callback for weechat_script_auto_load() function
 */

int
weechat_ruby_load_cb (void *data, char *filename)
{
    /* make C compiler happy */
    (void) data;
    
    return weechat_ruby_load (filename);
}

/*
 * weechat_ruby_unload: unload a Ruby script
 */

void
weechat_ruby_unload (struct t_plugin_script *script)
{
    int *r;
    char *ruby_argv[1] = { NULL };

    weechat_printf (NULL,
                    weechat_gettext ("%s%s: unloading script \"%s\""),
                    weechat_prefix ("info"), "ruby", script->name);
    
    if (script->shutdown_func && script->shutdown_func[0])
    {
        r = (int *) weechat_ruby_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
				       script->shutdown_func,
                                       ruby_argv);
	if (r)
	    free (r);
    }
    
    if (script->interpreter)
	rb_gc_unregister_address (script->interpreter);
    
    script_remove (weechat_ruby_plugin, &ruby_scripts, script);
}

/*
 * weechat_ruby_unload_name: unload a Ruby script by name
 */

void
weechat_ruby_unload_name (char *name)
{
    struct t_plugin_script *ptr_script;
    
    ptr_script = script_search (weechat_ruby_plugin, ruby_scripts, name);
    if (ptr_script)
    {
        weechat_ruby_unload (ptr_script);
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" unloaded"),
                        weechat_prefix ("info"), "ruby", name);
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), "ruby", name);
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
            path_script = script_search_full_name (weechat_ruby_plugin,
                                                   argv_eol[2]);
            weechat_ruby_load ((path_script) ? path_script : argv_eol[2]);
            if (path_script)
                free (path_script);
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
                            weechat_prefix ("error"), "ruby", "ruby");
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_ruby_completion_cb: callback for script completion
 */

int
weechat_ruby_completion_cb (void *data, char *completion,
                            struct t_gui_buffer *buffer,
                            struct t_weelist *list)
{
    /* make C compiler happy */
    (void) data;
    (void) completion;
    (void) buffer;
    
    script_completion (weechat_ruby_plugin, list, ruby_scripts);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_ruby_debug_dump_cb: dump Ruby plugin data in WeeChat log file
 */

int
weechat_ruby_debug_dump_cb (void *data, char *signal, char *type_data,
                            void *signal_data)
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
 * weechat_plugin_init: initialize Ruby plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin)
{
    int ruby_error;
    char *weechat_ruby_code =
        {
            "$stdout = WeechatOutputs\n"
            "$stderr = WeechatOutputs\n"
            "\n"
            "class Module\n"
            "  @load_eval_file_error = ''\n"
            "\n"
            "  def load_eval_file (file)\n"
            "    lines = ''\n"
	    "    begin\n"
	    "      f = File.open(file, 'r')\n"
	    "      lines = f.readlines.join\n"
	    "    rescue => e\n"
	    "      @load_eval_file_error = e\n"
	    "      return 1\n"
	    "    end\n"
	    "\n"	   
	    "    begin\n"
	    "      module_eval(lines)\n"
	    "    rescue => e\n"
	    "      @load_eval_file_error = e\n"
	    "      return 2\n"
	    "    end\n"
	    "\n"
	    "    has_init = false\n"
	    "\n"
	    "    instance_methods.each do |meth|\n"
	    "      if meth == 'weechat_init'\n"
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
    
    ruby_init ();
    ruby_init_loadpath ();
    ruby_script ("__weechat_plugin__");
    
    ruby_mWeechat = rb_define_module("Weechat");
    weechat_ruby_api_init (ruby_mWeechat);
    
    /* redirect stdin and stdout */
    ruby_mWeechatOutputs = rb_define_module("WeechatOutputs");
    rb_define_singleton_method(ruby_mWeechatOutputs, "write", weechat_ruby_output, 1);
    rb_define_singleton_method(ruby_mWeechatOutputs, "puts", weechat_ruby_output, 1);
    rb_define_singleton_method(ruby_mWeechatOutputs, "p", weechat_ruby_output, 1);
    rb_define_singleton_method(ruby_mWeechatOutputs, "flush", weechat_ruby_output_flush, 0);

    rb_eval_string_protect(weechat_ruby_code, &ruby_error);
    if (ruby_error)
    {
	VALUE ruby_error_info = rb_inspect(ruby_errinfo);
	weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to eval WeeChat ruby "
                                         "internal code"),
                        weechat_prefix ("error"), "ruby");
	weechat_printf (NULL,
                        weechat_gettext ("%s%s: error: %s"),
                        weechat_prefix ("error"), "ruby",
                        STR2CSTR(ruby_error_info));
	return WEECHAT_RC_ERROR;
    }
    
    script_init (weechat_ruby_plugin,
                 &weechat_ruby_command_cb,
                 &weechat_ruby_completion_cb,
                 &weechat_ruby_debug_dump_cb,
                 &weechat_ruby_load_cb);
    
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
    
    ruby_finalize();
    
    return WEECHAT_RC_OK;
}
