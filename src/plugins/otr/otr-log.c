/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Control of buffer logging during OTR sessions */

#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-buffer.h"
#include "otr-config.h"
#include "otr-context.h"
#include "otr-irc.h"
#include "otr-log.h"


/*
 * Check if a buffer is currently logged to disk.
 *
 * Return 1 if the buffer is logged, 0 otherwise.
 */

int
otr_log_buffer_is_logged (struct t_gui_buffer *buffer)
{
    struct t_infolist *ptr_infolist;
    int logged;

    if (!buffer)
        return 0;

    ptr_infolist = weechat_infolist_get ("logger_buffer", NULL, NULL);
    if (!ptr_infolist)
        return 0;

    logged = 0;
    while (weechat_infolist_next (ptr_infolist))
    {
        if (weechat_infolist_pointer (ptr_infolist, "buffer") == buffer)
        {
            logged = (weechat_infolist_integer (ptr_infolist,
                                                "log_enabled")) ? 1 : 0;
            break;
        }
    }
    weechat_infolist_free (ptr_infolist);

    return logged;
}

/*
 * Build the name of the logger level option for a buffer:
 * "logger.level.<plugin>.<name>".
 *
 * Note: result must be freed after use.
 */

char *
otr_log_logger_option_name (struct t_gui_buffer *buffer)
{
    const char *ptr_plugin, *ptr_name;
    char *option_name;

    ptr_plugin = weechat_buffer_get_string (buffer, "plugin");
    ptr_name = weechat_buffer_get_string (buffer, "name");

    weechat_asprintf (&option_name, "logger.level.%s.%s",
                      (ptr_plugin) ? ptr_plugin : "",
                      (ptr_name) ? ptr_name : "");

    return option_name;
}

/*
 * Get the logger level of a buffer.
 *
 * Return OTR_LOG_DEFAULT if the buffer has no specific logger level option,
 * otherwise the value of this option (even if the buffer is not logged, for
 * example if option logger.file.auto_log is off).
 */

int
otr_log_buffer_log_level (struct t_gui_buffer *buffer)
{
    struct t_config_option *ptr_option;
    char *option_name;

    option_name = otr_log_logger_option_name (buffer);
    if (!option_name)
        return OTR_LOG_DEFAULT;
    ptr_option = weechat_config_get (option_name);
    free (option_name);

    return (ptr_option) ?
        weechat_config_integer (ptr_option) : OTR_LOG_DEFAULT;
}

/*
 * Save the logger level of a buffer in per-peer data (if not already saved),
 * with the name of its logger level option, to be restored at the end of the
 * OTR session (even if the buffer is renamed meanwhile).
 */

void
otr_log_save_level (struct t_otr_peer_data *peer_data,
                    struct t_gui_buffer *buffer)
{
    if (!peer_data || !buffer || (peer_data->prev_log != OTR_LOG_UNTOUCHED))
        return;

    free (peer_data->log_option);
    peer_data->log_option = otr_log_logger_option_name (buffer);
    peer_data->prev_log = otr_log_buffer_log_level (buffer);
}

/*
 * Disable logging of the private buffer with the peer of a libotr context
 * during the OTR session; the previous logger level is saved in per-peer data,
 * to be restored at the end of the session.
 *
 * If a level is already saved (for example the peer ended the previous session
 * and started a new one, without the end of session on our side), this level
 * is kept (it is the level before the first session), but logging is disabled
 * anyway if the buffer is logged.
 */

void
otr_log_disable (ConnContext *context, struct t_otr_peer_data *peer_data)
{
    struct t_gui_buffer *ptr_buffer;

    if (!peer_data)
        return;

    ptr_buffer = otr_irc_buffer (otr_irc_account_server (context->accountname),
                                 context->username);
    if (!otr_buffer_is_private (ptr_buffer))
        return;

    otr_log_save_level (peer_data, ptr_buffer);
    if (otr_log_buffer_is_logged (ptr_buffer))
    {
        weechat_command (ptr_buffer, "/mute logger disable");
        otr_buffer_ctx_print (context, OTR_CONFIG_LEVEL_INFO,
                              _("Logs have been temporarily disabled for the "
                                "session. They will be restored upon finishing "
                                "the OTR session."));
    }
}

/*
 * Callback of timer used to restore the logger level after a buffer is closed:
 * run the command given in data.
 */

int
otr_log_restore_timer_cb (const void *pointer, void *data, int remaining_calls)
{
    /* Make C compiler happy. */
    (void) pointer;
    (void) remaining_calls;

    if (data)
        weechat_command (NULL, (const char *)data);

    return WEECHAT_RC_OK;
}

/*
 * Build the command to restore the logger level saved in per-peer data.
 *
 * Return NULL if there is nothing to restore.
 *
 * Note: result must be freed after use.
 */

char *
otr_log_restore_command (struct t_otr_peer_data *peer_data)
{
    char *command;

    if (!peer_data || !peer_data->log_option)
        return NULL;

    command = NULL;

    if ((peer_data->prev_log >= 0) && (peer_data->prev_log < 10))
    {
        weechat_asprintf (&command, "/mute set %s %d",
                          peer_data->log_option, peer_data->prev_log);
    }
    else if (peer_data->prev_log == OTR_LOG_DEFAULT)
    {
        weechat_asprintf (&command, "/mute unset %s", peer_data->log_option);
    }

    return command;
}

/*
 * Forget the logger level saved in per-peer data.
 */

void
otr_log_forget_level (struct t_otr_peer_data *peer_data)
{
    peer_data->prev_log = OTR_LOG_UNTOUCHED;
    free (peer_data->log_option);
    peer_data->log_option = NULL;
}

/*
 * Restore the logger level of the private buffer with the peer of a libotr
 * context, saved when the OTR session started (the option saved is used, even
 * if the buffer has been renamed or closed since).
 *
 * If buffer_closing is 1, the private buffer is being closed: nothing is
 * displayed and the logger level is restored after the buffer is closed (the
 * logger must not start logging again on a buffer being closed).
 */

void
otr_log_restore (ConnContext *context, struct t_otr_peer_data *peer_data,
                 int buffer_closing)
{
    char *command;

    if (!peer_data || (peer_data->prev_log == OTR_LOG_UNTOUCHED))
        return;

    command = otr_log_restore_command (peer_data);

    if (command && !buffer_closing)
    {
        if (peer_data->prev_log >= 0)
        {
            otr_buffer_ctx_print (context, OTR_CONFIG_LEVEL_WARNING,
                                  _("Restoring buffer logging value to: %d"),
                                  peer_data->prev_log);
        }
        else
        {
            otr_buffer_ctx_print (
                context, OTR_CONFIG_LEVEL_WARNING,
                _("Restoring buffer logging value to default"));
        }
    }

    if (command)
    {
        if (buffer_closing)
        {
            /* Command is freed by WeeChat when the timer is removed. */
            weechat_hook_timer (1, 0, 1,
                                &otr_log_restore_timer_cb, NULL, command);
        }
        else
        {
            weechat_command (NULL, command);
            free (command);
        }
    }

    otr_log_forget_level (peer_data);
}

/*
 * Handle the rename of the private buffer with the peer of a libotr context
 * during the OTR session (the IRC plugin renames it when the case of the
 * peer's nick changes): the logger level option depends on the buffer name, so
 * the level of the old option is restored, and if logging was disabled, it is
 * disabled again with the new buffer name (nothing is displayed).
 */

void
otr_log_buffer_renamed (struct t_otr_peer_data *peer_data,
                        struct t_gui_buffer *buffer)
{
    struct t_config_option *ptr_option;
    char *option_name, *command;
    int disabled;

    if (!peer_data || !peer_data->log_option || !buffer)
        return;

    option_name = otr_log_logger_option_name (buffer);
    if (!option_name)
        return;
    if (strcmp (option_name, peer_data->log_option) == 0)
    {
        free (option_name);
        return;
    }
    free (option_name);

    ptr_option = weechat_config_get (peer_data->log_option);
    disabled = (ptr_option && (weechat_config_integer (ptr_option) == 0)) ?
        1 : 0;

    command = otr_log_restore_command (peer_data);
    if (command)
    {
        weechat_command (NULL, command);
        free (command);
    }
    otr_log_forget_level (peer_data);

    if (disabled)
    {
        otr_log_save_level (peer_data, buffer);
        if (otr_log_buffer_is_logged (buffer))
            weechat_command (buffer, "/mute logger disable");
    }
}

/*
 * Callback for changes on logger level options of IRC buffers: update the OTR
 * bar item.
 */

int
otr_log_logger_level_update_cb (const void *pointer, void *data,
                                const char *option, const char *value)
{
    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) option;
    (void) value;

    weechat_bar_item_update (OTR_BAR_ITEM);

    return WEECHAT_RC_OK;
}
