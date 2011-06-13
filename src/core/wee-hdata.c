/*
 * Copyright (C) 2011 Sebastien Helleu <flashcode@flashtux.org>
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
 * wee-hdata.c: direct access to WeeChat data using hashtables (for C plugins)
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


struct t_hdata *weechat_hdata = NULL;
struct t_hdata *last_weechat_hdata = NULL;

char *hdata_type_string[6] =
{ "other", "integer", "long", "string", "pointer", "time" };


/*
 * hdata_new: create a new hdata
 */

struct t_hdata *
hdata_new (const char *hdata_name, const char *var_prev, const char *var_next)
{
    struct t_hdata *new_hdata;
    
    if (!hdata_name || !hdata_name[0])
        return NULL;
    
    new_hdata = malloc (sizeof (*new_hdata));
    if (new_hdata)
    {
        new_hdata->name = strdup (hdata_name);
        new_hdata->hash_var = hashtable_new (8,
                                             WEECHAT_HASHTABLE_STRING,
                                             WEECHAT_HASHTABLE_INTEGER,
                                             NULL,
                                             NULL);
        new_hdata->var_prev = (var_prev) ? strdup (var_prev) : NULL;
        new_hdata->var_next = (var_next) ? strdup (var_next) : NULL;
        new_hdata->hash_list = hashtable_new (8,
                                              WEECHAT_HASHTABLE_STRING,
                                              WEECHAT_HASHTABLE_POINTER,
                                              NULL,
                                              NULL);
        
        new_hdata->prev_hdata = last_weechat_hdata;
        new_hdata->next_hdata = NULL;
        if (weechat_hdata)
            last_weechat_hdata->next_hdata = new_hdata;
        else
            weechat_hdata = new_hdata;
        last_weechat_hdata = new_hdata;
    }
    
    return new_hdata;
}

/*
 * hdata_new_var: add a new variable (offset/type) in a hdata
 */

void
hdata_new_var (struct t_hdata *hdata, const char *name, int offset, int type)
{
    int value;
    
    if (hdata && name)
    {
        value = (type << 16) | (offset & 0xFFFF);
        hashtable_set (hdata->hash_var, name, &value);
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
 * hdata_integer: get integer value of a variable in structure using hdata
 */

int
hdata_integer (struct t_hdata *hdata, void *pointer, const char *name)
{
    int offset;
    
    if (hdata && pointer)
    {
        offset = hdata_get_var_offset (hdata, name);
        if (offset >= 0)
            return *((int *)(pointer + offset));
    }
    
    return 0;
}

/*
 * hdata_long: get long value of a variable in structure using hdata
 */

long
hdata_long (struct t_hdata *hdata, void *pointer, const char *name)
{
    int offset;
    
    if (hdata && pointer)
    {
        offset = hdata_get_var_offset (hdata, name);
        if (offset >= 0)
            return *((long *)(pointer + offset));
    }
    
    return 0;
}

/*
 * hdata_string: get string value of a variable in structure using hdata
 */

const char *
hdata_string (struct t_hdata *hdata, void *pointer, const char *name)
{
    int offset;
    
    if (hdata && pointer)
    {
        offset = hdata_get_var_offset (hdata, name);
        if (offset >= 0)
            return *((char **)(pointer + offset));
    }
    
    return NULL;
}

/*
 * hdata_pointer: get pointer value of a variable in structure using hdata
 */

void *
hdata_pointer (struct t_hdata *hdata, void *pointer, const char *name)
{
    int offset;
    
    if (hdata && pointer)
    {
        offset = hdata_get_var_offset (hdata, name);
        if (offset >= 0)
            return *((void **)(pointer + offset));
    }
    
    return NULL;
}

/*
 * hdata_time: get time value of a variable in structure using hdata
 */

time_t
hdata_time (struct t_hdata *hdata, void *pointer, const char *name)
{
    int offset;
    
    if (hdata && pointer)
    {
        offset = hdata_get_var_offset (hdata, name);
        if (offset >= 0)
            return *((time_t *)(pointer + offset));
    }
    
    return 0;
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
 * hdata_print_log: print hdata in log (usually for crash dump)
 */

void
hdata_print_log ()
{
    struct t_hdata *ptr_hdata;
    
    for (ptr_hdata = weechat_hdata; ptr_hdata;
         ptr_hdata = ptr_hdata->next_hdata)
    {
        log_printf ("");
        log_printf ("[hdata (addr:0x%lx)]", ptr_hdata);
        log_printf ("  name . . . . . . . . . : '%s'",  ptr_hdata->name);
        log_printf ("  hash_var . . . . . . . : 0x%lx (hashtable: '%s')",
                    ptr_hdata->hash_var,
                    hashtable_get_string (ptr_hdata->hash_var, "keys_values"));
        log_printf ("  var_prev . . . . . . . : '%s'",  ptr_hdata->var_prev);
        log_printf ("  var_next . . . . . . . : '%s'",  ptr_hdata->var_next);
        log_printf ("  hash_list. . . . . . . : 0x%lx (hashtable: '%s')",
                    ptr_hdata->hash_list,
                    hashtable_get_string (ptr_hdata->hash_list, "keys_values"));
        log_printf ("  prev_hdata . . . . . . : 0x%lx", ptr_hdata->prev_hdata);
        log_printf ("  next_hdata . . . . . . : 0x%lx", ptr_hdata->next_hdata);
    }
}
