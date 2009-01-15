/*
 * Copyright (c) 2009 by FlashCode <flashcode@flashtux.org>
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

/* jabber-completion.c: completion for Jabber commands */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "jabber.h"
#include "jabber-buddy.h"
#include "jabber-buffer.h"
#include "jabber-completion.h"
#include "jabber-config.h"
#include "jabber-muc.h"
#include "jabber-server.h"


/*
 * jabber_completion_server_cb: callback for completion with current server
 */

int
jabber_completion_server_cb (void *data, const char *completion_item,
                             struct t_gui_buffer *buffer,
                             struct t_gui_completion *completion)
{
    JABBER_GET_SERVER(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    
    if (ptr_server)
    {
        weechat_hook_completion_list_add (completion, ptr_server->name,
                                          0, WEECHAT_LIST_POS_SORT);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_completion_server_local_name_cb: callback for completion with local
 *                                         name on server
 */

int
jabber_completion_server_local_name_cb (void *data,
                                        const char *completion_item,
                                        struct t_gui_buffer *buffer,
                                        struct t_gui_completion *completion)
{
    const char *local_name;
    
    JABBER_GET_SERVER(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;

    if (ptr_server)
    {
        local_name = jabber_server_get_local_name (ptr_server);
        if (local_name && local_name[0])
        {
            weechat_hook_completion_list_add (completion, local_name,
                                              1, WEECHAT_LIST_POS_SORT);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_completion_server_buddies_cb: callback for completion with buddies
 *                                      of current server
 */

int
jabber_completion_server_buddies_cb (void *data, const char *completion_item,
                                     struct t_gui_buffer *buffer,
                                     struct t_gui_completion *completion)
{
    struct t_jabber_muc *ptr_muc2;
    struct t_jabber_buddy *ptr_buddy;
    const char *local_name;
    
    JABBER_GET_SERVER_MUC(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    
    if (ptr_server)
    {
        for (ptr_muc2 = ptr_server->mucs; ptr_muc2;
             ptr_muc2 = ptr_muc2->next_muc)
        {
            if (ptr_muc2->type == JABBER_MUC_TYPE_MUC)
            {
                for (ptr_buddy = ptr_muc2->buddies; ptr_buddy;
                     ptr_buddy = ptr_buddy->next_buddy)
                {
                    weechat_hook_completion_list_add (completion,
                                                      ptr_buddy->name,
                                                      1, WEECHAT_LIST_POS_SORT);
                }
            }
        }
        
        local_name = jabber_server_get_local_name (ptr_server);
        
        if (local_name && local_name[0])
        {
            /* add local name at the end */
            weechat_hook_completion_list_add (completion, local_name,
                                              1, WEECHAT_LIST_POS_END);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_completion_servers_cb: callback for completion with servers
 */

int
jabber_completion_servers_cb (void *data, const char *completion_item,
                           struct t_gui_buffer *buffer,
                           struct t_gui_completion *completion)
{
    struct t_jabber_server *ptr_server;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    for (ptr_server = jabber_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        weechat_hook_completion_list_add (completion, ptr_server->name,
                                          0, WEECHAT_LIST_POS_SORT);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_completion_muc_cb: callback for completion with current MUC
 */

int
jabber_completion_muc_cb (void *data, const char *completion_item,
                          struct t_gui_buffer *buffer,
                          struct t_gui_completion *completion)
{
    JABBER_GET_SERVER_MUC(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    
    if (ptr_muc)
    {
        weechat_hook_completion_list_add (completion, ptr_muc->name,
                                          0, WEECHAT_LIST_POS_SORT);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_completion_muc_buddies_cb: callback for completion with buddies
 *                                   of current MUC
 */

int
jabber_completion_muc_buddies_cb (void *data, const char *completion_item,
                                  struct t_gui_buffer *buffer,
                                  struct t_gui_completion *completion)
{
    struct t_jabber_buddy *ptr_buddy;
    const char *buddy, *local_name;
    int list_size, i, j;
    
    JABBER_GET_SERVER_MUC(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    
    if (ptr_muc)
    {
        switch (ptr_muc->type)
        {
            case JABBER_MUC_TYPE_MUC:
                for (ptr_buddy = ptr_muc->buddies; ptr_buddy;
                     ptr_buddy = ptr_buddy->next_buddy)
                {
                    weechat_hook_completion_list_add (completion,
                                                      ptr_buddy->name,
                                                      1,
                                                      WEECHAT_LIST_POS_SORT);
                }
                /* add buddies speaking recently on this MUC */
                if (weechat_config_boolean (jabber_config_look_nick_completion_smart))
                {
                    /* 0 => buddy speaking ; 1 => buddy speaking to me
                       (with highlight) */
                    for (i = 0; i < 2; i++)
                    {
                        if (ptr_muc->buddies_speaking[i])
                        {
                            list_size = weechat_list_size (ptr_muc->buddies_speaking[i]);
                            for (j = 0; j < list_size; j++)
                            {
                                buddy = weechat_list_string (weechat_list_get (ptr_muc->buddies_speaking[i], j));
                                if (buddy && jabber_buddy_search (NULL, ptr_muc, buddy))
                                {
                                    weechat_hook_completion_list_add (completion,
                                                                      buddy,
                                                                      1,
                                                                      WEECHAT_LIST_POS_BEGINNING);
                                }
                            }
                        }
                    }
                }
                /* add local name at the end */
                local_name = jabber_server_get_local_name (ptr_server);
                if (local_name && local_name[0])
                {
                    weechat_hook_completion_list_add (completion,
                                                      local_name,
                                                      1,
                                                      WEECHAT_LIST_POS_END);
                }
                break;
            case JABBER_MUC_TYPE_PRIVATE:
                weechat_hook_completion_list_add (completion,
                                                  ptr_muc->name,
                                                  0,
                                                  WEECHAT_LIST_POS_SORT);
                break;
        }
        ptr_muc->nick_completion_reset = 0;
    }
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_completion_muc_buddies_hosts_cb: callback for completion with buddies
 *                                         and hosts of current MUC
 */

int
jabber_completion_muc_buddies_hosts_cb (void *data,
                                        const char *completion_item,
                                        struct t_gui_buffer *buffer,
                                        struct t_gui_completion *completion)
{
    struct t_jabber_buddy *ptr_buddy;
    char *buf;
    int length;
    
    JABBER_GET_SERVER_MUC(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    
    if (ptr_muc)
    {
        switch (ptr_muc->type)
        {
            case JABBER_MUC_TYPE_MUC:
                for (ptr_buddy = ptr_muc->buddies; ptr_buddy;
                     ptr_buddy = ptr_buddy->next_buddy)
                {
                    weechat_hook_completion_list_add (completion,
                                                      ptr_buddy->name,
                                                      1,
                                                      WEECHAT_LIST_POS_SORT);
                    if (ptr_buddy->host)
                    {
                        length = strlen (ptr_buddy->name) + 1 +
                            strlen (ptr_buddy->host) + 1;
                        buf = malloc (length);
                        if (buf)
                        {
                            snprintf (buf, length, "%s!%s",
                                      ptr_buddy->name, ptr_buddy->host);
                            weechat_hook_completion_list_add (completion,
                                                              buf,
                                                              0,
                                                              WEECHAT_LIST_POS_SORT);
                            free (buf);
                        }
                    }
                }
                break;
            case JABBER_MUC_TYPE_PRIVATE:
                weechat_hook_completion_list_add (completion,
                                                  ptr_muc->name,
                                                  0,
                                                  WEECHAT_LIST_POS_SORT);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_completion_muc_topic_cb: callback for completion with topic of
 *                                 current MUC
 */

int
jabber_completion_muc_topic_cb (void *data, const char *completion_item,
                                struct t_gui_buffer *buffer,
                                struct t_gui_completion *completion)
{
    JABBER_GET_SERVER_MUC(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    
    if (ptr_muc && ptr_muc->topic && ptr_muc->topic[0])
    {
        weechat_hook_completion_list_add (completion,
                                          ptr_muc->topic,
                                          0,
                                          WEECHAT_LIST_POS_SORT);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_completion_mucs_cb: callback for completion with MUCs
 */

int
jabber_completion_mucs_cb (void *data, const char *completion_item,
                           struct t_gui_buffer *buffer,
                           struct t_gui_completion *completion)
{
    (void) completion;
    
    struct t_jabber_server *ptr_server;
    struct t_jabber_muc *ptr_muc;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    for (ptr_server = jabber_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        for (ptr_muc = ptr_server->mucs; ptr_muc; ptr_muc = ptr_muc->next_muc)
        {
            weechat_hook_completion_list_add (completion,
                                              ptr_muc->name,
                                              0,
                                              WEECHAT_LIST_POS_SORT);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_completion_msg_part_cb: callback for completion with default part
 *                                message
 */

int
jabber_completion_msg_part_cb (void *data, const char *completion_item,
                               struct t_gui_buffer *buffer,
                               struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    if (weechat_config_string (jabber_config_network_default_msg_part)
        && weechat_config_string (jabber_config_network_default_msg_part)[0])
    {
        weechat_hook_completion_list_add (completion,
                                          weechat_config_string (jabber_config_network_default_msg_part),
                                          0, WEECHAT_LIST_POS_SORT);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_completion_init: init completion for Jabber plugin
 */

void
jabber_completion_init ()
{
    weechat_hook_completion ("jabber_server",
                             &jabber_completion_server_cb, NULL);
    weechat_hook_completion ("jabber_server_local_name",
                             &jabber_completion_server_local_name_cb, NULL);
    weechat_hook_completion ("jabber_server_buddies",
                             &jabber_completion_server_buddies_cb, NULL);
    weechat_hook_completion ("jabber_servers",
                             &jabber_completion_servers_cb, NULL);
    weechat_hook_completion ("jabber_muc",
                             &jabber_completion_muc_cb, NULL);
    weechat_hook_completion ("buddy",
                             &jabber_completion_muc_buddies_cb, NULL);
    weechat_hook_completion ("jabber_muc_buddies_hosts",
                             &jabber_completion_muc_buddies_hosts_cb, NULL);
    weechat_hook_completion ("jabber_muc_topic",
                             &jabber_completion_muc_topic_cb, NULL);
    weechat_hook_completion ("jabber_mucs",
                             &jabber_completion_mucs_cb, NULL);
    weechat_hook_completion ("jabber_msg_part",
                             &jabber_completion_msg_part_cb, NULL);
}
