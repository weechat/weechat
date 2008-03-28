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

/* weechat-perl.c: Perl plugin for WeeChat */

#undef _

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

#include "../../weechat-plugin.h"
#include "../script.h"
#include "weechat-perl.h"
#include "weechat-perl-api.h"


WEECHAT_PLUGIN_NAME("perl");
WEECHAT_PLUGIN_DESCRIPTION("Perl plugin for WeeChat");
WEECHAT_PLUGIN_AUTHOR("FlashCode <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_WEECHAT_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE("GPL");

struct t_weechat_plugin *weechat_perl_plugin = NULL;

struct t_plugin_script *perl_scripts = NULL;
struct t_plugin_script *perl_current_script = NULL;
char *perl_current_script_filename = NULL;

#ifdef NO_PERL_MULTIPLICITY
#undef MULTIPLICITY
#endif

#ifndef MULTIPLICITY
#define PKG_NAME_PREFIX "WeechatPerlPackage"
static PerlInterpreter *perl_main = NULL;
int perl_num = 0;
#endif

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
    "$SIG{__WARN__} = sub { weechat::print(\"\", \"perl error: $_[0]\"); };"
    "$SIG{__DIE__} = sub { weechat::print(\"\", \"perl error: $_[0]\"); };"
};


/*
 * weechat_perl_exec: execute a Perl script
 */

void *
weechat_perl_exec (struct t_plugin_script *script,
		   int ret_type, char *function, char **argv)
{
    char *func;
    unsigned int count;
    void *ret_value;
    int *ret_i, mem_err, length;
    SV *ret_s;

    /* this code is placed here to conform ISO C90 */
    dSP;
    
#ifdef MULTIPLICITY
    (void) length;
    func = function;
    PERL_SET_CONTEXT (script->interpreter);
#else
    length = strlen (script->interpreter) + strlen (function) + 3;
    func = malloc (length);
    if (!func)
        return NULL;
    snprintf (func, length, "%s::%s", (char *) script->interpreter, function);
#endif    
    
    ENTER;
    SAVETMPS;
    PUSHMARK(sp);
    
    /* are we loading the script file ? */
    if (strcmp (function, "weechat_perl_load_eval_file") != 0)
        perl_current_script = script;
    
    count = perl_call_argv (func, G_EVAL | G_SCALAR, argv);
    ret_value = NULL;
    mem_err = 1;

    SPAGAIN;
    
    if (SvTRUE (ERRSV))
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error: %s"),
                        weechat_prefix ("error"), "perl", SvPV_nolen (ERRSV));
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
                            weechat_prefix ("error"), "perl", function, count);
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
            else
            {
                weechat_printf (NULL,
                                weechat_gettext ("%s%s: function \"%s\" is "
                                                 "internally misused"),
                                weechat_prefix ("error"), "perl", function);
                mem_err = 0;
            }
        }
    }
    
    PUTBACK;
    FREETMPS;
    LEAVE;

#ifndef MULTIPLICITY
    free (func);
#endif

    if (!ret_value && (mem_err == 1))
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: not enough memory in function "
                                         "\"%s\""),
                        weechat_prefix ("error"), "perl", function);
        return NULL;
    }
    
    return ret_value;
}

/*
 * weechat_perl_load: load a Perl script
 */

int
weechat_perl_load (char *filename)
{
    STRLEN len;
    struct t_plugin_script tempscript;
    int *eval;
    struct stat buf;
    char *perl_argv[2];
    
#ifdef MULTIPLICITY
    PerlInterpreter *perl_current_interpreter;
    char *perl_args[] = { "", "-e", "0" };
#else
    char pkgname[64];
#endif
    
    if (stat (filename, &buf) != 0)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not found"),
                        weechat_prefix ("error"), "perl", filename);
        return 0;
    }
    
    weechat_printf (NULL,
                    weechat_gettext ("%s%s: loading script \"%s\""),
                    weechat_prefix ("info"), "perl", filename);
    
    perl_current_script = NULL;
    
#ifdef MULTIPLICITY
    perl_current_interpreter = perl_alloc();
    
    if (!perl_current_interpreter)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to create new "
                                         "sub-interpreter"),
                        weechat_prefix ("error"), "perl");
        return 0;
    }
    
    perl_current_script_filename = filename;
    
    PERL_SET_CONTEXT (perl_current_interpreter);
    perl_construct (perl_current_interpreter);
    tempscript.interpreter = (PerlInterpreter *) perl_current_interpreter;
    perl_parse (perl_current_interpreter, weechat_perl_api_init, 3, perl_args,
                NULL);
    
    eval_pv (perl_weechat_code, TRUE);
    perl_argv[0] = filename;
    perl_argv[1] = NULL;
#else
    snprintf (pkgname, sizeof(pkgname), "%s%d", PKG_NAME_PREFIX, perl_num);
    perl_num++;
    tempscript.interpreter = "WeechatPerlScriptLoader";
    perl_argv[0] = filename;
    perl_argv[1] = pkgname;
    perl_argv[2] = NULL;
#endif
    eval = weechat_perl_exec (&tempscript,
			      WEECHAT_SCRIPT_EXEC_INT,
			      "weechat_perl_load_eval_file",
                              perl_argv);
    if (!eval)
    {
	weechat_printf (NULL,
                        weechat_gettext ("%s%s: not enough memory to parse "
                                         "file \"%s\""),
                        weechat_prefix ("error"), "perl", filename);
	return 0;
    }
    
    if (*eval != 0) 
    {
	if (*eval == 2) 
	{
	    weechat_printf (NULL,
                            weechat_gettext ("%s%s: unable to parse file "
                                             "\"%s\""),
                            weechat_prefix ("error"), "perl", filename);
	    weechat_printf (NULL,
                            weechat_gettext ("%s%s: error: %s"),
                            weechat_prefix ("error"), "perl",
#ifdef MULTIPLICITY
                            SvPV(perl_get_sv("weechat_perl_load_eval_file_error",
                                             FALSE), len)
#else
                            SvPV(perl_get_sv("WeechatPerlScriptLoader::"
                                             "weechat_perl_load_eval_file_error",
                                             FALSE), len)
#endif
		);
	}
	else if (*eval == 1)
	{
	    weechat_printf (NULL,
                            weechat_gettext ("%s%s: unable to run file \"%s\""),
                            weechat_prefix ("error"), "perl", filename);
	}
	else
        {
	    weechat_printf (NULL,
                            weechat_gettext ("%s%s: unknown error while "
                                             "loading file \"%s\""),
                            weechat_prefix ("error"), "perl", filename);
	}
#ifdef MULTIPLICITY
	perl_destruct (perl_current_interpreter);
        perl_free (perl_current_interpreter);
#endif
	if (perl_current_script && (perl_current_script != &tempscript))
        {
            script_remove (weechat_perl_plugin, &perl_scripts,
                           perl_current_script);
        }
	
	free (eval);
	return 0;
    }
    
    free (eval);

    if (!perl_current_script)
    {
	weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        weechat_prefix ("error"), "perl", filename);
#ifdef MULTIPLICITY
        perl_destruct (perl_current_interpreter);
        perl_free (perl_current_interpreter);
#endif
        return 0;
    }

#ifdef MULTIPLICITY
    perl_current_script->interpreter = (PerlInterpreter *)perl_current_interpreter;
#else
    perl_current_script->interpreter = strdup (pkgname);
#endif

    return 1;
}

/*
 * weechat_perl_load_cb: callback for weechat_script_auto_load() function
 */

int
weechat_perl_load_cb (void *data, char *filename)
{
    /* make C compiler happy */
    (void) data;
    
    return weechat_perl_load (filename);
}

/*
 * weechat_perl_unload: unload a Perl script
 */

void
weechat_perl_unload (struct t_plugin_script *script)
{
    int *r;
    char *perl_argv[1] = { NULL };
    
    weechat_printf (NULL,
                    weechat_gettext ("%s%s: unloading script \"%s\""),
                    weechat_prefix ("info"), "perl", script->name);
    
#ifdef MULTIPLICITY
    PERL_SET_CONTEXT (script->interpreter);
#else
    eval_pv (script->interpreter, TRUE);
#endif        

    if (script->shutdown_func && script->shutdown_func[0])
    {
        r = (int *) weechat_perl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
				       script->shutdown_func,
                                       perl_argv);
	if (r)
	    free (r);
    }
    
#ifdef MULTIPLICITY
    perl_destruct (script->interpreter);
    perl_free (script->interpreter);
#else
    if (script->interpreter)
	free (script->interpreter);
#endif
    
    script_remove (weechat_perl_plugin, &perl_scripts, script);
}

/*
 * weechat_perl_unload_name: unload a Perl script by name
 */

void
weechat_perl_unload_name (char *name)
{
    struct t_plugin_script *ptr_script;
    
    ptr_script = script_search (weechat_perl_plugin, perl_scripts, name);
    if (ptr_script)
    {
        weechat_perl_unload (ptr_script);
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" unloaded"),
                        weechat_prefix ("info"), "perl", name);
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), "perl", name);
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
            path_script = script_search_full_name (weechat_perl_plugin,
                                                   argv_eol[2]);
            weechat_perl_load ((path_script) ? path_script : argv_eol[2]);
            if (path_script)
                free (path_script);
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
                            weechat_prefix ("error"), "perl", "perl");
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_perl_completion_cb: callback for script completion
 */

int
weechat_perl_completion_cb (void *data, char *completion,
                            struct t_gui_buffer *buffer,
                            struct t_weelist *list)
{
    /* make C compiler happy */
    (void) data;
    (void) completion;
    (void) buffer;
    
    script_completion (weechat_perl_plugin, list, perl_scripts);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_perl_debug_dump_cb: dump Perl plugin data in WeeChat log file
 */

int
weechat_perl_debug_dump_cb (void *data, char *signal, char *type_data,
                            void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;
    
    script_print_log (weechat_perl_plugin, perl_scripts);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_init: initialize Perl plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin)
{
#ifndef MULTIPLICITY
    char *perl_args[] = { "", "-e", "0" };
#endif
    
    weechat_perl_plugin = plugin;
    
#ifndef MULTIPLICITY
    perl_main = perl_alloc ();
    
    if (!perl_main)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to initialize %s"),
                        weechat_prefix ("error"), "perl", "perl");
        return WEECHAT_RC_ERROR;
    }
    
    perl_construct (perl_main);
    perl_parse (perl_main, weechat_perl_api_init, 3, perl_args, NULL);
    eval_pv (perl_weechat_code, TRUE);
#endif
    
    script_init (weechat_perl_plugin,
                 &weechat_perl_command_cb,
                 &weechat_perl_completion_cb,
                 &weechat_perl_debug_dump_cb,
                 &weechat_perl_load_cb);
    
    /* init ok */
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end Perl plugin
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;
    
    /* unload all scripts */
    weechat_perl_unload_all ();
    
#ifndef MULTIPLICITY
    /* free perl intepreter */
    if (perl_main)
    {
	perl_destruct (perl_main);
	perl_free (perl_main);
	perl_main = NULL;
    }
#endif
    
    return WEECHAT_RC_OK;
}
