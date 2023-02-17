/*
 * wee-calc.c - calculate result of an expression
 *
 * Copyright (C) 2019-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <locale.h>

#include "weechat.h"
#include "wee-arraylist.h"
#include "wee-string.h"

enum t_calc_symbol
{
    CALC_SYMBOL_NONE = 0,
    CALC_SYMBOL_PARENTHESIS_OPEN,
    CALC_SYMBOL_PARENTHESIS_CLOSE,
    CALC_SYMBOL_VALUE,
    CALC_SYMBOL_OPERATOR,
};


/*
 * Callback called to free a value or op in the arraylist.
 */

void
calc_list_free_cb (void *data, struct t_arraylist *arraylist, void *pointer)
{
    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    free (pointer);
}

/*
 * Returns the precedence of an operator:
 *   - '*' and '/': 2
 *   - '+' and '-': 1
 *   - any other: 0
 */

int
calc_operator_precedence (const char *oper)
{
    if (!oper)
        return 0;

    if ((strcmp (oper, "*") == 0)
        || (strcmp (oper, "/") == 0)
        || (strcmp (oper, "//") == 0)
        || (strcmp (oper, "%") == 0)
        || (strcmp (oper, "**") == 0))
    {
        return 2;
    }

    if ((strcmp (oper, "+") == 0)
        || (strcmp (oper, "-") == 0))
    {
        return 1;
    }

    return 0;
}

/*
 * Pops an integer value from the stack of values.
 */

double
calc_pop_value (struct t_arraylist *list_values)
{
    int size_values;
    double *ptr_value, value;

    if (!list_values)
        return 0;

    size_values = arraylist_size (list_values);

    if (size_values < 1)
        return 0;

    ptr_value = arraylist_get (list_values, size_values - 1);
    value = *ptr_value;

    arraylist_remove (list_values, size_values - 1);

    return value;
}

/*
 * Calculates result of an operation using an operator and two values.
 */

double
calc_operation (const char *oper, double value1, double value2)
{
    if (!oper)
        return 0;

    if (strcmp (oper, "+") == 0)
        return value1 + value2;

    if (strcmp (oper, "-") == 0)
        return value1 - value2;

    if (strcmp (oper, "*") == 0)
        return value1 * value2;

    if (strcmp (oper, "/") == 0)
        return (value2 != 0) ? value1 / value2 : 0;

    if (strcmp (oper, "//") == 0)
        return (value2 != 0) ? floor (value1 / value2) : 0;

    if (strcmp (oper, "%") == 0)
        return (value2 != 0) ? fmod (value1, value2) : 0;

    if (strcmp (oper, "**") == 0)
        return pow (value1, value2);

    return 0;
}

/*
 * Calculates result of an operation using the operator on the operators stack
 * and the two values on the values stack.
 *
 * The result is pushed on values stack.
 */

void
calc_operation_stacks (struct t_arraylist *list_values,
                       struct t_arraylist *list_ops)
{
    int size_ops;
    double value1, value2, result, *ptr_result;
    char *ptr_operator;

    if (!list_values || !list_ops)
        return;

    size_ops = arraylist_size (list_ops);
    if (size_ops < 1)
        return;

    ptr_operator = arraylist_get (list_ops, size_ops - 1);

    value2 = calc_pop_value (list_values);
    value1 = calc_pop_value (list_values);

    result = calc_operation (ptr_operator, value1, value2);

    ptr_result = malloc (sizeof (result));
    *ptr_result = result;
    arraylist_add (list_values, ptr_result);

    arraylist_remove (list_ops, size_ops - 1);
}

/*
 * Sanitizes a decimal number: removes any thousands separator and replaces
 * the decimal separator by a dot.
 *
 * The string is updated in-place, the result has always a length shorter or
 * equal to the input string.
 *
 * Examples:
 *   1.23          -->  1.23
 *   1,23          -->  1,23
 *   1.234,56      -->  1234.56
 *   123.456.789   -->  123456789
 *   123,456,789   -->  123456789
 *   1.234.567,89  -->  1234567.89
 *   1,234,567.89  -->  1234567.89
 *   -2.345,67     -->  -2345.67
 *
 * Returns:
 *   1: the number has decimal part
 *   0: the number has no decimal part
 */

int
calc_sanitize_decimal_number (char *string)
{
    int i, j, count_sep, different_sep, index_last_sep;

    count_sep = 0;
    different_sep = 0;
    index_last_sep = -1;

    i = strlen (string) - 1;
    while (i >= 0)
    {
        if (!isdigit ((unsigned char)string[i]) && (string[i] != '-'))
        {
            count_sep++;
            if (index_last_sep < 0)
            {
                /* last separator found */
                index_last_sep = i;
            }
            else
            {
                /* another separator found */
                if (!different_sep && (string[i] != string[index_last_sep]))
                {
                    different_sep = 1;
                    break;
                }
            }
        }
        i--;
    }
    if ((count_sep > 1) && !different_sep)
    {
        /*
         * case of only thousands separators, like 123.456.789
         * => we strip all separators
         */
        index_last_sep = -1;
    }

    if (index_last_sep >= 0)
        string[index_last_sep] = '.';

    i = 0;
    j = 0;
    while (1)
    {
        if (((index_last_sep < 0) || (i < index_last_sep))
            && string[i]
            && !isdigit ((unsigned char)string[i])
            && (string[i] != '-'))
        {
            /* another separator found before the last one: skip it */
            i++;
        }
        else
        {
            if (j != i)
                string[j] = string[i];
            if (!string[i])
                break;
            i++;
            j++;
        }
    }

    return (index_last_sep >= 0) ? 1 : 0;
}

/*
 * Formats the result as a decimal number (locale independent): remove any
 * extra "0" at the and the decimal point if needed.
 */

void
calc_format_result (double value, char *result, int max_size)
{
    int i, has_decimal;

    snprintf (result, max_size,
              "%.10f",
              /* ensure result is not "-0" */
              (value == -0.0) ? 0.0 : value);

    has_decimal = calc_sanitize_decimal_number (result);

    i = strlen (result) - 1;
    while (i >= 0)
    {
        if (!isdigit ((unsigned char)result[i]) && (result[i] != '-'))
        {
            result[i] = '\0';
            break;
        }
        if (has_decimal && (result[i] == '0'))
        {
            result[i] = '\0';
            i--;
        }
        else
            break;
    }
}

/*
 * Calculates an expression, which can contain:
 *   - integer and decimal numbers (ie 2 or 2.5)
 *   - operators:
 *        +: addition
 *        -: subtraction
 *        *: multiplication
 *        /: division
 *        \: division giving an integer as result
 *        %: remainder of division
 *       **: power
 *   - parentheses: ( )
 *
 * The value returned is a string representation of the result, which can be
 * an integer or a double, according to the operations and numbers in input.
 *
 * Note: result must be freed after use (if not NULL).
 */

char *
calc_expression (const char *expr)
{
    struct t_arraylist *list_values, *list_ops;
    char str_result[64], *ptr_operator, *operator;
    int i, i2, index_op, decimals;
    enum t_calc_symbol last_symbol;
    double value, factor, *ptr_value;

    list_values = NULL;
    list_ops = NULL;

    /* return 0 by default in case of error */
    snprintf (str_result, sizeof (str_result), "0");

    if (!expr)
        goto end;

    /* stack with values */
    list_values = arraylist_new (32, 0, 1,
                                 NULL, NULL,
                                 &calc_list_free_cb, NULL);
    if (!list_values)
        goto end;

    /* stack with operators */
    list_ops = arraylist_new (32, 0, 1,
                              NULL, NULL,
                              &calc_list_free_cb, NULL);
    if (!list_ops)
        goto end;

    last_symbol = CALC_SYMBOL_NONE;
    for (i = 0; expr[i]; i++)
    {
        if (expr[i] == ' ')
        {
            /* ignore spaces */
            continue;
        }
        else if (expr[i] == '(')
        {
            ptr_operator = string_strndup (expr + i, 1);
            arraylist_add (list_ops, ptr_operator);
            last_symbol = CALC_SYMBOL_PARENTHESIS_OPEN;
        }
        else if (isdigit ((unsigned char)expr[i]) || (expr[i] == '.')
                 || ((expr[i] == '-')
                     && ((last_symbol == CALC_SYMBOL_NONE)
                         || (last_symbol == CALC_SYMBOL_PARENTHESIS_OPEN)
                         || (last_symbol == CALC_SYMBOL_OPERATOR))))
        {
            value = 0;
            decimals = 0;
            factor = 1;
            if (expr[i] == '-')
            {
                factor = -1;
                i++;
            }
            while (expr[i]
                   && (isdigit ((unsigned char)expr[i]) || (expr[i] == '.')))
            {
                if (expr[i] == '.')
                {
                    if (decimals == 0)
                        decimals = 10;
                }
                else
                {
                    if (decimals)
                    {
                        value = value + (((double)(expr[i] - '0')) / decimals);
                        decimals *= 10;
                    }
                    else
                    {
                        value = (value * 10) + (expr[i] - '0');
                    }
                }
                i++;
            }
            i--;
            value *= factor;
            ptr_value = malloc (sizeof (value));
            *ptr_value = value;
            arraylist_add (list_values, ptr_value);
            last_symbol = CALC_SYMBOL_VALUE;
        }
        else if (expr[i] == ')')
        {
            index_op = arraylist_size (list_ops) - 1;
            while (index_op >= 0)
            {
                ptr_operator = arraylist_get (list_ops, index_op);
                if (strcmp (ptr_operator, "(") == 0)
                    break;
                calc_operation_stacks (list_values, list_ops);
                index_op--;
            }
            /* remove "(" from operators */
            index_op = arraylist_size (list_ops) - 1;
            if (index_op >= 0)
                arraylist_remove (list_ops, index_op);
            last_symbol = CALC_SYMBOL_PARENTHESIS_CLOSE;
        }
        else
        {
            /* operator */
            i2 = i + 1;
            while (expr[i2] && (expr[i2] != ' ') && (expr[i2] != '(')
                   && (expr[i2] != ')') && (expr[i2] != '.')
                   && (expr[i2] != '-') && !isdigit ((unsigned char)expr[i2]))
            {
                i2++;
            }
            operator = string_strndup (expr + i, i2 - i);
            i = i2 - 1;
            if (operator)
            {
                index_op = arraylist_size (list_ops) - 1;
                while (index_op >= 0)
                {
                    ptr_operator = arraylist_get (list_ops, index_op);
                    if (calc_operator_precedence (ptr_operator) <
                        calc_operator_precedence (operator))
                        break;
                    calc_operation_stacks (list_values, list_ops);
                    index_op--;
                }
                arraylist_add (list_ops, operator);
            }
            last_symbol = CALC_SYMBOL_OPERATOR;
        }
    }

    while (arraylist_size (list_ops) > 0)
    {
        calc_operation_stacks (list_values, list_ops);
    }

    value = calc_pop_value (list_values);
    calc_format_result (value, str_result, sizeof (str_result));

end:
    if (list_values)
        arraylist_free (list_values);
    if (list_ops)
        arraylist_free (list_ops);

    return strdup (str_result);
}
