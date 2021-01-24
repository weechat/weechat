/*
 * wee-secure-buffer.c - secured data buffer
 *
 * Copyright (C) 2013-2021 Sébastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "weechat.h"
#include "wee-config-file.h"
#include "wee-hashtable.h"
#include "wee-secure.h"
#include "wee-secure-buffer.h"
#include "wee-secure-config.h"
#include "wee-string.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"

struct t_gui_buffer *secure_buffer = NULL;
int secure_buffer_display_values = 0;


/*
 * Displays a secured data.
 */

void
secure_buffer_display_data (void *data,
                            struct t_hashtable *hashtable,
                            const void *key, const void *value)
{
    int *line;

    /* make C compiler happy */
    (void) value;

    line = (int *)data;

    if (secure_buffer_display_values && (hashtable == secure_hashtable_data))
    {
        gui_chat_printf_y (secure_buffer, (*line)++,
                           "  %s%s = %s\"%s%s%s\"",
                           key,
                           GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                           GUI_COLOR(GUI_COLOR_CHAT),
                           GUI_COLOR(GUI_COLOR_CHAT_VALUE),
                           value,
                           GUI_COLOR(GUI_COLOR_CHAT));
    }
    else
    {
        gui_chat_printf_y (secure_buffer, (*line)++,
                           "  %s", key);
    }
}

/*
 * Displays content of secured data buffer.
 */

void
secure_buffer_display ()
{
    int line, count, count_encrypted;

    if (!secure_buffer)
        return;

    gui_buffer_clear (secure_buffer);

    /* set title buffer */
    gui_buffer_set_title (secure_buffer,
                          _("WeeChat secured data (sec.conf) | "
                            "Keys: [alt-v] Toggle values"));

    line = 0;

    gui_chat_printf_y (secure_buffer, line++,
                       "Hash algo: %s  Cipher: %s  Salt: %s",
                       secure_hash_algo_string[CONFIG_INTEGER(secure_config_crypt_hash_algo)],
                       secure_cipher_string[CONFIG_INTEGER(secure_config_crypt_cipher)],
                       (CONFIG_BOOLEAN(secure_config_crypt_salt)) ? _("on") : _("off"));

    /* display passphrase */
    line++;
    gui_chat_printf_y (secure_buffer, line++,
                       (secure_passphrase) ?
                       _("Passphrase is set") : _("Passphrase is not set"));

    /* display secured data */
    count = secure_hashtable_data->items_count;
    count_encrypted = secure_hashtable_data_encrypted->items_count;
    if (count > 0)
    {
        line++;
        gui_chat_printf_y (secure_buffer, line++, _("Secured data:"));
        line++;
        hashtable_map (secure_hashtable_data,
                       &secure_buffer_display_data, &line);
    }
    /* display secured data not decrypted */
    if (count_encrypted > 0)
    {
        line++;
        gui_chat_printf_y (secure_buffer, line++,
                           _("Secured data STILL ENCRYPTED: (use /secure decrypt, "
                             "see /help secure)"));
        line++;
        hashtable_map (secure_hashtable_data_encrypted,
                       &secure_buffer_display_data, &line);
    }
    if ((count == 0) && (count_encrypted == 0))
    {
        line++;
        gui_chat_printf_y (secure_buffer, line++, _("No secured data set"));
    }
}

/*
 * Input callback for secured data buffer.
 */

int
secure_buffer_input_cb (const void *pointer, void *data,
                        struct t_gui_buffer *buffer,
                        const char *input_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (string_strcasecmp (input_data, "q") == 0)
    {
        gui_buffer_close (buffer);
    }

    return WEECHAT_RC_OK;
}

/*
 * Close callback for secured data buffer.
 */

int
secure_buffer_close_cb (const void *pointer, void *data,
                        struct t_gui_buffer *buffer)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    secure_buffer = NULL;

    return WEECHAT_RC_OK;
}

/*
 * Assigns secured data buffer to pointer if it is not yet set.
 */

void
secure_buffer_assign ()
{
    if (!secure_buffer)
    {
        secure_buffer = gui_buffer_search_by_name (NULL, SECURE_BUFFER_NAME);
        if (secure_buffer)
        {
            secure_buffer->input_callback = &secure_buffer_input_cb;
            secure_buffer->close_callback = &secure_buffer_close_cb;
        }
    }
}

/*
 * Opens a buffer to display secured data.
 */

void
secure_buffer_open ()
{
    if (!secure_buffer)
    {
        secure_buffer = gui_buffer_new (NULL, SECURE_BUFFER_NAME,
                                        &secure_buffer_input_cb, NULL, NULL,
                                        &secure_buffer_close_cb, NULL, NULL);
        if (secure_buffer)
        {
            if (!secure_buffer->short_name)
                secure_buffer->short_name = strdup (SECURE_BUFFER_NAME);
            gui_buffer_set (secure_buffer, "type", "free");
            gui_buffer_set (secure_buffer, "localvar_set_no_log", "1");
            gui_buffer_set (secure_buffer, "key_bind_meta-v", "/secure toggle_values");
        }
        secure_buffer_display_values = 0;
    }

    if (!secure_buffer)
        return;

    gui_window_switch_to_buffer (gui_current_window, secure_buffer, 1);

    secure_buffer_display ();
}
