/*
 * wee-calc.c - calculate result of an expression
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
calc_operator_precedence (char *operator)
{
    if (!operator)
        return 0;

    if ((strcmp (operator, "*") == 0)
        || (strcmp (operator, "/") == 0)
        || (strcmp (operator, "//") == 0)
        || (strcmp (operator, "%") == 0))
    {
        return 2;
    }

    if ((strcmp (operator, "+") == 0)
        || (strcmp (operator, "-") == 0))
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
calc_operation (char *operator, double value1, double value2)
{
    if (strcmp (operator, "+") == 0)
        return value1 + value2;

    if (strcmp (operator, "-") == 0)
        return value1 - value2;

    if (strcmp (operator, "*") == 0)
        return value1 * value2;

    if (strcmp (operator, "/") == 0)
        return (value2 != 0) ? value1 / value2 : 0;

    if (strcmp (operator, "//") == 0)
        return (value2 != 0) ? floor (value1 / value2) : 0;

    if (strcmp (operator, "%") == 0)
        return (value2 != 0) ? fmod (value1, value2) : 0;

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
 * Formats the result as a decimal number (locale independent): remove any
 * extra "0" at the and the decimal point if needed.
 */

void
calc_format_result (double value, char *result, int max_size)
{
    char *pos_point;
    int i;

    /*
     * local-independent formatting of value, so that a decimal point is always
     * used (instead of a comma in French for example)
     */
    setlocale (LC_ALL, "C");
    snprintf (result, max_size, "%.10f", value);
    setlocale (LC_ALL, "");

    pos_point = strchr (result, '.');

    i = strlen (result) - 1;
    while (i >= 0)
    {
        if (!isdigit (result[i]) && (result[i] != '-'))
        {
            result[i] = '\0';
            break;
        }
        if (pos_point && (result[i] == '0'))
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
 *       +: addition
 *       -: subtraction
 *       *: multiplication
 *       /: division
 *       \: division giving an integer as result
 *       %: remainder of division
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
    double value, *ptr_value;

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
        }
        else if (isdigit (expr[i]) || (expr[i] == '.'))
        {
            value = 0;
            decimals = 0;
            while (expr[i] && (isdigit (expr[i]) || (expr[i] == '.')))
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
            ptr_value = malloc (sizeof (value));
            *ptr_value = value;
            arraylist_add (list_values, ptr_value);
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
        }
        else
        {
            /* operator */
            i2 = i + 1;
            while (expr[i2] && (expr[i2] != ' ') && (expr[i2] != '(')
                   && (expr[i2] != ')') && (expr[i2] != '.')
                   && !isdigit (expr[i2]))
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
