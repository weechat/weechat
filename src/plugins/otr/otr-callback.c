/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* libotr callbacks */

#include <stdio.h>
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


/*
 * Callback for libotr: get the OTR policy for a context, built from the OTR
 * policies options.
 *
 * Return OTR policy (0 if OTR versions 2 and 3 are both disabled).
 */

OtrlPolicy
otr_callback_policy (void *opdata, ConnContext *context)
{
    OtrlPolicy policy;
    int allow_v2, allow_v3;

    /* Make C compiler happy. */
    (void) opdata;

    allow_v2 = otr_policy_ctx (context, "allow_v2");
    allow_v3 = otr_policy_ctx (context, "allow_v3");
    if (!allow_v2 && !allow_v3)
        return 0;

    policy = OTRL_POLICY_WHITESPACE_START_AKE | OTRL_POLICY_ERROR_START_AKE;
    if (allow_v2)
        policy |= OTRL_POLICY_ALLOW_V2;
    if (allow_v3)
        policy |= OTRL_POLICY_ALLOW_V3;
    if (otr_policy_ctx (context, "require_encryption"))
        policy |= OTRL_POLICY_REQUIRE_ENCRYPTION;
    if (!otr_no_tag && otr_policy_ctx (context, "send_tag")
        && !otr_policy_no_send_tag (context->username))
    {
        policy |= OTRL_POLICY_SEND_WHITESPACE_TAG;
    }

    return policy;
}

/*
 * Callback for libotr: generate a private key for an account (this blocks
 * WeeChat a few seconds).
 */

void
otr_callback_create_privkey (void *opdata, const char *accountname,
                             const char *protocol)
{
    gcry_error_t error;

    /* Make C compiler happy. */
    (void) opdata;

    otr_buffer_print_str (
        NULL, OTR_CONFIG_LEVEL_INFO,
        _("Generating a new OTR key for this account. This can take a few "
          "seconds and blocks WeeChat meanwhile..."));
    error = otrl_privkey_generate (otr_us, otr_keyfile, accountname, protocol);
    if (error)
    {
        otr_buffer_error_core (_("key generation failed: %s"),
                               gcry_strerror (error));
    }
    else
    {
        otr_buffer_print_str (NULL, OTR_CONFIG_LEVEL_SUCCESS,
                              _("OTR key generated."));
    }
}

/*
 * Callback for libotr: generate an instance tag for an account.
 */

void
otr_callback_create_instag (void *opdata, const char *accountname,
                            const char *protocol)
{
    /* Make C compiler happy. */
    (void) opdata;

    otrl_instag_generate (otr_us, otr_instagfile, accountname, protocol);
}

/*
 * Callback for libotr: check if a recipient is logged in.
 *
 * On IRC, we can not know if the recipient is online, so the recipient is
 * considered logged in if we are connected to the server (this allows libotr
 * to notify the recipient when the OTR session is ended).
 *
 * Return 1 if we are connected to the server, 0 otherwise.
 */

int
otr_callback_is_logged_in (void *opdata, const char *accountname,
                           const char *protocol, const char *recipient)
{
    char *info_nick;
    int logged_in;

    /* Make C compiler happy. */
    (void) opdata;
    (void) protocol;
    (void) recipient;

    info_nick = weechat_info_get ("irc_nick",
                                  otr_irc_account_server (accountname));
    logged_in = (info_nick && info_nick[0]) ? 1 : 0;
    free (info_nick);

    return logged_in;
}

/*
 * Callback for libotr: send an OTR protocol message to a recipient.
 */

void
otr_callback_inject_message (void *opdata, const char *accountname,
                             const char *protocol, const char *recipient,
                             const char *message)
{
    /* Make C compiler happy. */
    (void) opdata;
    (void) protocol;

    otr_irc_send_raw (otr_irc_account_server (accountname), recipient, message);
}

/*
 * Callback for libotr: update the list of contexts (nothing is done).
 */

void
otr_callback_update_context_list (void *opdata)
{
    /* Make C compiler happy. */
    (void) opdata;
}

/*
 * Callback for libotr: write the fingerprints file.
 */

void
otr_callback_write_fingerprints (void *opdata)
{
    /* Make C compiler happy. */
    (void) opdata;

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
otr_callback_max_message_size (void *opdata, ConnContext *context)
{
    int max_size;

    /* Make C compiler happy. */
    (void) opdata;

    max_size = 415 - 10 - (int)strlen (context->username);

    return (max_size < 100) ? 100 : max_size;
}

/*
 * Build instructions to verify the identity of the peer of a libotr context
 * (with fingerprints or SMP).
 *
 * Note: result must be freed after use.
 */

char *
otr_callback_verify_instructions (ConnContext *context)
{
    char own_fingerprint[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
    char peer_fingerprint[OTRL_PRIVKEY_FPRINT_HUMAN_LEN], *instructions;
    const char *ptr_server;

    ptr_server = otr_irc_account_server (context->accountname);

    snprintf (own_fingerprint, sizeof (own_fingerprint), "?");
    snprintf (peer_fingerprint, sizeof (peer_fingerprint), "?");
    otrl_privkey_fingerprint (otr_us, own_fingerprint, context->accountname,
                              context->protocol);
    if (context->active_fingerprint)
    {
        otrl_privkey_hash_to_human (peer_fingerprint,
                                    context->active_fingerprint->fingerprint);
    }

    weechat_asprintf (
        &instructions,
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
        own_fingerprint,
        context->username, ptr_server, peer_fingerprint,
        context->username, ptr_server,
        context->username, ptr_server,
        context->username, ptr_server);

    return instructions;
}

/*
 * Callback for libotr: display the new fingerprint of a peer.
 */

void
otr_callback_new_fingerprint (void *opdata, OtrlUserState us,
                              const char *accountname, const char *protocol,
                              const char *username,
                              unsigned char fingerprint[20])
{
    char human_fingerprint[OTRL_PRIVKEY_FPRINT_HUMAN_LEN], *message;

    /* Make C compiler happy. */
    (void) opdata;
    (void) us;
    (void) protocol;

    otrl_privkey_hash_to_human (human_fingerprint, fingerprint);
    weechat_asprintf (&message, _("New fingerprint: %s"), human_fingerprint);
    if (message)
    {
        otr_buffer_peer_print (otr_irc_account_server (accountname), username,
                               OTR_CONFIG_LEVEL_WARNING, message);
        free (message);
    }
}

/*
 * Callback for libotr: the OTR session with a peer is now encrypted.
 *
 * Logging is disabled (unless policy "log" is enabled), and if the peer is not
 * authenticated, instructions to authenticate it are displayed.
 */

void
otr_callback_gone_secure (void *opdata, ConnContext *context)
{
    struct t_otr_peer_data *ptr_peer_data;
    struct t_gui_buffer *ptr_buffer;
    char *instructions;
    int verified;

    /* Make C compiler happy. */
    (void) opdata;

    ptr_peer_data = otr_context_peer_data_ctx (context);
    verified = (context->active_fingerprint
                && otrl_context_is_fingerprint_trusted (
                    context->active_fingerprint)) ? 1 : 0;

    /*
     * Disable logging before anything decrypted can be printed (if the
     * private buffer does not exist yet, logging is disabled when it is
     * opened, see function otr_buffer_irc_pv_opened_cb).
     */
    if (!otr_policy_ctx (context, "log"))
    {
        otr_log_disable (context, ptr_peer_data);
    }
    else
    {
        ptr_buffer = otr_irc_buffer (
            otr_irc_account_server (context->accountname), context->username);
        if (ptr_peer_data && (ptr_peer_data->prev_log == OTR_LOG_UNTOUCHED)
            && otr_buffer_is_private (ptr_buffer))
        {
            ptr_peer_data->prev_log = otr_log_buffer_log_level (ptr_buffer);
        }
        if (otr_log_buffer_is_logged (ptr_buffer))
        {
            otr_buffer_ctx_hint (
                context,
                _("You have enabled the recording to disk of OTR "
                  "conversations. By doing this you are potentially putting "
                  "yourself and your correspondent in danger. Please consider "
                  "disabling this policy with \"/otr policy default log off\". "
                  "To disable logging for this OTR session, use \"/otr log "
                  "stop\""));
        }
    }

    if (verified)
    {
        otr_buffer_ctx_print (
            context, OTR_CONFIG_LEVEL_SUCCESS,
            _("Authenticated secured OTR conversation started."));
    }
    else
    {
        otr_buffer_ctx_print (
            context, OTR_CONFIG_LEVEL_WARNING,
            _("Unauthenticated secured OTR conversation started."));
        instructions = otr_callback_verify_instructions (context);
        if (instructions)
        {
            otr_buffer_ctx_hint (context, instructions);
            free (instructions);
        }
    }

    weechat_bar_item_update (OTR_BAR_ITEM);
}

/*
 * Callback for libotr: the OTR session with a peer has been refreshed.
 */

void
otr_callback_still_secure (void *opdata, ConnContext *context, int is_reply)
{
    /* Make C compiler happy. */
    (void) opdata;
    (void) is_reply;

    otr_buffer_ctx_print (context, OTR_CONFIG_LEVEL_SUCCESS,
                          _("Private conversation has been refreshed."));

    weechat_bar_item_update (OTR_BAR_ITEM);
}

/*
 * Callback for libotr: get the prefix added to resent messages.
 *
 * Note: result is freed by function otr_callback_resent_msg_prefix_free.
 */

const char *
otr_callback_resent_msg_prefix (void *opdata, ConnContext *context)
{
    /* Make C compiler happy. */
    (void) opdata;
    (void) context;

    return strdup ("[resent] ");
}

/*
 * Callback for libotr: free the prefix added to resent messages.
 */

void
otr_callback_resent_msg_prefix_free (void *opdata, const char *prefix)
{
    /* Make C compiler happy. */
    (void) opdata;

    free ((void *)prefix);
}

/*
 * Callback for libotr: handle an SMP event (Socialist Millionaire Protocol).
 */

void
otr_callback_handle_smp_event (void *opdata, OtrlSMPEvent event,
                               ConnContext *context, unsigned short progress,
                               char *question)
{
    struct t_otr_peer_data *ptr_peer_data;
    int in_smp, smp_question, smp_asked;

    /* Make C compiler happy. */
    (void) opdata;

    ptr_peer_data = otr_context_peer_data_ctx (context);

    /*
     * New values for the SMP state of the peer (-1 = unchanged); smp_asked is
     * reset by any event except a request of the peer.
     */
    in_smp = -1;
    smp_question = -1;
    smp_asked = 0;

    switch (event)
    {
        case OTRL_SMPEVENT_ASK_FOR_SECRET:
            in_smp = 1;
            smp_question = 0;
            smp_asked = 1;
            otr_buffer_ctx_print (context, OTR_CONFIG_LEVEL_INFO,
                                  _("Peer has requested SMP verification.\n"
                                    "Respond with: /otr smp respond <secret>"));
            break;
        case OTRL_SMPEVENT_ASK_FOR_ANSWER:
            in_smp = 1;
            smp_question = 1;
            smp_asked = 1;
            otr_buffer_ctx_print (context, OTR_CONFIG_LEVEL_INFO,
                                  _("Peer has requested SMP verification: %s\n"
                                    "Respond with: /otr smp respond <answer>"),
                                  (question) ? question : "");
            break;
        case OTRL_SMPEVENT_IN_PROGRESS:
            otr_buffer_ctx_print (context, OTR_CONFIG_LEVEL_INFO,
                                  _("SMP progressing (%u%%)."),
                                  (unsigned int)progress);
            break;
        case OTRL_SMPEVENT_SUCCESS:
            /*
             * The trust of the fingerprint is set by libotr, except if we
             * answered a question of the peer: in this case only the peer
             * has authenticated us, not the opposite.
             */
            otr_buffer_ctx_print (context, OTR_CONFIG_LEVEL_SUCCESS,
                                  _("SMP verification succeeded."));
            if (ptr_peer_data && ptr_peer_data->smp_question)
            {
                otr_buffer_ctx_print (
                    context, OTR_CONFIG_LEVEL_INFO,
                    _("You may want to authenticate your peer by asking your "
                      "own question:\n/otr smp ask <'question'> 'secret'"));
            }
            in_smp = 0;
            smp_question = 0;
            break;
        case OTRL_SMPEVENT_FAILURE:
            otr_buffer_ctx_print (context, OTR_CONFIG_LEVEL_ERROR,
                                  _("SMP verification failed."));
            in_smp = 0;
            smp_question = 0;
            break;
        case OTRL_SMPEVENT_ABORT:
            otr_buffer_ctx_print (context, OTR_CONFIG_LEVEL_WARNING,
                                  _("SMP aborted."));
            in_smp = 0;
            smp_question = 0;
            break;
        case OTRL_SMPEVENT_CHEATED:
            otr_buffer_ctx_print (
                context, OTR_CONFIG_LEVEL_ERROR,
                _("SMP aborted: the peer behaved incorrectly."));
            in_smp = 0;
            smp_question = 0;
            break;
        case OTRL_SMPEVENT_ERROR:
            otr_buffer_ctx_print (context, OTR_CONFIG_LEVEL_ERROR,
                                  _("SMP error."));
            in_smp = 0;
            smp_question = 0;
            break;
        default:
            break;
    }

    if (ptr_peer_data)
    {
        if (in_smp >= 0)
            ptr_peer_data->in_smp = in_smp;
        if (smp_question >= 0)
            ptr_peer_data->smp_question = smp_question;
        ptr_peer_data->smp_asked = smp_asked;
    }

    weechat_bar_item_update (OTR_BAR_ITEM);
}

/*
 * Callback for libotr: handle a message event (display it for the peer).
 *
 * When receiving a message, opdata is a pointer to a string (char **): for
 * event OTRL_MSGEVENT_RCVDMSG_UNENCRYPTED (plaintext received while the
 * session is encrypted or encryption is required), the plaintext message,
 * which libotr ignores, is copied into this string, to be displayed by the
 * caller (the string must be freed after use).
 */

void
otr_callback_handle_msg_event (void *opdata, OtrlMessageEvent event,
                               ConnContext *context, const char *message,
                               gcry_error_t error)
{
    char **unencrypted;

    if (!context)
        return;

    switch (event)
    {
        case OTRL_MSGEVENT_ENCRYPTION_REQUIRED:
            otr_buffer_ctx_print (
                context, OTR_CONFIG_LEVEL_ERROR,
                _("Your message was NOT sent in the clear, because policy "
                  "requires an encrypted connection. It is held back and will "
                  "be sent once the OTR session is established."));
            otr_buffer_ctx_hint (
                context,
                _("Wait for the OTR connection or change the policy to allow "
                  "clear-text messages:\n/otr policy require_encryption off"));
            break;
        case OTRL_MSGEVENT_ENCRYPTION_ERROR:
            otr_buffer_ctx_print (
                context, OTR_CONFIG_LEVEL_ERROR,
                _("An error occurred while encrypting your message. It was NOT "
                  "sent."));
            break;
        case OTRL_MSGEVENT_CONNECTION_ENDED:
            otr_buffer_ctx_print (
                context, OTR_CONFIG_LEVEL_ERROR,
                _("Your message was not sent. End your private "
                  "conversation:\n/otr finish"));
            break;
        case OTRL_MSGEVENT_SETUP_ERROR:
            otr_buffer_ctx_print (
                context, OTR_CONFIG_LEVEL_ERROR,
                _("Error setting up the private conversation: %s"),
                (error) ? gcry_strerror (error) : "unknown error");
            break;
        case OTRL_MSGEVENT_MSG_REFLECTED:
            otr_buffer_ctx_print (
                context, OTR_CONFIG_LEVEL_WARNING,
                _("Received our own OTR message (reflected); ignored."));
            break;
        case OTRL_MSGEVENT_MSG_RESENT:
            otr_buffer_ctx_print (context, OTR_CONFIG_LEVEL_INFO,
                                  _("The last message was resent."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_NOT_IN_PRIVATE:
            otr_buffer_ctx_print (
                context, OTR_CONFIG_LEVEL_WARNING,
                _("Received encrypted data but no private session "
                  "established."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_UNREADABLE:
            otr_buffer_ctx_print (
                context, OTR_CONFIG_LEVEL_ERROR,
                _("Received an unreadable encrypted message."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_MALFORMED:
            otr_buffer_ctx_print (context, OTR_CONFIG_LEVEL_ERROR,
                                  _("Received a malformed OTR message."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_GENERAL_ERR:
            otr_buffer_ctx_print (context, OTR_CONFIG_LEVEL_ERROR,
                                  _("Received OTR error: %s"),
                                  (message) ? message : "");
            break;
        case OTRL_MSGEVENT_RCVDMSG_UNRECOGNIZED:
            otr_buffer_ctx_print (context, OTR_CONFIG_LEVEL_WARNING,
                                  _("Received an unrecognized OTR message."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_FOR_OTHER_INSTANCE:
            otr_buffer_ctx_print (
                context, OTR_CONFIG_LEVEL_INFO,
                _("Received a message meant for another client instance of "
                  "yours; ignored."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_UNENCRYPTED:
            /* Displayed with a message prefix by otr_modifier_in_cb. */
            if (opdata && message)
            {
                unencrypted = (char **)opdata;
                free (*unencrypted);
                *unencrypted = strdup (message);
            }
            break;
        default:
            otr_buffer_debug ("msg event %d", (int)event);
            break;
    }
}

const OtrlMessageAppOps otr_callback_ops = {
    .policy = &otr_callback_policy,
    .create_privkey = &otr_callback_create_privkey,
    .is_logged_in = &otr_callback_is_logged_in,
    .inject_message = &otr_callback_inject_message,
    .update_context_list = &otr_callback_update_context_list,
    .new_fingerprint = &otr_callback_new_fingerprint,
    .write_fingerprints = &otr_callback_write_fingerprints,
    .gone_secure = &otr_callback_gone_secure,
    .still_secure = &otr_callback_still_secure,
    .max_message_size = &otr_callback_max_message_size,
    .resent_msg_prefix = &otr_callback_resent_msg_prefix,
    .resent_msg_prefix_free = &otr_callback_resent_msg_prefix_free,
    .handle_smp_event = &otr_callback_handle_smp_event,
    .handle_msg_event = &otr_callback_handle_msg_event,
    .create_instag = &otr_callback_create_instag,
};
