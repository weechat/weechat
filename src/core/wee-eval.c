/*
 * wee-eval.c - evaluate expressions with references to internal vars
 *
 * Copyright (C) 2012-2017 Sébastien Helleu <flashcode@flashtux.org>
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
#include <time.h>

#include "weechat.h"
#include "wee-eval.h"
#include "wee-config-file.h"
#include "wee-hashtable.h"
#include "wee-hdata.h"
#include "wee-hook.h"
#include "wee-secure.h"
#include "wee-string.h"
#include "wee-utf8.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-color.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"


char *logical_ops[EVAL_NUM_LOGICAL_OPS] =
{ "||", "&&" };

char *comparisons[EVAL_NUM_COMPARISONS] =
{ "=~", "!~", "=*", "!*", "==", "!=", "<=", "<", ">=", ">" };


char *eval_replace_vars (const char *expr, struct t_hashtable *pointers,
                         struct t_hashtable *extra_vars, int extra_vars_eval,
                         const char *prefix, const char *suffix,
                         struct t_eval_regex *eval_regex);
char *eval_expression_condition (const char *expr,
                                 struct t_hashtable *pointers,
                                 struct t_hashtable *extra_vars,
                                 int extra_vars_eval,
                                 const char *prefix,
                                 const char *suffix);


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
 * Searches a string in another at same level (skip sub-expressions between
 * prefix/suffix).
 *
 * If escape is 1, the prefix can be escaped with '\' (and then is ignored).
 *
 * For example: eval_strstr_level ("(x || y) || z", "||")
 * will return a pointer on  "|| z" (because the first "||" is
 * in a sub-expression, which is skipped).
 *
 * Returns pointer to string found, or NULL if not found.
 */

const char *
eval_strstr_level (const char *string, const char *search,
                   const char *prefix, const char *suffix, int escape)
{
    const char *ptr_string;
    int level, length_search, length_prefix, length_suffix;

    if (!string || !search)
        return NULL;

    length_search = strlen (search);
    length_prefix = strlen (prefix);
    length_suffix = strlen (suffix);

    ptr_string = string;
    level = 0;
    while (ptr_string[0])
    {
        if (escape && (ptr_string[0] == '\\') && (ptr_string[1] == prefix[0]))
        {
            ptr_string++;
        }
        else if (strncmp (ptr_string, prefix, length_prefix) == 0)
        {
            level++;
            ptr_string += length_prefix;
        }
        else if (strncmp (ptr_string, suffix, length_suffix) == 0)
        {
            if (level > 0)
                level--;
            ptr_string += length_suffix;
        }
        else if ((level == 0)
                 && (strncmp (ptr_string, search, length_search) == 0))
        {
            return ptr_string;
        }
        else
        {
            ptr_string++;
        }
    }

    return NULL;
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
    const char *ptr_value, *hdata_name, *ptr_var_name;
    int type, index;
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
    hdata_get_index_and_name (var_name, &index, &ptr_var_name);
    type = hdata_get_var_type (hdata, ptr_var_name);
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
        case WEECHAT_HDATA_SHARED_STRING:
            ptr_value = hdata_string (hdata, pointer, var_name);
            value = (ptr_value) ? strdup (ptr_value) : NULL;
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
 *   1. an extra variable from hashtable "extra_vars"
 *   2. a string to evaluate (format: eval:xxx)
 *   3. a string with escaped chars (format: esc:xxx or \xxx)
 *   4. a string with chars to hide (format: hide:char,string)
 *   5. a string with max chars (format: cut:max,suffix,string or
 *      cut:+max,suffix,string) or max chars on screen
 *      (format: cutscr:max,suffix,string or cutscr:+max,suffix,string)
 *   6. a regex group captured (format: re:N (0.99) or re:+)
 *   7. a color (format: color:xxx)
 *   8. an info (format: info:name,arguments)
 *   9. current date/time (format: date or date:xxx)
 *  10. an environment variable (format: env:XXX)
 *  11. a ternary operator (format: if:condition?value_if_true:value_if_false)
 *  12. an option (format: file.section.option)
 *  13. a buffer local variable
 *  14. a hdata variable (format: hdata.var1.var2 or hdata[list].var1.var2
 *                        or hdata[ptr].var1.var2)
 *
 * See /help in WeeChat for examples.
 *
 * Note: result must be freed after use.
 */

char *
eval_replace_vars_cb (void *data, const char *text)
{
    struct t_hashtable *pointers, *extra_vars;
    struct t_eval_regex *eval_regex;
    struct t_config_option *ptr_option;
    struct t_gui_buffer *ptr_buffer;
    char str_value[512], *value, *pos, *pos1, *pos2, *hdata_name, *list_name;
    char *tmp, *info_name, *hide_char, *hidden_string, *error, *condition;
    const char *prefix, *suffix, *ptr_value, *ptr_arguments, *ptr_string;
    struct t_hdata *hdata;
    void *pointer;
    int i, length_hide_char, length, index, rc, extra_vars_eval, screen;
    int count_suffix;
    long number;
    long unsigned int ptr;
    time_t date;
    struct tm *date_tmp;

    pointers = (struct t_hashtable *)(((void **)data)[0]);
    extra_vars = (struct t_hashtable *)(((void **)data)[1]);
    extra_vars_eval = *(int *)(((void **)data)[2]);
    prefix = (const char *)(((void **)data)[3]);
    suffix = (const char *)(((void **)data)[4]);
    eval_regex = (struct t_eval_regex *)(((void **)data)[5]);

    /* 1. variable in hashtable "extra_vars" */
    if (extra_vars)
    {
        ptr_value = hashtable_get (extra_vars, text);
        if (ptr_value)
        {
            if (extra_vars_eval)
            {
                tmp = strdup (ptr_value);
                if (!tmp)
                    return NULL;
                hashtable_remove (extra_vars, text);
                value = eval_replace_vars (tmp, pointers,
                                           extra_vars, extra_vars_eval,
                                           prefix, suffix,
                                           eval_regex);
                hashtable_set (extra_vars, text, tmp);
                free (tmp);
                return value;
            }
            else
            {
                return strdup (ptr_value);
            }
        }
    }

    /*
     * 2. force evaluation of string (recursive call)
     *    --> use with caution: the text must be safe!
     */
    if (strncmp (text, "eval:", 5) == 0)
    {
        return eval_replace_vars (text + 5, pointers,
                                  extra_vars, extra_vars_eval,
                                  prefix, suffix,
                                  eval_regex);
    }

    /* 3. convert escaped chars */
    if (strncmp (text, "esc:", 4) == 0)
        return string_convert_escaped_chars (text + 4);
    if ((text[0] == '\\') && text[1] && (text[1] != '\\'))
        return string_convert_escaped_chars (text);

    /* 4. hide chars: replace all chars by a given char/string */
    if (strncmp (text, "hide:", 5) == 0)
    {
        hidden_string = NULL;
        ptr_string = strchr (text + 5,
                             (text[5] == ',') ? ';' : ',');
        if (!ptr_string)
            return strdup ("");
        hide_char = string_strndup (text + 5, ptr_string - text - 5);
        if (hide_char)
        {
            length_hide_char = strlen (hide_char);
            length = utf8_strlen (ptr_string + 1);
            hidden_string = malloc ((length * length_hide_char) + 1);
            if (hidden_string)
            {
                index = 0;
                for (i = 0; i < length; i++)
                {
                    memcpy (hidden_string + index, hide_char,
                            length_hide_char);
                    index += length_hide_char;
                }
                hidden_string[length * length_hide_char] = '\0';
            }
            free (hide_char);
        }
        return (hidden_string) ? hidden_string : strdup ("");
    }

    /*
     * 5. cut chars:
     *   cut: max number of chars, and add an optional suffix when the
     *        string is cut
     *   cutscr: max number of chars displayed on screen, and add an optional
     *           suffix when the string is cut
     */
    if ((strncmp (text, "cut:", 4) == 0)
        || (strncmp (text, "cutscr:", 7) == 0))
    {
        if (strncmp (text, "cut:", 4) == 0)
        {
            screen = 0;
            length = 4;
        }
        else
        {
            screen = 1;
            length = 7;
        }
        pos = strchr (text + length, ',');
        if (!pos)
            return strdup ("");
        count_suffix = 0;
        if (text[length] == '+')
        {
            length++;
            count_suffix = 1;
        }
        pos2 = strchr (pos + 1, ',');
        if (!pos2)
            return strdup ("");
        tmp = strndup (text + length, pos - text - length);
        if (!tmp)
            return strdup ("");
        number = strtol (tmp, &error, 10);
        if (!error || error[0] || (number < 0))
        {
            free (tmp);
            return strdup ("");
        }
        free (tmp);
        tmp = strndup (pos + 1, pos2 - pos - 1);
        if (!tmp)
            return strdup ("");
        value = string_cut (pos2 + 1, number, count_suffix, screen, tmp);
        free (tmp);
        return value;
    }

    /* 6. regex group captured */
    if (strncmp (text, "re:", 3) == 0)
    {
        if (eval_regex && eval_regex->result)
        {
            if (strcmp (text + 3, "+") == 0)
                number = eval_regex->last_match;
            else if (strcmp (text + 3, "#") == 0)
            {
                snprintf (str_value, sizeof (str_value),
                          "%d", eval_regex->last_match);
                return strdup (str_value);
            }
            else
            {
                number = strtol (text + 3, &error, 10);
                if (!error || error[0])
                    number = -1;
            }
            if ((number >= 0) && (number <= eval_regex->last_match))
            {
                return string_strndup (
                    eval_regex->result + eval_regex->match[number].rm_so,
                    eval_regex->match[number].rm_eo - eval_regex->match[number].rm_so);
            }
        }
        return strdup ("");
    }

    /* 7. color code */
    if (strncmp (text, "color:", 6) == 0)
    {
        ptr_value = gui_color_search_config (text + 6);
        if (ptr_value)
            return strdup (ptr_value);
        ptr_value = gui_color_get_custom (text + 6);
        return strdup ((ptr_value) ? ptr_value : "");
    }

    /* 8. info */
    if (strncmp (text, "info:", 5) == 0)
    {
        ptr_value = NULL;
        ptr_arguments = strchr (text + 5, ',');
        if (ptr_arguments)
        {
            info_name = string_strndup (text + 5, ptr_arguments - text - 5);
            ptr_arguments++;
        }
        else
            info_name = strdup (text + 5);
        if (info_name)
        {
            ptr_value = hook_info_get (NULL, info_name, ptr_arguments);
            free (info_name);
        }
        return strdup ((ptr_value) ? ptr_value : "");
    }

    /* 9. current date/time */
    if ((strncmp (text, "date", 4) == 0) && (!text[4] || (text[4] == ':')))
    {
        date = time (NULL);
        date_tmp = localtime (&date);
        if (!date_tmp)
            return strdup ("");
        rc = (int) strftime (str_value, sizeof (str_value),
                             (text[4] == ':') ? text + 5 : "%F %T",
                             date_tmp);
        return strdup ((rc > 0) ? str_value : "");
    }

    /* 10. environment variable */
    if (strncmp (text, "env:", 4) == 0)
    {
        ptr_value = getenv (text + 4);
        if (ptr_value)
            return strdup (ptr_value);
    }

    /* 11: ternary operator: if:condition?value_if_true:value_if_false */
    if (strncmp (text, "if:", 3) == 0)
    {
        value = NULL;
        pos = (char *)eval_strstr_level (text + 3, "?", prefix, suffix, 1);
        pos2 = (pos) ?
            (char *)eval_strstr_level (pos + 1, ":", prefix, suffix, 1) : NULL;
        condition = (pos) ?
            strndup (text + 3, pos - (text + 3)) : strdup (text + 3);
        if (!condition)
            return strdup ("");
        tmp = eval_expression_condition (condition, pointers,
                                         extra_vars, extra_vars_eval,
                                         prefix, suffix);
        rc = eval_is_true (tmp);
        if (tmp)
            free (tmp);
        if (rc)
        {
            /*
             * condition is true: return the "value_if_true"
             * (or EVAL_STR_TRUE if value is missing)
             */
            if (pos)
            {
                tmp = (pos2) ?
                    strndup (pos + 1, pos2 - pos - 1) : strdup (pos + 1);
                if (tmp)
                {
                    value = eval_replace_vars (tmp, pointers,
                                               extra_vars, extra_vars_eval,
                                               prefix, suffix,
                                               eval_regex);
                    free (tmp);
                }
            }
            else
            {
                value = strdup (EVAL_STR_TRUE);
            }
        }
        else
        {
            /*
             * condition is false: return the "value_if_false"
             * (or EVAL_STR_FALSE if both values are missing)
             */
            if (pos2)
            {
                value = eval_replace_vars (pos2 + 1, pointers,
                                           extra_vars, extra_vars_eval,
                                           prefix, suffix,
                                           eval_regex);
            }
            else
            {
                if (!pos)
                    value = strdup (EVAL_STR_FALSE);
            }
        }
        free (condition);
        return (value) ? value : strdup ("");
    }

    /* 12. option: if found, return this value */
    if (strncmp (text, "sec.data.", 9) == 0)
    {
        ptr_value = hashtable_get (secure_hashtable_data, text + 9);
        return strdup ((ptr_value) ? ptr_value : "");
    }
    else
    {
        config_file_search_with_string (text, NULL, NULL, &ptr_option, NULL);
        if (ptr_option)
        {
            if (!ptr_option->value)
                return strdup ("");
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
                    return strdup ("");
            }
        }
    }

    /* 13. local variable in buffer */
    ptr_buffer = hashtable_get (pointers, "buffer");
    if (ptr_buffer)
    {
        ptr_value = hashtable_get (ptr_buffer->local_variables, text);
        if (ptr_value)
            return strdup (ptr_value);
    }

    /* 14. hdata */
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
    {
        if (strncmp (list_name, "0x", 2) == 0)
        {
            rc = sscanf (list_name, "%lx", &ptr);
            if ((rc != EOF) && (rc != 0))
            {
                pointer = (void *)ptr;
                if (!hdata_check_pointer (hdata, NULL, pointer))
                    goto end;
            }
            else
                goto end;
        }
        else
            pointer = hdata_get_list (hdata, list_name);
    }

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
 *
 * Note: result must be freed after use.
 */

char *
eval_replace_vars (const char *expr, struct t_hashtable *pointers,
                   struct t_hashtable *extra_vars, int extra_vars_eval,
                   const char *prefix, const char *suffix,
                   struct t_eval_regex *eval_regex)
{
    const void *ptr[6];
    const char *no_replace_prefix_list[] = { "if:", NULL };

    ptr[0] = pointers;
    ptr[1] = extra_vars;
    ptr[2] = &extra_vars_eval;
    ptr[3] = prefix;
    ptr[4] = suffix;
    ptr[5] = eval_regex;

    return string_replace_with_callback (expr, prefix, suffix,
                                         no_replace_prefix_list,
                                         &eval_replace_vars_cb, ptr, NULL);
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
        regfree (&regex);
        if (comparison == EVAL_COMPARE_REGEX_NOT_MATCHING)
            rc ^= 1;
        goto end;
    }
    else if ((comparison == EVAL_COMPARE_STRING_MATCHING)
             || (comparison == EVAL_COMPARE_STRING_NOT_MATCHING))
    {
        rc = string_match (expr1, expr2, 0);
        if (comparison == EVAL_COMPARE_STRING_NOT_MATCHING)
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
 * Evaluates a condition (this function must not be called directly).
 *
 * For return value, see function eval_expression().
 *
 * Note: result must be freed after use (if not NULL).
 */

char *
eval_expression_condition (const char *expr,
                           struct t_hashtable *pointers,
                           struct t_hashtable *extra_vars,
                           int extra_vars_eval,
                           const char *prefix,
                           const char *suffix)
{
    int logic, comp, length, level, rc;
    const char *pos, *pos_end;
    char *expr2, *sub_expr, *value, *tmp_value, *tmp_value2;

    value = NULL;

    if (!expr)
        return NULL;

    if (!expr[0])
        return strdup (expr);

    /* skip spaces at beginning of string */
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

    /*
     * search for a logical operator, and if one is found:
     * - split expression into two sub-expressions
     * - evaluate first sub-expression
     * - if needed, evaluate second sub-expression
     * - return result
     */
    for (logic = 0; logic < EVAL_NUM_LOGICAL_OPS; logic++)
    {
        pos = eval_strstr_level (expr2, logical_ops[logic], "(", ")", 0);
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
            tmp_value = eval_expression_condition (sub_expr, pointers,
                                                   extra_vars, extra_vars_eval,
                                                   prefix, suffix);
            free (sub_expr);
            rc = eval_is_true (tmp_value);
            if (tmp_value)
                free (tmp_value);
            /*
             * if rc == 0 with "&&" or rc == 1 with "||", no need to
             * evaluate second sub-expression, just return the rc
             */
            if ((!rc && (logic == EVAL_LOGICAL_OP_AND))
                || (rc && (logic == EVAL_LOGICAL_OP_OR)))
            {
                value = strdup ((rc) ? EVAL_STR_TRUE : EVAL_STR_FALSE);
                goto end;
            }
            pos += strlen (logical_ops[logic]);
            while (pos[0] == ' ')
            {
                pos++;
            }
            tmp_value = eval_expression_condition (pos, pointers,
                                                   extra_vars, extra_vars_eval,
                                                   prefix, suffix);
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
        pos = eval_strstr_level (expr2, comparisons[comp], "(", ")", 0);
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
            pos += strlen (comparisons[comp]);
            while (pos[0] == ' ')
            {
                pos++;
            }
            if ((comp == EVAL_COMPARE_REGEX_MATCHING)
                || (comp == EVAL_COMPARE_REGEX_NOT_MATCHING))
            {
                /* for regex: just replace vars in both expressions */
                tmp_value = eval_replace_vars (sub_expr, pointers,
                                               extra_vars, extra_vars_eval,
                                               prefix, suffix,
                                               NULL);
                tmp_value2 = eval_replace_vars (pos, pointers,
                                                extra_vars, extra_vars_eval,
                                                prefix, suffix,
                                                NULL);
            }
            else
            {
                /* other comparison: fully evaluate both expressions */
                tmp_value = eval_expression_condition (sub_expr, pointers,
                                                       extra_vars,
                                                       extra_vars_eval,
                                                       prefix, suffix);
                tmp_value2 = eval_expression_condition (pos, pointers,
                                                        extra_vars,
                                                        extra_vars_eval,
                                                        prefix, suffix);
            }
            free (sub_expr);
            value = eval_compare (tmp_value, comp, tmp_value2);
            if (tmp_value)
                free (tmp_value);
            if (tmp_value2)
                free (tmp_value2);
            goto end;
        }
    }

    /*
     * evaluate sub-expressions between parentheses and replace them with their
     * value
     */
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
        /* closing parenthesis not found */
        if (pos[0] != ')')
            goto end;
        sub_expr = string_strndup (expr2 + 1, pos - expr2 - 1);
        if (!sub_expr)
            goto end;
        tmp_value = eval_expression_condition (sub_expr, pointers,
                                               extra_vars, extra_vars_eval,
                                               prefix, suffix);
        free (sub_expr);
        if (!pos[1])
        {
            /*
             * nothing around parentheses, then return value of
             * sub-expression as-is
             */
            value = tmp_value;
            goto end;
        }
        length = ((tmp_value) ? strlen (tmp_value) : 0) + 1 +
            strlen (pos + 1) + 1;
        tmp_value2 = malloc (length);
        if (!tmp_value2)
        {
            if (tmp_value)
                free (tmp_value);
            goto end;
        }
        tmp_value2[0] = '\0';
        if (tmp_value)
            strcat (tmp_value2, tmp_value);
        strcat (tmp_value2, " ");
        strcat (tmp_value2, pos + 1);
        free (expr2);
        expr2 = tmp_value2;
        if (tmp_value)
            free (tmp_value);
    }

    /*
     * at this point, there is no more logical operator neither comparison,
     * so we just replace variables in string and return the result
     */
    value = eval_replace_vars (expr2, pointers, extra_vars, extra_vars_eval,
                               prefix, suffix, NULL);

end:
    if (expr2)
        free (expr2);

    return value;
}

/*
 * Replaces text in a string using a regular expression and replacement text.
 *
 * The argument "regex" is a pointer to a regex compiled with WeeChat function
 * string_regcomp (or function regcomp).
 *
 * The argument "replace" is evaluated and can contain any valid expression,
 * and these ones:
 *   ${re:0} .. ${re:99}  match 0 to 99 (0 is whole match, 1 .. 99 are groups
 *                        captured)
 *   ${re:+}              the last match (with highest number)
 *
 * Examples:
 *
 *    string   | regex         | replace                    | result
 *   ----------+---------------+----------------------------+-------------
 *    test foo | test          | Z                          | Z foo
 *    test foo | ^(test +)(.*) | ${re:2}                    | foo
 *    test foo | ^(test +)(.*) | ${re:1}/ ${hide:*,${re:2}} | test / ***
 *    test foo | ^(test +)(.*) | ${hide:%,${re:+}}          | %%%
 *
 * Note: result must be freed after use.
 */

char *
eval_replace_regex (const char *string, regex_t *regex, const char *replace,
                    struct t_hashtable *pointers,
                    struct t_hashtable *extra_vars, int extra_vars_eval,
                    const char *prefix, const char *suffix)
{
    char *result, *result2, *str_replace;
    int length, length_replace, start_offset, i, rc, end;
    struct t_eval_regex eval_regex;

    if (!string || !regex || !replace)
        return NULL;

    length = strlen (string) + 1;
    result = malloc (length);
    if (!result)
        return NULL;
    snprintf (result, length, "%s", string);

    start_offset = 0;
    while (result && result[start_offset])
    {
        for (i = 0; i < 100; i++)
        {
            eval_regex.match[i].rm_so = -1;
        }

        rc = regexec (regex, result + start_offset, 100, eval_regex.match, 0);
        /*
         * no match found: exit the loop (if rm_eo == 0, it is an empty match
         * at beginning of string: we consider there is no match, to prevent an
         * infinite loop)
         */
        if ((rc != 0)
            || (eval_regex.match[0].rm_so < 0)
            || (eval_regex.match[0].rm_eo <= 0))
        {
            break;
        }

        /* adjust the start/end offsets */
        eval_regex.last_match = 0;
        for (i = 0; i < 100; i++)
        {
            if (eval_regex.match[i].rm_so >= 0)
            {
                eval_regex.last_match = i;
                eval_regex.match[i].rm_so += start_offset;
                eval_regex.match[i].rm_eo += start_offset;
            }
        }

        /* check if the regex matched the end of string */
        end = !result[eval_regex.match[0].rm_eo];

        eval_regex.result = result;

        str_replace = eval_replace_vars (replace, pointers,
                                         extra_vars, extra_vars_eval,
                                         prefix, suffix,
                                         &eval_regex);

        length_replace = (str_replace) ? strlen (str_replace) : 0;

        length = eval_regex.match[0].rm_so + length_replace +
            strlen (result + eval_regex.match[0].rm_eo) + 1;
        result2 = malloc (length);
        if (!result2)
        {
            free (result);
            return NULL;
        }
        result2[0] = '\0';
        if (eval_regex.match[0].rm_so > 0)
        {
            memcpy (result2, result, eval_regex.match[0].rm_so);
            result2[eval_regex.match[0].rm_so] = '\0';
        }
        if (str_replace)
            strcat (result2, str_replace);
        strcat (result2, result + eval_regex.match[0].rm_eo);

        free (result);
        result = result2;

        if (str_replace)
            free (str_replace);

        if (end)
            break;

        start_offset = eval_regex.match[0].rm_so + length_replace;
    }

    return result;
}

/*
 * Evaluates an expression.
 *
 * The hashtable "pointers" must have string for keys, pointer for values.
 * The hashtable "extra_vars" must have string for keys and values.
 * The hashtable "options" must have string for keys and values.
 *
 * Supported options:
 *   - prefix: change the default prefix before variables to replace ("${")
 *   - suffix: change the default suffix after variables to replace ('}")
 *   - type:
 *       - condition: evaluate as a condition (use operators/parentheses,
 *         return a boolean)
 *
 * If the expression is a condition, it can contain:
 *   - conditions:  ==  != <  <=  >  >=
 *   - logical operators:  &&  ||
 *   - parentheses for priority
 *
 * Examples of simple expression without condition (the [ ] are NOT part of
 * result):
 *   >> ${window.buffer.number}
 *   == [2]
 *   >> buffer:${window.buffer.full_name}
 *   == [buffer:irc.freenode.#weechat]
 *   >> ${window.win_width}
 *   == [112]
 *   >> ${window.win_height}
 *   == [40]
 *
 * Examples of conditions:
 *   >> ${window.buffer.full_name} == irc.freenode.#weechat
 *   == [1]
 *   >> ${window.buffer.full_name} == irc.freenode.#test
 *   == [0]
 *   >> ${window.win_width} >= 30 && ${window.win_height} >= 20
 *   == [1]
 *
 * Note: result must be freed after use (if not NULL).
 */

char *
eval_expression (const char *expr, struct t_hashtable *pointers,
                 struct t_hashtable *extra_vars, struct t_hashtable *options)
{
    int condition, extra_vars_eval, rc, pointers_allocated, regex_allocated;
    char *value;
    const char *prefix, *suffix;
    const char *default_prefix = EVAL_DEFAULT_PREFIX;
    const char *default_suffix = EVAL_DEFAULT_SUFFIX;
    const char *ptr_value, *regex_replace;
    struct t_gui_window *window;
    regex_t *regex;

    if (!expr)
        return NULL;

    condition = 0;
    extra_vars_eval = 0;
    pointers_allocated = 0;
    regex_allocated = 0;
    prefix = default_prefix;
    suffix = default_suffix;
    regex = NULL;
    regex_replace = NULL;

    if (pointers)
    {
        regex = (regex_t *)hashtable_get (pointers, "regex");
    }
    else
    {
        /* create hashtable pointers if it's NULL */
        pointers = hashtable_new (32,
                                  WEECHAT_HASHTABLE_STRING,
                                  WEECHAT_HASHTABLE_POINTER,
                                  NULL,
                                  NULL);
        if (!pointers)
            return NULL;
        pointers_allocated = 1;
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

    /* read options */
    if (options)
    {
        /* check the type of evaluation */
        ptr_value = hashtable_get (options, "type");
        if (ptr_value && (strcmp (ptr_value, "condition") == 0))
            condition = 1;

        /* check if extra vars must be evaluated */
        ptr_value = hashtable_get (options, "extra");
        if (ptr_value && (strcmp (ptr_value, "eval") == 0))
            extra_vars_eval = 1;

        /* check for custom prefix */
        ptr_value = hashtable_get (options, "prefix");
        if (ptr_value && ptr_value[0])
            prefix = ptr_value;

        /* check for custom suffix */
        ptr_value = hashtable_get (options, "suffix");
        if (ptr_value && ptr_value[0])
            suffix = ptr_value;

        /* check for regex */
        ptr_value = hashtable_get (options, "regex");
        if (ptr_value)
        {
            regex = malloc (sizeof (*regex));
            if (string_regcomp (regex, ptr_value,
                                REG_EXTENDED | REG_ICASE) == 0)
            {
                regex_allocated = 1;
            }
            else
            {
                free (regex);
                regex = NULL;
            }
        }

        /* check for regex replacement (evaluated later) */
        ptr_value = hashtable_get (options, "regex_replace");
        if (ptr_value)
        {
            regex_replace = ptr_value;
        }
    }

    /* evaluate expression */
    if (condition)
    {
        /* evaluate as condition (return a boolean: "0" or "1") */
        value = eval_expression_condition (expr, pointers,
                                           extra_vars, extra_vars_eval,
                                           prefix, suffix);
        rc = eval_is_true (value);
        if (value)
            free (value);
        value = strdup ((rc) ? EVAL_STR_TRUE : EVAL_STR_FALSE);
    }
    else
    {
        if (regex && regex_replace)
        {
            /* replace with regex */
            value = eval_replace_regex (expr, regex, regex_replace,
                                        pointers, extra_vars, extra_vars_eval,
                                        prefix, suffix);
        }
        else
        {
            /* only replace variables in expression */
            value = eval_replace_vars (expr, pointers,
                                       extra_vars, extra_vars_eval,
                                       prefix, suffix, NULL);
        }
    }

    if (pointers_allocated)
        hashtable_free (pointers);
    if (regex && regex_allocated)
    {
        regfree (regex);
        free (regex);
    }

    return value;
}
