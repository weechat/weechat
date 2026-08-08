/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Debug functions for IRC plugin */

#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-debug.h"
#include "irc-ignore.h"
#include "irc-redirect.h"
#include "irc-server.h"


/*
 * Dump IRC data in WeeChat log file.
 */

int
irc_debug_signal_debug_dump_cb (const void *pointer, void *data,
                                const char *signal,
                                const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data || (strcmp ((char *)signal_data, IRC_PLUGIN_NAME) == 0))
    {
        weechat_log_printf ("");
        weechat_log_printf ("***** \"%s\" plugin dump *****",
                            weechat_plugin->name);

        irc_server_print_log ();
        irc_ignore_print_log ();
        irc_redirect_pattern_print_log ();

        weechat_log_printf ("");
        weechat_log_printf ("***** End of \"%s\" plugin dump *****",
                            weechat_plugin->name);
    }

    return WEECHAT_RC_OK;
}

/*
 * Initialize debug for IRC plugin.
 */

void
irc_debug_init (void)
{
    weechat_hook_signal ("debug_dump",
                         &irc_debug_signal_debug_dump_cb, NULL, NULL);
}
