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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
WEECHAT_PLUGIN_VERSION("0.1");
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
#ifndef MULTIPLICITY
    "    my ($filename, $package) = @_;"
#else
    "    my $filename = shift;"
#endif
    "    my $content = weechat_perl_load_file ($filename);"
    "    if ($content eq \"__WEECHAT_PERL_ERROR__\")"
    "    {"
    "        return 1;"
    "    }"
#ifndef MULTIPLICITY
    "    my $eval = qq{package $package; $content;};"
#else
    "    my $eval = $content;"
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
    "$SIG{__WARN__} = sub { weechat::print \"Perl error: $_[0]\", \"\"; };"
    "$SIG{__DIE__} = sub { weechat::print \"Perl error: $_[0]\", \"\"; };"
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
    int *ret_i, mem_err;
    SV *ret_s;

    /* this code is placed here to conform ISO C90 */
    dSP;
    
#ifndef MULTIPLICITY
    int size = strlen (script->interpreter) + strlen(function) + 3;
    func = (char *)malloc (size * sizeof(char));
    if (!func)
	return NULL;
    snprintf (func, size, "%s::%s", (char *) script->interpreter, function);
#else
    func = function;
    PERL_SET_CONTEXT (script->interpreter);
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
		ret_i = (int *)malloc (sizeof(int));
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
    
#ifndef MULTIPLICITY
    char pkgname[64];
#else
    PerlInterpreter *perl_current_interpreter;
    char *perl_args[] = { "", "-e", "0" };    
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
    
#ifndef MULTIPLICITY
    snprintf (pkgname, sizeof(pkgname), "%s%d", PKG_NAME_PREFIX, perl_num);
    perl_num++;
    tempscript.interpreter = "WeechatPerlScriptLoader";
    perl_argv[0] = filename;
    perl_argv[1] = pkgname;
    perl_argv[2] = NULL;
    eval = weechat_perl_exec (plugin, &tempscript, 
			      WEECHAT_SCRIPT_EXEC_INT,
			      "weechat_perl_load_eval_file", perl_argv);
#else
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
    eval = weechat_perl_exec (&tempscript,
			      WEECHAT_SCRIPT_EXEC_INT,
			      "weechat_perl_load_eval_file",
                              perl_argv);
#endif
    if (!eval)
    {
	weechat_printf (NULL,
                        weechat_gettext ("%s%s: not enough memory to parse "
                                         "file \"%s\""),
                        weechat_prefix ("error"), "perl", filename);
	return 0;
    }
    
    if ( *eval != 0) 
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
#ifndef MULTIPLICITY
                            SvPV(perl_get_sv("WeechatPerlScriptLoader::"
                                             "weechat_perl_load_eval_file_error",
                                             FALSE), len)
#else
                            SvPV(perl_get_sv("weechat_perl_load_eval_file_error",
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

#ifndef MULTIPLICITY
    perl_current_script->interpreter = strdup (pkgname);
#else
    perl_current_script->interpreter = (PerlInterpreter *)perl_current_interpreter;
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
    
#ifndef MULTIPLICITY
    eval_pv (script->interpreter, TRUE);
#else
    PERL_SET_CONTEXT (script->interpreter);
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
    
#ifndef MULTIPLICITY
    if (script->interpreter)
	free (script->interpreter);
#else
    perl_destruct (script->interpreter);
    perl_free (script->interpreter);
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
    //int handler_found, modifier_found;
    char *path_script;
    struct t_plugin_script *ptr_script;
    //struct t_plugin_handler *ptr_handler;
    //struct t_plugin_modifier *ptr_modifier;

    /* make C compiler happy */
    (void) data;
    (void) buffer;
    
    if (argc == 1)
    {
        /* list registered Perl scripts */
        weechat_printf (NULL, "");
        weechat_printf (NULL,
                        weechat_gettext ("Registered %s scripts:"),
                        "perl");
        if (perl_scripts)
        {
            for (ptr_script = perl_scripts; ptr_script;
                 ptr_script = ptr_script->next_script)
            {
                weechat_printf (NULL,
                                weechat_gettext ("  %s v%s (%s), by %s, "
                                                 "license %s"),
                                ptr_script->name,
                                ptr_script->version,
                                ptr_script->description,
                                ptr_script->author,
                                ptr_script->license);
            }
        }
        else
            weechat_printf (NULL, weechat_gettext ("  (none)"));

        /*
        // list Perl message handlers
        plugin->print_server (plugin, "");
        plugin->print_server (plugin, "Perl message handlers:");
        handler_found = 0;
        for (ptr_handler = plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if ((ptr_handler->type == PLUGIN_HANDLER_MESSAGE)
                && (ptr_handler->handler_args))
            {
                handler_found = 1;
                plugin->print_server (plugin, "  IRC(%s) => Perl(%s)",
                                      ptr_handler->irc_command,
                                      ptr_handler->handler_args);
            }
        }
        if (!handler_found)
            plugin->print_server (plugin, "  (none)");
        
        // list Perl command handlers
        plugin->print_server (plugin, "");
        plugin->print_server (plugin, "Perl command handlers:");
        handler_found = 0;
        for (ptr_handler = plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if ((ptr_handler->type == PLUGIN_HANDLER_COMMAND)
                && (ptr_handler->handler_args))
            {
                handler_found = 1;
                plugin->print_server (plugin, "  /%s => Perl(%s)",
                                      ptr_handler->command,
                                      ptr_handler->handler_args);
            }
        }
        if (!handler_found)
            plugin->print_server (plugin, "  (none)");
        
        // list Perl timer handlers
        plugin->print_server (plugin, "");
        plugin->print_server (plugin, "Perl timer handlers:");
        handler_found = 0;
        for (ptr_handler = plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if ((ptr_handler->type == PLUGIN_HANDLER_TIMER)
                && (ptr_handler->handler_args))
            {
                handler_found = 1;
                plugin->print_server (plugin, "  %d seconds => Perl(%s)",
                                      ptr_handler->interval,
                                      ptr_handler->handler_args);
            }
        }
        if (!handler_found)
            plugin->print_server (plugin, "  (none)");
        
        // list Perl keyboard handlers
        plugin->print_server (plugin, "");
        plugin->print_server (plugin, "Perl keyboard handlers:");
        handler_found = 0;
        for (ptr_handler = plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if ((ptr_handler->type == PLUGIN_HANDLER_KEYBOARD)
                && (ptr_handler->handler_args))
            {
                handler_found = 1;
                plugin->print_server (plugin, "  Perl(%s)",
                                      ptr_handler->handler_args);
            }
        }
        if (!handler_found)
            plugin->print_server (plugin, "  (none)");
        
        // list Perl event handlers
        plugin->print_server (plugin, "");
        plugin->print_server (plugin, "Perl event handlers:");
        handler_found = 0;
        for (ptr_handler = plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if ((ptr_handler->type == PLUGIN_HANDLER_EVENT)
                && (ptr_handler->handler_args))
            {
                handler_found = 1;
                plugin->print_server (plugin, "  %s => Perl(%s)",
                                      ptr_handler->event,
                                      ptr_handler->handler_args);
            }
        }
        if (!handler_found)
            plugin->print_server (plugin, "  (none)");
        
        // list Perl modifiers
        plugin->print_server (plugin, "");
        plugin->print_server (plugin, "Perl modifiers:");
        modifier_found = 0;
        for (ptr_modifier = plugin->modifiers;
             ptr_modifier; ptr_modifier = ptr_modifier->next_modifier)
        {
            modifier_found = 1;
            if (ptr_modifier->type == PLUGIN_MODIFIER_IRC_IN)
                plugin->print_server (plugin, "  IRC(%s, %s) => Perl(%s)",
                                      ptr_modifier->command,
                                      PLUGIN_MODIFIER_IRC_IN_STR,
                                      ptr_modifier->modifier_args);
            else if (ptr_modifier->type == PLUGIN_MODIFIER_IRC_USER)
                plugin->print_server (plugin, "  IRC(%s, %s) => Perl(%s)",
                                      ptr_modifier->command,
                                      PLUGIN_MODIFIER_IRC_USER_STR,
                                      ptr_modifier->modifier_args);
            else if (ptr_modifier->type == PLUGIN_MODIFIER_IRC_OUT)
                plugin->print_server (plugin, "  IRC(%s, %s) => Perl(%s)",
                                      ptr_modifier->command,
                                      PLUGIN_MODIFIER_IRC_OUT_STR,
                                      ptr_modifier->modifier_args);
        }
        if (!modifier_found)
          plugin->print_server (plugin, "  (none)");
        */
    }
    else if (argc == 2)
    {
        if (weechat_strcasecmp (argv[1], "autoload") == 0)
        {
            script_auto_load (weechat_perl_plugin,
                              "perl", &weechat_perl_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "reload") == 0)
        {
            weechat_perl_unload_all ();
            script_auto_load (weechat_perl_plugin,
                              "perl", &weechat_perl_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "unload") == 0)
        {
            weechat_perl_unload_all ();
        }
    }
    else
    {
        if (weechat_strcasecmp (argv[1], "load") == 0)
        {
            /* load Perl script */
            path_script = script_search_full_name (weechat_perl_plugin,
                                                   "perl", argv_eol[2]);
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
 * weechat_perl_dump_data_cb: dump Perl plugin data in WeeChat log file
 */

int
weechat_perl_dump_data_cb (void *data, char *signal, char *type_data,
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
    weechat_perl_plugin = plugin;
    
#ifndef MULTIPLICITY
    char *perl_args[] = { "", "-e", "0" };
    
    perl_main = perl_alloc ();
    
    if (!perl_main)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to initialize %s"),
                        weechat_prefix ("error"), "perl", "perl");
        return WEECHAT_RC_ERROR;
    }
    
    perl_construct (perl_main);
    perl_parse (perl_main, weechat_perl_xs_init, 3, perl_args, NULL);
    eval_pv (perl_weechat_code, TRUE);
#endif
    
    weechat_hook_command ("perl",
                          weechat_gettext ("list/load/unload Perl scripts"),
                          weechat_gettext ("[load filename] | [autoload] | "
                                           "[reload] | [unload [script]]"),
                          weechat_gettext ("filename: Perl script (file) to "
                                           "load\n"
                                           "script: script name to unload\n\n"
                                           "Without argument, /perl command "
                                           "lists all loaded Perl scripts."),
                          "load|autoload|reload|unload %f",
                          &weechat_perl_command_cb, NULL);
    
    weechat_mkdir_home ("perl", 0644);
    weechat_mkdir_home ("perl/autoload", 0644);
    
    weechat_hook_signal ("dump_data", &weechat_perl_dump_data_cb, NULL);
    
    script_init (weechat_perl_plugin);
    script_auto_load (weechat_perl_plugin,
                      "perl", &weechat_perl_load_cb);
    
    /* init ok */
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end Perl plugin
 */

void
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
}
