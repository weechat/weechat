/*
 * SPDX-FileCopyrightText: 2013-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_SECURE_CONFIG_H
#define WEECHAT_SECURE_CONFIG_H

#define SECURE_CONFIG_NAME "sec"
#define SECURE_CONFIG_PRIO_NAME "120000|sec"

extern struct t_config_file *secure_config_file;
extern struct t_config_section *secure_config_section_pwd;

extern struct t_config_option *secure_config_crypt_cipher;
extern struct t_config_option *secure_config_crypt_hash_algo;
extern struct t_config_option *secure_config_crypt_passphrase_command;
extern struct t_config_option *secure_config_crypt_salt;

extern int secure_config_read (void);
extern int secure_config_write (void);
extern int secure_config_init (void);
extern void secure_config_free (void);

#endif /* WEECHAT_SECURE_CONFIG_H */
