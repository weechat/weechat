/*
 * Copyright (c) 2003 by FlashCode <flashcode@flashtux.org>
 *                       Bounga <bounga@altern.org>
 *                       Xahlexx <xahlexx@tuxisland.org>
 * See README for License detail.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


/* config.c: WeeChat configuration */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#include "weechat.h"
#include "config.h"
#include "command.h"
#include "irc/irc.h"
#include "gui/gui.h"


/* config sections */

t_config_section config_sections[CONFIG_NUMBER_SECTIONS] =
{ { CONFIG_SECTION_LOOK, "look" },
  { CONFIG_SECTION_COLORS, "colors" },
  { CONFIG_SECTION_HISTORY, "history" },
  { CONFIG_SECTION_LOG, "log" },
  { CONFIG_SECTION_DCC, "dcc" },
  { CONFIG_SECTION_PROXY, "proxy" },
  { CONFIG_SECTION_ALIAS, "alias" },
  { CONFIG_SECTION_SERVER, "server" }
};

/* config, look & feel section */

int cfg_look_startup_logo;
int cfg_look_startup_version;
char *cfg_look_weechat_slogan;
int cfg_look_color_nicks;
int cfg_look_color_actions;
int cfg_look_remove_colors_from_msgs;
int cfg_look_nicklist;
int cfg_look_nicklist_position;
char *cfg_look_nicklist_position_values[] =
{ "left", "right", "top", "bottom", NULL };
int cfg_look_nicklist_min_size;
int cfg_look_nicklist_max_size;
int cfg_look_nickmode;
int cfg_look_nickmode_empty;
char *cfg_look_no_nickname;
char *cfg_look_completor;

t_config_option weechat_options_look[] =
{ { "look_startup_logo", N_("display " WEECHAT_NAME " logo at startup"),
    N_("display " WEECHAT_NAME " logo at startup"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE,
    NULL, NULL, &cfg_look_startup_logo, NULL, NULL },
  { "look_startup_version", N_("display " WEECHAT_NAME " version at startup"),
    N_("display " WEECHAT_NAME " version at startup"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE,
    NULL, NULL, &cfg_look_startup_version, NULL, NULL },
  { "look_weechat_slogan", N_(WEECHAT_NAME "slogan"),
    N_(WEECHAT_NAME "slogan (if empty, slogan is not used)"),
    OPTION_TYPE_STRING, 0, 0, 0,
    "the geekest IRC client!", NULL, NULL, &cfg_look_weechat_slogan, NULL },
  { "look_color_nicks", N_("display nick names with different colors"),
    N_("display nick names with different colors"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE,
    NULL, NULL, &cfg_look_color_nicks, NULL, NULL },
  { "look_color_actions", N_("display actions with different colors"),
    N_("display actions with different colors"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE,
    NULL, NULL, &cfg_look_color_actions, NULL, NULL },
  { "look_remove_colors_from_msgs", N_("remove colors from incoming messages"),
    N_("remove colors from incoming messages"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE,
    NULL, NULL, &cfg_look_remove_colors_from_msgs, NULL, NULL },
  { "look_nicklist", N_("display nicklist window"),
    N_("display nicklist window (for channel windows)"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE,
    NULL, NULL, &cfg_look_nicklist, NULL, NULL },
  { "look_nicklist_position", N_("nicklist position"),
    N_("nicklist position (top, left, right (default), bottom)"),
    OPTION_TYPE_INT_WITH_STRING, 0, 0, 0,
    "right", cfg_look_nicklist_position_values, &cfg_look_nicklist_position, NULL, NULL },
  { "look_nicklist_min_size", N_("min size for nicklist"),
    N_("min size for nicklist (width or height, depending on look_nicklist_position "
    "(0 = no min size))"),
    OPTION_TYPE_INT, 0, 100, 0,
    NULL, NULL, &cfg_look_nicklist_min_size, NULL, NULL },
  { "look_nicklist_max_size", N_("max size for nicklist"),
    N_("max size for nicklist (width or height, depending on look_nicklist_position "
    "(0 = no max size; if min == max and > 0, then size is fixed))"),
    OPTION_TYPE_INT, 0, 100, 0,
    NULL, NULL, &cfg_look_nicklist_max_size, NULL, NULL },
  { "look_no_nickname", N_("text to display instead of nick when not connected"),
    N_("text to display instead of nick when not connected"),
    OPTION_TYPE_STRING, 0, 0, 0,
    "-cmd-", NULL, NULL, &cfg_look_no_nickname, NULL },
  { "look_nickmode", N_("display nick mode ((half)op/voice) before each nick"),
    N_("display nick mode ((half)op/voice) before each nick"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE,
    NULL, NULL, &cfg_look_nickmode, NULL, NULL },
  { "look_nickmode_empty", N_("display space if nick mode is not (half)op/voice"),
    N_("display space if nick mode is not (half)op/voice"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_FALSE,
    NULL, NULL, &cfg_look_nickmode_empty, NULL, NULL },
  { "look_nick_completor", N_("the string inserted after nick completion"),
    N_("the string inserted after nick completion"),
    OPTION_TYPE_STRING, 0, 0, 0,
    ":", NULL, NULL, &cfg_look_completor, NULL },
  { NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }
};

/* config, colors section */

int cfg_col_title;
int cfg_col_title_bg;
int cfg_col_chat;
int cfg_col_chat_time;
int cfg_col_chat_time_sep;
int cfg_col_chat_prefix1;
int cfg_col_chat_prefix2;
int cfg_col_chat_nick;
int cfg_col_chat_host;
int cfg_col_chat_channel;
int cfg_col_chat_dark;
int cfg_col_chat_bg;
int cfg_col_status;
int cfg_col_status_active;
int cfg_col_status_data_msg;
int cfg_col_status_data_other;
int cfg_col_status_more;
int cfg_col_status_bg;
int cfg_col_input;
int cfg_col_input_channel;
int cfg_col_input_nick;
int cfg_col_input_bg;
int cfg_col_nick;
int cfg_col_nick_op;
int cfg_col_nick_halfop;
int cfg_col_nick_voice;
int cfg_col_nick_sep;
int cfg_col_nick_self;
int cfg_col_nick_private;
int cfg_col_nick_bg;

t_config_option weechat_options_colors[] =
{ /* title window */
  { "col_title", N_("color for title bar"),
    N_("color for title bar"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "gray", NULL, &cfg_col_title, NULL, NULL },
  { "col_title_bg", N_("background for title bar"),
    N_("background for title bar"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "blue", NULL, &cfg_col_title_bg, NULL, NULL },
  
  /* chat window */
  { "col_chat", N_("color for chat text"),
    N_("color for chat text"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "gray", NULL, &cfg_col_chat, NULL, NULL },
  { "col_chat_time", N_("color for time"),
    N_("color for time in chat window"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "gray", NULL, &cfg_col_chat_time, NULL, NULL },
  { "col_chat_time_sep", N_("color for time separator"),
    N_("color for time separator (chat window)"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "brown", NULL, &cfg_col_chat_time_sep, NULL, NULL },
  { "col_chat_prefix1", N_("color for 1st and 3rd char of prefix"),
    N_("color for 1st and 3rd char of prefix"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "lightcyan", NULL, &cfg_col_chat_prefix1, NULL, NULL },
  { "col_chat_prefix2", N_("color for middle char of prefix"),
    N_("color for middle char of prefix"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "white", NULL, &cfg_col_chat_prefix2, NULL, NULL },
  { "col_chat_nick", N_("color for nicks in actions"),
    N_("color for nicks in actions (chat window)"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "lightcyan", NULL, &cfg_col_chat_nick, NULL, NULL },
  { "col_chat_host", N_("color for hostnames"),
    N_("color for hostnames (chat window)"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "cyan", NULL, &cfg_col_chat_host, NULL, NULL },
  { "col_chat_channel", N_("color for channel names in actions"),
    N_("color for channel names in actions (chat window)"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "white", NULL, &cfg_col_chat_channel, NULL, NULL },
  { "col_chat_dark", N_("color for dark separators"),
    N_("color for dark separators (chat window)"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "green", NULL, &cfg_col_chat_dark, NULL, NULL },
  { "col_chat_bg", N_("background for chat"),
    N_("background for chat window"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "default", NULL, &cfg_col_chat_bg, NULL, NULL },
  
  /* status window */
  { "col_status", N_("color for status bar"),
    N_("color for status bar"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "gray", NULL, &cfg_col_status, NULL, NULL },
  { "col_status_active", N_("color for active window"),
    N_("color for active window (status bar)"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "yellow", NULL, &cfg_col_status_active, NULL, NULL },
  { "col_status_data_msg", N_("color for window with new messages"),
    N_("color for window with new messages (status bar)"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "lightred", NULL, &cfg_col_status_data_msg, NULL, NULL },
  { "col_status_data_other", N_("color for window with new data (not messages)"),
    N_("color for window with new data (not messages) (status bar)"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "lightmagenta", NULL, &cfg_col_status_data_other, NULL, NULL },
  { "col_status_more", N_("color for \"*MORE*\" text"),
    N_("color for window with new data (status bar)"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "white", NULL, &cfg_col_status_more, NULL, NULL },
  { "col_status_bg", N_("background for status window"),
    N_("background for status window"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "blue", NULL, &cfg_col_status_bg, NULL, NULL },
  
  /* input window */
  { "col_input", N_("color for input text"),
    N_("color for input text"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "gray", NULL, &cfg_col_input, NULL, NULL },
  { "col_input_channel", N_("color for input text (channel name)"),
    N_("color for input text (channel name)"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "white", NULL, &cfg_col_input_channel, NULL, NULL },
  { "col_input_nick", N_("color for input text (nick name)"),
    N_("color for input text (nick name)"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "lightgreen", NULL, &cfg_col_input_nick, NULL, NULL },
  { "col_input_bg", N_("background for input window"),
    N_("background for input window"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "default", NULL, &cfg_col_input_bg, NULL, NULL },
  
  /* nick window */
  { "col_nick", N_("color for nicknames"),
    N_("color for nicknames"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "gray", NULL, &cfg_col_nick, NULL, NULL },
  { "col_nick_op", N_("color for operator symbol"),
    N_("color for operator symbol"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "lightgreen", NULL, &cfg_col_nick_op, NULL, NULL },
  { "col_nick_halfop", N_("color for half-operator symbol"),
    N_("color for half-operator symbol"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "lightmagenta", NULL, &cfg_col_nick_halfop, NULL, NULL },
  { "col_nick_voice", N_("color for voice symbol"),
    N_("color for voice symbol"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "yellow", NULL, &cfg_col_nick_voice, NULL, NULL },
  { "col_nick_sep", N_("color for nick separator"),
    N_("color for nick separator"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "blue", NULL, &cfg_col_nick_sep, NULL, NULL },
  { "col_nick_self", N_("color for local nick"),
    N_("color for local nick"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "white", NULL, &cfg_col_nick_self, NULL, NULL },
  { "col_nick_private", N_("color for other nick in private window"),
    N_("color for other nick in private window"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "brown", NULL, &cfg_col_nick_private, NULL, NULL },
  { "col_nick_bg", N_("background for nicknames"),
    N_("background for nicknames"),
    OPTION_TYPE_COLOR, 0, 0, 0,
    "default", NULL, &cfg_col_nick_bg, NULL, NULL },
  
  { NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }
};

/* config, history section */

int cfg_history_max_lines;
int cfg_history_max_commands;

t_config_option weechat_options_history[] =
{ { "history_max_lines", N_("max lines in history (per window)"),
    N_("maximum number of lines in history "
    "for one server/channel/private window (0 = unlimited)"),
    OPTION_TYPE_INT, 0, INT_MAX, 4096,
    NULL, NULL, &cfg_history_max_lines, NULL, NULL },
  { "history_max_commands", N_("max user commands in history"),
    N_("maximum number of user commands in history (0 = unlimited)"),
    OPTION_TYPE_INT, 0, INT_MAX, 100,
    NULL, NULL, &cfg_history_max_commands, NULL, NULL },
  { NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }
};

/* config, log section */

int cfg_log_auto_channels;
int cfg_log_auto_private;
char *cfg_log_path;
char *cfg_log_name;
char *cfg_log_timestamp;
char *cfg_log_start_string;
char *cfg_log_end_string;

t_config_option weechat_options_log[] =
{ { "log_auto_channels", N_("automatically log channel chats"),
    N_("automatically log channel chats"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE,
    NULL, NULL, &cfg_log_auto_channels, NULL, NULL },
  { "log_auto_private", N_("automatically log private chats"),
    N_("automatically log private chats"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE,
    NULL, NULL, &cfg_log_auto_private, NULL, NULL },
  { "log_path", N_("path for log files"),
    N_("path for " WEECHAT_NAME " log files"),
    OPTION_TYPE_STRING, 0, 0, 0,
    "~/.weechat/logs/", NULL, NULL, &cfg_log_path, NULL },
  { "log_name", N_("name for log files"),
    N_("name for log files (%S == irc server name, "
    "%N == channel name (or nickname if private chat)"),
    OPTION_TYPE_STRING, 0, 0, 0,
    "%S,%N.weechatlog", NULL, NULL, &cfg_log_name, NULL },
  { "log_timestamp", N_("timestamp for log"),
    N_("timestamp for log (see man strftime for date/time specifiers)"),
    OPTION_TYPE_STRING, 0, 0, 0,
    "~", NULL, NULL, &cfg_log_timestamp, NULL },
  { "log_start_string", N_("start string for log files"),
    N_("text writed when starting new log file "
    "(see man strftime for date/time specifiers)"),
    OPTION_TYPE_STRING, 0, 0, 0,
    "--- Log started %a %b %d %Y %H:%M:%s", NULL, NULL, &cfg_log_start_string, NULL },
  { "log_end_string", N_("end string for log files"),
    N_("text writed when ending log file "
    "(see man strftime for date/time specifiers)"),
    OPTION_TYPE_STRING, 0, 0, 0,
    "--- Log ended %a %b %d %Y %H:%M:%s", NULL, NULL, &cfg_log_end_string, NULL },
  { NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }
};

/* config, dcc section */

int cfg_dcc_auto_accept_files;
int cfg_dcc_auto_accept_max_size;
int cfg_dcc_auto_accept_chats;
int cfg_dcc_timeout;
char *cfg_dcc_download_path;
char *cfg_dcc_upload_path;
int cfg_dcc_auto_rename;
int cfg_dcc_auto_resume;

t_config_option weechat_options_dcc[] =
{ { "dcc_auto_accept_files", N_("automatically accept dcc files"),
    N_("automatically accept incoming dcc files"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE,
    NULL, NULL, &cfg_dcc_auto_accept_files, NULL, NULL },
  { "dcc_auto_accept_max_size", N_("max size when auto accepting file"),
    N_("maximum size for incoming file when automatically accepted"),
    OPTION_TYPE_INT, 0, INT_MAX, 0,
    NULL, NULL, &cfg_dcc_auto_accept_max_size, NULL, NULL },
  { "dcc_auto_accept_chats", N_("automatically accept dcc chats"),
    N_("automatically accept dcc chats (use carefully!)"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE,
    NULL, NULL, &cfg_dcc_auto_accept_chats, NULL, NULL },
  { "dcc_timeout", N_("timeout for dcc request"),
    N_("timeout for dcc request (in seconds)"),
    OPTION_TYPE_INT, 1, INT_MAX, 300,
    NULL, NULL, &cfg_dcc_timeout, NULL, NULL },
  { "dcc_download_path", N_("path for incoming files with dcc"),
    N_("path for writing incoming files with dcc (default: user home)"),
    OPTION_TYPE_STRING, 0, 0, 0, "~",
    NULL, NULL, &cfg_dcc_download_path, NULL },
  { "dcc_upload_path", N_("default path for sending files with dcc"),
    N_("path for reading files when sending thru dcc (when no path is specified)"),
    OPTION_TYPE_STRING, 0, 0, 0, "~",
    NULL, NULL, &cfg_dcc_upload_path, NULL },
  { "dcc_auto_rename", N_("automatically rename dcc files if already exists"),
    N_("rename incoming files if already exists (add '.1', '.2', ...)"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE,
    NULL, NULL, &cfg_dcc_auto_rename, NULL, NULL },
  { "dcc_auto_resume", N_("automatically resume aborted transfers"),
    N_("automatically resume dcc trsnafer if connection with remote host is loosed"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE,
    NULL, NULL, &cfg_dcc_auto_resume, NULL, NULL },
  { NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }
};

/* config, proxy section */

int cfg_proxy_use;
char *cfg_proxy_address;
int cfg_proxy_port;
char *cfg_proxy_password;

t_config_option weechat_options_proxy[] =
{ { "proxy_use", N_("use proxy"),
    N_("use a proxy server to connect to irc server"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_FALSE,
    NULL, NULL, &cfg_proxy_use, NULL, NULL },
  { "proxy_address", N_("proxy address"),
    N_("proxy server address (IP or hostname)"),
    OPTION_TYPE_STRING, 0, 0, 0,
    "", NULL, NULL, &cfg_proxy_address, NULL },
  { "proxy_port", N_("port for proxy"),
    N_("port for connecting to proxy server"),
    OPTION_TYPE_INT, 0, 65535, 1080,
    NULL, NULL, &cfg_proxy_port, NULL, NULL },
  { "proxy_password", N_("proxy password"),
    N_("password for proxy server"),
    OPTION_TYPE_STRING, 0, 0, 0,
    "", NULL, NULL, &cfg_proxy_password, NULL },
  { NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }
};

/* config, server section */

static t_irc_server cfg_server;

t_config_option weechat_options_server[] =
{ { "server_name", N_("server name"),
    N_("name associated to IRC server (for display only)"),
    OPTION_TYPE_STRING, 0, 0, 0,
    "", NULL, NULL, &(cfg_server.name), NULL },
  { "server_autoconnect", N_("automatically connect to server"),
    N_("automatically connect to server when " WEECHAT_NAME " is starting"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE,
    NULL, NULL, &(cfg_server.autoconnect), NULL, NULL },
  { "server_address", N_("server address or hostname"),
    N_("IP address or hostname of IRC server"),
    OPTION_TYPE_STRING, 0, 0, 0,
    "", NULL, NULL, &(cfg_server.address), NULL },
  { "server_port", N_("port for IRC server"),
    N_("port for connecting to server"),
    OPTION_TYPE_INT, 0, 65535, 6667,
    NULL, NULL, &(cfg_server.port), NULL, NULL },
  { "server_password", N_("server password"),
    N_("password for IRC server"),
    OPTION_TYPE_STRING, 0, 0, 0,
    "", NULL, NULL, &(cfg_server.password), NULL },
  { "server_nick1", N_("nickname for server"),
    N_("nickname to use on IRC server"),
    OPTION_TYPE_STRING, 0, 0, 0,
    "", NULL, NULL, &(cfg_server.nick1), NULL },
  { "server_nick2", N_("alternate nickname for server"),
    N_("alternate nickname to use on IRC server (if nickname is already used)"),
    OPTION_TYPE_STRING, 0, 0, 0,
    "", NULL, NULL, &(cfg_server.nick2), NULL },
  { "server_nick3", N_("2nd alternate nickname for server"),
    N_("2nd alternate nickname to use on IRC server (if alternate nickname is already used)"),
    OPTION_TYPE_STRING, 0, 0, 0,
    "", NULL, NULL, &(cfg_server.nick3), NULL },
  { "server_username", N_("user name for server"),
    N_("user name to use on IRC server"),
    OPTION_TYPE_STRING, 0, 0, 0,
    "", NULL, NULL, &(cfg_server.username), NULL },
  { "server_realname", N_("real name for server"),
    N_("real name to use on IRC server"),
    OPTION_TYPE_STRING, 0, 0, 0,
    "", NULL, NULL, &(cfg_server.realname), NULL },
  { NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }
};

/* all weechat options */

t_config_option *weechat_options[CONFIG_NUMBER_SECTIONS] =
{ weechat_options_look, weechat_options_colors, weechat_options_history,
  weechat_options_log, weechat_options_dcc, weechat_options_proxy,
  NULL, weechat_options_server
};


/*
 * get_pos_array_values: returns position of a string in an array of values
 *                       returns -1 if not found, otherwise position
 */

int
get_pos_array_values (char **array, char *string)
{
    int i;
    
    i = 0;
    while (array[i])
    {
        if (strcasecmp (array[i], string) == 0)
            return i;
        i++;
    }
    /* string not found in array */
    return -1;
}

/*
 * config_allocate_server: allocate a new server
 */

int
config_allocate_server (char *filename, int line_number)
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
        server_free_all ();
        gui_printf (NULL,
                    _("%s %s, line %d: new server, but previous was incomplete\n"),
                    WEECHAT_WARNING, filename, line_number);
        return 0;
        
    }
    if (server_name_already_exists (cfg_server.name))
    {
        server_free_all ();
        gui_printf (NULL,
                    _("%s %s, line %d: server '%s' already exists\n"),
                    WEECHAT_WARNING, filename, line_number, cfg_server.name);
        return 0;
    }
    if (!server_new (cfg_server.name,
        cfg_server.autoconnect, cfg_server.address, cfg_server.port,
        cfg_server.password, cfg_server.nick1, cfg_server.nick2,
        cfg_server.nick3, cfg_server.username, cfg_server.realname))
    {
        server_free_all ();
        gui_printf (NULL,
                    _("%s %s, line %d: unable to create server\n"),
                    WEECHAT_WARNING, filename, line_number);
        return 0;
    }
    
    server_destroy (&cfg_server);
    server_init (&cfg_server);
    
    return 1;
}

/*
 * config_default_values: initialize config variables with default values
 */

void
config_default_values ()
{
    int i, j, int_value;
    
    for (i = 0; i < CONFIG_NUMBER_SECTIONS; i++)
    {
        if ((i != CONFIG_SECTION_ALIAS) && (i != CONFIG_SECTION_SERVER))
        {
            for (j = 0; weechat_options[i][j].option_name; j++)
            {
                switch (weechat_options[i][j].option_type)
                {
                    case OPTION_TYPE_BOOLEAN:
                    case OPTION_TYPE_INT:
                        *weechat_options[i][j].ptr_int =
                            weechat_options[i][j].default_int;
                        break;
                    case OPTION_TYPE_INT_WITH_STRING:
                        int_value = get_pos_array_values (
                            weechat_options[i][j].array_values,
                            weechat_options[i][j].default_string);
                        if (int_value < 0)
                            gui_printf (NULL,
                                        _("%s unable to assign default int with string (\"%s\")\n"),
                                        weechat_options[i][j].default_string);
                        else
                            *weechat_options[i][j].ptr_int =
                                int_value;
                        break;
                    case OPTION_TYPE_COLOR:
                        if (!gui_assign_color (
                            weechat_options[i][j].ptr_int,
                            weechat_options[i][j].default_string))
                            gui_printf (NULL,
                                        _("%s unable to assign default color (\"%s\")\n"),
                                        weechat_options[i][j].default_string);
                        break;
                    case OPTION_TYPE_STRING:
                        *weechat_options[i][j].ptr_string =
                            strdup (weechat_options[i][j].default_string);
                        break;
                }
            }
        }
    }
}

/*
 * config_read: read WeeChat configuration
 *              returns:   0 = successful
 *                        -1 = config file file not found
 *                      < -1 = other error (fatal)
 */

int
config_read ()
{
    char *filename;
    FILE *file;
    int section, line_number, i, option_number, int_value;
    int server_found;
    char line[1024], *ptr_line, *pos, *pos2;

    filename =
        (char *) malloc ((strlen (getenv ("HOME")) + 64) * sizeof (char));
    sprintf (filename, "%s/.weechat/" WEECHAT_CONFIG_NAME, getenv ("HOME"));
    if ((file = fopen (filename, "rt")) == NULL)
    {
        gui_printf (NULL, _("%s config file \"%s\" not found.\n"),
                    WEECHAT_WARNING, filename);
        free (filename);
        return -1;
    }
    
    config_default_values ();
    server_init (&cfg_server);
    
    /* read config file */
    section = CONFIG_SECTION_NONE;
    server_found = 0;
    line_number = 0;
    while (!feof (file))
    {
        ptr_line = fgets (line, sizeof (line) - 1, file);
        line_number++;
        if (ptr_line)
        {
            /* skip spaces */
            while (ptr_line[0] == ' ')
                ptr_line++;
            /* not a comment and not an empty line */
            if ((ptr_line[0] != '#') && (ptr_line[0] != '\r')
                && (ptr_line[0] != '\n'))
            {
                /* beginning of section */
                if (ptr_line[0] == '[')
                {
                    pos = strchr (line, ']');
                    if (pos == NULL)
                    {
                        gui_printf (NULL,
                                    _("%s %s, line %d: invalid syntax, missing \"]\"\n"),
                                    WEECHAT_WARNING, filename, line_number);
                        fclose (file);
                        free (filename);
                        return -2;
                    }
                    pos[0] = '\0';
                    pos = ptr_line + 1;
                    section = CONFIG_SECTION_NONE;
                    for (i = 0; config_sections[i].section_name; i++)
                    {
                        if (strcmp (config_sections[i].section_name, pos) == 0)
                        {
                            section = i;
                            break;
                        }
                    }
                    if (section == CONFIG_SECTION_NONE)
                    {
                        gui_printf (NULL,
                                    _("%s %s, line %d: unknown section identifier (\"%s\")\n"),
                                    WEECHAT_WARNING, filename, line_number, pos);
                        fclose (file);
                        free (filename);
                        return -2;
                    }
                    if (server_found)
                    {
                        /* if server already started => create it */
                        if (!config_allocate_server (filename, line_number))
                        {
                            fclose (file);
                            free (filename);
                            return -2;
                        }
                    }
                    server_found = (section == CONFIG_SECTION_SERVER) ? 1 : 0;
                }
                else
                {
                    pos = strchr (line, '=');
                    if (pos == NULL)
                    {
                        gui_printf (NULL,
                                    _("%s %s, line %d: invalid syntax, missing \"=\"\n"),
                                    WEECHAT_WARNING, filename, line_number);
                        fclose (file);
                        free (filename);
                        return -2;
                    }
                    else
                    {
                        pos[0] = '\0';
                        pos++;
                        pos2 = strchr (pos, '\r');
                        if (pos2 != NULL)
                            pos2[0] = '\0';
                        pos2 = strchr (pos, '\n');
                        if (pos2 != NULL)
                            pos2[0] = '\0';
                        
                        if (section == CONFIG_SECTION_ALIAS)
                        {
                            if (alias_new (line, pos))
                                index_command_new (pos);
                        }
                        else
                        {
                            option_number = -1;
                            for (i = 0;
                                 weechat_options[section][i].option_name; i++)
                            {
                                if (strcmp
                                    (weechat_options[section][i].option_name,
                                     ptr_line) == 0)
                                {
                                    option_number = i;
                                    break;
                                }
                            }
                            if (option_number < 0)
                            {
                                gui_printf (NULL,
                                            _("%s %s, line %d: invalid option \"%s\"\n"),
                                            WEECHAT_WARNING, filename, line_number, ptr_line);
                                fclose (file);
                                free (filename);
                                return -2;
                            }
                            switch (weechat_options[section]
                                    [option_number].option_type)
                            {
                                case OPTION_TYPE_BOOLEAN:
                                    if (strcasecmp (pos, "on") == 0)
                                        *weechat_options[section]
                                            [option_number].ptr_int = BOOL_TRUE;
                                    else if (strcasecmp (pos, "off") == 0)
                                        *weechat_options[section]
                                            [option_number].ptr_int = BOOL_FALSE;
                                    else
                                    {
                                        gui_printf (NULL,
                                                    _("%s %s, line %d: invalid value for"
                                                    "option '%s'\n"
                                                    "Expected: boolean value: "
                                                    "'off' or 'on'\n"),
                                                    WEECHAT_WARNING, filename,
                                                    line_number, ptr_line);
                                        fclose (file);
                                        free (filename);
                                        return -2;
                                    }
                                    break;
                                case OPTION_TYPE_INT:
                                    int_value = atoi (pos);
                                    if ((int_value <
                                         weechat_options[section]
                                         [option_number].min)
                                        || (int_value >
                                            weechat_options[section]
                                            [option_number].max))
                                    {
                                        gui_printf (NULL,
                                                    _("%s %s, line %d: invalid value for"
                                                    "option '%s'\n"
                                                    "Expected: integer between %d "
                                                    "and %d\n"),
                                                    WEECHAT_WARNING, filename,
                                                    line_number, ptr_line,
                                                    weechat_options[section][option_number].min,
                                                    weechat_options[section][option_number].max);
                                        fclose (file);
                                        free (filename);
                                        return -2;
                                    }
                                    *weechat_options[section][option_number].ptr_int =
                                        int_value;
                                    break;
                                case OPTION_TYPE_INT_WITH_STRING:
                                    int_value = get_pos_array_values (
                                        weechat_options[section][option_number].array_values,
                                        pos);
                                    if (int_value < 0)
                                    {
                                        gui_printf (NULL,
                                                    _("%s %s, line %d: invalid value for"
                                                    "option '%s'\n"
                                                    "Expected: one of these strings: "),
                                                    WEECHAT_WARNING, filename,
                                                    line_number, ptr_line);
                                        i = 0;
                                        while (weechat_options[section][option_number].array_values[i])
                                        {
                                            gui_printf (NULL, "\"%s\" ",
                                                weechat_options[section][option_number].array_values[i]);
                                            i++;
                                        }
                                        gui_printf (NULL, "\n");
                                        fclose (file);
                                        free (filename);
                                        return -2;
                                    }
                                    *weechat_options[section][option_number].ptr_int =
                                        int_value;
                                    break;
                                case OPTION_TYPE_COLOR:
                                    if (!gui_assign_color (
                                        weechat_options[section][option_number].ptr_int,
                                        pos))
                                    {
                                        gui_printf (NULL,
                                                    _("%s %s, line %d: invalid color "
                                                    "name for option '%s'\n"),
                                                    WEECHAT_WARNING, filename,
                                                    line_number,
                                                    ptr_line);
                                        fclose (file);
                                        free (filename);
                                        return -2;
                                    }
                                    break;
                                case OPTION_TYPE_STRING:
                                    if (*weechat_options[section]
                                        [option_number].ptr_string)
                                        free (*weechat_options[section][option_number].ptr_string);
                                    *weechat_options[section][option_number].ptr_string =
                                        strdup (pos);
                                    break;
                            }
                        }
                    }
                }
            }
        }
    }
    
    if (server_found)
    {       
        if (!config_allocate_server (filename, line_number))
        {
            fclose (file);
            free (filename);
            return -2;
        }
    }
    
    /* set default colors for colors not set */
    /*for (i = 0; i < CONFIG_NUMBER_SECTIONS; i++)
    {
        if (i != CONFIG_SECTION_SERVER)
        {
            for (j = 0; weechat_options[i][j].option_name; j++)
            {
                if ((weechat_options[i][j].option_type == OPTION_TYPE_COLOR) &&
                    (*weechat_options[i][j].ptr_int == COLOR_NOT_SET))
                    *weechat_options[i][j].ptr_int =
                        gui_get_color_by_name (weechat_options[i][j].default_string);
            }
        }
    }*/
    
    fclose (file);
    free (filename);
    
    return 0;
}


/*
 * config_create_default: create default WeeChat config
 *                        return:  0 if ok
 *                               < 0 if error
 */

int
config_create_default ()
{
    char *filename;
    char line[1024];
    FILE *file;
    int i, j;
    time_t current_time;

    filename =
        (char *) malloc ((strlen (getenv ("HOME")) + 64) * sizeof (char));
    sprintf (filename, "%s/.weechat/" WEECHAT_CONFIG_NAME, getenv ("HOME"));
    if ((file = fopen (filename, "wt")) == NULL)
    {
        gui_printf (NULL, _("%s cannot create file \"%s\"\n"),
                    WEECHAT_ERROR, filename);
        free (filename);
        return -1;
    }
    
    printf (_(WEECHAT_NAME ": creating default config file...\n"));
    log_printf (_("creating default config file\n"));
    
    current_time = time (NULL);
    sprintf (line, _("#\n# " WEECHAT_NAME " configuration file, created by "
             WEECHAT_NAME " " WEECHAT_VERSION " on %s#\n"), ctime (&current_time));
    fputs (line, file);

    for (i = 0; i < CONFIG_NUMBER_SECTIONS; i++)
    {
        if ((i != CONFIG_SECTION_ALIAS) && (i != CONFIG_SECTION_SERVER))
        {
            sprintf (line, "\n[%s]\n", config_sections[i].section_name);
            fputs (line, file);
            if ((i == CONFIG_SECTION_HISTORY) || (i == CONFIG_SECTION_LOG) ||
                (i == CONFIG_SECTION_DCC) || (i == CONFIG_SECTION_PROXY))
            {
                sprintf (line,
                         "# WARNING!!! Options for section \"%s\" are not developed!\n",
                         config_sections[i].section_name);
                fputs (line, file);
            }
            for (j = 0; weechat_options[i][j].option_name; j++)
            {
                switch (weechat_options[i][j].option_type)
                {
                    case OPTION_TYPE_BOOLEAN:
                        sprintf (line, "%s=%s\n",
                                 weechat_options[i][j].option_name,
                                 (weechat_options[i][j].default_int) ?
                                 "on" : "off");
                        break;
                    case OPTION_TYPE_INT:
                        sprintf (line, "%s=%d\n",
                                 weechat_options[i][j].option_name,
                                 weechat_options[i][j].default_int);
                        break;
                    case OPTION_TYPE_INT_WITH_STRING:
                    case OPTION_TYPE_COLOR:
                    case OPTION_TYPE_STRING:
                        sprintf (line, "%s=%s\n",
                                 weechat_options[i][j].option_name,
                                 weechat_options[i][j].default_string);
                        break;
                }
                fputs (line, file);
            }
        }
    }
    
    /* default alias */
    fputs ("\n[alias]\n", file);
    fputs ("say=msg *\n", file);
    
    /* default server is freenode */
    fputs ("\n[server]\n", file);
    fputs ("server_name=freenode\n", file);
    fputs ("server_autoconnect=on\n", file);
    fputs ("server_address=irc.freenode.net\n", file);
    fputs ("server_port=6667\n", file);
    fputs ("server_password=\n", file);
    fputs ("server_nick1=weechat_user\n", file);
    fputs ("server_nick2=weechat2\n", file);
    fputs ("server_nick3=weechat3\n", file);
    fputs ("server_username=weechat\n", file);
    fputs ("server_realname=WeeChat default realname\n", file);
    
    fclose (file);
    free (filename);
    return 0;
}

/*
 * config_write: write WeeChat configurtion
 *               return:  0 if ok
 *                      < 0 if error
 */

int
config_write (char *config_name)
{
    char *filename;
    char line[1024];
    FILE *file;
    int i, j;
    time_t current_time;
    t_irc_server *ptr_server;
    t_weechat_alias *ptr_alias;

    if (config_name)
        filename = strdup (config_name);
    else
    {
        filename =
            (char *) malloc ((strlen (getenv ("HOME")) + 64) * sizeof (char));
        sprintf (filename, "%s/.weechat/" WEECHAT_CONFIG_NAME, getenv ("HOME"));
    }
    
    if ((file = fopen (filename, "wt")) == NULL)
    {
        gui_printf (NULL, _("%s cannot create file \"%s\"\n"),
                    WEECHAT_ERROR, filename);
        free (filename);
        return -1;
    }
    
    log_printf (_("saving config to disk\n"));
    
    current_time = time (NULL);
    sprintf (line, _("#\n# " WEECHAT_NAME " configuration file, created by "
             WEECHAT_NAME " " WEECHAT_VERSION " on %s#\n"), ctime (&current_time));
    fputs (line, file);

    for (i = 0; i < CONFIG_NUMBER_SECTIONS; i++)
    {
        if ((i != CONFIG_SECTION_ALIAS) && (i != CONFIG_SECTION_SERVER))
        {
            sprintf (line, "\n[%s]\n", config_sections[i].section_name);
            fputs (line, file);
            if ((i == CONFIG_SECTION_HISTORY) || (i == CONFIG_SECTION_LOG) ||
                (i == CONFIG_SECTION_DCC) || (i == CONFIG_SECTION_PROXY))
            {
                sprintf (line,
                         "# WARNING!!! Options for section \"%s\" are not developed!\n",
                         config_sections[i].section_name);
                fputs (line, file);
            }
            for (j = 0; weechat_options[i][j].option_name; j++)
            {
                switch (weechat_options[i][j].option_type)
                {
                    case OPTION_TYPE_BOOLEAN:
                        sprintf (line, "%s=%s\n",
                                 weechat_options[i][j].option_name,
                                 (weechat_options[i][j].ptr_int &&
                                 *weechat_options[i][j].ptr_int) ? 
                                 "on" : "off");
                        break;
                    case OPTION_TYPE_INT:
                        sprintf (line, "%s=%d\n",
                                 weechat_options[i][j].option_name,
                                 (weechat_options[i][j].ptr_int) ?
                                 *weechat_options[i][j].ptr_int :
                                 weechat_options[i][j].default_int);
                        break;
                    case OPTION_TYPE_INT_WITH_STRING:
                        sprintf (line, "%s=%s\n",
                                 weechat_options[i][j].option_name,
                                 (weechat_options[i][j].ptr_int) ?
                                 weechat_options[i][j].array_values[*weechat_options[i][j].ptr_int] :
                                 weechat_options[i][j].array_values[weechat_options[i][j].default_int]);
                        break;
                    case OPTION_TYPE_COLOR:
                        sprintf (line, "%s=%s\n",
                                 weechat_options[i][j].option_name,
                                 (weechat_options[i][j].ptr_int) ?
                                 gui_get_color_by_value (*weechat_options[i][j].ptr_int) :
                                 weechat_options[i][j].default_string);
                        break;
                    case OPTION_TYPE_STRING:
                        sprintf (line, "%s=%s\n",
                                 weechat_options[i][j].option_name,
                                 (weechat_options[i][j].ptr_string) ?
                                 *weechat_options[i][j].ptr_string :
                                 weechat_options[i][j].default_string);
                        break;
                }
                fputs (line, file);
            }
        }
    }
    
    /* alias section */
    fputs ("\n[alias]\n", file);
    for (ptr_alias = weechat_alias; ptr_alias;
         ptr_alias = ptr_alias->next_alias)
    {
        sprintf (line, "%s=%s\n",
                 ptr_alias->alias_name, ptr_alias->alias_command + 1);
        fputs (line, file);
    }
    
    /* server section */
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        /* default server is freenode */
        fputs ("\n[server]\n", file);
        sprintf (line, "server_name=%s\n", ptr_server->name);
        fputs (line, file);
        sprintf (line, "server_autoconnect=%s\n",
                 (ptr_server->autoconnect) ? "on" : "off");
        fputs (line, file);
        sprintf (line, "server_address=%s\n", ptr_server->address);
        fputs (line, file);
        sprintf (line, "server_port=%d\n", ptr_server->port);
        fputs (line, file);
        sprintf (line, "server_password=%s\n",
                 (ptr_server->password) ? ptr_server->password : "");
        fputs (line, file);
        sprintf (line, "server_nick1=%s\n", ptr_server->nick1);
        fputs (line, file);
        sprintf (line, "server_nick2=%s\n", ptr_server->nick2);
        fputs (line, file);
        sprintf (line, "server_nick3=%s\n", ptr_server->nick3);
        fputs (line, file);
        sprintf (line, "server_username=%s\n", ptr_server->username);
        fputs (line, file);
        sprintf (line, "server_realname=%s\n", ptr_server->realname);
        fputs (line, file);
    }
    
    fclose (file);
    free (filename);
    return 0;
}
