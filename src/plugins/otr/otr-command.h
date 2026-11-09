/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_COMMAND_H
#define WEECHAT_PLUGIN_OTR_COMMAND_H

struct peer_ref
{
    char nick[128];
    char server[128];
};

extern int peer_args (struct t_gui_buffer *buf, char **args, int nargs,
                      struct peer_ref *p);
extern int resolve (const struct peer_ref *p, char **account, char **peer);
extern void otr_command_init (void);

#endif /* WEECHAT_PLUGIN_OTR_COMMAND_H */
