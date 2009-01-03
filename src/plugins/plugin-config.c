/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
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
#include "../core/wee-list.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "plugin-config.h"
#include "weechat-plugin.h"


struct t_config_file *plugin_config_file = NULL;
struct t_config_section *plugin_config_section_var = NULL;


/*
 * plugin_config_search: search a plugin option
 */

struct t_config_option *
plugin_config_search (const char *plugin_name, const char *option_name)
{
    int length;
    char *option_full_name;
    struct t_config_option *ptr_option;
    
    ptr_option = NULL;
    
    length = strlen (plugin_name) + 1 + strlen (option_name) + 1;
    option_full_name = malloc (length);
    if (option_full_name)
    {
        snprintf (option_full_name, length, "%s.%s",
                  plugin_name, option_name);
        ptr_option = config_file_search_option (plugin_config_file,
                                                plugin_config_section_var,
                                                option_full_name);
        free (option_full_name);
    }
    
    return ptr_option;
}

/*
 * plugin_config_set_internal: set value for a plugin option (internal function)
 *                             This function should not be called directly.
 */

int
plugin_config_set_internal (const char *option, const char *value)
{
    int rc;
    struct t_config_option *ptr_option;
    
    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
    
    ptr_option = config_file_search_option (plugin_config_file,
                                            plugin_config_section_var,
                                            option);
    if (ptr_option)
    {
        rc = config_file_option_set (ptr_option, value, 0);
    }
    else
    {
        ptr_option = config_file_new_option (
            plugin_config_file, plugin_config_section_var,
            option, "string", NULL,
            NULL, 0, 0, "", value, 0, NULL, NULL, NULL, NULL, NULL, NULL);
        rc = (ptr_option) ? WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
    }
    
    return rc;
}

/*
 * plugin_config_set: set value for a plugin option (create it if not found)
 */

int
plugin_config_set (const char *plugin_name, const char *option_name,
                   const char *value)
{
    int length, rc;
    char *option_full_name;
    
    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
    
    length = strlen (plugin_name) + 1 + strlen (option_name) + 1;
    option_full_name = malloc (length);
    if (option_full_name)
    {
        snprintf (option_full_name, length, "%s.%s",
                  plugin_name, option_name);
        string_tolower (option_full_name);
        rc = plugin_config_set_internal (option_full_name, value);
        free (option_full_name);
    }
    
    return rc;
}

/*
 * plugin_config_reload: reload plugins configuration file
 */

int
plugin_config_reload (void *data, struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) data;
    
    /* remove all plugin options */
    config_file_section_free_options (plugin_config_section_var);
    
    /* reload plugins config file */
    return config_file_reload (config_file);
}

/*
 * plugin_config_create_option: set plugin option
 */

int
plugin_config_create_option (void *data, struct t_config_file *config_file,
                             struct t_config_section *section,
                             const char *option_name, const char *value)
{
    struct t_config_option *ptr_option;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_option = config_file_new_option (
        config_file, section,
        option_name, "string", NULL,
        NULL, 0, 0, "", value, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    
    return (ptr_option) ?
        WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
}

/*
 * plugin_config_init: init plugins config structure
 */

void
plugin_config_init ()
{
    plugin_config_file = config_file_new (NULL, PLUGIN_CONFIG_NAME,
                                          &plugin_config_reload, NULL);
    if (plugin_config_file)
    {
        plugin_config_section_var = config_file_new_section (
            plugin_config_file, "var", 1, 1,
            NULL, NULL,
            NULL, NULL,
            NULL, NULL,
            &plugin_config_create_option, NULL,
            NULL, NULL);
    }
    else
        plugin_config_section_var = NULL;
}

/*
 * plugin_config_read: read plugins configuration file
 */

int
plugin_config_read ()
{
    return config_file_read (plugin_config_file);
}

/*
 * plugin_config_write: write plugins configuration file
 */

int
plugin_config_write ()
{
    return config_file_write (plugin_config_file);
}

/*
 * plugin_config_end: end plugin config
 */

void
plugin_config_end ()
{
    /* free all plugin config options */
    config_file_section_free_options (plugin_config_section_var);
}
