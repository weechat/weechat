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

/* irc-core.c: main IRC functions */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

#include "irc.h"
#include "irc-command.h"
#include "irc-config.h"
#include "irc-server.h"


char plugin_name[] = "irc";
char plugin_version[]     = "0.1";
char plugin_description[] = "IRC (Internet Relay Chat)";

struct t_weechat_plugin *weechat_irc_plugin = NULL;

struct t_hook *irc_timer = NULL;
struct t_hook *irc_timer_check_away = NULL;

#ifdef HAVE_GNUTLS
gnutls_certificate_credentials gnutls_xcred; /* gnutls client credentials */
#endif


/*
 * irc_dump: dump IRC data in WeeChat log file
 */

/*int
irc_dump ()
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        weechat_log_printf ("");
        irc_server_print_log (ptr_server);
        
        for (ptr_channel = ptr_server->channels; ptr_channel;
            ptr_channel = ptr_channel->next_channel)
        {
            weechat_log_printf ("");
            irc_channel_print_log (ptr_channel);
            
            for (ptr_nick = ptr_channel->nicks; ptr_nick;
                ptr_nick = ptr_nick->next_nick)
            {
                weechat_log_printf ("");
                irc_nick_print_log (ptr_nick);
            }
        }
    }
    
    irc_dcc_print_log ();
    
    return PLUGIN_RC_SUCCESS;
}
*/

/*
 * irc_create_directories: create directories for IRC plugin
 */

void
irc_create_directories ()
{
    char *weechat_dir, *dir1, *dir2;
    
    /* create DCC download directory */
    weechat_dir = weechat_info_get ("weechat_dir");
    if (weechat_dir)
    {
        dir1 = weechat_string_replace (weechat_config_string (irc_config_dcc_download_path),
                                       "~", getenv ("HOME"));
        dir2 = weechat_string_replace (dir1, "%h", weechat_dir);
        if (dir2)
            (void) weechat_mkdir (dir2, 0700);
        if (dir1)
            free (dir1);
        if (dir2)
            free (dir2);
        free (weechat_dir);
    }
}

/*
 * irc_quit_cb: callback for event "quit"
 */

int
irc_quit_cb (void *data, char *event, void *pointer)
{
    struct t_irc_server *ptr_server;
    
    /* make C compiler happy */
    (void) data;
    (void) event;
    (void) pointer;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        irc_command_quit_server (ptr_server, (char *)pointer);
    }
    
    return PLUGIN_RC_SUCCESS;
}


/*
 * weechat_plugin_init: initialize IRC plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin)
{
    weechat_plugin = plugin;
    
#ifdef HAVE_GNUTLS
    /* init GnuTLS */
    gnutls_global_init ();
    gnutls_certificate_allocate_credentials (&gnutls_xcred);
    gnutls_certificate_set_x509_trust_file (gnutls_xcred, "ca.pem", GNUTLS_X509_FMT_PEM);
#endif
    
    if (!irc_config_init ())
        return PLUGIN_RC_FAILED;

    if (irc_config_read () < 0)
        return PLUGIN_RC_FAILED;

    irc_create_directories ();

    irc_command_init ();

    weechat_hook_event ("config_reload", &irc_config_reload_cb, NULL);

    weechat_hook_event ("quit", &irc_quit_cb, NULL);
    
    //irc_server_auto_connect (1, 0);

    /*
    irc_timer = weechat_hook_timer (1 * 1000, 0,
                                    &irc_server_timer,
                                    NULL);
    if (irc_cfg_irc_away_check != 0)
        irc_timer_check_away = weechat_hook_timer (irc_cfg_irc_away_check * 60 * 1000,
                                                   0,
                                                   &irc_server_timer_check_away,
                                                   NULL);
    */
    
    return PLUGIN_RC_SUCCESS;
}

/*
 * weechat_plugin_end: end IRC plugin
 */

int
weechat_plugin_end ()
{
    if (irc_timer)
    {
        weechat_unhook (irc_timer);
        irc_timer = NULL;
    }
    if (irc_timer_check_away)
    {
        weechat_unhook (irc_timer_check_away);
        irc_timer_check_away = NULL;
    }
    
    //irc_server_disconnect_all ();
    //irc_dcc_end ();
    //irc_server_free_all ();
    
    irc_config_write ();

#ifdef HAVE_GNUTLS
    /* GnuTLS end */
    gnutls_certificate_free_credentials (gnutls_xcred);
    gnutls_global_deinit();
#endif
    
    return PLUGIN_RC_SUCCESS;
}
