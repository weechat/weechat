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


void
otr_bar_item_part (struct t_otr_sbuf *b, const char *sep, int *first,
                   int color_idx, int look_idx)
{
    const char *str = weechat_config_string (otr_config_look[look_idx]);
    char col[64], def[64];

    if (!str || !*str)
        return;
    snprintf (
        col, sizeof (col), "%s",
        weechat_color (weechat_config_color (otr_config_color[color_idx])));
    snprintf (def, sizeof (def), "%s",
              weechat_color (weechat_config_color (
                  otr_config_color[OTR_CONFIG_COLOR_STATUS_DEFAULT])));
    if (!*first)
        otr_util_sb_adds (b, sep);
    *first = 0;
    otr_util_sb_adds (b, col);
    otr_util_sb_adds (b, str);
    otr_util_sb_adds (b, def);
}

char *
otr_bar_item_cb (const void *pointer, void *data, struct t_gui_bar_item *item,
                 struct t_gui_window *window, struct t_gui_buffer *buffer,
                 struct t_hashtable *extra_info)
{
    struct t_gui_buffer *buf = buffer ? buffer : weechat_current_buffer ();
    const char *own, *server, *peer;
    char *account, *peerlc, *res;
    ConnContext *ctx;
    struct t_otr_sbuf parts, out;
    int first = 1, enc, ver, logged;
    char col[64];

    (void)pointer; (void)data; (void)item; (void)window; (void)extra_info;

    if (!otr_us || !otr_buffer_is_private (buf))
        return strdup ("");

    own = weechat_buffer_get_string (buf, "localvar_nick");
    server = weechat_buffer_get_string (buf, "localvar_server");
    peer = weechat_buffer_get_string (buf, "localvar_channel");
    if (!own || !server || !peer)
        return strdup ("");
    {
        char *l = otr_util_lc_dup (own);

        account = l ? otr_util_strf ("%s@%s", l, server) : NULL;
        free (l);
    }
    peerlc = otr_util_lc_dup (peer);
    if (!account || !peerlc)
    {
        free (account);
        free (peerlc);
        return strdup ("");
    }
    ctx = otr_context_find (account, peerlc);
    enc = otr_context_encrypted (ctx);
    ver = otr_context_verified (ctx);
    logged = otr_log_buffer_is_logged (buf);

    otr_util_sb_init (&parts);
    if (enc)
    {
        otr_bar_item_part (
            &parts,
            weechat_config_string (
                otr_config_look[OTR_CONFIG_LOOK_BAR_SEPARATOR]),
            &first,
            OTR_CONFIG_COLOR_STATUS_ENCRYPTED,
            OTR_CONFIG_LOOK_BAR_ENCRYPTED);
        if (ver)
            otr_bar_item_part (
                &parts,
                weechat_config_string (
                    otr_config_look[OTR_CONFIG_LOOK_BAR_SEPARATOR]),
                &first,
                OTR_CONFIG_COLOR_STATUS_AUTHENTICATED,
                OTR_CONFIG_LOOK_BAR_AUTHENTICATED);
        else
            otr_bar_item_part (
                &parts,
                weechat_config_string (
                    otr_config_look[OTR_CONFIG_LOOK_BAR_SEPARATOR]),
                &first,
                OTR_CONFIG_COLOR_STATUS_UNAUTHENTICATED,
                OTR_CONFIG_LOOK_BAR_UNAUTHENTICATED);
        if (logged)
            otr_bar_item_part (
                &parts,
                weechat_config_string (
                    otr_config_look[OTR_CONFIG_LOOK_BAR_SEPARATOR]),
                &first,
                OTR_CONFIG_COLOR_STATUS_LOGGED,
                OTR_CONFIG_LOOK_BAR_LOGGED);
        else
            otr_bar_item_part (
                &parts,
                weechat_config_string (
                    otr_config_look[OTR_CONFIG_LOOK_BAR_SEPARATOR]),
                &first,
                OTR_CONFIG_COLOR_STATUS_NOTLOGGED,
                OTR_CONFIG_LOOK_BAR_NOTLOGGED);
    }
    else
        otr_bar_item_part (
            &parts,
            weechat_config_string (
                otr_config_look[OTR_CONFIG_LOOK_BAR_SEPARATOR]),
            &first,
            OTR_CONFIG_COLOR_STATUS_UNENCRYPTED,
            OTR_CONFIG_LOOK_BAR_UNENCRYPTED);

    otr_util_sb_init (&out);
    if (parts.len)
    {
        snprintf (col, sizeof (col), "%s",
                  weechat_color (weechat_config_color (
                      otr_config_color[OTR_CONFIG_COLOR_STATUS_DEFAULT])));
        otr_util_sb_adds (&out, col);
        otr_util_sb_adds (
            &out,
            weechat_config_string (
                otr_config_look[OTR_CONFIG_LOOK_BAR_PREFIX]));
        otr_util_sb_adds (&out, parts.s);
    }
    free (parts.s);
    res = otr_util_sb_take (&out);

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
