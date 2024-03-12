/*
 * test-core-calc.cpp - test calculations functions
 *
 * Copyright (C) 2019-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include "src/core/core-arraylist.h"
#include "src/core/core-calc.h"

extern void calc_list_free_cb (void *data, struct t_arraylist *arraylist,
                               void *pointer);
extern int calc_operator_precedence (const char *oper);
extern double calc_pop_value (struct t_arraylist *list_values);
extern double calc_operation (const char *oper, double value1, double value2);
extern void calc_operation_stacks (struct t_arraylist *list_values,
                                   struct t_arraylist *list_ops);
extern int calc_sanitize_decimal_number (char *string);
extern void calc_format_result (double value, char *result, int max_size);
}

#define WEE_CHECK_SANITIZE_DECIMAL_NUMBER(__result, __result_string,    \
                                          __number)                     \
    snprintf (str_number, sizeof (str_number), "%s", __number);         \
    LONGS_EQUAL(__result, calc_sanitize_decimal_number (str_number));   \
    STRCMP_EQUAL(__result_string, str_number);

#define WEE_CHECK_FORMAT_RESULT(__result, __value)                      \
    calc_format_result (__value, str_result, sizeof (str_result));      \
    STRCMP_EQUAL(__result, str_result);

#define WEE_CHECK_CALC(__result, __expr)                                \
    value = calc_expression (__expr);                                   \
    STRCMP_EQUAL(__result, value);                                      \
    free (value);

TEST_GROUP(CoreCalc)
{
};

/*
 * Tests functions:
 *   calc_operator_precedence
 */

TEST(CoreCalc, OperatorPrecedence)
{
    LONGS_EQUAL(0, calc_operator_precedence (NULL));
    LONGS_EQUAL(0, calc_operator_precedence (""));
    LONGS_EQUAL(0, calc_operator_precedence ("$"));

    LONGS_EQUAL(1, calc_operator_precedence ("+"));
    LONGS_EQUAL(1, calc_operator_precedence ("-"));

    LONGS_EQUAL(2, calc_operator_precedence ("*"));
    LONGS_EQUAL(2, calc_operator_precedence ("/"));
    LONGS_EQUAL(2, calc_operator_precedence ("//"));
    LONGS_EQUAL(2, calc_operator_precedence ("%"));
    LONGS_EQUAL(2, calc_operator_precedence ("**"));
}

/*
 * Tests functions:
 *   calc_pop_value
 *   calc_list_free_cb
 */

TEST(CoreCalc, PopValue)
{
    struct t_arraylist *list_values;
    double *ptr_value;

    list_values = arraylist_new (32, 0, 1,
                                 NULL, NULL,
                                 &calc_list_free_cb, NULL);

    DOUBLES_EQUAL(0, calc_pop_value (NULL), 0.001);
    DOUBLES_EQUAL(0, calc_pop_value (list_values), 0.001);

    ptr_value = (double *)malloc (sizeof (*ptr_value));
    *ptr_value = 123.5;
    arraylist_add (list_values, ptr_value);
    LONGS_EQUAL(1, list_values->size);

    DOUBLES_EQUAL(123.5, calc_pop_value (list_values), 0.001);
    LONGS_EQUAL(0, list_values->size);

    ptr_value = (double *)malloc (sizeof (*ptr_value));
    *ptr_value = 123.5;
    arraylist_add (list_values, ptr_value);
    LONGS_EQUAL(1, list_values->size);

    ptr_value = (double *)malloc (sizeof (*ptr_value));
    *ptr_value = 456.2;
    arraylist_add (list_values, ptr_value);
    LONGS_EQUAL(2, list_values->size);

    DOUBLES_EQUAL(456.2, calc_pop_value (list_values), 0.001);
    LONGS_EQUAL(1, list_values->size);

    DOUBLES_EQUAL(123.5, calc_pop_value (list_values), 0.001);
    LONGS_EQUAL(0, list_values->size);

    arraylist_free (list_values);
}

/*
 * Tests functions:
 *   calc_operation
 */

TEST(CoreCalc, Operation)
{
    DOUBLES_EQUAL(0, calc_operation (NULL, 2, 3), 0.001);
    DOUBLES_EQUAL(0, calc_operation ("", 2, 3), 0.001);
    DOUBLES_EQUAL(0, calc_operation ("", 2, 3), 0.001);
    DOUBLES_EQUAL(0, calc_operation ("$", 2, 3), 0.001);

    DOUBLES_EQUAL(5.2, calc_operation ("+", 2, 3.2), 0.001);
    DOUBLES_EQUAL(-1.2, calc_operation ("-", 2, 3.2), 0.001);
    DOUBLES_EQUAL(6.4, calc_operation ("*", 2, 3.2), 0.001);
    DOUBLES_EQUAL(0.625, calc_operation ("/", 2, 3.2), 0.001);
    DOUBLES_EQUAL(2, calc_operation ("//", 7, 3), 0.001);
    DOUBLES_EQUAL(3.3, calc_operation ("%", 9, 5.7), 0.001);
    DOUBLES_EQUAL(256, calc_operation ("**", 2, 8), 0.001);
}

/*
 * Tests functions:
 *   calc_operation_stacks
 */

TEST(CoreCalc, OperationStacks)
{
    struct t_arraylist *list_values, *list_ops;
    double *ptr_value;
    char *ptr_op;

    calc_operation_stacks (NULL, NULL);

    list_values = arraylist_new (32, 0, 1,
                                 NULL, NULL,
                                 &calc_list_free_cb, NULL);

    list_ops = arraylist_new (32, 0, 1,
                              NULL, NULL,
                              &calc_list_free_cb, NULL);

    calc_operation_stacks (list_values, list_ops);

    ptr_value = (double *)malloc (sizeof (*ptr_value));
    *ptr_value = 123.5;
    arraylist_add (list_values, ptr_value);

    ptr_value = (double *)malloc (sizeof (*ptr_value));
    *ptr_value = 456.2;
    arraylist_add (list_values, ptr_value);

    ptr_op = strdup ("+");
    arraylist_add (list_ops, ptr_op);

    calc_operation_stacks (list_values, list_ops);

    ptr_value = (double *)arraylist_get (list_values, 0);
    DOUBLES_EQUAL(579.7, *ptr_value, 0.001);

    arraylist_free (list_values);
    arraylist_free (list_ops);
}



/*
 * Tests functions:
 *   calc_sanitize_decimal_number
 */

TEST(CoreCalc, SanitizeDecimalNumber)
{
    char str_number[1024];

    WEE_CHECK_SANITIZE_DECIMAL_NUMBER(0, "0", "0");
    WEE_CHECK_SANITIZE_DECIMAL_NUMBER(1, "0.0", "0.0");
    WEE_CHECK_SANITIZE_DECIMAL_NUMBER(1, "0.0", "0,0");
    WEE_CHECK_SANITIZE_DECIMAL_NUMBER(1, "1.23", "1.23");
    WEE_CHECK_SANITIZE_DECIMAL_NUMBER(1, "1.23", "1,23");
    WEE_CHECK_SANITIZE_DECIMAL_NUMBER(1, "1234.56", "1.234,56");
    WEE_CHECK_SANITIZE_DECIMAL_NUMBER(0, "123456789", "123.456.789");
    WEE_CHECK_SANITIZE_DECIMAL_NUMBER(0, "123456789", "123,456,789");
    WEE_CHECK_SANITIZE_DECIMAL_NUMBER(1, "1234567.89", "1.234.567,89");
    WEE_CHECK_SANITIZE_DECIMAL_NUMBER(1, "1234567.89", "1,234,567.89");
    WEE_CHECK_SANITIZE_DECIMAL_NUMBER(1, "-2345.67", "-2.345,67");
}

/*
 * Tests functions:
 *   calc_format_result
 */

TEST(CoreCalc, FormatResult)
{
    char str_result[64];

    WEE_CHECK_FORMAT_RESULT("0", 0);
    WEE_CHECK_FORMAT_RESULT("0", 0.0);
    WEE_CHECK_FORMAT_RESULT("0", -0.0);
    WEE_CHECK_FORMAT_RESULT("12.5", 12.5);
    WEE_CHECK_FORMAT_RESULT("12.005", 12.005000);
    WEE_CHECK_FORMAT_RESULT("-12.005", -12.005000);
    WEE_CHECK_FORMAT_RESULT("0.0000000001", 0.0000000001);
    WEE_CHECK_FORMAT_RESULT("0", 0.00000000001);
    WEE_CHECK_FORMAT_RESULT("123456789012345", 123456789012345);

    setlocale (LC_ALL, "fr_FR.UTF-8");
    WEE_CHECK_FORMAT_RESULT("12.5", 12.5);
    WEE_CHECK_FORMAT_RESULT("-12.5", -12.5);
    setlocale (LC_ALL, "");
}

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
    WEE_CHECK_CALC("11", "2*3+5");
    WEE_CHECK_CALC("7", "5+2*3/3");
    WEE_CHECK_CALC("7", "2*3/3+5");

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

    /* with French locale: the result must always have "." instead of "," */
    setlocale (LC_ALL, "fr_FR.UTF-8");
    WEE_CHECK_CALC("12.5", "10.5+2");
    WEE_CHECK_CALC("-12.5", "-10.5-2");
    setlocale (LC_ALL, "");
}
