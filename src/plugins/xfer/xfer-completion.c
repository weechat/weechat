/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Nick completion for xfer chats */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "xfer.h"
#include "xfer-completion.h"


/*
 * Add nicks to completion list.
 */

int
xfer_completion_nick_cb (const void *pointer, void *data,
                         const char *completion_item,
                         struct t_gui_buffer *buffer,
                         struct t_gui_completion *completion)
{
    struct t_xfer *ptr_xfer;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;

    ptr_xfer = xfer_search_by_buffer (buffer);
    if (ptr_xfer)
    {
        /* remote nick */
        weechat_completion_list_add (completion,
                                     ptr_xfer->remote_nick,
                                     0,
                                     WEECHAT_LIST_POS_SORT);
        /* add self nick at the end */
        weechat_completion_list_add (completion,
                                     ptr_xfer->local_nick,
                                     1,
                                     WEECHAT_LIST_POS_END);
    }

    return WEECHAT_RC_OK;
}

/*
 * Hook completions.
 */

void
xfer_completion_init (void)
{
    weechat_hook_completion ("nick",
                             N_("nicks of DCC chat"),
                             &xfer_completion_nick_cb, NULL, NULL);
}
