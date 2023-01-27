/*
 * gui-bar-item-custom.c - custom bar item functions (used by all GUI)
 *
 * Copyright (C) 2022-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stddef.h>
#include <string.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-config-file.h"
#include "../core/wee-eval.h"
#include "../core/wee-hashtable.h"
#include "../core/wee-string.h"
#include "../plugins/plugin.h"
#include "gui-bar-item-custom.h"
#include "gui-bar-item.h"


char *gui_bar_item_custom_option_string[GUI_BAR_ITEM_CUSTOM_NUM_OPTIONS] =
{ "conditions", "content" };
char *gui_bar_item_custom_option_default[GUI_BAR_ITEM_CUSTOM_NUM_OPTIONS] =
{ "", "" };


struct t_gui_bar_item_custom *gui_custom_bar_items = NULL;
struct t_gui_bar_item_custom *last_gui_custom_bar_item = NULL;

/* custom bar items used when reading config */
struct t_gui_bar_item_custom *gui_temp_custom_bar_items = NULL;
struct t_gui_bar_item_custom *last_gui_temp_custom_bar_item = NULL;


/*
 * Checks if a custom bar item name is valid: it must not have any
 * space/period.
 *
 * Returns:
 *   1: name is valid
 *   0: name is invalid
 */

int
gui_bar_item_custom_name_valid (const char *name)
{
    if (!name || !name[0])
        return 0;

    /* no spaces allowed */
    if (strchr (name, ' '))
        return 0;

    /* no periods allowed */
    if (strchr (name, '.'))
        return 0;

    /* name is valid */
    return 1;
}

/*
 * Searches for a custom bar item option name.
 *
 * Returns index of option in enum t_gui_bar_item_custom_option,
 * -1 if not found.
 */

int
gui_bar_item_custom_search_option (const char *option_name)
{
    int i;

    if (!option_name)
        return -1;

    for (i = 0; i < GUI_BAR_ITEM_CUSTOM_NUM_OPTIONS; i++)
    {
        if (strcmp (gui_bar_item_custom_option_string[i], option_name) == 0)
            return i;
    }

    /* custom bar item option not found */
    return -1;
}

/*
 * Searches for a custom bar item by name.
 */

struct t_gui_bar_item_custom *
gui_bar_item_custom_search (const char *item_name)
{
    struct t_gui_bar_item_custom *ptr_item;

    if (!item_name || !item_name[0])
        return NULL;

    for (ptr_item = gui_custom_bar_items; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        if (strcmp (ptr_item->name, item_name) == 0)
            return ptr_item;
    }

    /* custom bar item not found */
    return NULL;
}

/*
 * Searches for a custom bar item with name of option (like "name.content").
 *
 * Returns pointer to custom bar item found, NULL if not found.
 */

struct t_gui_bar_item_custom *
gui_bar_item_custom_search_with_option_name (const char *option_name)
{
    char *item_name, *pos_option;
    struct t_gui_bar_item_custom *ptr_item;

    if (!option_name)
        return NULL;

    ptr_item = NULL;

    pos_option = strchr (option_name, '.');
    if (pos_option)
    {
        item_name = string_strndup (option_name, pos_option - option_name);
        if (item_name)
        {
            for (ptr_item = gui_custom_bar_items; ptr_item;
                 ptr_item = ptr_item->next_item)
            {
                if (strcmp (ptr_item->name, item_name) == 0)
                    break;
            }
            free (item_name);
        }
    }

    return ptr_item;
}

/*
 * Callback called when option "conditions" or "content" is changed.
 */

void
gui_bar_item_custom_config_change (const void *pointer, void *data,
                                   struct t_config_option *option)
{
    struct t_gui_bar_item_custom *ptr_item;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_item = gui_bar_item_custom_search_with_option_name (option->name);
    if (ptr_item)
        gui_bar_item_update (ptr_item->name);
}

/*
 * Creates an option for a custom bar item.
 *
 * Returns pointer to new option, NULL if error.
 */

struct t_config_option *
gui_bar_item_custom_create_option (const char *item_name, int index_option,
                                   const char *value)
{
    struct t_config_option *ptr_option;
    int length;
    char *option_name;

    ptr_option = NULL;

    length = strlen (item_name) + 1 +
        strlen (gui_bar_item_custom_option_string[index_option]) + 1;
    option_name = malloc (length);
    if (!option_name)
        return NULL;

    snprintf (option_name, length, "%s.%s",
              item_name, gui_bar_item_custom_option_string[index_option]);

    switch (index_option)
    {
        case GUI_BAR_ITEM_CUSTOM_OPTION_CONDITIONS:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_custom_bar_item,
                option_name, "string",
                N_("condition(s) to display the bar item "
                   "(evaluated, see /help eval)"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, NULL,
                &gui_bar_item_custom_config_change, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case GUI_BAR_ITEM_CUSTOM_OPTION_CONTENT:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_custom_bar_item,
                option_name, "string",
                N_("content of bar item (evaluated, see /help eval)"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, NULL,
                &gui_bar_item_custom_config_change, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case GUI_BAR_ITEM_CUSTOM_NUM_OPTIONS:
            break;
    }

    free (option_name);

    return ptr_option;
}

/*
 * Creates option for a temporary custom bar item (when reading configuration
 * file).
 */

void
gui_bar_item_custom_create_option_temp (struct t_gui_bar_item_custom *temp_item,
                                        int index_option,
                                        const char *value)
{
    struct t_config_option *new_option;

    new_option = gui_bar_item_custom_create_option (temp_item->name,
                                                    index_option,
                                                    value);
    if (new_option
        && (index_option >= 0)
        && (index_option < GUI_BAR_ITEM_CUSTOM_NUM_OPTIONS))
    {
        temp_item->options[index_option] = new_option;
    }
}

/*
 * Custom bar item callback.
 */

char *
gui_bar_item_custom_callback (const void *pointer,
                              void *data,
                              struct t_gui_bar_item *item,
                              struct t_gui_window *window,
                              struct t_gui_buffer *buffer,
                              struct t_hashtable *extra_info)
{
    struct t_gui_bar_item_custom *ptr_item;
    struct t_hashtable *pointers, *options;
    const char *ptr_conditions;
    char *result;
    int rc;

    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) extra_info;

    result = NULL;

    ptr_item = (struct t_gui_bar_item_custom *)pointer;
    if (!ptr_item)
        return NULL;

    pointers = hashtable_new (32,
                              WEECHAT_HASHTABLE_STRING,
                              WEECHAT_HASHTABLE_POINTER,
                              NULL,
                              NULL);
    if (pointers)
    {
        hashtable_set (pointers, "window", window);
        hashtable_set (pointers, "buffer", buffer);
    }

    options = hashtable_new (32,
                             WEECHAT_HASHTABLE_STRING,
                             WEECHAT_HASHTABLE_STRING,
                             NULL,
                             NULL);
    if (options)
        hashtable_set (options, "type", "condition");

    /* check conditions */
    ptr_conditions = CONFIG_STRING(ptr_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONDITIONS]);
    if (ptr_conditions && ptr_conditions[0])
    {
        result = eval_expression (
            CONFIG_STRING(ptr_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONDITIONS]),
            pointers, NULL, options);
        rc = eval_is_true (result);
        if (result)
        {
            free (result);
            result = NULL;
        }
        if (!rc)
            goto end;
    }

    /* evaluate content */
    result = eval_expression (
        CONFIG_STRING(ptr_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONTENT]),
        pointers, NULL, NULL);

end:
    if (pointers)
        hashtable_free (pointers);
    if (options)
        hashtable_free (options);

    return result;
}

/*
 * Allocates and initializes new custom bar item structure.
 *
 * Returns pointer to new custom bar item, NULL if error.
 */

struct t_gui_bar_item_custom *
gui_bar_item_custom_alloc (const char *name)
{
    struct t_gui_bar_item_custom *new_bar_item_custom;
    int i;

    new_bar_item_custom = malloc (sizeof (*new_bar_item_custom));
    if (!new_bar_item_custom)
        return NULL;

    new_bar_item_custom->name = strdup (name);
    for (i = 0; i < GUI_BAR_ITEM_CUSTOM_NUM_OPTIONS; i++)
    {
        new_bar_item_custom->options[i] = NULL;
    }
    new_bar_item_custom->bar_item = NULL;
    new_bar_item_custom->prev_item = NULL;
    new_bar_item_custom->next_item = NULL;

    return new_bar_item_custom;
}

/*
 * Creates bar item in a custom bar item.
 */

void
gui_bar_item_custom_create_bar_item (struct t_gui_bar_item_custom *item)
{
    if (item->bar_item)
        gui_bar_item_free (item->bar_item);
    item->bar_item = gui_bar_item_new (
        NULL,
        item->name,
        &gui_bar_item_custom_callback,
        item,
        NULL);
}

/*
 * Creates a new custom bar item with options.
 *
 * Returns pointer to new bar, NULL if error.
 */

struct t_gui_bar_item_custom *
gui_bar_item_custom_new_with_options (const char *name,
                                      struct t_config_option *conditions,
                                      struct t_config_option *content)
{
    struct t_gui_bar_item_custom *new_bar_item_custom;

    /* create custom bar item */
    new_bar_item_custom = gui_bar_item_custom_alloc (name);
    if (!new_bar_item_custom)
        return NULL;

    new_bar_item_custom->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONDITIONS] = conditions;
    new_bar_item_custom->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONTENT] = content;
    new_bar_item_custom->bar_item = NULL;

    /* add custom bar item to custom bar items queue */
    new_bar_item_custom->prev_item = last_gui_custom_bar_item;
    if (last_gui_custom_bar_item)
        last_gui_custom_bar_item->next_item = new_bar_item_custom;
    else
        gui_custom_bar_items = new_bar_item_custom;
    last_gui_custom_bar_item = new_bar_item_custom;
    new_bar_item_custom->next_item = NULL;

    return new_bar_item_custom;
}

/*
 * Creates a new custom bar item.
 *
 * Returns pointer to new custom bar item, NULL if not found.
 */

struct t_gui_bar_item_custom *
gui_bar_item_custom_new (const char *name, const char *conditions,
                         const char *content)
{
    struct t_config_option *option_conditions, *option_content;
    struct t_gui_bar_item_custom *new_bar_item_custom;

    if (!gui_bar_item_custom_name_valid (name))
        return NULL;

    if (gui_bar_item_custom_search (name))
        return NULL;

    option_conditions = gui_bar_item_custom_create_option (
        name,
        GUI_BAR_ITEM_CUSTOM_OPTION_CONDITIONS,
        conditions);
    option_content = gui_bar_item_custom_create_option (
        name,
        GUI_BAR_ITEM_CUSTOM_OPTION_CONTENT,
        content);

    new_bar_item_custom = gui_bar_item_custom_new_with_options (
        name,
        option_conditions,
        option_content);
    if (new_bar_item_custom)
    {
        gui_bar_item_custom_create_bar_item (new_bar_item_custom);
        gui_bar_item_update (name);
    }
    else
    {
        if (option_conditions)
            config_file_option_free (option_conditions, 0);
        if (option_content)
            config_file_option_free (option_content, 0);
    }

    return new_bar_item_custom;
}

/*
 * Uses temporary custom bar items (created by reading configuration file).
 */

void
gui_bar_item_custom_use_temp_items ()
{
    struct t_gui_bar_item_custom *ptr_temp_item;
    int i;

    for (ptr_temp_item = gui_temp_custom_bar_items; ptr_temp_item;
         ptr_temp_item = ptr_temp_item->next_item)
    {
        for (i = 0; i < GUI_BAR_ITEM_CUSTOM_NUM_OPTIONS; i++)
        {
            if (!ptr_temp_item->options[i])
            {
                ptr_temp_item->options[i] = gui_bar_item_custom_create_option (
                    ptr_temp_item->name,
                    i,
                    gui_bar_item_custom_option_default[i]);
            }
        }
        gui_bar_item_custom_create_bar_item (ptr_temp_item);
    }

    /* remove any existing custom bar item */
    gui_bar_item_custom_free_all ();

    /* replace custom bar items list by the temporary list */
    gui_custom_bar_items = gui_temp_custom_bar_items;
    last_gui_custom_bar_item = last_gui_temp_custom_bar_item;

    gui_temp_custom_bar_items = NULL;
    last_gui_temp_custom_bar_item = NULL;
}

/*
 * Renames a custom bar item.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_bar_item_custom_rename (struct t_gui_bar_item_custom *item,
                            const char *new_name)
{
    if (!item || !gui_bar_item_custom_name_valid (new_name))
        return 0;

    if (gui_bar_item_custom_search (new_name))
        return 0;

    free (item->bar_item->name);
    item->bar_item->name = strdup (new_name);

    gui_bar_item_update (item->name);
    gui_bar_item_update (item->bar_item->name);

    free (item->name);
    item->name = strdup (new_name);

    return 1;
}

/*
 * Deletes a custom bar item.
 */

void
gui_bar_item_custom_free (struct t_gui_bar_item_custom *item)
{
    char *name;
    int i;

    if (!item)
        return;

    name = strdup (item->name);

    /* remove bar item */
    gui_bar_item_free (item->bar_item);

    /* remove custom bar item from custom bar items list */
    if (item->prev_item)
        (item->prev_item)->next_item = item->next_item;
    if (item->next_item)
        (item->next_item)->prev_item = item->prev_item;
    if (gui_custom_bar_items == item)
        gui_custom_bar_items = item->next_item;
    if (last_gui_custom_bar_item == item)
        last_gui_custom_bar_item = item->prev_item;

    /* free data */
    if (item->name)
        free (item->name);
    for (i = 0; i < GUI_BAR_ITEM_CUSTOM_NUM_OPTIONS; i++)
    {
        if (item->options[i])
            config_file_option_free (item->options[i], 1);
    }

    free (item);

    gui_bar_item_update (name);

    if (name)
        free (name);
}

/*
 * Deletes all custom bar items.
 */

void
gui_bar_item_custom_free_all ()
{
    while (gui_custom_bar_items)
    {
        gui_bar_item_custom_free (gui_custom_bar_items);
    }
}
