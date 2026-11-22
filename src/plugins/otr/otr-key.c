/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Generation of OTR private keys in background */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-buffer.h"
#include "otr-config.h"
#include "otr-context.h"
#include "otr-irc.h"
#include "otr-key.h"
#include "otr-policy.h"


struct t_otr_key_generation *otr_key_generations = NULL;
struct t_otr_key_generation *last_otr_key_generation = NULL;


/*
 * Search a private key being generated for an account and a protocol.
 *
 * Return pointer to key generation found, NULL if not found.
 */

struct t_otr_key_generation *
otr_key_generation_search (const char *account, const char *protocol)
{
    struct t_otr_key_generation *ptr_generation;

    if (!account || !protocol)
        return NULL;

    for (ptr_generation = otr_key_generations; ptr_generation;
         ptr_generation = ptr_generation->next_generation)
    {
        if ((strcmp (ptr_generation->account, account) == 0)
            && (strcmp (ptr_generation->protocol, protocol) == 0))
        {
            return ptr_generation;
        }
    }

    return NULL;
}

/*
 * Add a peer waiting for a private key (if not already in the list).
 */

void
otr_key_generation_add_peer (struct t_otr_key_generation *generation,
                             const char *peer)
{
    char **peers;
    int i, num_peers, found;

    if (!generation || !peer || !peer[0])
        return;

    found = 0;
    peers = weechat_string_split (*(generation->peers), "\n", NULL, 0, 0,
                                  &num_peers);
    for (i = 0; peers && (i < num_peers); i++)
    {
        if (strcmp (peers[i], peer) == 0)
        {
            found = 1;
            break;
        }
    }
    weechat_string_free_split (peers);
    if (found)
        return;

    if ((*(generation->peers))[0])
        weechat_string_dyn_concat (generation->peers, "\n", -1);
    weechat_string_dyn_concat (generation->peers, peer, -1);
}

/*
 * Free a private key generation and remove it from the list; the process
 * generating the key is killed if it is still running.
 */

void
otr_key_generation_free (struct t_otr_key_generation *generation)
{
    if (!generation)
        return;

    if (generation->prev_generation)
        (generation->prev_generation)->next_generation =
            generation->next_generation;
    if (generation->next_generation)
        (generation->next_generation)->prev_generation =
            generation->prev_generation;
    if (otr_key_generations == generation)
        otr_key_generations = generation->next_generation;
    if (last_otr_key_generation == generation)
        last_otr_key_generation = generation->prev_generation;

    if (generation->hook)
        weechat_unhook (generation->hook);
    free (generation->account);
    free (generation->protocol);
    weechat_string_dyn_free (generation->output, 1);
    weechat_string_dyn_free (generation->error, 1);
    weechat_string_dyn_free (generation->peers, 1);
    free (generation);
}

/*
 * Free all private key generations.
 */

void
otr_key_generation_free_all (void)
{
    while (otr_key_generations)
    {
        otr_key_generation_free (otr_key_generations);
    }
}

/*
 * Write an account with its private key in a file of private keys (same
 * format as libotr).
 *
 * Return 1 if OK, 0 if error.
 */

int
otr_key_write_account (FILE *file, const char *account, const char *protocol,
                       gcry_sexp_t privkey)
{
    gcry_sexp_t sexp_account;
    char *buffer;
    size_t length;
    int rc;

    if (gcry_sexp_build (&sexp_account, NULL,
                         "(account (name %s) (protocol %s) %S)",
                         account, protocol, privkey))
    {
        return 0;
    }

    rc = 0;
    length = gcry_sexp_sprint (sexp_account, GCRYSEXP_FMT_ADVANCED, NULL, 0);
    buffer = malloc (length);
    if (buffer)
    {
        gcry_sexp_sprint (sexp_account, GCRYSEXP_FMT_ADVANCED, buffer, length);
        rc = (fprintf (file, "%s\n", buffer) >= 0) ? 1 : 0;
        free (buffer);
    }

    gcry_sexp_release (sexp_account);

    return rc;
}

/*
 * Save a private key generated (S-expression "(private-key ...)" in text):
 * the file of private keys is written with the other known keys and the new
 * one, then all keys are read again by libotr.
 *
 * Return 1 if OK, 0 if error.
 */

int
otr_key_save (const char *account, const char *protocol, const char *key)
{
    gcry_sexp_t sexp_key, privkey;
    OtrlPrivKey *ptr_privkey;
    char *tmp_filename;
    FILE *file;
    int fd, rc;

    if (!otr_us || !account || !protocol || !key || !key[0])
        return 0;

    if (gcry_sexp_new (&sexp_key, key, strlen (key), 1))
        return 0;
    privkey = gcry_sexp_find_token (sexp_key, "private-key", 0);
    gcry_sexp_release (sexp_key);
    if (!privkey)
        return 0;

    rc = 0;
    tmp_filename = NULL;
    file = NULL;

    weechat_asprintf (&tmp_filename, "%s.tmp", otr_keyfile);
    if (!tmp_filename)
        goto end;

    /* The file contains private keys: readable only by the user. */
    fd = open (tmp_filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0)
        goto end;
    file = fdopen (fd, "w");
    if (!file)
    {
        close (fd);
        goto end;
    }

    if (fprintf (file, "(privkeys\n") < 0)
        goto end;
    for (ptr_privkey = otr_us->privkey_root; ptr_privkey;
         ptr_privkey = ptr_privkey->next)
    {
        /* The new key replaces the key of the same account. */
        if ((strcmp (ptr_privkey->accountname, account) == 0)
            && (strcmp (ptr_privkey->protocol, protocol) == 0))
        {
            continue;
        }
        if (!otr_key_write_account (file, ptr_privkey->accountname,
                                    ptr_privkey->protocol,
                                    ptr_privkey->privkey))
        {
            goto end;
        }
    }
    if (!otr_key_write_account (file, account, protocol, privkey))
        goto end;
    if (fprintf (file, ")\n") < 0)
        goto end;

    if (fclose (file) != 0)
    {
        file = NULL;
        goto end;
    }
    file = NULL;

    if (rename (tmp_filename, otr_keyfile) != 0)
        goto end;

    rc = (otrl_privkey_read (otr_us, otr_keyfile) == 0) ? 1 : 0;

end:
    if (file)
        fclose (file);
    if (!rc && tmp_filename)
        unlink (tmp_filename);
    free (tmp_filename);
    gcry_sexp_release (privkey);
    return rc;
}

/*
 * Generate a private key in the child process (same key as libotr: DSA 1024
 * bits): the key is written on standard output (S-expression in text), an
 * error on standard error.
 *
 * Return the return code of the child process: 0 if OK, 1 if error.
 */

int
otr_key_generate_child (void)
{
    gcry_sexp_t parms, key, privkey;
    gcry_error_t error;
    char *buffer;
    size_t length;
    const char *str_parms = "(genkey (dsa (nbits 4:1024)))";

    error = gcry_sexp_new (&parms, str_parms, strlen (str_parms), 0);
    if (!error)
    {
        error = gcry_pk_genkey (&key, parms);
        gcry_sexp_release (parms);
    }
    if (error)
    {
        fprintf (stderr, "%s", gcry_strerror (error));
        return 1;
    }

    privkey = gcry_sexp_find_token (key, "private-key", 0);
    gcry_sexp_release (key);
    if (!privkey)
        return 1;

    length = gcry_sexp_sprint (privkey, GCRYSEXP_FMT_ADVANCED, NULL, 0);
    buffer = malloc (length);
    if (!buffer)
    {
        gcry_sexp_release (privkey);
        return 1;
    }
    gcry_sexp_sprint (privkey, GCRYSEXP_FMT_ADVANCED, buffer, length);
    gcry_sexp_release (privkey);

    fprintf (stdout, "%s", buffer);
    fflush (stdout);
    free (buffer);

    return 0;
}

/*
 * Send an OTR query to the peers waiting for the private key generated, so
 * that the OTR session (which could not start without the key) starts again.
 */

void
otr_key_send_queries (struct t_otr_key_generation *generation)
{
    const char *ptr_server, *ptr_query;
    char **peers;
    int i, num_peers;

    ptr_server = otr_irc_account_server (generation->account);

    peers = weechat_string_split (*(generation->peers), "\n", NULL, 0, 0,
                                  &num_peers);
    for (i = 0; peers && (i < num_peers); i++)
    {
        if (otr_context_encrypted (otr_context_find (generation->account,
                                                     peers[i])))
        {
            continue;
        }
        ptr_query = otr_policy_query (generation->account, peers[i]);
        if (!ptr_query)
            continue;
        otr_buffer_peer_hint (
            ptr_server, peers[i],
            _("Sending OTR query... Please wait for the OTR session to start "
              "before sending a message."));
        otr_irc_send_raw (ptr_server, peers[i], ptr_query);
    }
    weechat_string_free_split (peers);
}

/*
 * Callback of process generating a private key.
 *
 * In the child process, the key is generated; in WeeChat, the output of the
 * child process (the key) is saved when the process has ended.
 */

int
otr_key_process_cb (const void *pointer, void *data, const char *command,
                    int return_code, const char *out, const char *err)
{
    struct t_otr_key_generation *generation;

    /* Make C compiler happy. */
    (void) data;
    (void) command;

    if (return_code == WEECHAT_HOOK_PROCESS_CHILD)
        return otr_key_generate_child ();

    generation = (struct t_otr_key_generation *)pointer;

    if (out)
        weechat_string_dyn_concat (generation->output, out, -1);
    if (err)
        weechat_string_dyn_concat (generation->error, err, -1);

    if (return_code == WEECHAT_HOOK_PROCESS_RUNNING)
        return WEECHAT_RC_OK;

    /* The process has ended: the hook is removed by WeeChat. */
    generation->hook = NULL;

    if ((return_code == 0)
        && otr_key_save (generation->account, generation->protocol,
                         *(generation->output)))
    {
        otr_buffer_print_str (NULL, OTR_CONFIG_LEVEL_SUCCESS,
                              _("OTR key generated."));
        otr_key_send_queries (generation);
    }
    else
    {
        otr_buffer_error_core (
            _("key generation failed: %s"),
            ((*(generation->error))[0]) ? *(generation->error) : "?");
    }

    otr_key_generation_free (generation);

    return WEECHAT_RC_OK;
}

/*
 * Start the generation of a private key for an account in background (child
 * process), so that WeeChat is not blocked.
 *
 * The peer (can be NULL) is waiting for the key: an OTR query is sent to the
 * peer when the key is generated.
 */

void
otr_key_generate (const char *account, const char *protocol, const char *peer)
{
    struct t_otr_key_generation *new_generation;

    if (!account || !protocol)
        return;

    /* Key already being generated: just remember the peer. */
    new_generation = otr_key_generation_search (account, protocol);
    if (new_generation)
    {
        otr_key_generation_add_peer (new_generation, peer);
        return;
    }

    new_generation = calloc (1, sizeof (*new_generation));
    if (!new_generation)
        return;
    new_generation->account = strdup (account);
    new_generation->protocol = strdup (protocol);
    new_generation->output = weechat_string_dyn_alloc (1024);
    new_generation->error = weechat_string_dyn_alloc (64);
    new_generation->peers = weechat_string_dyn_alloc (64);
    if (!new_generation->account || !new_generation->protocol
        || !new_generation->output || !new_generation->error
        || !new_generation->peers)
    {
        otr_key_generation_free (new_generation);
        return;
    }
    otr_key_generation_add_peer (new_generation, peer);

    new_generation->prev_generation = last_otr_key_generation;
    if (last_otr_key_generation)
        last_otr_key_generation->next_generation = new_generation;
    else
        otr_key_generations = new_generation;
    last_otr_key_generation = new_generation;

    otr_buffer_print_str (
        NULL, OTR_CONFIG_LEVEL_INFO,
        _("Generating a new OTR key for this account in background..."));

    new_generation->hook = weechat_hook_process ("func:otr_key_generate", 0,
                                                 &otr_key_process_cb,
                                                 new_generation, NULL);
    if (!new_generation->hook)
    {
        otr_buffer_error_core (_("key generation failed: %s"), "?");
        otr_key_generation_free (new_generation);
    }
}
