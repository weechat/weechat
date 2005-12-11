/*
 * Copyright (c) 2003-2005 by FlashCode <flashcode@flashtux.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef __WEECHAT_SESSION_H
#define __WEECHAT_SESSION_H 1

#define WEECHAT_SESSION_NAME "weechat_session.bin"

#define SESSION_SIGNATURE "== WeeChat Upgrade file v1.0 - binary, do not edit! =="

/* For developers: please add new values ONLY AT THE END of enums */

enum t_session_type
{
    SESSION_TYPE_INT = 0,
    SESSION_TYPE_STR,
    SESSION_TYPE_BUF
};

enum t_session_object
{
    SESSION_OBJ_END = 0,
    SESSION_OBJ_SERVER,
    SESSION_OBJ_CHANNEL,
    SESSION_OBJ_NICK,
    SESSION_OBJ_DCC,
    SESSION_OBJ_HISTORY,
    SESSION_OBJ_BUFFER,
    SESSION_OBJ_LINE,
};

enum t_session_server
{
    SESSION_SERV_END = 0,
    SESSION_SERV_NAME,
    SESSION_SERV_AUTOCONNECT,
    SESSION_SERV_AUTORECONNECT,
    SESSION_SERV_AUTORECONNECT_DELAY,
    SESSION_SERV_COMMAND_LINE,
    SESSION_SERV_ADDRESS,
    SESSION_SERV_PORT,
    SESSION_SERV_IPV6,
    SESSION_SERV_SSL,
    SESSION_SERV_PASSWORD,
    SESSION_SERV_NICK1,
    SESSION_SERV_NICK2,
    SESSION_SERV_NICK3,
    SESSION_SERV_USERNAME,
    SESSION_SERV_REALNAME,
    SESSION_SERV_COMMAND,
    SESSION_SERV_COMMAND_DELAY,
    SESSION_SERV_AUTOJOIN,
    SESSION_SERV_AUTOREJOIN,
    SESSION_SERV_NOTIFY_LEVELS,
    SESSION_SERV_CHILD_PID,
    SESSION_SERV_CHILD_READ,
    SESSION_SERV_CHILD_WRITE,
    SESSION_SERV_SOCK,
    SESSION_SERV_IS_CONNECTED,
    SESSION_SERV_SSL_CONNECTED,
    SESSION_SERV_GNUTLS_SESS,
    SESSION_SERV_UNTERMINATED_MESSAGE,
    SESSION_SERV_NICK,
    SESSION_SERV_RECONNECT_START,
    SESSION_SERV_RECONNECT_JOIN,
    SESSION_SERV_IS_AWAY,
    SESSION_SERV_AWAY_TIME,
    SESSION_SERV_LAG,
    SESSION_SERV_LAG_CHECK_TIME,
    SESSION_SERV_LAG_NEXT_CHECK,
    SESSION_SERV_CHARSET_DECODE_ISO,
    SESSION_SERV_CHARSET_DECODE_UTF,
    SESSION_SERV_CHARSET_ENCODE,
};

enum t_session_channel
{
    SESSION_CHAN_END = 0,
    SESSION_CHAN_TYPE,
    SESSION_CHAN_NAME,
    SESSION_CHAN_TOPIC,
    SESSION_CHAN_MODES,
    SESSION_CHAN_LIMIT,
    SESSION_CHAN_KEY,
    SESSION_CHAN_NICKS_COUNT,
    SESSION_CHAN_CHECKING_AWAY
};

enum t_session_nick
{
    SESSION_NICK_END = 0,
    SESSION_NICK_NICK,
    SESSION_NICK_FLAGS,
    SESSION_NICK_COLOR
};

enum t_session_dcc
{
    SESSION_DCC_END = 0,
    SESSION_DCC_SERVER,
    SESSION_DCC_CHANNEL,
    SESSION_DCC_TYPE,
    SESSION_DCC_STATUS,
    SESSION_DCC_START_TIME,
    SESSION_DCC_START_TRANSFER,
    SESSION_DCC_ADDR,
    SESSION_DCC_PORT,
    SESSION_DCC_NICK,
    SESSION_DCC_SOCK,
    SESSION_DCC_UNTERMINATED_MESSAGE,
    SESSION_DCC_FILE,
    SESSION_DCC_FILENAME,
    SESSION_DCC_LOCAL_FILENAME,
    SESSION_DCC_FILENAME_SUFFIX,
    SESSION_DCC_SIZE,
    SESSION_DCC_POS,
    SESSION_DCC_ACK,
    SESSION_DCC_START_RESUME,
    SESSION_DCC_LAST_CHECK_TIME,
    SESSION_DCC_LAST_CHECK_POS,
    SESSION_DCC_LAST_ACTIVITY,
    SESSION_DCC_BYTES_PER_SEC,
    SESSION_DCC_ETA
};

enum t_session_history
{
    SESSION_HIST_END = 0,
    SESSION_HIST_TEXT
};

enum t_session_buffer
{
    SESSION_BUFF_END = 0,
    SESSION_BUFF_SERVER,
    SESSION_BUFF_CHANNEL,
    SESSION_BUFF_DCC
};

enum t_session_line
{
    SESSION_LINE_END = 0,
    SESSION_LINE_LENGTH,
    SESSION_LINE_LENGTH_ALIGN,
    SESSION_LINE_LOG_WRITE,
    SESSION_LINE_WITH_MESSAGE,
    SESSION_LINE_WITH_HIGHLIGHT,
    SESSION_LINE_DATA,
    SESSION_LINE_OFS_AFTER_DATE
};

int session_save (char *filename);
int session_load (char *filename);

#endif /* session.h */
