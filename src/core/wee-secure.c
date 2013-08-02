/*
 * wee-secure.c - secured data configuration options (file sec.conf)
 *
 * Copyright (C) 2013 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <gcrypt.h>

#include "weechat.h"
#include "wee-config-file.h"
#include "wee-hashtable.h"
#include "wee-secure.h"
#include "wee-string.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-main.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"

#define SALT_SIZE 8

struct t_config_file *secure_config_file = NULL;
struct t_config_section *secure_config_section_crypt = NULL;
struct t_config_section *secure_config_section_data = NULL;

struct t_config_option *secure_config_crypt_cipher = NULL;
struct t_config_option *secure_config_crypt_hash_algo = NULL;
struct t_config_option *secure_config_crypt_passphrase_file = NULL;
struct t_config_option *secure_config_crypt_salt = NULL;

/* the passphrase used to encrypt/decrypt data */
char *secure_passphrase = NULL;

/* decrypted data */
struct t_hashtable *secure_hashtable_data = NULL;

/* data still encrypted (if passphrase not set) */
struct t_hashtable *secure_hashtable_data_encrypted = NULL;

/* hash algorithms */
char *secure_hash_algo_string[] = { "sha224", "sha256", "sha384", "sha512",
                                    NULL };
int secure_hash_algo[] = { GCRY_MD_SHA224, GCRY_MD_SHA256, GCRY_MD_SHA384,
                           GCRY_MD_SHA512 };

/* ciphers */
char *secure_cipher_string[] = { "aes128", "aes192", "aes256", NULL };
int secure_cipher[] = { GCRY_CIPHER_AES128, GCRY_CIPHER_AES192,
                        GCRY_CIPHER_AES256 };

char *secure_decrypt_error[] = { "memory", "buffer", "key", "cipher", "setkey",
                                 "decrypt", "hash", "hash mismatch" };

/* used only when reading sec.conf: 1 if flag __passphrase__ is enabled */
int secure_data_encrypted = 0;

/* secured data buffer */
struct t_gui_buffer *secure_buffer = NULL;
int secure_buffer_display_values = 0;


/*
 * Searches for a hash algorithm.
 *
 * Returns hash algorithm value (from gcrypt constant), -1 if hash algorithm is
 * not found.
 */

int
secure_search_hash_algo (const char *hash_algo)
{
    int i;

    if (!hash_algo)
        return -1;

    for (i = 0; secure_hash_algo_string[i]; i++)
    {
        if (strcmp (secure_hash_algo_string[i], hash_algo) == 0)
            return secure_hash_algo[i];
    }

    /* hash algorithm not found */
    return -1;
}

/*
 * Searches for a cipher.
 *
 * Returns cipher value (from gcrypt constant), -1 if cipher is not
 * found.
 */

int
secure_search_cipher (const char *cipher)
{
    int i;

    if (!cipher)
        return -1;

    for (i = 0; secure_cipher_string[i]; i++)
    {
        if (strcmp (secure_cipher_string[i], cipher) == 0)
            return secure_cipher[i];
    }

    /* cipher not found */
    return -1;
}

/*
 * Derives a key from salt + passphrase (using a hash).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
secure_derive_key (const char *salt, const char *passphrase,
                   unsigned char *key, int length_key)
{
    unsigned char *buffer, *ptr_hash;
    int length, length_hash;
    gcry_md_hd_t hd_md;

    memset (key, 0, length_key);

    length = SALT_SIZE + strlen (passphrase);
    buffer = malloc (length);
    if (!buffer)
        return 0;

    /* build a buffer with salt + passphrase */
    memcpy (buffer, salt, SALT_SIZE);
    memcpy (buffer + SALT_SIZE, passphrase, strlen (passphrase));

    /* compute hash of buffer */
    if (gcry_md_open (&hd_md, GCRY_MD_SHA512, 0) != 0)
    {
        free (buffer);
        return 0;
    }
    length_hash = gcry_md_get_algo_dlen (GCRY_MD_SHA512);
    gcry_md_write (hd_md, buffer, length);
    ptr_hash = gcry_md_read (hd_md, GCRY_MD_SHA512);
    if (!ptr_hash)
    {
        gcry_md_close (hd_md);
        free (buffer);
        return 0;
    }

    /* copy beginning of hash (or full hash) in the key */
    memcpy (key, ptr_hash,
            (length_hash > length_key) ? length_key : length_hash);

    gcry_md_close (hd_md);
    free (buffer);

    return 1;
}

/*
 * Encrypts data using a hash algorithm + cipher + passphrase.
 *
 * Following actions are performed:
 *   1. derive a key from the passphrase (with optional salt)
 *   2. compute hash of data
 *   3. store hash + data in a buffer
 *   4. encrypt the buffer (hash + data), using the key
 *   5. return salt + encrypted hash/data
 *
 * Output buffer has following content:
 * - salt (8 bytes, used to derive a key from the passphrase)
 * - encrypted hash(data) + data
 *
 * So it looks like:
 *
 * +----------+------------+------------------------------+
 * |   salt   |    hash    |             data             |
 * +----------+------------+------------------------------+
 * \_ _ _ _ _/\_ _ _ _ _ _ /\_ _ _ _ _ _ _ _ _ _ _ _ _ _ _/
 *   8 bytes     N bytes         variable length
 *            \_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _/
 *                          encrypted data
 *
 * Returns:
 *     0: OK
 *    -1: not enough memory
 *    -2: key derive error
 *    -3: compute hash error
 *    -4: cipher open error
 *    -5: setkey error
 *    -6: encrypt error
 */

int
secure_encrypt_data (const char *data, int length_data,
                     int hash_algo, int cipher, const char *passphrase,
                     char **encrypted, int *length_encrypted)
{
    int rc, length_salt, length_hash, length_hash_data, length_key;
    gcry_md_hd_t *hd_md;
    gcry_cipher_hd_t *hd_cipher;
    char salt[SALT_SIZE];
    unsigned char *ptr_hash, *key, *hash_and_data;

    rc = -1;

    hd_md = NULL;
    hd_cipher = NULL;
    key = NULL;
    hash_and_data = NULL;

    hd_md = malloc (sizeof (gcry_md_hd_t));
    if (!hd_md)
        return -1;
    hd_cipher = malloc (sizeof (gcry_cipher_hd_t));
    if (!hd_cipher)
    {
        free (hd_md);
        return -1;
    }

    /* derive a key from the passphrase */
    length_key = gcry_cipher_get_algo_keylen (cipher);
    key = malloc (length_key);
    if (!key)
        goto encend;
    if (CONFIG_BOOLEAN(secure_config_crypt_salt))
        gcry_randomize (salt, SALT_SIZE, GCRY_STRONG_RANDOM);
    else
    {
        length_salt = strlen (SECURE_SALT_DEFAULT);
        if (length_salt < SALT_SIZE)
            memset (salt, 0, SALT_SIZE);
        memcpy (salt, SECURE_SALT_DEFAULT,
                (length_salt <= SALT_SIZE) ? length_salt : SALT_SIZE);
    }
    if (!secure_derive_key (salt, passphrase, key, length_key))
    {
        rc = -2;
        goto encend;
    }

    /* compute hash of data */
    if (gcry_md_open (hd_md, hash_algo, 0) != 0)
    {
        rc = -3;
        goto encend;
    }
    length_hash = gcry_md_get_algo_dlen (hash_algo);
    gcry_md_write (*hd_md, data, length_data);
    ptr_hash = gcry_md_read (*hd_md, hash_algo);
    if (!ptr_hash)
    {
        rc = -3;
        goto encend;
    }

    /* build a buffer with hash + data */
    length_hash_data = length_hash + length_data;
    hash_and_data = malloc (length_hash_data);
    if (!hash_and_data)
        goto encend;
    memcpy (hash_and_data, ptr_hash, length_hash);
    memcpy (hash_and_data + length_hash, data, length_data);

    /* encrypt hash + data */
    if (gcry_cipher_open (hd_cipher, cipher, GCRY_CIPHER_MODE_CFB, 0) != 0)
    {
        rc = -4;
        goto encend;
    }
    if (gcry_cipher_setkey (*hd_cipher, key, length_key) != 0)
    {
        rc = -5;
        goto encend;
    }
    if (gcry_cipher_encrypt (*hd_cipher, hash_and_data, length_hash_data,
                             NULL, 0) != 0)
    {
        rc = -6;
        goto encend;
    }

    /* create buffer and copy salt + encrypted hash/data into this buffer*/
    *length_encrypted = SALT_SIZE + length_hash_data;
    *encrypted = malloc (*length_encrypted);
    if (!*encrypted)
        goto encend;
    memcpy (*encrypted, salt, SALT_SIZE);
    memcpy (*encrypted + SALT_SIZE, hash_and_data, length_hash_data);

    rc = 0;

encend:
    if (hd_md)
    {
        gcry_md_close (*hd_md);
        free (hd_md);
    }
    if (hd_cipher)
    {
        gcry_cipher_close (*hd_cipher);
        free (hd_cipher);
    }
    if (key)
        free (key);
    if (hash_and_data)
        free (hash_and_data);

    return rc;
}

/*
 * Decrypts data using a hash algorithm + cipher + passphrase.
 *
 * The buffer must contain:
 * - salt (8 bytes, used to derive a key from the passphrase)
 * - encrypted hash(data) + data
 *
 * Following actions are performed:
 *   1. check length of buffer (it must have at least salt + hash + some data)
 *   2. derive a key from the passphrase using salt (at beginning of buffer)
 *   3. decrypt hash + data in a buffer
 *   4. compute hash of decrypted data
 *   5. check that decrypted hash is equal to hash of data
 *   6. return decrypted data
 *
 * Returns:
 *    0: OK
 *   -1: not enough memory
 *   -2: buffer is not long enough
 *   -3: key derive error
 *   -4: cipher open error
 *   -5: setkey error
 *   -6: decrypt error
 *   -7: compute hash error
 *   -8: hash does not match the decrypted data
 *
 * Note: when adding a return code, change the array "secure_decrypt_error"
 * accordingly.
 */

int
secure_decrypt_data (const char *buffer, int length_buffer,
                     int hash_algo, int cipher, const char *passphrase,
                     char **decrypted, int *length_decrypted)
{
    int rc, length_hash, length_key;
    gcry_md_hd_t *hd_md;
    gcry_cipher_hd_t *hd_cipher;
    unsigned char *ptr_hash, *key, *decrypted_hash_data;

    rc = -1;

    /* check length of buffer */
    length_hash = gcry_md_get_algo_dlen (hash_algo);
    if (length_buffer <= SALT_SIZE + length_hash)
        return -2;

    hd_md = NULL;
    hd_cipher = NULL;
    key = NULL;
    decrypted_hash_data = NULL;

    hd_md = malloc (sizeof (gcry_md_hd_t));
    if (!hd_md)
        return rc;
    hd_cipher = malloc (sizeof (gcry_cipher_hd_t));
    if (!hd_cipher)
    {
        free (hd_md);
        return rc;
    }

    /* derive a key from the passphrase */
    length_key = gcry_cipher_get_algo_keylen (cipher);
    key = malloc (length_key);
    if (!key)
        goto decend;
    if (!secure_derive_key (buffer, passphrase, key, length_key))
    {
        rc = -3;
        goto decend;
    }

    /* decrypt hash + data */
    decrypted_hash_data = malloc (length_buffer - SALT_SIZE);
    if (!decrypted_hash_data)
        goto decend;
    if (gcry_cipher_open (hd_cipher, cipher, GCRY_CIPHER_MODE_CFB, 0) != 0)
    {
        rc = -4;
        goto decend;
    }
    if (gcry_cipher_setkey (*hd_cipher, key, length_key) != 0)
    {
        rc = -5;
        goto decend;
    }
    if (gcry_cipher_decrypt (*hd_cipher,
                             decrypted_hash_data, length_buffer - SALT_SIZE,
                             buffer + SALT_SIZE, length_buffer - SALT_SIZE) != 0)
    {
        rc = -6;
        goto decend;
    }

    /* check if hash is OK for decrypted data */
    if (gcry_md_open (hd_md, hash_algo, 0) != 0)
    {
        rc = -7;
        goto decend;
    }
    gcry_md_write (*hd_md, decrypted_hash_data + length_hash,
                   length_buffer - SALT_SIZE - length_hash);
    ptr_hash = gcry_md_read (*hd_md, hash_algo);
    if (!ptr_hash)
    {
        rc = -7;
        goto decend;
    }
    if (memcmp (ptr_hash, decrypted_hash_data, length_hash) != 0)
    {
        rc = -8;
        goto decend;
    }

    /* return the decrypted data */
    *length_decrypted = length_buffer - SALT_SIZE - length_hash;
    *decrypted = malloc (*length_decrypted);
    if (!*decrypted)
        goto decend;

    memcpy (*decrypted, decrypted_hash_data + length_hash, *length_decrypted);

    rc = 0;

decend:
    if (hd_md)
    {
        gcry_md_close (*hd_md);
        free (hd_md);
    }
    if (hd_cipher)
    {
        gcry_cipher_close (*hd_cipher);
        free (hd_cipher);
    }
    if (key)
        free (key);
    if (decrypted_hash_data)
        free (decrypted_hash_data);

    return rc;
}

/*
 * Decrypts data still encrypted (data that could not be decrypted when reading
 * secured data configuration file (because no passphrase was given).
 *
 * Returns:
 *   > 0: number of decrypted data
 *     0: error decrypting data
 */

int
secure_decrypt_data_not_decrypted (const char *passphrase)
{
    char **keys, *buffer, *decrypted;
    const char *value;
    int num_ok, num_keys, i, length_buffer, length_decrypted, rc;

    /* we need a passphrase to decrypt data! */
    if (!passphrase || !passphrase[0])
        return 0;

    num_ok = 0;

    keys = string_split (hashtable_get_string (secure_hashtable_data_encrypted,
                                               "keys"),
                         ",", 0, 0, &num_keys);
    if (keys)
    {
        for (i = 0; i < num_keys; i++)
        {
            value = hashtable_get (secure_hashtable_data_encrypted, keys[i]);
            if (value && value[0])
            {
                buffer = malloc (strlen (value) + 1);
                if (buffer)
                {
                    length_buffer = string_decode_base16 (value, buffer);
                    decrypted = NULL;
                    length_decrypted = 0;
                    rc = secure_decrypt_data (buffer,
                                              length_buffer,
                                              secure_hash_algo[CONFIG_INTEGER(secure_config_crypt_hash_algo)],
                                              secure_cipher[CONFIG_INTEGER(secure_config_crypt_cipher)],
                                              passphrase,
                                              &decrypted,
                                              &length_decrypted);
                    if ((rc == 0) && decrypted)
                    {
                        hashtable_set (secure_hashtable_data, keys[i],
                                       decrypted);
                        hashtable_remove (secure_hashtable_data_encrypted,
                                          keys[i]);
                        num_ok++;
                    }
                    if (decrypted)
                        free (decrypted);
                    free (buffer);
                }
            }
        }
        string_free_split (keys);
    }

    return num_ok;
}

/*
 * Gets passphrase from user and puts it in variable "secure_passphrase".
 */

void
secure_get_passphrase_from_user (const char *error)
{
    char passphrase[1024];

    while (1)
    {
        gui_main_get_password (_("Please enter your passphrase to decrypt the "
                                 "data secured by WeeChat:"),
                               _("(enter just one space to skip the passphrase, "
                                 "but this will DISABLE all secured data!)"),
                               error,
                               passphrase, sizeof (passphrase));
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
 */

char *
secure_get_passphrase_from_file (const char *filename)
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
secure_check_crypt_passphrase_file (void *data,
                                    struct t_config_option *option,
                                    const char *value)
{
    char *passphrase;

    /* make C compiler happy */
    (void) data;
    (void) option;

    /* empty value is OK in option (no file used for passphrase) */
    if (!value || !value[0])
        return 1;

    passphrase = secure_get_passphrase_from_file (value);
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
secure_reload_cb (void *data, struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) data;

    if (secure_hashtable_data_encrypted->items_count > 0)
    {
        gui_chat_printf (NULL,
                         _("%sError: not possible to reload file sec.conf "
                           "because there is still encrypted data (use /secure "
                           "decrypt, see /help secure)"),
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
secure_data_read_cb (void *data,
                     struct t_config_file *config_file,
                     struct t_config_section *section,
                     const char *option_name, const char *value)
{
    char *buffer, *decrypted, str_error[1024];
    int length_buffer, length_decrypted, rc;

    /* make C compiler happy */
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
                secure_passphrase = secure_get_passphrase_from_file (CONFIG_STRING(secure_config_crypt_passphrase_file));

            /* ask passphrase to the user (if no file, or file not found) */
            if (!secure_passphrase)
                secure_get_passphrase_from_user ("");
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

    length_buffer = string_decode_base16 (value, buffer);
    while (1)
    {
        decrypted = NULL;
        length_decrypted = 0;
        rc = secure_decrypt_data (buffer,
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
            secure_get_passphrase_from_user (str_error);
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
secure_data_write_map_cb (void *data,
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
        rc = secure_encrypt_data (value, strlen (value) + 1,
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
                    string_encode_base16 (buffer, length_buffer, buffer_base16);
                    config_file_write_line (config_file, key,
                                            "\"%s\"", buffer_base16);
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
secure_data_write_map_encrypted_cb (void *data,
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
secure_data_write_cb (void *data, struct t_config_file *config_file,
                      const char *section_name)
{
    /* make C compiler happy */
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
                       &secure_data_write_map_cb, config_file);
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
                       &secure_data_write_map_encrypted_cb, config_file);
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
secure_init_options ()
{
    struct t_config_section *ptr_section;

    secure_config_file = config_file_new (NULL, SECURE_CONFIG_NAME,
                                          &secure_reload_cb, NULL);
    if (!secure_config_file)
        return 0;

    /* crypt */
    ptr_section = config_file_new_section (secure_config_file, "crypt",
                                           0, 0,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL, NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (secure_config_file);
        return 0;
    }

    secure_config_crypt_cipher = config_file_new_option (
        secure_config_file, ptr_section,
        "cipher", "integer",
        N_("cipher used to crypt data (the number after algorithm is the size "
           "of the key in bits)"),
        "aes128|aes192|aes256", 0, 0, "aes256", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL);
    secure_config_crypt_hash_algo = config_file_new_option (
        secure_config_file, ptr_section,
        "hash_algo", "integer",
        N_("hash algorithm used to check the decrypted data"),
        "sha224|sha256|sha384|sha512", 0, 0, "sha256", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL);
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
        &secure_check_crypt_passphrase_file, NULL, NULL, NULL, NULL, NULL);
    secure_config_crypt_salt = config_file_new_option (
        secure_config_file, ptr_section,
        "salt", "boolean",
        N_("use salt when generating key used in encryption (recommended for "
           "maximum security); when enabled, the content of crypted data in "
           "file sec.conf will be different on each write of the file; if you "
           "put the file sec.conf in a version control system, then you "
           "can turn off this option to have always same content in file"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    /* data */
    ptr_section = config_file_new_section (secure_config_file, "data",
                                           0, 0,
                                           &secure_data_read_cb, NULL,
                                           &secure_data_write_cb, NULL,
                                           &secure_data_write_cb, NULL,
                                           NULL, NULL, NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (secure_config_file);
        return 0;
    }

    return 1;
}

/*
 * Initializes secured data configuration.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
secure_init ()
{
    int rc;
    char *ptr_phrase;

    /* try to read passphrase (if not set) from env var "WEECHAT_PASSPHRASE" */
    if (!secure_passphrase)
    {
        ptr_phrase = getenv (SECURE_ENV_PASSPHRASE);
        if (ptr_phrase)
        {
            if (ptr_phrase[0])
                secure_passphrase = strdup (ptr_phrase);
            unsetenv (SECURE_ENV_PASSPHRASE);
        }
    }

    secure_hashtable_data = hashtable_new (32,
                                           WEECHAT_HASHTABLE_STRING,
                                           WEECHAT_HASHTABLE_STRING,
                                           NULL,
                                           NULL);
    if (!secure_hashtable_data)
        return 0;

    secure_hashtable_data_encrypted = hashtable_new (32,
                                                     WEECHAT_HASHTABLE_STRING,
                                                     WEECHAT_HASHTABLE_STRING,
                                                     NULL,
                                                     NULL);
    if (!secure_hashtable_data_encrypted)
    {
        hashtable_free (secure_hashtable_data);
        return 0;
    }

    rc = secure_init_options ();

    if (!rc)
    {
        gui_chat_printf (NULL,
                         _("FATAL: error initializing configuration options"));
    }

    return rc;
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
secure_read ()
{
    int rc;

    secure_data_encrypted = 0;

    rc = config_file_read (secure_config_file);

    if (rc != WEECHAT_CONFIG_READ_OK)
    {
        gui_chat_printf (NULL,
                         _("%sError reading configuration"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
    }

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
secure_write ()
{
    return config_file_write (secure_config_file);
}

/*
 * Frees secured data file and variables.
 */

void
secure_free ()
{
    config_file_free (secure_config_file);

    if (secure_hashtable_data)
    {
        hashtable_free (secure_hashtable_data);
        secure_hashtable_data = NULL;
    }
    if (secure_hashtable_data_encrypted)
    {
        hashtable_free (secure_hashtable_data_encrypted);
        secure_hashtable_data_encrypted = NULL;
    }
}

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
    if (secure_passphrase)
    {
        if (secure_buffer_display_values)
        {
            gui_chat_printf_y (secure_buffer, line++,
                               "%s%s = %s\"%s%s%s\"",
                               _("Passphrase"),
                               GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                               GUI_COLOR(GUI_COLOR_CHAT),
                               GUI_COLOR(GUI_COLOR_CHAT_VALUE),
                               secure_passphrase,
                               GUI_COLOR(GUI_COLOR_CHAT));
        }
        else
            gui_chat_printf_y (secure_buffer, line++, _("Passphrase is set"));
    }
    else
    {
        gui_chat_printf_y (secure_buffer, line++,
                           _("Passphrase is NOT set"));
    }

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
secure_buffer_input_cb (void *data, struct t_gui_buffer *buffer,
                        const char *input_data)
{
    /* make C compiler happy */
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
secure_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    /* make C compiler happy */
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
                                        &secure_buffer_input_cb, NULL,
                                        &secure_buffer_close_cb, NULL);
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
