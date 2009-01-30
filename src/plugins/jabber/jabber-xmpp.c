/*
 * Copyright (c) 2009 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* jabber-xmpp.c: XMPP protocol for Jabber plugin */


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <iksemel.h>

#include "../weechat-plugin.h"
#include "jabber.h"
#include "jabber-xmpp.h"
#include "jabber-buddy.h"
#include "jabber-buffer.h"
#include "jabber-config.h"
#include "jabber-debug.h"
#include "jabber-muc.h"
#include "jabber-server.h"


/*
 * jabber_xmpp_log_level_for_command: get log level for Jabber command
 */

int
jabber_xmpp_log_level_for_command (const char *command)
{
    if (!command || !command[0])
        return 0;
    
    if (strcmp (command, "chat_msg") == 0)
        return 1;
    
    return 3;
}

/*
 * jabber_xmpp_tags: build tags list with Jabber command and/or tags
 */

const char *
jabber_xmpp_tags (const char *command, const char *tags)
{
    static char string[512];
    int log_level;
    char str_log_level[32];
    
    log_level = 0;
    str_log_level[0] = '\0';
    
    if (command && command[0])
    {
        log_level = jabber_xmpp_log_level_for_command (command);
        if (log_level > 0)
        {
            snprintf (str_log_level, sizeof (str_log_level),
                      ",log%d", log_level);
        }
    }
    
    if (command && command[0] && tags && tags[0])
    {
        snprintf (string, sizeof (string),
                  "jabber_%s,%s%s", command, tags, str_log_level);
        return string;
    }
    
    if (command && command[0])
    {
        snprintf (string, sizeof (string),
                  "jabber_%s%s", command, str_log_level);
        return string;
    }
    
    if (tags && tags[0])
    {
        snprintf (string, sizeof (string), "%s", tags);
        return string;
    }
    
    return NULL;
}

/*
 * jabber_xmpp_recv_chat_message: receive a message
 */

int
jabber_xmpp_recv_chat_message (struct t_jabber_server *server,
                               iks *node)
{
    char *attrib_from, *from, *pos;
    char *body;
    struct t_jabber_muc *ptr_muc;
    
    attrib_from = iks_find_attrib (node, "from");
    if (!attrib_from || !attrib_from[0])
        return WEECHAT_RC_ERROR;
    
    body = iks_find_cdata (node, "body");
    if (!body)
        return WEECHAT_RC_ERROR;
    
    pos = strchr (attrib_from, '/');
    from = (pos) ?
        weechat_strndup (attrib_from, pos - attrib_from) : strdup (attrib_from);
    if (from)
    {
        ptr_muc = jabber_muc_search (server, from);
        if (!ptr_muc)
        {
            ptr_muc = jabber_muc_new (server,
                                      JABBER_MUC_TYPE_PRIVATE,
                                      from, 0);
            if (!ptr_muc)
            {
                weechat_printf (server->buffer,
                                _("%s%s: cannot create new "
                                  "private buffer \"%s\""),
                                jabber_buffer_get_server_prefix (server,
                                                                 "error"),
                                JABBER_PLUGIN_NAME, from);
                return WEECHAT_RC_ERROR;
            }
        }
        //jabber_muc_set_topic (ptr_channel, address);
        
        weechat_printf_tags (ptr_muc->buffer,
                             jabber_xmpp_tags ("chat_msg", "notify_private"),
                             "%s%s",
                             jabber_buddy_as_prefix (NULL,
                                                     from,
                                                     JABBER_COLOR_CHAT_NICK_OTHER),
                             body);
        
        weechat_hook_signal_send ("jabber_pv",
                                  WEECHAT_HOOK_SIGNAL_STRING,
                                  body);
        free (from);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_xmpp_send_chat_message: send a message to MUC or buddy
 */

void
jabber_xmpp_send_chat_message (struct t_jabber_server *server,
                               struct t_jabber_muc *muc,
                               const char *message)
{
    iks *msg;
    
    if (muc->type == JABBER_MUC_TYPE_PRIVATE)
    {
        msg = iks_make_msg (IKS_TYPE_CHAT, muc->name, message);
        if (msg)
        {
            iks_send (server->iks_parser, msg);
            iks_delete (msg);
        }
    }
    else
    {
        // TODO: send message to MUC
    }
}

/*
 * jabber_xmpp_iks_stream_hook: iksemel stream hook
 */

int
jabber_xmpp_iks_stream_hook (void *user_data, int type, iks *node)
{
    struct t_jabber_server *server;
    iks *x, *t;
    ikspak *pak;
    
    server = (struct t_jabber_server *)user_data;
    
    switch (type)
    {
        case IKS_NODE_START:
            if (JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_TLS)
                && !iks_is_secure (server->iks_parser))
            {
                iks_start_tls (server->iks_parser);
            }
            else
            {
                if (!JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_SASL))
                {
                    x = iks_make_auth (server->iks_id,
                                       JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_PASSWORD),
                                       iks_find_attrib (node, "id"));
                    iks_insert_attrib (x, "id", "auth");
                    iks_send (server->iks_parser, x);
                    iks_delete (x);
                }
            }
            break;
        case IKS_NODE_NORMAL:
            if (strcmp ("stream:features", iks_name (node)) == 0)
            {
                server->iks_features = iks_stream_features (node);
                if (JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_SASL))
                {
                    if (!JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_TLS)
                        || iks_is_secure (server->iks_parser))
                    {
                        if (server->iks_authorized)
                        {
                            if (server->iks_features & IKS_STREAM_BIND)
                            {
                                t = iks_make_resource_bind (server->iks_id);
                                iks_send (server->iks_parser, t);
                                iks_delete (t);
                            }
                            if (server->iks_features & IKS_STREAM_SESSION)
                            {
                                t = iks_make_session ();
                                iks_insert_attrib (t, "id", "auth");
                                iks_send (server->iks_parser, t);
                                iks_delete (t);
                            }
                        }
                        else
                        {
                            if (server->iks_features & IKS_STREAM_SASL_MD5)
                            {
                                iks_start_sasl (server->iks_parser,
                                                IKS_SASL_DIGEST_MD5,
                                                server->iks_id->user,
                                                server->iks_password);
                            }
                            else if (server->iks_features & IKS_STREAM_SASL_PLAIN)
                            {
                                iks_start_sasl (server->iks_parser,
                                                IKS_SASL_PLAIN,
                                                server->iks_id->user,
                                                server->iks_password);
                            }
                        }
                    }
                }
            }
            else if (strcmp ("failure", iks_name (node)) == 0)
            {
                weechat_printf (server->buffer,
                                _("%s%s: SASL authentication failed (check "
                                  "SASL option and password)"),
                                jabber_buffer_get_server_prefix (server,
                                                                 "error"),
                                JABBER_PLUGIN_NAME);
                jabber_server_disconnect (server, 0);
            }
            else if (strcmp ("success", iks_name (node)) == 0)
            {
                server->iks_authorized = 1;
                iks_send_header (server->iks_parser, server->iks_id->server);
            }
            else if (strcmp ("message", iks_name (node)) == 0)
            {
                jabber_xmpp_recv_chat_message (server, node);
            }
            else
            {
                pak = iks_packet (node);
                iks_filter_packet (server->iks_filter, pak);
            }
            break;
        case IKS_NODE_STOP:
            weechat_printf (server->buffer,
                            _("%s%s: server disconnected"),
                            jabber_buffer_get_server_prefix (server, "network"),
                            JABBER_PLUGIN_NAME);
            jabber_server_disconnect (server, 1);
            break;
        case IKS_NODE_ERROR:
            weechat_printf (server->buffer,
                                _("%s%s: stream error"),
                                jabber_buffer_get_server_prefix (server,
                                                                 "error"),
                                JABBER_PLUGIN_NAME);
            break;
    }
    
    if (node)
        iks_delete (node);
    
    return IKS_OK;
}

/*
 * jabber_xmpp_iks_log: log
 */

void
jabber_xmpp_iks_log (void *user_data, const char *data, size_t size,
                     int is_incoming)
{
    /* make C compiler happy */
    (void) size;
    
    jabber_debug_printf ((struct t_jabber_server *)user_data,
                         !is_incoming, 0,
                         data);
}

/*
 * jabber_xmpp_iks_result: iks result
 */

int
jabber_xmpp_iks_result (void *user_data, ikspak *pak)
{
    iks *x;
    struct t_jabber_server *server;

    /* make C compiler happy */
    (void) pak;
    
    server = (struct t_jabber_server *)user_data;
    
    weechat_printf (server->buffer,
                    _("%s%s: login ok"),
                    jabber_buffer_get_server_prefix (server, NULL),
                    JABBER_PLUGIN_NAME);
    
    x = iks_make_iq (IKS_TYPE_GET, IKS_NS_ROSTER);
    iks_insert_attrib (x, "id", "roster");
    iks_send (server->iks_parser, x);
    iks_delete (x);
    
    return IKS_FILTER_EAT;
}

/*
 * jabber_xmpp_iks_error: iks error
 */

int
jabber_xmpp_iks_error (void *user_data, ikspak *pak)
{
    struct t_jabber_server *server;
    
    /* make C compiler happy */
    (void) pak;
    
    server = (struct t_jabber_server *)user_data;
    
    weechat_printf (server->buffer,
                    _("%s%s: authentication failed (check SASL option and "
                      "password)"),
                    jabber_buffer_get_server_prefix (server, "error"),
                    JABBER_PLUGIN_NAME);
    
    jabber_server_disconnect (server, 0);
    
    return IKS_FILTER_EAT;
}

/*
 * jabber_xmpp_iks_roster: iks roster
 */

int
jabber_xmpp_iks_roster (void *user_data, ikspak *pak)
{
    struct t_jabber_server *server;
    char *jid, *id, *pos;
    iks *x;
    
    server = (struct t_jabber_server *)user_data;
    
    server->iks_roster = pak->x;
    
    x = iks_child(pak->query);
    while (x)
    {
        if (iks_strcmp (iks_name(x), "item") == 0)
        {
            jid = iks_find_attrib (x, "jid");
            if (jid)
            {
                pos = strchr (jid, '@');
                //id = (pos) ? weechat_strndup (jid, pos - jid) : strdup (jid);
                id = strdup (jid);
                if (id)
                {
                    jabber_buddy_new (server, NULL, id, 0, 0, 0, 0, 0, 0, 0, 0);
                    free (id);
                }
            }
        }
        x = iks_next (x);
    }
    if (x)
        iks_delete (x);
    
    return IKS_FILTER_EAT;
}
