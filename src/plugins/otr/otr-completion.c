/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Completions for OTR commands */


#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-completion.h"
#include "otr-config.h"


static int
policy_completion_cb (const void *pointer, void *data,
                      const char *completion_item,
                      struct t_gui_buffer *buffer,
                      struct t_gui_completion *completion)
{
    int i;

    (void)pointer; (void)data; (void)completion_item; (void)buffer;
    for (i = 0; i < N_POLICIES; i++)
        weechat_hook_completion_list_add (completion, policies[i].name, 0,
                                          WEECHAT_LIST_POS_SORT);
    return WEECHAT_RC_OK;
}

/*
 * Hook OTR completions.
 */

void
otr_completion_init (void)
{
    weechat_hook_completion ("otr_policy", N_("OTR policies"),
                             &policy_completion_cb, NULL, NULL);
}
