/*
 * core-secure-buffer.c - secured data buffer
 *
 * Copyright (C) 2013-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <gcrypt.h>

#include "weechat.h"
#include "core-config-file.h"
#include "core-crypto.h"
#include "core-hashtable.h"
#include "core-secure.h"
#include "core-secure-buffer.h"
#include "core-secure-config.h"
#include "core-string.h"
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
    int line, count, count_encrypted, hash_algo;
    char str_supported[1024];

    if (!secure_buffer)
        return;

    gui_buffer_clear (secure_buffer);

    /* set title buffer */
    gui_buffer_set_title (secure_buffer,
                          _("WeeChat secured data (sec.conf) | "
                            "Keys: [alt-v] Toggle values"));

    line = 0;

    str_supported[0] = '\0';
    hash_algo = weecrypto_get_hash_algo (
        config_file_option_string (secure_config_crypt_hash_algo));
    if (hash_algo == GCRY_MD_NONE)
    {
        snprintf (str_supported, sizeof (str_supported),
                  " (%s)",
                  /* TRANSLATORS: "hash algorithm not supported" */
                  _("not supported"));
    }

    gui_chat_printf_y (secure_buffer, line++,
                       "Hash algo: %s%s  Cipher: %s  Salt: %s",
                       config_file_option_string (secure_config_crypt_hash_algo),
                       str_supported,
                       config_file_option_string (secure_config_crypt_cipher),
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

    if (string_strcmp (input_data, "q") == 0)
        gui_buffer_close (buffer);

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
        secure_buffer = gui_buffer_search (NULL, SECURE_BUFFER_NAME);
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
    struct t_hashtable *properties;

    if (!secure_buffer)
    {
        properties = hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL, NULL);
        if (properties)
        {
            hashtable_set (properties, "type", "free");
            hashtable_set (properties, "localvar_set_no_log", "1");
            hashtable_set (properties,
                           "key_bind_meta-v", "/secure toggle_values");
        }

        secure_buffer = gui_buffer_new_props (
            NULL, SECURE_BUFFER_NAME, properties,
            &secure_buffer_input_cb, NULL, NULL,
            &secure_buffer_close_cb, NULL, NULL);

        if (secure_buffer && !secure_buffer->short_name)
            secure_buffer->short_name = strdup (SECURE_BUFFER_NAME);

        secure_buffer_display_values = 0;

        hashtable_free (properties);
    }

    if (!secure_buffer)
        return;

    gui_window_switch_to_buffer (gui_current_window, secure_buffer, 1);

    secure_buffer_display ();
}
