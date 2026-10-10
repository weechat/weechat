/*
 * SPDX-FileCopyrightText: 2014-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test URL functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "src/core/core-hashtable.h"
#include "src/core/core-string.h"
#include "src/core/core-url.h"
#include "src/plugins/weechat-plugin.h"

extern struct t_url_constant url_proxy_types[];
extern struct t_url_constant url_protocols[];
extern int weeurl_search_constant (struct t_url_constant *constants,
                                   const char *name);
extern int weeurl_search_option (const char *name);
}

TEST_GROUP(CoreUrl)
{
};

/*
 * Test functions:
 *   weeurl_search_constant
 */

TEST(CoreUrl, SearchConstant)
{
    LONGS_EQUAL(-1, weeurl_search_constant (NULL, NULL));
    LONGS_EQUAL(-1, weeurl_search_constant (NULL, ""));
    LONGS_EQUAL(-1, weeurl_search_constant (NULL, "test"));
    LONGS_EQUAL(-1, weeurl_search_constant (url_proxy_types, NULL));
    LONGS_EQUAL(-1, weeurl_search_constant (url_proxy_types, ""));
    LONGS_EQUAL(-1, weeurl_search_constant (url_proxy_types, "does_not_exist"));

    CHECK(weeurl_search_constant (url_proxy_types, "socks4") >= 0);
    CHECK(weeurl_search_constant (url_proxy_types, "SOCKS4") >= 0);

    CHECK(weeurl_search_constant (url_protocols, "https") >= 0);
    CHECK(weeurl_search_constant (url_protocols, "HTTPS") >= 0);
}

/*
 * Test functions:
 *   weeurl_get_mask_value
 */

TEST(CoreUrl, GetMaskValue)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   weeurl_search_option
 */

TEST(CoreUrl, SearchOption)
{
    LONGS_EQUAL(-1, weeurl_search_option (NULL));
    LONGS_EQUAL(-1, weeurl_search_option (""));
    LONGS_EQUAL(-1, weeurl_search_option ("does_not_exist"));

    /* string */
    CHECK(weeurl_search_option ("interface") >= 0);
    CHECK(weeurl_search_option ("INTERFACE") >= 0);

    /* long */
    CHECK(weeurl_search_option ("proxyport") >= 0);
    CHECK(weeurl_search_option ("PROXYPORT") >= 0);

    /* long long */
    CHECK(weeurl_search_option ("resume_from_large") >= 0);
    CHECK(weeurl_search_option ("RESUME_FROM_LARGE") >= 0);

    /* list */
    CHECK(weeurl_search_option ("httpheader") >= 0);
    CHECK(weeurl_search_option ("HTTPHEADER") >= 0);

    /* mask */
    CHECK(weeurl_search_option ("httpauth") >= 0);
    CHECK(weeurl_search_option ("HTTPAUTH") >= 0);
}

/*
 * Test functions:
 *   weeurl_read
 */

TEST(CoreUrl, Read)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   weeurl_write
 */

TEST(CoreUrl, Write)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   weeurl_option_map_cb
 */

TEST(CoreUrl, OptionMapCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   weeurl_set_proxy
 */

TEST(CoreUrl, SetProxy)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   weeurl_download
 */

TEST(CoreUrl, Download)
{
    struct t_hashtable *output;
    char *path, *real_path, *url;
    FILE *file;

    output = hashtable_new (32, WEECHAT_HASHTABLE_STRING,
                            WEECHAT_HASHTABLE_STRING, NULL, NULL);
    CHECK(output);

    /* Invalid URL */
    LONGS_EQUAL(1, weeurl_download (NULL, NULL, 0, output, NULL));
    STRCMP_EQUAL("invalid URL", (const char *)hashtable_get (output, "error"));
    hashtable_remove_all (output);
    LONGS_EQUAL(1, weeurl_download ("", NULL, 0, output, NULL));
    hashtable_remove_all (output);

    /* Local file */
    path = string_eval_path_home ("${weechat_data_dir}/test_url_download.txt",
                                  NULL, NULL, NULL);
    CHECK(path);
    file = fopen (path, "w");
    CHECK(file);
    fputs ("test file content\n", file);
    fclose (file);
    /* A "file://" URL needs an absolute path. */
    real_path = realpath (path, NULL);
    CHECK(real_path);
    string_asprintf (&url, "file://%s", real_path);
    CHECK(url);
    LONGS_EQUAL(0, weeurl_download (url, NULL, 10000, output, NULL));
    STRCMP_EQUAL("0", (const char *)hashtable_get (output, "response_code"));
    STRCMP_EQUAL("test file content\n",
                 (const char *)hashtable_get (output, "output"));
    POINTERS_EQUAL(NULL, hashtable_get (output, "error"));
    POINTERS_EQUAL(NULL, hashtable_get (output, "error_code_curl"));
    hashtable_remove_all (output);
    free (url);
    unlink (path);

    /* Missing file: curl error */
    string_asprintf (&url, "file://%s", real_path);
    CHECK(url);
    LONGS_EQUAL(2, weeurl_download (url, NULL, 10000, output, NULL));
    CHECK(hashtable_get (output, "error"));
    STRCMP_EQUAL("37", (const char *)hashtable_get (output, "error_code_curl"));
    POINTERS_EQUAL(NULL, hashtable_get (output, "response_code"));
    hashtable_remove_all (output);
    free (url);

    free (real_path);
    free (path);
    hashtable_free (output);
}

/*
 * Test functions:
 *   weeurl_option_add_to_infolist
 */

TEST(CoreUrl, AddToInfolist)
{
    /* TODO: write tests */
}
