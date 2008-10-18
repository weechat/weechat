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

/* wee-config-file.c: manages options in config files */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "weechat.h"
#include "wee-config-file.h"
#include "wee-hook.h"
#include "wee-infolist.h"
#include "wee-log.h"
#include "wee-string.h"
#include "../gui/gui-color.h"
#include "../gui/gui-chat.h"
#include "../plugins/plugin.h"


struct t_config_file *config_files = NULL;
struct t_config_file *last_config_file = NULL;

char *config_option_type_string[CONFIG_NUM_OPTION_TYPES] =
{ N_("boolean"), N_("integer"), N_("string"), N_("color") };
char *config_boolean_true[] = { "on", "yes", "y", "true", "t", "1", NULL };
char *config_boolean_false[] = { "off", "no", "n", "false", "f", "0", NULL };


/*
 * config_file_search: search a configuration file
 */

struct t_config_file *
config_file_search (const char *name)
{
    struct t_config_file *ptr_config;
    
    if (!name)
        return NULL;
    
    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        if (string_strcasecmp (ptr_config->name, name) == 0)
            return ptr_config;
    }
    
    /* configuration file not found */
    return NULL;
}

/*
 * config_file_new: create new config options structure
 */

struct t_config_file *
config_file_new (struct t_weechat_plugin *plugin, const char *name,
                 int (*callback_reload)(void *data,
                                        struct t_config_file *config_file),
                 void *callback_reload_data)
{
    struct t_config_file *new_config_file;
    char *filename;
    int length;
    
    if (!name)
        return NULL;
    
    /* it's NOT authorized to create two config files with same filename */
    if (config_file_search (name))
        return NULL;
    
    new_config_file = malloc (sizeof (*new_config_file));
    if (new_config_file)
    {
        new_config_file->plugin = plugin;
        new_config_file->name = strdup (name);
        length = strlen (name) + 8 + 1;
        filename = malloc (length);
        if (filename)
        {
            snprintf (filename, length, "%s.conf", name);
            new_config_file->filename = strdup (filename);
            free (filename);
        }
        else
            new_config_file->filename = strdup (name);
        new_config_file->file = NULL;
        new_config_file->callback_reload = callback_reload;
        new_config_file->callback_reload_data = callback_reload_data;
        new_config_file->sections = NULL;
        new_config_file->last_section = NULL;
        
        new_config_file->prev_config = last_config_file;
        new_config_file->next_config = NULL;
        if (config_files)
            last_config_file->next_config = new_config_file;
        else
            config_files = new_config_file;
        last_config_file = new_config_file;
    }
    
    return new_config_file;
}

/*
 * config_file_valid_for_plugin: check if a configuration file pointer exists for a plugin
 *                               return 1 if configuration file exists for plugin
 *                                      0 if configuration file is not found for plugin
 */

int
config_file_valid_for_plugin (struct t_weechat_plugin *plugin,
                              struct t_config_file *config_file)
{
    struct t_config_file *ptr_config;
    
    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        if ((ptr_config == config_file)
            && (ptr_config->plugin == plugin))
            return 1;
    }
    
    /* configuration file not found */
    return 0;
}

/*
 * config_file_new_section: create a new section in a config
 */

struct t_config_section *
config_file_new_section (struct t_config_file *config_file, const char *name,
                         int user_can_add_options, int user_can_delete_options,
                         int (*callback_read)(void *data,
                                              struct t_config_file *config_file,
                                              struct t_config_section *section,
                                              const char *option_name,
                                              const char *value),
                         void *callback_read_data,
                         void (*callback_write)(void *data,
                                                struct t_config_file *config_file,
                                                const char *section_name),
                         void *callback_write_data,
                         void (*callback_write_default)(void *data,
                                                        struct t_config_file *config_file,
                                                        const char *section_name),
                         void *callback_write_default_data,
                         int (*callback_create_option)(void *data,
                                                       struct t_config_file *config_file,
                                                       struct t_config_section *section,
                                                       const char *option_name,
                                                       const char *value),
                         void *callback_create_option_data)
{
    struct t_config_section *new_section;
    
    if (!config_file || !name)
        return NULL;
    
    if (config_file_search_section (config_file, name))
        return NULL;
    
    new_section = malloc (sizeof (*new_section));
    if (new_section)
    {
        new_section->name = strdup (name);
        new_section->user_can_add_options = user_can_add_options;
        new_section->user_can_delete_options = user_can_delete_options;
        new_section->callback_read = callback_read;
        new_section->callback_read_data = callback_read_data;
        new_section->callback_write = callback_write;
        new_section->callback_write_data = callback_write_data;
        new_section->callback_write_default = callback_write_default;
        new_section->callback_write_default_data = callback_write_default_data;
        new_section->callback_create_option = callback_create_option;
        new_section->callback_create_option_data = callback_create_option_data;
        new_section->options = NULL;
        new_section->last_option = NULL;
        
        new_section->prev_section = config_file->last_section;
        new_section->next_section = NULL;
        if (config_file->sections)
            config_file->last_section->next_section = new_section;
        else
            config_file->sections = new_section;
        config_file->last_section = new_section;
    }
    
    return new_section;
}

/*
 * config_file_search_section: search a section in a config structure
 */

struct t_config_section *
config_file_search_section (struct t_config_file *config_file,
                            const char *section_name)
{
    struct t_config_section *ptr_section;

    if (!config_file || !section_name)
        return NULL;
    
    for (ptr_section = config_file->sections; ptr_section;
         ptr_section = ptr_section->next_section)
    {
        if (string_strcasecmp (ptr_section->name, section_name) == 0)
            return ptr_section;
    }
    
    /* section not found */
    return NULL;
}

/*
 * config_file_section_valid_for_plugin: check if a section pointer exists for a plugin
 *                                       return 1 if section exists for plugin
 *                                              0 if section is not found for plugin
 */

int
config_file_section_valid_for_plugin (struct t_weechat_plugin *plugin,
                                      struct t_config_section *section)
{
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    
    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        if (ptr_config->plugin == plugin)
        {
            for (ptr_section = ptr_config->sections; ptr_section;
                 ptr_section = ptr_section->next_section)
            {
                if (ptr_section == section)
                    return 1;
            }
        }
    }
    
    /* section not found */
    return 0;
}

/*
 * config_file_option_find_pos: find position for an option in section
 *                              (for sorting options)
 */

struct t_config_option *
config_file_option_find_pos (struct t_config_section *section, const char *name)
{
    struct t_config_option *ptr_option;
    
    for (ptr_option = section->options; ptr_option;
         ptr_option = ptr_option->next_option)
    {
        if (string_strcasecmp (name, ptr_option->name) < 0)
            return ptr_option;
    }
    
    /* position not found (we will add to the end of list) */
    return NULL;
}

/*
 * config_file_option_insert_in_section: insert option in section (for sorting
 *                                       options)
 */

void
config_file_option_insert_in_section (struct t_config_option *option)
{
    struct t_config_option *pos_option;
    
    if (!option->section)
        return;
    
    if (option->section->options)
    {
        pos_option = config_file_option_find_pos (option->section,
                                                  option->name);
        if (pos_option)
        {
            /* insert option into the list (before option found) */
            option->prev_option = pos_option->prev_option;
            option->next_option = pos_option;
            if (pos_option->prev_option)
                (pos_option->prev_option)->next_option = option;
            else
                (option->section)->options = option;
            pos_option->prev_option = option;
        }
        else
        {
            /* add option to end of section */
            option->prev_option = (option->section)->last_option;
            option->next_option = NULL;
            (option->section)->last_option->next_option = option;
            (option->section)->last_option = option;
        }
    }
    else
    {
        /* first option for section */
        option->prev_option = NULL;
        option->next_option = NULL;
        (option->section)->options = option;
        (option->section)->last_option = option;
    }
}

/*
 * config_file_new_option: create a new option
 */

struct t_config_option *
config_file_new_option (struct t_config_file *config_file,
                        struct t_config_section *section, const char *name,
                        const char *type, const char *description,
                        const char *string_values, int min, int max,
                        const char *default_value,
                        int (*callback_check_value)(void *data,
                                                    struct t_config_option *option,
                                                    const char *value),
                        void *callback_check_value_data,
                        void (*callback_change)(void *data,
                                                struct t_config_option *option),
                        void *callback_change_data,
                        void (*callback_delete)(void *data,
                                                struct t_config_option *option),
                        void *callback_delete_data)
{
    struct t_config_option *new_option;
    int var_type, int_value, argc, i, index_value;
    long number;
    char *error;
    
    if (!name)
        return NULL;
    
    if (config_file && section
        && config_file_search_option (config_file, section, name))
        return NULL;
    
    var_type = -1;
    for (i = 0; i < CONFIG_NUM_OPTION_TYPES; i++)
    {
        if (string_strcasecmp (type, config_option_type_string[i]) == 0)
        {
            var_type = i;
            break;
        }
    }
    if (var_type < 0)
    {
        gui_chat_printf (NULL, "%sError: unknown option type \"%s\"",
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         type);
        return NULL;
    }
    
    new_option = malloc (sizeof (*new_option));
    if (new_option)
    {
        new_option->config_file = config_file;
        new_option->section = section;
        new_option->name = strdup (name);
        new_option->type = var_type;
        new_option->description = (description) ? strdup (description) : NULL;
        argc = 0;
        switch (var_type)
        {
            case CONFIG_OPTION_TYPE_BOOLEAN:
                new_option->string_values = NULL;
                new_option->min = CONFIG_BOOLEAN_FALSE;
                new_option->max = CONFIG_BOOLEAN_TRUE;
                int_value = config_file_string_to_boolean (default_value);
                new_option->default_value = malloc (sizeof (int));
                *((int *)new_option->default_value) = int_value;
                new_option->value = malloc (sizeof (int));
                *((int *)new_option->value) = int_value;
                break;
            case CONFIG_OPTION_TYPE_INTEGER:
                new_option->string_values = (string_values) ?
                    string_explode (string_values, "|", 0, 0, &argc) : NULL;
                if (new_option->string_values)
                {
                    new_option->min = 0;
                    new_option->max = (argc == 0) ? 0 : argc - 1;
                    index_value = 0;
                    for (i = 0; i < argc; i++)
                    {
                        if (string_strcasecmp (new_option->string_values[i],
                                               default_value) == 0)
                        {
                            index_value = i;
                            break;
                        }
                    }
                    new_option->default_value = malloc (sizeof (int));
                    *((int *)new_option->default_value) = index_value;
                    new_option->value = malloc (sizeof (int));
                    *((int *)new_option->value) = index_value;
                }
                else
                {
                    new_option->string_values = NULL;
                    new_option->min = min;
                    new_option->max = max;
                    error = NULL;
                    number = strtol (default_value, &error, 10);
                    if (!error || error[0])
                        number = 0;
                    new_option->default_value = malloc (sizeof (int));
                    *((int *)new_option->default_value) = number;
                    new_option->value = malloc (sizeof (int));
                    *((int *)new_option->value) = number;
                }
                break;
            case CONFIG_OPTION_TYPE_STRING:
                new_option->string_values = NULL;
                new_option->min = min;
                new_option->max = max;
                new_option->default_value = (default_value) ?
                    strdup (default_value) : NULL;
                new_option->value = (default_value) ?
                    strdup (default_value) : NULL;
                break;
            case CONFIG_OPTION_TYPE_COLOR:
                new_option->string_values = NULL;
                new_option->min = min;
                new_option->max = gui_color_get_number () - 1;
                new_option->default_value = malloc (sizeof (int));
                if (!gui_color_assign (new_option->default_value, default_value))
                    *((int *)new_option->default_value) = 0;
                new_option->value = malloc (sizeof (int));
                *((int *)new_option->value) = *((int *)new_option->default_value);
                break;
            case CONFIG_NUM_OPTION_TYPES:
                break;
        }
        new_option->callback_check_value = callback_check_value;
        new_option->callback_check_value_data = callback_check_value_data;
        new_option->callback_change = callback_change;
        new_option->callback_change_data = callback_change_data;
        new_option->callback_delete = callback_delete;
        new_option->callback_delete_data = callback_delete_data;
        new_option->loaded = 0;

        if (section)
        {
            config_file_option_insert_in_section (new_option);
        }
        else
        {
            new_option->prev_option = NULL;
            new_option->next_option = NULL;
        }
    }
    
    return new_option;
}

/*
 * config_file_option_full_name: build full name for an option
 */

char *
config_file_option_full_name (struct t_config_option *option)
{
    int length_option;
    char *option_full_name;
    
    length_option = strlen (option->config_file->name) + 1 +
        strlen (option->section->name) + 1 + strlen (option->name) + 1;
    option_full_name = malloc (length_option);
    if (option_full_name)
    {
        snprintf (option_full_name, length_option,
                  "%s.%s.%s",
                  option->config_file->name,
                  option->section->name,
                  option->name);
    }
    
    return option_full_name;
}

/*
 * config_file_search_option: search an option in a config or section
 */

struct t_config_option *
config_file_search_option (struct t_config_file *config_file,
                           struct t_config_section *section,
                           const char *option_name)
{
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;

    if (section)
    {
        for (ptr_option = section->options; ptr_option;
             ptr_option = ptr_option->next_option)
        {
            if (string_strcasecmp (ptr_option->name, option_name) == 0)
                return ptr_option;
        }
    }
    else if (config_file)
    {
        for (ptr_section = config_file->sections; ptr_section;
             ptr_section = ptr_section->next_section)
        {
            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                if (string_strcasecmp (ptr_option->name, option_name) == 0)
                    return ptr_option;
            }
        }
    }
    
    /* option not found */
    return NULL;
}

/*
 * config_file_search_section_option: search an option in a config or section
 *                                    and return section/option
 */

void
config_file_search_section_option (struct t_config_file *config_file,
                                   struct t_config_section *section,
                                   const char *option_name,
                                   struct t_config_section **section_found,
                                   struct t_config_option **option_found)
{
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    
    *section_found = NULL;
    *option_found = NULL;
    
    if (section)
    {
        for (ptr_option = section->options; ptr_option;
             ptr_option = ptr_option->next_option)
        {
            if (string_strcasecmp (ptr_option->name, option_name) == 0)
            {
                *section_found = section;
                *option_found = ptr_option;
                return;
            }
        }
    }
    else if (config_file)
    {
        for (ptr_section = config_file->sections; ptr_section;
             ptr_section = ptr_section->next_section)
        {
            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                if (string_strcasecmp (ptr_option->name, option_name) == 0)
                {
                    *section_found = ptr_section;
                    *option_found = ptr_option;
                }
            }
        }
    }
}

/*
 * config_file_search_with_string: search file/section/option for
 *                                 an option with full string
 *                                 (file.section.option)
 */

void
config_file_search_with_string (const char *option_name,
                                struct t_config_file **config_file,
                                struct t_config_section **section,
                                struct t_config_option **option,
                                char **pos_option_name)
{
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    char *file_name, *pos_section, *section_name, *pos_option;

    if (config_file)
        *config_file = NULL;
    if (section)
        *section = NULL;
    if (option)
        *option = NULL;
    if (pos_option_name)
        *pos_option_name = NULL;
    
    ptr_config = NULL;
    ptr_section = NULL;
    ptr_option = NULL;
    
    file_name = NULL;
    section_name = NULL;
    pos_option = NULL;
    
    pos_section = strchr (option_name, '.');
    pos_option = (pos_section) ? strchr (pos_section + 1, '.') : NULL;
    if (pos_section && pos_option)
    {
        file_name = string_strndup (option_name, pos_section - option_name);
        section_name = string_strndup (pos_section + 1,
                                       pos_option - pos_section - 1);
        pos_option++;
    }
    if (file_name && section_name && pos_option)
    {
        if (pos_option_name)
            *pos_option_name = pos_option;
        ptr_config = config_file_search (file_name);
        if (ptr_config)
        {
            ptr_section = config_file_search_section (ptr_config,
                                                      section_name);
            if (ptr_section)
            {
                ptr_option = config_file_search_option (ptr_config,
                                                        ptr_section,
                                                        pos_option);
            }
        }
    }
    
    if (file_name)
        free (file_name);
    if (section_name)
        free (section_name);
    
    if (config_file)
        *config_file = ptr_config;
    if (section)
        *section = ptr_section;
    if (option)
        *option = ptr_option;
}

/*
 * config_file_option_valid_for_plugin: check if an option pointer exists for a plugin
 *                                      return 1 if option exists for plugin
 *                                             0 if option is not found for plugin
 */

int
config_file_option_valid_for_plugin (struct t_weechat_plugin *plugin,
                                     struct t_config_option *option)
{
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;

    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        if (ptr_config->plugin == plugin)
        {
            for (ptr_section = ptr_config->sections; ptr_section;
                 ptr_section = ptr_section->next_section)
            {
                for (ptr_option = ptr_section->options; ptr_option;
                     ptr_option = ptr_option->next_option)
                {
                    if (ptr_option == option)
                        return 1;
                }
            }
        }
    }
    
    /* option not found */
    return 0;
}

/*
 * config_file_string_boolean_is_valid: return 1 if boolean is valid, otherwise 0
 */

int
config_file_string_boolean_is_valid (const char *text)
{
    int i;
    
    if (!text)
        return 0;
    
    for (i = 0; config_boolean_true[i]; i++)
    {
        if (string_strcasecmp (text, config_boolean_true[i]) == 0)
            return 1;
    }

    for (i = 0; config_boolean_false[i]; i++)
    {
        if (string_strcasecmp (text, config_boolean_false[i]) == 0)
            return 1;
    }
    
    /* text is not a boolean */
    return 0;
}

/*
 * config_file_string_to_boolean: return boolean value of string
 *                                (1 for true, 0 for false)
 */

int
config_file_string_to_boolean (const char *text)
{
    int i;
    
    if (!text)
        return CONFIG_BOOLEAN_FALSE;
    
    for (i = 0; config_boolean_true[i]; i++)
    {
        if (string_strcasecmp (text, config_boolean_true[i]) == 0)
            return CONFIG_BOOLEAN_TRUE;
    }
    
    return CONFIG_BOOLEAN_FALSE;
}

/*
 * config_file_option_reset: set default value for an option
 *                         return one of these values:
 *                           WEECHAT_CONFIG_OPTION_SET_OK_CHANGED
 *                           WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE
 *                           WEECHAT_CONFIG_OPTION_SET_ERROR
 *
 *                           2 if ok (value changed)
 *                           1 if ok (value is the same)
 *                           0 if failed
 */

int
config_file_option_reset (struct t_config_option *option, int run_callback)
{
    int rc;
    char value[256], *option_full_name;
    
    if (!option)
        return WEECHAT_CONFIG_OPTION_SET_ERROR;
    
    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
    value[0] = '\0';
    
    switch (option->type)
    {
        case CONFIG_OPTION_TYPE_BOOLEAN:
            if (CONFIG_BOOLEAN(option) == CONFIG_BOOLEAN_DEFAULT(option))
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            else
            {
                CONFIG_BOOLEAN(option) = CONFIG_BOOLEAN_DEFAULT(option);
                snprintf (value, sizeof (value), "%s",
                          CONFIG_BOOLEAN(option) ?
                          config_boolean_true[0] : config_boolean_false[0]);
                rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
            }
            break;
        case CONFIG_OPTION_TYPE_INTEGER:
            if (CONFIG_INTEGER(option) == CONFIG_INTEGER_DEFAULT(option))
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            else
            {
                CONFIG_INTEGER(option) = CONFIG_INTEGER_DEFAULT(option);
                snprintf (value, sizeof (value), "%d",
                          CONFIG_INTEGER(option));
                rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
            }
            break;
        case CONFIG_OPTION_TYPE_STRING:
            rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            if ((!option->value && option->default_value)
                || (option->value && !option->default_value)
                || (strcmp ((char *)option->value,
                            (char *)option->default_value) != 0))
                rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
            if (option->value)
                free (option->value);
            if (option->default_value)
            {
                option->value = strdup ((char *)option->default_value);
                if (option->value)
                {
                    snprintf (value, sizeof (value), "%s",
                              CONFIG_STRING(option));
                }
                else
                    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
            }
            else
                option->value = NULL;
            break;
        case CONFIG_OPTION_TYPE_COLOR:
            if (CONFIG_COLOR(option) == CONFIG_COLOR_DEFAULT(option))
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            else
            {
                CONFIG_COLOR(option) = CONFIG_COLOR_DEFAULT(option);
                snprintf (value, sizeof (value), "%d",
                          CONFIG_COLOR(option));
                rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
            }
            break;
        case CONFIG_NUM_OPTION_TYPES:
            break;
    }
    
    if ((rc == WEECHAT_CONFIG_OPTION_SET_OK_CHANGED)
        && run_callback && option->callback_change)
    {
        (void)(option->callback_change)(option->callback_change_data, option);
    }
    
    if (rc != WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        if (option->config_file && option->section)
        {
            option_full_name = config_file_option_full_name (option);
            if (option_full_name)
            {
                hook_config_exec (option_full_name, value);
                free (option_full_name);
            }
        }
    }
    
    return rc;
}

/*
 * config_file_option_set: set value for an option
 *                         if value is NULL, then default value for option is set
 *                         return one of these values:
 *                           WEECHAT_CONFIG_OPTION_SET_OK_CHANGED
 *                           WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE
 *                           WEECHAT_CONFIG_OPTION_SET_ERROR
 */

int
config_file_option_set (struct t_config_option *option, const char *value,
                        int run_callback)
{
    int value_int, i, rc, new_value_ok;
    long number;
    char *error, *option_full_name;
    
    if (!option)
        return WEECHAT_CONFIG_OPTION_SET_ERROR;
    
    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
    
    if (value && option->callback_check_value)
    {
        if (!(int)(option->callback_check_value)
            (option->callback_check_value_data,
             option,
             value))
            return WEECHAT_CONFIG_OPTION_SET_ERROR;
    }
    
    switch (option->type)
    {
        case CONFIG_OPTION_TYPE_BOOLEAN:
            if (value)
            {
                if (string_strcasecmp (value, "toggle") == 0)
                {
                    *((int *)option->value) =
                        (*((int *)option->value) == CONFIG_BOOLEAN_TRUE) ?
                        CONFIG_BOOLEAN_FALSE : CONFIG_BOOLEAN_TRUE;
                    rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                }
                else
                {
                    if (config_file_string_boolean_is_valid (value))
                    {
                        value_int = config_file_string_to_boolean (value);
                        if (value_int == *((int *)option->value))
                            rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                        else
                        {
                            *((int *)option->value) = value_int;
                            rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                        }
                    }
                }
            }
            break;
        case CONFIG_OPTION_TYPE_INTEGER:
            if (value)
            {
                if (option->string_values)
                {
                    value_int = -1;
                    if (strncmp (value, "++", 2) == 0)
                    {
                        error = NULL;
                        number = strtol (value + 2, &error, 10);
                        if (error && !error[0])
                        {
                            number = number % (option->max + 1);
                            value_int = (*((int *)option->value) + number) %
                                (option->max + 1);
                        }
                    }
                    else if (strncmp (value, "--", 2) == 0)
                    {
                        error = NULL;
                        number = strtol (value + 2, &error, 10);
                        if (error && !error[0])
                        {
                            number = number % (option->max + 1);
                            value_int = (*((int *)option->value) + (option->max + 1) - number) %
                                (option->max + 1);
                        }
                    }
                    else
                    {
                        for (i = 0; option->string_values[i]; i++)
                        {
                            if (string_strcasecmp (option->string_values[i],
                                                   value) == 0)
                            {
                                value_int = i;
                                break;
                            }
                        }
                    }
                    if (value_int >= 0)
                    {
                        if (value_int == *((int *)option->value))
                            rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                        else
                        {
                            *((int *)option->value) = value_int;
                            rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                        }
                    }
                }
                else
                {
                    new_value_ok = 0;
                    if (strncmp (value, "++", 2) == 0)
                    {
                        error = NULL;
                        number = strtol (value + 2, &error, 10);
                        if (error && !error[0])
                        {
                            value_int = *((int *)option->value) + number;
                            if (value_int <= option->max)
                                new_value_ok = 1;
                        }
                    }
                    else if (strncmp (value, "--", 2) == 0)
                    {
                        error = NULL;
                        number = strtol (value + 2, &error, 10);
                        if (error && !error[0])
                        {
                            value_int = *((int *)option->value) - number;
                            if (value_int >= option->min)
                                new_value_ok = 1;
                        }
                    }
                    else
                    {
                        error = NULL;
                        number = strtol (value, &error, 10);
                        if (error && !error[0])
                        {
                            value_int = number;
                            if ((value_int >= option->min)
                                && (value_int <= option->max))
                                new_value_ok = 1;
                        }
                    }
                    if (new_value_ok)
                    {
                        if (value_int == *((int *)option->value))
                            rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                        else
                        {
                            *((int *)option->value) = value_int;
                            rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                        }
                    }
                }
            }
            break;
        case CONFIG_OPTION_TYPE_STRING:
            rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            if ((!option->value && value)
                || (option->value && !value)
                || (strcmp ((char *)option->value, value) != 0))
                rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
            if (option->value)
                free (option->value);
            if (value)
            {
                option->value = strdup (value);
                if (!option->value)
                    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
            }
            else
                option->value = NULL;
            break;
        case CONFIG_OPTION_TYPE_COLOR:
            value_int = -1;
            if (strncmp (value, "++", 2) == 0)
            {
                error = NULL;
                number = strtol (value + 2, &error, 10);
                if (error && !error[0])
                {
                    number = number % (option->max + 1);
                    value_int = (*((int *)option->value) + number) %
                        (option->max + 1);
                }
            }
            else if (strncmp (value, "--", 2) == 0)
            {
                error = NULL;
                number = strtol (value + 2, &error, 10);
                if (error && !error[0])
                {
                    number = number % (option->max + 1);
                    value_int = (*((int *)option->value) + (option->max + 1) - number) %
                        (option->max + 1);
                }
            }
            else
            {
                gui_color_assign (&value_int, value);
            }
            if (value_int >= 0)
            {
                if (value_int == *((int *)option->value))
                    rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                else
                {
                    *((int *)option->value) = value_int;
                    rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                }
            }
            break;
        case CONFIG_NUM_OPTION_TYPES:
            break;
    }
    
    if ((rc == WEECHAT_CONFIG_OPTION_SET_OK_CHANGED)
        && run_callback && option->callback_change)
    {
        (void)(option->callback_change)(option->callback_change_data, option);
    }
    
    if (rc != WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        if (option->config_file && option->section)
        {
            option_full_name = config_file_option_full_name (option);
            if (option_full_name)
            {
                hook_config_exec (option_full_name, value);
                free (option_full_name);
            }
        }
    }
    
    return rc;
}

/*
 * config_file_option_unset: unset/reset option
 *                           return one of these values:
 *                             WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET
 *                             WEECHAT_CONFIG_OPTION_UNSET_OK_RESET
 *                             WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED
 *                             WEECHAT_CONFIG_OPTION_UNSET_ERROR
 */

int
config_file_option_unset (struct t_config_option *option)
{
    int rc;
    char *option_full_name;
    
    rc = WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET;
    
    if (option->section && option->section->user_can_delete_options)
    {
        /* delete option */
        if (option->callback_delete)
        {
            (void)(option->callback_delete)
                (option->callback_delete_data,
                 option);
        }
        
        option_full_name = config_file_option_full_name (option);
        
        config_file_option_free (option);
        rc = WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
        
        if (option_full_name)
        {
            hook_config_exec (option_full_name, NULL);
            free (option_full_name);
        }
    }
    else
    {
        /* reset value */
        switch (config_file_option_reset (option, 1))
        {
            case WEECHAT_CONFIG_OPTION_SET_ERROR:
                rc = WEECHAT_CONFIG_OPTION_UNSET_ERROR;
                break;
            case WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE:
                rc = WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET;
                break;
            case WEECHAT_CONFIG_OPTION_SET_OK_CHANGED:
                rc = WEECHAT_CONFIG_OPTION_UNSET_OK_RESET;
                break;
        }
    }
    
    return rc;
}

/*
 * config_file_option_rename: rename an option
 */

void
config_file_option_rename (struct t_config_option *option,
                           const char *new_name)
{
    if (!new_name || !new_name[0])
        return;
    
    /* remove option from list */
    if (option->section)
    {
        if (option->prev_option)
            (option->prev_option)->next_option = option->next_option;
        if (option->next_option)
            (option->next_option)->prev_option = option->prev_option;
        if (option->section->options == option)
            (option->section)->options = option->next_option;
        if (option->section->last_option == option)
            (option->section)->last_option = option->prev_option;
    }
    
    /* rename option */
    if (option->name)
        free (option->name);
    option->name = strdup (new_name);

    /* re-insert option in section */
    if (option->section)
        config_file_option_insert_in_section (option);
}

/*
 * config_file_option_get_pointer: get a pointer of an option property
 */

void *
config_file_option_get_pointer (struct t_config_option *option,
                                const char *property)
{
    if (string_strcasecmp (property, "config_file") == 0)
        return option->config_file;
    else if (string_strcasecmp (property, "section") == 0)
        return option->section;
    else if (string_strcasecmp (property, "name") == 0)
        return option->name;
    else if (string_strcasecmp (property, "type") == 0)
        return &option->type;
    else if (string_strcasecmp (property, "description") == 0)
        return option->description;
    else if (string_strcasecmp (property, "string_values") == 0)
        return option->string_values;
    else if (string_strcasecmp (property, "min") == 0)
        return &option->min;
    else if (string_strcasecmp (property, "max") == 0)
        return &option->max;
    else if (string_strcasecmp (property, "default_value") == 0)
        return option->default_value;
    else if (string_strcasecmp (property, "value") == 0)
        return option->value;
    else if (string_strcasecmp (property, "prev_option") == 0)
        return option->prev_option;
    else if (string_strcasecmp (property, "next_option") == 0)
        return option->next_option;
    
    return NULL;
}

/*
 * config_file_option_set_with_string: set value for an option (with a string
 *                                     for name of option)
 *                                     return one of these values:
 *                                       WEECHAT_CONFIG_OPTION_SET_OK_CHANGED
 *                                       WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE
 *                                       WEECHAT_CONFIG_OPTION_SET_ERROR
 *                                       WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND
 */

int
config_file_option_set_with_string (const char *option_name, const char *value)
{
    int rc;
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    char *pos_option;
    
    rc = WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND;
    
    config_file_search_with_string (option_name, &ptr_config, &ptr_section,
                                    &ptr_option, &pos_option);
    
    if (ptr_config && ptr_section)
    {
        if (ptr_option)
            rc = config_file_option_set (ptr_option, value, 1);
        else
        {
            if (ptr_section->user_can_add_options
                && ptr_section->callback_create_option)
            {
                rc = (int)(ptr_section->callback_create_option)
                    (ptr_section->callback_create_option_data,
                     ptr_config,
                     ptr_section,
                     pos_option,
                     value);
                if ((rc == WEECHAT_CONFIG_OPTION_SET_OK_CHANGED)
                    || (rc == WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE))
                {
                    hook_config_exec (option_name, value);
                }
            }
        }
    }
    
    return rc;
}

/*
 * config_file_option_unset_with_string: unset/reset option
 *                                       return one of these values:
 *                                         WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET
 *                                         WEECHAT_CONFIG_OPTION_UNSET_OK_RESET
 *                                         WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED
 *                                         WEECHAT_CONFIG_OPTION_UNSET_ERROR
 */

int
config_file_option_unset_with_string (const char *option_name)
{
    struct t_config_option *ptr_option;
    int rc;
    
    rc = WEECHAT_CONFIG_OPTION_UNSET_ERROR;
    
    config_file_search_with_string (option_name, NULL, NULL, &ptr_option, NULL);
    
    if (ptr_option)
        rc = config_file_option_unset (ptr_option);
    
    return rc;
}

/*
 * config_file_option_boolean: return boolean value of an option
 */

int
config_file_option_boolean (struct t_config_option *option)
{
    if (!option)
        return 0;
    
    if (option->type == CONFIG_OPTION_TYPE_BOOLEAN)
        return CONFIG_BOOLEAN(option);
    else
        return 0;
}

/*
 * config_file_option_integer: return integer value of an option
 */

int
config_file_option_integer (struct t_config_option *option)
{
    if (!option)
        return 0;
    
    switch (option->type)
    {
        case CONFIG_OPTION_TYPE_BOOLEAN:
            if (CONFIG_BOOLEAN(option) == CONFIG_BOOLEAN_TRUE)
                return 1;
            else
                return 0;
        case CONFIG_OPTION_TYPE_INTEGER:
        case CONFIG_OPTION_TYPE_COLOR:
            return CONFIG_INTEGER(option);
        case CONFIG_OPTION_TYPE_STRING:
            return 0;
        case CONFIG_NUM_OPTION_TYPES:
            break;
    }
    return 0;
}

/*
 * config_file_option_string: return string value of an option
 */

char *
config_file_option_string (struct t_config_option *option)
{
    if (!option)
        return NULL;
    
    switch (option->type)
    {
        case CONFIG_OPTION_TYPE_BOOLEAN:
            if (CONFIG_BOOLEAN(option))
                return config_boolean_true[0];
            else
                return config_boolean_false[0];
        case CONFIG_OPTION_TYPE_INTEGER:
            if (option->string_values)
                return option->string_values[CONFIG_INTEGER(option)];
            return NULL;
        case CONFIG_OPTION_TYPE_STRING:
            return CONFIG_STRING(option);
        case CONFIG_OPTION_TYPE_COLOR:
            return gui_color_get_name (CONFIG_COLOR(option));
        case CONFIG_NUM_OPTION_TYPES:
            return NULL;
    }
    return NULL;
}

/*
 * config_file_option_color: return color value of an option
 */

int
config_file_option_color (struct t_config_option *option)
{
    if (!option)
        return 0;
    
    if (option->type == CONFIG_OPTION_TYPE_COLOR)
        return CONFIG_COLOR(option);
    else
        return 0;
}

/*
 * config_file_write_option: write an option in a config file
 */

void
config_file_write_option (struct t_config_file *config_file,
                          struct t_config_option *option,
                          int default_value)
{
    void *value;
    
    if (!config_file || !config_file->file || !option)
        return;
    
    value = (default_value) ? option->default_value : option->value;
    
    switch (option->type)
    {
        case CONFIG_OPTION_TYPE_BOOLEAN:
            string_iconv_fprintf (config_file->file, "%s = %s\n",
                                  option->name,
                                  (*((int *)value)) == CONFIG_BOOLEAN_TRUE ?
                                  "on" : "off");
            break;
        case CONFIG_OPTION_TYPE_INTEGER:
            if (option->string_values)
                string_iconv_fprintf (config_file->file, "%s = %s\n",
                                      option->name,
                                      option->string_values[*((int *)value)]);
            else
                string_iconv_fprintf (config_file->file, "%s = %d\n",
                                      option->name,
                                      *((int *)value));
            break;
        case CONFIG_OPTION_TYPE_STRING:
            string_iconv_fprintf (config_file->file, "%s = \"%s\"\n",
                                  option->name,
                                  (char *)value);
            break;
        case CONFIG_OPTION_TYPE_COLOR:
            string_iconv_fprintf (config_file->file, "%s = %s\n",
                                  option->name,
                                  gui_color_get_name (*((int *)value)));
            break;
        case CONFIG_NUM_OPTION_TYPES:
            break;
    }
}

/*
 * config_file_write_line: write a line in a config file
 *                         if value is NULL, then write a section with [ ] around
 */

void
config_file_write_line (struct t_config_file *config_file,
                        const char *option_name, const char *value, ...)
{
    char buf[4096];
    va_list argptr;
    
    if (!config_file || !option_name)
        return;
    
    if (value)
    {
        va_start (argptr, value);
        vsnprintf (buf, sizeof (buf) - 1, value, argptr);
        va_end (argptr);
        string_iconv_fprintf (config_file->file, "%s = %s\n",
                              option_name, buf);
    }
    else
    {
        string_iconv_fprintf (config_file->file, "\n[%s]\n",
                              option_name);
    }
}

/*
 * config_file_write_internal: write a configuration file
 *                             (should not be called directly)
 *                             return one of these values:
 *                               WEECHAT_CONFIG_WRITE_OK
 *                               WEECHAT_CONFIG_WRITE_ERROR
 *                               WEECHAT_CONFIG_WRITE_MEMORY_ERROR
 */

int
config_file_write_internal (struct t_config_file *config_file,
                            int default_options)
{
    int filename_length, rc;
    char *filename, *filename2;
    time_t current_time;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    
    if (!config_file)
        return WEECHAT_CONFIG_WRITE_ERROR;

    /* build filename */
    filename_length = strlen (weechat_home) +
        strlen (config_file->filename) + 2;
    filename = malloc (filename_length);
    if (!filename)
        return WEECHAT_CONFIG_WRITE_MEMORY_ERROR;
    snprintf (filename, filename_length, "%s%s%s",
              weechat_home, DIR_SEPARATOR, config_file->filename);
    
    /* build temporary filename, this temp file will be renamed to filename
       after write */
    filename2 = malloc (filename_length + 32);
    if (!filename2)
    {
        free (filename);
        return WEECHAT_CONFIG_WRITE_MEMORY_ERROR;
    }
    snprintf (filename2, filename_length + 32, "%s.weechattmp", filename);
    
    /* open temp file in write mode */
    config_file->file = fopen (filename2, "w");
    if (!config_file->file)
    {
        gui_chat_printf (NULL,
                         _("%sError: cannot create file \"%s\""),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         filename2);
        free (filename);
        free (filename2);
        return WEECHAT_CONFIG_WRITE_ERROR;
    }
    
    log_printf (_("Writing configuration file %s %s"),
                config_file->filename,
                (default_options) ? _("(default options)") : "");
    
    /* write header with version and date */
    current_time = time (NULL);
    string_iconv_fprintf (config_file->file, "#\n");
    string_iconv_fprintf (config_file->file,
                          "# %s -- %s v%s, %s#\n",
                          config_file->filename, PACKAGE_NAME, PACKAGE_VERSION,
                          ctime (&current_time));
    
    /* write all sections */
    for (ptr_section = config_file->sections; ptr_section;
         ptr_section = ptr_section->next_section)
    {
        /* call write callback if defined for section */
        if (default_options && ptr_section->callback_write_default)
        {
            (void) (ptr_section->callback_write_default) (ptr_section->callback_write_default_data,
                                                          config_file,
                                                          ptr_section->name);
        }
        else if (!default_options && ptr_section->callback_write)
        {
            (void) (ptr_section->callback_write) (ptr_section->callback_write_data,
                                                  config_file,
                                                  ptr_section->name);
        }
        else
        {
            /* write all options for section */
            string_iconv_fprintf (config_file->file,
                                  "\n[%s]\n", ptr_section->name);
            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                config_file_write_option (config_file, ptr_option,
                                          default_options);
            }
        }
    }

    /* close temp file */
    fclose (config_file->file);
    config_file->file = NULL;

    /* update file mode */
    chmod (filename2, 0600);

    /* remove target file */
    unlink (filename);

    /* rename temp file to target file */
    rc = rename (filename2, filename);
    
    free (filename);
    free (filename2);
    
    if (rc != 0)
        return WEECHAT_CONFIG_WRITE_ERROR;
    
    return WEECHAT_CONFIG_WRITE_OK;
}

/*
 * config_file_write: write a configuration file
 *                    return one of these values:
 *                      WEECHAT_CONFIG_WRITE_OK
 *                      WEECHAT_CONFIG_WRITE_ERROR
 *                      WEECHAT_CONFIG_WRITE_MEMORY_ERROR
 */

int
config_file_write (struct t_config_file *config_file)
{
    return config_file_write_internal (config_file, 0);
}

/*
 * config_file_read_internal: read a configuration file
 *                            (should not be called directly)
 *                            return one of these values:
 *                              WEECHAT_CONFIG_READ_OK
 *                              WEECHAT_CONFIG_READ_MEMORY_ERROR
 *                              WEECHAT_CONFIG_READ_FILE_NOT_FOUND
 */

int
config_file_read_internal (struct t_config_file *config_file, int reload)
{
    int filename_length, line_number, rc;
    char *filename;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    char line[1024], *ptr_line, *ptr_line2, *pos, *pos2;
    
    if (!config_file)
        return WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
    
    /* build filename */
    filename_length = strlen (weechat_home) + strlen (config_file->filename) + 2;
    filename = malloc (filename_length);
    if (!filename)
        return WEECHAT_CONFIG_READ_MEMORY_ERROR;
    snprintf (filename, filename_length, "%s%s%s",
              weechat_home, DIR_SEPARATOR, config_file->filename);
    config_file->file = fopen (filename, "r");
    if (!config_file->file)
    {
        config_file_write_internal (config_file, 1);
        config_file->file = fopen (filename, "r");
        if (!config_file->file)
        {
            gui_chat_printf (NULL,
                             _("%sWarning: config file \"%s\" not found"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             filename);
            free (filename);
            return WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
        }
    }
    
    if (!reload)
        log_printf (_("Reading configuration file %s"), config_file->filename);
    
    /* read all lines */
    ptr_section = NULL;
    line_number = 0;
    while (!feof (config_file->file))
    {
        ptr_line = fgets (line, sizeof (line) - 1, config_file->file);
        line_number++;
        if (ptr_line)
        {
            /* encode line to internal charset */
            ptr_line2 = string_iconv_to_internal (NULL, ptr_line);
            if (ptr_line2)
            {
                snprintf (line, sizeof (line) - 1, "%s", ptr_line2);
                free (ptr_line2);
            }
            
            /* skip spaces */
            while (ptr_line[0] == ' ')
                ptr_line++;
            /* not a comment and not an empty line */
            if ((ptr_line[0] != '#') && (ptr_line[0] != '\r')
                && (ptr_line[0] != '\n'))
            {
                /* beginning of section */
                if (ptr_line[0] == '[')
                {
                    pos = strchr (line, ']');
                    if (!pos)
                    {
                        gui_chat_printf (NULL,
                                         _("%sWarning: %s, line %d: invalid "
                                           "syntax, missing \"]\""),
                                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                         filename, line_number);
                    }
                    else
                    {
                        pos[0] = '\0';
                        pos = ptr_line + 1;
                        ptr_section = config_file_search_section (config_file,
                                                                  pos);
                        if (!ptr_section)
                        {
                            gui_chat_printf (NULL,
                                             _("%sWarning: %s, line %d: unknown "
                                               "section identifier "
                                               "(\"%s\")"),
                                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                             filename, line_number, pos);
                        }
                    }
                }
                else
                {
                    pos = strstr (line, " = ");
                    if (!pos)
                    {
                        gui_chat_printf (NULL,
                                         _("%sWarning: %s, line %d: invalid "
                                           "syntax, missing \"=\""),
                                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                         filename, line_number);
                    }
                    else
                    {
                        pos[0] = '\0';
                        pos += 3;
                        
                        /* remove spaces before '=' */
                        pos2 = pos - 4;
                        while ((pos2 > line) && (pos2[0] == ' '))
                        {
                            pos2[0] = '\0';
                            pos2--;
                        }
                        
                        /* skip spaces after '=' */
                        while (pos[0] && (pos[0] == ' '))
                        {
                            pos++;
                        }
                        
                        /* remove CR/LF */
                        pos2 = strchr (pos, '\r');
                        if (pos2 != NULL)
                            pos2[0] = '\0';
                        pos2 = strchr (pos, '\n');
                        if (pos2 != NULL)
                            pos2[0] = '\0';
                        
                        /* remove simple or double quotes 
                           and spaces at the end */
                        if (strlen(pos) > 1)
                        {
                            pos2 = pos + strlen (pos) - 1;
                            while ((pos2 > pos) && (pos2[0] == ' '))
                            {
                                pos2[0] = '\0';
                                pos2--;
                            }
                            pos2 = pos + strlen (pos) - 1;
                            if (((pos[0] == '\'') &&
                                 (pos2[0] == '\'')) ||
                                ((pos[0] == '"') &&
                                 (pos2[0] == '"')))
                            {
                                pos2[0] = '\0';
                                pos++;
                            }
                        }
                        
                        if (ptr_section && ptr_section->callback_read)
                        {
                            ptr_option = NULL;
                            rc = (ptr_section->callback_read)
                                (ptr_section->callback_read_data,
                                 config_file,
                                 ptr_section,
                                 line,
                                 pos);
                        }
                        else
                        {
                            rc = WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND;
                            ptr_option = config_file_search_option (config_file,
                                                                    ptr_section,
                                                                    line);
                            if (ptr_option)
                            {
                                rc = config_file_option_set (ptr_option, pos, 1);
                                ptr_option->loaded = 1;
                            }
                            else
                            {
                                if (ptr_section
                                    && ptr_section->callback_create_option)
                                {
                                    rc = (int)(ptr_section->callback_create_option)
                                        (ptr_section->callback_create_option_data,
                                         config_file,
                                         ptr_section,
                                         line,
                                         pos);
                                }
                            }
                        }
                        
                        switch (rc)
                        {
                            case WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND:
                                if (ptr_section)
                                    gui_chat_printf (NULL,
                                                     _("%sWarning: %s, line %d: "
                                                       "option \"%s\" "
                                                       "unknown for "
                                                       "section \"%s\""),
                                                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                                     filename, line_number,
                                                     line, ptr_section->name);
                                else
                                    gui_chat_printf (NULL,
                                                     _("%sWarning: %s, line %d: "
                                                       "unknown option \"%s\" "
                                                       "(outside a section)"),
                                                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                                     filename, line_number,
                                                     line);
                                break;
                            case WEECHAT_CONFIG_OPTION_SET_ERROR:
                                gui_chat_printf (NULL,
                                                 _("%sWarning: %s, line %d: "
                                                   "invalid value for option "
                                                   "\"%s\""),
                                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                                 filename, line_number, line);
                                break;
                        }
                    }
                }
            }
        }
    }
    
    fclose (config_file->file);
    config_file->file = NULL;
    free (filename);
    
    return WEECHAT_CONFIG_READ_OK;
}

/*
 * config_file_read: read a configuration file
 *                   return one of these values:
 *                     WEECHAT_CONFIG_READ_OK
 *                     WEECHAT_CONFIG_READ_MEMORY_ERROR
 *                     WEECHAT_CONFIG_READ_FILE_NOT_FOUND
 */

int
config_file_read (struct t_config_file *config_file)
{
    return config_file_read_internal (config_file, 0);
}

/*
 * config_file_reload: reload a configuration file
 *                     return one of these values:
 *                       WEECHAT_CONFIG_READ_OK
 *                       WEECHAT_CONFIG_READ_MEMORY_ERROR
 *                       WEECHAT_CONFIG_READ_FILE_NOT_FOUND
 */

int
config_file_reload (struct t_config_file *config_file)
{
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    int rc;
    
    if (!config_file)
        return WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
    
    log_printf (_("Reloading configuration file %s"), config_file->filename);
    
    /* init "loaded" flag for all options */
    for (ptr_section = config_file->sections; ptr_section;
         ptr_section = ptr_section->next_section)
    {
        for (ptr_option = ptr_section->options; ptr_option;
             ptr_option = ptr_option->next_option)
        {
            ptr_option->loaded = 0;
        }
    }
    
    /* read configuration file */
    rc = config_file_read_internal (config_file, 1);
    
    /* reset options not found in configuration file */
    for (ptr_section = config_file->sections; ptr_section;
         ptr_section = ptr_section->next_section)
    {
        for (ptr_option = ptr_section->options; ptr_option;
             ptr_option = ptr_option->next_option)
        {
            if (!ptr_option->loaded)
                config_file_option_reset (ptr_option, 1);
        }
    }
    
    return rc;
}

/*
 * config_file_option_free_data: free data in an option
 */

void
config_file_option_free_data (struct t_config_option *option)
{
    if (option->name)
        free (option->name);
    if (option->description)
        free (option->description);
    if (option->string_values)
        string_free_exploded (option->string_values);
    if (option->default_value)
        free (option->default_value);
    if (option->value)
        free (option->value);
}

/*
 * config_file_option_free: free an option
 */

void
config_file_option_free (struct t_config_option *option)
{
    struct t_config_section *ptr_section;
    struct t_config_option *new_options;
    
    if (!option)
        return;

    ptr_section = option->section;
    
    /* remove option from section */
    if (ptr_section)
    {
        if (ptr_section->last_option == option)
            ptr_section->last_option = option->prev_option;
        if (option->prev_option)
        {
            (option->prev_option)->next_option = option->next_option;
            new_options = ptr_section->options;
        }
        else
            new_options = option->next_option;
        if (option->next_option)
            (option->next_option)->prev_option = option->prev_option;
    }
    
    /* free data */
    config_file_option_free_data (option);
    
    free (option);
    
    if (ptr_section)
        ptr_section->options = new_options;
}

/*
 * config_file_section_free_options: free options in a section
 */

void
config_file_section_free_options (struct t_config_section *section)
{
    if (!section)
        return;
    
    while (section->options)
    {
        config_file_option_free (section->options);
    }
}

/*
 * config_file_section_free: free a section
 */

void
config_file_section_free (struct t_config_file *config_file,
                          struct t_config_section *section)
{
    struct t_config_section *new_sections;
    
    if (!config_file || !section)
        return;
    
    /* remove section */
    if (config_file->last_section == section)
        config_file->last_section = section->prev_section;
    if (section->prev_section)
    {
        (section->prev_section)->next_section = section->next_section;
        new_sections = config_file->sections;
    }
    else
        new_sections = section->next_section;
    
    if (section->next_section)
        (section->next_section)->prev_section = section->prev_section;
    
    /* free data */
    config_file_section_free_options (section);
    if (section->name)
        free (section->name);
    
    free (section);
    
    config_file->sections = new_sections;
}

/*
 * config_file_free: free a configuration file
 */

void
config_file_free (struct t_config_file *config_file)
{
    struct t_config_file *new_config_files;
    
    if (!config_file)
        return;
    
    /* remove config file */
    if (last_config_file == config_file)
        last_config_file = config_file->prev_config;
    if (config_file->prev_config)
    {
        (config_file->prev_config)->next_config = config_file->next_config;
        new_config_files = config_files;
    }
    else
        new_config_files = config_file->next_config;
    
    if (config_file->next_config)
        (config_file->next_config)->prev_config = config_file->prev_config;
    
    /* free data */
    while (config_file->sections)
    {
        config_file_section_free (config_file, config_file->sections);
    }
    if (config_file->name)
        free (config_file->name);
    if (config_file->filename)
        free (config_file->filename);
    
    free (config_file);
    
    config_files = new_config_files;
}

/*
 * config_file_free_all: free all configuration files
 */

void
config_file_free_all ()
{
    while (config_files)
    {
        config_file_free (config_files);
    }
}

/*
 * config_file_free_all: free all configuration files for a plugin
 */

void
config_file_free_all_plugin (struct t_weechat_plugin *plugin)
{
    struct t_config_file *ptr_config, *next_config;
    
    ptr_config = config_files;
    while (ptr_config)
    {
        next_config = ptr_config->next_config;
        
        if (ptr_config->plugin == plugin)
            config_file_free (ptr_config);
        
        ptr_config = next_config;
    }
}

/*
 * config_file_add_to_infolist: add config options in an infolist
 *                              return 1 if ok, 0 if error
 */

int
config_file_add_to_infolist (struct t_infolist *infolist,
                             const char *option_name)
{
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    struct t_infolist_item *ptr_item;
    int length;
    char *option_full_name, value[128], *string_values;
    
    if (!infolist)
        return 0;
    
    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        for (ptr_section = ptr_config->sections; ptr_section;
             ptr_section = ptr_section->next_section)
        {
            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                length = strlen (ptr_config->name) + 1 +
                    strlen (ptr_section->name) + 1 +
                    strlen (ptr_option->name) + 1;
                option_full_name = malloc (length);
                if (option_full_name)
                {
                    snprintf (option_full_name, length, "%s.%s.%s",
                              ptr_config->name,
                              ptr_section->name,
                              ptr_option->name);
                    if (!option_name || !option_name[0]
                        || string_match (option_full_name, option_name, 0))
                    {
                        ptr_item = infolist_new_item (infolist);
                        if (!ptr_item)
                        {
                            free (option_full_name);
                            return 0;
                        }
                        if (!infolist_new_var_string (ptr_item,
                                                      "full_name",
                                                      option_full_name))
                        {
                            free (option_full_name);
                            return 0;
                        }
                        if (!infolist_new_var_string (ptr_item,
                                                      "config_name",
                                                      ptr_config->name))
                        {
                            free (option_full_name);
                            return 0;
                        }
                        if (!infolist_new_var_string (ptr_item,
                                                      "section_name",
                                                      ptr_section->name))
                        {
                            free (option_full_name);
                            return 0;
                        }
                        if (!infolist_new_var_string (ptr_item,
                                                      "option_name",
                                                      ptr_option->name))
                        {
                            free (option_full_name);
                            return 0;
                        }
                        if (!infolist_new_var_string (ptr_item,
                                                      "description",
                                                      ptr_option->description))
                        {
                            free (option_full_name);
                            return 0;
                        }
                        if (!infolist_new_var_string (ptr_item,
                                                      "description_nls",
                                                      (ptr_option->description
                                                       && ptr_option->description[0]) ?
                                                      _(ptr_option->description) : ""))
                        {
                            free (option_full_name);
                            return 0;
                        }
                        string_values = string_build_with_exploded (ptr_option->string_values,
                                                                    "|");
                        if (!infolist_new_var_string (ptr_item,
                                                      "string_values",
                                                      string_values))
                        {
                            if (string_values)
                                free (string_values);
                            free (option_full_name);
                            return 0;
                        }
                        if (string_values)
                            free (string_values);
                        if (!infolist_new_var_integer (ptr_item,
                                                       "min",
                                                       ptr_option->min))
                        {
                            free (option_full_name);
                            return 0;
                        }
                        if (!infolist_new_var_integer (ptr_item,
                                                       "max",
                                                       ptr_option->max))
                        {
                            free (option_full_name);
                            return 0;
                        }
                        switch (ptr_option->type)
                        {
                            case CONFIG_OPTION_TYPE_BOOLEAN:
                                if (!infolist_new_var_string (ptr_item,
                                                              "type",
                                                              "boolean"))
                                {
                                    free (option_full_name);
                                    return 0;
                                }
                                if (CONFIG_BOOLEAN(ptr_option) == CONFIG_BOOLEAN_TRUE)
                                    snprintf (value, sizeof (value), "on");
                                else
                                    snprintf (value, sizeof (value), "off");
                                if (!infolist_new_var_string (ptr_item,
                                                              "value",
                                                              value))
                                {
                                    free (option_full_name);
                                    return 0;
                                }
                                if (CONFIG_BOOLEAN_DEFAULT(ptr_option) == CONFIG_BOOLEAN_TRUE)
                                    snprintf (value, sizeof (value), "on");
                                else
                                    snprintf (value, sizeof (value), "off");
                                if (!infolist_new_var_string (ptr_item,
                                                              "default_value",
                                                              value))
                                {
                                    free (option_full_name);
                                    return 0;
                                }
                                break;
                            case CONFIG_OPTION_TYPE_INTEGER:
                                if (!infolist_new_var_string (ptr_item,
                                                              "type",
                                                              "integer"))
                                {
                                    free (option_full_name);
                                    return 0;
                                }
                                if (ptr_option->string_values)
                                {
                                    if (!infolist_new_var_string (ptr_item,
                                                                  "value",
                                                                  ptr_option->string_values[CONFIG_INTEGER(ptr_option)]))
                                    {
                                        free (option_full_name);
                                        return 0;
                                    }
                                    if (!infolist_new_var_string (ptr_item,
                                                                  "default_value",
                                                                  ptr_option->string_values[CONFIG_INTEGER_DEFAULT(ptr_option)]))
                                    {
                                        free (option_full_name);
                                        return 0;
                                    }
                                }
                                else
                                {
                                    snprintf (value, sizeof (value), "%d",
                                              CONFIG_INTEGER(ptr_option));
                                    if (!infolist_new_var_string (ptr_item,
                                                                  "value",
                                                                  value))
                                    {
                                        free (option_full_name);
                                        return 0;
                                    }
                                    snprintf (value, sizeof (value), "%d",
                                              CONFIG_INTEGER_DEFAULT(ptr_option));
                                    if (!infolist_new_var_string (ptr_item,
                                                                  "default_value",
                                                                  value))
                                    {
                                        free (option_full_name);
                                        return 0;
                                    }
                                }
                                break;
                            case CONFIG_OPTION_TYPE_STRING:
                                if (!infolist_new_var_string (ptr_item,
                                                              "type",
                                                              "string"))
                                {
                                    free (option_full_name);
                                    return 0;
                                }
                                if (!infolist_new_var_string (ptr_item,
                                                              "value",
                                                              CONFIG_STRING(ptr_option)))
                                {
                                    free (option_full_name);
                                    return 0;
                                }
                                if (!infolist_new_var_string (ptr_item,
                                                              "default_value",
                                                              CONFIG_STRING_DEFAULT(ptr_option)))
                                {
                                    free (option_full_name);
                                    return 0;
                                }
                                break;
                            case CONFIG_OPTION_TYPE_COLOR:
                                if (!infolist_new_var_string (ptr_item,
                                                              "type",
                                                              "color"))
                                {
                                    free (option_full_name);
                                    return 0;
                                }
                                if (!infolist_new_var_string (ptr_item,
                                                              "value",
                                                              gui_color_get_name (CONFIG_COLOR(ptr_option))))
                                {
                                    free (option_full_name);
                                    return 0;
                                }
                                if (!infolist_new_var_string (ptr_item,
                                                              "default_value",
                                                              gui_color_get_name (CONFIG_COLOR_DEFAULT(ptr_option))))
                                {
                                    free (option_full_name);
                                    return 0;
                                }
                                break;
                            case CONFIG_NUM_OPTION_TYPES:
                                break;
                        }
                    }
                    free (option_full_name);
                }
            }
        }
    }
    
    return 1;
}

/*
 * config_file_print_stdout: print options on standard output
 */

void
config_file_print_stdout (struct t_config_file *config_file)
{
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    char *color_name;
    int i;
    
    for (ptr_section = config_file->sections; ptr_section;
         ptr_section = ptr_section->next_section)
    {
        for (ptr_option = ptr_section->options; ptr_option;
             ptr_option = ptr_option->next_option)
        {
            string_iconv_fprintf (stdout,
                                  "* %s:\n",
                                  ptr_option->name);
            switch (ptr_option->type)
            {
                case CONFIG_OPTION_TYPE_BOOLEAN:
                    string_iconv_fprintf (stdout, _("  . type: boolean\n"));
                    string_iconv_fprintf (stdout, _("  . values: \"on\" or \"off\"\n"));
                    string_iconv_fprintf (stdout, _("  . default value: \"%s\"\n"),
                                          (CONFIG_BOOLEAN_DEFAULT(ptr_option) == CONFIG_BOOLEAN_TRUE) ?
                                          "on" : "off");
                    break;
                case CONFIG_OPTION_TYPE_INTEGER:
                    if (ptr_option->string_values)
                    {
                        string_iconv_fprintf (stdout, _("  . type: string\n"));
                        string_iconv_fprintf (stdout, _("  . values: "));
                        i = 0;
                        while (ptr_option->string_values[i])
                        {
                            string_iconv_fprintf (stdout, "\"%s\"",
                                                  ptr_option->string_values[i]);
                            if (ptr_option->string_values[i + 1])
                                string_iconv_fprintf (stdout, ", ");
                            i++;
                        }
                        string_iconv_fprintf (stdout, "\n");
                        string_iconv_fprintf (stdout, _("  . default value: \"%s\"\n"),
                                              ptr_option->string_values[CONFIG_INTEGER_DEFAULT(ptr_option)]);
                    }
                    else
                    {
                        string_iconv_fprintf (stdout, _("  . type: integer\n"));
                        string_iconv_fprintf (stdout, _("  . values: between %d and %d\n"),
                                              ptr_option->min,
                                              ptr_option->max);
                        string_iconv_fprintf (stdout, _("  . default value: %d\n"),
                                              CONFIG_INTEGER_DEFAULT(ptr_option));
                    }
                    break;
                case CONFIG_OPTION_TYPE_STRING:
                    switch (ptr_option->max)
                    {
                        case 0:
                            string_iconv_fprintf (stdout, _("  . type: string\n"));
                            string_iconv_fprintf (stdout, _("  . values: any string\n"));
                            break;
                        case 1:
                            string_iconv_fprintf (stdout, _("  . type: char\n"));
                            string_iconv_fprintf (stdout, _("  . values: any char\n"));
                            break;
                        default:
                            string_iconv_fprintf (stdout, _("  . type: string\n"));
                            string_iconv_fprintf (stdout, _("  . values: any string (limit: %d chars)\n"),
                                                  ptr_option->max);
                            break;
                    }
                    string_iconv_fprintf (stdout, _("  . default value: \"%s\"\n"),
                                          CONFIG_STRING_DEFAULT(ptr_option));
                    break;
                case CONFIG_OPTION_TYPE_COLOR:
                    color_name = gui_color_get_name (CONFIG_COLOR_DEFAULT(ptr_option));
                    string_iconv_fprintf (stdout, _("  . type: color\n"));
                    string_iconv_fprintf (stdout, _("  . values: color (depends on GUI used)\n"));
                    string_iconv_fprintf (stdout, _("  . default value: \"%s\"\n"),
                                          color_name);
                    break;
                case CONFIG_NUM_OPTION_TYPES:
                    break;
            }
            string_iconv_fprintf (stdout, _("  . description: %s\n"),
                                  _(ptr_option->description));
            string_iconv_fprintf (stdout, "\n");
        }
    }
}

/*
 * config_file_print_log: print config in log (usually for crash dump)
 */

void
config_file_print_log ()
{
    struct t_config_file *ptr_config_file;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    
    for (ptr_config_file = config_files; ptr_config_file;
         ptr_config_file = ptr_config_file->next_config)
    {
        log_printf ("");
        log_printf ("[config (addr:0x%x)]", ptr_config_file);
        log_printf ("  plugin . . . . . . . . : 0x%x", ptr_config_file->plugin);
        log_printf ("  name . . . . . . . . . : '%s'", ptr_config_file->name);
        log_printf ("  filename . . . . . . . : '%s'", ptr_config_file->filename);
        log_printf ("  file . . . . . . . . . : 0x%x", ptr_config_file->file);
        log_printf ("  callback_reload. . . . : 0x%x", ptr_config_file->callback_reload);
        log_printf ("  callback_reload_data . : 0x%x", ptr_config_file->callback_reload_data);
        log_printf ("  sections . . . . . . . : 0x%x", ptr_config_file->sections);
        log_printf ("  last_section . . . . . : 0x%x", ptr_config_file->last_section);
        log_printf ("  prev_config. . . . . . : 0x%x", ptr_config_file->prev_config);
        log_printf ("  next_config. . . . . . : 0x%x", ptr_config_file->next_config);
        
        for (ptr_section = ptr_config_file->sections; ptr_section;
             ptr_section = ptr_section->next_section)
        {
            log_printf ("");
            log_printf ("    [section (addr:0x%x)]", ptr_section);
            log_printf ("      name . . . . . . . . . . . : '%s'", ptr_section->name);
            log_printf ("      callback_read. . . . . . . : 0x%x", ptr_section->callback_read);
            log_printf ("      callback_read_data . . . . : 0x%x", ptr_section->callback_read_data);
            log_printf ("      callback_write . . . . . . : 0x%x", ptr_section->callback_write);
            log_printf ("      callback_write_data. . . . : 0x%x", ptr_section->callback_write_data);
            log_printf ("      callback_write_default . . : 0x%x", ptr_section->callback_write_default);
            log_printf ("      callback_write_default_data: 0x%x", ptr_section->callback_write_default_data);
            log_printf ("      callback_create_option. . .: 0x%x", ptr_section->callback_create_option);
            log_printf ("      callback_create_option_data: 0x%x", ptr_section->callback_create_option_data);
            log_printf ("      options. . . . . . . . . . : 0x%x", ptr_section->options);
            log_printf ("      last_option. . . . . . . . : 0x%x", ptr_section->last_option);
            log_printf ("      prev_section . . . . . . . : 0x%x", ptr_section->prev_section);
            log_printf ("      next_section . . . . . . . : 0x%x", ptr_section->next_section);
            
            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                log_printf ("");
                log_printf ("      [option (addr:0x%x)]", ptr_option);
                log_printf ("        config_file. . . . . : 0x%x", ptr_option->config_file);
                log_printf ("        section. . . . . . . : 0x%x", ptr_option->section);
                log_printf ("        name . . . . . . . . : '%s'", ptr_option->name);
                log_printf ("        type . . . . . . . . : %d",   ptr_option->type);
                log_printf ("        description. . . . . : '%s'", ptr_option->description);
                log_printf ("        string_values. . . . : 0x%x", ptr_option->string_values);
                log_printf ("        min. . . . . . . . . : %d",   ptr_option->min);
                log_printf ("        max. . . . . . . . . : %d",   ptr_option->max);
                switch (ptr_option->type)
                {
                    case CONFIG_OPTION_TYPE_BOOLEAN:
                        log_printf ("        default value. . . . : %s",
                                    (CONFIG_BOOLEAN_DEFAULT(ptr_option) == CONFIG_BOOLEAN_TRUE) ?
                                    "on" : "off");
                        log_printf ("        value (boolean). . . : %s",
                                    (CONFIG_BOOLEAN(ptr_option) == CONFIG_BOOLEAN_TRUE) ?
                                    "on" : "off");
                        break;
                    case CONFIG_OPTION_TYPE_INTEGER:
                        if (ptr_option->string_values)
                        {
                            log_printf ("        default value. . . . : '%s'",
                                        ptr_option->string_values[CONFIG_INTEGER_DEFAULT(ptr_option)]);
                            log_printf ("        value (integer/str). : '%s'",
                                        ptr_option->string_values[CONFIG_INTEGER(ptr_option)]);
                        }
                        else
                        {
                            log_printf ("        default value. . . . : %d",
                                        CONFIG_INTEGER_DEFAULT(ptr_option));
                            log_printf ("        value (integer). . . : %d",
                                        CONFIG_INTEGER(ptr_option));
                        }
                        break;
                    case CONFIG_OPTION_TYPE_STRING:
                        log_printf ("        default value. . . . : '%s'",
                                    CONFIG_STRING_DEFAULT(ptr_option));
                        log_printf ("        value (string) . . . : '%s'",
                                    CONFIG_STRING(ptr_option));
                        break;
                    case CONFIG_OPTION_TYPE_COLOR:
                        log_printf ("        default value. . . . : %d ('%s')",
                                    CONFIG_COLOR_DEFAULT(ptr_option),
                                    gui_color_get_name (CONFIG_COLOR_DEFAULT(ptr_option)));
                        log_printf ("        value (color). . . . : %d ('%s')",
                                    CONFIG_COLOR(ptr_option),
                                    gui_color_get_name (CONFIG_COLOR(ptr_option)));
                        break;
                    case CONFIG_NUM_OPTION_TYPES:
                        break;
                }
                log_printf ("        callback_change. . . : 0x%x", ptr_option->callback_change);
                log_printf ("        loaded . . . . . . . : %d",   ptr_option->loaded);
                log_printf ("        prev_option. . . . . : 0x%x", ptr_option->prev_option);
                log_printf ("        next_option. . . . . : 0x%x", ptr_option->next_option);
            }
        }
    }
}
