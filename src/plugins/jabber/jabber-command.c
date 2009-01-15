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

/* jabber-command.c: Jabber commands */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../weechat-plugin.h"
#include "jabber.h"
#include "jabber-command.h"
#include "jabber-buddy.h"
#include "jabber-buffer.h"
#include "jabber-config.h"
#include "jabber-input.h"
#include "jabber-muc.h"
#include "jabber-server.h"
#include "jabber-display.h"
#include "jabber-xmpp.h"


/*
 * jabber_command_quit_server: send QUIT to a server
 */

void
jabber_command_quit_server (struct t_jabber_server *server,
                            const char *arguments)
{
    if (!server)
        return;
    
    (void) arguments;
}

/*
 * jabber_command_jabber: test
 */

int
jabber_command_jabber (void *data, struct t_gui_buffer *buffer, int argc,
                       char **argv, char **argv_eol)
{
    int i, detailed_list, one_server_found;
    struct t_jabber_server *ptr_server2, *server_found, *new_server;
    char *server_name;
    
    JABBER_GET_SERVER_MUC(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;
    
    if ((argc == 1)
        || (weechat_strcasecmp (argv[1], "list") == 0)
        || (weechat_strcasecmp (argv[1], "listfull") == 0))
    {
        /* list servers */
        server_name = NULL;
        detailed_list = 0;
        for (i = 1; i < argc; i++)
        {
            if (weechat_strcasecmp (argv[i], "list") == 0)
                continue;
            if (weechat_strcasecmp (argv[i], "listfull") == 0)
            {
                detailed_list = 1;
                continue;
            }
            if (!server_name)
                server_name = argv[i];
        }
        if (!server_name)
        {
            if (jabber_servers)
            {
                weechat_printf (NULL, "");
                weechat_printf (NULL, _("All servers:"));
                for (ptr_server2 = jabber_servers; ptr_server2;
                     ptr_server2 = ptr_server2->next_server)
                {
                    jabber_display_server (ptr_server2, detailed_list);
                }
            }
            else
                weechat_printf (NULL, _("No server"));
        }
        else
        {
            one_server_found = 0;
            for (ptr_server2 = jabber_servers; ptr_server2;
                 ptr_server2 = ptr_server2->next_server)
            {
                if (weechat_strcasestr (ptr_server2->name, server_name))
                {
                    if (!one_server_found)
                    {
                        weechat_printf (NULL, "");
                        weechat_printf (NULL,
                                        _("Servers with \"%s\":"),
                                        server_name);
                    }
                    one_server_found = 1;
                    jabber_display_server (ptr_server2, detailed_list);
                }
            }
            if (!one_server_found)
                weechat_printf (NULL,
                                _("No server found with \"%s\""),
                                server_name);
        }
        
        return WEECHAT_RC_OK;
    }
    
    if (weechat_strcasecmp (argv[1], "add") == 0)
    {
        if (argc < 6)
        {
            JABBER_COMMAND_TOO_FEW_ARGUMENTS(NULL, "server add");
        }
        if (jabber_server_search (argv[2]))
        {
            weechat_printf (NULL,
                            _("%s%s: server \"%s\" already exists, "
                              "can't create it!"),
                            weechat_prefix ("error"), JABBER_PLUGIN_NAME,
                            argv[2]);
            return WEECHAT_RC_ERROR;
        }
        
        new_server = jabber_server_alloc (argv[2]);
        if (!new_server)
        {
            weechat_printf (NULL,
                            _("%s%s: unable to create server"),
                            weechat_prefix ("error"), JABBER_PLUGIN_NAME);
            return WEECHAT_RC_ERROR;
        }
        
        weechat_config_option_set (new_server->options[JABBER_SERVER_OPTION_USERNAME],
                                   argv[3], 1);
        weechat_config_option_set (new_server->options[JABBER_SERVER_OPTION_SERVER],
                                   argv[4], 1);
        weechat_config_option_set (new_server->options[JABBER_SERVER_OPTION_PASSWORD],
                                   argv[5], 1);
        
        /* parse arguments */
        for (i = 6; i < argc; i++)
        {
            if (argv[i][0] == '-')
            {
                if (weechat_strcasecmp (argv[i], "-auto") == 0)
                {
                    weechat_config_option_set (new_server->options[JABBER_SERVER_OPTION_AUTOCONNECT],
                                               "on", 1);
                }
                if (weechat_strcasecmp (argv[i], "-noauto") == 0)
                {
                    weechat_config_option_set (new_server->options[JABBER_SERVER_OPTION_AUTOCONNECT],
                                               "off", 1);
                }
                if (weechat_strcasecmp (argv[i], "-ipv6") == 0)
                {
                    weechat_config_option_set (new_server->options[JABBER_SERVER_OPTION_IPV6],
                                               "on", 1);
                }
                if (weechat_strcasecmp (argv[i], "-tls") == 0)
                {
                    weechat_config_option_set (new_server->options[JABBER_SERVER_OPTION_TLS],
                                               "on", 1);
                }
                if (weechat_strcasecmp (argv[i], "-sasl") == 0)
                {
                    weechat_config_option_set (new_server->options[JABBER_SERVER_OPTION_SASL],
                                               "on", 1);
                }
            }
        }
        
        weechat_printf (NULL,
                        _("%s: server %s%s%s created"),
                        JABBER_PLUGIN_NAME,
                        JABBER_COLOR_CHAT_SERVER,
                        new_server->name,
                        JABBER_COLOR_CHAT);
        
        if (JABBER_SERVER_OPTION_BOOLEAN(new_server, JABBER_SERVER_OPTION_AUTOCONNECT))
            jabber_server_connect (new_server);
        
        return WEECHAT_RC_OK;
    }
    
    if (weechat_strcasecmp (argv[1], "copy") == 0)
    {
        if (argc < 4)
        {
            JABBER_COMMAND_TOO_FEW_ARGUMENTS(NULL, "server copy");
        }
        
        /* look for server by name */
        server_found = jabber_server_search (argv[2]);
        if (!server_found)
        {
            weechat_printf (NULL,
                            _("%s%s: server \"%s\" not found for "
                              "\"%s\" command"),
                            weechat_prefix ("error"), JABBER_PLUGIN_NAME,
                            argv[2], "server copy");
            return WEECHAT_RC_ERROR;
        }
        
        /* check if target name already exists */
        if (jabber_server_search (argv[3]))
        {
            weechat_printf (NULL,
                            _("%s%s: server \"%s\" already exists for "
                              "\"%s\" command"),
                            weechat_prefix ("error"), JABBER_PLUGIN_NAME,
                            argv[3], "server copy");
            return WEECHAT_RC_ERROR;
        }
        
        /* copy server */
        new_server = jabber_server_copy (server_found, argv[3]);
        if (new_server)
        {
            weechat_printf (NULL,
                            _("%s: server %s%s%s has been copied to "
                              "%s%s"),
                            JABBER_PLUGIN_NAME,
                            JABBER_COLOR_CHAT_SERVER,
                            argv[2],
                            JABBER_COLOR_CHAT,
                            JABBER_COLOR_CHAT_SERVER,
                            argv[3]);
            return WEECHAT_RC_OK;
        }
        
        return WEECHAT_RC_ERROR;
    }

    if (weechat_strcasecmp (argv[1], "rename") == 0)
    {
        if (argc < 4)
        {
            JABBER_COMMAND_TOO_FEW_ARGUMENTS(NULL, "server rename");
        }
        
        /* look for server by name */
        server_found = jabber_server_search (argv[2]);
        if (!server_found)
        {
            weechat_printf (NULL,
                            _("%s%s: server \"%s\" not found for "
                              "\"%s\" command"),
                            weechat_prefix ("error"), JABBER_PLUGIN_NAME,
                            argv[2], "server rename");
            return WEECHAT_RC_ERROR;
        }
        
        /* check if target name already exists */
        if (jabber_server_search (argv[3]))
        {
            weechat_printf (NULL,
                            _("%s%s: server \"%s\" already exists for "
                              "\"%s\" command"),
                            weechat_prefix ("error"), JABBER_PLUGIN_NAME,
                            argv[3], "server rename");
            return WEECHAT_RC_ERROR;
        }
        
        /* rename server */
        if (jabber_server_rename (server_found, argv[3]))
        {
            weechat_printf (NULL,
                            _("%s: server %s%s%s has been renamed to "
                              "%s%s"),
                            JABBER_PLUGIN_NAME,
                            JABBER_COLOR_CHAT_SERVER,
                            argv[2],
                            JABBER_COLOR_CHAT,
                            JABBER_COLOR_CHAT_SERVER,
                            argv[3]);
            return WEECHAT_RC_OK;
        }
        
        return WEECHAT_RC_ERROR;
    }
    
    if (weechat_strcasecmp (argv[1], "keep") == 0)
    {
        if (argc < 3)
        {
            JABBER_COMMAND_TOO_FEW_ARGUMENTS(NULL, "server keep");
        }
        
        /* look for server by name */
        server_found = jabber_server_search (argv[2]);
        if (!server_found)
        {
            weechat_printf (NULL,
                            _("%s%s: server \"%s\" not found for "
                              "\"%s\" command"),
                            weechat_prefix ("error"), JABBER_PLUGIN_NAME,
                            argv[2], "server keep");
            return WEECHAT_RC_ERROR;
        }
        
        /* check that is it temporary server */
        if (!server_found->temp_server)
        {
            weechat_printf (NULL,
                            _("%s%s: server \"%s\" is not a temporary server"),
                            weechat_prefix ("error"), JABBER_PLUGIN_NAME,
                            argv[2], "server keep");
            return WEECHAT_RC_ERROR;
        }
        
        /* remove temporary flag on server */
        server_found->temp_server = 0;
        
        weechat_printf (NULL,
                        _("%s: server %s%s%s is not temporary any more"),
                        JABBER_PLUGIN_NAME,
                        JABBER_COLOR_CHAT_SERVER,
                        argv[2],
                        JABBER_COLOR_CHAT);
        
        return WEECHAT_RC_OK;
    }
    
    if (weechat_strcasecmp (argv[1], "del") == 0)
    {
        if (argc < 3)
        {
            JABBER_COMMAND_TOO_FEW_ARGUMENTS(NULL, "server del");
        }
        
        /* look for server by name */
        server_found = jabber_server_search (argv[2]);
        if (!server_found)
        {
            weechat_printf (NULL,
                            _("%s%s: server \"%s\" not found for "
                              "\"%s\" command"),
                            weechat_prefix ("error"), JABBER_PLUGIN_NAME,
                            argv[2], "server del");
            return WEECHAT_RC_ERROR;
        }
        if (server_found->is_connected)
        {
            weechat_printf (NULL,
                            _("%s%s: you can not delete server \"%s\" "
                              "because you are connected to. "
                              "Try \"/disconnect %s\" before."),
                            weechat_prefix ("error"), JABBER_PLUGIN_NAME,
                            argv[2], argv[2]);
            return WEECHAT_RC_ERROR;
        }
        
        server_name = strdup (server_found->name);
        jabber_server_free (server_found);
        weechat_printf (NULL,
                        _("%s: Server %s%s%s has been deleted"),
                        JABBER_PLUGIN_NAME,
                        JABBER_COLOR_CHAT_SERVER,
                        (server_name) ? server_name : "???",
                        JABBER_COLOR_CHAT);
        if (server_name)
            free (server_name);
        
        return WEECHAT_RC_OK;
    }
    
    if (weechat_strcasecmp (argv[1], "switch") == 0)
    {
        if (weechat_config_boolean (jabber_config_look_one_server_buffer))
        {
            if (jabber_current_server)
            {
                ptr_server2 = jabber_current_server->next_server;
                if (!ptr_server2)
                    ptr_server2 = jabber_servers;
                while (ptr_server2 != jabber_current_server)
                {
                    if (ptr_server2->buffer)
                    {
                        jabber_current_server = ptr_server2;
                        break;
                    }
                    ptr_server2 = ptr_server2->next_server;
                    if (!ptr_server2)
                        ptr_server2 = jabber_servers;
                }
            }
            else
            {
                for (ptr_server2 = jabber_servers; ptr_server2;
                     ptr_server2 = ptr_server2->next_server)
                {
                    if (ptr_server2->buffer)
                    {
                        jabber_current_server = ptr_server2;
                        break;
                    }
                }
            }
            jabber_server_set_current_server (jabber_current_server);
        }
        return WEECHAT_RC_OK;
    }
    
    weechat_printf (NULL,
                    _("%s%s: unknown option for \"%s\" command"),
                    weechat_prefix ("error"), JABBER_PLUGIN_NAME, "server");
    
    return WEECHAT_RC_ERROR;
}

/*
 * jabber_command_jchat: chat with a buddy
 */

int
jabber_command_jchat (void *data, struct t_gui_buffer *buffer, int argc,
                      char **argv, char **argv_eol)
{
    JABBER_GET_SERVER_MUC(buffer);
    if (!ptr_server || !ptr_server->is_connected || !ptr_server->iks_authorized)
        return WEECHAT_RC_ERROR;
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
    {
        /* create private window if not already opened */
        ptr_muc = jabber_muc_search (ptr_server, argv[1]);
        if (!ptr_muc)
        {
            ptr_muc = jabber_muc_new (ptr_server,
                                      JABBER_MUC_TYPE_PRIVATE,
                                      argv[1], 1);
            if (!ptr_muc)
            {
                weechat_printf (ptr_server->buffer,
                                _("%s%s: cannot create new private "
                                  "buffer \"%s\""),
                                jabber_buffer_get_server_prefix (ptr_server, "error"),
                                JABBER_PLUGIN_NAME, argv[1]);
                return WEECHAT_RC_ERROR;
            }
        }
        weechat_buffer_set (ptr_muc->buffer, "display", "1");
        
        /* display text if given */
        if (argv_eol[2])
        {
            jabber_xmpp_send_chat_message (ptr_server, ptr_muc, argv_eol[2]);
            jabber_input_user_message_display (ptr_muc->buffer, argv_eol[2]);
        }
    }
    else
    {
        JABBER_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "chat");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_command_jconnect_one_server: connect to one server
 *                                     return 0 if error, 1 if ok
 */

int
jabber_command_jconnect_one_server (struct t_jabber_server *server, int no_join)
{
    if (!server)
        return 0;
    
    if (server->is_connected)
    {
        weechat_printf (NULL,
                        _("%s%s: already connected to server "
                          "\"%s\"!"),
                        weechat_prefix ("error"), JABBER_PLUGIN_NAME,
                        server->name);
        return 0;
    }
    if (server->hook_connect)
    {
        weechat_printf (NULL,
                        _("%s%s: currently connecting to server "
                          "\"%s\"!"),
                        weechat_prefix ("error"), JABBER_PLUGIN_NAME,
                        server->name);
        return 0;
    }
    server->disable_autojoin = no_join;
    if (jabber_server_connect (server))
    {
        server->reconnect_start = 0;
        server->reconnect_join = (server->mucs) ? 1 : 0;
    }
    
    /* connect ok */
    return 1;
}

/*
 * jabber_command_jconnect: connect to server(s)
 */

int
jabber_command_jconnect (void *data, struct t_gui_buffer *buffer, int argc,
                         char **argv, char **argv_eol)
{
    int i, nb_connect, connect_ok, all_servers, no_join, port, ipv6, tls, sasl;
    char *name, *error;
    long number;
    
    JABBER_GET_SERVER(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    nb_connect = 0;
    connect_ok = 1;
    port = JABBER_SERVER_DEFAULT_PORT;
    ipv6 = 0;
    tls = 0;
    sasl = 0;
    
    all_servers = 0;
    no_join = 0;
    for (i = 1; i < argc; i++)
    {
        if (weechat_strcasecmp (argv[i], "-all") == 0)
            all_servers = 1;
        if (weechat_strcasecmp (argv[i], "-nojoin") == 0)
            no_join = 1;
        if (weechat_strcasecmp (argv[i], "-ipv6") == 0)
            ipv6 = 1;
        if (weechat_strcasecmp (argv[i], "-tls") == 0)
            tls = 1;
        if (weechat_strcasecmp (argv[i], "-sasl") == 0)
            sasl = 1;
        if (weechat_strcasecmp (argv[i], "-port") == 0)
        {
            if (i == (argc - 1))
            {
                weechat_printf (NULL,
                                _("%s%s: missing argument for \"%s\" "
                                  "option"),
                                weechat_prefix ("error"), JABBER_PLUGIN_NAME,
                                "-port");
                return WEECHAT_RC_ERROR;
            }
            error = NULL;
            number = strtol (argv[++i], &error, 10);
            if (error && !error[0])
                port = number;
        }
    }
    
    if (all_servers)
    {
        for (ptr_server = jabber_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            nb_connect++;
            if (!ptr_server->is_connected && (!ptr_server->hook_connect))
            {
                if (!jabber_command_jconnect_one_server (ptr_server, no_join))
                    connect_ok = 0;
            }
        }
    }
    else
    {
        for (i = 1; i < argc; i++)
        {
            if (argv[i][0] != '-')
            {
                nb_connect++;
                ptr_server = jabber_server_search (argv[i]);
                if (ptr_server)
                {
                    if (!jabber_command_jconnect_one_server (ptr_server, no_join))
                        connect_ok = 0;
                }
                else
                {
                    name = jabber_server_get_name_without_port (argv[i]);
                    ptr_server = jabber_server_alloc ((name) ? name : argv[i]);
                    if (name)
                        free (name);
                    if (ptr_server)
                    {
                        ptr_server->temp_server = 1;
                        weechat_config_option_set (ptr_server->options[JABBER_SERVER_OPTION_SERVER],
                                                   argv[i], 1);
                        weechat_printf (NULL,
                                        _("%s: server %s%s%s created (temporary server, NOT SAVED!)"),
                                        JABBER_PLUGIN_NAME,
                                        JABBER_COLOR_CHAT_SERVER,
                                        ptr_server->name,
                                        JABBER_COLOR_CHAT);
                        if (!jabber_command_jconnect_one_server (ptr_server, 0))
                            connect_ok = 0;
                    }
                    else
                    {
                        weechat_printf (NULL,
                                        _("%s%s: unable to create server "
                                          "\"%s\""),
                                        weechat_prefix ("error"),
                                        JABBER_PLUGIN_NAME, argv[i]);
                    }
                }
            }
            else
            {
                if (weechat_strcasecmp (argv[i], "-port") == 0)
                    i++;
            }
        }
    }
    
    if (nb_connect == 0)
        connect_ok = jabber_command_jconnect_one_server (ptr_server, no_join);
    
    if (!connect_ok)
        return WEECHAT_RC_ERROR;
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_command_jdisconnect_one_server: disconnect from a server
 *                                        return 0 if error, 1 if ok
 */

int
jabber_command_jdisconnect_one_server (struct t_jabber_server *server)
{
    if (!server)
        return 0;
    
    if ((!server->is_connected) && (!server->hook_connect)
        && (server->reconnect_start == 0))
    {
        weechat_printf (server->buffer,
                        _("%s%s: not connected to server \"%s\"!"),
                        jabber_buffer_get_server_prefix (server, "error"),
                        JABBER_PLUGIN_NAME, server->name);
        return 0;
    }
    if (server->reconnect_start > 0)
    {
        weechat_printf (server->buffer,
                        _("%s%s: auto-reconnection is cancelled"),
                        jabber_buffer_get_server_prefix (server, NULL),
                        JABBER_PLUGIN_NAME);
    }
    jabber_command_quit_server (server, NULL);
    jabber_server_disconnect (server, 0);
    
    /* disconnect ok */
    return 1;
}

/*
 * jabber_command_jdisconnect: disconnect from server(s)
 */

int
jabber_command_jdisconnect (void *data, struct t_gui_buffer *buffer, int argc,
                            char **argv, char **argv_eol)
{
    int i, disconnect_ok;
    
    JABBER_GET_SERVER(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    if (argc < 2)
        disconnect_ok = jabber_command_jdisconnect_one_server (ptr_server);
    else
    {
        disconnect_ok = 1;
        
        if (weechat_strcasecmp (argv[1], "-all") == 0)
        {
            for (ptr_server = jabber_servers; ptr_server;
                 ptr_server = ptr_server->next_server)
            {
                if ((ptr_server->is_connected) || (ptr_server->hook_connect)
                    || (ptr_server->reconnect_start != 0))
                {
                    if (!jabber_command_jdisconnect_one_server (ptr_server))
                        disconnect_ok = 0;
                }
            }
        }
        else
        {
            for (i = 1; i < argc; i++)
            {
                ptr_server = jabber_server_search (argv[i]);
                if (ptr_server)
                {
                    if (!jabber_command_jdisconnect_one_server (ptr_server))
                        disconnect_ok = 0;
                }
                else
                {
                    weechat_printf (NULL,
                                    _("%s%s: server \"%s\" not found"),
                                    weechat_prefix ("error"),
                                    JABBER_PLUGIN_NAME, argv[i]);
                    disconnect_ok = 0;
                }
            }
        }
    }
    
    if (!disconnect_ok)
        return WEECHAT_RC_ERROR;
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_command_init: init Jabber commands (create hooks)
 */

void
jabber_command_init ()
{
    weechat_hook_command ("jabber",
                          N_("list, add or remove Jabber servers"),
                          N_("[list [servername]] | [listfull [servername]] | "
                             "[add servername username hostname[/port] password "
                             "[-auto | -noauto] [-ipv6] [-tls] [-sasl]] | "
                             "[copy servername newservername] | "
                             "[rename servername newservername] | "
                             "[keep servername] | [del servername] | "
                             "[switch]"),
                          N_("      list: list servers (no parameter implies "
                             "this list)\n"
                             "  listfull: list servers with detailed info for "
                             "each server\n"
                             "       add: create a new server\n"
                             "servername: server name, for internal and "
                             "display use\n"
                             "  username: username to use on server\n"
                             "  hostname: name or IP address of server, with "
                             "optional port (default: 5222)\n"
                             "  password: password for username on server\n"
                             "      auto: automatically connect to server "
                             "when WeeChat starts\n"
                             "    noauto: do not connect to server when "
                             "WeeChat starts (default)\n"
                             "      ipv6: use IPv6 protocol\n"
                             "       tls: use TLS cryptographic protocol\n"
                             "      sasl: use SASL for authentication\n"
                             "      copy: duplicate a server\n"
                             "    rename: rename a server\n"
                             "      keep: keep server in config file (for "
                             "temporary servers only)\n"
                             "       del: delete a server\n"
                             "    switch: switch active server (when one "
                             "buffer is used for all servers, default key: "
                             "alt-s on server buffer)\n\n"
                             "Examples:\n"
                             "  /jabber listfull\n"
                             "  /jabber add jabberfr user jabber.fr/5222 "
                             "password -tls\n"
                             "  /jabber copy jabberfr jabberfr2\n"
                             "  /jabber rename jabberfr jabbfr\n"
                             "  /jabber del jabberfr\n"
                             "  /jabber switch"),
                          "add|copy|rename|keep|del|list|listfull|switch "
                          "%(jabber_servers) %(jabber_servers)",
                          &jabber_command_jabber, NULL);
    weechat_hook_command ("jchat",
                          N_("chat with a buddy"),
                          N_("buddy [text]"),
                          N_("buddy: buddy name for chat\n"
                             " text: text to send"),
                          "%n %-", &jabber_command_jchat, NULL);
    weechat_hook_command ("jconnect",
                          N_("connect to Jabber server(s)"),
                          N_("[-all [-nojoin] | servername [servername ...] "
                             "[-nojoin] | hostname [-port port] [-ipv6] "
                             "[-tls] [-sasl]]"),
                          N_("      -all: connect to all servers\n"
                             "servername: internal server name to connect\n"
                             "   -nojoin: do not join any MUC (even if "
                             "autojoin is enabled on server)\n"
                             "  hostname: hostname to connect\n"
                             "      port: port for server (integer, default "
                             "is 6667)\n"
                             "      ipv6: use IPv6 protocol\n"
                             "       tls: use TLS cryptographic protocol\n"
                             "      saal: use SASL for authentication"),
                          "%(jabber_servers)|-all|-nojoin|%*",
                          &jabber_command_jconnect, NULL);
    weechat_hook_command ("jdisconnect",
                          N_("disconnect from Jabber server(s)"),
                          N_("[-all | servername [servername ...]]"),
                          N_("      -all: disconnect from all servers\n"
                             "servername: server name to disconnect"),
                          "%(jabber_servers)|-all",
                          &jabber_command_jdisconnect, NULL);
}
