/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Generation of OTR private keys in background */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-buffer.h"
#include "otr-config.h"
#include "otr-context.h"
#include "otr-irc.h"
#include "otr-key.h"
#include "otr-policy.h"
#include "otr-util.h"


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
 * Remove a peer waiting for a private key: no OTR query will be sent to this
 * peer when the key is generated.
 *
 * Return 1 if the peer was waiting for the key, 0 otherwise.
 */

int
otr_key_generation_remove_peer (struct t_otr_key_generation *generation,
                                const char *peer)
{
    char **peers, **new_peers;
    int i, num_peers, found;

    if (!generation || !peer || !peer[0])
        return 0;

    peers = weechat_string_split (*(generation->peers), "\n", NULL, 0, 0,
                                  &num_peers);
    if (!peers)
        return 0;

    new_peers = weechat_string_dyn_alloc (64);
    if (!new_peers)
    {
        weechat_string_free_split (peers);
        return 0;
    }

    found = 0;
    for (i = 0; i < num_peers; i++)
    {
        if (strcmp (peers[i], peer) == 0)
        {
            found = 1;
            continue;
        }
        if ((*new_peers)[0])
            weechat_string_dyn_concat (new_peers, "\n", -1);
        weechat_string_dyn_concat (new_peers, peers[i], -1);
    }
    weechat_string_free_split (peers);

    if (found)
    {
        weechat_string_dyn_free (generation->peers, 1);
        generation->peers = new_peers;
    }
    else
    {
        weechat_string_dyn_free (new_peers, 1);
    }

    return found;
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
    otr_util_free_secret (weechat_string_dyn_free (generation->output, 0));
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
        otr_util_wipe (buffer, length);
        free (buffer);
    }

    gcry_sexp_release (sexp_account);

    return rc;
}

/*
 * Write the fingerprints of peers in the file of fingerprints: they are written
 * in a new temporary file (mode 0600), which then replaces the file of
 * fingerprints (if this file is a symbolic link, the target of the link is
 * replaced and the link is kept); an error is displayed if the file can not be
 * written.
 *
 * Return 1 if OK, 0 if error.
 */

int
otr_key_write_fingerprints (void)
{
    char *filename, *tmp_filename;
    FILE *file;
    gcry_error_t error;

    if (!otr_us || !otr_fprfile)
        return 0;

    error = 0;
    file = otr_util_file_replace_start (otr_fprfile, &filename, &tmp_filename);
    if (file)
    {
        error = otrl_privkey_write_fingerprints_FILEp (otr_us, file);
        if (otr_util_file_replace_end (file, filename, tmp_filename,
                                       (error) ? 0 : 1))
        {
            return 1;
        }
    }

    otr_buffer_error_core (
        _("unable to save OTR fingerprints in file \"%s\": %s"),
        otr_fprfile, (error) ? gcry_strerror (error) : strerror (errno));
    return 0;
}

/*
 * Display a message about the key generation of an account in core buffer
 * ("color" is a level, for example OTR_CONFIG_LEVEL_INFO).
 */

void
otr_key_print (int color, const char *format, const char *account)
{
    char *message;

    weechat_asprintf (&message, format, account);
    if (message)
    {
        otr_buffer_print_str (NULL, color, message);
        free (message);
    }
}

/*
 * Save a private key generated (S-expression "(private-key ...)" in text):
 * the file of private keys is written with the other known keys and the new
 * one, then all keys are read again by libotr.
 *
 * The keys are written in a new temporary file (mode 0600), which then
 * replaces the file of private keys; if this file is a symbolic link, the
 * target of the link is replaced and the link is kept.
 *
 * If error is not NULL and the key can not be saved, *error is set to the
 * reason of the failure (static string, must not be freed).
 *
 * Return 1 if OK, 0 if error.
 */

int
otr_key_save (const char *account, const char *protocol, const char *key,
              const char **error)
{
    gcry_sexp_t sexp_key, privkey;
    gcry_error_t gcry_error;
    OtrlPrivKey *ptr_privkey;
    char *filename, *tmp_filename;
    FILE *file;
    int write_ok, rc;

    if (error)
        *error = _("invalid key");

    if (!otr_us || !account || !protocol || !key || !key[0])
        return 0;

    if (gcry_sexp_new (&sexp_key, key, strlen (key), 1))
        return 0;
    privkey = gcry_sexp_find_token (sexp_key, "private-key", 0);
    gcry_sexp_release (sexp_key);
    if (!privkey)
        return 0;

    rc = 0;

    /* The file contains private keys: new file readable only by the user. */
    file = otr_util_file_replace_start (otr_keyfile, &filename, &tmp_filename);
    if (file)
    {
        write_ok = (fprintf (file, "(privkeys\n") >= 0) ? 1 : 0;
        for (ptr_privkey = otr_us->privkey_root; write_ok && ptr_privkey;
             ptr_privkey = ptr_privkey->next)
        {
            /* The new key replaces the key of the same account. */
            if ((strcmp (ptr_privkey->accountname, account) == 0)
                && (strcmp (ptr_privkey->protocol, protocol) == 0))
            {
                continue;
            }
            write_ok = otr_key_write_account (file, ptr_privkey->accountname,
                                              ptr_privkey->protocol,
                                              ptr_privkey->privkey);
        }
        write_ok = (write_ok
                    && otr_key_write_account (file, account, protocol, privkey)
                    && (fprintf (file, ")\n") >= 0)) ? 1 : 0;
        if (otr_util_file_replace_end (file, filename, tmp_filename, write_ok))
        {
            gcry_error = otrl_privkey_read (otr_us, otr_keyfile);
            if (gcry_error == 0)
                rc = 1;
            else if (error)
                *error = gcry_strerror (gcry_error);
        }
        else if (error)
        {
            *error = strerror (errno);
        }
    }
    else if (error)
    {
        *error = strerror (errno);
    }

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
    {
        fprintf (stderr, "private key not found");
        return 1;
    }

    length = gcry_sexp_sprint (privkey, GCRYSEXP_FMT_ADVANCED, NULL, 0);
    buffer = malloc (length);
    if (!buffer)
    {
        fprintf (stderr, "not enough memory");
        gcry_sexp_release (privkey);
        return 1;
    }
    gcry_sexp_sprint (privkey, GCRYSEXP_FMT_ADVANCED, buffer, length);
    gcry_sexp_release (privkey);

    fprintf (stdout, "%s", buffer);
    fflush (stdout);
    otr_util_wipe (buffer, length);
    free (buffer);

    return 0;
}

/*
 * Send an OTR query to the peers waiting for the private key generated, so
 * that the OTR session (which could not start without the key) starts again.
 *
 * The state of key exchange started without the key is cleared before (see
 * function otr_context_clear_auth).
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
        otr_context_clear_auth (generation->account, peers[i]);
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
    const char *error;

    /* Make C compiler happy. */
    (void) data;
    (void) command;

    error = NULL;

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

    if (return_code != 0)
    {
        otr_buffer_error_core (
            _("generation of OTR key for %s failed: %s"),
            generation->account,
            ((*(generation->error))[0]) ?
            *(generation->error) : _("process failed or timeout reached"));
    }
    else if (!otr_key_save (generation->account, generation->protocol,
                            *(generation->output), &error))
    {
        otr_buffer_error_core (
            _("generation of OTR key for %s failed: unable to save key in "
              "file \"%s\": %s"),
            generation->account, otr_keyfile, error);
    }
    else
    {
        otr_key_print (OTR_CONFIG_LEVEL_SUCCESS, _("OTR key generated for %s."),
                       generation->account);
        weechat_bar_item_update (OTR_BAR_ITEM);
        otr_key_send_queries (generation);
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
 *
 * Return 1 if the key is being generated, 0 if error.
 */

int
otr_key_generate (const char *account, const char *protocol, const char *peer)
{
    struct t_otr_key_generation *new_generation;
    const char *error;

    if (!account || !protocol)
        return 0;

    error = _("not enough memory");

    /* Key already being generated: just remember the peer. */
    new_generation = otr_key_generation_search (account, protocol);
    if (new_generation)
    {
        otr_key_generation_add_peer (new_generation, peer);
        return 1;
    }

    new_generation = calloc (1, sizeof (*new_generation));
    if (!new_generation)
        goto error;
    new_generation->account = strdup (account);
    new_generation->protocol = strdup (protocol);
    /* Big enough for the key: no realloc leaving copies of the key. */
    new_generation->output = weechat_string_dyn_alloc (4096);
    new_generation->error = weechat_string_dyn_alloc (64);
    new_generation->peers = weechat_string_dyn_alloc (64);
    if (!new_generation->account || !new_generation->protocol
        || !new_generation->output || !new_generation->error
        || !new_generation->peers)
    {
        otr_key_generation_free (new_generation);
        goto error;
    }
    otr_key_generation_add_peer (new_generation, peer);

    new_generation->prev_generation = last_otr_key_generation;
    if (last_otr_key_generation)
        last_otr_key_generation->next_generation = new_generation;
    else
        otr_key_generations = new_generation;
    last_otr_key_generation = new_generation;

    new_generation->hook = weechat_hook_process ("func:otr_key_generate",
                                                 OTR_KEY_GENERATION_TIMEOUT,
                                                 &otr_key_process_cb,
                                                 new_generation, NULL);
    if (!new_generation->hook)
    {
        error = _("unable to run process");
        otr_key_generation_free (new_generation);
        goto error;
    }

    otr_key_print (OTR_CONFIG_LEVEL_INFO,
                   _("Generating OTR key for %s in background..."), account);

    return 1;

error:
    otr_buffer_error_core (_("generation of OTR key for %s failed: %s"),
                           account, error);
    return 0;
}
