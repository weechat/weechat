/*
 * wee-secure-config.c - secured data configuration options (file sec.conf)
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
#include "wee-secure-config.h"
#include "wee-string.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-main.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"

struct t_config_file *secure_config_file = NULL;

struct t_config_option *secure_config_crypt_cipher = NULL;
struct t_config_option *secure_config_crypt_hash_algo = NULL;
struct t_config_option *secure_config_crypt_passphrase_file = NULL;
struct t_config_option *secure_config_crypt_salt = NULL;


/*
 * Gets passphrase from user and puts it in variable "secure_passphrase".
 */

void
secure_config_get_passphrase_from_user (const char *error)
{
    const char *prompt[5];
    char passphrase[SECURE_PASSPHRASE_MAX_LENGTH + 1];

    prompt[0] = _("Please enter your passphrase to decrypt the data secured "
                  "by WeeChat:");
    prompt[1] = _("(enter just one space to skip the passphrase, but this "
                  "will DISABLE all secured data!)");
    prompt[2] = _("(press ctrl-C to exit WeeChat now)");
    prompt[3] = error;
    prompt[4] = NULL;

    while (1)
    {
        gui_main_get_password (prompt, passphrase, sizeof (passphrase));
        if (secure_passphrase)
        {
            free (secure_passphrase);
            secure_passphrase = NULL;
        }
        if (passphrase[0])
        {
            /* the special value " " (one space) disables passphrase */
            if (strcmp (passphrase, " ") == 0)
            {
                gui_chat_printf (NULL,
                                 _("To recover your secured data, you can "
                                   "use /secure decrypt (see /help secure)"));
            }
            else if (strcmp (passphrase, "\x03") == 0)
            {
                /* ctrl-C pressed, just exit now */
                exit (1);
            }
            else
                secure_passphrase = strdup (passphrase);
            return;
        }
    }
}

/*
 * Gets passphrase from a file.
 *
 * Returns passphrase read in file (only the first line with max length of
 * 1024 chars), or NULL if error.
 *
 * Note: result must be freed after use.
 */

char *
secure_config_get_passphrase_from_file (const char *filename)
{
    FILE *file;
    char *passphrase, *filename2, buffer[1024+1], *pos;
    size_t num_read;

    passphrase = NULL;

    filename2 = string_expand_home (filename);
    if (!filename2)
        return NULL;

    file = fopen (filename2, "r");
    if (file)
    {
        num_read = fread (buffer, 1, sizeof (buffer) - 1, file);
        if (num_read > 0)
        {
            buffer[num_read] = '\0';
            pos = strchr (buffer, '\r');
            if (pos)
                pos[0] = '\0';
            pos = strchr (buffer, '\n');
            if (pos)
                pos[0] = '\0';
            if (buffer[0])
                passphrase = strdup (buffer);
        }
        fclose (file);
    }

    free (filename2);

    return passphrase;
}

/*
 * Checks option "sec.crypt.passphrase_file".
 */

int
secure_config_check_crypt_passphrase_file (const void *pointer, void *data,
                                           struct t_config_option *option,
                                           const char *value)
{
    char *passphrase;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    /* empty value is OK in option (no file used for passphrase) */
    if (!value || !value[0])
        return 1;

    passphrase = secure_config_get_passphrase_from_file (value);
    if (passphrase)
        free (passphrase);
    else
    {
        gui_chat_printf (NULL,
                         _("%sWarning: unable to read passphrase from file "
                           "\"%s\""),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         value);
    }

    return 1;
}

/*
 * Reloads secured data configuration file.
 *
 * Returns:
 *   WEECHAT_CONFIG_READ_OK: OK
 *   WEECHAT_CONFIG_READ_MEMORY_ERROR: not enough memory
 *   WEECHAT_CONFIG_READ_FILE_NOT_FOUND: file not found
 */

int
secure_config_reload_cb (const void *pointer, void *data,
                         struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (secure_hashtable_data_encrypted->items_count > 0)
    {
        gui_chat_printf (NULL,
                         _("%sUnable to reload file sec.conf because there is "
                           "still encrypted data (use /secure decrypt, see "
                           "/help secure)"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
        return WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
    }

    secure_data_encrypted = 0;

    /* remove all secured data */
    hashtable_remove_all (secure_hashtable_data);

    return config_file_reload (config_file);
}

/*
 * Reads a data option in secured data configuration file.
 */

int
secure_config_data_read_cb (const void *pointer, void *data,
                            struct t_config_file *config_file,
                            struct t_config_section *section,
                            const char *option_name, const char *value)
{
    char *buffer, *decrypted, str_error[1024];
    int length_buffer, length_decrypted, rc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;
    (void) section;

    if (!option_name || !value || !value[0])
    {
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
    }

    /* special line indicating if a passphrase must be used to decrypt data */
    if (strcmp (option_name, SECURE_DATA_PASSPHRASE_FLAG) == 0)
    {
        secure_data_encrypted = config_file_string_to_boolean (value);
        if (secure_data_encrypted && !secure_passphrase && !gui_init_ok)
        {
            /* if a passphrase file is set, use it */
            if (CONFIG_STRING(secure_config_crypt_passphrase_file)[0])
                secure_passphrase = secure_config_get_passphrase_from_file (
                    CONFIG_STRING(secure_config_crypt_passphrase_file));

            /* ask passphrase to the user (if no file, or file not found) */
            if (!secure_passphrase)
                secure_config_get_passphrase_from_user ("");
        }
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
    }

    if (!secure_data_encrypted)
    {
        /* clear data: just store value in hashtable */
        hashtable_set (secure_hashtable_data, option_name, value);
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
    }

    /* check that passphrase is set */
    if (!secure_passphrase)
    {
        gui_chat_printf (NULL,
                         _("%sPassphrase is not set, unable to decrypt data \"%s\""),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         option_name);
        hashtable_set (secure_hashtable_data_encrypted, option_name, value);
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
    }

    /* decrypt data */
    buffer = malloc (strlen (value) + 1);
    if (!buffer)
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    length_buffer = string_base16_decode (value, buffer);
    while (1)
    {
        decrypted = NULL;
        length_decrypted = 0;
        rc = secure_decrypt_data (
            buffer,
            length_buffer,
            secure_hash_algo[CONFIG_INTEGER(secure_config_crypt_hash_algo)],
            secure_cipher[CONFIG_INTEGER(secure_config_crypt_cipher)],
            secure_passphrase,
            &decrypted,
            &length_decrypted);
        if (rc == 0)
        {
            if (decrypted)
            {
                hashtable_set (secure_hashtable_data, option_name,
                               decrypted);
                free (decrypted);
                break;
            }
        }
        else
        {
            if (decrypted)
                free (decrypted);
            if (gui_init_ok)
            {
                gui_chat_printf (NULL,
                                 _("%sWrong passphrase, unable to decrypt data "
                                   "\"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 option_name);
                break;
            }
            snprintf (str_error, sizeof (str_error),
                      _("*** Wrong passphrase (decrypt error: %s) ***"),
                      secure_decrypt_error[(rc * -1) - 1]);
            secure_config_get_passphrase_from_user (str_error);
            if (!secure_passphrase)
            {
                gui_chat_printf (NULL,
                                 _("%sPassphrase is not set, unable to decrypt "
                                   "data \"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 option_name);
                hashtable_set (secure_hashtable_data_encrypted, option_name,
                               value);
                break;
            }
        }
    }
    free (buffer);

    return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * Encrypts data and writes it in secured data configuration file.
 */

void
secure_config_data_write_map_cb (void *data,
                                 struct t_hashtable *hashtable,
                                 const void *key, const void *value)
{
    struct t_config_file *config_file;
    char *buffer, *buffer_base16;
    int length_buffer, rc;

    /* make C compiler happy */
    (void) hashtable;

    config_file = (struct t_config_file *)data;

    buffer = NULL;
    length_buffer = 0;

    if (secure_passphrase)
    {
        /* encrypt password using passphrase */
        rc = secure_encrypt_data (
            value, strlen (value) + 1,
            secure_hash_algo[CONFIG_INTEGER(secure_config_crypt_hash_algo)],
            secure_cipher[CONFIG_INTEGER(secure_config_crypt_cipher)],
            secure_passphrase,
            &buffer,
            &length_buffer);
        if (rc == 0)
        {
            if (buffer)
            {
                buffer_base16 = malloc ((length_buffer * 2) + 1);
                if (buffer_base16)
                {
                    if (string_base16_encode (buffer, length_buffer,
                                              buffer_base16) >= 0)
                    {
                        config_file_write_line (config_file, key,
                                                "\"%s\"", buffer_base16);
                    }
                    free (buffer_base16);
                }
                free (buffer);
            }
        }
        else
        {
            gui_chat_printf (NULL,
                             _("%sError encrypting data \"%s\" (%d)"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             key, rc);
        }
    }
    else
    {
        /* store password as plain text */
        config_file_write_line (config_file, key, "\"%s\"", value);
    }
}

/*
 * Writes already encrypted data in secured data configuration file.
 */

void
secure_config_data_write_map_encrypted_cb (void *data,
                                           struct t_hashtable *hashtable,
                                           const void *key, const void *value)
{
    struct t_config_file *config_file;

    /* make C compiler happy */
    (void) hashtable;

    config_file = (struct t_config_file *)data;

    /* store data as-is (it is already encrypted) */
    config_file_write_line (config_file, key, "\"%s\"", value);
}

/*
 * Writes section "data" in secured data configuration file.
 */

int
secure_config_data_write_cb (const void *pointer, void *data,
                             struct t_config_file *config_file,
                             const char *section_name)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    /* write name of section */
    if (!config_file_write_line (config_file, section_name, NULL))
        return WEECHAT_CONFIG_WRITE_ERROR;

    if (secure_hashtable_data->items_count > 0)
    {
        /*
         * write a special line indicating if a passphrase must be used to
         * decrypt data (if not, then data is stored as plain text)
         */
        if (!config_file_write_line (config_file,
                                     SECURE_DATA_PASSPHRASE_FLAG,
                                     (secure_passphrase) ? "on" : "off"))
        {
            return WEECHAT_CONFIG_WRITE_ERROR;
        }
        /* encrypt and write secured data */
        hashtable_map (secure_hashtable_data,
                       &secure_config_data_write_map_cb, config_file);
    }
    else if (secure_hashtable_data_encrypted->items_count > 0)
    {
        /*
         * if there is encrypted data, that means passphrase was not set and
         * we were unable to decrypt => just save the encrypted content
         * as-is (so that content of sec.conf is not lost)
         */
        if (!config_file_write_line (config_file,
                                     SECURE_DATA_PASSPHRASE_FLAG, "on"))
        {
            return WEECHAT_CONFIG_WRITE_ERROR;
        }
        hashtable_map (secure_hashtable_data_encrypted,
                       &secure_config_data_write_map_encrypted_cb, config_file);
    }

    return WEECHAT_CONFIG_WRITE_OK;
}

/*
 * Creates options in secured data configuration.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
secure_config_init_options ()
{
    struct t_config_section *ptr_section;

    secure_config_file = config_file_new (NULL, SECURE_CONFIG_NAME,
                                          &secure_config_reload_cb, NULL, NULL);
    if (!secure_config_file)
        return 0;

    /* crypt */
    ptr_section = config_file_new_section (secure_config_file, "crypt",
                                           0, 0,
                                           NULL, NULL, NULL,
                                           NULL, NULL, NULL,
                                           NULL, NULL, NULL,
                                           NULL, NULL, NULL,
                                           NULL, NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (secure_config_file);
        secure_config_file = NULL;
        return 0;
    }

    secure_config_crypt_cipher = config_file_new_option (
        secure_config_file, ptr_section,
        "cipher", "integer",
        N_("cipher used to crypt data (the number after algorithm is the size "
           "of the key in bits)"),
        "aes128|aes192|aes256", 0, 0, "aes256", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    secure_config_crypt_hash_algo = config_file_new_option (
        secure_config_file, ptr_section,
        "hash_algo", "integer",
        N_("hash algorithm used to check the decrypted data"),
        "sha224|sha256|sha384|sha512", 0, 0, "sha256", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    secure_config_crypt_passphrase_file = config_file_new_option (
        secure_config_file, ptr_section,
        "passphrase_file", "string",
        N_("path to a file containing the passphrase to encrypt/decrypt secured "
           "data; this option is used only when reading file sec.conf; only "
           "first line of file is used; this file is used only if the "
           "environment variable \"WEECHAT_PASSPHRASE\" is not set (the "
           "environment variable has higher priority); security note: it is "
           "recommended to keep this file readable only by you and store it "
           "outside WeeChat home (for example in your home); example: "
           "\"~/.weechat-passphrase\""),
        NULL, 0, 0, "", NULL, 0,
        &secure_config_check_crypt_passphrase_file, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    secure_config_crypt_salt = config_file_new_option (
        secure_config_file, ptr_section,
        "salt", "boolean",
        N_("use salt when generating key used in encryption (recommended for "
           "maximum security); when enabled, the content of crypted data in "
           "file sec.conf will be different on each write of the file; if you "
           "put the file sec.conf in a version control system, then you "
           "can turn off this option to have always same content in file"),
        NULL, 0, 0, "on", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    /* data */
    ptr_section = config_file_new_section (
        secure_config_file, "data",
        0, 0,
        &secure_config_data_read_cb, NULL, NULL,
        &secure_config_data_write_cb, NULL, NULL,
        &secure_config_data_write_cb, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (secure_config_file);
        secure_config_file = NULL;
        return 0;
    }

    return 1;
}

/*
 * Reads secured data configuration file.
 *
 * Returns:
 *   WEECHAT_CONFIG_READ_OK: OK
 *   WEECHAT_CONFIG_READ_MEMORY_ERROR: not enough memory
 *   WEECHAT_CONFIG_READ_FILE_NOT_FOUND: file not found
 */

int
secure_config_read ()
{
    int rc;

    secure_data_encrypted = 0;

    rc = config_file_read (secure_config_file);

    return rc;
}

/*
 * Writes secured data configuration file.
 *
 * Returns:
 *   WEECHAT_CONFIG_WRITE_OK: OK
 *   WEECHAT_CONFIG_WRITE_ERROR: error
 *   WEECHAT_CONFIG_WRITE_MEMORY_ERROR: not enough memory
 */

int
secure_config_write ()
{
    return config_file_write (secure_config_file);
}

/*
 * Initializes secured data configuration.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
secure_config_init ()
{
    int rc;

    rc = secure_config_init_options ();

    if (!rc)
    {
        gui_chat_printf (NULL,
                         _("FATAL: error initializing configuration options"));
    }

    return rc;
}

/*
 * Frees secured data file and variables.
 */

void
secure_config_free ()
{
    config_file_free (secure_config_file);
    secure_config_file = NULL;
}
