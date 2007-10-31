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

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

#include "../../core/weechat.h"
#include "irc.h"
#include "../../core/log.h"
#include "../../gui/gui.h"


char protocol_name[]        = _PROTOCOL_NAME;
char protocol_version[]     = _PROTOCOL_VERSION;
char protocol_description[] = _PROTOCOL_DESC;

t_weechat_protocol *irc_protocol;
t_weechat_hook *irc_hook_timer = NULL;
t_weechat_hook *irc_hook_timer_check_away = NULL;

#ifdef HAVE_GNUTLS
gnutls_certificate_credentials gnutls_xcred;   /* gnutls client credentials */
#endif


/*
 * weechat_protocol_init: initialize IRC protocol
 */

int
weechat_protocol_init (t_weechat_protocol *protocol)
{
    irc_protocol = protocol;
    
#ifdef HAVE_GNUTLS
    /* init GnuTLS */
    gnutls_global_init ();
    gnutls_certificate_allocate_credentials (&gnutls_xcred);
    gnutls_certificate_set_x509_trust_file (gnutls_xcred, "ca.pem", GNUTLS_X509_FMT_PEM);
#endif
    
    irc_config_read ();
    
    return PROTOCOL_RC_OK;
}

/*
 * weechat_protocol_run: run IRC protocol: auto-connect to servers
 *                       and start timers
 */

int
weechat_protocol_run ()
{
    irc_server_auto_connect (1, 0);
    
    irc_hook_timer = weechat_hook_add_timer (1 * 1000,
                                             irc_server_timer,
                                             NULL);
    if (irc_cfg_irc_away_check != 0)
        weechat_hook_add_timer (irc_cfg_irc_away_check * 60 * 1000,
                                irc_server_timer_check_away,
                                NULL);
    
    return PROTOCOL_RC_OK;
}

/*
 * weechat_protocol_input_data: read data from user input
 */

int
weechat_protocol_input_data (t_gui_window *window, char *data)
{
    return irc_input_data (window, data);
}

/*
 * weechat_protocol_config_read: read IRC configuration file
 */

int
weechat_protocol_config_read ()
{
    return irc_config_read ();
}

/*
 * weechat_protocol_config_write: write IRC configuration file
 */

int
weechat_protocol_config_write ()
{
    return irc_config_write ();
}

/*
 * weechat_protocol_dump: dump protocol data in WeeChat log file
 */

int
weechat_protocol_dump ()
{
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
    for (ptr_server = irc_servers; ptr_server; ptr_server = ptr_server->next_server)
    {
        weechat_log_printf ("\n");
        irc_server_print_log (ptr_server);
        
        for (ptr_channel = ptr_server->channels; ptr_channel;
            ptr_channel = ptr_channel->next_channel)
        {
            weechat_log_printf ("\n");
            irc_channel_print_log (ptr_channel);
            
            for (ptr_nick = ptr_channel->nicks; ptr_nick;
                ptr_nick = ptr_nick->next_nick)
            {
                weechat_log_printf ("\n");
                irc_nick_print_log (ptr_nick);
            }
            
        }
    }
    
    irc_dcc_print_log ();
    
    return PROTOCOL_RC_OK;
}

/*
 * weechat_protocol_end: end IRC protocol
 */

int
weechat_protocol_end ()
{
    if (irc_hook_timer)
        weechat_hook_remove (irc_hook_timer);
    
    irc_server_disconnect_all ();
    irc_dcc_end ();
    irc_server_free_all ();
    
    irc_config_write ();

#ifdef HAVE_GNUTLS
    /* GnuTLS end */
    gnutls_certificate_free_credentials (gnutls_xcred);
    gnutls_global_deinit();
#endif
    
    return PROTOCOL_RC_OK;
}
