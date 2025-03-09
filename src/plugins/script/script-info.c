/*
 * script-info.c - info, infolist and hdata hooks for script plugin
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "script.h"
#include "script-repo.h"


/*
 * Returns script info "script_info".
 */

char *
script_info_info_script_info_cb (const void *pointer, void *data,
                                 const char *info_name,
                                 const char *arguments)
{
    int i, length;
    char *script_name, hdata_name[128], *str_to_eval, *info;
    const char *ptr_info, *ptr_name;
    struct t_hdata *hdata;
    struct t_hashtable *pointers;
    void *ptr_script;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    if (!arguments || !arguments[0])
        return NULL;

    ptr_info = strchr (arguments, ',');
    if (!ptr_info)
        return NULL;

    info = NULL;

    script_name = weechat_strndup (arguments, ptr_info - arguments);
    if (!script_name)
        goto end;

    ptr_info++;

    for (i = 0; i < SCRIPT_NUM_LANGUAGES; i++)
    {
        snprintf (hdata_name, sizeof (hdata_name),
                  "%s_script", script_language[i]);
        hdata = weechat_hdata_get (hdata_name);
        ptr_script = weechat_hdata_get_list (hdata, "scripts");
        while (ptr_script)
        {
            ptr_name = weechat_hdata_string (hdata, ptr_script, "name");
            if (ptr_name)
            {
                length = strlen (ptr_name);
                if ((strncmp (script_name, ptr_name, length) == 0)
                    && (script_name[length] == '.')
                    && (strcmp (script_name + length + 1, script_extension[i]) == 0))
                {
                    pointers = weechat_hashtable_new (
                        32,
                        WEECHAT_HASHTABLE_STRING,
                        WEECHAT_HASHTABLE_POINTER,
                        NULL,
                        NULL);
                    weechat_hashtable_set (pointers, hdata_name, ptr_script);
                    if (weechat_asprintf (&str_to_eval, "${%s.%s}", hdata_name, ptr_info) >= 0)
                    {
                        info = weechat_string_eval_expression (
                            str_to_eval, pointers, NULL, NULL);
                        free (str_to_eval);
                    }
                    weechat_hashtable_free (pointers);
                    goto end;
                }
            }
            ptr_script = weechat_hdata_move (hdata, ptr_script, 1);
        }
    }

end:
    free (script_name);
    return info;
}

/*
 * Returns script info "script_loaded".
 */

char *
script_info_info_script_loaded_cb (const void *pointer, void *data,
                                   const char *info_name,
                                   const char *arguments)
{
    int i, length;
    char hdata_name[128];
    const char *ptr_name;
    struct t_hdata *hdata;
    void *ptr_script;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    if (!arguments || !arguments[0])
        return NULL;

    for (i = 0; i < SCRIPT_NUM_LANGUAGES; i++)
    {
        snprintf (hdata_name, sizeof (hdata_name),
                  "%s_script", script_language[i]);
        hdata = weechat_hdata_get (hdata_name);
        ptr_script = weechat_hdata_get_list (hdata, "scripts");
        while (ptr_script)
        {
            ptr_name = weechat_hdata_string (hdata, ptr_script, "name");
            if (ptr_name)
            {
                length = strlen (ptr_name);
                if ((strncmp (arguments, ptr_name, length) == 0)
                    && (arguments[length] == '.')
                    && (strcmp (arguments + length + 1, script_extension[i]) == 0))
                {
                    /* script loaded */
                    return strdup ("1");
                }
            }
            ptr_script = weechat_hdata_move (hdata, ptr_script, 1);
        }
    }

    /* script not loaded */
    return NULL;
}

/*
 * Returns script infolist "script_script".
 */

struct t_infolist *
script_info_infolist_script_script_cb (const void *pointer, void *data,
                                       const char *infolist_name,
                                       void *obj_pointer,
                                       const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_script_repo *ptr_script;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;

    if (obj_pointer && !script_repo_script_valid (obj_pointer))
        return NULL;

    ptr_infolist = weechat_infolist_new ();
    if (!ptr_infolist)
        return NULL;

    if (obj_pointer)
    {
        /* build list with only one script */
        if (!script_repo_add_to_infolist (ptr_infolist, obj_pointer))
        {
            weechat_infolist_free (ptr_infolist);
            return NULL;
        }
        return ptr_infolist;
    }
    else
    {
        /* build list with all scripts matching arguments */
        for (ptr_script = scripts_repo; ptr_script;
             ptr_script = ptr_script->next_script)
        {
            if (!arguments || !arguments[0]
                || weechat_string_match (ptr_script->name_with_extension,
                                         arguments, 1))
            {
                if (!script_repo_add_to_infolist (ptr_infolist, ptr_script))
                {
                    weechat_infolist_free (ptr_infolist);
                    return NULL;
                }
            }
        }
        return ptr_infolist;
    }

    return NULL;
}

/*
 * Hooks infolist and hdata.
 */

void
script_info_init (void)
{
    /* info hooks */
    weechat_hook_info (
        "script_info",
        N_("info on a script"),
        N_("script,info (script name with extension and info is a hdata variable"),
        &script_info_info_script_info_cb, NULL, NULL);
    weechat_hook_info (
        "script_loaded",
        N_("1 if script is loaded"),
        N_("script name with extension"),
        &script_info_info_script_loaded_cb, NULL, NULL);

    /* infolist hooks */
    weechat_hook_infolist (
        "script_script",
        N_("list of scripts"),
        N_("script pointer (optional)"),
        N_("script name with extension "
           "(wildcard \"*\" is allowed) (optional)"),
        &script_info_infolist_script_script_cb, NULL, NULL);

    /* hdata hooks */
    weechat_hook_hdata (
        "script_script", N_("scripts from repository"),
        &script_repo_hdata_script_cb, NULL, NULL);
}
