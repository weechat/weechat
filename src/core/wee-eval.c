/*
 * wee-eval.c - evaluate expressions with references to internal vars
 *
 * Copyright (C) 2012-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdarg.h>
#include <regex.h>
#include <time.h>

#include "weechat.h"
#include "wee-eval.h"
#include "wee-calc.h"
#include "wee-config-file.h"
#include "wee-hashtable.h"
#include "wee-hdata.h"
#include "wee-hook.h"
#include "wee-secure.h"
#include "wee-string.h"
#include "wee-utf8.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"


#define EVAL_DEBUG_MSG(level, msg, argz...)                             \
    debug_id = -1;                                                      \
    if (eval_context->debug_level >= level)                             \
    {                                                                   \
        debug_id = ++(eval_context->debug_id);                          \
        (eval_context->debug_depth)++;                                  \
        eval_debug_message_vargs (eval_context, debug_id, msg, ##argz); \
    }

#define EVAL_DEBUG_RESULT(level, result)                                \
    if (eval_context->debug_level >= level)                             \
    {                                                                   \
        eval_debug_message (eval_context, debug_id, 1, result);         \
        (eval_context->debug_depth)--;                                  \
    }


char *eval_logical_ops[EVAL_NUM_LOGICAL_OPS] =
{ "||", "&&" };

char *eval_comparisons[EVAL_NUM_COMPARISONS] =
{ "=~", "!~",                /* regex */
  "==*", "!!*", "=*", "!*",  /* string match */
  "==-", "!!-", "=-", "!-",  /* includes */
  "==", "!=",                /* equal, not equal */
  "<=", "<", ">=", ">",      /* less than, greater than */
};

char *eval_range_chars[][2] =
{
    { "digit",  EVAL_RANGE_DIGIT },
    { "xdigit", EVAL_RANGE_XDIGIT },
    { "lower",  EVAL_RANGE_LOWER },
    { "upper",  EVAL_RANGE_UPPER },
    { "alpha",  EVAL_RANGE_ALPHA },
    { "alnum",  EVAL_RANGE_ALNUM },
    { NULL,     NULL },
};

char *eval_replace_vars (const char *expr,
                         struct t_eval_context *eval_context);
char *eval_expression_condition (const char *expr,
                                 struct t_eval_context *eval_context);


/*
 * Adds a debug message in the debug output.
 */

void
eval_debug_message (struct t_eval_context *eval_context, int debug_id,
                    int result, const char *message)
{
    int i;
    char str_id[64];

    if (*(eval_context->debug_output)[0])
        string_dyn_concat (eval_context->debug_output, "\n", -1);

    /* indentation */
    for (i = 1; i < eval_context->debug_depth; i++)
    {
        string_dyn_concat (eval_context->debug_output, "  ", -1);
    }

    /* debug id */
    if (debug_id >= 0)
    {
        snprintf (str_id, sizeof (str_id), "%d:", debug_id);
        string_dyn_concat (eval_context->debug_output, str_id, -1);
    }

    /* debug message */
    if (result)
    {
        string_dyn_concat (eval_context->debug_output, "== ", -1);
        if (message)
            string_dyn_concat (eval_context->debug_output, "\"", -1);
        string_dyn_concat (eval_context->debug_output,
                           (message) ? message : "null",
                           -1);
        if (message)
            string_dyn_concat (eval_context->debug_output, "\"", -1);
    }
    else
    {
        string_dyn_concat (eval_context->debug_output, message, -1);
    }
}


/*
 * Adds a debug message in the debug output, with variable arguments.
 */

void
eval_debug_message_vargs (struct t_eval_context *eval_context, int debug_id,
                          const char *message, ...)
{
    weechat_va_format (message);
    if (vbuffer)
    {
        eval_debug_message (eval_context, debug_id, 0,
                            vbuffer);
        free (vbuffer);
    }
}

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
 * For example:
 *   eval_strstr_level ("(x || y) || z", "||", eval_context, "(", ")", 0)
 * will return a pointer on  "|| z" (because the first "||" is
 * in a sub-expression, which is skipped).
 *
 * Returns pointer to string found, or NULL if not found.
 */

const char *
eval_strstr_level (const char *string, const char *search,
                   struct t_eval_context *eval_context,
                   const char *extra_prefix, const char *extra_suffix,
                   int escape)
{
    const char *ptr_string;
    int level, length_search, debug_id;
    int length_prefix2, length_suffix2;

    ptr_string = NULL;

    EVAL_DEBUG_MSG(2, "eval_strstr_level(\"%s\", \"%s\", \"%s\", \"%s\", %d)",
                   string, search, extra_prefix, extra_suffix, escape);

    if (!string || !search)
    {
        ptr_string = NULL;
        goto end;
    }

    length_search = strlen (search);

    length_prefix2 = (extra_prefix) ? strlen (extra_prefix) : 0;
    length_suffix2 = (extra_suffix) ? strlen (extra_suffix) : 0;

    ptr_string = string;
    level = 0;
    while (ptr_string[0])
    {
        if (escape
            && (ptr_string[0] == '\\')
            && ((ptr_string[1] == eval_context->prefix[0])
                || ((length_suffix2 > 0) && ptr_string[1] == extra_prefix[0])))
        {
            ptr_string++;
        }
        else if (strncmp (ptr_string, eval_context->prefix, eval_context->length_prefix) == 0)
        {
            level++;
            ptr_string += eval_context->length_prefix;
        }
        else if ((length_prefix2 > 0)
                 && (strncmp (ptr_string, extra_prefix, length_prefix2) == 0))
        {
            level++;
            ptr_string += length_prefix2;
        }
        else if (strncmp (ptr_string, eval_context->suffix, eval_context->length_suffix) == 0)
        {
            if (level > 0)
                level--;
            ptr_string += eval_context->length_suffix;
        }
        else if ((length_suffix2 > 0)
                 && (strncmp (ptr_string, extra_suffix, length_suffix2) == 0))
        {
            if (level > 0)
                level--;
            ptr_string += length_suffix2;
        }
        else if ((level == 0)
                 && (strncmp (ptr_string, search, length_search) == 0))
        {
            goto end;
        }
        else
        {
            ptr_string++;
        }
    }
    ptr_string = NULL;

end:
    EVAL_DEBUG_RESULT(2, ptr_string);

    return ptr_string;
}

/*
 * Evaluates a condition and returns boolean result:
 *   "0" if false
 *   "1" if true
 *
 * Note: result must be freed after use.
 */

char *
eval_string_eval_cond (const char *text, struct t_eval_context *eval_context)
{
    char *tmp;
    int rc;

    tmp = eval_expression_condition (text, eval_context);
    rc = eval_is_true (tmp);
    if (tmp)
        free (tmp);
    return strdup ((rc) ? EVAL_STR_TRUE : EVAL_STR_FALSE);
}

/*
 * Adds range of chars.
 *
 * Note: result must be freed after use.
 */

char *
eval_string_range_chars (const char *range)
{
    int i, char1, char2;
    const char *ptr_char;
    char **string, str_char[16], *result;

    string = NULL;
    result = NULL;

    for (i = 0; eval_range_chars[i][0]; i++)
    {
        if (strcmp (range, eval_range_chars[i][0]) == 0)
            return strdup (eval_range_chars[i][1]);
    }

    char1 = utf8_char_int (range);

    /* next char must be '-' */
    ptr_char = utf8_next_char (range);
    if (!ptr_char || !ptr_char[0] || (ptr_char[0] != '-'))
        goto end;

    /* next char is the char2 */
    ptr_char = utf8_next_char (ptr_char);
    if (!ptr_char || !ptr_char[0])
        goto end;
    char2 = utf8_char_int (ptr_char);

    /* output is limited to 1024 chars (not bytes) */
    if ((char1 > char2) || (char2 - char1  + 1 > 4096))
        goto end;

    string = string_dyn_alloc (128);
    if (!string)
        goto end;

    for (i = char1; i <= char2; i++)
    {
        utf8_int_string (i, str_char);
        string_dyn_concat (string, str_char, -1);
    }

end:
    if (string)
        result = string_dyn_free (string, 0);

    return (result) ? result : strdup ("");
}

/*
 * Hides chars in a string.
 *
 * Note: result must be freed after use.
 */

char *
eval_string_hide (const char *text)
{
    const char *ptr_string;
    char *hidden_string, *hide_char;
    int i, j, length, length_hide_char;

    hidden_string = NULL;

    ptr_string = strchr (text, (text[0] == ',') ? ';' : ',');
    if (!ptr_string)
        return strdup ("");

    hide_char = string_strndup (text, ptr_string - text);
    if (hide_char)
    {
        length_hide_char = strlen (hide_char);
        length = utf8_strlen (ptr_string + 1);
        hidden_string = malloc ((length * length_hide_char) + 1);
        if (hidden_string)
        {
            j = 0;
            for (i = 0; i < length; i++)
            {
                memcpy (hidden_string + j, hide_char, length_hide_char);
                j += length_hide_char;
            }
            hidden_string[length * length_hide_char] = '\0';
        }
        free (hide_char);
    }

    return (hidden_string) ? hidden_string : strdup ("");
}

/*
 * Cuts string.
 *
 * Note: result must be freed after use.
 */

char *
eval_string_cut (const char *text, int screen)
{
    const char *pos, *pos2;
    char *tmp, *error, *value;
    int count_suffix;
    long number;

    count_suffix = 0;
    if (text[0] == '+')
    {
        text++;
        count_suffix = 1;
    }

    pos = strchr (text, ',');
    if (!pos)
        return strdup ("");

    pos2 = strchr (pos + 1, ',');
    if (!pos2)
        return strdup ("");

    tmp = string_strndup (text, pos - text);
    if (!tmp)
        return strdup ("");

    number = strtol (tmp, &error, 10);
    if (!error || error[0] || (number < 0))
    {
        free (tmp);
        return strdup ("");
    }
    free (tmp);

    tmp = string_strndup (pos + 1, pos2 - pos - 1);
    if (!tmp)
        return strdup ("");

    value = string_cut (pos2 + 1, number, count_suffix, screen, tmp);

    free (tmp);

    return value;
}

/*
 * Repeats string.
 *
 * Note: result must be freed after use.
 */

char *
eval_string_repeat (const char *text)
{
    const char *pos;
    char *tmp, *error;
    long number;

    pos = strchr (text, ',');
    if (!pos)
        return strdup ("");

    tmp = string_strndup (text, pos - text);
    if (!tmp)
        return strdup ("");

    number = strtol (tmp, &error, 10);
    if (!error || error[0] || (number < 0))
    {
        free (tmp);
        return strdup ("");
    }
    free (tmp);

    return string_repeat (pos + 1, number);
}

/*
 * Splits string.
 *
 * Format: number,separators,flags,string
 *
 * If number == "count", returns the number of items after split.
 * If number == "random", returns a random item.
 * If number > 0, return this index (empty string if not enough items).
 * If number < 0, return this index starting from the end (-1 = last item,
 * -2 = penultimate item, etc.).
 *
 * If separators is empty string, a comma is used by default.
 *
 * Flags is a list of flags, separated by "+":
 *   strip_left
 *   strip_right
 *   collapse_seps
 *   keep_eol
 *   strip_items=xyz
 *   max_items=N
 *
 * Examples:
 *   ${split:1,,,abc,def,ghi}                              ==> abc
 *   ${split:-1,,,abc,def,ghi}                             ==> ghi
 *   ${split:count,,,abc,def,ghi}                          ==> 3
 *   ${split:random,,,abc,def,ghi}                         ==> def
 *   ${split:3,,collapse_seps,abc,,,def,,,ghi}             ==> ghi
 *   ${split:3,,strip_items=-_,_-abc-_,_-def-_,_-ghi-_}    ==> ghi
 *   ${split:2, ,,this is a test}                          ==> is
 *   ${split:2, ,strip_left+strip_right, this is a test }  ==> is
 *   ${split:2, ,keep_eol,this is a test}                  ==> is a test
 *
 * Note: result must be freed after use.
 */

char *
eval_string_split (const char *text)
{
    char *pos, *pos2, *pos3, *str_number, *separators, **items, *value, *error;
    char str_value[32], *str_flags, **list_flags, *strip_items;
    int i, num_items, count_items, random_item, flags;
    long number, max_items;

    str_number = NULL;
    separators = NULL;
    items = NULL;
    value = NULL;
    str_flags = NULL;
    list_flags = NULL;
    strip_items = NULL;
    count_items = 0;
    random_item = 0;
    flags = 0;
    max_items = 0;

    if (!text || !text[0])
        goto end;

    pos = strchr (text, ',');
    if (!pos || (pos == text))
        goto end;

    number = 0;
    str_number = string_strndup (text, pos - text);
    if (strcmp (str_number, "count") == 0)
    {
        count_items = 1;
    }
    else if (strcmp (str_number, "random") == 0)
    {
        random_item = 1;
    }
    else
    {
        number = strtol (str_number, &error, 10);
        if (!error || error[0] || (number == 0))
            goto end;
    }

    pos++;
    pos2 = strchr (pos, ',');
    if (!pos2)
        goto end;
    if (pos2 == pos)
        separators = strdup (",");
    else
        separators = string_strndup (pos, pos2 - pos);

    pos2++;
    pos3 = strchr (pos2, ',');
    if (!pos3)
        goto end;
    str_flags = string_strndup (pos2, pos3 - pos2);
    list_flags = string_split (str_flags, "+", NULL, 0, 0, NULL);
    if (list_flags)
    {
        for (i = 0; list_flags[i]; i++)
        {
            if (strcmp (list_flags[i], "strip_left") == 0)
                flags |= WEECHAT_STRING_SPLIT_STRIP_LEFT;
            else if (strcmp (list_flags[i], "strip_right") == 0)
                flags |= WEECHAT_STRING_SPLIT_STRIP_RIGHT;
            else if (strcmp (list_flags[i], "collapse_seps") == 0)
                flags |= WEECHAT_STRING_SPLIT_COLLAPSE_SEPS;
            else if (strcmp (list_flags[i], "keep_eol") == 0)
                flags |= WEECHAT_STRING_SPLIT_KEEP_EOL;
            else if (strncmp (list_flags[i], "strip_items=", 12) == 0)
            {
                if (strip_items)
                    free (strip_items);
                strip_items = strdup (list_flags[i] + 12);
            }
            else if (strncmp (list_flags[i], "max_items=", 10) == 0)
            {
                max_items = strtol (list_flags[i] + 10, &error, 10);
                if (!error || error[0] || (max_items < 0))
                    goto end;
            }
        }
    }

    pos3++;

    items = string_split (pos3, separators, strip_items, flags,
                          max_items, &num_items);

    /* if "count" was asked, return the number of items found after split */
    if (count_items)
    {
        snprintf (str_value, sizeof (str_value), "%d", num_items);
        value = strdup (str_value);
        goto end;
    }

    if (!items || (num_items < 1))
        goto end;

    /* if "random" was asked, return a random item */
    if (random_item)
        number = random () % num_items;
    else if (number > 0)
        number--;

    if (((number >= 0) && (number >= num_items))
        || ((number < 0) && (labs (number) > num_items)))
    {
        goto end;
    }

    if (number < 0)
        number = num_items + number;

    value = strdup (items[number]);

end:
    if (str_number)
        free (str_number);
    if (separators)
        free (separators);
    if (str_flags)
        free (str_flags);
    if (list_flags)
        string_free_split (list_flags);
    if (strip_items)
        free (strip_items);
    if (items)
        string_free_split (items);
    return (value) ? value : strdup ("");
}

/*
 * Splits shell arguments.
 *
 * Format: number,string
 *
 * If number == "count", returns the number of arguments.
 * If number == "random", returns a random argument.
 * If number > 0, return this index (empty string if not enough arguments).
 * If number < 0, return this index starting from the end (-1 = last argument,
 * -2 = penultimate argument, etc.).
 *
 * Examples:
 *   ${split_shell:count,"arg 1" arg2}   ==> "2"
 *   ${split_shell:random,"arg 1" arg2}  ==> "arg2"
 *   ${split_shell:1,"arg 1" arg2}       ==> "arg 1"
 *   ${split_shell:-1,"arg 1" arg2}      ==> "arg2"
 *
 * Note: result must be freed after use.
 */

char *
eval_string_split_shell (const char *text)
{
    char *pos, *str_number, **items, *value, *error, str_value[32];
    int num_items, count_items, random_item;
    long number;

    str_number = NULL;
    items = NULL;
    value = NULL;
    count_items = 0;
    random_item = 0;

    if (!text || !text[0])
        goto end;

    pos = strchr (text, ',');
    if (!pos || (pos == text))
        goto end;

    number = 0;
    str_number = string_strndup (text, pos - text);
    if (strcmp (str_number, "count") == 0)
    {
        count_items = 1;
    }
    else if (strcmp (str_number, "random") == 0)
    {
        random_item = 1;
    }
    else
    {
        number = strtol (str_number, &error, 10);
        if (!error || error[0] || (number == 0))
            goto end;
    }

    pos++;

    items = string_split_shell (pos, &num_items);

    /* if "count" was asked, return the number of items found after split */
    if (count_items)
    {
        snprintf (str_value, sizeof (str_value), "%d", num_items);
        value = strdup (str_value);
        goto end;
    }

    if (!items || (num_items < 1))
        goto end;

    /* if "random" was asked, return a random item */
    if (random_item)
        number = random () % num_items;
    else if (number > 0)
        number--;

    if (((number >= 0) && (number >= num_items))
        || ((number < 0) && (labs (number) > num_items)))
    {
        goto end;
    }

    if (number < 0)
        number = num_items + number;

    value = strdup (items[number]);

end:
    if (str_number)
        free (str_number);
    if (items)
        string_free_split (items);
    return (value) ? value : strdup ("");
}

/*
 * Returns a regex group captured.
 *
 * Note: result must be freed after use.
 */

char *
eval_string_regex_group (const char *text, struct t_eval_context *eval_context)
{
    char str_value[64], *error;
    long number;

    if (!eval_context->regex || !eval_context->regex->result)
        return strdup ("");

    if (strcmp (text, "#") == 0)
    {
        snprintf (str_value, sizeof (str_value),
                  "%d", eval_context->regex->last_match);
        return strdup (str_value);
    }

    if (strcmp (text, "repl_index") == 0)
    {
        snprintf (str_value, sizeof (str_value),
                  "%d", eval_context->regex_replacement_index);
        return strdup (str_value);
    }

    if (strcmp (text, "+") == 0)
    {
        number = eval_context->regex->last_match;
    }
    else
    {
        number = strtol (text, &error, 10);
        if (!error || error[0])
            number = -1;
    }
    if ((number >= 0) && (number <= eval_context->regex->last_match))
    {
        return string_strndup (
            eval_context->regex->result +
            eval_context->regex->match[number].rm_so,
            eval_context->regex->match[number].rm_eo -
            eval_context->regex->match[number].rm_so);
    }

    return strdup ("");
}

/*
 * Returns a string with color code.
 *
 * Note: result must be freed after use.
 */

char *
eval_string_color (const char *text)
{
    const char *ptr_value;

    ptr_value = gui_color_search_config (text);
    if (ptr_value)
        return strdup (ptr_value);

    ptr_value = gui_color_get_custom (text);
    return strdup ((ptr_value) ? ptr_value : "");
}

/*
 * Returns a string modified by a modifier.
 *
 * Note: result must be freed after use.
 */

char *
eval_string_modifier (const char *text)
{
    const char *ptr_arguments, *ptr_string;
    char *value, *modifier_name, *modifier_data;

    value = NULL;

    ptr_arguments = strchr (text, ',');
    if (!ptr_arguments)
        return strdup ("");

    ptr_arguments++;
    ptr_string = strchr (ptr_arguments, ',');
    if (!ptr_string)
        return strdup ("");

    ptr_string++;
    modifier_name = string_strndup (text, ptr_arguments - 1 - text);
    modifier_data = string_strndup (ptr_arguments,
                                    ptr_string - 1 - ptr_arguments);
    value = hook_modifier_exec (NULL, modifier_name, modifier_data,
                                ptr_string);
    if (modifier_name)
        free (modifier_name);
    if (modifier_data)
        free (modifier_data);

    return (value) ? value : strdup ("");
}

/*
 * Returns an info.
 *
 * Note: result must be freed after use.
 */

char *
eval_string_info (const char *text)
{
    const char *ptr_arguments;
    char *value, *info_name;

    value = NULL;

    ptr_arguments = strchr (text, ',');
    if (ptr_arguments)
    {
        info_name = string_strndup (text, ptr_arguments - text);
        ptr_arguments++;
    }
    else
    {
        info_name = strdup (text);
    }

    if (info_name)
    {
        value = hook_info_get (NULL, info_name, ptr_arguments);
        free (info_name);
    }

    return (value) ? value : strdup ("");
}

/*
 * Encodes a string in base 16, 32, or 64.
 *
 * Note: result must be freed after use.
 */

char *
eval_string_base_encode (const char *text)
{
    const char *ptr_string;
    char *value, *base, *error, *result;
    long number;
    int length;

    base = NULL;
    result = NULL;

    ptr_string = strchr (text, ',');
    if (!ptr_string)
        goto end;

    base = string_strndup (text, ptr_string - text);
    if (!base)
        goto end;

    number = strtol (base, &error, 10);
    if (!error || error[0])
        goto end;

    ptr_string++;
    length = strlen (ptr_string);
    result = malloc ((length * 4) + 1);
    if (!result)
        goto end;

    if (string_base_encode (number, ptr_string, length, result) < 0)
    {
        free (result);
        result = NULL;
    }

end:
    value = strdup ((result) ? result : "");
    if (base)
        free (base);
    if (result)
        free (result);
    return value;
}

/*
 * Decodes a string encoded in base 16, 32, or 64.
 *
 * Note: result must be freed after use.
 */

char *
eval_string_base_decode (const char *text)
{
    const char *ptr_string;
    char *value, *base, *error, *result;
    long number;

    base = NULL;
    result = NULL;

    ptr_string = strchr (text, ',');
    if (!ptr_string)
        goto end;

    base = string_strndup (text, ptr_string - text);
    if (!base)
        goto end;

    number = strtol (base, &error, 10);
    if (!error || error[0])
        goto end;

    ptr_string++;
    result = malloc (strlen (ptr_string) + 1);
    if (!result)
        goto end;

    if (string_base_decode (number, ptr_string, result) < 0)
    {
        free (result);
        result = NULL;
    }

end:
    value = strdup ((result) ? result : "");
    if (base)
        free (base);
    if (result)
        free (result);
    return value;
}

/*
 * Returns a date.
 *
 * Note: result must be freed after use.
 */

char *
eval_string_date (const char *text)
{
    char str_value[512];
    time_t date;
    struct tm *date_tmp;
    int rc;

    date = time (NULL);
    date_tmp = localtime (&date);
    if (!date_tmp)
        return strdup ("");

    rc = (int) strftime (str_value, sizeof (str_value),
                         (text[0] == ':') ? text + 1 : "%F %T",
                         date_tmp);

    return strdup ((rc > 0) ? str_value : "");
}

/*
 * Evaluates a condition and returns evaluated if/else clause.
 *
 * Note: result must be freed after use.
 */

char *
eval_string_if (const char *text, struct t_eval_context *eval_context)
{
    const char *pos, *pos2;
    char *value, *condition, *tmp;
    int rc;

    value = NULL;
    pos = (char *)eval_strstr_level (text, "?",
                                     eval_context, NULL, NULL, 1);
    pos2 = (pos) ?
        (char *)eval_strstr_level (pos + 1, ":",
                                   eval_context, NULL, NULL, 1) : NULL;
    condition = (pos) ?
        string_strndup (text, pos - text) : strdup (text);
    if (!condition)
        return strdup ("");
    tmp = eval_expression_condition (condition, eval_context);
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
                string_strndup (pos + 1, pos2 - pos - 1) : strdup (pos + 1);
            if (tmp)
            {
                value = eval_replace_vars (tmp, eval_context);
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
            value = eval_replace_vars (pos2 + 1, eval_context);
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

/*
 * Returns a random integer number.
 *
 * Note: result must be freed after use.
 */

char *
eval_string_random (const char *text)
{
    char *pos, *error, *tmp, result[128];
    long long min_number, max_number;

    if (!text || !text[0])
        goto error;

    pos = strchr (text, ',');
    if (!pos)
        goto error;

    tmp = string_strndup (text, pos - text);
    if (!tmp)
        goto error;
    min_number = strtoll (tmp, &error, 10);
    if (!error || error[0])
    {
        free (tmp);
        goto error;
    }
    free (tmp);

    max_number = strtoll (pos + 1, &error, 10);
    if (!error || error[0])
        goto error;

    if (min_number > max_number)
        goto error;
    if (max_number - min_number > RAND_MAX)
        goto error;

    /*
     * using modulo division on the random() value produces a biased result,
     * but this is enough for our usage here
     */
    snprintf (result, sizeof (result),
              "%lld", min_number + (random () % (max_number - min_number + 1)));
    return strdup (result);

error:
    return strdup ("0");
}

/*
 * Translates text.
 *
 * Note: result must be freed after use.
 */

char *
eval_string_translate (const char *text)
{
    const char *ptr_string;

    if (!text || !text[0])
        return strdup ("");

    ptr_string = gettext (text);

    return strdup ((ptr_string) ? ptr_string : "");
}

/*
 * Defines a variable.
 */

void
eval_string_define (const char *text, struct t_eval_context *eval_context)
{
    char *pos, *name;

    pos = strchr (text, ',');
    if (!pos)
        return;

    name = string_strndup (text, pos - text);
    if (!name)
        return;

    hashtable_set (eval_context->user_vars, name, pos + 1);

    free (name);
}

/*
 * Gets value of hdata using "path" to a variable.
 *
 * Note: result must be freed after use.
 */

char *
eval_hdata_get_value (struct t_hdata *hdata, void *pointer, const char *path,
                      struct t_eval_context *eval_context)
{
    char *value, *old_value, *var_name, str_value[128], *pos, *property;
    const char *ptr_value, *hdata_name, *ptr_var_name, *pos_open_paren;
    int type, debug_id;
    struct t_hashtable *hashtable;

    EVAL_DEBUG_MSG(1, "eval_hdata_get_value(\"%s\", 0x%lx, \"%s\")",
                   (hdata) ? hdata->name : "(null)",
                   pointer,
                   path);

    value = NULL;
    var_name = NULL;

    /* NULL pointer? return empty string */
    if (!pointer)
    {
        value = strdup ("");
        goto end;
    }

    /* no path? just return current pointer as string */
    if (!path || !path[0])
    {
        snprintf (str_value, sizeof (str_value),
                  "0x%lx", (unsigned long)pointer);
        value = strdup (str_value);
        goto end;
    }

    if (!hdata)
        goto end;

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
    hdata_get_index_and_name (var_name, NULL, &ptr_var_name);
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
                      "0x%lx", (unsigned long)pointer);
            value = strdup (str_value);
            break;
        case WEECHAT_HDATA_TIME:
            snprintf (str_value, sizeof (str_value),
                      "%lld", (long long)hdata_time (hdata, pointer, var_name));
            value = strdup (str_value);
            break;
        case WEECHAT_HDATA_HASHTABLE:
            pointer = hdata_hashtable (hdata, pointer, var_name);
            if (pos)
            {
                /*
                 * for a hashtable, if there is a "." after name of hdata:
                 * 1) If "()" is at the end, it is a function call to
                 *    hashtable_get_string().
                 * 2) Otherwise, get the value for this key in hashtable.
                 */
                hashtable = pointer;

                pos_open_paren = strchr (pos, '(');
                if (pos_open_paren
                    && (pos_open_paren > pos + 1)
                    && (pos_open_paren[1] == ')'))
                {
                    property = string_strndup (pos + 1,
                                               pos_open_paren - pos - 1);
                    ptr_value = hashtable_get_string (hashtable, property);
                    free (property);
                    value = (ptr_value) ? strdup (ptr_value) : NULL;
                    break;
                }

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
                                      "0x%lx", (unsigned long)ptr_value);
                            value = strdup (str_value);
                            break;
                        case HASHTABLE_TIME:
                            snprintf (str_value, sizeof (str_value),
                                      "%lld", (long long)(*((time_t *)ptr_value)));
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
                          "0x%lx", (unsigned long)pointer);
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
        value = eval_hdata_get_value (hdata,
                                      pointer,
                                      (pos) ? pos + 1 : NULL,
                                      eval_context);
        if (old_value)
            free (old_value);
    }

end:
    if (var_name)
        free (var_name);

    EVAL_DEBUG_RESULT(1, value);

    return value;
}

/*
 * Returns a string using hdata.
 *
 * Note: result must be freed after use.
 */

char *
eval_string_hdata (const char *text, struct t_eval_context *eval_context)
{
    const char *pos_vars, *pos1, *pos2;
    char *value, *hdata_name, *pointer_name, *tmp;
    void *pointer;
    struct t_hdata *hdata;
    int rc;
    unsigned long ptr;

    value = NULL;
    hdata_name = NULL;
    pointer_name = NULL;
    pointer = NULL;

    pos_vars = strchr (text, '.');
    if (pos_vars > text)
        hdata_name = string_strndup (text, pos_vars - text);
    else
        hdata_name = strdup (text);
    if (pos_vars)
        pos_vars++;

    if (!hdata_name)
        goto end;

    pos1 = strchr (hdata_name, '[');
    if (pos1 > hdata_name)
    {
        pos2 = strchr (pos1 + 1, ']');
        if (pos2)
        {
            if (pos2 > pos1 + 1)
                pointer_name = string_strndup (pos1 + 1, pos2 - pos1 - 1);
            else
                goto end;
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
    {
        if (pos_vars || pointer_name)
            goto end;
        /* case of a single pointer which is not hdata, eg: ${my_pointer} */
        pointer = hashtable_get (eval_context->pointers, hdata_name);
        value = eval_hdata_get_value (NULL, pointer, pos_vars, eval_context);
        goto end;
    }

    if (pointer_name)
    {
        if (strncmp (pointer_name, "0x", 2) == 0)
        {
            rc = sscanf (pointer_name, "%lx", &ptr);
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
        {
            pointer = hdata_get_list (hdata, pointer_name);
            if (!pointer)
            {
                pointer = hashtable_get (eval_context->pointers, pointer_name);
                if (!pointer)
                    goto end;
                if (!hdata_check_pointer (hdata, NULL, pointer))
                    goto end;
            }
        }
    }

    if (!pointer)
    {
        pointer = hashtable_get (eval_context->pointers, hdata_name);
        if (!pointer)
            goto end;
        if (!hdata_check_pointer (hdata, NULL, pointer))
            goto end;
    }

    value = eval_hdata_get_value (hdata, pointer, pos_vars, eval_context);

end:
    if (hdata_name)
        free (hdata_name);
    if (pointer_name)
        free (pointer_name);

    return (value) ? value : strdup ("");
}

/*
 * Replaces variables, which can be, by order of priority:
 *  - the string itself without evaluation (format: raw:xxx)
 *  - a variable from hashtable "user_vars" or "extra_vars"
 *  - a WeeChat home directory, one of: "weechat_config_dir",
 *    "weechat_data_dir", "weechat_cache_dir", "weechat_runtime_dir"
 *  - a string to evaluate (format: eval:xxx)
 *  - a condition to evaluate (format: eval_cond:xxx)
 *  - a string with escaped chars (format: esc:xxx or \xxx)
 *  - a string with a range of chars (format: chars:xxx)
 *  - a string converted to lower case (format: lower:xxx)
 *  - a string converted to upper case (format: upper:xxx)
 *  - a string with chars to hide (format: hide:char,string)
 *  - a string with max chars (format: cut:max,suffix,string or
 *    cut:+max,suffix,string) or max chars on screen
 *    (format: cutscr:max,suffix,string or cutscr:+max,suffix,string)
 *  - a reversed string (format: rev:xxx) or reversed string for screen,
 *    color codes are not reversed (format: revscr:xxx)
 *  - a repeated string (format: repeat:count,string)
 *  - length of a string (format: length:xxx) or length of a string on screen
 *    (format: lengthscr:xxx); color codes are ignored
 *  - split string (format: split:number,separators,flags,xxx
 *    or split:count,separators,flags,xxx
 *    or split:random,separators,flags,xxx)
 *  - split shell arguments (format: split:number,xxx or split:count,xxx
 *    or split:random,xxx)
 *  - a regex group captured (format: re:N (0.99) or re:+)
 *  - a color (format: color:xxx)
 *  - a modifier (format: modifier:name,data,xxx)
 *  - an info (format: info:name,arguments)
 *  - a base 16/32/64 encoded/decoded string (format: base_encode:base,xxx
 *    or base_decode:base,xxx)
 *  - current date/time (format: date or date:xxx)
 *  - an environment variable (format: env:XXX)
 *  - a ternary operator (format: if:condition?value_if_true:value_if_false)
 *  - calculate result of an expression (format: calc:xxx)
 *  - a random integer number in the range from "min" to "max"
 *    (format: random:min,max)
 *  - a translated string (format: translate:xxx)
 *  - define a new variable (format: define:name,value)
 *  - an option (format: file.section.option)
 *  - a buffer local variable
 *  - a pointer name from hashtable "pointers"
 *  - a hdata variable (format: hdata.var1.var2 or hdata[list].var1.var2
 *                      or hdata[ptr].var1.var2 or hdata[ptr_name].var1.var2)
 *
 * See /help in WeeChat for examples.
 *
 * Note: result must be freed after use.
 */

char *
eval_replace_vars_cb (void *data, const char *text)
{
    struct t_eval_context *eval_context;
    struct t_config_option *ptr_option;
    struct t_gui_buffer *ptr_buffer;
    char str_value[512], *value, *tmp;
    const char *ptr_value;
    int length, debug_id;

    value = NULL;

    eval_context = (struct t_eval_context *)data;

    EVAL_DEBUG_MSG(1, "eval_replace_vars_cb(\"%s\")", text);

    /* raw text (no evaluation at all) */
    if (strncmp (text, "raw:", 4) == 0)
    {
        value = strdup (text + 4);
        goto end;
    }

    /* variable in hashtable "user_vars" or "extra_vars" */
    ptr_value = hashtable_get (eval_context->user_vars, text);
    if (ptr_value)
    {
        value = strdup (ptr_value);
        goto end;
    }
    if (eval_context->extra_vars)
    {
        ptr_value = hashtable_get (eval_context->extra_vars, text);
        if (ptr_value)
        {
            if (eval_context->extra_vars_eval)
            {
                tmp = strdup (ptr_value);
                if (!tmp)
                    goto end;
                hashtable_remove (eval_context->extra_vars, text);
                value = eval_replace_vars (tmp, eval_context);
                hashtable_set (eval_context->extra_vars, text, tmp);
                free (tmp);
                goto end;
            }
            else
            {
                value = strdup (ptr_value);
                goto end;
            }
        }
    }

    /* WeeChat home directory */
    if (strcmp (text, "weechat_config_dir") == 0)
    {
        value = strdup (weechat_config_dir);
        goto end;
    }
    if (strcmp (text, "weechat_data_dir") == 0)
    {
        value = strdup (weechat_data_dir);
        goto end;
    }
    if (strcmp (text, "weechat_cache_dir") == 0)
    {
        value = strdup (weechat_cache_dir);
        goto end;
    }
    if (strcmp (text, "weechat_runtime_dir") == 0)
    {
        value = strdup (weechat_runtime_dir);
        goto end;
    }

    /*
     * force evaluation of string (recursive call)
     * --> use with caution: the text must be safe!
     */
    if (strncmp (text, "eval:", 5) == 0)
    {
        value = eval_replace_vars (text + 5, eval_context);
        goto end;
    }

    /*
     * force evaluation of condition (recursive call)
     * --> use with caution: the text must be safe!
     */
    if (strncmp (text, "eval_cond:", 10) == 0)
    {
        value = eval_string_eval_cond (text + 10, eval_context);
        goto end;
    }

    /* convert escaped chars */
    if (strncmp (text, "esc:", 4) == 0)
    {
        value = string_convert_escaped_chars (text + 4);
        goto end;
    }
    if ((text[0] == '\\') && text[1] && (text[1] != '\\'))
    {
        value = string_convert_escaped_chars (text);
        goto end;
    }

    /* range of chars */
    if (strncmp (text, "chars:", 6) == 0)
    {
        value = eval_string_range_chars (text + 6);
        goto end;
    }

    /* convert to lower case */
    if (strncmp (text, "lower:", 6) == 0)
    {
        value = string_tolower (text + 6);
        goto end;
    }

    /* convert to upper case */
    if (strncmp (text, "upper:", 6) == 0)
    {
        value = string_toupper (text + 6);
        goto end;
    }

    /* hide chars: replace all chars by a given char/string */
    if (strncmp (text, "hide:", 5) == 0)
    {
        value = eval_string_hide (text + 5);
        goto end;
    }

    /*
     * cut chars:
     *   cut: max number of chars, and add an optional suffix when the
     *        string is cut
     *   cutscr: max number of chars displayed on screen, and add an optional
     *           suffix when the string is cut
     */
    if (strncmp (text, "cut:", 4) == 0)
    {
        value = eval_string_cut (text + 4, 0);
        goto end;
    }
    if (strncmp (text, "cutscr:", 7) == 0)
    {
        value = eval_string_cut (text + 7, 1);
        goto end;
    }

    /* reverse string */
    if (strncmp (text, "rev:", 4) == 0)
    {
        value = string_reverse (text + 4);
        goto end;
    }
    if (strncmp (text, "revscr:", 7) == 0)
    {
        value = string_reverse_screen (text + 7);
        goto end;
    }

    /* repeated string */
    if (strncmp (text, "repeat:", 7) == 0)
    {
        value = eval_string_repeat (text + 7);
        goto end;
    }

    /*
     * length of string:
     *   length: number of chars
     *   lengthscr: number of chars displayed on screen
     */
    if (strncmp (text, "length:", 7) == 0)
    {
        length = gui_chat_strlen (text + 7);
        snprintf (str_value, sizeof (str_value), "%d", length);
        value = strdup (str_value);
        goto end;
    }
    if (strncmp (text, "lengthscr:", 10) == 0)
    {
        length = gui_chat_strlen_screen (text + 10);
        snprintf (str_value, sizeof (str_value), "%d", length);
        value = strdup (str_value);
        goto end;
    }

    /* split string */
    if (strncmp (text, "split:", 6) == 0)
    {
        value = eval_string_split (text + 6);
        goto end;
    }

    /* split shell */
    if (strncmp (text, "split_shell:", 12) == 0)
    {
        value = eval_string_split_shell (text + 12);
        goto end;
    }

    /* regex group captured */
    if (strncmp (text, "re:", 3) == 0)
    {
        value = eval_string_regex_group (text + 3, eval_context);
        goto end;
    }

    /* color code */
    if (strncmp (text, "color:", 6) == 0)
    {
        value = eval_string_color (text + 6);
        goto end;
    }

    /* modifier */
    if (strncmp (text, "modifier:", 9) == 0)
    {
        value = eval_string_modifier (text + 9);
        goto end;
    }

    /* info */
    if (strncmp (text, "info:", 5) == 0)
    {
        value = eval_string_info (text + 5);
        goto end;
    }

    /* base_encode/base_decode */
    if (strncmp (text, "base_encode:", 12) == 0)
    {
        value = eval_string_base_encode (text + 12);
        goto end;
    }
    if (strncmp (text, "base_decode:", 12) == 0)
    {
        value = eval_string_base_decode (text + 12);
        goto end;
    }

    /* current date/time */
    if ((strncmp (text, "date", 4) == 0) && (!text[4] || (text[4] == ':')))
    {
        value = eval_string_date (text + 4);
        goto end;
    }

    /* environment variable */
    if (strncmp (text, "env:", 4) == 0)
    {
        ptr_value = getenv (text + 4);
        value = strdup ((ptr_value) ? ptr_value : "");
        goto end;
    }

    /* ternary operator: if:condition?value_if_true:value_if_false */
    if (strncmp (text, "if:", 3) == 0)
    {
        value = eval_string_if (text + 3, eval_context);
        goto end;
    }

    /*
     * calculate the result of an expression
     * (with number, operators and parentheses)
     */
    if (strncmp (text, "calc:", 5) == 0)
    {
        value = calc_expression (text + 5);
        goto end;
    }

    /*
     * random number
     */
    if (strncmp (text, "random:", 7) == 0)
    {
        value = eval_string_random (text + 7);
        goto end;
    }

    /*
     * translated text
     */
    if (strncmp (text, "translate:", 10) == 0)
    {
        value = eval_string_translate (text + 10);
        goto end;
    }

    /* define a variable */
    if (strncmp (text, "define:", 7) == 0)
    {
        eval_string_define (text + 7, eval_context);
        value = strdup ("");
        goto end;
    }

    /* option: if found, return this value */
    if (strncmp (text, "sec.data.", 9) == 0)
    {
        ptr_value = hashtable_get (secure_hashtable_data, text + 9);
        value = strdup ((ptr_value) ? ptr_value : "");
        goto end;
    }
    config_file_search_with_string (text, NULL, NULL, &ptr_option, NULL);
    if (ptr_option)
    {
        if (!ptr_option->value)
        {
            value = strdup ("");
            goto end;
        }
        switch (ptr_option->type)
        {
            case CONFIG_OPTION_TYPE_BOOLEAN:
                value = strdup (CONFIG_BOOLEAN(ptr_option) ?
                                EVAL_STR_TRUE : EVAL_STR_FALSE);
                goto end;
            case CONFIG_OPTION_TYPE_INTEGER:
                if (ptr_option->string_values)
                {
                    value = strdup (ptr_option->string_values[CONFIG_INTEGER(ptr_option)]);
                    goto end;
                }
                snprintf (str_value, sizeof (str_value),
                          "%d", CONFIG_INTEGER(ptr_option));
                value = strdup (str_value);
                goto end;
            case CONFIG_OPTION_TYPE_STRING:
                value = strdup (CONFIG_STRING(ptr_option));
                goto end;
            case CONFIG_OPTION_TYPE_COLOR:
                value = strdup (gui_color_get_name (CONFIG_COLOR(ptr_option)));
                goto end;
            case CONFIG_NUM_OPTION_TYPES:
                value = strdup ("");
                goto end;
        }
    }

    /* local variable in buffer */
    ptr_buffer = hashtable_get (eval_context->pointers, "buffer");
    if (ptr_buffer)
    {
        ptr_value = hashtable_get (ptr_buffer->local_variables, text);
        if (ptr_value)
        {
            value = strdup (ptr_value);
            goto end;
        }
    }

    /* hdata */
    value = eval_string_hdata (text, eval_context);

end:
    EVAL_DEBUG_RESULT(1, value);

    return value;
}

/*
 * Replaces variables in a string.
 *
 * Note: result must be freed after use.
 */

char *
eval_replace_vars (const char *expr, struct t_eval_context *eval_context)
{
    const char *no_replace_prefix_list[] = { "if:", "raw:", NULL };
    char *result;
    int debug_id;

    EVAL_DEBUG_MSG(1, "eval_replace_vars(\"%s\")", expr);

    eval_context->recursion_count++;

    if (eval_context->recursion_count < EVAL_RECURSION_MAX)
    {
        result = string_replace_with_callback (expr,
                                               eval_context->prefix,
                                               eval_context->suffix,
                                               no_replace_prefix_list,
                                               &eval_replace_vars_cb,
                                               eval_context,
                                               NULL);
    }
    else
    {
        result = strdup ("");
    }

    eval_context->recursion_count--;

    EVAL_DEBUG_RESULT(1, result);

    return result;
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
eval_compare (const char *expr1, int comparison, const char *expr2,
              struct t_eval_context *eval_context)
{
    int rc, string_compare, length1, length2, debug_id;
    regex_t regex;
    double value1, value2;
    char *error, *value;

    EVAL_DEBUG_MSG(1, "eval_compare(\"%s\", \"%s\", \"%s\")",
                   expr1, eval_comparisons[comparison], expr2);

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
    else if ((comparison == EVAL_COMPARE_STRING_MATCHING_CASE_SENSITIVE)
             || (comparison == EVAL_COMPARE_STRING_NOT_MATCHING_CASE_SENSITIVE))
    {
        rc = string_match (expr1, expr2, 1);
        if (comparison == EVAL_COMPARE_STRING_NOT_MATCHING_CASE_SENSITIVE)
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
    else if ((comparison == EVAL_COMPARE_INCLUDE_CASE_SENSITIVE)
             || (comparison == EVAL_COMPARE_NOT_INCLUDE_CASE_SENSITIVE))
    {
        rc = (strstr (expr1, expr2)) ? 1 : 0;
        if (comparison == EVAL_COMPARE_NOT_INCLUDE_CASE_SENSITIVE)
            rc ^= 1;
        goto end;
    }
    else if ((comparison == EVAL_COMPARE_INCLUDE)
             || (comparison == EVAL_COMPARE_NOT_INCLUDE))
    {
        rc = (string_strcasestr (expr1, expr2)) ? 1 : 0;
        if (comparison == EVAL_COMPARE_NOT_INCLUDE)
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
        value1 = strtod (expr1, &error);
        if (!error || error[0])
        {
            string_compare = 1;
        }
        else
        {
            value2 = strtod (expr2, &error);
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
    value = strdup ((rc) ? EVAL_STR_TRUE : EVAL_STR_FALSE);

    EVAL_DEBUG_RESULT(1, value);

    return value;
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
                           struct t_eval_context *eval_context)
{
    int logic, comp, length, level, rc, debug_id;
    const char *pos, *pos_end;
    char *expr2, *sub_expr, *value, *tmp_value, *tmp_value2;

    EVAL_DEBUG_MSG(1, "eval_expression_condition(\"%s\")", expr);

    value = NULL;
    expr2 = NULL;

    if (!expr)
        goto end;

    if (!expr[0])
    {
        value = strdup (expr);
        goto end;
    }

    /* skip spaces at beginning of string */
    while (expr[0] == ' ')
    {
        expr++;
    }
    if (!expr[0])
    {
        value = strdup (expr);
        goto end;
    }

    /* skip spaces at end of string */
    pos_end = expr + strlen (expr) - 1;
    while ((pos_end > expr) && (pos_end[0] == ' '))
    {
        pos_end--;
    }

    expr2 = string_strndup (expr, pos_end + 1 - expr);
    if (!expr2)
        goto end;

    /*
     * search for a logical operator, and if one is found:
     * - split expression into two sub-expressions
     * - evaluate first sub-expression
     * - if needed, evaluate second sub-expression
     * - return result
     */
    for (logic = 0; logic < EVAL_NUM_LOGICAL_OPS; logic++)
    {
        pos = eval_strstr_level (expr2, eval_logical_ops[logic], eval_context,
                                 "(", ")", 0);
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
            tmp_value = eval_expression_condition (sub_expr, eval_context);
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
            pos += strlen (eval_logical_ops[logic]);
            while (pos[0] == ' ')
            {
                pos++;
            }
            tmp_value = eval_expression_condition (pos, eval_context);
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
        pos = eval_strstr_level (expr2, eval_comparisons[comp], eval_context,
                                 "(", ")", 0);
        if (pos >= expr2)
        {
            if (pos > expr2)
            {
                pos_end = pos - 1;
                while ((pos_end > expr2) && (pos_end[0] == ' '))
                {
                    pos_end--;
                }
                sub_expr = string_strndup (expr2, pos_end + 1 - expr2);
            }
            else
            {
                sub_expr = strdup ("");
            }
            if (!sub_expr)
                goto end;
            pos += strlen (eval_comparisons[comp]);
            while (pos[0] == ' ')
            {
                pos++;
            }
            if ((comp == EVAL_COMPARE_REGEX_MATCHING)
                || (comp == EVAL_COMPARE_REGEX_NOT_MATCHING))
            {
                /* for regex: just replace vars in both expressions */
                tmp_value = eval_replace_vars (sub_expr, eval_context);
                tmp_value2 = eval_replace_vars (pos, eval_context);
            }
            else
            {
                /* other comparison: fully evaluate both expressions */
                tmp_value = eval_expression_condition (sub_expr, eval_context);
                tmp_value2 = eval_expression_condition (pos, eval_context);
            }
            free (sub_expr);
            value = eval_compare (tmp_value, comp, tmp_value2, eval_context);
            if (tmp_value)
                free (tmp_value);
            if (tmp_value2)
                free (tmp_value2);
            goto end;
        }
    }

    /*
     * evaluate sub-expressions between parentheses and replace them with their
     * values
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
        tmp_value = eval_expression_condition (sub_expr, eval_context);
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
    value = eval_replace_vars (expr2, eval_context);

end:
    if (expr2)
        free (expr2);

    EVAL_DEBUG_RESULT(1, value);

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
                    struct t_eval_context *eval_context)
{
    char *result, *result2, *str_replace;
    int length, length_replace, start_offset, i, rc, end, debug_id;
    int empty_replace_allowed;
    struct t_eval_regex eval_regex;

    result = NULL;

    EVAL_DEBUG_MSG(1, "eval_replace_regex(\"%s\", 0x%lx, \"%s\")",
                   string, regex, replace);

    if (!string || !regex || !replace)
        goto end;

    length = strlen (string) + 1;
    result = malloc (length);
    if (!result)
        goto end;
    snprintf (result, length, "%s", string);

    eval_context->regex = &eval_regex;
    eval_context->regex_replacement_index = 1;

    start_offset = 0;

    /* we allow one empty replace if input string is empty */
    empty_replace_allowed = (result[0]) ? 0 : 1;

    while (result)
    {
        for (i = 0; i < 100; i++)
        {
            eval_regex.match[i].rm_so = -1;
        }

        rc = regexec (regex, result + start_offset, 100, eval_regex.match, 0);

        /* no match found: exit the loop */
        if ((rc != 0) || (eval_regex.match[0].rm_so < 0))
            break;

        /*
         * if empty string is matching, continue only if empty replace is
         * still allowed (to prevent infinite loop)
         */
        if (eval_regex.match[0].rm_eo <= 0)
        {
            if (!empty_replace_allowed)
                break;
            empty_replace_allowed = 0;
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

        str_replace = eval_replace_vars (replace, eval_context);

        length_replace = (str_replace) ? strlen (str_replace) : 0;

        length = eval_regex.match[0].rm_so + length_replace +
            strlen (result + eval_regex.match[0].rm_eo) + 1;
        result2 = malloc (length);
        if (!result2)
        {
            free (result);
            result = NULL;
            goto end;
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

        if (!result[start_offset])
            break;

        (eval_context->regex_replacement_index)++;
    }

end:
    EVAL_DEBUG_RESULT(1, result);

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
 *   == [buffer:irc.libera.#weechat]
 *   >> ${window.win_width}
 *   == [112]
 *   >> ${window.win_height}
 *   == [40]
 *
 * Examples of conditions:
 *   >> ${window.buffer.full_name} == irc.libera.#weechat
 *   == [1]
 *   >> ${window.buffer.full_name} == irc.libera.#test
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
    struct t_eval_context context, *eval_context;
    struct t_hashtable *user_vars;
    int condition, rc, pointers_allocated, regex_allocated, debug_id;
    int ptr_window_added, ptr_buffer_added;
    long number;
    char *value, *error;
    const char *default_prefix = EVAL_DEFAULT_PREFIX;
    const char *default_suffix = EVAL_DEFAULT_SUFFIX;
    const char *ptr_value, *regex_replace;
    struct t_gui_window *window;
    regex_t *regex;

    if (!expr)
        return NULL;

    condition = 0;
    user_vars = NULL;
    pointers_allocated = 0;
    regex_allocated = 0;
    regex = NULL;
    regex_replace = NULL;
    ptr_window_added = 0;
    ptr_buffer_added = 0;

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

    user_vars = hashtable_new (32,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL,
                               NULL);

    eval_context = &context;

    eval_context->pointers = pointers;
    eval_context->extra_vars = extra_vars;
    eval_context->user_vars = user_vars;
    eval_context->extra_vars_eval = 0;
    eval_context->prefix = default_prefix;
    eval_context->length_prefix = strlen (eval_context->prefix);
    eval_context->suffix = default_suffix;
    eval_context->length_suffix = strlen (eval_context->suffix);
    eval_context->regex = NULL;
    eval_context->regex_replacement_index = 1;
    eval_context->recursion_count = 0;
    eval_context->debug_level = 0;
    eval_context->debug_depth = 0;
    eval_context->debug_id = 0;
    eval_context->debug_output = NULL;

    /*
     * set window/buffer with pointer to current window/buffer
     * (if not already defined in the hashtable)
     */
    if (gui_current_window)
    {
        if (!hashtable_has_key (pointers, "window"))
        {
            hashtable_set (pointers, "window", gui_current_window);
            ptr_window_added = 1;
        }
        if (!hashtable_has_key (pointers, "buffer"))
        {
            window = (struct t_gui_window *)hashtable_get (pointers, "window");
            if (window)
            {
                hashtable_set (pointers, "buffer", window->buffer);
                ptr_buffer_added = 1;
            }
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
            eval_context->extra_vars_eval = 1;

        /* check for custom prefix */
        ptr_value = hashtable_get (options, "prefix");
        if (ptr_value && ptr_value[0])
        {
            eval_context->prefix = ptr_value;
            eval_context->length_prefix = strlen (eval_context->prefix);
        }

        /* check for custom suffix */
        ptr_value = hashtable_get (options, "suffix");
        if (ptr_value && ptr_value[0])
        {
            eval_context->suffix = ptr_value;
            eval_context->length_suffix = strlen (eval_context->suffix);
        }

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

        /* check for debug */
        ptr_value = hashtable_get (options, "debug");
        if (ptr_value && ptr_value[0])
        {
            number = strtol (ptr_value, &error, 10);
            if (error && !error[0] && (number >= 1))
            {
                eval_context->debug_level = (int)number;
                eval_context->debug_output = string_dyn_alloc (256);
            }
        }
    }

    EVAL_DEBUG_MSG(1, "eval_expression(\"%s\")", expr);

    /* evaluate expression */
    if (condition)
    {
        /* evaluate as condition (return a boolean: "0" or "1") */
        value = eval_expression_condition (expr, eval_context);
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
                                        eval_context);
        }
        else
        {
            /* only replace variables in expression */
            value = eval_replace_vars (expr, eval_context);
        }
    }

    if (pointers_allocated)
    {
        hashtable_free (pointers);
    }
    else
    {
        if (ptr_window_added)
            hashtable_remove (pointers, "window");
        if (ptr_buffer_added)
            hashtable_remove (pointers, "buffer");
    }
    if (user_vars)
        hashtable_free (user_vars);
    if (regex && regex_allocated)
    {
        regfree (regex);
        free (regex);
    }

    EVAL_DEBUG_RESULT(1, value);

    /* set debug in options hashtable */
    if (options && eval_context->debug_output)
        hashtable_set (options, "debug_output", *(eval_context->debug_output));
    if (eval_context->debug_output)
        string_dyn_free (eval_context->debug_output, 1);

    return value;
}
