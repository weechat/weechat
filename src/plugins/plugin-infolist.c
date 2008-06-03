/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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

/* plugin-infolist.c: manages plugin info lists */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../core/weechat.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "plugin-infolist.h"


struct t_plugin_infolist *plugin_infolists = NULL;
struct t_plugin_infolist *last_plugin_infolist = NULL;


/*
 * plugin_list_new: create a new plugin list
 */

struct t_plugin_infolist *
plugin_infolist_new ()
{
    struct t_plugin_infolist *new_infolist;

    new_infolist = malloc (sizeof (*new_infolist));
    if (new_infolist)
    {
        new_infolist->items = NULL;
        new_infolist->last_item = NULL;
        new_infolist->ptr_item = NULL;
        
        new_infolist->prev_infolist = last_plugin_infolist;
        new_infolist->next_infolist = NULL;
        if (plugin_infolists)
            last_plugin_infolist->next_infolist = new_infolist;
        else
            plugin_infolists = new_infolist;
        last_plugin_infolist = new_infolist;
    }
    
    return new_infolist;
}

/*
 * plugin_infolist_new_item: create a new item in a plugin list
 */

struct t_plugin_infolist_item *
plugin_infolist_new_item (struct t_plugin_infolist *list)
{
    struct t_plugin_infolist_item *new_item;

    new_item = malloc (sizeof (*new_item));
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
 * plugin_infolist_new_var_integer: create a new integer variable in an item
 */

struct t_plugin_infolist_var *
plugin_infolist_new_var_integer (struct t_plugin_infolist_item *item,
                                 const char *name, int value)
{
    struct t_plugin_infolist_var *new_var;
    
    if (!item || !name || !name[0])
        return NULL;
    
    new_var = malloc (sizeof (*new_var));
    if (new_var)
    {
        new_var->name = strdup (name);
        new_var->type = PLUGIN_INFOLIST_INTEGER;
        new_var->value = malloc (sizeof (int));
        if (new_var->value)
            *((int *)new_var->value) = value;
        
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
 * plugin_infolist_new_var_string: create a new string variable in an item
 */

struct t_plugin_infolist_var *
plugin_infolist_new_var_string (struct t_plugin_infolist_item *item,
                                const char *name, const char *value)
{
    struct t_plugin_infolist_var *new_var;
    
    if (!item || !name || !name[0])
        return NULL;
    
    new_var = malloc (sizeof (*new_var));
    if (new_var)
    {
        new_var->name = strdup (name);
        new_var->type = PLUGIN_INFOLIST_STRING;
        new_var->value = (value) ? strdup (value) : NULL;
        
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
 * plugin_infolist_new_var_pointer: create a new pointer variable in an item
 */

struct t_plugin_infolist_var *
plugin_infolist_new_var_pointer (struct t_plugin_infolist_item *item,
                                 const char *name, void *pointer)
{
    struct t_plugin_infolist_var *new_var;
    
    if (!item || !name || !name[0])
        return NULL;
    
    new_var = malloc (sizeof (*new_var));
    if (new_var)
    {
        new_var->name = strdup (name);
        new_var->type = PLUGIN_INFOLIST_POINTER;
        new_var->value = pointer;
        
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
 * plugin_infolist_new_var_time: create a new time variable in an item
 */

struct t_plugin_infolist_var *
plugin_infolist_new_var_time (struct t_plugin_infolist_item *item,
                              const char *name, time_t time)
{
    struct t_plugin_infolist_var *new_var;
    
    if (!item || !name || !name[0])
        return NULL;
    
    new_var = malloc (sizeof (*new_var));
    if (new_var)
    {
        new_var->name = strdup (name);
        new_var->type = PLUGIN_INFOLIST_TIME;
        new_var->value = malloc (sizeof (time_t));
        if (new_var->value)
            *((time_t *)new_var->value) = time;
        
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
 * plugin_infolist_valid: check if a list pointer exists
 *                        return 1 if list exists
 *                               0 if list is not found
 */

int
plugin_infolist_valid (struct t_plugin_infolist *list)
{
    struct t_plugin_infolist *ptr_infolist;
    
    for (ptr_infolist = plugin_infolists; ptr_infolist;
         ptr_infolist = ptr_infolist->next_infolist)
    {
        if (ptr_infolist == list)
            return 1;
    }
    
    /* list not found */
    return 0;
}

/*
 * plugin_infolist_next_item: return next item for a list
 *                            if current item pointer is NULL,
 *                            then return first item of list
 */

struct t_plugin_infolist_item *
plugin_infolist_next_item (struct t_plugin_infolist *list)
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
 * plugin_infolist_prev_item: return previous item for a list
 *                            if current item pointer is NULL,
 *                            then return last item of list
 */

struct t_plugin_infolist_item *
plugin_infolist_prev_item (struct t_plugin_infolist *list)
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
 * plugin_infolist_get_fields: get list of fields for current list item
 */

char *
plugin_infolist_get_fields (struct t_plugin_infolist *list)
{
    struct t_plugin_infolist_var *ptr_var;
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
    
    list->ptr_item->fields = malloc (length + 1);
    if (!list->ptr_item->fields)
        return NULL;
    
    list->ptr_item->fields[0] = '\0';
    for (ptr_var = list->ptr_item->vars; ptr_var; ptr_var = ptr_var->next_var)
    {
        switch (ptr_var->type)
        {
            case PLUGIN_INFOLIST_INTEGER:
                strcat (list->ptr_item->fields, "i:");
                break;
            case PLUGIN_INFOLIST_STRING:
                strcat (list->ptr_item->fields, "s:");
                break;
            case PLUGIN_INFOLIST_POINTER:
                strcat (list->ptr_item->fields, "p:");
                break;
            case PLUGIN_INFOLIST_TIME:
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
 * plugin_infolist_get_integer: get an integer variable value in current list item
 */

int
plugin_infolist_get_integer (struct t_plugin_infolist *list, const char *var)
{
    struct t_plugin_infolist_var *ptr_var;
    
    if (!list || !list->ptr_item || !var || !var[0])
        return 0;
    
    for (ptr_var = list->ptr_item->vars; ptr_var; ptr_var = ptr_var->next_var)
    {
        if (string_strcasecmp (ptr_var->name, var) == 0)
        {
            if (ptr_var->type == PLUGIN_INFOLIST_INTEGER)
                return *((int *)ptr_var->value);
            else
                return 0;
        }
    }
    
    /* variable not found */
    return 0;
}

/*
 * plugin_infolist_get_string: get a string variable value in current list item
 */

char *
plugin_infolist_get_string (struct t_plugin_infolist *list, const char *var)
{
    struct t_plugin_infolist_var *ptr_var;
    
    if (!list || !list->ptr_item || !var || !var[0])
        return NULL;
    
    for (ptr_var = list->ptr_item->vars; ptr_var; ptr_var = ptr_var->next_var)
    {
        if (string_strcasecmp (ptr_var->name, var) == 0)
        {
            if (ptr_var->type == PLUGIN_INFOLIST_STRING)
                return (char *)ptr_var->value;
            else
                return NULL;
        }
    }
    
    /* variable not found */
    return NULL;
}

/*
 * plugin_infolist_get_pointer: get a pointer variable value in current list item
 */

void *
plugin_infolist_get_pointer (struct t_plugin_infolist *list, const char *var)
{
    struct t_plugin_infolist_var *ptr_var;
    
    if (!list || !list->ptr_item || !var || !var[0])
        return NULL;
    
    for (ptr_var = list->ptr_item->vars; ptr_var; ptr_var = ptr_var->next_var)
    {
        if (string_strcasecmp (ptr_var->name, var) == 0)
        {
            if (ptr_var->type == PLUGIN_INFOLIST_POINTER)
                return ptr_var->value;
            else
                return NULL;
        }
    }
    
    /* variable not found */
    return NULL;
}

/*
 * plugin_infolist_get_time: get a time variable value in current list item
 */

time_t
plugin_infolist_get_time (struct t_plugin_infolist *list, const char *var)
{
    struct t_plugin_infolist_var *ptr_var;
    
    if (!list || !list->ptr_item || !var || !var[0])
        return 0;
    
    for (ptr_var = list->ptr_item->vars; ptr_var; ptr_var = ptr_var->next_var)
    {
        if (string_strcasecmp (ptr_var->name, var) == 0)
        {
            if (ptr_var->type == PLUGIN_INFOLIST_TIME)
                return *((time_t *)ptr_var->value);
            else
                return 0;
        }
    }
    
    /* variable not found */
    return 0;
}

/*
 * plugin_infolist_var_free: free a plugin list variable
 */

void
plugin_infolist_var_free (struct t_plugin_infolist_item *item,
                          struct t_plugin_infolist_var *var)
{
    struct t_plugin_infolist_var *new_vars;
    
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
    if (((var->type == PLUGIN_INFOLIST_INTEGER)
         || (var->type == PLUGIN_INFOLIST_STRING)
         || (var->type == PLUGIN_INFOLIST_TIME))
        && var->value)
    {
        free (var->value);
    }
    
    free (var);
    
    item->vars = new_vars;
}

/*
 * plugin_infolist_item_free: free a plugin list item
 */

void
plugin_infolist_item_free (struct t_plugin_infolist *list,
                           struct t_plugin_infolist_item *item)
{
    struct t_plugin_infolist_item *new_items;
    
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
        plugin_infolist_var_free (item, item->vars);
    }
    if (item->fields)
        free (item->fields);
    
    free (item);
    
    list->items = new_items;
}

/*
 * plugin_infolist_free: free a plugin list
 */

void
plugin_infolist_free (struct t_plugin_infolist *list)
{
    struct t_plugin_infolist *new_plugin_infolists;
    
    /* remove list */
    if (last_plugin_infolist == list)
        last_plugin_infolist = list->prev_infolist;
    if (list->prev_infolist)
    {
        (list->prev_infolist)->next_infolist = list->next_infolist;
        new_plugin_infolists = plugin_infolists;
    }
    else
        new_plugin_infolists = list->next_infolist;
    
    if (list->next_infolist)
        (list->next_infolist)->prev_infolist = list->prev_infolist;
    
    /* free data */
    while (list->items)
    {
        plugin_infolist_item_free (list, list->items);
    }
    
    free (list);
    
    plugin_infolists = new_plugin_infolists;
}

/*
 * plugin_infolist_print_log: print plugin lists infos in log (usually for crash dump)
 */

void
plugin_infolist_print_log ()
{
    struct t_plugin_infolist *ptr_infolist;
    struct t_plugin_infolist_item *ptr_item;
    struct t_plugin_infolist_var *ptr_var;
    
    for (ptr_infolist = plugin_infolists; ptr_infolist;
         ptr_infolist = ptr_infolist->next_infolist)
    {
        log_printf ("");
        log_printf ("[plugin infolist (addr:0x%x)]", ptr_infolist);
        log_printf ("  items. . . . . . . . . : 0x%x", ptr_infolist->items);
        log_printf ("  last_item. . . . . . . : 0x%x", ptr_infolist->last_item);
        log_printf ("  ptr_item . . . . . . . : 0x%x", ptr_infolist->ptr_item);
        log_printf ("  prev_infolist. . . . . : 0x%x", ptr_infolist->prev_infolist);
        log_printf ("  next_infolist. . . . . : 0x%x", ptr_infolist->next_infolist);
        
        for (ptr_item = ptr_infolist->items; ptr_item;
             ptr_item = ptr_item->next_item)
        {
            log_printf ("");
            log_printf ("    [item (addr:0x%x)]", ptr_item);
            log_printf ("      vars . . . . . . . . . : 0x%x", ptr_item->vars);
            log_printf ("      last_var . . . . . . . : 0x%x", ptr_item->last_var);
            log_printf ("      prev_item. . . . . . . : 0x%x", ptr_item->prev_item);
            log_printf ("      next_item. . . . . . . : 0x%x", ptr_item->next_item);
            
            for (ptr_var = ptr_item->vars; ptr_var;
                 ptr_var = ptr_var->next_var)
            {
                log_printf ("");
                log_printf ("      [var (addr:0x%x)]", ptr_var);
                log_printf ("        name . . . . . . . . : '%s'", ptr_var->name);
                log_printf ("        type . . . . . . . . : %d",   ptr_var->type);
                switch (ptr_var->type)
                {
                    case PLUGIN_INFOLIST_INTEGER:
                        log_printf ("        value (integer). . . : %d", *((int *)ptr_var->value));
                        break;
                    case PLUGIN_INFOLIST_STRING:
                        log_printf ("        value (string) . . . : '%s'", (char *)ptr_var->value);
                        break;
                    case PLUGIN_INFOLIST_POINTER:
                        log_printf ("        value (pointer). . . : 0x%x", ptr_var->value);
                        break;
                    case PLUGIN_INFOLIST_TIME:
                        log_printf ("        value (time) . . . . : %ld", *((time_t *)ptr_var->value));
                        break;
                }
                log_printf ("        prev_var . . . . . . : 0x%x", ptr_var->prev_var);
                log_printf ("        next_var . . . . . . : 0x%x", ptr_var->next_var);
            }
        }
    }
}
