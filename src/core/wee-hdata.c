/*
 * Copyright (C) 2011-2012 Sebastien Helleu <flashcode@flashtux.org>
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

/*
 * wee-hdata.c: direct access to WeeChat data using hashtables
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "weechat.h"
#include "wee-hdata.h"
#include "wee-hashtable.h"
#include "wee-log.h"
#include "wee-string.h"
#include "../plugins/plugin.h"


struct t_hashtable *weechat_hdata = NULL;

char *hdata_type_string[8] =
{ "other", "char", "integer", "long", "string", "pointer", "time",
  "hashtable" };


/*
 * hdata_new: create a new hdata
 */

struct t_hdata *
hdata_new (struct t_weechat_plugin *plugin, const char *hdata_name,
           const char *var_prev, const char *var_next)
{
    struct t_hdata *new_hdata;

    if (!hdata_name || !hdata_name[0])
        return NULL;

    new_hdata = malloc (sizeof (*new_hdata));
    if (new_hdata)
    {
        new_hdata->plugin = plugin;
        new_hdata->var_prev = (var_prev) ? strdup (var_prev) : NULL;
        new_hdata->var_next = (var_next) ? strdup (var_next) : NULL;
        new_hdata->hash_var = hashtable_new (8,
                                             WEECHAT_HASHTABLE_STRING,
                                             WEECHAT_HASHTABLE_INTEGER,
                                             NULL,
                                             NULL);
        new_hdata->hash_var_array_size = hashtable_new (8,
                                                        WEECHAT_HASHTABLE_STRING,
                                                        WEECHAT_HASHTABLE_STRING,
                                                        NULL,
                                                        NULL);
        new_hdata->hash_var_hdata = hashtable_new (8,
                                                   WEECHAT_HASHTABLE_STRING,
                                                   WEECHAT_HASHTABLE_STRING,
                                                   NULL,
                                                   NULL);
        new_hdata->hash_list = hashtable_new (8,
                                              WEECHAT_HASHTABLE_STRING,
                                              WEECHAT_HASHTABLE_POINTER,
                                              NULL,
                                              NULL);
        hashtable_set (weechat_hdata, hdata_name, new_hdata);
    }

    return new_hdata;
}

/*
 * hdata_new_var: add a new variable (offset/type) in a hdata
 */

void
hdata_new_var (struct t_hdata *hdata, const char *name, int offset, int type,
               const char *array_size, const char *hdata_name)
{
    int value;

    if (hdata && name)
    {
        value = (type << 16) | (offset & 0xFFFF);
        hashtable_set (hdata->hash_var, name, &value);
        if (array_size && array_size[0])
            hashtable_set (hdata->hash_var_array_size, name, array_size);
        if (hdata_name && hdata_name[0])
            hashtable_set (hdata->hash_var_hdata, name, hdata_name);
    }
}

/*
 * hdata_new_list: add a new list pointer in a hdata
 */

void
hdata_new_list (struct t_hdata *hdata, const char *name, void *pointer)
{
    if (hdata && name)
        hashtable_set (hdata->hash_list, name, pointer);
}

/*
 * hdata_get_var_offset: get offset of variable
 */

int
hdata_get_var_offset (struct t_hdata *hdata, const char *name)
{
    int *ptr_value;

    if (hdata && name)
    {
        ptr_value = hashtable_get (hdata->hash_var, name);
        if (ptr_value)
            return (*ptr_value) & 0xFFFF;
    }

    return -1;
}

/*
 * hdata_get_var_type: get type of variable (as integer)
 */

int
hdata_get_var_type (struct t_hdata *hdata, const char *name)
{
    int *ptr_value;

    if (hdata && name)
    {
        ptr_value = hashtable_get (hdata->hash_var, name);
        if (ptr_value)
            return (*ptr_value) >> 16;
    }

    return -1;
}

/*
 * hdata_get_var_type_string: get type of variable (as string)
 */

const char *
hdata_get_var_type_string (struct t_hdata *hdata, const char *name)
{
    int *ptr_value;

    if (hdata && name)
    {
        ptr_value = hashtable_get (hdata->hash_var, name);
        if (ptr_value)
            return hdata_type_string[(*ptr_value) >> 16];
    }

    return NULL;
}

/*
 * hdata_get_var_array_size: get array size for variable (if variable is an
 *                           array)
 *                           return -1 if if variable is not an array
 *                           (or if error)
 */

int
hdata_get_var_array_size (struct t_hdata *hdata, void *pointer,
                          const char *name)
{
    const char *ptr_size;
    char *error;
    long value;
    int i, type, offset;
    void *ptr_value;

    if (hdata && name)
    {
        ptr_size = hashtable_get (hdata->hash_var_array_size, name);
        if (ptr_size)
        {
            if (strcmp (ptr_size, "*") == 0)
            {
                /*
                 * automatic size: look for NULL in array
                 * (this automatic size is possible only with pointers, so with
                 * types: string, pointer, hashtable)
                 */
                type = hdata_get_var_type (hdata, name);
                if ((type == WEECHAT_HDATA_STRING)
                    || (type == WEECHAT_HDATA_POINTER)
                    || (type == WEECHAT_HDATA_HASHTABLE))
                {
                    offset = hdata_get_var_offset (hdata, name);
                    if (offset >= 0)
                    {
                        if (!(*((void **)(pointer + offset))))
                            return 0;
                        i = 0;
                        while (1)
                        {
                            switch (type)
                            {
                                case WEECHAT_HDATA_STRING:
                                    ptr_value = (*((char ***)(pointer + offset)))[i];
                                    break;
                                case WEECHAT_HDATA_POINTER:
                                    ptr_value = (*((void ***)(pointer + offset)))[i];
                                    break;
                                case WEECHAT_HDATA_HASHTABLE:
                                    ptr_value = (*((struct t_hashtable ***)(pointer + offset)))[i];
                                    break;
                            }
                            if (!ptr_value)
                                break;
                            i++;
                        }
                        return i;
                    }
                }
            }
            else
            {
                /* fixed size: the size can be a name of variable or integer */
                if (hdata_get_var_offset (hdata, ptr_size) >= 0)
                {
                    /* size is the name of a variable in hdata, read it */
                    offset = hdata_get_var_offset (hdata, ptr_size);
                    switch (hdata_get_var_type (hdata, ptr_size))
                    {
                        case WEECHAT_HDATA_CHAR:
                            return (int)(*((char *)(pointer + offset)));
                        case WEECHAT_HDATA_INTEGER:
                            return *((int *)(pointer + offset));
                        case WEECHAT_HDATA_LONG:
                            return (int)(*((long *)(pointer + offset)));
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
        }
    }

    return -1;
}

/*
 * hdata_get_var_array_size_string: get array size for variable as string
 */

const char *
hdata_get_var_array_size_string (struct t_hdata *hdata, void *pointer,
                                 const char *name)
{
    /* make C compiler happy */
    (void) pointer;

    if (hdata && name)
        return (const char *)hashtable_get (hdata->hash_var_array_size, name);

    return NULL;
}

/*
 * hdata_get_var_hdata: get hdata name for a variable (NULL if variable has
 *                      no hdata)
 */

const char *
hdata_get_var_hdata (struct t_hdata *hdata, const char *name)
{
    if (hdata && name)
        return (const char *)hashtable_get (hdata->hash_var_hdata, name);

    return NULL;
}

/*
 * hdata_get_var: get pointer to content of variable using hdata var name
 */

void *
hdata_get_var (struct t_hdata *hdata, void *pointer, const char *name)
{
    int offset;

    if (hdata && pointer)
    {
        offset = hdata_get_var_offset (hdata, name);
        if (offset >= 0)
            return pointer + offset;
    }

    return NULL;
}

/*
 * hdata_get_var_at_offset: get pointer to content of variable using hdata var
 *                          offset
 */

void *
hdata_get_var_at_offset (struct t_hdata *hdata, void *pointer, int offset)
{
    if (hdata && pointer)
        return pointer + offset;

    return NULL;
}

/*
 * hdata_get_list: get a list pointer
 */

void *
hdata_get_list (struct t_hdata *hdata, const char *name)
{
    void *ptr_value;

    if (hdata && name)
    {
        ptr_value = hashtable_get (hdata->hash_list, name);
        if (ptr_value)
            return *((void **)ptr_value);
    }

    return NULL;
}

/*
 * hdata_check_pointer: check if a pointer is valid for a given hdata/list
 *                      return 1 if pointer exists in list
 *                             0 if pointer does not exist
 */

int
hdata_check_pointer (struct t_hdata *hdata, void *list, void *pointer)
{
    void *ptr_current;

    if (hdata && list && pointer)
    {
        if (pointer == list)
            return 1;
        ptr_current = list;
        while (ptr_current)
        {
            ptr_current = hdata_move (hdata, ptr_current, 1);
            if (ptr_current && (ptr_current == pointer))
                return 1;
        }
    }

    return 0;
}

/*
 * hdata_move: move pointer to another element in list
 */

void *
hdata_move (struct t_hdata *hdata, void *pointer, int count)
{
    char *ptr_var;
    int i, abs_count;

    if (hdata && pointer && (count != 0))
    {
        ptr_var = (count < 0) ? hdata->var_prev : hdata->var_next;
        abs_count = abs(count);

        for (i = 0; i < abs_count; i++)
        {
            pointer = hdata_pointer (hdata, pointer, ptr_var);
            if (pointer)
                return pointer;
        }
    }

    return NULL;
}

/*
 * hdata_get_index_and_name: extract index from name of variable
 *                           A name can contain index with this format:
 *                           "N|name" (where N is an integer >= 0)
 *                           "index" is set to N, "ptr_name" points to start
 *                           of name in string (after the "|" if found)
 */

void
hdata_get_index_and_name (const char *name, int *index, const char **ptr_name)
{
    char *pos, *str_index, *error;
    long number;

    if (index)
        *index = 0;
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
 * hdata_char: get char value of a variable in structure using hdata
 */

char
hdata_char (struct t_hdata *hdata, void *pointer, const char *name)
{
    int offset, index;
    const char *ptr_name;

    if (hdata && pointer && name)
    {
        hdata_get_index_and_name (name, &index, &ptr_name);
        offset = hdata_get_var_offset (hdata, ptr_name);
        if (offset >= 0)
        {
            if (hdata_get_var_array_size_string (hdata, pointer, ptr_name))
            {
                if (*((void **)(pointer + offset)))
                    return (*((char **)(pointer + offset)))[index];
            }
            else
                return *((char *)(pointer + offset));
        }
    }

    return '\0';
}

/*
 * hdata_integer: get integer value of a variable in structure using hdata
 */

int
hdata_integer (struct t_hdata *hdata, void *pointer, const char *name)
{
    int offset, index;
    const char *ptr_name;

    if (hdata && pointer && name)
    {
        hdata_get_index_and_name (name, &index, &ptr_name);
        offset = hdata_get_var_offset (hdata, ptr_name);
        if (offset >= 0)
        {
            if (hdata_get_var_array_size_string (hdata, pointer, ptr_name))
            {
                if (*((void **)(pointer + offset)))
                    return (*((int **)(pointer + offset)))[index];
            }
            else
                return *((int *)(pointer + offset));
        }
    }

    return 0;
}

/*
 * hdata_long: get long value of a variable in structure using hdata
 */

long
hdata_long (struct t_hdata *hdata, void *pointer, const char *name)
{
    int offset, index;
    const char *ptr_name;

    if (hdata && pointer && name)
    {
        hdata_get_index_and_name (name, &index, &ptr_name);
        offset = hdata_get_var_offset (hdata, ptr_name);
        if (offset >= 0)
        {
            if (hdata_get_var_array_size_string (hdata, pointer, ptr_name))
            {
                if (*((void **)(pointer + offset)))
                    return (*((long **)(pointer + offset)))[index];
            }
            else
                return *((long *)(pointer + offset));
        }
    }

    return 0;
}

/*
 * hdata_string: get string value of a variable in structure using hdata
 */

const char *
hdata_string (struct t_hdata *hdata, void *pointer, const char *name)
{
    int offset, index;
    const char *ptr_name;

    if (hdata && pointer && name)
    {
        hdata_get_index_and_name (name, &index, &ptr_name);
        offset = hdata_get_var_offset (hdata, ptr_name);
        if (offset >= 0)
        {
            if (hdata_get_var_array_size_string (hdata, pointer, ptr_name))
            {
                if (*((void **)(pointer + offset)))
                    return (*((char ***)(pointer + offset)))[index];
            }
            else
                return *((char **)(pointer + offset));
        }
    }

    return NULL;
}

/*
 * hdata_pointer: get pointer value of a variable in structure using hdata
 */

void *
hdata_pointer (struct t_hdata *hdata, void *pointer, const char *name)
{
    int offset, index;
    const char *ptr_name;

    if (hdata && pointer && name)
    {
        hdata_get_index_and_name (name, &index, &ptr_name);
        offset = hdata_get_var_offset (hdata, ptr_name);
        if (offset >= 0)
        {
            if (hdata_get_var_array_size_string (hdata, pointer, ptr_name))
            {
                if (*((void **)(pointer + offset)))
                    return (*((void ***)(pointer + offset)))[index];
            }
            else
                return *((void **)(pointer + offset));
        }
    }

    return NULL;
}

/*
 * hdata_time: get time value of a variable in structure using hdata
 */

time_t
hdata_time (struct t_hdata *hdata, void *pointer, const char *name)
{
    int offset, index;
    const char *ptr_name;

    if (hdata && pointer && name)
    {
        hdata_get_index_and_name (name, &index, &ptr_name);
        offset = hdata_get_var_offset (hdata, ptr_name);
        if (offset >= 0)
        {
            if (hdata_get_var_array_size_string (hdata, pointer, ptr_name))
            {
                if (*((void **)(pointer + offset)))
                    return (*((time_t **)(pointer + offset)))[index];
            }
            else
                return *((time_t *)(pointer + offset));
        }
    }

    return 0;
}

/*
 * hdata_hashtable: get hashtable value of a variable in structure using hdata
 */

struct t_hashtable *
hdata_hashtable (struct t_hdata *hdata, void *pointer, const char *name)
{
    int offset, index;
    const char *ptr_name;

    if (hdata && pointer && name)
    {
        hdata_get_index_and_name (name, &index, &ptr_name);
        offset = hdata_get_var_offset (hdata, ptr_name);
        if (offset >= 0)
        {
            if (hdata_get_var_array_size_string (hdata, pointer, ptr_name))
            {
                if (*((void **)(pointer + offset)))
                    return (*((struct t_hashtable ***)(pointer + offset)))[index];
            }
            else
                return *((struct t_hashtable **)(pointer + offset));
        }
    }

    return NULL;
}

/*
 * hdata_get_string: get a hdata property as string
 */

const char *
hdata_get_string (struct t_hdata *hdata, const char *property)
{
    if (hdata && property)
    {
        if (string_strcasecmp (property, "var_keys") == 0)
            return hashtable_get_string (hdata->hash_var, "keys");
        else if (string_strcasecmp (property, "var_values") == 0)
            return hashtable_get_string (hdata->hash_var, "values");
        else if (string_strcasecmp (property, "var_keys_values") == 0)
            return hashtable_get_string (hdata->hash_var, "keys_values");
        else if (string_strcasecmp (property, "var_prev") == 0)
            return hdata->var_prev;
        else if (string_strcasecmp (property, "var_next") == 0)
            return hdata->var_next;
        else if (string_strcasecmp (property, "var_array_size_keys") == 0)
            return hashtable_get_string (hdata->hash_var_array_size, "keys");
        else if (string_strcasecmp (property, "var_array_size_values") == 0)
            return hashtable_get_string (hdata->hash_var_array_size, "values");
        else if (string_strcasecmp (property, "var_array_size_keys_values") == 0)
            return hashtable_get_string (hdata->hash_var_array_size, "keys_values");
        else if (string_strcasecmp (property, "var_hdata_keys") == 0)
            return hashtable_get_string (hdata->hash_var_hdata, "keys");
        else if (string_strcasecmp (property, "var_hdata_values") == 0)
            return hashtable_get_string (hdata->hash_var_hdata, "values");
        else if (string_strcasecmp (property, "var_hdata_keys_values") == 0)
            return hashtable_get_string (hdata->hash_var_hdata, "keys_values");
        else if (string_strcasecmp (property, "list_keys") == 0)
            return hashtable_get_string (hdata->hash_list, "keys");
        else if (string_strcasecmp (property, "list_values") == 0)
            return hashtable_get_string (hdata->hash_list, "values");
        else if (string_strcasecmp (property, "list_keys_values") == 0)
            return hashtable_get_string (hdata->hash_list, "keys_values");
    }

    return NULL;
}

/*
 * hdata_free: free a hdata
 */

void
hdata_free (struct t_hdata *hdata)
{
    if (hdata->hash_var)
        hashtable_free (hdata->hash_var);
    if (hdata->var_prev)
        free (hdata->var_prev);
    if (hdata->var_next)
        free (hdata->var_next);
    if (hdata->hash_var_array_size)
        hashtable_free (hdata->hash_var_array_size);
    if (hdata->hash_var_hdata)
        hashtable_free (hdata->hash_var_hdata);
    if (hdata->hash_list)
        hashtable_free (hdata->hash_list);

    free (hdata);
}

/*
 * hdata_free_all_plugin_map_cb: function called for each hdata in memory
 */

void
hdata_free_all_plugin_map_cb (void *data, struct t_hashtable *hashtable,
                              const void *key, const void *value)
{
    struct t_hdata *ptr_hdata;

    ptr_hdata = (struct t_hdata *)value;

    if (ptr_hdata->plugin == (struct t_weechat_plugin *)data)
    {
        hdata_free (ptr_hdata);
        hashtable_remove (hashtable, key);
    }
}

/*
 * hdata_free_all_plugin: free all hdata created by a plugin
 */

void
hdata_free_all_plugin (struct t_weechat_plugin *plugin)
{
    hashtable_map (weechat_hdata, &hdata_free_all_plugin_map_cb, plugin);
}

/*
 * hdata_free_all_map_cb: function called for each hdata in memory
 */

void
hdata_free_all_map_cb (void *data, struct t_hashtable *hashtable,
                       const void *key, const void *value)
{
    struct t_hdata *ptr_hdata;

    /* make C compiler happy */
    (void) data;

    ptr_hdata = (struct t_hdata *)value;

    hdata_free (ptr_hdata);
    hashtable_remove (hashtable, key);
}

/*
 * hdata_free_all: free all hdata
 */

void
hdata_free_all ()
{
    hashtable_map (weechat_hdata, &hdata_free_all_map_cb, NULL);
}

/*
 * hdata_print_log_map_cb: function called for each hdata in memory
 */

void
hdata_print_log_map_cb (void *data, struct t_hashtable *hashtable,
                        const void *key, const void *value)
{
    struct t_hdata *ptr_hdata;

    /* make C compiler happy */
    (void) data;
    (void) hashtable;

    ptr_hdata = (struct t_hdata *)value;

    log_printf ("");
    log_printf ("[hdata (addr:0x%lx, name:'%s')]", ptr_hdata, (const char *)key);
    log_printf ("  plugin . . . . . . . . : 0x%lx", ptr_hdata->plugin);
    log_printf ("  var_prev . . . . . . . : '%s'",  ptr_hdata->var_prev);
    log_printf ("  var_next . . . . . . . : '%s'",  ptr_hdata->var_next);
    log_printf ("  hash_var . . . . . . . : 0x%lx (hashtable: '%s')",
                ptr_hdata->hash_var,
                hashtable_get_string (ptr_hdata->hash_var, "keys_values"));
    log_printf ("  hash_var_array_size. . : 0x%lx (hashtable: '%s')",
                ptr_hdata->hash_var_array_size,
                hashtable_get_string (ptr_hdata->hash_var_array_size, "keys_values"));
    log_printf ("  hash_var_hdata . . . . : 0x%lx (hashtable: '%s')",
                ptr_hdata->hash_var_hdata,
                hashtable_get_string (ptr_hdata->hash_var_hdata, "keys_values"));
    log_printf ("  hash_list. . . . . . . : 0x%lx (hashtable: '%s')",
                ptr_hdata->hash_list,
                hashtable_get_string (ptr_hdata->hash_list, "keys_values"));
}

/*
 * hdata_print_log: print hdata in log (usually for crash dump)
 */

void
hdata_print_log ()
{
    hashtable_map (weechat_hdata, &hdata_print_log_map_cb, NULL);
}

/*
 * hdata_init: create hashtable with hdata
 */

void
hdata_init ()
{
    weechat_hdata = hashtable_new (16,
                                   WEECHAT_HASHTABLE_STRING,
                                   WEECHAT_HASHTABLE_POINTER,
                                   NULL,
                                   NULL);
}

/*
 * hdata_end: free all hdata and hashtable with hdata
 */

void
hdata_end ()
{
    hdata_free_all ();
    hashtable_free (weechat_hdata);
}
