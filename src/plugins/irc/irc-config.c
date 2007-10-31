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

#include "../../core/weechat.h"
#include "irc.h"
#include "../../core/config-option.h"
#include "../../core/config-file.h"
#include "../../core/hook.h"
#include "../../core/util.h"


/* config, irc section */

int irc_cfg_irc_one_server_buffer;
int irc_cfg_irc_open_near_server;
char *irc_cfg_irc_nick_prefix;
char *irc_cfg_irc_nick_suffix;
int irc_cfg_irc_display_away;
int irc_cfg_irc_show_away_once;
char *irc_cfg_irc_display_away_values[] =
{ "off", "local", "channel", NULL };
char *irc_cfg_irc_default_msg_part;
char *irc_cfg_irc_default_msg_quit;
int irc_cfg_irc_notice_as_pv;
int irc_cfg_irc_away_check;
int irc_cfg_irc_away_check_max_nicks;
int irc_cfg_irc_lag_check;
int irc_cfg_irc_lag_min_show;
int irc_cfg_irc_lag_disconnect;
int irc_cfg_irc_anti_flood;
char *irc_cfg_irc_highlight;
int irc_cfg_irc_colors_receive;
int irc_cfg_irc_colors_send;
int irc_cfg_irc_send_unknown_commands;

t_config_option irc_options_irc[] =
{ { "irc_one_server_buffer",
    N_("use same buffer for all servers"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_FALSE, NULL, NULL,
    &irc_cfg_irc_one_server_buffer, NULL, irc_config_change_one_server_buffer },
  { "irc_open_near_server",
    N_("open new channels/privates near server"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_FALSE, NULL, NULL,
    &irc_cfg_irc_open_near_server, NULL, irc_config_change_noop },
  { "irc_nick_prefix",
    N_("text to display before nick in chat window"),
    OPTION_TYPE_STRING, 0, 0, 0, "", NULL,
    NULL, &irc_cfg_irc_nick_prefix, irc_config_change_noop },
  { "irc_nick_suffix",
    N_("text to display after nick in chat window"),
    OPTION_TYPE_STRING, 0, 0, 0, " |", NULL,
    NULL, &irc_cfg_irc_nick_suffix, irc_config_change_noop },
  { "irc_display_away",
    N_("display message when (un)marking as away"),
    OPTION_TYPE_INT_WITH_STRING, 0, 0, 0, "off", irc_cfg_irc_display_away_values,
    &irc_cfg_irc_display_away, NULL, irc_config_change_noop },
  { "irc_show_away_once",
    N_("show remote away message only once in private"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &irc_cfg_irc_show_away_once, NULL, irc_config_change_noop },
  { "irc_default_msg_part",
    N_("default part message (leaving channel) ('%v' will be replaced by "
       "WeeChat version in string)"),
    OPTION_TYPE_STRING, 0, 0, 0, "WeeChat %v", NULL,
    NULL, &irc_cfg_irc_default_msg_part, irc_config_change_noop },
  { "irc_default_msg_quit",
    N_("default quit message ('%v' will be replaced by WeeChat version in "
       "string)"),
    OPTION_TYPE_STRING, 0, 0, 0, "WeeChat %v", NULL,
    NULL, &irc_cfg_irc_default_msg_quit, irc_config_change_noop },
  { "irc_notice_as_pv",
    N_("display notices as private messages"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_FALSE, NULL, NULL,
    &irc_cfg_irc_notice_as_pv, NULL, irc_config_change_noop },
  { "irc_away_check",
    N_("interval between two checks for away (in minutes, 0 = never check)"),
    OPTION_TYPE_INT, 0, INT_MAX, 0, NULL, NULL,
    &irc_cfg_irc_away_check, NULL, irc_config_change_away_check },
  { "irc_away_check_max_nicks",
    N_("do not check away nicks on channels with high number of nicks (0 = unlimited)"),
    OPTION_TYPE_INT, 0, INT_MAX, 0, NULL, NULL,
    &irc_cfg_irc_away_check_max_nicks, NULL, irc_config_change_away_check },
  { "irc_lag_check",
    N_("interval between two checks for lag (in seconds)"),
    OPTION_TYPE_INT, 30, INT_MAX, 60, NULL, NULL,
    &irc_cfg_irc_lag_check, NULL, irc_config_change_noop },
  { "irc_lag_min_show",
    N_("minimum lag to show (in seconds)"),
    OPTION_TYPE_INT, 0, INT_MAX, 1, NULL, NULL,
    &irc_cfg_irc_lag_min_show, NULL, irc_config_change_noop },
  { "irc_lag_disconnect",
    N_("disconnect after important lag (in minutes, 0 = never disconnect)"),
    OPTION_TYPE_INT, 0, INT_MAX, 5, NULL, NULL,
    &irc_cfg_irc_lag_disconnect, NULL, irc_config_change_noop },
  { "irc_anti_flood",
    N_("anti-flood: # seconds between two user messages (0 = no anti-flood)"),
    OPTION_TYPE_INT, 0, 5, 2, NULL, NULL,
    &irc_cfg_irc_anti_flood, NULL, irc_config_change_noop },
  { "irc_highlight",
    N_("comma separated list of words to highlight (case insensitive comparison, "
       "words may begin or end with \"*\" for partial match)"),
    OPTION_TYPE_STRING, 0, 0, 0, "", NULL,
    NULL, &irc_cfg_irc_highlight, irc_config_change_noop },
  { "irc_colors_receive",
    N_("when off, colors codes are ignored in incoming messages"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &irc_cfg_irc_colors_receive, NULL, irc_config_change_noop },
  { "irc_colors_send",
    N_("allow user to send colors with special codes (^Cb=bold, ^Ccxx=color, "
       "^Ccxx,yy=color+background, ^Cu=underline, ^Cr=reverse)"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &irc_cfg_irc_colors_send, NULL, irc_config_change_noop },
  { "irc_send_unknown_commands",
    N_("send unknown commands to IRC server"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_FALSE, NULL, NULL,
    &irc_cfg_irc_send_unknown_commands, NULL, irc_config_change_noop },
  { NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }
};

/* config, dcc section */

int irc_cfg_dcc_auto_accept_files;
int irc_cfg_dcc_auto_accept_chats;
int irc_cfg_dcc_timeout;
int irc_cfg_dcc_blocksize;
int irc_cfg_dcc_fast_send;
char *irc_cfg_dcc_port_range;
char *irc_cfg_dcc_own_ip;
char *irc_cfg_dcc_download_path;
char *irc_cfg_dcc_upload_path;
int irc_cfg_dcc_convert_spaces;
int irc_cfg_dcc_auto_rename;
int irc_cfg_dcc_auto_resume;

t_config_option irc_options_dcc[] =
{ { "dcc_auto_accept_files",
    N_("automatically accept incoming dcc files"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_FALSE, NULL, NULL,
    &irc_cfg_dcc_auto_accept_files, NULL, irc_config_change_noop },
  { "dcc_auto_accept_chats",
    N_("automatically accept dcc chats (use carefully!)"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_FALSE, NULL, NULL,
    &irc_cfg_dcc_auto_accept_chats, NULL, irc_config_change_noop },
  { "dcc_timeout",
    N_("timeout for dcc request (in seconds)"),
    OPTION_TYPE_INT, 5, INT_MAX, 300, NULL, NULL,
    &irc_cfg_dcc_timeout, NULL, irc_config_change_noop },
  { "dcc_blocksize",
    N_("block size for dcc packets in bytes (default: 65536)"),
    OPTION_TYPE_INT, IRC_DCC_MIN_BLOCKSIZE, IRC_DCC_MAX_BLOCKSIZE, 65536, NULL, NULL,
    &irc_cfg_dcc_blocksize, NULL, irc_config_change_noop },
  { "dcc_fast_send",
    N_("does not wait for ACK when sending file"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &irc_cfg_dcc_fast_send, NULL, irc_config_change_noop },
  { "dcc_port_range",
    N_("restricts outgoing dcc to use only ports in the given range "
       "(useful for NAT) (syntax: a single port, ie. 5000 or a port "
       "range, ie. 5000-5015, empty value means any port)"),
    OPTION_TYPE_STRING, 0, 0, 0, "", NULL,
    NULL, &irc_cfg_dcc_port_range, irc_config_change_noop },
  { "dcc_own_ip",
    N_("IP or DNS address used for outgoing dcc "
       "(if empty, local interface IP is used)"),
    OPTION_TYPE_STRING, 0, 0, 0, "", NULL,
    NULL, &irc_cfg_dcc_own_ip, irc_config_change_noop },
  { "dcc_download_path",
    N_("path for writing incoming files with dcc (default: user home)"),
    OPTION_TYPE_STRING, 0, 0, 0, "%h/dcc", NULL,
    NULL, &irc_cfg_dcc_download_path, irc_config_change_noop },
  { "dcc_upload_path",
    N_("path for reading files when sending thru dcc (when no path is "
       "specified)"),
    OPTION_TYPE_STRING, 0, 0, 0, "~", NULL,
    NULL, &irc_cfg_dcc_upload_path, irc_config_change_noop },
  { "dcc_convert_spaces",
    N_("convert spaces to underscores when sending files"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &irc_cfg_dcc_convert_spaces, NULL, irc_config_change_noop },
  { "dcc_auto_rename",
    N_("rename incoming files if already exists (add '.1', '.2', ...)"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &irc_cfg_dcc_auto_rename, NULL, irc_config_change_noop },
  { "dcc_auto_resume",
    N_("automatically resume dcc transfer if connection with remote host is "
       "loosed"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &irc_cfg_dcc_auto_resume, NULL, irc_config_change_noop },
  { NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }
};

/* config, log section */

int irc_cfg_log_auto_server;
int irc_cfg_log_auto_channel;
int irc_cfg_log_auto_private;
int irc_cfg_log_hide_nickserv_pwd;

t_config_option irc_options_log[] =
{ { "log_auto_server",
    N_("automatically log server messages"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_FALSE, NULL, NULL,
    &irc_cfg_log_auto_server, NULL, irc_config_change_log },
  { "log_auto_channel",
    N_("automatically log channel chats"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_FALSE, NULL, NULL,
    &irc_cfg_log_auto_channel, NULL, irc_config_change_log },
  { "log_auto_private",
    N_("automatically log private chats"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_FALSE, NULL, NULL,
    &irc_cfg_log_auto_private, NULL, irc_config_change_log },
  { "log_hide_nickserv_pwd",
    N_("hide password displayed by nickserv"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &irc_cfg_log_hide_nickserv_pwd, NULL, irc_config_change_noop },
  { NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }
};
  
/* config, server section */

static t_irc_server cfg_server;

t_config_option irc_options_server[] =
{ { "server_name",
    N_("name associated to IRC server (for display only)"),
    OPTION_TYPE_STRING, 0, 0, 0, "", NULL,
    NULL, &(cfg_server.name), NULL },
  { "server_autoconnect",
    N_("automatically connect to server when WeeChat is starting"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &(cfg_server.autoconnect), NULL, NULL },
  { "server_autoreconnect",
    N_("automatically reconnect to server when disconnected"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &(cfg_server.autoreconnect), NULL, NULL },
  { "server_autoreconnect_delay",
    N_("delay (in seconds) before trying again to reconnect to server"),
    OPTION_TYPE_INT, 0, 65535, 30, NULL, NULL,
    &(cfg_server.autoreconnect_delay), NULL, NULL },
  { "server_address",
    N_("IP address or hostname of IRC server"),
    OPTION_TYPE_STRING, 0, 0, 0, "", NULL,
    NULL, &(cfg_server.address), NULL },
  { "server_port",
    N_("port for connecting to server"),
    OPTION_TYPE_INT, 0, 65535, 6667, NULL, NULL,
    &(cfg_server.port), NULL, NULL },
  { "server_ipv6",
    N_("use IPv6 protocol for server communication"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_FALSE, NULL, NULL,
    &(cfg_server.ipv6), NULL, NULL },
  { "server_ssl",
    N_("use SSL for server communication"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_FALSE, NULL, NULL,
    &(cfg_server.ssl), NULL, NULL },
  { "server_password",
    N_("password for IRC server"),
    OPTION_TYPE_STRING, 0, 0, 0, "", NULL,
    NULL, &(cfg_server.password), NULL },
  { "server_nick1",
    N_("nickname to use on IRC server"),
    OPTION_TYPE_STRING, 0, 0, 0, "", NULL,
    NULL, &(cfg_server.nick1), NULL },
  { "server_nick2",
    N_("alternate nickname to use on IRC server (if nickname is already used)"),
    OPTION_TYPE_STRING, 0, 0, 0, "", NULL,
    NULL, &(cfg_server.nick2), NULL },
  { "server_nick3",
    N_("2nd alternate nickname to use on IRC server (if alternate nickname is "
       "already used)"),
    OPTION_TYPE_STRING, 0, 0, 0, "", NULL,
    NULL, &(cfg_server.nick3), NULL },
  { "server_username",
    N_("user name to use on IRC server"),
    OPTION_TYPE_STRING, 0, 0, 0, "", NULL,
    NULL, &(cfg_server.username), NULL },
  { "server_realname",
    N_("real name to use on IRC server"),
    OPTION_TYPE_STRING, 0, 0, 0, "", NULL,
    NULL, &(cfg_server.realname), NULL },
  { "server_hostname",
    N_("custom hostname/IP for server (optional, if empty local hostname is "
       "used)"),
    OPTION_TYPE_STRING, 0, 0, 0, "", NULL,
    NULL, &(cfg_server.hostname), NULL },
  { "server_command",
    N_("command(s) to run when connected to server (many commands should be "
       "separated by ';', use '\\;' for a semicolon, special variables $nick, "
       "$channel and $server are replaced by their value)"),
    OPTION_TYPE_STRING, 0, 0, 0, "", NULL,
    NULL, &(cfg_server.command), NULL },
  { "server_command_delay",
    N_("delay (in seconds) after command was executed (example: give some time "
       "for authentication)"),
    OPTION_TYPE_INT, 0, 3600, 0, NULL, NULL,
    &(cfg_server.command_delay), NULL, NULL },
  { "server_autojoin",
    N_("comma separated list of channels to join when connected to server "
       "(example: \"#chan1,#chan2,#chan3 key1,key2\")"),
    OPTION_TYPE_STRING, 0, 0, 0, "", NULL,
    NULL, &(cfg_server.autojoin), NULL },
  { "server_autorejoin",
    N_("automatically rejoin channels when kicked"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &(cfg_server.autorejoin), NULL, NULL },
  { "server_notify_levels",
    N_("comma separated list of notify levels for channels of this server "
       "(format: #channel:1,..), a channel name '*' is reserved for server "
       "default notify level"),
    OPTION_TYPE_STRING, 0, 0, 0, "", NULL,
    NULL, &(cfg_server.notify_levels), irc_config_change_notify_levels },
  { NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }
};

char *weechat_protocol_config_sections[] =
{ "irc", "dcc",
  "log", "server",
  NULL
};

t_config_option *weechat_protocol_config_options[] =
{ irc_options_irc, irc_options_dcc,
  irc_options_log, NULL,
  NULL };

t_config_func_read_option *irc_config_read_functions[] =
{ config_file_read_option, config_file_read_option,
  config_file_read_option, irc_config_read_server,
  NULL
};

t_config_func_write_options *irc_config_write_functions[] =
{ config_file_write_options, config_file_write_options,
  config_file_write_options, irc_config_write_servers,
  NULL
};

t_config_func_write_options *irc_config_write_default_functions[] =
{ config_file_write_options_default_values, config_file_write_options_default_values,
  config_file_write_options_default_values, irc_config_write_servers_default_values,
  NULL
};


/*
 * irc_config_change_noop: called when an option is changed by /set command
 *                         and that no special action is needed after that
 */

void
irc_config_change_noop ()
{
    /* do nothing */
}

/*
 * irc_config_change_one_server_buffer: called when the "one server buffer"
 *                                      setting is changed
 */

void
irc_config_change_one_server_buffer ()
{
    if (irc_cfg_irc_one_server_buffer)
        irc_buffer_merge_servers (gui_current_window);
    else
        irc_buffer_split_server (gui_current_window);
}

/*
 * irc_config_change_away_check: called when away check is changed
 */

void
irc_config_change_away_check ()
{
    if (irc_hook_timer_check_away)
    {
        weechat_hook_remove (irc_hook_timer_check_away);
        irc_hook_timer_check_away = NULL;
    }
    if (irc_cfg_irc_away_check == 0)
    {
        /* reset away flag for all nicks/chans/servers */
        irc_server_remove_away ();
    }
    weechat_hook_add_timer (irc_cfg_irc_away_check * 60 * 1000,
                            irc_server_timer_check_away,
                            NULL);
}

/*
 * irc_config_change_log: called when log settings are changed
 *                        (for server/channel/private logging)
 */

void
irc_config_change_log ()
{
    t_gui_buffer *ptr_buffer;
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
                if (irc_cfg_log_auto_server && !ptr_buffer->log_file)
                    gui_log_start (ptr_buffer);
                else if (!irc_cfg_log_auto_server && ptr_buffer->log_file)
                    gui_log_end (ptr_buffer);
            }
            if (ptr_server && ptr_channel)
            {
                if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                {
                    if (irc_cfg_log_auto_channel && !ptr_buffer->log_file)
                        gui_log_start (ptr_buffer);
                    else if (!irc_cfg_log_auto_channel && ptr_buffer->log_file)
                        gui_log_end (ptr_buffer);
                }
                else
                {
                    if (irc_cfg_log_auto_private && !ptr_buffer->log_file)
                        gui_log_start (ptr_buffer);
                    else if (!irc_cfg_log_auto_private && ptr_buffer->log_file)
                        gui_log_end (ptr_buffer);
                }
            }
        }
    }
}

/*
 * irc_config_change_notify_levels: called when notify level is changed
 */

void
irc_config_change_notify_levels ()
{
}

/*
 * irc_config_create_dirs: create configuratoin directories (read from configuration file)
 */

void
irc_config_create_dirs ()
{
    char *dir1, *dir2;
    
    /* create DCC download directory */
    dir1 = weechat_strreplace (irc_cfg_dcc_download_path, "~", getenv ("HOME"));
    dir2 = weechat_strreplace (dir1, "%h", weechat_home);
    (void) weechat_create_dir (dir2, 0700);
    if (dir1)
        free (dir1);
    if (dir2)
        free (dir2);
}

/*
 * irc_config_get_server_option_ptr: get a pointer to a server configuration option
 */

void *
irc_config_get_server_option_ptr (t_irc_server *server, char *option_name)
{
    if (weechat_strcasecmp (option_name, "server_name") == 0)
        return (void *)(&server->name);
    if (weechat_strcasecmp (option_name, "server_autoconnect") == 0)
        return (void *)(&server->autoconnect);
    if (weechat_strcasecmp (option_name, "server_autoreconnect") == 0)
        return (void *)(&server->autoreconnect);
    if (weechat_strcasecmp (option_name, "server_autoreconnect_delay") == 0)
        return (void *)(&server->autoreconnect_delay);
    if (weechat_strcasecmp (option_name, "server_address") == 0)
        return (void *)(&server->address);
    if (weechat_strcasecmp (option_name, "server_port") == 0)
        return (void *)(&server->port);
    if (weechat_strcasecmp (option_name, "server_ipv6") == 0)
        return (void *)(&server->ipv6);
    if (weechat_strcasecmp (option_name, "server_ssl") == 0)
        return (void *)(&server->ssl);
    if (weechat_strcasecmp (option_name, "server_password") == 0)
        return (void *)(&server->password);
    if (weechat_strcasecmp (option_name, "server_nick1") == 0)
        return (void *)(&server->nick1);
    if (weechat_strcasecmp (option_name, "server_nick2") == 0)
        return (void *)(&server->nick2);
    if (weechat_strcasecmp (option_name, "server_nick3") == 0)
        return (void *)(&server->nick3);
    if (weechat_strcasecmp (option_name, "server_username") == 0)
        return (void *)(&server->username);
    if (weechat_strcasecmp (option_name, "server_realname") == 0)
        return (void *)(&server->realname);
    if (weechat_strcasecmp (option_name, "server_hostname") == 0)
        return (void *)(&server->hostname);
    if (weechat_strcasecmp (option_name, "server_command") == 0)
        return (void *)(&server->command);
    if (weechat_strcasecmp (option_name, "server_command_delay") == 0)
        return (void *)(&server->command_delay);
    if (weechat_strcasecmp (option_name, "server_autojoin") == 0)
        return (void *)(&server->autojoin);
    if (weechat_strcasecmp (option_name, "server_autorejoin") == 0)
        return (void *)(&server->autorejoin);
    if (weechat_strcasecmp (option_name, "server_notify_levels") == 0)
        return (void *)(&server->notify_levels);
    /* option not found */
    return NULL;
}

/*
 * irc_config_set_server_value: set new value for an option of a server
 *                              return:  0 if success
 *                                      -1 if option not found
 *                                      -2 if bad value
 */

int
irc_config_set_server_value (t_irc_server *server, char *option_name,
                             char *value)
{
    t_config_option *ptr_option;
    int i;
    void *ptr_data;
    int int_value;
    
    ptr_data = irc_config_get_server_option_ptr (server, option_name);
    if (!ptr_data)
        return -1;
    
    ptr_option = NULL;
    for (i = 0; irc_options_server[i].name; i++)
    {
        /* if option found, return pointer */
        if (weechat_strcasecmp (irc_options_server[i].name, option_name) == 0)
        {
            ptr_option = &irc_options_server[i];
            break;
        }
    }
    if (!ptr_option)
        return -1;
    
    switch (ptr_option->type)
    {
        case OPTION_TYPE_BOOLEAN:
            int_value = config_option_option_get_boolean_value (value);
            switch (int_value)
            {
                case BOOL_TRUE:
                    *((int *)(ptr_data)) = BOOL_TRUE;
                    break;
                case BOOL_FALSE:
                    *((int *)(ptr_data)) = BOOL_FALSE;
                    break;
                default:
                    return -2;
            }
            break;
        case OPTION_TYPE_INT:
            int_value = atoi (value);
            if ((int_value < ptr_option->min) || (int_value > ptr_option->max))
                return -2;
            *((int *)(ptr_data)) = int_value;
            break;
        case OPTION_TYPE_INT_WITH_STRING:
            int_value = config_option_get_pos_array_values (ptr_option->array_values,
                                                            value);
            if (int_value < 0)
                return -2;
            *((int *)(ptr_data)) = int_value;
            break;
        case OPTION_TYPE_COLOR:
            if (!gui_color_assign ((int *)ptr_data, value))
                return -2;
            break;
        case OPTION_TYPE_STRING:
            if (*((char **)ptr_data))
                free (*((char **)ptr_data));
            *((char **)ptr_data) = strdup (value);
            break;
    }
    if (ptr_option->handler_change != NULL)
        (void) (ptr_option->handler_change());
    return 0;
}

/*
 * irc_config_allocate_server: allocate a new server
 */

int
irc_config_allocate_server (char *filename, int line_number)
{
    if (!cfg_server.name
        || !cfg_server.address
        || cfg_server.port < 0
        || !cfg_server.nick1
        || !cfg_server.nick2
        || !cfg_server.nick3
        || !cfg_server.username
        || !cfg_server.realname)
    {
        irc_server_free_all ();
        gui_chat_printf (NULL,
                         _("%s %s, line %d: new server, but previous was "
                           "incomplete\n"),
                         WEECHAT_WARNING, filename, line_number);
        return 0;
        
    }
    if (irc_server_name_already_exists (cfg_server.name))
    {
        irc_server_free_all ();
        gui_chat_printf (NULL,
                         _("%s %s, line %d: server '%s' already exists\n"),
                         WEECHAT_WARNING, filename, line_number,
                         cfg_server.name);
        return 0;
    }
    if (!irc_server_new (cfg_server.name,
                         cfg_server.autoconnect, cfg_server.autoreconnect,
                         cfg_server.autoreconnect_delay, 0, cfg_server.address,
                         cfg_server.port, cfg_server.ipv6, cfg_server.ssl,
                         cfg_server.password, cfg_server.nick1,
                         cfg_server.nick2, cfg_server.nick3,
                         cfg_server.username, cfg_server.realname,
                         cfg_server.hostname, cfg_server.command,
                         cfg_server.command_delay, cfg_server.autojoin,
                         cfg_server.autorejoin, cfg_server.notify_levels))
    {
        irc_server_free_all ();
        gui_chat_printf (NULL,
                         _("%s %s, line %d: unable to create server\n"),
                         WEECHAT_WARNING, filename, line_number);
        return 0;
    }
    
    irc_server_destroy (&cfg_server);
    irc_server_init (&cfg_server);
    
    return 1;
}

/*
 * irc_config_read_server: read a server option in configuration file
 *                         Return:  0 = successful
 *                                 -1 = option not found
 *                                 -2 = bad format/value
 */

int
irc_config_read_server (t_config_option *options,
                        char *option_name, char *value)
{
    /* make C compiler happy */
    (void) options;
    
    if (option_name)
    {
        if (value[0])
        {
            /* bind key (overwrite any binding with same key) */
            gui_keyboard_bind (option_name, value);
        }
        else
        {
            /* unbin key if no value given */
            gui_keyboard_unbind (option_name);
        }
    }
    else
    {
        /* does nothing for new [key] section */
    }
    
    /* all ok */
    return 0;
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
    irc_server_init (&cfg_server);
    
    return config_file_read (weechat_protocol_config_sections,
                             weechat_protocol_config_options,
                             irc_config_read_functions,
                             irc_config_write_default_functions,
                             IRC_CONFIG_NAME);
}

/*
 * irc_config_write_servers: write servers sections in configuration file
 *                           Return:  0 = successful
 *                                   -1 = write error
 */

int
irc_config_write_servers (FILE *file, char *section_name,
                          t_config_option *options)
{
    t_irc_server *ptr_server;
    
    /* make C compiler happy */
    (void) options;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (!ptr_server->temp_server)
        {
            weechat_iconv_fprintf (file, "\n[%s]\n", section_name);
            weechat_iconv_fprintf (file, "server_name = \"%s\"\n", ptr_server->name);
            weechat_iconv_fprintf (file, "server_autoconnect = %s\n",
                                   (ptr_server->autoconnect) ? "on" : "off");
            weechat_iconv_fprintf (file, "server_autoreconnect = %s\n",
                                   (ptr_server->autoreconnect) ? "on" : "off");
            weechat_iconv_fprintf (file, "server_autoreconnect_delay = %d\n",
                                   ptr_server->autoreconnect_delay);
            weechat_iconv_fprintf (file, "server_address = \"%s\"\n", ptr_server->address);
            weechat_iconv_fprintf (file, "server_port = %d\n", ptr_server->port);
            weechat_iconv_fprintf (file, "server_ipv6 = %s\n",
                                   (ptr_server->ipv6) ? "on" : "off");
            weechat_iconv_fprintf (file, "server_ssl = %s\n",
                                   (ptr_server->ssl) ? "on" : "off");
            weechat_iconv_fprintf (file, "server_password = \"%s\"\n",
                                   (ptr_server->password) ? ptr_server->password : "");
            weechat_iconv_fprintf (file, "server_nick1 = \"%s\"\n", ptr_server->nick1);
            weechat_iconv_fprintf (file, "server_nick2 = \"%s\"\n", ptr_server->nick2);
            weechat_iconv_fprintf (file, "server_nick3 = \"%s\"\n", ptr_server->nick3);
            weechat_iconv_fprintf (file, "server_username = \"%s\"\n", ptr_server->username);
            weechat_iconv_fprintf (file, "server_realname = \"%s\"\n", ptr_server->realname);
            weechat_iconv_fprintf (file, "server_hostname = \"%s\"\n",
                                   (ptr_server->hostname) ? ptr_server->hostname : "");
            weechat_iconv_fprintf (file, "server_command = \"%s\"\n",
                                   (ptr_server->command) ? ptr_server->command : "");
            weechat_iconv_fprintf (file, "server_command_delay = %d\n", ptr_server->command_delay);
            weechat_iconv_fprintf (file, "server_autojoin = \"%s\"\n",
                                   (ptr_server->autojoin) ? ptr_server->autojoin : "");
            weechat_iconv_fprintf (file, "server_autorejoin = %s\n",
                                   (ptr_server->autorejoin) ? "on" : "off");
            weechat_iconv_fprintf (file, "server_notify_levels = \"%s\"\n",
                                   (ptr_server->notify_levels) ? ptr_server->notify_levels : "");
        }
    }
    
    /* all ok */
    return 0;
}

/*
 * irc_config_write_server_default_values: write server section with default values
 *                                         in configuration file 
 *                                         Return:  0 = successful
 *                                                 -1 = write error
 */

int
irc_config_write_server_default_values (FILE *file, char *section_name,
                                        t_config_option *options)
{
    /* make C compiler happy */
    (void) options;
    
    struct passwd *my_passwd;
    char *realname, *pos;
    
    weechat_iconv_fprintf (file, "\n[%s]\n", section_name);
    
    weechat_iconv_fprintf (file, "server_name = \"freenode\"\n");
    weechat_iconv_fprintf (file, "server_autoconnect = on\n");
    weechat_iconv_fprintf (file, "server_autoreconnect = on\n");
    weechat_iconv_fprintf (file, "server_autoreconnect_delay = 30\n");
    weechat_iconv_fprintf (file, "server_address = \"irc.freenode.net\"\n");
    weechat_iconv_fprintf (file, "server_port = 6667\n");
    weechat_iconv_fprintf (file, "server_ipv6 = off\n");
    weechat_iconv_fprintf (file, "server_ssl = off\n");
    weechat_iconv_fprintf (file, "server_password = \"\"\n");
    
    /* Get the user's name from /etc/passwd */
    if ((my_passwd = getpwuid (geteuid ())) != NULL)
    {
        weechat_iconv_fprintf (file, "server_nick1 = \"%s\"\n", my_passwd->pw_name);
        weechat_iconv_fprintf (file, "server_nick2 = \"%s1\"\n", my_passwd->pw_name);
        weechat_iconv_fprintf (file, "server_nick3 = \"%s2\"\n", my_passwd->pw_name);
        weechat_iconv_fprintf (file, "server_username = \"%s\"\n", my_passwd->pw_name);
        if ((!my_passwd->pw_gecos)
            || (my_passwd->pw_gecos[0] == '\0')
            || (my_passwd->pw_gecos[0] == ',')
            || (my_passwd->pw_gecos[0] == ' '))
            weechat_iconv_fprintf (file, "server_realname = \"%s\"\n", my_passwd->pw_name);
        else
        {
            realname = strdup (my_passwd->pw_gecos);
            pos = strchr (realname, ',');
            if (pos)
                pos[0] = '\0';
            weechat_iconv_fprintf (file, "server_realname = \"%s\"\n",
                                   realname);
            if (pos)
                pos[0] = ',';
            free (realname);
        }
    }
    else
    {
        /* default values if /etc/passwd can't be read */
        weechat_iconv_fprintf (file, "server_nick1 = \"weechat1\"\n");
        weechat_iconv_fprintf (file, "server_nick2 = \"weechat2\"\n");
        weechat_iconv_fprintf (file, "server_nick3 = \"weechat3\"\n");
        weechat_iconv_fprintf (file, "server_username = \"weechat\"\n");
        weechat_iconv_fprintf (file, "server_realname = \"weechat\"\n");
    }
    
    weechat_iconv_fprintf (file, "server_hostname = \"\"\n");
    weechat_iconv_fprintf (file, "server_command = \"\"\n");
    weechat_iconv_fprintf (file, "server_command_delay = 0\n");
    weechat_iconv_fprintf (file, "server_autojoin = \"\"\n");
    weechat_iconv_fprintf (file, "server_autorejoin = on\n");
    weechat_iconv_fprintf (file, "server_notify_levels = \"\"\n");
    
    /* all ok */
    return 0;
}

/*
 * irc_config_write: write IRC configuration file
 *                   return:  0 if ok
 *                          < 0 if error
 */

int
irc_config_write ()
{
    return config_file_write (weechat_protocol_config_sections,
                              weechat_protocol_config_options,
                              irc_config_write_functions,
                              IRC_CONFIG_NAME);
}
