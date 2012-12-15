/*
 * rmodifier-info.c - info and infolist hooks for rmodifier plugin
 *
 * Copyright (C) 2010-2012 Sebastien Helleu <flashcode@flashtux.org>
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
#include "rmodifier.h"
#include "rmodifier-info.h"


/*
 * Returns infolist with rmodifier info.
 */

struct t_infolist *
rmodifier_info_get_infolist_cb (void *data, const char *infolist_name,
                                void *pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_rmodifier *ptr_rmodifier;

    /* make C compiler happy */
    (void) data;
    (void) arguments;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (weechat_strcasecmp (infolist_name, RMODIFIER_PLUGIN_NAME) == 0)
    {
        if (pointer && !rmodifier_valid (pointer))
            return NULL;

        ptr_infolist = weechat_infolist_new ();
        if (ptr_infolist)
        {
            if (pointer)
            {
                /* build list with only one rmodifier */
                if (!rmodifier_add_to_infolist (ptr_infolist, pointer))
                {
                    weechat_infolist_free (ptr_infolist);
                    return NULL;
                }
                return ptr_infolist;
            }
            else
            {
                /* build list with all rmodifiers matching arguments */
                for (ptr_rmodifier = rmodifier_list; ptr_rmodifier;
                     ptr_rmodifier = ptr_rmodifier->next_rmodifier)
                {
                    if (!arguments || !arguments[0]
                        || weechat_string_match (ptr_rmodifier->name, arguments, 0))
                    {
                        if (!rmodifier_add_to_infolist (ptr_infolist, ptr_rmodifier))
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
 * Hooks infolist.
 */

void
rmodifier_info_init ()
{
    weechat_hook_infolist ("rmodifier", N_("list of rmodifiers"),
                           N_("rmodifier pointer (optional)"),
                           N_("rmodifier name (can start or end with \"*\" as "
                              "joker) (optional)"),
                           &rmodifier_info_get_infolist_cb, NULL);
}
