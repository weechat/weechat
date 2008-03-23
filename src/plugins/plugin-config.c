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

/* plugin-config.c: plugin configuration */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hook.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "plugin-config.h"


struct t_config_file *plugin_config = NULL;
struct t_config_option *plugin_options = NULL;
struct t_config_option *last_plugin_option = NULL;


/*
 * plugin_config_search_internal: search a plugin option (internal function)
 *                                This function should not be called directly.
 */

struct t_config_option *
plugin_config_search_internal (char *option_name)
{
    struct t_config_option *ptr_option;
    
    for (ptr_option = plugin_options; ptr_option;
         ptr_option = ptr_option->next_option)
    {
        if (string_strcasecmp (ptr_option->name, option_name) == 0)
        {
            return ptr_option;
        }
    }
    
    /* plugin option not found */
    return NULL;
}

/*
 * plugin_config_search: search a plugin option
 */

struct t_config_option *
plugin_config_search (char *plugin_name, char *option_name)
{
    char *internal_option;
    struct t_config_option *ptr_option;
    
    internal_option = malloc (strlen (plugin_name) + strlen (option_name) + 2);
    if (!internal_option)
        return NULL;
    
    strcpy (internal_option, plugin_name);
    strcat (internal_option, ".");
    strcat (internal_option, option_name);
    
    ptr_option = plugin_config_search_internal (internal_option);
    
    free (internal_option);
    
    return ptr_option;
}

/*
 * plugin_config_find_pos: find position for a plugin option (for sorting options)
 */

struct t_config_option *
plugin_config_find_pos (char *name)
{
    struct t_config_option *ptr_option;
    
    for (ptr_option = plugin_options; ptr_option;
         ptr_option = ptr_option->next_option)
    {
        if (string_strcasecmp (name, ptr_option->name) < 0)
            return ptr_option;
    }
    
    /* position not found (we will add to the end of list) */
    return NULL;
}

/*
 * plugin_config_set_internal: set value for a plugin option (internal function)
 *                             This function should not be called directly.
 *                             Return: 1 if ok, 0 if error
 */

int
plugin_config_set_internal (char *option, char *value)
{
    struct t_config_option *ptr_option, *pos_option;
    int rc;

    rc = 0;
    
    ptr_option = plugin_config_search_internal (option);
    if (ptr_option)
    {
        if (!value || !value[0])
        {
            /* remove option from list */
            if (ptr_option->prev_option)
                (ptr_option->prev_option)->next_option =
                    ptr_option->next_option;
            else
                plugin_options = ptr_option->next_option;
            if (ptr_option->next_option)
                (ptr_option->next_option)->prev_option =
                    ptr_option->prev_option;
            rc = 1;
        }
        else
        {
            /* replace old value by new one */
            if (ptr_option->value)
                free (ptr_option->value);
            ptr_option->value = strdup (value);
            rc = 1;
        }
    }
    else
    {
        if (value && value[0])
        {
            ptr_option = malloc (sizeof (*ptr_option));
            if (ptr_option)
            {
                /* create new option */
                ptr_option->name = strdup (option);
                string_tolower (ptr_option->name);
                ptr_option->type = CONFIG_OPTION_STRING;
                ptr_option->description = NULL;
                ptr_option->string_values = NULL;
                ptr_option->min = 0;
                ptr_option->max = 0;
                ptr_option->default_value = NULL;
                ptr_option->value = strdup (value);
                ptr_option->callback_change = NULL;
                
                if (plugin_options)
                {
                    pos_option = plugin_config_find_pos (ptr_option->name);
                    if (pos_option)
                    {
                        /* insert option into the list (before option found) */
                        ptr_option->prev_option = pos_option->prev_option;
                        ptr_option->next_option = pos_option;
                        if (pos_option->prev_option)
                            pos_option->prev_option->next_option = ptr_option;
                        else
                            plugin_options = ptr_option;
                        pos_option->prev_option = ptr_option;
                    }
                    else
                    {
                        /* add option to the end */
                        ptr_option->prev_option = last_plugin_option;
                        ptr_option->next_option = NULL;
                        last_plugin_option->next_option = ptr_option;
                        last_plugin_option = ptr_option;
                    }
                }
                else
                {
                    ptr_option->prev_option = NULL;
                    ptr_option->next_option = NULL;
                    plugin_options = ptr_option;
                    last_plugin_option = ptr_option;
                }
                rc = 1;
            }
        }
        else
            rc = 0;
    }
    
    if (rc)
        hook_config_exec ("plugin", option, value);
    
    return rc;
}

/*
 * plugin_config_set: set value for a plugin option (create it if not found)
 *                    return: 1 if ok, 0 if error
 */

int
plugin_config_set (char *plugin_name, char *option_name, char *value)
{
    char *internal_option;
    int return_code;
    
    internal_option = malloc (strlen (plugin_name) + strlen (option_name) + 2);
    if (!internal_option)
        return 0;
    
    strcpy (internal_option, plugin_name);
    strcat (internal_option, ".");
    strcat (internal_option, option_name);
    
    return_code = plugin_config_set_internal (internal_option, value);
    free (internal_option);
    
    return return_code;
}

/*
 * plugin_config_free: free a plugin option and remove it from list
 */

void
plugin_config_free (struct t_config_option *option)
{
    struct t_config_option *new_plugin_options;
    
    /* remove option from list */
    if (last_plugin_option == option)
        last_plugin_option = option->prev_option;
    if (option->prev_option)
    {
        (option->prev_option)->next_option = option->next_option;
        new_plugin_options = plugin_options;
    }
    else
        new_plugin_options = option->next_option;
    
    if (option->next_option)
        (option->next_option)->prev_option = option->prev_option;
    
    /* free option */
    config_file_option_free_data (option);
    free (option);
    
    plugin_options = new_plugin_options;
}

/*
 * plugin_config_free_all: free all plugin options
 */

void
plugin_config_free_all ()
{
    while (plugin_options)
    {
        plugin_config_free (plugin_options);
    }
}

/*
 * plugin_config_reload: reload plugins configuration file
 *                       return:  0 = successful
 *                               -1 = config file file not found
 *                               -2 = error in config file
 */

int
plugin_config_reload (void *data, struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) data;
    (void) config_file;
    
    /* remove all plugin options */
    plugin_config_free_all ();
    
    /* reload plugins config file */
    return config_file_reload (plugin_config);
}

/*
 * plugin_config_read_option: read an option in config file
 *                            Return:  0 = successful
 *                                    -1 = option not found
 *                                    -2 = bad format/value
 */

void
plugin_config_read_option (void *data, struct t_config_file *config_file,
                           char *option_name, char *value)
{
    char *value2;
    
    /* make C compiler happy */
    (void) data;
    (void) config_file;

    if (option_name && value)
    {
        value2 = string_iconv_to_internal (NULL, value);
        plugin_config_set_internal (option_name,
                                    (value2) ? value2 : value);
        if (value2)
            free (value2);
    }
}

/*
 * plugin_config_write_options: write plugin options in configuration file
 */

void
plugin_config_write_options (void *data, struct t_config_file *config_file,
                             char *section_name)
{
    struct t_config_option *ptr_option;
    
    /* make C compiler happy */
    (void) data;
    
    config_file_write_line (config_file, section_name, NULL);
    
    for (ptr_option = plugin_options; ptr_option;
         ptr_option = ptr_option->next_option)
    {
        config_file_write_line (config_file,
                                ptr_option->name,
                                "%s", ptr_option->value);
    }
}

/*
 * plugin_config_init: init plugins config structure
 */

void
plugin_config_init ()
{
    plugin_config = config_file_new (NULL, PLUGIN_CONFIG_FILENAME,
                                     &plugin_config_reload, NULL);
    if (plugin_config)
    {
        config_file_new_section (plugin_config, "plugins",
                                 &plugin_config_read_option, NULL,
                                 &plugin_config_write_options, NULL,
                                 NULL, NULL);
    }
}

/*
 * plugin_config_read: read plugins configuration file
 *                     return:  0 = successful
 *                             -1 = config file file not found
 *                             -2 = error in config file
 */

int
plugin_config_read ()
{
    return config_file_read (plugin_config);
}

/*
 * plugin_config_write: write plugins configuration file
 *                      return:  0 if ok
 *                             < 0 if error
 */

int
plugin_config_write ()
{
    log_printf (_("Saving plugins configuration to disk"));
    return config_file_write (plugin_config);
}

/*
 * plugin_config_end: end plugin config
 */

void
plugin_config_end ()
{
    /* free all plugin config options */
    plugin_config_free_all ();
}
