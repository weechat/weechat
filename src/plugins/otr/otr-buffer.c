/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Display and buffer functions for OTR plugin */

#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-buffer.h"
#include "otr-callback.h"
#include "otr-command.h"
#include "otr-config.h"
#include "otr-context.h"
#include "otr-irc.h"
#include "otr-util.h"


/*
 * Display a debug message in buffer "OTR Debug" (created if needed), only if
 * option otr.general.debug is enabled.
 */

void
otr_buffer_debug (const char *format, ...)
{
    struct t_gui_buffer *ptr_buffer;

    if (!otr_config_general_debug
        || !weechat_config_boolean (otr_config_general_debug))
    {
        return;
    }

    ptr_buffer = weechat_buffer_search (OTR_PLUGIN_NAME, "OTR Debug");
    if (!ptr_buffer)
    {
        ptr_buffer = weechat_buffer_new ("OTR Debug",
                                         NULL, NULL, NULL,
                                         NULL, NULL, NULL);
        if (!ptr_buffer)
            return;
        weechat_buffer_set (ptr_buffer, "title", "OTR Debug");
        weechat_buffer_set (ptr_buffer, "localvar_set_no_log", "1");
    }

    weechat_va_format (format);
    if (!vbuffer)
        return;

    weechat_printf (ptr_buffer, "otr debug\t%s", vbuffer);

    free (vbuffer);
}

/*
 * Display a message in a buffer with the OTR prefix and a color (index in enum
 * t_otr_config_color); each line of the message is displayed separately.
 */

void
otr_buffer_print_str (struct t_gui_buffer *buffer, int color,
                      const char *message)
{
    char *prefix, *message2, *ptr_line, *ptr_newline;
    const char *ptr_color;

    message2 = otr_util_dupstr (message);
    if (!message2)
        return;

    prefix = weechat_string_eval_expression (
        weechat_config_string (otr_config_look[OTR_CONFIG_LOOK_PREFIX]),
        NULL, NULL, NULL);
    ptr_color = weechat_color (weechat_config_color (otr_config_color[color]));

    ptr_line = message2;
    while (ptr_line)
    {
        ptr_newline = strchr (ptr_line, '\n');
        if (ptr_newline)
            ptr_newline[0] = '\0';
        weechat_printf (buffer, "%s\t%s%s",
                        (prefix) ? prefix : OTR_PLUGIN_NAME,
                        (ptr_color) ? ptr_color : "",
                        ptr_line);
        ptr_line = (ptr_newline) ? ptr_newline + 1 : NULL;
    }

    free (message2);
    free (prefix);
}

/*
 * Display an error in core buffer.
 */

void
otr_buffer_error_core (const char *format, ...)
{
    weechat_va_format (format);
    if (!vbuffer)
        return;

    weechat_printf (NULL, "%s%s: %s",
                    weechat_prefix ("error"), OTR_PLUGIN_NAME, vbuffer);

    free (vbuffer);
}

/*
 * Check if a buffer is a private buffer.
 *
 * Return 1 if the buffer is private, 0 otherwise.
 */

int
otr_buffer_is_private (struct t_gui_buffer *buffer)
{
    const char *ptr_type;

    if (!buffer)
        return 0;

    ptr_type = weechat_buffer_get_string (buffer, "localvar_type");

    return (ptr_type && (strcmp (ptr_type, "private") == 0)) ? 1 : 0;
}

/*
 * Display a message for a peer, in the private buffer with the peer if it
 * exists, otherwise in core buffer with the nick of the peer before the
 * message.
 *
 * Nothing is displayed when the plugin is unloading.
 */

void
otr_buffer_peer_print (const char *server, const char *nick, int color,
                       const char *message)
{
    struct t_gui_buffer *ptr_buffer;
    char *message_with_nick;

    if (otr_shutdown)
        return;

    ptr_buffer = otr_irc_buffer (server, nick);
    message_with_nick = NULL;

    if (nick && nick[0] && !otr_buffer_is_private (ptr_buffer))
        weechat_asprintf (&message_with_nick, "[%s] %s", nick, message);

    otr_buffer_print_str (ptr_buffer, color,
                          (message_with_nick) ? message_with_nick : message);

    free (message_with_nick);
}

/*
 * Display a formatted message for the peer of a libotr context.
 */

void
otr_buffer_ctx_print (ConnContext *context, int color, const char *format, ...)
{
    weechat_va_format (format);
    if (!vbuffer)
        return;

    otr_buffer_peer_print (otr_irc_account_server (context->accountname),
                           context->username, color, vbuffer);

    free (vbuffer);
}

/*
 * Display a hint for the peer of a libotr context, if option otr.general.hints
 * is enabled.
 */

void
otr_buffer_ctx_hint (ConnContext *context, const char *message)
{
    if (weechat_config_boolean (otr_config_general_hints))
        otr_buffer_ctx_print (context, OTR_CONFIG_LEVEL_HINT, "%s", message);
}

/*
 * Display a hint for a peer, if option otr.general.hints is enabled.
 */

void
otr_buffer_peer_hint (const char *server, const char *nick,
                      const char *message)
{
    if (weechat_config_boolean (otr_config_general_hints))
        otr_buffer_peer_print (server, nick, OTR_CONFIG_LEVEL_HINT, message);
}

/*
 * Callback for signal "buffer_switch": update the OTR bar item.
 */

int
otr_buffer_switch_cb (const void *pointer, void *data, const char *signal,
                      const char *type_data, void *signal_data)
{
    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    weechat_bar_item_update (OTR_BAR_ITEM);

    return WEECHAT_RC_OK;
}

/*
 * Callback for signal "buffer_closing": end the OTR session when its private
 * buffer is closed.
 */

int
otr_buffer_closing_cb (const void *pointer, void *data, const char *signal,
                       const char *type_data, void *signal_data)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_otr_peer_ref peer_ref;
    char *account, *peer;
    ConnContext *ptr_context;

    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    ptr_buffer = (struct t_gui_buffer *)signal_data;

    if (!otr_command_peer_args (ptr_buffer, NULL, 0, &peer_ref)
        || !otr_command_resolve (&peer_ref, &account, &peer))
    {
        return WEECHAT_RC_OK;
    }

    ptr_context = otr_context_find (account, peer);
    if (ptr_context && (ptr_context->msgstate != OTRL_MSGSTATE_PLAINTEXT))
    {
        otrl_message_disconnect_all_instances (otr_us, &otr_callback_ops, NULL,
                                               account, OTR_PROTOCOL, peer);
    }

    free (account);
    free (peer);

    return WEECHAT_RC_OK;
}
