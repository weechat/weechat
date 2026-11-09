/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_COMMAND_H
#define WEECHAT_PLUGIN_OTR_COMMAND_H

struct t_otr_peer_ref
{
    char nick[128];                    /* nick of the peer                  */
    char server[128];                  /* internal name of IRC server       */
};

struct t_otr_table
{
    char *(*rows)[4];                  /* rows with 4 columns               */
    int num_rows;                      /* number of rows                    */
    int size_alloc;                    /* number of rows allocated          */
    size_t column_width[4];            /* max width of each column          */
};

extern int otr_command_peer_args (struct t_gui_buffer *buffer, char **args,
                                  int num_args,
                                  struct t_otr_peer_ref *peer_ref);
extern int otr_command_resolve (const struct t_otr_peer_ref *peer_ref,
                                char **account, char **peer);
extern void otr_command_init (void);

#endif /* WEECHAT_PLUGIN_OTR_COMMAND_H */
