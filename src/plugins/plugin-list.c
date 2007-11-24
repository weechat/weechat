/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* plugin-list.c: manages plugin info lists */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../core/weechat.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "plugin-list.h"


struct t_plugin_list *plugin_lists = NULL;
struct t_plugin_list *last_plugin_list = NULL;


/*
 * plugin_list_new: create a new plugin list
 */

struct t_plugin_list *
plugin_list_new ()
{
    struct t_plugin_list *new_list;

    new_list = (struct t_plugin_list *)malloc (sizeof (struct t_plugin_list));
    if (new_list)
    {
        new_list->items = NULL;
        new_list->last_item = NULL;
        new_list->ptr_item = NULL;
        
        new_list->prev_list = last_plugin_list;
        new_list->next_list = NULL;
        if (plugin_lists)
            last_plugin_list->next_list = new_list;
        else
            plugin_lists = new_list;
        last_plugin_list = new_list;
    }
    
    return new_list;
}

/*
 * plugin_list_new_item: create a new item in a plugin list
 */

struct t_plugin_list_item *
plugin_list_new_item (struct t_plugin_list *list)
{
    struct t_plugin_list_item *new_item;

    new_item = (struct t_plugin_list_item *)malloc (sizeof (struct t_plugin_list_item));
    if (new_item)
    {
        new_item->vars = NULL;
        new_item->last_var = NULL;
        new_item->fields = NULL;
        
        new_item->prev_item = list->last_item;
        new_item->next_item = NULL;
        if (list->items)
            list->last_item->next_item = new_item;
        else
            list->items = new_item;
        list->last_item = new_item;
    }
    
    return new_item;
}

/*
 * plugin_list_new_var_int: create a new integer variable in an item
 */

struct t_plugin_list_var *
plugin_list_new_var_int (struct t_plugin_list_item *item,
                         char *name, int value)
{
    struct t_plugin_list_var *new_var;
    
    if (!item || !name || !name[0])
        return NULL;
    
    new_var = (struct t_plugin_list_var *)malloc (sizeof (struct t_plugin_list_var));
    if (new_var)
    {
        new_var->name = strdup (name);
        new_var->type = PLUGIN_LIST_VAR_INTEGER;
        new_var->value_int = value;
        new_var->value_string = NULL;
        new_var->value_pointer = NULL;
        new_var->value_time = 0;
        
        new_var->prev_var = item->last_var;
        new_var->next_var = NULL;
        if (item->vars)
            item->last_var->next_var = new_var;
        else
            item->vars = new_var;
        item->last_var = new_var;
    }
    
    return new_var;
}

/*
 * plugin_list_new_var_string: create a new string variable in an item
 */

struct t_plugin_list_var *
plugin_list_new_var_string (struct t_plugin_list_item *item,
                            char *name, char *value)
{
    struct t_plugin_list_var *new_var;
    
    if (!item || !name || !name[0])
        return NULL;
    
    new_var = (struct t_plugin_list_var *)malloc (sizeof (struct t_plugin_list_var));
    if (new_var)
    {
        new_var->name = strdup (name);
        new_var->type = PLUGIN_LIST_VAR_STRING;
        new_var->value_int = 0;
        new_var->value_string = (value) ? strdup (value) : NULL;
        new_var->value_time = 0;
        
        new_var->prev_var = item->last_var;
        new_var->next_var = NULL;
        if (item->vars)
            item->last_var->next_var = new_var;
        else
            item->vars = new_var;
        item->last_var = new_var;
    }
    
    return new_var;
}

/*
 * plugin_list_new_var_pointer: create a new pointer variable in an item
 */

struct t_plugin_list_var *
plugin_list_new_var_pointer (struct t_plugin_list_item *item,
                             char *name, void *pointer)
{
    struct t_plugin_list_var *new_var;
    
    if (!item || !name || !name[0])
        return NULL;
    
    new_var = (struct t_plugin_list_var *)malloc (sizeof (struct t_plugin_list_var));
    if (new_var)
    {
        new_var->name = strdup (name);
        new_var->type = PLUGIN_LIST_VAR_POINTER;
        new_var->value_int = 0;
        new_var->value_string = NULL;
        new_var->value_pointer = pointer;
        new_var->value_time = 0;
        
        new_var->prev_var = item->last_var;
        new_var->next_var = NULL;
        if (item->vars)
            item->last_var->next_var = new_var;
        else
            item->vars = new_var;
        item->last_var = new_var;
    }
    
    return new_var;
}

/*
 * plugin_list_new_var_time: create a new time variable in an item
 */

struct t_plugin_list_var *
plugin_list_new_var_time (struct t_plugin_list_item *item,
                          char *name, time_t time)
{
    struct t_plugin_list_var *new_var;
    
    if (!item || !name || !name[0])
        return NULL;
    
    new_var = (struct t_plugin_list_var *)malloc (sizeof (struct t_plugin_list_var));
    if (new_var)
    {
        new_var->name = strdup (name);
        new_var->type = PLUGIN_LIST_VAR_TIME;
        new_var->value_int = 0;
        new_var->value_string = NULL;
        new_var->value_pointer = NULL;
        new_var->value_time = time;
        
        new_var->prev_var = item->last_var;
        new_var->next_var = NULL;
        if (item->vars)
            item->last_var->next_var = new_var;
        else
            item->vars = new_var;
        item->last_var = new_var;
    }
    
    return new_var;
}

/*
 * plugin_list_valid: check if a list pointer exists
 *                    return 1 if list exists
 *                           0 if list is not found
 */

int
plugin_list_valid (struct t_plugin_list *list)
{
    struct t_plugin_list *ptr_list;
    
    for (ptr_list = plugin_lists; ptr_list;
         ptr_list = ptr_list->next_list)
    {
        if (ptr_list == list)
            return 1;
    }
    
    /* list not found */
    return 0;
}

/*
 * plugin_list_next_item: return next item for a list
 *                        if current item pointer is NULL,
 *                        then return first item of list
 */

struct t_plugin_list_item *
plugin_list_next_item (struct t_plugin_list *list)
{
    if (!list->ptr_item)
    {
        list->ptr_item = list->items;
        return list->ptr_item;
    }
    list->ptr_item = list->ptr_item->next_item;
    return list->ptr_item;
}

/*
 * plugin_list_prev_item: return previous item for a list
 *                        if current item pointer is NULL,
 *                        then return last item of list
 */

struct t_plugin_list_item *
plugin_list_prev_item (struct t_plugin_list *list)
{
    if (!list->ptr_item)
    {
        list->ptr_item = list->last_item;
        return list->ptr_item;
    }
    list->ptr_item = list->ptr_item->prev_item;
    return list->ptr_item;
}

/*
 * plugin_list_get_fields: get list of fields for current list item
 */

char *
plugin_list_get_fields (struct t_plugin_list *list)
{
    struct t_plugin_list_var *ptr_var;
    int length;
    
    if (!list || !list->ptr_item)
        return NULL;

    /* list of fields already asked ? if yes, just return string */
    if (list->ptr_item->fields)
        return list->ptr_item->fields;
    
    length = 0;
    for (ptr_var = list->ptr_item->vars; ptr_var; ptr_var = ptr_var->next_var)
    {
        length += strlen (ptr_var->name) + 3;
    }
    
    list->ptr_item->fields = (char *)malloc (length + 1);
    if (!list->ptr_item->fields)
        return NULL;
    
    list->ptr_item->fields[0] = '\0';
    for (ptr_var = list->ptr_item->vars; ptr_var; ptr_var = ptr_var->next_var)
    {
        switch (ptr_var->type)
        {
            case PLUGIN_LIST_VAR_INTEGER:
                strcat (list->ptr_item->fields, "i:");
                break;
            case PLUGIN_LIST_VAR_STRING:
                strcat (list->ptr_item->fields, "s:");
                break;
            case PLUGIN_LIST_VAR_POINTER:
                strcat (list->ptr_item->fields, "p:");
                break;
            case PLUGIN_LIST_VAR_TIME:
                strcat (list->ptr_item->fields, "t:");
                break;
        }
        strcat (list->ptr_item->fields, ptr_var->name);
        if (ptr_var->next_var)
            strcat (list->ptr_item->fields, ",");
    }
    
    return list->ptr_item->fields;
}

/*
 * plugin_list_get_int: get an integer variable value in current list item
 */

int
plugin_list_get_int (struct t_plugin_list *list, char *var)
{
    struct t_plugin_list_var *ptr_var;
    
    if (!list || !list->ptr_item || !var || !var[0])
        return 0;
    
    for (ptr_var = list->ptr_item->vars; ptr_var; ptr_var = ptr_var->next_var)
    {
        if (string_strcasecmp (ptr_var->name, var) == 0)
        {
            if (ptr_var->type == PLUGIN_LIST_VAR_INTEGER)
                return ptr_var->value_int;
            else
                return 0;
        }
    }
    
    /* variable not found */
    return 0;
}

/*
 * plugin_list_get_string: get a string variable value in current list item
 */

char *
plugin_list_get_string (struct t_plugin_list *list, char *var)
{
    struct t_plugin_list_var *ptr_var;
    
    if (!list || !list->ptr_item || !var || !var[0])
        return NULL;
    
    for (ptr_var = list->ptr_item->vars; ptr_var; ptr_var = ptr_var->next_var)
    {
        if (string_strcasecmp (ptr_var->name, var) == 0)
        {
            if (ptr_var->type == PLUGIN_LIST_VAR_STRING)
                return ptr_var->value_string;
            else
                return NULL;
        }
    }
    
    /* variable not found */
    return NULL;
}

/*
 * plugin_list_get_pointer: get a pointer variable value in current list item
 */

void *
plugin_list_get_pointer (struct t_plugin_list *list, char *var)
{
    struct t_plugin_list_var *ptr_var;
    
    if (!list || !list->ptr_item || !var || !var[0])
        return NULL;
    
    for (ptr_var = list->ptr_item->vars; ptr_var; ptr_var = ptr_var->next_var)
    {
        if (string_strcasecmp (ptr_var->name, var) == 0)
        {
            if (ptr_var->type == PLUGIN_LIST_VAR_POINTER)
                return ptr_var->value_pointer;
            else
                return NULL;
        }
    }
    
    /* variable not found */
    return NULL;
}

/*
 * plugin_list_get_time: get a time variable value in current list item
 */

time_t
plugin_list_get_time (struct t_plugin_list *list, char *var)
{
    struct t_plugin_list_var *ptr_var;
    
    if (!list || !list->ptr_item || !var || !var[0])
        return 0;
    
    for (ptr_var = list->ptr_item->vars; ptr_var; ptr_var = ptr_var->next_var)
    {
        if (string_strcasecmp (ptr_var->name, var) == 0)
        {
            if (ptr_var->type == PLUGIN_LIST_VAR_TIME)
                return ptr_var->value_time;
            else
                return 0;
        }
    }
    
    /* variable not found */
    return 0;
}

/*
 * plugin_list_var_free: free a plugin list variable
 */

void
plugin_list_var_free (struct t_plugin_list_item *item,
                      struct t_plugin_list_var *var)
{
    struct t_plugin_list_var *new_vars;
    
    /* remove var */
    if (item->last_var == var)
        item->last_var = var->prev_var;
    if (var->prev_var)
    {
        (var->prev_var)->next_var = var->next_var;
        new_vars = item->vars;
    }
    else
        new_vars = var->next_var;
    
    if (var->next_var)
        (var->next_var)->prev_var = var->prev_var;
    
    /* free data */
    if (var->name)
        free (var->name);
    if (var->value_string)
        free (var->value_string);
    
    item->vars = new_vars;
}

/*
 * plugin_list_item_free: free a plugin list item
 */

void
plugin_list_item_free (struct t_plugin_list *list,
                       struct t_plugin_list_item *item)
{
    struct t_plugin_list_item *new_items;
    
    /* remove var */
    if (list->last_item == item)
        list->last_item = item->prev_item;
    if (item->prev_item)
    {
        (item->prev_item)->next_item = item->next_item;
        new_items = list->items;
    }
    else
        new_items = item->next_item;
    
    if (item->next_item)
        (item->next_item)->prev_item = item->prev_item;
    
    /* free data */
    while (item->vars)
    {
        plugin_list_var_free (item, item->vars);
    }
    if (item->fields)
        free (item->fields);
    
    list->items = new_items;
}

/*
 * plugin_list_free: free a plugin list
 */

void
plugin_list_free (struct t_plugin_list *list)
{
    struct t_plugin_list *new_plugin_lists;
    
    /* remove list */
    if (last_plugin_list == list)
        last_plugin_list = list->prev_list;
    if (list->prev_list)
    {
        (list->prev_list)->next_list = list->next_list;
        new_plugin_lists = plugin_lists;
    }
    else
        new_plugin_lists = list->next_list;
    
    if (list->next_list)
        (list->next_list)->prev_list = list->prev_list;
    
    /* free data */
    while (list->items)
    {
        plugin_list_item_free (list, list->items);
    }
    
    plugin_lists = new_plugin_lists;
}

/*
 * plugin_list_print_log: print plugin lists infos in log (usually for crash dump)
 */

void
plugin_list_print_log ()
{
    struct t_plugin_list *ptr_list;
    struct t_plugin_list_item *ptr_item;
    struct t_plugin_list_var *ptr_var;
    
    for (ptr_list = plugin_lists; ptr_list;
         ptr_list = ptr_list->next_list)
    {
        log_printf ("\n");
        log_printf ("[plugin list (addr:0x%X)]\n", ptr_list);
        log_printf ("  items. . . . . . . . . : 0x%X\n", ptr_list->items);
        log_printf ("  last_item. . . . . . . : 0x%X\n", ptr_list->last_item);
        log_printf ("  ptr_item . . . . . . . : 0x%X\n", ptr_list->ptr_item);
        log_printf ("  prev_list. . . . . . . : 0x%X\n", ptr_list->prev_list);
        log_printf ("  next_list. . . . . . . : 0x%X\n", ptr_list->next_list);
        
        for (ptr_item = ptr_list->items; ptr_item;
             ptr_item = ptr_item->next_item)
        {
            log_printf ("\n");
            log_printf ("    [item (addr:0x%X)]\n", ptr_item);
            log_printf ("      vars . . . . . . . . . : 0x%X\n", ptr_item->vars);
            log_printf ("      last_var . . . . . . . : 0x%X\n", ptr_item->last_var);
            log_printf ("      prev_item. . . . . . . : 0x%X\n", ptr_item->prev_item);
            log_printf ("      next_item. . . . . . . : 0x%X\n", ptr_item->next_item);
            
            for (ptr_var = ptr_item->vars; ptr_var;
                 ptr_var = ptr_var->next_var)
            {
                log_printf ("\n");
                log_printf ("      [var (addr:0x%X)]\n", ptr_var);
                log_printf ("        name . . . . . . . . : '%s'\n", ptr_var->name);
                log_printf ("        type . . . . . . . . : %d\n",   ptr_var->type);
                log_printf ("        value_int. . . . . . : %d\n",   ptr_var->value_int);
                log_printf ("        value_string . . . . : '%s'\n", ptr_var->value_string);
                log_printf ("        value_time . . . . . : %ld\n",  ptr_var->value_time);
                log_printf ("        prev_var . . . . . . : 0x%X\n", ptr_var->prev_var);
                log_printf ("        next_var . . . . . . : 0x%X\n", ptr_var->next_var);
            }
        }
    }
}
