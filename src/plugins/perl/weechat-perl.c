/*
 * weechat-perl.c - perl plugin for WeeChat
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005-2008 Emmanuel Bouthenot <kolter@openics.org>
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

#include <locale.h>
#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

#include "../weechat-plugin.h"
#include "../plugin-script.h"
#include "weechat-perl.h"
#include "weechat-perl-api.h"


WEECHAT_PLUGIN_NAME(PERL_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Support of perl scripts"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(PERL_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_perl_plugin = NULL;

struct t_plugin_script_data perl_data;

struct t_config_file *perl_config_file = NULL;
struct t_config_option *perl_config_look_check_license = NULL;
struct t_config_option *perl_config_look_eval_keep_context = NULL;

int perl_quiet = 0;

struct t_plugin_script *perl_script_eval = NULL;
int perl_eval_mode = 0;
int perl_eval_send_input = 0;
int perl_eval_exec_commands = 0;
struct t_gui_buffer *perl_eval_buffer = NULL;
#define PERL_EVAL_SCRIPT                                                \
    "sub script_perl_eval {\n"                                          \
    "    eval \"$_[0]\";\n"                                             \
    "}\n"                                                               \
    "weechat::register('" WEECHAT_SCRIPT_EVAL_NAME "', '', '1.0', "     \
    "'" WEECHAT_LICENSE "', 'Evaluation of source code', '', '');\n"

struct t_plugin_script *perl_scripts = NULL;
struct t_plugin_script *last_perl_script = NULL;
struct t_plugin_script *perl_current_script = NULL;
struct t_plugin_script *perl_registered_script = NULL;
const char *perl_current_script_filename = NULL;
#ifdef MULTIPLICITY
PerlInterpreter *perl_current_interpreter = NULL;
#endif /* MULTIPLICITY */
int perl_quit_or_upgrade = 0;
char **perl_buffer_output = NULL;

/*
 * string used to execute action "install":
 * when signal "perl_script_install" is received, name of string
 * is added to this string, to be installed later by a timer (when nothing is
 * running in script)
 */
char *perl_action_install_list = NULL;

/*
 * string used to execute action "remove":
 * when signal "perl_script_remove" is received, name of string
 * is added to this string, to be removed later by a timer (when nothing is
 * running in script)
 */
char *perl_action_remove_list = NULL;

/*
 * string used to execute action "autoload":
 * when signal "perl_script_autoload" is received, name of string
 * is added to this string, to autoload or disable autoload later by a timer
 * (when nothing is running in script)
 */
char *perl_action_autoload_list = NULL;

#ifdef NO_PERL_MULTIPLICITY
#undef MULTIPLICITY
#endif /* NO_PERL_MULTIPLICITY */

#ifndef MULTIPLICITY
#define PKG_NAME_PREFIX "WeechatPerlPackage"
static PerlInterpreter *perl_main = NULL;
int perl_num = 0;
#endif /* MULTIPLICITY */

char *perl_args[] = { "", "-e", "0", "-w", NULL };
int perl_args_count = 4;

char *perl_weechat_code =
{
#ifndef MULTIPLICITY
    "package %s;"
#endif /* MULTIPLICITY */
    "$SIG{__WARN__} = sub { weechat::print('', '%s '.$_[0]); };"
    "$SIG{__DIE__} = sub { weechat::print('', '%s '.$_[0]); };"
    "tie(*STDOUT, 'weechat_output');"
    "tie(*STDERR, 'weechat_output');"
    "do %s%s%s"
    "package weechat_output;"
    "sub TIEHANDLE { bless {}; }"
    "sub PRINT {"
    "  weechat::__output__($_[1]);"
    "}"
    "sub PRINTF {"
    "  my $self = shift;"
    "  my $fmt = shift;"
    "  weechat::__output__(sprintf($fmt, @_));"
    "}"
};

/*
 * Callback called for each key/value in a hashtable.
 */

void
weechat_perl_hashtable_map_cb (void *data,
                               struct t_hashtable *hashtable,
                               const char *key,
                               const char *value)
{
    HV *hash;

    /* make C compiler happy */
    (void) hashtable;

    hash = (HV *)data;

    (void) hv_store (hash, key, strlen (key), newSVpv (value, 0), 0);
}

/*
 * Converts a WeeChat hashtable to a perl hash.
 */

HV *
weechat_perl_hashtable_to_hash (struct t_hashtable *hashtable)
{
    HV *hash;

    hash = (HV *)newHV ();
    if (!hash)
        return NULL;

    weechat_hashtable_map_string (hashtable, &weechat_perl_hashtable_map_cb,
                                  hash);

    return hash;
}

/*
 * Converts a perl hash to a WeeChat hashtable.
 *
 * Note: hashtable must be freed after use.
 */

struct t_hashtable *
weechat_perl_hash_to_hashtable (SV *hash, int size, const char *type_keys,
                                const char *type_values)
{
    struct t_hashtable *hashtable;
    HV *hash2;
    SV *value;
    char *str_key;
    I32 retlen;

    hashtable = weechat_hashtable_new (size, type_keys, type_values,
                                       NULL, NULL);
    if (!hashtable)
        return NULL;

    if ((hash) && SvROK(hash) && SvRV(hash)
        && (SvTYPE(SvRV(hash)) == SVt_PVHV))
    {
        hash2 = (HV *)SvRV(hash);
        hv_iterinit (hash2);
        while ((value = hv_iternextsv (hash2, &str_key, &retlen)))
        {
            if (strcmp (type_values, WEECHAT_HASHTABLE_STRING) == 0)
            {
                weechat_hashtable_set (hashtable, str_key,
                                       SvPV (value, PL_na));
            }
            else if (strcmp (type_values, WEECHAT_HASHTABLE_POINTER) == 0)
            {
                weechat_hashtable_set (hashtable, str_key,
                                       plugin_script_str2ptr (
                                           weechat_perl_plugin,
                                           NULL, NULL,
                                           SvPV (value, PL_na)));
            }
        }
    }

    return hashtable;
}

/*
 * Flushes output.
 */

void
weechat_perl_output_flush ()
{
    const char *ptr_command;
    char *temp_buffer, *command;
    int length;

    if (!*perl_buffer_output[0])
        return;

    /* if there's no buffer, we catch the output, so there's no flush */
    if (perl_eval_mode && !perl_eval_buffer)
        return;

    temp_buffer = strdup (*perl_buffer_output);
    if (!temp_buffer)
        return;

    weechat_string_dyn_copy (perl_buffer_output, NULL);

    if (perl_eval_mode)
    {
        if (perl_eval_send_input)
        {
            if (perl_eval_exec_commands)
                ptr_command = temp_buffer;
            else
                ptr_command = weechat_string_input_for_buffer (temp_buffer);
            if (ptr_command)
            {
                weechat_command (perl_eval_buffer, temp_buffer);
            }
            else
            {
                length = 1 + strlen (temp_buffer) + 1;
                command = malloc (length);
                if (command)
                {
                    snprintf (command, length, "%c%s",
                              temp_buffer[0], temp_buffer);
                    weechat_command (perl_eval_buffer,
                                     (command[0]) ? command : " ");
                    free (command);
                }
            }
        }
        else
        {
            weechat_printf (perl_eval_buffer, "%s", temp_buffer);
        }
    }
    else
    {
        /* script (no eval mode) */
        weechat_printf (
            NULL,
            weechat_gettext ("%s: stdout/stderr (%s): %s"),
            PERL_PLUGIN_NAME,
            (perl_current_script) ? perl_current_script->name : "?",
            temp_buffer);
    }

    free (temp_buffer);
}

/*
 * Redirection for stdout and stderr.
 */

XS (weechat_perl_output)
{
    char *msg, *ptr_msg, *ptr_newline;
    dXSARGS;

    if (items < 1)
        return;

    msg = SvPV_nolen (ST (0));
    ptr_msg = msg;
    while ((ptr_newline = strchr (ptr_msg, '\n')) != NULL)
    {
        weechat_string_dyn_concat (perl_buffer_output,
                                   ptr_msg,
                                   ptr_newline - ptr_msg);
        weechat_perl_output_flush ();
        ptr_msg = ++ptr_newline;
    }
    weechat_string_dyn_concat (perl_buffer_output, ptr_msg, -1);
}

/*
 * Executes a perl function.
 */

void *
weechat_perl_exec (struct t_plugin_script *script,
                   int ret_type, const char *function,
                   const char *format, void **argv)
{
    char *func;
    unsigned int count;
    void *ret_value;
    int *ret_i, mem_err, length, i, argc;
    SV *ret_s;
    HV *hash;
    struct t_plugin_script *old_perl_current_script;
#ifdef MULTIPLICITY
    void *old_context;
#endif /* MULTIPLICITY */

    old_perl_current_script = perl_current_script;
    perl_current_script = script;

#ifdef MULTIPLICITY
    (void) length;
    func = (char *) function;
    old_context = PERL_GET_CONTEXT;
    if (script->interpreter)
        PERL_SET_CONTEXT (script->interpreter);
#else
    length = strlen ((script->interpreter) ? script->interpreter : perl_main) +
        strlen (function) + 3;
    func = (char *) malloc (length);
    if (!func)
        return NULL;
    snprintf (func, length, "%s::%s",
              (char *) ((script->interpreter) ? script->interpreter : perl_main),
              function);
#endif /* MULTIPLICITY */

    dSP;
    ENTER;
    SAVETMPS;

    PUSHMARK(SP);
    if (format && format[0])
    {
        argc = strlen (format);
        for (i = 0; i < argc; i++)
        {
            switch (format[i])
            {
                case 's': /* string or null */
                    if (argv[i])
                        XPUSHs (sv_2mortal (newSVpv((char *)argv[i], 0)));
                    else
                        XPUSHs (sv_2mortal (&PL_sv_undef));
                    break;
                case 'i': /* integer */
                    XPUSHs (sv_2mortal (newSViv (*((int *)argv[i]))));
                    break;
                case 'h': /* hash */
                    hash = weechat_perl_hashtable_to_hash (argv[i]);
                    XPUSHs (sv_2mortal (newRV_inc ((SV *)hash)));
                    break;
            }
        }
    }
    PUTBACK;
    count = call_pv (func, G_EVAL | G_SCALAR);

    ret_value = NULL;
    mem_err = 1;

    SPAGAIN;

    weechat_perl_output_flush ();

    if (SvTRUE (ERRSV))
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error: %s"),
                        weechat_prefix ("error"), PERL_PLUGIN_NAME,
                        SvPV_nolen (ERRSV));
        (void) POPs;  /* pop the "undef" */
        mem_err = 0;
    }
    else
    {
        if (count != 1)
        {
            if (ret_type != WEECHAT_SCRIPT_EXEC_IGNORE)
            {
                weechat_printf (NULL,
                                weechat_gettext ("%s%s: function \"%s\" must "
                                                 "return a valid value"),
                                weechat_prefix ("error"), PERL_PLUGIN_NAME,
                                function);
            }
            mem_err = 0;
        }
        else
        {
            if (ret_type == WEECHAT_SCRIPT_EXEC_STRING)
            {
                ret_s = newSVsv (POPs);
                ret_value = strdup (SvPV_nolen (ret_s));
                SvREFCNT_dec (ret_s);
            }
            else if (ret_type == WEECHAT_SCRIPT_EXEC_POINTER)
            {
                ret_s = newSVsv (POPs);
                ret_value = plugin_script_str2ptr (weechat_perl_plugin,
                                                   script->name, function,
                                                   SvPV_nolen (ret_s));
                SvREFCNT_dec (ret_s);
            }
            else if (ret_type == WEECHAT_SCRIPT_EXEC_INT)
            {
                ret_i = malloc (sizeof (*ret_i));
                if (ret_i)
                    *ret_i = POPi;
                ret_value = ret_i;
            }
            else if (ret_type == WEECHAT_SCRIPT_EXEC_HASHTABLE)
            {
                ret_value = weechat_perl_hash_to_hashtable (POPs,
                                                            WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                            WEECHAT_HASHTABLE_STRING,
                                                            WEECHAT_HASHTABLE_STRING);
            }
            else
            {
                if (ret_type != WEECHAT_SCRIPT_EXEC_IGNORE)
                {
                    weechat_printf (
                        NULL,
                        weechat_gettext ("%s%s: function \"%s\" must return "
                                         "a valid value"),
                        weechat_prefix ("error"), PERL_PLUGIN_NAME,
                        function);
                }
                mem_err = 0;
            }
        }
    }

    if ((ret_type != WEECHAT_SCRIPT_EXEC_IGNORE) && !ret_value)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error in function \"%s\""),
                        weechat_prefix ("error"), PERL_PLUGIN_NAME, function);
    }

    PUTBACK;
    FREETMPS;
    LEAVE;

    perl_current_script = old_perl_current_script;
#ifdef MULTIPLICITY
    PERL_SET_CONTEXT (old_context);
#else
    free (func);
#endif /* MULTIPLICITY */

    if (!ret_value && (mem_err == 1))
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: not enough memory in function "
                                         "\"%s\""),
                        weechat_prefix ("error"), PERL_PLUGIN_NAME, function);
        return NULL;
    }

    return ret_value;
}

/*
 * Loads a perl script.
 *
 * If code is NULL, the content of filename is read and executed.
 * If code is not NULL, it is executed (the file is not read).
 *
 * Returns pointer to new registered script, NULL if error.
 */

struct t_plugin_script *
weechat_perl_load (const char *filename, const char *code)
{
    char str_warning[512], str_error[512];

    struct t_plugin_script temp_script;
    struct stat buf;
    char *perl_code;
    int length;
#ifndef MULTIPLICITY
    char pkgname[64];
#endif /* MULTIPLICITY */

    temp_script.filename = NULL;
    temp_script.interpreter = NULL;
    temp_script.name = NULL;
    temp_script.author = NULL;
    temp_script.version = NULL;
    temp_script.license = NULL;
    temp_script.description = NULL;
    temp_script.shutdown_func = NULL;
    temp_script.charset = NULL;

    if (!code)
    {
        if (stat (filename, &buf) != 0)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: script \"%s\" not found"),
                            weechat_prefix ("error"), PERL_PLUGIN_NAME,
                            filename);
            return NULL;
        }
    }

    if ((weechat_perl_plugin->debug >= 2) || !perl_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: loading script \"%s\""),
                        PERL_PLUGIN_NAME, filename);
    }

    perl_current_script = NULL;
    perl_current_script_filename = filename;
    perl_registered_script = NULL;

#ifdef MULTIPLICITY
    perl_current_interpreter = perl_alloc ();

    if (!perl_current_interpreter)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to create new "
                                         "sub-interpreter"),
                        weechat_prefix ("error"), PERL_PLUGIN_NAME);
        return NULL;
    }

    snprintf (str_warning, sizeof (str_warning),
              weechat_gettext ("%s: warning:"),
              PERL_PLUGIN_NAME);
    snprintf (str_error, sizeof (str_error),
              weechat_gettext ("%s: error:"),
              PERL_PLUGIN_NAME);

    PERL_SET_CONTEXT (perl_current_interpreter);
    perl_construct (perl_current_interpreter);
    temp_script.interpreter = (PerlInterpreter *) perl_current_interpreter;
    perl_parse (perl_current_interpreter, weechat_perl_api_init,
                perl_args_count, perl_args, NULL);
#if PERL_REVISION >= 6 || (PERL_REVISION == 5 && PERL_VERSION >= 38)
    /* restore the locale that could be changed by Perl >= 5.38 */
    Perl_setlocale (LC_CTYPE, "");
#endif
    length = strlen (perl_weechat_code) + strlen (str_warning) +
        strlen (str_error) - 2 + 4 + strlen ((code) ? code : filename) + 4 + 1;
    perl_code = malloc (length);
    if (!perl_code)
        return NULL;
    snprintf (perl_code, length, perl_weechat_code,
              str_warning,
              str_error,
              (code) ? "{\n" : "'",
              (code) ? code : filename,
              (code) ? "\n};\n" : "';");
#else
    snprintf (pkgname, sizeof (pkgname), "%s%d", PKG_NAME_PREFIX, perl_num);
    perl_num++;
    length = strlen (perl_weechat_code) + strlen (str_warning) +
        strlen (str_error) - 4 + strlen (pkgname) + 4 +
        strlen ((code) ? code : filename) + 4 + 1;
    perl_code = malloc (length);
    if (!perl_code)
        return NULL;
    snprintf (perl_code, length, perl_weechat_code,
              pkgname,
              str_warning,
              str_error,
              (code) ? "{\n" : "'",
              (code) ? code : filename,
              (code) ? "\n};\n" : "';");
#endif /* MULTIPLICITY */
    eval_pv (perl_code, TRUE);
    free (perl_code);

    if (SvTRUE (ERRSV))
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to parse file "
                                         "\"%s\""),
                        weechat_prefix ("error"), PERL_PLUGIN_NAME,
                        filename);
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error: %s"),
                        weechat_prefix ("error"), PERL_PLUGIN_NAME,
                        SvPV_nolen (ERRSV));
#ifdef MULTIPLICITY
        perl_destruct (perl_current_interpreter);
        perl_free (perl_current_interpreter);
#endif /* MULTIPLICITY */
        if (perl_current_script && (perl_current_script != &temp_script))
        {
            plugin_script_remove (weechat_perl_plugin,
                                  &perl_scripts, &last_perl_script,
                                  perl_current_script);
            perl_current_script = NULL;
        }

        return NULL;
    }

    if (!perl_registered_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        weechat_prefix ("error"), PERL_PLUGIN_NAME, filename);
#ifdef MULTIPLICITY
        perl_destruct (perl_current_interpreter);
        perl_free (perl_current_interpreter);
#endif /* MULTIPLICITY */
        return NULL;
    }
    perl_current_script = perl_registered_script;

#ifndef MULTIPLICITY
    perl_current_script->interpreter = strdup (pkgname);
#endif /* MULTIPLICITY */

    /*
     * set input/close callbacks for buffers created by this script
     * (to restore callbacks after upgrade)
     */
    plugin_script_set_buffer_callbacks (weechat_perl_plugin,
                                        perl_scripts,
                                        perl_current_script,
                                        &weechat_perl_api_buffer_input_data_cb,
                                        &weechat_perl_api_buffer_close_cb);

    (void) weechat_hook_signal_send ("perl_script_loaded",
                                     WEECHAT_HOOK_SIGNAL_STRING,
                                     perl_current_script->filename);

    return perl_current_script;
}

/*
 * Callback for weechat_script_auto_load() function.
 */

void
weechat_perl_load_cb (void *data, const char *filename)
{
    const char *pos_dot;

    /* make C compiler happy */
    (void) data;

    pos_dot = strrchr (filename, '.');
    if (pos_dot && (strcmp (pos_dot, ".pl") == 0))
        weechat_perl_load (filename, NULL);
}

/*
 * Unloads a perl script.
 */

void
weechat_perl_unload (struct t_plugin_script *script)
{
    int *rc;
    void *interpreter;
    char *filename;

    if ((weechat_perl_plugin->debug >= 2) || !perl_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: unloading script \"%s\""),
                        PERL_PLUGIN_NAME, script->name);
    }

#ifdef MULTIPLICITY
    PERL_SET_CONTEXT (script->interpreter);
#endif /* MULTIPLICITY */

    if (script->shutdown_func && script->shutdown_func[0])
    {
        rc = (int *)weechat_perl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script->shutdown_func,
                                       NULL, NULL);
        free (rc);
    }

    filename = strdup (script->filename);
    interpreter = script->interpreter;

    if (perl_current_script == script)
    {
        perl_current_script = (perl_current_script->prev_script) ?
            perl_current_script->prev_script : perl_current_script->next_script;
    }

    plugin_script_remove (weechat_perl_plugin, &perl_scripts, &last_perl_script,
                          script);

#ifdef MULTIPLICITY
    if (interpreter)
    {
        perl_destruct (interpreter);
        perl_free (interpreter);
    }
    if (perl_current_script)
    {
        PERL_SET_CONTEXT (perl_current_script->interpreter);
    }
#else
    free (interpreter);
#endif /* MULTIPLICITY */

    (void) weechat_hook_signal_send ("perl_script_unloaded",
                                     WEECHAT_HOOK_SIGNAL_STRING, filename);
    free (filename);
}

/*
 * Unloads a perl script by name.
 */

void
weechat_perl_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = plugin_script_search (perl_scripts, name);
    if (ptr_script)
    {
        weechat_perl_unload (ptr_script);
        if (!perl_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: script \"%s\" unloaded"),
                            PERL_PLUGIN_NAME, name);
        }
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), PERL_PLUGIN_NAME, name);
    }
}

/*
 * Unloads all perl scripts.
 */

void
weechat_perl_unload_all ()
{
    while (perl_scripts)
    {
        weechat_perl_unload (perl_scripts);
    }
}

/*
 * Reloads a perl script by name.
 */

void
weechat_perl_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;

    ptr_script = plugin_script_search (perl_scripts, name);
    if (ptr_script)
    {
        filename = strdup (ptr_script->filename);
        if (filename)
        {
            weechat_perl_unload (ptr_script);
            if (!perl_quiet)
            {
                weechat_printf (NULL,
                                weechat_gettext ("%s: script \"%s\" unloaded"),
                                PERL_PLUGIN_NAME, name);
            }
            weechat_perl_load (filename, NULL);
            free (filename);
        }
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), PERL_PLUGIN_NAME, name);
    }
}

/*
 * Evaluates perl source code.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
weechat_perl_eval (struct t_gui_buffer *buffer, int send_to_buffer_as_input,
                   int exec_commands, const char *code)
{
    void *func_argv[1], *result;

    if (!perl_script_eval)
    {
        perl_quiet = 1;
        perl_script_eval = weechat_perl_load (WEECHAT_SCRIPT_EVAL_NAME,
                                              PERL_EVAL_SCRIPT);
        perl_quiet = 0;
        if (!perl_script_eval)
            return 0;
    }

    weechat_perl_output_flush ();

    perl_eval_mode = 1;
    perl_eval_send_input = send_to_buffer_as_input;
    perl_eval_exec_commands = exec_commands;
    perl_eval_buffer = buffer;

    func_argv[0] = (char *)code;
    result = weechat_perl_exec (perl_script_eval,
                                WEECHAT_SCRIPT_EXEC_IGNORE,
                                "script_perl_eval",
                                "s", func_argv);
    /* result is ignored */
    free (result);

    weechat_perl_output_flush ();

    perl_eval_mode = 0;
    perl_eval_send_input = 0;
    perl_eval_exec_commands = 0;
    perl_eval_buffer = NULL;

    if (!weechat_config_boolean (perl_config_look_eval_keep_context))
    {
        perl_quiet = 1;
        weechat_perl_unload (perl_script_eval);
        perl_quiet = 0;
        perl_script_eval = NULL;
    }

    return 1;
}

/*
 * Callback for command "/perl".
 */

int
weechat_perl_command_cb (const void *pointer, void *data,
                         struct t_gui_buffer *buffer,
                         int argc, char **argv, char **argv_eol)
{
    char *ptr_name, *ptr_code, *path_script;
    int i, send_to_buffer_as_input, exec_commands;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (argc == 1)
    {
        plugin_script_display_list (weechat_perl_plugin, perl_scripts,
                                    NULL, 0);
    }
    else if (argc == 2)
    {
        if (weechat_strcmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_perl_plugin, perl_scripts,
                                        NULL, 0);
        }
        else if (weechat_strcmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_perl_plugin, perl_scripts,
                                        NULL, 1);
        }
        else if (weechat_strcmp (argv[1], "autoload") == 0)
        {
            plugin_script_auto_load (weechat_perl_plugin, &weechat_perl_load_cb);
        }
        else if (weechat_strcmp (argv[1], "reload") == 0)
        {
            weechat_perl_unload_all ();
            plugin_script_auto_load (weechat_perl_plugin, &weechat_perl_load_cb);
        }
        else if (weechat_strcmp (argv[1], "unload") == 0)
        {
            weechat_perl_unload_all ();
        }
        else if (weechat_strcmp (argv[1], "version") == 0)
        {
            plugin_script_display_interpreter (weechat_perl_plugin, 0);
        }
        else
            WEECHAT_COMMAND_ERROR;
    }
    else
    {
        if (weechat_strcmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_perl_plugin, perl_scripts,
                                        argv_eol[2], 0);
        }
        else if (weechat_strcmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_perl_plugin, perl_scripts,
                                        argv_eol[2], 1);
        }
        else if ((weechat_strcmp (argv[1], "load") == 0)
                 || (weechat_strcmp (argv[1], "reload") == 0)
                 || (weechat_strcmp (argv[1], "unload") == 0))
        {
            ptr_name = argv_eol[2];
            if (strncmp (ptr_name, "-q ", 3) == 0)
            {
                perl_quiet = 1;
                ptr_name += 3;
                while (ptr_name[0] == ' ')
                {
                    ptr_name++;
                }
            }
            if (weechat_strcmp (argv[1], "load") == 0)
            {
                /* load perl script */
                path_script = plugin_script_search_path (weechat_perl_plugin,
                                                         ptr_name, 1);
                weechat_perl_load ((path_script) ? path_script : ptr_name,
                                   NULL);
                free (path_script);
            }
            else if (weechat_strcmp (argv[1], "reload") == 0)
            {
                /* reload one perl script */
                weechat_perl_reload_name (ptr_name);
            }
            else if (weechat_strcmp (argv[1], "unload") == 0)
            {
                /* unload perl script */
                weechat_perl_unload_name (ptr_name);
            }
            perl_quiet = 0;
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
            if (!weechat_perl_eval (buffer, send_to_buffer_as_input,
                                    exec_commands, ptr_code))
                WEECHAT_COMMAND_ERROR;
        }
        else
            WEECHAT_COMMAND_ERROR;
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds perl scripts to completion list.
 */

int
weechat_perl_completion_cb (const void *pointer, void *data,
                            const char *completion_item,
                            struct t_gui_buffer *buffer,
                            struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    plugin_script_completion (weechat_perl_plugin, completion, perl_scripts);

    return WEECHAT_RC_OK;
}

/*
 * Returns hdata for perl scripts.
 */

struct t_hdata *
weechat_perl_hdata_cb (const void *pointer, void *data,
                       const char *hdata_name)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    return plugin_script_hdata_script (weechat_plugin,
                                       &perl_scripts, &last_perl_script,
                                       hdata_name);
}

/*
 * Returns perl info "perl_eval".
 */

char *
weechat_perl_info_eval_cb (const void *pointer, void *data,
                           const char *info_name,
                           const char *arguments)
{
    char *output;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    weechat_perl_eval (NULL, 0, 0, (arguments) ? arguments : "");
    output = strdup (*perl_buffer_output);
    weechat_string_dyn_copy (perl_buffer_output, NULL);

    return output;
}

/*
 * Returns infolist with perl scripts.
 */

struct t_infolist *
weechat_perl_infolist_cb (const void *pointer, void *data,
                          const char *infolist_name,
                          void *obj_pointer, const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (strcmp (infolist_name, "perl_script") == 0)
    {
        return plugin_script_infolist_list_scripts (weechat_perl_plugin,
                                                    perl_scripts, obj_pointer,
                                                    arguments);
    }

    return NULL;
}

/*
 * Dumps perl plugin data in WeeChat log file.
 */

int
weechat_perl_signal_debug_dump_cb (const void *pointer, void *data,
                                   const char *signal,
                                   const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data || (strcmp ((char *)signal_data, PERL_PLUGIN_NAME) == 0))
    {
        plugin_script_print_log (weechat_perl_plugin, perl_scripts);
    }

    return WEECHAT_RC_OK;
}

/*
 * Timer for executing actions.
 */

int
weechat_perl_timer_action_cb (const void *pointer, void *data,
                              int remaining_calls)
{
    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    if (pointer)
    {
        if (pointer == &perl_action_install_list)
        {
            plugin_script_action_install (weechat_perl_plugin,
                                          perl_scripts,
                                          &weechat_perl_unload,
                                          &weechat_perl_load,
                                          &perl_quiet,
                                          &perl_action_install_list);
        }
        else if (pointer == &perl_action_remove_list)
        {
            plugin_script_action_remove (weechat_perl_plugin,
                                         perl_scripts,
                                         &weechat_perl_unload,
                                         &perl_quiet,
                                         &perl_action_remove_list);
        }
        else if (pointer == &perl_action_autoload_list)
        {
            plugin_script_action_autoload (weechat_perl_plugin,
                                           &perl_quiet,
                                           &perl_action_autoload_list);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback called when a script action is asked (install/remove a script).
 */

int
weechat_perl_signal_script_action_cb (const void *pointer, void *data,
                                      const char *signal,
                                      const char *type_data,
                                      void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "perl_script_install") == 0)
        {
            plugin_script_action_add (&perl_action_install_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_perl_timer_action_cb,
                                &perl_action_install_list, NULL);
        }
        else if (strcmp (signal, "perl_script_remove") == 0)
        {
            plugin_script_action_add (&perl_action_remove_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_perl_timer_action_cb,
                                &perl_action_remove_list, NULL);
        }
        else if (strcmp (signal, "perl_script_autoload") == 0)
        {
            plugin_script_action_add (&perl_action_autoload_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_perl_timer_action_cb,
                                &perl_action_autoload_list, NULL);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback called when exiting or upgrading WeeChat.
 */

int
weechat_perl_signal_quit_upgrade_cb (const void *pointer, void *data,
                                     const char *signal,
                                     const char *type_data,
                                     void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    if (!signal_data || (strcmp (signal_data, "save") != 0))
        perl_quit_or_upgrade = 1;

    return WEECHAT_RC_OK;
}

/*
 * Initializes perl plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
#ifdef PERL_SYS_INIT3
    int a;
    char **perl_args_local;
    char *perl_env[] = {};
    a = perl_args_count;
    perl_args_local = perl_args;
    (void) perl_env;
    PERL_SYS_INIT3 (&a, (char ***)&perl_args_local, (char ***)&perl_env);
#endif /* PERL_SYS_INIT3 */

    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_perl_plugin = plugin;

    perl_quiet = 0;
    perl_eval_mode = 0;
    perl_eval_send_input = 0;
    perl_eval_exec_commands = 0;

    /* set interpreter name and version */
    weechat_hashtable_set (plugin->variables, "interpreter_name",
                           plugin->name);
#ifdef PERL_VERSION_STRING
    weechat_hashtable_set (plugin->variables, "interpreter_version",
                           PERL_VERSION_STRING);
#else
    weechat_hashtable_set (plugin->variables, "interpreter_version",
                           "");
#endif /* PERL_VERSION_STRING */

    /* init stdout/stderr buffer */
    perl_buffer_output = weechat_string_dyn_alloc (256);
    if (!perl_buffer_output)
        return WEECHAT_RC_ERROR;

#ifndef MULTIPLICITY
    perl_main = perl_alloc ();

    if (!perl_main)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to initialize %s"),
                        weechat_prefix ("error"), PERL_PLUGIN_NAME,
                        PERL_PLUGIN_NAME);
        return WEECHAT_RC_ERROR;
    }

    perl_construct (perl_main);
    perl_parse (perl_main, weechat_perl_api_init, perl_args_count,
                perl_args, NULL);
#if PERL_REVISION >= 6 || (PERL_REVISION == 5 && PERL_VERSION >= 38)
    /* restore the locale that could be changed by Perl >= 5.38 */
    Perl_setlocale (LC_CTYPE, "");
#endif
#endif /* MULTIPLICITY */

    perl_data.config_file = &perl_config_file;
    perl_data.config_look_check_license = &perl_config_look_check_license;
    perl_data.config_look_eval_keep_context = &perl_config_look_eval_keep_context;
    perl_data.scripts = &perl_scripts;
    perl_data.last_script = &last_perl_script;
    perl_data.callback_command = &weechat_perl_command_cb;
    perl_data.callback_completion = &weechat_perl_completion_cb;
    perl_data.callback_hdata = &weechat_perl_hdata_cb;
    perl_data.callback_info_eval = &weechat_perl_info_eval_cb;
    perl_data.callback_infolist = &weechat_perl_infolist_cb;
    perl_data.callback_signal_debug_dump = &weechat_perl_signal_debug_dump_cb;
    perl_data.callback_signal_script_action = &weechat_perl_signal_script_action_cb;
    perl_data.callback_load_file = &weechat_perl_load_cb;
    perl_data.unload_all = &weechat_perl_unload_all;

    perl_quiet = 1;
    plugin_script_init (weechat_perl_plugin, &perl_data);
    perl_quiet = 0;

    plugin_script_display_short_list (weechat_perl_plugin,
                                      perl_scripts);

    weechat_hook_signal ("quit;upgrade",
                         &weechat_perl_signal_quit_upgrade_cb, NULL, NULL);

    /* init OK */
    return WEECHAT_RC_OK;
}

/*
 * Ends perl plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* unload all scripts */
    perl_quiet = 1;
    if (perl_script_eval)
    {
        weechat_perl_unload (perl_script_eval);
        perl_script_eval = NULL;
    }
    plugin_script_end (plugin, &perl_data);
    perl_quiet = 0;

#ifndef MULTIPLICITY
    /* free perl interpreter */
    if (perl_main)
    {
        perl_destruct (perl_main);
        perl_free (perl_main);
        perl_main = NULL;
    }
#endif /* MULTIPLICITY */

#if defined(PERL_SYS_TERM) && !defined(__FreeBSD__) && !defined(WIN32) && !defined(__CYGWIN__) && !(defined(__APPLE__) && defined(__MACH__))
    /*
     * we call this function on all OS, but NOT on FreeBSD or Cygwin,
     * because it crashes with no reason (bug in Perl?)
     */
    if (perl_quit_or_upgrade)
        PERL_SYS_TERM ();
#endif

    /* free some data */
    if (perl_action_install_list)
    {
        free (perl_action_install_list);
        perl_action_install_list = NULL;
    }
    if (perl_action_remove_list)
    {
        free (perl_action_remove_list);
        perl_action_remove_list = NULL;
    }
    if (perl_action_autoload_list)
    {
        free (perl_action_autoload_list);
        perl_action_autoload_list = NULL;
    }
    weechat_string_dyn_free (perl_buffer_output, 1);
    perl_buffer_output = NULL;

    return WEECHAT_RC_OK;
}
