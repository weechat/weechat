/*
 * test-core-url.cpp - test URL functions
 *
 * Copyright (C) 2014-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "src/core/core-url.h"

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
 * Tests functions:
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
 * Tests functions:
 *   weeurl_get_mask_value
 */

TEST(CoreUrl, GetMaskValue)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
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
 * Tests functions:
 *   weeurl_read
 */

TEST(CoreUrl, Read)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   weeurl_write
 */

TEST(CoreUrl, Write)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   weeurl_option_map_cb
 */

TEST(CoreUrl, OptionMapCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   weeurl_set_proxy
 */

TEST(CoreUrl, SetProxy)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   weeurl_download
 */

TEST(CoreUrl, Download)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   weeurl_option_add_to_infolist
 */

TEST(CoreUrl, AddToInfolist)
{
    /* TODO: write tests */
}
