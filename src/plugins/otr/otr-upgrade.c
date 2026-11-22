/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Save/restore OTR plugin data when upgrading WeeChat */

#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-buffer.h"
#include "otr-config.h"
#include "otr-context.h"
#include "otr-irc.h"
#include "otr-key.h"
#include "otr-log.h"
#include "otr-policy.h"
#include "otr-upgrade.h"


/*
 * 1 if the OTR sessions are kept when the plugin is unloaded (upgrade without
 * quit): they are not ended, an OTR query is sent to the peers after the
 * upgrade, to start a new session.
 */
int otr_upgrade_keep_sessions = 0;

/*
 * Peers restored after upgrade, not handled yet (lines "account peer query"):
 * if query is "1", an OTR query is sent to the peer (when the server is
 * connected), otherwise only the logger level saved before the session is
 * restored.
 */
char **otr_upgrade_queries = NULL;

/* Timer to handle the peers restored after upgrade. */
struct t_hook *otr_upgrade_timer = NULL;

/* Signal "irc_server_connected" to send OTR queries after reconnection. */
struct t_hook *otr_upgrade_hook_connected = NULL;


/*
 * Check if an OTR query is pending for a peer (restored after upgrade, server
 * not connected yet).
 *
 * Return 1 if a query is pending, 0 otherwise.
 */

int
otr_upgrade_query_pending (const char *account, const char *peer)
{
    char *line, **lines;
    int i, num_lines, pending;

    if (!otr_upgrade_queries || !account || !peer)
        return 0;

    weechat_asprintf (&line, "%s %s 1", account, peer);
    if (!line)
        return 0;

    pending = 0;
    lines = weechat_string_split (*otr_upgrade_queries, "\n", NULL, 0, 0,
                                  &num_lines);
    for (i = 0; lines && (i < num_lines); i++)
    {
        if (strcmp (lines[i], line) == 0)
        {
            pending = 1;
            break;
        }
    }
    weechat_string_free_split (lines);

    free (line);

    return pending;
}

/*
 * Check if the peer of a libotr master context has an encrypted OTR session
 * (in the master context or one of its instances), or an OTR query pending
 * after a previous upgrade.
 *
 * A session ended by the peer (state "finished") is not a session: no new
 * session is started with the peer after upgrade.
 *
 * Return 1 if the peer has an OTR session, 0 otherwise.
 */

int
otr_upgrade_peer_has_session (ConnContext *master)
{
    ConnContext *ptr_context;

    for (ptr_context = otr_us->context_root; ptr_context;
         ptr_context = ptr_context->next)
    {
        if ((ptr_context->m_context == master)
            && otr_context_encrypted (ptr_context))
        {
            return 1;
        }
    }

    return otr_upgrade_query_pending (master->accountname, master->username);
}

/*
 * Save peers (OTR session, logger level before the session, messages held) in
 * upgrade file.
 *
 * Return:
 *   1: OK
 *   0: error
 */

int
otr_upgrade_save_peers (struct t_upgrade_file *upgrade_file)
{
    struct t_infolist *infolist;
    struct t_infolist_item *item;
    struct t_otr_peer_data *ptr_peer_data;
    ConnContext *ptr_context;
    int session, rc;

    for (ptr_context = otr_us->context_root; ptr_context;
         ptr_context = ptr_context->next)
    {
        if (ptr_context->m_context != ptr_context)
            continue;
        session = otr_upgrade_peer_has_session (ptr_context);
        ptr_peer_data = (struct t_otr_peer_data *)ptr_context->app_data;
        if (!session
            && (!ptr_peer_data
                || ((ptr_peer_data->prev_log == OTR_LOG_UNTOUCHED)
                    && !ptr_peer_data->held_messages)))
        {
            continue;
        }
        infolist = weechat_infolist_new ();
        if (!infolist)
            return 0;
        item = weechat_infolist_new_item (infolist);
        if (!item
            || !weechat_infolist_new_var_string (item, "account",
                                                 ptr_context->accountname)
            || !weechat_infolist_new_var_string (item, "peer",
                                                 ptr_context->username)
            || !weechat_infolist_new_var_integer (item, "session", session)
            || !weechat_infolist_new_var_integer (
                item, "prev_log",
                (ptr_peer_data) ? ptr_peer_data->prev_log : OTR_LOG_UNTOUCHED)
            || !weechat_infolist_new_var_string (
                item, "log_option",
                (ptr_peer_data) ? ptr_peer_data->log_option : NULL)
            || !weechat_infolist_new_var_string (
                item, "held_messages",
                (ptr_peer_data && ptr_peer_data->held_messages) ?
                *(ptr_peer_data->held_messages) : NULL))
        {
            weechat_infolist_free (infolist);
            return 0;
        }
        rc = weechat_upgrade_write_object (upgrade_file,
                                           OTR_UPGRADE_TYPE_PEER, infolist);
        weechat_infolist_free (infolist);
        if (!rc)
            return 0;
    }

    return 1;
}

/*
 * Save upgrade file.
 *
 * If save_sessions is 0 (the sessions are ended before quitting), the file is
 * empty.
 *
 * Note: the private keys being generated are not saved: WeeChat can not be
 * upgraded while a process is running.
 *
 * Return:
 *   1: OK
 *   0: error
 */

int
otr_upgrade_save (int save_sessions)
{
    struct t_upgrade_file *upgrade_file;
    int rc;

    if (!otr_us)
        return 0;

    upgrade_file = weechat_upgrade_new (OTR_UPGRADE_FILENAME,
                                        NULL, NULL, NULL);
    if (!upgrade_file)
        return 0;

    rc = (!save_sessions || otr_upgrade_save_peers (upgrade_file)) ? 1 : 0;

    weechat_upgrade_close (upgrade_file);

    return rc;
}

/*
 * Callback for signal "upgrade".
 *
 * The data is saved in upgrade file; the OTR sessions are kept (not ended
 * when the plugin is unloaded) on upgrade without quit.
 */

int
otr_upgrade_signal_cb (const void *pointer, void *data,
                       const char *signal, const char *type_data,
                       void *signal_data)
{
    int quit, save;

    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    save = (signal_data && (strcmp (signal_data, "save") == 0)) ? 1 : 0;
    quit = (signal_data && (strcmp (signal_data, "quit") == 0)) ? 1 : 0;

    if (!otr_upgrade_save (!quit))
    {
        weechat_printf (NULL,
                        _("%s%s: failed to save upgrade data"),
                        weechat_prefix ("error"), OTR_PLUGIN_NAME);
        return WEECHAT_RC_ERROR;
    }

    /* Upgrade (not only save): keep sessions unless WeeChat quits. */
    if (!save)
        otr_upgrade_keep_sessions = (quit) ? 0 : 1;

    return WEECHAT_RC_OK;
}

/*
 * Add a peer restored after upgrade, to handle later: send an OTR query if
 * query is 1, otherwise restore the logger level saved before the session.
 */

void
otr_upgrade_add_query (const char *account, const char *peer, int query)
{
    if (!account || !peer)
        return;

    if (!otr_upgrade_queries)
    {
        otr_upgrade_queries = weechat_string_dyn_alloc (64);
        if (!otr_upgrade_queries)
            return;
    }
    else
    {
        weechat_string_dyn_concat (otr_upgrade_queries, "\n", -1);
    }
    weechat_string_dyn_concat (otr_upgrade_queries, account, -1);
    weechat_string_dyn_concat (otr_upgrade_queries, " ", -1);
    weechat_string_dyn_concat (otr_upgrade_queries, peer, -1);
    weechat_string_dyn_concat (otr_upgrade_queries, (query) ? " 1" : " 0", -1);
}

/*
 * Restore a peer read in upgrade file.
 */

void
otr_upgrade_restore_peer (struct t_infolist *infolist)
{
    struct t_otr_peer_data *ptr_peer_data;
    const char *account, *peer, *str;
    char **messages;
    int i, num_messages;

    account = weechat_infolist_string (infolist, "account");
    peer = weechat_infolist_string (infolist, "peer");
    if (!account || !account[0] || !peer || !peer[0])
        return;

    ptr_peer_data = otr_context_peer_data_for (account, peer);
    if (!ptr_peer_data)
        return;

    /* Logger level before the session, restored when the session ends. */
    ptr_peer_data->prev_log = weechat_infolist_integer (infolist, "prev_log");
    str = weechat_infolist_string (infolist, "log_option");
    free (ptr_peer_data->log_option);
    ptr_peer_data->log_option = (str && str[0]) ? strdup (str) : NULL;

    /* Messages held until the session is encrypted. */
    otr_context_clear_held_messages (ptr_peer_data);
    str = weechat_infolist_string (infolist, "held_messages");
    if (str && str[0])
    {
        messages = weechat_string_split (str, "\n", NULL, 0, 0,
                                         &num_messages);
        for (i = 0; messages && (i < num_messages); i++)
        {
            otr_context_hold_message (ptr_peer_data, messages[i]);
        }
        weechat_string_free_split (messages);
    }

    /*
     * A new session is started with the peer if there was a session (or
     * messages held), otherwise the logger level is restored.
     */
    otr_upgrade_add_query (
        account, peer,
        (weechat_infolist_integer (infolist, "session")
         || ptr_peer_data->held_messages) ? 1 : 0);
}

/*
 * Read callback for OTR upgrade file.
 */

int
otr_upgrade_read_cb (const void *pointer, void *data,
                     struct t_upgrade_file *upgrade_file,
                     int object_id,
                     struct t_infolist *infolist)
{
    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) upgrade_file;

    weechat_infolist_reset_item_cursor (infolist);
    while (weechat_infolist_next (infolist))
    {
        switch (object_id)
        {
            case OTR_UPGRADE_TYPE_PEER:
                otr_upgrade_restore_peer (infolist);
                break;
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Restore the logger level saved before the session for a peer restored after
 * upgrade, and drop the messages held (no session is started with the peer).
 *
 * If buffer_closing is 1, the private buffer with the peer is being closed (see
 * function otr_log_restore).
 */

void
otr_upgrade_restore_log (const char *account, const char *peer,
                         int buffer_closing)
{
    ConnContext *ptr_context;
    struct t_otr_peer_data *ptr_peer_data;

    ptr_context = otrl_context_find (otr_us, peer, account, OTR_PROTOCOL,
                                     OTRL_INSTAG_MASTER, 0, NULL, NULL, NULL);
    if (!ptr_context || otr_context_encrypted (otr_context_find (account, peer)))
        return;

    ptr_peer_data = (struct t_otr_peer_data *)ptr_context->app_data;
    otr_context_drop_held_messages (ptr_context, ptr_peer_data,
                                    (buffer_closing) ? 0 : 1);
    otr_log_restore (ptr_context, ptr_peer_data, buffer_closing);
}

/*
 * Cancel the OTR query pending after upgrade for a peer on a server (server
 * not connected yet): the query is not sent, the logger level saved before the
 * session is restored and the messages held are dropped.
 *
 * If buffer_closing is 1, the private buffer with the peer is being closed.
 *
 * Return 1 if a query was pending for the peer, 0 otherwise.
 */

int
otr_upgrade_cancel_query (const char *server, const char *peer,
                          int buffer_closing)
{
    char **queries, **items, *account;
    int i, num_queries, num_items, found;

    if (!otr_upgrade_queries || !server || !peer)
        return 0;

    queries = weechat_string_split (*otr_upgrade_queries, "\n", NULL, 0, 0,
                                    &num_queries);
    weechat_string_dyn_free (otr_upgrade_queries, 1);
    otr_upgrade_queries = NULL;

    found = 0;
    account = NULL;
    for (i = 0; queries && (i < num_queries); i++)
    {
        items = weechat_string_split (queries[i], " ", NULL, 0, 0,
                                      &num_items);
        if (items && (num_items == 3))
        {
            if (!found
                && (strcmp (otr_irc_account_server (items[0]), server) == 0)
                && (strcmp (items[1], peer) == 0))
            {
                found = 1;
                account = strdup (items[0]);
            }
            else
            {
                otr_upgrade_add_query (items[0], items[1],
                                       (strcmp (items[2], "1") == 0) ? 1 : 0);
            }
        }
        weechat_string_free_split (items);
    }
    weechat_string_free_split (queries);

    if (account)
    {
        otr_upgrade_restore_log (account, peer, buffer_closing);
        free (account);
    }

    if (!otr_upgrade_queries && otr_upgrade_hook_connected)
    {
        weechat_unhook (otr_upgrade_hook_connected);
        otr_upgrade_hook_connected = NULL;
    }

    return found;
}

/*
 * Move the per-peer data restored after upgrade (messages held, logger level
 * saved before the session) to another account (our nick changed during the
 * upgrade, for example a server with TLS reconnected with another nick).
 */

void
otr_upgrade_move_peer_data (const char *account, const char *new_account,
                            const char *peer)
{
    ConnContext *ptr_context;
    struct t_otr_peer_data *ptr_peer_data, *ptr_new_peer_data;

    ptr_context = otrl_context_find (otr_us, peer, account, OTR_PROTOCOL,
                                     OTRL_INSTAG_MASTER, 0, NULL, NULL, NULL);
    ptr_peer_data = (ptr_context) ?
        (struct t_otr_peer_data *)ptr_context->app_data : NULL;
    if (!ptr_peer_data)
        return;

    ptr_new_peer_data = otr_context_peer_data_for (new_account, peer);
    if (!ptr_new_peer_data || (ptr_new_peer_data == ptr_peer_data))
        return;

    otr_context_move_held_messages (ptr_peer_data, ptr_new_peer_data);
    if (ptr_new_peer_data->prev_log == OTR_LOG_UNTOUCHED)
    {
        ptr_new_peer_data->prev_log = ptr_peer_data->prev_log;
        free (ptr_new_peer_data->log_option);
        ptr_new_peer_data->log_option = ptr_peer_data->log_option;
        ptr_peer_data->log_option = NULL;
    }
    ptr_peer_data->prev_log = OTR_LOG_UNTOUCHED;
}

/*
 * Send the OTR query to a peer after upgrade, with our current nick on the
 * server (the per-peer data is moved to the new account if our nick changed).
 *
 * If there is no private key for the account, the key is generated and the
 * query is sent when the key is generated; if OTR is disabled by policy for
 * the peer, the logger level saved before the session is restored.
 */

void
otr_upgrade_send_query (const char *account, const char *peer)
{
    const char *ptr_server, *ptr_query;
    char *new_account;

    if (!otr_us)
        return;

    ptr_server = otr_irc_account_server (account);
    new_account = otr_irc_make_account (ptr_server);
    if (!new_account)
        return;

    if (strcmp (new_account, account) != 0)
        otr_upgrade_move_peer_data (account, new_account, peer);

    /* New session already started by the peer. */
    if (otr_context_encrypted (otr_context_find (new_account, peer)))
        goto end;

    ptr_query = otr_policy_query (new_account, peer);
    if (!ptr_query)
    {
        otr_upgrade_restore_log (new_account, peer, 0);
        goto end;
    }

    otr_buffer_peer_print (ptr_server, peer, OTR_CONFIG_LEVEL_INFO,
                           _("Starting a new OTR session after /upgrade..."));

    if (otrl_privkey_find (otr_us, new_account, OTR_PROTOCOL))
        otr_irc_send_raw (ptr_server, peer, ptr_query);
    else if (!otr_key_generate (new_account, OTR_PROTOCOL, peer))
        otr_upgrade_restore_log (new_account, peer, 0);

end:
    free (new_account);
}

/*
 * Handle the peers restored after upgrade: restore the logger level of peers
 * without session, and send the OTR queries to the peers on connected servers
 * (all servers if server is NULL).
 *
 * The queries for servers not connected yet (for example servers with TLS,
 * which are reconnected after upgrade) are kept, and sent when the server is
 * connected (signal "irc_server_connected").
 */

void
otr_upgrade_send_queries (const char *server)
{
    char **queries, **items;
    const char *ptr_server;
    int i, num_queries, num_items;

    if (!otr_upgrade_queries)
        return;

    queries = weechat_string_split (*otr_upgrade_queries, "\n", NULL, 0, 0,
                                    &num_queries);
    weechat_string_dyn_free (otr_upgrade_queries, 1);
    otr_upgrade_queries = NULL;

    for (i = 0; queries && (i < num_queries); i++)
    {
        items = weechat_string_split (queries[i], " ", NULL, 0, 0,
                                      &num_items);
        if (items && (num_items == 3))
        {
            ptr_server = otr_irc_account_server (items[0]);
            if (strcmp (items[2], "1") != 0)
            {
                otr_upgrade_restore_log (items[0], items[1], 0);
            }
            else if ((!server || (strcmp (ptr_server, server) == 0))
                     && otr_irc_server_connected (ptr_server))
            {
                otr_upgrade_send_query (items[0], items[1]);
            }
            else
            {
                otr_upgrade_add_query (items[0], items[1], 1);
            }
        }
        weechat_string_free_split (items);
    }
    weechat_string_free_split (queries);

    if (!otr_upgrade_queries && otr_upgrade_hook_connected)
    {
        weechat_unhook (otr_upgrade_hook_connected);
        otr_upgrade_hook_connected = NULL;
    }
}

/*
 * Callback of timer used to handle the peers restored after upgrade (when all
 * plugins are loaded and IRC servers restored).
 */

int
otr_upgrade_timer_cb (const void *pointer, void *data, int remaining_calls)
{
    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) remaining_calls;

    otr_upgrade_timer = NULL;

    otr_upgrade_send_queries (NULL);

    return WEECHAT_RC_OK;
}

/*
 * Callback for signal "irc_server_connected": send the OTR queries after
 * upgrade to the peers on this server.
 */

int
otr_upgrade_server_connected_cb (const void *pointer, void *data,
                                 const char *signal, const char *type_data,
                                 void *signal_data)
{
    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (signal_data)
        otr_upgrade_send_queries ((const char *)signal_data);

    return WEECHAT_RC_OK;
}

/*
 * Load upgrade file: restore peers, then schedule the OTR queries to start a
 * new session with the peers (sent when the server is connected, see function
 * otr_upgrade_send_queries).
 *
 * Return:
 *   1: OK
 *   0: error
 */

int
otr_upgrade_load (void)
{
    struct t_upgrade_file *upgrade_file;
    int rc;

    upgrade_file = weechat_upgrade_new (OTR_UPGRADE_FILENAME,
                                        &otr_upgrade_read_cb, NULL, NULL);
    if (!upgrade_file)
        return 0;

    rc = weechat_upgrade_read (upgrade_file);

    weechat_upgrade_close (upgrade_file);

    if (otr_upgrade_queries)
    {
        if (!otr_upgrade_timer)
        {
            otr_upgrade_timer = weechat_hook_timer (1, 0, 1,
                                                    &otr_upgrade_timer_cb,
                                                    NULL, NULL);
        }
        if (!otr_upgrade_hook_connected)
        {
            otr_upgrade_hook_connected = weechat_hook_signal (
                "irc_server_connected",
                &otr_upgrade_server_connected_cb, NULL, NULL);
        }
    }

    return rc;
}

/*
 * End upgrade: remove the timer, the signal and the OTR queries not sent.
 */

void
otr_upgrade_end (void)
{
    if (otr_upgrade_timer)
    {
        weechat_unhook (otr_upgrade_timer);
        otr_upgrade_timer = NULL;
    }
    if (otr_upgrade_hook_connected)
    {
        weechat_unhook (otr_upgrade_hook_connected);
        otr_upgrade_hook_connected = NULL;
    }
    weechat_string_dyn_free (otr_upgrade_queries, 1);
    otr_upgrade_queries = NULL;
}
