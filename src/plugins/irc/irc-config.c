/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

/* irc-config.c: IRC configuration options */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <pwd.h>

#include "irc.h"
#include "irc-config.h"
#include "irc-dcc.h"
#include "irc-server.h"


struct t_config_file *irc_config_file = NULL;
struct t_config_section *irc_config_section_server = NULL;

/* config, irc section */

struct t_config_option *irc_config_irc_one_server_buffer;
struct t_config_option *irc_config_irc_open_near_server;
struct t_config_option *irc_config_irc_nick_prefix;
struct t_config_option *irc_config_irc_nick_suffix;
struct t_config_option *irc_config_irc_display_away;
struct t_config_option *irc_config_irc_show_away_once;
struct t_config_option *irc_config_irc_default_msg_part;
struct t_config_option *irc_config_irc_notice_as_pv;
struct t_config_option *irc_config_irc_away_check;
struct t_config_option *irc_config_irc_away_check_max_nicks;
struct t_config_option *irc_config_irc_lag_check;
struct t_config_option *irc_config_irc_lag_min_show;
struct t_config_option *irc_config_irc_lag_disconnect;
struct t_config_option *irc_config_irc_anti_flood;
struct t_config_option *irc_config_irc_highlight;
struct t_config_option *irc_config_irc_colors_receive;
struct t_config_option *irc_config_irc_colors_send;
struct t_config_option *irc_config_irc_send_unknown_commands;

/* config, dcc section */

struct t_config_option *irc_config_dcc_auto_accept_files;
struct t_config_option *irc_config_dcc_auto_accept_chats;
struct t_config_option *irc_config_dcc_timeout;
struct t_config_option *irc_config_dcc_blocksize;
struct t_config_option *irc_config_dcc_fast_send;
struct t_config_option *irc_config_dcc_port_range;
struct t_config_option *irc_config_dcc_own_ip;
struct t_config_option *irc_config_dcc_download_path;
struct t_config_option *irc_config_dcc_upload_path;
struct t_config_option *irc_config_dcc_convert_spaces;
struct t_config_option *irc_config_dcc_auto_rename;
struct t_config_option *irc_config_dcc_auto_resume;

/* config, log section */

struct t_config_option *irc_config_log_auto_server;
struct t_config_option *irc_config_log_auto_channel;
struct t_config_option *irc_config_log_auto_private;
struct t_config_option *irc_config_log_hide_nickserv_pwd;

/* config, server section */

struct t_config_option *irc_config_server_name;
struct t_config_option *irc_config_server_autoconnect;
struct t_config_option *irc_config_server_autoreconnect;
struct t_config_option *irc_config_server_autoreconnect_delay;
struct t_config_option *irc_config_server_address;
struct t_config_option *irc_config_server_port;
struct t_config_option *irc_config_server_ipv6;
struct t_config_option *irc_config_server_ssl;
struct t_config_option *irc_config_server_password;
struct t_config_option *irc_config_server_nick1;
struct t_config_option *irc_config_server_nick2;
struct t_config_option *irc_config_server_nick3;
struct t_config_option *irc_config_server_username;
struct t_config_option *irc_config_server_realname;
struct t_config_option *irc_config_server_hostname;
struct t_config_option *irc_config_server_command;
struct t_config_option *irc_config_server_command_delay;
struct t_config_option *irc_config_server_autojoin;
struct t_config_option *irc_config_server_autorejoin;
struct t_config_option *irc_config_server_notify_levels;

struct t_irc_server *irc_config_server = NULL;
int irc_config_reload_flag;


/*
 * irc_config_change_one_server_buffer: called when the "one server buffer"
 *                                      setting is changed
 */

void
irc_config_change_one_server_buffer ()
{
    /*if (irc_config_irc_one_server_buffer)
        irc_buffer_merge_servers (gui_current_window);
    else
        irc_buffer_split_server (gui_current_window);
    */
}

/*
 * irc_config_change_away_check: called when away check is changed
 */

void
irc_config_change_away_check ()
{
    if (irc_timer_check_away)
    {
        weechat_unhook (irc_timer_check_away);
        irc_timer_check_away = NULL;
    }
    if (weechat_config_integer (irc_config_irc_away_check) == 0)
    {
        /* reset away flag for all nicks/chans/servers */
        //irc_server_remove_away ();
    }
    /*irc_timer_check_away = weechat_hook_timer (weechat_config_integer (irc_config_irc_away_check) * 60 * 1000,
                                               0,
                                               irc_server_timer_check_away,
                                               NULL);
    */
}

/*
 * irc_config_change_log: called when log settings are changed
 *                        (for server/channel/private logging)
 */

void
irc_config_change_log ()
{
    /*t_gui_buffer *ptr_buffer;
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->protocol == irc_protocol)
        {
            ptr_server = irc_server_search (ptr_buffer->category);
            ptr_channel = irc_channel_search (ptr_server, ptr_buffer->name);
            
            if (ptr_server && !ptr_channel)
            {
                if (irc_config_log_auto_server && !ptr_buffer->log_file)
                    gui_log_start (ptr_buffer);
                else if (!irc_config_log_auto_server && ptr_buffer->log_file)
                    gui_log_end (ptr_buffer);
            }
            if (ptr_server && ptr_channel)
            {
                if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                {
                    if (irc_config_log_auto_channel && !ptr_buffer->log_file)
                        gui_log_start (ptr_buffer);
                    else if (!irc_config_log_auto_channel && ptr_buffer->log_file)
                        gui_log_end (ptr_buffer);
                }
                else
                {
                    if (irc_config_log_auto_private && !ptr_buffer->log_file)
                        gui_log_start (ptr_buffer);
                    else if (!irc_config_log_auto_private && ptr_buffer->log_file)
                        gui_log_end (ptr_buffer);
                }
            }
        }
    }
    */
}

/*
 * irc_config_change_notify_levels: called when notify level is changed
 */

void
irc_config_change_notify_levels ()
{
}

/*
 * irc_config_read_server_line: read a server line in configuration file
 */

void
irc_config_read_server_line (void *config_file, char *option_name, char *value)
{
    struct t_config_option *ptr_option;
    int rc;
    
    /* make C compiler happy */
    (void) config_file;

    if (option_name && value)
    {
        if (irc_config_server)
        {
            ptr_option = weechat_config_search_option (irc_config_file,
                                                       irc_config_section_server,
                                                       option_name);
            if (ptr_option)
            {
                rc = weechat_config_option_set (ptr_option, value);
                switch (rc)
                {
                    case 2:
                        break;
                    case 1:
                        break;
                    case 0:
                        weechat_printf (NULL,
                                        _("%sirc: warning, failed to set option "
                                          "\"%s\" with value \"%s\""),
                                        weechat_prefix ("error"),
                                        option_name, value);
                        break;
                }
            }
            else
            {
                weechat_printf (NULL,
                                _("%sirc: warning, option not found in config "
                                  "file: \"%s\""),
                                weechat_prefix ("error"),
                                option_name);
            }
        }
    }
    else
    {
        /* beginning of [server] section: save current server and create new
           with default values for filling with next lines in file */
        if (irc_config_server)
        {
            irc_server_init_with_config_options (irc_config_server,
                                                 irc_config_section_server,
                                                 irc_config_reload_flag);
        }
        irc_config_server = irc_server_alloc ();
        if (!irc_config_server)
        {
            weechat_printf (NULL,
                            _("%sirc: error creating server for reading "
                              "config file"),
                            weechat_prefix ("error"));
        }
    }
}

/*
 * irc_config_write_servers: write servers in configuration file
 */

void
irc_config_write_servers (void *config_file, char *section_name)
{
    struct t_irc_server *ptr_server;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (!ptr_server->temp_server)
        {
            weechat_config_write_line (config_file, section_name, NULL);
            weechat_config_write_line (config_file, "server_name", "\"%s\"",
                                       ptr_server->name);
            weechat_config_write_line (config_file, "server_autoconnect", "%s",
                                       (ptr_server->autoconnect) ? "on" : "off");
            weechat_config_write_line (config_file, "server_autoreconnect", "%s",
                                       (ptr_server->autoreconnect) ? "on" : "off");
            weechat_config_write_line (config_file, "server_autoreconnect_delay", "%d",
                                       ptr_server->autoreconnect_delay);
            weechat_config_write_line (config_file, "server_address", "\"%s\"", ptr_server->address);
            weechat_config_write_line (config_file, "server_port", "%d", ptr_server->port);
            weechat_config_write_line (config_file, "server_ipv6", "%s",
                                       (ptr_server->ipv6) ? "on" : "off");
            weechat_config_write_line (config_file, "server_ssl", "%s",
                                       (ptr_server->ssl) ? "on" : "off");
            weechat_config_write_line (config_file, "server_password", "\"%s\"",
                                       (ptr_server->password) ? ptr_server->password : "");
            weechat_config_write_line (config_file, "server_nick1", "\"%s\"",
                                       ptr_server->nick1);
            weechat_config_write_line (config_file, "server_nick2", "\"%s\"",
                                       ptr_server->nick2);
            weechat_config_write_line (config_file, "server_nick3", "\"%s\"",
                                       ptr_server->nick3);
            weechat_config_write_line (config_file, "server_username", "\"%s\"",
                                       ptr_server->username);
            weechat_config_write_line (config_file, "server_realname", "\"%s\"",
                                       ptr_server->realname);
            weechat_config_write_line (config_file, "server_hostname", "\"%s\"",
                                       (ptr_server->hostname) ? ptr_server->hostname : "");
            weechat_config_write_line (config_file, "server_command", "\"%s\"",
                                       (ptr_server->command) ? ptr_server->command : "");
            weechat_config_write_line (config_file, "server_command_delay", "%d",
                                       ptr_server->command_delay);
            weechat_config_write_line (config_file, "server_autojoin", "\"%s\"",
                                       (ptr_server->autojoin) ? ptr_server->autojoin : "");
            weechat_config_write_line (config_file, "server_autorejoin", "%s",
                                       (ptr_server->autorejoin) ? "on" : "off");
            weechat_config_write_line (config_file, "server_notify_levels", "\"%s\"",
                                       (ptr_server->notify_levels) ? ptr_server->notify_levels : "");
        }
    }
}

/*
 * irc_config_write_server_default: write default server in configuration file
 */

void
irc_config_write_server_default (void *config_file, char *section_name)
{
    struct passwd *my_passwd;
    char *realname, *pos;
    
    weechat_config_write_line (config_file, section_name, NULL);
    
    weechat_config_write_line (config_file, "server_name", "%s", "\"freenode\"");
    weechat_config_write_line (config_file, "server_autoconnect", "%s", "off");
    weechat_config_write_line (config_file, "server_autoreconnect", "%s", "on");
    weechat_config_write_line (config_file, "server_autoreconnect_delay", "%s", "30");
    weechat_config_write_line (config_file, "server_address", "%s", "\"irc.freenode.net\"");
    weechat_config_write_line (config_file, "server_port", "%s", "6667");
    weechat_config_write_line (config_file, "server_ipv6", "%s", "off");
    weechat_config_write_line (config_file, "server_ssl", "%s", "off");
    weechat_config_write_line (config_file, "server_password", "%s", "\"\"");
    
    /* Get the user's name from /etc/passwd */
    if ((my_passwd = getpwuid (geteuid ())) != NULL)
    {
        weechat_config_write_line (config_file, "server_nick1", "\"%s\"", my_passwd->pw_name);
        weechat_config_write_line (config_file, "server_nick2", "\"%s1\"", my_passwd->pw_name);
        weechat_config_write_line (config_file, "server_nick3", "\"%s2\"", my_passwd->pw_name);
        weechat_config_write_line (config_file, "server_username", "\"%s\"", my_passwd->pw_name);
        if ((!my_passwd->pw_gecos)
            || (my_passwd->pw_gecos[0] == '\0')
            || (my_passwd->pw_gecos[0] == ',')
            || (my_passwd->pw_gecos[0] == ' '))
            weechat_config_write_line (config_file, "server_realname", "\"%s\"", my_passwd->pw_name);
        else
        {
            realname = strdup (my_passwd->pw_gecos);
            pos = strchr (realname, ',');
            if (pos)
                pos[0] = '\0';
            weechat_config_write_line (config_file, "server_realname", "\"%s\"",
                                       realname);
            if (pos)
                pos[0] = ',';
            free (realname);
        }
    }
    else
    {
        /* default values if /etc/passwd can't be read */
        weechat_config_write_line (config_file, "server_nick1", "%s", "\"weechat1\"");
        weechat_config_write_line (config_file, "server_nick2", "%s", "\"weechat2\"");
        weechat_config_write_line (config_file, "server_nick3", "%s", "\"weechat3\"");
        weechat_config_write_line (config_file, "server_username", "%s", "\"weechat\"");
        weechat_config_write_line (config_file, "server_realname", "%s", "\"weechat\"");
    }
    
    weechat_config_write_line (config_file, "server_hostname", "%s", "\"\"");
    weechat_config_write_line (config_file, "server_command", "%s", "\"\"");
    weechat_config_write_line (config_file, "server_command_delay", "%s", "0");
    weechat_config_write_line (config_file, "server_autojoin", "%s", "\"\"");
    weechat_config_write_line (config_file, "server_autorejoin", "%s", "on");
    weechat_config_write_line (config_file, "server_notify_levels", "%s", "\"\"");
}

/*
 * irc_config_init: init IRC configuration file
 *                  return: 1 if ok, 0 if error
 */

int
irc_config_init ()
{
    struct t_config_section *ptr_section;

    irc_config_file = weechat_config_new (IRC_CONFIG_FILENAME);
    if (!irc_config_file)
        return 0;
    
    ptr_section = weechat_config_new_section (irc_config_file, "irc",
                                              NULL, NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (irc_config_file);
        return 0;
    }

    irc_config_irc_one_server_buffer = weechat_config_new_option (
        ptr_section, "irc_one_server_buffer", "boolean",
        N_("use same buffer for all servers"),
        NULL, 0, 0, "off", &irc_config_change_one_server_buffer);
    irc_config_irc_open_near_server = weechat_config_new_option (
        ptr_section, "irc_open_near_server", "boolean",
        N_("open new channels/privates near server"),
        NULL, 0, 0, "off", NULL);
    irc_config_irc_nick_prefix = weechat_config_new_option (
        ptr_section, "irc_nick_prefix", "string",
        N_("text to display before nick in chat window"),
        NULL, 0, 0, "", NULL);
    irc_config_irc_nick_suffix = weechat_config_new_option (
        ptr_section, "irc_nick_suffix", "string",
        N_("text to display after nick in chat window"),
        NULL, 0, 0, "", NULL);
    irc_config_irc_display_away = weechat_config_new_option (
        ptr_section, "irc_display_away", "integer",
        N_("display message when (un)marking as away"),
        "off|local|channel", 0, 0, "local", NULL);
    irc_config_irc_show_away_once = weechat_config_new_option (
        ptr_section, "irc_show_away_once", "boolean",
        N_("show remote away message only once in private"),
        NULL, 0, 0, "on", NULL);
    irc_config_irc_default_msg_part = weechat_config_new_option (
        ptr_section, "irc_default_msg_part", "string",
        N_("default part message (leaving channel) ('%v' will be replaced by "
           "WeeChat version in string)"),
        NULL, 0, 0, "WeeChat %v", NULL);
    irc_config_irc_notice_as_pv = weechat_config_new_option (
        ptr_section, "irc_notice_as_pv", "boolean",
        N_("display notices as private messages"),
        NULL, 0, 0, "off", NULL);
    irc_config_irc_away_check = weechat_config_new_option (
        ptr_section, "irc_away_check", "integer",
        N_("interval between two checks for away (in minutes, 0 = never "
           "check)"),
        NULL, 0, INT_MAX, "0", &irc_config_change_away_check);
    irc_config_irc_away_check_max_nicks = weechat_config_new_option (
        ptr_section, "irc_away_check_max_nicks", "integer",
        N_("do not check away nicks on channels with high number of nicks "
           "(0 = unlimited)"),
        NULL, 0, INT_MAX, "0", &irc_config_change_away_check);
    irc_config_irc_lag_check = weechat_config_new_option (
        ptr_section, "irc_lag_check", "integer",
        N_("interval between two checks for lag (in seconds, 0 = never "
           "check)"),
        NULL, 0, INT_MAX, "60", NULL);
    irc_config_irc_lag_min_show = weechat_config_new_option (
        ptr_section, "irc_lag_min_show", "integer",
        N_("minimum lag to show (in seconds)"),
        NULL, 0, INT_MAX, "1", NULL);
    irc_config_irc_lag_disconnect = weechat_config_new_option (
        ptr_section, "irc_lag_disconnect", "integer",
        N_("disconnect after important lag (in minutes, 0 = never "
           "disconnect)"),
        NULL, 0, INT_MAX, "5", NULL);
    irc_config_irc_anti_flood = weechat_config_new_option (
        ptr_section, "irc_anti_flood", "integer",
        N_("anti-flood: # seconds between two user messages (0 = no "
           "anti-flood)"),
        NULL, 0, 5, "2", NULL);
    irc_config_irc_highlight = weechat_config_new_option (
        ptr_section, "irc_highlight", "string",
        N_("comma separated list of words to highlight (case insensitive "
           "comparison, words may begin or end with \"*\" for partial match)"),
        NULL, 0, 0, "", NULL);
    irc_config_irc_colors_receive = weechat_config_new_option (
        ptr_section, "irc_colors_receive", "boolean",
        N_("when off, colors codes are ignored in incoming messages"),
        NULL, 0, 0, "on", NULL);
    irc_config_irc_colors_send = weechat_config_new_option (
        ptr_section, "irc_colors_send", "boolean",
        N_("allow user to send colors with special codes (^Cb=bold, "
           "^Ccxx=color, ^Ccxx,yy=color+background, ^Cu=underline, "
           "^Cr=reverse)"),
        NULL, 0, 0, "on", NULL);
    irc_config_irc_send_unknown_commands = weechat_config_new_option (
        ptr_section, "irc_send_unknown_commands", "boolean",
        N_("send unknown commands to IRC server"),
        NULL, 0, 0, "off", NULL);
    
    ptr_section = weechat_config_new_section (irc_config_file, "dcc",
                                              NULL, NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (irc_config_file);
        return 0;
    }
    
    irc_config_dcc_auto_accept_files = weechat_config_new_option (
        ptr_section, "dcc_auto_accept_files", "boolean",
        N_("automatically accept incoming dcc files (use carefully!)"),
        NULL, 0, 0, "off", NULL);
    irc_config_dcc_auto_accept_chats = weechat_config_new_option (
        ptr_section, "dcc_auto_accept_chats", "boolean",
        N_("automatically accept dcc chats (use carefully!)"),
        NULL, 0, 0, "off", NULL);
    irc_config_dcc_timeout = weechat_config_new_option (
        ptr_section, "dcc_timeout", "integer",
        N_("timeout for dcc request (in seconds)"),
        NULL, 5, INT_MAX, "300", NULL);
    irc_config_dcc_blocksize = weechat_config_new_option (
        ptr_section, "dcc_blocksize", "integer",
        N_("block size for dcc packets in bytes"),
        NULL, IRC_DCC_MIN_BLOCKSIZE, IRC_DCC_MAX_BLOCKSIZE, "65536",
        NULL);
    irc_config_dcc_fast_send = weechat_config_new_option (
        ptr_section, "dcc_fast_send", "boolean",
        N_("does not wait for ACK when sending file"),
        NULL, 0, 0, "on", NULL);
    irc_config_dcc_port_range = weechat_config_new_option (
        ptr_section, "dcc_port_range", "string",
        N_("restricts outgoing dcc to use only ports in the given range "
           "(useful for NAT) (syntax: a single port, ie. 5000 or a port "
           "range, ie. 5000-5015, empty value means any port)"),
        NULL, 0, 0, "", NULL);
    irc_config_dcc_own_ip = weechat_config_new_option (
        ptr_section, "dcc_own_ip", "string",
        N_("IP or DNS address used for outgoing dcc "
           "(if empty, local interface IP is used)"),
        NULL, 0, 0, "", NULL);
    irc_config_dcc_download_path = weechat_config_new_option (
        ptr_section, "dcc_download_path", "string",
        N_("path for writing incoming files with dcc"),
        NULL, 0, 0, "%h/dcc", NULL);
    irc_config_dcc_upload_path = weechat_config_new_option (
        ptr_section, "dcc_upload_path", "string",
        N_("path for reading files when sending thru dcc (when no path is "
           "specified)"),
        NULL, 0, 0, "~", NULL);
    irc_config_dcc_convert_spaces = weechat_config_new_option (
        ptr_section, "dcc_convert_spaces", "boolean",
        N_("convert spaces to underscores when sending files"),
        NULL, 0, 0, "on", NULL);
    irc_config_dcc_auto_rename = weechat_config_new_option (
        ptr_section, "dcc_auto_rename", "boolean",
        N_("rename incoming files if already exists (add '.1', '.2', ...)"),
        NULL, 0, 0, "on", NULL);
    irc_config_dcc_auto_resume = weechat_config_new_option (
        ptr_section, "dcc_auto_resume", "boolean",
        N_("automatically resume dcc transfer if connection with remote host "
           "is loosed"),
        NULL, 0, 0, "on", NULL);
    
    ptr_section = weechat_config_new_section (irc_config_file, "log",
                                              NULL, NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (irc_config_file);
        return 0;
    }
    
    irc_config_log_auto_server = weechat_config_new_option (
        ptr_section, "log_auto_server", "boolean",
        N_("automatically log server messages"),
        NULL, 0, 0, "off", &irc_config_change_log);
    irc_config_log_auto_channel = weechat_config_new_option (
        ptr_section, "log_auto_channel", "boolean",
        N_("automatically log channel chats"),
        NULL, 0, 0, "off", &irc_config_change_log);
    irc_config_log_auto_private = weechat_config_new_option (
        ptr_section, "log_auto_private", "boolean",
        N_("automatically log private chats"),
        NULL, 0, 0, "off", &irc_config_change_log);
    irc_config_log_hide_nickserv_pwd = weechat_config_new_option (
        ptr_section, "log_hide_nickserv_pwd", "boolean",
        N_("hide password displayed by nickserv"),
        NULL, 0, 0, "on", &irc_config_change_log);
    
    ptr_section = weechat_config_new_section (irc_config_file, "server",
                                              irc_config_read_server_line,
                                              irc_config_write_servers,
                                              irc_config_write_server_default);
    if (!ptr_section)
    {
        weechat_config_free (irc_config_file);
        return 0;
    }
    
    irc_config_section_server = ptr_section;
    
    irc_config_server_name = weechat_config_new_option (
        ptr_section, "server_name", "string",
        N_("name associated to IRC server (for display only)"),
        NULL, 0, 0, "", NULL);
    irc_config_server_autoconnect = weechat_config_new_option (
        ptr_section, "server_autoconnect", "boolean",
        N_("automatically connect to server when WeeChat is starting"),
        NULL, 0, 0, "off", NULL);
    irc_config_server_autoreconnect = weechat_config_new_option (
        ptr_section, "server_autoreconnect", "boolean",
        N_("automatically reconnect to server when disconnected"),
        NULL, 0, 0, "on", NULL);
    irc_config_server_autoreconnect_delay = weechat_config_new_option (
        ptr_section, "server_autoreconnect_delay", "integer",
        N_("delay (in seconds) before trying again to reconnect to server"),
        NULL, 0, 65535, "30", NULL);
    irc_config_server_address = weechat_config_new_option (
        ptr_section, "server_address", "string",
        N_("IP address or hostname of IRC server"),
        NULL, 0, 0, "", NULL);
    irc_config_server_port = weechat_config_new_option (
        ptr_section, "server_port", "integer",
        N_("port for connecting to server"),
        NULL, 0, 65535, "6667", NULL);
    irc_config_server_ipv6 = weechat_config_new_option (
        ptr_section, "server_ipv6", "boolean",
        N_("use IPv6 protocol for server communication"),
        NULL, 0, 0, "on", NULL);
    irc_config_server_ssl = weechat_config_new_option (
        ptr_section, "server_ssl", "boolean",
        N_("use SSL for server communication"),
        NULL, 0, 0, "on", NULL);
    irc_config_server_password = weechat_config_new_option (
        ptr_section, "server_password", "string",
        N_("password for IRC server"),
        NULL, 0, 0, "", NULL);
    irc_config_server_nick1 = weechat_config_new_option (
        ptr_section, "server_nick1", "string",
        N_("nickname to use on IRC server"),
        NULL, 0, 0, "", NULL);
    irc_config_server_nick2 = weechat_config_new_option (
        ptr_section, "server_nick2", "string",
        N_("alternate nickname to use on IRC server (if nickname is already "
           "used)"),
        NULL, 0, 0, "", NULL);
    irc_config_server_nick3 = weechat_config_new_option (
        ptr_section, "server_nick3", "string",
        N_("2nd alternate nickname to use on IRC server (if alternate "
           "nickname is already used)"),
        NULL, 0, 0, "", NULL);
    irc_config_server_username = weechat_config_new_option (
        ptr_section, "server_username", "string",
        N_("user name to use on IRC server"),
        NULL, 0, 0, "", NULL);
    irc_config_server_realname = weechat_config_new_option (
        ptr_section, "server_realname", "string",
        N_("real name to use on IRC server"),
        NULL, 0, 0, "", NULL);
    irc_config_server_hostname = weechat_config_new_option (
        ptr_section, "server_hostname", "string",
        N_("custom hostname/IP for server (optional, if empty local hostname "
           "is used)"),
        NULL, 0, 0, "", NULL);
    irc_config_server_command = weechat_config_new_option (
        ptr_section, "server_command", "string",
        N_("command(s) to run when connected to server (many commands should "
           "be separated by ';', use '\\;' for a semicolon, special variables "
           "$nick, $channel and $server are replaced by their value)"),
        NULL, 0, 0, "", NULL);
    irc_config_server_command_delay = weechat_config_new_option (
        ptr_section, "server_command_delay", "integer",
        N_("delay (in seconds) after command was executed (example: give some "
           "time for authentication)"),
        NULL, 0, 3600, "0", NULL);
    irc_config_server_autojoin = weechat_config_new_option (
        ptr_section, "server_autojoin", "string",
        N_("comma separated list of channels to join when connected to server "
           "(example: \"#chan1,#chan2,#chan3 key1,key2\")"),
        NULL, 0, 0, "", NULL);
    irc_config_server_autorejoin = weechat_config_new_option (
        ptr_section, "server_autorejoin", "string",
        N_("automatically rejoin channels when kicked"),
        NULL, 0, 0, "on", NULL);
    irc_config_server_notify_levels = weechat_config_new_option (
        ptr_section, "server_notify_levels", "string",
        N_("comma separated list of notify levels for channels of this server "
           "(format: #channel:1,..), a channel name '*' is reserved for "
           "server default notify level"),
        NULL, 0, 0, "", NULL);
    
    return 1;
}

/*
 * irc_config_read: read IRC configuration file
 *                  return:  0 = successful
 *                          -1 = configuration file file not found
 *                          -2 = error in configuration file
 */

int
irc_config_read ()
{
    int rc;
    
    irc_config_server = NULL;
    irc_config_reload_flag = 0;
    
    rc = weechat_config_read (irc_config_file);

    if (irc_config_server)
        irc_server_init_with_config_options (irc_config_server,
                                             irc_config_section_server,
                                             irc_config_reload_flag);
    
    return rc;
}

/*
 * irc_config_reload_cb: read IRC configuration file
 */

int
irc_config_reload_cb (void *data, char *event, void *pointer)
{
    struct t_irc_server *ptr_server, *next_server;
    int rc;
    
    /* make C compiler happy */
    (void) data;
    (void) event;
    (void) pointer;
    
    irc_config_server = NULL;
    irc_config_reload_flag = 1;
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        ptr_server->reloaded_from_config = 0;
    }
    
    rc = weechat_config_reload (irc_config_file);

    if (rc == 0)
    {
        
        if (irc_config_server)
            irc_server_init_with_config_options (irc_config_server,
                                                 irc_config_section_server,
                                                 irc_config_reload_flag);
        
        ptr_server = irc_servers;
        while (ptr_server)
        {
            next_server = ptr_server->next_server;
            
            if (!ptr_server->reloaded_from_config)
            {
                if (ptr_server->is_connected)
                {
                    weechat_printf (NULL,
                                    _("%sirc: warning: server \"%s\" not found in "
                                      "configuration file, but was not deleted "
                                      "(currently used)"),
                                    weechat_prefix ("info"),
                                    ptr_server->name);
                }
                else
                    irc_server_free (ptr_server);
            }
            
            ptr_server = next_server;
        }

        weechat_printf (NULL,
                        _("%sirc: configuration file reloaded"),
                        weechat_prefix ("info"));
        return PLUGIN_RC_SUCCESS;
    }
    
    weechat_printf (NULL,
                    _("%sirc: failed to reload alias configuration "
                      "file"),
                    weechat_prefix ("error"));
    return PLUGIN_RC_FAILED;
}

/*
 * irc_config_write: write IRC configuration file
 *                   return:  0 if ok
 *                          < 0 if error
 */

int
irc_config_write ()
{
    return weechat_config_write (irc_config_file);
}
