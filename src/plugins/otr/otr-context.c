/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Per-peer data attached to libotr contexts */

#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-buffer.h"
#include "otr-callback.h"
#include "otr-config.h"
#include "otr-context.h"
#include "otr-irc.h"
#include "otr-log.h"


/*
 * Free per-peer data (called by libotr when the context is freed).
 */

void
otr_context_peer_data_free (void *data)
{
    struct t_otr_peer_data *peer_data;

    if (!data)
        return;

    peer_data = (struct t_otr_peer_data *)data;

    free (peer_data->assembly);
    weechat_string_dyn_free (peer_data->held_messages, 1);
    free (peer_data);
}

/*
 * Get per-peer data attached to the libotr master context of a peer; the
 * context and the data are created if needed.
 *
 * Return pointer to per-peer data, NULL if error.
 */

struct t_otr_peer_data *
otr_context_peer_data_for (const char *account, const char *peer)
{
    ConnContext *ptr_context;
    struct t_otr_peer_data *new_peer_data;

    ptr_context = otrl_context_find (otr_us, peer, account, OTR_PROTOCOL,
                                     OTRL_INSTAG_MASTER, 1, NULL, NULL, NULL);
    if (!ptr_context)
        return NULL;

    if (!ptr_context->app_data)
    {
        new_peer_data = calloc (1, sizeof (*new_peer_data));
        if (!new_peer_data)
            return NULL;
        new_peer_data->prev_log = OTR_LOG_UNTOUCHED;
        ptr_context->app_data = new_peer_data;
        ptr_context->app_data_free = &otr_context_peer_data_free;
    }

    return (struct t_otr_peer_data *)ptr_context->app_data;
}

/*
 * Get per-peer data for the account and the peer of a libotr context.
 *
 * Return pointer to per-peer data, NULL if error.
 */

struct t_otr_peer_data *
otr_context_peer_data_ctx (ConnContext *context)
{
    return otr_context_peer_data_for (context->accountname, context->username);
}

/*
 * Search the best libotr context for an account and a peer (the context is not
 * created).
 *
 * Return pointer to context, NULL if not found.
 */

ConnContext *
otr_context_find (const char *account, const char *peer)
{
    return otrl_context_find (otr_us, peer, account, OTR_PROTOCOL,
                              OTRL_INSTAG_BEST, 0, NULL, NULL, NULL);
}

/*
 * Check if a libotr context is encrypted.
 *
 * Return 1 if the context is encrypted, 0 otherwise.
 */

int
otr_context_encrypted (ConnContext *context)
{
    return (context && (context->msgstate == OTRL_MSGSTATE_ENCRYPTED)) ? 1 : 0;
}

/*
 * Check if a libotr context is encrypted and its active fingerprint is trusted.
 *
 * Return 1 if the context is encrypted and verified, 0 otherwise.
 */

int
otr_context_verified (ConnContext *context)
{
    return (otr_context_encrypted (context) && context->active_fingerprint
            && otrl_context_is_fingerprint_trusted (
                context->active_fingerprint)) ? 1 : 0;
}

/*
 * Check if the peer of a libotr context has requested SMP verification and is
 * waiting for our response, according to the SMP state in libotr (the state is
 * reset by libotr when the session ends, and each instance of the peer has its
 * own state).
 *
 * Return 1 if a response to the peer is expected, 0 otherwise.
 */

int
otr_context_smp_asked (ConnContext *context)
{
    OtrlSMState *ptr_smstate;

    if (!context || !context->smstate)
        return 0;

    ptr_smstate = context->smstate;

    return ((ptr_smstate->nextExpected == OTRL_SMP_EXPECT1)
            && (ptr_smstate->sm_prog_state == OTRL_SMP_PROG_OK)
            && ptr_smstate->g1 && ptr_smstate->x2
            && ptr_smstate->x3) ? 1 : 0;
}

/*
 * Reset the SMP state of a peer (SMP in progress, question and request of the
 * peer), for example when the OTR session ends.
 */

void
otr_context_smp_reset (struct t_otr_peer_data *peer_data)
{
    if (!peer_data)
        return;

    peer_data->in_smp = 0;
    peer_data->smp_question = 0;
    peer_data->smp_asked = 0;
}

/*
 * Hold a message (IRC text) sent to a peer, until the OTR session is encrypted.
 *
 * Return 1 if OK, 0 if error.
 */

int
otr_context_hold_message (struct t_otr_peer_data *peer_data,
                          const char *message)
{
    if (!peer_data || !message)
        return 0;

    if (!peer_data->held_messages)
    {
        peer_data->held_messages = weechat_string_dyn_alloc (256);
        if (!peer_data->held_messages)
            return 0;
    }
    else if (!weechat_string_dyn_concat (peer_data->held_messages, "\n", -1))
    {
        return 0;
    }

    return weechat_string_dyn_concat (peer_data->held_messages, message, -1);
}

/*
 * Remove messages held for a peer.
 */

void
otr_context_clear_held_messages (struct t_otr_peer_data *peer_data)
{
    if (!peer_data)
        return;

    weechat_string_dyn_free (peer_data->held_messages, 1);
    peer_data->held_messages = NULL;
}

/*
 * Callback of timer used to send the messages held for a peer: data is
 * "account peer".
 */

int
otr_context_send_held_messages_timer_cb (const void *pointer, void *data,
                                         int remaining_calls)
{
    struct t_otr_peer_data *ptr_peer_data;
    ConnContext *ptr_context;
    char *account, **messages, *command;
    const char *ptr_peer;
    int i, num_messages;

    /* Make C compiler happy. */
    (void) pointer;
    (void) remaining_calls;

    if (!otr_us || !data)
        return WEECHAT_RC_OK;

    ptr_peer = strchr ((const char *)data, ' ');
    if (!ptr_peer)
        return WEECHAT_RC_OK;
    account = weechat_strndup ((const char *)data,
                               ptr_peer - (const char *)data);
    if (!account)
        return WEECHAT_RC_OK;
    ptr_peer++;

    ptr_context = otrl_context_find (otr_us, ptr_peer, account, OTR_PROTOCOL,
                                     OTRL_INSTAG_MASTER, 0, NULL, NULL, NULL);
    ptr_peer_data = (ptr_context) ?
        (struct t_otr_peer_data *)ptr_context->app_data : NULL;
    if (!ptr_peer_data || !ptr_peer_data->held_messages
        || !otr_context_encrypted (otr_context_find (account, ptr_peer)))
    {
        free (account);
        return WEECHAT_RC_OK;
    }

    messages = weechat_string_split (*(ptr_peer_data->held_messages), "\n",
                                     NULL, 0, 0, &num_messages);
    otr_context_clear_held_messages (ptr_peer_data);

    /* Messages are sent again, so they are encrypted by the modifier. */
    for (i = 0; messages && (i < num_messages); i++)
    {
        weechat_asprintf (&command, "/quote -server %s PRIVMSG %s :%s",
                          otr_irc_account_server (account), ptr_peer,
                          messages[i]);
        if (command)
        {
            weechat_command (NULL, command);
            free (command);
        }
    }

    weechat_string_free_split (messages);
    free (account);

    return WEECHAT_RC_OK;
}

/*
 * Send the messages held for the peer of a libotr context, now that the session
 * is encrypted.
 *
 * Messages are sent with a timer, outside the libotr callbacks.
 */

void
otr_context_send_held_messages (ConnContext *context)
{
    struct t_otr_peer_data *ptr_peer_data;
    char *data;

    ptr_peer_data = otr_context_peer_data_ctx (context);
    if (!ptr_peer_data || !ptr_peer_data->held_messages)
        return;

    weechat_asprintf (&data, "%s %s", context->accountname, context->username);
    if (!data)
        return;

    /* Data is freed by WeeChat when the timer is removed. */
    weechat_hook_timer (1, 0, 1,
                        &otr_context_send_held_messages_timer_cb, NULL, data);
}

/*
 * End the OTR session with a peer: messages held for the session are removed,
 * libotr notifies the peer if the session is encrypted, then the logger level
 * of the private buffer is restored.
 *
 * If display is 1, a message is displayed for the peer.
 *
 * If buffer_closing is 1, the private buffer with the peer is being closed (see
 * function otr_log_restore).
 *
 * Return 1 if a session was ended, 0 if there was no session with the peer.
 */

int
otr_context_end_session (const char *account, const char *peer, int display,
                         int buffer_closing)
{
    ConnContext *ptr_context;

    /* Messages held for the session are not sent anymore. */
    ptr_context = otrl_context_find (otr_us, peer, account, OTR_PROTOCOL,
                                     OTRL_INSTAG_MASTER, 0, NULL, NULL, NULL);
    if (ptr_context)
    {
        otr_context_clear_held_messages (
            (struct t_otr_peer_data *)ptr_context->app_data);
    }

    ptr_context = otr_context_find (account, peer);
    if (!ptr_context || (ptr_context->msgstate == OTRL_MSGSTATE_PLAINTEXT))
        return 0;

    otrl_message_disconnect_all_instances (otr_us, &otr_callback_ops, NULL,
                                           account, OTR_PROTOCOL, peer);

    /*
     * libotr does not call the callback "gone_insecure" when the session is
     * ended locally, so the end of session is handled here.
     */
    ptr_context = otrl_context_find (otr_us, peer, account, OTR_PROTOCOL,
                                     OTRL_INSTAG_MASTER, 0, NULL, NULL, NULL);
    if (ptr_context)
    {
        if (display)
        {
            otr_buffer_ctx_print (ptr_context, OTR_CONFIG_LEVEL_INFO,
                                  _("Private conversation ended."));
        }
        otr_context_smp_reset (
            (struct t_otr_peer_data *)ptr_context->app_data);
        otr_log_restore (ptr_context,
                         (struct t_otr_peer_data *)ptr_context->app_data,
                         buffer_closing);
    }

    weechat_bar_item_update (OTR_BAR_ITEM);

    return 1;
}
