/*
 * rmodifier-completion.c - completion for rmodifier command
 *
 * Copyright (C) 2010-2013 Sebastien Helleu <flashcode@flashtux.org>
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
#include "rmodifier-completion.h"


/*
 * Adds list of rmodifiers to completion list.
 */

int
rmodifier_completion_cb (void *data, const char *completion_item,
                         struct t_gui_buffer *buffer,
                         struct t_gui_completion *completion)
{
    struct t_rmodifier *ptr_rmodifier;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_rmodifier = rmodifier_list; ptr_rmodifier;
         ptr_rmodifier = ptr_rmodifier->next_rmodifier)
    {
        weechat_hook_completion_list_add (completion, ptr_rmodifier->name,
                                          0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Hooks completion.
 */

void
rmodifier_completion_init ()
{
    weechat_hook_completion ("rmodifier", N_("list of rmodifiers"),
                             &rmodifier_completion_cb, NULL);
}
