/*
 * test-core-calc.cpp - test calculations functions
 *
 * Copyright (C) 2019 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/core/wee-calc.h"
}

#define WEE_CHECK_CALC(__result, __expr)                                \
    value = calc_expression (__expr);                                   \
    STRCMP_EQUAL(__result, value);                                      \
    free (value);

TEST_GROUP(CoreCalc)
{
};

/*
 * Tests functions:
 *   calc_expression
 */

TEST(CoreCalc, Expression)
{
    char *value;

    /* invalid expressions */
    WEE_CHECK_CALC("0", NULL);
    WEE_CHECK_CALC("0", "");
    WEE_CHECK_CALC("0", "(");
    WEE_CHECK_CALC("0", ")");
    WEE_CHECK_CALC("0", "+");
    WEE_CHECK_CALC("0", "-");
    WEE_CHECK_CALC("0", "*");
    WEE_CHECK_CALC("0", "/");
    WEE_CHECK_CALC("0", "%");
    WEE_CHECK_CALC("0", "0/0");
    WEE_CHECK_CALC("0", "0//0");
    WEE_CHECK_CALC("0", "0%0");

    /* no operator */
    WEE_CHECK_CALC("123", "123");
    WEE_CHECK_CALC("1.5", "1.5");

    /* addition */
    WEE_CHECK_CALC("-3", "-4+1");
    WEE_CHECK_CALC("3", "1+2");
    WEE_CHECK_CALC("4", " 1  +  3 ");

    /* subtraction */
    WEE_CHECK_CALC("5", "8-3");
    WEE_CHECK_CALC("-5", "3-8");

    /* unary minus */
    WEE_CHECK_CALC("0", "-0");
    WEE_CHECK_CALC("-0.001", "-0.001");
    WEE_CHECK_CALC("0", "(-0)");
    WEE_CHECK_CALC("0", "0-0");
    WEE_CHECK_CALC("-1", "-1");
    WEE_CHECK_CALC("-2", "-1+-1");
    WEE_CHECK_CALC("0", "-1+1");
    WEE_CHECK_CALC("-2", "-3+1");
    WEE_CHECK_CALC("-3", "1+-4");
    WEE_CHECK_CALC("-4", "2*-2");
    WEE_CHECK_CALC("-6", "-3*2");
    WEE_CHECK_CALC("9", "-3*-3");
    WEE_CHECK_CALC("-6", "3*(-2)");
    WEE_CHECK_CALC("6", "-3*(-2)");
    WEE_CHECK_CALC("12", "(-3)*(-4)");
    WEE_CHECK_CALC("15", "(-3)*-5");
    WEE_CHECK_CALC("9", "(-3)*(-4+1)");

    /* multiplication */
    WEE_CHECK_CALC("20", "10*2");
    WEE_CHECK_CALC("-8", "-2*4");
    WEE_CHECK_CALC("152415765279684", "12345678*12345678");

    /* division */
    WEE_CHECK_CALC("2", "6/3");
    WEE_CHECK_CALC("2.5", "10/4");

    /* floor division */
    WEE_CHECK_CALC("2", "10//4");

    /* modulo */
    WEE_CHECK_CALC("4", "9%5");
    WEE_CHECK_CALC("0.2", "9.2%3");
    WEE_CHECK_CALC("-2", "-2%4");
    WEE_CHECK_CALC("0", "-2%2");

    /* power */
    WEE_CHECK_CALC("1", "0**0");
    WEE_CHECK_CALC("0", "0**1");
    WEE_CHECK_CALC("1", "1**0");
    WEE_CHECK_CALC("1", "2**0");
    WEE_CHECK_CALC("2", "2**1");
    WEE_CHECK_CALC("4", "2**2");
    WEE_CHECK_CALC("8", "2**3");
    WEE_CHECK_CALC("4294967296", "2**32");
    WEE_CHECK_CALC("0.5", "2**-1");
    WEE_CHECK_CALC("0.25", "2**-2");

    /* multiple operators */
    WEE_CHECK_CALC("11", "5+2*3");

    /* expressions with decimal numbers */
    WEE_CHECK_CALC("12.5", "10.5+2");
    WEE_CHECK_CALC("3.3333333333", "10/3");
    WEE_CHECK_CALC("0.1428571429", "1/7");
    WEE_CHECK_CALC("0.0008103728", "1/1234");
    WEE_CHECK_CALC("0.0000810045", "1/12345");
    WEE_CHECK_CALC("0.0000081001", "1/123456");
    WEE_CHECK_CALC("0.00000081", "1/1234567");
    WEE_CHECK_CALC("0.000000081", "1/12345678");
    WEE_CHECK_CALC("0.0000000081", "1/123456789");
    WEE_CHECK_CALC("0.0000000008", "1/1234567890");
    WEE_CHECK_CALC("0.0000000001", "1/12345678901");
    WEE_CHECK_CALC("0", "1/123456789012");

    /* expressions with parentheses */
    WEE_CHECK_CALC("6", "((6))");
    WEE_CHECK_CALC("-7.234", "((-7.234))");
    WEE_CHECK_CALC("21", "(5+2)*3");
    WEE_CHECK_CALC("3.15", "(1.5+2)*(1.8/2)");
    WEE_CHECK_CALC("-1.26", "(1.5+2)*(1.8/(2-7))");
}
