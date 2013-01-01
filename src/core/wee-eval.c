/*
 * wee-eval.c - evaluate expressions with references to internal vars
 *
 * Copyright (C) 2013 Sebastien Helleu <flashcode@flashtux.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "weechat.h"
#include "wee-eval.h"
#include "wee-config-file.h"
#include "wee-hashtable.h"
#include "wee-hdata.h"
#include "wee-hook.h"
#include "wee-string.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-color.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"


char *logical_ops[EVAL_NUM_LOGICAL_OPS] = { "&&", "||" };
char *comparisons[EVAL_NUM_COMPARISONS] = { "==", "!=", "<=", "<", ">=", ">",
                                            "=~", "!~" };


/*
 * Checks if a value is true: a value is true if string is non-NULL, non-empty
 * and different from "0".
 *
 * Returns:
 *   1: value is true
 *   0: value is false
 */

int
eval_is_true (const char *value)
{
    return (value && value[0] && (strcmp (value, "0") != 0)) ? 1 : 0;
}

/*
 * Gets value of hdata using "path" to a variable.
 *
 * Note: result must be freed after use.
 */

char *
eval_hdata_get_value (struct t_hdata *hdata, void *pointer, const char *path)
{
    char *value, *old_value, *var_name, str_value[128], *pos;
    const char *ptr_value, *hdata_name;
    int type;
    struct t_hashtable *hashtable;

    value = NULL;
    var_name = NULL;

    /* NULL pointer? return empty string */
    if (!pointer)
        return strdup ("");

    /* no path? just return current pointer as string */
    if (!path || !path[0])
    {
        snprintf (str_value, sizeof (str_value),
                  "0x%lx", (long unsigned int)pointer);
        return strdup (str_value);
    }

    /*
     * look for name of hdata, for example in "window.buffer.full_name", the
     * hdata name is "window"
     */
    pos = strchr (path, '.');
    if (pos > path)
        var_name = string_strndup (path, pos - path);
    else
        var_name = strdup (path);

    if (!var_name)
        goto end;

    /* search type of variable in hdata */
    type = hdata_get_var_type (hdata, var_name);
    if (type < 0)
        goto end;

    /* build a string with the value or variable */
    switch (type)
    {
        case WEECHAT_HDATA_CHAR:
            snprintf (str_value, sizeof (str_value),
                      "%c", hdata_char (hdata, pointer, var_name));
            value = strdup (str_value);
            break;
        case WEECHAT_HDATA_INTEGER:
            snprintf (str_value, sizeof (str_value),
                      "%d", hdata_integer (hdata, pointer, var_name));
            value = strdup (str_value);
            break;
        case WEECHAT_HDATA_LONG:
            snprintf (str_value, sizeof (str_value),
                      "%ld", hdata_long (hdata, pointer, var_name));
            value = strdup (str_value);
            break;
        case WEECHAT_HDATA_STRING:
            value = strdup (hdata_string (hdata, pointer, var_name));
            break;
        case WEECHAT_HDATA_POINTER:
            pointer = hdata_pointer (hdata, pointer, var_name);
            snprintf (str_value, sizeof (str_value),
                      "0x%lx", (long unsigned int)pointer);
            value = strdup (str_value);
            break;
        case WEECHAT_HDATA_TIME:
            snprintf (str_value, sizeof (str_value),
                      "%ld", (long)hdata_time (hdata, pointer, var_name));
            value = strdup (str_value);
            break;
        case WEECHAT_HDATA_HASHTABLE:
            pointer = hdata_hashtable (hdata, pointer, var_name);
            if (pos)
            {
                /*
                 * for a hashtable, if there is a "." after name of hdata,
                 * get the value for this key in hashtable
                 */
                hashtable = pointer;
                ptr_value = hashtable_get (hashtable, pos + 1);
                if (ptr_value)
                {
                    switch (hashtable->type_values)
                    {
                        case HASHTABLE_INTEGER:
                            snprintf (str_value, sizeof (str_value),
                                      "%d", *((int *)ptr_value));
                            value = strdup (str_value);
                            break;
                        case HASHTABLE_STRING:
                            value = strdup (ptr_value);
                            break;
                        case HASHTABLE_POINTER:
                        case HASHTABLE_BUFFER:
                            snprintf (str_value, sizeof (str_value),
                                      "0x%lx", (long unsigned int)ptr_value);
                            value = strdup (str_value);
                            break;
                        case HASHTABLE_TIME:
                            snprintf (str_value, sizeof (str_value),
                                      "%ld", (long)(*((time_t *)ptr_value)));
                            value = strdup (str_value);
                            break;
                        case HASHTABLE_NUM_TYPES:
                            break;
                    }
                }
            }
            else
            {
                snprintf (str_value, sizeof (str_value),
                          "0x%lx", (long unsigned int)pointer);
                value = strdup (str_value);
            }
            break;
    }

    /*
     * if we are on a pointer and that something else is in path (after "."),
     * go on with this pointer and remaining path
     */
    if ((type == WEECHAT_HDATA_POINTER) && pos)
    {
        hdata_name = hdata_get_var_hdata (hdata, var_name);
        if (!hdata_name)
            goto end;

        hdata = hook_hdata_get (NULL, hdata_name);
        old_value = value;
        value = eval_hdata_get_value (hdata, pointer, (pos) ? pos + 1 : NULL);
        if (old_value)
            free (old_value);
    }

end:
    if (var_name)
        free (var_name);

    return value;
}

/*
 * Replaces variables, which can be, by order of priority:
 *   1. an extra variable (from hashtable "extra_vars")
 *   2. an name of option (file.section.option)
 *   3. a buffer local variable
 *   4. a hdata name/variable
 *
 * Examples:
 *   option: ${weechat.look.scroll_amount}
 *   hdata : ${window.buffer.full_name}
 *           ${window.buffer.local_variables.type}
 */

char *
eval_replace_vars_cb (void *data, const char *text)
{
    struct t_hashtable *pointers, *extra_vars;
    struct t_config_option *ptr_option;
    struct t_gui_buffer *ptr_buffer;
    char str_value[64], *value, *pos, *pos1, *pos2, *hdata_name, *list_name;
    char *tmp;
    const char *ptr_value;
    struct t_hdata *hdata;
    void *pointer;

    pointers = (struct t_hashtable *)(((void **)data)[0]);
    extra_vars = (struct t_hashtable *)(((void **)data)[1]);

    /* 1. look for var in hashtable "extra_vars" */
    ptr_value = hashtable_get (extra_vars, text);
    if (ptr_value)
        return strdup (ptr_value);

    /* 2. look for name of option: if found, return this value */
    config_file_search_with_string (text, NULL, NULL, &ptr_option, NULL);
    if (ptr_option)
    {
        switch (ptr_option->type)
        {
            case CONFIG_OPTION_TYPE_BOOLEAN:
                return strdup (CONFIG_BOOLEAN(ptr_option) ? EVAL_STR_TRUE : EVAL_STR_FALSE);
            case CONFIG_OPTION_TYPE_INTEGER:
                if (ptr_option->string_values)
                    return strdup (ptr_option->string_values[CONFIG_INTEGER(ptr_option)]);
                snprintf (str_value, sizeof (str_value),
                          "%d", CONFIG_INTEGER(ptr_option));
                return strdup (str_value);
            case CONFIG_OPTION_TYPE_STRING:
                return strdup (CONFIG_STRING(ptr_option));
            case CONFIG_OPTION_TYPE_COLOR:
                return strdup (gui_color_get_name (CONFIG_COLOR(ptr_option)));
            case CONFIG_NUM_OPTION_TYPES:
                return NULL;
        }
    }

    /* 3. look for local variable in buffer */
    ptr_buffer = hashtable_get (pointers, "buffer");
    if (ptr_buffer)
    {
        ptr_value = hashtable_get (ptr_buffer->local_variables, text);
        if (ptr_value)
            return strdup (ptr_value);
    }

    /* 4. look for hdata */
    value = NULL;
    hdata_name = NULL;
    list_name = NULL;
    pointer = NULL;

    pos = strchr (text, '.');
    if (pos > text)
        hdata_name = string_strndup (text, pos - text);
    else
        hdata_name = strdup (text);

    if (!hdata_name)
        goto end;

    pos1 = strchr (hdata_name, '[');
    if (pos1 > hdata_name)
    {
        pos2 = strchr (pos1 + 1, ']');
        if (pos2 > pos1 + 1)
        {
            list_name = string_strndup (pos1 + 1, pos2 - pos1 - 1);
        }
        tmp = string_strndup (hdata_name, pos1 - hdata_name);
        if (tmp)
        {
            free (hdata_name);
            hdata_name = tmp;
        }
    }

    hdata = hook_hdata_get (NULL, hdata_name);
    if (!hdata)
        goto end;

    if (list_name)
        pointer = hdata_get_list (hdata, list_name);
    if (!pointer)
    {
        pointer = hashtable_get (pointers, hdata_name);
        if (!pointer)
            goto end;
    }

    value = eval_hdata_get_value (hdata, pointer, (pos) ? pos + 1 : NULL);

end:
    if (hdata_name)
        free (hdata_name);
    if (list_name)
        free (list_name);

    return (value) ? value : strdup ("");
}

/*
 * Replaces variables in a string.
 */

char *
eval_replace_vars (const char *expr, struct t_hashtable *pointers,
                   struct t_hashtable *extra_vars)
{
    int errors;
    void *ptr[2];

    ptr[0] = pointers;
    ptr[1] = extra_vars;

    return string_replace_with_callback (expr,
                                         &eval_replace_vars_cb,
                                         ptr,
                                         &errors);
}

/*
 * Compares two expressions.
 *
 * Returns:
 *   "1": comparison is true
 *   "0": comparison is false
 *
 * Examples:
 *   "15 > 2": returns "1"
 *   "abc == def": returns "0"
 *
 * Note: result must be freed after use.
 */

char *
eval_compare (const char *expr1, int comparison, const char *expr2)
{
    int rc, string_compare, length1, length2;
    regex_t regex;
    long value1, value2;
    char *error;

    rc = 0;
    string_compare = 0;

    if (!expr1 || !expr2)
        goto end;

    if ((comparison == EVAL_COMPARE_REGEX_MATCHING)
        || (comparison == EVAL_COMPARE_REGEX_NOT_MATCHING))
    {
        if (string_regcomp (&regex, expr2,
                            REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0)
        {
            goto end;
        }
        rc = (regexec (&regex, expr1, 0, NULL, 0) == 0) ? 1 : 0;
        if (comparison == EVAL_COMPARE_REGEX_NOT_MATCHING)
            rc ^= 1;
        goto end;
    }

    length1 = strlen (expr1);
    length2 = strlen (expr2);

    /*
     * string comparison is forced if expr1 and expr2 have double quotes at
     * beginning/end
     */
    if (((length1 == 0) || ((expr1[0] == '"') && expr1[length1 - 1] == '"'))
        && ((length2 == 0) || ((expr2[0] == '"') && expr2[length2 - 1] == '"')))
    {
        string_compare = 1;
    }

    if (!string_compare)
    {
        value1 = strtol (expr1, &error, 10);
        if (!error || error[0])
            string_compare = 1;
        else
        {
            value2 = strtol (expr2, &error, 10);
            if (!error || error[0])
                string_compare = 1;
        }
    }

    if (string_compare)
        rc = strcmp (expr1, expr2);
    else
        rc = (value1 < value2) ? -1 : ((value1 > value2) ? 1 : 0);

    switch (comparison)
    {
        case EVAL_COMPARE_EQUAL:
            rc = (rc == 0);
            break;
        case EVAL_COMPARE_NOT_EQUAL:
            rc = (rc != 0);
            break;
        case EVAL_COMPARE_LESS_EQUAL:
            rc = (rc <= 0);
            break;
        case EVAL_COMPARE_LESS:
            rc = (rc < 0);
            break;
        case EVAL_COMPARE_GREATER_EQUAL:
            rc = (rc >= 0);
            break;
        case EVAL_COMPARE_GREATER:
            rc = (rc > 0);
            break;
        case EVAL_NUM_COMPARISONS:
            break;
    }

end:
    return strdup ((rc) ? EVAL_STR_TRUE : EVAL_STR_FALSE);
}

/*
 * Evaluates an expression (this function must not be called directly).
 *
 * Argument keep_parentheses is almost always 0, it is 1 only if the expression
 * is a regex (to keep flags inside the parentheses).
 *
 * For return value, see function eval_expression().
 * Note: result must be freed after use.
 */

char *
eval_expression_internal (const char *expr, struct t_hashtable *pointers,
                          struct t_hashtable *extra_vars,
                          int keep_parentheses)
{
    int logic, comp, length, level, rc;
    const char *pos_end;
    char *expr2, *sub_expr, *pos, *value, *tmp_value, *tmp_value2;

    value = NULL;

    if (!expr)
        return NULL;

    if (!expr[0])
        return strdup (expr);

    /*
     * skip spaces at beginning of string
     */
    while (expr[0] == ' ')
    {
        expr++;
    }
    if (!expr[0])
        return strdup (expr);

    /* skip spaces at end of string */
    pos_end = expr + strlen (expr) - 1;
    while ((pos_end > expr) && (pos_end[0] == ' '))
    {
        pos_end--;
    }

    expr2 = string_strndup (expr, pos_end + 1 - expr);
    if (!expr2)
        return NULL;

    /* evaluate sub-expression in parentheses and replace it with value */
    if (!keep_parentheses)
    {
        while (expr2[0] == '(')
        {
            level = 0;
            pos = expr2 + 1;
            while (pos[0])
            {
                if (pos[0] == '(')
                    level++;
                else if (pos[0] == ')')
                {
                    if (level == 0)
                        break;
                    level--;
                }
                pos++;
            }
            /* closing parenthese not found */
            if (pos[0] != ')')
                goto end;
            sub_expr = string_strndup (expr2 + 1, pos - expr2 - 1);
            if (!sub_expr)
                goto end;
            tmp_value = eval_expression_internal (sub_expr, pointers, extra_vars, 0);
            free (sub_expr);
            if (!pos[1])
            {
                /* nothing after ')', then return value of sub-expression as-is */
                value = tmp_value;
                goto end;
            }
            length = ((tmp_value) ? strlen (tmp_value) : 0) + 1 + strlen (pos + 1) + 1;
            tmp_value2 = malloc (length);
            if (!tmp_value2)
                goto end;
            tmp_value2[0] = '\0';
            if (tmp_value)
                strcat (tmp_value2, tmp_value);
            strcat (tmp_value2, " ");
            strcat (tmp_value2, pos + 1);
            free (expr2);
            expr2 = tmp_value2;
        }
    }

    /*
     * search for a logical operator, and if one is found:
     * - split expression into two sub-expressions
     * - evaluate first sub-expression
     * - if needed, evaluate second sub-expression
     * - return result
     */
    for (logic = 0; logic < EVAL_NUM_LOGICAL_OPS; logic++)
    {
        pos = strstr (expr2, logical_ops[logic]);
        if (pos > expr2)
        {
            pos_end = pos - 1;
            while ((pos_end > expr2) && (pos_end[0] == ' '))
            {
                pos_end--;
            }
            sub_expr = string_strndup (expr2, pos_end + 1 - expr2);
            if (!sub_expr)
                goto end;
            tmp_value = eval_expression_internal (sub_expr, pointers, extra_vars, 0);
            free (sub_expr);
            rc = eval_is_true (tmp_value);
            /*
             * if rc == 0 with "&&" or rc == 1 with "||", no need to evaluate
             * second sub-expression, just return the rc
             */
            if ((!rc && (logic == EVAL_LOGICAL_OP_AND))
                || (rc && (logic == EVAL_LOGICAL_OP_OR)))
            {
                if (tmp_value)
                    free (tmp_value);
                value = strdup ((rc) ? EVAL_STR_TRUE : EVAL_STR_FALSE);
                goto end;
            }
            pos += strlen (logical_ops[logic]);
            while (pos[0] == ' ')
            {
                pos++;
            }
            tmp_value = eval_expression_internal (pos, pointers, extra_vars, 0);
            rc = eval_is_true (tmp_value);
            if (tmp_value)
                free (tmp_value);
            value = strdup ((rc) ? EVAL_STR_TRUE : EVAL_STR_FALSE);
            goto end;
        }
    }

    /*
     * search for a comparison, and if one is found:
     * - split expression into two sub-expressions
     * - evaluate the two sub-expressions
     * - compare sub-expressions
     * - return result
     */
    for (comp = 0; comp < EVAL_NUM_COMPARISONS; comp++)
    {
        pos = strstr (expr2, comparisons[comp]);
        if (pos > expr2)
        {
            pos_end = pos - 1;
            while ((pos_end > expr2) && (pos_end[0] == ' '))
            {
                pos_end--;
            }
            sub_expr = string_strndup (expr2, pos_end + 1 - expr2);
            if (!sub_expr)
                goto end;
            tmp_value = eval_expression_internal (sub_expr, pointers, extra_vars, 0);
            free (sub_expr);
            pos += strlen (comparisons[comp]);
            while (pos[0] == ' ')
            {
                pos++;
            }
            tmp_value2 = eval_expression_internal (pos, pointers, extra_vars,
                                                   ((comp == EVAL_COMPARE_REGEX_MATCHING)
                                                    || (comp == EVAL_COMPARE_REGEX_NOT_MATCHING)) ? 1 : 0);
            value = eval_compare (tmp_value, comp, tmp_value2);
            if (tmp_value)
                free (tmp_value);
            if (tmp_value2)
                free (tmp_value2);
            goto end;
        }
    }

    /*
     * at this point, there is no more logical operator neither comparison,
     * so we just replace variables in string and return the result
     */
    value = eval_replace_vars (expr2, pointers, extra_vars);

end:
    if (expr2)
        free (expr2);

    return value;
}

/*
 * Evaluates an expression.
 *
 * The hashtable "pointers" must have string for keys, pointer for values.
 * The hashtable "extra_vars" must have string for keys and values.
 *
 * The expression can contain:
 *   - conditions:  ==  != <  <=  >  >=
 *   - logical operators:  &&  ||
 *   - parentheses for priority
 *
 * Examples (the [ ] are NOT part of result):
 *   >> ${window.buffer.number}
 *   == [2]
 *   >> buffer:${window.buffer.full_name}
 *   == [buffer:irc.freenode.#weechat]
 *   >> ${window.buffer.full_name} == irc.freenode.#weechat
 *   == [1]
 *   >> ${window.buffer.full_name} == irc.freenode.#test
 *   == [0]
 *   >> ${window.win_width}
 *   == [112]
 *   >> ${window.win_height}
 *   == [40]
 *   >> ${window.win_width} >= 30 && ${window.win_height} >= 20
 *   == [1]
 *
 * Note: result must be freed after use.
 */

char *
eval_expression (const char *expr, struct t_hashtable *pointers,
                 struct t_hashtable *extra_vars)
{
    int pointers_created, extra_vars_created;
    char *value;
    struct t_gui_window *window;

    if (!expr)
        return NULL;

    pointers_created = 0;
    extra_vars_created = 0;

    /* create hashtable pointers if it's NULL */
    if (!pointers)
    {
        pointers = hashtable_new (16,
                                  WEECHAT_HASHTABLE_STRING,
                                  WEECHAT_HASHTABLE_POINTER,
                                  NULL,
                                  NULL);
        if (!pointers)
            return NULL;
        pointers_created = 1;
    }

    /*
     * set window/buffer with pointer to current window/buffer
     * (if not already defined in the hashtable)
     */
    if (gui_current_window)
    {
        if (!hashtable_has_key (pointers, "window"))
            hashtable_set (pointers, "window", gui_current_window);
        if (!hashtable_has_key (pointers, "buffer"))
        {
            window = (struct t_gui_window *)hashtable_get (pointers, "window");
            if (window)
                hashtable_set (pointers, "buffer", window->buffer);
        }
    }

    /* create hashtable extra_vars if it's NULL */
    if (!extra_vars)
    {
        extra_vars = hashtable_new (16,
                                    WEECHAT_HASHTABLE_STRING,
                                    WEECHAT_HASHTABLE_STRING,
                                    NULL,
                                    NULL);
        if (!extra_vars)
            return NULL;
        extra_vars_created = 1;
    }

    value = eval_expression_internal (expr, pointers, extra_vars, 0);

    if (pointers_created)
        hashtable_free (pointers);
    if (extra_vars_created)
        hashtable_free (extra_vars);

    return value;
}
