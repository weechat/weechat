/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* OTR commands */

#include <ctype.h>
#include <stdio.h>
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
#include "otr-log.h"
#include "otr-policy.h"
#include "otr-util.h"


/*
 * Split a string into arguments, like a shell (quotes and backslash escapes are
 * supported); at most max_args arguments are stored in args.
 *
 * Return number of arguments, -1 if error (unterminated quote, too many
 * arguments or allocation error).
 *
 * Note: arguments stored in args must be freed after use.
 */

int
otr_command_split_args (const char *string, char **args, int max_args)
{
    const char *ptr_string;
    char *arg, *ptr_arg, quote;
    int num_args, i;

    ptr_string = string;
    num_args = 0;

    while (ptr_string[0])
    {
        while (isspace ((unsigned char)ptr_string[0]))
            ptr_string++;
        if (!ptr_string[0])
            break;
        if (num_args >= max_args)
            goto error;
        arg = malloc (strlen (ptr_string) + 1);
        if (!arg)
            goto error;
        ptr_arg = arg;
        quote = 0;
        while (ptr_string[0]
               && (quote || !isspace ((unsigned char)ptr_string[0])))
        {
            if (quote == '\'')
            {
                if (ptr_string[0] == '\'')
                    quote = 0;
                else
                    *ptr_arg++ = ptr_string[0];
                ptr_string++;
            }
            else if (quote == '"')
            {
                if (ptr_string[0] == '"')
                {
                    quote = 0;
                    ptr_string++;
                }
                else if ((ptr_string[0] == '\\')
                         && ((ptr_string[1] == '"')
                             || (ptr_string[1] == '\\')))
                {
                    *ptr_arg++ = ptr_string[1];
                    ptr_string += 2;
                }
                else
                {
                    *ptr_arg++ = *ptr_string++;
                }
            }
            else if ((ptr_string[0] == '\'') || (ptr_string[0] == '"'))
            {
                quote = ptr_string[0];
                ptr_string++;
            }
            else if ((ptr_string[0] == '\\') && ptr_string[1])
            {
                *ptr_arg++ = ptr_string[1];
                ptr_string += 2;
            }
            else
            {
                *ptr_arg++ = *ptr_string++;
            }
        }
        if (quote)
        {
            free (arg);
            goto error;
        }
        ptr_arg[0] = '\0';
        args[num_args++] = arg;
    }

    return num_args;

error:
    for (i = 0; i < num_args; i++)
    {
        free (args[i]);
    }
    return -1;
}

/*
 * Get the peer from command arguments "<nick> <server>" (if num_args is 2) or,
 * without arguments, from the current private buffer.
 *
 * Return 1 if OK, 0 if error.
 */

int
otr_command_peer_args (struct t_gui_buffer *buffer, char **args, int num_args,
                       struct t_otr_peer_ref *peer_ref)
{
    const char *ptr_nick, *ptr_server;

    if (num_args == 2)
    {
        snprintf (peer_ref->nick, sizeof (peer_ref->nick), "%s", args[0]);
        snprintf (peer_ref->server, sizeof (peer_ref->server), "%s", args[1]);
        return 1;
    }

    if ((num_args == 0) && otr_buffer_is_private (buffer))
    {
        ptr_nick = weechat_buffer_get_string (buffer, "localvar_channel");
        ptr_server = weechat_buffer_get_string (buffer, "localvar_server");
        if (ptr_nick && ptr_server)
        {
            snprintf (peer_ref->nick, sizeof (peer_ref->nick), "%s", ptr_nick);
            snprintf (peer_ref->server, sizeof (peer_ref->server),
                      "%s", ptr_server);
            return 1;
        }
    }

    return 0;
}

/*
 * Get the account and the peer nick (in lower case) for a peer; an error is
 * displayed if not connected to the server.
 *
 * Return 1 if OK (*account and *peer must be freed after use), 0 if error.
 */

int
otr_command_resolve (const struct t_otr_peer_ref *peer_ref, char **account,
                     char **peer)
{
    *account = otr_irc_make_account (peer_ref->server);
    *peer = weechat_string_tolower (peer_ref->nick);
    if (!*account || !*peer)
    {
        otr_buffer_error_core (_("not connected to server \"%s\""),
                               peer_ref->server);
        free (*account);
        free (*peer);
        *account = NULL;
        *peer = NULL;
        return 0;
    }

    return 1;
}

/*
 * Add a row with 4 columns to a table (for display).
 */

void
otr_command_table_add (struct t_otr_table *table, const char *column1,
                       const char *column2, const char *column3,
                       const char *column4)
{
    const char *columns[4];
    char *(*new_rows)[4];
    size_t length;
    int i;

    columns[0] = column1;
    columns[1] = column2;
    columns[2] = column3;
    columns[3] = column4;

    if (table->num_rows == table->size_alloc)
    {
        new_rows = realloc (
            table->rows,
            (size_t)((table->size_alloc) ? table->size_alloc * 2 : 8)
            * sizeof (table->rows[0]));
        if (!new_rows)
            return;
        table->rows = new_rows;
        table->size_alloc = (table->size_alloc) ? table->size_alloc * 2 : 8;
    }

    for (i = 0; i < 4; i++)
    {
        table->rows[table->num_rows][i] = otr_util_dupstr (columns[i]);
        if (table->rows[table->num_rows][i])
        {
            length = strlen (table->rows[table->num_rows][i]);
            if (length > table->column_width[i])
                table->column_width[i] = length;
        }
    }
    table->num_rows++;
}

/*
 * Display the first num_columns columns of a table with aligned columns, then
 * free the table.
 */

void
otr_command_table_print_free (struct t_otr_table *table, int num_columns)
{
    char **line;
    size_t length;
    int row, column;

    for (row = 0; row < table->num_rows; row++)
    {
        line = weechat_string_dyn_alloc (256);
        if (line)
        {
            for (column = 0; column < num_columns; column++)
            {
                if (column > 0)
                    weechat_string_dyn_concat (line, " |", -1);
                weechat_string_dyn_concat (line, table->rows[row][column], -1);
                for (length = strlen (table->rows[row][column]);
                     length < table->column_width[column]; length++)
                {
                    weechat_string_dyn_concat (line, " ", -1);
                }
            }
            otr_buffer_print_str (NULL, OTR_CONFIG_LEVEL_INFO, *line);
            weechat_string_dyn_free (line, 1);
        }
        for (column = 0; column < 4; column++)
        {
            free (table->rows[row][column]);
        }
    }
    free (table->rows);
}

/*
 * Display own fingerprints of all accounts.
 */

void
otr_command_show_account_fingerprints (void)
{
    struct t_otr_table table;
    OtrlPrivKey *ptr_privkey;
    char fingerprint[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];

    memset (&table, 0, sizeof (table));

    for (ptr_privkey = otr_us->privkey_root; ptr_privkey;
         ptr_privkey = ptr_privkey->next)
    {
        if (!otrl_privkey_fingerprint (otr_us, fingerprint,
                                       ptr_privkey->accountname,
                                       ptr_privkey->protocol))
        {
            continue;
        }
        otr_command_table_add (&table, ptr_privkey->accountname, fingerprint,
                               "", "");
    }

    otr_command_table_print_free (&table, 2);
}

/*
 * Display fingerprints of known peers; if search is not NULL, only peers whose
 * nick contains search are displayed.
 */

void
otr_command_show_peer_fingerprints (const char *search)
{
    struct t_otr_table table;
    ConnContext *ptr_context;
    Fingerprint *ptr_fingerprint;
    char fingerprint[OTRL_PRIVKEY_FPRINT_HUMAN_LEN], peer[256];
    const char *ptr_trust;

    memset (&table, 0, sizeof (table));

    for (ptr_context = otr_us->context_root; ptr_context;
         ptr_context = ptr_context->next)
    {
        if (ptr_context->their_instance != OTRL_INSTAG_MASTER)
            continue;
        snprintf (peer, sizeof (peer), "%s", ptr_context->username);
        if (search && !strstr (peer, search))
            continue;
        for (ptr_fingerprint = ptr_context->fingerprint_root.next;
             ptr_fingerprint; ptr_fingerprint = ptr_fingerprint->next)
        {
            otrl_privkey_hash_to_human (fingerprint,
                                        ptr_fingerprint->fingerprint);
            if (!ptr_fingerprint->trust || !ptr_fingerprint->trust[0])
                ptr_trust = _("unverified");
            else if (strcmp (ptr_fingerprint->trust, "smp") == 0)
                ptr_trust = _("SMP verified");
            else if (strcmp (ptr_fingerprint->trust, "verified") == 0)
                ptr_trust = _("verified");
            else
                ptr_trust = ptr_fingerprint->trust;
            otr_command_table_add (&table, peer, ptr_context->accountname,
                                   fingerprint, ptr_trust);
        }
    }

    otr_command_table_print_free (&table, 4);
}

/*
 * Start an OTR session with a peer (send an OTR query).
 *
 * Return WEECHAT_RC_OK if OK, WEECHAT_RC_ERROR if error.
 */

int
otr_command_start (struct t_gui_buffer *buffer,
                   const struct t_otr_peer_ref *peer_ref)
{
    char *account, *peer, *message;
    const char *ptr_query;
    int allow_v2, allow_v3;

    /* Make C compiler happy. */
    (void) buffer;

    if (!otr_command_resolve (peer_ref, &account, &peer))
        return WEECHAT_RC_ERROR;

    allow_v2 = otr_policy_for (account, peer, "allow_v2");
    allow_v3 = otr_policy_for (account, peer, "allow_v3");
    if (!allow_v2 && !allow_v3)
    {
        otr_buffer_peer_print (peer_ref->server, peer, OTR_CONFIG_LEVEL_ERROR,
                               _("OTR is disabled by policy (allow_v2 and "
                                 "allow_v3 are both off)."));
        goto end;
    }

    if (allow_v2 && allow_v3)
        ptr_query = "?OTRv23?";
    else if (allow_v3)
        ptr_query = "?OTRv3?";
    else
        ptr_query = "?OTRv2?";

    otr_buffer_peer_hint (
        peer_ref->server, peer,
        _("Sending OTR query... Please await confirmation of the OTR session "
          "being started before sending a message."));
    if (!otr_policy_for (account, peer, "send_tag"))
    {
        weechat_asprintf (
            &message,
            _("To try OTR on all conversations with %s@%s: /otr policy "
              "send_tag on"),
            peer, peer_ref->server);
        if (message)
        {
            otr_buffer_peer_hint (peer_ref->server, peer, message);
            free (message);
        }
    }
    otr_irc_send_raw (peer_ref->server, peer, ptr_query);

end:
    free (account);
    free (peer);
    return WEECHAT_RC_OK;
}

/*
 * End the OTR session with a peer.
 *
 * Return WEECHAT_RC_OK if OK, WEECHAT_RC_ERROR if error.
 */

int
otr_command_finish (const struct t_otr_peer_ref *peer_ref)
{
    char *account, *peer;

    if (!otr_command_resolve (peer_ref, &account, &peer))
        return WEECHAT_RC_ERROR;

    if (!otr_context_end_session (account, peer, 1, 0))
    {
        otr_buffer_peer_print (
            peer_ref->server, peer, OTR_CONFIG_LEVEL_INFO,
            _("There is no private conversation to finish."));
    }

    free (account);
    free (peer);
    return WEECHAT_RC_OK;
}

/*
 * Display the status of the OTR session with a peer (encryption, fingerprints,
 * authentication).
 *
 * Return WEECHAT_RC_OK if OK, WEECHAT_RC_ERROR if error.
 */

int
otr_command_status (const struct t_otr_peer_ref *peer_ref)
{
    char *account, *peer, *message;
    char own_fingerprint[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
    char peer_fingerprint[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
    ConnContext *ptr_context;

    if (!otr_command_resolve (peer_ref, &account, &peer))
        return WEECHAT_RC_ERROR;

    ptr_context = otr_context_find (account, peer);
    if (!otr_context_encrypted (ptr_context))
    {
        otr_buffer_peer_print (
            peer_ref->server, peer, OTR_CONFIG_LEVEL_WARNING,
            _("This current conversation is not encrypted."));
        goto end;
    }

    otr_buffer_peer_print (peer_ref->server, peer, OTR_CONFIG_LEVEL_SUCCESS,
                           _("This conversation is encrypted."));
    snprintf (own_fingerprint, sizeof (own_fingerprint), "?");
    snprintf (peer_fingerprint, sizeof (peer_fingerprint), "?");
    otrl_privkey_fingerprint (otr_us, own_fingerprint, account, OTR_PROTOCOL);
    if (ptr_context->active_fingerprint)
    {
        otrl_privkey_hash_to_human (
            peer_fingerprint, ptr_context->active_fingerprint->fingerprint);
    }
    weechat_asprintf (&message, _("Your fingerprint is: %s"), own_fingerprint);
    if (message)
    {
        otr_buffer_peer_print (peer_ref->server, peer, OTR_CONFIG_LEVEL_INFO,
                               message);
        free (message);
    }
    weechat_asprintf (&message, _("Your peer's fingerprint is: %s"),
                      peer_fingerprint);
    if (message)
    {
        otr_buffer_peer_print (peer_ref->server, peer, OTR_CONFIG_LEVEL_INFO,
                               message);
        free (message);
    }
    if (otr_context_verified (ptr_context))
    {
        otr_buffer_peer_print (peer_ref->server, peer, OTR_CONFIG_LEVEL_SUCCESS,
                               _("The peer's identity has been verified."));
    }
    else
    {
        otr_buffer_peer_print (
            peer_ref->server, peer, OTR_CONFIG_LEVEL_WARNING,
            _("You have not verified the peer's identity yet."));
    }

end:
    free (account);
    free (peer);
    return WEECHAT_RC_OK;
}

/*
 * Mark the active fingerprint of a peer as verified (if trust is 1) or not
 * verified (if trust is 0).
 *
 * Return WEECHAT_RC_OK if OK, WEECHAT_RC_ERROR if error.
 */

int
otr_command_trust (const struct t_otr_peer_ref *peer_ref, int trust)
{
    char *account, *peer, *message;
    ConnContext *ptr_context;

    if (!otr_command_resolve (peer_ref, &account, &peer))
        return WEECHAT_RC_ERROR;

    ptr_context = otr_context_find (account, peer);
    if (otr_context_encrypted (ptr_context) && ptr_context->active_fingerprint)
    {
        otrl_context_set_trust (ptr_context->active_fingerprint,
                                (trust) ? "verified" : "");
        otrl_privkey_write_fingerprints (otr_us, otr_fprfile);
        weechat_asprintf (&message,
                          (trust) ?
                          _("%s@%s is now authenticated.") :
                          _("%s@%s is now de-authenticated."),
                          peer, peer_ref->server);
        if (message)
        {
            otr_buffer_peer_print (peer_ref->server, peer,
                                   OTR_CONFIG_LEVEL_INFO, message);
            free (message);
        }
        weechat_bar_item_update (OTR_BAR_ITEM);
    }
    else
    {
        otr_buffer_peer_print (peer_ref->server, peer, OTR_CONFIG_LEVEL_ERROR,
                               _("No active fingerprint. Start an OTR "
                                 "conversation first: /otr start"));
    }

    free (account);
    free (peer);
    return WEECHAT_RC_OK;
}

/*
 * Run subcommand "smp": ask, respond or abort the authentication of a peer with
 * SMP (Socialist Millionaire Protocol).
 *
 * Return WEECHAT_RC_OK if OK, WEECHAT_RC_ERROR if error.
 */

int
otr_command_smp (struct t_gui_buffer *buffer, char **args, int num_args)
{
    struct t_otr_peer_ref peer_ref;
    struct t_otr_peer_data *ptr_peer_data;
    char *account, *peer, *message;
    const char *ptr_secret, *ptr_question, *ptr_action;
    ConnContext *ptr_context;
    int peer_ok;

    ptr_action = args[1];
    ptr_secret = NULL;
    ptr_question = NULL;
    peer_ok = 0;

    if (strcmp (ptr_action, "respond") == 0)
    {
        if ((num_args == 3)
            && otr_command_peer_args (buffer, args + 2, 0, &peer_ref))
        {
            ptr_secret = args[2];
            peer_ok = 1;
        }
        else if ((num_args == 5)
                 && otr_command_peer_args (buffer, args + 2, 2, &peer_ref))
        {
            ptr_secret = args[4];
            peer_ok = 1;
        }
    }
    else if (strcmp (ptr_action, "ask") == 0)
    {
        if ((num_args == 3)
            && otr_command_peer_args (buffer, args + 2, 0, &peer_ref))
        {
            ptr_secret = args[2];
            peer_ok = 1;
        }
        else if ((num_args == 4)
                 && otr_command_peer_args (buffer, args + 2, 0, &peer_ref))
        {
            ptr_question = args[2];
            ptr_secret = args[3];
            peer_ok = 1;
        }
        else if ((num_args == 5)
                 && otr_command_peer_args (buffer, args + 2, 2, &peer_ref))
        {
            ptr_secret = args[4];
            peer_ok = 1;
        }
        else if ((num_args == 6)
                 && otr_command_peer_args (buffer, args + 2, 2, &peer_ref))
        {
            ptr_question = args[4];
            ptr_secret = args[5];
            peer_ok = 1;
        }
    }
    else if (strcmp (ptr_action, "abort") == 0)
    {
        if ((num_args == 2)
            && otr_command_peer_args (buffer, args + 2, 0, &peer_ref))
        {
            peer_ok = 1;
        }
        else if ((num_args == 4)
                 && otr_command_peer_args (buffer, args + 2, 2, &peer_ref))
        {
            peer_ok = 1;
        }
    }

    if (!peer_ok)
        return WEECHAT_RC_ERROR;

    if (!otr_command_resolve (&peer_ref, &account, &peer))
        return WEECHAT_RC_ERROR;

    ptr_context = otr_context_find (account, peer);
    if (!otr_context_encrypted (ptr_context))
    {
        weechat_asprintf (
            &message,
            _("There is currently no encrypted session with %s@%s."),
            peer, peer_ref.server);
        if (message)
        {
            otr_buffer_peer_print (peer_ref.server, peer,
                                   OTR_CONFIG_LEVEL_ERROR, message);
            free (message);
        }
        goto end;
    }

    ptr_peer_data = otr_context_peer_data_for (account, peer);

    if (strcmp (ptr_action, "respond") == 0)
    {
        otrl_message_respond_smp (otr_us, &otr_callback_ops, NULL, ptr_context,
                                  (const unsigned char *)ptr_secret,
                                  strlen (ptr_secret));
    }
    else if (strcmp (ptr_action, "ask") == 0)
    {
        if (ptr_question)
        {
            otrl_message_initiate_smp_q (otr_us, &otr_callback_ops, NULL,
                                         ptr_context, ptr_question,
                                         (const unsigned char *)ptr_secret,
                                         strlen (ptr_secret));
            otr_buffer_peer_print (peer_ref.server, peer, OTR_CONFIG_LEVEL_INFO,
                                   _("SMP question sent..."));
        }
        else
        {
            otrl_message_initiate_smp (otr_us, &otr_callback_ops, NULL,
                                       ptr_context,
                                       (const unsigned char *)ptr_secret,
                                       strlen (ptr_secret));
            otr_buffer_peer_print (peer_ref.server, peer, OTR_CONFIG_LEVEL_INFO,
                                   _("SMP challenge sent..."));
        }
        if (ptr_peer_data)
            ptr_peer_data->in_smp = 1;
    }
    else
    {
        otrl_message_abort_smp (otr_us, &otr_callback_ops, NULL, ptr_context);
        if (ptr_peer_data)
        {
            ptr_peer_data->in_smp = 0;
            ptr_peer_data->smp_question = 0;
        }
        otr_buffer_peer_print (peer_ref.server, peer, OTR_CONFIG_LEVEL_INFO,
                               _("SMP aborted."));
    }

end:
    free (account);
    free (peer);
    return WEECHAT_RC_OK;
}

/*
 * Run subcommand "log": display or change logging of the current OTR session.
 *
 * Return WEECHAT_RC_OK if OK, WEECHAT_RC_ERROR if error.
 */

int
otr_command_log (struct t_gui_buffer *buffer, char **args, int num_args)
{
    struct t_otr_peer_ref peer_ref;
    struct t_otr_peer_data *ptr_peer_data;
    char *account, *peer;
    ConnContext *ptr_context;

    if (!otr_command_peer_args (buffer, args + 1, 0, &peer_ref))
    {
        otr_buffer_print_str (NULL, OTR_CONFIG_LEVEL_ERROR,
                              _("OTR LOG: Not in an OTR session"));
        return WEECHAT_RC_OK;
    }

    if (!otr_command_resolve (&peer_ref, &account, &peer))
        return WEECHAT_RC_ERROR;

    ptr_context = otr_context_find (account, peer);
    ptr_peer_data = otr_context_peer_data_for (account, peer);

    if (!otr_context_encrypted (ptr_context))
    {
        otr_buffer_peer_print (peer_ref.server, peer, OTR_CONFIG_LEVEL_ERROR,
                               _("OTR LOG: Not in an OTR session"));
    }
    else if (num_args == 1)
    {
        if (otr_log_buffer_is_logged (buffer))
        {
            otr_buffer_peer_print (
                peer_ref.server, peer, OTR_CONFIG_LEVEL_WARNING,
                _("This conversation is currently being logged."));
        }
        else
        {
            otr_buffer_peer_print (
                peer_ref.server, peer, OTR_CONFIG_LEVEL_INFO,
                _("This conversation is currently NOT being logged."));
        }
    }
    else if ((strcmp (args[1], "start") == 0)
             && !otr_log_buffer_is_logged (buffer))
    {
        if (ptr_peer_data && (ptr_peer_data->prev_log == OTR_LOG_UNTOUCHED))
            ptr_peer_data->prev_log = otr_log_buffer_log_level (buffer);
        otr_buffer_peer_print (peer_ref.server, peer, OTR_CONFIG_LEVEL_WARNING,
                               _("From this point on, this conversation will "
                                 "be logged. Please keep in mind that by doing "
                                 "so you are potentially putting yourself and "
                                 "your interlocutor at risk. You can disable "
                                 "this by doing /otr log stop"));
        weechat_command (buffer, "/mute logger set 9");
    }
    else if ((strcmp (args[1], "stop") == 0)
             && otr_log_buffer_is_logged (buffer))
    {
        if (ptr_peer_data && (ptr_peer_data->prev_log == OTR_LOG_UNTOUCHED))
            ptr_peer_data->prev_log = otr_log_buffer_log_level (buffer);
        weechat_command (buffer, "/mute logger set 0");
        otr_buffer_peer_print (peer_ref.server, peer, OTR_CONFIG_LEVEL_INFO,
                               _("From this point on, this conversation will "
                                 "NOT be logged ANYMORE."));
    }
    weechat_bar_item_update (OTR_BAR_ITEM);

    free (account);
    free (peer);
    return WEECHAT_RC_OK;
}

/*
 * Check if a name is a known OTR policy (case insensitive).
 *
 * Return 1 if the policy is known, 0 otherwise.
 */

int
otr_command_policy_known (const char *name)
{
    int i;

    for (i = 0; i < OTR_CONFIG_NUM_POLICIES; i++)
    {
        if (weechat_strcasecmp (otr_config_policies[i].name, name) == 0)
            return 1;
    }

    return 0;
}

/*
 * Check if a value is valid for a policy: "on", "off" or "toggle" (case
 * insensitive).
 *
 * Return 1 if the value is valid, 0 otherwise.
 */

int
otr_command_value_ok (const char *value)
{
    return ((weechat_strcasecmp (value, "on") == 0)
            || (weechat_strcasecmp (value, "off") == 0)
            || (weechat_strcasecmp (value, "toggle") == 0)) ? 1 : 0;
}

/*
 * Display a text in the private buffer with the current peer, or in core buffer
 * if the current buffer is not a private buffer.
 */

void
otr_command_print_policies_here (struct t_gui_buffer *buffer, const char *text)
{
    struct t_otr_peer_ref peer_ref;
    char *account, *peer;

    if (otr_command_peer_args (buffer, NULL, 0, &peer_ref)
        && otr_command_resolve (&peer_ref, &account, &peer))
    {
        otr_buffer_peer_print (peer_ref.server, peer, OTR_CONFIG_LEVEL_INFO,
                               text);
        free (account);
        free (peer);
    }
    else
    {
        otr_buffer_print_str (NULL, OTR_CONFIG_LEVEL_INFO, text);
    }
}

/*
 * Run subcommand "policy": display or change OTR policies of the current peer
 * or default OTR policies.
 *
 * Return WEECHAT_RC_OK if OK, WEECHAT_RC_ERROR if error.
 */

int
otr_command_policy (struct t_gui_buffer *buffer, char **args, int num_args)
{
    struct t_otr_peer_ref peer_ref;
    char *account, *peer, *text, *command, nick[128];

    if ((num_args == 1)
        || ((num_args == 2) && (weechat_strcasecmp (args[1], "default") == 0)))
    {
        if ((num_args == 1)
            && otr_command_peer_args (buffer, NULL, 0, &peer_ref)
            && otr_command_resolve (&peer_ref, &account, &peer))
        {
            text = otr_policy_format_peer (account, peer);
            if (text)
            {
                otr_buffer_peer_print (peer_ref.server, peer,
                                       OTR_CONFIG_LEVEL_INFO, text);
                free (text);
            }
            free (account);
            free (peer);
        }
        else
        {
            text = otr_policy_format_default ();
            if (text)
            {
                otr_command_print_policies_here (buffer, text);
                free (text);
            }
        }
        return WEECHAT_RC_OK;
    }

    if ((num_args == 3) && otr_command_policy_known (args[1])
        && otr_command_value_ok (args[2]))
    {
        if (!otr_command_peer_args (buffer, NULL, 0, &peer_ref)
            || !otr_command_resolve (&peer_ref, &account, &peer))
        {
            return WEECHAT_RC_ERROR;
        }
        otr_irc_account_nick (account, nick, sizeof (nick));
        weechat_asprintf (&command, "/set otr.policy.%s.%s.%s.%s %s",
                          peer_ref.server, nick, peer, args[1], args[2]);
        if (command)
        {
            weechat_command (NULL, command);
            free (command);
        }
        text = otr_policy_format_peer (account, peer);
        if (text)
        {
            otr_buffer_peer_print (peer_ref.server, peer,
                                   OTR_CONFIG_LEVEL_INFO, text);
            free (text);
        }
        free (account);
        free (peer);
        return WEECHAT_RC_OK;
    }

    if ((num_args == 4) && (weechat_strcasecmp (args[1], "default") == 0)
        && otr_command_policy_known (args[2])
        && otr_command_value_ok (args[3]))
    {
        weechat_asprintf (&command, "/set otr.policy.default.%s %s",
                          args[2], args[3]);
        if (command)
        {
            weechat_command (NULL, command);
            free (command);
        }
        text = otr_policy_format_default ();
        if (text)
        {
            otr_command_print_policies_here (buffer, text);
            free (text);
        }
        return WEECHAT_RC_OK;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * Callback for command "/otr": manage OTR sessions.
 */

int
otr_command_otr (const void *pointer, void *data, struct t_gui_buffer *buffer,
                 int argc, char **argv, char **argv_eol)
{
    struct t_otr_peer_ref peer_ref;
    char *args[16];
    const char *ptr_command;
    int num_args, rc, i;

    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) argv;

    num_args = 0;
    rc = WEECHAT_RC_ERROR;

    if (argc > 1)
        num_args = otr_command_split_args (argv_eol[1], args, 16);
    if (num_args <= 0)
    {
        otr_buffer_error_core (_("invalid arguments, see /help otr"));
        return WEECHAT_RC_ERROR;
    }

    ptr_command = args[0];

    if (((num_args == 1) || (num_args == 3))
        && ((strcmp (ptr_command, "start") == 0)
            || (strcmp (ptr_command, "refresh") == 0)))
    {
        if (otr_command_peer_args (buffer, args + 1, num_args - 1, &peer_ref))
            rc = otr_command_start (buffer, &peer_ref);
    }
    else if (((num_args == 1) || (num_args == 3))
             && ((strcmp (ptr_command, "finish") == 0)
                 || (strcmp (ptr_command, "end") == 0)))
    {
        if (otr_command_peer_args (buffer, args + 1, num_args - 1, &peer_ref))
            rc = otr_command_finish (&peer_ref);
    }
    else if (((num_args == 1) || (num_args == 3))
             && (strcmp (ptr_command, "status") == 0))
    {
        if (otr_command_peer_args (buffer, args + 1, num_args - 1, &peer_ref))
            rc = otr_command_status (&peer_ref);
    }
    else if ((num_args >= 2) && (num_args <= 6)
             && (strcmp (ptr_command, "smp") == 0))
    {
        rc = otr_command_smp (buffer, args, num_args);
    }
    else if (((num_args == 1) || (num_args == 3))
             && ((strcmp (ptr_command, "trust") == 0)
                 || (strcmp (ptr_command, "distrust") == 0)))
    {
        if (otr_command_peer_args (buffer, args + 1, num_args - 1, &peer_ref))
        {
            rc = otr_command_trust (
                &peer_ref, (strcmp (ptr_command, "trust") == 0) ? 1 : 0);
        }
    }
    else if (((num_args == 1) || (num_args == 2))
             && (strcmp (ptr_command, "log") == 0))
    {
        rc = otr_command_log (buffer, args, num_args);
    }
    else if ((num_args >= 1) && (num_args <= 4)
             && (strcmp (ptr_command, "policy") == 0))
    {
        rc = otr_command_policy (buffer, args, num_args);
    }
    else if (((num_args == 1) || (num_args == 2))
             && (strcmp (ptr_command, "fingerprint") == 0))
    {
        if (num_args == 1)
            otr_command_show_account_fingerprints ();
        else if (strcmp (args[1], "all") == 0)
            otr_command_show_peer_fingerprints (NULL);
        else
            otr_command_show_peer_fingerprints (args[1]);
        rc = WEECHAT_RC_OK;
    }

    if (rc == WEECHAT_RC_ERROR)
    {
        otr_buffer_error_core (_("invalid arguments or not in a private "
                                 "buffer, see /help otr"));
    }

    for (i = 0; i < num_args; i++)
    {
        free (args[i]);
    }

    return rc;
}

/*
 * Hook OTR command.
 */

void
otr_command_init (void)
{
    weechat_hook_command (
        "otr",
        N_("Off-the-Record (OTR) encryption for private IRC conversations"),
        N_("start|refresh [<nick> <server>]"
            " || finish|end [<nick> <server>]"
            " || status [<nick> <server>]"
            " || trust|distrust [<nick> <server>]"
            " || smp ask [<nick> <server>] [<question>] <secret>"
            " || smp respond [<nick> <server>] <secret>"
            " || smp abort [<nick> <server>]"
            " || log [start|stop]"
            " || policy [default] [<name> on|off|toggle]"
            " || fingerprint [<search>|all]"),
        N_("      start: start an OTR session with a peer (sends an OTR query)\n"
            "    refresh: refresh the OTR session (same as start)\n"
            "     finish: end the OTR session\n"
            "        end: same as finish\n"
            "     status: show whether the conversation is encrypted and authenticated, with both fingerprints\n"
            "      trust: mark the current fingerprint of the peer as verified\n"
            "   distrust: remove the verified mark from the current fingerprint of the peer\n"
            "        smp: authenticate the peer with the Socialist Millionaire Protocol (SMP):\n"
            "                 ask: send a shared secret, optionally with a question\n"
            "             respond: answer the request of the peer\n"
            "               abort: abort the running verification\n"
            "        log: show or change logging of the current OTR session; the previous setting is restored when the session ends\n"
            "     policy: show or change OTR policies, for the peer of the current private buffer or, with \"default\", for all peers\n"
            "fingerprint: show your own fingerprints (without argument), those of all known peers (all) or of the peers matching <search>\n"
            "\n"
            "    nick: nick of the peer (default: peer of the current private buffer)\n"
            "  server: internal name of the IRC server (default: server of the current private buffer)\n"
            "question: question that the peer has to answer\n"
            "  secret: secret that both you and the peer know\n"
            "    name: name of the policy\n"
            "  search: only show peers whose nick contains this text\n"
            "\n"
            "Policies:\n"
            "            allow_v2: allow OTR protocol version 2\n"
            "            allow_v3: allow OTR protocol version 3\n"
            "  require_encryption: refuse to send unencrypted messages when OTR is enabled\n"
            "                 log: enable logging of OTR conversations\n"
            "            send_tag: advertise your OTR capability using the whitespace tag\n"
            "         html_escape: escape HTML special characters in outbound messages\n"
            "         html_filter: filter HTML in incoming messages\n"
            "\n"
            "Note: OTR works for private conversations only, not for channels.\n"
            "Note: to show the OTR state in the status bar, add \"[otr]\" to the option weechat.bar.status.items; use /set otr.* to customize the item and the colors.\n"
            "Note: policies are stored in the options otr.policy.*; a more specific option (peer) overrides a more general one (server, default).\n"
            "\n"
            "Examples:\n"
            "  start an OTR session in the current private buffer:\n"
            "    /otr start\n"
            "  verify the peer with a shared secret:\n"
            "    /otr smp ask \"swordfish\"\n"
            "  verify the peer with a question and a shared secret:\n"
            "    /otr smp ask \"Where did we meet?\" \"Berlin\"\n"
            "  answer an SMP request of the peer:\n"
            "    /otr smp respond \"Berlin\"\n"
            "  mark the fingerprint as verified after comparing it over a secure channel:\n"
            "    /otr trust\n"
            "  show the policies of the current peer and enable the whitespace tag for it:\n"
            "    /otr policy\n"
            "    /otr policy send_tag on\n"
            "  enable the whitespace tag for all peers:\n"
            "    /otr policy default send_tag on\n"
            "  show the fingerprints of all known peers:\n"
            "    /otr fingerprint all\n"
            "  end the OTR session:\n"
            "    /otr finish"),
        "start|refresh|finish|end|status|trust|distrust %(nick) %(irc_servers) %-"
        " || smp ask|respond|abort %(nick) %(irc_servers) %-"
        " || log start|stop %-"
        " || policy default %(otr_policy) on|off|toggle %-"
        " || policy %(otr_policy) on|off|toggle %-"
        " || fingerprint all %-",
        &otr_command_otr, NULL, NULL);
}
