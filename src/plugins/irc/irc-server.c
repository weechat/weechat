/*
 * irc-server.c - I/O communication with IRC servers
 *
 * Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005-2010 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2012 Simon Arlott
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#include <sys/time.h>
#endif /* _WIN32 */
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#endif /* HAVE_GNUTLS */

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-server.h"
#include "irc-bar-item.h"
#include "irc-buffer.h"
#include "irc-channel.h"
#include "irc-color.h"
#include "irc-command.h"
#include "irc-config.h"
#include "irc-input.h"
#include "irc-message.h"
#include "irc-nick.h"
#include "irc-notify.h"
#include "irc-protocol.h"
#include "irc-raw.h"
#include "irc-redirect.h"
#include "irc-sasl.h"


struct t_irc_server *irc_servers = NULL;
struct t_irc_server *last_irc_server = NULL;

struct t_irc_message *irc_recv_msgq = NULL;
struct t_irc_message *irc_msgq_last_msg = NULL;

char *irc_server_sasl_fail_string[IRC_SERVER_NUM_SASL_FAIL] =
{ "continue", "reconnect", "disconnect" };

char *irc_server_options[IRC_SERVER_NUM_OPTIONS][2] =
{ { "addresses",            ""                    },
  { "proxy",                ""                    },
  { "ipv6",                 "on"                  },
  { "ssl",                  "off"                 },
  { "ssl_cert",             ""                    },
  { "ssl_priorities",       "NORMAL:-VERS-SSL3.0" },
  { "ssl_dhkey_size",       "2048"                },
  { "ssl_fingerprint",      ""                    },
  { "ssl_verify",           "on"                  },
  { "password",             ""                    },
  { "capabilities",         ""                    },
  { "sasl_mechanism",       "plain"               },
  { "sasl_username",        ""                    },
  { "sasl_password",        ""                    },
  { "sasl_key",             "",                   },
  { "sasl_timeout",         "15"                  },
  { "sasl_fail",            "continue"            },
  { "autoconnect",          "off"                 },
  { "autoreconnect",        "on"                  },
  { "autoreconnect_delay",  "10"                  },
  { "nicks",                ""                    },
  { "nicks_alternate",      "on"                  },
  { "username",             ""                    },
  { "realname",             ""                    },
  { "local_hostname",       ""                    },
  { "command",              ""                    },
  { "command_delay",        "0"                   },
  { "autojoin",             ""                    },
  { "autorejoin",           "off"                 },
  { "autorejoin_delay",     "30"                  },
  { "connection_timeout",   "60"                  },
  { "anti_flood_prio_high", "2"                   },
  { "anti_flood_prio_low",  "2"                   },
  { "away_check",           "0"                   },
  { "away_check_max_nicks", "25"                  },
  { "default_msg_kick",     ""                    },
  { "default_msg_part",     "WeeChat %v"          },
  { "default_msg_quit",     "WeeChat %v"          },
  { "notify",               ""                    },
};

char *irc_server_casemapping_string[IRC_SERVER_NUM_CASEMAPPING] =
{ "rfc1459", "strict-rfc1459", "ascii" };

char *irc_server_prefix_modes_default = "ov";
char *irc_server_prefix_chars_default = "@+";
char *irc_server_chanmodes_default    = "beI,k,l";

const char *irc_server_send_default_tags = NULL;  /* default tags when       */
                                                  /* sending a message       */

#ifdef HAVE_GNUTLS
gnutls_digest_algorithm_t irc_fingerprint_digest_algos[IRC_FINGERPRINT_NUM_ALGOS] =
{ GNUTLS_DIG_SHA1, GNUTLS_DIG_SHA256, GNUTLS_DIG_SHA512 };
char *irc_fingerprint_digest_algos_name[IRC_FINGERPRINT_NUM_ALGOS] =
{ "SHA-1", "SHA-256", "SHA-512" };
int irc_fingerprint_digest_algos_size[IRC_FINGERPRINT_NUM_ALGOS] =
{ 160, 256, 512 };
#endif /* HAVE_GNUTLS */


void irc_server_reconnect (struct t_irc_server *server);
void irc_server_free_data (struct t_irc_server *server);
void irc_server_autojoin_create_buffers (struct t_irc_server *server);


/*
 * Checks if a server pointer is valid.
 *
 * Returns:
 *   1: server exists
 *   0: server does not exist
 */

int
irc_server_valid (struct t_irc_server *server)
{
    struct t_irc_server *ptr_server;

    if (!server)
        return 0;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server == server)
            return 1;
    }

    /* server not found */
    return 0;
}

/*
 * Searches for a server by name.
 *
 * Returns pointer to server found, NULL if not found.
 */

struct t_irc_server *
irc_server_search (const char *server_name)
{
    struct t_irc_server *ptr_server;

    if (!server_name)
        return NULL;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (strcmp (ptr_server->name, server_name) == 0)
            return ptr_server;
    }

    /* server not found */
    return NULL;
}

/*
 * Searches for a server by name (case insensitive).
 *
 * Returns pointer to server found, NULL if not found.
 */

struct t_irc_server *
irc_server_casesearch (const char *server_name)
{
    struct t_irc_server *ptr_server;

    if (!server_name)
        return NULL;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (weechat_strcasecmp (ptr_server->name, server_name) == 0)
            return ptr_server;
    }

    /* server not found */
    return NULL;
}

/*
 * Searches for a server option name.
 *
 * Returns index of option in array "irc_server_option_string", -1 if not found.
 */

int
irc_server_search_option (const char *option_name)
{
    int i;

    if (!option_name)
        return -1;

    for (i = 0; i < IRC_SERVER_NUM_OPTIONS; i++)
    {
        if (weechat_strcasecmp (irc_server_options[i][0], option_name) == 0)
            return i;
    }

    /* server option not found */
    return -1;
}

/*
 * Searches for a casemapping.
 *
 * Returns index of casemapping in array "irc_server_casemapping_string", -1 if
 * not found.
 */

int
irc_server_search_casemapping (const char *casemapping)
{
    int i;

    for (i = 0; i < IRC_SERVER_NUM_CASEMAPPING; i++)
    {
        if (weechat_strcasecmp (irc_server_casemapping_string[i], casemapping) == 0)
            return i;
    }

    /* casemapping not found */
    return -1;
}

/*
 * Compares two strings on server (case insensitive, depends on casemapping).
 *
 * Returns:
 *   < 0: string1 < string2
 *     0: string1 == string2
 *   > 0: string1 > string2
 */

int
irc_server_strcasecmp (struct t_irc_server *server,
                       const char *string1, const char *string2)
{
    int casemapping, rc;

    casemapping = (server) ? server->casemapping : IRC_SERVER_CASEMAPPING_RFC1459;
    switch (casemapping)
    {
        case IRC_SERVER_CASEMAPPING_RFC1459:
            rc = weechat_strcasecmp_range (string1, string2, 30);
            break;
        case IRC_SERVER_CASEMAPPING_STRICT_RFC1459:
            rc = weechat_strcasecmp_range (string1, string2, 29);
            break;
        case IRC_SERVER_CASEMAPPING_ASCII:
            rc = weechat_strcasecmp (string1, string2);
            break;
        default:
            rc = weechat_strcasecmp_range (string1, string2, 30);
            break;
    }
    return rc;
}

/*
 * Compares two strings on server (case insensitive, depends on casemapping) for
 * max chars.
 *
 * Returns:
 *   < 0: string1 < string2
 *     0: string1 == string2
 *   > 0: string1 > string2
 */

int
irc_server_strncasecmp (struct t_irc_server *server,
                        const char *string1, const char *string2, int max)
{
    int casemapping, rc;

    casemapping = (server) ? server->casemapping : IRC_SERVER_CASEMAPPING_RFC1459;
    switch (casemapping)
    {
        case IRC_SERVER_CASEMAPPING_RFC1459:
            rc = weechat_strncasecmp_range (string1, string2, max, 30);
            break;
        case IRC_SERVER_CASEMAPPING_STRICT_RFC1459:
            rc = weechat_strncasecmp_range (string1, string2, max, 29);
            break;
        case IRC_SERVER_CASEMAPPING_ASCII:
            rc = weechat_strncasecmp (string1, string2, max);
            break;
        default:
            rc = weechat_strncasecmp_range (string1, string2, max, 30);
            break;
    }
    return rc;
}

/*
 * Checks if SASL is enabled on server.
 *
 * Returns:
 *   1: SASL is enabled
 *   0: SASL is disabled
 */

int
irc_server_sasl_enabled (struct t_irc_server *server)
{
    int sasl_mechanism, rc;
    char *sasl_username, *sasl_password;
    const char *sasl_key;

    sasl_mechanism = IRC_SERVER_OPTION_INTEGER(
        server, IRC_SERVER_OPTION_SASL_MECHANISM);
    sasl_username = weechat_string_eval_expression (
        IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_SASL_USERNAME),
        NULL, NULL, NULL);
    sasl_password = weechat_string_eval_expression (
        IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_SASL_PASSWORD),
        NULL, NULL, NULL);
    sasl_key = IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_SASL_KEY);

    /*
     * SASL is enabled if one of these conditions is true:
     * - mechanism is "external"
     * - mechanism is "ecdsa-nist256p-challenge" with username/key set
     * - another mechanism with username/password set
     */
    rc = ((sasl_mechanism == IRC_SASL_MECHANISM_EXTERNAL)
          || ((sasl_mechanism == IRC_SASL_MECHANISM_ECDSA_NIST256P_CHALLENGE)
              && sasl_username && sasl_username[0]
              && sasl_key && sasl_key[0])
          || (sasl_username && sasl_username[0]
              && sasl_password && sasl_password[0])) ? 1 : 0;

    if (sasl_username)
        free (sasl_username);
    if (sasl_password)
        free (sasl_password);

    return rc;
}

/*
 * Gets name of server without port (ends before first '/' if found).
 *
 * Note: result must be freed after use.
 */

char *
irc_server_get_name_without_port (const char *name)
{
    char *pos;

    if (!name)
        return NULL;

    pos = strchr (name, '/');
    if (pos && (pos != name))
        return weechat_strndup (name, pos - name);

    return strdup (name);
}

/*
 * Sets addresses for server.
 */

void
irc_server_set_addresses (struct t_irc_server *server, const char *addresses)
{
    int i;
    char *pos, *error, *addresses_eval;
    long number;

    /* free data */
    server->addresses_count = 0;
    if (server->addresses_array)
    {
        weechat_string_free_split (server->addresses_array);
        server->addresses_array = NULL;
    }
    if (server->ports_array)
    {
        free (server->ports_array);
        server->ports_array = NULL;
    }
    if (server->retry_array)
    {
        free (server->retry_array);
        server->retry_array = NULL;
    }

    /* set new addresses/ports */
    if (addresses && addresses[0])
    {
        addresses_eval = weechat_string_eval_expression (addresses,
                                                         NULL, NULL, NULL);
        server->addresses_array = weechat_string_split (
            (addresses_eval) ? addresses_eval : addresses,
            ",", 0, 0, &server->addresses_count);
        server->ports_array = malloc (
            server->addresses_count * sizeof (server->ports_array[0]));
        server->retry_array = malloc (
            server->addresses_count * sizeof (server->retry_array[0]));
        for (i = 0; i < server->addresses_count; i++)
        {
            pos = strchr (server->addresses_array[i], '/');
            if (pos)
            {
                pos[0] = 0;
                pos++;
                error = NULL;
                number = strtol (pos, &error, 10);
                server->ports_array[i] = (error && !error[0]) ?
                    number : IRC_SERVER_DEFAULT_PORT;
            }
            else
            {
                server->ports_array[i] = IRC_SERVER_DEFAULT_PORT;
            }
            server->retry_array[i] = 0;
        }
        if (addresses_eval)
            free (addresses_eval);
    }
}

/*
 * Sets index of current address for server.
 */

void
irc_server_set_index_current_address (struct t_irc_server *server, int index)
{
    if (server->current_address)
    {
        free (server->current_address);
        server->current_address = NULL;

        /* copy current retry value before loading next server */
        if (server->index_current_address < server->addresses_count)
            server->retry_array[server->index_current_address] = server->current_retry;
    }
    server->current_port = 0;
    server->current_retry = 0;

    if (server->addresses_count > 0)
    {
        index %= server->addresses_count;
        server->index_current_address = index;
        server->current_address = strdup (server->addresses_array[index]);
        server->current_port = server->ports_array[index];
        server->current_retry = server->retry_array[index];
    }
}

/*
 * Sets nicks for server.
 */

void
irc_server_set_nicks (struct t_irc_server *server, const char *nicks)
{
    char *nicks2;

    /* free data */
    server->nicks_count = 0;
    if (server->nicks_array)
    {
        weechat_string_free_split (server->nicks_array);
        server->nicks_array = NULL;
    }

    /* evaluate value */
    nicks2 = weechat_string_eval_expression (nicks, NULL, NULL, NULL);

    /* set new nicks */
    server->nicks_array = weechat_string_split (
        (nicks2) ? nicks2 : IRC_SERVER_DEFAULT_NICKS,
        ",", 0, 0, &server->nicks_count);

    if (nicks2)
        free (nicks2);
}

/*
 * Sets nickname for server.
 */

void
irc_server_set_nick (struct t_irc_server *server, const char *nick)
{
    struct t_irc_channel *ptr_channel;

    if (server->nick)
        free (server->nick);
    server->nick = (nick) ? strdup (nick) : NULL;

    /* set local variable "nick" for server and all channels/pv */
    weechat_buffer_set (server->buffer, "localvar_set_nick", nick);
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        weechat_buffer_set (ptr_channel->buffer, "localvar_set_nick", nick);
    }

    weechat_bar_item_update ("input_prompt");
}

/*
 * Gets index of nick in array "nicks_array".
 *
 * Returns index of nick in array, -1 if nick is not set or not found in
 * "nicks_array".
 */

int
irc_server_get_nick_index (struct t_irc_server *server)
{
    int i;

    if (!server->nick)
        return -1;

    for (i = 0; i < server->nicks_count; i++)
    {
        if (strcmp (server->nick, server->nicks_array[i]) == 0)
        {
            return i;
        }
    }

    /* nick not found */
    return -1;
}

/*
 * Gets an alternate nick when the nick is already used on server.
 *
 * First tries all declared nicks, then builds nicks by adding "_", until
 * length of 9.
 *
 * If all nicks are still used, builds 99 alternate nicks by using number at the
 * end.
 *
 * Example: nicks = "abcde,fghi,jkl"
 *          => nicks tried:  abcde
 *                          fghi
 *                          jkl
 *                          abcde_
 *                          abcde__
 *                          abcde___
 *                          abcde____
 *                          abcde___1
 *                          abcde___2
 *                          ...
 *                          abcde__99
 *
 * Returns NULL if no more alternate nick is available.
 */

const char *
irc_server_get_alternate_nick (struct t_irc_server *server)
{
    static char nick[64];
    char str_number[64];
    int nick_index, length_nick, length_number;

    nick[0] = '\0';

    /* we are still trying nicks from option "nicks" */
    if (server->nick_alternate_number < 0)
    {
        nick_index = irc_server_get_nick_index (server);
        if (nick_index < 0)
            nick_index = 0;
        else
        {
            nick_index = (nick_index + 1) % server->nicks_count;
            /* stop loop if first nick tried was not in the list of nicks */
            if ((nick_index == 0) && (server->nick_first_tried < 0))
                server->nick_first_tried = 0;
        }

        if (nick_index != server->nick_first_tried)
        {
            snprintf (nick, sizeof (nick),
                      "%s", server->nicks_array[nick_index]);
            return nick;
        }

        /* now we have tried all nicks in list */

        /* if alternate nicks are disabled, just return NULL */
        if (!IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_NICKS_ALTERNATE))
            return NULL;

        /* use main nick and we will add "_" and then number if needed */
        server->nick_alternate_number = 0;
        snprintf (nick, sizeof (nick), "%s", server->nicks_array[0]);
    }
    else
        snprintf (nick, sizeof (nick), "%s", server->nick);

    /* if length is < 9, just add a "_" */
    if (strlen (nick) < 9)
    {
        strcat (nick, "_");
        return nick;
    }

    server->nick_alternate_number++;

    /* number is max 99 */
    if (server->nick_alternate_number > 99)
        return NULL;

    /* be sure the nick has 9 chars max */
    nick[9] = '\0';

    /* generate number */
    snprintf (str_number, sizeof (str_number),
              "%d", server->nick_alternate_number);

    /* copy number in nick */
    length_nick = strlen (nick);
    length_number = strlen (str_number);
    if (length_number > length_nick)
        return NULL;
    memcpy (nick + length_nick - length_number, str_number, length_number);

    /* return alternate nick */
    return nick;
}

/*
 * Gets value of a feature item in "isupport" (copy of IRC message 005).
 *
 * Returns value of feature (empty string if feature has no value, NULL if
 * feature is not found).
 */

const char *
irc_server_get_isupport_value (struct t_irc_server *server, const char *feature)
{
    char feature2[64], *pos_feature, *pos_equal, *pos_space;
    int length;
    static char value[256];

    if (!server || !server->isupport || !feature)
        return NULL;

    /* search feature with value */
    snprintf (feature2, sizeof (feature2), " %s=", feature);
    pos_feature = strstr (server->isupport, feature2);
    if (pos_feature)
    {
        /* feature found with value, return value */
        pos_feature++;
        pos_equal = strchr (pos_feature, '=');
        pos_space = strchr (pos_feature, ' ');
        if (pos_space)
            length = pos_space - pos_equal - 1;
        else
            length = strlen (pos_equal) + 1;
        if (length > (int)sizeof (value) - 1)
            length = (int)sizeof (value) - 1;
        memcpy (value, pos_equal + 1, length);
        value[length] = '\0';
        return value;
    }

    /* search feature without value */
    feature2[strlen (feature2) - 1] = ' ';
    pos_feature = strstr (server->isupport, feature2);
    if (pos_feature)
    {
        value[0] = '\0';
        return value;
    }

    /* feature not found in isupport */
    return NULL;
}

/*
 * Sets "prefix_modes" and "prefix_chars" in server using value of PREFIX in IRC
 * message 005.
 *
 * For example, if prefix is "(ohv)@%+":
 *   prefix_modes is set to "ohv"
 *   prefix_chars is set to "@%+".
 */

void
irc_server_set_prefix_modes_chars (struct t_irc_server *server,
                                   const char *prefix)
{
    char *pos;
    int i, length_modes, length_chars;

    if (!server || !prefix)
        return;

    /* free previous values */
    if (server->prefix_modes)
    {
        free (server->prefix_modes);
        server->prefix_modes = NULL;
    }
    if (server->prefix_chars)
    {
        free (server->prefix_chars);
        server->prefix_chars = NULL;
    }

    /* assign new values */
    pos = strchr (prefix, ')');
    if (pos)
    {
        server->prefix_modes = weechat_strndup (prefix + 1,
                                                pos - prefix - 1);
        if (server->prefix_modes)
        {
            pos++;
            length_modes = strlen (server->prefix_modes);
            length_chars = strlen (pos);
            server->prefix_chars = malloc (length_modes + 1);
            if (server->prefix_chars)
            {
                for (i = 0; i < length_modes; i++)
                {
                    server->prefix_chars[i] = (i < length_chars) ? pos[i] : ' ';
                }
                server->prefix_chars[length_modes] = '\0';
            }
            else
            {
                free (server->prefix_modes);
                server->prefix_modes = NULL;
            }
        }
    }
}

/*
 * Gets prefix_modes for server (for example: "ohv").
 *
 * Returns default modes if prefix_modes is not set in server.
 */

const char *
irc_server_get_prefix_modes (struct t_irc_server *server)
{
    return (server && server->prefix_modes) ?
        server->prefix_modes : irc_server_prefix_modes_default;
}

/*
 * Gets prefix_chars for server (for example: "@%+").
 *
 * Returns default chars if prefix_chars is not set in server.
 */

const char *
irc_server_get_prefix_chars (struct t_irc_server *server)
{
    return (server && server->prefix_chars) ?
        server->prefix_chars : irc_server_prefix_chars_default;
}

/*
 * Gets index of mode in prefix_modes.
 *
 * The mode is for example 'o' or 'v'.
 *
 * Returns -1 if mode does not exist in server.
 */

int
irc_server_get_prefix_mode_index (struct t_irc_server *server, char mode)
{
    const char *prefix_modes;
    char *pos;

    if (server)
    {
        prefix_modes = irc_server_get_prefix_modes (server);
        pos = strchr (prefix_modes, mode);
        if (pos)
            return pos - prefix_modes;
    }

    return -1;
}

/*
 * Gets index of prefix_char in prefix_chars.
 *
 * The prefix char is for example '@' or '+'.
 *
 * Returns -1 if prefix_char does not exist in server.
 */

int
irc_server_get_prefix_char_index (struct t_irc_server *server,
                                  char prefix_char)
{
    const char *prefix_chars;
    char *pos;

    if (server)
    {
        prefix_chars = irc_server_get_prefix_chars (server);
        pos = strchr (prefix_chars, prefix_char);
        if (pos)
            return pos - prefix_chars;
    }

    return -1;
}

/*
 * Gets mode for prefix char.
 *
 * For example prefix_char '@' can return 'o'.
 *
 * Returns ' ' (space) if prefix char is not found.
 */

char
irc_server_get_prefix_mode_for_char (struct t_irc_server *server,
                                     char prefix_char)
{
    const char *prefix_modes;
    int index;

    if (server)
    {
        prefix_modes = irc_server_get_prefix_modes (server);
        index = irc_server_get_prefix_char_index (server, prefix_char);
        if (index >= 0)
            return prefix_modes[index];
    }

    return ' ';
}

/*
 * Gets prefix char for mode.
 *
 * For example mode 'o' can return '@'.
 *
 * Returns return ' ' (space) if mode is not found.
 */

char
irc_server_get_prefix_char_for_mode (struct t_irc_server *server, char mode)
{
    const char *prefix_chars;
    int index;

    if (server)
    {
        prefix_chars = irc_server_get_prefix_chars (server);
        index = irc_server_get_prefix_mode_index (server, mode);
        if (index >= 0)
            return prefix_chars[index];
    }

    return ' ';
}

/*
 * Gets chanmodes for server (for example: "eIb,k,l,imnpstS").
 *
 * Returns default chanmodes if chanmodes is not set in server.
 */

const char *
irc_server_get_chanmodes (struct t_irc_server *server)
{
    return (server && server->chanmodes) ?
        server->chanmodes : irc_server_chanmodes_default;
}

/*
 * Checks if a prefix char is valid for a status message
 * (message sent for example to ops/voiced).
 *
 * The prefix (for example '@' or '+') must be in STATUSMSG,
 * or in "prefix_chars" if STATUSMSG is not defined.
 *
 * Returns:
 *   1: prefix is valid for a status message
 *   0: prefix is NOT valid for a status message
 */

int
irc_server_prefix_char_statusmsg (struct t_irc_server *server,
                                  char prefix_char)
{
    const char *support_statusmsg;

    support_statusmsg = irc_server_get_isupport_value (server, "STATUSMSG");
    if (support_statusmsg)
        return (strchr (support_statusmsg, prefix_char)) ? 1 : 0;

    return (irc_server_get_prefix_char_index (server, prefix_char) >= 0) ?
        1 : 0;
}

/*
 * Allocates a new server and adds it to the servers queue.
 *
 * Returns pointer to new server, NULL if error.
 */

struct t_irc_server *
irc_server_alloc (const char *name)
{
    struct t_irc_server *new_server;
    int i, length;
    char *option_name;

    if (irc_server_casesearch (name))
        return NULL;

    /* alloc memory for new server */
    new_server = malloc (sizeof (*new_server));
    if (!new_server)
    {
        weechat_printf (NULL,
                        _("%s%s: error when allocating new server"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME);
        return NULL;
    }

    /* add new server to queue */
    new_server->prev_server = last_irc_server;
    new_server->next_server = NULL;
    if (irc_servers)
        last_irc_server->next_server = new_server;
    else
        irc_servers = new_server;
    last_irc_server = new_server;

    /* set name */
    new_server->name = strdup (name);

    /* internal vars */
    new_server->temp_server = 0;
    new_server->reloading_from_config = 0;
    new_server->reloaded_from_config = 0;
    new_server->addresses_count = 0;
    new_server->addresses_array = NULL;
    new_server->ports_array = NULL;
    new_server->retry_array = NULL;
    new_server->index_current_address = 0;
    new_server->current_address = NULL;
    new_server->current_ip = NULL;
    new_server->current_port = 0;
    new_server->current_retry = 0;
    new_server->sock = -1;
    new_server->hook_connect = NULL;
    new_server->hook_fd = NULL;
    new_server->hook_timer_connection = NULL;
    new_server->hook_timer_sasl = NULL;
    new_server->is_connected = 0;
    new_server->ssl_connected = 0;
    new_server->disconnected = 0;
    new_server->unterminated_message = NULL;
    new_server->nicks_count = 0;
    new_server->nicks_array = NULL;
    new_server->nick_first_tried = 0;
    new_server->nick_alternate_number = -1;
    new_server->nick = NULL;
    new_server->nick_modes = NULL;
    new_server->cap_away_notify = 0;
    new_server->cap_account_notify = 0;
    new_server->cap_extended_join = 0;
    new_server->isupport = NULL;
    new_server->prefix_modes = NULL;
    new_server->prefix_chars = NULL;
    new_server->nick_max_length = 0;
    new_server->casemapping = IRC_SERVER_CASEMAPPING_RFC1459;
    new_server->chantypes = NULL;
    new_server->chanmodes = NULL;
    new_server->monitor = 0;
    new_server->monitor_time = 0;
    new_server->reconnect_delay = 0;
    new_server->reconnect_start = 0;
    new_server->command_time = 0;
    new_server->reconnect_join = 0;
    new_server->disable_autojoin = 0;
    new_server->is_away = 0;
    new_server->away_message = NULL;
    new_server->away_time = 0;
    new_server->lag = 0;
    new_server->lag_displayed = -1;
    new_server->lag_check_time.tv_sec = 0;
    new_server->lag_check_time.tv_usec = 0;
    new_server->lag_next_check = time (NULL) +
        weechat_config_integer (irc_config_network_lag_check);
    new_server->lag_last_refresh = 0;
    new_server->cmd_list_regexp = NULL;
    new_server->last_user_message = 0;
    new_server->last_away_check = 0;
    new_server->last_data_purge = 0;
    for (i = 0; i < IRC_SERVER_NUM_OUTQUEUES_PRIO; i++)
    {
        new_server->outqueue[i] = NULL;
        new_server->last_outqueue[i] = NULL;
    }
    new_server->redirects = NULL;
    new_server->last_redirect = NULL;
    new_server->notify_list = NULL;
    new_server->last_notify = NULL;
    new_server->notify_count = 0;
    new_server->join_manual = weechat_hashtable_new (32,
                                                     WEECHAT_HASHTABLE_STRING,
                                                     WEECHAT_HASHTABLE_TIME,
                                                     NULL,
                                                     NULL);
    new_server->join_channel_key = weechat_hashtable_new (32,
                                                          WEECHAT_HASHTABLE_STRING,
                                                          WEECHAT_HASHTABLE_STRING,
                                                          NULL,
                                                          NULL);
    new_server->join_noswitch = weechat_hashtable_new (32,
                                                       WEECHAT_HASHTABLE_STRING,
                                                       WEECHAT_HASHTABLE_TIME,
                                                       NULL,
                                                       NULL);
    new_server->buffer = NULL;
    new_server->buffer_as_string = NULL;
    new_server->channels = NULL;
    new_server->last_channel = NULL;

    /* create options with null value */
    for (i = 0; i < IRC_SERVER_NUM_OPTIONS; i++)
    {
        length = strlen (new_server->name) + 1 +
            strlen (irc_server_options[i][0]) +
            512 +  /* inherited option name (irc.server_default.xxx) */
            1;
        option_name = malloc (length);
        if (option_name)
        {
            snprintf (option_name, length, "%s.%s << irc.server_default.%s",
                      new_server->name,
                      irc_server_options[i][0],
                      irc_server_options[i][0]);
            new_server->options[i] = irc_config_server_new_option (
                irc_config_file,
                irc_config_section_server,
                i,
                option_name,
                NULL,
                NULL,
                1,
                &irc_config_server_check_value_cb,
                irc_server_options[i][0],
                &irc_config_server_change_cb,
                irc_server_options[i][0]);
            irc_config_server_change_cb (irc_server_options[i][0],
                                         new_server->options[i]);
            free (option_name);
        }
    }

    return new_server;
}

/*
 * Initializes a server with URL of this form: irc://nick:pass@irc.toto.org:6667
 *
 * Returns pointer to new server, NULL if error.
 */

struct t_irc_server *
irc_server_alloc_with_url (const char *irc_url)
{
    char *irc_url2, *pos_server, *pos_nick, *pos_password;
    char *pos_address, *pos_port, *pos_channel, *pos;
    char *server_address, *server_nicks, *server_autojoin;
    char default_port[16];
    int ipv6, ssl, length;
    struct t_irc_server *ptr_server;

    irc_url2 = strdup (irc_url);
    if (!irc_url2)
        return NULL;

    pos_server = NULL;
    pos_nick = NULL;
    pos_password = NULL;
    pos_address = NULL;
    pos_port = NULL;
    pos_channel = NULL;

    ipv6 = 0;
    ssl = 0;
    snprintf (default_port, sizeof (default_port),
              "%d", IRC_SERVER_DEFAULT_PORT);

    pos_server = strstr (irc_url2, "://");
    if (!pos_server || !pos_server[3])
    {
        free (irc_url2);
        return NULL;
    }
    pos_server[0] = '\0';
    pos_server += 3;

    pos_channel = strstr (pos_server, "/");
    if (pos_channel)
    {
        pos_channel[0] = '\0';
        pos_channel++;
        while (pos_channel[0] == '/')
        {
            pos_channel++;
        }
    }

    /* check for SSL / IPv6 */
    if (weechat_strcasecmp (irc_url2, "irc6") == 0)
    {
        ipv6 = 1;
    }
    else if (weechat_strcasecmp (irc_url2, "ircs") == 0)
    {
        ssl = 1;
    }
    else if ((weechat_strcasecmp (irc_url2, "irc6s") == 0)
             || (weechat_strcasecmp (irc_url2, "ircs6") == 0))
    {
        ipv6 = 1;
        ssl = 1;
    }

    if (ssl)
    {
        snprintf (default_port, sizeof (default_port),
                  "%d", IRC_SERVER_DEFAULT_PORT_SSL);
    }

    /* search for nick, password, address+port */
    pos_address = strchr (pos_server, '@');
    if (pos_address)
    {
        pos_address[0] = '\0';
        pos_address++;
        pos_nick = pos_server;
        pos_password = strchr (pos_server, ':');
        if (pos_password)
        {
            pos_password[0] = '\0';
            pos_password++;
        }
    }
    else
        pos_address = pos_server;

    /*
     * search for port in address, and skip optional [ ] around address
     * (can be used to indicate IPv6 port, after ']')
     */
    if (pos_address[0] == '[')
    {
        pos_address++;
        pos = strchr (pos_address, ']');
        if (!pos)
        {
            free (irc_url2);
            return NULL;
        }
        pos[0] = '\0';
        pos++;
        pos_port = strchr (pos, ':');
        if (pos_port)
        {
            pos_port[0] = '\0';
            pos_port++;
        }
    }
    else
    {
        pos_port = strchr (pos_address, ':');
        if (pos_port)
        {
            pos_port[0] = '\0';
            pos_port++;
        }
    }

    ptr_server = irc_server_alloc (pos_address);
    if (ptr_server)
    {
        ptr_server->temp_server = 1;
        if (pos_address && pos_address[0])
        {
            length = strlen (pos_address) + 1 +
                ((pos_port) ? strlen (pos_port) : 16) + 1;
            server_address = malloc (length);
            if (server_address)
            {
                snprintf (server_address, length,
                          "%s/%s",
                          pos_address,
                          (pos_port && pos_port[0]) ? pos_port : default_port);
                weechat_config_option_set (
                    ptr_server->options[IRC_SERVER_OPTION_ADDRESSES],
                    server_address,
                    1);
                free (server_address);
            }
        }
        weechat_config_option_set (ptr_server->options[IRC_SERVER_OPTION_IPV6],
                                   (ipv6) ? "on" : "off",
                                   1);
        weechat_config_option_set (ptr_server->options[IRC_SERVER_OPTION_SSL],
                                   (ssl) ? "on" : "off",
                                   1);
        if (pos_nick && pos_nick[0])
        {
            length = ((strlen (pos_nick) + 2) * 5) + 1;
            server_nicks = malloc (length);
            if (server_nicks)
            {
                snprintf (server_nicks, length,
                          "%s,%s1,%s2,%s3,%s4",
                          pos_nick, pos_nick, pos_nick, pos_nick, pos_nick);
                weechat_config_option_set (
                    ptr_server->options[IRC_SERVER_OPTION_NICKS],
                    server_nicks,
                    1);
                free (server_nicks);
            }
        }
        if (pos_password && pos_password[0])
        {
            weechat_config_option_set (
                ptr_server->options[IRC_SERVER_OPTION_PASSWORD],
                pos_password,
                1);
        }
        weechat_config_option_set (
            ptr_server->options[IRC_SERVER_OPTION_AUTOCONNECT],
            "on",
            1);
        /* autojoin */
        if (pos_channel && pos_channel[0])
        {
            if (irc_channel_is_channel (ptr_server, pos_channel))
                server_autojoin = strdup (pos_channel);
            else
            {
                server_autojoin = malloc (strlen (pos_channel) + 2);
                if (server_autojoin)
                {
                    strcpy (server_autojoin, "#");
                    strcat (server_autojoin, pos_channel);
                }
            }
            if (server_autojoin)
            {
                weechat_config_option_set (
                    ptr_server->options[IRC_SERVER_OPTION_AUTOJOIN],
                    server_autojoin,
                    1);
                free (server_autojoin);
            }
        }
    }

    free (irc_url2);

    return ptr_server;
}

/*
 * Applies command line options to a server.
 *
 * For example: -ssl -nossl -password=test -proxy=myproxy
 */

void
irc_server_apply_command_line_options (struct t_irc_server *server,
                                       int argc, char **argv)
{
    int i, index_option;
    char *pos, *option_name, *ptr_value, *value_boolean[2] = { "off", "on" };

    for (i = 0; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            pos = strchr (argv[i], '=');
            if (pos)
            {
                option_name = weechat_strndup (argv[i] + 1, pos - argv[i] - 1);
                ptr_value = pos + 1;
            }
            else
            {
                option_name = strdup (argv[i] + 1);
                ptr_value = value_boolean[1];
            }
            if (option_name)
            {
                index_option = irc_server_search_option (option_name);
                if (index_option < 0)
                {
                    /* look if option is negative, like "-noxxx" */
                    if (weechat_strncasecmp (argv[i], "-no", 3) == 0)
                    {
                        free (option_name);
                        option_name = strdup (argv[i] + 3);
                        index_option = irc_server_search_option (option_name);
                        ptr_value = value_boolean[0];
                    }
                }
                if (index_option >= 0)
                {
                    weechat_config_option_set (server->options[index_option],
                                               ptr_value, 1);
                }
                free (option_name);
            }
        }
    }
}

/*
 * Adds a message in out queue.
 */

void
irc_server_outqueue_add (struct t_irc_server *server, int priority,
                         const char *command, const char *msg1,
                         const char *msg2, int modified, const char *tags,
                         struct t_irc_redirect *redirect)
{
    struct t_irc_outqueue *new_outqueue;

    new_outqueue = malloc (sizeof (*new_outqueue));
    if (new_outqueue)
    {
        new_outqueue->command = (command) ? strdup (command) : strdup ("unknown");
        new_outqueue->message_before_mod = (msg1) ? strdup (msg1) : NULL;
        new_outqueue->message_after_mod = (msg2) ? strdup (msg2) : NULL;
        new_outqueue->modified = modified;
        new_outqueue->tags = (tags) ? strdup (tags) : NULL;
        new_outqueue->redirect = redirect;

        new_outqueue->prev_outqueue = server->last_outqueue[priority];
        new_outqueue->next_outqueue = NULL;
        if (server->outqueue[priority])
            server->last_outqueue[priority]->next_outqueue = new_outqueue;
        else
            server->outqueue[priority] = new_outqueue;
        server->last_outqueue[priority] = new_outqueue;
    }
}

/*
 * Frees a message in out queue.
 */

void
irc_server_outqueue_free (struct t_irc_server *server,
                          int priority,
                          struct t_irc_outqueue *outqueue)
{
    struct t_irc_outqueue *new_outqueue;

    /* remove outqueue message */
    if (server->last_outqueue[priority] == outqueue)
        server->last_outqueue[priority] = outqueue->prev_outqueue;
    if (outqueue->prev_outqueue)
    {
        (outqueue->prev_outqueue)->next_outqueue = outqueue->next_outqueue;
        new_outqueue = server->outqueue[priority];
    }
    else
        new_outqueue = outqueue->next_outqueue;

    if (outqueue->next_outqueue)
        (outqueue->next_outqueue)->prev_outqueue = outqueue->prev_outqueue;

    /* free data */
    if (outqueue->command)
        free (outqueue->command);
    if (outqueue->message_before_mod)
        free (outqueue->message_before_mod);
    if (outqueue->message_after_mod)
        free (outqueue->message_after_mod);
    if (outqueue->tags)
        free (outqueue->tags);
    free (outqueue);

    /* set new head */
    server->outqueue[priority] = new_outqueue;
}

/*
 * Frees all messages in out queue.
 */

void
irc_server_outqueue_free_all (struct t_irc_server *server, int priority)
{
    while (server->outqueue[priority])
    {
        irc_server_outqueue_free (server, priority,
                                  server->outqueue[priority]);
    }
}

/*
 * Frees server data.
 */

void
irc_server_free_data (struct t_irc_server *server)
{
    int i;

    if (!server)
        return;

    /* free linked lists */
    for (i = 0; i < IRC_SERVER_NUM_OUTQUEUES_PRIO; i++)
    {
        irc_server_outqueue_free_all (server, i);
    }
    irc_redirect_free_all (server);
    irc_notify_free_all (server);
    irc_channel_free_all (server);

    /* free hashtables */
    weechat_hashtable_free (server->join_manual);
    weechat_hashtable_free (server->join_channel_key);
    weechat_hashtable_free (server->join_noswitch);

    /* free server data */
    for (i = 0; i < IRC_SERVER_NUM_OPTIONS; i++)
    {
        if (server->options[i])
            weechat_config_option_free (server->options[i]);
    }
    if (server->name)
        free (server->name);
    if (server->addresses_array)
        weechat_string_free_split (server->addresses_array);
    if (server->ports_array)
        free (server->ports_array);
    if (server->retry_array)
        free (server->retry_array);
    if (server->current_address)
        free (server->current_address);
    if (server->current_ip)
        free (server->current_ip);
    if (server->hook_connect)
        weechat_unhook (server->hook_connect);
    if (server->hook_fd)
        weechat_unhook (server->hook_fd);
    if (server->hook_timer_connection)
        weechat_unhook (server->hook_timer_connection);
    if (server->hook_timer_sasl)
        weechat_unhook (server->hook_timer_sasl);
    if (server->unterminated_message)
        free (server->unterminated_message);
    if (server->nicks_array)
        weechat_string_free_split (server->nicks_array);
    if (server->nick)
        free (server->nick);
    if (server->nick_modes)
        free (server->nick_modes);
    if (server->isupport)
        free (server->isupport);
    if (server->prefix_modes)
        free (server->prefix_modes);
    if (server->prefix_chars)
        free (server->prefix_chars);
    if (server->chantypes)
        free (server->chantypes);
    if (server->chanmodes)
        free (server->chanmodes);
    if (server->away_message)
        free (server->away_message);
    if (server->cmd_list_regexp)
    {
        regfree (server->cmd_list_regexp);
        free (server->cmd_list_regexp);
    }
    if (server->buffer_as_string)
        free (server->buffer_as_string);
}

/*
 * Frees a server and remove it from list of servers.
 */

void
irc_server_free (struct t_irc_server *server)
{
    struct t_irc_server *new_irc_servers;

    if (!server)
        return;

    /*
     * close server buffer (and all channels/privates)
     * (only if we are not in a /upgrade, because during upgrade we want to
     * keep connections and closing server buffer would disconnect from server)
     */
    if (server->buffer && !irc_signal_upgrade_received)
        weechat_buffer_close (server->buffer);

    /* remove server from queue */
    if (last_irc_server == server)
        last_irc_server = server->prev_server;
    if (server->prev_server)
    {
        (server->prev_server)->next_server = server->next_server;
        new_irc_servers = irc_servers;
    }
    else
        new_irc_servers = server->next_server;

    if (server->next_server)
        (server->next_server)->prev_server = server->prev_server;

    irc_server_free_data (server);
    free (server);
    irc_servers = new_irc_servers;
}

/*
 * Frees all servers.
 */

void
irc_server_free_all ()
{
    /* for each server in memory, remove it */
    while (irc_servers)
    {
        irc_server_free (irc_servers);
    }
}

/*
 * Copies a server.
 *
 * Returns pointer to new server, NULL if error.
 */

struct t_irc_server *
irc_server_copy (struct t_irc_server *server, const char *new_name)
{
    struct t_irc_server *new_server;
    struct t_infolist *infolist;
    char *mask, *pos;
    const char *option_name;
    int length, index_option;

    /* check if another server exists with this name */
    if (irc_server_casesearch (new_name))
        return NULL;

    new_server = irc_server_alloc (new_name);
    if (new_server)
    {
        /* duplicate options */
        length = 32 + strlen (server->name) + 1;
        mask = malloc (length);
        if (!mask)
            return 0;
        snprintf (mask, length, "irc.server.%s.*", server->name);
        infolist = weechat_infolist_get ("option", NULL, mask);
        free (mask);
        if (infolist)
        {
            while (weechat_infolist_next (infolist))
            {
                if (!weechat_infolist_integer (infolist, "value_is_null"))
                {
                    option_name = weechat_infolist_string (infolist,
                                                           "option_name");
                    pos = strrchr (option_name, '.');
                    if (pos)
                    {
                        index_option = irc_server_search_option (pos + 1);
                        if (index_option >= 0)
                        {
                            weechat_config_option_set (
                                new_server->options[index_option],
                                weechat_infolist_string (infolist, "value"),
                                1);
                        }
                    }
                }
            }
            weechat_infolist_free (infolist);
        }
    }

    return new_server;
}

/*
 * Renames a server (internal name).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_server_rename (struct t_irc_server *server, const char *new_name)
{
    int length;
    char *mask, *pos_option, *new_option_name, charset_modifier[256];
    const char *buffer_name, *option_name;
    struct t_infolist *infolist;
    struct t_config_option *ptr_option;
    struct t_irc_channel *ptr_channel;

    /* check if another server exists with this name */
    if (irc_server_casesearch (new_name))
        return 0;

    /* rename options */
    length = 32 + strlen (server->name) + 1;
    mask = malloc (length);
    if (!mask)
        return 0;
    snprintf (mask, length, "irc.server.%s.*", server->name);
    infolist = weechat_infolist_get ("option", NULL, mask);
    free (mask);
    if (infolist)
    {
        while (weechat_infolist_next (infolist))
        {
            ptr_option = weechat_config_get (
                weechat_infolist_string (infolist, "full_name"));
            if (ptr_option)
            {
                option_name = weechat_infolist_string (infolist, "option_name");
                if (option_name)
                {
                    pos_option = strrchr (option_name, '.');
                    if (pos_option)
                    {
                        pos_option++;
                        length = strlen (new_name) + 1 + strlen (pos_option) + 1;
                        new_option_name = malloc (length);
                        if (new_option_name)
                        {
                            snprintf (new_option_name, length,
                                      "%s.%s", new_name, pos_option);
                            weechat_config_option_rename (ptr_option, new_option_name);
                            free (new_option_name);
                        }
                    }
                }
            }
        }
        weechat_infolist_free (infolist);
    }

    /* rename server */
    if (server->name)
        free (server->name);
    server->name = strdup (new_name);

    /* change name and local variables on buffers */
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ptr_channel->buffer)
        {
            buffer_name = irc_buffer_build_name (server->name,
                                                 ptr_channel->name);
            weechat_buffer_set (ptr_channel->buffer, "name", buffer_name);
            weechat_buffer_set (ptr_channel->buffer, "localvar_set_server",
                                server->name);
        }
    }
    if (server->buffer)
    {
        buffer_name = irc_buffer_build_name (server->name, NULL);
        weechat_buffer_set (server->buffer, "name", buffer_name);
        weechat_buffer_set (server->buffer, "short_name", server->name);
        weechat_buffer_set (server->buffer, "localvar_set_server",
                            server->name);
        weechat_buffer_set (server->buffer, "localvar_set_channel",
                            server->name);
        snprintf (charset_modifier, sizeof (charset_modifier),
                  "irc.%s", server->name);
        weechat_buffer_set (server->buffer, "localvar_set_charset_modifier",
                            charset_modifier);
    }

    return 1;
}

/*
 * Reorders list of servers.
 *
 * Returns the number of servers moved in the list (>= 0).
 */

int
irc_server_reorder (const char **servers, int num_servers)
{
    struct t_irc_server *ptr_server, *ptr_server2;
    int i, num_moved;

    ptr_server = irc_servers;
    num_moved = 0;

    for (i = 0; ptr_server && (i < num_servers); i++)
    {
        for (ptr_server2 = ptr_server; ptr_server2;
             ptr_server2 = ptr_server2->next_server)
        {
            if (strcmp (ptr_server2->name, servers[i]) == 0)
                break;
        }
        if (ptr_server2 == ptr_server)
        {
            ptr_server = ptr_server->next_server;
        }
        else  if (ptr_server2)
        {
            /* extract server from list */
            if (ptr_server2 == irc_servers)
                irc_servers = ptr_server2->next_server;
            if (ptr_server2 == last_irc_server)
                last_irc_server = ptr_server2->prev_server;
            if (ptr_server2->prev_server)
                (ptr_server2->prev_server)->next_server = ptr_server2->next_server;
            if (ptr_server2->next_server)
                (ptr_server2->next_server)->prev_server = ptr_server2->prev_server;

            /* set pointers in ptr_server2 */
            ptr_server2->prev_server = ptr_server->prev_server;
            ptr_server2->next_server = ptr_server;

            /* insert ptr_server2 before ptr_server */
            if (ptr_server->prev_server)
                (ptr_server->prev_server)->next_server = ptr_server2;
            ptr_server->prev_server = ptr_server2;

            /* adjust list of servers if needed */
            if (ptr_server == irc_servers)
                irc_servers = ptr_server2;

            num_moved++;
        }
    }

    return num_moved;
}

/*
 * Sends a signal for an IRC message (received or sent).
 */

void
irc_server_send_signal (struct t_irc_server *server, const char *signal,
                        const char *command, const char *full_message,
                        const char *tags)
{
    int length;
    char *str_signal, *full_message_tags;

    length = strlen (server->name) + 1 + strlen (signal) + 1 + strlen (command) + 1;
    str_signal = malloc (length);
    if (str_signal)
    {
        snprintf (str_signal, length,
                  "%s,%s_%s", server->name, signal, command);
        if (tags)
        {
            length = strlen (tags) + 1 + strlen (full_message) + 1;
            full_message_tags = malloc (length);
            if (full_message_tags)
            {
                snprintf (full_message_tags, length,
                          "%s;%s", tags, full_message);
                (void) weechat_hook_signal_send (str_signal,
                                                 WEECHAT_HOOK_SIGNAL_STRING,
                                                 (void *)full_message_tags);
                free (full_message_tags);
            }
        }
        else
        {
            (void) weechat_hook_signal_send (str_signal,
                                             WEECHAT_HOOK_SIGNAL_STRING,
                                             (void *)full_message);
        }
        free (str_signal);
    }
}

/*
 * Sends data to IRC server.
 *
 * Returns number of bytes sent, -1 if error.
 */

int
irc_server_send (struct t_irc_server *server, const char *buffer, int size_buf)
{
    int rc;

    if (!server)
    {
        weechat_printf (
            NULL,
            _("%s%s: sending data to server: null pointer (please report "
              "problem to developers)"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME);
        return 0;
    }

    if (size_buf <= 0)
    {
        weechat_printf (
            server->buffer,
            _("%s%s: sending data to server: empty buffer (please report "
              "problem to developers)"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME);
        return 0;
    }

#ifdef HAVE_GNUTLS
    if (server->ssl_connected)
        rc = gnutls_record_send (server->gnutls_sess, buffer, size_buf);
    else
#endif /* HAVE_GNUTLS */
        rc = send (server->sock, buffer, size_buf, 0);

    if (rc < 0)
    {
#ifdef HAVE_GNUTLS
        if (server->ssl_connected)
        {
            weechat_printf (
                server->buffer,
                _("%s%s: sending data to server: error %d %s"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                rc, gnutls_strerror (rc));
        }
        else
#endif /* HAVE_GNUTLS */
        {
            weechat_printf (
                server->buffer,
                _("%s%s: sending data to server: error %d %s"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                errno, strerror (errno));
        }
    }

    return rc;
}

/*
 * Sets default tags used when sending message.
 */

void
irc_server_set_send_default_tags (const char *tags)
{
    irc_server_send_default_tags = tags;
}

/*
 * Gets tags to send by concatenation of tags and irc_server_send_default_tags
 * (if set).
 *
 * Note: result must be freed after use.
 */

char *
irc_server_get_tags_to_send (const char *tags)
{
    int length;
    char *buf;

    if (!tags && !irc_server_send_default_tags)
        return NULL;

    if (!tags)
        return strdup (irc_server_send_default_tags);

    if (!irc_server_send_default_tags)
        return strdup (tags);

    /* concatenate tags and irc_server_send_default_tags */
    length = strlen (tags) + 1 + strlen (irc_server_send_default_tags) + 1;
    buf = malloc (length);
    if (buf)
        snprintf (buf, length, "%s,%s", tags, irc_server_send_default_tags);
    return buf;
}

/*
 * Sends a message from out queue.
 */

void
irc_server_outqueue_send (struct t_irc_server *server)
{
    time_t time_now;
    char *pos, *tags_to_send;
    int priority, anti_flood;

    time_now = time (NULL);

    /* detect if system clock has been changed (now lower than before) */
    if (server->last_user_message > time_now)
        server->last_user_message = time_now;

    for (priority = 0; priority < IRC_SERVER_NUM_OUTQUEUES_PRIO; priority++)
    {
        switch (priority)
        {
            case 0:
                anti_flood = IRC_SERVER_OPTION_INTEGER(
                    server, IRC_SERVER_OPTION_ANTI_FLOOD_PRIO_HIGH);
                break;
            default:
                anti_flood = IRC_SERVER_OPTION_INTEGER(
                    server, IRC_SERVER_OPTION_ANTI_FLOOD_PRIO_LOW);
                break;
        }
        if (server->outqueue[priority]
            && (time_now >= server->last_user_message + anti_flood))
        {
            if (server->outqueue[priority]->message_before_mod)
            {
                pos = strchr (server->outqueue[priority]->message_before_mod,
                              '\r');
                if (pos)
                    pos[0] = '\0';
                irc_raw_print (server, IRC_RAW_FLAG_SEND,
                               server->outqueue[priority]->message_before_mod);
                if (pos)
                    pos[0] = '\r';
            }
            if (server->outqueue[priority]->message_after_mod)
            {
                pos = strchr (server->outqueue[priority]->message_after_mod,
                              '\r');
                if (pos)
                    pos[0] = '\0';
                irc_raw_print (server, IRC_RAW_FLAG_SEND |
                               ((server->outqueue[priority]->modified) ? IRC_RAW_FLAG_MODIFIED : 0),
                               server->outqueue[priority]->message_after_mod);
                if (pos)
                    pos[0] = '\r';

                /* send signal with command that will be sent to server */
                irc_server_send_signal (
                    server, "irc_out",
                    server->outqueue[priority]->command,
                    server->outqueue[priority]->message_after_mod,
                    NULL);
                tags_to_send = irc_server_get_tags_to_send (
                    server->outqueue[priority]->tags);
                irc_server_send_signal (
                    server, "irc_outtags",
                    server->outqueue[priority]->command,
                    server->outqueue[priority]->message_after_mod,
                    (tags_to_send) ? tags_to_send : "");
                if (tags_to_send)
                    free (tags_to_send);

                /* send command */
                irc_server_send (
                    server, server->outqueue[priority]->message_after_mod,
                    strlen (server->outqueue[priority]->message_after_mod));
                server->last_user_message = time_now;

                /* start redirection if redirect is set */
                if (server->outqueue[priority]->redirect)
                {
                    irc_redirect_init_command (
                        server->outqueue[priority]->redirect,
                        server->outqueue[priority]->message_after_mod);
                }
            }
            irc_server_outqueue_free (server, priority,
                                      server->outqueue[priority]);
            break;
        }
    }
}

/*
 * Sends one message to IRC server.
 *
 * If flag contains outqueue priority value, then messages are in a queue and
 * sent slowly (to be sure there will not be any "excess flood"), value of
 * queue_msg is priority:
 *   1 = higher priority, for user messages
 *   2 = lower priority, for other messages (like auto reply to CTCP queries)
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_server_send_one_msg (struct t_irc_server *server, int flags,
                         const char *message, const char *nick,
                         const char *command, const char *channel,
                         const char *tags)
{
    static char buffer[4096];
    const char *ptr_msg, *ptr_chan_nick;
    char *new_msg, *pos, *tags_to_send, *msg_encoded;
    char str_modifier[128], modifier_data[256];
    int rc, queue_msg, add_to_queue, first_message, anti_flood;
    time_t time_now;
    struct t_irc_redirect *ptr_redirect;

    rc = 1;

    /* run modifier "irc_out_xxx" */
    snprintf (str_modifier, sizeof (str_modifier),
              "irc_out_%s",
              (command) ? command : "unknown");
    new_msg = weechat_hook_modifier_exec (str_modifier,
                                          server->name,
                                          message);

    /* no changes in new message */
    if (new_msg && (strcmp (message, new_msg) == 0))
    {
        free (new_msg);
        new_msg = NULL;
    }

    /* message not dropped? */
    if (!new_msg || new_msg[0])
    {
        first_message = 1;
        ptr_msg = (new_msg) ? new_msg : message;

        msg_encoded = NULL;
        ptr_chan_nick = (channel) ? channel : nick;
        if (ptr_chan_nick)
        {
            snprintf (modifier_data, sizeof (modifier_data),
                      "%s.%s.%s",
                      weechat_plugin->name,
                      server->name,
                      ptr_chan_nick);
        }
        else
        {
            snprintf (modifier_data, sizeof (modifier_data),
                      "%s.%s",
                      weechat_plugin->name,
                      server->name);
        }
        msg_encoded = irc_message_convert_charset (
                ptr_msg,
                "charset_encode",
                modifier_data);

        if (msg_encoded)
            ptr_msg = msg_encoded;

        while (rc && ptr_msg && ptr_msg[0])
        {
            pos = strchr (ptr_msg, '\n');
            if (pos)
                pos[0] = '\0';

            snprintf (buffer, sizeof (buffer), "%s\r\n", ptr_msg);

            /* anti-flood: look whether we should queue outgoing message or not */
            time_now = time (NULL);

            /* detect if system clock has been changed (now lower than before) */
            if (server->last_user_message > time_now)
                server->last_user_message = time_now;

            /* get queue from flags */
            queue_msg = 0;
            if (flags & IRC_SERVER_SEND_OUTQ_PRIO_HIGH)
                queue_msg = 1;
            else if (flags & IRC_SERVER_SEND_OUTQ_PRIO_LOW)
                queue_msg = 2;

            switch (queue_msg - 1)
            {
                case 0:
                    anti_flood = IRC_SERVER_OPTION_INTEGER(
                        server, IRC_SERVER_OPTION_ANTI_FLOOD_PRIO_HIGH);
                    break;
                default:
                    anti_flood = IRC_SERVER_OPTION_INTEGER(
                        server, IRC_SERVER_OPTION_ANTI_FLOOD_PRIO_LOW);
                    break;
            }

            add_to_queue = 0;
            if ((queue_msg > 0)
                && (server->outqueue[queue_msg - 1]
                    || ((anti_flood > 0)
                        && (time_now - server->last_user_message < anti_flood))))
            {
                add_to_queue = queue_msg;
            }

            tags_to_send = irc_server_get_tags_to_send (tags);

            ptr_redirect = irc_redirect_search_available (server);

            if (add_to_queue > 0)
            {
                /* queue message (do not send anything now) */
                irc_server_outqueue_add (server, add_to_queue - 1, command,
                                         (new_msg && first_message) ? message : NULL,
                                         buffer,
                                         (new_msg) ? 1 : 0,
                                         tags_to_send,
                                         ptr_redirect);
                /* mark redirect as "used" */
                if (ptr_redirect)
                    ptr_redirect->assigned_to_command = 1;
            }
            else
            {
                if (first_message)
                {
                    irc_raw_print (server, IRC_RAW_FLAG_SEND,
                                   (msg_encoded) ? msg_encoded : message);
                }
                if (new_msg)
                {
                    irc_raw_print (server,
                                   IRC_RAW_FLAG_SEND | IRC_RAW_FLAG_MODIFIED,
                                   ptr_msg);
                }

                /* send signal with command that will be sent to server */
                irc_server_send_signal (server, "irc_out",
                                        (command) ? command : "unknown",
                                        ptr_msg,
                                        NULL);
                irc_server_send_signal (server, "irc_outtags",
                                        (command) ? command : "unknown",
                                        ptr_msg,
                                        (tags_to_send) ? tags_to_send : "");

                if (irc_server_send (server, buffer, strlen (buffer)) <= 0)
                    rc = 0;
                else
                {
                    if (queue_msg > 0)
                        server->last_user_message = time_now;
                }
                if (ptr_redirect)
                    irc_redirect_init_command (ptr_redirect, buffer);
            }

            if (tags_to_send)
                    free (tags_to_send);

            if (pos)
            {
                pos[0] = '\n';
                ptr_msg = pos + 1;
            }
            else
                ptr_msg = NULL;

            first_message = 0;
        }
        if (msg_encoded)
            free (msg_encoded);
    }
    else
    {
        irc_raw_print (server, IRC_RAW_FLAG_SEND | IRC_RAW_FLAG_MODIFIED,
                       _("(message dropped)"));
    }

    if (new_msg)
        free (new_msg);

    return rc;
}

/*
 * Sends formatted data to IRC server.
 *
 * Many messages may be sent, separated by '\n'.
 *
 * If flags contains "IRC_SERVER_SEND_RETURN_HASHTABLE", then a hashtable with
 * split of message is returned (see function irc_message_split() in
 * irc-message.c)
 *
 * Note: hashtable must be freed after use.
 */

struct t_hashtable *
irc_server_sendf (struct t_irc_server *server, int flags, const char *tags,
                  const char *format, ...)
{
    char **items, hash_key[32], value[32], *nick, *command, *channel, *new_msg;
    char str_modifier[128];
    const char *str_message, *str_args;
    int i, items_count, number, ret_number, rc;
    struct t_hashtable *hashtable, *ret_hashtable;

    if (!server)
        return NULL;

    weechat_va_format (format);
    if (!vbuffer)
        return NULL;

    ret_hashtable = NULL;
    ret_number = 1;
    if (flags & IRC_SERVER_SEND_RETURN_HASHTABLE)
    {
        ret_hashtable = weechat_hashtable_new (32,
                                               WEECHAT_HASHTABLE_STRING,
                                               WEECHAT_HASHTABLE_STRING,
                                               NULL,
                                               NULL);
    }

    rc = 1;
    items = weechat_string_split (vbuffer, "\n", 0, 0, &items_count);
    for (i = 0; i < items_count; i++)
    {
        /* run modifier "irc_out1_xxx" (like "irc_out_xxx", but before split) */
        irc_message_parse (server, items[i], NULL, NULL,
                           &nick, NULL, &command, &channel, NULL, NULL, NULL,
                           NULL, NULL, NULL);
        snprintf (str_modifier, sizeof (str_modifier),
                  "irc_out1_%s",
                  (command) ? command : "unknown");
        new_msg = weechat_hook_modifier_exec (str_modifier,
                                              server->name,
                                              items[i]);

        /* no changes in new message */
        if (new_msg && (strcmp (items[i], new_msg) == 0))
        {
            free (new_msg);
            new_msg = NULL;
        }

        /* message not dropped? */
        if (!new_msg || new_msg[0])
        {
            /* send signal with command that will be sent to server (before split) */
            irc_server_send_signal (server, "irc_out1",
                                    (command) ? command : "unknown",
                                    (new_msg) ? new_msg : items[i],
                                    NULL);

            /* split message if needed (max is 512 bytes including final "\r\n") */
            hashtable = irc_message_split (server,
                                           (new_msg) ? new_msg : items[i]);
            if (hashtable)
            {
                number = 1;
                while (1)
                {
                    snprintf (hash_key, sizeof (hash_key), "msg%d", number);
                    str_message = weechat_hashtable_get (hashtable, hash_key);
                    if (!str_message)
                        break;
                    snprintf (hash_key, sizeof (hash_key), "args%d", number);
                    str_args = weechat_hashtable_get (hashtable, hash_key);

                    rc = irc_server_send_one_msg (server, flags, str_message,
                                                  nick, command, channel, tags);
                    if (!rc)
                        break;

                    if (ret_hashtable)
                    {
                        snprintf (hash_key, sizeof (hash_key),
                                  "msg%d", ret_number);
                        weechat_hashtable_set (ret_hashtable,
                                               hash_key, str_message);
                        if (str_args)
                        {
                            snprintf (hash_key, sizeof (hash_key),
                                      "args%d", ret_number);
                            weechat_hashtable_set (ret_hashtable,
                                                   hash_key, str_args);
                        }
                        ret_number++;
                    }
                    number++;
                }
                if (ret_hashtable)
                {
                    snprintf (value, sizeof (value), "%d", ret_number - 1);
                    weechat_hashtable_set (ret_hashtable, "count", value);
                }
                weechat_hashtable_free (hashtable);
                if (!rc)
                    break;
            }
        }
        if (nick)
            free (nick);
        if (command)
            free (command);
        if (channel)
            free (channel);
        if (new_msg)
            free (new_msg);
    }
    if (items)
        weechat_string_free_split (items);

    free (vbuffer);

    return ret_hashtable;
}

/*
 * Adds a message to received messages queue (at the end).
 */

void
irc_server_msgq_add_msg (struct t_irc_server *server, const char *msg)
{
    struct t_irc_message *message;

    if (!server->unterminated_message && !msg[0])
        return;

    message = malloc (sizeof (*message));
    if (!message)
    {
        weechat_printf (server->buffer,
                        _("%s%s: not enough memory for received message"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME);
        return;
    }
    message->server = server;
    if (server->unterminated_message)
    {
        message->data = malloc (strlen (server->unterminated_message) +
                                strlen (msg) + 1);
        if (!message->data)
        {
            weechat_printf (server->buffer,
                            _("%s%s: not enough memory for received message"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME);
        }
        else
        {
            strcpy (message->data, server->unterminated_message);
            strcat (message->data, msg);
        }
        free (server->unterminated_message);
        server->unterminated_message = NULL;
    }
    else
        message->data = strdup (msg);

    message->next_message = NULL;

    if (irc_msgq_last_msg)
    {
        irc_msgq_last_msg->next_message = message;
        irc_msgq_last_msg = message;
    }
    else
    {
        irc_recv_msgq = message;
        irc_msgq_last_msg = message;
    }
}

/*
 * Adds an unterminated message to queue.
 */

void
irc_server_msgq_add_unterminated (struct t_irc_server *server,
                                  const char *string)
{
    char *unterminated_message2;

    if (!string[0])
        return;

    if (server->unterminated_message)
    {
        unterminated_message2 =
            realloc (server->unterminated_message,
                     (strlen (server->unterminated_message) +
                      strlen (string) + 1));
        if (!unterminated_message2)
        {
            weechat_printf (server->buffer,
                            _("%s%s: not enough memory for received message"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME);
            free (server->unterminated_message);
            server->unterminated_message = NULL;
            return;
        }
        server->unterminated_message = unterminated_message2;
        strcat (server->unterminated_message, string);
    }
    else
    {
        server->unterminated_message = strdup (string);
        if (!server->unterminated_message)
        {
            weechat_printf (server->buffer,
                            _("%s%s: not enough memory for received message"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME);
        }
    }
}

/*
 * Splits received buffer, creating queued messages.
 */

void
irc_server_msgq_add_buffer (struct t_irc_server *server, const char *buffer)
{
    char *pos_cr, *pos_lf;

    while (buffer[0])
    {
        pos_cr = strchr (buffer, '\r');
        pos_lf = strchr (buffer, '\n');

        if (!pos_cr && !pos_lf)
        {
            /* no CR/LF found => add to unterminated and return */
            irc_server_msgq_add_unterminated (server, buffer);
            return;
        }

        if (pos_cr && ((!pos_lf) || (pos_lf > pos_cr)))
        {
            /* found '\r' first => ignore this char */
            pos_cr[0] = '\0';
            irc_server_msgq_add_unterminated (server, buffer);
            buffer = pos_cr + 1;
        }
        else
        {
            /* found: '\n' first => terminate message */
            pos_lf[0] = '\0';
            irc_server_msgq_add_msg (server, buffer);
            buffer = pos_lf + 1;
        }
    }
}

/*
 * Flushes message queue.
 */

void
irc_server_msgq_flush ()
{
    struct t_irc_message *next;
    char *ptr_data, *new_msg, *new_msg2, *ptr_msg, *ptr_msg2, *ptr_msg3, *pos;
    char *tags, *nick, *host, *command, *channel, *arguments;
    char *msg_decoded, *msg_decoded_without_color;
    char str_modifier[128], modifier_data[256];

    while (irc_recv_msgq)
    {
        if (irc_recv_msgq->data)
        {
            /* read message only if connection was not lost */
            if (irc_recv_msgq->server->sock != -1)
            {
                ptr_data = irc_recv_msgq->data;
                while (ptr_data[0] == ' ')
                {
                    ptr_data++;
                }

                if (ptr_data[0])
                {
                    irc_raw_print (irc_recv_msgq->server, IRC_RAW_FLAG_RECV,
                                   ptr_data);

                    irc_message_parse (irc_recv_msgq->server,
                                       ptr_data, NULL, NULL, NULL, NULL,
                                       &command, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL);
                    snprintf (str_modifier, sizeof (str_modifier),
                              "irc_in_%s",
                              (command) ? command : "unknown");
                    new_msg = weechat_hook_modifier_exec (
                        str_modifier,
                        irc_recv_msgq->server->name,
                        ptr_data);
                    if (command)
                        free (command);

                    /* no changes in new message */
                    if (new_msg && (strcmp (ptr_data, new_msg) == 0))
                    {
                        free (new_msg);
                        new_msg = NULL;
                    }

                    /* message not dropped? */
                    if (!new_msg || new_msg[0])
                    {
                        /* use new message (returned by plugin) */
                        ptr_msg = (new_msg) ? new_msg : ptr_data;

                        while (ptr_msg && ptr_msg[0])
                        {
                            pos = strchr (ptr_msg, '\n');
                            if (pos)
                                pos[0] = '\0';

                            if (new_msg)
                            {
                                irc_raw_print (
                                    irc_recv_msgq->server,
                                    IRC_RAW_FLAG_RECV | IRC_RAW_FLAG_MODIFIED,
                                    ptr_msg);
                            }

                            msg_decoded = NULL;

                            /* convert charset for message */
                            snprintf (modifier_data,
                                      sizeof (modifier_data),
                                      "%s.%s",
                                      weechat_plugin->name,
                                      irc_recv_msgq->server->name);

                            msg_decoded = irc_message_convert_charset (
                                ptr_msg,
                                "charset_decode",
                                modifier_data);

                            irc_message_parse (irc_recv_msgq->server, ptr_msg,
                                               &tags, NULL, &nick, &host,
                                               &command, &channel, &arguments,
                                               NULL, NULL, NULL,
                                               NULL, NULL);


                            /* replace WeeChat internal color codes by "?" */
                            msg_decoded_without_color =
                                weechat_string_remove_color (
                                    (msg_decoded) ? msg_decoded : ptr_msg,
                                    "?");

                            /* call modifier after charset */
                            ptr_msg2 = (msg_decoded_without_color) ?
                                msg_decoded_without_color : ((msg_decoded) ? msg_decoded : ptr_msg);
                            snprintf (str_modifier, sizeof (str_modifier),
                                      "irc_in2_%s",
                                      (command) ? command : "unknown");
                            new_msg2 = weechat_hook_modifier_exec (
                                str_modifier,
                                irc_recv_msgq->server->name,
                                ptr_msg2);
                            if (new_msg2 && (strcmp (ptr_msg2, new_msg2) == 0))
                            {
                                free (new_msg2);
                                new_msg2 = NULL;
                            }

                            /* message not dropped? */
                            if (!new_msg2 || new_msg2[0])
                            {
                                /* use new message (returned by plugin) */
                                if (new_msg2)
                                    ptr_msg2 = new_msg2;

                                /* parse and execute command */
                                if (irc_redirect_message (irc_recv_msgq->server,
                                                          ptr_msg2, command,
                                                          arguments))
                                {
                                    /* message redirected, we'll not display it! */
                                }
                                else
                                {
                                    /* message not redirected, display it */
                                    ptr_msg3 = ptr_msg2;
                                    if (ptr_msg3[0] == '@')
                                    {
                                        /* skip tags in message */
                                        ptr_msg3 = strchr (ptr_msg3, ' ');
                                        if (ptr_msg3)
                                        {
                                            while (ptr_msg3[0] == ' ')
                                            {
                                                ptr_msg3++;
                                            }
                                        }
                                        else
                                            ptr_msg3 = ptr_msg2;
                                    }
                                    irc_protocol_recv_command (
                                        irc_recv_msgq->server,
                                        ptr_msg3,
                                        tags,
                                        command,
                                        channel);
                                }
                            }

                            if (new_msg2)
                                free (new_msg2);
                            if (nick)
                                free (nick);
                            if (host)
                                free (host);
                            if (command)
                                free (command);
                            if (channel)
                                free (channel);
                            if (arguments)
                                free (arguments);
                            if (msg_decoded)
                                free (msg_decoded);
                            if (msg_decoded_without_color)
                                free (msg_decoded_without_color);

                            if (pos)
                            {
                                pos[0] = '\n';
                                ptr_msg = pos + 1;
                            }
                            else
                                ptr_msg = NULL;
                        }
                    }
                    else
                    {
                        irc_raw_print (irc_recv_msgq->server,
                                       IRC_RAW_FLAG_RECV | IRC_RAW_FLAG_MODIFIED,
                                       _("(message dropped)"));
                    }
                    if (new_msg)
                        free (new_msg);
                }
            }
            free (irc_recv_msgq->data);
        }

        next = irc_recv_msgq->next_message;
        free (irc_recv_msgq);
        irc_recv_msgq = next;
        if (!irc_recv_msgq)
            irc_msgq_last_msg = NULL;
    }
}

/*
 * Receives data from a server.
 */

int
irc_server_recv_cb (void *data, int fd)
{
    struct t_irc_server *server;
    static char buffer[4096 + 2];
    int num_read, msgq_flush, end_recv;

    /* make C compiler happy */
    (void) fd;

    server = (struct t_irc_server *)data;
    if (!server)
        return WEECHAT_RC_ERROR;

    msgq_flush = 0;
    end_recv = 0;

    while (!end_recv)
    {
        end_recv = 1;

#ifdef HAVE_GNUTLS
        if (server->ssl_connected)
            num_read = gnutls_record_recv (server->gnutls_sess, buffer,
                                           sizeof (buffer) - 2);
        else
#endif /* HAVE_GNUTLS */
            num_read = recv (server->sock, buffer, sizeof (buffer) - 2, 0);

        if (num_read > 0)
        {
            buffer[num_read] = '\0';
            irc_server_msgq_add_buffer (server, buffer);
            msgq_flush = 1;  /* the flush will be done after the loop */
#ifdef HAVE_GNUTLS
            if (server->ssl_connected
                && (gnutls_record_check_pending (server->gnutls_sess) > 0))
            {
                /*
                 * if there are unread data in the gnutls buffers,
                 * go on with recv
                 */
                end_recv = 0;
            }
#endif /* HAVE_GNUTLS */
        }
        else
        {
#ifdef HAVE_GNUTLS
            if (server->ssl_connected)
            {
                if ((num_read == 0)
                    || ((num_read != GNUTLS_E_AGAIN)
                        && (num_read != GNUTLS_E_INTERRUPTED)))
                {
                    weechat_printf (
                        server->buffer,
                        _("%s%s: reading data on socket: error %d %s"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME,
                        num_read,
                        (num_read == 0) ? _("(connection closed by peer)") :
                        gnutls_strerror (num_read));
                    weechat_printf (
                        server->buffer,
                        _("%s%s: disconnecting from server..."),
                        weechat_prefix ("network"), IRC_PLUGIN_NAME);
                    irc_server_disconnect (server, !server->is_connected, 1);
                }
            }
            else
#endif /* HAVE_GNUTLS */
            {
                if ((num_read == 0)
                    || ((errno != EAGAIN) && (errno != EWOULDBLOCK)))
                {
                    weechat_printf (
                        server->buffer,
                        _("%s%s: reading data on socket: error %d %s"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME,
                        errno,
                        (num_read == 0) ? _("(connection closed by peer)") :
                        strerror (errno));
                    weechat_printf (
                        server->buffer,
                        _("%s%s: disconnecting from server..."),
                        weechat_prefix ("network"), IRC_PLUGIN_NAME);
                    irc_server_disconnect (server, !server->is_connected, 1);
                }
            }
        }
    }

    if (msgq_flush)
        irc_server_msgq_flush ();

    return WEECHAT_RC_OK;
}

/*
 * Callback for server connection: it is called if WeeChat is TCP-connected to
 * server, but did not receive message 001.
 */

int
irc_server_timer_connection_cb (void *data, int remaining_calls)
{
    struct t_irc_server *server;

    /* make C compiler happy */
    (void) remaining_calls;

    server = (struct t_irc_server *)data;

    if (!server)
        return WEECHAT_RC_ERROR;

    server->hook_timer_connection = NULL;

    if (!server->is_connected)
    {
        weechat_printf (
            server->buffer,
            _("%s%s: connection timeout (message 001 not received)"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME);
        irc_server_disconnect (server, !server->is_connected, 1);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for SASL authentication timer: it is called if there is a timeout
 * with SASL authentication (if SASL authentication is OK or failed, then hook
 * timer is removed before this callback is called).
 */

int
irc_server_timer_sasl_cb (void *data, int remaining_calls)
{
    struct t_irc_server *server;
    int sasl_fail;

    /* make C compiler happy */
    (void) remaining_calls;

    server = (struct t_irc_server *)data;

    if (!server)
        return WEECHAT_RC_ERROR;

    server->hook_timer_sasl = NULL;

    if (!server->is_connected)
    {
        weechat_printf (server->buffer,
                        _("%s%s: sasl authentication timeout"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME);
        sasl_fail = IRC_SERVER_OPTION_INTEGER(server,
                                              IRC_SERVER_OPTION_SASL_FAIL);
        if ((sasl_fail == IRC_SERVER_SASL_FAIL_RECONNECT)
            || (sasl_fail == IRC_SERVER_SASL_FAIL_DISCONNECT))
        {
            irc_server_disconnect (
                server, 0,
                (sasl_fail == IRC_SERVER_SASL_FAIL_RECONNECT) ? 1 : 0);
        }
        else
            irc_server_sendf (server, 0, NULL, "CAP END");
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback called for each manual join of a server: deletes old channels in the
 * hashtable.
 */

void
irc_server_check_join_manual_cb (void *data, struct t_hashtable *hashtable,
                                 const void *key, const void *value)
{
    /* make C compiler happy */
    (void) data;

    if (*((time_t *)value) + (60 * 10) < time (NULL))
        weechat_hashtable_remove (hashtable, key);
}

/*
 * Callback called for each join without switch of a server: deletes old channel
 * in the hashtable.
 */

void
irc_server_check_join_noswitch_cb (void *data, struct t_hashtable *hashtable,
                                   const void *key, const void *value)
{
    /* make C compiler happy */
    (void) data;

    if (*((time_t *)value) + (60 * 10) < time (NULL))
        weechat_hashtable_remove (hashtable, key);
}

/*
 * Callback called for each smart filtered join of a channel: deletes old
 * entries in the hashtable.
 */

void
irc_server_check_join_smart_filtered_cb (void *data,
                                         struct t_hashtable *hashtable,
                                         const void *key, const void *value)
{
    int unmask_delay;

    /* make C compiler happy */
    (void) data;

    unmask_delay = weechat_config_integer (irc_config_look_smart_filter_join_unmask);
    if ((unmask_delay == 0)
        || (*((time_t *)value) < time (NULL) - (unmask_delay * 60)))
    {
        weechat_hashtable_remove (hashtable, key);
    }
}

/*
 * Timer called each second to perform some operations on servers.
 */

int
irc_server_timer_cb (void *data, int remaining_calls)
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    struct t_irc_redirect *ptr_redirect, *ptr_next_redirect;
    time_t current_time;
    static struct timeval tv;
    int away_check;

    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    current_time = time (NULL);

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        /* check if reconnection is pending */
        if ((!ptr_server->is_connected)
            && (ptr_server->reconnect_start > 0)
            && (current_time >= (ptr_server->reconnect_start + ptr_server->reconnect_delay)))
        {
            irc_server_reconnect (ptr_server);
        }
        else
        {
            if (!ptr_server->is_connected)
                continue;

            /* send queued messages */
            irc_server_outqueue_send (ptr_server);

            /* check for lag */
            if ((weechat_config_integer (irc_config_network_lag_check) > 0)
                && (ptr_server->lag_check_time.tv_sec == 0)
                && (current_time >= ptr_server->lag_next_check))
            {
                irc_server_sendf (ptr_server, 0, NULL, "PING %s",
                                  (ptr_server->current_address) ?
                                  ptr_server->current_address : "weechat");
                gettimeofday (&(ptr_server->lag_check_time), NULL);
                ptr_server->lag = 0;
                ptr_server->lag_last_refresh = 0;
            }
            else
            {
                /* check away (only if lag check was not done) */
                away_check = IRC_SERVER_OPTION_INTEGER(
                    ptr_server, IRC_SERVER_OPTION_AWAY_CHECK);
                if (!ptr_server->cap_away_notify
                    && (away_check > 0)
                    && ((ptr_server->last_away_check == 0)
                        || (current_time >= ptr_server->last_away_check + (away_check * 60))))
                {
                    irc_server_check_away (ptr_server);
                }
            }

            /* check if it's time to autojoin channels (after command delay) */
            if ((ptr_server->command_time != 0)
                && (current_time >= ptr_server->command_time +
                    IRC_SERVER_OPTION_INTEGER(ptr_server, IRC_SERVER_OPTION_COMMAND_DELAY)))
            {
                irc_server_autojoin_channels (ptr_server);
                ptr_server->command_time = 0;
            }

            /* check if it's time to send MONITOR command */
            if ((ptr_server->monitor_time != 0)
                && (current_time >= ptr_server->monitor_time))
            {
                if (ptr_server->monitor > 0)
                    irc_notify_send_monitor (ptr_server);
                ptr_server->monitor_time = 0;
            }

            /* compute lag */
            if (ptr_server->lag_check_time.tv_sec != 0)
            {
                gettimeofday (&tv, NULL);
                ptr_server->lag = (int)(weechat_util_timeval_diff (&(ptr_server->lag_check_time),
                                                                   &tv) / 1000);
                /* refresh lag item if needed */
                if (((ptr_server->lag_last_refresh == 0)
                     || (current_time >= ptr_server->lag_last_refresh + weechat_config_integer (irc_config_network_lag_refresh_interval)))
                    && (ptr_server->lag >= weechat_config_integer (irc_config_network_lag_min_show)))
                {
                    ptr_server->lag_last_refresh = current_time;
                    if (ptr_server->lag != ptr_server->lag_displayed)
                    {
                        ptr_server->lag_displayed = ptr_server->lag;
                        weechat_bar_item_update ("lag");
                    }
                }
                /* lag timeout? => disconnect */
                if ((weechat_config_integer (irc_config_network_lag_reconnect) > 0)
                    && (ptr_server->lag >= weechat_config_integer (irc_config_network_lag_reconnect) * 1000))
                {
                    weechat_printf (
                        ptr_server->buffer,
                        _("%s%s: lag is high, reconnecting to server %s%s%s"),
                        weechat_prefix ("network"),
                        IRC_PLUGIN_NAME,
                        IRC_COLOR_CHAT_SERVER,
                        ptr_server->name,
                        IRC_COLOR_RESET);
                    irc_server_disconnect (ptr_server, 0, 1);
                }
                else
                {
                    /* stop lag counting if max lag is reached */
                    if ((weechat_config_integer (irc_config_network_lag_max) > 0)
                        && (ptr_server->lag >= (weechat_config_integer (irc_config_network_lag_max) * 1000)))
                    {
                        /* refresh lag item */
                        ptr_server->lag_last_refresh = current_time;
                        if (ptr_server->lag != ptr_server->lag_displayed)
                        {
                            ptr_server->lag_displayed = ptr_server->lag;
                            weechat_bar_item_update ("lag");
                        }

                        /* schedule next lag check in 5 seconds */
                        ptr_server->lag_check_time.tv_sec = 0;
                        ptr_server->lag_check_time.tv_usec = 0;
                        ptr_server->lag_next_check = time (NULL) +
                            weechat_config_integer (irc_config_network_lag_check);
                    }
                }
            }

            /* remove redirects if timeout occurs */
            ptr_redirect = ptr_server->redirects;
            while (ptr_redirect)
            {
                ptr_next_redirect = ptr_redirect->next_redirect;

                if ((ptr_redirect->start_time > 0)
                    && (ptr_redirect->start_time + ptr_redirect->timeout < current_time))
                {
                    irc_redirect_stop (ptr_redirect, "timeout");
                }

                ptr_redirect = ptr_next_redirect;
            }

            /* purge some data (every 10 minutes) */
            if (current_time > ptr_server->last_data_purge + (60 * 10))
            {
                weechat_hashtable_map (ptr_server->join_manual,
                                       &irc_server_check_join_manual_cb,
                                       NULL);
                weechat_hashtable_map (ptr_server->join_noswitch,
                                       &irc_server_check_join_noswitch_cb,
                                       NULL);
                for (ptr_channel = ptr_server->channels; ptr_channel;
                     ptr_channel = ptr_channel->next_channel)
                {
                    if (ptr_channel->join_smart_filtered)
                    {
                        weechat_hashtable_map (ptr_channel->join_smart_filtered,
                                               &irc_server_check_join_smart_filtered_cb,
                                               NULL);
                    }
                }
                ptr_server->last_data_purge = current_time;
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Closes server connection.
 */

void
irc_server_close_connection (struct t_irc_server *server)
{
    int i;

    if (server->hook_timer_connection)
    {
        weechat_unhook (server->hook_timer_connection);
        server->hook_timer_connection = NULL;
    }

    if (server->hook_timer_sasl)
    {
        weechat_unhook (server->hook_timer_sasl);
        server->hook_timer_sasl = NULL;
    }

    if (server->hook_fd)
    {
        weechat_unhook (server->hook_fd);
        server->hook_fd = NULL;
    }

    if (server->hook_connect)
    {
        weechat_unhook (server->hook_connect);
        server->hook_connect = NULL;
    }
    else
    {
#ifdef HAVE_GNUTLS
        /* close SSL connection */
        if (server->ssl_connected)
        {
            if (server->sock != -1)
                gnutls_bye (server->gnutls_sess, GNUTLS_SHUT_WR);
            gnutls_deinit (server->gnutls_sess);
        }
#endif /* HAVE_GNUTLS */
    }
    if (server->sock != -1)
    {
#ifdef _WIN32
        closesocket (server->sock);
#else
        close (server->sock);
#endif /* _WIN32 */
        server->sock = -1;
    }

    /* free any pending message */
    if (server->unterminated_message)
    {
        free (server->unterminated_message);
        server->unterminated_message = NULL;
    }
    for (i = 0; i < IRC_SERVER_NUM_OUTQUEUES_PRIO; i++)
    {
        irc_server_outqueue_free_all (server, i);
    }

    /* remove all redirects */
    irc_redirect_free_all (server);

    /* remove all manual joins */
    weechat_hashtable_remove_all (server->join_manual);

    /* remove all keys for pending joins */
    weechat_hashtable_remove_all (server->join_channel_key);

    /* remove all keys for joins without switch */
    weechat_hashtable_remove_all (server->join_noswitch);

    /* server is now disconnected */
    server->is_connected = 0;
    server->ssl_connected = 0;
}

/*
 * Schedules reconnection on server.
 */

void
irc_server_reconnect_schedule (struct t_irc_server *server)
{
    int minutes, seconds;

    if (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_AUTORECONNECT))
    {
        /* growing reconnect delay */
        if (server->reconnect_delay == 0)
            server->reconnect_delay = IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AUTORECONNECT_DELAY);
        else
            server->reconnect_delay = server->reconnect_delay * weechat_config_integer (irc_config_network_autoreconnect_delay_growing);
        if ((weechat_config_integer (irc_config_network_autoreconnect_delay_max) > 0)
            && (server->reconnect_delay > weechat_config_integer (irc_config_network_autoreconnect_delay_max)))
            server->reconnect_delay = weechat_config_integer (irc_config_network_autoreconnect_delay_max);

        server->reconnect_start = time (NULL);

        minutes = server->reconnect_delay / 60;
        seconds = server->reconnect_delay % 60;
        if ((minutes > 0) && (seconds > 0))
        {
            weechat_printf (
                server->buffer,
                _("%s%s: reconnecting to server in %d %s, %d %s"),
                weechat_prefix ("network"),
                IRC_PLUGIN_NAME,
                minutes,
                NG_("minute", "minutes", minutes),
                seconds,
                NG_("second", "seconds", seconds));
        }
        else if (minutes > 0)
        {
            weechat_printf (
                server->buffer,
                _("%s%s: reconnecting to server in %d %s"),
                weechat_prefix ("network"),
                IRC_PLUGIN_NAME,
                minutes,
                NG_("minute", "minutes", minutes));
        }
        else
        {
            weechat_printf (
                server->buffer,
                _("%s%s: reconnecting to server in %d %s"),
                weechat_prefix ("network"),
                IRC_PLUGIN_NAME,
                seconds,
                NG_("second", "seconds", seconds));
        }
    }
    else
    {
        server->reconnect_delay = 0;
        server->reconnect_start = 0;
    }
}

/*
 * Logins to server.
 */

void
irc_server_login (struct t_irc_server *server)
{
    const char *capabilities;
    char *password, *username, *realname, *username2;

    password = weechat_string_eval_expression (
        IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_PASSWORD),
        NULL, NULL, NULL);
    username = weechat_string_eval_expression (
        IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_USERNAME),
        NULL, NULL, NULL);
    realname = weechat_string_eval_expression (
        IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_REALNAME),
        NULL, NULL, NULL);

    capabilities = IRC_SERVER_OPTION_STRING(
        server, IRC_SERVER_OPTION_CAPABILITIES);

    if (password && password[0])
    {
        irc_server_sendf (
            server, 0, NULL,
            "PASS %s%s",
            ((password[0] == ':') || (strchr (password, ' '))) ? ":" : "",
            password);
    }

    if (!server->nick)
    {
        irc_server_set_nick (server,
                             (server->nicks_array) ?
                             server->nicks_array[0] : "weechat");
        server->nick_first_tried = 0;
    }
    else
        server->nick_first_tried = irc_server_get_nick_index (server);

    server->nick_alternate_number = -1;

    if (irc_server_sasl_enabled (server) || (capabilities && capabilities[0]))
    {
        irc_server_sendf (server, 0, NULL, "CAP LS");
    }

    username2 = (username && username[0]) ?
        weechat_string_replace (username, " ", "_") : strdup ("weechat");
    irc_server_sendf (
        server, 0, NULL,
        "NICK %s\n"
        "USER %s 0 * :%s",
        server->nick,
        (username2) ? username2 : "weechat",
        (realname && realname[0]) ? realname : ((username2) ? username2 : "weechat"));
    if (username2)
        free (username2);

    if (server->hook_timer_connection)
        weechat_unhook (server->hook_timer_connection);
    server->hook_timer_connection = weechat_hook_timer (
        IRC_SERVER_OPTION_INTEGER (server, IRC_SERVER_OPTION_CONNECTION_TIMEOUT) * 1000,
        0, 1,
        &irc_server_timer_connection_cb,
        server);

    if (password)
        free (password);
    if (username)
        free (username);
    if (realname)
        free (realname);
}

/*
 * Switches address and tries another (called if connection failed with an
 * address/port).
 */

void
irc_server_switch_address (struct t_irc_server *server, int connection)
{
    if (server->addresses_count > 1)
    {
        irc_server_set_index_current_address (
            server,
            (server->index_current_address + 1) % server->addresses_count);
        weechat_printf (
            server->buffer,
            _("%s%s: switching address to %s/%d"),
            weechat_prefix ("network"),
            IRC_PLUGIN_NAME,
            server->current_address,
            server->current_port);
        if (connection)
        {
            if (server->index_current_address == 0)
                irc_server_reconnect_schedule (server);
            else
                irc_server_connect (server);
        }
    }
    else
    {
        if (connection)
            irc_server_reconnect_schedule (server);
    }
}

/*
 * Reads connection status.
 */

int
irc_server_connect_cb (void *data, int status, int gnutls_rc, int sock,
                       const char *error, const char *ip_address)
{
    struct t_irc_server *server;
    const char *proxy;

    server = (struct t_irc_server *)data;

    proxy = IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_PROXY);

    server->hook_connect = NULL;

    switch (status)
    {
        case WEECHAT_HOOK_CONNECT_OK:
            /* set socket and IP */
            server->sock = sock;
            if (server->current_ip)
                free (server->current_ip);
            server->current_ip = (ip_address) ? strdup (ip_address) : NULL;
            weechat_printf (
                server->buffer,
                _("%s%s: connected to %s/%d (%s)"),
                weechat_prefix ("network"),
                IRC_PLUGIN_NAME,
                server->current_address,
                server->current_port,
                (server->current_ip) ? server->current_ip : "?");
            server->hook_fd = weechat_hook_fd (server->sock,
                                               1, 0, 0,
                                               &irc_server_recv_cb,
                                               server);
            /* login to server */
            irc_server_login (server);
            break;
        case WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND:
            weechat_printf (
                server->buffer,
                (proxy && proxy[0]) ?
                _("%s%s: proxy address \"%s\" not found") :
                _("%s%s: address \"%s\" not found"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                server->current_address);
            if (error && error[0])
            {
                weechat_printf (
                    server->buffer,
                    _("%s%s: error: %s"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, error);
            }
            irc_server_close_connection (server);
            irc_server_switch_address (server, 1);
            break;
        case WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND:
            weechat_printf (
                server->buffer,
                (proxy && proxy[0]) ?
                _("%s%s: proxy IP address not found") :
                _("%s%s: IP address not found"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            if (error && error[0])
            {
                weechat_printf (
                    server->buffer,
                    _("%s%s: error: %s"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, error);
            }
            irc_server_close_connection (server);
            irc_server_switch_address (server, 1);
            break;
        case WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED:
            weechat_printf (
                server->buffer,
                (proxy && proxy[0]) ?
                _("%s%s: proxy connection refused") :
                _("%s%s: connection refused"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            if (error && error[0])
            {
                weechat_printf (
                    server->buffer,
                    _("%s%s: error: %s"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, error);
            }
            irc_server_close_connection (server);
            server->current_retry++;
            irc_server_switch_address (server, 1);
            break;
        case WEECHAT_HOOK_CONNECT_PROXY_ERROR:
            weechat_printf (
                server->buffer,
                _("%s%s: proxy fails to establish connection to server (check "
                  "username/password if used and if server address/port is "
                  "allowed by proxy)"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            if (error && error[0])
            {
                weechat_printf (
                    server->buffer,
                    _("%s%s: error: %s"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, error);
            }
            irc_server_close_connection (server);
            irc_server_switch_address (server, 1);
            break;
        case WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR:
            weechat_printf (
                server->buffer,
                _("%s%s: unable to set local hostname/IP"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            if (error && error[0])
            {
                weechat_printf (
                    server->buffer,
                    _("%s%s: error: %s"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, error);
            }
            irc_server_close_connection (server);
            irc_server_reconnect_schedule (server);
            break;
        case WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR:
            weechat_printf (
                server->buffer,
                _("%s%s: TLS init error"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            if (error && error[0])
            {
                weechat_printf (
                    server->buffer,
                    _("%s%s: error: %s"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, error);
            }
            irc_server_close_connection (server);
            server->current_retry++;
            irc_server_reconnect_schedule (server);
            break;
        case WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR:
            weechat_printf (
                server->buffer,
                _("%s%s: TLS handshake failed"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            if (error && error[0])
            {
                weechat_printf (
                    server->buffer,
                    _("%s%s: error: %s"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, error);
            }
#ifdef HAVE_GNUTLS
            if (gnutls_rc == GNUTLS_E_DH_PRIME_UNACCEPTABLE)
            {
                weechat_printf (
                    server->buffer,
                    _("%s%s: you should play with option "
                      "irc.server.%s.ssl_dhkey_size (current value is %d, try "
                      "a lower value like %d or %d)"),
                    weechat_prefix ("error"),
                    IRC_PLUGIN_NAME,
                    server->name,
                    IRC_SERVER_OPTION_INTEGER (
                        server, IRC_SERVER_OPTION_SSL_DHKEY_SIZE),
                    IRC_SERVER_OPTION_INTEGER (
                        server, IRC_SERVER_OPTION_SSL_DHKEY_SIZE) / 2,
                    IRC_SERVER_OPTION_INTEGER (
                        server, IRC_SERVER_OPTION_SSL_DHKEY_SIZE) / 4);
            }
#else
            (void) gnutls_rc;
#endif /* HAVE_GNUTLS */
            irc_server_close_connection (server);
            server->current_retry++;
            irc_server_switch_address (server, 1);
            break;
        case WEECHAT_HOOK_CONNECT_MEMORY_ERROR:
            weechat_printf (
                server->buffer,
                _("%s%s: not enough memory"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            if (error && error[0])
            {
                weechat_printf (
                    server->buffer,
                    _("%s%s: error: %s"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, error);
            }
            irc_server_close_connection (server);
            irc_server_reconnect_schedule (server);
            break;
        case WEECHAT_HOOK_CONNECT_TIMEOUT:
            weechat_printf (
                server->buffer,
                _("%s%s: timeout"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            if (error && error[0])
            {
                weechat_printf (
                    server->buffer,
                    _("%s%s: error: %s"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, error);
            }
            irc_server_close_connection (server);
            server->current_retry++;
            irc_server_switch_address (server, 1);
            break;
        case WEECHAT_HOOK_CONNECT_SOCKET_ERROR:
            weechat_printf (
                server->buffer,
                _("%s%s: unable to create socket"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            if (error && error[0])
            {
                weechat_printf (
                    server->buffer,
                    _("%s%s: error: %s"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, error);
            }
            irc_server_close_connection (server);
            server->current_retry++;
            irc_server_reconnect_schedule (server);
            break;
    }

    return WEECHAT_RC_OK;
}

/*
 * Sets the title for a server buffer.
 */

void
irc_server_set_buffer_title (struct t_irc_server *server)
{
    char *title;
    int length;

    if (server && server->buffer)
    {
        if (server->is_connected)
        {
            length = 16 +
                ((server->current_address) ? strlen (server->current_address) : 16) +
                16 + ((server->current_ip) ? strlen (server->current_ip) : 16) + 1;
            title = malloc (length);
            if (title)
            {
                snprintf (title, length, "IRC: %s/%d (%s)",
                          server->current_address,
                          server->current_port,
                          (server->current_ip) ? server->current_ip : "");
                weechat_buffer_set (server->buffer, "title", title);
                free (title);
            }
        }
        else
        {
            weechat_buffer_set (server->buffer, "title", "");
        }
    }
}

/*
 * Creates a buffer for a server.
 *
 * Returns pointer to buffer, NULL if error.
 */

struct t_gui_buffer *
irc_server_create_buffer (struct t_irc_server *server)
{
    char buffer_name[256], charset_modifier[256];
    struct t_gui_buffer *ptr_buffer_for_merge;

    ptr_buffer_for_merge = NULL;
    switch (weechat_config_integer (irc_config_look_server_buffer))
    {
        case IRC_CONFIG_LOOK_SERVER_BUFFER_MERGE_WITH_CORE:
            /* merge with WeeChat core buffer */
            ptr_buffer_for_merge = weechat_buffer_search_main ();
            break;
        case IRC_CONFIG_LOOK_SERVER_BUFFER_MERGE_WITHOUT_CORE:
            /* find buffer used to merge all IRC server buffers */
            ptr_buffer_for_merge = irc_buffer_search_server_lowest_number ();
            break;
    }

    snprintf (buffer_name, sizeof (buffer_name),
              "server.%s", server->name);
    server->buffer = weechat_buffer_new (buffer_name,
                                         &irc_input_data_cb, NULL,
                                         &irc_buffer_close_cb, NULL);
    if (!server->buffer)
        return NULL;

    if (!weechat_buffer_get_integer (server->buffer, "short_name_is_set"))
        weechat_buffer_set (server->buffer, "short_name", server->name);
    weechat_buffer_set (server->buffer, "localvar_set_type", "server");
    weechat_buffer_set (server->buffer, "localvar_set_server", server->name);
    weechat_buffer_set (server->buffer, "localvar_set_channel", server->name);
    snprintf (charset_modifier, sizeof (charset_modifier),
              "irc.%s", server->name);
    weechat_buffer_set (server->buffer, "localvar_set_charset_modifier",
                        charset_modifier);

    (void) weechat_hook_signal_send ("logger_backlog",
                                     WEECHAT_HOOK_SIGNAL_POINTER,
                                     server->buffer);

    if (weechat_config_boolean (irc_config_network_send_unknown_commands))
        weechat_buffer_set (server->buffer, "input_get_unknown_commands", "1");

    /* set highlights settings on server buffer */
    weechat_buffer_set (server->buffer, "highlight_words_add",
                        weechat_config_string (irc_config_look_highlight_server));
    if (weechat_config_string (irc_config_look_highlight_tags_restrict)
        && weechat_config_string (irc_config_look_highlight_tags_restrict)[0])
    {
        weechat_buffer_set (
            server->buffer, "highlight_tags_restrict",
            weechat_config_string (irc_config_look_highlight_tags_restrict));
    }

    irc_server_set_buffer_title (server);

    /*
     * merge buffer if needed: if merge with(out) core set, and if no layout
     * number is assigned for this buffer (if layout number is assigned, then
     * buffer was already moved/merged by WeeChat core)
     */
    if (ptr_buffer_for_merge
        && (weechat_buffer_get_integer (server->buffer, "layout_number") < 1))
    {
        weechat_buffer_merge (server->buffer, ptr_buffer_for_merge);
    }

    (void) weechat_hook_signal_send ("irc_server_opened",
                                     WEECHAT_HOOK_SIGNAL_POINTER,
                                     server->buffer);

    return server->buffer;
}

/*
 * Searches for a fingerprint digest algorithm with the size (in bits).
 *
 * Returns index of algo in enum t_irc_fingerprint_digest_algo,
 * -1 if not found.
 */

#ifdef HAVE_GNUTLS
int
irc_server_fingerprint_search_algo_with_size (int size)
{
    int i;

    for (i = 0; i < IRC_FINGERPRINT_NUM_ALGOS; i++)
    {
        if (irc_fingerprint_digest_algos_size[i] == size)
            return i;
    }

    /* digest algorithm not found */
    return -1;
}
#endif /* HAVE_GNUTLS */

/*
 * Returns a string with sizes of allowed fingerprint,
 * in number of hexadecimal digits (== bits / 4).
 *
 * Example of output: "64=SHA-512, 32=SHA-256, 20=SHA-1".
 *
 * Note: result must be freed after use.
 */

#ifdef HAVE_GNUTLS
char *
irc_server_fingerprint_str_sizes ()
{
    char str_sizes[1024], str_one_size[128];
    int i;

    str_sizes[0] = '\0';

    for (i = IRC_FINGERPRINT_NUM_ALGOS - 1; i >= 0; i--)
    {
        snprintf (str_one_size, sizeof (str_one_size),
                  "%d=%s%s",
                  irc_fingerprint_digest_algos_size[i] / 8,
                  irc_fingerprint_digest_algos_name[i],
                  (i > 0) ? ", " : "");
        strcat (str_sizes, str_one_size);
    }

    return strdup (str_sizes);
}
#endif /* HAVE_GNUTLS */

/*
 * Compares two fingerprints: one hexadecimal (given by user), the second binary
 * (received from IRC server).
 *
 * Returns:
 *    0: fingerprints are the same
 *   -1: fingerprints are different
 */

#ifdef HAVE_GNUTLS
int
irc_server_compare_fingerprints (const char *fingerprint,
                                 const unsigned char *fingerprint_server,
                                 ssize_t fingerprint_size)
{
    ssize_t i;
    unsigned int value;

    if ((ssize_t)strlen (fingerprint) != fingerprint_size * 2)
        return -1;

    for (i = 0; i < fingerprint_size; i++)
    {
        if (sscanf (&fingerprint[i * 2], "%02x", &value) != 1)
            return -1;
        if (value != fingerprint_server[i])
            return -1;
    }

    /* fingerprints are the same */
    return 0;
}
#endif /* HAVE_GNUTLS */

/*
 * Checks if a GnuTLS session uses the certificate with a given fingerprint.
 *
 * Returns:
 *   1: certificate has the good fingerprint
 *   0: certificate does NOT have the good fingerprint
 */

#ifdef HAVE_GNUTLS
int
irc_server_check_certificate_fingerprint (struct t_irc_server *server,
                                          gnutls_x509_crt_t certificate,
                                          const char *good_fingerprints)
{
    unsigned char *fingerprint_server[IRC_FINGERPRINT_NUM_ALGOS];
    char **fingerprints;
    int i, rc, algo;
    size_t size_bits, size_bytes;

    for (i = 0; i < IRC_FINGERPRINT_NUM_ALGOS; i++)
    {
        fingerprint_server[i] = NULL;
    }

    /* split good_fingerprints */
    fingerprints = weechat_string_split (good_fingerprints, ",", 0, 0, NULL);
    if (!fingerprints)
        return 0;

    rc = 0;

    for (i = 0; fingerprints[i]; i++)
    {
        size_bits = strlen (fingerprints[i]) * 4;
        size_bytes = size_bits / 8;

        algo = irc_server_fingerprint_search_algo_with_size (size_bits);
        if (algo < 0)
            continue;

        if (!fingerprint_server[algo])
        {
            fingerprint_server[algo] = malloc (size_bytes);
            if (fingerprint_server[algo])
            {
                /* calculate the fingerprint for the certificate */
                if (gnutls_x509_crt_get_fingerprint (
                        certificate,
                        irc_fingerprint_digest_algos[algo],
                        fingerprint_server[algo],
                        &size_bytes) != GNUTLS_E_SUCCESS)
                {
                    weechat_printf (
                        server->buffer,
                        _("%sgnutls: failed to calculate certificate "
                          "fingerprint (%s)"),
                        weechat_prefix ("error"),
                        irc_fingerprint_digest_algos_name[algo]);
                    free (fingerprint_server[algo]);
                    fingerprint_server[algo] = NULL;
                }
            }
            else
            {
                weechat_printf (
                    server->buffer,
                    _("%s%s: not enough memory"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME);
            }
        }

        if (fingerprint_server[algo])
        {
            /* check if the fingerprint matches */
            if (irc_server_compare_fingerprints (fingerprints[i],
                                                 fingerprint_server[algo],
                                                 size_bytes) == 0)
            {
                rc = 1;
                break;
            }
        }
    }

    weechat_string_free_split (fingerprints);

    for (i = 0; i < IRC_FINGERPRINT_NUM_ALGOS; i++)
    {
        if (fingerprint_server[i])
            free (fingerprint_server[i]);
    }

    return rc;
}
#endif /* HAVE_GNUTLS */

/*
 * GnuTLS callback called during handshake.
 *
 * Returns:
 *    0: certificate OK
 *   -1: error in certificate
 */

#ifdef HAVE_GNUTLS
int
irc_server_gnutls_callback (void *data, gnutls_session_t tls_session,
                            const gnutls_datum_t *req_ca, int nreq,
                            const gnutls_pk_algorithm_t *pk_algos,
                            int pk_algos_len,
#if LIBGNUTLS_VERSION_NUMBER >= 0x020b00 /* 2.11.0 */
                            gnutls_retr2_st *answer,
#else
                            gnutls_retr_st *answer,
#endif /* LIBGNUTLS_VERSION_NUMBER >= 0x020b00 */
                            int action)
{
    struct t_irc_server *server;
#if LIBGNUTLS_VERSION_NUMBER >= 0x020b00 /* 2.11.0 */
    gnutls_retr2_st tls_struct;
#else
    gnutls_retr_st tls_struct;
#endif /* LIBGNUTLS_VERSION_NUMBER >= 0x020b00 */
    gnutls_x509_crt_t cert_temp;
    const gnutls_datum_t *cert_list;
    gnutls_datum_t filedatum;
    unsigned int i, cert_list_len, status;
    time_t cert_time;
    char *cert_path0, *cert_path1, *cert_path2, *cert_str;
    const char *weechat_dir, *fingerprint;
    int rc, ret, fingerprint_match, hostname_match, cert_temp_init;
#if LIBGNUTLS_VERSION_NUMBER >= 0x010706 /* 1.7.6 */
    gnutls_datum_t cinfo;
    int rinfo;
#endif /* LIBGNUTLS_VERSION_NUMBER >= 0x010706 */

    /* make C compiler happy */
    (void) req_ca;
    (void) nreq;
    (void) pk_algos;
    (void) pk_algos_len;

    rc = 0;

    if (!data)
        return -1;

    server = (struct t_irc_server *) data;
    cert_temp_init = 0;
    cert_list = NULL;
    cert_list_len = 0;

    if (action == WEECHAT_HOOK_CONNECT_GNUTLS_CB_VERIFY_CERT)
    {
        weechat_printf (
            server->buffer,
            _("%sgnutls: connected using %d-bit Diffie-Hellman shared secret "
              "exchange"),
            weechat_prefix ("network"),
            IRC_SERVER_OPTION_INTEGER (server,
                                       IRC_SERVER_OPTION_SSL_DHKEY_SIZE));

        /* initialize the certificate structure */
        if (gnutls_x509_crt_init (&cert_temp) != GNUTLS_E_SUCCESS)
        {
            weechat_printf (
                server->buffer,
                _("%sgnutls: failed to initialize certificate structure"),
                weechat_prefix ("error"));
            rc = -1;
            goto end;
        }

        /* flag to do the "deinit" (at the end of function) */
        cert_temp_init = 1;

        /* get fingerprint option in server */
        fingerprint = IRC_SERVER_OPTION_STRING (server,
                                                IRC_SERVER_OPTION_SSL_FINGERPRINT);

        /* set match options */
        fingerprint_match = (fingerprint && fingerprint[0]) ? 0 : 1;
        hostname_match = 0;

        /* get the peer's raw certificate (chain) as sent by the peer */
        cert_list = gnutls_certificate_get_peers (tls_session, &cert_list_len);
        if (cert_list)
        {
            weechat_printf (
                server->buffer,
                NG_("%sgnutls: receiving %d certificate",
                    "%sgnutls: receiving %d certificates",
                    cert_list_len),
                weechat_prefix ("network"),
                cert_list_len);

            for (i = 0; i < cert_list_len; i++)
            {
                if (gnutls_x509_crt_import (cert_temp,
                                            &cert_list[i],
                                            GNUTLS_X509_FMT_DER) != GNUTLS_E_SUCCESS)
                {
                    weechat_printf (
                        server->buffer,
                        _("%sgnutls: failed to import certificate[%d]"),
                        weechat_prefix ("error"), i + 1);
                    rc = -1;
                    goto end;
                }

                /* checks on first certificate received */
                if (i == 0)
                {
                    /* check if fingerprint matches the first certificate */
                    if (fingerprint && fingerprint[0])
                    {
                        fingerprint_match = irc_server_check_certificate_fingerprint (
                            server, cert_temp, fingerprint);
                    }
                    /* check if hostname matches in the first certificate */
                    if (gnutls_x509_crt_check_hostname (cert_temp,
                                                        server->current_address) != 0)
                    {
                        hostname_match = 1;
                    }
                }
#if LIBGNUTLS_VERSION_NUMBER >= 0x010706 /* 1.7.6 */
                /* display infos about certificate */
#if LIBGNUTLS_VERSION_NUMBER < 0x020400 /* 2.4.0 */
                rinfo = gnutls_x509_crt_print (cert_temp,
                                               GNUTLS_X509_CRT_ONELINE, &cinfo);
#else
                rinfo = gnutls_x509_crt_print (cert_temp,
                                               GNUTLS_CRT_PRINT_ONELINE, &cinfo);
#endif /*  LIBGNUTLS_VERSION_NUMBER < 0x020400 */
                if (rinfo == 0)
                {
                    weechat_printf (
                        server->buffer,
                        _("%s - certificate[%d] info:"),
                        weechat_prefix ("network"), i + 1);
                    weechat_printf (
                        server->buffer,
                        "%s   - %s",
                        weechat_prefix ("network"), cinfo.data);
                    gnutls_free (cinfo.data);
                }
#endif /* LIBGNUTLS_VERSION_NUMBER >= 0x010706 */
                /* check dates, only if fingerprint is not set */
                if (!fingerprint || !fingerprint[0])
                {
                    /* check expiration date */
                    cert_time = gnutls_x509_crt_get_expiration_time (cert_temp);
                    if (cert_time < time (NULL))
                    {
                        weechat_printf (
                            server->buffer,
                            _("%sgnutls: certificate has expired"),
                            weechat_prefix ("error"));
                        rc = -1;
                    }
                    /* check activation date */
                    cert_time = gnutls_x509_crt_get_activation_time (cert_temp);
                    if (cert_time > time (NULL))
                    {
                        weechat_printf (
                            server->buffer,
                            _("%sgnutls: certificate is not yet activated"),
                            weechat_prefix ("error"));
                        rc = -1;
                    }
                }
            }

            /*
             * if fingerprint is set, display if matches, and don't check
             * anything else
             */
            if (fingerprint && fingerprint[0])
            {
                if (fingerprint_match)
                {
                    weechat_printf (
                        server->buffer,
                        _("%sgnutls: certificate fingerprint matches"),
                        weechat_prefix ("network"));
                }
                else
                {
                    weechat_printf (
                        server->buffer,
                        _("%sgnutls: certificate fingerprint does NOT match "
                          "(check value of option "
                          "irc.server.%s.ssl_fingerprint)"),
                        weechat_prefix ("error"), server->name);
                    rc = -1;
                }
                goto end;
            }

            if (!hostname_match)
            {
                weechat_printf (
                    server->buffer,
                    _("%sgnutls: the hostname in the certificate does NOT "
                      "match \"%s\""),
                    weechat_prefix ("error"), server->current_address);
                rc = -1;
            }
        }

        /* verify the peer’s certificate */
        if (gnutls_certificate_verify_peers2 (tls_session, &status) < 0)
        {
            weechat_printf (
                server->buffer,
                _("%sgnutls: error while checking peer's certificate"),
                weechat_prefix ("error"));
            rc = -1;
            goto end;
        }

        /* check if certificate is trusted */
        if (status & GNUTLS_CERT_INVALID)
        {
            weechat_printf (
                server->buffer,
                _("%sgnutls: peer's certificate is NOT trusted"),
                weechat_prefix ("error"));
            rc = -1;
        }
        else
        {
            weechat_printf (
                server->buffer,
                _("%sgnutls: peer's certificate is trusted"),
                weechat_prefix ("network"));
        }

        /* check if certificate issuer is known */
        if (status & GNUTLS_CERT_SIGNER_NOT_FOUND)
        {
            weechat_printf (
                server->buffer,
                _("%sgnutls: peer's certificate issuer is unknown"),
                weechat_prefix ("error"));
            rc = -1;
        }

        /* check that certificate is not revoked */
        if (status & GNUTLS_CERT_REVOKED)
        {
            weechat_printf (
                server->buffer,
                _("%sgnutls: the certificate has been revoked"),
                weechat_prefix ("error"));
            rc = -1;
        }
    }
    else if (action == WEECHAT_HOOK_CONNECT_GNUTLS_CB_SET_CERT)
    {
        /* using client certificate if it exists */
        cert_path0 = (char *) IRC_SERVER_OPTION_STRING(
            server, IRC_SERVER_OPTION_SSL_CERT);
        if (cert_path0 && cert_path0[0])
        {
            weechat_dir = weechat_info_get ("weechat_dir", "");
            cert_path1 = weechat_string_replace (cert_path0, "%h", weechat_dir);
            cert_path2 = (cert_path1) ?
                weechat_string_expand_home (cert_path1) : NULL;

            if (cert_path2)
            {
                cert_str = weechat_file_get_content (cert_path2);
                if (cert_str)
                {
                    weechat_printf (
                        server->buffer,
                        _("%sgnutls: sending one certificate"),
                        weechat_prefix ("network"));

                    filedatum.data = (unsigned char *) cert_str;
                    filedatum.size = strlen (cert_str);

                    /* certificate */
                    gnutls_x509_crt_init (&server->tls_cert);
                    gnutls_x509_crt_import (server->tls_cert, &filedatum,
                                            GNUTLS_X509_FMT_PEM);

                    /* key */
                    gnutls_x509_privkey_init (&server->tls_cert_key);
                    ret = gnutls_x509_privkey_import (server->tls_cert_key,
                                                      &filedatum,
                                                      GNUTLS_X509_FMT_PEM);
                    if (ret < 0)
                    {
                        ret = gnutls_x509_privkey_import_pkcs8 (
                            server->tls_cert_key,
                            &filedatum,
                            GNUTLS_X509_FMT_PEM,
                            NULL,
                            GNUTLS_PKCS_PLAIN);
                    }
                    if (ret < 0)
                    {
                        weechat_printf (
                            server->buffer,
                            _("%sgnutls: invalid certificate \"%s\", error: "
                              "%s"),
                            weechat_prefix ("error"), cert_path2,
                            gnutls_strerror (ret));
                        rc = -1;
                    }
                    else
                    {

#if LIBGNUTLS_VERSION_NUMBER >= 0x020b00 /* 2.11.0 */
                        tls_struct.cert_type = GNUTLS_CRT_X509;
                        tls_struct.key_type = GNUTLS_PRIVKEY_X509;
#else
                        tls_struct.type = GNUTLS_CRT_X509;
#endif /* LIBGNUTLS_VERSION_NUMBER >= 0x020b00 */
                        tls_struct.ncerts = 1;
                        tls_struct.deinit_all = 0;
                        tls_struct.cert.x509 = &server->tls_cert;
                        tls_struct.key.x509 = server->tls_cert_key;
#if LIBGNUTLS_VERSION_NUMBER >= 0x010706 /* 1.7.6 */
                        /* client certificate info */
#if LIBGNUTLS_VERSION_NUMBER < 0x020400 /* 2.4.0 */
                        rinfo = gnutls_x509_crt_print (server->tls_cert,
                                                       GNUTLS_X509_CRT_ONELINE,
                                                       &cinfo);
#else
                        rinfo = gnutls_x509_crt_print (server->tls_cert,
                                                       GNUTLS_CRT_PRINT_ONELINE,
                                                       &cinfo);
#endif /* LIBGNUTLS_VERSION_NUMBER < 0x020400 */
                        if (rinfo == 0)
                        {
                            weechat_printf (
                                server->buffer,
                                _("%s - client certificate info (%s):"),
                                weechat_prefix ("network"), cert_path2);
                            weechat_printf (
                                server->buffer, "%s  - %s",
                                weechat_prefix ("network"), cinfo.data);
                            gnutls_free (cinfo.data);
                        }
#endif /* LIBGNUTLS_VERSION_NUMBER >= 0x010706 */
                        memcpy (answer, &tls_struct, sizeof (tls_struct));
                        free (cert_str);
                    }
                }
                else
                {
                    weechat_printf (
                        server->buffer,
                        _("%sgnutls: unable to read certificate \"%s\""),
                        weechat_prefix ("error"), cert_path2);
                }
            }

            if (cert_path1)
                free (cert_path1);
            if (cert_path2)
                free (cert_path2);
        }
    }

end:
    /* an error should stop the handshake unless the user doesn't care */
    if ((rc == -1)
        && (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_SSL_VERIFY) == 0))
    {
        rc = 0;
    }

    if (cert_temp_init)
        gnutls_x509_crt_deinit (cert_temp);

    return rc;
}
#endif /* HAVE_GNUTLS */

/*
 * Connects to a server.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_server_connect (struct t_irc_server *server)
{
    int length;
    char *option_name;
    struct t_config_option *proxy_type, *proxy_ipv6, *proxy_address;
    struct t_config_option *proxy_port;
    const char *proxy, *str_proxy_type, *str_proxy_address;

    server->disconnected = 0;

    if (!server->buffer)
    {
        if (!irc_server_create_buffer (server))
            return 0;
        weechat_buffer_set (server->buffer, "display", "auto");
    }

    irc_bar_item_update_channel ();

    irc_server_set_index_current_address (server,
                                          server->index_current_address);

    if (!server->current_address)
    {
        weechat_printf (
            server->buffer,
            _("%s%s: unknown address for server \"%s\", cannot connect"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, server->name);
        return 0;
    }

    /* free some old values (from a previous connection to server) */
    if (server->isupport)
    {
        free (server->isupport);
        server->isupport = NULL;
    }
    if (server->prefix_modes)
    {
        free (server->prefix_modes);
        server->prefix_modes = NULL;
    }
    if (server->prefix_chars)
    {
        free (server->prefix_chars);
        server->prefix_chars = NULL;
    }

    proxy_type = NULL;
    proxy_ipv6 = NULL;
    proxy_address = NULL;
    proxy_port = NULL;
    str_proxy_type = NULL;
    str_proxy_address = NULL;

    proxy = IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_PROXY);
    if (proxy && proxy[0])
    {
        length = 32 + strlen (proxy) + 1;
        option_name = malloc (length);
        if (!option_name)
        {
            weechat_printf (
                server->buffer,
                _("%s%s: not enough memory"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            return 0;
        }
        snprintf (option_name, length, "weechat.proxy.%s.type", proxy);
        proxy_type = weechat_config_get (option_name);
        snprintf (option_name, length, "weechat.proxy.%s.ipv6", proxy);
        proxy_ipv6 = weechat_config_get (option_name);
        snprintf (option_name, length, "weechat.proxy.%s.address", proxy);
        proxy_address = weechat_config_get (option_name);
        snprintf (option_name, length, "weechat.proxy.%s.port", proxy);
        proxy_port = weechat_config_get (option_name);
        free (option_name);
        if (!proxy_type || !proxy_address)
        {
            weechat_printf (
                server->buffer,
                _("%s%s: proxy \"%s\" not found for server \"%s\", cannot "
                  "connect"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME, proxy, server->name);
            return 0;
        }
        str_proxy_type = weechat_config_string (proxy_type);
        str_proxy_address = weechat_config_string (proxy_address);
        if (!str_proxy_type[0] || !proxy_ipv6 || !str_proxy_address[0]
            || !proxy_port)
        {
            weechat_printf (
                server->buffer,
                _("%s%s: missing proxy settings, check options for proxy "
                  "\"%s\""),
                weechat_prefix ("error"), IRC_PLUGIN_NAME, proxy);
            return 0;
        }
    }

    if (!server->nicks_array)
    {
        weechat_printf (
            server->buffer,
            _("%s%s: nicks not defined for server \"%s\", cannot connect"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, server->name);
        return 0;
    }

#ifndef HAVE_GNUTLS
    if (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_SSL))
    {
        weechat_printf (
            server->buffer,
            _("%s%s: cannot connect with SSL because WeeChat was not built "
              "with GnuTLS support"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME);
        return 0;
    }
#endif /* HAVE_GNUTLS */
    if (proxy_type)
    {
        weechat_printf (
            server->buffer,
            _("%s%s: connecting to server %s/%d%s via %s proxy %s/%d%s..."),
            weechat_prefix ("network"),
            IRC_PLUGIN_NAME,
            server->current_address,
            server->current_port,
            (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_SSL)) ?
            " (SSL)" : "",
            str_proxy_type,
            str_proxy_address,
            weechat_config_integer (proxy_port),
            (weechat_config_boolean (proxy_ipv6)) ? " (IPv6)" : "");
        weechat_log_printf (
            _("Connecting to server %s/%d%s via %s proxy %s/%d%s..."),
            server->current_address,
            server->current_port,
            (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_SSL)) ?
            " (SSL)" : "",
            str_proxy_type,
            str_proxy_address,
            weechat_config_integer (proxy_port),
            (weechat_config_boolean (proxy_ipv6)) ? " (IPv6)" : "");
    }
    else
    {
        weechat_printf (
            server->buffer,
            _("%s%s: connecting to server %s/%d%s..."),
            weechat_prefix ("network"),
            IRC_PLUGIN_NAME,
            server->current_address,
            server->current_port,
            (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_SSL)) ?
            " (SSL)" : "");
        weechat_log_printf (
            _("%s%s: connecting to server %s/%d%s..."),
            "",
            IRC_PLUGIN_NAME,
            server->current_address,
            server->current_port,
            (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_SSL)) ?
            " (SSL)" : "");
    }

    /* close connection if opened */
    irc_server_close_connection (server);

    /* open auto-joined channels now (if needed) */
    if (weechat_config_boolean (irc_config_look_buffer_open_before_autojoin)
        && !server->disable_autojoin)
    {
        irc_server_autojoin_create_buffers (server);
    }

    /* init SSL if asked and connect */
    server->ssl_connected = 0;
#ifdef HAVE_GNUTLS
    if (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_SSL))
        server->ssl_connected = 1;
    server->hook_connect = weechat_hook_connect (
        proxy,
        server->current_address,
        server->current_port,
        proxy_type ? weechat_config_integer (proxy_ipv6) : IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_IPV6),
        server->current_retry,
        (server->ssl_connected) ? &server->gnutls_sess : NULL,
        (server->ssl_connected) ? irc_server_gnutls_callback : NULL,
        IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_SSL_DHKEY_SIZE),
        IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_SSL_PRIORITIES),
        IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_LOCAL_HOSTNAME),
        &irc_server_connect_cb,
        server);
#else
    server->hook_connect = weechat_hook_connect (
        proxy,
        server->current_address,
        server->current_port,
        proxy_type ? weechat_config_integer (proxy_ipv6) : IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_IPV6),
        server->current_retry,
        NULL, NULL, 0, NULL,
        IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_LOCAL_HOSTNAME),
        &irc_server_connect_cb,
        server);
#endif /* HAVE_GNUTLS */

    /* send signal "irc_server_connecting" with server name */
    (void) weechat_hook_signal_send ("irc_server_connecting",
                                     WEECHAT_HOOK_SIGNAL_STRING, server->name);

    return 1;
}

/*
 * Reconnects to a server (after disconnection).
 */

void
irc_server_reconnect (struct t_irc_server *server)
{
    weechat_printf (
        server->buffer,
        _("%s%s: reconnecting to server..."),
        weechat_prefix ("network"), IRC_PLUGIN_NAME);

    server->reconnect_start = 0;

    if (irc_server_connect (server))
        server->reconnect_join = 1;
    else
        irc_server_reconnect_schedule (server);
}

/*
 * Callback for auto-connect to servers (called at startup).
 */

int
irc_server_auto_connect_timer_cb (void *data, int remaining_calls)
{
    struct t_irc_server *ptr_server;
    void *auto_connect;

    /* make C compiler happy */
    (void) remaining_calls;

    auto_connect = data;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if ((auto_connect || ptr_server->temp_server)
            && (IRC_SERVER_OPTION_BOOLEAN(ptr_server, IRC_SERVER_OPTION_AUTOCONNECT)))
        {
            if (!irc_server_connect (ptr_server))
                irc_server_reconnect_schedule (ptr_server);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Auto-connects to servers (called at startup).
 *
 * If auto_connect == 1, auto-connects to all servers with flag "autoconnect".
 * If auto_connect == 0, auto-connect to temporary servers only.
 */

void
irc_server_auto_connect (int auto_connect)
{
    weechat_hook_timer (1, 0, 1, &irc_server_auto_connect_timer_cb,
                        (auto_connect) ? (void *)1 : (void *)0);
}

/*
 * Disconnects from a server.
 */

void
irc_server_disconnect (struct t_irc_server *server, int switch_address,
                       int reconnect)
{
    struct t_irc_channel *ptr_channel;

    if (server->is_connected)
    {
        /*
         * remove all nicks and write disconnection message on each
         * channel/private buffer
         */
        for (ptr_channel = server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            irc_nick_free_all (server, ptr_channel);
            if (ptr_channel->hook_autorejoin)
            {
                weechat_unhook (ptr_channel->hook_autorejoin);
                ptr_channel->hook_autorejoin = NULL;
            }
            weechat_buffer_set (ptr_channel->buffer, "localvar_del_away", "");
            weechat_printf (
                ptr_channel->buffer,
                _("%s%s: disconnected from server"),
                weechat_prefix ("network"), IRC_PLUGIN_NAME);
        }
        /* remove away status on server buffer */
        weechat_buffer_set (server->buffer, "localvar_del_away", "");
    }

    irc_server_close_connection (server);

    if (server->buffer)
    {
        weechat_printf (
            server->buffer,
            _("%s%s: disconnected from server"),
            weechat_prefix ("network"), IRC_PLUGIN_NAME);
    }

    server->current_retry = 0;

    if (switch_address)
        irc_server_switch_address (server, 0);
    else
        irc_server_set_index_current_address (server, 0);

    if (server->nick_modes)
    {
        free (server->nick_modes);
        server->nick_modes = NULL;
        weechat_bar_item_update ("input_prompt");
        weechat_bar_item_update ("irc_nick_modes");
    }
    server->cap_away_notify = 0;
    server->cap_account_notify = 0;
    server->cap_extended_join = 0;
    server->is_away = 0;
    server->away_time = 0;
    server->lag = 0;
    server->lag_displayed = -1;
    server->lag_check_time.tv_sec = 0;
    server->lag_check_time.tv_usec = 0;
    server->lag_next_check = time (NULL) +
        weechat_config_integer (irc_config_network_lag_check);
    server->lag_last_refresh = 0;
    server->monitor = 0;
    server->monitor_time = 0;

    if (reconnect
        && IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_AUTORECONNECT))
        irc_server_reconnect_schedule (server);
    else
    {
        server->reconnect_delay = 0;
        server->reconnect_start = 0;
    }

    /* discard current nick if no reconnection asked */
    if (!reconnect && server->nick)
        irc_server_set_nick (server, NULL);

    irc_server_set_buffer_title (server);

    server->disconnected = 1;

    /* send signal "irc_server_disconnected" with server name */
    (void) weechat_hook_signal_send ("irc_server_disconnected",
                                     WEECHAT_HOOK_SIGNAL_STRING, server->name);
}

/*
 * Disconnects from all servers.
 */

void
irc_server_disconnect_all ()
{
    struct t_irc_server *ptr_server;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        irc_server_disconnect (ptr_server, 0, 0);
    }
}

/*
 * Creates buffers for auto-joined channels on a server.
 */

void
irc_server_autojoin_create_buffers (struct t_irc_server *server)
{
    const char *pos_space;
    char *autojoin, *autojoin2, **channels;
    int num_channels, i;

    /* buffers are opened only if no channels are currently opened */
    if (server->channels)
        return;

    /* evaluate server option "autojoin" */
    autojoin = weechat_string_eval_expression (
        IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_AUTOJOIN),
        NULL, NULL, NULL);

    /* extract channel names from autojoin option */
    if (autojoin && autojoin[0])
    {
        pos_space = strchr (autojoin, ' ');
        autojoin2 = (pos_space) ?
            weechat_strndup (autojoin, pos_space - autojoin) :
            strdup (autojoin);
        if (autojoin2)
        {
            channels = weechat_string_split (autojoin2, ",", 0, 0,
                                             &num_channels);
            if (channels)
            {
                for (i = 0; i < num_channels; i++)
                {
                    irc_channel_create_buffer (
                        server, IRC_CHANNEL_TYPE_CHANNEL, channels[i],
                        1, 1);
                }
                weechat_string_free_split (channels);
            }
            free (autojoin2);
        }
    }

    if (autojoin)
        free (autojoin);
}

/*
 * Autojoins (or auto-rejoins) channels.
 */

void
irc_server_autojoin_channels (struct t_irc_server *server)
{
    struct t_irc_channel *ptr_channel;
    char *autojoin;

    /* auto-join after disconnection (only rejoins opened channels) */
    if (!server->disable_autojoin && server->reconnect_join && server->channels)
    {
        for (ptr_channel = server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if ((ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                && !ptr_channel->part)
            {
                if (ptr_channel->key)
                {
                    irc_server_sendf (server,
                                      IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                                      "JOIN %s %s",
                                      ptr_channel->name, ptr_channel->key);
                }
                else
                {
                    irc_server_sendf (server,
                                      IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                                      "JOIN %s",
                                      ptr_channel->name);
                }
            }
        }
        server->reconnect_join = 0;
    }
    else
    {
        /* auto-join when connecting to server for first time */
        autojoin = weechat_string_eval_expression (
            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_AUTOJOIN),
            NULL, NULL, NULL);
        if (!server->disable_autojoin && autojoin && autojoin[0])
            irc_command_join_server (server, autojoin, 0, 0);
        if (autojoin)
            free (autojoin);
    }

    server->disable_autojoin = 0;
}

/*
 * Returns number of connected servers.
 */

int
irc_server_get_number_connected ()
{
    struct t_irc_server *ptr_server;
    int number;

    number = 0;
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->is_connected)
            number++;
    }
    return number;
}

/*
 * Returns position of a server and total number of servers with a buffer.
 */

void
irc_server_get_number_buffer (struct t_irc_server *server,
                              int *server_pos, int *server_total)
{
    struct t_irc_server *ptr_server;

    *server_pos = 0;
    *server_total = 0;
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->buffer)
        {
            (*server_total)++;
            if (ptr_server == server)
                *server_pos = *server_total;
        }
    }
}

/*
 * Returns number of channels for server.
 */

int
irc_server_get_channel_count (struct t_irc_server *server)
{
    int count;
    struct t_irc_channel *ptr_channel;

    count = 0;
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
        count++;
    }
    return count;
}

/*
 * Returns number of pv for server.
 */

int
irc_server_get_pv_count (struct t_irc_server *server)
{
    int count;
    struct t_irc_channel *ptr_channel;

    count = 0;
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
            count++;
    }
    return count;
}

/*
 * Removes away for all channels/nicks (for all servers).
 */

void
irc_server_remove_away (struct t_irc_server *server)
{
    struct t_irc_channel *ptr_channel;

    if (server->is_connected)
    {
        for (ptr_channel = server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                irc_channel_remove_away (server, ptr_channel);
        }
        server->last_away_check = 0;
    }
}

/*
 * Checks for away on all channels of a server.
 */

void
irc_server_check_away (struct t_irc_server *server)
{
    struct t_irc_channel *ptr_channel;

    if (server->is_connected)
    {
        for (ptr_channel = server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                irc_channel_check_whox (server, ptr_channel);
        }
        server->last_away_check = time (NULL);
    }
}

/*
 * Sets/unsets away status for a server (all channels).
 */

void
irc_server_set_away (struct t_irc_server *server, const char *nick, int is_away)
{
    struct t_irc_channel *ptr_channel;

    if (server->is_connected)
    {
        /* set/del "away" local variable on server buffer */
        if (is_away)
        {
            weechat_buffer_set (server->buffer,
                                "localvar_set_away", server->away_message);
        }
        else
        {
            weechat_buffer_set (server->buffer,
                                "localvar_del_away", "");
        }

        for (ptr_channel = server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            /* set away flag for nick on channel */
            if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                irc_channel_set_away (server, ptr_channel, nick, is_away);

            /* set/del "away" local variable on channel buffer */
            if (is_away)
            {
                weechat_buffer_set (ptr_channel->buffer,
                                    "localvar_set_away", server->away_message);
            }
            else
            {
                weechat_buffer_set (ptr_channel->buffer,
                                    "localvar_del_away", "");
            }
        }
    }
}

/*
 * Callback called when user sends (file or chat) to someone and that xfer
 * plugin successfully initialized xfer and is ready for sending.
 *
 * In that case, irc plugin sends message to remote nick and wait for "accept"
 * reply.
 */

int
irc_server_xfer_send_ready_cb (void *data, const char *signal,
                               const char *type_data, void *signal_data)
{
    struct t_infolist *infolist;
    struct t_irc_server *ptr_server;
    const char *plugin_name, *plugin_id, *type, *filename, *local_address;
    char converted_addr[NI_MAXHOST];
    struct addrinfo *ainfo;
    struct sockaddr_in *saddr;
    int spaces_in_name, rc;

    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    infolist = (struct t_infolist *)signal_data;

    if (weechat_infolist_next (infolist))
    {
        plugin_name = weechat_infolist_string (infolist, "plugin_name");
        plugin_id = weechat_infolist_string (infolist, "plugin_id");
        if (plugin_name && (strcmp (plugin_name, IRC_PLUGIN_NAME) == 0)
            && plugin_id)
        {
            ptr_server = irc_server_search (plugin_id);
            if (ptr_server)
            {
                converted_addr[0] = '\0';
                local_address = weechat_infolist_string (infolist,
                                                         "local_address");
                if (local_address)
                {
                    rc = getaddrinfo (local_address, NULL, NULL, &ainfo);
                    if ((rc == 0) && ainfo && ainfo->ai_addr)
                    {
                        if (ainfo->ai_family == AF_INET)
                        {
                            /* transform dotted 4 IP address to ulong string */
                            saddr = (struct sockaddr_in *)ainfo->ai_addr;
                            snprintf (converted_addr, sizeof (converted_addr),
                                      "%lu",
                                      (unsigned long)ntohl (saddr->sin_addr.s_addr));
                        }
                        else
                        {
                            snprintf (converted_addr, sizeof (converted_addr),
                                      "%s", local_address);
                        }
                    }
                }

                type = weechat_infolist_string (infolist, "type_string");
                if (type && converted_addr[0])
                {
                    /* send DCC PRIVMSG */
                    if (strcmp (type, "file_send") == 0)
                    {
                        filename = weechat_infolist_string (infolist, "filename");
                        spaces_in_name = (strchr (filename, ' ') != NULL);
                        irc_server_sendf (
                            ptr_server,
                            IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                            "PRIVMSG %s :\01DCC SEND %s%s%s "
                            "%s %d %s\01",
                            weechat_infolist_string (infolist, "remote_nick"),
                            (spaces_in_name) ? "\"" : "",
                            filename,
                            (spaces_in_name) ? "\"" : "",
                            converted_addr,
                            weechat_infolist_integer (infolist, "port"),
                            weechat_infolist_string (infolist, "size"));
                    }
                    else if (strcmp (type, "chat_send") == 0)
                    {
                        irc_server_sendf (
                            ptr_server,
                            IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                            "PRIVMSG %s :\01DCC CHAT chat %s %d\01",
                            weechat_infolist_string (infolist, "remote_nick"),
                            converted_addr,
                            weechat_infolist_integer (infolist, "port"));
                    }
                }
            }
        }
    }

    weechat_infolist_reset_item_cursor (infolist);

    return WEECHAT_RC_OK;
}

/*
 * Callback called when user receives a file and that resume is possible (file
 * is partially received).
 *
 * In that case, irc plugin sends message to remote nick with resume position.
 */

int
irc_server_xfer_resume_ready_cb (void *data, const char *signal,
                                 const char *type_data, void *signal_data)
{
    struct t_infolist *infolist;
    struct t_irc_server *ptr_server;
    const char *plugin_name, *plugin_id, *filename;
    int spaces_in_name;

    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    infolist = (struct t_infolist *)signal_data;

    if (weechat_infolist_next (infolist))
    {
        plugin_name = weechat_infolist_string (infolist, "plugin_name");
        plugin_id = weechat_infolist_string (infolist, "plugin_id");
        if (plugin_name && (strcmp (plugin_name, IRC_PLUGIN_NAME) == 0) && plugin_id)
        {
            ptr_server = irc_server_search (plugin_id);
            if (ptr_server)
            {
                filename = weechat_infolist_string (infolist, "filename");
                spaces_in_name = (strchr (filename, ' ') != NULL);
                irc_server_sendf (
                    ptr_server,
                    IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                    "PRIVMSG %s :\01DCC RESUME %s%s%s %d %s\01",
                    weechat_infolist_string (infolist, "remote_nick"),
                    (spaces_in_name) ? "\"" : "",
                    filename,
                    (spaces_in_name) ? "\"" : "",
                    weechat_infolist_integer (infolist, "port"),
                    weechat_infolist_string (infolist, "start_resume"));
            }
        }
    }

    weechat_infolist_reset_item_cursor (infolist);

    return WEECHAT_RC_OK;
}

/*
 * Callback called when xfer plugin accepted resume request from receiver.
 *
 * In that case, irc plugin sends accept message to remote nick with resume
 * position.
 */

int
irc_server_xfer_send_accept_resume_cb (void *data, const char *signal,
                                       const char *type_data,
                                       void *signal_data)
{
    struct t_infolist *infolist;
    struct t_irc_server *ptr_server;
    const char *plugin_name, *plugin_id, *filename;
    int spaces_in_name;

    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    infolist = (struct t_infolist *)signal_data;

    if (weechat_infolist_next (infolist))
    {
        plugin_name = weechat_infolist_string (infolist, "plugin_name");
        plugin_id = weechat_infolist_string (infolist, "plugin_id");
        if (plugin_name && (strcmp (plugin_name, IRC_PLUGIN_NAME) == 0) && plugin_id)
        {
            ptr_server = irc_server_search (plugin_id);
            if (ptr_server)
            {
                filename = weechat_infolist_string (infolist, "filename");
                spaces_in_name = (strchr (filename, ' ') != NULL);
                irc_server_sendf (
                    ptr_server,
                    IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                    "PRIVMSG %s :\01DCC ACCEPT %s%s%s %d %s\01",
                    weechat_infolist_string (infolist, "remote_nick"),
                    (spaces_in_name) ? "\"" : "",
                    filename,
                    (spaces_in_name) ? "\"" : "",
                    weechat_infolist_integer (infolist, "port"),
                    weechat_infolist_string (infolist, "start_resume"));
            }
        }
    }

    weechat_infolist_reset_item_cursor (infolist);

    return WEECHAT_RC_OK;
}

/*
 * Returns hdata for server.
 */

struct t_hdata *
irc_server_hdata_server_cb (void *data, const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) data;

    hdata = weechat_hdata_new (hdata_name, "prev_server", "next_server",
                               0, 0, NULL, NULL);
    if (hdata)
    {
        WEECHAT_HDATA_VAR(struct t_irc_server, name, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, options, POINTER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, temp_server, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, reloading_from_config, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, reloaded_from_config, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, addresses_count, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, addresses_array, STRING, 0, "addresses_count", NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, ports_array, INTEGER, 0, "addresses_count", NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, retry_array, INTEGER, 0, "addresses_count", NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, index_current_address, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, current_address, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, current_ip, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, current_port, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, current_retry, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, sock, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, hook_connect, POINTER, 0, NULL, "hook");
        WEECHAT_HDATA_VAR(struct t_irc_server, hook_fd, POINTER, 0, NULL, "hook");
        WEECHAT_HDATA_VAR(struct t_irc_server, hook_timer_connection, POINTER, 0, NULL, "hook");
        WEECHAT_HDATA_VAR(struct t_irc_server, hook_timer_sasl, POINTER, 0, NULL, "hook");
        WEECHAT_HDATA_VAR(struct t_irc_server, is_connected, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, ssl_connected, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, disconnected, INTEGER, 0, NULL, NULL);
#ifdef HAVE_GNUTLS
        WEECHAT_HDATA_VAR(struct t_irc_server, gnutls_sess, OTHER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, tls_cert, OTHER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, tls_cert_key, OTHER, 0, NULL, NULL);
#endif /* HAVE_GNUTLS */
        WEECHAT_HDATA_VAR(struct t_irc_server, unterminated_message, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, nicks_count, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, nicks_array, STRING, 0, "nicks_count", NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, nick_first_tried, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, nick_alternate_number, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, nick, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, nick_modes, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, cap_away_notify, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, cap_account_notify, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, cap_extended_join, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, isupport, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, prefix_modes, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, prefix_chars, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, nick_max_length, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, casemapping, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, chantypes, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, chanmodes, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, monitor, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, monitor_time, TIME, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, reconnect_delay, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, reconnect_start, TIME, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, command_time, TIME, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, reconnect_join, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, disable_autojoin, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, is_away, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, away_message, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, away_time, TIME, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, lag, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, lag_displayed, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, lag_check_time, OTHER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, lag_next_check, TIME, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, lag_last_refresh, TIME, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, cmd_list_regexp, POINTER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, last_user_message, TIME, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, last_away_check, TIME, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, last_data_purge, TIME, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, outqueue, POINTER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, last_outqueue, POINTER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, redirects, POINTER, 0, NULL, "irc_redirect");
        WEECHAT_HDATA_VAR(struct t_irc_server, last_redirect, POINTER, 0, NULL, "irc_redirect");
        WEECHAT_HDATA_VAR(struct t_irc_server, notify_list, POINTER, 0, NULL, "irc_notify");
        WEECHAT_HDATA_VAR(struct t_irc_server, last_notify, POINTER, 0, NULL, "irc_notify");
        WEECHAT_HDATA_VAR(struct t_irc_server, notify_count, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, join_manual, HASHTABLE, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, join_channel_key, HASHTABLE, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, join_noswitch, HASHTABLE, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, buffer, POINTER, 0, NULL, "buffer");
        WEECHAT_HDATA_VAR(struct t_irc_server, buffer_as_string, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_server, channels, POINTER, 0, NULL, "irc_channel");
        WEECHAT_HDATA_VAR(struct t_irc_server, last_channel, POINTER, 0, NULL, "irc_channel");
        WEECHAT_HDATA_VAR(struct t_irc_server, prev_server, POINTER, 0, NULL, hdata_name);
        WEECHAT_HDATA_VAR(struct t_irc_server, next_server, POINTER, 0, NULL, hdata_name);
        WEECHAT_HDATA_LIST(irc_servers, WEECHAT_HDATA_LIST_CHECK_POINTERS);
        WEECHAT_HDATA_LIST(last_irc_server, 0);
    }
    return hdata;
}

/*
 * Adds a server in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_server_add_to_infolist (struct t_infolist *infolist,
                            struct t_irc_server *server)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !server)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_string (ptr_item, "name", server->name))
        return 0;
    if (!weechat_infolist_new_var_pointer (ptr_item, "buffer", server->buffer))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "buffer_name",
                                          (server->buffer) ?
                                          weechat_buffer_get_string (server->buffer, "name") : ""))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "buffer_short_name",
                                          (server->buffer) ?
                                          weechat_buffer_get_string (server->buffer, "short_name") : ""))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "addresses",
                                          IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_ADDRESSES)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "proxy",
                                          IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_PROXY)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "ipv6",
                                           IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_IPV6)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "ssl",
                                           IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_SSL)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "ssl_cert",
                                          IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_SSL_CERT)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "ssl_priorities",
                                          IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_SSL_PRIORITIES)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "ssl_dhkey_size",
                                           IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_SSL_DHKEY_SIZE)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "ssl_fingerprint",
                                           IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_SSL_FINGERPRINT)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "ssl_verify",
                                           IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_SSL_VERIFY)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "password",
                                          IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_PASSWORD)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "capabilities",
                                          IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_CAPABILITIES)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "sasl_mechanism",
                                          IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_SASL_MECHANISM)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "sasl_username",
                                          IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_SASL_USERNAME)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "sasl_password",
                                          IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_SASL_PASSWORD)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "sasl_key",
                                          IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_SASL_KEY)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "sasl_fail",
                                           IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_SASL_FAIL)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "autoconnect",
                                           IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_AUTOCONNECT)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "autoreconnect",
                                           IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_AUTORECONNECT)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "autoreconnect_delay",
                                           IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AUTORECONNECT_DELAY)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "nicks",
                                          IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_NICKS)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "nicks_alternate",
                                           IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_NICKS_ALTERNATE)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "username",
                                          IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_USERNAME)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "realname",
                                          IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_REALNAME)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "local_hostname",
                                          IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_LOCAL_HOSTNAME)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "command",
                                          IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_COMMAND)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "command_delay",
                                           IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_COMMAND_DELAY)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "autojoin",
                                          IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_AUTOJOIN)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "autorejoin",
                                           IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_AUTOREJOIN)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "autorejoin_delay",
                                           IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AUTOREJOIN_DELAY)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "connection_timeout",
                                           IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_CONNECTION_TIMEOUT)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "anti_flood_prio_high",
                                           IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_ANTI_FLOOD_PRIO_HIGH)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "anti_flood_prio_low",
                                           IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_ANTI_FLOOD_PRIO_LOW)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "away_check",
                                           IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AWAY_CHECK)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "away_check_max_nicks",
                                           IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AWAY_CHECK_MAX_NICKS)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "default_msg_kick",
                                          IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_DEFAULT_MSG_KICK)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "default_msg_part",
                                          IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_DEFAULT_MSG_PART)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "default_msg_quit",
                                          IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_DEFAULT_MSG_QUIT)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "temp_server", server->temp_server))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "index_current_address", server->index_current_address))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "current_address", server->current_address))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "current_ip", server->current_ip))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "current_port", server->current_port))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "current_retry", server->current_retry))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "sock", server->sock))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "is_connected", server->is_connected))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "ssl_connected", server->ssl_connected))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "disconnected", server->disconnected))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "unterminated_message", server->unterminated_message))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "nick", server->nick))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "nick_modes", server->nick_modes))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "cap_away_notify", server->cap_away_notify))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "cap_account_notify", server->cap_account_notify))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "cap_extended_join", server->cap_extended_join))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "isupport", server->isupport))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "prefix_modes", server->prefix_modes))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "prefix_chars", server->prefix_chars))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "nick_max_length", server->nick_max_length))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "casemapping", server->casemapping))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "casemapping_string", irc_server_casemapping_string[server->casemapping]))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "chantypes", server->chantypes))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "chanmodes", server->chanmodes))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "monitor", server->monitor))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "monitor_time", server->monitor_time))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "reconnect_delay", server->reconnect_delay))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "reconnect_start", server->reconnect_start))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "command_time", server->command_time))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "reconnect_join", server->reconnect_join))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "disable_autojoin", server->disable_autojoin))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "is_away", server->is_away))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "away_message", server->away_message))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "away_time", server->away_time))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "lag", server->lag))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "lag_displayed", server->lag_displayed))
        return 0;
    if (!weechat_infolist_new_var_buffer (ptr_item, "lag_check_time", &(server->lag_check_time), sizeof (struct timeval)))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "lag_next_check", server->lag_next_check))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "lag_last_refresh", server->lag_last_refresh))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "last_user_message", server->last_user_message))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "last_away_check", server->last_away_check))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "last_data_purge", server->last_data_purge))
        return 0;

    return 1;
}

/*
 * Prints server infos in WeeChat log file (usually for crash dump).
 */

void
irc_server_print_log ()
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    int i;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[server %s (addr:0x%lx)]", ptr_server->name, ptr_server);
        /* addresses */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_ADDRESSES]))
            weechat_log_printf ("  addresses. . . . . . : null ('%s')",
                                IRC_SERVER_OPTION_STRING(ptr_server, IRC_SERVER_OPTION_ADDRESSES));
        else
            weechat_log_printf ("  addresses. . . . . . : '%s'",
                                weechat_config_string (ptr_server->options[IRC_SERVER_OPTION_ADDRESSES]));
        /* proxy */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_PROXY]))
            weechat_log_printf ("  proxy. . . . . . . . : null ('%s')",
                                IRC_SERVER_OPTION_STRING(ptr_server, IRC_SERVER_OPTION_PROXY));
        else
            weechat_log_printf ("  proxy. . . . . . . . : '%s'",
                                weechat_config_string (ptr_server->options[IRC_SERVER_OPTION_PROXY]));
        /* ipv6 */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_IPV6]))
            weechat_log_printf ("  ipv6 . . . . . . . . : null (%s)",
                                (IRC_SERVER_OPTION_BOOLEAN(ptr_server, IRC_SERVER_OPTION_IPV6)) ?
                                "on" : "off");
        else
            weechat_log_printf ("  ipv6 . . . . . . . . : %s",
                                (weechat_config_boolean (ptr_server->options[IRC_SERVER_OPTION_IPV6])) ?
                                "on" : "off");
        /* ssl */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_SSL]))
            weechat_log_printf ("  ssl. . . . . . . . . : null (%s)",
                                (IRC_SERVER_OPTION_BOOLEAN(ptr_server, IRC_SERVER_OPTION_SSL)) ?
                                "on" : "off");
        else
            weechat_log_printf ("  ssl. . . . . . . . . : %s",
                                (weechat_config_boolean (ptr_server->options[IRC_SERVER_OPTION_SSL])) ?
                                "on" : "off");
        /* ssl_cert */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_SSL_CERT]))
            weechat_log_printf ("  ssl_cert . . . . . . : null ('%s')",
                                IRC_SERVER_OPTION_STRING(ptr_server, IRC_SERVER_OPTION_SSL_CERT));
        else
            weechat_log_printf ("  ssl_cert . . . . . . : '%s'",
                                weechat_config_string (ptr_server->options[IRC_SERVER_OPTION_SSL_CERT]));
        /* ssl_priorities */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_SSL_PRIORITIES]))
            weechat_log_printf ("  ssl_priorities . . . : null ('%s')",
                                IRC_SERVER_OPTION_STRING(ptr_server, IRC_SERVER_OPTION_SSL_PRIORITIES));
        else
            weechat_log_printf ("  ssl_priorities . . . : '%s'",
                                weechat_config_string (ptr_server->options[IRC_SERVER_OPTION_SSL_PRIORITIES]));
        /* ssl_dhkey_size */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_SSL_DHKEY_SIZE]))
            weechat_log_printf ("  ssl_dhkey_size . . . : null ('%d')",
                                IRC_SERVER_OPTION_INTEGER(ptr_server, IRC_SERVER_OPTION_SSL_DHKEY_SIZE));
        else
            weechat_log_printf ("  ssl_dhkey_size . . . : '%d'",
                                weechat_config_integer (ptr_server->options[IRC_SERVER_OPTION_SSL_DHKEY_SIZE]));
        /* ssl_fingerprint */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_SSL_FINGERPRINT]))
            weechat_log_printf ("  ssl_fingerprint. . . : null ('%s')",
                                IRC_SERVER_OPTION_STRING(ptr_server, IRC_SERVER_OPTION_SSL_FINGERPRINT));
        else
            weechat_log_printf ("  ssl_fingerprint. . . : '%s'",
                                weechat_config_string (ptr_server->options[IRC_SERVER_OPTION_SSL_FINGERPRINT]));
        /* ssl_verify */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_SSL_VERIFY]))
            weechat_log_printf ("  ssl_verify . . . . . : null (%s)",
                                (IRC_SERVER_OPTION_BOOLEAN(ptr_server, IRC_SERVER_OPTION_SSL_VERIFY)) ?
                                "on" : "off");
        else
            weechat_log_printf ("  ssl_verify . . . . . : %s",
                                (weechat_config_boolean (ptr_server->options[IRC_SERVER_OPTION_SSL_VERIFY])) ?
                                "on" : "off");
        /* password */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_PASSWORD]))
            weechat_log_printf ("  password . . . . . . : null");
        else
            weechat_log_printf ("  password . . . . . . : (hidden)");
        /* client capabilities */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_CAPABILITIES]))
            weechat_log_printf ("  capabilities . . . . : null ('%s')",
                                IRC_SERVER_OPTION_STRING(ptr_server, IRC_SERVER_OPTION_CAPABILITIES));
        else
            weechat_log_printf ("  capabilities . . . . : '%s'",
                                weechat_config_string (ptr_server->options[IRC_SERVER_OPTION_CAPABILITIES]));
        /* sasl_mechanism */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_SASL_MECHANISM]))
            weechat_log_printf ("  sasl_mechanism . . . : null ('%s')",
                                irc_sasl_mechanism_string[IRC_SERVER_OPTION_INTEGER(ptr_server, IRC_SERVER_OPTION_SASL_MECHANISM)]);
        else
            weechat_log_printf ("  sasl_mechanism . . . : '%s'",
                                irc_sasl_mechanism_string[weechat_config_integer (ptr_server->options[IRC_SERVER_OPTION_SASL_MECHANISM])]);
        /* sasl_username */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_SASL_USERNAME]))
            weechat_log_printf ("  sasl_username. . . . : null ('%s')",
                                IRC_SERVER_OPTION_STRING(ptr_server, IRC_SERVER_OPTION_SASL_USERNAME));
        else
            weechat_log_printf ("  sasl_username. . . . : '%s'",
                                weechat_config_string (ptr_server->options[IRC_SERVER_OPTION_SASL_USERNAME]));
        /* sasl_password */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_SASL_PASSWORD]))
            weechat_log_printf ("  sasl_password. . . . : null");
        else
            weechat_log_printf ("  sasl_password. . . . : (hidden)");
        /* sasl_key */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_SASL_KEY]))
            weechat_log_printf ("  sasl_key. .  . . . . : null ('%s')",
                                IRC_SERVER_OPTION_STRING(ptr_server, IRC_SERVER_OPTION_SASL_KEY));
        else
            weechat_log_printf ("  sasl_key. .  . . . . : '%s'",
                                weechat_config_string (ptr_server->options[IRC_SERVER_OPTION_SASL_KEY]));
        /* sasl_fail */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_SASL_FAIL]))
            weechat_log_printf ("  sasl_fail. . . . . . : null ('%s')",
                                irc_server_sasl_fail_string[IRC_SERVER_OPTION_INTEGER(ptr_server, IRC_SERVER_OPTION_SASL_FAIL)]);
        else
            weechat_log_printf ("  sasl_fail. . . . . . : '%s'",
                                irc_server_sasl_fail_string[weechat_config_integer (ptr_server->options[IRC_SERVER_OPTION_SASL_FAIL])]);
        /* autoconnect */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_AUTOCONNECT]))
            weechat_log_printf ("  autoconnect. . . . . : null (%s)",
                                (IRC_SERVER_OPTION_BOOLEAN(ptr_server, IRC_SERVER_OPTION_AUTOCONNECT)) ?
                                "on" : "off");
        else
            weechat_log_printf ("  autoconnect. . . . . : %s",
                                (weechat_config_boolean (ptr_server->options[IRC_SERVER_OPTION_AUTOCONNECT])) ?
                                "on" : "off");
        /* autoreconnect */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_AUTORECONNECT]))
            weechat_log_printf ("  autoreconnect. . . . : null (%s)",
                                (IRC_SERVER_OPTION_BOOLEAN(ptr_server, IRC_SERVER_OPTION_AUTORECONNECT)) ?
                                "on" : "off");
        else
            weechat_log_printf ("  autoreconnect. . . . : %s",
                                (weechat_config_boolean (ptr_server->options[IRC_SERVER_OPTION_AUTORECONNECT])) ?
                                "on" : "off");
        /* autoreconnect_delay */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_AUTORECONNECT_DELAY]))
            weechat_log_printf ("  autoreconnect_delay. : null (%d)",
                                IRC_SERVER_OPTION_INTEGER(ptr_server, IRC_SERVER_OPTION_AUTORECONNECT_DELAY));
        else
            weechat_log_printf ("  autoreconnect_delay. : %d",
                                weechat_config_integer (ptr_server->options[IRC_SERVER_OPTION_AUTORECONNECT_DELAY]));
        /* nicks */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_NICKS]))
            weechat_log_printf ("  nicks. . . . . . . . : null ('%s')",
                                IRC_SERVER_OPTION_STRING(ptr_server, IRC_SERVER_OPTION_NICKS));
        else
            weechat_log_printf ("  nicks. . . . . . . . : '%s'",
                                weechat_config_string (ptr_server->options[IRC_SERVER_OPTION_NICKS]));
        /* nicks_alternate */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_NICKS_ALTERNATE]))
            weechat_log_printf ("  nicks_alternate. . . : null (%s)",
                                (IRC_SERVER_OPTION_BOOLEAN(ptr_server, IRC_SERVER_OPTION_NICKS_ALTERNATE)) ?
                                "on" : "off");
        else
            weechat_log_printf ("  nicks_alternate. . . : %s",
                                (weechat_config_boolean (ptr_server->options[IRC_SERVER_OPTION_NICKS_ALTERNATE])) ?
                                "on" : "off");
        /* username */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_USERNAME]))
            weechat_log_printf ("  username . . . . . . : null ('%s')",
                                IRC_SERVER_OPTION_STRING(ptr_server, IRC_SERVER_OPTION_USERNAME));
        else
            weechat_log_printf ("  username . . . . . . : '%s'",
                                weechat_config_string (ptr_server->options[IRC_SERVER_OPTION_USERNAME]));
        /* realname */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_REALNAME]))
            weechat_log_printf ("  realname . . . . . . : null ('%s')",
                                IRC_SERVER_OPTION_STRING(ptr_server, IRC_SERVER_OPTION_REALNAME));
        else
            weechat_log_printf ("  realname . . . . . . : '%s'",
                                weechat_config_string (ptr_server->options[IRC_SERVER_OPTION_REALNAME]));
        /* local_hostname */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_LOCAL_HOSTNAME]))
            weechat_log_printf ("  local_hostname . . . : null ('%s')",
                                IRC_SERVER_OPTION_STRING(ptr_server, IRC_SERVER_OPTION_LOCAL_HOSTNAME));
        else
            weechat_log_printf ("  local_hostname . . . : '%s'",
                                weechat_config_string (ptr_server->options[IRC_SERVER_OPTION_LOCAL_HOSTNAME]));
        /* command */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_COMMAND]))
            weechat_log_printf ("  command. . . . . . . : null");
        else
            weechat_log_printf ("  command. . . . . . . : (hidden)");
        /* command_delay */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_COMMAND_DELAY]))
            weechat_log_printf ("  command_delay. . . . : null (%d)",
                                IRC_SERVER_OPTION_INTEGER(ptr_server, IRC_SERVER_OPTION_COMMAND_DELAY));
        else
            weechat_log_printf ("  command_delay. . . . : %d",
                                weechat_config_integer (ptr_server->options[IRC_SERVER_OPTION_COMMAND_DELAY]));
        /* autojoin */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_AUTOJOIN]))
            weechat_log_printf ("  autojoin . . . . . . : null ('%s')",
                                IRC_SERVER_OPTION_STRING(ptr_server, IRC_SERVER_OPTION_AUTOJOIN));
        else
            weechat_log_printf ("  autojoin . . . . . . : '%s'",
                                weechat_config_string (ptr_server->options[IRC_SERVER_OPTION_AUTOJOIN]));
        /* autorejoin */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_AUTOREJOIN]))
            weechat_log_printf ("  autorejoin . . . . . : null (%s)",
                                (IRC_SERVER_OPTION_BOOLEAN(ptr_server, IRC_SERVER_OPTION_AUTOREJOIN)) ?
                                "on" : "off");
        else
            weechat_log_printf ("  autorejoin . . . . . : %s",
                                (weechat_config_boolean (ptr_server->options[IRC_SERVER_OPTION_AUTOREJOIN])) ?
                                "on" : "off");
        /* autorejoin_delay */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_AUTOREJOIN_DELAY]))
            weechat_log_printf ("  autorejoin_delay . . : null (%d)",
                                IRC_SERVER_OPTION_INTEGER(ptr_server, IRC_SERVER_OPTION_AUTOREJOIN_DELAY));
        else
            weechat_log_printf ("  autorejoin_delay . . : %d",
                                weechat_config_integer (ptr_server->options[IRC_SERVER_OPTION_AUTOREJOIN_DELAY]));
        /* connection_timeout */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_CONNECTION_TIMEOUT]))
            weechat_log_printf ("  connection_timeout . : null (%d)",
                                IRC_SERVER_OPTION_INTEGER(ptr_server, IRC_SERVER_OPTION_CONNECTION_TIMEOUT));
        else
            weechat_log_printf ("  connection_timeout . : %d",
                                weechat_config_integer (ptr_server->options[IRC_SERVER_OPTION_CONNECTION_TIMEOUT]));
        /* anti_flood_prio_high */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_ANTI_FLOOD_PRIO_HIGH]))
            weechat_log_printf ("  anti_flood_prio_high : null (%d)",
                                IRC_SERVER_OPTION_INTEGER(ptr_server, IRC_SERVER_OPTION_ANTI_FLOOD_PRIO_HIGH));
        else
            weechat_log_printf ("  anti_flood_prio_high : %d",
                                weechat_config_integer (ptr_server->options[IRC_SERVER_OPTION_ANTI_FLOOD_PRIO_HIGH]));
        /* anti_flood_prio_low */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_ANTI_FLOOD_PRIO_LOW]))
            weechat_log_printf ("  anti_flood_prio_low. : null (%d)",
                                IRC_SERVER_OPTION_INTEGER(ptr_server, IRC_SERVER_OPTION_ANTI_FLOOD_PRIO_LOW));
        else
            weechat_log_printf ("  anti_flood_prio_low. : %d",
                                weechat_config_integer (ptr_server->options[IRC_SERVER_OPTION_ANTI_FLOOD_PRIO_LOW]));
        /* away_check */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_AWAY_CHECK]))
            weechat_log_printf ("  away_check . . . . . : null (%d)",
                                IRC_SERVER_OPTION_INTEGER(ptr_server, IRC_SERVER_OPTION_AWAY_CHECK));
        else
            weechat_log_printf ("  away_check . . . . . : %d",
                                weechat_config_integer (ptr_server->options[IRC_SERVER_OPTION_AWAY_CHECK]));
        /* away_check_max_nicks */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_AWAY_CHECK_MAX_NICKS]))
            weechat_log_printf ("  away_check_max_nicks : null (%d)",
                                IRC_SERVER_OPTION_INTEGER(ptr_server, IRC_SERVER_OPTION_AWAY_CHECK_MAX_NICKS));
        else
            weechat_log_printf ("  away_check_max_nicks : %d",
                                weechat_config_integer (ptr_server->options[IRC_SERVER_OPTION_AWAY_CHECK_MAX_NICKS]));
        /* default_msg_kick */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_DEFAULT_MSG_KICK]))
            weechat_log_printf ("  default_msg_kick . . : null ('%s')",
                                IRC_SERVER_OPTION_STRING(ptr_server, IRC_SERVER_OPTION_DEFAULT_MSG_KICK));
        else
            weechat_log_printf ("  default_msg_kick . . : '%s'",
                                weechat_config_string (ptr_server->options[IRC_SERVER_OPTION_DEFAULT_MSG_KICK]));
        /* default_msg_part */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_DEFAULT_MSG_PART]))
            weechat_log_printf ("  default_msg_part . . : null ('%s')",
                                IRC_SERVER_OPTION_STRING(ptr_server, IRC_SERVER_OPTION_DEFAULT_MSG_PART));
        else
            weechat_log_printf ("  default_msg_part . . : '%s'",
                                weechat_config_string (ptr_server->options[IRC_SERVER_OPTION_DEFAULT_MSG_PART]));
        /* default_msg_quit */
        if (weechat_config_option_is_null (ptr_server->options[IRC_SERVER_OPTION_DEFAULT_MSG_QUIT]))
            weechat_log_printf ("  default_msg_quit . . : null ('%s')",
                                IRC_SERVER_OPTION_STRING(ptr_server, IRC_SERVER_OPTION_DEFAULT_MSG_QUIT));
        else
            weechat_log_printf ("  default_msg_quit . . : '%s'",
                                weechat_config_string (ptr_server->options[IRC_SERVER_OPTION_DEFAULT_MSG_QUIT]));
        /* other server variables */
        weechat_log_printf ("  temp_server. . . . . : %d",    ptr_server->temp_server);
        weechat_log_printf ("  reloading_from_config: %d",    ptr_server->reloaded_from_config);
        weechat_log_printf ("  reloaded_from_config : %d",    ptr_server->reloaded_from_config);
        weechat_log_printf ("  addresses_count. . . : %d",    ptr_server->addresses_count);
        weechat_log_printf ("  addresses_array. . . : 0x%lx", ptr_server->addresses_array);
        weechat_log_printf ("  ports_array. . . . . : 0x%lx", ptr_server->ports_array);
        weechat_log_printf ("  retry_array. . . . . : 0x%lx", ptr_server->retry_array);
        weechat_log_printf ("  index_current_address: %d",    ptr_server->index_current_address);
        weechat_log_printf ("  current_address. . . : '%s'",  ptr_server->current_address);
        weechat_log_printf ("  current_ip . . . . . : '%s'",  ptr_server->current_ip);
        weechat_log_printf ("  current_port . . . . : %d",    ptr_server->current_port);
        weechat_log_printf ("  current_retry. . . . : %d",    ptr_server->current_retry);
        weechat_log_printf ("  sock . . . . . . . . : %d",    ptr_server->sock);
        weechat_log_printf ("  hook_connect . . . . : 0x%lx", ptr_server->hook_connect);
        weechat_log_printf ("  hook_fd. . . . . . . : 0x%lx", ptr_server->hook_fd);
        weechat_log_printf ("  hook_timer_connection: 0x%lx", ptr_server->hook_timer_connection);
        weechat_log_printf ("  hook_timer_sasl. . . : 0x%lx", ptr_server->hook_timer_sasl);
        weechat_log_printf ("  is_connected . . . . : %d",    ptr_server->is_connected);
        weechat_log_printf ("  ssl_connected. . . . : %d",    ptr_server->ssl_connected);
        weechat_log_printf ("  disconnected . . . . : %d",    ptr_server->disconnected);
#ifdef HAVE_GNUTLS
        weechat_log_printf ("  gnutls_sess. . . . . : 0x%lx", ptr_server->gnutls_sess);
#endif /* HAVE_GNUTLS */
        weechat_log_printf ("  unterminated_message : '%s'",  ptr_server->unterminated_message);
        weechat_log_printf ("  nicks_count. . . . . : %d",    ptr_server->nicks_count);
        weechat_log_printf ("  nicks_array. . . . . : 0x%lx", ptr_server->nicks_array);
        weechat_log_printf ("  nick_first_tried . . : %d",    ptr_server->nick_first_tried);
        weechat_log_printf ("  nick_alternate_number: %d",    ptr_server->nick_alternate_number);
        weechat_log_printf ("  nick . . . . . . . . : '%s'",  ptr_server->nick);
        weechat_log_printf ("  nick_modes . . . . . : '%s'",  ptr_server->nick_modes);
        weechat_log_printf ("  cap_away_notify. . . : %d",    ptr_server->cap_away_notify);
        weechat_log_printf ("  cap_account_notify . : %d",    ptr_server->cap_account_notify);
        weechat_log_printf ("  cap_extended_join. . : %d",    ptr_server->cap_extended_join);
        weechat_log_printf ("  isupport . . . . . . : '%s'",  ptr_server->isupport);
        weechat_log_printf ("  prefix_modes . . . . : '%s'",  ptr_server->prefix_modes);
        weechat_log_printf ("  prefix_chars . . . . : '%s'",  ptr_server->prefix_chars);
        weechat_log_printf ("  nick_max_length. . . : %d",    ptr_server->nick_max_length);
        weechat_log_printf ("  casemapping. . . . . : %d (%s)",
                            ptr_server->casemapping,
                            irc_server_casemapping_string[ptr_server->casemapping]);
        weechat_log_printf ("  chantypes. . . . . . : '%s'",  ptr_server->chantypes);
        weechat_log_printf ("  chanmodes. . . . . . : '%s'",  ptr_server->chanmodes);
        weechat_log_printf ("  monitor. . . . . . . : %d",    ptr_server->monitor);
        weechat_log_printf ("  monitor_time . . . . : %ld",   ptr_server->monitor_time);
        weechat_log_printf ("  reconnect_delay. . . : %d",    ptr_server->reconnect_delay);
        weechat_log_printf ("  reconnect_start. . . : %ld",   ptr_server->reconnect_start);
        weechat_log_printf ("  command_time . . . . : %ld",   ptr_server->command_time);
        weechat_log_printf ("  reconnect_join . . . : %d",    ptr_server->reconnect_join);
        weechat_log_printf ("  disable_autojoin . . : %d",    ptr_server->disable_autojoin);
        weechat_log_printf ("  is_away. . . . . . . : %d",    ptr_server->is_away);
        weechat_log_printf ("  away_message . . . . : '%s'",  ptr_server->away_message);
        weechat_log_printf ("  away_time. . . . . . : %ld",   ptr_server->away_time);
        weechat_log_printf ("  lag. . . . . . . . . : %d",    ptr_server->lag);
        weechat_log_printf ("  lag_displayed. . . . : %d",    ptr_server->lag_displayed);
        weechat_log_printf ("  lag_check_time . . . : tv_sec:%d, tv_usec:%d",
                            ptr_server->lag_check_time.tv_sec,
                            ptr_server->lag_check_time.tv_usec);
        weechat_log_printf ("  lag_next_check . . . : %ld",   ptr_server->lag_next_check);
        weechat_log_printf ("  lag_last_refresh . . : %ld",   ptr_server->lag_last_refresh);
        weechat_log_printf ("  cmd_list_regexp. . . : 0x%lx", ptr_server->cmd_list_regexp);
        weechat_log_printf ("  last_user_message. . : %ld",   ptr_server->last_user_message);
        weechat_log_printf ("  last_away_check. . . : %ld",   ptr_server->last_away_check);
        weechat_log_printf ("  last_data_purge. . . : %ld",   ptr_server->last_data_purge);
        for (i = 0; i < IRC_SERVER_NUM_OUTQUEUES_PRIO; i++)
        {
            weechat_log_printf ("  outqueue[%02d] . . . . : 0x%lx", i, ptr_server->outqueue[i]);
            weechat_log_printf ("  last_outqueue[%02d]. . : 0x%lx", i, ptr_server->last_outqueue[i]);
        }
        weechat_log_printf ("  redirects. . . . . . : 0x%lx", ptr_server->redirects);
        weechat_log_printf ("  last_redirect. . . . : 0x%lx", ptr_server->last_redirect);
        weechat_log_printf ("  notify_list. . . . . : 0x%lx", ptr_server->notify_list);
        weechat_log_printf ("  last_notify. . . . . : 0x%lx", ptr_server->last_notify);
        weechat_log_printf ("  notify_count . . . . : %d",    ptr_server->notify_count);
        weechat_log_printf ("  join_manual. . . . . : 0x%lx (hashtable: '%s')",
                            ptr_server->join_manual,
                            weechat_hashtable_get_string (ptr_server->join_manual, "keys_values"));
        weechat_log_printf ("  join_channel_key . . : 0x%lx (hashtable: '%s')",
                            ptr_server->join_channel_key,
                            weechat_hashtable_get_string (ptr_server->join_channel_key, "keys_values"));
        weechat_log_printf ("  join_noswitch. . . . : 0x%lx (hashtable: '%s')",
                            ptr_server->join_noswitch,
                            weechat_hashtable_get_string (ptr_server->join_noswitch, "keys_values"));
        weechat_log_printf ("  buffer . . . . . . . : 0x%lx", ptr_server->buffer);
        weechat_log_printf ("  buffer_as_string . . : 0x%lx", ptr_server->buffer_as_string);
        weechat_log_printf ("  channels . . . . . . : 0x%lx", ptr_server->channels);
        weechat_log_printf ("  last_channel . . . . : 0x%lx", ptr_server->last_channel);
        weechat_log_printf ("  prev_server. . . . . : 0x%lx", ptr_server->prev_server);
        weechat_log_printf ("  next_server. . . . . : 0x%lx", ptr_server->next_server);

        irc_redirect_print_log (ptr_server);

        irc_notify_print_log (ptr_server);

        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            irc_channel_print_log (ptr_channel);
        }
    }
}
