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
#include <strings.h>

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


/* shlex-like splitting; returns count or -1 on error. Caller frees tokens. */
int
otr_command_split_args (const char *s, char **out, int max)
{
    const char *p = s;
    int n = 0, i;

    while (*p)
    {
        char *tok, *w, q = 0;

        while (isspace ((unsigned char)*p))
            p++;
        if (!*p)
            break;
        if (n >= max)
            goto fail;
        tok = malloc (strlen (p) + 1);
        if (!tok)
            goto fail;
        w = tok;
        while (*p && (q || !isspace ((unsigned char)*p)))
        {
            if (q == '\'')
            {
                if (*p == '\'')
                    q = 0;
                else
                    *w++ = *p;
                p++;
            }
            else if (q == '"')
            {
                if (*p == '"')
                {
                    q = 0;
                    p++;
                }
                else if (*p == '\\' && (p[1] == '"' || p[1] == '\\'))
                {
                    *w++ = p[1];
                    p += 2;
                }
                else
                    *w++ = *p++;
            }
            else if (*p == '\'' || *p == '"')
            {
                q = *p;
                p++;
            }
            else if (*p == '\\' && p[1])
            {
                *w++ = p[1];
                p += 2;
            }
            else
                *w++ = *p++;
        }
        if (q)
        {
            free (tok);
            goto fail;
        }
        *w = '\0';
        out[n++] = tok;
    }
    return n;
fail:
    for (i = 0; i < n; i++)
        free (out[i]);
    return -1;
}

/* [NICK SERVER] arguments or, with none, the current private buffer */
int
otr_command_peer_args (struct t_gui_buffer *buf, char **args, int nargs,
                       struct t_otr_peer_ref *p)
{
    const char *n, *s;

    if (nargs == 2)
    {
        snprintf (p->nick, sizeof (p->nick), "%s", args[0]);
        snprintf (p->server, sizeof (p->server), "%s", args[1]);
        return 1;
    }
    if (nargs == 0 && otr_buffer_is_private (buf))
    {
        n = weechat_buffer_get_string (buf, "localvar_channel");
        s = weechat_buffer_get_string (buf, "localvar_server");
        if (n && s)
        {
            snprintf (p->nick, sizeof (p->nick), "%s", n);
            snprintf (p->server, sizeof (p->server), "%s", s);
            return 1;
        }
    }
    return 0;
}

/* account + lowercased peer for a t_otr_peer_ref; prints an error on failure */
int
otr_command_resolve (const struct t_otr_peer_ref *p, char **account,
                     char **peer)
{
    *account = otr_irc_make_account (p->server);
    *peer = otr_util_lc_dup (p->nick);
    if (!*account || !*peer)
    {
        otr_buffer_error_core (_("not connected to server \"%s\""), p->server);
        free (*account);
        free (*peer);
        *account = *peer = NULL;
        return 0;
    }
    return 1;
}

struct t_otr_table
{
    char *(*rows)[4];
    int n, cap;
    size_t w[4];
};

void
otr_command_table_add (struct t_otr_table *t, const char *a, const char *b,
                       const char *c,
                       const char *d)
{
    const char *v[4];
    int i;
    void *nr;

    v[0] = a; v[1] = b; v[2] = c; v[3] = d;
    if (t->n == t->cap)
    {
        t->cap = t->cap ? t->cap * 2 : 8;
        nr = realloc (t->rows, (size_t)t->cap * sizeof (t->rows[0]));
        if (!nr)
            return;
        t->rows = nr;
    }
    for (i = 0; i < 4; i++)
    {
        t->rows[t->n][i] = otr_util_dupstr (v[i]);
        if (t->rows[t->n][i] && strlen (t->rows[t->n][i]) > t->w[i])
            t->w[i] = strlen (t->rows[t->n][i]);
    }
    t->n++;
}

void
otr_command_table_print_free (struct t_otr_table *t, int cols)
{
    int r, c;
    struct t_otr_sbuf b;
    char *line;

    for (r = 0; r < t->n; r++)
    {
        otr_util_sb_init (&b);
        for (c = 0; c < cols; c++)
        {
            size_t l = strlen (t->rows[r][c]);

            if (c)
                otr_util_sb_adds (&b, " |");
            otr_util_sb_adds (&b, t->rows[r][c]);
            while (l++ < t->w[c])
                otr_util_sb_addc (&b, ' ');
        }
        line = otr_util_sb_take (&b);
        if (line)
            otr_buffer_print_str (NULL, OTR_CONFIG_LEVEL_INFO, line);
        free (line);
        for (c = 0; c < 4; c++)
            free (t->rows[r][c]);
    }
    free (t->rows);
}

void
otr_command_show_account_fingerprints (void)
{
    struct t_otr_table t = { NULL, 0, 0, { 0, 0, 0, 0 } };
    OtrlPrivKey *k;
    char human[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];

    for (k = otr_us->privkey_root; k; k = k->next)
    {
        if (!otrl_privkey_fingerprint (otr_us, human, k->accountname,
                                       k->protocol))
            continue;
        otr_command_table_add (&t, k->accountname, human, "", "");
    }
    otr_command_table_print_free (&t, 2);
}

void
otr_command_show_peer_fingerprints (const char *grep)
{
    struct t_otr_table t = { NULL, 0, 0, { 0, 0, 0, 0 } };
    ConnContext *c;
    Fingerprint *f;
    char human[OTRL_PRIVKEY_FPRINT_HUMAN_LEN], peerfull[256];
    const char *trust;

    for (c = otr_us->context_root; c; c = c->next)
    {
        if (c->their_instance != OTRL_INSTAG_MASTER)
            continue;
        snprintf (peerfull, sizeof (peerfull), "%s", c->username);
        if (grep && !strstr (peerfull, grep))
            continue;
        for (f = c->fingerprint_root.next; f; f = f->next)
        {
            otrl_privkey_hash_to_human (human, f->fingerprint);
            if (!f->trust || !*f->trust)
                trust = _("unverified");
            else if (strcmp (f->trust, "smp") == 0)
                trust = _("SMP verified");
            else if (strcmp (f->trust, "verified") == 0)
                trust = _("verified");
            else
                trust = f->trust;
            otr_command_table_add (&t, peerfull, c->accountname, human, trust);
        }
    }
    otr_command_table_print_free (&t, 4);
}

int
otr_command_start (struct t_gui_buffer *buf, const struct t_otr_peer_ref *p)
{
    char *account, *peer;
    int v2, v3;
    const char *q;

    (void)buf;
    if (!otr_command_resolve (p, &account, &peer))
        return WEECHAT_RC_ERROR;
    v2 = otr_policy_for (account, peer, "allow_v2");
    v3 = otr_policy_for (account, peer, "allow_v3");
    if (!v2 && !v3)
    {
        otr_buffer_peer_print (p->server, peer, OTR_CONFIG_LEVEL_ERROR,
                               _("OTR is disabled by policy (allow_v2 and "
                               "allow_v3 are both off)."));
        free (account);
        free (peer);
        return WEECHAT_RC_OK;
    }
    q = (v2 && v3) ? "?OTRv23?" : (v3 ? "?OTRv3?" : "?OTRv2?");
    otr_buffer_peer_hint (
        p->server, peer,
        _("Sending OTR query... Please await confirmation of the OTR session "
        "being started before sending a message."));
    if (!otr_policy_for (account, peer, "send_tag"))
    {
        char *m = otr_util_strf (
            _("To try OTR on all conversations with %s@%s: /otr policy "
            "send_tag on"),
            peer, p->server);

        if (m)
            otr_buffer_peer_hint (p->server, peer, m);
        free (m);
    }
    otr_irc_send_raw (p->server, peer, q);
    free (account);
    free (peer);
    return WEECHAT_RC_OK;
}

int
otr_command_finish (const struct t_otr_peer_ref *p)
{
    char *account, *peer;
    ConnContext *ctx;

    if (!otr_command_resolve (p, &account, &peer))
        return WEECHAT_RC_ERROR;
    ctx = otr_context_find (account, peer);
    if (ctx && ctx->msgstate != OTRL_MSGSTATE_PLAINTEXT)
        otrl_message_disconnect_all_instances (otr_us, &otr_callback_ops, NULL,
                                               account,
                                               OTR_PROTOCOL, peer);
    else
        otr_buffer_peer_print (
            p->server, peer, OTR_CONFIG_LEVEL_INFO,
            _("There is no private conversation to finish."));
    weechat_bar_item_update (OTR_BAR_ITEM);
    free (account);
    free (peer);
    return WEECHAT_RC_OK;
}

int
otr_command_status (const struct t_otr_peer_ref *p)
{
    char *account, *peer;
    ConnContext *ctx;
    char own[OTRL_PRIVKEY_FPRINT_HUMAN_LEN], theirs[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
    char *m;

    if (!otr_command_resolve (p, &account, &peer))
        return WEECHAT_RC_ERROR;
    ctx = otr_context_find (account, peer);
    if (otr_context_encrypted (ctx))
    {
        otr_buffer_peer_print (p->server, peer, OTR_CONFIG_LEVEL_SUCCESS,
                               _("This conversation is encrypted."));
        strcpy (own, "?");
        strcpy (theirs, "?");
        otrl_privkey_fingerprint (otr_us, own, account, OTR_PROTOCOL);
        if (ctx->active_fingerprint)
            otrl_privkey_hash_to_human (theirs,
                                        ctx->active_fingerprint->fingerprint);
        m = otr_util_strf (_("Your fingerprint is: %s"), own);
        if (m)
            otr_buffer_peer_print (p->server, peer, OTR_CONFIG_LEVEL_INFO, m);
        free (m);
        m = otr_util_strf (_("Your peer's fingerprint is: %s"), theirs);
        if (m)
            otr_buffer_peer_print (p->server, peer, OTR_CONFIG_LEVEL_INFO, m);
        free (m);
        if (otr_context_verified (ctx))
            otr_buffer_peer_print (p->server, peer, OTR_CONFIG_LEVEL_SUCCESS,
                                   _("The peer's identity has been verified."));
        else
            otr_buffer_peer_print (
                p->server, peer, OTR_CONFIG_LEVEL_WARNING,
                _("You have not verified the peer's identity yet."));
    }
    else
        otr_buffer_peer_print (
            p->server, peer, OTR_CONFIG_LEVEL_WARNING,
            _("This current conversation is not encrypted."));
    free (account);
    free (peer);
    return WEECHAT_RC_OK;
}

int
otr_command_trust (const struct t_otr_peer_ref *p, int trust)
{
    char *account, *peer;
    ConnContext *ctx;
    char *m;

    if (!otr_command_resolve (p, &account, &peer))
        return WEECHAT_RC_ERROR;
    ctx = otr_context_find (account, peer);
    if (otr_context_encrypted (ctx) && ctx->active_fingerprint)
    {
        otrl_context_set_trust (ctx->active_fingerprint,
                                trust ? "verified" : "");
        otrl_privkey_write_fingerprints (otr_us, otr_fprfile);
        m = otr_util_strf (trust ? _("%s@%s is now authenticated.")
                        : _("%s@%s is now de-authenticated."), peer, p->server);
        if (m)
            otr_buffer_peer_print (p->server, peer, OTR_CONFIG_LEVEL_INFO, m);
        free (m);
        weechat_bar_item_update (OTR_BAR_ITEM);
    }
    else
        otr_buffer_peer_print (p->server, peer, OTR_CONFIG_LEVEL_ERROR,
                               _("No active fingerprint. Start an OTR "
                               "conversation first: /otr start"));
    free (account);
    free (peer);
    return WEECHAT_RC_OK;
}

int
otr_command_smp (struct t_gui_buffer *buf, char **tok, int n)
{
    struct t_otr_peer_ref p;
    char *account = NULL, *peer = NULL;
    const char *secret = NULL, *question = NULL, *action = tok[1];
    ConnContext *ctx;
    int rc = WEECHAT_RC_ERROR;
    struct t_otr_peer_data *pd;

    if (strcmp (action, "respond") == 0)
    {
        if (n == 3 && otr_command_peer_args (buf, tok + 2, 0, &p))
            secret = tok[2];
        else if (n == 5 && otr_command_peer_args (buf, tok + 2, 2, &p))
            secret = tok[4];
        else
            return WEECHAT_RC_ERROR;
    }
    else if (strcmp (action, "ask") == 0)
    {
        if (n == 3 && otr_command_peer_args (buf, tok + 2, 0, &p))
            secret = tok[2];
        else if (n == 4 && otr_command_peer_args (buf, tok + 2, 0, &p))
        {
            question = tok[2];
            secret = tok[3];
        }
        else if (n == 5 && otr_command_peer_args (buf, tok + 2, 2, &p))
            secret = tok[4];
        else if (n == 6 && otr_command_peer_args (buf, tok + 2, 2, &p))
        {
            question = tok[4];
            secret = tok[5];
        }
        else
            return WEECHAT_RC_ERROR;
    }
    else if (strcmp (action, "abort") == 0)
    {
        if (n == 2 && otr_command_peer_args (buf, tok + 2, 0, &p))
            ;
        else if (n == 4 && otr_command_peer_args (buf, tok + 2, 2, &p))
            ;
        else
            return WEECHAT_RC_ERROR;
    }
    else
        return WEECHAT_RC_ERROR;

    if (!otr_command_resolve (&p, &account, &peer))
        return WEECHAT_RC_ERROR;
    ctx = otr_context_find (account, peer);
    if (!otr_context_encrypted (ctx))
    {
        char *m = otr_util_strf (
            _("There is currently no encrypted session with %s@%s."), peer,
            p.server);

        if (m)
            otr_buffer_peer_print (p.server, peer, OTR_CONFIG_LEVEL_ERROR, m);
        free (m);
        rc = WEECHAT_RC_OK;
        goto done;
    }
    pd = otr_context_peer_data_for (account, peer);

    if (strcmp (action, "respond") == 0)
        otrl_message_respond_smp (otr_us, &otr_callback_ops, NULL, ctx,
                                  (const unsigned char *)secret,
                                  strlen (secret));
    else if (strcmp (action, "ask") == 0)
    {
        if (question)
        {
            otrl_message_initiate_smp_q (otr_us, &otr_callback_ops, NULL, ctx,
                                         question,
                                         (const unsigned char *)secret,
                                         strlen (secret));
            otr_buffer_peer_print (p.server, peer, OTR_CONFIG_LEVEL_INFO,
                                   _("SMP question sent..."));
        }
        else
        {
            otrl_message_initiate_smp (otr_us, &otr_callback_ops, NULL, ctx,
                                       (const unsigned char *)secret,
                                       strlen (secret));
            otr_buffer_peer_print (p.server, peer, OTR_CONFIG_LEVEL_INFO,
                                   _("SMP challenge sent..."));
        }
        if (pd)
            pd->in_smp = 1;
    }
    else
    {
        otrl_message_abort_smp (otr_us, &otr_callback_ops, NULL, ctx);
        if (pd)
        {
            pd->in_smp = 0;
            pd->smp_question = 0;
        }
        otr_buffer_peer_print (
            p.server, peer, OTR_CONFIG_LEVEL_INFO, _("SMP aborted."));
    }
    rc = WEECHAT_RC_OK;
done:
    free (account);
    free (peer);
    return rc;
}

int
otr_command_log (struct t_gui_buffer *buf, char **tok, int n)
{
    struct t_otr_peer_ref p;
    char *account, *peer;
    ConnContext *ctx;
    struct t_otr_peer_data *pd;

    if (!otr_command_peer_args (buf, tok + 1, 0, &p))
    {
        otr_buffer_print_str (NULL, OTR_CONFIG_LEVEL_ERROR,
                              _("OTR LOG: Not in an OTR session"));
        return WEECHAT_RC_OK;
    }
    if (!otr_command_resolve (&p, &account, &peer))
        return WEECHAT_RC_ERROR;
    ctx = otr_context_find (account, peer);
    pd = otr_context_peer_data_for (account, peer);

    if (!otr_context_encrypted (ctx))
        otr_buffer_peer_print (
            p.server, peer, OTR_CONFIG_LEVEL_ERROR,
            _("OTR LOG: Not in an OTR session"));
    else if (n == 1)
    {
        if (otr_log_buffer_is_logged (buf))
            otr_buffer_peer_print (
                p.server, peer, OTR_CONFIG_LEVEL_WARNING,
                _("This conversation is currently being logged."));
        else
            otr_buffer_peer_print (
                p.server, peer, OTR_CONFIG_LEVEL_INFO,
                _("This conversation is currently NOT being logged."));
    }
    else if (strcmp (tok[1], "start") == 0 && !otr_log_buffer_is_logged (buf))
    {
        if (pd && pd->prev_log == OTR_LOG_UNTOUCHED)
            pd->prev_log = otr_log_buffer_log_level (buf);
        otr_buffer_peer_print (p.server, peer, OTR_CONFIG_LEVEL_WARNING,
                               _("From this point on, this conversation will "
                               "be logged. Please keep in mind that by doing "
                               "so you are potentially putting yourself and "
                               "your interlocutor at risk. You can disable "
                               "this by doing /otr log stop"));
        weechat_command (buf, "/mute logger set 9");
    }
    else if (strcmp (tok[1], "stop") == 0 && otr_log_buffer_is_logged (buf))
    {
        if (pd && pd->prev_log == OTR_LOG_UNTOUCHED)
            pd->prev_log = otr_log_buffer_log_level (buf);
        weechat_command (buf, "/mute logger set 0");
        otr_buffer_peer_print (p.server, peer, OTR_CONFIG_LEVEL_INFO,
                               _("From this point on, this conversation will "
                               "NOT be logged ANYMORE."));
    }
    weechat_bar_item_update (OTR_BAR_ITEM);
    free (account);
    free (peer);
    return WEECHAT_RC_OK;
}

int
otr_command_policy_known (const char *name)
{
    int i;

    for (i = 0; i < OTR_CONFIG_NUM_POLICIES; i++)
        if (strcasecmp (otr_config_policies[i].name, name) == 0)
            return 1;
    return 0;
}

int
otr_command_value_ok (const char *v)
{
    return strcasecmp (v, "on") == 0 || strcasecmp (v, "off") == 0
        || strcasecmp (v, "toggle") == 0;
}

void
otr_command_print_policies_here (struct t_gui_buffer *buf, const char *text)
{
    struct t_otr_peer_ref p;
    char *account, *peer;

    if (otr_command_peer_args (buf, NULL, 0, &p)
        && otr_command_resolve (&p, &account, &peer))
    {
        otr_buffer_peer_print (p.server, peer, OTR_CONFIG_LEVEL_INFO, text);
        free (account);
        free (peer);
    }
    else
        otr_buffer_print_str (NULL, OTR_CONFIG_LEVEL_INFO, text);
}

int
otr_command_policy (struct t_gui_buffer *buf, char **tok, int n)
{
    struct t_otr_peer_ref p;
    char *account, *peer, *text, *cmd;

    if (n == 1 || (n == 2 && strcasecmp (tok[1], "default") == 0))
    {
        if (n == 1 && otr_command_peer_args (buf, NULL, 0, &p)
            && otr_command_resolve (&p, &account, &peer))
        {
            text = otr_policy_format_peer (account, peer);
            if (text)
                otr_buffer_peer_print (
                    p.server, peer, OTR_CONFIG_LEVEL_INFO, text);
            free (text);
            free (account);
            free (peer);
        }
        else
        {
            text = otr_policy_format_default ();
            if (text)
                otr_command_print_policies_here (buf, text);
            free (text);
        }
        return WEECHAT_RC_OK;
    }
    if (n == 3 && otr_command_policy_known (tok[1])
        && otr_command_value_ok (tok[2]))
    {
        if (!otr_command_peer_args (buf, NULL, 0, &p)
            || !otr_command_resolve (&p, &account, &peer))
            return WEECHAT_RC_ERROR;
        {
            char nick[128];

            otr_irc_account_nick (account, nick, sizeof (nick));
            cmd = otr_util_strf ("/set otr.policy.%s.%s.%s.%s %s", p.server,
                                 nick,
                                 peer, tok[1], tok[2]);
        }
        if (cmd)
            weechat_command (NULL, cmd);
        free (cmd);
        text = otr_policy_format_peer (account, peer);
        if (text)
            otr_buffer_peer_print (p.server, peer, OTR_CONFIG_LEVEL_INFO, text);
        free (text);
        free (account);
        free (peer);
        return WEECHAT_RC_OK;
    }
    if (n == 4 && strcasecmp (tok[1], "default") == 0
        && otr_command_policy_known (tok[2])
        && otr_command_value_ok (tok[3]))
    {
        cmd = otr_util_strf ("/set otr.policy.default.%s %s", tok[2], tok[3]);
        if (cmd)
            weechat_command (NULL, cmd);
        free (cmd);
        text = otr_policy_format_default ();
        if (text)
            otr_command_print_policies_here (buf, text);
        free (text);
        return WEECHAT_RC_OK;
    }
    return WEECHAT_RC_ERROR;
}

int
otr_command_otr (const void *pointer, void *data, struct t_gui_buffer *buffer,
                 int argc, char **argv, char **argv_eol)
{
    char *tok[16];
    int n = 0, i, rc = WEECHAT_RC_ERROR;
    struct t_otr_peer_ref p;
    const char *c;

    (void)pointer; (void)data; (void)argv;

    if (argc > 1)
        n = otr_command_split_args (argv_eol[1], tok, 16);
    if (n <= 0)
    {
        otr_buffer_error_core (_("invalid arguments, see /help otr"));
        return WEECHAT_RC_ERROR;
    }
    c = tok[0];

    if ((n == 1 || n == 3)
        && (strcmp (c, "start") == 0 || strcmp (c, "refresh") == 0))
    {
        if (otr_command_peer_args (buffer, tok + 1, n - 1, &p))
            rc = otr_command_start (buffer, &p);
    }
    else if ((n == 1 || n == 3)
             && (strcmp (c, "finish") == 0 || strcmp (c, "end") == 0))
    {
        if (otr_command_peer_args (buffer, tok + 1, n - 1, &p))
            rc = otr_command_finish (&p);
    }
    else if ((n == 1 || n == 3) && strcmp (c, "status") == 0)
    {
        if (otr_command_peer_args (buffer, tok + 1, n - 1, &p))
            rc = otr_command_status (&p);
    }
    else if (n >= 2 && n <= 6 && strcmp (c, "smp") == 0)
        rc = otr_command_smp (buffer, tok, n);
    else if ((n == 1 || n == 3)
             && (strcmp (c, "trust") == 0 || strcmp (c, "distrust") == 0))
    {
        if (otr_command_peer_args (buffer, tok + 1, n - 1, &p))
            rc = otr_command_trust (&p, strcmp (c, "trust") == 0);
    }
    else if ((n == 1 || n == 2) && strcmp (c, "log") == 0)
        rc = otr_command_log (buffer, tok, n);
    else if (n >= 1 && n <= 4 && strcmp (c, "policy") == 0)
        rc = otr_command_policy (buffer, tok, n);
    else if ((n == 1 || n == 2) && strcmp (c, "fingerprint") == 0)
    {
        if (n == 1)
            otr_command_show_account_fingerprints ();
        else if (strcmp (tok[1], "all") == 0)
            otr_command_show_peer_fingerprints (NULL);
        else
            otr_command_show_peer_fingerprints (tok[1]);
        rc = WEECHAT_RC_OK;
    }

    if (rc == WEECHAT_RC_ERROR)
        otr_buffer_error_core (_("invalid arguments or not in a private "
                               "buffer, see /help otr"));
    for (i = 0; i < n; i++)
        free (tok[i]);
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
