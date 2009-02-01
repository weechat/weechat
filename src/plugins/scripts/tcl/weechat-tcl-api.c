/*
 * Copyright (c) 2008-2009 Dmitry Kobylin <fnfal@academ.tsc.ru>
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

/* weechat-tcl-api.c: Tcl API functions */


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

#define TCL_RETURN_OK                            \
    {                                            \
	objp = Tcl_GetObjResult (interp);        \
	if (Tcl_IsShared (objp))                 \
        {                                        \
	    objp = Tcl_DuplicateObj (objp);      \
	    Tcl_IncrRefCount (objp);             \
	    Tcl_SetIntObj (objp,1);              \
	    Tcl_SetObjResult (interp, objp);     \
	    Tcl_DecrRefCount (objp);             \
	}                                        \
        else                                     \
	    Tcl_SetIntObj (objp,1);              \
	return TCL_OK;                           \
    }
#define TCL_RETURN_ERROR                          \
    {                                             \
	objp = Tcl_GetObjResult (interp);         \
	if (Tcl_IsShared (objp))                  \
        {                                         \
	    objp = Tcl_DuplicateObj (objp);       \
	    Tcl_IncrRefCount (objp);              \
	    Tcl_SetIntObj (objp,0);               \
	    Tcl_SetObjResult (interp, objp);      \
	    Tcl_DecrRefCount (objp);              \
	}                                         \
        else                                      \
	    Tcl_SetIntObj (objp,0);               \
	return TCL_ERROR;                         \
    }
#define TCL_RETURN_EMPTY                          \
    {                                             \
	objp = Tcl_GetObjResult (interp);         \
	if (Tcl_IsShared (objp))                  \
        {                                         \
	    objp = Tcl_DuplicateObj (objp);       \
	    Tcl_IncrRefCount (objp);              \
	    Tcl_SetStringObj (objp,"",-1);        \
	    Tcl_SetObjResult (interp, objp);      \
	    Tcl_DecrRefCount (objp);              \
	}                                         \
        else                                      \
	    Tcl_SetStringObj (objp,"",-1);        \
	return TCL_OK;                            \
    }
#define TCL_RETURN_STRING(__string)                         \
    {                                                       \
	objp = Tcl_GetObjResult (interp);                   \
	if (Tcl_IsShared (objp))                            \
        {                                                   \
	    objp = Tcl_DuplicateObj (objp);                 \
	    Tcl_IncrRefCount (objp);                        \
	    if (__string)                                   \
	    {                                               \
		Tcl_SetStringObj (objp,__string,-1);        \
                Tcl_SetObjResult (interp, objp);            \
		Tcl_DecrRefCount (objp);                    \
		return TCL_OK;                              \
	    }                                               \
	    Tcl_SetStringObj (objp,"",-1);                  \
	    Tcl_SetObjResult (interp, objp);                \
	    Tcl_DecrRefCount (objp);                        \
	}                                                   \
        else                                                \
        {                                                   \
	    if (__string)                                   \
	    {                                               \
		Tcl_SetStringObj (objp,__string,-1);        \
		return TCL_OK;                              \
	    }                                               \
	    Tcl_SetStringObj (objp,"",-1);                  \
	}                                                   \
	return TCL_OK;                                      \
    }
#define TCL_RETURN_STRING_FREE(__string)                    \
    {                                                       \
	objp = Tcl_GetObjResult (interp);                   \
	if (Tcl_IsShared (objp))                            \
        {                                                   \
	    objp = Tcl_DuplicateObj (objp);                 \
	    Tcl_IncrRefCount (objp);                        \
	    if (__string)                                   \
	    {                                               \
		Tcl_SetStringObj (objp,__string,-1);        \
		Tcl_SetObjResult (interp, objp);            \
		Tcl_DecrRefCount (objp);                    \
		free (__string);                            \
		return TCL_OK;                              \
	    }                                               \
	    Tcl_SetStringObj (objp,"",-1);                  \
	    Tcl_SetObjResult (interp, objp);                \
	    Tcl_DecrRefCount (objp);                        \
	}                                                   \
        else                                                \
        {                                                   \
	    if (__string)                                   \
	    {                                               \
		Tcl_SetStringObj (objp,__string,-1);        \
		free(__string);                             \
		return TCL_OK;                              \
	    }                                               \
	    Tcl_SetStringObj (objp,"",-1);                  \
	}                                                   \
	return TCL_OK;                                      \
    }
#define TCL_RETURN_INT(__int)                     \
    {                                             \
	objp = Tcl_GetObjResult (interp);         \
	if (Tcl_IsShared (objp))                  \
        {                                         \
	    objp = Tcl_DuplicateObj (objp);       \
	    Tcl_IncrRefCount (objp);              \
	    Tcl_SetIntObj (objp,__int);           \
	    Tcl_SetObjResult (interp, objp);      \
	    Tcl_DecrRefCount (objp);              \
	}                                         \
        else                                      \
	    Tcl_SetIntObj (objp,__int);           \
	return TCL_OK;                            \
    }


/*
 * weechat_tcl_api_register: startup function for all WeeChat Tcl scripts
 */

static int
weechat_tcl_api_register (ClientData clientData, Tcl_Interp *interp, int objc,
                          Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *name, *author, *version, *license, *description, *shutdown_func;
    char *charset;
    int i;
    
    (void) clientData;
    
    tcl_current_script = NULL;
    
    if (objc < 8)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("register");
        TCL_RETURN_ERROR;
    }
    
    name = Tcl_GetStringFromObj (objv[1], &i);
    author = Tcl_GetStringFromObj (objv[2], &i);
    version = Tcl_GetStringFromObj (objv[3], &i);
    license = Tcl_GetStringFromObj (objv[4], &i);
    description = Tcl_GetStringFromObj (objv[5], &i);
    shutdown_func = Tcl_GetStringFromObj (objv[6], &i);
    charset = Tcl_GetStringFromObj (objv[7], &i);
    
    if (script_search (weechat_tcl_plugin, tcl_scripts, name))
    {
        /* error: another script already exists with this name! */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to register script "
                                         "\"%s\" (another script already "
                                         "exists with this name)"),
                        weechat_prefix ("error"), TCL_PLUGIN_NAME, name);
        TCL_RETURN_ERROR;
    }
    
    /* register script */
    tcl_current_script = script_add (weechat_tcl_plugin,
                                     &tcl_scripts,
                                     (tcl_current_script_filename) ?
                                     tcl_current_script_filename : "",
                                     name, author, version, license,
                                     description, shutdown_func, charset);
    if (tcl_current_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: registered script \"%s\", "
                                         "version %s (%s)"),
                        TCL_PLUGIN_NAME, name, version, description);
	tcl_current_script->interpreter = (void *)interp;
    }
    else
    {
        TCL_RETURN_ERROR;
    }
   
    TCL_RETURN_OK;
}

/*
 * weechat_tcl_api_plugin_get_name: get name of plugin (return "core" for
 *                                  WeeChat core)
 */

static int
weechat_tcl_api_plugin_get_name (ClientData clientData, Tcl_Interp *interp,
                                 int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *plugin;
    const char *result;
    int i; 
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("plugin_get_name");
        TCL_RETURN_EMPTY;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("plugin_get_name");
        TCL_RETURN_EMPTY;
    }
    
    plugin = Tcl_GetStringFromObj (objv[1], &i);
    
    result = weechat_plugin_get_name (script_str2ptr (plugin));
    
    TCL_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_charset_set: set script charset
 */

static int
weechat_tcl_api_charset_set (ClientData clientData, Tcl_Interp *interp,
                             int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    int i; 
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("charset_set");
        TCL_RETURN_ERROR;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("charset_set");
        TCL_RETURN_ERROR;
    }
    
    script_api_charset_set (tcl_current_script,
                            Tcl_GetStringFromObj (objv[1], &i)); /* charset */
    
    TCL_RETURN_OK;
}

/*
 * weechat_tcl_api_iconv_to_internal: convert string to internal WeeChat
 *                                    charset
 */

static int
weechat_tcl_api_iconv_to_internal (ClientData clientData, Tcl_Interp *interp,
                                   int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *result, *charset, *string;
    int i; 

    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("iconv_to_internal");
        TCL_RETURN_EMPTY;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("iconv_to_internal");
        TCL_RETURN_EMPTY;
    }

    charset = Tcl_GetStringFromObj (objv[1], &i);
    string = Tcl_GetStringFromObj (objv[2], &i);
    result = weechat_iconv_to_internal (charset, string);
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_iconv_from_internal: convert string from WeeChat inernal
 *                                      charset to another one
 */

static int
weechat_tcl_api_iconv_from_internal (ClientData clientData, Tcl_Interp *interp,
                                     int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *result, *charset, *string;
    int i; 

    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("iconv_from_internal");
        TCL_RETURN_EMPTY;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("iconv_from_internal");
        TCL_RETURN_EMPTY;
    }
    
    charset = Tcl_GetStringFromObj (objv[1], &i);
    string = Tcl_GetStringFromObj (objv[2], &i);
    result = weechat_iconv_from_internal (charset, string);
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_gettext: get translated string
 */

static int
weechat_tcl_api_gettext (ClientData clientData, Tcl_Interp *interp,
                         int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    const char *result;
    int i; 
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("gettext");
        TCL_RETURN_EMPTY;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("gettext");
        TCL_RETURN_EMPTY;
    }
    
    result = weechat_gettext (Tcl_GetStringFromObj (objv[1], &i)); /* string */
    
    TCL_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_ngettext: get translated string with plural form
 */

static int
weechat_tcl_api_ngettext (ClientData clientData, Tcl_Interp *interp,
                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *single, *plural;
    const char *result;
    int i, count;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("ngettext");
        TCL_RETURN_EMPTY;
    }
    
    if (objc < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("ngettext");
        TCL_RETURN_EMPTY;
    }
    
    single = Tcl_GetStringFromObj (objv[1], &i);
    plural = Tcl_GetStringFromObj (objv[2], &i);

    if (Tcl_GetIntFromObj (interp, objv[3], &count) != TCL_OK)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("ngettext");
	TCL_RETURN_EMPTY;
    }

    result = weechat_ngettext (single, plural, count);
    
    TCL_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_mkdir_home: create a directory in WeeChat home
 */

static int
weechat_tcl_api_mkdir_home (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    int i, mode;

    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("mkdir_home");
        TCL_RETURN_ERROR;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("mkdir_home");
        TCL_RETURN_ERROR;
    }
    
    if (Tcl_GetIntFromObj (interp, objv[2], &mode) != TCL_OK)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("mkdir_home");
	TCL_RETURN_EMPTY;
    }
    
    if (weechat_mkdir_home (Tcl_GetStringFromObj (objv[1], &i), /* directory */
                            mode))
        TCL_RETURN_OK;
    
    TCL_RETURN_ERROR;
}

/*
 * weechat_tcl_api_mkdir: create a directory
 */

static int
weechat_tcl_api_mkdir (ClientData clientData, Tcl_Interp *interp,
                       int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    int i, mode;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("mkdir");
        TCL_RETURN_ERROR;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("mkdir");
        TCL_RETURN_ERROR;
    }
    
    if (Tcl_GetIntFromObj (interp, objv[2], &mode) != TCL_OK)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("mkdir");
	TCL_RETURN_EMPTY;
    }
    
    if (weechat_mkdir (Tcl_GetStringFromObj (objv[1], &i), /* directory */
                       mode))
        TCL_RETURN_OK;
    
    TCL_RETURN_ERROR;
}

/*
 * weechat_tcl_api_mkdir_parents: create a directory and make parent
 *                                directories as needed
 */

static int
weechat_tcl_api_mkdir_parents (ClientData clientData, Tcl_Interp *interp,
                               int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    int i, mode;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("mkdir_parents");
        TCL_RETURN_ERROR;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("mkdir_parents");
        TCL_RETURN_ERROR;
    }
    
    if (Tcl_GetIntFromObj (interp, objv[2], &mode) != TCL_OK)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("mkdir_parents");
	TCL_RETURN_EMPTY;
    }
    
    if (weechat_mkdir_parents (Tcl_GetStringFromObj (objv[1], &i), /* directory */
                               mode))
        TCL_RETURN_OK;
    
    TCL_RETURN_ERROR;
}

/*
 * weechat_tcl_api_list_new: create a new list
 */

static int
weechat_tcl_api_list_new (ClientData clientData, Tcl_Interp *interp,
                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *result;

    /* make C compiler happy */
    (void) clientData;
    (void) objc;
    (void) objv;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_new");
	TCL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_new ());
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_list_add: add a string to list
 */

static int
weechat_tcl_api_list_add (ClientData clientData, Tcl_Interp *interp,
                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *result, *weelist, *data, *where;
    int i; 

    /* make C compiler happy */
    (void) clientData;
    
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_add");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_add");
        TCL_RETURN_EMPTY;
    }
    
    weelist = Tcl_GetStringFromObj (objv[1], &i);
    data = Tcl_GetStringFromObj (objv[2], &i);
    where = Tcl_GetStringFromObj (objv[3], &i);
    result = script_ptr2str (weechat_list_add (script_str2ptr (weelist),
                                               data,
                                               where));
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_list_search: search a string in list
 */

static int
weechat_tcl_api_list_search (ClientData clientData, Tcl_Interp *interp,
                             int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *result, *weelist, *data;
    int i; 

    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_search");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_search");
        TCL_RETURN_EMPTY;
    }
    
    weelist = Tcl_GetStringFromObj (objv[1], &i);
    data = Tcl_GetStringFromObj (objv[2], &i);
    result = script_ptr2str (weechat_list_search (script_str2ptr (weelist),
                                                  data));
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_list_casesearch: search a string in list (ignore case)
 */

static int
weechat_tcl_api_list_casesearch (ClientData clientData, Tcl_Interp *interp,
                                 int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *result, *weelist, *data;
    int i; 

    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_casesearch");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_casesearch");
        TCL_RETURN_EMPTY;
    }
    
    weelist = Tcl_GetStringFromObj (objv[1], &i);
    data = Tcl_GetStringFromObj (objv[2], &i);
    result = script_ptr2str (weechat_list_casesearch (script_str2ptr (weelist),
                                                      data));
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_list_get: get item by position
 */

static int
weechat_tcl_api_list_get (ClientData clientData, Tcl_Interp *interp,
                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *result;
    int i, position;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_get");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_get");
        TCL_RETURN_EMPTY;
    }
    
    if (Tcl_GetIntFromObj (interp, objv[2], &position) != TCL_OK)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_get");
        TCL_RETURN_EMPTY;
    }	
    
    result = script_ptr2str (weechat_list_get (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i)), /* weelist */
                                               position));
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_list_set: set new value for item
 */

static int
weechat_tcl_api_list_set (ClientData clientData, Tcl_Interp *interp,
                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *item, *new_value;
    int i; 

    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_set");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_set");
        TCL_RETURN_ERROR;
    }
    
    item = Tcl_GetStringFromObj (objv[1], &i);
    new_value = Tcl_GetStringFromObj (objv[2], &i);
    weechat_list_set (script_str2ptr (item), new_value);
    
    TCL_RETURN_OK;
}

/*
 * weechat_tcl_api_list_next: get next item
 */

static int
weechat_tcl_api_list_next (ClientData clientData, Tcl_Interp *interp,
                           int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *result;
    int i; 

    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_next");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_next");
        TCL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_next (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i)))); /* item */
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_list_prev: get previous item
 */

static int
weechat_tcl_api_list_prev (ClientData clientData, Tcl_Interp *interp,
                           int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *result;
    int i; 

    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_prev");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_prev");
        TCL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_prev (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i)))); /* item */
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_list_string: get string value of item
 */

static int
weechat_tcl_api_list_string (ClientData clientData, Tcl_Interp *interp,
                             int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    const char *result;
    int i; 
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_string");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_string");
        TCL_RETURN_EMPTY;
    }
    
    result = weechat_list_string (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* item */
    
    TCL_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_list_size: get number of elements in list
 */

static int
weechat_tcl_api_list_size (ClientData clientData, Tcl_Interp *interp,
                           int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    int size;
    int i; 

    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_size");
	TCL_RETURN_INT(0);
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_size");
        TCL_RETURN_INT(0);
    }
    
    size = weechat_list_size (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* weelist */
    
    TCL_RETURN_INT(size);
}

/*
 * weechat_tcl_api_list_remove: remove item from list
 */

static int
weechat_tcl_api_list_remove (ClientData clientData, Tcl_Interp *interp,
                             int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *weelist, *item;
    int i; 

    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_remove");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_remove");
        TCL_RETURN_ERROR;
    }
    
    weelist = Tcl_GetStringFromObj (objv[1], &i);
    item = Tcl_GetStringFromObj (objv[2], &i);
    weechat_list_remove (script_str2ptr (weelist), script_str2ptr (item));
    
    TCL_RETURN_OK;
}

/*
 * weechat_tcl_api_list_remove_all: remove all items from list
 */

static int
weechat_tcl_api_list_remove_all (ClientData clientData, Tcl_Interp *interp,
                                 int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    int i;

    (void) clientData;

    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_remove_all");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_remove_all");
        TCL_RETURN_ERROR;
    }
    
    weechat_list_remove_all (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* weelist */
    
    TCL_RETURN_OK;
}

/*
 * weechat_tcl_api_list_free: free list
 */

static int
weechat_tcl_api_list_free (ClientData clientData, Tcl_Interp *interp,
                           int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    int i; 

    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_free");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_free");
        TCL_RETURN_ERROR;
    }
    
    weechat_list_free (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* weelist */
    
    TCL_RETURN_OK;
}

/*
 * weechat_tcl_api_config_reload_cb: callback for config reload
 */

int
weechat_tcl_api_config_reload_cb (void *data,
                                   struct t_config_file *config_file)
{
    struct t_script_callback *script_callback;
    char *tcl_argv[2];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;


    if (script_callback->function && script_callback->function[0])
    {
        tcl_argv[0] = script_ptr2str (config_file);
        tcl_argv[1] = NULL;
        
        rc = (int *) weechat_tcl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        tcl_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (tcl_argv[0])
            free (tcl_argv[0]);

        return ret;
    }
    
    return 0;
}

/*
 * weechat_tcl_api_config_new: create a new configuration file
 */

static int
weechat_tcl_api_config_new (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *result, *name, *function;
    int i; 

    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_new");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_new");
        TCL_RETURN_EMPTY;
    }
    
    name = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    result = script_ptr2str (script_api_config_new (weechat_tcl_plugin,
                                                    tcl_current_script,
                                                    name,
                                                    &weechat_tcl_api_config_reload_cb,
                                                    function));
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_config_section_read_cb: callback for reading option in
 *                                         section
 */

void
weechat_tcl_api_config_section_read_cb (void *data,
                                         struct t_config_file *config_file,
                                         const char *option_name, const char *value)
{
    struct t_script_callback *script_callback;
    char *tcl_argv[4];
    int *rc;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback->function && script_callback->function[0])
    {
        tcl_argv[0] = script_ptr2str (config_file);
        tcl_argv[1] = (char *)option_name;
        tcl_argv[2] = (char *)value;
        tcl_argv[3] = NULL;
        
        rc = (int *) weechat_tcl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        tcl_argv);
        
        if (rc)
            free (rc);
        if (tcl_argv[0])
            free (tcl_argv[0]);
    }
}

/*
 * weechat_tcl_api_config_section_write_cb: callback for writing section
 */

void
weechat_tcl_api_config_section_write_cb (void *data,
                                          struct t_config_file *config_file,
                                          const char *section_name)
{
    struct t_script_callback *script_callback;
    char *tcl_argv[3];
    int *rc;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback->function && script_callback->function[0])
    {
        tcl_argv[0] = script_ptr2str (config_file);
        tcl_argv[1] = (char *)section_name;
        tcl_argv[2] = NULL;

        rc = (int *) weechat_tcl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        tcl_argv);
        
	if (rc)
            free (rc);
        if (tcl_argv[0])
            free (tcl_argv[0]);
    }
}

/*
 * weechat_tcl_api_config_section_write_default_cb: callback for writing
 *                                                   default values for section
 */

void
weechat_tcl_api_config_section_write_default_cb (void *data,
                                                  struct t_config_file *config_file,
                                                  const char *section_name)
{
    struct t_script_callback *script_callback;
    char *tcl_argv[3];
    int *rc;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback->function && script_callback->function[0])
    {
        tcl_argv[0] = script_ptr2str (config_file);
        tcl_argv[1] = (char *)section_name;
        tcl_argv[2] = NULL;
        
        rc = (int *) weechat_tcl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        tcl_argv);
        
        if (rc)
            free (rc);
        if (tcl_argv[0])
            free (tcl_argv[0]);
    }
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
    char *tcl_argv[5];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback->function && script_callback->function[0])
    {
        tcl_argv[0] = script_ptr2str (config_file);
        tcl_argv[1] = script_ptr2str (section);
        tcl_argv[2] = (char *)option_name;
        tcl_argv[3] = (char *)value;
        tcl_argv[4] = NULL;
        
        rc = (int *) weechat_tcl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        tcl_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (tcl_argv[0])
            free (tcl_argv[0]);
        if (tcl_argv[1])
            free (tcl_argv[1]);
        
        return ret;
    }
    
    return 0;
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
    char *tcl_argv[4];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback->function && script_callback->function[0])
    {
        tcl_argv[0] = script_ptr2str (config_file);
        tcl_argv[1] = script_ptr2str (section);
        tcl_argv[2] = script_ptr2str (option);
        tcl_argv[3] = NULL;
        
        rc = (int *) weechat_tcl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        tcl_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (tcl_argv[0])
            free (tcl_argv[0]);
        if (tcl_argv[1])
            free (tcl_argv[1]);
        if (tcl_argv[2])
            free (tcl_argv[2]);
        
        return ret;
    }
    
    return 0;
}

/*
 * weechat_tcl_api_config_new_section: create a new section in configuration
 *                                     file
 */

static int
weechat_tcl_api_config_new_section (ClientData clientData, Tcl_Interp *interp,
                                    int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *result, *cfg_file, *name, *function_read, *function_write;
    char *function_write_default, *function_create_option;
    char *function_delete_option;
    int i,can_add,can_delete;
    
    /* make C compiler happy */
    (void) clientData; 

    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_new_section");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 10)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_new_section");
        TCL_RETURN_EMPTY;
    }
   
    if ((Tcl_GetIntFromObj (interp, objv[3], &can_add) != TCL_OK)
	|| (Tcl_GetIntFromObj (interp, objv[4], &can_delete) != TCL_OK))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_new_section");
        TCL_RETURN_EMPTY;
    }

    cfg_file = Tcl_GetStringFromObj (objv[1], &i);
    name = Tcl_GetStringFromObj (objv[2], &i);
    function_read = Tcl_GetStringFromObj (objv[5], &i);
    function_write = Tcl_GetStringFromObj (objv[6], &i);
    function_write_default = Tcl_GetStringFromObj (objv[7], &i);
    function_create_option = Tcl_GetStringFromObj (objv[8], &i);
    function_delete_option = Tcl_GetStringFromObj (objv[9], &i);
    result = script_ptr2str (script_api_config_new_section (weechat_tcl_plugin,
                                                            tcl_current_script,
                                                            script_str2ptr (cfg_file),
                                                            name,
                                                            can_add, /* user_can_add_options */
                                                            can_delete, /* user_can_delete_options */
                                                            &weechat_tcl_api_config_section_read_cb,
                                                            function_read,
                                                            &weechat_tcl_api_config_section_write_cb,
                                                            function_write,
                                                            &weechat_tcl_api_config_section_write_default_cb,
                                                            function_write_default,
                                                            &weechat_tcl_api_config_section_create_option_cb,
                                                            function_create_option,
                                                            &weechat_tcl_api_config_section_delete_option_cb,
                                                            function_delete_option));
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_config_search_section: search section in configuration file
 */

static int
weechat_tcl_api_config_search_section (ClientData clientData, Tcl_Interp *interp,
                                       int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *result, *config_file, *section_name;
    int i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_search_section");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_search_section");
        TCL_RETURN_EMPTY;
    }
    
    config_file = Tcl_GetStringFromObj (objv[1], &i);
    section_name = Tcl_GetStringFromObj (objv[2], &i);
    result = script_ptr2str (weechat_config_search_section (script_str2ptr (config_file),
                                                            section_name));
    
    TCL_RETURN_STRING_FREE(result);
}


/*
 * weechat_tcl_api_config_option_check_value_cb: callback for checking new
 *                                               value for option
 */

void
weechat_tcl_api_config_option_check_value_cb (void *data,
                                               struct t_config_option *option,
                                               const char *value)
{
    struct t_script_callback *script_callback;
    char *tcl_argv[3];
    int *rc;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback->function && script_callback->function[0])
    {
        tcl_argv[0] = script_ptr2str (option);
        tcl_argv[1] = (char *)value;
        tcl_argv[2] = NULL;
        
        rc = (int *) weechat_tcl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        tcl_argv);
        
        if (tcl_argv[0])
            free (tcl_argv[0]);
        
        if (rc)
            free (rc);
    }
}

/*
 * weechat_tcl_api_config_option_change_cb: callback for option changed
 */

void
weechat_tcl_api_config_option_change_cb (void *data,
                                          struct t_config_option *option)
{
    struct t_script_callback *script_callback;
    char *tcl_argv[2];
    int *rc;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback->function && script_callback->function[0])
    {
        tcl_argv[0] = script_ptr2str (option);
        tcl_argv[1] = NULL;
        
        rc = (int *) weechat_tcl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        tcl_argv);
        
        if (tcl_argv[0])
            free (tcl_argv[0]);
        
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
    char *tcl_argv[2];
    int *rc;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback->function && script_callback->function[0])
    {
        tcl_argv[0] = script_ptr2str (option);
        tcl_argv[1] = NULL;
        
        rc = (int *) weechat_tcl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        tcl_argv);
        
        if (tcl_argv[0])
            free (tcl_argv[0]);
        
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
    Tcl_Obj* objp;
    char *result, *config_file, *section, *name, *type;
    char *description, *string_values, *default_value, *value;
    char *function_check_value, *function_change, *function_delete;
    int i, min, max, null_value_allowed;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_new_option");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 15)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_new_option");
        TCL_RETURN_EMPTY;
    }
    
    if ((Tcl_GetIntFromObj (interp, objv[7], &min) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[8], &max) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[11], &null_value_allowed) != TCL_OK))
    {
	WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_new_option");
        TCL_RETURN_EMPTY;
    }
    
    config_file = Tcl_GetStringFromObj (objv[1], &i);
    section = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);
    type = Tcl_GetStringFromObj (objv[4], &i);
    description = Tcl_GetStringFromObj (objv[5], &i);
    string_values = Tcl_GetStringFromObj (objv[6], &i);
    default_value = Tcl_GetStringFromObj (objv[9], &i);
    value = Tcl_GetStringFromObj (objv[10], &i);
    function_check_value = Tcl_GetStringFromObj (objv[12], &i);
    function_change = Tcl_GetStringFromObj (objv[13], &i);
    function_delete = Tcl_GetStringFromObj (objv[14], &i);

    result = script_ptr2str (script_api_config_new_option (weechat_tcl_plugin,
                                                           tcl_current_script,
                                                           script_str2ptr (config_file),
                                                           script_str2ptr (section),
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
                                                           &weechat_tcl_api_config_option_change_cb,
                                                           function_change,
                                                           &weechat_tcl_api_config_option_delete_cb,
                                                           function_delete));

    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_config_search_option: search option in configuration file or
 *                                       section
 */

static int
weechat_tcl_api_config_search_option (ClientData clientData, Tcl_Interp *interp,
                                      int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *result, *config_file, *section, *option_name;
    int i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_search_option");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_search_option");
        TCL_RETURN_EMPTY;
    }
    
    config_file = Tcl_GetStringFromObj (objv[1], &i);
    section = Tcl_GetStringFromObj (objv[2], &i);
    option_name = Tcl_GetStringFromObj (objv[3], &i);
    result = script_ptr2str (weechat_config_search_option (script_str2ptr (config_file),
                                                           script_str2ptr (section),
                                                           option_name));
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_config_string_to_boolean: return boolean value of a string
 */

static int
weechat_tcl_api_config_string_to_boolean (ClientData clientData, Tcl_Interp *interp,
                                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    int result, i; 
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_string_to_boolean");
	TCL_RETURN_INT(0);
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_string_to_boolean");
        TCL_RETURN_INT(0);
    }
    
    result = weechat_config_string_to_boolean (Tcl_GetStringFromObj (objv[1], &i)); /* text */
    
    TCL_RETURN_INT(result);
}

/*
 * weechat_tcl_api_config_option_reset: reset an option with default value
 */

static int
weechat_tcl_api_config_option_reset (ClientData clientData, Tcl_Interp *interp,
                                     int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    int rc;
    char *option;
    int i,run_cb;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_option_reset");
	TCL_RETURN_INT(0);
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_option_reset");
        TCL_RETURN_INT(0);
    }

    if (Tcl_GetIntFromObj (interp, objv[2], &run_cb) != TCL_OK)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_option_reset");
        TCL_RETURN_INT(0);
    }
    
    option = Tcl_GetStringFromObj (objv[1], &i);
    rc = weechat_config_option_reset (script_str2ptr (option),
                                      run_cb); /* run_callback */
    
    TCL_RETURN_INT(rc);
}

/*
 * weechat_tcl_api_config_option_set: set new value for option
 */

static int
weechat_tcl_api_config_option_set (ClientData clientData, Tcl_Interp *interp,
                                   int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    int rc;
    char *option, *new_value;
    int i, run_callback;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_option_set");
	TCL_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    if (objc < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_option_set");
        TCL_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }

    if (Tcl_GetIntFromObj (interp, objv[3], &run_callback) != TCL_OK)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_option_set");
        TCL_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }

    option = Tcl_GetStringFromObj (objv[1], &i);
    new_value = Tcl_GetStringFromObj (objv[2], &i);
    rc = weechat_config_option_set (script_str2ptr (option),
                                    new_value,
                                    run_callback);
    
    TCL_RETURN_INT(rc);
}

/*
 * weechat_tcl_api_config_option_set_null: set null (undefined)value for option
 */

static int
weechat_tcl_api_config_option_set_null (ClientData clientData, Tcl_Interp *interp,
                                        int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    int rc;
    char *option;
    int i, run_callback;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_option_set_null");
	TCL_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_option_set_null");
        TCL_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }

    if (Tcl_GetIntFromObj (interp, objv[2], &run_callback) != TCL_OK)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_option_set_null");
        TCL_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    option = Tcl_GetStringFromObj (objv[1], &i);
    rc = weechat_config_option_set_null (script_str2ptr (option),
                                         run_callback);
    
    TCL_RETURN_INT(rc);
}

/*
 * weechat_tcl_api_config_option_unset: unset an option
 */

static int
weechat_tcl_api_config_option_unset (ClientData clientData, Tcl_Interp *interp,
                                     int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    int rc;
    char *option;
    int i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_option_unset");
	TCL_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_option_unset");
        TCL_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    option = Tcl_GetStringFromObj (objv[1], &i);
    rc = weechat_config_option_unset (script_str2ptr (option));
    
    TCL_RETURN_INT(rc);
}

/*
 * weechat_tcl_api_config_option_rename: rename an option
 */

static int
weechat_tcl_api_config_option_rename (ClientData clientData, Tcl_Interp *interp,
                                      int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *option, *new_name;
    int i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_option_rename");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_option_rename");
        TCL_RETURN_ERROR;
    }
    
    option = Tcl_GetStringFromObj (objv[1], &i);
    new_name = Tcl_GetStringFromObj (objv[2], &i);
    
    weechat_config_option_rename (script_str2ptr (option),
                                  new_name);
    
    TCL_RETURN_OK;
}

/*
 * weechat_tcl_api_config_option_is_null: return 1 if value of option is null
 */

static int
weechat_tcl_api_config_option_is_null (ClientData clientData, Tcl_Interp *interp,
                                       int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    int result, i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_option_is_null");
	TCL_RETURN_INT(1);
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_option_is_null");
        TCL_RETURN_INT(1);
    }
    
    result = weechat_config_option_is_null (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* option */
    
    TCL_RETURN_INT(result);
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
    Tcl_Obj* objp;
    int result, i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_option_default_is_null");
	TCL_RETURN_INT(1);
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_option_default_is_null");
        TCL_RETURN_INT(1);
    }
    
    result = weechat_config_option_default_is_null (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* option */
    
    TCL_RETURN_INT(result);
}

/*
 * weechat_tcl_api_config_boolean: return boolean value of option
 */

static int
weechat_tcl_api_config_boolean (ClientData clientData, Tcl_Interp *interp,
                                int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    int result, i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_boolean");
	TCL_RETURN_INT(0);
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_boolean");
        TCL_RETURN_INT(0);
    }
    
    result = weechat_config_boolean (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* option */
    
    TCL_RETURN_INT(result);
}

/*
 * weechat_tcl_api_config_integer: return integer value of option
 */

static int
weechat_tcl_api_config_integer (ClientData clientData, Tcl_Interp *interp,
                                int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    int result, i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_integer");
	TCL_RETURN_INT(0);
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_integer");
        TCL_RETURN_INT(0);
    }
    
    result = weechat_config_integer (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* option */
    
    TCL_RETURN_INT(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_string");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_string");
        TCL_RETURN_EMPTY;
    }
    
    result = weechat_config_string (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* option */
    
    TCL_RETURN_STRING(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_color");
	TCL_RETURN_INT(0);
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_color");
        TCL_RETURN_INT(0);
    }
    
    result = weechat_config_color (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* option */
    
    TCL_RETURN_STRING(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_write_option");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_write_option");
        TCL_RETURN_ERROR;
    }
    
    config_file = Tcl_GetStringFromObj (objv[1], &i);
    option = Tcl_GetStringFromObj (objv[2], &i);
    weechat_config_write_option (script_str2ptr (config_file),
                                 script_str2ptr (option));
    
    TCL_RETURN_OK;
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_write_line");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_write_line");
        TCL_RETURN_ERROR;
    }
    
    config_file = Tcl_GetStringFromObj (objv[1], &i);
    option_name = Tcl_GetStringFromObj (objv[2], &i);
    value = Tcl_GetStringFromObj (objv[3], &i);
    weechat_config_write_line (script_str2ptr (config_file), option_name,
                               "%s", value);
    
    TCL_RETURN_OK;
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_write");
	TCL_RETURN_INT(-1);
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_write");
        TCL_RETURN_INT(-1);
    }
    
    rc = weechat_config_write (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* config_file */
    
    TCL_RETURN_INT(rc);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_read");
	TCL_RETURN_INT(-1);
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_read");
        TCL_RETURN_INT(-1);
    }
    
    rc = weechat_config_read (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* config_file */
    
    TCL_RETURN_INT(rc);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_reload");
	TCL_RETURN_INT(-1);
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_reload");
        TCL_RETURN_INT(-1);
    }
    
    rc = weechat_config_reload (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* config_file */
    
    TCL_RETURN_INT(rc);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_free");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_free");
        TCL_RETURN_ERROR;
    }
    
    script_api_config_free (weechat_tcl_plugin,
                            tcl_current_script,
                            script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* config_file */
    
    TCL_RETURN_OK;
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_get");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_get");
        TCL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_config_get (Tcl_GetStringFromObj (objv[1], &i)));
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_get_plugin");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_get_plugin");
        TCL_RETURN_EMPTY;
    }
    
    result = script_api_config_get_plugin (weechat_tcl_plugin,
                                           tcl_current_script,
                                           Tcl_GetStringFromObj (objv[1], &i));
    
    TCL_RETURN_STRING(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_set_plugin");
	TCL_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_set_plugin");
        TCL_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    option = Tcl_GetStringFromObj (objv[1], &i);
    value = Tcl_GetStringFromObj (objv[2], &i);
    
    rc = script_api_config_set_plugin (weechat_tcl_plugin,
                                       tcl_current_script,
                                       option,
                                       value);
    
    TCL_RETURN_INT(rc);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_unset_plugin");
	TCL_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_unset_plugin");
        TCL_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    option = Tcl_GetStringFromObj (objv[1], &i);
    
    rc = script_api_config_unset_plugin (weechat_tcl_plugin,
                                         tcl_current_script,
                                         option);
    
    TCL_RETURN_INT(rc);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("prefix");
        TCL_RETURN_EMPTY;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("prefix");
        TCL_RETURN_EMPTY;
    }
    
    result = weechat_prefix (Tcl_GetStringFromObj (objv[1], &i)); /* prefix */
    
    TCL_RETURN_STRING(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("color");
        TCL_RETURN_EMPTY;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("color");
        TCL_RETURN_EMPTY;
    }
    
    result = weechat_color (Tcl_GetStringFromObj (objv[1], &i)); /* color */
    
    TCL_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_print: print message in a buffer
 */

static int
weechat_tcl_api_print (ClientData clientData, Tcl_Interp *interp,
                       int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *buffer, *message;
    int i;

    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("print");
        TCL_RETURN_ERROR;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("print");
        TCL_RETURN_ERROR;
    }
    
    buffer = Tcl_GetStringFromObj (objv[1], &i);
    message = Tcl_GetStringFromObj (objv[2], &i);
    script_api_printf (weechat_tcl_plugin,
                       tcl_current_script,
                       script_str2ptr (buffer),
                       "%s", message);
    
    TCL_RETURN_OK;
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
    int i,tdate;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("print_date_tags");
        TCL_RETURN_ERROR;
    }
    
    if (objc < 5)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("print_date_tags");
        TCL_RETURN_ERROR;
    }

    if (Tcl_GetIntFromObj (interp, objv[2], &tdate) != TCL_OK)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("print_date_tags");
	TCL_RETURN_EMPTY;
    }   

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    tags = Tcl_GetStringFromObj (objv[3], &i);
    message = Tcl_GetStringFromObj (objv[4], &i);
    script_api_printf_date_tags (weechat_tcl_plugin,
                                 tcl_current_script,
                                 script_str2ptr (buffer),
                                 tdate,
                                 tags,
                                 "%s", message);
    
    TCL_RETURN_OK;
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
    int i,y;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("print_y");
        TCL_RETURN_ERROR;
    }
    
    if (objc < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("print_y");
        TCL_RETURN_ERROR;
    }
   
    if (Tcl_GetIntFromObj (interp, objv[2], &y) != TCL_OK)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("print_y");
        TCL_RETURN_ERROR;
    }	

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    message = Tcl_GetStringFromObj (objv[3], &i);
    script_api_printf_y (weechat_tcl_plugin,
                         tcl_current_script,
                         script_str2ptr (buffer),
                         y,
                         "%s", message);
    
    TCL_RETURN_OK;
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

    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("log_print");
        TCL_RETURN_ERROR;
    }

    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("log_print");
        TCL_RETURN_ERROR;
    }
    
    script_api_log_printf (weechat_tcl_plugin,
                           tcl_current_script,
                           "%s", Tcl_GetStringFromObj (objv[1], &i)); /* message */
    
    TCL_RETURN_OK;
}

/*
 * weechat_tcl_api_hook_command_cb: callback for command hooked
 */

int
weechat_tcl_api_hook_command_cb (void *data, struct t_gui_buffer *buffer,
                                  int argc, char **argv, char **argv_eol)
{
    struct t_script_callback *script_callback;
    char *tcl_argv[3], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    /* make C compiler happy */
    (void) argv;
    
    script_callback = (struct t_script_callback *)data;
    
    tcl_argv[0] = script_ptr2str (buffer);
    tcl_argv[1] = (argc > 1) ? argv_eol[1] : empty_arg;
    tcl_argv[2] = NULL;
    
    rc = (int *) weechat_tcl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    tcl_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (tcl_argv[0])
        free (tcl_argv[0]);
    
    return ret;
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
    char *completion, *function;
    int i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_command");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 7)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_command");
        TCL_RETURN_EMPTY;
    }

    command = Tcl_GetStringFromObj (objv[1], &i);
    description = Tcl_GetStringFromObj (objv[2], &i);
    args = Tcl_GetStringFromObj (objv[3], &i);
    args_description = Tcl_GetStringFromObj (objv[4], &i);
    completion = Tcl_GetStringFromObj (objv[5], &i);
    function = Tcl_GetStringFromObj (objv[6], &i);
    result = script_ptr2str (script_api_hook_command (weechat_tcl_plugin,
                                                      tcl_current_script,
                                                      command,
                                                      description,
                                                      args,
                                                      args_description,
                                                      completion,
                                                      &weechat_tcl_api_hook_command_cb,
                                                      function));
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_timer_cb: callback for timer hooked
 */

int
weechat_tcl_api_hook_timer_cb (void *data)
{
    struct t_script_callback *script_callback;
    char *tcl_argv[1];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    tcl_argv[0] = NULL;
    
    rc = (int *) weechat_tcl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    tcl_argv);
    
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
 * weechat_tcl_api_hook_timer: hook a timer
 */

static int
weechat_tcl_api_hook_timer (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;
    int i,interval,align_second,max_calls;

    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_timer");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 5)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_timer");
        TCL_RETURN_EMPTY;
    }
    
    if ((Tcl_GetIntFromObj (interp, objv[1], &interval) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[2], &align_second) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[3], &max_calls) != TCL_OK))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_timer");
        TCL_RETURN_EMPTY;
    }

    
    result = script_ptr2str (script_api_hook_timer (weechat_tcl_plugin,
                                                    tcl_current_script,
                                                    interval, /* interval */
                                                    align_second, /* align_second */
                                                    max_calls, /* max_calls */
                                                    &weechat_tcl_api_hook_timer_cb,
                                                    Tcl_GetStringFromObj (objv[4], &i))); /* tcl function */
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_fd_cb: callback for fd hooked
 */

int
weechat_tcl_api_hook_fd_cb (void *data)
{
    struct t_script_callback *script_callback;
    char *tcl_argv[1];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    tcl_argv[0] = NULL;
    
    rc = (int *) weechat_tcl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    tcl_argv);
    
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
 * weechat_tcl_api_hook_fd: hook a fd
 */

static int
weechat_tcl_api_hook_fd (ClientData clientData, Tcl_Interp *interp,
                         int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result;
    int i,fd,read,write,exception;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_fd");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 6)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_fd");
        TCL_RETURN_EMPTY;
    }
    
    if ((Tcl_GetIntFromObj (interp, objv[1], &fd) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[2], &read) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[3], &write) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[4], &exception) != TCL_OK))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_fd");
        TCL_RETURN_EMPTY;
    }

    result = script_ptr2str (script_api_hook_fd (weechat_tcl_plugin,
                                                 tcl_current_script,
                                                 fd, /* fd */
                                                 read, /* read */
                                                 write, /* write */
                                                 exception, /* exception */
                                                 &weechat_tcl_api_hook_fd_cb,
                                                 Tcl_GetStringFromObj (objv[5], &i))); /* tcl function */
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_connect_cb: callback for connect hooked
 */

int
weechat_tcl_api_hook_connect_cb (void *data, int status,
                                 const char *ip_address)
{
    struct t_script_callback *script_callback;
    char *tcl_argv[3], str_status[32];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    snprintf (str_status, sizeof (str_status), "%d", status);
    
    tcl_argv[0] = str_status;
    tcl_argv[1] = (char *)ip_address;
    tcl_argv[2] = NULL;
    
    rc = (int *) weechat_tcl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    tcl_argv);
    
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
 * weechat_tcl_api_hook_connect: hook a connection
 */

static int
weechat_tcl_api_hook_connect (ClientData clientData, Tcl_Interp *interp,
                              int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *proxy, *address, *local_hostname, *result;
    int i, port, sock, ipv6;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_connect");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 7)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_connect");
        TCL_RETURN_EMPTY;
    }
    
    if ((Tcl_GetIntFromObj (interp, objv[3], &port) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[4], &sock) != TCL_OK)
        || (Tcl_GetIntFromObj (interp, objv[5], &ipv6) != TCL_OK))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_connect");
        TCL_RETURN_EMPTY;
    }
    
    proxy = Tcl_GetStringFromObj (objv[1], &i);
    address = Tcl_GetStringFromObj (objv[2], &i);
    local_hostname = Tcl_GetStringFromObj (objv[6], &i);
    
    result = script_ptr2str (script_api_hook_connect (weechat_tcl_plugin,
                                                      tcl_current_script,
                                                      proxy,
                                                      address,
                                                      port,
                                                      sock,
                                                      ipv6,
                                                      NULL, /* gnutls session */
                                                      local_hostname,
                                                      &weechat_tcl_api_hook_connect_cb,
                                                      Tcl_GetStringFromObj (objv[7], &i))); /* tcl function */
    
    TCL_RETURN_STRING_FREE(result);
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
    char *tcl_argv[8];
    static char timebuffer[64];
    int *rc, ret;
    
    /* make C compiler happy */
    (void) tags_count;
    
    script_callback = (struct t_script_callback *)data;
    
    snprintf (timebuffer, sizeof (timebuffer) - 1, "%ld", (long int)date);
    
    tcl_argv[0] = script_ptr2str (buffer);
    tcl_argv[1] = timebuffer;
    tcl_argv[2] = weechat_string_build_with_exploded (tags, ",");
    if (!tcl_argv[2])
        tcl_argv[2] = strdup ("");
    tcl_argv[3] = (displayed) ? strdup ("1") : strdup ("0");
    tcl_argv[4] = (highlight) ? strdup ("1") : strdup ("0");
    tcl_argv[5] = (char *)prefix;
    tcl_argv[6] = (char *)message;
    tcl_argv[7] = NULL;
    
    rc = (int *) weechat_tcl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    tcl_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (tcl_argv[0])
        free (tcl_argv[0]);
    if (tcl_argv[2])
        free (tcl_argv[2]);
    if (tcl_argv[3])
        free (tcl_argv[3]);
    if (tcl_argv[4])
        free (tcl_argv[4]);
    
    return ret;
}

/*
 * weechat_tcl_api_hook_print: hook a print
 */

static int
weechat_tcl_api_hook_print (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *buffer, *tags, *message, *function;
    int i,strip_colors;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_print");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 6)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_print");
        TCL_RETURN_EMPTY;
    }

    if (Tcl_GetIntFromObj (interp, objv[4], &strip_colors) != TCL_OK)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_print");
        TCL_RETURN_EMPTY;
    }
 
    buffer = Tcl_GetStringFromObj (objv[1], &i);
    tags = Tcl_GetStringFromObj (objv[2], &i);
    message = Tcl_GetStringFromObj (objv[3], &i);
    function = Tcl_GetStringFromObj (objv[5], &i);
    result = script_ptr2str (script_api_hook_print (weechat_tcl_plugin,
                                                    tcl_current_script,
                                                    script_str2ptr (buffer),
                                                    tags,
                                                    message,
                                                    strip_colors, /* strip_colors */
                                                    &weechat_tcl_api_hook_print_cb,
                                                    function));
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_signal_cb: callback for signal hooked
 */

int
weechat_tcl_api_hook_signal_cb (void *data, const char *signal, const char *type_data,
                                 void *signal_data)
{
    struct t_script_callback *script_callback;
    char *tcl_argv[3];
    static char value_str[64], empty_value[1] = { '\0' };
    int *rc, ret, free_needed;
    
    script_callback = (struct t_script_callback *)data;
    
    tcl_argv[0] = (char *)signal;
    free_needed = 0;
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        tcl_argv[1] = (signal_data) ? (char *)signal_data : empty_value;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        snprintf (value_str, sizeof (value_str) - 1,
                  "%d", *((int *)signal_data));
        tcl_argv[1] = value_str;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        tcl_argv[1] = script_ptr2str (signal_data);
        free_needed = 1;
    }
    else
        tcl_argv[1] = NULL;
    tcl_argv[2] = NULL;
    
    rc = (int *) weechat_tcl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    tcl_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (free_needed && tcl_argv[1])
        free (tcl_argv[1]);
    
    return ret;
}

/*
 * weechat_tcl_api_hook_signal: hook a signal
 */

static int
weechat_tcl_api_hook_signal (ClientData clientData, Tcl_Interp *interp,
                             int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *signal, *function;
    int i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_signal");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_signal");
        TCL_RETURN_EMPTY;
    }
    
    signal = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    result = script_ptr2str (script_api_hook_signal (weechat_tcl_plugin,
                                                     tcl_current_script,
                                                     signal,
                                                     &weechat_tcl_api_hook_signal_cb,
                                                     function));
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_signal_send");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_signal_send");
        TCL_RETURN_ERROR;
    }
    
    signal = Tcl_GetStringFromObj (objv[1], &i);
    type_data = Tcl_GetStringFromObj (objv[2], &i);
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        weechat_hook_signal_send (signal,
                                  type_data,
                                  Tcl_GetStringFromObj (objv[3], &i)); /* signal_data */
        TCL_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
	if (Tcl_GetIntFromObj (interp, objv[3], &number) != TCL_OK)
	{
	   TCL_RETURN_ERROR; 
	}
        weechat_hook_signal_send (signal,
                                  type_data,
                                  &number); /* signal_data */
        TCL_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        weechat_hook_signal_send (signal,
                                  type_data,
                                  Tcl_GetStringFromObj (objv[3], &i)); /* signal_data */
        TCL_RETURN_OK;
    }
    
    TCL_RETURN_ERROR;
}

/*
 * weechat_tcl_api_hook_config_cb: callback for config option hooked
 */

int
weechat_tcl_api_hook_config_cb (void *data, const char *option, const char *value)
{
    struct t_script_callback *script_callback;
    char *tcl_argv[3];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    tcl_argv[0] = (char *)option;
    tcl_argv[1] = (char *)value;
    tcl_argv[2] = NULL;
    
    rc = (int *) weechat_tcl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    tcl_argv);
    
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
 * weechat_tcl_api_hook_config: hook a config option
 */

static int
weechat_tcl_api_hook_config (ClientData clientData, Tcl_Interp *interp,
                             int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *option, *function;
    int i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_config");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_config");
        TCL_RETURN_EMPTY;
    }
    
    option = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    result = script_ptr2str (script_api_hook_config (weechat_tcl_plugin,
                                                     tcl_current_script,
                                                     option,
                                                     &weechat_tcl_api_hook_config_cb,
                                                     function));
    
    TCL_RETURN_STRING_FREE(result);
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
    char *tcl_argv[4];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    tcl_argv[0] = (char *)completion_item;
    tcl_argv[1] = script_ptr2str (buffer);
    tcl_argv[2] = script_ptr2str (completion);
    tcl_argv[3] = NULL;
    
    rc = (int *) weechat_tcl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    tcl_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (tcl_argv[1])
        free (tcl_argv[1]);
    if (tcl_argv[2])
        free (tcl_argv[2]);
    
    return ret;
}

/*
 * weechat_tcl_api_hook_completion: hook a completion
 */

static int
weechat_tcl_api_hook_completion (ClientData clientData, Tcl_Interp *interp,
                                 int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *completion, *function;
    int i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_completion");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_completion");
        TCL_RETURN_EMPTY;
    }
    
    completion = Tcl_GetStringFromObj (objv[1], &i);
    function = Tcl_GetStringFromObj (objv[2], &i);
    result = script_ptr2str (script_api_hook_completion (weechat_tcl_plugin,
                                                         tcl_current_script,
                                                         completion,
                                                         &weechat_tcl_api_hook_completion_cb,
                                                         function));
    
    TCL_RETURN_STRING_FREE(result);
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
    int i,nick_completion;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_completion_list_add");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 5)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_completion_list_add");
        TCL_RETURN_ERROR;
    }
   
    if (Tcl_GetIntFromObj (interp, objv[3], &nick_completion) != TCL_OK)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_completion_list_add");
        TCL_RETURN_ERROR;
    }

    completion = Tcl_GetStringFromObj (objv[1], &i);
    word = Tcl_GetStringFromObj (objv[2], &i);
    where = Tcl_GetStringFromObj (objv[4], &i);
    
    weechat_hook_completion_list_add (script_str2ptr (completion),
                                      word,
                                      nick_completion, /* nick_completion */
                                      where);
    
    TCL_RETURN_OK;
}

/*
 * weechat_tcl_api_hook_modifier_cb: callback for modifier hooked
 */

char *
weechat_tcl_api_hook_modifier_cb (void *data, const char *modifier,
                                   const char *modifier_data, const char *string)
{
    struct t_script_callback *script_callback;
    char *tcl_argv[4];
    
    script_callback = (struct t_script_callback *)data;
    
    tcl_argv[0] = (char *)modifier;
    tcl_argv[1] = (char *)modifier_data;
    tcl_argv[2] = (char *)string;
    tcl_argv[3] = NULL;
    
    return (char *)weechat_tcl_exec (script_callback->script,
                                      WEECHAT_SCRIPT_EXEC_STRING,
                                      script_callback->function,
                                      tcl_argv);
}

/*
 * weechat_tcl_api_hook_modifier: hook a modifier
 */

static int
weechat_tcl_api_hook_modifier (ClientData clientData, Tcl_Interp *interp,
                               int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *modifier, *tcl_fn;
    int i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_modifier");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_modifier");
        TCL_RETURN_EMPTY;
    }
    
    modifier = Tcl_GetStringFromObj (objv[1], &i);
    tcl_fn = Tcl_GetStringFromObj (objv[2], &i);
    result = script_ptr2str (script_api_hook_modifier (weechat_tcl_plugin,
                                                       tcl_current_script,
                                                       modifier,
                                                       &weechat_tcl_api_hook_modifier_cb,
                                                       tcl_fn));
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_modifier_exec");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_modifier_exec");
        TCL_RETURN_EMPTY;
    }
    
    modifier = Tcl_GetStringFromObj (objv[1], &i);
    modifier_data = Tcl_GetStringFromObj (objv[2], &i);
    string = Tcl_GetStringFromObj (objv[3], &i);
    result = weechat_hook_modifier_exec (modifier, modifier_data, string);
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_info_cb: callback for info hooked
 */

const char *
weechat_tcl_api_hook_info_cb (void *data, const char *info_name,
                               const char *arguments)
{
    struct t_script_callback *script_callback;
    char *tcl_argv[3];
    
    script_callback = (struct t_script_callback *)data;
    
    tcl_argv[0] = (char *)info_name;
    tcl_argv[1] = (char *)arguments;
    tcl_argv[2] = NULL;
    
    return (const char *)weechat_tcl_exec (script_callback->script,
                                           WEECHAT_SCRIPT_EXEC_STRING,
                                           script_callback->function,
                                           tcl_argv);
}

/*
 * weechat_tcl_api_hook_info: hook an info
 */

static int
weechat_tcl_api_hook_info (ClientData clientData, Tcl_Interp *interp,
                           int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *info_name, *description, *tcl_fn;
    int i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_info");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_info");
        TCL_RETURN_EMPTY;
    }
    
    info_name = Tcl_GetStringFromObj (objv[1], &i);
    description = Tcl_GetStringFromObj (objv[2], &i);
    tcl_fn = Tcl_GetStringFromObj (objv[3], &i);
    result = script_ptr2str (script_api_hook_info (weechat_tcl_plugin,
                                                   tcl_current_script,
                                                   info_name,
                                                   description,
                                                   &weechat_tcl_api_hook_info_cb,
                                                   tcl_fn));
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_hook_infolist_cb: callback for infolist hooked
 */

struct t_infolist *
weechat_tcl_api_hook_infolist_cb (void *data, const char *infolist_name,
                                   void *pointer, const char *arguments)
{
    struct t_script_callback *script_callback;
    char *tcl_argv[4];
    struct t_infolist *result;
    
    script_callback = (struct t_script_callback *)data;
    
    tcl_argv[0] = (char *)infolist_name;
    tcl_argv[1] = script_ptr2str (pointer);
    tcl_argv[2] = (char *)arguments;
    tcl_argv[3] = NULL;
    
    result = (struct t_infolist *)weechat_tcl_exec (script_callback->script,
                                                    WEECHAT_SCRIPT_EXEC_STRING,
                                                    script_callback->function,
                                                    tcl_argv);
    
    if (tcl_argv[1])
        free (tcl_argv[1]);
    
    return result;
}

/*
 * weechat_tcl_api_hook_infolist: hook an infolist
 */

static int
weechat_tcl_api_hook_infolist (ClientData clientData, Tcl_Interp *interp,
                               int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *infolist_name, *description, *tcl_fn;
    int i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_infolist");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_infolist");
        TCL_RETURN_EMPTY;
    }
    
    infolist_name = Tcl_GetStringFromObj (objv[1], &i);
    description = Tcl_GetStringFromObj (objv[2], &i);
    tcl_fn = Tcl_GetStringFromObj (objv[3], &i);
    result = script_ptr2str (script_api_hook_infolist (weechat_tcl_plugin,
                                                       tcl_current_script,
                                                       infolist_name,
                                                       description,
                                                       &weechat_tcl_api_hook_infolist_cb,
                                                       tcl_fn));
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("unhook");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("unhook");
        TCL_RETURN_ERROR;
    }
    
    script_api_unhook (weechat_tcl_plugin,
                       tcl_current_script,
                       script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* hook */
    
    TCL_RETURN_OK;
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
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("unhook_all");
	TCL_RETURN_ERROR;
    }
    
    script_api_unhook_all (tcl_current_script);
    
    TCL_RETURN_OK;
}

/*
 * weechat_tcl_api_buffer_input_data_cb: callback for input data in a buffer
 */

int
weechat_tcl_api_buffer_input_data_cb (void *data, struct t_gui_buffer *buffer,
                                       const char *input_data)
{
    struct t_script_callback *script_callback;
    char *tcl_argv[3];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    tcl_argv[0] = script_ptr2str (buffer);
    tcl_argv[1] = (char *)input_data;
    tcl_argv[2] = NULL;
    
    rc = (int *) weechat_tcl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    tcl_argv);
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (tcl_argv[0])
        free (tcl_argv[0]);
    
    return ret;
}

/*
 * weechat_tcl_api_buffer_close_cb: callback for buffer closed
 */

int
weechat_tcl_api_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    struct t_script_callback *script_callback;
    char *tcl_argv[2];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    tcl_argv[0] = script_ptr2str (buffer);
    tcl_argv[1] = NULL;
    
    rc = (int *) weechat_tcl_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script_callback->function,
                                    tcl_argv);
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (tcl_argv[0])
        free (tcl_argv[0]);
    
    return ret;
}

/*
 * weechat_tcl_api_buffer_new: create a new buffer
 */

static int
weechat_tcl_api_buffer_new (ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *name, *function_input, *function_close;
    int i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_new");
        TCL_RETURN_EMPTY;
    }
    
    if (objc < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_new");
        TCL_RETURN_EMPTY;
    }
    
    name = Tcl_GetStringFromObj (objv[1], &i);
    function_input = Tcl_GetStringFromObj (objv[2], &i);
    function_close = Tcl_GetStringFromObj (objv[3], &i);
    result = script_ptr2str (script_api_buffer_new (weechat_tcl_plugin,
                                                    tcl_current_script,
                                                    name,
                                                    &weechat_tcl_api_buffer_input_data_cb,
                                                    function_input,
                                                    &weechat_tcl_api_buffer_close_cb,
                                                    function_close));
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_search");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_search");
        TCL_RETURN_EMPTY;
    }
    
    plugin = Tcl_GetStringFromObj (objv[1], &i);
    name = Tcl_GetStringFromObj (objv[2], &i);
    result = script_ptr2str (weechat_buffer_search (plugin, name));
    
    TCL_RETURN_STRING_FREE(result);
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
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("current_buffer");
	TCL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_current_buffer ());
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_clear");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_clear");
        TCL_RETURN_ERROR;
    }
    
    weechat_buffer_clear (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* buffer */
    
    TCL_RETURN_OK;
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_close");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_close");
        TCL_RETURN_ERROR;
    }
    
    script_api_buffer_close (weechat_tcl_plugin,
                             tcl_current_script,
                             script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* buffer */
    
    TCL_RETURN_OK;
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_get_integer");
	TCL_RETURN_INT(-1);
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_get_integer");
        TCL_RETURN_INT(-1);
    }
    
    buffer = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);
    
    result = weechat_buffer_get_integer (script_str2ptr (buffer), property);
    
    TCL_RETURN_INT(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_get_string");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_get_string");
        TCL_RETURN_EMPTY;
    }
    
    buffer = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);
    
    result = weechat_buffer_get_string (script_str2ptr (buffer), property);
    
    TCL_RETURN_STRING(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_get_pointer");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_get_pointer");
        TCL_RETURN_EMPTY;
    }
    
    buffer = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);
    
    result = script_ptr2str (weechat_buffer_get_pointer (script_str2ptr (buffer),
                                                         property));
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_set");
        TCL_RETURN_ERROR;
    }
    
    if (objc < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_set");
        TCL_RETURN_ERROR;
    }
    
    buffer = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);
    value = Tcl_GetStringFromObj (objv[3], &i);
    
    weechat_buffer_set (script_str2ptr (buffer), property, value);
    
    TCL_RETURN_OK;
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
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("current_window");
	TCL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_current_window ());
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("window_get_integer");
	TCL_RETURN_INT(-1);
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("window_get_integer");
        TCL_RETURN_INT(-1);
    }
    
    window = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);
    
    result = weechat_window_get_integer (script_str2ptr (window), property);
    
    TCL_RETURN_INT(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("window_get_string");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("window_get_string");
        TCL_RETURN_EMPTY;
    }
    
    window = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);
    
    result = weechat_window_get_string (script_str2ptr (window), property);
    
    TCL_RETURN_STRING(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("window_get_pointer");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("window_get_pointer");
        TCL_RETURN_EMPTY;
    }
    
    window = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);
    
    result = script_ptr2str (weechat_window_get_pointer (script_str2ptr (window),
                                                         property));
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_nicklist_add_group: add a group in nicklist
 */

static int
weechat_tcl_api_nicklist_add_group (ClientData clientData, Tcl_Interp *interp,
                                    int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *result, *buffer, *parent_group, *name, *color;
    int i,visible;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_add_group");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 6)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_add_group");
        TCL_RETURN_EMPTY;
    }

    if (Tcl_GetIntFromObj (interp, objv[5], &visible) != TCL_OK)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_add_group");
        TCL_RETURN_EMPTY;
    }
    
    buffer = Tcl_GetStringFromObj (objv[1], &i);
    parent_group = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);
    color = Tcl_GetStringFromObj (objv[4], &i);
    result = script_ptr2str (weechat_nicklist_add_group (script_str2ptr (buffer),
                                                         script_str2ptr (parent_group),
                                                         name,
                                                         color,
                                                         visible)); /* visible */
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_nicklist_search_group: search a group in nicklist
 */

static int
weechat_tcl_api_nicklist_search_group (ClientData clientData, Tcl_Interp *interp,
                                       int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *result, *buffer, *from_group, *name;
    int i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_search_group");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_search_group");
        TCL_RETURN_EMPTY;
    }

    buffer = Tcl_GetStringFromObj (objv[1], &i);
    from_group = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);
    result = script_ptr2str (weechat_nicklist_search_group (script_str2ptr (buffer),
                                                            script_str2ptr (from_group),
                                                            name));
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_nicklist_add_nick: add a nick in nicklist
 */

static int
weechat_tcl_api_nicklist_add_nick (ClientData clientData, Tcl_Interp *interp,
                                   int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *prefix, char_prefix, *result, *buffer, *group, *name, *color;
    char *prefix_color;
    int i,visible;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_add_nick");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 8)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_add_nick");
        TCL_RETURN_EMPTY;
    }
    
    prefix = Tcl_GetStringFromObj (objv[5], &i);
    if (prefix && prefix[0])
        char_prefix = prefix[0];
    else
        char_prefix = ' ';

    if (Tcl_GetIntFromObj (interp, objv[7], &visible) != TCL_OK)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_add_nick");
        TCL_RETURN_EMPTY;
    }
    
    buffer = Tcl_GetStringFromObj (objv[1], &i);
    group = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);
    color = Tcl_GetStringFromObj (objv[4], &i);
    prefix_color = Tcl_GetStringFromObj (objv[6], &i);
    result = script_ptr2str (weechat_nicklist_add_nick (script_str2ptr (buffer),
                                                        script_str2ptr (group),
                                                        name,
                                                        color,
                                                        char_prefix,
                                                        prefix_color,
                                                        visible)); /* visible */
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_search_nick");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_search_nick");
        TCL_RETURN_EMPTY;
    }
    
    buffer = Tcl_GetStringFromObj (objv[1], &i);
    from_group = Tcl_GetStringFromObj (objv[2], &i);
    name = Tcl_GetStringFromObj (objv[3], &i);
    result = script_ptr2str (weechat_nicklist_search_nick (script_str2ptr (buffer),
                                                           script_str2ptr (from_group),
                                                           name));
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_remove_group");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_remove_group");
        TCL_RETURN_ERROR;
    }
    
    buffer = Tcl_GetStringFromObj (objv[1], &i);
    group = Tcl_GetStringFromObj (objv[2], &i);
    weechat_nicklist_remove_group (script_str2ptr (buffer), 
                                   script_str2ptr (group));
    
    TCL_RETURN_OK;
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_remove_nick");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_remove_nick");
        TCL_RETURN_ERROR;
    }
    
    buffer = Tcl_GetStringFromObj (objv[1], &i);
    nick = Tcl_GetStringFromObj (objv[2], &i);
    weechat_nicklist_remove_nick (script_str2ptr (buffer),
                                  script_str2ptr (nick));
    
    TCL_RETURN_OK;
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_remove_all");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_remove_all");
        TCL_RETURN_ERROR;
    }
    
    weechat_nicklist_remove_all (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* buffer */
    
    TCL_RETURN_OK;
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_item_search");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_item_search");
        TCL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_bar_item_search (Tcl_GetStringFromObj (objv[1], &i))); /* name */
    
    TCL_RETURN_STRING_FREE(result);
}

/*
 * weechat_tcl_api_bar_item_build_cb: callback for building bar item
 */

char *
weechat_tcl_api_bar_item_build_cb (void *data, struct t_gui_bar_item *item,
                                    struct t_gui_window *window,
                                    int max_width, int max_height)
{
    struct t_script_callback *script_callback;
    char *tcl_argv[5], *ret;
    static char str_width[32], str_height[32];
    
    script_callback = (struct t_script_callback *)data;
    
    snprintf (str_width, sizeof (str_width), "%d", max_width);
    snprintf (str_height, sizeof (str_height), "%d", max_height);
    
    tcl_argv[0] = script_ptr2str (item);
    tcl_argv[1] = script_ptr2str (window);
    tcl_argv[2] = str_width;
    tcl_argv[3] = str_height;
    tcl_argv[4] = NULL;
    
    ret = (char *)weechat_tcl_exec (script_callback->script,
                                     WEECHAT_SCRIPT_EXEC_STRING,
                                     script_callback->function,
                                     tcl_argv);
    
    if (tcl_argv[0])
        free (tcl_argv[0]);
    if (tcl_argv[1])
        free (tcl_argv[1]);
    
    return ret;
}

/*
 * weechat_tcl_api_bar_item_new: add a new bar item
 */

static int
weechat_tcl_api_bar_item_new (ClientData clientData, Tcl_Interp *interp,
                              int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    char *result, *name, *function_build;
    int i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_item_new");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_item_new");
        TCL_RETURN_EMPTY;
    }
    
    name = Tcl_GetStringFromObj (objv[1], &i);
    function_build = Tcl_GetStringFromObj (objv[2], &i);
    result = script_ptr2str (script_api_bar_item_new (weechat_tcl_plugin,
                                                      tcl_current_script,
                                                      name,
                                                      &weechat_tcl_api_bar_item_build_cb,
                                                      function_build));
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_item_update");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_item_update");
        TCL_RETURN_ERROR;
    }
    
    weechat_bar_item_update (Tcl_GetStringFromObj (objv[1], &i)); /* name */
    
    TCL_RETURN_OK;
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_item_remove");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_item_remove");
        TCL_RETURN_ERROR;
    }
    
    script_api_bar_item_remove (weechat_tcl_plugin,
                                tcl_current_script,
                                script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* item */
    
    TCL_RETURN_OK;
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_search");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_search");
        TCL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_bar_search (Tcl_GetStringFromObj (objv[1], &i))); /* name */
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_new");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 16)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_new");
        TCL_RETURN_EMPTY;
    }
    
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
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_set");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_set");
        TCL_RETURN_ERROR;
    }
    
    bar = Tcl_GetStringFromObj (objv[1], &i);
    property = Tcl_GetStringFromObj (objv[2], &i);
    value = Tcl_GetStringFromObj (objv[3], &i);
    
    weechat_bar_set (script_str2ptr (bar), property, value);
    
    TCL_RETURN_OK;
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_update");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_update");
        TCL_RETURN_ERROR;
    }
    
    weechat_bar_update (Tcl_GetStringFromObj (objv[1], &i)); /* name */
    
    TCL_RETURN_OK;
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_remove");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_remove");
        TCL_RETURN_ERROR;
    }
    
    weechat_bar_remove (script_str2ptr(Tcl_GetStringFromObj (objv[1], &i))); /* bar */
    
    TCL_RETURN_OK;
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("command");
        TCL_RETURN_ERROR;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("command");
        TCL_RETURN_ERROR;
    }
    
    buffer = Tcl_GetStringFromObj (objv[1], &i);
    command = Tcl_GetStringFromObj (objv[2], &i);
    script_api_command (weechat_tcl_plugin,
                        tcl_current_script,
                        script_str2ptr (buffer),
                        command);
    
    TCL_RETURN_OK;
}

/*
 * weechat_tcl_api_info_get: get info about WeeChat
 */

static int
weechat_tcl_api_info_get (ClientData clientData, Tcl_Interp *interp,
                          int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *objp;
    const char *result;
    int i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("info_get");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("info_get");
        TCL_RETURN_EMPTY;
    }
    
    result = weechat_info_get (Tcl_GetStringFromObj (objv[1], &i),
                               Tcl_GetStringFromObj (objv[2], &i));
    
    TCL_RETURN_STRING(result);
}

/*
 * weechat_tcl_api_infolist_new: create a new infolist
 */

static int
weechat_tcl_api_infolist_new (ClientData clientData, Tcl_Interp *interp,
                              int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj* objp;
    char *result;
    
    /* make C compiler happy */
    (void) clientData;
    (void) objc;
    (void) objv;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_new");
	TCL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_infolist_new ());
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_new_var_integer");
	TCL_RETURN_INT(0);
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_new_var_integer");
        TCL_RETURN_INT(0);
    }
    
    if (Tcl_GetIntFromObj (interp, objv[3], &value) != TCL_OK)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolits_new_var_integer");
        TCL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_infolist_new_var_integer (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i)), /* infolist */
                                                               Tcl_GetStringFromObj (objv[2], &i), /* name */
                                                               value));
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_new_var_string");
	TCL_RETURN_INT(0);
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_new_var_string");
        TCL_RETURN_INT(0);
    }
    
    result = script_ptr2str (weechat_infolist_new_var_string (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i)), /* infolist */
                                                              Tcl_GetStringFromObj (objv[2], &i), /* name */
                                                              Tcl_GetStringFromObj (objv[3], &i))); /* value */
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_new_var_pointer");
	TCL_RETURN_INT(0);
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_new_var_pointer");
        TCL_RETURN_INT(0);
    }
    
    result = script_ptr2str (weechat_infolist_new_var_pointer (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i)), /* infolist */
                                                               Tcl_GetStringFromObj (objv[2], &i), /* name */
                                                               script_str2ptr (Tcl_GetStringFromObj (objv[3], &i)))); /* value */
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_new_var_time");
	TCL_RETURN_INT(0);
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_new_var_time");
        TCL_RETURN_INT(0);
    }
    
    if (Tcl_GetIntFromObj (interp, objv[3], &value) != TCL_OK)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolits_new_var_time");
        TCL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_infolist_new_var_time (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i)), /* infolist */
                                                            Tcl_GetStringFromObj (objv[2], &i), /* name */
                                                            value));
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_get");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_get");
        TCL_RETURN_EMPTY;
    }
    
    name = Tcl_GetStringFromObj (objv[1], &i);
    pointer = Tcl_GetStringFromObj (objv[2], &i);
    arguments = Tcl_GetStringFromObj (objv[3], &i);
    result = script_ptr2str (weechat_infolist_get (name,
                                                   script_str2ptr (pointer),
                                                   arguments));
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_next");
	TCL_RETURN_INT(0);
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_next");
        TCL_RETURN_INT(0);
    }
    
    result = weechat_infolist_next (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* infolist */
    
    TCL_RETURN_INT(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_prev");
	TCL_RETURN_INT(0);
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_prev");
        TCL_RETURN_INT(0);
    }
    
    result = weechat_infolist_prev (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* infolist */
    
    TCL_RETURN_INT(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_fields");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_fields");
        TCL_RETURN_EMPTY;
    }
    
    result = weechat_infolist_fields (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* infolist */
    
    TCL_RETURN_STRING(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_integer");
	TCL_RETURN_INT(0);
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_integer");
        TCL_RETURN_INT(0);
    }
    
    infolist = Tcl_GetStringFromObj (objv[1], &i);
    variable = Tcl_GetStringFromObj (objv[2], &i);
    
    result = weechat_infolist_integer (script_str2ptr (infolist), variable);
    
    TCL_RETURN_INT(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_string");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_string");
        TCL_RETURN_EMPTY;
    }
    
    infolist = Tcl_GetStringFromObj (objv[1], &i);
    variable = Tcl_GetStringFromObj (objv[2], &i);
    
    result = weechat_infolist_string (script_str2ptr (infolist), variable);
    
    TCL_RETURN_STRING(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_pointer");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_pointer");
        TCL_RETURN_EMPTY;
    }
    
    infolist = Tcl_GetStringFromObj (objv[1], &i);
    variable = Tcl_GetStringFromObj (objv[2], &i);
    
    result = script_ptr2str (weechat_infolist_pointer (script_str2ptr (infolist), variable));
    
    TCL_RETURN_STRING_FREE(result);
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
    char timebuffer[64], *result, *infolist, *variable;
    int i;
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_time");
	TCL_RETURN_EMPTY;
    }
    
    if (objc < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_time");
        TCL_RETURN_EMPTY;
    }
    
    infolist = Tcl_GetStringFromObj (objv[1], &i);
    variable = Tcl_GetStringFromObj (objv[2], &i);
    time = weechat_infolist_time (script_str2ptr (infolist), variable);
    strftime (timebuffer, sizeof (timebuffer), "%F %T", localtime (&time));
    
    result = strdup (timebuffer);
    
    TCL_RETURN_STRING_FREE(result);
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
    
    /* make C compiler happy */
    (void) clientData;
    
    if (!tcl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_free");
	TCL_RETURN_ERROR;
    }
    
    if (objc < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_free");
        TCL_RETURN_ERROR;
    }
    
    weechat_infolist_free (script_str2ptr (Tcl_GetStringFromObj (objv[1], &i))); /* infolist */
    
    TCL_RETURN_OK;
}



/*
 * weechat_tcl_api_init: initialize subroutines
 */

void weechat_tcl_api_init (Tcl_Interp *interp) {
    int i;
    Tcl_Obj *objp;

    /* standard initializer */ 
    Tcl_Init (interp);
    
#ifdef HAVE_TCL_CREATE_NS
    /* create weechat namespace */
    Tcl_CreateNamespace (interp, "weechat",(ClientData)0,NULL);
#endif

    /* interface constants */
    /* set variables, TODO: make them unmodifiable (thru Tcl_TraceVar) ? */
    /* NOTE: it is not good for performance to convert "defines" to Tcl_Obj */
    objp = Tcl_NewIntObj (WEECHAT_RC_OK);
    Tcl_IncrRefCount (objp);

    Tcl_SetVar (interp, "weechat::WEECHAT_RC_OK", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_RC_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_RC_ERROR", Tcl_GetStringFromObj (objp, &i),0);
    
    Tcl_SetIntObj (objp,WEECHAT_CONFIG_READ_OK);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_READ_OK", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_CONFIG_READ_MEMORY_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_READ_MEMORY_ERROR", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_CONFIG_READ_FILE_NOT_FOUND);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_READ_FILE_NOT_FOUND", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_CONFIG_WRITE_OK);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_WRITE_OK", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_CONFIG_WRITE_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_WRITE_ERROR", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_CONFIG_WRITE_MEMORY_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_WRITE_MEMORY_ERROR", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_CONFIG_OPTION_SET_OK_CHANGED);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_OPTION_SET_OK_CHANGED", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_CONFIG_OPTION_SET_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_OPTION_SET_ERROR", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_CONFIG_OPTION_UNSET_OK_RESET);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_OPTION_UNSET_OK_RESET", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_CONFIG_OPTION_UNSET_ERROR", Tcl_GetStringFromObj (objp, &i),0);
    
    Tcl_SetStringObj (objp,WEECHAT_LIST_POS_SORT,-1);
    Tcl_SetVar (interp, "weechat::WEECHAT_LIST_POS_SORT", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetStringObj (objp,WEECHAT_LIST_POS_BEGINNING,-1);
    Tcl_SetVar (interp, "weechat::WEECHAT_LIST_POS_BEGINNING", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetStringObj (objp,WEECHAT_LIST_POS_END,-1);
    Tcl_SetVar (interp, "weechat::WEECHAT_LIST_POS_END", Tcl_GetStringFromObj (objp, &i),0);
    
    Tcl_SetStringObj (objp,WEECHAT_HOTLIST_LOW,-1);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOTLIST_LOW", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetStringObj (objp,WEECHAT_HOTLIST_MESSAGE,-1);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOTLIST_MESSAGE", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetStringObj (objp,WEECHAT_HOTLIST_PRIVATE,-1);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOTLIST_PRIVATE", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetStringObj (objp,WEECHAT_HOTLIST_HIGHLIGHT,-1);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOTLIST_HIGHLIGHT", Tcl_GetStringFromObj (objp, &i),0);
    
    Tcl_SetIntObj (objp,WEECHAT_HOOK_CONNECT_OK);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_OK", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_HOOK_CONNECT_PROXY_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_PROXY_ERROR", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetIntObj (objp,WEECHAT_HOOK_CONNECT_MEMORY_ERROR);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_CONNECT_MEMORY_ERROR", Tcl_GetStringFromObj (objp, &i),0);
    
    Tcl_SetStringObj (objp,WEECHAT_HOOK_SIGNAL_STRING,-1);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_SIGNAL_STRING", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetStringObj (objp,WEECHAT_HOOK_SIGNAL_INT,-1);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_SIGNAL_INT", Tcl_GetStringFromObj (objp, &i),0);
    Tcl_SetStringObj (objp,WEECHAT_HOOK_SIGNAL_POINTER,-1);
    Tcl_SetVar (interp, "weechat::WEECHAT_HOOK_SIGNAL_POINTER", Tcl_GetStringFromObj (objp, &i),0);

    Tcl_DecrRefCount (objp);
    
    /* interface functions */
    Tcl_CreateObjCommand (interp,"weechat::register",
                          weechat_tcl_api_register, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::plugin_get_name",
                          weechat_tcl_api_plugin_get_name, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::charset_set",
                          weechat_tcl_api_charset_set, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::iconv_to_internal",
                          weechat_tcl_api_iconv_to_internal, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::iconv_from_internal",
                          weechat_tcl_api_iconv_from_internal, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::gettext",
                          weechat_tcl_api_gettext, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::ngettext",
                          weechat_tcl_api_ngettext, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::mkdir_home",
                          weechat_tcl_api_mkdir_home, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::mkdir",
                          weechat_tcl_api_mkdir, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::mkdir_parents",
                          weechat_tcl_api_mkdir_parents, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::list_new",
                          weechat_tcl_api_list_new, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::list_add",
                          weechat_tcl_api_list_add, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::list_search",
                          weechat_tcl_api_list_search, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::list_casesearch",
                          weechat_tcl_api_list_casesearch, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::list_get",
                          weechat_tcl_api_list_get, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::list_set",
                          weechat_tcl_api_list_set, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::list_next",
                          weechat_tcl_api_list_next, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::list_prev",
                          weechat_tcl_api_list_prev, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::list_string",
                          weechat_tcl_api_list_string, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::list_size",
                          weechat_tcl_api_list_size, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::list_remove",
                          weechat_tcl_api_list_remove, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::list_remove_all",
                          weechat_tcl_api_list_remove_all, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::list_free",
                          weechat_tcl_api_list_free, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_new",
                          weechat_tcl_api_config_new, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_new_section",
                          weechat_tcl_api_config_new_section, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_search_section",
                          weechat_tcl_api_config_search_section, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_new_option",
                          weechat_tcl_api_config_new_option, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_search_option",
                          weechat_tcl_api_config_search_option, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_string_to_boolean",
                          weechat_tcl_api_config_string_to_boolean, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_option_reset",
                          weechat_tcl_api_config_option_reset, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_option_set",
                          weechat_tcl_api_config_option_set, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_option_set_null",
                          weechat_tcl_api_config_option_set_null, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_option_unset",
                          weechat_tcl_api_config_option_unset, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_option_rename",
                          weechat_tcl_api_config_option_rename, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_option_is_null",
                          weechat_tcl_api_config_option_is_null, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_option_default_is_null",
                          weechat_tcl_api_config_option_default_is_null, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_boolean",
                          weechat_tcl_api_config_boolean, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_integer",
                          weechat_tcl_api_config_integer, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_string",
                          weechat_tcl_api_config_string, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_color",
                          weechat_tcl_api_config_color, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_write_option",
                          weechat_tcl_api_config_write_option, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_write_line",
                          weechat_tcl_api_config_write_line, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_write",
                          weechat_tcl_api_config_write, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_read",
                          weechat_tcl_api_config_read, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_reload",
                          weechat_tcl_api_config_reload, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_free",
                          weechat_tcl_api_config_free, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_get",
                          weechat_tcl_api_config_get, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_get_plugin",
                          weechat_tcl_api_config_get_plugin, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_set_plugin",
                          weechat_tcl_api_config_set_plugin, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::config_unset_plugin",
                          weechat_tcl_api_config_unset_plugin, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::prefix",
                          weechat_tcl_api_prefix, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::color",
                          weechat_tcl_api_color, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::print",
                          weechat_tcl_api_print, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::print_date_tags",
                          weechat_tcl_api_print_date_tags, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::print_y",
                          weechat_tcl_api_print_y, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::log_print",
                          weechat_tcl_api_log_print, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::hook_command",
                          weechat_tcl_api_hook_command, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::hook_timer",
                          weechat_tcl_api_hook_timer, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::hook_fd",
                          weechat_tcl_api_hook_fd, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::hook_connect",
                          weechat_tcl_api_hook_connect, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::hook_print",
                          weechat_tcl_api_hook_print, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::hook_signal",
                          weechat_tcl_api_hook_signal, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::hook_signal_send",
                          weechat_tcl_api_hook_signal_send, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::hook_config",
                          weechat_tcl_api_hook_config, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::hook_completion",
                          weechat_tcl_api_hook_completion, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::hook_completion_list_add",
                          weechat_tcl_api_hook_completion_list_add, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::hook_modifier",
                          weechat_tcl_api_hook_modifier, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::hook_modifier_exec",
                          weechat_tcl_api_hook_modifier_exec, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::hook_info",
                          weechat_tcl_api_hook_info, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::hook_infolist",
                          weechat_tcl_api_hook_infolist, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::unhook",
                          weechat_tcl_api_unhook, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::unhook_all",
                          weechat_tcl_api_unhook_all, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::buffer_new",
                          weechat_tcl_api_buffer_new, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::buffer_search",
                          weechat_tcl_api_buffer_search, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::current_buffer",
                          weechat_tcl_api_current_buffer, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::buffer_clear",
                          weechat_tcl_api_buffer_clear, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::buffer_close",
                          weechat_tcl_api_buffer_close, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::buffer_get_integer",
                          weechat_tcl_api_buffer_get_integer, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::buffer_get_string",
                          weechat_tcl_api_buffer_get_string, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::buffer_get_pointer",
                          weechat_tcl_api_buffer_get_pointer, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::buffer_set",
                          weechat_tcl_api_buffer_set, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::current_window",
                          weechat_tcl_api_current_window, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::window_get_integer",
                          weechat_tcl_api_window_get_integer, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::window_get_string",
                          weechat_tcl_api_window_get_string, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::window_get_pointer",
                          weechat_tcl_api_window_get_pointer, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::nicklist_add_group",
                          weechat_tcl_api_nicklist_add_group, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::nicklist_search_group",
                          weechat_tcl_api_nicklist_search_group, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::nicklist_add_nick",
                          weechat_tcl_api_nicklist_add_nick, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::nicklist_search_nick",
                          weechat_tcl_api_nicklist_search_nick, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::nicklist_remove_group",
                          weechat_tcl_api_nicklist_remove_group, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::nicklist_remove_nick",
                          weechat_tcl_api_nicklist_remove_nick, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::nicklist_remove_all",
                          weechat_tcl_api_nicklist_remove_all, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::bar_item_search",
                          weechat_tcl_api_bar_item_search, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::bar_item_new",
                          weechat_tcl_api_bar_item_new, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::bar_item_update",
                          weechat_tcl_api_bar_item_update, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::bar_item_remove",
                          weechat_tcl_api_bar_item_remove, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::bar_search",
                          weechat_tcl_api_bar_search, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::bar_new",
                          weechat_tcl_api_bar_new, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::bar_set",
                          weechat_tcl_api_bar_set, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::bar_update",
                          weechat_tcl_api_bar_update, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::bar_remove",
                          weechat_tcl_api_bar_remove, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::command",
                          weechat_tcl_api_command, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::info_get",
                          weechat_tcl_api_info_get, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::infolist_new",
                          weechat_tcl_api_infolist_new, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::infolist_new_var_integer",
                          weechat_tcl_api_infolist_new_var_integer, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::infolist_new_var_string",
                          weechat_tcl_api_infolist_new_var_string, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::infolist_new_var_pointer",
                          weechat_tcl_api_infolist_new_var_pointer, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::infolist_new_var_time",
                          weechat_tcl_api_infolist_new_var_time, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::infolist_get",
                          weechat_tcl_api_infolist_get, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::infolist_next",
                          weechat_tcl_api_infolist_next, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::infolist_prev",
                          weechat_tcl_api_infolist_prev, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::infolist_fields",
                          weechat_tcl_api_infolist_fields, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::infolist_integer",
                          weechat_tcl_api_infolist_integer, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::infolist_string",
                          weechat_tcl_api_infolist_string, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::infolist_pointer",
                          weechat_tcl_api_infolist_pointer, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::infolist_time",
                          weechat_tcl_api_infolist_time, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand (interp,"weechat::infolist_free",
                          weechat_tcl_api_infolist_free, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
}
