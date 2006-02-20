/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* wee-ruby.c: Ruby plugin support for WeeChat */


#include <ruby.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#undef _
#include "../../weechat-plugin.h"
#include "../weechat-script.h"


char plugin_name[]        = "Ruby";
char plugin_version[]     = "0.1";
char plugin_description[] = "Ruby scripts support";

t_weechat_plugin *ruby_plugin;

t_plugin_script *ruby_scripts = NULL;
t_plugin_script *ruby_current_script = NULL;
char *ruby_current_script_filename = NULL;

VALUE mWeechat, mWeechatOutputs;

#define MOD_NAME_PREFIX "WeechatRubyModule"
int modnum = 0;

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
rb_protect_funcall(VALUE recv, ID mid, int *state, int argc, ...)
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

int
weechat_ruby_exec (t_weechat_plugin *plugin,
                   t_plugin_script *script,
                   char *function, char *server, char *arguments)
{
    VALUE ruby_retcode;
    int ruby_error;
    /* make gcc happy */
    (void) plugin;
    
    ruby_current_script = script;

    ruby_retcode = rb_protect_funcall ((VALUE) script->interpreter, rb_intern(function),
                                       &ruby_error, 2,
                                       rb_str_new2((server == NULL) ? "" : server),
                                       rb_str_new2((arguments == NULL) ? "" : arguments));
    if (ruby_error)
    {
	ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to run function \"%s\"",
                                   function);
	
	rb_eval_string_protect("Weechat.print(\"Ruby error: \" + $@.to_s)", NULL);
	rb_eval_string_protect("Weechat.print(\"Ruby error: \" + $!.to_s)", NULL);
	
        return PLUGIN_RC_KO;
    }
    
    return NUM2INT(ruby_retcode);
}

/*
 * weechat_ruby_handler: general message and command handler for Ruby
 */

int
weechat_ruby_handler (t_weechat_plugin *plugin,
                      char *server, char *command, char *arguments,
                      char *handler_args, void *handler_pointer)
{
    /* make gcc happy */
    (void) command;
    
    return weechat_ruby_exec (plugin, (t_plugin_script *)handler_pointer,
                              handler_args, server, arguments);
}

/*
 * weechat_ruby_register: startup function for all WeeChat Ruby scripts
 */

static VALUE
weechat_ruby_register (VALUE class, VALUE name, VALUE version,
                       VALUE shutdown_func, VALUE description)
{
    char *c_name, *c_version, *c_shutdown_func, *c_description;
    
    /* make gcc happy */
    (void) class;
    
    if (NIL_P (name) || NIL_P (version) || NIL_P (shutdown_func) || NIL_P (description))
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: wrong parameters for "
                                   "\"register\" function");
        return INT2FIX (0);
    }
    
    Check_Type (name, T_STRING);
    Check_Type (version, T_STRING);
    Check_Type (shutdown_func, T_STRING);
    Check_Type (description, T_STRING);
    
    c_name = STR2CSTR (name);
    c_version = STR2CSTR (version);
    c_shutdown_func = STR2CSTR (shutdown_func);
    c_description = STR2CSTR (description);
    
    if (weechat_script_search (ruby_plugin, &ruby_scripts, c_name))
    {
        /* error: another scripts already exists with this name! */
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to register "
                                   "\"%s\" script (another script "
                                   "already exists with this name)",
                                   c_name);
        return INT2FIX (0);
    }
    
    /* register script */
    ruby_current_script = weechat_script_add (ruby_plugin,
                                              &ruby_scripts,
                                              (ruby_current_script_filename) ?
                                              ruby_current_script_filename : "",
                                              c_name, c_version, c_shutdown_func,
                                              c_description);
    if (ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby: registered script \"%s\", "
                                   "version %s (%s)",
                                   c_name, c_version, c_description);
    }
    else
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to load script "
                                   "\"%s\" (not enough memory)",
                                   c_name);
        return INT2FIX (0);
    }
    
    return INT2FIX (1);
}

/*
 * weechat_ruby_print: print message into a buffer (current or specified one)
 */

static VALUE
weechat_ruby_print (int argc, VALUE *argv, VALUE class)
{
    VALUE message, channel_name, server_name;
    char *c_message, *c_channel_name, *c_server_name;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to print message, "
                                   "script not initialized");
        return INT2FIX (0);
    }
    
    message = Qnil;
    channel_name = Qnil;
    server_name = Qnil;
    c_message = NULL;
    c_channel_name = NULL;
    c_server_name = NULL;
 
    rb_scan_args (argc, argv, "12", &message, &channel_name, &server_name);
   
    if (NIL_P (message))
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: wrong parameters for "
                                   "\"print\" function");
        return INT2FIX (0);
    }
    
    Check_Type (message, T_STRING);
    c_message = STR2CSTR (message);

    if (!NIL_P (channel_name))
    {
        Check_Type (channel_name, T_STRING);
	c_channel_name = STR2CSTR (channel_name);
    }

    if (!NIL_P (server_name))
    {
        Check_Type (server_name, T_STRING);
	c_server_name = STR2CSTR (server_name);
    }
    
    ruby_plugin->print (ruby_plugin,
                        c_server_name, c_channel_name,
                        "%s", c_message);
    
    return INT2FIX (1);
}

/*
 * weechat_ruby_print_infobar: print message to infobar
 */

static VALUE
weechat_ruby_print_infobar (VALUE class, VALUE delay, VALUE message)
{
    int c_delay;
    char *c_message;

    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to print infobar message, "
                                   "script not initialized");
        return INT2FIX (0);
    }
    
    c_delay = 1;
    c_message = NULL;
    
    if (NIL_P (delay) || NIL_P (message))
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: wrong parameters for "
                                   "\"print_infobar\" function");
        return INT2FIX (0);
    }
    
    Check_Type (delay, T_FIXNUM);
    Check_Type (message, T_STRING);
    
    c_delay = FIX2INT (delay);
    c_message = STR2CSTR (message);
    
    ruby_plugin->print_infobar (ruby_plugin, c_delay, "%s", c_message);
    
    return INT2FIX (1);
}

/*
 * weechat_ruby_remove_infobar: remove message(s) from infobar
 */

static VALUE
weechat_ruby_remove_infobar (int argc, VALUE *argv, VALUE class)
{
    VALUE how_many;
    int c_how_many;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to remove infobar message(s), "
                                   "script not initialized");
        return INT2FIX (0);
    }
    
    how_many = Qnil;
    
    rb_scan_args (argc, argv, "01", &how_many);
    
    if (!NIL_P (how_many))
    {
        Check_Type (how_many, T_FIXNUM);
        c_how_many = FIX2INT (how_many);
    }
    else
        c_how_many = 0;
    
    ruby_plugin->infobar_remove (ruby_plugin, c_how_many);
    
    return INT2FIX (1);
}

/*
 * weechat_ruby_log: log message in server/channel (current or specified ones)
 */

static VALUE
weechat_ruby_log (int argc, VALUE *argv, VALUE class)
{
    VALUE message, channel_name, server_name;
    char *c_message, *c_channel_name, *c_server_name;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to log message, "
                                   "script not initialized");
        return INT2FIX (0);
    }
    
    message = Qnil;
    channel_name = Qnil;
    server_name = Qnil;
    c_message = NULL;
    c_channel_name = NULL;
    c_server_name = NULL;
 
    rb_scan_args (argc, argv, "12", &message, &channel_name, &server_name);
   
    if (NIL_P (message))
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: wrong parameters for "
                                   "\"log\" function");
        return INT2FIX (0);
    }
    
    Check_Type (message, T_STRING);
    c_message = STR2CSTR (message);

    if (!NIL_P (channel_name))
    {
        Check_Type (channel_name, T_STRING);
	c_channel_name = STR2CSTR (channel_name);
    }

    if (!NIL_P (server_name))
    {
        Check_Type (server_name, T_STRING);
	c_server_name = STR2CSTR (server_name);
    }
    
    ruby_plugin->log (ruby_plugin,
		      c_server_name, c_channel_name,
		      "%s", c_message);
    
    return INT2FIX (1);
}

/*
 * weechat_ruby_command: send command to server
 */

static VALUE
weechat_ruby_command (int argc, VALUE *argv, VALUE class)
{
    VALUE command, channel_name, server_name;
    char *c_command, *c_channel_name, *c_server_name;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to run command, "
                                   "script not initialized");
        return INT2FIX (0);
    }
    
    command = Qnil;
    channel_name = Qnil;
    server_name = Qnil;
    c_command = NULL;
    c_channel_name = NULL;
    c_server_name = NULL;
    
    rb_scan_args(argc, argv, "12", &command, &channel_name, &server_name);
    
    if (NIL_P (command))
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: wrong parameters for "
                                   "\"command\" function");
        return INT2FIX (0);
    }
    
    Check_Type (command, T_STRING);
    c_command = STR2CSTR (command);
    
    if (!NIL_P (channel_name))
    {
        Check_Type (channel_name, T_STRING);
	c_channel_name = STR2CSTR (channel_name);
    }

    if (!NIL_P (server_name))
    {
        Check_Type (server_name, T_STRING);
	c_server_name = STR2CSTR (server_name);
    }
    
    ruby_plugin->exec_command (ruby_plugin,
                               c_server_name, c_channel_name,
                               c_command);
    
    return INT2FIX (1);
}

/*
 * weechat_ruby_add_message_handler: add handler for messages
 */

static VALUE
weechat_ruby_add_message_handler (VALUE class, VALUE message, VALUE function)
{
    char *c_message, *c_function;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to add message handler, "
                                   "script not initialized");
        return INT2FIX (0);
    }
    
    c_message = NULL;
    c_function = NULL;
    
    if (NIL_P (message) || NIL_P (function))
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: wrong parameters for "
                                   "\"add_message_handler\" function");
        return INT2FIX (0);
    }
    
    Check_Type (message, T_STRING);
    Check_Type (function, T_STRING);
    
    c_message = STR2CSTR (message);
    c_function = STR2CSTR (function);
    
    if (ruby_plugin->msg_handler_add (ruby_plugin, c_message,
                                      weechat_ruby_handler, c_function,
                                      (void *)ruby_current_script))
        return INT2FIX (1);
    
    return INT2FIX (0);
}

/*
 * weechat_ruby_add_command_handler: define/redefines commands
 */

static VALUE
weechat_ruby_add_command_handler (int argc, VALUE *argv, VALUE class)
{
    VALUE command, function, description, arguments, arguments_description;
    VALUE completion_template;
    char *c_command, *c_function, *c_description, *c_arguments, *c_arguments_description;
    char *c_completion_template;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to add command handler, "
                                   "script not initialized");
        return INT2FIX (0);
    }
    
    command = Qnil;
    function = Qnil;
    description = Qnil;
    arguments = Qnil;
    arguments_description = Qnil;
    completion_template = Qnil;
    c_command = NULL;
    c_function = NULL;
    c_description = NULL;
    c_arguments = NULL;
    c_arguments_description = NULL;
    c_completion_template = NULL;
    
    rb_scan_args (argc, argv, "24", &command, &function, &description,
                  &arguments, &arguments_description, &completion_template);
    
    if (NIL_P (command) || NIL_P (function))
    {
        ruby_plugin->print_server (ruby_plugin,
                                    "Ruby error: wrong parameters for "
                                    "\"add_command_handler\" function");
        return INT2FIX (0);
    }
    
    Check_Type (command, T_STRING);
    c_command = STR2CSTR (command);
    Check_Type (function, T_STRING);
    c_function = STR2CSTR (function);

    if (!NIL_P (description))
    {
        Check_Type (description, T_STRING);
	c_description = STR2CSTR (description);
    }
    
    if (!NIL_P (arguments))
    {
        Check_Type (arguments, T_STRING);
	c_arguments = STR2CSTR (arguments);
    }
    
    if (!NIL_P (arguments_description))
    {
        Check_Type (arguments_description, T_STRING);
	c_arguments_description = STR2CSTR (arguments_description);
    }
    
    if (!NIL_P (completion_template))
    {
        Check_Type (completion_template, T_STRING);
	c_completion_template = STR2CSTR (completion_template);
    }
    
    if (ruby_plugin->cmd_handler_add (ruby_plugin,
                                      c_command,
                                      c_description,
                                      c_arguments,
                                      c_arguments_description,
                                      c_completion_template,
                                      weechat_ruby_handler,
                                      c_function,
                                      (void *)ruby_current_script))
        return INT2FIX (1);
    
    return INT2FIX (0);
}

/*
 * weechat_ruby_add_timer_handler: add a timer handler
 */

static VALUE
weechat_ruby_add_timer_handler (VALUE class, VALUE interval, VALUE function)
{
    int c_interval;
    char *c_function;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to add timer handler, "
                                   "script not initialized");
        return INT2FIX (0);
    }
    
    c_interval = 10;
    c_function = NULL;
    
    if (NIL_P (interval) || NIL_P (function))
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: wrong parameters for "
                                   "\"add_timer_handler\" function");
        return INT2FIX (0);
    }
    
    Check_Type (interval, T_FIXNUM);
    Check_Type (function, T_STRING);
    
    c_interval = FIX2INT (interval);
    c_function = STR2CSTR (function);
    
    if (ruby_plugin->timer_handler_add (ruby_plugin, c_interval,
                                        weechat_ruby_handler, c_function,
                                        (void *)ruby_current_script))
        return INT2FIX (1);
    
    return INT2FIX (0);
}

/*
 * weechat_ruby_remove_handler: remove a handler
 */

static VALUE
weechat_ruby_remove_handler (VALUE class, VALUE command, VALUE function)
{
    char *c_command, *c_function;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to remove handler, "
                                   "script not initialized");
        return INT2FIX (0);
    }
    
    c_command = NULL;
    c_function = NULL;
    
    if (NIL_P (command) || NIL_P (function))
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: wrong parameters for "
                                   "\"remove_handler\" function");
        return INT2FIX (0);
    }
    
    Check_Type (command, T_STRING);
    Check_Type (function, T_STRING);
    
    c_command = STR2CSTR (command);
    c_function = STR2CSTR (function);
    
    weechat_script_remove_handler (ruby_plugin, ruby_current_script,
                                   c_command, c_function);
    
    return INT2FIX (1);
}

/*
 * weechat_ruby_remove_timer_handler: remove a timer handler
 */

static VALUE
weechat_ruby_remove_timer_handler (VALUE class, VALUE function)
{
    char *c_function;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to remove timer handler, "
                                   "script not initialized");
        return INT2FIX (0);
    }
    
    c_function = NULL;
    
    if (NIL_P (function))
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: wrong parameters for "
                                   "\"remove_timer_handler\" function");
        return INT2FIX (0);
    }
    
    Check_Type (function, T_STRING);
    
    c_function = STR2CSTR (function);
    
    weechat_script_remove_timer_handler (ruby_plugin, ruby_current_script,
                                         c_function);
    
    return INT2FIX (1);
}

/*
 * weechat_ruby_get_info: get various infos
 */

static VALUE
weechat_ruby_get_info (int argc, VALUE *argv, VALUE class)
{
    char *c_arg, *c_server_name, *info;
    VALUE arg, server_name, return_value;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to get info, "
                                   "script not initialized");
        return INT2FIX (0);
    }
    
    arg = Qnil;
    server_name = Qnil;
    c_arg = NULL;
    c_server_name = NULL;

    rb_scan_args (argc, argv, "11", &arg, &server_name);
    
    if (NIL_P (arg))
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: wrong parameters for "
                                   "\"get_info\" function");
        return INT2FIX (0);
    }
    
    Check_Type (arg, T_STRING);
    c_arg = STR2CSTR (arg);

    if (!NIL_P (server_name))
    {
        Check_Type (server_name, T_STRING);
	c_server_name = STR2CSTR (server_name);
    }        
    
    if (c_arg)
    {
        info = ruby_plugin->get_info (ruby_plugin, c_arg, c_server_name);
        
        if (info)
        {
            return_value = rb_str_new2 (info);
            free (info);
            return return_value;
        }
    }
    
    return rb_str_new2 ("");
}

/*
 * weechat_ruby_get_dcc_info: get infos about DCC
 */

static VALUE
weechat_ruby_get_dcc_info (VALUE class)
{
    t_plugin_dcc_info *dcc_info, *ptr_dcc;
    VALUE dcc_list, dcc_list_member;
    char timebuffer1[64];
    char timebuffer2[64];
    struct in_addr in;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to get DCC info, "
                                   "script not initialized");
        return INT2FIX (0);
    }

    dcc_list = rb_ary_new();
    
    if (NIL_P (dcc_list))
        return Qnil;
    
    dcc_info = ruby_plugin->get_dcc_info (ruby_plugin);    
    if (!dcc_info)
        return dcc_list;
    
    for(ptr_dcc = dcc_info; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
    {
	strftime(timebuffer1, sizeof(timebuffer1), "%F %T",
		 localtime(&ptr_dcc->start_time));
	strftime(timebuffer2, sizeof(timebuffer2), "%F %T",
		 localtime(&ptr_dcc->start_transfer));
	in.s_addr = htonl(ptr_dcc->addr);

	dcc_list_member = rb_hash_new ();

	if (!NIL_P (dcc_list_member))
	{
	    rb_hash_aset (dcc_list_member, rb_str_new2("server"),
			  rb_str_new2(ptr_dcc->server));
	    rb_hash_aset (dcc_list_member, rb_str_new2("channel"),
			  rb_str_new2(ptr_dcc->channel));
	    rb_hash_aset (dcc_list_member, rb_str_new2("type"), 
			  INT2FIX(ptr_dcc->type));
	    rb_hash_aset (dcc_list_member, rb_str_new2("status"),
			  INT2FIX(ptr_dcc->status));	
	    rb_hash_aset (dcc_list_member, rb_str_new2("start_time"),
			  rb_str_new2(timebuffer1));
	    rb_hash_aset (dcc_list_member, rb_str_new2("start_transfer"),
			  rb_str_new2(timebuffer2));	    
	    rb_hash_aset (dcc_list_member, rb_str_new2("address"),
			  rb_str_new2(inet_ntoa(in)));
	    rb_hash_aset (dcc_list_member, rb_str_new2("port"),
			  INT2FIX(ptr_dcc->port));
	    rb_hash_aset (dcc_list_member, rb_str_new2("nick"),
			  rb_str_new2(ptr_dcc->nick));
	    rb_hash_aset (dcc_list_member, rb_str_new2("remote_file"),
			  rb_str_new2(ptr_dcc->filename));
	    rb_hash_aset (dcc_list_member, rb_str_new2("local_file"),
			  rb_str_new2(ptr_dcc->local_filename));
	    rb_hash_aset (dcc_list_member, rb_str_new2("filename_suffix"),
			  INT2FIX(ptr_dcc->filename_suffix));
	    rb_hash_aset (dcc_list_member, rb_str_new2("size"),
			  INT2FIX(ptr_dcc->size));
	    rb_hash_aset (dcc_list_member, rb_str_new2("pos"),
			  INT2FIX(ptr_dcc->pos));
	    rb_hash_aset (dcc_list_member, rb_str_new2("start_resume"),
			  INT2FIX(ptr_dcc->start_resume));
	    rb_hash_aset (dcc_list_member, rb_str_new2("cps"),
			  INT2FIX(ptr_dcc->bytes_per_sec));
	    
	    if (NIL_P (rb_ary_push (dcc_list, dcc_list_member)))
	    {
		rb_gc_unregister_address (&dcc_list_member);
		rb_gc_unregister_address (&dcc_list);
		ruby_plugin->free_dcc_info (ruby_plugin, dcc_info);
		return Qnil;
	    }
	}
	else
	{
	    rb_gc_unregister_address (&dcc_list);
	    ruby_plugin->free_dcc_info (ruby_plugin, dcc_info);
	    return Qnil;
	}	    
    }
    
    ruby_plugin->free_dcc_info (ruby_plugin, dcc_info);

    return dcc_list;
}

/*
 * weechat_ruby_get_config: get value of a WeeChat config option
 */

static VALUE
weechat_ruby_get_config (VALUE class, VALUE option)
{
    char *c_option, *return_value;
    VALUE ruby_return_value;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to get config option, "
                                   "script not initialized");
        return INT2FIX (0);
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: wrong parameters for "
                                   "\"get_config\" function");
        return INT2FIX (0);
    }
    
    Check_Type (option, T_STRING);
    c_option = STR2CSTR (option);
    
    if (c_option)
    {
        return_value = ruby_plugin->get_config (ruby_plugin, c_option);
        
        if (return_value)
        {
            ruby_return_value = rb_str_new2 (return_value);
            free (return_value);
            return ruby_return_value;
        }
    }
    
    return rb_str_new2 ("");
}

/*
 * weechat_ruby_set_config: set value of a WeeChat config option
 */

static VALUE
weechat_ruby_set_config (VALUE class, VALUE option, VALUE value)
{
    char *c_option, *c_value;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to set config option, "
                                   "script not initialized");
        return INT2FIX (0);
    }
    
    c_option = NULL;
    c_value = NULL;
    
    if (NIL_P (option))
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: wrong parameters for "
                                   "\"set_config\" function");
        return INT2FIX (0);
    }
    
    Check_Type (option, T_STRING);
    Check_Type (value, T_STRING);
    
    c_option = STR2CSTR (option);
    c_value = STR2CSTR (value);
    
    if (c_option && c_value)
    {
        if (ruby_plugin->set_config (ruby_plugin, c_option, c_value))
            return INT2FIX (1);
    }
    
    return INT2FIX (0);
}

/*
 * weechat_ruby_get_plugin_config: get value of a plugin config option
 */

static VALUE
weechat_ruby_get_plugin_config (VALUE class, VALUE option)
{
    char *c_option, *return_value;
    VALUE ruby_return_value;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to get plugin config option, "
                                   "script not initialized");
        return INT2FIX (0);
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: wrong parameters for "
                                   "\"get_plugin_config\" function");
        return INT2FIX (0);
    }
    
    Check_Type (option, T_STRING);
    c_option = STR2CSTR (option);
    
    if (c_option)
    {
        return_value = weechat_script_get_plugin_config (ruby_plugin,
                                                         ruby_current_script,
                                                         c_option);
        
        if (return_value)
        {
            ruby_return_value = rb_str_new2 (return_value);
            free (return_value);
            return ruby_return_value;
        }
    }
    
    return rb_str_new2 ("");
}

/*
 * weechat_ruby_set_plugin_config: set value of a plugin config option
 */

static VALUE
weechat_ruby_set_plugin_config (VALUE class, VALUE option, VALUE value)
{
    char *c_option, *c_value;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to set plugin config option, "
                                   "script not initialized");
        return INT2FIX (0);
    }
    
    c_option = NULL;
    c_value = NULL;
    
    if (NIL_P (option))
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: wrong parameters for "
                                   "\"set_plugin_config\" function");
        return INT2FIX (0);
    }
    
    Check_Type (option, T_STRING);
    Check_Type (value, T_STRING);
    
    c_option = STR2CSTR (option);
    c_value = STR2CSTR (value);
    
    if (c_option && c_value)
    {
        if (weechat_script_set_plugin_config (ruby_plugin,
                                              ruby_current_script,
                                              c_option, c_value))
            return INT2FIX (1);
    }
    
    return INT2FIX (0);
}

/*
 * weechat_ruby_get_server_info: get infos about servers
 */

static VALUE
weechat_ruby_get_server_info (VALUE class)
{
    t_plugin_server_info *server_info, *ptr_server;
    VALUE server_hash, server_hash_member;
    char timebuffer[64];
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to get server infos, "
                                   "script not initialized");
        return INT2FIX (0);
    }
    
    server_hash = rb_hash_new ();
    if (!server_hash)
	return Qnil;

    server_info = ruby_plugin->get_server_info (ruby_plugin);
    if  (!server_info)
	return server_hash;

    for(ptr_server = server_info; ptr_server; ptr_server = ptr_server->next_server)
    {
	strftime(timebuffer, sizeof(timebuffer), "%F %T",
		 localtime(&ptr_server->away_time));
	
	server_hash_member = rb_hash_new ();
	
	if (server_hash_member)
	{
	    rb_hash_aset (server_hash_member, rb_str_new2("autoconnect"),
			  INT2FIX(ptr_server->autoconnect));
	    rb_hash_aset (server_hash_member, rb_str_new2("autoreconnect"),
			  INT2FIX(ptr_server->autoreconnect));
	    rb_hash_aset (server_hash_member, rb_str_new2("autoreconnect_delay"),
			  INT2FIX(ptr_server->autoreconnect_delay));
	    rb_hash_aset (server_hash_member, rb_str_new2("command_line"),
			  INT2FIX(ptr_server->command_line));
	    rb_hash_aset (server_hash_member, rb_str_new2("address"),
			  rb_str_new2(ptr_server->address));
	    rb_hash_aset (server_hash_member, rb_str_new2("port"),
			  INT2FIX(ptr_server->port));
	    rb_hash_aset (server_hash_member, rb_str_new2("ipv6"),
			  INT2FIX(ptr_server->ipv6));
	    rb_hash_aset (server_hash_member, rb_str_new2("ssl"),
			  INT2FIX(ptr_server->ssl));
	    rb_hash_aset (server_hash_member, rb_str_new2("password"),
			  rb_str_new2(ptr_server->password));
	    rb_hash_aset (server_hash_member, rb_str_new2("nick1"),
			  rb_str_new2(ptr_server->nick1));
	    rb_hash_aset (server_hash_member, rb_str_new2("nick2"),
			  rb_str_new2(ptr_server->nick2));
	    rb_hash_aset (server_hash_member, rb_str_new2("nick3"),
			  rb_str_new2(ptr_server->nick3));
	    rb_hash_aset (server_hash_member, rb_str_new2("username"),
			  rb_str_new2(ptr_server->username));
	    rb_hash_aset (server_hash_member, rb_str_new2("realname"),
			  rb_str_new2(ptr_server->realname));
	    rb_hash_aset (server_hash_member, rb_str_new2("command"),
			  rb_str_new2(ptr_server->command));
	    rb_hash_aset (server_hash_member, rb_str_new2("command_delay"),
			  INT2FIX(ptr_server->command_delay));
	    rb_hash_aset (server_hash_member, rb_str_new2("autojoin"),
			  rb_str_new2(ptr_server->autojoin));
	    rb_hash_aset (server_hash_member, rb_str_new2("autorejoin"),
			  INT2FIX(ptr_server->autorejoin));
	    rb_hash_aset (server_hash_member, rb_str_new2("notify_levels"),
			  rb_str_new2(ptr_server->notify_levels));
	    rb_hash_aset (server_hash_member, rb_str_new2("charset_decode_iso"),
			  rb_str_new2(ptr_server->charset_decode_iso));
	    rb_hash_aset (server_hash_member, rb_str_new2("charset_decode_utf"),
			  rb_str_new2(ptr_server->charset_decode_utf));
	    rb_hash_aset (server_hash_member, rb_str_new2("charset_encode"),
			  rb_str_new2(ptr_server->charset_encode));
	    rb_hash_aset (server_hash_member, rb_str_new2("is_connected"),
			  INT2FIX(ptr_server->is_connected));
	    rb_hash_aset (server_hash_member, rb_str_new2("ssl_connected"),
			  INT2FIX(ptr_server->ssl_connected));
	    rb_hash_aset (server_hash_member, rb_str_new2("nick"),
			  rb_str_new2(ptr_server->nick));
	    rb_hash_aset (server_hash_member, rb_str_new2("away_time"),
			  rb_str_new2(timebuffer));
	    rb_hash_aset (server_hash_member, rb_str_new2("lag"),
			  INT2FIX(ptr_server->lag));
	    
	    rb_hash_aset (server_hash, rb_str_new2(ptr_server->name), server_hash_member);
	}
    }    

    ruby_plugin->free_server_info(ruby_plugin, server_info);
    
    return server_hash;
}

/*
 * weechat_ruby_get_channel_info: get infos about channels
 */

static VALUE
weechat_ruby_get_channel_info (VALUE class, VALUE server)
{
    t_plugin_channel_info *channel_info, *ptr_channel;
    VALUE channel_hash, channel_hash_member;
    char *c_server;
    
    /* make gcc happy */
    (void) class;
        
    if (!ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to get channel infos, "
                                   "script not initialized");
        return INT2FIX (0);
    }
    
    c_server = NULL;
    if (NIL_P (server))
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: wrong parameters for "
                                   "\"get_channel_info\" function");
        return INT2FIX (0);
    }
    
    Check_Type (server, T_STRING);
    c_server = STR2CSTR (server);

    if (!c_server)
	return INT2FIX (0);
    
    channel_hash = rb_hash_new ();
    if (!channel_hash)
	return Qnil;

    channel_info = ruby_plugin->get_channel_info (ruby_plugin, c_server);
    if  (!channel_info)
	return channel_hash;

    for(ptr_channel = channel_info; ptr_channel; ptr_channel = ptr_channel->next_channel)
    {
	channel_hash_member = rb_hash_new ();
	
	if (channel_hash_member)
	{
	    rb_hash_aset (channel_hash_member, rb_str_new2("type"),
			  INT2FIX(ptr_channel->type));
	    rb_hash_aset (channel_hash_member, rb_str_new2("topic"),
			  rb_str_new2(ptr_channel->topic));
	    rb_hash_aset (channel_hash_member, rb_str_new2("modes"),
			  rb_str_new2(ptr_channel->modes));
	    rb_hash_aset (channel_hash_member, rb_str_new2("limit"),
			  INT2FIX(ptr_channel->limit));
	    rb_hash_aset (channel_hash_member, rb_str_new2("key"),
			  rb_str_new2(ptr_channel->key));
	    rb_hash_aset (channel_hash_member, rb_str_new2("nicks_count"),
			  INT2FIX(ptr_channel->nicks_count));
	    
	    rb_hash_aset (channel_hash, rb_str_new2(ptr_channel->name), channel_hash_member);
	}
    }    

    ruby_plugin->free_channel_info(ruby_plugin, channel_info);
    
    return channel_hash;
}

/*
 * weechat_ruby_get_nick_info: get infos about nicks
 */

static VALUE
weechat_ruby_get_nick_info (VALUE class, VALUE server, VALUE channel)
{
    t_plugin_nick_info *nick_info, *ptr_nick;
    VALUE nick_hash, nick_hash_member;
    char *c_server, *c_channel;
    
    /* make gcc happy */
    (void) class;

    if (!ruby_current_script)
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to get channel infos, "
                                   "script not initialized");
        return INT2FIX (0);
    }
    
    c_server = NULL;
    c_channel = NULL;
    if (NIL_P (server) || NIL_P (channel))
    {
        ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: wrong parameters for "
                                   "\"get_nick_info\" function");
        return INT2FIX (0);
    }
    
    Check_Type (server, T_STRING);
    Check_Type (channel, T_STRING);
    
    c_server = STR2CSTR (server);
    c_channel = STR2CSTR (channel);

    if (!c_server || !c_channel)
	return INT2FIX (0);
    
    nick_hash = rb_hash_new ();
    if (!nick_hash)
	return Qnil;

    nick_info = ruby_plugin->get_nick_info (ruby_plugin, c_server, c_channel);
    if  (!nick_info)
	return nick_hash;

    for(ptr_nick = nick_info; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
	nick_hash_member = rb_hash_new ();
	
	if (nick_hash_member)
	{
	    rb_hash_aset (nick_hash_member, rb_str_new2("flags"),
			  INT2FIX(ptr_nick->flags));
	    
	    rb_hash_aset (nick_hash, rb_str_new2(ptr_nick->nick), nick_hash_member);
	}
    }    

    ruby_plugin->free_nick_info(ruby_plugin, nick_info);
    
    return nick_hash;
}

/*
 * weechat_ruby_output : redirection for stdout and stderr
 */

static VALUE 
weechat_ruby_output(VALUE self, VALUE str)
{
    char *msg, *p;
    /* make gcc happy */
    (void) self;
    
    msg = strdup(STR2CSTR(str));
    
    while ((p = strrchr(msg, '\n')) != NULL)
	*p = '\0';
    
    if (strlen(msg) > 0)
	ruby_plugin->print_server (ruby_plugin,
                                   "Ruby stdout/stderr: %s", msg);
    if (msg)
	free (msg);
    
    return Qnil;
}


/*
 * weechat_ruby_load: load a Ruby script
 */

int
weechat_ruby_load (t_weechat_plugin *plugin, char *filename)
{
    char modname[64];
    VALUE curModule, ruby_retcode;
    int ruby_error;
    
    plugin->print_server (plugin, "Loading Ruby script \"%s\"", filename);
    ruby_current_script = NULL;

    snprintf(modname, sizeof(modname), "%s%d", MOD_NAME_PREFIX, modnum);
    modnum++;

    curModule = rb_define_module(modname);

    ruby_current_script_filename = strdup (filename);
    
    ruby_retcode = rb_protect_funcall (curModule, rb_intern("load_eval_file"),
				       &ruby_error, 1, rb_str_new2(filename));
    
    free (ruby_current_script_filename);
    
    if (NUM2INT(ruby_retcode) != 0)
    {
	VALUE ruby_eval_error;
	
	switch (NUM2INT(ruby_retcode))
	{
	case 1:
	    ruby_plugin->print_server (ruby_plugin,
                                       "Ruby error: unable to read file \"%s\"",
                                       filename);
	    break;
	case 2:
	    
	
	    ruby_plugin->print_server (ruby_plugin,
                                       "Ruby error: error while loading file \"%s\"",
                                       filename);
	    break;
	case 3:
	    ruby_plugin->print_server (ruby_plugin,
                                       "Ruby error: unable to find \"weechat_init\" function in file \"%s\"",
                                       filename);
	    break;
	}
	
	if (NUM2INT(ruby_retcode) == 1 || NUM2INT(ruby_retcode) == 2)
	{
	    ruby_eval_error = rb_iv_get(curModule, "@load_eval_file_error");
	    if (ruby_eval_error)		      		
		ruby_plugin->print_server (ruby_plugin,
                                           "Ruby error: %s", STR2CSTR(ruby_eval_error));
	}
	
	return 0;
    }
    
    ruby_retcode = rb_protect_funcall (curModule, rb_intern("weechat_init"), &ruby_error, 0);
    
    if (ruby_error)
    {
	ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to eval weechat_init in file \"%s\"",
                                   filename);	
	rb_eval_string_protect("Weechat.print(\"Ruby error: \" + $@.to_s)", NULL);
	rb_eval_string_protect("Weechat.print(\"Ruby error: \" + $!.to_s)", NULL);
	
	if (ruby_current_script != NULL)
	    weechat_script_remove (plugin, &ruby_scripts, ruby_current_script);
	return 0;
    }
    
    if (ruby_current_script == NULL) {
	plugin->print_server (plugin,
                              "Ruby error: function \"register\" not found "
                              "in file \"%s\"",
                              filename);
        return 0;
    }
    
    ruby_current_script->interpreter = (VALUE *) curModule;
    rb_gc_register_address (ruby_current_script->interpreter);
    
    return 1;
}

/*
 * weechat_ruby_unload: unload a Ruby script
 */

void
weechat_ruby_unload (t_weechat_plugin *plugin, t_plugin_script *script)
{
    plugin->print_server (plugin,
                          "Unloading Ruby script \"%s\"",
                          script->name);
    
    if (script->shutdown_func[0])
        weechat_ruby_exec (plugin, script, script->shutdown_func, "", "");
    
    if (script->interpreter)
	rb_gc_unregister_address (script->interpreter);
    
    weechat_script_remove (plugin, &ruby_scripts, script);
}

/*
 * weechat_ruby_unload_name: unload a Ruby script by name
 */

void
weechat_ruby_unload_name (t_weechat_plugin *plugin, char *name)
{
    t_plugin_script *ptr_script;
    
    ptr_script = weechat_script_search (plugin, &ruby_scripts, name);
    if (ptr_script)
    {
        weechat_ruby_unload (plugin, ptr_script);
        plugin->print_server (plugin,
                              "Ruby script \"%s\" unloaded",
                              name);
    }
    else
    {
        plugin->print_server (plugin,
                              "Ruby error: script \"%s\" not loaded",
                              name);
    }
}

/*
 * weechat_ruby_unload_all: unload all Ruby scripts
 */

void
weechat_ruby_unload_all (t_weechat_plugin *plugin)
{

    plugin->print_server (plugin,
                          "Unloading all Ruby scripts");
    while (ruby_scripts)
	weechat_ruby_unload (plugin, ruby_scripts);
    
    plugin->print_server (plugin,
                          "Ruby scripts unloaded");
}

/*
 * weechat_ruby_cmd: /ruby command handler
 */

int
weechat_ruby_cmd (t_weechat_plugin *plugin,
                  char *server, char *command, char *arguments,
                  char *handler_args, void *handler_pointer)
{
    int argc, path_length, handler_found;
    char **argv, *path_script, *dir_home;
    t_plugin_script *ptr_script;
    t_plugin_handler *ptr_handler;
    
    /* make gcc happy */
    (void) server;
    (void) command;
    (void) handler_args;
    (void) handler_pointer;
    
    if (arguments)
        argv = plugin->explode_string (plugin, arguments, " ", 0, &argc);
    else
    {
        argv = NULL;
        argc = 0;
    }
    
    switch (argc)
    {
        case 0:
            /* list registered Ruby scripts */
            plugin->print_server (plugin, "");
            plugin->print_server (plugin, "Registered Ruby scripts:");
            if (ruby_scripts)
            {
                for (ptr_script = ruby_scripts;
                     ptr_script; ptr_script = ptr_script->next_script)
                {
                    plugin->print_server (plugin, "  %s v%s%s%s",
                                          ptr_script->name,
                                          ptr_script->version,
                                          (ptr_script->description[0]) ? " - " : "",
                                          ptr_script->description);
                }
            }
            else
                plugin->print_server (plugin, "  (none)");
            
            /* list Ruby message handlers */
            plugin->print_server (plugin, "");
            plugin->print_server (plugin, "Ruby message handlers:");
            handler_found = 0;
            for (ptr_handler = plugin->handlers;
                 ptr_handler; ptr_handler = ptr_handler->next_handler)
            {
                if ((ptr_handler->type == HANDLER_MESSAGE)
                    && (ptr_handler->handler_args))
                {
                    handler_found = 1;
                    plugin->print_server (plugin, "  IRC(%s) => Ruby(%s)",
                                          ptr_handler->irc_command,
                                          ptr_handler->handler_args);
                }
            }
            if (!handler_found)
                plugin->print_server (plugin, "  (none)");
            
            /* list Ruby command handlers */
            plugin->print_server (plugin, "");
            plugin->print_server (plugin, "Ruby command handlers:");
            handler_found = 0;
            for (ptr_handler = plugin->handlers;
                 ptr_handler; ptr_handler = ptr_handler->next_handler)
            {
                if ((ptr_handler->type == HANDLER_COMMAND)
                    && (ptr_handler->handler_args))
                {
                    handler_found = 1;
                    plugin->print_server (plugin, "  /%s => Ruby(%s)",
                                          ptr_handler->command,
                                          ptr_handler->handler_args);
                }
            }
            if (!handler_found)
                plugin->print_server (plugin, "  (none)");
            
            /* list Ruby timer handlers */
            plugin->print_server (plugin, "");
            plugin->print_server (plugin, "Ruby timer handlers:");
            handler_found = 0;
            for (ptr_handler = plugin->handlers;
                 ptr_handler; ptr_handler = ptr_handler->next_handler)
            {
                if ((ptr_handler->type == HANDLER_TIMER)
                    && (ptr_handler->handler_args))
                {
                    handler_found = 1;
                    plugin->print_server (plugin, "  %d seconds => Ruby(%s)",
                                          ptr_handler->interval,
                                          ptr_handler->handler_args);
                }
            }
            if (!handler_found)
                plugin->print_server (plugin, "  (none)");
            break;
        case 1:
            if (plugin->ascii_strcasecmp (plugin, argv[0], "autoload") == 0)
                weechat_script_auto_load (plugin, "ruby", weechat_ruby_load);
            else if (plugin->ascii_strcasecmp (plugin, argv[0], "reload") == 0)
            {
                weechat_ruby_unload_all (plugin);
                weechat_script_auto_load (plugin, "ruby", weechat_ruby_load);
            }
            else if (plugin->ascii_strcasecmp (plugin, argv[0], "unload") == 0)
                weechat_ruby_unload_all (plugin);
            break;
        case 2:
            if (plugin->ascii_strcasecmp (plugin, argv[0], "load") == 0)
            {
                /* load Ruby script */
                if ((strstr (argv[1], "/")) || (strstr (argv[1], "\\")))
                    path_script = NULL;
                else
                {
                    dir_home = plugin->get_info (plugin, "weechat_dir", NULL);
                    if (dir_home)
                    {
                        path_length = strlen (dir_home) + strlen (argv[1]) + 16;
                        path_script = (char *) malloc (path_length * sizeof (char));
                        if (path_script)
                            snprintf (path_script, path_length, "%s/ruby/%s",
                                      dir_home, argv[1]);
                        else
                            path_script = NULL;
                        free (dir_home);
                    }
                    else
                        path_script = NULL;
                }
                weechat_ruby_load (plugin, (path_script) ? path_script : argv[1]);
                if (path_script)
                    free (path_script);
            }
            else if (plugin->ascii_strcasecmp (plugin, argv[0], "unload") == 0)
            {
                /* unload Ruby script */
                weechat_ruby_unload_name (plugin, argv[1]);
            }
            else
            {
                plugin->print_server (plugin,
                                      "Ruby error: unknown option for "
                                      "\"ruby\" command");
            }
            break;
        default:
            plugin->print_server (plugin,
                                  "Ruby error: wrong argument count for \"ruby\" command");
    }
    
    if (argv)
        plugin->free_exploded_string (plugin, argv);
    
    return 1;
}

/*
 * weechat_plugin_init: initialize Ruby plugin
 */

int
weechat_plugin_init (t_weechat_plugin *plugin)
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
    ruby_plugin = plugin;
    ruby_error = 0;

    plugin->print_server (plugin, "Loading Ruby module \"weechat\"");
    
    ruby_init ();
    ruby_init_loadpath ();
    ruby_script ("__weechat_plugin__");
    
    mWeechat = rb_define_module("Weechat");
    rb_define_const(mWeechat, "PLUGIN_RC_OK", INT2NUM(PLUGIN_RC_OK));
    rb_define_const(mWeechat, "PLUGIN_RC_KO", INT2NUM(PLUGIN_RC_KO));
    rb_define_const(mWeechat, "PLUGIN_RC_OK_IGNORE_WEECHAT", INT2NUM(PLUGIN_RC_OK_IGNORE_WEECHAT));
    rb_define_const(mWeechat, "PLUGIN_RC_OK_IGNORE_PLUGINS", INT2NUM(PLUGIN_RC_OK_IGNORE_PLUGINS));
    rb_define_const(mWeechat, "PLUGIN_RC_OK_IGNORE_ALL", INT2NUM(PLUGIN_RC_OK_IGNORE_ALL));    
    rb_define_module_function (mWeechat, "register", weechat_ruby_register, 4);
    rb_define_module_function (mWeechat, "print", weechat_ruby_print, -1);
    rb_define_module_function (mWeechat, "print_infobar", weechat_ruby_print_infobar, 2);
    rb_define_module_function (mWeechat, "remove_infobar", weechat_ruby_remove_infobar, -1);
    rb_define_module_function (mWeechat, "log", weechat_ruby_log, -1);
    rb_define_module_function (mWeechat, "command", weechat_ruby_command, -1);
    rb_define_module_function (mWeechat, "add_message_handler", weechat_ruby_add_message_handler, 2);
    rb_define_module_function (mWeechat, "add_command_handler", weechat_ruby_add_command_handler, -1);
    rb_define_module_function (mWeechat, "add_timer_handler", weechat_ruby_add_timer_handler, 2);
    rb_define_module_function (mWeechat, "remove_handler", weechat_ruby_remove_handler, 2);
    rb_define_module_function (mWeechat, "remove_timer_handler", weechat_ruby_remove_timer_handler, 1);
    rb_define_module_function (mWeechat, "get_info", weechat_ruby_get_info, -1);
    rb_define_module_function (mWeechat, "get_dcc_info", weechat_ruby_get_dcc_info, 0);
    rb_define_module_function (mWeechat, "get_config", weechat_ruby_get_config, 1);
    rb_define_module_function (mWeechat, "set_config", weechat_ruby_set_config, 2);
    rb_define_module_function (mWeechat, "get_plugin_config", weechat_ruby_get_plugin_config, 1);
    rb_define_module_function (mWeechat, "set_plugin_config", weechat_ruby_set_plugin_config, 2);
    rb_define_module_function (mWeechat, "get_server_info", weechat_ruby_get_server_info, 0);
    rb_define_module_function (mWeechat, "get_channel_info", weechat_ruby_get_channel_info, 1);
    rb_define_module_function (mWeechat, "get_nick_info", weechat_ruby_get_nick_info, 2);
    
    /* redirect stdin and stdout */
    mWeechatOutputs = rb_define_module("WeechatOutputs");
    rb_define_singleton_method(mWeechatOutputs, "write", weechat_ruby_output, 1);
    rb_define_singleton_method(mWeechatOutputs, "puts", weechat_ruby_output, 1);
    rb_define_singleton_method(mWeechatOutputs, "p", weechat_ruby_output, 1);

    plugin->cmd_handler_add (plugin, "ruby",
                             "list/load/unload Ruby scripts",
                             "[load filename] | [autoload] | [reload] | [unload]",
                             "filename: Ruby script (file) to load\n\n"
                             "Without argument, /ruby command lists all loaded Ruby scripts.",
                             "load|autoload|reload|unload",
                             weechat_ruby_cmd, NULL, NULL);
    
    plugin->mkdir_home (plugin, "ruby");
    plugin->mkdir_home (plugin, "ruby/autoload");
    
    rb_eval_string_protect(weechat_ruby_code, &ruby_error);
    if (ruby_error) {	
	VALUE ruby_error_info = rb_inspect(ruby_errinfo);
	ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: unable to eval weechat ruby internal code");
	ruby_plugin->print_server (ruby_plugin,
                                   "Ruby error: %s", STR2CSTR(ruby_error_info));
	return PLUGIN_RC_KO;
    }
   
    weechat_script_auto_load (plugin, "ruby", weechat_ruby_load);
    
    /* init ok */
    return PLUGIN_RC_OK;
}

/*
 * weechat_plugin_end: shutdown Ruby interface
 */

void
weechat_plugin_end (t_weechat_plugin *plugin)
{
    /* unload all scripts */
    weechat_ruby_unload_all (plugin);
    
    ruby_finalize();
        
    ruby_plugin->print_server (ruby_plugin,
                               "Ruby plugin ended");
}
