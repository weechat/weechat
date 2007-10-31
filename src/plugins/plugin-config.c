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


struct t_plugin_option *plugin_options = NULL;
struct t_plugin_option *last_plugin_option = NULL;

char *plugin_config_sections[] =
{ "plugin", NULL };

struct t_config_option *plugin_config_options[] =
{ NULL, NULL };

t_config_func_read_option *plugin_config_read_functions[] =
{ plugin_config_read_option, NULL };

t_config_func_write_options *plugin_config_write_functions[] =
{ plugin_config_write_options, NULL };


/*
 * plugin_config_search_internal: search a plugin option (internal function)
 *                                This function should not be called directly.
 */

struct t_plugin_option *
plugin_config_search_internal (char *option)
{
    struct t_plugin_option *ptr_plugin_option;
    
    for (ptr_plugin_option = plugin_options; ptr_plugin_option;
         ptr_plugin_option = ptr_plugin_option->next_option)
    {
        if (string_strcasecmp (ptr_plugin_option->name, option) == 0)
        {
            return ptr_plugin_option;
        }
    }
    
    /* plugin option not found */
    return NULL;
}

/*
 * plugin_config_search: search a plugin option
 */

struct t_plugin_option *
plugin_config_search (char *plugin_name, char *option)
{
    char *internal_option;
    struct t_plugin_option *ptr_plugin_option;
    
    internal_option = (char *)malloc (strlen (plugin_name) +
                                      strlen (option) + 2);
    if (!internal_option)
        return NULL;
    
    strcpy (internal_option, plugin_name);
    strcat (internal_option, ".");
    strcat (internal_option, option);
    
    ptr_plugin_option = plugin_config_search_internal (internal_option);
    
    free (internal_option);
    
    return ptr_plugin_option;
}

/*
 * plugin_config_find_pos: find position for a plugin option (for sorting options)
 */

struct t_plugin_option *
plugin_config_find_pos (char *name)
{
    struct t_plugin_option *ptr_option;
    
    for (ptr_option = plugin_options; ptr_option;
         ptr_option = ptr_option->next_option)
    {
        if (string_strcasecmp (name, ptr_option->name) < 0)
            return ptr_option;
    }
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
    struct t_plugin_option *ptr_plugin_option, *pos_option;
    int rc;

    rc = 0;
    
    ptr_plugin_option = plugin_config_search_internal (option);
    if (ptr_plugin_option)
    {
        if (!value || !value[0])
        {
            /* remove option from list */
            if (ptr_plugin_option->prev_option)
                (ptr_plugin_option->prev_option)->next_option =
                    ptr_plugin_option->next_option;
            else
                plugin_options = ptr_plugin_option->next_option;
            if (ptr_plugin_option->next_option)
                (ptr_plugin_option->next_option)->prev_option =
                    ptr_plugin_option->prev_option;
            rc = 1;
        }
        else
        {
            /* replace old value by new one */
            if (ptr_plugin_option->value)
                free (ptr_plugin_option->value);
            ptr_plugin_option->value = strdup (value);
            rc = 1;
        }
    }
    else
    {
        if (value && value[0])
        {
            ptr_plugin_option = (struct t_plugin_option *)malloc (sizeof (struct t_plugin_option));
            if (ptr_plugin_option)
            {
                /* create new option */
                ptr_plugin_option->name = strdup (option);
                string_tolower (ptr_plugin_option->name);
                ptr_plugin_option->value = strdup (value);
                
                if (plugin_options)
                {
                    pos_option = plugin_config_find_pos (ptr_plugin_option->name);
                    if (pos_option)
                    {
                        /* insert option into the list (before option found) */
                        ptr_plugin_option->prev_option = pos_option->prev_option;
                        ptr_plugin_option->next_option = pos_option;
                        if (pos_option->prev_option)
                            pos_option->prev_option->next_option = ptr_plugin_option;
                        else
                            plugin_options = ptr_plugin_option;
                        pos_option->prev_option = ptr_plugin_option;
                    }
                    else
                    {
                        /* add option to the end */
                        ptr_plugin_option->prev_option = last_plugin_option;
                        ptr_plugin_option->next_option = NULL;
                        last_plugin_option->next_option = ptr_plugin_option;
                        last_plugin_option = ptr_plugin_option;
                    }
                }
                else
                {
                    ptr_plugin_option->prev_option = NULL;
                    ptr_plugin_option->next_option = NULL;
                    plugin_options = ptr_plugin_option;
                    last_plugin_option = ptr_plugin_option;
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
plugin_config_set (char *plugin_name, char *option, char *value)
{
    char *internal_option;
    int return_code;
    
    internal_option = (char *)malloc (strlen (plugin_name) +
                                      strlen (option) + 2);
    if (!internal_option)
        return 0;
    
    strcpy (internal_option, plugin_name);
    strcat (internal_option, ".");
    strcat (internal_option, option);
    
    return_code = plugin_config_set_internal (internal_option, value);
    free (internal_option);
    
    return return_code;
}

/*
 * plugin_config_read_option: read an option in config file
 *                            Return:  0 = successful
 *                                    -1 = option not found
 *                                    -2 = bad format/value
 */

int
plugin_config_read_option (struct t_config_option *options,
                           char *option_name, char *value)
{
    char *value2;
    
    /* make C compiler happy */
    (void) options;
    
    if (option_name)
    {
        value2 = string_iconv_to_internal (NULL, value);
        plugin_config_set_internal (option_name,
                                    (value2) ? value2 : value);
        if (value2)
            free (value2);
    }
    else
    {
        /* does nothing for new [plugin] section */
    }
    
    /* all ok */
    return 0;
} 

/*
 * plugin_config_read: read WeeChat plugins configuration file
 *                     return:  0 = successful
 *                             -1 = config file file not found
 *                             -2 = error in config file
 */

int
plugin_config_read ()
{
    return config_file_read (plugin_config_sections, plugin_config_options,
                             plugin_config_read_functions,
                             plugin_config_read_option,
                             NULL,
                             WEECHAT_PLUGIN_CONFIG_NAME);
}

/*
 * plugin_config_write_options: write plugin options in configuration file
 *                              Return:  0 = successful
 *                                      -1 = write error
 */

int
plugin_config_write_options (FILE *file, char *section_name,
                             struct t_config_option *options)
{
    /* make C compiler happy */
    (void) options;
    
    struct t_plugin_option *ptr_plugin_option;
    
    string_iconv_fprintf (file, "\n[%s]\n", section_name);
    
    for (ptr_plugin_option = plugin_options; ptr_plugin_option;
         ptr_plugin_option = ptr_plugin_option->next_option)
    {
        string_iconv_fprintf (file, "%s = \"%s\"\n",
                              ptr_plugin_option->name,
                              ptr_plugin_option->value);
    }
    
    /* all ok */
    return 0;
}

/*
 * plugin_config_write: write WeeChat configuration file
 *                      return:  0 if ok
 *                             < 0 if error
 */

int
plugin_config_write ()
{
    weechat_log_printf (_("Saving plugins configuration to disk\n"));
    return config_file_write (plugin_config_sections, plugin_config_options,
                              plugin_config_write_functions,
                              WEECHAT_PLUGIN_CONFIG_NAME);
}
