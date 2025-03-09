/*
 * plugin-config.c - plugin configuration options (file plugins.conf)
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../core/weechat.h"
#include "../core/core-config.h"
#include "../core/core-hook.h"
#include "../core/core-list.h"
#include "../core/core-log.h"
#include "../core/core-string.h"
#include "plugin-config.h"
#include "weechat-plugin.h"


struct t_config_file *plugin_config_file = NULL;
struct t_config_section *plugin_config_section_var = NULL;
struct t_config_section *plugin_config_section_desc = NULL;


/*
 * Searches for a plugin option.
 */

struct t_config_option *
plugin_config_search (const char *plugin_name, const char *option_name)
{
    char *option_full_name;
    struct t_config_option *ptr_option;

    if (!plugin_name || !option_name)
        return NULL;

    ptr_option = NULL;

    if (string_asprintf (&option_full_name,
                         "%s.%s",
                         plugin_name, option_name) >= 0)
    {
        ptr_option = config_file_search_option (plugin_config_file,
                                                plugin_config_section_var,
                                                option_full_name);
        free (option_full_name);
    }

    return ptr_option;
}

/*
 * Sets value for a plugin option (this function must not be called directly).
 */

int
plugin_config_set_internal (const char *option, const char *value)
{
    int rc;
    struct t_config_option *ptr_option;

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
            NULL, 0, 0, "", value, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        rc = (ptr_option) ? WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
    }

    return rc;
}

/*
 * Sets value for a plugin option (option is created if not found).
 */

int
plugin_config_set (const char *plugin_name, const char *option_name,
                   const char *value)
{
    int rc;
    char *option_full_name;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    if (string_asprintf (&option_full_name,
                         "%s.%s",
                         plugin_name, option_name) >= 0)
    {
        rc = plugin_config_set_internal (option_full_name, value);
        free (option_full_name);
    }

    return rc;
}

/*
 * Callback for changes on a description option.
 */

void
plugin_config_desc_changed_cb (const void *pointer, void *data,
                               struct t_config_option *option)
{
    struct t_config_option *ptr_option;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_option = config_file_search_option (plugin_config_file,
                                            plugin_config_section_var,
                                            option->name);
    if (ptr_option)
    {
        if (ptr_option->description)
        {
            free (ptr_option->description);
            ptr_option->description = NULL;
        }
        if (option->value)
            ptr_option->description = strdup (option->value);
    }
}

/*
 * Sets description for a plugin option (this function must not be called
 * directly).
 */

void
plugin_config_set_desc_internal (const char *option, const char *value)
{
    struct t_config_option *ptr_option;

    ptr_option = config_file_search_option (plugin_config_file,
                                            plugin_config_section_desc,
                                            option);
    if (ptr_option)
    {
        config_file_option_set (ptr_option, value, 1);
    }
    else
    {
        ptr_option = config_file_new_option (
            plugin_config_file, plugin_config_section_desc,
            option, "string", _("description of plugin option"),
            NULL, 0, 0, "", value, 0,
            NULL, NULL, NULL,
            &plugin_config_desc_changed_cb, NULL, NULL,
            NULL, NULL, NULL);
    }

    if (ptr_option)
        plugin_config_desc_changed_cb (NULL, NULL, ptr_option);
}

/*
 * Sets description for a plugin option.
 */

void
plugin_config_set_desc (const char *plugin_name, const char *option_name,
                        const char *description)
{
    char *option_full_name;

    if (string_asprintf (&option_full_name,
                         "%s.%s",
                         plugin_name, option_name) >= 0)
    {
        plugin_config_set_desc_internal (option_full_name, description);
        free (option_full_name);
    }
}

/*
 * Reloads plugins configuration file.
 */

int
plugin_config_reload (const void *pointer, void *data,
                      struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    /* remove all plugin options and descriptions */
    config_file_section_free_options (plugin_config_section_var);
    config_file_section_free_options (plugin_config_section_desc);

    /* reload plugins configuration file */
    return config_file_reload (config_file);
}

/*
 * Sets a plugin option.
 */

int
plugin_config_create_option (const void *pointer, void *data,
                             struct t_config_file *config_file,
                             struct t_config_section *section,
                             const char *option_name, const char *value)
{
    struct t_config_option *ptr_option_desc, *ptr_option;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_option_desc = config_file_search_option (config_file,
                                                 plugin_config_section_desc,
                                                 option_name);

    ptr_option = config_file_new_option (
        config_file, section,
        option_name, "string",
        (ptr_option_desc) ? CONFIG_STRING(ptr_option_desc) : NULL,
        NULL, 0, 0, "", value, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    return (ptr_option) ?
        WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
}

/*
 * Sets a plugin option description.
 */

int
plugin_config_create_desc (const void *pointer, void *data,
                           struct t_config_file *config_file,
                           struct t_config_section *section,
                           const char *option_name, const char *value)
{
    struct t_config_option *ptr_option_var, *ptr_option;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_option_var = config_file_search_option (config_file,
                                                plugin_config_section_var,
                                                option_name);
    if (ptr_option_var)
    {
        if (ptr_option_var->description)
        {
            free (ptr_option_var->description);
            ptr_option_var->description = NULL;
        }
        if (value)
            ptr_option_var->description = strdup (value);
    }

    ptr_option = config_file_new_option (
        config_file, section,
        option_name, "string", _("description of plugin option"),
        NULL, 0, 0, "", value, 0,
        NULL, NULL, NULL,
        &plugin_config_desc_changed_cb, NULL, NULL,
        NULL, NULL, NULL);

    return (ptr_option) ?
        WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
}

/*
 * Deletes a plugin option description.
 */

int
plugin_config_delete_desc (const void *pointer, void *data,
                           struct t_config_file *config_file,
                           struct t_config_section *section,
                           struct t_config_option *option)
{
    struct t_config_option *ptr_option_var;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) section;

    ptr_option_var = config_file_search_option (config_file,
                                                plugin_config_section_var,
                                                option->name);
    if (ptr_option_var)
    {
        if (ptr_option_var->description)
        {
            free (ptr_option_var->description);
            ptr_option_var->description = NULL;
        }
    }

    config_file_option_free (option, 1);

    return WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
}

/*
 * Initializes plugins configuration structure.
 */

void
plugin_config_init (void)
{
    plugin_config_file = config_file_new (NULL, PLUGIN_CONFIG_PRIO_NAME,
                                          &plugin_config_reload, NULL, NULL);
    if (plugin_config_file)
    {
        plugin_config_section_var = config_file_new_section (
            plugin_config_file, "var", 1, 1,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            &plugin_config_create_option, NULL, NULL,
            NULL, NULL, NULL);
        plugin_config_section_desc = config_file_new_section (
            plugin_config_file, "desc", 1, 1,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            &plugin_config_create_desc, NULL, NULL,
            &plugin_config_delete_desc, NULL, NULL);
    }
}

/*
 * Reads plugins configuration file.
 */

int
plugin_config_read (void)
{
    return config_file_read (plugin_config_file);
}

/*
 * Writes plugins configuration file.
 */

int
plugin_config_write (void)
{
    return config_file_write (plugin_config_file);
}

/*
 * Ends plugin configuration.
 */

void
plugin_config_end (void)
{
    /* free all plugin configuration options and descriptions */
    config_file_section_free_options (plugin_config_section_var);
    config_file_section_free_options (plugin_config_section_desc);
}
