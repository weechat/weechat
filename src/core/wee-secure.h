/*
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

#ifndef __WEECHAT_SECURE_H
#define __WEECHAT_SECURE_H 1

#define SECURE_CONFIG_NAME "sec"

#define SECURE_ENV_PASSPHRASE       "WEECHAT_PASSPHRASE"
#define SECURE_SALT_DEFAULT         "WeeChat!"
#define SECURE_DATA_PASSPHRASE_FLAG "__passphrase__"

#define SECURE_BUFFER_NAME          "secured_data"

enum t_secure_config_hash_algo
{
    SECURE_CONFIG_HASH_SHA224 = 0,
    SECURE_CONFIG_HASH_SHA256,
    SECURE_CONFIG_HASH_SHA384,
    SECURE_CONFIG_HASH_SHA512,
};

enum t_secure_config_cipher
{
    SECURE_CONFIG_CIPHER_AES128 = 0,
    SECURE_CONFIG_CIPHER_AES192,
    SECURE_CONFIG_CIPHER_AES256,
};

extern struct t_config_file *secure_config_file;
extern struct t_config_section *secure_config_section_pwd;

extern struct t_config_option *secure_config_crypt_cipher;
extern struct t_config_option *secure_config_crypt_hash_algo;
extern struct t_config_option *secure_config_crypt_passphrase_file;
extern struct t_config_option *secure_config_crypt_salt;

extern char *secure_passphrase;
extern struct t_hashtable *secure_hashtable_data;
extern struct t_hashtable *secure_hashtable_data_encrypted;

extern struct t_gui_buffer *secure_buffer;
extern int secure_buffer_display_values;

extern int secure_decrypt_data_not_decrypted (const char *passphrase);
extern int secure_init ();
extern int secure_read ();
extern int secure_write ();
extern void secure_free ();
extern void secure_buffer_display ();
extern void secure_buffer_assign ();
extern void secure_buffer_open ();

#endif /* __WEECHAT_SECURE_H */
