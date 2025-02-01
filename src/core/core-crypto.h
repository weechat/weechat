/*
 * Copyright (C) 2018-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_CRYPTO_H
#define WEECHAT_CRYPTO_H

#define WEECRYPTO_TOTP_MIN_DIGITS    4
#define WEECRYPTO_TOTP_MAX_DIGITS    10

extern int weecrypto_get_hash_algo (const char *hash_algo);
extern int weecrypto_get_cipher (const char *cipher);
extern int weecrypto_hash (const void *data, int data_size, int hash_algo,
                           void *hash, int *hash_size);
extern int weecrypto_hash_file (const char *filename, int hash_algo,
                                void *hash, int *hash_size);
extern int weecrypto_hash_pbkdf2 (const void *data, int data_size,
                                  int hash_algo,
                                  const void *salt, int salt_size,
                                  int iterations,
                                  void *hash, int *hash_size);
extern int weecrypto_hmac (const void *key, int key_size,
                           const void *message, int message_size,
                           int hash_algo,
                           void *hash, int *hash_size);
extern char *weecrypto_totp_generate (const char *secret, time_t totp_time,
                                      int digits);
extern int weecrypto_totp_validate (const char *secret, time_t totp_time,
                                    int window, const char *otp);

#endif /* WEECHAT_CRYPTO_H */
