/*
 * test-secure.cpp - test secured data functions
 *
 * Copyright (C) 2018 Sébastien Helleu <flashcode@flashtux.org>
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

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "src/core/wee-secure.h"
}

#define TOTP_SECRET "secretpasswordbase32"

#define WEE_CHECK_TOTP_GENERATE(__result, __secret, __time, __digits)   \
    totp = secure_totp_generate (__secret, __time, __digits);           \
    if (__result == NULL)                                               \
    {                                                                   \
        POINTERS_EQUAL(NULL, totp);                                     \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__result, totp);                                   \
        free (totp);                                                    \
    }
#define WEE_CHECK_TOTP_VALIDATE(__result, __secret, __time, __window,   \
                                __otp)                                  \
    LONGS_EQUAL(__result, secure_totp_validate (__secret, __time,       \
                                                __window, __otp));

TEST_GROUP(CoreSecure)
{
};

/*
 * Tests functions:
 *   secure_totp_generate
 */

TEST(CoreSecure, TotpGenerate)
{
    char *totp;

    /* invalid secret */
    WEE_CHECK_TOTP_GENERATE(NULL, NULL, 0, 6);
    WEE_CHECK_TOTP_GENERATE(NULL, "", 0, 6);
    WEE_CHECK_TOTP_GENERATE(NULL, "not_in_base32_0189", 0, 6);

    /* invalid number of digits (must be between 4 and 10) */
    WEE_CHECK_TOTP_GENERATE(NULL, TOTP_SECRET, 0, 3);
    WEE_CHECK_TOTP_GENERATE(NULL, TOTP_SECRET, 0, 11);

    /* TOTP with 6 digits */
    WEE_CHECK_TOTP_GENERATE("065486", TOTP_SECRET, 1540624066, 6);
    WEE_CHECK_TOTP_GENERATE("640073", TOTP_SECRET, 1540624085, 6);
    WEE_CHECK_TOTP_GENERATE("725645", TOTP_SECRET, 1540624110, 6);

    /* TOTP with 7 digits */
    WEE_CHECK_TOTP_GENERATE("0065486", TOTP_SECRET, 1540624066, 7);
    WEE_CHECK_TOTP_GENERATE("6640073", TOTP_SECRET, 1540624085, 7);
    WEE_CHECK_TOTP_GENERATE("4725645", TOTP_SECRET, 1540624110, 7);

    /* TOTP with 8 digits */
    WEE_CHECK_TOTP_GENERATE("40065486", TOTP_SECRET, 1540624066, 8);
    WEE_CHECK_TOTP_GENERATE("16640073", TOTP_SECRET, 1540624085, 8);
    WEE_CHECK_TOTP_GENERATE("94725645", TOTP_SECRET, 1540624110, 8);
}

/*
 * Tests functions:
 *   secure_totp_validate
 */

TEST(CoreSecure, TotpValidate)
{
    /* invalid secret */
    WEE_CHECK_TOTP_VALIDATE(0, NULL, 0, 0, "123456");
    WEE_CHECK_TOTP_VALIDATE(0, "", 0, 0, "123456");
    WEE_CHECK_TOTP_VALIDATE(0, "not_in_base32_0189", 0, 0, "123456");

    /* invalid window (must be ≥ 0) */
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 0, -1, "123456");

    /* invalid OTP */
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 0, 0, NULL);
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 0, 0, "");

    /* validation error (wrong OTP) */
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1540624110, 0, "065486");
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1540624110, 1, "065486");

    /* TOTP with 6 digits */
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624066, 0, "065486");
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624085, 0, "640073");
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624110, 0, "725645");

    /* TOTP with 7 digits */
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624066, 0, "0065486");
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624085, 0, "6640073");
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624110, 0, "4725645");

    /* TOTP with 7 digits */
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624066, 0, "40065486");
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624085, 0, "16640073");
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624110, 0, "94725645");

    /* TOTP with 6 digits, using window */
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1540624110, 0, "065486");
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1540624110, 1, "065486");
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624110, 2, "065486");

    /* TOTP with 7 digits, using window */
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1540624110, 0, "0065486");
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1540624110, 1, "0065486");
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624110, 2, "0065486");

    /* TOTP with 8 digits, using window */
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1540624110, 0, "40065486");
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1540624110, 1, "40065486");
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624110, 2, "40065486");
}
