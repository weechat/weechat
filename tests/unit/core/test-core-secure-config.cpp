/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test secured data configuration functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"
#include "tests-record.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/core/weechat.h"
#include "src/core/core-config-file.h"
#include "src/core/core-crypto.h"
#include "src/core/core-dir.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-secure.h"
#include "src/plugins/plugin.h"
#include "src/core/core-secure-config.h"
#include "src/core/core-string.h"
#include "src/gui/gui-window.h"

extern int secure_config_loading;
extern struct t_config_section *secure_config_section_data;
extern char *secure_config_get_passphrase_from_command (const char *command);
extern int secure_config_reload_cb (const void *pointer, void *data,
                                    struct t_config_file *config_file);
extern int secure_config_check_crypt_option_cb (const void *pointer,
                                                void *data,
                                                struct t_config_option *option,
                                                const char *value);
extern int secure_config_data_read_cb (const void *pointer, void *data,
                                       struct t_config_file *config_file,
                                       struct t_config_section *section,
                                       const char *option_name,
                                       const char *value);
}

#define TEST_PASSPHRASE "test_passphrase"

/*
 * Encrypt data with a passphrase and return it encoded in base16, as stored
 * in file sec.conf.
 *
 * Note: result must be freed after use.
 */

char *
test_secure_config_encrypt (const char *passphrase, const char *data)
{
    char *buffer, *buffer_base16;
    int length_buffer, rc;

    buffer = NULL;
    length_buffer = 0;
    rc = secure_encrypt_data (
        data, strlen (data) + 1,
        weecrypto_get_hash_algo (
            config_file_option_string (secure_config_crypt_hash_algo)),
        weecrypto_get_cipher (
            config_file_option_string (secure_config_crypt_cipher)),
        passphrase,
        &buffer,
        &length_buffer);
    if ((rc != 0) || !buffer)
        return NULL;

    buffer_base16 = (char *)malloc ((length_buffer * 2) + 1);
    if (buffer_base16)
        string_base16_encode (buffer, length_buffer, buffer_base16);
    free (buffer);

    return buffer_base16;
}

/*
 * Return content of file sec.conf.
 *
 * Note: result must be freed after use.
 */

char *
test_secure_config_get_file (void)
{
    char *path, *content;

    if (string_asprintf (&path, "%s/sec.conf", weechat_config_dir) < 0)
        return NULL;
    content = dir_file_get_content (path);
    free (path);

    return content;
}

TEST_GROUP(CoreSecureConfig)
{
    char *old_passphrase = NULL;

    void setup ()
    {
        /* The passphrase must never be asked to the user in tests. */
        CHECK(gui_init_ok);

        old_passphrase = (secure_passphrase) ?
            strdup (secure_passphrase) : NULL;
        free (secure_passphrase);
        secure_passphrase = NULL;
        hashtable_remove_all (secure_hashtable_data);
        hashtable_remove_all (secure_hashtable_data_encrypted);
        secure_data_encrypted = 0;
    }

    void teardown ()
    {
        record_stop ();
        hashtable_remove_all (secure_hashtable_data);
        hashtable_remove_all (secure_hashtable_data_encrypted);
        secure_data_encrypted = 0;
        free (secure_passphrase);
        secure_passphrase = old_passphrase;
        old_passphrase = NULL;
        config_file_option_reset (secure_config_crypt_cipher, 1);
        config_file_option_reset (secure_config_crypt_hash_algo, 1);
        config_file_option_reset (secure_config_crypt_passphrase_command, 1);
        config_file_option_reset (secure_config_crypt_salt, 1);
        secure_config_write ();
    }

    /* Call the read callback for a data in section "data". */

    int read_data (const char *name, const char *value)
    {
        return secure_config_data_read_cb (NULL, NULL,
                                           secure_config_file,
                                           secure_config_section_data,
                                           name, value);
    }
};

/*
 * Test functions:
 *   secure_config_get_passphrase_from_command
 */

TEST(CoreSecureConfig, GetPassphraseFromCommand)
{
    char *str;

    WEE_TEST_STR("secret",
                 secure_config_get_passphrase_from_command ("echo secret"));

    /* Only the first line is used, "\r" is ignored. */
    WEE_TEST_STR("abc",
                 secure_config_get_passphrase_from_command (
                     "printf 'abc\\r\\ndef\\n'"));

    /* Empty output. */
    WEE_TEST_STR(NULL, secure_config_get_passphrase_from_command ("true"));
    WEE_TEST_STR(NULL,
                 secure_config_get_passphrase_from_command ("printf '\\n'"));

    /* Passphrase is truncated to the max length. */
    str = secure_config_get_passphrase_from_command ("printf '%05000d' 1");
    CHECK(str);
    LONGS_EQUAL(SECURE_PASSPHRASE_MAX_LENGTH, strlen (str));
    LONGS_EQUAL('0', str[0]);
    LONGS_EQUAL('0', str[SECURE_PASSPHRASE_MAX_LENGTH - 1]);
    free (str);
}

/*
 * Test functions:
 *   secure_config_check_crypt_option_cb
 */

TEST(CoreSecureConfig, CheckCryptOptionCb)
{
    /* No encrypted data: any change is allowed. */
    LONGS_EQUAL(1, secure_config_check_crypt_option_cb (
                    NULL, NULL, secure_config_crypt_cipher, "aes128"));

    /* Encrypted data: changes are refused, except when reading the file. */
    hashtable_set (secure_hashtable_data_encrypted, "test", "0123");
    record_start ();
    LONGS_EQUAL(0, secure_config_check_crypt_option_cb (
                    NULL, NULL, secure_config_crypt_cipher, "aes128"));
    record_stop ();
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "Option sec.crypt.cipher cannot be changed because "
                     "there are still encrypted data",
                     NULL);
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (secure_config_crypt_hash_algo,
                                        "sha512", 1));
    STRCMP_EQUAL("sha256",
                 config_file_option_string (secure_config_crypt_hash_algo));

    secure_config_loading = 1;
    LONGS_EQUAL(1, secure_config_check_crypt_option_cb (
                    NULL, NULL, secure_config_crypt_cipher, "aes128"));
    secure_config_loading = 0;
}

/*
 * Test functions:
 *   secure_config_data_read_cb
 */

TEST(CoreSecureConfig, DataReadCb)
{
    char *value;

    /* Invalid name or value: ignored. */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                read_data (NULL, "value"));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                read_data ("name", NULL));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                read_data ("name", ""));
    LONGS_EQUAL(0, secure_hashtable_data->items_count);

    /* Data not encrypted. */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                read_data (SECURE_DATA_PASSPHRASE_FLAG, "off"));
    LONGS_EQUAL(0, secure_data_encrypted);
    read_data ("name1", "value1");
    STRCMP_EQUAL("value1",
                 (const char *)hashtable_get (secure_hashtable_data, "name1"));

    /* Data encrypted, passphrase is set: data is decrypted. */
    secure_passphrase = strdup (TEST_PASSPHRASE);
    read_data (SECURE_DATA_PASSPHRASE_FLAG, "on");
    LONGS_EQUAL(1, secure_data_encrypted);
    value = test_secure_config_encrypt (TEST_PASSPHRASE, "value2");
    CHECK(value);
    read_data ("name2", value);
    STRCMP_EQUAL("value2",
                 (const char *)hashtable_get (secure_hashtable_data, "name2"));
    POINTERS_EQUAL(NULL, hashtable_get (secure_hashtable_data_encrypted,
                                        "name2"));

    /* Data encrypted, passphrase not set: data is kept encrypted. */
    free (secure_passphrase);
    secure_passphrase = NULL;
    record_start ();
    read_data ("name3", value);
    record_stop ();
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "Passphrase is not set, unable to decrypt data \"name3\"",
                     NULL);
    POINTERS_EQUAL(NULL, hashtable_get (secure_hashtable_data, "name3"));
    STRCMP_EQUAL(value,
                 (const char *)hashtable_get (secure_hashtable_data_encrypted,
                                              "name3"));
    free (value);
}

/*
 * Test functions:
 *   secure_config_data_write_cb
 *   secure_config_data_write_map_cb
 *   secure_config_write
 *   secure_config_read
 */

TEST(CoreSecureConfig, WriteReadPlain)
{
    char *content;

    /* No data: no passphrase flag. */
    LONGS_EQUAL(WEECHAT_CONFIG_WRITE_OK, secure_config_write ());
    content = test_secure_config_get_file ();
    CHECK(content);
    POINTERS_EQUAL(NULL, strstr (content, SECURE_DATA_PASSPHRASE_FLAG));
    free (content);

    /* No passphrase: data is written as plain text. */
    hashtable_set (secure_hashtable_data, "name1", "value1");
    hashtable_set (secure_hashtable_data, "name2", "value 2");
    LONGS_EQUAL(WEECHAT_CONFIG_WRITE_OK, secure_config_write ());
    content = test_secure_config_get_file ();
    CHECK(content);
    CHECK(strstr (content,
                  "[data]\n"
                  SECURE_DATA_PASSPHRASE_FLAG " = off\n"
                  "name1 = \"value1\"\n"
                  "name2 = \"value 2\"\n"));
    free (content);

    hashtable_remove_all (secure_hashtable_data);
    LONGS_EQUAL(WEECHAT_CONFIG_READ_OK, secure_config_read ());
    LONGS_EQUAL(0, secure_data_encrypted);
    LONGS_EQUAL(2, secure_hashtable_data->items_count);
    STRCMP_EQUAL("value1",
                 (const char *)hashtable_get (secure_hashtable_data, "name1"));
    STRCMP_EQUAL("value 2",
                 (const char *)hashtable_get (secure_hashtable_data, "name2"));
    LONGS_EQUAL(0, secure_hashtable_data_encrypted->items_count);
}

/*
 * Test functions:
 *   secure_config_data_write_cb
 *   secure_config_data_write_map_cb
 *   secure_config_data_write_map_encrypted_cb
 *   secure_config_write
 *   secure_config_read
 */

TEST(CoreSecureConfig, WriteReadEncrypted)
{
    char *content, *content2;

    /* Passphrase set: data is encrypted. */
    secure_passphrase = strdup (TEST_PASSPHRASE);
    hashtable_set (secure_hashtable_data, "name1", "value1");
    hashtable_set (secure_hashtable_data, "name2", "value 2");
    LONGS_EQUAL(WEECHAT_CONFIG_WRITE_OK, secure_config_write ());
    content = test_secure_config_get_file ();
    CHECK(content);
    CHECK(strstr (content, SECURE_DATA_PASSPHRASE_FLAG " = on\n"));
    CHECK(strstr (content, "name1 = \""));
    CHECK(strstr (content, "name2 = \""));
    POINTERS_EQUAL(NULL, strstr (content, "value1"));
    POINTERS_EQUAL(NULL, strstr (content, "value 2"));

    /* Read file with the passphrase: data is decrypted. */
    hashtable_remove_all (secure_hashtable_data);
    LONGS_EQUAL(WEECHAT_CONFIG_READ_OK, secure_config_read ());
    LONGS_EQUAL(1, secure_data_encrypted);
    STRCMP_EQUAL("value1",
                 (const char *)hashtable_get (secure_hashtable_data, "name1"));
    STRCMP_EQUAL("value 2",
                 (const char *)hashtable_get (secure_hashtable_data, "name2"));
    LONGS_EQUAL(0, secure_hashtable_data_encrypted->items_count);

    /* Read file without passphrase: data is kept encrypted. */
    free (secure_passphrase);
    secure_passphrase = NULL;
    hashtable_remove_all (secure_hashtable_data);
    record_start ();
    LONGS_EQUAL(WEECHAT_CONFIG_READ_OK, secure_config_read ());
    record_stop ();
    LONGS_EQUAL(0, secure_hashtable_data->items_count);
    LONGS_EQUAL(2, secure_hashtable_data_encrypted->items_count);

    /* Encrypted data is written as-is. */
    LONGS_EQUAL(WEECHAT_CONFIG_WRITE_OK, secure_config_write ());
    content2 = test_secure_config_get_file ();
    CHECK(content2);
    CHECK(strstr (content2, SECURE_DATA_PASSPHRASE_FLAG " = on\n"));
    CHECK(strstr (content2,
                  (const char *)hashtable_get (secure_hashtable_data_encrypted,
                                               "name1")));
    CHECK(strstr (content2,
                  (const char *)hashtable_get (secure_hashtable_data_encrypted,
                                               "name2")));
    free (content);
    free (content2);

    /* Decrypt data with the passphrase. */
    LONGS_EQUAL(2, secure_decrypt_data_not_decrypted (TEST_PASSPHRASE));
    STRCMP_EQUAL("value1",
                 (const char *)hashtable_get (secure_hashtable_data, "name1"));
    LONGS_EQUAL(0, secure_hashtable_data_encrypted->items_count);
}

/*
 * Test functions:
 *   secure_config_reload_cb
 */

TEST(CoreSecureConfig, ReloadCb)
{
    hashtable_set (secure_hashtable_data, "name1", "value1");
    LONGS_EQUAL(WEECHAT_CONFIG_WRITE_OK, secure_config_write ());

    /* Reload: data is read again from file. */
    hashtable_set (secure_hashtable_data, "name2", "value2");
    LONGS_EQUAL(WEECHAT_CONFIG_READ_OK,
                secure_config_reload_cb (NULL, NULL, secure_config_file));
    LONGS_EQUAL(1, secure_hashtable_data->items_count);
    STRCMP_EQUAL("value1",
                 (const char *)hashtable_get (secure_hashtable_data, "name1"));

    /* Encrypted data: reload is refused. */
    hashtable_set (secure_hashtable_data_encrypted, "name3", "0123");
    record_start ();
    LONGS_EQUAL(WEECHAT_CONFIG_READ_FILE_NOT_FOUND,
                secure_config_reload_cb (NULL, NULL, secure_config_file));
    record_stop ();
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "Unable to reload file sec.conf because there is still "
                     "encrypted data (use /secure decrypt, see /help secure)",
                     NULL);
    LONGS_EQUAL(1, secure_hashtable_data->items_count);
}

/*
 * Test functions:
 *   secure_config_reload_cb (with a wrong passphrase)
 *   secure_config_data_read_cb
 */

TEST(CoreSecureConfig, ReloadCbWrongPassphrase)
{
    char *content;

    secure_passphrase = strdup (TEST_PASSPHRASE);
    hashtable_set (secure_hashtable_data, "name1", "value1");
    LONGS_EQUAL(WEECHAT_CONFIG_WRITE_OK, secure_config_write ());

    /* Wrong passphrase: data is kept encrypted. */
    free (secure_passphrase);
    secure_passphrase = strdup ("wrong");
    record_start ();
    LONGS_EQUAL(WEECHAT_CONFIG_READ_OK,
                secure_config_reload_cb (NULL, NULL, secure_config_file));
    record_stop ();
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "Wrong passphrase, unable to decrypt data \"name1\"",
                     NULL);
    LONGS_EQUAL(0, secure_hashtable_data->items_count);
    LONGS_EQUAL(1, secure_hashtable_data_encrypted->items_count);

    /* Encrypted data is not lost when the file is written. */
    LONGS_EQUAL(WEECHAT_CONFIG_WRITE_OK, secure_config_write ());
    content = test_secure_config_get_file ();
    CHECK(content);
    CHECK(strstr (content, "name1 = \""));
    free (content);

    /* Data can be decrypted with the right passphrase. */
    LONGS_EQUAL(1, secure_decrypt_data_not_decrypted (TEST_PASSPHRASE));
    STRCMP_EQUAL("value1",
                 (const char *)hashtable_get (secure_hashtable_data, "name1"));
    LONGS_EQUAL(0, secure_hashtable_data_encrypted->items_count);
}
