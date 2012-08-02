/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * weechat-perl.c: perl plugin for WeeChat
 */

#undef _

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

#include "../../weechat-plugin.h"
#include "../script.h"
#include "weechat-perl.h"
#include "weechat-perl-api.h"


WEECHAT_PLUGIN_NAME(PERL_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Support of perl scripts"));
WEECHAT_PLUGIN_AUTHOR("Sebastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);

struct t_weechat_plugin *weechat_perl_plugin = NULL;

int perl_quiet = 0;
struct t_plugin_script *perl_scripts = NULL;
struct t_plugin_script *last_perl_script = NULL;
struct t_plugin_script *perl_current_script = NULL;
struct t_plugin_script *perl_registered_script = NULL;
const char *perl_current_script_filename = NULL;
int perl_quit_or_upgrade = 0;

/*
 * string used to execute action "install":
 * when signal "perl_install_script" is received, name of string
 * is added to this string, to be installed later by a timer (when nothing is
 * running in script)
 */
char *perl_action_install_list = NULL;

/*
 * string used to execute action "remove":
 * when signal "perl_remove_script" is received, name of string
 * is added to this string, to be removed later by a timer (when nothing is
 * running in script)
 */
char *perl_action_remove_list = NULL;

#ifdef NO_PERL_MULTIPLICITY
#undef MULTIPLICITY
#endif

#ifndef MULTIPLICITY
#define PKG_NAME_PREFIX "WeechatPerlPackage"
static PerlInterpreter *perl_main = NULL;
int perl_num = 0;
#endif

char *perl_args[] = { "", "-e", "0", "-w", NULL };
int perl_args_count = 4;

char *perl_weechat_code =
{
#ifndef MULTIPLICITY
    "package WeechatPerlScriptLoader;"
#endif
    "$weechat_perl_load_eval_file_error = \"\";"
    "sub weechat_perl_load_file"
    "{"
    "    my $filename = shift;"
    "    local $/ = undef;"
    "    open FILE, $filename or return \"__WEECHAT_PERL_ERROR__\";"
    "    $_ = <FILE>;"
    "    close FILE;"
    "    return $_;"
    "}"
    "sub weechat_perl_load_eval_file"
    "{"
#ifdef MULTIPLICITY
    "    my $filename = shift;"
#else
    "    my ($filename, $package) = @_;"
#endif
    "    my $content = weechat_perl_load_file ($filename);"
    "    if ($content eq \"__WEECHAT_PERL_ERROR__\")"
    "    {"
    "        return 1;"
    "    }"
#ifdef MULTIPLICITY
    "    my $eval = $content;"
#else
    "    my $eval = qq{package $package; $content;};"
#endif
    "    {"
    "      eval $eval;"
    "    }"
    "    if ($@)"
    "    {"
    "        $weechat_perl_load_eval_file_error = $@;"
    "        return 2;"
    "    }"
    "    return 0;"
    "}"
    "$SIG{__WARN__} = sub { weechat::print(\"\", \"\tperl error: $_[0]\"); };"
    "$SIG{__DIE__} = sub { weechat::print(\"\", \"\tperl error: $_[0]\"); };"
};


/*
 * weechat_perl_hashtable_map_cb: callback called for each key/value in a
 *                                hashtable
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
 * weechat_perl_hashtable_to_hash: get perl hash with a WeeChat hashtable
 */

HV *
weechat_perl_hashtable_to_hash (struct t_hashtable *hashtable)
{
    HV *hash;

    hash = (HV *)newHV ();
    if (!hash)
        return NULL;

    weechat_hashtable_map_string (hashtable,
                                  &weechat_perl_hashtable_map_cb,
                                  hash);

    return hash;
}

/*
 * weechat_perl_hash_to_hashtable: get WeeChat hashtable with perl hash
 *                                 Hashtable returned has type string for
 *                                 both keys and values
 *                                 Note: hashtable has to be released after use
 *                                 with call to weechat_hashtable_free()
 */

struct t_hashtable *
weechat_perl_hash_to_hashtable (SV *hash, int hashtable_size)
{
    struct t_hashtable *hashtable;
    HV *hash2;
    SV *value;
    char *str_key;
    I32 retlen;

    hashtable = weechat_hashtable_new (hashtable_size,
                                       WEECHAT_HASHTABLE_STRING,
                                       WEECHAT_HASHTABLE_STRING,
                                       NULL,
                                       NULL);
    if (!hashtable)
        return NULL;

    if ((hash) && SvROK(hash) && SvRV(hash) && (SvTYPE(SvRV(hash)) == SVt_PVHV))
    {
        hash2 = (HV *) SvRV(hash);
        hv_iterinit (hash2);
        while ((value = hv_iternextsv (hash2, &str_key, &retlen)))
        {
            weechat_hashtable_set (hashtable, str_key, SvPV (value, PL_na));
        }
    }

    return hashtable;
}

/*
 * weechat_perl_exec: execute a perl function
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
#endif

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
#endif

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
                case 's': /* string */
                    XPUSHs(sv_2mortal(newSVpv((char *)argv[i], 0)));
                    break;
                case 'i': /* integer */
                    XPUSHs(sv_2mortal(newSViv(*((int *)argv[i]))));
                    break;
                case 'h': /* hash */
                    hash = weechat_perl_hashtable_to_hash (argv[i]);
                    XPUSHs(sv_2mortal(newRV_inc((SV *)hash)));
                    break;
            }
        }
    }
    PUTBACK;
    count = perl_call_pv (func, G_EVAL | G_SCALAR);

    ret_value = NULL;
    mem_err = 1;

    SPAGAIN;

    if (SvTRUE (ERRSV))
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error: %s"),
                        weechat_prefix ("error"), PERL_PLUGIN_NAME,
                        SvPV_nolen (ERRSV));
        (void) POPs; /* poping the 'undef' */
        mem_err = 0;
    }
    else
    {
        if (count != 1)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: function \"%s\" must "
                                             "return one valid value (%d)"),
                            weechat_prefix ("error"), PERL_PLUGIN_NAME,
                            function, count);
            mem_err = 0;
        }
        else
        {
            if (ret_type == WEECHAT_SCRIPT_EXEC_STRING)
            {
                ret_s = newSVsv(POPs);
                ret_value = strdup (SvPV_nolen (ret_s));
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
                                                            WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE);
            }
            else
            {
                weechat_printf (NULL,
                                weechat_gettext ("%s%s: function \"%s\" is "
                                                 "internally misused"),
                                weechat_prefix ("error"), PERL_PLUGIN_NAME,
                                function);
                mem_err = 0;
            }
        }
    }

    PUTBACK;
    FREETMPS;
    LEAVE;

    perl_current_script = old_perl_current_script;
#ifdef MULTIPLICITY
    PERL_SET_CONTEXT (old_context);
#else
    free (func);
#endif

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
 * weechat_perl_load: load a Perl script
 */

int
weechat_perl_load (const char *filename)
{
    STRLEN len;
    struct t_plugin_script temp_script;
    int *eval;
    struct stat buf;
    void *perl_argv[2];

#ifdef MULTIPLICITY
    PerlInterpreter *perl_current_interpreter;
#else
    char pkgname[64];
#endif

    temp_script.filename = NULL;
    temp_script.interpreter = NULL;
    temp_script.name = NULL;
    temp_script.author = NULL;
    temp_script.version = NULL;
    temp_script.license = NULL;
    temp_script.description = NULL;
    temp_script.shutdown_func = NULL;
    temp_script.charset = NULL;

    if (stat (filename, &buf) != 0)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not found"),
                        weechat_prefix ("error"), PERL_PLUGIN_NAME, filename);
        return 0;
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
    perl_current_interpreter = perl_alloc();

    if (!perl_current_interpreter)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to create new "
                                         "sub-interpreter"),
                        weechat_prefix ("error"), PERL_PLUGIN_NAME);
        return 0;
    }

    PERL_SET_CONTEXT (perl_current_interpreter);
    perl_construct (perl_current_interpreter);
    temp_script.interpreter = (PerlInterpreter *) perl_current_interpreter;
    perl_parse (perl_current_interpreter, weechat_perl_api_init,
                perl_args_count, perl_args, NULL);

    eval_pv (perl_weechat_code, TRUE);
    perl_argv[0] = (void *)filename;
    eval = weechat_perl_exec (&temp_script,
                              WEECHAT_SCRIPT_EXEC_INT,
                              "weechat_perl_load_eval_file",
                              "s", perl_argv);
#else
    snprintf (pkgname, sizeof(pkgname), "%s%d", PKG_NAME_PREFIX, perl_num);
    perl_num++;
    temp_script.interpreter = "WeechatPerlScriptLoader";
    perl_argv[0] = (void *)filename;
    perl_argv[1] = pkgname;
    eval = weechat_perl_exec (&temp_script,
                              WEECHAT_SCRIPT_EXEC_INT,
                              "weechat_perl_load_eval_file",
                              "ss", perl_argv);
#endif
    if (!eval)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: not enough memory to parse "
                                         "file \"%s\""),
                        weechat_prefix ("error"), PERL_PLUGIN_NAME, filename);
        return 0;
    }

    if (*eval != 0)
    {
        if (*eval == 2)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unable to parse file "
                                             "\"%s\""),
                            weechat_prefix ("error"), PERL_PLUGIN_NAME,
                            filename);
#ifdef MULTIPLICITY
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: error: %s"),
                            weechat_prefix ("error"), PERL_PLUGIN_NAME,
                            SvPV(perl_get_sv("weechat_perl_load_eval_file_error",
                                             FALSE), len));
#else
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: error: %s"),
                            weechat_prefix ("error"), PERL_PLUGIN_NAME,
                            SvPV(perl_get_sv("WeechatPerlScriptLoader::"
                                             "weechat_perl_load_eval_file_error",
                                             FALSE), len));
#endif
        }
        else if (*eval == 1)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unable to run file \"%s\""),
                            weechat_prefix ("error"), PERL_PLUGIN_NAME,
                            filename);
        }
        else
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unknown error while "
                                             "loading file \"%s\""),
                            weechat_prefix ("error"), PERL_PLUGIN_NAME,
                            filename);
        }
#ifdef MULTIPLICITY
        perl_destruct (perl_current_interpreter);
        perl_free (perl_current_interpreter);
#endif
        if (perl_current_script && (perl_current_script != &temp_script))
        {
            script_remove (weechat_perl_plugin,
                           &perl_scripts, &last_perl_script,
                           perl_current_script);
        }

        free (eval);
        return 0;
    }

    free (eval);

    if (!perl_registered_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        weechat_prefix ("error"), PERL_PLUGIN_NAME, filename);
#ifdef MULTIPLICITY
        perl_destruct (perl_current_interpreter);
        perl_free (perl_current_interpreter);
#endif
        return 0;
    }
    perl_current_script = perl_registered_script;

#ifdef MULTIPLICITY
    perl_current_script->interpreter = (PerlInterpreter *)perl_current_interpreter;
#else
    perl_current_script->interpreter = strdup (pkgname);
#endif

    /*
     * set input/close callbacks for buffers created by this script
     * (to restore callbacks after upgrade)
     */
    script_set_buffer_callbacks (weechat_perl_plugin,
                                 perl_scripts,
                                 perl_current_script,
                                 &weechat_perl_api_buffer_input_data_cb,
                                 &weechat_perl_api_buffer_close_cb);

    return 1;
}

/*
 * weechat_perl_load_cb: callback for weechat_script_auto_load() function
 */

void
weechat_perl_load_cb (void *data, const char *filename)
{
    /* make C compiler happy */
    (void) data;

    weechat_perl_load (filename);
}

/*
 * weechat_perl_unload: unload a Perl script
 */

void
weechat_perl_unload (struct t_plugin_script *script)
{
    int *rc;
    void *interpreter;

    if ((weechat_perl_plugin->debug >= 2) || !perl_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: unloading script \"%s\""),
                        PERL_PLUGIN_NAME, script->name);
    }

#ifdef MULTIPLICITY
    PERL_SET_CONTEXT (script->interpreter);
#else
    eval_pv (script->interpreter, TRUE);
#endif

    if (script->shutdown_func && script->shutdown_func[0])
    {
        rc = (int *)weechat_perl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script->shutdown_func,
                                       NULL, NULL);
        if (rc)
            free (rc);
    }

    interpreter = script->interpreter;

    if (perl_current_script == script)
        perl_current_script = (perl_current_script->prev_script) ?
            perl_current_script->prev_script : perl_current_script->next_script;

    script_remove (weechat_perl_plugin, &perl_scripts, &last_perl_script,
                   script);

#ifdef MULTIPLICITY
    if (interpreter)
    {
        perl_destruct (interpreter);
        perl_free (interpreter);
    }
#else
    if (interpreter)
        free (interpreter);
#endif
}

/*
 * weechat_perl_unload_name: unload a Perl script by name
 */

void
weechat_perl_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = script_search (weechat_perl_plugin, perl_scripts, name);
    if (ptr_script)
    {
        weechat_perl_unload (ptr_script);
        weechat_printf (NULL,
                        weechat_gettext ("%s: script \"%s\" unloaded"),
                        PERL_PLUGIN_NAME, name);
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), PERL_PLUGIN_NAME, name);
    }
}

/*
 * weechat_perl_unload_all: unload all Perl scripts
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
 * weechat_perl_reload_name: reload a Perl script by name
 */

void
weechat_perl_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;

    ptr_script = script_search (weechat_perl_plugin, perl_scripts, name);
    if (ptr_script)
    {
        filename = strdup (ptr_script->filename);
        if (filename)
        {
            weechat_perl_unload (ptr_script);
            weechat_printf (NULL,
                            weechat_gettext ("%s: script \"%s\" unloaded"),
                            PERL_PLUGIN_NAME, name);
            weechat_perl_load (filename);
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
 * weechat_perl_command_cb: callback for "/perl" command
 */

int
weechat_perl_command_cb (void *data, struct t_gui_buffer *buffer,
                         int argc, char **argv, char **argv_eol)
{
    char *path_script;

    /* make C compiler happy */
    (void) data;
    (void) buffer;

    if (argc == 1)
    {
        script_display_list (weechat_perl_plugin, perl_scripts,
                             NULL, 0);
    }
    else if (argc == 2)
    {
        if (weechat_strcasecmp (argv[1], "list") == 0)
        {
            script_display_list (weechat_perl_plugin, perl_scripts,
                                 NULL, 0);
        }
        else if (weechat_strcasecmp (argv[1], "listfull") == 0)
        {
            script_display_list (weechat_perl_plugin, perl_scripts,
                                 NULL, 1);
        }
        else if (weechat_strcasecmp (argv[1], "autoload") == 0)
        {
            script_auto_load (weechat_perl_plugin, &weechat_perl_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "reload") == 0)
        {
            weechat_perl_unload_all ();
            script_auto_load (weechat_perl_plugin, &weechat_perl_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "unload") == 0)
        {
            weechat_perl_unload_all ();
        }
    }
    else
    {
        if (weechat_strcasecmp (argv[1], "list") == 0)
        {
            script_display_list (weechat_perl_plugin, perl_scripts,
                                 argv_eol[2], 0);
        }
        else if (weechat_strcasecmp (argv[1], "listfull") == 0)
        {
            script_display_list (weechat_perl_plugin, perl_scripts,
                                 argv_eol[2], 1);
        }
        else if (weechat_strcasecmp (argv[1], "load") == 0)
        {
            /* load Perl script */
            path_script = script_search_path (weechat_perl_plugin,
                                              argv_eol[2]);
            weechat_perl_load ((path_script) ? path_script : argv_eol[2]);
            if (path_script)
                free (path_script);
        }
        else if (weechat_strcasecmp (argv[1], "reload") == 0)
        {
            /* reload one Perl script */
            weechat_perl_reload_name (argv_eol[2]);
        }
        else if (weechat_strcasecmp (argv[1], "unload") == 0)
        {
            /* unload Perl script */
            weechat_perl_unload_name (argv_eol[2]);
        }
        else
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unknown option for "
                                             "command \"%s\""),
                            weechat_prefix ("error"), PERL_PLUGIN_NAME, "perl");
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * weechat_perl_completion_cb: callback for script completion
 */

int
weechat_perl_completion_cb (void *data, const char *completion_item,
                            struct t_gui_buffer *buffer,
                            struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    script_completion (weechat_perl_plugin, completion, perl_scripts);

    return WEECHAT_RC_OK;
}

/*
 * weechat_perl_infolist_cb: callback for infolist
 */

struct t_infolist *
weechat_perl_infolist_cb (void *data, const char *infolist_name,
                          void *pointer, const char *arguments)
{
    /* make C compiler happy */
    (void) data;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (weechat_strcasecmp (infolist_name, "perl_script") == 0)
    {
        return script_infolist_list_scripts (weechat_perl_plugin,
                                             perl_scripts, pointer,
                                             arguments);
    }

    return NULL;
}

/*
 * weechat_perl_signal_debug_dump_cb: dump Perl plugin data in WeeChat log file
 */

int
weechat_perl_signal_debug_dump_cb (void *data, const char *signal,
                                   const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data
        || (weechat_strcasecmp ((char *)signal_data, PERL_PLUGIN_NAME) == 0))
    {
        script_print_log (weechat_perl_plugin, perl_scripts);
    }

    return WEECHAT_RC_OK;
}

/*
 * weechat_perl_signal_buffer_closed_cb: callback called when a buffer is closed
 */

int
weechat_perl_signal_buffer_closed_cb (void *data, const char *signal,
                                      const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (signal_data)
        script_remove_buffer_callbacks (perl_scripts, signal_data);

    return WEECHAT_RC_OK;
}

/*
 * weechat_perl_timer_action_cb: timer for executing actions
 */

int
weechat_perl_timer_action_cb (void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) remaining_calls;

    if (data)
    {
        if (data == &perl_action_install_list)
        {
            script_action_install (weechat_perl_plugin,
                                   perl_scripts,
                                   &weechat_perl_unload,
                                   &weechat_perl_load,
                                   &perl_action_install_list);
        }
        else if (data == &perl_action_remove_list)
        {
            script_action_remove (weechat_perl_plugin,
                                  perl_scripts,
                                  &weechat_perl_unload,
                                  &perl_action_remove_list);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * weechat_perl_signal_script_action_cb: callback called when a script action
 *                                       is asked (install/remove a script)
 */

int
weechat_perl_signal_script_action_cb (void *data, const char *signal,
                                      const char *type_data,
                                      void *signal_data)
{
    /* make C compiler happy */
    (void) data;

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "perl_script_install") == 0)
        {
            script_action_add (&perl_action_install_list,
                               (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_perl_timer_action_cb,
                                &perl_action_install_list);
        }
        else if (strcmp (signal, "perl_script_remove") == 0)
        {
            script_action_add (&perl_action_remove_list,
                               (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_perl_timer_action_cb,
                                &perl_action_remove_list);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * weechat_perl_signal_quit_upgrade_cb: callback called when exiting or
 *                                      upgrading WeeChat
 */

int
weechat_perl_signal_quit_upgrade_cb (void *data, const char *signal,
                                     const char *type_data,
                                     void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    perl_quit_or_upgrade = 1;

    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_init: initialize Perl plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    struct t_plugin_script_init init;
#ifdef PERL_SYS_INIT3
    int a;
    char **perl_args_local;
    char *perl_env[] = {};
#endif
#ifdef PERL_SYS_INIT3
    a = perl_args_count;
    perl_args_local = perl_args;
    (void) perl_env;
    PERL_SYS_INIT3 (&a, (char ***)&perl_args_local, (char ***)&perl_env);
#endif

    weechat_perl_plugin = plugin;

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
    eval_pv (perl_weechat_code, TRUE);
#endif

    init.callback_command = &weechat_perl_command_cb;
    init.callback_completion = &weechat_perl_completion_cb;
    init.callback_infolist = &weechat_perl_infolist_cb;
    init.callback_signal_debug_dump = &weechat_perl_signal_debug_dump_cb;
    init.callback_signal_buffer_closed = &weechat_perl_signal_buffer_closed_cb;
    init.callback_signal_script_action = &weechat_perl_signal_script_action_cb;
    init.callback_load_file = &weechat_perl_load_cb;

    perl_quiet = 1;
    script_init (weechat_perl_plugin, argc, argv, &init);
    perl_quiet = 0;

    script_display_short_list (weechat_perl_plugin,
                               perl_scripts);

    weechat_hook_signal ("quit", &weechat_perl_signal_quit_upgrade_cb, NULL);
    weechat_hook_signal ("upgrade", &weechat_perl_signal_quit_upgrade_cb, NULL);

    /* init ok */
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end Perl plugin
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* unload all scripts */
    perl_quiet = 1;
    script_end (plugin, &perl_scripts, &weechat_perl_unload_all);
    perl_quiet = 0;

#ifndef MULTIPLICITY
    /* free perl intepreter */
    if (perl_main)
    {
        perl_destruct (perl_main);
        perl_free (perl_main);
        perl_main = NULL;
    }
#endif

#if defined(PERL_SYS_TERM) && !defined(__FreeBSD__) && !defined(WIN32) && !defined(__CYGWIN__) && !(defined(__APPLE__) && defined(__MACH__))
    /*
     * we call this function on all OS, but NOT on FreeBSD or Cygwin,
     * because it crashes with no reason (bug in Perl?)
     */
    if (perl_quit_or_upgrade)
        PERL_SYS_TERM ();
#endif

    return WEECHAT_RC_OK;
}
