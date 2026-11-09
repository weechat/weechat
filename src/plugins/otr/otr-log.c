/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Control of buffer logging during OTR sessions */

#include <stdlib.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-buffer.h"
#include "otr-config.h"
#include "otr-context.h"
#include "otr-irc.h"
#include "otr-log.h"
#include "otr-util.h"


int
otr_log_buffer_is_logged (struct t_gui_buffer *buf)
{
    struct t_infolist *il;
    int res = 0;

    if (!buf)
        return 0;
    il = weechat_infolist_get ("logger_buffer", NULL, NULL);
    if (!il)
        return 0;
    while (weechat_infolist_next (il))
    {
        if (weechat_infolist_pointer (il, "buffer") == buf)
        {
            res = weechat_infolist_integer (il, "log_enabled") ? 1 : 0;
            break;
        }
    }
    weechat_infolist_free (il);
    return res;
}

static char *
otr_log_logger_option_name (struct t_gui_buffer *buf)
{
    const char *plugin = weechat_buffer_get_string (buf, "plugin");
    const char *name = weechat_buffer_get_string (buf, "name");

    return otr_util_strf ("logger.level.%s.%s", plugin ? plugin : "",
                          name ? name : "");
}

int
otr_log_buffer_log_level (struct t_gui_buffer *buf)
{
    char *opt = otr_log_logger_option_name (buf);
    struct t_infolist *il;
    int res = 0;

    if (!opt)
        return OTR_LOG_DEFAULT;
    if (!weechat_config_get (opt))
    {
        free (opt);
        return OTR_LOG_DEFAULT;
    }
    free (opt);
    il = weechat_infolist_get ("logger_buffer", NULL, NULL);
    if (il)
    {
        while (weechat_infolist_next (il))
        {
            if (weechat_infolist_pointer (il, "buffer") == buf)
            {
                res = weechat_infolist_integer (il, "log_level");
                break;
            }
        }
        weechat_infolist_free (il);
    }
    return res;
}

void
otr_log_disable (ConnContext *c, struct t_otr_peer_data *pd)
{
    struct t_gui_buffer *buf;

    if (!pd || pd->prev_log != OTR_LOG_UNTOUCHED)
        return;
    buf = otr_irc_buffer (otr_irc_account_server (c->accountname), c->username);
    if (!otr_buffer_is_private (buf))
        return;
    pd->prev_log = otr_log_buffer_log_level (buf);
    if (otr_log_buffer_is_logged (buf))
    {
        weechat_command (buf, "/mute logger disable");
        otr_buffer_ctx_print (c, OTR_CONFIG_LEVEL_INFO,
                              _("Logs have been temporarily disabled for the "
                              "session. They will be restored upon finishing "
                              "the OTR session."));
    }
}

void
otr_log_restore (ConnContext *c, struct t_otr_peer_data *pd)
{
    struct t_gui_buffer *buf;
    char *cmd, *opt;

    if (!pd || pd->prev_log == OTR_LOG_UNTOUCHED)
        return;
    buf = otr_irc_buffer (otr_irc_account_server (c->accountname), c->username);
    if (otr_buffer_is_private (buf))
    {
        if (pd->prev_log >= 0 && pd->prev_log < 10)
        {
            otr_buffer_ctx_print (
                c, OTR_CONFIG_LEVEL_WARNING,
                _("Restoring buffer logging value to: %d"), pd->prev_log);
            cmd = otr_util_strf ("/mute logger set %d", pd->prev_log);
            if (cmd)
                weechat_command (buf, cmd);
            free (cmd);
        }
        else if (pd->prev_log == OTR_LOG_DEFAULT)
        {
            otr_buffer_ctx_print (
                c, OTR_CONFIG_LEVEL_WARNING,
                _("Restoring buffer logging value to default"));
            opt = otr_log_logger_option_name (buf);
            cmd = opt ? otr_util_strf ("/mute unset %s", opt) : NULL;
            if (cmd)
                weechat_command (buf, cmd);
            free (cmd);
            free (opt);
        }
    }
    pd->prev_log = OTR_LOG_UNTOUCHED;
}

int
otr_log_logger_level_update_cb (const void *pointer, void *data,
                                const char *option,
                                const char *value)
{
    (void)pointer; (void)data; (void)option; (void)value;
    weechat_bar_item_update (OTR_BAR_ITEM);
    return WEECHAT_RC_OK;
}
