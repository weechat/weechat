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


static OtrlPolicy
op_policy (void *opdata, ConnContext *ctx)
{
    OtrlPolicy p = 0;
    int v2 = policy_ctx (ctx, "allow_v2"), v3 = policy_ctx (ctx, "allow_v3");

    (void)opdata;
    if (!v2 && !v3)
        return 0;
    p = OTRL_POLICY_WHITESPACE_START_AKE | OTRL_POLICY_ERROR_START_AKE;
    if (v2)
        p |= OTRL_POLICY_ALLOW_V2;
    if (v3)
        p |= OTRL_POLICY_ALLOW_V3;
    if (policy_ctx (ctx, "require_encryption"))
        p |= OTRL_POLICY_REQUIRE_ENCRYPTION;
    if (!g_no_tag && policy_ctx (ctx, "send_tag")
        && !no_send_tag (ctx->username))
        p |= OTRL_POLICY_SEND_WHITESPACE_TAG;
    return p;
}

static void
op_create_privkey (void *opdata, const char *accountname,
                   const char *protocol)
{
    gcry_error_t err;

    (void)opdata;
    otr_print_str (NULL, LVL_INFO, _("Generating a new OTR key for this "
                   "account. This can take a few seconds and blocks WeeChat "
                   "meanwhile..."));
    err = otrl_privkey_generate (otr_us, otr_keyfile, accountname, protocol);
    if (err)
        otr_error_core (_("key generation failed: %s"), gcry_strerror (err));
    else
        otr_print_str (NULL, LVL_SUCCESS, _("OTR key generated."));
}

static void
op_create_instag (void *opdata, const char *accountname,
                  const char *protocol)
{
    (void)opdata;
    otrl_instag_generate (otr_us, otr_instagfile, accountname, protocol);
}

static int
op_is_logged_in (void *opdata, const char *accountname, const char *protocol,
                 const char *recipient)
{
    (void)opdata; (void)accountname; (void)protocol; (void)recipient;
    return -1; /* unknown */
}

static void
op_inject_message (void *opdata, const char *accountname,
                   const char *protocol, const char *recipient,
                   const char *message)
{
    (void)opdata; (void)protocol;
    send_raw (account_server (accountname), recipient, message);
}

static void
op_update_context_list (void *opdata)
{
    (void)opdata;
}

static void
op_write_fingerprints (void *opdata)
{
    (void)opdata;
    otrl_privkey_write_fingerprints (otr_us, otr_fprfile);
}

static int
op_max_message_size (void *opdata, ConnContext *ctx)
{
    int r = 415 - 10 - (int)strlen (ctx->username);

    (void)opdata;
    return r < 100 ? 100 : r;
}

static char *
verify_instructions (ConnContext *ctx)
{
    char own[OTRL_PRIVKEY_FPRINT_HUMAN_LEN], theirs[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
    const char *server = account_server (ctx->accountname);

    strcpy (own, "?");
    strcpy (theirs, "?");
    otrl_privkey_fingerprint (otr_us, own, ctx->accountname, ctx->protocol);
    if (ctx->active_fingerprint)
        otrl_privkey_hash_to_human (theirs,
                                    ctx->active_fingerprint->fingerprint);
    return strf (
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

static void
op_new_fingerprint (void *opdata, OtrlUserState us, const char *accountname,
                    const char *protocol, const char *username,
                    unsigned char fingerprint[20])
{
    char human[OTRL_PRIVKEY_FPRINT_HUMAN_LEN], *m;

    (void)opdata; (void)us; (void)protocol;
    otrl_privkey_hash_to_human (human, fingerprint);
    m = strf (_("New fingerprint: %s"), human);
    if (m)
        peer_print (account_server (accountname), username, LVL_WARNING, m);
    free (m);
}

static void
op_gone_secure (void *opdata, ConnContext *ctx)
{
    struct peer_data *pd = peer_data_ctx (ctx);
    int verified = ctx->active_fingerprint
        && otrl_context_is_fingerprint_trusted (ctx->active_fingerprint);
    char *instr;

    (void)opdata;

    /* disable logging before anything decrypted can be printed */
    if (!policy_ctx (ctx, "log"))
        log_disable (ctx, pd);
    else
    {
        struct t_gui_buffer *buf = irc_buffer (account_server (ctx->accountname),
                                               ctx->username);

        if (pd && pd->prev_log == LOG_UNTOUCHED && buf_is_private (buf))
            pd->prev_log = buf_log_level (buf);
        if (buf_is_logged (buf))
            ctx_hint (ctx,
                      _("You have enabled the recording to disk of OTR "
                      "conversations. By doing this you are potentially "
                      "putting yourself and your correspondent in danger. "
                      "Please consider disabling this policy with "
                      "\"/otr policy default log off\". To disable logging "
                      "for this OTR session, use \"/otr log stop\""));
    }

    if (verified)
        ctx_print (ctx, LVL_SUCCESS,
                   _("Authenticated secured OTR conversation started."));
    else
    {
        ctx_print (ctx, LVL_WARNING,
                   _("Unauthenticated secured OTR conversation started."));
        instr = verify_instructions (ctx);
        if (instr)
            ctx_hint (ctx, instr);
        free (instr);
    }
    weechat_bar_item_update (OTR_BAR_ITEM);
}

static void
op_gone_insecure (void *opdata, ConnContext *ctx)
{
    (void)opdata;
    if (g_shutdown)
    {
        log_restore (ctx, peer_data_ctx (ctx));
        return;
    }
    if (ctx->msgstate == OTRL_MSGSTATE_FINISHED)
        ctx_print (ctx, LVL_INFO,
                   _("%s has ended the private conversation. You should do "
                   "the same:\n/otr finish"), ctx->username);
    else
    {
        ctx_print (ctx, LVL_INFO, _("Private conversation ended."));
        log_restore (ctx, peer_data_ctx (ctx));
    }
    weechat_bar_item_update (OTR_BAR_ITEM);
}

static void
op_still_secure (void *opdata, ConnContext *ctx, int is_reply)
{
    (void)opdata; (void)is_reply;
    ctx_print (ctx, LVL_SUCCESS, _("Private conversation has been refreshed."));
    weechat_bar_item_update (OTR_BAR_ITEM);
}

static const char *
op_resent_msg_prefix (void *opdata, ConnContext *ctx)
{
    (void)opdata; (void)ctx;
    return strdup ("[resent] ");
}

static void
op_resent_msg_prefix_free (void *opdata, const char *prefix)
{
    (void)opdata;
    free ((void *)prefix);
}

static void
op_handle_smp_event (void *opdata, OtrlSMPEvent ev, ConnContext *ctx,
                     unsigned short progress, char *question)
{
    struct peer_data *pd = peer_data_ctx (ctx);

    (void)opdata;
    switch (ev)
    {
        case OTRL_SMPEVENT_ASK_FOR_SECRET:
            if (pd) { pd->in_smp = 1; pd->smp_question = 0; }
            ctx_print (ctx, LVL_INFO,
                       _("Peer has requested SMP verification.\n"
                       "Respond with: /otr smp respond <secret>"));
            break;
        case OTRL_SMPEVENT_ASK_FOR_ANSWER:
            if (pd) { pd->in_smp = 1; pd->smp_question = 1; }
            ctx_print (ctx, LVL_INFO,
                       _("Peer has requested SMP verification: %s\n"
                       "Respond with: /otr smp respond <answer>"),
                       question ? question : "");
            break;
        case OTRL_SMPEVENT_IN_PROGRESS:
            ctx_print (ctx, LVL_INFO, _("SMP progressing (%u%%)."),
                       (unsigned)progress);
            break;
        case OTRL_SMPEVENT_SUCCESS:
            /* make sure the fingerprint is remembered as SMP verified */
            if (ctx->active_fingerprint
                && !otrl_context_is_fingerprint_trusted (
                       ctx->active_fingerprint))
            {
                otrl_context_set_trust (ctx->active_fingerprint, "smp");
                otrl_privkey_write_fingerprints (otr_us, otr_fprfile);
            }
            ctx_print (ctx, LVL_SUCCESS, _("SMP verification succeeded."));
            if (pd && pd->smp_question)
                ctx_print (ctx, LVL_INFO,
                           _("You may want to authenticate your peer by "
                           "asking your own question:\n"
                           "/otr smp ask <'question'> 'secret'"));
            if (pd) { pd->in_smp = 0; pd->smp_question = 0; }
            break;
        case OTRL_SMPEVENT_FAILURE:
            ctx_print (ctx, LVL_ERROR, _("SMP verification failed."));
            if (pd) { pd->in_smp = 0; pd->smp_question = 0; }
            break;
        case OTRL_SMPEVENT_ABORT:
            ctx_print (ctx, LVL_WARNING, _("SMP aborted."));
            if (pd) { pd->in_smp = 0; pd->smp_question = 0; }
            break;
        case OTRL_SMPEVENT_CHEATED:
            ctx_print (ctx, LVL_ERROR,
                       _("SMP aborted: the peer behaved incorrectly."));
            if (pd) { pd->in_smp = 0; pd->smp_question = 0; }
            break;
        case OTRL_SMPEVENT_ERROR:
            ctx_print (ctx, LVL_ERROR, _("SMP error."));
            if (pd) { pd->in_smp = 0; pd->smp_question = 0; }
            break;
        default:
            break;
    }
    weechat_bar_item_update (OTR_BAR_ITEM);
}

static void
op_handle_msg_event (void *opdata, OtrlMessageEvent ev, ConnContext *ctx,
                     const char *message, gcry_error_t err)
{
    (void)opdata;
    if (!ctx)
        return;
    switch (ev)
    {
        case OTRL_MSGEVENT_ENCRYPTION_REQUIRED:
            ctx_print (ctx, LVL_ERROR,
                       _("Your message was NOT sent in the clear, because "
                       "policy requires an encrypted connection. It is held "
                       "back and will be sent once the OTR session is "
                       "established."));
            ctx_hint (ctx, _("Wait for the OTR connection or change the "
                      "policy to allow clear-text messages:\n"
                      "/otr policy require_encryption off"));
            break;
        case OTRL_MSGEVENT_ENCRYPTION_ERROR:
            ctx_print (ctx, LVL_ERROR,
                       _("An error occurred while encrypting your message. "
                       "It was NOT sent."));
            break;
        case OTRL_MSGEVENT_CONNECTION_ENDED:
            ctx_print (ctx, LVL_ERROR,
                       _("Your message was not sent. End your private "
                       "conversation:\n/otr finish"));
            break;
        case OTRL_MSGEVENT_SETUP_ERROR:
            ctx_print (ctx, LVL_ERROR,
                       _("Error setting up the private conversation: %s"),
                       err ? gcry_strerror (err) : "unknown error");
            break;
        case OTRL_MSGEVENT_MSG_REFLECTED:
            ctx_print (ctx, LVL_WARNING,
                       _("Received our own OTR message (reflected); ignored."));
            break;
        case OTRL_MSGEVENT_MSG_RESENT:
            ctx_print (ctx, LVL_INFO, _("The last message was resent."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_NOT_IN_PRIVATE:
            ctx_print (ctx, LVL_WARNING,
                       _("Received encrypted data but no private session "
                       "established."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_UNREADABLE:
            ctx_print (ctx, LVL_ERROR,
                       _("Received an unreadable encrypted message."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_MALFORMED:
            ctx_print (ctx, LVL_ERROR,
                       _("Received a malformed OTR message."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_GENERAL_ERR:
            ctx_print (ctx, LVL_ERROR, _("Received OTR error: %s"),
                       message ? message : "");
            break;
        case OTRL_MSGEVENT_RCVDMSG_UNRECOGNIZED:
            ctx_print (ctx, LVL_WARNING,
                       _("Received an unrecognized OTR message."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_FOR_OTHER_INSTANCE:
            ctx_print (ctx, LVL_INFO,
                       _("Received a message meant for another client "
                       "instance of yours; ignored."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_UNENCRYPTED:
            /* reported with a message prefix in modifier_in_cb */
            break;
        default:
            otr_debug ("msg event %d", (int)ev);
            break;
    }
}

const OtrlMessageAppOps otr_ops = {
    .policy = op_policy,
    .create_privkey = op_create_privkey,
    .is_logged_in = op_is_logged_in,
    .inject_message = op_inject_message,
    .update_context_list = op_update_context_list,
    .new_fingerprint = op_new_fingerprint,
    .write_fingerprints = op_write_fingerprints,
    .gone_secure = op_gone_secure,
    .gone_insecure = op_gone_insecure,
    .still_secure = op_still_secure,
    .max_message_size = op_max_message_size,
    .resent_msg_prefix = op_resent_msg_prefix,
    .resent_msg_prefix_free = op_resent_msg_prefix_free,
    .handle_smp_event = op_handle_smp_event,
    .handle_msg_event = op_handle_msg_event,
    .create_instag = op_create_instag,
};
