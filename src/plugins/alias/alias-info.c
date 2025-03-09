/*
 * alias-info.c - info and infolist hooks for alias plugin
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

#include "../weechat-plugin.h"
#include "alias.h"
#include "alias-config.h"


/*
 * Returns infolist "alias".
 */

struct t_infolist *
alias_info_infolist_alias_cb (const void *pointer, void *data,
                              const char *infolist_name,
                              void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_alias *ptr_alias;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;
    (void) arguments;

    if (obj_pointer && !alias_valid (obj_pointer))
        return NULL;

    ptr_infolist = weechat_infolist_new ();
    if (!ptr_infolist)
        return NULL;

    if (obj_pointer)
    {
        /* build list with only one alias */
        if (!alias_add_to_infolist (ptr_infolist, obj_pointer))
        {
            weechat_infolist_free (ptr_infolist);
            return NULL;
        }
        return ptr_infolist;
    }
    else
    {
        /* build list with all aliases matching arguments */
        for (ptr_alias = alias_list; ptr_alias;
             ptr_alias = ptr_alias->next_alias)
        {
            if (!arguments || !arguments[0]
                || weechat_string_match (ptr_alias->name, arguments, 1))
            {
                if (!alias_add_to_infolist (ptr_infolist, ptr_alias))
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
 * Returns infolist "alias_default".
 */

struct t_infolist *
alias_info_infolist_alias_default_cb (const void *pointer, void *data,
                                      const char *infolist_name,
                                      void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_infolist_item *ptr_item;
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;
    (void) obj_pointer;
    (void) arguments;

    ptr_infolist = weechat_infolist_new ();
    if (!ptr_infolist)
        return NULL;

    for (i = 0; alias_default[i][0]; i++)
    {
        ptr_item = weechat_infolist_new_item (ptr_infolist);
        if (!ptr_item)
        {
            weechat_infolist_free (ptr_infolist);
            return NULL;
        }
        if (!weechat_infolist_new_var_string (ptr_item, "name", alias_default[i][0]))
        {
            weechat_infolist_free (ptr_infolist);
            return NULL;
        }
        if (!weechat_infolist_new_var_string (ptr_item, "command", alias_default[i][1]))
        {
            weechat_infolist_free (ptr_infolist);
            return NULL;
        }
        if (!weechat_infolist_new_var_string (ptr_item, "completion", alias_default[i][2]))
        {
            weechat_infolist_free (ptr_infolist);
            return NULL;
        }
    }

    return ptr_infolist;
}

/*
 * Hooks infolist for alias plugin.
 */

void
alias_info_init (void)
{
    weechat_hook_infolist (
        "alias", N_("list of aliases"),
        N_("alias pointer (optional)"),
        N_("alias name (wildcard \"*\" is allowed) (optional)"),
        &alias_info_infolist_alias_cb, NULL, NULL);
    weechat_hook_infolist (
        "alias_default", N_("list of default aliases"),
        NULL, NULL,
        &alias_info_infolist_alias_default_cb, NULL, NULL);
}
