/*
 * script-info.c - info, infolist and hdata hooks for script plugin
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
script_info_init ()
{
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
