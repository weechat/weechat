/*
 * alias-info.c - info and infolist hooks for alias plugin
 *
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>

#include "../weechat-plugin.h"
#include "alias.h"


/*
 * Returns infolist with alias info.
 */

struct t_infolist *
alias_info_get_infolist_cb (void *data, const char *infolist_name,
                            void *pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_alias *ptr_alias;

    /* make C compiler happy */
    (void) data;
    (void) arguments;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (weechat_strcasecmp (infolist_name, ALIAS_PLUGIN_NAME) == 0)
    {
        if (pointer && !alias_valid (pointer))
            return NULL;

        ptr_infolist = weechat_infolist_new ();
        if (ptr_infolist)
        {
            if (pointer)
            {
                /* build list with only one alias */
                if (!alias_add_to_infolist (ptr_infolist, pointer))
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
                        || weechat_string_match (ptr_alias->name, arguments, 0))
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
        }
    }

    return NULL;
}

/*
 * Hooks infolist for alias plugin.
 */

void
alias_info_init ()
{
    weechat_hook_infolist ("alias", N_("list of aliases"),
                           N_("alias pointer (optional)"),
                           N_("alias name (can start or end with \"*\" as wildcard) (optional)"),
                           &alias_info_get_infolist_cb, NULL);
}
