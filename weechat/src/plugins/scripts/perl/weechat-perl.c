/*
 * Copyright (c) 2003-2005 by FlashCode <flashcode@flashtux.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* weechat-perl.c: Perl plugin support for WeeChat */


#include <stdlib.h>
#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>
#undef _
#include "../../weechat-plugin.h"
#include "../weechat-script.h"

char plugin_name[]        = "Perl";
char plugin_version[]     = "0.1";
char plugin_description[] = "Perl scripts support";

t_weechat_plugin *perl_plugin;

t_plugin_script *perl_scripts = NULL;
t_plugin_script *perl_current_script = NULL;
char *perl_current_script_filename = NULL;

extern void boot_DynaLoader (pTHX_ CV* cv);

#ifdef NO_PERL_MULTIPLICITY
#undef MULTIPLICITY
#endif

#ifndef MULTIPLICITY
#define PKG_NAME_PREFIX "WeechatPerlPackage"
static PerlInterpreter *main_perl = NULL;
int packnum = 0;
#endif

char *weechat_perl_code =
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

int
weechat_perl_exec (t_weechat_plugin *plugin,
                   t_plugin_script *script,
                   char *function, char *server, char *arguments)
{
    char empty_server[1] = { '\0' };
    char *func;
    char *argv[3];
    unsigned int count;
    int return_code;
    SV *sv;
    
#ifndef MULTIPLICITY
    int size = strlen(script->interpreter) + strlen(function) + 3;
    func = (char *) malloc ( size * sizeof(char));
    if (func == NULL)
	return PLUGIN_RC_KO;
    snprintf(func, size, "%s::%s", (char *) script->interpreter, function);
#else
    func = function;
    PERL_SET_CONTEXT (script->interpreter);
#endif    
    
    dSP;
    ENTER;
    SAVETMPS;
    PUSHMARK(sp);
    if (!server)
        argv[0] = empty_server;
    else
        argv[0] = server;
    argv[1] = arguments;
    argv[2] = NULL;
    
    perl_current_script = script;
    
    count = perl_call_argv (func, G_EVAL | G_SCALAR, argv);
    
    SPAGAIN;
    
    sv = GvSV (gv_fetchpv ("@", TRUE, SVt_PV));
    return_code = PLUGIN_RC_KO;
    if (SvTRUE (sv))
    {
        plugin->printf_server (plugin, "Perl error: %s", SvPV (sv, count));
        POPs;
    }
    else
    {
        if (count != 1)
        {
            plugin->printf_server (plugin,
                                   "Perl error: too much values from \"%s\" (%d). Expected: 1.",
                                   function, count);
        }
        else
            return_code = POPi;
    }
    
    PUTBACK;
    FREETMPS;
    LEAVE;

#ifndef MULTIPLICITY
    free(func);
#endif
    
    return return_code;
}

/*
 * weechat_perl_handler: general message and command handler for Perl
 */

int
weechat_perl_handler (t_weechat_plugin *plugin,
                      char *server, char *command, char *arguments,
                      char *handler_args, void *handler_pointer)
{
    /* make gcc happy */
    (void) command;
    
    return weechat_perl_exec (plugin, (t_plugin_script *)handler_pointer,
                              handler_args, server, arguments);
}

/*
 * weechat::register: startup function for all WeeChat Perl scripts
 */

static XS (XS_weechat_register)
{
    char *name, *version, *shutdown_func, *description;
    unsigned int integer;
    dXSARGS;
    
    /* make gcc happy */
    (void) items;
    (void) cv;
    
    if (items != 4)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: wrong parameters for "
                                    "\"register\" function");
        XSRETURN_NO;
    }
    
    name = SvPV (ST (0), integer);
    version = SvPV (ST (1), integer);
    shutdown_func = SvPV (ST (2), integer);
    description = SvPV (ST (3), integer);
    
    if (weechat_script_search (perl_plugin, &perl_scripts, name))
    {
        /* error: another script already exists with this name! */
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: unable to register "
                                    "\"%s\" script (another script "
                                    "already exists with this name)",
                                    name);
        XSRETURN_NO;
    }
    
    /* register script */
    perl_current_script = weechat_script_add (perl_plugin,
                                              &perl_scripts,
                                              (perl_current_script_filename) ?
					      perl_current_script_filename : "",
                                              name, version, shutdown_func,
                                              description);
    if (perl_current_script)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl: registered script \"%s\", "
                                    "version %s (%s)",
                                    name, version, description);
    }
    else
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: unable to load script "
                                    "\"%s\" (not enough memory)",
                                    name);
        XSRETURN_NO;
    }
    
    XSRETURN_YES;
}

/*
 * weechat::print: print message into a buffer (current or specified one)
 */

static XS (XS_weechat_print)
{
    unsigned int integer;
    char *message, *channel_name, *server_name;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;

    if (!perl_current_script)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: unable to print message, "
                                    "script not initialized");
	XSRETURN_NO;
    }

    if (items < 1)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: wrong parameters for "
                                    "\"print\" function");
        XSRETURN_NO;
    }
    
    message = SvPV (ST (0), integer);
    
    channel_name = NULL;
    server_name = NULL;
    
    if (items > 1)
    {
        channel_name = SvPV (ST (1), integer);
        if (items > 2)
            server_name = SvPV (ST (2), integer);
    }
    
    perl_plugin->printf (perl_plugin,
                         server_name, channel_name,
                         "%s", message);
    
    XSRETURN_YES;
}

/*
 * weechat::print_infobar: print message to infobar
 */

static XS (XS_weechat_print_infobar)
{
    unsigned int integer;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: unable to print infobar message, "
                                    "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 2)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: wrong parameters for "
                                    "\"print_infobar\" function");
        XSRETURN_NO;
    }
    
    perl_plugin->infobar_printf (perl_plugin,
                                 SvIV (ST (0)),
                                 SvPV (ST (1), integer));
    
    XSRETURN_YES;
}

/*
 * weechat::command: send command to server
 */

static XS (XS_weechat_command)
{
    unsigned int integer;
    char *channel_name, *server_name;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: unable to run command, "
                                    "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 1)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: wrong parameters for "
                                    "\"command\" function");
        XSRETURN_NO;
    }
    
    channel_name = NULL;
    server_name = NULL;
    
    if (items > 1)
    {
        channel_name = SvPV (ST (1), integer);
        if (items > 2)
            server_name = SvPV (ST (2), integer);
    }
    
    perl_plugin->exec_command (perl_plugin,
                               server_name, channel_name,
                               SvPV (ST (0), integer));
    
    XSRETURN_YES;
}

/*
 * weechat::add_message_handler: add handler for messages (privmsg, ...)
 */

static XS (XS_weechat_add_message_handler)
{
    char *irc_command, *function;
    unsigned int integer;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: unable to add message handler, "
                                    "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 2)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: wrong parameters for "
                                    "\"add_message_handler\" function");
        XSRETURN_NO;
    }
    
    irc_command = SvPV (ST (0), integer);
    function = SvPV (ST (1), integer);
    
    if (perl_plugin->msg_handler_add (perl_plugin, irc_command,
                                      weechat_perl_handler, function,
                                      (void *)perl_current_script))
        XSRETURN_YES;
    
    XSRETURN_NO;
}

/*
 * weechat::add_command_handler: add command handler (define/redefine commands)
 */

static XS (XS_weechat_add_command_handler)
{
    char *command, *function, *description, *arguments, *arguments_description;
    char *completion_template;
    unsigned int integer;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: unable to add command handler, "
                                    "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 2)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: wrong parameters for "
                                    "\"add_command_handler\" function");
        XSRETURN_NO;
    }
    
    command = SvPV (ST (0), integer);
    function = SvPV (ST (1), integer);
    description = (items >= 3) ? SvPV (ST (2), integer) : NULL;
    arguments = (items >= 4) ? SvPV (ST (3), integer) : NULL;
    arguments_description = (items >= 5) ? SvPV (ST (4), integer) : NULL;
    completion_template = (items >= 6) ? SvPV (ST (5), integer) : NULL;
    
    if (perl_plugin->cmd_handler_add (perl_plugin,
                                      command,
                                      description,
                                      arguments,
                                      arguments_description,
                                      completion_template,
                                      weechat_perl_handler,
                                      function,
                                      (void *)perl_current_script))
        XSRETURN_YES;
    
    XSRETURN_NO;
}

/*
 * weechat::remove_handler: remove a handler
 */

static XS (XS_weechat_remove_handler)
{
    char *command, *function;
    unsigned int integer;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: unable to remove handler, "
                                    "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 2)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: wrong parameters for "
                                    "\"remove_handler\" function");
        XSRETURN_NO;
    }
    
    command = SvPV (ST (0), integer);
    function = SvPV (ST (1), integer);
    
    weechat_script_remove_handler (perl_plugin, perl_current_script,
                                   command, function);
    
    XSRETURN_YES;
}

/*
 * weechat::get_info: get various infos
 */

static XS (XS_weechat_get_info)
{
    char *arg, *info, *server_name;
    unsigned int integer;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: unable to get info, "
                                    "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 1)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: wrong parameters for "
                                    "\"get_info\" function");
        XSRETURN_NO;
    }
    
    server_name = (items == 2) ? SvPV (ST (1), integer) : NULL;
    
    arg = SvPV (ST (0), integer);
    if (arg)
    {
        info = perl_plugin->get_info (perl_plugin, arg, server_name);
        
        if (info)
        {
            XST_mPV (0, info);
            free (info);
            XSRETURN (1);
        }
    }
    
    XST_mPV (0, "");
    XSRETURN (1);
}

/*
 * weechat::get_dcc_info: get infos about DCC
 */

static XS (XS_weechat_get_dcc_info)
{
    t_plugin_dcc_info *dcc_info, *ptr_dcc;
    int dcc_count;
    char timebuffer1[64];
    char timebuffer2[64];
    struct in_addr in;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    (void) items;
    
    if (!perl_current_script)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: unable to get DCC info, "
                                    "script not initialized");
	XSRETURN_NO;
    }
    
    dcc_info = perl_plugin->get_dcc_info (perl_plugin);
    dcc_count = 0;
    
    if (!dcc_info)
        XSRETURN_NO;
    
    for (ptr_dcc = dcc_info; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
    {
	strftime(timebuffer1, sizeof(timebuffer1), "%F %T",
		 localtime(&ptr_dcc->start_time));
	strftime(timebuffer2, sizeof(timebuffer2), "%F %T",
		 localtime(&ptr_dcc->start_transfer));
	in.s_addr = htonl(ptr_dcc->addr);

        HV *infohash = (HV *) sv_2mortal((SV *) newHV());
        
        hv_store (infohash, "server",           6, newSVpv (ptr_dcc->server, 0), 0);
        hv_store (infohash, "channel",          7, newSVpv (ptr_dcc->channel, 0), 0);
        hv_store (infohash, "type",             4, newSViv (ptr_dcc->type), 0);
        hv_store (infohash, "status",           6, newSViv (ptr_dcc->status), 0);
        hv_store (infohash, "start_time",      10, newSVpv (timebuffer1, 0), 0);
        hv_store (infohash, "start_transfer",  14, newSVpv (timebuffer2, 0), 0);
        hv_store (infohash, "address",          7, newSVpv (inet_ntoa(in), 0), 0);
        hv_store (infohash, "port",             4, newSViv (ptr_dcc->port), 0);
        hv_store (infohash, "nick",             4, newSVpv (ptr_dcc->nick, 0), 0);
        hv_store (infohash, "remote_file",     11, newSVpv (ptr_dcc->filename, 0), 0);
        hv_store (infohash, "local_file",      10, newSVpv (ptr_dcc->local_filename, 0), 0);
        hv_store (infohash, "filename_suffix", 15, newSViv (ptr_dcc->filename_suffix), 0);
        hv_store (infohash, "size",             4, newSVnv (ptr_dcc->size), 0);
        hv_store (infohash, "pos",              3, newSVnv (ptr_dcc->pos), 0);
        hv_store (infohash, "start_resume",    12, newSVnv (ptr_dcc->start_resume), 0);
        hv_store (infohash, "cps",              3, newSViv (ptr_dcc->bytes_per_sec), 0);
        
        XPUSHs(newRV((SV *) infohash));
        dcc_count++;
    }
    
    perl_plugin->free_dcc_info (perl_plugin, dcc_info);
    
    XSRETURN (dcc_count);
}

/*
 * weechat::get_config: get value of a WeeChat config option
 */

static XS (XS_weechat_get_config)
{
    char *option, *return_value;
    unsigned int integer;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: unable to get config option, "
                                    "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 1)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: wrong parameters for "
                                    "\"get_config\" function");
        XSRETURN_NO;
    }
    
    option = SvPV (ST (0), integer);
    
    if (option)
    {
        return_value = perl_plugin->get_config (perl_plugin, option);
        
        if (return_value)
        {
            XST_mPV (0, return_value);
            free (return_value);
            XSRETURN (1);
        }
    }
    
    XST_mPV (0, "");
    XSRETURN (1);
}

/*
 * weechat::set_config: set value of a WeeChat config option
 */

static XS (XS_weechat_set_config)
{
    char *option, *value;
    unsigned int integer;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: unable to set config option, "
                                    "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 2)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: wrong parameters for "
                                    "\"set_config\" function");
        XSRETURN_NO;
    }
    
    option = SvPV (ST (0), integer);
    value = SvPV (ST (1), integer);
    
    if (option && value)
    {
        if (perl_plugin->set_config (perl_plugin, option, value))
            XSRETURN_YES;
    }
    
    XSRETURN_NO;
}

/*
 * weechat::get_plugin_config: get value of a plugin config option
 */

static XS (XS_weechat_get_plugin_config)
{
    char *option, *return_value;
    unsigned int integer;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: unable to get plugin config option, "
                                    "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 1)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: wrong parameters for "
                                    "\"get_plugin_config\" function");
        XSRETURN_NO;
    }
    
    option = SvPV (ST (0), integer);
    
    if (option)
    {
        return_value = weechat_script_get_plugin_config (perl_plugin,
                                                         perl_current_script,
                                                         option);
        
        if (return_value)
        {
            XST_mPV (0, return_value);
            free (return_value);
            XSRETURN (1);
        }
    }    
    
    XST_mPV (0, "");
    XSRETURN (1);
}

/*
 * weechat::set_plugin_config: set value of a WeeChat config option
 */

static XS (XS_weechat_set_plugin_config)
{
    char *option, *value;
    unsigned int integer;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: unable to set plugin config option, "
                                    "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 2)
    {
        perl_plugin->printf_server (perl_plugin,
                                    "Perl error: wrong parameters for "
                                    "\"set_plugin_config\" function");
        XSRETURN_NO;
    }
    
    option = SvPV (ST (0), integer);
    value = SvPV (ST (1), integer);
    
    if (option && value)
    {
        if (weechat_script_set_plugin_config (perl_plugin,
                                              perl_current_script,
                                              option, value))
            XSRETURN_YES;
    }
    
    XSRETURN_NO;
}

/*
 * weechat_perl_xs_init: initialize subroutines
 */

void
weechat_perl_xs_init (pTHX)
{
    HV *stash;
    
    newXS ("DynaLoader::boot_DynaLoader", boot_DynaLoader, __FILE__);
    
    /* interface functions */
    newXS ("weechat::register", XS_weechat_register, "weechat");
    newXS ("weechat::print", XS_weechat_print, "weechat");
    newXS ("weechat::print_infobar", XS_weechat_print_infobar, "weechat");
    newXS ("weechat::command", XS_weechat_command, "weechat");
    newXS ("weechat::add_message_handler", XS_weechat_add_message_handler, "weechat");
    newXS ("weechat::add_command_handler", XS_weechat_add_command_handler, "weechat");
    newXS ("weechat::remove_handler", XS_weechat_remove_handler, "weechat");
    newXS ("weechat::get_info", XS_weechat_get_info, "weechat");
    newXS ("weechat::get_dcc_info", XS_weechat_get_dcc_info, "weechat");
    newXS ("weechat::get_config", XS_weechat_get_config, "weechat");
    newXS ("weechat::set_config", XS_weechat_set_config, "weechat");
    newXS ("weechat::get_plugin_config", XS_weechat_get_plugin_config, "weechat");
    newXS ("weechat::set_plugin_config", XS_weechat_set_plugin_config, "weechat");
    
    /* interface constants */
    stash = gv_stashpv ("weechat", TRUE);
    newCONSTSUB (stash, "weechat::PLUGIN_RC_KO", newSViv (PLUGIN_RC_KO));
    newCONSTSUB (stash, "weechat::PLUGIN_RC_OK", newSViv (PLUGIN_RC_OK));
    newCONSTSUB (stash, "weechat::PLUGIN_RC_OK_IGNORE_WEECHAT", newSViv (PLUGIN_RC_OK_IGNORE_WEECHAT));
    newCONSTSUB (stash, "weechat::PLUGIN_RC_OK_IGNORE_PLUGINS", newSViv (PLUGIN_RC_OK_IGNORE_PLUGINS));
    newCONSTSUB (stash, "weechat::PLUGIN_RC_OK_IGNORE_ALL", newSViv (PLUGIN_RC_OK_IGNORE_ALL));
}

/*
 * weechat_perl_load: load a Perl script
 */

int
weechat_perl_load (t_weechat_plugin *plugin, char *filename)
{
    STRLEN len;
    t_plugin_script tempscript;
    int eval;

#ifndef MULTIPLICITY
    char pkgname[64];
#else
    PerlInterpreter *perl_current_interpreter;
    char *perl_args[] = { "", "-e", "0" };    
#endif
    
    plugin->printf_server (plugin, "Loading Perl script \"%s\"", filename);
    perl_current_script = NULL;

#ifndef MULTIPLICITY
    snprintf(pkgname, sizeof(pkgname), "%s%d", PKG_NAME_PREFIX, packnum);
    packnum++;
    tempscript.interpreter = "WeechatPerlScriptLoader";
    eval = weechat_perl_exec (plugin, &tempscript, "weechat_perl_load_eval_file", filename, pkgname);
#else
    perl_current_interpreter = perl_alloc();

    if (perl_current_interpreter == NULL)
    {
        plugin->printf_server (plugin,
                               "Perl error: unable to create new sub-interpreter");
        return 0;
    }

    perl_current_script_filename = strdup (filename);

    PERL_SET_CONTEXT (perl_current_interpreter);
    perl_construct (perl_current_interpreter);
    tempscript.interpreter = (PerlInterpreter *) perl_current_interpreter;
    perl_parse (perl_current_interpreter, weechat_perl_xs_init, 3, perl_args, NULL);
    
    eval_pv (weechat_perl_code, TRUE);
    eval = weechat_perl_exec (plugin, &tempscript, "weechat_perl_load_eval_file", filename, "");

    free (perl_current_script_filename);

#endif
    
    if ( eval != 0) 
    {
	if (eval == 2) 
	{
	    plugin->printf_server (plugin,
				   "Perl error: unable to parse file \"%s\"",
				   filename);	    
	    plugin->printf_server (plugin,
				   "Perl error: %s",
#ifndef MULTIPLICITY
				   SvPV(perl_get_sv("WeechatPerlScriptLoader::weechat_perl_load_eval_file_error", FALSE), len));
#else
				   SvPV(perl_get_sv("weechat_perl_load_eval_file_error", FALSE), len));
#endif
	}
	else if ( eval == 1)
	{
	    plugin->printf_server (plugin,
				   "Perl error: unable to run file \"%s\"",
				   filename);
	}
	else {
	    plugin->printf_server (plugin,
				   "Perl error: unknown error while loading file \"%s\"",
				   filename);
	}
#ifdef MULTIPLICITY
	perl_destruct (perl_current_interpreter);
        perl_free (perl_current_interpreter);
#endif
	if ((perl_current_script != NULL) && (perl_current_script != &tempscript))
            weechat_script_remove (plugin, &perl_scripts, perl_current_script);

	return 0;
    }
    
    if (perl_current_script == NULL) {
	plugin->printf_server (plugin,
                               "Perl error: function \"register\" not found "
                               "in file \"%s\"",
                               filename);
#ifdef MULTIPLICITY
        perl_destruct (perl_current_interpreter);
        perl_free (perl_current_interpreter);
#endif
        return 0;
    }

#ifndef MULTIPLICITY
    perl_current_script->interpreter = strdup(pkgname);
#else
    perl_current_script->interpreter = (PerlInterpreter *) perl_current_interpreter;
#endif

    return 1;
}

/*
 * weechat_perl_unload: unload a Perl script
 */

void
weechat_perl_unload (t_weechat_plugin *plugin, t_plugin_script *script)
{
    plugin->printf_server (plugin,
                           "Unloading Perl script \"%s\"",
                           script->name);

#ifndef MULTIPLICITY
    eval_pv(script->interpreter, TRUE);
#else
    PERL_SET_CONTEXT (script->interpreter);
#endif        

    if (script->shutdown_func[0])
        weechat_perl_exec (plugin, script, script->shutdown_func, "", "");

#ifndef MULTIPLICITY
    if (script->interpreter)
	free (script->interpreter);
#else
    perl_destruct (script->interpreter);
    perl_free (script->interpreter);
#endif
   
    weechat_script_remove (plugin, &perl_scripts, script);
}

/*
 * weechat_perl_unload_name: unload a Perl script by name
 */

void
weechat_perl_unload_name (t_weechat_plugin *plugin, char *name)
{
    t_plugin_script *ptr_script;
    
    ptr_script = weechat_script_search (plugin, &perl_scripts, name);
    if (ptr_script)
    {
        weechat_perl_unload (plugin, ptr_script);
        plugin->printf_server (plugin,
                               "Perl script \"%s\" unloaded",
                               name);
    }
    else
    {
        plugin->printf_server (plugin,
                               "Perl error: script \"%s\" not loaded",
                               name);
    }
}

/*
 * weechat_perl_unload_all: unload all Perl scripts
 */

void
weechat_perl_unload_all (t_weechat_plugin *plugin)
{
    plugin->printf_server (plugin,
                           "Unloading all Perl scripts");
    while (perl_scripts)
      weechat_perl_unload (plugin, perl_scripts);

    plugin->printf_server (plugin,
                           "Perl scripts unloaded");
}

/*
 * weechat_perl_cmd: /perl command handler
 */

int
weechat_perl_cmd (t_weechat_plugin *plugin,
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
            /* list registered Perl scripts */
            plugin->printf_server (plugin, "");
            plugin->printf_server (plugin, "Registered Perl scripts:");
            if (perl_scripts)
            {
                for (ptr_script = perl_scripts;
                     ptr_script; ptr_script = ptr_script->next_script)
                {
                    plugin->printf_server (plugin, "  %s v%s%s%s",
                                           ptr_script->name,
                                           ptr_script->version,
                                           (ptr_script->description[0]) ? " - " : "",
                                           ptr_script->description);
                }
            }
            else
                plugin->printf_server (plugin, "  (none)");
            
            /* list Perl message handlers */
            plugin->printf_server (plugin, "");
            plugin->printf_server (plugin, "Perl message handlers:");
            handler_found = 0;
            for (ptr_handler = plugin->handlers;
                 ptr_handler; ptr_handler = ptr_handler->next_handler)
            {
                if ((ptr_handler->type == HANDLER_MESSAGE)
                    && (ptr_handler->handler_args))
                {
                    handler_found = 1;
                    plugin->printf_server (plugin, "  IRC(%s) => Perl(%s)",
                                           ptr_handler->irc_command,
                                           ptr_handler->handler_args);
                }
            }
            if (!handler_found)
                plugin->printf_server (plugin, "  (none)");
            
            /* list Perl command handlers */
            plugin->printf_server (plugin, "");
            plugin->printf_server (plugin, "Perl command handlers:");
            handler_found = 0;
            for (ptr_handler = plugin->handlers;
                 ptr_handler; ptr_handler = ptr_handler->next_handler)
            {
                if ((ptr_handler->type == HANDLER_COMMAND)
                    && (ptr_handler->handler_args))
                {
                    handler_found = 1;
                    plugin->printf_server (plugin, "  /%s => Perl(%s)",
                                           ptr_handler->command,
                                           ptr_handler->handler_args);
                }
            }
            if (!handler_found)
                plugin->printf_server (plugin, "  (none)");
            break;
        case 1:
            if (plugin->ascii_strcasecmp (plugin, argv[0], "autoload") == 0)
                weechat_script_auto_load (plugin, "perl", weechat_perl_load);
            else if (plugin->ascii_strcasecmp (plugin, argv[0], "reload") == 0)
            {
                weechat_perl_unload_all (plugin);
                weechat_script_auto_load (plugin, "perl", weechat_perl_load);
            }
            else if (plugin->ascii_strcasecmp (plugin, argv[0], "unload") == 0)
                weechat_perl_unload_all (plugin);
            break;
        case 2:
            if (plugin->ascii_strcasecmp (plugin, argv[0], "load") == 0)
            {
                /* load Perl script */
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
                            snprintf (path_script, path_length, "%s/perl/%s",
                                      dir_home, argv[1]);
                        else
                            path_script = NULL;
                        free (dir_home);
                    }
                    else
                        path_script = NULL;
                }
                weechat_perl_load (plugin, (path_script) ? path_script : argv[1]);
                if (path_script)
                    free (path_script);
            }
	    else if (plugin->ascii_strcasecmp (plugin, argv[0], "unload") == 0)
            {
                /* unload Perl script */
                weechat_perl_unload_name (plugin, argv[1]);
            }
            else
            {
                plugin->printf_server (plugin,
                                       "Perl error: unknown option for "
                                       "\"perl\" command");
            }
            break;
        default:
            plugin->printf_server (plugin,
                                   "Perl error: wrong argument count for \"perl\" command");
    }
    
    if (argv)
        plugin->free_exploded_string (plugin, argv);
    
    return 1;
}

/*
 * weechat_plugin_init: initialize Perl plugin
 */

int
weechat_plugin_init (t_weechat_plugin *plugin)
{
    perl_plugin = plugin;

#ifdef MULTIPLICITY
    plugin->printf_server (plugin, "Loading Perl module \"weechat\"");
#else
    plugin->printf_server (plugin, "Loading Perl module \"weechat\" (without multiplicity)");

    char *perl_args[] = { "", "-e", "0" };
    
    main_perl = perl_alloc ();
    
    if (!main_perl)
    {
        plugin->printf_server (plugin,
                               "Perl error: unable to initialize Perl");
        return PLUGIN_RC_KO;
    }
    
    perl_construct (main_perl);
    perl_parse (main_perl, weechat_perl_xs_init, 3, perl_args, NULL);
    eval_pv (weechat_perl_code, TRUE);
#endif
    
    plugin->cmd_handler_add (plugin, "perl",
                             "list/load/unload Perl scripts",
                             "[load filename] | [autoload] | [reload] | [unload]",
                             "filename: Perl script (file) to load\n\n"
                             "Without argument, /perl command lists all loaded Perl scripts.",
                             "load|autoload|reload|unload",
                             weechat_perl_cmd, NULL, NULL);
    
    plugin->mkdir_home (plugin, "perl");
    plugin->mkdir_home (plugin, "perl/autoload");
    
    weechat_script_auto_load (plugin, "perl", weechat_perl_load);
    
    /* init ok */
    return PLUGIN_RC_OK;
}

/*
 * weechat_plugin_end: shutdown Perl interface
 */

void
weechat_plugin_end (t_weechat_plugin *plugin)
{
    /* unload all scripts */
    weechat_perl_unload_all (plugin);

#ifndef MULTIPLICITY
    /* free perl intepreter */
    if (main_perl)
    {
	perl_destruct (main_perl);
	perl_free (main_perl);
	main_perl = NULL;
    }
#endif
    
    perl_plugin->printf_server (perl_plugin,
                                "Perl plugin ended");
}
