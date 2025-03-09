/*
 * Copyright (C) 2013-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_SECURE_H
#define WEECHAT_SECURE_H

#include <time.h>

#define SECURE_ENV_PASSPHRASE        "WEECHAT_PASSPHRASE"
#define SECURE_PASSPHRASE_MAX_LENGTH 4096
#define SECURE_SALT_DEFAULT          "WeeChat!"
#define SECURE_DATA_PASSPHRASE_FLAG  "__passphrase__"
#define SECURE_SALT_SIZE             8

extern char *secure_passphrase;
extern struct t_hashtable *secure_hashtable_data;
extern struct t_hashtable *secure_hashtable_data_encrypted;
extern int secure_data_encrypted;
extern char *secure_decrypt_error[];

extern int secure_encrypt_data (const char *data, int length_data,
                                int hash_algo, int cipher,
                                const char *passphrase, char **encrypted,
                                int *length_encrypted);
extern int secure_decrypt_data (const char *buffer, int length_buffer,
                                int hash_algo, int cipher,
                                const char *passphrase, char **decrypted,
                                int *length_decrypted);
extern int secure_decrypt_data_not_decrypted (const char *passphrase);
extern int secure_init (void);
extern void secure_end (void);

#endif /* WEECHAT_SECURE_H */
