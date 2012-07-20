/*
 * Copyright (C) 2008-2010 Dmitry Kobylin <fnfal@academ.tsc.ru>
 * Copyright (C) 2008 Julien Louis <ptitlouis@sysif.net>
 * Copyright (C) 2008-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * weechat-tcl-api.c: tcl API functions
 */

#undef _

#include <tcl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../weechat-plugin.h"
#include "../script.h"
#include "../script-api.h"
#include "../script-callback.h"
#include "weechat-tcl.h"


#define API_FUNC(__init, __name, __ret)                                 \
    char *tcl_function_name = __name;                                   \
    (void) clientData;                                                  \
    if (__init                                                          \
        && (!tcl_current_script || !tcl_current_script->name))          \
    {                                                                   \
        WEECHAT_SCRIPT_MSG_NOT_INIT(TCL_CURRENT_SCRIPT_NAME,            \
                                    tcl_function_name);                 \
        __ret;                                                          \
    }
#define API_WRONG_ARGS(__ret)                                           \
    {                                                                   \
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(TCL_CURRENT_SCRIPT_NAME,          \
                                      tcl_function_name);               \
        __ret;                                                          \
    }
#define API_STR2PTR(__string)                                           \
    script_str2ptr (weechat_tcl_plugin, TCL_CURRENT_SCRIPT_NAME,        \
                    tcl_function_name, __string)
#define API_RETURN_OK                                                   \
    {                                                                   \
        objp = Tcl_GetObjResult (interp);                               \
        if (Tcl_IsShared (objp))                                        \
        {                                                               \
            objp = Tcl_DuplicateObj (objp);                             \
            Tcl_IncrRefCount (objp);                                    \
            Tcl_SetIntObj (objp, 1);                                    \
            Tcl_SetObjResult (interp, objp);                            \
            Tcl_DecrRefCount (objp);                                    \
        }                                                               \
        else                                                            \
            Tcl_SetIntObj (objp, 1);                                    \
        return TCL_OK;                                                  \
    }
#define API_RETURN_ERROR                                                \
    {                                                                   \
        objp = Tcl_GetObjResult (interp);                               \
        if (Tcl_IsShared (objp))                                        \
        {                                                               \
            objp = Tcl_DuplicateObj (objp);                             \
            Tcl_IncrRefCount (objp);                                    \
            Tcl_SetIntObj (objp, 0);                                    \
            Tcl_SetObjResult (interp, objp);                            \
            Tcl_DecrRefCount (objp);                                    \
        }                                                               \
        else                                                            \
            Tcl_SetIntObj (objp, 0);                                    \
        return TCL_ERROR;                                               \
    }
#define API_RETURN_EMPTY                                                \
    {                                                                   \
        objp = Tcl_GetObjResult (interp);                               \
        if (Tcl_IsShared (objp))                                        \
        {                                                               \
            objp = Tcl_DuplicateObj (objp);                             \
            Tcl_IncrRefCount (objp);                                    \
            Tcl_SetStringObj (objp, "", -1);                            \
            Tcl_SetObjResult (interp, objp);                            \
            Tcl_DecrRefCount (objp);                                    \
        }                                                               \
        else                                                            \
            Tcl_SetStringObj (objp, "", -1);                            \
        return TCL_OK;                                                  \
    }
#define API_RETURN_STRING(__string)                                     \
    {                                                                   \
        objp = Tcl_GetObjResult (interp);                               \
        if (Tcl_IsShared (objp))                                        \
        {                                                               \
            objp = Tcl_DuplicateObj (objp);                             \
            Tcl_IncrRefCount (objp);                                    \
            if (__string)                                               \
            {                                                           \
                Tcl_SetStringObj (objp, __string, -1);                  \
                Tcl_SetObjResult (interp, objp);                        \
                Tcl_DecrRefCount (objp);                                \
                return TCL_OK;                                          \
            }                                                           \
            Tcl_SetStringObj (objp, "", -1);                            \
            Tcl_SetObjResult (interp, objp);                            \
            Tcl_DecrRefCount (objp);                                    \
        }                                                               \
        else                                                            \
        {                                                               \
            if (__string)                                               \
            {                                                           \
                Tcl_SetStringObj (objp, __string, -1);                  \
                return TCL_OK;                                          \
            }                                                           \
            Tcl_SetStringObj (objp, "", -1);                            \
        }                                                               \
        return TCL_OK;                                                  \
    }
#define API_RETURN_STRING_FREE(__string)                                \
    {                                                                   \
        objp = Tcl_GetObjResult (interp);                               \
        if (Tcl_IsShared (objp))                                        \
        {                                                               \
            objp = Tcl_DuplicateObj (objp);                             \
            Tcl_IncrRefCount (objp);                                    \
            if (__string)                                               \
            {                                                           \
                Tcl_SetStringObj (objp, __string, -1);                  \
                Tcl_SetObjResult (interp, objp);                        \
                Tcl_DecrRefCount (objp);                                \
                free (__string);                                        \
                return TCL_OK;                                          \
            }                                                           \
            Tcl_SetStringObj (objp, "", -1);                            \
            Tcl_SetObjResult (interp, objp);                            \
            Tcl_DecrRefCount (objp);                                    \
        }                                                               \
        else                                                            \
        {                                                               \
            if (__string)                                               \
            {                                                           \
                Tcl_SetStringObj (objp, __string, -1);                  \
                free (__string);                                        \
                return TCL_OK;                                          \
            }                                                           \
            Tcl_SetStringObj (objp, "", -1);                            \
        }                                                               \
        return TCL_OK;                                                  \
    }
#define API_RETURN_INT(__int)                                           \
    {                                                                   \
        objp = Tcl_GetObjResult (interp);                               \
        if (Tcl_IsShared (objp))                                        \
        {                                                               \
            objp = Tcl_DuplicateObj (objp);                             \
            Tcl_IncrRefCount (objp);                                    \
            Tcl_SetIntObj (objp, __int);                                \
            Tcl_SetObjResult (interp, objp);                            \
            Tcl_DecrRefCount (objp);                                    \
        }                                                               \
        else                                                            \
            Tcl_SetIntObj (objp, __int);                                \
        return TCL_OK;                                                  \
    }
#define API_RETURN_LONG(__long)                                         \
    {                                                                   \
        objp = Tcl_GetObjResult (interp);                               \
        if (Tcl_IsShared (objp))                                        \
        {                                                               \
            objp = Tcl_DuplicateObj (objp);                             \
            Tcl_IncrRefCount (objp);                                    \
            Tcl_SetLongObj (objp, __long);                              \
            Tcl_SetObjResult (interp, objp);                            \
            Tcl_DecrRefCount (objp);                                    \
        }                                                               \
        else                                                            \
            Tcl_SetLongObj (objp, __long);                              \
        return TCL_OK;                                                  \
    }
#define API_RETURN_OBJ(__obj)                                           \
    {                                                                   \
        Tcl_SetObjResult (interp, __obj);                               \
        return TCL_OK;                                                  \
    }

#define API_DEF_FUNC(__name)                                            \
    Tcl_CreateObjCommand (interp, "weechat::" #__name,                  \
                          weechat_tcl_api_##__name,                     \
                          (ClientData) NULL,                            \
                          (Tcl_CmdDeleteProc*)NULL);


/*
 * weechat_tcl_api_register: startup function for all WeeChat Tcl scripts
 */

static int
weechat_tcl_api_register (ClientData clientData, Tcl_Interp *interp, int objc,
                          Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *name, *author, *version, *license, *description, *shutdown_func;
    char *charset;
    int i;

    API_FUNC(0, "register", API_RETURN_ERROR);
    if (tcl_registered_script)
    {
        /* script already registered */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" already "
                                         "registered (register ignored)"),
                        weechat_prefix ("error"), TCL_PLUGIN_NAME,
                        tcl_registered_script->name);
        API_RETURN_ERROR;
    }
    tcl_current_script = NULL;
    tcl_registered_script = NULL;

    if (objc < 8)
        API_WRONG_ARGS(API_RETURN_ERROR);

    name = Tcl_GetStringFromObj (objv[1], &i);
    author = Tcl_GetStringFromObj (objv[2], &i);
    version = Tcl_GetStringFromObj (objv[3], &i);
    license = Tcl_GetStringFromObj (objv[4], &i);
    description = Tcl_GetStringFromObj (objv[5], &i);
    shutdown_func = Tcl_GetStringFromObj (objv[6], &i);
    charset = Tcl_GetStringFromObj (objv[7], &i);

    if (script_search (weechat_tcl_plugin, tcl_scripts, name))
    {
        /* another script already exists with same name */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to register script "
                                         "\"%s\" (another script already "
                                         "exists with this name)"),
                        weechat_prefix ("error"), TCL_PLUGIN_NAME, name);
        API_RETURN_ERROR;
    }

    /* register script */
    tcl_current_script = script_add (weechat_tcl_plugin,
                                     &tcl_scripts, &last_tcl_script,
                                     (tcl_current_script_filename) ?
                                     tcl_current_script_filename : "",
                                     name, author, version, license,
                                     description, shutdown_func, charset);
    if (tcl_current_script)
    {
        tcl_registered_script = tcl_current_script;
        if ((weechat_tcl_plugin->debug >= 2) || !tcl_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: registered script \"%s\", "
                                             "version %s (%s)"),
                            TCL_PLUGIN_NAME, name, version, description);
        }
        tcl_current_script->interpreter = (void *)interp;
    }
    else
    {
        API_RETURN_ERROR;
    }

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_plugin_get_name: get name of plugin (return "core" for
 *                                  WeeChat core)
 */

static int
weechat_tcl_api_plugin_get_name (ClientData clientData, Tcl_Interp *interp,
                                 int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *plugin;
    const char *result;
    int i;

    API_FUNC(1, "plugin_get_name", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    plugin = Tcl_GetStringFromObj (objv[1], &i);

    result = weechat_plugin_get_name (API_STR2PTR(plugin));

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_charset_set: set script charset
 */

static int
weechat_tcl_api_charset_set (ClientData clientData, Tcl_Interp *interp,
                             int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i;

    API_FUNC(1, "charset_set", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    script_api_charset_set (tcl_current_script,
                            Tcl_GetStringFromObj (objv[1], &i)); /* charset */

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_iconv_to_internal: convert string to internal WeeChat
 *                                    charset
 */

static int
weechat_tcl_api_iconv_to_internal (ClientData clientData, Tcl_Interp *interp,
                                   int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *charset, *string;
    int i;

    API_FUNC(1, "iconv_to_internal", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    charset = Tcl_GetStringFromObj (objv[1], &i);
    string = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_iconv_to_internal (charset, string);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_iconv_from_internal: convert string from WeeChat inernal
 *                                      charset to another one
 */

static int
weechat_tcl_api_iconv_from_internal (ClientData clientData, Tcl_Interp *interp,
                                     int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *charset, *string;
    int i;

    API_FUNC(1, "iconv_from_internal", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    charset = Tcl_GetStringFromObj (objv[1], &i);
    string = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_iconv_from_internal (charset, string);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_gettext: get translated string
 */

static int
weechat_tcl_api_gettext (ClientData clientData, Tcl_Interp *interp,
                         int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_FUNC(1, "gettext", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_gettext (Tcl_GetStringFromObj (objv[1], &i)); /* string */

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_ngettext: get translated string with plural form
 */

static int
weechat_tcl_api_ngettext (ClientData clientData, Tcl_Interp *interp,
                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *single, *plural;
    const char *result;
    int i, count;

    API_FUNC(1, "ngettext", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    single = Tcl_GetStringFromObj (objv[1], &i);
    plural = Tcl_GetStringFromObj (objv[2], &i);

    if (Tcl_GetIntFromObj (interp, objv[3], &count) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_ngettext (single, plural, count);

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_string_match: return 1 if string matches a mask
 *                               mask can begin or end with "*", no other "*"
 *                               are allowed inside mask
 */

static int
weechat_tcl_api_string_match (ClientData clientData, Tcl_Interp *interp,
                              int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *string, *mask;
    int case_sensitive, result, i;

    API_FUNC(1, "string_match", API_RETURN_INT(0));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = Tcl_GetStringFromObj (objv[1], &i);
    mask = Tcl_GetStringFromObj (objv[2], &i);

    if (Tcl_GetIntFromObj (interp, objv[3], &case_sensitive) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_string_match (string, mask, case_sensitive);

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_string_has_highlight: return 1 if string contains a
 *                                       highlight (using list of words to
 *                                       highlight)
 *                                       return 0 if no highlight is found in
 *                                       string
 */

static int
weechat_tcl_api_string_has_highlight (ClientData clientData,
                                      Tcl_Interp *interp,
                                      int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *string, *highlight_words;
    int result, i;

    API_FUNC(1, "string_has_highlight", API_RETURN_INT(0));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = Tcl_GetStringFromObj (objv[1], &i);
    highlight_words = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_string_has_highlight (string, highlight_words);

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_string_has_highlight_regex: return 1 if string contains a
 *                                             highlight (using a regular
 *                                             expression)
 *                                             return 0 if no highlight is
 *                                             found in string
 */

static int
weechat_tcl_api_string_has_highlight_regex (ClientData clientData,
                                            Tcl_Interp *interp,
                                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *string, *regex;
    int result, i;

    API_FUNC(1, "string_has_highlight_regex", API_RETURN_INT(0));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = Tcl_GetStringFromObj (objv[1], &i);
    regex = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_string_has_highlight_regex (string, regex);

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_string_mask_to_regex: convert a mask (string with only
 *                                       "*" as wildcard) to a regex, paying
 *                                       attention to special chars in a
 *                                       regex
 */

static int
weechat_tcl_api_string_mask_to_regex (ClientData clientData,
                                      Tcl_Interp *interp,
                                      int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *mask;
    int i;

    API_FUNC(1, "string_mask_to_regex", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    mask = Tcl_GetStringFromObj (objv[1], &i);

    result = weechat_string_mask_to_regex (mask);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_string_remove_color: remove WeeChat color codes from string
 */

static int
weechat_tcl_api_string_remove_color (ClientData clientData, Tcl_Interp *interp,
                                     int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *replacement, *string;
    int i;

    API_FUNC(1, "string_remove_color", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    string = Tcl_GetStringFromObj (objv[1], &i);
    replacement = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_string_remove_color (string, replacement);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_string_is_command_char: check if first char of string is a
 *                                         command char
 */

static int
weechat_tcl_api_string_is_command_char (ClientData clientData, Tcl_Interp *interp,
                                        int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int result, i;

    API_FUNC(1, "string_is_command_char", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_string_is_command_char (Tcl_GetStringFromObj (objv[1], &i)); /* string */

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_string_input_for_buffer: return string with input text
 *                                          for buffer or empty string if
 *                                          it's a command
 */

static int
weechat_tcl_api_string_input_for_buffer (ClientData clientData, Tcl_Interp *interp,
                                         int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_FUNC(1, "string_input_for_buffer", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_string_input_for_buffer (Tcl_GetStringFromObj (objv[1], &i));

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_mkdir_home: create a directory in WeeChat home
 */

static int
weechat_tcl_api_mkdir_home (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i, mode;

    API_FUNC(1, "mkdir_home", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (Tcl_GetIntFromObj (interp, objv[2], &mode) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if (weechat_mkdir_home (Tcl_GetStringFromObj (objv[1], &i), /* directory */
                            mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

/*
 * weechat_tcl_api_mkdir: create a directory
 */

static int
weechat_tcl_api_mkdir (ClientData clientData, Tcl_Interp *interp,
                       int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i, mode;

    API_FUNC(1, "mkdir", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (Tcl_GetIntFromObj (interp, objv[2], &mode) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if (weechat_mkdir (Tcl_GetStringFromObj (objv[1], &i), /* directory */
                       mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

/*
 * weechat_tcl_api_mkdir_parents: create a directory and make parent
 *                                directories as needed
 */

static int
weechat_tcl_api_mkdir_parents (ClientData clientData, Tcl_Interp *interp,
                               int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i, mode;

    API_FUNC(1, "mkdir_parents", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (Tcl_GetIntFromObj (interp, objv[2], &mode) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if (weechat_mkdir_parents (Tcl_GetStringFromObj (objv[1], &i), /* directory */
                               mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

/*
 * weechat_tcl_api_list_new: create a new list
 */

static int
weechat_tcl_api_list_new (ClientData clientData, Tcl_Interp *interp,
                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;

    /* make C compiler happy */
    (void) clientData;
    (void) objc;
    (void) objv;

    API_FUNC(1, "list_new", API_RETURN_EMPTY);

    result = script_ptr2str (weechat_list_new ());

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_list_add: add a string to list
 */

static int
weechat_tcl_api_list_add (ClientData clientData, Tcl_Interp *interp,
                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *weelist, *data, *where, *user_data;
    int i;


    API_FUNC(1, "list_add", API_RETURN_EMPTY);
    if (objc < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    weelist = Tcl_GetStringFromObj (objv[1], &i);
    data = Tcl_GetStringFromObj (objv[2], &i);
    where = Tcl_GetStringFromObj (objv[3], &i);
    user_data = Tcl_GetStringFromObj (objv[4], &i);

    result = script_ptr2str (weechat_list_add (API_STR2PTR(weelist),
                                               data,
                                               where,
                                               API_STR2PTR(user_data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_list_search: search a string in list
 */

static int
weechat_tcl_api_list_search (ClientData clientData, Tcl_Interp *interp,
                             int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *weelist, *data;
    int i;

    API_FUNC(1, "list_search", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    weelist = Tcl_GetStringFromObj (objv[1], &i);
    data = Tcl_GetStringFromObj (objv[2], &i);

    result = script_ptr2str (weechat_list_search (API_STR2PTR(weelist),
                                                  data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_list_search_pos: search position of a string in list
 */

static int
weechat_tcl_api_list_search_pos (ClientData clientData, Tcl_Interp *interp,
                             int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *weelist, *data;
    int i, pos;

    API_FUNC(1, "list_search_pos", API_RETURN_INT(-1));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    weelist = Tcl_GetStringFromObj (objv[1], &i);
    data = Tcl_GetStringFromObj (objv[2], &i);

    pos = weechat_list_search_pos (API_STR2PTR(weelist), data);

    API_RETURN_INT(pos);
}

/*
 * weechat_tcl_api_list_casesearch: search a string in list (ignore case)
 */

static int
weechat_tcl_api_list_casesearch (ClientData clientData, Tcl_Interp *interp,
                                 int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *weelist, *data;
    int i;

    API_FUNC(1, "list_casesearch", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    weelist = Tcl_GetStringFromObj (objv[1], &i);
    data = Tcl_GetStringFromObj (objv[2], &i);

    result = script_ptr2str (weechat_list_casesearch (API_STR2PTR(weelist),
                                                      data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_list_casesearch_pos: search position of a string in list
 *                                      (ignore case)
 */

static int
weechat_tcl_api_list_casesearch_pos (ClientData clientData, Tcl_Interp *interp,
                                     int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *weelist, *data;
    int i, pos;

    API_FUNC(1, "list_casesearch_pos", API_RETURN_INT(-1));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    weelist = Tcl_GetStringFromObj (objv[1], &i);
    data = Tcl_GetStringFromObj (objv[2], &i);

    pos = weechat_list_casesearch_pos (API_STR2PTR(weelist), data);

    API_RETURN_INT(pos);
}

/*
 * weechat_tcl_api_list_get: get item by position
 */

static int
weechat_tcl_api_list_get (ClientData clientData, Tcl_Interp *interp,
                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;
    int i, position;

    API_FUNC(1, "list_get", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if (Tcl_GetIntFromObj (interp, objv[2], &position) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = script_ptr2str (weechat_list_get (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)), /* weelist */
                                               position));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_list_set: set new value for item
 */

static int
weechat_tcl_api_list_set (ClientData clientData, Tcl_Interp *interp,
                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *item, *new_value;
    int i;

    API_FUNC(1, "list_set", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    item = Tcl_GetStringFromObj (objv[1], &i);
    new_value = Tcl_GetStringFromObj (objv[2], &i);

    weechat_list_set (API_STR2PTR(item), new_value);

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_list_next: get next item
 */

static int
weechat_tcl_api_list_next (ClientData clientData, Tcl_Interp *interp,
                           int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;
    int i;

    API_FUNC(1, "list_next", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = script_ptr2str (weechat_list_next (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)))); /* item */

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_list_prev: get previous item
 */

static int
weechat_tcl_api_list_prev (ClientData clientData, Tcl_Interp *interp,
                           int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;
    int i;

    API_FUNC(1, "list_prev", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = script_ptr2str (weechat_list_prev (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)))); /* item */

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_list_string: get string value of item
 */

static int
weechat_tcl_api_list_string (ClientData clientData, Tcl_Interp *interp,
                             int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_FUNC(1, "list_string", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_list_string (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* item */

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_list_size: get number of elements in list
 */

static int
weechat_tcl_api_list_size (ClientData clientData, Tcl_Interp *interp,
                           int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int size;
    int i;

    API_FUNC(1, "list_size", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    size = weechat_list_size (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* weelist */

    API_RETURN_INT(size);
}

/*
 * weechat_tcl_api_list_remove: remove item from list
 */

static int
weechat_tcl_api_list_remove (ClientData clientData, Tcl_Interp *interp,
                             int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *weelist, *item;
    int i;

    API_FUNC(1, "list_remove", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weelist = Tcl_GetStringFromObj (objv[1], &i);
    item = Tcl_GetStringFromObj (objv[2], &i);

    weechat_list_remove (API_STR2PTR(weelist),
                         API_STR2PTR(item));

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_list_remove_all: remove all items from list
 */

static int
weechat_tcl_api_list_remove_all (ClientData clientData, Tcl_Interp *interp,
                                 int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i;

    (void) clientData;

    API_FUNC(1, "list_remove_all", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_list_remove_all (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* weelist */

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_list_free: free list
 */

static int
weechat_tcl_api_list_free (ClientData clientData, Tcl_Interp *interp,
                           int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i;

    API_FUNC(1, "list_free", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_list_free (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* weelist */

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_config_reload_cb: callback for config reload
 */

int
weechat_tcl_api_config_reload_cb (void *data,
                                  struct t_config_file *config_file)
{
    struct t_script_callback *script_callback;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_script_callback *)data;


    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = script_ptr2str (config_file);

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "ss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);

        return ret;
    }

    return WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
}

/*
 * weechat_tcl_api_config_new: create a new configuration file
 */

static int
weechat_tcl_api_config_new (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *name, *function, *data;
    int i;

    API_FUNC(1, "config_new", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    data = Tcl_GetStringFromObj (objv[3], &i);

    result = script_ptr2str (script_api_config_new (weechat_tcl_plugin,
                                                    tcl_current_script,
                                                    name,
                                                    &weechat_tcl_api_config_reload_cb,
                                                    function,
                                                    data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_config_section_read_cb: callback for reading option in
 *                                         section
 */

int
weechat_tcl_api_config_section_read_cb (void *data,
                                        struct t_config_file *config_file,
                                        struct t_config_section *section,
                                        const char *option_name, const char *value)
{
    struct t_script_callback *script_callback;
    void *func_argv[5];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = script_ptr2str (config_file);
        func_argv[2] = script_ptr2str (section);
        func_argv[3] = (option_name) ? (char *)option_name : empty_arg;
        func_argv[4] = (value) ? (char *)value : empty_arg;

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "sssss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_SET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);
        if (func_argv[2])
            free (func_argv[2]);

        return ret;
    }

    return WEECHAT_CONFIG_OPTION_SET_ERROR;
}

/*
 * weechat_tcl_api_config_section_write_cb: callback for writing section
 */

int
weechat_tcl_api_config_section_write_cb (void *data,
                                          struct t_config_file *config_file,
                                          const char *section_name)
{
    struct t_script_callback *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = script_ptr2str (config_file);
        func_argv[2] = (section_name) ? (char *)section_name : empty_arg;

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "sss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_WRITE_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);

        return ret;
    }

    return WEECHAT_CONFIG_WRITE_ERROR;
}

/*
 * weechat_tcl_api_config_section_write_default_cb: callback for writing
 *                                                   default values for section
 */

int
weechat_tcl_api_config_section_write_default_cb (void *data,
                                                  struct t_config_file *config_file,
                                                  const char *section_name)
{
    struct t_script_callback *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = script_ptr2str (config_file);
        func_argv[2] = (section_name) ? (char *)section_name : empty_arg;

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "sss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_WRITE_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);

        return ret;
    }

    return WEECHAT_CONFIG_WRITE_ERROR;
}

/*
 * weechat_tcl_api_config_section_create_option_cb: callback to create an
 *                                                  option
 */

int
weechat_tcl_api_config_section_create_option_cb (void *data,
                                                  struct t_config_file *config_file,
                                                  struct t_config_section *section,
                                                  const char *option_name,
                                                  const char *value)
{
    struct t_script_callback *script_callback;
    void *func_argv[5];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = script_ptr2str (config_file);
        func_argv[2] = script_ptr2str (section);
        func_argv[3] = (option_name) ? (char *)option_name : empty_arg;
        func_argv[4] = (value) ? (char *)value : empty_arg;

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "sssss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_SET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);
        if (func_argv[2])
            free (func_argv[2]);

        return ret;
    }

    return WEECHAT_CONFIG_OPTION_SET_ERROR;
}

/*
 * weechat_tcl_api_config_section_delete_option_cb: callback to delete an
 *                                                  option
 */

int
weechat_tcl_api_config_section_delete_option_cb (void *data,
                                                 struct t_config_file *config_file,
                                                 struct t_config_section *section,
                                                 struct t_config_option *option)
{
    struct t_script_callback *script_callback;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = script_ptr2str (config_file);
        func_argv[2] = script_ptr2str (section);
        func_argv[3] = script_ptr2str (option);

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "ssss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_UNSET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);
        if (func_argv[2])
            free (func_argv[2]);
        if (func_argv[3])
            free (func_argv[3]);

        return ret;
    }

    return WEECHAT_CONFIG_OPTION_UNSET_ERROR;
}

/*
 * weechat_tcl_api_config_new_section: create a new section in configuration
 *                                     file
 */

static int
weechat_tcl_api_config_new_section (ClientData clientData, Tcl_Interp *interp,
                                    int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *cfg_file, *name, *function_read, *data_read;
    char *function_write, *data_write, *function_write_default;
    char *data_write_default, *function_create_option, *data_create_option;
    char *function_delete_option, *data_delete_option;
    int i, can_add, can_delete;

    /* make C compiler happy */
    (void) clientData;

    API_FUNC(1, "config_new_section", API_RETURN_EMPTY);
    if (objc < 15)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if ((Tcl_GetIntFromObj (interp, objv[3], &can_add) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[4], &can_delete) != TCL_OK))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    cfg_file = Tcl_GetStringFromObj (objv[1], &i);
    name = Tcl_GetStringFromObj (objv[2], &i);
    function_read = Tcl_GetStringFromObj (objv[5], &i);
    data_read = Tcl_GetStringFromObj (objv[6], &i);
    function_write = Tcl_GetStringFromObj (objv[7], &i);
    data_write = Tcl_GetStringFromObj (objv[8], &i);
    function_write_default = Tcl_GetStringFromObj (objv[9], &i);
    data_write_default = Tcl_GetStringFromObj (objv[10], &i);
    function_create_option = Tcl_GetStringFromObj (objv[11], &i);
    data_create_option = Tcl_GetStringFromObj (objv[12], &i);
    function_delete_option = Tcl_GetStringFromObj (objv[13], &i);
    data_delete_option = Tcl_GetStringFromObj (objv[14], &i);

    result = script_ptr2str (script_api_config_new_section (weechat_tcl_plugin,
                                                            tcl_current_script,
                                                            API_STR2PTR(cfg_file),
                                                            name,
                                                            can_add, /* user_can_add_options */
                                                            can_delete, /* user_can_delete_options */
                                                            &weechat_tcl_api_config_section_read_cb,
                                                            function_read,
                                                            data_read,
                                                            &weechat_tcl_api_config_section_write_cb,
                                                            function_write,
                                                            data_write,
                                                            &weechat_tcl_api_config_section_write_default_cb,
                                                            function_write_default,
                                                            data_write_default,
                                                            &weechat_tcl_api_config_section_create_option_cb,
                                                            function_create_option,
                                                            data_create_option,
                                                            &weechat_tcl_api_config_section_delete_option_cb,
                                                            function_delete_option,
                                                            data_delete_option));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_config_search_section: search section in configuration file
 */

static int
weechat_tcl_api_config_search_section (ClientData clientData, Tcl_Interp *interp,
                                       int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *config_file, *section_name;
    int i;

    API_FUNC(1, "config_search_section", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = Tcl_GetStringFromObj (objv[1], &i);
    section_name = Tcl_GetStringFromObj (objv[2], &i);

    result = script_ptr2str (weechat_config_search_section (API_STR2PTR(config_file),
                                                            section_name));

    API_RETURN_STRING_FREE(result);
}


/*
 * weechat_tcl_api_config_option_check_value_cb: callback for checking new
 *                                               value for option
 */

int
weechat_tcl_api_config_option_check_value_cb (void *data,
                                               struct t_config_option *option,
                                               const char *value)
{
    struct t_script_callback *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = script_ptr2str (option);
        func_argv[2] = (value) ? (char *)value : empty_arg;

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "sss", func_argv);

        if (!rc)
            ret = 0;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);

        return ret;
    }

    return 0;
}

/*
 * weechat_tcl_api_config_option_change_cb: callback for option changed
 */

void
weechat_tcl_api_config_option_change_cb (void *data,
                                          struct t_config_option *option)
{
    struct t_script_callback *script_callback;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    int *rc;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = script_ptr2str (option);

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "ss", func_argv);

        if (func_argv[1])
            free (func_argv[1]);

        if (rc)
            free (rc);
    }
}

/*
 * weechat_tcl_api_config_option_delete_cb: callback when option is deleted
 */

void
weechat_tcl_api_config_option_delete_cb (void *data,
                                          struct t_config_option *option)
{
    struct t_script_callback *script_callback;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    int *rc;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = script_ptr2str (option);

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "ss", func_argv);

        if (func_argv[1])
            free (func_argv[1]);

        if (rc)
            free (rc);
    }
}

/*
 * weechat_tcl_api_config_new_option: create a new option in section
 */

static int
weechat_tcl_api_config_new_option (ClientData clientData, Tcl_Interp *interp,
                                   int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *config_file, *section, *name, *type;
    char *description, *string_values, *default_value, *value;
    char *function_check_value, *data_check_value, *function_change;
    char *data_change, *function_delete, *data_delete;
    int i, min, max, null_value_allowed;

    API_FUNC(1, "config_new_option", API_RETURN_EMPTY);
    if (objc < 18)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if ((Tcl_GetIntFromObj (interp, objv[7], &min) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[8], &max) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[11], &null_value_allowed) != TCL_OK))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = Tcl_GetStringFromObj (objv[1], &i);
    section = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);
    type = Tcl_GetStringFromObj (objv[4], &i);
    description = Tcl_GetStringFromObj (objv[5], &i);
    string_values = Tcl_GetStringFromObj (objv[6], &i);
    default_value = Tcl_GetStringFromObj (objv[9], &i);
    value = Tcl_GetStringFromObj (objv[10], &i);
    function_check_value = Tcl_GetStringFromObj (objv[12], &i);
    data_check_value = Tcl_GetStringFromObj (objv[13], &i);
    function_change = Tcl_GetStringFromObj (objv[14], &i);
    data_change = Tcl_GetStringFromObj (objv[15], &i);
    function_delete = Tcl_GetStringFromObj (objv[16], &i);
    data_delete = Tcl_GetStringFromObj (objv[17], &i);

    result = script_ptr2str (script_api_config_new_option (weechat_tcl_plugin,
                                                           tcl_current_script,
                                                           API_STR2PTR(config_file),
                                                           API_STR2PTR(section),
                                                           name,
                                                           type,
                                                           description,
                                                           string_values,
                                                           min,
                                                           max,
                                                           default_value,
                                                           value,
                                                           null_value_allowed,
                                                           &weechat_tcl_api_config_option_check_value_cb,
                                                           function_check_value,
                                                           data_check_value,
                                                           &weechat_tcl_api_config_option_change_cb,
                                                           function_change,
                                                           data_change,
                                                           &weechat_tcl_api_config_option_delete_cb,
                                                           function_delete,
                                                           data_delete));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_config_search_option: search option in configuration file or
 *                                       section
 */

static int
weechat_tcl_api_config_search_option (ClientData clientData, Tcl_Interp *interp,
                                      int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *config_file, *section, *option_name;
    int i;

    API_FUNC(1, "config_search_option", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = Tcl_GetStringFromObj (objv[1], &i);
    section = Tcl_GetStringFromObj (objv[2], &i);
    option_name = Tcl_GetStringFromObj (objv[3], &i);

    result = script_ptr2str (weechat_config_search_option (API_STR2PTR(config_file),
                                                           API_STR2PTR(section),
                                                           option_name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_config_string_to_boolean: return boolean value of a string
 */

static int
weechat_tcl_api_config_string_to_boolean (ClientData clientData, Tcl_Interp *interp,
                                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int result, i;

    API_FUNC(1, "config_string_to_boolean", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_config_string_to_boolean (Tcl_GetStringFromObj (objv[1], &i)); /* text */

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_config_option_reset: reset an option with default value
 */

static int
weechat_tcl_api_config_option_reset (ClientData clientData, Tcl_Interp *interp,
                                     int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int rc;
    char *option;
    int i, run_callback;

    API_FUNC(1, "config_option_reset", API_RETURN_INT(0));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    if (Tcl_GetIntFromObj (interp, objv[2], &run_callback) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = Tcl_GetStringFromObj (objv[1], &i);

    rc = weechat_config_option_reset (API_STR2PTR(option),
                                      run_callback);

    API_RETURN_INT(rc);
}

/*
 * weechat_tcl_api_config_option_set: set new value for option
 */

static int
weechat_tcl_api_config_option_set (ClientData clientData, Tcl_Interp *interp,
                                   int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int rc;
    char *option, *new_value;
    int i, run_callback;

    API_FUNC(1, "config_option_set", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    if (Tcl_GetIntFromObj (interp, objv[3], &run_callback) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    option = Tcl_GetStringFromObj (objv[1], &i);
    new_value = Tcl_GetStringFromObj (objv[2], &i);

    rc = weechat_config_option_set (API_STR2PTR(option),
                                    new_value,
                                    run_callback);

    API_RETURN_INT(rc);
}

/*
 * weechat_tcl_api_config_option_set_null: set null (undefined)value for option
 */

static int
weechat_tcl_api_config_option_set_null (ClientData clientData, Tcl_Interp *interp,
                                        int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int rc;
    char *option;
    int i, run_callback;

    API_FUNC(1, "config_option_set_null", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    if (Tcl_GetIntFromObj (interp, objv[2], &run_callback) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    option = Tcl_GetStringFromObj (objv[1], &i);

    rc = weechat_config_option_set_null (API_STR2PTR(option),
                                         run_callback);

    API_RETURN_INT(rc);
}

/*
 * weechat_tcl_api_config_option_unset: unset an option
 */

static int
weechat_tcl_api_config_option_unset (ClientData clientData, Tcl_Interp *interp,
                                     int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int rc;
    char *option;
    int i;

    API_FUNC(1, "config_option_unset", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    option = Tcl_GetStringFromObj (objv[1], &i);

    rc = weechat_config_option_unset (API_STR2PTR(option));

    API_RETURN_INT(rc);
}

/*
 * weechat_tcl_api_config_option_rename: rename an option
 */

static int
weechat_tcl_api_config_option_rename (ClientData clientData, Tcl_Interp *interp,
                                      int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *option, *new_name;
    int i;

    API_FUNC(1, "config_option_rename", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    option = Tcl_GetStringFromObj (objv[1], &i);
    new_name = Tcl_GetStringFromObj (objv[2], &i);

    weechat_config_option_rename (API_STR2PTR(option),
                                  new_name);

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_config_option_is_null: return 1 if value of option is null
 */

static int
weechat_tcl_api_config_option_is_null (ClientData clientData, Tcl_Interp *interp,
                                       int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int result, i;

    API_FUNC(1, "config_option_is_null", API_RETURN_INT(1));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(1));

    result = weechat_config_option_is_null (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_config_option_default_is_null: return 1 if default value of
 *                                                option is null
 */

static int
weechat_tcl_api_config_option_default_is_null (ClientData clientData,
                                               Tcl_Interp *interp,
                                               int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int result, i;

    API_FUNC(1, "config_option_default_is_null", API_RETURN_INT(1));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(1));

    result = weechat_config_option_default_is_null (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_config_boolean: return boolean value of option
 */

static int
weechat_tcl_api_config_boolean (ClientData clientData, Tcl_Interp *interp,
                                int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int result, i;

    API_FUNC(1, "config_boolean", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_config_boolean (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_config_boolean_default: return default boolean value of option
 */

static int
weechat_tcl_api_config_boolean_default (ClientData clientData, Tcl_Interp *interp,
                                        int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int result, i;

    API_FUNC(1, "config_boolean_default", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_config_boolean_default (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_config_integer: return integer value of option
 */

static int
weechat_tcl_api_config_integer (ClientData clientData, Tcl_Interp *interp,
                                int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int result, i;

    API_FUNC(1, "config_integer", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_config_integer (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_config_integer_default: return default integer value of option
 */

static int
weechat_tcl_api_config_integer_default (ClientData clientData, Tcl_Interp *interp,
                                        int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int result, i;

    API_FUNC(1, "config_integer_default", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_config_integer_default (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_config_string: return string value of option
 */

static int
weechat_tcl_api_config_string (ClientData clientData, Tcl_Interp *interp,
                               int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_FUNC(1, "config_string", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_config_string (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_config_string_default: return default string value of option
 */

static int
weechat_tcl_api_config_string_default (ClientData clientData, Tcl_Interp *interp,
                                       int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_FUNC(1, "config_string_default", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_config_string_default (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_config_color: return color value of option
 */

static int
weechat_tcl_api_config_color (ClientData clientData, Tcl_Interp *interp,
                              int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_FUNC(1, "config_color", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_config_color (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_config_color_default: return default color value of option
 */

static int
weechat_tcl_api_config_color_default (ClientData clientData, Tcl_Interp *interp,
                                      int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_FUNC(1, "config_color_default", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_config_color_default (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_config_write_option: write an option in configuration file
 */

static int
weechat_tcl_api_config_write_option (ClientData clientData, Tcl_Interp *interp,
                                     int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *config_file, *option;
    int i;

    API_FUNC(1, "config_write_option", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    config_file = Tcl_GetStringFromObj (objv[1], &i);
    option = Tcl_GetStringFromObj (objv[2], &i);

    weechat_config_write_option (API_STR2PTR(config_file),
                                 API_STR2PTR(option));

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_config_write_line: write a line in configuration file
 */

static int
weechat_tcl_api_config_write_line (ClientData clientData, Tcl_Interp *interp,
                                   int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *config_file, *option_name, *value;
    int i;

    API_FUNC(1, "config_write_line", API_RETURN_ERROR);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_ERROR);

    config_file = Tcl_GetStringFromObj (objv[1], &i);
    option_name = Tcl_GetStringFromObj (objv[2], &i);
    value = Tcl_GetStringFromObj (objv[3], &i);

    weechat_config_write_line (API_STR2PTR(config_file), option_name,
                               "%s", value);

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_config_write: write configuration file
 */

static int
weechat_tcl_api_config_write (ClientData clientData, Tcl_Interp *interp,
                              int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int rc;
    int i;

    API_FUNC(1, "config_write", API_RETURN_INT(-1));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    rc = weechat_config_write (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* config_file */

    API_RETURN_INT(rc);
}

/*
 * weechat_tcl_api_config_read: read configuration file
 */

static int
weechat_tcl_api_config_read (ClientData clientData, Tcl_Interp *interp,
                             int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int rc;
    int i;

    API_FUNC(1, "config_read", API_RETURN_INT(-1));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    rc = weechat_config_read (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* config_file */

    API_RETURN_INT(rc);
}

/*
 * weechat_tcl_api_config_reload: reload configuration file
 */

static int
weechat_tcl_api_config_reload (ClientData clientData, Tcl_Interp *interp,
                               int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int rc;
    int i;

    API_FUNC(1, "config_reload", API_RETURN_INT(-1));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    rc = weechat_config_reload (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* config_file */

    API_RETURN_INT(rc);
}

/*
 * weechat_tcl_api_config_option_free: free an option in configuration file
 */

static int
weechat_tcl_api_config_option_free (ClientData clientData, Tcl_Interp *interp,
                                    int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i;

    API_FUNC(1, "config_option_free", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    script_api_config_option_free (weechat_tcl_plugin,
                                   tcl_current_script,
                                   API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_config_section_free_options: free all options of a section
 *                                              in configuration file
 */

static int
weechat_tcl_api_config_section_free_options (ClientData clientData, Tcl_Interp *interp,
                                             int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i;

    API_FUNC(1, "config_section_free_options", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    script_api_config_section_free_options (weechat_tcl_plugin,
                                            tcl_current_script,
                                            API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* section */

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_config_section_free: free section in configuration file
 */

static int
weechat_tcl_api_config_section_free (ClientData clientData, Tcl_Interp *interp,
                                     int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i;

    API_FUNC(1, "config_section_free", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    script_api_config_section_free (weechat_tcl_plugin,
                                    tcl_current_script,
                                    API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* section */

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_config_free: free configuration file
 */

static int
weechat_tcl_api_config_free (ClientData clientData, Tcl_Interp *interp,
                             int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i;

    API_FUNC(1, "config_free", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    script_api_config_free (weechat_tcl_plugin,
                            tcl_current_script,
                            API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* config_file */

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_config_get: get config option
 */

static int
weechat_tcl_api_config_get (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;
    int i;

    API_FUNC(1, "config_get", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = script_ptr2str (weechat_config_get (Tcl_GetStringFromObj (objv[1], &i)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_config_get_plugin: get value of a plugin option
 */

static int
weechat_tcl_api_config_get_plugin (ClientData clientData, Tcl_Interp *interp,
                                   int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_FUNC(1, "config_get_plugin", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = script_api_config_get_plugin (weechat_tcl_plugin,
                                           tcl_current_script,
                                           Tcl_GetStringFromObj (objv[1], &i));

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_config_is_set_plugin: check if a plugin option is set
 */

static int
weechat_tcl_api_config_is_set_plugin (ClientData clientData, Tcl_Interp *interp,
                                      int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *option;
    int i, rc;

    API_FUNC(1, "config_is_set_plugin", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = Tcl_GetStringFromObj (objv[1], &i);

    rc = script_api_config_is_set_plugin (weechat_tcl_plugin,
                                          tcl_current_script,
                                          option);

    API_RETURN_INT(rc);
}

/*
 * weechat_tcl_api_config_set_plugin: set value of a plugin option
 */

static int
weechat_tcl_api_config_set_plugin (ClientData clientData, Tcl_Interp *interp,
                                   int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *option, *value;
    int i, rc;

    API_FUNC(1, "config_set_plugin", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    option = Tcl_GetStringFromObj (objv[1], &i);
    value = Tcl_GetStringFromObj (objv[2], &i);

    rc = script_api_config_set_plugin (weechat_tcl_plugin,
                                       tcl_current_script,
                                       option,
                                       value);

    API_RETURN_INT(rc);
}

/*
 * weechat_tcl_api_config_set_desc_plugin: set description of a plugin option
 */

static int
weechat_tcl_api_config_set_desc_plugin (ClientData clientData, Tcl_Interp *interp,
                                        int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *option, *description;
    int i;

    API_FUNC(1, "config_set_desc_plugin", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    option = Tcl_GetStringFromObj (objv[1], &i);
    description = Tcl_GetStringFromObj (objv[2], &i);

    script_api_config_set_desc_plugin (weechat_tcl_plugin,
                                       tcl_current_script,
                                       option,
                                       description);

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_config_set_plugin: unset plugin option
 */

static int
weechat_tcl_api_config_unset_plugin (ClientData clientData, Tcl_Interp *interp,
                                     int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *option;
    int i, rc;

    API_FUNC(1, "config_unset_plugin", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    option = Tcl_GetStringFromObj (objv[1], &i);

    rc = script_api_config_unset_plugin (weechat_tcl_plugin,
                                         tcl_current_script,
                                         option);

    API_RETURN_INT(rc);
}

/*
 * weechat_tcl_api_key_bind: bind key(s)
 */

static int
weechat_tcl_api_key_bind (ClientData clientData, Tcl_Interp *interp,
                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *context;
    struct t_hashtable *hashtable;
    int i, num_keys;

    API_FUNC(1, "key_bind", API_RETURN_INT(0));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    context = Tcl_GetStringFromObj (objv[1], &i);
    hashtable = weechat_tcl_dict_to_hashtable (interp, objv[2],
                                               WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE);

    num_keys = weechat_key_bind (context, hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);

    API_RETURN_INT(num_keys);
}

/*
 * weechat_tcl_api_key_unbind: unbind key(s)
 */

static int
weechat_tcl_api_key_unbind (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *context, *key;
    int i, num_keys;

    API_FUNC(1, "key_unbind", API_RETURN_INT(0));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    context = Tcl_GetStringFromObj (objv[1], &i);
    key = Tcl_GetStringFromObj (objv[2], &i);

    num_keys = weechat_key_unbind (context, key);

    API_RETURN_INT(num_keys);
}

/*
 * weechat_tcl_api_prefix: get a prefix, used for display
 */

static int
weechat_tcl_api_prefix (ClientData clientData, Tcl_Interp *interp,
                        int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_FUNC(0, "prefix", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_prefix (Tcl_GetStringFromObj (objv[1], &i)); /* prefix */

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_color: get a color code, used for display
 */

static int
weechat_tcl_api_color (ClientData clientData, Tcl_Interp *interp,
                       int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_FUNC(0, "color", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_color (Tcl_GetStringFromObj (objv[1], &i)); /* color */

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_print: print message in a buffer
 */

static int
weechat_tcl_api_print (ClientData clientData, Tcl_Interp *interp,
                       int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *message;
    int i;

    API_FUNC(0, "print", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    message = Tcl_GetStringFromObj (objv[2], &i);

    script_api_printf (weechat_tcl_plugin,
                       tcl_current_script,
                       API_STR2PTR(buffer),
                       "%s", message);

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_print_date_tags: print message in a buffer with optional
 *                                  date and tags
 */

static int
weechat_tcl_api_print_date_tags (ClientData clientData, Tcl_Interp *interp,
                                 int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *tags, *message;
    int i, tdate;

    API_FUNC(1, "print_date_tags", API_RETURN_ERROR);
    if (objc < 5)
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (Tcl_GetIntFromObj (interp, objv[2], &tdate) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    tags = Tcl_GetStringFromObj (objv[3], &i);
    message = Tcl_GetStringFromObj (objv[4], &i);

    script_api_printf_date_tags (weechat_tcl_plugin,
                                 tcl_current_script,
                                 API_STR2PTR(buffer),
                                 tdate,
                                 tags,
                                 "%s", message);

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_print_y: print message in a buffer with free content
 */

static int
weechat_tcl_api_print_y (ClientData clientData, Tcl_Interp *interp,
                         int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *message;
    int i, y;

    API_FUNC(1, "print_y", API_RETURN_ERROR);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (Tcl_GetIntFromObj (interp, objv[2], &y) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    message = Tcl_GetStringFromObj (objv[3], &i);

    script_api_printf_y (weechat_tcl_plugin,
                         tcl_current_script,
                         API_STR2PTR(buffer),
                         y,
                         "%s", message);

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_log_print: print message in WeeChat log file
 */

static int
weechat_tcl_api_log_print (ClientData clientData, Tcl_Interp *interp,
                           int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i;

    /* make C compiler happy */
    (void) clientData;

    API_FUNC(1, "log_print", API_RETURN_ERROR);

    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    script_api_log_printf (weechat_tcl_plugin,
                           tcl_current_script,
                           "%s", Tcl_GetStringFromObj (objv[1], &i)); /* message */

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_hook_command_cb: callback for command hooked
 */

int
weechat_tcl_api_hook_command_cb (void *data, struct t_gui_buffer *buffer,
                                 int argc, char **argv, char **argv_eol)
{
    struct t_script_callback *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    /* make C compiler happy */
    (void) argv;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = script_ptr2str (buffer);
        func_argv[2] = (argc > 1) ? argv_eol[1] : empty_arg;

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "sss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_tcl_api_hook_command: hook a command
 */

static int
weechat_tcl_api_hook_command (ClientData clientData, Tcl_Interp *interp,
                              int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *command, *description, *args, *args_description;
    char *completion, *function, *data;
    int i;

    API_FUNC(1, "hook_command", API_RETURN_EMPTY);
    if (objc < 8)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = Tcl_GetStringFromObj (objv[1], &i);
    description = Tcl_GetStringFromObj (objv[2], &i);
    args = Tcl_GetStringFromObj (objv[3], &i);
    args_description = Tcl_GetStringFromObj (objv[4], &i);
    completion = Tcl_GetStringFromObj (objv[5], &i);
    function = Tcl_GetStringFromObj (objv[6], &i);
    data = Tcl_GetStringFromObj (objv[7], &i);

    result = script_ptr2str (script_api_hook_command (weechat_tcl_plugin,
                                                      tcl_current_script,
                                                      command,
                                                      description,
                                                      args,
                                                      args_description,
                                                      completion,
                                                      &weechat_tcl_api_hook_command_cb,
                                                      function,
                                                      data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_command_run_cb: callback for command_run hooked
 */

int
weechat_tcl_api_hook_command_run_cb (void *data, struct t_gui_buffer *buffer,
                                     const char *command)
{
    struct t_script_callback *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = script_ptr2str (buffer);
        func_argv[2] = (command) ? (char *)command : empty_arg;

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "sss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_tcl_api_hook_command_run: hook a command_run
 */

static int
weechat_tcl_api_hook_command_run (ClientData clientData, Tcl_Interp *interp,
                                  int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *command, *function, *data;
    int i;

    API_FUNC(1, "hook_command_run", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    data = Tcl_GetStringFromObj (objv[3], &i);

    result = script_ptr2str (script_api_hook_command_run (weechat_tcl_plugin,
                                                          tcl_current_script,
                                                          command,
                                                          &weechat_tcl_api_hook_command_run_cb,
                                                          function,
                                                          data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_timer_cb: callback for timer hooked
 */

int
weechat_tcl_api_hook_timer_cb (void *data, int remaining_calls)
{
    struct t_script_callback *script_callback;
    void *func_argv[2];
    char str_remaining_calls[32], empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_remaining_calls, sizeof (str_remaining_calls),
                  "%d", remaining_calls);

        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = str_remaining_calls;

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "ss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_tcl_api_hook_timer: hook a timer
 */

static int
weechat_tcl_api_hook_timer (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;
    int i, interval, align_second, max_calls;

    API_FUNC(1, "hook_timer", API_RETURN_EMPTY);
    if (objc < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if ((Tcl_GetIntFromObj (interp, objv[1], &interval) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[2], &align_second) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[3], &max_calls) != TCL_OK))
        API_WRONG_ARGS(API_RETURN_EMPTY);


    result = script_ptr2str (script_api_hook_timer (weechat_tcl_plugin,
                                                    tcl_current_script,
                                                    interval, /* interval */
                                                    align_second, /* align_second */
                                                    max_calls, /* max_calls */
                                                    &weechat_tcl_api_hook_timer_cb,
                                                    Tcl_GetStringFromObj (objv[4], &i), /* tcl function */
                                                    Tcl_GetStringFromObj (objv[5], &i))); /* data */

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_fd_cb: callback for fd hooked
 */

int
weechat_tcl_api_hook_fd_cb (void *data, int fd)
{
    struct t_script_callback *script_callback;
    void *func_argv[2];
    char str_fd[32], empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_fd, sizeof (str_fd), "%d", fd);

        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = str_fd;

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "ss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_tcl_api_hook_fd: hook a fd
 */

static int
weechat_tcl_api_hook_fd (ClientData clientData, Tcl_Interp *interp,
                         int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;
    int i, fd, read, write, exception;

    API_FUNC(1, "hook_fd", API_RETURN_EMPTY);
    if (objc < 7)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if ((Tcl_GetIntFromObj (interp, objv[1], &fd) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[2], &read) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[3], &write) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[4], &exception) != TCL_OK))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = script_ptr2str (script_api_hook_fd (weechat_tcl_plugin,
                                                 tcl_current_script,
                                                 fd, /* fd */
                                                 read, /* read */
                                                 write, /* write */
                                                 exception, /* exception */
                                                 &weechat_tcl_api_hook_fd_cb,
                                                 Tcl_GetStringFromObj (objv[5], &i), /* tcl function */
                                                 Tcl_GetStringFromObj (objv[6], &i))); /* data */

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_process_cb: callback for process hooked
 */

int
weechat_tcl_api_hook_process_cb (void *data,
                                 const char *command, int return_code,
                                 const char *out, const char *err)
{
    struct t_script_callback *script_callback;
    void *func_argv[5];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = (command) ? (char *)command : empty_arg;
        func_argv[2] = &return_code;
        func_argv[3] = (out) ? (char *)out : empty_arg;
        func_argv[4] = (err) ? (char *)err : empty_arg;

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "ssiss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_tcl_api_hook_process: hook a process
 */

static int
weechat_tcl_api_hook_process (ClientData clientData, Tcl_Interp *interp,
                              int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *command, *function, *data, *result;
    int i, timeout;

    API_FUNC(1, "hook_process", API_RETURN_EMPTY);
    if (objc < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if ((Tcl_GetIntFromObj (interp, objv[2], &timeout) != TCL_OK))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[3], &i);
    data = Tcl_GetStringFromObj (objv[4], &i);

    result = script_ptr2str (script_api_hook_process (weechat_tcl_plugin,
                                                      tcl_current_script,
                                                      command,
                                                      timeout,
                                                      &weechat_tcl_api_hook_process_cb,
                                                      function,
                                                      data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_process_hashtable: hook a process with options in
 *                                         a hashtable
 */

static int
weechat_tcl_api_hook_process_hashtable (ClientData clientData,
                                        Tcl_Interp *interp,
                                        int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *command, *function, *data, *result;
    struct t_hashtable *options;
    int i, timeout;

    API_FUNC(1, "hook_process_hashtable", API_RETURN_EMPTY);
    if (objc < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if ((Tcl_GetIntFromObj (interp, objv[3], &timeout) != TCL_OK))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = Tcl_GetStringFromObj (objv[1], &i);
    options = weechat_tcl_dict_to_hashtable (interp, objv[2],
                                             WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE);
    function = Tcl_GetStringFromObj (objv[4], &i);
    data = Tcl_GetStringFromObj (objv[5], &i);

    result = script_ptr2str (script_api_hook_process_hashtable (weechat_tcl_plugin,
                                                                tcl_current_script,
                                                                command,
                                                                options,
                                                                timeout,
                                                                &weechat_tcl_api_hook_process_cb,
                                                                function,
                                                                data));

    if (options)
        weechat_hashtable_free (options);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_connect_cb: callback for connect hooked
 */

int
weechat_tcl_api_hook_connect_cb (void *data, int status, int gnutls_rc,
                                 const char *error, const char *ip_address)
{
    struct t_script_callback *script_callback;
    void *func_argv[5];
    char str_status[32], str_gnutls_rc[32];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_status, sizeof (str_status), "%d", status);
        snprintf (str_gnutls_rc, sizeof (str_gnutls_rc), "%d", gnutls_rc);

        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = str_status;
        func_argv[2] = str_gnutls_rc;
        func_argv[3] = (ip_address) ? (char *)ip_address : empty_arg;
        func_argv[4] = (error) ? (char *)error : empty_arg;

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "sssss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_tcl_api_hook_connect: hook a connection
 */

static int
weechat_tcl_api_hook_connect (ClientData clientData, Tcl_Interp *interp,
                              int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *proxy, *address, *local_hostname, *function, *data, *result;
    int i, port, sock, ipv6;

    API_FUNC(1, "hook_connect", API_RETURN_EMPTY);
    if (objc < 9)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if ((Tcl_GetIntFromObj (interp, objv[3], &port) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[4], &sock) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[5], &ipv6) != TCL_OK))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    proxy = Tcl_GetStringFromObj (objv[1], &i);
    address = Tcl_GetStringFromObj (objv[2], &i);
    local_hostname = Tcl_GetStringFromObj (objv[6], &i);
    function = Tcl_GetStringFromObj (objv[7], &i);
    data = Tcl_GetStringFromObj (objv[8], &i);

    result = script_ptr2str (script_api_hook_connect (weechat_tcl_plugin,
                                                      tcl_current_script,
                                                      proxy,
                                                      address,
                                                      port,
                                                      sock,
                                                      ipv6,
                                                      NULL, /* gnutls session */
                                                      NULL, /* gnutls callback */
                                                      0,    /* gnutls DH key size */
                                                      NULL, /* gnutls priorities */
                                                      local_hostname,
                                                      &weechat_tcl_api_hook_connect_cb,
                                                      function,
                                                      data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_print_cb: callback for print hooked
 */

int
weechat_tcl_api_hook_print_cb (void *data, struct t_gui_buffer *buffer,
                               time_t date,
                               int tags_count, const char **tags,
                               int displayed, int highlight,
                               const char *prefix, const char *message)
{
    struct t_script_callback *script_callback;
    void *func_argv[8];
    char empty_arg[1] = { '\0' };
    static char timebuffer[64];
    int *rc, ret;

    /* make C compiler happy */
    (void) tags_count;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (timebuffer, sizeof (timebuffer) - 1, "%ld", (long int)date);

        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = script_ptr2str (buffer);
        func_argv[2] = timebuffer;
        func_argv[3] = weechat_string_build_with_split_string (tags, ",");
        if (!func_argv[3])
            func_argv[3] = strdup ("");
        func_argv[4] = (displayed) ? strdup ("1") : strdup ("0");
        func_argv[5] = (highlight) ? strdup ("1") : strdup ("0");
        func_argv[6] = (prefix) ? (char *)prefix : empty_arg;
        func_argv[7] = (message) ? (char *)message : empty_arg;

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "ssssssss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);
        if (func_argv[3])
            free (func_argv[3]);
        if (func_argv[4])
            free (func_argv[4]);
        if (func_argv[5])
            free (func_argv[5]);

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_tcl_api_hook_print: hook a print
 */

static int
weechat_tcl_api_hook_print (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *buffer, *tags, *message, *function, *data;
    int i, strip_colors;

    API_FUNC(1, "hook_print", API_RETURN_EMPTY);
    if (objc < 7)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if (Tcl_GetIntFromObj (interp, objv[4], &strip_colors) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    tags = Tcl_GetStringFromObj (objv[2], &i);
    message = Tcl_GetStringFromObj (objv[3], &i);
    function = Tcl_GetStringFromObj (objv[5], &i);
    data = Tcl_GetStringFromObj (objv[6], &i);

    result = script_ptr2str (script_api_hook_print (weechat_tcl_plugin,
                                                    tcl_current_script,
                                                    API_STR2PTR(buffer),
                                                    tags,
                                                    message,
                                                    strip_colors, /* strip_colors */
                                                    &weechat_tcl_api_hook_print_cb,
                                                    function,
                                                    data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_signal_cb: callback for signal hooked
 */

int
weechat_tcl_api_hook_signal_cb (void *data, const char *signal, const char *type_data,
                                 void *signal_data)
{
    struct t_script_callback *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    static char value_str[64];
    int *rc, ret, free_needed;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = (signal) ? (char *)signal : empty_arg;
        free_needed = 0;
        if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
        {
            func_argv[2] = (signal_data) ? (char *)signal_data : empty_arg;
        }
        else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
        {
            snprintf (value_str, sizeof (value_str) - 1,
                      "%d", *((int *)signal_data));
            func_argv[2] = value_str;
        }
        else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
        {
            func_argv[2] = script_ptr2str (signal_data);
            free_needed = 1;
        }
        else
            func_argv[2] = empty_arg;

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "sss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (free_needed && func_argv[2])
            free (func_argv[2]);

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_tcl_api_hook_signal: hook a signal
 */

static int
weechat_tcl_api_hook_signal (ClientData clientData, Tcl_Interp *interp,
                             int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *signal, *function, *data;
    int i;

    API_FUNC(1, "hook_signal", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    signal = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    data = Tcl_GetStringFromObj (objv[3], &i);

    result = script_ptr2str (script_api_hook_signal (weechat_tcl_plugin,
                                                     tcl_current_script,
                                                     signal,
                                                     &weechat_tcl_api_hook_signal_cb,
                                                     function,
                                                     data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_signal_send: send a signal
 */

static int
weechat_tcl_api_hook_signal_send (ClientData clientData, Tcl_Interp *interp,
                                  int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *signal, *type_data;
    int number;
    int i;

    API_FUNC(1, "hook_signal_send", API_RETURN_ERROR);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_ERROR);

    signal = Tcl_GetStringFromObj (objv[1], &i);
    type_data = Tcl_GetStringFromObj (objv[2], &i);
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        weechat_hook_signal_send (signal,
                                  type_data,
                                  Tcl_GetStringFromObj (objv[3], &i)); /* signal_data */
        API_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        if (Tcl_GetIntFromObj (interp, objv[3], &number) != TCL_OK)
        {
            API_RETURN_ERROR;
        }
        weechat_hook_signal_send (signal,
                                  type_data,
                                  &number); /* signal_data */
        API_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        weechat_hook_signal_send (signal,
                                  type_data,
                                  API_STR2PTR(Tcl_GetStringFromObj (objv[3], &i))); /* signal_data */
        API_RETURN_OK;
    }

    API_RETURN_ERROR;
}

/*
 * weechat_tcl_api_hook_hsignal_cb: callback for hsignal hooked
 */

int
weechat_tcl_api_hook_hsignal_cb (void *data, const char *signal,
                                 struct t_hashtable *hashtable)
{
    struct t_script_callback *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = (signal) ? (char *)signal : empty_arg;
        func_argv[2] = hashtable;

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "ssh", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_tcl_api_hook_hsignal: hook a hsignal
 */

static int
weechat_tcl_api_hook_hsignal (ClientData clientData, Tcl_Interp *interp,
                              int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *signal, *function, *data;
    int i;

    API_FUNC(1, "hook_hsignal", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    signal = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    data = Tcl_GetStringFromObj (objv[3], &i);

    result = script_ptr2str (script_api_hook_hsignal (weechat_tcl_plugin,
                                                      tcl_current_script,
                                                      signal,
                                                      &weechat_tcl_api_hook_hsignal_cb,
                                                      function,
                                                      data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_hsignal_send: send a hsignal
 */

static int
weechat_tcl_api_hook_hsignal_send (ClientData clientData, Tcl_Interp *interp,
                                   int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *signal;
    struct t_hashtable *hashtable;
    int i;

    API_FUNC(1, "hook_hsignal_send", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    signal = Tcl_GetStringFromObj (objv[1], &i);
    hashtable = weechat_tcl_dict_to_hashtable (interp, objv[2],
                                               WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE);

    weechat_hook_hsignal_send (signal, hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_hook_config_cb: callback for config option hooked
 */

int
weechat_tcl_api_hook_config_cb (void *data, const char *option, const char *value)
{
    struct t_script_callback *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = (option) ? (char *)option : empty_arg;
        func_argv[2] = (value) ? (char *)value : empty_arg;

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "sss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_tcl_api_hook_config: hook a config option
 */

static int
weechat_tcl_api_hook_config (ClientData clientData, Tcl_Interp *interp,
                             int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *option, *function, *data;
    int i;

    API_FUNC(1, "hook_config", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    data = Tcl_GetStringFromObj (objv[3], &i);

    result = script_ptr2str (script_api_hook_config (weechat_tcl_plugin,
                                                     tcl_current_script,
                                                     option,
                                                     &weechat_tcl_api_hook_config_cb,
                                                     function,
                                                     data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_completion_cb: callback for completion hooked
 */

int
weechat_tcl_api_hook_completion_cb (void *data, const char *completion_item,
                                     struct t_gui_buffer *buffer,
                                     struct t_gui_completion *completion)
{
    struct t_script_callback *script_callback;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = (completion_item) ? (char *)completion_item : empty_arg;
        func_argv[2] = script_ptr2str (buffer);
        func_argv[3] = script_ptr2str (completion);

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "ssss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[2])
            free (func_argv[2]);
        if (func_argv[3])
            free (func_argv[3]);

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_tcl_api_hook_completion: hook a completion
 */

static int
weechat_tcl_api_hook_completion (ClientData clientData, Tcl_Interp *interp,
                                 int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *completion, *description, *function, *data;
    int i;

    API_FUNC(1, "hook_completion", API_RETURN_EMPTY);
    if (objc < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    completion = Tcl_GetStringFromObj (objv[1], &i);
    description = Tcl_GetStringFromObj (objv[2], &i);
    function = Tcl_GetStringFromObj (objv[3], &i);
    data = Tcl_GetStringFromObj (objv[4], &i);

    result = script_ptr2str (script_api_hook_completion (weechat_tcl_plugin,
                                                         tcl_current_script,
                                                         completion,
                                                         description,
                                                         &weechat_tcl_api_hook_completion_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_completion_list_add: add a word to list for a completion
 */

static int
weechat_tcl_api_hook_completion_list_add (ClientData clientData, Tcl_Interp *interp,
                                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *completion, *word, *where;
    int i, nick_completion;

    API_FUNC(1, "hook_completion_list_add", API_RETURN_ERROR);
    if (objc < 5)
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (Tcl_GetIntFromObj (interp, objv[3], &nick_completion) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_ERROR);

    completion = Tcl_GetStringFromObj (objv[1], &i);
    word = Tcl_GetStringFromObj (objv[2], &i);
    where = Tcl_GetStringFromObj (objv[4], &i);

    weechat_hook_completion_list_add (API_STR2PTR(completion),
                                      word,
                                      nick_completion, /* nick_completion */
                                      where);

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_hook_modifier_cb: callback for modifier hooked
 */

char *
weechat_tcl_api_hook_modifier_cb (void *data, const char *modifier,
                                   const char *modifier_data, const char *string)
{
    struct t_script_callback *script_callback;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = (modifier) ? (char *)modifier : empty_arg;
        func_argv[2] = (modifier_data) ? (char *)modifier_data : empty_arg;
        func_argv[3] = (string) ? (char *)string : empty_arg;

        return (char *)weechat_tcl_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_STRING,
                                         script_callback->function,
                                         "ssss", func_argv);
    }

    return NULL;
}

/*
 * weechat_tcl_api_hook_modifier: hook a modifier
 */

static int
weechat_tcl_api_hook_modifier (ClientData clientData, Tcl_Interp *interp,
                               int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *modifier, *function, *data;
    int i;

    API_FUNC(1, "hook_modifier", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    modifier = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    data = Tcl_GetStringFromObj (objv[3], &i);

    result = script_ptr2str (script_api_hook_modifier (weechat_tcl_plugin,
                                                       tcl_current_script,
                                                       modifier,
                                                       &weechat_tcl_api_hook_modifier_cb,
                                                       function,
                                                       data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_modifier_exec: execute a modifier hook
 */

static int
weechat_tcl_api_hook_modifier_exec (ClientData clientData, Tcl_Interp *interp,
                                    int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *modifier, *modifier_data, *string;
    int i;

    API_FUNC(1, "hook_modifier_exec", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    modifier = Tcl_GetStringFromObj (objv[1], &i);
    modifier_data = Tcl_GetStringFromObj (objv[2], &i);
    string = Tcl_GetStringFromObj (objv[3], &i);

    result = weechat_hook_modifier_exec (modifier, modifier_data, string);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_info_cb: callback for info hooked
 */

const char *
weechat_tcl_api_hook_info_cb (void *data, const char *info_name,
                               const char *arguments)
{
    struct t_script_callback *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = (info_name) ? (char *)info_name : empty_arg;
        func_argv[2] = (arguments) ? (char *)arguments : empty_arg;

        return (const char *)weechat_tcl_exec (script_callback->script,
                                               WEECHAT_SCRIPT_EXEC_STRING,
                                               script_callback->function,
                                               "sss", func_argv);
    }

    return NULL;
}

/*
 * weechat_tcl_api_hook_info: hook an info
 */

static int
weechat_tcl_api_hook_info (ClientData clientData, Tcl_Interp *interp,
                           int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *info_name, *description, *args_description, *function, *data;
    int i;

    API_FUNC(1, "hook_info", API_RETURN_EMPTY);
    if (objc < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    info_name = Tcl_GetStringFromObj (objv[1], &i);
    description = Tcl_GetStringFromObj (objv[2], &i);
    args_description = Tcl_GetStringFromObj (objv[3], &i);
    function = Tcl_GetStringFromObj (objv[4], &i);
    data = Tcl_GetStringFromObj (objv[5], &i);

    result = script_ptr2str (script_api_hook_info (weechat_tcl_plugin,
                                                   tcl_current_script,
                                                   info_name,
                                                   description,
                                                   args_description,
                                                   &weechat_tcl_api_hook_info_cb,
                                                   function,
                                                   data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_info_hashtable_cb: callback for info_hashtable hooked
 */

struct t_hashtable *
weechat_tcl_api_hook_info_hashtable_cb (void *data, const char *info_name,
                                        struct t_hashtable *hashtable)
{
    struct t_script_callback *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = (info_name) ? (char *)info_name : empty_arg;
        func_argv[2] = hashtable;

        return (struct t_hashtable *)weechat_tcl_exec (script_callback->script,
                                                       WEECHAT_SCRIPT_EXEC_HASHTABLE,
                                                       script_callback->function,
                                                       "ssh", func_argv);
    }

    return NULL;
}

/*
 * weechat_tcl_api_hook_info_hashtable: hook an info_hashtable
 */

static int
weechat_tcl_api_hook_info_hashtable (ClientData clientData, Tcl_Interp *interp,
                                     int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *info_name, *description, *args_description;
    char *output_description, *function, *data;
    int i;

    API_FUNC(1, "hook_info_hashtable", API_RETURN_EMPTY);
    if (objc < 7)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    info_name = Tcl_GetStringFromObj (objv[1], &i);
    description = Tcl_GetStringFromObj (objv[2], &i);
    args_description = Tcl_GetStringFromObj (objv[3], &i);
    output_description = Tcl_GetStringFromObj (objv[4], &i);
    function = Tcl_GetStringFromObj (objv[5], &i);
    data = Tcl_GetStringFromObj (objv[6], &i);

    result = script_ptr2str (script_api_hook_info_hashtable (weechat_tcl_plugin,
                                                             tcl_current_script,
                                                             info_name,
                                                             description,
                                                             args_description,
                                                             output_description,
                                                             &weechat_tcl_api_hook_info_hashtable_cb,
                                                             function,
                                                             data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_infolist_cb: callback for infolist hooked
 */

struct t_infolist *
weechat_tcl_api_hook_infolist_cb (void *data, const char *infolist_name,
                                   void *pointer, const char *arguments)
{
    struct t_script_callback *script_callback;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };
    struct t_infolist *result;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = (infolist_name) ? (char *)infolist_name : empty_arg;
        func_argv[2] = script_ptr2str (pointer);
        func_argv[3] = (arguments) ? (char *)arguments : empty_arg;

        result = (struct t_infolist *)weechat_tcl_exec (script_callback->script,
                                                        WEECHAT_SCRIPT_EXEC_STRING,
                                                        script_callback->function,
                                                        "ssss", func_argv);

        if (func_argv[2])
            free (func_argv[2]);

        return result;
    }

    return NULL;
}

/*
 * weechat_tcl_api_hook_infolist: hook an infolist
 */

static int
weechat_tcl_api_hook_infolist (ClientData clientData, Tcl_Interp *interp,
                               int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *infolist_name, *description, *pointer_description;
    char *args_description, *function, *data;
    int i;

    API_FUNC(1, "hook_infolist", API_RETURN_EMPTY);
    if (objc < 7)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist_name = Tcl_GetStringFromObj (objv[1], &i);
    description = Tcl_GetStringFromObj (objv[2], &i);
    pointer_description = Tcl_GetStringFromObj (objv[3], &i);
    args_description = Tcl_GetStringFromObj (objv[4], &i);
    function = Tcl_GetStringFromObj (objv[5], &i);
    data = Tcl_GetStringFromObj (objv[6], &i);

    result = script_ptr2str (script_api_hook_infolist (weechat_tcl_plugin,
                                                       tcl_current_script,
                                                       infolist_name,
                                                       description,
                                                       pointer_description,
                                                       args_description,
                                                       &weechat_tcl_api_hook_infolist_cb,
                                                       function,
                                                       data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_focus_cb: callback for focus hooked
 */

struct t_hashtable *
weechat_tcl_api_hook_focus_cb (void *data,
                               struct t_hashtable *info)
{
    struct t_script_callback *script_callback;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = info;

        return (struct t_hashtable *)weechat_tcl_exec (script_callback->script,
                                                       WEECHAT_SCRIPT_EXEC_HASHTABLE,
                                                       script_callback->function,
                                                       "sh", func_argv);
    }

    return NULL;
}

/*
 * weechat_tcl_api_hook_focus: hook a focus
 */

static int
weechat_tcl_api_hook_focus (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *area, *function, *data;
    int i;

    API_FUNC(1, "hook_focus", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    area = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    data = Tcl_GetStringFromObj (objv[3], &i);

    result = script_ptr2str (script_api_hook_focus (weechat_tcl_plugin,
                                                    tcl_current_script,
                                                    area,
                                                    &weechat_tcl_api_hook_focus_cb,
                                                    function,
                                                    data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_unhook: unhook something
 */

static int
weechat_tcl_api_unhook (ClientData clientData, Tcl_Interp *interp,
                        int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i;

    API_FUNC(1, "unhook", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    script_api_unhook (weechat_tcl_plugin,
                       tcl_current_script,
                       API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* hook */

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_unhook_all: unhook all for script
 */

static int
weechat_tcl_api_unhook_all (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;

    /* make C compiler happy */
    (void) clientData;
    (void) objc;
    (void) objv;

    API_FUNC(1, "unhook_all", API_RETURN_ERROR);

    script_api_unhook_all (tcl_current_script);

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_buffer_input_data_cb: callback for input data in a buffer
 */

int
weechat_tcl_api_buffer_input_data_cb (void *data, struct t_gui_buffer *buffer,
                                       const char *input_data)
{
    struct t_script_callback *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = script_ptr2str (buffer);
        func_argv[2] = (input_data) ? (char *)input_data : empty_arg;

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "sss", func_argv);
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_tcl_api_buffer_close_cb: callback for buffer closed
 */

int
weechat_tcl_api_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    struct t_script_callback *script_callback;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = script_ptr2str (buffer);

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "ss", func_argv);
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_tcl_api_buffer_new: create a new buffer
 */

static int
weechat_tcl_api_buffer_new (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *name, *function_input, *data_input, *function_close;
    char *data_close;
    int i;

    API_FUNC(1, "buffer_new", API_RETURN_EMPTY);
    if (objc < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = Tcl_GetStringFromObj (objv[1], &i);
    function_input = Tcl_GetStringFromObj (objv[2], &i);
    data_input = Tcl_GetStringFromObj (objv[3], &i);
    function_close = Tcl_GetStringFromObj (objv[4], &i);
    data_close = Tcl_GetStringFromObj (objv[5], &i);

    result = script_ptr2str (script_api_buffer_new (weechat_tcl_plugin,
                                                    tcl_current_script,
                                                    name,
                                                    &weechat_tcl_api_buffer_input_data_cb,
                                                    function_input,
                                                    data_input,
                                                    &weechat_tcl_api_buffer_close_cb,
                                                    function_close,
                                                    data_close));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_buffer_search: search a buffer
 */

static int
weechat_tcl_api_buffer_search (ClientData clientData, Tcl_Interp *interp,
                               int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *plugin, *name;
    int i;

    API_FUNC(1, "buffer_search", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    plugin = Tcl_GetStringFromObj (objv[1], &i);
    name = Tcl_GetStringFromObj (objv[2], &i);

    result = script_ptr2str (weechat_buffer_search (plugin, name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_buffer_search_main: search main buffer (WeeChat core buffer)
 */

static int
weechat_tcl_api_buffer_search_main (ClientData clientData, Tcl_Interp *interp,
                                    int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;

    /* make C compiler happy */
    (void) clientData;
    (void) objc;
    (void) objv;

    API_FUNC(1, "buffer_search_main", API_RETURN_EMPTY);

    result = script_ptr2str (weechat_buffer_search_main ());

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_current_buffer: get current buffer
 */

static int
weechat_tcl_api_current_buffer (ClientData clientData, Tcl_Interp *interp,
                                int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;

    /* make C compiler happy */
    (void) clientData;
    (void) objc;
    (void) objv;

    API_FUNC(1, "current_buffer", API_RETURN_EMPTY);

    result = script_ptr2str (weechat_current_buffer ());

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_buffer_clear: clear a buffer
 */

static int
weechat_tcl_api_buffer_clear (ClientData clientData, Tcl_Interp *interp,
                              int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i;

    API_FUNC(1, "buffer_clear", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_clear (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* buffer */

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_buffer_close: close a buffer
 */

static int
weechat_tcl_api_buffer_close (ClientData clientData, Tcl_Interp *interp,
                              int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i;

    API_FUNC(1, "buffer_close", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    script_api_buffer_close (weechat_tcl_plugin,
                             tcl_current_script,
                             API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* buffer */

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_buffer_merge: merge a buffer to another buffer
 */

static int
weechat_tcl_api_buffer_merge (ClientData clientData, Tcl_Interp *interp,
                              int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i;

    API_FUNC(1, "buffer_merge", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_merge (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)), /* buffer */
                          API_STR2PTR(Tcl_GetStringFromObj (objv[2], &i))); /* target_buffer */

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_buffer_unmerge: unmerge a buffer from a group of merged
 *                                 buffers
 */

static int
weechat_tcl_api_buffer_unmerge (ClientData clientData, Tcl_Interp *interp,
                                int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i, number;

    API_FUNC(1, "buffer_unmerge", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (Tcl_GetIntFromObj (interp, objv[2], &number) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_unmerge (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)),
                            number);

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_buffer_get_integer: get a buffer property as integer
 */

static int
weechat_tcl_api_buffer_get_integer (ClientData clientData, Tcl_Interp *interp,
                                    int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *property;
    int result;
    int i;

    API_FUNC(1, "buffer_get_integer", API_RETURN_INT(-1));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_buffer_get_integer (API_STR2PTR(buffer), property);

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_buffer_get_string: get a buffer property as string
 */

static int
weechat_tcl_api_buffer_get_string (ClientData clientData, Tcl_Interp *interp,
                                   int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *property;
    const char *result;
    int i;

    API_FUNC(1, "buffer_get_string", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_buffer_get_string (API_STR2PTR(buffer), property);

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_buffer_get_pointer: get a buffer property as pointer
 */

static int
weechat_tcl_api_buffer_get_pointer (ClientData clientData, Tcl_Interp *interp,
                                    int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *property, *result;
    int i;

    API_FUNC(1, "buffer_get_pointer", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);

    result = script_ptr2str (weechat_buffer_get_pointer (API_STR2PTR(buffer),
                                                         property));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_buffer_set: set a buffer property
 */

static int
weechat_tcl_api_buffer_set (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *property, *value;
    int i;

    API_FUNC(1, "buffer_set", API_RETURN_ERROR);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);
    value = Tcl_GetStringFromObj (objv[3], &i);

    weechat_buffer_set (API_STR2PTR(buffer), property, value);

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_buffer_string_replace_local_var: replace local variables ($var) in a string,
 *                                                  using value of local variables
 */

static int
weechat_tcl_api_buffer_string_replace_local_var (ClientData clientData, Tcl_Interp *interp,
                                                 int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *string, *result;
    int i;

    API_FUNC(1, "buffer_string_replace_local_var", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    string = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_buffer_string_replace_local_var (API_STR2PTR(buffer), string);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_buffer_match_list: return 1 if buffers matches list of buffers
 */

static int
weechat_tcl_api_buffer_match_list (ClientData clientData, Tcl_Interp *interp,
                                   int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *string;
    int result;
    int i;

    API_FUNC(1, "buffer_match_list", API_RETURN_INT(0));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    string = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_buffer_match_list (API_STR2PTR(buffer), string);

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_current_window: get current window
 */

static int
weechat_tcl_api_current_window (ClientData clientData, Tcl_Interp *interp,
                                int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;

    /* make C compiler happy */
    (void) clientData;
    (void) objc;
    (void) objv;

    API_FUNC(1, "current_window", API_RETURN_EMPTY);

    result = script_ptr2str (weechat_current_window ());

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_window_search_with_buffer: search a window with buffer
 *                                            pointer
 */

static int
weechat_tcl_api_window_search_with_buffer (ClientData clientData, Tcl_Interp *interp,
                                           int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *result;
    int i;

    API_FUNC(1, "window_search_with_buffer", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);

    result = script_ptr2str (weechat_window_search_with_buffer (API_STR2PTR(buffer)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_window_get_integer: get a window property as integer
 */

static int
weechat_tcl_api_window_get_integer (ClientData clientData, Tcl_Interp *interp,
                                    int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *window, *property;
    int result;
    int i;

    API_FUNC(1, "window_get_integer", API_RETURN_INT(-1));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    window = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_window_get_integer (API_STR2PTR(window), property);

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_window_get_string: get a window property as string
 */

static int
weechat_tcl_api_window_get_string (ClientData clientData, Tcl_Interp *interp,
                                   int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *window, *property;
    const char *result;
    int i;

    API_FUNC(1, "window_get_string", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    window = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_window_get_string (API_STR2PTR(window), property);

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_window_get_pointer: get a window property as pointer
 */

static int
weechat_tcl_api_window_get_pointer (ClientData clientData, Tcl_Interp *interp,
                                    int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *window, *property, *result;
    int i;

    API_FUNC(1, "window_get_pointer", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    window = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);

    result = script_ptr2str (weechat_window_get_pointer (API_STR2PTR(window),
                                                         property));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_window_set_title: set window title
 */

static int
weechat_tcl_api_window_set_title (ClientData clientData, Tcl_Interp *interp,
                                  int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *title;
    int i;

    API_FUNC(1, "window_set_title", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    title = Tcl_GetStringFromObj (objv[1], &i);

    weechat_window_set_title (title);

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_nicklist_add_group: add a group in nicklist
 */

static int
weechat_tcl_api_nicklist_add_group (ClientData clientData, Tcl_Interp *interp,
                                    int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *buffer, *parent_group, *name, *color;
    int i, visible;

    API_FUNC(1, "nicklist_add_group", API_RETURN_EMPTY);
    if (objc < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if (Tcl_GetIntFromObj (interp, objv[5], &visible) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    parent_group = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);
    color = Tcl_GetStringFromObj (objv[4], &i);

    result = script_ptr2str (weechat_nicklist_add_group (API_STR2PTR(buffer),
                                                         API_STR2PTR(parent_group),
                                                         name,
                                                         color,
                                                         visible)); /* visible */

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_nicklist_search_group: search a group in nicklist
 */

static int
weechat_tcl_api_nicklist_search_group (ClientData clientData, Tcl_Interp *interp,
                                       int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *buffer, *from_group, *name;
    int i;

    API_FUNC(1, "nicklist_search_group", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    from_group = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);

    result = script_ptr2str (weechat_nicklist_search_group (API_STR2PTR(buffer),
                                                            API_STR2PTR(from_group),
                                                            name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_nicklist_add_nick: add a nick in nicklist
 */

static int
weechat_tcl_api_nicklist_add_nick (ClientData clientData, Tcl_Interp *interp,
                                   int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *buffer, *group, *name, *color, *prefix, *prefix_color;
    int i, visible;

    API_FUNC(1, "nicklist_add_nick", API_RETURN_EMPTY);
    if (objc < 8)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if (Tcl_GetIntFromObj (interp, objv[7], &visible) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    group = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);
    color = Tcl_GetStringFromObj (objv[4], &i);
    prefix = Tcl_GetStringFromObj (objv[5], &i);
    prefix_color = Tcl_GetStringFromObj (objv[6], &i);

    result = script_ptr2str (weechat_nicklist_add_nick (API_STR2PTR(buffer),
                                                        API_STR2PTR(group),
                                                        name,
                                                        color,
                                                        prefix,
                                                        prefix_color,
                                                        visible)); /* visible */

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_nicklist_search_nick: search a nick in nicklist
 */

static int
weechat_tcl_api_nicklist_search_nick (ClientData clientData, Tcl_Interp *interp,
                                      int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *buffer, *from_group, *name;
    int i;

    API_FUNC(1, "nicklist_search_nick", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    from_group = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);

    result = script_ptr2str (weechat_nicklist_search_nick (API_STR2PTR(buffer),
                                                           API_STR2PTR(from_group),
                                                           name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_nicklist_remove_group: remove a group from nicklist
 */

static int
weechat_tcl_api_nicklist_remove_group (ClientData clientData, Tcl_Interp *interp,
                                       int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *group;
    int i;

    API_FUNC(1, "nicklist_remove_group", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    group = Tcl_GetStringFromObj (objv[2], &i);

    weechat_nicklist_remove_group (API_STR2PTR(buffer),
                                   API_STR2PTR(group));

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_nicklist_remove_nick: remove a nick from nicklist
 */

static int
weechat_tcl_api_nicklist_remove_nick (ClientData clientData, Tcl_Interp *interp,
                                      int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *nick;
    int i;

    API_FUNC(1, "nicklist_remove_nick", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    nick = Tcl_GetStringFromObj (objv[2], &i);

    weechat_nicklist_remove_nick (API_STR2PTR(buffer),
                                  API_STR2PTR(nick));

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_nicklist_remove_all: remove all groups/nicks from nicklist
 */

static int
weechat_tcl_api_nicklist_remove_all (ClientData clientData, Tcl_Interp *interp,
                                     int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i;

    API_FUNC(1, "nicklist_remove_all", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_nicklist_remove_all (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* buffer */

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_nicklist_group_get_integer: get a group property as integer
 */

static int
weechat_tcl_api_nicklist_group_get_integer (ClientData clientData,
                                            Tcl_Interp *interp,
                                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *group, *property;
    int result;
    int i;

    API_FUNC(1, "nicklist_group_get_integer", API_RETURN_INT(-1));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    group = Tcl_GetStringFromObj (objv[2], &i);
    property = Tcl_GetStringFromObj (objv[3], &i);

    result = weechat_nicklist_group_get_integer (API_STR2PTR(buffer),
                                                 API_STR2PTR(group),
                                                 property);

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_nicklist_group_get_string: get a group property as string
 */

static int
weechat_tcl_api_nicklist_group_get_string (ClientData clientData,
                                           Tcl_Interp *interp,
                                           int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *group, *property;
    const char *result;
    int i;

    API_FUNC(1, "nicklist_group_get_string", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    group = Tcl_GetStringFromObj (objv[2], &i);
    property = Tcl_GetStringFromObj (objv[3], &i);

    result = weechat_nicklist_group_get_string (API_STR2PTR(buffer),
                                                API_STR2PTR(group),
                                                property);

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_nicklist_group_get_pointer: get a group property as pointer
 */

static int
weechat_tcl_api_nicklist_group_get_pointer (ClientData clientData,
                                            Tcl_Interp *interp,
                                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *group, *property, *result;
    int i;

    API_FUNC(1, "nicklist_group_get_pointer", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    group = Tcl_GetStringFromObj (objv[2], &i);
    property = Tcl_GetStringFromObj (objv[3], &i);

    result = script_ptr2str (weechat_nicklist_group_get_pointer (API_STR2PTR(buffer),
                                                                 API_STR2PTR(group),
                                                                 property));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_nicklist_group_set: set a group property
 */

static int
weechat_tcl_api_nicklist_group_set (ClientData clientData, Tcl_Interp *interp,
                                    int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *group, *property, *value;
    int i;

    API_FUNC(1, "nicklist_group_set", API_RETURN_ERROR);
    if (objc < 5)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    group = Tcl_GetStringFromObj (objv[2], &i);
    property = Tcl_GetStringFromObj (objv[3], &i);
    value = Tcl_GetStringFromObj (objv[4], &i);

    weechat_nicklist_group_set (API_STR2PTR(buffer),
                                API_STR2PTR(group),
                                property,
                                value);

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_nicklist_nick_get_integer: get a nick property as integer
 */

static int
weechat_tcl_api_nicklist_nick_get_integer (ClientData clientData,
                                           Tcl_Interp *interp,
                                           int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *nick, *property;
    int result;
    int i;

    API_FUNC(1, "nicklist_nick_get_integer", API_RETURN_INT(-1));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    nick = Tcl_GetStringFromObj (objv[2], &i);
    property = Tcl_GetStringFromObj (objv[3], &i);

    result = weechat_nicklist_nick_get_integer (API_STR2PTR(buffer),
                                                API_STR2PTR(nick),
                                                property);

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_nicklist_nick_get_string: get a nick property as string
 */

static int
weechat_tcl_api_nicklist_nick_get_string (ClientData clientData,
                                          Tcl_Interp *interp,
                                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *nick, *property;
    const char *result;
    int i;

    API_FUNC(1, "nicklist_nick_get_string", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    nick = Tcl_GetStringFromObj (objv[2], &i);
    property = Tcl_GetStringFromObj (objv[3], &i);

    result = weechat_nicklist_nick_get_string (API_STR2PTR(buffer),
                                               API_STR2PTR(nick),
                                               property);

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_nicklist_nick_get_pointer: get a nick property as pointer
 */

static int
weechat_tcl_api_nicklist_nick_get_pointer (ClientData clientData,
                                           Tcl_Interp *interp,
                                           int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *nick, *property, *result;
    int i;

    API_FUNC(1, "nicklist_nick_get_pointer", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    nick = Tcl_GetStringFromObj (objv[2], &i);
    property = Tcl_GetStringFromObj (objv[3], &i);

    result = script_ptr2str (weechat_nicklist_nick_get_pointer (API_STR2PTR(buffer),
                                                                API_STR2PTR(nick),
                                                                property));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_nicklist_nick_set: set a nick property
 */

static int
weechat_tcl_api_nicklist_nick_set (ClientData clientData, Tcl_Interp *interp,
                                   int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *nick, *property, *value;
    int i;

    API_FUNC(1, "nicklist_nick_set", API_RETURN_ERROR);
    if (objc < 5)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    nick = Tcl_GetStringFromObj (objv[2], &i);
    property = Tcl_GetStringFromObj (objv[3], &i);
    value = Tcl_GetStringFromObj (objv[4], &i);

    weechat_nicklist_nick_set (API_STR2PTR(buffer),
                               API_STR2PTR(nick),
                               property,
                               value);

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_bar_item_search: search a bar item
 */

static int
weechat_tcl_api_bar_item_search (ClientData clientData, Tcl_Interp *interp,
                                 int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;
    int i;

    API_FUNC(1, "bar_item_search", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = script_ptr2str (weechat_bar_item_search (Tcl_GetStringFromObj (objv[1], &i))); /* name */

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_bar_item_build_cb: callback for building bar item
 */

char *
weechat_tcl_api_bar_item_build_cb (void *data, struct t_gui_bar_item *item,
                                   struct t_gui_window *window)
{
    struct t_script_callback *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' }, *ret;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = script_ptr2str (item);
        func_argv[2] = script_ptr2str (window);

        ret = (char *)weechat_tcl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_STRING,
                                        script_callback->function,
                                        "sss", func_argv);

        if (func_argv[1])
            free (func_argv[1]);
        if (func_argv[2])
            free (func_argv[2]);

        return ret;
    }

    return NULL;
}

/*
 * weechat_tcl_api_bar_item_new: add a new bar item
 */

static int
weechat_tcl_api_bar_item_new (ClientData clientData, Tcl_Interp *interp,
                              int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *name, *function, *data;
    int i;

    API_FUNC(1, "bar_item_new", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    data = Tcl_GetStringFromObj (objv[3], &i);

    result = script_ptr2str (script_api_bar_item_new (weechat_tcl_plugin,
                                                      tcl_current_script,
                                                      name,
                                                      &weechat_tcl_api_bar_item_build_cb,
                                                      function,
                                                      data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_bar_item_update: update a bar item on screen
 */

static int
weechat_tcl_api_bar_item_update (ClientData clientData, Tcl_Interp *interp,
                                 int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i;

    API_FUNC(1, "bar_item_update", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_bar_item_update (Tcl_GetStringFromObj (objv[1], &i)); /* name */

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_bar_item_remove: remove a bar item
 */

static int
weechat_tcl_api_bar_item_remove (ClientData clientData, Tcl_Interp *interp,
                                 int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i;

    API_FUNC(1, "bar_item_remove", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    script_api_bar_item_remove (weechat_tcl_plugin,
                                tcl_current_script,
                                API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* item */

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_bar_search: search a bar
 */

static int
weechat_tcl_api_bar_search (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;
    int i;

    API_FUNC(1, "bar_search", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = script_ptr2str (weechat_bar_search (Tcl_GetStringFromObj (objv[1], &i))); /* name */

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_bar_new: add a new bar
 */

static int
weechat_tcl_api_bar_new (ClientData clientData, Tcl_Interp *interp,
                         int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *name, *hidden, *priority, *type, *conditions, *position;
    char *filling_top_bottom, *filling_left_right, *size, *size_max, *color_fg;
    char *color_delim, *color_bg, *separator, *bar_items;
    int i;

    API_FUNC(1, "bar_new", API_RETURN_EMPTY);
    if (objc < 16)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = Tcl_GetStringFromObj (objv[1], &i);
    hidden = Tcl_GetStringFromObj (objv[2], &i);
    priority = Tcl_GetStringFromObj (objv[3], &i);
    type = Tcl_GetStringFromObj (objv[4], &i);
    conditions = Tcl_GetStringFromObj (objv[5], &i);
    position = Tcl_GetStringFromObj (objv[6], &i);
    filling_top_bottom = Tcl_GetStringFromObj (objv[7], &i);
    filling_left_right = Tcl_GetStringFromObj (objv[8], &i);
    size = Tcl_GetStringFromObj (objv[9], &i);
    size_max = Tcl_GetStringFromObj (objv[10], &i);
    color_fg = Tcl_GetStringFromObj (objv[11], &i);
    color_delim = Tcl_GetStringFromObj (objv[12], &i);
    color_bg = Tcl_GetStringFromObj (objv[13], &i);
    separator = Tcl_GetStringFromObj (objv[14], &i);
    bar_items = Tcl_GetStringFromObj (objv[15], &i);

    result = script_ptr2str (weechat_bar_new (name,
                                              hidden,
                                              priority,
                                              type,
                                              conditions,
                                              position,
                                              filling_top_bottom,
                                              filling_left_right,
                                              size,
                                              size_max,
                                              color_fg,
                                              color_delim,
                                              color_bg,
                                              separator,
                                              bar_items));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_bar_set: set a bar property
 */

static int
weechat_tcl_api_bar_set (ClientData clientData, Tcl_Interp *interp,
                         int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *bar, *property, *value;
    int i;

    API_FUNC(1, "bar_set", API_RETURN_ERROR);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_ERROR);

    bar = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);
    value = Tcl_GetStringFromObj (objv[3], &i);

    weechat_bar_set (API_STR2PTR(bar), property, value);

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_bar_update: update a bar on screen
 */

static int
weechat_tcl_api_bar_update (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i;

    API_FUNC(1, "bar_update", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_bar_update (Tcl_GetStringFromObj (objv[1], &i)); /* name */

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_bar_remove: remove a bar
 */

static int
weechat_tcl_api_bar_remove (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i;

    API_FUNC(1, "bar_remove", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_bar_remove (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* bar */

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_command: execute a command on a buffer
 */

static int
weechat_tcl_api_command (ClientData clientData, Tcl_Interp *interp,
                         int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *buffer, *command;
    int i;

    API_FUNC(1, "command", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    command = Tcl_GetStringFromObj (objv[2], &i);

    script_api_command (weechat_tcl_plugin,
                        tcl_current_script,
                        API_STR2PTR(buffer),
                        command);

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_info_get: get info (as string)
 */

static int
weechat_tcl_api_info_get (ClientData clientData, Tcl_Interp *interp,
                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_FUNC(1, "info_get", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_info_get (Tcl_GetStringFromObj (objv[1], &i),
                               Tcl_GetStringFromObj (objv[2], &i));

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_info_get_hashtable: get info (as hashtable)
 */

static int
weechat_tcl_api_info_get_hashtable (ClientData clientData, Tcl_Interp *interp,
                                    int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp, *result_dict;
    struct t_hashtable *hashtable, *result_hashtable;
    int i;

    API_FUNC(1, "info_get_hashtable", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hashtable = weechat_tcl_dict_to_hashtable (interp, objv[2],
                                               WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE);

    result_hashtable = weechat_info_get_hashtable (Tcl_GetStringFromObj (objv[1], &i),
                                                   hashtable);
    result_dict = weechat_tcl_hashtable_to_dict (interp, result_hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);
    if (result_hashtable)
        weechat_hashtable_free (result_hashtable);

    API_RETURN_OBJ(result_dict);
}

/*
 * weechat_tcl_api_infolist_new: create a new infolist
 */

static int
weechat_tcl_api_infolist_new (ClientData clientData, Tcl_Interp *interp,
                              int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;

    /* make C compiler happy */
    (void) clientData;
    (void) objc;
    (void) objv;

    API_FUNC(1, "infolist_new", API_RETURN_EMPTY);

    result = script_ptr2str (weechat_infolist_new ());

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_infolist_new_item: create new item in infolist
 */

static int
weechat_tcl_api_infolist_new_item (ClientData clientData, Tcl_Interp *interp,
                                   int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;
    int i;

    API_FUNC(1, "infolist_new_item", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = script_ptr2str (weechat_infolist_new_item (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)))); /* infolist */

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_infolist_new_var_integer: create new integer variable in
 *                                           infolist
 */

static int
weechat_tcl_api_infolist_new_var_integer (ClientData clientData, Tcl_Interp *interp,
                                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;
    int i, value;

    API_FUNC(1, "infolist_new_var_integer", API_RETURN_INT(0));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(0));

    if (Tcl_GetIntFromObj (interp, objv[3], &value) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = script_ptr2str (weechat_infolist_new_var_integer (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)), /* infolist */
                                                               Tcl_GetStringFromObj (objv[2], &i), /* name */
                                                               value));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_infolist_new_var_string: create new string variable in
 *                                          infolist
 */

static int
weechat_tcl_api_infolist_new_var_string (ClientData clientData, Tcl_Interp *interp,
                                         int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;
    int i;

    API_FUNC(1, "infolist_new_var_string", API_RETURN_INT(0));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = script_ptr2str (weechat_infolist_new_var_string (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)), /* infolist */
                                                              Tcl_GetStringFromObj (objv[2], &i), /* name */
                                                              Tcl_GetStringFromObj (objv[3], &i))); /* value */

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_infolist_new_var_pointer: create new pointer variable in infolist
 */

static int
weechat_tcl_api_infolist_new_var_pointer (ClientData clientData, Tcl_Interp *interp,
                                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;
    int i;

    API_FUNC(1, "infolist_new_var_pointer", API_RETURN_INT(0));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = script_ptr2str (weechat_infolist_new_var_pointer (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)), /* infolist */
                                                               Tcl_GetStringFromObj (objv[2], &i), /* name */
                                                               API_STR2PTR(Tcl_GetStringFromObj (objv[3], &i)))); /* value */

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_infolist_new_var_time: create new time variable in infolist
 */

static int
weechat_tcl_api_infolist_new_var_time (ClientData clientData, Tcl_Interp *interp,
                                       int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;
    int i, value;

    API_FUNC(1, "infolist_new_var_time", API_RETURN_INT(0));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(0));

    if (Tcl_GetIntFromObj (interp, objv[3], &value) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = script_ptr2str (weechat_infolist_new_var_time (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)), /* infolist */
                                                            Tcl_GetStringFromObj (objv[2], &i), /* name */
                                                            value));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_infolist_get: get list with infos
 */

static int
weechat_tcl_api_infolist_get (ClientData clientData, Tcl_Interp *interp,
                              int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *name, *pointer, *arguments;
    int i;

    API_FUNC(1, "infolist_get", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = Tcl_GetStringFromObj (objv[1], &i);
    pointer = Tcl_GetStringFromObj (objv[2], &i);
    arguments = Tcl_GetStringFromObj (objv[3], &i);

    result = script_ptr2str (weechat_infolist_get (name,
                                                   API_STR2PTR(pointer),
                                                   arguments));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_infolist_next: move item pointer to next item in infolist
 */

static int
weechat_tcl_api_infolist_next (ClientData clientData, Tcl_Interp *interp,
                               int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int result, i;

    API_FUNC(1, "infolist_next", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_infolist_next (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* infolist */

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_infolist_prev: move item pointer to previous item in
 *                                infolist
 */

static int
weechat_tcl_api_infolist_prev (ClientData clientData, Tcl_Interp *interp,
                               int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int result, i;

    API_FUNC(1, "infolist_prev", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_infolist_prev (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* infolist */

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_infolist_reset_item_cursor: reset pointer to current item in
 *                                             infolist
 */

static int
weechat_tcl_api_infolist_reset_item_cursor (ClientData clientData,
                                            Tcl_Interp *interp,
                                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i;

    API_FUNC(1, "infolist_reset_item_cursor", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_infolist_reset_item_cursor (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* infolist */

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_infolist_fields: get list of fields for current item of
 *                                  infolist
 */

static int
weechat_tcl_api_infolist_fields (ClientData clientData, Tcl_Interp *interp,
                                 int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_FUNC(1, "infolist_fields", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_infolist_fields (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* infolist */

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_infolist_integer: get integer value of a variable in
 *                                   infolist
 */

static int
weechat_tcl_api_infolist_integer (ClientData clientData, Tcl_Interp *interp,
                                  int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *infolist, *variable;
    int result, i;

    API_FUNC(1, "infolist_integer", API_RETURN_INT(0));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    infolist = Tcl_GetStringFromObj (objv[1], &i);
    variable = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_infolist_integer (API_STR2PTR(infolist), variable);

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_infolist_string: get string value of a variable in infolist
 */

static int
weechat_tcl_api_infolist_string (ClientData clientData, Tcl_Interp *interp,
                                 int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *infolist, *variable;
    const char *result;
    int i;

    API_FUNC(1, "infolist_string", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = Tcl_GetStringFromObj (objv[1], &i);
    variable = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_infolist_string (API_STR2PTR(infolist), variable);

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_infolist_pointer: get pointer value of a variable in
 *                                   infolist
 */

static int
weechat_tcl_api_infolist_pointer (ClientData clientData, Tcl_Interp *interp,
                                  int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *infolist, *variable, *result;
    int i;

    API_FUNC(1, "infolist_pointer", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = Tcl_GetStringFromObj (objv[1], &i);
    variable = Tcl_GetStringFromObj (objv[2], &i);

    result = script_ptr2str (weechat_infolist_pointer (API_STR2PTR(infolist), variable));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_infolist_time: get time value of a variable in infolist
 */

static int
weechat_tcl_api_infolist_time (ClientData clientData, Tcl_Interp *interp,
                               int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    time_t time;
    struct tm *date_tmp;
    char timebuffer[64], *result, *infolist, *variable;
    int i;

    API_FUNC(1, "infolist_time", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = Tcl_GetStringFromObj (objv[1], &i);
    variable = Tcl_GetStringFromObj (objv[2], &i);

    timebuffer[0] = '\0';
    time = weechat_infolist_time (API_STR2PTR(infolist), variable);
    date_tmp = localtime (&time);
    if (date_tmp)
        strftime (timebuffer, sizeof (timebuffer), "%F %T", date_tmp);
    result = strdup (timebuffer);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_infolist_free: free infolist
 */

static int
weechat_tcl_api_infolist_free (ClientData clientData, Tcl_Interp *interp,
                               int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    int i;

    API_FUNC(1, "infolist_free", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_infolist_free (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* infolist */

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_hdata_get: get hdata
 */

static int
weechat_tcl_api_hdata_get (ClientData clientData, Tcl_Interp *interp,
                           int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *name;
    int i;

    API_FUNC(1, "hdata_get", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = Tcl_GetStringFromObj (objv[1], &i);

    result = script_ptr2str (weechat_hdata_get (name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hdata_get_var_offset: get offset of variable in hdata
 */

static int
weechat_tcl_api_hdata_get_var_offset (ClientData clientData, Tcl_Interp *interp,
                                      int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *hdata, *name;
    int result, i;

    API_FUNC(1, "hdata_get_var_offset", API_RETURN_INT(0));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    name = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_hdata_get_var_offset (API_STR2PTR(hdata), name);

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_hdata_get_var_type_string: get type of variable as string in
 *                                            hdata
 */

static int
weechat_tcl_api_hdata_get_var_type_string (ClientData clientData,
                                           Tcl_Interp *interp,
                                           int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *hdata, *name;
    const char *result;
    int i;

    API_FUNC(1, "hdata_get_var_type_string", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    name = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_hdata_get_var_type_string (API_STR2PTR(hdata), name);

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_hdata_get_var_array_size: get array_size for variable in
 *                                           hdata
 */

static int
weechat_tcl_api_hdata_get_var_array_size (ClientData clientData,
                                          Tcl_Interp *interp,
                                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *hdata, *pointer, *name;
    int result, i;

    API_FUNC(1, "hdata_get_var_array_size", API_RETURN_INT(-1));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    pointer = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);

    result = weechat_hdata_get_var_array_size (API_STR2PTR(hdata),
                                               API_STR2PTR(pointer),
                                               name);

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_hdata_get_var_array_size_string: get array size for variable
 *                                                  in hdata (as string)
 */

static int
weechat_tcl_api_hdata_get_var_array_size_string (ClientData clientData,
                                                 Tcl_Interp *interp,
                                                 int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *hdata, *pointer, *name;
    const char *result;
    int i;

    API_FUNC(1, "hdata_get_var_array_size_string", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    pointer = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);

    result = weechat_hdata_get_var_array_size_string (API_STR2PTR(hdata),
                                                      API_STR2PTR(pointer),
                                                      name);

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_hdata_get_var_hdata: get hdata for variable in hdata
 */

static int
weechat_tcl_api_hdata_get_var_hdata (ClientData clientData, Tcl_Interp *interp,
                                     int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *hdata, *name;
    const char *result;
    int i;

    API_FUNC(1, "hdata_get_var_hdata", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    name = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_hdata_get_var_hdata (API_STR2PTR(hdata), name);

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_hdata_get_list: get list pointer in hdata
 */

static int
weechat_tcl_api_hdata_get_list (ClientData clientData, Tcl_Interp *interp,
                                int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *hdata, *name, *result;
    int i;

    API_FUNC(1, "hdata_get_list", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    name = Tcl_GetStringFromObj (objv[2], &i);

    result = script_ptr2str (weechat_hdata_get_list (API_STR2PTR(hdata),
                                                     name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hdata_check_pointer: check pointer with hdata/list
 */

static int
weechat_tcl_api_hdata_check_pointer (ClientData clientData, Tcl_Interp *interp,
                                     int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *hdata, *list, *pointer;
    int result, i;

    API_FUNC(1, "hdata_check_pointer", API_RETURN_INT(0));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    list = Tcl_GetStringFromObj (objv[2], &i);
    pointer = Tcl_GetStringFromObj (objv[3], &i);

    result = weechat_hdata_check_pointer (API_STR2PTR(hdata),
                                          API_STR2PTR(list),
                                          API_STR2PTR(pointer));

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_hdata_move: move pointer to another element in list
 */

static int
weechat_tcl_api_hdata_move (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *hdata, *pointer, *result;
    int i, count;

    API_FUNC(1, "hdata_move", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    pointer = Tcl_GetStringFromObj (objv[2], &i);

    if (Tcl_GetIntFromObj (interp, objv[3], &count) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = script_ptr2str (weechat_hdata_move (API_STR2PTR(hdata),
                                                 API_STR2PTR(pointer),
                                                 count));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hdata_char: get char value of a variable in structure
 *                             using hdata
 */

static int
weechat_tcl_api_hdata_char (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *hdata, *pointer, *name;
    int result, i;

    API_FUNC(1, "hdata_char", API_RETURN_INT(0));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    pointer = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);

    result = (int)weechat_hdata_char (API_STR2PTR(hdata),
                                      API_STR2PTR(pointer),
                                      name);

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_hdata_integer: get integer value of a variable in structure
 *                                using hdata
 */

static int
weechat_tcl_api_hdata_integer (ClientData clientData, Tcl_Interp *interp,
                               int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *hdata, *pointer, *name;
    int result, i;

    API_FUNC(1, "hdata_integer", API_RETURN_INT(0));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    pointer = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);

    result = weechat_hdata_integer (API_STR2PTR(hdata),
                                    API_STR2PTR(pointer),
                                    name);

    API_RETURN_INT(result);
}

/*
 * weechat_tcl_api_hdata_long: get long value of a variable in structure using
 *                             hdata
 */

static int
weechat_tcl_api_hdata_long (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *hdata, *pointer, *name;
    int result, i;

    API_FUNC(1, "hdata_long", API_RETURN_LONG(0));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_LONG(0));

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    pointer = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);

    result = weechat_hdata_long (API_STR2PTR(hdata),
                                 API_STR2PTR(pointer),
                                 name);

    API_RETURN_LONG(result);
}

/*
 * weechat_tcl_api_hdata_string: get string value of a variable in structure
 *                               using hdata
 */

static int
weechat_tcl_api_hdata_string (ClientData clientData, Tcl_Interp *interp,
                              int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *hdata, *pointer, *name;
    const char *result;
    int i;

    API_FUNC(1, "hdata_string", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    pointer = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);

    result = weechat_hdata_string (API_STR2PTR(hdata),
                                   API_STR2PTR(pointer),
                                   name);

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_hdata_pointer: get pointer value of a variable in structure
 *                                using hdata
 */

static int
weechat_tcl_api_hdata_pointer (ClientData clientData, Tcl_Interp *interp,
                               int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *hdata, *pointer, *name, *result;
    int i;

    API_FUNC(1, "hdata_pointer", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    pointer = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);

    result = script_ptr2str (weechat_hdata_pointer (API_STR2PTR(hdata),
                                                    API_STR2PTR(pointer),
                                                    name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hdata_time: get time value of a variable in structure using
 *                             hdata
 */

static int
weechat_tcl_api_hdata_time (ClientData clientData, Tcl_Interp *interp,
                               int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    time_t time;
    struct tm *date_tmp;
    char timebuffer[64], *result, *hdata, *pointer, *name;
    int i;

    API_FUNC(1, "hdata_time", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    pointer = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);

    timebuffer[0] = '\0';
    time = weechat_hdata_time (API_STR2PTR(hdata),
                               API_STR2PTR(pointer),
                               name);
    date_tmp = localtime (&time);
    if (date_tmp)
        strftime (timebuffer, sizeof (timebuffer), "%F %T", date_tmp);
    result = strdup (timebuffer);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hdata_hashtable: get hashtable value of a variable in
 *                                  structure using hdata
 */

static int
weechat_tcl_api_hdata_hashtable (ClientData clientData, Tcl_Interp *interp,
                                 int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp, *result_dict;
    char *hdata, *pointer, *name;
    int i;

    API_FUNC(1, "hdata_hashtable", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    pointer = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);

    result_dict = weechat_tcl_hashtable_to_dict (
        interp,
        weechat_hdata_hashtable (API_STR2PTR(hdata),
                                 API_STR2PTR(pointer),
                                 name));

    API_RETURN_OBJ(result_dict);
}

/*
 * weechat_tcl_api_hdata_get_string: get hdata property as string
 */

static int
weechat_tcl_api_hdata_get_string (ClientData clientData, Tcl_Interp *interp,
                                  int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *hdata, *property;
    const char *result;
    int i;

    API_FUNC(1, "hdata_get_string", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_hdata_get_string (API_STR2PTR(hdata), property);

    API_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_upgrade_new: create an upgrade file
 */

static int
weechat_tcl_api_upgrade_new (ClientData clientData, Tcl_Interp *interp,
                             int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *filename;
    int i, write;

    API_FUNC(1, "upgrade_new", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if (Tcl_GetIntFromObj (interp, objv[2], &write) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    filename = Tcl_GetStringFromObj (objv[1], &i);

    result = script_ptr2str (weechat_upgrade_new (filename, write));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_upgrade_write_object: write object in upgrade file
 */

static int
weechat_tcl_api_upgrade_write_object (ClientData clientData, Tcl_Interp *interp,
                                      int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *upgrade_file, *infolist;
    int rc, i, object_id;

    API_FUNC(1, "upgrade_write_object", API_RETURN_INT(0));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(0));

    if (Tcl_GetIntFromObj (interp, objv[2], &object_id) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    upgrade_file = Tcl_GetStringFromObj (objv[1], &i);
    infolist = Tcl_GetStringFromObj (objv[3], &i);

    rc = weechat_upgrade_write_object (API_STR2PTR(upgrade_file),
                                       object_id,
                                       API_STR2PTR(infolist));

    API_RETURN_INT(rc);
}

/*
 * weechat_tcl_api_upgrade_read_cb: callback for reading an object in upgrade file
 */

int
weechat_tcl_api_upgrade_read_cb (void *data,
                                 struct t_upgrade_file *upgrade_file,
                                 int object_id,
                                 struct t_infolist *infolist)
{
    struct t_script_callback *script_callback;
    void *func_argv[4];
    char empty_arg[1] = { '\0' }, str_object_id[32];
    int *rc, ret;

    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_object_id, sizeof (str_object_id), "%d", object_id);

        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = script_ptr2str (upgrade_file);
        func_argv[2] = str_object_id;
        func_argv[3] = script_ptr2str (infolist);

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "ssss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);
        if (func_argv[3])
            free (func_argv[3]);

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_tcl_api_upgrade_read: read upgrade file
 */

static int
weechat_tcl_api_upgrade_read (ClientData clientData, Tcl_Interp *interp,
                              int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *upgrade_file, *function, *data;
    int i, rc;

    API_FUNC(1, "upgrade_read", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    upgrade_file = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    data = Tcl_GetStringFromObj (objv[3], &i);

    rc = script_api_upgrade_read (weechat_tcl_plugin,
                                  tcl_current_script,
                                  API_STR2PTR(upgrade_file),
                                  &weechat_tcl_api_upgrade_read_cb,
                                  function,
                                  data);

    API_RETURN_INT(rc);
}

/*
 * weechat_tcl_api_upgrade_close: close upgrade file
 */

static int
weechat_tcl_api_upgrade_close (ClientData clientData, Tcl_Interp *interp,
                               int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *upgrade_file;
    int i;

    API_FUNC(1, "upgrade_close", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    upgrade_file = Tcl_GetStringFromObj (objv[1], &i);

    weechat_upgrade_close (API_STR2PTR(upgrade_file));

    API_RETURN_OK;
}

/*
 * weechat_tcl_api_init: initialize subroutines
 */

void weechat_tcl_api_init (Tcl_Interp *interp)
{
    int i;
    Tcl_Obj *objp;

    /* standard initializer */
    Tcl_Init (interp);

    Tcl_Eval (interp,"namespace eval weechat {}");

    /* interface constants */
    /* set variables, TODO: make them unmodifiable (thru Tcl_TraceVar) ? */
    /* NOTE: it is not good for performance to convert "defines" to Tcl_Obj */
    objp = Tcl_NewIntObj (WEECHAT_RC_OK);
    Tcl_IncrRefCount (objp);

    Tcl_SetVar (interp, "weechat::WEECHAT_RC_OK", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_RC_OK_EAT);
    Tcl_SetVar (interp, "weechat::WEECHAT_RC_OK_EAT", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_RC_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_RC_ERROR", Tcl_GetStringFromObj (objp, &i), 0);

    Tcl_SetIntObj (objp, WEECHAT_CONFIG_READ_OK);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_READ_OK", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_CONFIG_READ_MEMORY_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_READ_MEMORY_ERROR", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_CONFIG_READ_FILE_NOT_FOUND);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_READ_FILE_NOT_FOUND", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_CONFIG_WRITE_OK);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_WRITE_OK", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_CONFIG_WRITE_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_WRITE_ERROR", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_CONFIG_WRITE_MEMORY_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_WRITE_MEMORY_ERROR", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_CONFIG_OPTION_SET_OK_CHANGED);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_OPTION_SET_OK_CHANGED", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_CONFIG_OPTION_SET_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_OPTION_SET_ERROR", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_CONFIG_OPTION_UNSET_OK_RESET);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_OPTION_UNSET_OK_RESET", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_OPTION_UNSET_ERROR", Tcl_GetStringFromObj (objp, &i), 0);

    Tcl_SetStringObj (objp, WEECHAT_LIST_POS_SORT, -1);
    Tcl_SetVar (interp, "weechat::WEECHAT_LIST_POS_SORT", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetStringObj (objp, WEECHAT_LIST_POS_BEGINNING, -1);
    Tcl_SetVar (interp, "weechat::WEECHAT_LIST_POS_BEGINNING", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetStringObj (objp, WEECHAT_LIST_POS_END, -1);
    Tcl_SetVar (interp, "weechat::WEECHAT_LIST_POS_END", Tcl_GetStringFromObj (objp, &i), 0);

    Tcl_SetStringObj (objp, WEECHAT_HOTLIST_LOW, -1);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOTLIST_LOW", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetStringObj (objp, WEECHAT_HOTLIST_MESSAGE, -1);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOTLIST_MESSAGE", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetStringObj (objp, WEECHAT_HOTLIST_PRIVATE, -1);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOTLIST_PRIVATE", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetStringObj (objp, WEECHAT_HOTLIST_HIGHLIGHT, -1);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOTLIST_HIGHLIGHT", Tcl_GetStringFromObj (objp, &i), 0);

    Tcl_SetIntObj (objp, WEECHAT_HOOK_PROCESS_RUNNING);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_PROCESS_RUNNING", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_HOOK_PROCESS_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_PROCESS_ERROR", Tcl_GetStringFromObj (objp, &i), 0);

    Tcl_SetIntObj (objp, WEECHAT_HOOK_CONNECT_OK);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_OK", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_HOOK_CONNECT_PROXY_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_PROXY_ERROR", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_HOOK_CONNECT_MEMORY_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_MEMORY_ERROR", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetIntObj (objp, WEECHAT_HOOK_CONNECT_TIMEOUT);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_TIMEOUT", Tcl_GetStringFromObj (objp, &i), 0);

    Tcl_SetStringObj (objp, WEECHAT_HOOK_SIGNAL_STRING, -1);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_SIGNAL_STRING", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetStringObj (objp, WEECHAT_HOOK_SIGNAL_INT, -1);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_SIGNAL_INT", Tcl_GetStringFromObj (objp, &i), 0);
    Tcl_SetStringObj (objp, WEECHAT_HOOK_SIGNAL_POINTER, -1);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_SIGNAL_POINTER", Tcl_GetStringFromObj (objp, &i), 0);

    Tcl_DecrRefCount (objp);

    /* interface functions */
    API_DEF_FUNC(register);
    API_DEF_FUNC(plugin_get_name);
    API_DEF_FUNC(charset_set);
    API_DEF_FUNC(iconv_to_internal);
    API_DEF_FUNC(iconv_from_internal);
    API_DEF_FUNC(gettext);
    API_DEF_FUNC(ngettext);
    API_DEF_FUNC(string_match);
    API_DEF_FUNC(string_has_highlight);
    API_DEF_FUNC(string_has_highlight_regex);
    API_DEF_FUNC(string_mask_to_regex);
    API_DEF_FUNC(string_remove_color);
    API_DEF_FUNC(string_is_command_char);
    API_DEF_FUNC(string_input_for_buffer);
    API_DEF_FUNC(mkdir_home);
    API_DEF_FUNC(mkdir);
    API_DEF_FUNC(mkdir_parents);
    API_DEF_FUNC(list_new);
    API_DEF_FUNC(list_add);
    API_DEF_FUNC(list_search);
    API_DEF_FUNC(list_search_pos);
    API_DEF_FUNC(list_casesearch);
    API_DEF_FUNC(list_casesearch_pos);
    API_DEF_FUNC(list_get);
    API_DEF_FUNC(list_set);
    API_DEF_FUNC(list_next);
    API_DEF_FUNC(list_prev);
    API_DEF_FUNC(list_string);
    API_DEF_FUNC(list_size);
    API_DEF_FUNC(list_remove);
    API_DEF_FUNC(list_remove_all);
    API_DEF_FUNC(list_free);
    API_DEF_FUNC(config_new);
    API_DEF_FUNC(config_new_section);
    API_DEF_FUNC(config_search_section);
    API_DEF_FUNC(config_new_option);
    API_DEF_FUNC(config_search_option);
    API_DEF_FUNC(config_string_to_boolean);
    API_DEF_FUNC(config_option_reset);
    API_DEF_FUNC(config_option_set);
    API_DEF_FUNC(config_option_set_null);
    API_DEF_FUNC(config_option_unset);
    API_DEF_FUNC(config_option_rename);
    API_DEF_FUNC(config_option_is_null);
    API_DEF_FUNC(config_option_default_is_null);
    API_DEF_FUNC(config_boolean);
    API_DEF_FUNC(config_boolean_default);
    API_DEF_FUNC(config_integer);
    API_DEF_FUNC(config_integer_default);
    API_DEF_FUNC(config_string);
    API_DEF_FUNC(config_string_default);
    API_DEF_FUNC(config_color);
    API_DEF_FUNC(config_color_default);
    API_DEF_FUNC(config_write_option);
    API_DEF_FUNC(config_write_line);
    API_DEF_FUNC(config_write);
    API_DEF_FUNC(config_read);
    API_DEF_FUNC(config_reload);
    API_DEF_FUNC(config_option_free);
    API_DEF_FUNC(config_section_free_options);
    API_DEF_FUNC(config_section_free);
    API_DEF_FUNC(config_free);
    API_DEF_FUNC(config_get);
    API_DEF_FUNC(config_get_plugin);
    API_DEF_FUNC(config_is_set_plugin);
    API_DEF_FUNC(config_set_plugin);
    API_DEF_FUNC(config_set_desc_plugin);
    API_DEF_FUNC(config_unset_plugin);
    API_DEF_FUNC(key_bind);
    API_DEF_FUNC(key_unbind);
    API_DEF_FUNC(prefix);
    API_DEF_FUNC(color);
    API_DEF_FUNC(print);
    API_DEF_FUNC(print_date_tags);
    API_DEF_FUNC(print_y);
    API_DEF_FUNC(log_print);
    API_DEF_FUNC(hook_command);
    API_DEF_FUNC(hook_command_run);
    API_DEF_FUNC(hook_timer);
    API_DEF_FUNC(hook_fd);
    API_DEF_FUNC(hook_process);
    API_DEF_FUNC(hook_process_hashtable);
    API_DEF_FUNC(hook_connect);
    API_DEF_FUNC(hook_print);
    API_DEF_FUNC(hook_signal);
    API_DEF_FUNC(hook_signal_send);
    API_DEF_FUNC(hook_hsignal);
    API_DEF_FUNC(hook_hsignal_send);
    API_DEF_FUNC(hook_config);
    API_DEF_FUNC(hook_completion);
    API_DEF_FUNC(hook_completion_list_add);
    API_DEF_FUNC(hook_modifier);
    API_DEF_FUNC(hook_modifier_exec);
    API_DEF_FUNC(hook_info);
    API_DEF_FUNC(hook_info_hashtable);
    API_DEF_FUNC(hook_infolist);
    API_DEF_FUNC(hook_focus);
    API_DEF_FUNC(unhook);
    API_DEF_FUNC(unhook_all);
    API_DEF_FUNC(buffer_new);
    API_DEF_FUNC(buffer_search);
    API_DEF_FUNC(buffer_search_main);
    API_DEF_FUNC(current_buffer);
    API_DEF_FUNC(buffer_clear);
    API_DEF_FUNC(buffer_close);
    API_DEF_FUNC(buffer_merge);
    API_DEF_FUNC(buffer_unmerge);
    API_DEF_FUNC(buffer_get_integer);
    API_DEF_FUNC(buffer_get_string);
    API_DEF_FUNC(buffer_get_pointer);
    API_DEF_FUNC(buffer_set);
    API_DEF_FUNC(buffer_string_replace_local_var);
    API_DEF_FUNC(buffer_match_list);
    API_DEF_FUNC(current_window);
    API_DEF_FUNC(window_search_with_buffer);
    API_DEF_FUNC(window_get_integer);
    API_DEF_FUNC(window_get_string);
    API_DEF_FUNC(window_get_pointer);
    API_DEF_FUNC(window_set_title);
    API_DEF_FUNC(nicklist_add_group);
    API_DEF_FUNC(nicklist_search_group);
    API_DEF_FUNC(nicklist_add_nick);
    API_DEF_FUNC(nicklist_search_nick);
    API_DEF_FUNC(nicklist_remove_group);
    API_DEF_FUNC(nicklist_remove_nick);
    API_DEF_FUNC(nicklist_remove_all);
    API_DEF_FUNC(nicklist_group_get_integer);
    API_DEF_FUNC(nicklist_group_get_string);
    API_DEF_FUNC(nicklist_group_get_pointer);
    API_DEF_FUNC(nicklist_group_set);
    API_DEF_FUNC(nicklist_nick_get_integer);
    API_DEF_FUNC(nicklist_nick_get_string);
    API_DEF_FUNC(nicklist_nick_get_pointer);
    API_DEF_FUNC(nicklist_nick_set);
    API_DEF_FUNC(bar_item_search);
    API_DEF_FUNC(bar_item_new);
    API_DEF_FUNC(bar_item_update);
    API_DEF_FUNC(bar_item_remove);
    API_DEF_FUNC(bar_search);
    API_DEF_FUNC(bar_new);
    API_DEF_FUNC(bar_set);
    API_DEF_FUNC(bar_update);
    API_DEF_FUNC(bar_remove);
    API_DEF_FUNC(command);
    API_DEF_FUNC(info_get);
    API_DEF_FUNC(info_get_hashtable);
    API_DEF_FUNC(infolist_new);
    API_DEF_FUNC(infolist_new_item);
    API_DEF_FUNC(infolist_new_var_integer);
    API_DEF_FUNC(infolist_new_var_string);
    API_DEF_FUNC(infolist_new_var_pointer);
    API_DEF_FUNC(infolist_new_var_time);
    API_DEF_FUNC(infolist_get);
    API_DEF_FUNC(infolist_next);
    API_DEF_FUNC(infolist_prev);
    API_DEF_FUNC(infolist_reset_item_cursor);
    API_DEF_FUNC(infolist_fields);
    API_DEF_FUNC(infolist_integer);
    API_DEF_FUNC(infolist_string);
    API_DEF_FUNC(infolist_pointer);
    API_DEF_FUNC(infolist_time);
    API_DEF_FUNC(infolist_free);
    API_DEF_FUNC(hdata_get);
    API_DEF_FUNC(hdata_get_var_offset);
    API_DEF_FUNC(hdata_get_var_type_string);
    API_DEF_FUNC(hdata_get_var_array_size);
    API_DEF_FUNC(hdata_get_var_array_size_string);
    API_DEF_FUNC(hdata_get_var_hdata);
    API_DEF_FUNC(hdata_get_list);
    API_DEF_FUNC(hdata_check_pointer);
    API_DEF_FUNC(hdata_move);
    API_DEF_FUNC(hdata_char);
    API_DEF_FUNC(hdata_integer);
    API_DEF_FUNC(hdata_long);
    API_DEF_FUNC(hdata_string);
    API_DEF_FUNC(hdata_pointer);
    API_DEF_FUNC(hdata_time);
    API_DEF_FUNC(hdata_hashtable);
    API_DEF_FUNC(hdata_get_string);
    API_DEF_FUNC(upgrade_new);
    API_DEF_FUNC(upgrade_write_object);
    API_DEF_FUNC(upgrade_read);
    API_DEF_FUNC(upgrade_close);
}
