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

/* weechat-perl-api.c: Perl API functions */

#undef _

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

#include "../../weechat-plugin.h"
#include "../script.h"
#include "../script-api.h"
#include "../script-callback.h"
#include "weechat-perl.h"


#define PERL_RETURN_OK XSRETURN_YES
#define PERL_RETURN_ERROR XSRETURN_NO
#define PERL_RETURN_EMPTY XSRETURN_EMPTY
#define PERL_RETURN_STRING(__string)              \
    if (__string)                                 \
    {                                             \
        XST_mPV (0, __string);                    \
        XSRETURN (1);                             \
    }                                             \
    XST_mPV (0, "");                              \
    XSRETURN (1)
#define PERL_RETURN_STRING_FREE(__string)         \
    if (__string)                                 \
    {                                             \
        XST_mPV (0, __string);                    \
        free (__string);                          \
        XSRETURN (1);                             \
    }                                             \
    XST_mPV (0, "");                              \
    XSRETURN (1)

extern void boot_DynaLoader (pTHX_ CV* cv);


/*
 * weechat::register: startup function for all WeeChat Perl scripts
 */

static XS (XS_weechat_register)
{
    char *name, *author, *version, *license, *description, *shutdown_func;
    char *charset;
    dXSARGS;
    
    /* make C compiler happy */
    (void) items;
    (void) cv;
    
    perl_current_script = NULL;
    
    if (items < 7)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("register");
        PERL_RETURN_ERROR;
    }
    
    name = SvPV (ST (0), PL_na);
    author = SvPV (ST (1), PL_na);
    version = SvPV (ST (2), PL_na);
    license = SvPV (ST (3), PL_na);
    description = SvPV (ST (4), PL_na);
    shutdown_func = SvPV (ST (5), PL_na);
    charset = SvPV (ST (6), PL_na);
    
    if (script_search (weechat_perl_plugin, perl_scripts, name))
    {
        /* error: another script already exists with this name! */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to register script "
                                         "\"%s\" (another script already "
                                         "exists with this name)"),
                        weechat_prefix ("error"), "perl", name);
        PERL_RETURN_ERROR;
    }
    
    /* register script */
    perl_current_script = script_add (weechat_perl_plugin,
                                      &perl_scripts,
                                      (perl_current_script_filename) ?
                                      perl_current_script_filename : "",
                                      name, author, version, license,
                                      description, shutdown_func, charset);
    if (perl_current_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: registered script \"%s\", "
                                         "version %s (%s)"),
                        weechat_prefix ("info"), "perl",
                        name, version, description);
    }
    else
    {
        PERL_RETURN_ERROR;
    }
    
    PERL_RETURN_OK;
}

/*
 * weechat::charser_set: set script charset
 */

static XS (XS_weechat_charset_set)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("charset_set");
        PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("charset_set");
        PERL_RETURN_ERROR;
    }
    
    script_api_charset_set (perl_current_script,
                            SvPV (ST (0), PL_na)); /* charset */
    
    PERL_RETURN_OK;
}

/*
 * weechat::iconv_to_internal: convert string to internal WeeChat charset
 */

static XS (XS_weechat_iconv_to_internal)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("iconv_to_internal");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("iconv_to_internal");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_iconv_to_internal (SvPV (ST (0), PL_na), /* charset */
                                        SvPV (ST (1), PL_na)); /* string */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::iconv_from_internal: convert string from WeeChat inernal charset
 *                               to another one
 */

static XS (XS_weechat_iconv_from_internal)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("iconv_from_internal");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("iconv_from_internal");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_iconv_from_internal (SvPV (ST (0), PL_na), /* charset */
                                          SvPV (ST (1), PL_na)); /* string */
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::mkdir_home: create a directory in WeeChat home
 */

static XS (XS_weechat_mkdir_home)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("mkdir_home");
        PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("mkdir_home");
        PERL_RETURN_ERROR;
    }
    
    if (weechat_mkdir_home (SvPV (ST (0), PL_na), /* directory */
                            SvIV (ST (1)))) /* mode */
        PERL_RETURN_OK;
    
    PERL_RETURN_ERROR;
}

/*
 * weechat::mkdir: create a directory
 */

static XS (XS_weechat_mkdir)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("mkdir");
        PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("mkdir");
        PERL_RETURN_ERROR;
    }
    
    if (weechat_mkdir (SvPV (ST (0), PL_na), /* directory */
                       SvIV (ST (1)))) /* mode */
        PERL_RETURN_OK;
    
    PERL_RETURN_ERROR;
}

/*
 * weechat::prefix: get a prefix, used for display
 */

static XS (XS_weechat_prefix)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("prefix");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("prefix");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_prefix (SvPV (ST (0), PL_na));
    PERL_RETURN_STRING(result);
}

/*
 * weechat::color: get a color code, used for display
 */

static XS (XS_weechat_color)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("color");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("color");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_color (SvPV (ST (0), PL_na));
    PERL_RETURN_STRING(result);
}

/*
 * weechat::print: print message in a buffer
 */

static XS (XS_weechat_print)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("print");
        PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("print");
        PERL_RETURN_ERROR;
    }
    
    script_api_printf (weechat_perl_plugin,
                       perl_current_script,
                       script_string_to_pointer (SvPV (ST (0), PL_na)), /* buffer */
                       "%s", SvPV (ST (1), PL_na)); /* message */
    
    PERL_RETURN_OK;
}

/*
 * weechat::infobar_print: print message to infobar
 */

static XS (XS_weechat_infobar_print)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_print");
        PERL_RETURN_ERROR;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infobar_print");
        PERL_RETURN_ERROR;
    }
    
    script_api_infobar_printf (weechat_perl_plugin,
                               perl_current_script,
                               SvIV (ST (0)), /* delay */
                               SvPV (ST (1), PL_na), /* color */
                               "%s",
                               SvPV (ST (1), PL_na)); /* message */
    
    PERL_RETURN_OK;
}

/*
 * weechat::infobar_remove: remove message(s) from infobar
 */

static XS (XS_weechat_infobar_remove)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_remove");
        PERL_RETURN_ERROR;
    }
    
    weechat_infobar_remove ((items >= 1) ? SvIV (ST (0)) : 0);
    
    PERL_RETURN_OK;
}

/*
 * weechat::log_print: print message in WeeChat log file
 */

static XS (XS_weechat_log_print)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;

    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("log_print");
        PERL_RETURN_ERROR;
    }

    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("log_print");
        PERL_RETURN_ERROR;
    }
    
    script_api_log_printf (weechat_perl_plugin,
                           perl_current_script,
                           "%s", SvPV (ST (0), PL_na)); /* message */
    
    PERL_RETURN_OK;
}

/*
 * weechat_perl_api_hook_command_cb: callback for command hooked
 */

int
weechat_perl_api_hook_command_cb (void *data, struct t_gui_buffer *buffer,
                                  int argc, char **argv, char **argv_eol)
{
    struct t_script_callback *script_callback;
    char *perl_argv[3], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    /* make C compiler happy */
    (void) argv;
    
    script_callback = (struct t_script_callback *)data;
    
    perl_argv[0] = script_pointer_to_string (buffer);
    perl_argv[1] = (argc > 1) ? argv_eol[1] : empty_arg;
    perl_argv[2] = NULL;
    
    rc = (int *) weechat_perl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    perl_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (perl_argv[0])
        free (perl_argv[0]);
    
    return ret;
}

/*
 * weechat::hook_command: hook a command
 */

static XS (XS_weechat_hook_command)
{
    struct t_hook *new_hook;
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_command");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 6)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_command");
        PERL_RETURN_EMPTY;
    }
    
    new_hook = script_api_hook_command (weechat_perl_plugin,
                                        perl_current_script,
                                        SvPV (ST (0), PL_na), /* command */
                                        SvPV (ST (1), PL_na), /* description */
                                        SvPV (ST (2), PL_na), /* args */
                                        SvPV (ST (3), PL_na), /* args_description */
                                        SvPV (ST (4), PL_na), /* completion */
                                        &weechat_perl_api_hook_command_cb,
                                        SvPV (ST (5), PL_na)); /* perl function */
    
    result = script_pointer_to_string (new_hook);
    PERL_RETURN_STRING_FREE(result);;
}

/*
 * weechat_perl_api_hook_timer_cb: callback for timer hooked
 */

int
weechat_perl_api_hook_timer_cb (void *data)
{
    struct t_script_callback *script_callback;
    char *perl_argv[1] = { NULL };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    rc = (int *) weechat_perl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    perl_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    
    return ret;
}

/*
 * weechat::hook_timer: hook a timer
 */

static XS (XS_weechat_hook_timer)
{
    struct t_hook *new_hook;
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_timer");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_timer");
        PERL_RETURN_EMPTY;
    }
    
    new_hook = script_api_hook_timer (weechat_perl_plugin,
                                      perl_current_script,
                                      SvIV (ST (0)), /* interval */
                                      SvIV (ST (1)), /* align_second */
                                      SvIV (ST (2)), /* max_calls */
                                      &weechat_perl_api_hook_timer_cb,
                                      SvPV (ST (3), PL_na)); /* perl function */
    
    result = script_pointer_to_string (new_hook);
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_fd_cb: callback for fd hooked
 */

int
weechat_perl_api_hook_fd_cb (void *data)
{
    struct t_script_callback *script_callback;
    char *perl_argv[1] = { NULL };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    rc = (int *) weechat_perl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    perl_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    
    return ret;
}

/*
 * weechat::hook_fd: hook a fd
 */

static XS (XS_weechat_hook_fd)
{
    struct t_hook *new_hook;
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_fd");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 5)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_fd");
        PERL_RETURN_EMPTY;
    }
    
    new_hook = script_api_hook_fd (weechat_perl_plugin,
                                   perl_current_script,
                                   SvIV (ST (0)), /* fd */
                                   SvIV (ST (1)), /* read */
                                   SvIV (ST (2)), /* write */
                                   SvIV (ST (3)), /* exception */
                                   &weechat_perl_api_hook_fd_cb,
                                   SvPV (ST (4), PL_na)); /* perl function */
    
    result = script_pointer_to_string (new_hook);
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_print_cb: callback for print hooked
 */

int
weechat_perl_api_hook_print_cb (void *data, struct t_gui_buffer *buffer,
                                time_t date, char *prefix, char *message)
{
    struct t_script_callback *script_callback;
    char *perl_argv[5];
    static char timebuffer[64];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    snprintf (timebuffer, sizeof (timebuffer) - 1, "%ld", date);
    
    perl_argv[0] = script_pointer_to_string (buffer);
    perl_argv[1] = timebuffer;
    perl_argv[2] = prefix;
    perl_argv[3] = message;
    perl_argv[4] = NULL;
    
    rc = (int *) weechat_perl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    perl_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (perl_argv[0])
        free (perl_argv[0]);
    
    return ret;
}

/*
 * weechat::hook_print: hook a print
 */

static XS (XS_weechat_hook_print)
{
    struct t_hook *new_hook;
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_print");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_print");
        PERL_RETURN_EMPTY;
    }
    
    new_hook = script_api_hook_print (weechat_perl_plugin,
                                      perl_current_script,
                                      script_string_to_pointer (SvPV (ST (0), PL_na)), /* buffer */
                                      SvPV (ST (1), PL_na), /* message */
                                      SvIV (ST (2)), /* strip_colors */
                                      &weechat_perl_api_hook_print_cb,
                                      SvPV (ST (3), PL_na)); /* perl function */
    
    result = script_pointer_to_string (new_hook);
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_signal_cb: callback for signal hooked
 */

int
weechat_perl_api_hook_signal_cb (void *data, char *signal, char *type_data,
                                 void *signal_data)
{
    struct t_script_callback *script_callback;
    char *perl_argv[3];
    static char value_str[64];
    int *rc, ret, free_needed;
    
    script_callback = (struct t_script_callback *)data;
    
    perl_argv[0] = signal;
    free_needed = 0;
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        perl_argv[1] = (char *)signal_data;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        snprintf (value_str, sizeof (value_str) - 1,
                  "%d", *((int *)signal_data));
        perl_argv[1] = value_str;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        perl_argv[1] = script_pointer_to_string (signal_data);
        free_needed = 1;
    }
    else
        perl_argv[1] = NULL;
    perl_argv[2] = NULL;
    
    rc = (int *) weechat_perl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    perl_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (free_needed && perl_argv[1])
        free (perl_argv[1]);
    
    return ret;
}

/*
 * weechat::hook_signal: hook a signal
 */

static XS (XS_weechat_hook_signal)
{
    struct t_hook *new_hook;
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_signal");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_signal");
        PERL_RETURN_ERROR;
    }
    
    new_hook = script_api_hook_signal (weechat_perl_plugin,
                                       perl_current_script,
                                       SvPV (ST (0), PL_na), /* signal */
                                       &weechat_perl_api_hook_signal_cb,
                                       SvPV (ST (1), PL_na)); /* perl function */
    
    result = script_pointer_to_string (new_hook);
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::hook_signal_send: send a signal
 */

static XS (XS_weechat_hook_signal_send)
{
    char *type_data;
    int number;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_signal_send");
	PERL_RETURN_ERROR;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_signal_send");
        PERL_RETURN_ERROR;
    }
    
    type_data = SvPV (ST (1), PL_na);
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        weechat_hook_signal_send (SvPV (ST (0), PL_na), /* signal */
                                  type_data,
                                  SvPV (ST (2), PL_na)); /* signal_data */
        PERL_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        number = SvIV(ST (2));
        weechat_hook_signal_send (SvPV (ST (0), PL_na), /* signal */
                                  type_data,
                                  &number); /* signal_data */
        PERL_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        weechat_hook_signal_send (SvPV (ST (0), PL_na), /* signal */
                                  type_data,
                                  script_string_to_pointer (SvPV (ST (2), PL_na))); /* signal_data */
        PERL_RETURN_OK;
    }
    
    PERL_RETURN_ERROR;
}

/*
 * weechat_perl_api_hook_config_cb: callback for config option hooked
 */

int
weechat_perl_api_hook_config_cb (void *data, char *type, char *option,
                                 char *value)
{
    struct t_script_callback *script_callback;
    char *perl_argv[4];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    perl_argv[0] = type;
    perl_argv[1] = option;
    perl_argv[2] = value;
    perl_argv[3] = NULL;
    
    rc = (int *) weechat_perl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    perl_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    
    return ret;
}

/*
 * weechat::hook_config: hook a config option
 */

static XS (XS_weechat_hook_config)
{
    struct t_hook *new_hook;
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_config");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_config");
        PERL_RETURN_EMPTY;
    }
    
    new_hook = script_api_hook_config (weechat_perl_plugin,
                                       perl_current_script,
                                       SvPV (ST (0), PL_na), /* type */
                                       SvPV (ST (1), PL_na), /* option */
                                       &weechat_perl_api_hook_config_cb,
                                       SvPV (ST (2), PL_na)); /* perl function */
    
    result = script_pointer_to_string (new_hook);
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_completion_cb: callback for completion hooked
 */

int
weechat_perl_api_hook_completion_cb (void *data, char *completion,
                                     struct t_gui_buffer *buffer,
                                     struct t_weelist *list)
{
    struct t_script_callback *script_callback;
    char *perl_argv[4];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    perl_argv[0] = completion;
    perl_argv[1] = script_pointer_to_string (buffer);
    perl_argv[2] = script_pointer_to_string (list);
    perl_argv[3] = NULL;
    
    rc = (int *) weechat_perl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    perl_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (perl_argv[1])
        free (perl_argv[1]);
    if (perl_argv[2])
        free (perl_argv[2]);
    
    return ret;
}

/*
 * weechat::hook_completion: hook a completion
 */

static XS (XS_weechat_hook_completion)
{
    struct t_hook *new_hook;
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_completion");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_completion");
        PERL_RETURN_EMPTY;
    }
    
    new_hook = script_api_hook_completion (weechat_perl_plugin,
                                           perl_current_script,
                                           SvPV (ST (0), PL_na), /* completion */
                                           &weechat_perl_api_hook_completion_cb,
                                           SvPV (ST (1), PL_na)); /* perl function */
    
    result = script_pointer_to_string (new_hook);
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::unhook: unhook something
 */

static XS (XS_weechat_unhook)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("unhook");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("unhook");
        PERL_RETURN_ERROR;
    }
    
    if (script_api_unhook (weechat_perl_plugin,
                           perl_current_script,
                           script_string_to_pointer (SvPV (ST (0), PL_na))))
        PERL_RETURN_OK;
    
    PERL_RETURN_ERROR;
}

/*
 * weechat::unhook_all: unhook all for script
 */

static XS (XS_weechat_unhook_all)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    (void) items;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("unhook_all");
	PERL_RETURN_ERROR;
    }
    
    script_api_unhook_all (weechat_perl_plugin,
                           perl_current_script);
    
    PERL_RETURN_OK;
}

/*
 * weechat_perl_api_input_data_cb: callback for input data in a buffer
 */

int
weechat_perl_api_input_data_cb (void *data, struct t_gui_buffer *buffer,
                                char *input_data)
{
    struct t_script_callback *script_callback;
    char *perl_argv[3];
    int *r, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    perl_argv[0] = script_pointer_to_string (buffer);
    perl_argv[1] = input_data;
    perl_argv[2] = NULL;
    
    r = (int *) weechat_perl_exec (script_callback->script,
                                   WEECHAT_SCRIPT_EXEC_INT,
                                   script_callback->function,
                                   perl_argv);
    if (!r)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *r;
        free (r);
    }
    if (perl_argv[0])
        free (perl_argv[0]);
    
    return ret;
}

/*
 * weechat::buffer_new: create a new buffer
 */

static XS (XS_weechat_buffer_new)
{
    struct t_gui_buffer *new_buffer;
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_new");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_new");
        PERL_RETURN_EMPTY;
    }
    
    new_buffer = script_api_buffer_new (weechat_perl_plugin,
                                        perl_current_script,
                                        SvPV (ST (0), PL_na), /* category */
                                        SvPV (ST (1), PL_na), /* name */
                                        &weechat_perl_api_input_data_cb,
                                        SvPV (ST (2), PL_na)); /* perl function */
    
    result = script_pointer_to_string (new_buffer);
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::buffer_search: search a buffer
 */

static XS (XS_weechat_buffer_search)
{
    struct t_gui_buffer *ptr_buffer;
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_search");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_search");
        PERL_RETURN_EMPTY;
    }
    
    ptr_buffer = weechat_buffer_search (SvPV (ST (0), PL_na), /* category */
                                        SvPV (ST (1), PL_na)); /* name */
    
    result = script_pointer_to_string (ptr_buffer);
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::buffer_close: close a buffer
 */

static XS (XS_weechat_buffer_close)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_close");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_close");
        PERL_RETURN_ERROR;
    }
    
    script_api_buffer_close (weechat_perl_plugin,
                             perl_current_script,
                             script_string_to_pointer (SvPV (ST (0), PL_na)), /* buffer */
                             SvIV (ST (1))); /* switch_to_another */
    
    PERL_RETURN_OK;
}

/*
 * weechat::buffer_get: get a buffer property
 */

static XS (XS_weechat_buffer_get)
{
    char *value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_get");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_get");
        PERL_RETURN_EMPTY;
    }
    
    value = weechat_buffer_get (script_string_to_pointer (SvPV (ST (0), PL_na)), /* buffer */
                                SvPV (ST (1), PL_na)); /* property */
    PERL_RETURN_STRING(value);
}

/*
 * weechat::buffer_set: set a buffer property
 */

static XS (XS_weechat_buffer_set)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_set");
	PERL_RETURN_ERROR;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_set");
        PERL_RETURN_ERROR;
    }
    
    weechat_buffer_set (script_string_to_pointer (SvPV (ST (0), PL_na)), /* buffer */
                        SvPV (ST (1), PL_na), /* property */
                        SvPV (ST (2), PL_na)); /* value */
    
    PERL_RETURN_OK;
}

/*
 * weechat::nicklist_add_group: add a group in nicklist
 */

static XS (XS_weechat_nicklist_add_group)
{
    struct t_gui_nick_group *new_group;
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_add_group");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 5)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_add_group");
        PERL_RETURN_EMPTY;
    }
    
    new_group = weechat_nicklist_add_group (script_string_to_pointer (SvPV (ST (0), PL_na)), /* buffer */
                                            script_string_to_pointer (SvPV (ST (1), PL_na)), /* parent_group */
                                            SvPV (ST (2), PL_na), /* name */
                                            SvPV (ST (3), PL_na), /* color */
                                            SvIV (ST (4))); /* visible */
    
    result = script_pointer_to_string (new_group);
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::nicklist_search_group: search a group in nicklist
 */

static XS (XS_weechat_nicklist_search_group)
{
    struct t_gui_nick_group *ptr_group;
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_search_group");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_search_group");
        PERL_RETURN_EMPTY;
    }
    
    ptr_group = weechat_nicklist_search_group (script_string_to_pointer (SvPV (ST (0), PL_na)), /* buffer */
                                               script_string_to_pointer (SvPV (ST (1), PL_na)), /* from_group */
                                               SvPV (ST (2), PL_na)); /* name */
    
    result = script_pointer_to_string (ptr_group);
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::nicklist_add_nick: add a nick in nicklist
 */

static XS (XS_weechat_nicklist_add_nick)
{
    struct t_gui_nick *new_nick;
    char *prefix, char_prefix, *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_add_nick");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 7)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_add_nick");
        PERL_RETURN_EMPTY;
    }
    
    prefix = SvPV(ST (4), PL_na);
    if (prefix && prefix[0])
        char_prefix = prefix[0];
    else
        char_prefix = ' ';
    
    new_nick = weechat_nicklist_add_nick (script_string_to_pointer (SvPV (ST (0), PL_na)), /* buffer */
                                          script_string_to_pointer (SvPV (ST (1), PL_na)), /* group */
                                          SvPV (ST (2), PL_na), /* name */
                                          SvPV (ST (3), PL_na), /* color */
                                          char_prefix,
                                          SvPV (ST (5), PL_na), /* prefix_color */
                                          SvIV (ST (6))); /* visible */
    
    result = script_pointer_to_string (new_nick);
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::nicklist_search_nick: search a nick in nicklist
 */

static XS (XS_weechat_nicklist_search_nick)
{
    struct t_gui_nick *ptr_nick;
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_search_nick");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_search_nick");
        PERL_RETURN_EMPTY;
    }
    
    ptr_nick = weechat_nicklist_search_nick (script_string_to_pointer (SvPV (ST (0), PL_na)), /* buffer */
                                             script_string_to_pointer (SvPV (ST (1), PL_na)), /* from_group */
                                             SvPV (ST (2), PL_na)); /* name */
    
    result = script_pointer_to_string (ptr_nick);
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::nicklist_remove_group: remove a group from nicklist
 */

static XS (XS_weechat_nicklist_remove_group)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_remove_group");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_remove_group");
        PERL_RETURN_ERROR;
    }
    
    weechat_nicklist_remove_group (script_string_to_pointer (SvPV (ST (0), PL_na)), /* buffer */
                                   script_string_to_pointer (SvPV (ST (1), PL_na))); /* group */
    
    PERL_RETURN_OK;
}

/*
 * weechat::nicklist_remove_nick: remove a nick from nicklist
 */

static XS (XS_weechat_nicklist_remove_nick)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_remove_nick");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_remove_nick");
        PERL_RETURN_ERROR;
    }
    
    weechat_nicklist_remove_nick (script_string_to_pointer (SvPV (ST (0), PL_na)), /* buffer */
                                  script_string_to_pointer (SvPV (ST (1), PL_na))); /* nick */
    
    PERL_RETURN_OK;
}

/*
 * weechat::nicklist_remove_all: remove all groups/nicks from nicklist
 */

static XS (XS_weechat_nicklist_remove_all)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_remove_all");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_remove_all");
        PERL_RETURN_ERROR;
    }
    
    weechat_nicklist_remove_all (script_string_to_pointer (SvPV (ST (0), PL_na))); /* buffer */
    
    PERL_RETURN_OK;
}

/*
 * weechat::command: execute a command on a buffer
 */

static XS (XS_weechat_command)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("command");
        PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("command");
        PERL_RETURN_ERROR;
    }
    
    script_api_command (weechat_perl_plugin,
                        perl_current_script,
                        script_string_to_pointer (SvPV (ST (0), PL_na)), /* buffer */
                        SvPV (ST (1), PL_na)); /* command */
    
    PERL_RETURN_OK;
}

/*
 * weechat::info_get: get info about WeeChat
 */

static XS (XS_weechat_info_get)
{
    char *value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("info_get");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("info_get");
        PERL_RETURN_EMPTY;
    }
    
    value = weechat_info_get (SvPV (ST (0), PL_na));
    PERL_RETURN_STRING(value);
}

/*
 * weechat::get_dcc_info: get infos about DCC
 */

/*
static XS (XS_weechat_get_dcc_info)
{
    t_plugin_dcc_info *dcc_info, *ptr_dcc;    
    int count;
    char timebuffer1[64];
    char timebuffer2[64];
    struct in_addr in;
    HV *dcc_hash_member;
    dXSARGS;
    
    // make C compiler happy
    (void) cv;
    (void) items;
    
    if (!perl_current_script)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: unable to get DCC info, "
                                   "script not initialized");
	PERL_RETURN_EMPTY;
    }

    dcc_info = weechat_perl_plugin->get_dcc_info (weechat_perl_plugin);
    count = 0;
    if (!dcc_info)
	PERL_RETURN_EMPTY;
    
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
    weechat_perl_plugin->free_dcc_info (weechat_perl_plugin, dcc_info);    
    
    XSRETURN (count);
}
*/

/*
 * weechat::config_get_weechat: get value of a WeeChat config option
 */

 /*
static XS (XS_weechat_config_get_weechat)
{
    char *option;
    struct t_config_option *value;
    dXSARGS;
    
    // make C compiler happy
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_get_weechat");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_get_weechat");
        PERL_RETURN_EMPTY;
    }
    
    option = SvPV (ST (0), PL_na);
    
    if (option)
    {
        value = weechat_config_get_weechat (option);
        
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
*/

/*
 * weechat::set_config: set value of a WeeChat config option
 */

/*
static XS (XS_weechat_set_config)
{
    char *option, *value;
    dXSARGS;
    
    // make C compiler happy
    (void) cv;
    
    if (!perl_current_script)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: unable to set config option, "
                                   "script not initialized");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"set_config\" function");
        PERL_RETURN_ERROR;
    }
    
    option = SvPV (ST (0), PL_na);
    value = SvPV (ST (1), PL_na);
    
    if (option && value)
    {
        if (weechat_perl_plugin->set_config (weechat_perl_plugin, option, value))
            PERL_RETURN_OK;
    }
    
    PERL_RETURN_ERROR;
}
*/

/*
 * weechat::get_plugin_config: get value of a plugin config option
 */

/*
static XS (XS_weechat_get_plugin_config)
{
    char *option, *return_value;
    dXSARGS;
    
    // make C compiler happy
    (void) cv;
    
    if (!perl_current_script)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: unable to get plugin config option, "
                                   "script not initialized");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"get_plugin_config\" function");
        PERL_RETURN_EMPTY;
    }
    
    option = SvPV (ST (0), PL_na);
    
    if (option)
    {
        return_value = weechat_script_get_plugin_config (weechat_perl_plugin,
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
*/

/*
 * weechat::set_plugin_config: set value of a WeeChat config option
 */

/*
static XS (XS_weechat_set_plugin_config)
{
    char *option, *value;
    dXSARGS;
    
    // make C compiler happy
    (void) cv;
    
    if (!perl_current_script)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: unable to set plugin config option, "
                                   "script not initialized");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"set_plugin_config\" function");
        PERL_RETURN_ERROR;
    }
    
    option = SvPV (ST (0), PL_na);
    value = SvPV (ST (1), PL_na);
    
    if (option && value)
    {
        if (weechat_script_set_plugin_config (weechat_perl_plugin,
                                              perl_current_script,
                                              option, value))
            PERL_RETURN_OK;
    }
    
    PERL_RETURN_ERROR;
}
*/

/*
 * weechat::get_server_info: get infos about servers
 */

/*
static XS (XS_weechat_get_server_info)
{
    t_plugin_server_info *server_info, *ptr_server;
    char timebuffer[64];
    HV *server_hash, *server_hash_member;
    dXSARGS;
    
    // make C compiler happy
    (void) cv;
    (void) items;
    
    if (!perl_current_script)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: unable to get server info, "
                                   "script not initialized");
	PERL_RETURN_EMPTY;
    }
    
    server_info = weechat_perl_plugin->get_server_info (weechat_perl_plugin);
    if (!server_info)
    {
	PERL_RETURN_EMPTY;
    }    
    
    server_hash = (HV *) sv_2mortal((SV *) newHV());
    if (!server_hash)
    {
        weechat_perl_plugin->free_server_info (weechat_perl_plugin, server_info);
	PERL_RETURN_EMPTY;
    }
    
    for (ptr_server = server_info; ptr_server; ptr_server = ptr_server->next_server)
    {
	strftime(timebuffer, sizeof(timebuffer), "%F %T",
		 localtime(&ptr_server->away_time));
	
	server_hash_member = (HV *) sv_2mortal((SV *) newHV());	        

	hv_store (server_hash_member, "autoconnect",          11, newSViv (ptr_server->autoconnect), 0);
        hv_store (server_hash_member, "autoreconnect",        13, newSViv (ptr_server->autoreconnect), 0);
        hv_store (server_hash_member, "autoreconnect_delay",  19, newSViv (ptr_server->autoreconnect_delay), 0);
        hv_store (server_hash_member, "temp_server",          11, newSViv (ptr_server->temp_server), 0);
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
        hv_store (server_hash_member, "is_connected",         12, newSViv (ptr_server->is_connected), 0);
        hv_store (server_hash_member, "ssl_connected",        13, newSViv (ptr_server->ssl_connected), 0);
        hv_store (server_hash_member, "nick",                  4, newSVpv (ptr_server->nick, 0), 0);
        hv_store (server_hash_member, "nick_modes",           10, newSVpv (ptr_server->nick_modes, 0), 0);
	hv_store (server_hash_member, "away_time",             9, newSVpv (timebuffer, 0), 0);
	hv_store (server_hash_member, "lag",                   3, newSViv (ptr_server->lag), 0);

	hv_store (server_hash, ptr_server->name, strlen(ptr_server->name), newRV_inc((SV *) server_hash_member), 0);	
    }
    weechat_perl_plugin->free_server_info (weechat_perl_plugin, server_info);
    
    ST (0) = newRV_inc((SV *) server_hash);
    if (SvREFCNT(ST(0))) sv_2mortal(ST(0));
    
    XSRETURN (1);
}
*/

/*
 * weechat::get_channel_info: get infos about channels
 */

/*
static XS (XS_weechat_get_channel_info)
{
    t_plugin_channel_info *channel_info, *ptr_channel;
    char *server;
    HV *channel_hash, *channel_hash_member;
    dXSARGS;
    
    // make C compiler happy
    (void) cv;
    
    if (!perl_current_script)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: unable to get channel info, "
                                   "script not initialized");
	PERL_RETURN_EMPTY;
    }

    if (items != 1)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"get_channel_info\" function");
        PERL_RETURN_EMPTY;
    }
    
    server = SvPV (ST (0), PL_na);
    if (!server)
	PERL_RETURN_EMPTY;

    channel_info = weechat_perl_plugin->get_channel_info (weechat_perl_plugin, server);
    if (!channel_info)
    {
	PERL_RETURN_EMPTY;
    }
    
    channel_hash = (HV *) sv_2mortal((SV *) newHV());
    if (!channel_hash)
    {
        weechat_perl_plugin->free_channel_info (weechat_perl_plugin, channel_info);
	PERL_RETURN_EMPTY;
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
    weechat_perl_plugin->free_channel_info (weechat_perl_plugin, channel_info);
    
    ST (0) = newRV_inc((SV *) channel_hash);
    if (SvREFCNT(ST(0))) sv_2mortal(ST(0));

    XSRETURN (1);
}
*/

/*
 * weechat::get_nick_info: get infos about nicks
 */

/*
static XS (XS_weechat_get_nick_info)
{
    t_plugin_nick_info *nick_info, *ptr_nick;
    char *server, *channel;
    HV *nick_hash;
    dXSARGS;
    
    // make C compiler happy
    (void) cv;
    
    if (!perl_current_script)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: unable to get nick info, "
                                   "script not initialized");
	PERL_RETURN_EMPTY;
    }

    if (items != 2)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"get_nick_info\" function");
        PERL_RETURN_EMPTY;
    }
    
    server = SvPV (ST (0), PL_na);
    channel = SvPV (ST (1), PL_na);
    if (!server || !channel)
	PERL_RETURN_EMPTY;
    
    nick_info = weechat_perl_plugin->get_nick_info (weechat_perl_plugin, server, channel);
    if (!nick_info)
    {
	PERL_RETURN_EMPTY;
    }
    
    nick_hash = (HV *) sv_2mortal((SV *) newHV());
    if (!nick_hash)
    {
        weechat_perl_plugin->free_nick_info (weechat_perl_plugin, nick_info);
	PERL_RETURN_EMPTY;
    }
    
    for (ptr_nick = nick_info; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
	HV *nick_hash_member = (HV *) sv_2mortal((SV *) newHV());	        
       
	hv_store (nick_hash_member, "flags",  5, newSViv (ptr_nick->flags), 0);
	hv_store (nick_hash_member, "host",   4, newSVpv (
		      ptr_nick->host ? ptr_nick->host : "", 0), 0);
	
	hv_store (nick_hash, ptr_nick->nick, strlen(ptr_nick->nick), newRV_inc((SV *) nick_hash_member), 0);
    }
    weechat_perl_plugin->free_nick_info (weechat_perl_plugin, nick_info);
    
    ST (0) = newRV_inc((SV *) nick_hash);
    if (SvREFCNT(ST(0))) sv_2mortal(ST(0));
    
    XSRETURN (1);
}
*/

/*
 * weechat::color_input: add color in input buffer
 */

/*
static XS (XS_weechat_input_color)
{
    int color, start, length;
    dXSARGS;
    
    // make C compiler happy
    (void) cv;
    
    if (!perl_current_script)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: unable to colorize input, "
                                   "script not initialized");
	PERL_RETURN_ERROR;
    }

    if (items < 3)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"color_input\" function");
        PERL_RETURN_ERROR;
    }
    
    color = SvIV (ST (0));
    start = SvIV (ST (1));
    length = SvIV (ST (2));
    
    weechat_perl_plugin->input_color (weechat_perl_plugin, color, start, length);

    PERL_RETURN_OK;
}
*/

/*
 * weechat::get_irc_color:
 *          get the numeric value which identify an irc color by its name
 */

/*
static XS (XS_weechat_get_irc_color)
{
    char *color;
    dXSARGS;
    
    // make C compiler happy
    (void) cv;
    
    if (!perl_current_script)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: unable to get irc color, "
                                   "script not initialized");
	XST_mIV (0, -1);
	XSRETURN (1);
    }
    
    if (items != 1)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"get_irc_info\" function");
	XST_mIV (0, -1);
	XSRETURN (1);
    }
    
    color = SvPV (ST (0), PL_na);
    if (color)
    {
	XST_mIV (0, weechat_perl_plugin->get_irc_color (weechat_perl_plugin, color));
	XSRETURN (1);
    }

    XST_mIV (0, -1);
    XSRETURN (-1);
}
*/

/*
 * weechat::get_window_info: get infos about windows
 */

/*
static XS (XS_weechat_get_window_info)
{
    t_plugin_window_info *window_info, *ptr_win;
    int count;
    HV *window_hash_member;
    dXSARGS;
    
    // make C compiler happy
    (void) cv;
    (void) items;
    
    if (!perl_current_script)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: unable to get window info, "
                                   "script not initialized");
	PERL_RETURN_EMPTY;
    }
    
    window_info = weechat_perl_plugin->get_window_info (weechat_perl_plugin);
    count = 0;
    if (!window_info)
	PERL_RETURN_EMPTY;
    
    for (ptr_win = window_info; ptr_win; ptr_win = ptr_win->next_window)
    {
	window_hash_member = (HV *) sv_2mortal((SV *) newHV());	        
        
	hv_store (window_hash_member, "num_buffer",     10, newSViv (ptr_win->num_buffer), 0);
        hv_store (window_hash_member, "win_x",           5, newSViv (ptr_win->win_x), 0);
        hv_store (window_hash_member, "win_y",           5, newSViv (ptr_win->win_y), 0);
        hv_store (window_hash_member, "win_width",       9, newSViv (ptr_win->win_width), 0);
	hv_store (window_hash_member, "win_height",     10, newSViv (ptr_win->win_height), 0);
	hv_store (window_hash_member, "win_width_pct",  13, newSViv (ptr_win->win_width_pct), 0);
	hv_store (window_hash_member, "win_height_pct", 14, newSViv (ptr_win->win_height_pct), 0);
	
	XPUSHs(newRV_inc((SV *) window_hash_member));
	count++;
    }    
    weechat_perl_plugin->free_window_info (weechat_perl_plugin, window_info);
    
    XSRETURN (count);
}
*/

/*
 * weechat::get_buffer_info: get infos about buffers
 */

/*
static XS (XS_weechat_get_buffer_info)
{
    t_plugin_buffer_info *buffer_info, *ptr_buffer;
    HV *buffer_hash, *buffer_hash_member;
    char conv[8];
    dXSARGS;
    
    // make C compiler happy
    (void) cv;
    (void) items;
    
    if (!perl_current_script)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: unable to get buffer info, "
                                   "script not initialized");
	PERL_RETURN_EMPTY;
    }
    
    buffer_info = weechat_perl_plugin->get_buffer_info (weechat_perl_plugin);
    if (!buffer_info)
    {
	PERL_RETURN_EMPTY;
    }    
    
    buffer_hash = (HV *) sv_2mortal((SV *) newHV());
    if (!buffer_hash)
    {
        weechat_perl_plugin->free_buffer_info (weechat_perl_plugin, buffer_info);
	PERL_RETURN_EMPTY;
    }
    
    for (ptr_buffer = buffer_info; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
	buffer_hash_member = (HV *) sv_2mortal((SV *) newHV());	        

	hv_store (buffer_hash_member, "type",           4, newSViv (ptr_buffer->type), 0);
        hv_store (buffer_hash_member, "num_displayed", 13, newSViv (ptr_buffer->num_displayed), 0);
	hv_store (buffer_hash_member, "server",         6, 
		  newSVpv ((ptr_buffer->server_name == NULL) ? "" : ptr_buffer->server_name, 0), 0);
        hv_store (buffer_hash_member, "channel",        7, 
		  newSVpv ((ptr_buffer->channel_name == NULL) ? "" : ptr_buffer->channel_name, 0), 0);
        hv_store (buffer_hash_member, "notify_level",  12, newSViv (ptr_buffer->notify_level), 0);
	hv_store (buffer_hash_member, "log_filename",  12, 
		  newSVpv ((ptr_buffer->log_filename == NULL) ? "" : ptr_buffer->log_filename, 0), 0);
	snprintf(conv, sizeof(conv), "%d", ptr_buffer->number);
	hv_store (buffer_hash, conv, strlen(conv), newRV_inc((SV *) buffer_hash_member), 0);
    }    
    weechat_perl_plugin->free_buffer_info (weechat_perl_plugin, buffer_info);
    
    ST (0) = newRV_inc((SV *) buffer_hash);
    if (SvREFCNT(ST(0))) sv_2mortal(ST(0));
    
    XSRETURN (1);
}
*/

/*
 * weechat::get_buffer_data: get buffer content
 */

/*
static XS (XS_weechat_get_buffer_data)
{
    t_plugin_buffer_line *buffer_data, *ptr_data;
    HV *data_list_member;
    char *server, *channel;
    char timebuffer[64];
    int count;
    
    dXSARGS;
    
    // make C compiler happy
    (void) cv;
    (void) items;
    
    if (!perl_current_script)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: unable to get buffer data, "
                                   "script not initialized");
	PERL_RETURN_EMPTY;
    }
    
    if (items != 2)
    {
        weechat_perl_plugin->print_server (weechat_perl_plugin,
                                   "Perl error: wrong parameters for "
                                   "\"get_buffer_data\" function");
        PERL_RETURN_EMPTY;
    }
    
    channel = NULL;
    server = NULL;

    if (items >= 1)
	server = SvPV (ST (0), PL_na);
    if (items >= 2)
	channel = SvPV (ST (1), PL_na);

    SP -= items;
    
    buffer_data = weechat_perl_plugin->get_buffer_data (weechat_perl_plugin, server, channel);
    count = 0;
    if (!buffer_data)
	PERL_RETURN_EMPTY;
    
    for (ptr_data = buffer_data; ptr_data; ptr_data = ptr_data->next_line)
    {
	data_list_member = (HV *) sv_2mortal((SV *) newHV());

	strftime(timebuffer, sizeof(timebuffer), "%F %T",
		 localtime(&ptr_data->date));
        
	hv_store (data_list_member, "date", 4, newSVpv (timebuffer, 0), 0);
	hv_store (data_list_member, "nick", 4, 
		  newSVpv ((ptr_data->nick == NULL) ? "" : ptr_data->nick, 0), 0);
	hv_store (data_list_member, "data", 4, 
		  newSVpv ((ptr_data->data == NULL) ? "" : ptr_data->data, 0), 0);
	
	XPUSHs(newRV_inc((SV *) data_list_member));
	count++;
    }    
    weechat_perl_plugin->free_buffer_data (weechat_perl_plugin, buffer_data);
    
    XSRETURN (count);
}
*/

/*
 * weechat_perl_xs_init: initialize subroutines
 */

void
weechat_perl_api_init (pTHX)
{
    HV *stash;
    
    newXS ("DynaLoader::boot_DynaLoader", boot_DynaLoader, __FILE__);
    
    /* interface functions */
    newXS ("weechat::register", XS_weechat_register, "weechat");
    newXS ("weechat::charset_set", XS_weechat_charset_set, "weechat");
    newXS ("weechat::iconv_to_internal", XS_weechat_iconv_to_internal, "weechat");
    newXS ("weechat::iconv_from_internal", XS_weechat_iconv_from_internal, "weechat");
    newXS ("weechat::mkdir_home", XS_weechat_mkdir_home, "weechat");
    newXS ("weechat::mkdir", XS_weechat_mkdir, "weechat");
    newXS ("weechat::prefix", XS_weechat_prefix, "weechat");
    newXS ("weechat::color", XS_weechat_color, "weechat");
    newXS ("weechat::print", XS_weechat_print, "weechat");
    newXS ("weechat::infobar_print", XS_weechat_infobar_print, "weechat");
    newXS ("weechat::infobar_remove", XS_weechat_infobar_remove, "weechat");
    newXS ("weechat::log_print", XS_weechat_log_print, "weechat");
    newXS ("weechat::hook_command", XS_weechat_hook_command, "weechat");
    newXS ("weechat::hook_timer", XS_weechat_hook_timer, "weechat");
    newXS ("weechat::hook_fd", XS_weechat_hook_fd, "weechat");
    newXS ("weechat::hook_print", XS_weechat_hook_print, "weechat");
    newXS ("weechat::hook_signal", XS_weechat_hook_signal, "weechat");
    newXS ("weechat::hook_signal_send", XS_weechat_hook_signal_send, "weechat");
    newXS ("weechat::hook_config", XS_weechat_hook_config, "weechat");
    newXS ("weechat::hook_completion", XS_weechat_hook_completion, "weechat");
    newXS ("weechat::unhook", XS_weechat_unhook, "weechat");
    newXS ("weechat::unhook_all", XS_weechat_unhook_all, "weechat");
    newXS ("weechat::buffer_new", XS_weechat_buffer_new, "weechat");
    newXS ("weechat::buffer_search", XS_weechat_buffer_search, "weechat");
    newXS ("weechat::buffer_close", XS_weechat_buffer_close, "weechat");
    newXS ("weechat::buffer_get", XS_weechat_buffer_get, "weechat");
    newXS ("weechat::buffer_set", XS_weechat_buffer_set, "weechat");
    newXS ("weechat::nicklist_add_group", XS_weechat_nicklist_add_group, "weechat");
    newXS ("weechat::nicklist_search_group", XS_weechat_nicklist_search_group, "weechat");
    newXS ("weechat::nicklist_add_nick", XS_weechat_nicklist_add_nick, "weechat");
    newXS ("weechat::nicklist_search_nick", XS_weechat_nicklist_search_nick, "weechat");
    newXS ("weechat::nicklist_remove_group", XS_weechat_nicklist_remove_group, "weechat");
    newXS ("weechat::nicklist_remove_nick", XS_weechat_nicklist_remove_nick, "weechat");
    newXS ("weechat::nicklist_remove_all", XS_weechat_nicklist_remove_all, "weechat");
    newXS ("weechat::command", XS_weechat_command, "weechat");
    newXS ("weechat::info_get", XS_weechat_info_get, "weechat");
    //newXS ("weechat::get_dcc_info", XS_weechat_get_dcc_info, "weechat");
    //newXS ("weechat::get_config", XS_weechat_get_config, "weechat");
    //newXS ("weechat::set_config", XS_weechat_set_config, "weechat");
    //newXS ("weechat::get_plugin_config", XS_weechat_get_plugin_config, "weechat");
    //newXS ("weechat::set_plugin_config", XS_weechat_set_plugin_config, "weechat");
    //newXS ("weechat::get_server_info", XS_weechat_get_server_info, "weechat");
    //newXS ("weechat::get_channel_info", XS_weechat_get_channel_info, "weechat");
    //newXS ("weechat::get_nick_info", XS_weechat_get_nick_info, "weechat");
    //newXS ("weechat::input_color", XS_weechat_input_color, "weechat");
    //newXS ("weechat::get_irc_color", XS_weechat_get_irc_color, "weechat");
    //newXS ("weechat::get_window_info", XS_weechat_get_window_info, "weechat");
    //newXS ("weechat::get_buffer_info", XS_weechat_get_buffer_info, "weechat");
    //newXS ("weechat::get_buffer_data", XS_weechat_get_buffer_data, "weechat");

    /* interface constants */
    stash = gv_stashpv ("weechat", TRUE);
    newCONSTSUB (stash, "weechat::WEECHAT_RC_OK", newSViv (WEECHAT_RC_OK));
    newCONSTSUB (stash, "weechat::WEECHAT_RC_ERROR", newSViv (WEECHAT_RC_ERROR));
    newCONSTSUB (stash, "weechat::WEECHAT_RC_OK_IGNORE_WEECHAT", newSViv (WEECHAT_RC_OK_IGNORE_WEECHAT));
    newCONSTSUB (stash, "weechat::WEECHAT_RC_OK_IGNORE_PLUGINS", newSViv (WEECHAT_RC_OK_IGNORE_PLUGINS));
    newCONSTSUB (stash, "weechat::WEECHAT_RC_OK_IGNORE_ALL", newSViv (WEECHAT_RC_OK_IGNORE_ALL));
    newCONSTSUB (stash, "weechat::WEECHAT_RC_OK_WITH_HIGHLIGHT", newSViv (WEECHAT_RC_OK_WITH_HIGHLIGHT));
    newCONSTSUB (stash, "weechat::WEECHAT_LIST_POS_SORT", newSVpv (WEECHAT_LIST_POS_SORT, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_LIST_POS_BEGINNING", newSVpv (WEECHAT_LIST_POS_BEGINNING, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_LIST_POS_END", newSVpv (WEECHAT_LIST_POS_END, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_HOTLIST_LOW", newSVpv (WEECHAT_HOTLIST_LOW, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_HOTLIST_MESSAGE", newSVpv (WEECHAT_HOTLIST_MESSAGE, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_HOTLIST_PRIVATE", newSVpv (WEECHAT_HOTLIST_PRIVATE, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_HOTLIST_HIGHLIGHT", newSVpv (WEECHAT_HOTLIST_HIGHLIGHT, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_SIGNAL_STRING", newSVpv (WEECHAT_HOOK_SIGNAL_STRING, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_SIGNAL_INT", newSVpv (WEECHAT_HOOK_SIGNAL_INT, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_SIGNAL_POINTER", newSVpv (WEECHAT_HOOK_SIGNAL_POINTER, PL_na));
}
