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

/* alias-info.c: info and infolist hooks for alias plugin */


#include <stdlib.h>

#include "../weechat-plugin.h"
#include "alias.h"


/*
 * alias_info_get_infolist_cb: callback called when alias infolist is asked
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
    
    if (weechat_strcasecmp (infolist_name, "alias") == 0)
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
                /* build list with all aliases */
                for (ptr_alias = alias_list; ptr_alias;
                     ptr_alias = ptr_alias->next_alias)
                {
                    if (!alias_add_to_infolist (ptr_infolist, ptr_alias))
                    {
                        weechat_infolist_free (ptr_infolist);
                        return NULL;
                    }
                }
                return ptr_infolist;
            }
        }
    }
    
    return NULL;
}

/*
 * alias_info_init: initialize info and infolist hooks for alias plugin
 */

void
alias_info_init ()
{
    /* alias infolist hooks */
    weechat_hook_infolist ("alias", N_("list of alias"),
                           &alias_info_get_infolist_cb, NULL);
}
