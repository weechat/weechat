/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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

/* irc.c: IRC plugin for WeeChat */


#include <stdlib.h>
#include <string.h>

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-command.h"
#include "irc-completion.h"
#include "irc-config.h"
#include "irc-debug.h"
#include "irc-server.h"
#include "irc-channel.h"
#include "irc-nick.h"
#include "irc-dcc.h"


WEECHAT_PLUGIN_NAME("irc");
WEECHAT_PLUGIN_DESCRIPTION("IRC (Internet Relay Chat) plugin for WeeChat");
WEECHAT_PLUGIN_AUTHOR("FlashCode <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_WEECHAT_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE("GPL");

struct t_weechat_plugin *weechat_irc_plugin = NULL;

struct t_hook *irc_hook_timer = NULL;
struct t_hook *irc_hook_timer_check_away = NULL;

#ifdef HAVE_GNUTLS
gnutls_certificate_credentials gnutls_xcred; /* gnutls client credentials */
#endif


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
    }
}

/*
 * irc_signal_quit_cb: callback for "quit" signal
 */

int
irc_signal_quit_cb (void *data, char *signal, char *type_data,
                    void *signal_data)
{
    struct t_irc_server *ptr_server;
    
    /* make C compiler happy */
    (void) data;
    (void) signal;
    
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            irc_command_quit_server (ptr_server, (char *)signal_data);
        }
    }
    
    return WEECHAT_RC_OK;
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
        return WEECHAT_RC_ERROR;

    if (irc_config_read () < 0)
        return WEECHAT_RC_ERROR;

    irc_create_directories ();

    irc_command_init ();

    /* hook some signals */
    irc_debug_init ();
    weechat_hook_signal ("quit", &irc_signal_quit_cb, NULL);
    
    /* hook completions */
    irc_completion_init ();
    
    irc_server_auto_connect (1, 0);
    
    irc_hook_timer = weechat_hook_timer (1 * 1000, 0, 0,
                                         &irc_server_timer_cb, NULL);
    
    /*
    if (irc_cfg_irc_away_check != 0)
        irc_hook_timer_check_away = weechat_hook_timer (irc_cfg_irc_away_check * 60 * 1000,
                                                        0,
                                                        &irc_server_timer_check_away,
                                                        NULL);
    */
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end IRC plugin
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;
    
    irc_config_write ();
    
    irc_server_disconnect_all ();
    //irc_dcc_end ();
    irc_server_free_all ();
    
#ifdef HAVE_GNUTLS
    /* GnuTLS end */
    gnutls_certificate_free_credentials (gnutls_xcred);
    gnutls_global_deinit();
#endif
    
    return WEECHAT_RC_OK;
}
