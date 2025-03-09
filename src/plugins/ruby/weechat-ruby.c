/*
 * weechat-ruby.c - ruby plugin for WeeChat
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005-2007 Emmanuel Bouthenot <kolter@openics.org>
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

#undef _

#include <ruby.h>
#if (defined(RUBY_API_VERSION_MAJOR) && defined(RUBY_API_VERSION_MINOR)) && (RUBY_API_VERSION_MAJOR >= 2 || (RUBY_API_VERSION_MAJOR == 1 && RUBY_API_VERSION_MINOR >= 9))
#include <ruby/encoding.h>
#endif
#ifdef HAVE_RUBY_VERSION_H
#include <ruby/version.h>
#endif

#include <signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../weechat-plugin.h"
#include "../plugin-script.h"
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
WEECHAT_PLUGIN_DESCRIPTION(N_("Support of ruby scripts"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(RUBY_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_ruby_plugin = NULL;

struct t_plugin_script_data ruby_data;

struct t_config_file *ruby_config_file = NULL;
struct t_config_option *ruby_config_look_check_license = NULL;
struct t_config_option *ruby_config_look_eval_keep_context = NULL;

int ruby_quiet = 0;

struct t_plugin_script *ruby_script_eval = NULL;
int ruby_eval_mode = 0;
int ruby_eval_send_input = 0;
int ruby_eval_exec_commands = 0;
struct t_gui_buffer *ruby_eval_buffer = NULL;
#define RUBY_EVAL_SCRIPT                                                \
    "def weechat_init\n"                                                \
    "  Weechat.register('" WEECHAT_SCRIPT_EVAL_NAME "', '', '1.0', "    \
    "'" WEECHAT_LICENSE "', 'Evaluation of source code', '', '')\n"     \
    "  return Weechat::WEECHAT_RC_OK\n"                                 \
    "end\n"                                                             \
    "\n"                                                                \
    "def script_ruby_eval(code)\n"                                      \
    "  module_eval(code)\n"                                             \
    "end\n"

struct t_plugin_script *ruby_scripts = NULL;
struct t_plugin_script *last_ruby_script = NULL;
struct t_plugin_script *ruby_current_script = NULL;
struct t_plugin_script *ruby_registered_script = NULL;
const char *ruby_current_script_filename = NULL;
VALUE ruby_current_module;
char **ruby_buffer_output = NULL;

/*
 * string used to execute action "install":
 * when signal "ruby_script_install" is received, name of string
 * is added to this string, to be installed later by a timer (when nothing is
 * running in script)
 */
char *ruby_action_install_list = NULL;

/*
 * string used to execute action "remove":
 * when signal "ruby_script_remove" is received, name of string
 * is added to this string, to be removed later by a timer (when nothing is
 * running in script)
 */
char *ruby_action_remove_list = NULL;

/*
 * string used to execute action "autoload":
 * when signal "ruby_script_autoload" is received, name of string
 * is added to this string, to autoload or disable autoload later by a timer
 * (when nothing is running in script)
 */
char *ruby_action_autoload_list = NULL;

VALUE ruby_mWeechat, ruby_mWeechatOutputs;

#define MOD_NAME_PREFIX "WeechatRubyModule"
int ruby_num = 0;

typedef struct protect_call_arg {
    VALUE recv;
    ID mid;
    int argc;
    VALUE *argv;
} protect_call_arg_t;


/*
 * Callback called for each key/value in a hashtable.
 */

void
weechat_ruby_hashtable_map_cb (void *data,
                               struct t_hashtable *hashtable,
                               const char *key,
                               const char *value)
{
    VALUE *hash;

    /* make C compiler happy */
    (void) hashtable;

    hash = (VALUE *)data;

    rb_hash_aset (hash[0], rb_str_new2 (key), rb_str_new2 (value));
}

/*
 * Converts a WeeChat hashtable to a ruby hash.
 */

VALUE
weechat_ruby_hashtable_to_hash (struct t_hashtable *hashtable)
{
    VALUE hash;

    hash = rb_hash_new ();
    if (NIL_P (hash))
        return Qnil;

    weechat_hashtable_map_string (hashtable, &weechat_ruby_hashtable_map_cb,
                                  &hash);

    return hash;
}

/*
 * Callback called for each key/value in a hashtable.
 */

int
weechat_ruby_hash_foreach_cb (VALUE key, VALUE value, VALUE arg)
{
    struct t_hashtable *hashtable;
    const char *type_values;

    hashtable = (struct t_hashtable *)arg;

    if ((TYPE(key) == T_STRING) && (TYPE(value) == T_STRING))
    {
        type_values = weechat_hashtable_get_string (hashtable, "type_values");
        if (strcmp (type_values, WEECHAT_HASHTABLE_STRING) == 0)
        {
            weechat_hashtable_set (hashtable, StringValuePtr (key),
                                   StringValuePtr (value));
        }
        else if (strcmp (type_values, WEECHAT_HASHTABLE_POINTER) == 0)
        {
            weechat_hashtable_set (hashtable, StringValuePtr (key),
                                   plugin_script_str2ptr (
                                       weechat_ruby_plugin,
                                       NULL, NULL,
                                       StringValuePtr (value)));
        }
    }

    return 0;
}

/*
 * Converts a ruby hash to a WeeChat hashtable.
 *
 * Note: hashtable must be freed after use.
 */

struct t_hashtable *
weechat_ruby_hash_to_hashtable (VALUE hash, int size, const char *type_keys,
                                const char *type_values)
{
    struct t_hashtable *hashtable;

    hashtable = weechat_hashtable_new (size, type_keys, type_values,
                                       NULL, NULL);
    if (!hashtable)
        return NULL;

    rb_hash_foreach (hash, &weechat_ruby_hash_foreach_cb,
                     (unsigned long)hashtable);

    return hashtable;
}

/*
 * Used to protect a function call.
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
 * Calls function in protected mode.
 */

VALUE
rb_protect_funcall (VALUE recv, ID mid, int *state, int argc, VALUE *argv)
{
    struct protect_call_arg arg;

    arg.recv = recv;
    arg.mid = mid;
    arg.argc = argc;
    arg.argv = argv;
    return rb_protect (protect_funcall0, (VALUE) &arg, state);
}

/*
 * Displays ruby exception.
 */

int
weechat_ruby_print_exception (VALUE err)
{
    VALUE backtrace, message, class, class_name, tmp3;
    int i;
    int ruby_error;
    char *line, **cline, *err_msg, *err_class;

    backtrace = rb_protect_funcall (err, rb_intern ("backtrace"),
                                    &ruby_error, 0, NULL);

    message = rb_protect_funcall (err, rb_intern ("message"), &ruby_error, 0, NULL);
    err_msg = StringValueCStr (message);

    err_class = NULL;
    class = rb_protect_funcall (err, rb_intern ("singleton_class"),
                                &ruby_error, 0, NULL);
    if (class != Qnil)
    {
        class_name = rb_protect_funcall (class, rb_intern ("to_s"),
                                         &ruby_error, 0, NULL);
        err_class = StringValuePtr (class_name);
    }

    if (err_class && (strcmp (err_class, "SyntaxError") == 0))
    {
        tmp3 = rb_inspect (err);
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error: %s"),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME,
                        StringValuePtr (tmp3));
    }
    else
    {
        cline = weechat_string_dyn_alloc (256);
        for (i = 0; i < RARRAY_LEN(backtrace); i++)
        {
            line = StringValuePtr (RARRAY_PTR(backtrace)[i]);
            weechat_string_dyn_copy (cline, NULL);
            if (i == 0)
            {
                weechat_string_dyn_concat (cline, line, -1);
                weechat_string_dyn_concat (cline, ": ", -1);
                weechat_string_dyn_concat (cline, err_msg, -1);
                if (err_class)
                {
                    weechat_string_dyn_concat (cline, " (", -1);
                    weechat_string_dyn_concat (cline, err_class, -1);
                    weechat_string_dyn_concat (cline, ")", -1);
                }
            }
            else
            {
                weechat_string_dyn_concat (cline, "     from ", -1);
                weechat_string_dyn_concat (cline, line, -1);
            }
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: error: %s"),
                            weechat_prefix ("error"), RUBY_PLUGIN_NAME,
                            *cline);
        }
        weechat_string_dyn_free (cline, 1);
    }

    return 0;
}

/*
 * Function used for compatibility.
 */

static VALUE
weechat_ruby_output_flush_ruby (VALUE self)
{
    /* make C compiler happy */
    (void) self;

    return Qnil;
}

/*
 * Flushes output.
 */

void
weechat_ruby_output_flush (void)
{
    const char *ptr_command;
    char *temp_buffer, *command;

    if (!(*ruby_buffer_output)[0])
        return;

    /* if there's no buffer, we catch the output, so there's no flush */
    if (ruby_eval_mode && !ruby_eval_buffer)
        return;

    temp_buffer = strdup (*ruby_buffer_output);
    if (!temp_buffer)
        return;

    weechat_string_dyn_copy (ruby_buffer_output, NULL);

    if (ruby_eval_mode)
    {
        if (ruby_eval_send_input)
        {
            if (ruby_eval_exec_commands)
                ptr_command = temp_buffer;
            else
                ptr_command = weechat_string_input_for_buffer (temp_buffer);
            if (ptr_command)
            {
                weechat_command (ruby_eval_buffer, temp_buffer);
            }
            else
            {
                if (weechat_asprintf (&command,
                                      "%c%s",
                                      temp_buffer[0], temp_buffer) >= 0)
                {
                    weechat_command (ruby_eval_buffer,
                                     (command[0]) ? command : " ");
                    free (command);
                }
            }
        }
        else
        {
            weechat_printf (ruby_eval_buffer, "%s", temp_buffer);
        }
    }
    else
    {
        /* script (no eval mode) */
        weechat_printf (
            NULL,
            weechat_gettext ("%s: stdout/stderr (%s): %s"),
            RUBY_PLUGIN_NAME,
            (ruby_current_script) ? ruby_current_script->name : "?",
            temp_buffer);
    }

    free (temp_buffer);
}

/*
 * Redirection for stdout and stderr.
 */

static VALUE
weechat_ruby_output (VALUE self, VALUE str)
{
    char *msg, *ptr_msg, *ptr_newline;

    /* make C compiler happy */
    (void) self;

    msg = strdup (StringValuePtr (str));

    ptr_msg = msg;
    while ((ptr_newline = strchr (ptr_msg, '\n')) != NULL)
    {
        weechat_string_dyn_concat (ruby_buffer_output,
                                   ptr_msg,
                                   ptr_newline - ptr_msg);
        weechat_ruby_output_flush ();
        ptr_msg = ++ptr_newline;
    }
    weechat_string_dyn_concat (ruby_buffer_output, ptr_msg, -1);

    free (msg);

    return Qnil;
}

/*
 * Executes a ruby function.
 */

void *
weechat_ruby_exec (struct t_plugin_script *script,
                   int ret_type, const char *function,
                   const char *format, void **argv)
{
    VALUE rc, err;
    int ruby_error, i, argc, *ret_i;
    VALUE argv2[16];
    void *ret_value;
    struct t_plugin_script *old_ruby_current_script;

    ret_value = NULL;

    old_ruby_current_script = ruby_current_script;
    ruby_current_script = script;

    argc = 0;
    if (format && format[0])
    {
        argc = strlen (format);
        for (i = 0; i < argc; i++)
        {
            switch (format[i])
            {
                case 's': /* string or null */
                    if (argv[i])
                        argv2[i] = rb_str_new2 ((char *)argv[i]);
                    else
                        argv2[i] = Qnil;
                    break;
                case 'i': /* integer */
                    argv2[i] = INT2FIX (*((int *)argv[i]));
                    break;
                case 'h': /* hash */
                    argv2[i] = weechat_ruby_hashtable_to_hash (argv[i]);
                    break;
            }
        }
    }

    if (argc > 0)
    {
        rc = rb_protect_funcall ((VALUE) script->interpreter,
                                 rb_intern (function),
                                 &ruby_error, argc, argv2);
    }
    else
    {
        rc = rb_protect_funcall ((VALUE) script->interpreter,
                                 rb_intern (function),
                                 &ruby_error, 0, NULL);
    }

    weechat_ruby_output_flush ();

    if (ruby_error)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to run function \"%s\""),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME, function);

        err = rb_gv_get ("$!");
        weechat_ruby_print_exception (err);

        return NULL;
    }

    if ((TYPE(rc) == T_STRING) && (ret_type == WEECHAT_SCRIPT_EXEC_STRING))
    {
        if (StringValuePtr (rc))
            ret_value = strdup (StringValuePtr (rc));
        else
            ret_value = NULL;
    }
    else if ((TYPE(rc) == T_STRING) && (ret_type == WEECHAT_SCRIPT_EXEC_POINTER))
    {
        if (StringValuePtr (rc))
        {
            ret_value = plugin_script_str2ptr (weechat_ruby_plugin,
                                               script->name, function,
                                               StringValuePtr (rc));
        }
        else
        {
            ret_value = NULL;
        }
    }
    else if ((TYPE(rc) == T_FIXNUM) && (ret_type == WEECHAT_SCRIPT_EXEC_INT))
    {
        ret_i = malloc (sizeof (*ret_i));
        if (ret_i)
            *ret_i = NUM2INT(rc);
        ret_value = ret_i;
    }
    else if (ret_type == WEECHAT_SCRIPT_EXEC_HASHTABLE)
    {
        ret_value = weechat_ruby_hash_to_hashtable (rc,
                                                    WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                    WEECHAT_HASHTABLE_STRING,
                                                    WEECHAT_HASHTABLE_STRING);
    }
    else
    {
        if (ret_type != WEECHAT_SCRIPT_EXEC_IGNORE)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: function \"%s\" must "
                                             "return a valid value"),
                            weechat_prefix ("error"), RUBY_PLUGIN_NAME,
                            function);
        }
    }

    if ((ret_type != WEECHAT_SCRIPT_EXEC_IGNORE) && !ret_value)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error in function \"%s\""),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME, function);
    }

    ruby_current_script = old_ruby_current_script;

    return ret_value;
}

/*
 * Loads a ruby script.
 *
 * If code is NULL, the content of filename is read and executed.
 * If code is not NULL, it is executed (the file is not read).
 *
 * Returns pointer to new registered script, NULL if error.
 */

struct t_plugin_script *
weechat_ruby_load (const char *filename, const char *code)
{
    char modname[64];
    VALUE ruby_retcode, err, argv[2];
    int ruby_error;
    struct stat buf;

    if (!code)
    {
        if (stat (filename, &buf) != 0)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: script \"%s\" not found"),
                            weechat_prefix ("error"), RUBY_PLUGIN_NAME,
                            filename);
            return NULL;
        }
    }

    if ((weechat_ruby_plugin->debug >= 2) || !ruby_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: loading script \"%s\""),
                        RUBY_PLUGIN_NAME, filename);
    }

    ruby_current_script = NULL;
    ruby_registered_script = NULL;

    snprintf (modname, sizeof (modname), "%s%d", MOD_NAME_PREFIX, ruby_num);
    ruby_num++;

    ruby_current_module = rb_define_module (modname);

    ruby_current_script_filename = filename;

    argv[0] = rb_str_new2 (filename);
    argv[1] = rb_str_new2 ((code) ? code : "");
    ruby_retcode = rb_protect_funcall (ruby_current_module,
                                       rb_intern ("load_eval_file"),
                                       &ruby_error, 2, argv);

    if (ruby_retcode == Qnil)
    {
        err = rb_gv_get ("$!");
        weechat_ruby_print_exception (err);
        return NULL;
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

        if (NUM2INT(ruby_retcode) == 2)
        {
            weechat_ruby_print_exception (rb_iv_get (ruby_current_module,
                                                     "@load_eval_file_error"));
        }

        return NULL;
    }

    (void) rb_protect_funcall (ruby_current_module, rb_intern ("weechat_init"),
                               &ruby_error, 0, NULL);

    if (ruby_error)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to eval function "
                                         "\"weechat_init\" in file \"%s\""),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME, filename);

        err = rb_gv_get ("$!");
        weechat_ruby_print_exception (err);

        if (ruby_current_script)
        {
            plugin_script_remove (weechat_ruby_plugin,
                                  &ruby_scripts, &last_ruby_script,
                                  ruby_current_script);
            ruby_current_script = NULL;
        }

        return NULL;
    }

    if (!ruby_registered_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME, filename);
        return NULL;
    }
    ruby_current_script = ruby_registered_script;

    rb_gc_register_address (ruby_current_script->interpreter);

    /*
     * set input/close callbacks for buffers created by this script
     * (to restore callbacks after upgrade)
     */
    plugin_script_set_buffer_callbacks (weechat_ruby_plugin,
                                        ruby_scripts,
                                        ruby_current_script,
                                        &weechat_ruby_api_buffer_input_data_cb,
                                        &weechat_ruby_api_buffer_close_cb);

    (void) weechat_hook_signal_send ("ruby_script_loaded",
                                     WEECHAT_HOOK_SIGNAL_STRING,
                                     ruby_current_script->filename);

    return ruby_current_script;
}

/*
 * Callback for weechat_script_auto_load() function.
 */

void
weechat_ruby_load_cb (void *data, const char *filename)
{
    const char *pos_dot;

    /* make C compiler happy */
    (void) data;

    pos_dot = strrchr (filename, '.');
    if (pos_dot && (strcmp (pos_dot, ".rb") == 0))
        weechat_ruby_load (filename, NULL);
}

/*
 * Unloads a ruby script.
 */

void
weechat_ruby_unload (struct t_plugin_script *script)
{
    int *rc;
    void *interpreter;
    char *filename;

    if ((weechat_ruby_plugin->debug >= 2) || !ruby_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: unloading script \"%s\""),
                        RUBY_PLUGIN_NAME, script->name);
    }

    if (script->shutdown_func && script->shutdown_func[0])
    {
        rc = (int *)weechat_ruby_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script->shutdown_func,
                                       0, NULL);
        free (rc);
    }

    filename = strdup (script->filename);
    interpreter = script->interpreter;

    if (ruby_current_script == script)
        ruby_current_script = (ruby_current_script->prev_script) ?
            ruby_current_script->prev_script : ruby_current_script->next_script;

    plugin_script_remove (weechat_ruby_plugin, &ruby_scripts, &last_ruby_script,
                          script);

    if (interpreter)
        rb_gc_unregister_address (interpreter);

    (void) weechat_hook_signal_send ("ruby_script_unloaded",
                                     WEECHAT_HOOK_SIGNAL_STRING, filename);
    free (filename);
}

/*
 * Unloads a ruby script by name.
 */

void
weechat_ruby_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = plugin_script_search (ruby_scripts, name);
    if (ptr_script)
    {
        weechat_ruby_unload (ptr_script);
        if (!ruby_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: script \"%s\" unloaded"),
                            RUBY_PLUGIN_NAME, name);
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
 * Reloads a ruby script by name.
 */

void
weechat_ruby_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;

    ptr_script = plugin_script_search (ruby_scripts, name);
    if (ptr_script)
    {
        filename = strdup (ptr_script->filename);
        if (filename)
        {
            weechat_ruby_unload (ptr_script);
            if (!ruby_quiet)
            {
                weechat_printf (NULL,
                                weechat_gettext ("%s: script \"%s\" unloaded"),
                                RUBY_PLUGIN_NAME, name);
            }
            weechat_ruby_load (filename, NULL);
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
 * Unloads all ruby scripts.
 */

void
weechat_ruby_unload_all (void)
{
    while (ruby_scripts)
    {
        weechat_ruby_unload (ruby_scripts);
    }
}

/*
 * Evaluates ruby source code.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
weechat_ruby_eval (struct t_gui_buffer *buffer, int send_to_buffer_as_input,
                   int exec_commands, const char *code)
{
    void *func_argv[1], *result;
    char empty_arg[1] = { '\0' };
    int old_ruby_quiet;

    if (!ruby_script_eval)
    {
        old_ruby_quiet = ruby_quiet;
        ruby_quiet = 1;
        ruby_script_eval = weechat_ruby_load (WEECHAT_SCRIPT_EVAL_NAME,
                                              RUBY_EVAL_SCRIPT);
        ruby_quiet = old_ruby_quiet;
        if (!ruby_script_eval)
            return 0;
    }

    weechat_ruby_output_flush ();

    ruby_eval_mode = 1;
    ruby_eval_send_input = send_to_buffer_as_input;
    ruby_eval_exec_commands = exec_commands;
    ruby_eval_buffer = buffer;

    func_argv[0] = (code) ? (char *)code : empty_arg;
    result = weechat_ruby_exec (ruby_script_eval,
                                WEECHAT_SCRIPT_EXEC_IGNORE,
                                "script_ruby_eval",
                                "s", func_argv);
    /* result is ignored */
    free (result);

    weechat_ruby_output_flush ();

    ruby_eval_mode = 0;
    ruby_eval_send_input = 0;
    ruby_eval_exec_commands = 0;
    ruby_eval_buffer = NULL;

    if (!weechat_config_boolean (ruby_config_look_eval_keep_context))
    {
        old_ruby_quiet = ruby_quiet;
        ruby_quiet = 1;
        weechat_ruby_unload (ruby_script_eval);
        ruby_quiet = old_ruby_quiet;
        ruby_script_eval = NULL;
    }

    return 1;
}

/*
 * Callback for command "/ruby".
 */

int
weechat_ruby_command_cb (const void *pointer, void *data,
                         struct t_gui_buffer *buffer,
                         int argc, char **argv, char **argv_eol)
{
    char *ptr_name, *ptr_code, *path_script;
    int i, send_to_buffer_as_input, exec_commands, old_ruby_quiet;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (argc == 1)
    {
        plugin_script_display_list (weechat_ruby_plugin, ruby_scripts,
                                    NULL, 0);
    }
    else if (argc == 2)
    {
        if (weechat_strcmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_ruby_plugin, ruby_scripts,
                                        NULL, 0);
        }
        else if (weechat_strcmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_ruby_plugin, ruby_scripts,
                                        NULL, 1);
        }
        else if (weechat_strcmp (argv[1], "autoload") == 0)
        {
            plugin_script_auto_load (weechat_ruby_plugin, &weechat_ruby_load_cb);
        }
        else if (weechat_strcmp (argv[1], "reload") == 0)
        {
            weechat_ruby_unload_all ();
            plugin_script_auto_load (weechat_ruby_plugin, &weechat_ruby_load_cb);
        }
        else if (weechat_strcmp (argv[1], "unload") == 0)
        {
            weechat_ruby_unload_all ();
        }
        else if (weechat_strcmp (argv[1], "version") == 0)
        {
            plugin_script_display_interpreter (weechat_ruby_plugin, 0);
        }
        else
            WEECHAT_COMMAND_ERROR;
    }
    else
    {
        if (weechat_strcmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_ruby_plugin, ruby_scripts,
                                        argv_eol[2], 0);
        }
        else if (weechat_strcmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_ruby_plugin, ruby_scripts,
                                        argv_eol[2], 1);
        }
        else if ((weechat_strcmp (argv[1], "load") == 0)
                 || (weechat_strcmp (argv[1], "reload") == 0)
                 || (weechat_strcmp (argv[1], "unload") == 0))
        {
            old_ruby_quiet = ruby_quiet;
            ptr_name = argv_eol[2];
            if (strncmp (ptr_name, "-q ", 3) == 0)
            {
                ruby_quiet = 1;
                ptr_name += 3;
                while (ptr_name[0] == ' ')
                {
                    ptr_name++;
                }
            }
            if (weechat_strcmp (argv[1], "load") == 0)
            {
                /* load ruby script */
                path_script = plugin_script_search_path (weechat_ruby_plugin,
                                                         ptr_name, 1);
                weechat_ruby_load ((path_script) ? path_script : ptr_name,
                                   NULL);
                free (path_script);
            }
            else if (weechat_strcmp (argv[1], "reload") == 0)
            {
                /* reload one ruby script */
                weechat_ruby_reload_name (ptr_name);
            }
            else if (weechat_strcmp (argv[1], "unload") == 0)
            {
                /* unload ruby script */
                weechat_ruby_unload_name (ptr_name);
            }
            ruby_quiet = old_ruby_quiet;
        }
        else if (weechat_strcmp (argv[1], "eval") == 0)
        {
            send_to_buffer_as_input = 0;
            exec_commands = 0;
            ptr_code = argv_eol[2];
            for (i = 2; i < argc; i++)
            {
                if (argv[i][0] == '-')
                {
                    if (strcmp (argv[i], "-o") == 0)
                    {
                        if (i + 1 >= argc)
                            WEECHAT_COMMAND_ERROR;
                        send_to_buffer_as_input = 1;
                        exec_commands = 0;
                        ptr_code = argv_eol[i + 1];
                    }
                    else if (strcmp (argv[i], "-oc") == 0)
                    {
                        if (i + 1 >= argc)
                            WEECHAT_COMMAND_ERROR;
                        send_to_buffer_as_input = 1;
                        exec_commands = 1;
                        ptr_code = argv_eol[i + 1];
                    }
                }
                else
                    break;
            }
            if (!weechat_ruby_eval (buffer, send_to_buffer_as_input,
                                    exec_commands, ptr_code))
                WEECHAT_COMMAND_ERROR;
        }
        else
            WEECHAT_COMMAND_ERROR;
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds ruby scripts to completion list.
 */

int
weechat_ruby_completion_cb (const void *pointer, void *data,
                            const char *completion_item,
                            struct t_gui_buffer *buffer,
                            struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    plugin_script_completion (weechat_ruby_plugin, completion, ruby_scripts);

    return WEECHAT_RC_OK;
}

/*
 * Returns hdata for ruby scripts.
 */

struct t_hdata *
weechat_ruby_hdata_cb (const void *pointer, void *data,
                       const char *hdata_name)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    return plugin_script_hdata_script (weechat_plugin,
                                       &ruby_scripts, &last_ruby_script,
                                       hdata_name);
}

/*
 * Returns ruby info "ruby_eval".
 */

char *
weechat_ruby_info_eval_cb (const void *pointer, void *data,
                           const char *info_name,
                           const char *arguments)
{
    char *output;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    weechat_ruby_eval (NULL, 0, 0, (arguments) ? arguments : "");
    output = strdup (*ruby_buffer_output);
    weechat_string_dyn_copy (ruby_buffer_output, NULL);

    return output;
}

/*
 * Returns infolist with ruby scripts.
 */

struct t_infolist *
weechat_ruby_infolist_cb (const void *pointer, void *data,
                          const char *infolist_name,
                          void *obj_pointer, const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (strcmp (infolist_name, "ruby_script") == 0)
    {
        return plugin_script_infolist_list_scripts (weechat_ruby_plugin,
                                                    ruby_scripts, obj_pointer,
                                                    arguments);
    }

    return NULL;
}

/*
 * Dumps ruby plugin data in WeeChat log file.
 */

int
weechat_ruby_signal_debug_dump_cb (const void *pointer, void *data,
                                   const char *signal,
                                   const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data || (strcmp ((char *)signal_data, RUBY_PLUGIN_NAME) == 0))
    {
        plugin_script_print_log (weechat_ruby_plugin, ruby_scripts);
    }

    return WEECHAT_RC_OK;
}

/*
 * Timer for executing actions.
 */

int
weechat_ruby_timer_action_cb (const void *pointer, void *data,
                              int remaining_calls)
{
    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    if (pointer)
    {
        if (pointer == &ruby_action_install_list)
        {
            plugin_script_action_install (weechat_ruby_plugin,
                                          ruby_scripts,
                                          &weechat_ruby_unload,
                                          &weechat_ruby_load,
                                          &ruby_quiet,
                                          &ruby_action_install_list);
        }
        else if (pointer == &ruby_action_remove_list)
        {
            plugin_script_action_remove (weechat_ruby_plugin,
                                         ruby_scripts,
                                         &weechat_ruby_unload,
                                         &ruby_quiet,
                                         &ruby_action_remove_list);
        }
        else if (pointer == &ruby_action_autoload_list)
        {
            plugin_script_action_autoload (weechat_ruby_plugin,
                                           &ruby_quiet,
                                           &ruby_action_autoload_list);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback called when a script action is asked (install/remove a script).
 */

int
weechat_ruby_signal_script_action_cb (const void *pointer, void *data,
                                      const char *signal,
                                      const char *type_data,
                                      void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "ruby_script_install") == 0)
        {
            plugin_script_action_add (&ruby_action_install_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_ruby_timer_action_cb,
                                &ruby_action_install_list, NULL);
        }
        else if (strcmp (signal, "ruby_script_remove") == 0)
        {
            plugin_script_action_add (&ruby_action_remove_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_ruby_timer_action_cb,
                                &ruby_action_remove_list, NULL);
        }
        else if (strcmp (signal, "ruby_script_autoload") == 0)
        {
            plugin_script_action_add (&ruby_action_autoload_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_ruby_timer_action_cb,
                                &ruby_action_autoload_list, NULL);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Initializes ruby plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    int ruby_error, old_ruby_quiet;
    VALUE err;
    char* ruby_options_argv[] = { "ruby", "-enil", NULL };
    char *weechat_ruby_code = {
        "$stdout = WeechatOutputs\n"
        "$stderr = WeechatOutputs\n"
        "begin"
        "  if RUBY_VERSION.split('.')[0] == '1' and RUBY_VERSION.split('.')[1] <= '8'\n"
        "    require 'rubygems'\n"
        "  else\n"
        "    require 'thread'\n"
        "    class ::Mutex\n"
        "      def synchronize(*args)\n"
        "        yield\n"
        "      end\n"
        "    end\n"
        "    require 'rubygems'\n"
        "  end\n"
        "rescue LoadError\n"
        "end\n"
        "\n"
        "class Module\n"
        "\n"
        "  def load_eval_file (file, code)\n"
        "    if !code.empty?\n"
        "      lines = code\n"
        "    else\n"
        "      lines = ''\n"
        "      begin\n"
        "        lines = File.read(file)\n"
        "      rescue => e\n"
        "        return 1\n"
        "      end\n"
        "    end\n"
        "\n"
        "    begin\n"
        "      require 'enc/encdb.so'\n"
        "      require 'enc/trans/transdb.so'\n"
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
        "      module_eval('module_function :' + meth.to_s)\n"
        "    end\n"
        "\n"
        "    unless has_init\n"
        "      return 3\n"
        "    end\n"
        "\n"
        "    return 0\n"
        "  end\n"
        "\n"
        "  def eval_code (code)\n"
        "    module_eval(code)\n"
        "  end\n"
        "end\n"
    };

    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_ruby_plugin = plugin;

    ruby_quiet = 0;
    ruby_eval_mode = 0;
    ruby_eval_send_input = 0;
    ruby_eval_exec_commands = 0;

    /* set interpreter name and version */
    weechat_hashtable_set (plugin->variables, "interpreter_name",
                           plugin->name);
#ifdef HAVE_RUBY_VERSION_H
    weechat_hashtable_set (plugin->variables, "interpreter_version",
                           ruby_version);
#else
    weechat_hashtable_set (plugin->variables, "interpreter_version",
                           "");
#endif /* HAVE_RUBY_VERSION_H */

    ruby_error = 0;

    /* init stdout/stderr buffer */
    ruby_buffer_output = weechat_string_dyn_alloc (256);
    if (!ruby_buffer_output)
        return WEECHAT_RC_ERROR;

#if (defined(RUBY_API_VERSION_MAJOR) && defined(RUBY_API_VERSION_MINOR)) && (RUBY_API_VERSION_MAJOR >= 2 || (RUBY_API_VERSION_MAJOR == 1 && RUBY_API_VERSION_MINOR >= 9))
    RUBY_INIT_STACK;
#endif

    ruby_init ();

    ruby_options (2, ruby_options_argv);

    /* redirect stdin and stdout */
    ruby_mWeechatOutputs = rb_define_module ("WeechatOutputs");
    rb_define_singleton_method (ruby_mWeechatOutputs, "write",
                                weechat_ruby_output, 1);
    rb_define_singleton_method (ruby_mWeechatOutputs, "puts",
                                weechat_ruby_output, 1);
    rb_define_singleton_method (ruby_mWeechatOutputs, "p",
                                weechat_ruby_output, 1);
    rb_define_singleton_method (ruby_mWeechatOutputs, "flush",
                                weechat_ruby_output_flush_ruby, 0);

    ruby_script ("__weechat_plugin__");

    ruby_mWeechat = rb_define_module ("Weechat");
    weechat_ruby_api_init (ruby_mWeechat);

    rb_eval_string_protect (weechat_ruby_code, &ruby_error);
    if (ruby_error)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to eval WeeChat ruby "
                                         "internal code"),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME);
        err = rb_gv_get ("$!");
        weechat_ruby_print_exception (err);
        weechat_string_dyn_free (ruby_buffer_output, 1);
        return WEECHAT_RC_ERROR;
    }

    ruby_init_loadpath ();

    ruby_data.config_file = &ruby_config_file;
    ruby_data.config_look_check_license = &ruby_config_look_check_license;
    ruby_data.config_look_eval_keep_context = &ruby_config_look_eval_keep_context;
    ruby_data.scripts = &ruby_scripts;
    ruby_data.last_script = &last_ruby_script;
    ruby_data.callback_command = &weechat_ruby_command_cb;
    ruby_data.callback_completion = &weechat_ruby_completion_cb;
    ruby_data.callback_hdata = &weechat_ruby_hdata_cb;
    ruby_data.callback_info_eval = &weechat_ruby_info_eval_cb;
    ruby_data.callback_infolist = &weechat_ruby_infolist_cb;
    ruby_data.callback_signal_debug_dump = &weechat_ruby_signal_debug_dump_cb;
    ruby_data.callback_signal_script_action = &weechat_ruby_signal_script_action_cb;
    ruby_data.callback_load_file = &weechat_ruby_load_cb;
    ruby_data.init_before_autoload = NULL;
    ruby_data.unload_all = &weechat_ruby_unload_all;

    old_ruby_quiet = ruby_quiet;
    ruby_quiet = 1;
    plugin_script_init (weechat_ruby_plugin, &ruby_data);
    ruby_quiet = old_ruby_quiet;

    plugin_script_display_short_list (weechat_ruby_plugin,
                                      ruby_scripts);

    /* init OK */
    return WEECHAT_RC_OK;
}

/*
 * Ends ruby plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    int old_ruby_quiet;

    /* unload all scripts */
    old_ruby_quiet = ruby_quiet;
    ruby_quiet = 1;
    if (ruby_script_eval)
    {
        weechat_ruby_unload (ruby_script_eval);
        ruby_script_eval = NULL;
    }
    plugin_script_end (plugin, &ruby_data);
    ruby_quiet = old_ruby_quiet;

    ruby_cleanup (0);
    signal (SIGCHLD, SIG_DFL);

    /* free some data */
    if (ruby_action_install_list)
    {
        free (ruby_action_install_list);
        ruby_action_install_list = NULL;
    }
    if (ruby_action_remove_list)
    {
        free (ruby_action_remove_list);
        ruby_action_remove_list = NULL;
    }
    if (ruby_action_autoload_list)
    {
        free (ruby_action_autoload_list);
        ruby_action_autoload_list = NULL;
    }
    weechat_string_dyn_free (ruby_buffer_output, 1);
    ruby_buffer_output = NULL;

    return WEECHAT_RC_OK;
}
