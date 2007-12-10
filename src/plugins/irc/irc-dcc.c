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

/* irc-dcc.c: Direct Client-to-Client (DCC) communication (files & chat) */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "irc.h"
#include "irc-dcc.h"
#include "irc-config.h"


struct t_irc_dcc *irc_dcc_list = NULL; /* DCC files & chat list             */
struct t_irc_dcc *irc_last_dcc = NULL; /* last DCC in list                  */

char *irc_dcc_status_string[] =        /* strings for DCC status            */
{ N_("Waiting"), N_("Connecting"), N_("Active"), N_("Done"), N_("Failed"),
  N_("Aborted") };


/*
 * irc_dcc_redraw: redraw DCC buffer (and add to hotlist)
 */

void
irc_dcc_redraw (int highlight)
{
    (void) highlight;
    /*struct t_gui_buffer *ptr_buffer;

    ptr_buffer = gui_buffer_get_dcc (gui_current_window);
    gui_window_redraw_buffer (ptr_buffer);
    if (highlight && gui_add_hotlist && (ptr_buffer->num_displayed == 0))
    {
        gui_hotlist_add (highlight, NULL, ptr_buffer, 0);
        gui_status_draw (gui_current_window->buffer, 0);
    }
    */
}

/*
 * irc_dcc_search: search a DCC
 */

struct t_irc_dcc *
irc_dcc_search (struct t_irc_server *server, int type, int status, int port)
{
    struct t_irc_dcc *ptr_dcc;
    
    for (ptr_dcc = irc_dcc_list; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
    {
        if ((ptr_dcc->server == server)
            && (ptr_dcc->type == type)
            && (ptr_dcc->status = status)
            && (ptr_dcc->port == port))
            return ptr_dcc;
    }
    
    /* DCC not found */
    return NULL;
}

/*
 * irc_dcc_port_in_use: return 1 if a port is in used
 *                      (by an active or connecting DCC)
 */

int
irc_dcc_port_in_use (int port)
{
    struct t_irc_dcc *ptr_dcc;
    
    /* skip any currently used ports */
    for (ptr_dcc = irc_dcc_list; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
    {
        if ((ptr_dcc->port == port) && (!IRC_DCC_ENDED(ptr_dcc->status)))
            return 1;
    }
    
    /* port not in use */
    return 0;
}

/*
 * irc_dcc_file_is_resumable: check if a file can be used for resuming a download
 */

int
irc_dcc_file_is_resumable (struct t_irc_dcc *ptr_dcc, char *filename)
{
    struct stat st;
    
    if (!irc_cfg_dcc_auto_resume)
        return 0;
    
    if (access (filename, W_OK) == 0)
    {
        if (stat (filename, &st) != -1)
        {
            if ((unsigned long) st.st_size < ptr_dcc->size)
            {
                ptr_dcc->start_resume = (unsigned long) st.st_size;
                ptr_dcc->pos = st.st_size;
                ptr_dcc->last_check_pos = st.st_size;
                return 1;
            }
        }
    }
    
    /* not resumable */
    return 0;
}

/*
 * irc_dcc_find_filename: find local filename for a DCC
 *                        if type if file/recv, add a suffix (like .1) if needed
 *                        if download is resumable, set "start_resume" to good value
 */

void
irc_dcc_find_filename (struct t_irc_dcc *ptr_dcc)
{
    char *dir1, *dir2, *filename2;
    
    if (!IRC_DCC_IS_FILE(ptr_dcc->type))
        return;
    
    dir1 = weechat_strreplace (irc_cfg_dcc_download_path, "~", getenv ("HOME"));
    if (!dir1)
        return;
    dir2 = weechat_strreplace (dir1, "%h", weechat_home);
    if (!dir2)
    {
        free (dir1);
        return;
    }
    
    ptr_dcc->local_filename = (char *) malloc (strlen (dir2) +
                                               strlen (ptr_dcc->nick) +
                                               strlen (ptr_dcc->filename) + 4);
    if (!ptr_dcc->local_filename)
        return;
    
    strcpy (ptr_dcc->local_filename, dir2);
    if (ptr_dcc->local_filename[strlen (ptr_dcc->local_filename) - 1] != DIR_SEPARATOR_CHAR)
        strcat (ptr_dcc->local_filename, DIR_SEPARATOR);
    strcat (ptr_dcc->local_filename, ptr_dcc->nick);
    strcat (ptr_dcc->local_filename, ".");
    strcat (ptr_dcc->local_filename, ptr_dcc->filename);
    
    if (dir1)
        free (dir1);
    if (dir2 )
        free (dir2);
    
    /* file already exists? */
    if (access (ptr_dcc->local_filename, F_OK) == 0)
    {
        if (irc_dcc_file_is_resumable (ptr_dcc, ptr_dcc->local_filename))
            return;
        
        /* if auto rename is not set, then abort DCC */
        if (!irc_cfg_dcc_auto_rename)
        {
            irc_dcc_close (ptr_dcc, IRC_DCC_FAILED);
            irc_dcc_redraw (GUI_HOTLIST_MSG);
            return;
        }
        
        filename2 = (char *) malloc (strlen (ptr_dcc->local_filename) + 16);
        if (!filename2)
        {
            irc_dcc_close (ptr_dcc, IRC_DCC_FAILED);
            irc_dcc_redraw (GUI_HOTLIST_MSG);
            return;
        }
        ptr_dcc->filename_suffix = 0;
        do
        {
            ptr_dcc->filename_suffix++;
            sprintf (filename2, "%s.%d",
                     ptr_dcc->local_filename,
                     ptr_dcc->filename_suffix);
            if (access (filename2, F_OK) == 0)
            {
                if (irc_dcc_file_is_resumable (ptr_dcc, filename2))
                    break;
            }
            else
                break;
        }
        while (1);
        
        free (ptr_dcc->local_filename);
        ptr_dcc->local_filename = strdup (filename2);
        free (filename2);
    }
}

/*
 * irc_dcc_calculate_speed: calculate DCC speed (for files only)
 */

void
irc_dcc_calculate_speed (struct t_irc_dcc *ptr_dcc, int ended)
{
    time_t local_time, elapsed;
    unsigned long bytes_per_sec_total;
    
    local_time = time (NULL);
    if (ended || local_time > ptr_dcc->last_check_time)
    {
        if (ended)
        {
            /* calculate bytes per second (global) */
            elapsed = local_time - ptr_dcc->start_transfer;
            if (elapsed == 0)
                elapsed = 1;
            ptr_dcc->bytes_per_sec = (ptr_dcc->pos - ptr_dcc->start_resume) / elapsed;
            ptr_dcc->eta = 0;
        }
        else
        {
            /* calculate ETA */
            elapsed = local_time - ptr_dcc->start_transfer;
            if (elapsed == 0)
                elapsed = 1;
            bytes_per_sec_total = (ptr_dcc->pos - ptr_dcc->start_resume) / elapsed;
            if (bytes_per_sec_total == 0)
                bytes_per_sec_total = 1;
            ptr_dcc->eta = (ptr_dcc->size - ptr_dcc->pos) / bytes_per_sec_total;
            
            /* calculate bytes per second (since last check time) */
            elapsed = local_time - ptr_dcc->last_check_time;
            if (elapsed == 0)
                elapsed = 1;
            ptr_dcc->bytes_per_sec = (ptr_dcc->pos - ptr_dcc->last_check_pos) / elapsed;
        }
        ptr_dcc->last_check_time = local_time;
        ptr_dcc->last_check_pos = ptr_dcc->pos;
    }
}

/*
 * irc_dcc_connect_to_sender: connect to sender
 */

int
irc_dcc_connect_to_sender (struct t_irc_dcc *ptr_dcc)
{
    struct sockaddr_in addr;
    struct hostent *hostent;
    char *ip4;
    int ret;
    
    if (cfg_proxy_use)
    {
        memset (&addr, 0, sizeof (addr));
        addr.sin_addr.s_addr = htonl (ptr_dcc->addr);
        ip4 = inet_ntoa(addr.sin_addr);
        
        memset (&addr, 0, sizeof (addr));
        addr.sin_port = htons (cfg_proxy_port);
        addr.sin_family = AF_INET;
        if ((hostent = gethostbyname (cfg_proxy_address)) == NULL)
            return 0;
        memcpy(&(addr.sin_addr),*(hostent->h_addr_list), sizeof(struct in_addr));
        ret = connect (ptr_dcc->sock, (struct sockaddr *) &addr, sizeof (addr));
        if ((ret == -1) && (errno != EINPROGRESS))
            return 0;
        if (irc_server_pass_proxy (ptr_dcc->sock, ip4, ptr_dcc->port,
                                   ptr_dcc->server->username) == -1)
            return 0;
    }
    else
    {
        memset (&addr, 0, sizeof (addr));
        addr.sin_port = htons (ptr_dcc->port);
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl (ptr_dcc->addr);
        ret = connect (ptr_dcc->sock, (struct sockaddr *) &addr, sizeof (addr));
        if ((ret == -1) && (errno != EINPROGRESS))
            return 0;
    }
    return 1;
}

/*
 * irc_dcc_connect: connect to another host
 */

int
irc_dcc_connect (struct t_irc_dcc *ptr_dcc)
{
    if (ptr_dcc->type == IRC_DCC_CHAT_SEND)
        ptr_dcc->status = IRC_DCC_WAITING;
    else
        ptr_dcc->status = IRC_DCC_CONNECTING;
    
    if (ptr_dcc->sock < 0)
    {
        ptr_dcc->sock = socket (AF_INET, SOCK_STREAM, 0);
        if (ptr_dcc->sock < 0)
            return 0;
    }

    /* for chat or file sending, listen to socket for a connection */
    if (IRC_DCC_IS_SEND(ptr_dcc->type))
    {
        if (fcntl (ptr_dcc->sock, F_SETFL, O_NONBLOCK) == -1)
            return 0;	
        if (listen (ptr_dcc->sock, 1) == -1)
            return 0;
        if (fcntl (ptr_dcc->sock, F_SETFL, 0) == -1)
            return 0;
    }
    
    /* for chat receiving, connect to listening host */
    if (ptr_dcc->type == IRC_DCC_CHAT_RECV)
    {
        if (fcntl (ptr_dcc->sock, F_SETFL, O_NONBLOCK) == -1)
            return 0;
        irc_dcc_connect_to_sender (ptr_dcc);
    }

    /* for file receiving, connection is made in child process (blocking) socket */
    
    return 1;
}

/*
 * irc_dcc_free: free DCC struct and remove it from list
 */

void
irc_dcc_free (struct t_irc_dcc *ptr_dcc)
{
    struct t_irc_dcc *new_dcc_list;
    
    if (!ptr_dcc)
        return;
    
    /* DCC CHAT with channel => remove channel
       (to prevent channel from becoming standard pv) */
    if (ptr_dcc->channel)
    {
        /* check if channel is used for another active DCC CHAT */
        if (!ptr_dcc->channel->dcc_chat
            || (IRC_DCC_ENDED(ptr_dcc->channel->dcc_chat->status)))
        {
            gui_buffer_free (ptr_dcc->channel->buffer, 1);
            if (ptr_dcc->channel)
                irc_channel_free (ptr_dcc->server, ptr_dcc->channel);
        }
    }

    /* remove DCC from list */
    if (irc_last_dcc == ptr_dcc)
        irc_last_dcc = ptr_dcc->prev_dcc;
    if (ptr_dcc->prev_dcc)
    {
        (ptr_dcc->prev_dcc)->next_dcc = ptr_dcc->next_dcc;
        new_dcc_list = irc_dcc_list;
    }
    else
        new_dcc_list = ptr_dcc->next_dcc;
    if (ptr_dcc->next_dcc)
        (ptr_dcc->next_dcc)->prev_dcc = ptr_dcc->prev_dcc;

    /* free data */
    if (ptr_dcc->nick)
        free (ptr_dcc->nick);
    if (ptr_dcc->unterminated_message)
        free (ptr_dcc->unterminated_message);
    if (ptr_dcc->filename)
        free (ptr_dcc->filename);
    
    free (ptr_dcc);
    irc_dcc_list = new_dcc_list;
}

/*
 * irc_dcc_file_child_kill: kill child process and close pipe
 */

void
irc_dcc_file_child_kill (struct t_irc_dcc *ptr_dcc)
{
    /* kill process */
    if (ptr_dcc->child_pid > 0)
    {
        kill (ptr_dcc->child_pid, SIGKILL);
        waitpid (ptr_dcc->child_pid, NULL, 0);
        ptr_dcc->child_pid = 0;
    }
    
    /* close pipe used with child */
    if (ptr_dcc->child_read != -1)
    {
        close (ptr_dcc->child_read);
        ptr_dcc->child_read = -1;
    }
    if (ptr_dcc->child_write != -1)
    {
        close (ptr_dcc->child_write);
        ptr_dcc->child_write = -1;
    }
}

/*
 * irc_dcc_close: close a DCC connection
 */

void
irc_dcc_close (struct t_irc_dcc *ptr_dcc, int status)
{
    t_gui_buffer *ptr_buffer;
    struct stat st;
    
    ptr_dcc->status = status;
    
    if ((status == IRC_DCC_DONE) || (status == IRC_DCC_ABORTED)
        || (status == IRC_DCC_FAILED))
    {
        if (IRC_DCC_IS_FILE(ptr_dcc->type))
        {
            gui_chat_printf_info (ptr_dcc->server->buffer,
                                  _("DCC: file %s%s%s"),
                                  GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                  ptr_dcc->filename,
                                  GUI_COLOR(GUI_COLOR_CHAT));
            if (ptr_dcc->local_filename)
                gui_chat_printf (ptr_dcc->server->buffer,
                                 _(" (local filename: %s%s%s)"),
                                 GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                 ptr_dcc->local_filename,
                                 GUI_COLOR(GUI_COLOR_CHAT));
            if (ptr_dcc->type == IRC_DCC_FILE_SEND)
                gui_chat_printf (ptr_dcc->server->buffer,
                                 _(" sent to "));
            else
                gui_chat_printf (ptr_dcc->server->buffer,
                                 _(" received from "));
            gui_chat_printf (ptr_dcc->server->buffer, "%s%s%s: %s\n",
                             GUI_COLOR(GUI_COLOR_CHAT_NICK),
                             ptr_dcc->nick,
                             GUI_COLOR(GUI_COLOR_CHAT),
                             (status == IRC_DCC_DONE) ? _("OK") : _("FAILED"));
            irc_dcc_file_child_kill (ptr_dcc);
        }
    }
    if (status == IRC_DCC_ABORTED)
    {
        if (IRC_DCC_IS_CHAT(ptr_dcc->type))
        {
            if (ptr_dcc->channel)
                ptr_buffer = ptr_dcc->channel->buffer;
            else
                ptr_buffer = ptr_dcc->server->buffer;
            gui_chat_printf_info (ptr_buffer,
                                  _("DCC chat closed with %s%s "
                                    "%s(%s%d.%d.%d.%d%s)\n"),
                                  GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                  ptr_dcc->nick,
                                  GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                  GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                  ptr_dcc->addr >> 24,
                                  (ptr_dcc->addr >> 16) & 0xff,
                                  (ptr_dcc->addr >> 8) & 0xff,
                                  ptr_dcc->addr & 0xff,
                                  GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
        }
    }
    
    /* remove empty file if received file failed and nothing was transfered */
    if (((status == IRC_DCC_FAILED) || (status == IRC_DCC_ABORTED))
        && IRC_DCC_IS_FILE(ptr_dcc->type)
        && IRC_DCC_IS_RECV(ptr_dcc->type)
        && ptr_dcc->local_filename
        && ptr_dcc->pos == 0)
    {
        /* erase file only if really empty on disk */
        if (stat (ptr_dcc->local_filename, &st) != -1)
        {
            if ((unsigned long) st.st_size == 0)
                unlink (ptr_dcc->local_filename);
        }
    }
    
    if (IRC_DCC_IS_FILE(ptr_dcc->type))
        irc_dcc_calculate_speed (ptr_dcc, 1);
    
    if (ptr_dcc->sock >= 0)
    {
        close (ptr_dcc->sock);
        ptr_dcc->sock = -1;
    }
    if (ptr_dcc->file >= 0)
    {
        close (ptr_dcc->file);
        ptr_dcc->file = -1;
    }
}

/*
 * irc_dcc_channel_for_chat: create channel for DCC chat
 */

void
irc_dcc_channel_for_chat (struct t_irc_dcc *ptr_dcc)
{
    if (!irc_channel_create_dcc (ptr_dcc))
    {
        gui_chat_printf_error (ptr_dcc->server->buffer,
                               _("%s can't associate DCC chat with private "
                                 "buffer (maybe private buffer has already "
                                 "DCC CHAT?)\n"),
                               WEECHAT_ERROR);
        irc_dcc_close (ptr_dcc, IRC_DCC_FAILED);
        irc_dcc_redraw (GUI_HOTLIST_MSG);
        return;
    }
    
    gui_chat_printf_type (ptr_dcc->channel->buffer, GUI_MSG_TYPE_MSG,
                          cfg_look_prefix_info, cfg_col_chat_prefix_info,
                          _("Connected to %s%s %s(%s%d.%d.%d.%d%s)%s via DCC "
                            "chat\n"),
                          GUI_COLOR(GUI_COLOR_CHAT_NICK),
                          ptr_dcc->nick,
                          GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                          GUI_COLOR(GUI_COLOR_CHAT_HOST),
                          ptr_dcc->addr >> 24,
                          (ptr_dcc->addr >> 16) & 0xff,
                          (ptr_dcc->addr >> 8) & 0xff,
                          ptr_dcc->addr & 0xff,
                          GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                          GUI_COLOR(GUI_COLOR_CHAT));
}

/*
 * irc_dcc_chat_remove_channel: remove a buffer for DCC chat
 */

void
irc_dcc_chat_remove_channel (struct t_irc_channel *channel)
{
    struct t_irc_dcc *ptr_dcc;
    
    if (!channel)
        return;

    for (ptr_dcc = irc_dcc_list; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
    {
        if (ptr_dcc->channel == channel)
            ptr_dcc->channel = NULL;
    }
}

/*
 * irc_dcc_recv_connect_init: connect to sender and init file or chat
 */

void
irc_dcc_recv_connect_init (struct t_irc_dcc *ptr_dcc)
{
    if (!irc_dcc_connect (ptr_dcc))
    {
        irc_dcc_close (ptr_dcc, IRC_DCC_FAILED);
        irc_dcc_redraw (GUI_HOTLIST_MSG);
    }
    else
    {
        /* DCC file => launch child process */
        if (IRC_DCC_IS_FILE(ptr_dcc->type))
        {
            ptr_dcc->status = IRC_DCC_CONNECTING;
            irc_dcc_file_recv_fork (ptr_dcc);
        }
        else
        {
            /* DCC CHAT => associate DCC with channel */
            ptr_dcc->status = IRC_DCC_ACTIVE;
            irc_dcc_channel_for_chat (ptr_dcc);
        }
    }
    irc_dcc_redraw (GUI_HOTLIST_MSG);
}

/*
 * irc_dcc_accept: accepts a DCC file or chat request
 */

void
irc_dcc_accept (struct t_irc_dcc *ptr_dcc)
{
    if (IRC_DCC_IS_FILE(ptr_dcc->type) && (ptr_dcc->start_resume > 0))
    {
        ptr_dcc->status = IRC_DCC_CONNECTING;
        irc_server_sendf (ptr_dcc->server,
                          (strchr (ptr_dcc->filename, ' ')) ?
                          "PRIVMSG %s :\01DCC RESUME \"%s\" %d %u\01\n" :
                          "PRIVMSG %s :\01DCC RESUME %s %d %u\01",
                          ptr_dcc->nick, ptr_dcc->filename,
                          ptr_dcc->port, ptr_dcc->start_resume);
        irc_dcc_redraw (GUI_HOTLIST_MSG);
    }
    else
        irc_dcc_recv_connect_init (ptr_dcc);
}

/*
 * irc_dcc_accept_resume: accepts a resume and inform the receiver
 */

void
irc_dcc_accept_resume (struct t_irc_server *server, char *filename, int port,
                       unsigned long pos_start)
{
    struct t_irc_dcc *ptr_dcc;
    
    ptr_dcc = irc_dcc_search (server, IRC_DCC_FILE_SEND, IRC_DCC_CONNECTING,
                              port);
    if (ptr_dcc)
    {
        ptr_dcc->pos = pos_start;
        ptr_dcc->ack = pos_start;
        ptr_dcc->start_resume = pos_start;
        ptr_dcc->last_check_pos = pos_start;
        irc_server_sendf (ptr_dcc->server,
                          (strchr (ptr_dcc->filename, ' ')) ?
                          "PRIVMSG %s :\01DCC ACCEPT \"%s\" %d %u\01\n" :
                          "PRIVMSG %s :\01DCC ACCEPT %s %d %u\01",
                          ptr_dcc->nick, ptr_dcc->filename,
                          ptr_dcc->port, ptr_dcc->start_resume);
        
        gui_chat_printf_info (ptr_dcc->server->buffer,
                              _("DCC: file %s%s%s resumed at position %u\n"),
                              GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                              ptr_dcc->filename,
                              GUI_COLOR(GUI_COLOR_CHAT),
                              ptr_dcc->start_resume);
        irc_dcc_redraw (GUI_HOTLIST_MSG);
    }
    else
        gui_chat_printf (server->buffer,
                         _("%s can't resume file \"%s\" (port: %d, start "
                           "position: %u): DCC not found or ended\n"),
                         WEECHAT_ERROR, filename, port, pos_start);
}

/*
 * irc_dcc_start_resume: called when "DCC ACCEPT" is received
 *                       (resume accepted by sender)
 */

void
irc_dcc_start_resume (struct t_irc_server *server, char *filename, int port,
                      unsigned long pos_start)
{
    struct t_irc_dcc *ptr_dcc;
    
    ptr_dcc = irc_dcc_search (server, IRC_DCC_FILE_RECV, IRC_DCC_CONNECTING,
                              port);
    if (ptr_dcc)
    {
        ptr_dcc->pos = pos_start;
        ptr_dcc->ack = pos_start;
        ptr_dcc->start_resume = pos_start;
        ptr_dcc->last_check_pos = pos_start;
        irc_dcc_recv_connect_init (ptr_dcc);
    }
    else
        gui_chat_printf (server->buffer,
                         _("%s can't resume file \"%s\" (port: %d, start "
                           "position: %u): DCC not found or ended\n"),
                         WEECHAT_ERROR, filename, port, pos_start);
}

/*
 * irc_dcc_alloc: allocate a new DCC file
 */

struct t_irc_dcc *
irc_dcc_alloc ()
{
    struct t_irc_dcc *new_dcc;
    
    /* create new DCC struct */
    if ((new_dcc = (struct t_irc_dcc *) malloc (sizeof (struct t_irc_dcc))) == NULL)
        return NULL;
    
    /* default values */
    new_dcc->server = NULL;
    new_dcc->channel = NULL;
    new_dcc->type = 0;
    new_dcc->status = 0;
    new_dcc->start_time = 0;
    new_dcc->start_transfer = 0;
    new_dcc->addr = 0;
    new_dcc->port = 0;
    new_dcc->nick = NULL;
    new_dcc->sock = -1;
    new_dcc->child_pid = 0;
    new_dcc->child_read = -1;
    new_dcc->child_write = -1;
    new_dcc->unterminated_message = NULL;
    new_dcc->fast_send = irc_cfg_dcc_fast_send;
    new_dcc->file = -1;
    new_dcc->filename = NULL;
    new_dcc->local_filename = NULL;
    new_dcc->filename_suffix = -1;
    new_dcc->blocksize = irc_cfg_dcc_blocksize;
    new_dcc->size = 0;
    new_dcc->pos = 0;
    new_dcc->ack = 0;
    new_dcc->start_resume = 0;
    new_dcc->last_check_time = 0;
    new_dcc->last_check_pos = 0;
    new_dcc->last_activity = 0;
    new_dcc->bytes_per_sec = 0;
    new_dcc->eta = 0;
    
    new_dcc->prev_dcc = NULL;
    new_dcc->next_dcc = irc_dcc_list;
    if (irc_dcc_list)
        irc_dcc_list->prev_dcc = new_dcc;
    else
        irc_last_dcc = new_dcc;
    irc_dcc_list = new_dcc;
    
    return new_dcc;
}

/*
 * irc_dcc_add: add a DCC file to queue
 */

struct t_irc_dcc *
irc_dcc_add (struct t_irc_server *server, int type, unsigned long addr, int port, char *nick,
             int sock, char *filename, char *local_filename, unsigned long size)
{
    struct t_irc_dcc *new_dcc;
    
    new_dcc = irc_dcc_alloc ();
    if (!new_dcc)
    {
        gui_chat_printf_error (server->buffer,
                               _("%s not enough memory for new DCC\n"),
                               WEECHAT_ERROR);
        return NULL;
    }
    
    /* initialize new DCC */
    new_dcc->server = server;
    new_dcc->channel = NULL;
    new_dcc->type = type;
    new_dcc->status = IRC_DCC_WAITING;
    new_dcc->start_time = time (NULL);
    new_dcc->start_transfer = time (NULL);
    new_dcc->addr = addr;
    new_dcc->port = port;
    new_dcc->nick = strdup (nick);
    new_dcc->sock = sock;
    new_dcc->unterminated_message = NULL;
    new_dcc->file = -1;
    if (IRC_DCC_IS_CHAT(type))
        new_dcc->filename = strdup (_("DCC chat"));
    else
        new_dcc->filename = (filename) ? strdup (filename) : NULL;
    new_dcc->local_filename = NULL;
    new_dcc->filename_suffix = -1;
    new_dcc->size = size;
    new_dcc->pos = 0;
    new_dcc->ack = 0;
    new_dcc->start_resume = 0;
    new_dcc->last_check_time = time (NULL);
    new_dcc->last_check_pos = 0;
    new_dcc->last_activity = time (NULL);
    new_dcc->bytes_per_sec = 0;
    new_dcc->eta = 0;
    if (local_filename)
        new_dcc->local_filename = strdup (local_filename);
    else
        irc_dcc_find_filename (new_dcc);
    
    gui_current_window->dcc_first = NULL;
    gui_current_window->dcc_selected = NULL;
    
    /* write info message on server buffer */
    if (type == IRC_DCC_FILE_RECV)
    {
        gui_chat_printf_info (server->buffer,
                              _("Incoming DCC file from %s%s%s "
                                "(%s%d.%d.%d.%d%s)%s: %s%s%s, "
                                "%s%lu%s bytes\n"),
                              GUI_COLOR(GUI_COLOR_CHAT_NICK),
                              nick,
                              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                              GUI_COLOR(GUI_COLOR_CHAT_HOST),
                              addr >> 24,
                              (addr >> 16) & 0xff,
                              (addr >> 8) & 0xff,
                              addr & 0xff,
                              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                              GUI_COLOR(GUI_COLOR_CHAT),
                              GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                              filename,
                              GUI_COLOR(GUI_COLOR_CHAT),
                              GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                              size,
                              GUI_COLOR(GUI_COLOR_CHAT));
        irc_dcc_redraw (GUI_HOTLIST_MSG);
    }
    if (type == IRC_DCC_FILE_SEND)
    {
        gui_chat_printf_info (server->buffer,
                              _("Sending DCC file to %s%s%s: %s%s%s "
                                "(local filename: %s%s%s), %s%lu%s bytes\n"),
                              GUI_COLOR(GUI_COLOR_CHAT_NICK),
                              nick,
                              GUI_COLOR(GUI_COLOR_CHAT),
                              GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                              filename,
                              GUI_COLOR(GUI_COLOR_CHAT),
                              GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                              local_filename,
                              GUI_COLOR(GUI_COLOR_CHAT),
                              GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                              size,
                              GUI_COLOR(GUI_COLOR_CHAT));
        irc_dcc_redraw (GUI_HOTLIST_MSG);
    }
    if (type == IRC_DCC_CHAT_RECV)
    {
        gui_chat_printf_info (server->buffer,
                              _("Incoming DCC chat request from %s%s%s "
                                "(%s%d.%d.%d.%d%s)\n"),
                              GUI_COLOR(GUI_COLOR_CHAT_NICK),
                              nick,
                              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                              GUI_COLOR(GUI_COLOR_CHAT_HOST),
                              addr >> 24,
                              (addr >> 16) & 0xff,
                              (addr >> 8) & 0xff,
                              addr & 0xff,
                              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
        irc_dcc_redraw (GUI_HOTLIST_MSG);
    }
    if (type == IRC_DCC_CHAT_SEND)
    {
        gui_chat_printf_info (server->buffer,
                              _("Sending DCC chat request to %s%s\n"),
                              GUI_COLOR(GUI_COLOR_CHAT_NICK),
                              nick);
        irc_dcc_redraw (GUI_HOTLIST_MSG);
    }
    
    if (IRC_DCC_IS_FILE(type) && (!new_dcc->local_filename))
    {
        irc_dcc_close (new_dcc, IRC_DCC_FAILED);
        irc_dcc_redraw (GUI_HOTLIST_MSG);
        return NULL;
    }
    
    if (IRC_DCC_IS_FILE(type) && (new_dcc->start_resume > 0))
    {
        gui_chat_printf_info (new_dcc->server->buffer,
                              _("DCC: file %s%s%s (local filename: %s%s%s) "
                                "will be resumed at position %u\n"),
                              GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                              new_dcc->filename,
                              GUI_COLOR(GUI_COLOR_CHAT),
                              GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                              new_dcc->local_filename,
                              GUI_COLOR(GUI_COLOR_CHAT),
                              new_dcc->start_resume);
        irc_dcc_redraw (GUI_HOTLIST_MSG);
    }
    
    /* connect if needed and redraw DCC buffer */
    if (IRC_DCC_IS_SEND(type))
    {
        if (!irc_dcc_connect (new_dcc))
        {
            irc_dcc_close (new_dcc, IRC_DCC_FAILED);
            irc_dcc_redraw (GUI_HOTLIST_MSG);
            return NULL;
        }
    }
    
    if ( ( (type == IRC_DCC_CHAT_RECV) && (irc_cfg_dcc_auto_accept_chats) )
        || ( (type == IRC_DCC_FILE_RECV) && (irc_cfg_dcc_auto_accept_files) ) )
        irc_dcc_accept (new_dcc);
    else
        irc_dcc_redraw (GUI_HOTLIST_PRIVATE);
    gui_status_draw (gui_current_window->buffer, 0);
    
    return new_dcc;
}

/*
 * irc_dcc_send_request: send DCC request (file or chat)
 */

void
irc_dcc_send_request (struct t_irc_server *server, int type, char *nick, char *filename)
{
    char *dir1, *dir2, *filename2, *short_filename, *pos;
    int spaces, args, port_start, port_end;
    struct stat st;
    int sock, port;
    struct hostent *host;
    struct in_addr tmpaddr;
    struct sockaddr_in addr;
    socklen_t length;
    unsigned long local_addr;
    struct t_irc_dcc *ptr_dcc;
    
    filename2 = NULL;
    short_filename = NULL;
    spaces = 0;
    
    if (type == IRC_DCC_FILE_SEND)
    {
        /* add home if filename not beginning with '/' or '~' (not for Win32) */
#ifdef _WIN32
        filename2 = strdup (filename);
#else
        if (filename[0] == '/')
            filename2 = strdup (filename);
        else if (filename[0] == '~')
            filename2 = weechat_strreplace (filename, "~", getenv ("HOME"));
        else
        {
            dir1 = weechat_strreplace (irc_cfg_dcc_upload_path, "~",
                                       getenv ("HOME"));
            if (!dir1)
                return;
            dir2 = weechat_strreplace (dir1, "%h", weechat_home);
            if (!dir2)
            {
                free (dir1);
                return;
            }
            filename2 = (char *) malloc (strlen (dir2) +
                                         strlen (filename) + 4);
            if (!filename2)
            {
                gui_chat_printf_error (server->buffer,
                                       _("%s not enough memory for DCC SEND\n"),
                                       WEECHAT_ERROR);
                return;
            }
            strcpy (filename2, dir2);
            if (filename2[strlen (filename2) - 1] != DIR_SEPARATOR_CHAR)
                strcat (filename2, DIR_SEPARATOR);
            strcat (filename2, filename);
            if (dir1)
                free (dir1);
            if (dir2)
                free (dir2);
        }
#endif
        
        /* check if file exists */
        if (stat (filename2, &st) == -1)
        {
            gui_chat_printf_error (server->buffer,
                                   _("%s cannot access file \"%s\"\n"),
                                   WEECHAT_ERROR, filename2);
            if (filename2)
                free (filename2);
            return;
        }
    }
    
    /* get local IP address */
    
    /* look up the IP address from dcc_own_ip, if set */
    local_addr = 0;
    if (irc_cfg_dcc_own_ip && irc_cfg_dcc_own_ip[0])
    {
        host = gethostbyname (irc_cfg_dcc_own_ip);
        if (host)
        {
            memcpy (&tmpaddr, host->h_addr_list[0], sizeof(struct in_addr));
            local_addr = ntohl (tmpaddr.s_addr);
        }
        else
            gui_chat_printf (server->buffer,
                             _("%s could not find address for '%s'. Falling "
                               "back to local IP.\n"),
                             WEECHAT_WARNING, irc_cfg_dcc_own_ip);
    }
    
    /* use the local interface, from the server socket */
    memset (&addr, 0, sizeof (struct sockaddr_in));
    length = sizeof (addr);
    getsockname (server->sock, (struct sockaddr *) &addr, &length);
    addr.sin_family = AF_INET;
    
    /* fallback to the local IP address on the interface, if required */
    if (local_addr == 0)
        local_addr = ntohl (addr.sin_addr.s_addr);
    
    /* open socket for DCC */
    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        gui_chat_printf_error (server->buffer,
                               _("%s cannot create socket for DCC\n"),
                               WEECHAT_ERROR);
        if (filename2)
            free (filename2);
        return;
    }
    
    /* look for port */
    
    port = 0;
    
    if (irc_cfg_dcc_port_range && irc_cfg_dcc_port_range[0])
    {
        /* find a free port in the specified range */
        args = sscanf (irc_cfg_dcc_port_range, "%d-%d", &port_start, &port_end);
        if (args > 0)
        {
            port = port_start;
            if (args == 1)
                port_end = port_start;
            
            /* loop through the entire allowed port range */
            while (port <= port_end)
            {
                if (!irc_dcc_port_in_use (port))
                {
                    /* attempt to bind to the free port */
                    addr.sin_port = htons (port);
                    if (bind (sock, (struct sockaddr *) &addr, sizeof (addr)) == 0)
                        break;
                }
                port++;
            }
            
            if (port > port_end)
                port = -1;
        }
    }
    
    if (port == 0)
    {
        /* find port automatically */
        addr.sin_port = 0;
        if (bind (sock, (struct sockaddr *) &addr, sizeof (addr)) == 0)
        {
            length = sizeof (addr);
            getsockname (sock, (struct sockaddr *) &addr, &length);
            port = ntohs (addr.sin_port);
        }
        else
            port = -1;
    }

    if (port == -1)
    {
        /* Could not find any port to bind */
        gui_chat_printf_error (server->buffer,
                               _("%s cannot find available port for DCC\n"),
                               WEECHAT_ERROR);
        close (sock);
        if (filename2)
            free (filename2);
        return;
    }
    
    if (type == IRC_DCC_FILE_SEND)
    {
        /* extract short filename (without path) */
        pos = strrchr (filename2, DIR_SEPARATOR_CHAR);
        if (pos)
            short_filename = strdup (pos + 1);
        else
            short_filename = strdup (filename2);
        
        /* convert spaces to underscore if asked and needed */
        pos = short_filename;
        spaces = 0;
        while (pos[0])
        {
            if (pos[0] == ' ')
            {
                if (irc_cfg_dcc_convert_spaces)
                    pos[0] = '_';
                else
                    spaces = 1;
            }
            pos++;
        }
    }
    
    /* add DCC entry and listen to socket */
    if (type == IRC_DCC_CHAT_SEND)
        ptr_dcc = irc_dcc_add (server, IRC_DCC_CHAT_SEND, local_addr, port,
                               nick, sock, NULL, NULL, 0);
    else
        ptr_dcc = irc_dcc_add (server, IRC_DCC_FILE_SEND, local_addr, port,
                               nick, sock, short_filename, filename2,
                               st.st_size);
    if (!ptr_dcc)
    {
        gui_chat_printf_error (server->buffer,
                               _("%s cannot send DCC\n"),
                               WEECHAT_ERROR);
        close (sock);
        if (short_filename)
            free (short_filename);
        if (filename2)
            free (filename2);
        return;
    }
    
    /* send DCC request to nick */
    if (type == IRC_DCC_CHAT_SEND)
        irc_server_sendf (server, 
                          "PRIVMSG %s :\01DCC CHAT chat %lu %d\01",
                          nick, local_addr, port);
    else
        irc_server_sendf (server, 
                          (spaces) ?
                          "PRIVMSG %s :\01DCC SEND \"%s\" %lu %d %u\01\n" :
                          "PRIVMSG %s :\01DCC SEND %s %lu %d %u\01",
                          nick, short_filename, local_addr, port,
                          (unsigned long) st.st_size);
    
    if (short_filename)
        free (short_filename);
    if (filename2)
        free (filename2);
}

/*
 * irc_dcc_chat_send: send data to remote host via DCC CHAT
 */

int
irc_dcc_chat_send (struct t_irc_dcc *ptr_dcc, char *buffer, int size_buf)
{
    if (!ptr_dcc)
        return -1;
    
    return send (ptr_dcc->sock, buffer, size_buf, 0);
}

/*
 * irc_dcc_chat_sendf: send formatted data to remote host via DCC CHAT
 */

void
irc_dcc_chat_sendf (struct t_irc_dcc *ptr_dcc, char *fmt, ...)
{
    va_list args;
    static char buffer[4096];
    int size_buf;
    
    if (!ptr_dcc || (ptr_dcc->sock < 0))
        return;
    
    va_start (args, fmt);
    size_buf = vsnprintf (buffer, sizeof (buffer) - 1, fmt, args);
    va_end (args);
    
    if ((size_buf == 0) || (strcmp (buffer, "\r\n") == 0))
        return;
    
    buffer[sizeof (buffer) - 1] = '\0';
    if ((size_buf < 0) || (size_buf > (int) (sizeof (buffer) - 1)))
        size_buf = strlen (buffer);
#ifdef DEBUG
    buffer[size_buf - 2] = '\0';
    gui_chat_printf (ptr_dcc->server->buffer,
                     "[DEBUG] Sending to remote host (DCC CHAT) >>> %s\n",
                     buffer);
    buffer[size_buf - 2] = '\r';
#endif
    if (irc_dcc_chat_send (ptr_dcc, buffer, strlen (buffer)) <= 0)
    {
        gui_chat_printf_error (ptr_dcc->server->buffer,
                               _("%s error sending data to \"%s\" via DCC "
                                 "CHAT\n"),
                               WEECHAT_ERROR, ptr_dcc->nick);
        irc_dcc_close (ptr_dcc, IRC_DCC_FAILED);
    }
}

/*
 * irc_dcc_chat_recv: receive data from DCC CHAT host
 */

void
irc_dcc_chat_recv (struct t_irc_dcc *ptr_dcc)
{
    fd_set read_fd;
    static struct timeval timeout;
    static char buffer[4096 + 2];
    char *buf2, *pos, *ptr_buf, *next_ptr_buf;
    int num_read;
    
    FD_ZERO (&read_fd);
    FD_SET (ptr_dcc->sock, &read_fd);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    /* something to read on socket? */
    if (select (FD_SETSIZE, &read_fd, NULL, NULL, &timeout) <= 0)
        return;

    if (!FD_ISSET (ptr_dcc->sock, &read_fd))
        return;

    /* there's something to read on socket! */
    num_read = recv (ptr_dcc->sock, buffer, sizeof (buffer) - 2, 0);
    if (num_read > 0)
    {
        buffer[num_read] = '\0';
        
        buf2 = NULL;
        ptr_buf = buffer;
        if (ptr_dcc->unterminated_message)
        {
            buf2 = (char *) malloc (strlen (ptr_dcc->unterminated_message) +
                strlen (buffer) + 1);
            if (buf2)
            {
                strcpy (buf2, ptr_dcc->unterminated_message);
                strcat (buf2, buffer);
            }
            ptr_buf = buf2;
            free (ptr_dcc->unterminated_message);
            ptr_dcc->unterminated_message = NULL;
        }
        
        while (ptr_buf && ptr_buf[0])
        {
            next_ptr_buf = NULL;
            pos = strstr (ptr_buf, "\r\n");
            if (pos)
            {
                pos[0] = '\0';
                next_ptr_buf = pos + 2;
            }
            else
            {
                pos = strstr (ptr_buf, "\n");
                if (pos)
                {
                    pos[0] = '\0';
                    next_ptr_buf = pos + 1;
                }
                else
                {
                    ptr_dcc->unterminated_message = strdup (ptr_buf);
                    ptr_buf = NULL;
                    next_ptr_buf = NULL;
                }
            }
            
            if (ptr_buf)
            {
                if (irc_protocol_is_highlight (ptr_buf, ptr_dcc->server->nick))
                {
                    irc_display_nick (ptr_dcc->channel->buffer, NULL,
                                      ptr_dcc->nick,
                                      GUI_MSG_TYPE_NICK | GUI_MSG_TYPE_HIGHLIGHT,
                                      1,
                                      GUI_COLOR(GUI_COLOR_CHAT_HIGHLIGHT), 0);
                    if ((cfg_look_infobar_delay_highlight > 0)
                        && (ptr_dcc->channel->buffer != gui_current_window->buffer))
                    {
                        gui_infobar_printf (cfg_look_infobar_delay_highlight,
                                            GUI_COLOR_INFOBAR_HIGHLIGHT,
                                            _("Private %s> %s"),
                                            ptr_dcc->nick, ptr_buf);
                    }
                }
                else
                    irc_display_nick (ptr_dcc->channel->buffer, NULL,
                                      ptr_dcc->nick,
                                      GUI_MSG_TYPE_NICK, 1,
                                      GUI_COLOR(GUI_COLOR_CHAT_NICK_OTHER), 0);
                gui_chat_printf_type (ptr_dcc->channel->buffer,
                                      GUI_MSG_TYPE_MSG,
                                      NULL, -1,
                                      "%s%s\n",
                                      GUI_COLOR(GUI_COLOR_CHAT),
                                      ptr_buf);
            }
            
            ptr_buf = next_ptr_buf;
        }
        
        if (buf2)
            free (buf2);
    }
    else
    {
        irc_dcc_close (ptr_dcc, IRC_DCC_ABORTED);
        irc_dcc_redraw (GUI_HOTLIST_MSG);
    }
}

/*
 * irc_dcc_file_create_pipe: create pipe for communication with child process
 *                           return 1 if ok, 0 if error
 */

int
irc_dcc_file_create_pipe (struct t_irc_dcc *ptr_dcc)
{
    int child_pipe[2];
    
    if (pipe (child_pipe) < 0)
    {
        gui_chat_printf_error (ptr_dcc->server->buffer,
                               _("%s DCC: unable to create pipe\n"),
                               WEECHAT_ERROR);
        irc_dcc_close (ptr_dcc, IRC_DCC_FAILED);
        irc_dcc_redraw (GUI_HOTLIST_MSG);
        return 0;
    }
    
    ptr_dcc->child_read = child_pipe[0];
    ptr_dcc->child_write = child_pipe[1];
    return 1;
}

/*
 * irc_dcc_file_write_pipe: write data into pipe
 */

void
irc_dcc_file_write_pipe (struct t_irc_dcc *ptr_dcc, int status, int error)
{
    char buffer[1 + 1 + 12 + 1];   /* status + error + pos + \0 */

    snprintf (buffer, sizeof (buffer), "%c%c%012lu",
              status + '0', error + '0', ptr_dcc->pos);
    write (ptr_dcc->child_write, buffer, sizeof (buffer));
}

/*
 * irc_dcc_file_send_child: child process for sending file
 */

void
irc_dcc_file_send_child (struct t_irc_dcc *ptr_dcc)
{
    int num_read, num_sent;
    static char buffer[IRC_DCC_MAX_BLOCKSIZE];
    uint32_t ack;
    time_t last_sent, new_time;
    
    last_sent = time (NULL);
    while (1)
    {
        /* read DCC ACK (sent by receiver) */
        if (ptr_dcc->pos > ptr_dcc->ack)
        {
            /* we should receive ACK for packets sent previously */
            while (1)
            {
                num_read = recv (ptr_dcc->sock, (char *) &ack, 4, MSG_PEEK);
                if ((num_read < 1) &&
                    ((num_read != -1) || (errno != EAGAIN)))
                {
                    irc_dcc_file_write_pipe (ptr_dcc, IRC_DCC_FAILED, IRC_DCC_ERROR_SEND_BLOCK);
                    return;
                }
                if (num_read == 4)
                {
                    recv (ptr_dcc->sock, (char *) &ack, 4, 0);
                    ptr_dcc->ack = ntohl (ack);
                    
                    /* DCC send ok? */
                    if ((ptr_dcc->pos >= ptr_dcc->size)
                        && (ptr_dcc->ack >= ptr_dcc->size))
                    {
                        irc_dcc_file_write_pipe (ptr_dcc, IRC_DCC_DONE, IRC_DCC_NO_ERROR);
                        return;
                    }
                }
                else
                    break;
            }
        }
        
        /* send a block to receiver */
        if ((ptr_dcc->pos < ptr_dcc->size) &&
             (ptr_dcc->fast_send || (ptr_dcc->pos <= ptr_dcc->ack)))
        {
            lseek (ptr_dcc->file, ptr_dcc->pos, SEEK_SET);
            num_read = read (ptr_dcc->file, buffer, ptr_dcc->blocksize);
            if (num_read < 1)
            {
                irc_dcc_file_write_pipe (ptr_dcc, IRC_DCC_FAILED, IRC_DCC_ERROR_READ_LOCAL);
                return;
            }
            num_sent = send (ptr_dcc->sock, buffer, num_read, 0);
            if (num_sent < 0)
            {
                /* socket is temporarily not available (receiver can't receive
                   amount of data we sent ?!) */
                if (errno == EAGAIN)
                    usleep (1000);
                else
                {
                    irc_dcc_file_write_pipe (ptr_dcc, IRC_DCC_FAILED, IRC_DCC_ERROR_SEND_BLOCK);
                    return;
                }
            }
            if (num_sent > 0)
            {
                ptr_dcc->pos += (unsigned long) num_sent;
                new_time = time (NULL);
                if (last_sent != new_time)
                {
                    last_sent = new_time;
                    irc_dcc_file_write_pipe (ptr_dcc, IRC_DCC_ACTIVE, IRC_DCC_NO_ERROR);
                }
            }
        }
        else
            usleep (1000);
    }
}

/*
 * irc_dcc_file_recv_child: child process for receiving file
 */

void
irc_dcc_file_recv_child (struct t_irc_dcc *ptr_dcc)
{
    int num_read;
    static char buffer[IRC_DCC_MAX_BLOCKSIZE];
    uint32_t pos;
    time_t last_sent, new_time;
    
    /* first connect to sender (blocking) */
    if (!irc_dcc_connect_to_sender (ptr_dcc))
    {
        irc_dcc_file_write_pipe (ptr_dcc, IRC_DCC_FAILED, IRC_DCC_ERROR_CONNECT_SENDER);
        return;
    }

    /* connection is ok, change DCC status (inform parent process) */
    irc_dcc_file_write_pipe (ptr_dcc, IRC_DCC_ACTIVE, IRC_DCC_NO_ERROR);
    
    last_sent = time (NULL);
    while (1)
    {    
        num_read = recv (ptr_dcc->sock, buffer, sizeof (buffer), 0);
        if (num_read == -1)
        {
            /* socket is temporarily not available (sender is not fast ?!) */
            if (errno == EAGAIN)
                usleep (1000);
            else
            {
                irc_dcc_file_write_pipe (ptr_dcc, IRC_DCC_FAILED, IRC_DCC_ERROR_RECV_BLOCK);
                return;
            }
        }
        else
        {
            if (num_read == 0)
            {
                irc_dcc_file_write_pipe (ptr_dcc, IRC_DCC_FAILED, IRC_DCC_ERROR_RECV_BLOCK);
                return;
            }
            
            if (write (ptr_dcc->file, buffer, num_read) == -1)
            {
                irc_dcc_file_write_pipe (ptr_dcc, IRC_DCC_FAILED, IRC_DCC_ERROR_WRITE_LOCAL);
                return;
            }
            
            ptr_dcc->pos += (unsigned long) num_read;
            pos = htonl (ptr_dcc->pos);
            
            /* we don't check return code, not a problem if an ACK send failed */
            send (ptr_dcc->sock, (char *) &pos, 4, 0);

            /* file received ok? */
            if (ptr_dcc->pos >= ptr_dcc->size)
            {
                irc_dcc_file_write_pipe (ptr_dcc, IRC_DCC_DONE, IRC_DCC_NO_ERROR);
                return;
            }
            
            new_time = time (NULL);
            if (last_sent != new_time)
            {
                last_sent = new_time;
                irc_dcc_file_write_pipe (ptr_dcc, IRC_DCC_ACTIVE, IRC_DCC_NO_ERROR);
            }
        }
    }
}

/*
 * irc_dcc_file_child_read: read data from child via pipe
 */

void
irc_dcc_file_child_read (struct t_irc_dcc *ptr_dcc)
{
    fd_set read_fd;
    static struct timeval timeout;
    char bufpipe[1 + 1 + 12 + 1];
    int num_read;
    char *error;
    
    FD_ZERO (&read_fd);
    FD_SET (ptr_dcc->child_read, &read_fd);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    /* something to read on child pipe? */
    if (select (FD_SETSIZE, &read_fd, NULL, NULL, &timeout) <= 0)
        return;

    if (!FD_ISSET (ptr_dcc->child_read, &read_fd))
        return;

    /* there's something to read in pipe! */
    num_read = read (ptr_dcc->child_read, bufpipe, sizeof (bufpipe));
    if (num_read > 0)
    {
        error = NULL;
        ptr_dcc->pos = strtol (bufpipe + 2, &error, 10);
        ptr_dcc->last_activity = time (NULL);
        irc_dcc_calculate_speed (ptr_dcc, 0);
        
        /* read error code */
        switch (bufpipe[1] - '0')
        {
            /* errors for sender */
            case IRC_DCC_ERROR_READ_LOCAL:
                gui_chat_printf_error (ptr_dcc->server->buffer,
                                       _("%s DCC: unable to read local "
                                         "file\n"),
                                       WEECHAT_ERROR);
                break;
            case IRC_DCC_ERROR_SEND_BLOCK:
                gui_chat_printf_error (ptr_dcc->server->buffer,
                                       _("%s DCC: unable to send block to "
                                         "receiver\n"),
                                       WEECHAT_ERROR);
                break;
            case IRC_DCC_ERROR_READ_ACK:
                gui_chat_printf_error (ptr_dcc->server->buffer,
                                       _("%s DCC: unable to read ACK from "
                                         "receiver\n"),
                                       WEECHAT_ERROR);
                break;
            /* errors for receiver */
            case IRC_DCC_ERROR_CONNECT_SENDER:
                gui_chat_printf_error (ptr_dcc->server->buffer,
                                       _("%s DCC: unable to connect to "
                                         "sender\n"),
                                       WEECHAT_ERROR);
                break;
            case IRC_DCC_ERROR_RECV_BLOCK:
                gui_chat_printf_error (ptr_dcc->server->buffer,
                                       _("%s DCC: unable to receive block "
                                         "from sender\n"),
                                       WEECHAT_ERROR);
                break;
            case IRC_DCC_ERROR_WRITE_LOCAL:
                gui_chat_printf_error (ptr_dcc->server->buffer,
                                       _("%s DCC: unable to write local "
                                         "file\n"),
                                       WEECHAT_ERROR);
                break;
        }
        
        /* read new DCC status */
        switch (bufpipe[0] - '0')
        {
            case IRC_DCC_ACTIVE:
                if (ptr_dcc->status == IRC_DCC_CONNECTING)
                {
                    /* connection was successful by child, init transfert times */
                    ptr_dcc->status = IRC_DCC_ACTIVE;
                    ptr_dcc->start_transfer = time (NULL);
                    ptr_dcc->last_check_time = time (NULL);
                    irc_dcc_redraw (GUI_HOTLIST_MSG);
                }
                else
                    irc_dcc_redraw (GUI_HOTLIST_LOW);
                break;
            case IRC_DCC_DONE:
                irc_dcc_close (ptr_dcc, IRC_DCC_DONE);
                irc_dcc_redraw (GUI_HOTLIST_MSG);
                break;
            case IRC_DCC_FAILED:
                irc_dcc_close (ptr_dcc, IRC_DCC_FAILED);
                irc_dcc_redraw (GUI_HOTLIST_MSG);
                break;
        }
    }
}

/*
 * irc_dcc_file_send_fork: fork process for sending file
 */

void
irc_dcc_file_send_fork (struct t_irc_dcc *ptr_dcc)
{
    pid_t pid;
    
    if (!irc_dcc_file_create_pipe (ptr_dcc))
        return;

    ptr_dcc->file = open (ptr_dcc->local_filename, O_RDONLY | O_NONBLOCK, 0644);
    
    switch (pid = fork ())
    {
        /* fork failed */
        case -1:
            gui_chat_printf_error (ptr_dcc->server->buffer,
                                   _("%s DCC: unable to fork\n"),
                                   WEECHAT_ERROR);
            irc_dcc_close (ptr_dcc, IRC_DCC_FAILED);
            irc_dcc_redraw (GUI_HOTLIST_MSG);
            return;
            /* child process */
        case 0:
            setuid (getuid ());
            irc_dcc_file_send_child (ptr_dcc);
            _exit (EXIT_SUCCESS);
    }
    
    /* parent process */
    ptr_dcc->child_pid = pid;
}

/*
 * irc_dcc_file_recv_fork: fork process for receiving file
 */

void
irc_dcc_file_recv_fork (struct t_irc_dcc *ptr_dcc)
{
    pid_t pid;
    
    if (!irc_dcc_file_create_pipe (ptr_dcc))
        return;
    
    if (ptr_dcc->start_resume > 0)
        ptr_dcc->file = open (ptr_dcc->local_filename,
                              O_APPEND | O_WRONLY | O_NONBLOCK);
    else
        ptr_dcc->file = open (ptr_dcc->local_filename,
                              O_CREAT | O_TRUNC | O_WRONLY | O_NONBLOCK,
                              0644);
    
    switch (pid = fork ())
    {
        /* fork failed */
        case -1:
            gui_chat_printf_error (ptr_dcc->server->buffer,
                                   _("%s DCC: unable to fork\n"),
                                   WEECHAT_ERROR);
            irc_dcc_close (ptr_dcc, IRC_DCC_FAILED);
            irc_dcc_redraw (GUI_HOTLIST_MSG);
            return;
            /* child process */
        case 0:
            setuid (getuid ());
            irc_dcc_file_recv_child (ptr_dcc);
            _exit (EXIT_SUCCESS);
    }
    
    /* parent process */
    ptr_dcc->child_pid = pid;
}

/*
 * irc_dcc_handle: receive/send data for all active DCC
 */

void
irc_dcc_handle ()
{
    struct t_irc_dcc *ptr_dcc;
    fd_set read_fd;
    static struct timeval timeout;
    int sock;
    struct sockaddr_in addr;
    socklen_t length;
    
    for (ptr_dcc = irc_dcc_list; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
    {
        /* check DCC timeout */
        if (IRC_DCC_IS_FILE(ptr_dcc->type) && !IRC_DCC_ENDED(ptr_dcc->status))
        {
            if ((irc_cfg_dcc_timeout != 0)
                && (time (NULL) > ptr_dcc->last_activity + irc_cfg_dcc_timeout))
            {
                gui_chat_printf_error (ptr_dcc->server->buffer,
                                       _("%s DCC: timeout\n"),
                                       WEECHAT_ERROR);
                irc_dcc_close (ptr_dcc, IRC_DCC_FAILED);
                irc_dcc_redraw (GUI_HOTLIST_MSG);
                continue;
            }
        }
        
        if (ptr_dcc->status == IRC_DCC_CONNECTING)
        {
            if (ptr_dcc->type == IRC_DCC_FILE_SEND)
            {
                FD_ZERO (&read_fd);
                FD_SET (ptr_dcc->sock, &read_fd);
                timeout.tv_sec = 0;
                timeout.tv_usec = 0;
                
                /* something to read on socket? */
                if (select (FD_SETSIZE, &read_fd, NULL, NULL, &timeout) > 0)
                {
                    if (FD_ISSET (ptr_dcc->sock, &read_fd))
                    {
                        ptr_dcc->last_activity = time (NULL);
                        length = sizeof (addr);
                        sock = accept (ptr_dcc->sock,
                                       (struct sockaddr *) &addr, &length);
                        close (ptr_dcc->sock);
                        ptr_dcc->sock = -1;
                        if (sock < 0)
                        {
                            gui_chat_printf_error (ptr_dcc->server->buffer,
                                                   _("%s DCC: unable to "
                                                     "create socket for "
                                                     "sending file\n"),
                                                   WEECHAT_ERROR);
                            irc_dcc_close (ptr_dcc, IRC_DCC_FAILED);
                            irc_dcc_redraw (GUI_HOTLIST_MSG);
                            continue;
                        }
                        ptr_dcc->sock = sock;
                        if (fcntl (ptr_dcc->sock, F_SETFL, O_NONBLOCK) == -1)
                        {
                            gui_chat_printf_error (ptr_dcc->server->buffer,
                                                   _("%s DCC: unable to set "
                                                     "'nonblock' option for "
                                                     "socket\n"),
                                                   WEECHAT_ERROR);
                            irc_dcc_close (ptr_dcc, IRC_DCC_FAILED);
                            irc_dcc_redraw (GUI_HOTLIST_MSG);
                            continue;
                        }
                        ptr_dcc->addr = ntohl (addr.sin_addr.s_addr);
                        ptr_dcc->status = IRC_DCC_ACTIVE;
                        ptr_dcc->start_transfer = time (NULL);
                        irc_dcc_redraw (GUI_HOTLIST_MSG);
                        irc_dcc_file_send_fork (ptr_dcc);
                    }
                }
            }
            if (ptr_dcc->type == IRC_DCC_FILE_RECV)
            {
                if (ptr_dcc->child_read != -1)
                    irc_dcc_file_child_read (ptr_dcc);
            }
        }
        
        if (ptr_dcc->status == IRC_DCC_WAITING)
        {
            if (ptr_dcc->type == IRC_DCC_CHAT_SEND)
            {
                FD_ZERO (&read_fd);
                FD_SET (ptr_dcc->sock, &read_fd);
                timeout.tv_sec = 0;
                timeout.tv_usec = 0;
                
                /* something to read on socket? */
                if (select (FD_SETSIZE, &read_fd, NULL, NULL, &timeout) > 0)
                {
                    if (FD_ISSET (ptr_dcc->sock, &read_fd))
                    {
                        length = sizeof (addr);
                        sock = accept (ptr_dcc->sock, (struct sockaddr *) &addr, &length);
                        close (ptr_dcc->sock);
                        ptr_dcc->sock = -1;
                        if (sock < 0)
                        {
                            irc_dcc_close (ptr_dcc, IRC_DCC_FAILED);
                            irc_dcc_redraw (GUI_HOTLIST_MSG);
                            continue;
                        }
                        ptr_dcc->sock = sock;
                        if (fcntl (ptr_dcc->sock, F_SETFL, O_NONBLOCK) == -1)
                        {
                            irc_dcc_close (ptr_dcc, IRC_DCC_FAILED);
                            irc_dcc_redraw (GUI_HOTLIST_MSG);
                            continue;
                        }
                        ptr_dcc->addr = ntohl (addr.sin_addr.s_addr);
                        ptr_dcc->status = IRC_DCC_ACTIVE;
                        irc_dcc_redraw (GUI_HOTLIST_MSG);
                        irc_dcc_channel_for_chat (ptr_dcc);
                    }
                }
            }
        }
        
        if (ptr_dcc->status == IRC_DCC_ACTIVE)
        {
            if (IRC_DCC_IS_CHAT(ptr_dcc->type))
                irc_dcc_chat_recv (ptr_dcc);
            else
                irc_dcc_file_child_read (ptr_dcc);
        }
    }
}

/*
 * irc_dcc_end: close all opened sockets (called when WeeChat is exiting)
 */

void
irc_dcc_end ()
{
    struct t_irc_dcc *ptr_dcc;
    
    for (ptr_dcc = irc_dcc_list; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
    {
        if (ptr_dcc->sock >= 0)
        {
            if (ptr_dcc->status == IRC_DCC_ACTIVE)
                weechat_log_printf (_("Aborting active DCC: \"%s\" from %s"),
                                    ptr_dcc->filename, ptr_dcc->nick);
            irc_dcc_close (ptr_dcc, IRC_DCC_FAILED);
        }
    }
}

/*
 * irc_dcc_print_log: print DCC infos in log (usually for crash dump)
 */

void
irc_dcc_print_log ()
{
    struct t_irc_dcc *ptr_dcc;
    
    for (ptr_dcc = irc_dcc_list; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[DCC (addr:0x%X)]", ptr_dcc);
        weechat_log_printf ("  server. . . . . . . : 0x%X", ptr_dcc->server);
        weechat_log_printf ("  channel . . . . . . : 0x%X", ptr_dcc->channel);
        weechat_log_printf ("  type. . . . . . . . : %d",   ptr_dcc->type);
        weechat_log_printf ("  status. . . . . . . : %d",   ptr_dcc->status);
        weechat_log_printf ("  start_time. . . . . : %ld",  ptr_dcc->start_time);
        weechat_log_printf ("  start_transfer. . . : %ld",  ptr_dcc->start_transfer);
        weechat_log_printf ("  addr. . . . . . . . : %lu",  ptr_dcc->addr);
        weechat_log_printf ("  port. . . . . . . . : %d",   ptr_dcc->port);
        weechat_log_printf ("  nick. . . . . . . . : '%s'", ptr_dcc->nick);
        weechat_log_printf ("  sock. . . . . . . . : %d",   ptr_dcc->sock);
        weechat_log_printf ("  child_pid . . . . . : %d",   ptr_dcc->child_pid);
        weechat_log_printf ("  child_read. . . . . : %d",   ptr_dcc->child_read);
        weechat_log_printf ("  child_write . . . . : %d",   ptr_dcc->child_write);
        weechat_log_printf ("  unterminated_message: '%s'", ptr_dcc->unterminated_message);
        weechat_log_printf ("  fast_send . . . . . : %d",   ptr_dcc->fast_send);
        weechat_log_printf ("  file. . . . . . . . : %d",   ptr_dcc->file);
        weechat_log_printf ("  filename. . . . . . : '%s'", ptr_dcc->filename);
        weechat_log_printf ("  local_filename. . . : '%s'", ptr_dcc->local_filename);
        weechat_log_printf ("  filename_suffix . . : %d",   ptr_dcc->filename_suffix);
        weechat_log_printf ("  blocksize . . . . . : %d",   ptr_dcc->blocksize);
        weechat_log_printf ("  size. . . . . . . . : %lu",  ptr_dcc->size);
        weechat_log_printf ("  pos . . . . . . . . : %lu",  ptr_dcc->pos);
        weechat_log_printf ("  ack . . . . . . . . : %lu",  ptr_dcc->ack);
        weechat_log_printf ("  start_resume. . . . : %lu",  ptr_dcc->start_resume);
        weechat_log_printf ("  last_check_time . . : %ld",  ptr_dcc->last_check_time);
        weechat_log_printf ("  last_check_pos. . . : %lu",  ptr_dcc->last_check_pos);
        weechat_log_printf ("  last_activity . . . : %ld",  ptr_dcc->last_activity);
        weechat_log_printf ("  bytes_per_sec . . . : %lu",  ptr_dcc->bytes_per_sec);
        weechat_log_printf ("  eta . . . . . . . . : %lu",  ptr_dcc->eta);
        weechat_log_printf ("  prev_dcc. . . . . . : 0x%X", ptr_dcc->prev_dcc);
        weechat_log_printf ("  next_dcc. . . . . . : 0x%X", ptr_dcc->next_dcc);
    }
}
