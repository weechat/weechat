/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Bar item for OTR plugin */

#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-bar-item.h"
#include "otr-buffer.h"
#include "otr-config.h"
#include "otr-context.h"
#include "otr-log.h"


/*
 * Add a state (value of a look option, with a color) to the content of the OTR
 * bar item, after the separator if it is not the first state; nothing is added
 * if the look option is empty.
 */

void
otr_bar_item_part (char **content, const char *separator, int *first,
                   int color_index, int look_index)
{
    const char *ptr_state;

    ptr_state = weechat_config_string (otr_config_look[look_index]);
    if (!ptr_state || !ptr_state[0])
        return;

    if (!*first)
        weechat_string_dyn_concat (content, separator, -1);
    *first = 0;
    weechat_string_dyn_concat (
        content,
        weechat_color (weechat_config_color (otr_config_color[color_index])),
        -1);
    weechat_string_dyn_concat (content, ptr_state, -1);
    weechat_string_dyn_concat (
        content,
        weechat_color (
            weechat_config_color (
                otr_config_color[OTR_CONFIG_COLOR_STATUS_DEFAULT])),
        -1);
}

/*
 * Callback for bar item "otr": OTR state of the private buffer (encrypted,
 * authenticated, logged).
 *
 * The local variables "otr_encrypted", "otr_authenticated" and "otr_logged" are
 * set in the buffer.
 *
 * Return content of bar item (empty string if the buffer is not a private
 * buffer).
 */

char *
otr_bar_item_cb (const void *pointer, void *data, struct t_gui_bar_item *item,
                 struct t_gui_window *window, struct t_gui_buffer *buffer,
                 struct t_hashtable *extra_info)
{
    struct t_gui_buffer *ptr_buffer;
    const char *ptr_own_nick, *ptr_server, *ptr_peer, *ptr_separator;
    char *own_nick, *account, *peer, **states, **result;
    ConnContext *ptr_context;
    int first, encrypted, verified, logged;

    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    ptr_buffer = (buffer) ? buffer : weechat_current_buffer ();

    if (!otr_us || !otr_buffer_is_private (ptr_buffer))
        return strdup ("");

    ptr_own_nick = weechat_buffer_get_string (ptr_buffer, "localvar_nick");
    ptr_server = weechat_buffer_get_string (ptr_buffer, "localvar_server");
    ptr_peer = weechat_buffer_get_string (ptr_buffer, "localvar_channel");
    if (!ptr_own_nick || !ptr_server || !ptr_peer)
        return strdup ("");

    account = NULL;
    own_nick = weechat_string_tolower (ptr_own_nick);
    if (own_nick)
    {
        weechat_asprintf (&account, "%s@%s", own_nick, ptr_server);
        free (own_nick);
    }
    peer = weechat_string_tolower (ptr_peer);
    if (!account || !peer)
    {
        free (account);
        free (peer);
        return strdup ("");
    }

    ptr_context = otr_context_find (account, peer);
    encrypted = otr_context_encrypted (ptr_context);
    verified = otr_context_verified (ptr_context);
    logged = otr_log_buffer_is_logged (ptr_buffer);

    free (account);
    free (peer);

    states = weechat_string_dyn_alloc (64);
    if (!states)
        return strdup ("");

    ptr_separator = weechat_config_string (
        otr_config_look[OTR_CONFIG_LOOK_BAR_SEPARATOR]);
    first = 1;
    if (encrypted)
    {
        otr_bar_item_part (states, ptr_separator, &first,
                           OTR_CONFIG_COLOR_STATUS_ENCRYPTED,
                           OTR_CONFIG_LOOK_BAR_ENCRYPTED);
        if (verified)
        {
            otr_bar_item_part (states, ptr_separator, &first,
                               OTR_CONFIG_COLOR_STATUS_AUTHENTICATED,
                               OTR_CONFIG_LOOK_BAR_AUTHENTICATED);
        }
        else
        {
            otr_bar_item_part (states, ptr_separator, &first,
                               OTR_CONFIG_COLOR_STATUS_UNAUTHENTICATED,
                               OTR_CONFIG_LOOK_BAR_UNAUTHENTICATED);
        }
        if (logged)
        {
            otr_bar_item_part (states, ptr_separator, &first,
                               OTR_CONFIG_COLOR_STATUS_LOGGED,
                               OTR_CONFIG_LOOK_BAR_LOGGED);
        }
        else
        {
            otr_bar_item_part (states, ptr_separator, &first,
                               OTR_CONFIG_COLOR_STATUS_NOTLOGGED,
                               OTR_CONFIG_LOOK_BAR_NOTLOGGED);
        }
    }
    else
    {
        otr_bar_item_part (states, ptr_separator, &first,
                           OTR_CONFIG_COLOR_STATUS_UNENCRYPTED,
                           OTR_CONFIG_LOOK_BAR_UNENCRYPTED);
    }

    result = weechat_string_dyn_alloc (64);
    if (result && (*states)[0])
    {
        weechat_string_dyn_concat (
            result,
            weechat_color (
                weechat_config_color (
                    otr_config_color[OTR_CONFIG_COLOR_STATUS_DEFAULT])),
            -1);
        weechat_string_dyn_concat (
            result,
            weechat_config_string (otr_config_look[OTR_CONFIG_LOOK_BAR_PREFIX]),
            -1);
        weechat_string_dyn_concat (result, *states, -1);
    }
    weechat_string_dyn_free (states, 1);

    weechat_buffer_set (ptr_buffer, "localvar_set_otr_encrypted",
                        (encrypted) ? "true" : "false");
    weechat_buffer_set (ptr_buffer, "localvar_set_otr_authenticated",
                        (verified) ? "true" : "false");
    weechat_buffer_set (ptr_buffer, "localvar_set_otr_logged",
                        (logged) ? "true" : "false");

    return (result) ? weechat_string_dyn_free (result, 0) : strdup ("");
}
