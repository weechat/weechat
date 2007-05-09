/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* weechat-charset.c: Charset plugin for WeeChat */

#include <stdio.h>
#include <stdlib.h>
#define __USE_GNU
#include <string.h>
#include <iconv.h>

#include "../weechat-plugin.h"
#include "weechat-charset.h"


char *weechat_charset_terminal = NULL;
char *weechat_charset_internal = NULL;

/* set to 1 by /charset debug (hidden option) */
int weechat_charset_debug = 0;


/*
 * weechat_charset_strndup: define strndup function if not existing (FreeBSD and maybe other)
 */

char *
weechat_charset_strndup (char *string, int length)
{
    char *result;
    
    if ((int)strlen (string) < length)
        return strdup (string);
    
    result = (char *)malloc (length + 1);
    if (!result)
        return NULL;
    
    memcpy (result, string, length);
    result[length] = '\0';
    
    return result;
}

/*
 * weechat_charset_default_decode: set "global.decode" option if needed
 */

void
weechat_charset_default_decode (t_weechat_plugin *plugin)
{
    char *global_decode;
    int rc;
    
    global_decode = plugin->get_plugin_config (plugin, "global.decode");
    
    /* if global decode is not set, we may set it, depending on terminal charset */
    if (!global_decode || !global_decode[0])
    {
        /* set global decode charset to terminal if different from internal */
        /* otherwise use ISO-8859-1 */
        if (weechat_charset_terminal && weechat_charset_internal
            && (strcasecmp (weechat_charset_terminal, weechat_charset_internal) != 0))
            rc = plugin->set_plugin_config (plugin,
                                            "global.decode",
                                            weechat_charset_terminal);
        else
            rc = plugin->set_plugin_config (plugin,
                                            "global.decode",
                                            "ISO-8859-1");
        if (rc)
            plugin->print_server (plugin,
                                  "Charset: setting \"charset.global.decode\" to %s",
                                  weechat_charset_terminal);
        else
            plugin->print_server (plugin,
                                  "Charset: failed to set \"charset.global.decode\" option.");
    }
    if (global_decode)
        free (global_decode);
}

/*
 * weechat_charset_check: check if a charset is valid
 *                        if a charset is NULL, internal charset is used
 */

int
weechat_charset_check (char *charset)
{
    iconv_t cd;

    if (!charset || !charset[0])
        return 0;
    
    cd = iconv_open (charset, weechat_charset_internal);
    if (cd == (iconv_t)(-1))
        return 0;
    
    iconv_close (cd);
    return 1;
}

/*
 * weechat_charset_get_config: read a charset in config file
 *                             we first in this order: channel, server, global
 */

char *
weechat_charset_get_config (t_weechat_plugin *plugin,
                            char *type, char *server, char *channel)
{
    static char option[1024];
    char *value;

    /* we try channel first */
    if (server && channel)
    {
        snprintf (option, sizeof (option) - 1, "%s.%s.%s", type, server, channel);
        value = plugin->get_plugin_config (plugin, option);
        if (value && value[0])
            return value;
        if (value)
            free (value);
    }
    
    /* channel not found, we try server only */
    if (server)
    {
        snprintf (option, sizeof (option) - 1, "%s.%s", type, server);
        value = plugin->get_plugin_config (plugin, option);
        if (value && value[0])
            return value;
        if (value)
            free (value);
    }
    
    /* nothing found, we try global charset */
    snprintf (option, sizeof (option) - 1, "global.%s", type);
    value = plugin->get_plugin_config (plugin, option);
    if (value && value[0])
        return value;
    if (value)
        free (value);
    
    /* nothing found => no decode/encode for this message! */
    return NULL;
}

/*
 * weechat_charset_set_config: set a charset in config file
 */

void
weechat_charset_set_config (t_weechat_plugin *plugin,
                            char *type, char *server, char *channel, char *value)
{
    static char option[1024];
    
    if (server && channel)
        snprintf (option, sizeof (option) - 1, "%s.%s.%s", type, server, channel);
    else if (server)
        snprintf (option, sizeof (option) - 1, "%s.%s", type, server);
    else
        return;

    plugin->set_plugin_config (plugin, option, value);
}

/*
 * weechat_charset_parse_irc_msg: return nick, command, channel and position
 *                                of arguments in IRC message
 */

void
weechat_charset_parse_irc_msg (char *message, char **nick, char **command,
                               char **channel, char **pos_args)
{
    char *pos, *pos2, *pos3, *pos4, *pos_tmp;
    
    *nick = NULL;
    *command = NULL;
    *channel = NULL;
    *pos_args = NULL;
    
    if (message[0] == ':')
    {
        pos = message + 1;
        pos_tmp = strchr (pos, ' ');
        if (pos_tmp)
            pos_tmp[0] = '\0';
        pos2 = strchr (pos, '!');
        if (pos2)
            *nick = weechat_charset_strndup (pos, pos2 - pos);
        else
        {
            pos2 = strchr (pos, ' ');
            if (pos2)
                *nick = weechat_charset_strndup (pos, pos2 - pos);
        }
        if (pos_tmp)
            pos_tmp[0] = ' ';
        pos = strchr (message, ' ');
        if (!pos)
            pos = message;
    }
    else
        pos = message;
    
    if (pos && pos[0])
    {
        while (pos[0] == ' ')
            pos++;
        pos2 = strchr (pos, ' ');
        if (pos2)
        {
            *command = weechat_charset_strndup (pos, pos2 - pos);
            pos2++;
            while (pos2[0] == ' ')
                pos2++;
            *pos_args = pos2;
            if (pos2[0] != ':')
            {
                if ((pos2[0] == '#') || (pos2[0] == '&')
                    || (pos2[0] == '+') || (pos2[0] == '!'))
                {
                    pos3 = strchr (pos2, ' ');
                    if (pos3)
                        *channel = weechat_charset_strndup (pos2, pos3 - pos2);
                    else
                        *channel = strdup (pos2);
                }
                else
                {
                    pos3 = strchr (pos2, ' ');
                    if (!*nick)
                    {
                        if (pos3)
                            *nick = weechat_charset_strndup (pos2, pos3 - pos2);
                        else
                            *nick = strdup (pos2);
                    }
                    if (pos3)
                    {
                        pos3++;
                        while (pos3[0] == ' ')
                            pos3++;
                        if ((pos3[0] == '#') || (pos3[0] == '&')
                            || (pos3[0] == '+') || (pos3[0] == '!'))
                        {
                            pos4 = strchr (pos3, ' ');
                            if (pos4)
                                *channel = weechat_charset_strndup (pos3, pos4 - pos3);
                            else
                                *channel = strdup (pos3);
                        }
                    }
                }
            }
        }
    }
}

/*
 * weechat_charset_irc_in: transform charset for incoming messages
 *                         convert from any charset to WeeChat internal
 */

char *
weechat_charset_irc_in (t_weechat_plugin *plugin, int argc, char **argv,
                        char *handler_args, void *handler_pointer)
{
    char *nick, *command, *channel, *charset, *ptr_args;
    char *output;
    
    /* make C compiler happy */
    (void) argc;
    (void) handler_args;
    (void) handler_pointer;
    
    output = NULL;
    
    weechat_charset_parse_irc_msg (argv[1], &nick, &command, &channel, &ptr_args);
    
    charset = weechat_charset_get_config (plugin,
                                          "decode", argv[0],
                                          (channel) ? channel : nick);
    
    if (weechat_charset_debug)
        plugin->print(plugin, NULL, NULL,
                      "Charset IN: srv='%s', nick='%s', chan='%s', "
                      "msg='%s', ptr_args='%s' => charset: %s",
                      argv[0], nick, channel, argv[1], ptr_args, charset);
    
    if (charset)
    {
        output = plugin->iconv_to_internal (plugin, charset, argv[1]);
        if (charset)
            free (charset);
    }
    
    if (nick)
        free (nick);
    if (command)
        free (command);
    if (channel)
        free (channel);
    
    return output;
}

/*
 * weechat_charset_irc_out: transform charset for outgoing messages
 *                          convert from WeeChat internal charset to other
 */

char *
weechat_charset_irc_out (t_weechat_plugin *plugin, int argc, char **argv,
                         char *handler_args, void *handler_pointer)
{
    char *nick, *command, *channel, *charset, *ptr_args;
    char *output;
    
    /* make C compiler happy */
    (void) argc;
    (void) handler_args;
    (void) handler_pointer;
    
    output = NULL;
    
    weechat_charset_parse_irc_msg (argv[1], &nick, &command, &channel, &ptr_args);
    
    charset = weechat_charset_get_config (plugin,
                                          "encode", argv[0],
                                          (channel) ? channel : nick);
    
    if (weechat_charset_debug)
        plugin->print(plugin, NULL, NULL,
                      "Charset OUT: srv='%s', nick='%s', chan='%s', "
                      "msg='%s', ptr_args='%s' => charset: %s",
                      argv[0], nick, channel, argv[1], ptr_args, charset);

    if (charset)
    {
        output = plugin->iconv_from_internal (plugin, charset, argv[1]);
        if (charset)
            free (charset);
    }
    
    if (nick)
        free (nick);
    if (command)
        free (command);
    if (channel)
        free (channel);
    
    return output;
}

/*
 * weechat_charset_display: display charsets (global/server/channel)
 */

void
weechat_charset_display (t_weechat_plugin *plugin,
                         int display_on_server, char *server, char *channel)
{
    char *decode, *encode;
    static char option[1024];

    decode = NULL;
    encode = NULL;
    
    /* display global settings */
    if (!server && !channel)
    {
        decode = plugin->get_plugin_config (plugin, "global.decode");
        encode = plugin->get_plugin_config (plugin, "global.encode");

        if (display_on_server)
            plugin->print_server (plugin,
                                  "Charset: global charsets: decode = %s, encode = %s",
                                  (decode) ? decode : "(none)",
                                  (encode) ? encode : "(none)");
        else
            plugin->print (plugin, NULL, NULL,
                           "Charset: global charsets: decode = %s, encode = %s",
                           (decode) ? decode : "(none)",
                           (encode) ? encode : "(none)");
    }
    
    /* display server settings */
    if (server && !channel)
    {
        snprintf (option, sizeof (option) - 1, "decode.%s", server);
        decode = plugin->get_plugin_config (plugin, option);
        snprintf (option, sizeof (option) - 1, "encode.%s", server);
        encode = plugin->get_plugin_config (plugin, option);

        if (display_on_server)
            plugin->print_server (plugin,
                                  "Charset: decode / encode charset for server %s: %s / %s",
                                  server,
                                  (decode) ? decode : "(none)",
                                  (encode) ? encode : "(none)");
        else
            plugin->print (plugin, NULL, NULL,
                           "Charset: decode / encode charset for server %s: %s / %s",
                           server,
                           (decode) ? decode : "(none)",
                           (encode) ? encode : "(none)");
    }
    
    /* display chan/nick settings */
    if (server && channel)
    {
        snprintf (option, sizeof (option) - 1, "decode.%s.%s", server, channel);
        decode = plugin->get_plugin_config (plugin, option);
        snprintf (option, sizeof (option) - 1, "encode.%s.%s", server, channel);
        encode = plugin->get_plugin_config (plugin, option);

        if (display_on_server)
            plugin->print_server (plugin,
                                  "Charset: decode / encode charset for %s/%s: %s / %s",
                                  server, channel,
                                  (decode) ? decode : "(none)",
                                  (encode) ? encode : "(none)");
        else
            plugin->print (plugin, NULL, NULL,
                           "Charset: decode / encode charset for %s/%s: %s / %s",
                           server, channel,
                           (decode) ? decode : "(none)",
                           (encode) ? encode : "(none)");
    }
    
    if (decode)
        free (decode);
    if (encode)
        free (encode);
}

/*
 * weechat_charset_cmd: /charset command
 */

int
weechat_charset_cmd (t_weechat_plugin *plugin,
                     int cmd_argc, char **cmd_argv,
                     char *handler_args, void *handler_pointer)
{
    int argc;
    char **argv, *server, *channel;
    
    if (cmd_argc < 3)
        return PLUGIN_RC_KO;
    
    /* make C compiler happy */
    (void) handler_args;
    (void) handler_pointer;
    
    if (cmd_argv[2])
        argv = plugin->explode_string (plugin, cmd_argv[2], " ", 0, &argc);
    else
    {
        argv = NULL;
        argc = 0;
    }
    
    /* get command context */
    server = plugin->get_info (plugin, "server", NULL);
    channel = plugin->get_info (plugin, "channel", NULL);
    
    switch (argc)
    {
        case 0:
            plugin->print_server (plugin, "");
            weechat_charset_display (plugin, 1, NULL, NULL);
            weechat_charset_display (plugin, 1, server, NULL);
            if (channel)
                weechat_charset_display (plugin, 1, server, channel);
            break;
        case 1:
            if (strcasecmp (argv[0], "decode") == 0)
            {
                weechat_charset_set_config (plugin, "decode", server, channel, NULL);
                weechat_charset_display (plugin, 0, server, channel);
            }
            else if (strcasecmp (argv[0], "encode") == 0)
            {
                weechat_charset_set_config (plugin, "encode", server, channel, NULL);
                weechat_charset_display (plugin, 0, server, channel);
            }
            else if (strcasecmp (argv[0], "debug") == 0)
            {
                weechat_charset_debug ^= 1;
                plugin->print (plugin, NULL, NULL,
                               "Charset: debug [%s].",
                               (weechat_charset_debug) ? "ON" : "off");
            }
            else if (strcasecmp (argv[0], "reset") == 0)
            {
                weechat_charset_set_config (plugin, "decode", server, channel, NULL);
                weechat_charset_set_config (plugin, "encode", server, channel, NULL);
                weechat_charset_display (plugin, 0, server, channel);
            }
            else
            {
                if (!weechat_charset_check (argv[0]))
                    plugin->print_server (plugin,
                                          "Charset error: invalid charset \"%s\"",
                                          argv[0]);
                else
                {
                    weechat_charset_set_config (plugin, "decode", server, channel, argv[0]);
                    weechat_charset_set_config (plugin, "encode", server, channel, argv[0]);
                    weechat_charset_display (plugin, 0, server, channel);
                }
            }
            break;
        case 2:
            if (!weechat_charset_check (argv[1]))
                plugin->print_server (plugin,
                                      "Charset error: invalid charset \"%s\"",
                                      argv[1]);
            else
            {
                if (strcasecmp (argv[0], "decode") == 0)
                {
                    weechat_charset_set_config (plugin, "decode", server, channel, argv[1]);
                    weechat_charset_display (plugin, 0, server, channel);
                }
                else if (strcasecmp (argv[0], "encode") == 0)
                {
                    weechat_charset_set_config (plugin, "encode", server, channel, argv[1]);
                    weechat_charset_display (plugin, 0, server, channel);
                }
                else
                    plugin->print_server (plugin,
                                          "Charset error: unknown option \"%s\"",
                                          argv[0]);
            }
            break;
    }

    if (argv)
        plugin->free_exploded_string (plugin, argv);
    if (server)
        free (server);
    if (channel)
        free (channel);
    
    return PLUGIN_RC_OK;
}
    
/*
 * weechat_plugin_init: init charset plugin
 */

int
weechat_plugin_init (t_weechat_plugin *plugin)
{
    t_plugin_modifier *msg_irc_in, *msg_irc_out;
    t_plugin_handler *cmd_handler;

    /* get terminal & internal charsets */
    weechat_charset_terminal = plugin->get_info (plugin, "charset_terminal", NULL);
    weechat_charset_internal = plugin->get_info (plugin, "charset_internal", NULL);

    /* display message */
    plugin->print_server (plugin, "Charset plugin starting, terminal charset: %s (WeeChat internal: %s)",
                          weechat_charset_terminal, weechat_charset_internal);
    
    /* set global default decode charset */
    weechat_charset_default_decode (plugin);
    
    /* add command handler */
    cmd_handler = plugin->cmd_handler_add (plugin, "charset",
                                           "Charset management by server or channel",
                                           "[[decode | encode] charset] | [reset]",
                                           " decode: set a decoding charset for server/channel\n"
                                           " encode: set an encofing charset for server/channel\n"
                                           "charset: the charset for decoding or encoding messages\n"
                                           "  reset: reset charsets for server/channel\n\n"
                                           "To set global decode/encode charset (for all servers), use /setp charset.global.decode "
                                           "or /setp charset.global.encode\n"
                                           "To see all charsets for all servers, use /setp charset",
                                           "decode|encode|reset",
                                           &weechat_charset_cmd,
                                           NULL, NULL);
    
    /* add messge modifier */
    msg_irc_in = plugin->modifier_add (plugin, "irc_in", "*",
                                       &weechat_charset_irc_in,
                                       NULL, NULL);
    msg_irc_out = plugin->modifier_add (plugin, "irc_out", "*",
                                        &weechat_charset_irc_out,
                                        NULL, NULL);
    
    return PLUGIN_RC_OK;
}

/*
 * weechat_plugin_end: end charset plugin
 */

void
weechat_plugin_end (t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;
    
    if (weechat_charset_terminal)
        free (weechat_charset_terminal);
    if (weechat_charset_internal)
        free (weechat_charset_internal);
}
