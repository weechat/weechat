/*
 * weechat-tcl-api.c - tcl API functions
 *
 * Copyright (C) 2008-2010 Dmitry Kobylin <fnfal@academ.tsc.ru>
 * Copyright (C) 2008 Julien Louis <ptitlouis@sysif.net>
 * Copyright (C) 2008-2023 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2012 Simon Arlott
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

#include <tcl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "../plugin-script.h"
#include "../plugin-script-api.h"
#include "weechat-tcl.h"


/* Magic value to indicate NULL since Tcl only has string types. The value is
 * \uFFFF\uFFFF\uFFFFWEECHAT_NULL\uFFFF\uFFFF\uFFFF. \uFFFF is used because
 * it's reserved in Unicode as not a character, so this string is very unlikely
 * to appear unintentionally since it's not valid text. */
#define WEECHAT_NULL_STRING \
    "\xef\xbf\xbf\xef\xbf\xbf\xef\xbf\xbfWEECHAT_NULL\xef\xbf\xbf\xef\xbf\xbf\xef\xbf\xbf"


#define API_DEF_FUNC(__name)                                            \
    Tcl_CreateObjCommand (interp, "weechat::" #__name,                  \
                          weechat_tcl_api_##__name,                     \
                          (ClientData) NULL,                            \
                          (Tcl_CmdDeleteProc*)NULL);
#define API_FUNC(__name)                                                \
    static int                                                          \
    weechat_tcl_api_##__name (ClientData clientData,                    \
                              Tcl_Interp *interp,                       \
                              int objc,                                 \
                              Tcl_Obj *CONST objv[])
#define API_INIT_FUNC(__init, __name, __ret)                            \
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
#define API_PTR2STR(__pointer)                                          \
    plugin_script_ptr2str (__pointer)
#define API_STR2PTR(__string)                                           \
    plugin_script_str2ptr (weechat_tcl_plugin,                          \
                           TCL_CURRENT_SCRIPT_NAME,                     \
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


/*
 * Registers a tcl script.
 */

API_FUNC(register)
{
    Tcl_Obj *objp;
    char *name, *author, *version, *license, *description, *shutdown_func;
    char *charset;
    int i;

    API_INIT_FUNC(0, "register", API_RETURN_ERROR);
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

    if (plugin_script_search (tcl_scripts, name))
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
    tcl_current_script = plugin_script_add (weechat_tcl_plugin,
                                            &tcl_data,
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
 * Wrappers for functions in scripting API.
 *
 * For more info about these functions, look at their implementation in WeeChat
 * core.
 */

API_FUNC(plugin_get_name)
{
    Tcl_Obj *objp;
    char *plugin;
    const char *result;
    int i;

    API_INIT_FUNC(1, "plugin_get_name", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    plugin = Tcl_GetStringFromObj (objv[1], &i);

    result = weechat_plugin_get_name (API_STR2PTR(plugin));

    API_RETURN_STRING(result);
}

API_FUNC(charset_set)
{
    Tcl_Obj *objp;
    int i;

    API_INIT_FUNC(1, "charset_set", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_charset_set (tcl_current_script,
                                   Tcl_GetStringFromObj (objv[1], &i)); /* charset */

    API_RETURN_OK;
}

API_FUNC(iconv_to_internal)
{
    Tcl_Obj *objp;
    char *result, *charset, *string;
    int i;

    API_INIT_FUNC(1, "iconv_to_internal", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    charset = Tcl_GetStringFromObj (objv[1], &i);
    string = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_iconv_to_internal (charset, string);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(iconv_from_internal)
{
    Tcl_Obj *objp;
    char *result, *charset, *string;
    int i;

    API_INIT_FUNC(1, "iconv_from_internal", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    charset = Tcl_GetStringFromObj (objv[1], &i);
    string = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_iconv_from_internal (charset, string);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(gettext)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(1, "gettext", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_gettext (Tcl_GetStringFromObj (objv[1], &i)); /* string */

    API_RETURN_STRING(result);
}

API_FUNC(ngettext)
{
    Tcl_Obj *objp;
    char *single, *plural;
    const char *result;
    int i, count;

    API_INIT_FUNC(1, "ngettext", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    single = Tcl_GetStringFromObj (objv[1], &i);
    plural = Tcl_GetStringFromObj (objv[2], &i);

    if (Tcl_GetIntFromObj (interp, objv[3], &count) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_ngettext (single, plural, count);

    API_RETURN_STRING(result);
}

API_FUNC(strlen_screen)
{
    Tcl_Obj *objp;
    char *string;
    int result, i;

    API_INIT_FUNC(1, "strlen_screen", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = Tcl_GetStringFromObj (objv[1], &i);

    result = weechat_strlen_screen (string);

    API_RETURN_INT(result);
}

API_FUNC(string_match)
{
    Tcl_Obj *objp;
    char *string, *mask;
    int case_sensitive, result, i;

    API_INIT_FUNC(1, "string_match", API_RETURN_INT(0));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = Tcl_GetStringFromObj (objv[1], &i);
    mask = Tcl_GetStringFromObj (objv[2], &i);

    if (Tcl_GetIntFromObj (interp, objv[3], &case_sensitive) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_string_match (string, mask, case_sensitive);

    API_RETURN_INT(result);
}

API_FUNC(string_match_list)
{
    Tcl_Obj *objp;
    char *string, *masks;
    int case_sensitive, result, i;

    API_INIT_FUNC(1, "string_match_list", API_RETURN_INT(0));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = Tcl_GetStringFromObj (objv[1], &i);
    masks = Tcl_GetStringFromObj (objv[2], &i);

    if (Tcl_GetIntFromObj (interp, objv[3], &case_sensitive) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = plugin_script_api_string_match_list (weechat_tcl_plugin,
                                                  string,
                                                  masks,
                                                  case_sensitive);

    API_RETURN_INT(result);
}

API_FUNC(string_has_highlight)
{
    Tcl_Obj *objp;
    char *string, *highlight_words;
    int result, i;

    API_INIT_FUNC(1, "string_has_highlight", API_RETURN_INT(0));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = Tcl_GetStringFromObj (objv[1], &i);
    highlight_words = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_string_has_highlight (string, highlight_words);

    API_RETURN_INT(result);
}

API_FUNC(string_has_highlight_regex)
{
    Tcl_Obj *objp;
    char *string, *regex;
    int result, i;

    API_INIT_FUNC(1, "string_has_highlight_regex", API_RETURN_INT(0));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = Tcl_GetStringFromObj (objv[1], &i);
    regex = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_string_has_highlight_regex (string, regex);

    API_RETURN_INT(result);
}

API_FUNC(string_mask_to_regex)
{
    Tcl_Obj *objp;
    char *result, *mask;
    int i;

    API_INIT_FUNC(1, "string_mask_to_regex", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    mask = Tcl_GetStringFromObj (objv[1], &i);

    result = weechat_string_mask_to_regex (mask);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(string_format_size)
{
    Tcl_Obj *objp;
    char *result;
    long size;

    API_INIT_FUNC(1, "string_format_size", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if (Tcl_GetLongFromObj (interp, objv[1], &size) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_string_format_size ((unsigned long long)size);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(string_parse_size)
{
    Tcl_Obj *objp;
    char *size;
    unsigned long long value;
    int i;

    API_INIT_FUNC(1, "string_parse_size", API_RETURN_LONG(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_LONG(0));

    size = Tcl_GetStringFromObj (objv[1], &i);

    value = weechat_string_parse_size (size);

    API_RETURN_LONG(value);
}

API_FUNC(string_color_code_size)
{
    Tcl_Obj *objp;
    int result, i;

    API_INIT_FUNC(1, "string_color_code_size", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_string_color_code_size (Tcl_GetStringFromObj (objv[1], &i)); /* string */

    API_RETURN_INT(result);
}

API_FUNC(string_remove_color)
{
    Tcl_Obj *objp;
    char *result, *replacement, *string;
    int i;

    API_INIT_FUNC(1, "string_remove_color", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    string = Tcl_GetStringFromObj (objv[1], &i);
    replacement = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_string_remove_color (string, replacement);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(string_is_command_char)
{
    Tcl_Obj *objp;
    int result, i;

    API_INIT_FUNC(1, "string_is_command_char", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_string_is_command_char (Tcl_GetStringFromObj (objv[1], &i)); /* string */

    API_RETURN_INT(result);
}

API_FUNC(string_input_for_buffer)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(1, "string_input_for_buffer", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_string_input_for_buffer (Tcl_GetStringFromObj (objv[1], &i));

    API_RETURN_STRING(result);
}

API_FUNC(string_eval_expression)
{
    Tcl_Obj *objp;
    char *expr, *result;
    struct t_hashtable *pointers, *extra_vars, *options;
    int i;

    API_INIT_FUNC(1, "string_eval_expression", API_RETURN_EMPTY);
    if (objc < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    expr = Tcl_GetStringFromObj (objv[1], &i);
    pointers = weechat_tcl_dict_to_hashtable (interp, objv[2],
                                              WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                              WEECHAT_HASHTABLE_STRING,
                                              WEECHAT_HASHTABLE_POINTER);
    extra_vars = weechat_tcl_dict_to_hashtable (interp, objv[3],
                                                WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                WEECHAT_HASHTABLE_STRING,
                                                WEECHAT_HASHTABLE_STRING);
    options = weechat_tcl_dict_to_hashtable (interp, objv[4],
                                             WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                             WEECHAT_HASHTABLE_STRING,
                                             WEECHAT_HASHTABLE_STRING);

    result = weechat_string_eval_expression (expr, pointers, extra_vars,
                                             options);

    if (pointers)
        weechat_hashtable_free (pointers);
    if (extra_vars)
        weechat_hashtable_free (extra_vars);
    if (options)
        weechat_hashtable_free (options);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(string_eval_path_home)
{
    Tcl_Obj *objp;
    char *path, *result;
    struct t_hashtable *pointers, *extra_vars, *options;
    int i;

    API_INIT_FUNC(1, "string_eval_path_home", API_RETURN_EMPTY);
    if (objc < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    path = Tcl_GetStringFromObj (objv[1], &i);
    pointers = weechat_tcl_dict_to_hashtable (
        interp, objv[2],
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_POINTER);
    extra_vars = weechat_tcl_dict_to_hashtable (
        interp, objv[3],
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    options = weechat_tcl_dict_to_hashtable (
        interp, objv[4],
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);

    result = weechat_string_eval_path_home (path, pointers, extra_vars,
                                            options);

    if (pointers)
        weechat_hashtable_free (pointers);
    if (extra_vars)
        weechat_hashtable_free (extra_vars);
    if (options)
        weechat_hashtable_free (options);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(mkdir_home)
{
    Tcl_Obj *objp;
    int i, mode;

    API_INIT_FUNC(1, "mkdir_home", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (Tcl_GetIntFromObj (interp, objv[2], &mode) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (weechat_mkdir_home (Tcl_GetStringFromObj (objv[1], &i), /* directory */
                            mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

API_FUNC(mkdir)
{
    Tcl_Obj *objp;
    int i, mode;

    API_INIT_FUNC(1, "mkdir", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (Tcl_GetIntFromObj (interp, objv[2], &mode) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (weechat_mkdir (Tcl_GetStringFromObj (objv[1], &i), /* directory */
                       mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

API_FUNC(mkdir_parents)
{
    Tcl_Obj *objp;
    int i, mode;

    API_INIT_FUNC(1, "mkdir_parents", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (Tcl_GetIntFromObj (interp, objv[2], &mode) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (weechat_mkdir_parents (Tcl_GetStringFromObj (objv[1], &i), /* directory */
                               mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

API_FUNC(list_new)
{
    Tcl_Obj *objp;
    const char *result;

    /* make C compiler happy */
    (void) clientData;
    (void) objc;
    (void) objv;

    API_INIT_FUNC(1, "list_new", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_new ());

    API_RETURN_STRING(result);
}

API_FUNC(list_add)
{
    Tcl_Obj *objp;
    char *weelist, *data, *where, *user_data;
    const char *result;
    int i;


    API_INIT_FUNC(1, "list_add", API_RETURN_EMPTY);
    if (objc < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    weelist = Tcl_GetStringFromObj (objv[1], &i);
    data = Tcl_GetStringFromObj (objv[2], &i);
    where = Tcl_GetStringFromObj (objv[3], &i);
    user_data = Tcl_GetStringFromObj (objv[4], &i);

    result = API_PTR2STR(weechat_list_add (API_STR2PTR(weelist),
                                           data,
                                           where,
                                           API_STR2PTR(user_data)));

    API_RETURN_STRING(result);
}

API_FUNC(list_search)
{
    Tcl_Obj *objp;
    char *weelist, *data;
    const char *result;
    int i;

    API_INIT_FUNC(1, "list_search", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    weelist = Tcl_GetStringFromObj (objv[1], &i);
    data = Tcl_GetStringFromObj (objv[2], &i);

    result = API_PTR2STR(weechat_list_search (API_STR2PTR(weelist),
                                              data));

    API_RETURN_STRING(result);
}

API_FUNC(list_search_pos)
{
    Tcl_Obj *objp;
    char *weelist, *data;
    int i, pos;

    API_INIT_FUNC(1, "list_search_pos", API_RETURN_INT(-1));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    weelist = Tcl_GetStringFromObj (objv[1], &i);
    data = Tcl_GetStringFromObj (objv[2], &i);

    pos = weechat_list_search_pos (API_STR2PTR(weelist), data);

    API_RETURN_INT(pos);
}

API_FUNC(list_casesearch)
{
    Tcl_Obj *objp;
    char *weelist, *data;
    const char *result;
    int i;

    API_INIT_FUNC(1, "list_casesearch", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    weelist = Tcl_GetStringFromObj (objv[1], &i);
    data = Tcl_GetStringFromObj (objv[2], &i);

    result = API_PTR2STR(weechat_list_casesearch (API_STR2PTR(weelist),
                                                  data));

    API_RETURN_STRING(result);
}

API_FUNC(list_casesearch_pos)
{
    Tcl_Obj *objp;
    char *weelist, *data;
    int i, pos;

    API_INIT_FUNC(1, "list_casesearch_pos", API_RETURN_INT(-1));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    weelist = Tcl_GetStringFromObj (objv[1], &i);
    data = Tcl_GetStringFromObj (objv[2], &i);

    pos = weechat_list_casesearch_pos (API_STR2PTR(weelist), data);

    API_RETURN_INT(pos);
}

API_FUNC(list_get)
{
    Tcl_Obj *objp;
    const char *result;
    int i, position;

    API_INIT_FUNC(1, "list_get", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if (Tcl_GetIntFromObj (interp, objv[2], &position) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_get (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)), /* weelist */
                                           position));

    API_RETURN_STRING(result);
}

API_FUNC(list_set)
{
    Tcl_Obj *objp;
    char *item, *new_value;
    int i;

    API_INIT_FUNC(1, "list_set", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    item = Tcl_GetStringFromObj (objv[1], &i);
    new_value = Tcl_GetStringFromObj (objv[2], &i);

    weechat_list_set (API_STR2PTR(item), new_value);

    API_RETURN_OK;
}

API_FUNC(list_next)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(1, "list_next", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_next (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)))); /* item */

    API_RETURN_STRING(result);
}

API_FUNC(list_prev)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(1, "list_prev", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_prev (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)))); /* item */

    API_RETURN_STRING(result);
}

API_FUNC(list_string)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(1, "list_string", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_list_string (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* item */

    API_RETURN_STRING(result);
}

API_FUNC(list_size)
{
    Tcl_Obj *objp;
    int size;
    int i;

    API_INIT_FUNC(1, "list_size", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    size = weechat_list_size (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* weelist */

    API_RETURN_INT(size);
}

API_FUNC(list_remove)
{
    Tcl_Obj *objp;
    char *weelist, *item;
    int i;

    API_INIT_FUNC(1, "list_remove", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weelist = Tcl_GetStringFromObj (objv[1], &i);
    item = Tcl_GetStringFromObj (objv[2], &i);

    weechat_list_remove (API_STR2PTR(weelist),
                         API_STR2PTR(item));

    API_RETURN_OK;
}

API_FUNC(list_remove_all)
{
    Tcl_Obj *objp;
    int i;

    (void) clientData;

    API_INIT_FUNC(1, "list_remove_all", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_list_remove_all (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* weelist */

    API_RETURN_OK;
}

API_FUNC(list_free)
{
    Tcl_Obj *objp;
    int i;

    API_INIT_FUNC(1, "list_free", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_list_free (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* weelist */

    API_RETURN_OK;
}

int
weechat_tcl_api_config_reload_cb (const void *pointer, void *data,
                                  struct t_config_file *config_file)
{
    struct t_plugin_script *script;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);


    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(config_file);

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
                                       "ss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
}

API_FUNC(config_new)
{
    Tcl_Obj *objp;
    char *name, *function, *data;
    const char *result;
    int i;

    API_INIT_FUNC(1, "config_new", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    data = Tcl_GetStringFromObj (objv[3], &i);

    result = API_PTR2STR(plugin_script_api_config_new (weechat_tcl_plugin,
                                                       tcl_current_script,
                                                       name,
                                                       &weechat_tcl_api_config_reload_cb,
                                                       function,
                                                       data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_tcl_api_config_update_cb (const void *pointer, void *data,
                                  struct t_config_file *config_file,
                                  int version_read,
                                  struct t_hashtable *data_read)
{
    struct t_plugin_script *script;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    struct t_hashtable *ret_hashtable;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);


    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(config_file);
        func_argv[2] = &version_read;
        func_argv[3] = data_read;

        ret_hashtable = weechat_tcl_exec (script,
                                          WEECHAT_SCRIPT_EXEC_HASHTABLE,
                                          ptr_function,
                                          "ssih", func_argv);

        return ret_hashtable;
    }

    return NULL;
}

API_FUNC(config_set_version)
{
    Tcl_Obj *objp;
    char *config_file, *function, *data;
    int i, rc, version;

    API_INIT_FUNC(1, "config_set_version", API_RETURN_INT(0));
    if (objc < 5)
        API_WRONG_ARGS(API_RETURN_INT(0));

    if (Tcl_GetIntFromObj (interp, objv[2], &version) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[3], &i);
    data = Tcl_GetStringFromObj (objv[4], &i);

    rc = plugin_script_api_config_set_version (
        weechat_tcl_plugin,
        tcl_current_script,
        API_STR2PTR(config_file),
        version,
        &weechat_tcl_api_config_update_cb,
        function,
        data);

    API_RETURN_INT(rc);
}

int
weechat_tcl_api_config_section_read_cb (const void *pointer, void *data,
                                        struct t_config_file *config_file,
                                        struct t_config_section *section,
                                        const char *option_name, const char *value)
{
    struct t_plugin_script *script;
    void *func_argv[5];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(config_file);
        func_argv[2] = (char *)API_PTR2STR(section);
        func_argv[3] = (option_name) ? (char *)option_name : empty_arg;
        func_argv[4] = (value) ? (char *)value : WEECHAT_NULL_STRING;

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
                                       "sssss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_SET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_CONFIG_OPTION_SET_ERROR;
}

int
weechat_tcl_api_config_section_write_cb (const void *pointer, void *data,
                                          struct t_config_file *config_file,
                                          const char *section_name)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(config_file);
        func_argv[2] = (section_name) ? (char *)section_name : empty_arg;

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
                                       "sss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_WRITE_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_CONFIG_WRITE_ERROR;
}

int
weechat_tcl_api_config_section_write_default_cb (const void *pointer, void *data,
                                                  struct t_config_file *config_file,
                                                  const char *section_name)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(config_file);
        func_argv[2] = (section_name) ? (char *)section_name : empty_arg;

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
                                       "sss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_WRITE_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_CONFIG_WRITE_ERROR;
}

int
weechat_tcl_api_config_section_create_option_cb (const void *pointer, void *data,
                                                  struct t_config_file *config_file,
                                                  struct t_config_section *section,
                                                  const char *option_name,
                                                  const char *value)
{
    struct t_plugin_script *script;
    void *func_argv[5];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(config_file);
        func_argv[2] = (char *)API_PTR2STR(section);
        func_argv[3] = (option_name) ? (char *)option_name : empty_arg;
        func_argv[4] = (value) ? (char *)value : WEECHAT_NULL_STRING;

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
                                       "sssss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_SET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_CONFIG_OPTION_SET_ERROR;
}

int
weechat_tcl_api_config_section_delete_option_cb (const void *pointer, void *data,
                                                 struct t_config_file *config_file,
                                                 struct t_config_section *section,
                                                 struct t_config_option *option)
{
    struct t_plugin_script *script;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(config_file);
        func_argv[2] = (char *)API_PTR2STR(section);
        func_argv[3] = (char *)API_PTR2STR(option);

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
                                       "ssss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_UNSET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_CONFIG_OPTION_UNSET_ERROR;
}

API_FUNC(config_new_section)
{
    Tcl_Obj *objp;
    char *config_file, *name, *function_read, *data_read;
    char *function_write, *data_write, *function_write_default;
    char *data_write_default, *function_create_option, *data_create_option;
    char *function_delete_option, *data_delete_option;
    const char *result;
    int i, can_add, can_delete;

    /* make C compiler happy */
    (void) clientData;

    API_INIT_FUNC(1, "config_new_section", API_RETURN_EMPTY);
    if (objc < 15)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if ((Tcl_GetIntFromObj (interp, objv[3], &can_add) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[4], &can_delete) != TCL_OK))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = Tcl_GetStringFromObj (objv[1], &i);
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

    result = API_PTR2STR(
        plugin_script_api_config_new_section (
            weechat_tcl_plugin,
            tcl_current_script,
            API_STR2PTR(config_file),
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

    API_RETURN_STRING(result);
}

API_FUNC(config_search_section)
{
    Tcl_Obj *objp;
    char *config_file, *section_name;
    const char *result;
    int i;

    API_INIT_FUNC(1, "config_search_section", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = Tcl_GetStringFromObj (objv[1], &i);
    section_name = Tcl_GetStringFromObj (objv[2], &i);

    result = API_PTR2STR(weechat_config_search_section (API_STR2PTR(config_file),
                                                        section_name));

    API_RETURN_STRING(result);
}


int
weechat_tcl_api_config_option_check_value_cb (const void *pointer, void *data,
                                               struct t_config_option *option,
                                               const char *value)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(option);
        func_argv[2] = (value) ? (char *)value : empty_arg;

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
                                       "sss", func_argv);

        if (!rc)
            ret = 0;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return 0;
}

void
weechat_tcl_api_config_option_change_cb (const void *pointer, void *data,
                                          struct t_config_option *option)
{
    struct t_plugin_script *script;
    void *func_argv[2], *rc;
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(option);

        rc = weechat_tcl_exec (script,
                               WEECHAT_SCRIPT_EXEC_IGNORE,
                               ptr_function,
                               "ss", func_argv);

        if (rc)
            free (rc);
    }
}

void
weechat_tcl_api_config_option_delete_cb (const void *pointer, void *data,
                                          struct t_config_option *option)
{
    struct t_plugin_script *script;
    void *func_argv[2], *rc;
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(option);

        rc = weechat_tcl_exec (script,
                               WEECHAT_SCRIPT_EXEC_IGNORE,
                               ptr_function,
                               "ss", func_argv);

        if (rc)
            free (rc);
    }
}

API_FUNC(config_new_option)
{
    Tcl_Obj *objp;
    char *config_file, *section, *name, *type;
    char *description, *string_values, *default_value, *value;
    char *function_check_value, *data_check_value, *function_change;
    char *data_change, *function_delete, *data_delete;
    const char *result;
    int i, min, max, null_value_allowed;

    API_INIT_FUNC(1, "config_new_option", API_RETURN_EMPTY);
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
    if (strcmp (default_value, WEECHAT_NULL_STRING) == 0)
        default_value = NULL;
    value = Tcl_GetStringFromObj (objv[10], &i);
    if (strcmp (value, WEECHAT_NULL_STRING) == 0)
        value = NULL;
    function_check_value = Tcl_GetStringFromObj (objv[12], &i);
    data_check_value = Tcl_GetStringFromObj (objv[13], &i);
    function_change = Tcl_GetStringFromObj (objv[14], &i);
    data_change = Tcl_GetStringFromObj (objv[15], &i);
    function_delete = Tcl_GetStringFromObj (objv[16], &i);
    data_delete = Tcl_GetStringFromObj (objv[17], &i);

    result = API_PTR2STR(plugin_script_api_config_new_option (weechat_tcl_plugin,
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

    API_RETURN_STRING(result);
}

API_FUNC(config_search_option)
{
    Tcl_Obj *objp;
    char *config_file, *section, *option_name;
    const char *result;
    int i;

    API_INIT_FUNC(1, "config_search_option", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = Tcl_GetStringFromObj (objv[1], &i);
    section = Tcl_GetStringFromObj (objv[2], &i);
    option_name = Tcl_GetStringFromObj (objv[3], &i);

    result = API_PTR2STR(weechat_config_search_option (API_STR2PTR(config_file),
                                                       API_STR2PTR(section),
                                                       option_name));

    API_RETURN_STRING(result);
}

API_FUNC(config_string_to_boolean)
{
    Tcl_Obj *objp;
    int result, i;

    API_INIT_FUNC(1, "config_string_to_boolean", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_config_string_to_boolean (Tcl_GetStringFromObj (objv[1], &i)); /* text */

    API_RETURN_INT(result);
}

API_FUNC(config_option_reset)
{
    Tcl_Obj *objp;
    int rc;
    char *option;
    int i, run_callback;

    API_INIT_FUNC(1, "config_option_reset", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    if (Tcl_GetIntFromObj (interp, objv[2], &run_callback) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    option = Tcl_GetStringFromObj (objv[1], &i);

    rc = weechat_config_option_reset (API_STR2PTR(option),
                                      run_callback);

    API_RETURN_INT(rc);
}

API_FUNC(config_option_set)
{
    Tcl_Obj *objp;
    int rc;
    char *option, *new_value;
    int i, run_callback;

    API_INIT_FUNC(1, "config_option_set", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
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

API_FUNC(config_option_set_null)
{
    Tcl_Obj *objp;
    int rc;
    char *option;
    int i, run_callback;

    API_INIT_FUNC(1, "config_option_set_null", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    if (Tcl_GetIntFromObj (interp, objv[2], &run_callback) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    option = Tcl_GetStringFromObj (objv[1], &i);

    rc = weechat_config_option_set_null (API_STR2PTR(option),
                                         run_callback);

    API_RETURN_INT(rc);
}

API_FUNC(config_option_unset)
{
    Tcl_Obj *objp;
    int rc;
    char *option;
    int i;

    API_INIT_FUNC(1, "config_option_unset", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    option = Tcl_GetStringFromObj (objv[1], &i);

    rc = weechat_config_option_unset (API_STR2PTR(option));

    API_RETURN_INT(rc);
}

API_FUNC(config_option_rename)
{
    Tcl_Obj *objp;
    char *option, *new_name;
    int i;

    API_INIT_FUNC(1, "config_option_rename", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    option = Tcl_GetStringFromObj (objv[1], &i);
    new_name = Tcl_GetStringFromObj (objv[2], &i);

    weechat_config_option_rename (API_STR2PTR(option),
                                  new_name);

    API_RETURN_OK;
}

API_FUNC(config_option_is_null)
{
    Tcl_Obj *objp;
    int result, i;

    API_INIT_FUNC(1, "config_option_is_null", API_RETURN_INT(1));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(1));

    result = weechat_config_option_is_null (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_INT(result);
}

API_FUNC(config_option_default_is_null)
{
    Tcl_Obj *objp;
    int result, i;

    API_INIT_FUNC(1, "config_option_default_is_null", API_RETURN_INT(1));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(1));

    result = weechat_config_option_default_is_null (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_INT(result);
}

API_FUNC(config_boolean)
{
    Tcl_Obj *objp;
    int result, i;

    API_INIT_FUNC(1, "config_boolean", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_config_boolean (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_INT(result);
}

API_FUNC(config_boolean_default)
{
    Tcl_Obj *objp;
    int result, i;

    API_INIT_FUNC(1, "config_boolean_default", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_config_boolean_default (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_INT(result);
}

API_FUNC(config_integer)
{
    Tcl_Obj *objp;
    int result, i;

    API_INIT_FUNC(1, "config_integer", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_config_integer (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_INT(result);
}

API_FUNC(config_integer_default)
{
    Tcl_Obj *objp;
    int result, i;

    API_INIT_FUNC(1, "config_integer_default", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_config_integer_default (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_INT(result);
}

API_FUNC(config_string)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(1, "config_string", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_config_string (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_STRING(result);
}

API_FUNC(config_string_default)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(1, "config_string_default", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_config_string_default (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_STRING(result);
}

API_FUNC(config_color)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(1, "config_color", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_config_color (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_STRING(result);
}

API_FUNC(config_color_default)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(1, "config_color_default", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_config_color_default (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_STRING(result);
}

API_FUNC(config_write_option)
{
    Tcl_Obj *objp;
    char *config_file, *option;
    int i;

    API_INIT_FUNC(1, "config_write_option", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    config_file = Tcl_GetStringFromObj (objv[1], &i);
    option = Tcl_GetStringFromObj (objv[2], &i);

    weechat_config_write_option (API_STR2PTR(config_file),
                                 API_STR2PTR(option));

    API_RETURN_OK;
}

API_FUNC(config_write_line)
{
    Tcl_Obj *objp;
    char *config_file, *option_name, *value;
    int i;

    API_INIT_FUNC(1, "config_write_line", API_RETURN_ERROR);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_ERROR);

    config_file = Tcl_GetStringFromObj (objv[1], &i);
    option_name = Tcl_GetStringFromObj (objv[2], &i);
    value = Tcl_GetStringFromObj (objv[3], &i);

    weechat_config_write_line (API_STR2PTR(config_file), option_name,
                               "%s", value);

    API_RETURN_OK;
}

API_FUNC(config_write)
{
    Tcl_Obj *objp;
    int rc;
    int i;

    API_INIT_FUNC(1, "config_write", API_RETURN_INT(WEECHAT_CONFIG_WRITE_ERROR));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_WRITE_ERROR));

    rc = weechat_config_write (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* config_file */

    API_RETURN_INT(rc);
}

API_FUNC(config_read)
{
    Tcl_Obj *objp;
    int rc;
    int i;

    API_INIT_FUNC(1, "config_read", API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));

    rc = weechat_config_read (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* config_file */

    API_RETURN_INT(rc);
}

API_FUNC(config_reload)
{
    Tcl_Obj *objp;
    int rc;
    int i;

    API_INIT_FUNC(1, "config_reload", API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));

    rc = weechat_config_reload (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* config_file */

    API_RETURN_INT(rc);
}

API_FUNC(config_option_free)
{
    Tcl_Obj *objp;
    int i;

    API_INIT_FUNC(1, "config_option_free", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_option_free (
        API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* option */

    API_RETURN_OK;
}

API_FUNC(config_section_free_options)
{
    Tcl_Obj *objp;
    int i;

    API_INIT_FUNC(1, "config_section_free_options", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_section_free_options (
        API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* section */

    API_RETURN_OK;
}

API_FUNC(config_section_free)
{
    Tcl_Obj *objp;
    int i;

    API_INIT_FUNC(1, "config_section_free", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_section_free (
        API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* section */

    API_RETURN_OK;
}

API_FUNC(config_free)
{
    Tcl_Obj *objp;
    int i;

    API_INIT_FUNC(1, "config_free", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_free (
        API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* config_file */

    API_RETURN_OK;
}

API_FUNC(config_get)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(1, "config_get", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_config_get (Tcl_GetStringFromObj (objv[1], &i)));

    API_RETURN_STRING(result);
}

API_FUNC(config_get_plugin)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(1, "config_get_plugin", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = plugin_script_api_config_get_plugin (weechat_tcl_plugin,
                                                  tcl_current_script,
                                                  Tcl_GetStringFromObj (objv[1], &i));

    API_RETURN_STRING(result);
}

API_FUNC(config_is_set_plugin)
{
    Tcl_Obj *objp;
    char *option;
    int i, rc;

    API_INIT_FUNC(1, "config_is_set_plugin", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = Tcl_GetStringFromObj (objv[1], &i);

    rc = plugin_script_api_config_is_set_plugin (weechat_tcl_plugin,
                                                 tcl_current_script,
                                                 option);

    API_RETURN_INT(rc);
}

API_FUNC(config_set_plugin)
{
    Tcl_Obj *objp;
    char *option, *value;
    int i, rc;

    API_INIT_FUNC(1, "config_set_plugin", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    option = Tcl_GetStringFromObj (objv[1], &i);
    value = Tcl_GetStringFromObj (objv[2], &i);

    rc = plugin_script_api_config_set_plugin (weechat_tcl_plugin,
                                              tcl_current_script,
                                              option,
                                              value);

    API_RETURN_INT(rc);
}

API_FUNC(config_set_desc_plugin)
{
    Tcl_Obj *objp;
    char *option, *description;
    int i;

    API_INIT_FUNC(1, "config_set_desc_plugin", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    option = Tcl_GetStringFromObj (objv[1], &i);
    description = Tcl_GetStringFromObj (objv[2], &i);

    plugin_script_api_config_set_desc_plugin (weechat_tcl_plugin,
                                              tcl_current_script,
                                              option,
                                              description);

    API_RETURN_OK;
}

API_FUNC(config_unset_plugin)
{
    Tcl_Obj *objp;
    char *option;
    int i, rc;

    API_INIT_FUNC(1, "config_unset_plugin", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    option = Tcl_GetStringFromObj (objv[1], &i);

    rc = plugin_script_api_config_unset_plugin (weechat_tcl_plugin,
                                                tcl_current_script,
                                                option);

    API_RETURN_INT(rc);
}

API_FUNC(key_bind)
{
    Tcl_Obj *objp;
    char *context;
    struct t_hashtable *hashtable;
    int i, num_keys;

    API_INIT_FUNC(1, "key_bind", API_RETURN_INT(0));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    context = Tcl_GetStringFromObj (objv[1], &i);
    hashtable = weechat_tcl_dict_to_hashtable (interp, objv[2],
                                               WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                               WEECHAT_HASHTABLE_STRING,
                                               WEECHAT_HASHTABLE_STRING);

    num_keys = weechat_key_bind (context, hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);

    API_RETURN_INT(num_keys);
}

API_FUNC(key_unbind)
{
    Tcl_Obj *objp;
    char *context, *key;
    int i, num_keys;

    API_INIT_FUNC(1, "key_unbind", API_RETURN_INT(0));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    context = Tcl_GetStringFromObj (objv[1], &i);
    key = Tcl_GetStringFromObj (objv[2], &i);

    num_keys = weechat_key_unbind (context, key);

    API_RETURN_INT(num_keys);
}

API_FUNC(prefix)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(0, "prefix", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_prefix (Tcl_GetStringFromObj (objv[1], &i)); /* prefix */

    API_RETURN_STRING(result);
}

API_FUNC(color)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(0, "color", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_color (Tcl_GetStringFromObj (objv[1], &i)); /* color */

    API_RETURN_STRING(result);
}

API_FUNC(print)
{
    Tcl_Obj *objp;
    char *buffer, *message;
    int i;

    API_INIT_FUNC(0, "print", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    message = Tcl_GetStringFromObj (objv[2], &i);

    plugin_script_api_printf (weechat_tcl_plugin,
                              tcl_current_script,
                              API_STR2PTR(buffer),
                              "%s", message);

    API_RETURN_OK;
}

API_FUNC(print_date_tags)
{
    Tcl_Obj *objp;
    char *buffer, *tags, *message;
    int i;
    long date;

    API_INIT_FUNC(1, "print_date_tags", API_RETURN_ERROR);
    if (objc < 5)
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (Tcl_GetLongFromObj (interp, objv[2], &date) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    tags = Tcl_GetStringFromObj (objv[3], &i);
    message = Tcl_GetStringFromObj (objv[4], &i);

    plugin_script_api_printf_date_tags (weechat_tcl_plugin,
                                        tcl_current_script,
                                        API_STR2PTR(buffer),
                                        (time_t)date,
                                        tags,
                                        "%s", message);

    API_RETURN_OK;
}

API_FUNC(print_y)
{
    Tcl_Obj *objp;
    char *buffer, *message;
    int i, y;

    API_INIT_FUNC(1, "print_y", API_RETURN_ERROR);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (Tcl_GetIntFromObj (interp, objv[2], &y) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    message = Tcl_GetStringFromObj (objv[3], &i);

    plugin_script_api_printf_y (weechat_tcl_plugin,
                                tcl_current_script,
                                API_STR2PTR(buffer),
                                y,
                                "%s", message);

    API_RETURN_OK;
}

API_FUNC(print_y_date_tags)
{
    Tcl_Obj *objp;
    char *buffer, *tags, *message;
    int i, y;
    long date;

    API_INIT_FUNC(1, "print_y_date_tags", API_RETURN_ERROR);
    if (objc < 6)
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (Tcl_GetIntFromObj (interp, objv[2], &y) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (Tcl_GetLongFromObj (interp, objv[3], &date) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    tags = Tcl_GetStringFromObj (objv[4], &i);
    message = Tcl_GetStringFromObj (objv[5], &i);

    plugin_script_api_printf_y_date_tags (weechat_tcl_plugin,
                                          tcl_current_script,
                                          API_STR2PTR(buffer),
                                          y,
                                          (time_t)date,
                                          tags,
                                          "%s", message);

    API_RETURN_OK;
}

API_FUNC(log_print)
{
    Tcl_Obj *objp;
    int i;

    /* make C compiler happy */
    (void) clientData;

    API_INIT_FUNC(1, "log_print", API_RETURN_ERROR);

    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_log_printf (weechat_tcl_plugin,
                                  tcl_current_script,
                                  "%s", Tcl_GetStringFromObj (objv[1], &i)); /* message */

    API_RETURN_OK;
}

int
weechat_tcl_api_hook_command_cb (const void *pointer, void *data,
                                 struct t_gui_buffer *buffer,
                                 int argc, char **argv, char **argv_eol)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    /* make C compiler happy */
    (void) argv;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(buffer);
        func_argv[2] = (argc > 1) ? argv_eol[1] : empty_arg;

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
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

API_FUNC(hook_command)
{
    Tcl_Obj *objp;
    char *command, *description, *args, *args_description;
    char *completion, *function, *data;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hook_command", API_RETURN_EMPTY);
    if (objc < 8)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = Tcl_GetStringFromObj (objv[1], &i);
    description = Tcl_GetStringFromObj (objv[2], &i);
    args = Tcl_GetStringFromObj (objv[3], &i);
    args_description = Tcl_GetStringFromObj (objv[4], &i);
    completion = Tcl_GetStringFromObj (objv[5], &i);
    function = Tcl_GetStringFromObj (objv[6], &i);
    data = Tcl_GetStringFromObj (objv[7], &i);

    result = API_PTR2STR(plugin_script_api_hook_command (weechat_tcl_plugin,
                                                         tcl_current_script,
                                                         command,
                                                         description,
                                                         args,
                                                         args_description,
                                                         completion,
                                                         &weechat_tcl_api_hook_command_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING(result);
}

int
weechat_tcl_api_hook_completion_cb (const void *pointer, void *data,
                                    const char *completion_item,
                                     struct t_gui_buffer *buffer,
                                     struct t_gui_completion *completion)
{
    struct t_plugin_script *script;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (completion_item) ? (char *)completion_item : empty_arg;
        func_argv[2] = (char *)API_PTR2STR(buffer);
        func_argv[3] = (char *)API_PTR2STR(completion);

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
                                       "ssss", func_argv);

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

API_FUNC(hook_completion)
{
    Tcl_Obj *objp;
    char *completion, *description, *function, *data;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hook_completion", API_RETURN_EMPTY);
    if (objc < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    completion = Tcl_GetStringFromObj (objv[1], &i);
    description = Tcl_GetStringFromObj (objv[2], &i);
    function = Tcl_GetStringFromObj (objv[3], &i);
    data = Tcl_GetStringFromObj (objv[4], &i);

    result = API_PTR2STR(plugin_script_api_hook_completion (weechat_tcl_plugin,
                                                            tcl_current_script,
                                                            completion,
                                                            description,
                                                            &weechat_tcl_api_hook_completion_cb,
                                                            function,
                                                            data));

    API_RETURN_STRING(result);
}

/*
 * This function deprecated since WeeChat 2.9, kept for compatibility.
 * It is replaced by completion_get_string.
 */

API_FUNC(hook_completion_get_string)
{
    Tcl_Obj *objp;
    char *completion, *property;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hook_completion_get_string", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    completion = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_hook_completion_get_string (API_STR2PTR(completion),
                                                 property);

    API_RETURN_STRING(result);
}

/*
 * This function deprecated since WeeChat 2.9, kept for compatibility.
 * It is replaced by completion_list_add.
 */

API_FUNC(hook_completion_list_add)
{
    Tcl_Obj *objp;
    char *completion, *word, *where;
    int i, nick_completion;

    API_INIT_FUNC(1, "hook_completion_list_add", API_RETURN_ERROR);
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

int
weechat_tcl_api_hook_command_run_cb (const void *pointer, void *data,
                                     struct t_gui_buffer *buffer,
                                     const char *command)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(buffer);
        func_argv[2] = (command) ? (char *)command : empty_arg;

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
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

API_FUNC(hook_command_run)
{
    Tcl_Obj *objp;
    char *command, *function, *data;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hook_command_run", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    data = Tcl_GetStringFromObj (objv[3], &i);

    result = API_PTR2STR(plugin_script_api_hook_command_run (weechat_tcl_plugin,
                                                             tcl_current_script,
                                                             command,
                                                             &weechat_tcl_api_hook_command_run_cb,
                                                             function,
                                                             data));

    API_RETURN_STRING(result);
}

int
weechat_tcl_api_hook_timer_cb (const void *pointer, void *data,
                               int remaining_calls)
{
    struct t_plugin_script *script;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = &remaining_calls;

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
                                       "si", func_argv);

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

API_FUNC(hook_timer)
{
    Tcl_Obj *objp;
    const char *result;
    long interval;
    int i, align_second, max_calls;

    API_INIT_FUNC(1, "hook_timer", API_RETURN_EMPTY);
    if (objc < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if ((Tcl_GetLongFromObj (interp, objv[1], &interval) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[2], &align_second) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[3], &max_calls) != TCL_OK))
        API_WRONG_ARGS(API_RETURN_EMPTY);


    result = API_PTR2STR(plugin_script_api_hook_timer (weechat_tcl_plugin,
                                                       tcl_current_script,
                                                       interval, /* interval */
                                                       align_second, /* align_second */
                                                       max_calls, /* max_calls */
                                                       &weechat_tcl_api_hook_timer_cb,
                                                       Tcl_GetStringFromObj (objv[4], &i), /* tcl function */
                                                       Tcl_GetStringFromObj (objv[5], &i))); /* data */

    API_RETURN_STRING(result);
}

int
weechat_tcl_api_hook_fd_cb (const void *pointer, void *data, int fd)
{
    struct t_plugin_script *script;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = &fd;

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
                                       "si", func_argv);

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

API_FUNC(hook_fd)
{
    Tcl_Obj *objp;
    const char *result;
    int i, fd, read, write, exception;

    API_INIT_FUNC(1, "hook_fd", API_RETURN_EMPTY);
    if (objc < 7)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if ((Tcl_GetIntFromObj (interp, objv[1], &fd) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[2], &read) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[3], &write) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[4], &exception) != TCL_OK))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_fd (weechat_tcl_plugin,
                                                    tcl_current_script,
                                                    fd, /* fd */
                                                    read, /* read */
                                                    write, /* write */
                                                    exception, /* exception */
                                                    &weechat_tcl_api_hook_fd_cb,
                                                    Tcl_GetStringFromObj (objv[5], &i), /* tcl function */
                                                    Tcl_GetStringFromObj (objv[6], &i))); /* data */

    API_RETURN_STRING(result);
}

int
weechat_tcl_api_hook_process_cb (const void *pointer, void *data,
                                 const char *command, int return_code,
                                 const char *out, const char *err)
{
    struct t_plugin_script *script;
    void *func_argv[5];
    char empty_arg[1] = { '\0' }, *result;
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (return_code == WEECHAT_HOOK_PROCESS_CHILD)
    {
        if (strncmp (command, "func:", 5) == 0)
        {
            func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;

            result = (char *) weechat_tcl_exec (script,
                                                WEECHAT_SCRIPT_EXEC_STRING,
                                                command + 5,
                                                "s", func_argv);
            if (result)
            {
                printf ("%s", result);
                free (result);
                return 0;
            }
        }
        return 1;
    }
    else if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (command) ? (char *)command : empty_arg;
        func_argv[2] = &return_code;
        func_argv[3] = (out) ? (char *)out : empty_arg;
        func_argv[4] = (err) ? (char *)err : empty_arg;

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
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

API_FUNC(hook_process)
{
    Tcl_Obj *objp;
    char *command, *function, *data;
    const char *result;
    int i, timeout;

    API_INIT_FUNC(1, "hook_process", API_RETURN_EMPTY);
    if (objc < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if ((Tcl_GetIntFromObj (interp, objv[2], &timeout) != TCL_OK))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[3], &i);
    data = Tcl_GetStringFromObj (objv[4], &i);

    result = API_PTR2STR(plugin_script_api_hook_process (weechat_tcl_plugin,
                                                         tcl_current_script,
                                                         command,
                                                         timeout,
                                                         &weechat_tcl_api_hook_process_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_process_hashtable)
{
    Tcl_Obj *objp;
    char *command, *function, *data;
    const char *result;
    struct t_hashtable *options;
    int i, timeout;

    API_INIT_FUNC(1, "hook_process_hashtable", API_RETURN_EMPTY);
    if (objc < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if ((Tcl_GetIntFromObj (interp, objv[3], &timeout) != TCL_OK))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = Tcl_GetStringFromObj (objv[1], &i);
    options = weechat_tcl_dict_to_hashtable (interp, objv[2],
                                             WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                             WEECHAT_HASHTABLE_STRING,
                                             WEECHAT_HASHTABLE_STRING);
    function = Tcl_GetStringFromObj (objv[4], &i);
    data = Tcl_GetStringFromObj (objv[5], &i);

    result = API_PTR2STR(plugin_script_api_hook_process_hashtable (weechat_tcl_plugin,
                                                                   tcl_current_script,
                                                                   command,
                                                                   options,
                                                                   timeout,
                                                                   &weechat_tcl_api_hook_process_cb,
                                                                   function,
                                                                   data));

    if (options)
        weechat_hashtable_free (options);

    API_RETURN_STRING(result);
}

int
weechat_tcl_api_hook_connect_cb (const void *pointer, void *data,
                                 int status, int gnutls_rc,
                                 int sock, const char *error,
                                 const char *ip_address)
{
    struct t_plugin_script *script;
    void *func_argv[6];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = &status;
        func_argv[2] = &gnutls_rc;
        func_argv[3] = &sock;
        func_argv[4] = (ip_address) ? (char *)ip_address : empty_arg;
        func_argv[5] = (error) ? (char *)error : empty_arg;

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
                                       "siiiss", func_argv);

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

API_FUNC(hook_connect)
{
    Tcl_Obj *objp;
    char *proxy, *address, *local_hostname, *function, *data;
    const char *result;
    int i, port, ipv6, retry;

    API_INIT_FUNC(1, "hook_connect", API_RETURN_EMPTY);
    if (objc < 9)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if ((Tcl_GetIntFromObj (interp, objv[3], &port) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[4], &ipv6) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[5], &retry) != TCL_OK))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    proxy = Tcl_GetStringFromObj (objv[1], &i);
    address = Tcl_GetStringFromObj (objv[2], &i);
    local_hostname = Tcl_GetStringFromObj (objv[6], &i);
    function = Tcl_GetStringFromObj (objv[7], &i);
    data = Tcl_GetStringFromObj (objv[8], &i);

    result = API_PTR2STR(plugin_script_api_hook_connect (weechat_tcl_plugin,
                                                         tcl_current_script,
                                                         proxy,
                                                         address,
                                                         port,
                                                         ipv6,
                                                         retry,
                                                         NULL, /* gnutls session */
                                                         NULL, /* gnutls callback */
                                                         0,    /* gnutls DH key size */
                                                         NULL, /* gnutls priorities */
                                                         local_hostname,
                                                         &weechat_tcl_api_hook_connect_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_tcl_api_hook_line_cb (const void *pointer, void *data,
                              struct t_hashtable *line)
{
    struct t_plugin_script *script;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = line;

        return (struct t_hashtable *)weechat_tcl_exec (
            script,
            WEECHAT_SCRIPT_EXEC_HASHTABLE,
            ptr_function,
            "sh", func_argv);
    }

    return NULL;
}

API_FUNC(hook_line)
{
    Tcl_Obj *objp;
    char *buffer_type, *buffer_name, *tags, *function, *data;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hook_line", API_RETURN_EMPTY);
    if (objc < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer_type = Tcl_GetStringFromObj (objv[1], &i);
    buffer_name = Tcl_GetStringFromObj (objv[2], &i);
    tags = Tcl_GetStringFromObj (objv[3], &i);
    function = Tcl_GetStringFromObj (objv[4], &i);
    data = Tcl_GetStringFromObj (objv[5], &i);

    result = API_PTR2STR(plugin_script_api_hook_line (weechat_tcl_plugin,
                                                      tcl_current_script,
                                                      buffer_type,
                                                      buffer_name,
                                                      tags,
                                                      &weechat_tcl_api_hook_line_cb,
                                                      function,
                                                      data));

    API_RETURN_STRING(result);
}

int
weechat_tcl_api_hook_print_cb (const void *pointer, void *data,
                               struct t_gui_buffer *buffer,
                               time_t date,
                               int tags_count, const char **tags,
                               int displayed, int highlight,
                               const char *prefix, const char *message)
{
    struct t_plugin_script *script;
    void *func_argv[8];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    static char timebuffer[64];
    int *rc, ret;

    /* make C compiler happy */
    (void) tags_count;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        snprintf (timebuffer, sizeof (timebuffer), "%lld", (long long)date);

        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(buffer);
        func_argv[2] = timebuffer;
        func_argv[3] = weechat_string_rebuild_split_string (tags, ",", 0, -1);
        if (!func_argv[3])
            func_argv[3] = strdup ("");
        func_argv[4] = &displayed;
        func_argv[5] = &highlight;
        func_argv[6] = (prefix) ? (char *)prefix : empty_arg;
        func_argv[7] = (message) ? (char *)message : empty_arg;

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
                                       "ssssiiss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[3])
            free (func_argv[3]);

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

API_FUNC(hook_print)
{
    Tcl_Obj *objp;
    char *buffer, *tags, *message, *function, *data;
    const char *result;
    int i, strip_colors;

    API_INIT_FUNC(1, "hook_print", API_RETURN_EMPTY);
    if (objc < 7)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if (Tcl_GetIntFromObj (interp, objv[4], &strip_colors) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    tags = Tcl_GetStringFromObj (objv[2], &i);
    message = Tcl_GetStringFromObj (objv[3], &i);
    function = Tcl_GetStringFromObj (objv[5], &i);
    data = Tcl_GetStringFromObj (objv[6], &i);

    result = API_PTR2STR(plugin_script_api_hook_print (weechat_tcl_plugin,
                                                       tcl_current_script,
                                                       API_STR2PTR(buffer),
                                                       tags,
                                                       message,
                                                       strip_colors, /* strip_colors */
                                                       &weechat_tcl_api_hook_print_cb,
                                                       function,
                                                       data));

    API_RETURN_STRING(result);
}

int
weechat_tcl_api_hook_signal_cb (const void *pointer, void *data,
                                const char *signal, const char *type_data,
                                 void *signal_data)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    static char str_value[64];
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (signal) ? (char *)signal : empty_arg;
        if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
        {
            func_argv[2] = (signal_data) ? (char *)signal_data : empty_arg;
        }
        else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
        {
            str_value[0] = '\0';
            if (signal_data)
            {
                snprintf (str_value, sizeof (str_value),
                          "%d", *((int *)signal_data));
            }
            func_argv[2] = str_value;
        }
        else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
        {
            func_argv[2] = (char *)API_PTR2STR(signal_data);
        }
        else
            func_argv[2] = empty_arg;

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
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

API_FUNC(hook_signal)
{
    Tcl_Obj *objp;
    char *signal, *function, *data;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hook_signal", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    signal = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    data = Tcl_GetStringFromObj (objv[3], &i);

    result = API_PTR2STR(plugin_script_api_hook_signal (weechat_tcl_plugin,
                                                        tcl_current_script,
                                                        signal,
                                                        &weechat_tcl_api_hook_signal_cb,
                                                        function,
                                                        data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_signal_send)
{
    Tcl_Obj *objp;
    char *signal, *type_data;
    int number, i, rc;

    API_INIT_FUNC(1, "hook_signal_send", API_RETURN_INT(WEECHAT_RC_ERROR));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    signal = Tcl_GetStringFromObj (objv[1], &i);
    type_data = Tcl_GetStringFromObj (objv[2], &i);
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        rc = weechat_hook_signal_send (signal,
                                       type_data,
                                       Tcl_GetStringFromObj (objv[3], &i)); /* signal_data */
        API_RETURN_INT(rc);
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        if (Tcl_GetIntFromObj (interp, objv[3], &number) != TCL_OK)
        {
            API_RETURN_INT(WEECHAT_RC_ERROR);
        }
        rc = weechat_hook_signal_send (signal,
                                       type_data,
                                       &number); /* signal_data */
        API_RETURN_INT(rc);
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        rc = weechat_hook_signal_send (signal,
                                       type_data,
                                       API_STR2PTR(Tcl_GetStringFromObj (objv[3], &i))); /* signal_data */
        API_RETURN_INT(rc);
    }

    API_RETURN_INT(WEECHAT_RC_ERROR);
}

int
weechat_tcl_api_hook_hsignal_cb (const void *pointer, void *data,
                                 const char *signal,
                                 struct t_hashtable *hashtable)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (signal) ? (char *)signal : empty_arg;
        func_argv[2] = hashtable;

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
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

API_FUNC(hook_hsignal)
{
    Tcl_Obj *objp;
    char *signal, *function, *data;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hook_hsignal", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    signal = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    data = Tcl_GetStringFromObj (objv[3], &i);

    result = API_PTR2STR(plugin_script_api_hook_hsignal (weechat_tcl_plugin,
                                                         tcl_current_script,
                                                         signal,
                                                         &weechat_tcl_api_hook_hsignal_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_hsignal_send)
{
    Tcl_Obj *objp;
    char *signal;
    struct t_hashtable *hashtable;
    int i, rc;

    API_INIT_FUNC(1, "hook_hsignal_send", API_RETURN_INT(WEECHAT_RC_ERROR));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    signal = Tcl_GetStringFromObj (objv[1], &i);
    hashtable = weechat_tcl_dict_to_hashtable (interp, objv[2],
                                               WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                               WEECHAT_HASHTABLE_STRING,
                                               WEECHAT_HASHTABLE_STRING);

    rc = weechat_hook_hsignal_send (signal, hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);

    API_RETURN_INT(rc);
}

int
weechat_tcl_api_hook_config_cb (const void *pointer, void *data,
                                const char *option, const char *value)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (option) ? (char *)option : empty_arg;
        func_argv[2] = (value) ? (char *)value : empty_arg;

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
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

API_FUNC(hook_config)
{
    Tcl_Obj *objp;
    char *option, *function, *data;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hook_config", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    data = Tcl_GetStringFromObj (objv[3], &i);

    result = API_PTR2STR(plugin_script_api_hook_config (weechat_tcl_plugin,
                                                        tcl_current_script,
                                                        option,
                                                        &weechat_tcl_api_hook_config_cb,
                                                        function,
                                                        data));

    API_RETURN_STRING(result);
}

char *
weechat_tcl_api_hook_modifier_cb (const void *pointer, void *data,
                                  const char *modifier,
                                  const char *modifier_data,
                                  const char *string)
{
    struct t_plugin_script *script;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (modifier) ? (char *)modifier : empty_arg;
        func_argv[2] = (modifier_data) ? (char *)modifier_data : empty_arg;
        func_argv[3] = (string) ? (char *)string : empty_arg;

        return (char *)weechat_tcl_exec (script,
                                         WEECHAT_SCRIPT_EXEC_STRING,
                                         ptr_function,
                                         "ssss", func_argv);
    }

    return NULL;
}

API_FUNC(hook_modifier)
{
    Tcl_Obj *objp;
    char *modifier, *function, *data;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hook_modifier", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    modifier = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    data = Tcl_GetStringFromObj (objv[3], &i);

    result = API_PTR2STR(plugin_script_api_hook_modifier (weechat_tcl_plugin,
                                                          tcl_current_script,
                                                          modifier,
                                                          &weechat_tcl_api_hook_modifier_cb,
                                                          function,
                                                          data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_modifier_exec)
{
    Tcl_Obj *objp;
    char *result, *modifier, *modifier_data, *string;
    int i;

    API_INIT_FUNC(1, "hook_modifier_exec", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    modifier = Tcl_GetStringFromObj (objv[1], &i);
    modifier_data = Tcl_GetStringFromObj (objv[2], &i);
    string = Tcl_GetStringFromObj (objv[3], &i);

    result = weechat_hook_modifier_exec (modifier, modifier_data, string);

    API_RETURN_STRING_FREE(result);
}

char *
weechat_tcl_api_hook_info_cb (const void *pointer, void *data,
                              const char *info_name,
                               const char *arguments)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (info_name) ? (char *)info_name : empty_arg;
        func_argv[2] = (arguments) ? (char *)arguments : empty_arg;

        return (char *)weechat_tcl_exec (script,
                                         WEECHAT_SCRIPT_EXEC_STRING,
                                         ptr_function,
                                         "sss", func_argv);
    }

    return NULL;
}

API_FUNC(hook_info)
{
    Tcl_Obj *objp;
    char *info_name, *description, *args_description, *function, *data;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hook_info", API_RETURN_EMPTY);
    if (objc < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    info_name = Tcl_GetStringFromObj (objv[1], &i);
    description = Tcl_GetStringFromObj (objv[2], &i);
    args_description = Tcl_GetStringFromObj (objv[3], &i);
    function = Tcl_GetStringFromObj (objv[4], &i);
    data = Tcl_GetStringFromObj (objv[5], &i);

    result = API_PTR2STR(plugin_script_api_hook_info (weechat_tcl_plugin,
                                                      tcl_current_script,
                                                      info_name,
                                                      description,
                                                      args_description,
                                                      &weechat_tcl_api_hook_info_cb,
                                                      function,
                                                      data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_tcl_api_hook_info_hashtable_cb (const void *pointer, void *data,
                                        const char *info_name,
                                        struct t_hashtable *hashtable)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (info_name) ? (char *)info_name : empty_arg;
        func_argv[2] = hashtable;

        return (struct t_hashtable *)weechat_tcl_exec (
            script,
            WEECHAT_SCRIPT_EXEC_HASHTABLE,
            ptr_function,
            "ssh", func_argv);
    }

    return NULL;
}

API_FUNC(hook_info_hashtable)
{
    Tcl_Obj *objp;
    char *info_name, *description, *args_description;
    char *output_description, *function, *data;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hook_info_hashtable", API_RETURN_EMPTY);
    if (objc < 7)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    info_name = Tcl_GetStringFromObj (objv[1], &i);
    description = Tcl_GetStringFromObj (objv[2], &i);
    args_description = Tcl_GetStringFromObj (objv[3], &i);
    output_description = Tcl_GetStringFromObj (objv[4], &i);
    function = Tcl_GetStringFromObj (objv[5], &i);
    data = Tcl_GetStringFromObj (objv[6], &i);

    result = API_PTR2STR(plugin_script_api_hook_info_hashtable (weechat_tcl_plugin,
                                                                tcl_current_script,
                                                                info_name,
                                                                description,
                                                                args_description,
                                                                output_description,
                                                                &weechat_tcl_api_hook_info_hashtable_cb,
                                                                function,
                                                                data));

    API_RETURN_STRING(result);
}

struct t_infolist *
weechat_tcl_api_hook_infolist_cb (const void *pointer, void *data,
                                  const char *infolist_name,
                                  void *obj_pointer, const char *arguments)
{
    struct t_plugin_script *script;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    struct t_infolist *result;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (infolist_name) ? (char *)infolist_name : empty_arg;
        func_argv[2] = (char *)API_PTR2STR(obj_pointer);
        func_argv[3] = (arguments) ? (char *)arguments : empty_arg;

        result = (struct t_infolist *)weechat_tcl_exec (
            script,
            WEECHAT_SCRIPT_EXEC_POINTER,
            ptr_function,
            "ssss", func_argv);

        return result;
    }

    return NULL;
}

API_FUNC(hook_infolist)
{
    Tcl_Obj *objp;
    char *infolist_name, *description, *pointer_description;
    char *args_description, *function, *data;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hook_infolist", API_RETURN_EMPTY);
    if (objc < 7)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist_name = Tcl_GetStringFromObj (objv[1], &i);
    description = Tcl_GetStringFromObj (objv[2], &i);
    pointer_description = Tcl_GetStringFromObj (objv[3], &i);
    args_description = Tcl_GetStringFromObj (objv[4], &i);
    function = Tcl_GetStringFromObj (objv[5], &i);
    data = Tcl_GetStringFromObj (objv[6], &i);

    result = API_PTR2STR(plugin_script_api_hook_infolist (weechat_tcl_plugin,
                                                          tcl_current_script,
                                                          infolist_name,
                                                          description,
                                                          pointer_description,
                                                          args_description,
                                                          &weechat_tcl_api_hook_infolist_cb,
                                                          function,
                                                          data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_tcl_api_hook_focus_cb (const void *pointer, void *data,
                               struct t_hashtable *info)
{
    struct t_plugin_script *script;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = info;

        return (struct t_hashtable *)weechat_tcl_exec (
            script,
            WEECHAT_SCRIPT_EXEC_HASHTABLE,
            ptr_function,
            "sh", func_argv);
    }

    return NULL;
}

API_FUNC(hook_focus)
{
    Tcl_Obj *objp;
    char *area, *function, *data;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hook_focus", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    area = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    data = Tcl_GetStringFromObj (objv[3], &i);

    result = API_PTR2STR(plugin_script_api_hook_focus (weechat_tcl_plugin,
                                                       tcl_current_script,
                                                       area,
                                                       &weechat_tcl_api_hook_focus_cb,
                                                       function,
                                                       data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_set)
{
    Tcl_Obj *objp;
    char *hook, *property, *value;
    int i;

    API_INIT_FUNC(1, "hook_set", API_RETURN_ERROR);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_ERROR);

    hook = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);
    value = Tcl_GetStringFromObj (objv[3], &i);

    weechat_hook_set (API_STR2PTR(hook), property, value);

    API_RETURN_OK;
}

API_FUNC(unhook)
{
    Tcl_Obj *objp;
    int i;

    API_INIT_FUNC(1, "unhook", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_unhook (
        API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* hook */

    API_RETURN_OK;
}

API_FUNC(unhook_all)
{
    Tcl_Obj *objp;

    /* make C compiler happy */
    (void) clientData;
    (void) objc;
    (void) objv;

    API_INIT_FUNC(1, "unhook_all", API_RETURN_ERROR);

    weechat_unhook_all (tcl_current_script->name);

    API_RETURN_OK;
}

int
weechat_tcl_api_buffer_input_data_cb (const void *pointer, void *data,
                                      struct t_gui_buffer *buffer,
                                       const char *input_data)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(buffer);
        func_argv[2] = (input_data) ? (char *)input_data : empty_arg;

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
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

int
weechat_tcl_api_buffer_close_cb (const void *pointer,
                                 void *data, struct t_gui_buffer *buffer)
{
    struct t_plugin_script *script;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(buffer);

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
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

API_FUNC(buffer_new)
{
    Tcl_Obj *objp;
    char *name, *function_input, *data_input, *function_close, *data_close;
    const char *result;
    int i;

    API_INIT_FUNC(1, "buffer_new", API_RETURN_EMPTY);
    if (objc < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = Tcl_GetStringFromObj (objv[1], &i);
    function_input = Tcl_GetStringFromObj (objv[2], &i);
    data_input = Tcl_GetStringFromObj (objv[3], &i);
    function_close = Tcl_GetStringFromObj (objv[4], &i);
    data_close = Tcl_GetStringFromObj (objv[5], &i);

    result = API_PTR2STR(plugin_script_api_buffer_new (weechat_tcl_plugin,
                                                       tcl_current_script,
                                                       name,
                                                       &weechat_tcl_api_buffer_input_data_cb,
                                                       function_input,
                                                       data_input,
                                                       &weechat_tcl_api_buffer_close_cb,
                                                       function_close,
                                                       data_close));

    API_RETURN_STRING(result);
}

API_FUNC(buffer_new_props)
{
    Tcl_Obj *objp;
    char *name, *function_input, *data_input, *function_close, *data_close;
    struct t_hashtable *properties;
    const char *result;
    int i;

    API_INIT_FUNC(1, "buffer_new_props", API_RETURN_EMPTY);
    if (objc < 7)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = Tcl_GetStringFromObj (objv[1], &i);
    properties = weechat_tcl_dict_to_hashtable (interp, objv[2],
                                                WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                WEECHAT_HASHTABLE_STRING,
                                                WEECHAT_HASHTABLE_STRING);
    function_input = Tcl_GetStringFromObj (objv[3], &i);
    data_input = Tcl_GetStringFromObj (objv[4], &i);
    function_close = Tcl_GetStringFromObj (objv[5], &i);
    data_close = Tcl_GetStringFromObj (objv[6], &i);

    result = API_PTR2STR(
        plugin_script_api_buffer_new_props (
            weechat_tcl_plugin,
            tcl_current_script,
            name,
            properties,
            &weechat_tcl_api_buffer_input_data_cb,
            function_input,
            data_input,
            &weechat_tcl_api_buffer_close_cb,
            function_close,
            data_close));

    if (properties)
        weechat_hashtable_free (properties);

    API_RETURN_STRING(result);
}

API_FUNC(buffer_search)
{
    Tcl_Obj *objp;
    char *plugin, *name;
    const char *result;
    int i;

    API_INIT_FUNC(1, "buffer_search", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    plugin = Tcl_GetStringFromObj (objv[1], &i);
    name = Tcl_GetStringFromObj (objv[2], &i);

    result = API_PTR2STR(weechat_buffer_search (plugin, name));

    API_RETURN_STRING(result);
}

API_FUNC(buffer_search_main)
{
    Tcl_Obj *objp;
    const char *result;

    /* make C compiler happy */
    (void) clientData;
    (void) objc;
    (void) objv;

    API_INIT_FUNC(1, "buffer_search_main", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_buffer_search_main ());

    API_RETURN_STRING(result);
}

API_FUNC(current_buffer)
{
    Tcl_Obj *objp;
    const char *result;

    /* make C compiler happy */
    (void) clientData;
    (void) objc;
    (void) objv;

    API_INIT_FUNC(1, "current_buffer", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_current_buffer ());

    API_RETURN_STRING(result);
}

API_FUNC(buffer_clear)
{
    Tcl_Obj *objp;
    int i;

    API_INIT_FUNC(1, "buffer_clear", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_clear (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* buffer */

    API_RETURN_OK;
}

API_FUNC(buffer_close)
{
    Tcl_Obj *objp;
    int i;

    API_INIT_FUNC(1, "buffer_close", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_close (
        API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* buffer */

    API_RETURN_OK;
}

API_FUNC(buffer_merge)
{
    Tcl_Obj *objp;
    int i;

    API_INIT_FUNC(1, "buffer_merge", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_merge (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)), /* buffer */
                          API_STR2PTR(Tcl_GetStringFromObj (objv[2], &i))); /* target_buffer */

    API_RETURN_OK;
}

API_FUNC(buffer_unmerge)
{
    Tcl_Obj *objp;
    int i, number;

    API_INIT_FUNC(1, "buffer_unmerge", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (Tcl_GetIntFromObj (interp, objv[2], &number) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_unmerge (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)),
                            number);

    API_RETURN_OK;
}

API_FUNC(buffer_get_integer)
{
    Tcl_Obj *objp;
    char *buffer, *property;
    int result;
    int i;

    API_INIT_FUNC(1, "buffer_get_integer", API_RETURN_INT(-1));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_buffer_get_integer (API_STR2PTR(buffer), property);

    API_RETURN_INT(result);
}

API_FUNC(buffer_get_string)
{
    Tcl_Obj *objp;
    char *buffer, *property;
    const char *result;
    int i;

    API_INIT_FUNC(1, "buffer_get_string", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_buffer_get_string (API_STR2PTR(buffer), property);

    API_RETURN_STRING(result);
}

API_FUNC(buffer_get_pointer)
{
    Tcl_Obj *objp;
    char *buffer, *property;
    const char *result;
    int i;

    API_INIT_FUNC(1, "buffer_get_pointer", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);

    result = API_PTR2STR(weechat_buffer_get_pointer (API_STR2PTR(buffer),
                                                     property));

    API_RETURN_STRING(result);
}

API_FUNC(buffer_set)
{
    Tcl_Obj *objp;
    char *buffer, *property, *value;
    int i;

    API_INIT_FUNC(1, "buffer_set", API_RETURN_ERROR);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);
    value = Tcl_GetStringFromObj (objv[3], &i);

    weechat_buffer_set (API_STR2PTR(buffer), property, value);

    API_RETURN_OK;
}

API_FUNC(buffer_string_replace_local_var)
{
    Tcl_Obj *objp;
    char *buffer, *string, *result;
    int i;

    API_INIT_FUNC(1, "buffer_string_replace_local_var", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    string = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_buffer_string_replace_local_var (API_STR2PTR(buffer), string);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(buffer_match_list)
{
    Tcl_Obj *objp;
    char *buffer, *string;
    int result;
    int i;

    API_INIT_FUNC(1, "buffer_match_list", API_RETURN_INT(0));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    string = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_buffer_match_list (API_STR2PTR(buffer), string);

    API_RETURN_INT(result);
}

API_FUNC(current_window)
{
    Tcl_Obj *objp;
    const char *result;

    /* make C compiler happy */
    (void) clientData;
    (void) objc;
    (void) objv;

    API_INIT_FUNC(1, "current_window", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_current_window ());

    API_RETURN_STRING(result);
}

API_FUNC(window_search_with_buffer)
{
    Tcl_Obj *objp;
    char *buffer;
    const char *result;
    int i;

    API_INIT_FUNC(1, "window_search_with_buffer", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);

    result = API_PTR2STR(weechat_window_search_with_buffer (API_STR2PTR(buffer)));

    API_RETURN_STRING(result);
}

API_FUNC(window_get_integer)
{
    Tcl_Obj *objp;
    char *window, *property;
    int result;
    int i;

    API_INIT_FUNC(1, "window_get_integer", API_RETURN_INT(-1));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    window = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_window_get_integer (API_STR2PTR(window), property);

    API_RETURN_INT(result);
}

API_FUNC(window_get_string)
{
    Tcl_Obj *objp;
    char *window, *property;
    const char *result;
    int i;

    API_INIT_FUNC(1, "window_get_string", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    window = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_window_get_string (API_STR2PTR(window), property);

    API_RETURN_STRING(result);
}

API_FUNC(window_get_pointer)
{
    Tcl_Obj *objp;
    char *window, *property;
    const char *result;
    int i;

    API_INIT_FUNC(1, "window_get_pointer", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    window = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);

    result = API_PTR2STR(weechat_window_get_pointer (API_STR2PTR(window),
                                                     property));

    API_RETURN_STRING(result);
}

API_FUNC(window_set_title)
{
    Tcl_Obj *objp;
    char *title;
    int i;

    API_INIT_FUNC(1, "window_set_title", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    title = Tcl_GetStringFromObj (objv[1], &i);

    weechat_window_set_title (title);

    API_RETURN_OK;
}

API_FUNC(nicklist_add_group)
{
    Tcl_Obj *objp;
    char *buffer, *parent_group, *name, *color;
    const char *result;
    int i, visible;

    API_INIT_FUNC(1, "nicklist_add_group", API_RETURN_EMPTY);
    if (objc < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if (Tcl_GetIntFromObj (interp, objv[5], &visible) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    parent_group = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);
    color = Tcl_GetStringFromObj (objv[4], &i);

    result = API_PTR2STR(weechat_nicklist_add_group (API_STR2PTR(buffer),
                                                     API_STR2PTR(parent_group),
                                                     name,
                                                     color,
                                                     visible)); /* visible */

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_search_group)
{
    Tcl_Obj *objp;
    char *buffer, *from_group, *name;
    const char *result;
    int i;

    API_INIT_FUNC(1, "nicklist_search_group", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    from_group = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);

    result = API_PTR2STR(weechat_nicklist_search_group (API_STR2PTR(buffer),
                                                        API_STR2PTR(from_group),
                                                        name));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_add_nick)
{
    Tcl_Obj *objp;
    char *buffer, *group, *name, *color, *prefix, *prefix_color;
    const char *result;
    int i, visible;

    API_INIT_FUNC(1, "nicklist_add_nick", API_RETURN_EMPTY);
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

    result = API_PTR2STR(weechat_nicklist_add_nick (API_STR2PTR(buffer),
                                                    API_STR2PTR(group),
                                                    name,
                                                    color,
                                                    prefix,
                                                    prefix_color,
                                                    visible)); /* visible */

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_search_nick)
{
    Tcl_Obj *objp;
    char *buffer, *from_group, *name;
    const char *result;
    int i;

    API_INIT_FUNC(1, "nicklist_search_nick", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    from_group = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);

    result = API_PTR2STR(weechat_nicklist_search_nick (API_STR2PTR(buffer),
                                                       API_STR2PTR(from_group),
                                                       name));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_remove_group)
{
    Tcl_Obj *objp;
    char *buffer, *group;
    int i;

    API_INIT_FUNC(1, "nicklist_remove_group", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    group = Tcl_GetStringFromObj (objv[2], &i);

    weechat_nicklist_remove_group (API_STR2PTR(buffer),
                                   API_STR2PTR(group));

    API_RETURN_OK;
}

API_FUNC(nicklist_remove_nick)
{
    Tcl_Obj *objp;
    char *buffer, *nick;
    int i;

    API_INIT_FUNC(1, "nicklist_remove_nick", API_RETURN_ERROR);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    nick = Tcl_GetStringFromObj (objv[2], &i);

    weechat_nicklist_remove_nick (API_STR2PTR(buffer),
                                  API_STR2PTR(nick));

    API_RETURN_OK;
}

API_FUNC(nicklist_remove_all)
{
    Tcl_Obj *objp;
    int i;

    API_INIT_FUNC(1, "nicklist_remove_all", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_nicklist_remove_all (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* buffer */

    API_RETURN_OK;
}

API_FUNC(nicklist_group_get_integer)
{
    Tcl_Obj *objp;
    char *buffer, *group, *property;
    int result;
    int i;

    API_INIT_FUNC(1, "nicklist_group_get_integer", API_RETURN_INT(-1));
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

API_FUNC(nicklist_group_get_string)
{
    Tcl_Obj *objp;
    char *buffer, *group, *property;
    const char *result;
    int i;

    API_INIT_FUNC(1, "nicklist_group_get_string", API_RETURN_EMPTY);
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

API_FUNC(nicklist_group_get_pointer)
{
    Tcl_Obj *objp;
    char *buffer, *group, *property;
    const char *result;
    int i;

    API_INIT_FUNC(1, "nicklist_group_get_pointer", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    group = Tcl_GetStringFromObj (objv[2], &i);
    property = Tcl_GetStringFromObj (objv[3], &i);

    result = API_PTR2STR(weechat_nicklist_group_get_pointer (API_STR2PTR(buffer),
                                                             API_STR2PTR(group),
                                                             property));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_group_set)
{
    Tcl_Obj *objp;
    char *buffer, *group, *property, *value;
    int i;

    API_INIT_FUNC(1, "nicklist_group_set", API_RETURN_ERROR);
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

API_FUNC(nicklist_nick_get_integer)
{
    Tcl_Obj *objp;
    char *buffer, *nick, *property;
    int result;
    int i;

    API_INIT_FUNC(1, "nicklist_nick_get_integer", API_RETURN_INT(-1));
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

API_FUNC(nicklist_nick_get_string)
{
    Tcl_Obj *objp;
    char *buffer, *nick, *property;
    const char *result;
    int i;

    API_INIT_FUNC(1, "nicklist_nick_get_string", API_RETURN_EMPTY);
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

API_FUNC(nicklist_nick_get_pointer)
{
    Tcl_Obj *objp;
    char *buffer, *nick, *property;
    const char *result;
    int i;

    API_INIT_FUNC(1, "nicklist_nick_get_pointer", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    nick = Tcl_GetStringFromObj (objv[2], &i);
    property = Tcl_GetStringFromObj (objv[3], &i);

    result = API_PTR2STR(weechat_nicklist_nick_get_pointer (API_STR2PTR(buffer),
                                                            API_STR2PTR(nick),
                                                            property));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_nick_set)
{
    Tcl_Obj *objp;
    char *buffer, *nick, *property, *value;
    int i;

    API_INIT_FUNC(1, "nicklist_nick_set", API_RETURN_ERROR);
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

API_FUNC(bar_item_search)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(1, "bar_item_search", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_bar_item_search (Tcl_GetStringFromObj (objv[1], &i))); /* name */

    API_RETURN_STRING(result);
}

char *
weechat_tcl_api_bar_item_build_cb (const void *pointer, void *data,
                                   struct t_gui_bar_item *item,
                                   struct t_gui_window *window,
                                   struct t_gui_buffer *buffer,
                                   struct t_hashtable *extra_info)
{
    struct t_plugin_script *script;
    void *func_argv[5];
    char empty_arg[1] = { '\0' }, *ret;
    const char *ptr_function, *ptr_data;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        if (strncmp (ptr_function, "(extra)", 7) == 0)
        {
            /* new callback: data, item, window, buffer, extra_info */
            func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
            func_argv[1] = (char *)API_PTR2STR(item);
            func_argv[2] = (char *)API_PTR2STR(window);
            func_argv[3] = (char *)API_PTR2STR(buffer);
            func_argv[4] = extra_info;

            ret = (char *)weechat_tcl_exec (script,
                                            WEECHAT_SCRIPT_EXEC_STRING,
                                            ptr_function + 7,
                                            "ssssh", func_argv);
        }
        else
        {
            /* old callback: data, item, window */
            func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
            func_argv[1] = (char *)API_PTR2STR(item);
            func_argv[2] = (char *)API_PTR2STR(window);

            ret = (char *)weechat_tcl_exec (script,
                                            WEECHAT_SCRIPT_EXEC_STRING,
                                            ptr_function,
                                            "sss", func_argv);
        }

        return ret;
    }

    return NULL;
}

API_FUNC(bar_item_new)
{
    Tcl_Obj *objp;
    char *name, *function, *data;
    const char *result;
    int i;

    API_INIT_FUNC(1, "bar_item_new", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    data = Tcl_GetStringFromObj (objv[3], &i);

    result = API_PTR2STR(plugin_script_api_bar_item_new (weechat_tcl_plugin,
                                                         tcl_current_script,
                                                         name,
                                                         &weechat_tcl_api_bar_item_build_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING(result);
}

API_FUNC(bar_item_update)
{
    Tcl_Obj *objp;
    int i;

    API_INIT_FUNC(1, "bar_item_update", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_bar_item_update (Tcl_GetStringFromObj (objv[1], &i)); /* name */

    API_RETURN_OK;
}

API_FUNC(bar_item_remove)
{
    Tcl_Obj *objp;
    int i;

    API_INIT_FUNC(1, "bar_item_remove", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_bar_item_remove (
        API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* item */

    API_RETURN_OK;
}

API_FUNC(bar_search)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(1, "bar_search", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_bar_search (Tcl_GetStringFromObj (objv[1], &i))); /* name */

    API_RETURN_STRING(result);
}

API_FUNC(bar_new)
{
    Tcl_Obj *objp;
    char *name, *hidden, *priority, *type, *conditions, *position;
    char *filling_top_bottom, *filling_left_right, *size, *size_max, *color_fg;
    char *color_delim, *color_bg, *color_bg_inactive, *separator, *bar_items;
    const char *result;
    int i;

    API_INIT_FUNC(1, "bar_new", API_RETURN_EMPTY);
    if (objc < 17)
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
    color_bg_inactive = Tcl_GetStringFromObj (objv[14], &i);
    separator = Tcl_GetStringFromObj (objv[15], &i);
    bar_items = Tcl_GetStringFromObj (objv[16], &i);

    result = API_PTR2STR(weechat_bar_new (name,
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
                                          color_bg_inactive,
                                          separator,
                                          bar_items));

    API_RETURN_STRING(result);
}

API_FUNC(bar_set)
{
    Tcl_Obj *objp;
    char *bar, *property, *value;
    int i, rc;

    API_INIT_FUNC(1, "bar_set", API_RETURN_INT(0));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(0));

    bar = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);
    value = Tcl_GetStringFromObj (objv[3], &i);

    rc = weechat_bar_set (API_STR2PTR(bar), property, value);

    API_RETURN_INT(rc);
}

API_FUNC(bar_update)
{
    Tcl_Obj *objp;
    int i;

    API_INIT_FUNC(1, "bar_update", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_bar_update (Tcl_GetStringFromObj (objv[1], &i)); /* name */

    API_RETURN_OK;
}

API_FUNC(bar_remove)
{
    Tcl_Obj *objp;
    int i;

    API_INIT_FUNC(1, "bar_remove", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_bar_remove (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* bar */

    API_RETURN_OK;
}

API_FUNC(command)
{
    Tcl_Obj *objp;
    char *buffer, *command;
    int i, rc;

    API_INIT_FUNC(1, "command", API_RETURN_INT(WEECHAT_RC_ERROR));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    command = Tcl_GetStringFromObj (objv[2], &i);

    rc = plugin_script_api_command (weechat_tcl_plugin,
                                    tcl_current_script,
                                    API_STR2PTR(buffer),
                                    command);

    API_RETURN_INT(rc);
}

API_FUNC(command_options)
{
    Tcl_Obj *objp;
    char *buffer, *command;
    struct t_hashtable *options;
    int i, rc;

    API_INIT_FUNC(1, "command_options", API_RETURN_INT(WEECHAT_RC_ERROR));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    command = Tcl_GetStringFromObj (objv[2], &i);
    options = weechat_tcl_dict_to_hashtable (interp, objv[3],
                                             WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                             WEECHAT_HASHTABLE_STRING,
                                             WEECHAT_HASHTABLE_STRING);

    rc = plugin_script_api_command_options (weechat_tcl_plugin,
                                            tcl_current_script,
                                            API_STR2PTR(buffer),
                                            command,
                                            options);

    if (options)
        weechat_hashtable_free (options);

    API_RETURN_INT(rc);
}

API_FUNC(completion_new)
{
    Tcl_Obj *objp;
    char *buffer;
    const char *result;
    int i;

    API_INIT_FUNC(1, "completion_new", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = Tcl_GetStringFromObj (objv[1], &i);

    result = API_PTR2STR(weechat_completion_new (API_STR2PTR(buffer)));

    API_RETURN_STRING(result);
}

API_FUNC(completion_search)
{
    Tcl_Obj *objp;
    char *completion, *data;
    int i, position, direction, rc;

    API_INIT_FUNC(1, "completion_search", API_RETURN_INT(0));
    if (objc < 5)
        API_WRONG_ARGS(API_RETURN_INT(0));

    completion = Tcl_GetStringFromObj (objv[1], &i);
    data = Tcl_GetStringFromObj (objv[2], &i);

    if ((Tcl_GetIntFromObj (interp, objv[3], &position) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[4], &direction) != TCL_OK))
        API_WRONG_ARGS(API_RETURN_ERROR);

    rc = weechat_completion_search (API_STR2PTR(completion), data, position,
                                    direction);

    API_RETURN_INT(rc);
}

API_FUNC(completion_get_string)
{
    Tcl_Obj *objp;
    char *completion, *property;
    const char *result;
    int i;

    API_INIT_FUNC(1, "completion_get_string", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    completion = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_completion_get_string (API_STR2PTR(completion),
                                            property);

    API_RETURN_STRING(result);
}

API_FUNC(completion_list_add)
{
    Tcl_Obj *objp;
    char *completion, *word, *where;
    int i, nick_completion;

    API_INIT_FUNC(1, "completion_list_add", API_RETURN_ERROR);
    if (objc < 5)
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (Tcl_GetIntFromObj (interp, objv[3], &nick_completion) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_ERROR);

    completion = Tcl_GetStringFromObj (objv[1], &i);
    word = Tcl_GetStringFromObj (objv[2], &i);
    where = Tcl_GetStringFromObj (objv[4], &i);

    weechat_completion_list_add (API_STR2PTR(completion),
                                 word,
                                 nick_completion, /* nick_completion */
                                 where);

    API_RETURN_OK;
}

API_FUNC(info_get)
{
    Tcl_Obj *objp;
    char *result;
    int i;

    API_INIT_FUNC(1, "info_get", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_info_get (Tcl_GetStringFromObj (objv[1], &i),
                               Tcl_GetStringFromObj (objv[2], &i));

    API_RETURN_STRING_FREE(result);
}

API_FUNC(info_get_hashtable)
{
    Tcl_Obj *objp, *result_dict;
    struct t_hashtable *hashtable, *result_hashtable;
    int i;

    API_INIT_FUNC(1, "info_get_hashtable", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hashtable = weechat_tcl_dict_to_hashtable (interp, objv[2],
                                               WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                               WEECHAT_HASHTABLE_STRING,
                                               WEECHAT_HASHTABLE_STRING);

    result_hashtable = weechat_info_get_hashtable (Tcl_GetStringFromObj (objv[1], &i),
                                                   hashtable);
    result_dict = weechat_tcl_hashtable_to_dict (interp, result_hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);
    if (result_hashtable)
        weechat_hashtable_free (result_hashtable);

    API_RETURN_OBJ(result_dict);
}

API_FUNC(infolist_new)
{
    Tcl_Obj *objp;
    const char *result;

    /* make C compiler happy */
    (void) clientData;
    (void) objc;
    (void) objv;

    API_INIT_FUNC(1, "infolist_new", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new ());

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_item)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(1, "infolist_new_item", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_item (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)))); /* infolist */

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_integer)
{
    Tcl_Obj *objp;
    const char *result;
    int i, value;

    API_INIT_FUNC(1, "infolist_new_var_integer", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if (Tcl_GetIntFromObj (interp, objv[3], &value) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_var_integer (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)), /* item */
                                                           Tcl_GetStringFromObj (objv[2], &i), /* name */
                                                           value));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_string)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(1, "infolist_new_var_string", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_var_string (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)), /* item */
                                                          Tcl_GetStringFromObj (objv[2], &i), /* name */
                                                          Tcl_GetStringFromObj (objv[3], &i))); /* value */

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_pointer)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(1, "infolist_new_var_pointer", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_var_pointer (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)), /* item */
                                                           Tcl_GetStringFromObj (objv[2], &i), /* name */
                                                           API_STR2PTR(Tcl_GetStringFromObj (objv[3], &i)))); /* value */

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_time)
{
    Tcl_Obj *objp;
    const char *result;
    int i;
    long value;

    API_INIT_FUNC(1, "infolist_new_var_time", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    if (Tcl_GetLongFromObj (interp, objv[3], &value) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_var_time (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)), /* item */
                                                        Tcl_GetStringFromObj (objv[2], &i), /* name */
                                                        (time_t)value));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_search_var)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(1, "infolist_search_var", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_search_var (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i)), /* infolist */
                                                      Tcl_GetStringFromObj (objv[2], &i))); /* name */

    API_RETURN_STRING(result);
}

API_FUNC(infolist_get)
{
    Tcl_Obj *objp;
    char *name, *pointer, *arguments;
    const char *result;
    int i;

    API_INIT_FUNC(1, "infolist_get", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = Tcl_GetStringFromObj (objv[1], &i);
    pointer = Tcl_GetStringFromObj (objv[2], &i);
    arguments = Tcl_GetStringFromObj (objv[3], &i);

    result = API_PTR2STR(weechat_infolist_get (name,
                                               API_STR2PTR(pointer),
                                               arguments));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_next)
{
    Tcl_Obj *objp;
    int result, i;

    API_INIT_FUNC(1, "infolist_next", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_infolist_next (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* infolist */

    API_RETURN_INT(result);
}

API_FUNC(infolist_prev)
{
    Tcl_Obj *objp;
    int result, i;

    API_INIT_FUNC(1, "infolist_prev", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_infolist_prev (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* infolist */

    API_RETURN_INT(result);
}

API_FUNC(infolist_reset_item_cursor)
{
    Tcl_Obj *objp;
    int i;

    API_INIT_FUNC(1, "infolist_reset_item_cursor", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_infolist_reset_item_cursor (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* infolist */

    API_RETURN_OK;
}

API_FUNC(infolist_fields)
{
    Tcl_Obj *objp;
    const char *result;
    int i;

    API_INIT_FUNC(1, "infolist_fields", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_infolist_fields (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* infolist */

    API_RETURN_STRING(result);
}

API_FUNC(infolist_integer)
{
    Tcl_Obj *objp;
    char *infolist, *variable;
    int result, i;

    API_INIT_FUNC(1, "infolist_integer", API_RETURN_INT(0));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    infolist = Tcl_GetStringFromObj (objv[1], &i);
    variable = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_infolist_integer (API_STR2PTR(infolist), variable);

    API_RETURN_INT(result);
}

API_FUNC(infolist_string)
{
    Tcl_Obj *objp;
    char *infolist, *variable;
    const char *result;
    int i;

    API_INIT_FUNC(1, "infolist_string", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = Tcl_GetStringFromObj (objv[1], &i);
    variable = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_infolist_string (API_STR2PTR(infolist), variable);

    API_RETURN_STRING(result);
}

API_FUNC(infolist_pointer)
{
    Tcl_Obj *objp;
    char *infolist, *variable;
    const char *result;
    int i;

    API_INIT_FUNC(1, "infolist_pointer", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = Tcl_GetStringFromObj (objv[1], &i);
    variable = Tcl_GetStringFromObj (objv[2], &i);

    result = API_PTR2STR(weechat_infolist_pointer (API_STR2PTR(infolist), variable));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_time)
{
    Tcl_Obj *objp;
    time_t time;
    char *infolist, *variable;
    int i;

    API_INIT_FUNC(1, "infolist_time", API_RETURN_LONG(0));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_LONG(0));

    infolist = Tcl_GetStringFromObj (objv[1], &i);
    variable = Tcl_GetStringFromObj (objv[2], &i);

    time = weechat_infolist_time (API_STR2PTR(infolist), variable);

    API_RETURN_LONG(time);
}

API_FUNC(infolist_free)
{
    Tcl_Obj *objp;
    int i;

    API_INIT_FUNC(1, "infolist_free", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_infolist_free (API_STR2PTR(Tcl_GetStringFromObj (objv[1], &i))); /* infolist */

    API_RETURN_OK;
}

API_FUNC(hdata_get)
{
    Tcl_Obj *objp;
    char *name;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hdata_get", API_RETURN_EMPTY);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = Tcl_GetStringFromObj (objv[1], &i);

    result = API_PTR2STR(weechat_hdata_get (name));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_get_var_offset)
{
    Tcl_Obj *objp;
    char *hdata, *name;
    int result, i;

    API_INIT_FUNC(1, "hdata_get_var_offset", API_RETURN_INT(0));
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    name = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_hdata_get_var_offset (API_STR2PTR(hdata), name);

    API_RETURN_INT(result);
}

API_FUNC(hdata_get_var_type_string)
{
    Tcl_Obj *objp;
    char *hdata, *name;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hdata_get_var_type_string", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    name = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_hdata_get_var_type_string (API_STR2PTR(hdata), name);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_get_var_array_size)
{
    Tcl_Obj *objp;
    char *hdata, *pointer, *name;
    int result, i;

    API_INIT_FUNC(1, "hdata_get_var_array_size", API_RETURN_INT(-1));
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

API_FUNC(hdata_get_var_array_size_string)
{
    Tcl_Obj *objp;
    char *hdata, *pointer, *name;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hdata_get_var_array_size_string", API_RETURN_EMPTY);
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

API_FUNC(hdata_get_var_hdata)
{
    Tcl_Obj *objp;
    char *hdata, *name;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hdata_get_var_hdata", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    name = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_hdata_get_var_hdata (API_STR2PTR(hdata), name);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_get_list)
{
    Tcl_Obj *objp;
    char *hdata, *name;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hdata_get_list", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    name = Tcl_GetStringFromObj (objv[2], &i);

    result = API_PTR2STR(weechat_hdata_get_list (API_STR2PTR(hdata),
                                                 name));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_check_pointer)
{
    Tcl_Obj *objp;
    char *hdata, *list, *pointer;
    int result, i;

    API_INIT_FUNC(1, "hdata_check_pointer", API_RETURN_INT(0));
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

API_FUNC(hdata_move)
{
    Tcl_Obj *objp;
    char *hdata, *pointer;
    const char *result;
    int i, count;

    API_INIT_FUNC(1, "hdata_move", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    pointer = Tcl_GetStringFromObj (objv[2], &i);

    if (Tcl_GetIntFromObj (interp, objv[3], &count) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_hdata_move (API_STR2PTR(hdata),
                                             API_STR2PTR(pointer),
                                             count));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_search)
{
    Tcl_Obj *objp;
    char *hdata, *pointer, *search;
    const char *result;
    struct t_hashtable *pointers, *extra_vars, *options;
    int i, move;

    API_INIT_FUNC(1, "hdata_search", API_RETURN_EMPTY);
    if (objc < 8)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    pointer = Tcl_GetStringFromObj (objv[2], &i);
    search = Tcl_GetStringFromObj (objv[3], &i);
    pointers = weechat_tcl_dict_to_hashtable (interp, objv[4],
                                              WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                              WEECHAT_HASHTABLE_STRING,
                                              WEECHAT_HASHTABLE_POINTER);
    extra_vars = weechat_tcl_dict_to_hashtable (interp, objv[5],
                                                WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                WEECHAT_HASHTABLE_STRING,
                                                WEECHAT_HASHTABLE_STRING);
    options = weechat_tcl_dict_to_hashtable (interp, objv[6],
                                             WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                             WEECHAT_HASHTABLE_STRING,
                                             WEECHAT_HASHTABLE_STRING);

    if (Tcl_GetIntFromObj (interp, objv[7], &move) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_hdata_search (API_STR2PTR(hdata),
                                               API_STR2PTR(pointer),
                                               search,
                                               pointers,
                                               extra_vars,
                                               options,
                                               move));

    if (pointers)
        weechat_hashtable_free (pointers);
    if (extra_vars)
        weechat_hashtable_free (extra_vars);
    if (options)
        weechat_hashtable_free (options);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_char)
{
    Tcl_Obj *objp;
    char *hdata, *pointer, *name;
    int result, i;

    API_INIT_FUNC(1, "hdata_char", API_RETURN_INT(0));
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

API_FUNC(hdata_integer)
{
    Tcl_Obj *objp;
    char *hdata, *pointer, *name;
    int result, i;

    API_INIT_FUNC(1, "hdata_integer", API_RETURN_INT(0));
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

API_FUNC(hdata_long)
{
    Tcl_Obj *objp;
    char *hdata, *pointer, *name;
    int result, i;

    API_INIT_FUNC(1, "hdata_long", API_RETURN_LONG(0));
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

API_FUNC(hdata_string)
{
    Tcl_Obj *objp;
    char *hdata, *pointer, *name;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hdata_string", API_RETURN_EMPTY);
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

API_FUNC(hdata_pointer)
{
    Tcl_Obj *objp;
    char *hdata, *pointer, *name;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hdata_pointer", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    pointer = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);

    result = API_PTR2STR(weechat_hdata_pointer (API_STR2PTR(hdata),
                                                API_STR2PTR(pointer),
                                                name));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_time)
{
    Tcl_Obj *objp;
    time_t time;
    char *hdata, *pointer, *name;
    int i;

    API_INIT_FUNC(1, "hdata_time", API_RETURN_LONG(0));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_LONG(0));

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    pointer = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);

    time = weechat_hdata_time (API_STR2PTR(hdata),
                               API_STR2PTR(pointer),
                               name);

    API_RETURN_LONG(time);
}

API_FUNC(hdata_hashtable)
{
    Tcl_Obj *objp, *result_dict;
    char *hdata, *pointer, *name;
    int i;

    API_INIT_FUNC(1, "hdata_hashtable", API_RETURN_EMPTY);
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

API_FUNC(hdata_compare)
{
    Tcl_Obj *objp;
    char *hdata, *pointer1, *pointer2, *name;
    int case_sensitive, rc, i;

    API_INIT_FUNC(1, "hdata_compare", API_RETURN_INT(0));
    if (objc < 6)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    pointer1 = Tcl_GetStringFromObj (objv[2], &i);
    pointer2 = Tcl_GetStringFromObj (objv[3], &i);
    name = Tcl_GetStringFromObj (objv[4], &i);

    if (Tcl_GetIntFromObj (interp, objv[5], &case_sensitive) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_INT(0));

    rc = weechat_hdata_compare (API_STR2PTR(hdata),
                                API_STR2PTR(pointer1),
                                API_STR2PTR(pointer2),
                                name,
                                case_sensitive);

    API_RETURN_INT(rc);
}

API_FUNC(hdata_update)
{
    Tcl_Obj *objp;
    char *hdata, *pointer;
    struct t_hashtable *hashtable;
    int i, value;

    API_INIT_FUNC(1, "hdata_update", API_RETURN_INT(0));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    pointer = Tcl_GetStringFromObj (objv[2], &i);
    hashtable = weechat_tcl_dict_to_hashtable (interp, objv[3],
                                               WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                               WEECHAT_HASHTABLE_STRING,
                                               WEECHAT_HASHTABLE_STRING);

    value = weechat_hdata_update (API_STR2PTR(hdata),
                                  API_STR2PTR(pointer),
                                  hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);

    API_RETURN_INT(value);
}

API_FUNC(hdata_get_string)
{
    Tcl_Obj *objp;
    char *hdata, *property;
    const char *result;
    int i;

    API_INIT_FUNC(1, "hdata_get_string", API_RETURN_EMPTY);
    if (objc < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);

    result = weechat_hdata_get_string (API_STR2PTR(hdata), property);

    API_RETURN_STRING(result);
}

int
weechat_tcl_api_upgrade_read_cb (const void *pointer, void *data,
                                 struct t_upgrade_file *upgrade_file,
                                 int object_id,
                                 struct t_infolist *infolist)
{
    struct t_plugin_script *script;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(upgrade_file);
        func_argv[2] = &object_id;
        func_argv[3] = (char *)API_PTR2STR(infolist);

        rc = (int *) weechat_tcl_exec (script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       ptr_function,
                                       "ssis", func_argv);

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

API_FUNC(upgrade_new)
{
    Tcl_Obj *objp;
    char *filename, *function, *data;
    const char *result;
    int i;

    API_INIT_FUNC(1, "upgrade_new", API_RETURN_EMPTY);
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    filename = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    data = Tcl_GetStringFromObj (objv[3], &i);

    result = API_PTR2STR(
        plugin_script_api_upgrade_new (
            weechat_tcl_plugin,
            tcl_current_script,
            filename,
            &weechat_tcl_api_upgrade_read_cb,
            function,
            data));

    API_RETURN_STRING(result);
}

API_FUNC(upgrade_write_object)
{
    Tcl_Obj *objp;
    char *upgrade_file, *infolist;
    int rc, i, object_id;

    API_INIT_FUNC(1, "upgrade_write_object", API_RETURN_INT(0));
    if (objc < 4)
        API_WRONG_ARGS(API_RETURN_INT(0));

    if (Tcl_GetIntFromObj (interp, objv[2], &object_id) != TCL_OK)
        API_WRONG_ARGS(API_RETURN_INT(0));

    upgrade_file = Tcl_GetStringFromObj (objv[1], &i);
    infolist = Tcl_GetStringFromObj (objv[3], &i);

    rc = weechat_upgrade_write_object (API_STR2PTR(upgrade_file),
                                       object_id,
                                       API_STR2PTR(infolist));

    API_RETURN_INT(rc);
}

API_FUNC(upgrade_read)
{
    Tcl_Obj *objp;
    char *upgrade_file;
    int i, rc;

    API_INIT_FUNC(1, "upgrade_read", API_RETURN_INT(0));
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    upgrade_file = Tcl_GetStringFromObj (objv[1], &i);

    rc = weechat_upgrade_read (API_STR2PTR(upgrade_file));

    API_RETURN_INT(rc);
}

API_FUNC(upgrade_close)
{
    Tcl_Obj *objp;
    char *upgrade_file;
    int i;

    API_INIT_FUNC(1, "upgrade_close", API_RETURN_ERROR);
    if (objc < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    upgrade_file = Tcl_GetStringFromObj (objv[1], &i);

    weechat_upgrade_close (API_STR2PTR(upgrade_file));

    API_RETURN_OK;
}

/*
 * Initializes tcl functions and constants.
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

    Tcl_SetStringObj (objp, WEECHAT_NULL_STRING, -1);
    Tcl_SetVar (interp, "weechat::WEECHAT_NULL", Tcl_GetStringFromObj (objp, &i), 0);

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
    Tcl_SetIntObj (objp, WEECHAT_HOOK_CONNECT_SOCKET_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_SOCKET_ERROR", Tcl_GetStringFromObj (objp, &i), 0);

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
    API_DEF_FUNC(strlen_screen);
    API_DEF_FUNC(string_match);
    API_DEF_FUNC(string_match_list);
    API_DEF_FUNC(string_has_highlight);
    API_DEF_FUNC(string_has_highlight_regex);
    API_DEF_FUNC(string_mask_to_regex);
    API_DEF_FUNC(string_format_size);
    API_DEF_FUNC(string_parse_size);
    API_DEF_FUNC(string_color_code_size);
    API_DEF_FUNC(string_remove_color);
    API_DEF_FUNC(string_is_command_char);
    API_DEF_FUNC(string_input_for_buffer);
    API_DEF_FUNC(string_eval_expression);
    API_DEF_FUNC(string_eval_path_home);
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
    API_DEF_FUNC(config_set_version);
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
    API_DEF_FUNC(print_y_date_tags);
    API_DEF_FUNC(log_print);
    API_DEF_FUNC(hook_command);
    API_DEF_FUNC(hook_completion);
    API_DEF_FUNC(hook_completion_get_string);
    API_DEF_FUNC(hook_completion_list_add);
    API_DEF_FUNC(hook_command_run);
    API_DEF_FUNC(hook_timer);
    API_DEF_FUNC(hook_fd);
    API_DEF_FUNC(hook_process);
    API_DEF_FUNC(hook_process_hashtable);
    API_DEF_FUNC(hook_connect);
    API_DEF_FUNC(hook_line);
    API_DEF_FUNC(hook_print);
    API_DEF_FUNC(hook_signal);
    API_DEF_FUNC(hook_signal_send);
    API_DEF_FUNC(hook_hsignal);
    API_DEF_FUNC(hook_hsignal_send);
    API_DEF_FUNC(hook_config);
    API_DEF_FUNC(hook_modifier);
    API_DEF_FUNC(hook_modifier_exec);
    API_DEF_FUNC(hook_info);
    API_DEF_FUNC(hook_info_hashtable);
    API_DEF_FUNC(hook_infolist);
    API_DEF_FUNC(hook_focus);
    API_DEF_FUNC(hook_set);
    API_DEF_FUNC(unhook);
    API_DEF_FUNC(unhook_all);
    API_DEF_FUNC(buffer_new);
    API_DEF_FUNC(buffer_new_props);
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
    API_DEF_FUNC(command_options);
    API_DEF_FUNC(completion_new);
    API_DEF_FUNC(completion_search);
    API_DEF_FUNC(completion_get_string);
    API_DEF_FUNC(completion_list_add);
    API_DEF_FUNC(info_get);
    API_DEF_FUNC(info_get_hashtable);
    API_DEF_FUNC(infolist_new);
    API_DEF_FUNC(infolist_new_item);
    API_DEF_FUNC(infolist_new_var_integer);
    API_DEF_FUNC(infolist_new_var_string);
    API_DEF_FUNC(infolist_new_var_pointer);
    API_DEF_FUNC(infolist_new_var_time);
    API_DEF_FUNC(infolist_search_var);
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
    API_DEF_FUNC(hdata_search);
    API_DEF_FUNC(hdata_char);
    API_DEF_FUNC(hdata_integer);
    API_DEF_FUNC(hdata_long);
    API_DEF_FUNC(hdata_string);
    API_DEF_FUNC(hdata_pointer);
    API_DEF_FUNC(hdata_time);
    API_DEF_FUNC(hdata_hashtable);
    API_DEF_FUNC(hdata_compare);
    API_DEF_FUNC(hdata_update);
    API_DEF_FUNC(hdata_get_string);
    API_DEF_FUNC(upgrade_new);
    API_DEF_FUNC(upgrade_write_object);
    API_DEF_FUNC(upgrade_read);
    API_DEF_FUNC(upgrade_close);
}
