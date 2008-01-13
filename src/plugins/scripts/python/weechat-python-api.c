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

/* weechat-python-api.c: Python API functions */

#undef _

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Python.h>

#include "../../weechat-plugin.h"
#include "../script.h"
#include "../script-api.h"
#include "../script-callback.h"
#include "weechat-python.h"


#define PYTHON_RETURN_OK return Py_BuildValue ("i", 1);
#define PYTHON_RETURN_ERROR return Py_BuildValue ("i", 0);
#define PYTHON_RETURN_EMPTY \
    Py_INCREF(Py_None);     \
    return Py_None;
#define PYTHON_RETURN_STRING(__string)             \
    if (__string)                                  \
        return Py_BuildValue ("s", __string);      \
    return Py_BuildValue ("s", "");
#define PYTHON_RETURN_STRING_FREE(__string)        \
    if (__string)                                  \
    {                                              \
        object = Py_BuildValue ("s", __string);    \
        free (__string);                           \
        return object;                             \
    }                                              \
    return Py_BuildValue ("s", "");


/*
 * weechat_python_api_register: startup function for all WeeChat Python scripts
 */

static PyObject *
weechat_python_api_register (PyObject *self, PyObject *args)
{
    char *name, *author, *version, *license, *shutdown_func, *description;
    char *charset;
    
    /* make C compiler happy */
    (void) self;
    
    python_current_script = NULL;
    
    name = NULL;
    author = NULL;
    version = NULL;
    license = NULL;
    description = NULL;
    shutdown_func = NULL;
    charset = NULL;
    
    if (!PyArg_ParseTuple (args, "sssssss", &name, &author, &version,
                           &license, &description, &shutdown_func, &charset))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("register");
        PYTHON_RETURN_ERROR;
    }
    
    if (script_search (weechat_python_plugin, python_scripts, name))
    {
        /* error: another scripts already exists with this name! */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to register script "
                                         "\"%s\" (another script already "
                                         "exists with this name)"),
                        weechat_prefix ("error"), "python", name);
        PYTHON_RETURN_ERROR;
    }
    
    /* register script */
    python_current_script = script_add (weechat_python_plugin,
                                        &python_scripts,
                                        (python_current_script_filename) ?
                                        python_current_script_filename : "",
                                        name, author, version, license,
                                        description, shutdown_func, charset);
    if (python_current_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: registered script \"%s\", "
                                         "version %s (%s)"),
                        weechat_prefix ("info"), "python",
                        name, version, description);
    }
    else
    {
        PYTHON_RETURN_ERROR;
    }
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_charset_set: set script charset
 */

static PyObject *
weechat_python_api_charset_set (PyObject *self, PyObject *args)
{
    char *charset;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("charset_set");
        PYTHON_RETURN_ERROR;
    }
    
    charset = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &charset))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("charset_set");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_charset_set (python_current_script,
                            charset);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_iconv_to_internal: convert string to internal WeeChat charset
 */

static PyObject *
weechat_python_api_iconv_to_internal (PyObject *self, PyObject *args)
{
    char *charset, *string, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("iconv_to_internal");
        PYTHON_RETURN_EMPTY;
    }
    
    charset = NULL;
    string = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &charset, &string))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("iconv_to_internal");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_iconv_to_internal (charset, string);
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_iconv_from_internal: convert string from WeeChat internal
 *                                         charset to another one
 */

static PyObject *
weechat_python_api_iconv_from_internal (PyObject *self, PyObject *args)
{
    char *charset, *string, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("iconv_from_internal");
        PYTHON_RETURN_EMPTY;
    }
    
    charset = NULL;
    string = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &charset, &string))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("iconv_from_internal");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_iconv_from_internal (charset, string);
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_mkdir_home: create a directory in WeeChat home
 */

static PyObject *
weechat_python_api_mkdir_home (PyObject *self, PyObject *args)
{
    char *directory;
    int mode;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("mkdir_home");
        PYTHON_RETURN_ERROR;
    }
    
    directory = NULL;
    mode = 0;
    
    if (!PyArg_ParseTuple (args, "si", &directory, &mode))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("mkdir_home");
        PYTHON_RETURN_ERROR;
    }
    
    if (weechat_mkdir_home (directory, mode))
        PYTHON_RETURN_OK;

    PYTHON_RETURN_ERROR;
}

/*
 * weechat_python_api_mkdir: create a directory
 */

static PyObject *
weechat_python_api_mkdir (PyObject *self, PyObject *args)
{
    char *directory;
    int mode;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("mkdir");
        PYTHON_RETURN_ERROR;
    }
    
    directory = NULL;
    mode = 0;
    
    if (!PyArg_ParseTuple (args, "si", &directory, &mode))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("mkdir");
        PYTHON_RETURN_ERROR;
    }
    
    if (weechat_mkdir (directory, mode))
        PYTHON_RETURN_OK;
    
    PYTHON_RETURN_ERROR;
}

/*
 * weechat_python_api_prefix: get a prefix, used for display
 */

static PyObject *
weechat_python_api_prefix (PyObject *self, PyObject *args)
{
    char *prefix, *result;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("prefix");
        PYTHON_RETURN_EMPTY;
    }
    
    prefix = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &prefix))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("prefix");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_prefix (prefix);
    PYTHON_RETURN_STRING(result);
}

/*
 * weechat_python_api_color: get a color code, used for display
 */

static PyObject *
weechat_python_api_color (PyObject *self, PyObject *args)
{
    char *color, *result;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("color");
        PYTHON_RETURN_EMPTY;
    }
    
    color = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &color))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("color");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_prefix (color);
    PYTHON_RETURN_STRING(result);
}

/*
 * weechat_python_api_prnt: print message into a buffer (current or specified one)
 */

static PyObject *
weechat_python_api_prnt (PyObject *self, PyObject *args)
{
    char *buffer, *message;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("prnt");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    message = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &buffer, &message))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("prnt");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_printf (weechat_python_plugin,
                       python_current_script,
                       script_string_to_pointer (buffer),
                       "%s", message);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_infobar_print: print message to infobar
 */

static PyObject *
weechat_python_api_infobar_print (PyObject *self, PyObject *args)
{
    int delay;
    char *color, *message;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_print");
        PYTHON_RETURN_ERROR;
    }
    
    delay = 1;
    message = NULL;
    
    if (!PyArg_ParseTuple (args, "iss", &delay, &color, &message))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infobar_print");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_infobar_printf (weechat_python_plugin,
                               python_current_script,
                               delay, color, "%s", message);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_infobar_remove: remove message(s) from infobar
 */

static PyObject *
weechat_python_api_infobar_remove (PyObject *self, PyObject *args)
{
    int how_many;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_print");
        PYTHON_RETURN_ERROR;
    }
    
    how_many = 0;
    
    if (!PyArg_ParseTuple (args, "|i", &how_many))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infobar_print");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_infobar_remove (how_many);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_log_print: print message in WeeChat log file
 */

static PyObject *
weechat_python_api_log_print (PyObject *self, PyObject *args)
{
    char *message;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("log_print");
        PYTHON_RETURN_ERROR;
    }
    
    message = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &message))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("log_print");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_log_printf (weechat_python_plugin,
                           python_current_script,
                           "%s", message);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_hook_command_cb: callback for command hooked
 */

int
weechat_python_api_hook_command_cb (void *data, struct t_gui_buffer *buffer,
                                    int argc, char **argv, char **argv_eol)
{
    struct t_script_callback *script_callback;
    char *python_argv[3], empty_arg[1] = { '\0' };
    int *rc, ret;

    /* make C compiler happy */
    (void) argv;
    
    script_callback = (struct t_script_callback *)data;

    python_argv[0] = script_pointer_to_string (buffer);
    python_argv[1] = (argc > 1) ? argv_eol[1] : empty_arg;
    python_argv[2] = NULL;
    
    rc = (int *) weechat_python_exec (script_callback->script,
                                      WEECHAT_SCRIPT_EXEC_INT,
                                      script_callback->function,
                                      python_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (python_argv[0])
        free (python_argv[0]);
    
    return ret;
}

/*
 * weechat_python_api_hook_command: hook a command
 */

static PyObject *
weechat_python_api_hook_command (PyObject *self, PyObject *args)
{
    char *command, *description, *arguments, *args_description, *completion;
    char *function, *result;
    struct t_hook *new_hook;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_command");
        PYTHON_RETURN_EMPTY;
    }
    
    command = NULL;
    description = NULL;
    arguments = NULL;
    args_description = NULL;
    completion = NULL;
    function = NULL;
    
    if (!PyArg_ParseTuple (args, "ssssss", &command, &description, &arguments,
                           &args_description, &completion, &function))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_command");
        PYTHON_RETURN_ERROR;
    }
    
    new_hook = script_api_hook_command (weechat_python_plugin,
                                        python_current_script,
                                        command,
                                        description,
                                        arguments,
                                        args_description,
                                        completion,
                                        &weechat_python_api_hook_command_cb,
                                        function);
    
    result = script_pointer_to_string (new_hook);
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_hook_timer_cb: callback for timer hooked
 */

int
weechat_python_api_hook_timer_cb (void *data)
{
    struct t_script_callback *script_callback;
    char *python_argv[1] = { NULL };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    rc = (int *) weechat_python_exec (script_callback->script,
                                      WEECHAT_SCRIPT_EXEC_INT,
                                      script_callback->function,
                                      python_argv);
    
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
 * weechat_python_api_hook_timer: hook a timer
 */

static PyObject *
weechat_python_api_hook_timer (PyObject *self, PyObject *args)
{
    int interval, align_second, max_calls;
    char *function, *result;
    struct t_hook *new_hook;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_timer");
        PYTHON_RETURN_ERROR;
    }
    
    interval = 10;
    align_second = 0;
    max_calls = 0;
    function = NULL;
    
    if (!PyArg_ParseTuple (args, "iiis", &interval, &align_second, &max_calls,
                           &function))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_timer");
        PYTHON_RETURN_ERROR;
    }
    
    new_hook = script_api_hook_timer (weechat_python_plugin,
                                      python_current_script,
                                      interval,
                                      align_second,
                                      max_calls,
                                      &weechat_python_api_hook_timer_cb,
                                      function);
    
    result = script_pointer_to_string (new_hook);
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_hook_fd_cb: callback for fd hooked
 */

int
weechat_python_api_hook_fd_cb (void *data)
{
    struct t_script_callback *script_callback;
    char *python_argv[1] = { NULL };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    rc = (int *) weechat_python_exec (script_callback->script,
                                      WEECHAT_SCRIPT_EXEC_INT,
                                      script_callback->function,
                                      python_argv);
    
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
 * weechat_python_api_hook_fd: hook a fd
 */

static PyObject *
weechat_python_api_hook_fd (PyObject *self, PyObject *args)
{
    int fd, read, write, exception;
    char *function, *result;
    struct t_hook *new_hook;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_fd");
        PYTHON_RETURN_ERROR;
    }
    
    fd = 0;
    read = 0;
    write = 0;
    exception = 0;
    function = NULL;
    
    if (!PyArg_ParseTuple (args, "iiiis", &fd, &read, &write, &exception,
                           &function))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_fd");
        PYTHON_RETURN_ERROR;
    }
    
    new_hook = script_api_hook_fd (weechat_python_plugin,
                                   python_current_script,
                                   fd,
                                   read,
                                   write,
                                   exception,
                                   &weechat_python_api_hook_fd_cb,
                                   function);

    result = script_pointer_to_string (new_hook);
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_hook_print_cb: callback for print hooked
 */

int
weechat_python_api_hook_print_cb (void *data, struct t_gui_buffer *buffer,
                                  time_t date, char *prefix, char *message)
{
    struct t_script_callback *script_callback;
    char *python_argv[5];
    static char timebuffer[64];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    snprintf (timebuffer, sizeof (timebuffer) - 1, "%ld", date);
    
    python_argv[0] = script_pointer_to_string (buffer);
    python_argv[1] = timebuffer;
    python_argv[2] = prefix;
    python_argv[3] = message;
    python_argv[4] = NULL;
    
    rc = (int *) weechat_python_exec (script_callback->script,
                                      WEECHAT_SCRIPT_EXEC_INT,
                                      script_callback->function,
                                      python_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (python_argv[0])
        free (python_argv[0]);
    
    return ret;
}

/*
 * weechat_python_api_hook_print: hook a print
 */

static PyObject *
weechat_python_api_hook_print (PyObject *self, PyObject *args)
{
    char *buffer, *message, *function, *result;
    int strip_colors;
    struct t_hook *new_hook;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_print");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    message = NULL;
    strip_colors = 0;
    function = NULL;
    
    if (!PyArg_ParseTuple (args, "ssis", &buffer, &message, &strip_colors,
                           &function))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_print");
        PYTHON_RETURN_ERROR;
    }
    
    new_hook = script_api_hook_print (weechat_python_plugin,
                                      python_current_script,
                                      script_string_to_pointer (buffer),
                                      message,
                                      strip_colors,
                                      &weechat_python_api_hook_print_cb,
                                      function);

    result = script_pointer_to_string(new_hook);
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_hook_signal_cb: callback for signal hooked
 */

int
weechat_python_api_hook_signal_cb (void *data, char *signal, char *type_data,
                                   void *signal_data)
{
    struct t_script_callback *script_callback;
    char *python_argv[3];
    static char value_str[64];
    int *rc, ret, free_needed;
    
    script_callback = (struct t_script_callback *)data;
    
    python_argv[0] = signal;
    free_needed = 0;
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        python_argv[1] = (char *)signal_data;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        snprintf (value_str, sizeof (value_str) - 1,
                  "%d", *((int *)signal_data));
        python_argv[1] = value_str;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        python_argv[1] = script_pointer_to_string (signal_data);
        free_needed = 1;
    }
    else
        python_argv[1] = NULL;
    python_argv[2] = NULL;
    
    rc = (int *) weechat_python_exec (script_callback->script,
                                      WEECHAT_SCRIPT_EXEC_INT,
                                      script_callback->function,
                                      python_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (free_needed && python_argv[1])
        free (python_argv[1]);
    
    return ret;
}

/*
 * weechat_python_api_hook_signal: hook a signal
 */

static PyObject *
weechat_python_api_hook_signal (PyObject *self, PyObject *args)
{
    char *signal, *function, *result;
    struct t_hook *new_hook;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_signal");
        PYTHON_RETURN_ERROR;
    }
    
    signal = NULL;
    function = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &signal, &function))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_signal");
        PYTHON_RETURN_ERROR;
    }
    
    new_hook = script_api_hook_signal (weechat_python_plugin,
                                       python_current_script,
                                       signal,
                                       &weechat_python_api_hook_signal_cb,
                                       function);
    
    result = script_pointer_to_string (new_hook);
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_hook_signal_send: send a signal
 */

static PyObject *
weechat_python_api_hook_signal_send (PyObject *self, PyObject *args)
{
    char *signal, *type_data, *signal_data, *error;
    long number;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_signal_send");
        PYTHON_RETURN_ERROR;
    }
    
    signal = NULL;
    type_data = NULL;
    signal_data = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &signal, &type_data, &signal_data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_signal_send");
        PYTHON_RETURN_ERROR;
    }

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        weechat_hook_signal_send (signal, type_data, signal_data);
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        error = NULL;
        number = (int)strtol (signal_data, &error, 10);
        if (error && (error[0] == '\0'))
        {
            weechat_hook_signal_send (signal, type_data, &number);
        }
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        weechat_hook_signal_send (signal, type_data,
                                  script_string_to_pointer (signal_data));
    }
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_hook_config_cb: callback for config option hooked
 */

int
weechat_python_api_hook_config_cb (void *data, char *type, char *option,
                                   char *value)
{
    struct t_script_callback *script_callback;
    char *python_argv[4];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    python_argv[0] = type;
    python_argv[1] = option;
    python_argv[2] = value;
    python_argv[3] = NULL;
    
    rc = (int *) weechat_python_exec (script_callback->script,
                                      WEECHAT_SCRIPT_EXEC_INT,
                                      script_callback->function,
                                      python_argv);
    
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
 * weechat_python_api_hook_config: hook a config option
 */

static PyObject *
weechat_python_api_hook_config (PyObject *self, PyObject *args)
{
    char *type, *option, *function, *result;
    struct t_hook *new_hook;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_config");
        PYTHON_RETURN_ERROR;
    }
    
    type = NULL;
    option = NULL;
    function = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &type, &option, &function))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_config");
        PYTHON_RETURN_ERROR;
    }
    
    new_hook = script_api_hook_config (weechat_python_plugin,
                                       python_current_script,
                                       type,
                                       option,
                                       &weechat_python_api_hook_config_cb,
                                       function);

    result = script_pointer_to_string(new_hook);
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_hook_completion_cb: callback for completion hooked
 */

int
weechat_python_api_hook_completion_cb (void *data, char *completion,
                                       struct t_gui_buffer *buffer,
                                       struct t_weelist *list)
{
    struct t_script_callback *script_callback;
    char *python_argv[4];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    python_argv[0] = completion;
    python_argv[1] = script_pointer_to_string (buffer);
    python_argv[2] = script_pointer_to_string (list);
    python_argv[3] = NULL;
    
    rc = (int *) weechat_python_exec (script_callback->script,
                                      WEECHAT_SCRIPT_EXEC_INT,
                                      script_callback->function,
                                      python_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (python_argv[1])
        free (python_argv[1]);
    if (python_argv[2])
        free (python_argv[2]);
    
    return ret;
}

/*
 * weechat_python_api_hook_completion: hook a completion
 */

static PyObject *
weechat_python_api_hook_completion (PyObject *self, PyObject *args)
{
    char *completion, *function, *result;
    struct t_hook *new_hook;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_completion");
        PYTHON_RETURN_ERROR;
    }
    
    completion = NULL;
    function = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &completion, &function))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_completion");
        PYTHON_RETURN_ERROR;
    }
    
    new_hook = script_api_hook_completion (weechat_python_plugin,
                                           python_current_script,
                                           completion,
                                           &weechat_python_api_hook_completion_cb,
                                           function);

    result = script_pointer_to_string(new_hook);
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_unhook: unhook something
 */

static PyObject *
weechat_python_api_unhook (PyObject *self, PyObject *args)
{
    char *hook;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("unhook");
        PYTHON_RETURN_ERROR;
    }
    
    hook = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &hook))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("unhook");
        PYTHON_RETURN_ERROR;
    }
    
    if (script_api_unhook (weechat_python_plugin,
                           python_current_script,
                           script_string_to_pointer (hook)))
        PYTHON_RETURN_OK;
    
    PYTHON_RETURN_ERROR;
}

/*
 * weechat_python_api_unhook_all: unhook all for script
 */

static PyObject *
weechat_python_api_unhook_all (PyObject *self, PyObject *args)
{
    /* make C compiler happy */
    (void) self;
    (void) args;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("unhook_all");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_unhook_all (weechat_python_plugin,
                           python_current_script);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_input_data_cb: callback for input data in a buffer
 */

int
weechat_python_api_input_data_cb (void *data, struct t_gui_buffer *buffer,
                                  char *input_data)
{
    struct t_script_callback *script_callback;
    char *python_argv[3];
    int *r, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    python_argv[0] = script_pointer_to_string (buffer);
    python_argv[1] = input_data;
    python_argv[2] = NULL;
    
    r = (int *) weechat_python_exec (script_callback->script,
                                     WEECHAT_SCRIPT_EXEC_INT,
                                     script_callback->function,
                                     python_argv);
    if (!r)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *r;
        free (r);
    }
    if (python_argv[0])
        free (python_argv[0]);
    
    return ret;
}

/*
 * weechat_python_api_buffer_new: create a new buffer
 */

static PyObject *
weechat_python_api_buffer_new (PyObject *self, PyObject *args)
{
    struct t_gui_buffer *new_buffer;
    char *category, *name, *function, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_new");
        PYTHON_RETURN_EMPTY;
    }
    
    category = NULL;
    name = NULL;
    function = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &category, &name, &function))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_new");
        PYTHON_RETURN_EMPTY;
    }
    
    new_buffer = script_api_buffer_new (weechat_python_plugin,
                                        python_current_script,
                                        category,
                                        name,
                                        &weechat_python_api_input_data_cb,
                                        function);
    
    result = script_pointer_to_string (new_buffer);
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_buffer_search: search a buffer
 */

static PyObject *
weechat_python_api_buffer_search (PyObject *self, PyObject *args)
{
    char *category, *name;
    struct t_gui_buffer *ptr_buffer;
    char *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_search");
        PYTHON_RETURN_EMPTY;
    }
    
    category = NULL;
    name = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &category, &name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_search");
        PYTHON_RETURN_EMPTY;
    }
    
    ptr_buffer = weechat_buffer_search (category, name);
    
    result = script_pointer_to_string (ptr_buffer);
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_buffer_close: close a buffer
 */

static PyObject *
weechat_python_api_buffer_close (PyObject *self, PyObject *args)
{
    char *buffer;
    int switch_to_another;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_close");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    switch_to_another = 0;
    
    if (!PyArg_ParseTuple (args, "si", &buffer, &switch_to_another))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_close");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_buffer_close (weechat_python_plugin,
                             python_current_script,
                             script_string_to_pointer (buffer),
                             switch_to_another);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_buffer_get: get a buffer property
 */

static PyObject *
weechat_python_api_buffer_get (PyObject *self, PyObject *args)
{
    char *buffer, *property, *value;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_get");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    property = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &buffer, &property))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_get");
        PYTHON_RETURN_ERROR;
    }
    
    value = weechat_buffer_get (script_string_to_pointer (buffer), property);
    PYTHON_RETURN_STRING(value);
}

/*
 * weechat_python_api_buffer_set: set a buffer property
 */

static PyObject *
weechat_python_api_buffer_set (PyObject *self, PyObject *args)
{
    char *buffer, *property, *value;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_set");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    property = NULL;
    value = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &buffer, &property, &value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_set");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_buffer_set (script_string_to_pointer (buffer),
                        property,
                        value);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_nicklist_add_group: add a group in nicklist
 */

static PyObject *
weechat_python_api_nicklist_add_group (PyObject *self, PyObject *args)
{
    struct t_gui_nick_group *new_group;
    char *buffer, *parent_group, *name, *color, *result;
    int visible;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_add_group");
        PYTHON_RETURN_EMPTY;
    }
    
    buffer = NULL;
    parent_group = NULL;
    name = NULL;
    color = NULL;
    visible = 0;
    
    if (!PyArg_ParseTuple (args, "ssssi", &buffer, &parent_group, &name,
                           &color, &visible))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_add_group");
        PYTHON_RETURN_EMPTY;
    }
    
    new_group = weechat_nicklist_add_group (script_string_to_pointer (buffer),
                                            script_string_to_pointer (parent_group),
                                            name,
                                            color,
                                            visible);
    
    result = script_pointer_to_string (new_group);
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_nicklist_search_group: search a group in nicklist
 */

static PyObject *
weechat_python_api_nicklist_search_group (PyObject *self, PyObject *args)
{
    struct t_gui_nick_group *ptr_group;
    char *buffer, *from_group, *name, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_search_group");
        PYTHON_RETURN_EMPTY;
    }
    
    buffer = NULL;
    from_group = NULL;
    name = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &buffer, &from_group, &name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_search_group");
        PYTHON_RETURN_EMPTY;
    }
    
    ptr_group = weechat_nicklist_search_group (script_string_to_pointer (buffer),
                                               script_string_to_pointer (from_group),
                                               name);
    
    result = script_pointer_to_string (ptr_group);
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_nicklist_add_nick: add a nick in nicklist
 */

static PyObject *
weechat_python_api_nicklist_add_nick (PyObject *self, PyObject *args)
{
    struct t_gui_nick *new_nick;
    char *buffer, *group, *name, *color, *prefix, *prefix_color, *result;
    char char_prefix;
    int visible;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_add_nick");
        PYTHON_RETURN_EMPTY;
    }
    
    buffer = NULL;
    group = NULL;
    name = NULL;
    color = NULL;
    prefix = NULL;
    prefix_color = NULL;
    visible = 0;
    
    if (!PyArg_ParseTuple (args, "ssssssi", &buffer, &group, &name, &color,
                           &prefix, &prefix_color, &visible))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_add_nick");
        PYTHON_RETURN_EMPTY;
    }
    
    if (prefix && prefix[0])
        char_prefix = prefix[0];
    else
        char_prefix = ' ';
    
    new_nick = weechat_nicklist_add_nick (script_string_to_pointer (buffer),
                                          script_string_to_pointer (group),
                                          name,
                                          color,
                                          char_prefix,
                                          prefix_color,
                                          visible);
    
    result = script_pointer_to_string (new_nick);
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_nicklist_search_nick: search a nick in nicklist
 */

static PyObject *
weechat_python_api_nicklist_search_nick (PyObject *self, PyObject *args)
{
    struct t_gui_nick *ptr_nick;
    char *buffer, *from_group, *name, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_search_nick");
        PYTHON_RETURN_EMPTY;
    }
    
    buffer = NULL;
    from_group = NULL;
    name = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &buffer, &from_group, &name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_search_nick");
        PYTHON_RETURN_EMPTY;
    }
    
    ptr_nick = weechat_nicklist_search_nick (script_string_to_pointer (buffer),
                                             script_string_to_pointer (from_group),
                                             name);
    
    result = script_pointer_to_string (ptr_nick);
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_nicklist_remove_group: remove a group from nicklist
 */

static PyObject *
weechat_python_api_nicklist_remove_group (PyObject *self, PyObject *args)
{
    char *buffer, *group;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_remove_group");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    group = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &buffer, &group))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_remove_group");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_nicklist_remove_group (script_string_to_pointer (buffer),
                                   script_string_to_pointer (group));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_nicklist_remove_nick: remove a nick from nicklist
 */

static PyObject *
weechat_python_api_nicklist_remove_nick (PyObject *self, PyObject *args)
{
    char *buffer, *nick;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_remove_nick");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    nick = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &buffer, &nick))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_remove_nick");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_nicklist_remove_group (script_string_to_pointer (buffer),
                                   script_string_to_pointer (nick));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_nicklist_remove_all: remove all groups/nicks from nicklist
 */

static PyObject *
weechat_python_api_nicklist_remove_all (PyObject *self, PyObject *args)
{
    char *buffer;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_remove_all");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &buffer))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_remove_all");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_nicklist_remove_all (script_string_to_pointer (buffer));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_command: send command to server
 */

static PyObject *
weechat_python_api_command (PyObject *self, PyObject *args)
{
    char *buffer, *command;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("command");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    command = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &buffer, &command))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("command");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_command (weechat_python_plugin,
                        python_current_script,
                        script_string_to_pointer (buffer),
                        command);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_info_get: get info about WeeChat
 */

static PyObject *
weechat_python_api_info_get (PyObject *self, PyObject *args)
{
    char *info, *value;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("info_get");
        PYTHON_RETURN_EMPTY;
    }
    
    info = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &info))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("info_get");
        PYTHON_RETURN_EMPTY;
    }
    
    value = weechat_info_get (info);
    PYTHON_RETURN_STRING(value);
}

/*
 * weechat_python_api_get_dcc_info: get infos about DCC
 */

/*
static PyObject *
weechat_python_api_get_dcc_info (PyObject *self, PyObject *args)
{
    t_plugin_dcc_info *dcc_info, *ptr_dcc;
    PyObject *dcc_list, *dcc_list_member, *key, *value;
    char timebuffer1[64];
    char timebuffer2[64];
    struct in_addr in;
    
    // make C compiler happy
    (void) self;
    (void) args;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_print");
        PYTHON_RETURN_EMPTY;
    }

    dcc_list = PyList_New (0);    
    if (!dcc_list)
    {
        PYTHON_RETURN_EMPTY;
    }
    
    dcc_info = python_plugin->get_dcc_info (python_plugin);
    if (!dcc_info)
	return dcc_list;
    
    for(ptr_dcc = dcc_info; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
    {
	strftime(timebuffer1, sizeof(timebuffer1), "%F %T",
		 localtime(&ptr_dcc->start_time));
	strftime(timebuffer2, sizeof(timebuffer2), "%F %T",
		 localtime(&ptr_dcc->start_transfer));
	in.s_addr = htonl(ptr_dcc->addr);
	
	dcc_list_member= PyDict_New();
	
        if (dcc_list_member) 
        {
	    key = Py_BuildValue("s", "server");
	    value = Py_BuildValue("s", ptr_dcc->server);
	    PyDict_SetItem(dcc_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "channel");
	    value = Py_BuildValue("s", ptr_dcc->channel);
	    PyDict_SetItem(dcc_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "type");
	    value = Py_BuildValue("i", ptr_dcc->type);
	    PyDict_SetItem(dcc_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "status");
	    value = Py_BuildValue("i", ptr_dcc->status);
	    PyDict_SetItem(dcc_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "start_time");
	    value = Py_BuildValue("s", timebuffer1);
	    PyDict_SetItem(dcc_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "start_transfer");
	    value = Py_BuildValue("s", timebuffer2);
	    PyDict_SetItem(dcc_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "address");
	    value = Py_BuildValue("s", inet_ntoa(in));
	    PyDict_SetItem(dcc_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "port");
	    value = Py_BuildValue("i", ptr_dcc->port);
	    PyDict_SetItem(dcc_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "nick");
	    value = Py_BuildValue("s", ptr_dcc->nick);
	    PyDict_SetItem(dcc_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "remote_file");
	    value = Py_BuildValue("s", ptr_dcc->filename);
	    PyDict_SetItem(dcc_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "local_file");
	    value = Py_BuildValue("s", ptr_dcc->local_filename);
	    PyDict_SetItem(dcc_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "filename_suffix");
	    value = Py_BuildValue("i", ptr_dcc->filename_suffix);
	    PyDict_SetItem(dcc_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "size");
	    value = Py_BuildValue("k", ptr_dcc->size);
	    PyDict_SetItem(dcc_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "pos");
	    value = Py_BuildValue("k", ptr_dcc->pos);
	    PyDict_SetItem(dcc_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "start_resume");
	    value = Py_BuildValue("k", ptr_dcc->start_resume);
	    PyDict_SetItem(dcc_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "cps");
	    value = Py_BuildValue("k", ptr_dcc->bytes_per_sec);
	    PyDict_SetItem(dcc_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
            PyList_Append(dcc_list, dcc_list_member);
	    Py_DECREF (dcc_list_member);
        }
    }
    
    python_plugin->free_dcc_info (python_plugin, dcc_info);
    
    return dcc_list;
}
*/

/*
 * weechat_python_api_get_config: get value of a WeeChat config option
 */

/*
static PyObject *
weechat_python_api_get_config (PyObject *self, PyObject *args)
{
    char *option, *return_value;
    PyObject *python_return_value;
    
    // make C compiler happy 
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_print");
        PYTHON_RETURN_EMPTY;
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infobar_print");
        PYTHON_RETURN_EMPTY;
    }
    
    if (option)
    {
        return_value = python_plugin->get_config (python_plugin, option);
        
        if (return_value)
        {
            python_return_value = Py_BuildValue ("s", return_value);
            free (return_value);
            return python_return_value;
        }
    }
    
    return Py_BuildValue ("s", "");
}
*/

/*
 * weechat_python_api_set_config: set value of a WeeChat config option
 */

/*
static PyObject *
weechat_python_api_set_config (PyObject *self, PyObject *args)
{
    char *option, *value;
    
    // make C compiler happy
    (void) self;

    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_print");
        PYTHON_RETURN_ERROR;
    }
    
    option = NULL;
    value = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &option, &value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infobar_print");
        PYTHON_RETURN_ERROR;
    }
    
    if (option && value)
    {
        if (python_plugin->set_config (python_plugin, option, value))
            PYTHON_RETURN_OK;
    }
    
    PYTHON_RETURN_ERROR;
}
*/

/*
 * weechat_python_api_get_plugin_config: get value of a plugin config option
 */

/*
static PyObject *
weechat_python_api_get_plugin_config (PyObject *self, PyObject *args)
{
    char *option, *return_value;
    PyObject *python_return_value;
    
    // make C compiler happy
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_print");
        PYTHON_RETURN_EMPTY;
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infobar_print");
        PYTHON_RETURN_EMPTY;
    }
    
    if (option)
    {
        return_value = weechat_script_get_plugin_config (python_plugin,
                                                         python_current_script,
                                                         option);
        
        if (return_value)
        {
            python_return_value = Py_BuildValue ("s", return_value);
            free (return_value);
            return python_return_value;
        }
    }
    
    return Py_BuildValue ("s", "");
}
*/

/*
 * weechat_python_api_set_plugin_config: set value of a plugin config option
 */

 /*
static PyObject *
weechat_python_api_set_plugin_config (PyObject *self, PyObject *args)
{
    char *option, *value;
    
    // make C compiler happy
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_print");
        PYTHON_RETURN_ERROR;
    }
    
    option = NULL;
    value = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &option, &value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infobar_print");
        PYTHON_RETURN_ERROR;
    }
    
    if (option && value)
    {
        if (weechat_script_set_plugin_config (python_plugin,
                                              python_current_script,
                                              option, value))
            PYTHON_RETURN_OK;
    }
    
    PYTHON_RETURN_ERROR;
}
*/

/*
 * weechat_python_api_get_server_info: get infos about servers
 */

/*
static PyObject *
weechat_python_api_get_server_info (PyObject *self, PyObject *args)
{
    t_plugin_server_info *server_info, *ptr_server;
    PyObject *server_hash, *server_hash_member, *key, *value;
    char timebuffer[64];
    
    // make C compiler happy
    (void) self;
    (void) args;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_print");
        PYTHON_RETURN_EMPTY;
    }
    
    server_hash = PyDict_New ();
    if (!server_hash)
    {
        PYTHON_RETURN_EMPTY;
    }

    server_info = python_plugin->get_server_info (python_plugin);
    if  (!server_info)
	return server_hash;

    for(ptr_server = server_info; ptr_server; ptr_server = ptr_server->next_server)
    {
	strftime(timebuffer, sizeof(timebuffer), "%F %T",
		 localtime(&ptr_server->away_time));
	
	server_hash_member = PyDict_New();
	
	if (server_hash_member)
	{
	    key = Py_BuildValue("s", "autoconnect");
	    value = Py_BuildValue("i", ptr_server->autoconnect);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "autoreconnect");
	    value = Py_BuildValue("i", ptr_server->autoreconnect);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "autoreconnect_delay");
	    value = Py_BuildValue("i", ptr_server->autoreconnect_delay);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "temp_server");
	    value = Py_BuildValue("i", ptr_server->temp_server);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "address");	   
	    value = Py_BuildValue("s", ptr_server->address);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "port");
	    value = Py_BuildValue("i", ptr_server->port);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "ipv6");
	    value = Py_BuildValue("i", ptr_server->ipv6);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "ssl");
	    value = Py_BuildValue("i", ptr_server->ssl);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    	    
	    key = Py_BuildValue("s", "password");
	    value = Py_BuildValue("s", ptr_server->password);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "nick1");
	    value = Py_BuildValue("s", ptr_server->nick1);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "nick2");
	    value = Py_BuildValue("s", ptr_server->nick2);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "nick3");
	    value = Py_BuildValue("s", ptr_server->nick3);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "username");
	    value = Py_BuildValue("s", ptr_server->username);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "realname");
	    value = Py_BuildValue("s", ptr_server->realname);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "command");
	    value = Py_BuildValue("s", ptr_server->command);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "command_delay");
	    value = Py_BuildValue("i", ptr_server->command_delay);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "autojoin");
	    value = Py_BuildValue("s", ptr_server->autojoin);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "autorejoin");
	    value = Py_BuildValue("i", ptr_server->autorejoin);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "notify_levels");
	    value = Py_BuildValue("s", ptr_server->notify_levels);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "is_connected");
	    value = Py_BuildValue("i", ptr_server->is_connected);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "ssl_connected");
	    value = Py_BuildValue("i", ptr_server->ssl_connected);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "nick");
	    value = Py_BuildValue("s", ptr_server->nick);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
            key = Py_BuildValue("s", "nick_modes");
	    value = Py_BuildValue("s", ptr_server->nick_modes);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
            
	    key = Py_BuildValue("s", "away_time");
	    value = Py_BuildValue("s", timebuffer);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "lag");
	    value = Py_BuildValue("i", ptr_server->lag);
	    PyDict_SetItem(server_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", ptr_server->name);
	    PyDict_SetItem(server_hash, key, server_hash_member);
	    Py_DECREF (key);
	    Py_DECREF (server_hash_member);
	}
    }    
    
    python_plugin->free_server_info(python_plugin, server_info);
    
    return server_hash;
}
*/

/*
 * weechat_python_api_get_channel_info: get infos about channels
 */

/*
static PyObject *
weechat_python_api_get_channel_info (PyObject *self, PyObject *args)
{
    t_plugin_channel_info *channel_info, *ptr_channel;
    PyObject *channel_hash, *channel_hash_member, *key, *value;
    char *server;
    
    // make C compiler happy
    (void) self;
        
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_print");
        PYTHON_RETURN_EMPTY;
    }
    
    server = NULL;
    if (!PyArg_ParseTuple (args, "s", &server))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infobar_print");
        PYTHON_RETURN_EMPTY;
    }
    
    channel_hash = PyDict_New ();
    if (!channel_hash)
    {
        PYTHON_RETURN_EMPTY;
    }

    channel_info = python_plugin->get_channel_info (python_plugin, server);
    if  (!channel_info)
	return channel_hash;

    for(ptr_channel = channel_info; ptr_channel; ptr_channel = ptr_channel->next_channel)
    {
	channel_hash_member = PyDict_New();
	
	if (channel_hash_member)
	{	    
	    key = Py_BuildValue("s", "type");
	    value = Py_BuildValue("i", ptr_channel->type);
	    PyDict_SetItem(channel_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "topic");
	    value = Py_BuildValue("s", ptr_channel->topic);
	    PyDict_SetItem(channel_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "modes");
	    value = Py_BuildValue("s", ptr_channel->modes);
	    PyDict_SetItem(channel_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "limit");
	    value = Py_BuildValue("i", ptr_channel->limit);
	    PyDict_SetItem(channel_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "key");
	    value = Py_BuildValue("s", ptr_channel->key);
	    PyDict_SetItem(channel_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "nicks_count");
	    value = Py_BuildValue("i", ptr_channel->nicks_count);
	    PyDict_SetItem(channel_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", ptr_channel->name);
	    PyDict_SetItem(channel_hash, key, channel_hash_member);
	    Py_DECREF (key);
	    Py_DECREF (channel_hash_member);
	}
    }

    python_plugin->free_channel_info(python_plugin, channel_info);
    
    return channel_hash;
}
*/

/*
 * weechat_python_api_get_nick_info: get infos about nicks
 */

/*
static PyObject *
weechat_python_api_get_nick_info (PyObject *self, PyObject *args)
{
    t_plugin_nick_info *nick_info, *ptr_nick;
    PyObject *nick_hash, *nick_hash_member, *key, *value;
    char *server, *channel;
    
    // make C compiler happy
    (void) self;
        
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_print");
        PYTHON_RETURN_EMPTY;
    }
    
    server = NULL;
    channel = NULL;
    if (!PyArg_ParseTuple (args, "ss", &server, &channel))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infobar_print");
        PYTHON_RETURN_EMPTY;
    }
    
    nick_hash = PyDict_New ();
    if (!nick_hash)
    {
        PYTHON_RETURN_EMPTY;
    }

    nick_info = python_plugin->get_nick_info (python_plugin, server, channel);
    if  (!nick_info)
	return nick_hash;

    for(ptr_nick = nick_info; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
	nick_hash_member = PyDict_New();
	
	if (nick_hash_member)
	{
	    key = Py_BuildValue("s", "flags");
	    value = Py_BuildValue("i", ptr_nick->flags);
	    PyDict_SetItem(nick_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "host");
	    value = Py_BuildValue("s", ptr_nick->host ? ptr_nick->host : "");
	    PyDict_SetItem(nick_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", ptr_nick->nick);
	    PyDict_SetItem(nick_hash, key, nick_hash_member);
	    Py_DECREF (key);
	    Py_DECREF (nick_hash_member);
	}
    }    

    python_plugin->free_nick_info(python_plugin, nick_info);
    
    return nick_hash;
}
*/

/*
 * weechat_python_api_get_irc_color: get the numeric value which identify an
 *                                   irc color by its name
 */

/*
static PyObject *
weechat_python_api_get_irc_color (PyObject *self, PyObject *args)
{
    char *color;
    
    // make C compiler happy
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_print");
        return Py_BuildValue ("i", -1);
    }
    
    color = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &color))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infobar_print");
        return Py_BuildValue ("i", -1);
    }
    
    if (color)
	return Py_BuildValue ("i", python_plugin->get_irc_color (python_plugin, color));
    
    return Py_BuildValue ("i", -1);
}
*/

/*
 * weechat_python_api_get_window_info: get infos about windows
 */

/*
static PyObject *
weechat_python_api_get_window_info (PyObject *self, PyObject *args)
{
    t_plugin_window_info *window_info, *ptr_win;
    PyObject *window_list, *window_list_member, *key, *value;
    
    // make C compiler happy
    (void) self;
    (void) args;
        
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_print");
        PYTHON_RETURN_EMPTY;
    }
        
    window_list = PyList_New (0);
    if (!window_list)
    {
        PYTHON_RETURN_EMPTY;
    }

    window_info = python_plugin->get_window_info (python_plugin);
    if (!window_info)
	return window_list;

    for (ptr_win = window_info; ptr_win; ptr_win = ptr_win->next_window)
    {
	window_list_member = PyDict_New();
	
	if (window_list_member)
	{
	    key = Py_BuildValue("s", "num_buffer");
	    value = Py_BuildValue("i", ptr_win->num_buffer);
	    PyDict_SetItem(window_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "win_x");
	    value = Py_BuildValue("i", ptr_win->win_x);
	    PyDict_SetItem(window_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "win_y");
	    value = Py_BuildValue("i", ptr_win->win_y);
	    PyDict_SetItem(window_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "win_width");
	    value = Py_BuildValue("i", ptr_win->win_width);
	    PyDict_SetItem(window_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "win_height");
	    value = Py_BuildValue("i", ptr_win->win_height);
	    PyDict_SetItem(window_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "win_width_pct");
	    value = Py_BuildValue("i", ptr_win->win_width_pct);
	    PyDict_SetItem(window_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "win_height_pct");
	    value = Py_BuildValue("i", ptr_win->win_height_pct);
	    PyDict_SetItem(window_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    PyList_Append(window_list, window_list_member);
	    Py_DECREF (window_list_member);
	}
    }
    
    python_plugin->free_window_info(python_plugin, window_info);
    
    return window_list;
}
*/

/*
 * weechat_python_api_get_buffer_info: get infos about buffers
 */

/*
static PyObject *
weechat_python_api_get_buffer_info (PyObject *self, PyObject *args)
{
    t_plugin_buffer_info *buffer_info, *ptr_buffer;
    PyObject *buffer_hash, *buffer_hash_member, *key, *value;
    
    // make C compiler happy
    (void) self;
    (void) args;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_print");
        PYTHON_RETURN_EMPTY;
    }
    
    buffer_hash = PyDict_New ();
    if (!buffer_hash)
    {
        PYTHON_RETURN_EMPTY;
    }

    buffer_info = python_plugin->get_buffer_info (python_plugin);
    if  (!buffer_info)
	return buffer_hash;
    
    for(ptr_buffer = buffer_info; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
	buffer_hash_member = PyDict_New();
	
	if (buffer_hash_member)
	{

	    key = Py_BuildValue("s", "type");
	    value = Py_BuildValue("i", ptr_buffer->type);
	    PyDict_SetItem(buffer_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "num_displayed");
	    value = Py_BuildValue("i", ptr_buffer->num_displayed);
	    PyDict_SetItem(buffer_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "server");
	    value = Py_BuildValue("s", ptr_buffer->server_name == NULL ? "" : ptr_buffer->server_name);
	    PyDict_SetItem(buffer_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "channel");
	    value = Py_BuildValue("s", ptr_buffer->channel_name == NULL ? "" : ptr_buffer->channel_name);
	    PyDict_SetItem(buffer_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "notify_level");
	    value = Py_BuildValue("i", ptr_buffer->notify_level);
	    PyDict_SetItem(buffer_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "log_filename");
	    value = Py_BuildValue("s", ptr_buffer->log_filename == NULL ? "" : ptr_buffer->log_filename);
	    PyDict_SetItem(buffer_hash_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("i", ptr_buffer->number);
	    PyDict_SetItem(buffer_hash, key, buffer_hash_member);
	    Py_DECREF (key);
	    Py_DECREF (buffer_hash_member);
	}
    }    
    python_plugin->free_buffer_info(python_plugin, buffer_info);
    
    return buffer_hash;
}
*/

/*
 * weechat_python_api_get_buffer_data: get buffer content
 */

/*
static PyObject *
weechat_python_api_get_buffer_data (PyObject *self, PyObject *args)
{
    t_plugin_buffer_line *buffer_data, *ptr_data;
    PyObject *data_list, *data_list_member, *key, *value;
    char *server, *channel;
    char timebuffer[64];
    
    // make C compiler happy
    (void) self;
    (void) args;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_print");
        PYTHON_RETURN_EMPTY;
    }
    
    server = NULL;
    channel = NULL;
    
    if (!PyArg_ParseTuple (args, "ss|", &server, &channel))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infobar_print");
        PYTHON_RETURN_EMPTY;
    }

    data_list = PyList_New (0);    
    if (!data_list)
    {
        PYTHON_RETURN_EMPTY;
    }
    
    buffer_data = python_plugin->get_buffer_data (python_plugin, server, channel);
    if (!buffer_data)
	return data_list;
    
    for(ptr_data = buffer_data; ptr_data; ptr_data = ptr_data->next_line)
    {
	data_list_member= PyDict_New();
	
        if (data_list_member) 
        {
	    strftime(timebuffer, sizeof(timebuffer), "%F %T",
		     localtime(&ptr_data->date));
	    	    
	    key = Py_BuildValue("s", "date");
	    value = Py_BuildValue("s", timebuffer);
	    PyDict_SetItem(data_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);

	    key = Py_BuildValue("s", "nick");
	    value = Py_BuildValue("s", ptr_data->nick == NULL ? "" : ptr_data->nick);
	    PyDict_SetItem(data_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
	    key = Py_BuildValue("s", "data");
	    value = Py_BuildValue("s", ptr_data->data == NULL ? "" : ptr_data->data);
	    PyDict_SetItem(data_list_member, key, value);
	    Py_DECREF (key);
	    Py_DECREF (value);
	    
            PyList_Append(data_list, data_list_member);
	    Py_DECREF (data_list_member);
        }
    }
    
    python_plugin->free_buffer_data (python_plugin, buffer_data);
    
    return data_list;
}
*/

/*
 * Python subroutines
 */

PyMethodDef weechat_python_funcs[] =
{
    { "register", &weechat_python_api_register, METH_VARARGS, "" },
    { "charset_set", &weechat_python_api_charset_set, METH_VARARGS, "" },
    { "iconv_to_internal", &weechat_python_api_iconv_to_internal, METH_VARARGS, "" },
    { "iconv_from_internal", &weechat_python_api_iconv_from_internal, METH_VARARGS, "" },
    { "mkdir_home", &weechat_python_api_mkdir_home, METH_VARARGS, "" },
    { "mkdir", &weechat_python_api_mkdir, METH_VARARGS, "" },
    { "prefix", &weechat_python_api_prefix, METH_VARARGS, "" },
    { "color", &weechat_python_api_color, METH_VARARGS, "" },
    { "prnt", &weechat_python_api_prnt, METH_VARARGS, "" },
    { "infobar_print", &weechat_python_api_infobar_print, METH_VARARGS, "" },
    { "infobar_remove", &weechat_python_api_infobar_remove, METH_VARARGS, "" },
    { "log_print", &weechat_python_api_log_print, METH_VARARGS, "" },
    { "hook_command", &weechat_python_api_hook_command, METH_VARARGS, "" },
    { "hook_timer", &weechat_python_api_hook_timer, METH_VARARGS, "" },
    { "hook_fd", &weechat_python_api_hook_fd, METH_VARARGS, "" },
    { "hook_print", &weechat_python_api_hook_print, METH_VARARGS, "" },
    { "hook_signal", &weechat_python_api_hook_signal, METH_VARARGS, "" },
    { "hook_signal_send", &weechat_python_api_hook_signal_send, METH_VARARGS, "" },
    { "hook_config", &weechat_python_api_hook_config, METH_VARARGS, "" },
    { "hook_completion", &weechat_python_api_hook_completion, METH_VARARGS, "" },
    { "unhook", &weechat_python_api_unhook, METH_VARARGS, "" },
    { "unhook_all", &weechat_python_api_unhook_all, METH_VARARGS, "" },
    { "buffer_new", &weechat_python_api_buffer_new, METH_VARARGS, "" },
    { "buffer_search", &weechat_python_api_buffer_search, METH_VARARGS, "" },
    { "buffer_close", &weechat_python_api_buffer_close, METH_VARARGS, "" },
    { "buffer_get", &weechat_python_api_buffer_get, METH_VARARGS, "" },
    { "buffer_set", &weechat_python_api_buffer_set, METH_VARARGS, "" },
    { "nicklist_add_group", &weechat_python_api_nicklist_add_group, METH_VARARGS, "" },
    { "nicklist_search_group", &weechat_python_api_nicklist_search_group, METH_VARARGS, "" },
    { "nicklist_add_nick", &weechat_python_api_nicklist_add_nick, METH_VARARGS, "" },
    { "nicklist_search_nick", &weechat_python_api_nicklist_search_nick, METH_VARARGS, "" },
    { "nicklist_remove_group", &weechat_python_api_nicklist_remove_group, METH_VARARGS, "" },
    { "nicklist_remove_nick", &weechat_python_api_nicklist_remove_nick, METH_VARARGS, "" },
    { "nicklist_remove_all", &weechat_python_api_nicklist_remove_all, METH_VARARGS, "" },
    { "command", &weechat_python_api_command, METH_VARARGS, "" },
    { "info_get", &weechat_python_api_info_get, METH_VARARGS, "" },
    /*
    { "get_dcc_info", weechat_python_get_dcc_info, METH_VARARGS, "" },
    { "get_config", weechat_python_get_config, METH_VARARGS, "" },
    { "set_config", weechat_python_set_config, METH_VARARGS, "" },
    { "get_plugin_config", weechat_python_get_plugin_config, METH_VARARGS, "" },
    { "set_plugin_config", weechat_python_set_plugin_config, METH_VARARGS, "" },
    { "get_server_info", weechat_python_get_server_info, METH_VARARGS, "" },
    { "get_channel_info", weechat_python_get_channel_info, METH_VARARGS, "" },
    { "get_nick_info", weechat_python_get_nick_info, METH_VARARGS, "" },
    { "get_irc_color", weechat_python_get_irc_color, METH_VARARGS, "" },
    { "get_window_info", weechat_python_get_window_info, METH_VARARGS, "" },
    { "get_buffer_info", weechat_python_get_buffer_info, METH_VARARGS, "" },
    { "get_buffer_data", weechat_python_get_buffer_data, METH_VARARGS, "" },
    */
    { NULL, NULL, 0, NULL }
};
