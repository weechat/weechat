/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Bar item for OTR plugin */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-bar-item.h"
#include "otr-buffer.h"
#include "otr-config.h"
#include "otr-context.h"
#include "otr-log.h"
#include "otr-util.h"


static void
bar_part (struct sbuf *b, const char *sep, int *first, int color_idx,
          int look_idx)
{
    const char *str = weechat_config_string (opt_look[look_idx]);
    char col[64], def[64];

    if (!str || !*str)
        return;
    snprintf (col, sizeof (col), "%s",
              weechat_color (weechat_config_color (opt_color[color_idx])));
    snprintf (def, sizeof (def), "%s",
              weechat_color (weechat_config_color (
                  opt_color[COL_STATUS_DEFAULT])));
    if (!*first)
        sb_adds (b, sep);
    *first = 0;
    sb_adds (b, col);
    sb_adds (b, str);
    sb_adds (b, def);
}

char *
bar_item_cb (const void *pointer, void *data, struct t_gui_bar_item *item,
             struct t_gui_window *window, struct t_gui_buffer *buffer,
             struct t_hashtable *extra_info)
{
    struct t_gui_buffer *buf = buffer ? buffer : weechat_current_buffer ();
    const char *own, *server, *peer;
    char *account, *peerlc, *res;
    ConnContext *ctx;
    struct sbuf parts, out;
    int first = 1, enc, ver, logged;
    char col[64];

    (void)pointer; (void)data; (void)item; (void)window; (void)extra_info;

    if (!otr_us || !buf_is_private (buf))
        return strdup ("");

    own = weechat_buffer_get_string (buf, "localvar_nick");
    server = weechat_buffer_get_string (buf, "localvar_server");
    peer = weechat_buffer_get_string (buf, "localvar_channel");
    if (!own || !server || !peer)
        return strdup ("");
    {
        char *l = lc_dup (own);

        account = l ? strf ("%s@%s", l, server) : NULL;
        free (l);
    }
    peerlc = lc_dup (peer);
    if (!account || !peerlc)
    {
        free (account);
        free (peerlc);
        return strdup ("");
    }
    ctx = find_ctx (account, peerlc);
    enc = ctx_encrypted (ctx);
    ver = ctx_verified (ctx);
    logged = buf_is_logged (buf);

    sb_init (&parts);
    if (enc)
    {
        bar_part (&parts, weechat_config_string (opt_look[LOOK_BAR_SEPARATOR]),
                  &first, COL_STATUS_ENCRYPTED, LOOK_BAR_ENCRYPTED);
        if (ver)
            bar_part (&parts,
                      weechat_config_string (opt_look[LOOK_BAR_SEPARATOR]),
                      &first, COL_STATUS_AUTHENTICATED, LOOK_BAR_AUTHENTICATED);
        else
            bar_part (&parts,
                      weechat_config_string (opt_look[LOOK_BAR_SEPARATOR]),
                      &first, COL_STATUS_UNAUTHENTICATED,
                      LOOK_BAR_UNAUTHENTICATED);
        if (logged)
            bar_part (&parts,
                      weechat_config_string (opt_look[LOOK_BAR_SEPARATOR]),
                      &first, COL_STATUS_LOGGED, LOOK_BAR_LOGGED);
        else
            bar_part (&parts,
                      weechat_config_string (opt_look[LOOK_BAR_SEPARATOR]),
                      &first, COL_STATUS_NOTLOGGED, LOOK_BAR_NOTLOGGED);
    }
    else
        bar_part (&parts, weechat_config_string (opt_look[LOOK_BAR_SEPARATOR]),
                  &first, COL_STATUS_UNENCRYPTED, LOOK_BAR_UNENCRYPTED);

    sb_init (&out);
    if (parts.len)
    {
        snprintf (col, sizeof (col), "%s",
                  weechat_color (weechat_config_color (
                      opt_color[COL_STATUS_DEFAULT])));
        sb_adds (&out, col);
        sb_adds (&out, weechat_config_string (opt_look[LOOK_BAR_PREFIX]));
        sb_adds (&out, parts.s);
    }
    free (parts.s);
    res = sb_take (&out);

    weechat_buffer_set (buf, "localvar_set_otr_encrypted",
                        enc ? "true" : "false");
    weechat_buffer_set (buf, "localvar_set_otr_authenticated",
                        ver ? "true" : "false");
    weechat_buffer_set (buf, "localvar_set_otr_logged",
                        logged ? "true" : "false");

    free (account);
    free (peerlc);
    return res ? res : strdup ("");
}
