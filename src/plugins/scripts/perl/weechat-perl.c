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

/* weechat-perl.c: Perl plugin support for WeeChat */

#undef _

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

int
weechat_perl_exec (t_weechat_plugin *plugin,
                   t_plugin_script *script,
                   char *function, char *arg1, char *arg2, char *arg3)
{
    char empty_arg[1] = { '\0' };
    char *func;
    char *argv[4];
    unsigned int count;
    int return_code;
    SV *sv;

    /* this code is placed here to conform ISO C90 */
    dSP;
    
#ifndef MULTIPLICITY
    int size = strlen (script->interpreter) + strlen(function) + 3;
    func = (char *) malloc ( size * sizeof(char));
    if (!func)
	return PLUGIN_RC_KO;
    snprintf (func, size, "%s::%s", (char *) script->interpreter, function);
#else
    func = function;
    PERL_SET_CONTEXT (script->interpreter);
#endif    
    
    ENTER;
    SAVETMPS;
    PUSHMARK(sp);
    if (arg1)
    {
        argv[0] = (arg1) ? arg1 : empty_arg;
        if (arg2)
        {
            argv[1] = (arg2) ? arg2 : empty_arg;
            if (arg3)
            {
                argv[2] = (arg3) ? arg3 : empty_arg;
                argv[3] = NULL;
            }
            else
                argv[2] = NULL;
        }
        else
            argv[1] = NULL;
    }
    else
        argv[0] = NULL;
    
    perl_current_script = script;
    
    count = perl_call_argv (func, G_EVAL | G_SCALAR, argv);
    
    SPAGAIN;
    
    sv = GvSV (gv_fetchpv ("@", TRUE, SVt_PV));
    return_code = PLUGIN_RC_KO;
    if (SvTRUE (sv))
    {
        plugin->print_server (plugin, "Perl error: %s", SvPV_nolen (sv));
        POPs;
    }
    else
    {
        if (count != 1)
        {
            plugin->print_server (plugin,
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
    free (func);
#endif
    
    return return_code;
}

/*
 * weechat_perl_cmd_msg_handler: general command/message handler for Perl
 */

int
weechat_perl_cmd_msg_handler (t_weechat_plugin *plugin,
                              int argc, char **argv,
                              char *handler_args, void *handler_pointer)
{
    if (argc >= 3)
        return weechat_perl_exec (plugin, (t_plugin_script *)handler_pointer,
                                  handler_args, argv[0], argv[2], NULL);
    else
        return PLUGIN_RC_KO;
}

/*
 * weechat_perl_timer_handler: general timer handler for Perl
 */

int
weechat_perl_timer_handler (t_weechat_plugin *plugin,
                            int argc, char **argv,
                            char *handler_args, void *handler_pointer)
{
    /* make gcc happy */
    (void) argc;
    (void) argv;
    
    return weechat_perl_exec (plugin, (t_plugin_script *)handler_pointer,
                              handler_args, NULL, NULL, NULL);
}

/*
 * weechat_perl_keyboard_handler: general keyboard handler for Perl
 */

int
weechat_perl_keyboard_handler (t_weechat_plugin *plugin,
                               int argc, char **argv,
                               char *handler_args, void *handler_pointer)
{
    if (argc >= 3)
        return weechat_perl_exec (plugin, (t_plugin_script *)handler_pointer,
                                  handler_args, argv[0], argv[1], argv[2]);
    else
        return PLUGIN_RC_KO;
}

/*
 * weechat::register: startup function for all WeeChat Perl scripts
 */

static XS (XS_weechat_register)
{
    char *name, *version, *shutdown_func, *description;
    dXSARGS;
    
    /* make gcc happy */
    (void) items;
    (void) cv;

    perl_current_script = NULL;
    
    if (items != 4)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"register\" function");
        XSRETURN_NO;
    }
    
    name = SvPV (ST (0), PL_na);
    version = SvPV (ST (1), PL_na);
    shutdown_func = SvPV (ST (2), PL_na);
    description = SvPV (ST (3), PL_na);
    
    if (weechat_script_search (perl_plugin, &perl_scripts, name))
    {
        /* error: another script already exists with this name! */
        perl_plugin->print_server (perl_plugin,
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
        perl_plugin->print_server (perl_plugin,
                                   "Perl: registered script \"%s\", "
                                   "version %s (%s)",
                                   name, version, description);
    }
    else
    {
        XSRETURN_NO;
    }
    
    XSRETURN_YES;
}

/*
 * weechat::print: print message into a buffer (current or specified one)
 */

static XS (XS_weechat_print)
{
    char *message, *channel_name, *server_name;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;

    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to print message, "
                                   "script not initialized");
	XSRETURN_NO;
    }

    if (items < 1)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"print\" function");
        XSRETURN_NO;
    }
    
    message = SvPV (ST (0), PL_na);
    
    channel_name = NULL;
    server_name = NULL;
    
    if (items > 1)
    {
        channel_name = SvPV (ST (1), PL_na);
        if (items > 2)
            server_name = SvPV (ST (2), PL_na);
    }
    
    perl_plugin->print (perl_plugin,
                        server_name, channel_name,
                        "%s", message);
    
    XSRETURN_YES;
}

/*
 * weechat::print_server: print message into a server buffer
 */

static XS (XS_weechat_print_server)
{
    char *message;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;

    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to print message, "
                                   "script not initialized");
	XSRETURN_NO;
    }

    if (items < 1)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"print_server\" function");
        XSRETURN_NO;
    }
    
    message = SvPV (ST (0), PL_na);
    
    perl_plugin->print_server (perl_plugin, "%s", message);
    
    XSRETURN_YES;
}

/*
 * weechat::print_infobar: print message to infobar
 */

static XS (XS_weechat_print_infobar)
{
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to print infobar message, "
                                   "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 2)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"print_infobar\" function");
        XSRETURN_NO;
    }
    
    perl_plugin->print_infobar (perl_plugin,
                                SvIV (ST (0)),
                                "%s",
                                SvPV (ST (1), PL_na));
    
    XSRETURN_YES;
}

/*
 * weechat::remove_infobar: remove message(s) from infobar
 */

static XS (XS_weechat_remove_infobar)
{
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to remove infobar message(s), "
                                   "script not initialized");
	XSRETURN_NO;
    }
    
    perl_plugin->infobar_remove (perl_plugin,
                                 (items >= 1) ? SvIV (ST (0)) : 0);
    
    XSRETURN_YES;
}

/*
 * weechat::log: log message in server/channel (current or specified ones)
 */

static XS (XS_weechat_log)
{
    char *message, *channel_name, *server_name;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;

    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to print message, "
                                   "script not initialized");
	XSRETURN_NO;
    }

    if (items < 1)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"log\" function");
        XSRETURN_NO;
    }
    
    message = SvPV (ST (0), PL_na);
    
    channel_name = NULL;
    server_name = NULL;
    
    if (items > 1)
    {
        channel_name = SvPV (ST (1), PL_na);
        if (items > 2)
            server_name = SvPV (ST (2), PL_na);
    }
    
    perl_plugin->log (perl_plugin,
		      server_name, channel_name,
		      "%s", message);
    
    XSRETURN_YES;
}

/*
 * weechat::command: send command to server
 */

static XS (XS_weechat_command)
{
    char *channel_name, *server_name;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to run command, "
                                   "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 1)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"command\" function");
        XSRETURN_NO;
    }
    
    channel_name = NULL;
    server_name = NULL;
    
    if (items > 1)
    {
        channel_name = SvPV (ST (1), PL_na);
        if (items > 2)
            server_name = SvPV (ST (2), PL_na);
    }
    
    perl_plugin->exec_command (perl_plugin,
                               server_name, channel_name,
                               SvPV (ST (0), PL_na));
    
    XSRETURN_YES;
}

/*
 * weechat::add_message_handler: add a handler for messages (privmsg, ...)
 */

static XS (XS_weechat_add_message_handler)
{
    char *irc_command, *function;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to add message handler, "
                                   "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 2)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"add_message_handler\" function");
        XSRETURN_NO;
    }
    
    irc_command = SvPV (ST (0), PL_na);
    function = SvPV (ST (1), PL_na);
    
    if (perl_plugin->msg_handler_add (perl_plugin, irc_command,
                                      weechat_perl_cmd_msg_handler, function,
                                      (void *)perl_current_script))
        XSRETURN_YES;
    
    XSRETURN_NO;
}

/*
 * weechat::add_command_handler: add a command handler (define/redefine commands)
 */

static XS (XS_weechat_add_command_handler)
{
    char *command, *function, *description, *arguments, *arguments_description;
    char *completion_template;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to add command handler, "
                                   "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 2)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"add_command_handler\" function");
        XSRETURN_NO;
    }
    
    command = SvPV (ST (0), PL_na);
    function = SvPV (ST (1), PL_na);
    description = (items >= 3) ? SvPV (ST (2), PL_na) : NULL;
    arguments = (items >= 4) ? SvPV (ST (3), PL_na) : NULL;
    arguments_description = (items >= 5) ? SvPV (ST (4), PL_na) : NULL;
    completion_template = (items >= 6) ? SvPV (ST (5), PL_na) : NULL;
    
    if (perl_plugin->cmd_handler_add (perl_plugin,
                                      command,
                                      description,
                                      arguments,
                                      arguments_description,
                                      completion_template,
                                      weechat_perl_cmd_msg_handler,
                                      function,
                                      (void *)perl_current_script))
        XSRETURN_YES;
    
    XSRETURN_NO;
}

/*
 * weechat::add_timer_handler: add a timer handler
 */

static XS (XS_weechat_add_timer_handler)
{
    int interval;
    char *function;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to add timer handler, "
                                   "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 2)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"add_timer_handler\" function");
        XSRETURN_NO;
    }
    
    interval = SvIV (ST (0));
    function = SvPV (ST (1), PL_na);
    
    if (perl_plugin->timer_handler_add (perl_plugin, interval,
                                        weechat_perl_timer_handler, function,
                                        (void *)perl_current_script))
        XSRETURN_YES;
    
    XSRETURN_NO;
}

/*
 * weechat::add_keyboard_handler: add a keyboard handler
 */

static XS (XS_weechat_add_keyboard_handler)
{
    char *function;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to add keyboard handler, "
                                   "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 1)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"add_keyboard_handler\" function");
        XSRETURN_NO;
    }
    
    function = SvPV (ST (0), PL_na);
    
    if (perl_plugin->keyboard_handler_add (perl_plugin,
                                           weechat_perl_keyboard_handler,
                                           function,
                                           (void *)perl_current_script))
        XSRETURN_YES;
    
    XSRETURN_NO;
}

/*
 * weechat::remove_handler: remove a message/command handler
 */

static XS (XS_weechat_remove_handler)
{
    char *command, *function;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to remove handler, "
                                   "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 2)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"remove_handler\" function");
        XSRETURN_NO;
    }
    
    command = SvPV (ST (0), PL_na);
    function = SvPV (ST (1), PL_na);
    
    weechat_script_remove_handler (perl_plugin, perl_current_script,
                                   command, function);
    
    XSRETURN_YES;
}

/*
 * weechat::remove_timer_handler: remove a timer handler
 */

static XS (XS_weechat_remove_timer_handler)
{
    char *function;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to remove timer handler, "
                                   "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 1)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"remove_timer_handler\" function");
        XSRETURN_NO;
    }
    
    function = SvPV (ST (0), PL_na);
    
    weechat_script_remove_timer_handler (perl_plugin, perl_current_script,
                                         function);
    
    XSRETURN_YES;
}

/*
 * weechat::remove_keyboard_handler: remove a keyboard handler
 */

static XS (XS_weechat_remove_keyboard_handler)
{
    char *function;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to remove keyboard handler, "
                                   "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 1)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"remove_keyboard_handler\" function");
        XSRETURN_NO;
    }
    
    function = SvPV (ST (0), PL_na);
    
    weechat_script_remove_keyboard_handler (perl_plugin, perl_current_script,
                                            function);
    
    XSRETURN_YES;
}

/*
 * weechat::get_info: get various infos
 */

static XS (XS_weechat_get_info)
{
    char *arg, *info, *server_name;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to get info, "
                                   "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 1)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"get_info\" function");
        XSRETURN_NO;
    }
    
    server_name = (items == 2) ? SvPV (ST (1), PL_na) : NULL;
    
    arg = SvPV (ST (0), PL_na);
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
    int count;
    char timebuffer1[64];
    char timebuffer2[64];
    struct in_addr in;
    HV *dcc_hash_member;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    (void) items;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to get DCC info, "
                                   "script not initialized");
	XSRETURN_EMPTY;
    }

    dcc_info = perl_plugin->get_dcc_info (perl_plugin);
    count = 0;
    if (!dcc_info)
	XSRETURN_EMPTY;
    
    for (ptr_dcc = dcc_info; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
    {
	strftime(timebuffer1, sizeof(timebuffer1), "%F %T",
		 localtime(&ptr_dcc->start_time));
	strftime(timebuffer2, sizeof(timebuffer2), "%F %T",
		 localtime(&ptr_dcc->start_transfer));
	in.s_addr = htonl(ptr_dcc->addr);

        dcc_hash_member = (HV *) sv_2mortal ((SV *) newHV());
        
        hv_store (dcc_hash_member, "server",           6, newSVpv (ptr_dcc->server, 0), 0);
        hv_store (dcc_hash_member, "channel",          7, newSVpv (ptr_dcc->channel, 0), 0);
        hv_store (dcc_hash_member, "type",             4, newSViv (ptr_dcc->type), 0);
        hv_store (dcc_hash_member, "status",           6, newSViv (ptr_dcc->status), 0);
        hv_store (dcc_hash_member, "start_time",      10, newSVpv (timebuffer1, 0), 0);
        hv_store (dcc_hash_member, "start_transfer",  14, newSVpv (timebuffer2, 0), 0);
        hv_store (dcc_hash_member, "address",          7, newSVpv (inet_ntoa(in), 0), 0);
        hv_store (dcc_hash_member, "port",             4, newSViv (ptr_dcc->port), 0);
        hv_store (dcc_hash_member, "nick",             4, newSVpv (ptr_dcc->nick, 0), 0);
        hv_store (dcc_hash_member, "remote_file",     11, newSVpv (ptr_dcc->filename, 0), 0);
        hv_store (dcc_hash_member, "local_file",      10, newSVpv (ptr_dcc->local_filename, 0), 0);
        hv_store (dcc_hash_member, "filename_suffix", 15, newSViv (ptr_dcc->filename_suffix), 0);
        hv_store (dcc_hash_member, "size",             4, newSVnv (ptr_dcc->size), 0);
        hv_store (dcc_hash_member, "pos",              3, newSVnv (ptr_dcc->pos), 0);
        hv_store (dcc_hash_member, "start_resume",    12, newSVnv (ptr_dcc->start_resume), 0);
        hv_store (dcc_hash_member, "cps",              3, newSViv (ptr_dcc->bytes_per_sec), 0);
        
	XPUSHs(newRV_inc((SV *) dcc_hash_member));
	count++;
    }
    perl_plugin->free_dcc_info (perl_plugin, dcc_info);    
    
    XSRETURN (count);
}

/*
 * weechat::get_config: get value of a WeeChat config option
 */

static XS (XS_weechat_get_config)
{
    char *option, *return_value;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to get config option, "
                                   "script not initialized");
	XSRETURN_EMPTY;
    }
    
    if (items < 1)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"get_config\" function");
        XSRETURN_EMPTY;
    }
    
    option = SvPV (ST (0), PL_na);
    
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
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to set config option, "
                                   "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 2)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"set_config\" function");
        XSRETURN_NO;
    }
    
    option = SvPV (ST (0), PL_na);
    value = SvPV (ST (1), PL_na);
    
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
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to get plugin config option, "
                                   "script not initialized");
	XSRETURN_EMPTY;
    }
    
    if (items < 1)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"get_plugin_config\" function");
        XSRETURN_EMPTY;
    }
    
    option = SvPV (ST (0), PL_na);
    
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
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to set plugin config option, "
                                   "script not initialized");
	XSRETURN_NO;
    }
    
    if (items < 2)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"set_plugin_config\" function");
        XSRETURN_NO;
    }
    
    option = SvPV (ST (0), PL_na);
    value = SvPV (ST (1), PL_na);
    
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
 * weechat::get_server_info: get infos about servers
 */

static XS (XS_weechat_get_server_info)
{
    t_plugin_server_info *server_info, *ptr_server;
    char timebuffer[64];
    HV *server_hash, *server_hash_member;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    (void) items;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to get server info, "
                                   "script not initialized");
	XSRETURN_EMPTY;
    }
    
    server_info = perl_plugin->get_server_info (perl_plugin);
    if (!server_info)
    {
	XSRETURN_EMPTY;
    }    
    
    server_hash = (HV *) sv_2mortal((SV *) newHV());
    if (!server_hash)
    {
        perl_plugin->free_server_info (perl_plugin, server_info);
	XSRETURN_EMPTY;
    }
    
    for (ptr_server = server_info; ptr_server; ptr_server = ptr_server->next_server)
    {
	strftime(timebuffer, sizeof(timebuffer), "%F %T",
		 localtime(&ptr_server->away_time));
	
	server_hash_member = (HV *) sv_2mortal((SV *) newHV());	        
        
	hv_store (server_hash_member, "autoconnect",          11, newSViv (ptr_server->autoconnect), 0);
        hv_store (server_hash_member, "autoreconnect",        13, newSViv (ptr_server->autoreconnect), 0);
        hv_store (server_hash_member, "autoreconnect_delay",  19, newSViv (ptr_server->autoreconnect_delay), 0);
        hv_store (server_hash_member, "command_line",         12, newSViv (ptr_server->command_line), 0);
	hv_store (server_hash_member, "address",               7, newSVpv (ptr_server->address, 0), 0);
        hv_store (server_hash_member, "port",                  4, newSViv (ptr_server->port), 0);
	hv_store (server_hash_member, "ipv6",                  4, newSViv (ptr_server->ipv6), 0);
        hv_store (server_hash_member, "ssl",                   3, newSViv (ptr_server->ssl), 0);
        hv_store (server_hash_member, "password",              8, newSVpv (ptr_server->password, 0), 0);
        hv_store (server_hash_member, "nick1",                 5, newSVpv (ptr_server->nick1, 0), 0);
        hv_store (server_hash_member, "nick2",                 5, newSVpv (ptr_server->nick2, 0), 0);
        hv_store (server_hash_member, "nick3",                 5, newSVpv (ptr_server->nick3, 0), 0);
        hv_store (server_hash_member, "username",              8, newSVpv (ptr_server->username, 0), 0);
        hv_store (server_hash_member, "realname",              8, newSVpv (ptr_server->realname, 0), 0);
        hv_store (server_hash_member, "command",               7, newSVpv (ptr_server->command, 0), 0);
        hv_store (server_hash_member, "command_delay",        13, newSViv (ptr_server->command_delay), 0);
        hv_store (server_hash_member, "autojoin",              8, newSVpv (ptr_server->autojoin, 0), 0);
        hv_store (server_hash_member, "autorejoin",           10, newSViv (ptr_server->autorejoin), 0);
        hv_store (server_hash_member, "notify_levels",        13, newSVpv (ptr_server->notify_levels, 0), 0);
        hv_store (server_hash_member, "charset_decode_iso",   18, newSVpv (ptr_server->charset_decode_iso, 0), 0);
        hv_store (server_hash_member, "charset_decode_utf",   18, newSVpv (ptr_server->charset_decode_utf, 0), 0);
        hv_store (server_hash_member, "charset_encode",       14, newSVpv (ptr_server->charset_encode, 0), 0);
        hv_store (server_hash_member, "is_connected",         12, newSViv (ptr_server->is_connected), 0);
        hv_store (server_hash_member, "ssl_connected",        13, newSViv (ptr_server->ssl_connected), 0);
        hv_store (server_hash_member, "nick",                  4, newSVpv (ptr_server->nick, 0), 0);
        hv_store (server_hash_member, "nick_modes",           10, newSVpv (ptr_server->nick_modes, 0), 0);
	hv_store (server_hash_member, "away_time",             9, newSVpv (timebuffer, 0), 0);
	hv_store (server_hash_member, "lag",                   3, newSViv (ptr_server->lag), 0);
	
	hv_store (server_hash, ptr_server->name, strlen(ptr_server->name), newRV_inc((SV *) server_hash_member), 0);	
    }    
    perl_plugin->free_server_info (perl_plugin, server_info);
    
    ST (0) = newRV_inc((SV *) server_hash);
    XSRETURN (1);
}

/*
 * weechat::get_channel_info: get infos about channels
 */

static XS (XS_weechat_get_channel_info)
{
    t_plugin_channel_info *channel_info, *ptr_channel;
    char *server;
    HV *channel_hash, *channel_hash_member;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to get channel info, "
                                   "script not initialized");
	XSRETURN_EMPTY;
    }

    if (items != 1)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"get_channel_info\" function");
        XSRETURN_EMPTY;
    }
    
    server = SvPV (ST (0), PL_na);
    if (!server)
	XSRETURN_EMPTY;

    channel_info = perl_plugin->get_channel_info (perl_plugin, server);
    if (!channel_info)
    {
	XSRETURN_EMPTY;
    }
    
    channel_hash = (HV *) sv_2mortal((SV *) newHV());
    if (!channel_hash)
    {
        perl_plugin->free_channel_info (perl_plugin, channel_info);
	XSRETURN_EMPTY;
    }
    
    for (ptr_channel = channel_info; ptr_channel; ptr_channel = ptr_channel->next_channel)
    {
	channel_hash_member = (HV *) sv_2mortal((SV *) newHV());	        
       
	hv_store (channel_hash_member, "type",          4, newSViv (ptr_channel->type), 0);
	hv_store (channel_hash_member, "topic",         5, newSVpv (ptr_channel->topic, 0), 0);
	hv_store (channel_hash_member, "modes",         5, newSVpv (ptr_channel->modes, 0), 0);
	hv_store (channel_hash_member, "limit",         5, newSViv (ptr_channel->limit), 0);
	hv_store (channel_hash_member, "key",           3, newSVpv (ptr_channel->key, 0), 0);
	hv_store (channel_hash_member, "nicks_count",  11, newSViv (ptr_channel->nicks_count), 0);
	
	hv_store (channel_hash, ptr_channel->name, strlen(ptr_channel->name), newRV_inc((SV *) channel_hash_member), 0);
    }
    perl_plugin->free_channel_info (perl_plugin, channel_info);
    
    ST (0) = newRV_inc((SV *) channel_hash);
    XSRETURN (1);
}

/*
 * weechat::get_nick_info: get infos about nicks
 */

static XS (XS_weechat_get_nick_info)
{
    t_plugin_nick_info *nick_info, *ptr_nick;
    char *server, *channel;
    HV *nick_hash;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to get nick info, "
                                   "script not initialized");
	XSRETURN_EMPTY;
    }

    if (items != 2)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"get_nick_info\" function");
        XSRETURN_EMPTY;
    }
    
    server = SvPV (ST (0), PL_na);
    channel = SvPV (ST (1), PL_na);
    if (!server || !channel)
	XSRETURN_EMPTY;
    
    nick_info = perl_plugin->get_nick_info (perl_plugin, server, channel);
    if (!nick_info)
    {
	XSRETURN_EMPTY;
    }
    
    nick_hash = (HV *) sv_2mortal((SV *) newHV());
    if (!nick_hash)
    {
        perl_plugin->free_nick_info (perl_plugin, nick_info);
	XSRETURN_EMPTY;
    }
    
    for (ptr_nick = nick_info; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
	HV *nick_hash_member = (HV *) sv_2mortal((SV *) newHV());	        
       
	hv_store (nick_hash_member, "flags",  5, newSViv (ptr_nick->flags), 0);
	hv_store (nick_hash_member, "host",   4, newSVpv (
		      ptr_nick->host ? ptr_nick->host : "", 0), 0);
	
	hv_store (nick_hash, ptr_nick->nick, strlen(ptr_nick->nick), newRV_inc((SV *) nick_hash_member), 0);
    }
    perl_plugin->free_nick_info (perl_plugin, nick_info);
    
    ST (0) = newRV_inc((SV *) nick_hash);
    XSRETURN (1);
}

/*
 * weechat::color_input: add color in input buffer
 */

static XS (XS_weechat_input_color)
{
    int color, start, length;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to colorize input, "
                                   "script not initialized");
	XSRETURN_NO;
    }

    if (items < 3)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"color_input\" function");
        XSRETURN_NO;
    }
    
    color = SvIV (ST (0));
    start = SvIV (ST (1));
    length = SvIV (ST (2));
    
    perl_plugin->input_color (perl_plugin, color, start, length);

    XSRETURN_YES;
}

/*
 * weechat::get_irc_color:
 *          get the numeric value which identify an irc color by its name
 */

static XS (XS_weechat_get_irc_color)
{
    char *color;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to get irc color, "
                                   "script not initialized");
	XST_mIV (0, -1);
	XSRETURN (1);
    }
    
    if (items != 1)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"get_irc_info\" function");
	XST_mIV (0, -1);
	XSRETURN (1);
    }
    
    color = SvPV (ST (0), PL_na);
    if (color)
    {
	XST_mIV (0, perl_plugin->get_irc_color (perl_plugin, color));
	XSRETURN (1);
    }

    XST_mIV (0, -1);
    XSRETURN (-1);
}

/*
 * weechat::get_window_info: get infos about windows
 */

static XS (XS_weechat_get_window_info)
{
    t_plugin_window_info *window_info, *ptr_window;
    int count;
    HV *window_hash_member;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    (void) items;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to get window info, "
                                   "script not initialized");
	XSRETURN_EMPTY;
    }
    
    window_info = perl_plugin->get_window_info (perl_plugin);
    count = 0;
    if (!window_info)
	XSRETURN_EMPTY;
    
    for (ptr_window = window_info; ptr_window; ptr_window = ptr_window->next_window)
    {
	window_hash_member = (HV *) sv_2mortal((SV *) newHV());	        
        
	hv_store (window_hash_member, "num_buffer",     10, newSViv (ptr_window->num_buffer), 0);
        hv_store (window_hash_member, "win_x",           5, newSViv (ptr_window->win_x), 0);
        hv_store (window_hash_member, "win_y",           5, newSViv (ptr_window->win_y), 0);
        hv_store (window_hash_member, "win_width",       9, newSViv (ptr_window->win_width), 0);
	hv_store (window_hash_member, "win_height",     10, newSViv (ptr_window->win_height), 0);
	hv_store (window_hash_member, "win_width_pct",  13, newSViv (ptr_window->win_width_pct), 0);
	hv_store (window_hash_member, "win_height_pct", 14, newSViv (ptr_window->win_height_pct), 0);
	
	XPUSHs(newRV_inc((SV *) window_hash_member));
	count++;
    }    
    perl_plugin->free_window_info (perl_plugin, window_info);
    
    XSRETURN (count);
}

/*
 * weechat::get_buffer_info: get infos about buffers
 */

static XS (XS_weechat_get_buffer_info)
{
    t_plugin_buffer_info *buffer_info, *ptr_buffer;
    HV *buffer_hash, *buffer_hash_member;
    char conv[8];
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    (void) items;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to get buffer info, "
                                   "script not initialized");
	XSRETURN_EMPTY;
    }
    
    buffer_info = perl_plugin->get_buffer_info (perl_plugin);
    if (!buffer_info)
    {
	XSRETURN_EMPTY;
    }    
    
    buffer_hash = (HV *) sv_2mortal((SV *) newHV());
    if (!buffer_hash)
    {
        perl_plugin->free_buffer_info (perl_plugin, buffer_info);
	XSRETURN_EMPTY;
    }
    
    for (ptr_buffer = buffer_info; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
	buffer_hash_member = (HV *) sv_2mortal((SV *) newHV());	        
        
	hv_store (buffer_hash_member, "type",           4, newSViv (ptr_buffer->type), 0);
        hv_store (buffer_hash_member, "num_displayed", 13, newSViv (ptr_buffer->num_displayed), 0);
	hv_store (buffer_hash_member, "server",         6, 
		  newSVpv (ptr_buffer->server_name == NULL ? "" : ptr_buffer->server_name, 0), 0);
        hv_store (buffer_hash_member, "channel",        7, 
		  newSVpv (ptr_buffer->channel_name == NULL ? "" : ptr_buffer->channel_name, 0), 0);
        hv_store (buffer_hash_member, "notify_level",  12, newSViv (ptr_buffer->notify_level), 0);
	hv_store (buffer_hash_member, "log_filename",  12, 
		  newSVpv (ptr_buffer->log_filename == NULL ? "" : ptr_buffer->log_filename, 0), 0);
	
	snprintf(conv, sizeof(conv), "%d", ptr_buffer->number);
	hv_store (buffer_hash, conv, strlen(conv), newRV_inc((SV *) buffer_hash_member), 0);	
    }    
    perl_plugin->free_buffer_info (perl_plugin, buffer_info);
    
    ST (0) = newRV_inc((SV *) buffer_hash);
    XSRETURN (1);
}

/*
 * weechat::get_buffer_data: get buffer content
 */

static XS (XS_weechat_get_buffer_data)
{
    t_plugin_buffer_line *buffer_data, *ptr_data;
    HV *data_list_member;
    char *server, *channel;
    char timebuffer[64];
    int count;
    
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    (void) items;
    
    if (!perl_current_script)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: unable to get buffer data, "
                                   "script not initialized");
	XSRETURN_EMPTY;
    }
    
    if (items > 2)
    {
        perl_plugin->print_server (perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"get_buffer_data\" function");
        XSRETURN_EMPTY;
    }
    
    channel = NULL;
    server = NULL;

    if (items >= 1)
	server = SvPV (ST (0), PL_na);
    if (items >= 2)
	channel = SvPV (ST (1), PL_na);

    SP -= items;
    
    buffer_data = perl_plugin->get_buffer_data (perl_plugin, server, channel);
    count = 0;
    if (!buffer_data)
	XSRETURN_EMPTY;
    
    for (ptr_data = buffer_data; ptr_data; ptr_data = ptr_data->next_line)
    {
	data_list_member = (HV *) sv_2mortal((SV *) newHV());

	strftime(timebuffer, sizeof(timebuffer), "%F %T",
		 localtime(&ptr_data->date));
        
	hv_store (data_list_member, "date", 4, newSVpv (timebuffer, 0), 0);
	hv_store (data_list_member, "nick", 4, 
		  newSVpv (ptr_data->nick == NULL ? "" : ptr_data->nick, 0), 0);
	hv_store (data_list_member, "data", 4, 
		  newSVpv (ptr_data->data == NULL ? "" : ptr_data->data, 0), 0);
	
	XPUSHs(newRV_inc((SV *) data_list_member));
	count++;
    }    
    perl_plugin->free_buffer_data (perl_plugin, buffer_data);
    
    XSRETURN (count);
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
    newXS ("weechat::print_server", XS_weechat_print_server, "weechat");
    newXS ("weechat::print_infobar", XS_weechat_print_infobar, "weechat");
    newXS ("weechat::remove_infobar", XS_weechat_remove_infobar, "weechat");
    newXS ("weechat::log", XS_weechat_log, "weechat");
    newXS ("weechat::command", XS_weechat_command, "weechat");
    newXS ("weechat::add_message_handler", XS_weechat_add_message_handler, "weechat");
    newXS ("weechat::add_command_handler", XS_weechat_add_command_handler, "weechat");
    newXS ("weechat::add_timer_handler", XS_weechat_add_timer_handler, "weechat");
    newXS ("weechat::add_keyboard_handler", XS_weechat_add_keyboard_handler, "weechat");
    newXS ("weechat::remove_handler", XS_weechat_remove_handler, "weechat");
    newXS ("weechat::remove_timer_handler", XS_weechat_remove_timer_handler, "weechat");
    newXS ("weechat::remove_keyboard_handler", XS_weechat_remove_keyboard_handler, "weechat");
    newXS ("weechat::get_info", XS_weechat_get_info, "weechat");
    newXS ("weechat::get_dcc_info", XS_weechat_get_dcc_info, "weechat");
    newXS ("weechat::get_config", XS_weechat_get_config, "weechat");
    newXS ("weechat::set_config", XS_weechat_set_config, "weechat");
    newXS ("weechat::get_plugin_config", XS_weechat_get_plugin_config, "weechat");
    newXS ("weechat::set_plugin_config", XS_weechat_set_plugin_config, "weechat");
    newXS ("weechat::get_server_info", XS_weechat_get_server_info, "weechat");
    newXS ("weechat::get_channel_info", XS_weechat_get_channel_info, "weechat");
    newXS ("weechat::get_nick_info", XS_weechat_get_nick_info, "weechat");
    newXS ("weechat::input_color", XS_weechat_input_color, "weechat");
    newXS ("weechat::get_irc_color", XS_weechat_get_irc_color, "weechat");
    newXS ("weechat::get_window_info", XS_weechat_get_window_info, "weechat");
    newXS ("weechat::get_buffer_info", XS_weechat_get_buffer_info, "weechat");
    newXS ("weechat::get_buffer_data", XS_weechat_get_buffer_data, "weechat");

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
    
    plugin->print_server (plugin, "Loading Perl script \"%s\"", filename);
    perl_current_script = NULL;

#ifndef MULTIPLICITY
    snprintf(pkgname, sizeof(pkgname), "%s%d", PKG_NAME_PREFIX, perl_num);
    perl_num++;
    tempscript.interpreter = "WeechatPerlScriptLoader";
    eval = weechat_perl_exec (plugin, &tempscript, "weechat_perl_load_eval_file", filename, pkgname, "");
#else
    perl_current_interpreter = perl_alloc();

    if (perl_current_interpreter == NULL)
    {
        plugin->print_server (plugin,
                              "Perl error: unable to create new sub-interpreter");
        return 0;
    }

    perl_current_script_filename = strdup (filename);

    PERL_SET_CONTEXT (perl_current_interpreter);
    perl_construct (perl_current_interpreter);
    tempscript.interpreter = (PerlInterpreter *) perl_current_interpreter;
    perl_parse (perl_current_interpreter, weechat_perl_xs_init, 3, perl_args, NULL);
    
    eval_pv (perl_weechat_code, TRUE);
    eval = weechat_perl_exec (plugin, &tempscript, "weechat_perl_load_eval_file", filename, "", "");

    free (perl_current_script_filename);

#endif
    
    if ( eval != 0) 
    {
	if (eval == 2) 
	{
	    plugin->print_server (plugin,
                                  "Perl error: unable to parse file \"%s\"",
                                  filename);
	    plugin->print_server (plugin,
                                  "Perl error: %s",
#ifndef MULTIPLICITY
                                  SvPV(perl_get_sv("WeechatPerlScriptLoader::weechat_perl_load_eval_file_error", FALSE), len));
#else
                                  SvPV(perl_get_sv("weechat_perl_load_eval_file_error", FALSE), len));
#endif
	}
	else if ( eval == 1)
	{
	    plugin->print_server (plugin,
                                  "Perl error: unable to run file \"%s\"",
                                  filename);
	}
	else {
	    plugin->print_server (plugin,
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
    
    if (perl_current_script == NULL)
    {
	plugin->print_server (plugin,
                              "Perl error: function \"register\" not found "
                              "(or failed) in file \"%s\"",
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
    plugin->print_server (plugin,
                          "Unloading Perl script \"%s\"",
                          script->name);

#ifndef MULTIPLICITY
    eval_pv(script->interpreter, TRUE);
#else
    PERL_SET_CONTEXT (script->interpreter);
#endif        

    if (script->shutdown_func[0])
        weechat_perl_exec (plugin, script, script->shutdown_func, NULL, NULL, NULL);

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
        plugin->print_server (plugin,
                              "Perl script \"%s\" unloaded",
                              name);
    }
    else
    {
        plugin->print_server (plugin,
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
    plugin->print_server (plugin,
                          "Unloading all Perl scripts");
    while (perl_scripts)
      weechat_perl_unload (plugin, perl_scripts);

    plugin->print_server (plugin,
                          "Perl scripts unloaded");
}

/*
 * weechat_perl_cmd: /perl command handler
 */

int
weechat_perl_cmd (t_weechat_plugin *plugin,
                  int cmd_argc, char **cmd_argv,
                  char *handler_args, void *handler_pointer)
{
    int argc, handler_found;
    char **argv, *path_script;
    t_plugin_script *ptr_script;
    t_plugin_handler *ptr_handler;
    
    if (cmd_argc < 3)
        return PLUGIN_RC_KO;
    
    /* make gcc happy */
    (void) handler_args;
    (void) handler_pointer;
    
    if (cmd_argv[2])
        argv = plugin->explode_string (plugin, cmd_argv[2], " ", 0, &argc);
    else
    {
        argv = NULL;
        argc = 0;
    }
    
    switch (argc)
    {
        case 0:
            /* list registered Perl scripts */
            plugin->print_server (plugin, "");
            plugin->print_server (plugin, "Registered Perl scripts:");
            if (perl_scripts)
            {
                for (ptr_script = perl_scripts;
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
            
            /* list Perl message handlers */
            plugin->print_server (plugin, "");
            plugin->print_server (plugin, "Perl message handlers:");
            handler_found = 0;
            for (ptr_handler = plugin->handlers;
                 ptr_handler; ptr_handler = ptr_handler->next_handler)
            {
                if ((ptr_handler->type == HANDLER_MESSAGE)
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
            
            /* list Perl command handlers */
            plugin->print_server (plugin, "");
            plugin->print_server (plugin, "Perl command handlers:");
            handler_found = 0;
            for (ptr_handler = plugin->handlers;
                 ptr_handler; ptr_handler = ptr_handler->next_handler)
            {
                if ((ptr_handler->type == HANDLER_COMMAND)
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
            
            /* list Perl timer handlers */
            plugin->print_server (plugin, "");
            plugin->print_server (plugin, "Perl timer handlers:");
            handler_found = 0;
            for (ptr_handler = plugin->handlers;
                 ptr_handler; ptr_handler = ptr_handler->next_handler)
            {
                if ((ptr_handler->type == HANDLER_TIMER)
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
            
            /* list Perl keyboard handlers */
            plugin->print_server (plugin, "");
            plugin->print_server (plugin, "Perl keyboard handlers:");
            handler_found = 0;
            for (ptr_handler = plugin->handlers;
                 ptr_handler; ptr_handler = ptr_handler->next_handler)
            {
                if ((ptr_handler->type == HANDLER_KEYBOARD)
                    && (ptr_handler->handler_args))
                {
                    handler_found = 1;
                    plugin->print_server (plugin, "  Perl(%s)",
                                          ptr_handler->handler_args);
                }
            }
            if (!handler_found)
                plugin->print_server (plugin, "  (none)");
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
                path_script = weechat_script_search_full_name (plugin, "perl", argv[1]);
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
                plugin->print_server (plugin,
                                      "Perl error: unknown option for "
                                      "\"perl\" command");
            }
            break;
        default:
            plugin->print_server (plugin,
                                  "Perl error: wrong argument count for \"perl\" command");
    }
    
    if (argv)
        plugin->free_exploded_string (plugin, argv);
    
    return PLUGIN_RC_OK;
}

/*
 * weechat_plugin_init: initialize Perl plugin
 */

int
weechat_plugin_init (t_weechat_plugin *plugin)
{
    perl_plugin = plugin;

#ifdef MULTIPLICITY
    plugin->print_server (plugin, "Loading Perl module \"weechat\"");
#else
    char *perl_args[] = { "", "-e", "0" };
   
    plugin->print_server (plugin, "Loading Perl module \"weechat\" (without multiplicity)");
    
    perl_main = perl_alloc ();
    
    if (!perl_main)
    {
        plugin->print_server (plugin,
                              "Perl error: unable to initialize Perl");
        return PLUGIN_RC_KO;
    }
    
    perl_construct (perl_main);
    perl_parse (perl_main, weechat_perl_xs_init, 3, perl_args, NULL);
    eval_pv (perl_weechat_code, TRUE);
#endif
    
    plugin->cmd_handler_add (plugin, "perl",
                             "list/load/unload Perl scripts",
                             "[load filename] | [autoload] | [reload] | [unload [script]]",
                             "filename: Perl script (file) to load\n"
                             "script: script name to unload\n\n"
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
    if (perl_main)
    {
	perl_destruct (perl_main);
	perl_free (perl_main);
	perl_main = NULL;
    }
#endif
    
    perl_plugin->print_server (perl_plugin,
                               "Perl plugin ended");
}
