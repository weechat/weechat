/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_COMMAND_H
#define WEECHAT_PLUGIN_OTR_COMMAND_H

/*
 * Return code of functions of subcommands of /otr: error already displayed
 * (converted to WEECHAT_RC_ERROR by the command callback).
 */
#define OTR_COMMAND_RC_ERROR_DISPLAYED -2

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
extern void otr_command_init (void);

#endif /* WEECHAT_PLUGIN_OTR_COMMAND_H */
