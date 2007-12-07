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

/* wee-upgrade.c: save/restore session data */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/*#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif*/

#include "weechat.h"
#include "wee-upgrade.h"
#include "wee-log.h"
#include "wee-string.h"
#include "wee-utf8.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-main.h"


/* current server/channel (used when loading session) */
/*t_irc_server *session_current_server = NULL;
t_irc_channel *session_current_channel = NULL;
t_gui_buffer *session_current_buffer = NULL;*/

long session_last_read_pos = 0;
int session_last_read_length = 0;


/*
 * session_write_id: write object ID to file
 */

int
session_write_id (FILE *file, int id)
{
    return (fwrite ((void *)(&id), sizeof (int), 1, file) > 0);
}

/*
 * session_write_int: write an integer to file
 */

int
session_write_int (FILE *file, int id, int value)
{
    char type;
    
    if (id >= 0)
    {
        if (!session_write_id (file, id))
            return 0;
    }
    type = SESSION_TYPE_INT;
    if (fwrite ((void *)(&type), sizeof (char), 1, file) == 0)
        return 0;
    return (fwrite ((void *)(&value), sizeof (int), 1, file) > 0);
}

/*
 * session_write_str: write a string to file
 */

int
session_write_str (FILE *file, int id, char *string)
{
    char type;
    int length;
    
    if (id >= 0)
    {
        if (!session_write_id (file, id))
            return 0;
    }
    type = SESSION_TYPE_STR;
    if (fwrite ((void *)(&type), sizeof (char), 1, file) == 0)
        return 0;
    if (string && string[0])
    {
        length = strlen (string);
        if (fwrite ((void *)(&length), sizeof (int), 1, file) == 0)
            return 0;
        return (fwrite ((void *)string, length, 1, file) > 0);
    }
    else
    {
        length = 0;
        return (fwrite ((void *)(&length), sizeof (int), 1, file) > 0);
    }
}

/*
 * session_write_buf: write a buffer to file
 */

int
session_write_buf (FILE *file, int id, void *buffer, int size)
{
    char type;
    
    if (id >= 0)
    {
        if (!session_write_id (file, id))
            return 0;
    }
    type = SESSION_TYPE_BUF;
    if (fwrite ((void *)(&type), sizeof (char), 1, file) == 0)
        return 0;
    if (fwrite ((void *)(&size), sizeof (int), 1, file) == 0)
        return 0;
    return (fwrite (buffer, size, 1, file) > 0);
}

/*
 * session_save_nick: save a nick into session file
 */

/*int
session_save_nick (FILE *file, t_irc_nick *nick)
{
    int rc;
    
    rc = 1;
    rc = rc && (session_write_id  (file, SESSION_OBJ_NICK));
    rc = rc && (session_write_str (file, SESSION_NICK_NICK, nick->nick));
    rc = rc && (session_write_int (file, SESSION_NICK_FLAGS, nick->flags));
    rc = rc && (session_write_int (file, SESSION_NICK_COLOR, nick->color));
    rc = rc && (session_write_str (file, SESSION_NICK_HOST, nick->host));
    rc = rc && (session_write_id  (file, SESSION_NICK_END));
    return rc;
}*/

/*
 * session_save_channel: save a channel into session file
 */

/*int
session_save_channel (FILE *file, t_irc_channel *channel)
{
    int rc;
    t_irc_nick *ptr_nick;
    
    rc = 1;
    rc = rc && (session_write_id  (file, SESSION_OBJ_CHANNEL));
    rc = rc && (session_write_int (file, SESSION_CHAN_TYPE, channel->type));
    rc = rc && (session_write_str (file, SESSION_CHAN_NAME, channel->name));
    rc = rc && (session_write_str (file, SESSION_CHAN_TOPIC, channel->topic));
    rc = rc && (session_write_str (file, SESSION_CHAN_MODES, channel->modes));
    rc = rc && (session_write_int (file, SESSION_CHAN_LIMIT, channel->limit));
    rc = rc && (session_write_str (file, SESSION_CHAN_KEY, channel->key));
    rc = rc && (session_write_int (file, SESSION_CHAN_NICKS_COUNT, channel->nicks_count));
    rc = rc && (session_write_int (file, SESSION_CHAN_CHECKING_AWAY, channel->checking_away));
    rc = rc && (session_write_str (file, SESSION_CHAN_AWAY_MESSAGE, channel->away_message));
    rc = rc && (session_write_int (file, SESSION_CHAN_CYCLE, channel->cycle));
    rc = rc && (session_write_int (file, SESSION_CHAN_CLOSE, channel->close));
    rc = rc && (session_write_int (file, SESSION_CHAN_DISPLAY_CREATION_DATE, channel->display_creation_date));
    rc = rc && (session_write_id  (file, SESSION_CHAN_END));
    
    if (!rc)
        return 0;
    
    for (ptr_nick = channel->nicks; ptr_nick;
         ptr_nick = ptr_nick->next_nick)
    {
        if (!session_save_nick (file, ptr_nick))
            return 0;
    }
    
    return 1;
}*/

/*
 * session_save_servers: save all servers into session file
 */

/*int
session_save_servers (FILE *file)
{
    int rc;
#ifdef HAVE_GNUTLS
    void *session_data;
    size_t session_size;
#endif
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    
    rc = 1;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        rc = rc && (session_write_id  (file, SESSION_OBJ_SERVER));
        rc = rc && (session_write_str (file, SESSION_SERV_NAME, ptr_server->name));
        rc = rc && (session_write_int (file, SESSION_SERV_AUTOCONNECT, ptr_server->autoconnect));
        rc = rc && (session_write_int (file, SESSION_SERV_AUTORECONNECT, ptr_server->autoreconnect));
        rc = rc && (session_write_int (file, SESSION_SERV_AUTORECONNECT_DELAY, ptr_server->autoreconnect_delay));
        rc = rc && (session_write_int (file, SESSION_SERV_TEMP_SERVER, ptr_server->temp_server));
        rc = rc && (session_write_str (file, SESSION_SERV_ADDRESS, ptr_server->address));
        rc = rc && (session_write_int (file, SESSION_SERV_PORT, ptr_server->port));
        rc = rc && (session_write_int (file, SESSION_SERV_IPV6, ptr_server->ipv6));
        rc = rc && (session_write_int (file, SESSION_SERV_SSL, ptr_server->ssl));
        rc = rc && (session_write_str (file, SESSION_SERV_PASSWORD, ptr_server->password));
        rc = rc && (session_write_str (file, SESSION_SERV_NICK1, ptr_server->nick1));
        rc = rc && (session_write_str (file, SESSION_SERV_NICK2, ptr_server->nick2));
        rc = rc && (session_write_str (file, SESSION_SERV_NICK3, ptr_server->nick3));
        rc = rc && (session_write_str (file, SESSION_SERV_USERNAME, ptr_server->username));
        rc = rc && (session_write_str (file, SESSION_SERV_REALNAME, ptr_server->realname));
        rc = rc && (session_write_str (file, SESSION_SERV_HOSTNAME, ptr_server->hostname));
        rc = rc && (session_write_str (file, SESSION_SERV_COMMAND, ptr_server->command));
        rc = rc && (session_write_int (file, SESSION_SERV_COMMAND_DELAY, ptr_server->command_delay));
        rc = rc && (session_write_str (file, SESSION_SERV_AUTOJOIN, ptr_server->autojoin));
        rc = rc && (session_write_int (file, SESSION_SERV_AUTOREJOIN, ptr_server->autorejoin));
        rc = rc && (session_write_str (file, SESSION_SERV_NOTIFY_LEVELS, ptr_server->notify_levels));
        rc = rc && (session_write_int (file, SESSION_SERV_CHILD_PID, ptr_server->child_pid));
        rc = rc && (session_write_int (file, SESSION_SERV_CHILD_READ, ptr_server->child_read));
        rc = rc && (session_write_int (file, SESSION_SERV_CHILD_WRITE, ptr_server->child_write));
        rc = rc && (session_write_int (file, SESSION_SERV_SOCK, ptr_server->sock));
        rc = rc && (session_write_int (file, SESSION_SERV_IS_CONNECTED, ptr_server->is_connected));
        rc = rc && (session_write_int (file, SESSION_SERV_SSL_CONNECTED, ptr_server->ssl_connected));
#ifdef HAVE_GNUTLS
        if (ptr_server->is_connected && ptr_server->ssl_connected)
        {
            gnutls_session_get_data (ptr_server->gnutls_sess, NULL, &session_size);
            if (session_size > 0)
            {
                session_data = malloc (session_size);
                gnutls_session_get_data (ptr_server->gnutls_sess, session_data, &session_size);
                rc = rc && (session_write_buf (file, SESSION_SERV_GNUTLS_SESS, &(session_data), session_size));
                free (session_data);
            }
        }
#endif
        rc = rc && (session_write_str (file, SESSION_SERV_UNTERMINATED_MESSAGE, ptr_server->unterminated_message));
        rc = rc && (session_write_str (file, SESSION_SERV_NICK, ptr_server->nick));
        rc = rc && (session_write_str (file, SESSION_SERV_NICK_MODES, ptr_server->nick_modes));
        rc = rc && (session_write_str (file, SESSION_SERV_PREFIX, ptr_server->prefix));
        rc = rc && (session_write_buf (file, SESSION_SERV_RECONNECT_START, &(ptr_server->reconnect_start), sizeof (time_t)));
        rc = rc && (session_write_buf (file, SESSION_SERV_COMMAND_TIME, &(ptr_server->command_time), sizeof (time_t)));
        rc = rc && (session_write_int (file, SESSION_SERV_RECONNECT_JOIN, ptr_server->reconnect_join));
        rc = rc && (session_write_int (file, SESSION_SERV_IS_AWAY, ptr_server->is_away));
        rc = rc && (session_write_str (file, SESSION_SERV_AWAY_MESSAGE, ptr_server->away_message));
        rc = rc && (session_write_buf (file, SESSION_SERV_AWAY_TIME, &(ptr_server->away_time), sizeof (time_t)));
        rc = rc && (session_write_int (file, SESSION_SERV_LAG, ptr_server->lag));
        rc = rc && (session_write_buf (file, SESSION_SERV_LAG_CHECK_TIME, &(ptr_server->lag_check_time), sizeof (struct timeval)));
        rc = rc && (session_write_buf (file, SESSION_SERV_LAG_NEXT_CHECK, &(ptr_server->lag_next_check), sizeof (time_t)));
        rc = rc && (session_write_id  (file, SESSION_SERV_END));
        
        if (!rc)
            return 0;
        
        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if (!session_save_channel (file, ptr_channel))
                return 0;
        }
    }
    return 1;
}*/

/*
 * session_save_dcc: save all DCC into session file
 */

/*int
session_save_dcc (FILE *file)
{
    int rc;
    t_irc_dcc *ptr_dcc;
    
    rc = 1;
    
    for (ptr_dcc = irc_last_dcc; ptr_dcc;
         ptr_dcc = ptr_dcc->prev_dcc)
    {
        rc = rc && (session_write_id  (file, SESSION_OBJ_DCC));
        rc = rc && (session_write_str (file, SESSION_DCC_SERVER, (ptr_dcc->server) ? ptr_dcc->server->name : NULL));
        rc = rc && (session_write_str (file, SESSION_DCC_CHANNEL, (ptr_dcc->channel) ? ptr_dcc->channel->name : NULL));
        rc = rc && (session_write_int (file, SESSION_DCC_TYPE, ptr_dcc->type));
        rc = rc && (session_write_int (file, SESSION_DCC_STATUS, ptr_dcc->status));
        rc = rc && (session_write_buf (file, SESSION_DCC_START_TIME, &(ptr_dcc->start_time), sizeof (time_t)));
        rc = rc && (session_write_buf (file, SESSION_DCC_START_TRANSFER, &(ptr_dcc->start_transfer), sizeof (time_t)));
        rc = rc && (session_write_buf (file, SESSION_DCC_ADDR, &(ptr_dcc->addr), sizeof (unsigned long)));
        rc = rc && (session_write_int (file, SESSION_DCC_PORT, ptr_dcc->port));
        rc = rc && (session_write_str (file, SESSION_DCC_NICK, ptr_dcc->nick));
        rc = rc && (session_write_int (file, SESSION_DCC_SOCK, ptr_dcc->sock));
        rc = rc && (session_write_str (file, SESSION_DCC_UNTERMINATED_MESSAGE, ptr_dcc->unterminated_message));
        rc = rc && (session_write_int (file, SESSION_DCC_FILE, ptr_dcc->file));
        rc = rc && (session_write_str (file, SESSION_DCC_FILENAME, ptr_dcc->filename));
        rc = rc && (session_write_str (file, SESSION_DCC_LOCAL_FILENAME, ptr_dcc->local_filename));
        rc = rc && (session_write_int (file, SESSION_DCC_FILENAME_SUFFIX, ptr_dcc->filename_suffix));
        rc = rc && (session_write_buf (file, SESSION_DCC_SIZE, &(ptr_dcc->size), sizeof (unsigned long)));
        rc = rc && (session_write_buf (file, SESSION_DCC_POS, &(ptr_dcc->pos), sizeof (unsigned long)));
        rc = rc && (session_write_buf (file, SESSION_DCC_ACK, &(ptr_dcc->ack), sizeof (unsigned long)));
        rc = rc && (session_write_buf (file, SESSION_DCC_START_RESUME, &(ptr_dcc->start_resume), sizeof (unsigned long)));
        rc = rc && (session_write_buf (file, SESSION_DCC_LAST_CHECK_TIME, &(ptr_dcc->last_check_time), sizeof (time_t)));
        rc = rc && (session_write_buf (file, SESSION_DCC_LAST_CHECK_POS, &(ptr_dcc->last_check_pos), sizeof (unsigned long)));
        rc = rc && (session_write_buf (file, SESSION_DCC_LAST_ACTIVITY, &(ptr_dcc->last_activity), sizeof (time_t)));
        rc = rc && (session_write_buf (file, SESSION_DCC_BYTES_PER_SEC, &(ptr_dcc->bytes_per_sec), sizeof (unsigned long)));
        rc = rc && (session_write_buf (file, SESSION_DCC_ETA, &(ptr_dcc->eta), sizeof (unsigned long)));
        rc = rc && (session_write_int (file, SESSION_DCC_CHILD_PID, ptr_dcc->child_pid));
        rc = rc && (session_write_int (file, SESSION_DCC_CHILD_READ, ptr_dcc->child_read));
        rc = rc && (session_write_int (file, SESSION_DCC_CHILD_WRITE, ptr_dcc->child_write));
        rc = rc && (session_write_id  (file, SESSION_DCC_END));
        
        if (!rc)
            return 0;
    }
    return 1;
}*/

/*
 * session_save_history: save history into session file
 *                       (from last to first, to restore it in good order)
 */

/*int
session_save_history (FILE *file, t_history *last_history)
{
    int rc;
    t_history *ptr_history;
    
    rc = 1;
    rc = rc && (session_write_id (file, SESSION_OBJ_HISTORY));
    ptr_history = last_history;
    while (ptr_history)
    {
        rc = rc && (session_write_str (file, SESSION_HIST_TEXT, ptr_history->text));
        ptr_history = ptr_history->prev_history;
    }
    rc = rc && (session_write_id  (file, SESSION_HIST_END));
    return rc;
}*/

/*
 * session_save_line: save a buffer line into session file
 */

/*int
session_save_line (FILE *file, t_gui_line *line)
{
    int rc;
    
    rc = 1;
    rc = rc && (session_write_id  (file, SESSION_OBJ_LINE));
    rc = rc && (session_write_int (file, SESSION_LINE_LENGTH, line->length));
    rc = rc && (session_write_int (file, SESSION_LINE_LENGTH_ALIGN, line->length_align));
    rc = rc && (session_write_int (file, SESSION_LINE_LOG_WRITE, line->log_write));
    rc = rc && (session_write_int (file, SESSION_LINE_WITH_MESSAGE, line->line_with_message));
    rc = rc && (session_write_int (file, SESSION_LINE_WITH_HIGHLIGHT, line->line_with_highlight));
    rc = rc && (session_write_str (file, SESSION_LINE_DATA, line->data));
    rc = rc && (session_write_int (file, SESSION_LINE_OFS_AFTER_DATE, line->ofs_after_date));
    rc = rc && (session_write_int (file, SESSION_LINE_OFS_START_MESSAGE, line->ofs_start_message));
    rc = rc && (session_write_str (file, SESSION_LINE_NICK, line->nick));
    rc = rc && (session_write_buf (file, SESSION_LINE_DATE, &(line->date), sizeof (time_t)));
    rc = rc && (session_write_id  (file, SESSION_LINE_END));
    return rc;
}*/

/*
 * session_save_buffers: save all buffers into session file
 */

/*int
session_save_buffers (FILE *file)
{
    int rc;
    t_gui_buffer *ptr_buffer;
    t_gui_line *ptr_line;
    
    rc = 1;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        rc = rc && (session_write_id  (file, SESSION_OBJ_BUFFER));
        rc = rc && (session_write_str (file, SESSION_BUFF_SERVER, GUI_SERVER(ptr_buffer) ? GUI_SERVER(ptr_buffer)->name : NULL));
        rc = rc && (session_write_str (file, SESSION_BUFF_CHANNEL, GUI_CHANNEL(ptr_buffer) ? GUI_CHANNEL(ptr_buffer)->name : NULL));
        rc = rc && (session_write_int (file, SESSION_BUFF_TYPE, ptr_buffer->type));
        rc = rc && (session_write_int (file, SESSION_BUFF_ALL_SERVERS, ptr_buffer->all_servers));
        rc = rc && (session_write_id  (file, SESSION_BUFF_END));
        
        if (!rc)
            return 0;
        
        for (ptr_line = ptr_buffer->lines; ptr_line;
             ptr_line = ptr_line->next_line)
        {
            if (!session_save_line (file, ptr_line))
                return 0;
        }
        
        if (!session_save_history (file, ptr_buffer->last_history))
            return 0;
    }
    return 1;
}*/

/*
 * session_save_uptime: save uptime into session file
 */

/*int
session_save_uptime (FILE *file)
{
    int rc;
    
    rc = 1;
    
    rc = rc && (session_write_id  (file, SESSION_OBJ_UPTIME));
    rc = rc && (session_write_buf (file, SESSION_UPT_START_TIME, &weechat_start_time, sizeof (time_t)));
    rc = rc && (session_write_id  (file, SESSION_UPT_END));
    return rc;
}*/

/*
 * session_save_hotlist: save hotlist into session file
 */

/*int
session_save_hotlist (FILE *file)
{
    int rc;
    t_weechat_hotlist *ptr_hotlist;
    
    rc = 1;
    
    for (ptr_hotlist = weechat_hotlist; ptr_hotlist;
         ptr_hotlist = ptr_hotlist->next_hotlist)
    {
        rc = rc && (session_write_id  (file, SESSION_OBJ_HOTLIST));
        rc = rc && (session_write_int (file, SESSION_HOTL_PRIORITY, ptr_hotlist->priority));
        rc = rc && (session_write_str (file, SESSION_HOTL_SERVER, (ptr_hotlist->server) ? ptr_hotlist->server->name : NULL));
        rc = rc && (session_write_int (file, SESSION_HOTL_BUFFER_NUMBER, ptr_hotlist->buffer->number));
        rc = rc && (session_write_buf (file, SESSION_HOTL_CREATION_TIME, &(ptr_hotlist->creation_time), sizeof (struct timeval)));
        rc = rc && (session_write_id  (file, SESSION_HOTL_END));
        
        if (!rc)
            return 0;
    }
    return rc;
}*/

/*
 * session_save: save current session
 */

/*int
session_save (char *filename)
{
    FILE *file;
    int rc;
    
    if ((file = fopen (filename, "wb")) == NULL)
        return 0;
    
    rc = 1;
    rc = rc && (session_write_str (file, -1, SESSION_SIGNATURE));
    rc = rc && (session_save_servers (file));
    rc = rc && (session_save_dcc (file));
    rc = rc && (session_save_history (file, history_global_last));
    rc = rc && (session_save_buffers (file));
    rc = rc && (session_save_uptime (file));
    rc = rc && (session_save_hotlist (file));
    
    fclose (file);
    
    return rc;
}*/

/* ========================================================================== */

/*
 * session_crash: stop WeeChat if problem during session loading
 */

void
session_crash (FILE *file, char *message, ...)
{
    char buf[4096];
    va_list argptr;
    
    va_start (argptr, message);
    vsnprintf (buf, sizeof (buf) - 1, message, argptr);
    va_end (argptr);
    
    fclose (file);
    gui_main_end ();
    string_iconv_fprintf (stderr, "Error: %s\n", buf);
    string_iconv_fprintf (stderr,
                          _("Last operation with session file was at position %ld, "
                            "read of %d bytes\n"),
                          session_last_read_pos,
                          session_last_read_length);
    string_iconv_fprintf (stderr,
                          _("Please send %s/%s, %s/%s and "
                            "above messages to WeeChat developers for support.\n"
                            "Be careful, private info may be in these files.\n"),
                          weechat_home,
                          WEECHAT_LOG_NAME,
                          weechat_home,
                          WEECHAT_SESSION_NAME);
    exit (EXIT_FAILURE);
}

/*
 * session_read_int: read integer from file
 */

int
session_read_int (FILE *file, int *value)
{
    char type;
    
    session_last_read_pos = ftell (file);
    session_last_read_length = sizeof (char);
    
    if (fread ((void *)(&type), sizeof (char), 1, file) == 0)
        return 0;
    if (type != SESSION_TYPE_INT)
    {
        session_crash (file, _("wrong type in file (expected: %d, read: %d)"),
                       SESSION_TYPE_INT, type);
        return 0;
    }
    
    session_last_read_pos = ftell (file);
    session_last_read_length = sizeof (int);
    
    if (value)
        return (fread ((void *)value, sizeof (int), 1, file) > 0);
    else
        return (fseek (file, sizeof (int), SEEK_CUR) >= 0);
}

/*
 * session_read_str: read string from file
 */

int
session_read_str (FILE *file, char **string)
{
    char type;
    int length;
    
    if (string && *string)
        free (*string);
    
    session_last_read_pos = ftell (file);
    session_last_read_length = sizeof (char);
    
    if (fread ((void *)(&type), sizeof (char), 1, file) == 0)
        return 0;
    if (type != SESSION_TYPE_STR)
    {
        session_crash (file, _("wrong type in file (expected: %d, read: %d)"),
                       SESSION_TYPE_STR, type);
        return 0;
    }
    
    session_last_read_pos = ftell (file);
    session_last_read_length = sizeof (int);
    
    if (fread ((void *)(&length), sizeof (int), 1, file) == 0)
        return 0;
    
    if (length == 0)
    {
        if (string)
            (*string) = NULL;
        return 1;
    }
    
    session_last_read_pos = ftell (file);
    session_last_read_length = length;
    
    if (string)
    {
        (*string) = (char *) malloc (length + 1);
        if (!(*string))
            return 0;
        
        if (fread ((void *)(*string), length, 1, file) == 0)
        {
            free (*string);
            return 0;
        }
        (*string)[length] = '\0';
    }
    else
        return (fseek (file, length, SEEK_CUR) >= 0);
    
    return 1;
}

/*
 * session_read_str_utf8: read string from file, then normalize UTF-8
 */

int
session_read_str_utf8 (FILE *file, char **string)
{
    int rc;

    rc = session_read_str (file, string);
    if (rc && *string)
        utf8_normalize (*string, '?');
    
    return rc;
}

/*
 * session_read_buf: read buffer from file
 */

int
session_read_buf (FILE *file, void *buffer, int length_expected)
{
    char type;
    int length;
    
    session_last_read_pos = ftell (file);
    session_last_read_length = sizeof (char);
    
    if (fread ((void *)(&type), sizeof (char), 1, file) == 0)
        return 0;
    if (type != SESSION_TYPE_BUF)
    {
        session_crash (file, _("wrong type in file (expected: %d, read: %d)"),
                       SESSION_TYPE_BUF, type);
        return 0;
    }
    
    session_last_read_pos = ftell (file);
    session_last_read_length = sizeof (int);
    
    if (fread ((void *)(&length), sizeof (int), 1, file) == 0)
        return 0;
    if ((length <= 0) || ((length_expected > 0) && (length != length_expected)))
    {
        session_crash (file, _("invalid length for a buffer"));
        return 0;
    }
    
    session_last_read_pos = ftell (file);
    session_last_read_length = length;
    
    if (buffer)
        return (fread (buffer, length, 1, file) > 0);
    else
        return (fseek (file, length, SEEK_CUR) >= 0);
}

/*
 * session_read_buf_alloc: read buffer from file and allocate it in memory
 */

int
session_read_buf_alloc (FILE *file, void **buffer, int *buffer_length)
{
    char type;
    
    session_last_read_pos = ftell (file);
    session_last_read_length = sizeof (char);
    
    if (fread ((void *)(&type), sizeof (char), 1, file) == 0)
        return 0;
    if (type != SESSION_TYPE_BUF)
    {
        session_crash (file, _("wrong type in file (expected: %d, read: %d)"),
                       SESSION_TYPE_BUF, type);
        return 0;
    }
    
    session_last_read_pos = ftell (file);
    session_last_read_length = sizeof (int);
    
    if (fread ((void *)(buffer_length), sizeof (int), 1, file) == 0)
        return 0;
    if (*buffer_length <= 0)
    {
        session_crash (file, _("invalid length for a buffer"));
        return 0;
    }
    
    *buffer = malloc (*buffer_length);
    
    session_last_read_pos = ftell (file);
    session_last_read_length = *buffer_length;
    
    return (fread (*buffer, *buffer_length, 1, file) > 0);
}

/*
 * session_read_object: read an object in file
 */

int
session_read_object (FILE *file, int object_id, int type, void *target, int max_buf_length)
{
    int object_id_read;
    char type_read;
    
    if (fread ((void *)(&object_id_read), sizeof (int), 1, file) == 0)
    {
        session_crash (file, _("object read error"));
        return 0;
    }
    if (object_id_read != object_id)
    {
        session_crash (file, _("wrong object (expected: %d, read: %d)"),
                       object_id, object_id_read);
        return 0;
    }
    
    session_last_read_pos = ftell (file);
    session_last_read_length = sizeof (char);
    
    if (fread ((void *)(&type_read), sizeof (char), 1, file) == 0)
    {
        session_crash (file, _("type read error"));
        return 0;
    }
    if (type_read != type)
    {
        session_crash (file, _("wrong type (expected: %d, read: %d)"),
                       type, type_read);
        return 0;
    }
    if (fseek (file, sizeof (char) * (-1), SEEK_CUR) < 0)
        return 0;
    switch (type)
    {
        case SESSION_TYPE_INT:
            return session_read_int (file, (int *)target);
        case SESSION_TYPE_STR:
            return session_read_str (file, (char **)target);
        case SESSION_TYPE_BUF:
            return session_read_buf (file, target, max_buf_length);
    }
    return 0;
}

/*
 * session_read_ignore_value: ignore a value from file
 */

int
session_read_ignore_value (FILE *file)
{
    char type;
    
    if (fread ((void *)(&type), sizeof (char), 1, file) == 0)
        return 0;
    if (fseek (file, sizeof (char) * (-1), SEEK_CUR) < 0)
        return 0;
    switch (type)
    {
        case SESSION_TYPE_INT:
            return session_read_int (file, NULL);
        case SESSION_TYPE_STR:
            return session_read_str (file, NULL);
        case SESSION_TYPE_BUF:
            return session_read_buf (file, NULL, 0);
    }
    return 0;
}

/*
 * session_read_ignore_object: ignore an object from file
 */

int
session_read_ignore_object (FILE *file)
{
    int object_id;
    
    while (1)
    {
        if (fread ((void *)(&object_id), sizeof (int), 1, file) == 0)
            return 0;
        if (feof (file))
            return 0;
        if (object_id == SESSION_OBJ_END)
            return 1;
        if (!session_read_ignore_value (file))
            return 0;
    }
}

/*
 * session_load_server: load server from file
 */

/*int
session_load_server (FILE *file)
{
    int object_id, rc;
    char *server_name;
#ifdef HAVE_GNUTLS
    void *session_data;
    size_t session_size;
    int session_size_int;
#endif
    
    // read server name
    server_name = NULL;
    if (!session_read_object (file, SESSION_SERV_NAME, SESSION_TYPE_STR, &server_name, 0))
    {
        session_crash (file, _("server name not found"));
        return 0;
    }
    
    // use or allocate server
    log_printf (_("session: loading server \"%s\""),
                server_name);
    session_current_server = irc_server_search (server_name);
    if (session_current_server)
        log_printf (_("server found, updating values"));
    else
    {
        log_printf (_("server not found, creating new one"));
        session_current_server = irc_server_alloc ();
        if (!session_current_server)
        {
            free (server_name);
            session_crash (file, _("can't create new server"));
            return 0;
        }
        irc_server_init (session_current_server);
        session_current_server->name = strdup (server_name);
    }
    free (server_name);
    
    // read server values
    rc = 1;
    while (rc)
    {
        if (feof (file))
        {
            session_crash (file, _("unexpected end of file (reading server)"));
            return 0;
        }
        if (fread ((void *)(&object_id), sizeof (int), 1, file) == 0)
            return 0;
        switch (object_id)
        {
            case SESSION_SERV_END:
                return 1;
            case SESSION_SERV_AUTOCONNECT:
                rc = rc && (session_read_int (file, &(session_current_server->autoconnect)));
                break;
            case SESSION_SERV_AUTORECONNECT:
                rc = rc && (session_read_int (file, &(session_current_server->autoreconnect)));
                break;
            case SESSION_SERV_AUTORECONNECT_DELAY:
                rc = rc && (session_read_int (file, &(session_current_server->autoreconnect_delay)));
                break;
            case SESSION_SERV_TEMP_SERVER:
                rc = rc && (session_read_int (file, &(session_current_server->temp_server)));
                break;
            case SESSION_SERV_ADDRESS:
                rc = rc && (session_read_str (file, &(session_current_server->address)));
                break;
            case SESSION_SERV_PORT:
                rc = rc && (session_read_int (file, &(session_current_server->port)));
                break;
            case SESSION_SERV_IPV6:
                rc = rc && (session_read_int (file, &(session_current_server->ipv6)));
                break;
            case SESSION_SERV_SSL:
                rc = rc && (session_read_int (file, &(session_current_server->ssl)));
                break;
            case SESSION_SERV_PASSWORD:
                rc = rc && (session_read_str (file, &(session_current_server->password)));
                break;
            case SESSION_SERV_NICK1:
                rc = rc && (session_read_str (file, &(session_current_server->nick1)));
                break;
            case SESSION_SERV_NICK2:
                rc = rc && (session_read_str (file, &(session_current_server->nick2)));
                break;
            case SESSION_SERV_NICK3:
                rc = rc && (session_read_str (file, &(session_current_server->nick3)));
                break;
            case SESSION_SERV_USERNAME:
                rc = rc && (session_read_str (file, &(session_current_server->username)));
                break;
            case SESSION_SERV_REALNAME:
                rc = rc && (session_read_str (file, &(session_current_server->realname)));
                break;
            case SESSION_SERV_HOSTNAME:
                rc = rc && (session_read_str (file, &(session_current_server->hostname)));
                break;
            case SESSION_SERV_COMMAND:
                rc = rc && (session_read_str (file, &(session_current_server->command)));
                break;
            case SESSION_SERV_COMMAND_DELAY:
                rc = rc && (session_read_int (file, &(session_current_server->command_delay)));
                break;
            case SESSION_SERV_AUTOJOIN:
                rc = rc && (session_read_str (file, &(session_current_server->autojoin)));
                break;
            case SESSION_SERV_AUTOREJOIN:
                rc = rc && (session_read_int (file, &(session_current_server->autorejoin)));
                break;
            case SESSION_SERV_NOTIFY_LEVELS:
                rc = rc && (session_read_str (file, &(session_current_server->notify_levels)));
                break;
            case SESSION_SERV_CHILD_PID:
                rc = rc && (session_read_int (file, &(session_current_server->child_pid)));
                break;
            case SESSION_SERV_CHILD_READ:
                rc = rc && (session_read_int (file, &(session_current_server->child_read)));
                break;
            case SESSION_SERV_CHILD_WRITE:
                rc = rc && (session_read_int (file, &(session_current_server->child_write)));
                break;
            case SESSION_SERV_SOCK:
                rc = rc && (session_read_int (file, &(session_current_server->sock)));
                break;
            case SESSION_SERV_IS_CONNECTED:
                rc = rc && (session_read_int (file, &(session_current_server->is_connected)));
                break;
            case SESSION_SERV_SSL_CONNECTED:
                rc = rc && (session_read_int (file, &(session_current_server->ssl_connected)));
                break;
#ifdef HAVE_GNUTLS
            case SESSION_SERV_GNUTLS_SESS:
                if (gnutls_init (&(session_current_server->gnutls_sess), GNUTLS_CLIENT) != 0)
                {
                    session_crash (file, _("gnutls init error"));
                    return 0;
                }
                gnutls_set_default_priority (session_current_server->gnutls_sess);
                gnutls_certificate_type_set_priority (session_current_server->gnutls_sess, gnutls_cert_type_prio);
                gnutls_protocol_set_priority (session_current_server->gnutls_sess, gnutls_prot_prio);
                gnutls_credentials_set (session_current_server->gnutls_sess, GNUTLS_CRD_CERTIFICATE, gnutls_xcred);
                session_data = NULL;
                rc = rc && (session_read_buf_alloc (file, &session_data, &session_size_int));
                if (rc)
                {
                    session_size = session_size_int;
                    gnutls_session_set_data (session_current_server->gnutls_sess, session_data, session_size);
                    free (session_data);
                    gnutls_transport_set_ptr (session_current_server->gnutls_sess,
                                              (gnutls_transport_ptr) ((unsigned long) session_current_server->sock));
                    if (gnutls_handshake (session_current_server->gnutls_sess) < 0)
                    {
                        session_crash (file, _("gnutls handshake failed"));
                        return 0;
                    }
                }
                break;
#endif
            case SESSION_SERV_UNTERMINATED_MESSAGE:
                rc = rc && (session_read_str (file, &(session_current_server->unterminated_message)));
                break;
            case SESSION_SERV_NICK:
                rc = rc && (session_read_str (file, &(session_current_server->nick)));
                break;
            case SESSION_SERV_NICK_MODES:
                rc = rc && (session_read_str (file, &(session_current_server->nick_modes)));
                break;
            case SESSION_SERV_PREFIX:
                rc = rc && (session_read_str (file, &(session_current_server->prefix)));
                break;
            case SESSION_SERV_RECONNECT_START:
                rc = rc && (session_read_buf (file, &(session_current_server->reconnect_start), sizeof (time_t)));
                break;
            case SESSION_SERV_COMMAND_TIME:
                rc = rc && (session_read_buf (file, &(session_current_server->command_time), sizeof (time_t)));
                break;
            case SESSION_SERV_RECONNECT_JOIN:
                rc = rc && (session_read_int (file, &(session_current_server->reconnect_join)));
                break;
            case SESSION_SERV_IS_AWAY:
                rc = rc && (session_read_int (file, &(session_current_server->is_away)));
                break;
            case SESSION_SERV_AWAY_MESSAGE:
                 rc = rc && (session_read_str (file, &(session_current_server->away_message)));
                break;
            case SESSION_SERV_AWAY_TIME:
                rc = rc && (session_read_buf (file, &(session_current_server->away_time), sizeof (time_t)));
                break;
            case SESSION_SERV_LAG:
                rc = rc && (session_read_int (file, &(session_current_server->lag)));
                break;
            case SESSION_SERV_LAG_CHECK_TIME:
                rc = rc && (session_read_buf (file, &(session_current_server->lag_check_time), sizeof (struct timeval)));
                break;
            case SESSION_SERV_LAG_NEXT_CHECK:
                rc = rc && (session_read_buf (file, &(session_current_server->lag_next_check), sizeof (time_t)));
                break;
            case SESSION_SERV_CHARSET_DECODE_ISO__UNUSED:
                rc = rc && (session_read_ignore_value (file));
                break;
            case SESSION_SERV_CHARSET_DECODE_UTF__UNUSED:
                rc = rc && (session_read_ignore_value (file));
                break;
            case SESSION_SERV_CHARSET_ENCODE__UNUSED:
                rc = rc && (session_read_ignore_value (file));
                break;
            default:
                log_printf (_("session: warning: ignoring value from "
                            "server (object id: %d)"),
                            object_id);
                rc = rc && (session_read_ignore_value (file));
                break;
        }
    }
    return 0;
}*/

/*
 * session_load_channel: load channel from file
 */

/*int
session_load_channel (FILE *file)
{
    int object_id, rc, channel_type;
    char *channel_name;
    
    // check if server is allocated for this channel
    if (!session_current_server)
    {
        session_crash (file, _("channel found without server"));
        return 0;
    }
    
    // read channel type
    if (!session_read_object (file, SESSION_CHAN_TYPE, SESSION_TYPE_INT, &channel_type, 0))
    {
        session_crash (file, _("channel type not found"));
        return 0;
    }
    
    // read channel name
    channel_name = NULL;
    if (!session_read_object (file, SESSION_CHAN_NAME, SESSION_TYPE_STR, &channel_name, 0))
    {
        session_crash (file, _("channel name not found"));
        return 0;
    }
    
    // allocate channel
    log_printf (_("session: loading channel \"%s\""),
                channel_name);
    session_current_channel = irc_channel_new (session_current_server,
                                               channel_type,
                                               channel_name);
    free (channel_name);
    if (!session_current_channel)
    {
        session_crash (file, _("can't create new channel"));
        return 0;
    }
    
    // read channel values
    rc = 1;
    while (rc)
    {
        if (feof (file))
        {
            session_crash (file, _("unexpected end of file (reading channel)"));
            return 0;
        }
        if (fread ((void *)(&object_id), sizeof (int), 1, file) == 0)
            return 0;
        switch (object_id)
        {
            case SESSION_CHAN_END:
                return 1;
            case SESSION_CHAN_TOPIC:
                rc = rc && (session_read_str_utf8 (file, &(session_current_channel->topic)));
                break;
            case SESSION_CHAN_MODES:
                rc = rc && (session_read_str (file, (char **)(&(session_current_channel->modes))));
                break;
            case SESSION_CHAN_LIMIT:
                rc = rc && (session_read_int (file, &(session_current_channel->limit)));
                break;
            case SESSION_CHAN_KEY:
                rc = rc && (session_read_str (file, &(session_current_channel->key)));
                break;
            case SESSION_CHAN_NICKS_COUNT:
                rc = rc && (session_read_int (file, &(session_current_channel->nicks_count)));
                // will be incremented when adding nicks
                session_current_channel->nicks_count = 0;
                break;
            case SESSION_CHAN_CHECKING_AWAY:
                rc = rc && (session_read_int (file, &(session_current_channel->checking_away)));
                break;
            case SESSION_CHAN_AWAY_MESSAGE:
                rc = rc && (session_read_str (file, &(session_current_channel->away_message)));
                break;
            case SESSION_CHAN_CYCLE:
                rc = rc && (session_read_int (file, &(session_current_channel->cycle)));
                break;
            case SESSION_CHAN_CLOSE:
                rc = rc && (session_read_int (file, &(session_current_channel->close)));
                break;
            case SESSION_CHAN_DISPLAY_CREATION_DATE:
                rc = rc && (session_read_int (file, &(session_current_channel->display_creation_date)));
                break;
            default:
                log_printf (_("session: warning: ignoring value from "
                            "channel (object id: %d)"),
                            object_id);
                rc = rc && (session_read_ignore_value (file));
                break;
        }
    }
    return 0;
}*/

/*
 * session_load_nick: load nick from file
 */

/*int
session_load_nick (FILE *file)
{
    int rc, object_id;
    char *nick_name;
    t_irc_nick *nick;
    
    // check if channel is allocated for this nick
    if (!session_current_channel)
    {
        session_crash (file, _("nick found without channel"));
        return 0;
    }
    
    // read nick name
    nick_name = NULL;
    if (!session_read_object (file, SESSION_NICK_NICK, SESSION_TYPE_STR, &nick_name, 0))
    {
        session_crash (file, _("nick name not found"));
        return 0;
    }
    
    // allocate nick
    nick = irc_nick_new (session_current_server, session_current_channel,
                         nick_name, 0, 0, 0, 0, 0, 0, 0);
    free (nick_name);
    if (!nick)
    {
        session_crash (file, _("can't create new nick"));
        return 0;
    }
    
    // read nick values
    rc = 1;
    while (rc)
    {
        if (feof (file))
        {
            session_crash (file, _("unexpected end of file (reading nick)"));
            return 0;
        }
        if (fread ((void *)(&object_id), sizeof (int), 1, file) == 0)
            return 0;
        switch (object_id)
        {
            case SESSION_NICK_END:
                return 1;
            case SESSION_NICK_FLAGS:
                rc = rc && (session_read_int (file, &(nick->flags)));
                break;
            case SESSION_NICK_COLOR:
                rc = rc && (session_read_int (file, &(nick->color)));
                break;
            case SESSION_NICK_HOST:
                rc = rc && (session_read_str (file, &(nick->host)));
                break;
            default:
                log_printf (_("session: warning: ignoring value from "
                            "nick (object id: %d)"),
                            object_id);
                rc = rc && (session_read_ignore_value (file));
                break;
        }
    }
    return 0;
}*/

/*
 * session_load_dcc: load DCC from file
 */

/*int
session_load_dcc (FILE *file)
{
    int object_id, rc;
    t_irc_dcc *dcc;
    char *string;
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    
    // allocate DCC
    dcc = irc_dcc_alloc ();
    if (!dcc)
    {
        session_crash (file, _("can't create new DCC"));
        return 0;
    }
    
    log_printf (_("session: loading DCC"));
    
    // read DCC values
    ptr_server = NULL;
    ptr_channel = NULL;
    rc = 1;
    while (rc)
    {
        if (feof (file))
        {
            session_crash (file, _("unexpected end of file (reading DCC)"));
            return 0;
        }
        if (fread ((void *)(&object_id), sizeof (int), 1, file) == 0)
            return 0;
        switch (object_id)
        {
            case SESSION_DCC_END:
                return 1;
            case SESSION_DCC_SERVER:
                string = NULL;
                rc = rc && (session_read_str (file, &string));
                if (!rc)
                    return 0;
                if (string && string[0])
                {
                    ptr_server = irc_server_search (string);
                    if (!ptr_server)
                    {
                        session_crash (file, _("server not found for DCC"));
                        return 0;
                    }
                    dcc->server = ptr_server;
                }
                break;
            case SESSION_DCC_CHANNEL:
                if (!ptr_server)
                {
                    session_crash (file, _("DCC with channel but without server"));
                    return 0;
                }
                string = NULL;
                rc = rc && (session_read_str (file, &string));
                if (!rc)
                    return 0;
                if (string && string[0])
                {
                    ptr_channel = irc_channel_search_any (ptr_server, string);
                    if (!ptr_channel)
                    {
                        session_crash (file, _("channel not found for DCC"));
                        return 0;
                    }
                    dcc->channel = ptr_channel;
                    ptr_channel->dcc_chat = dcc;
                }
                break;
            case SESSION_DCC_TYPE:
                rc = rc && (session_read_int (file, &(dcc->type)));
                break;
            case SESSION_DCC_STATUS:
                rc = rc && (session_read_int (file, &(dcc->status)));
                break;
            case SESSION_DCC_START_TIME:
                rc = rc && (session_read_buf (file, &(dcc->start_time), sizeof (time_t)));
                break;
            case SESSION_DCC_START_TRANSFER:
                rc = rc && (session_read_buf (file, &(dcc->start_transfer), sizeof (time_t)));
                break;
            case SESSION_DCC_ADDR:
                rc = rc && (session_read_buf (file, &(dcc->addr), sizeof (unsigned long)));
                break;
            case SESSION_DCC_PORT:
                rc = rc && (session_read_int (file, &(dcc->port)));
                break;
            case SESSION_DCC_NICK:
                rc = rc && (session_read_str (file, &(dcc->nick)));
                break;
            case SESSION_DCC_SOCK:
                rc = rc && (session_read_int (file, &(dcc->sock)));
                break;
            case SESSION_DCC_UNTERMINATED_MESSAGE:
                rc = rc && (session_read_str (file, &(dcc->unterminated_message)));
                break;
            case SESSION_DCC_FILE:
                rc = rc && (session_read_int (file, &(dcc->file)));
                break;
            case SESSION_DCC_FILENAME:
                rc = rc && (session_read_str (file, &(dcc->filename)));
                break;
            case SESSION_DCC_LOCAL_FILENAME:
                rc = rc && (session_read_str (file, &(dcc->local_filename)));
                break;
            case SESSION_DCC_FILENAME_SUFFIX:
                rc = rc && (session_read_int (file, &(dcc->filename_suffix)));
                break;
            case SESSION_DCC_SIZE:
                rc = rc && (session_read_buf (file, &(dcc->size), sizeof (unsigned long)));
                break;
            case SESSION_DCC_POS:
                rc = rc && (session_read_buf (file, &(dcc->pos), sizeof (unsigned long)));
                break;
            case SESSION_DCC_ACK:
                rc = rc && (session_read_buf (file, &(dcc->ack), sizeof (unsigned long)));
                break;
            case SESSION_DCC_START_RESUME:
                rc = rc && (session_read_buf (file, &(dcc->start_resume), sizeof (unsigned long)));
                break;
            case SESSION_DCC_LAST_CHECK_TIME:
                rc = rc && (session_read_buf (file, &(dcc->last_check_time), sizeof (time_t)));
                break;
            case SESSION_DCC_LAST_CHECK_POS:
                rc = rc && (session_read_buf (file, &(dcc->last_check_pos), sizeof (unsigned long)));
                break;
            case SESSION_DCC_LAST_ACTIVITY:
                rc = rc && (session_read_buf (file, &(dcc->last_activity), sizeof (time_t)));
                break;
            case SESSION_DCC_BYTES_PER_SEC:
                rc = rc && (session_read_buf (file, &(dcc->bytes_per_sec), sizeof (unsigned long)));
                break;
            case SESSION_DCC_ETA:
                rc = rc && (session_read_buf (file, &(dcc->eta), sizeof (unsigned long)));
                break;
            case SESSION_DCC_CHILD_PID:
                rc = rc && (session_read_int (file, &(dcc->child_pid)));
                break;
            case SESSION_DCC_CHILD_READ:
                rc = rc && (session_read_int (file, &(dcc->child_read)));
                break;
            case SESSION_DCC_CHILD_WRITE:
                rc = rc && (session_read_int (file, &(dcc->child_write)));
                break;
            default:
                log_printf (_("session: warning: ignoring value from "
                            "DCC (object id: %d)"),
                            object_id);
                rc = rc && (session_read_ignore_value (file));
                break;
        }
    }
    return 0;
}*/

/*
 * session_load_history: load history from file (global or for a buffer)
 */

/*int
session_load_history (FILE *file)
{
    int object_id, rc;
    char *text;
    
    if (session_current_buffer)
        log_printf (_("session: loading buffer history"));
    else
        log_printf (_("session: loading global history"));
    
    // read history values
    rc = 1;
    while (rc)
    {
        if (feof (file))
        {
            session_crash (file, _("unexpected end of file (reading history)"));
            return 0;
        }
        if (fread ((void *)(&object_id), sizeof (int), 1, file) == 0)
            return 0;
        switch (object_id)
        {
            case SESSION_HIST_END:
                return 1;
            case SESSION_HIST_TEXT:
                text = NULL;
                if (!session_read_str_utf8 (file, &text))
                    return 0;
                if (session_current_buffer)
                    gui_history_buffer_add (session_current_buffer, text);
                else
                    gui_history_global_add (text);
                free (text);
                break;
            default:
                log_printf (_("session: warning: ignoring value from "
                            "history (object id: %d)"),
                            object_id);
                rc = rc && (session_read_ignore_value (file));
                break;
        }
    }
    return 0;
}*/

/*
 * session_load_buffer: load buffer from file
 */

/*int
session_load_buffer (FILE *file)
{
    int object_id, rc;
    char *server_name, *channel_name;
    int buffer_type;
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    
    // read server name
    server_name = NULL;
    if (!session_read_object (file, SESSION_BUFF_SERVER, SESSION_TYPE_STR, &server_name, 0))
    {
        session_crash (file, _("server name not found for buffer"));
        return 0;
    }
    
    // read channel name
    channel_name = NULL;
    if (!session_read_object (file, SESSION_BUFF_CHANNEL, SESSION_TYPE_STR, &channel_name, 0))
    {
        session_crash (file, _("channel name not found for buffer"));
        return 0;
    }
    
    // read buffer type
    if (!session_read_object (file, SESSION_BUFF_TYPE, SESSION_TYPE_INT, &buffer_type, 0))
    {
        session_crash (file, _("buffer type not found"));
        return 0;
    }
    
    // allocate buffer
    log_printf (_("session: loading buffer (server: %s, channel: %s, type: %d)"),
                (server_name) ? server_name : "-",
                (channel_name) ? channel_name : "-",
                buffer_type);
    ptr_server = NULL;
    ptr_channel = NULL;
    if (server_name)
    {
        ptr_server = irc_server_search (server_name);
        if (!ptr_server)
        {
            session_crash (file, _("server not found for buffer"));
            return 0;
        }
    }
    
    if (channel_name)
    {
        ptr_channel = irc_channel_search_any_without_buffer (ptr_server, channel_name);
        if (!ptr_channel)
        {
            session_crash (file, _("channel not found for buffer"));
            return 0;
        }
    }
    
    session_current_buffer = gui_buffer_new (gui_windows, weechat_protocols,
                                             ptr_server,
                                             ptr_channel, buffer_type, 1);
    if (!session_current_buffer)
    {
        session_crash (file, _("can't create new buffer"));
        return 0;
    }
    
    free (server_name);
    free (channel_name);
    
    // read buffer values
    rc = 1;
    while (rc)
    {
        if (feof (file))
        {
            session_crash (file, _("unexpected end of file (reading buffer)"));
            return 0;
        }
        if (fread ((void *)(&object_id), sizeof (int), 1, file) == 0)
            return 0;
        switch (object_id)
        {
            case SESSION_BUFF_END:
                return 1;
            case SESSION_BUFF_ALL_SERVERS:
                rc = rc && (session_read_int (file, &(session_current_buffer->all_servers)));
                break;
            default:
                log_printf (_("session: warning: ignoring value from "
                            "buffer (object id: %d)"),
                            object_id);
                rc = rc && (session_read_ignore_value (file));
                break;
        }
    }
    return 0;
}*/

/*
 * session_load_line: load buffer line from file
 */

/*int
session_load_line (FILE *file)
{
    int object_id, rc;
    t_gui_line *line;
    
    // check if buffer is allocated for this line
    if (!session_current_buffer)
    {
        session_crash (file, _("line found without buffer"));
        return 0;
    }
    
    // allocate line
    line = gui_buffer_line_new (session_current_buffer, time (NULL));
    if (!line)
    {
        session_crash (file, _("can't create new line"));
        return 0;
    }
    
    // read line values
    rc = 1;
    while (rc)
    {
        if (feof (file))
        {
            session_crash (file, _("unexpected end of file (reading line)"));
            return 0;
        }
        if (fread ((void *)(&object_id), sizeof (int), 1, file) == 0)
            return 0;
        switch (object_id)
        {
            case SESSION_LINE_END:
                return 1;
            case SESSION_LINE_LENGTH:
                rc = rc && (session_read_int (file, &(line->length)));
                break;
            case SESSION_LINE_LENGTH_ALIGN:
                rc = rc && (session_read_int (file, &(line->length_align)));
                break;
            case SESSION_LINE_LOG_WRITE:
                rc = rc && (session_read_int (file, &(line->log_write)));
                break;
            case SESSION_LINE_WITH_MESSAGE:
                rc = rc && (session_read_int (file, &(line->line_with_message)));
                break;
            case SESSION_LINE_WITH_HIGHLIGHT:
                rc = rc && (session_read_int (file, &(line->line_with_highlight)));
                break;
            case SESSION_LINE_DATA:
                rc = rc && (session_read_str_utf8 (file, &(line->data)));
                break;
            case SESSION_LINE_OFS_AFTER_DATE:
                rc = rc && (session_read_int (file, &(line->ofs_after_date)));
                break;
            case SESSION_LINE_OFS_START_MESSAGE:
                rc = rc && (session_read_int (file, &(line->ofs_start_message)));
                break;
            case SESSION_LINE_NICK:
                rc = rc && (session_read_str (file, &(line->nick)));
                break;
            case SESSION_LINE_DATE:
                rc = rc && (session_read_buf (file, &(line->date), sizeof (time_t)));
                break;
            default:
                log_printf (_("session: warning: ignoring value from "
                            "line (object id: %d)"),
                            object_id);
                rc = rc && (session_read_ignore_value (file));
                break;
        }
    }
    return 0;
}*/

/*
 * session_load_uptime: load uptime from file
 */

/*int
session_load_uptime (FILE *file)
{
    int object_id, rc;
    
    // read uptime values
    rc = 1;
    while (rc)
    {
        if (feof (file))
        {
            session_crash (file, _("unexpected end of file (reading uptime)"));
            return 0;
        }
        if (fread ((void *)(&object_id), sizeof (int), 1, file) == 0)
            return 0;
        switch (object_id)
        {
            case SESSION_UPT_END:
                return 1;
            case SESSION_UPT_START_TIME:
                rc = rc && (session_read_buf (file, &weechat_start_time, sizeof (time_t)));
                break;
            default:
                log_printf (_("session: warning: ignoring value from "
                            "uptime (object id: %d)"),
                            object_id);
                rc = rc && (session_read_ignore_value (file));
                break;
        }
    }
    return 0;
}*/

/*
 * session_load_hotlist: load hotlist from file
 */

/*int
session_load_hotlist (FILE *file)
{
    int object_id, rc;
    int priority;
    struct timeval creation_time;
    char *server_name;
    t_irc_server *ptr_server;
    int buffer_number;
    t_gui_buffer *ptr_buffer;

    priority = 0;
    creation_time.tv_sec = 0;
    creation_time.tv_usec = 0;
    ptr_server = NULL;
    ptr_buffer = NULL;
    
    // read hotlist values
    rc = 1;
    while (rc)
    {
        if (feof (file))
        {
            session_crash (file, _("unexpected end of file (reading hotlist)"));
            return 0;
        }
        if (fread ((void *)(&object_id), sizeof (int), 1, file) == 0)
            return 0;
        switch (object_id)
        {
            case SESSION_HOTL_END:
                hotlist_add (priority, &creation_time, ptr_server, ptr_buffer, 1);
                return 1;
            case SESSION_HOTL_PRIORITY:
                rc = rc && (session_read_int (file, &priority));
                break;
            case SESSION_HOTL_SERVER:
                server_name = NULL;
                if (!session_read_str (file, &server_name))
                    return 0;
                ptr_server = irc_server_search (server_name);
                free (server_name);
                break;
            case SESSION_HOTL_BUFFER_NUMBER:
                rc = rc && (session_read_int (file, &buffer_number));
                ptr_buffer = gui_buffer_search_by_number (buffer_number);
                break;
            case SESSION_HOTL_CREATION_TIME:
                rc = rc && (session_read_buf (file, &creation_time, sizeof (struct timeval)));
                break;
            default:
                log_printf (_("session: warning: ignoring value from "
                            "history (object id: %d)"),
                            object_id);
                rc = rc && (session_read_ignore_value (file));
                break;
        }
    }
    return 0;
}*/

/*
 * session_load: load session from file
 */

/*int
session_load (char *filename)
{
    FILE *file;
    char *signature;
    int object_id;
    t_irc_server *ptr_server;
    
    session_current_server = NULL;
    session_current_channel = NULL;
    session_current_buffer = NULL;
    
    session_last_read_pos = -1;
    session_last_read_length = -1;
    
    if ((file = fopen (filename, "rb")) == NULL)
    {
        session_crash (file, _("session file not found"));
        return 0;
    }
    
    signature = NULL;
    if (!session_read_str (file, &signature))
    {
        session_crash (file, _("signature not found"));
        return 0;
    }
    if (!signature || (strcmp (signature, SESSION_SIGNATURE) != 0))
    {
        session_crash (file, _("bad session signature"));
        return 0;
    }
    free (signature);
    
    while (!feof (file))
    {
        if (fread ((void *)(&object_id), sizeof (int), 1, file) == 0)
        {
            if (feof (file))
                break;
            session_crash (file, _("object id not found"));
            return 0;
        }
        switch (object_id)
        {
            case SESSION_OBJ_SERVER:
                if (!session_load_server (file))
                {
                    session_crash (file, _("failed to load server"));
                    return 0;
                }
                break;
            case SESSION_OBJ_CHANNEL:
                if (!session_load_channel (file))
                {
                    session_crash (file, _("failed to load channel"));
                    return 0;
                }
                break;
            case SESSION_OBJ_NICK:
                if (!session_load_nick (file))
                {
                    session_crash (file, _("failed to load nick"));
                    return 0;
                }
                break;
            case SESSION_OBJ_DCC:
                if (!session_load_dcc (file))
                {
                    session_crash (file, _("failed to load DCC"));
                    return 0;
                }
                break;
            case SESSION_OBJ_HISTORY:
                if (!session_load_history (file))
                {
                    session_crash (file, _("failed to load history"));
                    return 0;
                }
                break;
            case SESSION_OBJ_BUFFER:
                if (!session_load_buffer (file))
                {
                    session_crash (file, _("failed to load buffer"));
                    return 0;
                }
                break;
            case SESSION_OBJ_LINE:
                if (!session_load_line (file))
                {
                    session_crash (file, _("failed to load line"));
                    return 0;
                }
                break;
            case SESSION_OBJ_UPTIME:
                if (!session_load_uptime (file))
                {
                    session_crash (file, _("failed to load uptime"));
                    return 0;
                }
                break;
            case SESSION_OBJ_HOTLIST:
                if (!session_load_hotlist (file))
                {
                    session_crash (file, _("failed to load hotlist"));
                    return 0;
                }
                break;
            default:
                log_printf (_("ignoring object (id: %d)"),
                            object_id);
                if (!session_read_ignore_object (file))
                {
                    session_crash (file, _("failed to ignore object (id: %d)"),
                                   object_id);
                    return 0;
                }
        }
    }
    
    // assign a buffer to all connected servers
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if ((ptr_server->is_connected) && (!ptr_server->buffer))
            ptr_server->buffer = gui_buffers;
    }
    
    gui_window_switch_to_buffer (gui_windows, gui_buffers);
    gui_window_redraw_buffer (gui_current_window->buffer);
    
    fclose (file);
    
    if (unlink (filename) < 0)
    {
        gui_printf_error_nolog (gui_current_window->buffer,
                                _("Warning: can't delete session file (%s)\n"),
                                filename);
    }
    
    gui_printf_info_nolog (gui_current_window->buffer,
                           _("Upgrade completed successfully\n"));
    
    return 1;
}*/
