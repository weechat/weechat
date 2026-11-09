/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* libotr callbacks */

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
#include "otr-policy.h"
#include "otr-util.h"


/*
 * Callback for libotr: get the OTR policy for a context, built from the OTR
 * policies options.
 *
 * Return OTR policy (0 if OTR versions 2 and 3 are both disabled).
 */

OtrlPolicy
otr_callback_policy (void *opdata, ConnContext *ctx)
{
    OtrlPolicy p = 0;
    int v2 = otr_policy_ctx (ctx, "allow_v2");
    int v3 = otr_policy_ctx (ctx, "allow_v3");

    (void)opdata;
    if (!v2 && !v3)
        return 0;
    p = OTRL_POLICY_WHITESPACE_START_AKE | OTRL_POLICY_ERROR_START_AKE;
    if (v2)
        p |= OTRL_POLICY_ALLOW_V2;
    if (v3)
        p |= OTRL_POLICY_ALLOW_V3;
    if (otr_policy_ctx (ctx, "require_encryption"))
        p |= OTRL_POLICY_REQUIRE_ENCRYPTION;
    if (!otr_no_tag && otr_policy_ctx (ctx, "send_tag")
        && !otr_policy_no_send_tag (ctx->username))
        p |= OTRL_POLICY_SEND_WHITESPACE_TAG;
    return p;
}

/*
 * Callback for libotr: generate a private key for an account (this blocks
 * WeeChat a few seconds).
 */

void
otr_callback_create_privkey (void *opdata, const char *accountname,
                             const char *protocol)
{
    gcry_error_t err;

    (void)opdata;
    otr_buffer_print_str (
        NULL, OTR_CONFIG_LEVEL_INFO,
        _("Generating a new OTR key for this account. This can take a few "
        "seconds and blocks WeeChat meanwhile..."));
    err = otrl_privkey_generate (otr_us, otr_keyfile, accountname, protocol);
    if (err)
        otr_buffer_error_core (_("key generation failed: %s"),
                               gcry_strerror (err));
    else
        otr_buffer_print_str (
            NULL, OTR_CONFIG_LEVEL_SUCCESS, _("OTR key generated."));
}

/*
 * Callback for libotr: generate an instance tag for an account.
 */

void
otr_callback_create_instag (void *opdata, const char *accountname,
                            const char *protocol)
{
    (void)opdata;
    otrl_instag_generate (otr_us, otr_instagfile, accountname, protocol);
}

/*
 * Callback for libotr: check if a recipient is logged in.
 *
 * Return -1 (unknown).
 */

int
otr_callback_is_logged_in (void *opdata, const char *accountname,
                           const char *protocol,
                           const char *recipient)
{
    (void)opdata; (void)accountname; (void)protocol; (void)recipient;
    return -1; /* Unknown */
}

/*
 * Callback for libotr: send an OTR protocol message to a recipient.
 */

void
otr_callback_inject_message (void *opdata, const char *accountname,
                             const char *protocol, const char *recipient,
                             const char *message)
{
    (void)opdata; (void)protocol;
    otr_irc_send_raw (otr_irc_account_server (accountname), recipient, message);
}

/*
 * Callback for libotr: update the list of contexts (nothing is done).
 */

void
otr_callback_update_context_list (void *opdata)
{
    (void)opdata;
}

/*
 * Callback for libotr: write the fingerprints file.
 */

void
otr_callback_write_fingerprints (void *opdata)
{
    (void)opdata;
    otrl_privkey_write_fingerprints (otr_us, otr_fprfile);
}

/*
 * Callback for libotr: get the max size of an OTR message, according to the max
 * size of an IRC message and the length of the nick of the peer (libotr
 * fragments longer messages).
 *
 * Return max size of an OTR message (at least 100).
 */

int
otr_callback_max_message_size (void *opdata, ConnContext *ctx)
{
    int r = 415 - 10 - (int)strlen (ctx->username);

    (void)opdata;
    return r < 100 ? 100 : r;
}

/*
 * Build instructions to verify the identity of the peer of a libotr context
 * (with fingerprints or SMP).
 *
 * Note: result must be freed after use.
 */

char *
otr_callback_verify_instructions (ConnContext *ctx)
{
    char own[OTRL_PRIVKEY_FPRINT_HUMAN_LEN], theirs[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
    const char *server = otr_irc_account_server (ctx->accountname);

    strcpy (own, "?");
    strcpy (theirs, "?");
    otrl_privkey_fingerprint (otr_us, own, ctx->accountname, ctx->protocol);
    if (ctx->active_fingerprint)
        otrl_privkey_hash_to_human (theirs,
                                    ctx->active_fingerprint->fingerprint);
    return otr_util_strf (
        _("You can verify that this contact is who they claim to be in\n"
        "one of the following ways:\n\n"
        "1) Verify each other's fingerprints using a secure channel:\n"
        "  Your fingerprint : %s\n"
        "  %s@%s's fingerprint : %s\n"
        "  then use the command: /otr trust %s %s\n\n"
        "2) SMP pre-shared secret that you both know:\n"
        "  /otr smp ask %s %s 'secret'\n\n"
        "3) SMP pre-shared secret that you both know with a question:\n"
        "  /otr smp ask %s %s <'question'> 'secret'\n\n"
        "Note: You can safely omit specifying the peer and server when\n"
        "      executing these commands from the appropriate conversation\n"
        "      buffer"),
        own, ctx->username, server, theirs, ctx->username, server,
        ctx->username, server, ctx->username, server);
}

/*
 * Callback for libotr: display the new fingerprint of a peer.
 */

void
otr_callback_new_fingerprint (void *opdata, OtrlUserState us,
                              const char *accountname,
                              const char *protocol, const char *username,
                              unsigned char fingerprint[20])
{
    char human[OTRL_PRIVKEY_FPRINT_HUMAN_LEN], *m;

    (void)opdata; (void)us; (void)protocol;
    otrl_privkey_hash_to_human (human, fingerprint);
    m = otr_util_strf (_("New fingerprint: %s"), human);
    if (m)
        otr_buffer_peer_print (otr_irc_account_server (accountname), username,
                               OTR_CONFIG_LEVEL_WARNING, m);
    free (m);
}

/*
 * Callback for libotr: the OTR session with a peer is now encrypted.
 *
 * Logging is disabled (unless policy "log" is enabled), and if the peer is not
 * authenticated, instructions to authenticate it are displayed.
 */

void
otr_callback_gone_secure (void *opdata, ConnContext *ctx)
{
    struct t_otr_peer_data *pd = otr_context_peer_data_ctx (ctx);
    int verified = ctx->active_fingerprint
        && otrl_context_is_fingerprint_trusted (ctx->active_fingerprint);
    char *instr;

    (void)opdata;

    /* Disable logging before anything decrypted can be printed. */
    if (!otr_policy_ctx (ctx, "log"))
        otr_log_disable (ctx, pd);
    else
    {
        struct t_gui_buffer *buf = otr_irc_buffer (
            otr_irc_account_server (ctx->accountname), ctx->username);

        if (pd && pd->prev_log == OTR_LOG_UNTOUCHED
            && otr_buffer_is_private (buf))
            pd->prev_log = otr_log_buffer_log_level (buf);
        if (otr_log_buffer_is_logged (buf))
            otr_buffer_ctx_hint (ctx,
                                 _("You have enabled the recording to disk of "
                                 "OTR conversations. By doing this you are "
                                 "potentially putting yourself and your "
                                 "correspondent in danger. Please consider "
                                 "disabling this policy with \"/otr policy "
                                 "default log off\". To disable logging for "
                                 "this OTR session, use \"/otr log stop\""));
    }

    if (verified)
        otr_buffer_ctx_print (
            ctx, OTR_CONFIG_LEVEL_SUCCESS,
            _("Authenticated secured OTR conversation started."));
    else
    {
        otr_buffer_ctx_print (
            ctx, OTR_CONFIG_LEVEL_WARNING,
            _("Unauthenticated secured OTR conversation started."));
        instr = otr_callback_verify_instructions (ctx);
        if (instr)
            otr_buffer_ctx_hint (ctx, instr);
        free (instr);
    }
    weechat_bar_item_update (OTR_BAR_ITEM);
}

/*
 * Callback for libotr: the OTR session with a peer is not encrypted any more.
 *
 * If the peer ended the session, the user is asked to do the same, otherwise
 * the logger level is restored.
 */

void
otr_callback_gone_insecure (void *opdata, ConnContext *ctx)
{
    (void)opdata;
    if (otr_shutdown)
    {
        otr_log_restore (ctx, otr_context_peer_data_ctx (ctx));
        return;
    }
    if (ctx->msgstate == OTRL_MSGSTATE_FINISHED)
        otr_buffer_ctx_print (
            ctx, OTR_CONFIG_LEVEL_INFO,
            _("%s has ended the private conversation. You should do the "
            "same:\n/otr finish"),
            ctx->username);
    else
    {
        otr_buffer_ctx_print (
            ctx, OTR_CONFIG_LEVEL_INFO, _("Private conversation ended."));
        otr_log_restore (ctx, otr_context_peer_data_ctx (ctx));
    }
    weechat_bar_item_update (OTR_BAR_ITEM);
}

/*
 * Callback for libotr: the OTR session with a peer has been refreshed.
 */

void
otr_callback_still_secure (void *opdata, ConnContext *ctx, int is_reply)
{
    (void)opdata; (void)is_reply;
    otr_buffer_ctx_print (ctx, OTR_CONFIG_LEVEL_SUCCESS,
                          _("Private conversation has been refreshed."));
    weechat_bar_item_update (OTR_BAR_ITEM);
}

/*
 * Callback for libotr: get the prefix added to resent messages.
 *
 * Note: result is freed by function otr_callback_resent_msg_prefix_free.
 */

const char *
otr_callback_resent_msg_prefix (void *opdata, ConnContext *ctx)
{
    (void)opdata; (void)ctx;
    return strdup ("[resent] ");
}

/*
 * Callback for libotr: free the prefix added to resent messages.
 */

void
otr_callback_resent_msg_prefix_free (void *opdata, const char *prefix)
{
    (void)opdata;
    free ((void *)prefix);
}

/*
 * Callback for libotr: handle an SMP event (Socialist Millionaire Protocol).
 */

void
otr_callback_handle_smp_event (void *opdata, OtrlSMPEvent ev, ConnContext *ctx,
                               unsigned short progress, char *question)
{
    struct t_otr_peer_data *pd = otr_context_peer_data_ctx (ctx);

    (void)opdata;
    switch (ev)
    {
        case OTRL_SMPEVENT_ASK_FOR_SECRET:
            if (pd) { pd->in_smp = 1; pd->smp_question = 0; }
            otr_buffer_ctx_print (ctx, OTR_CONFIG_LEVEL_INFO,
                                  _("Peer has requested SMP verification.\n"
                                  "Respond with: /otr smp respond <secret>"));
            break;
        case OTRL_SMPEVENT_ASK_FOR_ANSWER:
            if (pd) { pd->in_smp = 1; pd->smp_question = 1; }
            otr_buffer_ctx_print (ctx, OTR_CONFIG_LEVEL_INFO,
                                  _("Peer has requested SMP verification: %s\n"
                                  "Respond with: /otr smp respond <answer>"),
                                  question ? question : "");
            break;
        case OTRL_SMPEVENT_IN_PROGRESS:
            otr_buffer_ctx_print (
                ctx, OTR_CONFIG_LEVEL_INFO, _("SMP progressing (%u%%)."),
                (unsigned)progress);
            break;
        case OTRL_SMPEVENT_SUCCESS:
            /* Make sure the fingerprint is remembered as SMP verified. */
            if (ctx->active_fingerprint
                && !otrl_context_is_fingerprint_trusted (
                       ctx->active_fingerprint))
            {
                otrl_context_set_trust (ctx->active_fingerprint, "smp");
                otrl_privkey_write_fingerprints (otr_us, otr_fprfile);
            }
            otr_buffer_ctx_print (ctx, OTR_CONFIG_LEVEL_SUCCESS,
                                  _("SMP verification succeeded."));
            if (pd && pd->smp_question)
                otr_buffer_ctx_print (ctx, OTR_CONFIG_LEVEL_INFO,
                                      _("You may want to authenticate your "
                                      "peer by asking your own question:\n/otr "
                                      "smp ask <'question'> 'secret'"));
            if (pd) { pd->in_smp = 0; pd->smp_question = 0; }
            break;
        case OTRL_SMPEVENT_FAILURE:
            otr_buffer_ctx_print (ctx, OTR_CONFIG_LEVEL_ERROR,
                                  _("SMP verification failed."));
            if (pd) { pd->in_smp = 0; pd->smp_question = 0; }
            break;
        case OTRL_SMPEVENT_ABORT:
            otr_buffer_ctx_print (
                ctx, OTR_CONFIG_LEVEL_WARNING, _("SMP aborted."));
            if (pd) { pd->in_smp = 0; pd->smp_question = 0; }
            break;
        case OTRL_SMPEVENT_CHEATED:
            otr_buffer_ctx_print (
                ctx, OTR_CONFIG_LEVEL_ERROR,
                _("SMP aborted: the peer behaved incorrectly."));
            if (pd) { pd->in_smp = 0; pd->smp_question = 0; }
            break;
        case OTRL_SMPEVENT_ERROR:
            otr_buffer_ctx_print (ctx, OTR_CONFIG_LEVEL_ERROR, _("SMP error."));
            if (pd) { pd->in_smp = 0; pd->smp_question = 0; }
            break;
        default:
            break;
    }
    weechat_bar_item_update (OTR_BAR_ITEM);
}

/*
 * Callback for libotr: handle a message event (display it for the peer).
 */

void
otr_callback_handle_msg_event (void *opdata, OtrlMessageEvent ev,
                               ConnContext *ctx,
                               const char *message, gcry_error_t err)
{
    (void)opdata;
    if (!ctx)
        return;
    switch (ev)
    {
        case OTRL_MSGEVENT_ENCRYPTION_REQUIRED:
            otr_buffer_ctx_print (ctx, OTR_CONFIG_LEVEL_ERROR,
                                  _("Your message was NOT sent in the clear, "
                                  "because policy requires an encrypted "
                                  "connection. It is held back and will be "
                                  "sent once the OTR session is established."));
            otr_buffer_ctx_hint (ctx,
                                 _("Wait for the OTR connection or change the "
                                 "policy to allow clear-text messages:\n"
                                 "/otr policy require_encryption off"));
            break;
        case OTRL_MSGEVENT_ENCRYPTION_ERROR:
            otr_buffer_ctx_print (ctx, OTR_CONFIG_LEVEL_ERROR,
                                  _("An error occurred while encrypting your "
                                  "message. It was NOT sent."));
            break;
        case OTRL_MSGEVENT_CONNECTION_ENDED:
            otr_buffer_ctx_print (ctx, OTR_CONFIG_LEVEL_ERROR,
                                  _("Your message was not sent. End your "
                                  "private conversation:\n/otr finish"));
            break;
        case OTRL_MSGEVENT_SETUP_ERROR:
            otr_buffer_ctx_print (ctx, OTR_CONFIG_LEVEL_ERROR,
                                  _("Error setting up the private "
                                  "conversation: %s"),
                                  err ? gcry_strerror (err) : "unknown error");
            break;
        case OTRL_MSGEVENT_MSG_REFLECTED:
            otr_buffer_ctx_print (
                ctx, OTR_CONFIG_LEVEL_WARNING,
                _("Received our own OTR message (reflected); ignored."));
            break;
        case OTRL_MSGEVENT_MSG_RESENT:
            otr_buffer_ctx_print (ctx, OTR_CONFIG_LEVEL_INFO,
                                  _("The last message was resent."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_NOT_IN_PRIVATE:
            otr_buffer_ctx_print (ctx, OTR_CONFIG_LEVEL_WARNING,
                                  _("Received encrypted data but no private "
                                  "session established."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_UNREADABLE:
            otr_buffer_ctx_print (
                ctx, OTR_CONFIG_LEVEL_ERROR,
                _("Received an unreadable encrypted message."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_MALFORMED:
            otr_buffer_ctx_print (ctx, OTR_CONFIG_LEVEL_ERROR,
                                  _("Received a malformed OTR message."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_GENERAL_ERR:
            otr_buffer_ctx_print (
                ctx, OTR_CONFIG_LEVEL_ERROR, _("Received OTR error: %s"),
                message ? message : "");
            break;
        case OTRL_MSGEVENT_RCVDMSG_UNRECOGNIZED:
            otr_buffer_ctx_print (ctx, OTR_CONFIG_LEVEL_WARNING,
                                  _("Received an unrecognized OTR message."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_FOR_OTHER_INSTANCE:
            otr_buffer_ctx_print (ctx, OTR_CONFIG_LEVEL_INFO,
                                  _("Received a message meant for another "
                                  "client instance of yours; ignored."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_UNENCRYPTED:
            /* Reported with a message prefix in otr_modifier_in_cb. */
            break;
        default:
            otr_buffer_debug ("msg event %d", (int)ev);
            break;
    }
}

const OtrlMessageAppOps otr_callback_ops = {
    .policy = otr_callback_policy,
    .create_privkey = otr_callback_create_privkey,
    .is_logged_in = otr_callback_is_logged_in,
    .inject_message = otr_callback_inject_message,
    .update_context_list = otr_callback_update_context_list,
    .new_fingerprint = otr_callback_new_fingerprint,
    .write_fingerprints = otr_callback_write_fingerprints,
    .gone_secure = otr_callback_gone_secure,
    .gone_insecure = otr_callback_gone_insecure,
    .still_secure = otr_callback_still_secure,
    .max_message_size = otr_callback_max_message_size,
    .resent_msg_prefix = otr_callback_resent_msg_prefix,
    .resent_msg_prefix_free = otr_callback_resent_msg_prefix_free,
    .handle_smp_event = otr_callback_handle_smp_event,
    .handle_msg_event = otr_callback_handle_msg_event,
    .create_instag = otr_callback_create_instag,
};
