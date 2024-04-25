/*
 * core-hdata.c - direct access to WeeChat data using hashtables
 *
 * Copyright (C) 2011-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#include "weechat.h"
#include "core-hdata.h"
#include "core-hook.h"
#include "core-eval.h"
#include "core-hashtable.h"
#include "core-log.h"
#include "core-string.h"
#include "../plugins/plugin.h"


struct t_hashtable *weechat_hdata = NULL;

char *hdata_type_string[WEECHAT_NUM_HDATA_TYPES] =
{ "other", "char", "integer", "long", "longlong", "string", "pointer", "time",
  "hashtable", "shared_string" };


/*
 * Frees a hdata variable.
 */

void
hdata_free_var_cb (struct t_hashtable *hashtable, const void *key, void *value)
{
    struct t_hdata_var *var;

    /* make C compiler happy */
    (void) hashtable;
    (void) key;

    var = (struct t_hdata_var *)value;
    if (var)
    {
        free (var->array_size);
        free (var->hdata_name);
        free (var);
    }
}

/*
 * Frees a hdata list.
 */

void
hdata_free_list_cb (struct t_hashtable *hashtable, const void *key, void *value)
{
    /* make C compiler happy */
    (void) hashtable;
    (void) key;

    free (value);
}

/*
 * Creates a new hdata.
 *
 * Returns pointer to new hdata, NULL if error.
 */

struct t_hdata *
hdata_new (struct t_weechat_plugin *plugin, const char *hdata_name,
           const char *var_prev, const char *var_next,
           int create_allowed, int delete_allowed,
           int (*callback_update)(void *data,
                                  struct t_hdata *hdata,
                                  void *pointer,
                                  struct t_hashtable *hashtable),
           void *callback_update_data)
{
    struct t_hdata *new_hdata;

    if (!hdata_name || !hdata_name[0])
        return NULL;

    new_hdata = malloc (sizeof (*new_hdata));
    if (new_hdata)
    {
        new_hdata->name = strdup (hdata_name);
        new_hdata->plugin = plugin;
        new_hdata->var_prev = (var_prev) ? strdup (var_prev) : NULL;
        new_hdata->var_next = (var_next) ? strdup (var_next) : NULL;
        new_hdata->hash_var = hashtable_new (32,
                                             WEECHAT_HASHTABLE_STRING,
                                             WEECHAT_HASHTABLE_POINTER,
                                             NULL,
                                             NULL);
        new_hdata->hash_var->callback_free_value = &hdata_free_var_cb;
        new_hdata->hash_list = hashtable_new (32,
                                              WEECHAT_HASHTABLE_STRING,
                                              WEECHAT_HASHTABLE_POINTER,
                                              NULL,
                                              NULL);
        new_hdata->hash_list->callback_free_value = &hdata_free_list_cb;
        hashtable_set (weechat_hdata, hdata_name, new_hdata);
        new_hdata->create_allowed = create_allowed;
        new_hdata->delete_allowed = delete_allowed;
        new_hdata->callback_update = callback_update;
        new_hdata->callback_update_data = callback_update_data;
        new_hdata->update_pending = 0;
    }

    return new_hdata;
}

/*
 * Adds a new variable in a hdata.
 */

void
hdata_new_var (struct t_hdata *hdata, const char *name, int offset, int type,
               int update_allowed, const char *array_size,
               const char *hdata_name)
{
    struct t_hdata_var *var;
    const char *ptr_array_size;

    if (!hdata || !name)
        return;

    var = malloc (sizeof (*var));
    if (var)
    {
        var->offset = offset;
        var->type = type;
        var->update_allowed = update_allowed;
        ptr_array_size = array_size;
        if (ptr_array_size && (strncmp (ptr_array_size, "*,", 2) == 0))
        {
            var->array_pointer = 1;
            ptr_array_size += 2;
        }
        else
        {
            var->array_pointer = 0;
        }
        var->array_size = (ptr_array_size && ptr_array_size[0]) ?
            strdup (ptr_array_size) : NULL;
        var->hdata_name = (hdata_name && hdata_name[0]) ?
            strdup (hdata_name) : NULL;
        hashtable_set (hdata->hash_var, name, var);
    }
}

/*
 * Adds a new list pointer in a hdata.
 */

void
hdata_new_list (struct t_hdata *hdata, const char *name, void *pointer,
                int flags)
{
    struct t_hdata_list *list;

    if (!hdata || !name)
        return;

    list = malloc (sizeof (*list));
    if (list)
    {
        list->pointer = pointer;
        list->flags = flags;
        hashtable_set (hdata->hash_list, name, list);
    }
}

/*
 * Gets offset of variable in hdata.
 */

int
hdata_get_var_offset (struct t_hdata *hdata, const char *name)
{
    struct t_hdata_var *var;

    if (!hdata || !name)
        return -1;

    var = hashtable_get (hdata->hash_var, name);
    if (var)
        return var->offset;

    return -1;
}

/*
 * Gets type of variable in hdata (as integer).
 */

int
hdata_get_var_type (struct t_hdata *hdata, const char *name)
{
    struct t_hdata_var *var;

    if (!hdata || !name)
        return -1;

    var = hashtable_get (hdata->hash_var, name);
    if (var)
        return var->type;

    return -1;
}

/*
 * Gets type of variable in hdata (as string).
 */

const char *
hdata_get_var_type_string (struct t_hdata *hdata, const char *name)
{
    struct t_hdata_var *var;

    if (!hdata || !name)
        return NULL;

    var = hashtable_get (hdata->hash_var, name);
    if (var)
        return hdata_type_string[(int)var->type];

    return NULL;
}

/*
 * Gets size of array for a variable (if variable is an array).
 *
 * Returns size of array, -1 if variable is not an array (or if error).
 */

int
hdata_get_var_array_size (struct t_hdata *hdata, void *pointer,
                          const char *name)
{
    struct t_hdata_var *var;
    const char *ptr_size;
    char *error;
    long value;
    int i, offset;
    void *ptr_value;

    if (!hdata || !name)
        return -1;

    var = hashtable_get (hdata->hash_var, name);
    if (!var)
        return -1;

    ptr_size = var->array_size;
    if (!ptr_size)
        return -1;

    if (strcmp (ptr_size, "*") == 0)
    {
        /*
         * automatic size: look for NULL in array
         * (this automatic size is possible only with pointers, so with
         * types: string, pointer, hashtable)
         */
        if ((var->type == WEECHAT_HDATA_STRING)
            || (var->type == WEECHAT_HDATA_SHARED_STRING)
            || (var->type == WEECHAT_HDATA_POINTER)
            || (var->type == WEECHAT_HDATA_HASHTABLE))
        {
            if (!(*((void **)(pointer + var->offset))))
                return 0;
            i = 0;
            while (1)
            {
                ptr_value = NULL;
                switch (var->type)
                {
                    case WEECHAT_HDATA_STRING:
                    case WEECHAT_HDATA_SHARED_STRING:
                        ptr_value = (*((char ***)(pointer + var->offset)))[i];
                        break;
                    case WEECHAT_HDATA_POINTER:
                        ptr_value = (*((void ***)(pointer + var->offset)))[i];
                        break;
                    case WEECHAT_HDATA_HASHTABLE:
                        ptr_value = (*((struct t_hashtable ***)(pointer + var->offset)))[i];
                        break;
                }
                if (!ptr_value)
                    break;
                i++;
            }
            return i;
        }
    }
    else
    {
        /* fixed size: the size can be a name of variable or integer */
        offset = hdata_get_var_offset (hdata, ptr_size);
        if (offset >= 0)
        {
            /* size is the name of a variable in hdata, read it */
            switch (hdata_get_var_type (hdata, ptr_size))
            {
                case WEECHAT_HDATA_CHAR:
                    return (int)(*((char *)(pointer + offset)));
                case WEECHAT_HDATA_INTEGER:
                    return *((int *)(pointer + offset));
                case WEECHAT_HDATA_LONG:
                    return (int)(*((long *)(pointer + offset)));
                case WEECHAT_HDATA_LONGLONG:
                    return (int)(*((long long *)(pointer + offset)));
                default:
                    break;
            }
        }
        else
        {
            /* check if the size is a valid integer */
            error = NULL;
            value = strtol (ptr_size, &error, 10);
            if (error && !error[0])
                return (int)value;
        }
    }

    return -1;
}

/*
 * Gets size of array for variable as string.
 */

const char *
hdata_get_var_array_size_string (struct t_hdata *hdata, void *pointer,
                                 const char *name)
{
    struct t_hdata_var *var;

    /* make C compiler happy */
    (void) pointer;

    if (!hdata || !name)
        return NULL;

    var = hashtable_get (hdata->hash_var, name);
    if (var)
        return (const char *)var->array_size;

    return NULL;
}

/*
 * Gets hdata name for a variable.
 *
 * Returns hdata name, NULL if variable has no hdata.
 */

const char *
hdata_get_var_hdata (struct t_hdata *hdata, const char *name)
{
    struct t_hdata_var *var;

    if (!hdata || !name)
        return NULL;

    var = hashtable_get (hdata->hash_var, name);
    if (var)
        return (const char *)var->hdata_name;

    return NULL;
}

/*
 * Gets pointer to content of variable using hdata variable name.
 */

void *
hdata_get_var (struct t_hdata *hdata, void *pointer, const char *name)
{
    int offset;

    if (!hdata || !pointer || !name)
        return NULL;

    offset = hdata_get_var_offset (hdata, name);
    if (offset >= 0)
        return pointer + offset;

    return NULL;
}

/*
 * Gets pointer to content of variable using hdata variable offset.
 */

void *
hdata_get_var_at_offset (struct t_hdata *hdata, void *pointer, int offset)
{
    if (!hdata || !pointer)
        return NULL;

    return pointer + offset;
}

/*
 * Gets a list pointer in hdata.
 */

void *
hdata_get_list (struct t_hdata *hdata, const char *name)
{
    struct t_hdata_list *ptr_list;

    if (!hdata || !name)
        return NULL;

    ptr_list = hashtable_get (hdata->hash_list, name);
    if (ptr_list)
        return *((void **)(ptr_list->pointer));

    return NULL;
}

/*
 * Checks if a pointer is in the list.
 *
 * Returns:
 *   1: pointer exists in list
 *   0: pointer does not exist
 */

int
hdata_check_pointer_in_list (struct t_hdata *hdata, void *list, void *pointer)
{
    void *ptr_current;

    if (!hdata || !pointer)
        return 0;

    if (pointer == list)
        return 1;

    ptr_current = list;
    while (ptr_current)
    {
        ptr_current = hdata_move (hdata, ptr_current, 1);
        if (ptr_current && (ptr_current == pointer))
            return 1;
    }

    return 0;
}

/*
 * Checks if a pointer is in a list with flag "check_pointers".
 */

void
hdata_check_pointer_map_cb (void *data, struct t_hashtable *hashtable,
                            const void *key, const void *value)
{
    void **pointers, *pointer, **num_lists, **found;
    struct t_hdata *ptr_hdata;
    struct t_hdata_list *ptr_list;

    /* make C compiler happy */
    (void) hashtable;
    (void) key;

    pointers = (void **)data;
    ptr_hdata = pointers[0];
    pointer = pointers[1];
    num_lists = &pointers[2];
    found = &pointers[3];

    /* pointer already found in another list? just exit */
    if (*found)
        return;

    ptr_list = (struct t_hdata_list *)value;
    if (!ptr_list || !(ptr_list->flags & WEECHAT_HDATA_LIST_CHECK_POINTERS))
        return;

    *found = (void *)((unsigned long)hdata_check_pointer_in_list (
                          ptr_hdata,
                          *((void **)(ptr_list->pointer)),
                          pointer));
    (*num_lists)++;
}

/*
 * Checks if a pointer is valid for a given hdata/list.
 *
 * If argument "list" is NULL, the check is made with all lists in hdata
 * that have flag "check_pointers". If no list is defined with this flag,
 * the pointer is considered valid (so this function returns 1); if the
 * pointer is not found in any list, this function returns 0.
 *
 * Returns:
 *   1: pointer exists in the given list (or a list with check_pointers flag)
 *   0: pointer does not exist
 */

int
hdata_check_pointer (struct t_hdata *hdata, void *list, void *pointer)
{
    void *pointers[4];

    if (!hdata || !pointer)
        return 0;

    if (list)
    {
        /* search pointer in the given list */
        return hdata_check_pointer_in_list (hdata, list, pointer);
    }
    else
    {
        /* search pointer in all lists with flag "check_pointers" */
        pointers[0] = hdata;
        pointers[1] = pointer;
        pointers[2] = 0;        /* number of lists with flag check_pointers */
        pointers[3] = 0;        /* pointer found? (0/1) */
        hashtable_map (hdata->hash_list,
                       &hdata_check_pointer_map_cb,
                       pointers);
        return ((pointers[2] == 0) || pointers[3]) ? 1 : 0;
    }
}

/*
 * Moves pointer to another element in list.
 */

void *
hdata_move (struct t_hdata *hdata, void *pointer, int count)
{
    char *ptr_var;
    int i, abs_count;

    if (!hdata || !pointer || (count == 0))
        return NULL;

    ptr_var = (count < 0) ? hdata->var_prev : hdata->var_next;
    abs_count = abs (count);

    for (i = 0; i < abs_count; i++)
    {
        pointer = hdata_pointer (hdata, pointer, ptr_var);
        if (!pointer)
            break;
    }

    return pointer;
}

/*
 * Searches for an element in list using expression.
 *
 * Returns pointer to element found, NULL if not found.
 */

void *
hdata_search (struct t_hdata *hdata,
              void *pointer,
              const char *search,
              struct t_hashtable *pointers,
              struct t_hashtable *extra_vars,
              struct t_hashtable *options,
              int move)
{
    struct t_hashtable *pointers2, *options2;
    char *result;
    void *ret_pointer;
    int rc;

    if (!hdata || !pointer || !search || !search[0] || (move == 0))
        return NULL;

    ret_pointer = NULL;

    /* duplicate or create hashtable with pointers */
    if (pointers)
    {
        pointers2 = hashtable_dup (pointers);
    }
    else
    {
        pointers2 = hashtable_new (32,
                                   WEECHAT_HASHTABLE_STRING,
                                   WEECHAT_HASHTABLE_POINTER,
                                   NULL,
                                   NULL);
    }

    /* duplicate or create hashtable with options */
    if (options)
    {
        options2 = hashtable_dup (options);
    }
    else
    {
        options2 = hashtable_new (32,
                                  WEECHAT_HASHTABLE_STRING,
                                  WEECHAT_HASHTABLE_POINTER,
                                  NULL,
                                  NULL);
    }
    if (options2)
        hashtable_set (options2, "type", "condition");

    while (pointer)
    {
        /* set pointer in hashtable (used for evaluating expression) */
        if (pointers2)
            hashtable_set (pointers2, hdata->name, pointer);

        /* evaluate expression */
        result = eval_expression (search, pointers2, extra_vars, options2);
        rc = eval_is_true (result);
        free (result);
        if (rc)
        {
            ret_pointer = pointer;
            goto end;
        }

        pointer = hdata_move (hdata, pointer, move);
    }

end:
    hashtable_free (pointers2);
    hashtable_free (options2);
    return ret_pointer;
}

/*
 * Extracts index from name of a variable.
 *
 * A name can contain index with this format: "NNN|name" (where NNN is an
 * integer >= 0).
 * Argument "index" is set to N, "ptr_name" points to start of name in string
 * (after the "|" if found).
 */

void
hdata_get_index_and_name (const char *name, int *index, const char **ptr_name)
{
    char *pos, *str_index, *error;
    long number;

    if (index)
        *index = -1;
    if (ptr_name)
        *ptr_name = name;

    if (!name)
        return;

    pos = strchr (name, '|');
    if (pos)
    {
        str_index = string_strndup (name, pos - name);
        if (str_index)
        {
            error = NULL;
            number = strtol (str_index, &error, 10);
            if (error && !error[0])
            {
                if (index)
                    *index = number;
                if (ptr_name)
                    *ptr_name = pos + 1;
            }
            free (str_index);
        }
    }
}

/*
 * Gets char value of a variable in hdata.
 */

char
hdata_char (struct t_hdata *hdata, void *pointer, const char *name)
{
    int index;
    const char *ptr_name;
    struct t_hdata_var *var;

    if (!hdata || !pointer || !name)
        return '\0';

    hdata_get_index_and_name (name, &index, &ptr_name);
    var = hashtable_get (hdata->hash_var, ptr_name);
    if (var && (var->offset >= 0))
    {
        if (var->array_size && (index >= 0))
        {
            if (var->array_pointer)
                return (*((char **)(pointer + var->offset)))[index];
            else
                return ((char *)(pointer + var->offset))[index];
        }
        else
        {
            return *((char *)(pointer + var->offset));
        }
    }

    return '\0';
}

/*
 * Gets integer value of a variable in hdata.
 */

int
hdata_integer (struct t_hdata *hdata, void *pointer, const char *name)
{
    int index;
    const char *ptr_name;
    struct t_hdata_var *var;

    if (!hdata || !pointer || !name)
        return 0;

    hdata_get_index_and_name (name, &index, &ptr_name);
    var = hashtable_get (hdata->hash_var, ptr_name);
    if (var && (var->offset >= 0))
    {
        if (var->array_size && (index >= 0))
        {
            if (var->array_pointer)
                return (*((int **)(pointer + var->offset)))[index];
            else
                return ((int *)(pointer + var->offset))[index];
        }
        else
        {
            return *((int *)(pointer + var->offset));
        }
    }

    return 0;
}

/*
 * Gets long value of a variable in hdata.
 */

long
hdata_long (struct t_hdata *hdata, void *pointer, const char *name)
{
    int index;
    const char *ptr_name;
    struct t_hdata_var *var;

    if (!hdata || !pointer || !name)
        return 0;

    hdata_get_index_and_name (name, &index, &ptr_name);
    var = hashtable_get (hdata->hash_var, ptr_name);
    if (var && (var->offset >= 0))
    {
        if (var->array_size && (index >= 0))
        {
            if (var->array_pointer)
                return (*((long **)(pointer + var->offset)))[index];
            else
                return ((long *)(pointer + var->offset))[index];
        }
        else
        {
            return *((long *)(pointer + var->offset));
        }
    }

    return 0L;
}

/*
 * Gets "long long" value of a variable in hdata.
 */

long long
hdata_longlong (struct t_hdata *hdata, void *pointer, const char *name)
{
    int index;
    const char *ptr_name;
    struct t_hdata_var *var;

    if (!hdata || !pointer || !name)
        return 0;

    hdata_get_index_and_name (name, &index, &ptr_name);
    var = hashtable_get (hdata->hash_var, ptr_name);
    if (var && (var->offset >= 0))
    {
        if (var->array_size && (index >= 0))
        {
            if (var->array_pointer)
                return (*((long long **)(pointer + var->offset)))[index];
            else
                return ((long long *)(pointer + var->offset))[index];
        }
        else
        {
            return *((long long *)(pointer + var->offset));
        }
    }

    return 0LL;
}

/*
 * Gets string value of a variable in hdata.
 */

const char *
hdata_string (struct t_hdata *hdata, void *pointer, const char *name)
{
    int index;
    const char *ptr_name;
    struct t_hdata_var *var;

    if (!hdata || !pointer || !name)
        return NULL;

    hdata_get_index_and_name (name, &index, &ptr_name);
    var = hashtable_get (hdata->hash_var, ptr_name);
    if (var && (var->offset >= 0))
    {
        if (var->array_size && (index >= 0))
        {
            if (var->array_pointer)
                return (*((char ***)(pointer + var->offset)))[index];
            else
            {
                /* we can not index a static array of strings */
                return NULL;
            }
        }
        else
        {
            return *((char **)(pointer + var->offset));
        }
    }

    return NULL;
}

/*
 * Gets pointer value of a variable in hdata.
 */

void *
hdata_pointer (struct t_hdata *hdata, void *pointer, const char *name)
{
    int index;
    const char *ptr_name;
    struct t_hdata_var *var;

    if (!hdata || !pointer || !name)
        return NULL;

    hdata_get_index_and_name (name, &index, &ptr_name);
    var = hashtable_get (hdata->hash_var, ptr_name);
    if (var && (var->offset >= 0))
    {
        if (var->array_size && (index >= 0))
        {
            if (var->array_pointer)
                return (*((void ***)(pointer + var->offset)))[index];
            else
                return ((void **)(pointer + var->offset))[index];
        }
        else
        {
            return *((void **)(pointer + var->offset));
        }
    }

    return NULL;
}

/*
 * Gets time value of a variable in hdata.
 */

time_t
hdata_time (struct t_hdata *hdata, void *pointer, const char *name)
{
    int index;
    const char *ptr_name;
    struct t_hdata_var *var;

    if (!hdata || !pointer || !name)
        return 0;

    hdata_get_index_and_name (name, &index, &ptr_name);
    var = hashtable_get (hdata->hash_var, ptr_name);
    if (var && (var->offset >= 0))
    {
        if (var->array_size && (index >= 0))
        {
            if (var->array_pointer)
                return (*((time_t **)(pointer + var->offset)))[index];
            else
                return ((time_t *)(pointer + var->offset))[index];
        }
        else
        {
            return *((time_t *)(pointer + var->offset));
        }
    }

    return (time_t)0;
}

/*
 * Gets hashtable value of a variable in hdata.
 */

struct t_hashtable *
hdata_hashtable (struct t_hdata *hdata, void *pointer, const char *name)
{
    int index;
    const char *ptr_name;
    struct t_hdata_var *var;

    if (!hdata || !pointer || !name)
        return NULL;

    hdata_get_index_and_name (name, &index, &ptr_name);
    var = hashtable_get (hdata->hash_var, ptr_name);
    if (var && (var->offset >= 0))
    {
        if (var->array_size && (index >= 0))
        {
            if (var->array_pointer)
                return (*((struct t_hashtable ***)(pointer + var->offset)))[index];
            else
                return ((struct t_hashtable **)(pointer + var->offset))[index];
        }
        else
        {
            return *((struct t_hashtable **)(pointer + var->offset));
        }
    }

    return NULL;
}

/*
 * Compares a hdata variable of two objects.
 *
 * If case_sensitive == 1, the comparison of strings is case sensitive.
 *
 * Returns:
 *   -1: variable1 < variable2
 *    0: variable1 == variable2
 *    1: variable1 > variable2
 */

int
hdata_compare (struct t_hdata *hdata, void *pointer1, void *pointer2,
               const char *name, int case_sensitive)
{
    int rc, type, type1, type2, int_value1, int_value2;
    long long_value1, long_value2;
    long long longlong_value1, longlong_value2;
    char *var_name, *property, char_value1, char_value2;
    const char *ptr_var_name, *pos, *pos_open_paren, *hdata_name;
    const char *str_value1, *str_value2;
    void *ptr_value1, *ptr_value2;
    time_t time_value1, time_value2;
    struct t_hashtable *hashtable1, *hashtable2;

    if (!hdata || !name)
        return 0;

    if (!pointer1 && pointer2)
        return -1;
    if (pointer1 && !pointer2)
        return 1;
    if (!pointer1 && !pointer2)
        return 0;

    rc = 0;
    ptr_value1 = NULL;
    ptr_value2 = NULL;

    pos = strchr (name, '.');
    if (pos > name)
        var_name = string_strndup (name, pos - name);
    else
        var_name = strdup (name);

    if (!var_name)
        goto end;

    hdata_get_index_and_name (var_name, NULL, &ptr_var_name);
    type = hdata_get_var_type (hdata, ptr_var_name);
    if (type < 0)
        goto end;

    switch (type)
    {
        case WEECHAT_HDATA_CHAR:
            char_value1 = hdata_char (hdata, pointer1, var_name);
            char_value2 = hdata_char (hdata, pointer2, var_name);
            rc = (char_value1 < char_value2) ?
                -1 : ((char_value1 > char_value2) ? 1 : 0);
            break;
        case WEECHAT_HDATA_INTEGER:
            int_value1 = hdata_integer (hdata, pointer1, var_name);
            int_value2 = hdata_integer (hdata, pointer2, var_name);
            rc = (int_value1 < int_value2) ?
                -1 : ((int_value1 > int_value2) ? 1 : 0);
            break;
        case WEECHAT_HDATA_LONG:
            long_value1 = hdata_long (hdata, pointer1, var_name);
            long_value2 = hdata_long (hdata, pointer2, var_name);
            rc = (long_value1 < long_value2) ?
                -1 : ((long_value1 > long_value2) ? 1 : 0);
            break;
        case WEECHAT_HDATA_LONGLONG:
            longlong_value1 = hdata_longlong (hdata, pointer1, var_name);
            longlong_value2 = hdata_longlong (hdata, pointer2, var_name);
            rc = (longlong_value1 < longlong_value2) ?
                -1 : ((longlong_value1 > longlong_value2) ? 1 : 0);
            break;
        case WEECHAT_HDATA_STRING:
        case WEECHAT_HDATA_SHARED_STRING:
            str_value1 = hdata_string (hdata, pointer1, var_name);
            str_value2 = hdata_string (hdata, pointer2, var_name);
            if (!str_value1 && !str_value2)
                rc = 0;
            else if (str_value1 && !str_value2)
                rc = 1;
            else if (!str_value1 && str_value2)
                rc = -1;
            else
            {
                if (case_sensitive)
                    rc = strcmp (str_value1, str_value2);
                else
                    rc = string_strcasecmp (str_value1, str_value2);
                if (rc < 0)
                    rc = -1;
                else if (rc > 0)
                    rc = 1;
            }
            break;
        case WEECHAT_HDATA_POINTER:
            ptr_value1 = hdata_pointer (hdata, pointer1, var_name);
            ptr_value2 = hdata_pointer (hdata, pointer2, var_name);
            rc = (ptr_value1 < ptr_value2) ?
                -1 : ((ptr_value1 > ptr_value2) ? 1 : 0);
            break;
        case WEECHAT_HDATA_TIME:
            time_value1 = hdata_time (hdata, pointer1, var_name);
            time_value2 = hdata_time (hdata, pointer2, var_name);
            rc = (time_value1 < time_value2) ?
                -1 : ((time_value1 > time_value2) ? 1 : 0);
            break;
        case WEECHAT_HDATA_HASHTABLE:
            ptr_value1 = hdata_hashtable (hdata, pointer1, var_name);
            ptr_value2 = hdata_hashtable (hdata, pointer2, var_name);
            if (pos)
            {
                /*
                 * for a hashtable, if there is a "." after name of hdata:
                 * 1) If "()" is at the end, it is a function call to
                 *    hashtable_get_string().
                 * 2) Otherwise, get the value for this key in hashtable.
                 */
                hashtable1 = ptr_value1;
                hashtable2 = ptr_value2;

                pos_open_paren = strchr (pos, '(');
                if (pos_open_paren
                    && (pos_open_paren > pos + 1)
                    && (pos_open_paren[1] == ')'))
                {
                    property = string_strndup (pos + 1,
                                               pos_open_paren - pos - 1);
                    ptr_value1 = (void *)hashtable_get_string (hashtable1, property);
                    ptr_value2 = (void *)hashtable_get_string (hashtable2, property);
                    free (property);
                    type1 = HASHTABLE_STRING;
                    type2 = HASHTABLE_STRING;
                }
                else
                {
                    ptr_value1 = hashtable_get (hashtable1, pos + 1);
                    ptr_value2 = hashtable_get (hashtable2, pos + 1);
                    type1 = hashtable1->type_values;
                    type2 = hashtable2->type_values;
                }

                if (!ptr_value1 && ptr_value2)
                    rc = -1;
                else if (ptr_value1 && !ptr_value2)
                    rc = 1;
                else if (!ptr_value1 && !ptr_value2)
                    rc = 0;
                else if (type1 != type2)
                    rc = 0;
                else
                {
                    switch (type1)
                    {
                        case HASHTABLE_INTEGER:
                            int_value1 = *((int *)ptr_value1);
                            int_value2 = *((int *)ptr_value2);
                            rc = (int_value1 < int_value2) ?
                                -1 : ((int_value1 > int_value2) ? 1 : 0);
                            break;
                        case HASHTABLE_STRING:
                            if (case_sensitive)
                                rc = strcmp ((const char *)ptr_value1,
                                             (const char *)ptr_value2);
                            else
                                rc = string_strcasecmp ((const char *)ptr_value1,
                                                        (const char *)ptr_value2);
                            if (rc < 0)
                                rc = -1;
                            else if (rc > 0)
                                rc = 1;
                            break;
                        case HASHTABLE_POINTER:
                        case HASHTABLE_BUFFER:
                            rc = (ptr_value1 < ptr_value2) ?
                                -1 : ((ptr_value1 > ptr_value2) ? 1 : 0);
                            break;
                        case HASHTABLE_TIME:
                            time_value1 = (long long)(*((time_t *)ptr_value1));
                            time_value2 = (long long)(*((time_t *)ptr_value2));
                            rc = (time_value1 < time_value2) ?
                                -1 : ((time_value1 > time_value2) ? 1 : 0);
                            break;
                        case HASHTABLE_NUM_TYPES:
                            break;
                    }
                }
            }
            else
            {
                /* compare hashtables by pointer */
                rc = (ptr_value1 < ptr_value2) ?
                    -1 : ((ptr_value1 > ptr_value2) ? 1 : 0);
            }
            break;
        case WEECHAT_HDATA_OTHER:
            /* no comparison for other types */
            rc = 0;
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
        rc = hdata_compare (hdata, ptr_value1, ptr_value2, pos + 1,
                            case_sensitive);
    }

end:
    free (var_name);
    return rc;
}

/*
 * Sets value for a variable in hdata.
 *
 * WARNING: this is dangerous, and only some variables can be set by this
 * function (this depends on hdata, see API doc for more info) and this
 * function can be called *ONLY* in an "update" callback (in hdata).
 *
 * Returns:
 *   1: OK (value set)
 *   0: error (or not allowed)
 */

int
hdata_set (struct t_hdata *hdata, void *pointer, const char *name,
           const char *value)
{
    struct t_hdata_var *var;
    char **ptr_string, *error;
    long number;
    long long number_longlong;
    unsigned long ptr;
    int rc;

    if (!hdata->update_pending)
        return 0;

    var = hashtable_get (hdata->hash_var, name);
    if (!var)
        return 0;

    if (!var->update_allowed)
        return 0;

    switch (var->type)
    {
        case WEECHAT_HDATA_OTHER:
            break;
        case WEECHAT_HDATA_CHAR:
            *((char *)(pointer + var->offset)) = (value) ? value[0] : '\0';
            return 1;
            break;
        case WEECHAT_HDATA_INTEGER:
            error = NULL;
            number = strtol (value, &error, 10);
            if (error && !error[0])
            {
                *((int *)(pointer + var->offset)) = (int)number;
                return 1;
            }
            break;
        case WEECHAT_HDATA_LONG:
            error = NULL;
            number = strtol (value, &error, 10);
            if (error && !error[0])
            {
                *((long *)(pointer + var->offset)) = number;
                return 1;
            }
            break;
        case WEECHAT_HDATA_LONGLONG:
            error = NULL;
            number_longlong = strtoll (value, &error, 10);
            if (error && !error[0])
            {
                *((long long *)(pointer + var->offset)) = number_longlong;
                return 1;
            }
            break;
        case WEECHAT_HDATA_STRING:
            ptr_string = (char **)(pointer + var->offset);
            free (*ptr_string);
            *ptr_string = (value) ? strdup (value) : NULL;
            return 1;
            break;
        case WEECHAT_HDATA_SHARED_STRING:
            ptr_string = (char **)(pointer + var->offset);
            string_shared_free (*ptr_string);
            *ptr_string = (value) ? (char *)string_shared_get (value) : NULL;
            return 1;
            break;
        case WEECHAT_HDATA_POINTER:
            if (value)
            {
                rc = sscanf (value, "%lx", &ptr);
                if ((rc != EOF) && (rc != 0))
                {
                    *((void **)(pointer + var->offset)) = (void *)ptr;
                    return 1;
                }
            }
            else
            {
                *((void **)(pointer + var->offset)) = NULL;
                return 1;
            }
            break;
        case WEECHAT_HDATA_TIME:
            error = NULL;
            number = strtol (value, &error, 10);
            if (error && !error[0] && (number >= 0))
            {
                *((time_t *)(pointer + var->offset)) = (time_t)number;
                return 1;
            }
            break;
        case WEECHAT_HDATA_HASHTABLE:
            break;
    }
    return 0;
}

/*
 * Updates some data in hdata.
 *
 * The hashtable contains keys with new values.
 * A special key "__delete" can be used to delete the whole structure at
 * pointer (if allowed for this hdata).
 *
 * WARNING: this is dangerous, and only some data can be updated by this
 * function (this depends on hdata, see API doc for more info).
 *
 * Returns number of variables updated, 0 if nothing has been updated.
 * In case of deletion, returns 1 if OK, 0 if error.
 */

int
hdata_update (struct t_hdata *hdata, void *pointer,
              struct t_hashtable *hashtable)
{
    const char *value;
    struct t_hdata_var *var;
    int rc;

    if (!hdata || !hashtable || !hdata->callback_update)
        return 0;

    /* check if create of structure is allowed */
    if (hashtable_has_key (hashtable, "__create_allowed"))
        return (int)hdata->create_allowed;

    /* check if delete of structure is allowed */
    if (hashtable_has_key (hashtable, "__delete_allowed"))
        return (int)hdata->delete_allowed;

    /* check if update of variable is allowed */
    value = hashtable_get (hashtable, "__update_allowed");
    if (value)
    {
        var = hashtable_get (hdata->hash_var, value);
        if (!var)
            return 0;
        return (var->update_allowed) ? 1 : 0;
    }

    /* update data */
    hdata->update_pending = 1;
    rc = (hdata->callback_update) (hdata->callback_update_data,
                                   hdata, pointer, hashtable);
    hdata->update_pending = 0;

    return rc;
}

/*
 * Gets a hdata property as string.
 */

const char *
hdata_get_string (struct t_hdata *hdata, const char *property)
{
    if (!hdata || !property)
        return NULL;

    if (strcmp (property, "var_keys") == 0)
        return hashtable_get_string (hdata->hash_var, "keys");
    else if (strcmp (property, "var_values") == 0)
        return hashtable_get_string (hdata->hash_var, "values");
    else if (strcmp (property, "var_keys_values") == 0)
        return hashtable_get_string (hdata->hash_var, "keys_values");
    else if (strcmp (property, "var_prev") == 0)
        return hdata->var_prev;
    else if (strcmp (property, "var_next") == 0)
        return hdata->var_next;
    else if (strcmp (property, "list_keys") == 0)
        return hashtable_get_string (hdata->hash_list, "keys");
    else if (strcmp (property, "list_values") == 0)
        return hashtable_get_string (hdata->hash_list, "values");
    else if (strcmp (property, "list_keys_values") == 0)
        return hashtable_get_string (hdata->hash_list, "keys_values");

    return NULL;
}

/*
 * Frees a hdata.
 */

void
hdata_free (struct t_hdata *hdata)
{
    if (!hdata)
        return;

    hashtable_free (hdata->hash_var);
    free (hdata->var_prev);
    free (hdata->var_next);
    hashtable_free (hdata->hash_list);
    free (hdata->name);

    free (hdata);
}

/*
 * Frees hdata for a plugin (callback called for each hdata in memory).
 */

void
hdata_free_all_plugin_map_cb (void *data, struct t_hashtable *hashtable,
                              const void *key, const void *value)
{
    struct t_hdata *ptr_hdata;

    ptr_hdata = (struct t_hdata *)value;

    if (ptr_hdata->plugin == (struct t_weechat_plugin *)data)
        hashtable_remove (hashtable, key);
}

/*
 * Frees all hdata created by a plugin.
 */

void
hdata_free_all_plugin (struct t_weechat_plugin *plugin)
{
    hashtable_map (weechat_hdata, &hdata_free_all_plugin_map_cb, plugin);
}

/*
 * Frees all hdata.
 */

void
hdata_free_all ()
{
    hashtable_remove_all (weechat_hdata);
}

/*
 * Prints variable of a hdata in WeeChat log file (callback called for each
 * variable in hdata).
 */

void
hdata_print_log_var_map_cb (void *data, struct t_hashtable *hashtable,
                            const void *key, const void *value)
{
    struct t_hdata_var *var;

    /* make C compiler happy */
    (void) data;
    (void) hashtable;

    var = (struct t_hdata_var *)value;

    log_printf ("");
    log_printf ("  [hdata var '%s']", (const char *)key);
    log_printf ("    offset . . . . . . . . : %d", var->offset);
    log_printf ("    type . . . . . . . . . : %d ('%s')", var->type, hdata_type_string[(int)var->type]);
    log_printf ("    update_allowed . . . . : %d", (int)var->update_allowed);
    log_printf ("    array_size . . . . . . : '%s'", var->array_size);
    log_printf ("    hdata_name . . . . . . : '%s'", var->hdata_name);
}

/*
 * Prints hdata in WeeChat log file (callback called for each hdata in memory).
 */

void
hdata_print_log_map_cb (void *data, struct t_hashtable *hashtable,
                        const void *key, const void *value)
{
    struct t_hdata *ptr_hdata;

    /* make C compiler happy */
    (void) data;
    (void) hashtable;
    (void) key;

    ptr_hdata = (struct t_hdata *)value;

    log_printf ("");
    log_printf ("[hdata (addr:%p)]", ptr_hdata);
    log_printf ("  name . . . . . . . . . : '%s'", ptr_hdata->name);
    log_printf ("  plugin . . . . . . . . : %p", ptr_hdata->plugin);
    log_printf ("  var_prev . . . . . . . : '%s'", ptr_hdata->var_prev);
    log_printf ("  var_next . . . . . . . : '%s'", ptr_hdata->var_next);
    log_printf ("  hash_var . . . . . . . : %p (hashtable: '%s')",
                ptr_hdata->hash_var,
                hashtable_get_string (ptr_hdata->hash_var, "keys_values"));
    log_printf ("  hash_list. . . . . . . : %p (hashtable: '%s')",
                ptr_hdata->hash_list,
                hashtable_get_string (ptr_hdata->hash_list, "keys_values"));
    log_printf ("  create_allowed . . . . : %d", (int)ptr_hdata->create_allowed);
    log_printf ("  delete_allowed . . . . : %d", (int)ptr_hdata->delete_allowed);
    log_printf ("  callback_update. . . . : %p", ptr_hdata->callback_update);
    log_printf ("  callback_update_data . : %p", ptr_hdata->callback_update_data);
    log_printf ("  update_pending . . . . : %d", (int)ptr_hdata->update_pending);
    hashtable_map (ptr_hdata->hash_var, &hdata_print_log_var_map_cb, NULL);
}

/*
 * Prints hdata in WeeChat log file (usually for crash dump).
 */

void
hdata_print_log ()
{
    hashtable_map (weechat_hdata, &hdata_print_log_map_cb, NULL);
}

/*
 * Frees a hdata in hashtable "weechat_hdata".
 */

void
hdata_free_hdata_cb (struct t_hashtable *hashtable,
                     const void *key, void *value)
{
    /* make C compiler happy */
    (void) hashtable;
    (void) key;

    hdata_free (value);
}

/*
 * Initializes hdata: creates a hashtable with hdata.
 */

void
hdata_init ()
{
    weechat_hdata = hashtable_new (32,
                                   WEECHAT_HASHTABLE_STRING,
                                   WEECHAT_HASHTABLE_POINTER,
                                   NULL,
                                   NULL);
    weechat_hdata->callback_free_value = &hdata_free_hdata_cb;
}

/*
 * Frees all hdata and hashtable with hdata.
 */

void
hdata_end ()
{
    hdata_free_all ();
    hashtable_free (weechat_hdata);
    weechat_hdata = NULL;
}
