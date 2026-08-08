/*
 * SPDX-FileCopyrightText: 2018-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
