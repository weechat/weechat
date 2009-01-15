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

/* jabber-info.c: info and infolist hooks for Jabber plugin */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "jabber.h"
#include "jabber-buddy.h"
#include "jabber-muc.h"
#include "jabber-server.h"


/*
 * jabber_info_create_string_with_pointer: create a string with a pointer inside
 *                                         a Jabber structure
 */

void
jabber_info_create_string_with_pointer (char **string, void *pointer)
{
    if (*string)
    {
        free (*string);
        *string = NULL;
    }
    if (pointer)
    {
        *string = malloc (64);
        if (*string)
        {
            snprintf (*string, 64 - 1, "0x%lx", (long unsigned int)pointer);
        }
    }
}

/*
 * jabber_info_get_info_cb: callback called when Jabber info is asked
 */

const char *
jabber_info_get_info_cb (void *data, const char *info_name,
                         const char *arguments)
{
    char *pos_comma, *pos_comma2, *server, *muc, *host;
    struct t_jabber_server *ptr_server;
    struct t_jabber_muc *ptr_muc;
    
    /* make C compiler happy */
    (void) data;
    
    if (weechat_strcasecmp (info_name, "jabber_buffer") == 0)
    {
        if (arguments && arguments[0])
        {
            server = NULL;
            muc = NULL;
            host = NULL;
            ptr_server = NULL;
            ptr_muc = NULL;
            
            pos_comma = strchr (arguments, ',');
            if (pos_comma)
            {
                server = weechat_strndup (arguments, pos_comma - arguments);
                pos_comma2 = strchr (pos_comma + 1, ',');
                if (pos_comma2)
                {
                    muc = weechat_strndup (pos_comma + 1,
                                           pos_comma2 - pos_comma - 1);
                    host = strdup (pos_comma2 + 1);
                }
                else
                    muc = strdup (pos_comma + 1);
            }
            
            /* replace MUC by buddy in host if MUC is not a MUC (private ?) */
            if (muc && host)
            {
                //if (!jabber_muc_is_muc (muc))
                //{
                //    free (muc);
                //    muc = NULL;
                //    buddy = jabber_xmpp_get_buddy_from_host (host);
                //    if (buddy)
                //        muc = strdup (buddy);
                //}
            }
            
            /* search for server or MUC buffer */
            if (server)
            {
                ptr_server = jabber_server_search (server);
                if (ptr_server && muc)
                    ptr_muc = jabber_muc_search (ptr_server, muc);
            }
            
            if (server)
                free (server);
            if (muc)
                free (muc);
            if (host)
                free (host);
            
            if (ptr_muc)
            {
                jabber_info_create_string_with_pointer (&ptr_muc->buffer_as_string,
                                                        ptr_muc->buffer);
                return ptr_muc->buffer_as_string;
            }
            if (ptr_server)
            {
                jabber_info_create_string_with_pointer (&ptr_server->buffer_as_string,
                                                        ptr_server->buffer);
                return ptr_server->buffer_as_string;
            }
        }
    }
    
    return NULL;
}

/*
 * jabber_info_get_infolist_cb: callback called when Jabber infolist is asked
 */

struct t_infolist *
jabber_info_get_infolist_cb (void *data, const char *infolist_name,
                             void *pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_jabber_server *ptr_server;
    struct t_jabber_muc *ptr_muc;
    struct t_jabber_buddy *ptr_buddy;
    char *pos_comma, *server_name;
    
    /* make C compiler happy */
    (void) data;
    
    if (!infolist_name || !infolist_name[0])
        return NULL;
    
    if (weechat_strcasecmp (infolist_name, "jabber_server") == 0)
    {
        if (pointer && !jabber_server_valid (pointer))
            return NULL;
        
        ptr_infolist = weechat_infolist_new ();
        if (ptr_infolist)
        {
            if (pointer)
            {
                /* build list with only one server */
                if (!jabber_server_add_to_infolist (ptr_infolist, pointer))
                {
                    weechat_infolist_free (ptr_infolist);
                    return NULL;
                }
                return ptr_infolist;
            }
            else
            {
                /* build list with all servers */
                for (ptr_server = jabber_servers; ptr_server;
                     ptr_server = ptr_server->next_server)
                {
                    if (!jabber_server_add_to_infolist (ptr_infolist, ptr_server))
                    {
                        weechat_infolist_free (ptr_infolist);
                        return NULL;
                    }
                }
                return ptr_infolist;
            }
        }
    }
    else if (weechat_strcasecmp (infolist_name, "jabber_muc") == 0)
    {
        if (arguments && arguments[0])
        {
            ptr_server = jabber_server_search (arguments);
            if (ptr_server)
            {
                if (pointer && !jabber_muc_valid (ptr_server, pointer))
                    return NULL;
                
                ptr_infolist = weechat_infolist_new ();
                if (ptr_infolist)
                {
                    if (pointer)
                    {
                        /* build list with only one MUC */
                        if (!jabber_muc_add_to_infolist (ptr_infolist, pointer))
                        {
                            weechat_infolist_free (ptr_infolist);
                            return NULL;
                        }
                        return ptr_infolist;
                    }
                    else
                    {
                        /* build list with all MUCs of server */
                        for (ptr_muc = ptr_server->mucs; ptr_muc;
                             ptr_muc = ptr_muc->next_muc)
                        {
                            if (!jabber_muc_add_to_infolist (ptr_infolist,
                                                             ptr_muc))
                            {
                                weechat_infolist_free (ptr_infolist);
                                return NULL;
                            }
                        }
                        return ptr_infolist;
                    }
                }
            }
        }
    }
    else if (weechat_strcasecmp (infolist_name, "jabber_buddy") == 0)
    {
        if (arguments && arguments[0])
        {
            ptr_server = NULL;
            ptr_muc = NULL;
            pos_comma = strchr (arguments, ',');
            if (pos_comma)
            {
                server_name = weechat_strndup (arguments, pos_comma - arguments);
                if (server_name)
                {
                    ptr_server = jabber_server_search (server_name);
                    if (ptr_server)
                    {
                        ptr_muc = jabber_muc_search (ptr_server,
                                                     pos_comma + 1);
                    }
                    free (server_name);
                }
            }
            if (ptr_muc)
            {
                if (pointer && !jabber_buddy_valid (NULL, ptr_muc, pointer))
                    return NULL;
                
                ptr_infolist = weechat_infolist_new ();
                if (ptr_infolist)
                {
                    if (pointer)
                    {
                        /* build list with only one buddy */
                        if (!jabber_buddy_add_to_infolist (ptr_infolist, pointer))
                        {
                            weechat_infolist_free (ptr_infolist);
                            return NULL;
                        }
                        return ptr_infolist;
                    }
                    else
                    {
                        /* build list with all buddies of MUC */
                        for (ptr_buddy = ptr_muc->buddies; ptr_buddy;
                             ptr_buddy = ptr_buddy->next_buddy)
                        {
                            if (!jabber_buddy_add_to_infolist (ptr_infolist,
                                                               ptr_buddy))
                            {
                                weechat_infolist_free (ptr_infolist);
                                return NULL;
                            }
                        }
                        return ptr_infolist;
                    }
                }
            }
            else if (ptr_server)
            {
                if (pointer && !jabber_buddy_valid (ptr_server, NULL, pointer))
                    return NULL;
                
                ptr_infolist = weechat_infolist_new ();
                if (ptr_infolist)
                {
                    if (pointer)
                    {
                        /* build list with only one buddy */
                        if (!jabber_buddy_add_to_infolist (ptr_infolist, pointer))
                        {
                            weechat_infolist_free (ptr_infolist);
                            return NULL;
                        }
                        return ptr_infolist;
                    }
                    else
                    {
                        /* build list with all buddies of server */
                        for (ptr_buddy = ptr_server->buddies; ptr_buddy;
                             ptr_buddy = ptr_buddy->next_buddy)
                        {
                            if (!jabber_buddy_add_to_infolist (ptr_infolist,
                                                               ptr_buddy))
                            {
                                weechat_infolist_free (ptr_infolist);
                                return NULL;
                            }
                        }
                        return ptr_infolist;
                    }
                }
            }
        }
    }
    
    return NULL;
}

/*
 * jabber_info_init: initialize info and infolist hooks for Jabber plugin
 */

void
jabber_info_init ()
{
    /* info hooks */
    weechat_hook_info ("jabber_buffer",
                       N_("get buffer pointer for a Jabber server/MUC"),
                       &jabber_info_get_info_cb, NULL);
    
    /* infolist hooks */
    weechat_hook_infolist ("jabber_server",
                           N_("list of Jabber servers"),
                           &jabber_info_get_infolist_cb, NULL);
    weechat_hook_infolist ("jabber_muc",
                           N_("list of MUCs for a Jabber server"),
                           &jabber_info_get_infolist_cb, NULL);
    weechat_hook_infolist ("jabber_buddy",
                           N_("list of buddies for a Jabber server or MUC"),
                           &jabber_info_get_infolist_cb, NULL);
}
