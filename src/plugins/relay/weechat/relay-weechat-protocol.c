/*
 * relay-weechat-protocol.c - WeeChat protocol for relay to client
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../../weechat-plugin.h"
#include "../relay.h"
#include "relay-weechat.h"
#include "relay-weechat-protocol.h"
#include "relay-weechat-msg.h"
#include "relay-weechat-nicklist.h"
#include "../relay-buffer.h"
#include "../relay-client.h"
#include "../relay-auth.h"
#include "../relay-config.h"
#include "../relay-raw.h"


/*
 * Gets buffer pointer with argument from a command.
 *
 * The argument "arg" can be a pointer ("0x12345678") or a full name
 * ("irc.libera.#weechat").
 *
 * Returns pointer to buffer found, NULL if not found.
 */

struct t_gui_buffer *
relay_weechat_protocol_get_buffer (const char *arg)
{
    struct t_gui_buffer *ptr_buffer;
    int rc;

    ptr_buffer = NULL;

    if (strncmp (arg, "0x", 2) == 0)
    {
        rc = sscanf (arg, "%p", &ptr_buffer);
        if ((rc != EOF) && (rc != 0) && ptr_buffer)
        {
            if (!weechat_hdata_check_pointer (
                    relay_hdata_buffer,
                    weechat_hdata_get_list (relay_hdata_buffer, "gui_buffers"),
                    ptr_buffer))
            {
                /* invalid pointer! */
                ptr_buffer = NULL;
            }
        }
        else
        {
            ptr_buffer = NULL;
        }
    }
    else
        ptr_buffer = weechat_buffer_search ("==", arg);

    return ptr_buffer;
}

/*
 * Gets integer value of a synchronization flag.
 */

int
relay_weechat_protocol_sync_flag (const char *flag)
{
    if (strcmp (flag, "buffer") == 0)
        return RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER;
    if (strcmp (flag, "nicklist") == 0)
        return RELAY_WEECHAT_PROTOCOL_SYNC_NICKLIST;
    if (strcmp (flag, "buffers") == 0)
        return RELAY_WEECHAT_PROTOCOL_SYNC_BUFFERS;
    if (strcmp (flag, "upgrade") == 0)
        return RELAY_WEECHAT_PROTOCOL_SYNC_UPGRADE;

    /* unknown flag */
    return 0;
}

/*
 * Checks if buffer is synchronized with at least one of the flags given.
 *
 * First searches buffer with full_name in hashtable "buffers_sync" (if buffer
 * is not NULL).
 * If buffer is NULL or not found, searches "*" (which means "all buffers").
 *
 * The "flags" argument can be a combination (logical OR) of:
 *   RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER
 *   RELAY_WEECHAT_PROTOCOL_SYNC_NICKLIST
 *   RELAY_WEECHAT_PROTOCOL_SYNC_BUFFERS
 *   RELAY_WEECHAT_PROTOCOL_SYNC_UPGRADE
 *
 * Returns:
 *   1: buffer is synchronized with at least one flag given
 *   0: buffer is NOT synchronized with any of the flags given
 */

int
relay_weechat_protocol_is_sync (struct t_relay_client *ptr_client,
                                struct t_gui_buffer *buffer, int flags)
{
    int *ptr_flags;

    /* search buffer using its full name */
    if (buffer)
    {
        ptr_flags = weechat_hashtable_get (RELAY_WEECHAT_DATA(ptr_client, buffers_sync),
                                           weechat_buffer_get_string (buffer, "full_name"));
        if (ptr_flags)
            return ((*ptr_flags) & flags) ? 1 : 0;
    }

    /* search special name "*" as fallback */
    ptr_flags = weechat_hashtable_get (RELAY_WEECHAT_DATA(ptr_client, buffers_sync),
                                       "*");
    if (ptr_flags)
        return ((*ptr_flags) & flags) ? 1 : 0;

    /*
     * buffer not found at all in hashtable (neither name, neither "*")
     * => it is NOT synchronized
     */
    return 0;
}

/*
 * Replies to a client handshake command.
 */

void
relay_weechat_protocol_handshake_reply (struct t_relay_client *client,
                                        const char *id)
{
    struct t_relay_weechat_msg *msg;
    struct t_hashtable *hashtable;
    char *totp_secret, string[64];

    totp_secret = weechat_string_eval_expression (
        weechat_config_string (relay_config_network_totp_secret),
        NULL, NULL, NULL);

    hashtable = weechat_hashtable_new (32,
                                       WEECHAT_HASHTABLE_STRING,
                                       WEECHAT_HASHTABLE_STRING,
                                       NULL, NULL);
    if (hashtable)
    {
        weechat_hashtable_set (
            hashtable,
            "password_hash_algo",
            (client->password_hash_algo >= 0) ?
            relay_auth_password_hash_algo_name[client->password_hash_algo] : "");
        snprintf (string, sizeof (string),
                  "%d",
                  weechat_config_integer (relay_config_network_password_hash_iterations));
        weechat_hashtable_set (
            hashtable,
            "password_hash_iterations",
            string);
        weechat_hashtable_set (
            hashtable,
            "nonce",
            client->nonce);
        weechat_hashtable_set (
            hashtable,
            "totp",
            (totp_secret && totp_secret[0]) ? "on" : "off");
        weechat_hashtable_set (
            hashtable,
            "compression",
            relay_weechat_compression_string[RELAY_WEECHAT_DATA(client, compression)]);
        weechat_hashtable_set (
            hashtable,
            "escape_commands",
            RELAY_WEECHAT_DATA(client, escape_commands) ? "on" : "off");

        msg = relay_weechat_msg_new (id);
        if (msg)
        {
            relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_HASHTABLE);
            relay_weechat_msg_add_hashtable (msg, hashtable);

            /* send message */
            relay_weechat_msg_send (client, msg);
            relay_weechat_msg_free (msg);
        }

        weechat_hashtable_free (hashtable);
    }

    free (totp_secret);
}


/*
 * Callback for command "handshake" (from client).
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(handshake)
{
    char **options, **auths, **compressions, *pos, **ptr_option, **ptr_auth;
    char **ptr_comp;
    int index_hash_algo, hash_algo_found, auth_allowed, compression;
    int password_received, plain_text_password;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(0);

    if (client->status != RELAY_STATUS_AUTHENTICATING)
        return WEECHAT_RC_OK;

    /* only one handshake is allowed */
    if (RELAY_WEECHAT_DATA(client, handshake_done))
    {
        relay_client_set_status (client, RELAY_STATUS_AUTH_FAILED);
        return WEECHAT_RC_OK;
    }

    hash_algo_found = -1;
    password_received = 0;

    options = (argc > 0) ?
        weechat_string_split_command (argv_eol[0], ',') : NULL;
    if (options)
    {
        for (ptr_option = options; *ptr_option; ptr_option++)
        {
            pos = strchr (*ptr_option, '=');
            if (pos)
            {
                pos[0] = '\0';
                pos++;
                if (strcmp (*ptr_option, "password_hash_algo") == 0)
                {
                    password_received = 1;
                    auths = weechat_string_split (
                        pos,
                        ":",
                        NULL,
                        WEECHAT_STRING_SPLIT_STRIP_LEFT
                        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                        0,
                        NULL);
                    if (auths)
                    {
                        for (ptr_auth = auths; *ptr_auth; ptr_auth++)
                        {
                            index_hash_algo = relay_auth_password_hash_algo_search (*ptr_auth);
                            if ((index_hash_algo >= 0) && (index_hash_algo > hash_algo_found))
                            {
                                auth_allowed = weechat_string_match_list (
                                    relay_auth_password_hash_algo_name[index_hash_algo],
                                    (const char **)relay_config_network_password_hash_algo_list,
                                    1);
                                if (auth_allowed)
                                    hash_algo_found = index_hash_algo;
                            }
                        }
                        weechat_string_free_split (auths);
                    }
                }
                else if (strcmp (*ptr_option, "compression") == 0)
                {
                    compressions = weechat_string_split (
                        pos,
                        ":",
                        NULL,
                        WEECHAT_STRING_SPLIT_STRIP_LEFT
                        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                        0,
                        NULL);
                    if (compressions)
                    {
                        for (ptr_comp = compressions; *ptr_comp; ptr_comp++)
                        {
                            compression = relay_weechat_compression_search (*ptr_comp);
                            if (compression >= 0)
                            {
                                RELAY_WEECHAT_DATA(client, compression) = compression;
                                break;
                            }
                        }
                        weechat_string_free_split (compressions);
                    }
                }
                else if (strcmp (*ptr_option, "escape_commands") == 0)
                {
                    RELAY_WEECHAT_DATA(client, escape_commands) =
                        (weechat_strcmp (pos, "on") == 0) ?
                        1 : 0;
                }
            }
        }
        weechat_string_free_split_command (options);
    }

    if (!password_received)
    {
        plain_text_password = weechat_string_match_list (
            relay_auth_password_hash_algo_name[0],
            (const char **)relay_config_network_password_hash_algo_list,
            1);
        if (plain_text_password)
            hash_algo_found = 0;
    }

    client->password_hash_algo = hash_algo_found;

    relay_weechat_protocol_handshake_reply (client, id);

    RELAY_WEECHAT_DATA(client, handshake_done) = 1;

    /* if no algo was found, we close the connection immediately */
    if (client->password_hash_algo < 0)
        relay_client_set_status (client, RELAY_STATUS_AUTH_FAILED);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "init" (from client).
 *
 * Format is:  init arg1=value1,arg2=value2
 *
 * Allowed arguments:
 *   password       plain text password (recommended with TLS only)
 *   password_hash  hashed password, value is: algorithm:[parameters:]hash
 *                  supported algorithms: sha256, sha512 and pbkdf2
 *                  for pbkdf2, parameters are: algorithm, salt, iterations
 *                  hash is given in hexadecimal
 *   totp           time-based one time password used as secondary
 *                  authentication factor
 *
 * Message looks like:
 *   init password=mypass
 *   init password_hash=sha256:71c480df93d6ae2f1efad1447c66c9…,totp=123456
 *   init password_hash=pbkdf2:sha256:414232…:100000:01757d53157c…,totp=123456
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(init)
{
    char **options, *pos, *relay_password, *totp_secret;
    char *info_totp_args, *info_totp, **ptr_option;
    int password_received, totp_received;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(0);

    relay_password = weechat_string_eval_expression (
        weechat_config_string (relay_config_network_password),
        NULL, NULL, NULL);
    totp_secret = weechat_string_eval_expression (
        weechat_config_string (relay_config_network_totp_secret),
        NULL, NULL, NULL);

    password_received = 0;
    totp_received = 0;

    options = (argc > 0) ?
        weechat_string_split_command (argv_eol[0], ',') : NULL;
    if (options)
    {
        for (ptr_option = options; *ptr_option; ptr_option++)
        {
            pos = strchr (*ptr_option, '=');
            if (pos)
            {
                pos[0] = '\0';
                pos++;
                if (strcmp (*ptr_option, "password") == 0)
                {
                    password_received = 1;
                    if ((client->password_hash_algo == RELAY_AUTH_PASSWORD_HASH_PLAIN)
                        && (relay_auth_check_password_plain (client, pos, relay_password) == 0))
                    {
                        RELAY_WEECHAT_DATA(client, password_ok) = 1;
                    }
                }
                else if (strcmp (*ptr_option, "password_hash") == 0)
                {
                    password_received = 1;
                    if (relay_auth_password_hash (client, pos, relay_password) == 0)
                        RELAY_WEECHAT_DATA(client, password_ok) = 1;
                }
                else if (strcmp (*ptr_option, "totp") == 0)
                {
                    totp_received = 1;
                    if (totp_secret)
                    {
                        /* validate the OTP received from the client */
                        if (weechat_asprintf (
                                &info_totp_args,
                                "%s,%s,0,%d",
                                totp_secret,  /* the shared secret */
                                pos,          /* the OTP from client */
                                weechat_config_integer (relay_config_network_totp_window)) >= 0)
                        {
                            info_totp = weechat_info_get ("totp_validate", info_totp_args);
                            if (info_totp && (strcmp (info_totp, "1") == 0))
                                RELAY_WEECHAT_DATA(client, totp_ok) = 1;
                            free (info_totp);
                            free (info_totp_args);
                        }
                    }
                }
            }
        }
        weechat_string_free_split_command (options);
    }

    /* if no password received and password is empty, it's OK */
    if (!password_received && (!relay_password || !relay_password[0]))
        RELAY_WEECHAT_DATA(client, password_ok) = 1;

    /* if no TOTP received and totp_secret is empty, it's OK */
    if (!totp_received && (!totp_secret || !totp_secret[0]))
        RELAY_WEECHAT_DATA(client, totp_ok) = 1;

    if (RELAY_WEECHAT_AUTH_OK(client))
    {
        weechat_hook_signal_send ("relay_client_auth_ok",
                                  WEECHAT_HOOK_SIGNAL_POINTER,
                                  client);
        relay_client_set_status (client, RELAY_STATUS_CONNECTED);
    }
    else
    {
        relay_client_set_status (client, RELAY_STATUS_AUTH_FAILED);
    }

    free (relay_password);
    free (totp_secret);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "hdata" (from client).
 *
 * Message looks like:
 *   hdata buffer:gui_buffers(*) id,number,name,type,nicklist,title
 *   hdata buffer:gui_buffers(*)/own_lines/first_line(*)/data date,displayed,prefix,message
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(hdata)
{
    struct t_relay_weechat_msg *msg;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(1);

    msg = relay_weechat_msg_new (id);
    if (msg)
    {
        if (!relay_weechat_msg_add_hdata (msg, argv[0],
                                          (argc > 1) ? argv_eol[1] : NULL))
        {
            relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_HDATA);
            relay_weechat_msg_add_string (msg, NULL);  /* h-path */
            relay_weechat_msg_add_string (msg, NULL);  /* keys */
            relay_weechat_msg_add_int (msg, 0);  /* count */
        }
        relay_weechat_msg_send (client, msg);
        relay_weechat_msg_free (msg);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "info" (from client).
 *
 * Message looks like:
 *   info version
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(info)
{
    struct t_relay_weechat_msg *msg;
    char *info;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(1);

    msg = relay_weechat_msg_new (id);
    if (msg)
    {
        info = weechat_info_get (argv[0],
                                 (argc > 1) ? argv_eol[1] : NULL);
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_INFO);
        relay_weechat_msg_add_string (msg, argv[0]);
        relay_weechat_msg_add_string (msg, info);
        relay_weechat_msg_send (client, msg);
        relay_weechat_msg_free (msg);
        free (info);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "infolist" (from client).
 *
 * Message looks like:
 *   infolist buffer
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(infolist)
{
    struct t_relay_weechat_msg *msg;
    void *pointer;
    char *args;
    int rc;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(1);

    msg = relay_weechat_msg_new (id);
    if (msg)
    {
        pointer = NULL;
        args = NULL;
        if (argc > 1)
        {
            rc = sscanf (argv[1], "%p", &pointer);
            if ((rc == EOF) || (rc == 0))
                pointer = NULL;
            if (argc > 2)
                args = argv_eol[2];
        }
        relay_weechat_msg_add_infolist (msg, argv[0], pointer, args);
        relay_weechat_msg_send (client, msg);
        relay_weechat_msg_free (msg);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "nicklist" (from client).
 *
 * Message looks like:
 *   nicklist irc.libera.#weechat
 *   nicklist 0x12345678
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(nicklist)
{
    struct t_relay_weechat_msg *msg;
    struct t_gui_buffer *ptr_buffer;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(0);

    ptr_buffer = NULL;

    if (argc > 0)
    {
        ptr_buffer = relay_weechat_protocol_get_buffer (argv[0]);
        if (!ptr_buffer)
        {
            if (weechat_relay_plugin->debug >= 1)
            {
                weechat_printf (NULL,
                                _("%s: invalid buffer in message: \"%s %s\""),
                                RELAY_PLUGIN_NAME,
                                command,
                                argv_eol[0]);
            }
            return WEECHAT_RC_OK;
        }
    }

    msg = relay_weechat_msg_new (id);
    if (msg)
    {
        relay_weechat_msg_add_nicklist (msg, ptr_buffer, NULL);
        relay_weechat_msg_send (client, msg);
        relay_weechat_msg_free (msg);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "input" (from client).
 *
 * Message looks like:
 *   input core.weechat /help filter
 *   input irc.libera.#weechat hello guys!
 *   input 0x12345678 hello guys!
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(input)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_hashtable *options;
    const char *ptr_commands;
    char *pos;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(1);

    ptr_buffer = relay_weechat_protocol_get_buffer (argv[0]);
    if (!ptr_buffer)
    {
        if (weechat_relay_plugin->debug >= 1)
        {
            weechat_printf (NULL,
                            _("%s: invalid buffer in message: \"%s %s\""),
                            RELAY_PLUGIN_NAME,
                            command,
                            argv[0]);
        }
        return WEECHAT_RC_OK;
    }

    pos = strchr (argv_eol[0], ' ');
    if (!pos)
        return WEECHAT_RC_OK;

    pos++;
    options = weechat_hashtable_new (8,
                                     WEECHAT_HASHTABLE_STRING,
                                     WEECHAT_HASHTABLE_STRING,
                                     NULL, NULL);
    if (!options)
    {
        weechat_printf (NULL,
                        _("%s%s: not enough memory"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        return WEECHAT_RC_OK;
    }

    ptr_commands = weechat_config_string (relay_config_network_commands);
    if (ptr_commands && ptr_commands[0])
        weechat_hashtable_set (options, "commands", ptr_commands);

    /*
     * delay the execution of command after we go back in the WeeChat
     * main loop (some commands like /upgrade executed now can cause
     * a crash)
     */
    weechat_hashtable_set (options, "delay", "1");

    /* execute the command, with the delay */
    weechat_command_options (ptr_buffer, pos, options);

    weechat_hashtable_free (options);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "completion" (from client).
 *
 * Message looks like:
 *   completion core.weechat -1 /help fi
 *   input irc.libera.#weechat 5 /quernick
 *   input 0x12345678 -1 nick
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(completion)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_completion *completion;
    struct t_gui_completion_word *word;
    struct t_arraylist *ptr_list;
    struct t_relay_weechat_msg *msg;
    char *error, *pos_data;
    int i, position, length_data, context, pos_start, size;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(0);

    completion = NULL;

    /* return an empty hdata as error if there are not enough arguments */
    if (argc < 2)
        goto error;

    ptr_buffer = relay_weechat_protocol_get_buffer (argv[0]);
    if (!ptr_buffer)
    {
        if (weechat_relay_plugin->debug >= 1)
        {
            weechat_printf (NULL,
                            _("%s: invalid buffer in message: \"%s %s\""),
                            RELAY_PLUGIN_NAME,
                            command,
                            argv[0]);
        }
        goto error;
    }

    error = NULL;
    position = (int)strtol (argv[1], &error, 10);
    if (!error || error[0])
        goto error;

    pos_data = strchr (argv_eol[1], ' ');
    if (pos_data)
        pos_data++;

    length_data = strlen ((pos_data) ? pos_data : "");
    if ((position < 0) || (position > length_data))
        position = length_data;

    completion = weechat_completion_new (ptr_buffer);
    if (!completion)
        goto error;

    if (!weechat_completion_search (completion,
                                    (pos_data) ? pos_data : "",
                                    position,
                                    1))
    {
        goto error;
    }

    ptr_list = weechat_hdata_pointer (relay_hdata_completion, completion, "list");
    if (!ptr_list)
        goto error;

    msg = relay_weechat_msg_new (id);
    if (msg)
    {
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_HDATA);
        relay_weechat_msg_add_string (msg, "completion");
        relay_weechat_msg_add_string (msg,
                                      "context:str,"
                                      "base_word:str,"
                                      "pos_start:int,"
                                      "pos_end:int,"
                                      "add_space:int,"
                                      "list:arr");
        relay_weechat_msg_add_int (msg, 1); /* count */
        relay_weechat_msg_add_pointer (msg, completion);
        /* context */
        context = weechat_hdata_integer (relay_hdata_completion, completion,
                                         "context");
        switch (context)
        {
            case 0:
                relay_weechat_msg_add_string (msg, "null");
                break;
            case 1:
                relay_weechat_msg_add_string (msg, "command");
                break;
            case 2:
                relay_weechat_msg_add_string (msg, "command_arg");
                break;
            default:
                relay_weechat_msg_add_string (msg, "auto");
                break;
        }
        /* base_word */
        relay_weechat_msg_add_string (
            msg,
            weechat_hdata_string (relay_hdata_completion,
                                  completion, "base_word"));
        /* pos_start */
        pos_start = weechat_hdata_integer (relay_hdata_completion,
                                           completion, "position_replace");
        relay_weechat_msg_add_int (msg, pos_start);

        /* pos_end */
        relay_weechat_msg_add_int (
            msg,
            (position > pos_start) ? position - 1 : position);
        /* add_space */
        relay_weechat_msg_add_int (
            msg,
            weechat_hdata_integer (relay_hdata_completion,
                                   completion, "add_space"));
        /* list */
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_STRING);
        size = weechat_arraylist_size (ptr_list);
        relay_weechat_msg_add_int (msg, size);
        for (i = 0; i < size; i++)
        {
            word = (struct t_gui_completion_word *)weechat_arraylist_get (
                ptr_list, i);
            relay_weechat_msg_add_string (
                msg,
                weechat_hdata_string (relay_hdata_completion_word, word, "word"));
        }

        /* send message */
        relay_weechat_msg_send (client, msg);
        relay_weechat_msg_free (msg);
    }

    weechat_completion_free (completion);

    return WEECHAT_RC_OK;

error:
    weechat_completion_free (completion);

    msg = relay_weechat_msg_new (id);
    if (msg)
    {
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_HDATA);
        relay_weechat_msg_add_string (msg, "completion");
        relay_weechat_msg_add_string (msg, NULL);  /* keys */
        relay_weechat_msg_add_int (msg, 0);  /* count */

        /* send message */
        relay_weechat_msg_send (client, msg);
        relay_weechat_msg_free (msg);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for signals "buffer_*".
 */

int
relay_weechat_protocol_signal_buffer_cb (const void *pointer, void *data,
                                         const char *signal,
                                         const char *type_data,
                                         void *signal_data)
{
    struct t_relay_client *ptr_client;
    struct t_gui_line *ptr_line;
    struct t_gui_line_data *ptr_line_data;
    struct t_gui_buffer *ptr_buffer;
    struct t_relay_weechat_msg *msg;
    char cmd_hdata[64], str_signal[128];
    const char *ptr_old_full_name;
    int *ptr_old_flags, flags;

    /* make C compiler happy */
    (void) data;
    (void) type_data;

    ptr_client = (struct t_relay_client *)pointer;
    if (!ptr_client || !relay_client_valid (ptr_client))
        return WEECHAT_RC_OK;

    snprintf (str_signal, sizeof (str_signal), "_%s", signal);

    if (strcmp (signal, "buffer_opened") == 0)
    {
        ptr_buffer = (struct t_gui_buffer *)signal_data;
        if (!ptr_buffer)
            return WEECHAT_RC_OK;

        /* send signal only if sync with flag "buffers" or "buffer" */
        if (relay_weechat_protocol_is_sync (ptr_client, ptr_buffer,
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFERS |
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER))
        {
            msg = relay_weechat_msg_new (str_signal);
            if (msg)
            {
                snprintf (cmd_hdata, sizeof (cmd_hdata),
                          "buffer:0x%lx", (unsigned long)ptr_buffer);
                relay_weechat_msg_add_hdata (msg, cmd_hdata,
                                             "id,number,full_name,short_name,"
                                             "nicklist,title,local_variables,"
                                             "prev_buffer,next_buffer");
                relay_weechat_msg_send (ptr_client, msg);
                relay_weechat_msg_free (msg);
            }
        }
    }
    else if (strcmp (signal, "buffer_type_changed") == 0)
    {
        ptr_buffer = (struct t_gui_buffer *)signal_data;
        if (!ptr_buffer)
            return WEECHAT_RC_OK;

        /* send signal only if sync with flag "buffers" or "buffer" */
        if (relay_weechat_protocol_is_sync (ptr_client, ptr_buffer,
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFERS |
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER))
        {
            msg = relay_weechat_msg_new (str_signal);
            if (msg)
            {
                snprintf (cmd_hdata, sizeof (cmd_hdata),
                          "buffer:0x%lx", (unsigned long)ptr_buffer);
                relay_weechat_msg_add_hdata (msg, cmd_hdata,
                                             "id,number,full_name,type");
                relay_weechat_msg_send (ptr_client, msg);
                relay_weechat_msg_free (msg);
            }
        }
    }
    else if (strcmp (signal, "buffer_moved") == 0)
    {
        ptr_buffer = (struct t_gui_buffer *)signal_data;
        if (!ptr_buffer)
            return WEECHAT_RC_OK;

        /* send signal only if sync with flag "buffers" or "buffer" */
        if (relay_weechat_protocol_is_sync (ptr_client, ptr_buffer,
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFERS |
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER))
        {
            msg = relay_weechat_msg_new (str_signal);
            if (msg)
            {
                snprintf (cmd_hdata, sizeof (cmd_hdata),
                          "buffer:0x%lx", (unsigned long)ptr_buffer);
                relay_weechat_msg_add_hdata (msg, cmd_hdata,
                                             "id,number,full_name,"
                                             "prev_buffer,next_buffer");
                relay_weechat_msg_send (ptr_client, msg);
                relay_weechat_msg_free (msg);
            }
        }
    }
    else if ((strcmp (signal, "buffer_merged") == 0)
             || (strcmp (signal, "buffer_unmerged") == 0))
    {
        ptr_buffer = (struct t_gui_buffer *)signal_data;
        if (!ptr_buffer)
            return WEECHAT_RC_OK;

        /* send signal only if sync with flag "buffers" or "buffer" */
        if (relay_weechat_protocol_is_sync (ptr_client, ptr_buffer,
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFERS |
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER))
        {
            msg = relay_weechat_msg_new (str_signal);
            if (msg)
            {
                snprintf (cmd_hdata, sizeof (cmd_hdata),
                          "buffer:0x%lx", (unsigned long)ptr_buffer);
                relay_weechat_msg_add_hdata (msg, cmd_hdata,
                                             "id,number,full_name,"
                                             "prev_buffer,next_buffer");
                relay_weechat_msg_send (ptr_client, msg);
                relay_weechat_msg_free (msg);
            }
        }
    }
    else if ((strcmp (signal, "buffer_hidden") == 0)
             || (strcmp (signal, "buffer_unhidden") == 0))
    {
        ptr_buffer = (struct t_gui_buffer *)signal_data;
        if (!ptr_buffer)
            return WEECHAT_RC_OK;

        /* send signal only if sync with flag "buffers" or "buffer" */
        if (relay_weechat_protocol_is_sync (ptr_client, ptr_buffer,
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFERS |
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER))
        {
            msg = relay_weechat_msg_new (str_signal);
            if (msg)
            {
                snprintf (cmd_hdata, sizeof (cmd_hdata),
                          "buffer:0x%lx", (unsigned long)ptr_buffer);
                relay_weechat_msg_add_hdata (msg, cmd_hdata,
                                             "id,number,full_name,"
                                             "prev_buffer,next_buffer");
                relay_weechat_msg_send (ptr_client, msg);
                relay_weechat_msg_free (msg);
            }
        }
    }
    else if (strcmp (signal, "buffer_renamed") == 0)
    {
        ptr_buffer = (struct t_gui_buffer *)signal_data;
        if (!ptr_buffer)
            return WEECHAT_RC_OK;

        /* rename old buffer name if present in hashtable "buffers_sync" */
        ptr_old_full_name = weechat_buffer_get_string (ptr_buffer,
                                                       "old_full_name");
        if (ptr_old_full_name && ptr_old_full_name[0])
        {
            ptr_old_flags = weechat_hashtable_get (
                RELAY_WEECHAT_DATA(ptr_client, buffers_sync),
                ptr_old_full_name);
            if (ptr_old_flags)
            {
                flags = *ptr_old_flags;
                weechat_hashtable_remove (
                    RELAY_WEECHAT_DATA(ptr_client, buffers_sync),
                    ptr_old_full_name);
                weechat_hashtable_set (
                    RELAY_WEECHAT_DATA(ptr_client, buffers_sync),
                    weechat_buffer_get_string (ptr_buffer, "full_name"),
                    &flags);
            }
        }

        /* send signal only if sync with flag "buffers" or "buffer" */
        if (relay_weechat_protocol_is_sync (ptr_client, ptr_buffer,
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFERS |
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER))
        {
            msg = relay_weechat_msg_new (str_signal);
            if (msg)
            {
                snprintf (cmd_hdata, sizeof (cmd_hdata),
                          "buffer:0x%lx", (unsigned long)ptr_buffer);
                relay_weechat_msg_add_hdata (msg, cmd_hdata,
                                             "id,number,full_name,short_name,"
                                             "local_variables");
                relay_weechat_msg_send (ptr_client, msg);
                relay_weechat_msg_free (msg);
            }
        }
    }
    else if (strcmp (signal, "buffer_title_changed") == 0)
    {
        ptr_buffer = (struct t_gui_buffer *)signal_data;
        if (!ptr_buffer)
            return WEECHAT_RC_OK;

        /* send signal only if sync with flag "buffers" or "buffer" */
        if (relay_weechat_protocol_is_sync (ptr_client, ptr_buffer,
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFERS |
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER))
        {
            msg = relay_weechat_msg_new (str_signal);
            if (msg)
            {
                snprintf (cmd_hdata, sizeof (cmd_hdata),
                          "buffer:0x%lx", (unsigned long)ptr_buffer);
                relay_weechat_msg_add_hdata (msg, cmd_hdata,
                                             "id,number,full_name,title");
                relay_weechat_msg_send (ptr_client, msg);
                relay_weechat_msg_free (msg);
            }
        }
    }
    else if (strncmp (signal, "buffer_localvar_", 16) == 0)
    {
        ptr_buffer = (struct t_gui_buffer *)signal_data;
        if (!ptr_buffer)
            return WEECHAT_RC_OK;

        /* send signal only if sync with flag "buffers" or "buffer" */
        if (relay_weechat_protocol_is_sync (ptr_client, ptr_buffer,
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFERS |
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER))
        {
            msg = relay_weechat_msg_new (str_signal);
            if (msg)
            {
                snprintf (cmd_hdata, sizeof (cmd_hdata),
                          "buffer:0x%lx", (unsigned long)ptr_buffer);
                relay_weechat_msg_add_hdata (msg, cmd_hdata,
                                             "id,number,full_name,local_variables");
                relay_weechat_msg_send (ptr_client, msg);
                relay_weechat_msg_free (msg);
            }
        }
    }
    else if (strcmp (signal, "buffer_cleared") == 0)
    {
        ptr_buffer = (struct t_gui_buffer *)signal_data;
        if (!ptr_buffer || relay_buffer_is_relay (ptr_buffer))
            return WEECHAT_RC_OK;

        /* send signal only if sync with flag "buffer" */
        if (relay_weechat_protocol_is_sync (ptr_client, ptr_buffer,
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER))
        {
            msg = relay_weechat_msg_new (str_signal);
            if (msg)
            {
                snprintf (cmd_hdata, sizeof (cmd_hdata),
                          "buffer:0x%lx", (unsigned long)ptr_buffer);
                relay_weechat_msg_add_hdata (msg, cmd_hdata,
                                             "id,number,full_name");
                relay_weechat_msg_send (ptr_client, msg);
                relay_weechat_msg_free (msg);
            }
        }
    }
    else if (strcmp (signal, "buffer_line_added") == 0)
    {
        ptr_line = (struct t_gui_line *)signal_data;
        if (!ptr_line)
            return WEECHAT_RC_OK;

        ptr_line_data = weechat_hdata_pointer (relay_hdata_line, ptr_line, "data");
        if (!ptr_line_data)
            return WEECHAT_RC_OK;

        ptr_buffer = weechat_hdata_pointer (relay_hdata_line_data, ptr_line_data,
                                            "buffer");
        if (!ptr_buffer || relay_buffer_is_relay (ptr_buffer))
            return WEECHAT_RC_OK;

        /* send signal only if sync with flag "buffer" */
        if (relay_weechat_protocol_is_sync (ptr_client, ptr_buffer,
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER))
        {
            msg = relay_weechat_msg_new (str_signal);
            if (msg)
            {
                snprintf (cmd_hdata, sizeof (cmd_hdata),
                          "line_data:0x%lx",
                          (unsigned long)ptr_line_data);
                relay_weechat_msg_add_hdata (
                    msg, cmd_hdata,
                    "buffer,id,date,date_usec,date_printed,date_usec_printed,"
                    "displayed,notify_level,highlight,tags_array,"
                    "prefix,message");
                relay_weechat_msg_send (ptr_client, msg);
                relay_weechat_msg_free (msg);
            }
        }
    }
    else if (strcmp (signal, "buffer_line_data_changed") == 0)
    {
        ptr_line_data = (struct t_gui_line_data *)signal_data;
        if (!ptr_line_data)
            return WEECHAT_RC_OK;

        ptr_buffer = weechat_hdata_pointer (relay_hdata_line_data, ptr_line_data,
                                            "buffer");
        if (!ptr_buffer || relay_buffer_is_relay (ptr_buffer))
            return WEECHAT_RC_OK;

        /* send signal only if sync with flag "buffer" */
        if (relay_weechat_protocol_is_sync (ptr_client, ptr_buffer,
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER))
        {
            msg = relay_weechat_msg_new (str_signal);
            if (msg)
            {
                snprintf (cmd_hdata, sizeof (cmd_hdata),
                          "line_data:0x%lx",
                          (unsigned long)ptr_line_data);
                relay_weechat_msg_add_hdata (
                    msg, cmd_hdata,
                    "buffer,id,date,date_usec,date_printed,date_usec_printed,"
                    "displayed,notify_level,highlight,tags_array,"
                    "prefix,message");
                relay_weechat_msg_send (ptr_client, msg);
                relay_weechat_msg_free (msg);
            }
        }
    }
    else if (strcmp (signal, "buffer_closing") == 0)
    {
        ptr_buffer = (struct t_gui_buffer *)signal_data;
        if (!ptr_buffer)
            return WEECHAT_RC_OK;

        /* send signal only if sync with flag "buffers" or "buffer" */
        if (relay_weechat_protocol_is_sync (ptr_client, ptr_buffer,
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFERS |
                                            RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER))
        {
            msg = relay_weechat_msg_new (str_signal);
            if (msg)
            {
                snprintf (cmd_hdata, sizeof (cmd_hdata),
                          "buffer:0x%lx", (unsigned long)ptr_buffer);
                relay_weechat_msg_add_hdata (msg, cmd_hdata,
                                             "id,number,full_name");
                relay_weechat_msg_send (ptr_client, msg);
                relay_weechat_msg_free (msg);
            }
        }

        /* remove buffer from hashtables */
        weechat_hashtable_remove (
            RELAY_WEECHAT_DATA(ptr_client, buffers_sync),
            weechat_buffer_get_string (ptr_buffer, "full_name"));
        weechat_hashtable_remove (
            RELAY_WEECHAT_DATA(ptr_client, buffers_nicklist),
            ptr_buffer);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for entries in hashtable "buffers_nicklist" of client (sends
 * nicklist for each buffer in this hashtable).
 */

void
relay_weechat_protocol_nicklist_map_cb (void *data,
                                        struct t_hashtable *hashtable,
                                        const void *key,
                                        const void *value)
{
    struct t_relay_client *ptr_client;
    struct t_gui_buffer *ptr_buffer;
    struct t_relay_weechat_nicklist *ptr_nicklist;
    struct t_relay_weechat_msg *msg;

    /* make C compiler happy */
    (void) hashtable;

    ptr_client = (struct t_relay_client *)data;
    ptr_buffer = (struct t_gui_buffer *)key;
    ptr_nicklist = (struct t_relay_weechat_nicklist *)value;

    if (weechat_hdata_check_pointer (
            relay_hdata_buffer,
            weechat_hdata_get_list (relay_hdata_buffer, "gui_buffers"),
            ptr_buffer))
    {
        /*
         * if no diff at all, or if diffs are bigger than nicklist:
         * send whole nicklist
         */
        if (ptr_nicklist
            && ((ptr_nicklist->items_count == 0)
                || (ptr_nicklist->items_count >= weechat_buffer_get_integer (ptr_buffer, "nicklist_count") + 1)))
        {
            ptr_nicklist = NULL;
        }

        /* send nicklist diffs or full nicklist */
        msg = relay_weechat_msg_new ((ptr_nicklist) ? "_nicklist_diff" : "_nicklist");
        if (msg)
        {
            relay_weechat_msg_add_nicklist (msg, ptr_buffer, ptr_nicklist);
            relay_weechat_msg_send (ptr_client, msg);
            relay_weechat_msg_free (msg);
        }
    }
}

/*
 * Callback for nicklist timer.
 */

int
relay_weechat_protocol_timer_nicklist_cb (const void *pointer, void *data,
                                          int remaining_calls)
{
    struct t_relay_client *ptr_client;

    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    ptr_client = (struct t_relay_client *)pointer;
    if (!ptr_client || !relay_client_valid (ptr_client))
        return WEECHAT_RC_OK;

    weechat_hashtable_map (RELAY_WEECHAT_DATA(ptr_client, buffers_nicklist),
                           &relay_weechat_protocol_nicklist_map_cb,
                           ptr_client);

    weechat_hashtable_remove_all (RELAY_WEECHAT_DATA(ptr_client, buffers_nicklist));

    RELAY_WEECHAT_DATA(ptr_client, hook_timer_nicklist) = NULL;

    return WEECHAT_RC_OK;
}

/*
 * Callback for hsignals "nicklist_*".
 */

int
relay_weechat_protocol_hsignal_nicklist_cb (const void *pointer, void *data,
                                            const char *signal,
                                            struct t_hashtable *hashtable)
{
    struct t_relay_client *ptr_client;
    struct t_gui_nick_group *parent_group, *group;
    struct t_gui_nick *nick;
    struct t_gui_buffer *ptr_buffer;
    struct t_relay_weechat_nicklist *ptr_nicklist;
    char diff;

    /* make C compiler happy */
    (void) data;

    ptr_client = (struct t_relay_client *)pointer;
    if (!ptr_client || !relay_client_valid (ptr_client))
        return WEECHAT_RC_OK;

    /* check if buffer is synchronized with flag "nicklist" */
    ptr_buffer = weechat_hashtable_get (hashtable, "buffer");
    if (!relay_weechat_protocol_is_sync (ptr_client, ptr_buffer,
                                         RELAY_WEECHAT_PROTOCOL_SYNC_NICKLIST))
        return WEECHAT_RC_OK;

    parent_group = weechat_hashtable_get (hashtable, "parent_group");
    group = weechat_hashtable_get (hashtable, "group");
    nick = weechat_hashtable_get (hashtable, "nick");

    /* if there is no parent group (for example "root" group), ignore the signal */
    if (!parent_group)
        return WEECHAT_RC_OK;

    ptr_nicklist = weechat_hashtable_get (RELAY_WEECHAT_DATA(ptr_client,
                                                             buffers_nicklist),
                                          ptr_buffer);
    if (!ptr_nicklist)
    {
        ptr_nicklist = relay_weechat_nicklist_new ();
        if (!ptr_nicklist)
            return WEECHAT_RC_OK;
        ptr_nicklist->nicklist_count = weechat_buffer_get_integer (ptr_buffer,
                                                                   "nicklist_count");
        weechat_hashtable_set (RELAY_WEECHAT_DATA(ptr_client, buffers_nicklist),
                               ptr_buffer,
                               ptr_nicklist);
    }

    /* set diff type */
    diff = RELAY_WEECHAT_NICKLIST_DIFF_UNKNOWN;
    if ((strcmp (signal, "nicklist_group_added") == 0)
        || (strcmp (signal, "nicklist_nick_added") == 0))
    {
        diff = RELAY_WEECHAT_NICKLIST_DIFF_ADDED;
    }
    else if ((strcmp (signal, "nicklist_group_removing") == 0)
        || (strcmp (signal, "nicklist_nick_removing") == 0))
    {
        diff = RELAY_WEECHAT_NICKLIST_DIFF_REMOVED;
    }
    else if ((strcmp (signal, "nicklist_group_changed") == 0)
        || (strcmp (signal, "nicklist_nick_changed") == 0))
    {
        diff = RELAY_WEECHAT_NICKLIST_DIFF_CHANGED;
    }

    if (diff != RELAY_WEECHAT_NICKLIST_DIFF_UNKNOWN)
    {
        /*
         * add items if nicklist was not empty or very small (otherwise we will
         * send full nicklist)
         */
        if (ptr_nicklist->nicklist_count > 1)
        {
            /* add nicklist item for parent group and group/nick */
            relay_weechat_nicklist_add_item (ptr_nicklist,
                                             RELAY_WEECHAT_NICKLIST_DIFF_PARENT,
                                             parent_group, NULL);
            relay_weechat_nicklist_add_item (ptr_nicklist, diff, group, nick);
        }

        /* add timer to send nicklist */
        if (RELAY_WEECHAT_DATA(ptr_client, hook_timer_nicklist))
        {
            weechat_unhook (RELAY_WEECHAT_DATA(ptr_client, hook_timer_nicklist));
            RELAY_WEECHAT_DATA(ptr_client, hook_timer_nicklist) = NULL;
        }
        relay_weechat_hook_timer_nicklist (ptr_client);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for signals "upgrade*".
 */

int
relay_weechat_protocol_signal_upgrade_cb (const void *pointer, void *data,
                                          const char *signal,
                                          const char *type_data,
                                          void *signal_data)
{
    struct t_relay_client *ptr_client;
    struct t_relay_weechat_msg *msg;
    char str_signal[128];

    /* make C compiler happy */
    (void) data;
    (void) type_data;
    (void) signal_data;

    ptr_client = (struct t_relay_client *)pointer;
    if (!ptr_client || !relay_client_valid (ptr_client))
        return WEECHAT_RC_OK;

    snprintf (str_signal, sizeof (str_signal), "_%s", signal);

    if ((strcmp (signal, "upgrade") == 0)
        || (strcmp (signal, "upgrade_ended") == 0))
    {
        /* send signal only if client is synchronized with flag "upgrade" */
        if (relay_weechat_protocol_is_sync (ptr_client, NULL,
                                            RELAY_WEECHAT_PROTOCOL_SYNC_UPGRADE))
        {
            msg = relay_weechat_msg_new (str_signal);
            if (msg)
            {
                relay_weechat_msg_send (ptr_client, msg);
                relay_weechat_msg_free (msg);
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "sync" (from client).
 *
 * Message looks like:
 *   sync
 *   sync * buffer
 *   sync irc.libera.#weechat buffer,nicklist
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(sync)
{
    char **buffers, **flags;
    const char *ptr_full_name;
    int num_buffers, num_flags, i, add_flags, mask, *ptr_old_flags, new_flags;
    struct t_gui_buffer *ptr_buffer;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(0);

    buffers = weechat_string_split ((argc > 0) ? argv[0] : "*",
                                    ",",
                                    NULL,
                                    WEECHAT_STRING_SPLIT_STRIP_LEFT
                                    | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                    | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                    0,
                                    &num_buffers);
    if (buffers)
    {
        add_flags = RELAY_WEECHAT_PROTOCOL_SYNC_ALL;
        if (argc > 1)
        {
            add_flags = 0;
            flags = weechat_string_split (argv[1], ",", NULL,
                                          WEECHAT_STRING_SPLIT_STRIP_LEFT
                                          | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                          | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                          0, &num_flags);
            if (flags)
            {
                for (i = 0; i < num_flags; i++)
                {
                    add_flags |= relay_weechat_protocol_sync_flag (flags[i]);
                }
                weechat_string_free_split (flags);
            }
        }
        if (add_flags)
        {
            for (i = 0; i < num_buffers; i++)
            {
                ptr_full_name = NULL;
                mask = RELAY_WEECHAT_PROTOCOL_SYNC_FOR_BUFFER;

                if (strcmp (buffers[i], "*") == 0)
                {
                    ptr_full_name = buffers[i];
                    mask = RELAY_WEECHAT_PROTOCOL_SYNC_ALL;
                }
                else
                {
                    ptr_buffer = relay_weechat_protocol_get_buffer (buffers[i]);
                    if (ptr_buffer)
                    {
                        ptr_full_name = weechat_buffer_get_string (ptr_buffer,
                                                                   "full_name");
                    }
                }

                if (ptr_full_name)
                {
                    ptr_old_flags = weechat_hashtable_get (RELAY_WEECHAT_DATA(client, buffers_sync),
                                                           ptr_full_name);
                    new_flags = ((ptr_old_flags) ? *ptr_old_flags : 0);
                    new_flags |= (add_flags & mask);
                    if (new_flags)
                    {
                        weechat_hashtable_set (RELAY_WEECHAT_DATA(client, buffers_sync),
                                               ptr_full_name,
                                               &new_flags);
                    }
                }
            }
        }
        weechat_string_free_split (buffers);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "desync" (from client).
 *
 * Message looks like:
 *   desync
 *   desync * nicklist
 *   desync irc.libera.#weechat buffer,nicklist
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(desync)
{
    char **buffers, **flags;
    const char *ptr_full_name;
    int num_buffers, num_flags, i, sub_flags, mask, *ptr_old_flags, new_flags;
    struct t_gui_buffer *ptr_buffer;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(0);

    buffers = weechat_string_split ((argc > 0) ? argv[0] : "*",
                                    ",",
                                    NULL,
                                    WEECHAT_STRING_SPLIT_STRIP_LEFT
                                    | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                    | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                    0,
                                    &num_buffers);
    if (buffers)
    {
        sub_flags = RELAY_WEECHAT_PROTOCOL_SYNC_ALL;
        if (argc > 1)
        {
            sub_flags = 0;
            flags = weechat_string_split (argv[1], ",", NULL,
                                          WEECHAT_STRING_SPLIT_STRIP_LEFT
                                          | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                          | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                          0, &num_flags);
            if (flags)
            {
                for (i = 0; i < num_flags; i++)
                {
                    sub_flags |= relay_weechat_protocol_sync_flag (flags[i]);
                }
                weechat_string_free_split (flags);
            }
        }
        if (sub_flags)
        {
            for (i = 0; i < num_buffers; i++)
            {
                ptr_full_name = NULL;
                mask = RELAY_WEECHAT_PROTOCOL_SYNC_FOR_BUFFER;

                if (strcmp (buffers[i], "*") == 0)
                {
                    ptr_full_name = buffers[i];
                    mask = RELAY_WEECHAT_PROTOCOL_SYNC_ALL;
                }
                else
                {
                    ptr_buffer = relay_weechat_protocol_get_buffer (buffers[i]);
                    if (ptr_buffer)
                    {
                        ptr_full_name = weechat_buffer_get_string (ptr_buffer,
                                                                   "full_name");
                    }
                }

                if (ptr_full_name)
                {
                    ptr_old_flags = weechat_hashtable_get (RELAY_WEECHAT_DATA(client, buffers_sync),
                                                           ptr_full_name);
                    new_flags = ((ptr_old_flags) ? *ptr_old_flags : 0);
                    new_flags &= ~(sub_flags & mask);
                    if (new_flags)
                    {
                        weechat_hashtable_set (RELAY_WEECHAT_DATA(client, buffers_sync),
                                               ptr_full_name,
                                               &new_flags);
                    }
                    else
                    {
                        weechat_hashtable_remove (RELAY_WEECHAT_DATA(client, buffers_sync),
                                                  ptr_full_name);
                    }
                }
            }
        }
        weechat_string_free_split (buffers);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "test" (from client).
 *
 * Message looks like:
 *   test
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(test)
{
    struct t_relay_weechat_msg *msg;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(0);

    msg = relay_weechat_msg_new (id);
    if (msg)
    {
        /* char */
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_CHAR);
        relay_weechat_msg_add_char (msg, 'A');

        /* integer */
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_INT);
        relay_weechat_msg_add_int (msg, 123456);

        /* integer (negative) */
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_INT);
        relay_weechat_msg_add_int (msg, -123456);

        /* long */
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_LONG);
        relay_weechat_msg_add_long (msg, 1234567890L);

        /* long (negative) */
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_LONG);
        relay_weechat_msg_add_long (msg, -1234567890L);

        /* string */
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_STRING);
        relay_weechat_msg_add_string (msg, "a string");

        /* empty string */
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_STRING);
        relay_weechat_msg_add_string (msg, "");

        /* NULL string */
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_STRING);
        relay_weechat_msg_add_string (msg, NULL);

        /* buffer */
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_BUFFER);
        relay_weechat_msg_add_buffer (msg, "buffer", 6);

        /* NULL buffer */
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_BUFFER);
        relay_weechat_msg_add_buffer (msg, NULL, 0);

        /* pointer */
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_POINTER);
        relay_weechat_msg_add_pointer (msg, (void *)0x1234abcd);

        /* NULL pointer */
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_POINTER);
        relay_weechat_msg_add_pointer (msg, NULL);

        /* time */
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_TIME);
        relay_weechat_msg_add_time (msg, 1321993456);

        /* array of strings: { "abc", "de" } */
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_ARRAY);
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_STRING);
        relay_weechat_msg_add_int (msg, 2);
        relay_weechat_msg_add_string (msg, "abc");
        relay_weechat_msg_add_string (msg, "de");

        /* array of integers: { 123, 456, 789 } */
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_ARRAY);
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_INT);
        relay_weechat_msg_add_int (msg, 3);
        relay_weechat_msg_add_int (msg, 123);
        relay_weechat_msg_add_int (msg, 456);
        relay_weechat_msg_add_int (msg, 789);

        /* send message */
        relay_weechat_msg_send (client, msg);
        relay_weechat_msg_free (msg);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "ping" (from client).
 *
 * Message looks like:
 *   ping
 *   ping 1370802127000
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(ping)
{
    struct t_relay_weechat_msg *msg;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(0);

    msg = relay_weechat_msg_new ("_pong");
    if (msg)
    {
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_STRING);
        relay_weechat_msg_add_string (msg, (argc > 0) ? argv_eol[0] : "");

        /* send message */
        relay_weechat_msg_send (client, msg);
        relay_weechat_msg_free (msg);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "quit" (from client).
 *
 * Message looks like:
 *   test
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(quit)
{
    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(0);

    relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);

    return WEECHAT_RC_OK;
}

/*
 * Reads a command from a client.
 */

void
relay_weechat_protocol_recv (struct t_relay_client *client, const char *data)
{
    const char *ptr_data;
    char *data_unescaped, *pos, *id, *command, **argv, **argv_eol;
    int i, argc, return_code;
    struct t_relay_weechat_protocol_cb protocol_cb[] =
        { { "handshake", &relay_weechat_protocol_cb_handshake },
          { "init", &relay_weechat_protocol_cb_init },
          { "hdata", &relay_weechat_protocol_cb_hdata },
          { "info", &relay_weechat_protocol_cb_info },
          { "infolist", &relay_weechat_protocol_cb_infolist },
          { "nicklist", &relay_weechat_protocol_cb_nicklist },
          { "input", &relay_weechat_protocol_cb_input },
          { "completion", &relay_weechat_protocol_cb_completion },
          { "sync", &relay_weechat_protocol_cb_sync },
          { "desync", &relay_weechat_protocol_cb_desync },
          { "test", &relay_weechat_protocol_cb_test },
          { "ping", &relay_weechat_protocol_cb_ping },
          { "quit", &relay_weechat_protocol_cb_quit },
          { NULL, NULL }
        };

    if (!data || !data[0] || RELAY_STATUS_HAS_ENDED(client->status))
        return;

    /* display debug message */
    if (weechat_relay_plugin->debug >= 2)
    {
        weechat_printf (NULL, "%s: recv from client %s%s%s: \"%s\"",
                        RELAY_PLUGIN_NAME,
                        RELAY_COLOR_CHAT_CLIENT,
                        client->desc,
                        RELAY_COLOR_CHAT,
                        data);
    }

    data_unescaped = NULL;
    ptr_data = data;
    id = NULL;
    command = NULL;
    argv = NULL;
    argv_eol = NULL;

    if (RELAY_WEECHAT_DATA(client, escape_commands))
    {
        data_unescaped = weechat_string_convert_escaped_chars (data);
        if (data_unescaped)
            ptr_data = data_unescaped;
    }

    /* extract id */
    if (ptr_data[0] == '(')
    {
        pos = strchr (ptr_data, ')');
        if (pos)
        {
            id = weechat_strndup (ptr_data + 1, pos - ptr_data - 1);
            ptr_data = pos + 1;
            while (ptr_data[0] == ' ')
            {
                ptr_data++;
            }
        }
    }

    /* search end of data */
    pos = strchr (ptr_data, ' ');
    if (pos)
        command = weechat_strndup (ptr_data, pos - ptr_data);
    else
        command = strdup (ptr_data);

    if (!command)
        goto end;

    argc = 0;
    argv = NULL;
    argv_eol = NULL;

    if (pos)
    {
        while (pos[0] == ' ')
        {
            pos++;
        }
        argv = weechat_string_split (pos, " ", NULL,
                                     WEECHAT_STRING_SPLIT_STRIP_LEFT
                                     | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                     | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                     0, &argc);
        argv_eol = weechat_string_split (pos, " ", NULL,
                                         WEECHAT_STRING_SPLIT_STRIP_LEFT
                                         | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS
                                         | WEECHAT_STRING_SPLIT_KEEP_EOL,
                                         0, NULL);
    }

    for (i = 0; protocol_cb[i].name; i++)
    {
        if (strcmp (protocol_cb[i].name, command) == 0)
        {
            if ((strcmp (protocol_cb[i].name, "handshake") != 0)
                && (strcmp (protocol_cb[i].name, "init") != 0)
                && !RELAY_WEECHAT_AUTH_OK(client))
            {
                /*
                 * command is not handshake/init and password or totp are not
                 * set? then close connection!
                 */
                relay_client_set_status (client,
                                         RELAY_STATUS_AUTH_FAILED);
            }
            else
            {
                return_code = (int) (protocol_cb[i].cmd_function) (client,
                                                                   id,
                                                                   protocol_cb[i].name,
                                                                   argc,
                                                                   argv,
                                                                   argv_eol);
                if ((weechat_relay_plugin->debug >= 1)
                    && (return_code == WEECHAT_RC_ERROR))
                {
                    weechat_printf (NULL,
                                    _("%s%s: failed to execute command \"%s\" "
                                      "for client %s%s%s"),
                                    weechat_prefix ("error"),
                                    RELAY_PLUGIN_NAME,
                                    command,
                                    RELAY_COLOR_CHAT_CLIENT,
                                    client->desc,
                                    RELAY_COLOR_CHAT);
                }
            }
            break;
        }
    }

end:
    free (data_unescaped);
    free (id);
    free (command);
    weechat_string_free_split (argv);
    weechat_string_free_split (argv_eol);
}
