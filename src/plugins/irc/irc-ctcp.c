/*
 * Copyright (C) 2003-2011 Sebastien Helleu <flashcode@flashtux.org>
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

/*
 * irc-ctcp.c: IRC CTCP protocol
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/utsname.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-ctcp.h"
#include "irc-channel.h"
#include "irc-color.h"
#include "irc-config.h"
#include "irc-nick.h"
#include "irc-protocol.h"
#include "irc-server.h"


struct t_irc_ctcp_reply irc_ctcp_default_reply[] =
{ { "clientinfo", "$clientinfo" },
  { "finger",     "WeeChat $version" },
  { "source",     "$download" },
  { "time",       "$time" },
  { "userinfo",   "$username ($realname)" },
  { "version",    "WeeChat $version ($compilation)" },
  { NULL,         NULL },
};


/*
 * irc_ctcp_get_default_reply: get default reply for a CTCP query
 *                             return NULL if CTCP is unknown
 */

const char *
irc_ctcp_get_default_reply (const char *ctcp)
{
    int i;
    
    for (i = 0; irc_ctcp_default_reply[i].name; i++)
    {
        if (weechat_strcasecmp (irc_ctcp_default_reply[i].name, ctcp) == 0)
            return irc_ctcp_default_reply[i].reply;
    }
    
    /* unknown CTCP */
    return NULL;
}

/*
 * irc_ctcp_get_reply: get reply for a CTCP query
 */

const char *
irc_ctcp_get_reply (struct t_irc_server *server, const char *ctcp)
{
    struct t_config_option *ptr_option;
    char option_name[512];

    snprintf (option_name, sizeof (option_name), "%s.%s", server->name, ctcp);
    
    /* search for CTCP in config file, for server */
    ptr_option = weechat_config_search_option (irc_config_file,
                                               irc_config_section_ctcp,
                                               option_name);
    if (ptr_option)
        return weechat_config_string (ptr_option);
    
    /* search for CTCP in config file */
    ptr_option = weechat_config_search_option (irc_config_file,
                                               irc_config_section_ctcp,
                                               ctcp);
    if (ptr_option)
        return weechat_config_string (ptr_option);
    
    /*
     * no CTCP reply found in config, then return default reply, or NULL
     * for unknown CTCP
     */
    return irc_ctcp_get_default_reply (ctcp);
}

/*
 * irc_ctcp_display_request: display CTCP requested by a nick
 */

void
irc_ctcp_display_request (struct t_irc_server *server,
                          const char *command,
                          struct t_irc_channel *channel,
                          const char *nick,  const char *ctcp,
                          const char *arguments,
                          const char *reply)
{
    /* CTCP blocked and user doesn't want to see message? then just return */
    if (reply && !reply[0]
        && !weechat_config_boolean (irc_config_look_display_ctcp_blocked))
        return;
    
    weechat_printf_tags ((channel) ? channel->buffer : server->buffer,
                         irc_protocol_tags (command, "irc_ctcp", NULL),
                         _("%sCTCP requested by %s%s%s: %s%s%s%s%s%s"),
                         weechat_prefix ("network"),
                         IRC_COLOR_CHAT_NICK,
                         nick,
                         IRC_COLOR_CHAT,
                         IRC_COLOR_CHAT_CHANNEL,
                         ctcp,
                         IRC_COLOR_CHAT,
                         (arguments) ? " " : "",
                         (arguments) ? arguments : "",
                         (reply && !reply[0]) ? _(" (blocked)") : "");
}

/*
 * irc_ctcp_display_reply_from_nick: display reply from a nick to a CTCP query
 */

void
irc_ctcp_display_reply_from_nick (struct t_irc_server *server,
                                  const char *command, const char *nick,
                                  char *arguments)
{
    char *pos_end, *pos_space, *pos_args, *pos_usec;
    struct timeval tv;
    long sec1, usec1, sec2, usec2, difftime;
    
    while (arguments && arguments[0])
    {
        pos_end = strchr (arguments + 1, '\01');
        if (pos_end)
            pos_end[0] = '\0';
        
        pos_space = strchr (arguments + 1, ' ');
        if (pos_space)
        {
            pos_space[0] = '\0';
            pos_args = pos_space + 1;
            while (pos_args[0] == ' ')
            {
                pos_args++;
            }
            if (strcmp (arguments + 1, "PING") == 0)
            {
                pos_usec = strchr (pos_args, ' ');
                if (pos_usec)
                {
                    pos_usec[0] = '\0';
                    
                    gettimeofday (&tv, NULL);
                    sec1 = atol (pos_args);
                    usec1 = atol (pos_usec + 1);
                    sec2 = tv.tv_sec;
                    usec2 = tv.tv_usec;
                    
                    difftime = ((sec2 * 1000000) + usec2) -
                        ((sec1 * 1000000) + usec1);
                    
                    weechat_printf_tags (server->buffer,
                                         irc_protocol_tags (command,
                                                            "irc_ctcp",
                                                            NULL),
                                         _("%sCTCP reply from %s%s%s: %s%s%s "
                                           "%ld.%ld %s"),
                                         weechat_prefix ("network"),
                                         IRC_COLOR_CHAT_NICK,
                                         nick,
                                         IRC_COLOR_CHAT,
                                         IRC_COLOR_CHAT_CHANNEL,
                                         arguments + 1,
                                         IRC_COLOR_CHAT,
                                         difftime / 1000000,
                                         (difftime % 1000000) / 1000,
                                         (NG_("second", "seconds",
                                              (difftime / 1000000))));
                    
                    pos_usec[0] = ' ';
                }
            }
            else
            {
                weechat_printf_tags (server->buffer,
                                     irc_protocol_tags (command,
                                                        "irc_ctcp",
                                                        NULL),
                                     _("%sCTCP reply from %s%s%s: %s%s%s%s%s"),
                                     weechat_prefix ("network"),
                                     IRC_COLOR_CHAT_NICK,
                                     nick,
                                     IRC_COLOR_CHAT,
                                     IRC_COLOR_CHAT_CHANNEL,
                                     arguments + 1,
                                     IRC_COLOR_CHAT,
                                     " ",
                                     pos_args);
            }
            pos_space[0] = ' ';
        }
        else
        {
            weechat_printf_tags (server->buffer,
                                 irc_protocol_tags (command, NULL, NULL),
                                 _("%sCTCP reply from %s%s%s: %s%s%s%s%s"),
                                 weechat_prefix ("network"),
                                 IRC_COLOR_CHAT_NICK,
                                 nick,
                                 IRC_COLOR_CHAT,
                                 IRC_COLOR_CHAT_CHANNEL,
                                 arguments + 1,
                                 "",
                                 "",
                                 "");
        }
        
        if (pos_end)
            pos_end[0] = '\01';
        
        arguments = (pos_end) ? pos_end + 1 : NULL;
    }
}

/*
 * irc_ctcp_reply_to_nick: display CTCP replied to a nick
 */

void
irc_ctcp_reply_to_nick (struct t_irc_server *server,
                        const char *command,
                        struct t_irc_channel *channel,
                        const char *nick, const char *ctcp,
                        const char *arguments)
{
    irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_LOW, NULL,
                      "NOTICE %s :\01%s%s%s\01",
                      nick, ctcp,
                      (arguments) ? " " : "",
                      (arguments) ? arguments : "");

    if (weechat_config_boolean (irc_config_look_display_ctcp_reply))
    {
        weechat_printf_tags ((channel) ? channel->buffer : server->buffer,
                             irc_protocol_tags (command,
                                                "irc_ctcp,irc_ctcp_reply,no_highlight",
                                                NULL),
                             _("%sCTCP reply to %s%s%s: %s%s%s%s%s"),
                             weechat_prefix ("network"),
                             IRC_COLOR_CHAT_NICK,
                             nick,
                             IRC_COLOR_CHAT,
                             IRC_COLOR_CHAT_CHANNEL,
                             ctcp,
                             (arguments) ? IRC_COLOR_CHAT : "",
                             (arguments) ? " " : "",
                             (arguments) ? arguments : "");
    }
}

/*
 * irc_ctcp_replace_variables: replace variables in CTCP format
 */

char *
irc_ctcp_replace_variables (struct t_irc_server *server, const char *format)
{
    char *res, *temp;
    const char *info;
    time_t now;
    char buf[1024];
    struct utsname *buf_uname;
    
    /* clientinfo */
    temp = weechat_string_replace (format, "$clientinfo",
                                   "ACTION DCC CLIENTINFO FINGER PING SOURCE "
                                   "TIME USERINFO VERSION");
    if (!temp)
        return NULL;
    res = temp;
    
    /* version */
    info = weechat_info_get ("version", "");
    temp = weechat_string_replace (res, "$version", info);
    free (res);
    if (!temp)
        return NULL;
    res = temp;
    
    /* compilation date */
    info = weechat_info_get ("date", "");
    temp = weechat_string_replace (res, "$compilation", info);
    free (res);
    if (!temp)
        return NULL;
    res = temp;
    
    /* info about OS */
    buf_uname = (struct utsname *)malloc (sizeof (struct utsname));
    if (buf_uname && (uname (buf_uname) >= 0))
    {
        snprintf (buf, sizeof (buf), "%s %s / %s",
                  buf_uname->sysname, buf_uname->release,
                  buf_uname->machine);
        free (buf_uname);
        temp = weechat_string_replace (res, "$osinfo", buf);
        free (res);
        if (!temp)
            return NULL;
        res = temp;
    }
    
    /* site */
    info = weechat_info_get ("weechat_site", "");
    temp = weechat_string_replace (res, "$site", info);
    free (res);
    if (!temp)
        return NULL;
    res = temp;
    
    /* site (download page) */
    info = weechat_info_get ("weechat_site_download", "");
    temp = weechat_string_replace (res, "$download", info);
    free (res);
    if (!temp)
        return NULL;
    res = temp;
    
    /* time */
    now = time (NULL);
    snprintf (buf, sizeof (buf), "%s", ctime (&now));
    buf[strlen (buf) - 1] = '\0';
    temp = weechat_string_replace (res, "$time", buf);
    free (res);
    if (!temp)
        return NULL;
    res = temp;
    
    /* username */
    temp = weechat_string_replace (res, "$username",
                                   IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_USERNAME));
    free (res);
    if (!temp)
        return NULL;
    res = temp;
    
    /* realname */
    temp = weechat_string_replace (res, "$realname",
                                   IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_REALNAME));
    free (res);
    if (!temp)
        return NULL;
    res = temp;
    
    /* return result */
    return res;
}

/*
 * irc_ctcp_dcc_filename_without_quotes: return filename for DCC, without
 *                                       double quotes
 */

char *
irc_ctcp_dcc_filename_without_quotes (const char *filename)
{
    int length;

    length = strlen (filename);
    if (length > 0)
    {
        if ((filename[0] == '\"') && (filename[length - 1] == '\"'))
            return weechat_strndup (filename + 1, length - 2);
    }
    return strdup (filename);
}

/*
 * irc_ctcp_recv_dcc: parse CTCP DCC
 */

void
irc_ctcp_recv_dcc (struct t_irc_server *server, const char *nick,
                   const char *arguments, char *message)
{
    char *dcc_args, *pos, *pos_file, *pos_addr, *pos_port, *pos_size;
    char *pos_start_resume, *filename;
    struct t_infolist *infolist;
    struct t_infolist_item *item;
    char charset_modifier[256];
    
    if (!arguments || !arguments[0])
        return;
    
    if (strncmp (arguments, "SEND ", 5) == 0)
    {
        arguments += 5;
        while (arguments[0] == ' ')
        {
            arguments++;
        }
        dcc_args = strdup (arguments);
        
        if (!dcc_args)
        {
            weechat_printf (server->buffer,
                            _("%s%s: not enough memory for \"%s\" "
                              "command"),
                            weechat_prefix ("error"),
                            IRC_PLUGIN_NAME, "privmsg");
            return;
        }
        
        /* DCC filename */
        pos_file = dcc_args;
        while (pos_file[0] == ' ')
        {
            pos_file++;
        }
        
        /* look for file size */
        pos_size = strrchr (pos_file, ' ');
        if (!pos_size)
        {
            weechat_printf (server->buffer,
                            _("%s%s: cannot parse \"%s\" command"),
                            weechat_prefix ("error"),
                            IRC_PLUGIN_NAME, "privmsg");
            free (dcc_args);
            return;
        }
        
        pos = pos_size;
        pos_size++;
        while (pos[0] == ' ')
        {
            pos--;
        }
        pos[1] = '\0';
        
        /* look for DCC port */
        pos_port = strrchr (pos_file, ' ');
        if (!pos_port)
        {
            weechat_printf (server->buffer,
                                _("%s%s: cannot parse \"%s\" command"),
                                weechat_prefix ("error"),
                                IRC_PLUGIN_NAME, "privmsg");
            free (dcc_args);
            return;
        }
        
        pos = pos_port;
        pos_port++;
        while (pos[0] == ' ')
        {
            pos--;
        }
        pos[1] = '\0';
        
        /* look for DCC IP address */
        pos_addr = strrchr (pos_file, ' ');
        if (!pos_addr)
        {
            weechat_printf (server->buffer,
                                _("%s%s: cannot parse \"%s\" command"),
                                weechat_prefix ("error"),
                                IRC_PLUGIN_NAME, "privmsg");
            free (dcc_args);
            return;
        }
        
        pos = pos_addr;
        pos_addr++;
        while (pos[0] == ' ')
        {
            pos--;
        }
        pos[1] = '\0';
        
        /* remove double quotes around filename */
        filename = irc_ctcp_dcc_filename_without_quotes (pos_file);
        
        /* add DCC file via xfer plugin */
        infolist = weechat_infolist_new ();
        if (infolist)
        {
            item = weechat_infolist_new_item (infolist);
            if (item)
            {
                weechat_infolist_new_var_string (item, "plugin_name", weechat_plugin->name);
                weechat_infolist_new_var_string (item, "plugin_id", server->name);
                weechat_infolist_new_var_string (item, "type", "file_recv");
                weechat_infolist_new_var_string (item, "protocol", "dcc");
                weechat_infolist_new_var_string (item, "remote_nick", nick);
                weechat_infolist_new_var_string (item, "local_nick", server->nick);
                weechat_infolist_new_var_string (item, "filename",
                                                 (filename) ? filename : pos_file);
                weechat_infolist_new_var_string (item, "size", pos_size);
                weechat_infolist_new_var_string (item, "proxy",
                                                 IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_PROXY));
                weechat_infolist_new_var_string (item, "address", pos_addr);
                weechat_infolist_new_var_integer (item, "port", atoi (pos_port));
                weechat_hook_signal_send ("xfer_add",
                                          WEECHAT_HOOK_SIGNAL_POINTER,
                                          infolist);
            }
            weechat_infolist_free (infolist);
        }
        
        weechat_hook_signal_send ("irc_dcc",
                                  WEECHAT_HOOK_SIGNAL_STRING,
                                  message);
        
        if (filename)
            free (filename);
        
        free (dcc_args);
    }
    else if (strncmp (arguments, "RESUME ", 7) == 0)
    {
        arguments += 7;
        while (arguments[0] == ' ')
        {
            arguments++;
        }
        dcc_args = strdup (arguments);
        
        if (!dcc_args)
        {
            weechat_printf (server->buffer,
                            _("%s%s: not enough memory for \"%s\" "
                              "command"),
                            weechat_prefix ("error"),
                            IRC_PLUGIN_NAME, "privmsg");
            return;
        }
        
        /* DCC filename */
        pos_file = dcc_args;
        while (pos_file[0] == ' ')
        {
            pos_file++;
        }
        
        /* look for resume start position */
        pos_start_resume = strrchr (pos_file, ' ');
        if (!pos_start_resume)
        {
            weechat_printf (server->buffer,
                            _("%s%s: cannot parse \"%s\" command"),
                            weechat_prefix ("error"),
                            IRC_PLUGIN_NAME, "privmsg");
            free (dcc_args);
            return;
        }
        pos = pos_start_resume;
        pos_start_resume++;
        while (pos[0] == ' ')
        {
            pos--;
        }
        pos[1] = '\0';
        
        /* look for DCC port */
        pos_port = strrchr (pos_file, ' ');
        if (!pos_port)
        {
            weechat_printf (server->buffer,
                            _("%s%s: cannot parse \"%s\" command"),
                            weechat_prefix ("error"),
                            IRC_PLUGIN_NAME, "privmsg");
            free (dcc_args);
            return;
        }
        pos = pos_port;
        pos_port++;
        while (pos[0] == ' ')
        {
            pos--;
        }
        pos[1] = '\0';
        
        /* remove double quotes around filename */
        filename = irc_ctcp_dcc_filename_without_quotes (pos_file);
        
        /* accept resume via xfer plugin */
        infolist = weechat_infolist_new ();
        if (infolist)
        {
            item = weechat_infolist_new_item (infolist);
            if (item)
            {
                weechat_infolist_new_var_string (item, "plugin_name", weechat_plugin->name);
                weechat_infolist_new_var_string (item, "plugin_id", server->name);
                weechat_infolist_new_var_string (item, "type", "file_recv");
                weechat_infolist_new_var_string (item, "filename",
                                                 (filename) ? filename : pos_file);
                weechat_infolist_new_var_integer (item, "port", atoi (pos_port));
                weechat_infolist_new_var_string (item, "start_resume", pos_start_resume);
                weechat_hook_signal_send ("xfer_accept_resume",
                                          WEECHAT_HOOK_SIGNAL_POINTER,
                                          infolist);
            }
            weechat_infolist_free (infolist);
        }
        
        weechat_hook_signal_send ("irc_dcc",
                                  WEECHAT_HOOK_SIGNAL_STRING,
                                  message);
        
        if (filename)
            free (filename);
        
        free (dcc_args);
    }
    else if (strncmp (arguments, "ACCEPT ", 7) == 0)
    {
        arguments += 7;
        while (arguments[0] == ' ')
        {
            arguments++;
        }
        dcc_args = strdup (arguments);
        
        if (!dcc_args)
        {
            weechat_printf (server->buffer,
                            _("%s%s: not enough memory for \"%s\" "
                              "command"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            "privmsg");
            return;
        }
        
        /* DCC filename */
        pos_file = dcc_args;
        while (pos_file[0] == ' ')
        {
            pos_file++;
        }
        
        /* look for resume start position */
        pos_start_resume = strrchr (pos_file, ' ');
        if (!pos_start_resume)
        {
            weechat_printf (server->buffer,
                            _("%s%s: cannot parse \"%s\" command"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            "privmsg");
            free (dcc_args);
            return;
        }
        pos = pos_start_resume;
        pos_start_resume++;
        while (pos[0] == ' ')
        {
            pos--;
        }
        pos[1] = '\0';
        
        /* look for DCC port */
        pos_port = strrchr (pos_file, ' ');
        if (!pos_port)
        {
            weechat_printf (server->buffer,
                            _("%s%s: cannot parse \"%s\" command"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            "privmsg");
            free (dcc_args);
            return;
        }
        pos = pos_port;
        pos_port++;
        while (pos[0] == ' ')
        {
            pos--;
        }
        pos[1] = '\0';
        
        /* remove double quotes around filename */
        filename = irc_ctcp_dcc_filename_without_quotes (pos_file);
        
        /* resume file via xfer plugin */
        infolist = weechat_infolist_new ();
        if (infolist)
        {
            item = weechat_infolist_new_item (infolist);
            if (item)
            {
                weechat_infolist_new_var_string (item, "plugin_name", weechat_plugin->name);
                weechat_infolist_new_var_string (item, "plugin_id", server->name);
                weechat_infolist_new_var_string (item, "type", "file_recv");
                weechat_infolist_new_var_string (item, "filename",
                                                 (filename) ? filename : pos_file);
                weechat_infolist_new_var_integer (item, "port", atoi (pos_port));
                weechat_infolist_new_var_string (item, "start_resume", pos_start_resume);
                weechat_hook_signal_send ("xfer_start_resume",
                                          WEECHAT_HOOK_SIGNAL_POINTER,
                                          infolist);
            }
            weechat_infolist_free (infolist);
        }
        
        weechat_hook_signal_send ("irc_dcc",
                                  WEECHAT_HOOK_SIGNAL_STRING,
                                  message);
        
        if (filename)
            free (filename);
        
        free (dcc_args);
    }
    else if (strncmp (arguments, "CHAT ", 5) == 0)
    {
        arguments += 5;
        while (arguments[0] == ' ')
        {
            arguments++;
        }
        dcc_args = strdup (arguments);
        
        if (!dcc_args)
        {
            weechat_printf (server->buffer,
                            _("%s%s: not enough memory for \"%s\" "
                              "command"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            "privmsg");
            return;
        }
        
        /* CHAT type */
        pos_file = dcc_args;
        while (pos_file[0] == ' ')
        {
            pos_file++;
        }
        
        /* DCC IP address */
        pos_addr = strchr (pos_file, ' ');
        if (!pos_addr)
        {
            weechat_printf (server->buffer,
                            _("%s%s: cannot parse \"%s\" command"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            "privmsg");
            free (dcc_args);
            return;
        }
        pos_addr[0] = '\0';
        pos_addr++;
        while (pos_addr[0] == ' ')
        {
            pos_addr++;
        }
        
        /* look for DCC port */
        pos_port = strchr (pos_addr, ' ');
        if (!pos_port)
        {
            weechat_printf (server->buffer,
                            _("%s%s: cannot parse \"%s\" command"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            "privmsg");
            free (dcc_args);
            return;
        }
        pos_port[0] = '\0';
        pos_port++;
        while (pos_port[0] == ' ')
        {
            pos_port++;
        }
        
        if (weechat_strcasecmp (pos_file, "chat") != 0)
        {
            weechat_printf (server->buffer,
                            _("%s%s: unknown DCC CHAT type "
                              "received from %s%s%s: \"%s\""),
                            weechat_prefix ("error"),
                            IRC_PLUGIN_NAME,
                            IRC_COLOR_CHAT_NICK,
                            nick,
                            IRC_COLOR_CHAT,
                            pos_file);
            free (dcc_args);
            return;
        }
        
        /* add DCC chat via xfer plugin */
        infolist = weechat_infolist_new ();
        if (infolist)
        {
            item = weechat_infolist_new_item (infolist);
            if (item)
            {
                weechat_infolist_new_var_string (item, "plugin_name", weechat_plugin->name);
                weechat_infolist_new_var_string (item, "plugin_id", server->name);
                weechat_infolist_new_var_string (item, "type", "chat_recv");
                weechat_infolist_new_var_string (item, "remote_nick", nick);
                weechat_infolist_new_var_string (item, "local_nick", server->nick);
                snprintf (charset_modifier, sizeof (charset_modifier),
                          "irc.%s.%s", server->name, nick);
                weechat_infolist_new_var_string (item, "charset_modifier", charset_modifier);
                weechat_infolist_new_var_string (item, "proxy",
                                                 IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_PROXY));
                weechat_infolist_new_var_string (item, "address", pos_addr);
                weechat_infolist_new_var_integer (item, "port", atoi (pos_port));
                weechat_hook_signal_send ("xfer_add",
                                          WEECHAT_HOOK_SIGNAL_POINTER,
                                          infolist);
            }
            weechat_infolist_free (infolist);
        }
        
        weechat_hook_signal_send ("irc_dcc",
                                  WEECHAT_HOOK_SIGNAL_STRING,
                                  message);
        
        free (dcc_args);
    }
}

/*
 * irc_ctcp_recv: receive a CTCP and if needed reply to query
 */

void
irc_ctcp_recv (struct t_irc_server *server, const char *command,
               struct t_irc_channel *channel, const char *address,
               const char *nick, const char *remote_nick, char *arguments,
               char *message)
{
    char *pos_end, *pos_space, *pos_args;
    const char *reply;
    char *decoded_reply;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    int nick_is_me;
    
    while (arguments && arguments[0])
    {
        pos_end = strrchr (arguments + 1, '\01');
        if (pos_end)
            pos_end[0] = '\0';
        
        pos_args = NULL;
        pos_space = strchr (arguments + 1, ' ');
        if (pos_space)
        {
            pos_space[0] = '\0';
            pos_args = pos_space + 1;
            while (pos_args[0] == ' ')
            {
                pos_args++;
            }
        }
        
        /* CTCP ACTION */
        if (strcmp (arguments + 1, "ACTION") == 0)
        {
            if (channel)
            {
                ptr_nick = irc_nick_search (channel, nick);
                
                irc_channel_nick_speaking_add (channel,
                                               nick,
                                               (pos_args) ?
                                               weechat_string_has_highlight (pos_args,
                                                                             server->nick) : 0);
                irc_channel_nick_speaking_time_remove_old (channel);
                irc_channel_nick_speaking_time_add (channel, nick,
                                                    time (NULL));
                
                weechat_printf_tags (channel->buffer,
                                     irc_protocol_tags (command,
                                                        "irc_action,notify_message",
                                                        nick),
                                     "%s%s%s%s%s%s",
                                     weechat_prefix ("action"),
                                     (ptr_nick) ? ptr_nick->color : IRC_COLOR_CHAT_NICK,
                                     nick,
                                     (pos_args) ? IRC_COLOR_CHAT : "",
                                     (pos_args) ? " " : "",
                                     (pos_args) ? pos_args : "");
            }
            else
            {
                nick_is_me = (strcmp (server->nick, nick) == 0);
                ptr_channel = irc_channel_search (server, remote_nick);
                if (!ptr_channel)
                {
                    ptr_channel = irc_channel_new (server,
                                                   IRC_CHANNEL_TYPE_PRIVATE,
                                                   remote_nick, 0, 0);
                    if (!ptr_channel)
                    {
                        weechat_printf (server->buffer,
                                        _("%s%s: cannot create new "
                                          "private buffer \"%s\""),
                                        weechat_prefix ("error"),
                                        IRC_PLUGIN_NAME, remote_nick);
                    }
                }
                if (ptr_channel)
                {
                    if (!ptr_channel->topic)
                        irc_channel_set_topic (ptr_channel, address);
                    
                    weechat_printf_tags (ptr_channel->buffer,
                                         irc_protocol_tags (command,
                                                            (nick_is_me) ?
                                                            "irc_action,notify_private,no_highlight" :
                                                            "irc_action,notify_private",
                                                            nick),
                                         "%s%s%s%s%s%s",
                                         weechat_prefix ("action"),
                                         (nick_is_me) ?
                                         IRC_COLOR_CHAT_NICK_SELF : irc_nick_color_for_pv (ptr_channel, nick),
                                         nick,
                                         (pos_args) ? IRC_COLOR_CHAT : "",
                                         (pos_args) ? " " : "",
                                         (pos_args) ? pos_args : "");
                    weechat_hook_signal_send ("irc_pv",
                                              WEECHAT_HOOK_SIGNAL_STRING,
                                              message);
                }
            }
        }
        /* CTCP PING */
        else if (strcmp (arguments + 1, "PING") == 0)
        {
            reply = irc_ctcp_get_reply (server, arguments + 1);
            irc_ctcp_display_request (server, command, channel, nick,
                                      arguments + 1, pos_args, reply);
            if (!reply || reply[0])
            {
                irc_ctcp_reply_to_nick (server, command, channel, nick,
                                        arguments + 1, pos_args);
            }
        }
        /* CTCP DCC */
        else if (strcmp (arguments + 1, "DCC") == 0)
        {
            irc_ctcp_recv_dcc (server, nick, pos_args, message);
        }
        /* other CTCP */
        else
        {
            reply = irc_ctcp_get_reply (server, arguments + 1);
            if (reply)
            {
                irc_ctcp_display_request (server, command, channel, nick,
                                          arguments + 1, pos_args, reply);
                
                if (reply[0])
                {
                    decoded_reply = irc_ctcp_replace_variables (server, reply);
                    if (decoded_reply)
                    {
                        irc_ctcp_reply_to_nick (server, command, channel, nick,
                                                arguments + 1, decoded_reply);
                        free (decoded_reply);
                    }
                }
            }
            else
            {
                if (weechat_config_boolean (irc_config_look_display_ctcp_unknown))
                {
                    weechat_printf_tags ((channel) ? channel->buffer : server->buffer,
                                         irc_protocol_tags (command,
                                                            "irc_ctcp",
                                                            NULL),
                                         _("%sUnknown CTCP requested by %s%s%s: "
                                           "%s%s%s%s%s"),
                                         weechat_prefix ("network"),
                                         IRC_COLOR_CHAT_NICK,
                                         nick,
                                         IRC_COLOR_CHAT,
                                         IRC_COLOR_CHAT_CHANNEL,
                                         arguments + 1,
                                         (pos_args) ? IRC_COLOR_CHAT : "",
                                         (pos_args) ? " " : "",
                                         (pos_args) ? pos_args : "");
                }
            }
        }
        
        weechat_hook_signal_send ("irc_ctcp",
                                  WEECHAT_HOOK_SIGNAL_STRING,
                                  message);
        
        if (pos_space)
            pos_space[0] = ' ';
        
        if (pos_end)
            pos_end[0] = '\01';
        
        arguments = (pos_end) ? pos_end + 1 : NULL;
    }
}
